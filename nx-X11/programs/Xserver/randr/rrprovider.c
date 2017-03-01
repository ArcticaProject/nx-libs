/*
 * Copyright © 2012 Red Hat Inc.
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
 * Authors: Dave Airlie
 */

#include "randrstr.h"
#include "swaprep.h"

RESTYPE RRProviderType;

/*
 * Initialize provider type error value
 */
void
RRProviderInitErrorValue(void)
{
#ifndef NXAGENT_SERVER
    SetResourceTypeErrorValue(RRProviderType, RRErrorBase + BadRRProvider);
#endif
}

#define ADD_PROVIDER(_pScreen) do {                                 \
    pScrPriv = rrGetScrPriv((_pScreen));                            \
    if (pScrPriv->provider) {                                   \
        providers[count_providers] = pScrPriv->provider->id;    \
        if (client->swapped)                                    \
            swapl(&providers[count_providers]);                 \
        count_providers++;                                      \
    }                                                           \
    } while(0)

int
ProcRRGetProviders(ClientPtr client)
{
    REQUEST(xRRGetProvidersReq);
    xRRGetProvidersReply rep;
    WindowPtr pWin;
    ScreenPtr pScreen;
    rrScrPrivPtr pScrPriv;
    int rc;
    CARD8 *extra;
    unsigned int extraLen;
    RRProvider *providers;
    int total_providers = 0, count_providers = 0;
#ifndef NXAGENT_SERVER
    ScreenPtr iter;
#endif

    REQUEST_SIZE_MATCH(xRRGetProvidersReq);
#ifndef NXAGENT_SERVER
    rc = dixLookupWindow(&pWin, stuff->window, client, DixGetAttrAccess);
#else
    pWin = SecurityLookupWindow(stuff->window, client, DixReadAccess);
    rc = pWin ? Success : BadWindow;
#endif

    if (rc != Success)
        return rc;

    pScreen = pWin->drawable.pScreen;

    pScrPriv = rrGetScrPriv(pScreen);

    if (pScrPriv->provider)
        total_providers++;
#ifndef NXAGENT_SERVER
    xorg_list_for_each_entry(iter, &pScreen->output_slave_list, output_head) {
        pScrPriv = rrGetScrPriv(iter);
        total_providers += pScrPriv->provider ? 1 : 0;
    }
    xorg_list_for_each_entry(iter, &pScreen->offload_slave_list, offload_head) {
        pScrPriv = rrGetScrPriv(iter);
        total_providers += pScrPriv->provider ? 1 : 0;
    }
    xorg_list_for_each_entry(iter, &pScreen->unattached_list, unattached_head) {
        pScrPriv = rrGetScrPriv(iter);
        total_providers += pScrPriv->provider ? 1 : 0;
    }
#endif

    pScrPriv = rrGetScrPriv(pScreen);

    if (!pScrPriv)
    {
        rep = (xRRGetProvidersReply) {
            .type = X_Reply,
            .sequenceNumber = client->sequence,
            .length = 0,
            .timestamp = currentTime.milliseconds,
            .nProviders = 0
        };
        extra = NULL;
        extraLen = 0;
    } else {
        rep = (xRRGetProvidersReply) {
            .type = X_Reply,
            .sequenceNumber = client->sequence,
            .timestamp = pScrPriv->lastSetTime.milliseconds,
            .nProviders = total_providers,
            .length = total_providers
        };
        extraLen = rep.length << 2;
        if (extraLen) {
            extra = malloc(extraLen);
            if (!extra)
                return BadAlloc;
        } else
            extra = NULL;

        providers = (RRProvider *) extra;
        ADD_PROVIDER(pScreen);
#ifndef NXAGENT_SERVER
        xorg_list_for_each_entry(iter, &pScreen->output_slave_list, output_head) {
            ADD_PROVIDER(iter);
        }
        xorg_list_for_each_entry(iter, &pScreen->offload_slave_list, offload_head) {
            ADD_PROVIDER(iter);
        }
        xorg_list_for_each_entry(iter, &pScreen->unattached_list, unattached_head) {
            ADD_PROVIDER(iter);
        }
#endif
    }

    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swapl(&rep.timestamp);
        swaps(&rep.nProviders);
    }
    WriteToClient(client, sizeof(xRRGetProvidersReply), &rep);
    if (extraLen)
    {
        WriteToClient(client, extraLen, extra);
        free(extra);
    }
    return Success;
}

