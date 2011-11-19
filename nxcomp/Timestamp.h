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

#ifndef Timestamp_H
#define Timestamp_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <time.h>
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
            << "current timestamp is " << diffTs << " Ms.\n"
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

char *strTimestamp(const T_timestamp &ts);
char *strMsTimestamp(const T_timestamp &ts);

inline char *strTimestamp()
{
  return strTimestamp(getTimestamp());
}

inline char *strMsTimestamp()
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
            << "old timestamp is " << diffTs << " Ms.\n"
            << logofs_flush;
  }

  #endif

  gettimeofday(&timestamp, NULL);

  return timestamp;
}

#endif /* Timestamp_H */
