/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2011 NoMachine (http://www.nomachine.com)          */
/* Copyright (c) 2008-2014 Oleksandr Shneyder <o.shneyder@phoca-gmbh.de>  */
/* Copyright (c) 2011-2016 Mike Gabriel <mike.gabriel@das-netzwerkteam.de>*/
/* Copyright (c) 2014-2016 Mihai Moldovan <ionic@ionic.de>                */
/* Copyright (c) 2014-2016 Ulrich Sibiller <uli42@gmx.de>                 */
/* Copyright (c) 2015-2016 Qindel Group (http://www.qindel.com)           */
/*                                                                        */
/* nx-X11, NX protocol compression and NX extensions to this software     */
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

/*

Copyright 1992, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from The Open Group.

*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <nx-X11/Xlibint.h>
#include <nx-X11/Xos.h>

/*ARGSUSED*/
Bool
_XAsyncErrorHandler(
    register Display *dpy,
    register xReply *rep,
    char *buf,
    int len,
    XPointer data)
{
    register _XAsyncErrorState *state;

    state = (_XAsyncErrorState *)data;
    if (rep->generic.type == X_Error &&
	(!state->error_code ||
	 rep->error.errorCode == state->error_code) &&
	(!state->major_opcode ||
	 rep->error.majorCode == state->major_opcode) &&
	(!state->minor_opcode ||
	 rep->error.minorCode == state->minor_opcode) &&
	(!state->min_sequence_number ||
	 (state->min_sequence_number <= dpy->last_request_read)) &&
	(!state->max_sequence_number ||
	 (state->max_sequence_number >= dpy->last_request_read))) {
	state->last_error_received = rep->error.errorCode;
	state->error_count++;
	return True;
    }
    return False;
}

void _XDeqAsyncHandler(
    Display *dpy,
    register _XAsyncHandler *handler)
{
    register _XAsyncHandler **prev;
    register _XAsyncHandler *async;

    for (prev = &dpy->async_handlers;
	 (async = *prev) && (async != handler);
	 prev = &async->next)
	;
    if (async)
	*prev = async->next;
}

char *
_XGetAsyncReply(
    register Display *dpy,
    register char *replbuf,	/* data is read into this buffer */
    register xReply *rep,	/* value passed to calling handler */
    char *buf,			/* value passed to calling handler */
    int len,			/* value passed to calling handler */
    int extra,			/* extra words to read, ala _XReply */
    Bool discard)		/* discard after extra?, ala _XReply */
{
    if (extra == 0) {
	if (discard && (rep->generic.length << 2) > len)
	    _XEatData (dpy, (rep->generic.length << 2) - len);
	return (char *)rep;
    }

    if (extra <= rep->generic.length) {
	int size = SIZEOF(xReply) + (extra << 2);
	if (size > len) {
	    memcpy(replbuf, buf, len);
	    _XRead(dpy, replbuf + len, size - len);
	    buf = replbuf;
	    len = size;
#ifdef MUSTCOPY
	} else {
	    memcpy(replbuf, buf, size);
	    buf = replbuf;
#endif
	}

	if (discard && rep->generic.length > extra &&
	    (rep->generic.length << 2) > len)
	    _XEatData (dpy, (rep->generic.length << 2) - len);

	return buf;
    }
    /*
     *if we get here, then extra > rep->generic.length--meaning we
     * read a reply that's shorter than we expected.  This is an
     * error,  but we still need to figure out how to handle it...
     */
    if ((rep->generic.length << 2) > len)
	_XEatData (dpy, (rep->generic.length << 2) - len);
#ifdef NX_TRANS_SOCKET

    /*
     * The original code has provision
     * for returning already.
     */

#endif
    _XIOError (dpy);
    return (char *)rep;
}

void
_XGetAsyncData(
    Display *dpy,
    char *data,			/* data is read into this buffer */
    char *buf,			/* value passed to calling handler */
    int len,			/* value passed to calling handler */
    int skip,			/* number of bytes already read in previous
				   _XGetAsyncReply or _XGetAsyncData calls */
    int datalen,		/* size of data buffer in bytes */
    int discardtotal)		/* min. bytes to consume (after skip) */
{
    buf += skip;
    len -= skip;
    if (!data) {
	if (datalen > len)
	    _XEatData(dpy, datalen - len);
    } else if (datalen <= len) {
	memcpy(data, buf, datalen);
    } else {
	memcpy(data, buf, len);
	_XRead(dpy, data + len, datalen - len);
    }
    if (discardtotal > len) {
	if (datalen > len)
	    len = datalen;
	_XEatData(dpy, discardtotal - len);
    }
}
