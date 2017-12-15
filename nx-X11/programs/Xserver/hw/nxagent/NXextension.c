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

#include "Trap.h"

#include "../../dix/extension.c"

int
ProcQueryExtension(ClientPtr client)
{
    xQueryExtensionReply reply;
    int i;
    REQUEST(xQueryExtensionReq);

    REQUEST_FIXED_SIZE(xQueryExtensionReq, stuff->nbytes);
    
    memset(&reply, 0, sizeof(xQueryExtensionReply));
    reply.type = X_Reply;
    reply.length = 0;
    reply.major_opcode = 0;
    reply.sequenceNumber = client->sequence;

    if ( ! NumExtensions )
        reply.present = xFalse;
    else
    {
	i = FindExtension((char *)&stuff[1], stuff->nbytes);
        if (i < 0

            /*
             * Hide RENDER if our implementation
             * is faulty.
             */

            || (nxagentRenderTrap && strcmp(extensions[i]->name, "RENDER") == 0)
#ifdef XCSECURITY
	    /* don't show insecure extensions to untrusted clients */
	    || (client->trustLevel == XSecurityClientUntrusted &&
		!extensions[i]->secure)
#endif
	    )
            reply.present = xFalse;
        else
        {            
            reply.present = xTrue;
	    reply.major_opcode = extensions[i]->base;
	    reply.first_event = extensions[i]->eventBase;
	    reply.first_error = extensions[i]->errorBase;
	}
    }
    WriteReplyToClient(client, sizeof(xQueryExtensionReply), &reply);
    return(client->noClientException);
}

int
ProcListExtensions(ClientPtr client)
{
    xListExtensionsReply reply;
    char *bufptr, *buffer;
    int total_length = 0;

    REQUEST_SIZE_MATCH(xReq);

    memset(&reply, 0, sizeof(xListExtensionsReply));
    reply.type = X_Reply;
    reply.nExtensions = 0;
    reply.length = 0;
    reply.sequenceNumber = client->sequence;
    buffer = NULL;

    if ( NumExtensions )
    {
        register int i, j;

        for (i=0;  i<NumExtensions; i++)
	{
#ifdef XCSECURITY
	    /* don't show insecure extensions to untrusted clients */
	    if (client->trustLevel == XSecurityClientUntrusted &&
		!extensions[i]->secure)
		continue;
#endif
            /*
             * Hide RENDER if our implementation
             * is faulty.
             */

            if (nxagentRenderTrap && strcmp(extensions[i]->name, "RENDER") == 0)
                continue;

	    total_length += strlen(extensions[i]->name) + 1;
	    reply.nExtensions += 1 + extensions[i]->num_aliases;
	    for (j = extensions[i]->num_aliases; --j >= 0;)
		total_length += strlen(extensions[i]->aliases[j]) + 1;
	}
        reply.length = (total_length + 3) >> 2;
	buffer = bufptr = (char *)malloc(total_length);
	if (!buffer)
	    return(BadAlloc);
        for (i=0;  i<NumExtensions; i++)
        {
	    int len;
#ifdef XCSECURITY
	    if (client->trustLevel == XSecurityClientUntrusted &&
		!extensions[i]->secure)
		continue;
#endif
            *bufptr++ = len = strlen(extensions[i]->name);
	    memmove(bufptr, extensions[i]->name,  len);
	    bufptr += len;
	    for (j = extensions[i]->num_aliases; --j >= 0;)
	    {
		*bufptr++ = len = strlen(extensions[i]->aliases[j]);
		memmove(bufptr, extensions[i]->aliases[j],  len);
		bufptr += len;
	    }
	}
    }
    WriteReplyToClient(client, sizeof(xListExtensionsReply), &reply);
    if (reply.length)
    {
        WriteToClient(client, total_length, buffer);
        free(buffer);
    }
    return(client->noClientException);
}
