/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2011 NoMachine (http://www.nomachine.com)          */
/* Copyright (c) 2008-2014 Oleksandr Shneyder <o.shneyder@phoca-gmbh.de>  */
/* Copyright (c) 2014-2016 Ulrich Sibiller <uli42@gmx.de>                 */
/* Copyright (c) 2014-2016 Mihai Moldovan <ionic@ionic.de>                */
/* Copyright (c) 2011-2016 Mike Gabriel <mike.gabriel@das-netzwerkteam.de>*/
/* Copyright (c) 2015-2016 Qindel Group (http://www.qindel.com)           */
/*                                                                        */
/* NXCOMP, NX protocol compression and NX extensions to this software     */
/* are copyright of the aforementioned persons and companies.             */
/*                                                                        */
/* Redistribution and use of the present software is allowed according    */
/* to terms specified in the file LICENSE.nxcomp which comes in the       */
/* source distribution.                                                   */
/*                                                                        */
/* All rights reserved.                                                   */
/*                                                                        */
/* NOTE: This software has received contributions from various other      */
/* contributors, only the core maintainers and supporters are listed as   */
/* copyright holders. Please contact us, if you feel you should be listed */
/* as copyright holder, as well.                                          */
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
