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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "Timestamp.h"

//
// Log level.
//

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

//
// Last timestamp taken from the system.
//

T_timestamp timestamp;

std::string strTimestamp(const T_timestamp &ts)
{
  std::string ret;

  char ctime_now[26] = { };
  bool err = true;

#if HAVE_CTIME_S
  errno_t retval = ::ctime_s(ctime_now, sizeof(ctime_now), static_cast<const time_t*>(&ts.tv_sec));

  if (retval != 0)
#else
  char *retval = ::ctime_r(static_cast<const time_t*>(&ts.tv_sec), ctime_now);

  if (!(retval))
#endif
  {
    std::cerr << "WARNING: converting time to string failed." << std::endl;
  }
  else
  {
    /* Replace newline at position 25 with a NULL byte. */
    ctime_now[24] = '\0';

    ret = ctime_now;
  }

  return ret;
}

//
// This is especially dirty.
//

std::string strMsTimestamp(const T_timestamp &ts)
{
  std::string ret;

  std::string ctime_now = strTimestamp(ts);

  if (!(ctime_now.empty()))
  {
    char ctime_new[26] = { };

    snprintf(ctime_new, sizeof(ctime_new), "%.8s:%3.3f",
             ctime_now.c_str() + 11, static_cast<float>(ts.tv_usec) / 1000);

    ret = ctime_new;
  }

  return ret;
}
