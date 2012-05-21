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

#ifndef __Display_H__
#define __Display_H__

#define MAXDEPTH 32
#define MAXVISUALSPERDEPTH 256

extern Display *nxagentDisplay;
extern Display *nxagentShadowDisplay;
extern XVisualInfo *nxagentVisuals;
extern int nxagentNumVisuals;
extern int nxagentDefaultVisualIndex;
extern Colormap *nxagentDefaultColormaps;
extern int nxagentNumDefaultColormaps;
extern int *nxagentDepths;
extern int nxagentNumDepths;
extern XPixmapFormatValues *nxagentPixmapFormats;
extern int nxagentNumPixmapFormats;
extern Pixel nxagentBlackPixel;
extern Pixel nxagentWhitePixel;
extern Drawable nxagentDefaultDrawables[MAXDEPTH + 1];
extern Pixmap nxagentScreenSaverPixmap;

/*
 * The "confine" window is used in nxagentConstrainCursor().
 * We are currently overriding the original Xnest behaviour
 * and just skip the "constrain" stuff.
 */

extern Window nxagentConfineWindow;

/*
 * Keyboard and pointer are handled as they were hardware
 * devices, that is we translate the key codes according to
 * our own transcripts. We inherit this behaviour from Xnest.
 * The following mask will contain the event mask selected
 * for the root window. All the keyboard and pointer events
 * are enqueued to the mi that translates and posts them to
 * managed clients.
 */

extern unsigned long nxagentEventMask;

void nxagentOpenDisplay(int argc, char *argv[]);
void nxagentWaitDisplay(void);
void nxagentCloseDisplay(void);
void nxagentAbortDisplay(void);

void nxagentAddXConnection(void);
void nxagentRemoveXConnection(void);

Bool nxagentXServerGeometryChanged(void);

/*
 * Create the default drawables.
 */

void nxagentGetDepthsAndPixmapFormats(void);
void nxagentSetDefaultDrawables(void);

extern Bool nxagentTrue24;

void nxagentBackupDisplayInfo(void);
void nxagentCleanupBackupDisplayInfo(void);

void nxagentInstallDisplayHandlers(void);
void nxagentPostInstallDisplayHandlers(void);
void nxagentResetDisplayHandlers(void);

void nxagentInstallSignalHandlers(void);
void nxagentPostInstallSignalHandlers(void);
void nxagentResetSignalHandlers(void);

void nxagentDisconnectDisplay(void);
Bool nxagentReconnectDisplay(void *p0);

/*
 * Deal with the smart scheduler.
 */

#ifdef SMART_SCHEDULE

#define nxagentInitTimer() \
\
    SmartScheduleInit();

#define nxagentStopTimer() \
\
    if (SmartScheduleTimerStopped == 0) \
    { \
      SmartScheduleStopTimer(); \
    } \
\
    SmartScheduleIdle = 1;

#define nxagentStartTimer() \
\
    if (SmartScheduleTimerStopped == 1) \
    { \
      SmartScheduleStartTimer(); \
    } \
\
      SmartScheduleIdle = 0;

#define nxagentDisableTimer() \
\
    if (SmartScheduleTimerStopped == 0) \
    { \
      SmartScheduleStopTimer(); \
    } \
\
    SmartScheduleDisable = 1;

#endif /* #ifdef SMART_SCHEDULE */

/*
 * File descriptor currently used by
 * Xlib for the agent display.
 */

extern int nxagentXConnectionNumber;

/*
 * File descriptor currently used by
 * Xlib for the agent shadow display.
 */

extern int nxagentShadowXConnectionNumber;

int nxagentServerOrder(void);

int nxagentGetDataRate(void);

#define nxagentClientOrder(client) \
    ((client)->swapped ? !nxagentServerOrder() : nxagentServerOrder())

/*
 * Terminate the agent after the next
 * dispatch loop.
 */

#define nxagentTerminateSession() \
    do \
    { \
      dispatchException |= DE_TERMINATE; \
    \
      isItTimeToYield = TRUE; \
    } \
    while (0)

#endif /* __Display_H__ */
