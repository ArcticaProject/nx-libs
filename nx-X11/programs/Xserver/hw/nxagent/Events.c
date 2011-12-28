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

#include "X.h"
#include "signal.h"
#include "unistd.h"
#define NEED_EVENTS
#include "Xproto.h"
#include "screenint.h"
#include "input.h"
#include "dix.h"
#include "misc.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "servermd.h"
#include "mi.h"
#include "selection.h"
#include "keysym.h"
#include "fb.h"
#include "mibstorest.h"
#include "osdep.h"

#include "Agent.h"
#include "Args.h"
#include "Atoms.h"
#include "Colormap.h"
#include "Display.h"
#include "Screen.h"
#include "Windows.h"
#include "Pixmaps.h"
#include "Keyboard.h"
#include "Keystroke.h"
#include "Events.h"
#include "Pointer.h"
#include "Rootless.h"
#include "Splash.h"
#include "Trap.h"
#include "Dialog.h"
#include "Client.h"
#include "Clipboard.h"
#include "Split.h"
#include "Drawable.h"
#include "Handlers.h"
#include "Utils.h"
#include "Error.h"

#include "NX.h"
#include "NXvars.h"
#include "NXproto.h"

#include "xfixesproto.h"
#define Window     XlibWindow
#define Atom   XlibAtom
#define Time XlibXID
#include <X11/extensions/Xfixes.h>
#undef Window
#undef Atom
#undef Time

#ifdef NXAGENT_FIXKEYS
#include "inputstr.h"
#include "input.h"
#endif

#define Time XlibXID
#include "XKBlib.h"
#undef Time

#define GC     XlibGC
#define Font   XlibFont
#define KeySym XlibKeySym
#define XID    XlibXID
#include "Xlibint.h"
#undef  GC
#undef  Font
#undef  KeySym
#undef  XID

#include <X11/cursorfont.h>

#include "Shadow.h"
#include "Xrandr.h"

#include "NXlib.h"

/*
 * Set here the required log level. Please note
 * that if you want to enable DEBUG here, then
 * you need to enable DEBUG even in Rootless.c
 */

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

/*
 * Log begin and end of the important handlers.
 */

#undef  BLOCKS

extern Bool nxagentOnce;

extern WindowPtr nxagentRootTileWindow;
extern int nxagentSplashCount;

extern int nxagentLastClipboardClient;

#ifdef NX_DEBUG_INPUT
int nxagentDebugInput = 0;
#endif

#ifdef DEBUG
extern Bool nxagentRootlessTreesMatch(void);
#endif

extern Selection *CurrentSelections;
extern int NumCurrentSelections;

typedef union _XFixesSelectionEvent {
        int                          type;
        XFixesSelectionNotifyEvent   xfixesselection;
        XEvent                       core;
} XFixesSelectionEvent;

Bool   xkbdRunning = False;
pid_t  pidkbd;

WindowPtr nxagentLastEnteredWindow = NULL;

PropertyRequestRec nxagentPropertyRequests[NXNumberOfResources];

void nxagentHandleCollectPropertyEvent(XEvent*);

/*
 * Finalize the asynchronous handling
 * of the X_GrabPointer requests.
 */

void nxagentHandleCollectGrabPointerEvent(int resource);

Bool nxagentCollectGrabPointerPredicate(Display *display, XEvent *X, XPointer ptr);

/*
 * Used in Handlers.c to synchronize
 * the agent with the remote X server.
 */

void nxagentHandleCollectInputFocusEvent(int resource);

/*
 * Viewport navigation.
 */

static int viewportInc = 1;
static enum HandleEventResult viewportLastKeyPressResult;
static int viewportLastX;
static int viewportLastY;
static Cursor viewportCursor;

/*
 * Keyboard and pointer are handled as they were real devices by
 * Xnest and we inherit this behaviour. The following mask will
 * contain the event mask selected for the root window of the
 * agent. All the keyboard and pointer events will be translated
 * by the agent and sent to the internal clients according to
 * events selected by the inferior windows.
 */

static Mask defaultEventMask;

static int lastEventSerial = 0;

#define MAX_INC 200
#define INC_STEP 5
#define nextinc(x)  ((x) < MAX_INC ? (x) += INC_STEP : (x))

/*
 * Used to mask the appropriate bits in
 * the state reported by XkbStateNotify
 * and XkbGetIndicatorState.
 */

#define CAPSFLAG_IN_REPLY     1
#define CAPSFLAG_IN_EVENT     2
#define NUMFLAG_IN_EVENT      16
#define NUMFLAG_IN_REPLY      2

CARD32 nxagentLastEventTime     = 0;
CARD32 nxagentLastKeyPressTime  = 0;
Time   nxagentLastServerTime    = 0;

/*
 * Used for storing windows that need to
 * receive expose events from the agent.
 */

#define nxagentExposeQueueHead nxagentExposeQueue.exposures[nxagentExposeQueue.start]

ExposeQueue nxagentExposeQueue;

RegionPtr nxagentRemoteExposeRegion = NULL;

static void nxagentForwardRemoteExpose(void);

static int nxagentClipAndSendExpose(WindowPtr pWin, pointer ptr);

/*
 * This is from NXproperty.c.
 */

int GetWindowProperty(WindowPtr pWin, Atom property, long longOffset,
                          long longLength, Bool delete, Atom type,
                              Atom *actualType, int *format, unsigned
                                  long *nItems, unsigned long *bytesAfter,
                                      unsigned char **propData);

/*
 * Associate a resource to a drawable and
 * store the region affected by the split
 * operation.
 */

SplitResourceRec nxagentSplitResources[NXNumberOfResources];

/*
 * Associate a resource to an unpack
 * operation.
 */

UnpackResourceRec nxagentUnpackResources[NXNumberOfResources];

/*
 * We have to check these before launching
 * the terminate dialog in rootless mode.
 */

Bool nxagentLastWindowDestroyed = False;
Time nxagentLastWindowDestroyedTime = 0;

/*
 * Set this flag when an user input event
 * is received.
 */

int nxagentInputEvent = 0;

int nxagentKeyDown = 0;

void nxagentSwitchResizeMode(ScreenPtr pScreen);

int nxagentCheckWindowConfiguration(XConfigureEvent* X);

#define nxagentMonitoredDuplicate(keysym) \
    ((keysym) == XK_Left || (keysym) == XK_Up || \
        (keysym) == XK_Right || (keysym) == XK_Down || \
            (keysym) == XK_Page_Up || (keysym) == XK_Page_Down || \
                (keysym) == XK_Delete || (keysym) == XK_BackSpace)

void nxagentRemoveDuplicatedKeys(XEvent *X);

void ProcessInputEvents()
{
  #ifdef NX_DEBUG_INPUT
  if (nxagentDebugInput == 1)
  {
    fprintf(stderr, "ProcessInputEvents: Processing input.\n");
  }
  #endif

  mieqProcessInputEvents();
}

#ifdef DEBUG_TREE

/*
 * Print ID and name of window.
 */

void nxagentRemoteWindowID(Window window, Bool newline)
{
#ifdef NO_I18N
    char *winName;
#else
    XTextProperty tp;
#endif

  fprintf(stderr, "0x%lx", window);

  if (!window)
  {
    fprintf(stderr, " (none) ");
  }
  else
  {
    if (window == DefaultRootWindow(nxagentDisplay))
    {
      fprintf(stderr, " (the root window) ");
    }

#ifdef NO_I18N

    if (!XFetchName(nxagentDisplay, window, &winName))
    {
      fprintf(stderr, " (has no name) ");
    }
    else if (winName)
    {
      fprintf(stderr, " \"%s\" ", winName);
      XFree(winName);
    }

#else

    if (XGetWMName(nxagentDisplay, window, &tp) != 0)
    {
      fprintf(stderr, " (has no name) ");
    }
    else if (tp.nitems > 0)
    {
      int count = 0;
      int i, ret;
      char **list = NULL;

      fprintf(stderr, " \"");

      ret = XmbTextPropertyToTextList(nxagentDisplay, &tp, &list, &count);

      if ((ret == Success || ret > 0) && list != NULL)
      {
        for (i = 0; i < count; i++)
        {
          fprintf(stderr, "%s", list[i]);
        }

        XFreeStringList(list);
      }
      else
      {
        fprintf(stderr, "%s", tp.value);
      }

      fprintf(stderr, "\" ");
    }

#endif

    else
    {
      fprintf(stderr, " (has no name) ");
    }
  }

  if (newline == TRUE)
  {
    fprintf(stderr, "\n");
  }

  return;
}

/*
 * Print info about remote window.
 */

void nxagentRemoteWindowInfo(Window win, int indent, Bool newLine)
{
  XWindowAttributes attributes;
  int i;

  if (XGetWindowAttributes(nxagentDisplay, win, &attributes) == 0)
  {
    return;
  }

  for (i = 0; i < indent; i++)
  {
    fprintf(stderr, " ");
  }

  fprintf(stderr, "x=%d y=%d width=%d height=%d class=%s map_state=%s "
             "override_redirect=%s\n", attributes.x, attributes.y,
                 attributes.width, attributes.height,
                     (attributes.class == 0) ? "CopyFromParent" :
                     ((attributes.class == 1) ? "InputOutput" : "InputOnly"),
                     (attributes.map_state == 0) ?
                         "IsUnmapped" : (attributes.map_state == 1 ?
                             "IsUnviewable" : "IsViewable"),
                                 (attributes.override_redirect == 0) ?
                                     "No" : "Yes" );

  if (newLine == TRUE)
  {
    fprintf(stderr, "\n");
  }
}

/*
 * Walk remote windows tree.
 */

void nxagentRemoteWindowsTree(Window window, int level)
{
  int i, j;
  unsigned long rootWin, parentWin;
  unsigned int numChildren;
  unsigned long *childList;

  if (!XQueryTree(nxagentDisplay, window, &rootWin, &parentWin, &childList,
                      &numChildren))
  {
    fprintf(stderr, "nxagentRemoteWindowsTree - XQueryTree failed.\n");

    return;
  }

  if (level == 0)
  {
    fprintf(stderr, "\n");

    fprintf(stderr, "  Root Window ID: ");
    nxagentRemoteWindowID(rootWin, TRUE);

    fprintf(stderr, "  Parent window ID: ");
    nxagentRemoteWindowID(parentWin, TRUE);
  }

  if (level == 0 || numChildren > 0)
  {
    fprintf(stderr, "     ");

    for (j = 0; j < level; j++)
    {
      fprintf(stderr, "    ");
    }

    fprintf(stderr, "%d child%s%s\n", numChildren, (numChildren == 1) ? "" :
               "ren", (numChildren == 1) ? ":" : ".");
  }

  for (i = (int) numChildren - 1; i >= 0; i--)
  {
    fprintf(stderr, "      ");

    for (j = 0; j < level; j++)
    {
      fprintf(stderr, "     ");
    }

    nxagentRemoteWindowID(childList[i], TRUE);

    nxagentRemoteWindowInfo(childList[i], (level * 5) + 6, TRUE);

    nxagentRemoteWindowsTree(childList[i], level + 1);
  }

  if (childList)
  {
    XFree((char *) childList);
  }
}

/*
 * Print info about internal window.
 */

void nxagentInternalWindowInfo(WindowPtr pWin, int indent, Bool newLine)
{
  int i;
  int result;
  unsigned long ulReturnItems;
  unsigned long ulReturnBytesLeft;
  Atom          atomReturnType;
  int           iReturnFormat;
  unsigned char *pszReturnData = NULL;

  fprintf(stderr, "Window ID=[0x%lx] Remote ID=[0x%lx] ", pWin -> drawable.id,
             nxagentWindow(pWin));

  result = GetWindowProperty(pWin, MakeAtom("WM_NAME", 7, False) , 0,
                                sizeof(CARD32), False, AnyPropertyType,
                                    &atomReturnType, &iReturnFormat,
                                        &ulReturnItems, &ulReturnBytesLeft,
                                            &pszReturnData);

  fprintf(stderr, "Name: ");

  if (result == Success && pszReturnData != NULL)
  {
    pszReturnData[ulReturnItems] = '\0';

    fprintf(stderr, "\"%s\"\n", (char *) pszReturnData);
  }
  else
  {
    fprintf(stderr, "%s\n", "( has no name )");
  }

  for (i = 0; i < indent; i++)
  {
    fprintf(stderr, " ");
  }

  fprintf(stderr, "x=%d y=%d width=%d height=%d class=%s map_state=%s "
             "override_redirect=%s", pWin -> drawable.x, pWin -> drawable.y,
                 pWin -> drawable.width, pWin -> drawable.height,
                     (pWin -> drawable.class == 0) ? "CopyFromParent" :
                     ((pWin -> drawable.class == 1) ? "InputOutput" :
                      "InputOnly"),
                      (pWin -> mapped == 0) ?
                             "IsUnmapped" : (pWin -> realized == 0 ?
                                 "IsUnviewable" : "IsViewable"),
                                     (pWin -> overrideRedirect == 0) ?
                                         "No" : "Yes");

  if (newLine == TRUE)
  {
    fprintf(stderr, "\n");
  }
}

/*
 * Walk internal windows tree.
 */

void nxagentInternalWindowsTree(WindowPtr pWin, int indent)
{
  WindowPtr pChild;
  int i;

  while (pWin)
  {
    pChild = pWin -> firstChild;

    for (i = 0; i < indent; i++)
    {
      fprintf(stderr, " ");
    }

    nxagentInternalWindowInfo(pWin, indent, TRUE);

    fprintf(stderr, "\n");

    nxagentInternalWindowsTree(pChild, indent + 4);

    pWin = pWin -> nextSib;
  }
}

#endif /* DEBUG_TREE */

void nxagentSwitchResizeMode(ScreenPtr pScreen)
{
  XSizeHints sizeHints;

  int desktopResize = nxagentOption(DesktopResize);

  nxagentChangeOption(DesktopResize, !desktopResize);

  sizeHints.flags = PMaxSize;

  if (nxagentOption(DesktopResize) == 0)
  {
    fprintf(stderr,"Info: Disabled desktop resize mode in agent.\n");

    nxagentLaunchDialog(DIALOG_DISABLE_DESKTOP_RESIZE_MODE);

    if (nxagentOption(Fullscreen) == 0)
    {
      sizeHints.max_width = nxagentOption(RootWidth);
      sizeHints.max_height = nxagentOption(RootHeight);

      XSetWMNormalHints(nxagentDisplay, nxagentDefaultWindows[pScreen->myNum],
                            &sizeHints);
    }
  }
  else
  {
    fprintf(stderr,"Info: Enabled desktop resize mode in agent.\n");

    nxagentLaunchDialog(DIALOG_ENABLE_DESKTOP_RESIZE_MODE);

    nxagentChangeScreenConfig(0, nxagentOption(Width), nxagentOption(Height),
                                  0, 0);

    if (nxagentOption(ClientOs) == ClientOsWinnt)
    {
      NXSetExposeParameters(nxagentDisplay, 0, 0, 0);
    }

    sizeHints.max_width = WidthOfScreen(DefaultScreenOfDisplay(nxagentDisplay));
    sizeHints.max_height = HeightOfScreen(DefaultScreenOfDisplay(nxagentDisplay));

    XSetWMNormalHints(nxagentDisplay, nxagentDefaultWindows[pScreen->myNum],
                          &sizeHints);
  }
}

void nxagentShadowSwitchResizeMode(ScreenPtr pScreen)
{
  XSizeHints sizeHints;

  int desktopResize = nxagentOption(DesktopResize);

  nxagentChangeOption(DesktopResize, !desktopResize);

  sizeHints.flags = PMaxSize;

  if (nxagentOption(DesktopResize) == 0)
  {
    nxagentShadowSetRatio(1.0, 1.0);

    nxagentShadowCreateMainWindow(screenInfo.screens[DefaultScreen(nxagentDisplay)], WindowTable[0],
                                      WindowTable[0] -> drawable.width, WindowTable[0] -> drawable.height);

    sizeHints.max_width = nxagentOption(RootWidth);
    sizeHints.max_height = nxagentOption(RootHeight);

    fprintf(stderr,"Info: Disabled resize mode in shadow agent.\n");
  }
  else
  {
    nxagentShadowSetRatio(nxagentOption(Width) * 1.0 /
                              WindowTable[0] -> drawable.width, 
                                  nxagentOption(Height) * 1.0 /
                                      WindowTable[0] -> drawable.height);

    nxagentShadowCreateMainWindow(screenInfo.screens[DefaultScreen(nxagentDisplay)],
                                      WindowTable[0], WindowTable[0] -> drawable.width,
                                          WindowTable[0] -> drawable.height);

    sizeHints.max_width = WidthOfScreen(DefaultScreenOfDisplay(nxagentDisplay));
    sizeHints.max_height = HeightOfScreen(DefaultScreenOfDisplay(nxagentDisplay));

    fprintf(stderr,"Info: Enabled resize mode in shadow agent.\n");
  }

  XSetWMNormalHints(nxagentDisplay, nxagentDefaultWindows[pScreen->myNum],
                       &sizeHints);
}

