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

#include "Colormap.h"
#include "Z.h"

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

#define COLORMAP_COMPRESSION_LEVEL      4
#define COLORMAP_COMPRESSION_THRESHOLD  32
#define COLORMAP_COMPRESSION_STRATEGY   Z_DEFAULT_STRATEGY

static int colormapCompressionLevel     = COLORMAP_COMPRESSION_LEVEL;
static int colormapCompressionThreshold = COLORMAP_COMPRESSION_THRESHOLD;
static int colormapCompressionStrategy  = COLORMAP_COMPRESSION_STRATEGY;

char *ColormapCompressData(const char *data, unsigned int size, unsigned int *compressed_size)
{
  return ZCompressData(data, size, colormapCompressionThreshold, colormapCompressionLevel,
                           colormapCompressionStrategy, compressed_size);
}
