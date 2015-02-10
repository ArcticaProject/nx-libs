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
#include "Events.h"
#include "Client.h"
#include "Trap.h"
#include "Split.h"
#include "Args.h"
#include "Screen.h"
#include "Pixels.h"
#include "Utils.h"

#include "NXlib.h"
#include "NXpack.h"

/*
 * Set here the required log level.
 */

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG
#undef  DUMP

/*
 * Don't pack the images having a width, a
 * height or a data size smaller or equal
 * to these thresholds.
 */

#define IMAGE_PACK_WIDTH     2
#define IMAGE_PACK_HEIGHT    2
#define IMAGE_PACK_LENGTH    512

/*
 * Compress the image with a lossless encoder
 * if the percentage of discrete pixels in the
 * image is below this threshold.
 */

#define IMAGE_UNIQUE_RATIO   10

/*
 * Introduce a small delay after each image
 * operation if the session is down. Value
 * is in microseconds and is multiplied by
 * the image data size in kilobytes.
 */

#define IMAGE_DELAY_IF_DOWN  250

/*
 * Preferred pack and split parameters we
 * got from the NX transport.
 */

int nxagentPackLossless   = -1;
int nxagentPackMethod     = -1;
int nxagentPackQuality    = -1;
int nxagentSplitThreshold = -1;

/*
 * Set if images can use the alpha channel.
 */

int nxagentAlphaEnabled = 0;
int nxagentAlphaCompat  = 0;

/*
 * Used to reformat image when connecting to
 * displays having different byte order.
 */

extern void BitOrderInvert(unsigned char *, int);
extern void TwoByteSwap(unsigned char *, register int);
extern void FourByteSwap(register unsigned char *, register int);

/*
 * Store the last visual used to unpack
 * the images for the given client.
 */

static VisualID nxagentUnpackVisualId[MAX_CONNECTIONS];

/*
 * Store the last alpha data set for the
 * client.
 */

typedef struct _UnpackAlpha
{
  char *data;
  int size;

} UnpackAlphaRec;

typedef UnpackAlphaRec *UnpackAlphaPtr;

static UnpackAlphaPtr nxagentUnpackAlpha[MAX_CONNECTIONS];

/*
 * Encode the imade alpha channel by using
 * a specific encoding, separating it from
 * the rest of the RGB data.
 */

static char *nxagentImageAlpha(XImage *ximage);
static void nxagentSetUnpackAlpha(DrawablePtr pDrawable, XImage *pImage, ClientPtr pClient);

/*
 * Copy the source data to the destination.
 */

static char *nxagentImageCopy(XImage *source, XImage *destination);

/*
 * Return true if the image can be cached.
 * Don't cache the images packed with the
 * bitmap method as the encoding is little
 * more expensive than a copy.
 */

#define nxagentNeedCache(image, method) \
\
  ((method) != PACK_BITMAP_16M_COLORS)

/*
 * With the bitmap encoding, if the image
 * is 32 bits-per-pixel the 4th byte is not
 * transmitted, so we don't need to clean
 * the image.
 */

#define nxagentNeedClean(image, method) \
\
  ((method) == PACK_RLE_16M_COLORS || \
       (method) == PACK_RGB_16M_COLORS || \
           ((method) == PACK_BITMAP_16M_COLORS && \
               image -> bits_per_pixel != 32))

/*
 * Collect the image cache statistics.
 */

typedef struct _ImageStatisticsRec
{
  double partialLookups;
  double partialMatches;
  double partialEncoded;
  double partialAdded;

  double totalLookups;
  double totalMatches;
  double totalEncoded;
  double totalAdded;

} ImageStatisticsRec;

ImageStatisticsRec nxagentImageStatistics;

int nxagentImageReformat(char *base, int nbytes, int bpp, int order)
{
  /*
   * This is used whenever we need to swap the image data.
   * If we got an image from a X server having a different
   * endianess, we will need to redormat the image to match
   * our own image-order so that ProcGetImage can return
   * the expected format to the client.
   */

  switch (bpp)
  {
    case 1:
    {
      if (BITMAP_BIT_ORDER != order)
      {
        #ifdef TEST
        fprintf(stderr, "nxagentImageReformat: Bit order invert with size [%d] "
                    "bits per pixel [%d] byte order [%d].\n", nbytes, bpp, order);
        #endif

        BitOrderInvert((unsigned char *) base, nbytes);
      }

      #if IMAGE_BYTE_ORDER != BITMAP_BIT_ORDER && BITMAP_SCANLINE_UNIT != 8

      nxagentImageReformat(base, nbytes, BITMAP_SCANLINE_UNIT, order);

      #endif

      break;
    }
    case 4:
    case 8:
    {
      break;
    }
    case 16:
    {
      if (IMAGE_BYTE_ORDER != order)
      {
        #ifdef TEST
        fprintf(stderr, "nxagentImageReformat: Two bytes swap with size [%d] "
                    "bits per pixel [%d] byte order [%d].\n", nbytes, bpp, order);
        #endif

        TwoByteSwap((unsigned char *) base, nbytes);
      }

      break;
    }
    case 32:
    {
      if (IMAGE_BYTE_ORDER != order)
      {
        #ifdef TEST
        fprintf(stderr, "nxagentImageReformat: Four bytes swap with size [%d] "
                    "bits per pixel [%d] byte order [%d].\n", nbytes, bpp, order);
        #endif

        FourByteSwap((unsigned char *) base, nbytes);
      }

      break;
    }
  }

  return 1;
}

int nxagentImageLength(int width, int height, int format, int leftPad, int depth)
{
  int line = 0;

  if (format == XYBitmap)
  {
    line = BitmapBytePad(width + leftPad);
  }
  else if (format == XYPixmap)
  {
    line  = BitmapBytePad(width + leftPad);

    line *= depth;
  }
  else if (format == ZPixmap)
  {
    line = PixmapBytePad(width, depth);
  }

  return line * height;
}

int nxagentImagePad(int width, int format, int leftPad, int depth)
{
  int line = 0;

  if (format == XYBitmap)
  {
    line = BitmapBytePad(width + leftPad);
  }
  else if (format == XYPixmap)
  {
    line = BitmapBytePad(width + leftPad);
  }
  else if (format == ZPixmap)
  {
    line = PixmapBytePad(width, depth);
  }

  return line;
}

