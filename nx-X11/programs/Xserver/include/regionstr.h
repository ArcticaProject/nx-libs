/* $XdotOrg: xc/programs/Xserver/include/regionstr.h,v 1.7 2005/08/24 11:18:31 daniels Exp $ */
/* $Xorg: regionstr.h,v 1.4 2001/02/09 02:05:15 xorgcvs Exp $ */
/***********************************************************

Copyright 1987, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.


Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/
/* $XFree86: xc/programs/Xserver/include/regionstr.h,v 1.12tsi Exp $ */

#ifndef REGIONSTRUCT_H
#define REGIONSTRUCT_H

typedef struct _Region RegionRec, *RegionPtr;

#include <stddef.h>
#include <limits.h>

#include "miscstruct.h"

/* Return values from RectIn() */

#define rgnOUT 0
#define rgnIN  1
#define rgnPART 2

#define NullRegion ((RegionPtr)0)

/*
 *   clip region
 */

typedef struct _RegData {
    long	size;
    long 	numRects;
/*  BoxRec	rects[size];   in memory but not explicitly declared */
} RegDataRec, *RegDataPtr;

struct _Region {
    BoxRec 	extents;
    RegDataPtr	data;
};

extern BoxRec miEmptyBox;
extern RegDataRec miEmptyData;
extern RegDataRec miBrokenData;

#define REGION_NIL(reg) ((reg)->data && !(reg)->data->numRects)
/* not a region */
#define REGION_NAR(reg)	((reg)->data == &miBrokenData)
#define REGION_NUM_RECTS(reg) ((reg)->data ? (reg)->data->numRects : 1)
#define REGION_SIZE(reg) ((reg)->data ? (reg)->data->size : 0)
#define REGION_RECTS(reg) ((reg)->data ? (BoxPtr)((reg)->data + 1) \
			               : &(reg)->extents)
#define REGION_BOXPTR(reg) ((BoxPtr)((reg)->data + 1))
#define REGION_BOX(reg,i) (&REGION_BOXPTR(reg)[i])
#define REGION_TOP(reg) REGION_BOX(reg, (reg)->data->numRects)
#define REGION_END(reg) REGION_BOX(reg, (reg)->data->numRects - 1)
#define REGION_SZOF(n) (n < ((INT_MAX - sizeof(RegDataRec)) / sizeof(BoxRec)) ?  sizeof(RegDataRec) + ((n) * sizeof(BoxRec)) : 0)

/* Keith recommends weaning the region code of pScreen argument */
#define REG_pScreen	screenInfo.screens[0]

#ifdef NEED_SCREEN_REGIONS

#define RegionCreate(_rect, _size) \
    (*(REG_pScreen)->RegionCreate)(_rect, _size)

#define RegionInit(_pReg, _rect, _size) \
    (*(REG_pScreen)->RegionInit)(_pReg, _rect, _size)

#define RegionCopy(dst, src) \
    (*(REG_pScreen)->RegionCopy)(dst, src)

#define RegionDestroy(_pReg) \
    (*(REG_pScreen)->RegionDestroy)(_pReg)

#define RegionUninit(_pReg) \
    (*(REG_pScreen)->RegionUninit)(_pReg)

#define RegionIntersect(newReg, reg1, reg2) \
    (*(REG_pScreen)->Intersect)(newReg, reg1, reg2)

#define RegionUnion(newReg, reg1, reg2) \
    (*(REG_pScreen)->Union)(newReg, reg1, reg2)

#define RegionSubtract(newReg, reg1, reg2) \
    (*(REG_pScreen)->Subtract)(newReg, reg1, reg2)

#define RegionInverse(newReg, reg1, invRect) \
    (*(REG_pScreen)->Inverse)(newReg, reg1, invRect)

#define RegionReset(_pReg, _pBox) \
    (*(REG_pScreen)->RegionReset)(_pReg, _pBox)

#define RegionTranslate(_pReg, _x, _y) \
    (*(REG_pScreen)->TranslateRegion)(_pReg, _x, _y)

#define RegionContainsRect(_pReg, prect) \
    (*(REG_pScreen)->RectIn)(_pReg, prect)

#define RegionContainsPoint(_pReg, _x, _y, prect) \
    (*(REG_pScreen)->PointInRegion)(_pReg, _x, _y, prect)

#define RegionNotEmpty(_pReg) \
    (*(REG_pScreen)->RegionNotEmpty)(_pReg)

#define RegionEqual(_pReg1, _pReg2) \
    (*(REG_pScreen)->RegionEqual)(_pReg1, _pReg2)

#define RegionBroken(_pReg) \
    (*(REG_pScreen)->RegionBroken)(_pReg)

