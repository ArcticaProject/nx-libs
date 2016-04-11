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

/* $XdotOrg: xc/programs/Xserver/dix/window.c,v 1.12 2005/07/03 08:53:38 daniels Exp $ */
/* $Xorg: window.c,v 1.4 2001/02/09 02:04:41 xorgcvs Exp $ */
/*

Copyright 1987, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from The Open Group.


Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,

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

*/

/* The panoramix components contained the following notice */
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

/* $XFree86: xc/programs/Xserver/dix/window.c,v 3.36 2003/11/14 23:52:50 torrey Exp $ */

#include "selection.h"

#include "Screen.h"
#include "Options.h"
#include "Atoms.h"
#include "Clipboard.h"
#include "Splash.h"
#include "Rootless.h"
#include "Composite.h"
#include "Drawable.h"
#include "Colormap.h"

/* prototypes (only MakeRootTile() required here) */

static void MakeRootTile(WindowPtr pWin);

#include "../../dix/window.c"

extern Bool nxagentWMIsRunning;
extern Bool nxagentScreenTrap;

/*
 * Set here the required log level.
 */

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

WindowPtr nxagentRootTileWindow;

void nxagentClearSplash(WindowPtr pW)
{
    ScreenPtr pScreen;

    pScreen = pW->drawable.pScreen;

    if (pW->backgroundState == BackgroundPixmap)
    {
      (*pScreen->DestroyPixmap)(pW->background.pixmap);
    }

    pW->backgroundState = BackgroundPixel;
    pW->background.pixel = nxagentLogoBlack;

    (*pScreen->ChangeWindowAttributes)(pW, CWBackPixmap|CWBackPixel);
}

static void
MakeRootTile(WindowPtr pWin)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;
    GCPtr pGC;
    unsigned char back[128];
    int len = BitmapBytePad(sizeof(long));
    register unsigned char *from, *to;
    register int i, j;

    pWin->background.pixmap = (*pScreen->CreatePixmap)(pScreen, 4, 4,
						    pScreen->rootDepth);

    pWin->backgroundState = BackgroundPixmap;
    pGC = GetScratchGC(pScreen->rootDepth, pScreen);
    if (!pWin->background.pixmap || !pGC)
	FatalError("could not create root tile");

    {
	CARD32 attributes[2];

	attributes[0] = pScreen->whitePixel;
	attributes[1] = pScreen->blackPixel;

	(void)ChangeGC(pGC, GCForeground | GCBackground, attributes);
    }

   ValidateGC((DrawablePtr)pWin->background.pixmap, pGC);

   from = (screenInfo.bitmapBitOrder == LSBFirst) ? _back_lsb : _back_msb;
   to = back;

   for (i = 4; i > 0; i--, from++)
	for (j = len; j > 0; j--)
	    *to++ = *from;

   if (blackRoot)
       bzero(back, sizeof(back));

   (*pGC->ops->PutImage)((DrawablePtr)pWin->background.pixmap, pGC, 1,
		    0, 0, len, 4, 0, XYBitmap, (char *)back);

   FreeScratchGC(pGC);

   nxagentRootTileWindow = pWin;
}

