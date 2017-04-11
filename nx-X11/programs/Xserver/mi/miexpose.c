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


#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <nx-X11/X.h>
#include <nx-X11/Xproto.h>
#include <nx-X11/Xprotostr.h>

#include "misc.h"
#include "regionstr.h"
#include "scrnintstr.h"
#include "gcstruct.h"
#include "windowstr.h"
#include "pixmap.h"
#include "input.h"

#include "dixstruct.h"
#include "mi.h"
#include <nx-X11/Xmd.h>

#include "globals.h"

#ifdef PANORAMIX
#include "panoramiX.h"
#include "panoramiXsrv.h"
#endif

/*
    machine-independent graphics exposure code.  any device that uses
the region package can call this.
*/

#ifndef RECTLIMIT
#define RECTLIMIT 25		/* pick a number, any number > 8 */
#endif

/* miHandleExposures 
    generate a region for exposures for areas that were copied from obscured or
non-existent areas to non-obscured areas of the destination.  Paint the
background for the region, if the destination is a window.

NOTE:
     this should generally be called, even if graphicsExposures is false,
because this is where bits get recovered from backing store.

NOTE:
     added argument 'plane' is used to indicate how exposures from backing
store should be accomplished. If plane is 0 (i.e. no bit plane), CopyArea
should be used, else a CopyPlane of the indicated plane will be used. The
exposing is done by the backing store's GraphicsExpose function, of course.

*/

