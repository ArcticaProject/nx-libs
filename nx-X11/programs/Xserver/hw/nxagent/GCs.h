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

#ifndef __GC_H__
#define __GC_H__

extern RESTYPE RT_NX_GC;

/* This file uses the GC definition form Xlib.h as XlibGC. */

typedef struct {
  XlibGC gc;

  int nClipRects;

  XGCValues lastServerValues;

  XID mid;

  PixmapPtr pPixmap;

} nxagentPrivGC;

extern int nxagentGCPrivateIndex;

typedef struct _nxagentGraphicContextsRec
{
  int   depth;
  GCPtr pGC;
  int   dirty;

} nxagentGraphicContextsRec;

typedef nxagentGraphicContextsRec *nxagentGraphicContextsPtr;
extern nxagentGraphicContextsPtr nxagentGraphicContexts;
extern int nxagentGraphicContextsSize;

#define nxagentGCPriv(pGC) \
  ((nxagentPrivGC *)((pGC) -> devPrivates[nxagentGCPrivateIndex].ptr))

#define nxagentGC(pGC) (nxagentGCPriv(pGC) -> gc)

#define nxagentCopyGCPriv(valueMask, valueField, src, mask, dst) \
\
  if (mask & valueMask) \
  { \
    nxagentGCPriv(dst) -> lastServerValues.valueField = \
           nxagentGCPriv(src) -> lastServerValues.valueField; \
  }

#define nxagentTestGC(newValue, pvalue) \
\
  ((nxagentGCPriv(pGC) -> lastServerValues.pvalue == newValue) ? 0 : 1); \
\
  nxagentGCPriv(pGC) -> lastServerValues.pvalue = newValue

Bool nxagentCreateGC(GCPtr pGC);
void nxagentValidateGC(GCPtr pGC, unsigned long changes, DrawablePtr pDrawable);
void nxagentChangeGC(GCPtr pGC, unsigned long mask);
void nxagentCopyGC(GCPtr pGCSrc, unsigned long mask, GCPtr pGCDst);
void nxagentDestroyGC(GCPtr pGC);
void nxagentChangeClip(GCPtr pGC, int type, pointer pValue, int nRects);
void nxagentDestroyClip(GCPtr pGC);
void nxagentDestroyClipHelper(GCPtr pGC);
void nxagentCopyClip(GCPtr pGCDst, GCPtr pGCSrc);

void nxagentDisconnectGC(pointer p0, XID x1, pointer p2);
Bool nxagentDisconnectAllGCs(void);

Bool nxagentReconnectAllGCs(void *p0);

int nxagentDestroyNewGCResourceType(pointer p, XID id);

void nxagentFreeGCList(void);
void nxagentInitGCSafeVector(void);

GCPtr nxagentGetScratchGC(unsigned depth, ScreenPtr pScreen);
void nxagentFreeScratchGC(GCPtr pGC);

GCPtr nxagentGetGraphicContext(DrawablePtr pDrawable);
void nxagentAllocateGraphicContexts(void);

#endif /* __GC_H__ */
