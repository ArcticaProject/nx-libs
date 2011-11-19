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

#include "Rgb.h"
#include "Z.h"

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

#define RGB_COMPRESSION_LEVEL      4
#define RGB_COMPRESSION_THRESHOLD  32
#define RGB_COMPRESSION_STRATEGY   Z_DEFAULT_STRATEGY

static int rgbCompressionLevel     = RGB_COMPRESSION_LEVEL;
static int rgbCompressionThreshold = RGB_COMPRESSION_THRESHOLD;
static int rgbCompressionStrategy  = RGB_COMPRESSION_STRATEGY;

char *RgbCompressData(XImage *image, unsigned int *size)
{
  return ZCompressData(image -> data, image -> bytes_per_line * image -> height,
                           rgbCompressionThreshold, rgbCompressionLevel,
                               rgbCompressionStrategy, size);
}
