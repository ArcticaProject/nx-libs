
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
 * Extension function to return the version of the extension.
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

#include "getvers.h"

XExtensionVersion	AllExtensionVersions[128];

/***********************************************************************
 *
 * Handle a request from a client with a different byte order than us.
 *
 */

int
SProcXGetExtensionVersion(client)
    register ClientPtr client;
    {
    REQUEST(xGetExtensionVersionReq);
    swaps(&stuff->length);
    REQUEST_AT_LEAST_SIZE(xGetExtensionVersionReq);
    swaps(&stuff->nbytes);
    return(ProcXGetExtensionVersion(client));
    }

/***********************************************************************
 *
 * This procedure lists the input devices available to the server.
 *
 */

int
ProcXGetExtensionVersion (client)
    register ClientPtr client;
    {
    xGetExtensionVersionReply	rep;

    REQUEST(xGetExtensionVersionReq);
    REQUEST_AT_LEAST_SIZE(xGetExtensionVersionReq);

    if (stuff->length != (sizeof(xGetExtensionVersionReq) + 
	stuff->nbytes + 3)>>2)
	{
	SendErrorToClient(client, IReqCode, X_GetExtensionVersion, 0, 
		BadLength);
	return Success;
	}

    memset(&rep, 0, sizeof(xGetExtensionVersionReply));
    rep.repType = X_Reply;
    rep.RepType = X_GetExtensionVersion;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    rep.major_version = 0;
    rep.minor_version = 0;

    rep.present = TRUE;
    if (rep.present)
	{
	rep.major_version = 
	    AllExtensionVersions[IReqCode-128].major_version;
	rep.minor_version = 
	    AllExtensionVersions[IReqCode-128].minor_version;
	}
    WriteReplyToClient (client, sizeof (xGetExtensionVersionReply), &rep);

    return Success;
    }

/***********************************************************************
 *
 * This procedure writes the reply for the XGetExtensionVersion function,
 * if the client and server have a different byte ordering.
 *
 */

void
SRepXGetExtensionVersion (client, size, rep)
    ClientPtr	client;
    int		size;
    xGetExtensionVersionReply	*rep;
    {
    swaps(&rep->sequenceNumber);
    swapl(&rep->length);
    swaps(&rep->major_version);
    swaps(&rep->minor_version);
    WriteToClient(client, size, rep);
    }
