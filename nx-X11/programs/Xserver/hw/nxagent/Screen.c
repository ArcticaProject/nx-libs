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

Copyright 1993 by Davor Matic

Permission to use, copy, modify, distribute, and sell this software
and its documentation for any purpose is hereby granted without fee,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation.  Davor Matic makes no representations about
the suitability of this software for any purpose.  It is provided "as
is" without express or implied warranty.

*/

/*
 * Used by the auto-disconnect feature.
 */

#include <signal.h>
#include <stdarg.h>
#include <math.h>
#include <float.h>
#include <time.h>

#include "scrnintstr.h"
#include "dix.h"
#include "dixstruct.h"
#include "mi.h"
#include "micmap.h"
#include "colormapst.h"
#include "resource.h"
#include "mipointer.h"
#include "../../fb/fb.h"
#include "../../randr/randrstr.h"
#include "inputstr.h"
#include "mivalidate.h"
#include "list.h"

#include "Agent.h"
#include "Display.h"
#include "Screen.h"
#include "Extensions.h"
#include "Atoms.h"
#include "GCs.h"
#include "GCOps.h"
#include "Image.h"
#include "Drawable.h"
#include "Font.h"
#include "Colormap.h"
#include "Cursor.h"
#include "Visual.h"
#include "Events.h"
#include "Init.h"
#include "Args.h"
#include "Client.h"
#include "Options.h"
#include "Splash.h"
#include "Holder.h"
#include "Render.h"
#include "Trap.h"
#include "Keyboard.h"
#include "Pointer.h"
#include "Reconnect.h"
#include "Composite.h"
#include <nx/Shadow.h>
#include "Utils.h"

#include <nx-X11/Xlib.h>
#include "X11/include/Xinerama_nxagent.h"


#define GC     XlibGC
#define Font   XlibFont
#define KeySym XlibKeySym
#define XID    XlibXID
#include <nx-X11/Xlibint.h>
#undef  GC
#undef  Font
#undef  KeySym
#undef  XID

#include "Xatom.h"
#include "Xproto.h"

#include "compext/Compext.h"

#include "mibstorest.h"

/*
 * Set here the required log level.
 */

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG
#undef  WATCH
#undef  DUMP

/*
 * Display a pixmap on an shadow
 * display used for debug.
 */

#ifdef DUMP

void nxagentShowPixmap(PixmapPtr pPixmap, int x, int y, int width, int height);

void nxagentFbRestoreArea(PixmapPtr pPixmap, WindowPtr pWin, int xSrc, int ySrc, int width,
                              int height, int xDst, int yDst)
#endif

#ifdef WATCH
#include "unistd.h"
#endif

extern Bool nxagentIpaq;
extern Pixmap nxagentIconPixmap;
extern Pixmap nxagentIconShape;
extern Bool useXpmIcon;

extern Bool nxagentReportWindowIds;

Window nxagentDefaultWindows[MAXSCREENS];
Window nxagentInputWindows[MAXSCREENS];
Window nxagentScreenSaverWindows[MAXSCREENS];

#ifdef NXAGENT_ONSTART
Atom nxagentWMStart;
Window nxagentSplashWindow = None;
Pixmap nxagentPixmapLogo;
#endif

ScreenPtr nxagentDefaultScreen = NULL;
int nxagentArgc = 0;
char **nxagentArgv = NULL;

#ifdef NXAGENT_ARTSD

char mcop_atom[] = "MCOPGLOBALS";
Atom mcop_local_atom = None;
unsigned char fromHexNibble(char c);
void nxagentPropagateArtsdProperties(ScreenPtr pScreen, char *port);

#endif

Window nxagentIconWindow = None;
Window nxagentFullscreenWindow = None;

#ifdef VIEWPORT_FRAME

WindowPtr nxagentViewportFrameLeft;
WindowPtr nxagentViewportFrameRight;
WindowPtr nxagentViewportFrameAbove;
WindowPtr nxagentViewportFrameBelow;

#endif /* #ifdef VIEWPORT_FRAME */

Bool nxagentCreateScreenResources(ScreenPtr pScreen);
void nxagentPrintAgentGeometry(char *hdrMessage, char *prefix);


/*
 * These variables are for shadowing feature.
 */

int nxagentShadowResize = 0;
 
WindowPtr nxagentShadowWindowPtr = NULL;

static XID           accessPixmapID;
static Window        accessWindowID;
static int           imageByteOrder;
static unsigned char nxagentMasterDepth;
static unsigned char nxagentCheckDepth = 0;
static unsigned int  nxagentBppShadow;
static unsigned int  nxagentBppMaster;
int                  nxagentShadowXConnectionNumber;
GCPtr                nxagentShadowGCPtr = NULL;
PixmapPtr            nxagentShadowPixmapPtr = NULL;
char *               nxagentShadowBuffer;
unsigned char        nxagentShadowDepth;
int                  nxagentShadowWidth;
int                  nxagentShadowHeight;
Display *            nxagentShadowDisplay;
short                nxagentShadowUid = -1;

void nxagentShadowAdaptDepth(unsigned int, unsigned int, unsigned int, char **);

RegionRec nxagentShadowUpdateRegion;

#define NXAGENT_DEFAULT_DPI 96
#define NXAGENT_AUTO_DPI -1

extern Bool nxagentAutoDPI;

/*
 * From randr/randr.c. This was originally static
 * but we need it here.
 */

int TellChanged(WindowPtr pWin, void * value);

int nxagentBitsPerPixel(int depth)
{
    if (depth == 1) return 1;
    else if (depth <= 8) return 8;
    else if (depth <= 16) return 16;
    else return 32;
}

void nxagentSetScreenInfo(ScreenInfo *screenInfo)
{
  /*
   * Setup global screen info parameters. In the Xnest
   * server this stuff is done after having opened the
   * real display as Xnest lets the screen reflect the
   * order of the remote end. Agent will instead set
   * the order according to local endianness and swap
   * data whenever it is appropriate.
   *
   * From a standard implementation:
   *
   * screenInfo->imageByteOrder = IMAGE_BYTE_ORDER;
   * screenInfo->bitmapScanlinePad = BITMAP_SCANLINE_PAD;
   * screenInfo->bitmapScanlineUnit = BITMAP_SCANLINE_UNIT;
   * screenInfo->bitmapBitOrder = BITMAP_BIT_ORDER;
   *
   * From Xnest implementation:
   *
   * screenInfo -> imageByteOrder = ImageByteOrder(nxagentDisplay);
   * screenInfo -> bitmapScanlineUnit = BitmapUnit(nxagentDisplay);
   * screenInfo -> bitmapScanlinePad = BitmapPad(nxagentDisplay);
   * screenInfo -> bitmapBitOrder = BitmapBitOrder(nxagentDisplay);
   */

  screenInfo -> imageByteOrder = IMAGE_BYTE_ORDER;
  screenInfo -> bitmapScanlinePad = BITMAP_SCANLINE_PAD;
  screenInfo -> bitmapScanlineUnit = BITMAP_SCANLINE_UNIT;
  screenInfo -> bitmapBitOrder = BITMAP_BIT_ORDER;

  #ifdef TEST
  fprintf(stderr, "nxagentSetScreenInfo: Server image order is [%d] bitmap order is [%d].\n",
              screenInfo -> imageByteOrder, screenInfo -> bitmapBitOrder);

  fprintf(stderr, "nxagentSetScreenInfo: Server scanline unit is [%d] scanline pad is [%d].\n",
              screenInfo -> bitmapScanlineUnit, screenInfo -> bitmapScanlinePad);
  #endif
}

void nxagentSetPixmapFormats(ScreenInfo *screenInfo)
{
  int i;

  /*
   * Formats are created with no care of which are supported
   * on the real display. Creating only formats supported
   * by the remote end makes troublesome handling migration
   * of session from a display to another.
   */

  screenInfo -> numPixmapFormats = nxagentNumPixmapFormats;

  for (i = 0; i < nxagentNumPixmapFormats; i++)
  {
    screenInfo -> formats[i].depth = nxagentPixmapFormats[i].depth;
    screenInfo -> formats[i].bitsPerPixel = nxagentPixmapFormats[i].bits_per_pixel;
    screenInfo -> formats[i].scanlinePad = nxagentPixmapFormats[i].scanline_pad;

    #ifdef TEST
    fprintf(stderr, "nxagentSetPixmapFormats: Set format at index [%d] to depth [%d] "
                "bits per pixel [%d] scanline pad [%d].\n", i,
                    screenInfo -> formats[i].depth, screenInfo -> formats[i].bitsPerPixel,
                        screenInfo -> formats[i].scanlinePad);
    #endif
  }
}

void nxagentMinimizeFromFullScreen(ScreenPtr pScreen)
{
  XUnmapWindow(nxagentDisplay, nxagentFullscreenWindow);

  if (nxagentIpaq)
  {
    XMapWindow(nxagentDisplay, nxagentIconWindow);
    XIconifyWindow(nxagentDisplay, nxagentIconWindow,
                       DefaultScreen(nxagentDisplay));
  }
  else
  {
    XIconifyWindow(nxagentDisplay, nxagentIconWindow,
                       DefaultScreen(nxagentDisplay));
  }
}

void nxagentMaximizeToFullScreen(ScreenPtr pScreen)
{
  if (nxagentIpaq)
  {
    XUnmapWindow(nxagentDisplay, nxagentIconWindow);

    XMapWindow(nxagentDisplay, nxagentFullscreenWindow);
  }
  else
  {
/*
    XUnmapWindow(nxagentDisplay, nxagentIconWindow);
*/
/*
FIXME: We'll check for ReparentNotify and LeaveNotify events after XReparentWindow()
       in order to avoid the session window is iconified.
       We could avoid the session window is iconified when a LeaveNotify event is received,
       so this check would be unnecessary.
*/
    struct timeval timeout;
    int i;
    XEvent e;

    XReparentWindow(nxagentDisplay, nxagentFullscreenWindow,
                        RootWindow(nxagentDisplay, DefaultScreen(nxagentDisplay)), 0, 0);

    for (i = 0; i < 100 && nxagentWMIsRunning; i++)
    {
      #ifdef TEST
      fprintf(stderr, "nxagentMaximizeToFullscreen: WARNING! Going to wait for the ReparentNotify event.\n");
      #endif

      if (XCheckTypedWindowEvent(nxagentDisplay, nxagentFullscreenWindow, ReparentNotify, &e))
      {
        break;
      }

      XSync(nxagentDisplay, 0);

      timeout.tv_sec = 0;
      timeout.tv_usec = 50 * 1000;

      nxagentWaitEvents(nxagentDisplay, &timeout);
    }

    XMapRaised(nxagentDisplay, nxagentFullscreenWindow);

    XIconifyWindow(nxagentDisplay, nxagentIconWindow,
                       DefaultScreen(nxagentDisplay));

    while (XCheckTypedWindowEvent(nxagentDisplay, nxagentFullscreenWindow, LeaveNotify, &e));
/*
    XMapWindow(nxagentDisplay, nxagentIconWindow);
*/
  }
}

Window nxagentCreateIconWindow(void)
{
  XSetWindowAttributes attributes;
  unsigned long valuemask;
  char* window_name;
  XTextProperty windowName;
  XSizeHints* sizeHints;
  XWMHints* wmHints;
  Window w;
  Mask mask;

  /*
   * Create icon window.
   */

  attributes.override_redirect = False;
  attributes.colormap = DefaultColormap(nxagentDisplay, DefaultScreen(nxagentDisplay));
  attributes.background_pixmap = nxagentScreenSaverPixmap;
  valuemask = CWOverrideRedirect | CWBackPixmap | CWColormap;

  #ifdef TEST
  fprintf(stderr, "nxagentCreateIconWindow: Going to create new icon window.\n");
  #endif

  w = XCreateWindow(nxagentDisplay, DefaultRootWindow(nxagentDisplay),
                        0, 0, 1, 1, 0,
                            DefaultDepth(nxagentDisplay, DefaultScreen(nxagentDisplay)),
                                InputOutput,
                                    DefaultVisual(nxagentDisplay, DefaultScreen(nxagentDisplay)),
                                        valuemask, &attributes);

  if (nxagentReportWindowIds)
  {
    fprintf (stderr, "NXAGENT_WINDOW_ID: ICON_WINDOW,WID:[0x%x]\n", nxagentIconWindow);
  }
  #ifdef TEST
  fprintf(stderr, "nxagentCreateIconWindow: Created new icon window with id [0x%x].\n",
              nxagentIconWindow);
  #endif

  /*
   *  Set hints to the window manager for the icon window.
   */

  window_name = nxagentWindowName;
  XStringListToTextProperty(&window_name, 1, &windowName);

  if ((sizeHints = XAllocSizeHints()))
  {
    sizeHints->flags = PMinSize | PMaxSize;
    sizeHints->min_width = sizeHints->max_width = 1;
    sizeHints->min_height = sizeHints->max_height = 1;
  }

  if ((wmHints = XAllocWMHints()))
  {
    wmHints->flags = IconPixmapHint | IconMaskHint;
    wmHints->initial_state = IconicState;
    wmHints->icon_pixmap = nxagentIconPixmap;

    if (useXpmIcon)
    {
      wmHints->icon_mask = nxagentIconShape;
      wmHints->flags = IconPixmapHint | IconMaskHint;
    }
    else
    {
      wmHints->flags = StateHint | IconPixmapHint;
    }
  }

  XSetWMProperties(nxagentDisplay, w,
                      &windowName, &windowName,
                          NULL , 0 , sizeHints, wmHints, NULL);

  if (sizeHints)
    XFree(sizeHints);

  if (wmHints)
    XFree(wmHints);

  /*
   * Enable events from the icon window.
   */

  nxagentGetDefaultEventMask(&mask);

  XSelectInput(nxagentDisplay, w, (mask & ~(KeyPressMask |
                   KeyReleaseMask)) | StructureNotifyMask);

  /*
   * Notify to client if user closes icon window.
   */

  if (nxagentWMIsRunning && !nxagentOption(Rootless))
  {
    XlibAtom deleteWMAtom = nxagentAtoms[2]; /* WM_DELETE_WINDOW */

    XSetWMProtocols(nxagentDisplay, w, &deleteWMAtom, 1);
  }

  return w;
}

Bool nxagentMagicPixelZone(int x, int y)
{
  return (x >= nxagentOption(Width) - 1 && y < 1);
}

void nxagentSetScreenSaverTime(void)
{
  #ifdef TEST
  fprintf(stderr, "nxagentSetScreenSaverTime: ScreenSaverTime was [%lu], ScreenSaverInterval was [%lu].\n",
                  (long unsigned int)ScreenSaverTime, (long unsigned int)ScreenSaverInterval);
  #endif

  /*
   * More than one timeout could be used here,
   * to make use of screen-saver handler not
   * only for the timeout feature. In a case
   * like this, the lower timeout have to be
   * used as ScreenSaverTime.
   */

  if (nxagentAutoDisconnectTimeout > 0)
  {
    ScreenSaverTime = nxagentAutoDisconnectTimeout;
  }

  ScreenSaverInterval = ScreenSaverTime;

  #ifdef TEST
  fprintf(stderr, "nxagentSetScreenSaverTime: ScreenSaverTime now is [%lu], ScreenSaverInterval now is [%lu].\n",
                  (long unsigned int)ScreenSaverTime, (long unsigned int)ScreenSaverInterval);
  #endif
}

static Bool nxagentSaveScreen(ScreenPtr pScreen, int what)
{
  #ifdef TEST
  fprintf(stderr, "nxagentSaveScreen: Called for screen at [%p] with parameter [%d].\n",
              (void *) pScreen, what);

  fprintf(stderr, "nxagentSaveScreen: SCREEN_SAVER_ON is [%d] SCREEN_SAVER_OFF is [%d] "
              "SCREEN_SAVER_FORCER is [%d] SCREEN_SAVER_CYCLE is [%d].\n",
                  SCREEN_SAVER_ON, SCREEN_SAVER_OFF, SCREEN_SAVER_FORCER,
                      SCREEN_SAVER_CYCLE);
  #endif

  /*
   * We need only to reset the timeouts
   * in this case.
   */

  if (what == SCREEN_SAVER_OFF)
  {
    nxagentAutoDisconnectTimeout = nxagentOption(Timeout) * MILLI_PER_SECOND;

    return 1;
  }

  /*
   * The lastDeviceEventTime is updated every time
   * a device event is received, and it is used by
   * WaitForSomething() to know when the SaveScreens()
   * function should be called. This solution doesn't
   * take care of a pointer button not released, so
   * we have to handle this case by ourselves.
   */

/*
FIXME: Do we need to check the key grab if the
       autorepeat feature is disabled?
*/
  if (inputInfo.pointer -> button -> buttonsDown > 0)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentSaveScreen: Ignoring timeout, there is a pointer button down.\n");
    #endif

    /*
     * Returning 0 the SaveScreens() function
     * (which calls this one) tries to build
     * a screen-saver creating a new window.
     * We don't want this, so we return 1 in
     * any case.
     */

    return 1;
  }

  /*
   * Handling the auto-disconnect feature.
   * If there is any client attached and the persisten-
   * ce is allowed then leave the session running, else
   * terminate it. It should use something less brutal,
   * though raising a signal should ensure that the code
   * follows the usual execution path.
   */

  if (nxagentOption(Timeout) > 0)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentSaveScreen: Auto-disconnect timeout was [%d].\n",
                nxagentAutoDisconnectTimeout);
    #endif

    nxagentAutoDisconnectTimeout  -= ScreenSaverTime;

    #ifdef TEST
    fprintf(stderr, "nxagentSaveScreen: Auto-disconnect timeout is [%d].\n",
                nxagentAutoDisconnectTimeout);
    #endif

    if (nxagentSessionState == SESSION_UP &&
            nxagentAutoDisconnectTimeout <= 0)
    {
      nxagentAutoDisconnectTimeout = nxagentOption(Timeout) * MILLI_PER_SECOND;

      if (nxagentClients == 0)
      {
        fprintf(stderr, "Info: Terminating session with no client running.\n");

        raise(SIGTERM);
      }
      else if (nxagentOption(Persistent) == 0)
      {
        fprintf(stderr, "Info: Terminating session with persistence not allowed.\n");

        raise(SIGTERM);
      }
      else
      {
        fprintf(stderr, "Info: Suspending session with %d clients running.\n",
                    nxagentClients);

        raise(SIGHUP);
      }
    }
  }

  return 1;
}

Bool nxagentCreateScreenResources(ScreenPtr pScreen)
{
  Bool ret;

  CreatePixmapProcPtr savedCreatePixmap = pScreen->CreatePixmap;
  ModifyPixmapHeaderProcPtr savedModifyPixmapHeader = pScreen->ModifyPixmapHeader;

  pScreen->CreatePixmap = fbCreatePixmap;
  pScreen->ModifyPixmapHeader = miModifyPixmapHeader;
  ret = miCreateScreenResources(pScreen);

  pScreen->CreatePixmap = savedCreatePixmap;
  pScreen->ModifyPixmapHeader = savedModifyPixmapHeader;

  return ret;
}

static Bool nxagentCursorOffScreen(ScreenPtr *pPtrScreen, int *x, int *y)
{
  return False;
}

static void nxagentCrossScreen(ScreenPtr pScreen, Bool entering)
{
}

static miPointerScreenFuncRec nxagentPointerCursorFuncs =
{
  nxagentCursorOffScreen,
  nxagentCrossScreen,
  miPointerWarpCursor
};

#ifdef VIEWPORT_FRAME

void nxagentInitViewportFrame(ScreenPtr pScreen, WindowPtr pRootWin)
{
  int error = Success;
  VisualID visual = 0;
  int i;
  XID xid;

  if (nxagentOption(Rootless))
  {
    return;
  }

  for (i = 0; i < pScreen -> numDepths; i++)
  {
    if (pScreen -> allowedDepths[i].depth == pRootWin -> drawable.depth)
    {
      visual = pScreen -> allowedDepths[i].vids[0];
      break;
    }
  }

  /*
   * It is not necessary create the windows on the real X server. But this
   * windows are not visible. Create them it is not a great effort, and avoids
   * many errors.
   *
   *  nxagentScreenTrap = True;
   */

  xid = FakeClientID(serverClient -> index);

  #ifdef TEST
  fprintf(stderr, "nxagentInitViewportFrame: XID = [%lx]\n", xid);
  #endif

  nxagentViewportFrameLeft = CreateWindow(xid, pRootWin, -NXAGENT_FRAME_WIDTH, 0, NXAGENT_FRAME_WIDTH,
                                              pRootWin -> drawable.height,
                                                  0, InputOutput, 0, NULL,
                                                      pRootWin -> drawable.depth,
                                                          serverClient, visual, &error);

  AddResource(xid, RT_WINDOW, (void *) nxagentViewportFrameLeft);

  if (error != Success)
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentInitViewportFrame: Error creating nxagentViewportFrameLeft.\n");
    #endif

    error = Success;
  }

  xid = FakeClientID(serverClient -> index);

  #ifdef TEST
  fprintf(stderr, "nxagentInitViewportFrame: XID = [%lx]\n", xid);
  #endif

  nxagentViewportFrameRight = CreateWindow(xid, pRootWin, pRootWin -> drawable.width, 0,
                                               NXAGENT_FRAME_WIDTH,
                                                   pRootWin -> drawable.height,
                                                       0, InputOutput, 0, NULL,
                                                           pRootWin -> drawable.depth,
                                                               serverClient, visual,
                                                                   &error);

  AddResource(xid, RT_WINDOW, (void *) nxagentViewportFrameRight);

  if (error != Success)
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentInitViewportFrame: Error creating nxagentViewportFrameRight.\n");
    #endif

    error = Success;
  }

  xid = FakeClientID(serverClient -> index);

  #ifdef TEST
  fprintf(stderr, "nxagentInitViewportFrame: XID = [%lx]\n", xid);
  #endif

  nxagentViewportFrameAbove = CreateWindow(xid, pRootWin, 0, -NXAGENT_FRAME_WIDTH,
                                               pRootWin -> drawable.width,
                                                   NXAGENT_FRAME_WIDTH, 0,
                                                       InputOutput, 0, NULL,
                                                           pRootWin -> drawable.depth,
                                                               serverClient, visual,
                                                                   &error);

  AddResource(xid, RT_WINDOW, (void *) nxagentViewportFrameAbove);

  if (error != Success)
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentInitViewportFrame: Error creating nxagentViewportFrameAbove.\n");
    #endif

    error = Success;
  }

  xid = FakeClientID(serverClient -> index);

  #ifdef TEST
  fprintf(stderr, "nxagentInitViewportFrame: XID = [%lx]\n", xid);
  #endif

  nxagentViewportFrameBelow = CreateWindow(xid, pRootWin, 0,
                                               pRootWin -> drawable.height,
                                                   pRootWin -> drawable.width,
                                                       NXAGENT_FRAME_WIDTH, 0,
                                                           InputOutput, 0, NULL,
                                                               pRootWin -> drawable.depth,
                                                                   serverClient, visual, &error);

  AddResource(xid, RT_WINDOW, (void *) nxagentViewportFrameBelow);

  if (error != Success)
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentInitViewportFrame: Error creating nxagentViewportFrameBelow.\n");
    #endif
  }

  nxagentViewportFrameLeft -> overrideRedirect = 1;
  nxagentViewportFrameRight -> overrideRedirect = 1;
  nxagentViewportFrameAbove -> overrideRedirect = 1;
  nxagentViewportFrameBelow -> overrideRedirect = 1;

  MapWindow(nxagentViewportFrameLeft, serverClient);
  MapWindow(nxagentViewportFrameRight, serverClient);
  MapWindow(nxagentViewportFrameAbove, serverClient);
  MapWindow(nxagentViewportFrameBelow, serverClient);

  /*
   * nxagentScreenTrap = False;
   */
}

#endif /* #ifdef VIEWPORT_FRAME */

void nxagentPrintAgentGeometry(char *hdrMessage, char *prefix)
{
  #ifdef WARNING

  if (prefix == NULL)
  {
    prefix = "";
  }

  if (hdrMessage)
  {
    fprintf(stderr, "--------------- %s -----------------.\n", hdrMessage);
  }

  fprintf(stderr, "%s Root window at offset (%d,%d) size (%d,%d).\n", prefix,
              nxagentOption(RootX), nxagentOption(RootY),
                  nxagentOption(RootWidth), nxagentOption(RootHeight));

  fprintf(stderr, "%s Default window at offset (%d,%d) size (%d,%d) border size %d.\n", prefix,
              nxagentOption(X), nxagentOption(Y), nxagentOption(Width), nxagentOption(Height),
                  nxagentOption(BorderWidth));

  fprintf(stderr, "%s Span between root window and default window is (%d,%d).\n", prefix,
              nxagentOption(ViewportXSpan), nxagentOption(ViewportYSpan));

  fprintf(stderr, "%s Default window in window mode has offset (%d,%d) and size (%d,%d).\n", prefix,
              nxagentOption(SavedX), nxagentOption(SavedY), nxagentOption(SavedWidth), nxagentOption(SavedHeight));

  fprintf(stderr, "%s Fullscreen is %s.\n", prefix,
              nxagentOption(Fullscreen) ? "ON" : "OFF");

  fprintf(stderr, "%s Desktop resize mode is %s.\n", prefix,
              nxagentOption(DesktopResize) ? "ON" : "OFF");

  fprintf(stderr, "%s Resize desktop at startup is %s.\n", prefix,
              nxagentResizeDesktopAtStartup ? "ON" : "OFF");

  #endif
}

static int nxagentColorOffset(unsigned long mask)
{
  int count;

  for (count = 0; !(mask & 1) && count < 32; count++)
  {
    mask >>= 1;
  }

  return count;
}

