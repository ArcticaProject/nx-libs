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

#include "X.h"
#include "Xproto.h"
#include "screenint.h"
#include "input.h"
#include "misc.h"
#include "cursor.h"
#include "cursorstr.h"
#include "scrnintstr.h"
#include "servermd.h"
#include "inputstr.h"

#include "Agent.h"
#include "Display.h"
#include "Options.h"
#include "Screen.h"
#include "Cursor.h"
#include "Image.h"
#include "Visual.h"
#include "Keyboard.h"
#include "Args.h"
#include "Windows.h"
#include "Events.h"
#include "Render.h"
#include "Client.h"

#include "windowstr.h"
#include "resource.h"

#include "NXlib.h"

/*
 * Set here the required log level.
 */

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

/*
 * Defined in Display.c. There are huge
 * problems mixing the GC definition in
 * Xlib with the server code. This must
 * be reworked.
 */

extern XlibGC nxagentBitmapGC;

/*
 * From NXevents.c.
 */

extern CursorPtr GetSpriteCursor(void);

void nxagentConstrainCursor(ScreenPtr pScreen, BoxPtr pBox)
{
  #ifdef TEST

  int width, height;

  width  = nxagentOption(RootWidth);
  height = nxagentOption(RootHeight);

  if (pBox->x1 <= 0 && pBox->y1 <= 0 &&
          pBox->x2 >= width && pBox->y2 >= height)
  {
    fprintf(stderr, "nxagentConstrainCursor: Called with box [%d,%d,%d,%d]. "
                "Skipping the operation.\n", pBox->x1, pBox->y1, pBox->x2, pBox->y2);
  }
  else
  {
    fprintf(stderr, "nxagentConstrainCursor: WARNING! Called with box [%d,%d,%d,%d].\n",
                pBox->x1, pBox->y1, pBox->x2, pBox->y2);
  }

  #endif
}

void nxagentCursorLimits(ScreenPtr pScreen, CursorPtr pCursor,
                             BoxPtr pHotBox, BoxPtr pTopLeftBox)
{
  *pTopLeftBox = *pHotBox;
}

Bool nxagentDisplayCursor(ScreenPtr pScreen, CursorPtr pCursor)
{

  /*
   * Don't define the root cursor
   * so that nxagent root window
   * inherits the parent's cursor.
   */

  Cursor cursor;

  cursor = (pCursor != rootCursor) ? nxagentCursor(pCursor, pScreen): None;

  if (nxagentOption(Rootless) == False)
  {
    XDefineCursor(nxagentDisplay,
                      nxagentInputWindows[pScreen -> myNum],
                          cursor);

    #ifdef TEST
    fprintf(stderr, "nxagentDisplayCursor: Called for cursor at [%p] with private [%p].\n",
                (void *) pCursor, pCursor->devPriv[pScreen->myNum]);
    #endif
  }

  return True;
}

