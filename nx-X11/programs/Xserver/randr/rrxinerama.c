/*
 * Copyright © 2006 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */
/*
 * This Xinerama implementation comes from the SiS driver which has
 * the following notice:
 */
/*
 * SiS driver main code
 *
 * Copyright (C) 2001-2005 by Thomas Winischhofer, Vienna, Austria.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1) Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2) Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3) The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Author: Thomas Winischhofer <thomas@winischhofer.net>
 *	- driver entirely rewritten since 2001, only basic structure taken from
 *	  old code (except sis_dri.c, sis_shadow.c, sis_accel.c and parts of
 *	  sis_dga.c; these were mostly taken over; sis_dri.c was changed for
 *	  new versions of the DRI layer)
 *
 * This notice covers the entire driver code unless indicated otherwise.
 *
 * Formerly based on code which was
 * 	     Copyright (C) 1998, 1999 by Alan Hourihane, Wigan, England.
 * 	     Written by:
 *           Alan Hourihane <alanh@fairlite.demon.co.uk>,
 *           Mike Chapman <mike@paranoia.com>,
 *           Juanjo Santamarta <santamarta@ctv.es>,
 *           Mitani Hiroshi <hmitani@drl.mei.co.jp>,
 *           David Thomas <davtom@dream.org.uk>.
 */

/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2011 NoMachine, http://www.nomachine.com/.         */
/*                                                                        */
/* NX-X11, NX protocol compression and NX extensions to this software     */
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

#include "randrstr.h"
#include "swaprep.h"
#include "panoramiXproto.h"
#ifndef NXAGENT_SERVER
#include "protocol-versions.h"
#else
#define SERVER_RRXINERAMA_MAJOR_VERSION     1
#define SERVER_RRXINERAMA_MINOR_VERSION     1
#endif

/* Xinerama is not multi-screen capable; just report about screen 0 */
#define RR_XINERAMA_SCREEN  0

static int ProcRRXineramaQueryVersion(ClientPtr client);
static int ProcRRXineramaGetState(ClientPtr client);
static int ProcRRXineramaGetScreenCount(ClientPtr client);
static int ProcRRXineramaGetScreenSize(ClientPtr client);
static int ProcRRXineramaIsActive(ClientPtr client);
static int ProcRRXineramaQueryScreens(ClientPtr client);
static int SProcRRXineramaDispatch(ClientPtr client);

extern Bool noRRXineramaExtension;

/* Proc */

int
ProcRRXineramaQueryVersion(ClientPtr client)
{
    int n;

    xPanoramiXQueryVersionReply rep = {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0,
        .majorVersion = SERVER_RRXINERAMA_MAJOR_VERSION,
        .minorVersion = SERVER_RRXINERAMA_MINOR_VERSION
    };

    REQUEST_SIZE_MATCH(xPanoramiXQueryVersionReq);
    if (client->swapped) {
        swaps(&rep.sequenceNumber, n);
        swapl(&rep.length, n);
        swaps(&rep.majorVersion, n);
        swaps(&rep.minorVersion, n);
    }
    WriteToClient(client, sizeof(xPanoramiXQueryVersionReply), (char *) &rep);
    return Success;
}

int
ProcRRXineramaGetState(ClientPtr client)
{
    REQUEST(xPanoramiXGetStateReq);
    WindowPtr pWin;
    xPanoramiXGetStateReply rep;
    register int rc;
    ScreenPtr pScreen;
    rrScrPrivPtr pScrPriv;
    Bool active = FALSE;
    int n;

    REQUEST_SIZE_MATCH(xPanoramiXGetStateReq);
#ifndef NXAGENT_SERVER
    rc = dixLookupWindow(&pWin, stuff->window, client, DixGetAttrAccess);
#else
    pWin = SecurityLookupWindow(stuff->window, client, SecurityReadAccess);
    rc = pWin ? Success : BadWindow;
#endif

    if (rc != Success)
        return rc;

    pScreen = pWin->drawable.pScreen;
    pScrPriv = rrGetScrPriv(pScreen);
    if (pScrPriv) {
        /* XXX do we need more than this? */
        active = TRUE;
    }

    rep = (xPanoramiXGetStateReply) {
        .type = X_Reply,
        .state = active,
        .sequenceNumber = client->sequence,
        .length = 0,
        .window = stuff->window
    };
    if (client->swapped) {
        swaps(&rep.sequenceNumber, n);
        swapl(&rep.length, n);
        swapl(&rep.window, n);
    }
    WriteToClient(client, sizeof(xPanoramiXGetStateReply), (char *) &rep);
    return Success;
}

static int
RRXineramaScreenCount(ScreenPtr pScreen)
{
    return RRMonitorCountList(pScreen);
}