Bool nxagentOpenScreen(ScreenPtr pScreen,
                           int argc, char *argv[])
{
  VisualPtr visuals;
  DepthPtr depths;
  int numVisuals, numDepths;
  int i, j, depthIndex;
  unsigned long valuemask;
  XSetWindowAttributes attributes;
  XWindowAttributes gattributes;
  XSizeHints* sizeHints;
  XWMHints* wmHints;
  Mask mask;
  Bool resetAgentPosition = False;

  VisualID defaultVisual;
  int rootDepth;

  void * pFrameBufferBits;
  int bitsPerPixel;
  int sizeInBytes;

  int defaultVisualIndex = 0;

  #ifdef TEST
  fprintf(stderr, "nxagentOpenScreen: Called for screen index [%d].\n",
              pScreen->myNum);
  #endif

  if (nxagentRenderEnable && nxagentReconnectTrap == False)
  {
    PictureScreenPrivateIndex = -1;
  }

  nxagentDefaultScreen = pScreen;

  nxagentQueryAtoms(pScreen);

  #ifdef NXAGENT_ONSTART
  nxagentWMStart = nxagentAtoms[3];  /* WM_NX_READY */
  #endif

  /*
   * Forced geometry parameter
   * to user geometry.
   */

  if (nxagentResizeDesktopAtStartup)
  {
    if (nxagentUserGeometry.flag & XValue)
    {
      nxagentChangeOption(X, nxagentUserGeometry.X);
    }

    if (nxagentUserGeometry.flag & YValue)
    {
      nxagentChangeOption(Y, nxagentUserGeometry.Y);
    }

    if (nxagentUserGeometry.flag & WidthValue)
    {
      nxagentChangeOption(Width, nxagentUserGeometry.Width);
      nxagentChangeOption(RootWidth, nxagentUserGeometry.Width);

      if (nxagentOption(SavedWidth) > nxagentUserGeometry.Width)
      {
        nxagentChangeOption(SavedWidth, nxagentUserGeometry.Width);
      }
    }

    if (nxagentUserGeometry.flag & HeightValue)
    {
      nxagentChangeOption(Height, nxagentUserGeometry.Height);
      nxagentChangeOption(RootHeight, nxagentUserGeometry.Height);

      if (nxagentOption(SavedHeight) > nxagentUserGeometry.Height)
      {
        nxagentChangeOption(SavedHeight, nxagentUserGeometry.Height);
      }
    }
  }

  /*
   * This is first time the
   * screen is initialized.
   * Filling the geometry parameter
   * from user geometry.
   */

  if (nxagentReconnectTrap == False)
  {
    if (nxagentUserGeometry.flag & XValue)
    {
      nxagentChangeOption(X, nxagentUserGeometry.X);
    }

    if (nxagentUserGeometry.flag & YValue)
    {
      nxagentChangeOption(Y, nxagentUserGeometry.Y);
    }

    if (nxagentUserGeometry.flag & WidthValue)
    {
      nxagentChangeOption(RootWidth, nxagentUserGeometry.Width);
    }

    if (nxagentUserGeometry.flag & HeightValue)
    {
      nxagentChangeOption(RootHeight, nxagentUserGeometry.Height);
    }
  }
  else if (nxagentWMIsRunning && !nxagentOption(Rootless) && !nxagentOption(Fullscreen))
  {
    /*
     * At reconnection, try to estimate the shift due to WM reparenting.
     */

    if (nxagentOption(X) >= 6)
    {
      nxagentChangeOption(X, nxagentOption(X) - 6);
    }

    if (nxagentOption(Y) >= 25)
    {
      nxagentChangeOption(Y, nxagentOption(Y) - 25);
    }
  }

  /*
   * Determine the size of the root window.
   * It is the maximum size of the screen
   * if we are either in rootless or in
   * fullscreen mode.
   */

  if (nxagentOption(Rootless) == False && nxagentWMIsRunning == False)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentOpenScreen: Forcing fullscreen mode with no window manager running.\n");
    #endif

    nxagentChangeOption(Fullscreen, True);

    if (nxagentOption(ClientOs) == ClientOsWinnt &&
            (nxagentReconnectTrap == False || nxagentResizeDesktopAtStartup))
    {
      NXSetExposeParameters(nxagentDisplay, 0, 0, 0);
    }
  }

  if (nxagentOption(Fullscreen) &&
          nxagentWMIsRunning &&
              nxagentReconnectTrap &&
                  nxagentResizeDesktopAtStartup == False &&
                      nxagentXServerGeometryChanged())
  {
    #ifdef TEST
    fprintf(stderr, "nxagentOpenScreen: Forcing window mode with server geometry changed.\n");
    #endif

    nxagentChangeOption(Fullscreen, False);

    nxagentChangeOption(AllScreens, False);

    nxagentFullscreenWindow = 0;

    resetAgentPosition = True;
  }

  if (nxagentOption(Fullscreen))
  {
    nxagentChangeOption(X, 0);
    nxagentChangeOption(Y, 0);

    nxagentChangeOption(Width, WidthOfScreen(DefaultScreenOfDisplay(nxagentDisplay)));
    nxagentChangeOption(Height, HeightOfScreen(DefaultScreenOfDisplay(nxagentDisplay)));

    nxagentChangeOption(BorderWidth, 0);

    if (nxagentReconnectTrap == False || nxagentResizeDesktopAtStartup)
    {
      nxagentChangeOption(RootWidth, WidthOfScreen(DefaultScreenOfDisplay(nxagentDisplay)));
      nxagentChangeOption(RootHeight, HeightOfScreen(DefaultScreenOfDisplay(nxagentDisplay)));

      if (nxagentOption(RootWidth) > WidthOfScreen(DefaultScreenOfDisplay(nxagentDisplay)))
      {
        nxagentChangeOption(SavedWidth, WidthOfScreen(DefaultScreenOfDisplay(nxagentDisplay)) * 3 / 4);
      }
      else
      {
        nxagentChangeOption(SavedWidth, nxagentOption(RootWidth));
      }

      if (nxagentOption(RootHeight) > HeightOfScreen(DefaultScreenOfDisplay(nxagentDisplay)))
      {
        nxagentChangeOption(SavedHeight, HeightOfScreen(DefaultScreenOfDisplay(nxagentDisplay)) * 3 / 4);
      }
      else
      {
        nxagentChangeOption(SavedHeight, nxagentOption(RootHeight));
      }
    }

    nxagentChangeOption(RootX, ((WidthOfScreen(DefaultScreenOfDisplay(nxagentDisplay)) -
                            nxagentOption(RootWidth)) / 2));
    nxagentChangeOption(RootY, ((HeightOfScreen(DefaultScreenOfDisplay(nxagentDisplay)) -
                            nxagentOption(RootHeight)) / 2));
  }
  else
  {
    nxagentChangeOption(BorderWidth, 0);

    if (nxagentReconnectTrap == False)
    {
      nxagentChangeOption(RootX, 0);
      nxagentChangeOption(RootY, 0);

      nxagentChangeOption(Width, nxagentOption(RootWidth));
      nxagentChangeOption(Height, nxagentOption(RootHeight));
    }

    /*
     * Be sure that the agent window won't be bigger
     * than the root window.
     */

    if (nxagentOption(Width) > nxagentOption(RootWidth))
    {
      nxagentChangeOption(Width, nxagentOption(RootWidth));
    }

    if (nxagentOption(Height) > nxagentOption(RootHeight))
    {
      nxagentChangeOption(Height, nxagentOption(RootHeight));
    }

    /*
     * Be sure that the agent window won't be bigger
     * than the X server root window.
     */

    if (nxagentOption(Width) > WidthOfScreen(DefaultScreenOfDisplay(nxagentDisplay)))
    {
      nxagentChangeOption(Width, WidthOfScreen(DefaultScreenOfDisplay(nxagentDisplay)) * 3 / 4);
    }

    if (nxagentOption(Height) > HeightOfScreen(DefaultScreenOfDisplay(nxagentDisplay)))
    {
      nxagentChangeOption(Height, HeightOfScreen(DefaultScreenOfDisplay(nxagentDisplay)) * 3 / 4);
    }

    /*
     * Forcing the agent window geometry to be equal to
     * the root window geometry the first time the
     * screen is initialized if the geometry hasn't been
     * esplicitly set in the option file and if
     * the root window isn't bigger than the X server
     * root window..
     */

    if (nxagentReconnectTrap == False)
    {
      if ((nxagentOption(RootWidth) < WidthOfScreen(DefaultScreenOfDisplay(nxagentDisplay))) &&
              !(nxagentUserGeometry.flag & WidthValue))
      {
        nxagentChangeOption(Width, nxagentOption(RootWidth));
      }

      if ((nxagentOption(RootHeight) < HeightOfScreen(DefaultScreenOfDisplay(nxagentDisplay))) &&
              !(nxagentUserGeometry.flag & HeightValue))
      {
        nxagentChangeOption(Height, nxagentOption(RootHeight));
      }
    }

    if (resetAgentPosition)
    {
      nxagentChangeOption(X, ((WidthOfScreen(DefaultScreenOfDisplay(nxagentDisplay)) -
                              nxagentOption(Width)) / 2));

      nxagentChangeOption(Y, ((HeightOfScreen(DefaultScreenOfDisplay(nxagentDisplay)) -
                              nxagentOption(Height)) / 2));
    }

    nxagentChangeOption(SavedWidth, nxagentOption(RootWidth));
    nxagentChangeOption(SavedHeight, nxagentOption(RootHeight));
  }

  if (nxagentOption(Rootless))
  {
    nxagentChangeOption(RootWidth, WidthOfScreen(DefaultScreenOfDisplay(nxagentDisplay)));
    nxagentChangeOption(RootHeight, HeightOfScreen(DefaultScreenOfDisplay(nxagentDisplay)));
  }

  nxagentChangeOption(SavedRootWidth, nxagentOption(RootWidth));
  nxagentChangeOption(SavedRootHeight, nxagentOption(RootHeight));

  nxagentChangeOption(ViewportXSpan, nxagentOption(Width) - nxagentOption(RootWidth));
  nxagentChangeOption(ViewportYSpan, nxagentOption(Height) - nxagentOption(RootHeight));

  if (nxagentReconnectTrap == 0)
  {
    if (nxagentOption(Persistent))
    {
      nxagentArgc = argc;
      nxagentArgv = argv;
    }

    #ifdef NXAGENT_TIMESTAMP

    {
      extern unsigned long startTime;

      fprintf(stderr, "Screen: going to open screen, time is [%d] milliseconds.\n",
                  GetTimeInMillis() - startTime);
    }

    #endif

    /*
     * Initialize all our privates.
     */

    if (AllocateWindowPrivate(pScreen, nxagentWindowPrivateIndex, sizeof(nxagentPrivWindowRec)) == 0 ||
            AllocateGCPrivate(pScreen, nxagentGCPrivateIndex, sizeof(nxagentPrivGC)) == 0 ||
                AllocateClientPrivate(nxagentClientPrivateIndex, sizeof(PrivClientRec)) == 0 ||
                    AllocatePixmapPrivate(pScreen, nxagentPixmapPrivateIndex, sizeof(nxagentPrivPixmapRec)) == 0)
    {
      return False;
    }

    /*
     * Initialize the depths.
     */

    depths = (DepthPtr) malloc(nxagentNumDepths * sizeof(DepthRec));

    for (i = 0; i < nxagentNumDepths; i++)
    {
      depths[i].depth = nxagentDepths[i];
      depths[i].numVids = 0;
      depths[i].vids = (VisualID *) malloc(MAXVISUALSPERDEPTH * sizeof(VisualID));
    }

    /*
     * Initialize the visuals.
     */

    #if defined(DEBUG) || defined(DEBUG_COLORMAP)
    fprintf(stderr, "Debug: Setting up visuals. Original array has size "
                "[%d].\n", nxagentNumVisuals); 
    #endif

    numVisuals = 0;
    numDepths = nxagentNumDepths;

    visuals = (VisualPtr) malloc(nxagentNumVisuals * sizeof(VisualRec));

    for (i = 0; i < nxagentNumVisuals; i++)
    {
      visuals[numVisuals].vid = FakeClientID(0);
      visuals[numVisuals].class = nxagentVisuals[i].class;
      visuals[numVisuals].bitsPerRGBValue = nxagentVisuals[i].bits_per_rgb;
      visuals[numVisuals].ColormapEntries = nxagentVisuals[i].colormap_size;
      visuals[numVisuals].nplanes = nxagentVisuals[i].depth;
      visuals[numVisuals].redMask = nxagentVisuals[i].red_mask;
      visuals[numVisuals].greenMask = nxagentVisuals[i].green_mask;
      visuals[numVisuals].blueMask = nxagentVisuals[i].blue_mask;
      visuals[numVisuals].offsetRed = nxagentColorOffset(nxagentVisuals[i].red_mask);
      visuals[numVisuals].offsetGreen = nxagentColorOffset(nxagentVisuals[i].green_mask);
      visuals[numVisuals].offsetBlue = nxagentColorOffset(nxagentVisuals[i].blue_mask);

      /*
       * Check for and remove the duplicates.
       */

      if (i == nxagentDefaultVisualIndex)
      {
        defaultVisualIndex = numVisuals;

        #if defined(DEBUG) || defined(DEBUG_COLORMAP)
        fprintf(stderr, "Debug: Set default visual index [%d].\n" ,
                    defaultVisualIndex); 
        #endif
      }
      else
      {
        for (j = 0; j < numVisuals; j++)
        {
          if (visuals[numVisuals].class == visuals[j].class &&
              visuals[numVisuals].bitsPerRGBValue ==
                  visuals[j].bitsPerRGBValue &&
              visuals[numVisuals].ColormapEntries ==
                  visuals[j].ColormapEntries &&
              visuals[numVisuals].nplanes == visuals[j].nplanes &&
              visuals[numVisuals].redMask == visuals[j].redMask &&
              visuals[numVisuals].greenMask == visuals[j].greenMask &&
              visuals[numVisuals].blueMask == visuals[j].blueMask &&
              visuals[numVisuals].offsetRed == visuals[j].offsetRed &&
              visuals[numVisuals].offsetGreen == visuals[j].offsetGreen &&
              visuals[numVisuals].offsetBlue == visuals[j].offsetBlue)
            break;
        }
 
        if (j < numVisuals)
            continue;

      }

      depthIndex = UNDEFINED;

      #if defined(DEBUG) || defined(DEBUG_COLORMAP)
      fprintf(stderr, "Debug: Added visual [%lu].\n" ,
                  (long unsigned int)visuals[numVisuals].vid); 
      #endif

      for (j = 0; j < numDepths; j++)
      {
        if (depths[j].depth == nxagentVisuals[i].depth)
        {
          depthIndex = j;
          break;
        }
      }

      if (depthIndex == UNDEFINED)
      {
        #ifdef WARNING
        fprintf(stderr, "nxagentOpenScreen: WARNING! Can't find a matching depth for visual depth [%d].\n",
                nxagentVisuals[i].depth);
        #endif

        depthIndex = numDepths;

        depths[depthIndex].depth = nxagentVisuals[i].depth;
        depths[depthIndex].numVids = 0;
        depths[depthIndex].vids = (VisualID *) malloc(MAXVISUALSPERDEPTH * sizeof(VisualID));

        numDepths++;
      }

      if (depths[depthIndex].numVids >= MAXVISUALSPERDEPTH)
      {
        FatalError("Visual table overflow");
      }

      depths[depthIndex].vids[depths[depthIndex].numVids] = visuals[numVisuals].vid;

      depths[depthIndex].numVids++;

      #if defined(DEBUG) || defined(DEBUG_COLORMAP)
      fprintf(stderr, "Debug: Registered visual [%lu] for depth [%d (%d)].\n" ,
                  (long unsigned int)visuals[numVisuals].vid, depthIndex,
                      depths[depthIndex].depth); 
      #endif

      numVisuals++;
    }

    #if defined(DEBUG) || defined(DEBUG_COLORMAP)
    fprintf(stderr, "Debug: Setting default visual [%d (%lu)].\n",
                defaultVisualIndex, (long unsigned int)visuals[defaultVisualIndex].vid);

    fprintf(stderr, "Debug: Setting root depth [%d].\n",
                visuals[defaultVisualIndex].nplanes); 
    #endif

    defaultVisual = visuals[defaultVisualIndex].vid;
    rootDepth = visuals[defaultVisualIndex].nplanes;

    nxagentInitAlphaVisual();

    bitsPerPixel = nxagentBitsPerPixel(rootDepth);

    if (bitsPerPixel == 1)
    {
      sizeInBytes = PixmapBytePad(nxagentOption(RootWidth), rootDepth) * nxagentOption(RootHeight);
    }
    else
    {
      sizeInBytes = PixmapBytePad(nxagentOption(RootWidth), rootDepth) * nxagentOption(RootHeight) * bitsPerPixel/8;
    }

    #ifdef TEST
    fprintf(stderr, "nxagentOpenScreen: Frame buffer allocated. rootDepth "
                "[%d] bitsPerPixel [%d] sizeInBytes [%d]\n", rootDepth, bitsPerPixel, sizeInBytes);
    #endif

    pFrameBufferBits = (char *) malloc(sizeInBytes);

    if (!pFrameBufferBits)
    {
      return FALSE;
    }

    #if defined(DEBUG) || defined(DEBUG_COLORMAP)
    fprintf(stderr, "nxagentOpenScreen: Before fbScreenInit numVisuals [%d] numDepths [%d] "
              "rootDepth [%d] defaultVisual [%lu].\n", numVisuals, numDepths,
                  rootDepth, (long unsigned int)defaultVisual);
    #endif

    if ((monitorResolution < 1) && (nxagentAutoDPI == False))
    {
      monitorResolution = NXAGENT_DEFAULT_DPI;
    }
    else if ((monitorResolution < 1) && (nxagentAutoDPI == True))
    {
      monitorResolution = NXAGENT_AUTO_DPI;
    }

    if (!fbScreenInit(pScreen, pFrameBufferBits, nxagentOption(RootWidth), nxagentOption(RootHeight),
          monitorResolution, monitorResolution, PixmapBytePad(nxagentOption(RootWidth), rootDepth), bitsPerPixel))
    {
      return FALSE;
    }

    #if defined(DEBUG) || defined(DEBUG_COLORMAP)
    fprintf(stderr, "nxagentOpenScreen: After fbScreenInit numVisuals [%d] numDepths [%d] "
              "rootDepth [%d] defaultVisual [%lu].\n", numVisuals, numDepths,
                  rootDepth, (long unsigned int)defaultVisual);
    #endif

    /*
     * Complete the initialization of the GLX
     * extension. This will add the GLX visuals
     * and will modify numVisuals and numDepths.
     */

    #if defined(DEBUG) || defined(DEBUG_COLORMAP)
    fprintf(stderr, "nxagentOpenScreen: Before GLX numVisuals [%d] numDepths [%d] "
              "rootDepth [%d] defaultVisual [%lu].\n", numVisuals, numDepths,
                  rootDepth, (long unsigned int)defaultVisual);
    #endif

    nxagentInitGlxExtension(&visuals, &depths, &numVisuals, &numDepths,
                                &rootDepth, &defaultVisual);

    #if defined(DEBUG) || defined(DEBUG_COLORMAP)
    fprintf(stderr, "nxagentOpenScreen: After GLX numVisuals [%d] numDepths [%d] "
              "rootDepth [%d] defaultVisual [%lu].\n", numVisuals, numDepths,
                  rootDepth, (long unsigned int)defaultVisual);
    #endif

    /*
     * Replace the visuals and depths initialized
     * by fbScreenInit with our own.
     */

    free(pScreen -> visuals);
    free(pScreen -> allowedDepths);

    pScreen -> visuals = visuals;
    pScreen -> allowedDepths = depths;
    pScreen -> numVisuals = numVisuals;
    pScreen -> numDepths = numDepths;
    pScreen -> rootVisual = defaultVisual;
    pScreen -> rootDepth = rootDepth;

    /*
     * Complete the initialization of the RANDR
     * extension.
     */

    nxagentInitRandRExtension(pScreen);

    /*
     * Set up the internal structures used for
     * tracking the proxy resources associated
     * to the unpack and split operations.
     */

    nxagentInitSplitResources();
    nxagentInitUnpackResources();

    /*
     * Initializing the pixmaps that will serve as
     * "placeholders" in lazy encoding. We need one 
     * pixmap for each depth.
     */

    for (i = 0; i < numDepths; i++)
    {
      nxagentMarkPlaceholderNotLoaded(i);
    }

    #ifdef WATCH

    fprintf(stderr, "nxagentOpenScreen: Watchpoint 7.\n");

/*
Reply   Total	Cached	Bits In			Bits Out		Bits/Reply	  Ratio
------- -----	------	-------			--------		----------	  -----
N/A
*/

    sleep(30);

    #endif

    if (nxagentParentWindow != 0)
    {
      /*
       * This would cause a GetWindowAttributes
       * and a GetGeometry (asynchronous) reply.
       */

      XGetWindowAttributes(nxagentDisplay, nxagentParentWindow, &gattributes);

      nxagentChangeOption(Width, gattributes.width);
      nxagentChangeOption(Height, gattributes.height);
    }

    if (nxagentOption(AllScreens))
    {
      attributes.override_redirect = True; 
    }

    if (nxagentOption(Fullscreen))
    {
      /*
       * We need to disable the host's screensaver or
       * it will otherwise grab the screen even if it
       *  is under agent's control.
       */

      XSetScreenSaver(nxagentDisplay, 0, 0, DefaultExposures, DefaultBlanking);
    }

    if (nxagentTrue24)
    {
      fbGetScreenPrivate(pScreen) -> win32bpp = visuals[nxagentDefaultVisualIndex].nplanes;
      fbGetScreenPrivate(pScreen) -> pix32bpp = visuals[nxagentDefaultVisualIndex].nplanes;
    }
    else
    {
      fbGetScreenPrivate(pScreen) -> win32bpp = 32;
      fbGetScreenPrivate(pScreen) -> pix32bpp = 32;
    }

    /*
     * We call miScreenInit with NULL in place of the screen area if we
     * don't want to initialize the frame buffer.
     *
     * if (!miScreenInit(pScreen, NULL, nxagentOption(RootWidth),
     *              nxagentOption(RootHeight), 1, 1, nxagentOption(RootWidth),
     *              visuals[nxagentDefaultVisualIndex].nplanes, / * Root depth. * / 
     *              numDepths, depths,
     *              visuals[nxagentDefaultVisualIndex].vid,* Root visual. * /
     *              numVisuals, visuals))
     *   return FALSE;
     */

    if (monitorResolution < 0)
    {
      pScreen->mmWidth  = nxagentOption(RootWidth) * DisplayWidthMM(nxagentDisplay,
                              DefaultScreen(nxagentDisplay)) / DisplayWidth(nxagentDisplay,
                                  DefaultScreen(nxagentDisplay));

      pScreen->mmHeight = nxagentOption(RootHeight) * DisplayHeightMM(nxagentDisplay,
                              DefaultScreen(nxagentDisplay)) / DisplayHeight(nxagentDisplay,
                                  DefaultScreen(nxagentDisplay));
    }

    pScreen->defColormap = (Colormap) FakeClientID(0);
    pScreen->minInstalledCmaps = MINCMAPS;
    pScreen->maxInstalledCmaps = MAXCMAPS;

    pScreen->whitePixel = nxagentWhitePixel;
    pScreen->blackPixel = nxagentBlackPixel;
    /* rgf */
    /* GCperDepth */
    /* PixmapPerDepth */
    /* WindowPrivateLen */
    /* WindowPrivateSizes */
    /* totalWindowSize */
    /* GCPrivateLen */
    /* GCPrivateSizes */
    /* totalGCSize */

    /*
     * Random screen procedures.
     */

    pScreen->CloseScreen = nxagentCloseScreen;
    pScreen->QueryBestSize = nxagentQueryBestSize;
    pScreen->SaveScreen = nxagentSaveScreen;
    pScreen->GetImage = nxagentGetImage;
    pScreen->GetSpans = nxagentGetSpans;
    pScreen->PointerNonInterestBox = (void (*)()) 0;
    pScreen->SourceValidate = (void (*)()) 0;

    pScreen->CreateScreenResources = nxagentCreateScreenResources;

    /*
     * Window Procedures.
     * 
     * Note that the following functions are not
     * replaced with nxagent counterparts:
     *
     * ValidateTreeProcPtr       ValidateTree;
     * ClearToBackgroundProcPtr  ClearToBackground;
     *
     * Note also that the ConfigureWindow procedure
     * has not a pointer in the screen structure.
     */

    pScreen->CreateWindow = nxagentCreateWindow;
    pScreen->DestroyWindow = nxagentDestroyWindow;
    pScreen->PositionWindow = nxagentPositionWindow;
    pScreen->ChangeWindowAttributes = nxagentChangeWindowAttributes;
    pScreen->RealizeWindow = nxagentRealizeWindow;
    pScreen->UnrealizeWindow = nxagentUnrealizeWindow;
    pScreen->PostValidateTree = nxagentPostValidateTree;
    pScreen->WindowExposures = nxagentWindowExposures;
    pScreen->PaintWindowBackground = nxagentPaintWindowBackground;
    pScreen->PaintWindowBorder = nxagentPaintWindowBorder;
    pScreen->CopyWindow = nxagentCopyWindow;
    pScreen->ClipNotify = nxagentClipNotify;
    pScreen->RestackWindow = nxagentRestackWindow;
    pScreen->ReparentWindow = nxagentReparentWindow;

    /*
     * Pixmap procedures.
     */

    pScreen->CreatePixmap = nxagentCreatePixmap;
    pScreen->DestroyPixmap = nxagentDestroyPixmap;

    /*
     * This is originally miModifyPixmapHeader()
     * from miscrinit.c. It is used to recycle
     * the scratch pixmap for this screen.
     */

    pScreen->ModifyPixmapHeader = nxagentModifyPixmapHeader;

    /*
     * Font procedures.
     */

    pScreen->RealizeFont = nxagentRealizeFont;
    pScreen->UnrealizeFont = nxagentUnrealizeFont;

    /*
     * GC procedures.
     */

    pScreen->CreateGC = nxagentCreateGC;
    pScreen->BitmapToRegion = nxagentPixmapToRegion;

    /*
     * Colormap procedures.
     */

    pScreen->CreateColormap = nxagentCreateColormap;
    pScreen->DestroyColormap = nxagentDestroyColormap;
    pScreen->InstallColormap = nxagentInstallColormap;
    pScreen->UninstallColormap = nxagentUninstallColormap;
    pScreen->ListInstalledColormaps = nxagentListInstalledColormaps;
    pScreen->StoreColors = nxagentStoreColors;
    pScreen->ResolveColor = nxagentResolveColor;

    /*
     * Backing store procedures.
     */

    pScreen->SaveDoomedAreas = (void (*)()) 0;
    pScreen->RestoreAreas = (RegionPtr (*)()) 0;
    pScreen->ExposeCopy = (void (*)()) 0;
    pScreen->TranslateBackingStore = (RegionPtr (*)()) 0;
    pScreen->ClearBackingStore = (RegionPtr (*)()) 0;
    pScreen->DrawGuarantee = (void (*)()) 0;

    if (enableBackingStore == 1)
    {
      #ifdef TEST
      fprintf(stderr, "nxagentOpenScreen: Going to initialize backing store.\n");
      #endif

      pScreen -> BackingStoreFuncs.SaveAreas = nxagentSaveAreas;
      pScreen -> BackingStoreFuncs.RestoreAreas = nxagentRestoreAreas;
      pScreen -> BackingStoreFuncs.SetClipmaskRgn = 0;
      pScreen -> BackingStoreFuncs.GetImagePixmap = 0;
      pScreen -> BackingStoreFuncs.GetSpansPixmap = 0;

      miInitializeBackingStore(pScreen);
    }

    /*
     * OS layer procedures.
     */

    pScreen->BlockHandler = (ScreenBlockHandlerProcPtr) NoopDDA;
    pScreen->WakeupHandler = (ScreenWakeupHandlerProcPtr) NoopDDA;
    pScreen->blockData = NULL;
    pScreen->wakeupData = NULL;

    #ifdef RENDER

    /*
     * Initialize picture support. This have to be
     * placed here because miDCInitialize calls
     * DamageSetup, that should wrap the picture
     * screen functions. So PictureInit has to be
     * called before.
     */

    if (nxagentRenderEnable && !nxagentReconnectTrap)
    {
      if (!nxagentPictureInit(pScreen, 0, 0))
      {
        nxagentRenderEnable = False;              

        return FALSE;
      }

      if (nxagentAlphaEnabled)
      {
        fprintf(stderr, "Info: Using alpha channel in render extension.\n");
      }
    }

    #endif /* RENDER */

    /*
     * From misprite.c: called from device-dependent screen
     * initialization proc after all of the function pointers
     * have been stored in the screen structure.
     */

    miDCInitialize(pScreen, &nxagentPointerCursorFuncs);

    /*
     * Cursor Procedures.
     */

    pScreen->ConstrainCursor = nxagentConstrainCursor;
    pScreen->CursorLimits = nxagentCursorLimits;
    pScreen->DisplayCursor = nxagentDisplayCursor;
    pScreen->RealizeCursor = nxagentRealizeCursor;
    pScreen->UnrealizeCursor = nxagentUnrealizeCursor;
    pScreen->RecolorCursor = nxagentRecolorCursor;

    nxagentSetCursorPositionW = pScreen->SetCursorPosition;

    pScreen->SetCursorPosition = nxagentSetCursorPosition;

    #define POSITION_OFFSET (pScreen->myNum * (nxagentOption(Width) + \
                               nxagentOption(Height)) / 32)
  }

  #ifdef TEST
  nxagentPrintAgentGeometry(NULL, "nxagentOpenScreen:");
  #endif

  if (nxagentDoFullGeneration == 1 ||
          nxagentReconnectTrap == 1)
  {
    valuemask = CWBackPixel | CWEventMask | CWColormap |
                    (nxagentOption(AllScreens) == 1 ? CWOverrideRedirect : 0);

    attributes.background_pixel = nxagentBlackPixel;

    /* Assume that the mask fits in int... broken on Big Endian 64bit systems. */
    Mask tmp_mask = attributes.event_mask;
    nxagentGetDefaultEventMask(&tmp_mask);
    attributes.event_mask = (int)tmp_mask;

    attributes.colormap = nxagentDefaultVisualColormap(nxagentDefaultVisual(pScreen));

    if (nxagentOption(AllScreens) == 1)
    {
      attributes.override_redirect = True;
    }

    if (nxagentOption(Fullscreen) == 1)
    {
      if (nxagentReconnectTrap)
      {
        /*
         * We need to disable the host's screensaver or
         * it will otherwise grab the screen even if it
         * is under agent's control.
         */
                                                                                                   
        XSetScreenSaver(nxagentDisplay, 0, 0, DefaultExposures, DefaultBlanking);
      }
    }

    /*
     * This would be used when running agent
     * embedded into another X window.
     */

    if (nxagentParentWindow != 0)
    {
      nxagentDefaultWindows[pScreen->myNum] = nxagentParentWindow;

      nxagentGetDefaultEventMask(&mask);

      XSelectInput(nxagentDisplay, nxagentDefaultWindows[pScreen->myNum], mask);
    }
    else
    {
      /*
       * Create any top-level window as a child of the
       * real root of the remote display. See also the
       * InitRootWindow() procedure and the function
       * handling the splash screen.
       */

      if (nxagentOption(Rootless) == True)
      {
        nxagentDefaultWindows[pScreen->myNum] = DefaultRootWindow(nxagentDisplay);

        #ifdef TEST
        fprintf(stderr, "nxagentOpenScreen: Using root window id [%ld].\n",
                    (long int)nxagentDefaultWindows[pScreen->myNum]);
        #endif
      }

      #ifdef TEST
      fprintf(stderr, "nxagentOpenScreen: Going to create new default window.\n");
      #endif

      nxagentDefaultWindows[pScreen->myNum] =
          XCreateWindow(nxagentDisplay,
                        DefaultRootWindow(nxagentDisplay),
                        nxagentOption(X) + POSITION_OFFSET,
                        nxagentOption(Y) + POSITION_OFFSET,
                        nxagentOption(Width),
                        nxagentOption(Height),
                        nxagentOption(BorderWidth),
                        pScreen->rootDepth,
                        InputOutput,
                        nxagentDefaultVisual(pScreen),
                        valuemask, &attributes);

       if (nxagentOption(Rootless) == 0)
       {
         valuemask = CWEventMask;
         mask = PointerMotionMask;
         attributes.event_mask = mask;

         nxagentInputWindows[pScreen -> myNum] =
             XCreateWindow(nxagentDisplay,
                           nxagentDefaultWindows[pScreen -> myNum],
                           0, 0,
                           nxagentOption(Width),
                           nxagentOption(Height),
                           0, 0, InputOnly,
                           nxagentDefaultVisual(pScreen),
                           valuemask , &attributes);
      }

      if (nxagentReportWindowIds)
      {
        fprintf (stderr, "NXAGENT_WINDOW_ID: SCREEN_WINDOW:[%d],WID:[0x%x]\n", pScreen -> myNum, nxagentInputWindows[pScreen->myNum]);
      }
      #ifdef TEST
      fprintf(stderr, "nxagentOpenScreen: Created new default window with id [0x%x].\n",
              nxagentDefaultWindows[pScreen->myNum]);
      #endif

      /*
       * Setting WM_CLASS to "X2GoAgent" when running in X2Go Agent mode
       * we need it to properly display all window parameters by some WMs
       * (for example on Maemo)
       */
      {
        #ifdef TEST
        fprintf(stderr, "nxagentOpenScreen: Setting WM_CLASS and WM_NAME for window with id [%ld].\n",
                (long int)nxagentDefaultWindows[pScreen->myNum]);
        #endif

        XClassHint hint;

        if(nxagentX2go)
        {
          hint.res_name = strdup("X2GoAgent");
          hint.res_class = strdup("X2GoAgent");
        }
        else
        {
          hint.res_name = strdup("NXAgent");
          hint.res_class = strdup("NXAgent");
        }
        XSetClassHint(nxagentDisplay, nxagentDefaultWindows[pScreen->myNum], &hint);
        free(hint.res_name);
        free(hint.res_class);
      }

      if (nxagentOption(Fullscreen))
      {
        nxagentFullscreenWindow = nxagentDefaultWindows[pScreen->myNum];
      }

      if (nxagentIpaq)
      {
        XWindowChanges ch;
        unsigned int ch_mask;

        ch.stack_mode = Below;
        ch_mask = CWStackMode;

        XConfigureWindow(nxagentDisplay, nxagentFullscreenWindow, ch_mask, &ch);
      }
    }

    if (nxagentOption(Fullscreen))
    {
      /*
       * FIXME: Do we still need to set this property?
       */

      if (nxagentAtoms[8] != 0)
      {
        XChangeProperty(nxagentDisplay,
                        nxagentDefaultWindows[pScreen->myNum],
                        nxagentAtoms[8], /* NX_AGENT_SIGNATURE */
                        XA_STRING,
                        8,
                        PropModeReplace,
                        (unsigned char*) "X-AGENT",
                        strlen("X-AGENT"));
      }

      nxagentGetDefaultEventMask(&mask);

      XSelectInput(nxagentDisplay, nxagentFullscreenWindow, mask);
    }

    if ((sizeHints = XAllocSizeHints()))
    {
      sizeHints->flags = PPosition | PMinSize | PMaxSize;
      sizeHints->x = nxagentOption(X) + POSITION_OFFSET;
      sizeHints->y = nxagentOption(Y) + POSITION_OFFSET;
      sizeHints->min_width = MIN_NXAGENT_WIDTH;
      sizeHints->min_height = MIN_NXAGENT_HEIGHT;

      sizeHints->width = nxagentOption(RootWidth);
      sizeHints->height = nxagentOption(RootHeight);

      if (nxagentOption(DesktopResize) == 1 || nxagentOption(Fullscreen) == 1)
      {
        sizeHints->max_width = WidthOfScreen(DefaultScreenOfDisplay(nxagentDisplay));
        sizeHints->max_height = HeightOfScreen(DefaultScreenOfDisplay(nxagentDisplay));
      }
      else
      {
        sizeHints->max_width = nxagentOption(RootWidth);
        sizeHints->max_height = nxagentOption(RootHeight);
      }

      if (nxagentUserGeometry.flag & XValue || nxagentUserGeometry.flag & YValue)
        sizeHints->flags |= USPosition;
      if (nxagentUserGeometry.flag & WidthValue || nxagentUserGeometry.flag & HeightValue)
        sizeHints->flags |= USSize;
    }
    /* FIXME: deprecated, replaced by XSetWmProperties() */
    XSetStandardProperties(nxagentDisplay,
                           nxagentDefaultWindows[pScreen->myNum],
                           nxagentWindowName,
                           nxagentWindowName,
                           nxagentIconPixmap,
                           argv, argc, sizeHints);

    if (sizeHints)
      XFree(sizeHints);

    if ((wmHints = XAllocWMHints()))
    {
      wmHints->icon_pixmap = nxagentIconPixmap;

      if (useXpmIcon)
      {
        wmHints->icon_mask = nxagentIconShape;
        wmHints->flags = IconPixmapHint | IconMaskHint;
      }
      else
      {
        wmHints->flags = IconPixmapHint;
      }

      XSetWMHints(nxagentDisplay, nxagentDefaultWindows[pScreen->myNum], wmHints);
      XFree(wmHints);
    }

    /*
     * Clear the window but let it unmapped. We'll map it
     * at the time the we'll initialize our screen root
     * and only if we are not running in rootless mode.
     */

    XClearWindow(nxagentDisplay, nxagentDefaultWindows[pScreen->myNum]);

    if (nxagentOption(AllScreens))
    {
      if (nxagentReconnectTrap)
      {
        XGrabKeyboard(nxagentDisplay, nxagentFullscreenWindow, True, GrabModeAsync,
                      GrabModeAsync, CurrentTime);
      }

      nxagentIconWindow = nxagentCreateIconWindow();
    }
    else
    {
      nxagentIconWindow = 0;
    }

    /*
     * When we don't have window manager we grab keyboard
     * to let nxagent get keyboard events.
     */

    if (!nxagentWMIsRunning && !nxagentOption(Fullscreen))
    {
      #ifdef TEST
      fprintf(stderr, "nxagentOpenScreen: No window manager, we call XGrabKeyboard.\n");
      #endif

      XGrabKeyboard(nxagentDisplay, RootWindow (nxagentDisplay, 0), True, GrabModeAsync,
                    GrabModeAsync, CurrentTime);
    }
  }

  if (!nxagentCreateDefaultColormap(pScreen))
  {
    #ifdef PANIC
    fprintf(stderr, "nxagentOpenScreen: Failed to create default colormap for screen.\n");
    #endif

    return False;
  }

  /*
   * The purpose of this check is to verify if there
   * is a window manager running. Unfortunately due
   * to the way we manage the intern atoms call, the
   * atom will always exist.
   */

  if (nxagentWMIsRunning)
  {
    XlibAtom deleteWMatom = nxagentAtoms[2];  /* WM_DELETE_WINDOW */

    #ifdef TEST
    fprintf(stderr, "nxagentOpenScreen: Found WM, delete window atom [%ld].\n",
                deleteWMatom);
    #endif

    /* FIXME: This doing the same thing in both cases. The
       comments do not seem accurate (anymore?) */
    if (nxagentOption(Rootless) == False)
    {
      /*
       * Set the WM_DELETE_WINDOW protocol for the main agent
       * window and, if we are in fullscreen mode, include the
       * icon window.
       */

      XSetWMProtocols(nxagentDisplay, nxagentDefaultWindows[pScreen->myNum], &deleteWMatom, 1);
    }
    else
    {
      /*
       * We need to register the ICCCM WM_DELETE_WINDOW
       * protocol for any top-level window or the agent
       * will be killed if any window is closed.
       */

      XSetWMProtocols(nxagentDisplay, nxagentDefaultWindows[pScreen->myNum], &deleteWMatom, 1);

      #ifdef TEST
      fprintf(stderr, "Warning: Not setting the WM_DELETE_WINDOW protocol.\n");
      #endif
    }
  }
  else
  {
    /*
     * We should always enable the configuration of the
     * remote X server's devices if we are running full-
     * screen and there is no WM running.
     */

    if (nxagentOption(Fullscreen))
    {
      #ifdef TEST
      fprintf(stderr, "nxagentOpenScreen: WARNING! Forcing propagation of device control changes.\n");
      #endif

      nxagentChangeOption(DeviceControl, True);
    }
  }

  /*
   * Inform the user whether the agent's clients will
   * be able to change the real X server's keyboard
   * and pointer settings.
   */

  if (nxagentOption(DeviceControl) == False)
  {
    fprintf(stderr, "Info: Not using local device configuration changes.\n");
  }
  else
  {
    fprintf(stderr, "Info: Using local device configuration changes.\n");
  }

  #ifdef RENDER

  /*
   * if (nxagentRenderEnable && !nxagentReconnectTrap)
   * {
   *   if (!nxagentPictureInit(pScreen, 0, 0))
   *   {
   *     nxagentRenderEnable = False;
   *
   *     return FALSE;
   *   }
   *
   *   if (nxagentAlphaEnabled)
   *   {
   *     fprintf(stderr, "Info: Using alpha channel in render extension.\n");
   *   }
   * }
   */

  #endif /* RENDER */

  /*
   * Check if the composite extension is
   * supported on the remote display and
   * prepare the agent for its use.
   */

  nxagentCompositeExtensionInit();

  /* We use this to get informed about RandR changes on the real display.
     FIXME: It would probably be better to use an RRScreenChangeNotifyEvent here. */
  XSelectInput(nxagentDisplay, DefaultRootWindow(nxagentDisplay), StructureNotifyMask);

  #ifdef NXAGENT_TIMESTAMP

  {
    extern unsigned long startTime;

    fprintf(stderr, "Screen: open screen finished, time is [%d] milliseconds.\n",
                GetTimeInMillis() - startTime);
  }

  #endif

  #ifdef WATCH

  fprintf(stderr, "nxagentOpenScreen: Watchpoint 8.\n");

/*
Reply   Total	Cached	Bits In			Bits Out		Bits/Reply	  Ratio
------- -----	------	-------			--------		----------	  -----
#1   U  2	1	5344 bits (1 KB) ->	2344 bits (0 KB) ->	2672/1 -> 1172/1	= 2.280:1
#16     11		2816 bits (0 KB) ->	197 bits (0 KB) ->	256/1 -> 18/1	= 14.294:1
#91     1		16640 bits (2 KB) ->	12314 bits (2 KB) ->	16640/1 -> 12314/1	= 1.351:1
#98     2		512 bits (0 KB) ->	57 bits (0 KB) ->	256/1 -> 28/1	= 8.982:1
*/

  sleep(30);

  #endif

  return True;
}

