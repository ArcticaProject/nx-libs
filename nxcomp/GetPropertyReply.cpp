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

#include "GetPropertyReply.h"

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

GetPropertyReplyStore::GetPropertyReplyStore(StaticCompressor *compressor)

  : MessageStore(compressor)
{
  enableCache    = GETPROPERTYREPLY_ENABLE_CACHE;
  enableData     = GETPROPERTYREPLY_ENABLE_DATA;
  enableSplit    = GETPROPERTYREPLY_ENABLE_SPLIT;
  enableCompress = GETPROPERTYREPLY_ENABLE_COMPRESS;

  if (control -> isProtoStep7() == 1)
  {
    enableCompress = GETPROPERTYREPLY_ENABLE_COMPRESS_IF_PROTO_STEP_7;
  }

  dataLimit  = GETPROPERTYREPLY_DATA_LIMIT;
  dataOffset = GETPROPERTYREPLY_DATA_OFFSET;

  cacheSlots          = GETPROPERTYREPLY_CACHE_SLOTS;
  cacheThreshold      = GETPROPERTYREPLY_CACHE_THRESHOLD;
  cacheLowerThreshold = GETPROPERTYREPLY_CACHE_LOWER_THRESHOLD;

  messages_ -> resize(cacheSlots);

  for (T_messages::iterator i = messages_ -> begin();
           i < messages_ -> end(); i++)
  {
    *i = NULL;
  }

  temporary_ = NULL;
}

GetPropertyReplyStore::~GetPropertyReplyStore()
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

int GetPropertyReplyStore::encodeIdentity(EncodeBuffer &encodeBuffer, const unsigned char *buffer,
                                              const unsigned int size, int bigEndian,
                                                  ChannelCache *channelCache) const
{
  ServerCache *serverCache = (ServerCache *) channelCache;

  #ifdef DEBUG
  *logofs << name() << ": Encoding full message identity.\n"
          << logofs_flush;
  #endif

  unsigned char format = (unsigned int) *(buffer + 1);

  encodeBuffer.encodeCachedValue(format, 8,
                     serverCache -> getPropertyFormatCache);

  unsigned int numBytes = GetULONG(buffer + 16, bigEndian);

  encodeBuffer.encodeValue(numBytes, 32, 9);

  if (format == 16)
  {
    numBytes <<= 1;
  }
  else if (format == 32)
  {
    numBytes <<= 2;
  }

  encodeBuffer.encodeCachedValue(GetULONG(buffer + 8, bigEndian), 29,
                     serverCache -> getPropertyTypeCache, 9);

  encodeBuffer.encodeValue(GetULONG(buffer + 12, bigEndian), 32, 9);

  #ifdef DEBUG
  *logofs << name() << ": Encoded full message identity.\n"
          << logofs_flush;
  #endif

  return 1;
}

int GetPropertyReplyStore::decodeIdentity(DecodeBuffer &decodeBuffer, unsigned char *&buffer,
                                              unsigned int &size, int bigEndian, WriteBuffer *writeBuffer,
                                                  ChannelCache *channelCache) const
{
  ServerCache *serverCache = (ServerCache *) channelCache;

  #ifdef DEBUG
  *logofs << name() << ": Decoding full message identity.\n"
          << logofs_flush;
  #endif

  unsigned char format;

  decodeBuffer.decodeCachedValue(format, 8,
                     serverCache -> getPropertyFormatCache);

  unsigned int length;

  decodeBuffer.decodeValue(length, 32, 9);

  unsigned int numBytes = length;

  if (format == 16)
  {
    numBytes <<= 1;
  }
  else if (format == 32)
  {
    numBytes <<= 2;
  }

  size = 32 + RoundUp4(numBytes);

  buffer = writeBuffer -> addMessage(size);

  *(buffer + 1) = format;

  PutULONG(length, buffer + 16, bigEndian);

  unsigned int value;

  decodeBuffer.decodeCachedValue(value, 29,
                     serverCache -> getPropertyTypeCache, 9);

  PutULONG(value, buffer + 8, bigEndian);

  decodeBuffer.decodeValue(value, 32, 9);

  PutULONG(value, buffer + 12, bigEndian);

  #ifdef DEBUG
  *logofs << name() << ": Decoded full message identity.\n"
          << logofs_flush;
  #endif

  return 1;
}

