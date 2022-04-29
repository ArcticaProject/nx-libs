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

/************************************************************

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

********************************************************/

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

/*****************************************************************

Copyright 2003-2005 Sun Microsystems, Inc.

All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, and/or sell copies of the Software, and to permit persons
to whom the Software is furnished to do so, provided that the above
copyright notice(s) and this permission notice appear in all copies of
the Software and that both the above copyright notice(s) and this
permission notice appear in supporting documentation.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT
OF THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
HOLDERS INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL
INDIRECT OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING
FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

Except as contained in this notice, the name of a copyright holder
shall not be used in advertising or otherwise to promote the sale, use
or other dealings in this Software without prior written authorization
of the copyright holder.

******************************************************************/


#include <nx-X11/Xlib.h>

#define XYWINDOWCALLBACK
#include "../../dix/events.c"

#include "compext/Compext.h"

#include "Events.h"
#include "Windows.h"
#include "Args.h"
#include "Clipboard.h"

extern Display *nxagentDisplay;

extern WindowPtr nxagentLastEnteredWindow;

#ifdef VIEWPORT_FRAME
extern void nxagentInitViewportFrame(ScreenPtr, WindowPtr);
#endif
extern int  nxagentShadowInit(ScreenPtr, WindowPtr);

void
ActivatePointerGrab(register DeviceIntPtr mouse, register GrabPtr grab,
                    TimeStamp time, Bool autoGrab)
{
    #ifdef DEBUG
    fprintf(stderr, "%s: called\n", __func__);
    #endif

    xorg_ActivatePointerGrab(mouse, grab, time, autoGrab);

    #ifdef NXAGENT_SERVER

    /*
     * If grab is synchronous, events are delivered to clients only if they send
     * an AllowEvent request. If mode field in AllowEvent request is SyncPointer, the 
     * delivered event is saved in a queue and replayed later, when grab is released.
     * We should export sync grab to X as async in order to avoid events to be
     * queued twice, in the agent and in the X server. This solution have a drawback:
     * replayed events are not delivered to that application that are not clients of
     * the agent.
     * A different solution could be to make the grab asynchronous in the agent and 
     * to export it as synchronous. But this seems to be less safe.
     *
     * To make internal grab asynchronous, change previous line as follows.
     *
     * if (nxagentOption(Rootless))
     * {
     *   CheckGrabForSyncs(mouse, GrabModeAsync, (Bool)grab->keyboardMode);
     * }
     * else
     * {
     *   CheckGrabForSyncs(mouse,(Bool)grab->pointerMode, (Bool)grab->keyboardMode);
     * }
     */

    if (nxagentOption(Rootless))
    {
      /*
       * from nxagent-1.5.0-20 changelog:
       * In rootless mode, grabs exported to X in
       * ActivatePointerGrab() are always made asynchronous. The
       * synchronous behaviour is implemented by the agent, so that
       * requiring a further synchronous grab down to the real X
       * server is of little use and potentially harmful.
       */

      /*
       * FIXME: We should use the correct value for the
       * cursor. Temporarily we set it to None.
       */

       int resource = nxagentWaitForResource(NXGetCollectGrabPointerResource,
                                                 nxagentCollectGrabPointerPredicate);

       NXCollectGrabPointer(nxagentDisplay, resource, nxagentWindow(grab -> window),
                                1, grab -> eventMask & PointerGrabMask,
                                    GrabModeAsync, GrabModeAsync, (grab -> confineTo) ?
                                        nxagentWindow(grab -> confineTo) : None,
                                            None, CurrentTime);
    }

    #endif /* NXAGENT_SERVER */
}

void
DeactivatePointerGrab(register DeviceIntPtr mouse)
{
    #ifdef DEBUG
    fprintf(stderr, "%s: called\n", __func__);
    #endif

    xorg_DeactivatePointerGrab(mouse);

    #ifdef NXAGENT_SERVER

    /*
     * excerpt from nxagent-1.5.0-20 changelog:
     * In DeactivatePointerGrab() function, mouse button state is set
     * to up if the window entered by the pointer is the root window
     * and the agent is in rootless mode. This change is needed
     * because the subsequent KeyRelease event could be not received
     * by the agent (for example if the focus had left the window), so
     * that agent could be unable to update the mouse button state.
     */
    if (nxagentOption(Rootless))
    {
      XUngrabPointer(nxagentDisplay, CurrentTime);

      if (sprite.win == ROOT)
      {
        mouse -> button -> state &=
            ~(Button1Mask | Button2Mask | Button3Mask |
                  Button4Mask | Button5Mask);
      }
    }

    #endif
}

int
ProcAllowEvents(register ClientPtr client)
{
    int rc = xorg_ProcAllowEvents(client);

    if (rc != Success)
        return rc;

    /*
     * This is not necessary if we export grab to X as asynchronous.
     *
     * if (nxagentOption(Rootless) && stuff -> mode != ReplayKeyboard &&
     *         stuff -> mode != SyncKeyboard && stuff -> mode != AsyncKeyboard)
     * {
     *   XAllowEvents(nxagentDisplay, stuff -> mode, CurrentTime);
     * }
     */

    return Success;
}

/*
 * called from XYToWindow to determine where XYToWindow() should start
 * going through the list.
 */