#ifndef NXAGENT_SERVER
RegionPtr
miHandleExposures(pSrcDrawable, pDstDrawable,
		  pGC, srcx, srcy, width, height, dstx, dsty, plane)
    register DrawablePtr	pSrcDrawable;
    register DrawablePtr	pDstDrawable;
    GCPtr 			pGC;
    int 			srcx, srcy;
    int 			width, height;
    int 			dstx, dsty;
    unsigned long		plane;
{
    register ScreenPtr pscr;
    RegionPtr prgnSrcClip;	/* drawable-relative source clip */
    RegionRec rgnSrcRec;
    RegionPtr prgnDstClip;	/* drawable-relative dest clip */
    RegionRec rgnDstRec;
    BoxRec srcBox;		/* unclipped source */
    RegionRec rgnExposed;	/* exposed region, calculated source-
				   relative, made dst relative to
				   intersect with visible parts of
				   dest and send events to client, 
				   and then screen relative to paint 
				   the window background
				*/
    WindowPtr pSrcWin;
    BoxRec expBox;
    Bool extents;

    /* This prevents warning about pscr not being used. */
    pGC->pScreen = pscr = pGC->pScreen;

    /* avoid work if we can */
    if (!pGC->graphicsExposures &&
	(pDstDrawable->type == DRAWABLE_PIXMAP) &&
	((pSrcDrawable->type == DRAWABLE_PIXMAP) ||
	 (((WindowPtr)pSrcDrawable)->backStorage == NULL)))
	return NULL;
	
    srcBox.x1 = srcx;
    srcBox.y1 = srcy;
    srcBox.x2 = srcx+width;
    srcBox.y2 = srcy+height;

    if (pSrcDrawable->type != DRAWABLE_PIXMAP)
    {
	BoxRec TsrcBox;

	TsrcBox.x1 = srcx + pSrcDrawable->x;
	TsrcBox.y1 = srcy + pSrcDrawable->y;
	TsrcBox.x2 = TsrcBox.x1 + width;
	TsrcBox.y2 = TsrcBox.y1 + height;
	pSrcWin = (WindowPtr) pSrcDrawable;
	if (pGC->subWindowMode == IncludeInferiors)
 	{
	    prgnSrcClip = NotClippedByChildren (pSrcWin);
	    if ((RegionContainsRect(prgnSrcClip, &TsrcBox)) == rgnIN)
	    {
		RegionDestroy(prgnSrcClip);
		return NULL;
	    }
	}
 	else
 	{
	    if ((RegionContainsRect(&pSrcWin->clipList, &TsrcBox)) == rgnIN)
		return NULL;
	    prgnSrcClip = &rgnSrcRec;
	    RegionNull(prgnSrcClip);
	    RegionCopy(prgnSrcClip, &pSrcWin->clipList);
	}
	RegionTranslate(prgnSrcClip,
				-pSrcDrawable->x, -pSrcDrawable->y);
    }
    else
    {
	BoxRec	box;

	if ((srcBox.x1 >= 0) && (srcBox.y1 >= 0) &&
	    (srcBox.x2 <= pSrcDrawable->width) &&
 	    (srcBox.y2 <= pSrcDrawable->height))
	    return NULL;

	box.x1 = 0;
	box.y1 = 0;
	box.x2 = pSrcDrawable->width;
	box.y2 = pSrcDrawable->height;
	prgnSrcClip = &rgnSrcRec;
	RegionInit(prgnSrcClip, &box, 1);
	pSrcWin = (WindowPtr)NULL;
    }

    if (pDstDrawable == pSrcDrawable)
    {
	prgnDstClip = prgnSrcClip;
    }
    else if (pDstDrawable->type != DRAWABLE_PIXMAP)
    {
	if (pGC->subWindowMode == IncludeInferiors)
	{
	    prgnDstClip = NotClippedByChildren((WindowPtr)pDstDrawable);
	}
	else
	{
	    prgnDstClip = &rgnDstRec;
	    RegionNull(prgnDstClip);
	    RegionCopy(prgnDstClip,
				&((WindowPtr)pDstDrawable)->clipList);
	}
	RegionTranslate(prgnDstClip,
				 -pDstDrawable->x, -pDstDrawable->y);
    }
    else
    {
	BoxRec	box;

	box.x1 = 0;
	box.y1 = 0;
	box.x2 = pDstDrawable->width;
	box.y2 = pDstDrawable->height;
	prgnDstClip = &rgnDstRec;
	RegionInit(prgnDstClip, &box, 1);
    }

    /* drawable-relative source region */
    RegionInit(&rgnExposed, &srcBox, 1);

    /* now get the hidden parts of the source box*/
    RegionSubtract(&rgnExposed, &rgnExposed, prgnSrcClip);

    if (pSrcWin && pSrcWin->backStorage)
    {
	/*
	 * Copy any areas from the source backing store. Modifies
	 * rgnExposed.
	 */
	(* pSrcWin->drawable.pScreen->ExposeCopy) ((WindowPtr)pSrcDrawable,
					      pDstDrawable,
					      pGC,
					      &rgnExposed,
					      srcx, srcy,
					      dstx, dsty,
					      plane);
    }
    
    /* move them over the destination */
    RegionTranslate(&rgnExposed, dstx-srcx, dsty-srcy);

    /* intersect with visible areas of dest */
    RegionIntersect(&rgnExposed, &rgnExposed, prgnDstClip);

    /*
     * If we have LOTS of rectangles, we decide to take the extents
     * and force an exposure on that.  This should require much less
     * work overall, on both client and server.  This is cheating, but
     * isn't prohibited by the protocol ("spontaneous combustion" :-)
     * for windows.
     */
    extents = pGC->graphicsExposures &&
	      (RegionNumRects(&rgnExposed) > RECTLIMIT) &&
	      (pDstDrawable->type != DRAWABLE_PIXMAP);
#ifdef SHAPE
    if (pSrcWin)
    {
	RegionPtr	region;
    	if (!(region = wClipShape (pSrcWin)))
    	    region = wBoundingShape (pSrcWin);
    	/*
     	 * If you try to CopyArea the extents of a shaped window, compacting the
     	 * exposed region will undo all our work!
     	 */
    	if (extents && pSrcWin && region &&
	    (RegionContainsRect(region, &srcBox) != rgnIN))
	    	extents = FALSE;
    }
#endif
    if (extents)
    {
	WindowPtr pWin = (WindowPtr)pDstDrawable;

	expBox = *RegionExtents(&rgnExposed);
	RegionReset(&rgnExposed, &expBox);
	/* need to clear out new areas of backing store */
	if (pWin->backStorage)
	    (void) (* pWin->drawable.pScreen->ClearBackingStore)(
					 pWin,
					 expBox.x1,
					 expBox.y1,
					 expBox.x2 - expBox.x1,
					 expBox.y2 - expBox.y1,
					 FALSE);
    }
    if ((pDstDrawable->type != DRAWABLE_PIXMAP) &&
	(((WindowPtr)pDstDrawable)->backgroundState != None))
    {
	WindowPtr pWin = (WindowPtr)pDstDrawable;

	/* make the exposed area screen-relative */
	RegionTranslate(&rgnExposed,
				 pDstDrawable->x, pDstDrawable->y);

	if (extents)
	{
	    /* PaintWindowBackground doesn't clip, so we have to */
	    RegionIntersect(&rgnExposed, &rgnExposed, &pWin->clipList);
	}
	(*pWin->drawable.pScreen->PaintWindowBackground)(
			(WindowPtr)pDstDrawable, &rgnExposed, PW_BACKGROUND);

	if (extents)
	{
	    RegionReset(&rgnExposed, &expBox);
	}
	else
	    RegionTranslate(&rgnExposed,
				     -pDstDrawable->x, -pDstDrawable->y);
    }
    if (prgnDstClip == &rgnDstRec)
    {
	RegionUninit(prgnDstClip);
    }
    else if (prgnDstClip != prgnSrcClip)
    {
	RegionDestroy(prgnDstClip);
    }

    if (prgnSrcClip == &rgnSrcRec)
    {
	RegionUninit(prgnSrcClip);
    }
    else
    {
	RegionDestroy(prgnSrcClip);
    }

    if (pGC->graphicsExposures)
    {
	/* don't look */
	RegionPtr exposed = RegionCreate(NullBox, 0);
	*exposed = rgnExposed;
	return exposed;
    }
    else
    {
	RegionUninit(&rgnExposed);
	return NULL;
    }
}
#endif