/*
 * Only copy the data, not the structure.
 * The data pointed by the destination is
 * lost. Used to clone two images that
 * point to the same data.
 */

char *nxagentImageCopy(XImage *source, XImage *destination)
{
  #ifdef DEBUG
  fprintf(stderr, "nxagentImageClone: Copying [%d] bytes of data from the source.\n",
              source -> bytes_per_line * source -> height);
  #endif

  destination -> data = Xmalloc(source -> bytes_per_line * source -> height);

  if (destination -> data == NULL)
  {
    #ifdef PANIC
    fprintf(stderr, "nxagentImageCopy: PANIC! Cannot allocate memory for the copy.\n");
    #endif
  }
  else
  {
    memcpy(destination -> data, source -> data,
               source -> bytes_per_line * source -> height);
  }

  return destination -> data;
}

char *nxagentImageAlpha(XImage *image)
{
  char *pData;

  char *pSrcData;
  char *pDstData;

  int size;
  int offset;

  /*
   * Use one byte per pixel.
   */

  size = (image -> bytes_per_line * image -> height) >> 2;

  pData = Xmalloc(size);

  if (pData == NULL)
  {
    return NULL;
  }

  /*
   * The image is supposed to be in
   * server order.
   */

  offset = (image -> byte_order == MSBFirst) ? 0 : 3;

  pSrcData = image -> data;
  pDstData = pData;

  while (size-- > 0)
  {
    *pDstData++ = *(pSrcData + offset);

    pSrcData += 4;
  }

  return pData;
}

/*
 * Write down the image cache statistics
 * to the buffer.
 */

void nxagentImageStatisticsHandler(char **buffer, int type)
{
/*
FIXME: Agent cache statistics have to be implemented.
*/
  #ifdef TEST
  fprintf(stderr, "nxagentImageStatisticsHandler: STATISTICS! Statistics requested to the agent.\n");
  #endif

  *buffer = NULL;
}

/*
 * This should be called only for drawables
 * having a depth of 32. In the other cases,
 * it would only generate useless traffic.
 */

void nxagentSetUnpackAlpha(DrawablePtr pDrawable, XImage *pImage, ClientPtr pClient)
{
  int resource = pClient -> index;

  unsigned int size = (pImage -> bytes_per_line * pImage -> height) >> 2;

  char *data = nxagentImageAlpha(pImage);

  if (data == NULL)
  {
    #ifdef PANIC
    fprintf(stderr, "nxagentSetUnpackAlpha: PANIC! Can't allocate data for the alpha channel.\n");
    #endif

    return;
  }

  /*
   * If we are synchronizing the drawable, discard
   * any unpack alpha stored for the client. The
   * alpha data, in fact, may be still traveling
   * and so we either wait until the end of the
   * split or send a fresh copy.
   */
/*
FIXME: Here the split trap is always set and so the caching of
       the alpha channel is useless. I remember we set the trap
       because of the cursor but why is it always set now?
*/
  #ifdef DEBUG
  fprintf(stderr, "nxagentSetUnpackAlpha: Checking alpha channel for client [%d] with trap [%d].\n",
              resource, nxagentSplitTrap);
  #endif

  if (nxagentSplitTrap == 1 || nxagentUnpackAlpha[resource] == NULL ||
          nxagentUnpackAlpha[resource] -> size != size ||
              memcmp(nxagentUnpackAlpha[resource] -> data, data, size) != 0)
  {
    #ifdef DEBUG
    fprintf(stderr, "nxagentSetUnpackAlpha: Sending alpha channel with width [%d] height [%d] "
                "bytes per line [%d] size [%d].\n", pImage -> width, pImage -> height,
                    pImage -> bytes_per_line, size);
    #endif

    /*
     * Check if we are connected to a newer proxy
     * version and so can send the alpha data in
     * compressed form.
     */

    if (nxagentAlphaCompat == 0)
    {
      NXSetUnpackAlpha(nxagentDisplay, resource, PACK_NONE, size, data, size);
    }
    else
    {
      NXSetUnpackAlphaCompat(nxagentDisplay, resource, size, data);
    }

    if (nxagentUnpackAlpha[resource] != NULL)
    {
      Xfree(nxagentUnpackAlpha[resource] -> data);
    }
    else if ((nxagentUnpackAlpha[resource] = Xmalloc(sizeof(UnpackAlphaRec))) == NULL)
    {
      #ifdef PANIC
      fprintf(stderr, "nxagentSetUnpackAlpha: PANIC! Can't allocate data for the alpha structure.\n");
      #endif

      Xfree(data);

      return;
    }

    #ifdef DEBUG
    fprintf(stderr, "nxagentSetUnpackAlpha: Saved alpha channel for client [%d] with size [%d].\n",
                resource, size);
    #endif

    nxagentUnpackAlpha[resource] -> size = size;
    nxagentUnpackAlpha[resource] -> data = data;
  }
  else
  {
    #ifdef DEBUG
    fprintf(stderr, "nxagentSetUnpackAlpha: Matched alpha channel for client [%d] with size [%d].\n",
                resource, size);
    #endif

    Xfree(data);
  }
}

/*
 * The NX agent's implementation of the
 * X server's image functions.
 */

