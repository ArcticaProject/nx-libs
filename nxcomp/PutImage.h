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

#ifndef PutImage_H
#define PutImage_H

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

#define PUTIMAGE_ENABLE_CACHE                            1
#define PUTIMAGE_ENABLE_DATA                             1
#define PUTIMAGE_ENABLE_SPLIT                            1
#define PUTIMAGE_ENABLE_COMPRESS                         1

#define PUTIMAGE_DATA_LIMIT                              262144 - 24
#define PUTIMAGE_DATA_OFFSET                             24

#define PUTIMAGE_CACHE_SLOTS                             6000
#define PUTIMAGE_CACHE_THRESHOLD                         70
#define PUTIMAGE_CACHE_LOWER_THRESHOLD                   50

#define PUTIMAGE_ENABLE_COMPRESS_IF_PROTO_STEP_7         0

#define PUTIMAGE_CACHE_THRESHOLD_IF_PACKED               10
#define PUTIMAGE_CACHE_LOWER_THRESHOLD_IF_PACKED         5

#define PUTIMAGE_CACHE_THRESHOLD_IF_SHADOW               97
#define PUTIMAGE_CACHE_LOWER_THRESHOLD_IF_SHADOW         90

#define PUTIMAGE_ENABLE_SPLIT_IF_PROTO_STEP_8            0

//
// The message class.
//

class PutImageMessage : public Message
{
  friend class PutImageStore;

  public:

  PutImageMessage()
  {
  }

  ~PutImageMessage()
  {
  }

  //
  // Put here the fields which constitute 
  // the 'identity' part of the message.
  //

  private:

  unsigned char  format;
  unsigned char  depth;
  unsigned char  left_pad;
  unsigned short width;
  unsigned short height;
  unsigned int   drawable;
  unsigned int   gcontext;
  unsigned short pos_x;
  unsigned short pos_y;
};

class PutImageStore : public MessageStore
{
  public:

  PutImageStore(StaticCompressor *compressor);

  virtual ~PutImageStore();

  virtual const char *name() const
  {
    return "PutImage";
  }

  virtual unsigned char opcode() const
  {
    return X_PutImage;
  }

  virtual unsigned int storage() const
  {
    return sizeof(PutImageMessage);
  }

  //
  // Message handling methods.
  //

  protected:

  virtual Message *create() const
  {
    return new PutImageMessage();
  }

  virtual Message *create(const Message &message) const
  {
    return new PutImageMessage((const PutImageMessage &) message);
  }

  virtual void destroy(Message *message) const
  {
    delete (PutImageMessage *) message;
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

#endif /* PutImage_H */
