/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2011 NoMachine (http://www.nomachine.com)          */
/* Copyright (c) 2008-2017 Oleksandr Shneyder <o.shneyder@phoca-gmbh.de>  */
/* Copyright (c) 2011-2022 Mike Gabriel <mike.gabriel@das-netzwerkteam.de>*/
/* Copyright (c) 2014-2019 Mihai Moldovan <ionic@ionic.de>                */
/* Copyright (c) 2014-2022 Ulrich Sibiller <uli42@gmx.de>                 */
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

#include "../../dix/property.c"

#include "Options.h"
#include "Rootless.h"
#include "Client.h"
#include "Windows.h"
#include "Clipboard.h"

extern Atom clientCutProperty;

#ifdef NXAGENT_SERVER
typedef struct
{
  CARD32 state;
  Window icon;
}
nxagentWMStateRec;
#endif

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

#ifdef NXAGENT_ARTSD
extern Atom mcop_local_atom;
#endif

#ifdef NX_DEBUG_INPUT
extern void nxagentGuessDumpInputInfo(ClientPtr client, Atom property, char *data);
#endif

int 
ProcChangeProperty(ClientPtr client)
{	      
    WindowPtr pWin;
    char format, mode;
    unsigned long len;
    int sizeInBytes;
    int totalSize;
    int err;
    REQUEST(xChangePropertyReq);

    REQUEST_AT_LEAST_SIZE(xChangePropertyReq);
    UpdateCurrentTime();
    format = stuff->format;
    mode = stuff->mode;
    if ((mode != PropModeReplace) && (mode != PropModeAppend) &&
	(mode != PropModePrepend))
    {
	client->errorValue = mode;
	return BadValue;
    }
    if ((format != 8) && (format != 16) && (format != 32))
    {
	client->errorValue = format;
        return BadValue;
    }
    len = stuff->nUnits;
    if (len > ((0xffffffff - sizeof(xChangePropertyReq)) >> 2))
	return BadLength;
    sizeInBytes = format>>3;
    totalSize = len * sizeInBytes;
    REQUEST_FIXED_SIZE(xChangePropertyReq, totalSize);

#ifdef NXAGENT_CLIPBOARD
    pWin = nxagentGetClipboardWindow(stuff->property);

    if (pWin == NULL)
#endif
    pWin = (WindowPtr)SecurityLookupWindow(stuff->window, client,
					   DixWriteAccess);
    if (!pWin)
	return(BadWindow);
    if (!ValidAtom(stuff->property))
    {
	client->errorValue = stuff->property;
	return(BadAtom);
    }
    if (!ValidAtom(stuff->type))
    {
	client->errorValue = stuff->type;
	return(BadAtom);
    }

#ifdef XCSECURITY
    switch (SecurityCheckPropertyAccess(client, pWin, stuff->property,
					DixWriteAccess))
    {
	case SecurityErrorOperation:
	    client->errorValue = stuff->property;
	    return BadAtom;
	case SecurityIgnoreOperation:
	    return Success;
    }
#endif

#ifdef NXAGENT_ARTSD
    {
    /* Do not process MCOPGLOBALS property changes,
      they are already set reflecting the server side settings.
      Just return success.
    */
      if (stuff->property == mcop_local_atom)
        return client->noClientException;
    }
#endif

#ifdef NXAGENT_SERVER
    /* prevent clients from changing the NX_AGENT_VERSION property */
    if (stuff->property == MakeAtom("NX_AGENT_VERSION", strlen("NX_AGENT_VERSION"), True))
      return client->noClientException;
#endif

    err = ChangeWindowProperty(pWin, stuff->property, stuff->type, (int)format,
			       (int)mode, len, (void *)&stuff[1], TRUE);
    if (err != Success)
	return err;
    else
    {
      if (nxagentOption(Rootless))
      {
        nxagentExportProperty(pWin, stuff->property, stuff->type, (int) format,
                                  (int) mode, len, (void *) &stuff[1]);
      }

      nxagentGuessClientHint(client, stuff->property, (char *) &stuff[1]);

      nxagentGuessShadowHint(client, stuff->property);

      #ifdef NX_DEBUG_INPUT
      nxagentGuessDumpInputInfo(client, stuff->property, (char *) &stuff[1]);
      #endif

      return client->noClientException;
    }
}

/*****************
 * GetProperty
 *    If type Any is specified, returns the property from the specified
 *    window regardless of its type.  If a type is specified, returns the
 *    property only if its type equals the specified type.
 *    If delete is True and a property is returned, the property is also
 *    deleted from the window and a PropertyNotify event is generated on the
 *    window.
 *****************/

int
ProcGetProperty(ClientPtr client)
{
    PropertyPtr pProp, prevProp;
    unsigned long n, len, ind;
    WindowPtr pWin;
    xGetPropertyReply reply;
    REQUEST(xGetPropertyReq);

    REQUEST_SIZE_MATCH(xGetPropertyReq);
    if (stuff->delete)
	UpdateCurrentTime();
    pWin = (WindowPtr)SecurityLookupWindow(stuff->window, client,
					   DixReadAccess);
    if (!pWin)
	return BadWindow;

    if (!ValidAtom(stuff->property))
    {
	client->errorValue = stuff->property;
	return(BadAtom);
    }
    if ((stuff->delete != xTrue) && (stuff->delete != xFalse))
    {
	client->errorValue = stuff->delete;
	return(BadValue);
    }
    if ((stuff->type != AnyPropertyType) && !ValidAtom(stuff->type))
    {
	client->errorValue = stuff->type;
	return(BadAtom);
    }

    pProp = wUserProps (pWin);
    prevProp = (PropertyPtr)NULL;
    while (pProp)
    {
	if (pProp->propertyName == stuff->property) 
	    break;
	prevProp = pProp;
	pProp = pProp->next;
    }

    memset(&reply, 0, sizeof(xGetPropertyReply));
    reply.type = X_Reply;
    reply.sequenceNumber = client->sequence;

    #ifdef NXAGENT_SERVER

    /*
     * Creating a reply for WM_STATE property if it doesn't exist.
     * This is intended to allow drag & drop work in JAva 1.6 when
     * the agent is connected to NXWin in multiwindow mode.
     */

    if (nxagentOption(Rootless) &&
            nxagentWindowTopLevel(pWin) &&
                (!pProp) &&
                    strcmp(NameForAtom(stuff->property), "WM_STATE") == 0)
    {
      nxagentWMStateRec wmState;
      nxagentWMStateRec *wmsP = &wmState;

      memset(&wmState, 0, sizeof(nxagentWMStateRec));
      wmState.state = 1;
      wmState.icon = None;

      if (ChangeWindowProperty(pWin, stuff->property, stuff->property, 32, 0, 2, &wmState, 1) == Success)
      {
        nxagentExportProperty(pWin, stuff->property, stuff->property, 32, 0, 2, &wmState);
      }

      n = 8;
      ind = stuff->longOffset << 2;        

      if (n < ind)
      {
        client->errorValue = stuff->longOffset;
        return BadValue;
      }

      len = min(n - ind, 4 * stuff->longLength);

      reply.bytesAfter = n - (ind + len);
      reply.length = (len + 3) >> 2;

      reply.format = 32;
      reply.nItems = len / 4;
      reply.propertyType = stuff->property;

      WriteReplyToClient(client, sizeof(xGenericReply), &reply);

      if (len)
      {
        client->pSwapReplyFunc = (ReplySwapPtr)CopySwap32Write;

        WriteSwappedDataToClient(client, len, (char *)wmsP + ind);
      }

      return(client->noClientException);
    }
    #endif

    if (!pProp) 
	return NullPropertyReply(client, None, 0, &reply);

#ifdef XCSECURITY
    {
	Mask access_mode = DixReadAccess;

	if (stuff->delete)
	    access_mode |= DixDestroyAccess;
	switch(SecurityCheckPropertyAccess(client, pWin, stuff->property,
					   access_mode))
	{
	    case SecurityErrorOperation:
		client->errorValue = stuff->property;
		return BadAtom;;
	    case SecurityIgnoreOperation:
		return NullPropertyReply(client, pProp->type, pProp->format,
					 &reply);
	}
    }
#endif
    /* If the request type and actual type don't match. Return the
    property information, but not the data. */

    if (((stuff->type != pProp->type) &&
	 (stuff->type != AnyPropertyType))
       )
    {
	reply.bytesAfter = pProp->size;
	reply.format = pProp->format;
	reply.length = 0;
	reply.nItems = 0;
	reply.propertyType = pProp->type;
	WriteReplyToClient(client, sizeof(xGenericReply), &reply);
	return(Success);
    }

/*
 *  Return type, format, value to client
 */
    n = (pProp->format/8) * pProp->size; /* size (bytes) of prop */
    ind = stuff->longOffset << 2;        

   /* If longOffset is invalid such that it causes "len" to
	    be negative, it's a value error. */

    if (n < ind)
    {
	client->errorValue = stuff->longOffset;
	return BadValue;
    }

    len = min(n - ind, 4 * stuff->longLength);

    reply.bytesAfter = n - (ind + len);
    reply.format = pProp->format;
    reply.length = (len + 3) >> 2;
    reply.nItems = len / (pProp->format / 8 );
    reply.propertyType = pProp->type;

    if (stuff->delete && (reply.bytesAfter == 0))
    { /* send the event */
	xEvent event;

	memset(&event, 0, sizeof(xEvent));
	event.u.u.type = PropertyNotify;
	event.u.property.window = pWin->drawable.id;
	event.u.property.state = PropertyDelete;
	event.u.property.atom = pProp->propertyName;
	event.u.property.time = currentTime.milliseconds;
	DeliverEvents(pWin, &event, 1, (WindowPtr)NULL);
    }

    WriteReplyToClient(client, sizeof(xGenericReply), &reply);
    if (len)
    {
	switch (reply.format) {
	case 32: client->pSwapReplyFunc = (ReplySwapPtr)CopySwap32Write; break;
	case 16: client->pSwapReplyFunc = (ReplySwapPtr)CopySwap16Write; break;
	default: client->pSwapReplyFunc = (ReplySwapPtr)WriteToClient; break;
	}
	WriteSwappedDataToClient(client, len,
				 (char *)pProp->data + ind);
    }

    if (stuff->delete && (reply.bytesAfter == 0))
    { /* delete the Property */
	if (prevProp == (PropertyPtr)NULL) /* takes care of head */
	{
	    if (!(pWin->optional->userProps = pProp->next))
		CheckWindowOptionalNeed (pWin);
	}
	else
	    prevProp->next = pProp->next;
	free(pProp->data);
	free(pProp);
    }
    return(client->noClientException);
}

int
ProcDeleteProperty(register ClientPtr client)
{
    REQUEST(xDeletePropertyReq);
    REQUEST_SIZE_MATCH(xDeletePropertyReq);
    /* prevent clients from deleting the NX_AGENT_VERSION property */
    if (stuff->property == MakeAtom("NX_AGENT_VERSION", strlen("NX_AGENT_VERSION"), True))
      return client->noClientException;

    return xorg_ProcDeleteProperty(client);
}

/* ---------------------------------------------------------------------- */

/*
 * GetWindowProperty is the internal implementation of the
 * XGetWindowProperty() Xlib call. It is called from
 * Clipboard.c, Window.c and Events.c
 *
 * FIXME: should be moved to a different file, is not derived from
 * dix
 */
int
GetWindowProperty(WindowPtr pWin, Atom property, long longOffset,
                      long longLength, Bool delete, Atom type,
                          Atom *actualType, int *format, unsigned
                              long *nItems, unsigned long *bytesAfter,
                                  unsigned char **propData)
{
    if (!pWin)
    {
        #ifdef DEBUG
	fprintf(stderr, "%s: invalid window\n", __func__);
        #endif
	return BadWindow;
    }

    if (!ValidAtom(property))
    {
        #ifdef DEBUG
	fprintf(stderr, "%s: invalid atom [%d]\n", __func__, property);
        #endif
	return BadAtom;
    }

    if ((type != AnyPropertyType) && !ValidAtom(type))
    {
        #ifdef DEBUG
	fprintf(stderr, "%s: invalid type [%d]\n", __func__, type);
        #endif
	return BadAtom;
    }

    PropertyPtr pProp = wUserProps (pWin);
    PropertyPtr prevProp = (PropertyPtr)NULL;

    while (pProp)
    {
	if (pProp->propertyName == property)
	    break;
	prevProp = pProp;
	pProp = pProp->next;
    }

    if (!pProp)
    {
        #ifdef DEBUG
	fprintf(stderr, "%s: property not found [%d]\n", __func__, property);
        #endif
	return BadAtom;
    }

    /* If the request type and actual type don't match. Return the
       property information, but not the data. */

    if (((type != pProp->type) &&
	 (type != AnyPropertyType))
       )
    {
	*bytesAfter = pProp->size;
	*format = pProp->format;
	*nItems = 0;
	*actualType = pProp->type;
	return Success;
    }

    /*
     *  Return type, format, value to client
     */
    unsigned long n = (pProp->format/8) * pProp->size; /* size (bytes) of prop */
    unsigned long ind = longOffset << 2;               /* byte offset */

    /* If longOffset is invalid such that it causes "len" to
       be negative, it's a value error. */

    if (n < ind)
    {
        #ifdef DEBUG
	fprintf(stderr, "%s: negative property len\n", __func__);
        #endif
	return BadValue;
    }

    /* minimum of prop size and requested size */
    unsigned long len = min(n - ind, 4 * longLength);

    *bytesAfter = n - (ind + len);
    *format = pProp->format;
    *nItems = len / (pProp->format / 8 );
    *actualType = pProp->type;

    if (delete && (*bytesAfter == 0))
    { /* send the event */
	xEvent event = {0};
	event.u.u.type = PropertyNotify;
	event.u.property.window = pWin->drawable.id;
	event.u.property.state = PropertyDelete;
	event.u.property.atom = pProp->propertyName;
	event.u.property.time = currentTime.milliseconds;
	DeliverEvents(pWin, &event, 1, (WindowPtr)NULL);
    }

    if (len)
    {
	*propData = (unsigned char *)(pProp->data) + ind;
    }

    if (delete && (*bytesAfter == 0))
    { /* delete the property */
	if (prevProp == (PropertyPtr)NULL) /* takes care of head */
	{
	    if (!(pWin->optional->userProps = pProp->next))
		CheckWindowOptionalNeed (pWin);
	}
	else
	    prevProp->next = pProp->next;
	free(pProp->data);
	free(pProp);
    }
    return Success;
}

int
ChangeWindowProperty(WindowPtr pWin, Atom property, Atom type, int format,
                     int mode, unsigned long len, void * value,
                     Bool sendevent)
{
    int sizeInBytes = format>>3;
    int totalSize = len * sizeInBytes;
    int copySize = nxagentOption(CopyBufferSize);

    if (copySize != COPY_UNLIMITED && property == clientCutProperty)
    {
      if (totalSize > copySize)
      {
        totalSize = copySize;
        totalSize = totalSize - (totalSize % sizeInBytes);
        len = totalSize / sizeInBytes;
      }
    }

    return xorg_ChangeWindowProperty(pWin, property, type, format, mode, len, value, sendevent);
}
