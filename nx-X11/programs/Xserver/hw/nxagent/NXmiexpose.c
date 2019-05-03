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

/***********************************************************

Copyright 1987, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.


Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/
/*****************************************************************

Copyright (c) 1991, 1997 Digital Equipment Corporation, Maynard, Massachusetts.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
DIGITAL EQUIPMENT CORPORATION BE LIABLE FOR ANY CLAIM, DAMAGES, INCLUDING,
BUT NOT LIMITED TO CONSEQUENTIAL OR INCIDENTAL DAMAGES, OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Digital Equipment Corporation
shall not be used in advertising or otherwise to promote the sale, use or other
dealings in this Software without prior written authorization from Digital
Equipment Corporation.

******************************************************************/


#include "Windows.h"

#include "../../mi/miexpose.c"

void 
miWindowExposures(pWin, prgn, other_exposed)
    WindowPtr pWin;
    register RegionPtr prgn, other_exposed;
{

    int total;

    RegionPtr   exposures = prgn;
    if (pWin->backStorage && prgn)
	/*
	 * in some cases, backing store will cause a different
	 * region to be exposed than needs to be repainted
	 * (like when a window is mapped).  RestoreAreas is
	 * allowed to return a region other than prgn,
	 * in which case this routine will free the resultant
	 * region.  If exposures is null, then no events will
	 * be sent to the client; if prgn is empty
	 * no areas will be repainted.
	 */
	exposures = (*pWin->drawable.pScreen->RestoreAreas)(pWin, prgn);
    if ((prgn && !RegionNil(prgn)) || 
	(exposures && !RegionNil(exposures)) || other_exposed)
    {
	RegionRec   expRec;
	int	    clientInterested;

	/*
	 * Restore from backing-store FIRST.
	 */
	clientInterested = (pWin->eventMask|wOtherEventMasks(pWin)) & ExposureMask;
	if (other_exposed)
	{
	    if (exposures)
	    {
		RegionUnion(other_exposed,
						  exposures,
					          other_exposed);
		if (exposures != prgn)
		    RegionDestroy(exposures);
	    }
	    exposures = other_exposed;
	}

        /*
         * If the number of rectangles is greater
         * than 4, let the function decide.
         */

        total = RegionNumRects(exposures);

        if (clientInterested && exposures && (total > RECTLIMIT ||
                (total > 4 && nxagentExtentsPredicate(total) == 1)))
	{
	    /*
	     * If we have LOTS of rectangles, we decide to take the extents
	     * and force an exposure on that.  This should require much less
	     * work overall, on both client and server.  This is cheating, but
	     * isn't prohibited by the protocol ("spontaneous combustion" :-).
	     */
	    BoxRec box;

	    box = *RegionExtents(exposures);
	    if (exposures == prgn) {
		exposures = &expRec;
		RegionInit(exposures, &box, 1);
		RegionReset(prgn, &box);
	    } else {
		RegionReset(exposures, &box);
		RegionUnion(prgn, prgn, exposures);
	    }
	    /* PaintWindowBackground doesn't clip, so we have to */
	    RegionIntersect(prgn, prgn, &pWin->clipList);
	    /* need to clear out new areas of backing store, too */
	    if (pWin->backStorage)
		(void) (* pWin->drawable.pScreen->ClearBackingStore)(
					     pWin,
					     box.x1 - pWin->drawable.x,
					     box.y1 - pWin->drawable.y,
					     box.x2 - box.x1,
					     box.y2 - box.y1,
					     FALSE);
	}
	if (prgn && !RegionNil(prgn))
	    (*pWin->drawable.pScreen->PaintWindowBackground)(pWin, prgn, PW_BACKGROUND);
	if (clientInterested && exposures && !RegionNil(exposures))
	    miSendExposures(pWin, exposures,
			    pWin->drawable.x, pWin->drawable.y);
	if (exposures == &expRec)
	{
	    RegionUninit(exposures);
	}
	else if (exposures && exposures != prgn && exposures != other_exposed)
	    RegionDestroy(exposures);
	if (prgn)
	    RegionEmpty(prgn);
    }
    else if (exposures && exposures != prgn)
	RegionDestroy(exposures);
}