Bool nxagentCloseScreen(ScreenPtr pScreen)
{
  int i;

  #ifdef DEBUG
  fprintf(stderr, "running nxagentCloseScreen()\n");
  #endif

  for (i = 0; i < pScreen->numDepths; i++)
  {
    free(pScreen->allowedDepths[i].vids);
  }

  /*
   * Free the frame buffer.
   */

  free(((PixmapPtr)pScreen -> devPrivate) -> devPrivate.ptr);

  free(pScreen->allowedDepths);
  free(pScreen->visuals);
  free(pScreen->devPrivate);

  /*
   * Reset the geometry and alpha information
   * used by proxy to unpack the packed images.
   */

  nxagentResetVisualCache();
  nxagentResetAlphaCache();
  nxagentReleaseAllSplits();

  /*
   * The assumption is that all X resources will be
   * destroyed upon closing the display connection.
   * There is no need to generate extra protocol.
   */

  return True;
}

/*
 * This function comes from the xfree86 Xserver.
 */

static void nxagentSetRootClip (ScreenPtr pScreen, Bool enable)
{
    WindowPtr   pWin = pScreen->root;
    WindowPtr   pChild;
    Bool        WasViewable = (Bool)(pWin->viewable);
    Bool        anyMarked = FALSE;
    RegionPtr   pOldClip = NULL, bsExposed;
#ifdef DO_SAVE_UNDERS
    Bool        dosave = FALSE;
#endif
    WindowPtr   pLayerWin;
    BoxRec      box;

    if (WasViewable)
    {
        for (pChild = pWin->firstChild; pChild; pChild = pChild->nextSib)
        {
            (void) (*pScreen->MarkOverlappedWindows)(pChild,
                                                     pChild,
                                                     &pLayerWin);
        }
        (*pScreen->MarkWindow) (pWin);
        anyMarked = TRUE;
        if (pWin->valdata)
        {
            if (HasBorder (pWin))
            {
                RegionPtr       borderVisible;

                borderVisible = RegionCreate(NullBox, 1);
                RegionSubtract(borderVisible,
                                &pWin->borderClip, &pWin->winSize);
                pWin->valdata->before.borderVisible = borderVisible;
            }
            pWin->valdata->before.resized = TRUE;
        }
    }

    /*
     * Use REGION_BREAK to avoid optimizations in ValidateTree
     * that assume the root borderClip can't change well, normally
     * it doesn't...)
     */
    if (enable)
    {
        box.x1 = 0;
        box.y1 = 0;
        box.x2 = pScreen->width;
        box.y2 = pScreen->height;
        RegionInit(&pWin->winSize, &box, 1);
        RegionInit(&pWin->borderSize, &box, 1);
        if (WasViewable)
            RegionReset(&pWin->borderClip, &box);
        pWin->drawable.width = pScreen->width;
        pWin->drawable.height = pScreen->height;
        RegionBreak(&pWin->clipList);
    }
    else
    {
        RegionEmpty(&pWin->borderClip);
        RegionBreak(&pWin->clipList);
    }

    ResizeChildrenWinSize (pWin, 0, 0, 0, 0);

    if (WasViewable)
    {
        if (pWin->backStorage)
        {
            pOldClip = RegionCreate(NullBox, 1);
            RegionCopy(pOldClip, &pWin->clipList);
        }

        if (pWin->firstChild)
        {
            anyMarked |= (*pScreen->MarkOverlappedWindows)(pWin->firstChild,
                                                           pWin->firstChild,
                                                           (WindowPtr *)NULL);
        }
        else
        {
            (*pScreen->MarkWindow) (pWin);
            anyMarked = TRUE;
        }

#ifdef DO_SAVE_UNDERS
        if (DO_SAVE_UNDERS(pWin))
        {
            dosave = (*pScreen->ChangeSaveUnder)(pLayerWin, pLayerWin);
        }
#endif /* DO_SAVE_UNDERS */

        if (anyMarked)
            (*pScreen->ValidateTree)(pWin, NullWindow, VTOther);
    }

    if (pWin->backStorage && pOldClip &&
        ((pWin->backingStore == Always) || WasViewable))
    {
        if (!WasViewable)
            pOldClip = &pWin->clipList; /* a convenient empty region */
        bsExposed = (*pScreen->TranslateBackingStore)
                             (pWin, 0, 0, pOldClip,
                              pWin->drawable.x, pWin->drawable.y);
        if (WasViewable)
            RegionDestroy(pOldClip);
        if (bsExposed)
        {
            RegionPtr   valExposed = NullRegion;

            if (pWin->valdata)
                valExposed = &pWin->valdata->after.exposed;
            (*pScreen->WindowExposures) (pWin, valExposed, bsExposed);
            if (valExposed)
                RegionEmpty(valExposed);
            RegionDestroy(bsExposed);
        }
    }
    if (WasViewable)
    {
        if (anyMarked)
            (*pScreen->HandleExposures)(pWin);
#ifdef DO_SAVE_UNDERS
        if (dosave)
            (*pScreen->PostChangeSaveUnder)(pLayerWin, pLayerWin);
#endif /* DO_SAVE_UNDERS */
        if (anyMarked && pScreen->PostValidateTree)
            (*pScreen->PostValidateTree)(pWin, NullWindow, VTOther);
    }
    if (pWin->realized)
        WindowsRestructured ();
    FlushAllOutput ();
}

Bool nxagentResizeScreen(ScreenPtr pScreen, int width, int height,
                             int mmWidth, int mmHeight)
{
  BoxRec box;
  PixmapPtr pPixmap;
  char *fbBits;
  
  int oldWidth;
  int oldHeight;
  int oldMmWidth;
  int oldMmHeight;

  #ifdef TEST
  nxagentPrintAgentGeometry("Before Resize Screen", "nxagentResizeScreen:");
  #endif
  
  /*
   * Change screen properties.
   */

  oldWidth = pScreen -> width;
  oldHeight = pScreen -> height;
  oldMmWidth = pScreen -> mmWidth;
  oldMmHeight = pScreen -> mmHeight;

  pScreen -> width = width;
  pScreen -> height = height;

  /*
   * Compute screen dimensions if they aren't given.
   */

  if (mmWidth == 0)
  {
    if (monitorResolution < 0)
    {
      mmWidth  = width * DisplayWidthMM(nxagentDisplay, DefaultScreen(nxagentDisplay)) /
                     DisplayWidth(nxagentDisplay, DefaultScreen(nxagentDisplay));
    }
    else
    {
      mmWidth  = (width * 254 + monitorResolution * 5) / (monitorResolution * 10);
    }

    if (mmWidth < 1)
    {
      mmWidth = 1;
    }

  }

  if (mmHeight == 0)
  {
    if (monitorResolution < 0)
    {
      mmHeight = height * DisplayHeightMM(nxagentDisplay, DefaultScreen(nxagentDisplay)) /
                     DisplayHeight(nxagentDisplay, DefaultScreen(nxagentDisplay));
    }
    else
    {
      mmHeight = (height * 254 + monitorResolution * 5) / (monitorResolution * 10);
    }

    if (mmHeight < 1)
    {
      mmHeight = 1;
    }

  }

  pScreen -> mmWidth = mmWidth;
  pScreen -> mmHeight = mmHeight;

  pPixmap = fbGetScreenPixmap(pScreen);

  if ((fbBits = realloc(pPixmap -> devPrivate.ptr, PixmapBytePad(width, pScreen->rootDepth) *
                            height * BitsPerPixel(pScreen->rootDepth) / 8)) == NULL)
  {
    pScreen -> width = oldWidth;
    pScreen -> height = oldHeight;
    pScreen -> mmWidth = oldMmWidth;
    pScreen -> mmHeight = oldMmHeight;

    goto nxagentResizeScreenError;
  }

  if (!miModifyPixmapHeader(pPixmap, width, height,
                                pScreen->rootDepth, BitsPerPixel(pScreen->rootDepth),
                                    PixmapBytePad(width,
                                        pScreen->rootDepth), fbBits))
  {
/*
FIXME: We should try to restore the previously
       reallocated frame buffer pixmap.
*/

    pScreen -> width = oldWidth;
    pScreen -> height = oldHeight;
    pScreen -> mmWidth = oldMmWidth;
    pScreen -> mmHeight = oldMmHeight;

    goto nxagentResizeScreenError;
  }

  nxagentChangeOption(RootWidth, width);
  nxagentChangeOption(RootHeight, height);

  if (nxagentOption(Fullscreen))
  {
    nxagentChangeOption(RootX, (nxagentOption(Width) -
                            nxagentOption(RootWidth)) / 2);
    nxagentChangeOption(RootY, (nxagentOption(Height) -
                            nxagentOption(RootHeight)) / 2);
  }
  else
  {
    nxagentChangeOption(RootX, 0);
    nxagentChangeOption(RootY, 0);
  }

  nxagentChangeOption(ViewportXSpan, nxagentOption(Width) - nxagentOption(RootWidth));
  nxagentChangeOption(ViewportYSpan, nxagentOption(Height) - nxagentOption(RootHeight));

  /*
   * Change agent window size and size hints.
   */

  if ((nxagentOption(Fullscreen) == 0 && nxagentOption(AllScreens) == 0))
  {
    nxagentSetWMNormalHints(pScreen->myNum, width, height);

    XResizeWindow(nxagentDisplay, nxagentDefaultWindows[pScreen->myNum], width, height);

    if (nxagentOption(Rootless) == 0)
    {
      XResizeWindow(nxagentDisplay, nxagentInputWindows[pScreen -> myNum], width, height);
    }
  }

  /*
   * Set properties for the agent root window.
   */

  box.x1 = 0;
  box.y1 = 0;
  box.x2 = width;
  box.y2 = height;

  pScreen->root -> drawable.width = width;
  pScreen->root -> drawable.height = height;
  pScreen->root -> drawable.x = 0;
  pScreen->root -> drawable.y = 0;

  RegionInit(&pScreen->root -> borderSize, &box, 1);
  RegionInit(&pScreen->root -> winSize, &box, 1);
  RegionInit(&pScreen->root -> clipList, &box, 1);
  RegionInit(&pScreen->root -> borderClip, &box, 1);

  (*pScreen -> PositionWindow)(pScreen->root, 0, 0);

  nxagentSetRootClip(pScreen, 1);

  XMoveWindow(nxagentDisplay, nxagentWindow(screenInfo.screens[0]->root),
                  nxagentOption(RootX), nxagentOption(RootY));

  nxagentMoveViewport(pScreen, 0, 0);

  /*
   * Update pointer bounds.
   */

  ScreenRestructured(pScreen);

  #ifdef TEST
  nxagentPrintAgentGeometry("After Resize Screen", "nxagentResizeScreen:");
  #endif

  nxagentSetPrintGeometry(pScreen -> myNum);

  return 1;

nxagentResizeScreenError:

  return 0;
}

void nxagentShadowSetRatio(float floatXRatio, float floatYRatio)
{
  int intXRatio;
  int intYRatio;

  if (floatXRatio == 0)
  {
    floatXRatio = 1.0;
  }

  if (floatYRatio == 0)
  {
    floatYRatio = 1.0;
  }

  intXRatio = floatXRatio * (1 << PRECISION);
  intYRatio = floatYRatio * (1 << PRECISION);

  nxagentChangeOption(FloatXRatio, floatXRatio);
  nxagentChangeOption(FloatYRatio, floatYRatio);

  nxagentChangeOption(XRatio, intXRatio);
  nxagentChangeOption(YRatio, intYRatio);

  #ifdef TEST
  fprintf(stderr, "Info: Using X ratio [%f] Y ratio [%f].\n",
              nxagentOption(FloatXRatio), nxagentOption(FloatYRatio));
  #endif
}

void nxagentShadowSetWindowsSize(void)
{
  XResizeWindow(nxagentDisplay, nxagentDefaultWindows[0],
                    nxagentOption(Width), nxagentOption(Height));

  XMoveResizeWindow(nxagentDisplay, nxagentInputWindows[0], 0, 0, 
                        nxagentOption(Width), nxagentOption(Height));
}

void nxagentShadowSetWindowOptions(void)
{
  nxagentChangeOption(RootWidth,  nxagentScale(nxagentShadowWidth,  nxagentOption(XRatio)));
  nxagentChangeOption(RootHeight, nxagentScale(nxagentShadowHeight, nxagentOption(YRatio)));

  nxagentChangeOption(SavedRootWidth,  nxagentOption(RootWidth));
  nxagentChangeOption(SavedRootHeight, nxagentOption(RootHeight));

  nxagentChangeOption(RootX, (nxagentOption(Width)  - nxagentOption(RootWidth))  >> 1);
  nxagentChangeOption(RootY, (nxagentOption(Height) - nxagentOption(RootHeight)) >> 1);
}

int nxagentShadowInit(ScreenPtr pScreen, WindowPtr pWin)
{
  int i;
  char *layout = NULL;
  extern char *nxagentKeyboard;
  XlibGC gc;
  XGCValues value;

  #ifndef __CYGWIN32__

  Atom nxagentShadowAtom;

  int fd;

  #endif

  #ifdef TEST
  fprintf(stderr, "Info: Init shadow session. nxagentDisplayName [%s] "
              "nxagentDisplay [%p] nxagentShadowDisplayName [%s].\n",
                  nxagentDisplayName, (void *) nxagentDisplay,
                      nxagentShadowDisplayName);
  #endif

  if (nxagentKeyboard != NULL)
  {
    for (i = 0; nxagentKeyboard[i] != '/' && nxagentKeyboard[i] != 0; i++);

    if(nxagentKeyboard[i] == 0 || nxagentKeyboard[i + 1] == 0 || i == 0)
    {
      #ifdef WARNING
      fprintf(stderr,"WARNING! Wrong keyboard type: %s.\n", nxagentKeyboard);
      #endif
    }
    else
    {
      layout = strdup(&nxagentKeyboard[i + 1]);
    }
  }

  #ifdef DEBUG
  fprintf(stderr, "nxagentShadowInit: Setting the master uid [%d].\n",
              nxagentShadowUid);
  #endif

#if !defined (__CYGWIN32__) && !defined (WIN32)

  if (nxagentShadowUid != -1)
  {
    NXShadowSetDisplayUid(nxagentShadowUid);
  }

  if (nxagentOption(UseDamage) == 0)
  {
    NXShadowDisableDamage();
  }

#endif

  if (NXShadowCreate(nxagentDisplay, layout, nxagentShadowDisplayName,
                         (void *) &nxagentShadowDisplay) != 1)
  {
    #ifdef PANIIC
    fprintf(stderr, "nxagentShadowInit: PANIC! Failed to initialize shadow "
                "display [%s].\n", nxagentShadowDisplayName);
    #endif

    return -1;
  }

  /*
   * The shadow nxagent sets the _NX_SHADOW
   * property on the master X server root
   * window in order to notify its presence.
   */

  #ifndef __CYGWIN__

  nxagentShadowAtom = XInternAtom(nxagentShadowDisplay, "_NX_SHADOW", False);

  XChangeProperty(nxagentShadowDisplay, DefaultRootWindow(nxagentShadowDisplay),
                      nxagentShadowAtom, XA_STRING, 8, PropModeReplace, NULL, 0);
  #endif

  if (NXShadowAddUpdaterDisplay(nxagentDisplay, &nxagentShadowWidth,
                                    &nxagentShadowHeight, &nxagentMasterDepth) == 0)
  {
    #ifdef PANIC
    fprintf(stderr, "nxagentShadowInit: PANIC! Failed to add display [%s].\n",
                nxagentDisplayName);
    #endif

    return -1;
  }

  #ifndef __CYGWIN32__

  if (nxagentOption(Fullscreen) == 1)
  {
    nxagentShadowSetRatio(WidthOfScreen(DefaultScreenOfDisplay(nxagentDisplay)) * 1.0 / nxagentShadowWidth,
                              HeightOfScreen(DefaultScreenOfDisplay(nxagentDisplay)) * 1.0 / nxagentShadowHeight);
  }
  else if (nxagentUserGeometry.flag != 0)
  {
    nxagentShadowSetRatio(nxagentOption(RootWidth) * 1.0 / nxagentShadowWidth,
                              nxagentOption(RootHeight) * 1.0 / nxagentShadowHeight);
  }

  if (DefaultVisualOfScreen(DefaultScreenOfDisplay(nxagentDisplay)) ->
          class != TrueColor)
  {
    #ifdef PANIC
    fprintf(stderr, "nxagentShadowInit: PANIC! The visual class of the remote "
                "X server is not TrueColor.\n");
    #endif

    return -1;
  }

  if (DefaultVisualOfScreen(DefaultScreenOfDisplay(nxagentShadowDisplay)) ->
          class != TrueColor)
  {
    #ifdef PANIC

    const char *className;

    switch (DefaultVisualOfScreen(DefaultScreenOfDisplay(nxagentShadowDisplay)) -> class)
    {
      case StaticGray:
      {
        className = "StaticGray";

        break;
      }
      case StaticColor:
      {
        className = "StaticColor";

        break;
      }
      case PseudoColor:
      {
        className = "PseudoColor";

        break;
      }
      case DirectColor:
      {
        className = "DirectColor";

        break;
      }
      case GrayScale:
      {
        className = "GrayScale";

        break;
      }
      default:
      {
        className = "";

        break;
      }
    }

    fprintf(stderr, "nxagentShadowInit: PANIC! Cannot shadow the display. "
                "%s visual class is not supported. Only TrueColor visuals "
                    "are supported.\n", className);

    #endif /* #endif PANIC */

    return -1;
  }

  #endif

  nxagentShadowDepth = pScreen -> rootDepth;

  switch (nxagentMasterDepth)
  {
    case 32:
    case 24:
    {
      if (nxagentShadowDepth == 16)
      {
        nxagentCheckDepth = 1;
      }
      else if (nxagentShadowDepth == 8)
      {
        #ifdef PANIC
        fprintf(stderr, "nxagentShadowInit: PANIC! Unable to shadow a %d bit "
                    "display with a 8 bit screen depth.\n", nxagentMasterDepth);
        #endif

        return -1;
      }

      nxagentBppMaster = 4;

      break;
    }
    case 16:
    {
      if (nxagentShadowDepth > 16)
      {
        nxagentCheckDepth = 1;
      }
      else if (nxagentShadowDepth == 8)
      {
        #ifdef PANIC
        fprintf(stderr, "nxagentShadowInit: PANIC! Unable to shadow a 16 bit "
                    "display with a 8 bit screen depth.\n");
        #endif

        return -1;
      }

      nxagentBppMaster = 2;

      break;
    }
    case 8:
    {
      if (nxagentShadowDepth != 8)
      {
        #ifdef PANIC
        fprintf(stderr, "nxagentShadowInit: PANIC! Unable to shadow a 8 bit "
                    "display with a %d bit screen depth.\n", nxagentShadowDepth);
        #endif

        return -1;
      }

      nxagentBppMaster = 1;

      break;
    }
    default:
    {
      #ifdef PANIC
      fprintf(stderr, "nxagentShadowInit: PANIC! The depth is not 32, 24, 16 or 8 bit.\n");
      #endif

      return -1;
    }
  }

  if (nxagentShadowDepth >= 24)
  {
    nxagentBppShadow = 4;
  }
  else if (nxagentShadowDepth == 16)
  {
    nxagentBppShadow = 2;
  }
  else if (nxagentShadowDepth == 8)
  {
    nxagentBppShadow = 1;
  }

#if !defined(__CYGWIN__)

  imageByteOrder = nxagentShadowDisplay -> byte_order;

  fd = XConnectionNumber(nxagentShadowDisplay);

  nxagentShadowXConnectionNumber = fd;

#endif

  #ifdef TEST
  fprintf(stderr, "nxagentShadowInit: Adding the X connection [%d] "
              "to the device set.\n", fd);
  #endif

  SetNotifyFd(nxagentShadowXConnectionNumber, nxagentNotifyConnection, X_NOTIFY_READ, NULL);

  accessPixmapID = FakeClientID(serverClient -> index);

  AddResource(accessPixmapID, RT_PIXMAP, (void *)nxagentShadowPixmapPtr);

  accessWindowID = FakeClientID(serverClient -> index);

  AddResource(accessWindowID, RT_WINDOW, (void *)nxagentShadowWindowPtr);

  nxagentResizeScreen(pScreen, nxagentShadowWidth, nxagentShadowHeight, pScreen -> mmWidth, pScreen -> mmHeight);

  nxagentShadowCreateMainWindow(pScreen, pWin, nxagentShadowWidth, nxagentShadowHeight);

  if (nxagentRemoteMajor <= 3)
  {
    nxagentShadowSetWindowsSize();

    nxagentSetWMNormalHints(0, nxagentOption(Width), nxagentOption(Height));
  }

  XMapWindow(nxagentDisplay, nxagentDefaultWindows[0]);

  /*
   * Clean up the main window.
   */

  value.foreground = 0x00000000;
  value.background = 0x00000000;
  value.plane_mask = 0xffffffff;
  value.fill_style = FillSolid;

  gc = XCreateGC(nxagentDisplay, nxagentPixmap(nxagentShadowPixmapPtr), GCBackground |
           GCForeground | GCFillStyle | GCPlaneMask, &value);

  XFillRectangle(nxagentDisplay, nxagentPixmap(nxagentShadowPixmapPtr), gc, 0, 0,
                     nxagentShadowWidth, nxagentShadowHeight);

  XFreeGC(nxagentDisplay, gc);

  RegionInit(&nxagentShadowUpdateRegion, (BoxRec*)NULL, 1);

  return 0;
}

int nxagentShadowCreateMainWindow(ScreenPtr pScreen, WindowPtr pWin, int width, int height)
{
  XWindowChanges changes;
  Mask mask,maskb;
  XID values[4], *vlist;
  int error;
  XID xid;

  nxagentShadowWidth = width;
  nxagentShadowHeight = height;

  NXShadowUpdateBuffer((void *)&nxagentShadowBuffer);

  #ifdef TEST
  fprintf(stderr, "nxagentShadowCreateMainWindow: Update frame buffer [%p].\n", nxagentShadowBuffer);
  #endif

  nxagentShadowSetWindowOptions();

  if (nxagentShadowPixmapPtr != NULL)
  {
    nxagentDestroyPixmap(nxagentShadowPixmapPtr);
  }

  if (nxagentShadowWindowPtr != NULL)
  {
    DeleteWindow(nxagentShadowWindowPtr, accessWindowID);
  }

  nxagentShadowPixmapPtr = nxagentCreatePixmap(pScreen, nxagentShadowWidth, nxagentShadowHeight, nxagentShadowDepth, 0);

  if (nxagentShadowPixmapPtr)
  {
    ChangeResourceValue(accessPixmapID, RT_PIXMAP, (void *) nxagentShadowPixmapPtr);
    nxagentShadowPixmapPtr -> drawable.id = accessPixmapID;

    #ifdef TEST
    fprintf(stderr, "nxagentShadowCreateMainWindow: nxagentShadowPixmapPtr [%p] PixmapM -> drawable.id [%lu].\n",
                (void *)nxagentShadowPixmapPtr, nxagentShadowPixmapPtr -> drawable.id);
    fprintf(stderr, "nxagentShadowCreateMainWindow: Create pixmap with width [%d] height [%d] depth [%d].\n",
                nxagentShadowWidth, nxagentShadowHeight, (int)nxagentShadowDepth);
    #endif
  }
  else
  {
    #ifdef PANIC
    fprintf(stderr, "nxagentShadowCreateMainWindow: PANIC! Failed to create pixmap with width [%d] height [%d] depth [%d].\n",
                nxagentShadowWidth, nxagentShadowHeight, (int)nxagentShadowDepth);
    #endif
  }

  XFreePixmap(nxagentDisplay, nxagentPixmap(nxagentVirtualPixmap(nxagentShadowPixmapPtr)));

  xid = XCreatePixmap(nxagentDisplay, nxagentDefaultWindows[0],
            nxagentScale(nxagentShadowWidth, nxagentOption(XRatio)),
                nxagentScale(nxagentShadowHeight, nxagentOption(YRatio)), nxagentShadowDepth);

  nxagentPixmap(nxagentVirtualPixmap(nxagentShadowPixmapPtr)) = xid;

  nxagentPixmap(nxagentRealPixmap(nxagentShadowPixmapPtr)) = xid;

  if (nxagentShadowGCPtr != NULL)
  {
    FreeScratchGC(nxagentShadowGCPtr);
  }
/*
 * FIXME: Should use CreateGC.
 */
  nxagentShadowGCPtr = GetScratchGC(nxagentShadowPixmapPtr -> drawable.depth, nxagentShadowPixmapPtr -> drawable.pScreen);

  if (nxagentShadowGCPtr)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentShadowCreateMainWindow: Created GC with pGC[%p]\n", (void *) nxagentShadowGCPtr);
    #endif
    ValidateGC((DrawablePtr)nxagentShadowPixmapPtr, nxagentShadowGCPtr);
  }
  else
  {
    #ifdef PANIC
    fprintf(stderr, "nxagentShadowCreateMainWindow: PANIC! Failed to create GC.");
    #endif
  }

  mask = CWBackPixmap | CWEventMask | CWCursor;

  nxagentGetDefaultEventMask(&maskb);
  maskb |= ResizeRedirectMask | ExposureMask;

  vlist = values;
  *vlist++ = (XID)nxagentShadowPixmapPtr -> drawable.id;
  *vlist++ = (XID)maskb;

  *vlist = (XID)None;

  nxagentShadowWindowPtr = CreateWindow(accessWindowID, pWin, 0, 0, nxagentShadowWidth,
                                            nxagentShadowHeight, 0, InputOutput, mask, (XID *)values,
                                                nxagentShadowDepth, serverClient, CopyFromParent, &error);

  mask = CWWidth | CWHeight;
  changes.width = nxagentScale(nxagentShadowWidth, nxagentOption(XRatio));
  changes.height = nxagentScale(nxagentShadowHeight, nxagentOption(YRatio));

  XConfigureWindow(nxagentDisplay, nxagentWindow(nxagentShadowWindowPtr), mask, &changes);

  nxagentMoveViewport(pScreen, 0, 0);

  if (nxagentShadowWindowPtr && !error)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentShadowCreateMainWindow: Create window with nxagentShadowWindowPtr [%p]"
                "nxagentShadowWindowPtr -> drawable.id [%lu].\n", (void *) nxagentShadowWindowPtr,
                     nxagentShadowWindowPtr -> drawable.id);

    fprintf(stderr, "nxagentShadowCreateMainWindow: parent nxagentShadowWindowPtr [%p] parent -> drawable.id [%lu].\n",
                (void *)nxagentShadowWindowPtr->parent, nxagentShadowWindowPtr -> parent -> drawable.id);

    #endif

    ChangeResourceValue(accessWindowID, RT_WINDOW, (void *) nxagentShadowWindowPtr);
  }
  else
  {
    #ifdef PANIC
    fprintf(stderr, "nxagentShadowCreateMainWindow: PANIC! Failed to create window.\n");
    #endif
  }

  XMapWindow(nxagentDisplay, nxagentWindow(nxagentShadowWindowPtr));
  MapWindow(nxagentShadowWindowPtr, serverClient);

  mask = CWX | CWY | CWWidth | CWHeight;
  changes.x = nxagentOption(RootX);
  changes.y = nxagentOption(RootY);
  changes.width = nxagentScale(nxagentShadowWidth, nxagentOption(XRatio));
  changes.height = nxagentScale(nxagentShadowHeight, nxagentOption(YRatio));
  XConfigureWindow(nxagentDisplay, nxagentWindow(pWin), mask, &changes);

  #ifdef TEST
  fprintf(stderr, "nxagentShadowCreateMainWindow: Completed mapping of Access window.\n");
  #endif

  return 0;
}

int nxagentShadowSendUpdates(int *suspended)
{
  *suspended = 0;

  if (RegionNil(&nxagentShadowUpdateRegion) == 1)
  {
    return 0;
  }

  nxagentMarkCorruptedRegion((DrawablePtr)nxagentShadowPixmapPtr, &nxagentShadowUpdateRegion);

  RegionEmpty(&nxagentShadowUpdateRegion);

  return 1;
}

int nxagentShadowPoll(PixmapPtr nxagentShadowPixmapPtr, GCPtr nxagentShadowGCPtr,
                            unsigned char nxagentShadowDepth, int nxagentShadowWidth,
                                 int nxagentShadowHeight, char *nxagentShadowBuffer, int *changed, int *suspended)
{
  int x, y, y2, n, c, line;
  int result;
  long numRects;
  unsigned int width, height, length;
  char *tBuffer = NULL;
  char *iBuffer, *ptBox;
  BoxRec *pBox;
  RegionRec updateRegion;
  RegionRec tempRegion;
  BoxRec box;
  int overlap;


  RegionNull(&updateRegion);

  RegionNull(&tempRegion);

#ifdef __CYGWIN32__

  if (NXShadowCaptureCursor(nxagentWindow(nxagentShadowWindowPtr),
          nxagentShadowWindowPtr -> drawable.pScreen -> visuals) == -1)
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentShadowPoll: Failed to capture cursor.\n");
    #endif
  }

#endif

  result = NXShadowHasChanged(nxagentUserInput, NULL, suspended);

  *changed = result;

  if (result == 1)
  {
    if (nxagentWMPassed == 0)
    {
      nxagentRemoveSplashWindow(NULL);
    }

    NXShadowExportChanges(&numRects, &ptBox);
    pBox = (BoxRec *)ptBox;

    #ifdef TEST
    fprintf(stderr, "nxagentShadowPoll: nRects[%ld], pBox[%p] depth[%d].\n", numRects, (void *) pBox, nxagentShadowDepth);
    #endif

    for (n = 0; n < numRects; n++)
    {
      /*
       * The BoxRec struct defined in the Xserver has a different
       * variable order in comparison with the BoxRec struct in the Xlib.
       * the second and third field are inverted.
       */

      x = pBox[n].x1;
      y = pBox[n].x2;
      y2 = pBox[n].y2;
      width = pBox[n].y1 - pBox[n].x1;/* y1 = x2 */
      height = y2 - pBox[n].x2;   /* x2 = y1 */

      if((x + width) > nxagentShadowWidth || (y + height) > nxagentShadowHeight)
      {
        /*
         * Out of bounds. Maybe a resize of the master session is going on.
         */

        continue;
      }

      line = PixmapBytePad(width, nxagentMasterDepth);

      #ifdef DEBUG
      fprintf(stderr, "nxagentShadowPoll: Rectangle Number[%d] - x[%d]y[%d]W[%u]H[%u].\n", n+1, x, y, width, height);
      #endif

      length = nxagentImageLength(width, height, ZPixmap, 0, nxagentMasterDepth);

      free(tBuffer);

      tBuffer = malloc(length);

      if (tBuffer == NULL)
      {
        #ifdef PANIC
        fprintf(stderr, "nxagentShadowPoll: malloc failed.\n");
        #endif

        return -1;
      }

      iBuffer = tBuffer;

      for (c = 0; c + y < y2; c++)
      {
        memcpy(tBuffer, nxagentShadowBuffer + x * nxagentBppMaster + 
                   (y + c) * nxagentShadowWidth * nxagentBppMaster, line);

        tBuffer += line;

      }

      tBuffer = iBuffer;

#ifdef __CYGWIN32__
      if (nxagentBppMaster == 2)
      {
        NXShadowCorrectColor(length, tBuffer);
      }
#else
      if (nxagentCheckDepth == 1)
      {
        nxagentShadowAdaptDepth(width, height, line, &tBuffer);
      }
#endif

      fbPutImage(nxagentVirtualDrawable((DrawablePtr)nxagentShadowPixmapPtr), nxagentShadowGCPtr,
                          nxagentShadowDepth, x, y, width, height, 0, ZPixmap, tBuffer);

      box.x1 = x;
      box.x2 = x + width;
      box.y1 = y;
      box.y2 = y + height;

      RegionInit(&tempRegion, &box, 1);

      RegionAppend(&updateRegion, &tempRegion);

      RegionUninit(&tempRegion);

      RegionValidate(&updateRegion, &overlap);

      RegionUnion(&nxagentShadowUpdateRegion, &nxagentShadowUpdateRegion, &updateRegion);
    }

    free(tBuffer);

    RegionUninit(&updateRegion);
  }
  else if (result == -1)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentShadowPoll: polling failed!\n");
    #endif

    usleep(50 * 1000);

    return -1;
  }

  return 0;
}

