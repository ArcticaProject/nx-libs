/*
 * midispcur.c
 *
 * machine independent cursor display routines
 */


/*

Copyright 1989, 1998  The Open Group

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
*/

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

# include   <nx-X11/X.h>
# include   "misc.h"
# include   "input.h"
# include   "cursorstr.h"
# include   "windowstr.h"
# include   "regionstr.h"
# include   "dixstruct.h"
# include   "scrnintstr.h"
# include   "servermd.h"
# include   "mipointer.h"
# include   "misprite.h"
# include   "gcstruct.h"

#ifdef ARGB_CURSOR
# include   "picturestr.h"
#endif

/* per-screen private data */

static DevPrivateKeyRec miDCScreenKeyRec;

#define miDCScreenKey (&miDCScreenKeyRec)

static Bool	miDCCloseScreen(ScreenPtr pScreen);

typedef struct {
    GCPtr	    pSourceGC, pMaskGC;
    GCPtr	    pSaveGC, pRestoreGC;
    CloseScreenProcPtr CloseScreen;
    PixmapPtr	    pSave;
#ifdef ARGB_CURSOR
    PicturePtr	    pRootPicture;
#endif
    PixmapPtr	    sourceBits;	    /* source bits */
    PixmapPtr	    maskBits;	    /* mask bits */
#ifdef ARGB_CURSOR
    PicturePtr	    pPicture;
#endif
    CursorPtr	    pCursor;
} miDCScreenRec, *miDCScreenPtr;

#define miGetDCScreen(s)       ((miDCScreenPtr)(dixLookupPrivate(&(s)->devPrivates, miDCScreenKey)))

/*
 * sprite/cursor method table
 */

static Bool	miDCRealizeCursor(ScreenPtr pScreen, CursorPtr pCursor);
static Bool	miDCUnrealizeCursor(ScreenPtr pScreen, CursorPtr pCursor);
static Bool	miDCPutUpCursor(ScreenPtr pScreen, CursorPtr pCursor,
				int x, int y, unsigned long source,
				unsigned long mask);
static Bool	miDCSaveUnderCursor(ScreenPtr pScreen, int x, int y,
				    int w, int h);
static Bool	miDCRestoreUnderCursor(ScreenPtr pScreen, int x, int y,
				       int w, int h);

static miSpriteCursorFuncRec miDCFuncs = {
    miDCRealizeCursor,
    miDCUnrealizeCursor,
    miDCPutUpCursor,
    miDCSaveUnderCursor,
    miDCRestoreUnderCursor,
};

Bool
miDCInitialize (pScreen, screenFuncs)
    ScreenPtr		    pScreen;
    miPointerScreenFuncPtr  screenFuncs;
{
    miDCScreenPtr pScreenPriv;

    if (!dixRegisterPrivateKey(&miDCScreenKeyRec, PRIVATE_SCREEN, 0))
        return FALSE;

    pScreenPriv = (miDCScreenPtr) calloc (1, sizeof (miDCScreenRec));
    if (!pScreenPriv)
	return FALSE;

    pScreenPriv->CloseScreen = pScreen->CloseScreen;
    pScreen->CloseScreen = miDCCloseScreen;

    dixSetPrivate(&pScreen->devPrivates, miDCScreenKey, pScreenPriv);

    if (!miSpriteInitialize (pScreen, &miDCFuncs, screenFuncs))
    {
	free ((void *) pScreenPriv);
	return FALSE;
    }
    return TRUE;
}

#define tossGC(gc)  (gc ? FreeGC (gc, (GContext) 0) : 0)
#define tossPix(pix)	(pix ? (*pScreen->DestroyPixmap) (pix) : TRUE)
#define tossPict(pict)	(pict ? FreePicture (pict, 0) : 0)

static void
miDCSwitchScreenCursor(ScreenPtr pScreen, CursorPtr pCursor, PixmapPtr sourceBits, PixmapPtr maskBits, PicturePtr pPicture)
{
    miDCScreenPtr pScreenPriv = dixLookupPrivate(&pScreen->devPrivates, miDCScreenKey);

    if (pScreenPriv->sourceBits)
        (*pScreen->DestroyPixmap)(pScreenPriv->sourceBits);
    pScreenPriv->sourceBits = sourceBits;

    if (pScreenPriv->maskBits)
        (*pScreen->DestroyPixmap)(pScreenPriv->maskBits);
    pScreenPriv->maskBits = maskBits;

#ifdef ARGB_CURSOR
    if (pScreenPriv->pPicture)
        FreePicture(pScreenPriv->pPicture, 0);
    pScreenPriv->pPicture = pPicture;
#endif

    pScreenPriv->pCursor = pCursor;
}

