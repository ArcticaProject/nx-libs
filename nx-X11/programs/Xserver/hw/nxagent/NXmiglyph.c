/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2011 NoMachine (http://www.nomachine.com)          */
/* Copyright (c) 2008-2014 Oleksandr Shneyder <o.shneyder@phoca-gmbh.de>  */
/* Copyright (c) 2011-2016 Mike Gabriel <mike.gabriel@das-netzwerkteam.de>*/
/* Copyright (c) 2014-2016 Mihai Moldovan <ionic@ionic.de>                */
/* Copyright (c) 2014-2016 Ulrich Sibiller <uli42@gmx.de>                 */
/* Copyright (c) 2015-2016 Qindel Group (http://www.qindel.com)           */
/*                                                                        */
/* NXAGENT, NX protocol compression and NX extensions to this software    */
/* are copyright of the aforementioned persons and companies.             */
/*                                                                        */
/* Redistribution and use of the present software is allowed according    */
/* to terms specified in the file LICENSE which comes in the source       */
/* distribution.                                                          */
/*                                                                        */
/* All rights reserved.                                                   */
/*                                                                        */
/* NOTE: This software has received contributions from various other      */
/* contributors, only the core maintainers and supporters are listed as   */
/* copyright holders. Please contact us, if you feel you should be listed */
/* as copyright holder, as well.                                          */
/*                                                                        */
/**************************************************************************/

/*
 * $XFree86: xc/programs/Xserver/render/miglyph.c,v 1.4 2000/11/20 07:13:13 keithp Exp $
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

#include "Render.h"

#include "../../render/miglyph.c"

void
miGlyphs (CARD8		op,
	  PicturePtr	pSrc,
	  PicturePtr	pDst,
	  PictFormatPtr	maskFormat,
	  INT16		xSrc,
	  INT16		ySrc,
	  int		nlist,
	  GlyphListPtr	list,
	  GlyphPtr	*glyphs)
{
    PixmapPtr	pPixmap = 0;
    PicturePtr	pPicture;
    PixmapPtr   pMaskPixmap = 0;
    PicturePtr  pMask;
    ScreenPtr   pScreen = pDst->pDrawable->pScreen;
    int		width = 0, height = 0;
    int		x, y;
    int		xDst = list->xOff, yDst = list->yOff;
    int		n;
    GlyphPtr	glyph;
    int		error;
    BoxRec	extents;
    CARD32	component_alpha;

    /*
     * Get rid of the warning.
     */

    extents.x1 = 0;
    extents.y1 = 0;

    if (maskFormat)
    {
	GCPtr	    pGC;
	xRectangle  rect;

        if (nxagentGlyphsExtents != NullBox)
        {
          memcpy(&extents, nxagentGlyphsExtents, sizeof(BoxRec));
        }
        else
        {
          nxagentGlyphsExtents = (BoxPtr) malloc(sizeof(BoxRec));

          miGlyphExtents (nlist, list, glyphs, &extents);

          memcpy(nxagentGlyphsExtents, &extents, sizeof(BoxRec));
        }

	if (extents.x2 <= extents.x1 || extents.y2 <= extents.y1)
	    return;
	width = extents.x2 - extents.x1;
	height = extents.y2 - extents.y1;
	pMaskPixmap = (*pScreen->CreatePixmap) (pScreen, width, height, maskFormat->depth);

	if (!pMaskPixmap)
	    return;

	component_alpha = NeedsComponent(maskFormat->format);
	pMask = CreatePicture (0, &pMaskPixmap->drawable,
			       maskFormat, CPComponentAlpha, &component_alpha,
			       serverClient, &error);

	if (!pMask)
	{
	    (*pScreen->DestroyPixmap) (pMaskPixmap);
	    return;
	}
	pGC = GetScratchGC (pMaskPixmap->drawable.depth, pScreen);
	ValidateGC (&pMaskPixmap->drawable, pGC);
	rect.x = 0;
	rect.y = 0;
	rect.width = width;
	rect.height = height;
	(*pGC->ops->PolyFillRect) (&pMaskPixmap->drawable, pGC, 1, &rect);
	FreeScratchGC (pGC);
	x = -extents.x1;
	y = -extents.y1;
    }
    else
    {
	pMask = pDst;
	x = 0;
	y = 0;
    }
    pPicture = 0;
    while (nlist--)
    {
	x += list->xOff;
	y += list->yOff;
	n = list->len;

	while (n--)
	{
	    glyph = *glyphs++;
	    if (!pPicture)
	    {
		pPixmap = GetScratchPixmapHeader (pScreen, glyph->info.width, glyph->info.height, 
						  list->format->depth,
						  list->format->depth, 
						  0, (void *) (glyph + 1));
		if (!pPixmap)
		    return;
		component_alpha = NeedsComponent(list->format->format);
		pPicture = CreatePicture (0, &pPixmap->drawable, list->format,
					  CPComponentAlpha, &component_alpha, 
					  serverClient, &error);
		if (!pPicture)
		{
		    FreeScratchPixmapHeader (pPixmap);
		    return;
		}
	    }
	    (*pScreen->ModifyPixmapHeader) (pPixmap, 
					    glyph->info.width, glyph->info.height,
					    0, 0, -1, (void *) (glyph + 1));

            /*
             * The following line fixes a problem with glyphs that appeared
             * as clipped. It was a side effect due the validate function
             * "ValidatePicture" that makes a check on the Drawable serial
             * number instead of the picture serial number, failing thus
             * the clip mask update.
             */

            pPicture->pDrawable->serialNumber = NEXT_SERIAL_NUMBER;

	    pPixmap->drawable.serialNumber = NEXT_SERIAL_NUMBER;
	    if (maskFormat)
	    {
		CompositePicture (PictOpAdd,
				  pPicture,
				  None,
				  pMask,
				  0, 0,
				  0, 0,
				  x - glyph->info.x,
				  y - glyph->info.y,
				  glyph->info.width,
				  glyph->info.height);
	    }
	    else
	    {
		CompositePicture (op,
				  pSrc,
				  pPicture,
				  pDst,
				  xSrc + (x - glyph->info.x) - xDst,
				  ySrc + (y - glyph->info.y) - yDst,
				  0, 0,
				  x - glyph->info.x,
				  y - glyph->info.y,
				  glyph->info.width,
				  glyph->info.height);
	    }
	    x += glyph->info.xOff;
	    y += glyph->info.yOff;
	}

	list++;
	if (pPicture)
	{
	    FreeScratchPixmapHeader (pPixmap);
	    FreePicture ((void *) pPicture, 0);
	    pPicture = 0;
	    pPixmap = 0;
	}
    }
    if (maskFormat)
    {
	x = extents.x1;
	y = extents.y1;
	CompositePicture (op,
			  pSrc,
			  pMask,
			  pDst,
			  xSrc + x - xDst,
			  ySrc + y - yDst,
			  0, 0,
			  x, y,
			  width, height);

	FreePicture ((void *) pMask, (XID) 0);
	(*pScreen->DestroyPixmap) (pMaskPixmap);
    }

}