void nxagentPutImage(DrawablePtr pDrawable, GCPtr pGC, int depth,
                         int dstX, int dstY, int dstWidth, int dstHeight,
                             int leftPad, int format, char *data)
{
  int length;

  RegionPtr pRegion = NullRegion;

  int resource = 0;
  int split    = 0;
  int cache    = 1;

  #ifdef TEST
  fprintf(stderr, "nxagentPutImage: Image data at [%p] drawable [%s][%p] geometry [%d,%d,%d,%d].\n",
              data, (pDrawable -> type == DRAWABLE_PIXMAP ? "Pixmap" : "Window"),
                  (void *) pDrawable, dstX, dstY, dstWidth, dstHeight);
  #endif

  /*
   * If the display is down and there is not an
   * nxagent attached, sleep for a while but
   * still give a chance to the client to write
   * to the framebuffer.
   */

  length = nxagentImageLength(dstWidth, dstHeight, format, leftPad, depth);

  if (nxagentShadowCounter == 0 &&
          NXDisplayError(nxagentDisplay) == 1)
  {
    int us;

    us = IMAGE_DELAY_IF_DOWN * (length / 1024);

    us = (us < 10000 ? 10000 : (us > 1000000 ? 1000000 : us));

    #ifdef DEBUG
    fprintf(stderr, "nxagentPutImage: Sleeping for [%d] milliseconds with length [%d] and link down.\n",
                us / 1000, length);
    #endif

    usleep(us);
  }

  /*
   * This is of little use because clients usually write
   * to windows only after an expose event, and, in the
   * rare case they use a direct put image to the window
   * (for a media player it should be a necessity), they
   * are likely to monitor the visibility of the window.
   */

  if (nxagentOption(IgnoreVisibility) == 0 && pDrawable -> type == DRAWABLE_WINDOW &&
          (nxagentWindowIsVisible((WindowPtr) pDrawable) == 0 ||
              (nxagentDefaultWindowIsVisible() == 0 && nxagentCompositeEnable == 0)))
  {

    #ifdef TEST
    fprintf(stderr, "nxagentPutImage: WARNING! Prevented operation on fully obscured "
                "window at [%p].\n", (void *) pDrawable);
    #endif

    goto nxagentPutImageEnd;
  }

  /*
   * This is more interesting. Check if the operation
   * will produce a visible result based on the clip
   * list of the window and the GC.
   */

  pRegion = nxagentCreateRegion(pDrawable, pGC, dstX, dstY, dstWidth, dstHeight);

  if (REGION_NIL(pRegion) == 1)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentPutImage: WARNING! Prevented operation on fully clipped "
                "region [%d,%d,%d,%d] for drawable at [%p].\n", pRegion -> extents.x1,
                    pRegion -> extents.y1, pRegion -> extents.x2, pRegion -> extents.y2,
                        (void *) pDrawable);
    #endif

    goto nxagentPutImageEnd;
  }

/*
FIXME: Should use these.

  int framebuffer = 1;
  int realize     = 1;
*/
  if (nxagentGCTrap == 1 && nxagentReconnectTrap == 0 &&
          nxagentFBTrap == 0 && nxagentShmTrap == 0)
  {
    if (pDrawable -> type == DRAWABLE_PIXMAP)
    {
      fbPutImage(nxagentVirtualDrawable(pDrawable), pGC, depth,
                     dstX, dstY, dstWidth, dstHeight, leftPad, format, data);
    }
    else
    {
      fbPutImage(pDrawable, pGC, depth,
                     dstX, dstY, dstWidth, dstHeight, leftPad, format, data);
    }

    goto nxagentPutImageEnd;
  }

  if (nxagentReconnectTrap == 0 &&
          nxagentSplitTrap == 0)
  {
    if (pDrawable -> type == DRAWABLE_PIXMAP &&
            nxagentFBTrap == 0 && nxagentShmTrap == 0)
    {
      fbPutImage(nxagentVirtualDrawable(pDrawable), pGC, depth,
                     dstX, dstY, dstWidth, dstHeight, leftPad, format, data);
    }
    else if (pDrawable -> type == DRAWABLE_WINDOW)
    {
      fbPutImage(pDrawable, pGC, depth,
                     dstX, dstY, dstWidth, dstHeight, leftPad, format, data);
    }
  }

  /*
   * We are going to realize the operation
   * on the real display. Let's check if
   * the link is down.
   */

  if (NXDisplayError(nxagentDisplay) == 1)
  {
    goto nxagentPutImageEnd;
  }

  /*
   * Mark the region as corrupted and skip the operation
   * if we went out of bandwidth. The drawable will be
   * synchronized at later time. Don't do that if the
   * image is likely to be a shape or a clip mask, if we
   * are here because we are actually synchronizing the
   * drawable or if the drawable's corrupted region is
   * over-age.
   */

  if (NXAGENT_SHOULD_DEFER_PUTIMAGE(pDrawable))
  {
    if (pDrawable -> type == DRAWABLE_PIXMAP &&
            pDrawable -> depth != 1 &&
                nxagentOption(DeferLevel) >= 1)
    {
    /* -- changed by dimbor (small "bed-sheets" never need be prevented - always put) --*/
     if (dstHeight > 16)
     {
    /* -------------------------------------------------------------------------------- */
      #ifdef TEST
      fprintf(stderr, "nxagentPutImage: WARNING! Prevented operation on region [%d,%d,%d,%d] "
                  "for drawable at [%p] with drawable pixmap.\n", pRegion -> extents.x1,
                      pRegion -> extents.y1, pRegion -> extents.x2, pRegion -> extents.y2,
                          (void *) pDrawable);
      #endif
  
      nxagentMarkCorruptedRegion(pDrawable, pRegion);

      goto nxagentPutImageEnd;
    /* --- changed by dimbor ---*/
     }
    /* ------------------------- */
    }

    if (pDrawable -> type == DRAWABLE_WINDOW &&
            nxagentOption(DeferLevel) >= 2)
    {
      #ifdef TEST
      fprintf(stderr, "nxagentPutImage: WARNING! Prevented operation on region [%d,%d,%d,%d] "
                  "for drawable at [%p] with drawable window.\n", pRegion -> extents.x1,
                      pRegion -> extents.y1, pRegion -> extents.x2, pRegion -> extents.y2,
                          (void *) pDrawable);
      #endif
  
      nxagentMarkCorruptedRegion(pDrawable, pRegion);

      goto nxagentPutImageEnd;
    }
  }
  #ifdef TEST
  else
  {
    fprintf(stderr, "nxagentPutImage: Operation on drawable [%p] not skipped with nxagentSplitTrap [%d].\n",
                (void *) pDrawable, nxagentSplitTrap);
  }
  #endif

  /*
   * Check whether we need to enclose the
   * image in a split sequence.
   */
