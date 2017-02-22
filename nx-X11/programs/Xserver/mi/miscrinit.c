/*

Copyright 1990, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from The Open Group.

*/

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <nx-X11/X.h>
#include "servermd.h"
#include "misc.h"
#include "mi.h"
#include "scrnintstr.h"
#include "pixmapstr.h"
#include "dix.h"
#include "miline.h"
#ifdef MITSHM
#include "shmint.h"
#endif

/* We use this structure to propogate some information from miScreenInit to
 * miCreateScreenResources.  miScreenInit allocates the structure, fills it
 * in, and puts it into pScreen->devPrivate.  miCreateScreenResources 
 * extracts the info and frees the structure.  We could've accomplished the
 * same thing by adding fields to the screen structure, but they would have
 * ended up being redundant, and would have exposed this mi implementation
 * detail to the whole server.
 */

typedef struct
{
    void * pbits; /* pointer to framebuffer */
    int width;    /* delta to add to a framebuffer addr to move one row down */
} miScreenInitParmsRec, *miScreenInitParmsPtr;


/* this plugs into pScreen->ModifyPixmapHeader */
Bool
miModifyPixmapHeader(pPixmap, width, height, depth, bitsPerPixel, devKind,
		     pPixData)
    PixmapPtr   pPixmap;
    int		width;
    int		height;
    int		depth;
    int		bitsPerPixel;
    int		devKind;
    void        *pPixData;
{
    if (!pPixmap)
	return FALSE;

    /*
     * If all arguments are specified, reinitialize everything (including
     * validated state).
     */
    if ((width > 0) && (height > 0) && (depth > 0) && (bitsPerPixel > 0) &&
	(devKind > 0) && pPixData) {
	pPixmap->drawable.depth = depth;
	pPixmap->drawable.bitsPerPixel = bitsPerPixel;
	pPixmap->drawable.id = 0;
	pPixmap->drawable.serialNumber = NEXT_SERIAL_NUMBER;
	pPixmap->drawable.x = 0;
	pPixmap->drawable.y = 0;
	pPixmap->drawable.width = width;
	pPixmap->drawable.height = height;
	pPixmap->devKind = devKind;
	pPixmap->refcnt = 1;
	pPixmap->devPrivate.ptr = pPixData;
    } else {
	/*
	 * Only modify specified fields, keeping all others intact.
	 */

	if (width > 0)
	    pPixmap->drawable.width = width;

	if (height > 0)
	    pPixmap->drawable.height = height;

	if (depth > 0)
	    pPixmap->drawable.depth = depth;

	if (bitsPerPixel > 0)
	    pPixmap->drawable.bitsPerPixel = bitsPerPixel;
	else if ((bitsPerPixel < 0) && (depth > 0))
	    pPixmap->drawable.bitsPerPixel = BitsPerPixel(depth);

	/*
	 * CAVEAT:  Non-SI DDXen may use devKind and devPrivate fields for
	 *          other purposes.
	 */
	if (devKind > 0)
	    pPixmap->devKind = devKind;
	else if ((devKind < 0) && ((width > 0) || (depth > 0)))
	    pPixmap->devKind = PixmapBytePad(pPixmap->drawable.width,
		pPixmap->drawable.depth);

	if (pPixData)
	    pPixmap->devPrivate.ptr = pPixData;
    }
    return TRUE;
}


/*ARGSUSED*/
Bool
miCloseScreen (iScreen, pScreen)
    int		iScreen;
    ScreenPtr	pScreen;
{
    return ((*pScreen->DestroyPixmap)((PixmapPtr)pScreen->devPrivate));
}

/* With the introduction of pixmap privates, the "screen pixmap" can no
 * longer be created in miScreenInit, since all the modules that could
 * possibly ask for pixmap private space have not been initialized at
 * that time.  pScreen->CreateScreenResources is called after all
 * possible private-requesting modules have been inited; we create the
 * screen pixmap here.
 */
