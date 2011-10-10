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
#include "gcstruct.h"
#include "windowstr.h"
#include "pixmapstr.h"
#include "scrnintstr.h"
#include "dixstruct.h"
#include "fontstruct.h"
#include "mistruct.h"
#include "region.h"

#include "Agent.h"

#include "Display.h"
#include "GCs.h"
#include "GCOps.h"
#include "Image.h"
#include "Drawable.h"
#include "Pixmaps.h"
#include "Font.h"
#include "Colormap.h"
#include "Trap.h"
#include "Screen.h"
#include "Pixels.h"

#include "../../fb/fb.h"

RESTYPE RT_NX_GC;
/*
 * Set here the required log level.
 */

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

int nxagentGCPrivateIndex;

nxagentGraphicContextsPtr nxagentGraphicContexts;
int nxagentGraphicContextsSize;

void nxagentDisconnectGraphicContexts(void);
GCPtr nxagentCreateGraphicContext(int depth);

static void nxagentReconnectGC(void*, XID, void*);
static void nxagentReconnectClip(GCPtr, int, pointer, int);
static int  nxagentCompareRegions(RegionPtr, RegionPtr);

struct nxagentGCRec
{
  GCPtr pGC;
  XlibGC gc;
  struct nxagentGCRec *next;
};

static struct
{
  struct nxagentGCRec *first;
  struct nxagentGCRec *last;
  int size;
} nxagentGCList = { NULL, NULL, 0 };

static struct nxagentGCRec *nxagentGetFirstGC(void);

static void nxagentRestoreGCList(void);

static GCFuncs nxagentFuncs =
{
  nxagentValidateGC,
  nxagentChangeGC,
  nxagentCopyGC,
  nxagentDestroyGC,
  nxagentChangeClip,
  nxagentDestroyClip,
  nxagentCopyClip,
};

static GCOps nxagentOps =
{
  nxagentFillSpans,
  nxagentSetSpans,
  nxagentPutImage,
  nxagentCopyArea,
  nxagentCopyPlane,
  nxagentPolyPoint,
  nxagentPolyLines,
  nxagentPolySegment,
  nxagentPolyRectangle,
  nxagentPolyArc,
  nxagentFillPolygon,
  nxagentPolyFillRect,
  nxagentPolyFillArc,
  nxagentPolyText8,
  nxagentPolyText16,
  nxagentImageText8,
  nxagentImageText16,
  nxagentImageGlyphBlt,
  nxagentPolyGlyphBlt,
  nxagentPushPixels
};

Bool nxagentCreateGC(GCPtr pGC)
{
  FbGCPrivPtr pPriv;

  pGC->clientClipType = CT_NONE;
  pGC->clientClip = NULL;

  pGC->funcs = &nxagentFuncs;
  pGC->ops = &nxagentOps;

  pGC->miTranslate = 1;

  if (pGC -> stipple && !nxagentPixmapIsVirtual(pGC -> stipple))
  {
    #ifdef DEBUG
    fprintf(stderr, "nxagentCreateGC: GC at [%p] got real stipple at [%p] switched to virtual.\n",
                (void*)pGC, (void*)pGC -> stipple);
    #endif

    pGC -> stipple = nxagentVirtualPixmap(pGC -> stipple);
  }

  /*
   * We create the GC based on the default
   * drawables. The proxy knows this and
   * optimizes the encoding of the create
   * GC message to include the id of the
   * drawable in the checksum.
   */

  nxagentGCPriv(pGC)->gc = XCreateGC(nxagentDisplay,
                                     nxagentDefaultDrawables[pGC->depth],
                                     0L, NULL);

  #ifdef TEST
  fprintf(stderr, "nxagentCreateGC: GC [%p]\n", (void *) pGC);
  #endif

  pPriv = (pGC)->devPrivates[fbGCPrivateIndex].ptr;

  fbGetRotatedPixmap(pGC) = 0;
  fbGetExpose(pGC) = 1;
  fbGetFreeCompClip(pGC) = 0;
  fbGetCompositeClip(pGC) = 0;

  pPriv->bpp = BitsPerPixel (pGC->depth);

  nxagentGCPriv(pGC)->nClipRects = 0;

  memset(&(nxagentGCPriv(pGC) -> lastServerValues), 0, sizeof(XGCValues));

  /*
   * Init to default GC values.
   */

  nxagentGCPriv(pGC) -> lastServerValues.background = 1;

  nxagentGCPriv(pGC) -> lastServerValues.plane_mask = ~0;

  nxagentGCPriv(pGC) -> lastServerValues.graphics_exposures = 1;

  nxagentGCPriv(pGC) -> lastServerValues.dashes = 4;

  nxagentGCPriv(pGC) -> mid = FakeClientID(serverClient -> index);

  nxagentGCPriv(pGC) -> pPixmap = NULL;

  AddResource(nxagentGCPriv(pGC) -> mid, RT_NX_GC, (pointer) pGC);

  return True;
}