/*
FIXME: Should we disable the split with link LAN?

  split = (nxagentOption(Streaming) == 1 &&
               nxagentOption(LinkType) != LINK_TYPE_NONE &&
                   nxagentOption(LinkType) != LINK_TYPE_LAN
*/
  split = (nxagentOption(Streaming) == 1 &&
               nxagentOption(LinkType) != LINK_TYPE_NONE
/*
FIXME: Do we stream the images from GLX or Xv? If we do that,
       we should also write on the frame buffer, including the
       images put on windows, to be able to reconstruct the
       region that is out of sync. Surely we should not try to
       cache the GLX and Xv images in memory or save them in
       the image cache on disk.
*/
/*
FIXME: Temporarily stream the GLX data.

                           && nxagentGlxTrap == 0
                               && nxagentXvTrap == 0
*/
);

  /*
   * Never split images whose depth
   * is less than 15.
   */

  if (split == 1 && (nxagentSplitTrap == 1 || depth < 15))
  {
    #ifdef TEST

    if (nxagentSplitTrap == 1 ||
            nxagentReconnectTrap == 1)
    {
      fprintf(stderr, "nxagentPutImage: Not splitting with reconnection [%d] trap [%d] "
                  "depth [%d].\n", nxagentSplitTrap, nxagentReconnectTrap, depth);
    }

    #endif

    split = 0;
  }
  #ifdef TEST
  else if (split == 1)
  {
    fprintf(stderr, "nxagentPutImage: Splitting with reconnection [%d] trap [%d] "
                "depth [%d].\n", nxagentSplitTrap, nxagentReconnectTrap, depth);
  }
  #endif

  #ifdef TEST

  if (split == 1)
  {
    fprintf(stderr, "nxagentPutImage: Splitting the image with size [%d] "
                "link [%d] GLX [%d] Xv [%d].\n", length, nxagentOption(LinkType),
                    nxagentGlxTrap, nxagentXvTrap);
  }
  else if (nxagentOption(LinkType) != LINK_TYPE_NONE)
  {
    fprintf(stderr, "nxagentPutImage: Not splitting the image with size [%d] "
                "link [%d] GLX [%d] Xv [%d].\n", length, nxagentOption(LinkType),
                    nxagentGlxTrap, nxagentXvTrap);
  }

  #endif

  /*
   * If the image was originated by a GLX
   * or Xvideo request, temporarily disable
   * the use of the cache.
   */

  if (nxagentOption(LinkType) != LINK_TYPE_NONE &&
          (nxagentGlxTrap == 1 || nxagentXvTrap == 1))
  {
    #ifdef TEST
    fprintf(stderr, "nxagentPutImage: Disabling the use of the cache with GLX or Xvideo.\n");
    #endif

    NXSetCacheParameters(nxagentDisplay, 0, 1, 0, 0);

    cache = 0;
  }

  /*
   * Enclose the next messages in a split
   * sequence. The proxy will tell us if
   * the split took place.
   */

  if (split == 1)
  {
    /*
     * If the drawable is already being split,
     * expand the region. Currently drawables
     * can't have more than a single split
     * region.
     */

    if (nxagentSplitResource(pDrawable) != NULL)
    {
      #ifdef WARNING
      fprintf(stderr, "nxagentPutImage: WARNING! Expanding the region with drawable at [%p] streaming.\n",
                  (void *) pDrawable);
      #endif
/*
FIXME: Should probably intersect the region with
       the region being split to also invalidate
       the commits.
*/
      nxagentMarkCorruptedRegion(pDrawable, pRegion);

      goto nxagentPutImageEnd;
    }
    else
    {
      /*
       * Assign a new resource to the drawable.
       * Will also assign the GC to use for the
       * operation.
       */

      resource = nxagentCreateSplit(pDrawable, &pGC);

      #ifdef TEST
      fprintf(stderr, "nxagentPutImage: Resource [%d] assigned to drawable at [%p].\n",
                  resource, (void *) pDrawable);
      #endif
    }

    NXStartSplit(nxagentDisplay, resource, NXSplitModeDefault);
  }

  nxagentRealizeImage(pDrawable, pGC, depth, dstX, dstY,
                          dstWidth, dstHeight, leftPad, format, data);

  if (split == 1)
  {
    NXEndSplit(nxagentDisplay, resource);

    /*
     * Now we need to check if all the messages went
     * straight through the output stream or any of
     * them required a split. If no split will take
     * place, we will remove the association with the
     * drawable and release the resource at the time
     * we will handle the no-split event.
     */

    split = nxagentWaitSplitEvent(resource);

    if (split == 1)
    {
      #ifdef TEST
      fprintf(stderr, "nxagentPutImage: Marking corrupted region [%d,%d,%d,%d] for drawable at [%p].\n",
                  pRegion -> extents.x1, pRegion -> extents.y1, pRegion -> extents.x2,
                      pRegion -> extents.y2, (void *) pDrawable);
      #endif

      /*
       * Marking the corrupted region we will check
       * if the region intersects the split region,
       * therefore the split region must be added
       * later.
       */

      nxagentMarkCorruptedRegion(pDrawable, pRegion);

      /*
       * Assign the region to the drawable.
       */

      nxagentRegionSplit(pDrawable, pRegion);

      pRegion = NullRegion;
    }
  }

  /*
   * The split value could be changed by a
   * no-split event in the block above, so
   * here we have to check the value again.
   */

  if (split == 0)
  {
    if (nxagentDrawableStatus(pDrawable) == NotSynchronized)
    {
      /*
       * We just covered the drawable with
       * a solid image. We can consider the
       * overlapping region as synchronized.
       */

      #ifdef TEST
      fprintf(stderr, "nxagentPutImage: Marking synchronized region [%d,%d,%d,%d] for drawable at [%p].\n",
                  pRegion -> extents.x1, pRegion -> extents.y1, pRegion -> extents.x2,
                      pRegion -> extents.y2, (void *) pDrawable);
      #endif

      nxagentUnmarkCorruptedRegion(pDrawable, pRegion);
    }
  }

nxagentPutImageEnd:

  /*
   * Check if we disabled caching.
   */

  if (cache == 0)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentPutImage: Reenabling the use of the cache.\n");
    #endif

    NXSetCacheParameters(nxagentDisplay, 1, 1, 1, 1);
  }

  if (pRegion != NullRegion)
  {
    nxagentFreeRegion(pDrawable, pRegion);
  }
}

