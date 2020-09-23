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
  ClientPtr  client;           /* internal client */
  Atom       intSelection;     /* internal Atom */
  XlibAtom   remSelection;     /* _external_ Atom */
  Window     window;           /* internal window id */
  WindowPtr  windowPtr;        /* internal window struct */
  Time       lastTimeChanged;  /* internal time */
} SelectionOwner;

/*
 * this contains the last selection owner in nxagent. The
 * lastTimeChanged is always an internal time. If .client is NULL the
 * owner is outside nxagent. .selection will _always_ contain the
 * external atom of the selection
 */

/* FIXME: these should also be stored per selection */
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

typedef struct _lastClient
{
  WindowPtr     windowPtr;
  ClientPtr     clientPtr;
  Window        requestor;
  Atom          property;
  Atom          intSelection;
  Atom          target;
  Time          time;
  Time          reqTime;
  unsigned long propertySize;
  ClientSelectionStage stage;
  int           resource;       /* nxcompext resource where collected proeprty data is stored */
} lastClient;

static lastClient *lastClients;

typedef struct _lastServer {
  Window        requestor;
  XlibAtom      property;
  XlibAtom      target;
  Time          time;
} lastServer;

static lastServer *lastServers;

static XlibAtom serverTARGETS;
static XlibAtom serverTIMESTAMP;
static XlibAtom serverTEXT;
static XlibAtom serverCOMPOUND_TEXT;
static XlibAtom serverUTF8_STRING;
static XlibAtom serverTransFromAgentProperty;
static Atom     clientTARGETS;
static Atom     clientTIMESTAMP;
static Atom     clientTEXT;
static Atom     clientCOMPOUND_TEXT;
static Atom     clientUTF8_STRING;
static Atom     clientCLIPBOARD;

static char     szAgentTARGETS[] = "TARGETS";
static char     szAgentTEXT[] = "TEXT";
static char     szAgentTIMESTAMP[] = "TIMESTAMP";
static char     szAgentCOMPOUND_TEXT[] = "COMPOUND_TEXT";
static char     szAgentUTF8_STRING[] = "UTF8_STRING";
static char     szAgentNX_CUT_BUFFER_CLIENT[] = "NX_CUT_BUFFER_CLIENT";
static char     szAgentCLIPBOARD[] = "CLIPBOARD";

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
#define printClientSelectionStage(_index) do {fprintf(stderr, "%s: Current selection stage for selection [%d] is [%s]\n", __func__, _index, getClientSelectionStageString(lastClients[_index].stage));} while (0)
#else
#define printClientSelectionStage(_index)
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

XFixesAgentInfoRec nxagentXFixesInfo = { -1, -1, -1, False };

extern Display *nxagentDisplay;

static Bool validServerTargets(XlibAtom target);
static void setClientSelectionStage(int stage, int index);
static void endTransfer(Bool success, int index);
#define SELECTION_SUCCESS True
#define SELECTION_FAULT False
static void transferSelectionFromXServer(int resource, int index);
#if 0
static void resetSelectionOwnerOnXServer(void);
#endif
static void initSelectionOwnerData(int index, Atom intSelection, XlibAtom remSelection);
static void clearSelectionOwnerData(int index);
static void storeSelectionOwnerData(int index, Selection *sel);
static Bool matchSelectionOwner(int index, ClientPtr pClient, WindowPtr pWindow);
static void setSelectionOwnerOnXServer(Selection *pSelection);
static int sendEventToClient(ClientPtr client, xEvent *pEvents);
static void sendSelectionNotifyEventToClient(ClientPtr client,
                                             Time time,
                                             Window requestor,
                                             Atom selection,
                                             Atom target,
                                             Atom property);
static Status sendSelectionNotifyEventToXServer(XSelectionEvent *event_to_send);
#ifdef DEBUG
static void printSelectionStat(int sel);
#endif
static void replyRequestSelectionToXServer(XEvent *X, Bool success);

void nxagentPrintClipboardStat(char *);

XlibAtom translateLocalToRemoteSelection(Atom local);
XlibAtom translateLocalToRemoteTarget(Atom local);

#ifdef NXAGENT_TIMESTAMP
extern unsigned long startTime;
#endif

static void printSelectionStat(int index)
{
  SelectionOwner lOwner = lastSelectionOwner[index];
  Selection curSel = CurrentSelections[index];
  char *s = NULL;

  fprintf(stderr, "  owner is inside nxagent?               %s\n", IS_INTERNAL_OWNER(index) ? "yes" : "no");
  fprintf(stderr, "  lastSelectionOwner[].client            %s\n", nxagentClientInfoString(lOwner.client));
  fprintf(stderr, "  lastSelectionOwner[].window            [0x%x]\n", lOwner.window);
  if (lOwner.windowPtr)
    fprintf(stderr, "  lastSelectionOwner[].windowPtr         [%p] ([0x%x]\n", (void *)lOwner.windowPtr, WINDOWID(lOwner.windowPtr));
  else
    fprintf(stderr, "  lastSelectionOwner[].windowPtr         -\n");
  fprintf(stderr, "  lastSelectionOwner[].lastTimeChanged   [%u]\n", lOwner.lastTimeChanged);

  fprintf(stderr, "  lastSelectionOwner[].intSelection      [% 4d][%s] (%s)\n", lOwner.intSelection, validateString(NameForAtom(lOwner.intSelection)), "inside nxagent");
  SAFE_XFree(s); s = XGetAtomName(nxagentDisplay, lOwner.remSelection);
  fprintf(stderr, "  lastSelectionOwner[].remSelection      [% 4ld][%s] (%s)\n", lOwner.remSelection, validateString(s), "remote X server");
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
          CLINDEX(curSel.client));
#endif
  fprintf(stderr, "  CurrentSelections[].window             [0x%x]\n", curSel.window);
  return;
}

static void printLastClientStat(int index)
{
  lastClient lc = lastClients[index];
  if (lc.windowPtr)
    fprintf(stderr, "  lastClients[].windowPtr    (WindowPtr) [%p] ([0x%x])\n", (void *)lc.windowPtr, WINDOWID(lc.windowPtr));
  else
    fprintf(stderr, "  lastClients[].windowPtr    (WindowPtr) -\n");
  fprintf(stderr, "  lastClients[].clientPtr    (ClientPtr) %s\n", nxagentClientInfoString(lc.clientPtr));
  fprintf(stderr, "  lastClients[].requestor       (Window) [0x%x]\n", lc.requestor);
  fprintf(stderr, "  lastClients[].property          (Atom) [% 4d][%s]\n", lc.property, NameForAtom(lc.property));
  fprintf(stderr, "  lastClients[].intSelection      (Atom) [% 4d][%s]\n", lc.intSelection, NameForAtom(lc.intSelection));
  fprintf(stderr, "  lastClients[].target            (Atom) [% 4d][%s]\n", lc.target, NameForAtom(lc.target));
  if (lc.time > 0)
    fprintf(stderr, "  lastClients[].time              (Time) [%u] ([%u]ms ago)\n", lc.time, GetTimeInMillis() - lc.time);
  else
    fprintf(stderr, "  lastClients[].time              (Time) [%u]\n", lc.time);
  if (lc.reqTime > 0)
    fprintf(stderr, "  lastClients[].reqTime           (Time) [%u] ([%u]ms ago)\n", lc.reqTime, GetTimeInMillis() - lc.reqTime);
  else
    fprintf(stderr, "  lastClients[].reqTime           (Time) [%u]\n", lc.reqTime);
  fprintf(stderr, "  lastClients[].propertySize     (ulong) [%lu]\n", lc.propertySize);
  fprintf(stderr, "  lastClients[].stage   (ClientSelStage) [%d][%s]\n", lc.stage, getClientSelectionStageString(lc.stage));
  fprintf(stderr, "  lastClients[].resource           (int) [%d]\n", lc.resource);
}

