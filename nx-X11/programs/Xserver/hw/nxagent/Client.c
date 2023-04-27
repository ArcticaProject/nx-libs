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

/*
 * Used in handling of karma on lost focus.
 */

#include <signal.h>
#include <time.h>
#include <errno.h>

#include <nx/NX.h>

#include "Xatom.h"
#include "dixstruct.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "osdep.h"

/*
 * NX specific includes and definitions.
 */

#include "Agent.h"
#include "Args.h"
#include "Display.h"
#include "Client.h"
#include "Dialog.h"
#include "Handlers.h"
#include "Events.h"
#include "Drawable.h"
#include "Utils.h"
#include "Clipboard.h"

/*
 * Need to include this after the stub definition of GC in Agent.h.
 */

#include "compext/Compext.h"

/*
 * Set here the required log level.
 */

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

void nxagentClientStateCallback(CallbackListPtr *callbacks, void *data, void *args);
static void initClientPrivates(ClientPtr client);
static void freeClientPrivates(ClientPtr client);
static void checkIfShadowAgent(ClientPtr client);

/*
 * Returns the last signal delivered to the process.
 */

extern int _X11TransSocketCheckSignal(void);

/*
 * Time in milliseconds of first iteration through the dispatcher.
 */

unsigned long nxagentStartTime = -1;

/*
 * If defined, add a function checking if we need a null timeout after
 * a client wakeup.
 */

#undef CHECK_RESTARTED_CLIENTS

#ifdef CHECK_RESTARTED_CLIENTS

void nxagentCheckRestartedClients(struct timeval **timeout);

#endif

/*
 * Allow attaching private members to the client.
 */

int nxagentClientPrivateIndex;

/*
 * The master nxagent holds in nxagentShadowCounter the number of
 * shadow nxagents connected to itself.
 */

int nxagentShadowCounter = 0;

/*
 * For the serverclient the ClientStateCallback will not be called on
 * shutdown resulting in memory allocated during initClientPrivates can
 * not be freed automatically. So instead of allocating some memory we
 * create a static string for the serverclient.
 */
static char *serverclientInfoString = "[0] (serverclient)";

/*
 * called whenever the client state changes. See dixstruct.h for a
 * list of known states.
 */

#ifdef DEBUG
const char * getClientStateString(int state)
{
  switch (state)
  {
    case ClientStateInitial:          { return "Initial"; break; };
    case ClientStateAuthenticating:   { return "Authenticating"; break; };
    case ClientStateRunning:          { return "Running"; break; };
    case ClientStateRetained:         { return "Retained"; break; };
    case ClientStateGone:             { return "Gone"; break; };
    case ClientStateCheckingSecurity: { return "CheckingSecurity"; break; };
    case ClientStateCheckedSecurity:  { return "CheckedSecurity"; break; };
    default:                          { return "UNKNOWN"; break; };
  }
}
#endif

void nxagentClientStateCallback(CallbackListPtr *callbacks, void *data, void *args)
{
  ClientPtr client = ((NewClientInfoRec *)args)->client;

  #ifdef DEBUG
  fprintf(stderr, "%s: client [%d] clientState [%s]\n", __func__, client->index,
              getClientStateString(client->clientState));
  #endif

  switch(client->clientState)
  {
    case ClientStateInitial:
    {
      initClientPrivates(client);
      break;
    }
    case ClientStateGone:
    {
      nxagentClearClipboard(client, NULL);

      /*
       * Check if the client is a shadow nxagent.
       */
      checkIfShadowAgent(client);

      freeClientPrivates(client);
      break;
    }
    default:
    {
      break;
    }
  }
}

static void initClientPrivates(ClientPtr client)
{
  #ifdef DEBUG
  fprintf(stderr, "%s: called\n", __func__);
  #endif

  if (nxagentClientPriv(client))
  {
    nxagentClientPriv(client) -> clientState = 0;
#ifdef COUNT_CLIENT_BYTES
    nxagentClientPriv(client) -> clientBytes = 0;
#endif
    nxagentClientPriv(client) -> clientHint  = UNKNOWN;
    nxagentClientPriv(client) -> clientInfoString = NULL;

    char *s = NULL;
    int size = 0;

    if (client->index == 0)
    {
      s = serverclientInfoString;
    }
    else
    {
#ifdef CLIENTIDS
      size = asprintf(&s, "[%d] (addr [%p] PID [%d] Cmd [%s])",
                          client->index, (void *)client,
                              GetClientPid(client),
                                  GetClientCmdName(client));
#else
      size = asprintf(&s, "[%d] (addr [%p])",
                          client->index, (void *)client);
#endif
    }

    if (size != -1)
    {
      #ifdef DEBUG
      fprintf(stderr, "%s: clientInfoString: \"%s\"\n", __func__, s);
      #endif

      nxagentClientPriv(client) -> clientInfoString = s;
    }
    else
    {
      #ifdef DEBUG
      fprintf(stderr, "%s: could not alloc clientInfoString\n", __func__);
      #endif
    }
  }
}

