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

#include "PutPackedImage.h"

#include "ClientCache.h"

#include "EncodeBuffer.h"
#include "DecodeBuffer.h"

#include "WriteBuffer.h"

//
// Set the verbosity level.
//

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG
#undef  DUMP

//
// Constructors and destructors.
//

PutPackedImageStore::PutPackedImageStore(StaticCompressor *compressor)

  : MessageStore(compressor)
{
  enableCache    = PUTPACKEDIMAGE_ENABLE_CACHE;
  enableData     = PUTPACKEDIMAGE_ENABLE_DATA;
  enableSplit    = PUTPACKEDIMAGE_ENABLE_SPLIT;
  enableCompress = PUTPACKEDIMAGE_ENABLE_COMPRESS;

  dataLimit  = PUTPACKEDIMAGE_DATA_LIMIT;
  dataOffset = PUTPACKEDIMAGE_DATA_OFFSET;

  cacheSlots          = PUTPACKEDIMAGE_CACHE_SLOTS;
  cacheThreshold      = PUTPACKEDIMAGE_CACHE_THRESHOLD;
  cacheLowerThreshold = PUTPACKEDIMAGE_CACHE_LOWER_THRESHOLD;

  if (control -> isProtoStep8() == 1)
  {
    enableSplit = PUTPACKEDIMAGE_ENABLE_SPLIT_IF_PROTO_STEP_8;
  }

  messages_ -> resize(cacheSlots);

  for (T_messages::iterator i = messages_ -> begin();
           i < messages_ -> end(); i++)
  {
    *i = NULL;
  }

  temporary_ = NULL;
}

PutPackedImageStore::~PutPackedImageStore()
{
  for (T_messages::iterator i = messages_ -> begin();
           i < messages_ -> end(); i++)
  {
    destroy(*i);
  }

  destroy(temporary_);
}

//
// Here are the methods to handle messages' content.
//

int PutPackedImageStore::encodeIdentity(EncodeBuffer &encodeBuffer, const unsigned char *buffer,
                                            const unsigned int size, int bigEndian,
                                                ChannelCache *channelCache) const
{
  ClientCache *clientCache = (ClientCache *) channelCache;

  #ifdef DEBUG
  *logofs << name() << ": Encoding full message identity.\n" << logofs_flush;
  #endif

  // Client.
  encodeBuffer.encodeCachedValue(*(buffer + 1), 8,
                     clientCache -> resourceCache);

  // Size.
  encodeBuffer.encodeValue(GetUINT(buffer + 2, bigEndian), 16, 10);

  // Drawable.
  encodeBuffer.encodeXidValue(GetULONG(buffer + 4, bigEndian),
                     clientCache -> drawableCache);
  // GC.
  encodeBuffer.encodeXidValue(GetULONG(buffer + 8, bigEndian),
                     clientCache -> gcCache);
  // Method.
  encodeBuffer.encodeCachedValue(*(buffer + 12), 8,
                     clientCache -> methodCache);
  // Format.
  encodeBuffer.encodeValue(*(buffer + 13), 2);

  // SrcDepth.
  encodeBuffer.encodeCachedValue(*(buffer + 14), 8,
                     clientCache -> depthCache);
  // DstDepth.
  encodeBuffer.encodeCachedValue(*(buffer + 15), 8,
                     clientCache -> depthCache);
  // SrcLength.
  encodeBuffer.encodeCachedValue(GetULONG(buffer + 16, bigEndian), 24,
                     clientCache -> putPackedImageSrcLengthCache);
  // DstLength.
  encodeBuffer.encodeCachedValue(GetULONG(buffer + 20, bigEndian), 24,
                     clientCache -> putPackedImageDstLengthCache);
  // SrcX.
  unsigned int x = GetUINT(buffer + 24, bigEndian);
  int xDiff = x - clientCache -> putImageLastX;
  clientCache -> putImageLastX = x;
  encodeBuffer.encodeCachedValue(xDiff, 16,
                     clientCache -> putImageXCache, 8);
  // SrcY.
  unsigned int y = GetUINT(buffer + 26, bigEndian);
  int yDiff = y - clientCache -> putImageLastY;
  clientCache -> putImageLastY = y;
  encodeBuffer.encodeCachedValue(yDiff, 16,
                     clientCache -> putImageYCache, 8);
  // SrcWidth.
  encodeBuffer.encodeCachedValue(GetUINT(buffer + 28, bigEndian), 16,
                     clientCache -> putImageWidthCache, 8);
  // SrcHeight.
  encodeBuffer.encodeCachedValue(GetUINT(buffer + 30, bigEndian), 16,
                     clientCache -> putImageHeightCache, 8);
  // DstX.
  x = GetUINT(buffer + 32, bigEndian);
  xDiff = x - clientCache -> putImageLastX;
  clientCache -> putImageLastX = x;
  encodeBuffer.encodeCachedValue(xDiff, 16,
                     clientCache -> putImageXCache, 8);
  // DstY.
  y = GetUINT(buffer + 34, bigEndian);
  yDiff = y - clientCache -> putImageLastY;
  clientCache -> putImageLastY = y;
  encodeBuffer.encodeCachedValue(yDiff, 16,
                     clientCache -> putImageYCache, 8);
  // DstWidth.
  encodeBuffer.encodeCachedValue(GetUINT(buffer + 36, bigEndian), 16,
                     clientCache -> putImageWidthCache, 8);
  // DstHeight.
  encodeBuffer.encodeCachedValue(GetUINT(buffer + 38, bigEndian), 16,
                     clientCache -> putImageHeightCache, 8);

  #ifdef DEBUG
  *logofs << name() << ": Encoded full message identity.\n" << logofs_flush;
  #endif

  return 1;
}

