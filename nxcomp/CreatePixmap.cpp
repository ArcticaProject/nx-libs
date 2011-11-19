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

#include "CreatePixmap.h"

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

//
// Constructors and destructors.
//

CreatePixmapStore::CreatePixmapStore()

  : MessageStore()
{
  enableCache    = CREATEPIXMAP_ENABLE_CACHE;
  enableData     = CREATEPIXMAP_ENABLE_DATA;
  enableSplit    = CREATEPIXMAP_ENABLE_SPLIT;
  enableCompress = CREATEPIXMAP_ENABLE_COMPRESS;

  dataLimit  = CREATEPIXMAP_DATA_LIMIT;
  dataOffset = CREATEPIXMAP_DATA_OFFSET;

  cacheSlots          = CREATEPIXMAP_CACHE_SLOTS;
  cacheThreshold      = CREATEPIXMAP_CACHE_THRESHOLD;
  cacheLowerThreshold = CREATEPIXMAP_CACHE_LOWER_THRESHOLD;

  messages_ -> resize(cacheSlots);

  for (T_messages::iterator i = messages_ -> begin();
           i < messages_ -> end(); i++)
  {
    *i = NULL;
  }

  temporary_ = NULL;
}

CreatePixmapStore::~CreatePixmapStore()
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

int CreatePixmapStore::encodeIdentity(EncodeBuffer &encodeBuffer, const unsigned char *buffer,
                                          const unsigned int size, int bigEndian,
                                              ChannelCache *channelCache) const
{
  ClientCache *clientCache = (ClientCache *) channelCache;

  encodeBuffer.encodeCachedValue(*(buffer + 1), 8,
                     clientCache -> depthCache);

  encodeBuffer.encodeNewXidValue(GetULONG(buffer + 4, bigEndian),
                     clientCache -> lastId, clientCache -> lastIdCache,
                         clientCache -> drawableCache,
                             clientCache -> freeDrawableCache);

  encodeBuffer.encodeXidValue(GetULONG(buffer + 8, bigEndian),
                     clientCache -> windowCache);

  encodeBuffer.encodeCachedValue(GetUINT(buffer + 12, bigEndian), 16,
                     clientCache -> createPixmapXCache, 8);

  encodeBuffer.encodeCachedValue(GetUINT(buffer + 14, bigEndian), 16,
                     clientCache -> createPixmapYCache, 8);

  #ifdef TEST
  *logofs << name() << ": Encoded message. Size is "
          << size << ".\n" << logofs_flush;
  #endif

  return 1;
}

int CreatePixmapStore::decodeIdentity(DecodeBuffer &decodeBuffer, unsigned char *&buffer,
                                          unsigned int &size, int bigEndian, WriteBuffer *writeBuffer,
                                              ChannelCache *channelCache) const
{
  ClientCache *clientCache = (ClientCache *) channelCache;

  unsigned char cValue;
  unsigned int  value;

  size = 16;

  buffer = writeBuffer -> addMessage(size);

  decodeBuffer.decodeCachedValue(cValue, 8,
                     clientCache -> depthCache);

  *(buffer + 1) = cValue;

  decodeBuffer.decodeNewXidValue(value,
                     clientCache -> lastId, clientCache -> lastIdCache,
                         clientCache -> drawableCache,
                             clientCache -> freeDrawableCache);

  PutULONG(value, buffer + 4, bigEndian);

  decodeBuffer.decodeXidValue(value,
                     clientCache -> windowCache);

  PutULONG(value, buffer + 8, bigEndian);

  decodeBuffer.decodeCachedValue(value, 16,
                     clientCache -> createPixmapXCache, 8);

  PutUINT(value, buffer + 12, bigEndian);

  decodeBuffer.decodeCachedValue(value, 16,
                     clientCache -> createPixmapYCache, 8);

  PutUINT(value, buffer + 14, bigEndian);

  #ifdef TEST
  *logofs << name() << ": Decoded message. Size is "
          << size << ".\n" << logofs_flush;
  #endif

  return 1;
}