void nxagentShadowAdaptDepth(unsigned int width, unsigned int height,
                                   unsigned int lineMaster, char **buffer)
{
  unsigned char red;
  unsigned char green;
  unsigned char blue;
  unsigned short color16 = 0;
  unsigned char * icBuffer;
  unsigned char * cBuffer = NULL;
  unsigned char * tBuffer = (unsigned char *) *buffer;
  unsigned int lineShadow;
  unsigned int length;
  unsigned int c;
  unsigned int pad;
  unsigned int color32 = 0;
  unsigned long redMask;
  unsigned long greenMask;
  unsigned long blueMask;
  Visual *pVisual;

  length = nxagentImageLength(width, height, ZPixmap, 0, nxagentShadowDepth);

  cBuffer = malloc(length);
  icBuffer = cBuffer;

  pVisual = nxagentImageVisual((DrawablePtr) nxagentShadowPixmapPtr, nxagentShadowDepth);

  if (pVisual == NULL)
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentCorrectDepthShadow: WARNING! Visual not found. Using default visual.\n");
    #endif

    pVisual = nxagentVisuals[nxagentDefaultVisualIndex].visual;
  }

  #ifdef TEST
  fprintf(stderr, "nxagentCorrectDepthShadow: Shadow redMask [%lu] greenMask[%lu] blueMask[%lu].\n",
             pVisual -> red_mask, pVisual -> green_mask, pVisual -> blue_mask);
  #endif

  redMask = nxagentShadowDisplay -> screens[0].root_visual[0].red_mask;
  greenMask = nxagentShadowDisplay -> screens[0].root_visual[0].green_mask;
  blueMask = nxagentShadowDisplay -> screens[0].root_visual[0].blue_mask;

  #ifdef TEST
  fprintf(stderr, "nxagentCorrectDepthShadow: Master redMask [%lu] greenMask[%lu] blueMask[%lu].\n",
              redMask, greenMask, blueMask);
  #endif

  switch(nxagentMasterDepth)
  {
   /*
    * The Shadow agent has 24 bit depth.
    */
    case 16:
    {
      pad = lineMaster - nxagentBppMaster * width;

      #ifdef TEST
      fprintf(stderr, "nxagentCorrectDepthShadow: line [%d] width[%d] pad[%d].\n", lineMaster, width, pad);
      #endif

      while (height > 0)
      {
        for (c = 0; c < width ; c++)
        {
          if (imageByteOrder == LSBFirst)
          {
            color16 = *tBuffer++;
            color16 |= (*tBuffer << 8);
          }
          else
          {
            color16 = (*tBuffer++) << 8;
            color16 |= *tBuffer;
          }

          blue = ((short) blueMask & color16) << 3;
          blue |= 0x3;

          if (greenMask == 0x7e0)
          {
            /*
             * bit mask 5-6-5
             */

            green = ((short) greenMask & color16) >> 3;
            green |= 0x2;

            red = ((short) redMask & color16) >> 8;
            red |= 0x3;
          }
          else
          {
            /*
             * bit mask 5-5-5
             */

            green = ((short) greenMask & color16) >> 2;
            green |= 0x3;

            red = ((short) redMask & color16) >> 7;
            red |= 0x3;
          }

          tBuffer++;

          if (nxagentDisplay -> byte_order == LSBFirst)
          {
            *cBuffer++ = blue;
            *cBuffer++ = green;
            *cBuffer++ = red;
            cBuffer++;
          }
          else
          {
            cBuffer++;
            *cBuffer++ = red;
            *cBuffer++ = green;
            *cBuffer++ = blue;
          }
        }
        tBuffer += pad;
        height--;
      }
      break;
    }

    /*
     * The Shadow agent has 16 bit depth.
     */
    case 24:
    {
      lineShadow = PixmapBytePad(width, nxagentShadowDepth);

      pad = lineShadow - nxagentBppShadow * width;

      #ifdef TEST
      fprintf(stderr, "nxagentCorrectDepthShadow: line [%d] width[%d] pad[%d].\n", lineShadow, width, pad);
      #endif

      while (height > 0)
      {
        for (c = 0; c < width; c++)
        {
          if (imageByteOrder == LSBFirst)
          {
            color32 = *tBuffer++;
            color32 |= (*tBuffer++ << 8);
            color32 |= (*tBuffer++ << 16);
            tBuffer++;
          }
          else
          {
            tBuffer++;
            color32 = (*tBuffer++ << 16);
            color32 |= (*tBuffer++ << 8);
            color32 |= *tBuffer++;
          }

          color16 = (color32 & (pVisual -> blue_mask << 3)) >> 3;

          if (pVisual -> green_mask == 0x7e0)
          {
            /*
             * bit mask 5-6-5
             */
             color16 |= (color32 & (pVisual -> green_mask << 5)) >> 5;
             color16 |= (color32 & (pVisual -> red_mask << 8)) >> 8;
          }
          else
          {
            /*
             * bit mask 5-5-5
             */
            color16 |= (color32 & (pVisual -> green_mask << 6)) >> 6;
            color16 |= (color32 & (pVisual -> red_mask << 9)) >> 9;
          }

          if (nxagentDisplay -> byte_order == LSBFirst)
          {
            *cBuffer++ = color16 & 0xff;
            *cBuffer++ = (color16 & 0xff00) >> 8;
          }
          else
          {
            *cBuffer++ = (color16 & 0xff00) >> 8;
            *cBuffer++ = color16 & 0xff;
          }
        }
        cBuffer += pad;
        height--;
      }
      break;
    }
  }
  cBuffer = (unsigned char *) *buffer;
  *buffer = (char *) icBuffer;

  free(cBuffer);
}

#ifdef NXAGENT_ARTSD

unsigned char fromHexNibble(char c)
{
  int uc = (unsigned char)c;

  if(uc >= '0' && uc <= '9') return uc - (unsigned char)'0';
  if(uc >= 'a' && uc <= 'f') return uc + 10 - (unsigned char)'a';
  if(uc >= 'A' && uc <= 'F') return uc + 10 - (unsigned char)'A';

  return 16; /*error*/
}

void nxagentPropagateArtsdProperties(ScreenPtr pScreen, char *port)
{
  Window                rootWin;
  XlibAtom              atomReturnType;
  XlibAtom              propAtom;
  int                   iReturnFormat;
  unsigned long         ulReturnItems;
  unsigned long         ulReturnBytesLeft;
  unsigned char         *pszReturnData = NULL;
  int                   iReturn;

  int i,in;
  char tchar[] = "    ";
/*
FIXME: The port information is not used at the moment and produces a
       warning on recent gcc versions. Do we need such information
       to run the audio forawrding?

  char *chport;
  char hex[] = "0123456789abcdef";
*/
  rootWin = DefaultRootWindow(nxagentDisplay);
  propAtom = nxagentAtoms[4];  /* MCOPGLOBALS */

  /*
   * Get at most 64KB of data.
   */

  iReturn = XGetWindowProperty(nxagentDisplay,
                               rootWin,
                               propAtom,
                               0,
                               65536 / 4,
                               False,
                               XA_STRING,
                               &atomReturnType,
                               &iReturnFormat,
                               &ulReturnItems,
                               &ulReturnBytesLeft,
                               &pszReturnData);

  if (iReturn == Success && atomReturnType != None &&
          ulReturnItems > 0 && pszReturnData != NULL)
  {
    char *local_buf;

    #ifdef TEST
    fprintf(stderr, "nxagentPropagateArtsdProperties: Got [%ld] elements of format [%d] with [%ld] bytes left.\n",
                ulReturnItems, iReturnFormat, ulReturnBytesLeft);
    #endif

    #ifdef WARNING

    if (ulReturnBytesLeft > 0)
    {
      fprintf(stderr, "nxagentPropagateArtsdProperties: WARNING! Could not get the whole ARTSD property data.\n");
    }

    #endif

    local_buf = (char *) malloc(strlen((char*)pszReturnData) + 100);

    if (local_buf)
    {
      memset(local_buf, 0, strlen((char *) pszReturnData));

      for (i = 0, in = 0; pszReturnData[i] != '\0'; i++)
      {
        local_buf[in]=pszReturnData[i];

        if(pszReturnData[i]==':')
        {
          i++;

          while(pszReturnData[i]!='\n')
          {
             unsigned char h;
             unsigned char l;

             h = fromHexNibble(pszReturnData[i]);
             i++;
             if(pszReturnData[i]=='\0') continue;
             l = fromHexNibble(pszReturnData[i]);
             i++;

             if(h >= 16 || l >= 16) continue;

             /*
              * FIXME: The array tchar[] was used uninitialized.
              * It's not clear to me the original purpose of the
              * piece of code using it. To be removed in future
              * versions.
              */

             tchar[0]=tchar[1];
             tchar[1]=tchar[2];
             tchar[2]=tchar[3];
             tchar[3] = (h << 4) + l;
             tchar[4]='\0';

             if (strncmp(tchar, "tcp:", 4) == 0)
             {
               local_buf[in-7]='1';
               local_buf[in-6]=strlen(port)+47;

               in++;
               local_buf[in]=pszReturnData[i-2];
               in++;
               local_buf[in]=pszReturnData[i-1];

               /* "localhost:" */
               strcat(local_buf,"6c6f63616c686f73743a");
               in+=20;

/*
FIXME: The port information is not used at the moment and produces a
       warning on recent gcc versions. Do we need such information
       to run the audio forawrding?

               chport=&port[0];

               while(*chport!='\0')
               {
                 in++;
                 local_buf[in]=hex[(*chport >> 4) & 0xf];
                 in++;
                 local_buf[in]=hex[*chport & 0xf];
                 *chport++;
               }
*/
               strcat(local_buf,"00");
               in+=2;

               while(pszReturnData[i]!='\n')
               {
                 i++;
               }
             }
             else
             {
               in++;
               local_buf[in]=pszReturnData[i-2];
               in++;
               local_buf[in]=pszReturnData[i-1];
             }
           }

           in++;
           local_buf[in]=pszReturnData[i];
        }

        in++;
      }

      local_buf[in]=0;

      if (strlen(local_buf))
      {
        mcop_local_atom = MakeAtom(mcop_atom, strlen(mcop_atom), 1);

        ChangeWindowProperty(pScreen->root,
                             mcop_local_atom,
                             XA_STRING,
                             iReturnFormat, PropModeReplace,
                             strlen(local_buf), local_buf, 1);
      }

      free(local_buf);
    }
  }
}

#endif

Bool nxagentReconnectScreen(void *p0)
{
  CARD16 w, h;
  PixmapPtr pPixmap = (PixmapPtr)nxagentDefaultScreen->devPrivate;
  Mask mask;

#if defined(NXAGENT_RECONNECT_DEBUG) || defined(NXAGENT_RECONNECT_SCREEN_DEBUG)
  fprintf(stderr, "nxagentReconnectScreen\n");
#endif

  if (!nxagentOpenScreen(nxagentDefaultScreen, nxagentArgc, nxagentArgv))
  {
    return False;
  }

  nxagentPixmap(pPixmap) = XCreatePixmap(nxagentDisplay, 
                                         nxagentDefaultWindows[nxagentDefaultScreen->myNum],
                                         pPixmap -> drawable.width,
                                         pPixmap -> drawable.height,
                                         pPixmap -> drawable.depth);
#ifdef NXAGENT_RECONNECT_SCREEN_DEBUG
  fprintf(stderr, "nxagentReconnectScreen: recreated %p - ID %lx\n",
                   pPixmap,
                   nxagentPixmap( pPixmap ));
#endif
  w = 16;
  h = 16;
  (*nxagentDefaultScreen->QueryBestSize)(StippleShape, &w, &h, nxagentDefaultScreen);
  if (!(nxagentPixmap(nxagentDefaultScreen->PixmapPerDepth[0]) =
       XCreatePixmap(nxagentDisplay, 
                     nxagentDefaultDrawables[1],
                     w, 
                     h, 
                     1)));

  nxagentGetDefaultEventMask(&mask);
  mask |= NXAGENT_KEYBOARD_EVENT_MASK | NXAGENT_POINTER_EVENT_MASK;
  nxagentSetDefaultEventMask(mask);
  XSelectInput(nxagentDisplay, nxagentDefaultWindows[0], mask);

  /*
   * Turn off the screen-saver and reset the
   * time to the next auto-disconnection.
   */

  SaveScreens(SCREEN_SAVER_OFF, ScreenSaverActive);

  lastDeviceEventTime.milliseconds = GetTimeInMillis();

  return True;  
}

/* intersect two rectangles */
static Bool intersect(int ax1, int ay1, unsigned int aw, unsigned int ah,
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

        #ifdef DEBUG
        fprintf(stderr, "intersect: the given rectangles do not intersect at all\n");
        #endif

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

        #ifdef DEBUG
        fprintf(stderr, "intersect: intersection rectangle not feasible\n");
        #endif

        return FALSE;
    }

    if (x) {
      *x = ix;
    }

    if (y) {
      *y = iy;
    }

    if (w) {
      *w = iw;
    }

    if (h) {
      *h = ih;
    }

    #ifdef DEBUG
    fprintf(stderr, "intersect: intersection is: ([%d],[%d]) [ %d x %d ]\n", (x) ? *(x) : (-1), (y) ? (*y) : (-1), (w) ? (*w) : (-1), (h) ? (*h) : (-1));
    #endif

    return TRUE;
}

#ifndef NXAGENT_RANDR_XINERAMA_CLIPPING
/* intersect two rectangles, return aw/ah for w/h if resulting
   rectangle is (partly) outside of bounding box */
static Bool intersect_bb(int ax1, int ay1, unsigned int aw, unsigned int ah,
	       int bx1, int by1, unsigned int bw, unsigned int bh,
	       int bbx1, int bby1, int bbx2, int bby2,
	       int *x, int *y, unsigned int *w, unsigned int *h)
{

  #ifdef DEBUG
  fprintf(stderr, "intersect_bb: session window: ([%d],[%d]) [ %d x %d ]\n", ax1, ay1, aw, ah);
  fprintf(stderr, "intersect_bb: crtc: ([%d],[%d]) [ %d x %d ]\n", bx1, by1, bw, bh);
  fprintf(stderr, "intersect_bb: bounding box: ([%d],[%d]) [ %d x %d ]\n", bbx1, bby1, bbx2-bbx1, bby2-bby1);
  #endif

  Bool result = intersect(ax1, ay1, aw, ah, bx1, by1, bw, bh, x, y, w, h);

  if (result == TRUE) {

    /*
     * ###### The X-Coordinate ######
     */

    /* check if outside-left of bounding box */
    if (bx1 == bbx1 && ax1 < bbx1) {

        *w += bbx1 - ax1;
        *x  = 0;

        #ifdef DEBUG
        fprintf(stderr, "intersect_bb: session box is outside-left of the bounding box - width gets adapted to [%d]\n", *w);
        #endif


    }

     /* check if outside-right of bounding box */
    if (bx1 + bw == bbx2 && ax1 + aw > bbx2) {

        *w += ax1 + aw - bbx2;

        #ifdef DEBUG
        fprintf(stderr, "intersect_bb: session box is outside-right of the bounding box - width gets adapted to [%d]\n", *w);
        #endif

    }

    /*
     * ###### The Y-Coordinate ######
     */

    /* check if outside-above of bounding box */
    if (by1 == bby1 && ay1 < bby1) {

        *h += bby1 - ay1;
        *y  = 0;

        #ifdef DEBUG
        fprintf(stderr, "intersect_bb: session box is outside-above of the bounding box - height gets adapted to [%d]\n", *h);
        #endif

    }

     /* check if outside-below of bounding box */
    if (by1 + bh == bby2 && ay1 + ah > bby2) {

        *h += ay1 + ah - bby2;

        #ifdef DEBUG
        fprintf(stderr, "intersect_bb: session box is outside-below of the bounding box - height gets adapted to [%d]\n", *h);
        #endif

    }

  }
  return result;
}
#endif

RRModePtr    nxagentRRCustomMode = NULL;

/*
   This is basically the code that was used on screen resize before
   xinerama was implemented. We need it as fallback if the user
   disables xinerama
*/
void nxagentAdjustCustomMode(ScreenPtr pScreen)
{
  rrScrPrivPtr pScrPriv = rrGetScrPriv(pScreen);
  RROutputPtr  output;

  if (pScrPriv)
  {
    output = RRFirstOutput(pScreen);

    if (output && output -> crtc)
    {
      RRCrtcPtr crtc;
      char         name[100];
      xRRModeInfo  modeInfo;
      const int    refresh = 60;
      int          width = nxagentOption(Width);
      int          height = nxagentOption(Height);

      crtc = output -> crtc;

      for (int c = 0; c < pScrPriv -> numCrtcs; c++)
      {
        RRCrtcSet(pScrPriv -> crtcs[c], NULL, 0, 0, RR_Rotate_0, 0, NULL);
      }

      memset(&modeInfo, '\0', sizeof(modeInfo));
      sprintf(name, "%dx%d", width, height);

      modeInfo.width  = width;
      modeInfo.height = height;
      modeInfo.hTotal = width;
      modeInfo.vTotal = height;
      modeInfo.dotClock = ((CARD32) width * (CARD32) height *
                           (CARD32) refresh);
      modeInfo.nameLength = strlen(name);

      if (nxagentRRCustomMode != NULL)
      {
        RROutputDeleteUserMode(output, nxagentRRCustomMode);
        FreeResource(nxagentRRCustomMode -> mode.id, 0);

        if (crtc != NULL && crtc -> mode == nxagentRRCustomMode)
        {
          RRCrtcSet(crtc, NULL, 0, 0, RR_Rotate_0, 0, NULL);
        }

        #ifdef TEST
        fprintf(stderr, "%s: Going to destroy mode %p with refcnt %d.\n",
                __func__, nxagentRRCustomMode, nxagentRRCustomMode->refcnt);
        #endif

        RRModeDestroy(nxagentRRCustomMode);
      }

      nxagentRRCustomMode = RRModeGet(&modeInfo, name);

      RROutputAddUserMode(output, nxagentRRCustomMode);

      RRCrtcSet(crtc, nxagentRRCustomMode, 0, 0, RR_Rotate_0, 1, &output);

      RROutputChanged(output, 1);
    }

    pScrPriv -> lastSetTime = currentTime;

    pScrPriv->changed = 1;
    pScrPriv->configChanged = 1;
  } /* if (pScrPriv) */

  RRScreenSizeNotify(pScreen);
}

int nxagentChangeScreenConfig(int screen, int width, int height)
{
  ScreenPtr    pScreen;
  int          r;

  #ifdef DEBUG
  fprintf(stderr, "nxagentChangeScreenConfig: called for screen [%d], width [%d] height [%d]\n", screen, width, height);
  #endif

  #ifdef TEST
  fprintf(stderr, "nxagentChangeScreenConfig: screenInfo.screens[%d]->root [%p]\n", screen, (void *) screenInfo.screens[screen]);
  #endif
  if (screenInfo.screens[screen]->root == NULL)
  {
    return 0;
  }

  UpdateCurrentTime();

  #ifdef DEBUG
  if (nxagentGrabServerInfo.grabstate == SERVER_GRABBED)
    fprintf(stderr, "nxagentChangeScreenConfig: grabstate [SERVER_GRABBED], client [%p]\n", (void *) nxagentGrabServerInfo.client);
  else if (nxagentGrabServerInfo.grabstate == SERVER_UNGRABBED)
    fprintf(stderr, "nxagentChangeScreenConfig: grabstate [SERVER_UNGRABBED], client [%p]\n", (void *) nxagentGrabServerInfo.client);
  else if (nxagentGrabServerInfo.grabstate == CLIENT_PERVIOUS)
    fprintf(stderr, "nxagentChangeScreenConfig: grabstate [CLIENT_PERVIOUS], client [%p]\n", (void *) nxagentGrabServerInfo.client);
  else if (nxagentGrabServerInfo.grabstate == CLIENT_IMPERVIOUS)
    fprintf(stderr, "nxagentChangeScreenConfig: grabstate [CLIENT_IMPERVIOUS], client [%p]\n", (void *) nxagentGrabServerInfo.client);
  else
    fprintf(stderr, "nxagentChangeScreenConfig: grabstate [UNKNOWN], client [%p]\n", (void *) nxagentGrabServerInfo.client);
  #endif

  if (nxagentGrabServerInfo.grabstate == SERVER_GRABBED && nxagentGrabServerInfo.client != NULL)
  {
    /*
     * If any client grabbed the server it won't expect screen
     * configuration changes until it releases the grab. That could
     * lead to an X error because available modes are changed
     * in the meantime.
     */

    #ifdef TEST
    fprintf(stderr, "nxagentChangeScreenConfig: Cancel with grabbed server (grab held by [%p]).\n", (void *) nxagentGrabServerInfo.client);
    #endif

    return 0;
  }

  pScreen = screenInfo.screens[screen] -> root -> drawable.pScreen;

  #ifdef TEST
  fprintf(stderr, "nxagentChangeScreenConfig: Changing config to %d x %d\n", width, height);
  #endif

  r = nxagentResizeScreen(pScreen, width, height, 0, 0);

  if (r != 0)
  {
    if (nxagentOption(Xinerama) && (noRRXineramaExtension == FALSE))
    {
      nxagentAdjustRandRXinerama(pScreen);
    }
    else
    {
      #ifdef DEBUG
      fprintf(stderr, "%s: Xinerama is disabled\n", __func__);
      #endif

      nxagentAdjustCustomMode(pScreen);
    }
  }

  #ifdef DEBUG
  fprintf(stderr, "nxagentChangeScreenConfig: current geometry: %d,%d %dx%d\n", nxagentOption(X), nxagentOption(Y), nxagentOption(Width), nxagentOption(Height));
  fprintf(stderr, "nxagentChangeScreenConfig: returning [%d]\n", r);
  #endif

  return r;
}

/*
 * Structure containing a list of splits.
 *
 * Only used locally, not exported.
 */
typedef struct {
  size_t  x_count;
  size_t  y_count;
  INT32  *x_splits;
  INT32  *y_splits;
} nxagentScreenSplits;

/*
 * Structure containing the boxes an nxagent session window is split into.
 *
 * Only used locally, not exported.
 */
typedef struct {
  struct xorg_list entry;
  Bool             obsolete;
  ssize_t          screen_id; /* Mapping to actual screen. */
  BoxPtr           box; /*
                         * You might be tempted to use RegionPtr here. Do not.
                         * A region is really an ordered and minimal set of horizontal
                         * bands. A window tiling will in most cases not be minimal,
                         * due to the need to split at display boundaries.
                         * A Region is more minimal than this.
                         * C.f., https://web.archive.org/web/20170520132137/http://magcius.github.io:80/xplain/article/regions.html
                         */
} nxagentScreenBoxesElem;

typedef struct {
  struct xorg_list head;
  ssize_t          screen_id; /* Mapping to actual screen. */
} nxagentScreenBoxes;

/*
 * Structure containing a (potentially partial) solution to a Crtc tiling.
 *
 * Only used locally, not exported.
 */
typedef struct {
  struct xorg_list    entry;
  size_t              rating_size_change;
  size_t              rating_cover_penalty;
  size_t              rating_extended_boxes_count;
  double              rating; /* Calculated via the components above. */
  nxagentScreenBoxes *solution_boxes;
  nxagentScreenBoxes *all_boxes;
} nxagentScreenCrtcsSolution;

typedef struct xorg_list nxagentScreenCrtcsSolutions;

#define INVALID_RATING ((-1) * (DBL_MAX))

static nxagentScreenCrtcsSolution *nxagentScreenCrtcsTiling = NULL;

/*
 * Helper function that takes a potential split point, the window bounds,
 * a split count and a splits array.
 *
 * The function checks if the split point is within bounds, not already
 * contained in the list and adds a new split point, modifying both the
 * count parameter and the splits list.
 *
 * In case of errors, does nothing.
 */
static void nxagentAddToSplits(const CARD16 test_edge, const CARD16 rwin_start, const CARD16 rwin_end, size_t *split_count, INT32 *splits) {
  if ((!(split_count)) || (!(splits))) {
    return;
  }

  /* FIXME: think through and check edge case: 1px split. */
  if ((rwin_start < test_edge) && (rwin_end > test_edge)) {
    /* Edge somewhere inside of agent window, split point. */

    /* Filter out if split point exists already. */
    Bool exists = FALSE;
    for (size_t i = 0; i < (*split_count); ++i) {
      if (splits[i] == test_edge) {
        exists = TRUE;
        break;
      }
    }

    if (!(exists)) {
      splits[(*split_count)++] = test_edge;
    }
  }
}

/* Helper function used while sorting the split lists. */
static int nxagentCompareSplits(const void *lhs, const void *rhs) {
  int ret = 0;

  const INT32 lhs_ = *((INT32*)(lhs)),
              rhs_ = *((INT32*)(rhs));

  if (lhs_ < rhs_) {
    ret = -1;
  }
  else if (lhs_ > rhs_) {
    ret = 1;
  }

  return(ret);
}

/* Helper function that deallocates a split list. */
static void nxagentFreeSplits(nxagentScreenSplits **splits) {
  if ((!(splits)) || (!(*splits))) {
    return;
  }

  nxagentScreenSplits *splits_ = (*splits);

  splits_->x_count = 0;
  splits_->y_count = 0;

  SAFE_FREE(splits_->x_splits);
  SAFE_FREE(splits_->y_splits);
  SAFE_FREE(splits_);
}

/*
 * Helper function compacting an nxagentScreenSplits structure.
 * The initial allocation assumes the worst case and overallocates memory,
 * upper bounded by the maximum count of possible splits for a given screen
 * configuration.
 * This function compacts the lists back down into what is necessary only.
 *
 * In case of memory allocation errors, it frees all allocated datan and sets
 * the splits list pointer to NULL!
 */
static void nxagentCompactSplits(nxagentScreenSplits **splits, const size_t actual_x_count, const size_t actual_y_count) {
  if ((!(splits)) || (!(*splits))) {
    return;
  }

  nxagentScreenSplits *splits_ = (*splits);

  splits_->x_count = actual_x_count;
  INT32 *new_data = NULL;
  if (actual_x_count) {
    /* Compact to accomodate actual data. */
    new_data = realloc(splits_->x_splits, sizeof(INT32) * actual_x_count);

    if (!(new_data)) {
      nxagentFreeSplits(splits);

      return;
    }

    splits_->x_splits = new_data;
  }
  else {
    /* No splits in this dimension, drop data. */
    SAFE_FREE(splits_->x_splits);
  }

  splits_->y_count = actual_y_count;
  if (actual_y_count) {
    new_data = realloc(splits_->y_splits, sizeof(INT32) * actual_y_count);

    if (!(new_data)) {
      nxagentFreeSplits(splits);

      return;
    }

    splits_->y_splits = new_data;
  }
  else {
    /* No splits in this dimension, drop data. */
    SAFE_FREE(splits_->y_splits);
  }
}

/*
 * Generate a list of splits.
 * This is based upon the client's system configuration and will
 * generally create more splits than we need/want.
 *
 * In case of errors, returns NULL.
 */
static nxagentScreenSplits* nxagentGenerateScreenSplitList(const XineramaScreenInfo *screen_info, const size_t screen_count) {
  nxagentScreenSplits *ret = NULL;

  if (!(screen_info)) {
    return(ret);
  }

  /*
   * The maximum number of split points per axis
   * is screen_count * 2 due to the rectangular nature of a screen
   * (and hence two vertical or horizontal edges).
   */
  size_t split_counts = (screen_count * 2);

  ret = calloc(1, sizeof(nxagentScreenSplits));
  if (!ret) {
    return(ret);
  }

  ret->x_splits = calloc(split_counts, sizeof(INT32));
  ret->y_splits = calloc(split_counts, sizeof(INT32));
  if ((!(ret->x_splits)) || (!(ret->y_splits))) {
    SAFE_FREE(ret->x_splits);
    SAFE_FREE(ret);

    return(ret);
  }

  /*
   * Initialize split arrays to -1, denoting no split.
   * Could be done only for the first invalid element, but play it safe.
   */
  for (size_t i = 0; i < split_counts; ++i) {
    ret->x_splits[i] = ret->y_splits[i] = -1;
  }

  const CARD16 sess_x_start = nxagentOption(X),
               sess_y_start = nxagentOption(Y),
               sess_x_end   = (sess_x_start + nxagentOption(Width)),
               sess_y_end   = (sess_y_start + nxagentOption(Height));

  size_t actual_x_splits = 0,
         actual_y_splits = 0;

  for (size_t i = 0; i < screen_count; ++i) {
    /* Handle x component. */
    size_t start_x = screen_info[i].x_org,
           end_x = start_x + screen_info[i].width;

    /* Left edge. */
    nxagentAddToSplits(start_x, sess_x_start, sess_x_end, &actual_x_splits, ret->x_splits);

    /* Right edge. */
    nxagentAddToSplits(end_x, sess_x_start, sess_x_end, &actual_x_splits, ret->x_splits);

    /* Handle y component. */
    size_t start_y = screen_info[i].y_org,
           end_y = start_y + screen_info[i].height;

    /* Top edge. */
    nxagentAddToSplits(start_y, sess_y_start, sess_y_end, &actual_y_splits, ret->y_splits);

    /* Bottom edge. */
    nxagentAddToSplits(end_y, sess_y_start, sess_y_end, &actual_y_splits, ret->y_splits);
  }

  /*
   * Fetched all split points.
   * Compact data and handle errors.
   */
  nxagentCompactSplits(&ret, actual_x_splits, actual_y_splits);

  return(ret);
}

/* Helper function printing out a splits list. */
static void nxagentPrintSplitsList(const INT32 *splits, const size_t count, const char *func, const Bool is_x) {
  if (!(func)) {
    func = "unknown function";
  }

  if (!(splits)) {
    fprintf(stderr, "%s: tried to print invalid split list.\n", func);
    return;
  }

  fprintf(stderr, "%s: ", func);
  if (is_x) {
    fprintf(stderr, "X");
  }
  else {
    fprintf(stderr, "Y");
  }
  fprintf(stderr, " split list: [");

  for (size_t i = 0; i < count; ++i) {
    if (i > 0) {
      fprintf(stderr, ", ");
    }

    fprintf(stderr, "%u", splits[i]);
  }

  fprintf(stderr, "]\n");
}

/* Helper to clear out a screen boxes list. */
static void nxagentFreeScreenBoxes(nxagentScreenBoxes *boxes, const Bool free_data) {
  if (!(boxes)) {
    return;
  }

  nxagentScreenBoxesElem *cur  = NULL,
                         *next = NULL;

  xorg_list_for_each_entry_safe(cur, next, &(boxes->head), entry) {
    xorg_list_del(&(cur->entry));

    if (free_data) {
      SAFE_FREE(cur->box);
    }

    SAFE_FREE(cur);
  }

  xorg_list_init(&(boxes->head));
}

/*
 * Given a list of splits, sorts them and calculates a tile pattern for the
 * current window.
 *
 * In case of errors, returns an empty list or NULL.
 */
static nxagentScreenBoxes* nxagentGenerateScreenCrtcs(nxagentScreenSplits *splits) {
  nxagentScreenBoxes *ret = NULL;

  ret = calloc(1, sizeof(nxagentScreenBoxes));

  if (!(ret)) {
    return(ret);
  }

  xorg_list_init(&(ret->head));
  ret->screen_id = -1;

  if ((!(splits)) || ((!(splits->x_splits)) && (splits->x_count)) || ((!(splits->y_splits)) && (splits->y_count))) {
    return(ret);
  }

  /*
   * Could drop these tests, since we checked the "if count is not zero the
   * pointers must exist" constraint above already, but play it safe.
   */
  if (splits->x_splits) {
    qsort(splits->x_splits, splits->x_count, sizeof(*(splits->x_splits)), nxagentCompareSplits);
  }
  if (splits->y_splits) {
    qsort(splits->y_splits, splits->y_count, sizeof(*(splits->y_splits)), nxagentCompareSplits);
  }

#ifdef DEBUG
  nxagentPrintSplitsList(splits->x_splits, splits->x_count, __func__, TRUE);
  nxagentPrintSplitsList(splits->y_splits, splits->y_count, __func__, FALSE);
#endif

  /*
   * Since the split lists are sorted now, we can go ahead and create boxes in
   * an iterative manner.
   */
#ifdef DEBUG
  {
    /* Avoid using %zu printf specifier which may not be supported everywhere. */
    const unsigned long long x_count = splits->x_count;
    const unsigned long long y_count = splits->y_count;
    fprintf(stderr, "%s: should generate (x_splits [%llu] + 1) * (y_splits [%llu] + 1) = %llu boxes.\n", __func__, x_count, y_count, (x_count + 1) * (y_count + 1));
  }
#endif
  /*                    v-- implicit split point at agent window's bottommost edge! */
  for (size_t i = 0; i <= splits->y_count; ++i) {
    /* Looping over "rows" here (hence y splits). */
    CARD32 start_y = -1,
           end_y   = -1;

    if (0 == i) {
      start_y = nxagentOption(Y);
    }
    else {
      start_y = splits->y_splits[i - 1];
    }

    if (i == splits->y_count) {
      end_y = (nxagentOption(Y) + nxagentOption(Height));
    }
    else {
      end_y = splits->y_splits[i];
    }

    /*                    v-- implicit split point at agent window's rightmost edge! */
    for (size_t y = 0; y <= splits->x_count; ++y) {
      /* Looping over "cols" here (hence x splits). */
      CARD32 start_x = -1,
             end_x   = -1;

      if (0 == y) {
        start_x = nxagentOption(X);
      }
      else {
        start_x = splits->x_splits[y - 1];
      }

      if (y == splits->x_count) {
        end_x = (nxagentOption(X) + nxagentOption(Width));
      }
      else {
        end_x = splits->x_splits[y];
      }

      nxagentScreenBoxesElem *new_box_list_entry = calloc(1, sizeof(nxagentScreenBoxesElem));

      if (!(new_box_list_entry)) {
        nxagentFreeScreenBoxes(ret, TRUE);

        return(ret);
      }

      new_box_list_entry->screen_id = -1;

      BoxPtr new_box = calloc(1, sizeof(BoxRec));

      if (!(new_box)) {
        nxagentFreeScreenBoxes(ret, TRUE);

        return(ret);
      }

      /* Box extends from (start_x, start_y) to (end_x, end_y). */
      new_box->x1 = start_x;
      new_box->x2 = end_x;
      new_box->y1 = start_y;
      new_box->y2 = end_y;

      new_box_list_entry->box = new_box;

      xorg_list_append(&(new_box_list_entry->entry), &(ret->head));
    }
  }

#if defined(DEBUG) || defined(WARNING)
  {
    unsigned long long count = 0;
    nxagentScreenBoxesElem *cur = NULL;
    xorg_list_for_each_entry(cur, &(ret->head), entry) {
      ++count;
    }
#ifdef DEBUG
    fprintf(stderr, "%s: generated %llu boxes\n", __func__, count);
#endif
    if (count < ((splits->x_count + 1) * (splits->y_count + 1))) {
      const unsigned long long expect = ((splits->x_count + 1) * (splits->y_count + 1));
      fprintf(stderr, "%s: WARNING! Generated only %llu boxes, expected: %llu\n", __func__, count, expect);
    }
  }
#endif

  return(ret);
}

/*
 * Helper returning true if a given box intersects with a given screen,
 * otherwise and on error false.
 */
static Bool nxagentScreenBoxIntersectsScreen(const nxagentScreenBoxesElem *box, const XineramaScreenInfo *screen_info) {
  Bool ret = FALSE;

  if ((!(box)) || (!(box->box)) || (!(screen_info))) {
    return(ret);
  }

  /* Find out if this box has intersections with display. */
  INT32 box_width  = (box->box->x2 - box->box->x1),
        box_height = (box->box->y2 - box->box->y1);
  ret = intersect(box->box->x1, box->box->y1,
                  box_width, box_height,
                  screen_info->x_org, screen_info->y_org,
                  screen_info->width, screen_info->height,
                  NULL, NULL, NULL, NULL);

  return(ret);
}

/* Helper printing out a single screen box. */
static char* nxagentPrintScreenBoxesElem(const nxagentScreenBoxesElem *box) {
  char *ret = NULL;

  if ((!(box)) || (!(box->box))) {
    return(ret);
  }

  BoxPtr box_data = box->box;

  char *construct = NULL;
  {
    const signed long long screen_id = box->screen_id;
    const unsigned obsolete = box->obsolete;
    if (-1 == asprintf(&construct, "(%u)[(%d, %d), (%d, %d)]->%lld", obsolete, box_data->x1, box_data->y1, box_data->x2, box_data->y2, screen_id)) {
      return(ret);
    }
  }

  ret = construct;

  return(ret);
}

/*
 * Helper comparing two box elements.
 * Returns true if both data sections match, false otherwise or on error.
 */
