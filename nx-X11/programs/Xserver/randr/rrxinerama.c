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
/* FIXME: there's no difference between
   X11/extensions/panoramiXproto.h and panoramiXproto.h */
#ifndef NXAGENT_SERVER
#include <X11/extensions/panoramiXproto.h>
#else
#include <X11/Xlib.h>
#include <X11/extensions/Xinerama.h>
#include <../hw/nxagent/Windows.h>
#include "panoramiXproto.h"
#endif

#define RR_XINERAMA_MAJOR_VERSION   1
#define RR_XINERAMA_MINOR_VERSION   1

/* Xinerama is not multi-screen capable; just report about screen 0 */
#define RR_XINERAMA_SCREEN  0

static int ProcRRXineramaQueryVersion(ClientPtr client);
static int ProcRRXineramaGetState(ClientPtr client);
static int ProcRRXineramaGetScreenCount(ClientPtr client);
static int ProcRRXineramaGetScreenSize(ClientPtr client);
static int ProcRRXineramaIsActive(ClientPtr client);
static int ProcRRXineramaQueryScreens(ClientPtr client);
#ifdef NXAGENT_SERVER
static int ProcRRXineramaQueryScreens_auto(ClientPtr client);
static int ProcRRXineramaQueryScreens_orig(ClientPtr client);
static int ProcRRXineramaQueryScreens_file(ClientPtr client, FILE *fptr );
#endif
static int SProcRRXineramaDispatch(ClientPtr client);

#ifdef NXAGENT_SERVER
extern Display *nxagentDisplay;
extern Window nxagentDefaultWindows[];
#endif

#undef DEBUG

/* Proc */

int
ProcRRXineramaQueryVersion(ClientPtr client)
{
    xPanoramiXQueryVersionReply	  rep;
    register int		  n;

    REQUEST_SIZE_MATCH(xPanoramiXQueryVersionReq);
    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    rep.majorVersion = RR_XINERAMA_MAJOR_VERSION;
    rep.minorVersion = RR_XINERAMA_MINOR_VERSION;
    if(client->swapped) {
        swaps(&rep.sequenceNumber, n);
        swapl(&rep.length, n);
        swaps(&rep.majorVersion, n);
        swaps(&rep.minorVersion, n);
    }
    WriteToClient(client, sizeof(xPanoramiXQueryVersionReply), (char *)&rep);
    return (client->noClientException);
}

int
ProcRRXineramaGetState(ClientPtr client)
{
    REQUEST(xPanoramiXGetStateReq);
    WindowPtr			pWin;
    xPanoramiXGetStateReply	rep;
    register int		n, rc;
    ScreenPtr			pScreen;
    rrScrPrivPtr		pScrPriv;
    Bool			active = FALSE;

    REQUEST_SIZE_MATCH(xPanoramiXGetStateReq);
    #ifndef NXAGENT_SERVER
    rc = dixLookupWindow(&pWin, stuff->window, client, DixGetAttrAccess);
    #else
    pWin = SecurityLookupWindow(stuff->window, client, SecurityReadAccess);
    rc = pWin ? Success : BadWindow;
    #endif
    if(rc != Success)
	return rc;

    pScreen = pWin->drawable.pScreen;
    pScrPriv = rrGetScrPriv(pScreen);
    if (pScrPriv)
    {
	/* XXX do we need more than this? */
	active = TRUE;
    }

    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    rep.state = active;
    rep.window = stuff->window;
    if(client->swapped) {
       swaps (&rep.sequenceNumber, n);
       swapl (&rep.length, n);
       swapl (&rep.window, n);
    }
    WriteToClient(client, sizeof(xPanoramiXGetStateReply), (char *)&rep);
    return client->noClientException;
}

static Bool
RRXineramaCrtcActive (RRCrtcPtr crtc)
{
    return crtc->mode != NULL && crtc->numOutputs > 0;
}

static int
RRXineramaScreenCount (ScreenPtr pScreen)
{
    int	i, n;

    n = 0;
    if (rrGetScrPriv (pScreen))
    {
	rrScrPriv(pScreen);
	for (i = 0; i < pScrPriv->numCrtcs; i++)
	    if (RRXineramaCrtcActive (pScrPriv->crtcs[i]))
		n++;
    }
    return n;
}

