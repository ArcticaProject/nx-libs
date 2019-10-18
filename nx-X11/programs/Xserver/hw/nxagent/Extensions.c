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

#ifdef HAVE_NXAGENT_CONFIG_H
#include <nxagent-config.h>
#endif

#include "micmap.h"
#include "scrnintstr.h"
#include "../../randr/randrstr.h"

#include "Agent.h"
#include "Display.h"
#include "Screen.h"
#include "Options.h"
#include "Extensions.h"
#include "Windows.h"

void GlxExtensionInit(void);
void GlxWrapInitVisuals(void *procPtr);

static int nxagentRandRScreenSetSize(ScreenPtr pScreen, CARD16 width,
                                         CARD16 height, CARD32 mmWidth,
                                             CARD32 mmHeight);

static int nxagentRandRInitSizes(ScreenPtr pScreen);

#if RANDR_14_INTERFACE
static Bool
nxagentRandRReplaceScanoutPixmap(DrawablePtr pDrawable,
				 PixmapPtr pPixmap,
				 Bool enable);
#endif

#if RANDR_13_INTERFACE
static Bool
nxagentRandROutputGetProperty(ScreenPtr pScreen,
                              RROutputPtr output,
                              Atom property);
static Bool
nxagentRandRGetPanning(ScreenPtr pScrn,
                       RRCrtcPtr crtc,
                       BoxPtr totalArea,
                       BoxPtr trackingArea,
                       INT16 *border);
static Bool
nxagentRandRSetPanning(ScreenPtr pScrn,
                       RRCrtcPtr crtc,
                       BoxPtr totalArea,
                       BoxPtr trackingArea,
                       INT16 *border);
#endif

#if RANDR_12_INTERFACE
static Bool nxagentRandRCrtcSet (ScreenPtr pScreen, RRCrtcPtr crtc,
				 RRModePtr mode, int x, int y,
				 Rotation rotation, int numOutputs,
				 RROutputPtr *outputs);
#endif

#ifdef __DARWIN__

void DarwinHandleGUI(int argc, char *argv[])
{
}

void DarwinGlxExtensionInit(void)
{
  GlxExtensionInit();
}

void DarwinGlxWrapInitVisuals(void *procPtr)
{
  GlxWrapInitVisuals(procPtr);
}

#endif

void nxagentInitGlxExtension(VisualPtr *visuals, DepthPtr *depths,
                                 int *numVisuals, int *numDepths, int *rootDepth,
                                     VisualID *defaultVisual)
{

  /*
   * Initialize the visuals to use the GLX extension.
   */

  miInitVisualsProcPtr initVisuals = NULL;

  GlxWrapInitVisuals(&initVisuals);

  if (initVisuals(visuals, depths, numVisuals, numDepths,
                      rootDepth, defaultVisual, 0, 0, 0) == 0)
  {
    fprintf(stderr, "Warning: Failed to initialize the GLX extension.\n");
  }
}

void nxagentInitRandRExtension(ScreenPtr pScreen)
{
  rrScrPrivPtr pRandRScrPriv;

  if (RRScreenInit(pScreen) == 0)
  {
    fprintf(stderr, "Warning: Failed to initialize the RandR extension.\n");
  }


  /* FIXME: do we need this at all with the new rand/xinerama stuff? */
  nxagentRandRInitSizes(pScreen);

  /*
   * RRScreenInit sets these pointers to NULL,
   * so requiring the server to set up its own
   * replacements.
   */

  pRandRScrPriv = rrGetScrPriv(pScreen);

  pRandRScrPriv -> rrGetInfo = nxagentRandRGetInfo;

  #if RANDR_15_INTERFACE
  /* nothing to be assigned here, so far */
  #endif

  #if RANDR_14_INTERFACE
  /* no pixmap sharing in nx-X11 */
  pScreen->ReplaceScanoutPixmap = nxagentRandRReplaceScanoutPixmap;
  pRandRScrPriv -> rrCrtcSetScanoutPixmap = NULL;

  /* only fake provider support in nx-X11, so far */
  pRandRScrPriv -> provider = RRProviderCreate(pScreen, "default", 7);
  pRandRScrPriv -> rrProviderSetOutputSource = NULL;
  pRandRScrPriv -> rrProviderSetOffloadSink = NULL;
  pRandRScrPriv -> rrProviderGetProperty = NULL;
  pRandRScrPriv -> rrProviderSetProperty = NULL;
  #endif

  #if RANDR_13_INTERFACE
  pRandRScrPriv -> rrOutputGetProperty = nxagentRandROutputGetProperty;
  pRandRScrPriv -> rrGetPanning = nxagentRandRGetPanning;
  pRandRScrPriv -> rrSetPanning = nxagentRandRSetPanning;
  #endif

  #if RANDR_12_INTERFACE
  pRandRScrPriv -> rrScreenSetSize = nxagentRandRScreenSetSize;
  pRandRScrPriv -> rrCrtcSet = nxagentRandRCrtcSet;
  #endif

  #if RANDR_10_INTERFACE
  pRandRScrPriv -> rrSetConfig = nxagentRandRSetConfig;
  #endif
}

