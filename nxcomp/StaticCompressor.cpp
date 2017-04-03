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

#include "Zlib.h"
#include "Misc.h"
#include "Control.h"

#include "EncodeBuffer.h"
#include "DecodeBuffer.h"

#include "StaticCompressor.h"

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

StaticCompressor::StaticCompressor(int compressionLevel,
                                       int compressionThreshold)
{
  buffer_     = NULL;
  bufferSize_ = 0;

  compressionStream_.zalloc = (alloc_func) 0;
  compressionStream_.zfree  = (free_func) 0;
  compressionStream_.opaque = (voidpf) 0;

  decompressionStream_.zalloc = (alloc_func) 0;
  decompressionStream_.zfree  = (free_func) 0;
  decompressionStream_.opaque = (void *) 0;

  decompressionStream_.next_in = (Bytef *) 0;
  decompressionStream_.avail_in = 0;

  #ifdef TEST
  *logofs << "StaticCompressor: Compression level is "
          << compressionLevel << ".\n" << logofs_flush;
  #endif

  int result = deflateInit2(&compressionStream_, compressionLevel, Z_DEFLATED,
                                15, 9, Z_DEFAULT_STRATEGY);

  if (result != Z_OK)
  {
    #ifdef PANIC
    *logofs << "StaticCompressor: PANIC! Cannot initialize the "
            << "compression stream. Error is '" << zError(result)
            << "'.\n" << logofs_flush;
    #endif

    cerr << "Error" << ": Cannot initialize the compression "
         << "stream. Error is '" << zError(result) << "'.\n";

    HandleAbort();
  }

  result = inflateInit2(&decompressionStream_, 15);

  if (result != Z_OK)
  {
    #ifdef PANIC
    *logofs << "StaticCompressor: PANIC! Cannot initialize the "
            << "decompression stream. Error is '" << zError(result)
            << "'.\n" << logofs_flush;
    #endif

    cerr << "Error" << ": Cannot initialize the decompression "
         << "stream. Error is '" << zError(result) << "'.\n";

    HandleAbort();
  }

  #ifdef TEST
  *logofs << "StaticCompressor: Compression threshold is "
          << compressionThreshold << ".\n" << logofs_flush;
  #endif

  threshold_ = compressionThreshold;
}

StaticCompressor::~StaticCompressor()
{
  int result = deflateEnd(&compressionStream_);

  if (result != Z_OK)
  {
    #ifdef PANIC
    *logofs << "StaticCompressor: PANIC! Cannot deinitialize the "
            << "compression stream. Error is '" << zError(result)
            << "'.\n" << logofs_flush;
    #endif

    cerr << "Error" << ": Cannot deinitialize the compression "
         << "stream. Error is '" << zError(result) << "'.\n";
  }

  result = inflateEnd(&decompressionStream_);

  if (result != Z_OK)
  {
    #ifdef PANIC
    *logofs << "StaticCompressor: PANIC! Cannot deinitialize the "
            << "decompression stream. Error is '" << zError(result)
            << "'.\n" << logofs_flush;
    #endif

    cerr << "Error" << ": Cannot deinitialize the decompression "
         << "stream. Error is '" << zError(result) << "'.\n";
  }

  delete [] buffer_;
}

//
// This function compresses and encodes the compressed
// buffer. It returns a pointer to the internal buffer
// where data was compressed.
//

int StaticCompressor::compressBuffer(const unsigned char *plainBuffer,
                                         const unsigned int plainSize,
                                             unsigned char *&compressedBuffer,
                                                 unsigned int &compressedSize,
                                                     EncodeBuffer &encodeBuffer)
{
  if (control -> LocalDataCompression == 0 ||
          compressBuffer(plainBuffer, plainSize, 
                             compressedBuffer, compressedSize) <= 0)
  {
    encodeBuffer.encodeBoolValue(0);

    encodeBuffer.encodeMemory(plainBuffer, plainSize);

    return 0;
  }
  else
  {
    encodeBuffer.encodeBoolValue(1);

    encodeBuffer.encodeValue(compressedSize, 32, 14);
    encodeBuffer.encodeValue(plainSize, 32, 14);

    encodeBuffer.encodeMemory(compressedBuffer, compressedSize);

    return 1;
  }
}

//
// This function compresses data into a dynamically
// allocated buffer and returns a pointer to it, so
// application must copy data before the next call.
//

