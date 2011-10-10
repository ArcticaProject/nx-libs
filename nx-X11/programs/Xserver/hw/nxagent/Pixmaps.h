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

#ifndef __Pixmap_H__
#define __Pixmap_H__

#include "Split.h"

extern RESTYPE RT_NX_PIXMAP;

/*
 * Pixmap privates structure.
 */

typedef struct
{
  Pixmap id;
  XID    mid;

  Bool isVirtual;
  Bool isShared;

  PixmapPtr pVirtualPixmap;
  PixmapPtr pRealPixmap;

  void *pPicture;

  RegionPtr corruptedRegion;

  int corruptedBackground;

  int containGlyphs;
  int containTrapezoids;

  int usageCounter;

  XID corruptedBackgroundId;
  XID corruptedId;

  PixmapPtr synchronizationBitmap;

  Time corruptedTimestamp;

  SplitResourcePtr splitResource;

  int isBackingPixmap;

} nxagentPrivPixmapRec;

typedef nxagentPrivPixmapRec *nxagentPrivPixmapPtr;

extern int nxagentPixmapPrivateIndex;

/*
 * Pixmap privates macro.
 */

#define nxagentPixmapPriv(pPixmap) \
    ((nxagentPrivPixmapPtr)((pPixmap) -> devPrivates[nxagentPixmapPrivateIndex].ptr))

#define nxagentPixmap(pPixmap) (nxagentPixmapPriv(pPixmap) -> id)

#define nxagentPixmapIsVirtual(pPixmap) \
    (nxagentPixmapPriv(pPixmap) -> isVirtual)

#define nxagentIsShmPixmap(pPixmap) \
    (nxagentPixmapPriv(pPixmap) -> isShared)

#define nxagentRealPixmap(pPixmap) \
    (nxagentPixmapPriv(pPixmap) -> pRealPixmap)

#define nxagentVirtualPixmap(pPixmap) \
    (nxagentPixmapPriv(pPixmap) -> isVirtual ? pPixmap : \
         nxagentPixmapPriv(pPixmap) -> pVirtualPixmap)

#define nxagentPixmapCorruptedRegion(pPixmap) \
    (nxagentPixmapPriv(nxagentRealPixmap(pPixmap)) -> corruptedRegion)

#define nxagentPixmapContainGlyphs(pPixmap) \
    (nxagentPixmapPriv(nxagentRealPixmap(pPixmap)) -> containGlyphs)

#define nxagentPixmapContainTrapezoids(pPixmap) \
    (nxagentPixmapPriv(nxagentRealPixmap(pPixmap)) -> containTrapezoids)

#define nxagentIsCorruptedBackground(pPixmap) \
    (nxagentPixmapPriv(nxagentRealPixmap(pPixmap)) -> corruptedBackground)

#define nxagentPixmapUsageCounter(pPixmap) \
    (nxagentPixmapPriv(nxagentRealPixmap(pPixmap)) -> usageCounter)

#define nxagentPixmapTimestamp(pPixmap) \
    (nxagentPixmapPriv(nxagentRealPixmap(pPixmap)) -> corruptedTimestamp)

PixmapPtr nxagentPixmapPtr(Pixmap pixmap);

PixmapPtr nxagentCreatePixmap(ScreenPtr pScreen, int width,
                                  int height, int depth);

Bool nxagentDestroyPixmap(PixmapPtr pPixmap);

RegionPtr nxagentPixmapToRegion(PixmapPtr pPixmap);

Bool nxagentModifyPixmapHeader(PixmapPtr pPixmap, int width, int height, int depth,
                                   int bitsPerPixel, int devKind, pointer pPixData);

RegionPtr nxagentCreateRegion(DrawablePtr pDrawable, GCPtr pGC, int x, int y,
                                  int width, int height);

void nxagentReconnectPixmap(void *p0, XID x1, void *p2);
Bool nxagentReconnectAllPixmaps(void *p0);
void nxagentDisconnectPixmap(void *p0, XID x1, void* p2);
Bool nxagentDisconnectAllPixmaps(void);

int nxagentDestroyNewPixmapResourceType(pointer p, XID id);

void nxagentSynchronizeShmPixmap(DrawablePtr pDrawable, int xPict, int yPict,
                                     int wPict, int hPict);

Bool nxagentPixmapOnShadowDisplay(PixmapPtr pMap);
Bool nxagentFbOnShadowDisplay();

#endif /* __Pixmap_H__ */
