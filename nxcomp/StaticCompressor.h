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

#ifndef StaticCompressor_H
#define StaticCompressor_H

#include "Z.h"

class EncodeBuffer;
class DecodeBuffer;

class StaticCompressor
{
  public:

  StaticCompressor(int compressionLevel, int compressionThreshold);

  ~StaticCompressor();

  int compressBuffer(const unsigned char *plainBuffer, const unsigned int plainSize,
                         unsigned char *&compressedBuffer, unsigned int &compressedSize,
                             EncodeBuffer &encodeBuffer);

  int compressBuffer(const unsigned char *plainBuffer, const unsigned int plainSize,
                         unsigned char *&compressedBuffer, unsigned int &compressedSize);

  int decompressBuffer(unsigned char *plainBuffer, unsigned int plainSize,
                           const unsigned char *&compressedBuffer, unsigned int &compressedSize,
                               DecodeBuffer &decodeBuffer);

  int decompressBuffer(unsigned char *plainBuffer, const unsigned int plainSize,
                           const unsigned char *compressedBuffer, const unsigned compressedSize);

  static int isCompressionLevel(int compressionLevel)
  {
    return (compressionLevel == Z_DEFAULT_COMPRESSION ||
                (compressionLevel >= Z_NO_COMPRESSION &&
                     compressionLevel <= Z_BEST_COMPRESSION));
  }

  int fullReset()
  {
    return (deflateReset(&compressionStream_) == Z_OK &&
                inflateReset(&decompressionStream_) == Z_OK);
  }

  private:

  z_stream compressionStream_;
  z_stream decompressionStream_;

  unsigned char *buffer_;
  unsigned int  bufferSize_;

  int threshold_;
};

#endif