void nxagentRealizeImage(DrawablePtr pDrawable, GCPtr pGC, int depth,
                             int x, int y, int w, int h, int leftPad,
                                 int format, char *data)
{
  int length;

  int bytesPerLine;
  int numSubImages;
  int totalHeight;

  /*
   * NXPutPackedImage is longer than PutPackedImage
   * so that we subtract the bigger one to be sure.
   */

  const int subSize = (MAX_REQUEST_SIZE << 2) - sizeof(xNXPutPackedImageReq);

  Visual *pVisual = NULL;

  RegionPtr clipRegion = NullRegion;

  XImage *image = NULL;


  if (NXDisplayError(nxagentDisplay) == 1)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentRealizeImage: Returning on display error.\n");
    #endif

    goto nxagentRealizeImageEnd;
  }

  /*
   * Get the visual according to the
   * drawable and depth.
   */

  pVisual = nxagentImageVisual(pDrawable, depth);

  if (pVisual == NULL)
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentRealizeImage: WARNING! Visual not found. Using default visual.\n");
    #endif
    
    pVisual = nxagentVisuals[nxagentDefaultVisualIndex].visual;
  } 

  /*
   * Get bytes per line according to format.
   */

  bytesPerLine = nxagentImagePad(w, format, leftPad, depth);

  if (nxagentOption(Shadow) == 1 && format == ZPixmap &&
          (nxagentOption(XRatio) != DONT_SCALE ||
              nxagentOption(YRatio) != DONT_SCALE) &&
                  pDrawable == (DrawablePtr) nxagentShadowPixmapPtr)
  {
    int scaledx;
    int scaledy;

    image = XCreateImage(nxagentDisplay, pVisual, depth, ZPixmap,
                             0, data, w, h, BitmapPad(nxagentDisplay), bytesPerLine);

    if (image != NULL)
    {
      image -> byte_order = IMAGE_BYTE_ORDER;

      image -> bitmap_bit_order = BITMAP_BIT_ORDER;

      nxagentScaleImage(x, y, nxagentOption(XRatio), nxagentOption(YRatio),
                            &image, &scaledx, &scaledy);

      x = scaledx;
      y = scaledy;

      w = image -> width;
      h = image -> height;

      data = image -> data;

      /*
       * Width of image has changed.
       */

      bytesPerLine = nxagentImagePad(w, format, leftPad, depth);
    }
    #ifdef WARNING
    else
    {
      fprintf(stderr, "nxagentRealizeImage: Failed to create XImage for scaling.\n");
    }
    #endif
  }

  if (w == 0 || h == 0)
  {
    goto nxagentRealizeImageEnd;
  }

  totalHeight = h;

  length = bytesPerLine * h;

  h = (subSize < length ? subSize : length) / bytesPerLine;

  numSubImages = totalHeight / h + 1;

  while (numSubImages > 0)
  {
    if (pDrawable -> type == DRAWABLE_WINDOW)
    {
      clipRegion = nxagentCreateRegion(pDrawable, pGC, x, y, w, h);
    }

    if (clipRegion == NullRegion || REGION_NIL(clipRegion) == 0)
    {
      nxagentPutSubImage(pDrawable, pGC, depth, x, y, w, h,
                             leftPad, format, data, pVisual);
    }
    #ifdef TEST
    else
    {
      fprintf(stderr, "nxagentRealizeImage: Skipping the put sub image with geometry "
                  "[%d,%d,%d,%d] on hidden window at [%p].\n", x, y, w, h, (void *) pDrawable);
    }
    #endif

    if (clipRegion != NullRegion)
    {
      nxagentFreeRegion(pDrawable, clipRegion);
    }

    y += h;

    data += h * bytesPerLine;

    if (--numSubImages == 1)
    {
      h = totalHeight % h;

      if (h == 0)
      {
        break;
      }
    }
  }

nxagentRealizeImageEnd:

  if (image != NULL)
  {
    XDestroyImage(image);
  }
}

void nxagentPutSubImage(DrawablePtr pDrawable, GCPtr pGC, int depth,
                            int x, int y, int w, int h, int leftPad, int format,
                                char *data, Visual *pVisual)
{
  NXPackedImage *packedImage    = NULL;
  XImage        *plainImage     = NULL;
  unsigned char *packedChecksum = NULL;

  unsigned int packMethod  = nxagentPackMethod;
  unsigned int packQuality = nxagentPackQuality;

  ClientPtr client;

  int lossless = 0;
  int clean    = 0;
  int pack     = 0;

  /*
   * XCreateImage is the place where the leftPad should be passed.
   * The image data is received from our client unmodified. In
   * theory what we would need to do is just creating an appropri-
   * ate XImage structure based on the incoming data and let Xlib
   * do the rest. Probably we don't have to pass leftPad again in
   * the src_x of XPutImage otherwise the src_x would make Xlib
   * to take into account the xoffset field twice. Unfortunately
   * passing the leftPad doesn't work.
   *
   * plainImage = XCreateImage(nxagentDisplay, pVisual,
   *                           depth, format, leftPad, data,
   *                           w, h, BitmapPad(nxagentDisplay),
   *                           nxagentImagePad(w, format, leftPad, depth));
   */

  if ((plainImage = XCreateImage(nxagentDisplay, pVisual,
                                 depth, format, leftPad, data,
                                 w, h, BitmapPad(nxagentDisplay),
                                 nxagentImagePad(w, format, leftPad, depth))) == NULL)
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentPutSubImage: WARNING! Failed to create image.\n");
    #endif

    goto nxagentPutSubImageEnd;
  }

  #ifdef DEBUG
  fprintf(stderr, "nxagentPutSubImage: Handling image with geometry [%d,%d] depth [%d].\n",
              w, h, depth);

  fprintf(stderr, "nxagentPutSubImage: Default pack method is [%d] quality is [%d].\n",
              packMethod, packQuality);
  #endif

