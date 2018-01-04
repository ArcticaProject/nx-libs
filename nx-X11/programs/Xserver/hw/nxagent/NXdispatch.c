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

/************************************************************

Copyright 1987, 1989, 1998  The Open Group

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


Copyright 1987, 1989 by Digital Equipment Corporation, Maynard, Massachusetts.

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

********************************************************/

/* The panoramix components contained the following notice */
/*****************************************************************

Copyright (c) 1991, 1997 Digital Equipment Corporation, Maynard, Massachusetts.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
DIGITAL EQUIPMENT CORPORATION BE LIABLE FOR ANY CLAIM, DAMAGES, INCLUDING,
BUT NOT LIMITED TO CONSEQUENTIAL OR INCIDENTAL DAMAGES, OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Digital Equipment Corporation
shall not be used in advertising or otherwise to promote the sale, use or other
dealings in this Software without prior written authorization from Digital
Equipment Corporation.

******************************************************************/


#ifdef __sun
#define False 0
#define True 1
#endif

#include <X11/fonts/fontstruct.h>

#define GC XlibGC
#include <nx-X11/Xlib.h>
#undef GC

#include "../../dix/dispatch.c"

#include "windowstr.h"

#include "Atoms.h"
#include "Splash.h"
#include "Client.h"
#include "Clipboard.h"
#include "Reconnect.h"
#include "Millis.h"
#include "Font.h"
#include <nx/Shadow.h>
#include "Handlers.h"
#include "Keyboard.h"

const int nxagentMaxFontNames = 10000;

char dispatchExceptionAtReset = DE_RESET;

/*
 * This allows the agent to exit if no
 * client is connected within a timeout.
 */

int nxagentClients = 0;

void nxagentWaitDisplay(void);

void nxagentListRemoteFonts(const char *, int);

unsigned int nxagentWMtimeout = 0;
Bool         nxagentWMPassed  = 0;

/*
 * Timeouts based on screen saver time.
 */

int nxagentAutoDisconnectTimeout = 0;

#include "Xatom.h"

/*
 * Set here the required log level.
 */

#define PANIC
#define WARNING
#undef  TEST
#undef  WATCH

/*
 * Log begin and end of the important handlers.
 */

#undef  BLOCKS

#ifdef WATCH
#include "unistd.h"
#endif

#ifdef TEST
#include "Literals.h"
#endif

#ifdef VIEWPORT_FRAME

extern WindowPtr nxagentViewportFrameLeft;
extern WindowPtr nxagentViewportFrameRight;
extern WindowPtr nxagentViewportFrameAbove;
extern WindowPtr nxagentViewportFrameBelow;

#define IsViewportFrame(pWin) ((pWin) == nxagentViewportFrameLeft || \
                                   (pWin) == nxagentViewportFrameRight || \
                                       (pWin) == nxagentViewportFrameAbove || \
                                           (pWin) == nxagentViewportFrameBelow)

#else

#define IsViewportFrame(pWin) (0)

#endif /* #ifdef VIEWPORT_FRAME */

extern int nxagentMaxAllowedResets;

extern int nxagentFindClientResource(int, RESTYPE, void *);


void
InitSelections()
{
    if (CurrentSelections)
	free(CurrentSelections);
    CurrentSelections = (Selection *)NULL;
    NumCurrentSelections = 0;

#ifdef NXAGENT_CLIPBOARD
    {
      Selection *newsels;
      newsels = (Selection *)malloc(2 * sizeof(Selection));
      if (!newsels)
        return;
      NumCurrentSelections += 2;
      CurrentSelections = newsels;

      CurrentSelections[0].selection = XA_PRIMARY;
      CurrentSelections[0].lastTimeChanged = ClientTimeToServerTime(0);
      CurrentSelections[0].window = screenInfo.screens[0]->root->drawable.id;
      CurrentSelections[0].pWin = NULL;
      CurrentSelections[0].client = NullClient;

      CurrentSelections[1].selection = MakeAtom("CLIPBOARD", 9, 1);
      CurrentSelections[1].lastTimeChanged = ClientTimeToServerTime(0);
      CurrentSelections[1].window = screenInfo.screens[0]->root->drawable.id;
      CurrentSelections[1].pWin = NULL;
      CurrentSelections[1].client = NullClient;
    }
#endif

}

#define MAJOROP ((xReq *)client->requestBuffer)->reqType