static void nxagentSwitchDeferMode(void)
{
  if (nxagentOption(DeferLevel) == 0)
  {
    nxagentChangeOption(DeferLevel, UNDEFINED);

    nxagentSetDeferLevel();
  }
  else
  {
    nxagentChangeOption(DeferLevel, 0);
  }

  if (nxagentOption(DeferLevel) != 0)
  {
    nxagentLaunchDialog(DIALOG_ENABLE_DEFER_MODE);
  }
  else
  {
    nxagentLaunchDialog(DIALOG_DISABLE_DEFER_MODE);

    nxagentForceSynchronization = 1;
  }
}

static Bool nxagentExposurePredicate(Display *display, XEvent *event, XPointer window)
{
  /*
   *  Handle both Expose and ProcessedExpose events.
   *  The latters are those not filtered by function
   *  nxagentWindowExposures().
   */

  if (window)
  {
    return ((event -> type == Expose || event -> type == ProcessedExpose) &&
                event -> xexpose.window == *((Window *) window));
  }
  else
  {
    return (event -> type == Expose || event -> type == ProcessedExpose);
  }
}

static int nxagentAnyEventPredicate(Display *display, XEvent *event, XPointer parameter)
{
  return 1;
}

int nxagentInputEventPredicate(Display *display, XEvent *event, XPointer parameter)
{
  switch (event -> type)
  {
    case KeyPress:
    case KeyRelease:
    case ButtonPress:
    case ButtonRelease:
    {
      return 1;
    }
    default:
    {
      return 0;
    }
  }
}

void nxagentInitDefaultEventMask()
{
  Mask mask = NoEventMask;

  mask |= (StructureNotifyMask | VisibilityChangeMask);

  mask |= ExposureMask;

  mask |= NXAGENT_KEYBOARD_EVENT_MASK;
  mask |= NXAGENT_POINTER_EVENT_MASK;

  defaultEventMask = mask;
}

void nxagentGetDefaultEventMask(Mask *mask_return)
{
  *mask_return = defaultEventMask;
}

void nxagentSetDefaultEventMask(Mask mask)
{
  defaultEventMask = mask;
}

void nxagentGetEventMask(WindowPtr pWin, Mask *mask_return)
{
  Mask mask = NoEventMask;

  if (nxagentOption(Rootless))
  {
    /*
     * mask = pWin -> eventMask &
     *           ~(NXAGENT_KEYBOARD_EVENT_MASK | NXAGENT_POINTER_EVENT_MASK);
     */

    if (pWin -> drawable.class == InputOutput)
    {
      if (nxagentWindowTopLevel(pWin))
      {
        mask = defaultEventMask;
      }
      else
      {
        mask = ExposureMask | VisibilityChangeMask | PointerMotionMask;
      }
    }

    mask |= PropertyChangeMask;
  }
  else if (pWin -> drawable.class != InputOnly)
  {
    mask = ExposureMask | VisibilityChangeMask;
  }

  *mask_return = mask;
}

static int nxagentChangeMapPrivate(WindowPtr pWin, pointer ptr)
{
  if (pWin && nxagentWindowPriv(pWin))
  {
    nxagentWindowPriv(pWin) -> isMapped = *((Bool *) ptr);
  }

  return WT_WALKCHILDREN;
}

static int nxagentChangeVisibilityPrivate(WindowPtr pWin, pointer ptr)
{
  if (pWin && nxagentWindowPriv(pWin))
  {
    nxagentWindowPriv(pWin) -> visibilityState = *((int *) ptr);
  }

  return WT_WALKCHILDREN;
}

