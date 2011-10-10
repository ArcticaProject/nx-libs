/*
 *
 * Copyright © 2000 SuSE, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of SuSE not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  SuSE makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * SuSE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL SuSE
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Keith Packard, SuSE, Inc.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "Xrenderint.h"

/* 
 * NX_RENDER_CLEANUP enables cleaning of padding bytes
 */

#define NX_RENDER_CLEANUP

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG
#undef  DUMP

#ifdef NX_RENDER_CLEANUP

#include <stdio.h>

#define ROUNDUP(nbits, pad) ((((nbits) + ((pad)-1)) / (pad)) * ((pad)>>3))

#endif /* NX_RENDER_CLEANUP */

GlyphSet
XRenderCreateGlyphSet (Display *dpy, _Xconst XRenderPictFormat *format)
{
    XRenderExtDisplayInfo		*info = XRenderFindDisplay (dpy);
    GlyphSet			gsid;
    xRenderCreateGlyphSetReq	*req;

    RenderCheckExtension (dpy, info, 0);
    LockDisplay(dpy);
    GetReq(RenderCreateGlyphSet, req);
    req->reqType = info->codes->major_opcode;
    req->renderReqType = X_RenderCreateGlyphSet;
    req->gsid = gsid = XAllocID(dpy);
    req->format = format->id;
    UnlockDisplay(dpy);
    SyncHandle();
    return gsid;
}

GlyphSet
XRenderReferenceGlyphSet (Display *dpy, GlyphSet existing)
{
    XRenderExtDisplayInfo             *info = XRenderFindDisplay (dpy);
    GlyphSet                    gsid;
    xRenderReferenceGlyphSetReq	*req;

    RenderCheckExtension (dpy, info, 0);
    LockDisplay(dpy);
    GetReq(RenderReferenceGlyphSet, req);
    req->reqType = info->codes->major_opcode;
    req->renderReqType = X_RenderReferenceGlyphSet;
    req->gsid = gsid = XAllocID(dpy);
    req->existing = existing;
    UnlockDisplay(dpy);
    SyncHandle();
    return gsid;
}

void
XRenderFreeGlyphSet (Display *dpy, GlyphSet glyphset)
{
    XRenderExtDisplayInfo         *info = XRenderFindDisplay (dpy);
    xRenderFreeGlyphSetReq  *req;

    RenderSimpleCheckExtension (dpy, info);
    LockDisplay(dpy);
    GetReq(RenderFreeGlyphSet, req);
    req->reqType = info->codes->major_opcode;
    req->renderReqType = X_RenderFreeGlyphSet;
    req->glyphset = glyphset;
    UnlockDisplay(dpy);
    SyncHandle();
}

#ifdef NX_RENDER_CLEANUP

