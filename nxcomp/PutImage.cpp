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

#include "PutImage.h"

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

PutImageStore::PutImageStore(StaticCompressor *compressor)

  : MessageStore(compressor)
{
  enableCache    = PUTIMAGE_ENABLE_CACHE;
  enableData     = PUTIMAGE_ENABLE_DATA;
  enableSplit    = PUTIMAGE_ENABLE_SPLIT;
  enableCompress = PUTIMAGE_ENABLE_COMPRESS;

  if (control -> isProtoStep7() == 1)
  {
    enableCompress = PUTIMAGE_ENABLE_COMPRESS_IF_PROTO_STEP_7;
  }

  dataLimit  = PUTIMAGE_DATA_LIMIT;
  dataOffset = PUTIMAGE_DATA_OFFSET;

  cacheSlots          = PUTIMAGE_CACHE_SLOTS;
  cacheThreshold      = PUTIMAGE_CACHE_THRESHOLD;
  cacheLowerThreshold = PUTIMAGE_CACHE_LOWER_THRESHOLD;

  if (control -> isProtoStep8() == 1)
  {
    enableSplit = PUTIMAGE_ENABLE_SPLIT_IF_PROTO_STEP_8;
  }

  messages_ -> resize(cacheSlots);

  for (T_messages::iterator i = messages_ -> begin();
           i < messages_ -> end(); i++)
  {
    *i = NULL;
  }

  temporary_ = NULL;
}

PutImageStore::~PutImageStore()
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

int PutImageStore::encodeIdentity(EncodeBuffer &encodeBuffer, const unsigned char *buffer,
                                      const unsigned int size, int bigEndian,
                                          ChannelCache *channelCache) const
{
  ClientCache *clientCache = (ClientCache *) channelCache;

  #ifdef DEBUG
  *logofs << name() << ": Encoding full message identity.\n" << logofs_flush;
  #endif

  encodeBuffer.encodeValue(GetUINT(buffer + 2, bigEndian), 16, 8);

  encodeBuffer.encodeValue((unsigned int) buffer[1], 2);

  encodeBuffer.encodeXidValue(GetULONG(buffer + 4, bigEndian),
                     clientCache -> drawableCache);
  encodeBuffer.encodeXidValue(GetULONG(buffer + 8, bigEndian),
                     clientCache -> gcCache);

  unsigned int width = GetUINT(buffer + 12, bigEndian);
  encodeBuffer.encodeCachedValue(width, 16,
                     clientCache -> putImageWidthCache, 8);

  unsigned int height = GetUINT(buffer + 14, bigEndian);
  encodeBuffer.encodeCachedValue(height, 16,
                     clientCache -> putImageHeightCache, 8);

  unsigned int x = GetUINT(buffer + 16, bigEndian);
  int xDiff = x - clientCache -> putImageLastX;
  clientCache -> putImageLastX = x;
  encodeBuffer.encodeCachedValue(xDiff, 16,
                     clientCache -> putImageXCache, 8);

  unsigned int y = GetUINT(buffer + 18, bigEndian);
  int yDiff = y - clientCache -> putImageLastY;
  clientCache -> putImageLastY = y;
  encodeBuffer.encodeCachedValue(yDiff, 16,
                     clientCache -> putImageYCache, 8);

  encodeBuffer.encodeCachedValue(buffer[20], 8,
                     clientCache -> putImageLeftPadCache);

  encodeBuffer.encodeCachedValue(buffer[21], 8,
                     clientCache -> depthCache);

  #ifdef DEBUG
  *logofs << name() << ": Encoded full message identity.\n" << logofs_flush;
  #endif

  return 1;
}

