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

#include "QueryFontReply.h"

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

QueryFontReplyStore::QueryFontReplyStore(StaticCompressor *compressor)

  : MessageStore(compressor)
{
  enableCache    = QUERYFONTREPLY_ENABLE_CACHE;
  enableData     = QUERYFONTREPLY_ENABLE_DATA;
  enableSplit    = QUERYFONTREPLY_ENABLE_SPLIT;
  enableCompress = QUERYFONTREPLY_ENABLE_COMPRESS;

  if (control -> isProtoStep7() == 1)
  {
    enableCompress = QUERYFONTREPLY_ENABLE_COMPRESS_IF_PROTO_STEP_7;
  }

  dataLimit  = QUERYFONTREPLY_DATA_LIMIT;
  dataOffset = QUERYFONTREPLY_DATA_OFFSET;

  cacheSlots          = QUERYFONTREPLY_CACHE_SLOTS;
  cacheThreshold      = QUERYFONTREPLY_CACHE_THRESHOLD;
  cacheLowerThreshold = QUERYFONTREPLY_CACHE_LOWER_THRESHOLD;

  messages_ -> resize(cacheSlots);

  for (T_messages::iterator i = messages_ -> begin();
           i < messages_ -> end(); i++)
  {
    *i = NULL;
  }

  temporary_ = NULL;
}

QueryFontReplyStore::~QueryFontReplyStore()
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

int QueryFontReplyStore::parseIdentity(Message *message, const unsigned char *buffer,
                                          unsigned int size, int bigEndian) const
{
  //
  // Clear the padding bytes.
  //

  unsigned char *pad = (unsigned char *) buffer;

  if (size >= 24)
  {
    PutULONG(0, pad + 20, bigEndian);
  }

  if (size >= 40)
  {
    PutULONG(0, pad + 36, bigEndian);
  }

  //
  // TODO: This doesn't work. Probably these
  // padding bytes are not padding anymore.
  // This is to be investigated.
  //
  // pad += 60;
  //
  // while (pad + 16 <= (buffer + size))
  // {
  //   PutULONG(0, pad + 12, bigEndian);
  //
  //   pad += 16;
  // }
  //

  #ifdef DEBUG
  *logofs << name() << ": Cleaned padding bytes of "
          << "message at " << message << ".\n"
          << logofs_flush;
  #endif

  return 1;
}

int QueryFontReplyStore::unparseIdentity(const Message *message, unsigned char *buffer,
                                            unsigned int size, int bigEndian) const
{
  return 1;
}

void QueryFontReplyStore::dumpIdentity(const Message *message) const
{
  #ifdef DUMP

  QueryFontReplyMessage *queryFontReply = (QueryFontReplyMessage *) message;

  *logofs << name() << ": Identity size " << queryFontReply -> size_ << ".\n";

  #endif
}

void QueryFontReplyStore::identityChecksum(const Message *message, const unsigned char *buffer,
                                               unsigned int size, int bigEndian) const
{
}
