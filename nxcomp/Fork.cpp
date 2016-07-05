/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2011 NoMachine (http://www.nomachine.com)          */
/* Copyright (c) 2008-2014 Oleksandr Shneyder <o.shneyder@phoca-gmbh.de>  */
/* Copyright (c) 2014-2016 Ulrich Sibiller <uli42@gmx.de>                 */
/* Copyright (c) 2014-2016 Mihai Moldovan <ionic@ionic.de>                */
/* Copyright (c) 2011-2016 Mike Gabriel <mike.gabriel@das-netzwerkteam.de>*/
/* Copyright (c) 2015-2016 Qindel Group (http://www.qindel.com)           */
/*                                                                        */
/* NXCOMP, NX protocol compression and NX extensions to this software     */
/* are copyright of the aforementioned persons and companies.             */
/*                                                                        */
/* Redistribution and use of the present software is allowed according    */
/* to terms specified in the file LICENSE.nxcomp which comes in the       */
/* source distribution.                                                   */
/*                                                                        */
/* All rights reserved.                                                   */
/*                                                                        */
/* NOTE: This software has received contributions from various other      */
/* contributors, only the core maintainers and supporters are listed as   */
/* copyright holders. Please contact us, if you feel you should be listed */
/* as copyright holder, as well.                                          */
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
