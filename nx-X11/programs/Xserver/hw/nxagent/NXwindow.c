/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2011 NoMachine (http://www.nomachine.com)          */
/* Copyright (c) 2008-2017 Oleksandr Shneyder <o.shneyder@phoca-gmbh.de>  */
/* Copyright (c) 2011-2022 Mike Gabriel <mike.gabriel@das-netzwerkteam.de>*/
/* Copyright (c) 2014-2019 Mihai Moldovan <ionic@ionic.de>                */
/* Copyright (c) 2014-2022 Ulrich Sibiller <uli42@gmx.de>                 */
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


#include "selection.h"

#include "Screen.h"
#include "Options.h"
#include "Clipboard.h"
#include "Splash.h"
#include "Rootless.h"
#include "Composite.h"
#include "Drawable.h"
#include "Colormap.h"

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

extern void nxagentSetVersionProperty(WindowPtr pWin);

void
InitRootWindow(WindowPtr pWin)
{
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

    if (nxagentOption(Rootless))
    {
      #ifdef TEST
      fprintf(stderr, "InitRootWindow: Assigned agent root to window at [%p][%d] with parent [%p].\n",
                  (void *) pWin, nxagentWindowPriv(pWin)->window, (void *) pWin -> parent);
      #endif

      nxagentRootlessWindow = pWin;
    }

    xorg_InitRootWindow(pWin);

    /*
     * Map both the root and the default agent window.
     */

    #ifdef TEST
    fprintf(stderr, "InitRootWindow: Mapping default windows.\n");
    #endif

    nxagentInitClipboard(pWin);

    nxagentMapDefaultWindows();

    nxagentRedirectDefaultWindows();

    #ifdef NXAGENT_ARTSD
    {
      char artsd_port[10];
      short int nPort = atoi(display) + 7000;
      sprintf(artsd_port,"%d", nPort);
      nxagentPropagateArtsdProperties(pWin->drawable.pScreen, artsd_port);
    }
    #endif

    nxagentSetVersionProperty(pWin);
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
		xEvent event = {0};
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

#ifdef NXAGENT_SERVER
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
#else
        (*pScreen->PositionWindow)(pSib, pSib->drawable.x, pSib->drawable.y);
#endif

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
    xEvent event = {0};

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
						RT_WINDOW, DixReadAccess);
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

	/* Figure out if the window should be moved.  Doesn't
	   make the changes to the window if event sent */

#ifdef NXAGENT_SERVER
    #ifdef TEST
    if (nxagentWindowTopLevel(pWin))
    {
      fprintf(stderr, "ConfigureWindow: pWin [%p] mask [%u] client [%p]\n",
                  (void *)pWin, mask, (void *)client);

      fprintf(stderr, "ConfigureWindow: x [%d] y [%d] w [%d] h [%d] CWStackMode [%d] "
                  "smode [%d] pSib [%p]\n",
                      x, y, w, h, (mask & CWStackMode) ? 1 : 0, smode, (void *)pSib);
    }
    #endif

    if (nxagentOption(Rootless) && nxagentWindowTopLevel(pWin) &&
            pWin -> overrideRedirect == 0 &&
                !nxagentScreenTrap)
    {
      nxagentConfigureRootlessWindow(pWin, x, y, w, h, bw, pSib, smode, mask);

      return Success;
    }
#endif

    if (mask & CWStackMode)
	pSib = WhereDoIGoInTheStack(pWin, pSib, pParent->drawable.x + x,
				    pParent->drawable.y + y,
				    w + (bw << 1), h + (bw << 1), smode);
    else
	pSib = pWin->nextSib;


    if ((!pWin->overrideRedirect) && 
	(RedirectSend(pParent)
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
	    xEvent eventT = {0};
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

#ifdef NXAGENT_SERVER
    nxagentFlushConfigureWindow();
#endif

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
    xEvent event = {0};
    int bw = wBorderWidth (pWin);
    register ScreenPtr pScreen;

    pScreen = pWin->drawable.pScreen;
    if (TraverseTree(pWin, CompareWIDs, (void *)&pParent->drawable.id) == WT_STOPWALKING)
	return(BadMatch);		
    if (!MakeWindowOptional(pWin))
	return(BadAlloc);

    if (WasMapped)
       UnmapWindow(pWin, FALSE);

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

    /* insert at beginning of pParent */
    pWin->parent = pParent;
    pPrev = RealChildHead(pParent);

#ifdef NXAGENT_SERVER
    if (pWin->parent == screenInfo.screens[0]->root)
    {
      nxagentSetTopLevelEventMask(pWin);
    }
#endif
 
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

#ifdef NXAGENT_SERVER
    #ifdef TEST
    if (nxagentWindowTopLevel(pWin))
    {
      fprintf(stderr, "MapWindow: pWin [%p] client [%p]\n", (void *)pWin, (void *)client);
    }
    #endif
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

	if ((!pWin->overrideRedirect) && 
	    (RedirectSend(pParent)
	))
	{
	    memset(&event, 0, sizeof(xEvent));
	    event.u.u.type = MapRequest;
	    event.u.mapRequest.window = pWin->drawable.id;
	    event.u.mapRequest.parent = pParent->drawable.id;

	    if (MaybeDeliverEventsToClient(pParent, &event, 1,
		SubstructureRedirectMask, client) == 1)
		return(Success);
	}

	pWin->mapped = TRUE;
	if (SubStrSend(pWin, pParent) && MapUnmapEventsEnabled(pWin))
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

#ifdef NXAGENT_SERVER
    nxagentFlushConfigureWindow();
#endif

    return(Success);
}