int PutPackedImageStore::decodeIdentity(DecodeBuffer &decodeBuffer, unsigned char *&buffer,
                                            unsigned int &size, int bigEndian, WriteBuffer *writeBuffer,
                                                ChannelCache *channelCache) const
{
  ClientCache *clientCache = (ClientCache *) channelCache;

  #ifdef DEBUG
  *logofs << name() << ": Decoding full message identity.\n" << logofs_flush;
  #endif

  unsigned int value;
  unsigned char cValue;

  // Client.
  decodeBuffer.decodeCachedValue(cValue, 8,
                     clientCache -> resourceCache);

  // Size.
  decodeBuffer.decodeValue(size, 16, 10);

  size <<= 2;

  buffer = writeBuffer -> addMessage(size);

  *(buffer + 1) = cValue;

  // Drawable.
  decodeBuffer.decodeXidValue(value, clientCache -> drawableCache);
  PutULONG(value, buffer + 4, bigEndian);

  // GC.
  decodeBuffer.decodeXidValue(value, clientCache -> gcCache);
  PutULONG(value, buffer + 8, bigEndian);

  // Method.
  decodeBuffer.decodeCachedValue(cValue, 8,
                     clientCache -> methodCache);
  *(buffer + 12) = cValue;

  // Format.
  decodeBuffer.decodeValue(value, 2);
  *(buffer + 13) = value;

  // SrcDepth.
  decodeBuffer.decodeCachedValue(cValue, 8,
                     clientCache -> depthCache);
  *(buffer + 14) = cValue;

  // DstDepth.
  decodeBuffer.decodeCachedValue(cValue, 8,
                     clientCache -> depthCache);
  *(buffer + 15) = cValue;

  // SrcLength.
  decodeBuffer.decodeCachedValue(value, 24,
                     clientCache -> putPackedImageSrcLengthCache);
  PutULONG(value, buffer + 16, bigEndian);

  // DstLength.
  decodeBuffer.decodeCachedValue(value, 24,
                     clientCache -> putPackedImageDstLengthCache);
  PutULONG(value, buffer + 20, bigEndian);

  // SrcX.
  decodeBuffer.decodeCachedValue(value, 16,
                     clientCache -> putImageXCache, 8);
  clientCache -> putImageLastX += value;
  clientCache -> putImageLastX &= 0xffff;
  PutUINT(clientCache -> putImageLastX, buffer + 24, bigEndian);

  // SrcY.
  decodeBuffer.decodeCachedValue(value, 16,
                     clientCache -> putImageYCache, 8);
  clientCache -> putImageLastY += value;
  clientCache -> putImageLastY &= 0xffff;
  PutUINT(clientCache -> putImageLastY, buffer + 26, bigEndian);

  // SrcWidth.
  decodeBuffer.decodeCachedValue(value, 16,
                     clientCache -> putImageWidthCache, 8);
  PutUINT(value, buffer + 28, bigEndian);

  // SrcHeight.
  decodeBuffer.decodeCachedValue(value, 16,
                     clientCache -> putImageHeightCache, 8);
  PutUINT(value, buffer + 30, bigEndian);

  // DstX.
  decodeBuffer.decodeCachedValue(value, 16,
                     clientCache -> putImageXCache, 8);
  clientCache -> putImageLastX += value;
  clientCache -> putImageLastX &= 0xffff;
  PutUINT(clientCache -> putImageLastX, buffer + 32, bigEndian);

  // DstY.
  decodeBuffer.decodeCachedValue(value, 16,
                     clientCache -> putImageYCache, 8);
  clientCache -> putImageLastY += value;
  clientCache -> putImageLastY &= 0xffff;
  PutUINT(clientCache -> putImageLastY, buffer + 34, bigEndian);

  // DstWidth.
  decodeBuffer.decodeCachedValue(value, 16,
                     clientCache -> putImageWidthCache, 8);
  PutUINT(value, buffer + 36, bigEndian);

  // DstHeight.
  decodeBuffer.decodeCachedValue(value, 16,
                     clientCache -> putImageHeightCache, 8);
  PutUINT(value, buffer + 38, bigEndian);

  #ifdef DEBUG
  *logofs << name() << ": Decoded full message identity.\n" << logofs_flush;
  #endif

  return 1;
}

