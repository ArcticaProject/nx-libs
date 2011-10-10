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
#include "resource.h"
#include "dixstruct.h"
#include "../../fb/fb.h"

#include "Agent.h"
#include "Composite.h"
#include "Display.h"
#include "Visual.h"
#include "Drawable.h"
#include "Pixmaps.h"
#include "GCs.h"
#include "Image.h"
#include "Font.h"
#include "Events.h"
#include "Client.h"
#include "Trap.h"
#include "Holder.h"
#include "Args.h"
#include "Screen.h"

#include "NXlib.h"

/*
 * Set here the required log level.
 */

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG
#undef  DUMP

/*
 * Temporarily set/reset the trap.
 */

static int nxagentSaveGCTrap;

#define SET_GC_TRAP() \
{ \
  nxagentSaveGCTrap = nxagentGCTrap;\
\
  nxagentGCTrap = 1; \
}

#define RESET_GC_TRAP() \
{ \
  nxagentGCTrap = nxagentSaveGCTrap; \
}
 
/*
 * This is currently unused.
 */

RegionPtr nxagentBitBlitHelper(GC *pGC);

/*
 * The NX agent implementation of the
 * X server's graphics functions.
 */

void nxagentFillSpans(DrawablePtr pDrawable, GCPtr pGC, int nSpans,
                          xPoint *pPoints, int *pWidths, int fSorted)
{
  if ((pDrawable)->type == DRAWABLE_PIXMAP)
  {
    #ifdef TEST
    fprintf(stderr, "GCOps: GC [%p] going to FillSpans on FB pixmap [%p].\n",
                (void *) pGC, (void *) nxagentVirtualDrawable(pDrawable));
    #endif

    fbFillSpans(nxagentVirtualDrawable(pDrawable), pGC, nSpans, pPoints, pWidths, fSorted);
  }
  else
  {
    fbFillSpans(pDrawable, pGC, nSpans, pPoints, pWidths, fSorted);
  }
}

void nxagentSetSpans(DrawablePtr pDrawable, GCPtr pGC, char *pSrc,
                         xPoint *pPoints, int *pWidths, int nSpans, int fSorted)
{
  if ((pDrawable)->type == DRAWABLE_PIXMAP)
  {
    #ifdef TEST
    fprintf(stderr, "GCOps: GC [%p] going to SetSpans on FB pixmap [%p].\n",
                (void *) pGC, (void *) nxagentVirtualDrawable(pDrawable));
    #endif

    fbSetSpans(nxagentVirtualDrawable(pDrawable), pGC, pSrc, pPoints, pWidths, nSpans, fSorted);
  }
  else
  {
    fbSetSpans(pDrawable, pGC, pSrc, pPoints, pWidths, nSpans, fSorted);
  } 
}

void nxagentGetSpans(DrawablePtr pDrawable, int maxWidth, xPoint *pPoints,
                         int *pWidths, int nSpans, char *pBuffer)
{
  if ((pDrawable)->type == DRAWABLE_PIXMAP)
  {
    #ifdef TEST
    fprintf(stderr, "GCOps: going to GetSpans on FB pixmap [%p].\n",
                (void *) nxagentVirtualDrawable(pDrawable));
    #endif

    fbGetSpans(nxagentVirtualDrawable(pDrawable), maxWidth, pPoints, pWidths, nSpans, pBuffer);
  }
  else
  {
    fbGetSpans(pDrawable, maxWidth, pPoints, pWidths, nSpans, pBuffer);
  }
}

void nxagentQueryBestSize(int class, unsigned short *pwidth,
                              unsigned short *pheight, ScreenPtr pScreen)
{
  unsigned width, test;

  switch(class)
  {
    case CursorShape:
      if (*pwidth > pScreen->width)
        *pwidth = pScreen->width;
      if (*pheight > pScreen->height)
        *pheight = pScreen->height;
      break;
    case TileShape:
    case StippleShape:
      width = *pwidth;
      if (!width) break;
      /* Return the closes power of two not less than what they gave me */
      test = 0x80000000;
      /* Find the highest 1 bit in the width given */
      while(!(test & width))
         test >>= 1;
      /* If their number is greater than that, bump up to the next
       *  power of two */
      if((test - 1) & width)
         test <<= 1;
      *pwidth = test;
      /* We don't care what height they use */
      break;
  }
}

RegionPtr nxagentBitBlitHelper(GC *pGC)
{
  #ifdef TEST
  fprintf(stderr, "nxagentBitBlitHelper: Called for GC at [%p].\n", (void *) pGC);
  #endif

  /*
   * Force NullRegion. We consider enough the graphics
   * expose events generated internally by the nxagent
   * server.
   */

  #ifdef TEST
  fprintf(stderr, "nxagentBitBlitHelper: WARNING! Skipping check on exposures events.\n");
  #endif

  return NullRegion;
}

/*
 * The deferring of X_RenderCompositeTrapezoids caused
 * an ugly effect on pulldown menu: as the background
 * may be not synchronized, the text floats in an invi-
 * sible window. To avoid such effects, we use a system
 * to guess if the destination target of a copy area
 * is a popup, by assuming that those kind of windows
 * use the override redirect property.
 */

int nxagentWindowIsPopup(DrawablePtr pDrawable)
{
  WindowPtr parent;

  int windowIsPopup;
  int level;

  if (pDrawable -> type != DRAWABLE_WINDOW)
  {
    return 0;
  }

  windowIsPopup = 0;

  if (((WindowPtr) pDrawable) -> overrideRedirect == 1)
  {
    windowIsPopup = 1;
  }
  else
  {
    parent = ((WindowPtr) pDrawable) -> parent;

    /*
     * Go up on the tree until a parent
     * exists or 4 windows has been che-
     * cked. This seems a good limit to
     * up children's popup.
     */

    level = 0;

    while (parent != NULL && ++level <= 4)
    {
      #ifdef DEBUG
      fprintf(stderr, "nxagentWindowIsPopup: Window [%p] has parent [%p] in tree with OverrideRedirect [%d] "
                  " Level [%d].\n", (void *) pDrawable, (void *) parent, parent -> overrideRedirect, level);
      #endif

      if (parent -> overrideRedirect == 1)
      {
        windowIsPopup = 1;

        break;
      }

      parent = parent -> parent;
    }
  }

  #ifdef TEST
  fprintf(stderr, "nxagentWindowIsPopup: Window [%p] %s to be a popup.\n", (void *) pDrawable,
              windowIsPopup == 1 ? "seems" : "does not seem");
  #endif

  return windowIsPopup;
}

/*
 * This function returns 1 if the
 * XCopyArea request must be skipped.
 */