int GetPropertyReplyStore::parseIdentity(Message *message, const unsigned char *buffer,
                                             unsigned int size, int bigEndian) const
{
  GetPropertyReplyMessage *getPropertyReply = (GetPropertyReplyMessage *) message;

  getPropertyReply -> format = *(buffer + 1);

  getPropertyReply -> type  = GetULONG(buffer + 8,  bigEndian);
  getPropertyReply -> after = GetULONG(buffer + 12, bigEndian);
  getPropertyReply -> items = GetULONG(buffer + 16, bigEndian);

  //
  // Cleanup the padding bytes.
  //

  unsigned int uiLengthInBytes;
  unsigned int uiFormat;

  if ((int) size > GETPROPERTYREPLY_DATA_OFFSET)
  {
    uiLengthInBytes = getPropertyReply -> items;

    uiFormat = *(buffer + 1);

    #ifdef DEBUG
    *logofs << name() << ": length " << uiLengthInBytes
            << ", format " << uiFormat << ", size "
            << size << ".\n" << logofs_flush;
    #endif

    if (uiFormat == 16)
    {
      uiLengthInBytes <<= 1;
    }
    else if (uiFormat == 32)
    {
      uiLengthInBytes <<= 2;
    }

    unsigned char *end = ((unsigned char *) buffer) + size;
    unsigned char *pad = ((unsigned char *) buffer) + GETPROPERTYREPLY_DATA_OFFSET + uiLengthInBytes;

    CleanData((unsigned char *) pad, end - pad);
  }

  #ifdef DEBUG
  *logofs << name() << ": Parsed identity for message at "
          << message << ".\n" << logofs_flush;
  #endif

  return 1;
}

int GetPropertyReplyStore::unparseIdentity(const Message *message, unsigned char *buffer,
                                               unsigned int size, int bigEndian) const
{
  GetPropertyReplyMessage *getPropertyReply = (GetPropertyReplyMessage *) message;

  *(buffer + 1) = getPropertyReply -> format;

  PutULONG(getPropertyReply -> type,  buffer + 8,  bigEndian);
  PutULONG(getPropertyReply -> after, buffer + 12, bigEndian);
  PutULONG(getPropertyReply -> items, buffer + 16, bigEndian);

  #ifdef DEBUG
  *logofs << name() << ": Unparsed identity for message at "
          << message << ".\n" << logofs_flush;
  #endif

  return 1;
}

void GetPropertyReplyStore::dumpIdentity(const Message *message) const
{
  #ifdef DUMP

  GetPropertyReplyMessage *getPropertyReply = (GetPropertyReplyMessage *) message;

  *logofs << name() << ": Identity format "
          << (unsigned) getPropertyReply -> format << ", type "
          << getPropertyReply -> type << ", after " << getPropertyReply -> after
          << ", items " << getPropertyReply -> items << ", size "
          << getPropertyReply -> size_ << ".\n";

  #endif
}

void GetPropertyReplyStore::identityChecksum(const Message *message, const unsigned char *buffer,
                                                 unsigned int size, int bigEndian) const
{
  //
  // Fields format, type, after, items.
  //

  md5_append(md5_state_, buffer + 1, 1);
  md5_append(md5_state_, buffer + 8, 12);
}

void GetPropertyReplyStore::updateIdentity(EncodeBuffer &encodeBuffer, const Message *message,
                                               const Message *cachedMessage,
                                                ChannelCache *channelCache) const
{
}

void GetPropertyReplyStore::updateIdentity(DecodeBuffer &decodeBuffer, const Message *message,
                                               ChannelCache *channelCache) const
{
}
