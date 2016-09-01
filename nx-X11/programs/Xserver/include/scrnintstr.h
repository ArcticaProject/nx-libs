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

#ifndef SCREENINTSTRUCT_H
#define SCREENINTSTRUCT_H

#include "screenint.h"
#include "regionstr.h"
#include "bstore.h"
#include "colormap.h"
#include "cursor.h"
#include "validate.h"
#include <X11/Xproto.h>
#include "dix.h"

typedef struct _PixmapFormat {
    unsigned char	depth;
    unsigned char	bitsPerPixel;
    unsigned char	scanlinePad;
    } PixmapFormatRec;
    
typedef struct _Visual {
    VisualID		vid;
    short		class;
    short		bitsPerRGBValue;
    short		ColormapEntries;
    short		nplanes;/* = log2 (ColormapEntries). This does not
				 * imply that the screen has this many planes.
				 * it may have more or fewer */
    unsigned long	redMask, greenMask, blueMask;
    int			offsetRed, offsetGreen, offsetBlue;
  } VisualRec;

typedef struct _Depth {
    unsigned char	depth;
    short		numVids;
    VisualID		*vids;    /* block of visual ids for this depth */
  } DepthRec;


/*
 *  There is a typedef for each screen function void * so that code that
 *  needs to declare a screen function void * (e.g. in a screen private
 *  or as a local variable) can easily do so and retain full type checking.
 */

typedef    Bool (* CloseScreenProcPtr)(
	int /*index*/,
	ScreenPtr /*pScreen*/);

typedef    void (* QueryBestSizeProcPtr)(
	int /*class*/,
	unsigned short * /*pwidth*/,
	unsigned short * /*pheight*/,
	ScreenPtr /*pScreen*/);

typedef    Bool (* SaveScreenProcPtr)(
	 ScreenPtr /*pScreen*/,
	 int /*on*/);

typedef    void (* GetImageProcPtr)(
	DrawablePtr /*pDrawable*/,
	int /*sx*/,
	int /*sy*/,
	int /*w*/,
	int /*h*/,
	unsigned int /*format*/,
	unsigned long /*planeMask*/,
	char * /*pdstLine*/);

typedef    void (* GetSpansProcPtr)(
	DrawablePtr /*pDrawable*/,
	int /*wMax*/,
	DDXPointPtr /*ppt*/,
	int* /*pwidth*/,
	int /*nspans*/,
	char * /*pdstStart*/);

typedef    void (* PointerNonInterestBoxProcPtr)(
	ScreenPtr /*pScreen*/,
	BoxPtr /*pBox*/);

typedef    void (* SourceValidateProcPtr)(
	DrawablePtr /*pDrawable*/,
	int /*x*/,
	int /*y*/,
	int /*width*/,
	int /*height*/);

typedef    Bool (* CreateWindowProcPtr)(
	WindowPtr /*pWindow*/);

typedef    Bool (* DestroyWindowProcPtr)(
	WindowPtr /*pWindow*/);

typedef    Bool (* PositionWindowProcPtr)(
	WindowPtr /*pWindow*/,
	int /*x*/,
	int /*y*/);

typedef    Bool (* ChangeWindowAttributesProcPtr)(
	WindowPtr /*pWindow*/,
	unsigned long /*mask*/);

typedef    Bool (* RealizeWindowProcPtr)(
	WindowPtr /*pWindow*/);

typedef    Bool (* UnrealizeWindowProcPtr)(
	WindowPtr /*pWindow*/);

typedef    void (* RestackWindowProcPtr)(
	WindowPtr /*pWindow*/,
	WindowPtr /*pOldNextSib*/);

typedef    int  (* ValidateTreeProcPtr)(
	WindowPtr /*pParent*/,
	WindowPtr /*pChild*/,
	VTKind /*kind*/);

typedef    void (* PostValidateTreeProcPtr)(
	WindowPtr /*pParent*/,
	WindowPtr /*pChild*/,
	VTKind /*kind*/);

typedef    void (* WindowExposuresProcPtr)(
	WindowPtr /*pWindow*/,
	RegionPtr /*prgn*/,
	RegionPtr /*other_exposed*/);