void
Dispatch(void)
{
    register int        *clientReady;     /* array of request ready clients */
    register int	result;
    register ClientPtr	client;
    register int	nready;
    register HWEventQueuePtr* icheck = checkForInput;
    long			start_tick;

    unsigned long currentDispatch = 0;

    nextFreeClientID = 1;
    InitSelections();
    nClients = 0;

    /*
     * The agent initialization was successfully
     * completed. We can now handle our clients.
     */

    #ifdef XKB

    nxagentInitXkbWrapper();

    nxagentTuneXkbWrapper();

    #endif

    #ifdef NXAGENT_ONSTART

    /*
     * Set NX_WM property (used by NX client to identify
     * the agent's window) three seconds since the first
     * client connects.
     */

    nxagentWMtimeout = GetTimeInMillis() + 3000;

    #endif

    clientReady = (int *) malloc(sizeof(int) * MaxClients);
    if (!clientReady)
	return;

  #ifdef WATCH

  fprintf(stderr, "Dispatch: Watchpoint 12.\n");

/*
Reply   Total	Cached	Bits In			Bits Out		Bits/Reply	  Ratio
------- -----	------	-------			--------		----------	  -----
#3      1		352 bits (0 KB) ->	236 bits (0 KB) ->	352/1 -> 236/1	= 1.492:1
#14     1		256 bits (0 KB) ->	101 bits (0 KB) ->	256/1 -> 101/1	= 2.535:1
#16     1		256 bits (0 KB) ->	26 bits (0 KB) ->	256/1 -> 26/1	= 9.846:1
#20     2	2	12256 bits (1 KB) ->	56 bits (0 KB) ->	6128/1 -> 28/1	= 218.857:1
#43     1		256 bits (0 KB) ->	45 bits (0 KB) ->	256/1 -> 45/1	= 5.689:1
#47     2	2	42304 bits (5 KB) ->	49 bits (0 KB) ->	21152/1 -> 24/1	= 863.347:1
#98     1		256 bits (0 KB) ->	34 bits (0 KB) ->	256/1 -> 34/1	= 7.529:1
*/

  sleep(30);

  #endif

  #ifdef TEST
  fprintf(stderr, "Dispatch: Value of dispatchException is [%x].\n",
              dispatchException);

  fprintf(stderr, "Dispatch: Value of dispatchExceptionAtReset is [%x].\n",
              dispatchExceptionAtReset);
  #endif

  if (!(dispatchException & DE_TERMINATE))
    dispatchException = 0;

  while (!dispatchException)
    {
        if (*icheck[0] != *icheck[1])
	{
	    ProcessInputEvents();
	    FlushIfCriticalOutputPending();
	}

        /*
         * Ensure we remove the splash after the timeout.
         * Initializing clientReady[0] to -1 will tell
         * WaitForSomething() to yield control after the
         * timeout set in clientReady[1].
         */

        clientReady[0] = 0;

        if (nxagentSplashWindow != None || (nxagentOption(Xdmcp) == 1 && nxagentXdmcpUp == 0))
        {
          #ifdef TEST
          fprintf(stderr, "******Dispatch: Requesting a timeout of [%d] Ms.\n",
                      NXAGENT_WAKEUP);
          #endif

          clientReady[0] = -1;
          clientReady[1] = NXAGENT_WAKEUP;
        }

        if (serverGeneration > nxagentMaxAllowedResets &&
                nxagentSessionState == SESSION_STARTING &&
                    (nxagentOption(Xdmcp) == 0 || nxagentXdmcpUp == 1))
        {
          #ifdef NX_DEBUG_INPUT
          fprintf(stderr, "Session: Session started at '%s' timestamp [%lu].\n",
                      GetTimeAsString(), GetTimeInMillis());
          #else
          fprintf(stderr, "Session: Session started at '%s'.\n",
                      GetTimeAsString());
          #endif

          nxagentSessionState = SESSION_UP;
          saveAgentState("RUNNING");
        }

        #ifdef BLOCKS
        fprintf(stderr, "[End dispatch]\n");
        #endif

	nready = WaitForSomething(clientReady);

        #ifdef BLOCKS
        fprintf(stderr, "[Begin dispatch]\n");
        #endif

        #ifdef TEST
        fprintf(stderr, "******Dispatch: Running with [%d] clients ready.\n",
                    nready);
        #endif
        
        #ifdef NXAGENT_ONSTART

        currentDispatch = GetTimeInMillis();

        /*
         * If the timeout is expired set the
         * selection informing the NX client
         * that the agent is ready.
         */

        if (!nxagentWMPassed && (nxagentWMtimeout < currentDispatch))
        {
          nxagentRemoveSplashWindow(NULL);
        }

        nxagentClients = nClients;

        #endif

	if (nready)
	{
	    clientReady[0] = SmartScheduleClient (clientReady, nready);
	    nready = 1;
	}
       /***************** 
	*  Handle events in round robin fashion, doing input between 
	*  each round 
	*****************/

	while (!dispatchException && (--nready >= 0))
	{
	    client = clients[clientReady[nready]];
	    if (! client)
	    {
		/* KillClient can cause this to happen */
		continue;
	    }
	    /* GrabServer activation can cause this to be true */
	    if (grabState == GrabKickout)
	    {
		grabState = GrabActive;
		break;
	    }
	    isItTimeToYield = FALSE;
 
            requestingClient = client;
	    start_tick = SmartScheduleTime;
	    while (!isItTimeToYield)
	    {
	        if (*icheck[0] != *icheck[1])
		{
		    ProcessInputEvents();
		    FlushIfCriticalOutputPending();
		}
		if ((SmartScheduleTime - start_tick) >= SmartScheduleSlice)
		{
		    /* Penalize clients which consume ticks */
		    if (client->smart_priority > SMART_MIN_PRIORITY)
			client->smart_priority--;
		    break;
		}
		/* now, finally, deal with client requests */

                #ifdef TEST
                fprintf(stderr, "******Dispatch: Reading request from client [%d].\n",
                            client->index);
                #endif

	        result = ReadRequestFromClient(client);
	        if (result <= 0) 
	        {
		    if (result < 0)
			CloseDownClient(client);
		    break;
	        }
#ifdef NXAGENT_SERVER

                #ifdef TEST

                else
                {

                    if (MAJOROP > 127)
                    {
                      fprintf(stderr, "******Dispatch: Read [Extension] request OPCODE#%d MINOR#%d "
                                  "size [%d] client [%d].\n", MAJOROP, *((char *) client->requestBuffer + 1),
                                      client->req_len << 2, client->index);
                    }
                    else
                    {
                      fprintf(stderr, "******Dispatch: Read [%s] request OPCODE#%d size [%d] client [%d].\n",
                                  nxagentRequestLiteral[MAJOROP], MAJOROP, client->req_len << 2,
                                      client->index);
                    }
                }

                #endif
#endif

		client->sequence++;
#ifdef DEBUG
		if ((client->requestLogIndex >= MAX_REQUEST_LOG) || (client->requestLogIndex <= 0))
		    client->requestLogIndex = 0;
		client->requestLog[client->requestLogIndex] = MAJOROP;
		client->requestLogIndex++;
#endif
		if (result > (maxBigRequestSize << 2))
		    result = BadLength;
		else
#ifdef NXAGENT_SERVER
                {
                    result = (* client->requestVector[MAJOROP])(client);

                    #ifdef TEST

                    if (MAJOROP > 127)
                    {
                      fprintf(stderr, "******Dispatch: Handled [Extension] request OPCODE#%d MINOR#%d "
                                  "size [%d] client [%d] result [%d].\n", MAJOROP,
                                      *((char *) client->requestBuffer + 1), client->req_len << 2,
                                          client->index, result);
                    }
                    else
                    {
                      fprintf(stderr, "******Dispatch: Handled [%s] request OPCODE#%d size [%d] client [%d] "
                                  "result [%d].\n", nxagentRequestLiteral[MAJOROP], MAJOROP,
                                      client->req_len << 2, client->index, result);
                    }

                    #endif

                    /*
                     * Can set isItTimeToYield to force
                     * the dispatcher to pay attention
                     * to another client.
                     */

                    nxagentDispatchHandler(client, client->req_len << 2, 0);
                }
#else
		    result = (* client->requestVector[MAJOROP])(client);
#endif


                if (!SmartScheduleSignalEnable)
                    SmartScheduleTime = GetTimeInMillis();

		if (result != Success) 
		{
		    if (client->noClientException != Success)
                        CloseDownClient(client);
                    else
		        SendErrorToClient(client, MAJOROP,
					  MinorOpcodeOfRequest(client),
					  client->errorValue, result);
		    break;
	        }
#ifdef DAMAGEEXT
		FlushIfCriticalOutputPending ();
#endif
	    }
	    FlushAllOutput();
	    client = clients[clientReady[nready]];
	    if (client)
		client->smart_stop_tick = SmartScheduleTime;
	    requestingClient = NULL;
	}
	dispatchException &= ~DE_PRIORITYCHANGE;
    }
#if defined(DDXBEFORERESET)
    ddxBeforeReset ();
#endif
    if ((dispatchException & DE_RESET) && 
            (serverGeneration > nxagentMaxAllowedResets))
    {
        dispatchException &= ~DE_RESET;
        dispatchException |= DE_TERMINATE;

        fprintf(stderr, "Info: Reached threshold of maximum allowed resets.\n");
    }

    nxagentResetAtomMap();

    if (serverGeneration > nxagentMaxAllowedResets)
    {
      /*
       * The session is terminating. Force an I/O
       * error on the display and wait until the
       * NX transport is gone.
       */
  
      fprintf(stderr, "Session: Terminating session at '%s'.\n", GetTimeAsString());
      saveAgentState("TERMINATING");

      nxagentWaitDisplay();

      fprintf(stderr, "Session: Session terminated at '%s'.\n", GetTimeAsString());
    }

    if (nxagentOption(Shadow) == 1)
    {
      NXShadowDestroy();
    }
    saveAgentState("TERMINATED");

    KillAllClients();
    free(clientReady);
    dispatchException &= ~DE_RESET;
}

