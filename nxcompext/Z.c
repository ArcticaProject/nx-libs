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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zlib.h>

#include "NXlib.h"

#include "Z.h"

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

#define Z_COMPRESSION_LEVEL      4
#define Z_COMPRESSION_THRESHOLD  32
#define Z_COMPRESSION_STRATEGY   Z_DEFAULT_STRATEGY

static int zCompressionLevel    = Z_COMPRESSION_LEVEL;
static int zCompressionStrategy = Z_COMPRESSION_STRATEGY;

static z_stream *zStream;

static int zInitialized;

static int ZConfigure(int level, int strategy);

static int ZDeflate(char *dest, unsigned int *destLen,
                        const char *source, unsigned int sourceLen);

char *ZCompressData(const char *plainData, unsigned int plainSize, int threshold,
                        int level, int strategy, unsigned int *compressedSize)
{
  char *compressedData;

  /*
   * Determine the size of the source image
   * data and make sure there is enough
   * space in the destination buffer.
   */

  *compressedSize = plainSize + (plainSize / 1000) + 12 + 1;

  compressedData = Xmalloc(*compressedSize);

  if (compressedData == NULL)
  {
    #ifdef PANIC
    fprintf(stderr, "******ZCompressData: PANIC! Failed to allocate [%d] bytes for the destination.\n",
                *compressedSize);
    #endif

    *compressedSize = 0;

    return NULL;
  }

  if (level == Z_NO_COMPRESSION || plainSize < threshold)
  {
    #ifdef TEST
    fprintf(stderr, "******ZCompressData: Not compressing [%d] bytes with level [%d] and "
	    "threshold [%d].\n", plainSize, level, threshold);
    #endif

    /*
     * Tell in the first byte of the buffer
     * if the remaining data is compressed
     * or not. This same byte can be used
     * in future to store some other flag.
     */

    *compressedData = 0;

    memcpy(compressedData + 1, plainData, plainSize);

    *compressedSize = plainSize + 1;

    return compressedData;
  }
  else
  {
    int result;

    /*
     * Reconfigure the stream if needed.
     */

    if (zCompressionLevel != level ||
            zCompressionStrategy != strategy)
    {
      ZConfigure(level, strategy);

      zCompressionLevel    = level;
      zCompressionStrategy = strategy;
    }

    result = ZDeflate(compressedData + 1, compressedSize, plainData, plainSize);

    if (result != Z_OK)
    {
      #ifdef PANIC
      fprintf(stderr, "******ZCompressData: PANIC! Failed to compress [%d] bytes with error [%s].\n",
                  plainSize, zError(result));
      #endif

      Xfree(compressedData);

      *compressedSize = 0;

      return NULL;
    }

    #ifdef TEST
    fprintf(stderr, "******ZCompressData: Source data of [%d] bytes compressed to [%d].\n",
                plainSize, *compressedSize);
    #endif

    *compressedData = 1;

    *compressedSize = *compressedSize + 1;

    return compressedData;
  }
}

int ZConfigure(int level, int strategy)
{
  /*
   * ZLIB wants the avail_out to be
   * non zero, even if the stream was
   * already flushed.
   */

  unsigned char dest[1];

  zStream -> next_out  = dest;
  zStream -> avail_out = 1;

  if (deflateParams(zStream, level, strategy) != Z_OK)
  {
    #ifdef PANIC
    fprintf(stderr, "******ZConfigure: PANIC! Failed to set level to [%d] and strategy to [%d].\n",
                level, strategy);
    #endif

    return -1;
  }
  #ifdef TEST
  else
  {
    fprintf(stderr, "******ZConfigure: Reconfigured the stream with level [%d] and strategy [%d].\n",
                level, strategy);
  }
  #endif

  return 1;
}

int ZDeflate(char *dest, unsigned int *destLen, const char *source, unsigned int sourceLen)
{
  int saveOut;
  int result;

  /*
   * Deal with the possible overflow.
   */

  if (zStream -> total_out & 0x80000000)
  {
    #ifdef TEST
    fprintf(stderr, "******ZDeflate: Reset Z stream counters with total in [%ld] total out [%ld].\n",
                zStream -> total_in, zStream -> total_out);
    #endif

    zStream -> total_in  = 0;
    zStream -> total_out = 0;
  }

  saveOut = zStream -> total_out;

  zStream -> next_in  = (Bytef *) source;
  zStream -> avail_in = (uInt) sourceLen;

  #ifdef MAXSEG_64K

  /*
   * Check if the source is greater
   * than 64K on a 16-bit machine.
   */

  if ((uLong) zStream -> avail_in != sourceLen) return Z_BUF_ERROR;

  #endif

  zStream -> next_out  = (unsigned char *) dest;
  zStream -> avail_out = (uInt) *destLen;

  if ((uLong) zStream -> avail_out != *destLen) return Z_BUF_ERROR;

  result = deflate(zStream, Z_FINISH);

  if (result != Z_STREAM_END)
  {
    deflateReset(zStream);

    return (result == Z_OK ? Z_BUF_ERROR : result);
  }

  *destLen = zStream -> total_out - saveOut;

  result = deflateReset(zStream);

  return result;
}

int ZInitEncoder()
{
  if (zInitialized == 0)
  {
    int result;

    zStream = Xmalloc(sizeof(z_stream));

    if (zStream == NULL)
    {
      #ifdef PANIC
      fprintf(stderr, "******ZInitEncoder: PANIC! Failed to allocate memory for the stream.\n");
      #endif

      return -1;
    }

    zStream -> zalloc = (alloc_func) 0;
    zStream -> zfree  = (free_func) 0;
    zStream -> opaque = (voidpf) 0;

    #ifdef TEST
    fprintf(stderr, "******ZInitEncoder: Initializing compressor with level [%d] and startegy [%d].\n",
                zCompressionLevel, zCompressionStrategy);
    #endif

    result = deflateInit2(zStream, zCompressionLevel, Z_DEFLATED,
                              15, 9, zCompressionStrategy);

    if (result != Z_OK)
    {
      #ifdef PANIC
      fprintf(stderr, "******ZInitEncoder: Failed to initialize the compressor with error [%s].\n",
                  zError(result));
      #endif

      return -1;
    }

    zInitialized = 1;
  }

  return zInitialized;
}

int ZResetEncoder()
{
  int result;

  if (zInitialized == 1)
  {
    result = deflateEnd(zStream);

    if (result != Z_OK)
    {
      #ifdef WARNING
      fprintf(stderr, "******ZResetEncoder: WARNING! Failed to deinitialize the compressor with error [%s].\n",
                  zError(result));
      #endif
    }

    Xfree(zStream);
  }

  zInitialized = 0;

  return 1;
}