void
XRenderCleanGlyphs(xGlyphInfo  *gi,
                        int         nglyphs,
                        CARD8       *images,
                        int         depth,
                        Display     *dpy)
{

  int widthInBits;
  int bytesPerLine;
  int bytesToClean;
  int bitsToClean;
  int widthInBytes;
  int height = gi -> height;
  register int i;
  int j;

  #ifdef DEBUG
  fprintf(stderr, "nxagentCleanGlyphs: Found a Glyph with Depth %d, width %d, pad %d.\n",
          depth, gi -> width, dpy -> bitmap_pad);
  #endif

  while (nglyphs > 0)
  {
    if (depth == 24)
    {
      widthInBits = gi -> width * 32;

      bytesPerLine = ROUNDUP(widthInBits, dpy -> bitmap_pad);

      bytesToClean = bytesPerLine * height;

      #ifdef DUBUG
      fprintf(stderr, "nxagentCleanGlyphs: Found glyph with depth 24, bytes to clean is %d"
              "width in bits is %d bytes per line [%d] height [%d].\n", bytesToClean,
                      widthInBits, bytesPerLine, height);
      #endif

      if (dpy -> byte_order == LSBFirst)
      {
        for (i = 3; i < bytesToClean; i += 4)
        {
          images[i] = 0x00;
        }
      }
      else
      {
        for (i = 0; i < bytesToClean; i += 4)
        {
          images[i] = 0x00;
        }
      }

      #ifdef DUMP
      fprintf(stderr, "nxagentCleanGlyphs: depth %d, bytesToClean %d, scanline: ", depth, bytesToClean);
      for (i = 0; i < bytesPerLine; i++)
      {
        fprintf(stderr, "[%d]", images[i]);
      }
      fprintf(stderr,"\n");
      #endif

      images += bytesToClean;

      gi++;

      nglyphs--;
    }
    else if (depth == 1)
    {
      widthInBits = gi -> width;

      bytesPerLine = ROUNDUP(widthInBits, dpy -> bitmap_pad);

      bitsToClean = (bytesPerLine << 3) - (gi -> width);

      #ifdef DEBUG
      fprintf(stderr, "nxagentCleanGlyphs: Found glyph with depth 1, width [%d], height [%d], bitsToClean [%d]," 
              " bytesPerLine [%d].\n", gi -> width, height, bitsToClean, bytesPerLine);
      #endif

      bytesToClean = bitsToClean >> 3;

      bitsToClean &= 7;

      #ifdef DEBUG
      fprintf(stderr, "nxagentCleanGlyphs: bitsToClean &=7 is %d, bytesToCLean is %d."
              " byte_order is %d, bitmap_bit_order is %d.\n", bitsToClean, bytesToClean,
              dpy -> byte_order, dpy -> bitmap_bit_order);
      #endif

      for (i = 1; i <= height; i++)
      {
        if (dpy -> byte_order == dpy -> bitmap_bit_order)
        {
          for (j = 1; j <= bytesToClean; j++)
          {
            images[i * bytesPerLine - j] = 0x00;

            #ifdef DEBUG
            fprintf(stderr, "nxagentCleanGlyphs: byte_order = bitmap_bit_orde, cleaning %d, i=%d, j=%d.\n"
                    , (i * bytesPerLine - j), i, j);
            #endif

          }
        }
        else
        {
          for (j = bytesToClean; j >= 1; j--)
          {
            images[i * bytesPerLine - j] = 0x00;

            #ifdef DEBUG
            fprintf(stderr, "nxagentCleanGlyphs: byte_order %d, bitmap_bit_order %d, cleaning %d, i=%d, j=%d.\n"
                    , dpy -> byte_order, dpy -> bitmap_bit_order, (i * bytesPerLine - j), i, j);
            #endif

          }
        }

        if (dpy -> bitmap_bit_order == MSBFirst)
        {
          images[i * bytesPerLine - j] &= 0xff << bitsToClean;

          #ifdef DEBUG
          fprintf(stderr, "nxagentCleanGlyphs: byte_order MSBFirst, cleaning %d, i=%d, j=%d.\n"
                  , (i * bytesPerLine - j), i, j);
          #endif
        }
        else
        {
          images[i * bytesPerLine - j] &= 0xff >> bitsToClean;

          #ifdef DEBUG
          fprintf(stderr, "nxagentCleanGlyphs: byte_order LSBFirst, cleaning %d, i=%d, j=%d.\n"
                  , (i * bytesPerLine - j), i, j);
          #endif
        }
      }
  
      #ifdef DUMP
      fprintf(stderr, "nxagentCleanGlyphs: depth %d, bytesToClean %d, scanline: ", depth, bytesToClean);
      for (i = 0; i < bytesPerLine; i++)
      {
        fprintf(stderr, "[%d]", images[i]);
      }
      fprintf(stderr,"\n");
      #endif

      images += bytesPerLine * height;

      gi++;

      nglyphs--;
    }
    else if ((depth == 8) || (depth == 16) )
    {
      widthInBits = gi -> width * depth;

      bytesPerLine = ROUNDUP(widthInBits, dpy -> bitmap_pad);

      widthInBytes = (widthInBits >> 3);

      bytesToClean = bytesPerLine - widthInBytes;

      #ifdef DEBUG
      fprintf(stderr, "nxagentCleanGlyphs: nglyphs is %d, width of glyph in bits is %d, in bytes is %d.\n",
              nglyphs, widthInBits, widthInBytes);

      fprintf(stderr, "nxagentCleanGlyphs: bytesPerLine is %d bytes, there are %d scanlines.\n", bytesPerLine, height);

      fprintf(stderr, "nxagentCleanGlyphs: Bytes to clean for each scanline are %d.\n", bytesToClean);
      #endif

      if (bytesToClean > 0)
      {
        while (height > 0)
        {
          i = bytesToClean;

          while (i > 0)
          {
            *(images + (bytesPerLine - i)) = 0;

            #ifdef DEBUG
            fprintf(stderr, "nxagentCleanGlyphs: cleaned a byte.\n");
            #endif

            i--;
          }

          #ifdef DUMP
          fprintf(stderr, "nxagentCleanGlyphs: depth %d, bytesToClean %d, scanline: ", depth, bytesToClean);
          for (i = 0; i < bytesPerLine; i++)
          {
            fprintf(stderr, "[%d]", images[i]);
          }
          fprintf(stderr,"\n");
          #endif

          images += bytesPerLine;

          height--;
        }
      }

      gi++;

      nglyphs--;

      #ifdef DEBUG
      fprintf(stderr, "nxagentCleanGlyphs: Breaking Out.\n");
      #endif
    }
    else if (depth == 32)
    {
      #ifdef DEBUG
      fprintf(stderr, "nxagentCleanGlyphs: Found glyph with depth 32.\n");
      #endif

      gi++;

      nglyphs--;
    }
    else
    {
      #ifdef WARNING
      fprintf(stderr, "nxagentCleanGlyphs: Unrecognized glyph, depth is not 8/16/24/32, it appears to be %d.\n",
              depth);
      #endif

      gi++;

      nglyphs--;
    }
  }
}

