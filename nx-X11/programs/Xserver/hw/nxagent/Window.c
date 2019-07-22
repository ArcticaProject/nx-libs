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

#include <unistd.h>

#include "X.h"
#include "Xproto.h"
#include "gcstruct.h"
#include "../../include/window.h"
#include "windowstr.h"
#include "pixmapstr.h"
#include "colormapst.h"
#include "scrnintstr.h"
#include "region.h"
#include "dixstruct.h"
#include "selection.h"
#include "mi.h"
#include "fb.h"
#include "mibstorest.h"

#include "Agent.h"
#include "Display.h"
#include "Screen.h"
#include "GCs.h"
#include "GCOps.h"
#include "Drawable.h"
#include "Colormap.h"
#include "Cursor.h"
#include "Visual.h"
#include "Events.h"
#include "Clipboard.h"
#include "Args.h"
#include "Trap.h"
#include "Rootless.h"
#include "Atoms.h"
#include "Client.h"
#include "Reconnect.h"
#include "Dialog.h"
#include "Splash.h"
#include "Holder.h"
#include "Init.h"
#include "Composite.h"
#include "Events.h"
#include "Utils.h"

#include <nx/NX.h>
#include "compext/Compext.h"

#include "Xatom.h"

/*
 * Used to register the window's privates.
 */

int nxagentWindowPrivateIndex;

/*
 * Number of windows which need synchronization.
 */

int nxagentCorruptedWindows;

/*
 * Used to track nxagent window's visibility.
 */

int nxagentVisibility = VisibilityUnobscured;
unsigned long nxagentVisibilityTimeout = 0;
Bool nxagentVisibilityStop = False;

/*
 * Set here the required log level.
 */

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

/*
 * Useful to test the window configuration
 * failures.
 */

#ifdef TEST
#define MAKE_SYNC_CONFIGURE_WINDOW  XSync(nxagentDisplay, 0)
#else
#define MAKE_SYNC_CONFIGURE_WINDOW
#endif

extern WindowPtr nxagentViewportFrameLeft;
extern WindowPtr nxagentViewportFrameRight;
extern WindowPtr nxagentViewportFrameAbove;
extern WindowPtr nxagentViewportFrameBelow;

extern WindowPtr nxagentRootTileWindow;

extern Bool nxagentReportPrivateWindowIds;

/*
 * Also referenced in Events.c.
 */

int nxagentSplashCount = 0;

#define RECTLIMIT 25
#define BSPIXMAPLIMIT 128

Bool nxagentExposeArrayIsInitialized = False;
Window nxagentConfiguredSynchroWindow;
static int nxagentExposeSerial = 0;

StoringPixmapPtr nxagentBSPixmapList[BSPIXMAPLIMIT];

/*
 * Used to walk through the window hierarchy
 * to find a window
 */

typedef struct _WindowMatch
{
  WindowPtr pWin;
  Window    id;
} WindowMatchRec;

Bool nxagentReconnectAllWindows(void *);
Bool nxagentDisconnectAllWindows(void);
Bool nxagentIsIconic(WindowPtr);

/*
 * From NXproperty.c.
 */

int GetWindowProperty(WindowPtr, Atom, long, long, Bool, Atom, Atom*, int*,
                                 unsigned long*, unsigned long*, unsigned char**);

/*
 * From NXwindow.c.
 */

void nxagentClearSplash(WindowPtr pWin);

/*
 * Other local functions.
 */

static Bool nxagentSomeWindowsAreMapped(void);
static void nxagentFrameBufferPaintWindow(WindowPtr pWin, RegionPtr pRegion, int what);
static void nxagentTraverseWindow(WindowPtr, void(*)(void *, XID, void *), void *);
static void nxagentDisconnectWindow(void *, XID, void *);
static Bool nxagentLoopOverWindows(void(*)(void *, XID, void *));
static void nxagentReconfigureWindowCursor(void *, XID, void *);
static void nxagentReconnectWindow(void *, XID, void *);
static void nxagentReconfigureWindow(void *, XID, void *);
static int nxagentForceExposure(WindowPtr pWin, void * ptr);

/* by dimbor */
typedef struct
{
  CARD32 state;
  Window icon;
}
nxagentWMStateRec;

/*
 * This is currently unused.
 */

#ifdef TEST
static Bool nxagentCheckWindowIntegrity(WindowPtr pWin);
#endif

WindowPtr nxagentGetWindowFromID(Window id)
{
  WindowPtr pWin = screenInfo.screens[0]->root;

  while (pWin && nxagentWindowPriv(pWin))
  {
    if (nxagentWindow(pWin) == id)
    {
      return pWin;
    }

    if (pWin -> nextSib)
    {
      pWin = pWin -> nextSib;
    }
    else
    {
      pWin = pWin -> firstChild;
    }
  }

  return NULL;
}

static int nxagentFindWindowMatch(WindowPtr pWin, void * ptr)
{
  WindowMatchRec *match = (WindowMatchRec *) ptr;

  if (match -> id == nxagentWindow(pWin))
  {
    match -> pWin = pWin;
    return WT_STOPWALKING;
  }
  else
  {
    return WT_WALKCHILDREN;
  }
}

WindowPtr nxagentWindowPtr(Window window)
{
  WindowMatchRec match = {.pWin = NullWindow, .id   = window};

  for (int i = 0; i < nxagentNumScreens; i++)
  {
    WalkTree(screenInfo.screens[i], nxagentFindWindowMatch, (void *) &match);

    if (match.pWin)
    {
      break;
    }
  }

  return match.pWin;
}

/*
 * from "Definition of the Porting Layer for X v11 Sample Server":
 *
 * This routine is a hook for when DIX creates a window. It should
 * fill in the "Window Procedures in the WindowRec" below and also
 * allocate the devPrivate block for it.
 *
 * See Xserver/fb/fbwindow.c for the sample server implementation.
 */
Bool nxagentCreateWindow(WindowPtr pWin)
{
  unsigned long mask;
  XSetWindowAttributes attributes;
  Visual *visual;
  ColormapPtr pCmap;

  if (nxagentScreenTrap)
  {
    return True;
  }

  nxagentSplashCount++;

  if (nxagentSplashCount == 2)
  {
      nxagentClearSplash(nxagentRootTileWindow);
  }
  #ifdef NXAGENT_LOGO_DEBUG
  fprintf(stderr, "nxagentCreateWindow: nxagentSplashCount [%d]\n", nxagentSplashCount);
  #endif

  if (pWin->drawable.class == InputOnly)
  {
    mask = CWEventMask;
    visual = CopyFromParent;
  }
  else
  {
    mask = CWEventMask | CWBackingStore;

    if (pWin->optional)
    {
      mask |= CWBackingPlanes | CWBackingPixel;
      attributes.backing_planes = pWin->optional->backingBitPlanes;
      attributes.backing_pixel = pWin->optional->backingPixel;
    }

    attributes.backing_store = NotUseful;

    #ifdef TEST
    fprintf(stderr, "nxagentCreateWindow: Backing store on window at [%p] is [%d].\n",
                (void*)pWin, attributes.backing_store);
    #endif

    /*
      FIXME: We need to set save under on the real display?
    */
    if (nxagentSaveUnder)
    {
      mask |= CWSaveUnder;
      attributes.save_under = False;
    }

    if (pWin->parent)
    {
      if (pWin->optional && pWin->optional->visual != wVisual(pWin->parent))
      {
        visual = nxagentVisualFromID(pWin->drawable.pScreen, wVisual(pWin));
        mask |= CWColormap;
        if (pWin->optional->colormap)
        {
          pCmap = (ColormapPtr)LookupIDByType(wColormap(pWin), RT_COLORMAP);
          attributes.colormap = nxagentColormap(pCmap);
        }
        else
        {
          attributes.colormap = nxagentDefaultVisualColormap(visual);
        }
      }
      else if (pWin->optional)
      {
        visual = CopyFromParent;
      }
      else
      {
        visual = nxagentVisualFromID(pWin->drawable.pScreen, wVisual(pWin));
        mask |= CWColormap;
        attributes.colormap = nxagentDefaultVisualColormap(visual);
      }
    }
    else
    {
      /* root windows have their own colormaps at creation time */
      visual = nxagentVisualFromID(pWin->drawable.pScreen, wVisual(pWin));
      pCmap = (ColormapPtr)LookupIDByType(wColormap(pWin), RT_COLORMAP);
      mask |= CWColormap;
      attributes.colormap = nxagentColormap(pCmap);
    }
  }

  if (mask & CWEventMask)
  {
    attributes.event_mask = nxagentGetEventMask(pWin);
  }
  #ifdef WARNING
  else
  {
    attributes.event_mask = NoEventMask;
  }
  #endif

  /*
   * Select the event mask if window is a top level
   * window. This at least makes the keyboard barely
   * work.
   */

  #ifdef TEST
  fprintf(stderr, "nxagentCreateWindow: Going to create new window.\n");
  #endif

  #ifdef TEST
  fprintf(stderr, "nxagentCreateWindow: Creating %swindow at %p current event mask = %lX mask & CWEventMask = %ld "
              "event_mask = %lX\n",
                  nxagentWindowTopLevel(pWin) ? "toplevel " : "", (void*)pWin, pWin -> eventMask,
                      mask & CWEventMask, attributes.event_mask);

  fprintf(stderr, "nxagentCreateWindow: position [%d,%d] size [%d,%d] depth [%d] border [%d] class [%d].\n",
              pWin->origin.x - wBorderWidth(pWin), pWin->origin.y - wBorderWidth(pWin),
                  pWin->drawable.width, pWin->drawable.height, pWin->drawable.depth, pWin->borderWidth,
                      pWin->drawable.class);
  #endif

  nxagentWindowPriv(pWin)->window = XCreateWindow(nxagentDisplay,
                                                  nxagentWindowParent(pWin),
                                                  pWin->origin.x -
                                                  wBorderWidth(pWin),
                                                  pWin->origin.y -
                                                  wBorderWidth(pWin),
                                                  pWin->drawable.width,
                                                  pWin->drawable.height,
                                                  pWin->borderWidth,
                                                  pWin->drawable.depth,
                                                  pWin->drawable.class,
                                                  visual,
                                                  mask, &attributes);

  nxagentWindowPriv(pWin) -> isMapped = 0;
  nxagentWindowPriv(pWin) -> isRedirected = 0;
  nxagentWindowPriv(pWin) -> visibilityState = VisibilityUnobscured;
  nxagentWindowPriv(pWin) -> corruptedRegion = RegionCreate(NULL, 1);
  nxagentWindowPriv(pWin) -> hasTransparentChildren = 0;
  nxagentWindowPriv(pWin) -> containGlyphs = 0;
  nxagentWindowPriv(pWin) -> corruptedId = 0;
  nxagentWindowPriv(pWin) -> deferredBackgroundExpose = 0;
  nxagentWindowPriv(pWin) -> synchronizationBitmap = NullPixmap;
  nxagentWindowPriv(pWin) -> corruptedTimestamp = 0;
  nxagentWindowPriv(pWin) -> splitResource = NULL;

  if (nxagentOption(Rootless) == 1)
  {
    if (pWin != nxagentRootlessWindow)
    {
      WindowPtr pParent = pWin -> parent;

      if (pParent && nxagentWindowPriv(pParent) -> isMapped == 1)
      {
        nxagentWindowPriv(pWin) -> isMapped = 1;
      }
      else
      {
        nxagentWindowPriv(pWin) -> isMapped = 0;
      }
    }
    else
    {
      nxagentWindowPriv(pWin) -> isMapped = 0;
    }
  }

  if (nxagentReportPrivateWindowIds)
  {
    fprintf (stderr, "NXAGENT_WINDOW_ID: PRIVATE_WINDOW,WID:[0x%x]\n", nxagentWindowPriv(pWin)->window);
  }
  #ifdef TEST
  fprintf(stderr, "nxagentCreateWindow: Created new window with id [0x%x].\n",
              nxagentWindowPriv(pWin)->window);
  #endif

  /*
   * Set the WM_DELETE_WINDOW protocols on every
   * top level window. Also redirect the window
   * if it is a top level.
   */

  if (nxagentOption(Rootless) && nxagentWindowTopLevel(pWin))
  {
    Atom prop = nxagentMakeAtom("WM_PROTOCOLS", strlen("WM_PROTOCOLS"), True);
    XlibAtom atom = nxagentMakeAtom("WM_DELETE_WINDOW", strlen("WM_DELETE_WINDOW"), True);

    XSetWMProtocols(nxagentDisplay, nxagentWindowPriv(pWin)->window, &atom, 1);

    nxagentAddPropertyToList(prop, pWin);

    /*
     * Redirect the window to the off-screen
     * memory, if the composite extension is
     * supported on the display.
     */
    /*
      FIXME: Do all the windows for which nxagentWindowTopLevel(pWin)
             returns true need to be redirected?
    */
    nxagentRedirectWindow(pWin);
  }

  if ((nxagentRealWindowProp) && (!nxagentWindowTopLevel(pWin)))
  {
    Atom prop = MakeAtom("NX_REAL_WINDOW", strlen("NX_REAL_WINDOW"), True);

    if (ChangeWindowProperty(pWin, prop, XA_WINDOW, 32, PropModeReplace, 1, nxagentWindowPriv(pWin), 1) != Success)
    {
      fprintf(stderr, "nxagentCreateWindow: Adding NX_REAL_WINDOW failed.\n");
    }
    #ifdef DEBUG
    else
    {
      fprintf(stderr, "nxagentCreateWindow: Added NX_REAL_WINDOW for Window ID [%x].\n", nxagentWindowPriv(pWin)->window);
    }
    #endif
  }

  nxagentWindowPriv(pWin)->x = pWin->origin.x - wBorderWidth(pWin);
  nxagentWindowPriv(pWin)->y = pWin->origin.y - wBorderWidth(pWin);
  nxagentWindowPriv(pWin)->width = pWin->drawable.width;
  nxagentWindowPriv(pWin)->height = pWin->drawable.height;
  nxagentWindowPriv(pWin)->borderWidth = pWin->borderWidth;
  nxagentWindowPriv(pWin)->siblingAbove = None;
  nxagentWindowPriv(pWin)->pPicture = NULL;

  if (nxagentRootTileWindow)
  {
    if (nxagentWindowPriv(pWin)->window != nxagentWindowPriv(nxagentRootTileWindow)->window)
    {
      XClearWindow(nxagentDisplay, nxagentWindowPriv(nxagentRootTileWindow)->window);
    }
  }

  if (pWin->nextSib)
  {
    nxagentWindowPriv(pWin->nextSib)->siblingAbove = nxagentWindow(pWin);
  }

  #ifdef SHAPE
    #ifdef NXAGENT_SHAPE2
      nxagentWindowPriv(pWin)->boundingShape = NULL;
      nxagentWindowPriv(pWin)->clipShape = NULL;
    #else
      nxagentWindowPriv(pWin)->boundingShape = RegionCreate(NULL, 1);
      nxagentWindowPriv(pWin)->clipShape = RegionCreate(NULL, 1);
    #endif
  #endif /* SHAPE */

  fbCreateWindow(pWin);

  /*
   * Only the root window will have
   * the right colormap.
   */

  if (!pWin->parent)
  {
    nxagentSetInstalledColormapWindows(pWin->drawable.pScreen);
  }

  return True;
}

