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
 * define this to see the clipboard content in the debug output. As
 * this can lead to information leaking it must be activated
 * explicitly!
 */
#undef PRINT_CLIPBOARD_CONTENT_ON_DEBUG

/*
 * Define these to also support special targets TEXT and COMPOUND_TEXT
 * in text-only mode. We do not have a special handling for these. See
 * https://www.x.org/releases/X11R7.6/doc/xorg-docs/specs/ICCCM/icccm.html#text_properties
 * for details.
 */
#undef SUPPORT_TEXT_TARGET
#undef SUPPORT_COMPOUND_TEXT_TARGET

/*
 * These are defined in the dispatcher.
 */

extern int NumCurrentSelections;
extern Selection *CurrentSelections;

static int agentClipboardInitialized = False;
static int clientAccum;

XlibAtom serverTransToAgentProperty;
Atom clientCutProperty;
static XlibWindow serverWindow;

const int nxagentPrimarySelection = 0;
const int nxagentClipboardSelection = 1;
const int nxagentMaxSelections = 2;

/* store the remote atom for all selections */
static XlibAtom *remSelAtoms = NULL;
static Atom *intSelAtoms = NULL;

typedef struct _SelectionOwner
{
  ClientPtr  client;           /* internal client */
  Window     window;           /* internal window id */
  WindowPtr  windowPtr;        /* internal window struct */
  Time       lastTimeChanged;  /* internal time */
} SelectionOwner;

/*
 * this contains the last selection owner in nxagent. The
 * lastTimeChanged is always an internal time. If .client is NULL the
 * owner is outside nxagent.
 */

static SelectionOwner *lastSelectionOwner = NULL;

/*
 * cache for targets the current selection owner
 * has to offer. We are storing the targets
 * after they have been converted
*/
typedef struct _Targets
{
  Bool          type;     /* EMPTY, FORINT, FORREM */
  unsigned int  numTargets;
  Atom         *forInt;   /* Atoms converted for internal -> type Atom, not XlibAtom */
  XlibAtom     *forRem;   /* Atoms converted for remote   -> type XlibAtom, not Atom */
} Targets;

#define EMPTY 0
#define FORREM 1
#define FORINT 2

static Targets *targetCache = NULL;

/* FIXME: can this also be stored per selection? */
static XlibAtom serverLastRequestedSelection = -1;

#define IS_INTERNAL_OWNER(lsoindex) (lastSelectionOwner[lsoindex].client != NullClient)

typedef enum
{
  SelectionStageNone,
  SelectionStageQuerySize,
  SelectionStageWaitSize,
  SelectionStageQueryData,
  SelectionStageWaitData
} ClientSelectionStage;

/*
 * Needed to handle the notify selection event to be sent to the
 * waiting client once the selection property has been retrieved from
 * the real X server.
 */

typedef struct _lastClient
{
  WindowPtr     windowPtr;
  ClientPtr     clientPtr;
  Window        requestor;
  Atom          property;
  Atom          target;
  Time          time;
  Time          reqTime;
  unsigned long propertySize;
  ClientSelectionStage stage;
  int           resource;       /* nxcompext resource where collected property data is stored */
} lastClient;

static lastClient *lastClients;

typedef struct _lastServer {
  XlibWindow    requestor;
  XlibAtom      property;
  XlibAtom      target;
  Time          time;
} lastServer;

static lastServer *lastServers;

/*
 * FIXME: use (additional) Atoms.c helpers to get rid of all these
 * Atoms and strings
 */
static XlibAtom serverTARGETS;
static XlibAtom serverTIMESTAMP;
static XlibAtom serverINCR;
static XlibAtom serverMULTIPLE;
static XlibAtom serverDELETE;
static XlibAtom serverINSERT_SELECTION;
static XlibAtom serverINSERT_PROPERTY;
static XlibAtom serverSAVE_TARGETS;
static XlibAtom serverTARGET_SIZES;
#ifdef SUPPORT_TEXT_TARGET
static XlibAtom serverTEXT;
#endif
#ifdef SUPPORT_COMPOUND_TEXT_TARGET
static XlibAtom serverCOMPOUND_TEXT;
#endif
static XlibAtom serverUTF8_STRING;
static XlibAtom serverTransFromAgentProperty;
static Atom clientTARGETS;
static Atom clientTIMESTAMP;
static Atom clientINCR;
static Atom clientMULTIPLE;
static Atom clientDELETE;
static Atom clientINSERT_SELECTION;
static Atom clientINSERT_PROPERTY;
static Atom clientSAVE_TARGETS;
static Atom clientTARGET_SIZES;
#ifdef SUPPORT_TEXT_TARGET
static Atom clientTEXT;
#endif
#ifdef SUPPORT_COMPOUND_TEXT_TARGET
static Atom clientCOMPOUND_TEXT;
#endif
static Atom clientUTF8_STRING;

static char szAgentTARGETS[] = "TARGETS";
#ifdef SUPPORT_TEXT_TARGET
static char szAgentTEXT[] = "TEXT";
#endif
#ifdef SUPPORT_COMPOUND_TEXT_TARGET
static char szAgentCOMPOUND_TEXT[] = "COMPOUND_TEXT";
#endif
static char szAgentTIMESTAMP[] = "TIMESTAMP";
static char szAgentINCR[] = "INCR";
static char szAgentMULTIPLE[] = "MULTIPLE";
static char szAgentDELETE[] = "DELETE";
static char szAgentINSERT_SELECTION[] = "INSERT_SELECTION";
static char szAgentINSERT_PROPERTY[] = "INSERT_PROPERTY";
static char szAgentSAVE_TARGETS[] = "SAVE_TARGETS";
static char szAgentTARGET_SIZES[] = "TARGET_SIZES";
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

static Bool isTextTarget(XlibAtom target);
static void setClientSelectionStage(int index, int stage);
static void endTransfer(int index, Bool success);
#define SELECTION_SUCCESS True
#define SELECTION_FAULT False
static void transferSelectionFromXServer(int index, int resource);
#if 0
static void resetSelectionOwnerOnXServer(void);
#endif
static void initSelectionOwnerData(int index);
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
static void replyPendingRequestSelectionToXServer(int index, Bool success);
#ifdef DEBUG
static void printSelectionStat(int sel);
#endif
static void replyRequestSelectionToXServer(XEvent *X, Bool success);
void handlePropertyTransferFromAgentToXserver(int index, XlibAtom property);

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

  fprintf(stderr, "selection [%d]:\n", index);

  fprintf(stderr, "  selection Atom                         internal [%d][%s]  remote [%ld][%s]\n",
              intSelAtoms[index], NameForIntAtom(intSelAtoms[index]),
                  remSelAtoms[index], NameForRemAtom(remSelAtoms[index]));
  fprintf(stderr, "  owner side                             %s\n", IS_INTERNAL_OWNER(index) ? "nxagent" : "real X server/none");
  fprintf(stderr, "  lastSelectionOwner[].client            %s\n", nxagentClientInfoString(lOwner.client));
  fprintf(stderr, "  lastSelectionOwner[].window            [0x%x]\n", lOwner.window);
  if (lOwner.windowPtr)
    fprintf(stderr, "  lastSelectionOwner[].windowPtr         [%p] (-> [0x%x])\n", (void *)lOwner.windowPtr, WINDOWID(lOwner.windowPtr));
  else
    fprintf(stderr, "  lastSelectionOwner[].windowPtr         -\n");
  fprintf(stderr, "  lastSelectionOwner[].lastTimeChanged   [%u]\n", lOwner.lastTimeChanged);

  fprintf(stderr, "  CurrentSelections[].client             %s\n", nxagentClientInfoString(curSel.client));
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
  fprintf(stderr, "  lastClients[].property          (Atom) [% 4d][%s]\n", lc.property, NameForIntAtom(lc.property));
  fprintf(stderr, "  lastClients[].target            (Atom) [% 4d][%s]\n", lc.target, NameForIntAtom(lc.target));
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
  fprintf(stderr, "  lastServer[].requestor    (XlibWindow) [0x%lx]\n", ls.requestor);
  fprintf(stderr, "  lastServer[].property       (XlibAtom) [% 4ld][%s]\n", ls.property, NameForRemAtom(ls.property));
  fprintf(stderr, "  lastServer[].target         (XlibAtom) [% 4ld][%s]\n", ls.target, NameForRemAtom(ls.target));
  fprintf(stderr, "  lastServer[].time               (Time) [%u]\n", ls.time);
}

static void printTargetCacheStat(int index)
{
  fprintf(stderr, "  targetCache[].type               (int) [%d]\n", targetCache[index].type);
  fprintf(stderr, "  targetCache[].forInt          (Atom *) [%p]\n", (void *)targetCache[index].forInt);
  fprintf(stderr, "  targetCache[].forRem      (XlibAtom *) [%p]\n", (void *)targetCache[index].forRem);
  fprintf(stderr, "  targetCache[].numTargets         (int) [%d]\n", targetCache[index].numTargets);
}