void nxagentDispatchEvents(PredicateFuncPtr predicate)
{
  XEvent X;
  xEvent x;
  ScreenPtr pScreen = NULL;

  Bool minimize = False;
  Bool startKbd = False;
  Bool closeSession = False;
  Bool switchFullscreen = False;
  Bool switchAllScreens = False;

  /*
   * Last entered top level window.
   */

  static WindowPtr nxagentLastEnteredTopLevelWindow = NULL;

  #ifdef BLOCKS
  fprintf(stderr, "[Begin read]\n");
  #endif

  #ifdef TEST
  fprintf(stderr, "nxagentDispatchEvents: Going to handle new events with "
              "predicate [%p].\n", predicate);
  #endif

  if (nxagentRemoteExposeRegion == NULL)
  {
    nxagentInitRemoteExposeRegion();
  }

  /*
   * We must read here, even if apparently there is
   * nothing to read. The ioctl() based readable
   * function, in fact, is often unable to detect a
   * failure of the socket, in particular if the
   * agent was connected to the proxy and the proxy
   * is gone. Thus we must trust the wakeup handler
   * that called us after the select().
   */

  #ifdef TEST

  if (nxagentPendingEvents(nxagentDisplay) == 0)
  {
    fprintf(stderr, "nxagentDispatchEvents: PANIC! No event needs to be dispatched.\n");
  }

  #endif

  /*
   * We want to process all the events already in
   * the queue, plus any additional event that may
   * be read from the network. If no event can be
   * read, we want to continue handling our clients
   * without flushing the output buffer.
   */

  while (nxagentCheckEvents(nxagentDisplay, &X, predicate != NULL ? predicate :
                                nxagentAnyEventPredicate, NULL) == 1)
  {
    #ifdef DEBUG
    fprintf(stderr, "nxagentDispatchEvents: Going to handle new event type [%d].\n",
                X.type);
    #endif

    /*
     * Handle the incoming event.
     */

    switch (X.type)
    {
      #ifdef NXAGENT_CLIPBOARD

      case SelectionClear:
      {
        #ifdef TEST
        fprintf(stderr, "nxagentDispatchEvents: Going to handle new SelectionClear event.\n");
        #endif

        nxagentClearSelection(&X);

        break;
      }
      case SelectionRequest:
      {
        #ifdef TEST
        fprintf(stderr, "nxagentDispatchEvents: Going to handle new SelectionRequest event.\n");
        #endif

        nxagentRequestSelection(&X);

        break;
      }
      case SelectionNotify:
      {
        #ifdef TEST
        fprintf(stderr, "nxagentDispatchEvents: Going to handle new SelectionNotify event.\n");
        #endif

        nxagentNotifySelection(&X);

        break;
      }

      #endif /* NXAGENT_CLIPBOARD */

      case PropertyNotify:
      {
        #ifdef TEST
        fprintf(stderr, "nxagentDispatchEvents: PropertyNotify on "
                    "prop %d[%s] window %lx state %d\n",
                        (int)X.xproperty.atom, validateString(XGetAtomName(nxagentDisplay, X.xproperty.atom)),
                            X.xproperty.window, X.xproperty.state);
        #endif

        nxagentHandlePropertyNotify(&X);

        break;
      }
      case KeyPress:
      {
        enum HandleEventResult result;

        KeySym keysym;

        #ifdef TEST
        fprintf(stderr, "nxagentDispatchEvents: Going to handle new KeyPress event.\n");
        #endif

        nxagentInputEvent = 1;

        nxagentKeyDown++;

        nxagentHandleKeyPress(&X, &result);

        if (viewportLastKeyPressResult != result)
        {
          viewportInc = 1;

          viewportLastKeyPressResult = result;
        }

        if (result != doNothing && result != doStartKbd)
        {
          pScreen = nxagentScreen(X.xkey.window);
        }

        switch (result)
        {
          case doNothing:
          {
            break;
          }

          #ifdef DEBUG_TREE

          case doDebugTree:
          {
            fprintf(stderr, "\n ========== nxagentRemoteWindowsTree ==========\n");
            nxagentRemoteWindowsTree(nxagentWindow(WindowTable[0]), 0);

            fprintf(stderr, "\n========== nxagentInternalWindowsTree ==========\n");
            nxagentInternalWindowsTree(WindowTable[0], 0);

            break;
          }

          #endif /* DEBUG_TREE */

          case doCloseSession:
          {
            closeSession = TRUE;

            break;
          }
          case doMinimize:
          {
            minimize = TRUE;

            break;
          }
          case doStartKbd:
          {
            startKbd = TRUE;

            break;
          }
          case doSwitchFullscreen:
          {
            switchFullscreen = TRUE;

            break;
          }
          case doSwitchAllScreens:
          {
            switchAllScreens = TRUE;

            break;
          }
          case doViewportMoveUp:
          {
            nxagentMoveViewport(pScreen, 0, -nxagentOption(Height));

            break;
          }
          case doViewportMoveDown:
          {
            nxagentMoveViewport(pScreen, 0, nxagentOption(Height));

            break;
          }
          case doViewportMoveLeft:
          {
            nxagentMoveViewport(pScreen, -nxagentOption(Width), 0);

            break;
          }
          case doViewportMoveRight:
          {
            nxagentMoveViewport(pScreen, nxagentOption(Width), 0);

            break;
          }
          case doViewportUp:
          {
            nxagentMoveViewport(pScreen, 0, -nextinc(viewportInc));

            break;
          }
          case doViewportDown:
          {
            nxagentMoveViewport(pScreen, 0, +nextinc(viewportInc));

            break;
          }
          case doViewportLeft:
          {
            nxagentMoveViewport(pScreen, -nextinc(viewportInc), 0);

            break;
          }
          case doViewportRight:
          {
            nxagentMoveViewport(pScreen, +nextinc(viewportInc), 0);

            break;
          }
          case doSwitchResizeMode:
          {
            if (nxagentOption(Shadow) == 0)
            {
              if (nxagentNoDialogIsRunning)
              {
                nxagentSwitchResizeMode(pScreen);
              }
            }
            else
            {
              nxagentShadowSwitchResizeMode(pScreen);
            }

            break;
          }
          case doSwitchDeferMode:
          {
            if (nxagentNoDialogIsRunning)
            {
              nxagentSwitchDeferMode();
            }

            break;
          }
          default:
          {
            FatalError("nxagentDispatchEvent: handleKeyPress returned unknown value\n");

            break;
          }
        }

        /*
         * Elide multiple KeyPress/KeyRelease events of
         * the same key and generate a single pair. This
         * is intended to reduce the impact of the laten-
         * cy on the key auto-repeat, handled by the re-
         * mote X server. We may optionally do that only
         * if the timestamps in the events show an exces-
         * sive delay.
         */

        keysym = XKeycodeToKeysym(nxagentDisplay, X.xkey.keycode, 0);

        if (nxagentMonitoredDuplicate(keysym) == 1)
        {
          nxagentRemoveDuplicatedKeys(&X);
        }

        if (nxagentOption(ViewOnly) == 0 && nxagentOption(Shadow) == 1 && result == doNothing)
        {
          X.xkey.keycode = nxagentConvertKeycode(X.xkey.keycode);

          NXShadowEvent(nxagentDisplay, X);
        }

        break;
      }
      case KeyRelease:
      {
        enum HandleEventResult result;
        int sendKey = 0;

/*
FIXME: If we don't flush the queue here, it could happen
       that the inputInfo structure will not be up to date
       when we perform the following check on down keys.
*/
        ProcessInputEvents();

/*
FIXME: Don't enqueue the KeyRelease event if the key was
       not already pressed. This workaround avoids a fake
       KeyPress is enqueued by the XKEYBOARD extension.
       Another solution would be to let the events are
       enqueued and to remove the KeyPress afterwards.
*/
        if (BitIsOn(inputInfo.keyboard -> key -> down,
                       nxagentConvertKeycode(X.xkey.keycode)))
        {
          sendKey = 1;
        }

        #ifdef TEST
        fprintf(stderr, "nxagentDispatchEvents: Going to handle new KeyRelease event.\n");
        #endif

        nxagentInputEvent = 1;

        nxagentKeyDown--;

        if (nxagentKeyDown <= 0)
        {
          nxagentKeyDown = 0;
        }

        if (nxagentXkbState.Initialized == 0)
        {
          if (X.xkey.keycode == nxagentCapsLockKeycode)
          {
            nxagentXkbCapsTrap = 1;
          }
          else if (X.xkey.keycode == nxagentNumLockKeycode)
          {
            nxagentXkbNumTrap = 1;
          }

          nxagentInitKeyboardState();

          nxagentXkbCapsTrap = 0;
          nxagentXkbNumTrap = 0;
        }

        x.u.u.type = KeyRelease;
        x.u.u.detail = nxagentConvertKeycode(X.xkey.keycode);
        x.u.keyButtonPointer.time = nxagentLastKeyPressTime +
            (X.xkey.time - nxagentLastServerTime);

        nxagentLastServerTime = X.xkey.time;

        nxagentLastEventTime = GetTimeInMillis();

        if (x.u.keyButtonPointer.time > nxagentLastEventTime)
        {
          x.u.keyButtonPointer.time = nxagentLastEventTime;
        }

        if (!(nxagentCheckSpecialKeystroke(&X.xkey, &result)) && sendKey == 1)
        {
          mieqEnqueue(&x);

          CriticalOutputPending = 1;

          if (nxagentOption(ViewOnly) == 0 && nxagentOption(Shadow))
          {
            X.xkey.keycode = nxagentConvertKeycode(X.xkey.keycode);

            NXShadowEvent(nxagentDisplay, X);
          }
        }

        break;
      }
      case ButtonPress:
      {
        #ifdef NX_DEBUG_INPUT
        if (nxagentDebugInput == 1)
        {
          fprintf(stderr, "nxagentDispatchEvents: Going to handle new ButtonPress event.\n");
        }
        #endif

        nxagentInputEvent = 1;

        if (nxagentOption(Fullscreen))
        {
          if (nxagentMagicPixelZone(X.xbutton.x, X.xbutton.y))
          {
            pScreen = nxagentScreen(X.xbutton.window);

            minimize = True;

            break;
          }
        }

        if (nxagentIpaq && nxagentClients <= 0)
        {
          closeSession = TRUE;
        }

        if (nxagentOption(DesktopResize) == False &&
                (X.xbutton.state & (ControlMask | Mod1Mask)) == (ControlMask | Mod1Mask))
        {
          /*
           * Start viewport navigation mode.
           */

          int resource = nxagentWaitForResource(NXGetCollectGrabPointerResource,
                                                    nxagentCollectGrabPointerPredicate);

          ScreenPtr pScreen = nxagentScreen(X.xbutton.window);
          viewportCursor = XCreateFontCursor(nxagentDisplay, XC_fleur);

          NXCollectGrabPointer(nxagentDisplay, resource,
                                   nxagentDefaultWindows[pScreen -> myNum], True,
                                       NXAGENT_POINTER_EVENT_MASK, GrabModeAsync,
                                           GrabModeAsync, None, viewportCursor,
                                               CurrentTime);
          viewportLastX = X.xbutton.x;
          viewportLastY = X.xbutton.y;

          break;
        }

        if (!(nxagentOption(Fullscreen) &&
                X.xbutton.window == nxagentFullscreenWindow &&
                    X.xbutton.subwindow == None))
        {
          x.u.u.type = ButtonPress;
          x.u.u.detail = inputInfo.pointer -> button -> map[nxagentReversePointerMap[X.xbutton.button - 1]];
          x.u.keyButtonPointer.time = nxagentLastEventTime = GetTimeInMillis();

          if (nxagentOption(Rootless))
          {
            x.u.keyButtonPointer.rootX = X.xmotion.x_root;
            x.u.keyButtonPointer.rootY = X.xmotion.y_root;
          }
          else
          {
            x.u.keyButtonPointer.rootX = X.xmotion.x - nxagentOption(RootX);
            x.u.keyButtonPointer.rootY = X.xmotion.y - nxagentOption(RootY);
          }

          #ifdef NX_DEBUG_INPUT
          if (nxagentDebugInput == 1)
          {
            fprintf(stderr, "nxagentDispatchEvents: Adding ButtonPress event.\n");
          }
          #endif

          mieqEnqueue(&x);

          CriticalOutputPending = 1;
        }

        if (nxagentOption(ViewOnly) == 0 && nxagentOption(Shadow))
        {
          X.xbutton.x -= nxagentOption(RootX);
          X.xbutton.y -= nxagentOption(RootY);

          if (nxagentOption(YRatio) != DONT_SCALE)
          {
            X.xbutton.x = (X.xbutton.x << PRECISION) / nxagentOption(YRatio);
          }

          if (nxagentOption(XRatio) != DONT_SCALE)
          {
            X.xbutton.y = (X.xbutton.y << PRECISION) / nxagentOption(YRatio);
          }

          NXShadowEvent(nxagentDisplay, X);
        }

        break;
      }
      case ButtonRelease:
      {
        #ifdef NX_DEBUG_INPUT
        if (nxagentDebugInput == 1)
        {
          fprintf(stderr, "nxagentDispatchEvents: Going to handle new ButtonRelease event.\n");
        }
        #endif

        nxagentInputEvent = 1;

        if (viewportCursor)
        {
          /*
           * Leave viewport navigation mode.
           */

          XUngrabPointer(nxagentDisplay, CurrentTime);

          XFreeCursor(nxagentDisplay, viewportCursor);

          viewportCursor = None;
        }

        if (minimize != True)
        {
          x.u.u.type = ButtonRelease;
          x.u.u.detail = inputInfo.pointer -> button -> map[nxagentReversePointerMap[X.xbutton.button - 1]];
          x.u.keyButtonPointer.time = nxagentLastEventTime = GetTimeInMillis();

          if (nxagentOption(Rootless))
          {
            x.u.keyButtonPointer.rootX = X.xmotion.x_root;
            x.u.keyButtonPointer.rootY = X.xmotion.y_root;
          }
          else
          {
            x.u.keyButtonPointer.rootX = X.xmotion.x - nxagentOption(RootX);
            x.u.keyButtonPointer.rootY = X.xmotion.y - nxagentOption(RootY);
          }

          #ifdef NX_DEBUG_INPUT
          if (nxagentDebugInput == 1)
          {
            fprintf(stderr, "nxagentDispatchEvents: Adding ButtonRelease event.\n");
          }
          #endif

          mieqEnqueue(&x);

          CriticalOutputPending = 1;
        }

        if (nxagentOption(ViewOnly) == 0 && nxagentOption(Shadow))
        {
          X.xbutton.x -= nxagentOption(RootX);
          X.xbutton.y -= nxagentOption(RootY);

          if (nxagentOption(XRatio) != DONT_SCALE)
          {
            X.xbutton.x = (X.xbutton.x << PRECISION) / nxagentOption(XRatio);
          }

          if (nxagentOption(YRatio) != DONT_SCALE)
          {
            X.xbutton.y = (X.xbutton.y << PRECISION) / nxagentOption(YRatio);
          }

          NXShadowEvent(nxagentDisplay, X);
        }

        break;
      }
      case MotionNotify:
      {
        ScreenPtr pScreen = nxagentScreen(X.xmotion.window);

        #ifdef TEST
        fprintf(stderr, "nxagentDispatchEvents: Going to handle new MotionNotify event.\n");
        #endif

        #ifdef NX_DEBUG_INPUT
        if (nxagentDebugInput == 1)
        {
          fprintf(stderr, "nxagentDispatchEvents: Handling motion notify window [%ld] root [%ld] child [%ld].\n",
                    X.xmotion.window, X.xmotion.root, X.xmotion.subwindow);

          fprintf(stderr, "nxagentDispatchEvents: Pointer at [%d][%d] relative root [%d][%d].\n",
                    X.xmotion.x, X.xmotion.y, X.xmotion.x_root, X.xmotion.y_root);
        }
        #endif

        x.u.u.type = MotionNotify;

        if (nxagentOption(Rootless))
        {
          WindowPtr pWin = nxagentWindowPtr(X.xmotion.window);

          if (pWin)
          {
            nxagentLastEnteredWindow = pWin;
          }

          if (nxagentPulldownDialogPid == 0 && nxagentLastEnteredTopLevelWindow &&
                  (X.xmotion.y_root < nxagentLastEnteredTopLevelWindow -> drawable.y + 4))
          {
            if (pWin && nxagentClientIsDialog(wClient(pWin)) == 0 &&
                    nxagentLastEnteredTopLevelWindow -> parent == WindowTable[0] &&
                        nxagentLastEnteredTopLevelWindow -> overrideRedirect == False &&
                            X.xmotion.x_root > (nxagentLastEnteredTopLevelWindow -> drawable.x +
                                (nxagentLastEnteredTopLevelWindow -> drawable.width >> 1) - 50) &&
                                    X.xmotion.x_root < (nxagentLastEnteredTopLevelWindow -> drawable.x +
                                        (nxagentLastEnteredTopLevelWindow -> drawable.width >> 1) + 50) &&
                                            nxagentOption(Menu) == 1)
            {
              nxagentPulldownDialog(nxagentLastEnteredTopLevelWindow -> drawable.id);
            }
          }

          x.u.keyButtonPointer.rootX = X.xmotion.x_root;
          x.u.keyButtonPointer.rootY = X.xmotion.y_root;
        }
        else
        {
          x.u.keyButtonPointer.rootX = X.xmotion.x - nxagentOption(RootX);
          x.u.keyButtonPointer.rootY = X.xmotion.y - nxagentOption(RootY);
        }

        x.u.keyButtonPointer.time = nxagentLastEventTime = GetTimeInMillis();

        if (viewportCursor == None &&
                !(nxagentOption(Fullscreen) &&
                    X.xmotion.window == nxagentDefaultWindows[pScreen -> myNum]
                        && X.xmotion.subwindow == None))
        {
          #ifdef NX_DEBUG_INPUT
          if (nxagentDebugInput == 1)
          {
            fprintf(stderr, "nxagentDispatchEvents: Adding motion event [%d, %d] to the queue.\n",
                        x.u.keyButtonPointer.rootX, x.u.keyButtonPointer.rootY);
          }
          #endif

          mieqEnqueue(&x);
        }

        /*
         * This test is more complicated and probably not necessary, compared
         * to a simple check on viewportCursor.
         *
         * if (!nxagentOption(Fullscreen) &&
         *       (X.xmotion.state & (ControlMask | Mod1Mask | Button1Mask)) ==
         *            (ControlMask | Mod1Mask | Button1Mask))
         */

        if (viewportCursor)
        {
          /*
           * Pointer is in viewport navigation mode.
           */

          nxagentMoveViewport(pScreen, viewportLastX - X.xmotion.x, viewportLastY - X.xmotion.y);

          viewportLastX = X.xmotion.x;
          viewportLastY = X.xmotion.y;
        }

        if (nxagentOption(ViewOnly) == 0 && nxagentOption(Shadow) && !viewportCursor)
        {
          X.xmotion.x -= nxagentOption(RootX);
          X.xmotion.y -= nxagentOption(RootY);

          if (nxagentOption(XRatio) != DONT_SCALE)
          {
            X.xmotion.x = (X.xmotion.x << PRECISION) / nxagentOption(XRatio);
          }

          if (nxagentOption(YRatio) != DONT_SCALE)
          {
            X.xmotion.y = (X.xmotion.y << PRECISION) / nxagentOption(YRatio);
          }

          NXShadowEvent(nxagentDisplay, X);
        }

        if (nxagentOption(Shadow) == 0)
        {
          nxagentInputEvent = 1;
        }

        break;
      }
      case FocusIn:
      {
        WindowPtr pWin;

        #ifdef TEST
        fprintf(stderr, "nxagentDispatchEvents: Going to handle new FocusIn event.\n");
        #endif

        /*
         * Here we change the focus state in the agent.
         * It looks like this is needed only for root-
         * less mode at the present moment.
         */

        if (nxagentOption(Rootless) &&
                (pWin = nxagentWindowPtr(X.xfocus.window)))
        {
          SetInputFocus(serverClient, inputInfo.keyboard, pWin -> drawable.id,
                            RevertToPointerRoot, GetTimeInMillis(), False);
        }

        if (X.xfocus.detail != NotifyInferior)
        {
          pScreen = nxagentScreen(X.xfocus.window);

          if (pScreen)
          {
            nxagentDirectInstallColormaps(pScreen);
          }
        }

        break;
      }
      case FocusOut:
      {
        #ifdef TEST
        fprintf(stderr, "nxagentDispatchEvents: Going to handle new FocusOut event.\n");
        #endif

        if (X.xfocus.detail != NotifyInferior)
        {
          pScreen = nxagentScreen(X.xfocus.window);

          if (pScreen)
          {
            nxagentDirectUninstallColormaps(pScreen);
          }
        }

        #ifdef NXAGENT_FIXKEYS

        {
          /*
           * Force the keys all up when focus is lost.
           */

          int i, k;
          int mask = 1;
          CARD8 val;

          XEvent xM;
          memset(&xM, 0, sizeof(XEvent));

          for (i = 0; i < DOWN_LENGTH; i++) /* input.h */
          {
            val = inputInfo.keyboard->key->down[i];

            if (val != 0)
            {
              for (k = 0; k < 8; k++)
              {
                if (val & (mask << k))
                {
                  #ifdef NXAGENT_FIXKEYS_DEBUG
                  fprintf(stderr, "sending KeyRelease event for keycode: %x\n",
                              i * 8 + k);
                  #endif

                  if (!nxagentOption(Rootless) ||
                          inputInfo.keyboard->key->modifierMap[i * 8 + k])
                  {
                    x.u.u.type = KeyRelease;
                    x.u.u.detail = i * 8 + k;
                    x.u.keyButtonPointer.time = nxagentLastEventTime = GetTimeInMillis();

                    if (nxagentOption(ViewOnly) == 0 && nxagentOption(Shadow))
                    {
                      xM.type = KeyRelease;
                      xM.xkey.display = nxagentDisplay;
                      xM.xkey.type = KeyRelease;
                      xM.xkey.keycode = i * 8 + k;
                      xM.xkey.state = inputInfo.keyboard->key->state;
                      xM.xkey.time = GetTimeInMillis();
                      NXShadowEvent(nxagentDisplay, xM);
                    }

                    mieqEnqueue(&x);
                  }
                }
              }
            }
          }

          nxagentKeyDown = 0;
        }

        #endif /* NXAGENT_FIXKEYS */

        break;
      }
      case KeymapNotify:
      {
        #ifdef TEST
        fprintf(stderr, "nxagentDispatchEvents: Going to handle new KeymapNotify event.\n");
        #endif

        break;
      }
      case EnterNotify:
      {
        WindowPtr pWin;

        WindowPtr pTLWin = NULL;

        #ifdef TEST
        fprintf(stderr, "nxagentDispatchEvents: Going to handle new EnterNotify event.\n");
        #endif

        if (nxagentOption(Rootless))
        {
          pWin = nxagentWindowPtr(X.xcrossing.window);

          if (pWin != NULL)
          {
            for (pTLWin = pWin;
                     pTLWin -> parent != WindowTable[pTLWin -> drawable.pScreen -> myNum];
                         pTLWin = pTLWin -> parent);
          }

          if (pTLWin)
          {
            nxagentLastEnteredTopLevelWindow = pTLWin;
          }

          #ifdef TEST
          fprintf(stderr, "nxagentDispatchEvents: nxagentLastEnteredTopLevelWindow [%p].\n",
                      nxagentLastEnteredTopLevelWindow);
          #endif
        }

        if (nxagentOption(Rootless) && nxagentWMIsRunning &&
                (pWin = nxagentWindowPtr(X.xcrossing.window)) &&
                    nxagentWindowTopLevel(pWin) && !pWin -> overrideRedirect &&
                        (pWin -> drawable.x != X.xcrossing.x_root - X.xcrossing.x - pWin -> borderWidth ||
                            pWin -> drawable.y != X.xcrossing.y_root - X.xcrossing.y - pWin -> borderWidth))
        {
          /*
           * This code is useful for finding the window
           * position. It should be re-implemented by
           * following the ICCCM 4.1.5 recommendations.
           */

          XID values[4];
          register XID *value = values;
          Mask mask = 0;
          ClientPtr pClient = wClient(pWin);

          #ifdef TEST
          fprintf(stderr, "nxagentDispatchEvents: pWin -> drawable.x [%d] pWin -> drawable.y [%d].\n",
                      pWin -> drawable.x, pWin -> drawable.y);
          #endif

          *value++ = (XID) (X.xcrossing.x_root - X.xcrossing.x - pWin -> borderWidth);
          *value++ = (XID) (X.xcrossing.y_root - X.xcrossing.y - pWin -> borderWidth);

          /*
           * nxagentWindowPriv(pWin)->x = (X.xcrossing.x_root - X.xcrossing.x);
           * nxagentWindowPriv(pWin)->y = (X.xcrossing.y_root - X.xcrossing.y);
           */

          mask = CWX | CWY;

          nxagentScreenTrap = 1;

          ConfigureWindow(pWin, mask, (XID *) values, pClient);

          nxagentScreenTrap = 0;
        }

        if (nxagentOption(Fullscreen) == 1 &&
                X.xcrossing.window == nxagentFullscreenWindow &&
                    X.xcrossing.detail != NotifyInferior)
        {
          nxagentGrabPointerAndKeyboard(&X);
        }

        if (X.xcrossing.detail != NotifyInferior)
        {
          pScreen = nxagentScreen(X.xcrossing.window);

          if (pScreen)
          {
            NewCurrentScreen(pScreen, X.xcrossing.x, X.xcrossing.y);

            x.u.u.type = MotionNotify;

            if (nxagentOption(Rootless))
            {
              nxagentLastEnteredWindow = nxagentWindowPtr(X.xcrossing.window);
              x.u.keyButtonPointer.rootX = X.xcrossing.x_root;
              x.u.keyButtonPointer.rootY = X.xcrossing.y_root;
            }
            else
            {
              x.u.keyButtonPointer.rootX = X.xcrossing.x - nxagentOption(RootX);
              x.u.keyButtonPointer.rootY = X.xcrossing.y - nxagentOption(RootY);
            }

            x.u.keyButtonPointer.time = nxagentLastEventTime = GetTimeInMillis();

            mieqEnqueue(&x);

            nxagentDirectInstallColormaps(pScreen);
          }
        }

        nxagentInputEvent = 1;

        break;
      }
      case LeaveNotify:
      {
        #ifdef TEST
        fprintf(stderr, "nxagentDispatchEvents: Going to handle new LeaveNotify event.\n");
        #endif

        if (nxagentOption(Rootless) && X.xcrossing.mode == NotifyNormal &&
                X.xcrossing.detail != NotifyInferior)
        {
          nxagentLastEnteredWindow = NULL;
        }

        if (X.xcrossing.window == nxagentDefaultWindows[0] &&
                X.xcrossing.detail != NotifyInferior &&
                    X.xcrossing.mode == NotifyNormal)
        {
          nxagentUngrabPointerAndKeyboard(&X);
        }

        if (X.xcrossing.detail != NotifyInferior)
        {
          pScreen = nxagentScreen(X.xcrossing.window);

          if (pScreen)
          {
            nxagentDirectUninstallColormaps(pScreen);
          }
        }

        nxagentInputEvent = 1;

        break;
      }
      case DestroyNotify:
      {
        #ifdef TEST
        fprintf(stderr, "nxagentDispatchEvents: Going to handle new DestroyNotify event.\n");
        #endif

        if (nxagentParentWindow != (Window) 0 &&
                X.xdestroywindow.window == nxagentParentWindow)
        {
          fprintf(stderr, "Warning: Unhandled destroy notify event received in agent.\n");
        }

        break;
      }
      case ClientMessage:
      {
        enum HandleEventResult result;

        #ifdef TEST
        fprintf(stderr, "nxagentDispatchEvents: Going to handle new ClientMessage event.\n");
        #endif

        nxagentHandleClientMessageEvent(&X, &result);

        if (result == doCloseSession)
        {
          closeSession = TRUE;
        }

        break;
      }
      case VisibilityNotify:
      {
        #ifdef TEST
        fprintf(stderr, "nxagentDispatchEvents: Going to handle new VisibilityNotify event.\n");
        #endif

        if (X.xvisibility.window != nxagentDefaultWindows[0])
        {
          Window window = X.xvisibility.window;

          WindowPtr pWin = nxagentWindowPtr(window);

          if (pWin && nxagentWindowPriv(pWin))
          {
            if (nxagentWindowPriv(pWin) -> visibilityState != X.xvisibility.state)
            {
              int value = X.xvisibility.state;

              if (nxagentOption(Rootless) == 1)
              {
                TraverseTree(pWin, nxagentChangeVisibilityPrivate, &value);
              }
              else
              {
                nxagentChangeVisibilityPrivate(pWin, &value);
              }
            }
          }

          #ifdef TEST
          fprintf(stderr, "nxagentDispatchEvents: Suppressing visibility notify on window [%lx].\n",
                      X.xvisibility.window);
          #endif

          break;
        }

        #ifdef TEST
        fprintf(stderr, "nxagentDispatchEvents: Visibility notify state is [%d] with previous [%d].\n",
                    X.xvisibility.state, nxagentVisibility);
        #endif

        nxagentVisibility = X.xvisibility.state;

        break;
      }
      case Expose:
      {
        #ifdef DEBUG
        fprintf(stderr, "nxagentDispatchEvents: Going to handle new Expose event.\n");
        #endif

        #ifdef DEBUG
        fprintf(stderr, "nxagentDispatchEvents: WARNING! Received Expose event "
                    "for drawable [%lx] geometry [%d, %d, %d, %d] count [%d].\n",
                        X.xexpose.window, X.xexpose.x, X.xexpose.y, X.xexpose.width,
                            X.xexpose.height, X.xexpose.count);
        #endif

        nxagentHandleExposeEvent(&X);

        break;
      }
      case GraphicsExpose:
      {
        #ifdef DEBUG
        fprintf(stderr, "nxagentDispatchEvents: Going to handle new GraphicsExpose event.\n");
        #endif

        #ifdef DEBUG
        fprintf(stderr, "nxagentDispatchEvents: WARNING! Received GraphicsExpose event "
                    "for drawable [%lx] geometry [%d, %d, %d, %d] count [%d].\n",
                        X.xgraphicsexpose.drawable, X.xgraphicsexpose.x, X.xgraphicsexpose.y,
                            X.xgraphicsexpose.width, X.xgraphicsexpose.height,
                                X.xgraphicsexpose.count);
        #endif

        nxagentHandleGraphicsExposeEvent(&X);

        break;
      }
      case NoExpose:
      {
        #ifdef DEBUG
        fprintf(stderr, "nxagentDispatchEvents: Going to handle new NoExpose event.\n");
        #endif

        #ifdef DEBUG
        fprintf(stderr, "nxagentDispatchEvents: WARNING! Received NoExpose event for "
                    "drawable [%lx].\n", X.xnoexpose.drawable);
        #endif

        break;
      }
      case CirculateNotify:
      {
        /*
         * WindowPtr pWin;
         * WindowPtr pSib;
         * ClientPtr pClient;

         * XID values[2];
         * register XID *value = values;
         * Mask mask = 0;
         */

        #ifdef WARNING
        fprintf(stderr, "nxagentDispatchEvents: Going to handle new CirculateNotify event.\n");
        #endif

        /*
         * FIXME: Do we need this?
         *
         * pWin = nxagentWindowPtr(X.xcirculate.window);
         *
         * if (!pWin)
         * {
         *   pWin = nxagentRootlessTopLevelWindow(X.xcirculate.window);
         * }
         *
         * if (!pWin)
         * {
         *   break;
         * }
         *
         * XQueryTree(nxagentDisplay, DefaultRootWindow(nxagentDisplay),
         *                &root_return, &parent_return, &children_return, &nchildren_return);
         *
         * nxagentRootlessRestack(children_return, nchildren_return);
         */

        break;
      }
      case ConfigureNotify:
      {
        #ifdef TEST
        fprintf(stderr, "nxagentDispatchEvents: Going to handle new ConfigureNotify event.\n");
        #endif

        if (nxagentConfiguredSynchroWindow == X.xconfigure.window)
        {
          if (nxagentExposeQueue.exposures[nxagentExposeQueue.start].serial != X.xconfigure.x)
          {
            #ifdef WARNING
            if (nxagentVerbose == 1)
            {
              fprintf(stderr, "nxagentDispatchEvents: Requested ConfigureNotify changes didn't take place.\n");
            }
            #endif
          }

          #ifdef TEST
          fprintf(stderr, "nxagentDispatchEvents: Received ConfigureNotify and going to call "
                      "nxagentSynchronizeExpose.\n");
          #endif

          nxagentSynchronizeExpose();

          break;
        }

        nxagentHandleConfigureNotify(&X);

        break;
      }
      case GravityNotify:
      {
        #ifdef TEST
        fprintf(stderr, "nxagentDispatchEvents: Going to handle new GravityNotify event.\n");
        #endif

        break;
      }
      case ReparentNotify:
      {
        #ifdef TEST
        fprintf(stderr, "nxagentDispatchEvents: Going to handle new ReparentNotify event.\n");
        #endif

        nxagentHandleReparentNotify(&X);

        break;
      }
      case UnmapNotify:
      {
        WindowPtr pWin;

        #ifdef TEST
        fprintf(stderr, "nxagentDispatchEvents: Going to handle new UnmapNotify event.\n");
        #endif

        if (nxagentOption(Rootless) == 1)
        {
          if ((pWin = nxagentRootlessTopLevelWindow(X.xunmap.window)) != NULL ||
                  ((pWin = nxagentWindowPtr(X.xunmap.window)) != NULL &&
                      nxagentWindowTopLevel(pWin) == 1))
          {
            nxagentScreenTrap = 1;

            UnmapWindow(pWin, False);

            nxagentScreenTrap = 0;
          }
        }

        if (nxagentUseNXTrans == 1 && nxagentOption(Rootless) == 0 &&
                nxagentOption(Nested) == 0 &&
                    X.xmap.window != nxagentIconWindow)
        {
          nxagentVisibility = VisibilityFullyObscured;
        }

        break;
      }
      case MapNotify:
      {
        WindowPtr pWin;
        ClientPtr pClient;

        #ifdef TEST
        fprintf(stderr, "nxagentDispatchEvents: Going to handle new MapNotify event.\n");
        #endif

        if (nxagentOption(Rootless) == 1)
        {
          Bool value = 1;

          if ((pWin = nxagentRootlessTopLevelWindow(X.xmap.window)) != NULL ||
                  ((pWin = nxagentWindowPtr(X.xmap.window)) != NULL &&
                      nxagentWindowTopLevel(pWin) == 1))
          {
            pClient = wClient(pWin);

            nxagentScreenTrap = 1;

            MapWindow(pWin, pClient);

            nxagentScreenTrap = 0;
          }

          if (pWin != NULL)
          {
            TraverseTree(pWin, nxagentChangeMapPrivate, &value);
          }
        }

        if (nxagentOption(AllScreens) == 1)
        {
          if (X.xmap.window == nxagentIconWindow)
          {
            pScreen = nxagentScreen(X.xmap.window);
            nxagentMaximizeToFullScreen(pScreen);
          }
        }

        if (nxagentOption(Fullscreen) == 1)
        {
          nxagentVisibility = VisibilityUnobscured;
          nxagentVisibilityStop = False;
          nxagentVisibilityTimeout = GetTimeInMillis() + 2000;
        }

        break;
      }
      case MappingNotify:
      {
        XMappingEvent *mappingEvent = (XMappingEvent *) &X;

        #ifdef DEBUG
        fprintf(stderr, "nxagentDispatchEvents: WARNING! Going to handle new MappingNotify event.\n");
        #endif

        if (mappingEvent -> request == MappingPointer)
        {
            nxagentInitPointerMap();
        }

        break;
      }
      default:
      {
        /*
         * Let's check if this is a XKB
         * state modification event.
         */

        if (nxagentHandleKeyboardEvent(&X) == 0 && nxagentHandleXFixesSelectionNotify(&X) == 0)
        {
          #ifdef TEST
          fprintf(stderr, "nxagentDispatchEvents: WARNING! Unhandled event code [%d].\n",
                      X.type);
          #endif
        }

        break;
      }

    } /* End of switch (X.type) */

    if (X.xany.serial < lastEventSerial)
    {
      /*
       * Start over.
       */

      nxagentDeleteStaticResizedWindow(0);
    }
    else
    {
      nxagentDeleteStaticResizedWindow(X.xany.serial - 1);
    }

    lastEventSerial = X.xany.serial;

  } /* End of while (...) */

  /*
   * Send the exposed regions to the clients.
   */

  nxagentForwardRemoteExpose();

  /*
   * Handle the agent window's changes.
   */
  
  if (closeSession)
  {
    if (nxagentOption(Persistent))
    {
      if (nxagentNoDialogIsRunning)
      {
        nxagentLaunchDialog(DIALOG_SUSPEND_SESSION);
      }
    }
    else
    {
      if (nxagentNoDialogIsRunning)
      {
        nxagentLaunchDialog(DIALOG_KILL_SESSION);
      }
    }
  }

  if (minimize)
  {
    nxagentWMDetect();

    if (nxagentWMIsRunning)
    {
      if (nxagentOption(AllScreens))
      {
        nxagentMinimizeFromFullScreen(pScreen);
      }
      else
      {
        XIconifyWindow(nxagentDisplay, nxagentDefaultWindows[0],
                           DefaultScreen(nxagentDisplay));
      }
    }
  }

  if (switchFullscreen)
  {
    if (nxagentOption(AllScreens) == 1 && nxagentOption(Fullscreen) == 1)
    {
      nxagentSwitchAllScreens(pScreen, 0);
    }
    else
    {
      nxagentSwitchFullscreen(pScreen, !nxagentOption(Fullscreen));
    }
  }

  if (switchAllScreens)
  {
    if (nxagentOption(AllScreens) == 0 && nxagentOption(Fullscreen) == 1)
    {
      nxagentSwitchFullscreen(pScreen, 0);
    }
    else
    {
      nxagentSwitchAllScreens(pScreen, !nxagentOption(AllScreens));
    }
  }

  if (startKbd)
  {
    if (xkbdRunning)
    {
      #ifdef NXAGENT_XKBD_DEBUG
      fprintf(stderr, "Events: nxkbd now is NOT running: %d, %d\n",
                  X.xkey.keycode, escapecode);
      #endif

      xkbdRunning = False;

      kill(pidkbd, 9);
    }
    else
    {
      char kbddisplay[6];
      char *kbdargs[6];

      strcpy(kbddisplay,":");
      strncat(kbddisplay, display, 4);

      kbdargs[0] = "nxkbd";
      kbdargs[1] = "-geometry";
      kbdargs[2] = "240x70+0+250";
      kbdargs[3] = "-display";
      kbdargs[4] = kbddisplay;
      kbdargs[5] = NULL;

      switch (pidkbd = fork())
      {
        case 0:
        {
          execvp(kbdargs[0], kbdargs);

          #ifdef NXAGENT_XKBD_DEBUG
          fprintf(stderr, "Events: The execvp of nxkbd process failed.\n");
          #endif

          exit(1);

          break;
        }
        case -1:
        {
          #ifdef NXAGENT_XKBD_DEBUG
          fprintf(stderr, "Events: Can't fork to run the nxkbd process.\n");
          #endif

          break;
        }
        default:
        {
          break;
        }
      }

      #ifdef NXAGENT_XKBD_DEBUG
      fprintf(stderr, "Events: The nxkbd process now running with [%d][%d].\n",
                  X.xkey.keycode, escapecode);
      #endif

      xkbdRunning = True;
    }
  }

  #ifdef BLOCKS
  fprintf(stderr, "[End read]\n");
  #endif

  /*
   * Let the underlying X server code
   * process the input events.
   */

  #ifdef BLOCKS
  fprintf(stderr, "[Begin events]\n");
  #endif

  ProcessInputEvents();

  #ifdef TEST
  fprintf(stderr, "nxagentDispatchEvents: Output pending flag is [%d] critical [%d].\n",
              NewOutputPending, CriticalOutputPending);
  #endif

  /*
   * Write the events to our clients. We may
   * flush only in the case of critical output
   * but this doesn't seem beneficial.
   *
   * if (CriticalOutputPending == 1)
   * {
   *   FlushAllOutput();
   * }
   */

  if (NewOutputPending == 1)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentDispatchEvents: Flushed the processed events to clients.\n");
    #endif

    FlushAllOutput();
  }

  #ifdef TEST

  if (nxagentPendingEvents(nxagentDisplay) > 0)
  {
    fprintf(stderr, "nxagentDispatchEvents: WARNING! More events need to be dispatched.\n");
  }

  #endif

  #ifdef BLOCKS
  fprintf(stderr, "[End events]\n");
  #endif
}

