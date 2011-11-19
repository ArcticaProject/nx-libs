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

#include "GenericRequest.h"

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

GenericRequestStore::GenericRequestStore(StaticCompressor *compressor)

  : MessageStore(compressor)
{
  enableCache    = GENERICREQUEST_ENABLE_CACHE;
  enableData     = GENERICREQUEST_ENABLE_DATA;
  enableSplit    = GENERICREQUEST_ENABLE_SPLIT;
  enableCompress = GENERICREQUEST_ENABLE_COMPRESS;

  if (control -> isProtoStep7() == 1)
  {
    enableCompress = GENERICREQUEST_ENABLE_COMPRESS_IF_PROTO_STEP_7;

    enableCompress = 0;
  }

  dataLimit  = GENERICREQUEST_DATA_LIMIT;
  dataOffset = GENERICREQUEST_DATA_OFFSET;

  cacheSlots          = GENERICREQUEST_CACHE_SLOTS;
  cacheThreshold      = GENERICREQUEST_CACHE_THRESHOLD;
  cacheLowerThreshold = GENERICREQUEST_CACHE_LOWER_THRESHOLD;

  messages_ -> resize(cacheSlots);

  for (T_messages::iterator i = messages_ -> begin();
           i < messages_ -> end(); i++)
  {
    *i = NULL;
  }

  temporary_ = NULL;
}

GenericRequestStore::~GenericRequestStore()
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

int GenericRequestStore::encodeIdentity(EncodeBuffer &encodeBuffer, const unsigned char *buffer,
                                            const unsigned int size, int bigEndian,
                                                ChannelCache *channelCache) const
{
  ClientCache *clientCache = (ClientCache *) channelCache;

  #ifdef DEBUG
  *logofs << name() << ": Encoding full message identity.\n" << logofs_flush;
  #endif

  encodeBuffer.encodeValue(size >> 2, 16, 10);

  encodeBuffer.encodeCachedValue(*(buffer + 1), 8,
                     clientCache -> genericRequestOpcodeCache);

  for (unsigned int i = 0; i < 8 && (i * 2 + 4) < size; i++)
  {
    #ifdef DEBUG
    *logofs << name() << ": Encoding data[" << i << "] "
            << "at position " << i * 2 + 4  << " with value "
            << GetUINT(buffer + (i * 2) + 4, bigEndian)
            << ".\n" << logofs_flush;
    #endif

    encodeBuffer.encodeCachedValue(GetUINT(buffer + (i * 2) + 4, bigEndian), 16,
                       *clientCache -> genericRequestDataCache[i]);
  }

  #ifdef DEBUG
  *logofs << name() << ": Encoded full message identity.\n" << logofs_flush;
  #endif

  return 1;
}

int GenericRequestStore::decodeIdentity(DecodeBuffer &decodeBuffer, unsigned char *&buffer,
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
                     clientCache -> genericRequestOpcodeCache);

  unsigned int value;

  for (unsigned int i = 0; i < 8 && (i * 2 + 4) < size; i++)
  {
    decodeBuffer.decodeCachedValue(value, 16,
                       *clientCache -> genericRequestDataCache[i]);

    #ifdef DEBUG
    *logofs << name() << ": Decoding data[" << i << "] "
            << "at position " << i * 2 + 4  << " with value "
            << value << ".\n" << logofs_flush;
    #endif

    PutUINT(value, buffer + 4 + (i * 2), bigEndian);
  }

  #ifdef DEBUG
  *logofs << name() << ": Decoded full message identity.\n" << logofs_flush;
  #endif

  return 1;
}

int GenericRequestStore::parseIdentity(Message *message, const unsigned char *buffer,
                                           unsigned int size, int bigEndian) const
{
  GenericRequestMessage *genericRequest = (GenericRequestMessage *) message;

  genericRequest -> opcode = *(buffer + 1);

  for (unsigned int i = 0; i < 8; i++)
  {
    if ((i * 2 + 4) < size)
    {
      genericRequest -> data[i] = GetUINT(buffer + i * 2 + 4, bigEndian); 

      #ifdef DEBUG
      *logofs << name() << ": Parsed data[" << i << "] "
              << "with value " << genericRequest -> data[i]
              << ".\n" << logofs_flush;
      #endif
    }
    else
    {
      genericRequest -> data[i] = 0;
    }
  }

  #ifdef DEBUG
  *logofs << name() << ": Parsed identity for message at "
          << this << ".\n" << logofs_flush;
  #endif

  return 1;
}