typedef    void (* PaintWindowProcPtr)(
	WindowPtr /*pWindow*/,
	RegionPtr /*pRegion*/,
	int /*what*/);

typedef PaintWindowProcPtr PaintWindowBackgroundProcPtr;
typedef PaintWindowProcPtr PaintWindowBorderProcPtr;

typedef    void (* CopyWindowProcPtr)(
	WindowPtr /*pWindow*/,
	DDXPointRec /*ptOldOrg*/,
	RegionPtr /*prgnSrc*/);

typedef    void (* ClearToBackgroundProcPtr)(
	WindowPtr /*pWindow*/,
	int /*x*/,
	int /*y*/,
	int /*w*/,
	int /*h*/,
	Bool /*generateExposures*/);

typedef    void (* ClipNotifyProcPtr)(
	WindowPtr /*pWindow*/,
	int /*dx*/,
	int /*dy*/);

typedef    PixmapPtr (* CreatePixmapProcPtr)(
	ScreenPtr /*pScreen*/,
	int /*width*/,
	int /*height*/,
	int /*depth*/);

typedef    Bool (* DestroyPixmapProcPtr)(
	PixmapPtr /*pPixmap*/);

typedef    void (* SaveDoomedAreasProcPtr)(
	WindowPtr /*pWindow*/,
	RegionPtr /*prgnSave*/,
	int /*xorg*/,
	int /*yorg*/);

typedef    RegionPtr (* RestoreAreasProcPtr)(
	WindowPtr /*pWindow*/,
	RegionPtr /*prgnRestore*/);

typedef    void (* ExposeCopyProcPtr)(
	WindowPtr /*pSrc*/,
	DrawablePtr /*pDst*/,
	GCPtr /*pGC*/,
	RegionPtr /*prgnExposed*/,
	int /*srcx*/,
	int /*srcy*/,
	int /*dstx*/,
	int /*dsty*/,
	unsigned long /*plane*/);

typedef    RegionPtr (* TranslateBackingStoreProcPtr)(
	WindowPtr /*pWindow*/,
	int /*windx*/,
	int /*windy*/,
	RegionPtr /*oldClip*/,
	int /*oldx*/,
	int /*oldy*/);

typedef    RegionPtr (* ClearBackingStoreProcPtr)(
	WindowPtr /*pWindow*/,
	int /*x*/,
	int /*y*/,
	int /*w*/,
	int /*h*/,
	Bool /*generateExposures*/);

typedef    void (* DrawGuaranteeProcPtr)(
	WindowPtr /*pWindow*/,
	GCPtr /*pGC*/,
	int /*guarantee*/);
    
typedef    Bool (* RealizeFontProcPtr)(
	ScreenPtr /*pScreen*/,
	FontPtr /*pFont*/);

typedef    Bool (* UnrealizeFontProcPtr)(
	ScreenPtr /*pScreen*/,
	FontPtr /*pFont*/);

typedef    void (* ConstrainCursorProcPtr)(
	ScreenPtr /*pScreen*/,
	BoxPtr /*pBox*/);

typedef    void (* CursorLimitsProcPtr)(
	ScreenPtr /*pScreen*/,
	CursorPtr /*pCursor*/,
	BoxPtr /*pHotBox*/,
	BoxPtr /*pTopLeftBox*/);

typedef    Bool (* DisplayCursorProcPtr)(
	ScreenPtr /*pScreen*/,
	CursorPtr /*pCursor*/);

typedef    Bool (* RealizeCursorProcPtr)(
	ScreenPtr /*pScreen*/,
	CursorPtr /*pCursor*/);

typedef    Bool (* UnrealizeCursorProcPtr)(
	ScreenPtr /*pScreen*/,
	CursorPtr /*pCursor*/);

typedef    void (* RecolorCursorProcPtr)(
	ScreenPtr /*pScreen*/,
	CursorPtr /*pCursor*/,
	Bool /*displayed*/);

typedef    Bool (* SetCursorPositionProcPtr)(
	ScreenPtr /*pScreen*/,
	int /*x*/,
	int /*y*/,
	Bool /*generateEvent*/);

typedef    Bool (* CreateGCProcPtr)(
	GCPtr /*pGC*/);

