/**************************************************************************

Copyright (C) Tungsten Graphics 2002.  All Rights Reserved.
The Weather Channel, Inc. funded Tungsten Graphics to develop the
initial release of the Radeon 8500 driver under the XFree86
license. This notice must be preserved.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation on the rights to use, copy, modify, merge, publish,
distribute, sub license, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NON-INFRINGEMENT. IN NO EVENT SHALL ATI, VA LINUX SYSTEMS AND/OR THEIR
SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

**************************************************************************/

/**
 * \file
 *
 * \author Gareth Hughes <gareth@valinux.com>
 *
 * \author Kevin E. Martin <martin@valinux.com>
 */

#include <errno.h>

#include "glheader.h"
#include "imports.h"
#include "context.h"
#include "colormac.h"
#include "macros.h"
#include "simple_list.h"
#include "radeon_reg.h"		/* gets definition for usleep */
#include "r300_context.h"
#include "r300_state.h"
#include "r300_cmdbuf.h"
#include "radeon_ioctl.h"
#include "r300_tex.h"
#include "r300_ioctl.h"
#include <unistd.h>		/* for usleep() */

#ifdef USER_BUFFERS
#include "r300_mem.h"
#endif

/**
 * Destroy any device-dependent state associated with the texture.  This may
 * include NULLing out hardware state that points to the texture.
 */
void r300DestroyTexObj(r300ContextPtr rmesa, r300TexObjPtr t)
{
	if (RADEON_DEBUG & DEBUG_TEXTURE) {
		fprintf(stderr, "%s( %p, %p )\n", __FUNCTION__,
			(void *)t, (void *)t->base.tObj);
	}

	if (rmesa != NULL) {
		unsigned i;

		for (i = 0; i < rmesa->radeon.glCtx->Const.MaxTextureUnits; i++) {
			if (t == rmesa->state.texture.unit[i].texobj) {
				rmesa->state.texture.unit[i].texobj = NULL;
				/* This code below is meant to shorten state
				   pushed to the hardware by not programming
				   unneeded units.

				   This does not appear to be worthwhile on R300 */
#if 0
				remove_from_list(&rmesa->hw.tex[i]);
				make_empty_list(&rmesa->hw.tex[i]);
				remove_from_list(&rmesa->hw.cube[i]);
				make_empty_list(&rmesa->hw.cube[i]);
#endif
			}
		}
	}
}

/* ------------------------------------------------------------
 * Texture image conversions
 */

static void r300UploadGARTClientSubImage(r300ContextPtr rmesa,
					 r300TexObjPtr t,
					 struct gl_texture_image *texImage,
					 GLint hwlevel,
					 GLint x, GLint y,
					 GLint width, GLint height)
{
	const struct gl_texture_format *texFormat = texImage->TexFormat;
	GLuint srcPitch, dstPitch;
	int blit_format;
	int srcOffset;

	/*
	 * XXX it appears that we always upload the full image, not a subimage.
	 * I.e. x==0, y==0, width=texWidth, height=texWidth.  If this is ever
	 * changed, the src pitch will have to change.
	 */
	switch (texFormat->TexelBytes) {
	case 1:
		blit_format = R300_CP_COLOR_FORMAT_CI8;
		srcPitch = t->image[0][0].width * texFormat->TexelBytes;
		dstPitch = t->image[0][0].width * texFormat->TexelBytes;
		break;
	case 2:
		blit_format = R300_CP_COLOR_FORMAT_RGB565;
		srcPitch = t->image[0][0].width * texFormat->TexelBytes;
		dstPitch = t->image[0][0].width * texFormat->TexelBytes;
		break;
	case 4:
		blit_format = R300_CP_COLOR_FORMAT_ARGB8888;
		srcPitch = t->image[0][0].width * texFormat->TexelBytes;
		dstPitch = t->image[0][0].width * texFormat->TexelBytes;
		break;
	case 8:
	case 16:
		blit_format = R300_CP_COLOR_FORMAT_CI8;
		srcPitch = t->image[0][0].width * texFormat->TexelBytes;
		dstPitch = t->image[0][0].width * texFormat->TexelBytes;
		break;
	default:
		return;
	}

	t->image[0][hwlevel].data = texImage->Data;
	srcOffset = r300GartOffsetFromVirtual(rmesa, texImage->Data);

	assert(srcOffset != ~0);

	/* Don't currently need to cope with small pitches?
	 */
	width = texImage->Width;
	height = texImage->Height;

	if (texFormat->TexelBytes > 4) {
		width *= texFormat->TexelBytes;
	}

	r300EmitWait(rmesa, R300_WAIT_3D);

	r300EmitBlit(rmesa, blit_format,
		     srcPitch,
		     srcOffset,
		     dstPitch,
		     t->bufAddr,
		     x,
		     y,
		     t->image[0][hwlevel].x + x,
		     t->image[0][hwlevel].y + y, width, height);

	r300EmitWait(rmesa, R300_WAIT_2D);
}