static Bool
RRXineramaScreenActive (ScreenPtr pScreen)
{
    return RRXineramaScreenCount (pScreen) > 0;
}

int
ProcRRXineramaGetScreenCount(ClientPtr client)
{
    REQUEST(xPanoramiXGetScreenCountReq);
    WindowPtr				pWin;
    xPanoramiXGetScreenCountReply	rep;
    register int			n, rc;

    REQUEST_SIZE_MATCH(xPanoramiXGetScreenCountReq);
    #ifndef NXAGENT_SERVER
    rc = dixLookupWindow(&pWin, stuff->window, client, DixGetAttrAccess);
    #else
    pWin = SecurityLookupWindow(stuff->window, client, SecurityReadAccess);
    rc = pWin ? Success : BadWindow;
    #endif
    if (rc != Success)
	return rc;

    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    rep.ScreenCount = RRXineramaScreenCount (pWin->drawable.pScreen);
    rep.window = stuff->window;
    if(client->swapped) {
       swaps(&rep.sequenceNumber, n);
       swapl(&rep.length, n);
       swapl(&rep.window, n);
    }
    WriteToClient(client, sizeof(xPanoramiXGetScreenCountReply), (char *)&rep);
    return client->noClientException;
}

int
ProcRRXineramaGetScreenSize(ClientPtr client)
{
    REQUEST(xPanoramiXGetScreenSizeReq);
    WindowPtr				pWin, pRoot;
    ScreenPtr				pScreen;
    xPanoramiXGetScreenSizeReply	rep;
    register int			n, rc;

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
    pRoot = WindowTable[pScreen->myNum];

    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    rep.width  = pRoot->drawable.width;
    rep.height = pRoot->drawable.height;
    rep.window = stuff->window;
    rep.screen = stuff->screen;
    if(client->swapped) {
       swaps(&rep.sequenceNumber, n);
       swapl(&rep.length, n);
       swapl(&rep.width, n);
       swapl(&rep.height, n);
       swapl(&rep.window, n);
       swapl(&rep.screen, n);
    }
    WriteToClient(client, sizeof(xPanoramiXGetScreenSizeReply), (char *)&rep);
    return client->noClientException;
}

int
ProcRRXineramaIsActive(ClientPtr client)
{
    xXineramaIsActiveReply	rep;

    REQUEST_SIZE_MATCH(xXineramaIsActiveReq);

    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
#ifndef NXAGENT_SERVER
    rep.state = RRXineramaScreenActive (screenInfo.screens[RR_XINERAMA_SCREEN]);
#else
    /* FIXME: cache that information? */
    if (nxagentOption(Rootless) == 1) {
      /* In rootless mode we do not need any xinerama stuff in the
	 nxagent. Xinerama is only relevant for the client side then. */
      rep.state = FALSE;
    }
    else {
      /* ask remote X server for that information */
      rep.state = !XineramaIsActive(nxagentDisplay);
    }
#ifdef DEBUG
    if (rep.state) {
      fprintf(stderr, "ProcRRXineramaIsActive: remote Xinerama on display %s is active\n", XDisplayString(nxagentDisplay));
    } else {
      fprintf(stderr, "ProcRRXineramaIsActive: remote Xinerama on display %s is not active\n", XDisplayString(nxagentDisplay));
    }
#endif
#endif
    if(client->swapped) {
	register int n;
	swaps(&rep.sequenceNumber, n);
	swapl(&rep.length, n);
	swapl(&rep.state, n);
    }
    WriteToClient(client, sizeof(xXineramaIsActiveReply), (char *) &rep);

    return client->noClientException;
}

