/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2011 NoMachine (http://www.nomachine.com)          */
/* Copyright (c) 2008-2017 Oleksandr Shneyder <o.shneyder@phoca-gmbh.de>  */
/* Copyright (c) 2011-2022 Mike Gabriel <mike.gabriel@das-netzwerkteam.de>*/
/* Copyright (c) 2014-2019 Mihai Moldovan <ionic@ionic.de>                */
/* Copyright (c) 2014-2022 Ulrich Sibiller <uli42@gmx.de>                 */
/* Copyright (c) 2015-2016 Qindel Group (http://www.qindel.com)           */
/*                                                                        */
/* nx-X11, NX protocol compression and NX extensions to this software     */
/* are copyright of the aforementioned persons and companies.             */
/*                                                                        */
/* Redistribution and use of the present software is allowed according    */
/* to terms specified in the file LICENSE which comes in the source       */
/* distribution.                                                          */
/*                                                                        */
/* All rights reserved.                                                   */
/*                                                                        */
/* NOTE: This software has received contributions from various other      */
/* contributors, only the core maintainers and supporters are listed as   */
/* copyright holders. Please contact us, if you feel you should be listed */
/* as copyright holder, as well.                                          */
/*                                                                        */
/**************************************************************************/

/*
 * Copyright © 2000 Compaq Computer Corporation
 * Copyright © 2002 Hewlett-Packard Company
 * Copyright © 2006 Intel Corporation
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 *
 * Author:  Jim Gettys, Hewlett-Packard Company, Inc.
 *	    Keith Packard, Intel Corporation
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "randrstr.h"

#ifndef NXAGENT_SERVER
#include "extinit.h"
#endif

/* From render.h */
#ifndef SubPixelUnknown
#define SubPixelUnknown 0
#endif

#define RR_VALIDATE
static int RRNScreens;

#define wrap(priv,real,mem,func) {\
    priv->mem = real->mem; \
    real->mem = func; \
}

#define unwrap(priv,real,mem) {\
    real->mem = priv->mem; \
}

static int ProcRRDispatch(ClientPtr pClient);
static int SProcRRDispatch(ClientPtr pClient);

int RREventBase;
int RRErrorBase;
RESTYPE RRClientType, RREventType;      /* resource types for event masks */

#ifndef NXAGENT_SERVER
DevPrivateKey RRClientPrivateKey = &RRClientPrivateKey;
DevPrivateKey rrPrivKey = &rrPrivKey;
#else
int RRClientPrivateIndex;
int rrPrivIndex = -1;
#endif

static void
RRClientCallback(CallbackListPtr *list, void *closure, void *data)
{
    NewClientInfoRec *clientinfo = (NewClientInfoRec *) data;
    ClientPtr pClient = clientinfo->client;

    rrClientPriv(pClient);
    RRTimesPtr pTimes = (RRTimesPtr) (pRRClient + 1);
    int i;

    pRRClient->major_version = 0;
    pRRClient->minor_version = 0;
    for (i = 0; i < screenInfo.numScreens; i++) {
        ScreenPtr pScreen = screenInfo.screens[i];

        rrScrPriv(pScreen);

        if (pScrPriv) {
            pTimes[i].setTime = pScrPriv->lastSetTime;
            pTimes[i].configTime = pScrPriv->lastConfigTime;
        }
    }
}

static Bool
RRCloseScreen(
              ScreenPtr pScreen)
{
    rrScrPriv(pScreen);
    int j;

    unwrap(pScrPriv, pScreen, CloseScreen);
    for (j = pScrPriv->numCrtcs - 1; j >= 0; j--)
        RRCrtcDestroy(pScrPriv->crtcs[j]);
    for (j = pScrPriv->numOutputs - 1; j >= 0; j--)
        RROutputDestroy(pScrPriv->outputs[j]);

    if (pScrPriv->provider)
        RRProviderDestroy(pScrPriv->provider);

    RRMonitorClose(pScreen);

    free(pScrPriv->crtcs);
    free(pScrPriv->outputs);
    free(pScrPriv);
    RRNScreens -= 1;            /* ok, one fewer screen with RandR running */
    return (*pScreen->CloseScreen) (pScreen);
}

