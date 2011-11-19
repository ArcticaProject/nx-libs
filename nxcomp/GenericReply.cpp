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

#include "GenericReply.h"

#include "ServerCache.h"

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

GenericReplyStore::GenericReplyStore(StaticCompressor *compressor)

  : MessageStore(compressor)
{
  enableCache    = GENERICREPLY_ENABLE_CACHE;
  enableData     = GENERICREPLY_ENABLE_DATA;
  enableSplit    = GENERICREPLY_ENABLE_SPLIT;
  enableCompress = GENERICREPLY_ENABLE_COMPRESS;

  if (control -> isProtoStep7() == 1)
  {
    enableCompress = GENERICREPLY_ENABLE_COMPRESS_IF_PROTO_STEP_7;
  }

  dataLimit  = GENERICREPLY_DATA_LIMIT;
  dataOffset = GENERICREPLY_DATA_OFFSET;

  cacheSlots          = GENERICREPLY_CACHE_SLOTS;
  cacheThreshold      = GENERICREPLY_CACHE_THRESHOLD;
  cacheLowerThreshold = GENERICREPLY_CACHE_LOWER_THRESHOLD;

  messages_ -> resize(cacheSlots);

  for (T_messages::iterator i = messages_ -> begin();
           i < messages_ -> end(); i++)
  {
    *i = NULL;
  }

  temporary_ = NULL;
}

GenericReplyStore::~GenericReplyStore()
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

int GenericReplyStore::encodeIdentity(EncodeBuffer &encodeBuffer, const unsigned char *buffer,
                                          const unsigned int size, int bigEndian,
                                              ChannelCache *channelCache) const
{
  ServerCache *serverCache = (ServerCache *) channelCache;

  #ifdef DEBUG
  *logofs << name() << ": Encoding full message identity.\n"
          << logofs_flush;
  #endif

  encodeBuffer.encodeValue(GetULONG(buffer + 4, bigEndian), 32, 15);

  encodeBuffer.encodeCachedValue(*(buffer + 1), 8,
                     serverCache -> genericReplyCharCache);

  for (unsigned int i = 0; i < 6; i++)
  {
    encodeBuffer.encodeCachedValue(GetULONG(buffer + i * 4 + 8, bigEndian), 32,
                 *serverCache -> genericReplyIntCache[i]);
  }

  #ifdef DEBUG
  *logofs << name() << ": Encoded full message identity.\n"
          << logofs_flush;
  #endif

  return 1;
}

int GenericReplyStore::decodeIdentity(DecodeBuffer &decodeBuffer, unsigned char *&buffer,
                                          unsigned int &size, int bigEndian, WriteBuffer *writeBuffer,
                                              ChannelCache *channelCache) const
{
  ServerCache *serverCache = (ServerCache *) channelCache;

  #ifdef DEBUG
  *logofs << name() << ": Decoding full message identity.\n"
          << logofs_flush;
  #endif

  decodeBuffer.decodeValue(size, 32, 15);

  size = 32 + (size << 2);

  buffer = writeBuffer -> addMessage(size);

  decodeBuffer.decodeCachedValue(*(buffer + 1), 8,
                     serverCache -> genericReplyCharCache);

  unsigned int value;

  for (unsigned int i = 0; i < 6; i++)
  {
    decodeBuffer.decodeCachedValue(value, 32,
                       *serverCache -> genericReplyIntCache[i]);

    PutULONG(value, buffer + i * 4 + 8, bigEndian);
  }

  #ifdef DEBUG
  *logofs << name() << ": Decoded full message identity.\n"
          << logofs_flush;
  #endif

  return 1;
}

int GenericReplyStore::parseIdentity(Message *message, const unsigned char *buffer,
                                          unsigned int size, int bigEndian) const
{
  GenericReplyMessage *genericReply = (GenericReplyMessage *) message;

  genericReply -> byte_data = *(buffer + 1);

  for (int i = 0; i < 12; i++)
  {
    genericReply -> short_data[i] = GetUINT(buffer + i * 2 + 8, bigEndian);
  }

  #ifdef DEBUG
  *logofs << name() << ": Parsed identity for message at "
          << message << ".\n" << logofs_flush;
  #endif

  return 1;
}