void
InitRootWindow(WindowPtr pWin)
{
    ScreenPtr pScreen;

    #ifdef TEST
    fprintf(stderr, "InitRootWindow: Called for window at [%p][%ld] with parent [%p].\n",
                (void *) pWin, nxagentWindowPriv(pWin)->window, (void *) pWin -> parent);
    #endif

    if (nxagentOption(Rootless))
    {
      #ifdef TEST
      fprintf(stderr, "InitRootWindow: Assigned agent root to window at [%p][%ld] with parent [%p].\n",
                  (void *) pWin, nxagentWindowPriv(pWin)->window, (void *) pWin -> parent);
      #endif

      nxagentRootlessWindow = pWin;
    }

    pScreen = pWin->drawable.pScreen;

    /*
     * A root window is created for each screen by main
     * and the pointer is saved in screenInfo.screens as
     * in the following snippet:
     *
     * for (i = 0; i < screenInfo.numScreens; i++)
     *          InitRootWindow(screenInfo.screens[i]->root);
     *
     * Our root window on the real display was already
     * created at the time the screen was opened, so it
     * is unclear how this window (or the other window,
     * if you prefer) fits in the big picture.
     */

    #ifdef TEST
    fprintf(stderr, "InitRootWindow: Going to create window as root at [%p][%ld] with parent [%p].\n",
                (void *) pWin, nxagentWindowPriv(pWin)->window, (void *) pWin -> parent);
    #endif

    if (!(*pScreen->CreateWindow)(pWin))
	return; /* XXX */

    #ifdef TEST
    fprintf(stderr, "InitRootWindow: Created window as root at [%p][%ld] with parent [%p].\n",
                (void *) pWin, nxagentWindowPriv(pWin)->window, (void *) pWin -> parent);
    #endif

    (*pScreen->PositionWindow)(pWin, 0, 0);

    pWin->cursorIsNone = FALSE;
    pWin->optional->cursor = rootCursor;
    rootCursor->refcnt++;
    pWin->backingStore = defaultBackingStore;
    pWin->forcedBS = (defaultBackingStore != NotUseful);

    #ifdef NXAGENT_SPLASH
    /* We SHOULD check for an error value here XXX */
    pWin -> background.pixel = pScreen -> blackPixel;
    (*pScreen->ChangeWindowAttributes)(pWin,
		       CWBackPixel|CWBorderPixel|CWCursor|CWBackingStore);
    #else
    (*pScreen->ChangeWindowAttributes)(pWin,
		       CWBackPixmap|CWBorderPixel|CWCursor|CWBackingStore);
    #endif

    MakeRootTile(pWin);

    /*
     * Map both the root and the default agent window.
     */

    #ifdef TEST
    fprintf(stderr, "InitRootWindow: Mapping default windows.\n");
    #endif

    nxagentInitAtoms(pWin);

    nxagentInitClipboard(pWin);

    nxagentMapDefaultWindows();

    nxagentRedirectDefaultWindows();

    #ifdef NXAGENT_ARTSD
    {
      char artsd_port[10];
      int nPort;
      extern void nxagentPropagateArtsdProperties(ScreenPtr pScreen, char *port);
      nPort = atoi(display) + 7000;
      sprintf(artsd_port,"%d", nPort);
      nxagentPropagateArtsdProperties(pScreen, artsd_port);
    }
    #endif
}

/*****
 *  DeleteWindow
 *	 Deletes child of window then window itself
 *	 If wid is None, don't send any events
 *****/

int
DeleteWindow(void * value, XID wid)
 {
    register WindowPtr pParent;
    register WindowPtr pWin = (WindowPtr)value;
    xEvent event;

    UnmapWindow(pWin, FALSE);

    CrushTree(pWin);

    pParent = pWin->parent;
    if (wid && pParent && SubStrSend(pWin, pParent))
    {
	memset(&event, 0, sizeof(xEvent));
	event.u.u.type = DestroyNotify;
	event.u.destroyNotify.window = pWin->drawable.id;
	DeliverEvents(pWin, &event, 1, NullWindow);		
    }

    FreeWindowResources(pWin);
    if (pParent)
    {
	if (pParent->firstChild == pWin)
	    pParent->firstChild = pWin->nextSib;
	if (pParent->lastChild == pWin)
	    pParent->lastChild = pWin->prevSib;
	if (pWin->nextSib)
	    pWin->nextSib->prevSib = pWin->prevSib;
	if (pWin->prevSib)
	    pWin->prevSib->nextSib = pWin->nextSib;
    }

    if (pWin -> optional &&
            pWin -> optional -> colormap &&
                pWin -> parent)
    {
      nxagentSetInstalledColormapWindows(pWin -> drawable.pScreen);
    }

    xfree(pWin);
    return Success;
}