void nxagentDumpClipboardStat(void)
{
  fprintf(stderr, "/----- Clipboard internal status -----\n");

  fprintf(stderr, "  current time                    (Time) [%u]\n", GetTimeInMillis());
  fprintf(stderr, "  agentClipboardInitialized       (Bool) [%s]\n", agentClipboardInitialized ? "True" : "False");
  fprintf(stderr, "  clientAccum                      (int) [%d]\n", clientAccum);
  fprintf(stderr, "  nxagentMaxSelections             (int) [%d]\n", nxagentMaxSelections);
  fprintf(stderr, "  NumCurrentSelections             (int) [%d]\n", NumCurrentSelections);
  fprintf(stderr, "  serverWindow              (XlibWindow) [0x%lx]\n", serverWindow);

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

  if (serverLastRequestedSelection == -1)
      fprintf(stderr, "  serverLastRequestedSelection           [-1](uninitialized)\n");
  else
      fprintf(stderr, "  serverLastRequestedSelection           [% 4ld][%s]\n", serverLastRequestedSelection, NameForRemAtom(serverLastRequestedSelection));

  fprintf(stderr, "Compile time settings\n");
#ifdef PRINT_CLIPBOARD_CONTENT_ON_DEBUG
  fprintf(stderr, "  PRINT_CLIPBOARD_CONTENT_ON_DEBUG       [enabled]\n");
#else
  fprintf(stderr, "  PRINT_CLIPBOARD_CONTENT_ON_DEBUG       [disabled]\n");
#endif
#ifdef SUPPORT_TEXT_TARGET
  fprintf(stderr, "  SUPPORT_TEXT_TARGET                    [enabled]\n");
#else
  fprintf(stderr, "  SUPPORT_TEXT_TARGET                    [disabled]\n");
#endif
#ifdef SUPPORT_COMPOUND_TEXT_TARGET
  fprintf(stderr, "  SUPPORT_COMPOUND_TEXT_TARGET           [enabled]\n");
#else
  fprintf(stderr, "  SUPPORT_COMPOUND_TEXT_TARGET           [disabled]\n");
#endif

#define WIDTH 32
  Atom cl = 0;
  XlibAtom sv = 0;
  int len = WIDTH;

  fprintf(stderr, "Atoms                                    internal%*sremote\n", WIDTH - 8, "");
  cl = clientTARGETS; sv = serverTARGETS; len = (int)(WIDTH - 9 - strlen(NameForIntAtom(cl)));
  fprintf(stderr, "  TARGETS                                [% 4d][%s]%*s [% 4ld][%s]\n", cl, NameForIntAtom(cl), len, "", sv, NameForRemAtom(sv));

  cl = clientTIMESTAMP; sv = serverTIMESTAMP; len = (int)(WIDTH - 9 - strlen(NameForIntAtom(cl)));
  fprintf(stderr, "  TIMESTAMP                              [% 4d][%s]%*s [% 4ld][%s]\n", cl, NameForIntAtom(cl), len, "", sv, NameForRemAtom(sv));

#ifdef SUPPORT_TEXT_TARGET
  cl = clientTEXT; sv = serverTEXT; len = (int)(WIDTH - 9 - strlen(NameForIntAtom(cl)));
  fprintf(stderr, "  TEXT                                   [% 4d][%s]%*s [% 4ld][%s]\n", cl, NameForIntAtom(cl), len, "", sv, NameForRemAtom(sv));
#endif
#ifdef SUPPORT_COMPOUND_TEXT_TARGET
  cl = clientCOMPOUND_TEXT; sv = serverCOMPOUND_TEXT; len = (int)(WIDTH - 9 - strlen(NameForIntAtom(cl)));
  fprintf(stderr, "  COMPOUND_TEXT                          [% 4d][%s]%*s [% 4ld][%s]\n", cl, NameForIntAtom(cl), len, "", sv, NameForRemAtom(sv));
#endif

  cl = clientUTF8_STRING; sv = serverUTF8_STRING; len = (int)(WIDTH - 9 - strlen(NameForIntAtom(cl)));
  fprintf(stderr, "  UTF8_STRING                            [% 4d][%s]%*s [% 4ld][%s]\n", cl, NameForIntAtom(cl), len, "", sv, NameForRemAtom(sv));

  sv = serverTransToAgentProperty;
  fprintf(stderr, "  serverTransToAgentProperty             - %*s[% 4ld][%s]\n", WIDTH - 2, "", sv, NameForRemAtom(sv));

  sv = serverTransFromAgentProperty;
  fprintf(stderr, "  serverTransFromAgentProperty           - %*s[% 4ld][%s]\n", WIDTH - 2, "", sv, NameForRemAtom(sv));

  cl = clientCutProperty; len = (int)(WIDTH - 9 - strlen(NameForIntAtom(cl)));
  fprintf(stderr, "  clientCutProperty                      [% 4d][%s]%*s\n", cl, NameForIntAtom(cl), len + 2, "-" );

  for (int index = 0; index < nxagentMaxSelections; index++)
  {
    printSelectionStat(index);
    printLastClientStat(index);
    printLastServerStat(index);
    printTargetCacheStat(index);
  }

  fprintf(stderr, "\\------------------------------------------------------------------------------\n");
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
  lastClients[index].target = 0;
  lastClients[index].time = 0;
  lastClients[index].reqTime = 0;
  lastClients[index].propertySize = 0;
  lastClients[index].resource = -1;
}

