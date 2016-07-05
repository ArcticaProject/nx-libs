/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2011 NoMachine (http://www.nomachine.com)          */
/* Copyright (c) 2008-2014 Oleksandr Shneyder <o.shneyder@phoca-gmbh.de>  */
/* Copyright (c) 2011-2016 Mike Gabriel <mike.gabriel@das-netzwerkteam.de>*/
/* Copyright (c) 2014-2016 Mihai Moldovan <ionic@ionic.de>                */
/* Copyright (c) 2014-2016 Ulrich Sibiller <uli42@gmx.de>                 */
/* Copyright (c) 2015-2016 Qindel Group (http://www.qindel.com)           */
/*                                                                        */
/* NXAGENT, NX protocol compression and NX extensions to this software    */
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

/************************************************************

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

********************************************************/
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

/* $Xorg: resource.c,v 1.5 2001/02/09 02:04:40 xorgcvs Exp $ */
/* $XdotOrg: xc/programs/Xserver/dix/resource.c,v 1.8 2005/07/03 08:53:38 daniels Exp $ */
/* $TOG: resource.c /main/41 1998/02/09 14:20:31 kaleb $ */

/*	Routines to manage various kinds of resources:
 *
 *	CreateNewResourceType, CreateNewResourceClass, InitClientResources,
 *	FakeClientID, AddResource, FreeResource, FreeClientResources,
 *	FreeAllResources, LookupIDByType, LookupIDByClass, GetXIDRange
 */

/* 
 *      A resource ID is a 32 bit quantity, the upper 2 bits of which are
 *	off-limits for client-visible resources.  The next 8 bits are
 *      used as client ID, and the low 22 bits come from the client.
 *	A resource ID is "hashed" by extracting and xoring subfields
 *      (varying with the size of the hash table).
 *
 *      It is sometimes necessary for the server to create an ID that looks
 *      like it belongs to a client.  This ID, however,  must not be one
 *      the client actually can create, or we have the potential for conflict.
 *      The 31st bit of the ID is reserved for the server's use for this
 *      purpose.  By setting CLIENT_ID(id) to the client, the SERVER_BIT to
 *      1, and an otherwise arbitrary ID in the low 22 bits, we can create a
 *      resource "owned" by the client.
 */
/* $XFree86: xc/programs/Xserver/dix/resource.c,v 3.13 2003/09/24 02:43:13 dawes Exp $ */

#include "../../dix/resource.c"

#include "Agent.h"
#include "Font.h"
#include "Pixmaps.h"
#include "GCs.h"

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

#ifdef NXAGENT_SERVER
static int nxagentResChangedFlag = 0;
#endif

#ifdef NXAGENT_SERVER
int nxagentFindClientResource(int client, RESTYPE type, void * value)
{
  ResourcePtr pResource;
  ResourcePtr *resources;

  int i;

  for (i = 0; i < clientTable[client].buckets; i++)
  {
    resources = clientTable[client].resources;

    for (pResource = resources[i]; pResource; pResource = pResource -> next)
    {
      if (pResource -> type == type && pResource -> value == value)
      {
        #ifdef TEST
        fprintf(stderr, "nxagentFindClientResource: Found resource [%p] type [%lu] "
                    "for client [%d].\n", (void *) value,
                        pResource -> type, client);
        #endif

        return 1;
      }
    }
  }

  return 0;
}

int nxagentSwitchResourceType(int client, RESTYPE type, void * value)
{
  ResourcePtr pResource;
  ResourcePtr *resources;

  RESTYPE internalType = 0;

  int i;

  if (type == RT_PIXMAP)
  {
    internalType = RT_NX_PIXMAP;
  }
  else if (type == RT_GC)
  {
    internalType = RT_NX_GC;
  }
  else if (type == RT_FONT)
  {
    internalType = RT_NX_FONT;
  }
  else
  {
    return 0;
  }

  if (client == serverClient -> index)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentSwitchResourceType: Requesting client is [%d]. Skipping the resource switch.\n",
                client);
    #endif

    return 0;
  }

  for (i = 0; i < clientTable[serverClient -> index].buckets; i++)
  {
    resources = clientTable[serverClient -> index].resources;

    for (pResource = resources[i]; pResource; pResource = pResource -> next)
    {
      if (pResource -> type == internalType &&
              pResource -> value == value)
      {
        #ifdef TEST
        fprintf(stderr, "nxagentSwitchResourceType: Changing resource [%p] type from [%lu] to "
                    "[%lu] for server client [%d].\n", (void *) value,
                        (unsigned long) pResource -> type, (unsigned long) type, serverClient -> index);
        #endif

        FreeResource(pResource -> id, RT_NONE);

        return 1;
      }
    }
  }

  return 0;
}
#endif /* NXAGENT_SERVER */