static Bool
miDCCloseScreen (pScreen)
    ScreenPtr	pScreen;
{
    miDCScreenPtr   pScreenPriv;

    pScreenPriv = (miDCScreenPtr) dixLookupPrivate(&pScreen->devPrivates, miDCScreenKey);
    pScreen->CloseScreen = pScreenPriv->CloseScreen;

    miDCSwitchScreenCursor(pScreen, NULL, NULL, NULL, NULL);
    tossGC (pScreenPriv->pSourceGC);
    tossGC (pScreenPriv->pMaskGC);
    tossGC (pScreenPriv->pSaveGC);
    tossGC (pScreenPriv->pRestoreGC);
    tossPix (pScreenPriv->pSave);
#ifdef ARGB_CURSOR
    tossPict (pScreenPriv->pRootPicture);
#endif
    free ((void *) pScreenPriv);
    return (*pScreen->CloseScreen) (pScreen);
}
static Bool
miDCRealizeCursor (pScreen, pCursor)
    ScreenPtr	pScreen;
    CursorPtr	pCursor;
{
    return TRUE;
}

#ifdef ARGB_CURSOR
#define EnsurePicture(picture,draw,win) (picture || miDCMakePicture(&picture,draw,win))

static PicturePtr
miDCMakePicture (PicturePtr *ppPicture, DrawablePtr pDraw, WindowPtr pWin)
{
    PictFormatPtr   pFormat;
    XID		    subwindow_mode = IncludeInferiors;
    PicturePtr	    pPicture;
    int		    error;


    pFormat = PictureWindowFormat(pWin);
    if (!pFormat)
	return 0;
    pPicture = CreatePicture (0, pDraw, pFormat,
			      CPSubwindowMode, &subwindow_mode,
			      serverClient, &error);
    *ppPicture = pPicture;
    return pPicture;
}
#endif

static Bool
miDCRealize (
    ScreenPtr	pScreen,
    CursorPtr	pCursor)
{
    miDCScreenPtr   pScreenPriv =  dixLookupPrivate(&pScreen->devPrivates, miDCScreenKey);
    GCPtr	    pGC;
    XID		    gcvals[3];
    PixmapPtr   sourceBits, maskBits;

    if (pScreenPriv->pCursor == pCursor)
	return TRUE;

#ifdef ARGB_CURSOR
    if (pCursor->bits->argb)
    {
	PixmapPtr	pPixmap;
	PictFormatPtr	pFormat;
	int		error;
	PicturePtr	pPicture;
	
	pFormat = PictureMatchFormat (pScreen, 32, PICT_a8r8g8b8);
	if(!pFormat)
	    return FALSE;
	
	pPixmap = (*pScreen->CreatePixmap) (pScreen, pCursor->bits->width,
					    pCursor->bits->height, 32,
					    CREATE_PIXMAP_USAGE_SCRATCH);
	if (!pPixmap)
	    return FALSE;

	pGC = GetScratchGC (32, pScreen);
	if (!pGC)
	{
	    (*pScreen->DestroyPixmap) (pPixmap);
	    return FALSE;
	}
	ValidateGC (&pPixmap->drawable, pGC);
	(*pGC->ops->PutImage) (&pPixmap->drawable, pGC, 32,
			       0, 0, pCursor->bits->width,
			       pCursor->bits->height,
			       0, ZPixmap, (char *) pCursor->bits->argb);
	FreeScratchGC (pGC);
	pPicture = CreatePicture(0, &pPixmap->drawable,
	                         pFormat, 0, 0, serverClient, &error);
        (*pScreen->DestroyPixmap) (pPixmap);
	if (!pPicture)
	    return FALSE;
	miDCSwitchScreenCursor(pScreen, pCursor, NULL, NULL, pPicture);
	return TRUE;
    }
#endif
    sourceBits = (*pScreen->CreatePixmap) (pScreen, pCursor->bits->width,
                                           pCursor->bits->height, 1, 0);
    if (!sourceBits)
	return FALSE;

    maskBits = (*pScreen->CreatePixmap) (pScreen, pCursor->bits->width,
                                         pCursor->bits->height, 1, 0);
    if (!maskBits) {
	(*pScreen->DestroyPixmap) (sourceBits);
	 return FALSE;
    }

    /* create the two sets of bits, clipping as appropriate */

    pGC = GetScratchGC (1, pScreen);
    if (!pGC)
    {
	(*pScreen->DestroyPixmap) (sourceBits);
	(*pScreen->DestroyPixmap) (maskBits);
	return FALSE;
    }

    ValidateGC ((DrawablePtr) sourceBits, pGC);
    (*pGC->ops->PutImage) ((DrawablePtr) sourceBits, pGC, 1,
			   0, 0, pCursor->bits->width, pCursor->bits->height,
 			   0, XYPixmap, (char *)pCursor->bits->source);
    gcvals[0] = GXand;
    ChangeGC (pGC, GCFunction, gcvals);
    ValidateGC ((DrawablePtr) sourceBits, pGC);
    (*pGC->ops->PutImage) ((DrawablePtr) sourceBits, pGC, 1,
			   0, 0, pCursor->bits->width, pCursor->bits->height,
 			   0, XYPixmap, (char *)pCursor->bits->mask);

    /* mask bits -- pCursor->mask & ~pCursor->source */
    gcvals[0] = GXcopy;
    ChangeGC (pGC, GCFunction, gcvals);
    ValidateGC ((DrawablePtr) maskBits, pGC);
    (*pGC->ops->PutImage) ((DrawablePtr) maskBits, pGC, 1,
			   0, 0, pCursor->bits->width, pCursor->bits->height,
 			   0, XYPixmap, (char *)pCursor->bits->mask);
    gcvals[0] = GXandInverted;
    ChangeGC (pGC, GCFunction, gcvals);
    ValidateGC ((DrawablePtr) maskBits, pGC);
    (*pGC->ops->PutImage) ((DrawablePtr) maskBits, pGC, 1,
			   0, 0, pCursor->bits->width, pCursor->bits->height,
 			   0, XYPixmap, (char *)pCursor->bits->source);
    FreeScratchGC (pGC);

    miDCSwitchScreenCursor(pScreen, pCursor, sourceBits, maskBits, NULL);
    return TRUE;
}

