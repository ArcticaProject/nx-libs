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

#include "X.h"
#include "Xproto.h"
#include "Xatom.h"
#include "selection.h"
#include "windowstr.h"
#include "scrnintstr.h"

#include "Agent.h"
#include "Windows.h"
#include "Atoms.h"
#include "Args.h"
#include "Trap.h"
#include "Rootless.h"
#include "Clipboard.h"
#include "Utils.h"
#include "Client.h"

#include "gcstruct.h"
#include "xfixeswire.h"
#include "X11/include/Xfixes_nxagent.h"

/*
 * Use asynchronous get property replies.
 */

#include "compext/Compext.h"

/*
 * Set here the required log level.
 */

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

/*
 * These are defined in the dispatcher.
 */

extern int NumCurrentSelections;
extern Selection *CurrentSelections;

int nxagentLastClipboardClient = -1;

static int agentClipboardInitialized = False;
static int clientAccum;

XlibAtom serverTransToAgentProperty;
Atom clientCutProperty;
static Window serverWindow;

const int nxagentPrimarySelection = 0;
const int nxagentClipboardSelection = 1;
const int nxagentMaxSelections = 2;

typedef struct _SelectionOwner
{
  XlibAtom selection;   /* _external_ Atom */
  ClientPtr client;     /* internal client */
  Window window;        /* internal window id */
  WindowPtr windowPtr;  /* internal window struct */
  Time lastTimeChanged; /* internal time */
} SelectionOwner;

/*
 * this contains the last selection owner in nxagent. The
 * lastTimeChanged is always an internal time. If .client is NULL the
 * owner is outside nxagent. .selection will _always_ contain the
 * external atom of the selection
 */
static SelectionOwner *lastSelectionOwner;
static XlibAtom serverLastRequestedSelection;

#define IS_INTERNAL_OWNER(lsoindex) (lastSelectionOwner[lsoindex].client != NULL)

/*
 * Needed to handle the notify selection event to
 * be sent to client once the selection property
 * has been retrieved from the real X server.
 */

typedef enum
{
  SelectionStageNone,
  SelectionStageQuerySize,
  SelectionStageWaitSize,
  SelectionStageQueryData,
  SelectionStageWaitData
} ClientSelectionStage;

static WindowPtr     lastClientWindowPtr;
static ClientPtr     lastClientClientPtr;
static Window        lastClientRequestor;
static Atom          lastClientProperty;
static Atom          lastClientSelection;
static Atom          lastClientTarget;
static Time          lastClientTime;
static Time          lastClientReqTime;
static unsigned long lastClientPropertySize;

static ClientSelectionStage lastClientStage;

static Window        lastServerRequestor;
static XlibAtom      lastServerProperty;
static XlibAtom      lastServerTarget;
static Time          lastServerTime;

static XlibAtom serverTARGETS;
static XlibAtom serverTIMESTAMP;
static XlibAtom serverTEXT;
static XlibAtom serverCOMPOUND_TEXT;
static XlibAtom serverUTF8_STRING;
static XlibAtom serverTransFromAgentProperty;
static Atom clientTARGETS;
static Atom clientTIMESTAMP;
static Atom clientTEXT;
static Atom clientCOMPOUND_TEXT;
static Atom clientUTF8_STRING;
static Atom clientCLIPBOARD;

static char szAgentTARGETS[] = "TARGETS";
static char szAgentTEXT[] = "TEXT";
static char szAgentTIMESTAMP[] = "TIMESTAMP";
static char szAgentCOMPOUND_TEXT[] = "COMPOUND_TEXT";
static char szAgentUTF8_STRING[] = "UTF8_STRING";
static char szAgentNX_CUT_BUFFER_CLIENT[] = "NX_CUT_BUFFER_CLIENT";
static char szAgentCLIPBOARD[] = "CLIPBOARD";

/* number of milliseconds to wait for a conversion from the real X server. */
#define CONVERSION_TIMEOUT 5000

/*
 * Time window (milliseconds) within to detect multiple conversion
 * calls of the same client.
 */
#define ACCUM_TIME 5000

/*
 * some helpers for debugging output
 */

static const char * getClientSelectionStageString(int stage)
{
  switch(stage)
  {
    case SelectionStageNone:      return("None"); break;;
    case SelectionStageQuerySize: return("QuerySize"); break;;
    case SelectionStageWaitSize:  return("WaitSize"); break;;
    case SelectionStageQueryData: return("QueryData"); break;;
    case SelectionStageWaitData:  return("WaitData"); break;;
    default:                      return("UNKNOWN!"); break;;
  }
}

#ifdef DEBUG
#define printClientSelectionStage() do {fprintf(stderr, "%s: Current selection stage [%s]\n", __func__, getClientSelectionStageString(lastClientStage));} while (0)
#else
#define printClientSelectionStage()
#endif

#define WINDOWID(ptr) (ptr) ? (ptr->drawable.id) : 0
#define CLINDEX(clientptr) (clientptr) ? (clientptr->index) : -1

#ifdef DEBUG
/*
 * see also nx-X11/lib/src/ErrDes.c
 *
 * We use our own version to avoid Xlib doing expensive calls.
 * FIXME: Must check if XGetErrorText() is really causing traffic over the wire.
 */
const char * getXErrorString(int code)
{
  switch(code)
  {
    case Success:           return("Success"); break;;
    case BadRequest:        return("BadRequest"); break;;
    case BadValue:          return("BadValue"); break;;
    case BadWindow:         return("BadWindow"); break;;
    case BadPixmap:         return("BadPixmap"); break;;
    case BadAtom:           return("BadAtom"); break;;
    case BadCursor:         return("BadCursor"); break;;
    case BadFont:           return("BadFont"); break;;
    case BadMatch:          return("BadMatch"); break;;
    case BadDrawable:       return("BadDrawable"); break;;
    case BadAccess:         return("BadAccess"); break;;
    case BadAlloc:          return("BadAlloc"); break;;
    case BadColor:          return("BadColor"); break;;
    case BadGC:             return("BadGC"); break;;
    case BadIDChoice:       return("BadIDChoice"); break;;
    case BadName:           return("BadName"); break;;
    case BadLength:         return("BadLength"); break;;
    case BadImplementation: return("BadImplementation"); break;;
    default:                return("UNKNOWN!"); break;;
  }
}
#endif

/*
 * Save the values queried from X server.
 */

XFixesAgentInfoRec nxagentXFixesInfo = { -1, -1, -1, 0 };

extern Display *nxagentDisplay;

static Bool validServerTargets(XlibAtom target);
static void setClientSelectionStage(int stage);
static void endTransfer(Bool success);
#define SELECTION_SUCCESS True
#define SELECTION_FAULT False
static void transferSelection(int resource);
#if 0
static void resetSelectionOwner(void);
#endif
static void initSelectionOwner(int index, Atom selection);
static void clearSelectionOwner(int index);
static void storeSelectionOwner(int index, Selection *sel);
static Bool matchSelectionOwner(int index, ClientPtr pClient, WindowPtr pWindow);
static void setSelectionOwner(Selection *pSelection);
static int sendEventToClient(ClientPtr client, xEvent *pEvents);
static void sendSelectionNotifyEventToClient(ClientPtr client,
                                             Time time,
                                             Window requestor,
                                             Atom selection,
                                             Atom target,
                                             Atom property);
static Status sendSelectionNotifyEventToServer(XSelectionEvent *event_to_send);
#ifdef DEBUG
static void printSelectionStat(int sel);
#endif
static void replyRequestSelection(XEvent *X, Bool success);

void nxagentPrintClipboardStat(char *);

#ifdef NXAGENT_TIMESTAMP
extern unsigned long startTime;
#endif