static void freeClientPrivates(ClientPtr client)
{
  #ifdef DEBUG
  fprintf(stderr, "%s: called\n", __func__);
  #endif

  if (nxagentClientPriv(client))
  {
    nxagentClientPriv(client) -> clientState = 0;
#ifdef COUNT_CLIENT_BYTES
    nxagentClientPriv(client) -> clientBytes = 0;
#endif
    nxagentClientPriv(client) -> clientHint  = UNKNOWN;

    if (client->index != 0)
      SAFE_free(nxagentClientPriv(client) -> clientInfoString);
  }
}

/*
 * Guess the running application based on the properties attached to
 * its main window.
 */

void nxagentGuessClientHint(ClientPtr client, Atom property, char *data)
{
  #ifdef TEST
  fprintf(stderr, "++++++nxagentGuessClientHint: Client [%d] setting property [%s] as [%s].\n",
              client -> index, validateString(NameForAtom(property)), validateString(data));
  #endif

  if (nxagentClientHint(client) == UNKNOWN)
  {
    if (property == XA_WM_CLASS)
    {
      if (strcmp(data, "nxclient") == 0)
      {
        #ifdef TEST
        fprintf(stderr, "++++++nxagentGuessClientHint: Detected nxclient as [%d].\n", client -> index);
        #endif

        nxagentClientHint(client) = NXCLIENT_WINDOW;
      }
      else if (strstr(data, "java"))
      {
        #ifdef TEST
        fprintf(stderr, "++++++nxagentGuessClientHint: Detected java as [%d].\n", client -> index);
        #endif

        nxagentClientHint(client) = JAVA_WINDOW;
      }
    }
  }

  if (nxagentClientHint(client) == NXCLIENT_WINDOW)
  {
    if (property == MakeAtom("WM_WINDOW_ROLE", 14, True) &&
            strncmp(data, "msgBox", 6) == 0)
    {
      #ifdef TEST
      fprintf(stderr, "++++++nxagentGuessClientHint: Detected nxclient dialog as [%d].\n", client -> index);
      #endif

      nxagentClientHint(client) = NXCLIENT_DIALOG;
    }
  }
}

void nxagentGuessShadowHint(ClientPtr client, Atom property)
{
  #ifdef DEBUG
  fprintf(stderr, "nxagentGuessShadowHint: Client [%d] setting property [%s].\n",
              client -> index,
                  validateString(NameForAtom(property)));
  #endif

  if (nxagentClientHint(client) == UNKNOWN)
  {
    if (strcmp(validateString(NameForAtom(property)), "_NX_SHADOW") == 0)
    {
      #ifdef TEST
       fprintf(stderr, "nxagentGuessShadowHint: nxagentShadowCounter [%d].\n",
                   nxagentShadowCounter);

       fprintf(stderr, "nxagentGuessShadowHint: Detected shadow nxagent as client [%d].\n",
                   client -> index);

      #endif

      nxagentClientHint(client) = NXAGENT_SHADOW;

      nxagentShadowCounter++;

      #ifdef TEST
       fprintf(stderr, "nxagentGuessShadowHint: nxagentShadowCounter [%d].\n",
                  nxagentShadowCounter);
      #endif

      /*
       * From this moment on we ignore the visibility checks to keep
       * the windows updated.
       */

      nxagentChangeOption(IgnoreVisibility, True);
    }
  }
}