int nxagentDeferCopyArea(DrawablePtr pSrcDrawable, DrawablePtr pDstDrawable,
                            GCPtr pGC, int srcx, int srcy, int width,
                                int height, int dstx, int dsty)
{
  RegionPtr pSrcRegion;
  RegionPtr pClipRegion, pCorruptedRegion;
  RegionRec corruptedRegion, tmpRegion;

  /*
   * If the destination drawable is a popup
   * window, we try to synchronize the source
   * drawable to show a nice menu. Anyway if
   * this synchronization breaks, the copy area
   * is handled in the normal way.
   */

/*
FIXME: The popup could be synchronized with one
       single put image, clipped to the corrup-
       ted region. As an intermediate step, the
       pixmap to synchronize could be copied on
       a cleared scratch pixmap, in order to
       have a solid color in the clipped regions.
*/

  if (nxagentOption(DeferLevel) >= 2 &&
          pSrcDrawable -> type == DRAWABLE_PIXMAP &&
              nxagentPixmapContainTrapezoids((PixmapPtr) pSrcDrawable) == 1 &&
                  nxagentWindowIsPopup(pDstDrawable) == 1)
  {
    pSrcRegion = nxagentCreateRegion(pSrcDrawable, NULL, srcx, srcy, width, height);

    #ifdef DEBUG
    fprintf(stderr, "nxagentDeferCopyArea: Copying to a popup menu. Source region [%d,%d,%d,%d].\n",
                pSrcRegion -> extents.x1, pSrcRegion -> extents.y1,
                    pSrcRegion -> extents.x2, pSrcRegion -> extents.y2);
    #endif

    REGION_INIT(pSrcDrawable -> pScreen, &corruptedRegion, NullBox, 1);

    REGION_INTERSECT(pSrcDrawable -> pScreen, &corruptedRegion,
                         pSrcRegion, nxagentCorruptedRegion(pSrcDrawable));

    if (REGION_NIL(&corruptedRegion) == 0)
    {
      #ifdef TEST
      fprintf(stderr, "nxagentDeferCopyArea: Forcing the synchronization of source drawable at [%p].\n",
                  (void *) pSrcDrawable);
      #endif

      nxagentSynchronizeRegion(pSrcDrawable, &corruptedRegion, EVENT_BREAK, NULL);
    }

    REGION_UNINIT(pSrcDrawable -> pScreen, &corruptedRegion);

    nxagentFreeRegion(pSrcDrawable, pSrcRegion);

    if (nxagentDrawableStatus(pSrcDrawable) == Synchronized)
    {
      return 0;
    }
  }

  /*
   * We are going to decide if the source drawable
   * must be synchronized before using it, or if
   * the copy will be clipped to the synchronized
   * source region.
   */

  if ((pDstDrawable -> type == DRAWABLE_PIXMAP &&
          nxagentOption(DeferLevel) > 0) || nxagentOption(DeferLevel) >= 2)
  {
    pClipRegion = nxagentCreateRegion(pSrcDrawable, NULL, srcx, srcy,
                                          width, height);

    /*
     * We called this variable pCorruptedRegion
     * because in the worst case the corrupted
     * region will be equal to the destination
     * region. The GC's clip mask is used to
     * narrow the destination.
     */

    pCorruptedRegion = nxagentCreateRegion(pDstDrawable, pGC, dstx, dsty,
                                               width, height);

    #ifdef DEBUG
    fprintf(stderr, "nxagentDeferCopyArea: Copy area source region is [%d,%d,%d,%d].\n",
                pClipRegion -> extents.x1, pClipRegion -> extents.y1,
                    pClipRegion -> extents.x2, pClipRegion -> extents.y2);
    #endif

    REGION_SUBTRACT(pSrcDrawable -> pScreen, pClipRegion, pClipRegion, nxagentCorruptedRegion(pSrcDrawable));

    #ifdef DEBUG
    fprintf(stderr, "nxagentDeferCopyArea: Usable copy area source region is [%d,%d,%d,%d].\n",
                pClipRegion -> extents.x1, pClipRegion -> extents.y1,
                    pClipRegion -> extents.x2, pClipRegion -> extents.y2);
    #endif

    if (pGC -> clientClip == NULL || pGC -> clientClipType != CT_REGION)
    {
      #ifdef WARNING

      if (pGC -> clientClipType != CT_NONE)
      {
        fprintf(stderr, "nxagentDeferCopyArea: WARNING! pGC [%p] has a clip type [%d].\n",
                    (void *) pGC, pGC -> clientClipType);
      }

      #endif

      REGION_TRANSLATE(pSrcDrawable -> pScreen, pClipRegion, dstx - srcx, dsty - srcy);
    }
    else
    {
      REGION_INIT(pDstDrawable -> pScreen, &tmpRegion, NullBox, 1);

      #ifdef DEBUG
      fprintf(stderr, "nxagentDeferCopyArea: Going to modify the original GC [%p] with clip mask "
                  "[%d,%d,%d,%d] and origin [%d,%d].\n",
                      (void *) pGC,
                      ((RegionPtr) pGC -> clientClip) -> extents.x1, ((RegionPtr) pGC -> clientClip) -> extents.y1,
                      ((RegionPtr) pGC -> clientClip) -> extents.x2, ((RegionPtr) pGC -> clientClip) -> extents.y2,
                      pGC -> clipOrg.x, pGC -> clipOrg.y);
      #endif

      REGION_COPY(pDstDrawable -> pScreen, &tmpRegion, (RegionPtr) pGC -> clientClip);

      if (pGC -> clipOrg.x != 0 || pGC -> clipOrg.y != 0)
      {
        REGION_TRANSLATE(pDstDrawable -> pScreen, &tmpRegion, pGC -> clipOrg.x, pGC -> clipOrg.y);
      }

      REGION_TRANSLATE(pSrcDrawable -> pScreen, pClipRegion, dstx - srcx, dsty - srcy);

      REGION_INTERSECT(pSrcDrawable -> pScreen, pClipRegion, &tmpRegion, pClipRegion);

      REGION_UNINIT(pSrcDrawable -> pScreen, &tmpRegion);
    }

    /*
     * The corrupted region on the destination
     * drawable is composed by the areas of the
     * destination that we are not going to copy.
     */

    REGION_SUBTRACT(pSrcDrawable -> pScreen, pCorruptedRegion, pCorruptedRegion, pClipRegion);

    #ifdef DEBUG
    fprintf(stderr, "nxagentDeferCopyArea: Recomputed clip region is [%d,%d,%d,%d][%ld].\n",
                pClipRegion -> extents.x1, pClipRegion -> extents.y1,
                    pClipRegion -> extents.x2, pClipRegion -> extents.y2,
                        REGION_NUM_RECTS(pClipRegion));

    fprintf(stderr, "nxagentDeferCopyArea: Inherited corrupted region is [%d,%d,%d,%d][%ld].\n",
                pCorruptedRegion -> extents.x1, pCorruptedRegion -> extents.y1,
                    pCorruptedRegion -> extents.x2, pCorruptedRegion -> extents.y2,
                        REGION_NUM_RECTS(pCorruptedRegion));
    #endif

    /*
     * The destination drawable inherits both the
     * synchronized and the corrupted region.
     */

    if (REGION_NIL(pClipRegion) == 0)
    {
      nxagentUnmarkCorruptedRegion(pDstDrawable, pClipRegion);
    }

    if (REGION_NIL(pCorruptedRegion) == 0)
    {
      nxagentMarkCorruptedRegion(pDstDrawable, pCorruptedRegion);
    }

    if (REGION_NIL(pClipRegion) == 0)
    {
      GCPtr  targetGC;

      CARD32 targetAttributes[2];

      Bool pClipRegionFree = True;

      /*
       * As we want to copy only the synchronized
       * areas of the source drawable, we create
       * a new GC copying the original one and
       * setting a new clip mask.
       */

      targetGC = GetScratchGC(pDstDrawable -> depth, pDstDrawable -> pScreen);

      ValidateGC(pDstDrawable, targetGC);

      CopyGC(pGC, targetGC, GCFunction | GCPlaneMask | GCSubwindowMode |
                 GCClipXOrigin | GCClipYOrigin | GCClipMask | GCForeground |
                     GCBackground | GCGraphicsExposures);

      if (REGION_NUM_RECTS(pClipRegion) == 1)
      {
        /*
         * If the region to copy is formed by one
         * rectangle, we change only the copy coor-
         * dinates.
         */

         srcx = srcx + pClipRegion -> extents.x1 - dstx;
         srcy = srcy + pClipRegion -> extents.y1 - dsty;

         dstx = pClipRegion -> extents.x1;
         dsty = pClipRegion -> extents.y1;

         width  = pClipRegion -> extents.x2 - pClipRegion -> extents.x1;
         height = pClipRegion -> extents.y2 - pClipRegion -> extents.y1;
      }
      else
      {
        /*
         * Setting the clip mask origin. This
         * operation must precede the clip chan-
         * ge, because the origin information is
         * used in the XSetClipRectangles().
         */

        targetAttributes[0] = 0;
        targetAttributes[1] = 0;

        ChangeGC(targetGC, GCClipXOrigin | GCClipYOrigin, targetAttributes);

        /*
         * Setting the new clip mask.
         */

        nxagentChangeClip(targetGC, CT_REGION, pClipRegion, 0);

        /*
         * Next call to nxagentChangeClip() will destroy
         * pClipRegion, so it has not to be freed.
         */

        pClipRegionFree = False;

        #ifdef DEBUG
        fprintf(stderr, "nxagentDeferCopyArea: Going to execute a copy area with clip mask "
                    "[%d,%d,%d,%d] and origin [%d,%d].\n", ((RegionPtr) targetGC -> clientClip) -> extents.x1,
                        ((RegionPtr) targetGC -> clientClip) -> extents.y1,
                            ((RegionPtr) targetGC -> clientClip) -> extents.x2,
                                ((RegionPtr) targetGC -> clientClip) -> extents.y2,
                                    targetGC -> clipOrg.x, targetGC -> clipOrg.y);
        #endif
      }

      XCopyArea(nxagentDisplay, nxagentDrawable(pSrcDrawable), nxagentDrawable(pDstDrawable),
                    nxagentGC(targetGC), srcx, srcy, width, height, dstx, dsty);

      nxagentChangeClip(targetGC, CT_NONE, NullRegion, 0);

      if (pClipRegionFree == True)
      {
        nxagentFreeRegion(pSrcDrawable, pClipRegion);
      }

      FreeScratchGC(targetGC);
    }
    else
    {
      #ifdef TEST
      fprintf(stderr, "nxagentDeferCopyArea: The clipped region is NIL. CopyArea skipped.\n");
      #endif

      /*
       * The pClipRegion is destroyed calling nxagentChangeClip(),
       * so we deallocate it explicitly only if we don't change
       * the clip.
       */

      nxagentFreeRegion(pSrcDrawable, pClipRegion);
    }

    nxagentFreeRegion(pSrcDrawable, pCorruptedRegion);

    return 1;
  }
  else
  {
    pSrcRegion = nxagentCreateRegion(pSrcDrawable, NULL, srcx, srcy, width, height);

    #ifdef DEBUG
    fprintf(stderr, "nxagentDeferCopyArea: Source region [%d,%d,%d,%d].\n",
                pSrcRegion -> extents.x1, pSrcRegion -> extents.y1,
                    pSrcRegion -> extents.x2, pSrcRegion -> extents.y2);
    #endif

    REGION_INIT(pSrcDrawable -> pScreen, &corruptedRegion, NullBox, 1);

    REGION_INTERSECT(pSrcDrawable -> pScreen, &corruptedRegion,
                         pSrcRegion, nxagentCorruptedRegion(pSrcDrawable));

    if (REGION_NIL(&corruptedRegion) == 0)
    {
      #ifdef TEST
      fprintf(stderr, "nxagentDeferCopyArea: Forcing the synchronization of source drawable at [%p].\n",
                  (void *) pSrcDrawable);
      #endif

      nxagentSynchronizeRegion(pSrcDrawable, &corruptedRegion /*pSrcRegion*/, NEVER_BREAK, NULL);
    }

    REGION_UNINIT(pSrcDrawable -> pScreen, &corruptedRegion);

    nxagentFreeRegion(pSrcDrawable, pSrcRegion);
  }

  return 0;
}

