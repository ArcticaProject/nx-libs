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

#include "X.h"
#include "Xproto.h"
#include "scrnintstr.h"
#include "../../include/window.h"
#include "windowstr.h"
#include "colormapst.h"
#include "resource.h"

#include "Agent.h"


#include "Display.h"
#include "Screen.h"
#include "Colormap.h"
#include "Visual.h"
#include "Windows.h"
#include "Args.h"

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

static ColormapPtr InstalledMaps[MAXSCREENS];

static Bool nxagentInstalledDefaultColormap = False;

Bool nxagentReconnectAllColormap(void *p0);

Bool nxagentCreateColormap(ColormapPtr pCmap)
{
  VisualPtr pVisual;
  XColor *colors;
  int i, ncolors;
  Pixel red, green, blue;
  Pixel redInc, greenInc, blueInc;

  Visual *visual;
  int class;

  #if defined(DEBUG) || defined(DEBUG_COLORMAP)
  fprintf(stderr, "nxagentCreateColormap: Going to create new colormap with "
              " visual [%lu].\n", pCmap->pVisual);
  #endif

  pVisual = pCmap->pVisual;
  ncolors = pVisual->ColormapEntries;

  pCmap->devPriv = (pointer)xalloc(sizeof(nxagentPrivColormap));

  if (((visual = nxagentVisual(pVisual))) == NULL)
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentCreateColormap: WARNING: Visual not found. Using default visual.\n");
    #endif

    visual = nxagentVisuals[nxagentDefaultVisualIndex].visual;
    class = nxagentVisuals[nxagentDefaultVisualIndex].class;
  }
  else
  {
    class = pVisual->class;
  }


  nxagentColormapPriv(pCmap)->colormap =
    XCreateColormap(nxagentDisplay,
                    nxagentDefaultWindows[pCmap->pScreen->myNum],
                    visual,
                    (class & DynamicClass) ?
                    AllocAll : AllocNone);

  switch (class) {
  case StaticGray: /* read only */
    colors = (XColor *)xalloc(ncolors * sizeof(XColor));
    for (i = 0; i < ncolors; i++)
      colors[i].pixel = i;
    XQueryColors(nxagentDisplay, nxagentColormap(pCmap), colors, ncolors);
    for (i = 0; i < ncolors; i++) {
      pCmap->red[i].co.local.red = colors[i].red;
      pCmap->red[i].co.local.green = colors[i].red;
      pCmap->red[i].co.local.blue = colors[i].red;
    }
    xfree(colors);
    break;

  case StaticColor: /* read only */
    colors = (XColor *)xalloc(ncolors * sizeof(XColor));
    for (i = 0; i < ncolors; i++)
      colors[i].pixel = i;
    XQueryColors(nxagentDisplay, nxagentColormap(pCmap), colors, ncolors);
    for (i = 0; i < ncolors; i++) {
      pCmap->red[i].co.local.red = colors[i].red;
      pCmap->red[i].co.local.green = colors[i].green;
      pCmap->red[i].co.local.blue = colors[i].blue;
    }
    xfree(colors);
    break;

  case TrueColor: /* read only */
    colors = (XColor *)xalloc(ncolors * sizeof(XColor));
    red = green = blue = 0L;
    redInc = lowbit(pVisual->redMask);
    greenInc = lowbit(pVisual->greenMask);
    blueInc = lowbit(pVisual->blueMask);
    for (i = 0; i < ncolors; i++) {
      colors[i].pixel = red | green | blue;
      red += redInc;
      if (red > pVisual->redMask) red = 0L;
      green += greenInc;
      if (green > pVisual->greenMask) green = 0L;
      blue += blueInc;
      if (blue > pVisual->blueMask) blue = 0L;
    }
    XQueryColors(nxagentDisplay, nxagentColormap(pCmap), colors, ncolors);
    for (i = 0; i < ncolors; i++) {
      pCmap->red[i].co.local.red = colors[i].red;
      pCmap->green[i].co.local.green = colors[i].green;
      pCmap->blue[i].co.local.blue = colors[i].blue;
    }
    xfree(colors);
    break;

  case GrayScale: /* read and write */
    break;

  case PseudoColor: /* read and write */
    break;

  case DirectColor: /* read and write */
    break;
  }

  return True;
}

