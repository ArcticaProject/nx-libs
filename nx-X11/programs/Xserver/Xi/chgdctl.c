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
 *  Change Device control attributes for an extension device.
 *
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <nx-X11/X.h>	/* for inputstr.h    */
#include <nx-X11/Xproto.h>	/* Request macro     */
#include "inputstr.h"	/* DeviceIntPtr      */
#include <nx-X11/extensions/XI.h>
#include <nx-X11/extensions/XIproto.h>	/* control constants */
#include "XIstubs.h"

#include "extnsionst.h"
#include "extinit.h"	/* LookupDeviceIntRec */
#include "exglobals.h"
#include "exevents.h"

#include "chgdctl.h"

/***********************************************************************
 *
 * This procedure changes the control attributes for an extension device,
 * for clients on machines with a different byte ordering than the server.
 *
 */

int
SProcXChangeDeviceControl(ClientPtr client)
{
    REQUEST(xChangeDeviceControlReq);
    swaps(&stuff->length);
    REQUEST_AT_LEAST_EXTRA_SIZE(xChangeDeviceControlReq, sizeof(xDeviceCtl));
    swaps(&stuff->control);
    return (ProcXChangeDeviceControl(client));
}

/***********************************************************************
 *
 * Change the control attributes.
 *
 */

int
ProcXChangeDeviceControl(ClientPtr client)
{
    unsigned len;
    int i, status, ret = BadValue;
    DeviceIntPtr dev;
    xDeviceResolutionCtl *r;
    xChangeDeviceControlReply rep;
    AxisInfoPtr a;
    CARD32 *resolution;
    xDeviceAbsCalibCtl *calib;
    xDeviceAbsAreaCtl *area;
    xDeviceCoreCtl *c;
    xDeviceEnableCtl *e;
    devicePresenceNotify dpn;

    REQUEST(xChangeDeviceControlReq);
    REQUEST_AT_LEAST_EXTRA_SIZE(xChangeDeviceControlReq, sizeof(xDeviceCtl));

    len = stuff->length - (sizeof(xChangeDeviceControlReq) >> 2);
    dev = LookupDeviceIntRec(stuff->deviceid);
    if (dev == NULL) {
        ret = BadDevice;
        goto out;
    }

    rep.repType = X_Reply;
    rep.RepType = X_ChangeDeviceControl;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;

    switch (stuff->control) {
    case DEVICE_RESOLUTION:
	r = (xDeviceResolutionCtl *) & stuff[1];
	if ((len < (sizeof(xDeviceResolutionCtl) >> 2)) ||
	    (len != (sizeof(xDeviceResolutionCtl) >> 2) + r->num_valuators)) {
            ret = BadLength;
            goto out;
	}
	if (!dev->valuator) {
            ret = BadMatch;
            goto out;
	}
	if ((dev->grab) && !SameClient(dev->grab, client)) {
	    rep.status = AlreadyGrabbed;
            ret = Success;
            goto out;
	}
	resolution = (CARD32 *) (r + 1);
	if (r->first_valuator + r->num_valuators > dev->valuator->numAxes) {
            ret = BadValue;
            goto out;
	}
	status = ChangeDeviceControl(client, dev, (xDeviceCtl *) r);
	if (status == Success) {
	    a = &dev->valuator->axes[r->first_valuator];
	    for (i = 0; i < r->num_valuators; i++)
		if (*(resolution + i) < (a + i)->min_resolution ||
		    *(resolution + i) > (a + i)->max_resolution) {
		    SendErrorToClient(client, IReqCode,
				      X_ChangeDeviceControl, 0, BadValue);
		    return Success;
		}
	    for (i = 0; i < r->num_valuators; i++)
		(a++)->resolution = *resolution++;

            ret = Success;
	} else if (status == DeviceBusy) {
	    rep.status = DeviceBusy;
            ret = Success;
	} else {
            ret = BadMatch;
	}
	break;
    case DEVICE_ABS_CALIB:
        calib = (xDeviceAbsCalibCtl *)&stuff[1];

        if (calib->button_threshold < 0 || calib->button_threshold > 255) {
            ret = BadValue;
            goto out;
        }

        status = ChangeDeviceControl(client, dev, (xDeviceCtl *) calib);

        if (status == Success) {
            dev->absolute->min_x = calib->min_x;
            dev->absolute->max_x = calib->max_x;
            dev->absolute->min_y = calib->min_y;
            dev->absolute->max_y = calib->max_y;
            dev->absolute->flip_x = calib->flip_x;
            dev->absolute->flip_y = calib->flip_y;
            dev->absolute->rotation = calib->rotation;
            dev->absolute->button_threshold = calib->button_threshold;
            ret = Success;
        } else if (status == DeviceBusy || status == BadValue) {
            rep.status = status;
            ret = Success;
        } else {
            ret = BadMatch;
        }

        break;
    case DEVICE_ABS_AREA:
        area = (xDeviceAbsAreaCtl *)&stuff[1];

        status = ChangeDeviceControl(client, dev, (xDeviceCtl *) area);

        if (status == Success) {
            dev->absolute->offset_x = area->offset_x;
            dev->absolute->offset_y = area->offset_y;
            dev->absolute->width = area->width;
            dev->absolute->height = area->height;
            dev->absolute->screen = area->screen;
            dev->absolute->following = area->following;
            ret = Success;
        } else if (status == DeviceBusy || status == BadValue) {
            rep.status = status;
            ret = Success;
        } else {
            ret = Success;
        }

        break;
    case DEVICE_CORE:
        c = (xDeviceCoreCtl *)&stuff[1];

        status = ChangeDeviceControl(client, dev, (xDeviceCtl *) c);

        if (status == Success) {
            dev->coreEvents = c->status;
            ret = Success;
        } else if (status == DeviceBusy) {
            rep.status = DeviceBusy;
            ret = Success;
        } else {
            ret = BadMatch;
        }

        break;
    case DEVICE_ENABLE:
        e = (xDeviceEnableCtl *)&stuff[1];

        status = ChangeDeviceControl(client, dev, (xDeviceCtl *) e);

        if (status == Success) {
            if (e->enable)
                EnableDevice(dev);
            else
                DisableDevice(dev);
            ret = Success;
        } else if (status == DeviceBusy) {
            rep.status = DeviceBusy;
            ret = Success;
        } else {
            ret = BadMatch;
        }

        break;
    default:
        ret = BadValue;
    }

out:
    if (ret == Success) {
        dpn.type = DevicePresenceNotify;
        dpn.time = currentTime.milliseconds;
        dpn.devchange = 1;
        dpn.deviceid = dev->id;
        dpn.control = stuff->control;
        SendEventToAllWindows(dev, DevicePresenceNotifyMask,
                              (xEvent *) &dpn, 1);

        WriteReplyToClient(client, sizeof(xChangeDeviceControlReply), &rep);
    }
    else {
        SendErrorToClient(client, IReqCode, X_ChangeDeviceControl, 0, ret);
    }

    return Success;
}

/***********************************************************************
 *
 * This procedure writes the reply for the xChangeDeviceControl function,
 * if the client and server have a different byte ordering.
 *
 */

void
SRepXChangeDeviceControl(ClientPtr client, int size,
			 xChangeDeviceControlReply * rep)
{
    swaps(&rep->sequenceNumber);
    swapl(&rep->length);
    WriteToClient(client, size, rep);
}
