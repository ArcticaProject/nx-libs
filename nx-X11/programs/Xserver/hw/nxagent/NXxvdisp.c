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

#ifdef HAVE_NXAGENT_CONFIG_H
#include <nxagent-config.h>
#endif

#if !defined(__sun) && !defined(__CYGWIN__)

#include "Trap.h"

#include "../../Xext/xvdisp.c"

#undef  TEST
#undef  DEBUG

int nxagent_ProcXvDispatch(ClientPtr client);
int nxagent_SProcXvDispatch(ClientPtr client);

/*
** ProcXvDispatch
**
**
**
*/

int
nxagent_ProcXvDispatch(ClientPtr client)
{
  REQUEST(xReq);

  UpdateCurrentTime();

  switch (stuff->data) 
    {
    case xv_QueryExtension: return(ProcXvQueryExtension(client));
    case xv_QueryAdaptors: return(ProcXvQueryAdaptors(client));
    case xv_QueryEncodings: return(ProcXvQueryEncodings(client));
    case xv_PutVideo:
#ifdef PANORAMIX
        if(!noPanoramiXExtension)
            return(XineramaXvPutVideo(client));
        else
#endif
	    return(ProcXvPutVideo(client));
    case xv_PutStill:
#ifdef PANORAMIX
        if(!noPanoramiXExtension)
            return(XineramaXvPutStill(client));
        else
#endif
	{
	    return(ProcXvPutStill(client));
	}
    case xv_GetVideo: return(ProcXvGetVideo(client));
    case xv_GetStill: return(ProcXvGetStill(client));
    case xv_GrabPort: return(ProcXvGrabPort(client));
    case xv_UngrabPort: return(ProcXvUngrabPort(client));
    case xv_SelectVideoNotify: return(ProcXvSelectVideoNotify(client));
    case xv_SelectPortNotify: return(ProcXvSelectPortNotify(client));
    case xv_StopVideo: 
#ifdef PANORAMIX
        if(!noPanoramiXExtension)
	    return(XineramaXvStopVideo(client));
	else
#endif
	    return(ProcXvStopVideo(client));
    case xv_SetPortAttribute: 
#ifdef PANORAMIX
        if(!noPanoramiXExtension)
	    return(XineramaXvSetPortAttribute(client));
	else
#endif
	    return(ProcXvSetPortAttribute(client));
    case xv_GetPortAttribute: return(ProcXvGetPortAttribute(client));
    case xv_QueryBestSize: return(ProcXvQueryBestSize(client));
    case xv_QueryPortAttributes: return(ProcXvQueryPortAttributes(client));
    case xv_PutImage:
#ifdef PANORAMIX
        if(!noPanoramiXExtension)
	    return(XineramaXvPutImage(client));
	else
#endif
	    return(ProcXvPutImage(client));
#ifdef MITSHM
    case xv_ShmPutImage: 
#ifdef PANORAMIX
        if(!noPanoramiXExtension)
	    return(XineramaXvShmPutImage(client));
	else
#endif
	    return(ProcXvShmPutImage(client));
#endif
    case xv_QueryImageAttributes: return(ProcXvQueryImageAttributes(client));
    case xv_ListImageFormats: return(ProcXvListImageFormats(client));
    default:
      if (stuff->data < xvNumRequests)
	{
	  SendErrorToClient(client, XvReqCode, stuff->data, 0, 
			    BadImplementation);
	  return(BadImplementation);
	}
      else
	{
	  SendErrorToClient(client, XvReqCode, stuff->data, 0, BadRequest);
	  return(BadRequest);
	}
    }
}

int
ProcXvDispatch(ClientPtr client)
{
  int result;

  /*
   * Report upstream that we are
   * dispatching a XVideo operation.
   */

  #ifdef TEST
  fprintf(stderr, "ProcXvDispatch: Going to dispatch XVideo operation [%d] for client [%d].\n",
              stuff->data, client -> index);
  #endif

  nxagentXvTrap = 1;

  result = nxagent_ProcXvDispatch(client);

  nxagentXvTrap = 0;

  #ifdef TEST
  fprintf(stderr, "ProcXvDispatch: Dispatched XVideo operation [%d] for client [%d].\n",
              stuff->data, client -> index);
  #endif

  return result;
}


int
nxagent_SProcXvDispatch(ClientPtr client)
{
  REQUEST(xReq);

  UpdateCurrentTime();

  switch (stuff->data) 
    {
    case xv_QueryExtension: return(SProcXvQueryExtension(client));
    case xv_QueryAdaptors: return(SProcXvQueryAdaptors(client));
    case xv_QueryEncodings: return(SProcXvQueryEncodings(client));
    case xv_PutVideo: return(SProcXvPutVideo(client));
    case xv_PutStill: return(SProcXvPutStill(client));
    case xv_GetVideo: return(SProcXvGetVideo(client));
    case xv_GetStill: return(SProcXvGetStill(client));
    case xv_GrabPort: return(SProcXvGrabPort(client));
    case xv_UngrabPort: return(SProcXvUngrabPort(client));
    case xv_SelectVideoNotify: return(SProcXvSelectVideoNotify(client));
    case xv_SelectPortNotify: return(SProcXvSelectPortNotify(client));
    case xv_StopVideo: return(SProcXvStopVideo(client));
    case xv_SetPortAttribute: return(SProcXvSetPortAttribute(client));
    case xv_GetPortAttribute: return(SProcXvGetPortAttribute(client));
    case xv_QueryBestSize: return(SProcXvQueryBestSize(client));
    case xv_QueryPortAttributes: return(SProcXvQueryPortAttributes(client));
    case xv_PutImage: return(SProcXvPutImage(client));
#ifdef MITSHM
    case xv_ShmPutImage: return(SProcXvShmPutImage(client));
#endif
    case xv_QueryImageAttributes: return(SProcXvQueryImageAttributes(client));
    case xv_ListImageFormats: return(SProcXvListImageFormats(client));
    default:
      if (stuff->data < xvNumRequests)
	{
	  SendErrorToClient(client, XvReqCode, stuff->data, 0, 
			    BadImplementation);
	  return(BadImplementation);
	}
      else
	{
	  SendErrorToClient(client, XvReqCode, stuff->data, 0, BadRequest);
	  return(BadRequest);
	}
    }
}


int
SProcXvDispatch(ClientPtr client)
{
  int result;

  /*
   * Report upstream that we are
   * dispatching a XVideo operation.
   */

  #ifdef TEST
  fprintf(stderr, "SProcXvDispatch: Going to dispatch XVideo operation [%d] for client [%d].\n",
              stuff->data, client -> index);
  #endif

  nxagentXvTrap = 1;

  result = nxagent_SProcXvDispatch(client);

  nxagentXvTrap = 0;

  #ifdef TEST
  fprintf(stderr, "SProcXvDispatch: Dispatched XVideo operation [%d] for client [%d].\n",
              stuff->data, client -> index);
  #endif

  return result;
}



#endif /* !defined(__sun) && !defined(__CYGWIN__) */
