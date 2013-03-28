/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2011 NoMachine, http://www.nomachine.com/.         */
/*                                                                        */
/* NXAGENT, NX protocol compression and NX extensions to this software    */
/* are copyright of NoMachine. Redistribution and use of the present      */
/* software is allowed according to terms specified in the file LICENSE   */
/* which comes in the source distribution.                                */
/*                                                                        */
/* Check http://www.nomachine.com/licensing.html for applicability.       */
/*                                                                        */
/* NX and NoMachine are trademarks of Medialogic S.p.A.                   */
/*                                                                        */
/* All rights reserved.                                                   */
/*                                                                        */
/**************************************************************************/

#define NEED_EVENTS

#include "X.h"
#include "Xproto.h"
#include "Xatom.h"
#include "selection.h"
#include "windowstr.h"

#include "Windows.h"
#include "Atoms.h"
#include "Agent.h"
#include "Args.h"
#include "Trap.h"
#include "Rootless.h"
#include "Clipboard.h"

#include "gcstruct.h"
#include "xfixeswire.h"
#include <X11/extensions/Xfixes.h>

/*
 * Use asyncronous get property replies.
 */

#include "NXlib.h"

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

static int agentClipboardStatus;
static int clientAccum;

Atom serverCutProperty;
Atom clientCutProperty;
static Window serverWindow;

static const int nxagentPrimarySelection = 0;
static const int nxagentClipboardSelection = 1;
static const int nxagentMaxSelections = 2;

typedef struct _SelectionOwner
{
    Atom selection;
    ClientPtr client;
    Window window;
    WindowPtr windowPtr;
    Time lastTimeChanged;

} SelectionOwner;

static SelectionOwner *lastSelectionOwner;
static Atom nxagentLastRequestedSelection;
static Atom nxagentClipboardAtom;
static Atom nxagentTimestampAtom;

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

static Window lastServerRequestor;
static Atom   lastServerProperty;
static Atom   lastServerTarget;
static Time   lastServerTime;

static Atom serverTARGETS;
static Atom serverTEXT;
static Atom serverUTF8_STRING;
static Atom clientTARGETS;
static Atom clientTEXT;
static Atom clientCOMPOUND_TEXT;
static Atom clientUTF8_STRING;

static char szAgentTARGETS[] = "TARGETS";
static char szAgentTEXT[] = "TEXT";
static char szAgentCOMPOUND_TEXT[] = "COMPOUND_TEXT";
static char szAgentUTF8_STRING[] = "UTF8_STRING";
static char szAgentNX_CUT_BUFFER_CLIENT[] = "NX_CUT_BUFFER_CLIENT";

/*
 * Save the values queried from X server.
 */

XFixesAgentInfoRec nxagentXFixesInfo = { -1, -1, -1, 0 };

extern Display *nxagentDisplay;

Bool nxagentValidServerTargets(Atom target);
void nxagentSendSelectionNotify(Atom property);
void nxagentTransferSelection(int resource);
void nxagentCollectPropertyEvent(int resource);
void nxagentResetSelectionOwner(void);
WindowPtr nxagentGetClipboardWindow(Atom property, WindowPtr pWin);
void nxagentNotifyConvertFailure(ClientPtr client, Window requestor,
                                     Atom selection, Atom target, Time time);
int nxagentSendNotify(xEvent *event);

/*
 * This is from NXproperty.c.
 */

int GetWindowProperty(WindowPtr pWin, Atom property, long longOffset, long longLength,
                          Bool delete, Atom type, Atom *actualType, int *format,
                              unsigned long *nItems, unsigned long *bytesAfter,
                                  unsigned char **propData);

Bool nxagentValidServerTargets(Atom target)
{
  #ifdef DEBUG
  fprintf(stderr, "nxagentValidServerTargets: Got called.\n");
  #endif

  if (target == XA_STRING) return True;
  if (target == serverTEXT) return True;
  /* by dimbor */
  if (target == serverUTF8_STRING) return True;
    
  return False;
}

void nxagentClearClipboard(ClientPtr pClient, WindowPtr pWindow)
{
  int i;

  #ifdef DEBUG
  fprintf(stderr, "nxagentClearClipboard: Called with client [%p] window [%p].\n",
              (void *) pClient, (void *) pWindow);
  #endif

  /*
   * Only for PRIMARY and CLIPBOARD selections.
   */

  for (i = 0; i < nxagentMaxSelections; i++)
  {
    if ((pClient != NULL && lastSelectionOwner[i].client == pClient) ||
            (pWindow != NULL && lastSelectionOwner[i].windowPtr == pWindow))
    {
      #ifdef TEST
      fprintf(stderr, "nxagentClearClipboard: Resetting state with client [%p] window [%p].\n",
                  (void *) pClient, (void *) pWindow);
      #endif

      lastSelectionOwner[i].client = NULL;
      lastSelectionOwner[i].window = None;
      lastSelectionOwner[i].windowPtr = NULL;
      lastSelectionOwner[i].lastTimeChanged = GetTimeInMillis();

      lastClientWindowPtr = NULL;
      lastClientStage = SelectionStageNone;

      lastServerRequestor = None;
    }
  }
 
  if (pWindow == lastClientWindowPtr)
  {
    lastClientWindowPtr = NULL;
    lastClientStage = SelectionStageNone;
  }

}