static Bool nxagentCompareScreenBoxData(const BoxPtr lhs, const BoxPtr rhs) {
  Bool ret = FALSE;

  if ((!(lhs)) || (!(rhs))) {
    return(ret);
  }

  if ((lhs->x1 == rhs->x1) && (lhs->x2 == rhs->x2) && (lhs->y1 == rhs->y1) && (lhs->y2 == rhs->y2)) {
    ret = TRUE;
  }

  return(ret);
}

/*
 * Helper marking a box in the all boxes list as obsolete.
 *
 * Returns true if such a box has been found (and marked), false otherwise
 * or on error.
 */
static Bool nxagentScreenBoxMarkObsolete(nxagentScreenBoxes *all_boxes, const nxagentScreenBoxesElem *ref) {
  Bool ret = FALSE;

  if ((!(ref)) || (!(all_boxes)) || (!(ref->box))) {
    return(ret);
  }

  nxagentScreenBoxesElem *cur_obsolete = NULL;
  xorg_list_for_each_entry(cur_obsolete, &(all_boxes->head), entry) {
    if (nxagentCompareScreenBoxData(cur_obsolete->box, ref->box)) {
      cur_obsolete->obsolete = TRUE;

      ret = TRUE;

      break;
    }
  }

  return(ret);
}

/*
 * Helper for checking box adjacency.
 *
 * The edges for which adjacency should be detected can be toggled via the
 * left, right, top or bottom parameters. Checking for adjacency with no edge
 * selected is an error.
 *
 * Adjancency can be strict of loose.
 * If strict checking is enabled via the strict parameter, boxes must strictly
 * share one of the enabled edges.
 * If strict checking is disabled, sharing only part of an selected edge is
 * enough make the check succeed.
 *
 * Returns true if boxes are adjacent to each other, false otherwise or on
 * error.
 */
static Bool nxagentCheckBoxAdjacency(const BoxPtr lhs, const BoxPtr rhs, const Bool strict, const Bool left, const Bool right, const Bool top, const Bool bottom) {
  Bool ret = FALSE;

  if ((!(lhs)) || (!(rhs)) || ((!(left)) && (!(right)) && (!(top)) && (!(bottom)))) {
    return(ret);
  }

  if (((left) && (lhs->x1 == rhs->x2)) || ((right) && (lhs->x2 == rhs->x1))) {
    if (strict) {
      if ((lhs->y1 == rhs->y1) && (lhs->y2 == rhs->y2)) {
        ret = TRUE;
      }
    }
    else {
      /* rhs->{y1,y2-1} within {lhs->y1, lhs->y2) */
      if (((rhs->y1 >= lhs->y1) && (rhs->y1 < lhs->y2)) || ((rhs->y2 > lhs->y1) && (rhs->y2 <= lhs->y2))) {
        ret = TRUE;
      }
    }
  }

  if (((top) && (lhs->y1 == rhs->y2)) || ((bottom) && (lhs->y2 == rhs->y1))) {
    if (strict) {
      if ((lhs->x1 == rhs->x1) && (lhs->x2 == rhs->x2)) {
        ret = TRUE;
      }
    }
    else {
      /* rhs->{x1,x2} within {lhs->x1, lhs->x2) */
      if (((rhs->x1 >= lhs->x1) && (rhs->x1 < lhs->x2)) || ((rhs->x2 > lhs->x1) && (rhs->x2 <= lhs->x2))) {
        ret = TRUE;
      }
    }
  }

  return(ret);
}

/*
 * Helper used to shallow- or deep-copy an nxagentScreenBoxesElem object.
 *
 * Returns a pointer to the copy or NULL on failure.
 */
static nxagentScreenBoxesElem* nxagentScreenBoxesElemCopy(const nxagentScreenBoxesElem *box, const Bool deep) {
  nxagentScreenBoxesElem *ret = NULL;

  if (!(box)) {
    return(ret);
  }

  ret = calloc(1, sizeof(nxagentScreenBoxesElem));

  if (!(ret)) {
    return(ret);
  }

  memmove(ret, box, sizeof(*box));

  xorg_list_init(&(ret->entry));

  /* For a deep copy, also copy the data. */
  if (deep) {
    BoxPtr new_box_data = calloc(1, sizeof(BoxRec));

    if (!(new_box_data)) {
      SAFE_FREE(ret);

      return(ret);
    }

    memmove(new_box_data, box->box, sizeof(*(box->box)));

    ret->box = new_box_data;
  }

  return(ret);
}

/*
 * Helper merging boxes on a low level.
 * Returns true if merging succeeded, otherwise or on error false.
 *
 * If merging succeded, the merge_boxes list shall contain only one element:
 * the extended box representing a screen.
 *
 * In case of errors, the original list may or may not be modified.
 * Assume that the data is invalid.
 */
static Bool nxagentMergeBoxes(nxagentScreenBoxes *all_boxes, nxagentScreenBoxes *merge_boxes) {
  Bool ret = FALSE;

  if ((!(all_boxes)) || (!(merge_boxes))) {
    return(ret);
  }

  /*
   * Special case: merge_boxes is empty. In such a case, return TRUE
   * immediately.
   */
  if (xorg_list_is_empty(&(merge_boxes->head))) {
    ret = TRUE;

    return(ret);
  }

  /* Naïve approach: merge of all boxes is the bounding box. */
  BoxRec bounding_box = { 0 };
  nxagentScreenBoxesElem *cur = NULL;
  Bool init = FALSE;
  size_t merge_boxes_count = 0;
  xorg_list_for_each_entry(cur, &(merge_boxes->head), entry) {
    if (!(cur->box)) {
      return(ret);
    }

    if (!(init)) {
      bounding_box.x1 = cur->box->x1;
      bounding_box.x2 = cur->box->x2;
      bounding_box.y1 = cur->box->y1;
      bounding_box.y2 = cur->box->y2;

      init = TRUE;

      ++merge_boxes_count;

      continue;
    }

    bounding_box.x1 = MIN(cur->box->x1, bounding_box.x1);
    bounding_box.x2 = MAX(cur->box->x2, bounding_box.x2);
    bounding_box.y1 = MIN(cur->box->y1, bounding_box.y1);
    bounding_box.y2 = MAX(cur->box->y2, bounding_box.y2);

    ++merge_boxes_count;
  }

  /*
   * Don't treat one box to merge as a special case.
   * Returning directly is a mistake since in this case the corresponding box
   * in the all boxes list wouldn't be correctly marked as obsolete.
   *
   * Copying the code for doing this into a special branch here doesn't make
   * sense, so just let the normal algorithm handle that.
   */

  /* Try to find a suitable merge pair. */
  cur = NULL;
  nxagentScreenBoxesElem *merge_rhs = NULL;
  Bool restart = TRUE;
  while (restart) {
    restart = FALSE;

    xorg_list_for_each_entry(cur, &(merge_boxes->head), entry) {
      if (!(cur->box)) {
        return(ret);
      }

      /*
       * Mark left-hand side box as obsolete.
       * We pick a box (usually the first one, but if there no mergable boxes
       * readily available maybe others), search for a different mergable box,
       * merge the picked box with the other box by extending it and mark the
       * right-hand side original box as obsolete in the all boxes list.
       *
       * Observant readers might have noticed that we will never mark a picked
       * box as obsolete this way, so we'll have to work around that issue at
       * the beginning.
       */
      if (cur->obsolete) {
        if (!(nxagentScreenBoxMarkObsolete(all_boxes, cur))) {
          /* False means that we haven't found a box to mark obsolete. */
#ifdef WARNING
          fprintf(stderr, "%s: couldn't find left-hand box in original list - box has likely already been merged into a different one.\n", __func__);
#endif
        }
      }

      xorg_list_for_each_entry(merge_rhs, &(cur->entry), entry) {
        if (&(merge_rhs->entry) == &(merge_boxes->head)) {
          /* Reached end of list. */
          merge_rhs = NULL;
          break;
        }

        /*
         * Do not move this up.
         *
         * The list head won't have a box element and accessing it would
         * actually read random data.
         */
        if (!(merge_rhs->box)) {
          return(ret);
        }

        /* Check adjacency. */
        if (nxagentCheckBoxAdjacency(cur->box, merge_rhs->box, TRUE, TRUE, TRUE, TRUE, TRUE)) {
          break;
        }
      }

      /* cur and merge_rhs are mergeable. */
      /*
       * Side note, since working with lists is tricky: normally, the element
       * pointer shouldn't be used after iterating over all list elements.
       * In such a case, it would point to an invalid entry, since the list
       * head is a simple xorg_list structure instead of an element-type.
       * The looping macro takes care of this internally by always checking
       * it->member against the known list head, which adds a previously
       * subtracted offset back to the pointer in the break statement. It does,
       * however, not modify the iteration pointer itself.
       * In this special case, continuing to use the iteration pointer is safe
       * due to always breaking out of the loop early when we know that we are
       * working on a valid element or setting the iterator pointer to NULL.
       */
      if (merge_rhs) {
#ifdef DEBUG
        {
          char *box_left_str = nxagentPrintScreenBoxesElem(cur);
          char *box_right_str = nxagentPrintScreenBoxesElem(merge_rhs);

          fprintf(stderr, "%s: mergeable boxes found: ", __func__);
          if (!(box_left_str)) {
            fprintf(stderr, "box with invalid data [%p]", (void*)(cur));
          }
          else {
            fprintf(stderr, "%s", box_left_str);
          }

          if (!(box_right_str)) {
            fprintf(stderr, ", box with invalid data [%p]\n", (void*)(merge_rhs));
          }
          else {
            fprintf(stderr, ", %s\n", box_right_str);
          }

          SAFE_FREE(box_left_str);
          SAFE_FREE(box_right_str);
        }
#endif
        cur->box->x1 = MIN(cur->box->x1, merge_rhs->box->x1);
        cur->box->x2 = MAX(cur->box->x2, merge_rhs->box->x2);
        cur->box->y1 = MIN(cur->box->y1, merge_rhs->box->y1);
        cur->box->y2 = MAX(cur->box->y2, merge_rhs->box->y2);

        /* Delete merge_rhs box out of merge list ... */
        xorg_list_del(&(merge_rhs->entry));

        /*
         * ... and mark an equivalent box in the all boxes list as obsolete.
         *
         * Note that it is not an error condition if no such box exists in the
         * all boxes list. More likely we tried to mark a box obsolete that
         * has already been merged with a different one (and now covers more
         * than one entry in the all boxes list).
         */
        if (merge_rhs->obsolete) {
          if (!(nxagentScreenBoxMarkObsolete(all_boxes, merge_rhs))) {
            /* False means that we haven't found a box to mark obsolete. */
#ifdef WARNING
            fprintf(stderr, "%s: merged boxes from merge list, but couldn't find right-hand box in original list - box has likely already been merged into a different one.\n", __func__);
#endif
          }
        }

        /*
         * Remove merge_rhs's internal box data.
         * Since it's a deep copy, only this element will be affected.
         */
        SAFE_FREE(merge_rhs->box);

        /*
         * At this point, merge_rhs's data has been free()d and the box
         * element is not part of the merge_boxes lists.
         * Delete the box element.
         */
        SAFE_FREE(merge_rhs);

        /*
         * Set restart flag and break out.
         *
         * After removing an entry from the list, we have to make sure to
         * restart the loop, since otherwise we'd be operating on free'd
         * data.
         *
         * An alternative would be to use xorg_list_for_each_entry_safe()
         * to skip over the removed element, but this wouldn't rewind the
         * pointer to the head element - which is what we also want to do.
         */
        restart = TRUE;

        break;
      }
      else {
#ifdef DEBUG
        char *box_str = nxagentPrintScreenBoxesElem(cur);

        fprintf(stderr, "%s: no mergeable box found for", __func__);
        if (box_str) {
          fprintf(stderr, " current box %s\n", box_str);
        }
        else {
          fprintf(stderr, " box with invalid data [%p]\n", (void*)(cur));
        }

        SAFE_FREE(box_str);
#endif
      }
    }
  }

  /* All boxes merged, we should only have one left. */
  merge_boxes_count = 0;
  xorg_list_for_each_entry(cur, &(merge_boxes->head), entry) {
    if (!(cur->box)) {
      return(ret);
    }

    ++merge_boxes_count;
  }

  if (1 < merge_boxes_count) {
#ifdef WARNING
    fprintf(stderr, "%s: WARNING: box merge operation produced more than one box - initial merge list was not a rectangle!\n", __func__);
#endif
  }
  else if (0 == merge_boxes_count) {
#ifdef WARNING
    fprintf(stderr, "%s: WARNING: box merge operation produced a merged box count of 0!\n", __func__);
#endif
  }
  else {
    /* Just take the first element, there should only be one box. */
    cur = xorg_list_first_entry(&(merge_boxes->head), nxagentScreenBoxesElem, entry);

    /*
     * Safe to use cur here as we know that list has exactly one element
     * and we break out directly at a point for which we know that the
     * pointer is valid.
     */
    if (nxagentCompareScreenBoxData(&bounding_box, cur->box)) {
#ifdef DEBUG
      fprintf(stderr, "%s: merging operations result is equal to bounding box, could have avoided complex calculations.\n", __func__);
#endif
      ret = TRUE;
    }
  }

  return(ret);
}

/*
 * Helper merging boxes that pertain to specific screen.
 * Expects a list of all boxes, an array of screen boxes, a screen info array
 * and the screen count.
 * The screen boxes array msut have been allocated by the caller with at least
 * screen count elements. Further elements are ignored.
 *
 * In case of errors, the original all boxes and screen boxes lists may or may
 * not be modified or even the data free'd.
 * Assume that the data is invalid.
 */
static Bool nxagentMergeScreenBoxes(nxagentScreenBoxes *all_boxes, nxagentScreenBoxes *screen_boxes, const XineramaScreenInfo *screen_info, const size_t screen_count) {
  Bool ret = FALSE;

  if ((!(all_boxes)) || (!(screen_boxes)) || (!(screen_info)) || (!(screen_count))) {
    return(ret);
  }

  for (size_t i = 0; i < screen_count; ++i) {
    /*
     * Structure holding the box elements intersecting with
     * the current screen.
     */
    nxagentScreenBoxes *cur_screen_boxes = (screen_boxes + i);
    xorg_list_init(&(cur_screen_boxes->head));
    cur_screen_boxes->screen_id = i;

    nxagentScreenBoxesElem *cur_box = NULL;
    xorg_list_for_each_entry(cur_box, &(all_boxes->head), entry) {
      Bool cur_intersect = nxagentScreenBoxIntersectsScreen(cur_box, &(screen_info[i]));

      if (cur_intersect) {
        /*
         * If a screen intersects the current box, we must:
         *  - create a deep copy of this box
         *  - add the copy to the screen boxes list
         *  - set the obsolete flag appropriately
         *  - start the low-level merge operation on the screen boxes list
         *
         * After this, assuming no error happened, the screen boxes list will
         * contain only one element: a box containing the screen area.
         */
        nxagentScreenBoxesElem *box_copy = nxagentScreenBoxesElemCopy(cur_box, TRUE);

        if (!(box_copy)) {
          nxagentFreeScreenBoxes(all_boxes, TRUE);
          nxagentFreeScreenBoxes(screen_boxes, TRUE);

          return(ret);
        }

        box_copy->screen_id = cur_screen_boxes->screen_id;
        box_copy->obsolete = TRUE;

        for (size_t y = (i + 1); y < screen_count; ++y) {
          if (nxagentScreenBoxIntersectsScreen(cur_box, &(screen_info[y]))) {
            /* Protect box, if still needed later on after merging this set. */
            box_copy->obsolete = FALSE;
            break;
          }
        }

        xorg_list_append(&(box_copy->entry), &(cur_screen_boxes->head));
      }
    }

    /* Actually merge the boxes. */
    if (!(nxagentMergeBoxes(all_boxes, cur_screen_boxes))) {
      return(ret);
    }
  }

  ret = TRUE;

  return(ret);
}

/*
 * Helper used to deep-copy an nxagentScreenBoxes list.
 *
 * Returns a pointer to a complete copy or NULL on failure.
 */
static nxagentScreenBoxes* nxagentScreenBoxesCopy(const nxagentScreenBoxes *boxes) {
  nxagentScreenBoxes *ret = NULL;

  if (!(boxes)) {
    return(ret);
  }

  ret = calloc(1, sizeof(nxagentScreenBoxes));

  if (!(ret)) {
    return(ret);
  }

  xorg_list_init(&(ret->head));

  nxagentScreenBoxesElem *cur = NULL;
  xorg_list_for_each_entry(cur, &(boxes->head), entry) {
    nxagentScreenBoxesElem *copy = nxagentScreenBoxesElemCopy(cur, TRUE);

    if (!(copy)) {
      nxagentFreeScreenBoxes(ret, TRUE);

      SAFE_FREE(ret);

      return(ret);
    }

    xorg_list_append(&(copy->entry), &(ret->head));
  }

  ret->screen_id = boxes->screen_id;

  return(ret);
}

/* Helper function that deallocates a single solution. */
static void nxagentScreenCrtcsFreeSolution(nxagentScreenCrtcsSolution *solution) {
  if (!(solution)) {
    return;
  }

  xorg_list_del(&(solution->entry));

  solution->rating_size_change = solution->rating_cover_penalty = solution->rating_extended_boxes_count = 0;
  solution->rating = 0.0;

  nxagentFreeScreenBoxes(solution->solution_boxes, TRUE);
  SAFE_FREE(solution->solution_boxes);

  nxagentFreeScreenBoxes(solution->all_boxes, TRUE);
  SAFE_FREE(solution->all_boxes);
}

/*
 * Helper to calculate a normalized sum for a list of boolean values.
 *
 * Each boolean value is normalized to to [0,1] range.
 *
 * Returns the sum of all normalized arguments.
 */
static size_t nxagentSumBools(const size_t count, ...) {
  size_t ret = 0;

  va_list ap;
  va_start(ap, count);
  for (size_t i = 0; i < count; ++i) {
    ret += (!(!(va_arg(ap, size_t))));
  }
  va_end(ap);

  return(ret);
}

/*
 * Helper returning true if a given box intersects with a given screen box,
 * otherwise and on error false.
 */
static Bool nxagentScreenBoxIntersectsScreenBox(const nxagentScreenBoxesElem *lhs, const nxagentScreenBoxesElem *rhs) {
  Bool ret = FALSE;

  if ((!(lhs)) || (!(lhs->box)) || (!(rhs)) || (!(rhs->box))) {
    return(ret);
  }

  /* Find out if this box has intersections with display. */
  INT32 lhs_width  = (lhs->box->x2 - lhs->box->x1),
        lhs_height = (lhs->box->y2 - lhs->box->y1),
        rhs_width  = (rhs->box->x2 - rhs->box->x1),
        rhs_height = (rhs->box->y2 - rhs->box->y1);
  ret = intersect(lhs->box->x1, lhs->box->y1,
                  lhs_width, lhs_height,
                  rhs->box->x1, rhs->box->y1,
                  rhs_width, rhs_height,
                  NULL, NULL, NULL, NULL);

  return(ret);
}

/*
 * Calculates the rating for a given solution.
 *
 * Expects the size change and cover penalty struct variables to be set
 * correctly.
 *
 * If boost is true, the rating calculated here will receive a small boost.
 *
 * On errors, sets the rating to INVALID_RATING or does nothing.
 */
static void nxagentScreenCrtcsSolutionCalculateRating(nxagentScreenCrtcsSolution *solution, const Bool boost) {
  if ((!(solution)) || (!(solution->all_boxes))) {
    return;
  }

  size_t all_boxes_count = 0;
  nxagentScreenBoxesElem *cur = NULL;
  xorg_list_for_each_entry(cur, &(solution->all_boxes->head), entry) {
    ++all_boxes_count;
  }

  if (!(all_boxes_count)) {
    solution->rating = INVALID_RATING;
    return;
  }

  solution->rating = (((double)(solution->rating_size_change) * ((double)(solution->rating_extended_boxes_count) + ((double)(solution->rating_size_change) / (double)(all_boxes_count)))) - (pow((double)(solution->rating_cover_penalty), 2.0)));

  if (boost) {
    /*
     * This is a magic number.
     * It should be big enough so that it boosts the rating a tiny bit to
     * prefer specific solutions, but small enough to never reach or exceed
     * a +1 boost.
     */
    solution->rating += (1.0 / 64.0);
  }
}

/*
 * Helper that generates a solution for extending a specific box in one
 * direction.
 *
 * Only one of the left, right, top or bottom parameters may be set to TRUE.
 *
 * The parameter "box" will be extended as much as possible in the specified
 * direction, with the following constraints:
 *   - it's extended at least once unless hitting the nxagent window edge
 *     directly
 *   - it's further extended step-by-step IFF there are no "obsolete" boxes
 *     (i.e., base-level boxes already covered by a different screen box) in
 *     the extension direction.
 *
 * Note that the initial extension might not be a meaningful one. Calling
 * functions must check the rating and determine if the solution is viable
 * to them.
 */
static nxagentScreenCrtcsSolution* nxagentScreenCrtcsGenerateSolutionsSingleStep(const nxagentScreenBoxes *all_boxes, const nxagentScreenBoxesElem *box, const nxagentScreenBoxes *other_screens, const Bool left, const Bool right, const Bool top, const Bool bottom) {
  nxagentScreenCrtcsSolution *ret = NULL;

  const size_t sum = nxagentSumBools(4, left, right, top, bottom);
  if ((0 == sum) || (1 < sum) || (!(all_boxes)) || (!(box)) || (!(other_screens))) {
    return(ret);
  }

  ret = calloc(1, sizeof(nxagentScreenCrtcsSolution));

  if (!(ret)) {
    return(ret);
  }

  xorg_list_init(&(ret->entry));
  ret->rating = INVALID_RATING;

  Bool init = TRUE,
       cont = TRUE;
  const nxagentScreenBoxesElem *box_ref = box;
  const nxagentScreenBoxes *all_boxes_ref = all_boxes;
  while (cont) {
    /*
     * Move one step into selected direction, unless hitting an obsolete block or the
     * window edge. Obsolete blocks are not an obstacle for the initial run.
     */
    size_t run_size_change = 0,
           run_cover_penalty = 0;
    nxagentScreenBoxesElem *cur = NULL;
    nxagentScreenBoxes *merge_list = calloc(1, sizeof(nxagentScreenBoxes));

    if (!(merge_list)) {
      nxagentScreenCrtcsFreeSolution(ret);

      SAFE_FREE(ret);

      return(ret);
    }

    xorg_list_init(&(merge_list->head));
    merge_list->screen_id = -1;
    xorg_list_for_each_entry(cur, &(all_boxes->head), entry) {
      /*
       * Skip boxes already covered by this screen or other screens
       * if we're looking for additional coverage.
       */
      if ((!(init)) && (cur->obsolete)) {
        continue;
      }

      if (!(box_ref) || (!(box_ref->box)) || (!(cur->box))) {
        nxagentFreeScreenBoxes(merge_list, TRUE);

        nxagentScreenCrtcsFreeSolution(ret);

        SAFE_FREE(merge_list);

        SAFE_FREE(ret);

        return(ret);
      }

      if (nxagentCheckBoxAdjacency(box_ref->box, cur->box, FALSE, left, right, top, bottom)) {
        /* Copy current box. */
        nxagentScreenBoxesElem *copy = nxagentScreenBoxesElemCopy(cur, TRUE);

        if (!(copy)) {
          nxagentFreeScreenBoxes(merge_list, TRUE);

          nxagentScreenCrtcsFreeSolution(ret);

          SAFE_FREE(merge_list);

          SAFE_FREE(ret);

          return(ret);
        }

        copy->obsolete = TRUE;

        xorg_list_append(&(copy->entry), &(merge_list->head));

        ++run_size_change;

        /*
         * Add a penalty value, if current box is already covered by at least
         * one different screen box.
         */
        nxagentScreenBoxesElem *cur_penalty_box = NULL;
        /* FIXME: do we need more thorough error-checking for other_screens? */
        xorg_list_for_each_entry(cur_penalty_box, &(other_screens->head), entry) {
          if (nxagentScreenBoxIntersectsScreenBox(cur, cur_penalty_box)) {
            /*
             * Add an initial penalty the first time for our current screen
             * box.
             */
            if (0 == run_cover_penalty) {
              ++run_cover_penalty;
            }

            ++run_cover_penalty;
          }
        }
      }

      /*
       * Break out early if possible.
       * This assumes that the all_boxes list is sorted according to rows and cols.
       */
      if ((left) || (right)) {
        /*
         * Discovering a box that is below the screen box means that any
         * following boxes will be unsuitable for horizontal expansion.
         */
        if (cur->box->y1 >= box_ref->box->y2) {
          break;
        }
      }

      if (top) {
        /*
         * Discovering a box that is below the screen box's top edge means
         * that any following boxes will be unsuitable for vertical expansion.
         */
        if (cur->box->y1 >= box_ref->box->y1) {
          break;
        }
      }
    }

    /*
     * At this point, merge_list should be populated with a list of boxes
     * to merge with the current screen box.
     * If it is incomplete (i.e., smaller height than the screen box for
     * horizontal expansions or smaller width than the screen box for vertical
     * expansions), merging will fail.
     * Such a situation is fine, but will mean that we cannot extend the box.
     */
    if (!(xorg_list_is_empty(&(merge_list->head)))) {
      nxagentScreenBoxes *all_boxes_copy = nxagentScreenBoxesCopy(all_boxes_ref);

      if (!(all_boxes_copy)) {
        nxagentFreeScreenBoxes(merge_list, TRUE);

        nxagentScreenCrtcsFreeSolution(ret);

        SAFE_FREE(merge_list);

        SAFE_FREE(ret);

        return(ret);
      }

      /* Deep-copy original screen box. */
      nxagentScreenBoxesElem *screen_box_copy = nxagentScreenBoxesElemCopy(box_ref, TRUE);

      if (!(screen_box_copy)) {
        nxagentFreeScreenBoxes(all_boxes_copy, TRUE);
        nxagentFreeScreenBoxes(merge_list, TRUE);

        nxagentScreenCrtcsFreeSolution(ret);

        SAFE_FREE(all_boxes_copy);
        SAFE_FREE(merge_list);

        SAFE_FREE(ret);

        return(ret);
      }

      /* Add copy to merge list. */
      /*
       * DO NOT change this to xorg_list_append(). Putting the screen box first
       * means that, theoretically, all other boxes will be merged into it and
       * we can assume that its screen_id entry stays valid.
       */
      xorg_list_add(&(screen_box_copy->entry), &(merge_list->head));

      /* Merge. */
      if (!(nxagentMergeBoxes(all_boxes_copy, merge_list))) {
        /*
         * Merging failed. Forgetting data and stopping extension.
         *
         * Make sure to not clear out ret. We want to retain and return a
         * previous solution/extension.
         */
        nxagentFreeScreenBoxes(all_boxes_copy, TRUE);
        nxagentFreeScreenBoxes(merge_list, TRUE);

        SAFE_FREE(all_boxes_copy);
        SAFE_FREE(merge_list);

        cont = FALSE;
      }
      else {
        /* Merge successful. Create solution. */
        nxagentFreeScreenBoxes(ret->all_boxes, TRUE);
        SAFE_FREE(ret->all_boxes);
        ret->all_boxes = all_boxes_copy;

        nxagentFreeScreenBoxes(ret->solution_boxes, TRUE);
        SAFE_FREE(ret->solution_boxes);
        ret->solution_boxes = nxagentScreenBoxesCopy(merge_list);

        /* Unconditionally get rid of data. */
        nxagentFreeScreenBoxes(merge_list, TRUE);

        SAFE_FREE(merge_list);

        if (!(ret->solution_boxes)) {
          nxagentScreenCrtcsFreeSolution(ret);

          SAFE_FREE(ret);

          return(ret);
        }

        /* Update reference variables. */
        all_boxes_ref = ret->all_boxes;

        /* Should only have one box, so taking the first entry is fine. */
        box_ref = xorg_list_first_entry(&(ret->solution_boxes->head), nxagentScreenBoxesElem, entry);

        /* Update size change. */
        ret->rating_size_change += run_size_change;

        /* Update cover penalty. */
        ret->rating_cover_penalty += run_cover_penalty;

        /* One box was extended, record. */
        ret->rating_extended_boxes_count = 1;
      }
    }
    else {
      /*
       * An empty merge list means that we didn't find other mergable boxes.
       * Not an error, but rather an indication to stop the loop.
       *
       * Make sure to not clear out ret.
       */
      nxagentFreeScreenBoxes(merge_list, TRUE);

      SAFE_FREE(merge_list);

      cont = FALSE;
    }

    init = FALSE;
  }

  if ((ret) && (ret->all_boxes) && (ret->solution_boxes)) {
    /*
     * Calculate actual rating.
     */
    nxagentScreenCrtcsSolutionCalculateRating(ret, (top || bottom));
  }
  else {
    /* Invalid solution, clear out. */
    nxagentScreenCrtcsFreeSolution(ret);

    SAFE_FREE(ret);
  }

  return(ret);
}

/*
 * Helper that deep-copies a single solution.
 *
 * Returns a pointer to the copy or NULL on error.
 */
static nxagentScreenCrtcsSolution* nxagentScreenCrtcsSolutionCopy(const nxagentScreenCrtcsSolution *solution) {
  nxagentScreenCrtcsSolution *ret = NULL;

  if (!(solution)) {
    return(ret);
  }

  ret = calloc(1, sizeof(nxagentScreenCrtcsSolution));

  if (!(ret)) {
    return(ret);
  }

  xorg_list_init(&(ret->entry));

  ret->rating_size_change = solution->rating_size_change;
  ret->rating_cover_penalty = solution->rating_cover_penalty;
  ret->rating_extended_boxes_count = solution->rating_extended_boxes_count;
  ret->rating = solution->rating;

  if (solution->solution_boxes) {
    ret->solution_boxes = nxagentScreenBoxesCopy(solution->solution_boxes);

    if (!(ret->solution_boxes)) {
      SAFE_FREE(ret);

      return(ret);
    }
  }

  if (solution->all_boxes) {
    ret->all_boxes = nxagentScreenBoxesCopy(solution->all_boxes);

    if (!(ret->all_boxes)) {
      nxagentFreeScreenBoxes(ret->solution_boxes, TRUE);

      SAFE_FREE(ret->solution_boxes);

      SAFE_FREE(ret);

      return(ret);
    }
  }

  return(ret);
}

/* Helper function that deallocates a solutions list. */
static void nxagentScreenCrtcsFreeSolutions(nxagentScreenCrtcsSolutions *solutions) {
  if (!(solutions)) {
    return;
  }

  nxagentScreenCrtcsSolution *cur, *next = NULL;
  xorg_list_for_each_entry_safe(cur, next, solutions, entry) {
    nxagentScreenCrtcsFreeSolution(cur);

    SAFE_FREE(cur);
  }

  xorg_list_init(solutions);
}

/*
 * Helper that extracts the best solutions out of a solutions list.
 *
 * Returns a list of best solutions or NULL on error. Might be empty.
 */
static nxagentScreenCrtcsSolutions* nxagentScreenCrtcsExtractBestSolutions(const nxagentScreenCrtcsSolutions *solutions) {
  nxagentScreenCrtcsSolutions *ret = NULL;

  if (!(solutions)) {
    return(ret);
  }

  ret = calloc(1, sizeof(nxagentScreenCrtcsSolutions));

  if (!(ret)) {
    return(ret);
  }

  xorg_list_init(ret);

  /* Get best rating value. */
  double best_rating = INVALID_RATING;
  nxagentScreenCrtcsSolution *cur = NULL;
  xorg_list_for_each_entry(cur, solutions, entry) {
    if (cur->rating > best_rating) {
      best_rating = cur->rating;
    }
  }

  if (INVALID_RATING == best_rating) {
    /* No need to go through the list again, return empty list. */
    return(ret);
  }

  xorg_list_for_each_entry(cur, solutions, entry) {
    if (cur->rating == best_rating) {
      nxagentScreenCrtcsSolution *cur_copy = nxagentScreenCrtcsSolutionCopy(cur);

      if (!(cur_copy)) {
        nxagentScreenCrtcsFreeSolutions(ret);

        SAFE_FREE(ret);

        return(ret);
      }

      xorg_list_append(&(cur_copy->entry), ret);
    }
  }

  return(ret);
}

/*
 * Helper generating a list of solutions, extending one specific initial
 * screen box.
 *
 * Returns either a pointer to the solutions list or NULL on failure.
 */
static nxagentScreenCrtcsSolutions* nxagentScreenCrtcsGenerateSolutionsSingleScreen(const nxagentScreenBoxes *all_boxes, const nxagentScreenBoxesElem *box, const nxagentScreenBoxes *other_screens) {
  nxagentScreenCrtcsSolutions *ret = NULL,
                              *tmp_solutions = NULL;

  if ((!(all_boxes)) || (!(box)) || (!(other_screens))) {
    return(ret);
  }

  tmp_solutions = calloc(1, sizeof(nxagentScreenCrtcsSolutions));

  if (!(tmp_solutions)) {
    return(ret);
  }

  xorg_list_init(tmp_solutions);

  nxagentScreenCrtcsSolution *cur_solution = NULL;

  /* Left expansion. */
  cur_solution = nxagentScreenCrtcsGenerateSolutionsSingleStep(all_boxes, box, other_screens, TRUE,  FALSE, FALSE, FALSE);

  if (cur_solution) {
    xorg_list_append(&(cur_solution->entry), tmp_solutions);
  }

  /* Right expansion. */
  cur_solution = nxagentScreenCrtcsGenerateSolutionsSingleStep(all_boxes, box, other_screens, FALSE, TRUE,  FALSE, FALSE);

  if (cur_solution) {
    xorg_list_append(&(cur_solution->entry), tmp_solutions);
  }

  /* Top expansion. */
  cur_solution = nxagentScreenCrtcsGenerateSolutionsSingleStep(all_boxes, box, other_screens, FALSE, FALSE, TRUE,  FALSE);

  if (cur_solution) {
    xorg_list_append(&(cur_solution->entry), tmp_solutions);
  }

  /* Bottom expansion. */
  cur_solution = nxagentScreenCrtcsGenerateSolutionsSingleStep(all_boxes, box, other_screens, FALSE, FALSE, FALSE, TRUE);

  if (cur_solution) {
    xorg_list_append(&(cur_solution->entry), tmp_solutions);
  }

  ret = nxagentScreenCrtcsExtractBestSolutions(tmp_solutions);

  /*
   * Since the best solutions list is a deep copy, we can clear out the
   * all solutions list.
   */
  nxagentScreenCrtcsFreeSolutions(tmp_solutions);

  SAFE_FREE(tmp_solutions);

  return(ret);
}