/* Helper, send the number of xinerama screens to the client */
void writeNumScreens(ClientPtr client, int number)
{
    xXineramaQueryScreensReply rep;

#ifdef DEBUG
    fprintf(stderr, "writeNumScreens: number=%d\n", number);
#endif

    rep.type = X_Reply;
    rep.sequenceNumber = client->sequence;
    rep.number = number;
    rep.length = rep.number * sz_XineramaScreenInfo >> 2;
    if (client->swapped) {
        register int n;
	swaps (&rep.sequenceNumber, n);
	swapl (&rep.length, n);
	swapl (&rep.number, n);
    }
    WriteToClient (client, sizeof (xXineramaQueryScreensReply), (char *) &rep);
}

/* Helper, send one xinerama screen to the client */
void writeOneScreen(ClientPtr client, int x, int y, int w, int h)
{
    xXineramaScreenInfo scratch;

#ifdef DEBUG
    fprintf(stderr, "writeOneScreen: x=%d, y=%d, w=%d, h=%d)\n", x, y, w, h);
#endif
    scratch.x_org  = x;
    scratch.y_org  = y;
    scratch.width  = w;
    scratch.height = h;
    if(client->swapped) {
        register int n;
	swaps(&scratch.x_org, n);
	swaps(&scratch.y_org, n);
	swaps(&scratch.width, n);
	swaps(&scratch.height, n);
    }
    WriteToClient(client, sz_XineramaScreenInfo, (char *)&scratch);
}

/* FIXME: there must be such macros somewhere already...*/
#define MAX(a,b) ((a) > (b)) ? (a) : (b);
#define MIN(a,b) ((a) < (b)) ? (a) : (b);

/* intersect two rectangles */
Bool intersect(int ax1, int ay1, unsigned int aw, unsigned int ah,
	       int bx1, int by1, unsigned int bw, unsigned int bh,
	       int *x, int *y, unsigned int *w, unsigned int *h)
{
    int tx1, ty1, tx2, ty2, ix, iy;
    unsigned int iw, ih;

    int ax2 = ax1 + aw;
    int ay2 = ay1 + ah;
    int bx2 = bx1 + bw;
    int by2 = by1 + bh;

    /* thanks to http://silentmatt.com/rectangle-intersection */

    /* check if there's any intersection at all */
    if (ax2 < bx1 || bx2 < ax1 || ay2 < by1 || by2 < ay1) {
        return FALSE;
    }

    tx1 = MAX(ax1, bx1);
    ty1 = MAX(ay1, by1);
    tx2 = MIN(ax2, bx2);
    ty2 = MIN(ay2, by2);

    ix = tx1 - ax1;
    iy = ty1 - ay1;
    iw = tx2 - tx1;
    ih = ty2 - ty1;

    /* check if the resulting rectangle is feasible */
    if (iw <= 0 || ih <= 0) {
        return FALSE;
    }
    *x = ix;
    *y = iy;
    *w = iw;
    *h = ih;
    return TRUE;
}

/* second Dispatcher to determine what xinerama code to use */
int
ProcRRXineramaQueryScreens(ClientPtr client)
{
#ifndef NXAGENT_SERVER
    return ProcRRXineramaQueryScreens_orig(client);
#else

    char* envvar = getenv("NX_XINERAMA_CONF");
    int res;
    /* if env is not set use auto_xinerama
       if env is set to "auto" use auto_xinerama
       if env point to existing file use file_xinerama
       else use original code
    */

#ifdef DEBUG
    if (envvar) {
        fprintf(stderr, "ProcRRXineramaQueryScreens: envvar NX_XINERAMA_CONF is set to '%s'\n", envvar);
    } else {
        fprintf(stderr, "ProcRRXineramaQueryScreens: envvar NX_XINERAMA_CONF is not set\n");
    }
#endif

    if (!envvar || strncmp("auto", envvar, 4) == 0) {
        res = ProcRRXineramaQueryScreens_auto(client);
    } else {
        FILE* fptr;
	if((fptr=fopen(envvar,"r"))) {
            res = ProcRRXineramaQueryScreens_file(client, fptr);
        } else {
            res = ProcRRXineramaQueryScreens_orig(client);
        }
    }
    return res;
#endif
}