/* XXX need to retile border on each window with ParentRelative origin */
void
ResizeChildrenWinSize(register WindowPtr pWin, int dx, int dy, int dw, int dh)
{
    register ScreenPtr pScreen;
    register WindowPtr pSib, pChild;
    Bool resized = (dw || dh);

    pScreen = pWin->drawable.pScreen;

    for (pSib = pWin->firstChild; pSib; pSib = pSib->nextSib)
    {
	if (resized && (pSib->winGravity > NorthWestGravity))
	{
	    int cwsx, cwsy;

	    cwsx = pSib->origin.x;
	    cwsy = pSib->origin.y;
	    GravityTranslate (cwsx, cwsy, cwsx - dx, cwsy - dy, dw, dh,
			pSib->winGravity, &cwsx, &cwsy);
	    if (cwsx != pSib->origin.x || cwsy != pSib->origin.y)
	    {
		xEvent event;

		event.u.u.type = GravityNotify;
		event.u.gravity.window = pSib->drawable.id;
		event.u.gravity.x = cwsx - wBorderWidth (pSib);
		event.u.gravity.y = cwsy - wBorderWidth (pSib);
		DeliverEvents (pSib, &event, 1, NullWindow);
		pSib->origin.x = cwsx;
		pSib->origin.y = cwsy;
	    }
	}
	pSib->drawable.x = pWin->drawable.x + pSib->origin.x;
	pSib->drawable.y = pWin->drawable.y + pSib->origin.y;
	SetWinSize (pSib);
	SetBorderSize (pSib);

        /*
         * Don't force X to move children. It will position them
         * according with gravity.
         *
         * (*pScreen->PositionWindow)(pSib, pSib->drawable.x, pSib->drawable.y);
         */

        /*
         * Update pSib privates, as this window is moved by X.
         */

        nxagentAddConfiguredWindow(pSib, CW_Update);

	if ( (pChild = pSib->firstChild) )
	{
	    while (1)
	    {
		pChild->drawable.x = pChild->parent->drawable.x +
				     pChild->origin.x;
		pChild->drawable.y = pChild->parent->drawable.y +
				     pChild->origin.y;
		SetWinSize (pChild);
		SetBorderSize (pChild);

                (*pScreen->PositionWindow)(pChild, pChild->drawable.x,
                                               pChild->drawable.y);

		if (pChild->firstChild)
		{
		    pChild = pChild->firstChild;
		    continue;
		}
		while (!pChild->nextSib && (pChild != pSib))
		    pChild = pChild->parent;
		if (pChild == pSib)
		    break;
		pChild = pChild->nextSib;
	    }
	}
    }
}

/*****
 * ConfigureWindow
 *****/

