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

#define SETUNPACKALPHA_DATA_LIMIT                         16384

#define SETUNPACKALPHA_CACHE_SLOTS                        2000
#define SETUNPACKALPHA_CACHE_THRESHOLD                    10
#define SETUNPACKALPHA_CACHE_LOWER_THRESHOLD              5

#define SETUNPACKALPHA_DATA_OFFSET_IF_PROTO_STEP_7        16
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