/* send GraphicsExpose events, or a NoExpose event, based on the region */

void
miSendGraphicsExpose (client, pRgn, drawable, major, minor)
    ClientPtr	client;
    RegionPtr	pRgn;
    XID		drawable;
    int	major;
    int	minor;
{
    if (pRgn && !RegionNil(pRgn))
    {
        xEvent *pEvent;
	register xEvent *pe;
	register BoxPtr pBox;
	register int i;
	int numRects;

	numRects = RegionNumRects(pRgn);
	pBox = RegionRects(pRgn);
	if(!(pEvent = (xEvent *)malloc(numRects * sizeof(xEvent))))
		return;
	pe = pEvent;

	for (i=1; i<=numRects; i++, pe++, pBox++)
	{
	    pe->u.u.type = GraphicsExpose;
	    pe->u.graphicsExposure.drawable = drawable;
	    pe->u.graphicsExposure.x = pBox->x1;
	    pe->u.graphicsExposure.y = pBox->y1;
	    pe->u.graphicsExposure.width = pBox->x2 - pBox->x1;
	    pe->u.graphicsExposure.height = pBox->y2 - pBox->y1;
	    pe->u.graphicsExposure.count = numRects - i;
	    pe->u.graphicsExposure.majorEvent = major;
	    pe->u.graphicsExposure.minorEvent = minor;
	}
	TryClientEvents(client, pEvent, numRects,
			    (Mask)0, NoEventMask, NullGrab);
	free(pEvent);
    }
    else
    {
        xEvent event;
	memset(&event, 0, sizeof(xEvent));
	event.u.u.type = NoExpose;
	event.u.noExposure.drawable = drawable;
	event.u.noExposure.majorEvent = major;
	event.u.noExposure.minorEvent = minor;
	TryClientEvents(client, &event, 1,
	    (Mask)0, NoEventMask, NullGrab);
    }
}