void nxagentClearSelection(XEvent *X)
{
  xEvent x;

  int i = 0;

  #ifdef DEBUG
  fprintf(stderr, "nxagentClearSelection: Got called.\n");
  #endif

  if (agentClipboardStatus != 1 ||
          nxagentOption(Clipboard) == ClipboardServer)
  {
    return;
  }

  #ifdef DEBUG
  fprintf(stderr, "nxagentClearSelection: SelectionClear event.\n");
  #endif

  while ((i < nxagentMaxSelections) &&
            (lastSelectionOwner[i].selection != X->xselectionclear.selection))
  {
    i++;
  }

  if (i < nxagentMaxSelections)
  {
    if (lastSelectionOwner[i].client != NULL)
    {
      x.u.u.type = SelectionClear;
      x.u.selectionClear.time = GetTimeInMillis();
      x.u.selectionClear.window = lastSelectionOwner[i].window;
      x.u.selectionClear.atom = CurrentSelections[i].selection;

      (void) TryClientEvents(lastSelectionOwner[i].client, &x, 1,
                             NoEventMask, NoEventMask,
                             NullGrab);
    }

    CurrentSelections[i].window = WindowTable[0]->drawable.id;
    CurrentSelections[i].client = NullClient;

    lastSelectionOwner[i].client = NULL;
    lastSelectionOwner[i].window = None;
    lastSelectionOwner[i].lastTimeChanged = GetTimeInMillis();
  }

  lastClientWindowPtr = NULL;
  lastClientStage = SelectionStageNone;
}

void nxagentRequestSelection(XEvent *X)
{
  int result;
  int i = 0;
  XSelectionEvent eventSelection;

  #ifdef DEBUG
  fprintf(stderr, "nxagentRequestSelection: Got called.\n");
  #endif

  if (agentClipboardStatus != 1)
  {
    return;
  }

  if (!nxagentValidServerTargets(X->xselectionrequest.target) ||
         (lastServerRequestor != None) ||
             ((X->xselectionrequest.selection != lastSelectionOwner[nxagentPrimarySelection].selection) &&
                 (X->xselectionrequest.selection != lastSelectionOwner[nxagentClipboardSelection].selection)))
  {
/*
FIXME: Do we need this?

    char *strTarget;

    strTarget = XGetAtomName(nxagentDisplay, X->xselectionrequest.target);

    fprintf(stderr, "SelectionRequest event aborting sele=[%s] target=[%s]\n",
                validateString(NameForAtom(X->xselectionrequest.selection)),
                    validateString(NameForAtom(X->xselectionrequest.target)));

    fprintf(stderr, "SelectionRequest event aborting sele=[%s] ext target=[%s] Atom size is [%d]\n",
                validateString(NameForAtom(X->xselectionrequest.selection)), strTarget, sizeof(Atom));

    if (strTarget != NULL)
    {
      XFree(strTarget);
    }
*/
    eventSelection.property = None;

    if (X->xselectionrequest.target == serverTARGETS)
    {
      Atom xa_STRING = XA_STRING;
      result = XChangeProperty (nxagentDisplay,
                                X->xselectionrequest.requestor,
                                X->xselectionrequest.property,
                                XInternAtom(nxagentDisplay, "ATOM", 0),
                                sizeof(Atom)*8,
                                PropModeReplace,
                                (unsigned char*)&xa_STRING,
                                1);
      eventSelection.property = X->xselectionrequest.property;
    }

    if (X->xselectionrequest.target == nxagentTimestampAtom)
    {
      while ((i < NumCurrentSelections) &&
                lastSelectionOwner[i].selection != X->xselectionrequest.selection) i++;

      if (i < NumCurrentSelections)
      {
        result = XChangeProperty(nxagentDisplay,
                                 X->xselectionrequest.requestor,
                                 X->xselectionrequest.property,
                                 X->xselectionrequest.target,
                                 32,
                                 PropModeReplace,
                                 (unsigned char *) &lastSelectionOwner[i].lastTimeChanged,
                                 1);
        eventSelection.property = X->xselectionrequest.property;
      }
    }

    eventSelection.type = SelectionNotify;
    eventSelection.send_event = True;
    eventSelection.display = nxagentDisplay;
    eventSelection.requestor = X->xselectionrequest.requestor;
    eventSelection.selection = X->xselectionrequest.selection;
    eventSelection.target = X->xselectionrequest.target;
    eventSelection.time = X->xselectionrequest.time;

    result = XSendEvent(nxagentDisplay,
                        eventSelection.requestor,
                        False,
                        0L,
                        (XEvent *) &eventSelection);

    #ifdef DEBUG
    if (result == BadValue || result == BadWindow)
    {
      fprintf(stderr, "nxagentRequestSelection: WARNING! XSendEvent failed.\n");
    }
    else
    {
      fprintf(stderr, "nxagentRequestSelection: XSendEvent sent to window [0x%lx].\n",
                  eventSelection.requestor);
    }
    #endif

    return;
  }

  /*
   * This is necessary in nxagentGetClipboardWindow.
   */

  nxagentLastRequestedSelection = X->xselectionrequest.selection;

  while ((i < nxagentMaxSelections) &&
            (lastSelectionOwner[i].selection != X->xselectionrequest.selection))
  {
    i++;
  }

  if (i < nxagentMaxSelections)
  {
    if ((lastClientWindowPtr != NULL) && (lastSelectionOwner[i].client != NULL))
    {
      XConvertSelection(nxagentDisplay, CurrentSelections[i].selection,
                            X->xselectionrequest.target, serverCutProperty,
                                serverWindow, lastClientTime);

      #ifdef DEBUG
      fprintf(stderr, "nxagentRequestSelection: Sent XConvertSelection.\n");
      #endif
    }
    else
    {
      if (lastSelectionOwner[i].client != NULL && 
             nxagentOption(Clipboard) != ClipboardClient)
      {
        xEvent x;

        lastServerProperty = X->xselectionrequest.property;
        lastServerRequestor = X->xselectionrequest.requestor;
        lastServerTarget = X->xselectionrequest.target;

        /* by dimbor */
        if (lastServerTarget != XA_STRING)
	  lastServerTarget = serverUTF8_STRING;
			  
	lastServerTime = X->xselectionrequest.time;

        x.u.u.type = SelectionRequest;
        x.u.selectionRequest.time = GetTimeInMillis();
        x.u.selectionRequest.owner = lastSelectionOwner[i].window;

        /*
         * Fictitious window.
         */

        x.u.selectionRequest.requestor = WindowTable[0]->drawable.id;

        /*
         * Don't send the same window, some programs are
         * clever and verify cut and paste operations
         * inside the same window and don't Notify at all.
         *
         * x.u.selectionRequest.requestor = lastSelectionOwnerWindow;
         */

        x.u.selectionRequest.selection = CurrentSelections[i].selection;

        /* by dimbor (idea from zahvatov) */
        if (X->xselectionrequest.target != XA_STRING)
          x.u.selectionRequest.target = clientUTF8_STRING;
        else
          x.u.selectionRequest.target = XA_STRING;
					    
        x.u.selectionRequest.property = clientCutProperty;

        (void) TryClientEvents(lastSelectionOwner[i].client, &x, 1,
                               NoEventMask, NoEventMask /* CantBeFiltered */,
                               NullGrab);

        #ifdef DEBUG
        fprintf(stderr, "nxagentRequestSelection: Executed TryClientEvents with clientCutProperty.\n");
        #endif
      }
      else
      {
        /*
         * Probably we must to send a Notify
         * to requestor with property None.
         */

        eventSelection.type = SelectionNotify;
        eventSelection.send_event = True;
        eventSelection.display = nxagentDisplay;
        eventSelection.requestor = X->xselectionrequest.requestor;
        eventSelection.selection = X->xselectionrequest.selection;
        eventSelection.target = X->xselectionrequest.target;
        eventSelection.property = None;
        eventSelection.time = X->xselectionrequest.time;

        result = XSendEvent(nxagentDisplay,
                            eventSelection.requestor,
                            False,
                            0L,
                            (XEvent *) &eventSelection);

        #ifdef DEBUG
        fprintf(stderr, "nxagentRequestSelection: Executed XSendEvent with property None.\n");
        #endif
      }
    }
  }
}

