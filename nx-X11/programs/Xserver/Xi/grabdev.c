
/************************************************************

Copyright 1989, 1998  The Open Group

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

Copyright 1989 by Hewlett-Packard Company, Palo Alto, California.

			All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Hewlett-Packard not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

HEWLETT-PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
HEWLETT-PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

********************************************************/

/***********************************************************************
 *
 * Extension function to grab an extension device.
 *
 */


#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/X.h>				/* for inputstr.h    */
#include <X11/Xproto.h>			/* Request macro     */
#include "inputstr.h"			/* DeviceIntPtr	     */
#include "windowstr.h"			/* window structure  */
#include <nx-X11/extensions/XI.h>
#include <nx-X11/extensions/XIproto.h>
#include "extnsionst.h"
#include "extinit.h"			/* LookupDeviceIntRec */
#include "exglobals.h"
#include "dixevents.h"			/* GrabDevice */

#include "grabdev.h"

extern	XExtEventInfo	EventInfo[];
extern int		ExtEventIndex;

/***********************************************************************
 *
 * Swap the request if the requestor has a different byte order than us.
 *
 */

int
SProcXGrabDevice(client)
    register ClientPtr client;
    {
    REQUEST(xGrabDeviceReq);
    swaps(&stuff->length);
    REQUEST_AT_LEAST_SIZE(xGrabDeviceReq);
    swapl(&stuff->grabWindow);
    swapl(&stuff->time);
    swaps(&stuff->event_count);

    if (stuff->length != (sizeof(xGrabDeviceReq) >> 2) + stuff->event_count)
       return BadLength;
    
    SwapLongs((CARD32 *) (&stuff[1]), stuff->event_count);

    return(ProcXGrabDevice(client));
    }

/***********************************************************************
 *
 * Grab an extension device.
 *
 */

int
ProcXGrabDevice(client)
    ClientPtr client;
    {
    int			error;
    xGrabDeviceReply 	rep;
    DeviceIntPtr 	dev;
    struct tmask	tmp[EMASKSIZE];

    REQUEST(xGrabDeviceReq);
    REQUEST_AT_LEAST_SIZE(xGrabDeviceReq);

    if (stuff->length !=(sizeof(xGrabDeviceReq)>>2) + stuff->event_count)
	{
	SendErrorToClient (client, IReqCode, X_GrabDevice, 0, BadLength);
	return Success;
	}

    rep.repType = X_Reply;
    rep.RepType = X_GrabDevice;
    rep.sequenceNumber = client->sequence;
    rep.length = 0;

    dev = LookupDeviceIntRec (stuff->deviceid);
    if (dev == NULL)
	{
	SendErrorToClient(client, IReqCode, X_GrabDevice, 0, BadDevice);
	return Success;
	}

    if (CreateMaskFromList (client, (XEventClass *)&stuff[1], 
	stuff->event_count, tmp, dev, X_GrabDevice) != Success)
	return Success;

    error = GrabDevice (client, dev, stuff->this_device_mode, 
	stuff->other_devices_mode, stuff->grabWindow, stuff->ownerEvents, 
	stuff->time, tmp[stuff->deviceid].mask, &rep.status);

    if (error != Success)
	{
	SendErrorToClient(client, IReqCode, X_GrabDevice, 0, error);
	return Success;
	}
    WriteReplyToClient(client, sizeof(xGrabDeviceReply), &rep);
    return Success;
    }


/***********************************************************************
 *
 * This procedure creates an event mask from a list of XEventClasses.
 *
 */

int
CreateMaskFromList (client, list, count, mask, dev, req)
    ClientPtr		client;
    XEventClass		*list;
    int			count;
    struct tmask	mask[];
    DeviceIntPtr	dev;
    int			req;
    {
    int			i,j;
    int			device;
    DeviceIntPtr	tdev;

    for (i=0; i<EMASKSIZE; i++)
	{
	mask[i].mask = 0;
	mask[i].dev = NULL;
	}

    for (i=0; i<count; i++, list++)
	{
	device = *list >> 8;
	if (device > 255)
	    {
	    SendErrorToClient(client, IReqCode, req, 0, BadClass);
	    return BadClass;
	    }
	tdev = LookupDeviceIntRec (device);
	if (tdev==NULL || (dev != NULL && tdev != dev))
	    {
	    SendErrorToClient(client, IReqCode, req, 0, BadClass);
	    return BadClass;
	    }

	for (j=0; j<ExtEventIndex; j++)
	    if (EventInfo[j].type == (*list & 0xff))
		{
		mask[device].mask |= EventInfo[j].mask;
		mask[device].dev = (Pointer) tdev;
		break;
		}
	}
    return Success;
    }

/***********************************************************************
 *
 * This procedure writes the reply for the XGrabDevice function,
 * if the client and server have a different byte ordering.
 *
 */

void
SRepXGrabDevice (client, size, rep)
    ClientPtr	client;
    int		size;
    xGrabDeviceReply	*rep;
    {
    swaps(&rep->sequenceNumber);
    swapl(&rep->length);
    WriteToClient(client, size, rep);
    }
