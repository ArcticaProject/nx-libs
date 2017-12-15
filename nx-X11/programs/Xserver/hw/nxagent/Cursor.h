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

#ifndef __Cursor_H__
#define __Cursor_H__

#include "cursorstr.h"
#include "picturestr.h"

typedef struct {
  Cursor cursor;
  PicturePtr picture;
  int uses_render;
  int x;
  int y;
} nxagentPrivCursor;

/*
 * _AnimCurElt and _AnimCur already defined in animcur.c.
 */

typedef struct _AnimCurElt {
    CursorPtr   pCursor;
    CARD32      delay;
} AnimCurElt;

typedef struct _AnimCur {
    int         nelt;
    AnimCurElt  *elts;
} AnimCurRec, *AnimCurPtr;

CursorBitsPtr nxagentAnimCursorBits;

#define nxagentIsAnimCursor(c)        ((c)->bits == nxagentAnimCursorBits)
#define nxagentGetAnimCursor(c)       ((AnimCurPtr) ((c) + 1))

#define nxagentCursorPriv(pCursor, pScreen) \
  ((nxagentPrivCursor *)((pCursor)->devPriv[pScreen->myNum]))

#define nxagentCursor(pCursor, pScreen) \
  (nxagentCursorPriv(pCursor, pScreen)->cursor)

#define nxagentCursorPicture(pCursor, pScreen) \
  (nxagentCursorPriv(pCursor, pScreen)->picture)

#define nxagentCursorUsesRender(pCursor, pScreen) \
  (nxagentCursorPriv(pCursor, pScreen)->uses_render)

#define nxagentCursorXOffset(pCursor, pScreen) \
  (nxagentCursorPriv(pCursor, pScreen)->x)

#define nxagentCursorYOffset(pCursor, pScreen) \
  (nxagentCursorPriv(pCursor, pScreen)->y)

void nxagentConstrainCursor(ScreenPtr pScreen, BoxPtr pBox);

void nxagentCursorLimits(ScreenPtr pScreen, CursorPtr pCursor,
                             BoxPtr pHotBox, BoxPtr pTopLeftBox);

Bool nxagentDisplayCursor(ScreenPtr pScreen, CursorPtr pCursor);

Bool nxagentRealizeCursor(ScreenPtr pScreen, CursorPtr pCursor);

Bool nxagentUnrealizeCursor(ScreenPtr pScreen, CursorPtr pCursor);

void nxagentRecolorCursor(ScreenPtr pScreen, CursorPtr pCursor,
                              Bool displayed);

Bool nxagentSetCursorPosition(ScreenPtr pScreen, int x, int y,
                                  Bool generateEvent);

extern Bool (*nxagentSetCursorPositionW)(ScreenPtr pScreen, int x, int y,
                                             Bool generateEvent);

void nxagentDisconnectCursor(void * p0, XID x1, void * p2);
void nxagentReconnectCursor(void * p0, XID x1, void * p2);
void nxagentReDisplayCurrentCursor(void);
Bool nxagentReconnectAllCursor(void *p0);
Bool nxagentDisconnectAllCursor(void);

#endif /* __Cursor_H__ */
