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

#ifndef GenericReply_H
#define GenericReply_H

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

#define GENERICREPLY_ENABLE_CACHE                       1
#define GENERICREPLY_ENABLE_DATA                        1
#define GENERICREPLY_ENABLE_SPLIT                       0

#define GENERICREPLY_DATA_LIMIT                         1048576 - 32
#define GENERICREPLY_DATA_OFFSET                        32

#define GENERICREPLY_CACHE_SLOTS                        400
#define GENERICREPLY_CACHE_THRESHOLD                    5
#define GENERICREPLY_CACHE_LOWER_THRESHOLD              1

#define GENERICREPLY_ENABLE_COMPRESS_IF_PROTO_STEP_7    0

//
// The message class.
//

class GenericReplyMessage : public Message
{
  friend class GenericReplyStore;

  public:

  GenericReplyMessage()
  {
  }

  ~GenericReplyMessage()
  {
  }

  //
  // Put here the fields which constitute the
  // 'identity' part of the message. Starting
  // from protocol level 3 we use short data
  // to increase cache efficiency.
  //

  private:

  unsigned char  byte_data;
  unsigned short short_data[12];
};

class GenericReplyStore : public MessageStore
{
  public:

  GenericReplyStore(StaticCompressor *compressor);

  virtual ~GenericReplyStore();

  virtual const char *name() const
  {
    return "GenericReply";
  }

  virtual unsigned char opcode() const
  {
    return X_NXInternalGenericReply;
  }

  virtual unsigned int storage() const
  {
    return sizeof(GenericReplyMessage);
  }

  //
  // Message handling methods.
  //

  protected:

  virtual Message *create() const
  {
    return new GenericReplyMessage();
  }

  virtual Message *create(const Message &message) const
  {
    return new GenericReplyMessage((const GenericReplyMessage &) message);
  }

  virtual void destroy(Message *message) const
  {
    delete (GenericReplyMessage *) message;
  }

  virtual int encodeIdentity(EncodeBuffer &encodeBuffer, const unsigned char *buffer,
                                 const unsigned int size, int bigEndian,
                                     ChannelCache *channelCache) const;

  virtual int decodeIdentity(DecodeBuffer &decodeBuffer, unsigned char *&buffer,
                                 unsigned int &size, int bigEndian, WriteBuffer *writeBuffer,
                                     ChannelCache *channelCache) const;

  virtual int parseIdentity(Message *message, const unsigned char *buffer,
                                unsigned int size, int bigEndian) const;

  virtual int unparseIdentity(const Message *message, unsigned char *buffer,
                                  unsigned int size, int bigEndian) const;

  virtual void updateIdentity(EncodeBuffer &encodeBuffer, const Message *message,
                                  const Message *cachedMessage,
                                      ChannelCache *channelCache) const;

  virtual void updateIdentity(DecodeBuffer &decodeBuffer, const Message *message,
                                  ChannelCache *channelCache) const;

  virtual void identityChecksum(const Message *message, const unsigned char *buffer,
                                    unsigned int size, int bigEndian) const;

  virtual void dumpIdentity(const Message *message) const;
};

#endif /* GenericReply_H */
