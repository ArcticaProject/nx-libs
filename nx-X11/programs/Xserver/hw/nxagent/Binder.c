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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <nx/NX.h>

#include "Binder.h"
#include "Options.h"

/*
 * Set here the required log level.
 */

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

int nxagentCheckBinder(int argc, char *argv[], int i)
{
  if (++i < argc)
  {
    char *display;
    char *found;

    int port;

    display = argv[i];

    /*
     * Check if a display specification follows
     * the -B switch.
     */

    found = rindex(display, ':');

    if (found == NULL || *(found + 1) == '\0' ||
            isdigit(*(found + 1)) == 0)
    {
      fprintf(stderr, "Error: Can't identify the display port in string '%s'.\n",
                  display);

      return 0;
    }

    port = atoi(found + 1);

    #ifdef TEST
    fprintf(stderr, "nxagentCheckBinder: Identified agent display port [%d].\n",
                port);
    #endif

    /*
     * The NX options must be specified in the DISPLAY
     * environment. Check if the display specified on
     * the command line matches the NX virtual display.
     */

    display = getenv("DISPLAY");

    if (display == NULL || *display == '\0')
    {
      fprintf(stderr, "Error: No DISPLAY environment found.\n");

      return 0;
    }

    found = rindex(display, ':');

    if (found == NULL || *(found + 1) == '\0' ||
            isdigit(*(found + 1)) == 0 || atoi(found + 1) != port)
    {
      fprintf(stderr, "Error: The NX display doesn't match the agent display :%d.\n",
                  port);

      return 0;
    }

    #ifdef TEST
    fprintf(stderr, "nxagentCheckBinder: Identified NX display port [%d].\n",
                atoi(found + 1));
    #endif

    /*
     * Save the proxy options. They will be later
     * used to create the transport.
     */

    nxagentChangeOption(Rootless, False);
    nxagentChangeOption(Desktop, False);
    nxagentChangeOption(Binder, True);

    /*
     * FIXME: This now points to the buffer that was
     * returned by getenv(). It is to be decided how
     * to handle the values of type string in the
     * Options repository.
     */
     
    nxagentChangeOption(BinderOptions, display);

    return 2;
  }

  fprintf(stderr, "Error: No display port specified in command.\n");

  return 0;
}

int nxagentBinderLoop(void)
{
  struct timeval timeout;

  char *options = nxagentOption(BinderOptions);

  #ifdef TEST
  fprintf(stderr, "nxagentBinderLoop: Creating the NX transport.\n");
  #endif

  if (NXTransCreate(NX_FD_ANY, NX_MODE_CLIENT, options) < 0)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentBinderLoop: PANIC! Error creating the NX transport.\n");
    #endif

    return -1;
  }

  #ifdef TEST
  fprintf(stderr, "nxagentBinderLoop: Yielding control to the NX entry point.\n");
  #endif

  while (NXTransRunning(NX_FD_ANY))
  {
    #ifdef DEBUG
    fprintf(stderr, "nxagentBinderLoop: Going to run a new NX proxy loop.\n");
    #endif

    timeout.tv_sec  = 10;
    timeout.tv_usec = 0;

    NXTransContinue(&timeout);

    #ifdef DEBUG
    fprintf(stderr, "nxagentBinderLoop: Completed execution of the NX loop.\n");
    #endif
  }

  #ifdef TEST
  fprintf(stderr, "nxagentBinderLoop: Exiting the NX proxy binder loop.\n");
  #endif

  return 1;
}

void nxagentBinderExit(int code)
{
  NXTransExit(code);
}
