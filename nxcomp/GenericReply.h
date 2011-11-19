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
#define GENERICREPLY_ENABLE_COMPRESS                    1

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
  unsigned int   int_data[6];
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