Bool nxagentRealizeCursor(ScreenPtr pScreen, CursorPtr pCursor)
{
  XImage *image;
  Pixmap source, mask;
  XColor fg_color, bg_color;
  unsigned long valuemask;
  XGCValues values;

  #ifdef TEST
  fprintf(stderr, "nxagentRealizeCursor: Called for cursor at [%p].\n", (void *) pCursor);
  #endif

  valuemask = GCFunction |
              GCPlaneMask |
              GCForeground |
              GCBackground |
              GCClipMask;

  values.function = GXcopy;
  values.plane_mask = AllPlanes;
  values.foreground = 1L;
  values.background = 0L;
  values.clip_mask = None;

  XChangeGC(nxagentDisplay, nxagentBitmapGC, valuemask, &values);

  source = XCreatePixmap(nxagentDisplay,
                         nxagentDefaultWindows[pScreen->myNum],
                         pCursor->bits->width,
                         pCursor->bits->height,
                         1);

  mask = XCreatePixmap(nxagentDisplay,
                       nxagentDefaultWindows[pScreen->myNum],
                       pCursor->bits->width,
                       pCursor->bits->height,
                       1);

  image = XCreateImage(nxagentDisplay,
                       nxagentDefaultVisual(pScreen),
                       1, XYBitmap, 0,
                       (char *)pCursor->bits->source,
                       pCursor->bits->width,
                       pCursor->bits->height,
                       BitmapPad(nxagentDisplay), 0);

  /*
   * If we used nxagentImageNormalize() here,
   * we'd swap our own cursor data in place.
   * Change byte_order and bitmap_bit_order
   * in the image struct to let Xlib do the
   * swap for us.
   */

  image -> byte_order = IMAGE_BYTE_ORDER;
  image -> bitmap_bit_order = BITMAP_BIT_ORDER;

  NXCleanImage(image);

  XPutImage(nxagentDisplay, source, nxagentBitmapGC, image,
                0, 0, 0, 0, pCursor->bits->width, pCursor->bits->height);

  XFree(image);

  image = XCreateImage(nxagentDisplay,
                       nxagentDefaultVisual(pScreen),
                       1, XYBitmap, 0,
                       (char *)pCursor->bits->mask,
                       pCursor->bits->width,
                       pCursor->bits->height,
                       BitmapPad(nxagentDisplay), 0);

  image -> byte_order = IMAGE_BYTE_ORDER;
  image -> bitmap_bit_order = BITMAP_BIT_ORDER;

  NXCleanImage(image);

  XPutImage(nxagentDisplay, mask, nxagentBitmapGC, image,
            0, 0, 0, 0, pCursor->bits->width, pCursor->bits->height);

  XFree(image);

  fg_color.red = pCursor->foreRed;
  fg_color.green = pCursor->foreGreen;
  fg_color.blue = pCursor->foreBlue;

  bg_color.red = pCursor->backRed;
  bg_color.green = pCursor->backGreen;
  bg_color.blue = pCursor->backBlue;

  pCursor->devPriv[pScreen->myNum] = (pointer) xalloc(sizeof(nxagentPrivCursor));

  nxagentCursorPriv(pCursor, pScreen)->cursor =
         XCreatePixmapCursor(nxagentDisplay, source, mask, &fg_color,
                                 &bg_color, pCursor->bits->xhot, pCursor->bits->yhot);

  nxagentCursorUsesRender(pCursor, pScreen) = 0;

  #ifdef TEST
  fprintf(stderr, "nxagentRealizeCursor: Set cursor private at [%p] cursor is [%ld].\n",
              (void *) nxagentCursorPriv(pCursor, pScreen),
                  nxagentCursorPriv(pCursor, pScreen) -> cursor);
  #endif

  XFreePixmap(nxagentDisplay, source);
  XFreePixmap(nxagentDisplay, mask);

  return True;
}

Bool nxagentUnrealizeCursor(ScreenPtr pScreen, CursorPtr pCursor)
{
  if (nxagentCursorUsesRender(pCursor, pScreen))
  {
    PicturePtr pPicture = nxagentCursorPicture(pCursor, pScreen);

    FreePicture(pPicture, pPicture -> id);
  }

  if (nxagentCursor(pCursor, pScreen) != None)
  {
    XFreeCursor(nxagentDisplay, nxagentCursor(pCursor, pScreen));

    nxagentCursor(pCursor, pScreen) = None;
  }

  xfree(nxagentCursorPriv(pCursor, pScreen));

  return True;
}

void nxagentRecolorCursor(ScreenPtr pScreen, CursorPtr pCursor,
                              Bool displayed)
{
  XColor fg_color, bg_color;

  fg_color.red = pCursor->foreRed;
  fg_color.green = pCursor->foreGreen;
  fg_color.blue = pCursor->foreBlue;

  bg_color.red = pCursor->backRed;
  bg_color.green = pCursor->backGreen;
  bg_color.blue = pCursor->backBlue;

  XRecolorCursor(nxagentDisplay,
                 nxagentCursor(pCursor, pScreen),
                 &fg_color, &bg_color);
}

Bool (*nxagentSetCursorPositionW)(ScreenPtr pScreen, int x, int y,
                                      Bool generateEvent);

Bool nxagentSetCursorPosition(ScreenPtr pScreen, int x, int y,
                                  Bool generateEvent)
{
  if (generateEvent != 0)
  {
    return (*nxagentSetCursorPositionW)(pScreen, x, y, generateEvent);
  }
  else
  {
    /*
     * Calling miSetCursorPosition with generateEvent == 0
     * causes a crash in miPoiterUpdate().
     */

    return 1;
  }
}

