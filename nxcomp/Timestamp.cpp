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

//
// The following functions all use the ctime
// static buffer from the C library.
//

char *strTimestamp(const T_timestamp &ts)
{
  char *ctime_now = ctime((time_t *) &ts.tv_sec);

  ctime_now[24] = '\0';

  return ctime_now;
}

//
// This is especially dirty. 
//

char *strMsTimestamp(const T_timestamp &ts)
{
  char *ctime_now = ctime((time_t *) &ts.tv_sec);

  char ctime_new[25];

  sprintf(ctime_new, "%.8s:%3.3f", ctime_now + 11,
              (float) ts.tv_usec / 1000);

  strncpy(ctime_now, ctime_new, 24);

  return ctime_now;
}
