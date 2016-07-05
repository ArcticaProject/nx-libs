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

Copyright 1993 by Davor Matic

Permission to use, copy, modify, distribute, and sell this software
and its documentation for any purpose is hereby granted without fee,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation.  Davor Matic makes no representations about
the suitability of this software for any purpose.  It is provided "as
is" without express or implied warranty.

*/

#ifndef __Keyboard_H__
#define __Keyboard_H__

#define NXAGENT_KEYBOARD_EVENT_MASK \
  (KeyPressMask | KeyReleaseMask | FocusChangeMask | KeymapStateMask)

#define NXAGENT_KEYBOARD_EXTENSION_EVENT_MASK \
  (XkbStateNotifyMask)

/*
 * Queried at XKB initialization.
 */

typedef struct _XkbAgentInfo
{
  int Opcode;
  int EventBase;
  int ErrorBase;
  int MajorVersion;
  int MinorVersion;

} XkbAgentInfoRec;

extern XkbAgentInfoRec nxagentXkbInfo;

typedef struct _XkbAgentState
{
  int Locked;
  int Caps;
  int Num;
  int Focus;
  int Initialized;

} XkbAgentStateRec;

extern XkbAgentStateRec nxagentXkbState;

/*
 * Info for enabling/disabling Xkb.
 */

typedef struct _XkbWrapper
{
  int base;
  int eventBase;
  int errorBase;
  int (* ProcXkbDispatchBackup)(ClientPtr);
  int (* SProcXkbDispatchBackup)(ClientPtr);

} XkbWrapperRec;

extern XkbWrapperRec nxagentXkbWrapper;

extern char *nxagentKeyboard;

/*
 * Keyboard device procedure
 * and utility functions.
 */

void nxagentBell(int volume, DeviceIntPtr pDev, void * ctrl, int cls);

int nxagentKeyboardProc(DeviceIntPtr pDev, int onoff);

void nxagentChangeKeyboardControl(DeviceIntPtr pDev, KeybdCtrl *ctrl);

void nxagentNotifyKeyboardChanges(int oldMinKeycode, int oldMaxKeycode);

int nxagentResetKeyboard(void);

#ifdef XKB

void nxagentInitXkbWrapper(void);

void nxagentDisableXkbExtension(void);

void nxagentEnableXkbExtension(void);

void nxagentTuneXkbWrapper(void);

void nxagentResetKeycodeConversion(void);

#endif

CARD8 nxagentConvertKeycode(CARD8 k);

extern CARD8 nxagentCapsLockKeycode;
extern CARD8 nxagentNumLockKeycode;

#endif /* __Keyboard_H__ */
