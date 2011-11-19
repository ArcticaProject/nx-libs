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

#ifndef GetImageReply_H
#define GetImageReply_H

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

#define GETIMAGEREPLY_ENABLE_CACHE                       1
#define GETIMAGEREPLY_ENABLE_DATA                        1
#define GETIMAGEREPLY_ENABLE_SPLIT                       0
#define GETIMAGEREPLY_ENABLE_COMPRESS                    1

#define GETIMAGEREPLY_DATA_LIMIT                         1048576 - 32
#define GETIMAGEREPLY_DATA_OFFSET                        32

#define GETIMAGEREPLY_CACHE_SLOTS                        1000
#define GETIMAGEREPLY_CACHE_THRESHOLD                    20
#define GETIMAGEREPLY_CACHE_LOWER_THRESHOLD              2

#define GETIMAGEREPLY_ENABLE_COMPRESS_IF_PROTO_STEP_7    0

//
// The message class.
//

class GetImageReplyMessage : public Message
{
  friend class GetImageReplyStore;

  public:

  GetImageReplyMessage()
  {
  }

  ~GetImageReplyMessage()
  {
  }

  //
  // Put here the fields which constitute 
  // the 'identity' part of the message.
  //

  private:

  unsigned char depth;
  unsigned int  visual;
};

class GetImageReplyStore : public MessageStore
{
  public:

  GetImageReplyStore(StaticCompressor *compressor);

  virtual ~GetImageReplyStore();

  virtual const char *name() const
  {
    return "GetImageReply";
  }

  virtual unsigned char opcode() const
  {
    return X_GetImage;
  }

  virtual unsigned int storage() const
  {
    return sizeof(GetImageReplyMessage);
  }

  //
  // Message handling methods.
  //

  protected:

  virtual Message *create() const
  {
    return new GetImageReplyMessage();
  }

  virtual Message *create(const Message &message) const
  {
    return new GetImageReplyMessage((const GetImageReplyMessage &) message);
  }

  virtual void destroy(Message *message) const
  {
    delete (GetImageReplyMessage *) message;
  }

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

#endif /* GetImageReply_H */
