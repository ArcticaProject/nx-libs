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

#include <time.h>
#include <stdio.h>

#include <X11/Xos.h>
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
