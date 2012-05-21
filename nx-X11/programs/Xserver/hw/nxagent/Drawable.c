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

#include "dixstruct.h"
#include "../../fb/fb.h"

#include "Agent.h"
#include "Display.h"
#include "Screen.h"
#include "Trap.h"
#include "Image.h"
#include "Drawable.h"
#include "Client.h"
#include "Visual.h"
#include "Events.h"
#include "GCs.h"
#include "Utils.h"
#include "Handlers.h"
#include "Pixels.h"
#include "Reconnect.h"
#include "GCOps.h"

#include "NXlib.h"

#include "mibstorest.h"

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG
#undef  DUMP

/*
 * The list of rectangles composing a region
 * s returned by nxagentGetOptimizedRegion-
 * Boxes() instead of REGION_RECTS().
 */

#define USE_OPTIMIZED_BOXES

/*
 * The rectangles composing a region are de-
 * fragmented to reduce the number of synch-
 * ronizing PutImage's.
 */

#define ADVANCED_BOXES_DEFRAG

/*
 * If defined, send the XClearArea at the end
 * of the loop synchronizing the shadow pixmap.
 * In this way, large images can be splitted but
 * the user will see more updates togheter.
 */

#undef  COLLECTED_UPDATES

#ifdef ADVANCED_BOXES_DEFRAG
#define INCLUDE_MARGIN 10
#endif

struct nxagentExposeBackground
{
  PixmapPtr pBackground;
  RegionPtr pExpose;
};

RESTYPE RT_NX_CORR_BACKGROUND;
RESTYPE RT_NX_CORR_WINDOW;
RESTYPE RT_NX_CORR_PIXMAP;

int nxagentCorruptedPixmaps     = 0;
int nxagentCorruptedWindows     = 0;
int nxagentCorruptedBackgrounds = 0;

int nxagentForceSynchronization = 0;

_nxagentSynchronizationRec nxagentSynchronization = { (DrawablePtr) NULL, 0, 0, 0, 0, 0 };

RegionPtr nxagentDeferredBackgroundExposures = NullRegion;

/*
 * Predicate functions used to synchronize the
 * content of the remote drawable with the data
 * stored in the virtual frame-buffer.
 */

void nxagentSynchronizeDrawablePredicate(void *p0, XID x1, void *p2);
void nxagentExposeBackgroundPredicate(void *p0, XID x1, void *p2);

/*
 * Imported from NXresource.c
 */

extern int nxagentFindClientResource(int, RESTYPE, pointer);

unsigned long nxagentGetColor(DrawablePtr pDrawable, int xPixel, int yPixel);
unsigned long nxagentGetDrawableColor(DrawablePtr pDrawable);
unsigned long nxagentGetRegionColor(DrawablePtr pDrawable, RegionPtr pRegion);

int nxagentSkipImage = 0;

static int nxagentTooManyImageData(void)
{
  unsigned int r;
  unsigned int limit;

  limit = nxagentOption(ImageRateLimit);

  r = nxagentGetDataRate() / 1000;

  #ifdef TEST
  if (r > limit)
  {
    fprintf(stderr, "Warning: Current bit rate is: %u kB/s.\n", r);
  }
  #endif

  return (r > limit);
}

int nxagentSynchronizeDrawable(DrawablePtr pDrawable, int wait, unsigned int breakMask, WindowPtr owner)
{
  int result;

  pDrawable = nxagentSplitDrawable(pDrawable);

  if (nxagentLosslessTrap == 0)
  {
    if (nxagentDrawableStatus(pDrawable) == Synchronized)
    {
      #ifdef TEST
      fprintf(stderr, "nxagentSynchronizeDrawable: Drawable [%s][%p] with id [%ld] already "
                  "synchronized.\n", nxagentDrawableType(pDrawable),
                      (void *) pDrawable, pDrawable -> id);
      #endif

      return 0;
    }
  }

  /*
   * What we want here is to avoid drawing on the
   * framebuffer and just perform the operation
   * on the real X server. This is the purpose of
   * the FB trap. At the same time we also want
   * to avoid a split, so that the image will be
   * transferred in a single operation.
   */

  nxagentFBTrap = 1;

  nxagentSplitTrap = 1;

  result = nxagentSynchronizeDrawableData(pDrawable, breakMask, owner);

  nxagentSplitTrap = 0;

  nxagentFBTrap = 0;

  if (wait == DO_WAIT && nxagentSplitResource(pDrawable) != NULL)
  {
    nxagentWaitDrawable(pDrawable);
  }

  #ifdef TEST

  if (nxagentDrawableStatus(pDrawable) == Synchronized)
  {
    fprintf(stderr, "nxagentSynchronizeDrawable: Drawable %s [%p] with id [%ld] now synchronized.\n",
                nxagentDrawableType(pDrawable), (void *) pDrawable, pDrawable -> id);
  }
  else
  {
    fprintf(stderr, "nxagentSynchronizeDrawable: Drawable %s [%p] with id [%ld] not fully synchronized.\n",
                nxagentDrawableType(pDrawable), (void *) pDrawable, pDrawable -> id);
  }

  #endif

  return result;
}

int nxagentSynchronizeDrawableData(DrawablePtr pDrawable, unsigned int breakMask, WindowPtr owner)
{
  int width, height, depth, length;
  unsigned int leftPad, format;

  char *data = NULL;
  DrawablePtr pSrcDrawable;
  GCPtr pGC;

  int success;

  if (pDrawable -> type == DRAWABLE_PIXMAP)
  {
    leftPad = 0;

    width  = pDrawable -> width;
    height = pDrawable -> height;
    depth  = pDrawable -> depth;

    #ifdef TEST
    fprintf(stderr, "nxagentSynchronizeDrawableData: Synchronizing drawable (%s) with geometry [%d][%d][%d].\n",
                nxagentDrawableType(pDrawable), width, height, depth);
    #endif

    format = (depth == 1) ? XYPixmap : ZPixmap;

    length = nxagentImageLength(width, height, format, leftPad, depth);

    if ((data = xalloc(length)) == NULL)
    {
      #ifdef WARNING
      fprintf(stderr, "nxagentSynchronizeDrawableData: WARNING! Failed to allocate memory for the operation.\n");
      #endif

      success = 0;

      goto nxagentSynchronizeDrawableDataEnd;
    }

    pSrcDrawable = (pDrawable -> type == DRAWABLE_PIXMAP ?
                        ((DrawablePtr) nxagentVirtualPixmap((PixmapPtr) pDrawable)) :
                            pDrawable);

    /*
     * Synchronize the whole pixmap if we need
     * to download a fresh copy with lossless
     * compression turned off.
     */

    if (nxagentLosslessTrap == 1)
    {
      pGC = nxagentGetGraphicContext(pDrawable);

      if (pGC == NULL)
      {
        #ifdef WARNING
        fprintf(stderr, "nxagentSynchronizeDrawableData: WARNING! Failed to get the temporary GC.\n");
        #endif

        success = 0;

        goto nxagentSynchronizeDrawableDataFree;
      }

      ValidateGC(pDrawable, pGC);

      fbGetImage(pSrcDrawable, 0, 0,
                     width, height, format, AllPlanes, data);

      #ifdef TEST
      fprintf(stderr, "nxagentSynchronizeDrawableData: Forcing synchronization of "
                  "pixmap at [%p] with lossless compression.\n", (void *) pDrawable);
      #endif

      nxagentPutImage(pDrawable, pGC, depth, 0, 0,
                          width, height, leftPad, format, data);

      success = 1;

      goto nxagentSynchronizeDrawableDataFree;
    }
    else if (nxagentReconnectTrap == 1)
    {
      /*
       * The pixmap data is not synchronized unless
       * we need it. We noticed we have to reconnect
       * the pixmaps used by the GC's clip mask.
       * The other data will be synchronized on demand.
       */

      if (pDrawable -> depth == 1)
      {
        #ifdef TEST

        if (nxagentReconnectTrap == 1)
        {
          static int totalLength;
          static int totalReconnectedPixmaps;

          totalLength += length;
          totalReconnectedPixmaps++;

          fprintf(stderr, "nxagentSynchronizeDrawableData: Reconnecting pixmap at [%p] [%dx%d] "
                      "Depth [%d] Size [%d]. Total size [%d]. Total reconnected pixmaps [%d].\n", 
                          (void *) pDrawable, width, height, depth, length,
                              totalLength, totalReconnectedPixmaps);
        }

        #endif

        pGC = nxagentGetGraphicContext(pDrawable);

        if (pGC == NULL)
        {
          #ifdef WARNING
          fprintf(stderr, "nxagentSynchronizeDrawableData: WARNING! Failed to create the temporary GC.\n");
          #endif

          success = 0;

          goto nxagentSynchronizeDrawableDataFree;
        }

        ValidateGC(pDrawable, pGC);

        fbGetImage(pSrcDrawable, 0, 0,
                       width, height, format, AllPlanes, data);

        nxagentPutImage(pDrawable, pGC, depth, 0, 0,
                            width, height, leftPad, format, data);

        success = 1;

        goto nxagentSynchronizeDrawableDataFree;
      }
      else
      {
        #ifdef TEST
        fprintf(stderr, "nxagentSynchronizeDrawableData: Skipping synchronization of "
                    "pixmap at [%p][%p] during reconnection.\n", (void *) pDrawable, (void*) nxagentVirtualPixmap((PixmapPtr)pDrawable));
        #endif

        nxagentMarkCorruptedRegion(pDrawable, NullRegion);

        success = 1;

        goto nxagentSynchronizeDrawableDataFree;
      }
    }
  }

  /*
   * By calling this function with the NullRegion
   * as parameter we are requesting to synchro-
   * nize the full visible corrupted region of
   * the drawable.
   */

  success = nxagentSynchronizeRegion(pDrawable, NullRegion, breakMask, owner);

nxagentSynchronizeDrawableDataFree:

  if (data != NULL)
  {
    xfree(data);
  }

nxagentSynchronizeDrawableDataEnd:

  return success;
}

/*
 * If pRegion is NullRegion, all the viewable
 * corrupted region will be synchronized.
 */

