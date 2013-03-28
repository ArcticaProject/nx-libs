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

#include "NX.h"
#include "NXlib.h"

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

static void nxagentTraverseWindow(WindowPtr, void(*)(pointer, XID, pointer), pointer);

static void nxagentDisconnectWindow(pointer, XID, pointer);

static Bool nxagentLoopOverWindows(void(*)(pointer, XID, pointer));

static void nxagentReconfigureWindowCursor(pointer, XID, pointer);

static void nxagentReconnectWindow(pointer, XID, pointer);

static void nxagentReconfigureWindow(pointer, XID, pointer);

static int nxagentForceExposure(WindowPtr pWin, pointer ptr);

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
  WindowPtr pWin = WindowTable[0];

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

static int nxagentFindWindowMatch(WindowPtr pWin, pointer ptr)
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
  int i;

  WindowMatchRec match;

  match.pWin = NullWindow;
  match.id   = window;

  for (i = 0; i < nxagentNumScreens; i++)
  {
    WalkTree(screenInfo.screens[i], nxagentFindWindowMatch, (pointer) &match);

    if (match.pWin) break;
  }

  return match.pWin;
}

Bool nxagentCreateWindow(pWin)
     WindowPtr pWin;
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
  fprintf(stderr, "nxagentSplashCount: %d\n", nxagentSplashCount);