RegionPtr nxagentCopyArea(DrawablePtr pSrcDrawable, DrawablePtr pDstDrawable,
                               GCPtr pGC, int srcx, int srcy, int width,
                                   int height, int dstx, int dsty)
{
  int leftPad = 0;
  unsigned int format;
  unsigned long planeMask = 0xffffffff;

  int oldDstxyValue;

  RegionPtr pDstRegion;

  int skip = 0;

  #ifdef TEST
  fprintf(stderr, "nxagentCopyArea: Image src [%s:%p], dst [%s:%p] (%d,%d) -> (%d,%d) size (%d,%d)\n",
              (pSrcDrawable -> type == DRAWABLE_PIXMAP) ? "PIXMAP" : "WINDOW", (void *) pSrcDrawable,
                      (pDstDrawable -> type == DRAWABLE_PIXMAP) ? "PIXMAP" : "WINDOW", 
                          (void *) pDstDrawable, srcx, srcy, dstx, dsty, width, height);
  #endif

 /*
  * Here, before using fbDoCopy() called by fbCopyArea(),
  * it should be provided that the cast in fbDoCopy() from
  * int to short int would not cut off significative bits.
  */

  if (dstx + pDstDrawable->x + width > 32767)
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentCopyArea: x2 exceeding short int.\n");
    #endif

    width = 32767 - dstx - pDstDrawable->x;

    if (width <= 0)
    {
      #ifdef TEST
      fprintf(stderr, "nxagentCopyArea: Returning null on x2 check.\n");
      #endif

      return NullRegion;
    }
  }

  if (dstx + pDstDrawable->x < -32768)
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentCopyArea: x1 exceeding short int.\n");
    #endif

    width += pDstDrawable->x + dstx + 32768;
    srcx  -= pDstDrawable->x + dstx + 32768;
    dstx = -32768 - pDstDrawable->x;

    if (width <= 0)
    {
      #ifdef TEST
      fprintf(stderr, "nxagentCopyArea: Returning null on x1 check.\n");
      #endif

      return NullRegion;
    }
  }

    oldDstxyValue = dsty;

  if (dsty + pDstDrawable->y + height > 32767)
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentCopyArea: y2 exceeding short int.\n");
    #endif

    height = 32767 - dsty - pDstDrawable->y;

    if (height <= 0)
    {
      #ifdef TEST
      fprintf(stderr, "nxagentCopyArea: Returning null on y2 check.\n");
      #endif

      return NullRegion;
    }
  }

  if (dsty + pDstDrawable->y < -32768)
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentCopyArea: y1 exceeding short int.\n");
    #endif

    height += 32768 + pDstDrawable->y + dsty;
    srcy   -= 32768 + pDstDrawable->y + dsty;
    dsty = -32768 - pDstDrawable->y;

    if (height <= 0)
    {
      #ifdef TEST
      fprintf(stderr, "nxagentCopyArea: Returning null on y1 check.\n");
      #endif

      return NullRegion;
    }
  }


  if (nxagentGCTrap == 1 || nxagentShmTrap == 1)
  {
    if (pSrcDrawable -> type == DRAWABLE_PIXMAP &&
            pDstDrawable -> type == DRAWABLE_PIXMAP)
    {
      return fbCopyArea(nxagentVirtualDrawable(pSrcDrawable), 
                          nxagentVirtualDrawable(pDstDrawable),
                            pGC, srcx, srcy, width, height, dstx, dsty);
    }
    else if (pSrcDrawable -> type == DRAWABLE_PIXMAP)
    {
      return fbCopyArea(nxagentVirtualDrawable(pSrcDrawable), pDstDrawable,
                            pGC, srcx, srcy, width, height, dstx, dsty);
    }
    else if (pDstDrawable -> type == DRAWABLE_PIXMAP)
    {
      return fbCopyArea(pSrcDrawable, nxagentVirtualDrawable(pDstDrawable),
                            pGC, srcx, srcy, width, height, dstx, dsty);
    }
    else
    {
      return fbCopyArea(pSrcDrawable, pDstDrawable,
                            pGC, srcx, srcy, width, height, dstx, dsty);
    }

    return NullRegion;
  }

  /*
   * Try to detect if the copy area is to a window
   * that is unmapped or fully covered. Similarly
   * to the check in Image.c, this is of little use.
   */

  if (nxagentOption(IgnoreVisibility) == 0 && pDstDrawable -> type == DRAWABLE_WINDOW &&
          (nxagentWindowIsVisible((WindowPtr) pDstDrawable) == 0 ||
              (nxagentDefaultWindowIsVisible() == 0 && nxagentCompositeEnable == 0)))
  {
    #ifdef TEST
    fprintf(stderr, "nxagentCopyArea: Prevented operation on fully obscured window at [%p].\n",
                (void *) pDstDrawable);
    #endif

    return NullRegion;
  }

  /*
   * If the pixmap is on shared memory, we can't
   * know if the pixmap content is changed and
   * so have to translate the operation in a put
   * image operation. This can seriously affect
   * the performance.
   */

  if (pSrcDrawable -> type == DRAWABLE_PIXMAP &&
         nxagentIsShmPixmap((PixmapPtr) pSrcDrawable))
  {
    char *data;
    int depth, length;

    depth  = pSrcDrawable -> depth;

    format = (depth == 1) ? XYPixmap : ZPixmap;

    length = nxagentImageLength(width, height, format, leftPad, depth);

    if ((data = xalloc(length)) == NULL)
    {
      #ifdef WARNING
      fprintf(stderr, "nxagentCopyArea: WARNING! Failed to allocate memory for the operation.\n");
      #endif

      return NullRegion;
    }

    fbGetImage(nxagentVirtualDrawable(pSrcDrawable), srcx, srcy, width, height, format, planeMask, data);

    /*
     * If the source is a shared memory pixmap,
     * put the image directly to the destination.
     */

    nxagentPutImage(pDstDrawable, pGC, depth, dstx, dsty,
                        width, height, leftPad, format, data);

    #ifdef TEST
    fprintf(stderr,"nxagentCopyArea: Realize Pixmap %p virtual %p x %d y %d w %d h %d\n",
               (void *) pSrcDrawable, (void *) nxagentVirtualDrawable(pSrcDrawable),
                   srcx, srcy, width, height);
    #endif

    xfree(data);

    /*
     * If the source is a shared memory pixmap, the
     * content of the framebuffer has been placed
     * directly on the destination so we can skip
     * the copy area operation.
     */

    skip = 1;
  }

  #ifdef TEST
  fprintf(stderr, "nxagentCopyArea: Image src [%s:%p], dst [%s:%p] sx %d sy %d dx %d dy %d size w %d h %d\n",
              ((pSrcDrawable)->type == DRAWABLE_PIXMAP) ? "PIXMAP" : "WINDOW", (void *) pSrcDrawable,
                   ((pDstDrawable)->type == DRAWABLE_PIXMAP) ? "PIXMAP" : "WINDOW",
                       (void *) pDstDrawable, srcx, srcy, dstx, dsty, width, height);
  #endif

  if (skip == 0 && nxagentDrawableStatus(pSrcDrawable) == NotSynchronized)
  {
    skip = nxagentDeferCopyArea(pSrcDrawable, pDstDrawable, pGC, srcx, srcy,
                                   width, height, dstx, dsty);
  }
  #ifdef TEST
  else
  {
    fprintf(stderr, "nxagentCopyArea: Source drawable at [%p] already synchronized.\n",
                (void *) pSrcDrawable);
  }
  #endif

  if (skip == 0)
  {
    XCopyArea(nxagentDisplay, nxagentDrawable(pSrcDrawable), nxagentDrawable(pDstDrawable),
                  nxagentGC(pGC), srcx, srcy, width, height, dstx, dsty);

    /*
     * The copy area restored the synchroni-
     * zation status of destination drawable.
     */

    if (nxagentDrawableStatus(pDstDrawable) == NotSynchronized)
    {
      pDstRegion = nxagentCreateRegion(pDstDrawable, pGC, dstx, dsty, width, height);

      nxagentUnmarkCorruptedRegion(pDstDrawable, pDstRegion);

      nxagentFreeRegion(pDstDrawable, pDstRegion);
    }
  }

  if (nxagentDrawableContainGlyphs(pSrcDrawable) == 1)
  {
    nxagentSetDrawableContainGlyphs(pDstDrawable, 1);
  }

  if (pSrcDrawable -> type == DRAWABLE_PIXMAP)
  {
    nxagentIncreasePixmapUsageCounter((PixmapPtr) pSrcDrawable);
  }

  if (pSrcDrawable -> type == DRAWABLE_PIXMAP &&
          pDstDrawable -> type == DRAWABLE_PIXMAP)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentCopyArea: Going to copy area from virtual pixmap [%p] to [%p]\n",
                (void *) nxagentVirtualDrawable(pSrcDrawable),
                    (void *) nxagentVirtualDrawable(pDstDrawable));
    #endif

    return fbCopyArea(nxagentVirtualDrawable(pSrcDrawable), 
                          nxagentVirtualDrawable(pDstDrawable),
                              pGC, srcx, srcy, width, height, dstx, dsty);
  }
  else if (pSrcDrawable -> type == DRAWABLE_PIXMAP)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentCopyArea: Going to copy area from virtual pixmap [%p] to window [%p]\n",
                (void *) nxagentVirtualDrawable(pSrcDrawable),
                    (void *) pDstDrawable);
    #endif

    return fbCopyArea(nxagentVirtualDrawable(pSrcDrawable), pDstDrawable,
                          pGC, srcx, srcy, width, height, dstx, dsty);
  }
  else if (pDstDrawable -> type == DRAWABLE_PIXMAP)
  {
    /*
     * If we are here the source drawable
     * must be a window.
     */

    if (((WindowPtr) pSrcDrawable) -> viewable)
    {
      #ifdef TEST
      fprintf(stderr, "nxagentCopyArea: Going to copy area from window [%p] to virtual pixmap [%p]\n",
                  (void *) pSrcDrawable, (void *) nxagentVirtualDrawable(pDstDrawable));
      #endif

      return fbCopyArea(pSrcDrawable, nxagentVirtualDrawable(pDstDrawable),
                            pGC, srcx, srcy, width, height, dstx, dsty);
    }
  }
  else
  {
    /*
     * If we are here the source drawable
     * must be a window.
     */

    if (((WindowPtr) pSrcDrawable) -> viewable)
    {
      #ifdef TEST
      fprintf(stderr, "nxagentCopyArea: Going to copy area from window [%p] to window [%p]\n",
                  (void *) pSrcDrawable, (void *) pDstDrawable);
      #endif

      return fbCopyArea(pSrcDrawable, pDstDrawable,
                            pGC, srcx, srcy, width, height, dstx, dsty);
    }
  }

  return miHandleExposures(pSrcDrawable, pDstDrawable, pGC, 
                               srcx, srcy, width, height, dstx, dsty, 0);
}