int nxagentSynchronizeRegion(DrawablePtr pDrawable, RegionPtr pRegion, unsigned int breakMask, WindowPtr owner)
{
  GCPtr pGC;
  DrawablePtr pSrcDrawable;
  BoxPtr pBox;
  RegionPtr clipRegion;

  RegionRec tileRegion;
  RegionRec exposeRegion;
  BoxRec box;
  BoxRec tileBox;

  #ifdef COLLECTED_UPDATES
  RegionRec collectedUpdates;
  #endif

  char *data;

  int nBox;
  int x, y;
  int w, h;
  int extentWidth, extentHeight;
  int tileWidth, tileHeight;
  int length, format, leftPad;
  int i;
  int saveTrap;
  int success;
  int useStoredBitmap;

  unsigned long now;
  unsigned long elapsedTime;


  leftPad    = 0;
  success    = 0;
  data       = NULL;
  pGC        = NULL;
  clipRegion = NullRegion;

  #ifdef COLLECTED_UPDATES
  REGION_INIT(pDrawable -> pScreen, &collectedUpdates, NullBox, 1);
  #endif

  REGION_INIT(pDrawable -> pScreen, &exposeRegion, NullBox, 1);

  if (nxagentDrawableBitmap(pDrawable) != NullPixmap &&
          nxagentDrawableStatus((DrawablePtr) nxagentDrawableBitmap(pDrawable)) == Synchronized)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentSynchronizeRegion: WARNING! Drawable [%s] at [%p] has an already synchronized "
                "bitmap at [%p].\n", nxagentDrawableType(pDrawable),
                    (void *) pDrawable, (void *) nxagentDrawableBitmap(pDrawable));
    #endif

    nxagentDestroyDrawableBitmap(pDrawable);
  }

  /*
   * The stored bitmap may be used if we
   * are going to synchronize the full
   * drawable.
   */

  useStoredBitmap = (nxagentDrawableBitmap(pDrawable) != NullPixmap && pRegion == NullRegion);

  if (useStoredBitmap != 0)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentSynchronizeRegion: Drawable [%s] at [%p] has a synchronization bitmap at [%p] "
                "[%d,%d,%d,%d] with [%ld] rects.\n", nxagentDrawableType(pDrawable),
                    (void *) pDrawable, (void *) nxagentDrawableBitmap(pDrawable),
                        nxagentCorruptedRegion((DrawablePtr) nxagentDrawableBitmap(pDrawable)) -> extents.x1,
                            nxagentCorruptedRegion((DrawablePtr) nxagentDrawableBitmap(pDrawable)) -> extents.y1,
                                nxagentCorruptedRegion((DrawablePtr) nxagentDrawableBitmap(pDrawable)) -> extents.x2,
                                    nxagentCorruptedRegion((DrawablePtr) nxagentDrawableBitmap(pDrawable)) -> extents.y2,
                                        REGION_NUM_RECTS(nxagentCorruptedRegion((DrawablePtr) nxagentDrawableBitmap(pDrawable))));
    #endif

    clipRegion = nxagentCreateRegion(pDrawable, NULL, 0, 0, pDrawable -> width, pDrawable -> height);

    /*
     * Intersecting the viewable region of the
     * drawable with the region remaining from
     * a previous loop.
     */

    REGION_INTERSECT(pDrawable -> pScreen, clipRegion, clipRegion,
                         nxagentCorruptedRegion((DrawablePtr) nxagentDrawableBitmap(pDrawable)));

    /*
     * The bitmap regions used in the synchro-
     * nizations are only those corrupted also
     * on the drawable. In this way, if we put
     * a tile in a bad position (e.g. if the
     * corrupted region moves), the next synch-
     * ronization will fix the error.
     */

    REGION_INTERSECT(pDrawable -> pScreen, clipRegion, clipRegion,
                         nxagentCorruptedRegion(pDrawable));

    /*
     * The bitmap to synchronize is clipped.
     */

    if (REGION_NIL(clipRegion) == 1)
    {
      #ifdef TEST
      fprintf(stderr, "nxagentSynchronizeRegion: The bitmap region [%d,%d,%d,%d] is not viewable. "
                  "Destroying it.\n", clipRegion -> extents.x1, clipRegion -> extents.y1,
                      clipRegion -> extents.x2, clipRegion -> extents.y2);
      #endif

      nxagentDestroyDrawableBitmap(pDrawable);

      goto nxagentSynchronizeRegionFree;
    }

    /*
     * Using the saved bitmap as source, instead
     * of the drawable itself.
     */

    pSrcDrawable = ((DrawablePtr) nxagentVirtualPixmap(nxagentDrawableBitmap(pDrawable)));
  }
  else
  {
    if (pRegion != NullRegion && REGION_NIL(pRegion) == 1)
    {
      #ifdef TEST
      fprintf(stderr, "nxagentSynchronizeRegion: Region [%d,%d,%d,%d] is nil. Skipping synchronization.\n",
                  pRegion -> extents.x1, pRegion -> extents.y1, pRegion -> extents.x2, pRegion -> extents.y2);
      #endif

      goto nxagentSynchronizeRegionFree;
    }

    if (nxagentDrawableStatus(pDrawable) == Synchronized)
    {
      #ifdef TEST
      fprintf(stderr, "nxagentSynchronizeRegion: The [%s] at [%p] is already synchronized.\n",
                  nxagentDrawableType(pDrawable), (void *) pDrawable);
      #endif

      goto nxagentSynchronizeRegionFree;
    }

    /*
     * Creating a region containing the viewable
     * area of drawable.
     */

    clipRegion = nxagentCreateRegion(pDrawable, NULL, 0, 0, pDrawable -> width, pDrawable -> height);

    /*
     * If the corrupted region is not viewable, we
     * can skip the synchronization.
     */

    REGION_INTERSECT(pDrawable -> pScreen, clipRegion, clipRegion, nxagentCorruptedRegion(pDrawable));

    if (REGION_NIL(clipRegion) == 1)
    {
      #ifdef TEST
      fprintf(stderr, "nxagentSynchronizeRegion: The corrupted region [%d,%d,%d,%d] is not viewable "
                  "on [%s] at [%p]. Skipping the synchronization.\n", clipRegion -> extents.x1,
                      clipRegion -> extents.y1, clipRegion -> extents.x2, clipRegion -> extents.y2,
                          nxagentDrawableType(pDrawable), (void *) pDrawable);
      #endif

     goto nxagentSynchronizeRegionFree; 
    }

    /*
     * We can skip the synchronization if the re-
     * quested region is not corrupted. Specifying
     * a NullRegion as parameter, all the viewable
     * corrupted region will be synchronized.
     */

    if (pRegion != NullRegion)
    {
      REGION_INTERSECT(pDrawable -> pScreen, clipRegion, clipRegion, pRegion);

      if (REGION_NIL(clipRegion) == 1)
      {
        #ifdef TEST
        fprintf(stderr, "nxagentSynchronizeRegion: Region requested [%d,%d,%d,%d] already "
                    "synchronized on [%s] at [%p].\n", pRegion -> extents.x1,
                        pRegion -> extents.y1, pRegion -> extents.x2, pRegion -> extents.y2,
                            nxagentDrawableType(pDrawable), (void *) pDrawable);
        #endif

        goto nxagentSynchronizeRegionFree;
      }
    }

    pSrcDrawable = (pDrawable -> type == DRAWABLE_PIXMAP ?
                      ((DrawablePtr) nxagentVirtualPixmap((PixmapPtr) pDrawable)) :
                          pDrawable);
  }

  #ifdef TEST
  fprintf(stderr, "nxagentSynchronizeRegion: Synchronizing region with coordinates [%d,%d,%d,%d] "
              "on [%s] at [%p].\n", clipRegion -> extents.x1, clipRegion -> extents.y1,
                  clipRegion -> extents.x2, clipRegion -> extents.y2,
                      nxagentDrawableType(pDrawable), (void *) pDrawable);
  #endif

  saveTrap = nxagentGCTrap;

  nxagentGCTrap = 0;

  nxagentFBTrap = 1;

  nxagentSplitTrap = 1;

  pGC = nxagentGetGraphicContext(pDrawable);

  if (pGC == NULL)
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentSynchronizeRegion: WARNING! Failed to create the temporary GC.\n");
    #endif

    goto nxagentSynchronizeRegionFree;
  }

  ValidateGC(pDrawable, pGC);

  #ifdef TEST
  fprintf(stderr, "nxagentSynchronizeRegion: Going to synchronize [%ld] rects of [%s] at [%p].\n",
              REGION_NUM_RECTS(clipRegion), nxagentDrawableType(pDrawable), (void *) pDrawable);

  fprintf(stderr, "nxagentSynchronizeRegion: Extents geometry [%d,%d,%d,%d].\n",
          clipRegion -> extents.x1, clipRegion -> extents.y1, clipRegion -> extents.x2, clipRegion -> extents.y2);

  fprintf(stderr, "nxagentSynchronizeRegion: Drawable geometry [%d,%d,%d,%d].\n",
              pDrawable -> x, pDrawable -> y, pDrawable -> width, pDrawable -> height);
  #endif

  /*
   * We are going to synchronize the corrupted
   * area, so we use the corrupted extents as
   * maximum size of the image data. It's im-
   * portant to avoid using the drawable size,
   * because in case of a huge window it had to
   * result in a failed data memory allocation.
   */

  extentWidth  = clipRegion -> extents.x2 - clipRegion -> extents.x1;
  extentHeight = clipRegion -> extents.y2 - clipRegion -> extents.y1;

  w = tileWidth  = (nxagentOption(TileWidth)  > extentWidth  ? extentWidth  : nxagentOption(TileWidth));
  h = tileHeight = (nxagentOption(TileHeight) > extentHeight ? extentHeight : nxagentOption(TileHeight));

  #ifdef DEBUG
  fprintf(stderr, "nxagentSynchronizeRegion: Using tiles of size [%dx%d].\n", tileWidth, tileHeight);
  #endif

  data = nxagentAllocateImageData(w, h, pDrawable -> depth, &length, &format);

  if (data == NULL)
  {
    #ifdef WARNING

    fprintf(stderr, "nxagentSynchronizeRegion: WARNING! Failed to allocate memory for synchronization.\n");

    /*
     * Print detailed informations if the
     * image length is zero.
     */

    if (length == 0)
    {
      fprintf(stderr, "nxagentSynchronizeRegion: Drawable [%s] at [%p] with region geometry [%ld][%d,%d,%d,%d].\n",
                  nxagentDrawableType(pDrawable), (void *) pDrawable, REGION_NUM_RECTS(clipRegion),
                      clipRegion -> extents.x1, clipRegion -> extents.y1,
                          clipRegion -> extents.x2, clipRegion -> extents.y2);
    }

    #endif

    goto nxagentSynchronizeRegionFree;
  }

  #ifndef USE_OPTIMIZED_BOXES

  pBox = REGION_RECTS(clipRegion);

  #else

  pBox = nxagentGetOptimizedRegionBoxes(clipRegion);

  #endif /* USE_OPTIMIZED_BOXES */

  nBox = REGION_NUM_RECTS(clipRegion);

  now = GetTimeInMillis();

  nxagentSynchronization.abort = 0;

  /*
   * Going to split the updated region into small blocks.
   */

  for (i = 0; i < nBox; i++)
  {
    #ifdef USE_OPTIMIZED_BOXES

    if (pBox[i].x1 == 0 && pBox[i].y1 == 0 &&
            pBox[i].x2 == 0 && pBox[i].y2 == 0)
    {
      continue;
    }

    #endif

    box = pBox[i];

    for (y = box.y1; y < box.y2; y += h)
    {
      h = MIN(box.y2 - y, tileHeight);

      for (x = box.x1; x < box.x2; x += w)
      {
        w = MIN(box.x2 - x, tileWidth);

        /*
         * FIXME: This should not occur.
         */

        if (nxagentDrawableStatus(pDrawable) == Synchronized)
        {
          #ifdef WARNING

          if (pDrawable -> type == DRAWABLE_WINDOW && pSrcDrawable != pDrawable)
          {
            fprintf(stderr, "nxagentSynchronizeRegion: WARNING! Trying to synchronize "
                        "the clean drawable type [%d] at [%p] with source at [%p].\n",
                            pDrawable -> type, (void *) pDrawable, (void *) pSrcDrawable);
          }

          #endif

          goto nxagentSynchronizeRegionStop;
        }

        if (canBreakOnTimeout(breakMask))
        {
          /*
           * Abort the synchronization loop if it
           * lasts for more than DeferTimeout
           * milliseconds.
           */

          elapsedTime = GetTimeInMillis() - now;

          if (elapsedTime > nxagentOption(DeferTimeout))
          {
            #ifdef TEST
            fprintf(stderr, "nxagentSynchronizeRegion: Synchronization break with "
                        "[%lu] ms elapsed.\n", elapsedTime);
            #endif

            nxagentSynchronization.abort = 1;

            goto nxagentSynchronizeRegionStop;
          }
        }

        /*
         * Abort the loop if we go out of bandwidth.
         */

        if (breakOnCongestionDrawable(breakMask, pDrawable) == 1)
        {
          #ifdef TEST
          fprintf(stderr, "nxagentSynchronizeRegion: Synchronization break with "
                      "congestion [%d] blocking [%d].\n", nxagentCongestion,
                          nxagentBlocking);
          #endif

          nxagentSynchronization.abort = 1;

          goto nxagentSynchronizeRegionStop;
        }

        /*
         * Abort the loop if the display blocks.
         */

        if (breakOnBlocking(breakMask) == 1)
        {
          #ifdef TEST
          fprintf(stderr, "nxagentSynchronizeRegion: Synchronization break with "
                      "blocking [%d] congestion [%d].\n", nxagentBlocking,
                          nxagentCongestion);
          #endif

          nxagentSynchronization.abort = 1;

          goto nxagentSynchronizeRegionStop;
        }

        tileBox.x1 = x;
        tileBox.y1 = y;
        tileBox.x2 = x + w;
        tileBox.y2 = y + h;

        #ifdef DEBUG
        fprintf(stderr, "nxagentSynchronizeRegion: Going to synchronize tile [%d,%d,%d,%d].\n",
                    tileBox.x1, tileBox.y1, tileBox.x2, tileBox.y2);
        #endif

        nxagentGetImage(pSrcDrawable, x, y, w, h, format, AllPlanes, data);

        /*
         * Going to unmark the synchronized
         * region.
         */

        REGION_INIT(pDrawable -> pScreen, &tileRegion, &tileBox, 1);

        REGION_UNION(pDrawable -> pScreen, &exposeRegion, &exposeRegion, &tileRegion);

        #ifdef COLLECTED_UPDATES
        REGION_APPEND(pDrawable -> pScreen, &collectedUpdates, &tileRegion);
        #endif

        if (useStoredBitmap != 0)
        {
          /*
           * When a bitmap's tile is synchronized,
           * we can clear the corresponding region.
           * We can't use the nxagentUnmarkCorrupted-
           * Region because we have not a resource
           * associated to this pixmap.
           */

          REGION_SUBTRACT(pDrawable -> pScreen, nxagentPixmapCorruptedRegion(nxagentDrawableBitmap(pDrawable)),
                              nxagentPixmapCorruptedRegion(nxagentDrawableBitmap(pDrawable)), &tileRegion); 

          /*
           * The drawable's corrupted region can
           * be cleared if the bitmap's tile data
           * matches the drawable's content at the
           * same position.
           */

          if (nxagentDrawableStatus(pDrawable) == NotSynchronized)
          {
            char *cmpData;

            int cmpLength, cmpFormat;

            cmpData = nxagentAllocateImageData(w, h, pDrawable -> depth, &cmpLength, &cmpFormat);

            if (cmpData != NULL)
            {
              nxagentGetImage(pDrawable, x, y, w, h, format, AllPlanes, cmpData);

              if (memcmp(data, cmpData, cmpLength) == 0)
              {
                #ifdef TEST
                fprintf(stderr, "nxagentSynchronizeRegion: Tile [%d,%d,%d,%d] matches drawable's data at same position.\n",
                            x, y, x + w, y + h);
                #endif

                nxagentUnmarkCorruptedRegion(pDrawable, &tileRegion);
              }
              #ifdef TEST
              else
              {
                fprintf(stderr, "nxagentSynchronizeRegion: Tile [%d,%d,%d,%d] on drawable [%p] doesn't match.\n",
                            x, y, x + w, y + h, (void *) pDrawable);
              }
              #endif
            }
            else
            {
              #ifdef WARNING
              fprintf(stderr, "nxagentSynchronizeRegion: WARNING! Failed to allocate memory to compare tiles.\n");
              #endif
            }

            if (cmpData != NULL)
            {
              xfree(cmpData);
            }
          }
        }
        else
        {
          nxagentUnmarkCorruptedRegion(pDrawable, &tileRegion);

          if (nxagentDrawableBitmap(pDrawable) != NullPixmap)
          {
            #ifdef TEST
            fprintf(stderr, "nxagentSynchronizeRegion: Going to clean bitmap at [%p] with newer data.\n",
                        (void *) nxagentDrawableBitmap(pDrawable));
            #endif

            REGION_SUBTRACT(pDrawable -> pScreen, nxagentPixmapCorruptedRegion(nxagentDrawableBitmap(pDrawable)),
                                nxagentPixmapCorruptedRegion(nxagentDrawableBitmap(pDrawable)), &tileRegion);
          }
        }

        /*
         * Realize the image after comparing the
         * source data with the bitmap data.
         */

        nxagentRealizeImage(pDrawable, pGC, pDrawable -> depth,
                                x, y, w, h, leftPad, format, data);

        REGION_UNINIT(pDrawable -> pScreen, &tileRegion);

        #if !defined(COLLECTED_UPDATES)

        if (owner != NULL)
        {
          if (nxagentOption(Shadow) == 1 &&
                  (nxagentOption(XRatio) != DONT_SCALE ||
                      nxagentOption(YRatio) != DONT_SCALE))
          {
            int scaledx;
            int scaledy;
            int scaledw;
            int scaledh;

            scaledx = nxagentScale(x, nxagentOption(XRatio));
            scaledy = nxagentScale(y, nxagentOption(YRatio));

            scaledw = nxagentScale(x + w, nxagentOption(XRatio)) - scaledx;
            scaledh = nxagentScale(y + h, nxagentOption(YRatio)) - scaledy;

            XClearArea(nxagentDisplay, nxagentWindow(owner), scaledx, scaledy, scaledw, scaledh, 0);
          }
          else
          {
            XClearArea(nxagentDisplay, nxagentWindow(owner), x, y, w, h, 0);
          }
        }

        #endif /* #if !defined(COLLECTED_UPDATES) */

        /*
         * Abort the loop on the user's input.
         * This is done here to check for events
         * read after the flush caused by the
         * PutImage.
         */

        nxagentDispatchHandler((ClientPtr) 0, 0, 0);

        if (breakOnEvent(breakMask) == 1)
        {
          #ifdef TEST
          fprintf(stderr, "nxagentSynchronizeRegion: Synchronization break with "
                      "new input events.\n");
          #endif

          nxagentSynchronization.abort = 1;

          goto nxagentSynchronizeRegionStop;
        }
      }
    }
  }