#define RegionBreak(_pReg) \
    (*(REG_pScreen)->RegionBreak)(_pReg)

#define RegionEmpty(_pReg) \
    (*(REG_pScreen)->RegionEmpty)(_pReg)

#define RegionExtents(_pReg) \
    (*(REG_pScreen)->RegionExtents)(_pReg)

#define RegionAppend(dstrgn, rgn) \
    (*(REG_pScreen)->RegionAppend)(dstrgn, rgn)

#define RegionValidate(badreg, pOverlap) \
    (*(REG_pScreen)->RegionValidate)(badreg, pOverlap)

#define BITMAP_TO_REGION(_pScreen, pPix) \
    (*(REG_pScreen)->BitmapToRegion)(pPix)

#define RegionFromRects(nrects, prect, ctype) \
    (*(REG_pScreen)->RectsToRegion)(nrects, prect, ctype)

#else /* !NEED_SCREEN_REGIONS */

/* Reference _pScreen macro argument and check its type */
#define REGION_SCREEN(_pScreen) (void)((REG_pScreen)->myNum)

#define RegionCreate(_rect, _size) \
    (REGION_SCREEN(_pScreen), miRegionCreate(_rect, _size))

#define RegionCopy(dst, src) \
    (REGION_SCREEN(_pScreen), miRegionCopy(dst, src))

#define RegionDestroy(_pReg) \
    (REGION_SCREEN(_pScreen), miRegionDestroy(_pReg))

#define RegionIntersect(newReg, reg1, reg2) \
    (REGION_SCREEN(_pScreen), miIntersect(newReg, reg1, reg2))

#define RegionUnion(newReg, reg1, reg2) \
    (REGION_SCREEN(_pScreen), miUnion(newReg, reg1, reg2))

#define RegionSubtract(newReg, reg1, reg2) \
    (REGION_SCREEN(_pScreen), miSubtract(newReg, reg1, reg2))

#define RegionInverse(newReg, reg1, invRect) \
    (REGION_SCREEN(_pScreen), miInverse(newReg, reg1, invRect))

#define RegionTranslate(_pReg, _x, _y) \
    (REGION_SCREEN(_pScreen), miTranslateRegion(_pReg, _x, _y))

#define RegionContainsRect(_pReg, prect) \
    (REGION_SCREEN(_pScreen), miRectIn(_pReg, prect))

#define RegionContainsPoint(_pReg, _x, _y, prect) \
    (REGION_SCREEN(_pScreen), miPointInRegion(_pReg, _x, _y, prect))

#define RegionAppend(dstrgn, rgn) \
    (REGION_SCREEN(_pScreen), miRegionAppend(dstrgn, rgn))

#define RegionValidate(badreg, pOverlap) \
    (REGION_SCREEN(_pScreen), miRegionValidate(badreg, pOverlap))

#define BITMAP_TO_REGION(_pScreen, pPix) \
    (*(_pScreen)->BitmapToRegion)(pPix) /* no mi version?! */

#define RegionFromRects(nrects, prect, ctype) \
    (REGION_SCREEN(_pScreen), miRectsToRegion(nrects, prect, ctype))

#define RegionEqual(_pReg1, _pReg2) \
    (REGION_SCREEN(_pScreen), miRegionEqual(_pReg1, _pReg2))

#define RegionBreak(_pReg) \
    (REGION_SCREEN(_pScreen), miRegionBreak(_pReg))

#ifdef DONT_INLINE_REGION_OPS

#define RegionInit(_pReg, _rect, _size) \
    (REGION_SCREEN(_pScreen), miRegionInit(_pReg, _rect, _size))

#define RegionUninit(_pReg) \
    (REGION_SCREEN(_pScreen), miRegionUninit(_pReg))

#define RegionReset(_pReg, _pBox) \
    (REGION_SCREEN(_pScreen), miRegionReset(_pReg, _pBox))

#define RegionNotEmpty(_pReg) \
    (REGION_SCREEN(_pScreen), miRegionNotEmpty(_pReg))

#define RegionBroken(_pReg) \
    (REGION_SCREEN(_pScreen), miRegionBroken(_pReg))

#define RegionEmpty(_pReg) \
    (REGION_SCREEN(_pScreen), miRegionEmpty(_pReg))

#define RegionExtents(_pReg) \
    (REGION_SCREEN(_pScreen), miRegionExtents(_pReg))

#else /* inline certain simple region ops for performance */