Bool
miCreateScreenResources(pScreen)
    ScreenPtr pScreen;
{
    miScreenInitParmsPtr pScrInitParms;
    void * value;

    pScrInitParms = (miScreenInitParmsPtr)pScreen->devPrivate;

    /* if width is non-zero, pScreen->devPrivate will be a pixmap
     * else it will just take the value pbits
     */
    if (pScrInitParms->width)
    {
	PixmapPtr pPixmap;

	/* create a pixmap with no data, then redirect it to point to
	 * the screen
	 */
	pPixmap = (*pScreen->CreatePixmap)(pScreen, 0, 0, pScreen->rootDepth, 0);
	if (!pPixmap)
	    return FALSE;

	if (!(*pScreen->ModifyPixmapHeader)(pPixmap, pScreen->width,
		    pScreen->height, pScreen->rootDepth,
		    BitsPerPixel(pScreen->rootDepth),
		    PixmapBytePad(pScrInitParms->width, pScreen->rootDepth),
		    pScrInitParms->pbits))
	    return FALSE;
	value = (void *)pPixmap;
    }
    else
    {
	value = pScrInitParms->pbits;
    }
    free(pScreen->devPrivate); /* freeing miScreenInitParmsRec */
    pScreen->devPrivate = value; /* pPixmap or pbits */
    return TRUE;
}

Bool
miScreenDevPrivateInit(pScreen, width, pbits)
    register ScreenPtr pScreen;
    int width;
    void * pbits;
{
    miScreenInitParmsPtr pScrInitParms;

    /* Stash pbits and width in a short-lived miScreenInitParmsRec attached
     * to the screen, until CreateScreenResources can put them in the
     * screen pixmap.
     */
    pScrInitParms = (miScreenInitParmsPtr)malloc(sizeof(miScreenInitParmsRec));
    if (!pScrInitParms)
	return FALSE;
    pScrInitParms->pbits = pbits;
    pScrInitParms->width = width;
    pScreen->devPrivate = (void *)pScrInitParms;
    return TRUE;
}