int StaticCompressor::compressBuffer(const unsigned char *plainBuffer,
                                         const unsigned int plainSize,
                                             unsigned char *&compressedBuffer,
                                                 unsigned int &compressedSize)
{
  #ifdef DEBUG
  *logofs << "StaticCompressor: Called for buffer at "
          << (void *) plainBuffer << ".\n"
          << logofs_flush;
  #endif

  compressedSize = plainSize;

  if (plainSize < (unsigned int) threshold_)
  {
    #ifdef TEST
    *logofs << "StaticCompressor: Leaving buffer unchanged. "
            << "Plain size is " << plainSize << " with threshold "
            << (unsigned int) threshold_ << ".\n" << logofs_flush;
    #endif

    return 0;
  }

  //
  // Determine the size of the temporary
  // buffer. 
  //

  unsigned int newSize = plainSize + (plainSize / 1000) + 12;

  //
  // Allocate a new buffer if it grows
  // beyond 64K.
  //

  if (buffer_ == NULL || (bufferSize_ > 65536 &&
          newSize < bufferSize_ / 2) || newSize > bufferSize_)
  {
    delete [] buffer_;

    buffer_ = new unsigned char[newSize];

    if (buffer_ == NULL)
    {
      #ifdef PANIC
      *logofs << "StaticCompressor: PANIC! Can't allocate compression "
              << "buffer of " << newSize << " bytes. Error is " << EGET()
              << " ' " << ESTR() << "'.\n" << logofs_flush;
      #endif

      cerr << "Warning" << ": Can't allocate compression buffer of "
           << newSize << " bytes. Error is " << EGET()
           << " '" << ESTR() << "'.\n";

      bufferSize_ = 0;

      return 0;
    }

    bufferSize_ = newSize;
  }

  unsigned int resultingSize = newSize; 

  int result = ZlibCompress(&compressionStream_, buffer_, &resultingSize,
                             plainBuffer, plainSize);

  if (result == Z_OK)
  {
    if (resultingSize > newSize)
    {
      #ifdef PANIC
      *logofs << "StaticCompressor: PANIC! Overflow in compression "
              << "buffer size. " << "Expected size was " << newSize
              << " while it is " << resultingSize << ".\n"
              << logofs_flush;
      #endif

      cerr << "Error" << ": Overflow in compress buffer size. "
           << "Expected size was " << newSize << " while it is "
           << resultingSize << ".\n";

      return -1;
    }
    else if (resultingSize >= plainSize)
    {
      #ifdef TEST
      *logofs << "StaticCompressor: Leaving buffer unchanged. "
              << "Plain size is " << plainSize << " compressed "
              << "size is " << resultingSize << ".\n"
              << logofs_flush;
      #endif

      return 0;
    }

    compressedBuffer = buffer_;
    compressedSize   = resultingSize;

    #ifdef TEST
    *logofs << "StaticCompressor: Compressed buffer from "
            << plainSize << " to " << resultingSize
            << " bytes.\n" << logofs_flush;
    #endif

    return 1;
  }

  #ifdef PANIC
  *logofs << "StaticCompressor: PANIC! Failed compression of buffer. "
          << "Error is '" << zError(result) << "'.\n"
          << logofs_flush;
  #endif

  cerr << "Error" << ": Failed compression of buffer. "
       << "Error is '" << zError(result) << "'.\n";

  return -1;
}

int StaticCompressor::decompressBuffer(unsigned char *plainBuffer,
                                           unsigned int plainSize,
                                               const unsigned char *&compressedBuffer,
                                                   unsigned int &compressedSize,
                                                       DecodeBuffer &decodeBuffer)
{
  #ifdef DEBUG
  *logofs << "StaticCompressor: Called for buffer at "
          << (void *) plainBuffer << ".\n"
          << logofs_flush;
  #endif

  unsigned int value;

  decodeBuffer.decodeBoolValue(value);

  if (value == 0)
  {
    memcpy(plainBuffer,
               decodeBuffer.decodeMemory(plainSize),
                   plainSize);

    return 0;
  }

  unsigned int checkSize = plainSize;

  decodeBuffer.decodeValue(value, 32, 14);
  compressedSize = value;

  decodeBuffer.decodeValue(value, 32, 14);
  checkSize = value;

  //
  // If caller needs the original compressed
  // data it must copy this to its own buffer
  // before using any further decode function.
  //

  compressedBuffer = decodeBuffer.decodeMemory(compressedSize);

  int result = ZlibDecompress(&decompressionStream_, plainBuffer, &checkSize,
                               compressedBuffer, compressedSize);

  if (result != Z_OK)
  {
    #ifdef PANIC
    *logofs << "StaticCompressor: PANIC! Failure decompressing buffer. "
            << "Error is '" << zError(result) << "'.\n"
            << logofs_flush;
    #endif

    cerr << "Error" << ": Failure decompressing buffer. "
         << "Error is '" << zError(result) << "'.\n";

    return -1;
  }
  else if (plainSize != checkSize)
  {
    #ifdef PANIC
    *logofs << "StaticCompressor: PANIC! Expected decompressed size was "
            << plainSize << " while it is " << checkSize 
            << ".\n" << logofs_flush;
    #endif

    cerr << "Error" << ": Expected decompressed size was "
         << plainSize << " while it is " << checkSize
         << ".\n";

    return -1;
  }

  return 1;
}

//
// This is used to uncompress on-the-fly
// messages whose data has been stored
// in compressed format.
//

int StaticCompressor::decompressBuffer(unsigned char *plainBuffer,
                                           const unsigned int plainSize,
                                               const unsigned char *compressedBuffer,
                                                   const unsigned int compressedSize)
{
  #ifdef TEST
  *logofs << "StaticCompressor: Called for buffer at "
          << (void *) plainBuffer << ".\n"
          << logofs_flush;
  #endif

  unsigned int checkSize = plainSize;

  int result = ZlibDecompress(&decompressionStream_, plainBuffer, &checkSize,
                               compressedBuffer, compressedSize);

  if (result != Z_OK)
  {
    #ifdef PANIC
    *logofs << "StaticCompressor: PANIC! Failure decompressing buffer. "
            << "Error is '" << zError(result) << "'.\n"
            << logofs_flush;
    #endif

    return -1;
  }

  if (plainSize != checkSize)
  {
    #ifdef PANIC
    *logofs << "StaticCompressor: PANIC! Expected decompressed size was "
            << plainSize << " while it is " << checkSize 
            << ".\n" << logofs_flush;
    #endif

    cerr << "Error" << ": Expected decompressed size was "
         << plainSize << " while it is " << checkSize
         << ".\n";

    return -1;
  }

  #ifdef TEST
  *logofs << "StaticCompressor: Decompressed buffer from "
          << compressedSize << " to " << plainSize
          << " bytes.\n" << logofs_flush;
  #endif

  return 1;
}
