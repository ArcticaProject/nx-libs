/*
 * $Id: compext.c,v 1.5 2005/07/03 07:37:34 daniels Exp $
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

#include "compint.h"
#include <X11/extensions/XI.h>
#include <X11/extensions/XIproto.h>
#include "protocol-versions.h"
#include "extinit.h"

static CARD8	CompositeReqCode;

#ifndef NXAGENT_SERVER
static DevPrivateKeyRec CompositeClientPrivateKeyRec;

#define CompositeClientPrivateKey (&CompositeClientPrivateKeyRec)
#else /* !defined(NXAGENT_SERVER) */
static int CompositeClientPrivIndex = -1;
#endif /* !defined(NXAGENT_SERVER) */

RESTYPE		CompositeClientWindowType;
RESTYPE		CompositeClientSubwindowsType;
RESTYPE CompositeClientOverlayType;

typedef struct _CompositeClient {
    int	    major_version;
    int	    minor_version;
} CompositeClientRec, *CompositeClientPtr;

#ifndef NXAGENT_SERVER
#define GetCompositeClient(pClient) ((CompositeClientPtr) \
    dixLookupPrivate(&(pClient)->devPrivates, CompositeClientPrivateKey))
#else /* !defined(NXAGENT_SERVER) */
#define GetCompositeClient(pClient) ((CompositeClientPtr) \
    (pClient)->devPrivates[CompositeClientPrivIndex].ptr)
#endif /* !edefined(NXAGENT_SERVER) */

static void
CompositeClientCallback (CallbackListPtr	*list,
			 void		*closure,
			 void		*data)
{
    NewClientInfoRec	*clientinfo = (NewClientInfoRec *) data;
    ClientPtr		pClient = clientinfo->client;
    CompositeClientPtr	pCompositeClient = GetCompositeClient (pClient);

    pCompositeClient->major_version = 0;
    pCompositeClient->minor_version = 0;
}

static void
CompositeResetProc (ExtensionEntry *extEntry)
{
}

static int
FreeCompositeClientWindow(void *value, XID ccwid)
{
    WindowPtr pWin = value;

    compFreeClientWindow(pWin, ccwid);
    return Success;
}
    
static int
FreeCompositeClientSubwindows(void *value, XID ccwid)
{
    WindowPtr	pWin = value;

    compFreeClientSubwindows(pWin, ccwid);
    return Success;
}

static int
FreeCompositeClientOverlay(void *value, XID ccwid)
{
    CompOverlayClientPtr pOc = (CompOverlayClientPtr) value;

    compFreeOverlayClient(pOc);
    return Success;
}

static int
ProcCompositeQueryVersion (ClientPtr client)
{
    CompositeClientPtr pCompositeClient = GetCompositeClient (client);
    xCompositeQueryVersionReply rep = {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0
    };

    REQUEST(xCompositeQueryVersionReq);

    REQUEST_SIZE_MATCH(xCompositeQueryVersionReq);
    if (stuff->majorVersion < SERVER_COMPOSITE_MAJOR_VERSION) {
	rep.majorVersion = stuff->majorVersion;
	rep.minorVersion = stuff->minorVersion;
    } else {
	rep.majorVersion = SERVER_COMPOSITE_MAJOR_VERSION;
	rep.minorVersion = SERVER_COMPOSITE_MINOR_VERSION;
    }
    pCompositeClient->major_version = rep.majorVersion;
    pCompositeClient->minor_version = rep.minorVersion;
    if (client->swapped) {
	swaps(&rep.sequenceNumber);
	swapl(&rep.length);
	swapl(&rep.majorVersion);
	swapl(&rep.minorVersion);
    }
    WriteToClient(client, sizeof(xCompositeQueryVersionReply), &rep);
    return Success;
}

/* Unsupported by current architecture and porting is too much effort. */
#if 0
#define VERIFY_WINDOW(pWindow, wid, client, mode)			\
    do {								\
	int err;							\
	err = dixLookupResourceByType((void **) &pWindow, wid,	\
				      RT_WINDOW, client, mode);		\
	if (err != Success) {						\
	    client->errorValue = wid;					\
	    return err;							\
	}								\
    } while (0)