/* determine the position and size of the nxproxy window */
/* FIXME: is border_width relevant here? */
BOOL getWindowGeometry(Display *display, Window win, int *x_org, int *y_org, unsigned int *width, unsigned int *height)
{
    int x,y, ox, oy;
    unsigned int w, h, border_width, depth;
    Status stat;
    Window root = NULL;
    Window child = NULL;

    x = y = 0;
    w = h = border_width = depth = 0;

    stat = XGetGeometry(display, win, &root, &x, &y, &w, &h, &border_width, &depth);
    if (stat) {
#ifdef DEBUG
        fprintf(stderr, "getGeometry: stat=%d, root=0x%x, x=%d, y=%d, width=%d, height=%d, border_width=%d, DISPLAY='%s'\n",
	        stat, root, x, y, w, h, border_width, XDisplayString(display));
#endif
        *width = w;
        *height = h;
    } else {
#ifdef DEBUG
        fprintf(stderr, "getGeometry: Cannot determine geometry for window 0x%x\n", win);
#endif
        return FALSE;
    }

    /* Get coordinates of the nxproxy window relative to
       the root window */
    if (!XTranslateCoordinates(display, win, root, 0, 0, &ox, &oy, &child)) {
        /* rc=FALSE means the window and root are on different
	   screens. This cannot happen as we got the root window
	   from the XGetGeometry call above so win MUST
	   reside on that root window.
	   FIXME: We skip this for now as it is unclear what to
	   do in this situation.*/
       ;
#ifdef DEBUG
       fprintf(stderr, "getGeometry: XTranslateCoordinates() failed\n");
#endif
    } else {
#ifdef DEBUG
        fprintf(stderr, "getGeometry: XTranslateCoordinates() result: x_org=%d, offset_y=%d\n", ox, oy);
#endif
        *x_org = ox;
        *y_org = oy;
    }

    return TRUE;
}


