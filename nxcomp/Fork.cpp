/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2010 NoMachine, http://www.nomachine.com/.         */
/*                                                                        */
/* NXCOMP, NX protocol compression and NX extensions to this software     */
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

#include <unistd.h>

#include "Fork.h"
#include "Misc.h"
#include "Timestamp.h"

//
// Set the verbosity level.
//

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

//
// Only on Cygwin, retry n times waiting a
// given amount of milliseconds after each
// attempt.
//

#define RETRY_LIMIT    30
#define RETRY_TIMEOUT  1000

int Fork()
{
  #ifdef __CYGWIN32__

  int limit   = RETRY_LIMIT;
  int timeout = RETRY_TIMEOUT;

  #else

  int limit   = 1;
  int timeout = 0;

  #endif

  int pid = 0;

  for (int i = 0; i < limit; i++)
  {
    #ifdef TEST
    *logofs << "Fork: Trying at " << strMsTimestamp()
            << ".\n" << logofs_flush;
    #endif

    //
    // It could optionally try again only if the
    // error code is 11, 'Resource temporarily
    // unavailable'.
    //

    if ((pid = fork()) >= 0)
    {
      break;
    }
    else if (i < limit - 1)
    {
      #ifdef WARNING
      *logofs << "Fork: WARNING! Function fork failed. "
              << "Error is " << EGET() << " '" << ESTR()
              << "'. Retrying...\n" << logofs_flush;
      #endif

      usleep(timeout * 1000);
    }
  }

  #ifdef TEST

  if (pid <= 0)
  {
    *logofs << "Fork: Returning at " << strMsTimestamp()
            << ".\n" << logofs_flush;
  }

  #endif

  return pid;
}
