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
   * the order according to local endianess and swap
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
       We could avoid the sesssion window is iconified when a LeaveNotify event is received,
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
  XSizeHints sizeHints;
  XWMHints wmHints;
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
  sizeHints.flags = PMinSize | PMaxSize;
  sizeHints.min_width = sizeHints.max_width = 1;
  sizeHints.min_height = sizeHints.max_height = 1;
  wmHints.flags = IconPixmapHint | IconMaskHint;
  wmHints.initial_state = IconicState;
  wmHints.icon_pixmap = nxagentIconPixmap;

  if (useXpmIcon)
  {
    wmHints.icon_mask = nxagentIconShape;
    wmHints.flags = IconPixmapHint | IconMaskHint;
  }
  else
  {
    wmHints.flags = StateHint | IconPixmapHint;
  }

  XSetWMProperties(nxagentDisplay, w,
                      &windowName, &windowName,
                          NULL , 0 , &sizeHints, &wmHints, NULL);

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
  XSizeHints sizeHints;
  XWMHints wmHints;
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
      if(nxagentX2go)
      {
        #ifdef TEST
        fprintf(stderr, "nxagentOpenScreen: Setting WM_CLASS and WM_NAME for window with id [%ld].\n",
                (long int)nxagentDefaultWindows[pScreen->myNum]);
        #endif
        XClassHint hint;
        hint.res_name = strdup("X2GoAgent");
        hint.res_class = strdup("X2GoAgent");
        XSetClassHint(nxagentDisplay, nxagentDefaultWindows[pScreen->myNum], &hint);
        free(hint.res_name);
        free(hint.res_class);
      }
      else
      {
        #ifdef TEST
        fprintf(stderr, "nxagentOpenScreen: Setting WM_CLASS and WM_NAME for window with id [%ld].\n",
                (long int)nxagentDefaultWindows[pScreen->myNum]);
        #endif

        XClassHint hint;
        hint.res_name = strdup("NXAgent");
        hint.res_class = strdup("NXAgent");
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

    sizeHints.flags = PPosition | PMinSize | PMaxSize;
    sizeHints.x = nxagentOption(X) + POSITION_OFFSET;
    sizeHints.y = nxagentOption(Y) + POSITION_OFFSET;
    sizeHints.min_width = MIN_NXAGENT_WIDTH;
    sizeHints.min_height = MIN_NXAGENT_HEIGHT;

    sizeHints.width = nxagentOption(RootWidth);
    sizeHints.height = nxagentOption(RootHeight);

    if (nxagentOption(DesktopResize) == 1 || nxagentOption(Fullscreen) == 1)
    {
      sizeHints.max_width = WidthOfScreen(DefaultScreenOfDisplay(nxagentDisplay));
      sizeHints.max_height = HeightOfScreen(DefaultScreenOfDisplay(nxagentDisplay));
    }
    else
    {
      sizeHints.max_width = nxagentOption(RootWidth);
      sizeHints.max_height = nxagentOption(RootHeight);
    }

    if (nxagentUserGeometry.flag & XValue || nxagentUserGeometry.flag & YValue)
      sizeHints.flags |= USPosition;
    if (nxagentUserGeometry.flag & WidthValue || nxagentUserGeometry.flag & HeightValue)
      sizeHints.flags |= USSize;

    XSetStandardProperties(nxagentDisplay,
                           nxagentDefaultWindows[pScreen->myNum],
                           nxagentWindowName,
                           nxagentWindowName,
                           nxagentIconPixmap,
                           argv, argc, &sizeHints);

    wmHints.icon_pixmap = nxagentIconPixmap;

    if (useXpmIcon)
    {
      wmHints.icon_mask = nxagentIconShape;
      wmHints.flags = IconPixmapHint | IconMaskHint;
    }
    else
    {
      wmHints.flags = IconPixmapHint;
    }

    XSetWMHints(nxagentDisplay, nxagentDefaultWindows[pScreen->myNum], &wmHints);

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
  XSizeHints sizeHints;
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
    sizeHints.flags = PPosition | PMinSize | PMaxSize;
    sizeHints.x = nxagentOption(X);
    sizeHints.y = nxagentOption(Y);

    sizeHints.min_width = MIN_NXAGENT_WIDTH;
    sizeHints.min_height = MIN_NXAGENT_HEIGHT;
    sizeHints.width = width;
    sizeHints.height = height;

    if (nxagentOption(DesktopResize) == 1)
    {
      sizeHints.max_width = WidthOfScreen(DefaultScreenOfDisplay(nxagentDisplay));
      sizeHints.max_height = HeightOfScreen(DefaultScreenOfDisplay(nxagentDisplay));
    }
    else
    {
      sizeHints.max_width = nxagentOption(RootWidth);
      sizeHints.max_height = nxagentOption(RootHeight);
    }

    if (nxagentUserGeometry.flag & XValue || nxagentUserGeometry.flag & YValue)
    {
      sizeHints.flags |= USPosition;
    }

    if (nxagentUserGeometry.flag & WidthValue || nxagentUserGeometry.flag & HeightValue)
    {
      sizeHints.flags |= USSize;
    }

    XSetWMNormalHints(nxagentDisplay, nxagentDefaultWindows[pScreen->myNum], &sizeHints);

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
   * Update void * bounds.
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

    nxagentSetWMNormalHints(0);
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
    *x = ix;
    *y = iy;
    *w = iw;
    *h = ih;

    #ifdef DEBUG
    fprintf(stderr, "intersect: intersection is: ([%d],[%d]) [ %d x %d ]\n", *x, *y, *w, *h);
    #endif

    return TRUE;
}