static void
SRRScreenChangeNotifyEvent(xRRScreenChangeNotifyEvent * from,
                           xRRScreenChangeNotifyEvent * to)
{
    to->type = from->type;
    to->rotation = from->rotation;
    cpswaps(from->sequenceNumber, to->sequenceNumber);
    cpswapl(from->timestamp, to->timestamp);
    cpswapl(from->configTimestamp, to->configTimestamp);
    cpswapl(from->root, to->root);
    cpswapl(from->window, to->window);
    cpswaps(from->sizeID, to->sizeID);
    cpswaps(from->subpixelOrder, to->subpixelOrder);
    cpswaps(from->widthInPixels, to->widthInPixels);
    cpswaps(from->heightInPixels, to->heightInPixels);
    cpswaps(from->widthInMillimeters, to->widthInMillimeters);
    cpswaps(from->heightInMillimeters, to->heightInMillimeters);
}

static void
SRRCrtcChangeNotifyEvent(xRRCrtcChangeNotifyEvent * from,
                         xRRCrtcChangeNotifyEvent * to)
{
    to->type = from->type;
    to->subCode = from->subCode;
    cpswaps(from->sequenceNumber, to->sequenceNumber);
    cpswapl(from->timestamp, to->timestamp);
    cpswapl(from->window, to->window);
    cpswapl(from->crtc, to->crtc);
    cpswapl(from->mode, to->mode);
    cpswaps(from->rotation, to->rotation);
    /* pad1 */
    cpswaps(from->x, to->x);
    cpswaps(from->y, to->y);
    cpswaps(from->width, to->width);
    cpswaps(from->height, to->height);
}

static void
SRROutputChangeNotifyEvent(xRROutputChangeNotifyEvent * from,
                           xRROutputChangeNotifyEvent * to)
{
    to->type = from->type;
    to->subCode = from->subCode;
    cpswaps(from->sequenceNumber, to->sequenceNumber);
    cpswapl(from->timestamp, to->timestamp);
    cpswapl(from->configTimestamp, to->configTimestamp);
    cpswapl(from->window, to->window);
    cpswapl(from->output, to->output);
    cpswapl(from->crtc, to->crtc);
    cpswapl(from->mode, to->mode);
    cpswaps(from->rotation, to->rotation);
    to->connection = from->connection;
    to->subpixelOrder = from->subpixelOrder;
}

static void
SRROutputPropertyNotifyEvent(xRROutputPropertyNotifyEvent * from,
                             xRROutputPropertyNotifyEvent * to)
{
    to->type = from->type;
    to->subCode = from->subCode;
    cpswaps(from->sequenceNumber, to->sequenceNumber);
    cpswapl(from->window, to->window);
    cpswapl(from->output, to->output);
    cpswapl(from->atom, to->atom);
    cpswapl(from->timestamp, to->timestamp);
    to->state = from->state;
    /* pad1 */
    /* pad2 */
    /* pad3 */
    /* pad4 */
}

static void
SRRProviderChangeNotifyEvent(xRRProviderChangeNotifyEvent * from,
                             xRRProviderChangeNotifyEvent * to)
{
    to->type = from->type;
    to->subCode = from->subCode;
    cpswaps(from->sequenceNumber, to->sequenceNumber);
    cpswapl(from->timestamp, to->timestamp);
    cpswapl(from->window, to->window);
    cpswapl(from->provider, to->provider);
}

static void
SRRProviderPropertyNotifyEvent(xRRProviderPropertyNotifyEvent * from,
                               xRRProviderPropertyNotifyEvent * to)
{
    to->type = from->type;
    to->subCode = from->subCode;
    cpswaps(from->sequenceNumber, to->sequenceNumber);
    cpswapl(from->window, to->window);
    cpswapl(from->provider, to->provider);
    cpswapl(from->atom, to->atom);
    cpswapl(from->timestamp, to->timestamp);
    to->state = from->state;
    /* pad1 */
    /* pad2 */
    /* pad3 */
    /* pad4 */
}

static void
SRRResourceChangeNotifyEvent(xRRResourceChangeNotifyEvent * from,
                             xRRResourceChangeNotifyEvent * to)
{
    to->type = from->type;
    to->subCode = from->subCode;
    cpswaps(from->sequenceNumber, to->sequenceNumber);
    cpswapl(from->timestamp, to->timestamp);
    cpswapl(from->window, to->window);
}

