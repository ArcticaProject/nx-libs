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

Copyright 1987, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from The Open Group.


Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,

			All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

*/

/* The panoramix components contained the following notice */
/*****************************************************************

Copyright (c) 1991, 1997 Digital Equipment Corporation, Maynard, Massachusetts.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
DIGITAL EQUIPMENT CORPORATION BE LIABLE FOR ANY CLAIM, DAMAGES, INCLUDING,
BUT NOT LIMITED TO CONSEQUENTIAL OR INCIDENTAL DAMAGES, OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Digital Equipment Corporation
shall not be used in advertising or otherwise to promote the sale, use or other
dealings in this Software without prior written authorization from Digital
Equipment Corporation.

******************************************************************/


#include "selection.h"

#include "Screen.h"
#include "Options.h"
#include "Atoms.h"
#include "Clipboard.h"
#include "Splash.h"
#include "Rootless.h"
#include "Composite.h"
#include "Drawable.h"
#include "Colormap.h"

#include "../../dix/window.c"

extern Bool nxagentWMIsRunning;
extern Bool nxagentScreenTrap;

/*
 * Set here the required log level.
 */

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

extern void nxagentSetVersionProperty(WindowPtr pWin);

void
InitRootWindow(WindowPtr pWin)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;
    int backFlag = CWBorderPixel | CWCursor | CWBackingStore;

    #ifdef TEST
    fprintf(stderr, "InitRootWindow: Called for window at [%p][%ld] with parent [%p].\n",
                (void *) pWin, nxagentWindowPriv(pWin)->window, (void *) pWin -> parent);
    #endif

    if (nxagentOption(Rootless))
    {
      #ifdef TEST
      fprintf(stderr, "InitRootWindow: Assigned agent root to window at [%p][%ld] with parent [%p].\n",
                  (void *) pWin, nxagentWindowPriv(pWin)->window, (void *) pWin -> parent);
      #endif

      nxagentRootlessWindow = pWin;
    }

    /*
     * A root window is created for each screen by main
     * and the pointer is saved in screenInfo.screens as
     * in the following snippet:
     *
     * for (i = 0; i < screenInfo.numScreens; i++)
     *          InitRootWindow(screenInfo.screens[i]->root);
     *
     * Our root window on the real display was already
     * created at the time the screen was opened, so it
     * is unclear how this window (or the other window,
     * if you prefer) fits in the big picture.
     */

    #ifdef TEST
    fprintf(stderr, "InitRootWindow: Going to create window as root at [%p][%ld] with parent [%p].\n",
                (void *) pWin, nxagentWindowPriv(pWin)->window, (void *) pWin -> parent);
    #endif

    if (!(*pScreen->CreateWindow)(pWin))
	return; /* XXX */

    #ifdef TEST
    fprintf(stderr, "InitRootWindow: Created window as root at [%p][%ld] with parent [%p].\n",
                (void *) pWin, nxagentWindowPriv(pWin)->window, (void *) pWin -> parent);
    #endif

    (*pScreen->PositionWindow)(pWin, 0, 0);

    pWin->cursorIsNone = FALSE;
    pWin->optional->cursor = rootCursor;
    rootCursor->refcnt++;

    if (blackRoot)
      pWin->background.pixel = pScreen->blackPixel;
    else
      pWin->background.pixel = pScreen->whitePixel;
    backFlag |= CWBackPixel;

    pWin->backingStore = defaultBackingStore;
    pWin->forcedBS = (defaultBackingStore != NotUseful);

    /* We SHOULD check for an error value here XXX */
    (*pScreen->ChangeWindowAttributes)(pWin, backFlag);

    /*
     * Map both the root and the default agent window.
     */

    #ifdef TEST
    fprintf(stderr, "InitRootWindow: Mapping default windows.\n");
    #endif

    nxagentInitAtoms(pWin);

    nxagentInitClipboard(pWin);

    nxagentMapDefaultWindows();

    nxagentRedirectDefaultWindows();

    #ifdef NXAGENT_ARTSD
    {
      char artsd_port[10];
      short int nPort;
      extern void nxagentPropagateArtsdProperties(ScreenPtr pScreen, char *port);
      nPort = atoi(display) + 7000;
      sprintf(artsd_port,"%d", nPort);
      nxagentPropagateArtsdProperties(pScreen, artsd_port);
    }
    #endif

    nxagentSetVersionProperty(pWin);
}

int
MapWindow(register WindowPtr pWin, ClientPtr client)
{
    #ifdef TEST
    if (nxagentWindowTopLevel(pWin))
    {
      fprintf(stderr, "MapWindow: pWin [%p] client [%p]\n", pWin, client);
    }
    #endif

    return xorg_MapWindow(pWin, client);
}

