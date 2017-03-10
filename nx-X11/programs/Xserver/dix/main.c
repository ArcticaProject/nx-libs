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

/* The panoramix components contained the following notice */
/*****************************************************************

Copyright (c) 1991, 1997 Digital Equipment Corporation, Maynard, Massachusetts.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
DIGITAL EQUIPMENT CORPORATION BE LIABLE FOR ANY CLAIM, DAMAGES, INCLUDING,
BUT NOT LIMITED TO CONSEQUENTIAL OR INCIDENTAL DAMAGES, OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Digital Equipment Corporation
shall not be used in advertising or otherwise to promote the sale, use or other
dealings in this Software without prior written authorization from Digital
Equipment Corporation.

******************************************************************/

/* $TOG: main.c /main/86 1998/02/09 14:20:03 kaleb $ */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <nx-X11/X.h>
#include <nx-X11/Xos.h>   /* for unistd.h  */
#include <nx-X11/Xproto.h>
#include "scrnintstr.h"
#include "misc.h"
#include "os.h"
#include "windowstr.h"
#include "resource.h"
#include "dixstruct.h"
#include "gcstruct.h"
#include "extension.h"
#include "colormap.h"
#include "colormapst.h"
#include "cursorstr.h"
#include <X11/fonts/font.h>
#include "opaque.h"
#include "servermd.h"
#include "site.h"
#include "dixfont.h"
#include "extnsionst.h"
#include "client.h"
#ifdef PANORAMIX
#include "panoramiXsrv.h"
#else
#include "dixevents.h"		/* InitEvents() */
#include "dispatch.h"		/* InitProcVectors() */
#endif

#ifdef DPMSExtension
#define DPMS_SERVER
#include <nx-X11/extensions/dpms.h>
#include "dpmsproc.h"
#endif

extern int InitClientPrivates(ClientPtr client);

extern void Dispatch(void);

xConnSetupPrefix connSetupPrefix;

extern FontPtr defaultFont;

extern void InitProcVectors(void);
extern Bool CreateGCperDepthArray(void);

extern void FreeScreen(ScreenPtr pScreen);

#ifndef PANORAMIX
static
#endif
Bool CreateConnectionBlock(void);

PaddingInfo PixmapWidthPaddingInfo[33];

int connBlockScreenStart;

static int restart = 0;

void
NotImplemented(xEvent *from, xEvent *to)
{
    FatalError("Not implemented");
}

/*
 * Dummy entry for ReplySwapVector[]
 */

void
ReplyNotSwappd(
	ClientPtr pClient ,
	int size ,
	void * pbuf
	)
{
    FatalError("Not implemented");
}