int
ProcRRXineramaQueryScreens_auto(ClientPtr client)
{
#ifdef DEBUG
    fprintf(stderr, "ProcRRXineramaQueryScreens_auto: entry\n");
#endif
    XineramaScreenInfo *screeninfo;

    /* these are used to store the xinerama screens */
    static XineramaScreenInfo *cache_screeninfo = NULL;
    static int cache_number = -1;

    int number;
    int i;

    REQUEST_SIZE_MATCH(xXineramaQueryScreensReq);

    /* This call will fail if nxagent is in disconnected mode. For now we
       try to use cached values then. */
    /* FIXME: if nxcomp could answer this call from its cache we could drop
       this code here */
    if ((screeninfo = XineramaQueryScreens(nxagentDisplay, &number)) == NULL) {
#ifdef DEBUG
        fprintf(stderr, "ProcRRXineramaQueryScreens_auto: remote XineramaQueryScreens() for display %s failed\n", XDisplayString(nxagentDisplay));
#endif
	/* if we have a cached result use that */
	if (cache_number > -1) {
#ifdef DEBUG
	    fprintf(stderr, "ProcRRXineramaQueryScreens_auto: using cached values\n");
#endif

	    writeNumScreens(client, cache_number);
	    for (i = 0; i < cache_number; i++) {
	        writeOneScreen(client, cache_screeninfo[i].x_org,
			               cache_screeninfo[i].y_org,
                                       cache_screeninfo[i].width,
                                       cache_screeninfo[i].height);
            }
        } else {
#ifdef DEBUG
            fprintf(stderr, "ProcRRXineramaQueryScreens_auto: cache is empty\n");
#endif
            /* we report "no xinerama screens found" */
	    writeNumScreens(client, 0);
        }
        return client->noClientException;
    }

#ifdef DEBUG
    fprintf(stderr, "ProcRRXineramaQueryScreens_auto: remote XineramaQueryScreens() for display %s was successful, returned %d screens\n", XDisplayString(nxagentDisplay), number);
#endif

    if (number) {
        int x_org, y_org;
	unsigned int width, height;
	int count = 0;

	/* nxagentDefaultWindows[0] is the window that is referred to in
	   hw/nxagent/Extensions.c:nxagentRandRSetWindowsSize() and in
	   this file's header, too. So this should be correct. This
	   window is NULL when nxagent is in disconnected state, but if
	   we reach this point it should always be set. Nevertheless we
	   do some sanity checks */
	if (!nxagentDefaultWindows[RR_XINERAMA_SCREEN]) {
	    /* should not happen... */
#ifdef DEBUG
	    fprintf(stderr, "ProcRRXineramaQueryScreens_auto: nxagentDefaultWindows[RR_XINERAMA_SCREEN] is NULL\n");
#endif
	    number = 0;
	} else {
            x_org = y_org = 0; /* get rid of compiler warning */
	    if (!getWindowGeometry(nxagentDisplay, nxagentDefaultWindows[RR_XINERAMA_SCREEN], &x_org, &y_org, &width, &height)) {
	        /* should also not happen... */
	        fprintf(stderr, "ProcRRXineramaQueryScreens_auto: could not determine pos and size of nxagentDefaultWindows[RR_XINERAMA_SCREEN]\n");
		number = 0;
	    }
	}

        /* We need to send the number of screens prior to the
	   screens. But we do not want to send screens that are outside
	   the nxproxy window so we do not know the right number
	   here. So we update the screeninfo structures and count the
	   number. After the loop we send the updated structures.  NOTE:
	   If we skip screens the caches screeninfo will be slightly
	   bigger than needed! But for ease of implementation this is
	   tolerable.
	*/
	for (i = 0; i < number; i++) {
	    int new_x, new_y;
	    unsigned int new_w, new_h;

	    /* if there's no intersection we skip that screen */
	    if (!intersect(x_org, y_org,
                           width, height,
                           screeninfo[i].x_org, screeninfo[i].y_org,
                           screeninfo[i].width, screeninfo[i].height,
			   &new_x, &new_y, &new_w, &new_h)) {
#ifdef DEBUG
	        fprintf(stderr, "ProcRRXineramaQueryScreens_auto: no (valid) intersection - skipping screen %d'\n", i);
#endif
		continue;
	    }

	    /* store the new values for reply/cache */
	    screeninfo[count].x_org  = new_x;
	    screeninfo[count].y_org  = new_y;
	    screeninfo[count].width  = new_w;
	    screeninfo[count].height = new_h;
	    count++;

#ifdef DEBUG
	    fprintf(stderr, "ProcRRXineramaQueryScreens_auto: intersected screen is x=%d, y=%d, width=%d, height=%d'\n", new_x, new_y, new_w, new_h);
#endif
	} /* end for */

	/* correct the number of screens we determined to be valid/of
	   interest for us (for reply and cache)*/
	number = count;
    } /* end if (number) */

    /* we have checked all screens, now answer with the valid ones */
    writeNumScreens(client, number);
    for (i = 0; i < number; i++) {
        writeOneScreen(client, screeninfo[i].x_org, screeninfo[i].y_org, screeninfo[i].width, screeninfo[i].height);
    }

    /* now cache the result */
    {
        /* invalidate exiting cache */
        if (cache_screeninfo != NULL) {
            XFree(cache_screeninfo);
	    cache_screeninfo = NULL;
        }

#ifdef DEBUG
        fprintf(stderr, "ProcRRXineramaQueryScreens_auto: caching calculated screeninfo\n");
#endif
        cache_screeninfo = screeninfo;
        cache_number = number;
    }

    return client->noClientException;
}


int
ProcRRXineramaQueryScreens_file(ClientPtr client, FILE* fptr)
{
#ifdef DEBUG
    fprintf(stderr, "ProcRRXineramaQueryScreens_file: entry\n");
#endif
    int i;
    int x,y,w,h;
    int number = 0;

    REQUEST_SIZE_MATCH(xXineramaQueryScreensReq);

    if (fptr) {
        while (!feof(fptr)) {
            w = h = 0;
	    fscanf(fptr,"%d %d %d %d",&x,&y,&w,&h);
	    if (w && h)
	        number++;
	}
	rewind(fptr);

	writeNumScreens(client, number);

	for (i = 0; i < number; i++) {
	    while (!feof(fptr)){
	        w = h = 0;
		fscanf(fptr,"%d %d %d %d", &x, &y, &w, &h);
		if (w && h) {
	            writeOneScreen(client, x, y, w, h);
		}
	    }
	}
	fclose(fptr);
    } else {
        /* report "no xinerama screens" */
        writeNumScreens(client, 0);
    }
    return client->noClientException;
}