void nxagentValidateGC(GCPtr pGC, unsigned long changes, DrawablePtr pDrawable)
{
  PixmapPtr lastTile, lastStipple;

  DrawablePtr pVirtual = (pDrawable -> type == DRAWABLE_PIXMAP) ?
                          nxagentVirtualDrawable(pDrawable) :
                          pDrawable;

  #ifdef TEST
  fprintf(stderr, "nxagentValidateGC: Going to validate GC at [%p] for drawable at [%p] with changes [%lx].\n",
              (void *) pGC, (void *) pDrawable, changes);
  #endif

  pGC->lastWinOrg.x = pDrawable->x;
  pGC->lastWinOrg.y = pDrawable->y;

  if (!pGC -> tileIsPixel && !nxagentPixmapIsVirtual(pGC -> tile.pixmap))
  {
    pGC -> tile.pixmap = nxagentVirtualPixmap(pGC -> tile.pixmap); 
  }

  lastTile = pGC -> tile.pixmap;

  lastStipple = pGC->stipple;
  
  if (lastStipple)
  {
    pGC->stipple = nxagentVirtualPixmap(pGC->stipple);
  }

  #ifdef TEST
  fprintf(stderr, "nxagentValidateGC: Drawable at [%p] has type [%s] virtual [%p] bits per pixel [%d].\n",
              (void *) pDrawable, (pDrawable -> type == DRAWABLE_PIXMAP) ? "PIXMAP" : "WINDOW",
                  (void *) pVirtual, pVirtual -> bitsPerPixel);
  #endif

  if (pVirtual -> bitsPerPixel == 0)
  {
    /*
     * Don't enter fbValidateGC() with 0 bpp
     * or agent will block in a endless loop.
     */

    #ifdef WARNING
    fprintf(stderr, "nxagentValidateGC: WARNING! Virtual drawable at [%p] has invalid bits per pixel.\n",
                (void *) pVirtual);

    fprintf(stderr, "nxagentValidateGC: WARNING! While validating GC at [%p] for drawable at [%p] with changes [%lx].\n",
                (void *) pGC, (void *) pDrawable, changes);

    fprintf(stderr, "nxagentValidateGC: WARNING! Bad drawable at [%p] has type [%s] virtual [%p] bits per pixel [%d].\n",
                (void *) pDrawable, (pDrawable -> type == DRAWABLE_PIXMAP) ? "PIXMAP" : "WINDOW",
                    (void *) pVirtual, pVirtual -> bitsPerPixel);
    #endif
  }
  else
  {
    fbValidateGC(pGC, changes, pVirtual);
  }

  if (pGC->tile.pixmap != lastTile)
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentValidateGC: WARNING! Transforming pixmap at [%p] virtual at [%p] "
                "in virtual pixmap.\n", (void *) nxagentPixmapPriv(pGC -> tile.pixmap) -> pRealPixmap,
                    (void *) nxagentPixmapPriv(pGC -> tile.pixmap) -> pRealPixmap);
    #endif

    #ifdef TEST
    fprintf(stderr, "nxagentValidateGC: GC [%p] new tile [%p] from fb set as virtual\n",
                (void *) pGC, (void *) pGC->tile.pixmap);
    #endif

    nxagentPixmapIsVirtual(pGC->tile.pixmap) = True;
    nxagentRealPixmap(pGC->tile.pixmap) = nxagentRealPixmap(lastTile);

    if (nxagentRealPixmap(lastTile))
    {
      nxagentPixmapPriv(nxagentRealPixmap(lastTile))->pVirtualPixmap = pGC->tile.pixmap;
    }
  }

  pGC->stipple = lastStipple;
}

void nxagentChangeGC(GCPtr pGC, unsigned long mask)
{
  #ifdef TEST
  static int nDiscarded;
  #endif

  XGCValues values;

  int changeFlag = 0;

  if (mask & GCFunction)
  {
    values.function = pGC->alu;

    changeFlag |= nxagentTestGC(values.function, function);
  }

  if (mask & GCPlaneMask)
  {
    values.plane_mask = pGC->planemask;

    changeFlag += nxagentTestGC(values.plane_mask, plane_mask);
  }

  if (mask & GCForeground)
  {
    values.foreground = nxagentPixel(pGC->fgPixel);

    changeFlag += nxagentTestGC(values.foreground, foreground);
  }

  if (mask & GCBackground)
  {
    values.background = nxagentPixel(pGC->bgPixel);

    changeFlag += nxagentTestGC(values.background, background);
  }

  if (mask & GCLineWidth)
  {
    values.line_width = pGC->lineWidth;

    changeFlag += nxagentTestGC(values.line_width, line_width);
  }

  if (mask & GCLineStyle)
  {
    values.line_style = pGC->lineStyle;

    changeFlag += nxagentTestGC(values.line_style, line_style);
  }

  if (mask & GCCapStyle)
  {
    values.cap_style = pGC->capStyle;

    changeFlag += nxagentTestGC(values.cap_style, cap_style);
  }

  if (mask & GCJoinStyle)
  {
    values.join_style = pGC->joinStyle;

    changeFlag += nxagentTestGC(values.join_style, join_style);
  }

  if (mask & GCFillStyle)
  {
    values.fill_style = pGC->fillStyle;

    changeFlag += nxagentTestGC(values.fill_style, fill_style);
  }

  if (mask & GCFillRule)
  {
    values.fill_rule = pGC->fillRule;

    changeFlag += nxagentTestGC(values.fill_rule, fill_rule);
  }

  if (mask & GCTile)
  {
    if (pGC->tileIsPixel)
    {
      mask &= ~GCTile;
    }
    else
    {
      if (nxagentDrawableStatus((DrawablePtr) pGC -> tile.pixmap) == NotSynchronized &&
              nxagentGCTrap == 0)
      {
        /*
         * If the tile is corrupted and is not too
         * much large, it can be synchronized imme-
         * diately. In the other cases, the tile is
         * cleared with a solid color to become usa-
         * ble. This approach should solve the high
         * delay on slow links waiting for a back-
         * ground tile to be synchronized.
         */

        if (nxagentOption(DeferLevel) >= 2 &&
               (pGC -> tile.pixmap -> drawable.width > 240 ||
                    pGC -> tile.pixmap -> drawable.height > 240))
        {
          #ifdef TEST
          fprintf(stderr, "nxagentChangeGC: WARNING! Going to fill with solid color the corrupted tile at [%p] "
                      "for GC at [%p] with size [%dx%d].\n", (void *) pGC -> tile.pixmap, (void *)pGC,
                          ((DrawablePtr) pGC -> tile.pixmap) -> width, ((DrawablePtr) pGC -> tile.pixmap) -> height);
          #endif

          nxagentFillRemoteRegion((DrawablePtr) pGC -> tile.pixmap, nxagentCorruptedRegion((DrawablePtr) pGC -> tile.pixmap));
        }
        else
        {
          #ifdef TEST
          fprintf(stderr, "nxagentChangeGC: WARNING! Synchronizing GC at [%p] due the tile at [%p] with size [%dx%d].\n",
                      (void *)pGC, (void *)pGC -> tile.pixmap, ((DrawablePtr) pGC -> tile.pixmap) -> width,
                          ((DrawablePtr) pGC -> tile.pixmap) -> height);
          #endif

          nxagentSynchronizeDrawable((DrawablePtr) pGC -> tile.pixmap, DO_WAIT, NEVER_BREAK, NULL);
        }
      }

      values.tile = nxagentPixmap(pGC->tile.pixmap);

      pGC->tile.pixmap = nxagentVirtualPixmap(pGC->tile.pixmap);

      #ifdef TEST
      fprintf(stderr, "nxagentChangeGC: New tile on GC [%p] tile is [%p]\n",
                  (void *) pGC, (void *) pGC->tile.pixmap);
      #endif

      changeFlag += nxagentTestGC(values.tile, tile);
    }
  }

  if (mask & GCStipple)
  {
    if (nxagentDrawableStatus((DrawablePtr) pGC -> stipple) == NotSynchronized &&
            nxagentGCTrap == 0)
    {
      #ifdef TEST
      fprintf(stderr, "nxagentChangeGC: WARNING! Synchronizing GC at [%p] due the stipple at [%p].\n",
                  (void *)pGC, (void *)pGC -> stipple);
      #endif

      nxagentSynchronizeDrawable((DrawablePtr) pGC -> stipple, DO_WAIT, NEVER_BREAK, NULL);
    }

    values.stipple = nxagentPixmap(pGC->stipple);

    pGC->stipple = nxagentVirtualPixmap(pGC->stipple);

    #ifdef TEST
    fprintf(stderr, "nxagentChangeGC: New stipple on GC [%p] stipple is [%p]\n",
                (void *) pGC, (void *) pGC->stipple);
    #endif

    changeFlag += nxagentTestGC(values.stipple, stipple);
  }

  if (mask & GCTileStipXOrigin)
  {
    values.ts_x_origin = pGC->patOrg.x;

    changeFlag += nxagentTestGC(values.ts_x_origin, ts_x_origin);
  }

  if (mask & GCTileStipYOrigin)
  {
    values.ts_y_origin = pGC->patOrg.y;

    changeFlag += nxagentTestGC(values.ts_y_origin, ts_y_origin);
  }

  if (mask & GCFont)
  {
    if (!nxagentFontStruct(pGC -> font))
    {
      mask &= ~GCFont;
    }
    else
    {
      values.font = nxagentFont(pGC->font);

      changeFlag += nxagentTestGC(values.font, font);
    }
  } 

  if (mask & GCSubwindowMode)
  {
    values.subwindow_mode = pGC->subWindowMode;

    changeFlag += nxagentTestGC(values.subwindow_mode, subwindow_mode);
  }

  if (mask & GCGraphicsExposures)
  {
    values.graphics_exposures = pGC->graphicsExposures;

    changeFlag += nxagentTestGC(values.graphics_exposures, graphics_exposures);
  }

  if (mask & GCClipXOrigin)
  {
    values.clip_x_origin = pGC->clipOrg.x;

    changeFlag += nxagentTestGC(values.clip_x_origin, clip_x_origin);
  }

  if (mask & GCClipYOrigin)
  {
    values.clip_y_origin = pGC->clipOrg.y;

    changeFlag += nxagentTestGC(values.clip_y_origin, clip_y_origin);
  }

  if (mask & GCClipMask)
  {
    /*
     * This is handled in the change clip.
     */

    mask &= ~GCClipMask;
  }

  if (mask & GCDashOffset)
  {
    values.dash_offset = pGC->dashOffset;

    changeFlag += nxagentTestGC(values.dash_offset, dash_offset);
  }

  if (mask & GCDashList)
  {
    mask &= ~GCDashList;

    if (nxagentGCTrap == 0)
    {
      XSetDashes(nxagentDisplay, nxagentGC(pGC),
                     pGC->dashOffset, (char *)pGC->dash, pGC->numInDashList);
    }
  }

  if (mask & GCArcMode)
  {
    values.arc_mode = pGC->arcMode;

    changeFlag += nxagentTestGC(values.arc_mode, arc_mode);
  }

  if (nxagentGCTrap == 1)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentChangeGC: Skipping change of GC at [%p] on the real X server.\n",
                (void *) pGC);
    #endif

    return;
  }

  if (mask && changeFlag)
  {
    XChangeGC(nxagentDisplay, nxagentGC(pGC), mask, &values);
  }
  #ifdef TEST
  else if (mask)
  {
    fprintf(stderr, "nxagentChangeGC: Discarded [%d] Mask [%lu]\n", ++nDiscarded, mask);
  }
  #endif
}