RegionPtr nxagentCopyPlane(DrawablePtr pSrcDrawable, DrawablePtr pDstDrawable,
                               GCPtr pGC, int srcx, int srcy, int width, int height,
                                   int dstx, int dsty, unsigned long plane)
{
  unsigned int format;
  int leftPad = 0;
  unsigned long planeMask = 0xffffffff;

  RegionPtr pSrcRegion, pDstRegion;
  RegionRec corruptedRegion;

  int skip = 0;

  #ifdef TEST
  fprintf(stderr, "nxagentCopyPlane: Image src [%s:%p], dst [%s:%p] (%d,%d) -> (%d,%d) size (%d,%d)\n",
              ((pSrcDrawable)->type == DRAWABLE_PIXMAP) ? "PIXMAP" : "WINDOW", (void *) pSrcDrawable,
                      ((pDstDrawable)->type == DRAWABLE_PIXMAP) ? "PIXMAP" : "WINDOW",
                          (void *) pDstDrawable, srcx, srcy, dstx, dsty, width, height);
  #endif

  if (nxagentGCTrap == 1 || nxagentShmTrap == 1)
  {
    if (pSrcDrawable -> type == DRAWABLE_PIXMAP &&
            pDstDrawable -> type == DRAWABLE_PIXMAP)
    {
      return fbCopyPlane(nxagentVirtualDrawable(pSrcDrawable), 
                           nxagentVirtualDrawable(pDstDrawable),
                             pGC, srcx, srcy, width, height, dstx, dsty, plane);
    }
    else if (pSrcDrawable -> type == DRAWABLE_PIXMAP)
    {
      return fbCopyPlane(nxagentVirtualDrawable(pSrcDrawable), pDstDrawable,
                             pGC, srcx, srcy, width, height, dstx, dsty, plane);
    }
    else if (pDstDrawable -> type == DRAWABLE_PIXMAP)
    {
      return fbCopyPlane(pSrcDrawable, nxagentVirtualDrawable(pDstDrawable),
                             pGC, srcx, srcy, width, height, dstx, dsty, plane);
    }
    else
    {
      return fbCopyPlane(pSrcDrawable, pDstDrawable, pGC, srcx, srcy, width, height,
                             dstx, dsty, plane);
    }

    return NullRegion;
  }

  /*
   * If the pixmap is on shared memory, we can't
   * know if the pixmap content is changed and
   * so have to translate the operation in a put
   * image operation. This can seriously affect
   * the performance.
   */

  if (pSrcDrawable -> type == DRAWABLE_PIXMAP &&
         nxagentIsShmPixmap((PixmapPtr) pSrcDrawable))
  {
    char *data;
    int depth, length;

    depth  = pSrcDrawable -> depth;

    format = (depth == 1) ? XYPixmap : ZPixmap;

    length = nxagentImageLength(width, height, format, leftPad, depth);

    if ((data = xalloc(length)) == NULL)
    {
      #ifdef DEBUG
      fprintf(stderr, "nxagentCopyPlane: WARNING! Failed to allocate memory for the operation.\n");
      #endif

      return 0;
    }

    fbGetImage(nxagentVirtualDrawable(pSrcDrawable), srcx, srcy, width, height, format, planeMask, data);

    /*
     * If the source is a shared memory pixmap,
     * put the image directly to the destination.
     */

    nxagentPutImage(pDstDrawable, pGC, depth, dstx, dsty,
                        width, height, leftPad, format, data);

    #ifdef TEST
    fprintf(stderr,"nxagentCopyPlane: Synchronize Pixmap %p virtual %p x %d y %d w %d h %d \n",
               (void *) pSrcDrawable, (void *) nxagentVirtualDrawable(pSrcDrawable),
                   srcx, srcy, width, height);
    #endif

    xfree(data);

    /*
     * If the source is a shared memory pixmap, the
     * content of the framebuffer has been placed
     * directly on the destination so we can skip
     * the copy plane operation.
     */

    skip = 1;
  }

  if (skip == 0 && nxagentDrawableStatus(pSrcDrawable) == NotSynchronized)
  {
    if (pDstDrawable -> type == DRAWABLE_PIXMAP &&
            nxagentOption(DeferLevel) > 0)
    {
      pDstRegion = nxagentCreateRegion(pDstDrawable, pGC, dstx, dsty, width, height);

      nxagentMarkCorruptedRegion(pDstDrawable, pDstRegion);

      nxagentFreeRegion(pDstDrawable, pDstRegion);

      skip = 1;
    }
    else
    {
      pSrcRegion = nxagentCreateRegion(pSrcDrawable, NULL, srcx, srcy, width, height);

      REGION_INIT(pSrcDrawable -> pScreen, &corruptedRegion, NullBox, 1);

      REGION_INTERSECT(pSrcDrawable -> pScreen, &corruptedRegion,
                           pSrcRegion, nxagentCorruptedRegion(pSrcDrawable));

      if (REGION_NIL(&corruptedRegion) == 0)
      {
        #ifdef TEST
        fprintf(stderr, "nxagentCopyPlane: Forcing the synchronization of source drawable at [%p].\n",
                    (void *) pSrcDrawable);
        #endif

        nxagentSynchronizeRegion(pSrcDrawable, &corruptedRegion /*pSrcRegion*/, NEVER_BREAK, NULL);

        pDstRegion = nxagentCreateRegion(pDstDrawable, pGC, dstx, dsty, width, height);

        nxagentUnmarkCorruptedRegion(pDstDrawable, pDstRegion);

        nxagentFreeRegion(pDstDrawable, pDstRegion);
      }

      REGION_UNINIT(pSrcDrawable -> pScreen, &corruptedRegion);

      nxagentFreeRegion(pSrcDrawable, pSrcRegion);
    }
  }
  #ifdef TEST
  else
  {
    fprintf(stderr, "nxagentCopyPlane: Source drawable at [%p] already synchronized.\n",
                (void *) pSrcDrawable);
  }
  #endif

  if (skip == 0)
  {
    XCopyPlane(nxagentDisplay,
                   nxagentDrawable(pSrcDrawable), nxagentDrawable(pDstDrawable),
                       nxagentGC(pGC), srcx, srcy, width, height, dstx, dsty, plane);
  }

  if ((pSrcDrawable)->type == DRAWABLE_PIXMAP &&
          (pDstDrawable)->type == DRAWABLE_PIXMAP)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentCopyPlane: going to copy plane from FB pixmap [%p] to [%p].\n",
                (void *) nxagentVirtualDrawable(pSrcDrawable),
                    (void *) nxagentVirtualDrawable(pDstDrawable));
    #endif

    return fbCopyPlane(nxagentVirtualDrawable(pSrcDrawable), 
                         nxagentVirtualDrawable(pDstDrawable),
                           pGC, srcx, srcy, width, height, dstx, dsty, plane);
  }
  else if (pSrcDrawable -> type == DRAWABLE_PIXMAP)
  {
    return fbCopyPlane(nxagentVirtualDrawable(pSrcDrawable), pDstDrawable, pGC,
                            srcx, srcy, width, height, dstx, dsty, plane);
  }
  else if (pDstDrawable -> type == DRAWABLE_PIXMAP)
  {
    return fbCopyPlane(pSrcDrawable, nxagentVirtualDrawable(pDstDrawable), pGC,
                            srcx, srcy, width, height, dstx, dsty, plane);
  }
  else
  {
    return fbCopyPlane(pSrcDrawable, pDstDrawable, pGC,
                            srcx, srcy, width, height, dstx, dsty, plane);
  }

  return miHandleExposures(pSrcDrawable, pDstDrawable, pGC, 
                               srcx, srcy, width, height, dstx, dsty, plane);
}