/*
 * Helper calculating an obsolete boxes count, given a list of all boxes.
 *
 * Note that it might be zero if there are no obsolete boxes or an error
 * happened. If the error parameter is non-NULL, its value will be set to true
 * to indicate an error.
 */
#ifdef DEBUG
/*
 * When debugging, make sure the guards are not optimized away. Otherwise
 * providing NULL for the err parameter will crash when calling the function
 * directly in debuggers.
 */
__attribute__((optimize("O0")))
#endif
static size_t nxagentScreenBoxesObsoleteCount(const nxagentScreenBoxes *all_boxes, Bool *err) {
  size_t ret = 0;

  if (err) {
    *err = FALSE;
  }

  if (!(all_boxes)) {
    if (err) {
      *err = TRUE;
    }

    return(ret);
  }

  nxagentScreenBoxesElem *cur = NULL;
  xorg_list_for_each_entry(cur, &(all_boxes->head), entry) {
    if (cur->obsolete) {
      ++ret;
    }
  }

  return(ret);
}

/*
 * Helper that exchanges a screen box for a new one.
 * The original box is destroyed, the new box is deep-copied.
 *
 * Returns true on success, otherwise false.
 */
static Bool nxagentScreenBoxesUpdateScreenBox(nxagentScreenBoxes *boxes, const size_t screen_number, const nxagentScreenBoxesElem *new_box) {
  Bool ret = FALSE;

  if ((!(boxes)) || (!(new_box))) {
    return(ret);
  }

  nxagentScreenBoxesElem *cur_box = NULL;
  size_t i = 0;
  xorg_list_for_each_entry(cur_box, &(boxes->head), entry) {
    if (cur_box->screen_id == new_box->screen_id) {
      /* Need to exchange the current box. */
      nxagentScreenBoxesElem *new_copy = nxagentScreenBoxesElemCopy(new_box, TRUE);

      if (!(new_copy)) {
        return(ret);
      }

      /*
       * Taking out the magic wand here:
       * Since an xorg_list is cyclic and we know that the current element
       * exists, we can use an append operation to insert the new element and
       * then delete the old one.
       * Instead of appending to the original list head (in this case boxes),
       * we'll append to the current element. The actual magic is that a list
       * append operation is *actually* a prepend operation relative to the
       * passed head element.
       *
       * Example:
       *   Original list (each edge is actually a double edge pointing in both
       *   directions):
       *
       *   ┌──────────────────────────────────────────┐
       *   ↓                                          │
       * [HEAD] ↔ [elem1] ↔ [elem2] ↔ [elem3] ↔ ... ←─┘
       *
       * Append operation relative to HEAD:
       *
       *   ┌───────────────────────────────────────────────────────┐
       *   ↓                                                       │
       * [HEAD] ↔ [elem1] ↔ [elem2] ↔ [elem3] ↔ ... ↔ [new_elem] ←─┘
       *
       * Append operation relative to elem2:
       *
       *   ┌───────────────────────────────────────────────────────┐
       *   ↓                                                       │
       * [HEAD] ↔ [elem1] ↔ [new_elem] ↔ [elem2] ↔ [elem3] ↔ ... ←─┘
       *
       * Afterwards, delete operation on elem2:
       *
       *   ┌─────────────────────────────────────────────┐
       *   ↓                                             │
       * [HEAD] ↔ [elem1] ↔ [new_elem] ↔ [elem3] ↔ ... ←─┘
       *
       * Observant readers might have noticed that using either a list append
       * or add operation (i.e., relative to given head element an inverse
       * append or "real" append operation) followed by a delete operation for
       * the current element will lead to the same result.
       *
       * We'll use an append/inverse append operation, though, since this makes
       * the most sense.
       */
      xorg_list_append(&(new_copy->entry), &(cur_box->entry));
      xorg_list_del(&(cur_box->entry));

      /* Get rid of cur_box. */
      SAFE_FREE(cur_box);

      ret = TRUE;
      break;
    }

    i++;
  }

  /*
   * If ret is still false here it means that the list did not contain an
   * element at position pos (i.e., it was too small).
   *
   * No need for special treatment.
   */

  return(ret);
}

/*
 * Helper that generates an array with solution lists for each screen box and
 * direction.
 *
 * The array will always be screen_count-sized.
 *
 * Returns a pointer to the array. Will be NULL on failure.
 */
static nxagentScreenCrtcsSolutions** nxagentScreenCrtcsGeneratePotentialSolutionArray(const nxagentScreenBoxes *all_boxes, const nxagentScreenBoxes *screen_boxes, const size_t screen_count) {
  nxagentScreenCrtcsSolutions **ret = NULL;

  /* FIXME: xorg_list_is_empty is not const-correct. */
  if ((!(screen_count)) || (xorg_list_is_empty((struct xorg_list *)(&(screen_boxes->head)))) || (xorg_list_is_empty((struct xorg_list *)(&(all_boxes->head))))) {
    return(ret);
  }

  ret = calloc(screen_count, sizeof(nxagentScreenCrtcsSolutions*));

  if (!(ret)) {
    return(ret);
  }

  for (size_t i = 0; i < screen_count; ++i) {
    nxagentScreenBoxesElem *tmp_box = NULL;
    {
      size_t y = 0;
      xorg_list_for_each_entry(tmp_box, &(screen_boxes->head), entry) {
        /* y == i means that we found our current box and can break out. */
        if (y++ == i) {
          break;
        }
      }

      if ((&(tmp_box->entry)) == &(screen_boxes->head)) {
#ifdef WARNING
        fprintf(stderr, "%s: reached end of list while fetching specific box. Algorithm error.\n", __func__);
#endif

        for (size_t z = 0; z < screen_count; ++z) {
          nxagentScreenCrtcsFreeSolutions(ret[z]);

          SAFE_FREE(ret[z]);
        }

        SAFE_FREE(ret);

        return(ret);
      }
    }

    /* Build other_screens list. */
    nxagentScreenBoxes *other_screens = calloc(1, sizeof(nxagentScreenBoxes));

    if (!(other_screens)) {
#ifdef WARNING
      fprintf(stderr, "%s: unable to allocate space for other_screens list.\n", __func__);
#endif

      for (size_t y = 0; y < screen_count; ++y) {
        nxagentScreenCrtcsFreeSolutions(ret[y]);

        SAFE_FREE(ret[y]);
      }

      SAFE_FREE(ret);

      return(ret);
    }

    xorg_list_init(&(other_screens->head));
    other_screens->screen_id = -1;

    nxagentScreenBoxesElem *tmp = NULL;
    xorg_list_for_each_entry(tmp, &(screen_boxes->head), entry) {
      if (tmp != tmp_box) {
        /* Copy current element. */
        nxagentScreenBoxesElem *box_copy = nxagentScreenBoxesElemCopy(tmp, TRUE);

        if (!(box_copy)) {
#ifdef WARNING
          fprintf(stderr, "%s: unable to copy current screen box.\n", __func__);
#endif

          for (size_t y = 0; y < screen_count; ++y) {
            nxagentScreenCrtcsFreeSolutions(ret[y]);

            SAFE_FREE(ret[y]);
          }

          nxagentFreeScreenBoxes(other_screens, TRUE);

          SAFE_FREE(other_screens);

          SAFE_FREE(ret);

          return(ret);
        }

        /* Add to other_screens list. */
        xorg_list_append(&(box_copy->entry), &(other_screens->head));
      }
    }

    /*
     * Right now, all_boxes contains all boxes, including obsolete
     * information, tmp_box is the current screen box to extend and
     * other_screens contains the other screen boxes.
     *
     * With that, we can fetch a solutions list comprising of the best(!)
     * solutions for extending the current box in all directions.
     */
    nxagentScreenCrtcsSolutions *cur_screen_solutions = nxagentScreenCrtcsGenerateSolutionsSingleScreen(all_boxes, tmp_box, other_screens);

    /*
     * Clean up other_screens. Doing that now means we don't have to do it in
     * the error handling.
     */
    nxagentFreeScreenBoxes(other_screens, TRUE);

    SAFE_FREE(other_screens);

    /* NULL means failure or no solutions. */
    if (!(cur_screen_solutions)) {
#ifdef WARNING
      fprintf(stderr, "%s: no solution found for current configuration. Algorithm error.\n", __func__);
#endif

      for (size_t y = 0; y < screen_count; ++y) {
        nxagentScreenCrtcsFreeSolutions(ret[y]);

        SAFE_FREE(ret[y]);
      }

      SAFE_FREE(ret);

      return(ret);
    }

    /*
     * If everything worked out, we'll have a solutions list.
     * It might contain multiple entries, but at this point we don't care,
     * since they may not have the highest overall rating.
     * Just add them to our general solutions list.
     *
     * We're just setting plain pointer values here, which should work fine
     * since the next and prev pointers point to the addresses of an
     * element's entry xorg_list struct.
     *
     * Be careful, though.
     */
    ret[i] = cur_screen_solutions;
  }

  return(ret);
}

/*
 * Helper that extracts the best solutions from a screen_count-sized solutions
 * array and, at the same time, records if a screen is suitable for extension.
 *
 * The arrays will always likewise be screen_count-sized.
 *
 * The last two parameters are output parameters that expect to be passed a
 * valid address. NULL pointers for any parameter or a zero screen count will be
 * treated as an error. init might be zero (i.e., false).
 *
 * Returns true on success, otherwise false.
 */
static Bool nxagentScreenCrtcsFilterScreenSolutions(const nxagentScreenCrtcsSolutions * const *solutions, const size_t screen_count, const Bool init, const Bool *screens_init, nxagentScreenCrtcsSolutions ***best_screen_solutions, Bool **screen_selection) {
  Bool ret = FALSE;

  if ((!(solutions)) || (!(screen_count)) || (!(screens_init)) || (!(best_screen_solutions)) || (!(screen_selection))) {
    return(ret);
  }

  (*screen_selection) = NULL;
  (*best_screen_solutions) = NULL;

  double *screen_ratings = calloc(screen_count, sizeof(double));

  if (!(screen_ratings)) {
    return(ret);
  }

  Bool *screen_overlap_free = calloc(screen_count, sizeof(Bool));

  if (!(screen_overlap_free)) {
    SAFE_FREE(screen_ratings);

    return(ret);
  }

  (*screen_selection) = calloc(screen_count, sizeof(Bool));
  (*best_screen_solutions) = calloc(screen_count, sizeof(nxagentScreenCrtcsSolutions*));

  if ((!((*screen_selection))) || (!((*best_screen_solutions)))) {
    SAFE_FREE(screen_ratings);
    SAFE_FREE(screen_overlap_free);

    /* Let caller handle other cleanup. It'll be done there anyway. */
    return(ret);
  }

  /*
   * We consider two different cases when filtering solutions:
   *   - the initial case, used as long as we have non-extended (i.e.,
   *     initial) screen boxes and
   *   - the normal case.
   *
   * The initial case is slightly different, since we generally need to
   * consider every best solution per initial box.
   * This ensures initial expansion (together with the rating function
   * favoring initial expansion).
   *
   * In both cases, we still have to consider all screen boxes (i.e., also
   * the already extended ones in the initial case), since we want to fish
   * out solutions with no additional overlap.
   *
   * Viable solutions will be selected with this preference (n.b.: as the
   * name implies, all solutions we can select from are the best for their
   * respective screen):
   *   - if we have solutions with no additional overlap, take the best
   *     one(s)
   *   otherwise
   *   - if there are still screens to extend, take the best one(s) for an
   *     unextended screen
   *   otherwise
   *   - take the overall best one(s).
   *
   * Mixing all solutions into one list and extracting the best solutions is
   * not feasible in this algorithmic branch siince we wouldn't be able to
   * map the solution back to an initial screen.
   *
   * Instead, we'll mark screens as selected and pull out solutions manually.
   */

  /* Update the screen_overlap_free and screen_ratings arrays. */
  Bool overlap_free = FALSE;
  double max_rating = INVALID_RATING,
         max_overlap_free_rating = INVALID_RATING;
  for (size_t i = 0; i < screen_count; ++i) {
    screen_ratings[i] = INVALID_RATING;
    screen_overlap_free[i] = FALSE;

    /* Extract best solutions first. */
    (*best_screen_solutions)[i] = nxagentScreenCrtcsExtractBestSolutions(solutions[i]);

    if (!((*best_screen_solutions)[i])) {
      SAFE_FREE(screen_ratings);
      SAFE_FREE(screen_overlap_free);

      /* Let caller handle other cleanup. It'll be done there anyway. */
      return(ret);
    }

    /* Skip this part if we have no solutions in the list. */
    if (!(xorg_list_is_empty((*best_screen_solutions)[i]))) {
      nxagentScreenCrtcsSolution *cur = xorg_list_first_entry((*best_screen_solutions)[i], nxagentScreenCrtcsSolution, entry);

      /*
       * Rating must be the same for all entries in a list as returned by
       * nxagentScreenCrtcsExtractBestSolutions().
       */
      screen_ratings[i] = cur->rating;

      /*
       * Covers might be different per-solution, so go through the full list.
       */
      cur = NULL;
      xorg_list_for_each_entry(cur, (*best_screen_solutions)[i], entry) {
        if (!(cur->rating_cover_penalty)) {
          screen_overlap_free[i] = TRUE;

          if (screen_ratings[i] > max_overlap_free_rating) {
            max_overlap_free_rating = screen_ratings[i];
          }

          /* First non-overlapping solution is all we care about. */
          break;
        }
      }
    }

    overlap_free |= screen_overlap_free[i];

    /*
     * max_rating is useful in the case we have no overlap-free solutions.
     *
     * We care for all screens after initialization or for non-extended screens
     * prior to initialization.
     */
    if ((init) || (!(screens_init[i]))) {
      if (screen_ratings[i] > max_rating) {
        max_rating = screen_ratings[i];
      }
    }
  }

  if (((overlap_free) && (INVALID_RATING == max_overlap_free_rating)) || ((!(overlap_free)) && (INVALID_RATING == max_rating))) {
#ifdef WARNING
    fprintf(stderr, "%s: no solution found for current configuration in %sscreen extension run%s. Algorithm error.\n", __func__, init ? "initial " : "", init ? ", but not all initial screen boxes have been extended yet" : "");
#endif

    SAFE_FREE(screen_ratings);
    SAFE_FREE(screen_overlap_free);

    /* Let caller handle other cleanup. It'll be done there anyway. */
    return(ret);
  }

  if (overlap_free) {
    /*
     * We know that there is at least one overlap-free solution. The real
     * question left is which screens have the highest-rating solutions.
     *
     * Go through the list and compare with the best rating.
     *
     * FIXME: do we want some more magic here to prefer non-extended screens
     *        during the initial extension run, even if that would mean picking
     *        worse (i.e., non-best) solutions?
     */
    for (size_t i = 0; i < screen_count; ++i) {
      if ((screen_overlap_free[i]) && (screen_ratings[i] == max_overlap_free_rating)) {
        /*
         * This screen has a maximum overlap-free rating. Since it may also
         * contain non-overlap-free solutions, filter them out to make later
         * handling easier.
         */
        nxagentScreenCrtcsSolution *cur = NULL,
                                   *tmp = NULL;
        xorg_list_for_each_entry_safe(cur, tmp, (*best_screen_solutions)[i], entry) {
          if (cur->rating_cover_penalty) {
            xorg_list_del(&(cur->entry));

            nxagentScreenCrtcsFreeSolution(cur);

            SAFE_FREE(cur);
          }
        }

        /* Mark this screen as suitable for further processing. */
        (*screen_selection)[i] = TRUE;
      }
    }
  }
  else {
    /*
     * No non-overlapping solutions found, so either select the best-rated
     * non-extended screen(s) or generally the best-rated screen(s).
     *
     * Luckily we've already got the best rating value as max_rating, so we'll
     * just need to go through the list again and mark the affected solutions
     * as selected.
     */
    for (size_t i = 0; i < screen_count; ++i) {
      if (screen_ratings[i] == max_rating) {
        (*screen_selection)[i] = TRUE;
      }
    }
  }

  ret = TRUE;

  SAFE_FREE(screen_ratings);
  SAFE_FREE(screen_overlap_free);

  return(ret);
}

/*
 * Helper setting up its my_solution output parameter according to the first
 * solution in the best_screen_solutions list.
 *
 * No pointer parameters may be NULL. screen_number is allowed to be zero
 * (indicating the first screen).
 *
 * Returns true on success, otherwise false.
 */
static Bool nxagentScreenCrtcsSelectSolution(const nxagentScreenBoxes *screen_boxes, const size_t screen_number, const Bool *screens_init, const nxagentScreenCrtcsSolutions *best_screen_solutions, nxagentScreenCrtcsSolution **my_solution) {
  Bool ret = FALSE;

  if ((!(screen_boxes)) || (!(screens_init)) || (!(best_screen_solutions)) || (!(my_solution))) {
    return(ret);
  }

  /*
   * Always take the first entry for the current run.
   */
  nxagentScreenCrtcsSolution *first_entry = xorg_list_first_entry(best_screen_solutions, nxagentScreenCrtcsSolution, entry);

  /*
   * Assert that first_entry is always a legit one - since we checked
   * the amount of solutions before, that should be safe.
   * This is why we don't check the return value here.
   */

  nxagentScreenCrtcsSolution *tmp_solution = nxagentScreenCrtcsSolutionCopy(first_entry);

  if (!(tmp_solution)) {
#ifdef WARNING
    fprintf(stderr, "%s: unable to copy generated solution.\n", __func__);
#endif

    return(ret);
  }

  if (!(*my_solution)) {
    /* If we don't have a solution yet, use this one. */
    (*my_solution) = tmp_solution;
  }
  else {
    /* Otherwise, modify my_solution. */
    (*my_solution)->rating_size_change += tmp_solution->rating_size_change;
    (*my_solution)->rating_cover_penalty += tmp_solution->rating_cover_penalty;

    if (!(screens_init[screen_number])) {
      (*my_solution)->rating_extended_boxes_count += tmp_solution->rating_extended_boxes_count; /* Should always be +1. */
    }

    (*my_solution)->rating += tmp_solution->rating;

    /* Plainly take the all_boxes pointer. */
    nxagentFreeScreenBoxes((*my_solution)->all_boxes, TRUE);
    SAFE_FREE((*my_solution)->all_boxes);
    (*my_solution)->all_boxes = nxagentScreenBoxesCopy(tmp_solution->all_boxes);

    if (!((*my_solution)->all_boxes)) {
#ifdef WARNING
      fprintf(stderr, "%s: unable to copy current all boxes state.\n", __func__);
#endif

      return(ret);
    }
  }

  /*
   * The solution boxes list handling is more complicated.
   * Our new, temporary solution only has the extended screen box
   * in its solution list - we will want to merge that into our
   * solutions list, replacing the original one in there (if it
   * exists).
   */

  /* Take a copy of the original solutions boxes pointer. */
  nxagentScreenBoxes *orig_solution_boxes = (*my_solution)->solution_boxes;

  /* Copy work_screens to the solution boxes of my_solution. */
  (*my_solution)->solution_boxes = nxagentScreenBoxesCopy(screen_boxes);

  if (!((*my_solution)->solution_boxes)) {
#ifdef WARNING
    fprintf(stderr, "%s: unable to copy current screen state.\n", __func__);
#endif

    return(ret);
  }

  if ((!(orig_solution_boxes)) || (xorg_list_is_empty(&(orig_solution_boxes->head)))) {
#ifdef WARNING
    fprintf(stderr, "%s: original solution boxes list invalid or empty.\n", __func__);
#endif

    nxagentFreeScreenBoxes(orig_solution_boxes, TRUE);

    SAFE_FREE(orig_solution_boxes);

    return(ret);
  }

  /*
   * Fetch actual screen box. The solutions list should only
   * contain one element, so breaking out directly should be
   * safe.
   */
  nxagentScreenBoxesElem *cur_box = xorg_list_first_entry(&(first_entry->solution_boxes->head), nxagentScreenBoxesElem, entry);

  const Bool update = nxagentScreenBoxesUpdateScreenBox((*my_solution)->solution_boxes, screen_number, cur_box);

  /*
   * Outside of error handling, since we need to get rid of this
   * data unconditionally.
   */
  nxagentFreeScreenBoxes(orig_solution_boxes, TRUE);

  SAFE_FREE(orig_solution_boxes);

  if (!(update)) {
#ifdef WARNING
    {
      const unsigned long long screen_number_ = screen_number;
      fprintf(stderr, "%s: unable to update solution screen number %llu.\n", __func__, screen_number_);
    }
#endif

    return(ret);
  }

  /* Delete taken solution out of the list. */
  xorg_list_del(&(first_entry->entry));

  /* Get rid of the entry. */
  nxagentScreenCrtcsFreeSolution(first_entry);

  SAFE_FREE(first_entry);

  ret = TRUE;

  return(ret);
}

/*
 * Declaration needed since the next function is using one that is only defined
 * at a later point.
 *
 * Moving it up would be problematic since in that case we'd need even more
 * declarations for other functions.
 */
static nxagentScreenCrtcsSolutions* nxagentScreenCrtcsGenerateSolutions(const nxagentScreenBoxes *all_boxes, const nxagentScreenBoxes *initial_screens, const size_t all_boxes_count, const size_t screen_count, const Bool *orig_screens_init);

/*
 * Helper handling all the solutions in best_screen_solutions recursively,
 * adding them to the ret_solutions output parameter list.
 *
 * No pointer parameters may be NULL. screen_count and screen_number might be
 * zero.
 *
 * Returns true on success, otherwise false.
 */
static Bool nxagentScreenCrtcsRecurseSolutions(const nxagentScreenBoxes *screen_boxes, const size_t screen_count, const size_t all_boxes_count, const Bool *screens_init, const size_t screen_number, const nxagentScreenCrtcsSolutions *best_screen_solutions, nxagentScreenCrtcsSolutions *ret_solutions) {
  Bool ret = FALSE;

  if ((!(screen_boxes)) || (!(screen_count)) || (!(all_boxes_count)) || (!(screens_init)) || (!(best_screen_solutions)) || (!(ret_solutions))) {
    return(ret);
  }

  nxagentScreenCrtcsSolution *cur_solution = NULL;
  xorg_list_for_each_entry(cur_solution, best_screen_solutions, entry) {
    /* Other solutions will be handled recursively. */

    /* Copy screens_init and set current screen value to true. */
    Bool *recursive_screens_init = calloc(screen_count, sizeof(Bool));

    if (!(recursive_screens_init)) {
#ifdef WARNING
      fprintf(stderr, "%s: unable to copy screen initialization array.\n", __func__);
#endif

      return(ret);
    }

    memmove(recursive_screens_init, screens_init, (screen_count * sizeof(Bool)));

    recursive_screens_init[screen_number] = TRUE;

    nxagentScreenBoxes *recursive_work_screens = nxagentScreenBoxesCopy(screen_boxes);

    if (!(recursive_work_screens)) {
#ifdef WARNING
      fprintf(stderr, "%s: unable to copy current screen state.\n", __func__);
#endif

      SAFE_FREE(recursive_screens_init);

      return(ret);
    }

    if ((!(cur_solution->solution_boxes)) || (xorg_list_is_empty(&(cur_solution->solution_boxes->head)))) {
#ifdef WARNING
      fprintf(stderr, "%s: current solution boxes list is empty or invalid. Algorithm error.\n", __func__);
#endif

      nxagentFreeScreenBoxes(recursive_work_screens, TRUE);

      SAFE_FREE(recursive_work_screens);

      SAFE_FREE(recursive_screens_init);

      return(ret);
    }

    nxagentScreenBoxesElem *cur_box = xorg_list_first_entry(&(cur_solution->solution_boxes->head), nxagentScreenBoxesElem, entry);

    const Bool update = nxagentScreenBoxesUpdateScreenBox(recursive_work_screens, screen_number, cur_box);

    if (!(update)) {
#ifdef WARNING
      fprintf(stderr, "%s: unable to update screen state.\n", __func__);
#endif

      nxagentFreeScreenBoxes(recursive_work_screens, TRUE);

      SAFE_FREE(recursive_screens_init);

      SAFE_FREE(recursive_work_screens);

      return(ret);
    }

    nxagentScreenCrtcsSolutions *tmp_solutions = nxagentScreenCrtcsGenerateSolutions(cur_solution->all_boxes, recursive_work_screens, all_boxes_count, screen_count, recursive_screens_init);

    /* Get rid of the temporary screens init array again. */
    SAFE_FREE(recursive_screens_init);

    /* Get rid of the modified work screens list. */
    nxagentFreeScreenBoxes(recursive_work_screens, TRUE);

    SAFE_FREE(recursive_work_screens);

    if (!(tmp_solutions)) {
#ifdef WARNING
      fprintf(stderr, "%s: unable to generate a new solutions list. Algorithm error.\n", __func__);
#endif

      return(ret);
    }

    /*
     * tmp_solutions should now contain a list of possible solutions,
     * add to ret_solutions.
     */
    nxagentScreenCrtcsSolution *cur_solution_it = NULL,
                               *next_solution = NULL;
    xorg_list_for_each_entry_safe(cur_solution_it, next_solution, tmp_solutions, entry) {
      xorg_list_del(&(cur_solution_it->entry));
      xorg_list_append(&(cur_solution_it->entry), ret_solutions);
    }

    /* tmp_solutions should be empty now, safe to free. */
    SAFE_FREE(tmp_solutions);
  }

  ret = TRUE;

  return(ret);
}

/*
 * Helper for handling solution lists. This probably is the heart of the screen
 * extension code. The function is called once per extension run and calls
 * other functions to save the very first solution and recursively generate
 * alternative solutions.
 *
 * No pointer parameters might be NULL. init might be zero (i.e., false).
 *
 * Returns true on success, otherwise false.
 */
static Bool nxagentScreenCrtcsHandleSolutions(const nxagentScreenBoxes *all_boxes, const nxagentScreenBoxes *screen_boxes, const Bool init, const Bool *screens_init, const size_t screen_count, const size_t all_boxes_count, const Bool *screen_selection, nxagentScreenCrtcsSolutions *ret_solutions, ssize_t *screen_to_init, nxagentScreenCrtcsSolutions **best_screen_solutions, nxagentScreenCrtcsSolution **my_solution) {
  Bool ret = FALSE;

  if ((!(all_boxes)) || (!(screen_boxes)) || (!(screens_init)) || (!(screen_count)) || (!(all_boxes_count)) || (!(screen_selection)) || (!(ret_solutions)) || (!(screen_to_init)) || (!(best_screen_solutions)) || (!(my_solution))) {
    return(ret);
  }

  /*
   * In case we have multiple solutions with a maximum rating, we need to
   * consider each solution, which means branching off for all but one
   * solution and only handling one solution in this run.
   * Selecting the solution for the current run is tricky, though. We
   * could either take the very first one, which is relatively easy, the
   * last one, which is complicated because there might be multiple
   * screens with a maximum rating and finding the last one is tricky
   * with a spread-out array. Merging all solutions into one list and then
   * taking the last element would be easy to do, but has the negative
   * consequence of not being able to tell what screen the individual
   * solutions belonged to originally - at least not without
   * "sophisticated" means like keeping the original list and deep-checking
   * objects for equality or creating another structure.
   * Selecting a more or less random solution at the end of the first
   * screen would work, but feels weird if there are more screens with
   * potential solutions.
   *
   * Hence, let's go for selecting the very first solution.
   */
  Bool fetched_solution = FALSE;
  (*screen_to_init) = -1;
  for (size_t i = 0; i < screen_count; ++i) {
    if (screen_selection[i]) {
      /*
       * This screen has been selected.
       * Its solution list may include more than one solution, though,
       * which means that we have to branch off and consider each
       * individual solution.
       * At the very end, we select (potentially one of) the overall best
       * solution.
       */

      if ((!(best_screen_solutions[i])) || (xorg_list_is_empty(best_screen_solutions[i]))) {
#ifdef WARNING
        fprintf(stderr, "%s: current screen marked with a maximum rating, but no solutions found in screen extension run. Algorithm error.\n", __func__);
#endif

        return(ret);
      }

      /*
       * One or more solution(s), if necessary take the first one as the
       * current solution and then branch off for the others.
       */
      if (!(fetched_solution)) {
        Bool fetch = nxagentScreenCrtcsSelectSolution(screen_boxes, i, screens_init, best_screen_solutions[i], my_solution);

        if (!(fetch)) {
#ifdef WARNING
          fprintf(stderr, "%s: error while selecting solution for current run.\n", __func__);
#endif

          return(ret);
        }

        fetched_solution = TRUE;

        /*
         * DO NOT modify other data (screen, all boxes or screen initialization
         * array) here!
         * We will need to change these variables eventually, but given
         * that we may have further solutions to process/generate, doing
         * it here would be an error.
         * Refer to the later part of nxagentScreenCrtcsGenerateSolutions for
         * this.
         */
        if (init) {
          (*screen_to_init) = i;
        }
      }

      Bool recursive_solutions = nxagentScreenCrtcsRecurseSolutions(screen_boxes, screen_count, all_boxes_count, screens_init, i, best_screen_solutions[i], ret_solutions);

      if (!(recursive_solutions)) {
#ifdef WARNING
        fprintf(stderr, "%s: error while handling other solutions recursively in current run.\n", __func__);
#endif

        return(ret);
      }
    }
  }

  ret = TRUE;

  return(ret);
}

/*
 * Helper updating internal data in nxagentScreenCrtcsGenerateSolutions().
 * This mostly exists to avoid complicated data freeing while updating the
 * internal data.
 *
 * No pointers might be NULL. screens_to_init is allowed to be zero or
 * negative, although negative values will not lead to changed data. This is
 * not considered an error.
 *
 * Returns true on success, otherwise false.
 */
static Bool nxagentScreenCrtcsGenerateSolutionsUpdateInternalData(const nxagentScreenBoxes *all_boxes, const nxagentScreenBoxes *screen_boxes, const ssize_t screen_to_init, nxagentScreenBoxes **work_all_boxes, nxagentScreenBoxes **work_screens, Bool *screens_init) {
  Bool ret = FALSE;

  if ((!(all_boxes)) || (!(screen_boxes)) || (!(work_all_boxes)) || (!(work_screens)) || (!(screens_init))) {
    return(ret);
  }

  nxagentFreeScreenBoxes((*work_all_boxes), TRUE);

  SAFE_FREE((*work_all_boxes));

  nxagentFreeScreenBoxes((*work_screens), TRUE);

  SAFE_FREE((*work_screens));

  (*work_all_boxes) = nxagentScreenBoxesCopy(all_boxes);

  if (!((*work_all_boxes))) {
#ifdef WARNING
    fprintf(stderr, "%s: unable to copy current screen boxes.\n", __func__);
#endif

    return(ret);
  }

  (*work_screens) = nxagentScreenBoxesCopy(screen_boxes);

  if (!((*work_screens))) {
#ifdef WARNING
    fprintf(stderr, "%s: unable to copy current screen boxes.\n", __func__);
#endif

    nxagentFreeScreenBoxes((*work_all_boxes), TRUE);

    SAFE_FREE((*work_all_boxes));

    return(ret);
  }

  /*
   * Mark the current screen as initialized.
   * DO NOT move this to the other functions, since we might have
   * multiple screens with a maximum rating, but will not extend
   * the other screens in the current run (but rather in recursive
   * calls).
   */
  if (0 <= screen_to_init) {
#ifdef WARNING
    if (screens_init[screen_to_init]) {
      const unsigned long long screen_number = screen_to_init;
      fprintf(stderr, "%s: shall set screen init for screen number %llu to TRUE, but already marked as initialized. Algorithm warning.\n", __func__, screen_number);
    }
#endif

    screens_init[screen_to_init] = TRUE;
  }

  ret = TRUE;

  return(ret);
}

/*
 * Helper generating a "fake" solution based on the passed-in data.
 *
 * This is useful if no screens needed extension.
 *
 * No pointer parameters might be NULL.
 *
 * On success, returns a "fake" solution, otherwise NULL.
 */
static nxagentScreenCrtcsSolution* nxagentScreenCrtcsGenerateFakeSolution(const nxagentScreenBoxes *all_boxes, const nxagentScreenBoxes *screen_boxes) {
  nxagentScreenCrtcsSolution *ret = NULL;

  if ((!(all_boxes)) || (!(screen_boxes))) {
    return(ret);
  }

  ret = calloc(1, sizeof(nxagentScreenCrtcsSolution));

  if (!(ret)) {
    return(ret);
  }

  xorg_list_init(&(ret->entry));

  ret->all_boxes = nxagentScreenBoxesCopy(all_boxes);

  if (!(ret->all_boxes)) {
    nxagentScreenCrtcsFreeSolution(ret);

    SAFE_FREE(ret);

    return(ret);
  }

  ret->solution_boxes = nxagentScreenBoxesCopy(screen_boxes);

  if (!(ret->solution_boxes)) {
    nxagentScreenCrtcsFreeSolution(ret);

    SAFE_FREE(ret);

    return(ret);
  }

  ret->rating_size_change = ret->rating_cover_penalty = ret->rating_extended_boxes_count = 0;
  ret->rating = 0.0;

  nxagentScreenCrtcsSolutionCalculateRating(ret, FALSE);

  return(ret);
}

/*
 * Helper generating a list of solutions, extending the initial screen boxes.
 *
 * All pointer arguments but orig_screens_init must be non-NULL. All size
 * parameters must be non-zero.
 *
 * Returns either a pointer to the solutions list or NULL on failure.
 */
