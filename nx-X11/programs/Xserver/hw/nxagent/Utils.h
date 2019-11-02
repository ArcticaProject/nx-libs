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

#ifndef __Utils_H__
#define __Utils_H__

/*
 * Number of bits in fixed point operations.
 */

#define PRECISION  16

/*
 * "1" ratio means "don't scale".
 */

#define DONT_SCALE  (1 << PRECISION)

#define nxagentScale(i, ratio) (((i) * (ratio)) >> (PRECISION))

static inline const char * validateString(const char *str) {
  return str ? str : "(null)";
}

/*
 * nxagentChecksum used to be in Holder.c but was broken beyond
 * repair. As Holder.c was removed we put it here as a stub until we
 * need it for debugging.
 */

static inline const char *nxagentChecksum(char *data, int size)
{
  return "not_implemented";
}

#define SAFE_XFree(what) do {if (what) {XFree(what); what = NULL;}} while (0)
#define SAFE_free(what) do {free(what); what = NULL;} while (0)

/* some helper macros to produce a quoted string from other macros */
#define QUOTEEXP(str) #str
#define QUOTE(str) QUOTEEXP(str)

#endif /* __Utils_H__ */