void nxagentCopyGC(GCPtr pGCSrc, unsigned long mask, GCPtr pGCDst)
{
  #ifdef TEST
  fprintf(stderr, "nxagentCopyGC: Copying the GC with source at [%p] destination "
              "at [%p] mask [%lu].\n", pGCSrc, pGCDst, mask);
  #endif

  /*
   * The MI function doesn't do anything.
   *
   * miCopyGC(pGCSrc, mask, pGCDst);
   */

  XCopyGC(nxagentDisplay, nxagentGC(pGCSrc), mask, nxagentGC(pGCDst));

  /*
   * Copy the private foreground field
   * of the GC if GCForeground is set.
   */

  nxagentCopyGCPriv(GCForeground,foreground,pGCSrc,mask,pGCDst);
}

void nxagentDestroyGC(GCPtr pGC)
{
  #ifdef TEST
  fprintf(stderr, "nxagentDestroyGC: GC at [%p].\n", (void *) pGC);
  #endif

  if (nxagentGCPriv(pGC) -> mid != 0)
  {
    FreeResource(nxagentGCPriv(pGC) -> mid, RT_NONE);
  }

  XFreeGC(nxagentDisplay, nxagentGC(pGC));

  miDestroyGC(pGC);
}

void nxagentChangeClip(GCPtr pGC, int type, pointer pValue, int nRects)
{
  int i, size;
  BoxPtr pBox;
  XRectangle *pRects;
  int clipsMatch = 0;

  #ifdef TEST
  fprintf(stderr, "nxagentChangeClip: Going to change clip on GC [%p]\n",
              (void *) pGC);
  #endif

  switch (type)
  {
    case CT_NONE:
    {
      clipsMatch = (pGC -> clientClipType == None);

      break;
    }
    case CT_REGION:
    {
      clipsMatch = nxagentCompareRegions(pGC -> clientClip, (RegionPtr) pValue);

      break;
    }
    case CT_UNSORTED:
    case CT_YSORTED:
    case CT_YXSORTED:
    case CT_YXBANDED:
    {
      RegionPtr pReg = RECTS_TO_REGION(pGC->pScreen, nRects, (xRectangle *)pValue, type);

      clipsMatch = nxagentCompareRegions(pGC -> clientClip, pReg);

      REGION_DESTROY(pGC->pScreen, pReg);

      break;
    }
    default:
    {
      clipsMatch = 0;

      break;
    }
  }

  nxagentDestroyClipHelper(pGC);

  #ifdef TEST
  fprintf(stderr, "nxagentChangeClip: Type [%d] regions clipsMatch [%d].\n",
              type, clipsMatch);
  #endif

  switch (type)
  {
    case CT_NONE:
    {
      if (clipsMatch == 0 && nxagentGCTrap == 0)
      {
        XSetClipMask(nxagentDisplay, nxagentGC(pGC), None);
      }

      break;
    }
    case CT_REGION:
    {
      if (clipsMatch == 0 && nxagentGCTrap == 0)
      {
        nRects = REGION_NUM_RECTS((RegionPtr)pValue);
        size = nRects * sizeof(*pRects);
        pRects = (XRectangle *) xalloc(size);
        pBox = REGION_RECTS((RegionPtr)pValue);

        for (i = nRects; i-- > 0;)
        {
          pRects[i].x = pBox[i].x1;
          pRects[i].y = pBox[i].y1;
          pRects[i].width = pBox[i].x2 - pBox[i].x1;
          pRects[i].height = pBox[i].y2 - pBox[i].y1;
        }

        XSetClipRectangles(nxagentDisplay, nxagentGC(pGC), pGC -> clipOrg.x, pGC -> clipOrg.y,
                               pRects, nRects, Unsorted);
        xfree((char *) pRects);
      }

      break;
    }
    case CT_PIXMAP:
    {
      if (nxagentGCTrap == 0)
      {
        XSetClipMask(nxagentDisplay, nxagentGC(pGC),
                         nxagentPixmap((PixmapPtr)pValue));
      }

      pGC->clientClip = (pointer) (*pGC->pScreen->BitmapToRegion)((PixmapPtr) pValue);

      nxagentGCPriv(pGC)->pPixmap = (PixmapPtr)pValue;

      pValue = pGC->clientClip;

      type = CT_REGION;

      break;
    }
    case CT_UNSORTED:
    {
      if (clipsMatch == 0 && nxagentGCTrap == 0)
      {    
        XSetClipRectangles(nxagentDisplay, nxagentGC(pGC),
                               pGC->clipOrg.x, pGC->clipOrg.y,
                                   (XRectangle *)pValue, nRects, Unsorted);
      }

      break;
    }
    case CT_YSORTED:
    {
      if (clipsMatch == 0 && nxagentGCTrap == 0)
      {
        XSetClipRectangles(nxagentDisplay, nxagentGC(pGC),
                           pGC->clipOrg.x, pGC->clipOrg.y,
                           (XRectangle *)pValue, nRects, YSorted);
      }

      break;
    }
    case CT_YXSORTED:
    {
      if (clipsMatch == 0 && nxagentGCTrap == 0)
      {
        XSetClipRectangles(nxagentDisplay, nxagentGC(pGC),
                           pGC->clipOrg.x, pGC->clipOrg.y,
                           (XRectangle *)pValue, nRects, YXSorted);
      }

      break;
    }
    case CT_YXBANDED:
    {
      if (clipsMatch == 0 && nxagentGCTrap == 0)
      {
        XSetClipRectangles(nxagentDisplay, nxagentGC(pGC),
                         pGC->clipOrg.x, pGC->clipOrg.y,
                         (XRectangle *)pValue, nRects, YXBanded);
      }

      break;
    }
  }

  switch(type)
  {
    case CT_UNSORTED:
    case CT_YSORTED:
    case CT_YXSORTED:
    case CT_YXBANDED:
    {
      /*
       * Other parts of the server can only
       * deal with CT_NONE, CT_PIXMAP and
       * CT_REGION client clips.
       */

      pGC->clientClip = (pointer) RECTS_TO_REGION(pGC->pScreen, nRects,
                                                  (xRectangle *)pValue, type);
      xfree(pValue);

      pValue = pGC->clientClip;

      type = CT_REGION;

      break;
    }
    default:
    {
      break;
    }
  }

  pGC->clientClipType = type;
  pGC->clientClip = pValue;

  nxagentGCPriv(pGC)->nClipRects = nRects;
}