#undef MAJOROP

int
ProcReparentWindow(register ClientPtr client)
{
    register WindowPtr pWin, pParent;
    REQUEST(xReparentWindowReq);
    register int result;

    REQUEST_SIZE_MATCH(xReparentWindowReq);
    pWin = (WindowPtr)SecurityLookupWindow(stuff->window, client,
					   DixWriteAccess);
    if (!pWin)
        return(BadWindow);

    if (!nxagentWMPassed)
    {
      nxagentRemoveSplashWindow(pWin);
    }

    pParent = (WindowPtr)SecurityLookupWindow(stuff->parent, client,
					      DixWriteAccess);
    if (!pParent)
        return(BadWindow);
    if (SAME_SCREENS(pWin->drawable, pParent->drawable))
    {
        if ((pWin->backgroundState == ParentRelative) &&
            (pParent->drawable.depth != pWin->drawable.depth))
            return BadMatch;
	if ((pWin->drawable.class != InputOnly) &&
	    (pParent->drawable.class == InputOnly))
	    return BadMatch;
        result =  ReparentWindow(pWin, pParent, 
			 (short)stuff->x, (short)stuff->y, client);
	if (client->noClientException != Success)
            return(client->noClientException);
	else
            return(result);
    }
    else 
        return (BadMatch);
}