/*
 * Functions providing the ad-hoc handling
 * of the remote X events.
 */

int nxagentHandleKeyPress(XEvent *X, enum HandleEventResult *result)
{
  xEvent x;

  if (nxagentXkbState.Initialized == 0)
  {
    if (X -> xkey.keycode == nxagentCapsLockKeycode)
    {
      nxagentXkbCapsTrap = 1;
    }
    else if (X -> xkey.keycode == nxagentNumLockKeycode)
    {
      nxagentXkbNumTrap = 1;
    }

    nxagentInitKeyboardState();

    nxagentXkbCapsTrap = 0;
    nxagentXkbNumTrap = 0;
  }

  if (nxagentCheckSpecialKeystroke(&X -> xkey, result))
  {
    return 1;
  }

  if (X -> xkey.keycode == nxagentCapsLockKeycode)
  {
    nxagentXkbState.Caps = (~nxagentXkbState.Caps & 1);
  }
  else if (X -> xkey.keycode == nxagentNumLockKeycode)
  {
    nxagentXkbState.Num = (~nxagentXkbState.Num & 1);
  }

  nxagentLastEventTime = nxagentLastKeyPressTime = GetTimeInMillis();
  
  x.u.u.type = KeyPress;
  x.u.u.detail = nxagentConvertKeycode(X -> xkey.keycode);
  x.u.keyButtonPointer.time = nxagentLastKeyPressTime;

  nxagentLastServerTime = X -> xkey.time;

  mieqEnqueue(&x);

  CriticalOutputPending = 1;

  return 1;
}

int nxagentHandlePropertyNotify(XEvent *X)
{
  int resource;

  if (nxagentOption(Rootless) && !nxagentNotifyMatchChangeProperty((XPropertyEvent *) X))
  {
    #ifdef TEST
    fprintf(stderr, "nxagentHandlePropertyNotify: Property %ld on window %lx.\n",
                X -> xproperty.atom, X -> xproperty.window);
    #endif

    if (nxagentWindowPtr(X -> xproperty.window) != NULL)
    {
      resource = NXGetCollectPropertyResource(nxagentDisplay);

      if (resource == -1)
      {
        #ifdef WARNING
        fprintf(stderr, "nxagentHandlePropertyNotify: WARNING! Asyncronous get property queue is full.\n");
        #endif

        return 0;
      }

      NXCollectProperty(nxagentDisplay, resource,
                            X -> xproperty.window, X -> xproperty.atom, 0,
                                MAX_RETRIEVED_PROPERTY_SIZE, False, AnyPropertyType);

      nxagentPropertyRequests[resource].window = X -> xproperty.window;
      nxagentPropertyRequests[resource].property = X -> xproperty.atom;
    }
    #ifdef TEST
    else
    {
      fprintf(stderr, "nxagentHandlePropertyNotify: Failed to look up remote window property.\n");
    }
    #endif
  }

  return 1;
}