static void
SRRNotifyEvent(xEvent *from, xEvent *to)
{
    switch (from->u.u.detail) {
    case RRNotify_CrtcChange:
        SRRCrtcChangeNotifyEvent((xRRCrtcChangeNotifyEvent *) from,
                                 (xRRCrtcChangeNotifyEvent *) to);
        break;
    case RRNotify_OutputChange:
        SRROutputChangeNotifyEvent((xRROutputChangeNotifyEvent *) from,
                                   (xRROutputChangeNotifyEvent *) to);
        break;
    case RRNotify_OutputProperty:
        SRROutputPropertyNotifyEvent((xRROutputPropertyNotifyEvent *) from,
                                     (xRROutputPropertyNotifyEvent *) to);
        break;
    case RRNotify_ProviderChange:
        SRRProviderChangeNotifyEvent((xRRProviderChangeNotifyEvent *) from,
                                     (xRRProviderChangeNotifyEvent *) to);
        break;
    case RRNotify_ProviderProperty:
        SRRProviderPropertyNotifyEvent((xRRProviderPropertyNotifyEvent *) from,
                                       (xRRProviderPropertyNotifyEvent *) to);
        break;
    case RRNotify_ResourceChange:
        SRRResourceChangeNotifyEvent((xRRResourceChangeNotifyEvent *) from,
                                     (xRRResourceChangeNotifyEvent *) to);
    default:
        break;
    }
}

static int RRGeneration;

Bool
RRInit(void)
{
    if (RRGeneration != serverGeneration) {
#ifdef NXAGENT_SERVER
        if ((rrPrivIndex = AllocateScreenPrivateIndex()) < 0)
            return FALSE;
#endif
        if (!RRModeInit())
            return FALSE;
        if (!RRCrtcInit())
            return FALSE;
        if (!RROutputInit())
            return FALSE;
        if (!RRProviderInit())
            return FALSE;
        RRGeneration = serverGeneration;
    }
#ifndef NXAGENT_SERVER
    if (!dixRegisterPrivateKey(&rrPrivKeyRec, PRIVATE_SCREEN, 0))
        return FALSE;
#endif                          /* !defined(NXAGENT_SERVER) */

    return TRUE;
}

Bool
RRScreenInit(ScreenPtr pScreen)
{
    rrScrPrivPtr pScrPriv;

    if (!RRInit())
        return FALSE;

    pScrPriv = (rrScrPrivPtr) calloc(1, sizeof(rrScrPrivRec));
    if (!pScrPriv)
        return FALSE;

    SetRRScreen(pScreen, pScrPriv);

    /*
     * Calling function best set these function vectors
     */
    pScrPriv->rrGetInfo = 0;
    pScrPriv->maxWidth = pScrPriv->minWidth = pScreen->width;
    pScrPriv->maxHeight = pScrPriv->minHeight = pScreen->height;

    pScrPriv->width = pScreen->width;
    pScrPriv->height = pScreen->height;
    pScrPriv->mmWidth = pScreen->mmWidth;
    pScrPriv->mmHeight = pScreen->mmHeight;
#if RANDR_12_INTERFACE
    pScrPriv->rrScreenSetSize = NULL;
    pScrPriv->rrCrtcSet = NULL;
    pScrPriv->rrCrtcSetGamma = NULL;
#endif
#if RANDR_10_INTERFACE
    pScrPriv->rrSetConfig = 0;
    pScrPriv->rotations = RR_Rotate_0;
    pScrPriv->reqWidth = pScreen->width;
    pScrPriv->reqHeight = pScreen->height;
    pScrPriv->nSizes = 0;
    pScrPriv->pSizes = NULL;
    pScrPriv->rotation = RR_Rotate_0;
    pScrPriv->rate = 0;
    pScrPriv->size = 0;
#endif

    /*
     * This value doesn't really matter -- any client must call
     * GetScreenInfo before reading it which will automatically update
     * the time
     */
    pScrPriv->lastSetTime = currentTime;
    pScrPriv->lastConfigTime = currentTime;

    wrap(pScrPriv, pScreen, CloseScreen, RRCloseScreen);

    pScreen->ConstrainCursorHarder = RRConstrainCursorHarder;
    pScreen->ReplaceScanoutPixmap = RRReplaceScanoutPixmap;
    pScrPriv->numOutputs = 0;
    pScrPriv->outputs = NULL;
    pScrPriv->numCrtcs = 0;
    pScrPriv->crtcs = NULL;

    RRMonitorInit(pScreen);

    RRNScreens += 1;            /* keep count of screens that implement randr */
    return TRUE;
}

 /*ARGSUSED*/ static int