int
ProcRRGetProviderInfo(ClientPtr client)
{
    REQUEST(xRRGetProviderInfoReq);
    xRRGetProviderInfoReply rep;
    rrScrPrivPtr pScrPriv;
#ifndef NXAGENT_SERVER
    rrScrPrivPtr pScrProvPriv;
#endif
    RRProviderPtr provider;
    ScreenPtr pScreen;
    CARD8 *extra;
    unsigned int extraLen = 0;
    RRCrtc *crtcs;
    RROutput *outputs;
    int i;
    char *name;
#ifndef NXAGENT_SERVER
    ScreenPtr provscreen;
#endif
    RRProvider *providers;
    uint32_t *prov_cap;

    REQUEST_SIZE_MATCH(xRRGetProviderInfoReq);
    VERIFY_RR_PROVIDER(stuff->provider, provider, DixReadAccess);

    pScreen = provider->pScreen;
    pScrPriv = rrGetScrPriv(pScreen);

    rep = (xRRGetProviderInfoReply) {
        .type = X_Reply,
        .status = RRSetConfigSuccess,
        .sequenceNumber = client->sequence,
        .length = 0,
        .capabilities = provider->capabilities,
        .nameLength = provider->nameLength,
        .timestamp = pScrPriv->lastSetTime.milliseconds,
        .nCrtcs = pScrPriv->numCrtcs,
        .nOutputs = pScrPriv->numOutputs,
        .nAssociatedProviders = 0
    };

    /* count associated providers */
    if (provider->offload_sink)
        rep.nAssociatedProviders++;
    if (provider->output_source)
        rep.nAssociatedProviders++;
#ifndef NXAGENT_SERVER
    xorg_list_for_each_entry(provscreen, &pScreen->output_slave_list, output_head)
        rep.nAssociatedProviders++;
    xorg_list_for_each_entry(provscreen, &pScreen->offload_slave_list, offload_head)
        rep.nAssociatedProviders++;
#endif

    rep.length = (pScrPriv->numCrtcs + pScrPriv->numOutputs +
                  (rep.nAssociatedProviders * 2) + bytes_to_int32(rep.nameLength));

    extraLen = rep.length << 2;
    if (extraLen) {
        extra = malloc(extraLen);
        if (!extra)
            return BadAlloc;
    }
    else
        extra = NULL;

    crtcs = (RRCrtc *) extra;
    outputs = (RROutput *) (crtcs + rep.nCrtcs);
    providers = (RRProvider *) (outputs + rep.nOutputs);
    prov_cap = (unsigned int *) (providers + rep.nAssociatedProviders);
    name = (char *) (prov_cap + rep.nAssociatedProviders);

    for (i = 0; i < pScrPriv->numCrtcs; i++) {
        crtcs[i] = pScrPriv->crtcs[i]->id;
        if (client->swapped)
            swapl(&crtcs[i]);
    }

    for (i = 0; i < pScrPriv->numOutputs; i++) {
        outputs[i] = pScrPriv->outputs[i]->id;
        if (client->swapped)
            swapl(&outputs[i]);
    }

    i = 0;
    if (provider->offload_sink) {
        providers[i] = provider->offload_sink->id;
        if (client->swapped)
            swapl(&providers[i]);
        prov_cap[i] = RR_Capability_SinkOffload;
        if (client->swapped)
            swapl(&prov_cap[i]);
        i++;
    }
    if (provider->output_source) {
        providers[i] = provider->output_source->id;
        if (client->swapped)
            swapl(&providers[i]);
        prov_cap[i] = RR_Capability_SourceOutput;
        swapl(&prov_cap[i]);
        i++;
    }
#ifndef NXAGENT_SERVER
    xorg_list_for_each_entry(provscreen, &pScreen->output_slave_list, output_head) {
        pScrProvPriv = rrGetScrPriv(provscreen);
        providers[i] = pScrProvPriv->provider->id;
        if (client->swapped)
            swapl(&providers[i]);
        prov_cap[i] = RR_Capability_SinkOutput;
        if (client->swapped)
            swapl(&prov_cap[i]);
        i++;
    }
    xorg_list_for_each_entry(provscreen, &pScreen->offload_slave_list, offload_head) {
        pScrProvPriv = rrGetScrPriv(provscreen);
        providers[i] = pScrProvPriv->provider->id;
        if (client->swapped)
            swapl(&providers[i]);
        prov_cap[i] = RR_Capability_SourceOffload;
        if (client->swapped)
            swapl(&prov_cap[i]);
        i++;
    }
#endif
    memcpy(name, provider->name, rep.nameLength);
    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swapl(&rep.capabilities);
        swaps(&rep.nCrtcs);
        swaps(&rep.nOutputs);
        swaps(&rep.nameLength);
    }
    WriteToClient(client, sizeof(xRRGetProviderInfoReply), &rep);
    if (extraLen)
    {
        WriteToClient(client, extraLen, extra);
        free(extra);
    }
    return Success;
}

int
ProcRRSetProviderOutputSource(ClientPtr client)
{
    REQUEST(xRRSetProviderOutputSourceReq);
    rrScrPrivPtr pScrPriv;
    RRProviderPtr provider, source_provider = NULL;
    ScreenPtr pScreen;

    REQUEST_SIZE_MATCH(xRRSetProviderOutputSourceReq);

    VERIFY_RR_PROVIDER(stuff->provider, provider, DixReadAccess);

    if (!(provider->capabilities & RR_Capability_SinkOutput))
        return BadValue;

    if (stuff->source_provider) {
        VERIFY_RR_PROVIDER(stuff->source_provider, source_provider, DixReadAccess);

        if (!(source_provider->capabilities & RR_Capability_SourceOutput))
            return BadValue;
    }

    pScreen = provider->pScreen;
    pScrPriv = rrGetScrPriv(pScreen);

    pScrPriv->rrProviderSetOutputSource(pScreen, provider, source_provider);

    provider->changed = TRUE;
    RRSetChanged(pScreen);

    RRTellChanged(pScreen);

    return Success;
}