int
main(int argc, char *argv[], char *envp[])
{
    int		i, j, k, error;
    char	*xauthfile;
    HWEventQueueType	alwaysCheckForInput[2];

    display = "0";

    InitGlobals();

    /* Quartz support on Mac OS X requires that the Cocoa event loop be in
     * the main thread. This allows the X server main to be called again
     * from another thread. */
#if defined(__DARWIN__) && defined(DARWIN_WITH_QUARTZ)
    DarwinHandleGUI(argc, argv, envp);
#endif

    /* Notice if we're restarted.  Probably this is because we jumped through
     * an uninitialized pointer */
    if (restart)
	FatalError("server restarted. Jumped through uninitialized pointer?\n");
    else
	restart = 1;

    CheckUserParameters(argc, argv, envp);

    CheckUserAuthorization();

#ifdef COMMANDLINE_CHALLENGED_OPERATING_SYSTEMS
    ExpandCommandLine(&argc, &argv);
#endif

    InitConnectionLimits();

    /* These are needed by some routines which are called from interrupt
     * handlers, thus have no direct calling path back to main and thus
     * can't be passed argc, argv as parameters */
    argcGlobal = argc;
    argvGlobal = argv;
    /* prep X authority file from environment; this can be overriden by a
     * command line option */
    xauthfile = getenv("XAUTHORITY");
    if (xauthfile) InitAuthorization (xauthfile);
    ProcessCommandLine(argc, argv);

    alwaysCheckForInput[0] = 0;
    alwaysCheckForInput[1] = 1;
    while(1)
    {
	serverGeneration++;
	ScreenSaverTime = defaultScreenSaverTime;
	ScreenSaverInterval = defaultScreenSaverInterval;
	ScreenSaverBlanking = defaultScreenSaverBlanking;
	ScreenSaverAllowExposures = defaultScreenSaverAllowExposures;
#ifdef DPMSExtension
	DPMSStandbyTime = defaultDPMSStandbyTime;
	DPMSSuspendTime = defaultDPMSSuspendTime;
	DPMSOffTime = defaultDPMSOffTime;
	DPMSEnabled = defaultDPMSEnabled;
	DPMSPowerLevel = 0;
#endif
	InitBlockAndWakeupHandlers();
	/* Perform any operating system dependent initializations you'd like */
	OsInit();		
	if(serverGeneration == 1)
	{
	    CreateWellKnownSockets();
	    InitProcVectors();
	    clients = (ClientPtr *)malloc(MAXCLIENTS * sizeof(ClientPtr));
	    if (!clients)
		FatalError("couldn't create client array");
	    for (i=1; i<MAXCLIENTS; i++) 
		clients[i] = NullClient;
	    serverClient = (ClientPtr)malloc(sizeof(ClientRec));
	    if (!serverClient)
		FatalError("couldn't create server client");
	    InitClient(serverClient, 0, (void *)NULL);
	}
	else
	    ResetWellKnownSockets ();
	clients[0] = serverClient;
	currentMaxClients = 1;

	if (!InitClientResources(serverClient))      /* for root resources */
	    FatalError("couldn't init server resources");

	SetInputCheck(&alwaysCheckForInput[0], &alwaysCheckForInput[1]);
	screenInfo.arraySize = MAXSCREENS;
	screenInfo.numScreens = 0;
	screenInfo.numVideoScreens = -1;

	/*
	 * Just in case the ddx doesnt supply a format for depth 1 (like qvss).
	 */
	j = indexForBitsPerPixel[ 1 ];
	k = indexForScanlinePad[ BITMAP_SCANLINE_PAD ];
	PixmapWidthPaddingInfo[1].padRoundUp = BITMAP_SCANLINE_PAD-1;
	PixmapWidthPaddingInfo[1].padPixelsLog2 = answer[j][k];
 	j = indexForBitsPerPixel[8]; /* bits per byte */
 	PixmapWidthPaddingInfo[1].padBytesLog2 = answer[j][k];
	PixmapWidthPaddingInfo[1].bitsPerPixel = 1;

	InitAtoms();
	InitEvents();
	InitGlyphCaching();
	ResetClientPrivates();
	ResetScreenPrivates();
	ResetWindowPrivates();
	ResetGCPrivates();
#ifdef PIXPRIV
	ResetPixmapPrivates();
#endif
	ResetColormapPrivates();
	ResetFontPrivateIndex();
	ResetDevicePrivateIndex();
	InitCallbackManager();
	InitVisualWrap();
	InitOutput(&screenInfo, argc, argv);

	if (screenInfo.numScreens < 1)
	    FatalError("no screens found");
	if (screenInfo.numVideoScreens < 0)
	    screenInfo.numVideoScreens = screenInfo.numScreens;
	InitExtensions(argc, argv);
	if (!InitClientPrivates(serverClient))
	    FatalError("failed to allocate serverClient devprivates");
	for (i = 0; i < screenInfo.numScreens; i++)
	{
	    ScreenPtr pScreen = screenInfo.screens[i];
	    if (!CreateScratchPixmapsForScreen(i))
		FatalError("failed to create scratch pixmaps");
	    if (pScreen->CreateScreenResources &&
		!(*pScreen->CreateScreenResources)(pScreen))
		FatalError("failed to create screen resources");
	    if (!CreateGCperDepth(i))
		FatalError("failed to create scratch GCs");
	    if (!CreateDefaultStipple(i))
		FatalError("failed to create default stipple");
	    if (!CreateRootWindow(pScreen))
		FatalError("failed to create root window");
	}
	InitInput(argc, argv);
	if (InitAndStartDevices() != Success)
	    FatalError("failed to initialize core devices");
	ReserveClientIds(serverClient);

	InitFonts();
	if (loadableFonts) {
	    SetFontPath(0, 0, (unsigned char *)defaultFontPath, &error);
	} else {
	    if (SetDefaultFontPath(defaultFontPath) != Success)
		ErrorF("failed to set default font path '%s'\n",
			defaultFontPath);
	}
	if (!SetDefaultFont(defaultTextFont))
	    FatalError("could not open default font '%s'", defaultTextFont);
	if (!(rootCursor = CreateRootCursor(defaultCursorFont, 0)))
	    FatalError("could not open default cursor font '%s'",
		       defaultCursorFont);
#ifdef DPMSExtension
 	/* check all screens, looking for DPMS Capabilities */
 	DPMSCapableFlag = DPMSSupported();
	if (!DPMSCapableFlag)
     	    DPMSEnabled = FALSE;
#endif

#ifdef PANORAMIX
	/*
	 * Consolidate window and colourmap information for each screen
	 */
	if (!noPanoramiXExtension)
	    PanoramiXConsolidate();
#endif

	for (i = 0; i < screenInfo.numScreens; i++)
	    InitRootWindow(screenInfo.screens[i]->root);
	DefineInitialRootWindow(screenInfo.screens[0]->root);
	SaveScreens(SCREEN_SAVER_FORCER, ScreenSaverReset);
#ifdef DPMSExtension
	SetDPMSTimers();
#endif

#ifdef PANORAMIX
	if (!noPanoramiXExtension) {
	    if (!PanoramiXCreateConnectionBlock())
		FatalError("could not create connection block info");
	} else
#endif
	{
	    if (!CreateConnectionBlock())
	    	FatalError("could not create connection block info");
	}

	NotifyParentProcess();

	Dispatch();

	/* Now free up whatever must be freed */
	if (screenIsSaved == SCREEN_SAVER_ON)
	    SaveScreens(SCREEN_SAVER_OFF, ScreenSaverReset);
	FreeScreenSaverTimer();
	CloseDownExtensions();

#ifdef PANORAMIX
	{
	    Bool remember_it = noPanoramiXExtension;
	    noPanoramiXExtension = TRUE;
	    FreeAllResources();
	    noPanoramiXExtension = remember_it;
	}
#else
	FreeAllResources();
#endif

	for (i = 0; i < screenInfo.numScreens; i++)
	   screenInfo.screens[i]->root = NullWindow;
	CloseDownDevices();
	CloseDownEvents();

	for (i = screenInfo.numScreens - 1; i >= 0; i--)
	{
	    FreeScratchPixmapsForScreen(i);
	    FreeGCperDepth(i);
	    FreeDefaultStipple(i);
	    (* screenInfo.screens[i]->CloseScreen)(i, screenInfo.screens[i]);
	    FreeScreen(screenInfo.screens[i]);
	    screenInfo.numScreens = i;
	}
	FreeFonts();

#ifdef DPMSExtension
	FreeDPMSTimers();
#endif
	FreeAuditTimer();

	ReleaseClientIds(serverClient);
	free(serverClient->devPrivates);
	serverClient->devPrivates = NULL;

	if (dispatchException & DE_TERMINATE)
	{
	    CloseWellKnownConnections();
	}

	OsCleanup((dispatchException & DE_TERMINATE) != 0);

	if (dispatchException & DE_TERMINATE)
	{
	    ddxGiveUp();
	    break;
	}

	free(ConnectionInfo);
	ConnectionInfo = NULL;
    }
    return(0);
}