static void r300UploadRectSubImage(r300ContextPtr rmesa,
				   r300TexObjPtr t,
				   struct gl_texture_image *texImage,
				   GLint x, GLint y, GLint width, GLint height)
{
	const struct gl_texture_format *texFormat = texImage->TexFormat;
	int blit_format, dstPitch, done;

	switch (texFormat->TexelBytes) {
	case 1:
		blit_format = R300_CP_COLOR_FORMAT_CI8;
		break;
	case 2:
		blit_format = R300_CP_COLOR_FORMAT_RGB565;
		break;
	case 4:
		blit_format = R300_CP_COLOR_FORMAT_ARGB8888;
		break;
	case 8:
	case 16:
		blit_format = R300_CP_COLOR_FORMAT_CI8;
		break;
	default:
		return;
	}

	t->image[0][0].data = texImage->Data;

	/* Currently don't need to cope with small pitches.
	 */
	width = texImage->Width;
	height = texImage->Height;
	dstPitch = t->pitch;

	if (texFormat->TexelBytes > 4) {
		width *= texFormat->TexelBytes;
	}

	if (rmesa->prefer_gart_client_texturing && texImage->IsClientData) {
		/* In this case, could also use GART texturing.  This is
		 * currently disabled, but has been tested & works.
		 */
		t->offset = r300GartOffsetFromVirtual(rmesa, texImage->Data);
		t->pitch = texImage->RowStride * texFormat->TexelBytes - 32;

		if (RADEON_DEBUG & DEBUG_TEXTURE)
			fprintf(stderr,
				"Using GART texturing for rectangular client texture\n");

		/* Release FB memory allocated for this image:
		 */
		/* FIXME This may not be correct as driSwapOutTextureObject sets
		 * FIXME dirty_images.  It may be fine, though.
		 */
		if (t->base.memBlock) {
			driSwapOutTextureObject((driTextureObject *) t);
		}
	} else if (texImage->IsClientData) {
		/* Data already in GART memory, with usable pitch.
		 */
		GLuint srcPitch;
		srcPitch = texImage->RowStride * texFormat->TexelBytes;
		r300EmitBlit(rmesa,
			     blit_format,
			     srcPitch,
			     r300GartOffsetFromVirtual(rmesa, texImage->Data),
			     dstPitch, t->bufAddr, 0, 0, 0, 0, width, height);
	} else {
		/* Data not in GART memory, or bad pitch.
		 */
		for (done = 0; done < height;) {
			struct r300_dma_region region;
			int lines =
			    MIN2(height - done, RADEON_BUFFER_SIZE / dstPitch);
			int src_pitch;
			char *tex;

			src_pitch = texImage->RowStride * texFormat->TexelBytes;

			tex = (char *)texImage->Data + done * src_pitch;

			memset(&region, 0, sizeof(region));
			r300AllocDmaRegion(rmesa, &region, lines * dstPitch,
					   1024);

			/* Copy texdata to dma:
			 */
			if (RADEON_DEBUG & DEBUG_TEXTURE)
				fprintf(stderr,
					"%s: src_pitch %d dst_pitch %d\n",
					__FUNCTION__, src_pitch, dstPitch);

			if (src_pitch == dstPitch) {
				memcpy(region.address + region.start, tex,
				       lines * src_pitch);
			} else {
				char *buf = region.address + region.start;
				int i;
				for (i = 0; i < lines; i++) {
					memcpy(buf, tex, src_pitch);
					buf += dstPitch;
					tex += src_pitch;
				}
			}

			r300EmitWait(rmesa, R300_WAIT_3D);

			/* Blit to framebuffer
			 */
			r300EmitBlit(rmesa,
				     blit_format,
				     dstPitch, GET_START(&region),
				     dstPitch | (t->tile_bits >> 16),
				     t->bufAddr, 0, 0, 0, done, width, lines);

			r300EmitWait(rmesa, R300_WAIT_2D);
#ifdef USER_BUFFERS
			r300_mem_use(rmesa, region.buf->id);
#endif

			r300ReleaseDmaRegion(rmesa, &region, __FUNCTION__);
			done += lines;
		}
	}
}