#endif /* #ifdef NX_RENDER_CLEANUP */

void
XRenderAddGlyphs (Display	*dpy,
		  GlyphSet	glyphset,
		  _Xconst Glyph		*gids,
		  _Xconst XGlyphInfo	*glyphs,
		  int		nglyphs,
		  _Xconst char		*images,
		  int		nbyte_images)
{
    XRenderExtDisplayInfo         *info = XRenderFindDisplay (dpy);
    xRenderAddGlyphsReq	    *req;
    long		    len;

    if (nbyte_images & 3)
	nbyte_images += 4 - (nbyte_images & 3);
    RenderSimpleCheckExtension (dpy, info);
    LockDisplay(dpy);
    GetReq(RenderAddGlyphs, req);
    req->reqType = info->codes->major_opcode;
    req->renderReqType = X_RenderAddGlyphs;
    req->glyphset = glyphset;
    req->nglyphs = nglyphs;
    len = (nglyphs * (SIZEOF (xGlyphInfo) + 4) + nbyte_images) >> 2;
    SetReqLen(req, len, len);
    Data32 (dpy, (long *) gids, nglyphs * 4);
    Data16 (dpy, (short *) glyphs, nglyphs * SIZEOF (xGlyphInfo));
    Data (dpy, images, nbyte_images);
    UnlockDisplay(dpy);
    SyncHandle();
}

void
XRenderFreeGlyphs (Display   *dpy,
		   GlyphSet  glyphset,
		   _Xconst Glyph     *gids,
		   int       nglyphs)
{
    XRenderExtDisplayInfo         *info = XRenderFindDisplay (dpy);
    xRenderFreeGlyphsReq    *req;
    long                    len;

    RenderSimpleCheckExtension (dpy, info);
    LockDisplay(dpy);
    GetReq(RenderFreeGlyphs, req);
    req->reqType = info->codes->major_opcode;
    req->renderReqType = X_RenderFreeGlyphs;
    req->glyphset = glyphset;
    len = nglyphs;
    SetReqLen(req, len, len);
    len <<= 2;
    Data32 (dpy, (long *) gids, len);
    UnlockDisplay(dpy);
    SyncHandle();
}
	   