static int  VendorRelease = VENDOR_RELEASE;
static char *VendorString = VENDOR_STRING;

void
SetVendorRelease(int release)
{
    VendorRelease = release;
}

void
SetVendorString(char *string)
{
    VendorString = string;
}

static int padlength[4] = {0, 3, 2, 1};

#ifndef PANORAMIX
static
#endif
Bool
CreateConnectionBlock()
{
    xConnSetup setup;
    xWindowRoot root;
    xDepth	depth;
    xVisualType visual;
    xPixmapFormat format;
    unsigned long vid;
    int i, j, k,
        lenofblock,
        sizesofar = 0;
    char *pBuf;

    
    memset(&setup, 0, sizeof(xConnSetup));
    /* Leave off the ridBase and ridMask, these must be sent with 
       connection */

    setup.release = VendorRelease;
    /*
     * per-server image and bitmap parameters are defined in Xmd.h
     */
    setup.imageByteOrder = screenInfo.imageByteOrder;

    setup.bitmapScanlineUnit = screenInfo.bitmapScanlineUnit;
    setup.bitmapScanlinePad = screenInfo.bitmapScanlinePad;

    setup.bitmapBitOrder = screenInfo.bitmapBitOrder;
    setup.motionBufferSize = NumMotionEvents();
    setup.numRoots = screenInfo.numScreens;
    setup.nbytesVendor = strlen(VendorString); 
    setup.numFormats = screenInfo.numPixmapFormats;
    setup.maxRequestSize = MAX_REQUEST_SIZE;
    QueryMinMaxKeyCodes(&setup.minKeyCode, &setup.maxKeyCode);
    
    lenofblock = sizeof(xConnSetup) + 
            ((setup.nbytesVendor + 3) & ~3) +
	    (setup.numFormats * sizeof(xPixmapFormat)) +
            (setup.numRoots * sizeof(xWindowRoot));
    ConnectionInfo = (char *) malloc(lenofblock);
    if (!ConnectionInfo)
	return FALSE;

    memmove(ConnectionInfo, (char *)&setup, sizeof(xConnSetup));
    sizesofar = sizeof(xConnSetup);
    pBuf = ConnectionInfo + sizeof(xConnSetup);

    memmove(pBuf, VendorString, (int)setup.nbytesVendor);
    sizesofar += setup.nbytesVendor;
    pBuf += setup.nbytesVendor;
    i = padlength[setup.nbytesVendor & 3];
    sizesofar += i;
    while (--i >= 0)
	*pBuf++ = 0;
    
    memset(&format, 0, sizeof(xPixmapFormat));
    for (i=0; i<screenInfo.numPixmapFormats; i++)
    {
	format.depth = screenInfo.formats[i].depth;
	format.bitsPerPixel = screenInfo.formats[i].bitsPerPixel;
	format.scanLinePad = screenInfo.formats[i].scanlinePad;
	memmove(pBuf, (char *)&format, sizeof(xPixmapFormat));
	pBuf += sizeof(xPixmapFormat);
	sizesofar += sizeof(xPixmapFormat);
    }

    connBlockScreenStart = sizesofar;
    memset(&depth, 0, sizeof(xDepth));
    memset(&visual, 0, sizeof(xVisualType));
    for (i=0; i<screenInfo.numScreens; i++) 
    {
	ScreenPtr	pScreen;
	DepthPtr	pDepth;
	VisualPtr	pVisual;

	pScreen = screenInfo.screens[i];
	root.windowId = screenInfo.screens[i]->root->drawable.id;
	root.defaultColormap = pScreen->defColormap;
	root.whitePixel = pScreen->whitePixel;
	root.blackPixel = pScreen->blackPixel;
	root.currentInputMask = 0;    /* filled in when sent */
	root.pixWidth = pScreen->width;
	root.pixHeight = pScreen->height;
	root.mmWidth = pScreen->mmWidth;
	root.mmHeight = pScreen->mmHeight;
	root.minInstalledMaps = pScreen->minInstalledCmaps;
	root.maxInstalledMaps = pScreen->maxInstalledCmaps; 
	root.rootVisualID = pScreen->rootVisual;		
	root.backingStore = pScreen->backingStoreSupport;
	root.saveUnders = pScreen->saveUnderSupport != NotUseful;
	root.rootDepth = pScreen->rootDepth;
	root.nDepths = pScreen->numDepths;
	memmove(pBuf, (char *)&root, sizeof(xWindowRoot));
	sizesofar += sizeof(xWindowRoot);
	pBuf += sizeof(xWindowRoot);

	pDepth = pScreen->allowedDepths;
	for(j = 0; j < pScreen->numDepths; j++, pDepth++)
	{
	    lenofblock += sizeof(xDepth) + 
		    (pDepth->numVids * sizeof(xVisualType));
	    pBuf = (char *)realloc(ConnectionInfo, lenofblock);
	    if (!pBuf)
	    {
		free(ConnectionInfo);
		return FALSE;
	    }
	    ConnectionInfo = pBuf;
	    pBuf += sizesofar;            
	    depth.depth = pDepth->depth;
	    depth.nVisuals = pDepth->numVids;
	    memmove(pBuf, (char *)&depth, sizeof(xDepth));
	    pBuf += sizeof(xDepth);
	    sizesofar += sizeof(xDepth);
	    for(k = 0; k < pDepth->numVids; k++)
	    {
		vid = pDepth->vids[k];
		for (pVisual = pScreen->visuals;
		     pVisual->vid != vid;
		     pVisual++)
		    ;
		visual.visualID = vid;
		visual.class = pVisual->class;
		visual.bitsPerRGB = pVisual->bitsPerRGBValue;
		visual.colormapEntries = pVisual->ColormapEntries;
		visual.redMask = pVisual->redMask;
		visual.greenMask = pVisual->greenMask;
		visual.blueMask = pVisual->blueMask;
		memmove(pBuf, (char *)&visual, sizeof(xVisualType));
		pBuf += sizeof(xVisualType);
		sizesofar += sizeof(xVisualType);
	    }
	}
    }
    connSetupPrefix.success = xTrue;
    connSetupPrefix.length = lenofblock/4;
    connSetupPrefix.majorVersion = X_PROTOCOL;
    connSetupPrefix.minorVersion = X_PROTOCOL_REVISION;
    return TRUE;
}
