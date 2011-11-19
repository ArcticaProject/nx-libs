/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2007 NoMachine, http://www.nomachine.com/.         */
/*                                                                        */
/* NXCOMPEXT, NX protocol compression and NX extensions to this software  */
/* are copyright of NoMachine. Redistribution and use of the present      */
/* software is allowed according to terms specified in the file LICENSE   */
/* which comes in the source distribution.                                */
/*                                                                        */
/* Check http://www.nomachine.com/licensing.html for applicability.       */
/*                                                                        */
/* NX and NoMachine are trademarks of NoMachine S.r.l.                    */
/*                                                                        */
/* All rigths reserved.                                                   */
/*                                                                        */
/**************************************************************************/

#include <zlib.h>

#include "NXlib.h"

#include "Alpha.h"
#include "Z.h"

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
  return ZCompressData(data, size, alphaCompressionThreshold, alphaCompressionLevel,
                           alphaCompressionStrategy, compressed_size);
}