void
XRenderCompositeString8 (Display	    *dpy,
			 int		    op,
			 Picture	    src,
			 Picture	    dst,
			 _Xconst XRenderPictFormat  *maskFormat,
			 GlyphSet	    glyphset,
			 int		    xSrc,
			 int		    ySrc,
			 int		    xDst,
			 int		    yDst,
			 _Xconst char	    *string,
			 int		    nchar)
{
    XRenderExtDisplayInfo		*info = XRenderFindDisplay (dpy);
    xRenderCompositeGlyphs8Req	*req;
    long			len;
    xGlyphElt			*elt;
    int				nbytes;

    if (!nchar)
	return;
    
    RenderSimpleCheckExtension (dpy, info);
    LockDisplay(dpy);

    GetReq(RenderCompositeGlyphs8, req);
    req->reqType = info->codes->major_opcode;
    req->renderReqType = X_RenderCompositeGlyphs8;
    req->op = op;
    req->src = src;
    req->dst = dst;
    req->maskFormat = maskFormat ? maskFormat->id : None;
    req->glyphset = glyphset;
    req->xSrc = xSrc;
    req->ySrc = ySrc;    

    /*
     * xGlyphElt must be aligned on a 32-bit boundary; this is
     * easily done by filling no more than 252 glyphs in each
     * bucket
     */
    
#define MAX_8 252

    len = SIZEOF(xGlyphElt) * ((nchar + MAX_8-1) / MAX_8) + nchar;
    
    req->length += (len + 3)>>2;  /* convert to number of 32-bit words */
    
    /* 
     * If the entire request does not fit into the remaining space in the
     * buffer, flush the buffer first.
     */

    if (dpy->bufptr + len > dpy->bufmax)
    	_XFlush (dpy);

    while(nchar > MAX_8)
    {
	nbytes = MAX_8 + SIZEOF(xGlyphElt);
	BufAlloc (xGlyphElt *, elt, nbytes);
	elt->len = MAX_8;
	elt->deltax = xDst;
	elt->deltay = yDst;
	xDst = 0;
	yDst = 0;
	memcpy ((char *) (elt + 1), string, MAX_8);
	nchar = nchar - MAX_8;
	string += MAX_8;
    }
	
    if (nchar)
    {
	nbytes = (nchar + SIZEOF(xGlyphElt) + 3) & ~3;
	BufAlloc (xGlyphElt *, elt, nbytes); 
	elt->len = nchar;
	elt->deltax = xDst;
	elt->deltay = yDst;
	memcpy ((char *) (elt + 1), string, nchar);
    }
#undef MAX_8
    
    UnlockDisplay(dpy);
    SyncHandle();
}
void
XRenderCompositeString16 (Display	    *dpy,
			  int		    op,
			  Picture	    src,
			  Picture	    dst,
			  _Xconst XRenderPictFormat *maskFormat,
			  GlyphSet	    glyphset,
			  int		    xSrc,
			  int		    ySrc,
			  int		    xDst,
			  int		    yDst,
			  _Xconst unsigned short    *string,
			  int		    nchar)
{
    XRenderExtDisplayInfo		*info = XRenderFindDisplay (dpy);
    xRenderCompositeGlyphs8Req	*req;
    long			len;
    xGlyphElt			*elt;
    int				nbytes;

    if (!nchar)
	return;
    
    RenderSimpleCheckExtension (dpy, info);
    LockDisplay(dpy);
    
    GetReq(RenderCompositeGlyphs16, req);
    req->reqType = info->codes->major_opcode;
    req->renderReqType = X_RenderCompositeGlyphs16;
    req->op = op;
    req->src = src;
    req->dst = dst;
    req->maskFormat = maskFormat ? maskFormat->id : None;
    req->glyphset = glyphset;
    req->xSrc = xSrc;
    req->ySrc = ySrc;    

#define MAX_16	254

    len = SIZEOF(xGlyphElt) * ((nchar + MAX_16-1) / MAX_16) + nchar * 2;
    
    req->length += (len + 3)>>2;  /* convert to number of 32-bit words */
    
    /* 
     * If the entire request does not fit into the remaining space in the
     * buffer, flush the buffer first.
     */

    if (dpy->bufptr + len > dpy->bufmax)
    	_XFlush (dpy);

    while(nchar > MAX_16)
    {
	nbytes = MAX_16 * 2 + SIZEOF(xGlyphElt);
	BufAlloc (xGlyphElt *, elt, nbytes);
	elt->len = MAX_16;
	elt->deltax = xDst;
	elt->deltay = yDst;
	xDst = 0;
	yDst = 0;
	memcpy ((char *) (elt + 1), (char *) string, MAX_16 * 2);
	nchar = nchar - MAX_16;
	string += MAX_16;
    }
	
    if (nchar)
    {
	nbytes = (nchar * 2 + SIZEOF(xGlyphElt) + 3) & ~3;
	BufAlloc (xGlyphElt *, elt, nbytes); 
	elt->len = nchar;
	elt->deltax = xDst;
	elt->deltay = yDst;
	memcpy ((char *) (elt + 1), (char *) string, nchar * 2);
    }
#undef MAX_16
    
    UnlockDisplay(dpy);
    SyncHandle();
}