int
ConfigureWindow(register WindowPtr pWin, register Mask mask, XID *vlist, ClientPtr client)
{
#define RESTACK_WIN    0
#define MOVE_WIN       1
#define RESIZE_WIN     2
#define REBORDER_WIN   3
    register WindowPtr pSib = NullWindow;
    register WindowPtr pParent = pWin->parent;
    Window sibwid = 0;
    Mask index2, tmask;
    register XID *pVlist;
    short x,   y, beforeX, beforeY;
    unsigned short w = pWin->drawable.width,
		   h = pWin->drawable.height,
		   bw = pWin->borderWidth;
    int action, smode = Above;
#ifdef XAPPGROUP
    ClientPtr win_owner;
    ClientPtr ag_leader = NULL;
#endif
    xEvent event;

    if ((pWin->drawable.class == InputOnly) && (mask & IllegalInputOnlyConfigureMask))
	return(BadMatch);

    if ((mask & CWSibling) && !(mask & CWStackMode))
	return(BadMatch);

    pVlist = vlist;

    if (pParent)
    {
	x = pWin->drawable.x - pParent->drawable.x - (int)bw;
	y = pWin->drawable.y - pParent->drawable.y - (int)bw;
    }
    else
    {
	x = pWin->drawable.x;
	y = pWin->drawable.y;
    }
    beforeX = x;
    beforeY = y;
    action = RESTACK_WIN;	
    if ((mask & (CWX | CWY)) && (!(mask & (CWHeight | CWWidth))))
    {
	GET_INT16(CWX, x);
	GET_INT16(CWY, y);
	action = MOVE_WIN;
    }
	/* or should be resized */
    else if (mask & (CWX |  CWY | CWWidth | CWHeight))
    {
	GET_INT16(CWX, x);
	GET_INT16(CWY, y);
	GET_CARD16(CWWidth, w);
	GET_CARD16 (CWHeight, h);
	if (!w || !h)
	{
	    client->errorValue = 0;
	    return BadValue;
	}
	action = RESIZE_WIN;
    }
    tmask = mask & ~ChangeMask;
    while (tmask)
    {
	index2 = (Mask)lowbit (tmask);
	tmask &= ~index2;
	switch (index2)
	{
	  case CWBorderWidth:
	    GET_CARD16(CWBorderWidth, bw);
	    break;
	  case CWSibling:
	    sibwid = (Window ) *pVlist;
	    pVlist++;
	    pSib = (WindowPtr )SecurityLookupIDByType(client, sibwid,
						RT_WINDOW, SecurityReadAccess);
	    if (!pSib)
	    {
		client->errorValue = sibwid;
		return(BadWindow);
	    }
	    if (pSib->parent != pParent)
		return(BadMatch);
	    if (pSib == pWin)
		return(BadMatch);
	    break;
	  case CWStackMode:
	    GET_CARD8(CWStackMode, smode);
	    if ((smode != TopIf) && (smode != BottomIf) &&
		(smode != Opposite) && (smode != Above) && (smode != Below))
	    {
		client->errorValue = smode;
		return(BadValue);
	    }
	    break;
	  default:
	    client->errorValue = mask;
	    return(BadValue);
	}
    }
	/* root really can't be reconfigured, so just return */
    if (!pParent)
	return Success;

	/* Figure out if the window should be moved.  Doesnt
	   make the changes to the window if event sent */

    #ifdef TEST
    if (nxagentWindowTopLevel(pWin))
    {

      fprintf(stderr, "ConfigureWindow: pWin [%p] mask [%lu] client [%p]\n",
                  pWin, mask, client);

      fprintf(stderr, "ConfigureWindow: x [%d] y [%d] w [%d] h [%d] CWStackMode [%d] "
                  "smode [%d] pSib [%p]\n",
                      x, y, w, h, (mask & CWStackMode) ? 1 : 0, smode, pSib);
    }
    #endif

    if (nxagentOption(Rootless) && nxagentWindowTopLevel(pWin) &&
            pWin -> overrideRedirect == 0 &&
                nxagentScreenTrap == 0)
    {
      nxagentConfigureRootlessWindow(pWin, x, y, w, h, bw, pSib, smode, mask);

      return Success;
    }

    if (mask & CWStackMode)
	pSib = WhereDoIGoInTheStack(pWin, pSib, pParent->drawable.x + x,
				    pParent->drawable.y + y,
				    w + (bw << 1), h + (bw << 1), smode);
    else
	pSib = pWin->nextSib;

#ifdef XAPPGROUP
    win_owner = clients[CLIENT_ID(pWin->drawable.id)];
    ag_leader = XagLeader (win_owner);
#endif

    if ((!pWin->overrideRedirect) && 
	(RedirectSend(pParent)
#ifdef XAPPGROUP
	|| (win_owner->appgroup && ag_leader && 
	    XagIsControlledRoot (client, pParent))
#endif
	))
    {
	memset(&event, 0, sizeof(xEvent));
	event.u.u.type = ConfigureRequest;
	event.u.configureRequest.window = pWin->drawable.id;
	if (mask & CWSibling)
	   event.u.configureRequest.sibling = sibwid;
	else
	    event.u.configureRequest.sibling = None;
	if (mask & CWStackMode)
	   event.u.u.detail = smode;
	else
	    event.u.u.detail = Above;
	event.u.configureRequest.x = x;
	event.u.configureRequest.y = y;
#ifdef PANORAMIX
	if(!noPanoramiXExtension && (!pParent || !pParent->parent)) {
            event.u.configureRequest.x += panoramiXdataPtr[0].x;
            event.u.configureRequest.y += panoramiXdataPtr[0].y;
	}
#endif
	event.u.configureRequest.width = w;
	event.u.configureRequest.height = h;
	event.u.configureRequest.borderWidth = bw;
	event.u.configureRequest.valueMask = mask;
#ifdef XAPPGROUP
	/* make sure if the ag_leader maps the window it goes to the wm */
	if (ag_leader && ag_leader != client && 
	    XagIsControlledRoot (client, pParent)) {
	    event.u.configureRequest.parent = XagId (win_owner);
	    (void) TryClientEvents (ag_leader, &event, 1,
				    NoEventMask, NoEventMask, NullGrab);
	    return Success;
	}
#endif
	event.u.configureRequest.parent = pParent->drawable.id;
	if (MaybeDeliverEventsToClient(pParent, &event, 1,
		SubstructureRedirectMask, client) == 1)
	    return(Success);
    }
    if (action == RESIZE_WIN)
    {
	Bool size_change = (w != pWin->drawable.width)
			|| (h != pWin->drawable.height);
	if (size_change && ((pWin->eventMask|wOtherEventMasks(pWin)) & ResizeRedirectMask))
	{
	    xEvent eventT;
	    memset(&eventT, 0, sizeof(xEvent));
	    eventT.u.u.type = ResizeRequest;
	    eventT.u.resizeRequest.window = pWin->drawable.id;
	    eventT.u.resizeRequest.width = w;
	    eventT.u.resizeRequest.height = h;
	    if (MaybeDeliverEventsToClient(pWin, &eventT, 1,
				       ResizeRedirectMask, client) == 1)
	    {
		/* if event is delivered, leave the actual size alone. */
		w = pWin->drawable.width;
		h = pWin->drawable.height;
		size_change = FALSE;
	    }
	}
	if (!size_change)
	{
	    if (mask & (CWX | CWY))
		action = MOVE_WIN;
	    else if (mask & (CWStackMode | CWBorderWidth))
		action = RESTACK_WIN;
	    else   /* really nothing to do */
		return(Success) ;
	}
    }

    if (action == RESIZE_WIN)
	    /* we've already checked whether there's really a size change */
	    goto ActuallyDoSomething;
    if ((mask & CWX) && (x != beforeX))
	    goto ActuallyDoSomething;
    if ((mask & CWY) && (y != beforeY))
	    goto ActuallyDoSomething;
    if ((mask & CWBorderWidth) && (bw != wBorderWidth (pWin)))
	    goto ActuallyDoSomething;
    if (mask & CWStackMode)
    {
#ifndef ROOTLESS
        /* See above for why we always reorder in rootless mode. */
	if (pWin->nextSib != pSib)
#endif
	    goto ActuallyDoSomething;
    }
    return(Success);

ActuallyDoSomething:
    if (SubStrSend(pWin, pParent))
    {
	memset(&event, 0, sizeof(xEvent));
	event.u.u.type = ConfigureNotify;
	event.u.configureNotify.window = pWin->drawable.id;
	if (pSib)
	    event.u.configureNotify.aboveSibling = pSib->drawable.id;
	else
	    event.u.configureNotify.aboveSibling = None;
	event.u.configureNotify.x = x;
	event.u.configureNotify.y = y;
#ifdef PANORAMIX
	if(!noPanoramiXExtension && (!pParent || !pParent->parent)) {
	    event.u.configureNotify.x += panoramiXdataPtr[0].x;
            event.u.configureNotify.y += panoramiXdataPtr[0].y;
	}
#endif
	event.u.configureNotify.width = w;
	event.u.configureNotify.height = h;
	event.u.configureNotify.borderWidth = bw;
	event.u.configureNotify.override = pWin->overrideRedirect;
	DeliverEvents(pWin, &event, 1, NullWindow);
    }
    if (mask & CWBorderWidth)
    {
	if (action == RESTACK_WIN)
	{
	    action = MOVE_WIN;
	    pWin->borderWidth = bw;
	}
	else if ((action == MOVE_WIN) &&
		 (beforeX + wBorderWidth (pWin) == x + (int)bw) &&
		 (beforeY + wBorderWidth (pWin) == y + (int)bw))
	{
	    action = REBORDER_WIN;
	    (*pWin->drawable.pScreen->ChangeBorderWidth)(pWin, bw);
	}
	else
	    pWin->borderWidth = bw;
    }
    if (action == MOVE_WIN)
	(*pWin->drawable.pScreen->MoveWindow)(pWin, x, y, pSib,
		   (mask & CWBorderWidth) ? VTOther : VTMove);
    else if (action == RESIZE_WIN)
	(*pWin->drawable.pScreen->ResizeWindow)(pWin, x, y, w, h, pSib);
    else if (mask & CWStackMode)
	ReflectStackChange(pWin, pSib, VTOther);

    if (action != RESTACK_WIN)
	CheckCursorConfinement(pWin);

    nxagentFlushConfigureWindow();

    return(Success);
#undef RESTACK_WIN
#undef MOVE_WIN
#undef RESIZE_WIN
#undef REBORDER_WIN
}