nxagentSynchronizeRegionStop:

  nxagentSplitTrap = 0;

  nxagentFBTrap = 0;

  nxagentGCTrap = saveTrap;

  success = 1;

  if (nxagentOption(Shadow) == 0)
  {
    if (nxagentSynchronization.abort == 1)
    {
      /*
       * Storing the pointer to the drawable we
       * were synchronizing when the loop aborted.
       * It is used in nxagentSynchronizeDrawable-
       * Predicate.
       */

      nxagentSynchronization.pDrawable = pDrawable;
      nxagentSynchronization.drawableType = pDrawable -> type;

      if (nxagentDrawableBitmap(pDrawable) == NullPixmap)
      {
        #ifdef TEST
        fprintf(stderr, "nxagentSynchronizeRegion: Going to create the synchronization bitmap.\n");
        #endif

        nxagentCreateDrawableBitmap(pDrawable);
      }
    }
    else
    {
      if (nxagentDrawableBitmap(pDrawable) != NullPixmap)
      {
        #ifdef TEST
        fprintf(stderr, "nxagentSynchronizeRegion: Synchronization loop finished. Going to destroy synchronization bitmap.\n");
        #endif

        nxagentDestroyDrawableBitmap(pDrawable);
      }
    }

    if (pDrawable -> type == DRAWABLE_PIXMAP &&
            nxagentIsCorruptedBackground((PixmapPtr) pDrawable) == 1 &&
                REGION_NIL(&exposeRegion) == 0)
    {
      struct nxagentExposeBackground eb;

      int i;

      eb.pBackground = (PixmapPtr) pDrawable;
      eb.pExpose = &exposeRegion;

      for (i = 0; i < MAXCLIENTS; i++)
      {
        if (clients[i] != NULL)
        {
          FindClientResourcesByType(clients[i], RT_WINDOW,
                                        nxagentExposeBackgroundPredicate, &eb);
        }
      }
    }
  }
  #ifdef COLLECTED_UPDATES
  else
  {
    if (owner != NULL)
    {
      int overlap = 0;

      REGION_VALIDATE(pDrawable -> pScreen, &collectedUpdates, &overlap);

      for (i = 0; i < REGION_NUM_RECTS(&collectedUpdates); i++)
      {
        x = REGION_RECTS(&collectedUpdates)[i].x1;
        y = REGION_RECTS(&collectedUpdates)[i].y1;
        w = REGION_RECTS(&collectedUpdates)[i].x2 - REGION_RECTS(&collectedUpdates)[i].x1;
        h = REGION_RECTS(&collectedUpdates)[i].y2 - REGION_RECTS(&collectedUpdates)[i].y1;
       
        if (nxagentOption(Shadow) == 1 &&
                (nxagentOption(XRatio) != DONT_SCALE ||
                    nxagentOption(YRatio) != DONT_SCALE))
        {
          int scaledx;
          int scaledy;
          int scaledw;
          int scaledh;

          scaledx = nxagentScale(x, nxagentOption(XRatio));
          scaledy = nxagentScale(y, nxagentOption(YRatio));

          scaledw = nxagentScale(x + w, nxagentOption(XRatio)) - scaledx;
          scaledh = nxagentScale(y + h, nxagentOption(YRatio)) - scaledy;

          XClearArea(nxagentDisplay, nxagentWindow(owner), scaledx, scaledy, scaledw, scaledh, 0);
        }
        else
        {
          XClearArea(nxagentDisplay, nxagentWindow(owner), x, y, w, h, 0);
        }
      }
    }
  }
  #endif /* #ifdef COLLECTED_UPDATES */

nxagentSynchronizeRegionFree:

  if (clipRegion != NullRegion)
  {
    nxagentFreeRegion(pDrawable, clipRegion);
  }

  if (data != NULL)
  {
    xfree(data);
  }

  REGION_UNINIT(pDrawable -> pScreen, &exposeRegion);

  #ifdef COLLECTED_UPDATES

  REGION_UNINIT(pDrawable -> pScreen, &collectedUpdates);

  #endif /* #ifdef COLLECTED_UPDATES */

  return success;
}

void nxagentSynchronizeBox(DrawablePtr pDrawable, BoxPtr pBox, unsigned int breakMask)
{
  RegionPtr pRegion;

  if (nxagentDrawableStatus(pDrawable) == Synchronized)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentSynchronizeBox: The [%s] at [%p] is already synchronized.\n",
                nxagentDrawableType(pDrawable), (void *) pDrawable);
    #endif

    return;
  }

  if (pBox == NullBox)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentSynchronizeBox: Going to synchronize the whole [%s] at [%p].\n",
                nxagentDrawableType(pDrawable), (void *) pDrawable);
    #endif

    nxagentSynchronizeRegion(pDrawable, NullRegion, breakMask, NULL);
  }
  else
  {
    #ifdef TEST
    fprintf(stderr, "nxagentSynchronizeBox: Going to create a region from box [%d,%d,%d,%d].\n",
                pBox -> x1, pBox -> y1, pBox -> x2, pBox -> y2);
    #endif

    pRegion = nxagentCreateRegion(pDrawable, NULL, pBox -> x1, pBox -> y1,
                                      pBox -> x2 - pBox -> x1, pBox -> y2 - pBox -> y1);


    if (REGION_NIL(pRegion) == 1)
    {
      #ifdef TEST
      fprintf(stderr, "nxagentSynchronizeBox: Resulting region [%d,%d,%d,%d] is nil. Skipping synchronization.\n",
                  pRegion -> extents.x1, pRegion -> extents.y1, pRegion -> extents.x2, pRegion -> extents.y2);
      #endif

      nxagentFreeRegion(pDrawable, pRegion);

      return;
    }

    #ifdef TEST
    fprintf(stderr, "nxagentSynchronizeBox: Going to synchronize the region [%d,%d,%d,%d] of "
                "[%s] at [%p].\n", pRegion -> extents.x1, pRegion -> extents.y1, pRegion -> extents.x2,
                    pRegion -> extents.y2, nxagentDrawableType(pDrawable), (void *) pDrawable);
    #endif

    nxagentSynchronizeRegion(pDrawable, pRegion, breakMask, NULL);

    nxagentFreeRegion(pDrawable, pRegion);
  }
}

void nxagentSynchronizeDrawablePredicate(void *p0, XID x1, void *p2)
{
  DrawablePtr pDrawable = (DrawablePtr) p0;
  unsigned int *breakMask = (unsigned int *) p2;

  int shouldClearHiddenRegion = 1;

  /*
   * The nxagentSynchronization.abort propa-
   * gates a break condition across the resour-
   * ces loop, in order to block also the sub-
   * sequent synchronizations.
   */

  if (nxagentSynchronization.abort == 1 ||
          nxagentDrawableStatus(pDrawable) == Synchronized)
  {
    return;
  }

  /*
   * In order to implement a kind of round-robin
   * synchronization, the previous incomplete
   * drawable synchronization is saved to jump
   * to the next resource available of same type.
   */

  if (nxagentSynchronization.pDrawable != NULL &&
          pDrawable -> type == nxagentSynchronization.drawableType)
  {
    if (nxagentSynchronization.pDrawable != pDrawable)
    {
      #ifdef TEST
      fprintf(stderr, "nxagentSynchronizeDrawablePredicate: Skipping drawable [%s][%p] while looking "
                  "for last synchronized drawable [%p].\n", nxagentDrawableType(pDrawable),
                      (void *) pDrawable, (void *) nxagentSynchronization.pDrawable);
      #endif

      return;
    }
    else
    {
      #ifdef TEST
      fprintf(stderr, "nxagentSynchronizeDrawablePredicate: Last synchronized drawable [%p] found. "
                  "Skipping to the next resource.\n", (void *) nxagentSynchronization.pDrawable);
      #endif

      nxagentSynchronization.pDrawable = NULL;

      return;
    }
  }

  if (pDrawable -> type == DRAWABLE_PIXMAP)
  {
    /*
     * The pixmaps to be synchronized are those
     * used as background or used as source of
     * any deferred operations for at least 2
     * times.
     */

    if (NXAGENT_SHOULD_SYNCHRONIZE_PIXMAP(pDrawable) == 0)
    {
      #ifdef TEST
      fprintf(stderr, "nxagentSynchronizeDrawablePredicate: Skipping pixmap at [%p] "
                  "with usage [%d] background [%d].\n", (void *) pDrawable,
                      nxagentPixmapUsageCounter((PixmapPtr) pDrawable),
                          nxagentIsCorruptedBackground((PixmapPtr) pDrawable));
      #endif

      return;
    }
    #ifdef TEST
    else
    {
      fprintf(stderr, "nxagentSynchronizeDrawablePredicate: Synchronizing pixmap at [%p] "
                  "with usage [%d] background [%d].\n", (void *) pDrawable,
                      nxagentPixmapUsageCounter((PixmapPtr) pDrawable),
                          nxagentIsCorruptedBackground((PixmapPtr) pDrawable));
    }
    #endif
  }
  else if (NXAGENT_SHOULD_SYNCHRONIZE_WINDOW(pDrawable) == 0)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentSynchronizeDrawablePredicate: Skipping not visible window at [%p].\n",
                (void *) pDrawable);
    #endif

    if (shouldClearHiddenRegion == 1)
    {
      #ifdef TEST
      fprintf(stderr, "nxagentSynchronizeDrawablePredicate: Clearing out the not visible window "
                  "at [%p].\n", (void *) pDrawable);
      #endif

      nxagentCleanCorruptedDrawable(pDrawable);
    }

    return;
  }

  /*
   * Postpone the synchronization if we went
   * out of bandwidth or if the display blocks.
   * The pixmap synchronization is more careful
   * with bandwidth usage.
   */

