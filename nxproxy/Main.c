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

#include "NX.h"

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

/*
 * Entry point when running nxproxy stand-alone.
 */

int main(int argc, const char **argv)
{
  int result = -1;

  char *options = NULL;

  options = getenv("NX_DISPLAY");

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
  fprintf(stderr, "Main: Yielding control to NX entry point.\n");
  #endif

  result = NXTransProxy(NX_FD_ANY, NX_MODE_ANY, NX_DISPLAY_ANY);

  /*
   * ...So these should not be called.
   */

  NXTransExit(result);

  return 0;
}