void
XRenderCompositeString32 (Display	    *dpy,
			  int		    op,
			  Picture	    src,
			  Picture	    dst,
			  _Xconst XRenderPictFormat  *maskFormat,
			  GlyphSet	    glyphset,
			  int		    xSrc,
			  int		    ySrc,
			  int		    xDst,
			  int		    yDst,
			  _Xconst unsigned int	    *string,
			  int		    nchar)
{
    XRenderExtDisplayInfo		*info = XRenderFindDisplay (dpy);
    xRenderCompositeGlyphs8Req	*req;
    long			len;
    xGlyphElt			*elt;
    int				nbytes;

    if (!nchar)
	return;
    
    RenderSimpleCheckExtension (dpy, info);
    LockDisplay(dpy);
    
    GetReq(RenderCompositeGlyphs32, req);
    req->reqType = info->codes->major_opcode;
    req->renderReqType = X_RenderCompositeGlyphs32;
    req->op = op;
    req->src = src;
    req->dst = dst;
    req->maskFormat = maskFormat ? maskFormat->id : None;
    req->glyphset = glyphset;
    req->xSrc = xSrc;
    req->ySrc = ySrc;    

#define MAX_32	254

    len = SIZEOF(xGlyphElt) * ((nchar + MAX_32-1) / MAX_32) + nchar * 4;
    
    req->length += (len + 3)>>2;  /* convert to number of 32-bit words */
    
    /* 
     * If the entire request does not fit into the remaining space in the
     * buffer, flush the buffer first.
     */

    if (dpy->bufptr + len > dpy->bufmax)
    	_XFlush (dpy);

    while(nchar > MAX_32)
    {
	nbytes = MAX_32 * 4 + SIZEOF(xGlyphElt);
	BufAlloc (xGlyphElt *, elt, nbytes);
	elt->len = MAX_32;
	elt->deltax = xDst;
	elt->deltay = yDst;
	xDst = 0;
	yDst = 0;
	memcpy ((char *) (elt + 1), (char *) string, MAX_32 * 4);
	nchar = nchar - MAX_32;
	string += MAX_32;
    }
	
    if (nchar)
    {
	nbytes = nchar * 4 + SIZEOF(xGlyphElt);
	BufAlloc (xGlyphElt *, elt, nbytes); 
	elt->len = nchar;
	elt->deltax = xDst;
	elt->deltay = yDst;
	memcpy ((char *) (elt + 1), (char *) string, nchar * 4);
    }
#undef MAX_32
    
    UnlockDisplay(dpy);
    SyncHandle();
}

