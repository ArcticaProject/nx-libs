/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2011 NoMachine (http://www.nomachine.com)          */
/* Copyright (c) 2008-2017 Oleksandr Shneyder <o.shneyder@phoca-gmbh.de>  */
/* Copyright (c) 2011-2022 Mike Gabriel <mike.gabriel@das-netzwerkteam.de>*/
/* Copyright (c) 2014-2019 Mihai Moldovan <ionic@ionic.de>                */
/* Copyright (c) 2014-2022 Ulrich Sibiller <uli42@gmx.de>                 */
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

#ifndef __Drawable_H__
#define __Drawable_H__

#include "resource.h"
#include "Windows.h"
#include "Pixmaps.h"

/*
 * Structures and macros used to manage the Lazy encoding.
 */

typedef struct
{
  DrawablePtr pDrawable;
  int         drawableType;
  Bool        abort;
  int         windowBitmaps;
  int         pixmapBitmaps;
  int         backgroundBitmaps;

} _nxagentSynchronizationRec;

extern _nxagentSynchronizationRec nxagentSynchronization;

enum DrawableStatus
{
  Synchronized,
  NotSynchronized
};

enum SynchronizationPredicate
{
  Needed,
  NotNeeded,
  Delayed
};

/*
 * Shall the synchronization wait for the
 * drawable wakening?
 */

#define DONT_WAIT 0
#define DO_WAIT   1

/*
 * How the synchronization loop can be
 * interrupted? A 0 mask means that the
 * loop can't be stopped.
 */

#define EVENT_BREAK       (1 << 0)
#define CONGESTION_BREAK  (1 << 1)
#define BLOCKING_BREAK    (1 << 2)

#define NEVER_BREAK  0
#define ALWAYS_BREAK (EVENT_BREAK      | \
                      CONGESTION_BREAK | \
                      BLOCKING_BREAK)

/*
 * Minimum value of usage counter which
 * a pixmap should have to be synchronized.
 */

#define MINIMUM_PIXMAP_USAGE_COUNTER 2

/*
 * This is the macro used to get the external XID
 * from a generic pointer to a drawable.
 */

#define nxagentDrawable(pDrawable) \
  ((pDrawable)->type == DRAWABLE_WINDOW ? \
      nxagentWindow((WindowPtr)pDrawable) : \
          nxagentPixmap((PixmapPtr)pDrawable))

#define nxagentVirtualDrawable(pDrawable) \
  (DrawablePtr)((pDrawable)->type == DRAWABLE_WINDOW ? \
      NULL : nxagentVirtualPixmap((PixmapPtr)pDrawable))

#define nxagentDrawablePicture(pDrawable) \
  ((DrawablePtr)((pDrawable)->type == DRAWABLE_WINDOW ? \
      nxagentWindowPriv((WindowPtr)pDrawable) -> pPicture : \
          nxagentPixmapPriv((PixmapPtr)pDrawable) -> pPicture))

#define nxagentCorruptedRegion(pDrawable) \
    ((pDrawable) -> type == DRAWABLE_PIXMAP ? \
        nxagentPixmapCorruptedRegion((PixmapPtr) pDrawable) : \
            nxagentWindowCorruptedRegion((WindowPtr) pDrawable))

#define nxagentDrawableStatus(pDrawable) \
    (RegionNil(nxagentCorruptedRegion(pDrawable)) ? \
        Synchronized : NotSynchronized)

#define nxagentDrawableContainGlyphs(pDrawable) \
    ((pDrawable) -> type == DRAWABLE_PIXMAP ? \
        nxagentPixmapContainGlyphs((PixmapPtr) (pDrawable)) : \
            nxagentWindowContainGlyphs((WindowPtr) (pDrawable)))

#define nxagentSetDrawableContainGlyphs(pDrawable, value) \
do \
{ \
  if ((pDrawable) -> type == DRAWABLE_PIXMAP) \
  { \
    nxagentPixmapContainGlyphs(nxagentRealPixmap((PixmapPtr) (pDrawable))) = (value); \
  } \
  else \
  { \
    nxagentWindowContainGlyphs((WindowPtr) (pDrawable)) = (value); \
  } \
} while (0)

#define nxagentDrawableBitmap(pDrawable) \
    ((pDrawable) -> type == DRAWABLE_PIXMAP ? \
        nxagentPixmapPriv(nxagentRealPixmap((PixmapPtr) (pDrawable))) -> synchronizationBitmap : \
            nxagentWindowPriv((WindowPtr) (pDrawable)) -> synchronizationBitmap)