#endif /* 0 */

static int
ProcCompositeRedirectWindow (ClientPtr client)
{
    WindowPtr	pWin;
    REQUEST(xCompositeRedirectWindowReq);

    REQUEST_SIZE_MATCH(xCompositeRedirectWindowReq);
    /* Unsupported by current architecture and porting is too much effort. */
    /*
    VERIFY_WINDOW(pWin, stuff->window, client,
                  DixSetAttrAccess | DixManageAccess | DixBlendAccess);
    */
    pWin = (WindowPtr) LookupIDByType (stuff->window, RT_WINDOW);
    if (!pWin)
    {
	client->errorValue = stuff->window;
	return BadWindow;
    }

    return compRedirectWindow (client, pWin, stuff->update);
}

static int
ProcCompositeRedirectSubwindows (ClientPtr client)
{
    WindowPtr	pWin;
    REQUEST(xCompositeRedirectSubwindowsReq);

    REQUEST_SIZE_MATCH(xCompositeRedirectSubwindowsReq);
    /* Unsupported by current architecture and porting is too much effort. */
    /*
    VERIFY_WINDOW(pWin, stuff->window, client,
                  DixSetAttrAccess | DixManageAccess | DixBlendAccess);
    */
    pWin = (WindowPtr) LookupIDByType (stuff->window, RT_WINDOW);
    if (!pWin)
    {
	client->errorValue = stuff->window;
	return BadWindow;
    }

    return compRedirectSubwindows (client, pWin, stuff->update);
}

static int
ProcCompositeUnredirectWindow (ClientPtr client)
{
    WindowPtr	pWin;
    REQUEST(xCompositeUnredirectWindowReq);

    REQUEST_SIZE_MATCH(xCompositeUnredirectWindowReq);
    /* Unsupported by current architecture and porting is too much effort. */
    /*
    VERIFY_WINDOW(pWin, stuff->window, client,
                  DixSetAttrAccess | DixManageAccess | DixBlendAccess);
    */
    pWin = (WindowPtr) LookupIDByType (stuff->window, RT_WINDOW);
    if (!pWin)
    {
	client->errorValue = stuff->window;
	return BadWindow;
    }

    return compUnredirectWindow (client, pWin, stuff->update);
}

static int
ProcCompositeUnredirectSubwindows (ClientPtr client)
{
    WindowPtr	pWin;
    REQUEST(xCompositeUnredirectSubwindowsReq);

    REQUEST_SIZE_MATCH(xCompositeUnredirectSubwindowsReq);
    /* Unsupported by current architecture and porting is too much effort. */
    /*
    VERIFY_WINDOW(pWin, stuff->window, client,
                  DixSetAttrAccess | DixManageAccess | DixBlendAccess);
    */
    pWin = (WindowPtr) LookupIDByType (stuff->window, RT_WINDOW);
    if (!pWin)
    {
	client->errorValue = stuff->window;
	return BadWindow;
    }

    return compUnredirectSubwindows (client, pWin, stuff->update);
}

static int
ProcCompositeCreateRegionFromBorderClip (ClientPtr client)
{
    WindowPtr	    pWin;
    CompWindowPtr   cw;
    RegionPtr	    pBorderClip, pRegion;
    REQUEST(xCompositeCreateRegionFromBorderClipReq);

    REQUEST_SIZE_MATCH(xCompositeCreateRegionFromBorderClipReq);
    /* Unsupported by current architecture and porting is too much effort. */
    /*
    VERIFY_WINDOW(pWin, stuff->window, client, DixGetAttrAccess);
    */
    pWin = (WindowPtr) LookupIDByType (stuff->window, RT_WINDOW);
    if (!pWin)
    {
	client->errorValue = stuff->window;
	return BadWindow;
    }
    LEGAL_NEW_RESOURCE (stuff->region, client);
    
    cw = GetCompWindow (pWin);
    if (cw)
	pBorderClip = &cw->borderClip;
    else
	pBorderClip = &pWin->borderClip;
    pRegion = XFixesRegionCopy (pBorderClip);
    if (!pRegion)
	return BadAlloc;
    RegionTranslate(pRegion, -pWin->drawable.x, -pWin->drawable.y);
    
    if (!AddResource (stuff->region, RegionResType, (void *) pRegion))
	return BadAlloc;

    return Success;
}