static void setClientSelectionStage(int index, int stage)
{
  if (stage == SelectionStageNone)
  {
    resetClientSelectionStage(index);
  }
  else
  {
    #ifdef DEBUG
    fprintf(stderr, "%s: Changing selection stage for [%d] from [%s] to [%s]\n", __func__, index,
                getClientSelectionStageString(lastClients[index].stage), getClientSelectionStageString(stage));
    #endif
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
  XlibWindow w = event_to_send->requestor;

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
    fprintf(stderr, "%s: XSendEvent to [0x%lx] failed.\n", __func__, w);
  }
  else
  {
    if (result == BadValue || result == BadWindow)
    {
      fprintf(stderr, "%s: WARNING! XSendEvent to [0x%lx] failed: %s\n", __func__, w, getXErrorString(result));
    }
    else
    {
      fprintf(stderr, "%s: XSendEvent() successfully sent to [0x%lx]\n", __func__, w);
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
    fprintf(stderr, "%s: Denying request to client %s - event time [%u].\n", __func__,
            nxagentClientInfoString(client), time);
  else
    fprintf(stderr, "%s: Sending event to client %s - event time [%u].\n", __func__,
            nxagentClientInfoString(client), time);
  #endif

  sendEventToClient(client, &x);
}

/*
 * Check if target is a valid text content type target sent by the real X
 * server, like .e.g XA_STRING or UTF8_STRING.
 */
static Bool isTextTarget(XlibAtom target)
{
  if (target == XA_STRING)
  {
    #ifdef DEBUG
    fprintf(stderr, "%s: valid target [XA_STRING].\n", __func__);
    #endif
    return True;
  }
#ifdef SUPPORT_TEXT_TARGET
  else if (target == serverTEXT)
  {
    #ifdef DEBUG
    fprintf(stderr, "%s: valid target [TEXT].\n", __func__);
    #endif
    return True;
  }
#endif
#ifdef SUPPORT_COMPOUND_TEXT_TARGET
  else if (target == serverCOMPOUND_TEXT)
  {
    #ifdef DEBUG
    fprintf(stderr, "%s: valid target [COMPOUND_TEXT].\n", __func__);
    #endif
    return True;
  }
#endif
  else if (target == serverUTF8_STRING)
  {
    #ifdef DEBUG
    fprintf(stderr, "%s: valid target [UTF8_STRING].\n", __func__);
    #endif
    return True;
  }
  /* FIXME: add text/plain */

  #ifdef DEBUG
  fprintf(stderr, "%s: not a text target [%lu].\n", __func__, target);
  #endif
  return False;
}

static void initSelectionOwnerData(int index)
{
  lastSelectionOwner[index].client = NullClient;
  lastSelectionOwner[index].window = screenInfo.screens[0]->root->drawable.id;
  lastSelectionOwner[index].windowPtr = NULL;
  lastSelectionOwner[index].lastTimeChanged = GetTimeInMillis();
}

/* there's no owner on nxagent side anymore */
static void clearSelectionOwnerData(int index)
{
  lastSelectionOwner[index].client = NullClient;
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

  /* FIXME: there's almost identical code in nxagentClipboardInit */
  for (int index = 0; index < nxagentMaxSelections; index++)
  {
    if (matchSelectionOwner(index, pClient, pWindow))
    {
      #ifdef TEST
      fprintf(stderr, "%s: Resetting state [%d] with client [%p] window [%p].\n", __func__,
                  index, (void *) pClient, (void *) pWindow);
      #endif

      clearSelectionOwnerData(index);

      setClientSelectionStage(index, SelectionStageNone);

      replyPendingRequestSelectionToXServer(index, False);
    }

    if (pWindow && pWindow == lastClients[index].windowPtr)
    {
      setClientSelectionStage(index, SelectionStageNone);
    }
  }
}

/*
 * Find the index of the lastSelectionOwner with the selection
 * sel. sel is an atom on the real X server. If the index cannot be
 * determined it will return -1.
 */
int nxagentFindLastSelectionOwnerIndex(XlibAtom sel)
{
  for (int index = 0; index < nxagentMaxSelections; index++)
  {
    if (remSelAtoms[index] == sel)
    {
      #ifdef DEBUG
      fprintf(stderr, "%s: remote selection [%ld][%s] belongs to index [%d]\n", __func__, sel, NameForRemAtom(sel), index);
      #endif
      return index;
    }
  }
  #ifdef DEBUG
  fprintf(stderr, "%s: remote selection [%ld][%s] does not belong to any index!\n", __func__, sel, NameForRemAtom(sel));
  #endif
  return -1;
}

/*
 * Find the index of CurrentSelection with the selection
 * sel. sel is an internal atom. If the index cannot be
 * determined it will return -1.
 */
int nxagentFindCurrentSelectionIndex(Atom sel)
{
  /*
   * Normally you'd expect the loop going up to
   * NumCurrentSelections. But the dix code will increase that number
   * (but not nxagentMaxSelections) when drag and drop comes into
   * play. In that case this helper will report a match for other
   * selections than the ones the clipboard code knows about. The
   * subsequent code will then use a higher index which will be used
   * by the clipboard code and will lead to out of range data reads
   * (and writes!). Therefore we take nxagentMaxSelections here. The
   * startup code ensures that both arrays will refer to the same
   * selection for the first nxagentMaxSelections selection atoms.
   */

  //  for (int index = 0; index < NumCurrentSelections; index++)
  for (int index = 0; index < nxagentMaxSelections; index++)
  {
    if (CurrentSelections[index].selection == sel)
    {
      #ifdef DEBUG
      fprintf(stderr, "%s: selection [%d][%s] belongs to index [%d]\n", __func__, sel, NameForIntAtom(sel), index);
      #endif
      return index;
    }
  }
  #ifdef DEBUG
  fprintf(stderr, "%s: selection [%d][%s] does not belong to any index!\n", __func__, sel, NameForIntAtom(sel));
  #endif
  return -1;
}

void cacheTargetsForInt(int index, Atom* targets, int numTargets)
{
  #ifdef DEBUG
  fprintf(stderr, "%s: caching [%d] targets for internal requests\n", __func__, numTargets);
  #endif

  SAFE_free(targetCache[index].forInt);
  SAFE_free(targetCache[index].forRem);
  targetCache[index].type = FORINT;
  targetCache[index].forInt = targets;
  targetCache[index].numTargets = numTargets;
}

void cacheTargetsForRem(int index, XlibAtom* targets, int numTargets)
{
  #ifdef DEBUG
  fprintf(stderr, "%s: caching [%d] targets for remote requests\n", __func__, numTargets);
  #endif

  SAFE_free(targetCache[index].forInt);
  SAFE_free(targetCache[index].forRem);
  targetCache[index].type = FORREM;
  targetCache[index].forRem = targets;
  targetCache[index].numTargets = numTargets;
}

/* this is called on init, reconnect and SelectionClear */
void invalidateTargetCache(int index)
{
  #ifdef DEBUG
  fprintf(stderr, "%s: invalidating target cache [%d]\n", __func__, index);
  #endif

  SAFE_free(targetCache[index].forInt);
  SAFE_free(targetCache[index].forRem);
  targetCache[index].type = EMPTY;
  targetCache[index].numTargets = 0;
}

void invalidateTargetCaches(void)
{
  #ifdef DEBUG
  fprintf(stderr, "%s: invalidating all target caches\n", __func__);
  #endif

  for (int index = 0; index < nxagentMaxSelections; index++)
  {
    SAFE_free(targetCache[index].forInt);
    SAFE_free(targetCache[index].forRem);
    targetCache[index].type = EMPTY;
    targetCache[index].numTargets = 0;
  }
}

/*
 * This is called from Events.c dispatch loop on reception of a
 * SelectionClear event. We receive this event if someone on the real
 * X server claims the selection ownership.
 */
void nxagentHandleSelectionClearFromXServer(XEvent *X)
{
  #ifdef DEBUG
  fprintf(stderr, "---------\n%s: SelectionClear event for selection [%lu][%s] window [0x%lx] time [%lu].\n",
              __func__, X->xselectionclear.selection, NameForRemAtom(X->xselectionclear.selection),
                  X->xselectionclear.window, X->xselectionclear.time);
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
  if (index != -1)
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

    setClientSelectionStage(index, SelectionStageNone);

    invalidateTargetCache(index);
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
  fprintf(stderr, "---------\n%s: Received SelectionRequestEvent from real server: selection [%ld][%s] " \
          "target [%ld][%s] requestor [display[%s]/0x%lx] destination [%ld][%s] time [%lu]\n",
          __func__,
          X->xselectionrequest.selection, NameForRemAtom(X->xselectionrequest.selection),
          X->xselectionrequest.target,    NameForRemAtom(X->xselectionrequest.target),
          DisplayString(nxagentDisplay), X->xselectionrequest.requestor,
          X->xselectionrequest.property,  NameForRemAtom(X->xselectionrequest.property),
          X->xselectionrequest.time);
  if (X->xselectionrequest.requestor == serverWindow)
  {
    fprintf(stderr, "%s: this event has been sent by nxagent!\n", __func__);;
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
  if (index == -1)
  {
    #ifdef DEBUG
    fprintf(stderr, "%s: not owning selection [%ld] - denying request.\n", __func__, X->xselectionrequest.selection);
    #endif

    replyRequestSelectionToXServer(X, False);
    return;
  }

  #ifdef DEBUG
  fprintf(stderr, "%s: lastServers[%d].requestor [0x%lx].\n", __func__, index, lastServers[index].requestor);
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

  if (X->xselectionrequest.target == serverTARGETS)
  {
    /*
     * In TextClipboard mode answer with a predefined list of
     * targets. This is just the previous implementation of handling
     * the clipboard.
     */
    if (nxagentOption(TextClipboard))
    {
      /*
       * the selection request target is TARGETS. The requestor is
       * asking for a list of supported data formats.
       *
       * The selection does not matter here, we will return this for
       * PRIMARY and CLIPBOARD.
       *
       * The list is aligned with the one in nxagentConvertSelection()
       * and in isTextTarget().
       */

      XlibAtom targets[] = {XA_STRING,
                            serverUTF8_STRING,
#ifdef SUPPORT_TEXT_TARGET
                            serverTEXT,
#endif
#ifdef SUPPORT_COMPOUND_TEXT_TARGET
                            serverCOMPOUND_TEXT,
#endif
                            serverTARGETS,
                            serverTIMESTAMP};
      int numTargets = sizeof(targets) / sizeof(targets[0]);

      #ifdef DEBUG
      fprintf(stderr, "%s: Sending %d available targets:\n", __func__, numTargets);
      for (int i = 0; i < numTargets; i++)
      {
        fprintf(stderr, "%s: %ld %s\n", __func__, targets[i], NameForRemAtom(targets[i]));
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
                      (unsigned char*)targets,
                      numTargets);

      replyRequestSelectionToXServer(X, True);
      return;
    }
    else
    {
      /*
       * Shortcut: Some applications tend to post multiple
       * SelectionRequests. Further it can happen that multiple
       * clients are interested in clipboard content. If we already
       * know the answer and no intermediate SelectionOwner event
       * occured we can answer with the cached list of targets.
       */

      if (targetCache[index].type == FORREM && targetCache[index].forRem)
      {
        XlibAtom *targets = targetCache[index].forRem;
        unsigned int numTargets = targetCache[index].numTargets;

        #ifdef DEBUG
        fprintf(stderr, "%s: Sending %d cached targets to remote requestor:\n", __func__, numTargets);
        for (int i = 0; i < numTargets; i++)
        {
          fprintf(stderr, "%s: %ld %s\n", __func__, targets[i], NameForRemAtom(targets[i]));
        }
        #endif

        XChangeProperty(nxagentDisplay,
                        X->xselectionrequest.requestor,
                        X->xselectionrequest.property,
                        XInternAtom(nxagentDisplay, "ATOM", 0),
                        32,
                        PropModeReplace,
                        (unsigned char *)targets,
                        numTargets);

        replyRequestSelectionToXServer(X, True);
        return;
      }
    }
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
    return;
  }
  else if (X->xselectionrequest.target == serverMULTIPLE)
  {
    #ifdef DEBUG
    fprintf(stderr, "%s: (currently) unsupported target [MULTIPLE] - denying request.\n", __func__);
    #endif
    replyRequestSelectionToXServer(X, False);
    return;
  }
  else if (X->xselectionrequest.target == serverDELETE)
  {
    #ifdef DEBUG
    fprintf(stderr, "%s: (currently) unsupported target [DELETE] - denying request.\n", __func__);
    #endif
    replyRequestSelectionToXServer(X, False);
    return;
  }
  else if (X->xselectionrequest.target == serverINSERT_SELECTION)
  {
    #ifdef DEBUG
    fprintf(stderr, "%s: (currently) unsupported target [INSERT_SELECTION] - denying request.\n", __func__);
    #endif
    replyRequestSelectionToXServer(X, False);
    return;
  }
  else if (X->xselectionrequest.target == serverINSERT_PROPERTY)
  {
    #ifdef DEBUG
    fprintf(stderr, "%s: (currently) unsupported target [INSERT_PROPERTY] - denying request.\n", __func__);
    #endif
    replyRequestSelectionToXServer(X, False);
    return;
  }
  else if (X->xselectionrequest.target == serverSAVE_TARGETS)
  {
    #ifdef DEBUG
    fprintf(stderr, "%s: (currently) unsupported target [SAVE_TARGETS] - denying request.\n", __func__);
    #endif
    replyRequestSelectionToXServer(X, False);
    return;
  }
  else if (X->xselectionrequest.target == serverTARGET_SIZES)
  {
    #ifdef DEBUG
    fprintf(stderr, "%s: (currently) unsupported target [TARGET_SIZES] - denying request.\n", __func__);
    #endif
    replyRequestSelectionToXServer(X, False);
    return;
  }

  if (nxagentOption(TextClipboard))
  {
    if (!isTextTarget(X->xselectionrequest.target))
    {
      #ifdef DEBUG
      fprintf(stderr, "%s: denying request for non-text target [%ld][%s].\n", __func__,
                  X->xselectionrequest.target, NameForRemAtom(X->xselectionrequest.target));
      #endif

      replyRequestSelectionToXServer(X, False);

      return;
    }
    /* go on, target is acceptable */
  }

  #ifdef DEBUG
  fprintf(stderr, "%s: target [%ld][%s].\n", __func__, X->xselectionrequest.target,
              NameForRemAtom(X->xselectionrequest.target));
  #endif

  /*
   * reaching this means the request is a normal, valid request. We
   * can process it now.
   */

  if (!nxagentOption(TextClipboard))
  {
    /* Optimization: if we have a current target cache check if the
     * requested target is supported by the owner. If not we can take
     * a shortcut and deny the request immediately without doing any
     * further communication */
    if (targetCache[index].type == FORREM && targetCache[index].forRem)
    {
      XlibAtom *targets = targetCache[index].forRem;

      #ifdef DEBUG
      fprintf(stderr, "%s: Checking target validity\n", __func__);
      #endif
      Bool match = False;
      for (int i = 0; i < targetCache[index].numTargets; i++)
      {
        if (targets[i] == X->xselectionrequest.target)
        {
          match = True;
          break;
        }
      }
      if (!match)
      {
        #ifdef DEBUG
        fprintf(stderr, "%s: target [%ld][%s] is not supported by the owner - denying request.\n",
                    __func__, X->xselectionrequest.target, NameForRemAtom(X->xselectionrequest.target));
        #endif
        replyRequestSelectionToXServer(X, False);
        return;
      }
    }
    else
    {
      /*
       * at this stage we know a remote client has asked for a selection
       * target without having retrieved the list of supported targets
       * first.
       */
      #ifdef DEBUG
      if (X->xselectionrequest.target != serverTARGETS)
      {
         fprintf(stderr, "%s: WARNING: remote client has not retrieved TARGETS before asking for selection!\n",
                   __func__);
      }
      #endif
    }
  }

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
    XDeleteProperty(nxagentDisplay, serverWindow, serverTransToAgentProperty);
    XConvertSelection(nxagentDisplay, CurrentSelections[index].selection,
                          X->xselectionrequest.target, serverTransToAgentProperty,
                              serverWindow, lastClients[index].time);

    NXFlushDisplay(nxagentDisplay, NXFlushLink);

    #ifdef DEBUG
    fprintf(stderr, "%s: Sent XConvertSelection: selection [%d][%s] target [%ld][%s] property [%ld][%s] window [0x%lx] time [%u] .\n", __func__,
                CurrentSelections[index].selection, NameForRemAtom(CurrentSelections[index].selection)),
                    X->xselectionrequest.target, NameForRemAtom(X->xselectionrequest.target),
                        serverTransToAgentProperty, NameForRemAtom(serverTransToAgentProperty),
                            serverWindow, lastClients[index].time);
    #endif
  }
  else
#endif
  {
    if (!(nxagentOption(Clipboard) == ClipboardServer ||
          nxagentOption(Clipboard) == ClipboardBoth))
    {
      #ifdef DEBUG
      fprintf (stderr, "%s: clipboard (partly) disabled - denying request.\n", __func__);
      #endif

      /* deny the request */
      replyRequestSelectionToXServer(X, False);
      return;
    }

    /*
     * if one of our clients owns the selection we ask it to copy
     * the selection to the clientCutProperty on nxagent's root
     * window in the first step. We then later push that property's
     * content to the real X server.
     */
    if (IS_INTERNAL_OWNER(index))
    {
      /*
       * store who on the real X server requested the data and how
       * and where it wants to have it
       */
      lastServers[index].property = X->xselectionrequest.property;
      lastServers[index].requestor = X->xselectionrequest.requestor;
      lastServers[index].target = X->xselectionrequest.target;
      lastServers[index].time = X->xselectionrequest.time;

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
       * don't notify at all.
       *
       * x.u.selectionRequest.requestor = lastSelectionOwner[index].window;
       */

      /*
       * if textclipboard is requested simply use the previous clipboard
       * handling code
       */
      if (nxagentOption(TextClipboard))
      {
        /* by dimbor */
        if (X->xselectionrequest.target != XA_STRING)
        {
          lastServers[index].target = serverUTF8_STRING;
          /* by dimbor (idea from zahvatov) */
          x.u.selectionRequest.target = clientUTF8_STRING;
        }
        else
        {
          x.u.selectionRequest.target = XA_STRING;
        }
      }
      else
      {
        x.u.selectionRequest.target = nxagentRemoteToLocalAtom(X->xselectionrequest.target);
      }

      /*
       * delete property before sending the request to the client -
       * required by ICCCM
       */
      DeleteProperty(lastSelectionOwner[index].windowPtr, clientCutProperty);

      sendEventToClient(lastSelectionOwner[index].client, &x);

      #ifdef DEBUG
      fprintf(stderr, "%s: sent SelectionRequest event to client %s property [%d][%s] " \
              "target [%d][%s] requestor [0x%x] selection [%d][%s].\n", __func__,
              nxagentClientInfoString(lastSelectionOwner[index].client),
              x.u.selectionRequest.property, NameForIntAtom(x.u.selectionRequest.property),
              x.u.selectionRequest.target, NameForIntAtom(x.u.selectionRequest.target),
              x.u.selectionRequest.requestor,
              x.u.selectionRequest.selection, NameForIntAtom(x.u.selectionRequest.selection));
      #endif
      /* no reply to Xserver yet - we will do that once the answer of
         the above sendEventToClient arrives */
    }
    else
    {
      #ifdef DEBUG
      fprintf(stderr, "%s: no internal owner for selection [%ld][%s] - denying request.\n", __func__,
                  X->xselectionrequest.selection, NameForRemAtom(X->xselectionrequest.selection));
      #endif

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
static void endTransfer(int index, Bool success)
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
                  nxagentClientInfoString(lastClients[index].clientPtr), lastClients[index].property, NameForIntAtom(lastClients[index].property));
    else
      fprintf(stderr, "%s: sending negative notification to client %s\n", __func__,
                  nxagentClientInfoString(lastClients[index].clientPtr));
    #endif

    sendSelectionNotifyEventToClient(lastClients[index].clientPtr,
                                     lastClients[index].time,
                                     lastClients[index].requestor,
                                     intSelAtoms[index],
                                     lastClients[index].target,
                                     success == SELECTION_SUCCESS ? lastClients[index].property : None);
  }

  /*
   * Enable further requests from clients.
   */
  setClientSelectionStage(index, SelectionStageNone);
}

static void transferSelectionFromXServer(int index, int resource)
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

    endTransfer(index, SELECTION_FAULT);

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

        endTransfer(index, SELECTION_FAULT);

        return;
      }

      setClientSelectionStage(index, SelectionStageWaitSize);

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

      /* get next free resource slot */
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
        /*
         * now initiate kind of an asynchronuos GetProperty()
         * call. Once the property has been retrieved we will
         * receive an NXCollectPropertyNotify event which will then
         * be handled in
         * nxagentCollectPropertyEventFromXServer().
         */
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

        endTransfer(index, SELECTION_FAULT);

        return;
      }

      setClientSelectionStage(index, SelectionStageWaitData);

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

  if (resource < 0)
  {
    #ifdef DEBUG
    fprintf (stderr, "%s: resource [%d] is invalid.\n", __func__, resource);
    #endif
    return False;
  }

  /* determine the selection we are talking about here */
  for (index = 0; index < nxagentMaxSelections; index++)
  {
    /*
    #ifdef DEBUG
    fprintf(stderr, "%s: lastClients[%d].resource [%d] resource [%d]\n", __func__, index, lastClients[index].resource, resource);
    #endif
    */
    if (lastClients[index].resource == resource)
    {
      #ifdef DEBUG
      fprintf (stderr, "%s: resource [%d] belongs to selection [%d].\n", __func__, resource, index);
      #endif
      break;
    }
  }

  if (index == nxagentMaxSelections)
  {
    /*
    #ifdef DEBUG
    fprintf (stderr, "%s: resource [%d] does not belong to any selection we handle.\n", __func__, resource);
    #endif
    */
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

  #ifdef DEBUG
  fprintf(stderr, "%s: NXGetCollectedProperty: result [%d]\n", __func__, result);
  fprintf(stderr, "%s:   atomReturnType [%ld]\n", __func__, atomReturnType);
  fprintf(stderr, "%s:   resultFormat [%d]\n", __func__, resultFormat);
  fprintf(stderr, "%s:   ulReturnItems [%lu]\n", __func__, ulReturnItems);
  fprintf(stderr, "%s:   ulReturnBytesLeft [%lu]\n", __func__, ulReturnBytesLeft);
  #endif

  lastClients[index].resource = -1;

  /*
   * ICCCM states: "The requestor must delete the property named in
   * the SelectionNotify once all the data has been retrieved. The
   * requestor should invoke either DeleteProperty or GetProperty
   * (delete==True) after it has successfully retrieved all the data
   * in the selection."
   * FIXME: this uses serverTransToAgentProperty which is shared between
   * all the selections. Could be a problem with simultaneous transfers.
   * FIXME: NXGetCollectedProperty can return 0 and True. Some other
   * functions in this field return False as well. Clean up that
   * mess...
   */
  if (result == True && ulReturnBytesLeft == 0)
  {
    #ifdef DEBUG
    fprintf (stderr, "%s: Retrieved property data - deleting property [%ld][%s] "
             "for ICCCM conformity.\n", __func__, serverTransToAgentProperty,
             NameForRemAtom(serverTransToAgentProperty));
    #endif
    XDeleteProperty(nxagentDisplay, serverWindow, serverTransToAgentProperty);
  }

  if (result == 0)
  {
    #ifdef DEBUG
    fprintf (stderr, "%s: Failed to get reply data.\n", __func__);
    #endif

    endTransfer(index, SELECTION_FAULT);
  }
  else if (resultFormat != 8 && resultFormat != 16 && resultFormat != 32)
  {
    #ifdef DEBUG
    fprintf(stderr, "%s: WARNING! Invalid property format [%d].\n", __func__, resultFormat);
    #endif

    endTransfer(index, SELECTION_FAULT);
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

          endTransfer(index, SELECTION_FAULT);
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
          setClientSelectionStage(index, SelectionStageQueryData);

          transferSelectionFromXServer(index, resource);
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

          endTransfer(index, SELECTION_FAULT);
        }
        else
        {
          #ifdef DEBUG
          fprintf(stderr, "%s: Got property content from remote server. [%lu] items with format [%d] = [%lu] bytes.\n", __func__, ulReturnItems, resultFormat, (ulReturnItems * resultFormat/8));
          #endif

          if (lastClients[index].target == clientTARGETS)
          {
            Atom * targets = calloc(sizeof(Atom), ulReturnItems);
            if (targets == NULL)
            {
              #ifdef WARNING
              fprintf(stderr, "%s: WARNING! Could not alloc memory for clipboard targets transmission.\n", __func__);
              #endif

              /* operation failed */
              endTransfer(index, SELECTION_FAULT);
            }
            else
            {
              /* fprintf(stderr, "sizeof(Atom) [%lu], sizeof(XlibAtom) [%lu], sizeof(long) [%lu], sizeof(CARD32) [%lu] sizeof(INT32) [%lu]\n", sizeof(Atom), sizeof(XlibAtom), sizeof(long), sizeof(CARD32), sizeof(INT32)); */

              Atom *addr = targets;
              unsigned int numTargets = ulReturnItems;

              for (int i = 0; i < numTargets; i++)
              {
                XlibAtom remote = *((XlibAtom*)(pszReturnData + i*resultFormat/8));
                Atom local = nxagentRemoteToLocalAtom(remote);
                *(addr++) = local;

                #ifdef DEBUG
                fprintf(stderr, "%s: converting atom: remote [%u][%s] -> local [%u][%s]\n", __func__,
                            (unsigned int)remote, NameForRemAtom(remote), local, NameForIntAtom(local));
                #endif
              }
              ChangeWindowProperty(lastClients[index].windowPtr,
                                   lastClients[index].property,
                                   MakeAtom("ATOM", 4, 1),
                                   32, PropModeReplace,
                                   ulReturnItems, (unsigned char*)targets, 1);

              cacheTargetsForInt(index, targets, numTargets);

              endTransfer(index, SELECTION_SUCCESS);
            }
          }
          else
          {
            ChangeWindowProperty(lastClients[index].windowPtr,
                                 lastClients[index].property,
                                 lastClients[index].target,
                                 resultFormat, PropModeReplace,
                                 ulReturnItems, pszReturnData, 1);

            #ifdef DEBUG
            fprintf(stderr, "%s: Selection property [%d][%s] changed to"
                    #ifdef PRINT_CLIPBOARD_CONTENT_ON_DEBUG
                    "[\"%*.*s\"...]"
                    #endif
                    "\n", __func__,
                    lastClients[index].property,
                    validateString(NameForIntAtom(lastClients[index].property))
                    #ifdef PRINT_CLIPBOARD_CONTENT_ON_DEBUG
                    ,(int)(min(20, ulReturnItems * resultFormat / 8)),
                    (int)(min(20, ulReturnItems * resultFormat / 8)),
                    pszReturnData
                    #endif
                   );
            #endif

            endTransfer(index, SELECTION_SUCCESS);
          }
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

  XSelectionEvent * e = (XSelectionEvent *)X;
  #ifdef DEBUG
  fprintf(stderr, "---------\n%s: Received SelectionNotify event from real X server, property " \
              "[%ld][%s] requestor [0x%lx] selection [%s] target [%ld][%s] time [%lu] send_event [%d].\n",
                  __func__, e->property, NameForRemAtom(e->property), e->requestor,
                      NameForRemAtom(e->selection), e->target,
                          NameForRemAtom(e->target), e->time, e->send_event);

  /* this has not been SENT by nxagent but is the answer to a request of nxagent */
  if (e->requestor == serverWindow)
  {
    fprintf(stderr, "%s: requestor is nxagent's serverWindow!\n", __func__);;
  }
  #endif

  /* determine the selection we are talking about here */
  int index = nxagentFindLastSelectionOwnerIndex(e->selection);
  if (index == -1)
  {
    #ifdef DEBUG
    fprintf (stderr, "%s: unknown selection [%ld] .\n", __func__, e->selection);
    #endif
    return;
  }

  printClientSelectionStage(index);

  /*
   * if the property is serverTransFromAgentProperty this means we are
   * transferring data FROM the agent TO the server.
   */

  if (X->xselection.property != serverTransFromAgentProperty && lastClients[index].windowPtr != NULL)
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

        setClientSelectionStage(index, SelectionStageQueryData);
        lastClients[index].propertySize = 262144;

        transferSelectionFromXServer(index, lastClients[index].clientPtr -> index);
      }
      else if (X->xselection.property == 0)
      {
        #ifdef DEBUG
        fprintf(stderr, "%s: WARNING! Resetting selection transferral for client [%d] because of failure notification from real X server.\n", __func__,
                    CLINDEX(lastClients[index].clientPtr));
        #endif
        endTransfer(index, SELECTION_FAULT);
      }
      else
      {
        #ifdef DEBUG
        fprintf(stderr, "%s: Unexpected property [%ld][%s] - reporting conversion failure.\n",
                    __func__, X->xselection.property, NameForRemAtom(X->xselection.property));
        #endif
        endTransfer(index, SELECTION_FAULT);
      }
    }
    else
    {
      #ifdef DEBUG
      fprintf(stderr, "%s: WARNING! Resetting selection transferral for client [%d] because of unexpected stage.\n", __func__,
                  CLINDEX(lastClients[index].clientPtr));
      #endif

      endTransfer(index, SELECTION_FAULT);
    }
  }
  else
  {
    handlePropertyTransferFromAgentToXserver(index, X->xselection.property);
  }
}