void nxagentSendSelectionNotify(Atom property)
{
  xEvent x;

  #ifdef DEBUG
  fprintf (stderr, "nxagentSendSelectionNotify: Sending event to client [%d].\n",
               lastClientClientPtr -> index);
  #endif

  x.u.u.type = SelectionNotify;

  x.u.selectionNotify.time = lastClientTime;
  x.u.selectionNotify.requestor = lastClientRequestor;
  x.u.selectionNotify.selection = lastClientSelection;
  x.u.selectionNotify.target = lastClientTarget;

  x.u.selectionNotify.property = property;

  TryClientEvents(lastClientClientPtr, &x, 1, NoEventMask,
                      NoEventMask , NullGrab);

  return;
}

void nxagentTransferSelection(int resource)
{
  int result;

  if (lastClientClientPtr -> index != resource)
  {
    #ifdef DEBUG
    fprintf (stderr, "nxagentTransferSelection: WARNING! Inconsistent resource [%d] with current client [%d].\n",
                 resource, lastClientClientPtr -> index);
    #endif

    nxagentSendSelectionNotify(None);

    lastClientWindowPtr = NULL;
    lastClientStage = SelectionStageNone;

    return;
  }

  switch (lastClientStage)
  {
    case SelectionStageQuerySize:
    {
      /*
       * Don't get data yet, just get size. We skip
       * this stage in current implementation and
       * go straight to the data.
       */

      nxagentLastClipboardClient = NXGetCollectPropertyResource(nxagentDisplay);

      if (nxagentLastClipboardClient == -1)
      {
        #ifdef WARNING
        fprintf(stderr, "nxagentTransferSelection: WARNING! Asyncronous GetProperty queue full.\n");
        #endif

        result = -1;
      }
      else
      {
        result = NXCollectProperty(nxagentDisplay,
                                   nxagentLastClipboardClient,
                                   serverWindow,
                                   serverCutProperty,
                                   0,
                                   0,
                                   False,
                                   AnyPropertyType);
      }

      if (result == -1)
      {
        #ifdef DEBUG
        fprintf (stderr, "nxagentTransferSelection: Aborting selection notify procedure for client [%d].\n",
                     lastClientClientPtr -> index);
        #endif

        nxagentSendSelectionNotify(None);

        lastClientWindowPtr = NULL;
        lastClientStage = SelectionStageNone;

        return;
      }

      #ifdef DEBUG
      fprintf (stderr, "nxagentTransferSelection: Setting stage to [%d] for client [%d].\n",
                   SelectionStageWaitSize, lastClientClientPtr -> index);
      #endif

      lastClientStage = SelectionStageWaitSize;

      break;
    }
    case SelectionStageQueryData:
    {
      /*
       * Request the selection data now.
       */

      #ifdef DEBUG
      fprintf(stderr, "nxagentTransferSelection: Getting property content from remote server.\n");
      #endif

      nxagentLastClipboardClient = NXGetCollectPropertyResource(nxagentDisplay);

      if (nxagentLastClipboardClient == -1)
      {
        #ifdef WARNING
        fprintf(stderr, "nxagentTransferSelection: WARNING! Asyncronous GetProperty queue full.\n");
        #endif

        result = -1;
      }
      else
      {
        result = NXCollectProperty(nxagentDisplay,
                                   nxagentLastClipboardClient,
                                   serverWindow,
                                   serverCutProperty,
                                   0,
                                   lastClientPropertySize,
                                   False,
                                   AnyPropertyType);
      }

      if (result == -1)
      {
        #ifdef DEBUG
        fprintf (stderr, "nxagentTransferSelection: Aborting selection notify procedure for client [%d].\n",
                     lastClientClientPtr -> index);
        #endif

        nxagentSendSelectionNotify(None);

        lastClientWindowPtr = NULL;
        lastClientStage = SelectionStageNone;

        return;
      }

      #ifdef DEBUG
      fprintf (stderr, "nxagentTransferSelection: Setting stage to [%d] for client [%d].\n",
                   SelectionStageWaitData, lastClientClientPtr -> index);
      #endif

      lastClientStage = SelectionStageWaitData;

      break;
    }
    default:
    {
      #ifdef DEBUG
      fprintf (stderr, "nxagentTransferSelection: WARNING! Inconsistent state [%d] for client [%d].\n",
                   lastClientStage, lastClientClientPtr -> index);
      #endif

      break;
    }
  }
}