void
XRenderCompositeText8 (Display			    *dpy,
		       int			    op,
		       Picture			    src,
		       Picture			    dst,
		       _Xconst XRenderPictFormat    *maskFormat,
		       int			    xSrc,
		       int			    ySrc,
		       int			    xDst,
		       int			    yDst,
		       _Xconst XGlyphElt8	    *elts,
		       int			    nelt)
{
    XRenderExtDisplayInfo		*info = XRenderFindDisplay (dpy);
    xRenderCompositeGlyphs8Req	*req;
    GlyphSet			glyphset;
    long			len;
    long			elen;
    xGlyphElt			*elt;
    int				i;
    _Xconst char		*chars;
    int				nchars;

    #ifdef NX_RENDER_CLEANUP

    char 			tmpChar[4];
    int				bytes_to_clean;
    int				bytes_to_write;

    #endif /* NX_RENDER_CLEANUP */

    if (!nelt)
	return;
    
    RenderSimpleCheckExtension (dpy, info);
    LockDisplay(dpy);

    GetReq(RenderCompositeGlyphs8, req);
    req->reqType = info->codes->major_opcode;
    req->renderReqType = X_RenderCompositeGlyphs8;
    req->op = op;
    req->src = src;
    req->dst = dst;
    req->maskFormat = maskFormat ? maskFormat->id : None;
    req->glyphset = elts[0].glyphset;
    req->xSrc = xSrc;
    req->ySrc = ySrc;    

    /*
     * Compute the space necessary
     */
    len = 0;
    
#define MAX_8 252

    glyphset = elts[0].glyphset;
    for (i = 0; i < nelt; i++)
    {
	/*
	 * Check for glyphset change
	 */
	if (elts[i].glyphset != glyphset)
	{
	    glyphset = elts[i].glyphset;
	    len += (SIZEOF (xGlyphElt) + 4) >> 2;
	}
	nchars = elts[i].nchars;
	/*
	 * xGlyphElt must be aligned on a 32-bit boundary; this is
	 * easily done by filling no more than 252 glyphs in each
	 * bucket
	 */
	elen = SIZEOF(xGlyphElt) * ((nchars + MAX_8-1) / MAX_8) + nchars;
	len += (elen + 3) >> 2;
    }
    
    req->length += len;

    /*
     * Send the glyphs
     */
    glyphset = elts[0].glyphset;
    for (i = 0; i < nelt; i++)
    {
	/*
	 * Switch glyphsets
	 */
	if (elts[i].glyphset != glyphset)
	{
	    glyphset = elts[i].glyphset;
	    BufAlloc (xGlyphElt *, elt, SIZEOF (xGlyphElt));

	    #ifdef NX_RENDER_CLEANUP

	    elt->pad1 = 0;
	    elt->pad2 = 0;

	    #endif /* NX_RENDER_CLEANUP */

	    elt->len = 0xff;
	    elt->deltax = 0;
	    elt->deltay = 0;
	    Data32(dpy, &glyphset, 4);
	}
	nchars = elts[i].nchars;
	xDst = elts[i].xOff;
	yDst = elts[i].yOff;
	chars = elts[i].chars;
	while (nchars)
	{
	    int this_chars = nchars > MAX_8 ? MAX_8 : nchars;

	    BufAlloc (xGlyphElt *, elt, SIZEOF(xGlyphElt))

	    #ifdef NX_RENDER_CLEANUP

	    elt->pad1 = 0;
	    elt->pad2 = 0;

	    #endif /* NX_RENDER_CLEANUP */

	    elt->len = this_chars;
	    elt->deltax = xDst;
	    elt->deltay = yDst;
	    xDst = 0;
	    yDst = 0;

            #ifdef NX_RENDER_CLEANUP

            bytes_to_write = this_chars & ~3;

            bytes_to_clean = ((this_chars + 3) & ~3) - this_chars;

            #ifdef DEBUG
            fprintf(stderr, "XRenderCompositeText8: bytes_to_write %d, bytes_to_clean are %d,"
                        " this_chars %d.\n", bytes_to_write, bytes_to_clean, this_chars);
            #endif

            if (bytes_to_clean > 0)
            {
              if (bytes_to_write > 0)
               {
                #ifdef DEBUG
                fprintf(stderr, "XRenderCompositeText8: found %d clean bytes, bytes_to_clean are %d,"
                        " this_chars %d.\n", bytes_to_write, bytes_to_clean, this_chars);
                #endif

                Data (dpy, chars, bytes_to_write);
                chars += bytes_to_write;
              }

              bytes_to_write = this_chars % 4;
              memcpy (tmpChar, chars, bytes_to_write);
              chars += bytes_to_write;

              #ifdef DEBUG
              fprintf(stderr, "XRenderCompositeText8: last 32 bit, bytes_to_write are %d,"
                      " bytes_to_clean are %d, this_chars are %d.\n", bytes_to_write, bytes_to_clean, this_chars);
              #endif

              #ifdef DUMP
              fprintf(stderr, "XRenderCompositeText8: bytes_to_clean %d, ", bytes_to_clean);
              #endif

              while (bytes_to_clean > 0)
              {
                tmpChar[4 - bytes_to_clean] = 0;
                bytes_to_clean--;

                #ifdef DEBUG
                fprintf(stderr, "XRenderCompositeText8: Cleaned %d byte.\n", 4 - bytes_to_clean);
                #endif
              }

              Data (dpy, tmpChar, 4);
              nchars -= this_chars;

              #ifdef DUMP
              fprintf(stderr, "Data: ");
              for (i = 0; i < 4; i++)
              {
                fprintf(stderr, "[%d]", tmpChar[i]);
              }
              fprintf(stderr,"\n");
              #endif

              #ifdef DEBUG
              fprintf(stderr, "XRenderCompositeText8: nchars now is %d.\n", nchars);
              #endif

              continue;
            }

            #endif /* NX_RENDER_CLEANUP */

	    Data (dpy, chars, this_chars);
	    nchars -= this_chars;
	    chars += this_chars;
	}
    }
#undef MAX_8
    
    UnlockDisplay(dpy);
    SyncHandle();
}