static Bool
miDCUnrealizeCursor (pScreen, pCursor)
    ScreenPtr	pScreen;
    CursorPtr	pCursor;
{
    miDCScreenPtr pScreenPriv = dixLookupPrivate(&pScreen->devPrivates, miDCScreenKey);

    if (pCursor == pScreenPriv->pCursor)
	miDCSwitchScreenCursor(pScreen, NULL, NULL, NULL, NULL);
    return TRUE;
}

static void
miDCPutBits (
    DrawablePtr	    pDrawable,
    GCPtr	    sourceGC,
    GCPtr	    maskGC,
    int             x_org,
    int             y_org,
    unsigned        w,
    unsigned        h,
    unsigned long   source,
    unsigned long   mask)
{
    miDCScreenPtr pScreenPriv = dixLookupPrivate(&pDrawable->pScreen->devPrivates, miDCScreenKey);
    XID	    gcvals[1];
    int     x, y;

    if (sourceGC->fgPixel != source)
    {
	gcvals[0] = source;
	DoChangeGC (sourceGC, GCForeground, gcvals, 0);
    }
    if (sourceGC->serialNumber != pDrawable->serialNumber)
	ValidateGC (pDrawable, sourceGC);

    if(sourceGC->miTranslate) 
    {
        x = pDrawable->x + x_org;
        y = pDrawable->y + y_org;
    } 
    else
    {
        x = x_org;
        y = y_org;
    }

    (*sourceGC->ops->PushPixels) (sourceGC, pScreenPriv->sourceBits, pDrawable, w, h, x, y);
    if (maskGC->fgPixel != mask)
    {
	gcvals[0] = mask;
	DoChangeGC (maskGC, GCForeground, gcvals, 0);
    }
    if (maskGC->serialNumber != pDrawable->serialNumber)
	ValidateGC (pDrawable, maskGC);

    if(maskGC->miTranslate) 
    {
        x = pDrawable->x + x_org;
        y = pDrawable->y + y_org;
    } 
    else
    {
        x = x_org;
        y = y_org;
    }

    (*maskGC->ops->PushPixels) (maskGC, pScreenPriv->maskBits, pDrawable, w, h, x, y);
}

#define EnsureGC(gc,win) (gc || miDCMakeGC(&gc, win))

