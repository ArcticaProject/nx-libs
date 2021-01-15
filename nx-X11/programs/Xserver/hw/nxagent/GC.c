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

#include "X.h"
#include "Xproto.h"
#include "gcstruct.h"
#include "windowstr.h"
#include "pixmapstr.h"
#include "scrnintstr.h"
#include "dixstruct.h"
#include <X11/fonts/fontstruct.h>
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
#include "Utils.h"

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
static void nxagentReconnectClip(GCPtr, int, void *, int);
static Bool nxagentCompareRegions(RegionPtr, RegionPtr);

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
   * We create the GC based on the default drawables. The proxy knows
   * this and optimizes the encoding of the create GC message to
   * include the id of the drawable in the checksum.
   */

  nxagentGCPriv(pGC)->gc = XCreateGC(nxagentDisplay,
                                     nxagentDefaultDrawables[pGC->depth],
                                     0L, NULL);

  #ifdef TEST
  fprintf(stderr, "nxagentCreateGC: GC [%p]\n", (void *) pGC);
  #endif

  FbGCPrivPtr pPriv = (pGC)->devPrivates[fbGCPrivateIndex].ptr;

  fbGetRotatedPixmap(pGC) = 0;
  fbGetExpose(pGC) = 1;
  fbGetFreeCompClip(pGC) = 0;
  fbGetCompositeClip(pGC) = 0;

  pPriv->bpp = BitsPerPixel (pGC->depth);

  /*
   * Init to default GC values.
   */

  memset(&(nxagentGCPriv(pGC) -> lastServerValues), 0, sizeof(XGCValues));
  nxagentGCPriv(pGC) -> lastServerValues.background = 1;
  nxagentGCPriv(pGC) -> lastServerValues.plane_mask = ~0;
  nxagentGCPriv(pGC) -> lastServerValues.graphics_exposures = 1;
  nxagentGCPriv(pGC) -> lastServerValues.dashes = 4;

  nxagentGCPriv(pGC) -> nClipRects = 0;
  nxagentGCPriv(pGC) -> mid = FakeClientID(serverClient -> index);
  nxagentGCPriv(pGC) -> pPixmap = NULL;

  AddResource(nxagentGCPriv(pGC) -> mid, RT_NX_GC, (void *) pGC);

  return True;
}

