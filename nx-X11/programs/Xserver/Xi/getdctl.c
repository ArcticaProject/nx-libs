
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

/********************************************************************
 *
 *  Get Device control attributes for an extension device.
 *
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/X.h>				/* for inputstr.h    */
#include <X11/Xproto.h>			/* Request macro     */
#include "inputstr.h"			/* DeviceIntPtr	     */
#include <X11/extensions/XI.h>
#include <X11/extensions/XIproto.h>
#include "extnsionst.h"
#include "extinit.h"			/* LookupDeviceIntRec */
#include "exglobals.h"

#include "getdctl.h"

/***********************************************************************
 *
 * This procedure gets the control attributes for an extension device,
 * for clients on machines with a different byte ordering than the server.
 *
 */

int
SProcXGetDeviceControl(client)
    register ClientPtr client;
    {
    REQUEST(xGetDeviceControlReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xGetDeviceControlReq);
    swaps(&stuff->control);
    return(ProcXGetDeviceControl(client));
    }

/***********************************************************************
 *
 * Get the state of the specified device control.
 *
 */

int
ProcXGetDeviceControl(client)
    ClientPtr client;
    {
    int	total_length = 0;
    char *buf, *savbuf;
    register DeviceIntPtr dev;
    xGetDeviceControlReply rep;

    REQUEST(xGetDeviceControlReq);
    REQUEST_SIZE_MATCH(xGetDeviceControlReq);

    dev = LookupDeviceIntRec (stuff->deviceid);
    if (dev == NULL)
	{
	SendErrorToClient (client, IReqCode, X_GetDeviceControl, 0, 
		BadDevice);
	return Success;
	}

    rep.repType = X_Reply;
    rep.RepType = X_GetDeviceControl;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;

    switch (stuff->control)
	{
	case DEVICE_RESOLUTION:
	    if (!dev->valuator)
		{
		SendErrorToClient (client, IReqCode, X_GetDeviceControl, 0, 
		    BadMatch);
		return Success;
		}
	    total_length = sizeof (xDeviceResolutionState) +
		(3 * sizeof(int) * dev->valuator->numAxes);
	    break;
	default:
	    SendErrorToClient (client, IReqCode, X_GetDeviceControl, 0, 
		BadValue);
	    return Success;
	}

    buf = (char *) malloc (total_length);
    if (!buf)
	{
	SendErrorToClient(client, IReqCode, X_GetDeviceControl, 0, 
		BadAlloc);
	return Success;
	}
    savbuf=buf;

    switch (stuff->control)
	{
	case DEVICE_RESOLUTION:
	    CopySwapDeviceResolution(client, dev->valuator, buf,
		total_length);
	    break;
	default:
	    break;
	}

    rep.length = (total_length+3) >> 2;
    WriteReplyToClient(client, sizeof(xGetDeviceControlReply), &rep);
    WriteToClient(client, total_length, savbuf);
    free (savbuf);
    return Success;
    }

/***********************************************************************
 *
 * This procedure copies DeviceResolution data, swapping if necessary.
 *
 */

void
CopySwapDeviceResolution (client, v, buf, length)
    ClientPtr 		client;
    ValuatorClassPtr	v;
    char 		*buf;
    int			length;
    {
    AxisInfoPtr	a;
    xDeviceResolutionState *r;
    int i, *iptr;

    r = (xDeviceResolutionState *) buf;
    r->control = DEVICE_RESOLUTION;
    r->length =  length;
    r->num_valuators =  v->numAxes;
    buf += sizeof (xDeviceResolutionState);
    iptr = (int *) buf;
    for (i=0,a=v->axes; i<v->numAxes; i++,a++)
	*iptr++ = a->resolution;
    for (i=0,a=v->axes; i<v->numAxes; i++,a++)
	*iptr++ = a->min_resolution;
    for (i=0,a=v->axes; i<v->numAxes; i++,a++)
	*iptr++ = a->max_resolution;
    if (client->swapped)
	{
	swaps (&r->control);
	swaps (&r->length);
	swapl (&r->num_valuators);
	iptr = (int *) buf;
	for (i=0; i < (3 * v->numAxes); i++,iptr++)
	    {
	    swapl (iptr);
	    }
	}
    }

/***********************************************************************
 *
 * This procedure writes the reply for the xGetDeviceControl function,
 * if the client and server have a different byte ordering.
 *
 */

void
SRepXGetDeviceControl (client, size, rep)
    ClientPtr	client;
    int		size;
    xGetDeviceControlReply	*rep;
    {
    swaps(&rep->sequenceNumber);
    swapl(&rep->length);
    WriteToClient(client, size, rep);
    }