static Bool
RRXineramaScreenActive(ScreenPtr pScreen)
{
    return RRXineramaScreenCount(pScreen) > 0;
}

int
ProcRRXineramaGetScreenCount(ClientPtr client)
{
    REQUEST(xPanoramiXGetScreenCountReq);
    WindowPtr pWin;
    xPanoramiXGetScreenCountReply rep;
    register int rc;
    int n;

    REQUEST_SIZE_MATCH(xPanoramiXGetScreenCountReq);

#ifndef NXAGENT_SERVER
    rc = dixLookupWindow(&pWin, stuff->window, client, DixGetAttrAccess);
#else
    pWin = SecurityLookupWindow(stuff->window, client, SecurityReadAccess);
    rc = pWin ? Success : BadWindow;
#endif
    if (rc != Success)
        return rc;

    rep = (xPanoramiXGetScreenCountReply) {
        .type = X_Reply,
        .ScreenCount = RRXineramaScreenCount(pWin->drawable.pScreen),
        .sequenceNumber = client->sequence,
        .length = 0,
        .window = stuff->window
    };
    if (client->swapped) {
        swaps(&rep.sequenceNumber, n);
        swapl(&rep.length, n);
        swapl(&rep.window, n);
    }
    WriteToClient(client, sizeof(xPanoramiXGetScreenCountReply), (char *) &rep);
    return Success;
}

int
ProcRRXineramaGetScreenSize(ClientPtr client)
{
    REQUEST(xPanoramiXGetScreenSizeReq);
    WindowPtr pWin, pRoot;
    ScreenPtr pScreen;
    xPanoramiXGetScreenSizeReply rep;
    register int rc;
    int n;

    REQUEST_SIZE_MATCH(xPanoramiXGetScreenSizeReq);
#ifndef NXAGENT_SERVER
    rc = dixLookupWindow(&pWin, stuff->window, client, DixGetAttrAccess);
#else
    pWin = SecurityLookupWindow(stuff->window, client, SecurityReadAccess);
    rc = pWin ? Success : BadWindow;
#endif

    if (rc != Success)
        return rc;

    pScreen = pWin->drawable.pScreen;
    pRoot = pScreen->root;

    rep = (xPanoramiXGetScreenSizeReply) {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0,
        .width = pRoot->drawable.width,
        .height = pRoot->drawable.height,
        .window = stuff->window,
        .screen = stuff->screen
    };
    if (client->swapped) {
        swaps(&rep.sequenceNumber, n);
        swapl(&rep.length, n);
        swapl(&rep.width, n);
        swapl(&rep.height, n);
        swapl(&rep.window, n);
        swapl(&rep.screen, n);
    }
    WriteToClient(client, sizeof(xPanoramiXGetScreenSizeReply), (char *) &rep);
    return Success;
}

int
ProcRRXineramaIsActive(ClientPtr client)
{
    xXineramaIsActiveReply rep;
    int n;

    REQUEST_SIZE_MATCH(xXineramaIsActiveReq);

    rep = (xXineramaIsActiveReply) {
        .type = X_Reply,
        .length = 0,
        .sequenceNumber = client->sequence,
        .state = RRXineramaScreenActive(screenInfo.screens[RR_XINERAMA_SCREEN])
    };
    if (client->swapped) {
        swaps(&rep.sequenceNumber, n);
        swapl(&rep.length, n);
        swapl(&rep.state, n);
    }
    WriteToClient(client, sizeof(xXineramaIsActiveReply), (char *) &rep);
    return Success;
}

static void
RRXineramaWriteMonitor(ClientPtr client, RRMonitorPtr monitor)
{
    xXineramaScreenInfo scratch;

    scratch.x_org = monitor->geometry.box.x1;
    scratch.y_org = monitor->geometry.box.y1;
    scratch.width = monitor->geometry.box.x2 - monitor->geometry.box.x1;
    scratch.height = monitor->geometry.box.y2 - monitor->geometry.box.y1;

    WriteToClient(client, sz_XineramaScreenInfo, (char *) &scratch);
}

int
ProcRRXineramaQueryScreens(ClientPtr client)
{
    xXineramaQueryScreensReply rep;
    ScreenPtr pScreen = screenInfo.screens[RR_XINERAMA_SCREEN];
    int m;
    RRMonitorPtr monitors = NULL;
    int nmonitors = 0;
    int n;

    REQUEST_SIZE_MATCH(xXineramaQueryScreensReq);

    if (RRXineramaScreenActive(pScreen)) {
        RRGetInfo(pScreen, FALSE);
        if (!RRMonitorMakeList(pScreen, TRUE, &monitors, &nmonitors))
            return BadAlloc;
    }

    rep = (xXineramaQueryScreensReply) {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = bytes_to_int32(nmonitors * sz_XineramaScreenInfo),
        .number = nmonitors
    };
    if (client->swapped) {
        swaps(&rep.sequenceNumber, n);
        swapl(&rep.length, n);
        swapl(&rep.number, n);
    }
    WriteToClient(client, sizeof(xXineramaQueryScreensReply), (char *) &rep);

    for (m = 0; m < nmonitors; m++)
        RRXineramaWriteMonitor(client, &monitors[m]);

    if (monitors)
        RRMonitorFreeList(monitors, nmonitors);

    return Success;
}