static nxagentScreenCrtcsSolutions* nxagentScreenCrtcsGenerateSolutions(const nxagentScreenBoxes *all_boxes, const nxagentScreenBoxes *initial_screens, const size_t all_boxes_count, const size_t screen_count, const Bool *orig_screens_init) {
  nxagentScreenCrtcsSolutions *ret = NULL;

  /*
   * We assume that the screen and all boxes count as passed in match the
   * actual data.
   *
   * We also assume that there is at least one screen. Otherwise, generating a
   * fake one here and running an expensive algorithm on this which trivially
   * will cover all base boxes anyway doesn't make a lot of sense.
   * Theoretically, such a situation could occur if moving the nxagent window
   * completely out of any screen bounds. This could potentially also happen if
   * the window is initialized on a screen, which is later disconnected.
   * Normally X11 window managers should take care of this situation and move
   * the window to a connected screen again, but that doesn't happen on Windows
   * for instance. This makes such windows inaccessible and would lead to an
   * empty initial screens list.
   */
  if ((!(all_boxes)) || (!(initial_screens)) || (!(all_boxes_count)) || (!(screen_count))) {
    return(ret);
  }

  /* Check that initial_screens and all_boxes are not empty. */
  /* FIXME: xorg_list_is_empty is not const-correct. */
  if ((xorg_list_is_empty((struct xorg_list *)(&(initial_screens->head)))) || (xorg_list_is_empty((struct xorg_list *)(&(all_boxes->head))))) {
#ifdef WARNING
    fprintf(stderr, "%s: initial_screens or all_boxes empty, assuming error and returning NULL.\n", __func__);
#endif

    return(ret);
  }

  Bool err = FALSE;
  size_t obsolete_boxes_count = nxagentScreenBoxesObsoleteCount(all_boxes, &err);

  if (err) {
    return(ret);
  }

#ifdef DEBUG
  {
    const unsigned long long obsolete_boxes_count_ = obsolete_boxes_count;
    fprintf(stderr, "%s: calculated initial obsolete boxes count: %llu\n", __func__, obsolete_boxes_count_);
  }
#endif

  /*
   * orig_screens_init as passed-in to the function (if non-NULL) will serve as
   * the base initialization of the array.
   * Each function execution is reponsible for freeing the memory at the end -
   * not callees.
   */
  Bool *screens_init = calloc(screen_count, sizeof(Bool));

  if (!(screens_init)) {
    return(ret);
  }

  if (orig_screens_init) {
    memmove(screens_init, orig_screens_init, (screen_count * sizeof(*screens_init)));
  }

  /*
   * Let work_screens and work_all_boxes point to initial_screens and all_boxes
   * respectively.
   */
  nxagentScreenBoxes *work_screens = nxagentScreenBoxesCopy(initial_screens);

  if (!(work_screens)) {
    SAFE_FREE(screens_init);

    return(ret);
  }

  nxagentScreenBoxes *work_all_boxes = nxagentScreenBoxesCopy(all_boxes);

  if (!(work_all_boxes)) {
    nxagentFreeScreenBoxes(work_screens, TRUE);

    SAFE_FREE(work_screens);

    SAFE_FREE(screens_init);

    return(ret);
  }

  ret = calloc(1, sizeof(nxagentScreenCrtcsSolutions));

  if (!(ret)) {
    nxagentFreeScreenBoxes(work_screens, TRUE);
    nxagentFreeScreenBoxes(work_all_boxes, TRUE);

    SAFE_FREE(work_screens);
    SAFE_FREE(work_all_boxes);

    SAFE_FREE(screens_init);

    return(ret);
  }

  xorg_list_init(ret);

  Bool init = TRUE;
  nxagentScreenCrtcsSolution *my_solution = NULL;
  while (obsolete_boxes_count < all_boxes_count) {
    ssize_t screen_to_init = -1;

    nxagentScreenCrtcsSolutions **extended_screens = nxagentScreenCrtcsGeneratePotentialSolutionArray(work_all_boxes, work_screens, screen_count);

    if (!(extended_screens)) {
      nxagentScreenCrtcsFreeSolutions(ret);

      nxagentScreenCrtcsFreeSolution(my_solution);

      nxagentFreeScreenBoxes(work_screens, TRUE);
      nxagentFreeScreenBoxes(work_all_boxes, TRUE);

      SAFE_FREE(ret);

      SAFE_FREE(my_solution);

      SAFE_FREE(work_screens);
      SAFE_FREE(work_all_boxes);

      SAFE_FREE(screens_init);

      return(ret);
    }

    init = FALSE;

    /* If one screen wasn't extended yet, init should be true. Sync state. */
    for (size_t i = 0; i < screen_count; ++i) {
      init |= (!(screens_init[i]));
    }

    nxagentScreenCrtcsSolutions **best_screen_solutions = NULL;
    Bool *screen_selection = NULL;

    /*
     * Could work without an explicit cast, but C doesn't implement a more
     * complicated implicit cast rule while C++ does.
     */
    Bool filter = nxagentScreenCrtcsFilterScreenSolutions((const nxagentScreenCrtcsSolutions * const *)(extended_screens), screen_count, init, screens_init, &best_screen_solutions, &screen_selection);

    /*
     * Clean up extended_screens. We don't need it any longer.
     * Do this before error handling, since it will need to be free'd in any
     * case.
     */
    for (size_t i = 0; i < screen_count; ++i) {
      nxagentScreenCrtcsFreeSolutions(extended_screens[i]);

      SAFE_FREE(extended_screens[i]);
    }

    SAFE_FREE(extended_screens);

    if (!(filter)) {
      for (size_t i = 0; i < screen_count; ++i) {
        nxagentScreenCrtcsFreeSolutions(best_screen_solutions[i]);

        SAFE_FREE(best_screen_solutions[i]);
      }

      nxagentScreenCrtcsFreeSolutions(ret);

      nxagentScreenCrtcsFreeSolution(my_solution);

      nxagentFreeScreenBoxes(work_screens, TRUE);
      nxagentFreeScreenBoxes(work_all_boxes, TRUE);

      SAFE_FREE(best_screen_solutions);

      SAFE_FREE(ret);

      SAFE_FREE(my_solution);

      SAFE_FREE(work_screens);
      SAFE_FREE(work_all_boxes);

      SAFE_FREE(screens_init);
      SAFE_FREE(screen_selection);

      return(ret);
    }

    Bool solution_handling = nxagentScreenCrtcsHandleSolutions(work_all_boxes, work_screens, init, screens_init, screen_count, all_boxes_count, screen_selection, ret, &screen_to_init, best_screen_solutions, &my_solution);

    /* Unconditionally get rid of best_screen_solutions. */
    for (size_t i = 0; i < screen_count; ++i) {
      nxagentScreenCrtcsFreeSolutions(best_screen_solutions[i]);

      SAFE_FREE(best_screen_solutions[i]);
    }

    SAFE_FREE(best_screen_solutions);

    /* And screen_selection. */
    SAFE_FREE(screen_selection);

    if (!(solution_handling)) {
#ifdef WARNING
      fprintf(stderr, "%s: unable to handle screen boxes in current run.\n", __func__);
#endif

      nxagentScreenCrtcsFreeSolutions(ret);

      nxagentScreenCrtcsFreeSolution(my_solution);

      nxagentFreeScreenBoxes(work_screens, TRUE);
      nxagentFreeScreenBoxes(work_all_boxes, TRUE);

      SAFE_FREE(ret);

      SAFE_FREE(my_solution);

      SAFE_FREE(work_screens);
      SAFE_FREE(work_all_boxes);

      SAFE_FREE(screens_init);

      return(ret);
    }

    /*
     * This is actually the right place to change these variables. For more
     * information, refer to comments in the other functions.
     */
    Bool update_data = nxagentScreenCrtcsGenerateSolutionsUpdateInternalData(my_solution->all_boxes, my_solution->solution_boxes, screen_to_init, &work_all_boxes, &work_screens, screens_init);

    if (!(update_data)) {
#ifdef WARNING
      fprintf(stderr, "%s: unable to update internal data in current run.\n", __func__);
#endif

      nxagentScreenCrtcsFreeSolutions(ret);

      nxagentScreenCrtcsFreeSolution(my_solution);

      nxagentFreeScreenBoxes(work_screens, TRUE);
      nxagentFreeScreenBoxes(work_all_boxes, TRUE);

      SAFE_FREE(ret);

      SAFE_FREE(my_solution);

      SAFE_FREE(work_screens);
      SAFE_FREE(work_all_boxes);

      SAFE_FREE(screens_init);

      return(ret);
    }

    obsolete_boxes_count = nxagentScreenBoxesObsoleteCount(work_all_boxes, &err);

    if (err) {
#ifdef WARNING
      fprintf(stderr, "%s: unable to update obsolete base boxes.\n", __func__);
#endif

      nxagentScreenCrtcsFreeSolutions(ret);

      nxagentScreenCrtcsFreeSolution(my_solution);

      nxagentFreeScreenBoxes(work_screens, TRUE);
      nxagentFreeScreenBoxes(work_all_boxes, TRUE);

      SAFE_FREE(ret);

      SAFE_FREE(my_solution);

      SAFE_FREE(work_screens);
      SAFE_FREE(work_all_boxes);

      SAFE_FREE(screens_init);

      return(ret);
    }

#ifdef DEBUG
    {
      const unsigned long long obsolete_boxes_count_ = obsolete_boxes_count;
      fprintf(stderr, "%s: recalculated obsolete boxes count: %llu\n", __func__, obsolete_boxes_count_);
    }
#endif
  }

  /* Unconditional cleanup. */
  SAFE_FREE(screens_init);

  /*
   * Having no solution means that we didn't have to generate one, i.e., that
   * the original screen boxes were all extended in the first place.
   *
   * In such a case, copy the input data and recalculate the rating with size
   * changes set to zero.
   */
  if (!(my_solution)) {
    my_solution = nxagentScreenCrtcsGenerateFakeSolution(work_all_boxes, work_screens);
  }

  /* Cleanup. */
  nxagentFreeScreenBoxes(work_screens, TRUE);
  nxagentFreeScreenBoxes(work_all_boxes, TRUE);

  SAFE_FREE(work_screens);
  SAFE_FREE(work_all_boxes);

  if (!(my_solution)) {
#ifdef WARNING
    fprintf(stderr, "%s: unable to generate \"fake\" solution.\n", __func__);
#endif

    nxagentScreenCrtcsFreeSolutions(ret);

    SAFE_FREE(ret);

    return(ret);
  }

  /*
   * Reaching this point means that we've extended everything to cover all
   * non-obsoleted base boxes.
   *
   * my_solution isn't part of ret yet, so add it.
   */
  xorg_list_append(&(my_solution->entry), ret);

  /*
   * At the end of this function, we should only have fully extended solutions
   * (i.e., no partial ones).
   * Due to that, extracing the best solution(s) should work fine and leave out
   * solutions that are not interesting to us.
   */
  nxagentScreenCrtcsSolutions *best_ret = nxagentScreenCrtcsExtractBestSolutions(ret);

  /* Get rid of old solutions list. */
  nxagentScreenCrtcsFreeSolutions(ret);

  SAFE_FREE(ret);

  ret = best_ret;

  return(ret);
}

/* Helper printing out a screen boxes list. */
static char* nxagentPrintScreenBoxes(const nxagentScreenBoxes *boxes) {
  char *ret = NULL;

  if ((!(boxes))) {
    return(ret);
  }

  char *construct = NULL;
  /* FIXME: xorg_list_is_empty is not const-correct. */
  if (xorg_list_is_empty((struct xorg_list *)(&(boxes->head)))) {
    if (-1 == asprintf(&construct, "empty")) {
      return(ret);
    }
  }
  else {
    size_t total_length = 512,
           last_pos = 0;
    construct = calloc(total_length, sizeof(char));

    if (!(construct)) {
      return(ret);
    }

    nxagentScreenBoxesElem *cur = NULL;
    xorg_list_for_each_entry(cur, &(boxes->head), entry) {
      char *box_str = nxagentPrintScreenBoxesElem(cur);

      if (!(box_str)) {
        SAFE_FREE(construct);
      }
      else {
        const size_t box_str_raw_len = (strlen(box_str) + 1);

        /*                    v-- implicit " -- " chars */
        while (((box_str_raw_len + 4) + last_pos) > total_length) {
          /* Buffer space needs to be expanded. */
          total_length += 512;
          char *new_construct = realloc(construct, total_length);

          if (!(new_construct)) {
            SAFE_FREE(construct);

            break;
          }
          else {
            construct = new_construct;

            memset((construct + last_pos), 0, (total_length - last_pos));
          }
        }

        if (construct) {
          const size_t write_count = snprintf((construct + last_pos), (total_length - last_pos), "%s -- ", box_str);

          if (0 > write_count) {
            const int errno_save = errno;

            fprintf(stderr, "%s: error writing box string representation to buffer: %s\n", __func__, strerror(errno_save));

            SAFE_FREE(construct);
          }
          else if (write_count >= (total_length - last_pos)) {
            fprintf(stderr, "%s: buffer was too small to hold new box string representation. Algorithm error.\n", __func__);

            SAFE_FREE(construct);
          }
          else {
            last_pos += write_count;
          }
        }
      }

      SAFE_FREE(box_str);

      if (!(construct)) {
        break;
      }
    }

    if (construct) {
      /* Drop the last delimiter (" -- ") character string. */
      construct[last_pos - 4] = 0;
    }
  }

  ret = construct;

  return(ret);
}

/*
 * Helper generating a "fake" solutions list based on the global window data.
 *
 * This is useful if no screens intersect the nxagent window.
 *
 * No pointer parameters might be NULL.
 *
 * On success, returns a "fake" solutions list, otherwise NULL.
 */
static nxagentScreenCrtcsSolutions* nxagentScreenCrtcsGenerateFakeSolutions(const nxagentScreenBoxes *all_boxes) {
  nxagentScreenCrtcsSolutions *ret = NULL;

  if (!(all_boxes)) {
    return(ret);
  }

  ret = calloc(1, sizeof(nxagentScreenCrtcsSolutions));

  if (!(ret)) {
    return(ret);
  }

  xorg_list_init(ret);

  nxagentScreenBoxes *fake_screen = calloc(1, sizeof(nxagentScreenBoxes));

  if (!(fake_screen)) {
    SAFE_FREE(ret);

    return(ret);
  }

  xorg_list_init(&(fake_screen->head));
  fake_screen->screen_id = -1;

  nxagentScreenBoxesElem *fake_screen_box = calloc(1, sizeof(nxagentScreenBoxesElem));

  if (!(fake_screen_box)) {
    SAFE_FREE(fake_screen);

    SAFE_FREE(ret);

    return(ret);
  }

  xorg_list_init(&(fake_screen_box->entry));
  fake_screen_box->screen_id = -1;

  BoxPtr fake_screen_box_data = calloc(1, sizeof(BoxRec));

  if (!(fake_screen_box_data)) {
    SAFE_FREE(fake_screen_box);

    SAFE_FREE(fake_screen);

    SAFE_FREE(ret);

    return(ret);
  }

  fake_screen_box_data->x1 = fake_screen_box_data->x2 = nxagentOption(X);
  fake_screen_box_data->y1 = fake_screen_box_data->y2 = nxagentOption(Y);
  fake_screen_box_data->x2 += nxagentOption(Width);
  fake_screen_box_data->y2 += nxagentOption(Height);

  fake_screen_box->box = fake_screen_box_data;

  xorg_list_append(&(fake_screen_box->entry), &(fake_screen->head));

  nxagentScreenCrtcsSolution *solution = nxagentScreenCrtcsGenerateFakeSolution(all_boxes, fake_screen);

  nxagentFreeScreenBoxes(fake_screen, TRUE);

  SAFE_FREE(fake_screen);

  if (!(solution)) {
    SAFE_FREE(ret);

    return(ret);
  }

  xorg_list_append(&(solution->entry), ret);

  return(ret);
}

/*
 * High-level wrapper generating a screen partition solution based upon a list
 * of base boxes and the remote Xinerama screen information.
 *
 * No pointers might be NULL. screen_count might not be zero.
 *
 * On success, returns one specific screen partition solution, otherwise NULL.
 */
static nxagentScreenCrtcsSolution* nxagentMergeScreenCrtcs(nxagentScreenBoxes *boxes, const XineramaScreenInfo *screen_info, const size_t screen_count) {
  nxagentScreenCrtcsSolution *ret = NULL;

  if ((!(boxes)) || (!(screen_info)) || (!(screen_count))) {
    return(ret);
  }

  size_t boxes_count = 0;
  nxagentScreenBoxesElem *cur = NULL;
  xorg_list_for_each_entry(cur, &(boxes->head), entry) {
    ++boxes_count;
  }

  /*
   * Step 1: consolidate boxes.
   *
   * Boxes might intersect with one screen, multiple screens or no screen.
   * We will consolidate boxes on a per-screen basis, in a way such that each
   * box will not be next to another that intersects the same screens.
   * Overlapping screens are handled by leaving a box in place if intersected
   * by multiple screens, if its neighbors are not also intersected by the
   * same screens.
   *
   * Example:
   *
   * ┌─────┬──────┬─────┬─────┐
   * │  1  │  1,2 │  2  │     │
   * ├─────┼──────┼─────┼─────┤
   * │  1  │  1   │     │     │
   * └─────┴──────┴─────┴─────┘
   *
   * Will/should be merged to:
   *
   * ┌─────┬──────┬─────┬─────┐
   * │  1  │  1,2 │  2  │     │
   * │     └──────┼─────┼─────┤
   * │  1     1   │     │     │
   * └────────────┴─────┴─────┘
   *
   * I.e., after the operation, these boxes will/should exist:
   *
   * ┌────────────┐  ┌────────────┐
   * │     1      │  │      2     │
   * │            │  └────────────┘
   * └────────────┘
   */

  nxagentScreenBoxes *screen_boxes = calloc(screen_count, sizeof(nxagentScreenBoxes));

  if (!(screen_boxes)) {
    nxagentFreeScreenBoxes(boxes, TRUE);

    return(ret);
  }

  for (size_t i = 0; i < screen_count; ++i) {
    xorg_list_init(&((screen_boxes + i)->head));
  }

  nxagentScreenBoxes *initial_screens = calloc(1, sizeof(nxagentScreenBoxes));

  if (!(initial_screens)) {
    nxagentFreeScreenBoxes(boxes, TRUE);

    return(ret);
  }

  xorg_list_init(&(initial_screens->head));
  initial_screens->screen_id = -1;

  if (!(nxagentMergeScreenBoxes(boxes, screen_boxes, screen_info, screen_count))) {
    for (size_t i = 0; i < screen_count; ++i) {
      nxagentFreeScreenBoxes((screen_boxes + i), TRUE);
    }

    SAFE_FREE(screen_boxes);

    nxagentFreeScreenBoxes(boxes, TRUE);

    return(ret);
  }

  /* Step 2: merge screen boxes into initial_screens. */
  size_t real_screen_count = 0;
  for (size_t i = 0; i < screen_count; ++i) {
    /* Filter out boxes with no intersections. */
    if (!(xorg_list_is_empty(&((screen_boxes) + i)->head))) {
      /* If merging was successful, we should only have one box per list. */
      nxagentScreenBoxesElem *cur = xorg_list_first_entry(&((screen_boxes + i)->head), nxagentScreenBoxesElem, entry);

      /* Remove from old list. */
      xorg_list_del(&(cur->entry));

      /* Add to the other list. */
      xorg_list_append(&(cur->entry), &(initial_screens->head));

      ++real_screen_count;

#ifdef WARNING
      if (i != cur->screen_id) {
        const unsigned long long idx = i;
        const signed long long screen_id = cur->screen_id;
        fprintf(stderr, "%s: internal screen id %lld doesn't match expected screen id %llu! Algorithm warning.\n", __func__, screen_id, idx);
      }
#endif
    }
  }

  /* Lists should be all empty now, get rid of list heads. */
  SAFE_FREE(screen_boxes);

#ifdef DEBUG
  fprintf(stderr, "%s: merged initial screens into initial_screens, all boxes should be correctly marked.\n", __func__);

  fprintf(stderr, "%s: dumping initial_screens:\n", __func__);
  {
    char *initial_screens_str = nxagentPrintScreenBoxes(initial_screens);

    if (initial_screens_str) {
      fprintf(stderr, "%s:   %s\n", __func__, initial_screens_str);
    }
    else {
      fprintf(stderr, "%s:   error!\n", __func__);
    }

    SAFE_FREE(initial_screens_str);
  }

  fprintf(stderr, "%s: dumping all boxes:\n", __func__);
  {
    char *boxes_str = nxagentPrintScreenBoxes(boxes);

    if (boxes_str) {
      fprintf(stderr, "%s:   %s\n", __func__, boxes_str);
    }
    else {
      fprintf(stderr, "%s:   error!\n", __func__);
    }

    SAFE_FREE(boxes_str);
  }
#endif

  nxagentScreenCrtcsSolutions *solutions = NULL;
  /* Step 3: extend original screen boxes to cover the whole area. */
  if (real_screen_count) {
    solutions = nxagentScreenCrtcsGenerateSolutions(boxes, initial_screens, boxes_count, real_screen_count, NULL);
  }
  else {
    /*
     * No screens intersecting the window maeans that we can create a fake
     * solution containing just one (virtual) screen and just use that one.
     */
    solutions = nxagentScreenCrtcsGenerateFakeSolutions(boxes);
  }

  /*
   * Everything should be copied internally, so get rid of our original data.
   */
  nxagentFreeScreenBoxes(initial_screens, TRUE);

  SAFE_FREE(initial_screens);

  if ((!(solutions)) || (xorg_list_is_empty(solutions))) {
    /*
     * Invalid or empty solutions list means that something is wrong.
     * Error out.
     */
#ifdef WARNING
    fprintf(stderr, "%s: solutions list empty or invalid. Algorithm error.\n", __func__);
#endif

    nxagentScreenCrtcsFreeSolutions(solutions);

    SAFE_FREE(solutions);

    return(ret);
  }

  /*
   * Step 4: select specific solution.
   * Should be valid, checked for emptiness before. It's possible to have
   * multiple solutions (logically with the same rating), but we have to select
   * a specific one here.
   * We'll use the very first one.
   */
  nxagentScreenCrtcsSolution *first_entry = xorg_list_first_entry(solutions, nxagentScreenCrtcsSolution, entry);

  ret = nxagentScreenCrtcsSolutionCopy(first_entry);

  nxagentScreenCrtcsFreeSolutions(solutions);

  SAFE_FREE(solutions);

  if (!(ret)) {
#ifdef WARNING
    fprintf(stderr, "%s: unable to copy first solution entry.\n", __func__);
#endif

    return(ret);
  }

  return(ret);
}

/*
 Destroy an output after removing it from any crtc that might reference it
 */
void nxagentDropOutput(RROutputPtr o) {
  RRCrtcPtr c = o->crtc;
  if (c) {
    for (int i = 0; i < c->numOutputs; i++) {
      if (c->outputs[i] == o) {
#ifdef DEBUG
	fprintf(stderr, "nxagentDropOutput: output [%s] is in use by crtc [%p], removing it from there\n", o->name, c);
#endif
	RRCrtcSet(c, NULL, 0, 0, RR_Rotate_0, 0, NULL);
      }
    }
  }
#ifdef DEBUG
  fprintf(stderr, "nxagentDropOutput: destroying output [%s]\n", o->name);
#endif
  RROutputDestroy(o);
}

/*
 * Helper used to swap the *data* of two nxagentScreenBoxesElem objects.
 * Metadata, such as the internal list pointers, is not touched.
 *
 * Returns true on success, otherwise false.
 */
static Bool nxagentScreenBoxesElemSwap(nxagentScreenBoxesElem *lhs, nxagentScreenBoxesElem *rhs) {
  Bool ret = FALSE;

  if ((!(lhs)) || (!(rhs))) {
    return(ret);
  }

  nxagentScreenBoxesElem *tmp = nxagentScreenBoxesElemCopy(lhs, FALSE);

  if (!(tmp)) {
    return(ret);
  }

  lhs->obsolete  = rhs->obsolete;
  lhs->screen_id = rhs->screen_id;
  lhs->box       = rhs->box;

  rhs->obsolete  = tmp->obsolete;
  rhs->screen_id = tmp->screen_id;
  rhs->box       = tmp->box;

  SAFE_FREE(tmp);

  ret = TRUE;

  return(ret);
}

/*
 * Helper executing the actual quicksort implementation.
 *
 * No pointer parameters might be NULL.
 *
 * Returns true on success, otherwise false.
 */
static Bool nxagentScreenBoxesQSortImpl(nxagentScreenBoxesElem *left, nxagentScreenBoxesElem *right, const size_t left_idx, const size_t right_idx, nxagentScreenBoxes *boxes) {
  Bool ret = TRUE;

  if ((!(left)) || (!(right)) || (!(boxes))) {
    ret = FALSE;

    return(ret);
  }

  if (left_idx >= right_idx) {
    return(ret);
  }

  /* Select pivot. */
  size_t diff = (right_idx - left_idx);
  size_t pivot_i = (left_idx + (rand() % diff));

  nxagentScreenBoxesElem *pivot = NULL;
  {
    size_t i = 0;
    xorg_list_for_each_entry(pivot, &(boxes->head), entry) {
      if (i++ == pivot_i) {
        break;
      }
    }
  }

  /* IDs should be unique, so no need to optimize for same values. */
  nxagentScreenBoxesElem *left_  = left,
                         *right_ = right,
                         *split  = NULL;
  ssize_t left_i  = left_idx,
          right_i = right_idx,
          split_i = -1;
  while (TRUE) {
    /*
     * Careful: xorg_list_for_each_entry() skips over the first element (since
     * it's assumed to be the list head without actual data), so we'll need to
     * "rewind" the pointer first.
     *
     * Don't do this for right_, since we use a special implementation for
     * iterating backwards.
     */
    left_ = xorg_list_last_entry(&(left_->entry), nxagentScreenBoxesElem, entry);

    xorg_list_for_each_entry(left_, &(left_->entry), entry) {
      if (&(left_->entry) == &(boxes->head)) {
        ret = FALSE;

        break;
      }

      /*
       * Normally implementations check if they should continue, we check if we
       * should break out instead.
       *
       * N.B.: left_ should never reach the list head.
       */
      if ((left_->screen_id) >= (pivot->screen_id)) {
        break;
      }

      ++left_i;
    }

    if (!(ret)) {
      break;
    }

    /*
     * The xorg_list implementation does not have a way to iterate over a list
     * reversely, so implement it with basic building blocks.
     */
    while (&(right_->entry) != &(right->entry)) {
      if (&(right_->entry) == &(boxes->head)) {
        ret = FALSE;

        break;
      }

      /*
       * Normally implementations check if they should continue, we check if we
       * should break out instead.
       *
       * N.B.: right_ should never reach the list head.
       */
      if ((right_->screen_id) <= (pivot->screen_id)) {
        break;
      }

      --right_i;

      /*
       * Move backwards. Last entry is actually the previous one. For more
       * information see the comments in nxagentScreenBoxesUpdateScreenBox().
       */
      right_ = xorg_list_last_entry(&(right_->entry), nxagentScreenBoxesElem, entry);
    }

    if (!(ret)) {
      break;
    }

    if (left_i >= right_i) {
      split   = right;
      split_i = right_i;

      break;
    }

    ret = nxagentScreenBoxesElemSwap(left_, right_);

    if (!(ret)) {
      break;
    }
  }

  if (!(ret)) {
    return(ret);
  }

  ret = nxagentScreenBoxesQSortImpl(left, split, left_idx, split_i, boxes);

  if (!(ret)) {
    return(ret);
  }

  ret = nxagentScreenBoxesQSortImpl(xorg_list_first_entry(&(split->entry), nxagentScreenBoxesElem, entry), right, (split_i + 1), right_idx, boxes);

  return(ret);
}

/*
 * Helper sorting an nxagentScreenBoxes list.
 *
 * No pointer parameters might be NULL.
 *
 * Returns true on success, otherwise false.
 */
static Bool nxagentScreenBoxesQSort(nxagentScreenBoxes *boxes) {
  Bool ret = FALSE;

  if (!(boxes)) {
    return(ret);
  }

  if (xorg_list_is_empty(&(boxes->head))) {
    ret = TRUE;

    return(ret);
  }

  /*
   * Questionable error handling: check that each screen_id is unique.
   *
   * Otherwise this implementation will crash.
   * That's probably fine since having a list with duplicated screen_id entries
   * is a bug in the generation code anyway.
   */
  Bool unique = TRUE;
  {
    nxagentScreenBoxesElem *lhs = NULL;
    xorg_list_for_each_entry(lhs, &(boxes->head), entry) {
      nxagentScreenBoxesElem *rhs = NULL;
      xorg_list_for_each_entry(rhs, &(lhs->entry), entry) {
        /* Check for actual list head and break out. */
        if (&(rhs->entry) == &(boxes->head)) {
          break;
        }

        /* Otherwise rhs is a valid entry. */
        if (lhs->screen_id == rhs->screen_id) {
          unique = FALSE;
          break;
        }
      }

      if (!(unique)) {
        /* Found at least one duplicated entry, break out. */
        break;
      }
    }
  }

  if (!(unique)) {
    return(ret);
  }

  /*
   * Questionable optimization: check if list is already sorted.
   */
  Bool sorted = TRUE;
  {
    ssize_t last_screen_id = -1;
    nxagentScreenBoxesElem *cur = NULL;
    xorg_list_for_each_entry(cur, &(boxes->head), entry) {
      if (cur->screen_id < last_screen_id) {
        sorted = FALSE;

        break;
      }

      last_screen_id = cur->screen_id;
    }
  }

  if (sorted) {
    ret = TRUE;

    return(ret);
  }

  /* Seed PRNG. We don't need good entropy, some is enough. */
  srand((unsigned int)(time(NULL)));

  /* Get boxes count. */
  size_t boxes_count = 0;
  {
    nxagentScreenBoxesElem *cur = NULL;
    xorg_list_for_each_entry(cur, &(boxes->head), entry) {
      ++boxes_count;
    }
  }

  ret = nxagentScreenBoxesQSortImpl(xorg_list_first_entry(&(boxes->head), nxagentScreenBoxesElem, entry), xorg_list_last_entry(&(boxes->head), nxagentScreenBoxesElem, entry), 0, (boxes_count - 1), boxes);

  return(ret);
}

