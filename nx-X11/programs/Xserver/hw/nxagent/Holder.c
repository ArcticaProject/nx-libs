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

#include <signal.h>
#include <stdio.h>

/*
 * Set here the required log level.
 */

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG
#undef  DUMP

#ifdef DUMP

static char hexdigit(char c)
{
  char map[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', '?'};

  return map[c];
}

/*
FIXME: Please, check the implementation of the same
       function in nxcomp.
*/
char *nxagentChecksum(char *string, int length)
{
  static char md5_output[MD5_DIGEST_LENGTH * 2 + 1];
  static char md5[MD5_DIGEST_LENGTH];
  char * ret;
  int i;

  memset(md5, 0, sizeof(md5));
  memset(md5_output, 0, sizeof(md5_output));

  ret = MD5(string, length, md5);

  for (i = 0; i < MD5_DIGEST_LENGTH; i++)
  {
    char c = md5[i];

    md5_output[i * 2 + 0] = hexdigit((c >> 0) & 0xF);
    md5_output[i * 2 + 1] = hexdigit((c >> 4) & 0xF);
  }

  return md5_output;
}

#else

const char *nxagentChecksum(char *data, int size)
{
  return "";
}

#endif