void nxagentPolyPoint(DrawablePtr pDrawable, GCPtr pGC, int mode,
                          int nPoints, xPoint *pPoints)
{
  #ifdef TEST
  fprintf(stderr, "nxagentPolyPoint: Drawable at [%p] GC at [%p] Points [%d].\n",
              (void *) pDrawable, (void *) pGC, nPoints);
  #endif

  if (nxagentGCTrap == 1)
  {
    if ((pDrawable)->type == DRAWABLE_PIXMAP)
    {
      fbPolyPoint(nxagentVirtualDrawable(pDrawable), pGC, mode, nPoints, pPoints);
    }
    else
    {
      fbPolyPoint(pDrawable, pGC, mode, nPoints, pPoints);
    }

    return;
  }

  if ((pDrawable)->type == DRAWABLE_PIXMAP)
  {
    #ifdef TEST
    fprintf(stderr, "GCOps: GC [%p] going to poly point on FB pixmap [%p].\n",
                (void *) pGC, (void *) nxagentVirtualDrawable(pDrawable));
    #endif

    if (nxagentPixmapIsVirtual((PixmapPtr) pDrawable))
    {
      #ifdef TEST
      fprintf(stderr, "GCOps: poly point enters with virtual pixmap [%p] parent is [%p]\n",
                  (void *) nxagentVirtualDrawable(pDrawable),
                      (void *) nxagentRealPixmap((PixmapPtr) pDrawable));
      #endif

      if (nxagentRealPixmap((PixmapPtr) pDrawable))
      {
        XDrawPoints(nxagentDisplay, nxagentDrawable((DrawablePtr) nxagentRealPixmap((PixmapPtr) pDrawable)),
                        nxagentGC(pGC), (XPoint *) pPoints, nPoints, mode);
      }
    }
    else
    {
      XDrawPoints(nxagentDisplay, nxagentDrawable(pDrawable), nxagentGC(pGC),
                      (XPoint *) pPoints, nPoints, mode);
    }

    fbPolyPoint(nxagentVirtualDrawable(pDrawable), pGC, mode, nPoints, pPoints);

    return;
  }
  else
  {
    XDrawPoints(nxagentDisplay, nxagentDrawable(pDrawable), nxagentGC(pGC),
                    (XPoint *) pPoints, nPoints, mode);

    fbPolyPoint(pDrawable, pGC, mode, nPoints, pPoints);
  }
}

void nxagentPolyLines(DrawablePtr pDrawable, GCPtr pGC, int mode,
                          int nPoints, xPoint *pPoints)
{
  if (nxagentGCTrap == 1)
  {
    if ((pDrawable)->type == DRAWABLE_PIXMAP)
    {
      fbPolyLine(nxagentVirtualDrawable(pDrawable), pGC, mode, nPoints, pPoints);
    }
    else
    {
      fbPolyLine(pDrawable, pGC, mode, nPoints, pPoints);
    }

    return;
  }

  if ((pDrawable)->type == DRAWABLE_PIXMAP)
  {
    #ifdef TEST
    fprintf(stderr, "GCOps: GC [%p] going to poly line on FB pixmap [%p].\n",
                (void *) pGC, (void *) nxagentVirtualDrawable(pDrawable));
    #endif

    if (nxagentPixmapIsVirtual((PixmapPtr) pDrawable))
    {
      #ifdef TEST
      fprintf(stderr, "GCOps: poly lines enters with virtual pixmap = [%p] parent is = [%p]\n",
                  (void *) nxagentVirtualDrawable(pDrawable),
                       (void *) nxagentRealPixmap((PixmapPtr) pDrawable));
      #endif

      if (nxagentRealPixmap((PixmapPtr) pDrawable))
      {
        XDrawLines(nxagentDisplay, nxagentDrawable((DrawablePtr) nxagentRealPixmap((PixmapPtr) pDrawable)),
                       nxagentGC(pGC), (XPoint *) pPoints, nPoints, mode);
      }
    }
    else
    {
      XDrawLines(nxagentDisplay, nxagentDrawable(pDrawable), nxagentGC(pGC),
                 (XPoint *) pPoints, nPoints, mode);
    }

    fbPolyLine(nxagentVirtualDrawable(pDrawable), pGC, mode, nPoints, pPoints);

    return;
  }
  else
  {
    XDrawLines(nxagentDisplay, nxagentDrawable(pDrawable), nxagentGC(pGC),
               (XPoint *) pPoints, nPoints, mode);

    fbPolyLine(pDrawable, pGC, mode, nPoints, pPoints);
  }
}

void nxagentPolySegment(DrawablePtr pDrawable, GCPtr pGC,
                            int nSegments, xSegment *pSegments)
{
  #ifdef TEST

  if (nSegments == 1)
  {
    fprintf(stderr, "nxagentPolySegment: Drawable at [%p] GC at [%p] Segment [%d,%d,%d,%d].\n",
                (void *) pDrawable, (void *) pGC,
                    pSegments -> x1, pSegments -> y1, pSegments -> x2, pSegments -> y2);
  }
  else
  {
    fprintf(stderr, "nxagentPolySegment: Drawable at [%p] GC at [%p] Segments [%d].\n",
                (void *) pDrawable, (void *) pGC, nSegments);
  }

  #endif

  if (nxagentGCTrap == 1)
  {
    if ((pDrawable)->type == DRAWABLE_PIXMAP)
    {
      fbPolySegment(nxagentVirtualDrawable(pDrawable), pGC, nSegments, pSegments);
    }
    else
    {
      fbPolySegment(pDrawable, pGC, nSegments, pSegments);
    }

    return;
  }

  if ((pDrawable)->type == DRAWABLE_PIXMAP)
  {
    #ifdef TEST
    fprintf(stderr, "GCOps: GC [%p] going to poly segment on FB pixmap [%p].\n",
                (void *) pGC, (void *) nxagentVirtualDrawable(pDrawable));
    #endif

    if (nxagentPixmapIsVirtual((PixmapPtr) pDrawable))
    {
      #ifdef TEST
      fprintf(stderr, "GCOps: poly segment enters with virtual pixmap = [%p] parent is = [%p]\n",
                  (void *) nxagentVirtualDrawable(pDrawable),
                      (void *) nxagentRealPixmap((PixmapPtr) pDrawable));
      #endif

      if (nxagentRealPixmap((PixmapPtr) pDrawable))
      {
        XDrawSegments(nxagentDisplay, nxagentDrawable((DrawablePtr) nxagentRealPixmap((PixmapPtr) pDrawable)),
                          nxagentGC(pGC), (XSegment *) pSegments, nSegments);
      }
    }
    else
    {
      XDrawSegments(nxagentDisplay, nxagentDrawable(pDrawable), nxagentGC(pGC),
                    (XSegment *) pSegments, nSegments);
    }

    SET_GC_TRAP();
    fbPolySegment(nxagentVirtualDrawable(pDrawable), pGC, nSegments, pSegments);
    RESET_GC_TRAP();

    return;
  }
  else
  {
    XDrawSegments(nxagentDisplay, nxagentDrawable(pDrawable), nxagentGC(pGC),
                 (XSegment *) pSegments, nSegments);

    SET_GC_TRAP();
    fbPolySegment(pDrawable, pGC, nSegments, pSegments);
    RESET_GC_TRAP();
  }
}