/*
FIXME: This condition sounds only as a
       complication, as the break parameters
       are already checked while synchroni-
       zing the drawable.

  if (breakOnCongestion(*breakMask) == 1 ||
          (pDrawable -> type == DRAWABLE_PIXMAP &&
               *breakMask != NEVER_BREAK && nxagentCongestion > 0))
  {
    #ifdef TEST
    fprintf(stderr, "nxagentSynchronizeDrawablePredicate: WARNING! Breaking the "
                "synchronization with congestion [%d] blocking [%d].\n",
                    nxagentCongestion, nxagentBlocking);
    #endif

    nxagentSynchronization.abort = 1;

    return;
  }
*/

  #ifdef TEST
  fprintf(stderr, "nxagentSynchronizeDrawablePredicate: Synchronizing drawable [%s][%p] "
              "with geometry (%dx%d).\n", nxagentDrawableType(pDrawable),
                  (void *) pDrawable, pDrawable -> width, pDrawable -> height);

  fprintf(stderr, "nxagentSynchronizeDrawablePredicate: Corrupted extents [%d,%d,%d,%d] "
              "with [%ld] rects.\n", nxagentCorruptedRegion(pDrawable) -> extents.x1,
                  nxagentCorruptedRegion(pDrawable) -> extents.y1, nxagentCorruptedRegion(pDrawable) ->
                      extents.x2, nxagentCorruptedRegion(pDrawable) -> extents.y2,
                          REGION_NUM_RECTS(nxagentCorruptedRegion(pDrawable)));
  #endif

  /*
   * The stored bitmap is destroyed inside
   * the synchronization loop, so we have
   * to check here its presence to know if
   * we can clear the dirty windows.
   */

  shouldClearHiddenRegion = (nxagentDrawableBitmap(pDrawable) == NullPixmap);

  nxagentSynchronizeDrawable(pDrawable, DONT_WAIT, *breakMask, NULL);

  if (nxagentDrawableStatus(pDrawable) == NotSynchronized)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentSynchronizeDrawablePredicate: Drawable [%s][%p] not fully synchronized.\n",
                nxagentDrawableType(pDrawable), (void *) pDrawable);
    #endif

    /*
     * If the remaining corrupted region is on
     * an hidden section (not viewable or outside
     * of the pixmap's area) of a drawable,
     * we can clear it.
     */

    if (nxagentSynchronization.abort == 0 &&
            shouldClearHiddenRegion == 1)
    {
      #ifdef TEST
      fprintf(stderr, "nxagentSynchronizeDrawablePredicate: Clearing out the remaining corrupted "
                  "[%s] at [%p].\n", nxagentDrawableType(pDrawable), (void *) pDrawable);
      #endif

      nxagentCleanCorruptedDrawable(pDrawable);
    }
  }
}

void nxagentSynchronizationLoop(unsigned int mask)
{
  unsigned int breakMask;

  int doRoundRobin;

/*
FIXME: All drawables should be set as synchronized and
       never marked as corrupted while the display is
       down.
*/

  nxagentSkipImage = nxagentTooManyImageData();

  if (nxagentOption(ImageRateLimit) && nxagentSkipImage)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentSynchronizeDrawable: Skipping due to bit rate limit reached.\n");
    #endif

    return;
  }

  if (NXDisplayError(nxagentDisplay) == 1)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentSynchronizationLoop: WARNING! Not synchronizing the drawables "
                "with the display down.\n");
    #endif

    return;
  }

  #ifdef TEST
  fprintf(stderr, "nxagentSynchronizationLoop: Synchronizing [%d] windows [%d] pixmaps "
              "and [%d] backgrounds with mask [%u].\n", nxagentCorruptedWindows, nxagentCorruptedPixmaps,
                  nxagentCorruptedBackgrounds, mask);

  fprintf(stderr, "nxagentSynchronizationLoop: Stored bitmaps [%d] windows [%d] pixmaps "
              "and [%d] backgrounds.\n", nxagentSynchronization.windowBitmaps,
                  nxagentSynchronization.pixmapBitmaps, nxagentSynchronization.backgroundBitmaps);

  fprintf(stderr, "nxagentSynchronizationLoop: Starting loops with congestion [%d] "
              "blocking [%d].\n", nxagentCongestion, nxagentBlocking);
  #endif

  breakMask = mask;

  /*
   * The resource counter can be reset if we
   * have not aborted the synchronization loop,
   * if we are not skipping resources to do
   * round-robin and if the bitmaps are all
   * synchronized.
   */

  doRoundRobin = (nxagentSynchronization.pDrawable != NULL);

  nxagentSynchronization.abort = 0;

  /*
   * Synchronize the windows.
   */

  if (NXAGENT_SHOULD_SYNCHRONIZE_CORRUPTED_WINDOWS(mask))
  {
    #ifdef TEST
    fprintf(stderr, "nxagentSynchronizationLoop: Going to loop through corrupted window resources.\n");
    #endif

    FindClientResourcesByType(clients[serverClient -> index], RT_NX_CORR_WINDOW,
                                  nxagentSynchronizeDrawablePredicate, &breakMask);

    #ifdef TEST

    if (nxagentSynchronization.abort == 0 &&
            nxagentSynchronization.windowBitmaps == 0 &&
                doRoundRobin == 0)
    {
      if (nxagentCorruptedWindows > 0)
      {
        fprintf(stderr, "nxagentSynchronizationLoop: Closing the loop with [%d] "
                    "corrupted windows.\n", nxagentCorruptedWindows);
      }

      nxagentCorruptedWindows = 0;
    }

    #endif
  }

  /*
   * Synchronize the backgrounds.
   */

  if (nxagentSynchronization.abort == 0 &&
          NXAGENT_SHOULD_SYNCHRONIZE_CORRUPTED_BACKGROUNDS(mask))
  {
    #ifdef TEST
    fprintf(stderr, "nxagentSynchronizationLoop: Going to loop through corrupted background resources.\n");
    #endif

    FindClientResourcesByType(clients[serverClient -> index], RT_NX_CORR_BACKGROUND,
                                  nxagentSynchronizeDrawablePredicate, &breakMask);

    #ifdef TEST

    if (nxagentSynchronization.abort == 0 &&
            nxagentSynchronization.backgroundBitmaps == 0 &&
                doRoundRobin == 0)
    {
      if (nxagentCorruptedBackgrounds > 0)
      {
        fprintf(stderr, "nxagentSynchronizationLoop: Closing the loop with [%d] "
                    "corrupted backgrounds.\n", nxagentCorruptedBackgrounds);
      }

      nxagentCorruptedBackgrounds = 0;
    }

    #endif
  }

  /*
   * If there is bandwidth remaining, synchronize
   * the pixmaps. Synchronizing a pixmap doesn't
   * produce any visible results. Better is to
   * synchronize them on demand, before using the
   * pixmap in a copy or in a composite operation.
   */

  if (nxagentSynchronization.abort == 0 &&
          NXAGENT_SHOULD_SYNCHRONIZE_CORRUPTED_PIXMAPS(mask))
  {
    #ifdef TEST
    fprintf(stderr, "nxagentSynchronizationLoop: Going to loop through corrupted pixmap resources.\n");
    #endif

    FindClientResourcesByType(clients[serverClient -> index], RT_NX_CORR_PIXMAP,
                                  nxagentSynchronizeDrawablePredicate, &breakMask);


    if (nxagentSynchronization.abort == 0 &&
            nxagentSynchronization.pixmapBitmaps == 0 &&
                doRoundRobin == 0)
    {
      #ifdef TEST

      if (nxagentCorruptedPixmaps > 0)
      {
        fprintf(stderr, "nxagentSynchronizationLoop: Closing the loop with [%d] "
                    "corrupted pixmaps.\n", nxagentCorruptedPixmaps);
      }

      #endif

      nxagentCorruptedPixmaps = 0;
    }

  }

  /*
   * If the last synchronized drawable has been
   * removed, we have to reset the variable sto-
   * ring its pointer.
   */

  if (nxagentSynchronization.pDrawable != NULL &&
          nxagentFindClientResource(serverClient -> index, RT_NX_CORR_WINDOW,
              nxagentSynchronization.pDrawable) == 0 &&
                  nxagentFindClientResource(serverClient -> index, RT_NX_CORR_BACKGROUND,
                      nxagentSynchronization.pDrawable) == 0 &&
                          nxagentFindClientResource(serverClient -> index, RT_NX_CORR_PIXMAP,
                              nxagentSynchronization.pDrawable) == 0)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentSynchronizationLoop: Synchronization drawable [%p] removed from resources.\n",
                (void *) nxagentSynchronization.pDrawable);
    #endif

    nxagentSynchronization.pDrawable = NULL;
  }

  #ifdef TEST
  fprintf(stderr, "nxagentSynchronizationLoop: Closing loops with congestion [%d] "
              "blocking [%d].\n", nxagentCongestion, nxagentBlocking);

  fprintf(stderr, "nxagentSynchronizationLoop: There are now [%d] windows [%d] pixmaps "
              "and [%d] backgrounds to synchronize.\n", nxagentCorruptedWindows,
                  nxagentCorruptedPixmaps, nxagentCorruptedBackgrounds);
  #endif
}

RegionPtr nxagentCreateRegion(DrawablePtr pDrawable, GCPtr pGC, int x, int y,
                                  int width, int height)
{
  RegionPtr pRegion;
  BoxRec box;

  box.x1 = x;
  box.y1 = y;
  box.x2 = x + width;
  box.y2 = y + height;

  pRegion = REGION_CREATE(pDrawable -> pScreen, &box, 1);

  /*
   * Clipping the region.
   */

  if (pDrawable -> type == DRAWABLE_PIXMAP)
  {
    BoxRec tmpBox;
    RegionRec tmpRegion;

    /*
     * The region created doesn't need to be clipped
     * if it has the pixmap dimensions.
     */

    if (x != 0 || y != 0 ||
            width != pDrawable -> width ||
                height != pDrawable -> height)
    {
      tmpBox.x1 = 0;
      tmpBox.y1 = 0;
      tmpBox.x2 = pDrawable -> width;
      tmpBox.y2 = pDrawable -> height;

      REGION_INIT(pDrawable -> pScreen, &tmpRegion, &tmpBox, 1);

      REGION_INTERSECT(pDrawable -> pScreen, pRegion, &tmpRegion, pRegion);

      REGION_UNINIT(pDrawable -> pScreen, &tmpRegion);
    }
  }
  else
  {
    /*
     * We use the clipList because the borderClip
     * contains also parts of the window covered
     * by its children.
     */

    REGION_TRANSLATE(pDrawable -> pScreen, pRegion,
                         pDrawable -> x, pDrawable -> y);

    if (nxagentWindowPriv((WindowPtr) pDrawable) -> hasTransparentChildren == 1)
    {
      REGION_INTERSECT(pDrawable -> pScreen, pRegion, pRegion, &((WindowPtr) pDrawable) -> borderClip);
    }
    else
    {
      REGION_INTERSECT(pDrawable -> pScreen, pRegion, pRegion, &((WindowPtr) pDrawable) -> clipList);
    }

    REGION_TRANSLATE(pDrawable -> pScreen, pRegion,
                         -pDrawable -> x, -pDrawable -> y);
  }

  #ifdef TEST
  fprintf(stderr, "nxagentCreateRegion: New region created with coordinates [%d,%d,%d,%d].\n",
              pRegion -> extents.x1, pRegion -> extents.y1,
                  pRegion -> extents.x2, pRegion -> extents.y2);
  #endif

  /*
   * If the pRegion is NIL we don't need
   * to intersect it with the GC's clipmask.
   */

  if (REGION_NIL(pRegion) == 0 &&
          pGC != NULL && pGC -> clientClip != NULL &&
              pGC -> clientClipType == CT_REGION)
  {
    RegionRec clipRegion;

    REGION_INIT(pDrawable -> pScreen, &clipRegion, NullBox, 1);

    REGION_COPY(pDrawable -> pScreen, &clipRegion, (RegionPtr) pGC -> clientClip);

    /*
     * The clip origin is relative to the origin of
     * the destination drawable. The clip mask coor-
     * dinates are relative to the clip origin.
     */

    if (pGC -> clipOrg.x != 0 || pGC -> clipOrg.y != 0)
    {
      REGION_TRANSLATE(pDrawable -> pScreen, &clipRegion, pGC -> clipOrg.x, pGC -> clipOrg.y);
    }

    #ifdef TEST
    fprintf(stderr, "nxagentCreateRegion: Clipping region to the clip mask with coordinates [%d,%d,%d,%d].\n",
                clipRegion.extents.x1, clipRegion.extents.y1,
                    clipRegion.extents.x2, clipRegion.extents.y2);
    #endif

    REGION_INTERSECT(pDrawable -> pScreen, pRegion, pRegion, &clipRegion);

    REGION_UNINIT(pDrawable -> pScreen, &clipRegion);
  }

  return pRegion;
}

void nxagentMarkCorruptedRegion(DrawablePtr pDrawable, RegionPtr pRegion)
{
  int x;
  int y;
  int width;
  int height;

  if (pRegion != NullRegion && REGION_NIL(pRegion) == 1)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentMarkCorruptedRegion: Region [%d,%d,%d,%d] is nil. Skipping operation.\n",
                pRegion -> extents.x1, pRegion -> extents.y1, pRegion -> extents.x2, pRegion -> extents.y2);
    #endif

    return;
  }

  /*
   * If the drawable was synchronized, the counter
   * reporting the number of corrupted drawables
   * must be increased. Moreover the corrupted ti-
   * mestamp must be set.
   */

  if (nxagentDrawableStatus(pDrawable) == Synchronized)
  {
    if (pDrawable -> type == DRAWABLE_WINDOW)
    {
      nxagentAllocateCorruptedResource(pDrawable, RT_NX_CORR_WINDOW);
    }

    nxagentSetCorruptedTimestamp(pDrawable);
  }

  if (pRegion == NullRegion)
  {
    x = 0;
    y = 0;

    width  = pDrawable -> width;
    height = pDrawable -> height;

    #ifdef TEST
    fprintf(stderr, "nxagentMarkCorruptedRegion: Fully invalidating %s [%p] with "
                "coordinates [%d,%d][%d,%d].\n", nxagentDrawableType(pDrawable),
                    (void *) pDrawable, x, y, width, height);
    #endif

    pRegion = nxagentCreateRegion(pDrawable, NULL, x, y, width, height);

    nxagentValidateSplit(pDrawable, pRegion);

    REGION_UNION(pDrawable -> pScreen, nxagentCorruptedRegion(pDrawable),
                     nxagentCorruptedRegion(pDrawable), pRegion);

    nxagentFreeRegion(pDrawable, pRegion);
  }
  else
  {
    #ifdef TEST

    x = pRegion -> extents.x1;
    y = pRegion -> extents.y1;

    width = pRegion -> extents.x2 - pRegion -> extents.x1;
    height = pRegion -> extents.y2 - pRegion -> extents.y1;

    fprintf(stderr, "nxagentMarkCorruptedRegion: Partly invalidating %s [%p] with "
                "coordinates [%d,%d][%d,%d].\n", nxagentDrawableType(pDrawable),
                    (void *) pDrawable, x, y, width, height);

    #endif

    nxagentValidateSplit(pDrawable, pRegion);

    REGION_UNION(pDrawable -> pScreen, nxagentCorruptedRegion(pDrawable),
                     nxagentCorruptedRegion(pDrawable), pRegion);
  }
}