/**
 * Upload the texture image associated with texture \a t at the specified
 * level at the address relative to \a start.
 */
static void r300UploadSubImage(r300ContextPtr rmesa, r300TexObjPtr t,
			       GLint hwlevel,
			       GLint x, GLint y, GLint width, GLint height,
			       GLuint face)
{
	struct gl_texture_image *texImage = NULL;
	GLuint offset;
	GLint imageWidth, imageHeight;
	GLint ret;
	drm_radeon_texture_t tex;
	drm_radeon_tex_image_t tmp;
	const int level = hwlevel + t->base.firstLevel;

	if (RADEON_DEBUG & DEBUG_TEXTURE) {
		fprintf(stderr,
			"%s( %p, %p ) level/width/height/face = %d/%d/%d/%u\n",
			__FUNCTION__, (void *)t, (void *)t->base.tObj, level,
			width, height, face);
	}

	ASSERT(face < 6);

	/* Ensure we have a valid texture to upload */
	if ((hwlevel < 0) || (hwlevel >= RADEON_MAX_TEXTURE_LEVELS)) {
		_mesa_problem(NULL, "bad texture level in %s", __FUNCTION__);
		return;
	}

	texImage = t->base.tObj->Image[face][level];

	if (!texImage) {
		if (RADEON_DEBUG & DEBUG_TEXTURE)
			fprintf(stderr, "%s: texImage %d is NULL!\n",
				__FUNCTION__, level);
		return;
	}
	if (!texImage->Data) {
		if (RADEON_DEBUG & DEBUG_TEXTURE)
			fprintf(stderr, "%s: image data is NULL!\n",
				__FUNCTION__);
		return;
	}

	if (t->base.tObj->Target == GL_TEXTURE_RECTANGLE_NV) {
		assert(level == 0);
		assert(hwlevel == 0);
		if (RADEON_DEBUG & DEBUG_TEXTURE)
			fprintf(stderr, "%s: image data is rectangular\n",
				__FUNCTION__);
		r300UploadRectSubImage(rmesa, t, texImage, x, y, width, height);
		return;
	} else if (texImage->IsClientData) {
		if (RADEON_DEBUG & DEBUG_TEXTURE)
			fprintf(stderr,
				"%s: image data is in GART client storage\n",
				__FUNCTION__);
		r300UploadGARTClientSubImage(rmesa, t, texImage, hwlevel, x, y,
					     width, height);
		return;
	} else if (RADEON_DEBUG & DEBUG_TEXTURE)
		fprintf(stderr, "%s: image data is in normal memory\n",
			__FUNCTION__);

	imageWidth = texImage->Width;
	imageHeight = texImage->Height;

	offset = t->bufAddr + t->base.totalSize / 6 * face;

	if (RADEON_DEBUG & (DEBUG_TEXTURE | DEBUG_IOCTL)) {
		GLint imageX = 0;
		GLint imageY = 0;
		GLint blitX = t->image[face][hwlevel].x;
		GLint blitY = t->image[face][hwlevel].y;
		GLint blitWidth = t->image[face][hwlevel].width;
		GLint blitHeight = t->image[face][hwlevel].height;
		fprintf(stderr, "   upload image: %d,%d at %d,%d\n",
			imageWidth, imageHeight, imageX, imageY);
		fprintf(stderr, "   upload  blit: %d,%d at %d,%d\n",
			blitWidth, blitHeight, blitX, blitY);
		fprintf(stderr, "       blit ofs: 0x%07x level: %d/%d\n",
			(GLuint) offset, hwlevel, level);
	}

	t->image[face][hwlevel].data = texImage->Data;

	/* Init the DRM_RADEON_TEXTURE command / drm_radeon_texture_t struct.
	 * NOTE: we're always use a 1KB-wide blit and I8 texture format.
	 * We used to use 1, 2 and 4-byte texels and used to use the texture
	 * width to dictate the blit width - but that won't work for compressed
	 * textures. (Brian)
	 * NOTE: can't do that with texture tiling. (sroland)
	 */
	tex.offset = offset;
	tex.image = &tmp;
	/* copy (x,y,width,height,data) */
	memcpy(&tmp, &t->image[face][hwlevel], sizeof(tmp));

	if (texImage->TexFormat->TexelBytes > 4) {
		const int log2TexelBytes =
		    (3 + (texImage->TexFormat->TexelBytes >> 4));
		tex.format = RADEON_TXFORMAT_I8;	/* any 1-byte texel format */
		tex.pitch =
		    MAX2((texImage->Width * texImage->TexFormat->TexelBytes) /
			 64, 1);
		tex.height = imageHeight;
		tex.width = imageWidth << log2TexelBytes;
		tex.offset += (tmp.x << log2TexelBytes) & ~1023;
		tmp.x = tmp.x % (1024 >> log2TexelBytes);
		tmp.width = tmp.width << log2TexelBytes;
	} else if (texImage->TexFormat->TexelBytes) {
		/* use multi-byte upload scheme */
		tex.height = imageHeight;
		tex.width = imageWidth;
		switch (texImage->TexFormat->TexelBytes) {
		case 1:
			tex.format = RADEON_TXFORMAT_I8;
			break;
		case 2:
			tex.format = RADEON_TXFORMAT_AI88;
			break;
		case 4:
			tex.format = RADEON_TXFORMAT_ARGB8888;
			break;
		}
		tex.pitch =
		    MAX2((texImage->Width * texImage->TexFormat->TexelBytes) /
			 64, 1);
		tex.offset += tmp.x & ~1023;
		tmp.x = tmp.x % 1024;

		if (t->tile_bits & R300_TXO_MICRO_TILE) {
			/* need something like "tiled coordinates" ? */
			tmp.y = tmp.x / (tex.pitch * 128) * 2;
			tmp.x =
			    tmp.x % (tex.pitch * 128) / 2 /
			    texImage->TexFormat->TexelBytes;
			tex.pitch |= RADEON_DST_TILE_MICRO >> 22;
		} else {
			tmp.x = tmp.x >> (texImage->TexFormat->TexelBytes >> 1);
		}
#if 1
		if ((t->tile_bits & R300_TXO_MACRO_TILE) &&
		    (texImage->Width * texImage->TexFormat->TexelBytes >= 256)
		    && ((!(t->tile_bits & R300_TXO_MICRO_TILE)
			 && (texImage->Height >= 8))
			|| (texImage->Height >= 16))) {
			/* weird: R200 disables macro tiling if mip width is smaller than 256 bytes,
			   OR if height is smaller than 8 automatically, but if micro tiling is active
			   the limit is height 16 instead ? */
			tex.pitch |= RADEON_DST_TILE_MACRO >> 22;
		}
#endif
	} else {
		/* In case of for instance 8x8 texture (2x2 dxt blocks),
		   padding after the first two blocks is needed (only
		   with dxt1 since 2 dxt3/dxt5 blocks already use 32 Byte). */
		/* set tex.height to 1/4 since 1 "macropixel" (dxt-block)
		   has 4 real pixels. Needed so the kernel module reads
		   the right amount of data. */
		tex.format = RADEON_TXFORMAT_I8;	/* any 1-byte texel format */
		tex.pitch = (R300_BLIT_WIDTH_BYTES / 64);
		tex.height = (imageHeight + 3) / 4;
		tex.width = (imageWidth + 3) / 4;
		if ((t->format & R300_TX_FORMAT_DXT1) == R300_TX_FORMAT_DXT1) {
			tex.width *= 8;
		} else {
			tex.width *= 16;
		}
	}

	LOCK_HARDWARE(&rmesa->radeon);
	do {
		ret =
		    drmCommandWriteRead(rmesa->radeon.dri.fd,
					DRM_RADEON_TEXTURE, &tex,
					sizeof(drm_radeon_texture_t));
		if (ret) {
			if (RADEON_DEBUG & DEBUG_IOCTL)
				fprintf(stderr,
					"DRM_RADEON_TEXTURE:  again!\n");
			usleep(1);
		}
	} while (ret == -EAGAIN);

	UNLOCK_HARDWARE(&rmesa->radeon);

	if (ret) {
		fprintf(stderr, "DRM_RADEON_TEXTURE: return = %d\n", ret);
		fprintf(stderr, "   offset=0x%08x\n", offset);
		fprintf(stderr, "   image width=%d height=%d\n",
			imageWidth, imageHeight);
		fprintf(stderr, "    blit width=%d height=%d data=%p\n",
			t->image[face][hwlevel].width,
			t->image[face][hwlevel].height,
			t->image[face][hwlevel].data);
		_mesa_exit(-1);
	}
}