int nxagentHandleExposeEvent(XEvent *X)
{
  WindowPtr pWin = NULL;
  Window window = None;

  RegionRec sum;
  RegionRec add;
  BoxRec box;
  int index = 0;
  int overlap = 0;

  StaticResizedWindowStruct *resizedWinPtr = NULL;

  #ifdef DEBUG
  fprintf(stderr, "nxagentHandleExposeEvent: Checking remote expose events.\n");
  #endif

  #ifdef DEBUG
  fprintf(stderr, "nxagentHandleExposeEvent: Looking for window id [%ld].\n",
              X -> xexpose.window);
  #endif

  window = X -> xexpose.window;

  pWin = nxagentWindowPtr(window);

  if (pWin != NULL)
  {
    REGION_INIT(pWin -> drawable.pScreen, &sum, (BoxRec *) NULL, 1);
/*
FIXME: This can be maybe optimized by consuming the
       events that do not match the predicate.
*/
    do
    {
      #ifdef DEBUG
      fprintf(stderr, "nxagentHandleExposeEvent: Adding event for window id [%ld].\n",
                  X -> xexpose.window);
      #endif

      box.x1 = pWin -> drawable.x + wBorderWidth(pWin) + X -> xexpose.x;
      box.y1 = pWin -> drawable.y + wBorderWidth(pWin) + X -> xexpose.y;

      resizedWinPtr = nxagentFindStaticResizedWindow(X -> xany.serial);

      while (resizedWinPtr)
      {
        if (resizedWinPtr -> pWin == pWin)
        {
          box.x1 += resizedWinPtr -> offX;
          box.y1 += resizedWinPtr -> offY;
        }

        resizedWinPtr = resizedWinPtr -> prev;
      }

      box.x2 = box.x1 + X -> xexpose.width;
      box.y2 = box.y1 + X -> xexpose.height;

      REGION_INIT(pWin -> drawable.pScreen, &add, &box, 1);

      REGION_APPEND(pWin -> drawable.pScreen, &sum, &add);

      REGION_UNINIT(pWin -> drawable.pScreen, &add);

      if (X -> xexpose.count == 0)
      {
        break;
      }
    }
    while (nxagentCheckEvents(nxagentDisplay, X, nxagentExposurePredicate,
                                  (XPointer) &window) == 1);

    REGION_VALIDATE(pWin -> drawable.pScreen, &sum, &overlap);

    REGION_INTERSECT(pWin->drawable.pScreen, &sum, &sum,
                         &WindowTable[pWin->drawable.pScreen->myNum]->winSize);

    #ifdef DEBUG
    fprintf(stderr, "nxagentHandleExposeEvent: Sending events for window id [%ld].\n",
                X -> xexpose.window);
    #endif

    /*
     * If the agent has already sent auto-generated expose,
     * save received exposes for later processing.
     */

    index = nxagentLookupByWindow(pWin);

    if (index == -1)
    {
      miWindowExposures(pWin, &sum, NullRegion);
    }
    else
    {
      REGION_TRANSLATE(pWin -> drawable.pScreen, &sum, -pWin -> drawable.x, -pWin -> drawable.y);

      if (nxagentExposeQueue.exposures[index].remoteRegion == NullRegion)
      {
        nxagentExposeQueue.exposures[index].remoteRegion = REGION_CREATE(pwin -> drawable.pScreen, NULL, 1);
      }

      REGION_UNION(pWin -> drawable.pScreen, nxagentExposeQueue.exposures[index].remoteRegion,
                       nxagentExposeQueue.exposures[index].remoteRegion, &sum);

      #ifdef TEST
      fprintf(stderr, "nxagentHandleExposeEvent: Added region for window [%ld] to position [%d].\n",
                  nxagentWindow(pWin), index);
      #endif

      if (X -> xexpose.count == 0)
      {
        nxagentExposeQueue.exposures[index].remoteRegionIsCompleted = True;
      }
      else
      {
        nxagentExposeQueue.exposures[index].remoteRegionIsCompleted = False;
      }
    }

    if (nxagentRootTileWindow != NULL)
    {
      if (nxagentWindowPriv(nxagentRootTileWindow) -> window == nxagentWindowPriv(pWin) -> window &&
              nxagentSplashCount == 1 && X -> xexpose.count == 0)
      {
        #ifdef DEBUG
        fprintf(stderr, "nxagentHandleExposeEvent: Clearing root tile window id [%ld].\n",
                    nxagentWindowPriv(nxagentRootTileWindow) -> window);
        #endif

        XClearWindow(nxagentDisplay, nxagentWindowPriv(nxagentRootTileWindow) -> window);
      }
    }

    REGION_UNINIT(pWin -> drawable.pScreen, &sum);
  }

  return 1;
}

int nxagentHandleGraphicsExposeEvent(XEvent *X)
{
  /*
   * Send an expose event to client, instead of graphics
   * expose. If target drawable is a backing pixmap, send
   * expose event for the saved window, else do nothing.
   */

  RegionPtr exposeRegion;
  BoxRec rect;
  WindowPtr pWin;
  ScreenPtr pScreen;
  StoringPixmapPtr pStoringPixmapRec = NULL;
  miBSWindowPtr pBSwindow = NULL;
  int drawableType;

  pWin = nxagentWindowPtr(X -> xgraphicsexpose.drawable);

  if (pWin != NULL)
  {
    drawableType = DRAWABLE_WINDOW;
  }
  else
  {
    drawableType = DRAWABLE_PIXMAP;
  }

  if (drawableType == DRAWABLE_PIXMAP)
  {
    pStoringPixmapRec = nxagentFindItemBSPixmapList(X -> xgraphicsexpose.drawable);

    if (pStoringPixmapRec == NULL)
    {
      #ifdef TEST
      fprintf(stderr, "nxagentHandleGraphicsExposeEvent: WARNING! Storing pixmap not found.\n");
      #endif

      return 1;
    }

    pBSwindow = (miBSWindowPtr) pStoringPixmapRec -> pSavedWindow -> backStorage;

    if (pBSwindow == NULL)
    {
      #ifdef TEST
      fprintf(stderr, "nxagentHandleGraphicsExposeEvent: WARNING! Back storage not found.\n");
      #endif

      return 1;
    }

    pWin = pStoringPixmapRec -> pSavedWindow;
  }

  pScreen = pWin -> drawable.pScreen;

  /*
   * Rectangle affected by GraphicsExpose
   * event.
   */

  rect.x1 = X -> xgraphicsexpose.x;
  rect.y1 = X -> xgraphicsexpose.y;
  rect.x2 = rect.x1 + X -> xgraphicsexpose.width;
  rect.y2 = rect.y1 + X -> xgraphicsexpose.height;

  exposeRegion = REGION_CREATE(pScreen, &rect, 0);

  if (drawableType == DRAWABLE_PIXMAP)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentHandleGraphicsExposeEvent: Handling GraphicsExpose event on pixmap with id"
                " [%lu].\n", X -> xgraphicsexpose.drawable);
    #endif

    /*
     * The exposeRegion coordinates are relative
     * to the pixmap to which GraphicsExpose
     * event refers. But the BS coordinates of
     * the savedRegion  are relative to the
     * window.
     */

    REGION_TRANSLATE(pScreen, exposeRegion, pStoringPixmapRec -> backingStoreX,
                         pStoringPixmapRec -> backingStoreY);

    /*
     * We remove from SavedRegion the part
     * affected by the GraphicsExpose event.
     */

    REGION_SUBTRACT(pScreen, &(pBSwindow -> SavedRegion), &(pBSwindow -> SavedRegion),
                        exposeRegion);
  }

  /*
   * Store the exposeRegion in order to send
   * the expose event later. The coordinates
   * must be relative to the screen.
   */

  REGION_TRANSLATE(pScreen, exposeRegion, pWin -> drawable.x, pWin -> drawable.y);

  REGION_UNION(pScreen, nxagentRemoteExposeRegion, nxagentRemoteExposeRegion, exposeRegion);

  REGION_DESTROY(pScreen, exposeRegion);

  return 1;
}

int nxagentHandleClientMessageEvent(XEvent *X, enum HandleEventResult *result)
{
  ScreenPtr pScreen;
  WindowPtr pWin;
  xEvent x;

  *result = doNothing;

  #ifdef TEST
  fprintf(stderr, "nxagentHandleClientMessageEvent: ClientMessage event window [%ld] with "
              "type [%ld] format [%d].\n", X -> xclient.window, X -> xclient.message_type,
                  X -> xclient.format);
  #endif

  /*
   * If window is 0, message_type is 0 and format is
   * 32 then we assume event is coming from proxy.
   */

  if (X -> xclient.window == 0 &&
          X -> xclient.message_type == 0 &&
              X -> xclient.format == 32)
  {
    nxagentHandleProxyEvent(X);

    return 1;
  }

  if (nxagentOption(Rootless))
  {
    Atom message_type = nxagentRemoteToLocalAtom(X -> xclient.message_type);

    if (!ValidAtom(message_type))
    {
      #ifdef WARNING
      fprintf(stderr, "nxagentHandleClientMessageEvent: WARNING Invalid type in client message.\n");
      #endif

      return 0;
    }

    pWin = nxagentWindowPtr(X -> xclient.window);

    if (pWin == NULL)
    {
      #ifdef WARNING
      fprintf(stderr, "WARNING: Invalid window in ClientMessage.\n");
      #endif

      return 0;
    }

    if (message_type == MakeAtom("WM_PROTOCOLS", strlen("WM_PROTOCOLS"), False))
    {
      char *message_data;

      x.u.u.type = ClientMessage;
      x.u.u.detail = X -> xclient.format;

      x.u.clientMessage.window = pWin -> drawable.id;
      x.u.clientMessage.u.l.type = message_type;
      x.u.clientMessage.u.l.longs0 = nxagentRemoteToLocalAtom(X -> xclient.data.l[0]);
      x.u.clientMessage.u.l.longs1 = GetTimeInMillis();

      if (!ValidAtom(x.u.clientMessage.u.l.longs0))
      {
        #ifdef WARNING
        fprintf(stderr, "nxagentHandleClientMessageEvent: WARNING Invalid value in client message "
                    "of type WM_PROTOCOLS.\n");
        #endif

        return 0;
      }
      else
      {
        message_data = validateString(NameForAtom(x.u.clientMessage.u.l.longs0));
      }

      #ifdef TEST
      fprintf(stderr, "nxagentHandleClientMessageEvent: Sent client message of type WM_PROTOCOLS "
                  "and value [%s].\n", message_data);
      #endif

      TryClientEvents(wClient(pWin), &x, 1, 1, 1, 0);
    }
    else
    {
      #ifdef WARNING
      fprintf(stderr, "nxagentHandleClientMessageEvent: Ignored message type %ld [%s].\n",
                  (long int) message_type, validateString(NameForAtom(message_type)));
      #endif

      return 0;
    }

    return 1;
  }

  if (X -> xclient.message_type == nxagentAtoms[1]) /* WM_PROTOCOLS */
  {
    Atom deleteWMatom, wmAtom;

    wmAtom = (Atom) X -> xclient.data.l[0];

    deleteWMatom = nxagentAtoms[2]; /* WM_DELETE_WINDOW */

    if (wmAtom == deleteWMatom)
    {
      if (nxagentOnce && (nxagentClients == 0))
      {
        GiveUp(0);
      }
      else
      {
        #ifdef TEST
        fprintf(stderr, "Events: WM_DELETE_WINDOW arrived Atom = %ld.\n", wmAtom);
        #endif

        if (X -> xclient.window == nxagentIconWindow)
        {
          pScreen = nxagentScreen(X -> xmap.window);

          XMapRaised(nxagentDisplay, nxagentFullscreenWindow);

          XIconifyWindow(nxagentDisplay, nxagentIconWindow,
                             DefaultScreen(nxagentDisplay));

        }

        if (X -> xclient.window == (nxagentOption(Fullscreen) ?
                nxagentIconWindow : nxagentDefaultWindows[0]) ||
                    nxagentWMIsRunning == 0)
        {
          *result = doCloseSession;
        }
      }
    }
  }

  return 1;
}

int nxagentHandleKeyboardEvent(XEvent *X)
{
  XkbEvent *xkbev = (XkbEvent *) X;

  #ifdef TEST
  fprintf(stderr, "nxagentHandleKeyboardEvent: Handling event with caps [%d] num [%d] locked [%d].\n",
              nxagentXkbState.Caps, nxagentXkbState.Num, nxagentXkbState.Locked);
  #endif

  if (xkbev -> type == nxagentXkbInfo.EventBase + XkbEventCode &&
          xkbev -> any.xkb_type == XkbStateNotify)
  {
    nxagentXkbState.Locked = xkbev -> state.locked_mods;

    #ifdef TEST
    fprintf(stderr, "nxagentHandleKeyboardEvent: Updated XKB locked modifier bits to [%x].\n",
                nxagentXkbState.Locked);
    #endif

    nxagentXkbState.Initialized = 1;

    if (nxagentXkbState.Caps == 0 &&
            (nxagentXkbState.Locked & CAPSFLAG_IN_EVENT))
    {
      nxagentXkbState.Caps = 1;

      #ifdef TEST
      fprintf(stderr, "nxagentHandleKeyboardEvent: Sending fake key [66] to engage capslock.\n");
      #endif

      if (!nxagentXkbCapsTrap)
      {
        nxagentSendFakeKey(66);
      }
    }

    if (nxagentXkbState.Caps == 1 &&
          !(nxagentXkbState.Locked & CAPSFLAG_IN_EVENT))
    {
      nxagentXkbState.Caps = 0;

      #ifdef TEST
      fprintf(stderr, "nxagentHandleKeyboardEvent: Sending fake key [66] to release capslock.\n");
      #endif

      nxagentSendFakeKey(66);
    }

    if (nxagentXkbState.Caps == 0 &&
          !(nxagentXkbState.Locked & CAPSFLAG_IN_EVENT) &&
              nxagentXkbCapsTrap)
    {

      #ifdef TEST
      fprintf(stderr, "nxagentHandleKeyboardEvent: Sending fake key [66] to release capslock.\n");
      #endif

      nxagentSendFakeKey(66);
    }

    if (nxagentXkbState.Num == 0 &&
            (nxagentXkbState.Locked & NUMFLAG_IN_EVENT))
    {
      nxagentXkbState.Num = 1;

      #ifdef TEST
      fprintf(stderr, "nxagentHandleKeyboardEvent: Sending fake key [77] to engage numlock.\n");
      #endif

      if (!nxagentXkbNumTrap)
      {
        nxagentSendFakeKey(77);
      }
    }

    if (nxagentXkbState.Num == 1 &&
            !(nxagentXkbState.Locked & NUMFLAG_IN_EVENT))
    {
      nxagentXkbState.Num = 0;

      #ifdef TEST
      fprintf(stderr, "nxagentHandleKeyboardEvent: Sending fake key [77] to release numlock.\n");
      #endif

      nxagentSendFakeKey(77);
    }

    if (nxagentXkbState.Num == 0 &&
          !(nxagentXkbState.Locked & NUMFLAG_IN_EVENT) &&
              nxagentXkbNumTrap)
    {

      #ifdef TEST
      fprintf(stderr, "nxagentHandleKeyboardEvent: Sending fake key [77] to release numlock.\n");
      #endif

      nxagentSendFakeKey(77);
    }

    return 1;
  }

  return 0;
}

int nxagentHandleXFixesSelectionNotify(XEvent *X)
{
  int i;
  Atom local;

  XFixesSelectionEvent *xfixesEvent = (XFixesSelectionEvent *) X;

  if (nxagentXFixesInfo.Initialized == 0 ||
          xfixesEvent -> type != (nxagentXFixesInfo.EventBase + XFixesSelectionNotify))
    return 0;

  #ifdef TEST
  fprintf(stderr, "nxagentHandleXFixesSelectionNotify: Handling event.\n");
  #endif

  local = nxagentRemoteToLocalAtom(xfixesEvent -> xfixesselection.selection);

  if (SelectionCallback)
  {
    i = 0;

    while ((i < NumCurrentSelections) &&
            CurrentSelections[i].selection != local)
      i++;

    if (i < NumCurrentSelections)
    {
      SelectionInfoRec    info;

      if (CurrentSelections[i].client != 0)
      {
        #ifdef TEST
        fprintf(stderr, "nxagentHandleXFixesSelectionNotify: Do nothing.\n");
        #endif

        return 1;
      }

      #ifdef TEST
      fprintf(stderr, "nxagentHandleXFixesSelectionNotify: Calling callbacks for %d [%s] selection.\n",
                       CurrentSelections[i].selection, NameForAtom(CurrentSelections[i].selection));
      #endif

      #ifdef DEBUG
      fprintf(stderr, "nxagentHandleXFixesSelectionNotify: Subtype ");

      switch (xfixesEvent -> xfixesselection.subtype)
      {
        case SelectionSetOwner:
          fprintf(stderr, "SelectionSetOwner.\n");
          break;
        case SelectionWindowDestroy:
          fprintf(stderr, "SelectionWindowDestroy.\n");
          break;
        case SelectionClientClose:
          fprintf(stderr, "SelectionClientClose.\n");
          break;
        default:
          fprintf(stderr, ".\n");
          break;
      }
      #endif

      info.selection = &CurrentSelections[i];
      info.kind = xfixesEvent->xfixesselection.subtype;
      CallCallbacks(&SelectionCallback, &info);
    }
  }
  return 1;
}