static int
ProcCompositeNameWindowPixmap (ClientPtr client)
{
    WindowPtr	    pWin;
    CompWindowPtr   cw;
    PixmapPtr	    pPixmap;
    ScreenPtr pScreen;
    int rc;

    REQUEST(xCompositeNameWindowPixmapReq);

    REQUEST_SIZE_MATCH(xCompositeNameWindowPixmapReq);
    /* Unsupported by current architecture and porting is too much effort. */
    /*
    VERIFY_WINDOW(pWin, stuff->window, client, DixGetAttrAccess);
    */
    pWin = (WindowPtr) LookupIDByType (stuff->window, RT_WINDOW);
    if (!pWin)
    {
	client->errorValue = stuff->window;
	return BadWindow;
    }

    pScreen = pWin->drawable.pScreen;

    if (!pWin->viewable)
        return BadMatch;

    LEGAL_NEW_RESOURCE (stuff->pixmap, client);
    
    cw = GetCompWindow (pWin);
    if (!cw)
	return BadMatch;

    pPixmap = (*pScreen->GetWindowPixmap) (pWin);
    if (!pPixmap)
	return BadMatch;

    /* security creation/labeling check */
    /*
    rc = XaceHook(XACE_RESOURCE_ACCESS, client, stuff->pixmap, RT_PIXMAP,
                  pPixmap, RT_WINDOW, pWin, DixCreateAccess);
    */
    rc = Success;
    if (rc != Success)
        return rc;

    ++pPixmap->refcnt;
    
    if (!AddResource (stuff->pixmap, RT_PIXMAP, (void *) pPixmap))
	return BadAlloc;

    /* Unsupported by current architecture. */
    /*
    if (pScreen->NameWindowPixmap) {
        rc = pScreen->NameWindowPixmap(pWin, pPixmap, stuff->pixmap);
        if (rc != Success) {
            FreeResource(stuff->pixmap, RT_NONE);
            return rc;
        }
    }
    */

    return Success;
}

static int
ProcCompositeGetOverlayWindow(ClientPtr client)
{
    REQUEST(xCompositeGetOverlayWindowReq);
    xCompositeGetOverlayWindowReply rep;
    WindowPtr pWin;
    ScreenPtr pScreen;
    CompScreenPtr cs;
    CompOverlayClientPtr pOc;
    int rc;

    REQUEST_SIZE_MATCH(xCompositeGetOverlayWindowReq);
    /* Unsupported by current architecture and porting is too much effort. */
    /*
    VERIFY_WINDOW(pWin, stuff->window, client, DixGetAttrAccess);
    */
    pWin = (WindowPtr) LookupIDByType (stuff->window, RT_WINDOW);
    if (!pWin)
    {
	client->errorValue = stuff->window;
	return BadWindow;
    }
    pScreen = pWin->drawable.pScreen;

    /*
     * Create an OverlayClient structure to mark this client's
     * interest in the overlay window
     */
    pOc = compCreateOverlayClient(pScreen, client);
    if (pOc == NULL)
        return BadAlloc;

    /*
     * Make sure the overlay window exists
     */
    cs = GetCompScreen(pScreen);
    if (cs->pOverlayWin == NULL)
        if (!compCreateOverlayWindow(pScreen)) {
            FreeResource(pOc->resource, RT_NONE);
            return BadAlloc;
        }

    /*
    rc = XaceHook(XACE_RESOURCE_ACCESS, client, cs->pOverlayWin->drawable.id,
                  RT_WINDOW, cs->pOverlayWin, RT_NONE, NULL, DixGetAttrAccess);
    */
    rc = Success;
    if (rc != Success) {
        FreeResource(pOc->resource, RT_NONE);
        return rc;
    }

    rep = (xCompositeGetOverlayWindowReply) {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0,
        .overlayWin = cs->pOverlayWin->drawable.id
    };

    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swapl(&rep.overlayWin);
    }
    WriteToClient(client, sz_xCompositeGetOverlayWindowReply, &rep);

    return Success;
}