#define RegionInit(_pReg, _rect, _size) \
{ \
    REGION_SCREEN(_pScreen); \
    if (_rect) \
    { \
        (_pReg)->extents = *(_rect); \
        (_pReg)->data = (RegDataPtr)NULL; \
    } \
    else \
    { \
        size_t rgnSize; \
        (_pReg)->extents = miEmptyBox; \
        if (((_size) > 1) && ((rgnSize = REGION_SZOF(_size)) > 0) && \
            ((_pReg)->data = (RegDataPtr)xalloc(rgnSize))) \
        { \
            (_pReg)->data->size = (_size); \
            (_pReg)->data->numRects = 0; \
        } \
        else \
            (_pReg)->data = &miEmptyData; \
    } \
 }


#define RegionUninit(_pReg) \
{ \
    REGION_SCREEN(_pScreen); \
    if ((_pReg)->data && (_pReg)->data->size) { \
	xfree((_pReg)->data); \
	(_pReg)->data = NULL; \
    } \
}

#define RegionReset(_pReg, _pBox) \
{ \
    REGION_SCREEN(_pScreen); \
    (_pReg)->extents = *(_pBox); \
    RegionUninit(_pReg); \
    (_pReg)->data = (RegDataPtr)NULL; \
}

#define RegionNotEmpty(_pReg) \
    (REGION_SCREEN(_pScreen), !REGION_NIL(_pReg))

#define RegionBroken(_pReg) \
    (REGION_SCREEN(_pScreen), REGION_NAR(_pReg))

#define RegionEmpty(_pReg) \
{ \
    RegionUninit(_pReg); \
    (_pReg)->extents.x2 = (_pReg)->extents.x1; \
    (_pReg)->extents.y2 = (_pReg)->extents.y1; \
    (_pReg)->data = &miEmptyData; \
}

#define RegionExtents(_pReg) \
    (REGION_SCREEN(_pScreen), &(_pReg)->extents)

#define RegionNull(_pReg) \
{ \
    REGION_SCREEN(_pScreen); \
    (_pReg)->extents = miEmptyBox; \
    (_pReg)->data = &miEmptyData; \
}

#endif /* DONT_INLINE_REGION_OPS */

#endif /* NEED_SCREEN_REGIONS */

#ifndef RegionNull
#define RegionNull(_pReg) \
    RegionInit(_pReg, NullBox, 1)
#endif

/* moved from mi.h */

extern RegionPtr miRegionCreate(
    BoxPtr /*rect*/,
    int /*size*/);

extern void miRegionInit(
    RegionPtr /*pReg*/,
    BoxPtr /*rect*/,
    int /*size*/);

extern void miRegionDestroy(
    RegionPtr /*pReg*/);

extern void miRegionUninit(
    RegionPtr /*pReg*/);

extern Bool miRegionCopy(
    RegionPtr /*dst*/,
    RegionPtr /*src*/);

extern Bool miIntersect(
    RegionPtr /*newReg*/,
    RegionPtr /*reg1*/,
    RegionPtr /*reg2*/);

extern Bool miUnion(
    RegionPtr /*newReg*/,
    RegionPtr /*reg1*/,
    RegionPtr /*reg2*/);

extern Bool miRegionAppend(
    RegionPtr /*dstrgn*/,
    RegionPtr /*rgn*/);

extern Bool miRegionValidate(
    RegionPtr /*badreg*/,
    Bool * /*pOverlap*/);

extern RegionPtr miRectsToRegion(
    int /*nrects*/,
    xRectanglePtr /*prect*/,
    int /*ctype*/);

extern Bool miSubtract(
    RegionPtr /*regD*/,
    RegionPtr /*regM*/,
    RegionPtr /*regS*/);

extern Bool miInverse(
    RegionPtr /*newReg*/,
    RegionPtr /*reg1*/,
    BoxPtr /*invRect*/);

extern int miRectIn(
    RegionPtr /*region*/,
    BoxPtr /*prect*/);

extern void miTranslateRegion(
    RegionPtr /*pReg*/,
    int /*x*/,
    int /*y*/);

extern void miRegionReset(
    RegionPtr /*pReg*/,
    BoxPtr /*pBox*/);

extern Bool miRegionBreak(
    RegionPtr /*pReg*/);

extern Bool miPointInRegion(
    RegionPtr /*pReg*/,
    int /*x*/,
    int /*y*/,
    BoxPtr /*box*/);

extern Bool miRegionEqual(
    RegionPtr /*pReg1*/,
    RegionPtr /*pReg2*/);

extern Bool miRegionNotEmpty(
    RegionPtr /*pReg*/);

extern void miRegionEmpty(
    RegionPtr /*pReg*/);

extern BoxPtr miRegionExtents(
    RegionPtr /*pReg*/);

#endif /* REGIONSTRUCT_H */
