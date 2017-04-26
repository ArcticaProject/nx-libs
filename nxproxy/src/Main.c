/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2011 NoMachine, http://www.nomachine.com/.         */
/*                                                                        */
/* NXPROXY, NX protocol compression and NX extensions to this software    */
/* are copyright of NoMachine. Redistribution and use of the present      */
/* software is allowed according to terms specified in the file LICENSE   */
/* which comes in the source distribution.                                */
/*                                                                        */
/* Check http://www.nomachine.com/licensing.html for applicability.       */
/*                                                                        */
/* NX and NoMachine are trademarks of Medialogic S.p.A.                   */
/*                                                                        */
/* All rigths reserved.                                                   */
/*                                                                        */
/**************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>

#include "NX.h"

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

extern const char *__progname;

/*
 * Entry point when running nxproxy stand-alone.
 */

int main(int argc, const char **argv)
{
  int result = -1;

  char *options = NULL;

  char *nx_commfd_str = NULL;

  options = getenv("NX_DISPLAY");

  if ((nx_commfd_str = getenv("NX_COMMFD")) != NULL)
  {
    errno = 0;
    unsigned long int nx_commfd = strtoul(nx_commfd_str, NULL, 10);

    if ((errno) && (0 == nx_commfd))
    {
      fprintf(stderr, "%s: NX_COMMFD environment variable's value [%s] is not a file descriptor number. "
                      "Aborting...\n",
                      __progname, nx_commfd_str
                      );
      NXTransCleanup();
    }
    else if ((unsigned long int) INT_MAX < nx_commfd)
    {
      fprintf(stderr, "%s: NX_COMMFD environment variable's value [%lu] is out of range for a file descriptor number. "
                      "Aborting...\n",
                      __progname, nx_commfd);
      NXTransCleanup();
    }
    else {
      result = NXTransCreate(nx_commfd, NX_MODE_SERVER, options);

      if (result != 1)
      {
        fprintf(stderr, "%s: NXTransCreate failed for FD#%lu\n"
                        "Aborting...",
                        __progname, nx_commfd);
        NXTransCleanup();
      }

    }

    // go into endless loop

    if (result == 1)
    {
      while (NXTransRunning(NX_FD_ANY))
        result = NXTransContinue(NULL);
    }
  }
  else
  {

    if (NXTransParseCommandLine(argc, argv) < 0)
    {
      NXTransCleanup();
    }

    if (NXTransParseEnvironment(options, 0) < 0)
    {
      NXTransCleanup();
    }

    /*
     * This should not return...
     */

    #ifdef TEST
    fprintf(stderr, "%s: Yielding control to NX entry point.\n", __progname);
    #endif

    result = NXTransProxy(NX_FD_ANY, NX_MODE_ANY, NX_DISPLAY_ANY);
  }

  /*
   * ...So these should not be called.
   */

  NXTransExit(result);

  return 0;
}