int PutPackedImageStore::parseIdentity(Message *message, const unsigned char *buffer,
                                           unsigned int size, int bigEndian) const
{
  PutPackedImageMessage *putPackedImage = (PutPackedImageMessage *) message;

  //
  // Here is the fingerprint.
  //

  putPackedImage -> client = *(buffer + 1);

  putPackedImage -> drawable = GetULONG(buffer + 4,  bigEndian); 
  putPackedImage -> gcontext = GetULONG(buffer + 8,  bigEndian);

  putPackedImage -> method = *(buffer + 12);

  putPackedImage -> format    = *(buffer + 13);
  putPackedImage -> src_depth = *(buffer + 14);
  putPackedImage -> dst_depth = *(buffer + 15);

  putPackedImage -> src_length = GetULONG(buffer + 16,  bigEndian);
  putPackedImage -> dst_length = GetULONG(buffer + 20,  bigEndian);

  putPackedImage -> src_x      = GetUINT(buffer + 24, bigEndian);
  putPackedImage -> src_y      = GetUINT(buffer + 26, bigEndian);
  putPackedImage -> src_width  = GetUINT(buffer + 28, bigEndian);
  putPackedImage -> src_height = GetUINT(buffer + 30, bigEndian);

  putPackedImage -> dst_x      = GetUINT(buffer + 32, bigEndian);
  putPackedImage -> dst_y      = GetUINT(buffer + 34, bigEndian);
  putPackedImage -> dst_width  = GetUINT(buffer + 36, bigEndian);
  putPackedImage -> dst_height = GetUINT(buffer + 38, bigEndian);

  #ifdef DEBUG
  *logofs << name() << ": Parsed identity for message at "
          << message << ".\n" << logofs_flush;
  #endif

  return 1;
}

int PutPackedImageStore::unparseIdentity(const Message *message, unsigned char *buffer,
                                             unsigned int size, int bigEndian) const
{
  PutPackedImageMessage *putPackedImage = (PutPackedImageMessage *) message;

  //
  // Fill all the message's fields.
  //

  *(buffer + 1) = putPackedImage -> client;

  PutULONG(putPackedImage -> drawable, buffer + 4, bigEndian);
  PutULONG(putPackedImage -> gcontext, buffer + 8, bigEndian);

  *(buffer + 12) = putPackedImage -> method;

  *(buffer + 13) = putPackedImage -> format;
  *(buffer + 14) = putPackedImage -> src_depth;
  *(buffer + 15) = putPackedImage -> dst_depth;

  PutULONG(putPackedImage -> src_length, buffer + 16, bigEndian);
  PutULONG(putPackedImage -> dst_length, buffer + 20, bigEndian);

  PutUINT(putPackedImage -> src_x,      buffer + 24, bigEndian);
  PutUINT(putPackedImage -> src_y,      buffer + 26, bigEndian);
  PutUINT(putPackedImage -> src_width,  buffer + 28, bigEndian);
  PutUINT(putPackedImage -> src_height, buffer + 30, bigEndian);

  PutUINT(putPackedImage -> dst_x,      buffer + 32, bigEndian);
  PutUINT(putPackedImage -> dst_y,      buffer + 34, bigEndian);
  PutUINT(putPackedImage -> dst_width,  buffer + 36, bigEndian);
  PutUINT(putPackedImage -> dst_height, buffer + 38, bigEndian);

  #ifdef DEBUG
  *logofs << name() << ": Unparsed identity for message at "
          << message << ".\n" << logofs_flush;
  #endif

  return 1;
}