/* set the NX_AGENT_VERSION property for the given window (normally
   the root window) */
void nxagentSetVersionProperty(WindowPtr pWin)
{
  char *name = "NX_AGENT_VERSION";
  Atom prop = MakeAtom(name, strlen(name), True);

  if (ChangeWindowProperty(pWin, prop, XA_STRING, 8, PropModeReplace, strlen(NX_VERSION_CURRENT_STRING), NX_VERSION_CURRENT_STRING, True) != Success)
  {
    fprintf(stderr, "%s: Adding property [%s], value [%s] failed.\n", __func__, name, NX_VERSION_CURRENT_STRING);
  }
  #ifdef DEBUG
  else
  {
    fprintf(stderr, "%s: Added property [%s], value [%s] for root window [%x].\n", __func__, name, NX_VERSION_CURRENT_STRING, pWin);
  }
  #endif
}

Bool nxagentSomeWindowsAreMapped(void)
{
  WindowPtr pWin = screenInfo.screens[0]->root -> firstChild;

  while (pWin)
  {
    if ((pWin -> mapped || nxagentIsIconic(pWin)) &&
            pWin -> drawable.class == InputOutput)
    {
      return True;
    }

    pWin = pWin -> nextSib;
  }

  return False;
}

/*
 * from "Definition of the Porting Layer for X v11 Sample Server":
 *
 * This routine is a hook for when DIX destroys a window. It should
 * deallocate the devPrivate block for it and any other blocks that
 * need to be freed, besides doing other cleanup actions.
 *
 * See Xserver/fb/fbwindow.c for the sample server implementation.
 */
Bool nxagentDestroyWindow(WindowPtr pWin)
{
  nxagentPrivWindowPtr pWindowPriv;

  if (nxagentScreenTrap == 1)
  {
    return 1;
  }

  nxagentClearClipboard(NULL, pWin);

  for (int j = 0; j < nxagentExposeQueue.length; j++)
  {
    int i = (nxagentExposeQueue.start + j) % EXPOSED_SIZE;

    if (nxagentExposeQueue.exposures[i].pWindow == pWin)
    {
      if (nxagentExposeQueue.exposures[i].localRegion != NullRegion)
      {
        RegionDestroy(nxagentExposeQueue.exposures[i].localRegion);
      }

      nxagentExposeQueue.exposures[i].localRegion = NullRegion;

      if (nxagentExposeQueue.exposures[i].remoteRegion != NullRegion)
      {
        RegionDestroy(nxagentExposeQueue.exposures[i].remoteRegion);
      }

      nxagentExposeQueue.exposures[i].remoteRegion = NullRegion;
    }
  }

  nxagentDeleteConfiguredWindow(pWin);

  pWindowPriv = nxagentWindowPriv(pWin);

  if (pWin->nextSib)
  {
    nxagentWindowPriv(pWin->nextSib)->siblingAbove =
           pWindowPriv->siblingAbove;
  }

  #ifdef NXAGENT_SHAPE2
    #ifdef SHAPE
    if (pWindowPriv->boundingShape)
    {
      RegionDestroy(pWindowPriv->boundingShape);
    }

    if (pWindowPriv->clipShape)
    {
      RegionDestroy(pWindowPriv->clipShape);
    }
    #endif
  #else
    RegionDestroy(pWindowPriv->boundingShape);
    RegionDestroy(pWindowPriv->clipShape);
  #endif

  if (pWindowPriv -> corruptedRegion)
  {
    RegionDestroy(pWindowPriv -> corruptedRegion);
    pWindowPriv -> corruptedRegion = NULL;
  }

  if (nxagentSynchronization.pDrawable == (DrawablePtr) pWin)
  {
    nxagentSynchronization.pDrawable = NULL;

    #ifdef TEST
    fprintf(stderr, "nxagentDestroyWindow: Synchronization drawable [%p] removed from resources.\n",
                (void *) pWin);
    #endif
  }

  nxagentDestroyCorruptedResource((DrawablePtr) pWin, RT_NX_CORR_WINDOW);
  nxagentDestroyDrawableBitmap((DrawablePtr) pWin);

  if (pWindowPriv -> splitResource != NULL)
  {
    nxagentReleaseSplit((DrawablePtr) pWin);
  }

  XDestroyWindow(nxagentDisplay, nxagentWindow(pWin));

  if (nxagentOption(Rootless))
  {
    nxagentRootlessDelTopLevelWindow(pWin);
  }

  nxagentSplashCount--;

  #ifdef DEBUG
  fprintf(stderr, "nxagentDestroyWindow: The splash counter is now [%d].\n",
              nxagentSplashCount);
  #endif

  if (nxagentSplashCount == 1)
  {
    XClearWindow(nxagentDisplay, nxagentWindowPriv(nxagentRootTileWindow) -> window);
  }

  if (pWin == nxagentRootTileWindow)
  {
    nxagentWindowPriv(nxagentRootTileWindow)->window = None;
    nxagentRootTileWindow = None;
  }

  pWindowPriv->window = None;

  if (pWin -> optional)
  {
    pWin -> optional -> userProps = NULL;
  }

  if (nxagentOption(Rootless) && nxagentRootlessDialogPid == 0 &&
          nxagentLastWindowDestroyed == False && nxagentSomeWindowsAreMapped() == False)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentDestroyWindow: Last mapped window as been destroyed.\n");
    #endif

    nxagentLastWindowDestroyed     = True;
    nxagentLastWindowDestroyedTime = GetTimeInMillis();
  }

  return True;
}

/*
 * from "Definition of the Porting Layer for X v11 Sample Server":
 *
 * This routine is a hook for when DIX moves or resizes a window. It
 * should do whatever private operations need to be done when a window
 * is moved or resized. For instance, if DDX keeps a pixmap tile used
 * for drawing the background or border, and it keeps the tile rotated
 * such that it is longword aligned to longword locations in the frame
 * buffer, then you should rotate your tiles here. The actual graphics
 * involved in moving the pixels on the screen and drawing the border
 * are handled by CopyWindow(), below.
 *
 * See Xserver/fb/fbwindow.c for the sample server implementation.
 */
Bool nxagentPositionWindow(WindowPtr pWin, int x, int y)
{
  if (nxagentScreenTrap == 1)
  {
    return True;
  }

  #ifdef TEST
  fprintf(stderr, "nxagentPositionWindow: Changing position of window [%p][%ld] to [%d,%d].\n",
              (void *) pWin, nxagentWindow(pWin), x, y);
  #endif

  nxagentAddConfiguredWindow(pWin, CWSibling | CWX | CWY | CWWidth |
                                 CWHeight | CWBorderWidth);

  return True;
}

void nxagentRestackWindow(WindowPtr pWin, WindowPtr pOldNextSib)
{
  if (nxagentScreenTrap == 1)
  {
    return;
  }

  nxagentAddConfiguredWindow(pWin, CW_RootlessRestack);
}

void nxagentSwitchFullscreen(ScreenPtr pScreen, Bool switchOn)
{
  if (nxagentOption(Rootless) == 1)
  {
    return;
  }

  if (!switchOn)
  {
    nxagentWMDetect();

    /*
     * The smart scheduler could be stopped while
     * waiting for the reply. In this case we need
     * to yield explicitly to avoid to be stuck in
     * the dispatch loop forever.
     */

    isItTimeToYield = 1;

    if (!nxagentWMIsRunning)
    {
      #ifdef WARNING
      fprintf(stderr, "Warning: Can't switch to window mode, no window manager "
                  "has been detected.\n");
      #endif

      return;
    }
  }

  #ifdef TEST
  fprintf(stderr, "nxagentSwitchFullscreen: Switching to %s mode.\n",
              switchOn ? "fullscreen" : "windowed");
  #endif

  nxagentChangeOption(Fullscreen, switchOn);

  XEvent e = {
    .xclient.type = ClientMessage,
    .xclient.message_type = nxagentAtoms[13], /* _NET_WM_STATE */
    .xclient.display = nxagentDisplay,
    .xclient.window = nxagentDefaultWindows[pScreen -> myNum],
    .xclient.format = 32,
    .xclient.data.l[0] = nxagentOption(Fullscreen) ? 1 : 0,
    .xclient.data.l[1] = nxagentAtoms[14] /* _NET_WM_STATE_FULLSCREEN */
  };
  XSendEvent(nxagentDisplay, DefaultRootWindow(nxagentDisplay), False,
                 SubstructureRedirectMask, &e);

  if (switchOn)
  {
    nxagentFullscreenWindow = nxagentDefaultWindows[pScreen -> myNum];
    nxagentGrabPointerAndKeyboard(NULL);
  }
  else
  {
    nxagentFullscreenWindow = None;
    nxagentUngrabPointerAndKeyboard(NULL);
  }
}

void nxagentSwitchAllScreens(ScreenPtr pScreen, Bool switchOn)
{
  Window w;
  XSetWindowAttributes attributes;
  unsigned long valuemask;

  if (nxagentOption(Rootless))
  {
    return;
  }

  if (!switchOn)
  {
    nxagentWMDetect();

    if (!nxagentWMIsRunning)
    {
      #ifdef WARNING
      fprintf(stderr, "Warning: Can't switch to window mode, no window manager has been detected.\n");
      #endif

      return;
    }
  }

  w = nxagentDefaultWindows[pScreen -> myNum];

  /*
   * override_redirect makes the window manager ignore the window and
   * not add decorations, see ICCCM)
   */
  attributes.override_redirect = switchOn;
  valuemask = CWOverrideRedirect;
  XUnmapWindow(nxagentDisplay, w);
  XChangeWindowAttributes(nxagentDisplay, w, valuemask, &attributes);

  XReparentWindow(nxagentDisplay, w, DefaultRootWindow(nxagentDisplay), 0, 0);

  if (switchOn)
  {
    /*
     * Change to fullscreen mode.
     */

    struct timeval timeout;
    int i;
    XEvent e;

    /*
     * Wait for window manager reparenting the default window.
     */

    for (i = 0; i < 100 && nxagentWMIsRunning; i++)
    {
      #ifdef TEST
      fprintf(stderr, "nxagentSwitchAllScreens: WARNING! Going to wait for the ReparentNotify event.\n");
      #endif

      if (XCheckTypedWindowEvent(nxagentDisplay, w, ReparentNotify, &e))
      {
        break;
      }

      /*
       * This should also flush the NX link for us.
       */

      XSync(nxagentDisplay, 0);

      timeout.tv_sec  = 0;
      timeout.tv_usec = 50 * 1000;

      nxagentWaitEvents(nxagentDisplay, &timeout);
    }

    if (i < 100)
    {
      /*
       * The window manager has done with the reparent
       * operation. We can resize and map the window.
       */

      nxagentChangeOption(Fullscreen, True);
      nxagentChangeOption(AllScreens, True);

      /*
       * Save the window-mode configuration.
       */

      nxagentChangeOption(SavedX, nxagentOption(X));
      nxagentChangeOption(SavedY, nxagentOption(Y));
      nxagentChangeOption(SavedWidth, nxagentOption(Width));
      nxagentChangeOption(SavedHeight, nxagentOption(Height));
      nxagentChangeOption(SavedRootWidth, nxagentOption(RootWidth));
      nxagentChangeOption(SavedRootHeight, nxagentOption(RootHeight));

      /*
       * Reconf the Default window.
       */

      nxagentChangeOption(X, 0);
      nxagentChangeOption(Y, 0);
      nxagentChangeOption(Width, WidthOfScreen(DefaultScreenOfDisplay(nxagentDisplay)));
      nxagentChangeOption(Height, HeightOfScreen(DefaultScreenOfDisplay(nxagentDisplay)));

      /*
       * Move the root window.
       */

      nxagentChangeOption(RootX, (nxagentOption(Width) - nxagentOption(RootWidth)) / 2);
      nxagentChangeOption(RootY, (nxagentOption(Height) - nxagentOption(RootHeight)) / 2);
      nxagentChangeOption(ViewportXSpan, nxagentOption(Width) - nxagentOption(RootWidth));
      nxagentChangeOption(ViewportYSpan, nxagentOption(Height) - nxagentOption(RootHeight));

      XMoveResizeWindow(nxagentDisplay, w, nxagentOption(X), nxagentOption(Y),
                            nxagentOption(Width), nxagentOption(Height));

      nxagentUpdateViewportFrame(0, 0, nxagentOption(RootWidth), nxagentOption(RootHeight));

      XMoveWindow(nxagentDisplay, nxagentWindow(pScreen->root),
                      nxagentOption(RootX), nxagentOption(RootY));

      /*
       * We disable the screensaver when changing
       * mode to fullscreen. Is it really needed?
       */

      XSetScreenSaver(nxagentDisplay, 0, 0, DefaultExposures, DefaultBlanking);

      if (nxagentIconWindow == None)
      {
        nxagentIconWindow = nxagentCreateIconWindow();
        XMapWindow(nxagentDisplay, nxagentIconWindow);
      }

      XMapRaised(nxagentDisplay, w);
      XSetInputFocus(nxagentDisplay, w, RevertToParent, CurrentTime);
      XCheckTypedWindowEvent(nxagentDisplay, w, LeaveNotify, &e);
      nxagentFullscreenWindow = w;

      if (nxagentOption(DesktopResize) == 1)
      {
        if (nxagentOption(Shadow) == 0)
        {
          nxagentChangeScreenConfig(0, WidthOfScreen(DefaultScreenOfDisplay(nxagentDisplay)),
                                        HeightOfScreen(DefaultScreenOfDisplay(nxagentDisplay)));
        }
        else
        {
          nxagentShadowAdaptToRatio();
        }
      }
    }
    else
    {
      /*
       * We have waited for a reparent event unsuccessfully.
       * Something happened to the window manager.
       */

      #ifdef WARNING
      fprintf(stderr, "nxagentSwitchAllScreens: WARNING! Expected ReparentNotify event missing.\n");
      #endif

      nxagentWMIsRunning = False;
      attributes.override_redirect = False;
      XChangeWindowAttributes(nxagentDisplay, w, valuemask, &attributes);
      XMapWindow(nxagentDisplay, w);
    }
  }
  else
  {
    /*
     * FIXME:
     * It could be necessary:
     * - To restore screensaver.
     * - To set or reset nxagentForceBackingStore flag.
     * - To propagate device settings to the X server if no WM is running.
     */

    /*
     * Change to windowed mode.
     */

    nxagentChangeOption(Fullscreen, False);
    nxagentChangeOption(AllScreens, False);

    XDestroyWindow(nxagentDisplay, nxagentIconWindow);

    nxagentIconWindow = nxagentFullscreenWindow = None;

    if (nxagentOption(DesktopResize) == 1)
    {
      nxagentChangeOption(RootWidth, nxagentOption(SavedRootWidth));
      nxagentChangeOption(RootHeight, nxagentOption(SavedRootHeight));

      if (nxagentOption(Shadow) == 0)
      {
        nxagentChangeScreenConfig(0, nxagentOption(RootWidth),
                                      nxagentOption(RootHeight));
      }
    }

    /*
     * FIXME: These are 0 most of the time nowadays. The effect is,
     * that the window is moving a bit to right/bottom every time
     * fullscreen mode is left. To fix this query the frame extents
     * from the window manager via _NET_REQUEST_FRAME_EXTENTS
     */

    if (nxagentOption(WMBorderWidth) > 0)
    {
      nxagentChangeOption(X, nxagentOption(SavedX) - nxagentOption(WMBorderWidth));
    }
    else
    {
      nxagentChangeOption(X, nxagentOption(SavedX));
    }

    if (nxagentOption(WMTitleHeight) > 0)
    {
      nxagentChangeOption(Y, nxagentOption(SavedY) - nxagentOption(WMTitleHeight));
    }
    else
    {
      nxagentChangeOption(Y, nxagentOption(SavedY));
    }

    nxagentChangeOption(Width, nxagentOption(SavedWidth));
    nxagentChangeOption(Height, nxagentOption(SavedHeight));

    if (nxagentOption(Shadow) == 1 && nxagentOption(DesktopResize) == 1)
    {
      nxagentShadowAdaptToRatio();
    }

    XMoveResizeWindow(nxagentDisplay, w, nxagentOption(X), nxagentOption(Y),
                          nxagentOption(Width), nxagentOption(Height));

    nxagentUpdateViewportFrame(0, 0, nxagentOption(Width), nxagentOption(Height));

    XMoveWindow(nxagentDisplay, nxagentWindow(pScreen->root), 0, 0);
    XMapWindow(nxagentDisplay, w);

    nxagentChangeOption(RootX, 0);
    nxagentChangeOption(RootY, 0);
  }

  XMoveResizeWindow(nxagentDisplay, nxagentInputWindows[0], 0, 0,
                        nxagentOption(Width), nxagentOption(Height));

  nxagentSetPrintGeometry(pScreen -> myNum);
}