Bool
miScreenInit(pScreen, pbits, xsize, ysize, dpix, dpiy, width,
	     rootDepth, numDepths, depths, rootVisual, numVisuals, visuals)
    register ScreenPtr pScreen;
    void * pbits;		/* pointer to screen bits */
    int xsize, ysize;		/* in pixels */
    int dpix, dpiy;		/* dots per inch */
    int width;			/* pixel width of frame buffer */
    int rootDepth;		/* depth of root window */
    int numDepths;		/* number of depths supported */
    DepthRec *depths;		/* supported depths */
    VisualID rootVisual;	/* root visual */
    int numVisuals;		/* number of visuals supported */
    VisualRec *visuals;		/* supported visuals */
{
    pScreen->width = xsize;
    pScreen->height = ysize;
    pScreen->mmWidth = (xsize * 254 + dpix * 5) / (dpix * 10);
    pScreen->mmHeight = (ysize * 254 + dpiy * 5) / (dpiy * 10);
    pScreen->numDepths = numDepths;
    pScreen->rootDepth = rootDepth;
    pScreen->allowedDepths = depths;
    pScreen->rootVisual = rootVisual;
    /* defColormap */
    pScreen->minInstalledCmaps = 1;
    pScreen->maxInstalledCmaps = 1;
    pScreen->backingStoreSupport = NotUseful;
    pScreen->saveUnderSupport = NotUseful;
    /* whitePixel, blackPixel */
    pScreen->ModifyPixmapHeader = miModifyPixmapHeader;
    pScreen->CreateScreenResources = miCreateScreenResources;
    pScreen->GetScreenPixmap = miGetScreenPixmap;
    pScreen->SetScreenPixmap = miSetScreenPixmap;
    pScreen->numVisuals = numVisuals;
    pScreen->visuals = visuals;
    if (width)
    {
#ifdef MITSHM
	ShmRegisterFbFuncs(pScreen);
#endif
	pScreen->CloseScreen = miCloseScreen;
    }
    /* else CloseScreen */
    /* QueryBestSize, SaveScreen, GetImage, GetSpans */
    pScreen->PointerNonInterestBox = (PointerNonInterestBoxProcPtr) 0;
    pScreen->SourceValidate = (SourceValidateProcPtr) 0;
    /* CreateWindow, DestroyWindow, PositionWindow, ChangeWindowAttributes */
    /* RealizeWindow, UnrealizeWindow */
    pScreen->ValidateTree = miValidateTree;
    pScreen->PostValidateTree = (PostValidateTreeProcPtr) 0;
    pScreen->WindowExposures = miWindowExposures;
    /* PaintWindowBackground, PaintWindowBorder, CopyWindow */
    pScreen->ClearToBackground = miClearToBackground;
    pScreen->ClipNotify = (ClipNotifyProcPtr) 0;
    pScreen->RestackWindow = (RestackWindowProcPtr) 0;
    /* CreatePixmap, DestroyPixmap */
    /* RealizeFont, UnrealizeFont */
    /* CreateGC */
    /* CreateColormap, DestroyColormap, InstallColormap, UninstallColormap */
    /* ListInstalledColormaps, StoreColors, ResolveColor */
#ifdef NEED_SCREEN_REGIONS
    pScreen->RegionCreate = RegionCreate;
    pScreen->RegionInit = RegionInit;
    pScreen->RegionCopy = RegionCopy;
    pScreen->RegionDestroy = RegionDestroy;
    pScreen->RegionUninit = RegionUninit;
    pScreen->Intersect = RegionIntersect;
    pScreen->Union = RegionUnion;
    pScreen->Subtract = RegionSubtract;
    pScreen->Inverse = RegionInverse;
    pScreen->RegionReset = RegionReset;
    pScreen->TranslateRegion = RegionTranslate;
    pScreen->RectIn = RegionContainsRect;
    pScreen->PointInRegion = RegionContainsPoint;
    pScreen->RegionNotEmpty = RegionNotEmpty;
    pScreen->RegionEqual = RegionEqual;
    pScreen->RegionBroken = RegionBroken;
    pScreen->RegionBreak = RegionBreak;
    pScreen->RegionEmpty = RegionEmpty;
    pScreen->RegionExtents = RegionExtents;
    pScreen->RegionAppend = RegionAppend;
    pScreen->RegionValidate = RegionValidate;
#endif /* NEED_SCREEN_REGIONS */
    /* BitmapToRegion */
#ifdef NEED_SCREEN_REGIONS
    pScreen->RectsToRegion = RegionFromRects;
#endif /* NEED_SCREEN_REGIONS */
    pScreen->SendGraphicsExpose = miSendGraphicsExpose;
    pScreen->BlockHandler = (ScreenBlockHandlerProcPtr)NoopDDA;
    pScreen->WakeupHandler = (ScreenWakeupHandlerProcPtr)NoopDDA;
    pScreen->blockData = (void *)0;
    pScreen->wakeupData = (void *)0;
    pScreen->MarkWindow = miMarkWindow;
    pScreen->MarkOverlappedWindows = miMarkOverlappedWindows;
    pScreen->ChangeSaveUnder = miChangeSaveUnder;
    pScreen->PostChangeSaveUnder = miPostChangeSaveUnder;
    pScreen->MoveWindow = miMoveWindow;
    pScreen->ResizeWindow = miSlideAndSizeWindow;
    pScreen->GetLayerWindow = miGetLayerWindow;
    pScreen->HandleExposures = miHandleValidateExposures;
    pScreen->ReparentWindow = (ReparentWindowProcPtr) 0;
    pScreen->ChangeBorderWidth = miChangeBorderWidth;
#ifdef SHAPE
    pScreen->SetShape = miSetShape;
#endif
    pScreen->MarkUnrealizedWindow = miMarkUnrealizedWindow;

    pScreen->SaveDoomedAreas = 0;
    pScreen->RestoreAreas = 0;
    pScreen->ExposeCopy = 0;
    pScreen->TranslateBackingStore = 0;
    pScreen->ClearBackingStore = 0;
    pScreen->DrawGuarantee = 0;

    miSetZeroLineBias(pScreen, DEFAULTZEROLINEBIAS);

    return miScreenDevPrivateInit(pScreen, width, pbits);
}

int
miAllocateGCPrivateIndex()
{
    static int privateIndex = -1;
    static unsigned long miGeneration = 0;

    if (miGeneration != serverGeneration)
    {
	privateIndex = AllocateGCPrivateIndex();
	miGeneration = serverGeneration;
    }
    return privateIndex;
}

int miZeroLineScreenIndex;
unsigned int miZeroLineGeneration = 0;

void
miSetZeroLineBias(pScreen, bias)
    ScreenPtr pScreen;
    unsigned int bias;
{
    if (miZeroLineGeneration != serverGeneration)
    {
	miZeroLineScreenIndex = AllocateScreenPrivateIndex();
	miZeroLineGeneration = serverGeneration;
    }
    if (miZeroLineScreenIndex >= 0)
	pScreen->devPrivates[miZeroLineScreenIndex].uval = bias;
}

PixmapPtr
miGetScreenPixmap(pScreen)
    ScreenPtr pScreen;
{
    return (PixmapPtr)(pScreen->devPrivate);
}

void
miSetScreenPixmap(pPix)
    PixmapPtr pPix;
{
    if (pPix)
	pPix->drawable.pScreen->devPrivate = (void *)pPix;
}
