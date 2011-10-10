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

#include "scrnintstr.h"
#include "dix.h"
#include "mi.h"
#include "mibstore.h"
#include "resource.h"

#include "X.h"
#include "Xproto.h"

#include "Agent.h"
#include "Display.h"
#include "Visual.h"

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

/*
 * Predefined visual used for drawables
 * having a 32 bits depth.
 */

Visual nxagentAlphaVisual;

Visual *nxagentVisual(VisualPtr pVisual)
{
  XVisualInfo visual;

  int i;

  visual.class = pVisual->class;
  visual.bits_per_rgb = pVisual->bitsPerRGBValue;
  visual.colormap_size = pVisual->ColormapEntries;
  visual.depth = pVisual->nplanes;
  visual.red_mask = pVisual->redMask;
  visual.green_mask = pVisual->greenMask;
  visual.blue_mask = pVisual->blueMask;

  for (i = 0; i < nxagentNumVisuals; i++)
  {
    if (nxagentCompareVisuals(visual, nxagentVisuals[i]) == 1)
    {
      return nxagentVisuals[i].visual;
    }
  }

  return NULL;
}

Visual *nxagentVisualFromID(ScreenPtr pScreen, VisualID visual)
{
  int i;

  for (i = 0; i < pScreen->numVisuals; i++)
  {
    if (pScreen->visuals[i].vid == visual)
    {
      return nxagentVisual(&pScreen->visuals[i]);
    }
  }
  
  return NULL;
}

Colormap nxagentDefaultVisualColormap(Visual *visual)
{
  int i;

  for (i = 0; i < nxagentNumVisuals; i++)
  {
    if (nxagentVisuals[i].visual == visual)
    {
      return nxagentDefaultColormaps[i];
    }
  }

  return None;
}

/*
 * This is currently unused. It should serve
 * the scope of matching a visual whenever
 * a drawable has a different depth than the
 * real display.
 */

Visual *nxagentVisualFromDepth(ScreenPtr pScreen, int depth)
{
  int i;

  for (i = 0; i < pScreen->numVisuals; i++)
  {
    if (pScreen->visuals[i].nplanes == depth)
    {
      return nxagentVisual(&pScreen->visuals[i]);
    }
  }

  return NULL;
}

/*
 * Create a fake 32 bits depth visual and
 * initialize it based on the endianess
 * of the remote display.
 */

void nxagentInitAlphaVisual()
{
  nxagentAlphaVisual.visualid = XAllocID(nxagentDisplay);

  /*
   * Color masks are referred to bits inside
   * the pixel. This is independent from the
   * endianess.
   */

  nxagentAlphaVisual.red_mask   = 0x00ff0000;
  nxagentAlphaVisual.green_mask = 0x0000ff00;
  nxagentAlphaVisual.blue_mask  = 0x000000ff;

  #ifdef TEST
  fprintf(stderr,"nxagentInitAlphaVisual: Set alpha visual with id [0x%lx] mask [0x%lx,0x%lx,0x%lx].\n",
              nxagentAlphaVisual.visualid, nxagentAlphaVisual.red_mask,
                  nxagentAlphaVisual.green_mask, nxagentAlphaVisual.blue_mask);
  #endif
}
