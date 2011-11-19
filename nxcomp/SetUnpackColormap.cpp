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

#include "SetUnpackColormap.h"

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

SetUnpackColormapStore::SetUnpackColormapStore(StaticCompressor *compressor)

  : MessageStore(compressor)
{
  enableCache    = SETUNPACKCOLORMAP_ENABLE_CACHE;
  enableData     = SETUNPACKCOLORMAP_ENABLE_DATA;
  enableSplit    = SETUNPACKCOLORMAP_ENABLE_SPLIT;
  enableCompress = SETUNPACKCOLORMAP_ENABLE_COMPRESS_IF_PROTO_STEP_7;

  dataLimit  = SETUNPACKCOLORMAP_DATA_LIMIT;
  dataOffset = SETUNPACKCOLORMAP_DATA_OFFSET_IF_PROTO_STEP_7;

  cacheSlots          = SETUNPACKCOLORMAP_CACHE_SLOTS;
  cacheThreshold      = SETUNPACKCOLORMAP_CACHE_THRESHOLD;
  cacheLowerThreshold = SETUNPACKCOLORMAP_CACHE_LOWER_THRESHOLD;

  if (control -> isProtoStep8() == 1)
  {
    enableSplit = SETUNPACKCOLORMAP_ENABLE_SPLIT_IF_PROTO_STEP_8;
  }

  messages_ -> resize(cacheSlots);

  for (T_messages::iterator i = messages_ -> begin();
           i < messages_ -> end(); i++)
  {
    *i = NULL;
  }

  temporary_ = NULL;
}

SetUnpackColormapStore::~SetUnpackColormapStore()
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

int SetUnpackColormapStore::encodeIdentity(EncodeBuffer &encodeBuffer, const unsigned char *buffer,
                                               const unsigned int size, int bigEndian,
                                                   ChannelCache *channelCache) const
{
  ClientCache *clientCache = (ClientCache *) channelCache;

  #ifdef DEBUG
  *logofs << name() << ": Encoding full message identity.\n" << logofs_flush;
  #endif

  //
  // Encode the source length first because
  // we need it to determine the size of
  // the output buffer.
  //

  // SrcLength.
  encodeBuffer.encodeValue(GetULONG(buffer + 8, bigEndian), 32, 9);

  // Client.
  encodeBuffer.encodeCachedValue(*(buffer + 1), 8,
                     clientCache -> resourceCache);
  // Method.
  encodeBuffer.encodeCachedValue(*(buffer + 4), 8,
                     clientCache -> methodCache);
  // DstLength.
  encodeBuffer.encodeValue(GetULONG(buffer + 12, bigEndian), 32, 9);

  #ifdef DEBUG
  *logofs << name() << ": Encoded full message identity.\n" << logofs_flush;
  #endif

  return 1;
}

int SetUnpackColormapStore::decodeIdentity(DecodeBuffer &decodeBuffer, unsigned char *&buffer,
                                               unsigned int &size, int bigEndian, WriteBuffer *writeBuffer,
                                                   ChannelCache *channelCache) const
{
  ClientCache *clientCache = (ClientCache *) channelCache;

  #ifdef DEBUG
  *logofs << name() << ": Decoding full message identity.\n" << logofs_flush;
  #endif

  unsigned int  value;
  unsigned char cValue;

  // SrcLength.
  decodeBuffer.decodeValue(value, 32, 9);

  size = RoundUp4(value) + 16;

  buffer = writeBuffer -> addMessage(size);

  PutULONG(value, buffer + 8, bigEndian);

  // Client.
  decodeBuffer.decodeCachedValue(cValue, 8,
                     clientCache -> resourceCache);

  *(buffer + 1) = cValue;

  // Method.
  decodeBuffer.decodeCachedValue(cValue, 8,
                     clientCache -> methodCache);

  *(buffer + 4) = cValue;

  // DstLength.
  decodeBuffer.decodeValue(value, 32, 9);

  PutULONG(value, buffer + 12, bigEndian);

  #ifdef DEBUG
  *logofs << name() << ": Decoded full message identity.\n" << logofs_flush;
  #endif

  return 1;
}

int SetUnpackColormapStore::parseIdentity(Message *message, const unsigned char *buffer,
                                              unsigned int size, int bigEndian) const
{
  SetUnpackColormapMessage *setUnpackColormap = (SetUnpackColormapMessage *) message;

  setUnpackColormap -> client = *(buffer + 1);
  setUnpackColormap -> method = *(buffer + 4);

  setUnpackColormap -> src_length = GetULONG(buffer + 8, bigEndian);
  setUnpackColormap -> dst_length = GetULONG(buffer + 12, bigEndian);

  #ifdef DEBUG
  *logofs << name() << ": Parsed identity for message at " << message << ".\n" << logofs_flush;
  #endif

  return 1;
}

int SetUnpackColormapStore::unparseIdentity(const Message *message, unsigned char *buffer,
                                                unsigned int size, int bigEndian) const
{
  SetUnpackColormapMessage *setUnpackColormap = (SetUnpackColormapMessage *) message;

  *(buffer + 1) = setUnpackColormap -> client;
  *(buffer + 4) = setUnpackColormap -> method;

  PutULONG(setUnpackColormap -> src_length, buffer + 8,  bigEndian);
  PutULONG(setUnpackColormap -> dst_length, buffer + 12, bigEndian);

  #ifdef DEBUG
  *logofs << name() << ": Unparsed identity for message at " << message << ".\n" << logofs_flush;
  #endif

  return 1;
}

void SetUnpackColormapStore::dumpIdentity(const Message *message) const
{
  #ifdef DUMP

  SetUnpackColormapMessage *setUnpackColormap = (SetUnpackColormapMessage *) message;

  *logofs << name() << ": Identity client "
          << (unsigned int) setUnpackColormap -> client << " method "
          << (unsigned int) setUnpackColormap -> method << " source length "
          << setUnpackColormap -> src_length << " destination length "
          << setUnpackColormap -> dst_length << " size "
          << setUnpackColormap -> size_ << ".\n";

  #endif
}

void SetUnpackColormapStore::identityChecksum(const Message *message, const unsigned char *buffer,
                                                  unsigned int size, int bigEndian) const
{
  //
  // Include the pack method and the source
  // and destination length.
  //

  md5_append(md5_state_, buffer + 4, 1);
  md5_append(md5_state_, buffer + 8, 8);
}

void SetUnpackColormapStore::updateIdentity(EncodeBuffer &encodeBuffer, const Message *message,
                                                const Message *cachedMessage,
                                                    ChannelCache *channelCache) const
{
  SetUnpackColormapMessage *setUnpackColormap       = (SetUnpackColormapMessage *) message;
  SetUnpackColormapMessage *cachedSetUnpackColormap = (SetUnpackColormapMessage *) cachedMessage;

  ClientCache *clientCache = (ClientCache *) channelCache;

  encodeBuffer.encodeCachedValue(setUnpackColormap -> client, 8,
                     clientCache -> resourceCache);

  cachedSetUnpackColormap -> client = setUnpackColormap -> client;
}

void SetUnpackColormapStore::updateIdentity(DecodeBuffer &decodeBuffer, const Message *message,
                                                ChannelCache *channelCache) const
{
  SetUnpackColormapMessage *setUnpackColormap = (SetUnpackColormapMessage *) message;

  ClientCache *clientCache = (ClientCache *) channelCache;

  decodeBuffer.decodeCachedValue(setUnpackColormap -> client, 8,
                     clientCache -> resourceCache);
}
