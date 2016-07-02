/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2011 NoMachine, http://www.nomachine.com/.         */
/*                                                                        */
/* NXAGENT, NX protocol compression and NX extensions to this software    */
/* are copyright of NoMachine. Redistribution and use of the present      */
/* software is allowed according to terms specified in the file LICENSE   */
/* which comes in the source distribution.                                */
/*                                                                        */
/* Check http://www.nomachine.com/licensing.html for applicability.       */
/*                                                                        */
/* NX and NoMachine are trademarks of Medialogic S.p.A.                   */
/*                                                                        */
/* All rights reserved.                                                   */
/*                                                                        */
/**************************************************************************/

/*
 * $XFree86: xc/programs/Xserver/render/mitrap.c,v 1.8 2002/09/03 19:28:28 keithp Exp $
 *
 * Copyright © 2002 Keith Packard, member of The XFree86 Project, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include "Render.h"

#include "../../render/mitrap.c"

void
miTrapezoids (CARD8	    op,
	      PicturePtr    pSrc,
	      PicturePtr    pDst,
	      PictFormatPtr maskFormat,
	      INT16	    xSrc,
	      INT16	    ySrc,
	      int	    ntrap,
	      xTrapezoid    *traps)
{
    ScreenPtr		pScreen = pDst->pDrawable->pScreen;
    PictureScreenPtr    ps = GetPictureScreen(pScreen);

    /*
     * Check for solid alpha add
     */
    if (op == PictOpAdd && miIsSolidAlpha (pSrc))
    {
	for (; ntrap; ntrap--, traps++)
	    (*ps->RasterizeTrapezoid) (pDst, traps, 0, 0);
    } 
    else if (maskFormat)
    {
	PicturePtr	pPicture;
	BoxRec		bounds;
	INT16		xDst, yDst;
	INT16		xRel, yRel;
	
	xDst = traps[0].left.p1.x >> 16;
	yDst = traps[0].left.p1.y >> 16;

        if (nxagentTrapezoidExtents != NullBox)
        {
          memcpy(&bounds, nxagentTrapezoidExtents, sizeof(BoxRec));
        }
        else
        {
          nxagentTrapezoidExtents = (BoxPtr) malloc(sizeof(BoxRec));

          miTrapezoidBounds (ntrap, traps, &bounds);

          memcpy(nxagentTrapezoidExtents, &bounds, sizeof(BoxRec));
        }

	if (bounds.y1 >= bounds.y2 || bounds.x1 >= bounds.x2)
	    return;
	pPicture = miCreateAlphaPicture (pScreen, pDst, maskFormat,
					 bounds.x2 - bounds.x1,
					 bounds.y2 - bounds.y1);
	if (!pPicture)
	    return;
	for (; ntrap; ntrap--, traps++)
	    (*ps->RasterizeTrapezoid) (pPicture, traps, 
				       -bounds.x1, -bounds.y1);
	xRel = bounds.x1 + xSrc - xDst;
	yRel = bounds.y1 + ySrc - yDst;
	CompositePicture (op, pSrc, pPicture, pDst,
			  xRel, yRel, 0, 0, bounds.x1, bounds.y1,
			  bounds.x2 - bounds.x1,
			  bounds.y2 - bounds.y1);
	FreePicture (pPicture, 0);
    }
    else
    {
	if (pDst->polyEdge == PolyEdgeSharp)
	    maskFormat = PictureMatchFormat (pScreen, 1, PICT_a1);
	else
	    maskFormat = PictureMatchFormat (pScreen, 8, PICT_a8);
	for (; ntrap; ntrap--, traps++)
	    miTrapezoids (op, pSrc, pDst, maskFormat, xSrc, ySrc, 1, traps);
    }
}
