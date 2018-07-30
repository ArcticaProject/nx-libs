/*
 * $Id: damagestr.h,v 1.6 2005/07/03 07:02:01 daniels Exp $
 *
 * Copyright © 2003 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifndef _DAMAGESTR_H_
#define _DAMAGESTR_H_

#include "damage.h"
#include "gcstruct.h"
#include "privates.h"
#ifdef RENDER
# include "picturestr.h"
#endif

typedef struct _damage {
    DamagePtr		pNext;
    DamagePtr		pNextWin;
    RegionRec		damage;
    
    DamageReportLevel	damageLevel;
    Bool		isInternal;
    void		*closure;
    Bool		isWindow;
    DrawablePtr		pDrawable;
    
    DamageReportFunc	damageReport;
    DamageDestroyFunc	damageDestroy;
} DamageRec;

typedef struct _damageScrPriv {
    int				internalLevel;

    /*
     * For DDXen which don't provide GetScreenPixmap, this provides
     * a place to hook damage for windows on the screen
     */
    DamagePtr			pScreenDamage;

    PaintWindowBackgroundProcPtr PaintWindowBackground;
    PaintWindowBorderProcPtr	PaintWindowBorder;
    CopyWindowProcPtr		CopyWindow;
    CloseScreenProcPtr		CloseScreen;
    CreateGCProcPtr		CreateGC;
    DestroyPixmapProcPtr	DestroyPixmap;
    SetWindowPixmapProcPtr	SetWindowPixmap;
    DestroyWindowProcPtr	DestroyWindow;
#ifdef RENDER
    CompositeProcPtr		Composite;
    GlyphsProcPtr		Glyphs;
#endif
} DamageScrPrivRec, *DamageScrPrivPtr;

typedef struct _damageGCPriv {
    GCOps   *ops;
    GCFuncs *funcs;
} DamageGCPrivRec, *DamageGCPrivPtr;

extern DevPrivateKeyRec damageScrPrivateKeyRec;
extern DevPrivateKeyRec damagePixPrivateKeyRec;
extern DevPrivateKeyRec damageGCPrivateKeyRec;
extern DevPrivateKeyRec damageWinPrivateKeyRec;

/* XXX should move these into damage.c, damage<Xxx>PrivateKeyRec are static */
#define damageGetScrPriv(pScr) ((DamageScrPrivPtr) \
    dixLookupPrivate(&(pScr)->devPrivates, damageScrPrivateKey))

#define damageScrPriv(pScr) \
    DamageScrPrivPtr    pScrPriv = damageGetScrPriv(pScr)

#define damageGetPixPriv(pPix) \
    dixLookupPrivate(&(pPix)->devPrivates, damagePixPrivateKey)

#define damgeSetPixPriv(pPix,v) \
    dixSetPrivate(&(pPix)->devPrivates, damagePixPrivateKey, v)

#define damagePixPriv(pPix) \
    DamagePtr       pDamage = damageGetPixPriv(pPix)

#define damageGetGCPriv(pGC) \
    dixLookupPrivate(&(pGC)->devPrivates, damageGCPrivateKey)

#define damageGCPriv(pGC) \
    DamageGCPrivPtr  pGCPriv = damageGetGCPriv(pGC)

#define damageGetWinPriv(pWin) \
    ((DamagePtr)dixLookupPrivate(&(pWin)->devPrivates, damageWinPrivateKey))

#define damageSetWinPriv(pWin,d) \
    dixSetPrivate(&(pWin)->devPrivates, damageWinPrivateKey, d)

#endif /* _DAMAGESTR_H_ */