int
ProcRRSetProviderOffloadSink(ClientPtr client)
{
    REQUEST(xRRSetProviderOffloadSinkReq);
    rrScrPrivPtr pScrPriv;
    RRProviderPtr provider, sink_provider = NULL;
    ScreenPtr pScreen;

    REQUEST_SIZE_MATCH(xRRSetProviderOffloadSinkReq);

    VERIFY_RR_PROVIDER(stuff->provider, provider, DixReadAccess);
    if (!(provider->capabilities & RR_Capability_SourceOffload))
        return BadValue;
#ifndef NXAGENT_SERVER
    if (!provider->pScreen->isGPU)
        return BadValue;
#endif                          /* !defined(NXAGENT_SERVER) */

    if (stuff->sink_provider) {
        VERIFY_RR_PROVIDER(stuff->sink_provider, sink_provider, DixReadAccess);
        if (!(sink_provider->capabilities & RR_Capability_SinkOffload))
            return BadValue;
    }
    pScreen = provider->pScreen;
    pScrPriv = rrGetScrPriv(pScreen);

    pScrPriv->rrProviderSetOffloadSink(pScreen, provider, sink_provider);

    provider->changed = TRUE;
    RRSetChanged(pScreen);

    RRTellChanged(pScreen);

    return Success;
}

RRProviderPtr
RRProviderCreate(ScreenPtr pScreen, const char *name,
                 int nameLength)
{
    RRProviderPtr provider;
    rrScrPrivPtr pScrPriv;

    pScrPriv = rrGetScrPriv(pScreen);

    provider = calloc(1, sizeof(RRProviderRec) + nameLength + 1);
    if (!provider)
        return NULL;

    provider->id = FakeClientID(0);
    provider->pScreen = pScreen;
    provider->name = (char *) (provider + 1);
    provider->nameLength = nameLength;
    memcpy(provider->name, name, nameLength);
    provider->name[nameLength] = '\0';
    provider->changed = FALSE;

    if (!AddResource(provider->id, RRProviderType, (void *) provider))
        return NULL;
    pScrPriv->provider = provider;
    return provider;
}

/*
 * Destroy a provider at shutdown
 */
void
RRProviderDestroy(RRProviderPtr provider)
{
    FreeResource(provider->id, 0);
}

void
RRProviderSetCapabilities(RRProviderPtr provider, uint32_t capabilities)
{
    provider->capabilities = capabilities;
}

static int
RRProviderDestroyResource(void *value, XID pid)
{
    RRProviderPtr provider = (RRProviderPtr) value;
    ScreenPtr pScreen = provider->pScreen;

    if (pScreen)
    {
        rrScrPriv(pScreen);

        if (pScrPriv->rrProviderDestroy)
            (*pScrPriv->rrProviderDestroy) (pScreen, provider);
        pScrPriv->provider = NULL;
    }
    free(provider);
    return 1;
}

Bool
RRProviderInit(void)
{
    RRProviderType = CreateNewResourceType(RRProviderDestroyResource
#ifndef NXAGENT_SERVER
                                           , "Provider"
#endif                          /* !defined(NXAGENT_SERVER) */
        );
    if (!RRProviderType)
        return FALSE;

#ifdef NXAGENT_SERVER
    RegisterResourceName(RRProviderType, "Provider");
#endif

    return TRUE;
}

extern _X_EXPORT Bool
RRProviderLookup(XID id, RRProviderPtr * provider_p)
{
#ifndef NXAGENT_SERVER
    int rc = dixLookupResourceByType((void **) provider_p, id,
                                     RRProviderType, NullClient, DixReadAccess);
    if (rc == Success)
        return TRUE;
#else                           /* !defined(NXAGENT_SERVER) */
    provider_p = (RRProviderPtr *) LookupIDByType(id, RREventType);
    if (provider_p)
        return TRUE;
#endif                          /* !defined(NXAGENT_SERVER) */

    return FALSE;
}

void
RRDeliverProviderEvent(ClientPtr client, WindowPtr pWin, RRProviderPtr provider)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;

    rrScrPriv(pScreen);

    xRRProviderChangeNotifyEvent pe = {
        .type = RRNotify + RREventBase,
        .subCode = RRNotify_ProviderChange,
#ifdef NXAGENT_SERVER
        .sequenceNumber = client->sequence,
#endif
        .timestamp = pScrPriv->lastSetTime.milliseconds,
        .window = pWin->drawable.id,
        .provider = provider->id
    };

    WriteEventsToClient(client, 1, (xEvent *) &pe);
}