void nxagentPolyRectangle(DrawablePtr pDrawable, GCPtr pGC,
                              int nRectangles, xRectangle *pRectangles)
{
  #ifdef TEST

  if (nRectangles == 1)
  {
    fprintf(stderr, "nxagentPolyRectangle: Drawable at [%p] GC at [%p] Rectangle [%d,%d][%d,%d].\n",
                (void *) pDrawable, (void *) pGC,
                    pRectangles -> x, pRectangles -> y, pRectangles -> width, pRectangles -> height);
  }
  else
  {
    fprintf(stderr, "nxagentPolyRectangle: Drawable at [%p] GC at [%p] Rectangles [%d].\n",
                (void *) pDrawable, (void *) pGC, nRectangles);
  }

  #endif

  if (nxagentGCTrap == 1)
  {
    if ((pDrawable)->type == DRAWABLE_PIXMAP)
    {
      miPolyRectangle(nxagentVirtualDrawable(pDrawable), pGC, nRectangles, pRectangles);
    }
    else
    {
      miPolyRectangle(pDrawable, pGC, nRectangles, pRectangles);
    }

    return;
  }

  if ((pDrawable)->type == DRAWABLE_PIXMAP)
  {
    #ifdef TEST
    fprintf(stderr, "GCOps: GC [%p] going to poly rectangle on FB pixmap [%p].\n",
                (void *) pGC, (void *) nxagentVirtualDrawable(pDrawable));
    #endif

    if (nxagentPixmapIsVirtual((PixmapPtr) pDrawable))
    {
      #ifdef TEST
      fprintf(stderr, "GCOps: poly rectangle enters with virtual pixmap = [%p] parent is = [%p]\n",
                  (void *) nxagentVirtualDrawable(pDrawable),
                      (void *) nxagentRealPixmap((PixmapPtr) pDrawable));
      #endif

      if (nxagentRealPixmap((PixmapPtr) pDrawable))
      {
        XDrawRectangles(nxagentDisplay, nxagentDrawable((DrawablePtr) nxagentRealPixmap((PixmapPtr) pDrawable)),
                            nxagentGC(pGC), (XRectangle *) pRectangles, nRectangles);
      }
    }
    else
    {
      XDrawRectangles(nxagentDisplay, nxagentDrawable(pDrawable), nxagentGC(pGC),
                      (XRectangle *) pRectangles, nRectangles);
    }

    SET_GC_TRAP();

    miPolyRectangle(nxagentVirtualDrawable(pDrawable), pGC, nRectangles, pRectangles);

    RESET_GC_TRAP();

    return;
  }
  else
  {
    XDrawRectangles(nxagentDisplay, nxagentDrawable(pDrawable), nxagentGC(pGC),
                    (XRectangle *) pRectangles, nRectangles);

    SET_GC_TRAP();

    miPolyRectangle(pDrawable, pGC, nRectangles, pRectangles);

    RESET_GC_TRAP();
  }
}

void nxagentPolyArc(DrawablePtr pDrawable, GCPtr pGC,
                        int nArcs, xArc *pArcs)
{
  if (nxagentGCTrap == 1)
  {
    if ((pDrawable)->type == DRAWABLE_PIXMAP)
    {
      fbPolyArc(nxagentVirtualDrawable(pDrawable), pGC, nArcs, pArcs);
    }
    else
    {
      fbPolyArc(pDrawable, pGC, nArcs, pArcs);
    }

    return;
  }

  if ((pDrawable)->type == DRAWABLE_PIXMAP)
  {
    #ifdef TEST
    fprintf(stderr, "GCOps: GC [%p] going to poly arc on FB pixmap [%p].\n",
                (void *) pGC, (void *) nxagentVirtualDrawable(pDrawable));
    #endif

    if (nxagentPixmapIsVirtual((PixmapPtr) pDrawable))
    {
      #ifdef TEST
      fprintf(stderr, "GCOps: poly arc enters with virtual pixmap = [%p] parent is = [%p]\n",
                  (void *) nxagentVirtualDrawable(pDrawable),
                      (void *) nxagentRealPixmap((PixmapPtr) pDrawable));
      #endif

      if (nxagentRealPixmap((PixmapPtr) pDrawable))
      {
        XDrawArcs(nxagentDisplay, nxagentDrawable((DrawablePtr) nxagentRealPixmap((PixmapPtr) pDrawable)),
                      nxagentGC(pGC), (XArc *) pArcs, nArcs);
      }
    }
    else
    {
       XDrawArcs(nxagentDisplay, nxagentDrawable(pDrawable), nxagentGC(pGC),
                     (XArc *) pArcs, nArcs);
    }

    fbPolyArc(nxagentVirtualDrawable(pDrawable), pGC, nArcs, pArcs);

    return;
  }
  else
  {
    XDrawArcs(nxagentDisplay, nxagentDrawable(pDrawable), nxagentGC(pGC),
                  (XArc *) pArcs, nArcs);

    fbPolyArc(pDrawable, pGC, nArcs, pArcs);
  }
}

void nxagentFillPolygon(DrawablePtr pDrawable, GCPtr pGC, int shape,
                            int mode, int nPoints, xPoint *pPoints)
{
  xPoint *newPoints = NULL;

  if (nxagentGCTrap == 1)
  {
    if ((pDrawable)->type == DRAWABLE_PIXMAP)
    {
      miFillPolygon(nxagentVirtualDrawable(pDrawable), pGC, shape, mode, nPoints, pPoints);
    }
    else
    {
      miFillPolygon(pDrawable, pGC, shape, mode, nPoints, pPoints);
    }

    return;
  }

  /*
   * The coordinate-mode must be CoordModePrevious
   * to make better use of differential encoding of
   * X_FillPoly request by the side of proxy.
   */

  if (mode == CoordModeOrigin)
  {
    int i;

    mode = CoordModePrevious;

    newPoints = xalloc(nPoints * sizeof(xPoint));

    /*
     * The first point is always relative
     * to the drawable's origin.
     */

    newPoints[0].x = pPoints[0].x;
    newPoints[0].y = pPoints[0].y;

    /*
     * If coordinate-mode is CoordModePrevious,
     * the points following the first are rela-
     * tive to the previous point.
     */

    for (i = 1; i < nPoints; i++)
    {
      newPoints[i].x = pPoints[i].x - pPoints[i-1].x;
      newPoints[i].y = pPoints[i].y - pPoints[i-1].y;
    }

    pPoints = newPoints;
  }

  if ((pDrawable)->type == DRAWABLE_PIXMAP)
  {
    #ifdef TEST
    fprintf(stderr, "GCOps: GC [%p] going to fill polygon on FB pixmap [%p].\n",
                (void *) pGC, (void *) nxagentVirtualDrawable(pDrawable));
    #endif

    if (nxagentPixmapIsVirtual((PixmapPtr) pDrawable))
    {
      #ifdef TEST
      fprintf(stderr, "GCOps: fill polygon enters with virtual pixmap = [%p] parent is = [%p]\n",
                  (void *) nxagentVirtualDrawable(pDrawable),
                      (void *) nxagentRealPixmap((PixmapPtr) pDrawable));
      #endif

      if (nxagentRealPixmap((PixmapPtr) pDrawable))
      {
        XFillPolygon(nxagentDisplay, nxagentDrawable((DrawablePtr) nxagentRealPixmap((PixmapPtr) pDrawable)),
                         nxagentGC(pGC), (XPoint *) pPoints, nPoints, shape, mode);
      }
    }
    else
    {
      XFillPolygon(nxagentDisplay, nxagentDrawable(pDrawable), nxagentGC(pGC),
                       (XPoint *) pPoints, nPoints, shape, mode);
    }

    SET_GC_TRAP();

    miFillPolygon(nxagentVirtualDrawable(pDrawable), pGC, shape, mode, nPoints, pPoints);

    RESET_GC_TRAP();
  }
  else
  {
    XFillPolygon(nxagentDisplay, nxagentDrawable(pDrawable), nxagentGC(pGC),
                     (XPoint *) pPoints, nPoints, shape, mode);

    SET_GC_TRAP();

    miFillPolygon(pDrawable, pGC, shape, mode, nPoints, pPoints);

    RESET_GC_TRAP();
  }

  if (newPoints != NULL)
  {
    xfree(newPoints);
  }
}

