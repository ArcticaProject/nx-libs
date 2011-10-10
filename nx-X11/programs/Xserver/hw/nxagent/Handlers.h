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

#ifndef __Handlers_H__
#define __Handlers_H__

/*
 * Current size of the display buffer.
 */

extern int nxagentBuffer;

/*
 * Set if we had to block waiting for
 * the display to become writable.
 */

extern int nxagentBlocking;

/*
 * Current congestion level based on
 * the sync requests awaited or the
 * proxy tokens.
 */

extern int nxagentCongestion;

/*
 * Bytes read from the agent's clients
 * and written to the display socket.
 */

extern double nxagentBytesIn;
extern double nxagentBytesOut;

/*
 * Total number of descriptors ready
 * as reported by the wakeup handler.
 */

extern int nxagentReady;

/*
 * Timestamp of the last write to the
 * remote display.
 */

extern int nxagentFlush;

/*
 * Let the dispatch loop yield control to
 * a different client after a fair amount
 * of time or after enough data has been
 * processed.
 */

struct _DispatchRec
{
  int client;

  double in;
  double out;

  unsigned long start;
};

extern struct _DispatchRec nxagentDispatch;

/*
 * Ensure that we synchronize with the X
 * server after a given amount of output
 * is produced.
 */

struct _TokensRec
{
  int soft;
  int hard;

  int pending;
};

extern struct _TokensRec nxagentTokens;

/*
 * The agent's block and wakeup handlers.
 */

void nxagentBlockHandler(pointer data, struct timeval **timeout, pointer mask);
void nxagentWakeupHandler(pointer data, int count, pointer mask);

/*
 * Executed after each request processed.
 */

void nxagentDispatchHandler(ClientPtr client, int in, int out);

void nxagentShadowBlockHandler(pointer data, struct timeval **timeout, pointer mask);
void nxagentShadowWakeupHandler(pointer data, int count, pointer mask);

extern GCPtr nxagentShadowGCPtr;
extern unsigned char nxagentShadowDepth;
extern int nxagentShadowWidth;
extern int nxagentShadowHeight;
extern char *nxagentShadowBuffer;

#endif /* __Handlers_H__ */