int GenericRequestStore::unparseIdentity(const Message *message, unsigned char *buffer,
                                             unsigned int size, int bigEndian) const
{
  GenericRequestMessage *genericRequest = (GenericRequestMessage *) message;

  *(buffer + 1) = genericRequest -> opcode;

  for (unsigned int i = 0; i < 8 && (i * 2 + 4) < size; i++)
  {
    #ifdef DEBUG
    *logofs << name() << ": Unparsed data[" << i << "] "
            << "with value " << genericRequest -> data[i]
            << ".\n" << logofs_flush;
    #endif

    PutUINT(genericRequest -> data[i], buffer + i * 2 + 4, bigEndian);
  }

  #ifdef DEBUG
  *logofs << name() << ": Unparsed identity for message at "
          << this << ".\n" << logofs_flush;
  #endif

  return 1;
}

void GenericRequestStore::dumpIdentity(const Message *message) const
{
  #ifdef DUMP

  GenericRequestMessage *genericRequest = (GenericRequestMessage *) message;

  *logofs << name() << ": Identity opcode " << (unsigned) genericRequest -> opcode;

  for (int i = 0; i < 8; i++)
  {
    *logofs << ", data[" << i << "] " << genericRequest -> data[i];
  }

  *logofs << ", size " << genericRequest -> size_ << ".\n" << logofs_flush;

  #endif
}

void GenericRequestStore::identityChecksum(const Message *message, const unsigned char *buffer,
                                               unsigned int size, int bigEndian) const
{
  //
  // As data offset can be beyond the real end of
  // the message, we need to include the message's
  // size or we will match any message whose size
  // is less or equal to the data offset.
  //

  md5_append(md5_state_, buffer + 2,  2);
}

void GenericRequestStore::updateIdentity(EncodeBuffer &encodeBuffer, const Message *message,
                                             const Message *cachedMessage,
                                                 ChannelCache *channelCache) const
{
  //
  // Encode the variant part.
  //

  GenericRequestMessage *genericRequest       = (GenericRequestMessage *) message;
  GenericRequestMessage *cachedGenericRequest = (GenericRequestMessage *) cachedMessage;

  ClientCache *clientCache = (ClientCache *) channelCache;

  #ifdef TEST
  *logofs << name() << ": Updating value "
          << (unsigned) genericRequest -> opcode
          << " as opcode field.\n" << logofs_flush;
  #endif

  encodeBuffer.encodeCachedValue((unsigned int) genericRequest -> opcode, 8,
                     clientCache -> genericRequestOpcodeCache);

  cachedGenericRequest -> opcode = genericRequest -> opcode;

  for (int i = 0; i < 8 && (i * 2 + 4) < genericRequest -> size_; i++)
  {
    #ifdef TEST
    *logofs << name() << ": Updating data[" << i << "] "
            << "with value " << genericRequest -> data[i]
            << ".\n" << logofs_flush;
    #endif

    encodeBuffer.encodeCachedValue((unsigned int) genericRequest -> data[i], 16,
                 *clientCache -> genericRequestDataCache[i]);

    cachedGenericRequest -> data[i] = genericRequest -> data[i];
  }
}

void GenericRequestStore::updateIdentity(DecodeBuffer &decodeBuffer, const Message *message,
                                             ChannelCache *channelCache) const
{
  GenericRequestMessage *genericRequest = (GenericRequestMessage *) message;

  ClientCache *clientCache = (ClientCache *) channelCache;

  decodeBuffer.decodeCachedValue(genericRequest -> opcode, 8,
                     clientCache -> genericRequestOpcodeCache);

  #ifdef TEST
  *logofs << name() << ": Updated value "
          << (unsigned) genericRequest -> opcode
          << " as opcode field.\n" << logofs_flush;
  #endif

  unsigned int value;

  for (int i = 0; i < 8 && (i * 2 + 4) < genericRequest -> size_; i++)
  {
    decodeBuffer.decodeCachedValue(value, 16,
                 *clientCache -> genericRequestDataCache[i]);

    genericRequest -> data[i] = (unsigned short) value;

    #ifdef TEST
    *logofs << name() << ": Updated data[" << i << "] "
            << "with value " << genericRequest -> data[i]
            << ".\n" << logofs_flush;
    #endif
  }
}