#ifndef NXAGENT_RANDR_XINERAMA_CLIPPING
/* intersect two rectangles, return aw/ah for w/h if resulting
   rectangle is (partly) outside of bounding box */
Bool intersect_bb(int ax1, int ay1, unsigned int aw, unsigned int ah,
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

int nxagentChangeScreenConfig(int screen, int width, int height, int mmWidth, int mmHeight)
{
  ScreenPtr    pScreen;
  int          r;

  #ifdef DEBUG
  fprintf(stderr, "nxagentChangeScreenConfig: called for screen [%d], width [%d] height [%d] mmWidth [%d] mmHeight [%d]\n", screen, width, height, mmWidth, mmHeight);
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
  fprintf(stderr, "nxagentChangeScreenConfig: Changing config to %d x %d (%dmm x %dmm).\n", width, height, mmWidth, mmHeight);
  #endif

  r = nxagentResizeScreen(pScreen, width, height, mmWidth, mmHeight);

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

int nxagentAdjustRandRXinerama(ScreenPtr pScreen)
{
  rrScrPrivPtr pScrPriv;
  RROutputPtr  output;
  xRRModeInfo  modeInfo;
  char         name[100];
  int          refresh = 60;
  int          width = nxagentOption(Width);
  int          height = nxagentOption(Height);

  pScrPriv = rrGetScrPriv(pScreen);

  if (pScrPriv)
  {
    int i;
    int number = 0;

    XineramaScreenInfo *screeninfo = NULL;

    screeninfo = XineramaQueryScreens(nxagentDisplay, &number);
    if (number)
    {
#ifdef DEBUG
      fprintf(stderr, "nxagentAdjustRandRXinerama: XineramaQueryScreens() returned [%d] screens:\n", number);
      for (int i=0; i < number; i++) {
	fprintf(stderr, "nxagentAdjustRandRXinerama:   screen_number [%d] x_org [%d] y_org [%d] width [%d] height [%d]\n", screeninfo[i].screen_number, screeninfo[i].x_org, screeninfo[i].y_org, screeninfo[i].width, screeninfo[i].height);
      }
#endif
    }
    else
    {
#ifdef DEBUG
      fprintf(stderr, "nxagentAdjustRandRXinerama: XineramaQueryScreens() failed - continuing without Xinerama\n");
#endif
    }

    /*
     * if there's no xinerama on the real server or xinerama is
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

      free(screeninfo);

      if (!(screeninfo = malloc(sizeof(XineramaScreenInfo)))) {
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
       * do this here it will be done implicitely later and add mode(s) to
       * our crtc(s)!
       */
      rrgetinfo = RRGetInfo(pScreen, FALSE);

      fprintf(stderr, "nxagentAdjustRandRXinerama: RRGetInfo returned [%d]\n", rrgetinfo);
    }
#else
    /* we are not interested in the return code */
    RRGetInfo(pScreen, FALSE);
#endif

#ifndef NXAGENT_RANDR_XINERAMA_CLIPPING
    /* calculate bounding box (outer edges) */
    int bbx2, bbx1, bby1, bby2;
    bbx2 = bby2 = 0;
    bbx1 = bby1 = INT_MAX;

    for (i = 0; i < number; i++) {
      bbx2 = MAX(bbx2, screeninfo[i].x_org + screeninfo[i].width);
      bby2 = MAX(bby2, screeninfo[i].y_org + screeninfo[i].height);
      bbx1 = MIN(bbx1, screeninfo[i].x_org);
      bby1 = MIN(bby1, screeninfo[i].y_org);
    }
    #ifdef DEBUG
    fprintf(stderr, "nxagentAdjustRandRXinerama: bounding box: left [%d] right [%d] top [%d] bottom [%d]\n", bbx1, bbx2, bby1, bby2);
    #endif
#endif

    #ifdef DEBUG
    fprintf(stderr, "nxagentAdjustRandRXinerama: numCrtcs [%d], numOutputs [%d]\n", pScrPriv->numCrtcs, pScrPriv->numOutputs);
    #endif

    /* adjust the number of CRTCs to match the number of reported
       xinerama screens on the real server */
    while (number != pScrPriv->numCrtcs) {
      if (number < pScrPriv->numCrtcs) {
        #ifdef DEBUG
        fprintf(stderr, "nxagentAdjustRandRXinerama: destroying crtc\n");
        #endif
        /* first reset the crtc to free possible outputs, then destroy the crtc */
        RRCrtcSet(pScrPriv->crtcs[pScrPriv->numCrtcs - 1], NULL, 0, 0, RR_Rotate_0, 0, NULL);
        RRCrtcDestroy(pScrPriv->crtcs[pScrPriv->numCrtcs - 1]);
      }
      else
      {
        #ifdef DEBUG
        fprintf(stderr, "nxagentAdjustRandRXinerama: adding crtc\n");
        #endif
        RRCrtcCreate(pScreen, NULL);
      }
    }

    #ifdef DEBUG
    fprintf(stderr, "nxagentAdjustRandRXinerama: numCrtcs [%d], numOutputs [%d]\n", pScrPriv->numCrtcs, pScrPriv->numOutputs);
    #endif

    /* set gamma. Currently the only reason for doing this is
       preventing the xrandr command from complaining about missing
       gamma. */
    for (i = 0; i < pScrPriv->numCrtcs; i++) {
      if (pScrPriv->crtcs[i]->gammaSize == 0) {
	CARD16 gamma = 0;
	RRCrtcGammaSetSize(pScrPriv->crtcs[i], 1);
	RRCrtcGammaSet(pScrPriv->crtcs[i], &gamma, &gamma, &gamma);
	RRCrtcGammaNotify(pScrPriv->crtcs[i]);
      }
    }

    /* delete superfluous non-NX outputs */
    for (i = pScrPriv->numOutputs - 1; i >= 0; i--)
      if (strncmp(pScrPriv->outputs[i]->name, "NX", 2))
        nxagentDropOutput(pScrPriv->outputs[i]);

    /* at this stage only NX outputs are left - we delete the superfluous ones */
    for (i = pScrPriv->numOutputs - 1; i >= number; i--)
      nxagentDropOutput(pScrPriv->outputs[i]);

    /* add and init outputs */
    for (i = 0; i < number; i++) {
      if (i >= pScrPriv->numOutputs) {
        sprintf(name, "NX%d", i+1);
        output = RROutputCreate(pScreen, name, strlen(name), NULL);
	/* will be done later
	RROutputSetConnection(output, RR_Disconnected); 
	*/
        #ifdef DEBUG
        fprintf(stderr, "nxagentAdjustRandRXinerama: created new output [%s]\n", name);
        #endif
      }
      else
      {
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

    for (i = 0; i < pScrPriv->numOutputs; i++) {
      Bool disable_output = FALSE;
      RRModePtr mymode, prevmode;
      int new_x = 0;
      int new_y = 0;
      unsigned int new_w = 0;
      unsigned int new_h = 0;

      /*
      if ((nxagentOption(X) < bbx1 || (nxagentOption(X) + width >= bbx2 )) {
        #ifdef DEBUG
        fprintf(stderr, "nxagentAdjustRandRXinerama: output [%d] name [%s]: window has parts outside visible area - width stays unchanged [%d]\n", i, pScrPriv->outputs[i]->name, width);
        #endif
	new_w = width;
      }

	if ((nxagentOption(Y) < bby1 || (nxagentOption(Y) + height >= bby2 ) {
          #ifdef DEBUG
          fprintf(stderr, "nxagentAdjustRandRXinerama: output [%d] name [%s]: window has parts outside visible area - height stays unchanged [%d]\n", i, pScrPriv->outputs[i]->name, height);
          #endif
	  new_h = height;
	}
      */

      /* if there's no intersection disconnect the output */
#ifdef NXAGENT_RANDR_XINERAMA_CLIPPING
      disable_output = !intersect(nxagentOption(X), nxagentOption(Y),
                                  width, height,
                                  screeninfo[i].x_org, screeninfo[i].y_org,
                                  screeninfo[i].width, screeninfo[i].height,
                                  &new_x, &new_y, &new_w, &new_h);
#else
      disable_output = !intersect_bb(nxagentOption(X), nxagentOption(Y),
                                  width, height,
                                  screeninfo[i].x_org, screeninfo[i].y_org,
                                  screeninfo[i].width, screeninfo[i].height,
	                          bbx1, bby1, bbx2, bby2,
                                  &new_x, &new_y, &new_w, &new_h);
#endif

      /* save previous mode */
      prevmode = pScrPriv->crtcs[i]->mode;
      #ifdef DEBUG
      if (prevmode) {
        fprintf(stderr, "nxagentAdjustRandRXinerama: output [%d] name [%s]: prevmode [%s] ([%p]) refcnt [%d]\n", i, pScrPriv->outputs[i]->name, prevmode->name, (void *)prevmode, prevmode->refcnt);
      } else {
        fprintf(stderr, "nxagentAdjustRandRXinerama: output [%d] name [%s]: no prevmode\n", i, pScrPriv->outputs[i]->name);
      }
      #endif

      RROutputSetCrtcs(pScrPriv->outputs[i], &(pScrPriv->crtcs[i]), 1);

      if (disable_output) {
        #ifdef DEBUG
        fprintf(stderr, "nxagentAdjustRandRXinerama: output [%d] name [%s]: no (valid) intersection - disconnecting\n", i, pScrPriv->outputs[i]->name);
        #endif
        RROutputSetConnection(pScrPriv->outputs[i], RR_Disconnected);

        /*
         * Tests revealed that some window managers (e.g. LXDE) also
         * take disconnected outputs into account when calculating
         * stuff like wallpaper tile size and maximum window
         * size. This is problematic when a disconnected output is
         * smaller than any of the connected ones. Solution: unset the
         * mode of the output's crtc. This also leads to xinerama not
         * showing the disconnected head anymore.
         */
        if (prevmode) {
          #ifdef DEBUG
          fprintf(stderr, "nxagentAdjustRandRXinerama: removing mode from output [%d] name [%s]\n", i, pScrPriv->outputs[i]->name);
          #endif
          RROutputSetModes(pScrPriv->outputs[i], NULL, 0, 0);

          #ifdef DEBUG
          fprintf(stderr, "nxagentAdjustRandRXinerama: removing mode from ctrc [%d]\n", i);
          #endif
          RRCrtcSet(pScrPriv->crtcs[i], NULL, 0, 0, RR_Rotate_0, 1, &(pScrPriv->outputs[i]));
        }
      }
      else
      {
        #ifdef DEBUG
        fprintf(stderr, "nxagentAdjustRandRXinerama: output [%d] name [%s]: intersection is x [%d] y [%d] width [%d] height [%d]\n", i, pScrPriv->outputs[i]->name, new_x, new_y, new_w, new_h);
        #endif

        RROutputSetConnection(pScrPriv->outputs[i], RR_Connected);

        memset(&modeInfo, '\0', sizeof(modeInfo));

#ifdef NXAGENT_RANDR_MODE_PREFIX
        /* avoid collisions with pre-existing default modes by using a
           separate namespace. If we'd simply use XxY we could not
           distinguish between pre-existing modes which should stay
           and our own modes that should be removed after use. */
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
        if (mymode) {
          fprintf(stderr, "nxagentAdjustRandRXinerama: output [%d] name [%s]: mode [%s] ([%p]) created/received, refcnt [%d]\n", i, pScrPriv->outputs[i]->name, name, (void *) mymode, mymode->refcnt);
        }
        else
        {
	  /* FIXME: what is the correct behaviour in this case? */
          fprintf(stderr, "nxagentAdjustRandRXinerama: output [%d] name [%s]: mode [%s] creation failed!\n", i, pScrPriv->outputs[i]->name, name);
        }
#endif
	if (prevmode && mymode == prevmode) {
          #ifdef DEBUG
          fprintf(stderr, "nxagentAdjustRandRXinerama: mymode [%s] ([%p]) == prevmode [%s] ([%p])\n", mymode->name, (void *) mymode, prevmode->name, (void *)prevmode);
          #endif

          /* if they are the same RRModeGet() has increased the
	     refcnt by 1. We decrease it again by calling only
	     RRModeDestroy() and forget about prevmode */
	  RRModeDestroy(mymode);
        }
        else
	{ 
          #ifdef DEBUG
          fprintf(stderr, "nxagentAdjustRandRXinerama: setting mode [%s] ([%p]) refcnt [%d] for output %d [%s]\n", mymode->name, (void *) mymode, mymode->refcnt, i, pScrPriv->outputs[i]->name);
          #endif
          RROutputSetModes(pScrPriv->outputs[i], &mymode, 1, 0);
        }
      } /* if disable_output */

      #ifdef DEBUG
      fprintf(stderr, "nxagentAdjustRandRXinerama: setting mode [%s] ([%p]) refcnt [%d] for crtc %d\n", mymode->name, (void *) mymode, mymode->refcnt, i);
      #endif
      RRCrtcSet(pScrPriv->crtcs[i], mymode, new_x, new_y, RR_Rotate_0, 1, &(pScrPriv->outputs[i]));

      /* throw away the mode if otherwise unused. We do not need it
         anymore. We call FreeResource() to ensure the system will not
         try to free it again on shutdown */
      if (prevmode && prevmode->refcnt == 1) {
        #ifdef DEBUG
        fprintf(stderr, "nxagentAdjustRandRXinerama: destroying prevmode [%s]\n", prevmode->name);
        #endif
	FreeResource(prevmode->mode.id, 0);
      }

      RROutputChanged(pScrPriv->outputs[i], TRUE);
      RRCrtcChanged(pScrPriv->crtcs[i], TRUE);
    }

    /* release allocated memory */
    free(screeninfo);
    screeninfo = NULL;

#ifdef DEBUG
    for (i = 0; i < pScrPriv->numCrtcs; i++) {
      RRModePtr mode = pScrPriv->crtcs[i]->mode;
      if (mode) {
	fprintf(stderr, "nxagentAdjustRandRXinerama: crtc [%d] ([%p]) has mode [%s] ([%p]), refcnt [%d] and [%d] outputs:\n", i, (void *) pScrPriv->crtcs[i], pScrPriv->crtcs[i]->mode->name, (void *)pScrPriv->crtcs[i]->mode, pScrPriv->crtcs[i]->mode->refcnt, pScrPriv->crtcs[i]->numOutputs);
      }
      else
      {
	fprintf(stderr, "nxagentAdjustRandRXinerama: crtc [%d] ([%p]) has no mode and [%d] outputs:\n", i, (void *) pScrPriv->crtcs[i], pScrPriv->crtcs[i]->numOutputs);
      }

      if (pScrPriv->crtcs[i]->numOutputs > 0)
        for (int j=0; j < pScrPriv->crtcs[i]->numOutputs; j++)
	  fprintf(stderr, "nxagentAdjustRandRXinerama:   output [%d] name [%s]->crtc=[%p]\n", j, pScrPriv->crtcs[i]->outputs[j]->name, (void *)pScrPriv->crtcs[i]->outputs[j]->crtc);
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

void nxagentSetWMNormalHints(int screen)
{
  XSizeHints sizeHints;

  /*
   * Change agent window size and size hints.
   */

  sizeHints.flags = PPosition | PMinSize | PMaxSize;
  sizeHints.x = nxagentOption(X);
  sizeHints.y = nxagentOption(Y);

  sizeHints.min_width = MIN_NXAGENT_WIDTH;
  sizeHints.min_height = MIN_NXAGENT_HEIGHT;

  sizeHints.width = nxagentOption(Width);
  sizeHints.height = nxagentOption(Height);

  if (nxagentOption(DesktopResize) == 1)
  {
    sizeHints.max_width = WidthOfScreen(DefaultScreenOfDisplay(nxagentDisplay));
    sizeHints.max_height = HeightOfScreen(DefaultScreenOfDisplay(nxagentDisplay));
  }
  else
  {
    sizeHints.max_width = nxagentOption(RootWidth);
    sizeHints.max_height = nxagentOption(RootHeight);
  }

  if (nxagentUserGeometry.flag & XValue || nxagentUserGeometry.flag & YValue)
  {
    sizeHints.flags |= USPosition;
  }

  if (nxagentUserGeometry.flag & WidthValue || nxagentUserGeometry.flag & HeightValue)
  {
    sizeHints.flags |= USSize;
  }

  XSetWMNormalHints(nxagentDisplay, nxagentDefaultWindows[screen], &sizeHints);
}

void nxagentShadowAdaptToRatio(void)
{
  XSizeHints sizeHints;
  ScreenPtr pScreen;
  RegionRec region;
  BoxRec box;

  pScreen = screenInfo.screens[0];

  nxagentShadowSetRatio(nxagentOption(Width) * 1.0 / nxagentShadowWidth,
                            nxagentOption(Height) * 1.0 / nxagentShadowHeight);

  nxagentShadowCreateMainWindow(pScreen, screenInfo.screens[0]->root, nxagentShadowWidth, nxagentShadowHeight);

  sizeHints.max_width = WidthOfScreen(DefaultScreenOfDisplay(nxagentDisplay));
  sizeHints.max_height = HeightOfScreen(DefaultScreenOfDisplay(nxagentDisplay));

  sizeHints.flags = PMaxSize;

  XSetWMNormalHints(nxagentDisplay, nxagentDefaultWindows[pScreen->myNum], &sizeHints);

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