static int
ProcCompositeReleaseOverlayWindow(ClientPtr client)
{
    REQUEST(xCompositeReleaseOverlayWindowReq);
    WindowPtr pWin;
    CompOverlayClientPtr pOc;

    REQUEST_SIZE_MATCH(xCompositeReleaseOverlayWindowReq);
    /* Unsupported by current architecture and porting is too much effort. */
    /*
    VERIFY_WINDOW(pWin, stuff->window, client, DixGetAttrAccess);
    */
    pWin = (WindowPtr) LookupIDByType (stuff->window, RT_WINDOW);
    if (!pWin)
    {
	client->errorValue = stuff->window;
	return BadWindow;
    }

    /*
     * Has client queried a reference to the overlay window
     * on this screen? If not, generate an error.
     */
    pOc = compFindOverlayClient(pWin->drawable.pScreen, client);
    if (pOc == NULL)
        return BadMatch;

    /* The delete function will free the client structure */
    FreeResource(pOc->resource, RT_NONE);

    return Success;
}

static int (*ProcCompositeVector[CompositeNumberRequests])(ClientPtr) = {
    ProcCompositeQueryVersion,
    ProcCompositeRedirectWindow,
    ProcCompositeRedirectSubwindows,
    ProcCompositeUnredirectWindow,
    ProcCompositeUnredirectSubwindows,
    ProcCompositeCreateRegionFromBorderClip,
    ProcCompositeNameWindowPixmap,
    ProcCompositeGetOverlayWindow, ProcCompositeReleaseOverlayWindow,
};

static int
ProcCompositeDispatch (ClientPtr client)
{
    REQUEST(xReq);
    
    if (stuff->data < CompositeNumberRequests)
	return (*ProcCompositeVector[stuff->data]) (client);
    else
	return BadRequest;
}

static int
SProcCompositeQueryVersion (ClientPtr client)
{
    REQUEST(xCompositeQueryVersionReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xCompositeQueryVersionReq);
    swapl(&stuff->majorVersion);
    swapl(&stuff->minorVersion);
    return (*ProcCompositeVector[stuff->compositeReqType]) (client);
}

static int
SProcCompositeRedirectWindow (ClientPtr client)
{
    REQUEST(xCompositeRedirectWindowReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xCompositeRedirectWindowReq);
    swapl (&stuff->window);
    return (*ProcCompositeVector[stuff->compositeReqType]) (client);
}

static int
SProcCompositeRedirectSubwindows (ClientPtr client)
{
    REQUEST(xCompositeRedirectSubwindowsReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xCompositeRedirectSubwindowsReq);
    swapl (&stuff->window);
    return (*ProcCompositeVector[stuff->compositeReqType]) (client);
}

static int
SProcCompositeUnredirectWindow (ClientPtr client)
{
    REQUEST(xCompositeUnredirectWindowReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xCompositeUnredirectWindowReq);
    swapl (&stuff->window);
    return (*ProcCompositeVector[stuff->compositeReqType]) (client);
}

static int
SProcCompositeUnredirectSubwindows (ClientPtr client)
{
    REQUEST(xCompositeUnredirectSubwindowsReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xCompositeUnredirectSubwindowsReq);
    swapl (&stuff->window);
    return (*ProcCompositeVector[stuff->compositeReqType]) (client);
}

static int
SProcCompositeCreateRegionFromBorderClip (ClientPtr client)
{
    REQUEST(xCompositeCreateRegionFromBorderClipReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xCompositeCreateRegionFromBorderClipReq);
    swapl (&stuff->region);
    swapl (&stuff->window);
    return (*ProcCompositeVector[stuff->compositeReqType]) (client);
}

static int
SProcCompositeNameWindowPixmap (ClientPtr client)
{
    REQUEST(xCompositeNameWindowPixmapReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xCompositeNameWindowPixmapReq);
    swapl (&stuff->window);
    swapl (&stuff->pixmap);
    return (*ProcCompositeVector[stuff->compositeReqType]) (client);
}

