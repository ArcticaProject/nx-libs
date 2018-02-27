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

#include "scrnintstr.h"
#include "windowstr.h"

#include "Agent.h"
#include "Screen.h"
#include "Display.h"
#include "Options.h"
#include "Windows.h"

#include "X11/include/Xcomposite_nxagent.h"

/*
 * Set here the required log level.
 */

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

/*
 * Set if the composite extension is supported
 * by the remote display.
 */

int nxagentCompositeEnable = UNDEFINED;

void nxagentCompositeExtensionInit(void)
{
  /*
   * Set the flag only if the initialization
   * completes.
   */

  nxagentCompositeEnable = 0;

  if (nxagentOption(Composite) == 1)
  {
    int eventBase, errorBase;

    #ifdef TEST
    fprintf(stderr, "nxagentCompositeExtensionInit: Checking if the composite extension is supported.\n");
    #endif

    if (XCompositeQueryExtension(nxagentDisplay, &eventBase, &errorBase) == 1)
    {
      /*
       * At the moment we don't need to care
       * the version of the extension.
       */

      #ifdef TEST

      int major = -1;
      int minor = -1;

      XCompositeQueryVersion(nxagentDisplay, &major, &minor);

      fprintf(stderr, "nxagentCompositeExtensionInit: The remote display supports version [%d] "
                  "minor [%d].\n", major, minor);

      if (major < 0 || minor < 2)
      {
        #ifdef WARNING
        fprintf(stderr, "nxagentCompositeExtensionInit: WARNING! Potentially incompatible version "
                    "[%d] minor [%d] detected.\n", major, minor);
        #endif
      }

      #endif

      #ifdef TEST
      fprintf(stderr, "nxagentCompositeExtensionInit: Enabling the use of the composite extension.\n");
      #endif

      nxagentCompositeEnable = 1;
    }
    #ifdef TEST
    else
    {
      fprintf(stderr, "nxagentCompositeExtensionInit: Composite extension not supported on this display.\n");
    }
    #endif
  }
  #ifdef TEST
  else
  {
    fprintf(stderr, "nxagentCompositeExtensionInit: Use of the composite extension not enabled.\n");
  }
  #endif
}

void nxagentRedirectDefaultWindows(void)
{
  int i;

  if (nxagentOption(Rootless) == 1 ||
          nxagentCompositeEnable == 0)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentRedirectDefaultWindows: Not redirecting default "
                "windows with rootless mode [%d] and composite [%d].\n",
                    nxagentOption(Rootless), nxagentCompositeEnable);
    #endif

    return;
  }

  for (i = 0; i < screenInfo.numScreens; i++)
  {
    WindowPtr pWin = screenInfo.screens[i]->root;

    ScreenPtr pScreen = pWin -> drawable.pScreen;

    #ifdef TEST
    fprintf(stderr, "nxagentRedirectDefaultWindows: WARNING! Redirecting default window id [%ld] "
                "to off-screen memory.\n", (long int)nxagentDefaultWindows[pScreen->myNum]);
    #endif

    /*
     * When trying to redirect only the top level window,
     * and not the subwindows, we incur in a strange be-
     * haviour. The top level is unmapped, mapped, unmap-
     * ped and then reparented. This at first makes the
     * agent think that the window manager is gone, then
     * the agent window disappears. To make thinks even
     * more weird, this happens only at reconnection.
     */
 
    XCompositeRedirectSubwindows(nxagentDisplay, nxagentDefaultWindows[pScreen->myNum],
                                     CompositeRedirectAutomatic);
  }
}

void nxagentRedirectWindow(WindowPtr pWin)
{
  if (nxagentOption(Rootless) == 0 ||
          nxagentCompositeEnable == 0)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentRedirectWindow: Not redirecting window id [%ld] "
                "to off-screen memory with rootless mode [%d] and composite [%d].\n",
                    nxagentWindow(pWin), nxagentOption(Rootless),
                        nxagentCompositeEnable);
    #endif

    return;
  }

  #ifdef TEST
  fprintf(stderr, "nxagentRedirectWindow: WARNING! Redirecting window id [%ld] "
              "to off-screen memory.\n", (long int)nxagentWindow(pWin));
  #endif

  XCompositeRedirectWindow(nxagentDisplay, nxagentWindow(pWin),
                               CompositeRedirectAutomatic);

  nxagentWindowPriv(pWin) -> isRedirected = 1;
}

void nxagentUnredirectWindow(WindowPtr pWin)
{
  if (nxagentWindowPriv(pWin) -> isRedirected == 1)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentUnredirectWindow: Disabling redirection of window id [%ld] "
                "to off-screen memory.\n", nxagentWindow(pWin));
    #endif

    XCompositeUnredirectWindow(nxagentDisplay, nxagentWindow(pWin),
                                   CompositeRedirectAutomatic);

    nxagentWindowPriv(pWin) -> isRedirected = 0;
  }
  #ifdef WARNING
  else
  {
    fprintf(stderr, "nxagentUnredirectWindow: WARNING! The window id [%ld] "
                "was not redirected.\n", (long int)nxagentWindow(pWin));
  }
  #endif
}
