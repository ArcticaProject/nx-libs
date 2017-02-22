/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

#include "glheader.h"
#include "macros.h"
#include "mtypes.h"
#include "colormac.h"

#include "intel_screen.h"

#include "intel_span.h"
#include "intel_ioctl.h"
#include "swrast/swrast.h"


#define DBG 0

#define LOCAL_VARS						\
   intelContextPtr intel = INTEL_CONTEXT(ctx);                    \
   __DRIdrawablePrivate *dPriv = intel->driDrawable;		\
   intelScreenPrivate *intelScreen = intel->intelScreen;		\
   GLuint pitch = intelScreen->backPitch * intelScreen->cpp;	\
   GLuint height = dPriv->h;					\
   char *buf = (char *)(intel->drawMap +			\
			dPriv->x * intelScreen->cpp +		\
			dPriv->y * pitch);			\
   char *read_buf = (char *)(intel->readMap +			\
			     dPriv->x * intelScreen->cpp +	\
			     dPriv->y * pitch); 		\
   GLushort p;         						\
   (void) read_buf; (void) buf; (void) p

#define LOCAL_DEPTH_VARS					\
   intelContextPtr intel = INTEL_CONTEXT(ctx);                    \
   __DRIdrawablePrivate *dPriv = intel->driDrawable;		\
   intelScreenPrivate *intelScreen = intel->intelScreen;		\
   GLuint pitch = intelScreen->backPitch * intelScreen->cpp;	\
   GLuint height = dPriv->h;					\
   char *buf = (char *)(intelScreen->depth.map +			\
			dPriv->x * intelScreen->cpp +		\
			dPriv->y * pitch)

#define LOCAL_STENCIL_VARS LOCAL_DEPTH_VARS 

#define INIT_MONO_PIXEL(p,color)\
	 p = INTEL_PACKCOLOR565(color[0],color[1],color[2])

#define Y_FLIP(_y) (height - _y - 1)

#define HW_LOCK()

#define HW_UNLOCK()

/* 16 bit, 565 rgb color spanline and pixel functions
 */
#define WRITE_RGBA( _x, _y, r, g, b, a )				\
   *(GLushort *)(buf + _x*2 + _y*pitch)  = ( (((int)r & 0xf8) << 8) |	\
		                             (((int)g & 0xfc) << 3) |	\
		                             (((int)b & 0xf8) >> 3))
#define WRITE_PIXEL( _x, _y, p )  \
   *(GLushort *)(buf + _x*2 + _y*pitch) = p

#define READ_RGBA( rgba, _x, _y )				\
do {								\
   GLushort p = *(GLushort *)(read_buf + _x*2 + _y*pitch);	\
   rgba[0] = (((p >> 11) & 0x1f) * 255) / 31;			\
   rgba[1] = (((p >>  5) & 0x3f) * 255) / 63;			\
   rgba[2] = (((p >>  0) & 0x1f) * 255) / 31;			\
   rgba[3] = 255;						\
} while(0)

#define TAG(x) intel##x##_565
#include "spantmp.h"

/* 15 bit, 555 rgb color spanline and pixel functions
 */
#define WRITE_RGBA( _x, _y, r, g, b, a )			\
   *(GLushort *)(buf + _x*2 + _y*pitch)  = (((r & 0xf8) << 7) |	\
		                            ((g & 0xf8) << 3) |	\
                         		    ((b & 0xf8) >> 3))

#define WRITE_PIXEL( _x, _y, p )  \
   *(GLushort *)(buf + _x*2 + _y*pitch)  = p

#define READ_RGBA( rgba, _x, _y )				\
do {								\
   GLushort p = *(GLushort *)(read_buf + _x*2 + _y*pitch);	\
   rgba[0] = (p >> 7) & 0xf8;					\
   rgba[1] = (p >> 3) & 0xf8;					\
   rgba[2] = (p << 3) & 0xf8;					\
   rgba[3] = 255;						\
} while(0)

#define TAG(x) intel##x##_555
#include "spantmp.h"

/* 16 bit depthbuffer functions.
 */