int nxagentHandleProxyEvent(XEvent *X)
{
  switch (X -> xclient.data.l[0])
  {
    case NXNoSplitNotify:
    case NXStartSplitNotify:
    {
      /*
       * We should never receive such events
       * in the event loop, as they should
       * be caught at the time the split is
       * initiated.
       */

      #ifdef PANIC

      int client = (int) X -> xclient.data.l[1];

      if (X -> xclient.data.l[0] == NXNoSplitNotify)
      {
        fprintf(stderr, "nxagentHandleProxyEvent: PANIC! NXNoSplitNotify received "
                    "with client [%d].\n", client);
      }
      else
      {
        fprintf(stderr, "nxagentHandleProxyEvent: PANIC! NXStartSplitNotify received "
                    "with client [%d].\n", client);
      }

      #endif

      return 1;
    }
    case NXCommitSplitNotify:
    {
      /*
       * We need to commit an image. Image can be the
       * result of a PutSubImage() generated by Xlib,
       * so there can be more than a single image to
       * commit, even if only one PutImage was perfor-
       * med by the agent.
       */

      int client   = (int) X -> xclient.data.l[1];
      int request  = (int) X -> xclient.data.l[2];
      int position = (int) X -> xclient.data.l[3];

      #ifdef TEST
      fprintf(stderr, "nxagentHandleProxyEvent: NXCommitSplitNotify received with "
                  "client [%d] request [%d] and position [%d].\n",
                      client, request, position);
      #endif

      nxagentHandleCommitSplitEvent(client, request, position);

      return 1;
    }
    case NXEndSplitNotify:
    {
      /*
       * All images for the split were transferred and
       * we need to restart the client.
       */

      int client = (int) X -> xclient.data.l[1];

      #ifdef TEST
      fprintf(stderr, "nxagentHandleProxyEvent: NXEndSplitNotify received with "
                  "client [%d].\n", client);
      #endif

      nxagentHandleEndSplitEvent(client);

      return 1;
    }
    case NXEmptySplitNotify:
    {
      /*
       * All splits have been completed and none remain.
       */

      #ifdef TEST
      fprintf(stderr, "nxagentHandleProxyEvent: NXEmptySplitNotify received.\n");
      #endif

      nxagentHandleEmptySplitEvent();

      return 1;
    }
    case NXCollectPropertyNotify:
    {
      #ifdef TEST
      int resource = (int) X -> xclient.data.l[1];

      fprintf(stderr, "nxagentHandleProxyEvent: NXCollectPropertyNotify received with resource [%d].\n",
                  resource);
      #endif

      nxagentHandleCollectPropertyEvent(X);

      return 1;
    }
    case NXCollectGrabPointerNotify:
    {
      int resource = (int) X -> xclient.data.l[1];

      #ifdef TEST
      fprintf(stderr, "nxagentHandleProxyEvent: NXCollectGrabPointerNotify received with resource [%d].\n",
                  resource);
      #endif

      nxagentHandleCollectGrabPointerEvent(resource);

      return 1;
    }
    case NXCollectInputFocusNotify:
    {
      int resource = (int) X -> xclient.data.l[1];

      /*
       * This is not used at the present moment.
       */

      #ifdef TEST
      fprintf(stderr, "nxagentHandleProxyEvent: NXCollectInputFocusNotify received with resource [%d].\n",
                  resource);
      #endif

      nxagentHandleCollectInputFocusEvent(resource);

      return 1;
    }
    default:
    {
      /*
       *  Not a recognized ClientMessage event.
       */

      #ifdef WARNING
      fprintf(stderr, "nxagentHandleProxyEvent: WARNING! Not a recognized ClientMessage proxy event [%d].\n",
                  (int) X -> xclient.data.l[0]);
      #endif

      return 0;
    }
  }
}

/*
 * In this function it is assumed that we never
 * get a configure with both stacking order and
 * geometry changed, this way we can ignore
 * stacking changes if the geometry has changed.
 */

int nxagentCheckWindowConfiguration(XConfigureEvent* X)
{
  static int x = 0;
  static int y = 0;
  static int width = 0;
  static int height = 0;
  static Window win = None;
  Bool geometryChanged = False;

  XlibWindow root_return = 0;
  XlibWindow parent_return = 0;
  XlibWindow *children_return = NULL;
  unsigned int nchildren_return = 0;
  Status result;

  WindowPtr pWin;

  pWin = nxagentWindowPtr(X -> window);

  /*
   * This optimization has some problems to
   * work in rootless mode inside NXWin. To 
   * verify this you can launch xterm and
   * another application, f.e. firefox. By
   * raising xterm above firefox, the stack
   * order seems to become incoherent showing
   * the underneath window content in the
   * overlapping area when the mouse botton is
   * pressed with the pointer inside of such area.
   *
   *  if ((pWin != NULL) && X -> override_redirect == 0)
   *  {
   *    return 1;
   *  }
   *
   */

  if (win == X -> window)
  {
    if (x != X -> x ||
            y != X -> y ||
                width != X -> width ||
                    height != X -> height)
    {
      geometryChanged = True;
    }
  }

  win = X -> window;

  x = X -> x;
  y = X -> y;
  width = X -> width;
  height = X -> height;

  if (geometryChanged)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentCheckWindowConfiguration: Configure frame. No restack.\n");
    #endif

    return 1;
  }

  #ifdef TEST
  {
    WindowPtr pSib;

    fprintf(stderr, "nxagentCheckWindowConfiguration: Before restacking top level window [%p]\n",
                (void *) nxagentWindowPtr(X -> window));

    for (pSib = WindowTable[0] -> firstChild; pSib; pSib = pSib -> nextSib)
    {
      fprintf(stderr, "nxagentCheckWindowConfiguration: Top level window: [%p].\n",
                  (void *) pSib);
    }
  }
  #endif

  result = XQueryTree(nxagentDisplay, DefaultRootWindow(nxagentDisplay),
                          &root_return, &parent_return, &children_return, &nchildren_return);

  if (result)
  {
    nxagentRootlessRestack(children_return, nchildren_return);
  }
  else
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentCheckWindowConfiguration: WARNING! Failed QueryTree request.\n");
    #endif
  }

  if (result && nchildren_return)
  {
    XFree(children_return);
  }

  #if 0
  fprintf(stderr, "nxagentCheckWindowConfiguration: Trees match: %s\n",
              nxagentRootlessTreesMatch() ? "Yes" : "No");
  #endif

  return 1;
}

int nxagentHandleConfigureNotify(XEvent* X)
{
  if (nxagentOption(Rootless) == True)
  {
    ClientPtr pClient;
    WindowPtr pWinWindow;
    WindowPtr pWin;
    xEvent x;
    int sendEventAnyway = 0;

    pWinWindow = nxagentWindowPtr(X -> xconfigure.window);

    #ifdef TEST
    {
      WindowPtr pWinEvent  = nxagentWindowPtr(X -> xconfigure.event);

      fprintf(stderr, "nxagentHandleConfigureNotify: Generating window is [%p][%ld] target [%p][%ld].\n",
                  (void *) pWinEvent, X -> xconfigure.event, (void *) pWinWindow, X -> xconfigure.window);
    }
    #endif

    #ifdef TEST
    fprintf(stderr, "nxagentHandleConfigureNotify: New configuration for window [%p][%ld] is [%d][%d][%d][%d] "
                "send_event [%i].\n", (void *) pWinWindow, X -> xconfigure.window,
                    X -> xconfigure.x, X -> xconfigure.y, X -> xconfigure.width,
                        X -> xconfigure.height, X -> xconfigure.send_event);
    #endif

    if ((pWin = nxagentRootlessTopLevelWindow(X -> xconfigure.window)) != NULL)
    {
      /*
       * Cheking for new geometry or stacking order changes.
       */

      nxagentCheckWindowConfiguration((XConfigureEvent*)X);

      return 1;
    }

    if (nxagentWindowTopLevel(pWinWindow) && !X -> xconfigure.override_redirect)
    {
      XID values[5];
      Mask mask = 0;

      register XID *value = values;

      pClient = wClient(pWinWindow);

      if (X -> xconfigure.send_event || !nxagentWMIsRunning ||
                X -> xconfigure.override_redirect)
      {
        *value++ = (XID)X -> xconfigure.x;
        *value++ = (XID)X -> xconfigure.y;

        /*
         * nxagentWindowPriv(pWinWindow)->x = X -> xconfigure.x;
         * nxagentWindowPriv(pWinWindow)->y = X -> xconfigure.y;
         */

        mask |= CWX | CWY;
      }

      *value++ = (XID)X -> xconfigure.width;
      *value++ = (XID)X -> xconfigure.height;
      *value++ = (XID)X -> xconfigure.border_width;

      /*
       * We don't need width and height here.
       *
       * nxagentWindowPriv(pWinWindow)->width = X -> xconfigure.width;
       * nxagentWindowPriv(pWinWindow)->height = X -> xconfigure.height;
       */

      mask |= CWHeight | CWWidth | CWBorderWidth;

      nxagentScreenTrap = 1;

      ConfigureWindow(pWinWindow, mask, (XID *) values, pClient);

      nxagentScreenTrap = 0;

      nxagentCheckWindowConfiguration((XConfigureEvent*)X);

      /*
       * This workaround should help with
       * Java 1.6.0 that seems to ignore
       * non-synthetic events.
       */

      if (nxagentOption(ClientOs) == ClientOsWinnt)
      {
        #ifdef TEST
        fprintf(stderr, "nxagentHandleConfigureNotify: Apply workaround for NXWin.\n");
        #endif

        sendEventAnyway = 1;
      }

      if (sendEventAnyway || X -> xconfigure.send_event)
      {
        x.u.u.type = X -> xconfigure.type;
        x.u.u.type |= 0x80;

        x.u.configureNotify.event = pWinWindow -> drawable.id;
        x.u.configureNotify.window = pWinWindow -> drawable.id;

        if (pWinWindow -> nextSib)
        {
          x.u.configureNotify.aboveSibling = pWinWindow -> nextSib -> drawable.id;
        }
        else
        {
          x.u.configureNotify.aboveSibling = None;
        }

        x.u.configureNotify.x = X -> xconfigure.x;
        x.u.configureNotify.y = X -> xconfigure.y;
        x.u.configureNotify.width = X -> xconfigure.width;
        x.u.configureNotify.height = X -> xconfigure.height;
        x.u.configureNotify.borderWidth = X -> xconfigure.border_width;
        x.u.configureNotify.override = X -> xconfigure.override_redirect;

        TryClientEvents(pClient, &x, 1, 1, 1, 0);
      }

      return 1;
    }
  }
  else
  {
    /*
     * Save the position of the agent default window. Don't
     * save the values if the agent is in fullscreen mode.
     *
     * If we use these values to restore the position of a
     * window after that we have dynamically changed the
     * fullscreen attribute, depending on the behaviour of
     * window manager, we could be not able to place the
     * window exactly in the requested position, so let the
     * window manager do the job for us.
     */

    ScreenPtr pScreen = nxagentScreen(X -> xconfigure.window);

    Bool doRandR = False;
    struct timeval timeout;

    if (X -> xconfigure.window == nxagentDefaultWindows[pScreen -> myNum])
    {
      if (nxagentOption(AllScreens) == 0)
      {
        if (nxagentOption(DesktopResize) == 1)
        {
          if (nxagentOption(Width) != X -> xconfigure.width ||
                nxagentOption(Height) != X -> xconfigure.height)
          {
            Bool newEvents = False;

            doRandR = True;

            NXFlushDisplay(nxagentDisplay, NXFlushLink);

            do
            {
              newEvents = False;

              timeout.tv_sec  = 0;
              timeout.tv_usec = 500 * 1000;

              nxagentWaitEvents(nxagentDisplay, &timeout);

              /*
               * This should also flush
               * the NX link for us.
               */

              XSync(nxagentDisplay, 0);

              while (XCheckTypedWindowEvent(nxagentDisplay, nxagentDefaultWindows[pScreen -> myNum],
                                              ConfigureNotify, X))
              {
                newEvents = True;
              }

            } while (newEvents);
          }
        }

        if (nxagentWMIsRunning == 0 || X -> xconfigure.send_event)
        {
          nxagentChangeOption(X, X -> xconfigure.x);
          nxagentChangeOption(Y, X -> xconfigure.y);
        }

        if (nxagentOption(Shadow) == 1 && nxagentOption(DesktopResize) == 1 &&
                (nxagentOption(Width) != X -> xconfigure.width ||
                    nxagentOption(Height) != X -> xconfigure.height))
        {
          nxagentShadowResize = 1;
        }

        nxagentChangeOption(Width, X -> xconfigure.width);
        nxagentChangeOption(Height, X -> xconfigure.height);

        nxagentChangeOption(ViewportXSpan, (int) X -> xconfigure.width -
                                (int) nxagentOption(RootWidth));
        nxagentChangeOption(ViewportYSpan, (int) X -> xconfigure.height -
                                (int) nxagentOption(RootHeight));

        nxagentMoveViewport(pScreen, 0, 0);

        if (nxagentOption(Shadow) == 1 ||
                (nxagentOption(Width) == nxagentOption(RootWidth) &&
                    nxagentOption(Height) == nxagentOption(RootHeight)))
        {
          doRandR = 0;
        }

        nxagentChangeOption(Width, X -> xconfigure.width);
        nxagentChangeOption(Height, X -> xconfigure.height);

        XMoveResizeWindow(nxagentDisplay, nxagentInputWindows[0], 0, 0,
                              X -> xconfigure.width, X -> xconfigure.height);

        if (nxagentOption(Fullscreen) == 0)
        {
          nxagentMoveViewport(pScreen, 0, 0);
        }
        else
        {
          nxagentChangeOption(RootX, (nxagentOption(Width) -
                                  nxagentOption(RootWidth)) / 2);
          nxagentChangeOption(RootY, (nxagentOption(Height) -
                                  nxagentOption(RootHeight)) / 2);
          nxagentChangeOption(ViewportXSpan, nxagentOption(Width) -
                                  nxagentOption(RootWidth));
          nxagentChangeOption(ViewportYSpan, nxagentOption(Height) -
                                  nxagentOption(RootHeight));

          nxagentUpdateViewportFrame(0, 0, nxagentOption(RootWidth),
                                         nxagentOption(RootHeight));

          XMoveWindow(nxagentDisplay, nxagentWindow(WindowTable[pScreen -> myNum]),
                          nxagentOption(RootX), nxagentOption(RootY));
        }

        if (doRandR)
        {
          #ifdef TEST
          fprintf(stderr,"nxagentHandleConfigureNotify: Width %d Height %d.\n",
                      nxagentOption(Width), nxagentOption(Height));
          #endif

          nxagentChangeScreenConfig(0, nxagentOption(Width),
                                        nxagentOption(Height), 0, 0);
        }
      }

      return 1;
    }
  }

  return 0;
}