/*****
 *  ReparentWindow
 *****/

int
ReparentWindow(register WindowPtr pWin, register WindowPtr pParent,
               int x, int y, ClientPtr client)
{
    WindowPtr pPrev, pPriorParent;
    Bool WasMapped = (Bool)(pWin->mapped);
    xEvent event;
    int bw = wBorderWidth (pWin);
    register ScreenPtr pScreen;

    pScreen = pWin->drawable.pScreen;
    if (TraverseTree(pWin, CompareWIDs, (void *)&pParent->drawable.id) == WT_STOPWALKING)
	return(BadMatch);		
    if (!MakeWindowOptional(pWin))
	return(BadAlloc);

    if (WasMapped)
       UnmapWindow(pWin, FALSE);

    memset(&event, 0, sizeof(xEvent));
    event.u.u.type = ReparentNotify;
    event.u.reparent.window = pWin->drawable.id;
    event.u.reparent.parent = pParent->drawable.id;
    event.u.reparent.x = x;
    event.u.reparent.y = y;
#ifdef PANORAMIX
    if(!noPanoramiXExtension && !pParent->parent) {
	event.u.reparent.x += panoramiXdataPtr[0].x;
	event.u.reparent.y += panoramiXdataPtr[0].y;
    }
#endif
    event.u.reparent.override = pWin->overrideRedirect;
    DeliverEvents(pWin, &event, 1, pParent);

    /* take out of sibling chain */

    pPriorParent = pPrev = pWin->parent;
    if (pPrev->firstChild == pWin)
	pPrev->firstChild = pWin->nextSib;
    if (pPrev->lastChild == pWin)
	pPrev->lastChild = pWin->prevSib;

    if (pWin->nextSib)
	pWin->nextSib->prevSib = pWin->prevSib;
    if (pWin->prevSib)
	pWin->prevSib->nextSib = pWin->nextSib;

    /* insert at begining of pParent */
    pWin->parent = pParent;
    pPrev = RealChildHead(pParent);

    if (pWin->parent == screenInfo.screens[0]->root)
    {
      nxagentSetTopLevelEventMask(pWin);
    }
 
    if (pPrev)
    {
	pWin->nextSib = pPrev->nextSib;
	if (pPrev->nextSib)
	    pPrev->nextSib->prevSib = pWin;
	else
	    pParent->lastChild = pWin;
	pPrev->nextSib = pWin;
	pWin->prevSib = pPrev;
    }
    else
    {
	pWin->nextSib = pParent->firstChild;
	pWin->prevSib = NullWindow;
	if (pParent->firstChild)
	    pParent->firstChild->prevSib = pWin;
	else
	    pParent->lastChild = pWin;
	pParent->firstChild = pWin;
    }

    pWin->origin.x = x + bw;
    pWin->origin.y = y + bw;
    pWin->drawable.x = x + bw + pParent->drawable.x;
    pWin->drawable.y = y + bw + pParent->drawable.y;

    /* clip to parent */
    SetWinSize (pWin);
    SetBorderSize (pWin);

    if (pScreen->ReparentWindow)
	(*pScreen->ReparentWindow)(pWin, pPriorParent);

    (*pScreen->PositionWindow)(pWin, pWin->drawable.x, pWin->drawable.y);

    ResizeChildrenWinSize(pWin, 0, 0, 0, 0);

    CheckWindowOptionalNeed(pWin);

    if (WasMapped)
	MapWindow(pWin, client);
    RecalculateDeliverableEvents(pWin);
    return(Success);
}