typedef    Bool (* CreateColormapProcPtr)(
	ColormapPtr /*pColormap*/);

typedef    void (* DestroyColormapProcPtr)(
	ColormapPtr /*pColormap*/);

typedef    void (* InstallColormapProcPtr)(
	ColormapPtr /*pColormap*/);

typedef    void (* UninstallColormapProcPtr)(
	ColormapPtr /*pColormap*/);

typedef    int (* ListInstalledColormapsProcPtr) (
	ScreenPtr /*pScreen*/,
	XID* /*pmaps */);

typedef    void (* StoreColorsProcPtr)(
	ColormapPtr /*pColormap*/,
	int /*ndef*/,
	xColorItem * /*pdef*/);

typedef    void (* ResolveColorProcPtr)(
	unsigned short* /*pred*/,
	unsigned short* /*pgreen*/,
	unsigned short* /*pblue*/,
	VisualPtr /*pVisual*/);

#ifdef NEED_SCREEN_REGIONS

typedef    RegionPtr (* RegionCreateProcPtr)(
	BoxPtr /*rect*/,
	int /*size*/);

typedef    void (* RegionInitProcPtr)(
	RegionPtr /*pReg*/,
	BoxPtr /*rect*/,
	int /*size*/);

typedef    Bool (* RegionCopyProcPtr)(
	RegionPtr /*dst*/,
	RegionPtr /*src*/);

typedef    void (* RegionDestroyProcPtr)(
	RegionPtr /*pReg*/);

typedef    void (* RegionUninitProcPtr)(
	RegionPtr /*pReg*/);

typedef    Bool (* IntersectProcPtr)(
	RegionPtr /*newReg*/,
	RegionPtr /*reg1*/,
	RegionPtr /*reg2*/);

typedef    Bool (* UnionProcPtr)(
	RegionPtr /*newReg*/,
	RegionPtr /*reg1*/,
	RegionPtr /*reg2*/);

typedef    Bool (* SubtractProcPtr)(
	RegionPtr /*regD*/,
	RegionPtr /*regM*/,
	RegionPtr /*regS*/);

typedef    Bool (* InverseProcPtr)(
	RegionPtr /*newReg*/,
	RegionPtr /*reg1*/,
	BoxPtr /*invRect*/);

typedef    void (* RegionResetProcPtr)(
	RegionPtr /*pReg*/,
	BoxPtr /*pBox*/);

typedef    void (* TranslateRegionProcPtr)(
	RegionPtr /*pReg*/,
	int /*x*/,
	int /*y*/);

typedef    int (* RectInProcPtr)(
	RegionPtr /*region*/,
	BoxPtr /*prect*/);

typedef    Bool (* PointInRegionProcPtr)(
	RegionPtr /*pReg*/,
	int /*x*/,
	int /*y*/,
	BoxPtr /*box*/);

typedef    Bool (* RegionNotEmptyProcPtr)(
	RegionPtr /*pReg*/);

typedef    Bool (* RegionEqualProcPtr)(
	RegionPtr /*pReg1*/,
	RegionPtr /*pReg2*/);

typedef    Bool (* RegionBrokenProcPtr)(
	RegionPtr /*pReg*/);

typedef    Bool (* RegionBreakProcPtr)(
	RegionPtr /*pReg*/);

typedef    void (* RegionEmptyProcPtr)(
	RegionPtr /*pReg*/);

typedef    BoxPtr (* RegionExtentsProcPtr)(
	RegionPtr /*pReg*/);

typedef    Bool (* RegionAppendProcPtr)(
	RegionPtr /*dstrgn*/,
	RegionPtr /*rgn*/);

typedef    Bool (* RegionValidateProcPtr)(
	RegionPtr /*badreg*/,
	Bool* /*pOverlap*/);

#endif /* NEED_SCREEN_REGIONS */

typedef    RegionPtr (* BitmapToRegionProcPtr)(
	PixmapPtr /*pPix*/);

#ifdef NEED_SCREEN_REGIONS

typedef    RegionPtr (* RectsToRegionProcPtr)(
	int /*nrects*/,
	xRectangle* /*prect*/,
	int /*ctype*/);

#endif /* NEED_SCREEN_REGIONS */

