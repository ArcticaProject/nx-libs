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

#ifndef __Split_H__
#define __Split_H__

typedef struct _SplitResourceRec
{
  int         pending;
  int         split;
  int         unpack;
  DrawablePtr drawable;
  RegionPtr   region;
  GCPtr       gc;
  int         commit;

} SplitResourceRec;

typedef SplitResourceRec *SplitResourcePtr;

extern SplitResourceRec nxagentSplitResources[];

typedef struct _UnpackResourceRec
{
  int         pending;
  int         unpack;
  DrawablePtr drawable;

} UnpackResourceRec;

typedef UnpackResourceRec *UnpackResourcePtr;

extern UnpackResourceRec nxagentUnpackResources[];

void nxagentInitSplitResources();
void nxagentInitUnpackResources();

SplitResourcePtr nxagentAllocSplitResource();
void nxagentFreeSplitResource(SplitResourcePtr pResource);

UnpackResourcePtr nxagentAllocUnpackResource();
void nxagentFreeUnpackResource(UnpackResourcePtr pResource);

#define nxagentSplitResource(pDrawable) \
    ((pDrawable) -> type == DRAWABLE_PIXMAP ? \
        (nxagentPixmapPriv(nxagentRealPixmap((PixmapPtr) pDrawable)) -> splitResource) : \
            (nxagentWindowPriv((WindowPtr) pDrawable) -> splitResource))

/*
FIXME: Make it a real function to log a warning
       in the logs.

#define nxagentSplitDrawable(pDrawable) \
    (((pDrawable) -> type == DRAWABLE_PIXMAP && \
         nxagentPixmapIsVirtual((PixmapPtr) pDrawable)) ? \
             (DrawablePtr) nxagentRealPixmap((PixmapPtr) pDrawable) : pDrawable)
*/
DrawablePtr nxagentSplitDrawable(DrawablePtr pDrawable);

int nxagentCreateSplit(DrawablePtr pDrawable, GCPtr *pGC);
void nxagentRegionSplit(DrawablePtr pDrawable, RegionPtr pRegion);
void nxagentValidateSplit(DrawablePtr pDrawable, RegionPtr pRegion);
void nxagentReleaseSplit(DrawablePtr pDrawable);

void nxagentReleaseAllSplits(void);

void nxagentSetCorruptedTimestamp(DrawablePtr pDrawable);
void nxagentResetCorruptedTimestamp(DrawablePtr pDrawable);

#endif /* __Split_H__ */