int nxagentHandleReparentNotify(XEvent* X)
{
  #ifdef TEST
  fprintf(stderr, "nxagentHandleReparentNotify: Going to handle a new reparent event.\n");
  #endif

  if (nxagentOption(Rootless))
  {
    WindowPtr pWin;

    XlibWindow w;
    XlibWindow root_return = 0;
    XlibWindow parent_return = 0;
    XlibWindow *children_return = NULL;
    unsigned int nchildren_return = 0;
    Status result;

    pWin = nxagentWindowPtr(X -> xreparent.window);

    #ifdef TEST

    {
      WindowPtr pParent = nxagentWindowPtr(X -> xreparent.parent);
      WindowPtr pEvent = nxagentWindowPtr(X -> xreparent.event);

      fprintf(stderr, "nxagentHandleReparentNotify: event %p[%lx] window %p[%lx] parent %p[%lx] at (%d, %d)\n",
                  (void*)pEvent, X -> xreparent.event, (void*)pWin, X -> xreparent.window,
                          (void*)pParent, X -> xreparent.parent, X -> xreparent.x, X -> xreparent.y);
    }

    #endif

    if (nxagentWindowTopLevel(pWin))
    {
      /*
       * If the window manager reparents our top level
       * window, we need to know the new top level
       * ancestor.
       */

      w = None;
      parent_return = X -> xreparent.parent;

      while (parent_return != RootWindow(nxagentDisplay, 0))
      {
        w = parent_return;
        result = XQueryTree(nxagentDisplay, w, &root_return,
                                &parent_return, &children_return, &nchildren_return);

        if (!result)
        {
          #ifdef WARNING
          fprintf(stderr, "nxagentHandleReparentNotify: WARNING! Failed QueryTree request.\n");
          #endif

          break;
        }

        if (result && children_return)
        {
          XFree(children_return);
        }
      }

      if (w && !nxagentWindowPtr(w))
      {
        XSelectInput(nxagentDisplay, w, StructureNotifyMask);

        nxagentRootlessAddTopLevelWindow(pWin, w);

        #ifdef TEST
        fprintf(stderr, "nxagentHandleReparentNotify: new top level window [%ld].\n", w);
        fprintf(stderr, "nxagentHandleReparentNotify: reparented window [%ld].\n",
                    X -> xreparent.window);
        #endif

        result = XQueryTree(nxagentDisplay, DefaultRootWindow(nxagentDisplay),
                                &root_return, &parent_return, &children_return, &nchildren_return);

        if (result)
        {
          nxagentRootlessRestack(children_return, nchildren_return);
        }
        else
        {
          #ifdef WARNING
          fprintf(stderr, "nxagentHandleReparentNotify: WARNING! Failed QueryTree request.\n");
          #endif
        }

        if (result && nchildren_return)
        {
          XFree(children_return);
        }
      }
      else
      {
        #ifdef TEST
        fprintf(stderr, "nxagentHandleReparentNotify: Window at [%p] has been reparented to [%ld]"
                    " top level parent [%ld].\n", (void *) pWin, X -> xreparent.parent, w);
        #endif

        nxagentRootlessDelTopLevelWindow(pWin);
      }
    }

    return 1;
  }
  else if (nxagentWMIsRunning == 1 && nxagentOption(Fullscreen) == 0 &&
               nxagentOption(WMBorderWidth) == -1)
  {
    XlibWindow w;
    XlibWindow rootReturn = 0;
    XlibWindow parentReturn = 0;
    XlibWindow junk;
    XlibWindow *childrenReturn = NULL;
    unsigned int nchildrenReturn = 0;
    Status result;
    XWindowAttributes attributes;
    int x, y;
    int xParent, yParent;

    /*
     * Calculate the absolute upper-left X e Y 
     */

    if ((XGetWindowAttributes(nxagentDisplay, X -> xreparent.window,
                                  &attributes) == 0))
    {
      #ifdef WARNING
      fprintf(stderr, "nxagentHandleReparentNotify: WARNING! "
                  "XGetWindowAttributes failed.\n");
      #endif

      return 1;
    }

    x = attributes.x;
    y = attributes.y;

    XTranslateCoordinates(nxagentDisplay, X -> xreparent.window,
                              attributes.root, -attributes.border_width,
                                  -attributes.border_width, &x, &y, &junk);

   /*
    * Calculate the parent X and parent Y.
    */

    w = X -> xreparent.parent;

    if (w != DefaultRootWindow(nxagentDisplay))
    {
      do
      {
        result = XQueryTree(nxagentDisplay, w, &rootReturn, &parentReturn,
                                &childrenReturn, &nchildrenReturn);
    
        if (parentReturn == rootReturn || parentReturn == 0 || result == 0)
        {
          break;
        }

        if (result == 1 && childrenReturn != NULL)
        {
          XFree(childrenReturn);
        }
    
        w = parentReturn;
      }
      while (True);

      /*
       * WM reparented. Find edge of the frame.
       */

      if (XGetWindowAttributes(nxagentDisplay, w, &attributes) == 0)
      {
        #ifdef WARNING
        fprintf(stderr, "nxagentHandleReparentNotify: WARNING! "
                    "XGetWindowAttributes failed for parent window.\n");
        #endif

        return 1;
      }

      xParent = attributes.x;
      yParent = attributes.y;

      /*
       * Difference between Absolute X and Parent X gives thickness of side frame.
       * Difference between Absolute Y and Parent Y gives thickness of title bar. 
       */

      nxagentChangeOption(WMBorderWidth, (x - xParent));
      nxagentChangeOption(WMTitleHeight, (y - yParent));
    }
  }

  return 1;
}

void nxagentEnableKeyboardEvents()
{
  int i;
  Mask mask;

  nxagentGetDefaultEventMask(&mask);

  mask |= NXAGENT_KEYBOARD_EVENT_MASK;

  nxagentSetDefaultEventMask(mask);

  for (i = 0; i < nxagentNumScreens; i++)
  {
    XSelectInput(nxagentDisplay, nxagentDefaultWindows[i], mask);
  }

  XkbSelectEvents(nxagentDisplay, XkbUseCoreKbd,
                      NXAGENT_KEYBOARD_EXTENSION_EVENT_MASK,
                          NXAGENT_KEYBOARD_EXTENSION_EVENT_MASK);
}

void nxagentDisableKeyboardEvents()
{
  int i;
  Mask mask;

  nxagentGetDefaultEventMask(&mask);

  mask &= ~NXAGENT_KEYBOARD_EVENT_MASK;

  nxagentSetDefaultEventMask(mask);

  for (i = 0; i < nxagentNumScreens; i++)
  {
    XSelectInput(nxagentDisplay, nxagentDefaultWindows[i], mask);
  }

  XkbSelectEvents(nxagentDisplay, XkbUseCoreKbd, 0x0, 0x0);
}

void nxagentEnablePointerEvents()
{
  int i;
  Mask mask;

  nxagentGetDefaultEventMask(&mask);

  mask |= NXAGENT_POINTER_EVENT_MASK;

  nxagentSetDefaultEventMask(mask);

  for (i = 0; i < nxagentNumScreens; i++)
  {
    XSelectInput(nxagentDisplay, nxagentDefaultWindows[i], mask);
  }
}

void nxagentDisablePointerEvents()
{
  int i;
  Mask mask;

  nxagentGetDefaultEventMask(&mask);

  mask &= ~NXAGENT_POINTER_EVENT_MASK;

  nxagentSetDefaultEventMask(mask);

  for (i = 0; i < nxagentNumScreens; i++)
  {
    XSelectInput(nxagentDisplay, nxagentDefaultWindows[i], mask);
  }
}

void nxagentSendFakeKey(int key)
{
  xEvent fake;
  Time   now;

  now = GetTimeInMillis();

  fake.u.u.type = KeyPress;
  fake.u.u.detail = key;
  fake.u.keyButtonPointer.time = now;

  mieqEnqueue(&fake);

  fake.u.u.type = KeyRelease;
  fake.u.u.detail = key;
  fake.u.keyButtonPointer.time = now;

  mieqEnqueue(&fake);
}

int nxagentInitKeyboardState()
{
  XEvent X;

  unsigned int modifiers;

  XkbEvent *xkbev = (XkbEvent *) &X;

  #ifdef TEST
  fprintf(stderr, "nxagentInitKeyboardState: Initializing XKB state.\n");
  #endif

  XkbGetIndicatorState(nxagentDisplay, XkbUseCoreKbd, &modifiers);

  xkbev -> state.locked_mods = 0x0;

  if (modifiers & CAPSFLAG_IN_REPLY)
  {
    xkbev -> state.locked_mods |= CAPSFLAG_IN_EVENT;
  }

  if (modifiers & NUMFLAG_IN_REPLY)
  {
    xkbev -> state.locked_mods |= NUMFLAG_IN_EVENT;
  }

  #ifdef TEST
  fprintf(stderr, "nxagentInitKeyboardState: Assuming XKB locked modifier bits [%x].\n",
              xkbev -> state.locked_mods);
  #endif

  xkbev -> type         = nxagentXkbInfo.EventBase + XkbEventCode;
  xkbev -> any.xkb_type = XkbStateNotify;

  nxagentHandleKeyboardEvent(&X);

  return 1;
}

int nxagentWaitForResource(GetResourceFuncPtr pGetResource, PredicateFuncPtr pPredicate)
{
  int resource;

  while ((resource = (*pGetResource)(nxagentDisplay)) == -1)
  {
    if (nxagentWaitEvents(nxagentDisplay, NULL) == -1)
    {
      return -1;
    }

    nxagentDispatchEvents(pPredicate);
  }

  return resource;
}

void nxagentGrabPointerAndKeyboard(XEvent *X)
{
  unsigned long now;

  int resource;

  int result;

  #ifdef TEST
  fprintf(stderr, "nxagentGrabPointerAndKeyboard: Grabbing pointer and keyboard with event at [%p].\n",
              (void *) X);
  #endif

  if (X != NULL)
  {
    now = X -> xcrossing.time;
  }
  else
  {
    now = CurrentTime;
  }

  #ifdef TEST
  fprintf(stderr, "nxagentGrabPointerAndKeyboard: Going to grab the keyboard in context [B1].\n");
  #endif

  result = XGrabKeyboard(nxagentDisplay, nxagentFullscreenWindow,
                             True, GrabModeAsync, GrabModeAsync, now);

  if (result != GrabSuccess)
  {
    return;
  }

  /*
   * The smart scheduler could be stopped while
   * waiting for the reply. In this case we need
   * to yield explicitly to avoid to be stuck in
   * the dispatch loop forever.
   */

  isItTimeToYield = 1;

  #ifdef TEST
  fprintf(stderr, "nxagentGrabPointerAndKeyboard: Going to grab the pointer in context [B2].\n");
  #endif

  resource = nxagentWaitForResource(NXGetCollectGrabPointerResource,
                                        nxagentCollectGrabPointerPredicate);

  NXCollectGrabPointer(nxagentDisplay, resource,
                           nxagentFullscreenWindow, True, NXAGENT_POINTER_EVENT_MASK,
                               GrabModeAsync, GrabModeAsync, None, None, now);

  /*
   * This should not be needed.
   *
   * XGrabKey(nxagentDisplay, AnyKey, AnyModifier, nxagentFullscreenWindow,
   *              True, GrabModeAsync, GrabModeAsync);
   */

  if (X != NULL)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentGrabPointerAndKeyboard: Going to force focus in context [B4].\n");
    #endif

    XSetInputFocus(nxagentDisplay, nxagentFullscreenWindow,
                       RevertToParent, now);
  }
}

void nxagentUngrabPointerAndKeyboard(XEvent *X)
{
  unsigned long now;

  #ifdef TEST
  fprintf(stderr, "nxagentUngrabPointerAndKeyboard: Ungrabbing pointer and keyboard with event at [%p].\n",
              (void *) X);
  #endif

  if (X != NULL)
  {
    now = X -> xcrossing.time;
  }
  else
  {
    now = CurrentTime;
  }

  #ifdef TEST
  fprintf(stderr, "nxagentUngrabPointerAndKeyboard: Going to ungrab the keyboard in context [B5].\n");
  #endif

  XUngrabKeyboard(nxagentDisplay, now);

  #ifdef TEST
  fprintf(stderr, "nxagentGrabPointerAndKeyboard: Going to ungrab the pointer in context [B6].\n");
  #endif

  XUngrabPointer(nxagentDisplay, now);
}

void nxagentDeactivatePointerGrab()
{
  GrabPtr grab = inputInfo.pointer -> grab;

  XButtonEvent X;

  if (grab)
  {
    X.type = ButtonRelease;
    X.serial = 0;
    X.send_event = FALSE;
    X.time = currentTime.milliseconds;
    X.display = nxagentDisplay;
    X.window = nxagentWindow(grab -> window);
    X.root = RootWindow(nxagentDisplay, 0);
    X.subwindow = 0;
    X.x = X.y = X.x_root = X.y_root = 0;
    X.state = 0x100;
    X.button = 1;
    X.same_screen = TRUE;

    XPutBackEvent(nxagentDisplay, (XEvent*)&X);
  }
}

Bool nxagentCollectGrabPointerPredicate(Display *display, XEvent *X, XPointer ptr)
{
  return (X -> xclient.window == 0 &&
             X -> xclient.message_type == 0 &&
                 X -> xclient.format == 32 &&
                     X -> xclient.data.l[0] == NXCollectGrabPointerNotify);
}

void nxagentHandleCollectGrabPointerEvent(int resource)
{
  int status;

  if (NXGetCollectedGrabPointer(nxagentDisplay, resource, &status) == 0)
  {
    #ifdef PANIC
    fprintf(stderr, "nxagentHandleCollectGrabPointerEvent: PANIC! Failed to get GrabPointer "
                "reply for resource [%d].\n", resource);
    #endif
  }
}

void nxagentHandleCollectPropertyEvent(XEvent *X)
{
  Window window;
  Atom property;
  Atom atomReturnType;
  int resultFormat;
  unsigned long ulReturnItems;
  unsigned long ulReturnBytesLeft;
  unsigned char *pszReturnData = NULL;
  int result;
  int resource;

  resource = X -> xclient.data.l[1];

  if (X -> xclient.data.l[2] == False)
  {
    #ifdef DEBUG
    fprintf (stderr, "nxagentHandleCollectPropertyEvent: Failed to get reply data for client [%d].\n",
                 resource);
    #endif

    return;
  }

  if (resource == nxagentLastClipboardClient)
  {
    nxagentCollectPropertyEvent(resource);
  }
  else
  {
    result = NXGetCollectedProperty(nxagentDisplay,
                                    resource,
                                    &atomReturnType,
                                    &resultFormat,
                                    &ulReturnItems,
                                    &ulReturnBytesLeft,
                                    &pszReturnData);

    if (result == True)
    {
      window = nxagentPropertyRequests[resource].window;
      property = nxagentPropertyRequests[resource].property;

      nxagentImportProperty(window, property, atomReturnType, resultFormat,
                                ulReturnItems, ulReturnBytesLeft, pszReturnData);
    }

    if (result == 0)
    {
      #ifdef DEBUG
      fprintf (stderr, "nxagentHandleCollectPropertyEvent: Failed to get reply data for client [%d].\n",
                   resource);
      #endif
    }

    if (pszReturnData != NULL)
    {
      XFree(pszReturnData);
    }

    return;
  }
}

void nxagentSynchronizeExpose(void)
{
  WindowPtr pWin;

  if (nxagentExposeQueue.length <= 0)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentSynchronizeExpose: PANIC! Called with nxagentExposeQueue.length [%d].\n",
                nxagentExposeQueue.length);
    #endif

    return;
  }

  pWin = nxagentExposeQueueHead.pWindow;

  if (pWin)
  {
    if ((nxagentExposeQueueHead.localRegion) != NullRegion)
    {
      REGION_TRANSLATE(pWin -> drawable.pScreen, (nxagentExposeQueueHead.localRegion),
                           pWin -> drawable.x, pWin -> drawable.y);
    }

    if ((nxagentExposeQueueHead.remoteRegion) != NullRegion)
    {
      REGION_TRANSLATE(pWin -> drawable.pScreen, (nxagentExposeQueueHead.remoteRegion),
                           pWin -> drawable.x, pWin -> drawable.y);
    }

    if ((nxagentExposeQueueHead.localRegion) != NullRegion &&
             (nxagentExposeQueueHead.remoteRegion) != NullRegion)
    {
      REGION_SUBTRACT(pWin -> drawable.pScreen, (nxagentExposeQueueHead.remoteRegion),
                          (nxagentExposeQueueHead.remoteRegion),
                              (nxagentExposeQueueHead.localRegion));

      if (REGION_NIL(nxagentExposeQueueHead.remoteRegion) == 0 &&
             ((pWin -> eventMask|wOtherEventMasks(pWin)) & ExposureMask))
      {
        #ifdef TEST
        fprintf(stderr, "nxagentSynchronizeExpose: Going to call miWindowExposures"
                    " for window [%ld] - rects [%ld].\n", nxagentWindow(pWin),
                        REGION_NUM_RECTS(nxagentExposeQueueHead.remoteRegion));
        #endif

        miWindowExposures(pWin, nxagentExposeQueueHead.remoteRegion, NullRegion);
      }
    }
  }

  nxagentExposeQueueHead.pWindow = NULL;

  if (nxagentExposeQueueHead.localRegion != NullRegion)
  {
    REGION_DESTROY(nxagentDefaultScreen, nxagentExposeQueueHead.localRegion);
  }

  nxagentExposeQueueHead.localRegion = NullRegion;

  if (nxagentExposeQueueHead.remoteRegion != NullRegion)
  {
    REGION_DESTROY(nxagentDefaultScreen, nxagentExposeQueueHead.remoteRegion);
  }

  nxagentExposeQueueHead.remoteRegion = NullRegion;

  nxagentExposeQueueHead.remoteRegionIsCompleted = False;

  nxagentExposeQueue.start = (nxagentExposeQueue.start + 1) % EXPOSED_SIZE;

  nxagentExposeQueue.length--;

  return;
}

int nxagentLookupByWindow(WindowPtr pWin)
{
  int i;
  int j;

  for (j = 0; j < nxagentExposeQueue.length; j++)
  {
    i = (nxagentExposeQueue.start + j) % EXPOSED_SIZE;

    if (nxagentExposeQueue.exposures[i].pWindow == pWin &&
            !nxagentExposeQueue.exposures[i].remoteRegionIsCompleted)
    {
      return i; 
    }
  }

  return -1;
}