int
UnmapWindow(register WindowPtr pWin, Bool fromConfigure)
{
    #ifdef TEST
    if (nxagentWindowTopLevel(pWin))
    {
      fprintf(stderr, "UnmapWindow: pWin [%p] fromConfigure [%d]\n", pWin,
                  fromConfigure);
    }
    #endif

    return xorg_UnmapWindow(pWin, fromConfigure);
}

void
SaveScreens(int on, int mode)
{
    int i;
    int what;
    int type;

    if (on == SCREEN_SAVER_FORCER)
    {
	if (mode == ScreenSaverReset)
	    what = SCREEN_SAVER_OFF;
	else
	    what = SCREEN_SAVER_ON;
	type = what;
    }
    else
    {
	what = on;
	type = what;
	if (what == screenIsSaved)
	    type = SCREEN_SAVER_CYCLE;
    }
    for (i = 0; i < screenInfo.numScreens; i++)
    {
	if (on == SCREEN_SAVER_FORCER)
	   (* screenInfo.screens[i]->SaveScreen) (screenInfo.screens[i], on);
	if (savedScreenInfo[i].ExternalScreenSaver)
	{
          if (nxagentOption(Timeout) != 0)
          {
            #ifdef TEST
            fprintf(stderr, "SaveScreens: An external screen-saver handler is installed. "
                        "Ignoring it to let the auto-disconnect feature work.\n");
            #endif
          }
          else
          {
	      if ((*savedScreenInfo[i].ExternalScreenSaver)
		  (screenInfo.screens[i], type, on == SCREEN_SAVER_FORCER))
		  continue;
          }
	}
	if (type == screenIsSaved)
	    continue;
	switch (type) {
	case SCREEN_SAVER_OFF:
	    if (savedScreenInfo[i].blanked == SCREEN_IS_BLANKED)
	    {
	       (* screenInfo.screens[i]->SaveScreen) (screenInfo.screens[i],
						      what);
	    }
	    else if (HasSaverWindow (i))
	    {
		savedScreenInfo[i].pWindow = NullWindow;
		FreeResource(savedScreenInfo[i].wid, RT_NONE);
	    }
	    break;
	case SCREEN_SAVER_CYCLE:
	    if (savedScreenInfo[i].blanked == SCREEN_IS_TILED)
	    {
		WindowPtr pWin = savedScreenInfo[i].pWindow;
		/* make it look like screen saver is off, so that
		 * NotClippedByChildren will compute a clip list
		 * for the root window, so miPaintWindow works
		 */
		screenIsSaved = SCREEN_SAVER_OFF;
#ifndef NOLOGOHACK
		if (logoScreenSaver)
		    (*pWin->drawable.pScreen->ClearToBackground)(pWin, 0, 0, 0, 0, FALSE);
#endif
		(*pWin->drawable.pScreen->MoveWindow)(pWin,
			   (short)(-(rand() % RANDOM_WIDTH)),
			   (short)(-(rand() % RANDOM_WIDTH)),
			   pWin->nextSib, VTMove);
#ifndef NOLOGOHACK
		if (logoScreenSaver)
		    DrawLogo(pWin);
#endif
		screenIsSaved = SCREEN_SAVER_ON;
	    }
	    /*
	     * Call the DDX saver in case it wants to do something
	     * at cycle time
	     */
	    else if (savedScreenInfo[i].blanked == SCREEN_IS_BLANKED)
	    {
		(* screenInfo.screens[i]->SaveScreen) (screenInfo.screens[i],
						       type);
	    }
	    break;
	case SCREEN_SAVER_ON:
	    if (ScreenSaverBlanking != DontPreferBlanking)
	    {
		if ((* screenInfo.screens[i]->SaveScreen)
		   (screenInfo.screens[i], what))
		{
		   savedScreenInfo[i].blanked = SCREEN_IS_BLANKED;
		   continue;
		}
		if ((ScreenSaverAllowExposures != DontAllowExposures) &&
		    TileScreenSaver(i, SCREEN_IS_BLACK))
		{
		    savedScreenInfo[i].blanked = SCREEN_IS_BLACK;
		    continue;
		}
	    }
	    if ((ScreenSaverAllowExposures != DontAllowExposures) &&
		TileScreenSaver(i, SCREEN_IS_TILED))
	    {
		savedScreenInfo[i].blanked = SCREEN_IS_TILED;
	    }
	    else
		savedScreenInfo[i].blanked = SCREEN_ISNT_SAVED;
	    break;
	}
    }
    screenIsSaved = what;
    if (mode == ScreenSaverReset)
       SetScreenSaverTimer();
}