int
ProcQueryTree(register ClientPtr client)
{
    xQueryTreeReply reply;
    int numChildren = 0;
    register WindowPtr pChild, pWin, pHead;
    Window  *childIDs = (Window *)NULL;
    REQUEST(xResourceReq);

    REQUEST_SIZE_MATCH(xResourceReq);
    pWin = (WindowPtr)SecurityLookupWindow(stuff->id, client,
					   DixReadAccess);
    if (!pWin)
        return(BadWindow);
    memset(&reply, 0, sizeof(xQueryTreeReply));
    reply.type = X_Reply;
    reply.root = pWin->drawable.pScreen->root->drawable.id;
    reply.sequenceNumber = client->sequence;
    if (pWin->parent)
	reply.parent = pWin->parent->drawable.id;
    else
        reply.parent = (Window)None;
    pHead = RealChildHead(pWin);
    for (pChild = pWin->lastChild; pChild != pHead; pChild = pChild->prevSib)
    {
      if (!IsViewportFrame(pChild))
      {
	numChildren++;
      }
    }
    if (numChildren)
    {
	int curChild = 0;

	childIDs = (Window *) malloc(numChildren * sizeof(Window));
	if (!childIDs)
	    return BadAlloc;
	for (pChild = pWin->lastChild; pChild != pHead; pChild = pChild->prevSib)
        {
          if (!IsViewportFrame(pChild))
          {
	    childIDs[curChild++] = pChild->drawable.id;
          }
        }
    }
    
    reply.nChildren = numChildren;
    reply.length = (numChildren * sizeof(Window)) >> 2;
    
    WriteReplyToClient(client, sizeof(xQueryTreeReply), &reply);
    if (numChildren)
    {
    	client->pSwapReplyFunc = (ReplySwapPtr) Swap32Write;
	WriteSwappedDataToClient(client, numChildren * sizeof(Window), childIDs);
	free(childIDs);
    }

    return(client->noClientException);
}


