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

#ifndef __Events_H__
#define __Events_H__

#include <X11/Xmd.h>

#define ProcessedExpose (LASTEvent + 1)
#define ProcessedNotify (LASTEvent + 2)

#define EXPOSED_SIZE 256

enum HandleEventResult
{
  doNothing = 0,
  doMinimize,
  doDebugTree,
  doCloseSession,
  doStartKbd,
  doSwitchFullscreen,
  doSwitchAllScreens,
  doViewportMoveUp,
  doViewportMoveLeft,
  doViewportMoveRight,
  doViewportMoveDown,
  doViewportLeft,
  doViewportUp,
  doViewportRight,
  doViewportDown,
  doSwitchResizeMode,
  doSwitchDeferMode
};

extern CARD32 nxagentLastEventTime;

/*
 * Manage incoming events.
 */

typedef Bool (*PredicateFuncPtr)(Display*, XEvent*, XPointer);

extern void nxagentDispatchEvents(PredicateFuncPtr);

typedef int (*GetResourceFuncPtr)(Display*);

int nxagentWaitForResource(GetResourceFuncPtr, PredicateFuncPtr);

Bool nxagentCollectGrabPointerPredicate(Display *display, XEvent *X, XPointer ptr);

int nxagentInputEventPredicate(Display *display, XEvent *event, XPointer parameter);

/*
 * Enable and disable notification of
 * remote X server events.
 */

extern void nxagentEnableKeyboardEvents(void);
extern void nxagentEnablePointerEvents(void);

extern void nxagentDisableKeyboardEvents(void);
extern void nxagentDisablePointerEvents(void);

/*
 * Manage default event mask.
 */

extern void nxagentInitDefaultEventMask(void);
extern void nxagentGetDefaultEventMask(Mask *mask_return);
extern void nxagentSetDefaultEventMask(Mask mask);
extern void nxagentGetEventMask(WindowPtr pWin, Mask *mask_return);

/*
 * Bring keyboard device in known state. It needs
 * a round-trip so it only gets called if a pre-
 * vious XKB event did not implicitly initialized
 * the internal state. This is unlikely to happen.
 */

extern int nxagentInitKeyboardState(void);

/*
 * Update the keyboard state according
 * to focus and XKB events received
 * from the remote X server.
 */

extern int nxagentHandleKeyboardEvent(XEvent *X);

/*
 * Handle sync and karma messages and
 * other notification event coming
 * from proxy.
 */

extern int nxagentHandleProxyEvent(XEvent *X);

/*
 * Other functions providing the ad-hoc
 * handling of the remote X events.
 */

extern int nxagentHandleExposeEvent(XEvent *X);
extern int nxagentHandleGraphicsExposeEvent(XEvent *X);
extern int nxagentHandleClientMessageEvent(XEvent *X, enum HandleEventResult*);
extern int nxagentHandlePropertyNotify(XEvent *X);
extern int nxagentHandleKeyPress(XEvent *X, enum HandleEventResult*);
extern int nxagentHandleReparentNotify(XEvent *X);
extern int nxagentHandleConfigureNotify(XEvent *X);
extern int nxagentHandleXFixesSelectionNotify(XEvent *X);

/*
 * Send a fake keystroke to the remote
 * X server.
 */

extern void nxagentSendFakeKey(int key);

/*
 * Called to manage grab of pointer and
 * keyboard when running in fullscreen
 * mode.
 */

extern void nxagentGrabPointerAndKeyboard(XEvent *X);
extern void nxagentUngrabPointerAndKeyboard(XEvent *X);

extern void nxagentDeactivatePointerGrab(void);

/*
 * Handle the selection property received
 * in the event loop.
 */

void nxagentCollectPropertyEvent(int resource);

/*
 * Synchronize expose events between agent and
 * the real X server.
 */

typedef struct _ExposuresRec
{
  WindowPtr pWindow;
  RegionPtr localRegion;
  RegionPtr remoteRegion;
  Bool remoteRegionIsCompleted;
  int serial;
  int synchronize;

} ExposuresRec;

extern RegionPtr nxagentRemoteExposeRegion;

typedef struct _ExposeQueue
{
  unsigned int start;
  int length;
  ExposuresRec exposures[EXPOSED_SIZE];
} ExposeQueue;

extern void nxagentSynchronizeExpose(void);
extern int nxagentLookupByWindow(WindowPtr pWin);
extern void nxagentUpdateExposeArray(void);

extern ExposeQueue nxagentExposeQueue;

/*
 * Handle the split related notifications.
 */

int nxagentWaitSplitEvent(int resource);

void nxagentHandleNoSplitEvent(int resource);
void nxagentHandleStartSplitEvent(int resource);
void nxagentHandleCommitSplitEvent(int resource, int request, int position);
void nxagentHandleEndSplitEvent(int resource);
void nxagentHandleEmptySplitEvent(void);

void nxagentInitRemoteExposeRegion(void);
void nxagentAddRectToRemoteExposeRegion(BoxPtr);

extern int nxagentUserInput(void *p);

/*
* We have to check these before launching the terminate
 * dialog in rootless mode.
 */

extern Bool nxagentLastWindowDestroyed;
extern Time nxagentLastWindowDestroyedTime;

/*
 * Set this flag if an user input event is received.
 */

extern int nxagentInputEvent;

/*
 * Event-handling utilities.
 */

Bool nxagentPendingEvents(Display *dpy);

#define nxagentQueuedEvents(display) \
    XQLength((display))

#define nxagentReadEvents(display) \
    XEventsQueued((display), QueuedAfterReading)

#define nxagentCheckEvents(display, event, predicate, argument) \
    XCheckIfEventNoFlush((display), (event), (predicate), (argument)) 

int nxagentWaitEvents(Display *, struct timeval *);

#endif /* __Events_H__ */
