/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2011 NoMachine (http://www.nomachine.com)          */
/* Copyright (c) 2008-2014 Oleksandr Shneyder <o.shneyder@phoca-gmbh.de>  */
/* Copyright (c) 2014-2016 Ulrich Sibiller <uli42@gmx.de>                 */
/* Copyright (c) 2014-2016 Mihai Moldovan <ionic@ionic.de>                */
/* Copyright (c) 2011-2016 Mike Gabriel <mike.gabriel@das-netzwerkteam.de>*/
/* Copyright (c) 2015-2016 Qindel Group (http://www.qindel.com)           */
/*                                                                        */
/* NXCOMPEXT, NX protocol compression and NX extensions to this software  */
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

#include <zlib.h>

#include "Compext.h"

#include "Alpha.h"
#include "Zlib.h"

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

#define ALPHA_COMPRESSION_LEVEL      1
#define ALPHA_COMPRESSION_THRESHOLD  32
#define ALPHA_COMPRESSION_STRATEGY   Z_RLE

static int alphaCompressionLevel     = ALPHA_COMPRESSION_LEVEL;
static int alphaCompressionThreshold = ALPHA_COMPRESSION_THRESHOLD;
static int alphaCompressionStrategy  = ALPHA_COMPRESSION_STRATEGY;

char *AlphaCompressData(const char *data, unsigned int size, unsigned int *compressed_size)
{
  return ZlibCompressData(data, size, alphaCompressionThreshold, alphaCompressionLevel,
                           alphaCompressionStrategy, compressed_size);
}
