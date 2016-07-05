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

#ifndef __Color_H__
#define __Color_H__

#define DUMB_WINDOW_MANAGERS

#define MAXCMAPS 1
#define MINCMAPS 1

typedef struct {
  Colormap colormap;
} nxagentPrivColormap;

typedef struct {
  int numCmapIDs;
  Colormap *cmapIDs;
  int numWindows;
  Window *windows;
  int index;
} nxagentInstalledColormapWindows;

#define nxagentColormapPriv(pCmap) \
  ((nxagentPrivColormap *)((pCmap)->devPriv))

#define nxagentColormap(pCmap) (nxagentColormapPriv(pCmap)->colormap)

#define nxagentPixel(pixel) (pixel)

Bool nxagentCreateColormap(ColormapPtr pCmap);

void nxagentDestroyColormap(ColormapPtr pCmap);

void nxagentSetInstalledColormapWindows(ScreenPtr pScreen);

void nxagentSetScreenSaverColormapWindow(ScreenPtr pScreen);

void nxagentDirectInstallColormaps(ScreenPtr pScreen);

void nxagentDirectUninstallColormaps(ScreenPtr pScreen);

void nxagentInstallColormap(ColormapPtr pCmap);

void nxagentUninstallColormap(ColormapPtr pCmap);

int nxagentListInstalledColormaps(ScreenPtr pScreen, Colormap *pCmapIds);

void nxagentStoreColors(ColormapPtr pCmap, int nColors, xColorItem *pColors);

void nxagentResolveColor(unsigned short *pRed, unsigned short *pGreen,
                             unsigned short *pBlue, VisualPtr pVisual);

Bool nxagentCreateDefaultColormap(ScreenPtr pScreen);

#endif /* __Color_H__ */