RRFreeClient(void *data, XID id)
{
    RREventPtr pRREvent;
    WindowPtr pWin;
    RREventPtr *pHead, pCur, pPrev;

    pRREvent = (RREventPtr) data;
    pWin = pRREvent->window;
#ifndef NXAGENT_SERVER
    dixLookupResourceByType((void **) &pHead, pWin->drawable.id,
                            RREventType, serverClient, DixDestroyAccess);
#else                           /* !defined(NXAGENT_SERVER) */
    pHead = (RREventPtr *) LookupIDByType(pWin->drawable.id, RREventType);
#endif                          /* !defined(NXAGENT_SERVER) */

    if (pHead) {
        pPrev = 0;
        for (pCur = *pHead; pCur && pCur != pRREvent; pCur = pCur->next)
            pPrev = pCur;
        if (pCur) {
            if (pPrev)
                pPrev->next = pRREvent->next;
            else
                *pHead = pRREvent->next;
        }
    }
    free((void *) pRREvent);
    return 1;
}

 /*ARGSUSED*/ static int
RRFreeEvents(void *data, XID id)
{
    RREventPtr *pHead, pCur, pNext;

    pHead = (RREventPtr *) data;
    for (pCur = *pHead; pCur; pCur = pNext) {
        pNext = pCur->next;
        FreeResource(pCur->clientResource, RRClientType);
        free((void *) pCur);
    }
    free((void *) pHead);
    return 1;
}

void
RRExtensionInit(void)
{
    ExtensionEntry *extEntry;

    if (RRNScreens == 0)
        return;

#ifndef NXAGENT_SERVER
    if (!dixRegisterPrivateKey(&RRClientPrivateKeyRec, PRIVATE_CLIENT,
                               sizeof(RRClientRec) +
                               screenInfo.numScreens * sizeof(RRTimesRec)))
        return;
#else                           /* !defined(NXAGENT_SERVER) */
    RRClientPrivateIndex = AllocateClientPrivateIndex();
    if (!AllocateClientPrivate(RRClientPrivateIndex,
                               sizeof(RRClientRec) +
                               screenInfo.numScreens * sizeof(RRTimesRec)))
        return;
#endif                          /* !defined(NXAGENT_SERVER) */

    if (!AddCallback(&ClientStateCallback, RRClientCallback, 0))
        return;

    RRClientType = CreateNewResourceType(RRFreeClient
#ifndef NXAGENT_SERVER
                                         , "RandRClient"
#endif
        );
    if (!RRClientType)
        return;

#ifdef NXAGENT_SERVER
    RegisterResourceName(RRClientType, "RandRClient");
#endif

    RREventType = CreateNewResourceType(RRFreeEvents
#ifndef NXAGENT_SERVER
                                        , "RandREvent"
#endif
        );
    if (!RREventType)
        return;

#ifdef NXAGENT_SERVER
    RegisterResourceName(RREventType, "RandREvent");
#endif

    extEntry = AddExtension(RANDR_NAME, RRNumberEvents, RRNumberErrors,
                            ProcRRDispatch, SProcRRDispatch,
                            NULL, StandardMinorOpcode);
    if (!extEntry)
        return;
    RRErrorBase = extEntry->errorBase;
    RREventBase = extEntry->eventBase;
    EventSwapVector[RREventBase + RRScreenChangeNotify] = (EventSwapPtr)
        SRRScreenChangeNotifyEvent;
    EventSwapVector[RREventBase + RRNotify] = (EventSwapPtr)
        SRRNotifyEvent;

    RRModeInitErrorValue();
    RRCrtcInitErrorValue();
    RROutputInitErrorValue();
    RRProviderInitErrorValue();
#ifdef PANORAMIX
    RRXineramaExtensionInit();
#endif
}

void
RRResourcesChanged(ScreenPtr pScreen)
{
    rrScrPriv(pScreen);
    pScrPriv->resourcesChanged = TRUE;

    RRSetChanged(pScreen);
}

static void
RRDeliverResourceEvent(ClientPtr client, WindowPtr pWin)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;

    rrScrPriv(pScreen);

    xRRResourceChangeNotifyEvent re = {
        .type = RRNotify + RREventBase,
        .subCode = RRNotify_ResourceChange,
#ifdef NXAGENT_SERVER
        .sequenceNumber = client->sequence,
#endif
        .timestamp = pScrPriv->lastSetTime.milliseconds,
        .window = pWin->drawable.id
    };

    WriteEventsToClient(client, 1, (xEvent *) &re);
}