void nxagentPolyFillRect(DrawablePtr pDrawable, GCPtr pGC,
                             int nRectangles, xRectangle *pRectangles)
{
  RegionPtr rectRegion;

  int inheritCorruptedRegion;

  #ifdef TEST

  if (nRectangles == 1)
  {
    fprintf(stderr, "nxagentPolyFillRect: Drawable at [%p] GC at [%p] FillStyle [%d] Rectangle [%d,%d][%d,%d].\n",
                (void *) pDrawable, (void *) pGC, pGC -> fillStyle, 
                    pRectangles -> x, pRectangles -> y, pRectangles -> width, pRectangles -> height);
  }
  else
  {
    fprintf(stderr, "nxagentPolyFillRect: Drawable at [%p] GC at [%p] FillStyle [%d] Rectangles [%d].\n",
                (void *) pDrawable, (void *) pGC, pGC -> fillStyle, nRectangles);
  }

  #endif

  if (nxagentGCTrap == 1)
  {
    if ((pDrawable)->type == DRAWABLE_PIXMAP)
    {
      fbPolyFillRect(nxagentVirtualDrawable(pDrawable), pGC, nRectangles, pRectangles);
    }
    else
    {
      fbPolyFillRect(pDrawable, pGC, nRectangles, pRectangles);
    }

    return;
  }

  /*
   * The PolyFillRect acts in two ways: if the GC
   * has a corrupted tile, the operation propagates
   * the corrupted region on the destination. In
   * other cases, because the PolyFillRect will
   * cover the destination, any corrupted region
   * intersecting the target will be cleared.
   */

  inheritCorruptedRegion = 0;

  if (pGC -> fillStyle == FillTiled &&
          pGC -> tileIsPixel == 0 && pGC -> tile.pixmap != NULL)
  {
    nxagentIncreasePixmapUsageCounter(pGC -> tile.pixmap);

    if (nxagentDrawableStatus((DrawablePtr) pGC -> tile.pixmap) == NotSynchronized)
    {
      #ifdef TEST
      fprintf(stderr, "nxagentPolyFillRect: GC at [%p] uses corrupted tile pixmap at [%p]. Going to "
                  "corrupt the destination [%s][%p].\n", (void *) pGC, (void *) pGC -> tile.pixmap,
                      (pDrawable -> type == DRAWABLE_PIXMAP ? "pixmap" : "window"), (void *) pDrawable);

      #endif

      inheritCorruptedRegion = 1;
    }
  }

  if (inheritCorruptedRegion == 1 || nxagentDrawableStatus(pDrawable) == NotSynchronized)
  {
    rectRegion = RECTS_TO_REGION(pDrawable -> pScreen, nRectangles, pRectangles, CT_REGION);

    if (pGC -> clientClip != NULL)
    {
      RegionRec tmpRegion;

      REGION_INIT(pDrawable -> pScreen, &tmpRegion, NullBox, 1);

      REGION_COPY(pDrawable -> pScreen, &tmpRegion, ((RegionPtr) pGC -> clientClip));

      if (pGC -> clipOrg.x != 0 || pGC -> clipOrg.y != 0)
      {
        REGION_TRANSLATE(pDrawable -> pScreen, &tmpRegion, pGC -> clipOrg.x, pGC -> clipOrg.y);
      }

      REGION_INTERSECT(pDrawable -> pScreen, rectRegion, rectRegion, &tmpRegion);

      REGION_UNINIT(pDrawable -> pScreen, &tmpRegion);
    }

    if (inheritCorruptedRegion == 1)
    {
      /*
       * The fill style should affect the cor-
       * rupted region propagation: if the tile
       * is not completely corrupted the region
       * should be 'tiled' over the destination.
       */

      nxagentMarkCorruptedRegion(pDrawable, rectRegion);
    }
    else
    {
      if (pGC -> fillStyle != FillStippled && pGC -> fillStyle != FillOpaqueStippled)
      {
        nxagentUnmarkCorruptedRegion(pDrawable, rectRegion);
      }
      else
      {
        /*
         * The stipple mask computation could cause
         * an high fragmentation of the destination
         * region. An analysis should be done to exa-
         * mine the better solution (e.g.rdesktop
         * uses stipples to draw texts).
         */

        #ifdef TEST
        fprintf(stderr, "nxagentPolyFillRect: Synchronizing the region [%d,%d,%d,%d] before using "
                    "the stipple at [%p].\n", rectRegion -> extents.x1, rectRegion -> extents.y1,
                        rectRegion -> extents.x2, rectRegion -> extents.y2, (void *) pGC -> stipple);
        #endif

        nxagentSynchronizeRegion(pDrawable, rectRegion, NEVER_BREAK, NULL);
      }
    }

    REGION_DESTROY(pDrawable -> pScreen, rectRegion);
  }

  if ((pDrawable)->type == DRAWABLE_PIXMAP)
  {
    #ifdef TEST
    fprintf(stderr, "GCOps: GC [%p] going to poly fill rect on FB pixmap [%p].\n",
                (void *) pGC, (void *) nxagentVirtualDrawable(pDrawable));
    #endif

    if (nxagentPixmapIsVirtual((PixmapPtr) pDrawable))
    {
      #ifdef TEST
      fprintf(stderr, "GCOps: poly fill rect enters with virtual pixmap = [%p] parent is = [%p]\n",
                  (void *) nxagentVirtualDrawable(pDrawable),
                      (void *) nxagentRealPixmap((PixmapPtr) pDrawable));
      #endif

      if (nxagentRealPixmap((PixmapPtr) pDrawable))
      {
        XFillRectangles(nxagentDisplay, nxagentDrawable((DrawablePtr) nxagentRealPixmap((PixmapPtr) pDrawable)),
                            nxagentGC(pGC), (XRectangle *) pRectangles, nRectangles);
      }
    }
    else
    {
      XFillRectangles(nxagentDisplay, nxagentDrawable(pDrawable), nxagentGC(pGC),
                          (XRectangle *) pRectangles, nRectangles);
    }

    fbPolyFillRect(nxagentVirtualDrawable(pDrawable), pGC, nRectangles, pRectangles);

    return;
  }
  else
  {
    XFillRectangles(nxagentDisplay, nxagentDrawable(pDrawable), nxagentGC(pGC),
                        (XRectangle *) pRectangles, nRectangles);

    fbPolyFillRect(pDrawable, pGC, nRectangles, pRectangles);
  }
}

void nxagentPolyFillArc(DrawablePtr pDrawable, GCPtr pGC,
                            int nArcs, xArc *pArcs)
{
  if (nxagentGCTrap == 1)
  {
    if ((pDrawable)->type == DRAWABLE_PIXMAP)
    {
      miPolyFillArc(nxagentVirtualDrawable(pDrawable), pGC, nArcs, pArcs);
    }
    else
    {
      miPolyFillArc(pDrawable, pGC, nArcs, pArcs);
    }

    return;
  }

  if ((pDrawable)->type == DRAWABLE_PIXMAP)
  {
    #ifdef TEST
    fprintf(stderr, "GCOps: GC [%p] going to poly fillarc on FB pixmap [%p].\n",
                (void *) pGC, (void *) nxagentVirtualDrawable(pDrawable));
    #endif

    if (nxagentPixmapIsVirtual((PixmapPtr) pDrawable))
    {
      #ifdef TEST
      fprintf(stderr, "GCOps: poly fill arc enters with virtual pixmap = [%p] parent is = [%p]\n",
                  (void *) nxagentVirtualDrawable(pDrawable),
                       (void *) nxagentRealPixmap((PixmapPtr) pDrawable));
      #endif

      if (nxagentRealPixmap((PixmapPtr) pDrawable))
      {
        XFillArcs(nxagentDisplay, nxagentDrawable((DrawablePtr) nxagentRealPixmap((PixmapPtr) pDrawable)),
                      nxagentGC(pGC), (XArc *) pArcs, nArcs);
      }
    }
    else
    {
      XFillArcs(nxagentDisplay, nxagentDrawable(pDrawable), nxagentGC(pGC),
                (XArc *) pArcs, nArcs);
    }

    SET_GC_TRAP();

    miPolyFillArc(nxagentVirtualDrawable(pDrawable), pGC, nArcs, pArcs);

    RESET_GC_TRAP();

    return;
  }
  else
  {
    XFillArcs(nxagentDisplay, nxagentDrawable(pDrawable),
                  nxagentGC(pGC), (XArc *) pArcs, nArcs);

    SET_GC_TRAP();

    miPolyFillArc(pDrawable, pGC, nArcs, pArcs);

    RESET_GC_TRAP();
  }
}

int nxagentPolyText8(DrawablePtr pDrawable, GCPtr pGC, int x,
                         int y, int count, char *string)
{
  int width;

  /*
   * While the session is suspended
   * the font structure is NULL.
   */

  if (nxagentFontStruct(pGC -> font) == NULL)
  {
    return x;
  }

  width = XTextWidth(nxagentFontStruct(pGC->font), string, count);

  if (nxagentGCTrap == 1)
  {
    if ((pDrawable)->type == DRAWABLE_PIXMAP)
    {
      miPolyText8(nxagentVirtualDrawable(pDrawable), pGC, x, y, count, string);
    }
    else
    {
      miPolyText8(pDrawable, pGC, x, y, count, string);
    }

    return width + x;
  }

  if ((pDrawable)->type == DRAWABLE_PIXMAP)
  {
    #ifdef TEST
    fprintf(stderr, "GCOps: GC [%p] going to poly text8 on FB pixmap [%p] with string [%s].\n",
                (void *) pGC, (void *) nxagentVirtualDrawable(pDrawable), (char *) string);
    #endif

    if (nxagentPixmapIsVirtual((PixmapPtr) pDrawable))
    {
      #ifdef TEST
      fprintf(stderr, "GCOps: poly text8 enters with virtual pixmap [%p] parent is [%p]\n",
                  (void *) nxagentVirtualDrawable(pDrawable),
                      (void *) nxagentRealPixmap((PixmapPtr) pDrawable));
      #endif

      if (nxagentRealPixmap((PixmapPtr) pDrawable))
      {
         XDrawString(nxagentDisplay, nxagentDrawable((DrawablePtr) nxagentRealPixmap((PixmapPtr) pDrawable)),
                         nxagentGC(pGC), x, y, string, count);
      }
    }
    else
    {
      XDrawString(nxagentDisplay, nxagentDrawable(pDrawable),
                      nxagentGC(pGC), x, y, string, count);
    }

    miPolyText8(nxagentVirtualDrawable(pDrawable), pGC, x, y, count, string);

    return width + x;
  }
  else
  {
    XDrawString(nxagentDisplay, nxagentDrawable(pDrawable), nxagentGC(pGC),
                    x, y, string, count);

    miPolyText8(pDrawable, pGC, x, y, count, string);
  }

  return width + x;
}