void
miSendExposures(pWin, pRgn, dx, dy)
    WindowPtr pWin;
    RegionPtr pRgn;
    register int dx, dy;
{
    register BoxPtr pBox;
    int numRects;
    register xEvent *pEvent, *pe;
    register int i;

    pBox = RegionRects(pRgn);
    numRects = RegionNumRects(pRgn);
    if(!(pEvent = (xEvent *) malloc(numRects * sizeof(xEvent))))
	return;
    memset(pEvent, 0, numRects * sizeof(xEvent));

    for (i=numRects, pe = pEvent; --i >= 0; pe++, pBox++)
    {
	pe->u.u.type = Expose;
	pe->u.expose.window = pWin->drawable.id;
	pe->u.expose.x = pBox->x1 - dx;
	pe->u.expose.y = pBox->y1 - dy;
	pe->u.expose.width = pBox->x2 - pBox->x1;
	pe->u.expose.height = pBox->y2 - pBox->y1;
	pe->u.expose.count = i;
    }

#ifdef PANORAMIX
    if(!noPanoramiXExtension) {
	int scrnum = pWin->drawable.pScreen->myNum;
	int x = 0, y = 0;
	XID realWin = 0;

	if(!pWin->parent) {
	    x = panoramiXdataPtr[scrnum].x;
	    y = panoramiXdataPtr[scrnum].y;
	    pWin = screenInfo.screens[0]->root;
	    realWin = pWin->drawable.id;
	} else if (scrnum) {
	    PanoramiXRes *win;
	    win = PanoramiXFindIDByScrnum(XRT_WINDOW, 
			pWin->drawable.id, scrnum);
	    if(!win) {
		free(pEvent);
		return;
	    }
	    realWin = win->info[0].id;
	    pWin = LookupIDByType(realWin, RT_WINDOW);
	}
	if(x || y || scrnum)
	  for (i = 0; i < numRects; i++) {
	      pEvent[i].u.expose.window = realWin;
	      pEvent[i].u.expose.x += x;
	      pEvent[i].u.expose.y += y;
	  }
    }
#endif

    DeliverEvents(pWin, pEvent, numRects, NullWindow);

    free(pEvent);
}

