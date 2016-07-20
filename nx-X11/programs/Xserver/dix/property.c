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

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/X.h>
#include <X11/Xproto.h>
#include "windowstr.h"
#include "propertyst.h"
#include "dixstruct.h"
#include "dispatch.h"
#include "swaprep.h"
#ifdef XCSECURITY
#define _SECURITY_SERVER
#include <nx-X11/extensions/security.h>
#endif

/*****************************************************************
 * Property Stuff
 *
 *    ChangeProperty, DeleteProperty, GetProperties,
 *    ListProperties
 *
 *   Properties below to windows.  A allocate slots each time
 *   a property is added.  No fancy searching done.
 *
 *****************************************************************/

#ifdef notdef
static void
PrintPropertys(WindowPtr pWin)
{
    PropertyPtr pProp;
    register int j;

    pProp = pWin->userProps;
    while (pProp)
    {
        ErrorF(  "%x %x\n", pProp->propertyName, pProp->type);
        ErrorF("property format: %d\n", pProp->format);
        ErrorF("property data: \n");
        for (j=0; j<(pProp->format/8)*pProp->size; j++)
           ErrorF("%c\n", pProp->data[j]);
        pProp = pProp->next;
    }
}
#endif

int
ProcRotateProperties(ClientPtr client)
{
    int     i, j, delta;
    REQUEST(xRotatePropertiesReq);
    WindowPtr pWin;
    register    Atom * atoms;
    PropertyPtr * props;               /* array of pointer */
    PropertyPtr pProp;
    xEvent event;

    REQUEST_FIXED_SIZE(xRotatePropertiesReq, stuff->nAtoms << 2);
    UpdateCurrentTime();
    pWin = (WindowPtr) SecurityLookupWindow(stuff->window, client,
					    SecurityWriteAccess);
    if (!pWin)
        return(BadWindow);
    if (!stuff->nAtoms)
	return(Success);
    atoms = (Atom *) & stuff[1];
    props = (PropertyPtr *)ALLOCATE_LOCAL(stuff->nAtoms * sizeof(PropertyPtr));
    if (!props)
	return(BadAlloc);
    for (i = 0; i < stuff->nAtoms; i++)
    {
#ifdef XCSECURITY
	char action = SecurityCheckPropertyAccess(client, pWin, atoms[i],
				SecurityReadAccess|SecurityWriteAccess);
#endif
        if (!ValidAtom(atoms[i])
#ifdef XCSECURITY
	    || (SecurityErrorOperation == action)
#endif
	   )
        {
            DEALLOCATE_LOCAL(props);
	    client->errorValue = atoms[i];
            return BadAtom;
        }
#ifdef XCSECURITY
	if (SecurityIgnoreOperation == action)
        {
            DEALLOCATE_LOCAL(props);
	    return Success;
	}
#endif
        for (j = i + 1; j < stuff->nAtoms; j++)
            if (atoms[j] == atoms[i])
            {
                DEALLOCATE_LOCAL(props);
                return BadMatch;
            }
        pProp = wUserProps (pWin);
        while (pProp)
        {
            if (pProp->propertyName == atoms[i])
                goto found;
	    pProp = pProp->next;
        }
        DEALLOCATE_LOCAL(props);
        return BadMatch;
found: 
        props[i] = pProp;
    }
    delta = stuff->nPositions;

    /* If the rotation is a complete 360 degrees, then moving the properties
	around and generating PropertyNotify events should be skipped. */

    if ( (stuff->nAtoms != 0) && (abs(delta) % stuff->nAtoms) != 0 ) 
    {
	while (delta < 0)                  /* faster if abs value is small */
            delta += stuff->nAtoms;
    	for (i = 0; i < stuff->nAtoms; i++)
 	{
	    /* Generate a PropertyNotify event for each property whose value
		is changed in the order in which they appear in the request. */
 
 	    event.u.u.type = PropertyNotify;
            event.u.property.window = pWin->drawable.id;
    	    event.u.property.state = PropertyNewValue;
	    event.u.property.atom = props[i]->propertyName;	
	    event.u.property.time = currentTime.milliseconds;
	    DeliverEvents(pWin, &event, 1, (WindowPtr)NULL);
	
            props[i]->propertyName = atoms[(i + delta) % stuff->nAtoms];
	}
    }
    DEALLOCATE_LOCAL(props);
    return Success;
}