void nxagentValidateGC(GCPtr pGC, unsigned long changes, DrawablePtr pDrawable)
{
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

  PixmapPtr lastTile = pGC -> tile.pixmap;

  PixmapPtr lastStipple = pGC->stipple;
  
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
     * Don't enter fbValidateGC() with 0 bpp or agent will block in a
     * endless loop.
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

#define CHECKGCVAL(cmask, member, val) do {if (mask & cmask) { values.member = (val); changeFlag += nxagentTestGC(values.member, member); } } while (0)

void nxagentChangeGC(GCPtr pGC, unsigned long mask)
{
  #ifdef TEST
  static int nDiscarded;
  #endif

  XGCValues values = {0};
  int changeFlag = 0;

  CHECKGCVAL(GCFunction, function, pGC->alu);
  CHECKGCVAL(GCPlaneMask, plane_mask, pGC->planemask);
  CHECKGCVAL(GCForeground, foreground, nxagentPixel(pGC->fgPixel));
  CHECKGCVAL(GCBackground, background, nxagentPixel(pGC->bgPixel));
  CHECKGCVAL(GCLineWidth, line_width, pGC->lineWidth);
  CHECKGCVAL(GCLineStyle, line_style, pGC->lineStyle);
  CHECKGCVAL(GCCapStyle, cap_style, pGC->capStyle);
  CHECKGCVAL(GCJoinStyle, join_style, pGC->joinStyle);
  CHECKGCVAL(GCFillStyle, fill_style, pGC->fillStyle);
  CHECKGCVAL(GCFillRule, fill_rule, pGC->fillRule);

  if (mask & GCTile)
  {
    if (pGC->tileIsPixel)
    {
      mask &= ~GCTile;
    }
    else
    {
      if (nxagentDrawableStatus((DrawablePtr) pGC -> tile.pixmap) == NotSynchronized &&
              !nxagentGCTrap)
      {
        /*
         * If the tile is corrupted and is not too large, it can be
         * synchronized immediately. In the other cases, the tile is
         * cleared with a solid color to become usable. This approach
         * should solve the high delay on slow links waiting for a
         * background tile to be synchronized.
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
            !nxagentGCTrap)
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

  CHECKGCVAL(GCTileStipXOrigin, ts_x_origin, pGC->patOrg.x);
  CHECKGCVAL(GCTileStipYOrigin, ts_y_origin, pGC->patOrg.y);

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

  CHECKGCVAL(GCSubwindowMode, subwindow_mode, pGC->subWindowMode);
  CHECKGCVAL(GCGraphicsExposures, graphics_exposures, pGC->graphicsExposures);
  CHECKGCVAL(GCClipXOrigin, clip_x_origin, pGC->clipOrg.x);
  CHECKGCVAL(GCClipYOrigin, clip_y_origin, pGC->clipOrg.y);

  if (mask & GCClipMask)
  {
    /*
     * This is handled in the change clip.
     */

    mask &= ~GCClipMask;
  }

  CHECKGCVAL(GCDashOffset, dash_offset, pGC->dashOffset);

  if (mask & GCDashList)
  {
    mask &= ~GCDashList;

    if (!nxagentGCTrap)
    {
      XSetDashes(nxagentDisplay, nxagentGC(pGC),
                     pGC->dashOffset, (char *)pGC->dash, pGC->numInDashList);
    }
  }

  CHECKGCVAL(GCArcMode, arc_mode, pGC->arcMode);

  if (nxagentGCTrap)
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
              "at [%p] mask [%lu].\n", (void *)pGCSrc, (void *)pGCDst, mask);
  #endif

  /*
   * The MI function doesn't do anything.
   *
   * miCopyGC(pGCSrc, mask, pGCDst);
   */

  XCopyGC(nxagentDisplay, nxagentGC(pGCSrc), mask, nxagentGC(pGCDst));

  /*
   * Copy the private foreground field of the GC if GCForeground is
   * set.
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

void nxagentChangeClip(GCPtr pGC, int type, void * pValue, int nRects)
{
  Bool clipsMatch = False;

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
      RegionPtr pReg = RegionFromRects(nRects, (xRectangle *)pValue, type);
      clipsMatch = nxagentCompareRegions(pGC -> clientClip, pReg);
      RegionDestroy(pReg);
      break;
    }
    default:
    {
      clipsMatch = False;
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
      if (!clipsMatch && !nxagentGCTrap)
      {
        XSetClipMask(nxagentDisplay, nxagentGC(pGC), None);
      }
      break;
    }
    case CT_REGION:
    {
      if (!clipsMatch && !nxagentGCTrap)
      {
        XRectangle *pRects;
        nRects = RegionNumRects((RegionPtr)pValue);
        int size = nRects * sizeof(*pRects);
        pRects = (XRectangle *) malloc(size);
        BoxPtr pBox = RegionRects((RegionPtr)pValue);

        for (int i = nRects; i-- > 0;)
        {
          pRects[i].x = pBox[i].x1;
          pRects[i].y = pBox[i].y1;
          pRects[i].width = pBox[i].x2 - pBox[i].x1;
          pRects[i].height = pBox[i].y2 - pBox[i].y1;
        }

        XSetClipRectangles(nxagentDisplay, nxagentGC(pGC), pGC -> clipOrg.x, pGC -> clipOrg.y,
                               pRects, nRects, Unsorted);
        SAFE_free(pRects);
      }
      break;
    }
    case CT_PIXMAP:
    {
      if (!nxagentGCTrap)
      {
        XSetClipMask(nxagentDisplay, nxagentGC(pGC),
                         nxagentPixmap((PixmapPtr)pValue));
      }

      pGC->clientClip = (void *) (*pGC->pScreen->BitmapToRegion)((PixmapPtr) pValue);

      nxagentGCPriv(pGC)->pPixmap = (PixmapPtr)pValue;

      pValue = pGC->clientClip;

      type = CT_REGION;

      break;
    }
    case CT_UNSORTED:
    {
      if (!clipsMatch && !nxagentGCTrap)
      {    
        XSetClipRectangles(nxagentDisplay, nxagentGC(pGC),
                               pGC->clipOrg.x, pGC->clipOrg.y,
                                   (XRectangle *)pValue, nRects, Unsorted);
      }
      break;
    }
    case CT_YSORTED:
    {
      if (!clipsMatch && !nxagentGCTrap)
      {
        XSetClipRectangles(nxagentDisplay, nxagentGC(pGC),
                           pGC->clipOrg.x, pGC->clipOrg.y,
                           (XRectangle *)pValue, nRects, YSorted);
      }
      break;
    }
    case CT_YXSORTED:
    {
      if (!clipsMatch && !nxagentGCTrap)
      {
        XSetClipRectangles(nxagentDisplay, nxagentGC(pGC),
                           pGC->clipOrg.x, pGC->clipOrg.y,
                           (XRectangle *)pValue, nRects, YXSorted);
      }
      break;
    }
    case CT_YXBANDED:
    {
      if (!clipsMatch && !nxagentGCTrap)
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
       * Other parts of the server can only deal with CT_NONE,
       * CT_PIXMAP and CT_REGION client clips.
       */

      pGC->clientClip = (void *) RegionFromRects(nRects,
                                                  (xRectangle *)pValue, type);
      SAFE_free(pValue);

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

  if (!nxagentGCTrap)
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
      RegionDestroy(pGC->clientClip);
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
  #ifdef TEST
  fprintf(stderr, "nxagentCopyClip: Going to copy clip from GC [%p] to GC [%p]\n",
              (void *) pGCDst, (void *) pGCSrc);
  #endif

  switch (pGCSrc->clientClipType)
  {
    case CT_REGION:
      if (nxagentGCPriv(pGCSrc)->pPixmap == NULL)
      {
        RegionPtr pRgn = RegionCreate(NULL, 1);
        RegionCopy(pRgn, pGCSrc->clientClip);
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

  SAFE_free(t -> gc);
  SAFE_free(t);
}

static void nxagentRestoreGCRec(struct nxagentGCRec *t)
{
  #ifdef TEST
  fprintf(stderr, "nxagentRestoreGCRec: Freeing record at %p GC freed at %p.\n",
              (void*)t, (void*)t -> gc);
  #endif

  SAFE_free(nxagentGC(t -> pGC));

  nxagentGC(t -> pGC) = t -> gc;

  SAFE_free(t);
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

void nxagentFreeGCList(void)
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

static void nxagentRestoreGCList(void)
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

int nxagentDestroyNewGCResourceType(void * p, XID id)
{
  /*
   * Address of the destructor is set in Init.c.
   */

  #ifdef TEST
  fprintf(stderr, "nxagentDestroyNewGCResourceType: Destroying mirror id [%u] for GC at [%p].\n",
              nxagentGCPriv((GCPtr) p) -> mid, (void *) p);
  #endif

  nxagentGCPriv((GCPtr) p) -> mid = None;

  return 1;
}

#define SETGCVAL(mask, member, val) valuemask |= mask; values.member = (val)

static void nxagentReconnectGC(void *param0, XID param1, void * param2)
{
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

  XGCValues values = {0};
  unsigned long valuemask = 0;
  SETGCVAL(GCFunction, function, pGC->alu);
  SETGCVAL(GCPlaneMask, plane_mask, pGC->planemask);
  SETGCVAL(GCForeground, foreground, nxagentPixel(pGC->fgPixel));
  SETGCVAL(GCBackground, background, nxagentPixel(pGC->bgPixel));
  SETGCVAL(GCLineWidth, line_width, pGC->lineWidth);
  SETGCVAL(GCLineStyle, line_style, pGC->lineStyle);
  SETGCVAL(GCCapStyle, cap_style, pGC->capStyle);
  SETGCVAL(GCJoinStyle, join_style, pGC->joinStyle);
  SETGCVAL(GCFillStyle, fill_style, pGC->fillStyle);
  SETGCVAL(GCFillRule, fill_rule, pGC->fillRule);

  if (!pGC -> tileIsPixel && (pGC -> tile.pixmap != NULL))
  {
    if (nxagentPixmapIsVirtual(pGC -> tile.pixmap))
    {
      SETGCVAL(GCTile, tile, nxagentPixmap(nxagentRealPixmap(pGC -> tile.pixmap)));
    }
    else
    {
      SETGCVAL(GCTile, tile, nxagentPixmap(pGC -> tile.pixmap));
    }
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

      SETGCVAL(GCStipple, stipple, nxagentPixmap(nxagentRealPixmap(pGC -> stipple)));
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

      SETGCVAL(GCStipple, stipple, nxagentPixmap(pGC->stipple));
    }
  }

  SETGCVAL(GCTileStipXOrigin, ts_x_origin, pGC->patOrg.x);
  SETGCVAL(GCTileStipYOrigin, ts_y_origin, pGC->patOrg.y);

  if (pGC->font != NULL)
  {
    SETGCVAL(GCFont, font, nxagentFont(pGC->font));
  }

  SETGCVAL(GCSubwindowMode, subwindow_mode, pGC->subWindowMode);
  SETGCVAL(GCGraphicsExposures, graphics_exposures, pGC->graphicsExposures);
  SETGCVAL(GCClipXOrigin, clip_x_origin, pGC->clipOrg.x);
  SETGCVAL(GCClipYOrigin, clip_y_origin, pGC->clipOrg.y);
  valuemask |= GCClipMask;  /* FIXME: where's the ClipMask pixmap?? */
  SETGCVAL(GCDashOffset, dash_offset, pGC->dashOffset);

  if (pGC->dash != NULL)
  {
    SETGCVAL(GCDashList, dashes, *pGC->dash);
  }

  SETGCVAL(GCArcMode, arc_mode, pGC->arcMode);

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
  Bool GCSuccess = True;

  #ifdef DEBUG
  fprintf(stderr, "nxagentReconnectAllGCs\n");
  #endif

  /*
   * The resource type RT_NX_GC is created on the server client only,
   * so we can avoid to loop through all the clients.
   */

  FindClientResourcesByType(clients[serverClient -> index], RT_NX_GC, nxagentReconnectGC, &GCSuccess);

  for (int cid = 0; (cid < MAXCLIENTS) && GCSuccess; cid++)
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

void nxagentDisconnectGC(void * p0, XID x1, void * p2)
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

Bool nxagentDisconnectAllGCs(void)
{
  Bool success = True;

  #ifdef DEBUG
  fprintf(stderr, "nxagentDisconnectAllGCs\n");
  #endif

  /*
   * The resource type RT_NX_GC is created on the server client only,
   * so we can avoid to loop through all the clients.
   */

  FindClientResourcesByType(clients[serverClient -> index], RT_NX_GC,
                                (FindResType) nxagentDisconnectGC, &success);

  for (int cid = 0; (cid < MAXCLIENTS) && success; cid++)
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

static void nxagentReconnectClip(GCPtr pGC, int type, void * pValue, int nRects)
{
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
        nRects = RegionNumRects((RegionPtr)pValue);
        int size = nRects * sizeof(XRectangle *);
        XRectangle *pRects = (XRectangle *) malloc(size);
        BoxPtr pBox = RegionRects((RegionPtr)pValue);
        for (int i = nRects; i-- > 0;) {
          pRects[i].x = pBox[i].x1;
          pRects[i].y = pBox[i].y1;
          pRects[i].width = pBox[i].x2 - pBox[i].x1;
          pRects[i].height = pBox[i].y2 - pBox[i].y1;
        }

        /*
         * Originally, the clip origin area were 0,0 but it didn't
         * work with kedit and family, because it got the clip mask of
         * the pixmap all traslated.
         */

        XSetClipRectangles(nxagentDisplay, nxagentGC(pGC), pGC -> clipOrg.x, pGC -> clipOrg.y,
                           pRects, nRects, Unsorted);
        SAFE_free(pRects);
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

      pGC->clientClip = (void *) (*pGC->pScreen->BitmapToRegion)((PixmapPtr) pValue);

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
       * other parts of server can only deal with CT_NONE, CT_PIXMAP
       * and CT_REGION client clips.
       */

      pGC->clientClip = (void *) RegionFromRects(nRects,
                                                  (xRectangle *)pValue, type);
      SAFE_free(pValue);
      pValue = pGC->clientClip;
      type = CT_REGION;

      break;
    }

  pGC->clientClipType = type;
  pGC->clientClip = pValue;

 nxagentGCPriv(pGC)->nClipRects = nRects;
}

static Bool nxagentCompareRegions(RegionPtr r1, RegionPtr r2)
{
 /*
  *  It returns True if regions are equal, False otherwise
  */

  if (r1 == NULL && r2 == NULL)
  {
    return True;
  }

  if ((r1 == NULL) || (r2 == NULL))
  {
    return False;
  }

  if (RegionNumRects(r1) !=  RegionNumRects(r2))
  {
    return False;
  }
  else if (RegionNumRects(r1) == 0)
  {
    return True;
  }
  else if ((*RegionExtents(r1)).x1 !=  (*RegionExtents(r2)).x1) return 0;
  else if ((*RegionExtents(r1)).x2 !=  (*RegionExtents(r2)).x2) return 0;
  else if ((*RegionExtents(r1)).y1 !=  (*RegionExtents(r2)).y1) return 0;
  else if ((*RegionExtents(r1)).y2 !=  (*RegionExtents(r2)).y2) return 0;
  else
  {
    for (int i = 0; i < RegionNumRects(r1); i++)
    {
      if (RegionRects(r1)[i].x1 !=  RegionRects(r2)[i].x1) return 0;
      else if (RegionRects(r1)[i].x2 !=  RegionRects(r2)[i].x2) return 0;
      else if (RegionRects(r1)[i].y1 !=  RegionRects(r2)[i].y1) return 0;
      else if (RegionRects(r1)[i].y2 !=  RegionRects(r2)[i].y2) return 0;
    }
  }
  return True;
}

/*
 * This function have to be called in the place of GetScratchGC if the
 * GC will be used to perform operations also on the remote X Server.
 * This is why we call the XChangeGC at the end of the function.
 */
GCPtr nxagentGetScratchGC(unsigned depth, ScreenPtr pScreen)
{
  /*
   * The GC trap is temporarily disabled in order to allow the remote
   * clipmask reset requested by GetScratchGC().
   */

  int nxagentSaveGCTrap = nxagentGCTrap;

  nxagentGCTrap = False;

  GCPtr pGC = GetScratchGC(depth, pScreen);

  nxagentGCTrap = nxagentSaveGCTrap;

  if (pGC == NULL)
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentGetScratchGC: Failed to retrieve the scratch GC.\n");
    #endif

    return NULL;
  }

  unsigned long valuemask = 0;
  XGCValues values = {0};
  SETGCVAL(GCFunction, function, pGC -> alu);
  SETGCVAL(GCPlaneMask, plane_mask, pGC -> planemask);
  SETGCVAL(GCForeground, foreground, nxagentPixel(pGC -> fgPixel));
  SETGCVAL(GCBackground, background, nxagentPixel(pGC -> bgPixel));
  SETGCVAL(GCLineWidth, line_width, pGC -> lineWidth);
  SETGCVAL(GCLineStyle, line_style, pGC -> lineStyle);
  SETGCVAL(GCCapStyle, cap_style, pGC -> capStyle);
  SETGCVAL(GCJoinStyle, join_style, pGC -> joinStyle);
  SETGCVAL(GCFillStyle, fill_style, pGC -> fillStyle);
  SETGCVAL(GCFillRule, fill_rule, pGC -> fillRule);
  SETGCVAL(GCArcMode, arc_mode, pGC -> arcMode);
  SETGCVAL(GCTileStipXOrigin, ts_x_origin, pGC -> patOrg.x);
  SETGCVAL(GCTileStipYOrigin, ts_y_origin, pGC -> patOrg.y);
  SETGCVAL(GCSubwindowMode, subwindow_mode, pGC -> subWindowMode);
  SETGCVAL(GCGraphicsExposures, graphics_exposures, pGC -> graphicsExposures);
  SETGCVAL(GCClipXOrigin, clip_x_origin, pGC -> clipOrg.x);
  SETGCVAL(GCClipYOrigin, clip_y_origin, pGC -> clipOrg.y);

  /* The GCClipMask is set to none inside the GetScratchGC() function. */

  /* FIXME: What about GCDashOffset? */

  XChangeGC(nxagentDisplay, nxagentGC(pGC), valuemask, &values);

  memset(&(nxagentGCPriv(pGC) -> lastServerValues), 0, sizeof(XGCValues));

  return pGC;
}

/*
 * This function is only a wrapper for FreeScratchGC.
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
 * The GCs belonging to this list are used only in the synchronization
 * put images, to be sure they preserve the default values and to
 * avoid XChangeGC() requests.
 */

GCPtr nxagentGetGraphicContext(DrawablePtr pDrawable)
{
  for (int i = 0; i < nxagentGraphicContextsSize; i++)
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

        int result = 1;

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
  /*
   * We have not found a GC, so we have to spread the list and add a
   * new GC.
   */

  nxagentGraphicContextsPtr nxagentGCs = realloc(nxagentGraphicContexts, (nxagentGraphicContextsSize + 1) * sizeof(nxagentGraphicContextsRec));
   
  if (nxagentGCs == NULL)
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentCreateGraphicContext: Cannot allocate memory for a GC.\n");
    #endif

    return NULL;
  }

  nxagentGraphicContexts = nxagentGCs;

  GCPtr pGC = CreateScratchGC(nxagentDefaultScreen, depth);

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

  XID attributes[2];

  attributes[0] = 0xc1c1c1;

  if (depth == 15 || depth == 16)
  {
    Color32to16(attributes[0]);
  }

  /*
   * The IncludeInferiors property is useful to solve problems when
   * synchronizing windows covered by an invisible child.
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
 * This initialization is called in the InitOutput() function
 * immediately after opening the screen, which is used to create the
 * GCs.
 */

void nxagentAllocateGraphicContexts(void)
{
  int *depths = nxagentDepths;

  for (int i = 0; i < nxagentNumDepths; i++)
  {
    nxagentCreateGraphicContext(*depths);
    depths++;
  }
}

void nxagentDisconnectGraphicContexts(void)
{
  for (int i = 0; i < nxagentGraphicContextsSize; i++)
  {
    nxagentGraphicContexts[i].dirty = 1;
  }

  return;
}

