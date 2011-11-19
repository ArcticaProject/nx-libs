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

#ifndef CreatePixmap_H
#define CreatePixmap_H

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

#define CREATEPIXMAP_ENABLE_CACHE               1
#define CREATEPIXMAP_ENABLE_DATA                0
#define CREATEPIXMAP_ENABLE_SPLIT               0
#define CREATEPIXMAP_ENABLE_COMPRESS            0

#define CREATEPIXMAP_DATA_LIMIT                 16
#define CREATEPIXMAP_DATA_OFFSET                16

#define CREATEPIXMAP_CACHE_SLOTS                1000
#define CREATEPIXMAP_CACHE_THRESHOLD            2
#define CREATEPIXMAP_CACHE_LOWER_THRESHOLD      1

//
// The message class.
//

class CreatePixmapMessage : public Message
{
  friend class CreatePixmapStore;

  public:

  CreatePixmapMessage()
  {
  }

  ~CreatePixmapMessage()
  {
  }

  //
  // Put here the fields which constitute 
  // the 'identity' part of the message.
  //

  private:

  unsigned char depth;

  unsigned int id;
  unsigned int drawable;

  unsigned short width;
  unsigned short height;
};

class CreatePixmapStore : public MessageStore
{
  public:

  CreatePixmapStore();

  virtual ~CreatePixmapStore();

  virtual const char *name() const
  {
    return "CreatePixmap";
  }

  virtual unsigned char opcode() const
  {
    return X_CreatePixmap;
  }

  virtual unsigned int storage() const
  {
    return sizeof(CreatePixmapMessage);
  }

  //
  // Message handling methods.
  //

  protected:

  virtual Message *create() const
  {
    return new CreatePixmapMessage();
  }

  virtual Message *create(const Message &message) const
  {
    return new CreatePixmapMessage((const CreatePixmapMessage &) message);
  }

  virtual void destroy(Message *message) const
  {
    delete (CreatePixmapMessage *) message;
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

#endif /* CreatePixmap_H */