void PutPackedImageStore::dumpIdentity(const Message *message) const
{
  #ifdef DUMP

  PutPackedImageMessage *putPackedImage = (PutPackedImageMessage *) message;

  *logofs << name() << ": Identity format "

          << "drawable " << putPackedImage -> drawable << ", "
          << "gcontext " << putPackedImage -> gcontext << ", "

          << "format "     << (unsigned int) putPackedImage -> format     << ", "
          << "method "     << (unsigned int) putPackedImage -> method     << ", "

          << "src_depth "  << (unsigned int) putPackedImage -> src_depth  << ", "
          << "dst_depth "  << (unsigned int) putPackedImage -> dst_depth  << ", "

          << "src_length " << putPackedImage -> src_length << ", "
          << "dst_length " << putPackedImage -> dst_length << ", "

          << "src_x "      << putPackedImage -> src_x      << ", "
          << "src_y "      << putPackedImage -> src_y      << ", "
          << "src_width "  << putPackedImage -> src_width  << ", "
          << "src_height " << putPackedImage -> src_height << ", "

          << "dst_x "      << putPackedImage -> dst_x      << ", "
          << "dst_y "      << putPackedImage -> dst_y      << ", "
          << "dst_width "  << putPackedImage -> dst_width  << ", "
          << "dst_height " << putPackedImage -> dst_height << ", "

          << "size "   << putPackedImage -> size_   << ".\n"
          << logofs_flush;

  #endif
}

void PutPackedImageStore::identityChecksum(const Message *message, const unsigned char *buffer,
                                               unsigned int size, int bigEndian) const
{
  //
  // Fields method, format, src_depth, dst_depth,
  // src_length, dst_length, src_x, src_y, src_width,
  // src_height.
  //
  //
  // TODO: We should better investigate the effect of
  // having fields src_x and src_y in identity instead
  // of keeping them in differences.
  //

  md5_append(md5_state_, buffer + 12, 20);
}

void PutPackedImageStore::updateIdentity(EncodeBuffer &encodeBuffer, const Message *message,
                                             const Message *cachedMessage,
                                                 ChannelCache *channelCache) const
{
  //
  // Encode the variant part.
  //

  PutPackedImageMessage *putPackedImage       = (PutPackedImageMessage *) message;
  PutPackedImageMessage *cachedPutPackedImage = (PutPackedImageMessage *) cachedMessage;

  ClientCache *clientCache = (ClientCache *) channelCache;

  #ifdef TEST
  *logofs << name() << ": Encoding value "
          << (unsigned int) putPackedImage -> client
          << " as client field.\n" << logofs_flush;
  #endif

  encodeBuffer.encodeCachedValue(putPackedImage -> client, 8,
                     clientCache -> resourceCache);

  cachedPutPackedImage -> client = putPackedImage -> client;

  #ifdef TEST
  *logofs << name() << ": Encoding value " << putPackedImage -> drawable
          << " as drawable field.\n" << logofs_flush;
  #endif

  encodeBuffer.encodeXidValue(putPackedImage -> drawable, clientCache -> drawableCache);

  cachedPutPackedImage -> drawable = putPackedImage -> drawable;

  #ifdef TEST
  *logofs << name() << ": Encoding value " << putPackedImage -> gcontext
          << " as gcontext field.\n" << logofs_flush;
  #endif

  encodeBuffer.encodeXidValue(putPackedImage -> gcontext, clientCache -> gcCache);

  cachedPutPackedImage -> gcontext = putPackedImage -> gcontext;

  #ifdef TEST
  *logofs << name() << ": Encoding value " << putPackedImage -> dst_x
          << " as " << "dst_x" << " field.\n" << logofs_flush;
  #endif

  unsigned short int diff_x = putPackedImage -> dst_x - cachedPutPackedImage -> dst_x;

  encodeBuffer.encodeCachedValue(diff_x, 16,
                     clientCache -> putImageXCache, 8);

  cachedPutPackedImage -> dst_x = putPackedImage -> dst_x;

  #ifdef TEST
  *logofs << name() << ": Encoding value " << putPackedImage -> dst_y
          << " as " << "dst_y" << " field.\n" << logofs_flush;
  #endif

  unsigned short int diff_y = putPackedImage -> dst_y - cachedPutPackedImage -> dst_y;

  encodeBuffer.encodeCachedValue(diff_y, 16,
                     clientCache -> putImageYCache, 8);

  cachedPutPackedImage -> dst_y = putPackedImage -> dst_y;

  #ifdef TEST
  *logofs << name() << ": Encoding value " << putPackedImage -> dst_width
          << " as " << "dst_width" << " field.\n" << logofs_flush;
  #endif

  encodeBuffer.encodeCachedValue(putPackedImage -> dst_width, 16,
                     clientCache -> putImageWidthCache, 8);

  cachedPutPackedImage -> dst_width = putPackedImage -> dst_width;

  #ifdef TEST
  *logofs << name() << ": Encoding value " << putPackedImage -> dst_height
          << " as " << "dst_height" << " field.\n" << logofs_flush;
  #endif

  encodeBuffer.encodeCachedValue(putPackedImage -> dst_height, 16,
                     clientCache -> putImageHeightCache, 8);

  cachedPutPackedImage -> dst_height = putPackedImage -> dst_height;
}

