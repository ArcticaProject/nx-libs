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

#include "SetUnpackAlphaCompat.h"

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

SetUnpackAlphaCompatStore::SetUnpackAlphaCompatStore(StaticCompressor *compressor)

  : MessageStore(compressor)
{
  enableCache    = SETUNPACKALPHA_ENABLE_CACHE;
  enableData     = SETUNPACKALPHA_ENABLE_DATA;
  enableSplit    = SETUNPACKALPHA_ENABLE_SPLIT;
  enableCompress = SETUNPACKALPHA_ENABLE_COMPRESS;

  dataLimit  = SETUNPACKALPHA_DATA_LIMIT;
  dataOffset = SETUNPACKALPHA_DATA_OFFSET;

  cacheSlots          = SETUNPACKALPHA_CACHE_SLOTS;
  cacheThreshold      = SETUNPACKALPHA_CACHE_THRESHOLD;
  cacheLowerThreshold = SETUNPACKALPHA_CACHE_LOWER_THRESHOLD;

  messages_ -> resize(cacheSlots);

  for (T_messages::iterator i = messages_ -> begin();
           i < messages_ -> end(); i++)
  {
    *i = NULL;
  }

  temporary_ = NULL;
}

SetUnpackAlphaCompatStore::~SetUnpackAlphaCompatStore()
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

int SetUnpackAlphaCompatStore::encodeIdentity(EncodeBuffer &encodeBuffer, const unsigned char *buffer,
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
  // Entries.
  encodeBuffer.encodeValue(GetULONG(buffer + 4, bigEndian), 32, 9);

  #ifdef DEBUG
  *logofs << name() << ": Encoded full message identity.\n" << logofs_flush;
  #endif

  return 1;
}

int SetUnpackAlphaCompatStore::decodeIdentity(DecodeBuffer &decodeBuffer, unsigned char *&buffer,
                                                  unsigned int &size, int bigEndian, WriteBuffer *writeBuffer,
                                                      ChannelCache *channelCache) const
{
  ClientCache *clientCache = (ClientCache *) channelCache;

  #ifdef DEBUG
  *logofs << name() << ": Decoding full message identity.\n" << logofs_flush;
  #endif

  unsigned int  value;
  unsigned char cValue;

  // Client.
  decodeBuffer.decodeCachedValue(cValue, 8,
                     clientCache -> resourceCache);
  // Entries.
  decodeBuffer.decodeValue(value, 32, 9);

  size = RoundUp4(value) + 8;

  buffer = writeBuffer -> addMessage(size);

  *(buffer + 1) = cValue;

  PutULONG(value, buffer + 4, bigEndian);

  #ifdef DEBUG
  *logofs << name() << ": Decoded full message identity.\n" << logofs_flush;
  #endif

  return 1;
}

int SetUnpackAlphaCompatStore::parseIdentity(Message *message, const unsigned char *buffer,
                                                 unsigned int size, int bigEndian) const
{
  SetUnpackAlphaCompatMessage *setUnpackAlpha = (SetUnpackAlphaCompatMessage *) message;

  setUnpackAlpha -> client = *(buffer + 1);

  setUnpackAlpha -> entries = GetULONG(buffer + 4, bigEndian);

  #ifdef DEBUG
  *logofs << name() << ": Parsed identity for message at " << message << ".\n" << logofs_flush;
  #endif

  return 1;
}

int SetUnpackAlphaCompatStore::unparseIdentity(const Message *message, unsigned char *buffer,
                                                   unsigned int size, int bigEndian) const
{
  SetUnpackAlphaCompatMessage *setUnpackAlpha = (SetUnpackAlphaCompatMessage *) message;

  *(buffer + 1) = setUnpackAlpha -> client;

  PutULONG(setUnpackAlpha -> entries, buffer + 4, bigEndian);

  #ifdef DEBUG
  *logofs << name() << ": Unparsed identity for message at " << message << ".\n" << logofs_flush;
  #endif

  return 1;
}

void SetUnpackAlphaCompatStore::dumpIdentity(const Message *message) const
{
  #ifdef DUMP

  SetUnpackAlphaCompatMessage *setUnpackAlpha = (SetUnpackAlphaCompatMessage *) message;

  *logofs << name() << ": Identity client "
          << (unsigned int) setUnpackAlpha -> client << " entries "
          << setUnpackAlpha -> entries << " size "
          << setUnpackAlpha -> size_ << ".\n";

  #endif
}

void SetUnpackAlphaCompatStore::identityChecksum(const Message *message, const unsigned char *buffer,
                                                     unsigned int size, int bigEndian) const
{
  md5_append(md5_state_, buffer + 4, 4);
}

void SetUnpackAlphaCompatStore::updateIdentity(EncodeBuffer &encodeBuffer, const Message *message,
                                                   const Message *cachedMessage,
                                                       ChannelCache *channelCache) const
{
  SetUnpackAlphaCompatMessage *setUnpackAlpha       = (SetUnpackAlphaCompatMessage *) message;
  SetUnpackAlphaCompatMessage *cachedSetUnpackAlpha = (SetUnpackAlphaCompatMessage *) cachedMessage;

  ClientCache *clientCache = (ClientCache *) channelCache;

  encodeBuffer.encodeCachedValue(setUnpackAlpha -> client, 8,
                     clientCache -> resourceCache);

  cachedSetUnpackAlpha -> client = setUnpackAlpha -> client;

  if (cachedSetUnpackAlpha -> entries != setUnpackAlpha -> entries)
  {
    #ifdef TEST
    *logofs << name() << ": Encoding value " << setUnpackAlpha -> entries
            << " as entries field.\n" << logofs_flush;
    #endif

    encodeBuffer.encodeBoolValue(1);

    encodeBuffer.encodeValue(setUnpackAlpha -> entries, 32, 9);

    cachedSetUnpackAlpha -> entries = setUnpackAlpha -> entries;
  }
  else
  {
    encodeBuffer.encodeBoolValue(0);
  }
}

void SetUnpackAlphaCompatStore::updateIdentity(DecodeBuffer &decodeBuffer, const Message *message,
                                                   ChannelCache *channelCache) const
{
  SetUnpackAlphaCompatMessage *setUnpackAlpha = (SetUnpackAlphaCompatMessage *) message;

  ClientCache *clientCache = (ClientCache *) channelCache;

  unsigned int value;

  decodeBuffer.decodeCachedValue(setUnpackAlpha -> client, 8,
                     clientCache -> resourceCache);

  decodeBuffer.decodeBoolValue(value);

  if (value)
  {
    decodeBuffer.decodeValue(value, 32, 9);

    setUnpackAlpha -> entries = value;

    #ifdef DEBUG
    *logofs << name() << ": Decoded value " << setUnpackAlpha -> entries
            << " as entries field.\n" << logofs_flush;
    #endif
  }
}
