/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2010 NoMachine, http://www.nomachine.com/.         */
/*                                                                        */
/* NXCOMP, NX protocol compression and NX extensions to this software     */
/* are copyright of NoMachine. Redistribution and use of the present      */
/* software is allowed according to terms specified in the file LICENSE   */
/* which comes in the source distribution.                                */
/*                                                                        */
/* Check http://www.nomachine.com/licensing.html for applicability.       */
/*                                                                        */
/* NX and NoMachine are trademarks of Medialogic S.p.A.                   */
/*                                                                        */
/* All rights reserved.                                                   */
/*                                                                        */
/**************************************************************************/

#include "Z.h"
#include "Misc.h"

int ZCompress(z_stream *stream, unsigned char *dest, unsigned int *destLen,
                  const unsigned char *source, unsigned int sourceLen)
{
  //
  // Deal with the possible overflow.
  //

  if (stream -> total_out & 0x80000000)
  {
    #ifdef TEST
    *logofs << "ZCompress: Reset stream counters with "
            << "total in " << stream -> total_in
            << " and total out " << stream -> total_out
            << ".\n" << logofs_flush;
    #endif

    stream -> total_in  = 0;
    stream -> total_out = 0;
  }

  unsigned int saveOut = stream -> total_out;

  stream -> next_in  = (Bytef *) source;
  stream -> avail_in = sourceLen;

  //
  // Check if the source is bigger than
  // 64K on 16-bit machine.
  //

  #ifdef MAXSEG_64K

  if ((uLong) stream -> avail_in != sourceLen) return Z_BUF_ERROR;

  #endif

  stream -> next_out  = dest;
  stream -> avail_out = *destLen;

  #ifdef MAXSEG_64K

  if ((uLong) stream -> avail_out != *destLen) return Z_BUF_ERROR;

  #endif

  int result = deflate(stream, Z_FINISH);

  if (result != Z_STREAM_END)
  {
    deflateReset(stream);

    return (result == Z_OK ? Z_BUF_ERROR : result);
  }

  *destLen = stream -> total_out - saveOut;

  result = deflateReset(stream);

  return result;
}

int ZDecompress(z_stream *stream, unsigned char *dest, unsigned int *destLen,
                    const unsigned char *source, unsigned int sourceLen)
{
  stream -> next_in  = (Bytef *) source;
  stream -> avail_in = sourceLen;

  //
  // Deal with the possible overflow.
  //

  if (stream -> total_out & 0x80000000)
  {
    #ifdef TEST
    *logofs << "ZDecompress: Reset stream counters with "
            << "total in " << stream -> total_in
            << " and total out " << stream -> total_out
            << ".\n" << logofs_flush;
    #endif

    stream -> total_in  = 0;
    stream -> total_out = 0;
  }

  unsigned int saveOut = stream -> total_out;

  if (stream -> avail_in != sourceLen)
  {
    return Z_BUF_ERROR;
  }

  stream -> next_out  = dest;
  stream -> avail_out = *destLen;

  if (stream -> avail_out != *destLen)
  {
    return Z_BUF_ERROR;
  }

  int result = inflate(stream, Z_FINISH);

  if (result != Z_STREAM_END)
  {
    inflateReset(stream);

    return (result == Z_OK ? Z_BUF_ERROR : result);
  }

  *destLen = stream -> total_out - saveOut;

  result = inflateReset(stream);

  return result;
}