int PutImageStore::decodeIdentity(DecodeBuffer &decodeBuffer, unsigned char *&buffer,
                                      unsigned int &size, int bigEndian, WriteBuffer *writeBuffer,
                                          ChannelCache *channelCache) const
{
  ClientCache *clientCache = (ClientCache *) channelCache;

  #ifdef DEBUG
  *logofs << name() << ": Decoding full message identity.\n" << logofs_flush;
  #endif

  unsigned int  value;
  unsigned char cValue;

  decodeBuffer.decodeValue(value, 16, 8);

  size = (value << 2);

  buffer = writeBuffer -> addMessage(size);

  decodeBuffer.decodeValue(value, 2);
  buffer[1] = (unsigned char) value;

  decodeBuffer.decodeXidValue(value, clientCache -> drawableCache);
  PutULONG(value, buffer + 4, bigEndian);

  decodeBuffer.decodeXidValue(value, clientCache -> gcCache);
  PutULONG(value, buffer + 8, bigEndian);

  unsigned int width;
  decodeBuffer.decodeCachedValue(width, 16,
                     clientCache -> putImageWidthCache, 8);
  PutUINT(width, buffer + 12, bigEndian);

  unsigned int height;
  decodeBuffer.decodeCachedValue(height, 16,
                     clientCache -> putImageHeightCache, 8);
  PutUINT(height, buffer + 14, bigEndian);

  decodeBuffer.decodeCachedValue(value, 16,
                     clientCache -> putImageXCache, 8);
  clientCache -> putImageLastX += value;
  clientCache -> putImageLastX &= 0xffff;
  PutUINT(clientCache -> putImageLastX, buffer + 16, bigEndian);

  decodeBuffer.decodeCachedValue(value, 16,
                     clientCache -> putImageYCache, 8);
  clientCache -> putImageLastY += value;
  clientCache -> putImageLastY &= 0xffff;
  PutUINT(clientCache -> putImageLastY, buffer + 18, bigEndian);

  decodeBuffer.decodeCachedValue(cValue, 8,
                     clientCache -> putImageLeftPadCache);
  buffer[20] = cValue;

  decodeBuffer.decodeCachedValue(cValue, 8,
                     clientCache -> depthCache);
  buffer[21] = cValue;

  #ifdef DEBUG
  *logofs << name() << ": Decoded full message identity.\n" << logofs_flush;
  #endif

  return 1;
}

int PutImageStore::parseIdentity(Message *message, const unsigned char *buffer,
                                     unsigned int size, int bigEndian) const
{
  PutImageMessage *putImage = (PutImageMessage *) message;

  //
  // Here is the fingerprint.
  //

  putImage -> format   = *(buffer + 1);
  putImage -> depth    = *(buffer + 21);
  putImage -> left_pad = *(buffer + 20);

  putImage -> width  = GetUINT(buffer + 12, bigEndian);
  putImage -> height = GetUINT(buffer + 14, bigEndian);
  putImage -> pos_x  = GetUINT(buffer + 16, bigEndian);
  putImage -> pos_y  = GetUINT(buffer + 18, bigEndian);

  putImage -> drawable = GetULONG(buffer + 4, bigEndian); 
  putImage -> gcontext = GetULONG(buffer + 8, bigEndian);

  #ifdef DEBUG
  *logofs << name() << ": Parsed identity for message at " << message << ".\n" << logofs_flush;
  #endif

  return 1;
}

int PutImageStore::unparseIdentity(const Message *message, unsigned char *buffer,
                                       unsigned int size, int bigEndian) const
{
  PutImageMessage *putImage = (PutImageMessage *) message;

  //
  // Fill all the message's fields.
  //

  *(buffer + 1) = putImage -> format;

  PutULONG(putImage -> drawable, buffer + 4, bigEndian);
  PutULONG(putImage -> gcontext, buffer + 8, bigEndian);

  PutUINT(putImage -> width, buffer + 12, bigEndian);
  PutUINT(putImage -> height, buffer + 14, bigEndian);
  PutUINT(putImage -> pos_x, buffer + 16, bigEndian);
  PutUINT(putImage -> pos_y, buffer + 18, bigEndian);

  *(buffer + 20) = (unsigned char) putImage -> left_pad;
  *(buffer + 21) = (unsigned char) putImage -> depth;

  #ifdef DEBUG
  *logofs << name() << ": Unparsed identity for message at "
          << message << ".\n" << logofs_flush;
  #endif

  return 1;
}

void PutImageStore::dumpIdentity(const Message *message) const
{
  #ifdef DUMP

  PutImageMessage *putImage = (PutImageMessage *) message;

  *logofs << name() << ": Identity format " << (unsigned) putImage -> format 
          << ", depth " << (unsigned) putImage -> depth << ", left_pad " 
          << (unsigned) putImage -> left_pad << ", width " << putImage -> width  
          << ", height " << putImage -> height << ", pos_x " << putImage -> pos_x 
          << ", pos_y " << putImage -> pos_y << ", drawable " << putImage -> drawable 
          << ", gcontext " << putImage -> gcontext  << ", size " << putImage -> size_
          << ".\n" << logofs_flush;

  #endif
}