/*
FIXME: Should use an unpack resource here.
*/
  client = requestingClient;

  if (client == NULL)
  {
    client = serverClient;
 
    #ifdef TEST
    fprintf(stderr, "nxagentPutSubImage: WARNING! Using the server client with index [%d].\n",
                client -> index);
    #endif
  }

  #ifdef TEST
  fprintf(stderr, "nxagentPutSubImage: Server order is [%d] client order is [%d].\n",
              nxagentServerOrder(), nxagentClientOrder(client));
  #endif

  #ifdef TEST
  fprintf(stderr, "nxagentPutSubImage: Display image order is [%d] bitmap order is [%d].\n",
              ImageByteOrder(nxagentDisplay), BitmapBitOrder(nxagentDisplay));
  #endif

  /*
   * We got the image data from the X client or
   * from the frame-buffer with our own endianess.
   * Byte swap the image data if the display has
   * a different endianess than our own.
   */

  if (nxagentImageNormalize(plainImage) != 0)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentPutSubImage: WARNING! Reformatted the image with remote order [%d/%d].\n",
                plainImage -> byte_order, plainImage -> bitmap_bit_order);
    #endif
  }

  #ifdef TEST
  fprintf(stderr, "nxagentPutSubImage: Image has visual with RGB color masks [%0lx][%0lx][%0lx].\n",
              pVisual -> red_mask, pVisual -> green_mask, pVisual -> blue_mask);
  #endif

  /*
   * Check if the user requested to pack the
   * image but don't pack it if we are not
   * connected to a proxy or if the depth is
   * less than 15 bpp.
   */

  pack = (nxagentOption(LinkType) != LINK_TYPE_NONE &&
              packMethod != PACK_NONE && depth > 8 && format == ZPixmap);

  lossless = (packMethod == nxagentPackLossless);

  if (pack == 1 && lossless == 0)
  {
    /*
     * Force the image to be sent as a plain
     * bitmap if we don't have any lossless
     * encoder available.
     */

    if (w <= IMAGE_PACK_WIDTH || h <= IMAGE_PACK_HEIGHT ||
            nxagentImageLength(w, h, format, leftPad, depth) <=
                IMAGE_PACK_LENGTH || nxagentLosslessTrap == 1)
    {
      if (nxagentPackLossless == PACK_NONE)
      {
        #ifdef TEST
        fprintf(stderr, "nxagentPutSubImage: Disabling pack with lossless method [%d] "
                    "trap [%d] size [%d].\n", nxagentPackLossless, nxagentLosslessTrap,
                        nxagentImageLength(w, h, format, leftPad, depth));
        #endif

        pack = 0;
      }
      else
      {
        #ifdef TEST
        fprintf(stderr, "nxagentPutSubImage: Lossless encoder with geometry [%d,%d] "
                    "trap [%d] size [%d].\n", w, h, nxagentLosslessTrap,
                        nxagentImageLength(w, h, format, leftPad, depth));
        #endif

        packMethod = nxagentPackLossless;

        lossless = 1;
      }
    }
  }

  /*
   * Do we still want to pack the image?
   */

  if (pack == 1)
  {
    /*
     * Set the geometry and alpha channel
     * to be used for the unpacked image.
     */

    if (nxagentUnpackVisualId[client -> index] != pVisual -> visualid)
    {
      nxagentUnpackVisualId[client -> index] = pVisual -> visualid;

      #ifdef DEBUG
      fprintf(stderr, "nxagentPutSubImage: Sending new geometry for client [%d].\n",
                  client -> index);
      #endif

      NXSetUnpackGeometry(nxagentDisplay, client -> index, pVisual);
    }

    /*
     * Check if the image is supposed to carry
     * the alpha data in the fourth byte and,
     * if so, send the alpha channel using the
     * specific encoding.
     */

    if (plainImage -> depth == 32)
    {
      nxagentSetUnpackAlpha(pDrawable, plainImage, client);
    }

    /*
     * If the image doesn't come from the XVideo or the
     * GLX extension try to locate it in the cache. The
     * case of the lossless trap is also special, as we
     * want to eventually encode the image again using
     * a lossless compression.
     */
/*
FIXME: Should try to locate the image anyway, if the lossless
       trap is set, and if the image was encoded by a lossy
       compressor, roll back the changes and encode the image
       again using the preferred method.
*/
    if (nxagentNeedCache(plainImage, packMethod) &&
            nxagentGlxTrap == 0 && nxagentXvTrap == 0 &&
                nxagentLosslessTrap == 0 && NXImageCacheSize > 0)
    {
      /*
       * Be sure that the padding bits are
       * cleaned before calculating the MD5
       * checksum.
       */
/*
FIXME: There should be a callback registered by the agent that
       provides a statistics report, in text format, telling
       for example how many images were searched in the cache,
       how many were found, how many drawables are to be synch-
       ronized, etc. This statistics report would be included
       by the proxy in its stat output.
*/
      clean = 1;

      NXCleanImage(plainImage);

      /*
       * Will return a pointer to the image and checksum
       * taken from the cache, if found. If the image is
       * not found, the function returns a null image and
       * a pointer to the calculated checksum. It is up
       * to the application to free the memory. We will
       * use the checksum to add the image in the cache.
       */

      packedImage = NXCacheFindImage(plainImage, &packMethod, &packedChecksum);

      nxagentImageStatistics.partialLookups++;
      nxagentImageStatistics.totalLookups++;

      if (packedImage != NULL)
      {
        #ifdef DEBUG
        fprintf(stderr, "nxagentPutSubImage: Matched image of geometry [%d,%d] data size [%d] in cache.\n",
                    w, h, packedImage -> xoffset);
        #endif

        NXPutPackedImage(nxagentDisplay, client -> index, nxagentDrawable(pDrawable),
                             nxagentGC(pGC), packedImage, packMethod, depth,
                                 0, 0, x, y, w, h);

        nxagentImageStatistics.partialMatches++;
        nxagentImageStatistics.totalMatches++;

        packedChecksum = NULL;
        packedImage    = NULL;

        goto nxagentPutSubImageEnd;
      }
      #ifdef DEBUG
      else
      {
        fprintf(stderr, "nxagentPutSubImage: WARNING! Missed image of geometry [%d,%d] in cache.\n",
                    w, h);
      }
      #endif
    }

    /*
     * If a specific encoder was not mandated,
     * try to guess if a lossless encoder will
     * compress better.
     */

    if (lossless == 0 && nxagentOption(Adaptive) == 1)
    {
      int ratio = nxagentUniquePixels(plainImage);

      if (ratio <= IMAGE_UNIQUE_RATIO)
      {
        #ifdef TEST
        fprintf(stderr, "nxagentPutSubImage: Lossless encoder with geometry [%d,%d] ratio [%d%%].\n",
                    w, h, ratio);
        #endif

        packMethod = nxagentPackLossless;

        lossless = 1;
      }
      #ifdef TEST
      else
      {
        fprintf(stderr, "nxagentPutSubImage: Default encoder with geometry [%d,%d] ratio [%d%%].\n",
                    w, h, ratio);
      }
      #endif
    }

    /*
     * Encode the image using the selected
     * pack method.
     */

    if (packMethod == PACK_RLE_16M_COLORS ||
            packMethod == PACK_RGB_16M_COLORS ||
                packMethod == PACK_BITMAP_16M_COLORS)
    {
      /*
       * Cleanup the image if we didn't do that yet.
       * We assume that the JPEG and PNG compression
       * methods will actually ignore the padding
       * bytes. In other words, bitmap images prod-
       * ucing the same visual output should produce
       * compressed images that are bitwise the same
       * regardless the padding bits.
       */

      if (clean == 0)
      {
        if (nxagentNeedClean(plainImage, packMethod) == 1)
        {
          clean = 1;

          NXCleanImage(plainImage);
        }
      }

      switch (packMethod)
      {
        /*
         * If nothing is done by the bitmap encoder,
         * it saves an allocation and a memory copy
         * by setting the data field of the packed
         * image to the original data. We need to
         * check this at the time we will free the
         * packed image.
         */

        case PACK_BITMAP_16M_COLORS:
        {
          packedImage = NXEncodeBitmap(plainImage, packMethod, packQuality);

          break;
        }
        case PACK_RGB_16M_COLORS:
        {
          packedImage = NXEncodeRgb(plainImage, packMethod, packQuality);

          break;
        }
        default:
        {
          packedImage = NXEncodeRle(plainImage, packMethod, packQuality);

          break;
        }
      }
    }
    else if (packMethod >= PACK_JPEG_8_COLORS &&
                 packMethod <= PACK_JPEG_16M_COLORS)
    {
      packedImage = NXEncodeJpeg(plainImage, packMethod, packQuality);
    }
    else if (packMethod >= PACK_PNG_8_COLORS &&
                 packMethod <= PACK_PNG_16M_COLORS)
    {
      packedImage = NXEncodePng(plainImage, packMethod, packQuality);
    }
    else if (packMethod != PACK_NONE)
    {
      #ifdef WARNING
      fprintf(stderr, "nxagentPutSubImage: WARNING! Ignoring deprecated pack method [%d].\n",
                  packMethod);
      #endif

      packMethod = PACK_NONE;
    }
  }

  /*
   * If we didn't produce a valid packed
   * image, send the image as a X bitmap.
   */

  if (packedImage != NULL)
  {
    nxagentImageStatistics.partialEncoded++;
    nxagentImageStatistics.totalEncoded++;

    #ifdef DEBUG
    fprintf(stderr, "nxagentPutSubImage: Using packed image with method [%d] quality [%d] "
                "geometry [%d,%d] data size [%d].\n", packMethod, packQuality,
                    w, h, packedImage -> xoffset);
    #endif

    NXPutPackedImage(nxagentDisplay, client -> index, nxagentDrawable(pDrawable),
                         nxagentGC(pGC), packedImage, packMethod, depth,
                             0, 0, x, y, w, h);

    /*
     * Add the image only if we have a valid
     * checksum. This is the case only if we
     * originally tried to find the image in
     * cache.
     */
      
    if (NXImageCacheSize > 0 && packedChecksum != NULL)
    {
      #ifdef DEBUG
      fprintf(stderr, "nxagentPutSubImage: Adding image with geometry [%d,%d] data size [%d] "
                  "to the cache.\n", w, h, packedImage -> xoffset);
      #endif

      /*
       * Check if both the plain and the packed
       * image point to the same data. In this
       * case we need a copy.
       */

      if (packedImage -> data == plainImage -> data &&
              nxagentImageCopy(plainImage, packedImage) == NULL)
      {
        goto nxagentPutSubImageEnd;
      }

      NXCacheAddImage(packedImage, packMethod, packedChecksum);

      nxagentImageStatistics.partialAdded++;
      nxagentImageStatistics.totalAdded++;

      packedChecksum = NULL;
      packedImage    = NULL;
    }
  }
  else
  {
    /*
     * Clean the image to help the proxy to match
     * the checksum in its cache. Do that only if
     * the differential compression is enabled and
     * if the image is not supposed to carry the
     * alpha data in the fourth byte of the pixel.
     */
/*
FIXME: If we failed to encode the image by any of the available
       methods, for example if we couldn't allocate memory, we
       may need to ripristinate the alpha channel, that in the
       meanwhile was sent in the unpack alpha message. This can
       be done here, if the clean flag is true and we are going
       to send a plain image.
*/
    if (clean == 0)
    {
      clean = (nxagentOption(LinkType) != LINK_TYPE_NONE &&
                   nxagentOption(LinkType) != LINK_TYPE_LAN && depth != 32);

      if (clean == 1)
      {
        #ifdef DEBUG
        fprintf(stderr, "nxagentPutSubImage: Cleaning the image with link type [%d] and depth [%d].\n",
                    nxagentOption(LinkType), depth);
        #endif

        NXCleanImage(plainImage);
      }
      #ifdef DEBUG
      else
      {
        fprintf(stderr, "nxagentPutSubImage: Not cleaning the image with link type [%d] and depth [%d].\n",
                    nxagentOption(LinkType), depth);
      }
      #endif
    }

    #ifdef DEBUG
    fprintf(stderr, "nxagentPutSubImage: Calling XPutImage with geometry [%d,%d] and data size [%d].\n",
                w, h, plainImage -> bytes_per_line * plainImage -> height);
    #endif

    /*
     * Passing the leftPad value in src_x doesn't work.
     *
     * XPutImage(nxagentDisplay, nxagentDrawable(pDrawable),
     *               nxagentGC(pGC), plainImage, leftPad, 0, x, y, w, h);
     */

    XPutImage(nxagentDisplay, nxagentDrawable(pDrawable),
                  nxagentGC(pGC), plainImage, 0, 0, x, y, w, h);
  }

