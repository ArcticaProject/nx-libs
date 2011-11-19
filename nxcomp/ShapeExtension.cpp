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

#include "ShapeExtension.h"

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

ShapeExtensionStore::ShapeExtensionStore(StaticCompressor *compressor)

  : MessageStore(compressor)
{
  enableCache    = SHAPEEXTENSION_ENABLE_CACHE;
  enableData     = SHAPEEXTENSION_ENABLE_DATA;
  enableSplit    = SHAPEEXTENSION_ENABLE_SPLIT;
  enableCompress = SHAPEEXTENSION_ENABLE_COMPRESS;

  if (control -> isProtoStep7() == 1)
  {
    enableCompress = SHAPEEXTENSION_ENABLE_COMPRESS_IF_PROTO_STEP_7;
  }

  dataLimit  = SHAPEEXTENSION_DATA_LIMIT;
  dataOffset = SHAPEEXTENSION_DATA_OFFSET;

  cacheSlots          = SHAPEEXTENSION_CACHE_SLOTS;
  cacheThreshold      = SHAPEEXTENSION_CACHE_THRESHOLD;
  cacheLowerThreshold = SHAPEEXTENSION_CACHE_LOWER_THRESHOLD;

  opcode_ = X_NXInternalShapeExtension;

  messages_ -> resize(cacheSlots);

  for (T_messages::iterator i = messages_ -> begin();
           i < messages_ -> end(); i++)
  {
    *i = NULL;
  }

  temporary_ = NULL;
}

ShapeExtensionStore::~ShapeExtensionStore()
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

int ShapeExtensionStore::encodeIdentity(EncodeBuffer &encodeBuffer, const unsigned char *buffer,
                                            const unsigned int size, int bigEndian,
                                                ChannelCache *channelCache) const
{
  //
  // Handle this extension in a way similar to shape.
  //

  ClientCache *clientCache = (ClientCache *) channelCache;

  #ifdef DEBUG
  *logofs << name() << ": Encoding full message identity.\n" << logofs_flush;
  #endif

  //
  // We handle all possible requests of this extension
  // using the same opcode. We give to message a data
  // offset of 4 (or 16 if proto is >= 3) and handle
  // the first 16 bytes through an array of caches.
  //

  encodeBuffer.encodeValue(size >> 2, 16, 10);

  encodeBuffer.encodeCachedValue(*(buffer + 1), 8,
                     clientCache -> shapeOpcodeCache);

  for (unsigned int i = 0; i < 8 && (i * 2 + 4) < size; i++)
  {
    encodeBuffer.encodeCachedValue(GetUINT(buffer + (i * 2) + 4, bigEndian), 16,
                       *clientCache -> shapeDataCache[i]);
  }

  #ifdef DEBUG
  *logofs << name() << ": Encoded full message identity.\n" << logofs_flush;
  #endif

  return 1;
}

int ShapeExtensionStore::decodeIdentity(DecodeBuffer &decodeBuffer, unsigned char *&buffer,
                                            unsigned int &size, int bigEndian, WriteBuffer *writeBuffer,
                                                ChannelCache *channelCache) const
{
  ClientCache *clientCache = (ClientCache *) channelCache;

  #ifdef DEBUG
  *logofs << name() << ": Decoding full message identity.\n" << logofs_flush;
  #endif

  decodeBuffer.decodeValue(size, 16, 10);

  size <<= 2;

  buffer = writeBuffer -> addMessage(size);

  decodeBuffer.decodeCachedValue(*(buffer + 1), 8,
                     clientCache -> shapeOpcodeCache);

  unsigned int value;

  for (unsigned int i = 0; i < 8 && (i * 2 + 4) < size; i++)
  {
    decodeBuffer.decodeCachedValue(value, 16,
                       *clientCache -> shapeDataCache[i]);

    PutUINT(value, buffer + 4 + (i * 2), bigEndian);
  }

  #ifdef DEBUG
  *logofs << name() << ": Decoded full message identity.\n" << logofs_flush;
  #endif

  return 1;
}