#if RANDR_14_INTERFACE
static Bool
nxagentRandRReplaceScanoutPixmap(DrawablePtr pDrawable,
				 PixmapPtr pPixmap,
				 Bool enable)
{
  /* FALSE means: not supported */
#ifdef DEBUG
  fprintf(stderr, "nxagentRandRReplaceScanoutPixmap: NX's RANDR does not support scan-out pixmaps.\n");
#endif
  return FALSE;
}
#endif

#if RANDR_13_INTERFACE
static Bool
nxagentRandROutputGetProperty(ScreenPtr pScreen,
			      RROutputPtr output,
			      Atom property)
{
  /* FALSE means: no property required to be modified on the fly here */
  return FALSE;
}

static Bool
nxagentRandRGetPanning(ScreenPtr pScrn,
                       RRCrtcPtr crtc,
                       BoxPtr totalArea,
                       BoxPtr trackingArea,
                       INT16 *border)
{
  /* FALSE means: no, panning is not supported at the moment...
   *              Panning requires several modes to be available for
   *              the NX<n> output(s).
   *
   * FIXME: Add more modes per output than the current window size.
   *        At least when in fullscreen mode.
   */
#ifdef DEBUG
  fprintf(stderr, "nxagentRandRGetPanning: RANDR Panning is currently not supported.\n");
#endif
  return FALSE;
}

static Bool
nxagentRandRSetPanning(ScreenPtr pScrn,
                       RRCrtcPtr crtc,
                       BoxPtr totalArea,
                       BoxPtr trackingArea,
                       INT16 *border)
{
  /* FALSE means: no, panning is not supported at the moment...
   *              Panning requires several modes to be available for
   *              the NX<n> output(s).
   *
   * FIXME: Add more modes per output than the current window size.
   *        At least when in fullscreen mode.
   */
#ifdef DEBUG
  fprintf(stderr, "nxagentRandRSetPanning: RANDR Panning is currently not supported.\n");
#endif
  return FALSE;
}
#endif

#if RANDR_12_INTERFACE
/*
 * Request that the Crtc be reconfigured
 */

static Bool
nxagentRandRCrtcSet (ScreenPtr   pScreen,
		     RRCrtcPtr   crtc,
		     RRModePtr   mode,
		     int         x,
		     int         y,
		     Rotation    rotation,
		     int         numOutputs,
		     RROutputPtr *outputs)
{
  return RRCrtcNotify(crtc, mode, x, y, rotation, NULL, numOutputs, outputs);
}
#endif


int nxagentRandRGetInfo(ScreenPtr pScreen, Rotation *pRotations)
{
  /*
   * Rotation is not supported.
   */

  *pRotations = RR_Rotate_0;

  return 1;
}

