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

#include "SetUnpackColormapCompat.h"

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

SetUnpackColormapCompatStore::SetUnpackColormapCompatStore(StaticCompressor *compressor)

  : MessageStore(compressor)
{
  enableCache    = SETUNPACKCOLORMAP_ENABLE_CACHE;
  enableData     = SETUNPACKCOLORMAP_ENABLE_DATA;
  enableSplit    = SETUNPACKCOLORMAP_ENABLE_SPLIT;
  enableCompress = SETUNPACKCOLORMAP_ENABLE_COMPRESS;

  dataLimit  = SETUNPACKCOLORMAP_DATA_LIMIT;
  dataOffset = SETUNPACKCOLORMAP_DATA_OFFSET;

  cacheSlots          = SETUNPACKCOLORMAP_CACHE_SLOTS;
  cacheThreshold      = SETUNPACKCOLORMAP_CACHE_THRESHOLD;
  cacheLowerThreshold = SETUNPACKCOLORMAP_CACHE_LOWER_THRESHOLD;

  messages_ -> resize(cacheSlots);

  for (T_messages::iterator i = messages_ -> begin();
           i < messages_ -> end(); i++)
  {
    *i = NULL;
  }

  temporary_ = NULL;
}

SetUnpackColormapCompatStore::~SetUnpackColormapCompatStore()
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

int SetUnpackColormapCompatStore::encodeIdentity(EncodeBuffer &encodeBuffer, const unsigned char *buffer,
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

int SetUnpackColormapCompatStore::decodeIdentity(DecodeBuffer &decodeBuffer, unsigned char *&buffer,
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

  size = (value << 2) + 8;

  buffer = writeBuffer -> addMessage(size);

  *(buffer + 1) = cValue;

  PutULONG(value, buffer + 4, bigEndian);

  #ifdef DEBUG
  *logofs << name() << ": Decoded full message identity.\n" << logofs_flush;
  #endif

  return 1;
}

int SetUnpackColormapCompatStore::parseIdentity(Message *message, const unsigned char *buffer,
                                                    unsigned int size, int bigEndian) const
{
  SetUnpackColormapCompatMessage *setUnpackColormap = (SetUnpackColormapCompatMessage *) message;

  setUnpackColormap -> client = *(buffer + 1);

  setUnpackColormap -> entries = GetULONG(buffer + 4, bigEndian);

  #ifdef DEBUG
  *logofs << name() << ": Parsed identity for message at " << message << ".\n" << logofs_flush;
  #endif

  return 1;
}

int SetUnpackColormapCompatStore::unparseIdentity(const Message *message, unsigned char *buffer,
                                                      unsigned int size, int bigEndian) const
{
  SetUnpackColormapCompatMessage *setUnpackColormap = (SetUnpackColormapCompatMessage *) message;

  *(buffer + 1) = setUnpackColormap -> client;

  PutULONG(setUnpackColormap -> entries, buffer + 4, bigEndian);

  #ifdef DEBUG
  *logofs << name() << ": Unparsed identity for message at " << message << ".\n" << logofs_flush;
  #endif

  return 1;
}

void SetUnpackColormapCompatStore::dumpIdentity(const Message *message) const
{
  #ifdef DUMP

  SetUnpackColormapCompatMessage *setUnpackColormap = (SetUnpackColormapCompatMessage *) message;

  *logofs << name() << ": Identity client "
          << (unsigned int) setUnpackColormap -> client << " entries "
          << setUnpackColormap -> entries << " size "
          << setUnpackColormap -> size_ << ".\n";

  #endif
}

void SetUnpackColormapCompatStore::identityChecksum(const Message *message, const unsigned char *buffer,
                                                        unsigned int size, int bigEndian) const
{
  md5_append(md5_state_, buffer + 4, 4);
}

void SetUnpackColormapCompatStore::updateIdentity(EncodeBuffer &encodeBuffer, const Message *message,
                                                      const Message *cachedMessage,
                                                          ChannelCache *channelCache) const
{
  SetUnpackColormapCompatMessage *setUnpackColormap       = (SetUnpackColormapCompatMessage *) message;
  SetUnpackColormapCompatMessage *cachedSetUnpackColormap = (SetUnpackColormapCompatMessage *) cachedMessage;

  ClientCache *clientCache = (ClientCache *) channelCache;

  #ifdef TEST
  *logofs << name() << ": Encoding value "
          << (unsigned int) setUnpackColormap -> client
          << " as client field.\n" << logofs_flush;
  #endif

  encodeBuffer.encodeCachedValue(setUnpackColormap -> client, 8,
                     clientCache -> resourceCache);

  cachedSetUnpackColormap -> client = setUnpackColormap -> client;

  if (cachedSetUnpackColormap -> entries != setUnpackColormap -> entries)
  {
    #ifdef TEST
    *logofs << name() << ": Encoding value " << setUnpackColormap -> entries
            << " as entries field.\n" << logofs_flush;
    #endif

    encodeBuffer.encodeBoolValue(1);

    encodeBuffer.encodeValue(setUnpackColormap -> entries, 32, 9);

    cachedSetUnpackColormap -> entries = setUnpackColormap -> entries;
  }
  else
  {
    encodeBuffer.encodeBoolValue(0);
  }
}

void SetUnpackColormapCompatStore::updateIdentity(DecodeBuffer &decodeBuffer, const Message *message,
                                                      ChannelCache *channelCache) const
{
  SetUnpackColormapCompatMessage *setUnpackColormap = (SetUnpackColormapCompatMessage *) message;

  ClientCache *clientCache = (ClientCache *) channelCache;

  unsigned int value;

  decodeBuffer.decodeCachedValue(setUnpackColormap -> client, 8,
                     clientCache -> resourceCache);

  #ifdef DEBUG
  *logofs << name() << ": Decoded value "
          << (unsigned int) setUnpackColormap -> client
          << " as client field.\n" << logofs_flush;
  #endif

  decodeBuffer.decodeBoolValue(value);

  if (value)
  {
    decodeBuffer.decodeValue(value, 32, 9);

    setUnpackColormap -> entries = value;

    #ifdef DEBUG
    *logofs << name() << ": Decoded value " << setUnpackColormap -> entries
            << " as entries field.\n" << logofs_flush;
    #endif
  }
}