#ifdef VIEWPORT_FRAME

void nxagentUpdateViewportFrame(int x, int y, int w, int h)
{
  /*
   * Four virtual windows make a frame around the viewport. We move the frame
   * with the viewport together. The areas going into the viewport were covered by
   * the frame and become exposed. This make the agent send expose events to his
   * clients.
   */

  XID values[3];
  Mask mask;

  /*
   * nxagentScreenTrap = True;
   */

  values[2] = Above;

  values[1] = nxagentOption(RootHeight);

  mask = CWX | CWHeight | CWStackMode;
  values[0] = x - NXAGENT_FRAME_WIDTH;
  ConfigureWindow(nxagentViewportFrameLeft, mask, (XID *) &values, serverClient);

  values[0] = x + w;
  ConfigureWindow(nxagentViewportFrameRight, mask, (XID *) &values, serverClient);

  values[1] = nxagentOption(RootWidth);

  mask = CWY | CWWidth | CWStackMode;
  values[0] = y - NXAGENT_FRAME_WIDTH;
  ConfigureWindow(nxagentViewportFrameAbove, mask, (XID *) &values, serverClient);

  values[0] = y + h;
  ConfigureWindow(nxagentViewportFrameBelow, mask, (XID *) &values, serverClient);

  /*
   * nxagentScreenTrap = False;
   */
}

#endif /* #ifdef VIEWPORT_FRAME */

void nxagentMoveViewport(ScreenPtr pScreen, int hShift, int vShift)
{
  int newX, newY, oldX = 0, oldY = 0;
  Bool doMove = False;

  if (nxagentOption(Rootless))
  {
    return;
  }

  /*
   * We must keep x coordinate between viewportXSpan and zero, if viewportXSpan
   * is less then zero. If viewportXSpan is greater or equal to zero, it means
   * the agent root window has a size smaller than the agent default window.
   * In this case we keep the old coordinate.
   */

  #ifdef DEBUG
  fprintf(stderr, "nxagentMoveViewport: RootX[%i] RootY[%i], hShift[%i] vShift[%i].\n",
          nxagentOption(RootX), nxagentOption(RootY), hShift, vShift);
  #endif

  nxagentChangeOption(ViewportXSpan, nxagentOption(Width) - nxagentOption(RootWidth));
  nxagentChangeOption(ViewportYSpan, nxagentOption(Height) - nxagentOption(RootHeight));

  if (nxagentOption(ViewportXSpan) < 0)
  {
    newX = nxagentOption(RootX) - hShift;

    if (newX > 0)
    {
      newX = 0;
    }
    else if (newX < nxagentOption(ViewportXSpan))
    {
      newX = nxagentOption(ViewportXSpan);
    }
  }
  else if (nxagentOption(ViewportXSpan) == 0)
  {
    newX = 0;
  }
  else
  {
    newX = nxagentOption(RootX);
  }

  if (nxagentOption(ViewportYSpan) < 0)
  {
    newY = nxagentOption(RootY) - vShift;

    if (newY > 0)
    {
      newY = 0;
    }
    else if (newY < nxagentOption(ViewportYSpan))
    {
      newY = nxagentOption(ViewportYSpan);
    }
  }
  else if (nxagentOption(ViewportYSpan) == 0)
  {
    newY = 0;
  }
  else
  {
    newY = nxagentOption(RootY);
  }

  oldX = nxagentOption(RootX);

  if (newX != nxagentOption(RootX))
  {
    nxagentChangeOption(RootX, newX);

    doMove = True;
  }

  oldY = nxagentOption(RootY);

  if (newY != nxagentOption(RootY))
  {
    nxagentChangeOption(RootY, newY);

    doMove = True;
  }

  if (doMove)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentMoveViewport: New viewport geometry: (%d, %d)-"
                "(%d, %d)\n", -nxagentOption(RootX), -nxagentOption(RootY),
                    -nxagentOption(RootX) + nxagentOption(Width),
                        -nxagentOption(RootY) + nxagentOption(Height));

    fprintf(stderr, "nxagentMoveViewport: Root geometry x=[%d] y=[%d]\n",
                pScreen->root -> drawable.x,
                    pScreen->root -> drawable.y );
    #endif

    XMoveWindow(nxagentDisplay, nxagentWindow(pScreen->root),
                    nxagentOption(RootX), nxagentOption(RootY));

    if (nxagentOption(ClientOs) == ClientOsWinnt)
    {
      /*
       * If doMove is True we add exposed rectangles
       * to the remote expose region. This is done to
       * refresh the areas showed newly in the viewport.
       * We create two rectangles, one for horizontal
       * pan and one for vertical pan.
       */

      BoxRec hRect = {.x1 = -newX, .y1 = -newY};

      if (hShift < 0)
      {
        hRect.x2 = -oldX;
        hRect.y2 = -newY + nxagentOption(Height);
      }
      else if (hShift > 0)
      {
        hRect.x1 = -oldX + nxagentOption(Width);
        hRect.x2 = -newX + nxagentOption(Width);
        hRect.y2 = -newY + nxagentOption(Height);
      }

      #ifdef DEBUG
      fprintf(stderr, "nxagentMoveViewport: hRect p1[%i, %i] - p2[%i, %i].\n", hRect.x1, hRect.y1, hRect.x2, hRect.y2);
      #endif

      BoxRec vRect = {.x1 = -newX, .y1 = -newY};

      if (vShift < 0)
      {
        vRect.x2 = -newX + nxagentOption(Width);
        vRect.y2 = -oldY;
      }
      else if (vShift > 0)
      {
        vRect.y1 = -oldY + nxagentOption(Height);
        vRect.x2 = -newX + nxagentOption(Width);
        vRect.y2 = -newY + nxagentOption(Height);
      }

      #ifdef DEBUG
      fprintf(stderr, "nxagentMoveViewport: vRect p1[%i, %i] - p2[%i, %i].\n", vRect.x1, vRect.y1, vRect.x2, vRect.y2);
      #endif

      if (oldX != newX && hRect.x1 != hRect.x2 && hRect.y1 != hRect.y2)
      {
        nxagentAddRectToRemoteExposeRegion(&hRect);
      }

      if (oldY != newY && vRect.x1 != vRect.x2 && vRect.y1 != vRect.y2)
      {
        nxagentAddRectToRemoteExposeRegion(&vRect);
      }
    }
  }

  nxagentUpdateViewportFrame(-nxagentOption(RootX), -nxagentOption(RootY),
                                 nxagentOption(Width), nxagentOption(Height));
}

/*
 * This will update the window on the real X server by calling
 * XConfigureWindow()/XMapWindow()/XLowerWindow()/XRaiseWindow()
 * mask definesthe values that need to be updated, see e.g
 * man XConfigureWindow.
 *
 * In addition to the bit flags known to Xorg it uses these
 * self-defined ones: CW_Update, CW_Shape, CW_Map, CW_RootlessRestack.
 */
void nxagentConfigureWindow(WindowPtr pWin, unsigned int mask)
{
  unsigned int valuemask;
  XWindowChanges values;
  int offX = nxagentWindowPriv(pWin)->x - pWin->origin.x;
  int offY = nxagentWindowPriv(pWin)->y - pWin->origin.y;

  if (nxagentScreenTrap == 1)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentConfigureWindow: WARNING: Called with the screen trap set.\n");
    #endif

    return;
  }

  if (nxagentOption(Rootless) == 1 &&
          nxagentWindowTopLevel(pWin) == 1)
  {
    mask &= ~(CWSibling | CWStackMode);
  }
  else
  {
    if (mask & CW_RootlessRestack)
    {
      mask = CWStackMode;
    }
  }

  #ifdef TEST
  fprintf(stderr, "nxagentConfigureWindow: Called with window [%p][%ld] and mask [%x].\n",
              (void *) pWin, nxagentWindow(pWin), mask);
  #endif

  nxagentMoveCorruptedRegion(pWin, mask);

  valuemask = 0;

  if (mask & CW_Update)
  {
    mask |= CWX | CWY | CWWidth | CWHeight | CWBorderWidth | CWStackMode;
  }

  if (mask & CWX)
  {
    valuemask |= CWX;

    values.x = nxagentWindowPriv(pWin)->x = pWin->origin.x - wBorderWidth(pWin);
  }

  if (mask & CWY)
  {
    valuemask |= CWY;

    values.y = nxagentWindowPriv(pWin)->y = pWin->origin.y - wBorderWidth(pWin);
  }

  if (mask & CWWidth)
  {
    valuemask |= CWWidth;

    values.width = nxagentWindowPriv(pWin)->width = pWin->drawable.width;
  }

  if (mask & CWHeight)
  {
    valuemask |= CWHeight;

    values.height = nxagentWindowPriv(pWin)->height = pWin->drawable.height;
  }

  if (mask & CWBorderWidth)
  {
    valuemask |= CWBorderWidth;

    values.border_width = nxagentWindowPriv(pWin)->borderWidth =
        pWin->borderWidth;
  }

  if (mask & CW_Update)
  {
    valuemask = 0;
  }

  if (valuemask)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentConfigureWindow: Going to configure window [%p][%ld] with mask [%x].\n",
                (void *) pWin, nxagentWindow(pWin), valuemask);
    #endif

    if (pWin->bitGravity == StaticGravity &&
            ((mask & CWX) || (mask & CWY)) &&
                ((mask & CWWidth) || (mask & CWHeight)))
    {
      #ifdef TEST
      fprintf(stderr, "nxagentConfigureWindow: Window has StaticGravity. Going to translate Expose events by offset [%d, %d].\n",
                  offX, offY);
      #endif

      nxagentAddStaticResizedWindow(pWin, XNextRequest(nxagentDisplay), offX, offY);

      for (int j = 0; j < nxagentExposeQueue.length; j++)
      {
        int i = (nxagentExposeQueue.start + j) % EXPOSED_SIZE;

        if (nxagentExposeQueue.exposures[i].pWindow == pWin &&
                nxagentExposeQueue.exposures[i].remoteRegion != NullRegion)
        {
          RegionTranslate(nxagentExposeQueue.exposures[i].remoteRegion, offX, offY);
        }
      }
    }

    XConfigureWindow(nxagentDisplay, nxagentWindow(pWin), valuemask, &values);

    MAKE_SYNC_CONFIGURE_WINDOW;
  }

  if (mask & CWStackMode &&
          nxagentWindowPriv(pWin)->siblingAbove != nxagentWindowSiblingAbove(pWin))
  {
    WindowPtr pSib;

    /*
     * Find the top sibling.
     */

    for (pSib = pWin; pSib->prevSib != NullWindow; pSib = pSib->prevSib);

    /*
     * Configure the top sibling.
     */

    valuemask = CWStackMode;

    values.stack_mode = Above;

    #ifdef TEST
    fprintf(stderr, "nxagentConfigureWindow: Going to configure top sibling [%ld] "
                "with mask [%x] and parent [%ld].\n", nxagentWindow(pSib),
                    valuemask, nxagentWindowParent(pWin));
    #endif

    XConfigureWindow(nxagentDisplay, nxagentWindow(pSib), valuemask, &values);

    MAKE_SYNC_CONFIGURE_WINDOW;

    nxagentWindowPriv(pSib)->siblingAbove = None;

    /*
     * Configure the rest of siblings.
     */

    valuemask = CWSibling | CWStackMode;

    values.stack_mode = Below;

    for (pSib = pSib->nextSib; pSib != NullWindow; pSib = pSib->nextSib)
    {
      values.sibling = nxagentWindowSiblingAbove(pSib);

      #ifdef TEST
      fprintf(stderr, "nxagentConfigureWindow: Going to configure other sibling [%ld] "
                  "with mask [%x] and parent [%ld] below [%ld].\n", nxagentWindow(pSib),
                      valuemask, nxagentWindowParent(pWin), nxagentWindowSiblingAbove(pSib));
      #endif

      XConfigureWindow(nxagentDisplay, nxagentWindow(pSib), valuemask, &values);

      MAKE_SYNC_CONFIGURE_WINDOW;

      nxagentWindowPriv(pSib)->siblingAbove = nxagentWindowSiblingAbove(pSib);
    }

    #ifdef TEST
    {
      Window root_return;
      Window parent_return;
      Window *children_return = NULL;
      unsigned int nchildren_return;
      Status result;

      result = XQueryTree(nxagentDisplay, DefaultRootWindow(nxagentDisplay),
                              &root_return, &parent_return, &children_return, &nchildren_return);

      if (result)
      {
        fprintf(stderr, "nxagentConfigureWindow: Children of the root: ");
        while(nchildren_return > 0)
        {
          pSib = nxagentWindowPtr(children_return[--nchildren_return]);
          if (pSib)
          {
            fprintf(stderr, "%lu  ", children_return[nchildren_return]);
          }
        }
        fprintf(stderr, "\n");
      }
      else
      {
        fprintf(stderr, "nxagentConfigureWindow: Failed QueryTree request.\n ");
      }

      SAFE_XFree(children_return);
    }
    #endif
  }

  #ifdef NXAGENT_SPLASH
  /*
   * This should bring again the splash window
   * on top, so why the else clause? Is this
   * really needed?
   *
   *
   *  else if (mask & CWStackMode)
   *  {
   *    if (nxagentSplashWindow)
   *    {
   *      valuemask = CWStackMode;
   *
   *      values.stack_mode = Above;
   *
   *      #ifdef TEST
   *      fprintf(stderr, "nxagentConfigureWindow: Going to configure splash window [%ld].\n",
   *                  nxagentSplashWindow);
   *      #endif
   *
   *      XConfigureWindow(nxagentDisplay, nxagentSplashWindow, valuemask, &values);
   *
   *      MAKE_SYNC_CONFIGURE_WINDOW;
   *    }
   *  }
   */
  #endif /* NXAGENT_SPLASH */

  if (mask & CW_RootlessRestack)
  {
    if (!pWin -> prevSib)
    {
      #ifdef TEST
      fprintf(stderr, "nxagentConfigureWindow: Raising window [%p][%ld].\n",
                  (void *) pWin, nxagentWindow(pWin));
      #endif

      XRaiseWindow(nxagentDisplay, nxagentWindow(pWin));
    }
    else if (!pWin -> nextSib)
    {
      #ifdef TEST
      fprintf(stderr, "nxagentConfigureWindow: Lowering window [%p][%ld].\n",
                  (void *) pWin, nxagentWindow(pWin));
      #endif

      XLowerWindow(nxagentDisplay, nxagentWindow(pWin));
    }
    else
    {
      XlibWindow windowList[2];

      #ifdef TEST
      fprintf(stderr, "nxagentConfigureWindow: Putting window [%p][%ld] in the middle.\n",
                  (void *) pWin, nxagentWindow(pWin));
      #endif

      windowList[0] = nxagentWindow(pWin->prevSib);
      windowList[1] = nxagentWindow(pWin);

      XRestackWindows(nxagentDisplay, windowList, 2);
    }
  }

  #ifdef SHAPE
  if (mask & CW_Shape)
  {
    nxagentShapeWindow(pWin);
  }
  #endif

  if (mask & CW_Map &&
         (!nxagentOption(Rootless) ||
             nxagentRootlessWindow != pWin))
  {
    XMapWindow(nxagentDisplay, nxagentWindow(pWin));
    return;
  }
}