void nxagentRemoveDuplicatedKeys(XEvent *X)
{
  _XQEvent *prev;
  _XQEvent *qelt;

  _XQEvent *qeltKeyRelease;
  _XQEvent *prevKeyRelease;

  KeyCode lastKeycode = X -> xkey.keycode;

  qelt = nxagentDisplay -> head;

  if (qelt == NULL)
  {
    #ifdef TEST

    int more;

    fprintf(stderr, "nxagentRemoveDuplicatedKeys: Trying to read more events "
                "from the X server.\n");

    more = nxagentReadEvents(nxagentDisplay);

    if (more > 0)
    {
      fprintf(stderr, "nxagentRemoveDuplicatedKeys: Successfully read more events "
                  "from the X server.\n");
    }

    #else

    nxagentReadEvents(nxagentDisplay);

    #endif

    qelt = nxagentDisplay -> head;
  }

  if (qelt != NULL)
  {
    prev = qeltKeyRelease = prevKeyRelease = NULL;

    LockDisplay(nxagentDisplay);

    while (qelt != NULL)
    {
      if (qelt -> event.type == KeyRelease ||
              qelt -> event.type == KeyPress)
      {
        if (qelt -> event.xkey.keycode != lastKeycode ||
               (qelt -> event.type == KeyPress && qeltKeyRelease == NULL) ||
                   (qelt -> event.type == KeyRelease && qeltKeyRelease != NULL))
        {
          break;
        }

        if (qelt -> event.type == KeyRelease)
        {
          prevKeyRelease = prev;

          qeltKeyRelease = qelt;
        }
        else if (qelt -> event.type == KeyPress)
        {
          _XDeq(nxagentDisplay, prev, qelt);

          qelt = prev -> next;

          if (prev == qeltKeyRelease)
          {
            prev = prevKeyRelease;
          }

          _XDeq(nxagentDisplay, prevKeyRelease, qeltKeyRelease);

          qeltKeyRelease = prevKeyRelease = NULL;

          continue;
        }
      }

      prev = qelt;

      qelt = qelt -> next;
    }

    UnlockDisplay(nxagentDisplay);
  }
}

void nxagentInitRemoteExposeRegion(void)
{
  if (nxagentRemoteExposeRegion == NULL)
  {
    nxagentRemoteExposeRegion = REGION_CREATE(pWin -> drawable.pScreen, NULL, 1);

    if (nxagentRemoteExposeRegion == NULL)
    {
      #ifdef PANIC
      fprintf(stderr, "nxagentInitRemoteExposeRegion: PANIC! Failed to create expose region.\n");
      #endif
    }
  }
}

void nxagentForwardRemoteExpose(void)
{
  if (REGION_NOTEMPTY(WindowTable[0] -> drawable.pScreen, nxagentRemoteExposeRegion))
  {
    #ifdef DEBUG
    fprintf(stderr, "nxagentForwardRemoteExpose: Going to forward events.\n");
    #endif

    TraverseTree(WindowTable[0], nxagentClipAndSendExpose, (void *)nxagentRemoteExposeRegion);

    /*
     * Now this region should be empty.
     */

    REGION_EMPTY(WindowTable[0] -> drawable.pScreen, nxagentRemoteExposeRegion);
  }
}

void nxagentAddRectToRemoteExposeRegion(BoxPtr rect)
{
  RegionRec exposeRegion;

  if (nxagentRemoteExposeRegion == NULL)
  {
    return;
  }

  REGION_INIT(nxagentDefaultScreen, &exposeRegion, rect, 1);

  REGION_UNION(nxagentDefaultScreen, nxagentRemoteExposeRegion,
                   nxagentRemoteExposeRegion, &exposeRegion);

  REGION_UNINIT(nxagentDefaultScreen, &exposeRegion);
}

int nxagentClipAndSendExpose(WindowPtr pWin, pointer ptr)
{
  RegionPtr exposeRgn;
  RegionPtr remoteExposeRgn;
  BoxRec box;

  #ifdef DEBUG
  fprintf(stderr, "nxagentClipAndSendExpose: Called.\n");
  #endif

  remoteExposeRgn = (RegionRec *) ptr;

  if (pWin -> drawable.class != InputOnly)
  {
    exposeRgn = REGION_CREATE(pWin -> drawable.pScreen, NULL, 1);

    box = *REGION_EXTENTS(pWin->drawable.pScreen, remoteExposeRgn);

    #ifdef DEBUG
    fprintf(stderr, "nxagentClipAndSendExpose: Root expose extents: [%d] [%d] [%d] [%d].\n",
                box.x1, box.y1, box.x2, box.y2);
    #endif

    box = *REGION_EXTENTS(pWin->drawable.pScreen, &pWin -> clipList);

    #ifdef DEBUG
    fprintf(stderr, "nxagentClipAndSendExpose: Clip list extents for window at [%p]: [%d] [%d] [%d] [%d].\n",
                pWin, box.x1, box.y1, box.x2, box.y2);
    #endif

    REGION_INTERSECT(pWin -> drawable.pScreen, exposeRgn, remoteExposeRgn, &pWin -> clipList);

    if (REGION_NOTEMPTY(pWin -> drawable.pScreen, exposeRgn))
    {
      #ifdef DEBUG
      fprintf(stderr, "nxagentClipAndSendExpose: Forwarding expose to window at [%p] pWin.\n",
                  pWin);
      #endif

      /*
       * The miWindowExposures() clears out the
       * region parameters, so the subtract ope-
       * ration must be done before calling it.
       */

      REGION_SUBTRACT(pWin -> drawable.pScreen, remoteExposeRgn, remoteExposeRgn, exposeRgn);

      miWindowExposures(pWin, exposeRgn, NullRegion);
    }

    REGION_DESTROY(pWin -> drawable.pScreen, exposeRgn);
  }

  if (REGION_NOTEMPTY(pWin -> drawable.pScreen, remoteExposeRgn))
  {
    #ifdef DEBUG
    fprintf(stderr, "nxagentClipAndSendExpose: Region not empty. Walk children.\n");
    #endif

    return WT_WALKCHILDREN;
  }
  else
  {
    #ifdef DEBUG
    fprintf(stderr, "nxagentClipAndSendExpose: Region empty. Stop walking.\n");
    #endif

    return WT_STOPWALKING;
  }
}

int nxagentUserInput(void *p)
{
  int result = 0;

  /*
   * This function is used as callback in
   * the polling handler of agent in shadow
   * mode. When inside the polling loop the
   * handlers are never called, so we have
   * to dispatch enqueued events to eventu-
   * ally change the nxagentInputEvent sta-
   * tus.
   */

  if (nxagentOption(Shadow) == 1 &&
          nxagentPendingEvents(nxagentDisplay) > 0)
  {
    nxagentDispatchEvents(NULL);
  }

  if (nxagentInputEvent == 1)
  {
    nxagentInputEvent = 0;

    result = 1;
  }

  /*
   * The agent working in shadow mode synch-
   * ronizes the remote X server even if a
   * button/key is not released (i.e. when
   * scrolling a long browser's page), in
   * order to update the screen smoothly.
   */

  if (nxagentOption(Shadow) == 1)
  {
    return result;
  }

  if (result == 0)
  {
    /*
     * If there is at least one button/key down,
     * we are receiving an input. This is not a
     * condition to break a synchronization loop
     * if there is enough bandwidth.
     */

    if (nxagentCongestion > 0 &&
            (inputInfo.pointer -> button -> buttonsDown > 0 ||
                nxagentKeyDown > 0))
    {
      #ifdef TEST
      fprintf(stderr, "nxagentUserInput: Buttons [%d] Keys [%d].\n",
                  inputInfo.pointer -> button -> buttonsDown, nxagentKeyDown);
      #endif

      result = 1;
    }
  }

  return result;
}

int nxagentHandleRRScreenChangeNotify(XEvent *X)
{
  XRRScreenChangeNotifyEvent *Xr;

  Xr = (XRRScreenChangeNotifyEvent *) X;

  nxagentResizeScreen(screenInfo.screens[DefaultScreen(nxagentDisplay)], Xr -> width, Xr -> height,
                          Xr -> mwidth, Xr -> mheight);

  nxagentShadowCreateMainWindow(screenInfo.screens[DefaultScreen(nxagentDisplay)], WindowTable[0],
                                Xr -> width, Xr -> height);

  nxagentShadowSetWindowsSize();

  return 1;
}

/*
 * Returns true if there is any event waiting to
 * be dispatched. This function is critical for
 * the performance because it is called very,
 * very often. It must also handle the case when
 * the display is down. The display descriptor,
 * in fact, may have been reused by some other
 * client.
 */

int nxagentPendingEvents(Display *dpy)
{
  if (_XGetIOError(dpy) != 0)
  {
    #ifdef DEBUG
    fprintf(stderr, "nxagentPendingEvents: Returning error with display down.\n");
    #endif

    return -1;
  }
  else if (XQLength(dpy) > 0)
  {
    #ifdef DEBUG
    fprintf(stderr, "nxagentPendingEvents: Returning true with [%d] events queued.\n",
                XQLength(dpy));
    #endif

    return 1;
  }
  else
  {
    int result;
    int readable;

    result = NXTransReadable(dpy -> fd, &readable);

    if (result == 0)
    {
      if (readable > 0)
      {
        #ifdef DEBUG
        fprintf(stderr, "nxagentPendingEvents: Returning true with [%d] bytes readable.\n",
                    readable);
        #endif

        return 1;
      }

      #ifdef DEBUG
      fprintf(stderr, "nxagentPendingEvents: Returning false with [%d] bytes readable.\n",
                  readable);
      #endif

      return 0;
    }

    #ifdef TEST
    fprintf(stderr, "nxagentPendingEvents: WARNING! Error detected on the X display.\n");
    #endif

    NXForceDisplayError(dpy);

    return -1;
  }
}

/*
 * Blocks until an event becomes
 * available.
 */

int nxagentWaitEvents(Display *dpy, struct timeval *tm)
{
  XEvent ev;

  NXFlushDisplay(dpy, NXFlushLink);

  /*
   * If the transport is not running we
   * have to rely on Xlib to wait for an
   * event. In this case the timeout is
   * ignored.
   */

  if (NXTransRunning(NX_FD_ANY) == 1)
  {
    NXTransContinue(tm);
  }
  else
  {
    XPeekEvent(dpy, &ev);
  }

  /*
   * Check if we encountered a display
   * error. If we did, wait for the
   * time requested by the caller.
   */

  if (NXDisplayError(dpy) == 1)
  {
    if (tm != NULL)
    {
      usleep(tm -> tv_sec * 1000 * 1000 +
                 tm -> tv_usec);
    }

    return -1;
  }

  return 1;
}

#ifdef NX_DEBUG_INPUT

void nxagentDumpInputInfo(void)
{
  fprintf(stderr, "Dumping input info ON.\n");
}

void nxagentGuessDumpInputInfo(ClientPtr client, Atom property, char *data)
{
  if (strcmp(validateString(NameForAtom(property)), "NX_DEBUG_INPUT") == 0)
  {
    if (*data != 0)
    {
      nxagentDebugInput = 1;
    }
    else
    {
      nxagentDebugInput = 0;
    }
  }
}

void nxagentDeactivateInputDevicesGrabs()
{
  fprintf(stderr, "Info: Deactivating input devices grabs.\n");

  if (inputInfo.pointer -> grab)
  {
    (*inputInfo.pointer -> DeactivateGrab)(inputInfo.pointer);
  }

  if (inputInfo.keyboard -> grab)
  {
    (*inputInfo.keyboard -> DeactivateGrab)(inputInfo.keyboard);
  }
}

static const char *nxagentGrabStateToString(int state)
{
  switch (state)
  {
    case 0:
      return "NOT_GRABBED";
    case 1:
      return "THAWED";
    case 2:
      return "THAWED_BOTH";
    case 3:
      return "FREEZE_NEXT_EVENT";
    case 4:
      return "FREEZE_BOTH_NEXT_EVENT";
    case 5:
      return "FROZEN_NO_EVENT";
    case 6:
      return "FROZEN_WITH_EVENT";
    case 7:
      return "THAW_OTHERS";
    default:
      return "unknown state";
  }
}

void nxagentDumpInputDevicesState(void)
{
  int i, k;
  int mask = 1;
  CARD8 val;
  DeviceIntPtr dev;
  GrabPtr grab;
  WindowPtr pWin = NULL;

  fprintf(stderr, "\n*** Dump input devices state: BEGIN ***"
              "\nKeys down:");

  dev = inputInfo.keyboard;

  for (i = 0; i < DOWN_LENGTH; i++)
  {
    val = dev -> key -> down[i];

    if (val != 0)
    {
      for (k = 0; k < 8; k++)
      {
        if (val & (mask << k))
        {
          fprintf(stderr, "\n\t[%d] [%s]", i * 8 + k,
                      XKeysymToString(XKeycodeToKeysym(nxagentDisplay, i * 8 + k, 0)));
        }
      }
    }
  }

  fprintf(stderr, "\nKeyboard device state: \n\tdevice [%p]\n\tlast grab time [%lu]"
              "\n\tfrozen [%s]\n\tstate [%s]\n\tother [%p]\n\tevent count [%d]"
                  "\n\tfrom passive grab [%s]\n\tactivating key [%d]", dev,
                      dev -> grabTime.milliseconds, dev -> sync.frozen ? "Yes": "No",
                          nxagentGrabStateToString(dev -> sync.state),
                              dev -> sync.other, dev -> sync.evcount,
                                  dev -> fromPassiveGrab ? "Yes" : "No",
                                      dev -> activatingKey);

  grab = dev -> grab;

  if (grab)
  {
    fprintf(stderr, "\nKeyboard grab state: \n\twindow pointer [%p]"
                "\n\towner events flag [%s]\n\tgrab mode [%s]",
                    grab -> window, grab -> ownerEvents ? "True" : "False",
                        grab -> keyboardMode ? "asynchronous" : "synchronous");

   /*
    * Passive grabs.
    */

    pWin = grab -> window;
    grab = wPassiveGrabs(pWin);

    while (grab)
    {
      fprintf(stderr, "\nPassive grab state: \n\tdevice [%p]\n\towner events flag [%s]"
                  "\n\tpointer grab mode [%s]\n\tkeyboard grab mode [%s]\n\tevent type [%d]"
                      "\n\tmodifiers [%x]\n\tbutton/key [%u]\n\tevent mask [%lx]",
                          grab -> device, grab -> ownerEvents ? "True" : "False",
                              grab -> pointerMode ? "asynchronous" : "synchronous",
                                  grab -> keyboardMode ? "asynchronous" : "synchronous",
                                      grab -> type, grab -> modifiersDetail.exact,
                                          grab -> detail.exact, grab -> eventMask);

      grab = grab -> next;
    }
  }

  fprintf(stderr, "\nButtons down:");

  dev = inputInfo.pointer;

  for (i = 0; i < DOWN_LENGTH; i++)
  {
    val = dev -> button -> down[i];

    if (val != 0)
    {
      for (k = 0; k < 8; k++)
      {
        if (val & (mask << k))
        {
          fprintf(stderr, "\n\t[%d]", i * 8 + k);
        }
      }
    }
  }

  fprintf(stderr, "\nPointer device state: \n\tdevice [%p]\n\tlast grab time [%lu]"
              "\n\tfrozen [%s]\n\tstate [%s]\n\tother [%p]\n\tevent count [%d]"
                  "\n\tfrom passive grab [%s]\n\tactivating button [%d]", dev,
                      dev -> grabTime.milliseconds, dev -> sync.frozen ? "Yes" : "No",
                          nxagentGrabStateToString(dev -> sync.state),
                              dev -> sync.other, dev -> sync.evcount,
                                  dev -> fromPassiveGrab ? "Yes" : "No",
                                      dev -> activatingKey);

  grab = dev -> grab;

  if (grab)
  {
    fprintf(stderr, "\nPointer grab state: \n\twindow pointer [%p]"
                "\n\towner events flag [%s]\n\tgrab mode [%s]",
                    grab -> window, grab -> ownerEvents ? "True" : "False",
                        grab -> pointerMode ? "asynchronous" : "synchronous");

    if (grab -> window != pWin)
    {
      /*
       * Passive grabs.
       */

      grab = wPassiveGrabs(grab -> window);

      while (grab)
      {
        fprintf(stderr, "\nPassive grab state: \n\tdevice [%p]\n\towner events flag [%s]"
                    "\n\tpointer grab mode [%s]\n\tkeyboard grab mode [%s]\n\tevent type [%d]"
                        "\n\tmodifiers [%x]\n\tbutton/key [%u]\n\tevent mask [%lx]",
                            grab -> device, grab -> ownerEvents ? "True" : "False",
                                grab -> pointerMode ? "asynchronous" : "synchronous",
                                    grab -> keyboardMode ? "asynchronous" : "synchronous",
                                        grab -> type, grab -> modifiersDetail.exact,
                                            grab -> detail.exact, grab -> eventMask);

        grab = grab -> next;
      }
    }
  }

  fprintf(stderr, "\n*** Dump input devices state: FINISH ***\n");
}

#endif