void nxagentUnmarkCorruptedRegion(DrawablePtr pDrawable, RegionPtr pRegion)
{
  int oldStatus;

  if (pRegion != NullRegion && REGION_NIL(pRegion) == 1)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentUnmarkCorruptedRegion: Region [%d,%d,%d,%d] is nil. Skipping operation.\n",
                pRegion -> extents.x1, pRegion -> extents.y1, pRegion -> extents.x2, pRegion -> extents.y2);
    #endif

    return;
  }

  oldStatus = nxagentDrawableStatus(pDrawable);

  if (oldStatus == Synchronized)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentUnmarkCorruptedRegion: Drawable %s [%p] already synchronized.\n",
                nxagentDrawableType(pDrawable), (void *) pDrawable);
    #endif

    return;
  }

  if (pRegion == NullRegion)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentUnmarkCorruptedRegion: Fully validating %s [%p].\n",
                nxagentDrawableType(pDrawable), (void *) pDrawable);
    #endif

    nxagentValidateSplit(pDrawable, NULL);

    REGION_EMPTY(pDrawable -> pScreen, nxagentCorruptedRegion(pDrawable));
  }
  else
  {
    #ifdef TEST

    fprintf(stderr, "nxagentUnmarkCorruptedRegion: Validating %s [%p] with region [%d,%d,%d,%d].\n",
                nxagentDrawableType(pDrawable), (void *) pDrawable,
                    pRegion -> extents.x1, pRegion -> extents.y1, pRegion -> extents.x2, pRegion -> extents.y2);

    #endif

    nxagentValidateSplit(pDrawable, pRegion);

    REGION_SUBTRACT(pDrawable -> pScreen, nxagentCorruptedRegion(pDrawable),
                        nxagentCorruptedRegion(pDrawable), pRegion);
  }

  /*
   * If the drawable becomes synchronized, the
   * counter reporting the number of corrupted
   * drawables must be decreased. Moreover the
   * corrupted timestamp must be reset.
   */

  if (oldStatus == NotSynchronized &&
          nxagentDrawableStatus(pDrawable) == Synchronized)
  {
    if (pDrawable -> type == DRAWABLE_PIXMAP)
    {
      nxagentDestroyCorruptedResource(pDrawable, RT_NX_CORR_BACKGROUND);

      nxagentDestroyCorruptedResource(pDrawable, RT_NX_CORR_PIXMAP);

      nxagentPixmapPriv(nxagentRealPixmap((PixmapPtr) pDrawable)) -> containTrapezoids = 0;
    }
    else
    {
      nxagentDestroyCorruptedResource(pDrawable, RT_NX_CORR_WINDOW);
    }

    nxagentResetCorruptedTimestamp(pDrawable);

    /*
     * If the resource is no longer dirty,
     * the associated bitmap is destroyed.
     */

    if (nxagentDrawableBitmap(pDrawable) != NullPixmap)
    {
       nxagentDestroyDrawableBitmap(pDrawable);
    }
  }
}

void nxagentMoveCorruptedRegion(WindowPtr pWin, unsigned int mask)
{
  /*
   * If a window is resized, its corrupted
   * region is moved according to the bit
   * gravity.
   */

  if (nxagentDrawableStatus((DrawablePtr) pWin) == NotSynchronized)
  {
    if (((mask & CWHeight) && nxagentWindowPriv(pWin) -> height != pWin -> drawable.height) ||
            ((mask & CWWidth) && nxagentWindowPriv(pWin) -> width != pWin -> drawable.width))
    {
      int nx, ny;

      GravityTranslate(0, 0,
                       nxagentWindowPriv(pWin) -> x - pWin -> origin.x + wBorderWidth(pWin),
                       nxagentWindowPriv(pWin) -> y - pWin -> origin.y + wBorderWidth(pWin),
                       pWin -> drawable.width - nxagentWindowPriv(pWin) -> width,
                       pWin -> drawable.height - nxagentWindowPriv(pWin) -> height,
                       pWin -> bitGravity, &nx, &ny);

      #ifdef TEST
      fprintf(stderr, "nxagentMoveCorruptedRegion: Moving the corrupted region to [%d,%d] for window [%p].\n",
                  nx, ny, (void *) pWin);
      #endif

      REGION_TRANSLATE(pWin -> pScreen, nxagentCorruptedRegion((DrawablePtr) pWin),
                           nx, ny);

      /*
       * Having moved the corrupted region, we
       * need to invalidate the pending commits
       * or otherwise the image will fall in
       * the wrong area.
       */

      nxagentValidateSplit((DrawablePtr) pWin, NULL);


      /*
       * The window reconfiguration invalidates
       * the synchronization bitmap.
       */

      nxagentDestroyDrawableBitmap((DrawablePtr) pWin);
    }
  }
}

/*
 * The DDX layer uses an 'Y-X banding' representation of
 * regions: it sorts all rectangles composing a region
 * using first the y-dimension, than the x-dimension; mo-
 * reover it organizes the rectangles in 'bands' sharing
 * the same y-dimension. This representation does not mi-
 * nimize the number of rectangles. For example, the fol-
 * lowing region has 4 rectangles:
 *
 *    +-----------+
 *    |           |   +---+
 *    |     A     |   | B |
 *    |           |   +---+
 *    +-----------+
 *
 * The rectangle 'B' creates a band which splits the rec-
 * tangle A in 3 parts, for a total of 3 bands. The num-
 * ber of rectangles composing the region is 4.
 *
 * This kind of representation is not advisable for the
 * lazy synchronization because, in the example above,
 * the nxagent had to send 4 put images instead of 2.
 *
 * To minimize the problem we use the following function:
 * by traversing the list of rectangles we merge all bo-
 * xes with same x coordinates and coincident y, in order
 * to create an X-Y banding.
 *
 * Be careful: all the coordinates of boxes merged are
 * set to 0, so take care of this when looping through
 * the box list returned by this function.
 */

BoxPtr nxagentGetOptimizedRegionBoxes(RegionPtr pRegion)
{
  BoxPtr pBox;

  BoxRec boxExtents;

  int nBox;
  int i, j;

  #ifdef DEBUG
  int nBoxOptim;
  #endif

  pBox = REGION_RECTS(pRegion);

  nBox = REGION_NUM_RECTS(pRegion);

  #ifdef TEST
  fprintf(stderr, "nxagentGetOptimizedRegionBoxes: Going to optimize region at [%p] with [%d] rects.\n",
              (void *) pRegion, nBox);
  #endif

  if (nBox <= 1)
  {
    return pBox;
  }

  #ifdef DEBUG
  nBoxOptim = nBox;
  #endif

  /*
   * The boxes are now grouped to grown as much
   * as possible, using their overlapping vertex
   * as rule.
   */

  for (i = 0; i < nBox; i++)
  {
    /*
     * If the coordinates are (0,0) the box
     * has been already merged, so we can skip
     * it.
     */

    if (pBox[i].x1 == 0 && pBox[i].y1 == 0 &&
            pBox[i].x2 == 0 && pBox[i].y2 == 0)
    {
      continue;
    }

    #ifdef DEBUG
    fprintf(stderr, "nxagentGetOptimizedRegionBoxes: Referential box [%d] has coordinates [%d,%d,%d,%d].\n",
                i, pBox[i].x1, pBox[i].y1, pBox[i].x2, pBox[i].y2);
    #endif

    #ifdef ADVANCED_BOXES_DEFRAG

    boxExtents.x1 = pBox[i].x1;
    boxExtents.y1 = pBox[i].y1;
    boxExtents.x2 = pBox[i].x2;

    #endif

    boxExtents.y2 = pBox[i].y2;

    for (j = i+1; j < nBox; j++)
    {
      if (pBox[j].x1 == 0 && pBox[j].y1 == 0 &&
              pBox[j].x2 == 0 && pBox[j].y2 == 0)
      {
        continue;
      }

      #ifdef DEBUG
      fprintf(stderr, "nxagentGetOptimizedRegionBoxes: Mergeable box [%d] has coordinates [%d,%d,%d,%d].\n",
                  j, pBox[j].x1, pBox[j].y1, pBox[j].x2, pBox[j].y2);
      #endif

      /*
       * Each consequent box is merged if its
       * higher side overlaps the lower side
       * of current box.
       * In case of ADVANCED_BOXES_DEFRAG the higher
       * side must be included within a range
       * defined by INCLUDE_MARGIN.
       */

      #ifndef ADVANCED_BOXES_DEFRAG

      if (pBox[j].y1 == boxExtents.y2 &&
          pBox[j].x1 == pBox[i].x1 &&
          pBox[j].x2 == pBox[i].x2)

      #else

      if (pBox[j].x1 > boxExtents.x1 - INCLUDE_MARGIN &&
          pBox[j].x1 < boxExtents.x1 + INCLUDE_MARGIN &&
          pBox[j].y1 > boxExtents.y2 - INCLUDE_MARGIN &&
          pBox[j].y1 < boxExtents.y2 + INCLUDE_MARGIN &&
          pBox[j].x2 > boxExtents.x2 - INCLUDE_MARGIN &&
          pBox[j].x2 < boxExtents.x2 + INCLUDE_MARGIN)

      #endif
      {
        #ifdef DEBUG
        fprintf(stderr, "nxagentGetOptimizedRegionBoxes: Going to merge box at [%d] with box at [%d].\n",
                    j, i);
        #endif

        #ifdef ADVANCED_BOXES_DEFRAG

        if (pBox[j].x1 < boxExtents.x1)
        {
          boxExtents.x1 = pBox[j].x1;
        }

        if (pBox[j].x2 > boxExtents.x2)
        {
          boxExtents.x2 = pBox[j].x2;
        }

        if (pBox[j].y1 < boxExtents.y1)
        {
          boxExtents.y1 = pBox[j].y1;
        }

        #endif

        if (pBox[j].y2 > boxExtents.y2)
        {
          boxExtents.y2 = pBox[j].y2;
        }

        /*
         * By appending a box to another, we have
         * to remove it from the box list. We do
         * this by setting its coordinates to (0,0)
         * and by checking their value in the main
         * loop.
         */

        pBox[j].x1 = pBox[j].y1 = pBox[j].x2 = pBox[j].y2 = 0;

        #ifdef DEBUG
        nBoxOptim--;
        #endif
      }
    }

    /*
     * Extend the box height.
     */

    #ifdef ADVANCED_BOXES_DEFRAG

    pBox[i].x1 = boxExtents.x1;
    pBox[i].y1 = boxExtents.y1;
    pBox[i].x2 = boxExtents.x2;

    #endif

    pBox[i].y2 = boxExtents.y2;
  }

  #ifdef ADVANCED_BOXES_DEFRAG

  /*
   * The new list need to be validated to
   * avoid boxes overlapping. This code may
   * be improved to remove also the partial-
   * ly overlapping boxes.
   */

  for (i = 0; i < nBox; i++)
  {
    if (pBox[i].x1 == 0 && pBox[i].y1 == 0 &&
            pBox[i].x2 == 0 && pBox[i].y2 == 0)
    {
      continue;
    }

    #ifdef DEBUG
    fprintf(stderr, "nxagentGetOptimizedRegionBoxes: Referential box [%d] has coordinates [%d,%d,%d,%d].\n",
                i, pBox[i].x1, pBox[i].y1, pBox[i].x2, pBox[i].y2);
    #endif

    boxExtents.x1 = pBox[i].x1;
    boxExtents.y1 = pBox[i].y1;
    boxExtents.x2 = pBox[i].x2;
    boxExtents.y2 = pBox[i].y2;

    for (j = i+1; j < nBox; j++)
    {
      if (pBox[j].x1 == 0 && pBox[j].y1 == 0 &&
              pBox[j].x2 == 0 && pBox[j].y2 == 0)
      {
        continue;
      }

      #ifdef DEBUG
      fprintf(stderr, "nxagentGetOptimizedRegionBoxes: Mergeable box [%d] has coordinates [%d,%d,%d,%d].\n",
                  j, pBox[j].x1, pBox[j].y1, pBox[j].x2, pBox[j].y2);
      #endif

      if ((boxExtents.x1 <= pBox[j].x1 &&
           boxExtents.x2 >= pBox[j].x2 &&
           boxExtents.y1 <= pBox[j].y1 &&
           boxExtents.y2 >= pBox[j].y2))
      {
        /*
         * If a box is completely inside
         * another, we set its coordinates
         * to 0 to consider it as merged.
         */

        #ifdef DEBUG
        fprintf(stderr, "nxagentGetOptimizedRegionBoxes: Going to merge box [%d,%d,%d,%d] "
                    "with its box container [%d,%d,%d,%d].\n", pBox[j].x1, pBox[j].y1,
                        pBox[j].x2, pBox[j].y2, boxExtents.x1, boxExtents.y1, boxExtents.y1,
                            boxExtents.y2);
        #endif

        pBox[j].x1 = pBox[j].y1 = pBox[j].x2 = pBox[j].y2 = 0;

        #ifdef DEBUG
        nBoxOptim--;
        #endif
      }
    }
  }

  #endif

  #ifdef DEBUG
  fprintf(stderr, "nxagentGetOptimizedRegionBoxes: Original boxes number [%d] Optimized boxes number [%d].\n",
              nBox, nBoxOptim);
  #endif

  return pBox;
}

