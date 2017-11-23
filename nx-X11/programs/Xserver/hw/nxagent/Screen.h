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

#ifndef __Screen_H__
#define __Screen_H__

#include "scrnintstr.h"

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

Bool nxagentOpenScreen(ScreenPtr pScreen,
                           int argc, char *argv[]);

Bool nxagentCloseScreen(ScreenPtr pScreen);

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

int nxagentAdjustRandRXinerama(ScreenPtr pScreen);

extern Bool nxagentReconnectScreen(void *p0);

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