#define WRITE_DEPTH( _x, _y, d ) \
   *(GLushort *)(buf + (_x)*2 + (_y)*pitch)  = d;

#define READ_DEPTH( d, _x, _y )	\
   d = *(GLushort *)(buf + (_x)*2 + (_y)*pitch);	 


#define TAG(x) intel##x##_16
#include "depthtmp.h"


#undef LOCAL_VARS
#define LOCAL_VARS						\
   intelContextPtr intel = INTEL_CONTEXT(ctx);			\
   __DRIdrawablePrivate *dPriv = intel->driDrawable;		\
   intelScreenPrivate *intelScreen = intel->intelScreen;		\
   GLuint pitch = intelScreen->backPitch * intelScreen->cpp;	\
   GLuint height = dPriv->h;					\
   char *buf = (char *)(intel->drawMap +			\
			dPriv->x * intelScreen->cpp +		\
			dPriv->y * pitch);			\
   char *read_buf = (char *)(intel->readMap +			\
			     dPriv->x * intelScreen->cpp +	\
			     dPriv->y * pitch);			\
   GLuint p;							\
   (void) read_buf; (void) buf; (void) p

#undef INIT_MONO_PIXEL
#define INIT_MONO_PIXEL(p,color)\
	 p = INTEL_PACKCOLOR8888(color[0],color[1],color[2],color[3])

/* 32 bit, 8888 argb color spanline and pixel functions
 */
#define WRITE_RGBA(_x, _y, r, g, b, a)			\
    *(GLuint *)(buf + _x*4 + _y*pitch) = ((r << 16) |	\
					  (g << 8)  |	\
					  (b << 0)  |	\
					  (a << 24) )

#define WRITE_PIXEL(_x, _y, p)			\
    *(GLuint *)(buf + _x*4 + _y*pitch) = p


#define READ_RGBA(rgba, _x, _y)					\
    do {							\
	GLuint p = *(GLuint *)(read_buf + _x*4 + _y*pitch);	\
	rgba[0] = (p >> 16) & 0xff;				\
	rgba[1] = (p >> 8)  & 0xff;				\
	rgba[2] = (p >> 0)  & 0xff;				\
	rgba[3] = (p >> 24) & 0xff;				\
    } while (0)

#define TAG(x) intel##x##_8888
#include "spantmp.h"


/* 24/8 bit interleaved depth/stencil functions
 */
#define WRITE_DEPTH( _x, _y, d ) {			\
   GLuint tmp = *(GLuint *)(buf + (_x)*4 + (_y)*pitch);	\
   tmp &= 0xff000000;					\
   tmp |= (d) & 0xffffff;				\
   *(GLuint *)(buf + (_x)*4 + (_y)*pitch) = tmp;		\
}

#define READ_DEPTH( d, _x, _y )		\
   d = *(GLuint *)(buf + (_x)*4 + (_y)*pitch) & 0xffffff;


#define TAG(x) intel##x##_24_8
#include "depthtmp.h"

#define WRITE_STENCIL( _x, _y, d ) {			\
   GLuint tmp = *(GLuint *)(buf + (_x)*4 + (_y)*pitch);	\
   tmp &= 0xffffff;					\
   tmp |= ((d)<<24);					\
   *(GLuint *)(buf + (_x)*4 + (_y)*pitch) = tmp;		\
}

#define READ_STENCIL( d, _x, _y )			\
   d = *(GLuint *)(buf + (_x)*4 + (_y)*pitch) >> 24;

#define TAG(x) intel##x##_24_8
#include "stenciltmp.h"


/*
 * This function is called to specify which buffer to read and write
 * for software rasterization (swrast) fallbacks.  This doesn't necessarily
 * correspond to glDrawBuffer() or glReadBuffer() calls.
 */
static void intelSetBuffer(GLcontext *ctx, GLframebuffer *colorBuffer,
                          GLuint bufferBit)
{
   intelContextPtr intel = INTEL_CONTEXT(ctx);
   if (bufferBit == BUFFER_BIT_FRONT_LEFT) {
      intel->drawMap = (char *)intel->driScreen->pFB;
      intel->readMap = (char *)intel->driScreen->pFB;
   } else if (bufferBit == BUFFER_BIT_BACK_LEFT) {
      intel->drawMap = intel->intelScreen->back.map;
      intel->readMap = intel->intelScreen->back.map;
   } else {
      ASSERT(0);
   }
}