int
ProcRRXineramaQueryScreens_orig(ClientPtr client)
{
#ifdef DEBUG
    fprintf(stderr, "ProcRRXineramaQueryScreens_orig: entry\n");
#endif
    ScreenPtr	pScreen = screenInfo.screens[RR_XINERAMA_SCREEN];
    int number;

    REQUEST_SIZE_MATCH(xXineramaQueryScreensReq);

    if (RRXineramaScreenActive (pScreen))
    {
	rrScrPriv(pScreen);
	if (pScrPriv->numCrtcs == 0 || pScrPriv->numOutputs == 0)
	    RRGetInfo (pScreen);
    }

    number = RRXineramaScreenCount (pScreen);
    writeNumScreens(client, number);

    if(number) {
	rrScrPriv(pScreen);
	int i;

	for(i = 0; i < pScrPriv->numCrtcs; i++) {
	    RRCrtcPtr	crtc = pScrPriv->crtcs[i];
	    if (RRXineramaCrtcActive (crtc))
	    {
	        int width, height;
		RRCrtcGetScanoutSize (crtc, &width, &height);
		writeOneScreen(client, crtc->x, crtc->y, width, height);
	    }
	}
    }

    return client->noClientException;
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
SProcRRXineramaQueryVersion (ClientPtr client)
{
    REQUEST(xPanoramiXQueryVersionReq);
    register int n;
    swaps(&stuff->length,n);
    REQUEST_SIZE_MATCH (xPanoramiXQueryVersionReq);
    return ProcRRXineramaQueryVersion(client);
}

static int
SProcRRXineramaGetState(ClientPtr client)
{
    REQUEST(xPanoramiXGetStateReq);
    register int n;
    swaps (&stuff->length, n);
    REQUEST_SIZE_MATCH(xPanoramiXGetStateReq);
    swapl (&stuff->window, n);
    return ProcRRXineramaGetState(client);
}

static int
SProcRRXineramaGetScreenCount(ClientPtr client)
{
    REQUEST(xPanoramiXGetScreenCountReq);
    register int n;
    swaps (&stuff->length, n);
    REQUEST_SIZE_MATCH(xPanoramiXGetScreenCountReq);
    swapl (&stuff->window, n);
    return ProcRRXineramaGetScreenCount(client);
}

static int
SProcRRXineramaGetScreenSize(ClientPtr client)
{
    REQUEST(xPanoramiXGetScreenSizeReq);
    register int n;
    swaps (&stuff->length, n);
    REQUEST_SIZE_MATCH(xPanoramiXGetScreenSizeReq);
    swapl (&stuff->window, n);
    swapl (&stuff->screen, n);
    return ProcRRXineramaGetScreenSize(client);
}

static int
SProcRRXineramaIsActive(ClientPtr client)
{
    REQUEST(xXineramaIsActiveReq);
    register int n;
    swaps (&stuff->length, n);
    REQUEST_SIZE_MATCH(xXineramaIsActiveReq);
    return ProcRRXineramaIsActive(client);
}

static int
SProcRRXineramaQueryScreens(ClientPtr client)
{
    REQUEST(xXineramaQueryScreensReq);
    register int n;
    swaps (&stuff->length, n);
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

static void
RRXineramaResetProc(ExtensionEntry* extEntry)
{
}

void
RRXineramaExtensionInit(void)
{
#ifdef PANORAMIX
    if(!noPanoramiXExtension)
	return;
#endif

    /*
     * Xinerama isn't capable enough to have multiple protocol screens each
     * with their own output geometry.  So if there's more than one protocol
     * screen, just don't even try.
     */
    if (screenInfo.numScreens > 1)
	return;

    (void) AddExtension(PANORAMIX_PROTOCOL_NAME, 0,0,
			ProcRRXineramaDispatch,
			SProcRRXineramaDispatch,
			RRXineramaResetProc,
			StandardMinorOpcode);
}
