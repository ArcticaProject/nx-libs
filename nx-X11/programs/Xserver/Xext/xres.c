/*
   Copyright (c) 2002  XFree86 Inc
*/

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/X.h>
#include <X11/Xproto.h>
#include "misc.h"
#include "os.h"
#include "dixstruct.h"
#include "extnsionst.h"
#include "swaprep.h"
#include <nx-X11/extensions/XResproto.h>
#include "pixmapstr.h"
#include "modinit.h"
#include "protocol-versions.h"

static int
ProcXResQueryVersion (ClientPtr client)
{
    xXResQueryVersionReply rep;

    REQUEST_SIZE_MATCH (xXResQueryVersionReq);

    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    rep.server_major = SERVER_XRES_MAJOR_VERSION;
    rep.server_minor = SERVER_XRES_MINOR_VERSION;
    if (client->swapped) { 
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swaps(&rep.server_major);
        swaps(&rep.server_minor);
    }
    WriteToClient(client, sizeof (xXResQueryVersionReply), &rep);
    return (client->noClientException);
}

static int
ProcXResQueryClients (ClientPtr client)
{
    /* REQUEST(xXResQueryClientsReq); */
    xXResQueryClientsReply rep;
    int *current_clients;
    int i, num_clients;

    REQUEST_SIZE_MATCH(xXResQueryClientsReq);

    current_clients = ALLOCATE_LOCAL((currentMaxClients - 1) * sizeof(int));

    num_clients = 0;
    for(i = 1; i < currentMaxClients; i++) {
       if(clients[i]) {
           current_clients[num_clients] = i;
           num_clients++;   
       }
    }

    rep.type = X_Reply;
    rep.sequenceNumber = client->sequence;
    rep.num_clients = num_clients;
    rep.length = rep.num_clients * sz_xXResClient >> 2;
    if (client->swapped) {
        swaps (&rep.sequenceNumber);
        swapl (&rep.length);
        swapl (&rep.num_clients);
    }   
    WriteToClient (client, sizeof (xXResQueryClientsReply), &rep);

    if(num_clients) {
        xXResClient scratch;

        for(i = 0; i < num_clients; i++) {
            scratch.resource_base = clients[current_clients[i]]->clientAsMask;
            scratch.resource_mask = RESOURCE_ID_MASK;
        
            if(client->swapped) {
                swapl (&scratch.resource_base);
                swapl (&scratch.resource_mask);
            }
            WriteToClient (client, sz_xXResClient, &scratch);
        }
    }

    DEALLOCATE_LOCAL(current_clients);

    return (client->noClientException);
}


static void
ResFindAllRes (void * value, XID id, RESTYPE type, void * cdata)
{
    int *counts = (int *)cdata;

    counts[(type & TypeMask) - 1]++;
}

static int
ProcXResQueryClientResources (ClientPtr client)
{
    REQUEST(xXResQueryClientResourcesReq);
    xXResQueryClientResourcesReply rep;
    int i, clientID, num_types;
    int *counts;

    REQUEST_SIZE_MATCH(xXResQueryClientResourcesReq);

    clientID = CLIENT_ID(stuff->xid);

    /* we could remove the (clientID == 0) check if we wanted to allow
       probing the X-server's resource usage */
    if(!clientID || (clientID >= currentMaxClients) || !clients[clientID]) {
        client->errorValue = stuff->xid;
        return BadValue;
    }

    counts = ALLOCATE_LOCAL((lastResourceType + 1) * sizeof(int));

    memset(counts, 0, (lastResourceType + 1) * sizeof(int));

    FindAllClientResources(clients[clientID], ResFindAllRes, counts);

    num_types = 0;

    for(i = 0; i <= lastResourceType; i++) {
       if(counts[i]) num_types++;
    }

    rep.type = X_Reply;
    rep.sequenceNumber = client->sequence;
    rep.num_types = num_types;
    rep.length = rep.num_types * sz_xXResType >> 2;
    if (client->swapped) {
        swaps (&rep.sequenceNumber);
        swapl (&rep.length);
        swapl (&rep.num_types);
    }   
    WriteToClient (client,sizeof(xXResQueryClientResourcesReply),&rep);

    if(num_types) {
        xXResType scratch;

        for(i = 0; i < lastResourceType; i++) {
            if(!counts[i]) continue;

            if(!ResourceNames[i + 1]) {
                char buf[40];
                snprintf(buf, sizeof(buf), "Unregistered resource %i", i + 1);
                RegisterResourceName(i + 1, buf);
            }

            scratch.resource_type = ResourceNames[i + 1];
            scratch.count = counts[i];

            if(client->swapped) {
                swapl (&scratch.resource_type);
                swapl (&scratch.count);
            }
            WriteToClient (client, sz_xXResType, &scratch);
        }
    }

    DEALLOCATE_LOCAL(counts);
    
    return (client->noClientException);
}