void nxagentDestroyColormap(ColormapPtr pCmap)
{
  XFreeColormap(nxagentDisplay, nxagentColormap(pCmap));
  xfree(pCmap->devPriv);
}

#define SEARCH_PREDICATE \
  (nxagentWindow(pWin) != None && wColormap(pWin) == icws->cmapIDs[i])

static int nxagentCountInstalledColormapWindows(WindowPtr pWin, pointer ptr)
{
  nxagentInstalledColormapWindows *icws = (nxagentInstalledColormapWindows *) ptr;

  int i;

  for (i = 0; i < icws->numCmapIDs; i++)
    if (SEARCH_PREDICATE) {
      icws->numWindows++;
      return WT_DONTWALKCHILDREN;
    }

  return WT_WALKCHILDREN;
}

static int nxagentGetInstalledColormapWindows(WindowPtr pWin, pointer ptr)
{
  nxagentInstalledColormapWindows *icws = (nxagentInstalledColormapWindows *)ptr;
  int i;

  for (i = 0; i < icws->numCmapIDs; i++)
    if (SEARCH_PREDICATE) {
      icws->windows[icws->index++] = nxagentWindow(pWin);
      return WT_DONTWALKCHILDREN;
    }

  return WT_WALKCHILDREN;
}

static Window *nxagentOldInstalledColormapWindows = NULL;
static int nxagentNumOldInstalledColormapWindows = 0;

static Bool nxagentSameInstalledColormapWindows(Window *windows, int numWindows)
{
  if (nxagentNumOldInstalledColormapWindows != numWindows)
    return False;

  if (nxagentOldInstalledColormapWindows == windows)
    return True;

  if (nxagentOldInstalledColormapWindows == NULL || windows == NULL)
    return False;

  if (memcmp(nxagentOldInstalledColormapWindows, windows,
	   numWindows * sizeof(Window)))
    return False;

  return True;
}

void nxagentSetInstalledColormapWindows(ScreenPtr pScreen)
{
  nxagentInstalledColormapWindows icws;
  int numWindows;

  icws.cmapIDs = (Colormap *)xalloc(pScreen->maxInstalledCmaps *
				    sizeof(Colormap));
  icws.numCmapIDs = nxagentListInstalledColormaps(pScreen, icws.cmapIDs);
  icws.numWindows = 0;
  WalkTree(pScreen, nxagentCountInstalledColormapWindows, (pointer)&icws);
  if (icws.numWindows) {
    icws.windows = (Window *)xalloc((icws.numWindows + 1) * sizeof(Window));
    icws.index = 0;
    WalkTree(pScreen, nxagentGetInstalledColormapWindows, (pointer)&icws);
    icws.windows[icws.numWindows] = nxagentDefaultWindows[pScreen->myNum];
    numWindows = icws.numWindows + 1;
  }
  else {
    icws.windows = NULL;
    numWindows = 0;
  }

  xfree(icws.cmapIDs);

  if (!nxagentSameInstalledColormapWindows(icws.windows, icws.numWindows)) {
    if (nxagentOldInstalledColormapWindows)
      xfree(nxagentOldInstalledColormapWindows);

#ifdef _XSERVER64
    {
      int i;
      Window64 *windows = (Window64 *)xalloc(numWindows * sizeof(Window64));

      for(i = 0; i < numWindows; ++i)
	  windows[i] = icws.windows[i];
      XSetWMColormapWindows(nxagentDisplay, nxagentDefaultWindows[pScreen->myNum],
			    windows, numWindows);
      xfree(windows);
    }
#else
    XSetWMColormapWindows(nxagentDisplay, nxagentDefaultWindows[pScreen->myNum],
			  icws.windows, numWindows);
#endif

    nxagentOldInstalledColormapWindows = icws.windows;
    nxagentNumOldInstalledColormapWindows = icws.numWindows;

#ifdef DUMB_WINDOW_MANAGERS
    /*
      This code is for dumb window managers.
      This will only work with default local visual colormaps.
      */
    if (icws.numWindows)
      {
	WindowPtr pWin;
	Visual *visual;
	ColormapPtr pCmap;

	pWin = nxagentWindowPtr(icws.windows[0]);
	visual = nxagentVisualFromID(pScreen, wVisual(pWin));

	if (visual == nxagentDefaultVisual(pScreen))
	  pCmap = (ColormapPtr)LookupIDByType(wColormap(pWin),
					      RT_COLORMAP);
	else
	  pCmap = (ColormapPtr)LookupIDByType(pScreen->defColormap,
					      RT_COLORMAP);

        if (pCmap != NULL)
        {
          XSetWindowColormap(nxagentDisplay,
	                         nxagentDefaultWindows[pScreen->myNum],
                                     nxagentColormap(pCmap));
        }
        #ifdef WARNING
        else
        {
          fprintf(stderr, "nxagentSetInstalledColormapWindows: WARNING! "
                      "Window at [%p] has no colormap with class [%d].\n",
                          pWin, pWin -> drawable.class);
        }
        #endif
      }
#endif /* DUMB_WINDOW_MANAGERS */
  }
  else
    if (icws.windows) xfree(icws.windows);
}