int
ProcSetSelectionOwner(register ClientPtr client)
{
    WindowPtr pWin;
    TimeStamp time;
    REQUEST(xSetSelectionOwnerReq);

    REQUEST_SIZE_MATCH(xSetSelectionOwnerReq);
    UpdateCurrentTime();
    time = ClientTimeToServerTime(stuff->time);

    /* If the client's time stamp is in the future relative to the server's
	time stamp, do not set the selection, just return success. */
    if (CompareTimeStamps(time, currentTime) == LATER)
    	return Success;
    if (stuff->window != None)
    {
        pWin = (WindowPtr)SecurityLookupWindow(stuff->window, client,
					       DixReadAccess);
        if (!pWin)
            return(BadWindow);
    }
    else
        pWin = (WindowPtr)None;
    if (ValidAtom(stuff->selection))
    {
	int i = 0;

	/*
	 * First, see if the selection is already set... 
	 */
	while ((i < NumCurrentSelections) && 
	       CurrentSelections[i].selection != stuff->selection) 
            i++;
        if (i < NumCurrentSelections)
        {        
	    xEvent event = {0};

	    /* If the timestamp in client's request is in the past relative
		to the time stamp indicating the last time the owner of the
		selection was set, do not set the selection, just return 
		success. */
            if (CompareTimeStamps(time, CurrentSelections[i].lastTimeChanged)
		== EARLIER)
		return Success;
	    if (CurrentSelections[i].client &&
		(!pWin || (CurrentSelections[i].client != client)))
	    {
		event.u.u.type = SelectionClear;
		event.u.selectionClear.time = time.milliseconds;
		event.u.selectionClear.window = CurrentSelections[i].window;
		event.u.selectionClear.atom = CurrentSelections[i].selection;
		(void) TryClientEvents (CurrentSelections[i].client, &event, 1,
				NoEventMask, NoEventMask /* CantBeFiltered */,
				NullGrab);
	    }
	}
	else
	{
	    /*
	     * It doesn't exist, so add it...
	     */
	    Selection *newsels;

	    if (i == 0)
		newsels = (Selection *)malloc(sizeof(Selection));
	    else
		newsels = (Selection *)realloc(CurrentSelections,
			    (NumCurrentSelections + 1) * sizeof(Selection));
	    if (!newsels)
		return BadAlloc;
	    NumCurrentSelections++;
	    CurrentSelections = newsels;
	    CurrentSelections[i].selection = stuff->selection;
	}
        CurrentSelections[i].lastTimeChanged = time;
	CurrentSelections[i].window = stuff->window;
	CurrentSelections[i].pWin = pWin;
	CurrentSelections[i].client = (pWin ? client : NullClient);
	if (SelectionCallback)
	{
	    SelectionInfoRec	info;

	    info.selection = &CurrentSelections[i];
	    info.kind= SelectionSetOwner;
	    CallCallbacks(&SelectionCallback, &info);
	}

#ifdef NXAGENT_CLIPBOARD
      if ((CurrentSelections[i].pWin != NULL) &&
              (nxagentOption(Clipboard) != ClipboardNone) &&
                  ((CurrentSelections[i].selection == XA_PRIMARY) ||
                       (CurrentSelections[i].selection == MakeAtom("CLIPBOARD", 9, 0))))
      {
        nxagentSetSelectionOwner(&CurrentSelections[i]);
      }
#endif
	return (client->noClientException);
    }
    else 
    {
	client->errorValue = stuff->selection;
        return (BadAtom);
    }
}


int
ProcConvertSelection(register ClientPtr client)
{
    Bool paramsOkay;
    xEvent event;
    WindowPtr pWin;
    REQUEST(xConvertSelectionReq);

    REQUEST_SIZE_MATCH(xConvertSelectionReq);
    pWin = (WindowPtr)SecurityLookupWindow(stuff->requestor, client,
					   DixReadAccess);
    if (!pWin)
        return(BadWindow);

#ifdef NXAGENT_CLIPBOARD
    if (((stuff->selection == XA_PRIMARY) ||
           (stuff->selection == MakeAtom("CLIPBOARD", 9, 0))) &&
               nxagentOption(Clipboard) != ClipboardNone)
    {
      int i = 0;

      while ((i < NumCurrentSelections) &&
                CurrentSelections[i].selection != stuff->selection) i++;

      if ((i < NumCurrentSelections) && (CurrentSelections[i].window != None))
      {
        if (nxagentConvertSelection(client, pWin, stuff->selection, stuff->requestor,
                                       stuff->property, stuff->target, stuff->time))
        {
          return (client->noClientException);
        }
      }
    }
#endif

    paramsOkay = (ValidAtom(stuff->selection) && ValidAtom(stuff->target));
    if (stuff->property != None)
	paramsOkay &= ValidAtom(stuff->property);
    if (paramsOkay)
    {
	int i;

	i = 0;
	while ((i < NumCurrentSelections) && 
	       CurrentSelections[i].selection != stuff->selection) i++;
	if ((i < NumCurrentSelections) && 
	    (CurrentSelections[i].window != None) && (CurrentSelections[i].client != NullClient)
#ifdef XCSECURITY
	    && (!client->CheckAccess ||
		(* client->CheckAccess)(client, CurrentSelections[i].window,
					RT_WINDOW, DixReadAccess,
					CurrentSelections[i].pWin))
#endif
	    )

	{        
	    memset(&event, 0, sizeof(xEvent));
	    event.u.u.type = SelectionRequest;
	    event.u.selectionRequest.time = stuff->time;
	    event.u.selectionRequest.owner = 
			CurrentSelections[i].window;
	    event.u.selectionRequest.requestor = stuff->requestor;
	    event.u.selectionRequest.selection = stuff->selection;
	    event.u.selectionRequest.target = stuff->target;
	    event.u.selectionRequest.property = stuff->property;
	    if (TryClientEvents(
		CurrentSelections[i].client, &event, 1, NoEventMask,
		NoEventMask /* CantBeFiltered */, NullGrab))
		return (client->noClientException);
	}
	memset(&event, 0, sizeof(xEvent));
	event.u.u.type = SelectionNotify;
	event.u.selectionNotify.time = stuff->time;
	event.u.selectionNotify.requestor = stuff->requestor;
	event.u.selectionNotify.selection = stuff->selection;
	event.u.selectionNotify.target = stuff->target;
	event.u.selectionNotify.property = None;
	(void) TryClientEvents(client, &event, 1, NoEventMask,
			       NoEventMask /* CantBeFiltered */, NullGrab);
	return (client->noClientException);
    }
    else 
    {
	client->errorValue = stuff->property;
        return (BadAtom);
    }
}


