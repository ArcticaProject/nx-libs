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

/* $XdotOrg: xc/programs/Xserver/Xext/xvdisp.c,v 1.6 2005/07/03 08:53:36 daniels Exp $ */
/***********************************************************
Copyright 1991 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Digital or MIT not be
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
/* $XFree86: xc/programs/Xserver/Xext/xvdisp.c,v 1.27 2003/07/16 01:38:31 dawes Exp $ */

#if !defined(__sun) && !defined(__CYGWIN__)

#include "Trap.h"

#include "../../Xext/xvdisp.c"

#undef  TEST
#undef  DEBUG

/*
** ProcXvDispatch
**
**
**
*/

int
ProcXvDispatch(ClientPtr client)
{
  int result;

  REQUEST(xReq);

  UpdateCurrentTime();

  /*
   * Report upstream that we are
   * dispatching a XVideo operation.
   */

  nxagentXvTrap = 1;

  #ifdef TEST
  fprintf(stderr, "ProcXvDispatch: Going to dispatch XVideo operation [%d] for client [%d].\n", 
              stuff->data, client -> index);
  #endif

  switch (stuff->data) 
    {
    case xv_QueryExtension: result = (ProcXvQueryExtension(client)); break;
    case xv_QueryAdaptors: result = (ProcXvQueryAdaptors(client)); break;
    case xv_QueryEncodings: result = (ProcXvQueryEncodings(client)); break;
    case xv_PutVideo:
#ifdef PANORAMIX
        if(!noPanoramiXExtension)
            result = (XineramaXvPutVideo(client));
        else
#endif
            result = (ProcXvPutVideo(client));
            break;
    case xv_PutStill:
#ifdef PANORAMIX
        if(!noPanoramiXExtension)
            result = (XineramaXvPutStill(client));
        else
#endif
    	    result = (ProcXvPutStill(client));
    	    break;
    case xv_GetVideo: result = (ProcXvGetVideo(client)); break;
    case xv_GetStill: result = (ProcXvGetStill(client)); break;
    case xv_GrabPort: result = (ProcXvGrabPort(client)); break;
    case xv_UngrabPort: result = (ProcXvUngrabPort(client)); break;
    case xv_SelectVideoNotify: result = (ProcXvSelectVideoNotify(client)); break;
    case xv_SelectPortNotify: result = (ProcXvSelectPortNotify(client)); break;
    case xv_StopVideo: 
#ifdef PANORAMIX
        if(!noPanoramiXExtension)
	    result = (XineramaXvStopVideo(client));
	else
#endif
	    result = (ProcXvStopVideo(client));
            break;
    case xv_SetPortAttribute: 
#ifdef PANORAMIX
        if(!noPanoramiXExtension)
	    result = (XineramaXvSetPortAttribute(client));
	else
#endif
	    result = (ProcXvSetPortAttribute(client));
            break;
    case xv_GetPortAttribute: result = (ProcXvGetPortAttribute(client)); break;
    case xv_QueryBestSize: result = (ProcXvQueryBestSize(client)); break;
    case xv_QueryPortAttributes: result = (ProcXvQueryPortAttributes(client)); break;
    case xv_PutImage:
#ifdef PANORAMIX
        if(!noPanoramiXExtension)
	    result = (XineramaXvPutImage(client));
	else
#endif
	    result = (ProcXvPutImage(client));
            break;
#ifdef MITSHM
    case xv_ShmPutImage: 
#ifdef PANORAMIX
        if(!noPanoramiXExtension)
	    result = (XineramaXvShmPutImage(client));
	else
#endif
	    result = (ProcXvShmPutImage(client));
	    break;
#endif
    case xv_QueryImageAttributes: result = (ProcXvQueryImageAttributes(client)); break;
    case xv_ListImageFormats: result = (ProcXvListImageFormats(client)); break;
    default:
      if (stuff->data < xvNumRequests)
	{
	  SendErrorToClient(client, XvReqCode, stuff->data, 0, 
			    BadImplementation);
	  result = (BadImplementation); break;
	}
      else
	{
	  SendErrorToClient(client, XvReqCode, stuff->data, 0, BadRequest);
	  result = (BadRequest);  break;
	}
    }

  nxagentXvTrap = 0;

  #ifdef TEST
  fprintf(stderr, "ProcXvDispatch: Dispatched XVideo operation [%d] for client [%d].\n", 
              stuff->data, client -> index);
  #endif

  return result;
}

int
SProcXvDispatch(ClientPtr client)
{
  int result;

  REQUEST(xReq);

  UpdateCurrentTime();

  /*
   * Report upstream that we are
   * dispatching a XVideo operation.
   */

  nxagentXvTrap = 1;

  #ifdef TEST
  fprintf(stderr, "SProcXvDispatch: Going to dispatch XVideo operation [%d] for client [%d].\n", 
              stuff->data, client -> index);
  #endif

  switch (stuff->data) 
    {
    case xv_QueryExtension: result = (SProcXvQueryExtension(client)); break;
    case xv_QueryAdaptors: result = (SProcXvQueryAdaptors(client)); break;
    case xv_QueryEncodings: result = (SProcXvQueryEncodings(client)); break;
    case xv_PutVideo: result = (SProcXvPutVideo(client)); break;
    case xv_PutStill: result = (SProcXvPutStill(client)); break;
    case xv_GetVideo: result = (SProcXvGetVideo(client)); break;
    case xv_GetStill: result = (SProcXvGetStill(client)); break;
    case xv_GrabPort: result = (SProcXvGrabPort(client)); break;
    case xv_UngrabPort: result = (SProcXvUngrabPort(client)); break;
    case xv_SelectVideoNotify: result = (SProcXvSelectVideoNotify(client)); break;
    case xv_SelectPortNotify: result = (SProcXvSelectPortNotify(client)); break;
    case xv_StopVideo: result = (SProcXvStopVideo(client)); break;
    case xv_SetPortAttribute: result = (SProcXvSetPortAttribute(client)); break;
    case xv_GetPortAttribute: result = (SProcXvGetPortAttribute(client)); break;
    case xv_QueryBestSize: result = (SProcXvQueryBestSize(client)); break;
    case xv_QueryPortAttributes: result = (SProcXvQueryPortAttributes(client)); break;
    case xv_PutImage: result = (SProcXvPutImage(client)); break;
#ifdef MITSHM
    case xv_ShmPutImage: result = (SProcXvShmPutImage(client)); break;
#endif
    case xv_QueryImageAttributes: result = (SProcXvQueryImageAttributes(client)); break;
    case xv_ListImageFormats: result = (SProcXvListImageFormats(client)); break;
    default:
      if (stuff->data < xvNumRequests)
	{
	  SendErrorToClient(client, XvReqCode, stuff->data, 0, 
			    BadImplementation);
	  result = (BadImplementation); break;
	}
      else
	{
	  SendErrorToClient(client, XvReqCode, stuff->data, 0, BadRequest);
	  result = (BadRequest); break;
	}
    }

  nxagentXvTrap = 0;

  #ifdef TEST
  fprintf(stderr, "ProcXvDispatch: Dispatched XVideo operation [%d] for client [%d].\n", 
              stuff->data, client -> index);
  #endif

  return result;
}
#endif /* !defined(__sun) && !defined(__CYGWIN__) */
