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

#include "SetUnpackGeometry.h"

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

SetUnpackGeometryStore::SetUnpackGeometryStore(StaticCompressor *compressor)

  : MessageStore(compressor)
{
  enableCache    = SETUNPACKGEOMETRY_ENABLE_CACHE;
  enableData     = SETUNPACKGEOMETRY_ENABLE_DATA;
  enableSplit    = SETUNPACKGEOMETRY_ENABLE_SPLIT;
  enableCompress = SETUNPACKGEOMETRY_ENABLE_COMPRESS;

  dataLimit  = SETUNPACKGEOMETRY_DATA_LIMIT;
  dataOffset = SETUNPACKGEOMETRY_DATA_OFFSET;

  cacheSlots          = SETUNPACKGEOMETRY_CACHE_SLOTS;
  cacheThreshold      = SETUNPACKGEOMETRY_CACHE_THRESHOLD;
  cacheLowerThreshold = SETUNPACKGEOMETRY_CACHE_LOWER_THRESHOLD;

  messages_ -> resize(cacheSlots);

  for (T_messages::iterator i = messages_ -> begin();
           i < messages_ -> end(); i++)
  {
    *i = NULL;
  }

  temporary_ = NULL;
}

SetUnpackGeometryStore::~SetUnpackGeometryStore()
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

int SetUnpackGeometryStore::encodeIdentity(EncodeBuffer &encodeBuffer, const unsigned char *buffer,
                                            const unsigned int size, int bigEndian,
                                                ChannelCache *channelCache) const
{
  ClientCache *clientCache = (ClientCache *) channelCache;

  #ifdef DEBUG
  *logofs << name() << ": Encoding full message identity.\n" << logofs_flush;
  #endif

  encodeBuffer.encodeCachedValue(*(buffer + 1), 8,
                     clientCache -> resourceCache);

  const unsigned char *nextChar = buffer + 4;

  for (int i = 0; i < 6; i++)
  {
    encodeBuffer.encodeCachedValue(*nextChar++, 8,
                       clientCache -> depthCache);
  }

  encodeBuffer.encodeValue(GetULONG(buffer + 12, bigEndian), 32);
  encodeBuffer.encodeValue(GetULONG(buffer + 16, bigEndian), 32);
  encodeBuffer.encodeValue(GetULONG(buffer + 20, bigEndian), 32);

  #ifdef DEBUG
  *logofs << name() << ": Encoded full message identity.\n" << logofs_flush;
  #endif

  return 1;
}

int SetUnpackGeometryStore::decodeIdentity(DecodeBuffer &decodeBuffer, unsigned char *&buffer,
                                            unsigned int &size, int bigEndian, WriteBuffer *writeBuffer,
                                                ChannelCache *channelCache) const
{
  ClientCache *clientCache = (ClientCache *) channelCache;

  #ifdef DEBUG
  *logofs << name() << ": Decoding full message identity.\n" << logofs_flush;
  #endif

  size   = 24;
  buffer = writeBuffer -> addMessage(size);

  unsigned char cValue;

  decodeBuffer.decodeCachedValue(cValue, 8,
                     clientCache -> resourceCache);
  *(buffer + 1) = cValue;

  decodeBuffer.decodeCachedValue(cValue, 8,
                     clientCache -> depthCache);
  *(buffer + 4) = cValue;

  decodeBuffer.decodeCachedValue(cValue, 8,
                     clientCache -> depthCache);
  *(buffer + 5) = cValue;

  decodeBuffer.decodeCachedValue(cValue, 8,
                     clientCache -> depthCache);
  *(buffer + 6) = cValue;

  decodeBuffer.decodeCachedValue(cValue, 8,
                     clientCache -> depthCache);
  *(buffer + 7) = cValue;

  decodeBuffer.decodeCachedValue(cValue, 8,
                     clientCache -> depthCache);
  *(buffer + 8) = cValue;

  decodeBuffer.decodeCachedValue(cValue, 8,
                     clientCache -> depthCache);
  *(buffer + 9) = cValue;

  unsigned int value;

  decodeBuffer.decodeValue(value, 32);
  PutULONG(value, buffer + 12, bigEndian);

  decodeBuffer.decodeValue(value, 32);
  PutULONG(value, buffer + 16, bigEndian);

  decodeBuffer.decodeValue(value, 32);
  PutULONG(value, buffer + 20, bigEndian);

  #ifdef DEBUG
  *logofs << name() << ": Decoded full message identity.\n" << logofs_flush;
  #endif

  return 1;
}