int GenericReplyStore::unparseIdentity(const Message *message, unsigned char *buffer,
                                            unsigned int size, int bigEndian) const
{
  GenericReplyMessage *genericReply = (GenericReplyMessage *) message;

  *(buffer + 1) = genericReply -> byte_data;

  for (int i = 0; i < 12; i++)
  {
    PutUINT(genericReply -> short_data[i], buffer + i * 2 + 8, bigEndian);
  }

  #ifdef DEBUG
  *logofs << name() << ": Unparsed identity for message at "
          << message << ".\n" << logofs_flush;
  #endif

  return 1;
}

void GenericReplyStore::dumpIdentity(const Message *message) const
{
  #ifdef DUMP

  GenericReplyMessage *genericReply = (GenericReplyMessage *) message;

  *logofs << name() << ": Identity byte_data "
          << (unsigned) genericReply -> byte_data;

  for (int i = 0; i < 12; i++)
  {
    *logofs << ", short_data[" << i << "]"
            << (unsigned) genericReply -> short_data[i];
  }

  *logofs << ", size " << genericReply -> size_ << ".\n";

  #endif
}

void GenericReplyStore::identityChecksum(const Message *message, const unsigned char *buffer,
                                              unsigned int size, int bigEndian) const
{
}

void GenericReplyStore::updateIdentity(EncodeBuffer &encodeBuffer, const Message *message,
                                            const Message *cachedMessage,
                                                ChannelCache *channelCache) const
{
  //
  // Encode the variant part.
  //

  GenericReplyMessage *genericReply       = (GenericReplyMessage *) message;
  GenericReplyMessage *cachedGenericReply = (GenericReplyMessage *) cachedMessage;

  ServerCache *serverCache = (ServerCache *) channelCache;

  #ifdef TEST
  *logofs << name() << ": Encoding value "
          << (unsigned int) genericReply -> byte_data
          << " as byte_data field.\n" << logofs_flush;
  #endif

  encodeBuffer.encodeCachedValue(genericReply -> byte_data, 8,
                     serverCache -> genericReplyCharCache);

  cachedGenericReply -> byte_data = genericReply -> byte_data;

  for (unsigned int i = 0; i < 12; i++)
  {
    #ifdef TEST
    *logofs << name() << ": Encoding value " << genericReply -> short_data[i]
            << " as short_data[" << i << "] field.\n" << logofs_flush;
    #endif

    encodeBuffer.encodeCachedValue(genericReply -> short_data[i], 16,
                       *serverCache -> genericReplyIntCache[i]);

    cachedGenericReply -> short_data[i] = genericReply -> short_data[i];
  }
}

void GenericReplyStore::updateIdentity(DecodeBuffer &decodeBuffer, const Message *message,
                                            ChannelCache *channelCache) const
{
  //
  // Decode the variant part.
  //

  GenericReplyMessage *genericReply = (GenericReplyMessage *) message;

  ServerCache *serverCache = (ServerCache *) channelCache;

  decodeBuffer.decodeCachedValue(genericReply -> byte_data, 8,
               serverCache -> genericReplyCharCache);

  #ifdef TEST
  *logofs << name() << ": Decoded value "
          << (unsigned int) genericReply -> byte_data
          << " as byte_data field.\n" << logofs_flush;
  #endif

  unsigned int value;

  for (unsigned int i = 0; i < 12; i++)
  {
    decodeBuffer.decodeCachedValue(value, 16,
                 *serverCache -> genericReplyIntCache[i]);

    genericReply -> short_data[i] = (unsigned short) value;

    #ifdef TEST
    *logofs << name() << ": Decoded value " << genericReply -> short_data[i]
            << " as short_data[" << i << "] field.\n" << logofs_flush;
    #endif
  }
}
