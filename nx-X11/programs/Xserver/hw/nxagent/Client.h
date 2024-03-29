/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2011 NoMachine (http://www.nomachine.com)          */
/* Copyright (c) 2008-2017 Oleksandr Shneyder <o.shneyder@phoca-gmbh.de>  */
/* Copyright (c) 2011-2022 Mike Gabriel <mike.gabriel@das-netzwerkteam.de>*/
/* Copyright (c) 2014-2019 Mihai Moldovan <ionic@ionic.de>                */
/* Copyright (c) 2014-2022 Ulrich Sibiller <uli42@gmx.de>                 */
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

#ifndef __Client_H__
#define __Client_H__

#define MAX_CONNECTIONS 256

/*
 * The master nxagent holds in nxagentShadowCounter
 * the number of shadow nxagents connected to itself.
 */

extern int nxagentShadowCounter;

enum ClientHint
{
  UNKNOWN = 0,
  NXCLIENT_WINDOW,
  NXCLIENT_DIALOG,
  NXAGENT_SHADOW,
  JAVA_WINDOW
};

/* client byte counting is currently unused */
#undef COUNT_CLIENT_BYTES

typedef struct _PrivClientRec
{
  int clientState;
#ifdef COUNT_CLIENT_BYTES
  long clientBytes;
#endif
  enum ClientHint clientHint;
  char *clientInfoString;
} PrivClientRec;

extern int nxagentClientPrivateIndex;

#define nxagentClientPriv(pClient) \
  ((PrivClientRec *)((pClient)->devPrivates[nxagentClientPrivateIndex].ptr))

extern void nxagentClientStateCallback(CallbackListPtr *callbacks, void *data, void *args);

#ifdef COUNT_CLIENT_BYTES
#define nxagentClientAddBytes(pClient, size)	\
  (nxagentClientPriv(pClient) -> clientBytes += (size))

#define nxagentClientBytes(pClient)	\
    (nxagentClientPriv(pClient) -> clientBytes)
#endif

#define nxagentClientHint(pClient) \
    (nxagentClientPriv(pClient) -> clientHint)

#define nxagentClientIsDialog(pClient) \
    (nxagentClientHint(pClient) == NXCLIENT_DIALOG)

#define nxagentClientInfoString(pClient) \
    ((pClient) ? nxagentClientPriv(pClient) -> clientInfoString : NULL)

/*
 * The actual reason why the client is sleeping.
 */

#define SleepingBySplit  1

#define nxagentNeedWakeup(client) \
               ((nxagentClientPriv(client) -> \
                      clientState) != 0)

#define nxagentNeedWakeupBySplit(client) \
               (((nxagentClientPriv(client) -> \
                      clientState) & SleepingBySplit) != 0)

void nxagentGuessClientHint(ClientPtr, Atom, char*);

void nxagentGuessShadowHint(ClientPtr, Atom);

/*
 * Suspend or restart the agent's client.
 */

int nxagentSuspendBySplit(ClientPtr client);
int nxagentWakeupBySplit(ClientPtr client);

/*
 * Wait until the given client is restarted.
 */

void nxagentWaitWakeupBySplit(ClientPtr client);

/*
FIXME: This must be moved to Drawable.h.
*/
void nxagentWaitDrawable(DrawablePtr pDrawable);

/*
 * Wakeup all the sleeping clients.
 */

void nxagentWakeupByReconnect(void);

/*
 * Reset the client state before closing it down.
 */

void nxagentWakeupByReset(ClientPtr client);

#endif /* __Client_H__ */