int nxagentPolyText16(DrawablePtr pDrawable, GCPtr pGC, int x,
                          int y, int count, unsigned short *string)
{
  int width;

  /*
   * While the session is suspended
   * the font structure is NULL.
   */

  if (nxagentFontStruct(pGC -> font) == NULL)
  {
    return x;
  }

  width = XTextWidth16(nxagentFontStruct(pGC->font), (XChar2b *)string, count);

  if (nxagentGCTrap == 1)
  {
    if ((pDrawable)->type == DRAWABLE_PIXMAP)
    {
      miPolyText16(nxagentVirtualDrawable(pDrawable), pGC, x, y, count, string);
    }
    else
    {
      miPolyText16(pDrawable, pGC, x, y, count, string);
    }

    return width + x;
  }

  if ((pDrawable)->type == DRAWABLE_PIXMAP)
  {
    #ifdef TEST
    fprintf(stderr, "GCOps: GC [%p] going to poly text16 on FB pixmap %p] with string [%s]\n",
                (void *) pGC, (void *) nxagentVirtualDrawable(pDrawable), (char *) string);
    #endif

    if (nxagentPixmapIsVirtual((PixmapPtr) pDrawable))
    {
      #ifdef TEST
      fprintf(stderr, "GCOps: poly text16 enters with virtual pixmap = [%p] parent is = [%p]\n",
                  (void *) nxagentVirtualDrawable(pDrawable),
                      (void *) nxagentRealPixmap((PixmapPtr) pDrawable));
      #endif

      if (nxagentRealPixmap((PixmapPtr) pDrawable))
      {
        XDrawString16(nxagentDisplay, nxagentDrawable((DrawablePtr) nxagentRealPixmap((PixmapPtr) pDrawable)),
                          nxagentGC(pGC), x, y, (XChar2b *)string, count);
      }
    }
    else
    {
      XDrawString16(nxagentDisplay, nxagentDrawable(pDrawable),
                        nxagentGC(pGC), x, y, (XChar2b *)string, count);
    }

    miPolyText16(nxagentVirtualDrawable(pDrawable), pGC, x, y, count, string);

    return width + x;
  }
  else
  {
    XDrawString16(nxagentDisplay, nxagentDrawable(pDrawable),
                      nxagentGC(pGC), x, y, (XChar2b *)string, count);

    miPolyText16(pDrawable, pGC, x, y, count, string);
  }

  return width + x;
}

void nxagentImageText8(DrawablePtr pDrawable, GCPtr pGC, int x,
                           int y, int count, char *string)
{
  if (nxagentGCTrap == 1)
  {
    if ((pDrawable)->type == DRAWABLE_PIXMAP)
    {
      miImageText8(nxagentVirtualDrawable(pDrawable), pGC, x, y, count, string);
    }
    else
    {
      miImageText8(pDrawable, pGC, x, y, count, string);
    }

    return;
  }

  if ((pDrawable)->type == DRAWABLE_PIXMAP)
  {
    #ifdef TEST
    fprintf(stderr, "GCOps: GC [%p] going to image text8 on FB pixmap [%p] with string [%s].\n",
                (void *) pGC, (void *) nxagentVirtualDrawable(pDrawable), string);
    #endif

    if (nxagentPixmapIsVirtual((PixmapPtr) pDrawable))
    {
      #ifdef TEST
      fprintf(stderr, "GCOps: poly image text8 enters with virtual pixmap [%p] parent is [%p]\n",
                  (void *) nxagentVirtualDrawable(pDrawable),
                      (void *) nxagentRealPixmap((PixmapPtr) pDrawable));
      #endif

      if (nxagentRealPixmap((PixmapPtr) pDrawable))
      {
        XDrawImageString(nxagentDisplay, nxagentDrawable((DrawablePtr) nxagentRealPixmap((PixmapPtr) pDrawable)),
                             nxagentGC(pGC), x, y, string, count);
      }
    }
    else
    {
      XDrawImageString(nxagentDisplay, nxagentDrawable(pDrawable),
                           nxagentGC(pGC), x, y, string, count);
    }

    miImageText8(nxagentVirtualDrawable(pDrawable), pGC, x, y, count, string);

    return;
  }
  else
  {
    XDrawImageString(nxagentDisplay, nxagentDrawable(pDrawable),
                         nxagentGC(pGC), x, y, string, count);

    miImageText8(pDrawable, pGC, x, y, count, string);
  }
}

void nxagentImageText16(DrawablePtr pDrawable, GCPtr pGC, int x,
                            int y, int count, unsigned short *string)
{
  if (nxagentGCTrap == 1)
  {
    if ((pDrawable)->type == DRAWABLE_PIXMAP)
    {
      miImageText16(nxagentVirtualDrawable(pDrawable), pGC, x, y, count, string);
    }
    else
    {
      miImageText16(pDrawable, pGC, x, y, count, string);
    }

    return;
  }

  if ((pDrawable)->type == DRAWABLE_PIXMAP)
  {
    #ifdef TEST
    fprintf(stderr, "GCOps: GC [%p] going to image text16 on FB pixmap [%p] with string [%s].\n",
                (void *) pGC, (void *) nxagentVirtualDrawable(pDrawable), (char *) string);
    #endif

    if (nxagentPixmapIsVirtual((PixmapPtr) pDrawable))
    {
      #ifdef TEST
      fprintf(stderr, "GCOps: poly image text16 enters with virtual pixmap = [%p] parent is = [%p]\n",
                  (void *) nxagentVirtualDrawable(pDrawable),
                      (void *) nxagentRealPixmap((PixmapPtr) pDrawable));
      #endif

      if (nxagentRealPixmap((PixmapPtr) pDrawable))
      {
        XDrawImageString16(nxagentDisplay, nxagentDrawable((DrawablePtr) nxagentRealPixmap((PixmapPtr) pDrawable)),
                               nxagentGC(pGC), x, y, (XChar2b *)string, count);
      }
    }
    else
    {
      XDrawImageString16(nxagentDisplay, nxagentDrawable(pDrawable),
                             nxagentGC(pGC), x, y, (XChar2b *)string, count);
    }

    miImageText16(nxagentVirtualDrawable(pDrawable), pGC, x, y, count, string);

    return;
  }
  else
  {
    XDrawImageString16(nxagentDisplay, nxagentDrawable(pDrawable),
                           nxagentGC(pGC), x, y, (XChar2b *)string, count);

    miImageText16(pDrawable, pGC, x, y, count, string);
  }
}

void nxagentImageGlyphBlt(DrawablePtr pDrawable, GCPtr pGC, int x, int y,
                              unsigned int nGlyphs, CharInfoPtr *pCharInfo,
                                  pointer pGlyphBase)
{
  if ((pDrawable)->type == DRAWABLE_PIXMAP)
  {
    #ifdef TEST
    fprintf(stderr, "GCOps: GC [%p] going to imageGlyphBlt on FB pixmap [%p].\n",
                (void *) pGC, (void *) nxagentVirtualDrawable(pDrawable));
     #endif

    fbImageGlyphBlt(nxagentVirtualDrawable(pDrawable), pGC, x, y, nGlyphs, pCharInfo, pGlyphBase);
  }
  else
  {
    fbImageGlyphBlt(pDrawable, pGC, x, y, nGlyphs, pCharInfo, pGlyphBase);
  }
}

void nxagentPolyGlyphBlt(DrawablePtr pDrawable, GCPtr pGC, int x, int y,
                             unsigned int nGlyphs, CharInfoPtr *pCharInfo,
                                 pointer pGlyphBase)
{
  if ((pDrawable)->type == DRAWABLE_PIXMAP)
  {
    #ifdef TEST
    fprintf(stderr, "GCOps: GC [%p] going to PolyGlyphBlt on FB pixmap [%p].\n",
                (void *) pGC, (void *) nxagentVirtualDrawable(pDrawable));
    #endif

    fbPolyGlyphBlt(nxagentVirtualDrawable(pDrawable), pGC, x, y, nGlyphs, pCharInfo, pGlyphBase);
  }
  else
  {
    fbPolyGlyphBlt(pDrawable, pGC, x, y, nGlyphs, pCharInfo, pGlyphBase);
  }
}

void nxagentPushPixels(GCPtr pGC, PixmapPtr pBitmap, DrawablePtr pDrawable,
                           int width, int height, int x, int y)
{
  if ((pDrawable)->type == DRAWABLE_PIXMAP)
  {
    #ifdef TEST
    fprintf(stderr, "GCOps: GC [%p] going to PushPixels on FB pixmap [%p].\n",
                (void *) pGC, (void *) nxagentVirtualDrawable(pDrawable));
    #endif

    fbPushPixels(pGC, nxagentVirtualPixmap(pBitmap),
                     (DrawablePtr) nxagentVirtualDrawable(pDrawable),
                         width, height, x, y);
  }
  else
  {
    fbPushPixels(pGC, nxagentVirtualPixmap(pBitmap),
                     (DrawablePtr) pDrawable, width, height, x, y);
  }
}