#endif

  if (pWin->drawable.class == InputOnly) {
    mask = CWEventMask;
    visual = CopyFromParent;
  }
  else {
    mask = CWEventMask | CWBackingStore;

    if (pWin->optional)
    {
      mask |= CWBackingPlanes | CWBackingPixel;
      attributes.backing_planes = pWin->optional->backingBitPlanes;
      attributes.backing_pixel = pWin->optional->backingPixel;
    }

    attributes.backing_store = NotUseful;

    #ifdef TEST
    fprintf(stderr, "nxagentCreateWindow: Backing store on window at %p is %d.\n",
                (void*)pWin, attributes.backing_store);
    #endif

/*
FIXME: We need to set save under on the real display?
*/
    if (nxagentSaveUnder == True)
      {
        mask |= CWSaveUnder;
        attributes.save_under = False;
      }

    if (pWin->parent) {
      if (pWin->optional && pWin->optional->visual != wVisual(pWin->parent)) {
        visual = nxagentVisualFromID(pWin->drawable.pScreen, wVisual(pWin));
        mask |= CWColormap;
        if (pWin->optional->colormap) {
          pCmap = (ColormapPtr)LookupIDByType(wColormap(pWin), RT_COLORMAP);
          attributes.colormap = nxagentColormap(pCmap);
        }
        else
          attributes.colormap = nxagentDefaultVisualColormap(visual);
      }
      else if (pWin->optional)
       visual = CopyFromParent;
      else {
        visual = nxagentVisualFromID(pWin->drawable.pScreen, wVisual(pWin));
        mask |= CWColormap;
        attributes.colormap = nxagentDefaultVisualColormap(visual);
      }
    }
    else { /* root windows have their own colormaps at creation time */
      visual = nxagentVisualFromID(pWin->drawable.pScreen, wVisual(pWin));
      pCmap = (ColormapPtr)LookupIDByType(wColormap(pWin), RT_COLORMAP);
      mask |= CWColormap;
      attributes.colormap = nxagentColormap(pCmap);
    }
  }

  if (mask & CWEventMask)
  {
    nxagentGetEventMask(pWin, (Mask*)&attributes.event_mask);
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

  nxagentWindowPriv(pWin) -> corruptedRegion = REGION_CREATE(pWin -> drawable.pScreen, NULL, 1);

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

  #ifdef TEST
  fprintf(stderr, "nxagentCreateWindow: Created new window with id [%ld].\n",
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

#ifdef NXAGENT_SHAPE2
#ifdef SHAPE
  nxagentWindowPriv(pWin)->boundingShape = NULL;
  nxagentWindowPriv(pWin)->clipShape = NULL;
#endif /* SHAPE */
#else
#ifdef SHAPE
  nxagentWindowPriv(pWin)->boundingShape = REGION_CREATE(pWin->drawable.pScreen, NULL, 1);
  nxagentWindowPriv(pWin)->clipShape = REGION_CREATE(pWin->drawable.pScreen, NULL, 1);
#endif /* SHAPE */
#endif

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

Bool nxagentSomeWindowsAreMapped()
{
  WindowPtr pWin = WindowTable[0] -> firstChild;

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

Bool nxagentDestroyWindow(WindowPtr pWin)
{
  int i;
  int j;

  nxagentPrivWindowPtr pWindowPriv;

  if (nxagentScreenTrap == 1)
  {
    return 1;
  }

  nxagentClearClipboard(NULL, pWin);

  for (j = 0; j < nxagentExposeQueue.length; j++)
  {
    i = (nxagentExposeQueue.start + j) % EXPOSED_SIZE;

    if (nxagentExposeQueue.exposures[i].pWindow == pWin)
    {
      if (nxagentExposeQueue.exposures[i].localRegion != NullRegion)
      {
        REGION_DESTROY(pWin -> drawable.pScreen, nxagentExposeQueue.exposures[i].localRegion);
      }

      nxagentExposeQueue.exposures[i].localRegion = NullRegion;

      if (nxagentExposeQueue.exposures[i].remoteRegion != NullRegion)
      {
        REGION_DESTROY(pWin -> drawable.pScreen, nxagentExposeQueue.exposures[i].remoteRegion);
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
    REGION_DESTROY(pWin->drawable.pScreen,
                       pWindowPriv->boundingShape);
  }

  if (pWindowPriv->clipShape)
  {
    REGION_DESTROY(pWin->drawable.pScreen,
                       pWindowPriv->clipShape);
  }

  #endif

  #else

  REGION_DESTROY(pWin->drawable.pScreen,
                     pWindowPriv->boundingShape);

  REGION_DESTROY(pWin->drawable.pScreen,
                     pWindowPriv->clipShape);

  #endif

  if (pWindowPriv -> corruptedRegion)
  {
    REGION_DESTROY(pWin -> drawable.pScreen,
                       pWindowPriv -> corruptedRegion);

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

  nxagentAddConfiguredWindow(pWin, CWParent | CWX | CWY | CWWidth |
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
  XEvent e;

  if (nxagentOption(Rootless) == 1)
  {
    return;
  }

  if (switchOn == 0)
  {
    nxagentWMDetect();

    /*
     * The smart scheduler could be stopped while
     * waiting for the reply. In this case we need
     * to yield explicitly to avoid to be stuck in
     * the dispatch loop forever.
     */

    isItTimeToYield = 1;

    if (nxagentWMIsRunning == 0)
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

  memset(&e, 0, sizeof(e));

  e.xclient.type = ClientMessage;
  e.xclient.message_type = nxagentAtoms[13]; /* _NET_WM_STATE */
  e.xclient.display = nxagentDisplay;
  e.xclient.window = nxagentDefaultWindows[pScreen -> myNum];
  e.xclient.format = 32;
  e.xclient.data.l[0] = nxagentOption(Fullscreen) ? 1 : 0;
  e.xclient.data.l[1] = nxagentAtoms[14]; /* _NET_WM_STATE_FULLSCREEN */

  XSendEvent(nxagentDisplay, DefaultRootWindow(nxagentDisplay), False,
                 SubstructureRedirectMask, &e);

  if (switchOn == 1)
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
       * This should also flush
       * the NX link for us.
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

      XMoveWindow(nxagentDisplay, nxagentWindow(WindowTable[pScreen -> myNum]),
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
                                        HeightOfScreen(DefaultScreenOfDisplay(nxagentDisplay)), 0, 0);
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
                                      nxagentOption(RootHeight), 0, 0);
      }
    }

    if (nxagentOption(WMBorderWidth) > 0 && nxagentOption(WMTitleHeight) > 0)
    {
      nxagentChangeOption(X, nxagentOption(SavedX) -
                              nxagentOption(WMBorderWidth));
      nxagentChangeOption(Y, nxagentOption(SavedY) -
                              nxagentOption(WMTitleHeight));
    }
    else
    {
      nxagentChangeOption(X, nxagentOption(SavedX));
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

    XMoveWindow(nxagentDisplay, nxagentWindow(WindowTable[pScreen -> myNum]), 0, 0);
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
  int newX;
  int newY;
  int oldX;
  int oldY;

  Bool doMove = False;
  oldX = 0;
  oldY = 0;

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
                WindowTable[pScreen -> myNum] -> drawable.x,
                    WindowTable[pScreen -> myNum] -> drawable.y );
    #endif

    XMoveWindow(nxagentDisplay, nxagentWindow(WindowTable[pScreen -> myNum]),
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

      BoxRec hRect;
      BoxRec vRect;

      hRect.x1 = -newX;
      hRect.y1 = -newY;

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

      vRect.x1 = -newX;
      vRect.y1 = -newY;

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

void nxagentConfigureWindow(WindowPtr pWin, unsigned int mask)
{
  unsigned int valuemask;
  XWindowChanges values;
  int offX, offY;
  int i, j;

  offX = nxagentWindowPriv(pWin)->x - pWin->origin.x;
  offY = nxagentWindowPriv(pWin)->y - pWin->origin.y;

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
      mask = CWStackingOrder;
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
    mask |= CWX | CWY | CWWidth | CWHeight | CWBorderWidth | CWStackingOrder;
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

      for (j = 0; j < nxagentExposeQueue.length; j++)
      {
        i = (nxagentExposeQueue.start + j) % EXPOSED_SIZE;

        if (nxagentExposeQueue.exposures[i].pWindow == pWin &&
                nxagentExposeQueue.exposures[i].remoteRegion != NullRegion)
        {
          REGION_TRANSLATE(pWin -> drawable.pScreen, nxagentExposeQueue.exposures[i].remoteRegion, offX, offY);
        }
      }
    }

    XConfigureWindow(nxagentDisplay, nxagentWindow(pWin), valuemask, &values);

    MAKE_SYNC_CONFIGURE_WINDOW;
  }

  if (mask & CWStackingOrder &&
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
      Window *children_return;
      unsigned int nchildren_return;
      Window *pw;
      Status result;

      result = XQueryTree(nxagentDisplay, DefaultRootWindow(nxagentDisplay),
                              &root_return, &parent_return, &children_return, &nchildren_return);

      if (result)
      {
        pw = children_return;

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

        if (children_return)
        {
          XFree(children_return);
        }
      }
      else
      {
        fprintf(stderr, "nxagentConfigureWindow: Failed QueryTree request.\n ");
      }
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
   *  else if (mask & CWStackingOrder)
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

  #endif

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

void nxagentReparentWindow(pWin, pOldParent)
     WindowPtr pWin;
     WindowPtr pOldParent;
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

Bool nxagentChangeWindowAttributes(pWin, mask)
     WindowPtr pWin;
     unsigned long mask;
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
   * As we set this bit, whe must change dix in
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
    ColormapPtr pCmap;

    pCmap = (ColormapPtr) LookupIDByType(wColormap(pWin), RT_COLORMAP);

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

Bool nxagentRealizeWindow(WindowPtr pWin)
{
  if (nxagentScreenTrap == 1)
  {
    return True;
  }

  /*
   * Not needed.
   *
   * nxagentConfigureWindow(pWin, CWStackingOrder);
   *
   * nxagentFlushConfigureWindow();
   */

  nxagentAddConfiguredWindow(pWin, CWStackingOrder);
  nxagentAddConfiguredWindow(pWin, CW_Shape);

  /* add by dimbor */
  if (nxagentOption(Rootless) && nxagentWindowTopLevel(pWin))
  {
    Atom prop = MakeAtom("WM_STATE", strlen("WM_STATE"), True);
    nxagentWMStateRec wmState;
    wmState.state = 1; /* NormalState */
    wmState.icon = None;
    if (ChangeWindowProperty(pWin, prop, prop, 32, 0, 2, &wmState, 1) != Success)
      fprintf(stderr, "nxagentRealizeWindow: Additing WM_STATE fail.\n");
  }

  #ifdef SHAPE

  /*
   * Not needed.
   *
   * nxagentShapeWindow(pWin);
   */

  #endif /* SHAPE */

  /*
   * Mapping of the root window is called by
   * InitRootWindow in DIX. Skip the operation
   * if we are in rootless mode.
   */

  /*
   * if (!nxagentOption(Rootless) ||
   *         nxagentRootlessWindow != pWin)
   * {
   *   XMapWindow(nxagentDisplay, nxagentWindow(pWin));
   * }
   */

  #ifdef TEST
  if (nxagentOption(Rootless) && nxagentLastWindowDestroyed)
  {
    fprintf(stderr, "nxagentRealizeWindow: Window realized. Stopped termination for rootless session.\n");
  }
  #endif

  nxagentAddConfiguredWindow(pWin, CW_Map);

  nxagentLastWindowDestroyed = False;

  return True;
}

Bool nxagentUnrealizeWindow(pWin)
    WindowPtr pWin;
{
  if (nxagentScreenTrap)
  {
    return True;
  }

  /* add by dimbor */
  if (nxagentOption(Rootless) && nxagentWindowTopLevel(pWin))
  {
    Atom prop = MakeAtom("WM_STATE", strlen("WM_STATE"), True);
    nxagentWMStateRec wmState;
    wmState.state = 3; /* WithdrawnState */
    wmState.icon = None;
    if (ChangeWindowProperty(pWin, prop, prop, 32, 0, 2, &wmState, 1) != Success)
      fprintf(stderr, "nxagentUnRealizeWindow: Changing WM_STATE failed.\n");
  }

  XUnmapWindow(nxagentDisplay, nxagentWindow(pWin));

  return True;
}

void nxagentFrameBufferPaintWindow(WindowPtr pWin, RegionPtr pRegion, int what)
{

  void *PaintWindowBackgroundBackup;

  if (pWin->backgroundState == BackgroundPixmap)
  {
    pWin->background.pixmap = nxagentVirtualPixmap(pWin->background.pixmap);
  }

  if (pWin->borderIsPixel == False)
  {
    pWin->border.pixmap = nxagentVirtualPixmap(pWin->border.pixmap);
  }

  PaintWindowBackgroundBackup = pWin->drawable.pScreen -> PaintWindowBackground;

  pWin->drawable.pScreen -> PaintWindowBackground = nxagentFrameBufferPaintWindow;

  fbPaintWindow(pWin, pRegion, what);

  pWin->drawable.pScreen -> PaintWindowBackground = PaintWindowBackgroundBackup;

  if (pWin->backgroundState == BackgroundPixmap)
  {
    pWin->background.pixmap = nxagentRealPixmap(pWin->background.pixmap);
  }

  if (pWin->borderIsPixel == False)
  {
    pWin->border.pixmap = nxagentRealPixmap(pWin->border.pixmap);
  }
}

void nxagentPaintWindowBackground(pWin, pRegion, what)
     WindowPtr pWin;
     RegionPtr pRegion;
     int what;
{
  int i;

  RegionRec temp;

  if (pWin -> realized)
  {
    BoxPtr pBox;

    pBox = REGION_RECTS(pRegion);

    for (i = 0; i < REGION_NUM_RECTS(pRegion); i++)
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

  REGION_INIT(pWin -> drawable.pScreen, &temp, NullBox, 1);

  REGION_INTERSECT(pWin -> drawable.pScreen, &temp, pRegion, &pWin -> clipList);

  nxagentFrameBufferPaintWindow(pWin, &temp, what);

  REGION_UNINIT(pWin -> drawable.pScreen, &temp);
}

void nxagentPaintWindowBorder(WindowPtr pWin, RegionPtr pRegion, int what)
{
  RegionRec temp;

  /*
   * The framebuffer operations don't take care of
   * clipping to the actual area of the framebuffer
   * so we need to clip ourselves.
   */

  REGION_INIT(pWin -> drawable.pScreen, &temp, NullBox, 1);

  REGION_INTERSECT(pWin -> drawable.pScreen, &temp, pRegion, &pWin -> borderClip);

  nxagentFrameBufferPaintWindow(pWin, &temp, what);

  REGION_UNINIT(pWin -> drawable.pScreen, &temp);
}

void nxagentCopyWindow(WindowPtr pWin, xPoint oldOrigin, RegionPtr oldRegion)
{
  fbCopyWindow(pWin, oldOrigin, oldRegion);
}

void nxagentClipNotify(WindowPtr pWin, int dx, int dy)
{
  /*
   * nxagentConfigureWindow(pWin, CWStackingOrder);
   */

  nxagentAddConfiguredWindow(pWin, CWStackingOrder);
  nxagentAddConfiguredWindow(pWin, CW_Shape);

#ifdef NXAGENT_SHAPE
  return;
#else

#ifdef SHAPE

/*
 * nxagentShapeWindow(pWin);
 */

#endif /* SHAPE */

#endif /* NXAGENT_SHAPE */
}

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
   *  ourselves with both the remote X server and/or the window manager.
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
      int i;

      XSetWindowAttributes attributes;

      #ifdef TEST
      fprintf(stderr, "nxagentWindowExposures: Initializing expose queue.\n");
      #endif

      attributes.event_mask = StructureNotifyMask;

      for (i = 0; i < EXPOSED_SIZE; i++)
      {
        nxagentExposeQueue.exposures[i].pWindow = NULL;
        nxagentExposeQueue.exposures[i].localRegion = NullRegion;
        nxagentExposeQueue.exposures[i].remoteRegion = NullRegion;
        nxagentExposeQueue.exposures[i].remoteRegionIsCompleted = False;
        nxagentExposeQueue.exposures[i].serial = 0;
      }

      nxagentExposeQueue.length = 0;

      nxagentExposeSerial = 0;

      nxagentExposeQueue.start = 0;

      nxagentConfiguredSynchroWindow = XCreateWindow(nxagentDisplay, DefaultRootWindow(nxagentDisplay), 0, 0,
                                                           1, 1, 0, 0, InputOutput, 0, CWEventMask, &attributes);

      nxagentInitRemoteExposeRegion();

      nxagentExposeArrayIsInitialized = 1;
    }

    REGION_INIT(pWin -> drawable.pScreen, &temp, (BoxRec *) NULL, 1);

    if (pRgn != NULL)
    {
      if (REGION_NUM_RECTS(pRgn) > RECTLIMIT)
      {
        box = *REGION_EXTENTS(pWin -> drawable.pScreen, pRgn);

        REGION_EMPTY(pWin -> drawable.pScreen, pRgn);
        REGION_INIT(pWin -> drawable.pScreen, pRgn, &box, 1);
      }

      REGION_UNION(pWin -> drawable.pScreen, &temp, &temp, pRgn);
    }

    if (other_exposed != NULL)
    {
      REGION_UNION(pWin -> drawable.pScreen, &temp, &temp, other_exposed);
    }

    if (REGION_NIL(&temp) == 0)
    {
      REGION_TRANSLATE(pWin -> drawable.pScreen, &temp,
                           -(pWin -> drawable.x), -(pWin -> drawable.y));

      if (nxagentExposeQueue.length < EXPOSED_SIZE)
      {
        XWindowChanges changes;
        int index;

        index = (nxagentExposeQueue.start + nxagentExposeQueue.length) % EXPOSED_SIZE;

        nxagentExposeQueue.exposures[index].pWindow = pWin;

        nxagentExposeQueue.exposures[index].localRegion = REGION_CREATE(pwin -> drawable.pScreen, NULL, 1);

        if (nxagentOption(Rootless) && nxagentWindowPriv(pWin) &&
                (nxagentWindowPriv(pWin) -> isMapped == 0 ||
                     nxagentWindowPriv(pWin) -> visibilityState != VisibilityUnobscured))
        {
          nxagentExposeQueue.exposures[index].remoteRegion = REGION_CREATE(pwin -> drawable.pScreen, NULL, 1);

          REGION_UNION(pWin -> drawable.pScreen, nxagentExposeQueue.exposures[index].remoteRegion,
                           nxagentExposeQueue.exposures[index].remoteRegion, &temp);

          #ifdef TEST
          fprintf(stderr, "nxagentWindowExposures: Added region to remoteRegion for window [%ld] to position [%d].\n",
                      nxagentWindow(pWin), nxagentExposeQueue.length);
          #endif
        }
        else
        {
          REGION_UNION(pWin -> drawable.pScreen, nxagentExposeQueue.exposures[index].localRegion,
                           nxagentExposeQueue.exposures[index].localRegion, &temp);

          #ifdef TEST
          fprintf(stderr, "nxagentWindowExposures: Added region to localRegion for window [%ld] to position [%d].\n",
                      nxagentWindow(pWin), nxagentExposeQueue.length);
          #endif
        }

        nxagentExposeSerial = (nxagentExposeSerial - 1) % EXPOSED_SIZE;

        changes.x = nxagentExposeSerial;
        changes.y = -2;

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
          REGION_UNINIT(pWin -> drawable.pScreen, &temp);

          return;
        }
      }
      else
      {
        REGION_UNINIT(pWin -> drawable.pScreen, &temp);

        #ifdef TEST
        fprintf(stderr, "nxagentWindowExposures: WARNING! Reached maximum size of collect exposures vector.\n");
        #endif

        if ((pRgn != NULL && REGION_NOTEMPTY(pWin -> drawable.pScreen, pRgn) != 0) ||
               (other_exposed != NULL && REGION_NOTEMPTY(pWin -> drawable.pScreen, other_exposed) != 0))
        {
          nxagentUnmarkExposedRegion(pWin, pRgn, other_exposed);

          miWindowExposures(pWin, pRgn, other_exposed);
        }

        return;
      }
    }

    REGION_UNINIT(pWin -> drawable.pScreen, &temp);
  }

  if ((pRgn != NULL && REGION_NOTEMPTY(pWin -> drawable.pScreen, pRgn) != 0) ||
         (other_exposed != NULL && REGION_NOTEMPTY(pWin -> drawable.pScreen, other_exposed) != 0))
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

  if (pReg1 == pReg2) return True;

  if (pReg1 == NullRegion || pReg2 == NullRegion) return False;

  pBox1 = REGION_RECTS(pReg1);
  n1 = REGION_NUM_RECTS(pReg1);

  pBox2 = REGION_RECTS(pReg2);
  n2 = REGION_NUM_RECTS(pReg2);

  if (n1 != n2) return False;

  if (pBox1 == pBox2) return True;

  if (memcmp(pBox1, pBox2, n1 * sizeof(BoxRec))) return False;

  return True;
}

void nxagentShapeWindow(WindowPtr pWin)
{
  Region reg;
  BoxPtr pBox;
  XRectangle rect;
  int i;

  if (NXDisplayError(nxagentDisplay) == 1)
  {
    return;
  }

  #ifdef DEBUG
  fprintf(stderr, "nxagentShapeWindow: Window at [%p][%ld].\n",
              (void *) pWin, nxagentWindow(pWin));
  #endif

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
                  REGION_NUM_RECTS(wBoundingShape(pWin)));
      #endif

#ifdef NXAGENT_SHAPE2
      if (!nxagentWindowPriv(pWin)->boundingShape)
      {
         nxagentWindowPriv(pWin)->boundingShape = REGION_CREATE(pWin->drawable.pScreen, NULL, 1);
      }
#endif

      REGION_COPY(pWin->drawable.pScreen,
                      nxagentWindowPriv(pWin)->boundingShape, wBoundingShape(pWin));

      reg = XCreateRegion();
      pBox = REGION_RECTS(nxagentWindowPriv(pWin)->boundingShape);
      for (i = 0;
           i < REGION_NUM_RECTS(nxagentWindowPriv(pWin)->boundingShape);
           i++)
      {
        rect.x = pBox[i].x1;
        rect.y = pBox[i].y1;
        rect.width = pBox[i].x2 - pBox[i].x1;
        rect.height = pBox[i].y2 - pBox[i].y1;
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

      REGION_EMPTY(pWin->drawable.pScreen,
                       nxagentWindowPriv(pWin)->boundingShape);

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
                  REGION_NUM_RECTS(wClipShape(pWin)));
      #endif

#ifdef NXAGENT_SHAPE2
      if (!nxagentWindowPriv(pWin)->clipShape)
      {
        nxagentWindowPriv(pWin)->clipShape = REGION_CREATE(pWin->drawable.pScreen, NULL, 1);
      }
#endif

      REGION_COPY(pWin->drawable.pScreen,
                  nxagentWindowPriv(pWin)->clipShape, wClipShape(pWin));

      reg = XCreateRegion();
      pBox = REGION_RECTS(nxagentWindowPriv(pWin)->clipShape);
      for (i = 0;
           i < REGION_NUM_RECTS(nxagentWindowPriv(pWin)->clipShape);
           i++)
      {
        rect.x = pBox[i].x1;
        rect.y = pBox[i].y1;
        rect.width = pBox[i].x2 - pBox[i].x1;
        rect.height = pBox[i].y2 - pBox[i].y1;
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

      REGION_EMPTY(pWin->drawable.pScreen,
                       nxagentWindowPriv(pWin)->clipShape);

#ifndef NXAGENT_SHAPE
      XShapeCombineMask(nxagentDisplay, nxagentWindow(pWin),
                            ShapeClip, 0, 0, None, ShapeSet);
#endif
    }
  }
}
#endif /* SHAPE */

static int nxagentForceExposure(WindowPtr pWin, pointer ptr)
{
  RegionPtr exposedRgn;
  BoxRec Box;
  WindowPtr pRoot = WindowTable[pWin->drawable.pScreen->myNum];

  if (pWin -> drawable.class != InputOnly)
  {
    Box.x1 = pWin->drawable.x;
    Box.y1 = pWin->drawable.y;
    Box.x2 = Box.x1 + pWin->drawable.width;
    Box.y2 = Box.y1 + pWin->drawable.height;

    exposedRgn = REGION_CREATE(pWin->drawable.pScreen, &Box, 1);
    REGION_INTERSECT(pWin->drawable.pScreen, exposedRgn, exposedRgn, &pRoot->winSize);

    if (exposedRgn != NULL && REGION_NOTEMPTY(pWin -> drawable.pScreen, exposedRgn) != 0)
    {
      miWindowExposures(pWin, exposedRgn, NullRegion);
    }

    REGION_DESTROY(pWin->drawable.pScreen, exposedRgn);
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
  int i;

  if (nxagentOption(Fullscreen) == 1)
  {
    for (i = 0; i < screenInfo.numScreens; i++)
    {
      if (nxagentDefaultWindows[i])
      {
        XUnmapWindow(nxagentDisplay, nxagentDefaultWindows[i]);
      }
    }
  }

  NXFlushDisplay(nxagentDisplay, NXFlushLink);
}

void nxagentMapDefaultWindows()
{
  int i;

  for (i = 0; i < screenInfo.numScreens; i++)
  {
    WindowPtr pWin = WindowTable[i];

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

      if (nxagentOption(Shadow) == 0 || nxagentWMIsRunning == 0)
      {
        #ifdef TEST
        fprintf(stderr, "nxagentMapDefaultWindows: Mapping default window id [%ld].\n",
                    nxagentDefaultWindows[pScreen->myNum]);
        #endif

        XMapWindow(nxagentDisplay, nxagentDefaultWindows[pScreen->myNum]);

        if (nxagentOption(Fullscreen) == 1 && nxagentWMIsRunning == 1)
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

  if (nxagentIconWindow != 0)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentMapDefaultWindows: Mapping icon window id [%ld].\n",
                nxagentIconWindow);
    #endif

    XMapWindow(nxagentDisplay, nxagentIconWindow);

    if (nxagentIpaq != 0)
    {
      XIconifyWindow(nxagentDisplay, nxagentIconWindow,
                         DefaultScreen(nxagentDisplay));
    }
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
  Bool succeded = True;
  int i;
  WindowPtr pWin;

  #if defined(NXAGENT_RECONNECT_DEBUG) || defined(NXAGENT_RECONNECT_WINDOW_DEBUG)
  fprintf(stderr, "nxagentDisconnectAllWindows\n");
  #endif

  for (i = 0; i < screenInfo.numScreens; i++)
  {
    pWin = WindowTable[i];
    nxagentTraverseWindow( pWin, nxagentDisconnectWindow, &succeded);
    nxagentDefaultWindows[i] = None;
  }

  #ifdef NXAGENT_RECONNECT_WINDOW_DEBUG
  fprintf(stderr, "nxagentDisconnectAllWindows: all windows disconnected\n");
  #endif

  return succeded;
}

/*
 * FIXME: We are giving up reconnecting those pointer
 * that are not resource, and we are just disconnecting them.
 * perhaps we could do better and reconnect them.
 */

void nxagentDisconnectWindow(pointer p0, XID x1, pointer p2)
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

    if (*pBool == False)
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

  nxagentWindow(pWin) = None;

  if (nxagentDrawableStatus((DrawablePtr) pWin) == NotSynchronized)
  {
    nxagentDestroyCorruptedResource((DrawablePtr) pWin, RT_NX_CORR_WINDOW);
  }
}

Bool nxagentReconnectAllWindows(void *p0)
{
  int flexibility = *(int *) p0;

  flexibility = flexibility;

  #if defined(NXAGENT_RECONNECT_DEBUG) || defined(NXAGENT_RECONNECT_WINDOW_DEBUG)
  fprintf(stderr, "nxagentReconnectAllWindows\n");
  #endif

  if (WindowTable[0] -> backgroundState == BackgroundPixmap &&
          WindowTable[0] -> background.pixmap == NULL)
  {
    FatalError("nxagentReconnectAllWindows: correct the FIXME\n");
  }

  if (nxagentOption(Fullscreen))
  {
    WindowTable[0] -> origin.x = nxagentOption(RootX);
    WindowTable[0] -> origin.y = nxagentOption(RootY);
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
    int i;

    for (i = 0; i < screenInfo.numScreens; i++)
    {
      XRaiseWindow(nxagentDisplay, nxagentInputWindows[i]);
    }
  }

  nxagentFlushConfigureWindow();

  if (nxagentOption(Fullscreen))
  {
    WindowTable[0] -> origin.x = 0;
    WindowTable[0] -> origin.y = 0;
  }

  #ifdef NXAGENT_RECONNECT_WINDOW_DEBUG

  XSync(nxagentDisplay, 0);

  fprintf(stderr, "nxagentReconnectAllWindows: All windows reconfigured.\n");

  #endif

  if (nxagentInitClipboard(WindowTable[0]) == -1)
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
  int flexibility = *(int *) p0;

  flexibility = flexibility;

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
  fprintf(stderr, "nxagentLoopOverWindows: All cursors configured.\n");
  #endif

  nxagentReDisplayCurrentCursor();

  return True;
}

static void nxagentTraverseWindow(
  WindowPtr pWin,
  void (*pF)(pointer, XID, pointer),
  pointer p)
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

static Bool nxagentLoopOverWindows(void (*pF)(pointer, XID, pointer))
{
  int i;
  Bool windowSuccess = True;
  WindowPtr pWin;

  for (i = 0; i < screenInfo.numScreens; i++)
  {
    pWin = WindowTable[i];
    nxagentTraverseWindow(pWin, pF, &windowSuccess);
  }

  return windowSuccess;
}

static void nxagentReconnectWindow(pointer param0, XID param1, pointer data_buffer)
{
  WindowPtr pWin = (WindowPtr)param0;
  Bool *pBool = (Bool*)data_buffer;
  Visual *visual;
  unsigned long mask;
  XSetWindowAttributes attributes;
  ColormapPtr pCmap;

  if (!pWin || !*pBool)
    return;

  #ifdef NXAGENT_RECONNECT_WINDOW_DEBUG
  fprintf(stderr, "nxagentReconnectWindow: %p - ID %lx\n", pWin, nxagentWindow(pWin));
  #endif

  if (pWin->drawable.class == InputOnly) {
    mask = CWEventMask;
    visual = CopyFromParent;
  }
  else {
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
    if (nxagentSaveUnder == True)
    {
      mask |= CWSaveUnder;
      attributes.save_under = pWin->saveUnder;
    }

    if (pWin->parent) {
        if (pWin->optional && pWin->optional->visual != wVisual(pWin->parent)) {
        visual = nxagentVisualFromID(pWin->drawable.pScreen, wVisual(pWin));
        mask |= CWColormap;
        if (pWin->optional->colormap) {
          pCmap = (ColormapPtr)LookupIDByType(wColormap(pWin), RT_COLORMAP);
          attributes.colormap = nxagentColormap(pCmap);
        }
        else
          attributes.colormap = nxagentDefaultVisualColormap(visual);
      }
      else
        visual = CopyFromParent;
    }
    else { /* root windows have their own colormaps at creation time */
      visual = nxagentVisualFromID(pWin->drawable.pScreen, wVisual(pWin));
      pCmap = (ColormapPtr)LookupIDByType(wColormap(pWin), RT_COLORMAP);
      mask |= CWColormap;
      attributes.colormap = nxagentColormap(pCmap);
    }
  }

  if (mask & CWEventMask)
  {
    nxagentGetEventMask(pWin, (Mask*)&attributes.event_mask);
  }
  #ifdef WARNING
  else
  {
    attributes.event_mask = NoEventMask;
  }
  #endif

  #ifdef TEST
  fprintf(stderr, "nxagentReconnectWindow: Going to create new window.\n");
  #endif

  #ifdef TEST
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

  #ifdef TEST
  fprintf(stderr, "nxagentReconnectWindow: Created new window with id [%ld].\n",
              nxagentWindowPriv(pWin)->window);
  #endif

  /*
   * We have to set the WM_DELETE_WINDOW protocols
   * on every top level window, because we don't know
   * if a client handles this.
   */

  if (nxagentOption(Rootless) && (pWin != WindowTable[0]))
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
      XSizeHints *props, hints;
      unsigned char *data = NULL;

      #ifdef _XSERVER64

      unsigned char *data64 = NULL;
      unsigned int i;

      #endif

      hints.flags = 0;

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
        #ifdef TEST
        fprintf(stderr, "nxagentReconnectWindow: setting WMSizeHints on window %p [%lx - %lx].\n",
                    (void*)pWin, pWin -> drawable.id, nxagentWindow(pWin));
        #endif

        #ifdef _XSERVER64

        data64 = (unsigned char *) malloc(sizeof(XSizeHints) + 4);

        for (i = 0; i < 4; i++)
        {
          *(data64 + i) = *(data + i);
        }

        *(((int *) data64) + 1) = 0;

        for (i = 8; i < sizeof(XSizeHints) + 4; i++)
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

      if (data64 != NULL)
      {
        free(data64);
      }

      #endif
    }
  }

  if (nxagentDrawableStatus((DrawablePtr) pWin) == NotSynchronized)
  {
    nxagentAllocateCorruptedResource((DrawablePtr) pWin, RT_NX_CORR_WINDOW);
  }
}

static void nxagentReconfigureWindowCursor(pointer param0, XID param1, pointer data_buffer)
{
  WindowPtr pWin = (WindowPtr)param0;
  Bool *pBool = (Bool*)data_buffer;
  CursorPtr   pCursor;
  ScreenPtr   pScreen = pWin -> drawable.pScreen;

  if (!pWin || !*pBool || !(pCursor = wCursor(pWin)) ||
          !(nxagentCursorPriv(pCursor, pScreen)))
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

static void nxagentReconfigureWindow(pointer param0, XID param1, pointer data_buffer)
{
  WindowPtr pWin = (WindowPtr)param0;
  unsigned long mask = 0;

  #ifdef DEBUG
  fprintf(stderr, "nxagentReconfigureWindow: pWin %p - ID %lx\n", pWin, nxagentWindow(pWin));
  #endif

  if (pWin -> drawable.class == InputOnly)
    mask = CWWinGravity | CWEventMask | CWDontPropagate | CWOverrideRedirect | CWCursor;
  else
    mask = CWBackPixmap | CWBackPixel | CWBorderPixmap | CWBorderPixel |
         CWBitGravity | CWWinGravity | CWBackingStore | CWBackingPlanes |
         CWBackingPixel | CWOverrideRedirect | CWSaveUnder | CWEventMask |
         CWDontPropagate | CWColormap | CWCursor;

  nxagentChangeWindowAttributes(pWin, mask);

#ifdef SHAPE
  if (nxagentWindowPriv(pWin) -> boundingShape)
  {
    REGION_DESTROY(pWin -> drawable.pScreen,
                   nxagentWindowPriv(pWin) -> boundingShape);
    nxagentWindowPriv(pWin) -> boundingShape = NULL;
  }

  if (nxagentWindowPriv(pWin) -> clipShape)
  {
    REGION_DESTROY(pWin -> drawable.pScreen,
                   nxagentWindowPriv(pWin) -> clipShape);
    nxagentWindowPriv(pWin) -> clipShape = NULL;
  }
  nxagentShapeWindow(pWin);
#endif

  if (pWin != WindowTable[0])
  {
    if (pWin->realized)
    {
        nxagentRealizeWindow (pWin);
    }
/*
XXX: This would break Motif menus.
     If pWin is mapped but not realized, a followin UnmapWindow() wouldn't
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
        pWin == WindowTable[0] &&
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
     data = malloc(length);

     if (data == NULL)
     {
       FatalError("nxagentCheckWindowIntegrity: Failed to allocate a buffer of size %d.\n", length);
     }

     memset(data, 0, length);

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
       #ifdef TEST
       int i;
       char *p, *q;
       #endif

       integrity = False;

       #ifdef TEST
       for (i = 0, p = image->data, q = data; i < length; i++)
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
                   "now remote and frambuffer data are synchronized.\n", (void*) pWin);
       #endif
     }

     if (image)
     {
       XDestroyImage(image);
     }

     if (data)
     {
       free(data);
     }
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

#endif

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

void nxagentSetTopLevelEventMask(pWin)
     WindowPtr pWin;
{
  unsigned long mask = CWEventMask;
  XSetWindowAttributes attributes;

  if (nxagentOption(Rootless) && nxagentWindowTopLevel(pWin))
  {
    nxagentGetEventMask(pWin, (Mask*)&attributes.event_mask);

    XChangeWindowAttributes(nxagentDisplay, nxagentWindow(pWin), mask, &attributes);
  }
}

/*
 * This function must return 1 if we want the
 * exposures to be sent as the window's extents.
 * This is actually a harmless, but useful hack,
 * as it speeds up the window redraws considera-
 * bly, when using a very popular WM theme.
 */

int nxagentExtentsPredicate(int total)
{
  #ifdef TEST

  if (total == 6 || total == 11 || total == 10)
  {
    fprintf(stderr, "nxagentExtentsPredicate: WARNING! Returning [%d] with [%d] rectangles.\n",
                (total == 6 || total == 11 || total == 10), total);
  }

  #endif

  return (total == 6 || total == 11 || total == 10);
}

void nxagentFlushConfigureWindow(void)
{
  ConfiguredWindowStruct *index;
  XWindowChanges changes;
  int i;
  int j;

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
    ConfiguredWindowStruct *tmp;

    WindowPtr pWin = index -> pWin;
    unsigned int valuemask = index -> valuemask;

    if (pWin && valuemask)
    {
      nxagentConfigureWindow(pWin, valuemask);
    }

    tmp = index;

    if (index == nxagentConfiguredWindowList)
    {
      free(tmp);
      break;
    }

    index = index -> prev;
    free(tmp);
  }

  nxagentConfiguredWindowList = NULL;

  for (j = 0; j < nxagentExposeQueue.length; j++)
  {
    i = (nxagentExposeQueue.start + j) % EXPOSED_SIZE;

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

void nxagentPostValidateTree(WindowPtr pParent, WindowPtr pChild, VTKind kind)
{
/*
FIXME: Do we need this here?

  nxagentFlushConfigureWindow();
*/

  return;
}

void nxagentAddConfiguredWindow(WindowPtr pWin, unsigned int valuemask)
{
  unsigned int mask;

  mask = valuemask & (CWParent | CWX | CWY | CWWidth | CWHeight |
                   CWBorderWidth | CWStackingOrder | CW_Map | CW_Update | CW_Shape);

  valuemask &= ~(CWParent | CWX | CWY | CWWidth | CWHeight | CWBorderWidth | CWStackingOrder);

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

  if (mask & CWStackingOrder &&
          nxagentWindowPriv(pWin)->siblingAbove !=
              nxagentWindowSiblingAbove(pWin))
  {
    valuemask |= CWStackingOrder;
  }

  if (nxagentConfiguredWindowList == NULL)
  {
    nxagentConfiguredWindowList = malloc(sizeof(ConfiguredWindowStruct));
    nxagentConfiguredWindowList -> next = NULL;
    nxagentConfiguredWindowList -> prev = NULL;

    nxagentConfiguredWindowList -> pWin = pWin;
  }
  else
  {
    ConfiguredWindowStruct *tmp;

    tmp = malloc(sizeof(ConfiguredWindowStruct));

    tmp -> next = nxagentConfiguredWindowList;
    nxagentConfiguredWindowList -> prev = tmp;
    tmp -> prev = NULL;
    nxagentConfiguredWindowList = tmp;
    nxagentConfiguredWindowList -> pWin = pWin;
  }

  nxagentConfiguredWindowList -> valuemask = valuemask;

  return;
}

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
        free(nxagentConfiguredWindowList);
        nxagentConfiguredWindowList = NULL;

        return;
      }
      else if (index -> prev == NULL)
      {
        tmp = nxagentConfiguredWindowList;
        index = nxagentConfiguredWindowList = tmp -> next;
        free(tmp);
        nxagentConfiguredWindowList -> prev = NULL;

        continue;
      }
      else if (index -> next == NULL)
      {
        tmp = index;
        index = index -> prev;
        free(tmp);
        index -> next = NULL;

        return;
      }

      previous = index -> prev;
      tmp = index;
      index = index -> next;
      previous -> next = index;
      index -> prev = previous;
      free(tmp);

      continue;
    }

    index = index -> next;
  }

  return;
}

void nxagentAddStaticResizedWindow(WindowPtr pWin, unsigned long sequence, int offX, int offY)
{
  if (nxagentStaticResizedWindowList == NULL)
  {
    nxagentStaticResizedWindowList = malloc(sizeof(StaticResizedWindowStruct));
    nxagentStaticResizedWindowList -> next = NULL;
    nxagentStaticResizedWindowList -> prev = NULL;
  }
  else
  {
    StaticResizedWindowStruct *tmp;

    tmp = malloc(sizeof(StaticResizedWindowStruct));

    tmp -> next = nxagentStaticResizedWindowList;
    nxagentStaticResizedWindowList -> prev = tmp;
    tmp -> prev = NULL;
    nxagentStaticResizedWindowList = tmp;
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
        free(nxagentStaticResizedWindowList);
        nxagentStaticResizedWindowList = NULL;

        return;
      }
      else if (index -> prev == NULL)
      {
        tmp = nxagentStaticResizedWindowList;
        index = nxagentStaticResizedWindowList = tmp -> next;
        free(tmp);
        nxagentStaticResizedWindowList -> prev = NULL;

        continue;
      }
      else if (index -> next == NULL)
      {
        tmp = index;
        index = index -> prev;
        free(tmp);
        index -> next = NULL;

        return;
      }

      previous = index -> prev;
      tmp = index;
      index = index -> next;
      previous -> next = index;
      index -> prev = previous;
      free(tmp);

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

void nxagentEmptyBackingStoreRegion(pointer param0, XID param1, pointer data_buffer)
{
  WindowPtr pWin = (WindowPtr) param0;

  miBSWindowPtr pBackingStore = (miBSWindowPtr)pWin->backStorage;

  if (pBackingStore != NULL)
  {
    REGION_EMPTY(pWin -> pScreen, &pBackingStore->SavedRegion);

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
  int i;

  for (i = 0; i < BSPIXMAPLIMIT; i++)
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
  int i;
  int j;

  if (pixmapId == 0 || nxagentBSPixmapList[0] == NULL)
  {
    return 0;
  }

  for (i = 0; i < BSPIXMAPLIMIT; i++)
  {
    if ((nxagentBSPixmapList[i] != NULL) &&
            (nxagentBSPixmapList[i] -> storingPixmapId == pixmapId))
    {
      free(nxagentBSPixmapList[i]);
      nxagentBSPixmapList[i] = NULL;

      if (i < BSPIXMAPLIMIT - 1)
      {
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

int nxagentEmptyBSPixmapList()
{
  int i;

  for (i = 0; i < BSPIXMAPLIMIT; i++)
  {
    if (nxagentBSPixmapList[i] != NULL)
    {
      free(nxagentBSPixmapList[i]);
      nxagentBSPixmapList[i] = NULL;
    }
  }

  return 1;
}

StoringPixmapPtr nxagentFindItemBSPixmapList(unsigned long pixmapId)
{
  int i;

  for (i = 0; i < BSPIXMAPLIMIT; i++)
  {
    if ((nxagentBSPixmapList[i] != NULL) &&
            (nxagentBSPixmapList[i] -> storingPixmapId == pixmapId))
    {
      #ifdef TEST
      fprintf(stderr, "nxagentFindItemBSPixmapList: pixmapId [%lu].\n", pixmapId);
      fprintf(stderr, "nxagentFindItemBSPixmapList: nxagentBSPixmapList[%d] -> storingPixmapId [%lu].\n",
                  i, nxagentBSPixmapList[i] -> storingPixmapId);
      #endif

      return nxagentBSPixmapList[i];
    }
  }

  #ifdef TEST
  fprintf(stderr, "nxagentFindItemBSPixmapList: WARNING! Item not found.\n");
  #endif
  
  #ifdef TEST
  fprintf(stderr, "nxagentFindItemBSPixmapList: Pixmap with id [%lu] not found.\n",
              pixmapId);
  fprintf(stderr, "nxagentBSPixmapList[%d] = [%p].\n",
              i, (void *) nxagentBSPixmapList[i]);
  #endif

  return NULL;
}