int ShapeExtensionStore::parseIdentity(Message *message, const unsigned char *buffer,
                                           unsigned int size, int bigEndian) const
{
  ShapeExtensionMessage *shapeExtension = (ShapeExtensionMessage *) message;

  shapeExtension -> opcode = *(buffer + 1);

  for (unsigned int i = 0; i < 8; i++)
  {
    if ((i * 2 + 4) < size)
    {
      shapeExtension -> data[i] = GetUINT(buffer + i * 2 + 4, bigEndian); 

      #ifdef DEBUG
      *logofs << name() << ": Parsed data[" << i << "].\n"
              << logofs_flush;
      #endif
    }
    else
    {
      shapeExtension -> data[i] = 0;
    }
  }

  #ifdef DEBUG
  *logofs << name() << ": Parsed identity for message at "
          << this << ".\n" << logofs_flush;
  #endif

  return 1;
}

int ShapeExtensionStore::unparseIdentity(const Message *message, unsigned char *buffer,
                                             unsigned int size, int bigEndian) const
{
  ShapeExtensionMessage *shapeExtension = (ShapeExtensionMessage *) message;

  *(buffer + 1) = shapeExtension -> opcode;

  for (unsigned int i = 0; i < 8 && (i * 2 + 4) < size; i++)
  {
    PutUINT(shapeExtension -> data[i], buffer + i * 2 + 4, bigEndian);
  }

  #ifdef DEBUG
  *logofs << name() << ": Unparsed identity for message at "
          << this << ".\n" << logofs_flush;
  #endif

  return 1;
}

void ShapeExtensionStore::dumpIdentity(const Message *message) const
{
  #ifdef DUMP

  ShapeExtensionMessage *shapeExtension = (ShapeExtensionMessage *) message;

  *logofs << name() << ": Identity opcode " << (unsigned) shapeExtension -> opcode;

  for (int i = 0; i < 8; i++)
  {
    *logofs << ", data[" << i << "] " << shapeExtension -> data[i];
  }

  *logofs << ", size " << shapeExtension -> size_ << ".\n" << logofs_flush;

  #endif
}

void ShapeExtensionStore::identityChecksum(const Message *message, const unsigned char *buffer,
                                               unsigned int size, int bigEndian) const
{
  //
  // Include minor opcode in the checksum. As data
  // offset can be beyond the real end of message,
  // we need to include size or we will match any
  // message of size less or equal to data offset.
  //

  md5_append(md5_state_, buffer + 1,  3);
}

void ShapeExtensionStore::updateIdentity(EncodeBuffer &encodeBuffer, const Message *message,
                                             const Message *cachedMessage,
                                                 ChannelCache *channelCache) const
{
  //
  // Encode the variant part.
  //

  ShapeExtensionMessage *shapeExtension       = (ShapeExtensionMessage *) message;
  ShapeExtensionMessage *cachedShapeExtension = (ShapeExtensionMessage *) cachedMessage;

  ClientCache *clientCache = (ClientCache *) channelCache;

  for (int i = 0; i < 8 && (i * 2 + 4) < shapeExtension -> size_; i++)
  {
    #ifdef TEST
    *logofs << name() << ": Encoding value " << shapeExtension -> data[i]
            << " as data[" << i << "] field.\n" << logofs_flush;
    #endif

    encodeBuffer.encodeCachedValue((unsigned int) shapeExtension -> data[i], 16,
                 *clientCache -> shapeDataCache[i]);

    cachedShapeExtension -> data[i] = shapeExtension -> data[i];
  }
}

void ShapeExtensionStore::updateIdentity(DecodeBuffer &decodeBuffer, const Message *message,
                                             ChannelCache *channelCache) const
{
  ShapeExtensionMessage *shapeExtension = (ShapeExtensionMessage *) message;

  ClientCache *clientCache = (ClientCache *) channelCache;

  unsigned int value;

  for (int i = 0; i < 8 && (i * 2 + 4) < shapeExtension -> size_; i++)
  {
    decodeBuffer.decodeCachedValue(value, 16,
                 *clientCache -> shapeDataCache[i]);

    shapeExtension -> data[i] = (unsigned short) value;

    #ifdef TEST
    *logofs << name() << ": Decoded value " << shapeExtension -> data[i]
            << " as data[" << i << "] field.\n" << logofs_flush;
    #endif
  }
}