Bool
AddResource(XID id, RESTYPE type, void * value)
{
    int client;
    register ClientResourceRec *rrec;
    register ResourcePtr res, *head;

    client = CLIENT_ID(id);
    rrec = &clientTable[client];
    if (!rrec->buckets)
    {
	ErrorF("AddResource(%lx, %lx, %lx), client=%d \n",
		(unsigned long)id, type, (unsigned long)value, client);
        FatalError("client not in use\n");
    }

#ifdef NXAGENT_SERVER

    nxagentSwitchResourceType(client, type, value);

    #ifdef TEST
    fprintf(stderr, "AddResource: Adding resource for client [%d] type [%lu] value [%p] id [%lu].\n",
                client, (unsigned long) type, (void *) value, (unsigned long) id);
    #endif

#endif

    if ((rrec->elements >= 4*rrec->buckets) &&
	(rrec->hashsize < MAXHASHSIZE))
	RebuildTable(client);
    head = &rrec->resources[Hash(client, id)];
    res = (ResourcePtr)malloc(sizeof(ResourceRec));
    if (!res)
    {
	(*DeleteFuncs[type & TypeMask])(value, id);
	return FALSE;
    }
    res->next = *head;
    res->id = id;
    res->type = type;
    res->value = value;
    *head = res;
    rrec->elements++;
    #ifdef NXAGENT_SERVER
    nxagentResChangedFlag = 1;
    #endif
    if (!(id & SERVER_BIT) && (id >= rrec->expectID))
	rrec->expectID = id + 1;
    return TRUE;
}

void
FreeResource(XID id, RESTYPE skipDeleteFuncType)
{
    int		cid;
    register    ResourcePtr res;
    register	ResourcePtr *prev, *head;
    register	int *eltptr;
    int		elements;
    Bool	gotOne = FALSE;

#ifdef NXAGENT_SERVER

    #ifdef TEST
    fprintf(stderr, "FreeResource: Freeing resource id [%lu].\n", (unsigned long) id);
    #endif

#endif

    if (((cid = CLIENT_ID(id)) < MAXCLIENTS) && clientTable[cid].buckets)
    {
	head = &clientTable[cid].resources[Hash(cid, id)];
	eltptr = &clientTable[cid].elements;

	prev = head;
	while ( (res = *prev) )
	{
	    if (res->id == id)
	    {
		RESTYPE rtype = res->type;
		*prev = res->next;
		elements = --*eltptr;
                #ifdef NXAGENT_SERVER
                nxagentResChangedFlag = 1;
                #endif
		if (rtype != skipDeleteFuncType)
		    (*DeleteFuncs[rtype & TypeMask])(res->value, res->id);
		free(res);
		if (*eltptr != elements)
		    prev = head; /* prev may no longer be valid */
		gotOne = TRUE;
	    }
	    else
		prev = &res->next;
        }
    }
    if (!gotOne)
	ErrorF("Freeing resource id=%lX which isn't there.\n",
		   (unsigned long)id);
}


void
FreeResourceByType(XID id, RESTYPE type, Bool skipFree)
{
    int		cid;
    register    ResourcePtr res;
    register	ResourcePtr *prev, *head;
    if (((cid = CLIENT_ID(id)) < MAXCLIENTS) && clientTable[cid].buckets)
    {
	head = &clientTable[cid].resources[Hash(cid, id)];

	prev = head;
	while ( (res = *prev) )
	{
	    if (res->id == id && res->type == type)
	    {
		*prev = res->next;
                #ifdef NXAGENT_SERVER
                nxagentResChangedFlag = 1;
                #endif
		if (!skipFree)
		    (*DeleteFuncs[type & TypeMask])(res->value, res->id);
		free(res);
		break;
	    }
	    else
		prev = &res->next;
        }
    }
}

/* Note: if func adds or deletes resources, then func can get called
 * more than once for some resources.  If func adds new resources,
 * func might or might not get called for them.  func cannot both
 * add and delete an equal number of resources!
 */

