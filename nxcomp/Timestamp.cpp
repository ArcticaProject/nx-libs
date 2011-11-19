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
