
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
 * Request to release a grab of a button on an extension device.
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
#include "dixgrabs.h"

#include "ungrdevb.h"

#define AllModifiersMask ( \
	ShiftMask | LockMask | ControlMask | Mod1Mask | Mod2Mask | \
	Mod3Mask | Mod4Mask | Mod5Mask )

/***********************************************************************
 *
 * Handle requests from a client with a different byte order.
 *
 */

int
SProcXUngrabDeviceButton(client)
    register ClientPtr client;
    {
    REQUEST(xUngrabDeviceButtonReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xUngrabDeviceButtonReq);
    swapl(&stuff->grabWindow);
    swaps(&stuff->modifiers);
    return(ProcXUngrabDeviceButton(client));
    }

/***********************************************************************
 *
 * Release a grab of a button on an extension device.
 *
 */

int
ProcXUngrabDeviceButton(client)
    ClientPtr client;
    {
    DeviceIntPtr	dev;
    DeviceIntPtr	mdev;
    WindowPtr		pWin;
    GrabRec		temporaryGrab;

    REQUEST(xUngrabDeviceButtonReq);
    REQUEST_SIZE_MATCH(xUngrabDeviceButtonReq);

    dev = LookupDeviceIntRec (stuff->grabbed_device);
    if (dev == NULL)
	{
	SendErrorToClient(client, IReqCode, X_UngrabDeviceButton, 0, 
	    BadDevice);
	return Success;
	}
    if (dev->button == NULL)
	{
	SendErrorToClient(client, IReqCode, X_UngrabDeviceButton, 0, 
		BadMatch);
	return Success;
	}

    if (stuff->modifier_device != UseXKeyboard)
	{
	mdev = LookupDeviceIntRec (stuff->modifier_device);
	if (mdev == NULL)
	    {
	    SendErrorToClient(client, IReqCode, X_UngrabDeviceButton, 0, 
	        BadDevice);
	    return Success;
	    }
	if (mdev->key == NULL)
	    {
	    SendErrorToClient(client, IReqCode, X_UngrabDeviceButton, 0, 
		BadMatch);
	    return Success;
	    }
	}
    else
	mdev = (DeviceIntPtr) LookupKeyboardDevice();

    pWin = LookupWindow(stuff->grabWindow, client);
    if (!pWin)
	{
	SendErrorToClient(client, IReqCode, X_UngrabDeviceButton, 0, 
	    BadWindow);
	return Success;
	}

    if ((stuff->modifiers != AnyModifier) &&
	(stuff->modifiers & ~AllModifiersMask))
	{
	SendErrorToClient(client, IReqCode, X_UngrabDeviceButton, 0, 
	    BadValue);
	return Success;
	}

    temporaryGrab.resource = client->clientAsMask;
    temporaryGrab.device = dev;
    temporaryGrab.window = pWin;
    temporaryGrab.type = DeviceButtonPress;
    temporaryGrab.modifierDevice = mdev;
    temporaryGrab.modifiersDetail.exact = stuff->modifiers;
    temporaryGrab.modifiersDetail.pMask = NULL;
    temporaryGrab.detail.exact = stuff->button;
    temporaryGrab.detail.pMask = NULL;

    DeletePassiveGrabFromList(&temporaryGrab);
    return Success;
    }