unsigned long nxagentGetColor(DrawablePtr pDrawable, int xPixel, int yPixel)
{
  XImage *ximage;
  Visual *pVisual;
  char *data;

  int depth, format, length;
  int leftPad = 0;
  unsigned long pixel;

  depth = pDrawable -> depth;
  format = (depth == 1) ? XYPixmap : ZPixmap;
  length = nxagentImageLength(1, 1, format, leftPad, depth);

  if ((data = xalloc(length)) == NULL)
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentGetColor: WARNING! Failed to allocate memory for the operation.\n");
    #endif

    return -1;
  }

  pVisual = nxagentImageVisual(pDrawable, depth);

  if (pVisual == NULL)
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentGetColor: WARNING! Visual not found. Using default visual.\n");
    #endif

    pVisual = nxagentVisuals[nxagentDefaultVisualIndex].visual;
  }

  fbGetImage(pDrawable, xPixel, yPixel, 1, 1, format, AllPlanes, data);

  ximage = XCreateImage(nxagentDisplay, pVisual, depth, format, leftPad, (char *) data,
                            1, 1, BitmapPad(nxagentDisplay),
                                nxagentImagePad(1, format, leftPad, 1));

  if (ximage == NULL)
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentGetColor: WARNING! Failed to create the XImage.\n");
    #endif

    xfree(data);

    return -1;
  }

  pixel = XGetPixel(ximage, 0, 0);

  XDestroyImage(ximage);

  return pixel;
}

/*
 * This function could be used to determine
 * the ClearArea color of corrupted regions
 * on screen.
 */

unsigned long nxagentGetRegionColor(DrawablePtr pDrawable, RegionPtr pRegion)
{
  int xPicker, yPicker;

  if (REGION_NIL(pRegion) == 1)
  {
    return nxagentGetDrawableColor(pDrawable);
  }

  /*
   * The pixel used as reference is the first
   * outer pixel at the bottom right corner
   * of corrupted region extents.
   */

  xPicker = pRegion -> extents.x2 + 1;

  if (xPicker > pDrawable -> width)
  {
    xPicker = pDrawable -> width;
  }

  yPicker = pRegion -> extents.y2 + 1;

  if (yPicker > pDrawable -> height)
  {
    yPicker = pDrawable -> height;
  }

  return nxagentGetColor(pDrawable, xPicker, yPicker);
}

unsigned long nxagentGetDrawableColor(DrawablePtr pDrawable)
{
  int xPicker, yPicker;

  /*
   * The pixel used to determine the co-
   * lor of a drawable is at coordinates
   * (x + width - 4, y + 4).
   */

  xPicker = pDrawable -> width - 4;

  yPicker = 4;

  return nxagentGetColor(pDrawable, xPicker, yPicker);
}

void nxagentClearRegion(DrawablePtr pDrawable, RegionPtr pRegion)
{
  WindowPtr pWin;
  BoxPtr pBox;

  unsigned long color;
  unsigned long backupPixel = 0;
  int nBox, i;
  int restore;

  #ifdef DEBUG
  static int nBoxCleared;
  #endif

  if (pDrawable -> type != DRAWABLE_WINDOW)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentClearRegion: Cannot clear a pixmap. Exiting.\n");
    #endif

    return;
  }

  if (pRegion == NullRegion || REGION_NIL(pRegion) == 1)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentClearRegion: The region is empty. Exiting.\n");
    #endif

    return;
  }

  pWin = (WindowPtr) pDrawable;

  restore = 0;

  /*
   * If the window has already a background, we
   * can hope it will be nice.
   */

  if (pWin -> backgroundState != None)
  {
    #ifdef DEBUG
    fprintf(stderr, "nxagentClearRegion: Window at [%p] has background state [%u].\n",
                (void *) pWin, pWin -> backgroundState);
    #endif
  }
  else
  {
    /*
     * Save the original state.
     */

    backupPixel = pWin -> background.pixel;

    color = nxagentGetDrawableColor((DrawablePtr) pWin);

    if (color == -1)
    {
      color = 0xffffff;
    }

    pWin -> backgroundState = BackgroundPixel;
    pWin -> background.pixel = color;

    nxagentChangeWindowAttributes(pWin, CWBackPixel);

    #ifdef DEBUG
    fprintf(stderr, "nxagentClearRegion: Window at [%p] now has pixel background [%ld].\n",
                (void *) pWin, color);
    #endif

    restore = 1;
  }

  pBox = nxagentGetOptimizedRegionBoxes(pRegion);

  nBox = REGION_NUM_RECTS(pRegion);

  for (i = 0; i < nBox; i++)
  {
    if (pBox[i].x1 == 0 && pBox[i].y1 == 0 &&
            pBox[i].x2 == 0 && pBox[i].y2 == 0)
    {
      continue;
    }

    XClearArea(nxagentDisplay, nxagentWindow(pWin), pBox[i].x1, pBox[i].y1,
                   pBox[i].x2 - pBox[i].x1, pBox[i].y2 - pBox[i].y1, False);

    #ifdef DEBUG
    nBoxCleared++;
    #endif
  }

  /*
   * Restore the old state.
   */

  if (restore == 1)
  {
    pWin -> backgroundState = None;
    pWin -> background.pixel = backupPixel;
  }

  #ifdef DEBUG
  fprintf(stderr, "nxagentClearRegion: Number of cleared boxes is [%d].\n", nBoxCleared);
  #endif
}

void nxagentFillRemoteRegion(DrawablePtr pDrawable, RegionPtr pRegion)
{
  GCPtr      pGC;
  BoxPtr     pBox;
  XRectangle *pRects;

  int        nrects;
  int        i;

  if (REGION_NIL(pRegion) == 1)
  {
    return;
  }

  pGC = nxagentGetGraphicContext(pDrawable);

  nrects = REGION_NUM_RECTS(pRegion);

  #ifdef TEST
  fprintf(stderr, "nxagentFillRemoteRegion: Going to fill remote region [%d,%d,%d,%d] rects [%d] with color [%lu].\n",
              pRegion -> extents.x1, pRegion -> extents.y1, pRegion -> extents.x2, pRegion -> extents.y2,
                  nrects, pGC -> fgPixel);
  #endif

  if (nrects == 1)
  {
    XFillRectangle(nxagentDisplay, nxagentDrawable(pDrawable), nxagentGC(pGC),
                      pRegion -> extents.x1, pRegion -> extents.y1,
                          pRegion -> extents.x2 - pRegion -> extents.x1,
                              pRegion -> extents.y2 - pRegion -> extents.y1);
  }
  else
  {
    pBox = REGION_RECTS(pRegion);

    pRects = xalloc(nrects * sizeof(XRectangle));

    for (i = 0; i < nrects; i++)
    {
      pRects[i].x      = pBox[i].x1;
      pRects[i].y      = pBox[i].y1;
      pRects[i].width  = pBox[i].x2 - pBox[i].x1;
      pRects[i].height = pBox[i].y2 - pBox[i].y1;
    }

    XFillRectangles(nxagentDisplay, nxagentDrawable(pDrawable), nxagentGC(pGC),
                        pRects, nrects);

    xfree(pRects);
  }
}

int nxagentDestroyCorruptedWindowResource(pointer p, XID id)
{
  #ifdef TEST
  fprintf(stderr, "nxagentDestroyCorruptedWindowResource: Removing corrupted window [%p] from resources.\n",
              (void *) p);
  #endif

  nxagentWindowPriv((WindowPtr) p) -> corruptedId = None;

  return 1;
}

int nxagentDestroyCorruptedPixmapResource(pointer p, XID id)
{
  #ifdef TEST
  fprintf(stderr, "nxagentDestroyCorruptedPixmapResource: Removing corrupted pixmap [%p] from resources.\n",
              (void *) p);
  #endif

  nxagentPixmapPriv((PixmapPtr) p) -> corruptedId = None;

  return 1;
}

int nxagentDestroyCorruptedBackgroundResource(pointer p, XID id)
{
  #ifdef TEST
  fprintf(stderr, "nxagentDestroyCorruptedBackgroundResource: Removing corrupted pixmap background [%p] from resources.\n",
              (void *) p);
  #endif

  nxagentPixmapPriv((PixmapPtr) p) -> corruptedBackgroundId = None;

  return 1;
}

void nxagentPointsToDirtyRegion(DrawablePtr pDrawable, int mode,
                                    int nPoints, xPoint *pPoints)
{
  RegionPtr pRegion;
  RegionRec tmpRegion;
  BoxRec box, extents;

  xPoint *xp;
  int np;

  np = nPoints;
  xp = pPoints;

  pRegion = REGION_CREATE(pDrawable -> pScreen, NullBox, 1);

  while (np--)
  {
    if (CoordModePrevious)
    {
      box.x1 = box.x2 = (xp-1) -> x + xp -> x;
      box.y1 = box.y2 = (xp-1) -> y + xp -> y;
    }
    else
    {
      box.x1 = box.x2 = xp -> x;
      box.y1 = box.y2 = xp -> y;
    }

    #ifdef TEST
    fprintf(stderr, "nxagentPointsToDirtyRegion: Adding the point (%d,%d) to the dirty region.\n",
                box.x1, box.y1);
    #endif

    /*
     * By using REGION_APPEND() and REGION_VALIDATE()
     * this loop could become less expensive.
     */

    REGION_INIT(pDrawable -> pScreen, &tmpRegion, &box, 1);

    REGION_UNION(pDrawable -> pScreen, pRegion, pRegion, &tmpRegion);

    REGION_UNINIT(pDrawable -> pScreen, &tmpRegion);

    xp++;
  }

  extents = *REGION_EXTENTS(pDrawable -> pScreen, pRegion);

  REGION_RESET(pDrawable -> pScreen, pRegion, &extents);

  #ifdef TEST
  fprintf(stderr, "nxagentPointsToDirtyRegion: The resulting dirty region has [%ld] rects and"
              " extents (%d,%d,%d,%d).\n", REGION_NUM_RECTS(pRegion), extents.x1,
                  extents.y1, extents.x2, extents.y2);
  #endif

  nxagentMarkCorruptedRegion(pDrawable, pRegion);

  REGION_DESTROY(pDrawable -> pScreen, pRegion);
}

#ifdef DUMP

#define USE_MULTIPLE_COLORS

void nxagentCorruptedRegionOnWindow(void *p0, XID x, void *p2)
{
  WindowPtr pWin = (WindowPtr) p0;
  RegionPtr clipRegion;
  RegionRec visRegion;
  BoxPtr pBox;

  XlibGC gc;
  XGCValues value;

  static unsigned long color = 0xff000000;
  int nrectangles;
  int i;

  /*
   * There are no regions to draw.
   */

  if (nxagentDrawableStatus((DrawablePtr) pWin) == Synchronized)
  {
    return;
  }

  /*
   * The window is not visible.
   */

  if (nxagentWindowIsVisible(pWin) == 0)
  {
    return;
  }

  #ifdef TEST
  fprintf(stderr, "nxagentCorruptedRegionOnWindow: Going to draw on window at [%p].\n",
              (void *) pWin);
  #endif

  clipRegion = nxagentCreateRegion((DrawablePtr) pWin, NULL, 0, 0,
                                      pWin -> drawable.width, pWin -> drawable.height);

  REGION_INIT(pWin -> drawable.pScreen, &visRegion, NullBox, 1);

  REGION_INTERSECT(pWin -> drawable.pScreen, &visRegion, clipRegion, nxagentCorruptedRegion((DrawablePtr) pWin));

  nxagentFreeRegion(pWin -> drawable.pScreen, clipRegion);

  if (REGION_NIL(&visRegion) == 1)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentCorruptedRegionOnWindow: The corrupted region of window at [%p] is hidden.\n",
                (void *) pWin);
    #endif

    REGION_UNINIT(pWin -> drawable.pScreen, &visRegion);

    return;
  }

  nxagentClearRegion((DrawablePtr) pWin, &visRegion);

  #ifdef USE_MULTIPLE_COLORS

  color += nxagentWindow(pWin) * 5;

  if (color == 0 || color == 0xffffffff)
  {
    color = 0xff000000;
  }

  #endif

  value.foreground = color;
  value.subwindow_mode = IncludeInferiors;

  gc = XCreateGC(nxagentDisplay, nxagentWindow(pWin), GCForeground | GCSubwindowMode, &value);

  nrectangles = REGION_NUM_RECTS(&visRegion);

  #ifdef TEST
  fprintf(stderr, "nxagentCorruptedRegionOnWindow: Going to draw the region with extents [%d,%d,%d,%d] and [%d] rects.\n",
              visRegion.extents.x1, visRegion.extents.y1, visRegion.extents.x2, visRegion.extents.y2,
                  nrectangles);
  #endif

  pBox = nxagentGetOptimizedRegionBoxes(&visRegion);

  for (i = 0; i < nrectangles; i++)
  {
    if (pBox[i].x1 == 0 && pBox[i].y1 == 0 &&
            pBox[i].x2 == 0 && pBox[i].y2 == 0)
    {
      continue;
    }

    XDrawRectangle(nxagentDisplay, nxagentWindow(pWin), gc,
                       pBox[i].x1, pBox[i].y1, pBox[i].x2 - pBox[i].x1 - 1,
                           pBox[i].y2 - pBox[i].y1 - 1);
  }

  XFreeGC(nxagentDisplay, gc);

  REGION_UNINIT(pWin -> drawable.pScreen, &visRegion);
}