static void 
ResFindPixmaps (void * value, XID id, void * cdata)
{
   unsigned long *bytes = (unsigned long *)cdata;
   PixmapPtr pix = (PixmapPtr)value;

   *bytes += (pix->devKind * pix->drawable.height);
}

static int
ProcXResQueryClientPixmapBytes (ClientPtr client)
{
    REQUEST(xXResQueryClientPixmapBytesReq);
    xXResQueryClientPixmapBytesReply rep;
    int clientID;
    unsigned long bytes;

    REQUEST_SIZE_MATCH(xXResQueryClientPixmapBytesReq);

    clientID = CLIENT_ID(stuff->xid);

    /* we could remove the (clientID == 0) check if we wanted to allow
       probing the X-server's resource usage */
    if(!clientID || (clientID >= currentMaxClients) || !clients[clientID]) {
        client->errorValue = stuff->xid;
        return BadValue;
    }

    bytes = 0;

    FindClientResourcesByType(clients[clientID], RT_PIXMAP, ResFindPixmaps, 
                              (void *)(&bytes));

    rep.type = X_Reply;
    rep.sequenceNumber = client->sequence;
    rep.length = 0;
    rep.bytes = bytes;
#ifdef XSERVER64
    rep.bytes_overflow = bytes >> 32;
#else
    rep.bytes_overflow = 0;
#endif
    if (client->swapped) {
        swaps (&rep.sequenceNumber);
        swapl (&rep.length);
        swapl (&rep.bytes);
        swapl (&rep.bytes_overflow);
    }
    WriteToClient (client,sizeof(xXResQueryClientPixmapBytesReply),&rep);

    return (client->noClientException);
}


static void
ResResetProc (ExtensionEntry *extEntry) { }

static int
ProcResDispatch (ClientPtr client)
{
    REQUEST(xReq);
    switch (stuff->data) {
    case X_XResQueryVersion:
        return ProcXResQueryVersion(client);
    case X_XResQueryClients:
        return ProcXResQueryClients(client);
    case X_XResQueryClientResources:
        return ProcXResQueryClientResources(client);
    case X_XResQueryClientPixmapBytes:
        return ProcXResQueryClientPixmapBytes(client);
    default: break;
    }

    return BadRequest;
}

static int
SProcXResQueryVersion (ClientPtr client)
{
    REQUEST_SIZE_MATCH (xXResQueryVersionReq);
    return ProcXResQueryVersion(client);
}

static int
SProcXResQueryClientResources (ClientPtr client)
{
    REQUEST(xXResQueryClientResourcesReq);

    REQUEST_SIZE_MATCH (xXResQueryClientResourcesReq);
    swapl(&stuff->xid);
    return ProcXResQueryClientResources(client);
}

static int
SProcXResQueryClientPixmapBytes (ClientPtr client)
{
    REQUEST(xXResQueryClientPixmapBytesReq);

    REQUEST_SIZE_MATCH (xXResQueryClientPixmapBytesReq);
    swapl(&stuff->xid);
    return ProcXResQueryClientPixmapBytes(client);
}

static int
SProcResDispatch (ClientPtr client)
{
    REQUEST(xReq);

    swaps(&stuff->length);

    switch (stuff->data) {
    case X_XResQueryVersion:
        return SProcXResQueryVersion(client);
    case X_XResQueryClients:  /* nothing to swap */
        return ProcXResQueryClients(client);
    case X_XResQueryClientResources:
        return SProcXResQueryClientResources(client);
    case X_XResQueryClientPixmapBytes:
        return SProcXResQueryClientPixmapBytes(client);
    default: break;
    }

    return BadRequest;
}

void
ResExtensionInit(void)
{
    (void) AddExtension(XRES_NAME, 0, 0,
                            ProcResDispatch, SProcResDispatch,
                            ResResetProc, StandardMinorOpcode);

    RegisterResourceName(RT_NONE, "NONE");
    RegisterResourceName(RT_WINDOW, "WINDOW");
    RegisterResourceName(RT_PIXMAP, "PIXMAP");
    RegisterResourceName(RT_GC, "GC");
    RegisterResourceName(RT_FONT, "FONT");
    RegisterResourceName(RT_CURSOR, "CURSOR");
    RegisterResourceName(RT_COLORMAP, "COLORMAP");
    RegisterResourceName(RT_CMAPENTRY, "COLORMAP ENTRY");
    RegisterResourceName(RT_OTHERCLIENT, "OTHER CLIENT");
    RegisterResourceName(RT_PASSIVEGRAB, "PASSIVE GRAB");
}