static void printSelectionStat(int sel)
{
  SelectionOwner lOwner = lastSelectionOwner[sel];
  Selection curSel = CurrentSelections[sel];
  char *s = NULL;

  fprintf(stderr, "  owner is inside nxagent?               %s\n", IS_INTERNAL_OWNER(sel) ? "yes" : "no");
  fprintf(stderr, "  lastSelectionOwner[].client            %s\n", nxagentClientInfoString(lOwner.client));
  fprintf(stderr, "  lastSelectionOwner[].window            [0x%x]\n", lOwner.window);
  if (lOwner.windowPtr)
    fprintf(stderr, "  lastSelectionOwner[].windowPtr         [%p] ([0x%x]\n", (void *)lOwner.windowPtr, WINDOWID(lOwner.windowPtr));
  else
    fprintf(stderr, "  lastSelectionOwner[].windowPtr         -\n");
  fprintf(stderr, "  lastSelectionOwner[].lastTimeChanged   [%u]\n", lOwner.lastTimeChanged);

  /*
    print the selection name. selection is _always_ a a remote Atom!
  */
  SAFE_XFree(s); s = XGetAtomName(nxagentDisplay, lOwner.selection);
  fprintf(stderr, "  lastSelectionOwner[].selection         [% 4ld][%s] (%s)\n", lOwner.selection, validateString(s), lOwner.client ? "inside nxagent" : "remote X server");
  SAFE_XFree(s);
#ifdef CLIENTIDS
  fprintf(stderr, "  CurrentSelections[].client             [%p] index [%d] PID [%d] Cmd [%s]\n",
          (void *)curSel.client,
          CLINDEX(curSel.client),
          GetClientPid(curSel.client),
          GetClientCmdName(curSel.client));
#else
  fprintf(stderr, "  CurrentSelections[].client             [%p] index [%d]\n",
          (void *)curSel.client,
          CLINDEX(curSel.client);
#endif
  fprintf(stderr, "  CurrentSelections[].window             [0x%x]\n", curSel.window);
  return;
}

void nxagentDumpClipboardStat(void)
{
  char *s = NULL;

  fprintf(stderr, "/----- Clipboard internal status -----\n");

  fprintf(stderr, "  current time                    (Time) [%u]\n", GetTimeInMillis());
  fprintf(stderr, "  agentClipboardInitialized       (Bool) [%s]\n", agentClipboardInitialized ? "True" : "False");
  fprintf(stderr, "  clientAccum                      (int) [%d]\n", clientAccum);
  fprintf(stderr, "  nxagentMaxSelections             (int) [%d]\n", nxagentMaxSelections);
  fprintf(stderr, "  NumCurrentSelections             (int) [%d]\n", NumCurrentSelections);
  fprintf(stderr, "  serverWindow                  (Window) [0x%x]\n", serverWindow);
  fprintf(stderr, "  nxagentLastClipboardClient       (int) [%d]\n", nxagentLastClipboardClient);

  fprintf(stderr, "  Clipboard mode                         ");
  switch(nxagentOption(Clipboard))
  {
    case ClipboardBoth:    fprintf(stderr, "[Both]"); break;;
    case ClipboardClient:  fprintf(stderr, "[Client]"); break;;
    case ClipboardServer:  fprintf(stderr, "[Server]"); break;;
    case ClipboardNone:    fprintf(stderr, "[None]"); break;;
    default:               fprintf(stderr, "[UNKNOWN] (FAIL!)"); break;;
  }
  fprintf(stderr, "\n");

  fprintf(stderr, "lastServer\n");
  fprintf(stderr, "  lastServerRequestor           (Window) [0x%x]\n", lastServerRequestor);
  SAFE_XFree(s); s = XGetAtomName(nxagentDisplay, lastServerProperty);
  fprintf(stderr, "  lastServerProperty              (Atom) [% 4ld][%s]\n", lastServerProperty, validateString(s));
  SAFE_XFree(s); s = XGetAtomName(nxagentDisplay, lastServerTarget);
  fprintf(stderr, "  lastServerTarget                (Atom) [% 4ld][%s]\n", lastServerTarget, validateString(s));
  fprintf(stderr, "  lastServerTime                  (Time) [%u]\n", lastServerTime);

  fprintf(stderr, "lastClient\n");
  if (lastClientWindowPtr)
    fprintf(stderr, "  lastClientWindowPtr        (WindowPtr) [%p] ([0x%x])\n", (void *)lastClientWindowPtr, WINDOWID(lastClientWindowPtr));
  else
    fprintf(stderr, "  lastClientWindowPtr        (WindowPtr) -\n");
  fprintf(stderr, "  lastClientClientPtr        (ClientPtr) %s\n", nxagentClientInfoString(lastClientClientPtr));
  fprintf(stderr, "  lastClientRequestor           (Window) [0x%x]\n", lastClientRequestor);
  fprintf(stderr, "  lastClientProperty              (Atom) [% 4d][%s]\n", lastClientProperty, NameForAtom(lastClientProperty));
  fprintf(stderr, "  lastClientSelection             (Atom) [% 4d][%s]\n", lastClientSelection, NameForAtom(lastClientSelection));
  fprintf(stderr, "  lastClientTarget                (Atom) [% 4d][%s]\n", lastClientTarget, NameForAtom(lastClientTarget));
  if (lastClientTime > 0)
    fprintf(stderr, "  lastClientTime                  (Time) [%u] ([%u]ms ago)\n", lastClientTime, GetTimeInMillis() - lastClientTime);
  else
    fprintf(stderr, "  lastClientTime                  (Time) [%u]\n", lastClientTime);
  if (lastClientReqTime > 0)
    fprintf(stderr, "  lastClientReqTime               (Time) [%u] ([%u]ms ago)\n", lastClientReqTime, GetTimeInMillis() - lastClientReqTime);
  else
    fprintf(stderr, "  lastClientReqTime               (Time) [%u]\n", lastClientReqTime);
  fprintf(stderr, "  lastClientPropertySize (unsigned long) [%lu]\n", lastClientPropertySize);
  fprintf(stderr, "  lastClientStage (ClientSelectionStage) [%d][%s]\n", lastClientStage, getClientSelectionStageString(lastClientStage));

  fprintf(stderr, "PRIMARY\n");
  printSelectionStat(nxagentPrimarySelection);
  fprintf(stderr, "CLIPBOARD\n");
  printSelectionStat(nxagentClipboardSelection);

  fprintf(stderr, "Atoms (remote X server)\n");
  SAFE_XFree(s); s = XGetAtomName(nxagentDisplay, serverTARGETS);
  fprintf(stderr, "  serverTARGETS                          [% 4ld][%s]\n", serverTARGETS, validateString(s));
  SAFE_XFree(s); s = XGetAtomName(nxagentDisplay, serverTIMESTAMP);
  fprintf(stderr, "  serverTIMESTAMP                        [% 4ld][%s]\n", serverTIMESTAMP, validateString(s));
  SAFE_XFree(s); s = XGetAtomName(nxagentDisplay, serverTEXT);
  fprintf(stderr, "  serverTEXT                             [% 4ld][%s]\n", serverTEXT, validateString(s));
  SAFE_XFree(s); s = XGetAtomName(nxagentDisplay, serverCOMPOUND_TEXT);
  fprintf(stderr, "  serverCOMPOUND_TEXT                    [% 4ld][%s]\n", serverCOMPOUND_TEXT, validateString(s));
  SAFE_XFree(s); s = XGetAtomName(nxagentDisplay, serverUTF8_STRING);
  fprintf(stderr, "  serverUTF8_STRING                      [% 4ld][%s]\n", serverUTF8_STRING, validateString(s));
  SAFE_XFree(s); s = XGetAtomName(nxagentDisplay, serverTransToAgentProperty);
  fprintf(stderr, "  serverTransToAgentProperty             [% 4ld][%s]\n", serverTransFromAgentProperty, validateString(s));
  SAFE_XFree(s); s = XGetAtomName(nxagentDisplay, serverTransFromAgentProperty);
  fprintf(stderr, "  serverTransFromAgentProperty           [% 4ld][%s]\n", serverTransToAgentProperty, validateString(s));
  SAFE_XFree(s); s = XGetAtomName(nxagentDisplay, serverLastRequestedSelection);
  fprintf(stderr, "  serverLastRequestedSelection           [% 4ld][%s]\n", serverLastRequestedSelection, validateString(s));

  fprintf(stderr, "Atoms (inside nxagent)\n");
  fprintf(stderr, "  clientTARGETS                          [% 4d][%s]\n", clientTARGETS, NameForAtom(clientTARGETS));
  fprintf(stderr, "  clientTIMESTAMP                        [% 4d][%s]\n", clientTIMESTAMP, NameForAtom(clientTIMESTAMP));
  fprintf(stderr, "  clientTEXT                             [% 4d][%s]\n", clientTEXT, NameForAtom(clientTEXT));
  fprintf(stderr, "  clientCOMPOUND_TEXT                    [% 4d][%s]\n", clientCOMPOUND_TEXT, NameForAtom(clientCOMPOUND_TEXT));
  fprintf(stderr, "  clientUTF8_STRING                      [% 4d][%s]\n", clientUTF8_STRING, NameForAtom(clientUTF8_STRING));
  fprintf(stderr, "  clientCLIPBOARD                        [% 4d][%s]\n", clientCLIPBOARD, NameForAtom(clientCLIPBOARD));
  fprintf(stderr, "  clientCutProperty                      [% 4d][%s]\n", clientCutProperty, NameForAtom(clientCutProperty));

  fprintf(stderr, "\\------------------------------------------------------------------------------\n");

  SAFE_XFree(s);
}

/*
 * Helper to handle data transfer
 */
static void setClientSelectionStage(int stage)
{
  if (lastClientStage == stage)
  {
    #ifdef DEBUG
    fprintf(stderr, "%s: selection stage already set to [%s] - doing nothing\n", __func__,
	    getClientSelectionStageString(lastClientStage));
    #endif
    return;
  }

  #ifdef DEBUG
  fprintf(stderr, "%s: Changing selection stage from [%s] to [%s]\n", __func__,
	  getClientSelectionStageString(lastClientStage), getClientSelectionStageString(stage));
  #endif

  lastClientStage = stage;
  if (stage == SelectionStageNone)
  {
    lastClientWindowPtr = NULL;
    lastClientClientPtr = NULL;
    lastClientRequestor = 0;
    lastClientProperty = 0;
    lastClientSelection = 0;
    lastClientTarget = 0;
    lastClientTime = 0;
    lastClientReqTime = 0;
    lastClientPropertySize = 0;
  }
}

/*
 * This is from NXproperty.c.
 */

int GetWindowProperty(WindowPtr pWin, Atom property, long longOffset, long longLength,
                          Bool delete, Atom type, Atom *actualType, int *format,
                              unsigned long *nItems, unsigned long *bytesAfter,
                                  unsigned char **propData);

/*
 * Send a SelectionNotify event to the real X server and do some error
 * handling (in DEBUG mode)
 */
static Status sendSelectionNotifyEventToServer(XSelectionEvent *event_to_send)
{
  Window w = event_to_send->requestor;

  event_to_send->type = SelectionNotify;
  event_to_send->send_event = True;
  event_to_send->display = nxagentDisplay;

  Status result = XSendEvent(nxagentDisplay, w, False, 0L, (XEvent *)event_to_send);

  #ifdef DEBUG
  /*
   * man XSendEvent: XSendEvent returns zero if the conversion to wire
   * protocol format failed and returns nonzero otherwise.  XSendEvent
   * can generate BadValue and BadWindow errors.
   */
  if (result == 0)
  {
    fprintf(stderr, "%s: XSendEvent to [0x%x] failed.\n", __func__, w);
  }
  else
  {
    if (result == BadValue || result == BadWindow)
    {
      fprintf(stderr, "%s: WARNING! XSendEvent to [0x%x] failed: %s\n", __func__, w, getXErrorString(result));
    }
    else
    {
      fprintf(stderr, "%s: XSendEvent() successfully sent to [0x%x]\n", __func__, w);
    }
  }
  #endif

  NXFlushDisplay(nxagentDisplay, NXFlushLink);

  return result;
}

static int sendEventToClient(ClientPtr client, xEvent *pEvents)
{
  return TryClientEvents(client, pEvents, 1, NoEventMask, NoEventMask, NullGrab);
}

static void sendSelectionNotifyEventToClient(ClientPtr client,
                                            Time time,
                                            Window requestor,
                                            Atom selection,
                                            Atom target,
                                            Atom property)
{
  /*
   * Check if the client is still valid.
   */
  if (clients[client -> index] != client)
  {
    #ifdef WARNING
    fprintf(stderr, "%s: WARNING! Invalid client pointer.", __func__);
    #endif

    return;
  }

  xEvent x = {0};
  x.u.u.type = SelectionNotify;
  x.u.selectionNotify.time = time;
  x.u.selectionNotify.requestor = requestor;
  x.u.selectionNotify.selection = selection;
  x.u.selectionNotify.target = target;
  x.u.selectionNotify.property = property;

  #ifdef DEBUG
  if (property == None)
    fprintf(stderr, "%s: Denying request to client %s.\n", __func__,
                nxagentClientInfoString(client));
  else
    fprintf(stderr, "%s: Sending event to client %s.\n", __func__,
                nxagentClientInfoString(client));
  #endif

  sendEventToClient(client, &x);
}

/*
 * Check if target is a valid content type target sent by the real X
 * server, like .e.g XA_STRING or UTF8_STRING. Other, non content type
 * targets like "TARGETS" or "TIMESTAMP" will return false.
 */
static Bool validServerTargets(XlibAtom target)
{
  if (target == XA_STRING)
  {
    #ifdef DEBUG
    fprintf(stderr, "%s: valid target [XA_STRING].\n", __func__);
    #endif
    return True;
  }
  else if (target == serverTEXT)
  {
    #ifdef DEBUG
    fprintf(stderr, "%s: valid target [TEXT].\n", __func__);
    #endif
    return True;
  }
  /* by dimbor */
  else if (target == serverUTF8_STRING)
  {
    #ifdef DEBUG
    fprintf(stderr, "%s: valid target [UTF8_STRING].\n", __func__);
    #endif
    return True;
  }
  else if (target == serverCOMPOUND_TEXT)
  {
    #ifdef DEBUG
    fprintf(stderr, "%s: valid target [COMPOUND_TEXT].\n", __func__);
    #endif
    return True;
  }
  else if (target == serverTARGETS)
  {
    #ifdef DEBUG
    fprintf(stderr, "%s: special target [TARGETS].\n", __func__);
    #endif
    return False;
  }
  else if (target == serverTIMESTAMP)
  {
    #ifdef DEBUG
    fprintf(stderr, "%s: special target [TIMESTAMP].\n", __func__);
    #endif
    return False;
  }

  #ifdef DEBUG
  fprintf(stderr, "%s: invalid target [%lu].\n", __func__, target);
  #endif
  return False;
}

static void initSelectionOwner(int index, Atom selection)
{
  lastSelectionOwner[index].selection = selection;
  lastSelectionOwner[index].client = NullClient;
  lastSelectionOwner[index].window = screenInfo.screens[0]->root->drawable.id;
  lastSelectionOwner[index].windowPtr = NULL;
  lastSelectionOwner[index].lastTimeChanged = GetTimeInMillis();
}

/* there's no owner on nxagent side anymore */
static void clearSelectionOwner(int index)
{
  lastSelectionOwner[index].client = NULL;
  lastSelectionOwner[index].window = None;
  lastSelectionOwner[index].windowPtr = NULL;
  lastSelectionOwner[index].lastTimeChanged = GetTimeInMillis();
}

static void storeSelectionOwner(int index, Selection *sel)
{
  lastSelectionOwner[index].client = sel->client;
  lastSelectionOwner[index].window = sel->window;
  lastSelectionOwner[index].windowPtr = sel->pWin;
  lastSelectionOwner[index].lastTimeChanged = GetTimeInMillis();
}

static Bool matchSelectionOwner(int index, ClientPtr pClient, WindowPtr pWindow)
{
  return ((pClient && lastSelectionOwner[index].client == pClient) ||
          (pWindow && lastSelectionOwner[index].windowPtr == pWindow));
}

/*
 * Clear relevant clipboard states if a client or window is closing.
 * Attention: does not work properly when both client AND window
 * are passed as setClientSelectionStage(None) will also clear
 * the lastClientWindowPtr!
 */
void nxagentClearClipboard(ClientPtr pClient, WindowPtr pWindow)
{
  #ifdef DEBUG
  fprintf(stderr, "%s: Called with client [%p] index [%d] window [%p] ([0x%x]).\n", __func__,
              (void *) pClient, CLINDEX(pClient), (void *) pWindow, WINDOWID(pWindow));
  #endif

  /*
   * Only for PRIMARY and CLIPBOARD selections.
   */

  for (int i = 0; i < nxagentMaxSelections; i++)
  {
    if (matchSelectionOwner(i, pClient, pWindow))
    {
      #ifdef TEST
      fprintf(stderr, "%s: Resetting state with client [%p] window [%p].\n", __func__,
                  (void *) pClient, (void *) pWindow);
      #endif

      clearSelectionOwner(i);

      setClientSelectionStage(SelectionStageNone);

      lastServerRequestor = None;
    }
  }

  if (pWindow && pWindow == lastClientWindowPtr)
  {
    setClientSelectionStage(SelectionStageNone);
  }
}

/*
 * Find the index of the lastSelectionOwner with the selection
 * sel. sel is an atom on the real X server.
 */
int nxagentFindLastSelectionOwnerIndex(XlibAtom sel)
{
  int i = 0;
  while (i < nxagentMaxSelections &&
            lastSelectionOwner[i].selection != sel)
  {
    i++;
  }
  return i;
}

/*
 * Find the index of CurrentSelection with the selection
 * sel. sel is an internal atom.
 */
int nxagentFindCurrentSelectionIndex(Atom sel)
{
  int i = 0;
  while (i < NumCurrentSelections &&
            CurrentSelections[i].selection != sel)
  {
    i++;
  }
  return i;
}

/*
 * This is called from Events.c dispatch loop on reception of a
 * SelectionClear event. We receive this event if someone on the real
 * X server claims the selection ownership.
 */
void nxagentHandleSelectionClearFromXServer(XEvent *X)
{
  #ifdef DEBUG
  fprintf(stderr, "%s: SelectionClear event for selection [%lu].\n", __func__, X->xselectionclear.selection);
  #endif

  if (!agentClipboardInitialized)
  {
    #ifdef DEBUG
    fprintf(stderr, "%s: clipboard not initialized - doing nothing.\n", __func__);
    #endif
    return;
  }

  if (nxagentOption(Clipboard) == ClipboardServer)
  {
    #ifdef DEBUG
    fprintf(stderr, "%s: clipboard mode 'server' - doing nothing.\n", __func__);
    #endif
    return;
  }

  int i = nxagentFindLastSelectionOwnerIndex(X->xselectionclear.selection);
  if (i < nxagentMaxSelections)
  {
    if (IS_INTERNAL_OWNER(i))
    {
      /* send a SelectionClear event to (our) previous owner */
      xEvent x = {0};
      x.u.u.type = SelectionClear;
      x.u.selectionClear.time = GetTimeInMillis();
      x.u.selectionClear.window = lastSelectionOwner[i].window;
      x.u.selectionClear.atom = CurrentSelections[i].selection;

      sendEventToClient(lastSelectionOwner[i].client, &x);
    }

    /*
     * set the root window with the NullClient as selection owner. Our
     * clients asking for the owner via XGetSelectionOwner() will get
     * these for an answer
     */
    CurrentSelections[i].window = screenInfo.screens[0]->root->drawable.id;
    CurrentSelections[i].client = NullClient;

    clearSelectionOwner(i);
  }

  setClientSelectionStage(SelectionStageNone);
}

/*
 * Send a SelectionNotify event as reply to the RequestSelection
 * event X. If success is True take the property from the event, else
 * take None (which reports "failed/denied" to the requestor).
 */
static void replyRequestSelection(XEvent *X, Bool success)
{
  XSelectionEvent eventSelection = {
    .requestor = X->xselectionrequest.requestor,
    .selection = X->xselectionrequest.selection,
    .target    = X->xselectionrequest.target,
    .time      = X->xselectionrequest.time,
    .property  = X->xselectionrequest.property
  };

  if (!success)
  {
    #ifdef DEBUG
    fprintf(stderr, "%s: denying request\n", __func__);
    #endif
    eventSelection.property = None;
  }

  sendSelectionNotifyEventToServer(&eventSelection);
}

/*
 * This is called from Events.c dispatch loop on reception of a
 * SelectionRequest event, meaning a client of the real X server wants
 * to have the selection content. The real X server knows the nxagent
 * as selection owner. But in reality one of our windows is the owner,
 * so we must pass the request on to the real owner.
 */
void nxagentHandleSelectionRequestFromXServer(XEvent *X)
{
  #ifdef DEBUG
  {
      char *strTarget = XGetAtomName(nxagentDisplay, X->xselectionrequest.target);
      char *strSelection = XGetAtomName(nxagentDisplay, X->xselectionrequest.selection);
      char *strProperty = XGetAtomName(nxagentDisplay, X->xselectionrequest.property);

      fprintf(stderr, "%s: Received SelectionRequestEvent from real server: selection [%ld][%s] " \
              "target [%ld][%s] requestor [display[%s]/0x%lx] destination [%ld][%s] lastServerRequestor [0x%x]\n",
              __func__,
              X->xselectionrequest.selection, validateString(strSelection),
              X->xselectionrequest.target,    validateString(strTarget),
              DisplayString(nxagentDisplay), X->xselectionrequest.requestor,
              X->xselectionrequest.property,  validateString(strProperty),
              lastServerRequestor);

      SAFE_XFree(strTarget);
      SAFE_XFree(strSelection);
      SAFE_XFree(strProperty);
  }
  #endif

  if (!agentClipboardInitialized)
  {
    #ifdef DEBUG
    fprintf(stderr, "%s: clipboard not initialized - doing nothing.\n", __func__);
    #endif
    return;
  }

  /* lastServerRequestor in non-NULL (= we are currently in the transfer phase) */
  if (lastServerRequestor != None)
  {
    #ifdef DEBUG
    fprintf(stderr, "%s: denying additional request during transfer phase.\n", __func__);
    #endif

    replyRequestSelection(X, False);
    return;
  }

  /* the selection in this request is none we own. */
  {
    int i = nxagentFindLastSelectionOwnerIndex(X->xselectionrequest.selection);
    if (i == nxagentMaxSelections)
    {
      #ifdef DEBUG
      fprintf(stderr, "%s: not owning selection [%ld] - denying request.\n", __func__, X->xselectionrequest.selection);
      #endif

      replyRequestSelection(X, False);
      return;
    }
  }

  /* this is a special request like TARGETS or TIMESTAMP */
  if (!validServerTargets(X->xselectionrequest.target))
  {
    if (X->xselectionrequest.target == serverTARGETS)
    {
      /*
       * the selection request target is TARGETS. The requestor is
       * asking for a list of supported data formats.
       *
       * The selection does not matter here, we will return this for
       * PRIMARY and CLIPBOARD.
       *
       * The list is aligned with the one in nxagentConvertSelection.
       *
       * FIXME: the perfect solution should not just answer with a
       * hardcoded list but ask the real owner what format it
       * supports. The result should then be sent to the original
       * requestor.
       */

      long targets[] = {XA_STRING, serverUTF8_STRING, serverTEXT, serverCOMPOUND_TEXT, serverTARGETS, serverTIMESTAMP};
      int numTargets = sizeof(targets) / sizeof(targets[0]);

      #ifdef DEBUG
      {
        fprintf(stderr, "%s: Sending %d available targets:\n", __func__, numTargets);
        for (int i = 0; i < numTargets; i++)
        {
          char *s = XGetAtomName(nxagentDisplay, targets[i]);
          fprintf(stderr, "%s: %ld %s\n", __func__, targets[i], s);
          SAFE_XFree(s);
        }
        fprintf(stderr, "\n");
      }
      #endif

      /*
       * pass on the requested list by setting the property provided
       * by the requestor accordingly.
       */
      XChangeProperty(nxagentDisplay,
                      X->xselectionrequest.requestor,
                      X->xselectionrequest.property,
                      XInternAtom(nxagentDisplay, "ATOM", 0),
                      32,
                      PropModeReplace,
                      (unsigned char*)&targets,
                      numTargets);

      replyRequestSelection(X, True);
    }
    else if (X->xselectionrequest.target == serverTIMESTAMP)
    {
      /*
       * Section 2.6.2 of the ICCCM states:
       * TIMESTAMP - To avoid some race conditions, it is important
       * that requestors be able to discover the timestamp the owner
       * used to acquire ownership. Until and unless the protocol is
       * changed so that a GetSelectionOwner request returns the
       * timestamp used to acquire ownership, selection owners must
       * support conversion to TIMESTAMP, returning the timestamp they
       * used to obtain the selection.
       *
       * FIXME: ensure we are reporting an _external_ timestamp
       * FIXME: for a 32 bit property list we need to pass a "long" array, not "char"!
       * FIXME: selection has already been checked above, so we do not need to check again here
       */

      int i = nxagentFindLastSelectionOwnerIndex(X->xselectionrequest.selection);
      if (i < nxagentMaxSelections)
      {
        XChangeProperty(nxagentDisplay,
                        X->xselectionrequest.requestor,
                        X->xselectionrequest.property,
                        XA_INTEGER,
                        32,
                        PropModeReplace,
                        (unsigned char *) &lastSelectionOwner[i].lastTimeChanged,
                        1);
        replyRequestSelection(X, True);
      }
    }
    else
    {
      /*
       * unknown special request - probably bug! Check if this code handles all cases
       * that are handled in validServerTargets!
       */
      #ifdef DEBUG
      fprintf(stderr, "%s: unknown special target [%ld] - denying request.\n", __func__, X->xselectionrequest.target);
      #endif
      replyRequestSelection(X, False);
    }
    return;
  }

  /*
   * reaching this means the request is a normal, valid request. We
   * can process it now.
   */

  /*
   * This is required for nxagentGetClipboardWindow.
   */
  serverLastRequestedSelection = X->xselectionrequest.selection;

  /* find the index of the requested selection */
  int i = nxagentFindLastSelectionOwnerIndex(X->xselectionrequest.selection);
  if (i < nxagentMaxSelections)
  {
#if 0
    if (lastClientWindowPtr != NULL && IS_INTERNAL_OWNER(i))
    {
      /*
       * Request the real X server to transfer the selection content
       * to the NX_CUT_BUFFER_SERVER property of the serverWindow.
       * FIXME: document how we can end up here
       *
       * It looks like this is only reached when the real X server is
       * on Windows. I suspect that this is connected to the windows X
       * server (VcXSrv in my test) pushing PRIMARY or CLIPBOARD
       * content to the Windows clipboard whenever they change.
       */
      XConvertSelection(nxagentDisplay, CurrentSelections[i].selection,
                            X->xselectionrequest.target, serverTransToAgentProperty,
                                serverWindow, lastClientTime);

      #ifdef DEBUG
      char *strTarget = XGetAtomName(nxagentDisplay, X->xselectionrequest.target);
      char *strSelection = XGetAtomName(nxagentDisplay, CurrentSelections[i].selection);
      char *strProperty = XGetAtomName(nxagentDisplay, serverTransToAgentProperty);
      fprintf(stderr, "%s: Sent XConvertSelection: selection [%d][%s] target [%ld][%s] property [%ld][%s] window [0x%x] time [%u] .\n", __func__,
	      CurrentSelections[i].selection, strSelection,
	      X->xselectionrequest.target, strTarget,
	      serverTransToAgentProperty, strProperty,
	      serverWindow, lastClientTime);
      #endif
    }
    else
    {
      /*
       * if one of our clients owns the selection we ask it to copy
       * the selection to the clientCutProperty on nxagent's root
       * window in the first step. We then later push that property's
       * content to the real X server.
       */
      if (IS_INTERNAL_OWNER(i) &&
	      (nxagentOption(Clipboard) == ClipboardServer ||
	           nxagentOption(Clipboard) == ClipboardBoth))
      {
        /*
         * store who on the real X server requested the data and how
         * and where it wants to have it
         */
        lastServerProperty = X->xselectionrequest.property;
        lastServerRequestor = X->xselectionrequest.requestor;
        lastServerTarget = X->xselectionrequest.target;
        lastServerTime = X->xselectionrequest.time;

        /* by dimbor */
        if (lastServerTarget != XA_STRING)
            lastServerTarget = serverUTF8_STRING;

        /* prepare the request (like XConvertSelection, but internally) */
        xEvent x = {0};
        x.u.u.type = SelectionRequest;
        x.u.selectionRequest.time = GetTimeInMillis();
        x.u.selectionRequest.owner = lastSelectionOwner[i].window;
        x.u.selectionRequest.selection = CurrentSelections[i].selection;
        x.u.selectionRequest.property = clientCutProperty;
        x.u.selectionRequest.requestor = screenInfo.screens[0]->root->drawable.id; /* Fictitious window.*/

        /*
         * Don't send the same window, some programs are clever and
         * verify cut and paste operations inside the same window and
         * don't Notify at all.
         *
         * x.u.selectionRequest.requestor = lastSelectionOwnerWindow;
         */

        /* by dimbor (idea from zahvatov) */
        if (X->xselectionrequest.target != XA_STRING)
          x.u.selectionRequest.target = clientUTF8_STRING;
        else
          x.u.selectionRequest.target = XA_STRING;

        sendEventToClient(lastSelectionOwner[i].client, &x);

        #ifdef DEBUG
        fprintf(stderr, "%s: sent SelectionRequest event to client %s property [%d][%s]" \
                "target [%d][%s] requestor [0x%x].\n", __func__,
                nxagentClientInfoString(lastSelectionOwner[i].client),
                x.u.selectionRequest.property, NameForAtom(x.u.selectionRequest.property),
                x.u.selectionRequest.target, NameForAtom(x.u.selectionRequest.target),
                x.u.selectionRequest.requestor);
        #endif
      }
      else
      {
        /* deny the request */
        replyRequestSelection(X, False);
      }
    }
  }
}

/*
 * end current selection transfer by sending a notification to the
 * client and resetting the corresponding variables and the state
 * machine. If success is False send a None reply, meaning "request
 * denied/failed"
 * Use SELECTION_SUCCESS and SELECTION_FAULT macros for success.
 */
static void endTransfer(Bool success)
{
  if (lastClientClientPtr == NULL)
  {
    #ifdef DEBUG
    fprintf(stderr, "%s: lastClientClientPtr is NULL - doing nothing.\n", __func__);
    #endif
  }
  else
  {
    #ifdef DEBUG
    if (success == SELECTION_SUCCESS)
      fprintf(stderr, "%s: sending notification to client %s, property [%d][%s]\n", __func__,
                  nxagentClientInfoString(lastClientClientPtr), lastClientProperty, NameForAtom(lastClientProperty));
    else
      fprintf(stderr, "%s: sending negative notification to client %s\n", __func__,
                  nxagentClientInfoString(lastClientClientPtr));
    #endif

    sendSelectionNotifyEventToClient(lastClientClientPtr,
                                     lastClientTime,
                                     lastClientRequestor,
                                     lastClientSelection,
                                     lastClientTarget,
                                     success == SELECTION_SUCCESS ? lastClientProperty : None);
  }

  /*
   * Enable further requests from clients.
   */
  setClientSelectionStage(SelectionStageNone);
}

static void transferSelection(int resource)
{
  if (lastClientClientPtr -> index != resource)
  {
    #ifdef DEBUG
    fprintf (stderr, "%s: WARNING! Inconsistent resource [%d] with current client %s.\n", __func__,
                 resource, nxagentClientInfoString(lastClientClientPtr));
    #endif

    endTransfer(SELECTION_FAULT);

    return;
  }

  switch (lastClientStage)
  {
    case SelectionStageQuerySize:
    {
      int result;

      printClientSelectionStage();

      /*
       * Don't get data yet, just get size. We skip this stage in
       * current implementation and go straight to the data.
       */

      nxagentLastClipboardClient = NXGetCollectPropertyResource(nxagentDisplay);

      if (nxagentLastClipboardClient == -1)
      {
        #ifdef WARNING
        fprintf(stderr, "%s: WARNING! Asynchronous GetProperty queue full.\n", __func__);
        #endif

        result = -1;
      }
      else
      {
        result = NXCollectProperty(nxagentDisplay,
                                   nxagentLastClipboardClient,
                                   serverWindow,
                                   serverTransToAgentProperty,
                                   0,
                                   0,
                                   False,
                                   AnyPropertyType);
      }

      if (result == -1)
      {
        #ifdef DEBUG
        fprintf (stderr, "%s: Aborting selection notify procedure for client %s.\n", __func__,
                     nxagentClientInfoString(lastClientClientPtr));
        #endif

        endTransfer(SELECTION_FAULT);

        return;
      }

      setClientSelectionStage(SelectionStageWaitSize);

      NXFlushDisplay(nxagentDisplay, NXFlushLink);

      break;
    }
    case SelectionStageQueryData:
    {
      int result;

      printClientSelectionStage();

      /*
       * Request the selection data now.
       */

      #ifdef DEBUG
      fprintf(stderr, "%s: Getting property content from remote server.\n", __func__);
      #endif

      nxagentLastClipboardClient = NXGetCollectPropertyResource(nxagentDisplay);

      if (nxagentLastClipboardClient == -1)
      {
        #ifdef WARNING
        fprintf(stderr, "%s: WARNING! Asynchronous GetProperty queue full.\n", __func__);
        #endif

        result = -1;
      }
      else
      {
        result = NXCollectProperty(nxagentDisplay,
                                   nxagentLastClipboardClient,
                                   serverWindow,
                                   serverTransToAgentProperty,
                                   0,
                                   lastClientPropertySize,
                                   False,
                                   AnyPropertyType);
      }

      if (result == -1)
      {
        #ifdef DEBUG
        fprintf (stderr, "%s: Aborting selection notify procedure for client %s.\n", __func__,
                     nxagentClientInfoString(lastClientClientPtr));
        #endif

        endTransfer(SELECTION_FAULT);

        return;
      }

      setClientSelectionStage(SelectionStageWaitData);

      /* we've seen situations where you had to move the mouse or press a
         key to let the transfer complete. Flushing here fixed it */
      NXFlushDisplay(nxagentDisplay, NXFlushLink);

      break;
    }
    default:
    {
      #ifdef DEBUG
      fprintf (stderr, "%s: WARNING! Inconsistent state [%s] for client %s.\n", __func__,
                   getClientSelectionStageString(lastClientStage), nxagentClientInfoString(lastClientClientPtr));
      #endif

      break;
    }
  }
}

/*
   Called from Events.c/nxagentHandlePropertyNotify

   This event is generated after XChangeProperty(), XDeleteProperty() or
   XGetWindowProperty(delete=True)
*/

void nxagentCollectPropertyEvent(int resource)
{
  Atom                  atomReturnType;
  int                   resultFormat;
  unsigned long         ulReturnItems;
  unsigned long         ulReturnBytesLeft;
  unsigned char         *pszReturnData = NULL;

  /*
   * We have received the notification so we can safely retrieve data
   * from the client structure.
   */

  int result = NXGetCollectedProperty(nxagentDisplay,
                                      resource,
                                      &atomReturnType,
                                      &resultFormat,
                                      &ulReturnItems,
                                      &ulReturnBytesLeft,
                                      &pszReturnData);

  nxagentLastClipboardClient = -1;

  if (result == 0)
  {
    #ifdef DEBUG
    fprintf (stderr, "%s: Failed to get reply data.\n", __func__);
    #endif

    endTransfer(SELECTION_FAULT);
  }
  else if (resultFormat != 8 && resultFormat != 16 && resultFormat != 32)
  {
    #ifdef DEBUG
    fprintf (stderr, "%s: WARNING! Invalid property format.\n", __func__);
    #endif

    endTransfer(SELECTION_FAULT);
  }
  else
  {
    switch (lastClientStage)
    {
      case SelectionStageWaitSize:
      {
        printClientSelectionStage();
        #ifdef DEBUG
        fprintf (stderr, "%s: Got size notify event for client %s.\n", __func__, nxagentClientInfoString(lastClientClientPtr));
        #endif

        if (ulReturnBytesLeft == 0)
        {
          #ifdef DEBUG
          fprintf (stderr, "%s: data size is [0] - aborting selection notify procedure.\n", __func__);
          #endif

          endTransfer(SELECTION_FAULT);
        }
        else
        {
          #ifdef DEBUG
          fprintf(stderr, "%s: Got property size [%lu] from remote server.\n", __func__, ulReturnBytesLeft);
          #endif

          /*
           * Request the selection data now.
           */
          lastClientPropertySize = ulReturnBytesLeft;
          setClientSelectionStage(SelectionStageQueryData);

          transferSelection(resource);
        }
        break;
      }
      case SelectionStageWaitData:
      {
        printClientSelectionStage();
        #ifdef DEBUG
        fprintf (stderr, "%s: Got data notify event for waiting client %s.\n", __func__, nxagentClientInfoString(lastClientClientPtr));
        #endif

        if (ulReturnBytesLeft != 0)
        {
          #ifdef DEBUG
          fprintf (stderr, "%s: not all content could be retrieved - [%lu] bytes left - aborting selection notify procedure.\n", __func__, ulReturnBytesLeft);
          #endif

          endTransfer(SELECTION_FAULT);
        }
        else
        {
          #ifdef DEBUG
          fprintf(stderr, "%s: Got property content from remote server. size [%lu] bytes.\n", __func__, (ulReturnItems * resultFormat / 8));
          #endif

          ChangeWindowProperty(lastClientWindowPtr,
                               lastClientProperty,
                               lastClientTarget,
                               resultFormat, PropModeReplace,
                               ulReturnItems, pszReturnData, 1);

          #ifdef DEBUG
          fprintf(stderr, "%s: Selection property [%d][%s] changed to [\"%*.*s\"...]\n", __func__,
                      lastClientProperty, validateString(NameForAtom(lastClientProperty)),
                          (int)(min(20, ulReturnItems * resultFormat / 8)),
                              (int)(min(20, ulReturnItems * resultFormat / 8)),
                                 pszReturnData);
          #endif

          endTransfer(SELECTION_SUCCESS);
        }
        break;
      }
      default:
      {
        #ifdef DEBUG
        fprintf (stderr, "%s: WARNING! Inconsistent state [%s] for client %s.\n", __func__,
		 getClientSelectionStageString(lastClientStage), nxagentClientInfoString(lastClientClientPtr));
        #endif
        break;
      }
    }
  }
  SAFE_XFree(pszReturnData);
}

/*
 * This is _only_ called from Events.c dispatch loop on reception of a
 * SelectionNotify event from the real X server. These events are
 * sent out by nxagent itself!
 */
void nxagentHandleSelectionNotifyFromXServer(XEvent *X)
{
  if (!agentClipboardInitialized)
  {
    #ifdef DEBUG
    fprintf(stderr, "%s: clipboard not initialized - doing nothing.\n", __func__);
    #endif
    return;
  }

  #ifdef DEBUG
  XSelectionEvent * e = (XSelectionEvent *)X;
  {
    char * s = XGetAtomName(nxagentDisplay, e->property);
    char * t = XGetAtomName(nxagentDisplay, e->target);
    fprintf(stderr, "%s: SelectionNotify event from real X server, property "\
            "[%ld][%s] requestor [0x%lx] selection [%s] target [%ld][%s] time [%lu] send_event [%d].\n",
            __func__, e->property, validateString(s), e->requestor,
	    XGetAtomName(nxagentDisplay, e->selection), e->target,
            validateString(t), e->time, e->send_event);
    SAFE_XFree(s);
    SAFE_XFree(t);
  }
  #endif

  printClientSelectionStage();

  if (lastClientWindowPtr != NULL)
  {
    /*
     * We reach here after a paste inside the nxagent, triggered by
     * the XConvertSelection call in nxagentConvertSelection(). This
     * means that data we need has been transferred to the
     * serverTransToAgentProperty of the serverWindow (our window on
     * the real X server). We now need to transfer it to the original
     * requestor, which is stored in the lastClient* variables.
     */

    #ifdef DEBUG
    nxagentDumpClipboardStat();
    #endif
    if (lastClientStage == SelectionStageNone)
    {
      if (X->xselection.property == serverTransToAgentProperty)
      {
        #ifdef DEBUG
        fprintf(stderr, "%s: Starting selection transferral for client %s.\n", __func__,
                nxagentClientInfoString(lastClientClientPtr));
        #endif

        /*
         * The state machine is able to work in two phases. In the first
         * phase we get the size of property data, in the second we get
         * the actual data. We save a round-trip by requesting a prede-
         * termined amount of data in a single GetProperty and by discar-
         * ding the remaining part. This is not the optimal solution (we
         * could get the remaining part if it doesn't fit in a single
         * reply) but, at least with text, it should work in most situa-
         * tions.
         */

        setClientSelectionStage(SelectionStageQueryData);
        lastClientPropertySize = 262144;

        transferSelection(lastClientClientPtr -> index);
      }
      else
      {
        #ifdef DEBUG
        fprintf(stderr, "%s: SelectionNotify event reporting failed conversion.\n", __func__);
        #endif
	  //        endTransfer(SELECTION_FAULT);
      }
    }
    else
    {
      #ifdef DEBUG
      fprintf(stderr, "%s: WARNING! Resetting selection transferral for client [%d] because of unexpected stage.\n", __func__,
	      CLINDEX(lastClientClientPtr));
      #endif

      endTransfer(SELECTION_FAULT);
    }
  }
  else
  {
    int i = nxagentFindLastSelectionOwnerIndex(X->xselection.selection);
    if (i < nxagentMaxSelections)
    {
      /* if the last owner was an internal one, read the
       * clientCutProperty and push the contents to the
       * lastServerRequestor on the real X server.
       */
      if (IS_INTERNAL_OWNER(i) &&
             lastSelectionOwner[i].windowPtr != NULL &&
                 X->xselection.property == serverTransFromAgentProperty)
      {
        Atom            atomReturnType;
        int             resultFormat;
        unsigned long   ulReturnItems;
        unsigned long   ulReturnBytesLeft;
        unsigned char   *pszReturnData = NULL;

        /* first get size values ... */
        int result = GetWindowProperty(lastSelectionOwner[i].windowPtr, clientCutProperty, 0, 0, False,
                                           AnyPropertyType, &atomReturnType, &resultFormat,
                                               &ulReturnItems, &ulReturnBytesLeft, &pszReturnData);

        #ifdef DEBUG
        fprintf(stderr, "%s: GetWindowProperty() window [0x%x] property [%d] returned [%s]\n", __func__,
                    lastSelectionOwner[i].window, clientCutProperty, getXErrorString(result));
        #endif
        if (result == BadAlloc || result == BadAtom ||
                result == BadWindow || result == BadValue)
        {
          lastServerProperty = None;
        }
        else
        {
          /* ... then use the size values for the actual request */
          result = GetWindowProperty(lastSelectionOwner[i].windowPtr, clientCutProperty, 0,
                                         ulReturnBytesLeft, False, AnyPropertyType, &atomReturnType,
                                             &resultFormat, &ulReturnItems, &ulReturnBytesLeft,
                                                 &pszReturnData);
          #ifdef DEBUG
          fprintf(stderr, "%s: GetWindowProperty() window [0x%x] property [%d] returned [%s]\n", __func__,
                      lastSelectionOwner[i].window, clientCutProperty, getXErrorString(result));
          #endif

          if (result == BadAlloc || result == BadAtom ||
                  result == BadWindow || result == BadValue)
          {
            lastServerProperty = None;
          }
          else
          {
            /* Fill the property on the initial requestor with the requested data */
            /* The XChangeProperty source code reveals it will always
               return 1, no matter what, so no need to check the result */
            /* FIXME: better use the format returned by above request */
            XChangeProperty(nxagentDisplay,
                            lastServerRequestor,
                            lastServerProperty,
                            lastServerTarget,
                            8,
                            PropModeReplace,
                            pszReturnData,
                            ulReturnItems);

            #ifdef DEBUG
            {
              char *s = XGetAtomName(nxagentDisplay, lastServerProperty);
              fprintf(stderr, "%s: XChangeProperty sent to window [0x%x] for property [%ld][%s] value [\"%*.*s\"...]\n",
                      __func__,
                      lastServerRequestor,
                      lastServerProperty,
                      s,
                      (int)(min(20, ulReturnItems * 8 / 8)),
                      (int)(min(20, ulReturnItems * 8 / 8)),
                      pszReturnData);
              SAFE_XFree(s);
            }
            #endif
          }

          /* FIXME: free it or not? */
          /*
           * SAFE_XFree(pszReturnData);
           */
        }

        /*
         * inform the initial requestor that the requested data has
         * arrived in the desired property. If we have been unable to
         * get the data from the owner XChangeProperty will not have
         * been called and lastServerProperty will be None which
         * effectively will send a "Request denied" to the initial
         * requestor.
         */
        XSelectionEvent eventSelection = {
          .requestor = lastServerRequestor,
          .selection = X->xselection.selection,
          /* .target = X->xselection.target, */
          .target    = lastServerTarget,
          .property  = lastServerProperty,
          .time      = lastServerTime,
          /* .time   = CurrentTime */
        };
        #ifdef DEBUG
        fprintf(stderr, "%s: Sending SelectionNotify event to requestor [%p].\n", __func__,
                (void *)eventSelection.requestor);
        #endif

        sendSelectionNotifyEventToServer(&eventSelection);

        lastServerRequestor = None; /* allow further request */
      }
    }
  }
}

#if 0
/* FIXME: currently unused */
/*
 * Let nxagent's serverWindow acquire the selection. All requests from
 * the real X server (or its clients) will be sent to this window. The
 * real X server never communicates with our windows directly.
 */
static void resetSelectionOwner(void)
{
  if (lastServerRequestor != None)
  {
    /*
     * we are in the process of communicating back and forth between
     * real X server and nxagent's clients - let's not disturb.
     */
    #if defined(TEST) || defined(DEBUG)
    fprintf(stderr, "%s: WARNING! Requestor window [0x%x] already found.\n", __func__,
                lastServerRequestor);
    #endif

    /* FIXME: maybe we should put back the event that lead us here. */
    return;
  }

  /*
   * Only for PRIMARY and CLIPBOARD selections.
   */

  for (int i = 0; i < nxagentMaxSelections; i++)
  {
    XSetSelectionOwner(nxagentDisplay, lastSelectionOwner[i].selection, serverWindow, CurrentTime);

    #ifdef DEBUG
    fprintf(stderr, "%s: Reset selection state for selection [%d].\n", __func__, i);
    #endif

    clearSelectionOwner(i);
  }

  setClientSelectionStage(SelectionStageNone);

  /* Hmm, this is already None when reaching this */
  lastServerRequestor = None;
}
#endif

#ifdef NXAGENT_CLIPBOARD

/*
 * The callback is called from dix. This is the normal operation
 * mode. The callback is also called when nxagent gets XFixes events
 * from the real X server. In that case the Trap is set and the
 * callback will do nothing.
 */

void nxagentSetSelectionCallback(CallbackListPtr *callbacks, void *data,
                                   void *args)
{
  /*
   * Only act if the trap is unset. The trap indicates that we are
   * triggered by an XFixes clipboard event originating from the real
   * X server. In that case we do not want to propagate back changes
   * to the real X server, because it already knows about them and we
   * would end up in an infinite loop of events. If there was a better
   * way to identify that situation during callback processing we
   * could get rid of the Trap...
  */
  if (nxagentExternalClipboardEventTrap != 0)
  {
    #ifdef DEBUG
    fprintf(stderr, "%s: Trap is set, doing nothing\n", __func__);
    #endif
    return;
  }

  SelectionInfoRec *info = (SelectionInfoRec *)args;

  Selection * pCurSel = (Selection *)info->selection;

  #ifdef DEBUG
  fprintf(stderr, "%s: pCurSel->lastTimeChanged [%u]\n", __func__, pCurSel->lastTimeChanged.milliseconds);
  #endif

  if (info->kind == SelectionSetOwner)
  {
    #ifdef DEBUG
    fprintf(stderr, "%s: called with SelectionCallbackKind SelectionSetOwner\n", __func__);
    fprintf(stderr, "%s: pCurSel->pWin [0x%x]\n", __func__, WINDOWID(pCurSel->pWin));
    fprintf(stderr, "%s: pCurSel->selection [%s]\n", __func__, NameForAtom(pCurSel->selection));
    #endif

    if (pCurSel->pWin != NULL &&
        nxagentOption(Clipboard) != ClipboardNone && /* FIXME: shouldn't we also check for != ClipboardClient? */
        (pCurSel->selection == XA_PRIMARY ||
         pCurSel->selection == clientCLIPBOARD))
    {
      #ifdef DEBUG
      fprintf(stderr, "%s: calling setSelectionOwner\n", __func__);
      #endif
      setSelectionOwner(pCurSel);
    }
  }
  else if (info->kind == SelectionWindowDestroy)
  {
    #ifdef DEBUG
    fprintf(stderr, "%s: called with SelectionCallbackKind SelectionWindowDestroy\n", __func__);
    #endif
  }
  else if (info->kind == SelectionClientClose)
  {
    #ifdef DEBUG
    fprintf(stderr, "%s: called with SelectionCallbackKind SelectionClientClose\n", __func__);
    #endif
  }
  else
  {
    #ifdef DEBUG
    fprintf(stderr, "%s: called with unknown SelectionCallbackKind\n", __func__);
    #endif
  }
}
#endif

/*
 * This is called from the nxagentSetSelectionCallback, so it is using
 * internal Atoms
 */
static void setSelectionOwner(Selection *pSelection)
{
  if (!agentClipboardInitialized)
  {
    #ifdef DEBUG
    fprintf(stderr, "%s: clipboard not initialized - doing nothing.\n", __func__);
    #endif
    return;
  }

  #ifdef DEBUG
  fprintf(stderr, "%s: Setting selection owner to serverwindow ([0x%x]).\n", __func__,
              serverWindow);
  #endif

  #if defined(TEST) || defined(DEBUG)
  if (lastServerRequestor != None)
  {
    /*
     * we are in the process of communicating back and forth between
     * real X server and nxagent's clients - let's not disturb
     * FIXME: by continuing after the warning were ARE disturbing!
     */
    fprintf (stderr, "%s: WARNING! Requestor window [0x%x] already set.\n", __func__,
                 lastServerRequestor);
  }
  #endif

  int i = nxagentFindCurrentSelectionIndex(pSelection->selection);
  if (i < NumCurrentSelections)
  {
    #ifdef DEBUG
    fprintf(stderr, "%s: lastSelectionOwner.client %s -> %s\n", __func__,
                nxagentClientInfoString(lastSelectionOwner[i].client),
                    nxagentClientInfoString(pSelection->client));
    fprintf(stderr, "%s: lastSelectionOwner.window [0x%x] -> [0x%x]\n", __func__,
            lastSelectionOwner[i].window, pSelection->window);
    fprintf(stderr, "%s: lastSelectionOwner.windowPtr [%p] -> [%p] [0x%x] (serverWindow: [0x%x])\n", __func__,
            (void *)lastSelectionOwner[i].windowPtr, (void *)pSelection->pWin,
            nxagentWindow(pSelection->pWin), serverWindow);
    fprintf(stderr, "%s: lastSelectionOwner.lastTimeChanged [%u]\n", __func__,
            lastSelectionOwner[i].lastTimeChanged);
    #endif

    /*
     * inform the real X server that our serverWindow is the
     * clipboard owner.
     */
    XSetSelectionOwner(nxagentDisplay, lastSelectionOwner[i].selection, serverWindow, CurrentTime);

    /*
     * The real owner window (inside nxagent) is stored in
     * lastSelectionOwner.window.  lastSelectionOwner.windowPtr
     * points to the struct that contains all information about the
     * owner window.
     */
    storeSelectionOwner(i, pSelection);
  }

  setClientSelectionStage(SelectionStageNone);

  lastServerRequestor = None;

/*
FIXME

   if (XGetSelectionOwner(nxagentDisplay,pSelection->selection) == serverWindow)
   {
      fprintf (stderr, "%s: SetSelectionOwner OK\n", __func__);

      lastSelectionOwnerSelection = pSelection;
      lastSelectionOwnerClient = pSelection->client;
      lastSelectionOwnerWindow = pSelection->window;
      lastSelectionOwnerWindowPtr = pSelection->pWin;

      setClientSelectionStage(SelectionStageNone);

      lastServerRequestor = None;
   }
   else fprintf (stderr, "%s: SetSelectionOwner failed\n", __func__);
*/
}

/*
 * This is called from dix (ProcConvertSelection) if an nxagent client
 * issues a ConvertSelection request. So all the Atoms are internal
 * return codes:
 * 0: let dix process the request
 * 1: don't let dix process the request
 */
int nxagentConvertSelection(ClientPtr client, WindowPtr pWin, Atom selection,
                                Window requestor, Atom property, Atom target, Time time)
{
  if (!agentClipboardInitialized)
  {
    #ifdef DEBUG
    fprintf(stderr, "%s: clipboard not initialized - doing nothing.\n", __func__);
    #endif
    return 0;
  }

  if (nxagentOption(Clipboard) == ClipboardServer)
  {
    #ifdef DEBUG
    fprintf(stderr, "%s: clipboard mode 'server' - doing nothing.\n", __func__);
    #endif
    return 0;
  }

  {
    int i = nxagentFindCurrentSelectionIndex(selection);
    if (i < NumCurrentSelections && IS_INTERNAL_OWNER(i))
    {
      /*
       * There is a client owner on the agent side, let normal dix stuff happen.
       */
      return 0;
    }
  }

  /*
   * if lastClientWindowPtr is set we are waiting for an answer from
   * the real X server. If that answer takes more than 5 seconds we
   * consider the conversion failed and tell our client about that.
   * The new request that lead us here is then processed.
   */
  if (lastClientWindowPtr != NULL)
  {
    #ifdef TEST
    fprintf(stderr, "%s: lastClientWindowPtr != NULL.\n", __func__);
    #endif

    if ((GetTimeInMillis() - lastClientReqTime) >= CONVERSION_TIMEOUT)
    {
      #ifdef DEBUG
      fprintf(stderr, "%s: timeout expired on last request, "
                  "notifying failure to client %s\n", __func__, nxagentClientInfoString(client));
      #endif

      /* notify the waiting client of failure */
      endTransfer(SELECTION_FAULT);
    }
    else
    {
      /*
       * we got another convert request while already waiting for an
       * answer from the real X server to a previous convert request,
       * which we cannot handle (yet). So return an error.
       */
      #ifdef DEBUG
      fprintf(stderr, "%s: got new request "
                  "before timeout expired on previous request, notifying failure to client %s\n",
                      __func__, nxagentClientInfoString(client));
      #endif

      sendSelectionNotifyEventToClient(client, time, requestor, selection, target, None);

      return 1;
    }
  }

  const char *strTarget = NameForAtom(target);

  #ifdef DEBUG
  fprintf(stderr, "%s: client %s requests sel [%s] "
              "on window [0x%x] prop [%d][%s] target [%d][%s].\n", __func__,
                  nxagentClientInfoString(client), validateString(NameForAtom(selection)), requestor,
                      property, validateString(NameForAtom(property)),
                          target, validateString(strTarget));
  #endif

  if (strTarget == NULL)
  {
    #ifdef DEBUG
    fprintf(stderr, "%s: cannot find name for target Atom [%d] - returning\n", __func__, target);
    #endif
    return 1;
  }

  /*
   * The selection request target is TARGETS. The requestor is asking
   * for a list of supported data formats.
   *
   * The list is aligned with the one in nxagentHandleSelectionRequestFromXServer.
   */
  if (target == clientTARGETS)
  {
    Atom targets[] = {XA_STRING, clientUTF8_STRING, clientTEXT, clientCOMPOUND_TEXT, clientTARGETS, clientTIMESTAMP};
    int numTargets = sizeof(targets) / sizeof(targets[0]);

    #ifdef DEBUG
    fprintf(stderr, "%s: available targets [%d]:\n", __func__, numTargets);
    for (int i = 0; i < numTargets; i++)
        fprintf(stderr, "%s:  %s\n", __func__, NameForAtom(targets[i]));
    fprintf(stderr, "\n");
    #endif

    ChangeWindowProperty(pWin,
                         property,
                         MakeAtom("ATOM", 4, 1),
                         sizeof(Atom)*8,
                         PropModeReplace,
                         numTargets,
                         &targets,
                         1);

    sendSelectionNotifyEventToClient(client, time, requestor, selection, target, property);

    return 1;
  }

  /*
   * Section 2.6.2 of the ICCCM states:
   * "TIMESTAMP - To avoid some race conditions, it is important
   * that requestors be able to discover the timestamp the owner
   * used to acquire ownership. Until and unless the protocol is
   * changed so that a GetSelectionOwner request returns the
   * timestamp used to acquire ownership, selection owners must
   * support conversion to TIMESTAMP, returning the timestamp they
   * used to obtain the selection."
   */
  if (target == clientTIMESTAMP)
  {
    int i = nxagentFindCurrentSelectionIndex(selection);
    if (i < NumCurrentSelections)
    {
      /*
       * "If the specified property is not None, the owner should place
       * the data resulting from converting the selection into the
       * specified property on the requestor window and should set the
       * property's type to some appropriate value, which need not be
       * the same as the specified target."
       */
      ChangeWindowProperty(pWin,
                           property,
                           XA_INTEGER,
                           32,
                           PropModeReplace,
                           1,
                           (unsigned char *) &lastSelectionOwner[i].lastTimeChanged,
                           1);

      sendSelectionNotifyEventToClient(client, time, requestor, selection, target, property);

      return 1;
    }
  }

  if (lastClientClientPtr == client && (GetTimeInMillis() - lastClientReqTime < ACCUM_TIME))
  {
    /*
     * The same client made consecutive requests of clipboard content
     * with less than 5 seconds time interval between them.
     * FIXME: this does not take the selection into account, so a
     * client requesting PRIMARY and CLIPBOARD would match here, too
     */

    #ifdef DEBUG
    fprintf(stderr, "%s: Consecutives request from client %s selection [%u] "
                "elapsed time [%u] clientAccum [%d]\n", __func__, nxagentClientInfoString(client),
                    selection, GetTimeInMillis() - lastClientReqTime, clientAccum);
    #endif

    clientAccum++;
  }
  else
  {
    /* reset clientAccum as now another client requested the clipboard content */
    if (lastClientClientPtr != client)
    {
      clientAccum = 0;
    }
  }

  if (target == clientTEXT ||
          target == XA_STRING ||
              target == clientCOMPOUND_TEXT ||
                  target == clientUTF8_STRING)
  {
    setClientSelectionStage(SelectionStageNone);

    /*
     * store the original requestor, we need that later after
     * serverTransToAgentProperty contains the desired selection content
     */
    lastClientRequestor = requestor;
    lastClientWindowPtr = pWin;
    lastClientClientPtr = client;
    lastClientTime = time;
    lastClientProperty = property;
    lastClientSelection = selection;
    lastClientTarget = target;

    /* if the last client request time is more than 5s ago update it. Why? */
    if ((GetTimeInMillis() - lastClientReqTime) >= CONVERSION_TIMEOUT)
      lastClientReqTime = GetTimeInMillis();

    if (selection == clientCLIPBOARD)
    {
      selection = lastSelectionOwner[nxagentClipboardSelection].selection;
    }

    /*
     * we only convert to either UTF8 or XA_STRING, despite accepting
     * TEXT and COMPOUND_TEXT.
     */
    XlibAtom p = serverTransToAgentProperty;
    XlibAtom t;
    #ifdef DEBUG
    char * pstr = "NX_CUT_BUFFER_SERVER";
    const char * tstr;
    #endif
    if (target == clientUTF8_STRING)
    {
      t = serverUTF8_STRING;
      #ifdef DEBUG
      tstr = szAgentUTF8_STRING;
      #endif
    }
    else
    {
      t = XA_STRING;
      #ifdef DEBUG
      tstr = validateString(NameForAtom(XA_STRING));
      #endif
    }

    #ifdef DEBUG
    fprintf(stderr, "%s: Sending XConvertSelection to real X server: requestor [0x%x] target [%ld][%s] property [%ld][%s] time [0][CurrentTime]\n", __func__,
            serverWindow, t, tstr, p, pstr);
    #endif

    UpdateCurrentTime();
    XConvertSelection(nxagentDisplay, selection, t, p, serverWindow, CurrentTime);

    /* FIXME: check result of XConvertSelection? */

    #ifdef DEBUG
    fprintf(stderr, "%s: Sent XConvertSelection with target [%s], property [%s]\n", __func__, tstr, pstr);
    #endif

    return 1;
  }
  else
  {
    /* deny request */
    #ifdef DEBUG
    fprintf(stderr, "%s: Unsupported target [%d][%s] - denying request\n", __func__, target,
                validateString(NameForAtom(target)));
    #endif
    sendSelectionNotifyEventToClient(client, time, requestor, selection, target, None);

    return 1;
  }
  return 0;
}

/*
 * This is _only_ called from ProcSendEvent in NXevents.c. It is used
 * to send a SelectionNotify event to our server window which will
 * trigger the dispatch loop in Events.c to run
 * nxagentHandleSelectionNotifyFromXServer which in turn will take
 * care of transferring the selection content from the owning client
 * to a property of the server window.
 *
 * Returning 1 here means the client request will not be further
 * handled by dix. Returning 0 means a SelectionNotify event being
 * pushed out to our clients.
 *
 * From https://tronche.com/gui/x/xlib/events/client-communication/selection.html:
 * "This event is generated by the X server in response to a
 * ConvertSelection protocol request when there is no owner for the
 * selection. When there is an owner, it should be generated by the
 * owner of the selection by using XSendEvent()."
 */
int nxagentSendNotify(xEvent *event)
{
  #ifdef DEBUG
  fprintf(stderr, "%s: Got called.\n", __func__);
  #endif

  if (!agentClipboardInitialized)
  {
    #ifdef DEBUG
    fprintf(stderr, "%s: clipboard not initialized - doing nothing.\n", __func__);
    #endif
    return 0;
  }

  #ifdef DEBUG
  fprintf(stderr, "%s: property is [%d][%s].\n", __func__,
          event->u.selectionNotify.property,
          NameForAtom(event->u.selectionNotify.property));
  fprintf(stderr, "%s: requestor is [0x%x].\n", __func__, event->u.selectionNotify.requestor);
  fprintf(stderr, "%s: lastServerRequestor is [0x%x].\n", __func__, lastServerRequestor);
  #endif

  /*
   * If we have nested sessions there are situations where we do not
   * need to send out anything to the real X server because
   * communication happens completely between our own clients (some of
   * which can be nxagents themselves). In that case we return 0 (tell
   * dix to go on) and do nothing!
   */
  if (event->u.selectionNotify.property != clientCutProperty || lastServerRequestor == None)
  {
    #ifdef DEBUG
    fprintf(stderr, "%s: sent nothing.\n", __func__);
    #endif
    return 0;
  }
  else
  {
    /*
     * Setup selection notify event to real server.
     *
     * .property must be a server-side Atom. As this property is only
     * set on our serverWindow and normally there are few other
     * properties except serverTransToAgentProperty, the only thing
     * we need to ensure is that the internal Atom clientCutProperty
     * differs from the server-side serverTransToAgentProperty
     * Atom. The actual name is not important. To be clean here we use
     * a separate serverTransFromAgentProperty.
     */

    XSelectionEvent eventSelection = {
      .requestor = serverWindow,
      .selection = event->u.selectionNotify.selection,
      .target    = event->u.selectionNotify.target,
      .property  = serverTransFromAgentProperty,
      .time      = CurrentTime,
    };

    /*
     * On the real server, the right CLIPBOARD atom is
     * XInternAtom(nxagentDisplay, "CLIPBOARD", 1), which is stored in
     * lastSelectionOwner[nxagentClipboardSelection].selection. For
     * PRIMARY there's nothing to map because that is identical on all
     * X servers (defined in Xatom.h).
     */

    if (event->u.selectionNotify.selection == clientCLIPBOARD)
    {
      eventSelection.selection = lastSelectionOwner[nxagentClipboardSelection].selection;
    }

    /*
     * .target must be translated, too, as a client on the real
     * server is requested to fill our property and it needs to know
     * the format.
     */

    if (event->u.selectionNotify.target == clientUTF8_STRING)
    {
      eventSelection.target = serverUTF8_STRING;
    }
    else if (event->u.selectionNotify.target == clientTEXT)
    {
      eventSelection.target = serverTEXT;
    }
    else if (event->u.selectionNotify.target == clientCOMPOUND_TEXT)
    {
      eventSelection.target = serverCOMPOUND_TEXT;
    }
    else
    {
      eventSelection.target = XA_STRING;
    }

    #ifdef DEBUG
    fprintf(stderr, "%s: mapping local to remote Atom: [%d] -> [%ld] [%s]\n", __func__,
            event->u.selectionNotify.selection, eventSelection.selection,
            NameForAtom(event->u.selectionNotify.selection));
    fprintf(stderr, "%s: mapping local to remote Atom: [%d] -> [%ld] [%s]\n", __func__,
            event->u.selectionNotify.target, eventSelection.target,
            NameForAtom(event->u.selectionNotify.target));
    fprintf(stderr, "%s: mapping local to remote Atom: [%d] -> [%ld] [%s]\n", __func__,
            event->u.selectionNotify.property, eventSelection.property,
            NameForAtom(event->u.selectionNotify.property));
    #endif

    sendSelectionNotifyEventToServer(&eventSelection);

    return 1;
  }
}

/*
 * This is called from NXproperty.c to determine if a client sets the
 * property we are waiting for.
 * FIXME: in addition we should check if the client is the one we expect
 */
WindowPtr nxagentGetClipboardWindow(Atom property)
{
  int i = nxagentFindLastSelectionOwnerIndex(serverLastRequestedSelection);
  if (i < nxagentMaxSelections &&
          property == clientCutProperty &&
              lastSelectionOwner[i].windowPtr != NULL)
  {
    #ifdef DEBUG
    fprintf(stderr, "%s: Returning last [%ld] selection owner window [%p] (0x%x).\n", __func__,
            lastSelectionOwner[i].selection,
            (void *)lastSelectionOwner[i].windowPtr, WINDOWID(lastSelectionOwner[i].windowPtr));
    #endif

    return lastSelectionOwner[i].windowPtr;
  }
  else
  {
    return NULL;
  }
}

/*
 * Initialize the clipboard
 * Returns: True for success else False
 */
Bool nxagentInitClipboard(WindowPtr pWin)
{
  #ifdef DEBUG
  fprintf(stderr, "%s: Got called.\n", __func__);
  #endif

  #ifdef NXAGENT_TIMESTAMP
  {
    fprintf(stderr, "%s: Clipboard init starts at [%lu] ms.\n", __func__,
            GetTimeInMillis() - startTime);
  }
  #endif

  agentClipboardInitialized = False;
  serverWindow = nxagentWindow(pWin);

  if (!nxagentReconnectTrap)
  {
    SAFE_free(lastSelectionOwner);

    lastSelectionOwner = (SelectionOwner *) malloc(nxagentMaxSelections * sizeof(SelectionOwner));

    if (lastSelectionOwner == NULL)
    {
      FatalError("nxagentInitClipboard: Failed to allocate memory for the clipboard selections.\n");
    }
    initSelectionOwner(nxagentPrimarySelection, XA_PRIMARY);
    initSelectionOwner(nxagentClipboardSelection, nxagentAtoms[10]);   /* CLIPBOARD */
  }
  else
  {
    /* the clipboard selection atom might have changed on a new X
       server. Primary is constant. */
    lastSelectionOwner[nxagentClipboardSelection].selection = nxagentAtoms[10];   /* CLIPBOARD */
  }

  serverTARGETS = nxagentAtoms[6];  /* TARGETS */
  serverTEXT = nxagentAtoms[7];  /* TEXT */
  serverCOMPOUND_TEXT = nxagentAtoms[16]; /* COMPOUND_TEXT */
  serverUTF8_STRING = nxagentAtoms[12]; /* UTF8_STRING */
  serverTIMESTAMP = nxagentAtoms[11];   /* TIMESTAMP */

  /*
   * Server side properties to hold pasted data.
   * see nxagentSendNotify for an explanation
   */
  serverTransFromAgentProperty = nxagentAtoms[15]; /* NX_SELTRANS_FROM_AGENT */
  serverTransToAgentProperty = nxagentAtoms[5];  /* NX_CUT_BUFFER_SERVER */

  if (serverTransToAgentProperty == None)
  {
    #ifdef PANIC
    fprintf(stderr, "%s: PANIC! Could not create %s atom\n", __func__, "NX_CUT_BUFFER_SERVER");
    #endif

    return False;
  }

  #ifdef TEST
  fprintf(stderr, "%s: Setting owner of selection [%d][%s] on window 0x%x\n", __func__,
              (int) serverTransToAgentProperty, "NX_CUT_BUFFER_SERVER", serverWindow);
  #endif

  XSetSelectionOwner(nxagentDisplay, serverTransToAgentProperty, serverWindow, CurrentTime);

  if (XQueryExtension(nxagentDisplay,
                      "XFIXES",
                      &nxagentXFixesInfo.Opcode,
                      &nxagentXFixesInfo.EventBase,
                      &nxagentXFixesInfo.ErrorBase) == 0)
  {
    ErrorF("Unable to initialize XFixes extension.\n");
  }
  else
  {
    #ifdef TEST
    fprintf(stderr, "%s: Registering for XFixesSelectionNotify events.\n", __func__);
    #endif

    for (int i = 0; i < nxagentMaxSelections; i++)
    {
      XFixesSelectSelectionInput(nxagentDisplay, serverWindow,
                                 lastSelectionOwner[i].selection,
                                 XFixesSetSelectionOwnerNotifyMask |
                                 XFixesSelectionWindowDestroyNotifyMask |
                                 XFixesSelectionClientCloseNotifyMask);
    }

    nxagentXFixesInfo.Initialized = 1;
  }

  /*
     The first paste from CLIPBOARD did not work directly after
     session start. Removing this code makes it work. It is unsure why
     it was introduced in the first place so it is possible that we
     see other effects by leaving out this code.

     Fixes X2Go bug #952, see https://bugs.x2go.org/952 for details .

  if (nxagentSessionId[0])
  {
    #ifdef TEST
    fprintf(stderr, "%s: setting the ownership of %s to %lx"
                " and registering for PropertyChangeMask events\n", __func__,
                    validateString(XGetAtomName(nxagentDisplay, nxagentAtoms[10])), serverWindow);
    #endif

    XSetSelectionOwner(nxagentDisplay, nxagentAtoms[10], serverWindow, CurrentTime);
    pWin -> eventMask |= PropertyChangeMask;
    nxagentChangeWindowAttributes(pWin, CWEventMask);
  }
  */

  if (nxagentReconnectTrap)
  {
    if (nxagentOption(Clipboard) == ClipboardServer ||
        nxagentOption(Clipboard) == ClipboardBoth)
    {
      for (int i = 0; i < nxagentMaxSelections; i++)
      {
        /*
         * if we have a selection inform the (new) real Xserver and
         * claim the ownership. Note that we report our serverWindow as
         * owner, not the real window!
         */
         if (IS_INTERNAL_OWNER(i) && lastSelectionOwner[i].window)
         {
           XSetSelectionOwner(nxagentDisplay, lastSelectionOwner[i].selection, serverWindow, CurrentTime);
         }
      }
    }
    /* FIXME: Shouldn't we reset lastServer* and lastClient* here? */
  }
  else
  {
    for (int i = 0; i < nxagentMaxSelections; i++)
    {
      clearSelectionOwner(i);
    }

    lastServerRequestor = None;

    setClientSelectionStage(SelectionStageNone);
    lastClientReqTime = GetTimeInMillis();

    clientTARGETS = MakeAtom(szAgentTARGETS, strlen(szAgentTARGETS), True);
    clientTEXT = MakeAtom(szAgentTEXT, strlen(szAgentTEXT), True);
    clientCOMPOUND_TEXT = MakeAtom(szAgentCOMPOUND_TEXT, strlen(szAgentCOMPOUND_TEXT), True);
    clientUTF8_STRING = MakeAtom(szAgentUTF8_STRING, strlen(szAgentUTF8_STRING), True);
    clientTIMESTAMP = MakeAtom(szAgentTIMESTAMP, strlen(szAgentTIMESTAMP), True);
    clientCLIPBOARD = MakeAtom(szAgentCLIPBOARD, strlen(szAgentCLIPBOARD), True);

    clientCutProperty = MakeAtom(szAgentNX_CUT_BUFFER_CLIENT,
                                    strlen(szAgentNX_CUT_BUFFER_CLIENT), True);
    if (clientCutProperty == None)
    {
      #ifdef PANIC
      fprintf(stderr, "%s: PANIC! "
              "Could not create %s atom.\n", __func__, szAgentNX_CUT_BUFFER_CLIENT);
      #endif

      return False;
    }
  }

  agentClipboardInitialized = True;

  #ifdef DEBUG
  fprintf(stderr, "%s: Clipboard initialization completed.\n", __func__);
  #endif

  #ifdef NXAGENT_TIMESTAMP
  {
    fprintf(stderr, "%s: Clipboard init ends at [%lu] ms.\n", __func__,
                GetTimeInMillis() - startTime);
  }
  #endif

  return True;
}