static int
SProcCompositeGetOverlayWindow(ClientPtr client)
{
    REQUEST(xCompositeGetOverlayWindowReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xCompositeGetOverlayWindowReq);
    swapl(&stuff->window);
    return (*ProcCompositeVector[stuff->compositeReqType]) (client);
}

static int
SProcCompositeReleaseOverlayWindow(ClientPtr client)
{
    REQUEST(xCompositeReleaseOverlayWindowReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xCompositeReleaseOverlayWindowReq);
    swapl(&stuff->window);
    return (*ProcCompositeVector[stuff->compositeReqType]) (client);
}

static int (*SProcCompositeVector[CompositeNumberRequests])(ClientPtr) = {
    SProcCompositeQueryVersion,
    SProcCompositeRedirectWindow,
    SProcCompositeRedirectSubwindows,
    SProcCompositeUnredirectWindow,
    SProcCompositeUnredirectSubwindows,
    SProcCompositeCreateRegionFromBorderClip,
    SProcCompositeNameWindowPixmap,
    SProcCompositeGetOverlayWindow, SProcCompositeReleaseOverlayWindow,
};

static int
SProcCompositeDispatch (ClientPtr client)
{
    REQUEST(xReq);
    
    if (stuff->data < CompositeNumberRequests)
	return (*SProcCompositeVector[stuff->data]) (client);
    else
	return BadRequest;
}

/* Both unused and incompatible due to architecture, disabled. */
#if 0
/** @see GetDefaultBytes */
static void
GetCompositeClientWindowBytes(void *value, XID id, ResourceSizePtr size)
{
    WindowPtr window = value;

    /* Currently only pixmap bytes are reported to clients. */
    size->resourceSize = 0;

    /* Calculate pixmap reference sizes. */
    size->pixmapRefSize = 0;
    if (window->redirectDraw != RedirectDrawNone)
    {
        SizeType pixmapSizeFunc = GetResourceTypeSizeFunc(RT_PIXMAP);
        ResourceSizeRec pixmapSize = { 0, 0 };
        ScreenPtr screen = window->drawable.pScreen;
        PixmapPtr pixmap = screen->GetWindowPixmap(window);
        pixmapSizeFunc(pixmap, pixmap->drawable.id, &pixmapSize);
        size->pixmapRefSize += pixmapSize.pixmapRefSize;
    }
}
#endif /* 0 */

void
CompositeExtensionInit (void)
{
    ExtensionEntry  *extEntry;
    int		    s;

    /* Assume initialization is going to fail */
    noCompositeExtension = TRUE;

    fprintf(stderr, "COMPOSITE: trying to initialize extension.\n");

    for (s = 0; s < screenInfo.numScreens; s++) {
        ScreenPtr pScreen = screenInfo.screens[s];
        VisualPtr vis;

        /* Composite on 8bpp pseudocolor root windows appears to fail, so
         * just disable it on anything pseudocolor for safety.
         */
        for (vis = pScreen->visuals; vis->vid != pScreen->rootVisual; vis++);
        if ((vis->class | DynamicClass) == PseudoColor)
            return;

        /* Ensure that Render is initialized, which is required for automatic
         * compositing.
         */
        if (GetPictureScreenIfSet(pScreen) == NULL)
            return;
    }
#ifdef PANORAMIX
    /* Xinerama's rewriting of window drawing before Composite gets to it
     * breaks Composite.
     */
    if (!noPanoramiXExtension)
	return;
#endif

    CompositeClientWindowType = CreateNewResourceType (FreeCompositeClientWindow);
    if (!CompositeClientWindowType)
	return;

    /* SetResourceTypeSizeFunc(CompositeClientWindowType,
                            GetCompositeClientWindowBytes);
     */

    CompositeClientSubwindowsType = CreateNewResourceType (FreeCompositeClientSubwindows);
    if (!CompositeClientSubwindowsType)
	return;

    CompositeClientOverlayType = CreateNewResourceType
        (FreeCompositeClientOverlay);
    if (!CompositeClientOverlayType)
        return;

#ifndef NXAGENT_SERVER
    if (!dixRegisterPrivateKey(&CompositeClientPrivateKeyRec, PRIVATE_CLIENT,
				sizeof (CompositeClientRec)))
	return;
#else /* !defined(NXAGENT_SERVER) */
    if ((CompositeClientPrivIndex = AllocateClientPrivateIndex()) < 0)
        return;
    if (!AllocateClientPrivate(CompositeClientPrivIndex, sizeof (CompositeClientRec)))
        return;
#endif /* !defined(NXAGENT_SERVER) */

    if (!AddCallback (&ClientStateCallback, CompositeClientCallback, 0))
	return;

    for (s = 0; s < screenInfo.numScreens; s++)
	if (!compScreenInit (screenInfo.screens[s])) {
            fprintf (stderr, "COMPOSITE: could not initialize via compScreenInit() for screen %d\n", s);
	    return;
        }

    extEntry = AddExtension (COMPOSITE_NAME, 0, 0,
			     ProcCompositeDispatch, SProcCompositeDispatch,
			     CompositeResetProc, StandardMinorOpcode);
    if (!extEntry)
	return;
    CompositeReqCode = (CARD8) extEntry->base;

    /* Initialization succeeded */
    noCompositeExtension = FALSE;
}