void PutImageStore::identityChecksum(const Message *message, const unsigned char *buffer,
                                         unsigned int size, int bigEndian) const
{
  //
  // Fields format, width, height, left_pad, depth.
  //

  md5_append(md5_state_, buffer + 1,  1);
  md5_append(md5_state_, buffer + 12, 4);
  md5_append(md5_state_, buffer + 20, 2);
}

void PutImageStore::updateIdentity(EncodeBuffer &encodeBuffer, const Message *message,
                                       const Message *cachedMessage,
                                           ChannelCache *channelCache) const
{
  //
  // Encode the variant part.
  //

  PutImageMessage *putImage       = (PutImageMessage *) message;
  PutImageMessage *cachedPutImage = (PutImageMessage *) cachedMessage;

  ClientCache *clientCache = (ClientCache *) channelCache;

  #ifdef TEST
  *logofs << name() << ": Encoding value " << putImage -> drawable
          << " as drawable field.\n" << logofs_flush;
  #endif

  encodeBuffer.encodeXidValue(putImage -> drawable, clientCache -> drawableCache);

  cachedPutImage -> drawable = putImage -> drawable;

  #ifdef TEST
  *logofs << name() << ": Encoding value " << putImage -> gcontext
          << " as gcontext field.\n" << logofs_flush;
  #endif

  encodeBuffer.encodeXidValue(putImage -> gcontext, clientCache -> gcCache);

  cachedPutImage -> gcontext = putImage -> gcontext;

  #ifdef TEST
  *logofs << name() << ": Encoding value " << putImage -> pos_x
          << " as " << "pos_x" << " field.\n" << logofs_flush;
  #endif

  unsigned short int diff_x = putImage -> pos_x - cachedPutImage -> pos_x;

  encodeBuffer.encodeCachedValue(diff_x, 16,
                     clientCache -> putImageXCache, 8);

  cachedPutImage -> pos_x = putImage -> pos_x;

  #ifdef TEST
  *logofs << name() << ": Encoding value " << putImage -> pos_y
          << " as " << "pos_y" << " field.\n" << logofs_flush;
  #endif

  unsigned short int diff_y = putImage -> pos_y - cachedPutImage -> pos_y;

  encodeBuffer.encodeCachedValue(diff_y, 16,
                     clientCache -> putImageYCache, 8);

  cachedPutImage -> pos_y = putImage -> pos_y;
}

void PutImageStore::updateIdentity(DecodeBuffer &decodeBuffer, const Message *message,
                                       ChannelCache *channelCache) const
{
  //
  // Decode the variant part.
  //

  PutImageMessage *putImage = (PutImageMessage *) message;

  ClientCache *clientCache = (ClientCache *) channelCache;

  unsigned int value;

  decodeBuffer.decodeXidValue(value, clientCache -> drawableCache);

  putImage -> drawable = value;

  #ifdef DEBUG
  *logofs << name() << ": Decoded value " << putImage -> drawable
          << " as drawable field.\n" << logofs_flush;
  #endif

  decodeBuffer.decodeXidValue(value, clientCache -> gcCache);

  putImage -> gcontext = value;

  #ifdef DEBUG
  *logofs << name() << ": Decoded value " << putImage -> gcontext
          << " as gcontext field.\n" << logofs_flush;
  #endif

  decodeBuffer.decodeCachedValue(value, 16,
               clientCache -> putImageXCache, 8);

  putImage -> pos_x += value;
  putImage -> pos_x &= 0xffff;

  #ifdef DEBUG
  *logofs << name() << ": Decoded value " << putImage -> pos_x
          << " as pos_x field.\n" << logofs_flush;
  #endif

  decodeBuffer.decodeCachedValue(value, 16,
               clientCache -> putImageYCache, 8);

  putImage -> pos_y += value;
  putImage -> pos_y &= 0xffff;

  #ifdef DEBUG
  *logofs << name() << ": Decoded value " << putImage -> pos_y
          << " as pos_y field.\n" << logofs_flush;
  #endif
}
