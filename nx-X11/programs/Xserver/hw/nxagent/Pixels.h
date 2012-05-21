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

#ifndef __Pixels_H__
#define __Pixels_H__

#include "Visual.h"
#include "Drawable.h"
#include "Composite.h"

/*
 * Count how many pixels are different
 * in the image.
 */

int nxagentUniquePixels(XImage *image);

/*
 * Convert a 32 bit pixel to 16 bit.
 */

#define Color32to16(color) \
do \
{ \
  Visual *pVisual; \
\
  pVisual = nxagentDefaultVisual(nxagentDefaultScreen); \
\
    if (pVisual -> green_mask == 0x7e0) \
    { \
      /* \
       * bit mask 5-6-5 \
       */ \
\
      color = (((color & (pVisual -> blue_mask << 3)) >> 3) |        \
                   ((color & (pVisual -> green_mask << 5)) >> 5) |   \
                       ((color & (pVisual -> red_mask << 8)) >> 8)); \
    } \
    else \
    { \
      /* \
       * bit mask 5-5-5 \
       */ \
\
      color = (((color & (pVisual -> blue_mask << 3)) >> 3) |        \
                   ((color & (pVisual -> green_mask << 6)) >> 6) |   \
                       ((color & (pVisual -> red_mask << 9)) >> 9)); \
    } \
} \
while (0)

/*
 * Rules to break the synchronization loop.
 */

#define breakOnBlocking(mask)                                \
    (((mask) != NEVER_BREAK) && ((mask) & BLOCKING_BREAK) && \
        nxagentBlocking == 1)

#define breakOnCongestion(mask)                                \
    (((mask) != NEVER_BREAK) && ((mask) & CONGESTION_BREAK) && \
        nxagentCongestion > 4)

#define breakOnBlockingOrCongestion(mask) \
    (breakOnBlocking(mask) != 0 || breakOnCongestion(mask) != 0)

#define breakOnCongestionDrawable(mask, pDrawable)             \
    (((mask) != NEVER_BREAK) && ((mask) & CONGESTION_BREAK) && \
        (nxagentCongestion > 4 ||                              \
            ((pDrawable) -> type == DRAWABLE_PIXMAP &&         \
                nxagentCongestion > 1)))

#define breakOnEvent(mask)                                \
    (((mask) != NEVER_BREAK) && ((mask) & EVENT_BREAK) && \
        nxagentUserInput(NULL) == 1)

#define canBreakOnTimeout(mask) \
    (((mask) != NEVER_BREAK) && nxagentOption(Shadow) == 0)

/*
 * Macros defining the conditions to
 * defer X requests.
 */

#define NXAGENT_SHOULD_DEFER_TRAPEZOIDS(pDrawable)       \
    (nxagentOption(DeferLevel) >= 2 &&                   \
         nxagentDrawableContainGlyphs(pDrawable) == 0 && \
             nxagentOption(LinkType) < LINK_TYPE_ADSL && \
                 nxagentCongestion > 4)

/*
FIXME: The condition checking for the render
       version is a workaround implemented to
       avoid problems with the render composi-
       te on XFree86 remote server.
*/
/*
FIXME: Changed macro: NXAGENT_SHOULD_DEFER_COMPOSITE
       to handle situation, when pSrc -> pDrawable
       is NULL. This case happens with gradients
       and solid fill.

#define NXAGENT_SHOULD_DEFER_COMPOSITE(pSrc, pMask, pDst)                 \
    ((nxagentRenderVersionMajor == 0 &&                                   \
     nxagentRenderVersionMinor == 8 &&                                \
     (pDst) -> pDrawable -> type == DRAWABLE_PIXMAP) ||                        \
         ((pDst) -> pDrawable -> type == DRAWABLE_PIXMAP &&                    \
          (nxagentDrawableStatus((pSrc) -> pDrawable) == NotSynchronized || \
          ((pMask) && nxagentDrawableStatus((pMask) -> pDrawable) == NotSynchronized)) &&  \
          nxagentOption(DeferLevel) == 1) ||               \
             (nxagentOption(DeferLevel) >= 2 &&           \
              nxagentOption(LinkType) < LINK_TYPE_ADSL))
*/
#define NXAGENT_SHOULD_DEFER_COMPOSITE(pSrc, pMask, pDst)                                                \
    ((nxagentRenderVersionMajor == 0 &&                                                                  \
      nxagentRenderVersionMinor == 8 &&                                                                  \
      (pDst) -> pDrawable -> type == DRAWABLE_PIXMAP) ||                                                 \
         (nxagentOption(DeferLevel) >= 2 &&                                                              \
          nxagentOption(LinkType) < LINK_TYPE_ADSL) ||                                                   \
             (nxagentOption(DeferLevel) == 1 &&                                                          \
              (pDst) -> pDrawable -> type == DRAWABLE_PIXMAP &&                                          \
              (((pSrc) -> pDrawable && nxagentDrawableStatus((pSrc) -> pDrawable) == NotSynchronized) || \
              ((pMask) && (pMask) -> pDrawable && nxagentDrawableStatus((pMask) -> pDrawable) == NotSynchronized))))


#define NXAGENT_SHOULD_DEFER_PUTIMAGE(pDrawable) \
    (nxagentSplitTrap == 0 &&                    \
         nxagentOption(DeferLevel) > 0)

/*
 * Macros defining the conditions to
 * start the synchronization loops of
 * resources.
 */

#define NXAGENT_SHOULD_SYNCHRONIZE_CORRUPTED_WINDOWS(mask)            \
    ((nxagentCorruptedWindows > 0 && breakOnBlockingOrCongestion(mask) == 0) || \
         mask == NEVER_BREAK)

#define NXAGENT_SHOULD_SYNCHRONIZE_CORRUPTED_BACKGROUNDS(mask)           \
    ((nxagentCorruptedWindows == 0 && nxagentCorruptedBackgrounds > 0 && \
         breakOnBlockingOrCongestion(mask) == 0) || mask == NEVER_BREAK)

#define NXAGENT_SHOULD_SYNCHRONIZE_CORRUPTED_PIXMAPS(mask)           \
    ((nxagentCorruptedWindows == 0 && nxagentCorruptedPixmaps > 0 && \
         nxagentCongestion == 0 && nxagentBlocking == 0) ||          \
             mask == NEVER_BREAK)

/*
 * Macros defining the conditions to
 * synchronize a single resource.
 */

#define NXAGENT_SHOULD_SYNCHRONIZE_WINDOW(pDrawable)  \
    (nxagentWindowIsVisible((WindowPtr) pDrawable) == 1 && \
        (nxagentDefaultWindowIsVisible() == 1 || nxagentCompositeEnable == 1))

#define MINIMUM_PIXMAP_USAGE_COUNTER 2

#define NXAGENT_SHOULD_SYNCHRONIZE_PIXMAP(pDrawable)      \
    (nxagentPixmapUsageCounter((PixmapPtr) pDrawable) >=  \
         MINIMUM_PIXMAP_USAGE_COUNTER ||                  \
             nxagentIsCorruptedBackground((PixmapPtr) pDrawable) == 1)

#endif /* __Pixels_H__ */

