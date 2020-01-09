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
#include "Utils.h"

#include "windowstr.h"
#include "resource.h"

#include "compext/Compext.h"

/*
 * Set here the required log level.
 */

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

/*
 * Defined in Display.c. There are huge problems mixing the GC
 * definition in Xlib with the server code. This must be reworked.
 */

extern XlibGC nxagentBitmapGC;

/*
 * From NXevents.c.
 */

extern CursorPtr GetSpriteCursor(void);

void nxagentConstrainCursor(ScreenPtr pScreen, BoxPtr pBox)
{
  #ifdef TEST
  int width  = nxagentOption(RootWidth);
  int height = nxagentOption(RootHeight);

  if (pBox->x1 <= 0 && pBox->y1 <= 0 &&
          pBox->x2 >= width && pBox->y2 >= height)
  {
    fprintf(stderr, "%s: Called with box [%d,%d,%d,%d]. Skipping the operation.\n",
                __func__, pBox->x1, pBox->y1, pBox->x2, pBox->y2);
  }
  else
  {
    fprintf(stderr, "%s: WARNING! Called with box [%d,%d,%d,%d].\n", __func__,
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
   * Don't define the root cursor so that nxagent root window inherits
   * the parent's cursor.
   */

  Cursor cursor = (pCursor != rootCursor) ? nxagentCursor(pCursor, pScreen): None;

  if (nxagentOption(Rootless) == False)
  {
    XDefineCursor(nxagentDisplay,
                      nxagentInputWindows[pScreen -> myNum],
                          cursor);

    #ifdef TEST
    fprintf(stderr, "%s: Called for cursor at [%p] with private [%p].\n", __func__,
                (void *) pCursor, pCursor->devPriv[pScreen->myNum]);
    #endif
  }

  return True;
}

Bool nxagentRealizeCursor(ScreenPtr pScreen, CursorPtr pCursor)
{
  #ifdef TEST
  fprintf(stderr, "%s: Called for cursor at [%p].\n", __func__, (void *) pCursor);
  #endif

  unsigned long valuemask = GCFunction | GCPlaneMask | GCForeground | GCBackground | GCClipMask;

  XGCValues values = {
    .function = GXcopy,
    .plane_mask = AllPlanes,
    .foreground = 1L,
    .background = 0L,
    .clip_mask = None,
  };

  XChangeGC(nxagentDisplay, nxagentBitmapGC, valuemask, &values);

  Pixmap source = XCreatePixmap(nxagentDisplay,
                                nxagentDefaultWindows[pScreen->myNum],
                                pCursor->bits->width,
                                pCursor->bits->height,
                                1);

  Pixmap mask = XCreatePixmap(nxagentDisplay,
                              nxagentDefaultWindows[pScreen->myNum],
                              pCursor->bits->width,
                              pCursor->bits->height,
                              1);

  XImage *image = XCreateImage(nxagentDisplay,
                               nxagentDefaultVisual(pScreen),
                               1, XYBitmap, 0,
                               (char *)pCursor->bits->source,
                               pCursor->bits->width,
                               pCursor->bits->height,
                               BitmapPad(nxagentDisplay), 0);

  /*
   * If we used nxagentImageNormalize() here, we'd swap our own cursor
   * data in place.  Change byte_order and bitmap_bit_order in the
   * image struct to let Xlib do the swap for us.
   */

  image -> byte_order = IMAGE_BYTE_ORDER;
  image -> bitmap_bit_order = BITMAP_BIT_ORDER;

  NXCleanImage(image);

  XPutImage(nxagentDisplay, source, nxagentBitmapGC, image,
                0, 0, 0, 0, pCursor->bits->width, pCursor->bits->height);

  SAFE_XFree(image);

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

  SAFE_XFree(image);

  XColor fg_color = {
    .red = pCursor->foreRed,
    .green = pCursor->foreGreen,
    .blue = pCursor->foreBlue,
  };

  XColor bg_color = {
    .red = pCursor->backRed,
    .green = pCursor->backGreen,
    .blue = pCursor->backBlue,
  };

  pCursor->devPriv[pScreen->myNum] = (void *) malloc(sizeof(nxagentPrivCursor));

  nxagentCursorPriv(pCursor, pScreen)->cursor =
         XCreatePixmapCursor(nxagentDisplay, source, mask, &fg_color,
                                 &bg_color, pCursor->bits->xhot, pCursor->bits->yhot);

  nxagentCursorUsesRender(pCursor, pScreen) = 0;

  #ifdef TEST
  fprintf(stderr, "%s: Set cursor private at [%p] cursor is [%ld].\n", __func__,
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

  free(nxagentCursorPriv(pCursor, pScreen));

  return True;
}

void nxagentRecolorCursor(ScreenPtr pScreen, CursorPtr pCursor,
                              Bool displayed)
{
  XColor fg_color = {
    .red = pCursor->foreRed,
    .green = pCursor->foreGreen,
    .blue = pCursor->foreBlue,
  };

  XColor bg_color = {
    .red = pCursor->backRed,
    .green = pCursor->backGreen,
    .blue = pCursor->backBlue,
  };

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
     * Calling miSetCursorPosition with generateEvent == 0 causes a
     * crash in miPoiterUpdate().
     */
    return 1;
  }
}

void nxagentReconnectCursor(void * p0, XID x1, void * p2)
{
  Bool* pBool = (Bool*)p2;
  CursorPtr pCursor = (CursorPtr) p0;

  #if defined( TEST) || defined(NXAGENT_RECONNECT_CURSOR_DEBUG)
  fprintf(stderr, "%s:  pCursor at [%p]\n", __func__, pCursor);
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
      fprintf(stderr, "%s: nxagentIsAnimCursor   pCursor at [%p]\n", __func__, pCursor);
      #endif

      AnimCurPtr ac = nxagentGetAnimCursor(pCursor);

      for (int j = 0; j < ac->nelt; j++)
      {
        nxagentReconnectCursor (ac->elts[j].pCursor, x1, p2);

        #ifdef TEST
        fprintf(stderr, "%s: Iteration [%d]   pCursor at [%p]\n", __func__, j, ac->elts[j].pCursor);
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
      free(nxagentCursorPriv(pCursor, nxagentDefaultScreen));
      if (!nxagentRealizeCursor(nxagentDefaultScreen, pCursor))
      {
        fprintf(stderr, "%s: nxagentRealizeCursor failed\n", __func__);
        *pBool = False;
      }
    }
  }

  #ifdef NXAGENT_RECONNECT_CURSOR_DEBUG
  fprintf(stderr, "%s: %p - ID %lx\n", __func__, pCursor, nxagentCursor(pCursor, nxagentDefaultScreen));
  #endif
}

/*
 * The parameter is ignored at the moment.
 */

void nxagentReDisplayCurrentCursor(void)
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
  Bool r = True;

  GrabPtr grab = inputInfo.pointer -> grab;

  #if defined(NXAGENT_RECONNECT_DEBUG) || defined(NXAGENT_RECONNECT_CURSOR_DEBUG)
  fprintf(stderr, "%s\n", __func__);
  #endif

  for (int i = 0; i < MAXCLIENTS; r = 1, i++)
  {
    if (clients[i])
    {
      FindClientResourcesByType(clients[i], RT_CURSOR, nxagentReconnectCursor, &r);

      #ifdef WARNING
      if (r == False)
      {
        fprintf(stderr, "%s: WARNING! Failed to recreate "
                    "cursor for client [%d].\n", __func__, i);
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

void nxagentDisconnectCursor(void * p0, XID x1, void * p2)
{
  Bool* pBool = (Bool *) p2;
  CursorPtr pCursor = (CursorPtr) p0;

  if (!*pBool || !pCursor)
  {
    return;
  }

  if (nxagentCursorPriv(pCursor, nxagentDefaultScreen) == 0)
  {
    if (nxagentIsAnimCursor(pCursor))
    {
      #ifdef TEST
      fprintf(stderr, "%s: nxagentIsAnimCursor   pCursor at [%p]\n", __func__, pCursor);
      #endif

      AnimCurPtr ac = nxagentGetAnimCursor(pCursor);

      for (int j = 0; j < ac->nelt; j++)
      {
        nxagentDisconnectCursor (ac->elts[j].pCursor, x1, p2);

        #ifdef TEST
        fprintf(stderr, "%s: Iteration [%d]   pCursor at [%p]\n", __func__, j, ac->elts[j].pCursor);
        #endif
      }
    }
    return;
  }

  #ifdef NXAGENT_RECONNECT_CURSOR_DEBUG
  fprintf(stderr, "%s: %p - ID %lx\n", __func__, pCursor,
              nxagentCursor(pCursor, nxagentDefaultScreen));
  #endif

  #ifdef TEST
  fprintf(stderr, "%s: Called with bool [%d].\n", __func__, *pBool);

  fprintf(stderr, "%s: Pointer to cursor is [%p] with counter [%d].\n", __func__,
              (void *) pCursor, pCursor -> refcnt);

  fprintf(stderr, "%s: Dummy screen is at [%p].\n", __func__,
              (void *) nxagentDefaultScreen);

  fprintf(stderr, "%s: Cursor private is at [%p].\n", __func__,
              (void *) nxagentCursorPriv(pCursor, nxagentDefaultScreen));
  #endif

  #ifdef TEST
  fprintf(stderr, "%s: Dummy screen number is [%d].\n", __func__,
              nxagentDefaultScreen -> myNum);

  fprintf(stderr, "%s: Cursor is [%ld].\n", __func__,
              nxagentCursor(pCursor, nxagentDefaultScreen));
  #endif

  nxagentCursor(pCursor, nxagentDefaultScreen) = None;

  if (nxagentCursorUsesRender(pCursor, nxagentDefaultScreen))
  {
    PicturePtr pPicture = nxagentCursorPicture(pCursor, nxagentDefaultScreen);
    int        ret = 1;

    #if defined(NXAGENT_RECONNECT_CURSOR_DEBUG) || defined(NXAGENT_RECONNECT_PICTURE_DEBUG)
    fprintf(stderr, "%s: disconnecting attached picture %p\n", __func__, pPicture);
    #endif

    nxagentDisconnectPicture(pPicture, 0, &ret);
  }
}

void nxagentDisconnectAllCursor(void)
{
  Bool r = True;

  GrabPtr grab = inputInfo.pointer -> grab;

  #ifdef TEST
  fprintf(stderr, "%s: Going to iterate through cursor resources.\n", __func__);
  #endif

  for (int i = 0; i < MAXCLIENTS; r = 1, i++)
  {
    if (clients[i])
    {
      FindClientResourcesByType(clients[i], RT_CURSOR, nxagentDisconnectCursor, &r);

      #ifdef WARNING
      if (r == False)
      {
        fprintf(stderr, "%s: WARNING! Failed to disconnect "
                    "cursor for client [%d].\n", __func__, i);
      }
      #endif
    }
  }

  if (grab)
  {
    nxagentDisconnectCursor(grab -> cursor, 0, &r);
  }

  return;
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
  Bool r = True;

  for (int i = 0; i < MAXCLIENTS; r = 1, i++)
  {
    if (clients[i])
    {
      FindClientResourcesByType(clients[i], RT_CURSOR, nxagentListCursor, &r);

      #ifdef WARNING
      if (r == False)
      {
        fprintf(stderr, "%s: WARNING! Failed to list cursor for client [%d].\n", __func__, i);
      }
      #endif
    }
  }

  return True;
}

#endif /* NXAGENT_RECONNECT_CURSOR_DEBUG */