/*****
 * MapWindow
 *    If some other client has selected SubStructureReDirect on the parent
 *    and override-redirect is xFalse, then a MapRequest event is generated,
 *    but the window remains unmapped.	Otherwise, the window is mapped and a
 *    MapNotify event is generated.
 *****/

int
MapWindow(register WindowPtr pWin, ClientPtr client)
{
    register ScreenPtr pScreen;

    register WindowPtr pParent;
#ifdef DO_SAVE_UNDERS
    Bool	dosave = FALSE;
#endif
    WindowPtr  pLayerWin;

    #ifdef TEST
    if (nxagentWindowTopLevel(pWin))
    {
      fprintf(stderr, "MapWindow: pWin [%p] client [%p]\n", pWin, client);
    }
    #endif

    if (pWin->mapped)
	return(Success);

#ifdef XCSECURITY
    /*  don't let an untrusted client map a child-of-trusted-window, InputOnly
     *  window; too easy to steal device input
     */
    if ( (client->trustLevel != XSecurityClientTrusted) &&
	 (pWin->drawable.class == InputOnly) &&
	 (wClient(pWin->parent)->trustLevel == XSecurityClientTrusted) )
	 return Success;
#endif	

    pScreen = pWin->drawable.pScreen;
    if ( (pParent = pWin->parent) )
    {
	xEvent event;
	Bool anyMarked;
#ifdef XAPPGROUP
	ClientPtr win_owner = clients[CLIENT_ID(pWin->drawable.id)];
	ClientPtr ag_leader = XagLeader (win_owner);
#endif

	if ((!pWin->overrideRedirect) && 
	    (RedirectSend(pParent)
#ifdef XAPPGROUP
	    || (win_owner->appgroup && ag_leader &&
		XagIsControlledRoot (client, pParent))
#endif
	))
	{
	    memset(&event, 0, sizeof(xEvent));
	    event.u.u.type = MapRequest;
	    event.u.mapRequest.window = pWin->drawable.id;
#ifdef XAPPGROUP
	    /* make sure if the ag_leader maps the window it goes to the wm */
	    if (ag_leader && ag_leader != client &&
		XagIsControlledRoot (client, pParent)) {
		event.u.mapRequest.parent = XagId (win_owner);
		(void) TryClientEvents (ag_leader, &event, 1,
					NoEventMask, NoEventMask, NullGrab);
		return Success;
	    }
#endif
	    event.u.mapRequest.parent = pParent->drawable.id;

	    if (MaybeDeliverEventsToClient(pParent, &event, 1,
		SubstructureRedirectMask, client) == 1)
		return(Success);
	}

	pWin->mapped = TRUE;
	if (SubStrSend(pWin, pParent))
	{
	    memset(&event, 0, sizeof(xEvent));
	    event.u.u.type = MapNotify;
	    event.u.mapNotify.window = pWin->drawable.id;
	    event.u.mapNotify.override = pWin->overrideRedirect;
	    DeliverEvents(pWin, &event, 1, NullWindow);
	}

	if (!pParent->realized)
	    return(Success);
	RealizeTree(pWin);
	if (pWin->viewable)
	{
	    anyMarked = (*pScreen->MarkOverlappedWindows)(pWin, pWin,
							  &pLayerWin);
#ifdef DO_SAVE_UNDERS
	    if (DO_SAVE_UNDERS(pWin))
	    {
		dosave = (*pScreen->ChangeSaveUnder)(pLayerWin, pWin->nextSib);
	    }
#endif /* DO_SAVE_UNDERS */
	    if (anyMarked)
	    {
		(*pScreen->ValidateTree)(pLayerWin->parent, pLayerWin, VTMap);
		(*pScreen->HandleExposures)(pLayerWin->parent);
	    }
#ifdef DO_SAVE_UNDERS
	    if (dosave)
		(*pScreen->PostChangeSaveUnder)(pLayerWin, pWin->nextSib);
#endif /* DO_SAVE_UNDERS */
	if (anyMarked && pScreen->PostValidateTree)
	    (*pScreen->PostValidateTree)(pLayerWin->parent, pLayerWin, VTMap);
	}
	WindowsRestructured ();
    }
    else
    {
	RegionRec   temp;

	pWin->mapped = TRUE;
	pWin->realized = TRUE;	   /* for roots */
	pWin->viewable = pWin->drawable.class == InputOutput;
	/* We SHOULD check for an error value here XXX */
	(*pScreen->RealizeWindow)(pWin);
	if (pScreen->ClipNotify)
	    (*pScreen->ClipNotify) (pWin, 0, 0);
	if (pScreen->PostValidateTree)
	    (*pScreen->PostValidateTree)(NullWindow, pWin, VTMap);
	RegionNull(&temp);
	RegionCopy(&temp, &pWin->clipList);
	(*pScreen->WindowExposures) (pWin, &temp, NullRegion);
	RegionUninit(&temp);
    }

    nxagentFlushConfigureWindow();

    return(Success);
}

