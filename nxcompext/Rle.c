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

#include "Rle.h"
#include "Z.h"

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

#define RLE_COMPRESSION_LEVEL      1
#define RLE_COMPRESSION_THRESHOLD  32
#define RLE_COMPRESSION_STRATEGY   Z_RLE

static int rleCompressionLevel     = RLE_COMPRESSION_LEVEL;
static int rleCompressionThreshold = RLE_COMPRESSION_THRESHOLD;
static int rleCompressionStrategy  = RLE_COMPRESSION_STRATEGY;

char *RleCompressData(XImage *image, unsigned int *size)
{
  return ZCompressData(image -> data, image -> bytes_per_line * image -> height,
                           rleCompressionThreshold, rleCompressionLevel,
                               rleCompressionStrategy, size);
}