void nxagentRegionsOnScreen()
{
  FindClientResourcesByType(clients[serverClient -> index], RT_NX_CORR_WINDOW,
                                nxagentCorruptedRegionOnWindow, NULL);
}

#endif

/*
 * If the synchronization loop breaks and the
 * drawable synchronization cannot be completed,
 * the remaining data is stored in a bitmap.
 * The synchronization loop is then restarted
 * using the bitmap as source instead of the
 * drawable.
 */

void nxagentCreateDrawableBitmap(DrawablePtr pDrawable)
{
  PixmapPtr pBitmap;
  GCPtr pGC = NULL;
  RegionPtr pClipRegion = NullRegion;

  int x, y;
  int w, h;
  int saveTrap;

  #ifdef TEST
  fprintf(stderr, "nxagentCreateDrawableBitmap: Creating synchronization bitmap for [%s] at [%p].\n",
              nxagentDrawableType(pDrawable), (void *) pDrawable);
  #endif

  /*
   * The bitmap is created only in the
   * nxagent.
   */

  saveTrap = nxagentGCTrap;

  nxagentGCTrap = 1;

  if (nxagentDrawableStatus(pDrawable) == Synchronized)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentCreateDrawableBitmap: The drawable is already synchronized. Skipping bitmap creation.\n");
    #endif

    goto nxagentCreateDrawableBitmapEnd;
  }

  /*
   * Should create a function to append
   * a bitmap to another, instead of de-
   * stroy the old one.
   */

  if (nxagentDrawableBitmap(pDrawable) != NullPixmap)
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentCreateDrawableBitmap: WARNING! Going to replace the bitmap at [%p] with corrupted [%d,%d,%d,%d].\n",
                  (void *) nxagentDrawableBitmap(pDrawable),
                      nxagentCorruptedRegion((DrawablePtr) nxagentDrawableBitmap(pDrawable)) -> extents.x1,
                          nxagentCorruptedRegion((DrawablePtr) nxagentDrawableBitmap(pDrawable)) -> extents.y1,
                              nxagentCorruptedRegion((DrawablePtr) nxagentDrawableBitmap(pDrawable)) -> extents.x2,
                                  nxagentCorruptedRegion((DrawablePtr) nxagentDrawableBitmap(pDrawable)) -> extents.y2);
    #endif

    nxagentDestroyDrawableBitmap(pDrawable);
  }

  /*
   * Clipping to the visible area.
   */

  pClipRegion = nxagentCreateRegion(pDrawable, NULL, 0, 0, pDrawable -> width, pDrawable -> height);

  REGION_INTERSECT(pDrawable -> pScreen, pClipRegion, pClipRegion, nxagentCorruptedRegion(pDrawable));

  if (REGION_NIL(pClipRegion) == 1)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentCreateDrawableBitmap: The corrupted region is not visible. Skipping bitmap creation.\n");
    #endif

    goto nxagentCreateDrawableBitmapEnd;
  }

  /*
   * FIXME: A better way it would be create the bitmap
   * with the same extents of the clipRegion. This
   * requires to save the offset with respect to the
   * drawable origin like in the backing store. This
   * becomes particularly important when the drawable
   * is a huge window, because the pixmap creation
   * would fail.
   */

  pBitmap = nxagentCreatePixmap(pDrawable -> pScreen, pDrawable -> width, pDrawable -> height, pDrawable -> depth);

  if (pBitmap == NULL)
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentCreateDrawableBitmap: Cannot create pixmap for the bitmap data.\n");
    #endif

    goto nxagentCreateDrawableBitmapEnd;
  }

  pGC = GetScratchGC(pBitmap -> drawable.depth, pBitmap -> drawable.pScreen);

  ValidateGC((DrawablePtr) pBitmap, pGC);

  x = pClipRegion -> extents.x1;
  y = pClipRegion -> extents.y1;
  w = pClipRegion -> extents.x2 - pClipRegion -> extents.x1;
  h = pClipRegion -> extents.y2 - pClipRegion -> extents.y1;

  nxagentCopyArea(pDrawable, (DrawablePtr) pBitmap, pGC, x, y, w, h, x, y);

  REGION_UNION(pDrawable -> pScreen, nxagentCorruptedRegion((DrawablePtr) pBitmap),
                   nxagentCorruptedRegion((DrawablePtr) pBitmap), pClipRegion);

  if (pDrawable -> type == DRAWABLE_PIXMAP)
  {
    nxagentPixmapPriv(nxagentRealPixmap((PixmapPtr) pDrawable)) -> synchronizationBitmap = pBitmap;

    if (nxagentIsCorruptedBackground((PixmapPtr) pDrawable) == 1)
    {
      nxagentSynchronization.backgroundBitmaps++;
    }
    else
    {
      nxagentSynchronization.pixmapBitmaps++;
    }
  }
  else
  {
    nxagentWindowPriv((WindowPtr) pDrawable) -> synchronizationBitmap = pBitmap;

    nxagentSynchronization.windowBitmaps++;
  }

  #ifdef TEST
  fprintf(stderr, "nxagentCreateDrawableBitmap: Drawable [%p] has bitmap at [%p] with corrupted [%d,%d,%d,%d].\n",
                (void *) pDrawable, (void *) nxagentDrawableBitmap(pDrawable),
                    nxagentCorruptedRegion((DrawablePtr) nxagentDrawableBitmap(pDrawable)) -> extents.x1,
                        nxagentCorruptedRegion((DrawablePtr) nxagentDrawableBitmap(pDrawable)) -> extents.y1,
                            nxagentCorruptedRegion((DrawablePtr) nxagentDrawableBitmap(pDrawable)) -> extents.x2,
                                nxagentCorruptedRegion((DrawablePtr) nxagentDrawableBitmap(pDrawable)) -> extents.y2);
  #endif

nxagentCreateDrawableBitmapEnd:

  nxagentGCTrap = saveTrap;

  if (pClipRegion != NullRegion)
  {
    nxagentFreeRegion(pDrawable, pClipRegion);
  }

  if (pGC != NULL)
  {
    FreeScratchGC(pGC);
  }
}

void nxagentDestroyDrawableBitmap(DrawablePtr pDrawable)
{
  if (nxagentDrawableBitmap(pDrawable) != NullPixmap)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentDestroyDrawableBitmap: Destroying bitmap for drawable at [%p].\n",
                (void *) pDrawable);
    #endif

    nxagentDestroyPixmap(nxagentDrawableBitmap(pDrawable));

    if (pDrawable -> type == DRAWABLE_PIXMAP)
    {
      nxagentPixmapPriv(nxagentRealPixmap((PixmapPtr) pDrawable)) -> synchronizationBitmap = NullPixmap;

      if (nxagentIsCorruptedBackground((PixmapPtr) pDrawable) == 1)
      {
        nxagentSynchronization.backgroundBitmaps--;
      }
      else
      {
        nxagentSynchronization.pixmapBitmaps--;
      }
    }
    else
    {
      nxagentWindowPriv((WindowPtr) pDrawable) -> synchronizationBitmap = NullPixmap;

      nxagentSynchronization.windowBitmaps--;
    }
  }
}

void nxagentIncreasePixmapUsageCounter(PixmapPtr pPixmap)
{
  if (nxagentDrawableStatus((DrawablePtr) pPixmap) == Synchronized)
  {
    return;
  }

  #ifdef TEST
  fprintf(stderr, "nxagentIncreasePixmapUsageCounter: Pixmap usage counter was [%d].\n",
              nxagentPixmapUsageCounter(pPixmap));
  #endif

  nxagentPixmapUsageCounter(pPixmap) += 1;

  nxagentAllocateCorruptedResource((DrawablePtr) pPixmap, RT_NX_CORR_PIXMAP);
}

void nxagentAllocateCorruptedResource(DrawablePtr pDrawable, RESTYPE type)
{
  PixmapPtr pRealPixmap;

  if (nxagentSessionState == SESSION_DOWN)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentAllocateCorruptedResource: WARNING! Not allocated corrupted resource "
                "[%s][%p] with the display down.\n", nxagentDrawableType(pDrawable),
                    (void *) pDrawable);
    #endif

    return;
  }

  if (type == RT_NX_CORR_WINDOW)
  {
    if (nxagentWindowPriv((WindowPtr) pDrawable) -> corruptedId == 0)
    {
      #ifdef TEST
      fprintf(stderr, "nxagentAllocateCorruptedResource: New resource at [%p]. Corrupted "
                  "windows counter was [%d].\n", (void *) pDrawable, nxagentCorruptedWindows);
      #endif

      nxagentCorruptedWindows++;

      nxagentWindowPriv((WindowPtr) pDrawable) -> corruptedId = FakeClientID(serverClient -> index);

      AddResource(nxagentWindowPriv((WindowPtr) pDrawable) -> corruptedId, RT_NX_CORR_WINDOW,
                      (pointer) pDrawable);
    }
  }
  else if (type == RT_NX_CORR_BACKGROUND)
  {
    pRealPixmap = nxagentRealPixmap((PixmapPtr) pDrawable);

    if (nxagentPixmapPriv(pRealPixmap) -> corruptedBackgroundId == 0)
    {
      /*
       * When a pixmap is added to the background
       * corrupted resources, it must be removed
       * from the pixmap corrupted resources.
       */

      nxagentDestroyCorruptedResource(pDrawable, RT_NX_CORR_PIXMAP);

      #ifdef TEST
      fprintf(stderr, "nxagentAllocateCorruptedResource: New resource at [%p]. Corrupted "
                  "backgrounds counter was [%d].\n", (void *) pDrawable, nxagentCorruptedBackgrounds);
      #endif

      nxagentCorruptedBackgrounds++;

      nxagentPixmapPriv(pRealPixmap) -> corruptedBackgroundId = FakeClientID(serverClient -> index);

      AddResource(nxagentPixmapPriv(pRealPixmap) -> corruptedBackgroundId, RT_NX_CORR_BACKGROUND,
                      (pointer) pRealPixmap);
    }
  }
  else if (type == RT_NX_CORR_PIXMAP)
  {
    /*
     * The shared memory pixmaps are always dirty
     * and shouldn't be synchronized.
     */

    if (nxagentPixmapUsageCounter((PixmapPtr) pDrawable) >= MINIMUM_PIXMAP_USAGE_COUNTER &&
            nxagentIsShmPixmap((PixmapPtr) pDrawable) == 0)
    {
      pRealPixmap = nxagentRealPixmap((PixmapPtr) pDrawable);

      if (nxagentPixmapPriv(pRealPixmap) -> corruptedId == 0)
      {
        #ifdef TEST
        fprintf(stderr, "nxagentAllocateCorruptedResource: New resource at [%p]. Corrupted "
                    "pixmaps counter was [%d].\n", (void *) pDrawable, nxagentCorruptedPixmaps);
        #endif

        nxagentCorruptedPixmaps++;

        nxagentPixmapPriv(pRealPixmap) -> corruptedId = FakeClientID(serverClient -> index);

        AddResource(nxagentPixmapPriv(pRealPixmap) -> corruptedId, RT_NX_CORR_PIXMAP,
                        (pointer) pRealPixmap);
      }
    }
  }
}