void nxagentReconnectCursor(pointer p0, XID x1, pointer p2)
{
  Bool* pBool = (Bool*)p2;
  CursorPtr pCursor = (CursorPtr) p0;

  AnimCurPtr ac;
  int j;

  #ifdef TEST
  fprintf(stderr, "nxagentReconnectCursor:  pCursor at [%p]\n", pCursor);
  #endif

  #ifdef NXAGENT_RECONNECT_CURSOR_DEBUG
  fprintf(stderr, "nxagentReconnectCursor:  pCursor at [%p]\n", pCursor);
  #endif

  if (!*pBool || !pCursor)
  {
    return;
  }

  if (nxagentCursorPriv(pCursor, nxagentDefaultScreen) == 0)
  {
    if (nxagentIsAnimCursor(pCursor))
    {
      #ifdef TEST
      fprintf(stderr, "nxagentReconnectCursor: nxagentIsAnimCursor   pCursor at [%p]\n", pCursor);
      #endif

      ac = nxagentGetAnimCursor(pCursor);

      for (j = 0; j < ac->nelt; j++)
      {
        nxagentReconnectCursor (ac->elts[j].pCursor, x1, p2);

        #ifdef TEST
        fprintf(stderr, "nxagentReconnectCursor: Iteration [%d]   pCursor at [%p]\n", j, ac->elts[j].pCursor);
        #endif
      }
    }
  }

  else
  {
    if (nxagentCursorUsesRender(pCursor, nxagentDefaultScreen))
    {
      PicturePtr pPicture = nxagentCursorPicture(pCursor, nxagentDefaultScreen);
      int ret = 1;

      nxagentReconnectPicture(pPicture, 0, &ret);

      nxagentRenderRealizeCursor(nxagentDefaultScreen, pCursor);
    }
    else
    {
      xfree(nxagentCursorPriv(pCursor, nxagentDefaultScreen));
      if (!nxagentRealizeCursor(nxagentDefaultScreen, pCursor))
      {
        fprintf(stderr, "nxagentReconnectCursor: nxagentRealizeCursor failed\n");
        *pBool = False;
      }
    }
  }

  #ifdef NXAGENT_RECONNECT_CURSOR_DEBUG
  fprintf(stderr, "nxagentReconnectCursor: %p - ID %lx\n", pCursor, nxagentCursor(pCursor, nxagentDefaultScreen));
  #endif
}

/*
 * The parameter is ignored at the moment.
 */

void nxagentReDisplayCurrentCursor()
{
  CursorPtr pCursor = GetSpriteCursor();

  if (pCursor &&
          nxagentCursorPriv(pCursor, nxagentDefaultScreen) &&
              nxagentCursor(pCursor, nxagentDefaultScreen))
  {
    nxagentDisplayCursor(nxagentDefaultScreen, pCursor);
  }
}

Bool nxagentReconnectAllCursor(void *p0)
{
  int i;
  Bool r = True;

  GrabPtr grab = inputInfo.pointer -> grab;

  #if defined(NXAGENT_RECONNECT_DEBUG) || defined(NXAGENT_RECONNECT_CURSOR_DEBUG)
  fprintf(stderr, "nxagentReconnectAllCursor\n");
  #endif

  for (i = 0, r = 1; i < MAXCLIENTS; r = 1, i++)
  {
    if (clients[i])
    {
      FindClientResourcesByType(clients[i], RT_CURSOR, nxagentReconnectCursor, &r);

      #ifdef WARNING

      if (r == False)
      {
        fprintf(stderr, "nxagentReconnectAllCursor: WARNING! Failed to recreate "
                    "cursor for client [%d].\n", i);
      }

      #endif
    }
  }

  if (grab)
  {
    nxagentReconnectCursor(grab -> cursor, 0, &r);
  }

  return r;
}

