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

#ifndef Timestamp_H
#define Timestamp_H

#if HAVE_CTIME_S
#define __STDC_WANT_LIB_EXT1__ 1
#include <time.h>
#endif /* HAVE_CTIME_S */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <string>

#include <sys/time.h>

#include "Misc.h"

//
// Log level.
//

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

//
// If not defined, always query the system time.
//

#undef  CACHE_TIMESTAMP

//
// Log a warning if the time difference since
// the last update exceeds the given number
// of milliseconds.
//

#define DRIFT_TIMESTAMP  1

//
// Type used for timeout manipulation.
//

typedef struct timeval T_timestamp;

//
// Last timestamp taken from the system. If the
// timestamp is cached, we need to explicitly
// get a new timestamp after any operation that
// may have required a relevant amount of time.
//

extern T_timestamp timestamp;

//
// Get a timestamp instance with values set
// at the given amount of milliseconds.
//

inline T_timestamp getTimestamp(long ms)
{
  struct timeval ts;

  ts.tv_sec  = ms / 1000;
  ts.tv_usec = (ms % 1000) * 1000;

  return ts;
}

//
// Return the difference in milliseconds
// between the two timestamps.
//

inline long diffTimestamp(const T_timestamp &ts1, const T_timestamp &ts2)
{
  //
  // Add 500 microseconds to round up
  // to the nearest millisecond.
  //

  return ((ts2.tv_sec * 1000 + (ts2.tv_usec + 500) / 1000) -
             (ts1.tv_sec * 1000 + (ts1.tv_usec + 500) / 1000));
}

//
// The same in microseconds. It doesn't
// round the value.
//

inline long diffUsTimestamp(const T_timestamp &ts1, const T_timestamp &ts2)
{
  return ((ts2.tv_sec * 1000000 + ts2.tv_usec) -
             (ts1.tv_sec * 1000000 + ts1.tv_usec));
}

//
// Return the last timestamp taken from the
// system. It doesn't update the timestamp.
//

inline T_timestamp getTimestamp()
{
  #ifdef CACHE_TIMESTAMP

  #ifdef TEST

  T_timestamp ts;

  gettimeofday(&ts, NULL);

  long diffTs = diffTimestamp(timestamp, ts);

  if (diffTs > DRIFT_TIMESTAMP)
  {
    *logofs << "Timestamp: WARNING! Time difference since the "
            << "current timestamp is " << diffTs << "ms.\n"
            << logofs_flush;
  }

  #endif

  return timestamp;

  #else

  gettimeofday(&timestamp, NULL);

  return timestamp;

  #endif
}

inline T_timestamp &setTimestamp(T_timestamp &ts, long ms)
{
  ts.tv_sec  = ms / 1000;
  ts.tv_usec = (ms % 1000) * 1000;

  return ts;
}

//
// Return the smaller between two timestamps.
//

inline T_timestamp &setMinTimestamp(T_timestamp &ts, long ms)
{
  if ((ts.tv_sec * 1000 + ts.tv_usec / 1000) > ms)
  {
    ts.tv_sec  = ms / 1000;
    ts.tv_usec = (ms % 1000) * 1000;
  }

  return ts;
}

inline T_timestamp &setMinTimestamp(T_timestamp &ts1, T_timestamp &ts2)
{
  if ((ts1.tv_sec * 1000000 + ts1.tv_usec) >
           (ts2.tv_sec * 1000000 + ts2.tv_usec))
  {
    ts1.tv_sec  = ts2.tv_sec;
    ts1.tv_usec = ts2.tv_usec;
  }

  return ts1;
}

//
// Convert a timestamp in the total number
// of milliseconds.
//

inline long getMsTimestamp(const T_timestamp &ts)
{
  return ts.tv_sec * 1000 + ts.tv_usec / 1000;
}

//
// A 0 value on both seconds and microseconds
// fields means that timestamp is invalid or
// not set.
//

inline T_timestamp nullTimestamp()
{
  struct timeval ts;

  ts.tv_sec  = 0;
  ts.tv_usec = 0;

  return ts;
}

inline bool isTimestamp(const T_timestamp &ts)
{
  if (ts.tv_sec == 0 && ts.tv_usec == 0)
  {
     return 0;
  }

  return 1;
}

inline void subMsTimestamp(T_timestamp &ts, long ms)
{
  ts.tv_sec  -= ms / 1000;
  ts.tv_usec -= (ms % 1000) * 1000;
}

inline void addMsTimestamp(T_timestamp &ts, long ms)
{
  ts.tv_sec  += ms / 1000;
  ts.tv_usec += (ms % 1000) * 1000;
}

//
// Check the difference between timestamps.
// Return 0 if the system time went backward
// compared to the second timestamp, or the
// difference between the timestamps exceeds
// the given number of milliseconds.
//

inline int checkDiffTimestamp(const T_timestamp &ts1, const T_timestamp &ts2,
                                  long ms = 30000)
{
  long diffTs = diffTimestamp(ts1, ts2);

  if (diffTs < 0 || diffTs > ms)
  {
    return 0;
  }

  return 1;
}

//
// Return a string representing the timestamp.
//

std::string strTimestamp(const T_timestamp &ts);
std::string strMsTimestamp(const T_timestamp &ts);

inline std::string strTimestamp()
{
  return strTimestamp(getTimestamp());
}

inline std::string strMsTimestamp()
{
  return strMsTimestamp(getTimestamp());
}

//
// Update the current timestamp.
//

inline T_timestamp getNewTimestamp()
{
  #ifdef TEST

  T_timestamp ts;

  gettimeofday(&ts, NULL);

  *logofs << "Timestamp: Updating the current timestamp at "
          << strMsTimestamp(ts) << ".\n" << logofs_flush;

  long diffTs = diffTimestamp(timestamp, ts);

  if (diffTs > DRIFT_TIMESTAMP)
  {
    *logofs << "Timestamp: WARNING! Time difference since the "
            << "old timestamp is " << diffTs << "ms.\n"
            << logofs_flush;
  }

  #endif

  gettimeofday(&timestamp, NULL);

  return timestamp;
}

#endif /* Timestamp_H */