int nxagentAdjustRandRXinerama(ScreenPtr pScreen)
{
  rrScrPrivPtr pScrPriv;
  RROutputPtr  output;
  xRRModeInfo  modeInfo;
  char         name[100];
  int          refresh = 60;

  pScrPriv = rrGetScrPriv(pScreen);

  if (pScrPriv) {
    int number = 0;

    XineramaScreenInfo *screeninfo = NULL;

    screeninfo = XineramaQueryScreens(nxagentDisplay, &number);
    if (number) {
#ifdef DEBUG
      fprintf(stderr, "nxagentAdjustRandRXinerama: XineramaQueryScreens() returned [%d] screens:\n", number);
      for (size_t i = 0; i < number; ++i) {
        fprintf(stderr, "nxagentAdjustRandRXinerama:   screen_number [%d] x_org [%d] y_org [%d] width [%d] height [%d]\n", screeninfo[i].screen_number, screeninfo[i].x_org, screeninfo[i].y_org, screeninfo[i].width, screeninfo[i].height);
      }
#endif
    }
    else {
#ifdef DEBUG
      fprintf(stderr, "nxagentAdjustRandRXinerama: XineramaQueryScreens() failed - continuing without Xinerama\n");
#endif
    }

    /*
     * If there's no xinerama on the real server or xinerama is
     * disabled in nxagent we only report one big screen. Clients
     * still see xinerama enabled but it will report only one (big)
     * screen. This is consistent with the way rrxinerama always
     * behaved. The single PanoramiX/Xinerama extension however
     * disables xinerama if only one screen exists.
     */
    if (number == 0) {
      #ifdef DEBUG
      fprintf(stderr, "nxagentAdjustRandRXinerama: faking xinerama\n");
      #endif
      number = 1;

      SAFE_FREE(screeninfo);

      if (!(screeninfo = calloc(1, sizeof(XineramaScreenInfo)))) {
        return FALSE;
      }

      /* fake a xinerama screeninfo that covers the whole screen */
      screeninfo->screen_number = 0;
      screeninfo->x_org = nxagentOption(X);
      screeninfo->y_org = nxagentOption(Y);
      screeninfo->width = nxagentOption(Width);
      screeninfo->height = nxagentOption(Height);
    }

#ifdef DEBUG
    fprintf(stderr, "nxagentAdjustRandRXinerama: numCrtcs [%d], numOutputs [%d]\n", pScrPriv->numCrtcs, pScrPriv->numOutputs);
    {
      Bool rrgetinfo;

      /*
       * Convert old RANDR 1.0 data (if any) to current structure. This
       * is needed once at the first run of this function. If we don't
       * do this here it will be done implicitly later and add mode(s) to
       * our crtc(s)!
       */
      rrgetinfo = RRGetInfo(pScreen, FALSE);

      fprintf(stderr, "nxagentAdjustRandRXinerama: RRGetInfo returned [%d]\n", rrgetinfo);

      const signed int x = nxagentOption(X),
                       y = nxagentOption(Y),
                       w = nxagentOption(Width),
                       h = nxagentOption(Height);
      fprintf(stderr, "%s: nxagent window extends: [(%d, %d), (%d, %d)]\n", __func__, x, y, (x + w), (y + h));
    }
#else
    /* we are not interested in the return code */
    RRGetInfo(pScreen, FALSE);
#endif

    nxagentScreenSplits *splits = nxagentGenerateScreenSplitList(screeninfo, number);

    if (!(splits)) {
      fprintf(stderr, "%s: unable to generate screen split list.\n", __func__);

      SAFE_FREE(screeninfo);

      return(FALSE);
    }

    nxagentScreenBoxes *all_boxes = nxagentGenerateScreenCrtcs(splits);

    /* Get rid of splits. */
    SAFE_FREE(splits->x_splits);
    SAFE_FREE(splits->y_splits);
    SAFE_FREE(splits);

    if ((!(all_boxes)) || xorg_list_is_empty(&(all_boxes->head))) {
      fprintf(stderr, "%s: unable to generate screen boxes list from screen splitting list.\n", __func__);

      SAFE_FREE(screeninfo);

      return(FALSE);
    }

    nxagentScreenCrtcsSolution *solution = nxagentMergeScreenCrtcs(all_boxes, screeninfo, number);

    /* Get rid of all_boxes. */
    nxagentFreeScreenBoxes(all_boxes, TRUE);

    SAFE_FREE(all_boxes);

    if ((!(solution)) || (!(solution->solution_boxes)) || (!(solution->all_boxes))) {
      fprintf(stderr, "%s: unable to extract screen boxes from screen metadata.\n", __func__);

      SAFE_FREE(screeninfo);

      return(FALSE);
    }

    /* Sort new solution boxes list based on screen ID. */
    Bool sorted = nxagentScreenBoxesQSort(solution->solution_boxes);

    if (!(sorted)) {
      fprintf(stderr, "%s: unable to sort solution screen boxes based on screen IDs.\n", __func__);

      nxagentScreenCrtcsFreeSolution(solution);

      SAFE_FREE(solution);

      SAFE_FREE(screeninfo);

      return(FALSE);
    }

    #ifdef DEBUG
    fprintf(stderr, "nxagentAdjustRandRXinerama: numCrtcs [%d], numOutputs [%d]\n", pScrPriv->numCrtcs, pScrPriv->numOutputs);
    #endif

    #ifdef WARNING
    if (nxagentScreenCrtcsTiling) {
      size_t old_screen_count = 0;
      nxagentScreenBoxesElem *cur = NULL;
      xorg_list_for_each_entry(cur, &(nxagentScreenCrtcsTiling->solution_boxes->head), entry) {
        ++old_screen_count;
      }

      if (old_screen_count != pScrPriv->numCrtcs) {
        const unsigned long long old_screen_count_ = old_screen_count;
        const signed long long cur_crtcs_count = pScrPriv->numCrtcs;
        fprintf(stderr, "%s: current CRTCs count [%lld] doesn't match old tiling data [%llu]. Algorithm warning.\n", __func__, cur_crtcs_count, old_screen_count_);
      }
    }
    #endif

    size_t new_crtcs_count = 0;
    {
      nxagentScreenBoxesElem *cur = NULL;
      xorg_list_for_each_entry(cur, &(solution->solution_boxes->head), entry) {
        ++new_crtcs_count;
      }
    }

    /*
     * Adjust the number of CRTCs to match the number of reported Xinerama
     * screens on the real server that intersect the nxagent window.
     */
    /*
     * The number of CRTCs might not be an appropriate means of setting up
     * screen splitting since old and new screen IDs might differ. Doing some
     * more complicated mapping between old and new screen IDs here would be
     * possible, but likely isn't needed since each CRTC here is a purely
     * virtual one in the first place.
     *
     * Pretend we have three screens in both the old and new solutions, but the
     * middle one switched IDs from 2 to 4. Doing some complicated mapping
     * wouldn't really lead to a different result, since we'd need to drop and
     * re-add the virtual screen with a different size anyway. Just naïvely
     * matching screen counts, however, has the added benefit of less virtual
     * screen removals and additions if only metadata changed, but not the
     * actual virtual screens count.
     */
    while (new_crtcs_count != pScrPriv->numCrtcs) {
      if (new_crtcs_count < pScrPriv->numCrtcs) {
        #ifdef DEBUG
        fprintf(stderr, "nxagentAdjustRandRXinerama: destroying crtc\n");
        #endif
        /* first reset the crtc to free possible outputs, then destroy the crtc */
        RRCrtcSet(pScrPriv->crtcs[pScrPriv->numCrtcs - 1], NULL, 0, 0, RR_Rotate_0, 0, NULL);
        RRCrtcDestroy(pScrPriv->crtcs[pScrPriv->numCrtcs - 1]);
      }
      else {
        #ifdef DEBUG
        fprintf(stderr, "nxagentAdjustRandRXinerama: adding crtc\n");
        #endif
        RRCrtcCreate(pScreen, NULL);
      }
    }

    #ifdef DEBUG
    fprintf(stderr, "nxagentAdjustRandRXinerama: numCrtcs [%d], numOutputs [%d]\n", pScrPriv->numCrtcs, pScrPriv->numOutputs);
    #endif

    /*
     * Set gamma. Currently the only reason for doing this is preventing the
     * xrandr command from complaining about missing gamma.
     */
    for (size_t i = 0; i < pScrPriv->numCrtcs; ++i) {
      if (pScrPriv->crtcs[i]->gammaSize == 0) {
        CARD16 gamma = 0;
        RRCrtcGammaSetSize(pScrPriv->crtcs[i], 1);
        RRCrtcGammaSet(pScrPriv->crtcs[i], &gamma, &gamma, &gamma);
        RRCrtcGammaNotify(pScrPriv->crtcs[i]);
      }
    }

    /* Delete superfluous non-NX outputs. */
    for (ptrdiff_t i = pScrPriv->numOutputs - 1; i >= 0; --i) {
      if (strncmp(pScrPriv->outputs[i]->name, "NX", 2)) {
        nxagentDropOutput(pScrPriv->outputs[i]);
      }
    }

    /*
     * At this stage only NX outputs are left - we delete the superfluous
     * ones.
     */
    if (new_crtcs_count > (ptrdiff_t)(PTRDIFF_MAX)) {
      const unsigned long long new_crtcs_count_ = new_crtcs_count;
      const unsigned long long max_crtcs = PTRDIFF_MAX;
      fprintf(stderr, "%s: too many screen CRTCs [%llu], supporting at most [%llu]; erroring out, but keeping old solution.\n", __func__, new_crtcs_count_, max_crtcs);

      nxagentScreenCrtcsFreeSolution(solution);

      SAFE_FREE(solution);

      SAFE_FREE(screeninfo);

      return(FALSE);
    }

    for (ptrdiff_t i = pScrPriv->numOutputs - 1; i >= (ptrdiff_t)(new_crtcs_count); --i) {
      nxagentDropOutput(pScrPriv->outputs[i]);
    }

    /* Add and init outputs. */
    for (size_t i = 0; i < new_crtcs_count; ++i) {
      if (i >= pScrPriv->numOutputs) {
        const unsigned long long i_ = i;
        sprintf(name, "NX%llu", (i_ + 1));
        output = RROutputCreate(pScreen, name, strlen(name), NULL);
        /* will be done later
        RROutputSetConnection(output, RR_Disconnected);
        */
        #ifdef DEBUG
        fprintf(stderr, "nxagentAdjustRandRXinerama: created new output [%s]\n", name);
        #endif
      }
      else {
        output = pScrPriv->outputs[i];
      }
      #ifdef DEBUG
      fprintf(stderr, "nxagentAdjustRandRXinerama: adjusting output [%s]\n", pScrPriv->outputs[i]->name);
      #endif
      RROutputSetCrtcs(output, &(pScrPriv->crtcs[i]), 1);
      /* FIXME: Isn't there a function for setting this? */
      output->crtc = pScrPriv->crtcs[i];
      /* FIXME: get SubPixelOrder from real X server */
      RROutputSetSubpixelOrder(output, SubPixelUnknown);
      /* FIXME: What is the correct physical size here? */
      RROutputSetPhysicalSize(output, 0, 0);
    }

    nxagentScreenBoxesElem *cur_screen_box = xorg_list_first_entry(&(solution->solution_boxes->head), nxagentScreenBoxesElem, entry);
    for (size_t i = 0; i < pScrPriv->numOutputs; ++i) {
      RRModePtr mymode = NULL, prevmode = NULL;

      /* Sanity checks. */
      Bool fail = FALSE;
      if (cur_screen_box->box->x1 < nxagentOption(X)) {
        fprintf(stderr, "%s: left X position out of bounds - less than left window bound [%d] < [%d]\n", __func__, cur_screen_box->box->x1, nxagentOption(X));

        fail = TRUE;
      }

      if (cur_screen_box->box->x1 >= (nxagentOption(X) + nxagentOption(Width))) {
        fprintf(stderr, "%s: left X position out of bounds - higher than or equal right window bound [%d] >= [%d]\n", __func__, cur_screen_box->box->x1, (nxagentOption(X) + nxagentOption(Width)));

        fail = TRUE;
      }

      if (cur_screen_box->box->x2 <= cur_screen_box->box->x1) {
        fprintf(stderr, "%s: right X position out of bounds - less than or equal left X position [%d] <= [%d]\n", __func__, cur_screen_box->box->x2, cur_screen_box->box->x1);

        fail = TRUE;
      }

      if (cur_screen_box->box->x2 > (nxagentOption(X) + nxagentOption(Width))) {
        fprintf(stderr, "%s: right X position out of bounds - less than or equal right window bound [%d] <= [%d]\n", __func__, cur_screen_box->box->x2, (nxagentOption(X) + nxagentOption(Width)));

        fail = TRUE;
      }

      if (cur_screen_box->box->y1 < nxagentOption(Y)) {
        fprintf(stderr, "%s: top Y position out of bounds - less than top window bound [%d] < [%d]\n", __func__, cur_screen_box->box->y1, nxagentOption(Y));

        fail = TRUE;
      }

      if (cur_screen_box->box->y1 >= (nxagentOption(Y) + nxagentOption(Height))) {
        fprintf(stderr, "%s: top Y position out of bounds - higher than or equal top window bound [%d] >= [%d]\n", __func__, cur_screen_box->box->y1, (nxagentOption(Y) + nxagentOption(Height)));

        fail = TRUE;
      }

      if (cur_screen_box->box->y2 <= cur_screen_box->box->y1) {
        fprintf(stderr, "%s: bottom Y position out of bounds - less than or equal top Y position [%d] <= [%d]\n", __func__, cur_screen_box->box->y2, cur_screen_box->box->y1);

        fail = TRUE;
      }

      if (cur_screen_box->box->y2 > (nxagentOption(Y) + nxagentOption(Height))) {
        fprintf(stderr, "%s: bottom Y position out of bounds - less than or equal bottom window bound [%d] <= [%d]\n", __func__, cur_screen_box->box->y2, (nxagentOption(Y) + nxagentOption(Height)));

        fail = TRUE;
      }

      if (fail) {
        nxagentScreenCrtcsFreeSolution(solution);

        SAFE_FREE(solution);

        SAFE_FREE(screeninfo);

        return(FALSE);
      }

      const int new_x = (cur_screen_box->box->x1 - nxagentOption(X)),
                new_y = (cur_screen_box->box->y1 - nxagentOption(Y));
      const unsigned int new_w = (cur_screen_box->box->x2 - cur_screen_box->box->x1),
                         new_h = (cur_screen_box->box->y2 - cur_screen_box->box->y1);

      /* Save previous mode. */
      prevmode = pScrPriv->crtcs[i]->mode;
      #ifdef DEBUG
      {
        const unsigned long long i_ = i;
        if (prevmode) {
          fprintf(stderr, "nxagentAdjustRandRXinerama: output [%llu] name [%s]: prevmode [%s] ([%p]) refcnt [%d]\n", i_, pScrPriv->outputs[i]->name, prevmode->name, (void *)prevmode, prevmode->refcnt);
        }
        else {
          fprintf(stderr, "nxagentAdjustRandRXinerama: output [%llu] name [%s]: no prevmode\n", i_, pScrPriv->outputs[i]->name);
        }
      }
      #endif

      /* Map output to CRTC. */
      RROutputSetCrtcs(pScrPriv->outputs[i], &(pScrPriv->crtcs[i]), 1);

      #ifdef DEBUG
      {
        const unsigned long long i_ = i;
        fprintf(stderr, "nxagentAdjustRandRXinerama: output [%llu] name [%s]: CRTC is x [%d] y [%d] width [%d] height [%d]\n", i_, pScrPriv->outputs[i]->name, new_x, new_y, new_w, new_h);
      }
      #endif

      RROutputSetConnection(pScrPriv->outputs[i], RR_Connected);

      memset(&modeInfo, '\0', sizeof(modeInfo));

#ifdef NXAGENT_RANDR_MODE_PREFIX
      /*
       * Avoid collisions with pre-existing default modes by using a
       * separate namespace. If we'd simply use XxY we could not
       * distinguish between pre-existing modes which should stay
       * and our own modes that should be removed after use.
       */
      sprintf(name, "nx_%dx%d", new_w, new_h);
#else
      sprintf(name, "%dx%d", new_w, new_h);
#endif

      modeInfo.width  = new_w;
      modeInfo.height = new_h;
      modeInfo.hTotal = new_w;
      modeInfo.vTotal = new_h;
      modeInfo.dotClock = ((CARD32) new_w * (CARD32) new_h * (CARD32) refresh);
      modeInfo.nameLength = strlen(name);

      mymode = RRModeGet(&modeInfo, name);

#ifdef DEBUG
      {
        const unsigned long long i_ = i;
        if (mymode) {
          fprintf(stderr, "nxagentAdjustRandRXinerama: output [%llu] name [%s]: mode [%s] ([%p]) created/received, refcnt [%d]\n", i_, pScrPriv->outputs[i]->name, name, (void *) mymode, mymode->refcnt);
        }
        else {
          /* FIXME: what is the correct behaviour in this case? */
          fprintf(stderr, "nxagentAdjustRandRXinerama: output [%llu] name [%s]: mode [%s] creation failed!\n", i_, pScrPriv->outputs[i]->name, name);
        }
      }
#endif
      if (prevmode && mymode == prevmode) {
        #ifdef DEBUG
        fprintf(stderr, "nxagentAdjustRandRXinerama: mymode [%s] ([%p]) == prevmode [%s] ([%p])\n", mymode->name, (void *) mymode, prevmode->name, (void *)prevmode);
        #endif

        /*
         * If they are the same RRModeGet() has increased the
         * refcnt by 1. We decrease it again by calling only
         * RRModeDestroy() and forget about prevmode.
         */
        RRModeDestroy(mymode);
      }
      else {
        #ifdef DEBUG
        const unsigned long long i_ = i;
        fprintf(stderr, "nxagentAdjustRandRXinerama: setting mode [%s] ([%p]) refcnt [%d] for output %llu [%s]\n", mymode->name, (void *) mymode, mymode->refcnt, i_, pScrPriv->outputs[i]->name);
        #endif
        RROutputSetModes(pScrPriv->outputs[i], &mymode, 1, 0);
      }

      #ifdef DEBUG
      {
        const unsigned long long i_ = i;
        fprintf(stderr, "nxagentAdjustRandRXinerama: setting mode [%s] ([%p]) refcnt [%d] for crtc %llu\n", mymode->name, (void *) mymode, mymode->refcnt, i_);
      }
      #endif
      RRCrtcSet(pScrPriv->crtcs[i], mymode, new_x, new_y, RR_Rotate_0, 1, &(pScrPriv->outputs[i]));

      /*
       * Throw away the mode if otherwise unused. We do not need it
       * anymore. We call FreeResource() to ensure the system will not
       * try to free it again on shutdown.
       */
      if (prevmode && prevmode->refcnt == 1) {
        #ifdef DEBUG
        fprintf(stderr, "nxagentAdjustRandRXinerama: destroying prevmode [%s]\n", prevmode->name);
        #endif
        FreeResource(prevmode->mode.id, 0);
      }

      RROutputChanged(pScrPriv->outputs[i], TRUE);
      RRCrtcChanged(pScrPriv->crtcs[i], TRUE);

      cur_screen_box = xorg_list_first_entry(&(cur_screen_box->entry), nxagentScreenBoxesElem, entry);
    }

    /* Update internal data. */
    nxagentScreenCrtcsSolution *old_solution = nxagentScreenCrtcsTiling;
    nxagentScreenCrtcsTiling = solution;

    /* Release allocated memory. */
    nxagentScreenCrtcsFreeSolution(old_solution);

    SAFE_FREE(old_solution);

    SAFE_FREE(screeninfo);

#ifdef DEBUG
    for (size_t i = 0; i < pScrPriv->numCrtcs; ++i) {
      const unsigned long long i_ = i;
      RRModePtr mode = pScrPriv->crtcs[i]->mode;
      if (mode) {
        fprintf(stderr, "nxagentAdjustRandRXinerama: crtc [%llu] ([%p]) has mode [%s] ([%p]), refcnt [%d] and [%d] outputs:\n", i_, (void *) pScrPriv->crtcs[i], pScrPriv->crtcs[i]->mode->name, (void *)pScrPriv->crtcs[i]->mode, pScrPriv->crtcs[i]->mode->refcnt, pScrPriv->crtcs[i]->numOutputs);
      }
      else {
        fprintf(stderr, "nxagentAdjustRandRXinerama: crtc [%llu] ([%p]) has no mode and [%d] outputs:\n", i_, (void *) pScrPriv->crtcs[i], pScrPriv->crtcs[i]->numOutputs);
      }

      if (pScrPriv->crtcs[i]->numOutputs > 0) {
        for (size_t j = 0; j < pScrPriv->crtcs[i]->numOutputs; ++j) {
          const unsigned long long j_ = j;
          fprintf(stderr, "nxagentAdjustRandRXinerama:   output [%llu] name [%s]->crtc=[%p]\n", j_, pScrPriv->crtcs[i]->outputs[j]->name, (void *)pScrPriv->crtcs[i]->outputs[j]->crtc);
        }
      }
    }
#endif

    pScrPriv -> lastSetTime = currentTime;

    pScrPriv->changed = TRUE;
    pScrPriv->configChanged = TRUE;
  }

  /*
   * Adjust screen size according the newly set modes.
   * Not calling this function leads to the initial screen size left in place,
   * which is not what we want in case the window is resizable.
   */
  RRScreenSizeNotify(pScreen);

  /* FIXME: adjust maximum screen size according to remote randr/xinerama setup */

  #ifdef DEBUG
  fprintf(stderr, "nxagentAdjustRandRXinerama: Min %dx%d, Max %dx%d \n", pScrPriv->minWidth, pScrPriv->minHeight, pScrPriv->maxWidth, pScrPriv->maxHeight);
  #endif

  return TRUE;
}

void nxagentSaveAreas(PixmapPtr pPixmap, RegionPtr prgnSave, int xorg, int yorg, WindowPtr pWin)
{
  PixmapPtr pVirtualPixmap;
  nxagentPrivPixmapPtr pPrivPixmap;
  XlibGC gc;
  XGCValues values;
  int i;
  int xSrc, ySrc, xDst, yDst, w, h;
  int nRects;
  int size;
  BoxPtr pBox;
  XRectangle *pRects;
  BoxRec extents;
  RegionRec cleanRegion;

  miBSWindowPtr pBackingStore = (miBSWindowPtr) pWin -> backStorage;

  pVirtualPixmap = nxagentVirtualPixmap(pPixmap);

  pPrivPixmap = nxagentPixmapPriv(pPixmap);

  pPrivPixmap -> isBackingPixmap = 1;

  fbCopyWindowProc(&pWin -> drawable, &pVirtualPixmap -> drawable, 0, RegionRects(prgnSave),
                       RegionNumRects(prgnSave), xorg, yorg, FALSE, FALSE, 0, 0);

  values.subwindow_mode = IncludeInferiors;

  gc = XCreateGC(nxagentDisplay, nxagentWindow(screenInfo.screens[0]->root), GCSubwindowMode, &values);

  /*
   * Initialize to the corrupted region.
   * Coordinates are relative to the window.
   */

  RegionInit(&cleanRegion, NullBox, 1);

  RegionCopy(&cleanRegion, nxagentCorruptedRegion((DrawablePtr) pWin));

  /*
   * Subtract the corrupted region from the saved region.
   */

  RegionSubtract(&pBackingStore -> SavedRegion, &pBackingStore -> SavedRegion, &cleanRegion);

  /*
   * Translate the corrupted region. Coordinates
   * are relative to the backing store pixmap.
   */

  RegionTranslate(&cleanRegion, -pBackingStore -> x, -pBackingStore -> y);

  /*
   * Compute the clean region to be saved: subtract
   * the corrupted region from the region to be saved.
   */

  RegionSubtract(&cleanRegion, prgnSave, &cleanRegion);

  nRects = RegionNumRects(&cleanRegion);
  size = nRects * sizeof(*pRects);
  pRects = (XRectangle *) malloc(size);
  pBox = RegionRects(&cleanRegion);

  for (i = nRects; i-- > 0;)
  {
    pRects[i].x = pBox[i].x1;
    pRects[i].y = pBox[i].y1;
    pRects[i].width = pBox[i].x2 - pBox[i].x1;
    pRects[i].height = pBox[i].y2 - pBox[i].y1;
  }

  XSetClipRectangles(nxagentDisplay, gc, 0, 0, pRects, nRects, Unsorted);

  free((char *) pRects);

  extents = *RegionExtents(&cleanRegion);

  RegionUninit(&cleanRegion);

  xDst = extents.x1;
  yDst = extents.y1;

/*
 * Left here the wrong solution. The window could be not
 * configured yet on the real X, whilst the x and y in the
 * WindowRec are the new coordinates. The right solution
 * is the other, as it is independent from the window
 * coordinates.
 *
 *  xSrc = xDst + xorg - pWin -> drawable.x;
 *  ySrc = yDst + yorg - pWin -> drawable.y;
 */

  xSrc = xDst + pBackingStore -> x;
  ySrc = yDst + pBackingStore -> y;

  w = extents.x2 - extents.x1;
  h = extents.y2 - extents.y1;

  XCopyArea(nxagentDisplay, nxagentWindow(pWin), nxagentPixmap(pPixmap), gc,
                xSrc, ySrc, w, h, xDst, yDst);

  nxagentAddItemBSPixmapList(nxagentPixmap(pPixmap), pPixmap, pWin,
                                 pBackingStore -> x, pBackingStore -> y);

  #ifdef TEST
  fprintf(stderr,"nxagentSaveAreas: Added pixmap [%p] with id [%d] on window [%p] to BSPixmapList.\n",
                 (void *) pPixmap, nxagentPixmap(pPixmap), (void *) pWin);
  #endif

  XFreeGC(nxagentDisplay, gc);

  return;
}

void nxagentRestoreAreas(PixmapPtr pPixmap, RegionPtr prgnRestore, int xorg,
                             int yorg, WindowPtr pWin)
{
  PixmapPtr pVirtualPixmap;
  RegionPtr clipRegion;
  XlibGC gc;
  XGCValues values;
  int i;
  int xSrc, ySrc, xDst, yDst, w, h;
  int nRects;
  int size;
  BoxPtr pBox;
  XRectangle *pRects;
  BoxRec extents;
  miBSWindowPtr pBackingStore;

  /*
   * Limit the area to restore to the
   * root window size.
   */

  RegionIntersect(prgnRestore, prgnRestore,
                       &pWin -> drawable.pScreen -> root -> winSize);

  pBackingStore = (miBSWindowPtr) pWin -> backStorage;

  pVirtualPixmap = nxagentVirtualPixmap(pPixmap);

  fbCopyWindowProc(&pVirtualPixmap -> drawable, &pWin -> drawable, 0, RegionRects(prgnRestore),
                       RegionNumRects(prgnRestore), -xorg, -yorg, FALSE, FALSE, 0, 0);

  values.subwindow_mode = ClipByChildren;

  gc = XCreateGC(nxagentDisplay, nxagentWindow(screenInfo.screens[0]->root), GCSubwindowMode, &values);

  /*
   * Translate the reference point to the origin of the window.
   */

  RegionTranslate(prgnRestore,
                       -pWin -> drawable.x - pWin -> borderWidth,
                           -pWin -> drawable.y - pWin -> borderWidth);

  clipRegion = prgnRestore;

  if (nxagentDrawableStatus((DrawablePtr) pPixmap) == NotSynchronized)
  {
    clipRegion = RegionCreate(NullBox, 1);

    RegionCopy(clipRegion,
                        nxagentCorruptedRegion((DrawablePtr) pPixmap));

    /*
     * Translate the reference point to the origin of the window.
     */

    RegionTranslate(clipRegion,
                         pBackingStore -> x, pBackingStore -> y);

    RegionIntersect(clipRegion, prgnRestore, clipRegion);

    /*
     * Subtract the corrupted region from the saved areas.
     * miBSRestoreAreas will return the exposure region.
     */

    RegionSubtract(&pBackingStore->SavedRegion,
                        &pBackingStore->SavedRegion, clipRegion);

    /*
     * Store the corrupted region to send expose later.
     */

    if (nxagentRemoteExposeRegion != NULL)
    {
      RegionTranslate(clipRegion, pWin -> drawable.x, pWin -> drawable.y);

      RegionUnion(nxagentRemoteExposeRegion, nxagentRemoteExposeRegion, clipRegion);

      RegionTranslate(clipRegion, -pWin -> drawable.x, -pWin -> drawable.y);
    }

    /*
     * Compute the region to be restored.
     */

    RegionSubtract(clipRegion, prgnRestore, clipRegion);
  }

  nRects = RegionNumRects(clipRegion);
  size = nRects * sizeof(*pRects);
  pRects = (XRectangle *) malloc(size);
  pBox = RegionRects(clipRegion);

  for (i = nRects; i-- > 0;)
  {
    pRects[i].x = pBox[i].x1;
    pRects[i].y = pBox[i].y1;
    pRects[i].width = pBox[i].x2 - pBox[i].x1;
    pRects[i].height = pBox[i].y2 - pBox[i].y1;
  }

  XSetClipRectangles(nxagentDisplay, gc, 0, 0, pRects, nRects, Unsorted);

  free(pRects);

  extents = *RegionExtents(clipRegion);

  xDst = extents.x1;
  yDst = extents.y1;

  xSrc = xDst - xorg + pWin -> drawable.x;
  ySrc = yDst - yorg + pWin -> drawable.y;

  w = extents.x2 - extents.x1;
  h = extents.y2 - extents.y1;

  nxagentFlushConfigureWindow();

  XCopyArea(nxagentDisplay, nxagentPixmap(pPixmap), nxagentWindow(pWin), gc,
                xSrc, ySrc, w, h, xDst, yDst);

  XFreeGC(nxagentDisplay, gc);

  if (clipRegion != NULL && clipRegion != prgnRestore)
  {
    RegionDestroy(clipRegion);
  }

  /*
   * Restore the reference point to the origin of the screen.
   */

  RegionTranslate(prgnRestore,
                       pWin -> drawable.x - pWin -> borderWidth,
                           pWin -> drawable.y + pWin -> borderWidth);

  return;
}

void nxagentSetWMNormalHints(int screen, int width, int height)
{
  XSizeHints* sizeHints = XAllocSizeHints();

  if (!sizeHints)
    return;

  /*
   * Change agent window size and size hints.
   */

  sizeHints->flags = PPosition | PMinSize | PMaxSize;
  sizeHints->x = nxagentOption(X);
  sizeHints->y = nxagentOption(Y);

  sizeHints->min_width = MIN_NXAGENT_WIDTH;
  sizeHints->min_height = MIN_NXAGENT_HEIGHT;

  sizeHints->width = width;
  sizeHints->height = height;

  if (nxagentOption(DesktopResize) == 1)
  {
    sizeHints->max_width = WidthOfScreen(DefaultScreenOfDisplay(nxagentDisplay));
    sizeHints->max_height = HeightOfScreen(DefaultScreenOfDisplay(nxagentDisplay));
  }
  else
  {
    sizeHints->max_width = nxagentOption(RootWidth);
    sizeHints->max_height = nxagentOption(RootHeight);
  }

  if (nxagentUserGeometry.flag & XValue || nxagentUserGeometry.flag & YValue)
  {
    sizeHints->flags |= USPosition;
  }

  if (nxagentUserGeometry.flag & WidthValue || nxagentUserGeometry.flag & HeightValue)
  {
    sizeHints->flags |= USSize;
  }

  XSetWMNormalHints(nxagentDisplay, nxagentDefaultWindows[screen], sizeHints);

  XFree(sizeHints);
}

/*
  set maxsize in WMNormalSizeHints
  Note: this will _drop_ all existing hints since XSetWMNormalHints()
  replaces any existing property
*/
void nxagentSetWMNormalHintsMaxsize(ScreenPtr pScreen, int maxwidth, int maxheight)
{
  XSizeHints* sizeHints;

  if ((sizeHints = XAllocSizeHints()))
  {
    sizeHints->flags = PMaxSize;
    sizeHints->max_width = maxwidth;
    sizeHints->max_height = maxheight;
    XSetWMNormalHints(nxagentDisplay, nxagentDefaultWindows[pScreen->myNum],
                      sizeHints);
    XFree(sizeHints);
  }
}

void nxagentShadowAdaptToRatio(void)
{
  ScreenPtr pScreen;
  RegionRec region;
  BoxRec box;

  pScreen = screenInfo.screens[0];

  nxagentShadowSetRatio(nxagentOption(Width) * 1.0 / nxagentShadowWidth,
                            nxagentOption(Height) * 1.0 / nxagentShadowHeight);

  nxagentShadowCreateMainWindow(pScreen, screenInfo.screens[0]->root, nxagentShadowWidth, nxagentShadowHeight);

  nxagentSetWMNormalHintsMaxsize(pScreen,
                                 WidthOfScreen(DefaultScreenOfDisplay(nxagentDisplay)),
                                 HeightOfScreen(DefaultScreenOfDisplay(nxagentDisplay)));

  box.x1 = 0;
  box.y1 = 0;
  box.x2 = nxagentShadowPixmapPtr -> drawable.width;
  box.y2 = nxagentShadowPixmapPtr -> drawable.height;

  RegionInit(&region, &box, 1);

  nxagentMarkCorruptedRegion((DrawablePtr)nxagentShadowPixmapPtr, &region);

  RegionUninit(&region);
}

void nxagentPrintGeometry(void)
{
  int i;

  for (i = 0; i < screenInfo.numScreens; i++)
  {
    if (nxagentPrintGeometryFlags && (1 << i))
    {
      fprintf(stderr, "Info: Screen [%d] resized to geometry [%dx%d] "
                  "fullscreen [%d].\n", i, screenInfo.screens[i] -> width,
                      screenInfo.screens[i] -> height,
                          nxagentOption(Fullscreen));
    }
  }

  nxagentPrintGeometryFlags = 0;
}

#ifdef DUMP

void nxagentShowPixmap(PixmapPtr pPixmap, int x, int y, int width, int height)
{
  static int init = 1;
  static Display *shadow;
  static Window win;

  XlibGC gc;
  XGCValues value;
  XImage *image;
  WindowPtr pWin = screenInfo.screens[0]->root;
  unsigned int format;
  int depth, pixmapWidth, pixmapHeight, length;
  char *data;

  depth = pPixmap -> drawable.depth;
  pixmapWidth = pPixmap -> drawable.width;
  pixmapHeight = pPixmap -> drawable.height;
  format = (depth == 1) ? XYPixmap : ZPixmap;

  if (init)
  {
    shadow = XOpenDisplay("localhost:0");

    if (shadow == NULL)
    {
      #ifdef WARNING
      fprintf(stderr, "nxagentShowPixmap: WARNING! Shadow display not opened.\n");
      #endif

      return;
    }

    init = False;
  }

  if (win == 0)
  {
    win = XCreateSimpleWindow(shadow, DefaultRootWindow(shadow), 0, 0,
                                    width, height, 0, 0xFFCC33, 0xFF);

    XSelectInput(shadow, win, StructureNotifyMask);

    XMapWindow(shadow, win);

    for(;;)
    {
      XEvent e;

      XNextEvent(shadow, &e);

      if (e.type == MapNotify)
      {
        break;
      }
    }
  }
  else
  {
    XResizeWindow(nxagentDisplay, win, width, height);
    XRaiseWindow(nxagentDisplay, win);
  }

  length = nxagentImageLength(width, height, format, 0, depth);

  if ((data = malloc(length)) == NULL)
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentShowPixmap: malloc failed.\n");
    #endif

    return;
  }

/*
FIXME
  image = XGetImage(nxagentDisplay, nxagentPixmap(pPixmap), x, y,
                        width, height, AllPlanes, format);
*/

  image = XGetImage(nxagentDisplay, RootWindow(nxagentDisplay, 0), 0, 0,
                        width, height, AllPlanes, format);

  if (image == NULL)
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentShowPixmap: XGetImage failed.\n");
    #endif

    free(data);

    return;
  }

  fbGetImage((DrawablePtr)pPixmap, 0, 0,
                 width, height, format, AllPlanes, data);

  memcpy(image -> data, data, length);

  value.foreground = 0xffffff;
  value.background = 0x000000;
  value.plane_mask = 0xffffff;
  value.fill_style = FillSolid;

  gc = XCreateGC(shadow, win, GCBackground |
                     GCForeground | GCFillStyle | GCPlaneMask, &value);

  XSync(shadow, 0);

  NXCleanImage(image);

  XPutImage(shadow, win, gc, image, 0, 0, 0, 0, width, height);

  XFlush(shadow);

  XFreeGC(shadow, gc);

  if (image != NULL)
  {
    XDestroyImage(image);
  }

  free(data);

/*
FIXME
  if (win != NULL)
  {
    XDestroyWindow(shadow, win);
  }
*/
}

void nxagentFbRestoreArea(PixmapPtr pPixmap, WindowPtr pWin, int xSrc, int ySrc, int width,
                              int height, int xDst, int yDst)
{
  Display *shadow;

  XlibGC gc;
  XGCValues value;
  XImage *image;
  unsigned int format;
  int depth, pixmapWidth, pixmapHeight, length;
  char *data = NULL;
  Visual *pVisual;

  depth = pPixmap -> drawable.depth;
  pixmapWidth = pPixmap -> drawable.width;
  pixmapHeight = pPixmap -> drawable.height;
  format = (depth == 1) ? XYPixmap : ZPixmap;

  shadow = nxagentDisplay;

  length = nxagentImageLength(width, height, format, 0, depth);

  if ((data = malloc(length)) == NULL)
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentFbRestoreArea: malloc failed.\n");
    #endif

    return;
  }
/*
  image = XGetImage(nxagentDisplay, nxagentPixmap(pPixmap), xSrc, ySrc,
                        width, height, AllPlanes, format);
*/

  if (image == NULL)
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentFbRestoreArea: XGetImage failed.\n");
    #endif

    free(data);

    return;
  }

  fbGetImage((DrawablePtr)pPixmap, xSrc, ySrc,
                 width, height, format, AllPlanes, data);

/*
FIXME
*/
  pVisual = nxagentImageVisual((DrawablePtr) pPixmap, depth);

  if (pVisual == NULL)
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentFbRestoreArea: WARNING! Visual not found. Using default visual.\n");
    #endif
    
    pVisual = nxagentVisuals[nxagentDefaultVisualIndex].visual;
  } 

  image = XCreateImage(nxagentDisplay, pVisual,
                                  depth, format, 0, (char *) data,
                                  width, height, BitmapPad(nxagentDisplay),
                                  nxagentImagePad(width, format, 0, depth));
/*
FIXME
  memcpy(image -> data, data, length);
*/

  fprintf(stderr, "nxagentFbRestoreArea: Cleaning %d bytes of image.\n", length);

  value.foreground = 0xffffff;
  value.background = 0x000000;
  value.plane_mask = 0xffffff;
  value.fill_style = FillSolid;
  value.function = GXcopy;

  gc = XCreateGC(shadow, nxagentWindow(screenInfo.screens[0]->root), GCBackground |
                     GCForeground | GCFillStyle | GCPlaneMask | GCFunction, &value);

  NXCleanImage(image);

  XPutImage(shadow, nxagentWindow(pWin), gc, image, 0, 0, xDst, yDst, width, height);

/*
FIXME
*/
  XFlush(shadow);

  XFreeGC(shadow, gc);

  if (image)
  {
    XDestroyImage(image);
  }

/*
FIXME
  free(data);
*/
}

#endif
