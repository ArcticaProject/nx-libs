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

void nxagentBlockHandler(void * data, struct timeval **timeout, void * mask);
void nxagentWakeupHandler(void * data, int count, void * mask);

/*
 * Executed after each request processed.
 */

void nxagentDispatchHandler(ClientPtr client, int in, int out);

void nxagentShadowBlockHandler(void * data, struct timeval **timeout, void * mask);
void nxagentShadowWakeupHandler(void * data, int count, void * mask);

extern GCPtr nxagentShadowGCPtr;
extern unsigned char nxagentShadowDepth;
extern int nxagentShadowWidth;
extern int nxagentShadowHeight;
extern char *nxagentShadowBuffer;

#endif /* __Handlers_H__ */