#ifndef NXAGENT_SERVER
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

    pWin = (WindowPtr)SecurityLookupWindow(stuff->window, client,
					   SecurityWriteAccess);
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
					SecurityWriteAccess))
    {
	case SecurityErrorOperation:
	    client->errorValue = stuff->property;
	    return BadAtom;
	case SecurityIgnoreOperation:
	    return Success;
    }
#endif

    err = ChangeWindowProperty(pWin, stuff->property, stuff->type, (int)format,
			       (int)mode, len, (void *)&stuff[1], TRUE);
    if (err != Success)
	return err;
    else
	return client->noClientException;
}

int
ChangeWindowProperty(WindowPtr pWin, Atom property, Atom type, int format, 
                     int mode, unsigned long len, void * value, 
                     Bool sendevent)
{
    PropertyPtr pProp;
    xEvent event;
    int sizeInBytes;
    int totalSize;
    void * data;

    sizeInBytes = format>>3;
    totalSize = len * sizeInBytes;

    /* first see if property already exists */

    pProp = wUserProps (pWin);
    while (pProp)
    {
	if (pProp->propertyName == property)
	    break;
	pProp = pProp->next;
    }
    if (!pProp)   /* just add to list */
    {
	if (!pWin->optional && !MakeWindowOptional (pWin))
	    return(BadAlloc);
        pProp = (PropertyPtr)malloc(sizeof(PropertyRec));
	if (!pProp)
	    return(BadAlloc);
        data = (void *)malloc(totalSize);
	if (!data && len)
	{
	    free(pProp);
	    return(BadAlloc);
	}
        pProp->propertyName = property;
        pProp->type = type;
        pProp->format = format;
        pProp->data = data;
	if (len)
	    memmove((char *)data, (char *)value, totalSize);
	pProp->size = len;
        pProp->next = pWin->optional->userProps;
        pWin->optional->userProps = pProp;
    }
    else
    {
	/* To append or prepend to a property the request format and type
		must match those of the already defined property.  The
		existing format and type are irrelevant when using the mode
		"PropModeReplace" since they will be written over. */

        if ((format != pProp->format) && (mode != PropModeReplace))
	    return(BadMatch);
        if ((pProp->type != type) && (mode != PropModeReplace))
            return(BadMatch);
        if (mode == PropModeReplace)
        {
	    if (totalSize != pProp->size * (pProp->format >> 3))
	    {
		data = (void *)realloc(pProp->data, totalSize);
	    	if (!data && len)
		    return(BadAlloc);
            	pProp->data = data;
	    }
	    if (len)
		memmove((char *)pProp->data, (char *)value, totalSize);
	    pProp->size = len;
    	    pProp->type = type;
	    pProp->format = format;
	}
	else if (len == 0)
	{
	    /* do nothing */
	}
        else if (mode == PropModeAppend)
        {
	    data = (void *)realloc(pProp->data,
				     sizeInBytes * (len + pProp->size));
	    if (!data)
		return(BadAlloc);
            pProp->data = data;
	    memmove(&((char *)data)[pProp->size * sizeInBytes], 
		    (char *)value,
		  totalSize);
            pProp->size += len;
	}
        else if (mode == PropModePrepend)
        {
            data = (void *)malloc(sizeInBytes * (len + pProp->size));
	    if (!data)
		return(BadAlloc);
	    memmove(&((char *)data)[totalSize], (char *)pProp->data, 
		  (int)(pProp->size * sizeInBytes));
            memmove((char *)data, (char *)value, totalSize);
	    free(pProp->data);
            pProp->data = data;
            pProp->size += len;
	}
    }
    if (sendevent)
    {
	event.u.u.type = PropertyNotify;
	event.u.property.window = pWin->drawable.id;
	event.u.property.state = PropertyNewValue;
	event.u.property.atom = pProp->propertyName;
	event.u.property.time = currentTime.milliseconds;
	DeliverEvents(pWin, &event, 1, (WindowPtr)NULL);
    }
    return(Success);
}
#endif /* NXAGENT_SERVER */

int
DeleteProperty(WindowPtr pWin, Atom propName)
{
    PropertyPtr pProp, prevProp;
    xEvent event;

    if (!(pProp = wUserProps (pWin)))
	return(Success);
    prevProp = (PropertyPtr)NULL;
    while (pProp)
    {
	if (pProp->propertyName == propName)
	    break;
        prevProp = pProp;
	pProp = pProp->next;
    }
    if (pProp) 
    {		    
        if (prevProp == (PropertyPtr)NULL)      /* takes care of head */
        {
            if (!(pWin->optional->userProps = pProp->next))
		CheckWindowOptionalNeed (pWin);
        }
	else
        {
            prevProp->next = pProp->next;
        }
	event.u.u.type = PropertyNotify;
	event.u.property.window = pWin->drawable.id;
	event.u.property.state = PropertyDelete;
        event.u.property.atom = pProp->propertyName;
	event.u.property.time = currentTime.milliseconds;
	DeliverEvents(pWin, &event, 1, (WindowPtr)NULL);
	free(pProp->data);
        free(pProp);
    }
    return(Success);
}