int SetUnpackGeometryStore::parseIdentity(Message *message, const unsigned char *buffer,
                                           unsigned int size, int bigEndian) const
{
  SetUnpackGeometryMessage *setUnpackGeometry = (SetUnpackGeometryMessage *) message;

  setUnpackGeometry -> client = *(buffer + 1);

  setUnpackGeometry -> depth_1_bpp  = *(buffer + 4);
  setUnpackGeometry -> depth_4_bpp  = *(buffer + 5);
  setUnpackGeometry -> depth_8_bpp  = *(buffer + 6);
  setUnpackGeometry -> depth_16_bpp = *(buffer + 7);
  setUnpackGeometry -> depth_24_bpp = *(buffer + 8);
  setUnpackGeometry -> depth_32_bpp = *(buffer + 9);

  setUnpackGeometry -> red_mask   = GetULONG(buffer + 12, bigEndian);
  setUnpackGeometry -> green_mask = GetULONG(buffer + 16, bigEndian);
  setUnpackGeometry -> blue_mask  = GetULONG(buffer + 20, bigEndian);

  #ifdef DEBUG
  *logofs << name() << ": Parsed identity for message at " << message << ".\n" << logofs_flush;
  #endif

  return 1;
}

int SetUnpackGeometryStore::unparseIdentity(const Message *message, unsigned char *buffer,
                                             unsigned int size, int bigEndian) const
{
  SetUnpackGeometryMessage *setUnpackGeometry = (SetUnpackGeometryMessage *) message;

  *(buffer + 1) = setUnpackGeometry -> client;

  *(buffer + 4) = setUnpackGeometry -> depth_1_bpp;
  *(buffer + 5) = setUnpackGeometry -> depth_4_bpp;
  *(buffer + 6) = setUnpackGeometry -> depth_8_bpp;
  *(buffer + 7) = setUnpackGeometry -> depth_16_bpp;
  *(buffer + 8) = setUnpackGeometry -> depth_24_bpp;
  *(buffer + 9) = setUnpackGeometry -> depth_32_bpp;

  PutULONG(setUnpackGeometry -> red_mask,   buffer + 12, bigEndian);
  PutULONG(setUnpackGeometry -> green_mask, buffer + 16, bigEndian);
  PutULONG(setUnpackGeometry -> blue_mask,  buffer + 20, bigEndian);

  #ifdef DEBUG
  *logofs << name() << ": Unparsed identity for message at " << message << ".\n" << logofs_flush;
  #endif

  return 1;
}

void SetUnpackGeometryStore::dumpIdentity(const Message *message) const
{
  #ifdef DUMP

  SetUnpackGeometryMessage *setUnpackGeometry = (SetUnpackGeometryMessage *) message;

  *logofs << name() << ": Identity client "
          << (unsigned) setUnpackGeometry -> client << " depth_1_bpp "
          << (unsigned) setUnpackGeometry -> depth_1_bpp << " depth_4_bpp "
          << (unsigned int) setUnpackGeometry -> depth_4_bpp << " depth_8_bpp "
          << (unsigned int) setUnpackGeometry -> depth_8_bpp << " depth_16_bpp "
          << (unsigned int) setUnpackGeometry -> depth_16_bpp << " depth_24_bpp "
          << (unsigned int) setUnpackGeometry -> depth_24_bpp << " depth_32_bpp "
          << (unsigned int) setUnpackGeometry -> depth_32_bpp

          << " red_mask " << setUnpackGeometry -> red_mask
          << " green_mask " << setUnpackGeometry -> green_mask
          << " blue_mask " << setUnpackGeometry -> blue_mask

          << " size " << setUnpackGeometry -> size_ << ".\n";

  #endif
}

void SetUnpackGeometryStore::identityChecksum(const Message *message, const unsigned char *buffer,
                                               unsigned int size, int bigEndian) const
{
  md5_append(md5_state_, buffer + 4,  6);
  md5_append(md5_state_, buffer + 12, 12);
}

void SetUnpackGeometryStore::updateIdentity(EncodeBuffer &encodeBuffer, const Message *message,
                                             const Message *cachedMessage,
                                                 ChannelCache *channelCache) const
{
  SetUnpackGeometryMessage *setUnpackGeometry       = (SetUnpackGeometryMessage *) message;
  SetUnpackGeometryMessage *cachedSetUnpackGeometry = (SetUnpackGeometryMessage *) cachedMessage;

  ClientCache *clientCache = (ClientCache *) channelCache;

  #ifdef TEST
  *logofs << name() << ": Encoding value "
          << (unsigned int) setUnpackGeometry -> client
          << " as client field.\n" << logofs_flush;
  #endif

  encodeBuffer.encodeCachedValue(setUnpackGeometry -> client, 8,
                     clientCache -> resourceCache);

  cachedSetUnpackGeometry -> client = setUnpackGeometry -> client;
}

void SetUnpackGeometryStore::updateIdentity(DecodeBuffer &decodeBuffer, const Message *message,
                                             ChannelCache *channelCache) const
{
  SetUnpackGeometryMessage *setUnpackGeometry = (SetUnpackGeometryMessage *) message;

  ClientCache *clientCache = (ClientCache *) channelCache;

  decodeBuffer.decodeCachedValue(setUnpackGeometry -> client, 8,
                     clientCache -> resourceCache);

  #ifdef DEBUG
  *logofs << name() << ": Decoded value "
          << (unsigned int) setUnpackGeometry -> client
          << " as client field.\n" << logofs_flush;
  #endif
}
