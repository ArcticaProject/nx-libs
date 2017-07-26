/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2011 NoMachine (http://www.nomachine.com)          */
/* Copyright (c) 2008-2014 Oleksandr Shneyder <o.shneyder@phoca-gmbh.de>  */
/* Copyright (c) 2014-2016 Ulrich Sibiller <uli42@gmx.de>                 */
/* Copyright (c) 2014-2016 Mihai Moldovan <ionic@ionic.de>                */
/* Copyright (c) 2011-2016 Mike Gabriel <mike.gabriel@das-netzwerkteam.de>*/
/* Copyright (c) 2015-2016 Qindel Group (http://www.qindel.com)           */
/*                                                                        */
/* NXCOMP, NX protocol compression and NX extensions to this software     */
/* are copyright of the aforementioned persons and companies.             */
/*                                                                        */
/* Redistribution and use of the present software is allowed according    */
/* to terms specified in the file LICENSE.nxcomp which comes in the       */
/* source distribution.                                                   */
/*                                                                        */
/* All rights reserved.                                                   */
/*                                                                        */
/* NOTE: This software has received contributions from various other      */
/* contributors, only the core maintainers and supporters are listed as   */
/* copyright holders. Please contact us, if you feel you should be listed */
/* as copyright holder, as well.                                          */
/*                                                                        */
/**************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

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

  // Since ProtoStep7 (#issue 108)
  enableCompress = QUERYFONTREPLY_ENABLE_COMPRESS_IF_PROTO_STEP_7;

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