/*
 * from "Definition of the Porting Layer for X v11 Sample Server":
 *
 * This function will be called when a window is reparented. At the
 * time of the call, pWin will already be spliced into its new
 * position in the window tree, and pPriorParent is its previous
 * parent. This function can be NULL.
 *
 * We simply pass this pver to the real X server.
 */
void nxagentReparentWindow(WindowPtr pWin, WindowPtr pOldParent)
{
  if (nxagentScreenTrap)
  {
    return;
  }

  #ifdef TEST
  fprintf(stderr, "nxagentReparentWindow:  window at %p [%lx] previous parent at %p [%lx].\n",
              (void*)pWin, nxagentWindow(pWin),
                  (void*)pOldParent, nxagentWindow(pOldParent));
  #endif

  XReparentWindow(nxagentDisplay, nxagentWindow(pWin),
                      nxagentWindowParent(pWin),
                          pWin->origin.x - wBorderWidth(pWin),
                              pWin->origin.y - wBorderWidth(pWin));
}

/*
 * from "Definition of the Porting Layer for X v11 Sample Server":
 *
 * ChangeWindowAttributes is called whenever DIX changes window
 * attributes, such as the size, front-to-back ordering, title, or
 * anything of lesser severity that affects the window itself. The
 * sample server implements this routine. It computes accelerators for
 * quickly putting up background and border tiles. (See description of
 * the set of routines stored in the WindowRec.)
 */
Bool nxagentChangeWindowAttributes(WindowPtr pWin, unsigned long mask)
{
  XSetWindowAttributes attributes;

  #ifdef TEST
  fprintf(stderr, "nxagentChangeWindowAttributes: Changing attributes for window at [%p] with mask [%lu].\n",
              (void *) pWin, mask);
  #endif

  if (nxagentScreenTrap)
  {
    return True;
  }

  if (mask & CWBackPixmap)
  {
    switch (pWin->backgroundState)
    {
      case None:
      {
        attributes.background_pixmap = None;
        attributes.background_pixel = nxagentWhitePixel;

        /*
         * One of problems faced during the implementation of lazy
         * encoding policies was due to the presence of windows with
         * transparent background, usually created by X clients to
         * cover some sensible areas (i.e. checkboxes used by
         * Konqueror 3.5). The sequence of operations consists in
         * drawing the underneath part before covering it with the
         * transparent window, so when we synchronize the deferred
         * drawing operation we have to bear in mind that the dest-
         * ination area is covered by a window. By using the Inclu-
         * deInferiors GC's property and by clipping the region to
         * synchronize to the borderClip instead of clipList (to
         * include the areas covered by children) we can easily take
         * care of this situation, but there is a drawback: if the
         * children are not transparent, we are going to synchronize
         * invisible areas. To avoid this we have added the 'has-
         * TransparentChildren' flag, which is set when a window has
         * at least one child with background None. The problem is
         * that we don't know when to reset the flag. This solution,
         * also, doesn't take care of transparent windows which don't
         * have childhood relationships with underneath windows.
         * We tried to mark the whole windows as dirty when they are
         * created to force the synchronization of transparent windows
         * with the content of underneath windows, but, of course,
         * this works only with the first synchronization because the
         * transparent windows will be never marked again as dirty.
         */

        if (pWin -> parent != NULL)
        {
          nxagentWindowPriv(pWin -> parent) -> hasTransparentChildren = 1;

          #ifdef DEBUG
          fprintf(stderr, "nxagentChangeWindowAttributes: WARNING! Window at [%p] got the "
                      "hasTransparentChildren flag.\n", (void *) pWin);
          #endif
        }

        break;
      }
      case ParentRelative:
      {
        attributes.background_pixmap = ParentRelative;

        break;
      }
      case BackgroundPixmap:
      {
        /*
         * If a window background is corrupted, we grant
         * its usability by clearing it with a solid co-
         * lor. When the pixmap will be fully synchroni-
         * zed, an expose will be sent to the window's
         * hierarchy.
         */

        if (nxagentDrawableStatus((DrawablePtr) pWin -> background.pixmap) == NotSynchronized)
        {
          #ifdef TEST
          fprintf(stderr, "nxagentChangeWindowAttributes: The window at [%p] has the background at [%p] "
                      "not synchronized.\n", (void *) pWin, (void *) pWin -> background.pixmap);
          #endif

          if (nxagentIsCorruptedBackground(pWin -> background.pixmap) == 0)
          {
            nxagentIsCorruptedBackground(pWin -> background.pixmap) = 1;

            nxagentAllocateCorruptedResource((DrawablePtr) pWin -> background.pixmap, RT_NX_CORR_BACKGROUND);

            /*
             * Clearing the remote background to
             * make it usable.
             */

            nxagentFillRemoteRegion((DrawablePtr) pWin -> background.pixmap,
                                        nxagentCorruptedRegion((DrawablePtr) pWin -> background.pixmap));
          }
        }

        attributes.background_pixmap = nxagentPixmap(pWin -> background.pixmap);

        break;
      }
      case BackgroundPixel:
      {
        mask &= ~CWBackPixmap;

        break;
      }
    }
  }

  if (mask & CWBackPixel)
  {
    if (pWin -> backgroundState == BackgroundPixel)
    {
      attributes.background_pixel = nxagentPixel(pWin -> background.pixel);
    }
    else
    {
      mask &= ~CWBackPixel;
    }
  }

  if (mask & CWBorderPixmap)
  {
    if (pWin -> borderIsPixel != 0)
    {
      mask &= ~CWBorderPixmap;
    }
    else
    {
      attributes.border_pixmap = nxagentPixmap(pWin -> border.pixmap);
    }
  }

  if (mask & CWBorderPixel)
  {
    if (pWin -> borderIsPixel != 0)
    {
      attributes.border_pixel = nxagentPixel(pWin -> border.pixel);
    }
    else
    {
      mask &= ~CWBorderPixel;
    }
  }

  if (mask & CWBitGravity)
  {
    attributes.bit_gravity = pWin -> bitGravity;
  }

  /*
   * As we set this bit, we must change dix in
   * order not to perform PositionWindow and let
   * X move children windows for us.
   */

  if (mask & CWWinGravity)
  {
    attributes.win_gravity = pWin -> winGravity;
  }

  /*
    FIXME: Do we need to set the attribute on the
           remote display?
  */
  if (mask & CWBackingStore)
  {
    attributes.backing_store = pWin -> backingStore;

    #ifdef TEST
    fprintf(stderr, "nxagentChangeWindowAttributes: Changing backing store value to %d"
                " for window at %p.\n", pWin -> backingStore, (void*)pWin);
    #endif
  }

  if (mask & CWBackingPlanes)
  {
    if ((nxagentBackingStore == NotUseful) || (pWin -> optional == NULL))
    {
      mask &= ~CWBackingPlanes;
    }
    else
    {
      attributes.backing_planes = pWin -> optional -> backingBitPlanes;
    }
  }

  if (mask & CWBackingPixel)
  {
    if ((nxagentBackingStore == NotUseful) || (pWin -> optional == NULL))
    {
      mask &= ~CWBackingPixel;
    }
    else
    {
      attributes.backing_pixel = pWin -> optional -> backingPixel;
    }
  }

  if (mask & CWOverrideRedirect)
  {
    attributes.override_redirect = pWin -> overrideRedirect;
  }

  /*
    FIXME: Do we need to set the attribute on the
           remote display?
  */
  if (mask & CWSaveUnder)
  {
    attributes.save_under = pWin -> saveUnder;
  }

  /*
   * Events are handled elsewhere.
   */

  if (mask & CWEventMask)
  {
    mask &= ~CWEventMask;
  }

  if (mask & CWDontPropagate)
  {
    mask &= ~CWDontPropagate;
  }

  if (mask & CWColormap)
  {
    ColormapPtr pCmap = (ColormapPtr) LookupIDByType(wColormap(pWin), RT_COLORMAP);

    /*
      FIXME: When the caller is nxagentReconfigureWindow
             sometimes wColormap(pWin) is 0. Could a window
             have no colormap?
    */
    if (pCmap != NULL)
    {
      attributes.colormap = nxagentColormap(pCmap);

      nxagentSetInstalledColormapWindows(pWin -> drawable.pScreen);
    }
    else
    {
      #ifdef WARNING
      fprintf(stderr, "nxagentChangeWindowAttributes: WARNING! Bad colormap "
                  "[%lu] for window at [%p].\n", wColormap(pWin), (void *) pWin);
      #endif

      mask &= ~CWColormap;
    }
  }

  if (mask & CWCursor)
  {
    if (nxagentOption(Rootless))
    {
      if (pWin->cursorIsNone == 0 && pWin->optional != NULL &&
              pWin->optional->cursor != NULL && nxagentCursorPriv(pWin ->
                  optional -> cursor, pWin -> drawable.pScreen) != NULL)
      {
        attributes.cursor = nxagentCursor(pWin -> optional -> cursor,
                                              pWin -> drawable.pScreen);
      }
      else
      {
        attributes.cursor = None;
      }
    }
    else
    {
      /*
       * This is handled in cursor code
       */

      mask &= ~CWCursor;
    }
  }

  if (mask != 0)
  {
    XChangeWindowAttributes(nxagentDisplay, nxagentWindow(pWin), mask, &attributes);
  }

  return 1;
}

/* Set the WM_STATE property of pWin to the desired value */
void nxagentSetWMState(WindowPtr pWin, CARD32 desired)
{
  Atom prop = MakeAtom("WM_STATE", strlen("WM_STATE"), True);
  nxagentWMStateRec wmState = {.state = desired, .icon = None};
  if (ChangeWindowProperty(pWin, prop, prop, 32, 0, 2, &wmState, 1) != Success)
  {
    #ifdef WARNING
    fprintf(stderr, "%s: Changing WM_STATE failed.\n", __func__);
    #endif
  }
}

/*
 * from "Definition of the Porting Layer for X v11 Sample Server":
 *
 * RealizeWindow/UnRealizeWindow:
 * These routines are hooks for when DIX maps (makes visible) and
 * unmaps (makes invisible) a window. It should do whatever private
 * operations need to be done when these happen, such as allocating or
 * deallocating structures that are only needed for visible
 * windows. RealizeWindow does NOT draw the window border, background
 * or contents; UnrealizeWindow does NOT erase the window or generate
 * exposure events for underlying windows; this is taken care of by
 * DIX. DIX does, however, call PaintWindowBackground() and
 * PaintWindowBorder() to perform some of these.
-+ */
Bool nxagentRealizeWindow(WindowPtr pWin)
{
  if (nxagentScreenTrap == 1)
  {
    return True;
  }

  /*
   * Not needed.
   *
   * nxagentConfigureWindow(pWin, CWStackMode);
   *
   * nxagentFlushConfigureWindow();
   */

  nxagentAddConfiguredWindow(pWin, CWStackMode);
  nxagentAddConfiguredWindow(pWin, CW_Shape);

  /* add by dimbor */
  if (nxagentOption(Rootless) && nxagentWindowTopLevel(pWin))
  {
    nxagentSetWMState(pWin, NormalState);
  }

  /*
   * Not needed.
   *
    #ifdef SHAPE
    nxagentShapeWindow(pWin);
    #endif
   */

  #ifdef TEST
  if (nxagentOption(Rootless) && nxagentLastWindowDestroyed)
  {
    fprintf(stderr, "%s: Window realized. Stopped termination for rootless session.\n", __func__);
  }
  #endif

  nxagentAddConfiguredWindow(pWin, CW_Map);

  nxagentLastWindowDestroyed = False;

  return True;
}

/* See nxagentRealizeWindow for a description */
Bool nxagentUnrealizeWindow(WindowPtr pWin)
{
  if (nxagentScreenTrap)
  {
    return True;
  }

  /* add by dimbor */
  if (nxagentOption(Rootless) && nxagentWindowTopLevel(pWin))
  {
    /*
     * The original _comment_ was WithdrawnState, while the _value_
     * was 3, which is IconicState.
     */
    nxagentSetWMState(pWin, IconicState);
  }

  XUnmapWindow(nxagentDisplay, nxagentWindow(pWin));

  return True;
}

void nxagentFrameBufferPaintWindow(WindowPtr pWin, RegionPtr pRegion, int what)
{
  if (pWin->backgroundState == BackgroundPixmap)
  {
    pWin->background.pixmap = nxagentVirtualPixmap(pWin->background.pixmap);
  }

  if (pWin->borderIsPixel == False)
  {
    pWin->border.pixmap = nxagentVirtualPixmap(pWin->border.pixmap);
  }

  /*
   * Call fbPaintWindow(). We need to temporarily replace
   * PaintWindowBackground() by ourself because fbPaintWindow() is
   * recursively calling it for parent windows, too.
   */
  {
    void (*PaintWindowBackgroundBackup)(WindowPtr, RegionPtr, int);

    PaintWindowBackgroundBackup = pWin->drawable.pScreen -> PaintWindowBackground;

    pWin->drawable.pScreen -> PaintWindowBackground = nxagentFrameBufferPaintWindow;

    fbPaintWindow(pWin, pRegion, what);

    pWin->drawable.pScreen -> PaintWindowBackground = PaintWindowBackgroundBackup;
  }

  if (pWin->backgroundState == BackgroundPixmap)
  {
    pWin->background.pixmap = nxagentRealPixmap(pWin->background.pixmap);
  }

  if (pWin->borderIsPixel == False)
  {
    pWin->border.pixmap = nxagentRealPixmap(pWin->border.pixmap);
  }
}