static void checkIfShadowAgent(ClientPtr client)
{
  if (nxagentClientHint(client) == NXAGENT_SHADOW)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentCheckIfShadowAgent: nxagentShadowCounter [%d].\n",
                nxagentShadowCounter);

    fprintf(stderr, "nxagentCheckIfShadowAgent: Shadow nxagent as client [%d] detached.\n",
                client -> index);

    fprintf(stderr, "nxagentCheckIfShadowAgent: Decreasing nxagentShadowCounter.\n");
    #endif

    /*
     * We decrease nxagentShadowCounter.
     */

    nxagentShadowCounter--;

    #ifdef TEST
    fprintf(stderr, "nxagentCheckIfShadowAgent: nxagentShadowCounter [%d].\n",
                nxagentShadowCounter);
    #endif

    if (nxagentShadowCounter == 0)
    {
      /*
       * The last shadow nxagent has been detached from master
       * nxagent.  The master nxagent could do some action here.
       */

       #ifdef TEST
       fprintf(stderr, "nxagentCheckIfShadowAgent: The last shadow nxagent has been detached.\n");
       #endif

      nxagentChangeOption(IgnoreVisibility, False);
    }
  }
}

void nxagentWakeupByReconnect(void)
{
  #ifdef TEST
  fprintf(stderr, "++++++nxagentWakeupByReconnect: Going to wakeup all clients.\n");
  #endif

  for (int i = 1; i < currentMaxClients; i++)
  {
    if (clients[i] != NULL)
    {
      nxagentWakeupByReset(clients[i]);
    }
  }
}

void nxagentWakeupByReset(ClientPtr client)
{
  #ifdef TEST
  fprintf(stderr, "++++++nxagentWakeupByReset: Going to check client id [%d].\n",
              client -> index);
  #endif

  if (nxagentNeedWakeup(client))
  {
    #ifdef TEST
    fprintf(stderr, "++++++nxagentWakeupByReset: Going to wakeup client id [%d].\n",
                client -> index);
    #endif

    if (client -> index < MAX_CONNECTIONS)
    {
      if (nxagentNeedWakeupBySplit(client))
      {
        nxagentWakeupBySplit(client);
      }
    }
  }

#ifdef COUNT_CLIENT_BYTES
  if (client -> index < MAX_CONNECTIONS)
  {
    #ifdef TEST
    fprintf(stderr, "++++++nxagentWakeupByReset: Going to reset bytes received for client id [%d].\n",
                client -> index);
    #endif

    nxagentClientBytes(client) = 0;
  }
#endif
}

/*
 * Wait for any event.
 */

#define WAIT_ALL_EVENTS

#ifndef WAIT_ALL_EVENTS

static Bool nxagentWaitWakeupBySplitPredicate(Display *disp, XEvent *event, XPointer ptr)
{
  return (event -> type == ClientMessage &&
              (event -> xclient.data.l[0] == NXNoSplitNotify ||
                  event -> xclient.data.l[0] == NXStartSplitNotify ||
                      event -> xclient.data.l[0] == NXCommitSplitNotify ||
                          event -> xclient.data.l[0] == NXEndSplitNotify ||
                              event -> xclient.data.l[0] == NXEmptySplitNotify) &&
                                  event -> xclient.window == 0 && event -> xclient.message_type == 0 &&
                                      event -> xclient.format == 32);
}

#endif

#define USE_FINISH_SPLIT

void nxagentWaitWakeupBySplit(ClientPtr client)
{
  #ifdef TEST

  if (!nxagentNeedWakeupBySplit(client))
  {
    fprintf(stderr, "++++++nxagentWaitWakeupBySplit: WARNING! The client [%d] is already awake.\n",
                client -> index);
  }

  fprintf(stderr, "++++++nxagentWaitWakeupBySplit: Going to wait for the client [%d].\n",
              client -> index);
  #endif

  /*
   * Be sure we intercept an I/O error as well as an interrupt.
   */

  #ifdef USE_FINISH_SPLIT

  NXFinishSplit(nxagentDisplay, client -> index);

  #endif

  NXFlushDisplay(nxagentDisplay, NXFlushBuffer);

  for (;;)
  {
    /*
     * Can we handle all the possible events here or we need to select
     * only the split events?  Handling all the possible events would
     * preempt the queue and make a better use of the link.
     */

    #ifdef WAIT_ALL_EVENTS

    nxagentDispatchEvents(NULL);

    #else

    nxagentDispatchEvents(nxagentWaitWakeupBySplitPredicate);

    #endif

    if (!nxagentNeedWakeupBySplit(client) ||
            NXDisplayError(nxagentDisplay) == 1)
    {
      #ifdef TEST

      if (!nxagentNeedWakeupBySplit(client))
      {
        fprintf(stderr, "++++++nxagentWaitWakeupBySplit: Client [%d] can now run.\n",
                    client -> index);
      }
      else
      {
        fprintf(stderr, "++++++nxagentWaitWakeupBySplit: WARNING! Display error "
                    "detected waiting for restart.\n");
      }

      #endif

      return;
    }

    #ifdef TEST
    fprintf(stderr, "++++++nxagentWaitWakeupBySplit: Yielding control to the NX transport.\n");
    #endif

    nxagentWaitEvents(nxagentDisplay, 0);
  }
}