int
ProcOpenFont(register ClientPtr client)
{
    int	err;
    char fontReq[256];
    REQUEST(xOpenFontReq);

    REQUEST_FIXED_SIZE(xOpenFontReq, stuff->nbytes);
    client->errorValue = stuff->fid;
    LEGAL_NEW_RESOURCE(stuff->fid, client);

    memcpy(fontReq,(char *)&stuff[1],(stuff->nbytes<256)?stuff->nbytes:255);
    fontReq[stuff->nbytes]=0;
    if (strchr(fontReq,'*') || strchr(fontReq,'?'))
    {
       extern int nxOpenFont(ClientPtr, XID, Mask, unsigned, char*);
#ifdef NXAGENT_FONTMATCH_DEBUG
       fprintf(stderr, "Dispatch: ProcOpenFont try to find a common font with font pattern=%s\n",fontReq);
#endif
       nxagentListRemoteFonts(fontReq, nxagentMaxFontNames);
       err = nxOpenFont(client, stuff->fid, (Mask) 0,
		stuff->nbytes, (char *)&stuff[1]);
    }
    else
    err = OpenFont(client, stuff->fid, (Mask) 0,
		stuff->nbytes, (char *)&stuff[1]);
    if (err == Success)
    {
	return(client->noClientException);
    }
    else
	return err;
}

int
ProcCloseFont(register ClientPtr client)
{
    FontPtr pFont;
    REQUEST(xResourceReq);

    REQUEST_SIZE_MATCH(xResourceReq);
    pFont = (FontPtr)SecurityLookupIDByType(client, stuff->id, RT_FONT,
					    DixDestroyAccess);
    if (pFont != (FontPtr)NULL)
    {
        #ifdef NXAGENT_SERVER

        /*
         * When a client closes a font the resource
         * should not be lost if the reference counter
         * is not 0, otherwise the server will not be
         * able to find this font looping through the
         * resources.
         */

        if (pFont -> refcnt > 0)
        {
          if (nxagentFindClientResource(serverClient -> index, RT_NX_FONT, pFont) == 0)
          {
            #ifdef TEST
            fprintf(stderr, "ProcCloseFont: Switching resource for font at [%p].\n",
                        (void *) pFont);
            #endif

            nxagentFontPriv(pFont) -> mirrorID = FakeClientID(serverClient -> index);

            AddResource(nxagentFontPriv(pFont) -> mirrorID, RT_NX_FONT, pFont);

          }
          #ifdef TEST
          else
          {
            fprintf(stderr, "ProcCloseFont: Found duplicated font at [%p], "
                        "resource switching skipped.\n", (void *) pFont);
          }
          #endif
        }

        #endif

        FreeResource(stuff->id, RT_NONE);
	return(client->noClientException);
    }
    else
    {
	client->errorValue = stuff->id;
        return (BadFont);
    }
}


int
ProcListFonts(register ClientPtr client)
{
    char tmp[256];

    REQUEST(xListFontsReq);

    REQUEST_FIXED_SIZE(xListFontsReq, stuff->nbytes);
    memcpy(tmp,(unsigned char *) &stuff[1],(stuff->nbytes<256)?stuff->nbytes:255);
    tmp[stuff->nbytes]=0;

#ifdef NXAGENT_FONTMATCH_DEBUG
    fprintf(stderr, "Dispatch: ListFont request with pattern %s max_names=%d\n",tmp,stuff->maxNames);
#endif
    nxagentListRemoteFonts(tmp, stuff -> maxNames < nxagentMaxFontNames ? nxagentMaxFontNames : stuff->maxNames);
    return ListFonts(client, (unsigned char *) &stuff[1], stuff->nbytes, 
	stuff->maxNames);
}

int
ProcListFontsWithInfo(register ClientPtr client)
{
    char tmp[256];
    REQUEST(xListFontsWithInfoReq);

    REQUEST_FIXED_SIZE(xListFontsWithInfoReq, stuff->nbytes);

    memcpy(tmp,(unsigned char *) &stuff[1],(stuff->nbytes<256)?stuff->nbytes:255);
    tmp[stuff->nbytes]=0;
#ifdef NXAGENT_FONTMATCH_DEBUG
    fprintf(stderr, "Dispatch: ListFont with info request with pattern %s max_names=%d\n",tmp,stuff->maxNames);
#endif
    nxagentListRemoteFonts(tmp, stuff -> maxNames < nxagentMaxFontNames ? nxagentMaxFontNames :stuff->maxNames);

    return StartListFontsWithInfo(client, stuff->nbytes,
				  (unsigned char *) &stuff[1], stuff->maxNames);
}