void nxagentPaintWindowBackground(WindowPtr pWin, RegionPtr pRegion, int what)
{
  RegionRec temp;

  if (pWin -> realized)
  {
    BoxPtr pBox = RegionRects(pRegion);

    for (int i = 0; i < RegionNumRects(pRegion); i++)
    {
      XClearArea(nxagentDisplay, nxagentWindow(pWin),
                 pBox[i].x1 - pWin->drawable.x,
                 pBox[i].y1 - pWin->drawable.y,
                 pBox[i].x2 - pBox[i].x1,
                 pBox[i].y2 - pBox[i].y1,
                 False);
    }
  }
  #ifdef TEST
  else
  {
    fprintf(stderr, "nxagentPaintWindowBackground: Saving the operation with window "
                "at [%p] not realized.\n", (void *) pWin);
  }
  #endif

  /*
   * The framebuffer operations don't take care of
   * clipping to the actual area of the framebuffer
   * so we need to clip ourselves.
   */

  RegionInit(&temp, NullBox, 1);
  RegionIntersect(&temp, pRegion, &pWin -> clipList);
  nxagentFrameBufferPaintWindow(pWin, &temp, what);
  RegionUninit(&temp);
}

void nxagentPaintWindowBorder(WindowPtr pWin, RegionPtr pRegion, int what)
{
  RegionRec temp;

  /*
   * The framebuffer operations don't take care of
   * clipping to the actual area of the framebuffer
   * so we need to clip ourselves.
   */

  RegionInit(&temp, NullBox, 1);
  RegionIntersect(&temp, pRegion, &pWin -> borderClip);
  nxagentFrameBufferPaintWindow(pWin, &temp, what);
  RegionUninit(&temp);
}

/*
 * from "Definition of the Porting Layer for X v11 Sample Server":
 *
 * CopyWindow is called when a window is moved, and graphically moves
 * to pixels of a window on the screen. It should not change any other
 * state within DDX (see PositionWindow(), above).
 *
 * oldpt is the old location of the upper-left corner. oldRegion is
 * the old region it is coming from. The new location and new region
 * is stored in the WindowRec. oldRegion might modified in place by
 * this routine (the sample implementation does this).
 *
 * CopyArea could be used, except that this operation has more
 * complications. First of all, you do not want to copy a rectangle
 * onto a rectangle. The original window may be obscured by other
 * windows, and the new window location may be similarly
 * obscured. Second, some hardware supports multiple windows with
 * multiple depths, and your routine needs to take care of that.
 *
 * The pixels in oldRegion (with reference point oldpt) are copied to
 * the window's new region (pWin->borderClip). pWin->borderClip is
 * gotten directly from the window, rather than passing it as a
 * parameter.
 *
 * The sample server implementation is in Xserver/fb/fbwindow.c.
 */
void nxagentCopyWindow(WindowPtr pWin, xPoint oldOrigin, RegionPtr oldRegion)
{
  fbCopyWindow(pWin, oldOrigin, oldRegion);
}

/*
 * from "Definition of the Porting Layer for X v11 Sample Server":
 *
 * Whenever the cliplist for a window is changed, this function is
 * called to perform whatever hardware manipulations might be
 * necessary. When called, the clip list and border clip regions in
 * the window are set to the new values. dx,dy are the distance that
 * the window has been moved (if at all).
 */
void nxagentClipNotify(WindowPtr pWin, int dx, int dy)
{
  /*
   * nxagentConfigureWindow(pWin, CWStackMode);
   */

  nxagentAddConfiguredWindow(pWin, CWStackMode);
  nxagentAddConfiguredWindow(pWin, CW_Shape);

  #ifndef NXAGENT_SHAPE
    #ifdef SHAPE
    /*
     * nxagentShapeWindow(pWin);
     */
    #endif /* SHAPE */
  #endif /* NXAGENT_SHAPE */
}

/*
 * from "Definition of the Porting Layer for X v11 Sample Server":
 *
 * The WindowExposures() routine paints the border and generates
 * exposure events for the window. pRegion is an unoccluded region of
 * the window, and pBSRegion is an occluded region that has backing
 * store. Since exposure events include a rectangle describing what
 * was exposed, this routine may have to send back a series of
 * exposure events, one for each rectangle of the region. The count
 * field in the expose event is a hint to the client as to the number
 * of regions that are after this one. This routine must be
 * provided. The sample server has a machine-independent version in
 * Xserver/mi/miexpose.c.
 */
void nxagentWindowExposures(WindowPtr pWin, RegionPtr pRgn, RegionPtr other_exposed)
{
  /*
   * The problem: we want to synthetize the expose events internally, so
   * that we reduce the time between a window operation and the corresp-
   * onding graphical output, but at the same time we need to take care
   * of the remote exposures, as we need to handle those cases where our
   * windows are covered by the windows on the real display. Handling
   * both the local and the remote exposures we would generate the same
   * redraws twice, something we call "double refreshes", so we must be
   * able to identify which events have been already sent to our clients.
   *
   * Here is how the algorithm is working:
   *
   * - We collect the exposures that the agent sent to its clients in a
   *   region (the "local-region") and store the region for a given win-
   *   dow in a vector.
   *
   * - Another region collects the expose that were received from the
   *   real X server (the "remote-region") for the same window.
   *
   * - We create a "fake" off-screen window. For every generated region
   *   we send a ConfigureWindow request for that window to synchronize
   *   ourselves with both the remote X server and/or the window manager.
   *
   * - When the ConfigureNotify is received, we calculate the difference
   *   between the "remote-region" and the "local-region" for the window
   *   that had collected exposures.
   *
   * - Finally we send the resulting exposures to our clients.
   *
   * As you may have guessed, the windows are synchronized per-region,
   * that is there is a single region for a set of exposures. The regions
   * are handled in order. This means that we can always calculate the
   * final region by referring to the first element of the vector.
   */

  RegionRec temp;
  BoxRec box;

  if (nxagentSessionState != SESSION_DOWN)
  {
    if (nxagentExposeArrayIsInitialized == 0)
    {
      #ifdef TEST
      fprintf(stderr, "nxagentWindowExposures: Initializing expose queue.\n");
      #endif

      for (int i = 0; i < EXPOSED_SIZE; i++)
      {
        nxagentExposeQueue.exposures[i].pWindow = NULL;
        nxagentExposeQueue.exposures[i].localRegion = NullRegion;
        nxagentExposeQueue.exposures[i].remoteRegion = NullRegion;
        nxagentExposeQueue.exposures[i].remoteRegionIsCompleted = False;
        nxagentExposeQueue.exposures[i].serial = 0;
      }

      nxagentExposeQueue.start = 0;
      nxagentExposeQueue.length = 0;
      nxagentExposeSerial = 0;

      XSetWindowAttributes attributes = {.event_mask = StructureNotifyMask};
      nxagentConfiguredSynchroWindow = XCreateWindow(nxagentDisplay, DefaultRootWindow(nxagentDisplay), 0, 0,
                                                           1, 1, 0, 0, InputOutput, 0, CWEventMask, &attributes);

      nxagentInitRemoteExposeRegion();

      nxagentExposeArrayIsInitialized = 1;
    }

    RegionInit(&temp, (BoxRec *) NULL, 1);

    if (pRgn != NULL)
    {
      if (RegionNumRects(pRgn) > RECTLIMIT)
      {
        box = *RegionExtents(pRgn);

        RegionEmpty(pRgn);
        RegionInit(pRgn, &box, 1);
      }

      RegionUnion(&temp, &temp, pRgn);
    }

    if (other_exposed != NULL)
    {
      RegionUnion(&temp, &temp, other_exposed);
    }

    if (RegionNil(&temp) == 0)
    {
      RegionTranslate(&temp,
                           -(pWin -> drawable.x), -(pWin -> drawable.y));

      if (nxagentExposeQueue.length < EXPOSED_SIZE)
      {
        int index = (nxagentExposeQueue.start + nxagentExposeQueue.length) % EXPOSED_SIZE;

        nxagentExposeQueue.exposures[index].pWindow = pWin;
        nxagentExposeQueue.exposures[index].localRegion = RegionCreate(NULL, 1);

        if (nxagentOption(Rootless) && nxagentWindowPriv(pWin) &&
                (nxagentWindowPriv(pWin) -> isMapped == 0 ||
                     nxagentWindowPriv(pWin) -> visibilityState != VisibilityUnobscured))
        {
          nxagentExposeQueue.exposures[index].remoteRegion = RegionCreate(NULL, 1);

          RegionUnion(nxagentExposeQueue.exposures[index].remoteRegion,
                           nxagentExposeQueue.exposures[index].remoteRegion, &temp);

          #ifdef TEST
          fprintf(stderr, "nxagentWindowExposures: Added region to remoteRegion for window [%ld] to position [%d].\n",
                      nxagentWindow(pWin), nxagentExposeQueue.length);
          #endif
        }
        else
        {
          RegionUnion(nxagentExposeQueue.exposures[index].localRegion,
                           nxagentExposeQueue.exposures[index].localRegion, &temp);

          #ifdef TEST
          fprintf(stderr, "nxagentWindowExposures: Added region to localRegion for window [%ld] to position [%d].\n",
                      nxagentWindow(pWin), nxagentExposeQueue.length);
          #endif
        }

        nxagentExposeSerial = (nxagentExposeSerial - 1) % EXPOSED_SIZE;

        nxagentExposeQueue.exposures[index].serial = nxagentExposeSerial;

        #ifdef TEST
        fprintf(stderr, "nxagentWindowExposures: Added region to queue with serial [%d].\n", nxagentExposeSerial);
        #endif

        /*
         * Mark this region for sending a synchro,
         * in nxagentFlushConfigureWindow().
         */

        nxagentExposeQueue.exposures[index].synchronize = 1;
        nxagentExposeQueue.length++;

        if (nxagentOption(Rootless) && nxagentWindowPriv(pWin) &&
                (nxagentWindowPriv(pWin) -> isMapped == 0 ||
                     nxagentWindowPriv(pWin) -> visibilityState != VisibilityUnobscured))
        {
          RegionUninit(&temp);

          return;
        }
      }
      else
      {
        RegionUninit(&temp);

        #ifdef TEST
        fprintf(stderr, "nxagentWindowExposures: WARNING! Reached maximum size of collect exposures vector.\n");
        #endif

        if ((pRgn != NULL && RegionNotEmpty(pRgn) != 0) ||
               (other_exposed != NULL && RegionNotEmpty(other_exposed) != 0))
        {
          nxagentUnmarkExposedRegion(pWin, pRgn, other_exposed);

          miWindowExposures(pWin, pRgn, other_exposed);
        }

        return;
      }
    }

    RegionUninit(&temp);
  }

  if ((pRgn != NULL && RegionNotEmpty(pRgn) != 0) ||
         (other_exposed != NULL && RegionNotEmpty(other_exposed) != 0))
  {
    nxagentUnmarkExposedRegion(pWin, pRgn, other_exposed);

    miWindowExposures(pWin, pRgn, other_exposed);
  }

  return;
}

#ifdef SHAPE
static Bool nxagentRegionEqual(RegionPtr pReg1, RegionPtr pReg2)
{
  BoxPtr pBox1, pBox2;
  unsigned int n1, n2;

  if (pReg1 == pReg2)
  {
    return True;
  }

  if (pReg1 == NullRegion || pReg2 == NullRegion)
  {
    return False;
  }

  pBox1 = RegionRects(pReg1);
  n1 = RegionNumRects(pReg1);

  pBox2 = RegionRects(pReg2);
  n2 = RegionNumRects(pReg2);

  if (n1 != n2)
  {
    return False;
  }

  if (pBox1 == pBox2)
  {
    return True;
  }

  if (memcmp(pBox1, pBox2, n1 * sizeof(BoxRec)))
  {
    return False;
  }

  return True;
}

void nxagentShapeWindow(WindowPtr pWin)
{
  Region reg;
  BoxPtr pBox;

  if (NXDisplayError(nxagentDisplay) == 1)
  {
    return;
  }

  #ifdef DEBUG
  fprintf(stderr, "nxagentShapeWindow: Window at [%p][%ld].\n",
              (void *) pWin, nxagentWindow(pWin));
  #endif

  /*
    FIXME: this is the same code as below, just with another shape. Maybe move
    this code to a helper function?
   */
  if (!nxagentRegionEqual(nxagentWindowPriv(pWin)->boundingShape,
                        wBoundingShape(pWin)))
  {
    #ifdef DEBUG
    fprintf(stderr, "nxagentShapeWindow: Bounding shape differs.\n");
    #endif

    if (wBoundingShape(pWin))
    {
      #ifdef DEBUG
      fprintf(stderr, "nxagentShapeWindow: wBounding shape has [%ld] rects.\n",
                  RegionNumRects(wBoundingShape(pWin)));
      #endif

      #ifdef NXAGENT_SHAPE2
      if (!nxagentWindowPriv(pWin)->boundingShape)
      {
        nxagentWindowPriv(pWin)->boundingShape = RegionCreate(NULL, 1);
      }
      #endif

      RegionCopy(nxagentWindowPriv(pWin)->boundingShape, wBoundingShape(pWin));

      reg = XCreateRegion();
      pBox = RegionRects(nxagentWindowPriv(pWin)->boundingShape);
      for (int i = 0;
           i < RegionNumRects(nxagentWindowPriv(pWin)->boundingShape);
           i++)
      {
        XRectangle rect = {
          .x = pBox[i].x1,
          .y = pBox[i].y1,
          .width = pBox[i].x2 - pBox[i].x1,
          .height = pBox[i].y2 - pBox[i].y1
        };
        XUnionRectWithRegion(&rect, reg, reg);
      }

      #ifndef NXAGENT_SHAPE
      XShapeCombineRegion(nxagentDisplay, nxagentWindow(pWin),
                              ShapeBounding, 0, 0, reg, ShapeSet);
      #endif

      XDestroyRegion(reg);
    }
    else
    {
      #ifdef DEBUG
      fprintf(stderr, "nxagentShapeWindow: wBounding shape does not exist. Removing the shape.\n");
      #endif

      RegionEmpty(nxagentWindowPriv(pWin)->boundingShape);

      #ifndef NXAGENT_SHAPE
      XShapeCombineMask(nxagentDisplay, nxagentWindow(pWin),
                            ShapeBounding, 0, 0, None, ShapeSet);
      #endif
    }
  }

  if (!nxagentRegionEqual(nxagentWindowPriv(pWin)->clipShape, wClipShape(pWin)))
  {
    #ifdef DEBUG
    fprintf(stderr, "nxagentShapeWindow: Clip shape differs.\n");
    #endif

    if (wClipShape(pWin))
    {
      #ifdef DEBUG
      fprintf(stderr, "nxagentShapeWindow: wClip shape has [%ld] rects.\n",
                  RegionNumRects(wClipShape(pWin)));
      #endif

      #ifdef NXAGENT_SHAPE2
      if (!nxagentWindowPriv(pWin)->clipShape)
      {
        nxagentWindowPriv(pWin)->clipShape = RegionCreate(NULL, 1);
      }
      #endif

      RegionCopy(nxagentWindowPriv(pWin)->clipShape, wClipShape(pWin));

      reg = XCreateRegion();
      pBox = RegionRects(nxagentWindowPriv(pWin)->clipShape);
      for (int i = 0;
           i < RegionNumRects(nxagentWindowPriv(pWin)->clipShape);
           i++)
      {
        XRectangle rect = {
          .x = pBox[i].x1,
          .y = pBox[i].y1,
          .width = pBox[i].x2 - pBox[i].x1,
          .height = pBox[i].y2 - pBox[i].y1
        };
        XUnionRectWithRegion(&rect, reg, reg);
      }

      #ifndef NXAGENT_SHAPE
      XShapeCombineRegion(nxagentDisplay, nxagentWindow(pWin),
                              ShapeClip, 0, 0, reg, ShapeSet);
      #endif

      XDestroyRegion(reg);
    }
    else
    {
      #ifdef DEBUG
      fprintf(stderr, "nxagentShapeWindow: wClip shape does not exist. Removing the shape.\n");
      #endif

      RegionEmpty(nxagentWindowPriv(pWin)->clipShape);

      #ifndef NXAGENT_SHAPE
      XShapeCombineMask(nxagentDisplay, nxagentWindow(pWin),
                            ShapeClip, 0, 0, None, ShapeSet);
      #endif
    }
  }
}
#endif /* SHAPE */