/*****
 * UnmapWindow
 *    If the window is already unmapped, this request has no effect.
 *    Otherwise, the window is unmapped and an UnMapNotify event is
 *    generated.  Cannot unmap a root window.
 *****/

int
UnmapWindow(register WindowPtr pWin, Bool fromConfigure)
{
    register WindowPtr pParent;
    xEvent event;
    Bool wasRealized = (Bool)pWin->realized;
    Bool wasViewable = (Bool)pWin->viewable;
    ScreenPtr pScreen = pWin->drawable.pScreen;
    WindowPtr pLayerWin = pWin;

    #ifdef TEST
    if (nxagentWindowTopLevel(pWin))
    {
      fprintf(stderr, "UnmapWindow: pWin [%p] fromConfigure [%d]\n", pWin,
                  fromConfigure);
    }
    #endif

    if ((!pWin->mapped) || (!(pParent = pWin->parent)))
	return(Success);
    if (SubStrSend(pWin, pParent))
    {
	memset(&event, 0, sizeof(xEvent));
	event.u.u.type = UnmapNotify;
	event.u.unmapNotify.window = pWin->drawable.id;
	event.u.unmapNotify.fromConfigure = fromConfigure;
	DeliverEvents(pWin, &event, 1, NullWindow);
    }
    if (wasViewable && !fromConfigure)
    {
	pWin->valdata = UnmapValData;
	(*pScreen->MarkOverlappedWindows)(pWin, pWin->nextSib, &pLayerWin);
	(*pScreen->MarkWindow)(pLayerWin->parent);
    }
    pWin->mapped = FALSE;
    if (wasRealized)
	UnrealizeTree(pWin, fromConfigure);
    if (wasViewable)
    {
	if (!fromConfigure)
	{
	    (*pScreen->ValidateTree)(pLayerWin->parent, pWin, VTUnmap);
	    (*pScreen->HandleExposures)(pLayerWin->parent);
	}
#ifdef DO_SAVE_UNDERS
	if (DO_SAVE_UNDERS(pWin))
	{
	    if ( (*pScreen->ChangeSaveUnder)(pLayerWin, pWin->nextSib) )
	    {
		(*pScreen->PostChangeSaveUnder)(pLayerWin, pWin->nextSib);
	    }
	}
	pWin->DIXsaveUnder = FALSE;
#endif /* DO_SAVE_UNDERS */
	if (!fromConfigure && pScreen->PostValidateTree)
	    (*pScreen->PostValidateTree)(pLayerWin->parent, pWin, VTUnmap);
    }
    if (wasRealized && !fromConfigure)
	WindowsRestructured ();
    return(Success);
}