nxagentPutSubImageEnd:

  #ifdef TEST
  fprintf(stderr, "nxagentPutSubImage: Performed [%.0f] lookups in the cache with [%.0f] matches.\n",
              nxagentImageStatistics.totalLookups, nxagentImageStatistics.totalMatches);

  fprintf(stderr, "nxagentPutSubImage: Encoded [%.0f] images with [%.0f] added to the cache.\n",
              nxagentImageStatistics.totalEncoded, nxagentImageStatistics.totalAdded);
  #endif

  if (packedChecksum != NULL)
  {
    Xfree(packedChecksum);
  }

  if (packedImage != NULL)
  {
    if (packedImage -> data != NULL &&
            packedImage -> data != plainImage -> data)
    {
      Xfree(packedImage -> data);
    }

    Xfree(packedImage);
  }

  Xfree(plainImage);
}

void nxagentGetImage(DrawablePtr pDrawable, int x, int y, int w, int h,
                         unsigned int format, unsigned long planeMask, char *data)
{
  #ifdef TEST
  fprintf(stderr, "nxagentGetImage: Called with drawable at [%p] geometry [%d,%d,%d,%d].\n",
              (void *) pDrawable, x, y, w, h);

  fprintf(stderr, "nxagentGetImage: Format is [%d] plane mask is [%lx].\n",
              format, planeMask);
  #endif

  if ((pDrawable)->type == DRAWABLE_PIXMAP)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentGetImage: Going to get image from virtual pixmap at [%p].\n",
                (void *) nxagentVirtualDrawable(pDrawable));
    #endif

    fbGetImage(nxagentVirtualDrawable(pDrawable), x, y, w, h, format, planeMask, data);
  }
  else
  {
    fbGetImage(pDrawable, x, y, w, h, format, planeMask, data);
  }
}