static int nxagentForceExposure(WindowPtr pWin, void * ptr)
{
  if (pWin -> drawable.class != InputOnly)
  {
    WindowPtr pRoot = pWin->drawable.pScreen->root;
    BoxRec Box = {
      .x1 = pWin->drawable.x,
      .y1 = pWin->drawable.y,
      .x2 = Box.x1 + pWin->drawable.width,
      .y2 = Box.y1 + pWin->drawable.height,
    };
    RegionPtr exposedRgn = RegionCreate(&Box, 1);

    RegionIntersect(exposedRgn, exposedRgn, &pRoot->winSize);

    if (exposedRgn != NULL && RegionNotEmpty(exposedRgn) != 0)
    {
      miWindowExposures(pWin, exposedRgn, NullRegion);
    }

    RegionDestroy(exposedRgn);
  }

  return WT_WALKCHILDREN;
}

void nxagentRefreshWindows(WindowPtr pWin)
{
  int action = 1;

  TraverseTree(pWin, nxagentForceExposure, &action);
}

void nxagentUnmapWindows(void)
{
  if (nxagentOption(Fullscreen) == 1)
  {
    for (int i = 0; i < screenInfo.numScreens; i++)
    {
      if (nxagentDefaultWindows[i])
      {
        XUnmapWindow(nxagentDisplay, nxagentDefaultWindows[i]);
      }
    }
  }

  NXFlushDisplay(nxagentDisplay, NXFlushLink);
}

void nxagentMapDefaultWindows(void)
{
  for (int i = 0; i < screenInfo.numScreens; i++)
  {
    WindowPtr pWin = screenInfo.screens[i]->root;
    ScreenPtr pScreen = pWin -> drawable.pScreen;

    MapWindow(pWin, serverClient);

    if (nxagentOption(Rootless) == 0)
    {
      /*
       * Show the NX splash screen.
       */

      #ifdef TEST
      fprintf(stderr, "nxagentMapDefaultWindows: Showing the splash window.\n");
      #endif

      nxagentShowSplashWindow(nxagentDefaultWindows[pScreen->myNum]);

      /*
       * Map the default window. Defer the mapping if the session is
       * of shadow type. If no WM is running on the remote display,
       * map the window soon anyway: this avoids a flickering effect
       * on the !M logo if the shadow session is displayed from a
       * Windows client.
       */

      if (nxagentOption(Shadow) == 0 || !nxagentWMIsRunning)
      {
        #ifdef TEST
        fprintf(stderr, "nxagentMapDefaultWindows: Mapping default window id [%ld].\n",
                    nxagentDefaultWindows[pScreen->myNum]);
        #endif

        XMapWindow(nxagentDisplay, nxagentDefaultWindows[pScreen->myNum]);

        if (nxagentOption(Fullscreen) == 1 && nxagentWMIsRunning)
        {
          nxagentMaximizeToFullScreen(pScreen);
        }
      }

      /*
       * Map and raise the input window.
       */

      XMapWindow(nxagentDisplay, nxagentInputWindows[pScreen->myNum]);

      /*
       * At reconnection the Input Window is
       * raised in nxagentReconnectAllWindows,
       * after the Root Window is mapped.
       */

      if (nxagentReconnectTrap == 0)
      {
        XRaiseWindow(nxagentDisplay, nxagentInputWindows[pScreen->myNum]);
      }
    }

    /*
     * Send a SetSelectionOwner request
     * to notify of the agent start.
     */

    XSetSelectionOwner(nxagentDisplay, serverCutProperty,
                           nxagentDefaultWindows[i], CurrentTime);
  }

  /*
   * Map the icon window.
   */

  if (nxagentIconWindow != None)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentMapDefaultWindows: Mapping icon window id [%ld].\n",
                nxagentIconWindow);
    #endif

    XMapWindow(nxagentDisplay, nxagentIconWindow);
  }

  /*
   * Ensure that the fullscreen window gets the focus.
   */

  if (nxagentFullscreenWindow != 0)
  {
    XSetInputFocus(nxagentDisplay, nxagentFullscreenWindow,
                       RevertToParent, CurrentTime);
  }

  #ifdef TEST
  fprintf(stderr, "nxagentMapDefaultWindows: Completed mapping of default windows.\n");
  #endif
}

Bool nxagentDisconnectAllWindows(void)
{
  Bool succeeded = True;

  #if defined(NXAGENT_RECONNECT_DEBUG) || defined(NXAGENT_RECONNECT_WINDOW_DEBUG)
  fprintf(stderr, "nxagentDisconnectAllWindows\n");
  #endif

  for (int i = 0; i < screenInfo.numScreens; i++)
  {
    WindowPtr pWin = screenInfo.screens[i]->root;
    nxagentTraverseWindow( pWin, nxagentDisconnectWindow, &succeeded);
    nxagentDefaultWindows[i] = None;
  }

  #ifdef NXAGENT_RECONNECT_WINDOW_DEBUG
  fprintf(stderr, "nxagentDisconnectAllWindows: all windows disconnected\n");
  #endif

  return succeeded;
}

/*
 * FIXME: We are giving up reconnecting those void *
 * that are not resource, and we are just disconnecting them.
 * Perhaps we could do better and reconnect them.
 */

void nxagentDisconnectWindow(void * p0, XID x1, void * p2)
{
  WindowPtr pWin = (WindowPtr)p0;
  Bool*     pBool = (Bool*)p2;
  CursorPtr pCursor = wCursor(pWin);
  ScreenPtr pScreen = pWin -> drawable.pScreen;

  if ((pCursor = wCursor(pWin)) &&
         nxagentCursorPriv(pCursor, pScreen) &&
           nxagentCursor(pCursor, pScreen))
  {
    #ifdef NXAGENT_RECONNECT_CURSOR_DEBUG_disabled
    char msg[] = "nxagentDisconnectWindow:";

    nxagentPrintCursorInfo(pCursor, msg);
    #endif

    #ifdef NXAGENT_RECONNECT_CURSOR_DEBUG
    fprintf(stderr, "nxagentDisconnectWindow: window %p - disconnecting cursor %p ID %lx\n",
                pWin, pCursor, nxagentCursor(pCursor, pScreen));
    #endif

    nxagentDisconnectCursor(pCursor, (XID)0, pBool);

    if (!*pBool)
    {
      #ifdef WARNING
      fprintf(stderr, "nxagentDisconnectWindow: WARNING failed disconnection of cursor at [%p]"
                  " for window at [%p]: ignoring it.\n", (void*)pCursor, (void*)pWin);
      #endif

      *pBool = True;
    }
  }
  #ifdef NXAGENT_RECONNECT_CURSOR_DEBUG
  else if (pCursor)
  {
    fprintf(stderr, "nxagentDisconnectWindow: window %p - cursor %p already disconnected\n",
                pWin, pCursor);
  }
  #endif

  if ((nxagentRealWindowProp) && (!nxagentWindowTopLevel(pWin)))
  {
    Atom prop = MakeAtom("NX_REAL_WINDOW", strlen("NX_REAL_WINDOW"), True);

    if (DeleteProperty(pWin, prop) != Success)
    {
        fprintf(stderr, "nxagentDisconnectWindow: Deleting NX_REAL_WINDOW failed.\n");
    }
    #ifdef DEBUG
    else
    {
      fprintf(stderr, "nxagentDisconnectWindow: Deleting NX_REAL_WINDOW from Window ID [%x].\n", nxagentWindowPriv(pWin)->window);
    }
    #endif
  }

  nxagentWindow(pWin) = None;

  if (nxagentDrawableStatus((DrawablePtr) pWin) == NotSynchronized)
  {
    nxagentDestroyCorruptedResource((DrawablePtr) pWin, RT_NX_CORR_WINDOW);
  }
}

Bool nxagentReconnectAllWindows(void *p0)
{
  /*
    access the parameter like this if this function needs it in future:
    int flexibility = *(int *) p0;
  */

  #if defined(NXAGENT_RECONNECT_DEBUG) || defined(NXAGENT_RECONNECT_WINDOW_DEBUG)
  fprintf(stderr, "nxagentReconnectAllWindows\n");
  #endif

  if (screenInfo.screens[0]->root -> backgroundState == BackgroundPixmap &&
          screenInfo.screens[0]->root -> background.pixmap == NULL)
  {
    FatalError("nxagentReconnectAllWindows: correct the FIXME\n");
  }

  if (nxagentOption(Fullscreen))
  {
    screenInfo.screens[0]->root -> origin.x = nxagentOption(RootX);
    screenInfo.screens[0]->root -> origin.y = nxagentOption(RootY);
  }

  if (!nxagentLoopOverWindows(nxagentReconnectWindow))
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentReconnectAllWindows: couldn't recreate windows\n");
    #endif

    return False;
  }

  #ifdef NXAGENT_RECONNECT_WINDOW_DEBUG
  XSync(nxagentDisplay, 0);
  fprintf(stderr, "nxagentReconnectAllWindows: all windows recreated\n");
  #endif

  if (!nxagentLoopOverWindows(nxagentReconfigureWindow))
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentReconnectAllWindows: couldn't reconfigure windows\n");
    #endif

    return False;
  }

  /*
   * After the Root Window has
   * been mapped, the Input
   * Windows is raised.
   */

  if (nxagentOption(Rootless) == 0)
  {
    for (int i = 0; i < screenInfo.numScreens; i++)
    {
      XRaiseWindow(nxagentDisplay, nxagentInputWindows[i]);
    }
  }

  nxagentFlushConfigureWindow();

  if (nxagentOption(Fullscreen))
  {
    screenInfo.screens[0]->root -> origin.x = 0;
    screenInfo.screens[0]->root -> origin.y = 0;
  }

  #ifdef NXAGENT_RECONNECT_WINDOW_DEBUG
  XSync(nxagentDisplay, 0);
  fprintf(stderr, "nxagentReconnectAllWindows: All windows reconfigured.\n");
  #endif

  if (nxagentInitClipboard(screenInfo.screens[0]->root) == -1)
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentReconnectAllWindows: WARNING! Couldn't initialize the clipboard.\n");
    #endif

    return False;
  }

  #ifdef NXAGENT_RECONNECT_WINDOW_DEBUG
  XSync(nxagentDisplay, 0);
  fprintf(stderr, "nxagentReconnectAllWindows: Clipboard initialized.\n");
  #endif

  #ifdef VIEWPORT_FRAME
  /*
   * We move the viewport frames out of the way on the X server side.
   */

  if (nxagentViewportFrameLeft &&
          nxagentViewportFrameRight &&
              nxagentViewportFrameAbove &&
                  nxagentViewportFrameBelow)
  {
    XMoveWindow(nxagentDisplay, nxagentWindow(nxagentViewportFrameLeft),
                    -NXAGENT_FRAME_WIDTH, 0);
    XMoveWindow(nxagentDisplay, nxagentWindow(nxagentViewportFrameRight),
                    nxagentOption(RootWidth), 0);
    XMoveWindow(nxagentDisplay, nxagentWindow(nxagentViewportFrameAbove),
                    0, -NXAGENT_FRAME_WIDTH);
    XMoveWindow(nxagentDisplay, nxagentWindow(nxagentViewportFrameBelow),
                    0, nxagentOption(RootHeight));
  }
  #endif /* #ifdef VIEWPORT_FRAME */

  return True;
}

Bool nxagentSetWindowCursors(void *p0)
{
  /*
    access the parameter like this if this function needs it in future:
    int flexibility = *(int *) p0;
   */

  #if defined(NXAGENT_RECONNECT_DEBUG) || defined(NXAGENT_RECONNECT_WINDOW_DEBUG)
  fprintf(stderr, "nxagentSetWindowCursors: Going to loop over the windows.\n");
  #endif

  if (!nxagentLoopOverWindows(nxagentReconfigureWindowCursor))
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentSetWindowCursors: WARNING! Couldn't configure all windows' cursors.\n");
    #endif

    return False;
  }

  #ifdef NXAGENT_RECONNECT_WINDOW_DEBUG
  fprintf(stderr, "nxagentSetWindowCursors: All cursors configured.\n");
  #endif

  nxagentReDisplayCurrentCursor();

  return True;
}

static void nxagentTraverseWindow(
  WindowPtr pWin,
  void (*pF)(void *, XID, void *),
  void * p)
{
  pF(pWin, 0, p);

  if (pWin -> nextSib)
  {
    nxagentTraverseWindow(pWin -> nextSib, pF, p);
  }

  if (pWin -> firstChild)
  {
    nxagentTraverseWindow(pWin -> firstChild, pF, p);
  }
}

static Bool nxagentLoopOverWindows(void (*pF)(void *, XID, void *))
{
  Bool windowSuccess = True;

  for (int i = 0; i < screenInfo.numScreens; i++)
  {
    nxagentTraverseWindow(screenInfo.screens[i]->root, pF, &windowSuccess);
  }

  return windowSuccess;
}