void nxagentCollectPropertyEvent(int resource)
{
  Atom                  atomReturnType;
  int                   resultFormat;
  unsigned long         ulReturnItems;
  unsigned long         ulReturnBytesLeft;
  unsigned char         *pszReturnData = NULL;
  int                   result;

  /*
   * We have received the notification so
   * we can safely retrieve data from the
   * client structure.
   */

  result = NXGetCollectedProperty(nxagentDisplay,
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
    fprintf (stderr, "nxagentCollectPropertyEvent: Failed to get reply data for client [%d].\n",
                 lastClientClientPtr -> index);
    #endif

    nxagentSendSelectionNotify(None);

    lastClientWindowPtr = NULL;
    lastClientStage = SelectionStageNone;

    if (pszReturnData != NULL)
    {
      XFree(pszReturnData);
    }

    return;
  }
 
  if (resultFormat != 8 && resultFormat != 16 && resultFormat != 32)
  {

    #ifdef DEBUG
    fprintf (stderr, "nxagentCollectPropertyEvent: WARNING! Invalid property "
                 "value.\n");
    #endif

    if (lastClientClientPtr != NULL)
    {
      nxagentSendSelectionNotify(None);
    }

    lastClientWindowPtr = NULL;
    lastClientStage = SelectionStageNone;

    if (pszReturnData != NULL)
    {
      XFree(pszReturnData);
    }

    return;
  }

  switch (lastClientStage)
  {
    case SelectionStageWaitSize:
    {
      #ifdef DEBUG
      fprintf (stderr, "nxagentCollectPropertyEvent: Got size notify event for client [%d].\n",
                   lastClientClientPtr -> index);
      #endif

      if (ulReturnBytesLeft == 0)
      {
        #ifdef DEBUG
        fprintf (stderr, "nxagentCollectPropertyEvent: Aborting selection notify procedure for client [%d].\n",
                     lastClientClientPtr -> index);
        #endif

        nxagentSendSelectionNotify(None);

        lastClientWindowPtr = NULL;
        lastClientStage = SelectionStageNone;

        if (pszReturnData != NULL)
        {
          Xfree(pszReturnData);
        }

        return;
      }

      #ifdef DEBUG
      fprintf(stderr, "nxagentCollectPropertyEvent: Got property size from remote server.\n");
      #endif

      /*
       * Request the selection data now.
       */

      lastClientPropertySize = ulReturnBytesLeft;
      lastClientStage = SelectionStageQueryData;

      nxagentTransferSelection(resource);

      break;
    }
    case SelectionStageWaitData:
    {
      #ifdef DEBUG
      fprintf (stderr, "nxagentCollectPropertyEvent: Got data notify event for client [%d].\n",
                   lastClientClientPtr -> index);
      #endif

      if (ulReturnBytesLeft != 0)
      {
        #ifdef DEBUG
        fprintf (stderr, "nxagentCollectPropertyEvent: Aborting selection notify procedure for client [%d].\n",
                     lastClientClientPtr -> index);
        #endif

        nxagentSendSelectionNotify(None);

        lastClientWindowPtr = NULL;
        lastClientStage = SelectionStageNone;

        if (pszReturnData != NULL)
        {
          Xfree(pszReturnData);
        }

        return;
      }

      #ifdef DEBUG
      fprintf(stderr, "nxagentCollectPropertyEvent: Got property content from remote server.\n");
      #endif

      ChangeWindowProperty(lastClientWindowPtr,
                           lastClientProperty,
                           lastClientTarget,
                           resultFormat, PropModeReplace,
                           ulReturnItems, pszReturnData, 1);

      #ifdef DEBUG
      fprintf(stderr, "nxagentCollectPropertyEvent: Selection property [%s] changed to [%s]\n",
                   validateString(NameForAtom(lastClientProperty)), pszReturnData);
      #endif

      nxagentSendSelectionNotify(lastClientProperty);

      /*
       * Enable further requests from clients.
       */

      lastClientWindowPtr = NULL;
      lastClientStage = SelectionStageNone;

      break;
    }
    default:
    {
      #ifdef DEBUG
      fprintf (stderr, "nxagentCollectPropertyEvent: WARNING! Inconsistent state [%d] for client [%d].\n",
                   lastClientStage, lastClientClientPtr -> index);
      #endif

      break;
    }
  }

  XFree(pszReturnData);
  pszReturnData = NULL;
}