static WindowPtr 
GetXYStartWindow(WindowPtr pWin)
{

    if (nxagentOption(Rootless))
    {
      /*
       * explanation from the original changelog for nxagent-1.5.0-20:
       * Modified function XYToWindow() in order to manage the case
       * that mouse pointer is located on the title bar of a top level
       * window in rootless mode.
       */

      if (nxagentLastEnteredWindow == NULL)
      {
        return ROOT;
      }

      /*
       * explanation from the original changelog for nxagent-1.5.0-17:
       * In rootless mode, now function XYToWindow() starts search from
       * the last window originated an EnterNotify event. In this way, we
       * can prevent shaded windows from getting mouse events.
       */
      pWin = ROOT->lastChild;

      while (pWin && pWin != ROOT->firstChild && pWin != nxagentLastEnteredWindow)
      {
        pWin = pWin->prevSib;
      }
    }
    return pWin;
}

static Bool
CheckMotion(xEvent *xE)
{
    WindowPtr prevSpriteWin = sprite.win;

#ifdef PANORAMIX
    if(!noPanoramiXExtension)
	return XineramaCheckMotion(xE);
#endif

    if (xE && !syncEvents.playingEvents)
    {
	if (sprite.hot.pScreen != sprite.hotPhys.pScreen)
	{
	    sprite.hot.pScreen = sprite.hotPhys.pScreen;
	    ROOT = sprite.hot.pScreen->root;
	}
	sprite.hot.x = XE_KBPTR.rootX;
	sprite.hot.y = XE_KBPTR.rootY;
	if (sprite.hot.x < sprite.physLimits.x1)
	    sprite.hot.x = sprite.physLimits.x1;
	else if (sprite.hot.x >= sprite.physLimits.x2)
	    sprite.hot.x = sprite.physLimits.x2 - 1;
	if (sprite.hot.y < sprite.physLimits.y1)
	    sprite.hot.y = sprite.physLimits.y1;
	else if (sprite.hot.y >= sprite.physLimits.y2)
	    sprite.hot.y = sprite.physLimits.y2 - 1;
#ifdef SHAPE
	if (sprite.hotShape)
	    ConfineToShape(sprite.hotShape, &sprite.hot.x, &sprite.hot.y);
#endif
	sprite.hotPhys = sprite.hot;

#ifdef NXAGENT_SERVER
        /*
         * This code force cursor position to be inside the
         * root window of the agent. We can't view a reason
         * to do this and it interacts in an undesirable way
         * with toggling fullscreen.
         *
         * if ((sprite.hotPhys.x != XE_KBPTR.rootX) ||
         *          (sprite.hotPhys.y != XE_KBPTR.rootY))
         * {
         *   (*sprite.hotPhys.pScreen->SetCursorPosition)(
         *       sprite.hotPhys.pScreen,
         *           sprite.hotPhys.x, sprite.hotPhys.y, FALSE);
         * }
         */
#else
	if ((sprite.hotPhys.x != XE_KBPTR.rootX) ||
	    (sprite.hotPhys.y != XE_KBPTR.rootY))
	{
	    (*sprite.hotPhys.pScreen->SetCursorPosition)(
		sprite.hotPhys.pScreen,
		sprite.hotPhys.x, sprite.hotPhys.y, FALSE);
	}
#endif
	XE_KBPTR.rootX = sprite.hot.x;
	XE_KBPTR.rootY = sprite.hot.y;
    }

    sprite.win = XYToWindow(sprite.hot.x, sprite.hot.y);
#ifdef notyet
    if (!(sprite.win->deliverableEvents &
	  Motion_Filter(inputInfo.pointer->button))
	!syncEvents.playingEvents)
    {
	/* XXX Do PointerNonInterestBox here */
    }
#endif
    if (sprite.win != prevSpriteWin)
    {
	if (prevSpriteWin != NullWindow) {
	    if (!xE)
		UpdateCurrentTimeIf();
	    DoEnterLeaveEvents(prevSpriteWin, sprite.win, NotifyNormal);
	}
	PostNewCursor();
        return FALSE;
    }
    return TRUE;
}

void
DefineInitialRootWindow(register WindowPtr win)
{
    register ScreenPtr pScreen = win->drawable.pScreen;

    xorg_DefineInitialRootWindow(win);

    #ifdef VIEWPORT_FRAME
    nxagentInitViewportFrame(pScreen, win);
    #endif

    if (nxagentOption(Shadow))
    {
      if (nxagentShadowInit(pScreen, win) == -1)
      {
        FatalError("Failed to connect to display '%s'", nxagentShadowDisplayName);
      }
    }
}

int
ProcSendEvent(ClientPtr client)
{
#ifdef NXAGENT_CLIPBOARD

    REQUEST(xSendEventReq);

    REQUEST_SIZE_MATCH(xSendEventReq);

    if (nxagentOption(Rootless) && stuff->event.u.u.type == ClientMessage)
    {
        ForwardClientMessage(client, stuff);
        return Success;
    }

    if (stuff -> event.u.u.type == SelectionNotify)
    {
        #ifdef DEBUG
        fprintf(stderr, "%s: sending SelectionNotify to ourselves"\n, __func__);
        #endif
        if (nxagentSendNotificationToSelfViaXServer(&stuff->event) == 1)
            return Success;
    }
#endif
    return xorg_ProcSendEvent(client);
}