void nxagentDestroyCorruptedResource(DrawablePtr pDrawable, RESTYPE type)
{
  PixmapPtr pRealPixmap;

  if (type == RT_NX_CORR_WINDOW)
  {
    if (nxagentWindowPriv((WindowPtr) pDrawable) -> corruptedId != 0)
    {
      #ifdef TEST
      fprintf(stderr, "nxagentDestroyCorruptedResource: Removing resource at [%p]. Corrupted "
                  "windows counter was [%d].\n", (void *) pDrawable, nxagentCorruptedWindows);
      #endif

      if (nxagentCorruptedWindows > 0)
      {
        nxagentCorruptedWindows--;
      }

      FreeResource(nxagentWindowPriv((WindowPtr) pDrawable) -> corruptedId, RT_NONE);

      if (nxagentSynchronization.pDrawable == pDrawable)
      {
        nxagentSynchronization.pDrawable = NULL;
      }
    }
  }
  else if (type == RT_NX_CORR_BACKGROUND)
  {
    pRealPixmap = nxagentRealPixmap((PixmapPtr) pDrawable);

    if (nxagentPixmapPriv(pRealPixmap) -> corruptedBackgroundId != 0)
    {
      #ifdef TEST
      fprintf(stderr, "nxagentDestroyCorruptedResource:  Removing resource at [%p]. Corrupted "
                  "backgrounds counter was [%d].\n", (void *) pDrawable, nxagentCorruptedBackgrounds);
      #endif

      if (nxagentCorruptedBackgrounds > 0)
      {
        nxagentCorruptedBackgrounds--;
      }

      FreeResource(nxagentPixmapPriv(pRealPixmap) -> corruptedBackgroundId, RT_NONE);

      if (nxagentSynchronization.pDrawable == pDrawable)
      {
        nxagentSynchronization.pDrawable = NULL;
      }
    }
  }
  else if (type == RT_NX_CORR_PIXMAP)
  {
    pRealPixmap = nxagentRealPixmap((PixmapPtr) pDrawable);

    if (nxagentPixmapPriv(pRealPixmap) -> corruptedId != 0)
    {
      #ifdef TEST
      fprintf(stderr, "nxagentDestroyCorruptedResource:  Removing resource at [%p]. Corrupted "
                  "pixmaps counter was [%d].\n", (void *) pDrawable, nxagentCorruptedPixmaps);
      #endif

      if (nxagentCorruptedPixmaps > 0)
      {
        nxagentCorruptedPixmaps--;
      }

      FreeResource(nxagentPixmapPriv(pRealPixmap) -> corruptedId, RT_NONE);

      if (nxagentSynchronization.pDrawable == pDrawable)
      {
        nxagentSynchronization.pDrawable = NULL;
      }
    }
  }
}

void nxagentCleanCorruptedDrawable(DrawablePtr pDrawable)
{
  if (nxagentDrawableStatus(pDrawable) == Synchronized)
  {
    return;
  }

  #ifdef TEST
  fprintf(stderr, "nxagentCleanCorruptedDrawable: Clearing out the corrupted drawable [%s][%p].\n",
              nxagentDrawableType(pDrawable), (void *) pDrawable);
  #endif

  nxagentUnmarkCorruptedRegion(pDrawable, NullRegion);

  if (nxagentDrawableBitmap(pDrawable) != NullPixmap)
  {
    nxagentDestroyDrawableBitmap(pDrawable);
  }
}

void nxagentUnmarkExposedRegion(WindowPtr pWin, RegionPtr pRegion, RegionPtr pOther)
{
  RegionRec clipRegion;

  if (pRegion != NullRegion && REGION_NIL(pRegion) == 0 &&
          nxagentDrawableStatus((DrawablePtr) pWin) == NotSynchronized)
  {
    REGION_INIT(pWin -> drawable.pScreen, &clipRegion, NullBox, 1);

    REGION_COPY(pWin -> drawable.pScreen, &clipRegion, pRegion);
    
    if (pOther != NullRegion && REGION_NIL(pOther) == 0)
    {
      REGION_UNION(pWin -> drawable.pScreen, &clipRegion, &clipRegion, pOther);
    }

    REGION_TRANSLATE(pWin -> drawable.pScreen, &clipRegion, -pWin -> drawable.x, -pWin -> drawable.y);

    #ifdef TEST
    fprintf(stderr, "nxagentUnmarkExposedRegion: Validating expose region [%d,%d,%d,%d] "
                "on window [%p].\n", clipRegion.extents.x1, clipRegion.extents.y1,
                    clipRegion.extents.x2, clipRegion.extents.y2, (void *) pWin);
    #endif

    nxagentUnmarkCorruptedRegion((DrawablePtr) pWin, &clipRegion);

    REGION_UNINIT(pWin -> drawable.pScreen, &clipRegion);
  }
}

int nxagentSynchronizationPredicate()
{
  if (nxagentCorruptedWindows == 0 &&
          nxagentCorruptedBackgrounds == 0 &&
              nxagentCorruptedPixmaps == 0)
  {
    return NotNeeded;
  }

  if (nxagentBlocking == 0 &&
          nxagentCongestion <= 4 &&
              nxagentReady == 0 &&
                  nxagentUserInput(NULL) == 0)
  {
    return Needed;
  }

  /*
   * If there are resources to synchronize
   * but the conditions to start the loop
   * are not satisfied, a little delay is
   * requested to check for a new loop as
   * soon as possible.
   */

  return Delayed;
}

void nxagentSendBackgroundExpose(WindowPtr pWin, PixmapPtr pBackground, RegionPtr pExpose)
{
  RegionRec expose;
  miBSWindowPtr pBackingStore;

  REGION_INIT(pWin -> pScreen, &expose, NullBox, 1);

  #ifdef DEBUG
  fprintf(stderr, "nxagentSendBackgroundExpose: Original expose region is [%d,%d,%d,%d].\n",
              pExpose -> extents.x1, pExpose -> extents.y1,
                  pExpose -> extents.x2, pExpose -> extents.y2);

  fprintf(stderr, "nxagentSendBackgroundExpose: Window clipList is [%d,%d,%d,%d].\n",
              pWin -> clipList.extents.x1, pWin -> clipList.extents.y1,
                  pWin -> clipList.extents.x2, pWin -> clipList.extents.y2);
  #endif

  if (nxagentDrawableStatus((DrawablePtr) pBackground) == Synchronized &&
          (pBackground -> drawable.width < pWin -> drawable.width ||
              pBackground -> drawable.height < pWin -> drawable.height))
  {
    #ifdef TEST
    fprintf(stderr, "nxagentSendBackgroundExpose: Pixmap background [%dx%d] is "
                "smaller than window [%dx%d]. Going to expose the winSize.\n",
                    pBackground -> drawable.width, pBackground -> drawable.height, 
                        pWin -> drawable.width, pWin -> drawable.height);
    #endif

    REGION_COPY(pWin -> pScreen, &expose, &pWin -> winSize);
  }
  else
  {
    REGION_COPY(pWin -> pScreen, &expose, pExpose);

    REGION_TRANSLATE(pWin -> pScreen, &expose, pWin -> drawable.x, pWin -> drawable.y);
  }

  REGION_SUBTRACT(pWin -> pScreen, &expose, &expose, nxagentCorruptedRegion((DrawablePtr) pWin));

  if (REGION_NIL(&pWin -> clipList) != 0)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentSendBackgroundExpose: Exposures deferred because the window "
                "is hidden.\n");
    #endif

    REGION_UNION(pWin -> pScreen, nxagentDeferredBackgroundExposures,
                     nxagentDeferredBackgroundExposures, &expose);

    nxagentWindowPriv(pWin) -> deferredBackgroundExpose = 1;

    goto nxagentSendBackgroundExposeEnd;
  }

  #ifdef TEST
  fprintf(stderr, "nxagentSendBackgroundExpose: Sending expose [%d,%d,%d,%d].\n",
              expose.extents.x1, expose.extents.y1, expose.extents.x2, expose.extents.y2);
  #endif

  /*
   * This prevents hidden region to be exposed.
   */

  pBackingStore = (miBSWindowPtr)pWin->backStorage;

  if ((pBackingStore != NULL) && (REGION_NIL(&pBackingStore->SavedRegion) == 0))
  {
    REGION_TRANSLATE(pWin -> pScreen, &expose, -pWin -> drawable.x, -pWin -> drawable.y);

    REGION_SUBTRACT(pWin -> pScreen, &expose, &expose, &pBackingStore -> SavedRegion);

    REGION_TRANSLATE(pWin -> pScreen, &expose, pWin -> drawable.x, pWin -> drawable.y);
  }

  REGION_INTERSECT(pWin -> pScreen, &expose, &expose, &pWin -> clipList);

  /*
   * Reduce the overall region to expose.
   */
  
  REGION_TRANSLATE(pWin -> pScreen, &expose, -pWin -> drawable.x, -pWin -> drawable.y);
  
  REGION_SUBTRACT(pWin -> pScreen, pExpose, pExpose, &expose);
  
  REGION_TRANSLATE(pWin -> pScreen, &expose, pWin -> drawable.x, pWin -> drawable.y);

  miWindowExposures(pWin, &expose, &expose);

nxagentSendBackgroundExposeEnd:

  REGION_UNINIT(pWin -> pScreen, &expose);
}

void nxagentExposeBackgroundPredicate(void *p0, XID x1, void *p2)
{
  WindowPtr pWin = (WindowPtr) p0;
  WindowPtr pParent;

  struct nxagentExposeBackground *pPair = p2;

  if (REGION_NIL(pPair -> pExpose) != 0)
  {
    return;
  }

  if (pWin -> backgroundState == BackgroundPixmap &&
          pWin -> background.pixmap == pPair -> pBackground)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentExposeBackgroundPredicate: Window at [%p] uses pixmap [%p] "
                "as background.\n", (void *) pWin, (void *) pPair -> pBackground);
    #endif

    nxagentSendBackgroundExpose(pWin, pPair -> pBackground, pPair -> pExpose);
  }
  else if (pWin -> backgroundState == ParentRelative)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentExposeBackgroundPredicate: Window [%p] uses parent's background.\n",
                (void *) pWin);
    #endif

    pParent = pWin -> parent;

    while (pParent != NULL)
    {
      if (pParent -> backgroundState == BackgroundPixmap &&
              pParent -> background.pixmap == pPair -> pBackground)
      {
        #ifdef TEST
        fprintf(stderr, "nxagentExposeBackgroundPredicate: Parent window at [%p] uses pixmap [%p] "
                    "as background.\n", (void *) pParent, (void *) pPair -> pBackground);
        #endif

        nxagentSendBackgroundExpose(pWin, pPair -> pBackground, pPair -> pExpose);

        break;
      }

      pParent = pParent -> parent;
    }
  }
}

/*
 * This function is similar to nxagentClipAndSendExpose().
 */

int nxagentClipAndSendClearExpose(WindowPtr pWin, pointer ptr)
{
  RegionPtr exposeRgn;
  RegionPtr remoteExposeRgn;

  #ifdef DEBUG
  BoxRec box;
  #endif

  remoteExposeRgn = (RegionRec *) ptr;

  if (nxagentWindowPriv(pWin) -> deferredBackgroundExpose == 1)
  {
    exposeRgn = REGION_CREATE(pWin -> drawable.pScreen, NULL, 1);

    #ifdef DEBUG
    box = *REGION_EXTENTS(pWin->drawable.pScreen, remoteExposeRgn);

    fprintf(stderr, "nxagentClipAndSendClearExpose: Background expose extents: [%d,%d,%d,%d].\n",
                box.x1, box.y1, box.x2, box.y2);

    box = *REGION_EXTENTS(pWin->drawable.pScreen, &pWin -> clipList);

    fprintf(stderr, "nxagentClipAndSendClearExpose: Clip list extents for window at [%p]: [%d,%d,%d,%d].\n",
                (void *) pWin, box.x1, box.y1, box.x2, box.y2);
    #endif

    REGION_INTERSECT(pWin -> drawable.pScreen, exposeRgn, remoteExposeRgn, &pWin -> clipList);

    /*
     * If the region will be synchronized,
     * the expose on corrupted regions can
     * be ignored.
     */

    REGION_SUBTRACT(pWin -> drawable.pScreen, exposeRgn, exposeRgn, nxagentCorruptedRegion((DrawablePtr) pWin));

    if (REGION_NOTEMPTY(pWin -> drawable.pScreen, exposeRgn))
    {
      #ifdef DEBUG
      box = *REGION_EXTENTS(pWin->drawable.pScreen, exposeRgn);

      fprintf(stderr, "nxagentClipAndSendClearExpose: Forwarding expose [%d,%d,%d,%d] to window at [%p] pWin.\n",
                  box.x1, box.y1, box.x2, box.y2, (void *) pWin);
      #endif

      REGION_SUBTRACT(pWin -> drawable.pScreen, remoteExposeRgn, remoteExposeRgn, exposeRgn);

      miWindowExposures(pWin, exposeRgn, exposeRgn);
    }

    REGION_DESTROY(pWin -> drawable.pScreen, exposeRgn);

    nxagentWindowPriv(pWin) -> deferredBackgroundExpose = 0;
  }

  if (REGION_NOTEMPTY(pWin -> drawable.pScreen, remoteExposeRgn))
  {
    #ifdef DEBUG
    fprintf(stderr, "nxagentClipAndSendClearExpose: Region not empty. Walk children.\n");
    #endif


    return WT_WALKCHILDREN;
  }
  else
  {
    #ifdef DEBUG
    fprintf(stderr, "nxagentClipAndSendClearExpose: Region empty. Stop walking.\n");
    #endif

    return WT_STOPWALKING;
  }
}

void nxagentSendDeferredBackgroundExposures(void)
{
  if (nxagentDeferredBackgroundExposures == NullRegion)
  {
    nxagentDeferredBackgroundExposures = REGION_CREATE(WindowTable[0] -> drawable.pScreen, NullBox, 1);
  }

  if (REGION_NOTEMPTY(WindowTable[0] -> drawable.pScreen, nxagentDeferredBackgroundExposures) != 0)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentSendDeferredBackgroundExposures: Going to send deferred exposures to the root window.\n");
    #endif

    TraverseTree(WindowTable[0], nxagentClipAndSendClearExpose, (void *) nxagentDeferredBackgroundExposures);

    REGION_EMPTY(WindowTable[0] -> drawable.pScreen, nxagentDeferredBackgroundExposures);
  }
}