void nxagentNotifySelection(XEvent *X)
{
  int result;

  XSelectionEvent eventSelection;

  #ifdef DEBUG
  fprintf(stderr, "nxagentNotifySelection: Got called.\n");
  #endif

  if (agentClipboardStatus != 1)
  {
    return;
  }

  #ifdef DEBUG
  fprintf(stderr, "nxagentNotifySelection: SelectionNotify event.\n");
  #endif

  if (lastClientWindowPtr != NULL)
  {
    if ((lastClientStage == SelectionStageNone) && (X->xselection.property == serverCutProperty))
    {
      #ifdef DEBUG
      fprintf(stderr, "nxagentNotifySelection: Starting selection transferral for client [%d].\n",
                  lastClientClientPtr -> index);
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

      lastClientStage = SelectionStageQueryData;
      lastClientPropertySize = 262144;

      nxagentTransferSelection(lastClientClientPtr -> index);
    }
    else
    {
      #ifdef DEBUG
      fprintf(stderr, "nxagentNotifySelection: WARNING! Resetting selection transferral for client [%d].\n",
                  lastClientClientPtr -> index);
      #endif

      nxagentSendSelectionNotify(None);

      lastClientWindowPtr = NULL;
      lastClientStage = SelectionStageNone;
    }

    return;
  }
  else
  {
    int i = 0;

    while ((i < nxagentMaxSelections) && (lastSelectionOwner[i].selection != X->xselection.selection))
    {
      i++;
    }

    if (i < nxagentMaxSelections)
    {
      if ((lastSelectionOwner[i].client != NULL) &&
             (lastSelectionOwner[i].windowPtr != NULL) &&
                 (X->xselection.property == clientCutProperty))
      {
        Atom            atomReturnType;
        int             resultFormat;
        unsigned long   ulReturnItems;
        unsigned long   ulReturnBytesLeft;
        unsigned char   *pszReturnData = NULL;

        result = GetWindowProperty(lastSelectionOwner[i].windowPtr, clientCutProperty, 0, 0, False,
                                       AnyPropertyType, &atomReturnType, &resultFormat,
                                           &ulReturnItems, &ulReturnBytesLeft, &pszReturnData);

        if (result == BadAlloc || result == BadAtom ||
                result == BadMatch || result == BadValue ||
                    result == BadWindow)
        {
          fprintf (stderr, "Client GetProperty failed error =");
          lastServerProperty = None;
          switch (result)
          {
            case BadAtom:
                 fprintf (stderr, "BadAtom\n");
                 break;
            case BadValue:
                 fprintf (stderr, "BadValue\n");
                 break;
            case BadWindow:
                 fprintf (stderr, "BadWindow\n");
                 break;
          }
        }
        else
        {
          result = GetWindowProperty(lastSelectionOwner[i].windowPtr, clientCutProperty, 0,
                                         ulReturnBytesLeft, False, AnyPropertyType, &atomReturnType,
                                             &resultFormat, &ulReturnItems, &ulReturnBytesLeft,
                                                 &pszReturnData);

          if (result == BadAlloc || result == BadAtom ||
                  result == BadMatch || result == BadValue ||
                      result == BadWindow)
          {
            fprintf (stderr, "SelectionNotify - XChangeProperty failed\n");

            lastServerProperty = None;
          }
          else
          {
            result = XChangeProperty(nxagentDisplay,
                                     lastServerRequestor,
                                     lastServerProperty,
                                     lastServerTarget,
                                     8,
                                     PropModeReplace,
                                     pszReturnData,
                                     ulReturnItems);
          }

          /*
           * if (pszReturnData)
           * {
           *   free(pszReturnData);
           *   pszReturnData=NULL;
           * }
           */

        }

        eventSelection.type = SelectionNotify;
        eventSelection.send_event = True;
        eventSelection.display = nxagentDisplay;
        eventSelection.requestor = lastServerRequestor;

        eventSelection.selection = X->xselection.selection;

        /*
         * eventSelection.target = X->xselection.target;
         */

        eventSelection.target = lastServerTarget;
        eventSelection.property = lastServerProperty;
        eventSelection.time = lastServerTime;

        /*
         * eventSelection.time = CurrentTime;
         * eventSelection.time = lastServerTime;
         */

        #ifdef DEBUG
        fprintf(stderr, "nxagentNotifySelection: Sending event to requestor.\n");
        #endif

        result = XSendEvent(nxagentDisplay,
                             eventSelection.requestor,
                             False,
                             0L,
                             (XEvent *) &eventSelection);

        if (result == BadValue || result == BadWindow)
        {
          fprintf (stderr, "SelectionRequest - XSendEvent failed\n");
        }

        lastServerRequestor = None; /* allow further request */
      }
    }
  }
}

/*
 * Acquire selection so we don't get selection
 * requests from real X clients.
 */

void nxagentResetSelectionOwner()
{
  int i;

  if (lastServerRequestor != None)
  {
    #ifdef TEST
    fprintf (stderr, "nxagentResetSelectionOwner: WARNING! Requestor window [0x%lx] already found.\n",
                 lastServerRequestor);
    #endif

    return;
  }

  /*
   * Only for PRIMARY and CLIPBOARD selections.
   */

  for (i = 0; i < nxagentMaxSelections; i++)
  {
    XSetSelectionOwner(nxagentDisplay, lastSelectionOwner[i].selection, serverWindow, CurrentTime);

    fprintf (stderr, "nxagentResetSelectionOwner: Reset clipboard state.\n");

    lastSelectionOwner[i].client = NULL;
    lastSelectionOwner[i].window = None;
    lastSelectionOwner[i].windowPtr = NULL;
    lastSelectionOwner[i].lastTimeChanged = GetTimeInMillis();
  }

  lastClientWindowPtr = NULL;
  lastClientStage = SelectionStageNone;

  lastServerRequestor = None;

  return;
}