void nxagentDestroyClip(GCPtr pGC)
{
  miDestroyClip(pGC);

  if (pGC->clientClipType == CT_PIXMAP)
  {
    (*pGC->pScreen->DestroyPixmap)((PixmapPtr) (pGC->clientClip));
  }

  nxagentDestroyClipHelper(pGC);

  if (nxagentGCTrap == 0)
  {
    XSetClipMask(nxagentDisplay, nxagentGC(pGC), None);
  }

  pGC->clientClipType = CT_NONE;
  pGC->clientClip = NULL;

  nxagentGCPriv(pGC)->nClipRects = 0;
}

void nxagentDestroyClipHelper(GCPtr pGC)
{
  switch (pGC->clientClipType)
  {
    default:
    case CT_NONE:
      break;
    case CT_REGION:
      REGION_DESTROY(pGC->pScreen, pGC->clientClip);
      break;
    case CT_PIXMAP:
      nxagentDestroyPixmap((PixmapPtr)pGC->clientClip);
      break;
  }

  if (nxagentGCPriv(pGC)->pPixmap != NULL)
  {
    nxagentDestroyPixmap(nxagentGCPriv(pGC)->pPixmap);
    nxagentGCPriv(pGC)->pPixmap = NULL;
  }
}

void nxagentCopyClip(GCPtr pGCDst, GCPtr pGCSrc)
{
  RegionPtr pRgn;

  #ifdef TEST
  fprintf(stderr, "nxagentCopyClip: Going to copy clip from GC [%p] to GC [%p]\n",
              (void *) pGCDst, (void *) pGCSrc);
  #endif

  switch (pGCSrc->clientClipType)
  {
    case CT_REGION:
      if (nxagentGCPriv(pGCSrc)->pPixmap == NULL)
      {
        pRgn = REGION_CREATE(pGCDst->pScreen, NULL, 1);
        REGION_COPY(pGCDst->pScreen, pRgn, pGCSrc->clientClip);
        nxagentChangeClip(pGCDst, CT_REGION, pRgn, 0);
      }
      else
      {
        nxagentGCPriv(pGCSrc)->pPixmap->refcnt++;

        nxagentChangeClip(pGCDst, CT_PIXMAP, nxagentGCPriv(pGCSrc)->pPixmap, 0);
      }
      break;
    case CT_PIXMAP:

      #ifdef WARNING
      fprintf(stderr, "nxagentCopyClip: WARNING! Not incrementing counter for virtual pixmap at [%p].\n",
                  (void *) nxagentVirtualPixmap((PixmapPtr) pGCSrc->clientClip));
      #endif

      ((PixmapPtr) pGCSrc->clientClip)->refcnt++;

      nxagentChangeClip(pGCDst, CT_PIXMAP, pGCSrc->clientClip, 0);

      break;

    case CT_NONE:
      nxagentDestroyClip(pGCDst);
      break;

  }
}