void nxagentSetScreenSaverColormapWindow(ScreenPtr pScreen)
{
  if (nxagentOldInstalledColormapWindows)
    xfree(nxagentOldInstalledColormapWindows);

#ifdef _XSERVER64
  {
    Window64 window;

    window = nxagentScreenSaverWindows[pScreen->myNum];
    XSetWMColormapWindows(nxagentDisplay, nxagentDefaultWindows[pScreen->myNum],
			  &window, 1);
    nxagentScreenSaverWindows[pScreen->myNum] = window;
  }
#else
  XSetWMColormapWindows(nxagentDisplay, nxagentDefaultWindows[pScreen->myNum],
			&nxagentScreenSaverWindows[pScreen->myNum], 1);
#endif /* _XSERVER64 */

  nxagentOldInstalledColormapWindows = NULL;
  nxagentNumOldInstalledColormapWindows = 0;

  nxagentDirectUninstallColormaps(pScreen);
}

void nxagentDirectInstallColormaps(ScreenPtr pScreen)
{
  int i, n;
  Colormap pCmapIDs[MAXCMAPS];

  if (!nxagentDoDirectColormaps) return;

  n = (*pScreen->ListInstalledColormaps)(pScreen, pCmapIDs);

  for (i = 0; i < n; i++) {
    ColormapPtr pCmap;

    pCmap = (ColormapPtr)LookupIDByType(pCmapIDs[i], RT_COLORMAP);
    if (pCmap)
      XInstallColormap(nxagentDisplay, nxagentColormap(pCmap));
  }
}

void nxagentDirectUninstallColormaps(ScreenPtr pScreen)
{
  int i, n;
  Colormap pCmapIDs[MAXCMAPS];

  if (!nxagentDoDirectColormaps) return;

  n = (*pScreen->ListInstalledColormaps)(pScreen, pCmapIDs);

  for (i = 0; i < n; i++) {
    ColormapPtr pCmap;

    pCmap = (ColormapPtr)LookupIDByType(pCmapIDs[i], RT_COLORMAP);
    if (pCmap)
      XUninstallColormap(nxagentDisplay, nxagentColormap(pCmap));
  }
}

