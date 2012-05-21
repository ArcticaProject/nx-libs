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

typedef struct _PrivClientRec
{
  int clientState;
  long clientBytes;
  enum ClientHint clientHint;

} PrivClientRec;

extern int nxagentClientPrivateIndex;

#define nxagentClientPriv(pClient) \
  ((PrivClientRec *)((pClient)->devPrivates[nxagentClientPrivateIndex].ptr))

void nxagentInitClientPrivates(ClientPtr);

#define nxagentClientAddBytes(pClient, size)	\
  (nxagentClientPriv(pClient) -> clientBytes += (size))

#define nxagentClientBytes(pClient)	\
    (nxagentClientPriv(pClient) -> clientBytes)

#define nxagentClientHint(pClient) \
    (nxagentClientPriv(pClient) -> clientHint)

#define nxagentClientIsDialog(pClient) \
    (nxagentClientHint(pClient) == NXCLIENT_DIALOG)

/*
 * The actual reason why the client
 * is sleeping.
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

void nxagentCheckIfShadowAgent(ClientPtr);

/*
 * Suspend or restart the agent's
 * client.
 */

int nxagentSuspendBySplit(ClientPtr client);
int nxagentWakeupBySplit(ClientPtr client);

/*
 * Wait until the given client is
 * restarted.
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
 * Reset the client state before
 * closing it down.
 */

void nxagentWakeupByReset(ClientPtr client);

#endif /* __Client_H__ */