void
SaveScreens(int on, int mode)
{
    int i;
    int what;
    int type;

    if (on == SCREEN_SAVER_FORCER)
    {
	UpdateCurrentTimeIf();
	lastDeviceEventTime = currentTime;
	if (mode == ScreenSaverReset)
	    what = SCREEN_SAVER_OFF;
	else
	    what = SCREEN_SAVER_ON;
	type = what;
    }
    else
    {
	what = on;
	type = what;
	if (what == screenIsSaved)
	    type = SCREEN_SAVER_CYCLE;
    }
    for (i = 0; i < screenInfo.numScreens; i++)
    {
	if (on == SCREEN_SAVER_FORCER)
	   (* screenInfo.screens[i]->SaveScreen) (screenInfo.screens[i], on);
	if (savedScreenInfo[i].ExternalScreenSaver)
	{
          if (nxagentOption(Timeout) != 0)
          {
            #ifdef TEST
            fprintf(stderr, "SaveScreens: An external screen-saver handler is installed. "
                        "Ignoring it to let the auto-disconnect feature work.\n");
            #endif
          }
          else
          {
	      if ((*savedScreenInfo[i].ExternalScreenSaver)
		  (screenInfo.screens[i], type, on == SCREEN_SAVER_FORCER))
		  continue;
          }
	}
	if (type == screenIsSaved)
	    continue;
	switch (type) {
	case SCREEN_SAVER_OFF:
	    if (savedScreenInfo[i].blanked == SCREEN_IS_BLANKED)
	    {
	       (* screenInfo.screens[i]->SaveScreen) (screenInfo.screens[i],
						      what);
	    }
	    else if (HasSaverWindow (i))
	    {
		savedScreenInfo[i].pWindow = NullWindow;
		FreeResource(savedScreenInfo[i].wid, RT_NONE);
	    }
	    break;
	case SCREEN_SAVER_CYCLE:
	    if (savedScreenInfo[i].blanked == SCREEN_IS_TILED)
	    {
		WindowPtr pWin = savedScreenInfo[i].pWindow;
		/* make it look like screen saver is off, so that
		 * NotClippedByChildren will compute a clip list
		 * for the root window, so miPaintWindow works
		 */
		screenIsSaved = SCREEN_SAVER_OFF;
#ifndef NOLOGOHACK
		if (logoScreenSaver)
		    (*pWin->drawable.pScreen->ClearToBackground)(pWin, 0, 0, 0, 0, FALSE);
#endif
		(*pWin->drawable.pScreen->MoveWindow)(pWin,
			   (short)(-(rand() % RANDOM_WIDTH)),
			   (short)(-(rand() % RANDOM_WIDTH)),
			   pWin->nextSib, VTMove);
#ifndef NOLOGOHACK
		if (logoScreenSaver)
		    DrawLogo(pWin);
#endif
		screenIsSaved = SCREEN_SAVER_ON;
	    }
	    /*
	     * Call the DDX saver in case it wants to do something
	     * at cycle time
	     */
	    else if (savedScreenInfo[i].blanked == SCREEN_IS_BLANKED)
	    {
		(* screenInfo.screens[i]->SaveScreen) (screenInfo.screens[i],
						       type);
	    }
	    break;
	case SCREEN_SAVER_ON:
	    if (ScreenSaverBlanking != DontPreferBlanking)
	    {
		if ((* screenInfo.screens[i]->SaveScreen)
		   (screenInfo.screens[i], what))
		{
		   savedScreenInfo[i].blanked = SCREEN_IS_BLANKED;
		   continue;
		}
		if ((ScreenSaverAllowExposures != DontAllowExposures) &&
		    TileScreenSaver(i, SCREEN_IS_BLACK))
		{
		    savedScreenInfo[i].blanked = SCREEN_IS_BLACK;
		    continue;
		}
	    }
	    if ((ScreenSaverAllowExposures != DontAllowExposures) &&
		TileScreenSaver(i, SCREEN_IS_TILED))
	    {
		savedScreenInfo[i].blanked = SCREEN_IS_TILED;
	    }
	    else
		savedScreenInfo[i].blanked = SCREEN_ISNT_SAVED;
	    break;
	}
    }
    screenIsSaved = what;
    if (mode == ScreenSaverReset)
       SetScreenSaverTimer();
}