/*
 * This code requires features the current infrastructure does not provide
 * and will thus be disabled.
 */
#if 0

#ifdef PANORAMIX
#include "panoramiXsrv.h"

int (*PanoramiXSaveCompositeVector[CompositeNumberRequests]) (ClientPtr);

static int
PanoramiXCompositeRedirectWindow(ClientPtr client)
{
    PanoramiXRes *win;
    int rc = 0, j;

    REQUEST(xCompositeRedirectWindowReq);

    REQUEST_SIZE_MATCH(xCompositeRedirectWindowReq);

    if ((rc = dixLookupResourceByType((void **) &win, stuff->window, XRT_WINDOW,
                                      client, DixUnknownAccess))) {
        client->errorValue = stuff->window;
        return rc;
    }

    FOR_NSCREENS_FORWARD(j) {
        stuff->window = win->info[j].id;
        rc = (*PanoramiXSaveCompositeVector[stuff->compositeReqType]) (client);
        if (rc != Success)
            break;
    }

    return rc;
}

static int
PanoramiXCompositeRedirectSubwindows(ClientPtr client)
{
    PanoramiXRes *win;
    int rc = 0, j;

    REQUEST(xCompositeRedirectSubwindowsReq);

    REQUEST_SIZE_MATCH(xCompositeRedirectSubwindowsReq);

    if ((rc = dixLookupResourceByType((void **) &win, stuff->window, XRT_WINDOW,
                                      client, DixUnknownAccess))) {
        client->errorValue = stuff->window;
        return rc;
    }

    FOR_NSCREENS_FORWARD(j) {
        stuff->window = win->info[j].id;
        rc = (*PanoramiXSaveCompositeVector[stuff->compositeReqType]) (client);
        if (rc != Success)
            break;
    }

    return rc;
}

static int
PanoramiXCompositeUnredirectWindow(ClientPtr client)
{
    PanoramiXRes *win;
    int rc = 0, j;

    REQUEST(xCompositeUnredirectWindowReq);

    REQUEST_SIZE_MATCH(xCompositeUnredirectWindowReq);

    if ((rc = dixLookupResourceByType((void **) &win, stuff->window, XRT_WINDOW,
                                      client, DixUnknownAccess))) {
        client->errorValue = stuff->window;
        return rc;
    }

    FOR_NSCREENS_FORWARD(j) {
        stuff->window = win->info[j].id;
        rc = (*PanoramiXSaveCompositeVector[stuff->compositeReqType]) (client);
        if (rc != Success)
            break;
    }

    return rc;
}