typedef    void (* SendGraphicsExposeProcPtr)(
	ClientPtr /*client*/,
	RegionPtr /*pRgn*/,
	XID /*drawable*/,
	int /*major*/,
	int /*minor*/);

typedef    void (* ScreenBlockHandlerProcPtr)(
	int /*screenNum*/,
	void * /*blockData*/,
	void * /*pTimeout*/,
	void * /*pReadmask*/);

typedef    void (* ScreenWakeupHandlerProcPtr)(
	 int /*screenNum*/,
	 void * /*wakeupData*/,
	 unsigned long /*result*/,
	 void * /*pReadMask*/);

typedef    Bool (* CreateScreenResourcesProcPtr)(
	ScreenPtr /*pScreen*/);

typedef    Bool (* ModifyPixmapHeaderProcPtr)(
	PixmapPtr /*pPixmap*/,
	int /*width*/,
	int /*height*/,
	int /*depth*/,
	int /*bitsPerPixel*/,
	int /*devKind*/,
	void * /*pPixData*/);

typedef    PixmapPtr (* GetWindowPixmapProcPtr)(
	WindowPtr /*pWin*/);

typedef    void (* SetWindowPixmapProcPtr)(
	WindowPtr /*pWin*/,
	PixmapPtr /*pPix*/);

typedef    PixmapPtr (* GetScreenPixmapProcPtr)(
	ScreenPtr /*pScreen*/);

typedef    void (* SetScreenPixmapProcPtr)(
	PixmapPtr /*pPix*/);

typedef    void (* MarkWindowProcPtr)(
	WindowPtr /*pWin*/);

typedef    Bool (* MarkOverlappedWindowsProcPtr)(
	WindowPtr /*parent*/,
	WindowPtr /*firstChild*/,
	WindowPtr * /*pLayerWin*/);

typedef    Bool (* ChangeSaveUnderProcPtr)(
	WindowPtr /*pLayerWin*/,
	WindowPtr /*firstChild*/);

typedef    void (* PostChangeSaveUnderProcPtr)(
	WindowPtr /*pLayerWin*/,
	WindowPtr /*firstChild*/);

typedef    void (* MoveWindowProcPtr)(
	WindowPtr /*pWin*/,
	int /*x*/,
	int /*y*/,
	WindowPtr /*pSib*/,
	VTKind /*kind*/);

typedef    void (* ResizeWindowProcPtr)(
    WindowPtr /*pWin*/,
    int /*x*/,
    int /*y*/, 
    unsigned int /*w*/,
    unsigned int /*h*/,
    WindowPtr /*pSib*/
);

typedef    WindowPtr (* GetLayerWindowProcPtr)(
    WindowPtr /*pWin*/
);

typedef    void (* HandleExposuresProcPtr)(
    WindowPtr /*pWin*/);

typedef    void (* ReparentWindowProcPtr)(
    WindowPtr /*pWin*/,
    WindowPtr /*pPriorParent*/);

#ifdef SHAPE
typedef    void (* SetShapeProcPtr)(
	WindowPtr /*pWin*/);
#endif /* SHAPE */

typedef    void (* ChangeBorderWidthProcPtr)(
	WindowPtr /*pWin*/,
	unsigned int /*width*/);

typedef    void (* MarkUnrealizedWindowProcPtr)(
	WindowPtr /*pChild*/,
	WindowPtr /*pWin*/,
	Bool /*fromConfigure*/);

typedef    void (*ConstrainCursorHarderProcPtr)(
	ScreenPtr, /*pScreen*/
	int, /*mode*/
	int *, /*x*/
	int *  /*y*/);

typedef     Bool (*ReplaceScanoutPixmapProcPtr)(
	DrawablePtr, /*pDrawable*/
	PixmapPtr, /*pPixmap*/
	Bool /*enable*/);

