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

#include <X11/X.h>
#include <X11/Xproto.h>
#include <X11/Xlib.h>
#undef Bool
#include "screenint.h"
#include "input.h"
#include "misc.h"
#include "scrnintstr.h"
#include "servermd.h"
#include "mipointer.h"
#define XTestSERVER_SIDE
#include "xtestext1.h"

extern CARD32 nxagentLastEventTime;

void XTestGetPointerPos(short *fmousex, short *fmousey);

void XTestJumpPointer(int jx, int jy, int dev_type);

void XTestGenerateEvent(int dev_type, int keycode, int keystate,
                            int mousex, int mousey);

void XTestGetPointerPos(short *fmousex, short *fmousey)
{
  int x,y;
  
  miPointerPosition(&x, &y);
  *fmousex = x;
  *fmousey = y;
}

void XTestJumpPointer(int jx, int jy, int dev_type)
{
  miPointerAbsoluteCursor(jx, jy, GetTimeInMillis());
}

void XTestGenerateEvent(int dev_type, int keycode, int keystate,
                            int mousex, int mousey)
{
/*
  xEvent tevent;
  
  tevent.u.u.type = (dev_type == XE_POINTER) ?
    (keystate == XTestKEY_UP) ? ButtonRelease : ButtonPress :
      (keystate == XTestKEY_UP) ? KeyRelease : KeyPress;
  tevent.u.u.detail = keycode;
  tevent.u.keyButtonPointer.rootX = mousex;
  tevent.u.keyButtonPointer.rootY = mousey;
  tevent.u.keyButtonPointer.time = nxagentLastEventTime = GetTimeInMillis();
  mieqEnqueue(&tevent);
*/
}
