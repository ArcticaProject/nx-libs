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
#define QUERYFONTREPLY_ENABLE_COMPRESS                    1

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