typedef struct _Screen {
    int			myNum;	/* index of this instance in Screens[] */
    ATOM		id;
    short		width, height;
    short		mmWidth, mmHeight;
    short		numDepths;
    unsigned char      	rootDepth;
    DepthPtr       	allowedDepths;
    unsigned long      	rootVisual;
    unsigned long	defColormap;
    short		minInstalledCmaps, maxInstalledCmaps;
    char                backingStoreSupport, saveUnderSupport;
    unsigned long	whitePixel, blackPixel;
    unsigned long	rgf;	/* array of flags; she's -- HUNGARIAN */
    GCPtr		GCperDepth[MAXFORMATS+1];
			/* next field is a stipple to use as default in
			   a GC.  we don't build default tiles of all depths
			   because they are likely to be of a color
			   different from the default fg pixel, so
			   we don't win anything by building
			   a standard one.
			*/
    PixmapPtr		PixmapPerDepth[1];
    void *		devPrivate;
    short       	numVisuals;
    VisualPtr		visuals;
    WindowPtr		root;
    int			WindowPrivateLen;
    unsigned		*WindowPrivateSizes;
    unsigned		totalWindowSize;
    int			GCPrivateLen;
    unsigned		*GCPrivateSizes;
    unsigned		totalGCSize;

    /* Random screen procedures */

    CloseScreenProcPtr		CloseScreen;
    QueryBestSizeProcPtr	QueryBestSize;
    SaveScreenProcPtr		SaveScreen;
    GetImageProcPtr		GetImage;
    GetSpansProcPtr		GetSpans;
    PointerNonInterestBoxProcPtr PointerNonInterestBox;
    SourceValidateProcPtr	SourceValidate;

    /* Window Procedures */

    CreateWindowProcPtr		CreateWindow;
    DestroyWindowProcPtr	DestroyWindow;
    PositionWindowProcPtr	PositionWindow;
    ChangeWindowAttributesProcPtr ChangeWindowAttributes;
    RealizeWindowProcPtr	RealizeWindow;
    UnrealizeWindowProcPtr	UnrealizeWindow;
    ValidateTreeProcPtr		ValidateTree;
    PostValidateTreeProcPtr	PostValidateTree;
    WindowExposuresProcPtr	WindowExposures;
    PaintWindowBackgroundProcPtr PaintWindowBackground;
    PaintWindowBorderProcPtr	PaintWindowBorder;
    CopyWindowProcPtr		CopyWindow;
    ClearToBackgroundProcPtr	ClearToBackground;
    ClipNotifyProcPtr		ClipNotify;
    RestackWindowProcPtr	RestackWindow;

    /* Pixmap procedures */

    CreatePixmapProcPtr		CreatePixmap;
    DestroyPixmapProcPtr	DestroyPixmap;

    /* Backing store procedures */

    SaveDoomedAreasProcPtr	SaveDoomedAreas;
    RestoreAreasProcPtr		RestoreAreas;
    ExposeCopyProcPtr		ExposeCopy;
    TranslateBackingStoreProcPtr TranslateBackingStore;
    ClearBackingStoreProcPtr	ClearBackingStore;
    DrawGuaranteeProcPtr	DrawGuarantee;
    /*
     * A read/write copy of the lower level backing store vector is needed now
     * that the functions can be wrapped.
     */
    BSFuncRec			BackingStoreFuncs;
    
    /* Font procedures */

    RealizeFontProcPtr		RealizeFont;
    UnrealizeFontProcPtr	UnrealizeFont;

    /* Cursor Procedures */

    ConstrainCursorProcPtr	ConstrainCursor;
    ConstrainCursorHarderProcPtr	ConstrainCursorHarder;
    CursorLimitsProcPtr		CursorLimits;
    DisplayCursorProcPtr	DisplayCursor;
    RealizeCursorProcPtr	RealizeCursor;
    UnrealizeCursorProcPtr	UnrealizeCursor;
    RecolorCursorProcPtr	RecolorCursor;
    SetCursorPositionProcPtr	SetCursorPosition;

    /* GC procedures */

    CreateGCProcPtr		CreateGC;

    /* Colormap procedures */

    CreateColormapProcPtr	CreateColormap;
    DestroyColormapProcPtr	DestroyColormap;
    InstallColormapProcPtr	InstallColormap;
    UninstallColormapProcPtr	UninstallColormap;
    ListInstalledColormapsProcPtr ListInstalledColormaps;
    StoreColorsProcPtr		StoreColors;
    ResolveColorProcPtr		ResolveColor;

    /* Region procedures */

#ifdef NEED_SCREEN_REGIONS
    RegionCreateProcPtr		RegionCreate;
    RegionInitProcPtr		RegionInit;
    RegionCopyProcPtr		RegionCopy;
    RegionDestroyProcPtr	RegionDestroy;
    RegionUninitProcPtr		RegionUninit;
    IntersectProcPtr		Intersect;
    UnionProcPtr		Union;
    SubtractProcPtr		Subtract;
    InverseProcPtr		Inverse;
    RegionResetProcPtr		RegionReset;
    TranslateRegionProcPtr	TranslateRegion;
    RectInProcPtr		RectIn;
    PointInRegionProcPtr	PointInRegion;
    RegionNotEmptyProcPtr	RegionNotEmpty;
    RegionEqualProcPtr		RegionEqual;
    RegionBrokenProcPtr		RegionBroken;
    RegionBreakProcPtr		RegionBreak;
    RegionEmptyProcPtr		RegionEmpty;
    RegionExtentsProcPtr	RegionExtents;
    RegionAppendProcPtr		RegionAppend;
    RegionValidateProcPtr	RegionValidate;
#endif /* NEED_SCREEN_REGIONS */
    BitmapToRegionProcPtr	BitmapToRegion;
#ifdef NEED_SCREEN_REGIONS
    RectsToRegionProcPtr	RectsToRegion;
#endif /* NEED_SCREEN_REGIONS */
    SendGraphicsExposeProcPtr	SendGraphicsExpose;

    /* os layer procedures */

    ScreenBlockHandlerProcPtr	BlockHandler;
    ScreenWakeupHandlerProcPtr	WakeupHandler;

    void * blockData;
    void * wakeupData;

    /* anybody can get a piece of this array */
    DevUnion	*devPrivates;

    CreateScreenResourcesProcPtr CreateScreenResources;
    ModifyPixmapHeaderProcPtr	ModifyPixmapHeader;

    GetWindowPixmapProcPtr	GetWindowPixmap;
    SetWindowPixmapProcPtr	SetWindowPixmap;
    GetScreenPixmapProcPtr	GetScreenPixmap;
    SetScreenPixmapProcPtr	SetScreenPixmap;

    PixmapPtr pScratchPixmap;		/* scratch pixmap "pool" */

#ifdef PIXPRIV
    int			PixmapPrivateLen;
    unsigned int		*PixmapPrivateSizes;
    unsigned int		totalPixmapSize;
#endif

    MarkWindowProcPtr		MarkWindow;
    MarkOverlappedWindowsProcPtr MarkOverlappedWindows;
    ChangeSaveUnderProcPtr	ChangeSaveUnder;
    PostChangeSaveUnderProcPtr	PostChangeSaveUnder;
    MoveWindowProcPtr		MoveWindow;
    ResizeWindowProcPtr		ResizeWindow;
    GetLayerWindowProcPtr	GetLayerWindow;
    HandleExposuresProcPtr	HandleExposures;
    ReparentWindowProcPtr	ReparentWindow;

#ifdef SHAPE
    SetShapeProcPtr		SetShape;
#endif /* SHAPE */

    ChangeBorderWidthProcPtr	ChangeBorderWidth;
    MarkUnrealizedWindowProcPtr	MarkUnrealizedWindow;

    ReplaceScanoutPixmapProcPtr ReplaceScanoutPixmap;
} ScreenRec;

static inline RegionPtr BitmapToRegion(ScreenPtr _pScreen, PixmapPtr pPix) {
    return (*(_pScreen)->BitmapToRegion)(pPix); /* no mi version?! */
}

typedef struct _ScreenInfo {
    int		imageByteOrder;
    int		bitmapScanlineUnit;
    int		bitmapScanlinePad;
    int		bitmapBitOrder;
    int		numPixmapFormats;
    PixmapFormatRec
		formats[MAXFORMATS];
    int		arraySize;
    int		numScreens;
    ScreenPtr	screens[MAXSCREENS];
    int		numVideoScreens;
} ScreenInfo;

extern ScreenInfo screenInfo;

extern void InitOutput(
    ScreenInfo 	* /*pScreenInfo*/,
    int     	/*argc*/,
    char    	** /*argv*/);

#endif /* SCREENINTSTRUCT_H */