static void nxagentReconnectWindow(void * param0, XID param1, void * data_buffer)
{
  WindowPtr pWin = (WindowPtr)param0;
  Bool *pBool = (Bool*)data_buffer;
  Visual *visual;
  unsigned long mask;
  XSetWindowAttributes attributes;
  ColormapPtr pCmap;

  if (!pWin || !*pBool)
  {
    return;
  }

  #ifdef NXAGENT_RECONNECT_WINDOW_DEBUG
  fprintf(stderr, "nxagentReconnectWindow: %p - ID %lx\n", pWin, nxagentWindow(pWin));
  #endif

  if (pWin->drawable.class == InputOnly)
  {
    mask = CWEventMask;
    visual = CopyFromParent;
  }
  else
  {
    mask = CWEventMask | CWBackingStore;

    attributes.backing_store = NotUseful;

    if (pWin->optional)
    {
      mask |= CWBackingPlanes | CWBackingPixel;
      attributes.backing_planes = pWin->optional->backingBitPlanes;
      attributes.backing_pixel = pWin->optional->backingPixel;
    }

    /*
      FIXME: Do we need to set save unders attribute here?
    */
    if (nxagentSaveUnder)
    {
      mask |= CWSaveUnder;
      attributes.save_under = pWin->saveUnder;
    }

    if (pWin->parent)
    {
      if (pWin->optional && pWin->optional->visual != wVisual(pWin->parent))
      {
        visual = nxagentVisualFromID(pWin->drawable.pScreen, wVisual(pWin));
        mask |= CWColormap;
        if (pWin->optional->colormap)
        {
          pCmap = (ColormapPtr)LookupIDByType(wColormap(pWin), RT_COLORMAP);
          attributes.colormap = nxagentColormap(pCmap);
        }
        else
        {
          attributes.colormap = nxagentDefaultVisualColormap(visual);
        }
      }
      else
      {
        visual = CopyFromParent;
      }
    }
    else
    {
      /* root windows have their own colormaps at creation time */
      visual = nxagentVisualFromID(pWin->drawable.pScreen, wVisual(pWin));
      pCmap = (ColormapPtr)LookupIDByType(wColormap(pWin), RT_COLORMAP);
      mask |= CWColormap;
      attributes.colormap = nxagentColormap(pCmap);
    }
  }

  if (mask & CWEventMask)
  {
    attributes.event_mask = nxagentGetEventMask(pWin);
  }
  #ifdef WARNING
  else
  {
    attributes.event_mask = NoEventMask;
  }
  #endif

  #ifdef TEST
  fprintf(stderr, "nxagentReconnectWindow: Going to create new window.\n");
  fprintf(stderr, "nxagentReconnectWindow: Recreating %swindow at %p current event mask = %lX mask & CWEventMask = %ld "
              "event_mask = %lX\n",
                  nxagentWindowTopLevel(pWin) ? "toplevel " : "", (void*)pWin, pWin -> eventMask,
                      mask & CWEventMask, attributes.event_mask);
  #endif

  /*
   * FIXME: This quick hack is intended to solve a
   *        problem of NXWin X server for windows.
   *        The NXWin minimize the windows moving them
   *        out of the screen area, this behaviour
   *        can cause problem when a rootless session
   *        is disconnected and an apps is minimized.
   *        It will be solved with new Xorg version of
   *        the NXWin server.
   */

  if (nxagentOption(Rootless))
  {
    if (pWin -> drawable.x == -32000 && pWin -> drawable.y == -32000)
    {
      pWin -> drawable.x = (pWin -> drawable.pScreen -> width - pWin -> drawable.width) / 2;
      pWin -> drawable.y = (pWin -> drawable.pScreen -> height - pWin -> drawable.height) /2;
    }

    if (pWin -> origin.x == -32000 && pWin -> origin.y == -32000)
    {
      pWin -> origin.x = (pWin -> drawable.pScreen -> width - pWin -> drawable.width) / 2;
      pWin -> origin.y = (pWin -> drawable.pScreen -> height - pWin -> drawable.height) / 2;
    }
  }

  nxagentWindow(pWin) = XCreateWindow(nxagentDisplay,
                                      nxagentWindowParent(pWin),
                                      pWin->origin.x -
                                      wBorderWidth(pWin),
                                      pWin->origin.y -
                                      wBorderWidth(pWin),
                                      pWin->drawable.width,
                                      pWin->drawable.height,
                                      pWin->borderWidth,
                                      pWin->drawable.depth,
                                      pWin->drawable.class,
                                      visual,
                                      mask,
                                      &attributes);

  if (nxagentReportPrivateWindowIds)
  {
    fprintf (stderr, "NXAGENT_WINDOW_ID: PRIVATE_WINDOW,WID:[0x%x]\n", nxagentWindowPriv(pWin)->window);
  }
  #ifdef TEST
  fprintf(stderr, "nxagentReconnectWindow: Created new window with id [0x%x].\n",
              nxagentWindowPriv(pWin)->window);
  #endif

  /*
   * We have to set the WM_DELETE_WINDOW protocols
   * on every top level window, because we don't know
   * if a client handles this.
   */

  if (nxagentOption(Rootless) && (pWin != screenInfo.screens[0]->root))
  {
    if (nxagentWindowTopLevel(pWin))
    {
      Atom prop = nxagentMakeAtom("WM_PROTOCOLS", strlen("WM_PROTOCOLS"), True);

      XlibAtom atom = nxagentMakeAtom("WM_DELETE_WINDOW", strlen("WM_DELETE_WINDOW"), True);

      XSetWMProtocols(nxagentDisplay, nxagentWindow(pWin), &atom, 1);

      nxagentAddPropertyToList(prop, pWin);
    }

    nxagentExportAllProperty(pWin);

    if (nxagentWindowTopLevel(pWin))
    {
      int ret;
      Atom type;
      int format;
      unsigned long nItems, bytesLeft;
      XSizeHints hints = {0};
      unsigned char *data = NULL;
      #ifdef _XSERVER64
      unsigned char *data64 = NULL;
      #endif

      ret = GetWindowProperty(pWin,
                                  XA_WM_NORMAL_HINTS,
                                      0, sizeof(XSizeHints),
                                          False, XA_WM_SIZE_HINTS,
                                              &type, &format, &nItems, &bytesLeft, &data);

      /*
       * 72 is the number of bytes returned by
       * sizeof(XSizeHints) on 32 bit platforms.
       */

      if (ret == Success &&
              ((format >> 3) * nItems) == 72 &&
                  bytesLeft == 0 &&
                      type == XA_WM_SIZE_HINTS)
      {
        XSizeHints *props;
        #ifdef TEST
        fprintf(stderr, "nxagentReconnectWindow: setting WMSizeHints on window %p [%lx - %lx].\n",
                    (void*)pWin, pWin -> drawable.id, nxagentWindow(pWin));
        #endif

        #ifdef _XSERVER64
        data64 = (unsigned char *) malloc(sizeof(XSizeHints) + 4);

        for (int i = 0; i < 4; i++)
        {
          *(data64 + i) = *(data + i);
        }

        *(((int *) data64) + 1) = 0;

        for (int i = 8; i < sizeof(XSizeHints) + 4; i++)
        {
          *(data64 + i) = *(data + i - 4);
        }

        props = (XSizeHints *) data64;
        #else
        props = (XSizeHints *) data;
        #endif   /* _XSERVER64 */

        hints = *props;
      }
      else
      {
        #ifdef WARNING
        fprintf(stderr, "nxagentReconnectWindow: Failed to get property WM_NORMAL_HINTS on window %p\n",
                    (void*)pWin);
        #endif
      }

      hints.flags |= (USPosition | PWinGravity);
      hints.x = pWin -> drawable.x;
      hints.y = pWin -> drawable.y;
      hints.win_gravity = StaticGravity;

      XSetWMNormalHints(nxagentDisplay,
                           nxagentWindow(pWin),
                              &hints);

      #ifdef _XSERVER64
      SAFE_free(data64);
      #endif
    }
  }

  if ((nxagentRealWindowProp) && (!nxagentWindowTopLevel(pWin)))
  {
    Atom prop = MakeAtom("NX_REAL_WINDOW", strlen("NX_REAL_WINDOW"), True);

    if (ChangeWindowProperty(pWin, prop, XA_WINDOW, 32, PropModeReplace, 1, nxagentWindowPriv(pWin), 1) != Success)
    {
      fprintf(stderr, "nxagentReconnectWindow: Updating NX_REAL_WINDOW failed.\n");
    }
    #ifdef DEBUG
    else
    {
      fprintf(stderr, "nxagentReconnectWindow: Updated NX_REAL_WINDOW for Window ID [%x].\n", nxagentWindowPriv(pWin)->window);
    }
    #endif
  }

  if (nxagentDrawableStatus((DrawablePtr) pWin) == NotSynchronized)
  {
    nxagentAllocateCorruptedResource((DrawablePtr) pWin, RT_NX_CORR_WINDOW);
  }
}

static void nxagentReconfigureWindowCursor(void * param0, XID param1, void * data_buffer)
{
  WindowPtr pWin = (WindowPtr)param0;
  Bool *pBool = (Bool*)data_buffer;
  CursorPtr   pCursor;
  ScreenPtr   pScreen;

  if (!pWin || !*pBool || !(pCursor = wCursor(pWin)))
  {
    return;
  }

  pScreen = pWin -> drawable.pScreen;

  if (!(nxagentCursorPriv(pCursor, pScreen)))
  {
    return;
  }

  #ifdef DEBUG
  fprintf(stderr, "nxagentReconfigureWindowCursor: %p - ID %lx geometry (%d,%d,%d,%d) "
                  "cursor %p - ID %lx\n",
                  pWin, nxagentWindow(pWin),
                  pWin -> drawable.x,
                  pWin -> drawable.y,
                  pWin -> drawable.width,
                  pWin -> drawable.height,
                  pCursor, nxagentCursor(pCursor, pScreen));
  #endif

  if (nxagentCursor(pCursor, pScreen) == None)
  {
    #ifdef NXAGENT_RECONNECT_WINDOW_DEBUG
    fprintf(stderr, "nxagentReconfigureWindowCursor: reconnecting valid cursor %lx\n",
                (void*)pCursor);
    #endif

    nxagentReconnectCursor(pCursor, 0, pBool);

    if (!*pBool)
    {
      #ifdef WARNING
      fprintf(stderr, "nxagentReconfigureWindowCursor: WARNING "
                  "failed reconnection of cursor at [%p] for window at [%p]: ignoring it.\n",
                      (void*)pCursor, (void*)pWin);
      #endif

      *pBool = True;
    }
  }

  if (nxagentOption(Rootless))
  {
    XDefineCursor(nxagentDisplay,nxagentWindow(pWin),nxagentCursor(pCursor,pScreen));
  }
}

static void nxagentReconfigureWindow(void * param0, XID param1, void * data_buffer)
{
  WindowPtr pWin = (WindowPtr)param0;
  unsigned long mask = 0;

  #ifdef DEBUG
  fprintf(stderr, "nxagentReconfigureWindow: pWin %p - ID %lx\n", pWin, nxagentWindow(pWin));
  #endif

  if (pWin -> drawable.class == InputOnly)
  {
    mask = CWWinGravity | CWEventMask | CWDontPropagate | CWOverrideRedirect | CWCursor;
  }
  else
  {
    mask = CWBackPixmap | CWBackPixel | CWBorderPixmap | CWBorderPixel |
         CWBitGravity | CWWinGravity | CWBackingStore | CWBackingPlanes |
         CWBackingPixel | CWOverrideRedirect | CWSaveUnder | CWEventMask |
         CWDontPropagate | CWColormap | CWCursor;
  }
  nxagentChangeWindowAttributes(pWin, mask);

  #ifdef SHAPE
  if (nxagentWindowPriv(pWin) -> boundingShape)
  {
    RegionDestroy(nxagentWindowPriv(pWin) -> boundingShape);
    nxagentWindowPriv(pWin) -> boundingShape = NULL;
  }

  if (nxagentWindowPriv(pWin) -> clipShape)
  {
    RegionDestroy(nxagentWindowPriv(pWin) -> clipShape);
    nxagentWindowPriv(pWin) -> clipShape = NULL;
  }
  nxagentShapeWindow(pWin);
  #endif

  if (pWin != screenInfo.screens[0]->root)
  {
    if (pWin->realized)
    {
        nxagentRealizeWindow (pWin);
    }
/*
XXX: This would break Motif menus.
     If pWin is mapped but not realized, a following UnmapWindow() wouldn't
     do anything, leaving this mapped window around. XMapWindow()
     is called in nxagentRealizeWindow() and there it is enough.

    else if (pWin->mapped)
    {
      XMapWindow(nxagentDisplay, nxagentWindow(pWin));
    }
*/
    else if (nxagentOption(Rootless) && pWin -> overrideRedirect == 0 &&
                 nxagentWindowTopLevel(pWin) && nxagentIsIconic(pWin))
    {
      MapWindow(pWin, serverClient);
      XIconifyWindow(nxagentDisplay, nxagentWindow(pWin), pWin -> drawable.pScreen -> myNum);
    }
  }
  else if (nxagentOption(Rootless) == 0)
  {
    /*
     * Map the root window.
     */

    XMoveWindow(nxagentDisplay, nxagentWindow(pWin),
                    nxagentOption(RootX), nxagentOption(RootY));
    XMapWindow(nxagentDisplay, nxagentWindow(pWin));
  }
}

Bool nxagentCheckIllegalRootMonitoring(WindowPtr pWin, Mask mask)
{
  Mask invalidMask = SubstructureRedirectMask | ResizeRedirectMask | ButtonPressMask;

  if (nxagentOption(Rootless) &&
        pWin == screenInfo.screens[0]->root &&
          (mask & invalidMask))
  {
    return True;
  }

  return False;
}

#ifdef TEST
Bool nxagentCheckWindowIntegrity(WindowPtr pWin)
{
  Bool integrity = True;
  XImage *image;
  char *data;
  int format;
  unsigned long plane_mask = AllPlanes;
  unsigned int width, height, length, depth;

  width = pWin -> drawable.width;
  height = pWin -> drawable.height;
  depth = pWin -> drawable.depth;
  format = (depth == 1) ? XYPixmap : ZPixmap;

  if (width && height)
  {
     length = nxagentImageLength(width, height, format, 0, depth);
     data = calloc(1, length);
     if (data == NULL)
     {
       FatalError("nxagentCheckWindowIntegrity: Failed to allocate a buffer of size %d.\n", length);
     }

     image = XGetImage(nxagentDisplay, nxagentWindow(pWin), 0, 0,
                                   width, height, plane_mask, format);
     if (image == NULL)
     {
       fprintf(stderr, "XGetImage: Failed.\n");
       return False;
     }

     fbGetImage((DrawablePtr)pWin, 0, 0, width, height, format, plane_mask, data);

     if (image && memcmp(image->data, data, length) != 0)
     {
       integrity = False;

       #ifdef TEST
       char *p = image->data, *q = data;

       for (int i = 0; i < length; i++)
       {
         if (p[i] != q[i])
         {
           fprintf(stderr, "[%d] %d - %d !!!!!!!!!!!!!!!!!!! **************** !!!!!!!!!!!!!!!!!\n", i, p[i], q[i]);
         }
         else
         {
           fprintf(stderr, "[%d] %d - %d\n", i, p[i], q[i]);
         }
       }
       #endif

       #ifdef WARNING
       fprintf(stderr, "nxagentCheckWindowIntegrity: Window %p width %d, height %d, has been realized "
                 "but the data buffer still differs.\n", (void*) pWin, width, height);
       fprintf(stderr, "nxagentCheckWindowIntegrity: bytes_per_line = %d byte pad %d format %d.\n",
                 image -> bytes_per_line, nxagentImagePad(width, height, 0, depth), image->format);

       fprintf(stderr, "nxagentCheckWindowIntegrity: image is corrupted!!\n");
       #endif
     }
     else
     {
       #ifdef WARNING
       fprintf(stderr, "nxagentCheckWindowIntegrity: Window %p has been realized "
                   "now remote and framebuffer data are synchronized.\n", (void*) pWin);
       #endif
     }

     if (image)
     {
       XDestroyImage(image);
     }

     SAFE_free(data);
  }
  else
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentCheckWindowIntegrity: ignored window %p with geometry (%d,%d).\n",
                (void*) pWin, width, height);
    #endif
  }

  return integrity;
}
#endif /* TEST */