void nxagentDisconnectCursor(pointer p0, XID x1, pointer p2)
{
  Bool* pBool = (Bool *) p2;
  CursorPtr pCursor = (CursorPtr) p0;

  AnimCurPtr ac;
  int j;

  if (!*pBool || !pCursor)
  {
    return;
  }

  if (nxagentCursorPriv(pCursor, nxagentDefaultScreen) == 0)
  {
    if (nxagentIsAnimCursor(pCursor))
    {
      #ifdef TEST
      fprintf(stderr, "nxagentDisconnectCursor: nxagentIsAnimCursor   pCursor at [%p]\n", pCursor);
      #endif

      ac = nxagentGetAnimCursor(pCursor);

      for (j = 0; j < ac->nelt; j++)
      {
        nxagentDisconnectCursor (ac->elts[j].pCursor, x1, p2);

        #ifdef TEST
        fprintf(stderr, "nxagentDisconnectCursor: Iteration [%d]   pCursor at [%p]\n", j, ac->elts[j].pCursor);
        #endif
      }
    }
    return;
  }

  #ifdef NXAGENT_RECONNECT_CURSOR_DEBUG
  fprintf(stderr, "nxagentDisconnectCursor: %p - ID %lx\n",
          pCursor,
          nxagentCursor(pCursor, nxagentDefaultScreen));
  #endif

  #ifdef TEST
  fprintf(stderr, "nxagentDisconnectCursor: Called with bool [%d].\n", *pBool);

  fprintf(stderr, "nxagentDisconnectCursor: Pointer to cursor is [%p] with counter [%d].\n",
              (void *) pCursor, pCursor -> refcnt);

  fprintf(stderr, "nxagentDisconnectCursor: Dummy screen is at [%p].\n",
              (void *) nxagentDefaultScreen);

  fprintf(stderr, "nxagentDisconnectCursor: Cursor private is at [%p].\n",
              (void *) nxagentCursorPriv(pCursor, nxagentDefaultScreen));
  #endif

  #ifdef TEST
  fprintf(stderr, "nxagentDisconnectCursor: Dummy screen number is [%d].\n",
              nxagentDefaultScreen -> myNum);

  fprintf(stderr, "nxagentDisconnectCursor: Cursor is [%ld].\n",
              nxagentCursor(pCursor, nxagentDefaultScreen));
  #endif

  nxagentCursor(pCursor, nxagentDefaultScreen) = None;

  if (nxagentCursorUsesRender(pCursor, nxagentDefaultScreen))
  {
    PicturePtr pPicture = nxagentCursorPicture(pCursor, nxagentDefaultScreen);
    int        ret = 1;

    #if defined(NXAGENT_RECONNECT_CURSOR_DEBUG) || defined(NXAGENT_RECONNECT_PICTURE_DEBUG)
    fprintf(stderr, "nxagentDisconnectCursor: disconnecting attached picture %p\n", pPicture);
    #endif

    nxagentDisconnectPicture(pPicture, 0, &ret);
  }
}

Bool nxagentDisconnectAllCursor()
{
  int i;
  Bool r = True;

  GrabPtr grab = inputInfo.pointer -> grab;

  #ifdef TEST
  fprintf(stderr, "nxagentDisconnectAllCursor: Going to iterate through cursor resources.\n");
  #endif

  for (i = 0, r = 1; i < MAXCLIENTS; r = 1, i++)
  {
    if (clients[i])
    {
      FindClientResourcesByType(clients[i], RT_CURSOR, nxagentDisconnectCursor, &r);

      #ifdef WARNING

      if (r == False)
      {
        fprintf(stderr, "nxagentDisconnectAllCursor: WARNING! Failed to disconnect "
                    "cursor for client [%d].\n", i);
      }

      #endif
    }
  }

  if (grab)
  {
    nxagentDisconnectCursor(grab -> cursor, 0, &r);
  }

  return r;
}


#ifdef NXAGENT_RECONNECT_CURSOR_DEBUG

void nxagentPrintCursorInfo(CursorPtr pCursor, char msg[])
{
  fprintf(stderr, "%s: %p - ID %lx - ref count %d\n",
           msg, pCursor, nxagentCursor(pCursor, nxagentDefaultScreen), pCursor->refcnt);
}

void nxagentListCursor(void *p0, void *p1, void *p2)
{
  CursorPtr pCursor = (CursorPtr)p0;

  nxagentPrintCursorInfo(pCursor, "CursorDebug:");
}

void nxagentListCursors(void)
{
  int i;
  Bool r;

  for (i = 0, r = 1; i < MAXCLIENTS; r = 1, i++)
  {
    if (clients[i])
    {
      FindClientResourcesByType(clients[i], RT_CURSOR, nxagentListCursor, &r);

      #ifdef WARNING

      if (r == False)
      {
        fprintf(stderr, "nxagentListCursors: WARNING! Failed to list "
                    "cursor for client [%d].\n", i);
      }

      #endif
    }
  }

  return True;
}

#endif /* NXAGENT_RECONNECT_CURSOR_DEBUG */