void PutPackedImageStore::updateIdentity(DecodeBuffer &decodeBuffer, const Message *message,
                                             ChannelCache *channelCache) const
{
  //
  // Decode the variant part.
  //

  PutPackedImageMessage *putPackedImage = (PutPackedImageMessage *) message;

  ClientCache *clientCache = (ClientCache *) channelCache;

  unsigned int  value;

  decodeBuffer.decodeCachedValue(putPackedImage -> client, 8,
                     clientCache -> resourceCache);

  #ifdef DEBUG
  *logofs << name() << ": Decoded value "
          << (unsigned int) putPackedImage -> client
          << " as client field.\n" << logofs_flush;
  #endif

  decodeBuffer.decodeXidValue(value, clientCache -> drawableCache);

  putPackedImage -> drawable = value;

  #ifdef DEBUG
  *logofs << name() << ": Decoded value " << putPackedImage -> drawable
          << " as drawable field.\n" << logofs_flush;
  #endif

  decodeBuffer.decodeXidValue(value, clientCache -> gcCache);

  putPackedImage -> gcontext = value;

  #ifdef DEBUG
  *logofs << name() << ": Decoded value " << putPackedImage -> gcontext
          << " as gcontext field.\n" << logofs_flush;
  #endif

  decodeBuffer.decodeCachedValue(value, 16,
               clientCache -> putImageXCache, 8);

  putPackedImage -> dst_x += value;
  putPackedImage -> dst_x &= 0xffff;

  #ifdef DEBUG
  *logofs << name() << ": Decoded value " << putPackedImage -> dst_x
          << " as dst_x field.\n" << logofs_flush;
  #endif

  decodeBuffer.decodeCachedValue(value, 16,
               clientCache -> putImageYCache, 8);

  putPackedImage -> dst_y += value;
  putPackedImage -> dst_y &= 0xffff;

  #ifdef DEBUG
  *logofs << name() << ": Decoded value " << putPackedImage -> dst_y
          << " as dst_y field.\n" << logofs_flush;
  #endif

  decodeBuffer.decodeCachedValue(value, 16,
               clientCache -> putImageWidthCache, 8);

  putPackedImage -> dst_width = value;

  #ifdef DEBUG
  *logofs << name() << ": Decoded value " << putPackedImage -> dst_width
          << " as dst_width field.\n" << logofs_flush;
  #endif

  decodeBuffer.decodeCachedValue(value, 16,
               clientCache -> putImageHeightCache, 8);

  putPackedImage -> dst_height = value;

  #ifdef DEBUG
  *logofs << name() << ": Decoded value " << putPackedImage -> dst_height
          << " as dst_height field.\n" << logofs_flush;
  #endif
}