#define nxagentDrawableTimestamp(pDrawable) \
    ((pDrawable) -> type == DRAWABLE_PIXMAP ? \
        nxagentPixmapTimestamp((PixmapPtr) pDrawable) : \
            nxagentWindowTimestamp((WindowPtr) pDrawable))

#define nxagentDrawableType(pDrawable) \
    ((pDrawable) -> type == DRAWABLE_PIXMAP ? "pixmap" : "window")

extern RESTYPE RT_NX_CORR_BACKGROUND;
extern RESTYPE RT_NX_CORR_WINDOW;
extern RESTYPE RT_NX_CORR_PIXMAP;

extern int nxagentCorruptedPixmaps;
extern int nxagentCorruptedWindows;
extern int nxagentCorruptedBackgrounds;

extern Bool nxagentForceSynchronization;

extern RegionPtr nxagentCreateRegion(DrawablePtr pDrawable, GCPtr pGC, int x, int y,
                                  int width, int height);

#define nxagentFreeRegion(pRegion) RegionDestroy(pRegion);

extern void nxagentMarkCorruptedRegion(DrawablePtr pDrawable, RegionPtr pRegion);
extern void nxagentUnmarkCorruptedRegion(DrawablePtr pDrawable, RegionPtr pRegion);
extern void nxagentMoveCorruptedRegion(WindowPtr pWin, unsigned int mask);
extern void nxagentIncreasePixmapUsageCounter(PixmapPtr pPixmap);

extern int  nxagentSynchronizeRegion(DrawablePtr pDrawable, RegionPtr pRegion, unsigned int breakMask, WindowPtr owner);
extern void nxagentSynchronizeBox(DrawablePtr pDrawable, BoxPtr pBox, unsigned int breakMask);
extern int  nxagentSynchronizeDrawable(DrawablePtr pDrawable, int wait, unsigned int breakMask, WindowPtr owner);
extern int  nxagentSynchronizeDrawableData(DrawablePtr pDrawable, unsigned int breakMask, WindowPtr owner);
extern void nxagentSynchronizationLoop(unsigned int mask);

extern int nxagentSynchronizationPredicate(void);

extern BoxPtr nxagentGetOptimizedRegionBoxes(RegionPtr pRegion);

extern void nxagentCleanCorruptedDrawable(DrawablePtr pDrawable);

extern void nxagentUnmarkExposedRegion(WindowPtr pWin, RegionPtr pRegion, RegionPtr pOther);
extern void nxagentSendDeferredBackgroundExposures(void);

extern void nxagentClearRegion(DrawablePtr pDrawable, RegionPtr pRegion);
extern void nxagentFillRemoteRegion(DrawablePtr pDrawable, RegionPtr pRegion);

extern void nxagentAllocateCorruptedResource(DrawablePtr pDrawable, RESTYPE type);
extern void nxagentDestroyCorruptedResource(DrawablePtr pDrawable, RESTYPE type);
extern int nxagentDestroyCorruptedBackgroundResource(void * p, XID id);
extern int nxagentDestroyCorruptedWindowResource(void * p, XID id);
extern int nxagentDestroyCorruptedPixmapResource(void * p, XID id);

extern void nxagentCreateDrawableBitmap(DrawablePtr pDrawable);
extern void nxagentDestroyDrawableBitmap(DrawablePtr pDrawable);

extern void nxagentRegionsOnScreen(void);

#define PRINT_REGION_BOXES(pRegion, strRegion) \
do \
{ \
  int i; \
  int numRects; \
  BoxPtr pBox; \
\
  if (pRegion == NullRegion) \
  { \
    fprintf(stderr, "printRegionBoxes:: Region " strRegion " is null.\n"); \
    break; \
  } \
\
  numRects = RegionNumRects(pRegion); \
  pBox = RegionRects(pRegion); \
\
  fprintf(stderr, "printRegionBoxes:: Region " strRegion " at [%p] has [%d] boxes:\n", \
              (void *) (pRegion), numRects); \
\
  for (i = 0; i < numRects; i++) \
  { \
    fprintf(stderr, "[%d] [%d,%d,%d,%d]\n", \
                i, pBox[i].x1, pBox[i].y1, pBox[i].x2, pBox[i].y2); \
  } \
\
} while (0)

#endif /* __Drawable_H__ */
