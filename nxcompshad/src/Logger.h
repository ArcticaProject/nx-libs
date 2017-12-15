/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2011 NoMachine (http://www.nomachine.com)          */
/* Copyright (c) 2008-2014 Oleksandr Shneyder <o.shneyder@phoca-gmbh.de>  */
/* Copyright (c) 2014-2016 Ulrich Sibiller <uli42@gmx.de>                 */
/* Copyright (c) 2014-2016 Mihai Moldovan <ionic@ionic.de>                */
/* Copyright (c) 2011-2016 Mike Gabriel <mike.gabriel@das-netzwerkteam.de>*/
/* Copyright (c) 2015-2016 Qindel Group (http://www.qindel.com)           */
/*                                                                        */
/* NXCOMPSHAD, NX protocol compression and NX extensions to this software */
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

#ifndef Logger_H
#define Logger_H

#include <cerrno>
#include <cstdarg>

//
// Error handling macros.
//

#define ESET(e)  (errno = (e))
#define EGET()   (errno)
#define ESTR()   strerror(errno)

extern class Logger logger;

class Logger
{
  public:

  void user(const char *format, ...) __attribute__((format(gnu_printf, 2, 3)));

  void error(const char *name, int error);

  void warning(const char *name, const char *format, ...) __attribute__((format(gnu_printf, 3, 4)));

  void test(const char *name, const char *format, ...) __attribute__((format(gnu_printf, 3, 4)));

  void trace(const char *name);

  void debug(const char *name, const char *format, ...) __attribute__((format(gnu_printf, 3, 4)));

  void dump(const char *name, const char *data, int size);
};

static inline void logUser(const char *format, ...) \
    __attribute__((format(printf, 1, 2))) __attribute__((__unused__));

static inline void logError(const char *name, int error) \
    __attribute__((__unused__));

static inline void logWarning(const char *name, const char *format, ...) \
    __attribute__((__unused__));

static inline void logTest(const char *name, const char *format, ...) \
    __attribute__((format(printf, 2, 3))) __attribute__((__unused__));

static inline void logTrace(const char *name) \
    __attribute__((__unused__));

static inline void logDebug(const char *name, const char *format, ...) \
    __attribute__((format(printf, 2, 3))) __attribute__((__unused__));

static inline void logDump(const char *name, const char *data, int size) \
    __attribute__((__unused__));

static inline void logUser(const char *format, ...)
{
  va_list arguments;

  va_start(arguments, format);

  logger.user(format, arguments);

  va_end(arguments);
}

static inline void logError(const char *name, int error)
{
  #if defined(DEBUG) || defined(TEST) || \
          defined(WARNING) || defined(PANIC)

  logger.error(name, error);

  #endif
}

static inline void logWarning(const char *name, const char *format, ...)
{
  #if defined(DEBUG) || defined(TEST) || \
          defined(WARNING)

  va_list arguments;

  va_start(arguments, format);

  logger.warning(name, format, arguments);

  va_end(arguments);

  #endif
}

static inline void logTest(const char *name, const char *format, ...)
{
  #if defined(TEST)

  va_list arguments;

  va_start(arguments, format);

  logger.test(name, format, arguments);

  va_end(arguments);

  #endif
}

static inline void logTrace(const char *name)
{
  #if defined(DEBUG)

  logger.trace(name);

  #endif
}

static inline void logDebug(const char *name, const char *format, ...)
{
  #if defined(DEBUG)

  va_list arguments;

  va_start(arguments, format);

  logger.debug(name, format, arguments);

  va_end(arguments);

  #endif
}

static inline void logDump(const char *name, const char *data, int size)
{
  #if defined(TEST)

  logger.dump(name, data, size);

  #endif
}

#endif /* Logger_H */