void
XRenderCompositeText16 (Display			    *dpy,
			int			    op,
			Picture			    src,
			Picture			    dst,
			_Xconst XRenderPictFormat   *maskFormat,
			int			    xSrc,
			int			    ySrc,
			int			    xDst,
			int			    yDst,
			_Xconst XGlyphElt16	    *elts,
			int			    nelt)
{
    XRenderExtDisplayInfo		*info = XRenderFindDisplay (dpy);
    xRenderCompositeGlyphs16Req	*req;
    GlyphSet			glyphset;
    long			len;
    long			elen;
    xGlyphElt			*elt;
    int				i;
    _Xconst unsigned short    	*chars;
    int				nchars;

    #ifdef NX_RENDER_CLEANUP

    int				bytes_to_write;
    int				bytes_to_clean;
    char			tmpChar[4];

    #endif /* NX_RENDER_CLEANUP */

    if (!nelt)
	return;
    
    RenderSimpleCheckExtension (dpy, info);
    LockDisplay(dpy);

    GetReq(RenderCompositeGlyphs16, req);
    req->reqType = info->codes->major_opcode;
    req->renderReqType = X_RenderCompositeGlyphs16;
    req->op = op;
    req->src = src;
    req->dst = dst;
    req->maskFormat = maskFormat ? maskFormat->id : None;
    req->glyphset = elts[0].glyphset;
    req->xSrc = xSrc;
    req->ySrc = ySrc;    

    /*
     * Compute the space necessary
     */
    len = 0;
    
#define MAX_16	254

    glyphset = elts[0].glyphset;
    for (i = 0; i < nelt; i++)
    {
	/*
	 * Check for glyphset change
	 */
	if (elts[i].glyphset != glyphset)
	{
	    glyphset = elts[i].glyphset;
	    len += (SIZEOF (xGlyphElt) + 4) >> 2;
	}
	nchars = elts[i].nchars;
	/*
	 * xGlyphElt must be aligned on a 32-bit boundary; this is
	 * easily done by filling no more than 254 glyphs in each
	 * bucket
	 */
	elen = SIZEOF(xGlyphElt) * ((nchars + MAX_16-1) / MAX_16) + nchars * 2;
	len += (elen + 3) >> 2;
    }
    
    req->length += len;

    glyphset = elts[0].glyphset;
    for (i = 0; i < nelt; i++)
    {
	/*
	 * Switch glyphsets
	 */
	if (elts[i].glyphset != glyphset)
	{
	    glyphset = elts[i].glyphset;
	    BufAlloc (xGlyphElt *, elt, SIZEOF (xGlyphElt));

	    #ifdef NX_RENDER_CLEANUP

	    elt->pad1 = 0;
	    elt->pad2 = 0;

	    #endif /* NX_RENDER_CLEANUP */

	    elt->len = 0xff;
	    elt->deltax = 0;
	    elt->deltay = 0;
	    Data32(dpy, &glyphset, 4);
	}
	nchars = elts[i].nchars;
	xDst = elts[i].xOff;
	yDst = elts[i].yOff;
	chars = elts[i].chars;
	while (nchars)
	{
	    int this_chars = nchars > MAX_16 ? MAX_16 : nchars;
	    int this_bytes = this_chars * 2;

	    BufAlloc (xGlyphElt *, elt, SIZEOF(xGlyphElt))

	    #ifdef NX_RENDER_CLEANUP

	    elt->pad1 = 0;
	    elt->pad2 = 0;

	    #endif /* NX_RENDER_CLEANUP */

	    elt->len = this_chars;
	    elt->deltax = xDst;
	    elt->deltay = yDst;
	    xDst = 0;
	    yDst = 0;
	    
            #ifdef NX_RENDER_CLEANUP

            bytes_to_write = this_bytes & ~3;
            bytes_to_clean = ((this_bytes + 3) & ~3) - this_bytes;

            #ifdef DEBUG
            fprintf(stderr, "XRenderCompositeText16: this_chars %d, this_bytes %d.\n"
                    "bytes_to_write %d, bytes_to_clean are %d.\n", this_chars, this_bytes,
                    bytes_to_write, bytes_to_clean);
            #endif

            if (bytes_to_clean > 0)
            {
              if (bytes_to_write > 0)
              {
                Data16 (dpy, chars, bytes_to_write);

                /*
                 * Cast chars to avoid errors with pointer arithmetic.
                 */

                chars = (unsigned short *) ((char *) chars + bytes_to_write);
              }

              bytes_to_write = this_bytes % 4;
              memcpy (tmpChar, (char *) chars, bytes_to_write);
              chars = (unsigned short *) ((char *) chars + bytes_to_write);

              #ifdef DEBUG
              fprintf(stderr, "XRenderCompositeText16: last 32 bit, bytes_to_write are %d,"
                      " bytes_to_clean are %d.\n", bytes_to_write, bytes_to_clean);
              #endif

              while (bytes_to_clean > 0)
              {
                tmpChar[4 - bytes_to_clean] = 0;
                bytes_to_clean--;

                #ifdef DEBUG
                fprintf(stderr, "XRenderCompositeText16: Cleaned %d byte.\n", 4 - bytes_to_clean);
                #endif
              }

              Data16 (dpy, tmpChar, 4);
              nchars -= this_chars;

              #ifdef DEBUG
              fprintf(stderr, "XRenderCompositeText16: nchars now is %d.\n", nchars);
              #endif

              continue;
            }

            #endif /* NX_RENDER_CLEANUP */

	    Data16 (dpy, chars, this_bytes);
	    nchars -= this_chars;
	    chars += this_chars;
	}
    }
#undef MAX_16
    
    UnlockDisplay(dpy);
    SyncHandle();
}