void
DeleteAllWindowProperties(WindowPtr pWin)
{
    PropertyPtr pProp, pNextProp;
    xEvent event;

    pProp = wUserProps (pWin);
    while (pProp)
    {
	event.u.u.type = PropertyNotify;
	event.u.property.window = pWin->drawable.id;
	event.u.property.state = PropertyDelete;
	event.u.property.atom = pProp->propertyName;
	event.u.property.time = currentTime.milliseconds;
	DeliverEvents(pWin, &event, 1, (WindowPtr)NULL);
	pNextProp = pProp->next;
        free(pProp->data);
        free(pProp);
	pProp = pNextProp;
    }
}

static int
NullPropertyReply(
    ClientPtr client,
    ATOM propertyType,
    int format,
    xGetPropertyReply *reply)
{
    reply->nItems = 0;
    reply->length = 0;
    reply->bytesAfter = 0;
    reply->propertyType = propertyType;
    reply->format = format;
    WriteReplyToClient(client, sizeof(xGenericReply), reply);
    return(client->noClientException);
}

#ifndef NXAGENT_SERVER

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
					   SecurityReadAccess);
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
    if (!pProp) 
	return NullPropertyReply(client, None, 0, &reply);

#ifdef XCSECURITY
    {
	Mask access_mode = SecurityReadAccess;

	if (stuff->delete)
	    access_mode |= SecurityDestroyAccess;
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
#endif /* NXAGENT_SERVER */

int
ProcListProperties(ClientPtr client)
{
    Atom *pAtoms = NULL, *temppAtoms;
    xListPropertiesReply xlpr;
    int	numProps = 0;
    WindowPtr pWin;
    PropertyPtr pProp;
    REQUEST(xResourceReq);

    REQUEST_SIZE_MATCH(xResourceReq);
    pWin = (WindowPtr)SecurityLookupWindow(stuff->id, client,
					   SecurityReadAccess);
    if (!pWin)
        return(BadWindow);

    pProp = wUserProps (pWin);
    while (pProp)
    {        
        pProp = pProp->next;
	numProps++;
    }
    if (numProps)
        if(!(pAtoms = (Atom *)ALLOCATE_LOCAL(numProps * sizeof(Atom))))
            return(BadAlloc);

    xlpr.type = X_Reply;
    xlpr.nProperties = numProps;
    xlpr.length = (numProps * sizeof(Atom)) >> 2;
    xlpr.sequenceNumber = client->sequence;
    pProp = wUserProps (pWin);
    temppAtoms = pAtoms;
    while (pProp)
    {
	*temppAtoms++ = pProp->propertyName;
	pProp = pProp->next;
    }
    WriteReplyToClient(client, sizeof(xGenericReply), &xlpr);
    if (numProps)
    {
        client->pSwapReplyFunc = (ReplySwapPtr)Swap32Write;
        WriteSwappedDataToClient(client, numProps * sizeof(Atom), pAtoms);
        DEALLOCATE_LOCAL(pAtoms);
    }
    return(client->noClientException);
}

int 
ProcDeleteProperty(register ClientPtr client)
{
    WindowPtr pWin;
    REQUEST(xDeletePropertyReq);
    int result;
              
    REQUEST_SIZE_MATCH(xDeletePropertyReq);
    UpdateCurrentTime();
    pWin = (WindowPtr)SecurityLookupWindow(stuff->window, client,
					   SecurityWriteAccess);
    if (!pWin)
        return(BadWindow);
    if (!ValidAtom(stuff->property))
    {
	client->errorValue = stuff->property;
	return (BadAtom);
    }

#ifdef XCSECURITY
    switch(SecurityCheckPropertyAccess(client, pWin, stuff->property,
				       SecurityDestroyAccess))
    {
	case SecurityErrorOperation:
	    client->errorValue = stuff->property;
	    return BadAtom;;
	case SecurityIgnoreOperation:
	    return Success;
    }
#endif

    result = DeleteProperty(pWin, stuff->property);
    if (client->noClientException != Success)
	return(client->noClientException);
    else
	return(result);
}
