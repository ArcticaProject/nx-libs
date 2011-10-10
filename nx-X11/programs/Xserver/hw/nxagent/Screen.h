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

#ifndef __Screen_H__
#define __Screen_H__

#define MIN_NXAGENT_WIDTH  80
#define MIN_NXAGENT_HEIGHT 60
#define NXAGENT_FRAME_WIDTH 2000

#define nxagentSetPrintGeometry(screen) \
    nxagentPrintGeometryFlags = (1 << (screen));
    
extern int nxagentClients;

extern int nxagentAutoDisconnectTimeout;

extern ScreenPtr nxagentDefaultScreen;

extern Pixmap nxagentPixmapLogo;

extern Window nxagentIconWindow;

extern Window nxagentFullscreenWindow;

extern RegionRec nxagentShadowUpdateRegion;

extern WindowPtr nxagentShadowWindowPtr;

extern int nxagentShadowResize;

extern short nxagentShadowUid;

void nxagentSetScreenInfo(ScreenInfo *screenInfo);
void nxagentSetPixmapFormats(ScreenInfo *screenInfo);

void nxagentPrintGeometry();

extern Window nxagentDefaultWindows[MAXSCREENS];
extern Window nxagentInputWindows[MAXSCREENS];
extern Window nxagentScreenSaverWindows[MAXSCREENS];

#ifdef VIEWPORT_FRAME

void nxagentInitViewportFrame(ScreenPtr pScreen, WindowPtr pRootWin);

#else /* #ifdef VIEWPORT_FRAME */

#define nxagentInitViewportFrame(pScreen, pRootWin)

#endif /* #ifdef VIEWPORT_FRAME */

Bool nxagentOpenScreen(int index, ScreenPtr pScreen,
                           int argc, char *argv[]);

Bool nxagentCloseScreen(int index, ScreenPtr pScreen);

#define nxagentScreen(window) nxagentDefaultScreen

extern int nxagentBitsPerPixel(int depth);

void nxagentSetScreenSaverTime(void);

void nxagentMinimizeFromFullScreen(ScreenPtr pScreen);

void nxagentMaximizeToFullScreen(ScreenPtr pScreen);

Window nxagentCreateIconWindow(void);

Bool nxagentMagicPixelZone(int x, int y);

Bool nxagentResizeScreen(ScreenPtr pScreen, int width, int height,
                             int mmWidth, int mmHeight);

int nxagentChangeScreenConfig(int screen, int width, int height, int mmWidth, int mmHeight);

extern Bool nxagentReconnectScreen(void *p0);

void nxagentSaveAreas(PixmapPtr pPixmap, RegionPtr prgnSave, int xorg, int yorg, WindowPtr pWin);

void nxagentRestoreAreas(PixmapPtr pPixmap, RegionPtr prgnRestore, int xorg, int yorg, WindowPtr pWin);

extern int monitorResolution;

int nxagentShadowCreateMainWindow( ScreenPtr pScreen, WindowPtr pWin,int width, int height);

int nxagentShadowSendUpdates(int *);

int nxagentShadowPoll(PixmapPtr, GCPtr, unsigned char, int, int, char *, int *, int *);

void nxagentShadowSetWindowsSize(void);

void nxagentSetWMNormalHints(int);

void nxagentShadowSetRatio(float, float);

/*
 * Change window settings to adapt to a ratio.
 */

extern void nxagentShadowAdaptToRatio(void);

/*
 * The pixmap shadowing the real frame buffer.
 */

extern PixmapPtr nxagentShadowPixmapPtr;

#endif /* __Screen_H__ */