void nxagentSetSelectionOwner(Selection *pSelection)
{
  int i;
  #ifdef DEBUG
  fprintf(stderr, "nxagentSetSelectionOwner: Got called.\n");
  #endif

  if (agentClipboardStatus != 1)
  {
    return;
  }

  #ifdef DEBUG
  fprintf(stderr, "nxagentSetSelectionOwner: Setting selection owner to window [0x%lx].\n",
              serverWindow);
  #endif

  #ifdef TEST
  if (lastServerRequestor != None)
  {
    fprintf (stderr, "nxagentSetSelectionOwner: WARNING! Requestor window [0x%lx] already found.\n",
                 lastServerRequestor);
  }
  #endif

  /*
   * Only for PRIMARY and CLIPBOARD selections.
   */

  for (i = 0; i < nxagentMaxSelections; i++)
  {
    if (pSelection->selection == CurrentSelections[i].selection)
    {
      XSetSelectionOwner(nxagentDisplay, lastSelectionOwner[i].selection, serverWindow, CurrentTime);

      lastSelectionOwner[i].client = pSelection->client;
      lastSelectionOwner[i].window = pSelection->window;
      lastSelectionOwner[i].windowPtr = pSelection->pWin;
      lastSelectionOwner[i].lastTimeChanged = GetTimeInMillis();
    }
  }

  lastClientWindowPtr = NULL;
  lastClientStage = SelectionStageNone;

  lastServerRequestor = None;

/*
FIXME

   if (XGetSelectionOwner(nxagentDisplay,pSelection->selection)==serverWindow)
   {
      fprintf (stderr, "NXdispatch: SetSelectionOwner OK\n");

      lastSelectionOwnerSelection = pSelection;
      lastSelectionOwnerClient = pSelection->client;
      lastSelectionOwnerWindow = pSelection->window;
      lastSelectionOwnerWindowPtr = pSelection->pWin;

      lastClientWindowPtr = NULL;
      lastClientStage     = SelectionStageNone;

      lastServerRequestor = None;
   }
   else fprintf (stderr, "nxagentSetSelectionOwner: SetSelectionOwner failed\n");
*/
}

void nxagentNotifyConvertFailure(ClientPtr client, Window requestor,
                                     Atom selection, Atom target, Time time)
{
  xEvent x;

/*
FIXME: Why this pointer can be not a valid
       client pointer?
*/
  if (clients[client -> index] != client)
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentNotifyConvertFailure: WARNING! Invalid client pointer.");
    #endif

    return;
  }

  x.u.u.type = SelectionNotify;
  x.u.selectionNotify.time = time;
  x.u.selectionNotify.requestor = requestor;
  x.u.selectionNotify.selection = selection;
  x.u.selectionNotify.target = target;
  x.u.selectionNotify.property = None;

  (void) TryClientEvents(client, &x, 1, NoEventMask,
                           NoEventMask , NullGrab);
}

int nxagentConvertSelection(ClientPtr client, WindowPtr pWin, Atom selection,
                                Window requestor, Atom property, Atom target, Time time)
{
  char *strTarget;
  int i;

  if (agentClipboardStatus != 1 ||
           nxagentOption(Clipboard) == ClipboardServer)
  {
    return 0;
  }

  /*
   * There is a client owner on the agent side, let normal stuff happen.
   */

  /*
   * Only for PRIMARY and CLIPBOARD selections.
   */

  for (i = 0; i < nxagentMaxSelections; i++)
  {
    if ((selection == CurrentSelections[i].selection) &&
           (lastSelectionOwner[i].client != NULL))
    {
      return 0;
    }
  }

  if (lastClientWindowPtr != NULL)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentConvertSelection: lastClientWindowPtr != NULL.\n");
    #endif

    if ((GetTimeInMillis() - lastClientReqTime) > 5000)
    {
      #ifdef DEBUG
      fprintf(stderr, "nxagentConvertSelection: timeout expired on last request, "
                        "notifying failure to client\n");
      #endif

      nxagentNotifyConvertFailure(lastClientClientPtr, lastClientRequestor,
                                     lastClientSelection, lastClientTarget, lastClientTime);

      lastClientWindowPtr = NULL;
      lastClientStage = SelectionStageNone;
    }
    else
    {
      #ifdef DEBUG
      fprintf(stderr, "nxagentConvertSelection: got request "
                        "before timeout expired on last request, notifying failure to client\n");
      #endif

      nxagentNotifyConvertFailure(client, requestor, selection, target, time);

      return 1;
    }
  }

  #ifdef TEST
  fprintf(stderr, "nxagentConvertSelection: client [%d] ask for sel [%s] "
              "on window [%lx] prop [%s] target [%s].\n",
                  client -> index, validateString(NameForAtom(selection)), requestor,
                      validateString(NameForAtom(property)), validateString(NameForAtom(target)));
  #endif

  strTarget = NameForAtom(target);

  if (strTarget == NULL)
  {
    return 1;
  }

  if (target == clientTARGETS)
  {
    Atom xa_STRING[4];
    xEvent x;

    /* --- Order changed by dimbor (prevent sending COMPOUND_TEXT to client --- */
    xa_STRING[0] = XA_STRING;
    xa_STRING[1] = clientUTF8_STRING;
    xa_STRING[2] = clientTEXT;
    xa_STRING[3] = clientCOMPOUND_TEXT;

    ChangeWindowProperty(pWin,
                         property,
                         MakeAtom("ATOM", 4, 1),
                         sizeof(Atom)*8,
                         PropModeReplace,
                         4,
                         &xa_STRING, 1);

    x.u.u.type = SelectionNotify;
    x.u.selectionNotify.time = time;
    x.u.selectionNotify.requestor = requestor;
    x.u.selectionNotify.selection = selection;
    x.u.selectionNotify.target = target;
    x.u.selectionNotify.property = property;

    (void) TryClientEvents(client, &x, 1, NoEventMask,
                           NoEventMask , NullGrab);

    return 1;
  }

  if (target == MakeAtom("TIMESTAMP", 9, 1))
  {
    xEvent x;
    int i = 0;

    while ((i < NumCurrentSelections) &&
              CurrentSelections[i].selection != selection) i++;

    if (i < NumCurrentSelections)
    {
      ChangeWindowProperty(pWin,
                           property,
                           target,
                           32,
                           PropModeReplace,
                           1,
                           (unsigned char *) &lastSelectionOwner[i].lastTimeChanged,
                           1);

      x.u.u.type = SelectionNotify;
      x.u.selectionNotify.time = time;
      x.u.selectionNotify.requestor = requestor;
      x.u.selectionNotify.selection = selection;
      x.u.selectionNotify.target = target;
      x.u.selectionNotify.property = property;
  
      (void) TryClientEvents(client, &x, 1, NoEventMask,
                             NoEventMask , NullGrab);
  
      return 1;

    }
  }

  if (lastClientClientPtr == client && (GetTimeInMillis() - lastClientReqTime < 5000))
  {
    /*
     * The same client made consecutive requests
     * of clipboard contents with less than 5
     * seconds time interval between them.
     */

    #ifdef DEBUG
    fprintf(stderr, "nxagentConvertSelection: Consecutives request from client [%p] selection [%ld] "
                "elapsed time [%lu] clientAccum [%d]\n", (void *) client, selection,
                    GetTimeInMillis() - lastClientReqTime, clientAccum);
    #endif

    clientAccum++;
  }
  else
  {
    if (lastClientClientPtr != client)
    {
      clientAccum = 0;
    }
  }

  if ((target == clientTEXT) ||
          (target == XA_STRING) ||
              (target == clientCOMPOUND_TEXT) ||
                  (target == clientUTF8_STRING))
  {
    lastClientWindowPtr = pWin;
    lastClientStage = SelectionStageNone;
    lastClientRequestor = requestor;
    lastClientClientPtr = client;
    lastClientTime = time;
    lastClientProperty = property;
    lastClientSelection = selection;
    lastClientTarget = target;

    lastClientReqTime = (GetTimeInMillis() - lastClientReqTime) > 5000 ?
                          GetTimeInMillis() : lastClientReqTime;

    if (selection == MakeAtom("CLIPBOARD", 9, 0))
    {
      selection = lastSelectionOwner[nxagentClipboardSelection].selection;
    }

    if (target == clientUTF8_STRING)
    {
      XConvertSelection(nxagentDisplay, selection, serverUTF8_STRING, serverCutProperty,
                           serverWindow, CurrentTime);
    }
    else
    {
      XConvertSelection(nxagentDisplay, selection, XA_STRING, serverCutProperty,
                           serverWindow, CurrentTime);
    }

    #ifdef DEBUG
    fprintf(stderr, "nxagentConvertSelection: Sent XConvertSelection with target=[%s], property [%s]\n",
                validateString(NameForAtom(target)), validateString(NameForAtom(property)));
    #endif

    return 1;
  }
  else
  {
    xEvent x;

    #ifdef DEBUG
    fprintf(stderr, "nxagentConvertSelection: Xserver generates a SelectionNotify event "
               "to the requestor with property None.\n");
    #endif

    x.u.u.type = SelectionNotify;
    x.u.selectionNotify.time = time;
    x.u.selectionNotify.requestor = requestor;
    x.u.selectionNotify.selection = selection;
    x.u.selectionNotify.target = target;
    x.u.selectionNotify.property = None;
    (void) TryClientEvents(client, &x, 1, NoEventMask, NoEventMask , NullGrab);
    return 1;
  }
  return 0;
}