void
XRenderCompositeText32 (Display			    *dpy,
			int			    op,
			Picture			    src,
			Picture			    dst,
			_Xconst XRenderPictFormat   *maskFormat,
			int			    xSrc,
			int			    ySrc,
			int			    xDst,
			int			    yDst,
			_Xconst XGlyphElt32	    *elts,
			int			    nelt)
{
    XRenderExtDisplayInfo		*info = XRenderFindDisplay (dpy);
    xRenderCompositeGlyphs32Req	*req;
    GlyphSet			glyphset;
    long			len;
    long			elen;
    xGlyphElt			*elt;
    int				i;
    _Xconst unsigned int    	*chars;
    int				nchars;

    if (!nelt)
	return;
    
    RenderSimpleCheckExtension (dpy, info);
    LockDisplay(dpy);

    
    GetReq(RenderCompositeGlyphs32, req);
    req->reqType = info->codes->major_opcode;
    req->renderReqType = X_RenderCompositeGlyphs32;
    req->op = op;
    req->src = src;
    req->dst = dst;
    req->maskFormat = maskFormat ? maskFormat->id : None;
    req->glyphset = elts[0].glyphset;
    req->xSrc = xSrc;
    req->ySrc = ySrc;    

    /*
     * Compute the space necessary
     */
    len = 0;

#define MAX_32	254
    
    glyphset = elts[0].glyphset;
    for (i = 0; i < nelt; i++)
    {
	/*
	 * Check for glyphset change
	 */
	if (elts[i].glyphset != glyphset)
	{
	    glyphset = elts[i].glyphset;
	    len += (SIZEOF (xGlyphElt) + 4) >> 2;
	}
	nchars = elts[i].nchars;
	elen = SIZEOF(xGlyphElt) * ((nchars + MAX_32) / MAX_32) + nchars *4;
	len += (elen + 3) >> 2;
    }
    
    req->length += len;

    glyphset = elts[0].glyphset;
    for (i = 0; i < nelt; i++)
    {
	/*
	 * Switch glyphsets
	 */
	if (elts[i].glyphset != glyphset)
	{
	    glyphset = elts[i].glyphset;
	    BufAlloc (xGlyphElt *, elt, SIZEOF (xGlyphElt));

	    #ifdef NX_RENDER_CLEANUP

	    elt->pad1 = 0;
	    elt->pad2 = 0;

	    #endif /* NX_RENDER_CLEANUP */

	    elt->len = 0xff;
	    elt->deltax = 0;
	    elt->deltay = 0;
	    Data32(dpy, &glyphset, 4);
	}
	nchars = elts[i].nchars;
	xDst = elts[i].xOff;
	yDst = elts[i].yOff;
	chars = elts[i].chars;
	while (nchars)
	{
	    int this_chars = nchars > MAX_32 ? MAX_32 : nchars;
	    int this_bytes = this_chars * 4;
	    BufAlloc (xGlyphElt *, elt, SIZEOF(xGlyphElt))

	    #ifdef NX_RENDER_CLEANUP

	    elt->pad1 = 0;
	    elt->pad2 = 0;

	    #endif /* NX_RENDER_CLEANUP */

	    elt->len = this_chars;
	    elt->deltax = xDst;
	    elt->deltay = yDst;
	    xDst = 0;
	    yDst = 0;

            #ifdef TEST
            fprintf(stderr, "XRenderCompositeText32: this_chars %d, this_bytes %d.\n",
                    this_chars, this_bytes);
            #endif

	    DataInt32 (dpy, chars, this_bytes);
	    nchars -= this_chars;
	    chars += this_chars;
	}
    }
#undef MAX_32
    
    UnlockDisplay(dpy);
    SyncHandle();
}