static void printLastServerStat(int index)
{
  lastServer ls = lastServers[index];
  char *s = NULL;

  fprintf(stderr, "  lastServer[].requestor        (Window) [0x%x]\n", ls.requestor);
  SAFE_XFree(s); s = XGetAtomName(nxagentDisplay, ls.property);
  fprintf(stderr, "  lastServer[].property           (Atom) [% 4ld][%s]\n", ls.property, validateString(s));
  SAFE_XFree(s); s = XGetAtomName(nxagentDisplay, ls.target);
  fprintf(stderr, "  lastServer[].target             (Atom) [% 4ld][%s]\n", ls.target, validateString(s));
  fprintf(stderr, "  lastServer[].time               (Time) [%u]\n", ls.time);
  SAFE_XFree(s);
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

  fprintf(stderr, "PRIMARY\n");
  printSelectionStat(nxagentPrimarySelection);
  printLastClientStat(nxagentPrimarySelection);
  printLastServerStat(nxagentPrimarySelection);
  fprintf(stderr, "CLIPBOARD\n");
  printSelectionStat(nxagentClipboardSelection);
  printLastClientStat(nxagentClipboardSelection);
  printLastServerStat(nxagentClipboardSelection);

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
static void resetClientSelectionStage(int index)
{
  #ifdef DEBUG
  fprintf(stderr, "%s: Resetting selection stage for [%d]\n", __func__, index);
  #endif

  lastClients[index].stage = SelectionStageNone;
  lastClients[index].windowPtr = NULL;
  lastClients[index].clientPtr = NULL;
  lastClients[index].requestor = 0;
  lastClients[index].property = 0;
  /* NX_PRIMARY or clipboard atom */
  /* FIXME: we should only set the once on init and then never touch it again */
  lastClients[index].intSelection = lastSelectionOwner[index].intSelection;
  lastClients[index].target = 0;
  lastClients[index].time = 0;
  lastClients[index].reqTime = 0;
  lastClients[index].propertySize = 0;
  lastClients[index].resource = -1;
}

static void setClientSelectionStage(int stage, int index)
{
  if (lastClients[index].stage == stage)
  {
    #ifdef DEBUG
    fprintf(stderr, "%s: lastClient [%d] selection stage already set to [%s] - doing nothing\n", __func__,
                index, getClientSelectionStageString(lastClients[index].stage));
    #endif
    return;
  }

  #ifdef DEBUG
  fprintf(stderr, "%s: Changing selection stage for [%d] from [%s] to [%s]\n", __func__, index,
              getClientSelectionStageString(lastClients[index].stage), getClientSelectionStageString(stage));
  #endif

  if (stage == SelectionStageNone)
  {
    resetClientSelectionStage(index);
  }
  else
  {
    lastClients[index].stage = stage;
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
static Status sendSelectionNotifyEventToXServer(XSelectionEvent *event_to_send)
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

static void initSelectionOwnerData(int index, Atom intSelection, XlibAtom remSelection)
{
  lastSelectionOwner[index].intSelection = intSelection;
  lastSelectionOwner[index].remSelection = remSelection;
  lastSelectionOwner[index].client = NullClient;
  lastSelectionOwner[index].window = screenInfo.screens[0]->root->drawable.id;
  lastSelectionOwner[index].windowPtr = NULL;
  lastSelectionOwner[index].lastTimeChanged = GetTimeInMillis();
}

/* there's no owner on nxagent side anymore */
static void clearSelectionOwnerData(int index)
{
  lastSelectionOwner[index].client = NULL;
  lastSelectionOwner[index].window = None;
  lastSelectionOwner[index].windowPtr = NULL;
  lastSelectionOwner[index].lastTimeChanged = GetTimeInMillis();
}

static void storeSelectionOwnerData(int index, Selection *sel)
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

  for (int index = 0; index < nxagentMaxSelections; index++)
  {
    if (matchSelectionOwner(index, pClient, pWindow))
    {
      #ifdef TEST
      fprintf(stderr, "%s: Resetting state [%d] with client [%p] window [%p].\n", __func__,
                  index, (void *) pClient, (void *) pWindow);
      #endif

      clearSelectionOwnerData(index);

      setClientSelectionStage(SelectionStageNone, index);

      lastServers[index].requestor = None;
    }

    if (pWindow && pWindow == lastClients[index].windowPtr)
    {
      setClientSelectionStage(SelectionStageNone, index);
    }
  }
}

/*
 * Find the index of the lastSelectionOwner with the selection
 * sel. sel is an atom on the real X server.
 */
int nxagentFindLastSelectionOwnerIndex(XlibAtom sel)
{
  int index = 0;
  while (index < nxagentMaxSelections &&
            lastSelectionOwner[index].remSelection != sel)
  {
    index++;
  }
  return index;
}

/*
 * Find the index of CurrentSelection with the selection
 * sel. sel is an internal atom.
 */
int nxagentFindCurrentSelectionIndex(Atom sel)
{
  int index = 0;
  while (index < NumCurrentSelections &&
            CurrentSelections[index].selection != sel)
  {
    index++;
  }
  return index;
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

  int index = nxagentFindLastSelectionOwnerIndex(X->xselectionclear.selection);
  if (index < nxagentMaxSelections)
  {
    if (IS_INTERNAL_OWNER(index))
    {
      /* send a SelectionClear event to (our) previous owner */
      xEvent x = {0};
      x.u.u.type = SelectionClear;
      x.u.selectionClear.time = GetTimeInMillis();
      x.u.selectionClear.window = lastSelectionOwner[index].window;
      x.u.selectionClear.atom = CurrentSelections[index].selection;

      sendEventToClient(lastSelectionOwner[index].client, &x);
    }

    /*
     * set the root window with the NullClient as selection owner. Our
     * clients asking for the owner via XGetSelectionOwner() will get
     * these for an answer
     */
    CurrentSelections[index].window = screenInfo.screens[0]->root->drawable.id;
    CurrentSelections[index].client = NullClient;

    clearSelectionOwnerData(index);

    setClientSelectionStage(SelectionStageNone, index);
  }
}

/*
 * Send a SelectionNotify event as reply to the RequestSelection
 * event X. If success is True take the property from the event, else
 * take None (which reports "failed/denied" to the requestor).
 */
static void replyRequestSelectionToXServer(XEvent *X, Bool success)
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

  sendSelectionNotifyEventToXServer(&eventSelection);
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
              "target [%ld][%s] requestor [display[%s]/0x%lx] destination [%ld][%s]\n",
              __func__,
              X->xselectionrequest.selection, validateString(strSelection),
              X->xselectionrequest.target,    validateString(strTarget),
              DisplayString(nxagentDisplay), X->xselectionrequest.requestor,
              X->xselectionrequest.property,  validateString(strProperty));

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

  /* the selection in this request is none we own. */
  int index = nxagentFindLastSelectionOwnerIndex(X->xselectionrequest.selection);
  if (index == nxagentMaxSelections)
  {
    #ifdef DEBUG
    fprintf(stderr, "%s: not owning selection [%ld] - denying request.\n", __func__, X->xselectionrequest.selection);
    #endif

    replyRequestSelectionToXServer(X, False);
    return;
  }

  #ifdef DEBUG
  fprintf(stderr, "%s: lastServers[%d].requestor [0x%x].\n", __func__, index, lastServers[index].requestor);
  #endif

  /* lastServers[index].requestor in non-NULL (= we are currently in the transfer phase) */
  if (lastServers[index].requestor != None)
  {
    #ifdef DEBUG
    fprintf(stderr, "%s: denying additional request during transfer phase.\n", __func__);
    #endif

    replyRequestSelectionToXServer(X, False);
    return;
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
      fprintf(stderr, "%s: Sending %d available targets:\n", __func__, numTargets);
      for (int i = 0; i < numTargets; i++)
      {
        char *s = XGetAtomName(nxagentDisplay, targets[i]);
        fprintf(stderr, "%s: %ld %s\n", __func__, targets[i], s);
        SAFE_XFree(s);
      }
      fprintf(stderr, "\n");
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

      replyRequestSelectionToXServer(X, True);
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

      XChangeProperty(nxagentDisplay,
                      X->xselectionrequest.requestor,
                      X->xselectionrequest.property,
                      XA_INTEGER,
                      32,
                      PropModeReplace,
                      (unsigned char *) &lastSelectionOwner[index].lastTimeChanged,
                      1);
      replyRequestSelectionToXServer(X, True);
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
      replyRequestSelectionToXServer(X, False);
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

#if 0
  if (lastClients[index].windowPtr != NULL && IS_INTERNAL_OWNER(index))
  {
    /*
     * Request the real X server to transfer the selection content
     * to the NX_CUT_BUFFER_SERVER property of the serverWindow.
     * We reach here as follows:
     * - mark someting in the nx session
     *   -> nxagent claims ownership of PRIMARY on real X server
     * - at the same time paste _clipboard_ to the client (now) owning primary
     *   ->vcxsrv will ask for primary contents to store them to Windows clipboard
     * - vcxsrv request is for _primary_ and takes this path as the _clipboard_ transfer
     * has set lastClients[index].windowPtr
     */
    XConvertSelection(nxagentDisplay, CurrentSelections[index].selection,
                          X->xselectionrequest.target, serverTransToAgentProperty,
                              serverWindow, lastClients[index].time);

    #ifdef DEBUG
    char *strTarget = XGetAtomName(nxagentDisplay, X->xselectionrequest.target);
    char *strSelection = XGetAtomName(nxagentDisplay, CurrentSelections[index].selection);
    char *strProperty = XGetAtomName(nxagentDisplay, serverTransToAgentProperty);
    fprintf(stderr, "%s: Sent XConvertSelection: selection [%d][%s] target [%ld][%s] property [%ld][%s] window [0x%x] time [%u] .\n", __func__,
                CurrentSelections[index].selection, strSelection,
                    X->xselectionrequest.target, strTarget,
                        serverTransToAgentProperty, strProperty,
                            serverWindow, lastClients[index].time);
    SAFE_XFree(strTarget);
    SAFE_XFree(strSelection);
    SAFE_XFree(strProperty);
    #endif
  }
  else
#endif
  {
    /*
     * if one of our clients owns the selection we ask it to copy
     * the selection to the clientCutProperty on nxagent's root
     * window in the first step. We then later push that property's
     * content to the real X server.
     */
    if (IS_INTERNAL_OWNER(index) &&
            (nxagentOption(Clipboard) == ClipboardServer ||
                 nxagentOption(Clipboard) == ClipboardBoth))
    {
      /*
       * store who on the real X server requested the data and how
       * and where it wants to have it
       */
      lastServers[index].property = X->xselectionrequest.property;
      lastServers[index].requestor = X->xselectionrequest.requestor;
      lastServers[index].target = X->xselectionrequest.target;
      lastServers[index].time = X->xselectionrequest.time;

      /* by dimbor */
      if (lastServers[index].target != XA_STRING)
          lastServers[index].target = serverUTF8_STRING;

      /* prepare the request (like XConvertSelection, but internally) */
      xEvent x = {0};
      x.u.u.type = SelectionRequest;
      x.u.selectionRequest.time = GetTimeInMillis();
      x.u.selectionRequest.owner = lastSelectionOwner[index].window;
      x.u.selectionRequest.selection = CurrentSelections[index].selection;
      x.u.selectionRequest.property = clientCutProperty;
      x.u.selectionRequest.requestor = screenInfo.screens[0]->root->drawable.id; /* Fictitious window.*/

      /*
       * Don't send the same window, some programs are clever and
       * verify cut and paste operations inside the same window and
       * don't Notify at all.
       *
       * x.u.selectionRequest.requestor = lastSelectionOwner[index].window;
       */

      /* by dimbor (idea from zahvatov) */
      if (X->xselectionrequest.target != XA_STRING)
        x.u.selectionRequest.target = clientUTF8_STRING;
      else
        x.u.selectionRequest.target = XA_STRING;

      x.u.selectionRequest.target = nxagentRemoteToLocalAtom(X->xselectionrequest.target);
      sendEventToClient(lastSelectionOwner[index].client, &x);

      #ifdef DEBUG
      fprintf(stderr, "%s: sent SelectionRequest event to client %s property [%d][%s] " \
              "target [%d][%s] requestor [0x%x] selection [%d][%s].\n", __func__,
              nxagentClientInfoString(lastSelectionOwner[index].client),
              x.u.selectionRequest.property, NameForAtom(x.u.selectionRequest.property),
              x.u.selectionRequest.target, NameForAtom(x.u.selectionRequest.target),
              x.u.selectionRequest.requestor,
              x.u.selectionRequest.selection, NameForAtom(x.u.selectionRequest.selection));
      #endif
    }
    else
    {
      /* deny the request */
      replyRequestSelectionToXServer(X, False);
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
static void endTransfer(Bool success, int index)
{
  if (lastClients[index].clientPtr == NULL)
  {
    #ifdef DEBUG
    fprintf(stderr, "%s: lastClients[%d].clientPtr is NULL - doing nothing.\n", __func__, index);
    #endif
  }
  else
  {
    #ifdef DEBUG
    if (success == SELECTION_SUCCESS)
      fprintf(stderr, "%s: sending notification to client %s, property [%d][%s]\n", __func__,
                  nxagentClientInfoString(lastClients[index].clientPtr), lastClients[index].property, NameForAtom(lastClients[index].property));
    else
      fprintf(stderr, "%s: sending negative notification to client %s\n", __func__,
                  nxagentClientInfoString(lastClients[index].clientPtr));
    #endif

    sendSelectionNotifyEventToClient(lastClients[index].clientPtr,
                                     lastClients[index].time,
                                     lastClients[index].requestor,
                                     lastClients[index].intSelection,
                                     lastClients[index].target,
                                     success == SELECTION_SUCCESS ? lastClients[index].property : None);
  }

  /*
   * Enable further requests from clients.
   */
  setClientSelectionStage(SelectionStageNone, index);
}

static void transferSelectionFromXServer(int resource, int index)
{
  #ifdef DEBUG
  fprintf(stderr, "%s: resource [%d] lastClients[%d].clientPtr->index [%d].\n", __func__,
              resource, index, lastClients[index].clientPtr -> index);
  #endif
  /* FIXME: can we use this instead of lastClients[index].resource? */
  if (lastClients[index].clientPtr -> index != resource)
  {
    #ifdef DEBUG
    fprintf (stderr, "%s: WARNING! Inconsistent resource [%d] with current client %s.\n", __func__,
                 resource, nxagentClientInfoString(lastClients[index].clientPtr));
    #endif

    endTransfer(SELECTION_FAULT, index);

    return;
  }

  switch (lastClients[index].stage)
  {
    case SelectionStageQuerySize:
    {
      int result;

      printClientSelectionStage(index);

      /*
       * Don't get data yet, just get size. We skip this stage in
       * current implementation and go straight to the data.
       */

      /* get next free resource index */
      int free_resource = NXGetCollectPropertyResource(nxagentDisplay);

      lastClients[index].resource = free_resource;

      if (free_resource == -1)
      {
        #ifdef WARNING
        fprintf(stderr, "%s: WARNING! Asynchronous GetProperty queue full.\n", __func__);
        #endif

        result = -1;
      }
      else
      {
        /* collect property and store with index "resource"  */
        result = NXCollectProperty(nxagentDisplay,
                                   free_resource,
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
                     nxagentClientInfoString(lastClients[index].clientPtr));
        #endif

        endTransfer(SELECTION_FAULT, index);

        return;
      }

      setClientSelectionStage(SelectionStageWaitSize, index);

      NXFlushDisplay(nxagentDisplay, NXFlushLink);

      break;
    }
    case SelectionStageQueryData:
    {
      int result;

      printClientSelectionStage(index);

      /*
       * Request the selection data now.
       */

      #ifdef DEBUG
      fprintf(stderr, "%s: Getting property content from remote server.\n", __func__);
      #endif

      /* get next free resouce index */
      resource = NXGetCollectPropertyResource(nxagentDisplay);

      lastClients[index].resource = resource;

      if (resource == -1)
      {
        #ifdef WARNING
        fprintf(stderr, "%s: WARNING! Asynchronous GetProperty queue full.\n", __func__);
        #endif

        result = -1;
      }
      else
      {
        result = NXCollectProperty(nxagentDisplay,
                                   resource,
                                   serverWindow,
                                   serverTransToAgentProperty,
                                   0,
                                   lastClients[index].propertySize,
                                   False,
                                   AnyPropertyType);
      }

      if (result == -1)
      {
        #ifdef DEBUG
        fprintf (stderr, "%s: Aborting selection notify procedure for client %s.\n", __func__,
                     nxagentClientInfoString(lastClients[index].clientPtr));
        #endif

        endTransfer(SELECTION_FAULT, index);

        return;
      }

      setClientSelectionStage(SelectionStageWaitData, index);

      /* we've seen situations where you had to move the mouse or press a
         key to let the transfer complete. Flushing here fixed it */
      NXFlushDisplay(nxagentDisplay, NXFlushLink);

      break;
    }
    default:
    {
      #ifdef DEBUG
      fprintf (stderr, "%s: WARNING! Inconsistent state [%s] for selection [%d] for client %s.\n", __func__,
                   getClientSelectionStageString(lastClients[index].stage), index,
                       nxagentClientInfoString(lastClients[index].clientPtr));
      #endif

      break;
    }
  }
}

/*
   Called from Events.c/nxagentHandleCollectPropertyEvent
   This event is generated after XChangeProperty(), XDeleteProperty() or
   XGetWindowProperty(delete=True)

   Returncode:
   True: processed
   False: not processed, resource is not ours
*/

Bool nxagentCollectPropertyEventFromXServer(int resource)
{
  XlibAtom              atomReturnType;
  int                   resultFormat;
  unsigned long         ulReturnItems;
  unsigned long         ulReturnBytesLeft;
  unsigned char         *pszReturnData = NULL;

  int index = 0;

  /* determine the selection we are talking about here */
  for (index = 0; index < nxagentMaxSelections; index++)
  {
    #ifdef DEBUG
    fprintf(stderr, "%s: lastClients[%d].resource [%d] resource [%d]\n", __func__, index, lastClients[index].resource, resource);
    #endif
    if (lastClients[index].resource == resource)
    {
      break;
    }
  }

  if (index == nxagentMaxSelections)
  {
    #ifdef DEBUG
    fprintf (stderr, "%s: resource does not belong to any selection we handle.\n", __func__);
    #endif
    return False;
  }

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

  lastClients[index].resource = -1;

  if (result == 0)
  {
    #ifdef DEBUG
    fprintf (stderr, "%s: Failed to get reply data.\n", __func__);
    #endif

    endTransfer(SELECTION_FAULT, index);
  }
  else if (resultFormat != 8 && resultFormat != 16 && resultFormat != 32)
  {
    #ifdef DEBUG
    fprintf(stderr, "%s: WARNING! Invalid property format.\n", __func__);
    #endif

    endTransfer(SELECTION_FAULT, index);
  }
  else
  {
    switch (lastClients[index].stage)
    {
      case SelectionStageWaitSize:
      {
        printClientSelectionStage(index);
        #ifdef DEBUG
        fprintf (stderr, "%s: Got size notify event for client %s.\n", __func__,
                     nxagentClientInfoString(lastClients[index].clientPtr));
        #endif

        if (ulReturnBytesLeft == 0)
        {
          #ifdef DEBUG
          fprintf (stderr, "%s: data size is [0] - aborting selection notify procedure.\n", __func__);
          #endif

          endTransfer(SELECTION_FAULT, index);
        }
        else
        {
          #ifdef DEBUG
          fprintf(stderr, "%s: Got property size [%lu] from remote server.\n", __func__, ulReturnBytesLeft);
          #endif

          /*
           * Request the selection data now.
           */
          lastClients[index].propertySize = ulReturnBytesLeft;
          setClientSelectionStage(SelectionStageQueryData, index);

          transferSelectionFromXServer(resource, index);
        }
        break;
      }
      case SelectionStageWaitData:
      {
        printClientSelectionStage(index);
        #ifdef DEBUG
        fprintf (stderr, "%s: Got data notify event for waiting client %s.\n", __func__,
                     nxagentClientInfoString(lastClients[index].clientPtr));
        #endif

        if (ulReturnBytesLeft != 0)
        {
          #ifdef DEBUG
          fprintf (stderr, "%s: not all content could be retrieved - [%lu] bytes left - aborting selection notify procedure.\n", __func__, ulReturnBytesLeft);
          #endif

          endTransfer(SELECTION_FAULT, index);
        }
        else
        {
          #ifdef DEBUG
          fprintf(stderr, "%s: Got property content from remote server. size [%lu] bytes.\n", __func__, (ulReturnItems * resultFormat / 8));
          #endif

          ChangeWindowProperty(lastClients[index].windowPtr,
                               lastClients[index].property,
                               lastClients[index].target,
                               resultFormat, PropModeReplace,
                               ulReturnItems, pszReturnData, 1);

          #ifdef DEBUG
          fprintf(stderr, "%s: Selection property [%d][%s] changed to [\"%*.*s\"...]\n", __func__,
                      lastClients[index].property,
                          validateString(NameForAtom(lastClients[index].property)),
                              (int)(min(20, ulReturnItems * resultFormat / 8)),
                                  (int)(min(20, ulReturnItems * resultFormat / 8)),
                                      pszReturnData);
          #endif

          endTransfer(SELECTION_SUCCESS, index);
        }
        break;
      }
      default:
      {
        #ifdef DEBUG
        fprintf(stderr, "%s: WARNING! Inconsistent state [%s] for client %s.\n", __func__,
                    getClientSelectionStageString(lastClients[index].stage),
                        nxagentClientInfoString(lastClients[index].clientPtr));
        #endif
        break;
      }
    }
  }
  SAFE_XFree(pszReturnData);

  return True;
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
    char * p = XGetAtomName(nxagentDisplay, e->property);
    char * t = XGetAtomName(nxagentDisplay, e->target);
    char * s = XGetAtomName(nxagentDisplay, e->selection);
    if (e->requestor == serverWindow)
    {
      fprintf(stderr, "%s: this event has been sent by nxagent!\n", __func__);;
    }

    fprintf(stderr, "%s: SelectionNotify event from real X server, property " \
                "[%ld][%s] requestor [0x%lx] selection [%s] target [%ld][%s] time [%lu] send_event [%d].\n",
                    __func__, e->property, validateString(p), e->requestor,
                        validateString(s), e->target,
                            validateString(t), e->time, e->send_event);
    SAFE_XFree(p);
    SAFE_XFree(t);
    SAFE_XFree(s);
  }
  #endif

  /* determine the selection we are talking about here */
  int index = 0;
  for (index = 0; index < nxagentMaxSelections; index++)
  {
    #ifdef DEBUG
    fprintf (stderr, "%s: lastClients[%d].intSelection [%d] selection [%d] .\n",
                 __func__, index, lastClients[index].intSelection,
	             nxagentRemoteToLocalAtom(e->selection));
    #endif
    if (lastClients[index].intSelection == nxagentRemoteToLocalAtom(e->selection))
    {
      break;
    }
  }

  if (index == nxagentMaxSelections)
  {
    #ifdef DEBUG
    fprintf (stderr, "%s: unknown selection [%ld] .\n", __func__, e->selection);
    #endif
    return;
  }

  printClientSelectionStage(index);

  if (lastClients[index].windowPtr != NULL)
  {
    /*
     * We reach here after a paste inside the nxagent, triggered by
     * the XConvertSelection call in nxagentConvertSelection(). This
     * means that data we need has been transferred to the
     * serverTransToAgentProperty of the serverWindow (our window on
     * the real X server). We now need to transfer it to the original
     * requestor, which is stored in the lastClients[index].* variables.
     */

    #ifdef DEBUG
    nxagentDumpClipboardStat();
    #endif
    if (lastClients[index].stage == SelectionStageNone)
    {
      if (X->xselection.property == serverTransToAgentProperty)
      {
        #ifdef DEBUG
        fprintf(stderr, "%s: Starting selection transferral for client %s.\n", __func__,
                nxagentClientInfoString(lastClients[index].clientPtr));
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

        setClientSelectionStage(SelectionStageQueryData, index);
        lastClients[index].propertySize = 262144;

        transferSelectionFromXServer(lastClients[index].clientPtr -> index, index);
      }
      else if (X->xselection.property == 0)
      {
        #ifdef DEBUG
        fprintf(stderr, "%s: WARNING! Resetting selection transferral for client [%d] because of failure notification from real X server.\n", __func__,
                    CLINDEX(lastClients[index].clientPtr));
        #endif
        endTransfer(SELECTION_FAULT, index);
      }
      else
      {
        #ifdef DEBUG
        char *s = XGetAtomName(nxagentDisplay, X->xselection.property);
        fprintf(stderr, "%s: Unexpected property [%ld][%s] - reporting conversion failure.\n",
                    __func__, X->xselection.property, s);
        SAFE_XFree(s);
        #endif
        endTransfer(SELECTION_FAULT, index);
      }
    }
    else
    {
      #ifdef DEBUG
      fprintf(stderr, "%s: WARNING! Resetting selection transferral for client [%d] because of unexpected stage.\n", __func__,
                  CLINDEX(lastClients[index].clientPtr));
      #endif

      endTransfer(SELECTION_FAULT, index);
    }
  }
  else
  {
    int index = nxagentFindLastSelectionOwnerIndex(X->xselection.selection);
    if (index < nxagentMaxSelections)
    {
      /* if the last owner was an internal one, read the
       * clientCutProperty and push the contents to the
       * lastServers[index].requestor on the real X server.
       */
      if (IS_INTERNAL_OWNER(index) &&
             lastSelectionOwner[index].windowPtr != NULL &&
                 X->xselection.property == serverTransFromAgentProperty)
      {
        Atom            atomReturnType;
        int             resultFormat;
        unsigned long   ulReturnItems;
        unsigned long   ulReturnBytesLeft;
        unsigned char   *pszReturnData = NULL;

        /* first get size values ... */
        int result = GetWindowProperty(lastSelectionOwner[index].windowPtr, clientCutProperty, 0, 0, False,
                                           AnyPropertyType, &atomReturnType, &resultFormat,
                                               &ulReturnItems, &ulReturnBytesLeft, &pszReturnData);

        #ifdef DEBUG
        fprintf(stderr, "%s: GetWindowProperty() window [0x%x] property [%d] returned [%s]\n", __func__,
                    lastSelectionOwner[index].window, clientCutProperty, getXErrorString(result));
        #endif
        if (result == BadAlloc || result == BadAtom ||
                result == BadWindow || result == BadValue)
        {
          lastServers[index].property = None;
        }
        else
        {
          /* ... then use the size values for the actual request */
          result = GetWindowProperty(lastSelectionOwner[index].windowPtr, clientCutProperty, 0,
                                         ulReturnBytesLeft, False, AnyPropertyType, &atomReturnType,
                                             &resultFormat, &ulReturnItems, &ulReturnBytesLeft,
                                                 &pszReturnData);
          #ifdef DEBUG
          fprintf(stderr, "%s: GetWindowProperty() window [0x%x] property [%d] returned [%s]\n", __func__,
                      lastSelectionOwner[index].window, clientCutProperty, getXErrorString(result));
          #endif

          if (result == BadAlloc || result == BadAtom ||
                  result == BadWindow || result == BadValue)
          {
            lastServers[index].property = None;
          }
          else
          {
            /* Fill the property on the initial requestor with the requested data */
            /* The XChangeProperty source code reveals it will always
               return 1, no matter what, so no need to check the result */
            /* FIXME: better use the format returned by above request */
            XChangeProperty(nxagentDisplay,
                            lastServers[index].requestor,
                            lastServers[index].property,
                            lastServers[index].target,
                            8,
                            PropModeReplace,
                            pszReturnData,
                            ulReturnItems);

            #ifdef DEBUG
            {
              char *s = XGetAtomName(nxagentDisplay, lastServers[index].property);
              fprintf(stderr, "%s: XChangeProperty sent to window [0x%x] for property [%ld][%s] value [\"%*.*s\"...]\n",
                      __func__,
                      lastServers[index].requestor,
                      lastServers[index].property,
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
         * been called and lastServers[index].property will be None which
         * effectively will send a "Request denied" to the initial
         * requestor.
         */
        XSelectionEvent eventSelection = {
          .requestor = lastServers[index].requestor,
          .selection = X->xselection.selection,
          /* .target = X->xselection.target, */
          .target    = lastServers[index].target,
          .property  = lastServers[index].property,
          .time      = lastServers[index].time,
          /* .time   = CurrentTime */
        };
        #ifdef DEBUG
        fprintf(stderr, "%s: Sending SelectionNotify event to requestor [%p].\n", __func__,
                (void *)eventSelection.requestor);
        #endif

        sendSelectionNotifyEventToXServer(&eventSelection);

        lastServers[index].requestor = None; /* allow further request */
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
static void resetSelectionOwnerOnXServer(void)
{
  if (lastServers[index].requestor != None)
  {
    /*
     * we are in the process of communicating back and forth between
     * real X server and nxagent's clients - let's not disturb.
     */
    #if defined(TEST) || defined(DEBUG)
    fprintf(stderr, "%s: WARNING! Requestor window [0x%x] already set.\n", __func__,
                lastServers[index].requestor);
    #endif

    /* FIXME: maybe we should put back the event that lead us here. */
    return;
  }

  /*
   * Only for PRIMARY and CLIPBOARD selections.
   */

  for (int index = 0; index < nxagentMaxSelections; index++)
  {
    XSetSelectionOwner(nxagentDisplay, lastSelectionOwner[index].remSelection, serverWindow, CurrentTime);

    #ifdef DEBUG
    fprintf(stderr, "%s: Reset selection state for selection [%d].\n", __func__, index);
    #endif

    clearSelectionOwnerData(index);

    setClientSelectionStage(SelectionStageNone, index);

    /* Hmm, this is already None when reaching this */
    lastServers[index].requestor = None;
  }
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
  if (nxagentExternalClipboardEventTrap)
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
      fprintf(stderr, "%s: calling setSelectionOwnerOnXServer\n", __func__);
      #endif
      setSelectionOwnerOnXServer(pCurSel);
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
static void setSelectionOwnerOnXServer(Selection *pSelection)
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

  int index = nxagentFindCurrentSelectionIndex(pSelection->selection);
  if (index < NumCurrentSelections)
  {
    #ifdef DEBUG
    fprintf(stderr, "%s: lastSelectionOwner.client %s -> %s\n", __func__,
                nxagentClientInfoString(lastSelectionOwner[index].client),
                    nxagentClientInfoString(pSelection->client));
    fprintf(stderr, "%s: lastSelectionOwner.window [0x%x] -> [0x%x]\n", __func__,
            lastSelectionOwner[index].window, pSelection->window);
    fprintf(stderr, "%s: lastSelectionOwner.windowPtr [%p] -> [%p] [0x%x] (serverWindow: [0x%x])\n", __func__,
            (void *)lastSelectionOwner[index].windowPtr, (void *)pSelection->pWin,
            nxagentWindow(pSelection->pWin), serverWindow);
    fprintf(stderr, "%s: lastSelectionOwner.lastTimeChanged [%u]\n", __func__,
            lastSelectionOwner[index].lastTimeChanged);
    #endif

    #if defined(TEST) || defined(DEBUG)
    if (lastServers[index].requestor != None)
    {
      /*
       * we are in the process of communicating back and forth between
       * real X server and nxagent's clients - let's not disturb
       * FIXME: by continuing after the warning were ARE disturbing!
       */
      fprintf (stderr, "%s: WARNING! lastServers[%d].requestor window [0x%x] already set.\n",
                   __func__, index, lastServers[index].requestor);
    }
    #endif

    /*
     * inform the real X server that our serverWindow is the
     * clipboard owner.
     */
    XSetSelectionOwner(nxagentDisplay, lastSelectionOwner[index].remSelection, serverWindow, CurrentTime);

    /*
     * The real owner window (inside nxagent) is stored in
     * lastSelectionOwner.window.  lastSelectionOwner.windowPtr
     * points to the struct that contains all information about the
     * owner window.
     */
    storeSelectionOwnerData(index, pSelection);

    setClientSelectionStage(SelectionStageNone, index);
  }

  /* FIXME: commented because index is invalid here! */
  /* lastServers[index].requestor = None; */

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

      lastServers[index].requestor = None;
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

  int index = nxagentFindCurrentSelectionIndex(selection);
  if (index == NumCurrentSelections)
  {
    #ifdef DEBUG
    fprintf(stderr, "%s: cannot find index for selection [%u]\n", __func__, selection);
    #endif
    return 0;
  }

  if (IS_INTERNAL_OWNER(index))
  {
    /*
     * There is a client owner on the agent side, let normal dix stuff happen.
     */
    return 0;
  }

  /*
   * if lastClients[index].windowPtr is set we are waiting for an answer from
   * the real X server. If that answer takes more than 5 seconds we
   * consider the conversion failed and tell our client about that.
   * The new request that lead us here is then processed.
   */
  if (lastClients[index].windowPtr != NULL)
  {
    #ifdef TEST
    fprintf(stderr, "%s: lastClients[%d].windowPtr != NULL.\n", __func__, index);
    #endif

    #ifdef DEBUG
    fprintf(stderr, "%s: lastClients[%d].intSelection [%d] - selection [%d]\n", __func__, index, lastClients[index].intSelection, selection);
    #endif

    if ((GetTimeInMillis() - lastClients[index].reqTime) >= CONVERSION_TIMEOUT)
    {
      #ifdef DEBUG
      fprintf(stderr, "%s: timeout expired on previous request, "
                  "notifying failure to client %s\n", __func__, nxagentClientInfoString(client));
      #endif

      /* notify the waiting client of failure */
      endTransfer(SELECTION_FAULT, index);

      /* do NOT return here but process the new request instead! */
    }
    else
    {
      /*
       * we got another convert request while already waiting for an
       * answer from the real X server to a previous convert request
       * for this selection, which we cannot handle (yet). So return
       * an error for the new request.
       */
      #ifdef DEBUG
      fprintf(stderr, "%s: got new request "
                  "before timeout expired on previous request, notifying failure to client %s\n",
                      __func__, nxagentClientInfoString(client));
      #endif

      /* notify the sender of the new request of failure */
      sendSelectionNotifyEventToClient(client, time, requestor, selection, target, None);

      return 1;
    }
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
    for (int j = 0; j < numTargets; j++)
        fprintf(stderr, "%s:  %s\n", __func__, NameForAtom(targets[j]));
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
                         (unsigned char *) &lastSelectionOwner[index].lastTimeChanged,
                         1);
    sendSelectionNotifyEventToClient(client, time, requestor, selection, target, property);
    return 1;
  }

  if (lastClients[index].clientPtr == client && (GetTimeInMillis() - lastClients[index].reqTime < ACCUM_TIME))
  {
    /*
     * The same client made consecutive requests of clipboard content
     * with less than 5 seconds time interval between them.
     */
     #ifdef DEBUG
     fprintf(stderr, "%s: Consecutives request from client %s selection [%u] "
                "elapsed time [%u] clientAccum [%d]\n", __func__, nxagentClientInfoString(client),
                    selection, GetTimeInMillis() - lastClients[index].reqTime, clientAccum);
     #endif

     clientAccum++;
  }
  else
  {
    /* reset clientAccum as now another client requested the clipboard content */
    if (lastClients[index].clientPtr != client)
    {
      clientAccum = 0;
    }
  }

  if (target == clientTEXT ||
          target == XA_STRING ||
              target == clientCOMPOUND_TEXT ||
                  target == clientUTF8_STRING)
  {
    setClientSelectionStage(SelectionStageNone, index);

    /*
     * store the original requestor, we need that later after
     * serverTransToAgentProperty contains the desired selection content
     */
    lastClients[index].requestor = requestor;
    lastClients[index].windowPtr = pWin;
    lastClients[index].clientPtr = client;
    lastClients[index].time = time;
    lastClients[index].property = property;
    lastClients[index].intSelection = selection;
    lastClients[index].target = target;
    /* if the last client request time is more than 5s ago update it. Why? */
    if ((GetTimeInMillis() - lastClients[index].reqTime) >= CONVERSION_TIMEOUT)
      lastClients[index].reqTime = GetTimeInMillis();

    XlibAtom remSelection = translateLocalToRemoteSelection(selection);
    XlibAtom remTarget = translateLocalToRemoteTarget(target);
    XlibAtom remProperty = serverTransToAgentProperty;

    #ifdef DEBUG
    fprintf(stderr, "%s: replacing local by remote property: [%d][%s] -> [%ld][%s]\n",
                __func__, property, NameForAtom(property),
                    remProperty, "NX_CUT_BUFFER_SERVER");
    #endif

    /* FIXME: check why using CurrentTime will not replace the value
     * by a real time. The reply also contains time "0" which is
     * unexpected (for me) */
    #ifdef DEBUG
    fprintf(stderr, "%s: Sending XConvertSelection to real X server: requestor [0x%x] target [%ld][%s] property [%ld][%s] selection [%ld][%s] time [0][CurrentTime]\n", __func__,
            serverWindow, remTarget, XGetAtomName(nxagentDisplay, remTarget),
                remProperty, XGetAtomName(nxagentDisplay, remProperty),
                    remSelection, XGetAtomName(nxagentDisplay, remSelection));
    #endif

    XConvertSelection(nxagentDisplay, remSelection, remTarget, remProperty, serverWindow, CurrentTime);

    /* XConvertSelection will always return (check the source!), so no need to check */

    #ifdef DEBUG
    fprintf(stderr, "%s: Sent XConvertSelection\n", __func__);
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

XlibAtom translateLocalToRemoteSelection(Atom local)
{
  /*
   * On the real server, the right CLIPBOARD atom is
   * XInternAtom(nxagentDisplay, "CLIPBOARD", 1), which is stored in
   * lastSelectionOwner[nxagentClipboardSelection].remSelection. For
   * PRIMARY there's nothing to map because that is identical on all
   * X servers (defined in Xatom.h).
   */

  XlibAtom remote;

  if (local == XA_PRIMARY)
  {
    remote = XA_PRIMARY;
  }
  else if (local == clientCLIPBOARD)
  {
    remote = lastSelectionOwner[nxagentClipboardSelection].remSelection;
  }
  else
  {
    remote = nxagentLocalToRemoteAtom(local);
  }

  #ifdef DEBUG
  fprintf(stderr, "%s: mapping local to remote selection: [%d][%s] -> [%ld] [%s]\n", __func__,
          local, NameForAtom(local), remote, XGetAtomName(nxagentDisplay, remote));
  #endif

  return remote;
}

XlibAtom translateLocalToRemoteTarget(Atom local)
{
  /*
   * .target must be translated, too, as a client on the real
   * server is requested to fill our property and it needs to know
   * the format.
   */

  XlibAtom remote;

  /*
   * we only convert to either UTF8 or XA_STRING, despite accepting
   * TEXT and COMPOUND_TEXT.
   */

  if (local == clientUTF8_STRING)
  {
    remote = serverUTF8_STRING;
  }
#if 0
  else if (local == clientTEXT)
  {
    remote = serverTEXT;
  }
  else if (local == clientCOMPOUND_TEXT)
  {
    remote = serverCOMPOUND_TEXT;
  }
#endif
  else
  {
    remote = XA_STRING;
  }

  #ifdef DEBUG
  fprintf(stderr, "%s: mapping local to remote target: [%d][%s] -> [%ld] [%s]\n", __func__,
          local, NameForAtom(local), remote, XGetAtomName(nxagentDisplay, remote));
  #endif

  return remote;
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
int nxagentSendNotificationToSelfViaXServer(xEvent *event)
{
  if (!agentClipboardInitialized)
  {
    #ifdef DEBUG
    fprintf(stderr, "%s: clipboard not initialized - doing nothing.\n", __func__);
    #endif
    return 0;
  }

  #ifdef DEBUG
  fprintf(stderr, "%s: Received SendNotify by client: property [%d][%s] target [%d][%s] selection [%d][%s] requestor [0x%x] time [%u].\n", __func__,
          event->u.selectionNotify.property, NameForAtom(event->u.selectionNotify.property),
          event->u.selectionNotify.target, NameForAtom(event->u.selectionNotify.target),
          event->u.selectionNotify.selection, NameForAtom(event->u.selectionNotify.selection),
          event->u.selectionNotify.requestor, event->u.selectionNotify.time);
  #endif

  int index = nxagentFindCurrentSelectionIndex(event->u.selectionNotify.selection);
  if (index == nxagentMaxSelections)
  {
    #ifdef DEBUG
    fprintf(stderr, "%s: unknown selection [%d]\n", __func__,
                event->u.selectionNotify.selection);
    #endif
    return 0;
  }

  #ifdef DEBUG
  fprintf(stderr, "%s: lastServers[index].requestor is [0x%x].\n", __func__, lastServers[index].requestor);
  #endif

  /*
   * If we have nested sessions there are situations where we do not
   * need to send out anything to the real X server because
   * communication happens completely between our own clients (some of
   * which can be nxagents themselves). In that case we return 0 (tell
   * dix to go on) and do nothing!
   * Be sure to not let this trigger for the failure answer (property 0)
   */
  if (!(event->u.selectionNotify.property == clientCutProperty || event->u.selectionNotify.property == 0) || lastServers[index].requestor == None)
  {
    #ifdef DEBUG
    fprintf(stderr, "%s: sent nothing - message to real X server is not required.\n", __func__);
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
      .selection = translateLocalToRemoteSelection(event->u.selectionNotify.selection),
      .target    = translateLocalToRemoteTarget(event->u.selectionNotify.target),
      .property  = serverTransFromAgentProperty,
      .time      = CurrentTime,
    };

    sendSelectionNotifyEventToXServer(&eventSelection);

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
  int index = nxagentFindLastSelectionOwnerIndex(serverLastRequestedSelection);
  if (index < nxagentMaxSelections &&
          property == clientCutProperty &&
              lastSelectionOwner[index].windowPtr != NULL)
  {
    #ifdef DEBUG
    fprintf(stderr, "%s: Returning last [%d] selection owner window [%p] (0x%x).\n", __func__,
            lastSelectionOwner[index].intSelection,
            (void *)lastSelectionOwner[index].windowPtr, WINDOWID(lastSelectionOwner[index].windowPtr));
    #endif

    return lastSelectionOwner[index].windowPtr;
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
    /* cannot move that down to others - we need it for
     * initSelectionOwnerData ! */
    clientCLIPBOARD = MakeAtom(szAgentCLIPBOARD, strlen(szAgentCLIPBOARD), True);
    clientTARGETS = MakeAtom(szAgentTARGETS, strlen(szAgentTARGETS), True);
    clientTEXT = MakeAtom(szAgentTEXT, strlen(szAgentTEXT), True);
    clientCOMPOUND_TEXT = MakeAtom(szAgentCOMPOUND_TEXT, strlen(szAgentCOMPOUND_TEXT), True);
    clientUTF8_STRING = MakeAtom(szAgentUTF8_STRING, strlen(szAgentUTF8_STRING), True);
    clientTIMESTAMP = MakeAtom(szAgentTIMESTAMP, strlen(szAgentTIMESTAMP), True);

    SAFE_free(lastSelectionOwner);
    lastSelectionOwner = (SelectionOwner *) malloc(nxagentMaxSelections * sizeof(SelectionOwner));

    if (lastSelectionOwner == NULL)
    {
      FatalError("nxagentInitClipboard: Failed to allocate memory for the clipboard selections.\n");
    }
    initSelectionOwnerData(nxagentPrimarySelection, XA_PRIMARY, XA_PRIMARY);
    initSelectionOwnerData(nxagentClipboardSelection, clientCLIPBOARD, nxagentAtoms[10]);   /* CLIPBOARD */

    SAFE_free(lastClients);
    lastClients = (lastClient *) calloc(nxagentMaxSelections, sizeof(lastClient));
    if (lastClients == NULL)
    {
      FatalError("nxagentInitClipboard: Failed to allocate memory for the last clients array.\n");
    }

    SAFE_free(lastServers);
    lastServers = (lastServer *) calloc(nxagentMaxSelections, sizeof(lastServer));
    if (lastServers == NULL)
    {
      FatalError("nxagentInitClipboard: Failed to allocate memory for the last servers array.\n");
    }
  }
  else
  {
    /* the clipboard selection atom might have changed on a new X
       server. Primary is constant. */
    lastSelectionOwner[nxagentClipboardSelection].remSelection = nxagentAtoms[10];   /* CLIPBOARD */
  }

  serverTARGETS = nxagentAtoms[6];  /* TARGETS */
  serverTEXT = nxagentAtoms[7];  /* TEXT */
  serverCOMPOUND_TEXT = nxagentAtoms[16]; /* COMPOUND_TEXT */
  serverUTF8_STRING = nxagentAtoms[12]; /* UTF8_STRING */
  serverTIMESTAMP = nxagentAtoms[11];   /* TIMESTAMP */

  /*
   * Server side properties to hold pasted data.
   * see nxagentSendNotificationToSelfViaXServer for an explanation
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
  fprintf(stderr, "%s: Setting owner of selection [%d][%s] to serverwindow [0x%x]\n", __func__,
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

    for (int index = 0; index < nxagentMaxSelections; index++)
    {
      XFixesSelectSelectionInput(nxagentDisplay, serverWindow,
                                 lastSelectionOwner[index].remSelection,
                                 XFixesSetSelectionOwnerNotifyMask |
                                 XFixesSelectionWindowDestroyNotifyMask |
                                 XFixesSelectionClientCloseNotifyMask);
    }

    nxagentXFixesInfo.Initialized = True;
  }

#ifdef NXAGENT_ONSTART
  /*
     The first paste from CLIPBOARD did not work directly after
     session start. Removing this code makes it work. It is unsure why
     it was introduced in the first place so it is possible that we
     see other effects by leaving out this code.

     Fixes X2Go bug #952, see https://bugs.x2go.org/952 for details .

  if (nxagentSessionId[0])
  {
    // nxagentAtoms[10] is the CLIPBOARD atom
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
#endif

  if (nxagentReconnectTrap)
  {
    if (nxagentOption(Clipboard) == ClipboardServer ||
        nxagentOption(Clipboard) == ClipboardBoth)
    {
      for (int index = 0; index < nxagentMaxSelections; index++)
      {
        /*
         * if we have a selection inform the (new) real Xserver and
         * claim the ownership. Note that we report our serverWindow as
         * owner, not the real window!
         */
         if (IS_INTERNAL_OWNER(index) && lastSelectionOwner[index].window)
         {
           XSetSelectionOwner(nxagentDisplay, lastSelectionOwner[index].remSelection, serverWindow, CurrentTime);
         }
      }
    }
    /* FIXME: Shouldn't we reset lastServers[index].* and lastClients[index].* here? */
  }
  else
  {
    for (int index = 0; index < nxagentMaxSelections; index++)
    {
      clearSelectionOwnerData(index);
      resetClientSelectionStage(index);
      /* FIXME: required? move to setSelectionStage? */
      lastClients[index].reqTime = GetTimeInMillis();
      lastServers[index].requestor = None;
    }

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
