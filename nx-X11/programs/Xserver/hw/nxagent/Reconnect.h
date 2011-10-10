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
