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

#ifndef __Pointer_H__
#define __Pointer_H__

#define MAXBUTTONS 256

#define NXAGENT_POINTER_EVENT_MASK \
  (ButtonPressMask | ButtonReleaseMask | PointerMotionMask | \
       EnterWindowMask | LeaveWindowMask)

/*
 * The nxagentReversePointerMap array is used to
 * memorize remote display pointer map.
 */

extern unsigned char nxagentReversePointerMap[MAXBUTTONS];

void nxagentChangePointerControl(DeviceIntPtr pDev, PtrCtrl *ctrl);

int nxagentPointerProc(DeviceIntPtr pDev, int onoff);

void nxagentInitPointerMap(void);

#endif /* __Pointer_H__ */
