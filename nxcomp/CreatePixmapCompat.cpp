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

#include "CreatePixmapCompat.h"

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

CreatePixmapCompatStore::CreatePixmapCompatStore()

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

CreatePixmapCompatStore::~CreatePixmapCompatStore()
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

int CreatePixmapCompatStore::encodeIdentity(EncodeBuffer &encodeBuffer, const unsigned char *buffer,
                                                const unsigned int size, int bigEndian,
                                                    ChannelCache *channelCache) const
{
  ClientCache *clientCache = (ClientCache *) channelCache;

  encodeBuffer.encodeCachedValue(*(buffer + 1), 8,
                     clientCache -> depthCache);

  encodeBuffer.encodeDiffCachedValue(GetULONG(buffer + 4, bigEndian),
                     clientCache -> createPixmapLastId, 29,
                         clientCache -> createPixmapIdCache, 4);

  encodeBuffer.encodeXidValue(GetULONG(buffer + 8, bigEndian),
                     clientCache -> drawableCache);

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

int CreatePixmapCompatStore::decodeIdentity(DecodeBuffer &decodeBuffer, unsigned char *&buffer,
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

  decodeBuffer.decodeDiffCachedValue(value,
                     clientCache -> createPixmapLastId, 29,
                         clientCache -> createPixmapIdCache, 4);

  PutULONG(value, buffer + 4, bigEndian);

  decodeBuffer.decodeXidValue(value,
                     clientCache -> drawableCache);

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

int CreatePixmapCompatStore::parseIdentity(Message *message, const unsigned char *buffer,
                                               unsigned int size, int bigEndian) const
{
  CreatePixmapCompatMessage *createPixmap = (CreatePixmapCompatMessage *) message;

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

int CreatePixmapCompatStore::unparseIdentity(const Message *message, unsigned char *buffer,
                                                 unsigned int size, int bigEndian) const
{
  CreatePixmapCompatMessage *createPixmap = (CreatePixmapCompatMessage *) message;

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

void CreatePixmapCompatStore::dumpIdentity(const Message *message) const
{
  #ifdef DUMP

  #ifdef WARNING
  *logofs << name() << ": WARNING! Dump of identity not implemented.\n"
          << logofs_flush;
  #endif

  #endif
}

void CreatePixmapCompatStore::identityChecksum(const Message *message, const unsigned char *buffer,
                                                   unsigned int size, int bigEndian) const
{
  md5_append(md5_state_, buffer + 1, 1);
  md5_append(md5_state_, buffer + 8, 8);
}

void CreatePixmapCompatStore::updateIdentity(EncodeBuffer &encodeBuffer, const Message *message,
                                                 const Message *cachedMessage,
                                                     ChannelCache *channelCache) const
{
  CreatePixmapCompatMessage *createPixmap       = (CreatePixmapCompatMessage *) message;
  CreatePixmapCompatMessage *cachedCreatePixmap = (CreatePixmapCompatMessage *) cachedMessage;

  ClientCache *clientCache = (ClientCache *) channelCache;

  encodeBuffer.encodeDiffCachedValue(createPixmap -> id,
                     clientCache -> createPixmapLastId, 29,
                         clientCache -> createPixmapIdCache, 4);

  cachedCreatePixmap -> id = createPixmap -> id;

  encodeBuffer.encodeXidValue(createPixmap -> drawable,
                     clientCache -> drawableCache);

  cachedCreatePixmap -> drawable = createPixmap -> drawable;

  #ifdef TEST
  *logofs << name() << ": Encoded update. Size is "
          << createPixmap -> size_ << " identity is "
          << createPixmap -> i_size_ << ".\n"
          << logofs_flush;
  #endif
}

void CreatePixmapCompatStore::updateIdentity(DecodeBuffer &decodeBuffer, const Message *message,
                                                 ChannelCache *channelCache) const
{
  CreatePixmapCompatMessage *createPixmap = (CreatePixmapCompatMessage *) message;

  ClientCache *clientCache = (ClientCache *) channelCache;

  decodeBuffer.decodeDiffCachedValue(createPixmap -> id,
                     clientCache -> createPixmapLastId, 29,
                         clientCache -> createPixmapIdCache, 4);

  decodeBuffer.decodeXidValue(createPixmap -> drawable,
                     clientCache -> drawableCache);

  #ifdef TEST
  *logofs << name() << ": Decoded update. Size is "
          << createPixmap -> size_ << " identity is "
          << createPixmap -> i_size_ << ".\n"
          << logofs_flush;
  #endif
}
