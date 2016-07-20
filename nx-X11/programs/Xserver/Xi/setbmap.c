
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
 * Request to change the button mapping of an extension device.
 *
 */

#define IsOn(ptr, bit) \
	(((BYTE *) (ptr))[(bit)>>3] & (1 << ((bit) & 7)))

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/X.h>				/* for inputstr.h    */
#include <X11/Xproto.h>			/* Request macro     */
#include "inputstr.h"			/* DeviceIntPtr	     */
#include <X11/extensions/XI.h>
#include <X11/extensions/XIproto.h>
#include "exevents.h"
#include "extnsionst.h"
#include "extinit.h"			/* LookupDeviceIntRec */
#include "exglobals.h"

#include "setbmap.h"

/***********************************************************************
 *
 * This procedure changes the button mapping.
 *
 */

int
SProcXSetDeviceButtonMapping(client)
    register ClientPtr client;
    {
    REQUEST(xSetDeviceButtonMappingReq);
    swaps(&stuff->length);
    return(ProcXSetDeviceButtonMapping(client));
    }

/***********************************************************************
 *
 * This procedure lists the input devices available to the server.
 *
 */

int
ProcXSetDeviceButtonMapping (client)
    register ClientPtr client;
    {
    int					ret;
    xSetDeviceButtonMappingReply	rep;
    DeviceIntPtr dev;

    REQUEST(xSetDeviceButtonMappingReq);
    REQUEST_AT_LEAST_SIZE(xSetDeviceButtonMappingReq);

    if (stuff->length != (sizeof(xSetDeviceButtonMappingReq) + 
	stuff->map_length + 3)>>2)
	{
	SendErrorToClient(client, IReqCode, X_SetDeviceButtonMapping, 0, 
		BadLength);
	return Success;
	}

    rep.repType = X_Reply;
    rep.RepType = X_SetDeviceButtonMapping;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    rep.status = MappingSuccess;

    dev = LookupDeviceIntRec (stuff->deviceid);
    if (dev == NULL)
	{
	SendErrorToClient(client, IReqCode, X_SetDeviceButtonMapping, 0, 
		BadDevice);
	return Success;
	}

    ret = SetButtonMapping (client, dev, stuff->map_length, (BYTE *)&stuff[1]);

    if (ret == BadValue || ret == BadMatch)
	{
	SendErrorToClient(client, IReqCode, X_SetDeviceButtonMapping, 0, 
		ret);
	return Success;
	}
    else
	{
	rep.status = ret;
	WriteReplyToClient(client, sizeof(xSetDeviceButtonMappingReply), &rep);
	}

    if (ret != MappingBusy)
        SendDeviceMappingNotify(MappingPointer, 0, 0, dev);
    return Success;
    }

/***********************************************************************
 *
 * This procedure writes the reply for the XSetDeviceButtonMapping function,
 * if the client and server have a different byte ordering.
 *
 */

void
SRepXSetDeviceButtonMapping (client, size, rep)
    ClientPtr	client;
    int		size;
    xSetDeviceButtonMappingReply	*rep;
    {
    swaps(&rep->sequenceNumber);
    swapl(&rep->length);
    WriteToClient(client, size, rep);
    }