/**
 * Upload the texture images associated with texture \a t.  This might
 * require the allocation of texture memory.
 *
 * \param rmesa Context pointer
 * \param t Texture to be uploaded
 * \param face Cube map face to be uploaded.  Zero for non-cube maps.
 */

int r300UploadTexImages(r300ContextPtr rmesa, r300TexObjPtr t, GLuint face)
{
	const int numLevels = t->base.lastLevel - t->base.firstLevel + 1;

	if (t->image_override)
		return 0;

	if (RADEON_DEBUG & (DEBUG_TEXTURE | DEBUG_IOCTL)) {
		fprintf(stderr, "%s( %p, %p ) sz=%d lvls=%d-%d\n", __FUNCTION__,
			(void *)rmesa->radeon.glCtx, (void *)t->base.tObj,
			t->base.totalSize, t->base.firstLevel,
			t->base.lastLevel);
	}

	if (!t || t->base.totalSize == 0)
		return 0;

	if (RADEON_DEBUG & DEBUG_SYNC) {
		fprintf(stderr, "%s: Syncing\n", __FUNCTION__);
		radeonFinish(rmesa->radeon.glCtx);
	}

	LOCK_HARDWARE(&rmesa->radeon);

	if (t->base.memBlock == NULL) {
		int heap;

		heap = driAllocateTexture(rmesa->texture_heaps, rmesa->nr_heaps,
					  (driTextureObject *) t);
		if (heap == -1) {
			UNLOCK_HARDWARE(&rmesa->radeon);
			return -1;
		}

		/* Set the base offset of the texture image */
		t->bufAddr = rmesa->radeon.radeonScreen->texOffset[heap]
		    + t->base.memBlock->ofs;
		t->offset = t->bufAddr;

		if (!(t->base.tObj->Image[0][0]->IsClientData)) {
			/* hope it's safe to add that here... */
			t->offset |= t->tile_bits;
		}

		/* Mark this texobj as dirty on all units:
		 */
		t->dirty_state = TEX_ALL;
	}

	/* Let the world know we've used this memory recently.
	 */
	driUpdateTextureLRU((driTextureObject *) t);
	UNLOCK_HARDWARE(&rmesa->radeon);

	/* Upload any images that are new */
	if (t->base.dirty_images[face]) {
		int i;
		for (i = 0; i < numLevels; i++) {
			if ((t->base.
			     dirty_images[face] & (1 <<
						   (i + t->base.firstLevel))) !=
			    0) {
				r300UploadSubImage(rmesa, t, i, 0, 0,
						   t->image[face][i].width,
						   t->image[face][i].height,
						   face);
			}
		}
		t->base.dirty_images[face] = 0;
	}

	if (RADEON_DEBUG & DEBUG_SYNC) {
		fprintf(stderr, "%s: Syncing\n", __FUNCTION__);
		radeonFinish(rmesa->radeon.glCtx);
	}

	return 0;
}