static int
ProcRRXineramaDispatch(ClientPtr client)
{
    REQUEST(xReq);
    switch (stuff->data) {
    case X_PanoramiXQueryVersion:
        return ProcRRXineramaQueryVersion(client);
    case X_PanoramiXGetState:
        return ProcRRXineramaGetState(client);
    case X_PanoramiXGetScreenCount:
        return ProcRRXineramaGetScreenCount(client);
    case X_PanoramiXGetScreenSize:
        return ProcRRXineramaGetScreenSize(client);
    case X_XineramaIsActive:
        return ProcRRXineramaIsActive(client);
    case X_XineramaQueryScreens:
        return ProcRRXineramaQueryScreens(client);
    }
    return BadRequest;
}

/* SProc */

static int
SProcRRXineramaQueryVersion(ClientPtr client)
{
    int n;

    REQUEST(xPanoramiXQueryVersionReq);
    swaps(&stuff->length, n);
    REQUEST_SIZE_MATCH(xPanoramiXQueryVersionReq);
    return ProcRRXineramaQueryVersion(client);
}

static int
SProcRRXineramaGetState(ClientPtr client)
{
    int n;

    REQUEST(xPanoramiXGetStateReq);
    swaps(&stuff->length, n);
    REQUEST_SIZE_MATCH(xPanoramiXGetStateReq);
    swapl(&stuff->window, n);
    return ProcRRXineramaGetState(client);
}

static int
SProcRRXineramaGetScreenCount(ClientPtr client)
{
    int n;

    REQUEST(xPanoramiXGetScreenCountReq);
    swaps(&stuff->length, n);
    REQUEST_SIZE_MATCH(xPanoramiXGetScreenCountReq);
    swapl(&stuff->window, n);
    return ProcRRXineramaGetScreenCount(client);
}

static int
SProcRRXineramaGetScreenSize(ClientPtr client)
{
    int n;

    REQUEST(xPanoramiXGetScreenSizeReq);
    swaps(&stuff->length, n);
    REQUEST_SIZE_MATCH(xPanoramiXGetScreenSizeReq);
    swapl(&stuff->window, n);
    swapl(&stuff->screen, n);
    return ProcRRXineramaGetScreenSize(client);
}

static int
SProcRRXineramaIsActive(ClientPtr client)
{
    int n;

    REQUEST(xXineramaIsActiveReq);
    swaps(&stuff->length, n);
    REQUEST_SIZE_MATCH(xXineramaIsActiveReq);
    return ProcRRXineramaIsActive(client);
}

static int
SProcRRXineramaQueryScreens(ClientPtr client)
{
    int n;

    REQUEST(xXineramaQueryScreensReq);
    swaps(&stuff->length, n);
    REQUEST_SIZE_MATCH(xXineramaQueryScreensReq);
    return ProcRRXineramaQueryScreens(client);
}

int
SProcRRXineramaDispatch(ClientPtr client)
{
    REQUEST(xReq);
    switch (stuff->data) {
    case X_PanoramiXQueryVersion:
        return SProcRRXineramaQueryVersion(client);
    case X_PanoramiXGetState:
        return SProcRRXineramaGetState(client);
    case X_PanoramiXGetScreenCount:
        return SProcRRXineramaGetScreenCount(client);
    case X_PanoramiXGetScreenSize:
        return SProcRRXineramaGetScreenSize(client);
    case X_XineramaIsActive:
        return SProcRRXineramaIsActive(client);
    case X_XineramaQueryScreens:
        return SProcRRXineramaQueryScreens(client);
    }
    return BadRequest;
}

void
RRXineramaExtensionInit(void)
{
#ifdef PANORAMIX
    if (!noPanoramiXExtension)
        return;
#endif

    if (noRRXineramaExtension)
        return;

    /*
     * Xinerama isn't capable enough to have multiple protocol screens each
     * with their own output geometry.  So if there's more than one protocol
     * screen, just don't even try.
     */
    if (screenInfo.numScreens > 1)
        return;

    (void) AddExtension(PANORAMIX_PROTOCOL_NAME, 0, 0,
                        ProcRRXineramaDispatch,
                        SProcRRXineramaDispatch, NULL, StandardMinorOpcode);
}
