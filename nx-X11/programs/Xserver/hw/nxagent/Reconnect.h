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

#ifndef __Reconnect_H__
#define __Reconnect_H__

extern Display *nxagentDisplayState;

struct nxagentExceptionStruct
{
  int sigHup;
  int ioError;
};

extern struct nxagentExceptionStruct nxagentException;

void nxagentSetReconnectError(int id, char *format, ...);

void nxagentInitReconnector(void);
Bool nxagentReconnectSession(void);
int nxagentHandleConnectionStates(void);
void nxagentHandleConnectionChanges(void);
void setStatePath(char*);
void saveAgentState(char*);

enum SESSION_STATE
{
  SESSION_STARTING,
  SESSION_UP,
  SESSION_DOWN,
  SESSION_GOING_DOWN,
  SESSION_GOING_UP
};

extern enum SESSION_STATE nxagentSessionState;

#define DECODE_SESSION_STATE(state) \
    ((state) == SESSION_STARTING ? "SESSION_STARTING" : \
        (state) == SESSION_UP ? "SESSION_UP" : \
            (state) == SESSION_GOING_UP? "SESSION_GOING_UP" : \
                (state) == SESSION_DOWN ? "SESSION_DOWN" : \
                    (state) == SESSION_GOING_DOWN? "SESSION_GOING_DOWN" : \
                        "UNKNOWN")

/*
 * Use this macro in the block and wakeup
 * handlers to save a function call.
 */

#define nxagentNeedConnectionChange() \
    (nxagentSessionState == SESSION_GOING_DOWN || \
         nxagentSessionState == SESSION_GOING_UP)

void nxagentDisconnectSession(void);

#endif /* __Reconnect_H__ */