void nxagentInstallColormap(ColormapPtr pCmap)
{
  int index;
  ColormapPtr pOldCmap;

  index = pCmap->pScreen->myNum;
  pOldCmap = InstalledMaps[index];

  if(pCmap != pOldCmap)
    {
      nxagentDirectUninstallColormaps(pCmap->pScreen);

      /* Uninstall pInstalledMap. Notify all interested parties. */
      if(pOldCmap != (ColormapPtr)None)
	WalkTree(pCmap->pScreen, TellLostMap, (pointer)&pOldCmap->mid);

      InstalledMaps[index] = pCmap;
      WalkTree(pCmap->pScreen, TellGainedMap, (pointer)&pCmap->mid);

      nxagentSetInstalledColormapWindows(pCmap->pScreen);
      nxagentDirectInstallColormaps(pCmap->pScreen);
    }
}

void nxagentUninstallColormap(ColormapPtr pCmap)
{
  int index;
  ColormapPtr pCurCmap;

  index = pCmap->pScreen->myNum;
  pCurCmap = InstalledMaps[index];

  if(pCmap == pCurCmap)
    {
      if ((unsigned int)pCmap->mid != pCmap->pScreen->defColormap)
        {
	  pCurCmap = (ColormapPtr)LookupIDByType(pCmap->pScreen->defColormap,
						 RT_COLORMAP);
	  (*pCmap->pScreen->InstallColormap)(pCurCmap);
        }
    }
}

int nxagentListInstalledColormaps(ScreenPtr pScreen, Colormap *pCmapIds)
{
  if (nxagentInstalledDefaultColormap)
  {
    *pCmapIds = InstalledMaps[pScreen->myNum]->mid;

    return 1;
  }
  else
  {
    return 0;
  }
}

void nxagentStoreColors(ColormapPtr pCmap, int nColors, xColorItem *pColors)
{
  if (pCmap->pVisual->class & DynamicClass)
#ifdef _XSERVER64
  {
    int i;
    XColor *pColors64 = (XColor *)xalloc(nColors * sizeof(XColor) );

    for(i = 0; i < nColors; ++i)
    {
      pColors64[i].pixel = pColors[i].pixel;
      pColors64[i].red = pColors[i].red;
      pColors64[i].green = pColors[i].green;
      pColors64[i].blue = pColors[i].blue;
      pColors64[i].flags = pColors[i].flags;
    }
    XStoreColors(nxagentDisplay, nxagentColormap(pCmap), pColors64, nColors);
    xfree(pColors64);
  }
#else
    XStoreColors(nxagentDisplay, nxagentColormap(pCmap),
		 (XColor *)pColors, nColors);
#endif
}

void nxagentResolveColor(unsigned short *pRed, unsigned short *pGreen,
                             unsigned short *pBlue, VisualPtr pVisual)
{
  int shift;
  unsigned int lim;

  shift = 16 - pVisual->bitsPerRGBValue;
  lim = (1 << pVisual->bitsPerRGBValue) - 1;

  if ((pVisual->class == PseudoColor) || (pVisual->class == DirectColor))
    {
      /* rescale to rgb bits */
      *pRed = ((*pRed >> shift) * 65535) / lim;
      *pGreen = ((*pGreen >> shift) * 65535) / lim;
      *pBlue = ((*pBlue >> shift) * 65535) / lim;
    }
  else if (pVisual->class == GrayScale)
    {
      /* rescale to gray then rgb bits */
      *pRed = (30L * *pRed + 59L * *pGreen + 11L * *pBlue) / 100;
      *pBlue = *pGreen = *pRed = ((*pRed >> shift) * 65535) / lim;
    }
  else if (pVisual->class == StaticGray)
    {
      unsigned int limg;

      limg = pVisual->ColormapEntries - 1;
      /* rescale to gray then [0..limg] then [0..65535] then rgb bits */
      *pRed = (30L * *pRed + 59L * *pGreen + 11L * *pBlue) / 100;
      *pRed = ((((*pRed * (limg + 1))) >> 16) * 65535) / limg;
      *pBlue = *pGreen = *pRed = ((*pRed >> shift) * 65535) / lim;
    }
  else
    {
      unsigned limr, limg, limb;

      limr = pVisual->redMask >> pVisual->offsetRed;
      limg = pVisual->greenMask >> pVisual->offsetGreen;
      limb = pVisual->blueMask >> pVisual->offsetBlue;
      /* rescale to [0..limN] then [0..65535] then rgb bits */
      *pRed = ((((((*pRed * (limr + 1)) >> 16) *
		  65535) / limr) >> shift) * 65535) / lim;
      *pGreen = ((((((*pGreen * (limg + 1)) >> 16) *
		    65535) / limg) >> shift) * 65535) / lim;
      *pBlue = ((((((*pBlue * (limb + 1)) >> 16) *
		   65535) / limb) >> shift) * 65535) / lim;
    }
}