static struct nxagentGCRec *nxagentGetFirstGC()
{
  struct nxagentGCRec *tmp = nxagentGCList.first;

  if (nxagentGCList.size)
  {
    nxagentGCList.first = nxagentGCList.first -> next;
    nxagentGCList.size--;

    if (nxagentGCList.size == 0)
    {
      nxagentGCList.last = NULL;
    }
  }

  return tmp;
}

static void nxagentFreeGCRec(struct nxagentGCRec *t)
{
  #ifdef TEST
  fprintf(stderr, "nxagentFreeGCRec: Freeing record at %p GC freed at %p.\n",
              (void *) t, (void *) t -> gc);
  #endif

  xfree(t -> gc);

  free(t);
}

static void nxagentRestoreGCRec(struct nxagentGCRec *t)
{
  #ifdef TEST
  fprintf(stderr, "nxagentRestoreGCRec: Freeing record at %p GC freed at %p.\n",
              (void*)t, (void*)t -> gc);
  #endif

  if (nxagentGC(t -> pGC))
  {
    xfree(nxagentGC(t -> pGC));
  }

  nxagentGC(t -> pGC) = t -> gc;

  free(t);
}

static void nxagentAddGCToList(GCPtr pGC)
{
  struct nxagentGCRec *tempGC = malloc(sizeof(struct nxagentGCRec));

  if (tempGC == NULL)
  {
    FatalError("nxagentAddGCToList: malloc failed.");
  }

  #ifdef TEST
  fprintf(stderr, "nxagentAddGCToList: Adding GC %p to list at memory %p list size is %d.\n",
              (void *) pGC, (void *) tempGC, nxagentGCList.size);
  #endif

  tempGC -> pGC = pGC;
  tempGC -> gc = nxagentGC(pGC);
  tempGC -> next = NULL;

  if (nxagentGCList.size == 0 || nxagentGCList.first == NULL || nxagentGCList.last == NULL)
  {
    nxagentGCList.first = tempGC;
  }
  else
  {
    nxagentGCList.last -> next = tempGC;
  }

  nxagentGCList.last = tempGC;
  nxagentGCList.size++;
}

void nxagentFreeGCList()
{
  struct nxagentGCRec *tempGC;

  #ifdef TEST
  fprintf(stderr, "nxagentFreeGCList: List size is %d first elt at %p last elt at %p.\n",
              nxagentGCList.size, (void*)nxagentGCList.first, (void*)nxagentGCList.last);
  #endif

  while ((tempGC = nxagentGetFirstGC()))
  {
    nxagentFreeGCRec(tempGC);
  }
}

static void nxagentRestoreGCList()
{
  struct nxagentGCRec *tempGC;

  #ifdef TEST
  fprintf(stderr, "nxagentRestoreGCList: List size is %d first elt at %p last elt at %p.\n",
              nxagentGCList.size, (void*)nxagentGCList.first, (void*)nxagentGCList.last);
  #endif

  while ((tempGC = nxagentGetFirstGC()))
  {
    nxagentRestoreGCRec(tempGC);
  }
}

int nxagentDestroyNewGCResourceType(pointer p, XID id)
{
  /*
   * Address of the destructor is set in Init.c.
   */

  #ifdef TEST
  fprintf(stderr, "nxagentDestroyNewGCResourceType: Destroying mirror id [%ld] for GC at [%p].\n",
              nxagentGCPriv((GCPtr) p) -> mid, (void *) p);
  #endif

  nxagentGCPriv((GCPtr) p) -> mid = None;

  return 1;
}

