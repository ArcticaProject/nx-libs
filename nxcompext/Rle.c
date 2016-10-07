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