int nxagentSendNotify(xEvent *event)
{
  #ifdef DEBUG
  fprintf(stderr, "nxagentSendNotify: Got called.\n");
  #endif

  if (agentClipboardStatus != 1)
  {
    return 0;
  }

  if (event->u.selectionNotify.property == clientCutProperty)
  {
    XSelectionEvent x;
    int result;

    /*
     * Setup selection notify event to real server.
     */

    x.type = SelectionNotify;
    x.send_event = True;
    x.display = nxagentDisplay;
    x.requestor = serverWindow;

    /*
     * On real server, the right CLIPBOARD atom is
     * XInternAtom(nxagentDisplay, "CLIPBOARD", 1).
     */

    if (event->u.selectionNotify.selection == MakeAtom("CLIPBOARD", 9, 0))
    {
      x.selection = lastSelectionOwner[nxagentClipboardSelection].selection;
    }
    else
    {
      x.selection = event->u.selectionNotify.selection;
    }

    x.target = event->u.selectionNotify.target;
    x.property = event->u.selectionNotify.property;
    x.time = CurrentTime;

    #ifdef DEBUG
    fprintf(stderr, "nxagentSendNotify: Propagating clientCutProperty.\n");
    #endif

    result = XSendEvent (nxagentDisplay, x.requestor, False,
                             0L, (XEvent *) &x);

    if (result == BadValue || result == BadWindow)
    {
       fprintf (stderr, "nxagentSendNotify: XSendEvent failed.\n");
    }

    return 1;
  }

  return 0;
}

WindowPtr nxagentGetClipboardWindow(Atom property, WindowPtr pWin)
{
  int i = 0;

  #ifdef DEBUG
  fprintf(stderr, "nxagentGetClipboardWindow: Got called.\n");
  #endif

  while ((i < nxagentMaxSelections) &&
            (lastSelectionOwner[i].selection != nxagentLastRequestedSelection))
  {
    i++;
  }

  if ((i < nxagentMaxSelections) && (property == clientCutProperty) &&
          (lastSelectionOwner[i].windowPtr != NULL))
  {
    #ifdef DEBUG
    fprintf(stderr, "nxagentGetClipboardWindow: Returning last clipboard owner window.\n");
    #endif

    return lastSelectionOwner[i].windowPtr;
  }
  else
  {
    #ifdef DEBUG
    fprintf(stderr, "nxagentGetClipboardWindow: Returning original target window.\n");
    #endif

    return pWin;
  }

}

