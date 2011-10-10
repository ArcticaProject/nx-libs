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

#ifdef __DARWIN__

void DarwinHandleGUI(int argc, char *argv[])
{
}

void DarwinGlxExtensionInit()
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
  miInitVisualsProcPtr initVisuals;

  /*
   * Initialize the visuals to use the GLX extension.
   */

  initVisuals = NULL;

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

  nxagentRandRInitSizes(pScreen);

  /*
   * RRScreenInit sets these pointers to NULL,
   * so requiring the server to set up its own
   * replacements.
   */

  pRandRScrPriv = rrGetScrPriv(pScreen);

  pRandRScrPriv -> rrGetInfo   = nxagentRandRGetInfo;

  #if RANDR_12_INTERFACE
  pRandRScrPriv -> rrScreenSetSize = nxagentRandRScreenSetSize;
  #endif

  #if RANDR_10_INTERFACE
  pRandRScrPriv -> rrSetConfig = nxagentRandRSetConfig;
  #endif
}

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
  RRScreenSizePtr pSize;
  rrScrPrivPtr pRandRScrPriv = rrGetScrPriv(pScreen);

  int width;
  int height;

  int maxWidth;
  int maxHeight;

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

  int i;
  int nSizes;

  int mmWidth;
  int mmHeight;

  /*
   * Register all the supported sizes. The third
   * parameter is the refresh rate.
   */

  maxWidth  = WidthOfScreen(DefaultScreenOfDisplay(nxagentDisplay));
  maxHeight = HeightOfScreen(DefaultScreenOfDisplay(nxagentDisplay));

  nSizes = sizeof w / sizeof(int);

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

  w[0] = w[2];
  h[0] = h[2];

  for (i = 3; i < nSizes - 1; i++)
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

  for (i = 0; i < nSizes; i++)
  {
    width = w[i];
    height = h[i];

    mmWidth  = (width * 254 + monitorResolution * 5) / (monitorResolution * 10);

    if (mmWidth < 1)
    {
      mmWidth = 1;
    }

    mmHeight = (height * 254 + monitorResolution * 5) / (monitorResolution * 10);

    if (mmHeight < 1)
    {
      mmHeight = 1;
    }

    pSize = RRRegisterSize(pScreen, width, height, mmWidth, mmHeight);

    if (pSize == NULL)
    {
      return 0;
    }

    RRRegisterRate (pScreen, pSize, 60);
  }

  RRSetCurrentConfig(pScreen, RR_Rotate_0, 60, pSize);

  return 1;
}

#if RANDR_10_INTERFACE

int nxagentRandRSetConfig(ScreenPtr pScreen, Rotation rotation,
                              int rate, RRScreenSizePtr pSize)
{
  int r;
  rrScrPrivPtr pRandRScrPriv;

  UpdateCurrentTime();

  /*
   * Whatever size is OK for us.
   */

  r = nxagentResizeScreen(pScreen, pSize -> width, pSize -> height,
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
  int result;
  rrScrPrivPtr pRandRScrPriv;

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

  result = nxagentResizeScreen(pScreen, width, height, mmWidth, mmHeight);

  if (result == 1 && nxagentOption(DesktopResize) == 1 &&
          nxagentOption(Fullscreen) == 0 && nxagentOption(AllScreens) == 0)
  {
    nxagentRandRSetWindowsSize(width, height);
    nxagentSetWMNormalHints(pScreen -> myNum);
  }

  nxagentMoveViewport(pScreen, 0, 0);

  return result;
}

#endif
