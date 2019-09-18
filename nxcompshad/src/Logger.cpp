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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <ctype.h>
#include <unistd.h>

#define PANIC
#define WARNING
#undef  TEST
#define DEBUG

#include "Misc.h"
#include "Logger.h"

Logger logger;

void Logger::user(const char *format, va_list arguments)
{
  char string[1024];

  vsnprintf(string, 1024, format, arguments);

  fprintf(stderr, "%s\n", string);
}

void Logger::error(const char *name, int error)
{
  fprintf(stderr, "PANIC! %s: Failed with code %d: %s\n",
               name, error, strerror(error));
}

void Logger::warning(const char *name, const char *format, va_list arguments)
{
  char string[1024];

  vsnprintf(string, 1024, format, arguments);

  fprintf(stderr, "%s: WARNING! %s\n", name, string);
}

void Logger::test(const char *name, const char *format, va_list arguments)
{
  char string[1024];

  vsnprintf(string, 1024, format, arguments);

  fprintf(stderr, "%s: %s\n", name, string);
}

void Logger::trace(const char *name)
{
  fprintf(stderr, "%s\n", name);
}

void Logger::debug(const char *name, const char *format, va_list arguments)
{
  char string[1024];

  vsnprintf(string, 1024, format, arguments);

  fprintf(stderr, "%s: %s\n", name, string);
}

void Logger::dump(const char *name, const char *data, int size)
{
  fprintf(stderr, "%s: Dumping %d bytes of data at %p\n",
              name, size, data);

  for (int i = 0; i < size;)
  {
    fprintf(stderr, "[%d]\t", i);

    int t = i;

    for (unsigned int ii = 0; i < size && ii < 8; i++, ii++)
    {
      fprintf(stderr, "%02x/%d\t", data[i] & 0xff, data[i]);
    }

    for (unsigned int ii = i % 8; ii > 0 && ii < 8; ii++)
    {
      fprintf(stderr, "\t");
    }

    i = t;

    for (unsigned int ii = 0; i < size && ii < 8; i++, ii++)
    {
      if (isprint(data[i]))
      {
        fprintf(stderr, "%c", data[i]);
      }
      else
      {
        fprintf(stderr, ".");
      }
    }

    fprintf(stderr, "\n");
  }
}