void handlePropertyTransferFromAgentToXserver(int index, XlibAtom property)
{
  /* if the last owner was an internal one, read the
   * clientCutProperty and push the contents to the
   * lastServers[index].requestor on the real X server.
   */
  if (IS_INTERNAL_OWNER(index) &&
         lastSelectionOwner[index].windowPtr != NULL &&
             property == serverTransFromAgentProperty)
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
    fprintf(stderr, "%s: GetWindowProperty() window [0x%x] property [%d][%s] returned [%s]\n", __func__,
                lastSelectionOwner[index].window, clientCutProperty, NameForIntAtom(clientCutProperty),
                    getXErrorString(result));
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
      fprintf(stderr, "%s: GetWindowProperty() window [0x%x] property [%d][%s] returned [%s]\n", __func__,
                  lastSelectionOwner[index].window, clientCutProperty, NameForIntAtom(clientCutProperty),
                      getXErrorString(result));
      #endif

      if (result == BadAlloc || result == BadAtom ||
              result == BadWindow || result == BadValue)
      {
        lastServers[index].property = None;
      }
      else
      {
        if (lastServers[index].target == serverTARGETS)
        {
          #ifdef DEBUG
          fprintf(stderr, "%s: ulReturnItems [%ld]\n", __func__, ulReturnItems);
          fprintf(stderr, "%s: resultformat [%d]\n", __func__, resultFormat);
          #endif

          XlibAtom * targets = calloc(sizeof(XlibAtom), ulReturnItems);
          if (targets == NULL)
          {
            #ifdef WARNING
            fprintf(stderr, "%s: WARNING! Could not alloc memory for clipboard targets transmission.\n", __func__);
            #endif
            /* this will effectively lead to the request being answered as failed */
            lastServers[index].property = None;
          }
          else
          {
            /* Convert the targets to remote atoms */
            XlibAtom *addr = targets;
            unsigned int numTargets = ulReturnItems;

            for (int i = 0; i < numTargets; i++)
            {
              Atom local = *((Atom*)(pszReturnData + i*resultFormat/8));
              XlibAtom remote = nxagentLocalToRemoteAtom(local);
              *(addr++) = remote;

              #ifdef DEBUG
              fprintf(stderr, "%s: converting atom: local [%d][%s] -> remote [%ld][%s]\n", __func__,
                          local, NameForIntAtom(local), remote, NameForRemAtom(remote));
              #endif
            }

            /* FIXME: do we need to take care of swapping byte order here? */
            XChangeProperty(nxagentDisplay,
                            lastServers[index].requestor,
                            lastServers[index].property,
                            XInternAtom(nxagentDisplay, "ATOM", 0),
                            32,
                            PropModeReplace,
                            (unsigned char*)targets,
                            numTargets);

            cacheTargetsForRem(index, targets, numTargets);
          }
        }
        else
        {
          /* Fill the property on the requestor with the requested data */
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
            fprintf(stderr, "%s: XChangeProperty sent to window [0x%lx] for property [%ld][%s] len [%d]"
                    #ifdef PRINT_CLIPBOARD_CONTENT_ON_DEBUG
		    "value [\"%*.*s\"...]"
                    #endif
		    "\n",
                    __func__,
                    lastServers[index].requestor,
                    lastServers[index].property,
                    NameForRemAtom(lastServers[index].property),
                    (int)ulReturnItems * 8 / 8
                    #ifdef PRINT_CLIPBOARD_CONTENT_ON_DEBUG
                    ,(int)(min(20, ulReturnItems * 8 / 8)),
                    (int)(min(20, ulReturnItems * 8 / 8)),
                    pszReturnData
                    #endif
		    );
          }
          #endif
        }

        /* FIXME: free it or not? */
        /*
         * SAFE_XFree(pszReturnData);
         */
      }
    }

    /*
     * inform the initial requestor that the requested data has
     * arrived in the desired property. If we have been unable to
     * get the data from the owner XChangeProperty will not have
     * been called and lastServers[index].property will be None which
     * effectively will send a "Request denied" to the initial
     * requestor.
     */
    replyPendingRequestSelectionToXServer(index, True);
  }
}