static void nxagentReconnectGC(void *param0, XID param1, pointer param2)
{
  XGCValues values;
  unsigned long valuemask;
  GCPtr pGC = (GCPtr) param0;
  Bool *pBool = (Bool*)param2;

  if (pGC == NULL || !*pBool)
  {
    return;
  }

  if (nxagentGC(pGC))
  {
    nxagentAddGCToList(pGC);
  }
  else
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentReconnectGC: GCRec %p doesn't have a valid pointer to GC data.\n", (void*)pGC);
    #endif
  }

  #ifdef DEBUG 
  fprintf(stderr, "nxagentReconnectGC: GC at [%p].\n", (void *) pGC);
  #endif

  valuemask = 0;
  memset(&values,0,sizeof(XGCValues));
  values.function = pGC->alu;
  valuemask |= GCFunction;
  values.plane_mask = pGC->planemask;
  valuemask |= GCPlaneMask;
  values.foreground = nxagentPixel(pGC->fgPixel);
  valuemask |= GCForeground;
  values.background = nxagentPixel(pGC->bgPixel);
  valuemask |= GCBackground;

  values.line_width = pGC->lineWidth;
  valuemask |= GCLineWidth;
  values.line_style = pGC->lineStyle;
  valuemask |= GCLineStyle;
  values.cap_style = pGC->capStyle;
  valuemask |= GCCapStyle;
  values.join_style = pGC->joinStyle;
  valuemask |= GCJoinStyle;
  values.fill_style = pGC->fillStyle;
  valuemask |= GCFillStyle;
  values.fill_rule = pGC->fillRule;
  valuemask |= GCFillRule;

  if (!pGC -> tileIsPixel  && (pGC -> tile.pixmap != NULL))
  {
    if (nxagentPixmapIsVirtual(pGC -> tile.pixmap))
    {
      values.tile = nxagentPixmap(nxagentRealPixmap(pGC -> tile.pixmap));
    }
    else
    {
      values.tile = nxagentPixmap(pGC -> tile.pixmap);
    }

    valuemask |= GCTile;
  }

  if (pGC->stipple != NULL)
  {
    if (nxagentPixmapIsVirtual(pGC -> stipple))
    {
      #ifdef TEST
      fprintf(stderr, "nxagentReconnectGC: Reconnecting virtual stipple [%p] for GC [%p].\n",
                  (void *) pGC -> stipple, (void *) pGC);
      #endif

      if (nxagentPixmap(nxagentRealPixmap(pGC -> stipple)) == 0)
      {
        nxagentReconnectPixmap(nxagentRealPixmap(pGC -> stipple), 0, pBool);
      }

      values.stipple = nxagentPixmap(nxagentRealPixmap(pGC -> stipple));
    }
    else
    {
      #ifdef TEST
      fprintf(stderr, "nxagentReconnectGC: Reconnecting stipple [%p] for GC [%p].\n",
                  (void *) pGC -> stipple, (void *) pGC);
      #endif

      if (nxagentPixmap(pGC -> stipple) == 0)
      {
        nxagentReconnectPixmap(pGC -> stipple, 0, pBool);
      }

      values.stipple = nxagentPixmap(pGC->stipple);
    }
    valuemask |= GCStipple;
  }

  values.ts_x_origin = pGC->patOrg.x;
  valuemask |= GCTileStipXOrigin;
  values.ts_y_origin = pGC->patOrg.y;
  valuemask |= GCTileStipYOrigin;

  if (pGC->font != NULL)
  {
    values.font = nxagentFont(pGC->font);
    valuemask |= GCFont;
  }

  values.subwindow_mode = pGC->subWindowMode;
  valuemask |= GCSubwindowMode;
  values.graphics_exposures = pGC->graphicsExposures;
  valuemask |= GCGraphicsExposures;
  values.clip_x_origin = pGC->clipOrg.x;
  valuemask |= GCClipXOrigin;
  values.clip_y_origin = pGC->clipOrg.y;
  valuemask |= GCClipYOrigin;
  valuemask |= GCClipMask;
  values.dash_offset = pGC->dashOffset;
  valuemask |= GCDashOffset;

  if (pGC->dash != NULL)
  {
    values.dashes = *pGC->dash;
    valuemask |= GCDashList;
  }

  values.arc_mode = pGC->arcMode;
  valuemask |= GCArcMode;

  if ((nxagentGC(pGC) = XCreateGC(nxagentDisplay,
                                  nxagentDefaultDrawables[pGC->depth],
                                  valuemask, &values)) == NULL)
  {
    *pBool = False;
  }

  nxagentReconnectClip(pGC,
                       pGC -> clientClipType,
                       pGC -> clientClip, nxagentGCPriv(pGC) -> nClipRects);

  #ifdef DEBUG
  XSync(nxagentDisplay, 0);
  #endif
}

Bool nxagentReconnectAllGCs(void *p0)
{
  int flexibility;
  int cid;
  Bool GCSuccess = True;

  flexibility = *(int*)p0;

  #ifdef DEBUG
  fprintf(stderr, "nxagentReconnectAllGCs\n");
  #endif

  /*
   * The resource type RT_NX_GC is created on the
   * server client only, so we can avoid to loop
   * through all the clients.
   */

  FindClientResourcesByType(clients[serverClient -> index], RT_NX_GC, nxagentReconnectGC, &GCSuccess);

  for (cid = 0; (cid < MAXCLIENTS) && GCSuccess; cid++)
  {
    if (clients[cid])
    {
      #ifdef TEST
      fprintf(stderr, "nxagentReconnectAllGCs: Going to reconnect GC of client [%d].\n", cid);
      #endif

      FindClientResourcesByType(clients[cid], RT_GC, nxagentReconnectGC, &GCSuccess);
    }
  }

  #ifdef TEST
  fprintf(stderr, "nxagentReconnectAllGCs: GCs reconnection completed.\n");
  #endif

  return GCSuccess;
}

void nxagentDisconnectGC(pointer p0, XID x1, pointer p2)
{
  GCPtr pGC = (GCPtr) p0;
  Bool* pBool = (Bool*) p2;

  if (!*pBool || !pGC)
  {
    if (!pGC) 
    {
      #ifdef WARNING
      fprintf(stderr, "nxagentDisconnectGC: WARNING! pGC is NULL.\n");
      #endif
    }

    return;
  }

  if (pGC -> stipple)
  {
    PixmapPtr pMap = pGC -> stipple;

    nxagentDisconnectPixmap(nxagentRealPixmap(pMap), 0, pBool);
  }
} 

Bool nxagentDisconnectAllGCs()
{
  int cid;
  Bool success = True;

  #ifdef DEBUG
  fprintf(stderr, "nxagentDisconnectAllGCs\n");
  #endif

  /*
   * The resource type RT_NX_GC is created on the
   * server client only, so we can avoid to loop
   * through all the clients.
   */

  FindClientResourcesByType(clients[serverClient -> index], RT_NX_GC,
                                (FindResType) nxagentDisconnectGC, &success);

  for (cid = 0; (cid < MAXCLIENTS) && success; cid++)
  {
    if (clients[cid])
    {
      #ifdef TEST
      fprintf(stderr, "nxagentDisconnectAllGCs: Going to disconnect GC of client [%d].\n", cid);
      #endif

      FindClientResourcesByType(clients[cid], RT_GC,
                                    (FindResType) nxagentDisconnectGC, &success);
    }
  }

  #ifdef TEST
  fprintf(stderr, "nxagentDisconnectAllGCs: GCs disconnection completed.\n");
  #endif

  nxagentRestoreGCList();

  nxagentDisconnectGraphicContexts();

  return success;
}