int CreatePixmapStore::parseIdentity(Message *message, const unsigned char *buffer,
                                         unsigned int size, int bigEndian) const
{
  CreatePixmapMessage *createPixmap = (CreatePixmapMessage *) message;

  createPixmap -> depth = *(buffer + 1);

  createPixmap -> id       = GetULONG(buffer + 4, bigEndian);
  createPixmap -> drawable = GetULONG(buffer + 8, bigEndian);

  createPixmap -> width  = GetUINT(buffer + 12, bigEndian);
  createPixmap -> height = GetUINT(buffer + 14, bigEndian);

  #ifdef TEST
  *logofs << name() << ": Parsed identity. Size is "
          << createPixmap -> size_ << " identity is "
          << createPixmap -> i_size_ << ".\n"
          << logofs_flush;
  #endif

  return 1;
}

int CreatePixmapStore::unparseIdentity(const Message *message, unsigned char *buffer,
                                           unsigned int size, int bigEndian) const
{
  CreatePixmapMessage *createPixmap = (CreatePixmapMessage *) message;

  *(buffer + 1) = createPixmap -> depth;

  PutULONG(createPixmap -> id,       buffer + 4, bigEndian);
  PutULONG(createPixmap -> drawable, buffer + 8, bigEndian);

  PutUINT(createPixmap -> width,  buffer + 12, bigEndian);
  PutUINT(createPixmap -> height, buffer + 14, bigEndian);

  #ifdef TEST
  *logofs << name() << ": Unparsed identity. Size is "
          << createPixmap -> size_ << " identity is "
          << createPixmap -> i_size_ << ".\n"
          << logofs_flush;
  #endif

  return 1;
}

void CreatePixmapStore::dumpIdentity(const Message *message) const
{
  #ifdef DUMP

  #ifdef WARNING
  *logofs << name() << ": WARNING! Dump of identity not implemented.\n"
          << logofs_flush;
  #endif

  #endif
}

void CreatePixmapStore::identityChecksum(const Message *message, const unsigned char *buffer,
                                             unsigned int size, int bigEndian) const
{
  md5_append(md5_state_, buffer + 1, 1);
  md5_append(md5_state_, buffer + 8, 8);
}

void CreatePixmapStore::updateIdentity(EncodeBuffer &encodeBuffer, const Message *message,
                                           const Message *cachedMessage,
                                               ChannelCache *channelCache) const
{
  CreatePixmapMessage *createPixmap       = (CreatePixmapMessage *) message;
  CreatePixmapMessage *cachedCreatePixmap = (CreatePixmapMessage *) cachedMessage;

  ClientCache *clientCache = (ClientCache *) channelCache;

  encodeBuffer.encodeNewXidValue(createPixmap -> id,
                     clientCache -> lastId, clientCache -> lastIdCache,
                         clientCache -> drawableCache,
                             clientCache -> freeDrawableCache);

  cachedCreatePixmap -> id = createPixmap -> id;

  #ifdef TEST
  *logofs << name() << ": Encoded update. Size is "
          << createPixmap -> size_ << " identity is "
          << createPixmap -> i_size_ << ".\n"
          << logofs_flush;
  #endif
}

void CreatePixmapStore::updateIdentity(DecodeBuffer &decodeBuffer, const Message *message,
                                           ChannelCache *channelCache) const
{
  CreatePixmapMessage *createPixmap = (CreatePixmapMessage *) message;

  ClientCache *clientCache = (ClientCache *) channelCache;

  decodeBuffer.decodeNewXidValue(createPixmap -> id,
                     clientCache -> lastId, clientCache -> lastIdCache,
                         clientCache -> drawableCache,
                             clientCache -> freeDrawableCache);

  #ifdef TEST
  *logofs << name() << ": Decoded update. Size is "
          << createPixmap -> size_ << " identity is "
          << createPixmap -> i_size_ << ".\n"
          << logofs_flush;
  #endif
}