int nxagentInitClipboard(WindowPtr pWin)
{
  int i;
  Window iWindow = nxagentWindow(pWin);

  #ifdef DEBUG
  fprintf(stderr, "nxagentInitClipboard: Got called.\n");
  #endif

  if (lastSelectionOwner != NULL)
  {
    xfree(lastSelectionOwner);
    lastSelectionOwner = NULL;
  }

  lastSelectionOwner = (SelectionOwner *) xalloc(2 * sizeof(SelectionOwner));

  if (lastSelectionOwner == NULL)
  {
    FatalError("nxagentInitClipboard: Failed to allocate memory for the clipboard selections.\n");
  }

  nxagentClipboardAtom = nxagentAtoms[10];   /* CLIPBOARD */
  nxagentTimestampAtom = nxagentAtoms[11];   /* TIMESTAMP */

  lastSelectionOwner[nxagentPrimarySelection].selection = XA_PRIMARY;
  lastSelectionOwner[nxagentPrimarySelection].client = NullClient;
  lastSelectionOwner[nxagentPrimarySelection].window = WindowTable[0]->drawable.id;
  lastSelectionOwner[nxagentPrimarySelection].windowPtr = NULL;
  lastSelectionOwner[nxagentPrimarySelection].lastTimeChanged = GetTimeInMillis();

  lastSelectionOwner[nxagentClipboardSelection].selection = nxagentClipboardAtom;
  lastSelectionOwner[nxagentClipboardSelection].client = NullClient;
  lastSelectionOwner[nxagentClipboardSelection].window = WindowTable[0]->drawable.id;
  lastSelectionOwner[nxagentClipboardSelection].windowPtr = NULL;
  lastSelectionOwner[nxagentClipboardSelection].lastTimeChanged = GetTimeInMillis();

  #ifdef NXAGENT_TIMESTAMP
  {
    extern unsigned long startTime;

    fprintf(stderr, "nxagentInitClipboard: Initializing start [%d] milliseconds.\n",
            GetTimeInMillis() - startTime);
  }
  #endif

  agentClipboardStatus = 0;
  serverWindow = iWindow;

  /*
   * Local property to hold pasted data.
   */

  serverCutProperty = nxagentAtoms[5];  /* NX_CUT_BUFFER_SERVER */
  serverTARGETS = nxagentAtoms[6];  /* TARGETS */
  serverTEXT = nxagentAtoms[7];  /* TEXT */
  serverUTF8_STRING = nxagentAtoms[12]; /* UTF8_STRING */

  if (serverCutProperty == None)
  {
    #ifdef PANIC
    fprintf(stderr, "nxagentInitClipboard: PANIC! Could not create NX_CUT_BUFFER_SERVER atom\n");
    #endif

    return -1;
  }

  #ifdef TEST
  fprintf(stderr, "nxagentInitClipboard: Setting owner of selection [%s][%d] on window 0x%lx\n",
              "NX_CUT_BUFFER_SERVER", (int) serverCutProperty, iWindow);
  #endif

  XSetSelectionOwner(nxagentDisplay, serverCutProperty, iWindow, CurrentTime);

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
    fprintf(stderr, "nxagentInitClipboard: Registering for XFixesSelectionNotify events.\n");
    #endif

    XFixesSelectSelectionInput(nxagentDisplay, iWindow, nxagentClipboardAtom,
                               XFixesSetSelectionOwnerNotifyMask |
                               XFixesSelectionWindowDestroyNotifyMask |
                               XFixesSelectionClientCloseNotifyMask);

    nxagentXFixesInfo.Initialized = 1;
  }

  if (nxagentSessionId[0])
  {
    #ifdef TEST
    fprintf(stderr, "nxagentInitClipboard: setting the ownership of %s to %lx"
                " and registering for PropertyChangeMask events\n",
                    validateString(XGetAtomName(nxagentDisplay, nxagentAtoms[10])), iWindow);
    #endif

    XSetSelectionOwner(nxagentDisplay, nxagentAtoms[10], iWindow, CurrentTime);
    pWin -> eventMask |= PropertyChangeMask;
    nxagentChangeWindowAttributes(pWin, CWEventMask);
  }

  if (nxagentReconnectTrap)
  {
    /*
     * Only for PRIMARY and CLIPBOARD selections.
     */

    for (i = 0; i < nxagentMaxSelections; i++)
    {
      if (lastSelectionOwner[i].client && lastSelectionOwner[i].window)
      {
        XSetSelectionOwner(nxagentDisplay, lastSelectionOwner[i].selection, iWindow, CurrentTime);
      }
    }
  }
  else
  {
    lastSelectionOwner[nxagentPrimarySelection].client = NULL;
    lastSelectionOwner[nxagentClipboardSelection].client = NULL;

    lastServerRequestor = None;

    lastClientWindowPtr = NULL;
    lastClientStage = SelectionStageNone;
    lastClientReqTime = GetTimeInMillis();

    clientCutProperty = MakeAtom(szAgentNX_CUT_BUFFER_CLIENT,
                                         strlen(szAgentNX_CUT_BUFFER_CLIENT), 1);
    clientTARGETS = MakeAtom(szAgentTARGETS, strlen(szAgentTARGETS), True);
    clientTEXT = MakeAtom(szAgentTEXT, strlen(szAgentTEXT), True);
    clientCOMPOUND_TEXT = MakeAtom(szAgentCOMPOUND_TEXT, strlen(szAgentCOMPOUND_TEXT), True);
    clientUTF8_STRING = MakeAtom(szAgentUTF8_STRING, strlen(szAgentUTF8_STRING), True);

    if (clientCutProperty == None)
    {
      #ifdef PANIC
      fprintf(stderr, "nxagentInitClipboard: PANIC! "
                  "Could not create NX_CUT_BUFFER_CLIENT atom.\n");
      #endif

      return -1;
    }
  }

  agentClipboardStatus = 1;

  #ifdef DEBUG
  fprintf(stderr, "nxagentInitClipboard: Clipboard initialization completed.\n");
  #endif

  #ifdef NXAGENT_TIMESTAMP
  {
    extern unsigned long startTime;

    fprintf(stderr, "nxagentInitClipboard: initializing ends [%d] milliseconds.\n",
                GetTimeInMillis() - startTime);
  }
  #endif

  return 1;
}