static int
PanoramiXCompositeUnredirectSubwindows(ClientPtr client)
{
    PanoramiXRes *win;
    int rc = 0, j;

    REQUEST(xCompositeUnredirectSubwindowsReq);

    REQUEST_SIZE_MATCH(xCompositeUnredirectSubwindowsReq);

    if ((rc = dixLookupResourceByType((void **) &win, stuff->window, XRT_WINDOW,
                                      client, DixUnknownAccess))) {
        client->errorValue = stuff->window;
        return rc;
    }

    FOR_NSCREENS_FORWARD(j) {
        stuff->window = win->info[j].id;
        rc = (*PanoramiXSaveCompositeVector[stuff->compositeReqType]) (client);
        if (rc != Success)
            break;
    }

    return rc;
}

static int
PanoramiXCompositeNameWindowPixmap(ClientPtr client)
{
    WindowPtr pWin;
    CompWindowPtr cw;
    PixmapPtr pPixmap;
    int rc;
    PanoramiXRes *win, *newPix;
    int i;

    REQUEST(xCompositeNameWindowPixmapReq);

    REQUEST_SIZE_MATCH(xCompositeNameWindowPixmapReq);

    if ((rc = dixLookupResourceByType((void **) &win, stuff->window, XRT_WINDOW,
                                      client, DixUnknownAccess))) {
        client->errorValue = stuff->window;
        return rc;
    }

    LEGAL_NEW_RESOURCE(stuff->pixmap, client);

    if (!(newPix = malloc(sizeof(PanoramiXRes))))
        return BadAlloc;

    newPix->type = XRT_PIXMAP;
    newPix->u.pix.shared = FALSE;
    panoramix_setup_ids(newPix, client, stuff->pixmap);

    FOR_NSCREENS(i) {
        rc = dixLookupResourceByType((void **) &pWin, win->info[i].id,
                                     RT_WINDOW, client, DixGetAttrAccess);
        if (rc != Success) {
            client->errorValue = stuff->window;
            free(newPix);
            return rc;
        }

        if (!pWin->viewable) {
            free(newPix);
            return BadMatch;
        }

        cw = GetCompWindow(pWin);
        if (!cw) {
            free(newPix);
            return BadMatch;
        }

        pPixmap = (*pWin->drawable.pScreen->GetWindowPixmap) (pWin);
        if (!pPixmap) {
            free(newPix);
            return BadMatch;
        }

        if (!AddResource(newPix->info[i].id, RT_PIXMAP, (void *) pPixmap))
            return BadAlloc;

        ++pPixmap->refcnt;
    }

    if (!AddResource(stuff->pixmap, XRT_PIXMAP, (void *) newPix))
        return BadAlloc;

    return Success;
}

static int
PanoramiXCompositeGetOverlayWindow(ClientPtr client)
{
    REQUEST(xCompositeGetOverlayWindowReq);
    xCompositeGetOverlayWindowReply rep;
    WindowPtr pWin;
    ScreenPtr pScreen;
    CompScreenPtr cs;
    CompOverlayClientPtr pOc;
    int rc;
    PanoramiXRes *win, *overlayWin = NULL;
    int i;

    REQUEST_SIZE_MATCH(xCompositeGetOverlayWindowReq);

    if ((rc = dixLookupResourceByType((void **) &win, stuff->window, XRT_WINDOW,
                                      client, DixUnknownAccess))) {
        client->errorValue = stuff->window;
        return rc;
    }

    cs = GetCompScreen(screenInfo.screens[0]);
    if (!cs->pOverlayWin) {
        if (!(overlayWin = malloc(sizeof(PanoramiXRes))))
            return BadAlloc;

        overlayWin->type = XRT_WINDOW;
        overlayWin->u.win.root = FALSE;
    }

    FOR_NSCREENS_BACKWARD(i) {
        rc = dixLookupResourceByType((void **) &pWin, win->info[i].id,
                                     RT_WINDOW, client, DixGetAttrAccess);
        if (rc != Success) {
            client->errorValue = stuff->window;
            free(overlayWin);
            return rc;
        }
        pScreen = pWin->drawable.pScreen;

        /*
         * Create an OverlayClient structure to mark this client's
         * interest in the overlay window
         */
        pOc = compCreateOverlayClient(pScreen, client);
        if (pOc == NULL) {
            free(overlayWin);
            return BadAlloc;
        }

        /*
         * Make sure the overlay window exists
         */
        cs = GetCompScreen(pScreen);
        if (cs->pOverlayWin == NULL)
            if (!compCreateOverlayWindow(pScreen)) {
                FreeResource(pOc->resource, RT_NONE);
                free(overlayWin);
                return BadAlloc;
            }

        /*
        rc = XaceHook(XACE_RESOURCE_ACCESS, client,
                      cs->pOverlayWin->drawable.id,
                      RT_WINDOW, cs->pOverlayWin, RT_NONE, NULL,
                      DixGetAttrAccess);
        */
        rc = Success;
        if (rc != Success) {
            FreeResource(pOc->resource, RT_NONE);
            free(overlayWin);
            return rc;
        }
    }

    if (overlayWin) {
        FOR_NSCREENS(i) {
            cs = GetCompScreen(screenInfo.screens[i]);
            overlayWin->info[i].id = cs->pOverlayWin->drawable.id;
        }

        AddResource(overlayWin->info[0].id, XRT_WINDOW, overlayWin);
    }

    cs = GetCompScreen(screenInfo.screens[0]);

    rep = (xCompositeGetOverlayWindowReply) {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0,
        .overlayWin = cs->pOverlayWin->drawable.id
    };

    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swapl(&rep.overlayWin);
    }
    WriteToClient(client, sz_xCompositeGetOverlayWindowReply, &rep);

    return Success;
}