static int nxagentRandRInitSizes(ScreenPtr pScreen)
{
  const int refresh_rate = 60;
  RRScreenSizePtr pSize = NULL;

  /*
    Index[0]: default size
    Index[nsizes-1]: current size
    Index[nsizes-2]: max size
  */

  /*
  int w[] = {0, 160, 320, 640, 800, 1024, 1152, 1280, 1280, 1280, 1280, 1280,
                 1280, 1360, 1440, 1600, 1600, 1680, 1920, 1920, 0, 0};
  int h[] = {0, 120, 240, 480, 600,  768,  864,  600,  720,  800,  854,  960,
                 1024,  768,  900,  900, 1200, 1050, 1080, 1200, 0, 0};
  */
  int w[] = {0, 320, 640, 640, 800, 800, 1024, 1024, 1152, 1280, 1280, 1280, 1360,
                 1440, 1600, 1600, 1680, 1920, 1920, 0, 0};
  int h[] = {0, 240, 360, 480, 480, 600,  600,  768,  864,  720,  800, 1024,  768,
                  900,  900, 1200, 1050, 1080, 1200, 0, 0};

  int maxWidth  = WidthOfScreen(DefaultScreenOfDisplay(nxagentDisplay));
  int maxHeight = HeightOfScreen(DefaultScreenOfDisplay(nxagentDisplay));

  int nSizes = sizeof w / sizeof(int);

  /*
   * Add current and max sizes.
   */

  w[nSizes - 1] = pScreen -> width;
  h[nSizes - 1] = pScreen -> height;

  w[nSizes - 2] = maxWidth;
  h[nSizes - 2] = maxHeight;

  /*
   * Compute default size.
   */

  w[0] = w[1];
  h[0] = h[1];

  for (int i = 2; i < nSizes - 1; i++)
  {
    if ((w[i] <= maxWidth * 3 / 4) && 
            (h[i] <= maxHeight * 3 / 4) &&
                (w[i] >= w[0]) &&
                    (h[i] >= h[0]))
    {
      w[0] = w[i];
      h[0] = h[i];
    }
  }

  /*
   * Register all the supported sizes at a fixed refresh rate.
   */

  for (int i = 0; i < nSizes; i++)
  {
    int mmWidth, mmHeight;

    if (monitorResolution < 0)
    {
      mmWidth  = w[i] * DisplayWidthMM(nxagentDisplay, DefaultScreen(nxagentDisplay)) /
                     DisplayWidth(nxagentDisplay, DefaultScreen(nxagentDisplay));

      mmHeight = h[i] * DisplayHeightMM(nxagentDisplay, DefaultScreen(nxagentDisplay)) /
                     DisplayHeight(nxagentDisplay, DefaultScreen(nxagentDisplay));
    }
    else
    {
      mmWidth  = (w[i] * 254 + monitorResolution * 5) / (monitorResolution * 10);
      mmHeight = (h[i] * 254 + monitorResolution * 5) / (monitorResolution * 10);
    }

    pSize = RRRegisterSize(pScreen, w[i], h[i], mmWidth < 1 ? 1 : mmWidth, mmHeight < 1 ? 1 : mmHeight);

    if (pSize == NULL)
    {
      return 0;
    }

    RRRegisterRate (pScreen, pSize, refresh_rate);
  }

  /*
   * the last registered size should be the current size
   */

  if (pSize)
  {
    RRSetCurrentConfig(pScreen, RR_Rotate_0, refresh_rate, pSize);
  }

  return 1;
}

#if RANDR_10_INTERFACE

int nxagentRandRSetConfig(ScreenPtr pScreen, Rotation rotation,
                              int rate, RRScreenSizePtr pSize)
{
  UpdateCurrentTime();

  /*
   * Whatever size is OK for us.
   */

  int r = nxagentResizeScreen(pScreen, pSize -> width, pSize -> height,
                                 pSize -> mmWidth, pSize -> mmHeight);

  nxagentMoveViewport(pScreen, 0, 0);

  return r;
}

#endif

#if RANDR_12_INTERFACE

void nxagentRandRSetWindowsSize(int width, int height)
{
  if (width == 0)
  {
    if (nxagentOption(Fullscreen) == 1)
    {
      width = WidthOfScreen(DefaultScreenOfDisplay(nxagentDisplay));
    }
    else
    {
      width = nxagentOption(Width);
    }
  }

  if (height == 0)
  {
    if (nxagentOption(Fullscreen) == 1)
    {
      height = HeightOfScreen(DefaultScreenOfDisplay(nxagentDisplay));
    }
    else
    {
      height = nxagentOption(Height);
    }
  }

  XResizeWindow(nxagentDisplay, nxagentDefaultWindows[0], width, height);

  if (nxagentOption(Rootless) == 0)
  {
    XMoveResizeWindow(nxagentDisplay, nxagentInputWindows[0], 0, 0, width,
                          height);
  }
}

int nxagentRandRScreenSetSize(ScreenPtr pScreen, CARD16 width, CARD16 height,
                                   CARD32 mmWidth, CARD32 mmHeight)
{
  UpdateCurrentTime();

  if (nxagentOption(DesktopResize) == 1 &&
          (nxagentOption(Fullscreen) == 1 ||
               width > WidthOfScreen(DefaultScreenOfDisplay(nxagentDisplay)) ||
                   height > HeightOfScreen(DefaultScreenOfDisplay(nxagentDisplay))))
  {
    if (nxagentOption(ClientOs) != ClientOsWinnt
            /*&& nxagentOption(ClientOs) != ClientNXPlayer*/)
    {
      nxagentChangeOption(DesktopResize, 0);
    }
  }

  if (nxagentOption(DesktopResize) == 1 && nxagentOption(Fullscreen) == 0 &&
          nxagentOption(AllScreens) == 0)
  {
    nxagentChangeOption(Width, width);
    nxagentChangeOption(Height, height);
  }

  int result = nxagentResizeScreen(pScreen, width, height, mmWidth, mmHeight);

  if (result == 1 && nxagentOption(DesktopResize) == 1 &&
          nxagentOption(Fullscreen) == 0 && nxagentOption(AllScreens) == 0)
  {
    nxagentRandRSetWindowsSize(width, height);
    nxagentSetWMNormalHints(pScreen -> myNum, nxagentOption(Width), nxagentOption(Height));
  }

  nxagentMoveViewport(pScreen, 0, 0);

  return result;
}

#endif