int nxagentSuspendBySplit(ClientPtr client)
{
/*
FIXME: Should record a serial number for the client, so that the
       client is not restarted because of an end of split of a
       previous client with the same index.
*/
  if (client -> index < MAX_CONNECTIONS)
  {
    if (!nxagentNeedWakeup(client))
    {
      #ifdef TEST
      fprintf(stderr, "++++++nxagentSuspendBySplit: Suspending client [%d] with agent sequence [%ld].\n",
                  client -> index, NextRequest(nxagentDisplay) - 1);
      #endif

      if (client -> clientGone == 0)
      {
        #ifdef TEST
        fprintf(stderr, "++++++nxagentSuspendBySplit: Client [%d] suspended.\n", client -> index);
        #endif

        IgnoreClient(client);
      }
    }
    #ifdef TEST
    else
    {
      fprintf(stderr, "++++++nxagentSuspendBySplit: WARNING! Client [%d] already ignored with state [%x].\n",
                  client -> index, nxagentClientPriv(client) -> clientState);
    }
    #endif

    nxagentClientPriv(client) -> clientState |= SleepingBySplit;

    return 1;
  }

  #ifdef WARNING
  fprintf(stderr, "++++++nxagentSuspendBySplit: WARNING! Invalid client [%d] provided to function.\n",
              client -> index);
  #endif

  return -1;
}

int nxagentWakeupBySplit(ClientPtr client)
{
/*
FIXME: Should record a serial number for the client, so that the
       client is not restarted because of the end of the split for a
       previous client with the same index.
*/
  if (client -> index < MAX_CONNECTIONS)
  {
    nxagentClientPriv(client) -> clientState &= ~SleepingBySplit;

    if (!nxagentNeedWakeup(client))
    {
      #ifdef TEST
      fprintf(stderr, "++++++nxagentWakeupBySplit: Resuming client [%d] with agent sequence [%ld].\n",
                  client -> index, NextRequest(nxagentDisplay) - 1);
      #endif

      if (client -> clientGone == 0)
      {
        AttendClient(client);
      }
    }
    #ifdef TEST
    else
    {
      fprintf(stderr, "++++++nxagentWakeupBySplit: WARNING! Client [%d] still suspended with state [%x].\n",
                  client -> index, nxagentClientPriv(client) -> clientState);
    }
    #endif

    return 1;
  }

  #ifdef WARNING
  fprintf(stderr, "++++++nxagentWakeupBySplit: WARNING! Invalid client [%d] provided to function.\n",
              client -> index);
  #endif

  return -1;
}

#ifdef CHECK_RESTARTED_CLIENTS

void nxagentCheckRestartedClients(struct timeval **timeout)
{
  static struct timeval zero;

  /*
   * If any of the restarted clients had requests in input we'll need
   * to enter the select with a null timeout, or we will block until
   * any other client becomes available.
   */

  for (int i = 1; i < currentMaxClients; i++)
  {
    if (clients[i] != NULL && clients[i] -> osPrivate != NULL &&
           !nxagentNeedWakeup(clients[i]))
    {
      int fd = ((OsCommPtr) clients[i] -> osPrivate) -> fd;

      if (FD_ISSET(fd, &ClientsWithInput))
      {
        #ifdef WARNING
        fprintf(stderr, "nxagentCheckRestartedClients: WARNING! Client [%d] with fd [%d] has input.\n",
                    clients[i] -> index, fd);
        #endif

        #ifdef DEBUG
        fprintf(stderr, "nxagentCheckRestartedClients: Setting a null timeout with former timeout [%ld] Ms.\n",
                    (*timeout) -> tv_sec * 1000 + (*timeout) -> tv_usec / 1000);
        #endif

        if (*timeout != NULL)
        {
          (*timeout) -> tv_sec  = 0;
          (*timeout) -> tv_usec = 0;
        }
        else
        {
          zero.tv_sec  = 0;
          zero.tv_usec = 0;

          *timeout = &zero;
        }
      }
    }
  }
}
#endif