/* Move locking out to get reasonable span performance.
 */
void intelSpanRenderStart( GLcontext *ctx )
{
   intelContextPtr intel = INTEL_CONTEXT(ctx);

   intelFlush(&intel->ctx);
   LOCK_HARDWARE(intel);
   intelWaitForIdle(intel);
}

void intelSpanRenderFinish( GLcontext *ctx )
{
   intelContextPtr intel = INTEL_CONTEXT( ctx );
   _swrast_flush( ctx );
   UNLOCK_HARDWARE( intel );
}

void intelInitSpanFuncs( GLcontext *ctx )
{
   intelContextPtr intel = INTEL_CONTEXT(ctx);
   intelScreenPrivate *intelScreen = intel->intelScreen;

   struct swrast_device_driver *swdd = _swrast_GetDeviceDriverReference(ctx);

   swdd->SetBuffer = intelSetBuffer;

   switch (intelScreen->fbFormat) {
   case DV_PF_555:
#if 0
      swdd->WriteRGBASpan = intelWriteRGBASpan_555;
      swdd->WriteRGBSpan = intelWriteRGBSpan_555;
      swdd->WriteMonoRGBASpan = intelWriteMonoRGBASpan_555;
      swdd->WriteRGBAPixels = intelWriteRGBAPixels_555;
      swdd->WriteMonoRGBAPixels = intelWriteMonoRGBAPixels_555;
      swdd->ReadRGBASpan = intelReadRGBASpan_555;
      swdd->ReadRGBAPixels = intelReadRGBAPixels_555;
      swdd->ReadDepthSpan = intelReadDepthSpan_16;
      swdd->WriteDepthSpan = intelWriteDepthSpan_16;
      swdd->ReadDepthPixels = intelReadDepthPixels_16;
      swdd->WriteDepthPixels = intelWriteDepthPixels_16;
#endif
      break;

   case DV_PF_565:
#if 0
      swdd->WriteRGBASpan = intelWriteRGBASpan_565;
      swdd->WriteRGBSpan = intelWriteRGBSpan_565;
      swdd->WriteMonoRGBASpan = intelWriteMonoRGBASpan_565;
      swdd->WriteRGBAPixels = intelWriteRGBAPixels_565;
      swdd->WriteMonoRGBAPixels = intelWriteMonoRGBAPixels_565; 
      swdd->ReadRGBASpan = intelReadRGBASpan_565;
      swdd->ReadRGBAPixels = intelReadRGBAPixels_565;
      swdd->ReadDepthSpan = intelReadDepthSpan_16;
      swdd->WriteDepthSpan = intelWriteDepthSpan_16;
      swdd->ReadDepthPixels = intelReadDepthPixels_16;
      swdd->WriteDepthPixels = intelWriteDepthPixels_16;
#endif
      break;

   case DV_PF_8888:
#if 0
      swdd->WriteRGBASpan = intelWriteRGBASpan_8888;
      swdd->WriteRGBSpan = intelWriteRGBSpan_8888;
      swdd->WriteMonoRGBASpan = intelWriteMonoRGBASpan_8888;
      swdd->WriteRGBAPixels = intelWriteRGBAPixels_8888;
      swdd->WriteMonoRGBAPixels = intelWriteMonoRGBAPixels_8888;
      swdd->ReadRGBASpan = intelReadRGBASpan_8888;
      swdd->ReadRGBAPixels = intelReadRGBAPixels_8888;
      swdd->ReadDepthSpan = intelReadDepthSpan_24_8;
      swdd->WriteDepthSpan = intelWriteDepthSpan_24_8;
      swdd->ReadDepthPixels = intelReadDepthPixels_24_8;
      swdd->WriteDepthPixels = intelWriteDepthPixels_24_8;

      swdd->WriteStencilSpan = intelWriteStencilSpan_24_8;
      swdd->ReadStencilSpan = intelReadStencilSpan_24_8;
      swdd->WriteStencilPixels = intelWriteStencilPixels_24_8;
      swdd->ReadStencilPixels = intelReadStencilPixels_24_8;
#endif
      break;
   }

   swdd->SpanRenderStart = intelSpanRenderStart;
   swdd->SpanRenderFinish = intelSpanRenderFinish; 
}


