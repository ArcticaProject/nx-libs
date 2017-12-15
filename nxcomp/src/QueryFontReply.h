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

#ifndef QueryFontReply_H
#define QueryFontReply_H

#include "Message.h"

//
// Set the verbosity level.
//

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG
#undef  DUMP

//
// Set default values.
//
#define QUERYFONTREPLY_ENABLE_CACHE                       1
#define QUERYFONTREPLY_ENABLE_DATA                        1
#define QUERYFONTREPLY_ENABLE_SPLIT                       0

#define QUERYFONTREPLY_DATA_LIMIT                         1048576 - 32
#define QUERYFONTREPLY_DATA_OFFSET                        8

#define QUERYFONTREPLY_CACHE_SLOTS                        200
#define QUERYFONTREPLY_CACHE_THRESHOLD                    20
#define QUERYFONTREPLY_CACHE_LOWER_THRESHOLD              5

#define QUERYFONTREPLY_ENABLE_COMPRESS_IF_PROTO_STEP_7    0

//
// The message class.
//

class QueryFontReplyMessage : public Message
{
  friend class QueryFontReplyStore;

  public:

  QueryFontReplyMessage()
  {
  }

  ~QueryFontReplyMessage()
  {
  }
};

class QueryFontReplyStore : public MessageStore
{
  //
  // Constructors and destructors.
  //

  public:

  QueryFontReplyStore(StaticCompressor *compressor);

  virtual ~QueryFontReplyStore();

  virtual const char *name() const
  {
    return "QueryFontReply";
  }

  virtual unsigned char opcode() const
  {
    return X_QueryFont;
  }

  virtual unsigned int storage() const
  {
    return sizeof(QueryFontReplyMessage);
  }

  //
  // Message handling methods.
  //

  protected:

  virtual Message *create() const
  {
    return new QueryFontReplyMessage();
  }

  virtual Message *create(const Message &message) const
  {
    return new QueryFontReplyMessage((const QueryFontReplyMessage &) message);
  }

  virtual void destroy(Message *message) const
  {
    delete (QueryFontReplyMessage *) message;
  }

  virtual int parseIdentity(Message *message, const unsigned char *buffer, 
                                unsigned int size, int bigEndian) const;

  virtual int unparseIdentity(const Message *message, unsigned char *buffer, 
                                  unsigned int size, int bigEndian) const;

  virtual void identityChecksum(const Message *message, const unsigned char *buffer, 
                                    unsigned int size, int bigEndian) const;

  virtual void dumpIdentity(const Message *message) const;
};

#endif /* QueryFontReply_H */