void
miPaintWindow(pWin, prgn, what)
register WindowPtr pWin;
RegionPtr prgn;
int what;
{
    int	status;

    Bool usingScratchGC = FALSE;
    WindowPtr pRoot;
	
#define FUNCTION	0
#define FOREGROUND	1
#define TILE		2
#define FILLSTYLE	3
#define ABSX		4
#define ABSY		5
#define CLIPMASK	6
#define SUBWINDOW	7
#define COUNT_BITS	8

    ChangeGCVal gcval[7];
    ChangeGCVal newValues [COUNT_BITS] = {{ 0 }};

    BITS32 gcmask, index, mask;
    RegionRec prgnWin;
    DDXPointRec oldCorner;
    BoxRec box = {0};
    WindowPtr	pBgWin;
    GCPtr pGC;
    register int i;
    register BoxPtr pbox;
    register ScreenPtr pScreen = pWin->drawable.pScreen;
    register xRectangle *prect;
    int numRects;

    /*
     * Set the elements reported by the compiler
     * as uninitialized.
     */

    prgnWin.extents.x1 = 0;
    prgnWin.extents.y1 = 0;
    prgnWin.extents.x2 = 0;
    prgnWin.extents.y2 = 0;

    prgnWin.data = NULL;

    oldCorner.x = 0;
    oldCorner.y = 0;

    gcmask = 0;

    if (what == PW_BACKGROUND)
    {
	switch (pWin->backgroundState) {
	case None:
	    return;
	case ParentRelative:
	    (*pWin->parent->drawable.pScreen->PaintWindowBackground)(pWin->parent, prgn, what);
	    return;
	case BackgroundPixel:
	    newValues[FOREGROUND].val = pWin->background.pixel;
	    newValues[FILLSTYLE].val  = FillSolid;
	    gcmask |= GCForeground | GCFillStyle;
	    break;
	case BackgroundPixmap:
	    newValues[TILE].ptr = (void *)pWin->background.pixmap;
	    newValues[FILLSTYLE].val = FillTiled;
	    gcmask |= GCTile | GCFillStyle | GCTileStipXOrigin | GCTileStipYOrigin;
	    break;
	}
    }
    else
    {
	if (pWin->borderIsPixel)
	{
	    newValues[FOREGROUND].val = pWin->border.pixel;
	    newValues[FILLSTYLE].val  = FillSolid;
	    gcmask |= GCForeground | GCFillStyle;
	}
	else
	{
	    newValues[TILE].ptr = (void *)pWin->border.pixmap;
	    newValues[FILLSTYLE].val = FillTiled;
	    gcmask |= GCTile | GCFillStyle | GCTileStipXOrigin | GCTileStipYOrigin;
	}
    }

    prect = (xRectangle *)calloc(RegionNumRects(prgn), sizeof(xRectangle));
    if (!prect)
	return;

    newValues[FUNCTION].val = GXcopy;
    gcmask |= GCFunction | GCClipMask;

    i = pScreen->myNum;
    pRoot = screenInfo.screens[i]->root;

    pBgWin = pWin;
    if (what == PW_BORDER)
    {
	while (pBgWin->backgroundState == ParentRelative)
	    pBgWin = pBgWin->parent;
    }

    if ((pWin->drawable.depth != pRoot->drawable.depth) ||
	(pWin->drawable.bitsPerPixel != pRoot->drawable.bitsPerPixel))
    {
	usingScratchGC = TRUE;
	pGC = GetScratchGC(pWin->drawable.depth, pWin->drawable.pScreen);
	if (!pGC)
	{
	    free(prect);
	    return;
	}
	/*
	 * mash the clip list so we can paint the border by
	 * mangling the window in place, pretending it
	 * spans the entire screen
	 */
	if (what == PW_BORDER)
	{
	    prgnWin = pWin->clipList;
	    oldCorner.x = pWin->drawable.x;
	    oldCorner.y = pWin->drawable.y;
	    pWin->drawable.x = pWin->drawable.y = 0;
	    box.x1 = 0;
	    box.y1 = 0;
	    box.x2 = pScreen->width;
	    box.y2 = pScreen->height;
	    RegionInit(&pWin->clipList, &box, 1);
	    pWin->drawable.serialNumber = NEXT_SERIAL_NUMBER;
	    newValues[ABSX].val = pBgWin->drawable.x;
	    newValues[ABSY].val = pBgWin->drawable.y;
	}
	else
	{
	    newValues[ABSX].val = 0;
	    newValues[ABSY].val = 0;
	}
    } else {
	/*
	 * draw the background to the root window
	 */
	if (screenContext[i] == (GCPtr)NULL)
	{
	    if (!ResType && !(ResType = CreateNewResourceType(tossGC)))
		return;
	    screenContext[i] = CreateGC((DrawablePtr)pWin, (BITS32) 0,
					(XID *)NULL, &status);
	    if (!screenContext[i]) {
		free(prect);
		return;
	    }
	    numGCs++;
	    if (!AddResource(FakeClientID(0), ResType,
			     (void *)screenContext[i]))
	    {
		free(prect);
		return;
	    }
	}
	pGC = screenContext[i];
	newValues[SUBWINDOW].val = IncludeInferiors;
	newValues[ABSX].val = pBgWin->drawable.x;
	newValues[ABSY].val = pBgWin->drawable.y;
	gcmask |= GCSubwindowMode;
	pWin = pRoot;
    }
    
    if (pWin->backStorage)
	(*pWin->drawable.pScreen->DrawGuarantee) (pWin, pGC, GuaranteeVisBack);

    mask = gcmask;
    gcmask = 0;
    i = 0;
    while (mask) {
    	index = lowbit (mask);
	mask &= ~index;
	switch (index) {
	case GCFunction:
	    if (pGC->alu != newValues[FUNCTION].val) {
		gcmask |= index;
		gcval[i++].val = newValues[FUNCTION].val;
	    }
	    break;
	case GCTileStipXOrigin:
	    if ( pGC->patOrg.x != newValues[ABSX].val) {
		gcmask |= index;
		gcval[i++].val = newValues[ABSX].val;
	    }
	    break;
	case GCTileStipYOrigin:
	    if ( pGC->patOrg.y != newValues[ABSY].val) {
		gcmask |= index;
		gcval[i++].val = newValues[ABSY].val;
	    }
	    break;
	case GCClipMask:
	    if ( pGC->clientClipType != CT_NONE) {
		gcmask |= index;
		gcval[i++].val = CT_NONE;
	    }
	    break;
	case GCSubwindowMode:
	    if ( pGC->subWindowMode != newValues[SUBWINDOW].val) {
		gcmask |= index;
		gcval[i++].val = newValues[SUBWINDOW].val;
	    }
	    break;
	case GCTile:
	    if (pGC->tileIsPixel || pGC->tile.pixmap != newValues[TILE].ptr)
 	    {
		gcmask |= index;
		gcval[i++].ptr = newValues[TILE].ptr;
	    }
	    break;
	case GCFillStyle:
	    if ( pGC->fillStyle != newValues[FILLSTYLE].val) {
		gcmask |= index;
		gcval[i++].val = newValues[FILLSTYLE].val;
	    }
	    break;
	case GCForeground:
	    if ( pGC->fgPixel != newValues[FOREGROUND].val) {
		gcmask |= index;
		gcval[i++].val = newValues[FOREGROUND].val;
	    }
	    break;
	}
    }

    if (gcmask)
        dixChangeGC(NullClient, pGC, gcmask, NULL, gcval);

    if (pWin->drawable.serialNumber != pGC->serialNumber)
	ValidateGC((DrawablePtr)pWin, pGC);

    numRects = RegionNumRects(prgn);
    pbox = RegionRects(prgn);
    for (i= numRects; --i >= 0; pbox++, prect++)
    {
	prect->x = pbox->x1 - pWin->drawable.x;
	prect->y = pbox->y1 - pWin->drawable.y;
	prect->width = pbox->x2 - pbox->x1;
	prect->height = pbox->y2 - pbox->y1;
    }
    prect -= numRects;
    (*pGC->ops->PolyFillRect)((DrawablePtr)pWin, pGC, numRects, prect);
    free(prect);

    if (pWin->backStorage)
	(*pWin->drawable.pScreen->DrawGuarantee) (pWin, pGC, GuaranteeNothing);

    if (usingScratchGC)
    {
	if (what == PW_BORDER)
	{
	    RegionUninit(&pWin->clipList);
	    pWin->clipList = prgnWin;
	    pWin->drawable.x = oldCorner.x;
	    pWin->drawable.y = oldCorner.y;
	    pWin->drawable.serialNumber = NEXT_SERIAL_NUMBER;
	}
	FreeScratchGC(pGC);
    }
}