/*
 * We have to reset the visual cache before
 * connecting to another display, so that a
 * new unpack geometry can be communicated
 * to the new proxy.
 */

void nxagentResetVisualCache()
{
  int i;

  for (i = 0; i < MAX_CONNECTIONS; i++)
  {
    nxagentUnpackVisualId[i] = None;
  }
}

void nxagentResetAlphaCache()
{
  int i;

  for (i = 0; i < MAX_CONNECTIONS; i++)
  {
    if (nxagentUnpackAlpha[i])
    {
      Xfree(nxagentUnpackAlpha[i] -> data);

      Xfree(nxagentUnpackAlpha[i]);

      nxagentUnpackAlpha[i] = NULL;
    }
  }
}

int nxagentScaleImage(int x, int y, unsigned xRatio, unsigned yRatio,
                          XImage **pImage, int *scaledx, int *scaledy)
{
  int x1;
  int x2;
  int y1;
  int y2;

  int xx1;
  int xx2;
  int yy1;
  int yy2;

  int newWidth;
  int newHeight;

  int i;
  int j;
  int k;
  int l;

  unsigned long val;

  XImage *newImage;
  XImage *image = *pImage;

  #ifdef FAST_GET_PUT_PIXEL

  register char *srcPixel;
  register char *dstPixel;

  int i;

  #endif

  if (image == NULL)
  {
    return 0;
  }

  x1 = (xRatio * x) >> PRECISION;
  x2 = (xRatio * (x + image -> width)) >> PRECISION;

  y1 = (yRatio * y) >> PRECISION;
  y2 = (yRatio * (y + image -> height)) >> PRECISION;

  newWidth = x2 - x1;
  newHeight = y2 - y1;

  newImage = XCreateImage(nxagentDisplay, NULL, image -> depth, image -> format, 0, NULL,
                              newWidth, newHeight, BitmapPad(nxagentDisplay),
                                  PixmapBytePad(newWidth, image -> depth));

  if (newImage == NULL)
  {
    #ifdef PANIC
    fprintf(stderr, "nxagentScaleImage: PANIC! Failed to create the target image.\n");
    #endif

    return 0;
  }

  newImage -> red_mask   = image -> red_mask;
  newImage -> green_mask = image -> green_mask;
  newImage -> blue_mask  = image -> blue_mask;

  newImage -> byte_order = IMAGE_BYTE_ORDER;
  newImage -> bitmap_bit_order = BITMAP_BIT_ORDER;

  newImage -> data = Xmalloc(newImage -> bytes_per_line * newHeight);

  if (newImage -> data == NULL)
  {
    Xfree(newImage);
    
    #ifdef PANIC
    fprintf(stderr, "nxagentScaleImage: PANIC! Failed to create the target image data.\n");
    #endif

    return 0;
  }

  newImage -> width  = newWidth;
  newImage -> height = newHeight;

  for (j = y; j < y + image -> height; j++)
  {
    yy1 = (yRatio * j) >> PRECISION;
    yy2 = (yRatio * (j + 1)) >> PRECISION;

    for (i = x; i < x + image -> width; i++)
    {
      #ifndef FAST_GET_PUT_PIXEL

      val = XGetPixel(image, i - x, j - y);

      #else

      srcPixel = &image -> data[(j * image -> bytes_per_line) +
                                  ((i * image -> bits_per_pixel) >> 3)];

      dstPixel = (char *) &val;

      val = 0;

      for (i = (image -> bits_per_pixel + 7) >> 3; --i >= 0; )
      {
        *dstPixel++ = *srcPixel++;
      }

      #endif

      xx1 = (xRatio * i) >> PRECISION;
      xx2 = (xRatio * (i + 1)) >> PRECISION;

      for (l = yy1; l < yy2; l++)
      {
        for (k = xx1; k < xx2; k++)
        {
          #ifndef FAST_GET_PUT_PIXEL

          XPutPixel(newImage, k - x1, l - y1, val);

          #else

          dstPixel = &newImage -> data[((l - y1) * newImage -> bytes_per_line) +
                                           (((k - x1) * newImage -> bits_per_pixel) >> 3)];

          srcPixel = (char *) &val;

          for (i = (newImage -> bits_per_pixel + 7) >> 3; --i >= 0; )
          {
            *dstPixel++ = *srcPixel++;
          }

          #endif
        }
      }
    }
  }

  if (image -> obdata != NULL)
  {
    Xfree((char *) image -> obdata);
  }

  Xfree((char *) image);

  *pImage = newImage;

  *scaledx = x1;
  *scaledy = y1;

  return 1;
}

char *nxagentAllocateImageData(int width, int height, int depth, int *length, int *format)
{
  char *data;

  int leftPad;

  leftPad = 0;

  *format = (depth == 1) ? XYPixmap : ZPixmap;

  *length = nxagentImageLength(width, height, *format, leftPad, depth);

  data = NULL;

  if ((data = xalloc(*length)) == NULL)
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentAllocateImageData: WARNING! Failed to allocate [%d] bytes of memory.\n", *length);
    #endif
  }

  return data;
}