static int
PanoramiXCompositeReleaseOverlayWindow(ClientPtr client)
{
    REQUEST(xCompositeReleaseOverlayWindowReq);
    WindowPtr pWin;
    CompOverlayClientPtr pOc;
    PanoramiXRes *win;
    int i, rc;

    REQUEST_SIZE_MATCH(xCompositeReleaseOverlayWindowReq);

    if ((rc = dixLookupResourceByType((void **) &win, stuff->window, XRT_WINDOW,
                                      client, DixUnknownAccess))) {
        client->errorValue = stuff->window;
        return rc;
    }

    FOR_NSCREENS_BACKWARD(i) {
        if ((rc = dixLookupResourceByType((void **) &pWin, win->info[i].id,
                                          XRT_WINDOW, client,
                                          DixUnknownAccess))) {
            client->errorValue = stuff->window;
            return rc;
        }

        /*
         * Has client queried a reference to the overlay window
         * on this screen? If not, generate an error.
         */
        pOc = compFindOverlayClient(pWin->drawable.pScreen, client);
        if (pOc == NULL)
            return BadMatch;

        /* The delete function will free the client structure */
        FreeResource(pOc->resource, RT_NONE);
    }

    return Success;
}

void
PanoramiXCompositeInit(void)
{
    int i;

    for (i = 0; i < CompositeNumberRequests; i++)
        PanoramiXSaveCompositeVector[i] = ProcCompositeVector[i];
    /*
     * Stuff in Xinerama aware request processing hooks
     */
    ProcCompositeVector[X_CompositeRedirectWindow] =
        PanoramiXCompositeRedirectWindow;
    ProcCompositeVector[X_CompositeRedirectSubwindows] =
        PanoramiXCompositeRedirectSubwindows;
    ProcCompositeVector[X_CompositeUnredirectWindow] =
        PanoramiXCompositeUnredirectWindow;
    ProcCompositeVector[X_CompositeUnredirectSubwindows] =
        PanoramiXCompositeUnredirectSubwindows;
    ProcCompositeVector[X_CompositeNameWindowPixmap] =
        PanoramiXCompositeNameWindowPixmap;
    ProcCompositeVector[X_CompositeGetOverlayWindow] =
        PanoramiXCompositeGetOverlayWindow;
    ProcCompositeVector[X_CompositeReleaseOverlayWindow] =
        PanoramiXCompositeReleaseOverlayWindow;
}

void
PanoramiXCompositeReset(void)
{
    int i;

    for (i = 0; i < CompositeNumberRequests; i++)
        ProcCompositeVector[i] = PanoramiXSaveCompositeVector[i];
}

#endif

#endif /* 0 */