/**
 * Plug in the Get/Put routines for the given driRenderbuffer.
 */
void
intelSetSpanFunctions(driRenderbuffer *drb, const GLvisual *vis)
{
   if (drb->Base.InternalFormat == GL_RGBA) {
      if (vis->redBits == 5 && vis->greenBits == 5 && vis->blueBits == 5) {
         drb->Base.GetRow        = intelReadRGBASpan_555;
         drb->Base.GetValues     = intelReadRGBAPixels_555;
         drb->Base.PutRow        = intelWriteRGBASpan_555;
         drb->Base.PutRowRGB     = intelWriteRGBSpan_555;
         drb->Base.PutMonoRow    = intelWriteMonoRGBASpan_555;
         drb->Base.PutValues     = intelWriteRGBAPixels_555;
         drb->Base.PutMonoValues = intelWriteMonoRGBAPixels_555;
      }
      else if (vis->redBits == 5 && vis->greenBits == 6 && vis->blueBits == 5) {
         drb->Base.GetRow        = intelReadRGBASpan_565;
         drb->Base.GetValues     = intelReadRGBAPixels_565;
         drb->Base.PutRow        = intelWriteRGBASpan_565;
         drb->Base.PutRowRGB     = intelWriteRGBSpan_565;
         drb->Base.PutMonoRow    = intelWriteMonoRGBASpan_565;
         drb->Base.PutValues     = intelWriteRGBAPixels_565;
         drb->Base.PutMonoValues = intelWriteMonoRGBAPixels_565;
      }
      else {
         assert(vis->redBits == 8);
         assert(vis->greenBits == 8);
         assert(vis->blueBits == 8);
         drb->Base.GetRow        = intelReadRGBASpan_8888;
         drb->Base.GetValues     = intelReadRGBAPixels_8888;
         drb->Base.PutRow        = intelWriteRGBASpan_8888;
         drb->Base.PutRowRGB     = intelWriteRGBSpan_8888;
         drb->Base.PutMonoRow    = intelWriteMonoRGBASpan_8888;
         drb->Base.PutValues     = intelWriteRGBAPixels_8888;
         drb->Base.PutMonoValues = intelWriteMonoRGBAPixels_8888;
      }
   }
   else if (drb->Base.InternalFormat == GL_DEPTH_COMPONENT16) {
      drb->Base.GetRow        = intelReadDepthSpan_16;
      drb->Base.GetValues     = intelReadDepthPixels_16;
      drb->Base.PutRow        = intelWriteDepthSpan_16;
      drb->Base.PutMonoRow    = intelWriteMonoDepthSpan_16;
      drb->Base.PutValues     = intelWriteDepthPixels_16;
      drb->Base.PutMonoValues = NULL;
   }
   else if (drb->Base.InternalFormat == GL_DEPTH_COMPONENT24) {
      drb->Base.GetRow        = intelReadDepthSpan_24_8;
      drb->Base.GetValues     = intelReadDepthPixels_24_8;
      drb->Base.PutRow        = intelWriteDepthSpan_24_8;
      drb->Base.PutMonoRow    = intelWriteMonoDepthSpan_24_8;
      drb->Base.PutValues     = intelWriteDepthPixels_24_8;
      drb->Base.PutMonoValues = NULL;
   }
   else if (drb->Base.InternalFormat == GL_STENCIL_INDEX8_EXT) {
      drb->Base.GetRow        = intelReadStencilSpan_24_8;
      drb->Base.GetValues     = intelReadStencilPixels_24_8;
      drb->Base.PutRow        = intelWriteStencilSpan_24_8;
      drb->Base.PutMonoRow    = intelWriteMonoStencilSpan_24_8;
      drb->Base.PutValues     = intelWriteStencilPixels_24_8;
      drb->Base.PutMonoValues = NULL;
   }
}