static GCPtr
miDCMakeGC(
    GCPtr	*ppGC,
    WindowPtr	pWin)
{
    GCPtr pGC;
    int   status;
    XID   gcvals[2];

    gcvals[0] = IncludeInferiors;
    gcvals[1] = FALSE;
    pGC = CreateGC((DrawablePtr)pWin,
		   GCSubwindowMode|GCGraphicsExposures, gcvals, &status);
    *ppGC = pGC;
    return pGC;
}


static Bool
miDCPutUpCursor (pScreen, pCursor, x, y, source, mask)
    ScreenPtr	    pScreen;
    CursorPtr	    pCursor;
    int		    x, y;
    unsigned long   source, mask;
{
    miDCScreenPtr   pScreenPriv = dixLookupPrivate(&pScreen->devPrivates, miDCScreenKey);
    WindowPtr	    pWin;

    if (!miDCRealize(pScreen, pCursor))
	return FALSE;

    pWin = pScreen->root;
#ifdef ARGB_CURSOR
    if (pScreenPriv->pPicture)
    {
	if (!EnsurePicture(pScreenPriv->pRootPicture, &pWin->drawable, pWin))
	    return FALSE;
	CompositePicture (PictOpOver,
			  pScreenPriv->pPicture,
			  NULL,
			  pScreenPriv->pRootPicture,
			  0, 0, 0, 0, 
			  x, y, 
			  pCursor->bits->width,
			  pCursor->bits->height);
    }
    else
#endif
    {
	if (!EnsureGC(pScreenPriv->pSourceGC, pWin))
	    return FALSE;
	if (!EnsureGC(pScreenPriv->pMaskGC, pWin))
	{
	    FreeGC (pScreenPriv->pSourceGC, (GContext) 0);
	    pScreenPriv->pSourceGC = 0;
	    return FALSE;
	}
	miDCPutBits ((DrawablePtr)pWin,
		     pScreenPriv->pSourceGC, pScreenPriv->pMaskGC,
		     x, y, pCursor->bits->width, pCursor->bits->height,
		     source, mask);
    }
    return TRUE;
}

static Bool
miDCSaveUnderCursor (pScreen, x, y, w, h)
    ScreenPtr	pScreen;
    int		x, y, w, h;
{
    miDCScreenPtr   pScreenPriv;
    PixmapPtr	    pSave;
    WindowPtr	    pWin;
    GCPtr	    pGC;

    pScreenPriv = (miDCScreenPtr)dixLookupPrivate(&pScreen->devPrivates, miDCScreenKey);
    pSave = pScreenPriv->pSave;
    pWin = pScreen->root;
    if (!pSave || pSave->drawable.width < w || pSave->drawable.height < h)
    {
	if (pSave)
	    (*pScreen->DestroyPixmap) (pSave);
	pScreenPriv->pSave = pSave =
		(*pScreen->CreatePixmap) (pScreen, w, h, pScreen->rootDepth, 0);
	if (!pSave)
	    return FALSE;
    }
    if (!EnsureGC(pScreenPriv->pSaveGC, pWin))
	return FALSE;
    pGC = pScreenPriv->pSaveGC;
    if (pSave->drawable.serialNumber != pGC->serialNumber)
	ValidateGC ((DrawablePtr) pSave, pGC);
    (*pGC->ops->CopyArea) ((DrawablePtr) pWin, (DrawablePtr) pSave, pGC,
			    x, y, w, h, 0, 0);
    return TRUE;
}

static Bool
miDCRestoreUnderCursor (pScreen, x, y, w, h)
    ScreenPtr	pScreen;
    int		x, y, w, h;
{
    miDCScreenPtr   pScreenPriv;
    PixmapPtr	    pSave;
    WindowPtr	    pWin;
    GCPtr	    pGC;

    pScreenPriv = (miDCScreenPtr)dixLookupPrivate(&pScreen->devPrivates, miDCScreenKey);
    pSave = pScreenPriv->pSave;
    pWin = pScreen->root;
    if (!pSave)
	return FALSE;
    if (!EnsureGC(pScreenPriv->pRestoreGC, pWin))
	return FALSE;
    pGC = pScreenPriv->pRestoreGC;
    if (pWin->drawable.serialNumber != pGC->serialNumber)
	ValidateGC ((DrawablePtr) pWin, pGC);
    (*pGC->ops->CopyArea) ((DrawablePtr) pSave, (DrawablePtr) pWin, pGC,
			    0, 0, w, h, x, y);
    return TRUE;
}