static void nxagentReconnectClip(GCPtr pGC, int type, pointer pValue, int nRects)
{
  int i, size;
  BoxPtr pBox;
  XRectangle *pRects;

  #ifdef TEST
  fprintf(stderr, "nxagentReconnectClip: going to change clip on GC [%p]\n",
              (void *) pGC);
  #endif

  #ifdef DEBUG
  fprintf(stderr, "nxagentReconnectClip: Type is [%s].\n", (type == CT_NONE) ?
              "CT_NONE" : (type == CT_REGION) ? "CT_REGION" : (type == CT_PIXMAP) ?
                  "CT_REGION" : "UNKNOWN");
  #endif

  switch(type)
    {
    case CT_NONE:
      XSetClipMask(nxagentDisplay, nxagentGC(pGC), None);
      break;

    case CT_REGION:
      if (nxagentGCPriv(pGC)->pPixmap == NULL)
      {
        nRects = REGION_NUM_RECTS((RegionPtr)pValue);
        size = nRects * sizeof(*pRects);
        pRects = (XRectangle *) xalloc(size);
        pBox = REGION_RECTS((RegionPtr)pValue);
        for (i = nRects; i-- > 0;) {
          pRects[i].x = pBox[i].x1;
          pRects[i].y = pBox[i].y1;
          pRects[i].width = pBox[i].x2 - pBox[i].x1;
          pRects[i].height = pBox[i].y2 - pBox[i].y1;
        }

        /*
         * Originally, the clip origin area were 0,0 
         * but it didn't work with kedit and family,
         * because it got the clip mask of the pixmap
         * all traslated.
         */

        XSetClipRectangles(nxagentDisplay, nxagentGC(pGC), pGC -> clipOrg.x, pGC -> clipOrg.y,
                           pRects, nRects, Unsorted);
        xfree((char *) pRects);
      }
      else
      {
        XSetClipMask(nxagentDisplay, nxagentGC(pGC),
                         nxagentPixmap(nxagentGCPriv(pGC)->pPixmap));

        XSetClipOrigin(nxagentDisplay, nxagentGC(pGC), pGC -> clipOrg.x, pGC -> clipOrg.y);
      }

      break;

    case CT_PIXMAP:

      XSetClipMask(nxagentDisplay, nxagentGC(pGC),
                       nxagentPixmap((PixmapPtr)pValue));

      XSetClipOrigin(nxagentDisplay, nxagentGC(pGC), pGC -> clipOrg.x, pGC -> clipOrg.y);

      pGC->clientClip = (pointer) (*pGC->pScreen->BitmapToRegion)((PixmapPtr) pValue);

      nxagentGCPriv(pGC)->pPixmap = (PixmapPtr)pValue;

      pValue = pGC->clientClip;

      type = CT_REGION;

      break;

    case CT_UNSORTED:
      XSetClipRectangles(nxagentDisplay, nxagentGC(pGC),
                         pGC->clipOrg.x, pGC->clipOrg.y,
                         (XRectangle *)pValue, nRects, Unsorted);
      break;

    case CT_YSORTED:
      XSetClipRectangles(nxagentDisplay, nxagentGC(pGC),
                         pGC->clipOrg.x, pGC->clipOrg.y,
                         (XRectangle *)pValue, nRects, YSorted);
      break;

    case CT_YXSORTED:
      XSetClipRectangles(nxagentDisplay, nxagentGC(pGC),
                         pGC->clipOrg.x, pGC->clipOrg.y,
                         (XRectangle *)pValue, nRects, YXSorted);
      break;

    case CT_YXBANDED:
      XSetClipRectangles(nxagentDisplay, nxagentGC(pGC),
                         pGC->clipOrg.x, pGC->clipOrg.y,
                         (XRectangle *)pValue, nRects, YXBanded);
      break;
    }

  switch(type)
    {
    default:
      break;

    case CT_UNSORTED:
    case CT_YSORTED:
    case CT_YXSORTED:
    case CT_YXBANDED:

      /*
       * other parts of server can only deal with CT_NONE,
       * CT_PIXMAP and CT_REGION client clips.
       */

      pGC->clientClip = (pointer) RECTS_TO_REGION(pGC->pScreen, nRects,
                                                  (xRectangle *)pValue, type);
      xfree(pValue);
      pValue = pGC->clientClip;
      type = CT_REGION;

      break;
    }

  pGC->clientClipType = type;
  pGC->clientClip = pValue;

 nxagentGCPriv(pGC)->nClipRects = nRects;
}

static int nxagentCompareRegions(RegionPtr r1, RegionPtr r2)
{
  int i;

 /*
  *  It returns 1 if regions are equal, 0 otherwise
  */

  if (r1 == NULL && r2 == NULL)
  {
    return 1;
  }

  if ((r1 == NULL) || (r2 == NULL))
  {
    return 0;
  }

  if (REGION_NUM_RECTS(r1) !=  REGION_NUM_RECTS(r2))
  {
    return 0;
  }
  else if (REGION_NUM_RECTS(r1) == 0)
  {
    return 1;
  }
  else if ((*REGION_EXTENTS(pScreen, r1)).x1 !=  (*REGION_EXTENTS(pScreen, r2)).x1) return 0;
  else if ((*REGION_EXTENTS(pScreen, r1)).x2 !=  (*REGION_EXTENTS(pScreen, r2)).x2) return 0;
  else if ((*REGION_EXTENTS(pScreen, r1)).y1 !=  (*REGION_EXTENTS(pScreen, r2)).y1) return 0;
  else if ((*REGION_EXTENTS(pScreen, r1)).y2 !=  (*REGION_EXTENTS(pScreen, r2)).y2) return 0;
  else
  {
    for (i = 0; i < REGION_NUM_RECTS(r1); i++)
    {
      if (REGION_RECTS(r1)[i].x1 !=  REGION_RECTS(r2)[i].x1) return 0;
      else if (REGION_RECTS(r1)[i].x2 !=  REGION_RECTS(r2)[i].x2) return 0;
      else if (REGION_RECTS(r1)[i].y1 !=  REGION_RECTS(r2)[i].y1) return 0;
      else if (REGION_RECTS(r1)[i].y2 !=  REGION_RECTS(r2)[i].y2) return 0;
    }
  }

  return 1;
}

/*
 * This function have to be called in the place
 * of GetScratchGC if the GC will be used to per-
 * form operations also on the remote X Server.
 * This is why we call the XChangeGC at the end of
 * the function.
 */