static int
TellChanged(WindowPtr pWin, void *value)
{
    RREventPtr *pHead, pRREvent;
    ClientPtr client;
    ScreenPtr pScreen = pWin->drawable.pScreen;

#ifndef NXAGENT_SERVER
    ScreenPtr iter;
    rrScrPrivPtr pSlaveScrPriv;
#endif

    rrScrPriv(pScreen);
    int i;

#ifndef NXAGENT_SERVER
    dixLookupResourceByType((void **) &pHead, pWin->drawable.id,
                            RREventType, serverClient, DixReadAccess);
#else                           /* !defined(NXAGENT_SERVER) */
    pHead = (RREventPtr *) LookupIDByType(pWin->drawable.id, RREventType);
#endif                          /* !defined(NXAGENT_SERVER) */
    if (!pHead)
        return WT_WALKCHILDREN;

    for (pRREvent = *pHead; pRREvent; pRREvent = pRREvent->next) {
        client = pRREvent->client;
        if (client == serverClient || client->clientGone)
            continue;

        if (pRREvent->mask & RRScreenChangeNotifyMask)
            RRDeliverScreenEvent(client, pWin, pScreen);

        if (pRREvent->mask & RRCrtcChangeNotifyMask) {
            for (i = 0; i < pScrPriv->numCrtcs; i++) {
                RRCrtcPtr crtc = pScrPriv->crtcs[i];

                if (crtc->changed)
                    RRDeliverCrtcEvent(client, pWin, crtc);
            }

#ifndef NXAGENT_SERVER
            xorg_list_for_each_entry(iter, &pScreen->output_slave_list, output_head) {
                pSlaveScrPriv = rrGetScrPriv(iter);
                for (i = 0; i < pSlaveScrPriv->numCrtcs; i++) {
                    RRCrtcPtr crtc = pSlaveScrPriv->crtcs[i];

                    if (crtc->changed)
                        RRDeliverCrtcEvent(client, pWin, crtc);
                }
            }
#endif
        }

        if (pRREvent->mask & RROutputChangeNotifyMask) {
            for (i = 0; i < pScrPriv->numOutputs; i++) {
                RROutputPtr output = pScrPriv->outputs[i];

                if (output->changed)
                    RRDeliverOutputEvent(client, pWin, output);
            }

#ifndef NXAGENT_SERVER
            xorg_list_for_each_entry(iter, &pScreen->output_slave_list, output_head) {
                pSlaveScrPriv = rrGetScrPriv(iter);
                for (i = 0; i < pSlaveScrPriv->numOutputs; i++) {
                    RROutputPtr output = pSlaveScrPriv->outputs[i];

                    if (output->changed)
                        RRDeliverOutputEvent(client, pWin, output);
                }
            }
#endif
        }

#ifndef NXAGENT_SERVER
        if (pRREvent->mask & RRProviderChangeNotifyMask) {
            xorg_list_for_each_entry(iter, &pScreen->output_slave_list, output_head) {
                pSlaveScrPriv = rrGetScrPriv(iter);
                if (pSlaveScrPriv->provider->changed)
                    RRDeliverProviderEvent(client, pWin, pSlaveScrPriv->provider);
            }
            xorg_list_for_each_entry(iter, &pScreen->offload_slave_list, offload_head) {
                pSlaveScrPriv = rrGetScrPriv(iter);
                if (pSlaveScrPriv->provider->changed)
                    RRDeliverProviderEvent(client, pWin, pSlaveScrPriv->provider);
            }
            xorg_list_for_each_entry(iter, &pScreen->unattached_list, unattached_head) {
                pSlaveScrPriv = rrGetScrPriv(iter);
                if (pSlaveScrPriv->provider->changed)
                    RRDeliverProviderEvent(client, pWin, pSlaveScrPriv->provider);
            }
        }
#endif

        if (pRREvent->mask & RRResourceChangeNotifyMask) {
            if (pScrPriv->resourcesChanged) {
                RRDeliverResourceEvent(client, pWin);
            }
        }
    }
    return WT_WALKCHILDREN;
}

void
RRSetChanged(ScreenPtr pScreen)
{
#ifndef NXAGENT_SERVER
    /* set changed bits on the master screen only */
    ScreenPtr master;

    rrScrPriv(pScreen);
    rrScrPrivPtr mastersp;

    if (pScreen->isGPU) {
        master = pScreen->current_master;
        if (!master)
            return;
        mastersp = rrGetScrPriv(master);
    }
    else
    {
        master = pScreen;
        mastersp = pScrPriv;
    }

    mastersp->changed = TRUE;
#else /* !defined(NXAGENT_SERVER) */
    rrScrPriv(pScreen);
    pScrPriv->changed = TRUE;
#endif
}