Bool nxagentCreateDefaultColormap(ScreenPtr pScreen)
{
  VisualPtr   pVisual;
  ColormapPtr pCmap;
  unsigned short zero = 0, ones = 0xFFFF;
  Pixel wp, bp;

  #if defined(DEBUG) || defined(DEBUG_COLORMAP)
  fprintf(stderr, "Debug: Searching for the root visual [%lu].\n",
              pScreen->rootVisual);
  #endif

  for (pVisual = pScreen->visuals;
       pVisual->vid != pScreen->rootVisual;
       pVisual++);

  if (CreateColormap(pScreen->defColormap, pScreen, pVisual, &pCmap,
		     (pVisual->class & DynamicClass) ? AllocNone : AllocAll, 0)
      != Success)
    return False;

  wp = pScreen->whitePixel;
  bp = pScreen->blackPixel;
  if ((AllocColor(pCmap, &ones, &ones, &ones, &wp, 0) !=
       Success) ||
      (AllocColor(pCmap, &zero, &zero, &zero, &bp, 0) !=
       Success))
    return FALSE;
  pScreen->whitePixel = wp;
  pScreen->blackPixel = bp;
  (*pScreen->InstallColormap)(pCmap);

  nxagentInstalledDefaultColormap = True;

  return True;
}

static void nxagentReconnectColormap(pointer p0, XID x1, pointer p2)
{
  ColormapPtr pCmap = (ColormapPtr)p0;
  Bool* pBool = (Bool*)p2;
  VisualPtr pVisual;

  #ifdef NXAGENT_RECONNECT_COLORMAP_DEBUG
  fprintf(stderr, "nxagentReconnectColormap: %p\n", pCmap);
  #endif

  if (!*pBool || !pCmap)
    return;

  pVisual = pCmap -> pVisual;

  nxagentColormapPriv(pCmap)->colormap =
      XCreateColormap(nxagentDisplay,
          nxagentDefaultWindows[pCmap->pScreen->myNum],
              nxagentVisual(pVisual),
                  (pVisual->class & DynamicClass) ?
                      AllocAll : AllocNone);

  #ifdef NXAGENT_RECONNECT_COLORMAP_DEBUG
  fprintf(stderr, "nxagentReconnectColormap: %p - ID %xl\n",
              pCmap, nxagentColormap(pCmap));
  #endif
}

Bool nxagentReconnectAllColormap(void *p0)
{
  int flexibility;
  int cid;
  Bool success = True;
  flexibility = *(int*)p0;

  #if defined(NXAGENT_RECONNECT_DEBUG) || defined(NXAGENT_RECONNECT_COLORMAP_DEBUG)
  fprintf(stderr, "nxagentReconnectAllColormap\n");
  #endif

  for (cid = 0; (cid < MAXCLIENTS) && success; cid++)
  {
    if (clients[cid] && success)
    {
      FindClientResourcesByType(clients[cid], RT_COLORMAP, nxagentReconnectColormap, &success);
    }
  }

  return success;
}