GCPtr nxagentGetScratchGC(unsigned depth, ScreenPtr pScreen)
{
  GCPtr pGC;
  XGCValues values;
  unsigned long mask;
  int nxagentSaveGCTrap;

  /*
   * The GC trap is temporarily disabled in
   * order to allow the remote clipmask reset
   * requested by GetScratchGC().
   */

  nxagentSaveGCTrap = nxagentGCTrap;

  nxagentGCTrap = 0;

  pGC = GetScratchGC(depth, pScreen);

  nxagentGCTrap = nxagentSaveGCTrap;

  if (pGC == NULL)
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentGetScratchGC: Failed to retrieve the scratch GC.\n");
    #endif

    return NULL;
  }

  mask = 0;

  values.function = pGC -> alu;
  mask |= GCFunction;

  values.plane_mask = pGC -> planemask;
  mask |= GCPlaneMask;

  values.foreground = nxagentPixel(pGC -> fgPixel);
  mask |= GCForeground;

  values.background = nxagentPixel(pGC -> bgPixel);
  mask |= GCBackground;

  values.line_width = pGC -> lineWidth;
  mask |= GCLineWidth;

  values.line_style = pGC -> lineStyle;
  mask |= GCLineStyle;

  values.cap_style = pGC -> capStyle;
  mask |= GCCapStyle;

  values.join_style = pGC -> joinStyle;
  mask |= GCJoinStyle;

  values.fill_style = pGC -> fillStyle;
  mask |= GCFillStyle;

  values.fill_rule = pGC -> fillRule;
  mask |= GCFillRule;

  values.arc_mode = pGC -> arcMode;
  mask |= GCArcMode;

  values.ts_x_origin = pGC -> patOrg.x;
  mask |= GCTileStipXOrigin;

  values.ts_y_origin = pGC -> patOrg.y;
  mask |= GCTileStipYOrigin;

  values.subwindow_mode = pGC -> subWindowMode;
  mask |= GCSubwindowMode;

  values.graphics_exposures = pGC -> graphicsExposures;
  mask |= GCGraphicsExposures;

  /*
   * The GCClipMask is set to none inside
   * the GetScratchGC() function.
   */

  values.clip_x_origin = pGC -> clipOrg.x;
  mask |= GCClipXOrigin;

  values.clip_y_origin = pGC -> clipOrg.y;
  mask |= GCClipYOrigin;

  XChangeGC(nxagentDisplay, nxagentGC(pGC), mask, &values);

  memset(&(nxagentGCPriv(pGC) -> lastServerValues), 0, sizeof(XGCValues));

  return pGC;
}

/*
 * This function is only a wrapper for
 * FreeScratchGC.
 */
void nxagentFreeScratchGC(GCPtr pGC)
{
  if (pGC == NULL)
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentFreeScratchGC: WARNING! pGC is NULL.\n");
    #endif

    return;
  }

  FreeScratchGC(pGC);
}

/*
 * The GCs belonging to this list are used
 * only in the synchronization put images,
 * to be sure they preserve the default va-
 * lues and to avoid XChangeGC() requests.
 */

GCPtr nxagentGetGraphicContext(DrawablePtr pDrawable)
{
  int i;
  int result;

  for (i = 0; i < nxagentGraphicContextsSize; i++)
  {
    if (pDrawable -> depth == nxagentGraphicContexts[i].depth)
    {
      #ifdef DEBUG
      fprintf(stderr, "nxagentGetGraphicContext: Found a valid GC at [%p] for depth [%d].\n",
                  (void *) nxagentGraphicContexts[i].pGC, pDrawable -> depth);
      #endif

      /*
       * Reconnect the GC if needed.
       */

      if (nxagentGraphicContexts[i].dirty == 1)
      {
        #ifdef DEBUG
        fprintf(stderr, "nxagentGetGraphicContext: Going to reconnect the GC.\n");
        #endif

        result = 1;

        nxagentReconnectGC(nxagentGraphicContexts[i].pGC, (XID) 0, &result);

        if (result == 0)
        {
          #ifdef WARNING
          fprintf(stderr, "nxagentGetGraphicContext: WARNING! Failed to reconnect the GC.\n");
          #endif

          return NULL;
        }

        nxagentGraphicContexts[i].dirty = 0;
      }

      return nxagentGraphicContexts[i].pGC;
    }
  }

  return nxagentCreateGraphicContext(pDrawable -> depth);
}

GCPtr nxagentCreateGraphicContext(int depth)
{
  GCPtr pGC;

  nxagentGraphicContextsPtr nxagentGCs;

  XID attributes[2];

  /*
   * We have not found a GC, so we have
   * to spread the list and add a new GC.
   */

  nxagentGCs = xrealloc(nxagentGraphicContexts, (nxagentGraphicContextsSize + 1) * sizeof(nxagentGraphicContextsRec));
   
  if (nxagentGCs == NULL)
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentCreateGraphicContext: Cannot allocate memory for a GC.\n");
    #endif

    return NULL;
  }

  nxagentGraphicContexts = nxagentGCs;

  pGC = CreateScratchGC(nxagentDefaultScreen, depth);

  if (pGC == NULL)
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentCreateGraphicContext: Failed to create a GC for depth [%d].\n",
                depth);
    #endif

    return NULL;
  }

  /*
   * Color used in nxagentFillRemoteRegion().
   */

  attributes[0] = 0xc1c1c1;

  if (depth == 15 || depth == 16)
  {
    Color32to16(attributes[0]);
  }

  /*
   * The IncludeInferiors property is useful to
   * solve problems when synchronizing windows
   * covered by an invisible child.
   */

  attributes[1] = IncludeInferiors;

  ChangeGC(pGC, GCForeground | GCSubwindowMode, attributes);

  nxagentGraphicContexts[nxagentGraphicContextsSize].pGC   = pGC;
  nxagentGraphicContexts[nxagentGraphicContextsSize].depth = depth;
  nxagentGraphicContexts[nxagentGraphicContextsSize].dirty = 0;

  nxagentGraphicContextsSize++;

  #ifdef DEBUG
  fprintf(stderr, "nxagentCreateGraphicContext: GC [%p] for depth [%d] added to the list of size [%d].\n",
              (void *) pGC, depth, nxagentGraphicContextsSize);
  #endif

  return pGC;
}

/*
 * This initialization is called in the InitOutput()
 * function immediately after opening the screen,
 * which is used to create the GCs. 
 */

void nxagentAllocateGraphicContexts(void)
{
  int *depths;

  int i;

  depths = nxagentDepths;

  for (i = 0; i < nxagentNumDepths; i++)
  {
    nxagentCreateGraphicContext(*depths);

    depths++;
  }
}

void nxagentDisconnectGraphicContexts(void)
{
  int i;

  for (i = 0; i < nxagentGraphicContextsSize; i++)
  {
    nxagentGraphicContexts[i].dirty = 1;
  }

  return;
}

