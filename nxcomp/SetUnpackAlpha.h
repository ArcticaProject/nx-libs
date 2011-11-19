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

#ifndef SetUnpackAlpha_H
#define SetUnpackAlpha_H

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

#define SETUNPACKALPHA_ENABLE_CACHE                       1
#define SETUNPACKALPHA_ENABLE_DATA                        1
#define SETUNPACKALPHA_ENABLE_SPLIT                       0
#define SETUNPACKALPHA_ENABLE_COMPRESS                    1

#define SETUNPACKALPHA_DATA_LIMIT                         16384
#define SETUNPACKALPHA_DATA_OFFSET                        8

#define SETUNPACKALPHA_CACHE_SLOTS                        2000
#define SETUNPACKALPHA_CACHE_THRESHOLD                    10
#define SETUNPACKALPHA_CACHE_LOWER_THRESHOLD              5

#define SETUNPACKALPHA_DATA_OFFSET_IF_PROTO_STEP_7        16
#define SETUNPACKALPHA_ENABLE_SPLIT_IF_PROTO_STEP_7       1
#define SETUNPACKALPHA_ENABLE_COMPRESS_IF_PROTO_STEP_7    0

#define SETUNPACKALPHA_ENABLE_SPLIT_IF_PROTO_STEP_8       0

//
// The message class.
//

class SetUnpackAlphaMessage : public Message
{
  friend class SetUnpackAlphaStore;

  public:

  SetUnpackAlphaMessage()
  {
  }

  ~SetUnpackAlphaMessage()
  {
  }

  //
  // Put here the fields which constitute 
  // the 'identity' part of the message.
  //

  private:

  unsigned char client;
  unsigned char method;

  unsigned int src_length;
  unsigned int dst_length;
};

class SetUnpackAlphaStore : public MessageStore
{
  public:

  SetUnpackAlphaStore(StaticCompressor *compressor);

  virtual ~SetUnpackAlphaStore();

  virtual const char *name() const
  {
    return "SetUnpackAlpha";
  }

  virtual unsigned char opcode() const
  {
    return X_NXSetUnpackAlpha;
  }

  virtual unsigned int storage() const
  {
    return sizeof(SetUnpackAlphaMessage);
  }

  //
  // Message handling methods.
  //

  protected:

  virtual Message *create() const
  {
    return new SetUnpackAlphaMessage();
  }

  virtual Message *create(const Message &message) const
  {
    return new SetUnpackAlphaMessage((const SetUnpackAlphaMessage &) message);
  }

  virtual void destroy(Message *message) const
  {
    delete (SetUnpackAlphaMessage *) message;
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

#endif /* SetUnpackAlpha_H */