/*
 * This is similar to replyRequestSelectionToXServer(), but gets the
 * required values from a stored request instead of an XEvent
 * structure.
 */
void replyPendingRequestSelectionToXServer(int index, Bool success)
{
  if (lastServers[index].requestor == None)
  {
    #ifdef DEBUG
    fprintf(stderr, "%s: no pending request for index [%d] - doing nothing\n", __func__, index);
    #endif
  }
  else
  {
    XSelectionEvent eventSelection = {
      .requestor = lastServers[index].requestor,
      .selection = remSelAtoms[index],
      .target    = lastServers[index].target,
      .time      = lastServers[index].time,
      .property  = success ? lastServers[index].property : None,
    };

    #ifdef DEBUG
    fprintf(stderr, "%s: Sending %s SelectionNotify event to requestor [%p].\n", __func__,
                success ? "positive" : "negative", (void *)eventSelection.requestor);
    #endif

    sendSelectionNotifyEventToXServer(&eventSelection);

    lastServers[index].requestor = None; /* allow further request */
    lastServers[index].property = 0;
    lastServers[index].target = 0;
    lastServers[index].time = 0;
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

  for (int index = 0; index < nxagentMaxSelections; index++)
  {
    XSetSelectionOwner(nxagentDisplay, remSelAtoms[index], serverWindow, CurrentTime);

    #ifdef DEBUG
    fprintf(stderr, "%s: Reset selection state for selection [%d].\n", __func__, index);
    #endif

    clearSelectionOwnerData(index);

    setClientSelectionStage(index, SelectionStageNone);

    /* Hmm, this is already None when reaching here */
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

  SelectionInfoRec *info = (SelectionInfoRec *)args;

  #ifdef DEBUG
  fprintf(stderr, "---------\n");
  if (info->kind == SelectionSetOwner)
  {
    fprintf(stderr, "%s: SelectionCallbackKind [SelectionSetOwner]\n", __func__);
  }
  else if (info->kind == SelectionWindowDestroy)
  {
    fprintf(stderr, "%s: SelectionCallbackKind [SelectionWindowDestroy]\n", __func__);
  }
  else if (info->kind == SelectionClientClose)
  {
    fprintf(stderr, "%s: SelectionCallbackKind [SelectionClientClose]\n", __func__);
  }
  else
  {
    fprintf(stderr, "%s: SelectionCallbackKind [unknown]\n", __func__);
  }
  #endif

  Selection * pCurSel = (Selection *)info->selection;

  int index = nxagentFindCurrentSelectionIndex(pCurSel->selection);
  if (index == -1)
  {
    #ifdef DEBUG
    fprintf(stderr, "%s: selection [%s] will not be handled by the clipboard code\n", __func__, NameForIntAtom(pCurSel->selection));
    #endif
    return;
  }

  /*
   * always invalidate the target cache for the relevant selection,
   * even if the trap is set. This ensures not having invalid data in
   * the cache.
   */
  invalidateTargetCache(index);

  if (nxagentExternalClipboardEventTrap)
  {
    #ifdef DEBUG
    fprintf(stderr, "%s: Trap is set, doing nothing\n", __func__);
    #endif
    return;
  }


  #ifdef DEBUG
  fprintf(stderr, "%s: pCurSel->lastTimeChanged [%u]\n", __func__, pCurSel->lastTimeChanged.milliseconds);
  #endif

  if (info->kind == SelectionSetOwner)
  {
    #ifdef DEBUG
    fprintf(stderr, "%s: pCurSel->pWin [0x%x]\n", __func__, WINDOWID(pCurSel->pWin));
    fprintf(stderr, "%s: pCurSel->selection [%s]\n", __func__, NameForIntAtom(pCurSel->selection));
    #endif

    if (pCurSel->pWin != NULL &&
        nxagentOption(Clipboard) != ClipboardNone && /* FIXME: shouldn't we also check for != ClipboardClient? */
        (pCurSel->selection == intSelAtoms[nxagentPrimarySelection] ||
         pCurSel->selection == intSelAtoms[nxagentClipboardSelection]))
    {
      #ifdef DEBUG
      fprintf(stderr, "%s: calling setSelectionOwnerOnXServer\n", __func__);
      #endif
      setSelectionOwnerOnXServer(pCurSel);
    }
  }
  else if (info->kind == SelectionWindowDestroy)
  {
  }
  else if (info->kind == SelectionClientClose)
  {
  }
  else
  {
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
  fprintf(stderr, "%s: Setting selection owner to serverwindow ([0x%lx]).\n", __func__,
              serverWindow);
  #endif

  int index = nxagentFindCurrentSelectionIndex(pSelection->selection);
  if (index != -1)
  {
    #ifdef DEBUG
    fprintf(stderr, "%s: lastSelectionOwner.client %s -> %s\n", __func__,
                nxagentClientInfoString(lastSelectionOwner[index].client),
                    nxagentClientInfoString(pSelection->client));
    fprintf(stderr, "%s: lastSelectionOwner.window [0x%x] -> [0x%x]\n", __func__,
                lastSelectionOwner[index].window, pSelection->window);
    fprintf(stderr, "%s: lastSelectionOwner.windowPtr [%p] -> [%p] [0x%x] (serverWindow: [0x%lx])\n", __func__,
                (void *)lastSelectionOwner[index].windowPtr, (void *)pSelection->pWin,
                    nxagentWindow(pSelection->pWin), serverWindow);
    fprintf(stderr, "%s: lastSelectionOwner.lastTimeChanged [%u]\n", __func__,
                lastSelectionOwner[index].lastTimeChanged);
    #endif

    #if defined(TEST) || defined(DEBUG)
    if (lastServers[index].requestor != None)
    {
      /*
       * There's an X client on the real X server waiting for a
       * reply. That reply will never come because now we are the
       * owner so let's be fair and cancel that request.
       */
      fprintf(stderr, "%s: WARNING! lastServers[%d].requestor window [0x%lx] already set. Cancelling pending request.\n",
                   __func__, index, lastServers[index].requestor);

      replyPendingRequestSelectionToXServer(index, False);

      /* now we can go on */
    }
    #endif

    /*
     * inform the real X server that our serverWindow is the
     * clipboard owner.
     * https://www.freedesktop.org/wiki/ClipboardManager/ states
     * "In order to support peers who use the XFIXES extension to
     * watch clipboard ownership changes, clipboard owners should
     * reacquire the clipboard whenever the content or metadata (e.g
     * the list of supported targets) changes."
     */
    XSetSelectionOwner(nxagentDisplay, remSelAtoms[index], serverWindow, CurrentTime);

    /*
     * The real owner window (inside nxagent) is stored in
     * lastSelectionOwner[index].window.
     * lastSelectionOwner[index].windowPtr points to the struct that
     * contains all information about the owner window.
     */
    storeSelectionOwnerData(index, pSelection);

    setClientSelectionStage(index, SelectionStageNone);

    /*
     * this will be repeated on reception of the SelectionOwner callback
     * but we cannot be sure if there are any intermediate requests in the queue
     * already so better do it here, too
     */
    invalidateTargetCache(index);

    /* FIXME: commented because index is invalid here! */
    /* lastServers[index].requestor = None; */

/*
FIXME

FIXME2: instead of XGetSelectionOwner we could check if the Xfixes
        SetSelectionOwner event has arrived in the event queue;
        possibly saving one roundtrip.

   if (XGetSelectionOwner(nxagentDisplay, pSelection->selection) == serverWindow)
   {
      fprintf (stderr, "%s: SetSelectionOwner OK\n", __func__);

      lastSelectionOwnerSelection = pSelection;
      lastSelectionOwnerClient = pSelection->client;
      lastSelectionOwnerWindow = pSelection->window;
      lastSelectionOwnerWindowPtr = pSelection->pWin;

      setClientSelectionStage(index, SelectionStageNone);

      lastServers[index].requestor = None;
   }
   else fprintf (stderr, "%s: SetSelectionOwner failed\n", __func__);
*/
  }
}

/*
 * This is called from dix (ProcConvertSelection) if an nxagent client
 * issues a ConvertSelection request. So all the Atoms are internal.
 * return codes:
 * 0: let dix process the request
 * 1: don't let dix process the request
 */
int nxagentConvertSelection(ClientPtr client, WindowPtr pWin, Atom selection,
                                Window requestor, Atom property, Atom target, Time time)
{
  #ifdef DEBUG
  fprintf(stderr, "---------\n%s: client %s requests sel [%s] "
              "on window [0x%x] prop [%d][%s] target [%d][%s] time [%u].\n", __func__,
                  nxagentClientInfoString(client), NameForIntAtom(selection), requestor,
                      property, NameForIntAtom(property),
                          target, NameForIntAtom(target), time);
  #endif

  /* We cannot use NameForIntAtom() here! FIXME: Why not? */
  if (NameForAtom(target) == NULL)
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
  if (index == -1)
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
    #ifdef DEBUG
    fprintf(stderr, "%s: clipboard is owned by internal client - let dix process the request\n", __func__);
    #endif
    return 0;
  }

  /*
   * if lastClients[index].windowPtr is set we are waiting for an answer from
   * the real X server. If that answer takes more than 5 seconds we
   * consider the conversion failed and tell our client about that.
   * The new request that lead us here is then processed.
   */
  #ifdef TEST
  fprintf(stderr, "%s: lastClients[%d].windowPtr [0x%lx].\n", __func__, index, (unsigned long)lastClients[index].windowPtr);
  #endif

  if (lastClients[index].windowPtr != NULL)
  {
    #ifdef TEST
    fprintf(stderr, "%s: lastClients[%d].windowPtr != NULL.\n", __func__, index);
    #endif

    #ifdef DEBUG
    fprintf(stderr, "%s: intSelAtoms[%d] [%d] - selection [%d]\n", __func__, index, intSelAtoms[index], selection);
    #endif

    if ((GetTimeInMillis() - lastClients[index].reqTime) >= CONVERSION_TIMEOUT)
    {
      #ifdef DEBUG
      fprintf(stderr, "%s: timeout expired on previous request - "
                  "notifying failure to waiting client\n", __func__);
      #endif

      /* notify the waiting client of failure */
      endTransfer(index, SELECTION_FAULT);

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
   */

  if (target == clientTARGETS)
  {
    /*
     * In TextClipboard mode answer with a predefined list that was used
     * in previous versions.
     */
    if (nxagentOption(TextClipboard))
    {
      /*
       * The list is aligned with the one in
       * nxagentHandleSelectionRequestFromXServer.
       */
      Atom targets[] = {XA_STRING,
                        clientUTF8_STRING,
#ifdef SUPPORT_TEXT_TARGET
                        clientTEXT,
#endif
#ifdef SUPPORT_COMPOUND_TEXT_TARGET
                        clientCOMPOUND_TEXT,
#endif
                        clientTARGETS,
                        clientTIMESTAMP};
      int numTargets = sizeof(targets) / sizeof(targets[0]);

      #ifdef DEBUG
      fprintf(stderr, "%s: Sending %d available targets:\n", __func__, numTargets);
      for (int i = 0; i < numTargets; i++)
      {
        fprintf(stderr, "%s: %d %s\n", __func__, targets[i], NameForIntAtom(targets[i]));
      }
      #endif

      ChangeWindowProperty(pWin,
                           property,
                           MakeAtom("ATOM", 4, 1),
                           sizeof(Atom)*8,
                           PropModeReplace,
                           numTargets,
                           targets,
                           1);

      sendSelectionNotifyEventToClient(client, time, requestor, selection,
                                           target, property);

      return 1;
    }
    else
    {
      /*
       * Shortcut: Some applications tend to post multiple
       * SelectionRequests. Further it can happen that multiple
       * clients are interested in clipboard content. If we already
       * know the answer and no intermediate SelectionOwner event
       * occured we can answer with the cached list of targets.
       */

      if (targetCache[index].type == FORINT && targetCache[index].forInt)
      {
        Atom *targets = targetCache[index].forInt;
        int numTargets = targetCache[index].numTargets;

        #ifdef DEBUG
        fprintf(stderr, "%s: Sending %d cached targets to internal client:\n", __func__, numTargets);
        for (int i = 0; i < numTargets; i++)
        {
          fprintf(stderr, "%s: %d %s\n", __func__, targets[i], NameForIntAtom(targets[i]));
        }
        #endif

        ChangeWindowProperty(pWin,
                             property,
                             MakeAtom("ATOM", 4, 1),
                             sizeof(Atom)*8,
                             PropModeReplace,
                             numTargets,
                             targets,
                             1);

        sendSelectionNotifyEventToClient(client, time, requestor, selection,
                                             target, property);

        return 1;
      }

      /*
       * do nothing - TARGETS will be handled like any other target
       * and passed on to the owner on the remote side.
       */
    }
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
  else if (target == clientTIMESTAMP)
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
  else if (target == clientMULTIPLE)
  {
    #ifdef DEBUG
    fprintf(stderr, "%s: (currently) unsupported target [MULTIPLE] - denying request.\n", __func__);
    #endif
    sendSelectionNotifyEventToClient(client, time, requestor, selection, target, None);
    return 1;
  }
  else if (target == clientDELETE)
  {
    #ifdef DEBUG
    fprintf(stderr, "%s: (currently) unsupported target [DELETE] - denying request.\n", __func__);
    #endif
    sendSelectionNotifyEventToClient(client, time, requestor, selection, target, None);
    return 1;
  }
  else if (target == clientINSERT_SELECTION)
  {
    #ifdef DEBUG
    fprintf(stderr, "%s: (currently) unsupported target [INSERT_SELECTION] - denying request.\n", __func__);
    #endif
    sendSelectionNotifyEventToClient(client, time, requestor, selection, target, None);
    return 1;
  }
  else if (target == clientINSERT_PROPERTY)
  {
    #ifdef DEBUG
    fprintf(stderr, "%s: (currently) unsupported target [INSERT_PROPERTY] - denying request.\n", __func__);
    #endif
    sendSelectionNotifyEventToClient(client, time, requestor, selection, target, None);
    return 1;
  }
  else if (target == clientSAVE_TARGETS)
  {
    #ifdef DEBUG
    fprintf(stderr, "%s: (currently) unsupported target [SAVE_TARGETS] - denying request.\n", __func__);
    #endif
    sendSelectionNotifyEventToClient(client, time, requestor, selection, target, None);
    return 1;
  }
  else if (target == clientTARGET_SIZES)
  {
    #ifdef DEBUG
    fprintf(stderr, "%s: (currently) unsupported target [TARGET_SIZES] - denying request.\n", __func__);
    #endif
    sendSelectionNotifyEventToClient(client, time, requestor, selection, target, None);
    return 1;
  }

  /* in TextClipboard mode reject all non-text targets */
  if (nxagentOption(TextClipboard))
  {
    if (!isTextTarget(translateLocalToRemoteTarget(target)))
    {
      #ifdef DEBUG
      fprintf(stderr, "%s: denying request for non-text target [%d][%s].\n", __func__,
                  target, NameForIntAtom(target));
      #endif

      sendSelectionNotifyEventToClient(client, time, requestor, selection, target, None);
      return 1;
    }
    /* go on, target is acceptable */
  }

  #ifdef DEBUG
  fprintf(stderr, "%s: target [%d][%s].\n", __func__, target,
              NameForIntAtom(target));
  #endif

  if (!nxagentOption(TextClipboard))
  {
    /* Optimization: if we have a current target cache check if the
     * requested target is supported by the owner. If not we can take
     * a shortcut and deny the request immediately without doing any
     * further communication */
    if (targetCache[index].type == FORINT && targetCache[index].forInt)
    {
      Atom *targets = targetCache[index].forInt;

      #ifdef DEBUG
      fprintf(stderr, "%s: Checking target validity\n", __func__);
      #endif
      Bool match = False;
      for (int i = 0; i < targetCache[index].numTargets; i++)
      {
        if (targets[i] == target)
        {
          match = True;
          break;
        }
      }
      if (!match)
      {
        #ifdef DEBUG
        fprintf(stderr, "%s: target [%d][%s] is not supported by the owner - denying request.\n",
                    __func__, target, NameForIntAtom(target));
        #endif
        sendSelectionNotifyEventToClient(client, time, requestor, selection, target, None);
        return 1;
      }
    }
    else
    {
      /*
       * at this stage we know a client has asked for a selection
       * target without having retrieved the list of supported targets
       * first.
       */
      #ifdef DEBUG
      if (target != clientTARGETS)
      {
         fprintf(stderr, "%s: WARNING: client has not retrieved TARGETS before asking for selection!\n",
                   __func__);
      }
      #endif
    }
  }

  if (lastClients[index].clientPtr == client)
  {
    #ifdef DEBUG
    fprintf(stderr, "%s: same client as previous request\n", __func__);
    #endif

    if (GetTimeInMillis() - lastClients[index].reqTime < ACCUM_TIME)
    {
      /*
       * The same client made consecutive requests of clipboard content
       * with less than 5 seconds time interval between them.
       */
       #ifdef DEBUG
       fprintf(stderr, "%s: Consecutives request from client %s selection [%u] "
                  "elapsed time [%u] clientAccum [%d]\n", __func__,
                      nxagentClientInfoString(client),
                          selection, GetTimeInMillis() - lastClients[index].reqTime,
                              clientAccum);
       #endif

       clientAccum++;
    }
  }
  else
  {
    /*
     * reset clientAccum as now another client requested the clipboard
     * content
     */
    clientAccum = 0;
  }

  setClientSelectionStage(index, SelectionStageNone);

  /*
   * store the original requestor, we need that later after
   * serverTransToAgentProperty contains the desired selection content
   */
  lastClients[index].requestor = requestor;
  lastClients[index].windowPtr = pWin;
  lastClients[index].clientPtr = client;
  lastClients[index].time = time;
  lastClients[index].property = property;
  lastClients[index].target = target;
  /* if the last client request time is more than 5s ago update it. Why? */
  if ((GetTimeInMillis() - lastClients[index].reqTime) >= CONVERSION_TIMEOUT)
    lastClients[index].reqTime = GetTimeInMillis();

  XlibAtom remSelection = translateLocalToRemoteSelection(selection);
  XlibAtom remTarget = translateLocalToRemoteTarget(target);
  XlibAtom remProperty = serverTransToAgentProperty;

  #ifdef DEBUG
  fprintf(stderr, "%s: replacing local by remote property: [%d][%s] -> [%ld][%s]\n",
              __func__, property, NameForIntAtom(property),
                  remProperty, "NX_CUT_BUFFER_SERVER");
  #endif

  #ifdef DEBUG
  fprintf(stderr, "%s: Sending XConvertSelection to real X server: requestor [0x%lx] target [%ld][%s] property [%ld][%s] selection [%ld][%s] time [0][CurrentTime]\n", __func__,
          serverWindow, remTarget, NameForRemAtom(remTarget),
              remProperty, NameForRemAtom(remProperty),
                  remSelection, NameForRemAtom(remSelection));
  #endif

  /*
   * ICCCM: "It is necessary for requestors to delete the property
   * before issuing the request so that the target can later be
   * extended to take parameters without introducing an
   * incompatibility. Also note that the requestor of a selection need
   * not know the client that owns the selection nor the window on
   * which the selection was acquired."
   */

  XDeleteProperty(nxagentDisplay, serverWindow, remProperty);

  /*
   * FIXME: ICCCM states: "Clients should not use CurrentTime for the
   * time argument of a ConvertSelection request. Instead, they should
   * use the timestamp of the event that caused the request to be
   * made."
   */

  UpdateCurrentTime();

  XConvertSelection(nxagentDisplay, remSelection, remTarget, remProperty,
                        serverWindow, CurrentTime);

  NXFlushDisplay(nxagentDisplay, NXFlushLink);

  /* XConvertSelection will always return 1 (check the source!), so no
     need to check the return code. */

  #ifdef DEBUG
  fprintf(stderr, "%s: Sent XConvertSelection\n", __func__);
  #endif

  return 1;
}

/* FIXME: do we still need this special treatment? Can't we just
   call nxagentLocalToRemoteAtom() everywhere? */
XlibAtom translateLocalToRemoteSelection(Atom local)
{
  /*
   * On the real server, the right CLIPBOARD atom is
   * XInternAtom(nxagentDisplay, "CLIPBOARD", 1), which is stored in
   * remSelAtoms[nxagentClipboardSelection]. For
   * PRIMARY there's nothing to map because that is identical on all
   * X servers (defined in Xatom.h). We do it anyway so we do not
   * require a special treatment.
   */

  XlibAtom remote = -1;

  for (int index = 0; index < nxagentMaxSelections; index++)
  {
    if (local == intSelAtoms[index])
    {
      remote = remSelAtoms[index];
      break;
    }
  }

  if (remote == -1)
  {
    remote = nxagentLocalToRemoteAtom(local);
  }

  #ifdef DEBUG
  fprintf(stderr, "%s: mapping local to remote selection: [%d][%s] -> [%ld][%s]\n", __func__,
              local, NameForIntAtom(local), remote, NameForRemAtom(remote));
  #endif

  return remote;
}

/* FIXME: do we still need this special treatment? Can't we just
   call nxagentLocalToRemoteAtom() everywhere? */
XlibAtom translateLocalToRemoteTarget(Atom local)
{
  /*
   * .target must be translated, too, as a client on the real
   * server is requested to fill our property and it needs to know
   * the format.
   */

  XlibAtom remote;

  /*
   * we only convert to either UTF8 or XA_STRING
#ifdef SUPPORT_TEXT_TARGET
   * despite accepting TEXT
#endif
#ifdef SUPPORT_COMPOUND_TEXT_TARGET
   * and COMPOUND_TEXT.
#endif
   */

  if (local == clientUTF8_STRING)
  {
    remote = serverUTF8_STRING;
  }
#ifdef SUPPORT_TEXT_TARGET
  else if (local == clientTEXT)
  {
    remote = serverTEXT;
  }
#endif
#ifdef SUPPORT_COMPOUND_TEXT_TARGET
  else if (local == clientCOMPOUND_TEXT)
  {
    remote = serverCOMPOUND_TEXT;
  }
#endif
  else if (local == clientTARGETS)
  {
    remote = serverTARGETS;
  }
  else
  {
    remote = nxagentLocalToRemoteAtom(local);
  }

  #ifdef DEBUG
  fprintf(stderr, "%s: mapping local to remote target: [%d][%s] -> [%ld][%s]\n", __func__,
              local, NameForIntAtom(local), remote, NameForRemAtom(remote));
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
  fprintf(stderr, "---------\n%s: Received SendNotify from client: property [%d][%s] target [%d][%s] selection [%d][%s] requestor [0x%x] time [%u].\n", __func__,
          event->u.selectionNotify.property, NameForIntAtom(event->u.selectionNotify.property),
          event->u.selectionNotify.target, NameForIntAtom(event->u.selectionNotify.target),
          event->u.selectionNotify.selection, NameForIntAtom(event->u.selectionNotify.selection),
          event->u.selectionNotify.requestor, event->u.selectionNotify.time);
  #endif

  int index = nxagentFindCurrentSelectionIndex(event->u.selectionNotify.selection);
  if (index == -1)
  {
    #ifdef DEBUG
    fprintf(stderr, "%s: unknown selection [%d]\n", __func__,
                event->u.selectionNotify.selection);
    #endif
    return 0;
  }

  #ifdef DEBUG
  fprintf(stderr, "%s: lastServers[index].requestor is [0x%lx].\n", __func__, lastServers[index].requestor);
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
     * if the property is 0 (reporting failure) we can directly
     * answer to the lastServer about the failure!
     */
    if (lastServers[index].requestor != None && event->u.selectionNotify.property == 0)
    {
      replyPendingRequestSelectionToXServer(index, False);
      return 1;
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

      #ifdef DEBUG
      fprintf(stderr, "%s: remote property [%ld][%s].\n", __func__,
                  serverTransFromAgentProperty, NameForRemAtom(serverTransFromAgentProperty));
      #endif
      sendSelectionNotifyEventToXServer(&eventSelection);
      return 1;
    }
  }
}

/*
 * This is called from NXproperty.c to determine if a client sets the
 * property we are waiting for.
 * FIXME: in addition we should check if the client is the one we expect
 */
WindowPtr nxagentGetClipboardWindow(Atom property)
{
  if (serverLastRequestedSelection == -1)
  {
    return NULL;
  }

  int index = nxagentFindLastSelectionOwnerIndex(serverLastRequestedSelection);
  if (index != -1 &&
          property == clientCutProperty &&
              lastSelectionOwner[index].windowPtr != NULL)
  {
    #ifdef DEBUG
    fprintf(stderr, "%s: Returning last [%d] selection owner window [%p] (0x%x).\n", __func__,
            intSelAtoms[index],
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
    /* FIXME: it is probably better to re-use the strings from Atoms.c here */
    clientTARGETS = MakeAtom(szAgentTARGETS, strlen(szAgentTARGETS), True);
#ifdef SUPPORT_TEXT_TARGET
    clientTEXT = MakeAtom(szAgentTEXT, strlen(szAgentTEXT), True);
#endif
#ifdef SUPPORT_COMPOUND_TEXT_TARGET
    clientCOMPOUND_TEXT = MakeAtom(szAgentCOMPOUND_TEXT, strlen(szAgentCOMPOUND_TEXT), True);
#endif
    clientUTF8_STRING = MakeAtom(szAgentUTF8_STRING, strlen(szAgentUTF8_STRING), True);
    clientTIMESTAMP = MakeAtom(szAgentTIMESTAMP, strlen(szAgentTIMESTAMP), True);
    clientINCR = MakeAtom(szAgentINCR, strlen(szAgentINCR), True);
    clientMULTIPLE = MakeAtom(szAgentMULTIPLE, strlen(szAgentMULTIPLE), True);
    clientDELETE = MakeAtom(szAgentDELETE, strlen(szAgentDELETE), True);
    clientINSERT_SELECTION = MakeAtom(szAgentINSERT_SELECTION, strlen(szAgentINSERT_SELECTION), True);
    clientINSERT_PROPERTY = MakeAtom(szAgentINSERT_PROPERTY, strlen(szAgentINSERT_PROPERTY), True);
    clientSAVE_TARGETS = MakeAtom(szAgentSAVE_TARGETS, strlen(szAgentSAVE_TARGETS), True);
    clientTARGET_SIZES = MakeAtom(szAgentTARGET_SIZES, strlen(szAgentTARGET_SIZES), True);

    SAFE_free(lastSelectionOwner);
    lastSelectionOwner = (SelectionOwner *) malloc(nxagentMaxSelections * sizeof(SelectionOwner));

    if (lastSelectionOwner == NULL)
    {
      FatalError("nxagentInitClipboard: Failed to allocate memory for the clipboard selections.\n");
    }
    initSelectionOwnerData(nxagentPrimarySelection);
    initSelectionOwnerData(nxagentClipboardSelection);

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

    SAFE_free(intSelAtoms);
    intSelAtoms = (Atom *) calloc(nxagentMaxSelections, sizeof(Atom));
    if (intSelAtoms == NULL)
    {
      FatalError("nxagentInitClipboard: Failed to allocate memory for the internal selection Atoms array.\n");
    }
    intSelAtoms[nxagentPrimarySelection] = XA_PRIMARY;
    intSelAtoms[nxagentClipboardSelection] = MakeAtom(szAgentCLIPBOARD, strlen(szAgentCLIPBOARD), True);

    SAFE_free(remSelAtoms);
    remSelAtoms = (XlibAtom *) calloc(nxagentMaxSelections, sizeof(XlibAtom));
    if (remSelAtoms == NULL)
    {
      FatalError("nxagentInitClipboard: Failed to allocate memory for the remote selection Atoms array.\n");
    }

    SAFE_free(targetCache);
    targetCache = (Targets *) calloc(nxagentMaxSelections, sizeof(Targets));
    if (targetCache == NULL)
    {
      FatalError("nxagentInitClipboard: Failed to allocate memory for target cache.\n");
    }
  }

  /*
   * the clipboard selection atom might have changed on a new X
   * server. Primary is constant.
   */
  remSelAtoms[nxagentPrimarySelection] = XA_PRIMARY;
  remSelAtoms[nxagentClipboardSelection] = nxagentAtoms[10];   /* CLIPBOARD */

  serverTARGETS = nxagentAtoms[6];  /* TARGETS */
#ifdef SUPPORT_TEXT_TARGET
  serverTEXT = nxagentAtoms[7];  /* TEXT */
#endif
#ifdef SUPPORT_COMPOUND_TEXT_TARGET
  serverCOMPOUND_TEXT = nxagentAtoms[16]; /* COMPOUND_TEXT */
#endif
  serverUTF8_STRING = nxagentAtoms[12]; /* UTF8_STRING */
  serverTIMESTAMP = nxagentAtoms[11];   /* TIMESTAMP */
  serverINCR = nxagentAtoms[17];   /* INCR */
  serverMULTIPLE = nxagentAtoms[18];   /* MULTIPLE */
  serverDELETE = nxagentAtoms[19];   /* DELETE */
  serverINSERT_SELECTION = nxagentAtoms[20];   /* INSERT_SELECTION */
  serverINSERT_PROPERTY = nxagentAtoms[21];   /* INSERT_PROPERTY */
  serverSAVE_TARGETS = nxagentAtoms[22];   /* SAVE_TARGETS */
  serverTARGET_SIZES = nxagentAtoms[23];   /* TARGET_SIZES */

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

  /* this is probably to communicate with nomachine nxclient */
  #ifdef TEST
  fprintf(stderr, "%s: Setting owner of selection [%d][%s] to serverWindow [0x%lx]\n", __func__,
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
                                 remSelAtoms[index],
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
                    validateString(NameForRemAtom(nxagentAtoms[10])), serverWindow);
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
          /* remSelAtoms have already been adjusted above */
          XSetSelectionOwner(nxagentDisplay, remSelAtoms[index], serverWindow, CurrentTime);
        }

        /*
         * On reconnect there cannot be any external requestor
         * waiting for a reply so clean this
         */
        lastServers[index].requestor = None;

        /*
         * FIXME: We should reset lastClients[index].* here! Problem
         * is that internal clients might still be waiting for
         * answers. Should reply with failure then.
         */

        invalidateTargetCache(index);
      }
    }
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
      invalidateTargetCache(index);
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