int
ProcFreePixmap(register ClientPtr client)
{
    PixmapPtr pMap;

    REQUEST(xResourceReq);

    REQUEST_SIZE_MATCH(xResourceReq);
    pMap = (PixmapPtr)SecurityLookupIDByType(client, stuff->id, RT_PIXMAP,
					     DixDestroyAccess);
    if (pMap) 
    {
        #ifdef NXAGENT_SERVER

        /*
         * When a client releases a pixmap the resource
         * should not be lost if the reference counter
         * is not 0, otherwise the server will not be
         * able to find this pixmap looping through the
         * resources.
         */

        if (pMap -> refcnt > 0)
        {
          if (nxagentFindClientResource(serverClient -> index, RT_NX_PIXMAP, pMap) == 0)
          {
            #ifdef TEST
            fprintf(stderr, "ProcFreePixmap: Switching resource for pixmap at [%p].\n",
                       (void *) pMap);
            #endif

            nxagentPixmapPriv(pMap) -> mid = FakeClientID(serverClient -> index);

            AddResource(nxagentPixmapPriv(pMap) -> mid, RT_NX_PIXMAP, pMap);
          }
          #ifdef TEST
          else
          {
            fprintf(stderr, "ProcFreePixmap: Found duplicated pixmap at [%p], "
                        "resource switching skipped.\n", (void *) pMap);
          }
          #endif
        }

        #endif

	FreeResource(stuff->id, RT_NONE);
	return(client->noClientException);
    }
    else 
    {
	client->errorValue = stuff->id;
	return (BadPixmap);
    }
}


int
ProcSetScreenSaver (register ClientPtr client)
{
    int blankingOption, exposureOption;
    REQUEST(xSetScreenSaverReq);

    REQUEST_SIZE_MATCH(xSetScreenSaverReq);
    blankingOption = stuff->preferBlank;
    if ((blankingOption != DontPreferBlanking) &&
        (blankingOption != PreferBlanking) &&
        (blankingOption != DefaultBlanking))
    {
	client->errorValue = blankingOption;
        return BadValue;
    }
    exposureOption = stuff->allowExpose;
    if ((exposureOption != DontAllowExposures) &&
        (exposureOption != AllowExposures) &&
        (exposureOption != DefaultExposures))
    {
	client->errorValue = exposureOption;
        return BadValue;
    }
    if (stuff->timeout < -1)
    {
	client->errorValue = stuff->timeout;
        return BadValue;
    }
    if (stuff->interval < -1)
    {
	client->errorValue = stuff->interval;
        return BadValue;
    }

    /*
     * The NX agent uses the screen saver procedure
     * to monitor the user activities and launch its
     * handlers (like timeout feature), so we can't
     * always allow the clients to change our values.
     */

    #ifdef TEST
    fprintf(stderr, "ProcSetScreenSaver: Called with timeout [%d] interval [%d] Blanking [%d] Exposure [%d].\n",
                stuff -> timeout, stuff -> interval, blankingOption, exposureOption);
    #endif

    if (nxagentOption(Timeout) == 0)
    {
      if (blankingOption == DefaultBlanking)
      {
	ScreenSaverBlanking = defaultScreenSaverBlanking;
      }
      else
      {
	ScreenSaverBlanking = blankingOption; 
      }

      if (exposureOption == DefaultExposures)
      {
	ScreenSaverAllowExposures = defaultScreenSaverAllowExposures;
      }
      else
      {
        ScreenSaverAllowExposures = exposureOption;
      }

      if (stuff->timeout >= 0)
      {
        ScreenSaverTime = stuff->timeout * MILLI_PER_SECOND;
      }
      else
      {
        ScreenSaverTime = defaultScreenSaverTime;
      }

      if (stuff->interval >= 0)
      {
        ScreenSaverInterval = stuff->interval * MILLI_PER_SECOND;
      }
      else
      {
        ScreenSaverInterval = defaultScreenSaverInterval;
      }

      SetScreenSaverTimer();
    }
    #ifdef TEST

    else
    {
      fprintf(stderr, "ProcSetScreenSaver: Keeping auto-disconnect timeout set to [%d] seconds.\n",
                  nxagentOption(Timeout));
    }

    #endif

    return (client->noClientException);
}


int ProcForceScreenSaver(register ClientPtr client)
{
    REQUEST(xForceScreenSaverReq);

    REQUEST_SIZE_MATCH(xForceScreenSaverReq);

    if ((stuff->mode != ScreenSaverReset) &&
	(stuff->mode != ScreenSaverActive))
    {
	client->errorValue = stuff->mode;
        return BadValue;
    }

    /*
     * The NX agent uses the screen saver procedure
     * to monitor the user activities and launch its
     * handlers (like timeout feature), so we can't
     * always allow the clients to force the screen
     * saver handler execution.
     */

    if (nxagentOption(Timeout) == 0)
    {
      SaveScreens(SCREEN_SAVER_FORCER, (int)stuff->mode);
    }

    #ifdef TEST

    else
    {
      fprintf(stderr, "ProcForceScreenSaver: Ignoring the client request with mode [%d].\n",
                  stuff -> mode);
    }

    #endif

    return client->noClientException;
}