/*
 * Something changed; send events and adjust pointer position
 */
void
RRTellChanged(ScreenPtr pScreen)
{
    ScreenPtr master;
    rrScrPriv(pScreen);
    rrScrPrivPtr mastersp;
    int i;
#ifndef NXAGENT_SERVER
    ScreenPtr iter;
    rrScrPrivPtr pSlaveScrPriv;
#endif

#ifndef NXAGENT_SERVER
    if (pScreen->isGPU) {
        master = pScreen->current_master;
        mastersp = rrGetScrPriv(master);
    }
    else
#endif
    {
        master = pScreen;
        mastersp = pScrPriv;
    }

    if (mastersp->changed) {
        UpdateCurrentTimeIf();
        if (mastersp->configChanged) {
            mastersp->lastConfigTime = currentTime;
            mastersp->configChanged = FALSE;
        }
        pScrPriv->changed = FALSE;
        mastersp->changed = FALSE;

        WalkTree(master, TellChanged, (void *) master);

        mastersp->resourcesChanged = FALSE;

        for (i = 0; i < pScrPriv->numOutputs; i++)
            pScrPriv->outputs[i]->changed = FALSE;
        for (i = 0; i < pScrPriv->numCrtcs; i++)
            pScrPriv->crtcs[i]->changed = FALSE;

#ifndef NXAGENT_SERVER
        xorg_list_for_each_entry(iter, &master->output_slave_list, output_head) {
            pSlaveScrPriv = rrGetScrPriv(iter);
            pSlaveScrPriv->provider->changed = FALSE;
            for (i = 0; i < pSlaveScrPriv->numOutputs; i++)
                pSlaveScrPriv->outputs[i]->changed = FALSE;
            for (i = 0; i < pSlaveScrPriv->numCrtcs; i++)
                pSlaveScrPriv->crtcs[i]->changed = FALSE;
        }
        xorg_list_for_each_entry(iter, &master->offload_slave_list, offload_head) {
            pSlaveScrPriv = rrGetScrPriv(iter);
            pSlaveScrPriv->provider->changed = FALSE;
        }
        xorg_list_for_each_entry(iter, &master->unattached_list, unattached_head) {
            pSlaveScrPriv = rrGetScrPriv(iter);
            pSlaveScrPriv->provider->changed = FALSE;
        }
#endif /* !defined(NXAGENT_SERVER) */

        if (mastersp->layoutChanged) {
            pScrPriv->layoutChanged = FALSE;
            RRPointerScreenConfigured(master);
            RRSendConfigNotify(master);
        }
    }
}

/*
 * Return the first output which is connected to an active CRTC
 * Used in emulating 1.0 behaviour
 */
RROutputPtr
RRFirstOutput(ScreenPtr pScreen)
{
    rrScrPriv(pScreen);
    RROutputPtr output;
    int i, j;

    if (!pScrPriv)
        return NULL;

    if (pScrPriv->primaryOutput && pScrPriv->primaryOutput->crtc)
        return pScrPriv->primaryOutput;

    for (i = 0; i < pScrPriv->numCrtcs; i++) {
        RRCrtcPtr crtc = pScrPriv->crtcs[i];

        for (j = 0; j < pScrPriv->numOutputs; j++) {
            output = pScrPriv->outputs[j];
            if (output->crtc == crtc)
                return output;
        }
    }
    return NULL;
}

CARD16
RRVerticalRefresh(xRRModeInfo * mode)
{
    CARD32 refresh;
    CARD32 dots = mode->hTotal * mode->vTotal;

    if (!dots)
        return 0;
    refresh = (mode->dotClock + dots / 2) / dots;
    if (refresh > 0xffff)
        refresh = 0xffff;
    return (CARD16) refresh;
}

static int
ProcRRDispatch(ClientPtr client)
{
    REQUEST(xReq);
    if (stuff->data >= RRNumberRequests || !ProcRandrVector[stuff->data])
        return BadRequest;
    UpdateCurrentTimeIf();
    return (*ProcRandrVector[stuff->data]) (client);
}

static int
SProcRRDispatch(ClientPtr client)
{
    REQUEST(xReq);
    if (stuff->data >= RRNumberRequests || !SProcRandrVector[stuff->data])
        return BadRequest;
    UpdateCurrentTimeIf();
    return (*SProcRandrVector[stuff->data]) (client);
}
