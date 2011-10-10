/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2011 NoMachine, http://www.nomachine.com/.         */
/*                                                                        */
/* NXAGENT, NX protocol compression and NX extensions to this software    */
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

#include <time.h>
#include <stdio.h>

#include "Xos.h"
#include "Millis.h"

#ifdef DDXTIME

CARD32 GetTimeInMillis()
{
  struct timeval ts;

  X_GETTIMEOFDAY(&ts);

  return(ts.tv_sec * 1000) + (ts.tv_usec / 1000);
}

#endif

const char *GetTimeAsString()
{
  char *value;

  struct timeval ts;

  X_GETTIMEOFDAY(&ts);

  value = ctime((time_t *) &ts.tv_sec);

  *(value + strlen(value) - 1) = '\0';

  return value;
}

const char *GetTimeInMillisAsString()
{
  char *value;

  char tb[25];

  struct timeval ts;

  X_GETTIMEOFDAY(&ts);

  value = ctime((time_t *) &ts.tv_sec);

  sprintf(tb, "%.8s:%3.3f", value + 11,
              (float) ts.tv_usec / 1000);

  strncpy(value, tb, 24);

  return value;
}
