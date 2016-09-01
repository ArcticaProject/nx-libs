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

#include <X11/X.h>
#include <X11/Xproto.h>
#include <nx-X11/Xlib.h>
#undef Bool
#include "screenint.h"
#include "input.h"
#include "misc.h"
#include "scrnintstr.h"
#include "servermd.h"
#include "mipointer.h"
#define XTestSERVER_SIDE
#include <X11/extensions/xtestext1.h>

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