Bool nxagentIsIconic(WindowPtr pWin)
{
  int           iReturn;
  unsigned long ulReturnItems;
  unsigned long ulReturnBytesLeft;
  Atom          atomReturnType;
  int           iReturnFormat;
  unsigned char *pszReturnData = NULL;

  if (!wUserProps (pWin))
  {
    return 0;
  }

  iReturn = GetWindowProperty(pWin, MakeAtom("WM_STATE", 8, False), 0, sizeof(CARD32), False,
                              AnyPropertyType, &atomReturnType, &iReturnFormat,
                              &ulReturnItems, &ulReturnBytesLeft, &pszReturnData);

  if (iReturn == Success)
  {
    return (((CARD32 *)pszReturnData)[0] == IconicState);
  }
  else
  {
    return 0;
  }
}

/* pass Eventmask to the real X server (for the rootless toplevel window only) */
void nxagentSetTopLevelEventMask(WindowPtr pWin)
{
  if (nxagentOption(Rootless) && nxagentWindowTopLevel(pWin))
  {
    XSetWindowAttributes attributes = {.event_mask = nxagentGetEventMask(pWin)};
    XChangeWindowAttributes(nxagentDisplay, nxagentWindow(pWin), CWEventMask, &attributes);
  }
}

/*
 * Run nxagentConfigureWindow() on all windows in
 * nxagentConfiguredWindowList and move them from the list
 * afterwards. The list will be empty then.
 *
 * This is also taking care of entries in nxagentExposeQueue that need
 * to be synchronized with the real X server.
 */
void nxagentFlushConfigureWindow(void)
{
  ConfiguredWindowStruct *index;
  XWindowChanges changes;

  index = nxagentConfiguredWindowList;

  while (index)
  {
    if (index -> next == NULL)
    {
      break;
    }

    index = index -> next;
  }

  while (index)
  {
    WindowPtr pWin = index -> pWin;
    unsigned int valuemask = index -> valuemask;

    if (pWin && valuemask)
    {
      nxagentConfigureWindow(pWin, valuemask);
    }

    if (index == nxagentConfiguredWindowList)
    {
      SAFE_free(index);
      break;
    }
    else
    {
      ConfiguredWindowStruct *tmp = index;
      index = index -> prev;
      SAFE_free(tmp);
    }
  }

  nxagentConfiguredWindowList = NULL;

  for (int j = 0; j < nxagentExposeQueue.length; j++)
  {
    int i = (nxagentExposeQueue.start + j) % EXPOSED_SIZE;

    if (nxagentExposeQueue.exposures[i].synchronize == 1)
    {
      changes.x = nxagentExposeQueue.exposures[i].serial;
      changes.y = -2;

      #ifdef DEBUG
      fprintf(stderr, "nxagentFlushConfigureWindow: Sending synch ConfigureWindow for "
                  "index [%d] serial [%d].\n", i, nxagentExposeQueue.exposures[i].serial);
      #endif

      XConfigureWindow(nxagentDisplay, nxagentConfiguredSynchroWindow,
                           CWX | CWY, &changes);

      nxagentExposeQueue.exposures[i].synchronize = 0;
    }
  }

  nxagentSendDeferredBackgroundExposures();

  return;
}

/*
 * from "Definition of the Porting Layer for X v11 Sample Server":
 *
 * If this routine is not NULL, DIX calls it shortly after calling
 * ValidateTree, passing it the same arguments. This is useful for
 * managing multi-layered framebuffers. The sample server sets this to
 * NULL.
 */
void nxagentPostValidateTree(WindowPtr pParent, WindowPtr pChild, VTKind kind)
{
  /*
    FIXME: Do we need this here?

  nxagentFlushConfigureWindow();
  */

  return;
}

/*
 * Add the given window to the beginning of
 * nxagentconfiguredWindowList. This list collects all windows that
 * need to be reconfigured on the real X server. valuemask defines
 * what changes need to be done. The required values (like position,
 * size, ...) are already stored in pWin).
 *
 * Note that we just add the window to the list here. The actual work
 * will be done by nxagentFlushConfigureWindow() later.
 */
void nxagentAddConfiguredWindow(WindowPtr pWin, unsigned int valuemask)
{
  unsigned int mask;

  mask = valuemask & (CWSibling | CWX | CWY | CWWidth | CWHeight |
                   CWBorderWidth | CWStackMode | CW_Map | CW_Update | CW_Shape);

  valuemask &= ~(CWSibling | CWX | CWY | CWWidth | CWHeight | CWBorderWidth | CWStackMode);

  if (mask & CWX &&
          nxagentWindowPriv(pWin)->x !=
              pWin->origin.x - wBorderWidth(pWin))
  {
    valuemask |= CWX;
  }

  if (mask & CWY &&
          nxagentWindowPriv(pWin)->y !=
              pWin->origin.y - wBorderWidth(pWin))
  {
    valuemask |= CWY;
  }

  if (mask & CWWidth &&
          nxagentWindowPriv(pWin)->width !=
              pWin->drawable.width)
  {
    valuemask |= CWWidth;
  }

  if (mask & CWHeight &&
          nxagentWindowPriv(pWin)->height !=
              pWin->drawable.height)
  {
    valuemask |= CWHeight;
  }

  if (mask & CWBorderWidth &&
          nxagentWindowPriv(pWin)->borderWidth !=
              pWin->borderWidth)
  {
    valuemask |= CWBorderWidth;
  }

  if (mask & CWStackMode &&
          nxagentWindowPriv(pWin)->siblingAbove !=
              nxagentWindowSiblingAbove(pWin))
  {
    valuemask |= CWStackMode;
  }

  {
    ConfiguredWindowStruct *tmp = nxagentConfiguredWindowList;

    nxagentConfiguredWindowList = malloc(sizeof(ConfiguredWindowStruct));
    nxagentConfiguredWindowList -> next = tmp; /* can be NULL */
    nxagentConfiguredWindowList -> prev = NULL;
    nxagentConfiguredWindowList -> pWin = pWin;
    nxagentConfiguredWindowList -> valuemask = valuemask;

    if (tmp)
    {
      tmp -> prev = nxagentConfiguredWindowList;
    }
  }

  return;
}

/*
 * remove pWin from nxgentConfigureWindowList
 *
 * This is just updating the linked list and freeing the
 * given entry. It will not perform any X stuff
 */
void nxagentDeleteConfiguredWindow(WindowPtr pWin)
{
  ConfiguredWindowStruct *index, *previous, *tmp;

  index = nxagentConfiguredWindowList;

  while (index)
  {
    WindowPtr pDel = index -> pWin;

    if (pDel == pWin)
    {
      if (index -> prev == NULL && index -> next == NULL)
      {
        SAFE_free(nxagentConfiguredWindowList);
        return;
      }
      else if (index -> prev == NULL)
      {
        tmp = nxagentConfiguredWindowList;
        index = nxagentConfiguredWindowList = tmp -> next;
        SAFE_free(tmp);
        nxagentConfiguredWindowList -> prev = NULL;

        continue;
      }
      else if (index -> next == NULL)
      {
        tmp = index;
        index = index -> prev;
        SAFE_free(tmp);
        index -> next = NULL;

        return;
      }

      previous = index -> prev;
      tmp = index;
      index = index -> next;
      previous -> next = index;
      index -> prev = previous;
      SAFE_free(tmp);

      continue;
    }

    index = index -> next;
  }

  return;
}

void nxagentAddStaticResizedWindow(WindowPtr pWin, unsigned long sequence, int offX, int offY)
{
  StaticResizedWindowStruct *tmp = nxagentStaticResizedWindowList;

  nxagentStaticResizedWindowList = malloc(sizeof(StaticResizedWindowStruct));
  nxagentStaticResizedWindowList -> next = tmp;
  nxagentStaticResizedWindowList -> prev = NULL;

  if (tmp)
  {
    tmp -> prev = nxagentStaticResizedWindowList;
  }

  nxagentStaticResizedWindowList -> pWin = pWin;
  nxagentStaticResizedWindowList -> sequence = sequence;
  nxagentStaticResizedWindowList -> offX = offX;
  nxagentStaticResizedWindowList -> offY = offY;
}

void nxagentDeleteStaticResizedWindow(unsigned long sequence)
{
  StaticResizedWindowStruct *index, *previous, *tmp;

  index = nxagentStaticResizedWindowList;

  while (index)
  {
    if (index -> sequence <= sequence)
    {
      if (index -> prev == NULL && index -> next == NULL)
      {
        SAFE_free(nxagentStaticResizedWindowList);
        return;
      }
      else if (index -> prev == NULL)
      {
        tmp = nxagentStaticResizedWindowList;
        index = nxagentStaticResizedWindowList = tmp -> next;
        SAFE_free(tmp);
        nxagentStaticResizedWindowList -> prev = NULL;

        continue;
      }
      else if (index -> next == NULL)
      {
        tmp = index;
        index = index -> prev;
        SAFE_free(tmp);
        index -> next = NULL;

        return;
      }

      previous = index -> prev;
      tmp = index;
      index = index -> next;
      previous -> next = index;
      index -> prev = previous;
      SAFE_free(tmp);

      continue;
    }

    index = index -> next;
  }

  return;
}

StaticResizedWindowStruct *nxagentFindStaticResizedWindow(unsigned long sequence)
{
  StaticResizedWindowStruct *index;
  StaticResizedWindowStruct *ret = NULL;

  if (nxagentStaticResizedWindowList == NULL)
  {
    return NULL;
  }

  index = nxagentStaticResizedWindowList;

  while (index && index -> sequence > sequence)
  {
    ret = index;
    index = index -> next;
  }

  return ret;
}

void nxagentEmptyBackingStoreRegion(void * param0, XID param1, void * data_buffer)
{
  WindowPtr pWin = (WindowPtr) param0;

  miBSWindowPtr pBackingStore = (miBSWindowPtr)pWin->backStorage;

  if (pBackingStore != NULL)
  {
    RegionEmpty(&pBackingStore->SavedRegion);

    #ifdef TEST
    fprintf(stderr, "nxagentEmptyBackingStoreRegion: Emptying saved region for window at [%p].\n", (void*) pWin);
    #endif

    if (pBackingStore -> pBackingPixmap != NULL)
    {
      #ifdef TEST
      fprintf(stderr, "nxagentEmptyBackingStoreRegion: Emptying corrupted region for drawable at [%p].\n",
                  (void*) pBackingStore -> pBackingPixmap);
      #endif

      nxagentUnmarkCorruptedRegion((DrawablePtr) pBackingStore -> pBackingPixmap, NullRegion);
    }
  }
}

void nxagentEmptyAllBackingStoreRegions(void)
{
  if (nxagentLoopOverWindows(nxagentEmptyBackingStoreRegion) == 0)
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentEmptyAllSavedRegions: Failed to empty backing store saved regions.\n");
    #endif
  }
}

void nxagentInitBSPixmapList(void)
{
  memset(nxagentBSPixmapList, 0, BSPIXMAPLIMIT * sizeof( StoringPixmapPtr));
}

int nxagentAddItemBSPixmapList(unsigned long id, PixmapPtr pPixmap, WindowPtr pWin, int bsx, int bsy)
{
  for (int i = 0; i < BSPIXMAPLIMIT; i++)
  {
    if (nxagentBSPixmapList[i] == NULL)
    {
      nxagentBSPixmapList[i] = malloc(sizeof(StoringPixmapRec));

      if (nxagentBSPixmapList[i] == NULL)
      {
        FatalError("nxagentAddItemBSPixmapList: Failed to allocate memory for nxagentBSPixmapList.\n");
      }

      nxagentBSPixmapList[i] -> storingPixmapId = id;
      nxagentBSPixmapList[i] -> pStoringPixmap = pPixmap;
      nxagentBSPixmapList[i] -> pSavedWindow = pWin;
      nxagentBSPixmapList[i] -> backingStoreX = bsx;
      nxagentBSPixmapList[i] -> backingStoreY = bsy;

      #ifdef TEST
      fprintf(stderr, "nxagentAddItemBSPixmapList: Added Pixmap with id [%lu] to nxagentBSPixmapList.\n", id);
      #endif

      return 1;
    }

    if (nxagentBSPixmapList[i] -> storingPixmapId == id)
    {
      nxagentBSPixmapList[i] -> pStoringPixmap = pPixmap;
      nxagentBSPixmapList[i] -> pSavedWindow = pWin;
      nxagentBSPixmapList[i] -> backingStoreX = bsx;
      nxagentBSPixmapList[i] -> backingStoreY = bsy;

      #ifdef TEST
      fprintf(stderr, "nxagentAddItemBSPixmapList: Updated existing item for id [%lu].\n", id);
      #endif

      return 1;
    }
  }

  #ifdef TEST
  fprintf(stderr, "nxagentAddItemBSPixmapList: WARNING! List item full.\n");
  #endif

  return 0;
}

int nxagentRemoveItemBSPixmapList(unsigned long pixmapId)
{
  if (pixmapId == 0 || nxagentBSPixmapList[0] == NULL)
  {
    return 0;
  }

  for (int i = 0; i < BSPIXMAPLIMIT; i++)
  {
    if ((nxagentBSPixmapList[i] != NULL) &&
            (nxagentBSPixmapList[i] -> storingPixmapId == pixmapId))
    {
      SAFE_free(nxagentBSPixmapList[i]);

      if (i < BSPIXMAPLIMIT - 1)
      {
        int j;

        for (j = i; j < BSPIXMAPLIMIT -1; j++)
        {
          nxagentBSPixmapList[j] = nxagentBSPixmapList[j + 1];
        }

        if (nxagentBSPixmapList[j] == nxagentBSPixmapList[j - 1])
        {
          nxagentBSPixmapList[j] = NULL;
        }
      }

      #ifdef TEST
      fprintf(stderr, "nxagentRemoveItemBSPixmapList: Removed Pixmap with id [%lu] from list.\n",
                  pixmapId);
      #endif

      return 1;
    }
  }

  #ifdef TEST
  fprintf(stderr, "nxagentRemoveItemBSPixmapList: WARNING! Can't remove item [%lu]: item not found.\n",
              pixmapId);
  #endif

  return 0;
}

int nxagentEmptyBSPixmapList(void)
{
  for (int i = 0; i < BSPIXMAPLIMIT; i++)
  {
    SAFE_free(nxagentBSPixmapList[i]);
  }

  return 1;
}

StoringPixmapPtr nxagentFindItemBSPixmapList(unsigned long pixmapId)
{
  for (int i = 0; i < BSPIXMAPLIMIT; i++)
  {
    if ((nxagentBSPixmapList[i] != NULL) &&
            (nxagentBSPixmapList[i] -> storingPixmapId == pixmapId))
    {
      #ifdef TEST
      fprintf(stderr, "%s: pixmapId [%lu].\n", __func__, pixmapId);
      fprintf(stderr, "%s: nxagentBSPixmapList[%d] = [%p].\n", __func__,
                  i, (void *) nxagentBSPixmapList[i]);
      fprintf(stderr, "%s: nxagentBSPixmapList[%d] -> storingPixmapId [%lu].\n", __func__,
                  i, nxagentBSPixmapList[i] -> storingPixmapId);
      #endif

      return nxagentBSPixmapList[i];
    }
  }

  #ifdef WARNING
  fprintf(stderr, "%s: WARNING! Item not found.\n", __func__);
  #endif

  #ifdef TEST
  fprintf(stderr, "%s: Pixmap with id [%lu] not found.\n", __func__,
              pixmapId);
  #endif

  return NULL;
}