void
FindClientResourcesByType(
    ClientPtr client,
    RESTYPE type,
    FindResType func,
    void * cdata
){
    register ResourcePtr *resources;
    register ResourcePtr this, next;
    int i, elements;
    register int *eltptr;

    #ifdef NXAGENT_SERVER
    register ResourcePtr **resptr;
    #endif

    if (!client)
	client = serverClient;

/*
 * If func triggers a resource table
 * rebuild then restart the loop.
 */

#ifdef NXAGENT_SERVER
RestartLoop:
#endif

    resources = clientTable[client->index].resources;

    #ifdef NXAGENT_SERVER
    resptr = &clientTable[client->index].resources;
    #endif

    eltptr = &clientTable[client->index].elements;
    for (i = 0; i < clientTable[client->index].buckets; i++) 
    {
        for (this = resources[i]; this; this = next)
	{
	    next = this->next;
	    if (!type || this->type == type) {
		elements = *eltptr;

                /*
                 * FIXME:
                 * It is not safe to let a function change the resource
                 * table we are reading!
                 */

                #ifdef NXAGENT_SERVER
                nxagentResChangedFlag = 0;
                #endif
		(*func)(this->value, this->id, cdata);

                /*
                 * Avoid that a call to RebuildTable() could invalidate the
                 * pointer. This is safe enough, because in RebuildTable()
                 * the new pointer is allocated just before the old one is
                 * freed, so it can't point to the same address.
                 */

                #ifdef NXAGENT_SERVER
                if (*resptr != resources)
                   goto RestartLoop;
                #endif

                /*
                 * It's not enough to check if the number of elements has
                 * changed, beacause it could happen that the number of
                 * resources that have been added matches the number of
                 * the freed ones.
                 * 'nxagentResChangedFlag' is set if a resource has been
                 * added or freed.
                 */

                #ifdef NXAGENT_SERVER
                if (*eltptr != elements || nxagentResChangedFlag)
                #else
		if (*eltptr != elements)
                #endif
		    next = resources[i]; /* start over */
	    }
	}
    }
}

void
FindAllClientResources(
    ClientPtr client,
    FindAllRes func,
    void * cdata
){
    register ResourcePtr *resources;
    register ResourcePtr this, next;
    int i, elements;
    register int *eltptr;

    #ifdef NXAGENT_SERVER
    register ResourcePtr **resptr;
    #endif

    if (!client)
        client = serverClient;

/*
 * If func triggers a resource table
 * rebuild then restart the loop.
 */

#ifdef NXAGENT_SERVER
RestartLoop:
#endif

    resources = clientTable[client->index].resources;

    #ifdef NXAGENT_SERVER
    resptr = &clientTable[client->index].resources;
    #endif

    eltptr = &clientTable[client->index].elements;
    for (i = 0; i < clientTable[client->index].buckets; i++)
    {
        for (this = resources[i]; this; this = next)
        {
            next = this->next;
            elements = *eltptr;

            /*
             * FIXME:
             * It is not safe to let a function change the resource
             * table we are reading!
             */

            #ifdef NXAGENT_SERVER
            nxagentResChangedFlag = 0;
            #endif
            (*func)(this->value, this->id, this->type, cdata);

            /*
             * Avoid that a call to RebuildTable() could invalidate the
             * pointer. This is safe enough, because in RebuildTable()
             * the new pointer is allocated just before the old one is
             * freed, so it can't point to the same address.
             */

            #ifdef NXAGENT_SERVER
            if (*resptr != resources)
                goto RestartLoop;
            #endif

            /*
             * It's not enough to check if the number of elements has
             * changed, beacause it could happen that the number of
             * resources that have been added matches the number of
             * the freed ones.
             * 'nxagentResChangedFlag' is set if a resource has been
             * added or freed.
             */

            #ifdef NXAGENT_SERVER
            if (*eltptr != elements || nxagentResChangedFlag)
            #else
            if (*eltptr != elements)
            #endif
                next = resources[i]; /* start over */
        }
    }
}


void *
LookupClientResourceComplex(
    ClientPtr client,
    RESTYPE type,
    FindComplexResType func,
    void * cdata
){
    ResourcePtr *resources;
    ResourcePtr this;
    int i;

    #ifdef NXAGENT_SERVER
    ResourcePtr **resptr;
    Bool res;
    #endif

    if (!client)
	client = serverClient;

/*
 * If func triggers a resource table
 * rebuild then restart the loop.
 */

#ifdef NXAGENT_SERVER
RestartLoop:
#endif

    resources = clientTable[client->index].resources;

    #ifdef NXAGENT_SERVER
    resptr = &clientTable[client->index].resources;
    #endif

    for (i = 0; i < clientTable[client->index].buckets; i++) {
        for (this = resources[i]; this; this = this->next) {
	    if (!type || this->type == type) {
                #ifdef NXAGENT_SERVER
                res = (*func)(this->value, this->id, cdata);

                if (*resptr != resources)
                    goto RestartLoop;

                if (res)
                    return this->value;
                #else
		if((*func)(this->value, this->id, cdata))
		    return this->value;
                #endif
	    }
	}
    }
    return NULL;
}