/**********************
 * CloseDownClient
 *
 *  Client can either mark his resources destroy or retain.  If retained and
 *  then killed again, the client is really destroyed.
 *********************/

void
CloseDownClient(register ClientPtr client)
{
    Bool really_close_down = client->clientGone ||
			     client->closeDownMode == DestroyAll;

    /*
     * There must be a better way to hook a
     * call-back function to be called any
     * time a client is going to be closed.
     */

    nxagentClearClipboard(client, NULL);

    /*
     * Need to reset the karma counter and
     * get rid of the pending sync replies.
     */

    nxagentWakeupByReset(client);

    /*
     * Check if the client
     * is a shadow nxagent.
     */

    nxagentCheckIfShadowAgent(client);

    if (!client->clientGone)
    {
	/* ungrab server if grabbing client dies */
	if (grabState != GrabNone && grabClient == client)
	{
	    UngrabServer(client);
	}
	BITCLEAR(grabWaiters, client->index);
	DeleteClientFromAnySelections(client);
	ReleaseActiveGrabs(client);
	DeleteClientFontStuff(client);
	if (!really_close_down)
	{
	    /*  This frees resources that should never be retained
	     *  no matter what the close down mode is.  Actually we
	     *  could do this unconditionally, but it's probably
	     *  better not to traverse all the client's resources
	     *  twice (once here, once a few lines down in
	     *  FreeClientResources) in the common case of
	     *  really_close_down == TRUE.
	     */
	    FreeClientNeverRetainResources(client);
	    client->clientState = ClientStateRetained;
  	    if (ClientStateCallback)
            {
		NewClientInfoRec clientinfo;

		clientinfo.client = client; 
		clientinfo.prefix = (xConnSetupPrefix *)NULL;  
		clientinfo.setup = (xConnSetup *) NULL;
		CallCallbacks((&ClientStateCallback), (void *)&clientinfo);
            } 
	}
	client->clientGone = TRUE;  /* so events aren't sent to client */
	if (ClientIsAsleep(client))
	    ClientSignal (client);
	ProcessWorkQueueZombies();
	CloseDownConnection(client);

	/* If the client made it to the Running stage, nClients has
	 * been incremented on its behalf, so we need to decrement it
	 * now.  If it hasn't gotten to Running, nClients has *not*
	 * been incremented, so *don't* decrement it.
	 */
	if (client->clientState != ClientStateInitial &&
	    client->clientState != ClientStateAuthenticating )
	{
	    --nClients;
	}
    }

    if (really_close_down)
    {
	if (client->clientState == ClientStateRunning && nClients == 0)
	    dispatchException |= dispatchExceptionAtReset;

	client->clientState = ClientStateGone;
	if (ClientStateCallback)
	{
	    NewClientInfoRec clientinfo;

	    clientinfo.client = client; 
	    clientinfo.prefix = (xConnSetupPrefix *)NULL;  
	    clientinfo.setup = (xConnSetup *) NULL;
	    CallCallbacks((&ClientStateCallback), (void *)&clientinfo);
	} 	    
	FreeClientResources(client);
	if (client->index < nextFreeClientID)
	    nextFreeClientID = client->index;
	clients[client->index] = NullClient;
	SmartLastClient = NullClient;
	free(client);

	while (!clients[currentMaxClients-1])
	    currentMaxClients--;
    }
}

int
InitClientPrivates(ClientPtr client)
{
    register char *ptr;
    DevUnion *ppriv;
    register unsigned *sizes;
    register unsigned size;
    register int i;

    if (totalClientSize == sizeof(ClientRec))
	ppriv = (DevUnion *)NULL;
    else if (client->index)
	ppriv = (DevUnion *)(client + 1);
    else
    {
	ppriv = (DevUnion *)malloc(totalClientSize - sizeof(ClientRec));
	if (!ppriv)
	    return 0;
    }
    client->devPrivates = ppriv;
    sizes = clientPrivateSizes;
    ptr = (char *)(ppriv + clientPrivateLen);
    for (i = clientPrivateLen; --i >= 0; ppriv++, sizes++)
    {
	if ( (size = *sizes) )
	{
	    ppriv->ptr = (void *)ptr;
	    ptr += size;
	}
	else
	    ppriv->ptr = (void *)NULL;
    }

    /*
     * Initialize the private members.
     */

    nxagentInitClientPrivates(client);

    return 1;
}
