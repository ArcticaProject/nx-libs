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

#include "GetImageReply.h"

#include "ServerCache.h"

#include "EncodeBuffer.h"
#include "DecodeBuffer.h"

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

GetImageReplyStore::GetImageReplyStore(StaticCompressor *compressor)

  : MessageStore(compressor)
{
  enableCache    = GETIMAGEREPLY_ENABLE_CACHE;
  enableData     = GETIMAGEREPLY_ENABLE_DATA;
  enableSplit    = GETIMAGEREPLY_ENABLE_SPLIT;
  enableCompress = GETIMAGEREPLY_ENABLE_COMPRESS;

  if (control -> isProtoStep7() == 1)
  {
    enableCompress = GETIMAGEREPLY_ENABLE_COMPRESS_IF_PROTO_STEP_7;
  }

  dataLimit  = GETIMAGEREPLY_DATA_LIMIT;
  dataOffset = GETIMAGEREPLY_DATA_OFFSET;

  cacheSlots          = GETIMAGEREPLY_CACHE_SLOTS;
  cacheThreshold      = GETIMAGEREPLY_CACHE_THRESHOLD;
  cacheLowerThreshold = GETIMAGEREPLY_CACHE_LOWER_THRESHOLD;

  messages_ -> resize(cacheSlots);

  for (T_messages::iterator i = messages_ -> begin();
           i < messages_ -> end(); i++)
  {
    *i = NULL;
  }

  temporary_ = NULL;
}

GetImageReplyStore::~GetImageReplyStore()
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

int GetImageReplyStore::parseIdentity(Message *message, const unsigned char *buffer,
                                          unsigned int size, int bigEndian) const
{
  GetImageReplyMessage *getImageReply = (GetImageReplyMessage *) message;

  //
  // Here is the fingerprint.
  //

  getImageReply -> depth = *(buffer + 1);

  getImageReply -> visual = GetULONG(buffer + 8, bigEndian);

  #ifdef DEBUG
  *logofs << name() << ": Parsed identity for message at "
          << message << ".\n" << logofs_flush;
  #endif

  return 1;
}

int GetImageReplyStore::unparseIdentity(const Message *message, unsigned char *buffer,
                                            unsigned int size, int bigEndian) const
{
  GetImageReplyMessage *getImageReply = (GetImageReplyMessage *) message;

  //
  // Fill all the message's fields.
  //

  *(buffer + 1) = getImageReply -> depth;

  PutULONG(getImageReply -> visual, buffer + 8, bigEndian);

  #ifdef DEBUG
  *logofs << name() << ": Unparsed identity for message at "
          << message << ".\n" << logofs_flush;
  #endif

  return 1;
}

void GetImageReplyStore::dumpIdentity(const Message *message) const
{
  #ifdef DUMP

  GetImageReplyMessage *getImageReply = (GetImageReplyMessage *) message;

  *logofs << name() << ": Identity depth " << (unsigned) getImageReply -> depth
          << ",  visual " << getImageReply -> visual << ", size "
          << getImageReply -> size_ << ".\n";

  #endif
}

void GetImageReplyStore::identityChecksum(const Message *message, const unsigned char *buffer,
                                              unsigned int size, int bigEndian) const
{
  //
  // Field depth.
  //

  md5_append(md5_state_, buffer + 1,  1);
}

void GetImageReplyStore::updateIdentity(EncodeBuffer &encodeBuffer, const Message *message,
                                            const Message *cachedMessage,
                                                ChannelCache *channelCache) const
{
  //
  // Encode the variant part.
  //

  GetImageReplyMessage *getImageReply = (GetImageReplyMessage *) message;

  ServerCache *serverCache = (ServerCache *) channelCache;

  #ifdef TEST
  *logofs << name() << ": Encoding value " << getImageReply -> visual
          << " as visual field.\n" << logofs_flush;
  #endif

  encodeBuffer.encodeCachedValue(getImageReply -> visual, 29, 
                     serverCache -> visualCache);
}

void GetImageReplyStore::updateIdentity(DecodeBuffer &decodeBuffer, const Message *message,
                                            ChannelCache *channelCache) const
{
  //
  // Decode the variant part.
  //

  GetImageReplyMessage *getImageReply = (GetImageReplyMessage *) message;

  ServerCache *serverCache = (ServerCache *) channelCache;

  decodeBuffer.decodeCachedValue(getImageReply -> visual, 29, 
                     serverCache -> visualCache);
}