#ifndef NXAGENT_SERVER
void 
miWindowExposures(pWin, prgn, other_exposed)
    WindowPtr pWin;
    register RegionPtr prgn, other_exposed;
{
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
	if (clientInterested && exposures && (RegionNumRects(exposures) > RECTLIMIT))
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
#endif

/*
    this code is highly unlikely.  it is not haile selassie.

    there is some hair here.  we can't just use the window's
clip region as it is, because if we are painting the border,
the border is not in the client area and so we will be excluded
when we validate the GC, and if we are painting a parent-relative
background, the area we want to paint is in some other window.
since we trust the code calling us to tell us to paint only areas
that are really ours, we will temporarily give the window a
clipList the size of the whole screen and an origin at (0,0).
this more or less assumes that ddX code will do translation
based on the window's absolute position, and that ValidateGC will
look at clipList, and that no other fields from the
window will be used.  it's not possible to just draw
in the root because it may be a different depth.

to get the tile to align correctly we set the GC's tile origin to
be the (x,y) of the window's upper left corner, after which we
get the right bits when drawing into the root.

because the clip_mask is being set to None, we may call DoChangeGC with
fPointer set true, thus we no longer need to install the background or
border tile in the resource table.
*/

static RESTYPE ResType = 0;
static int numGCs = 0;
static GCPtr	screenContext[MAXSCREENS];

/*ARGSUSED*/
static int
tossGC (
    void * value,
    XID id)
{
    GCPtr pGC = (GCPtr)value;
    screenContext[pGC->pScreen->myNum] = (GCPtr)NULL;
    FreeGC (pGC, id);
    numGCs--;
    if (!numGCs)
	ResType = 0;

    return 0;
}

void
miPaintWindow(WindowPtr pWin, RegionPtr prgn, int what)
{
    ScreenPtr	pScreen = pWin->drawable.pScreen;
    ChangeGCVal gcval[5];
    BITS32	gcmask;
    GCPtr	pGC;
    int		i;
    BoxPtr	pbox;
    xRectangle	*prect;
    int		numRects;
    /*
     * Distance from screen to destination drawable, use this
     * to adjust rendering coordinates which come in in screen space
     */
    int		draw_x_off, draw_y_off;
    /*
     * Tile offset for drawing; these need to align the tile
     * to the appropriate window origin
     */
    int		tile_x_off, tile_y_off;

    PixUnion   fill;
    Bool       solid = TRUE;
    DrawablePtr        drawable = &pWin->drawable;

    while (pWin->backgroundState == ParentRelative)
        pWin = pWin->parent;

    if (what == PW_BACKGROUND)
    {
	draw_x_off = drawable->x;
	draw_y_off = drawable->y;

	tile_x_off = 0;
	tile_y_off = 0;
	fill = pWin->background;
	switch (pWin->backgroundState) {
	    case None:
		return;
	    case BackgroundPixmap:
		solid = FALSE;
		break;
	}
    }
    else
    {
	PixmapPtr   pixmap;

	tile_x_off = drawable->x;
	tile_y_off = drawable->y;

	/* servers without pixmaps draw their own borders */
	if (!pScreen->GetWindowPixmap)
	    return;
	pixmap = (*pScreen->GetWindowPixmap) ((WindowPtr) drawable);
	drawable = &pixmap->drawable;

#ifdef COMPOSITE
	draw_x_off = pixmap->screen_x;
	draw_y_off = pixmap->screen_y;
	tile_x_off -= draw_x_off;
	tile_y_off -= draw_y_off;
#else
	draw_x_off = 0;
	draw_y_off = 0;
#endif
	fill = pWin->border;
	solid = pWin->borderIsPixel;
    }

    gcval[0].val = GXcopy;
    gcmask = GCFunction;

    if (solid)
    {
	gcval[1].val = fill.pixel;
	gcval[2].val  = FillSolid;
	gcmask |= GCForeground | GCFillStyle;
    }
    else
    {
	gcval[1].val = FillTiled;
	gcval[2].ptr = (pointer)fill.pixmap;
	gcval[3].val = tile_x_off;
	gcval[4].val = tile_y_off;
	gcmask |= GCFillStyle | GCTile | GCTileStipXOrigin | GCTileStipYOrigin;
    }

    prect = (xRectangle *)xallocarray(RegionNumRects(prgn), sizeof(xRectangle));
    if (!prect)
	return;

    pGC = GetScratchGC(drawable->depth, drawable->pScreen);
    if (!pGC)
    {
	free(prect);
	return;
    }

    dixChangeGC(NullClient, pGC, gcmask, NULL, gcval);
    ValidateGC(drawable, pGC);

    numRects = RegionNumRects(prgn);
    pbox = RegionRects(prgn);
    for (i= numRects; --i >= 0; pbox++, prect++)
    {
	prect->x = pbox->x1 - draw_x_off;
	prect->y = pbox->y1 - draw_y_off;
	prect->width = pbox->x2 - pbox->x1;
	prect->height = pbox->y2 - pbox->y1;
    }
    prect -= numRects;
    (*pGC->ops->PolyFillRect)(drawable, pGC, numRects, prect);
    free(prect);

    FreeScratchGC(pGC);
}

/* MICLEARDRAWABLE -- sets the entire drawable to the background color of
 * the GC.  Useful when we have a scratch drawable and need to initialize 
 * it. */
void
miClearDrawable(pDraw, pGC)
    DrawablePtr	pDraw;
    GCPtr	pGC;
{
    XID fg = pGC->fgPixel;
    XID bg = pGC->bgPixel;
    xRectangle rect;

    rect.x = 0;
    rect.y = 0;
    rect.width = pDraw->width;
    rect.height = pDraw->height;
    DoChangeGC(pGC, GCForeground, &bg, 0);
    ValidateGC(pDraw, pGC);
    (*pGC->ops->PolyFillRect)(pDraw, pGC, 1, &rect);
    DoChangeGC(pGC, GCForeground, &fg, 0);
    ValidateGC(pDraw, pGC);
}
