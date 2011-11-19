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

#ifndef ShapeExtension_H
#define ShapeExtension_H

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

#define SHAPEEXTENSION_ENABLE_CACHE                       1
#define SHAPEEXTENSION_ENABLE_DATA                        1
#define SHAPEEXTENSION_ENABLE_SPLIT                       0
#define SHAPEEXTENSION_ENABLE_COMPRESS                    1

#define SHAPEEXTENSION_DATA_LIMIT                         3200
#define SHAPEEXTENSION_DATA_OFFSET                        20

#define SHAPEEXTENSION_CACHE_SLOTS                        3000
#define SHAPEEXTENSION_CACHE_THRESHOLD                    10
#define SHAPEEXTENSION_CACHE_LOWER_THRESHOLD              5

#define SHAPEEXTENSION_ENABLE_COMPRESS_IF_PROTO_STEP_7    0

//
// The message class.
//

class ShapeExtensionMessage : public Message
{
  friend class ShapeExtensionStore;

  public:

  ShapeExtensionMessage()
  {
  }

  ~ShapeExtensionMessage()
  {
  }

  //
  // Note for encoding in protocol level 1: we consider
  // for this message a data offset of 4. Bytes from 5
  // to 20, if present, are taken as part of identity
  // and encoded through an array of int caches.
  //

  private:

  unsigned char  opcode;
  unsigned short data[8];
};

class ShapeExtensionStore : public MessageStore
{
  public:

  ShapeExtensionStore(StaticCompressor *compressor);

  virtual ~ShapeExtensionStore();

  virtual const char *name() const
  {
    return "ShapeExtension";
  }

  virtual unsigned char opcode() const
  {
    return opcode_;
  }

  virtual unsigned int storage() const
  {
    return sizeof(ShapeExtensionMessage);
  }

  //
  // Message handling methods.
  //

  public:

  virtual Message *create() const
  {
    return new ShapeExtensionMessage();
  }

  virtual Message *create(const Message &message) const
  {
    return new ShapeExtensionMessage((const ShapeExtensionMessage &) message);
  }

  virtual void destroy(Message *message) const
  {
    delete (ShapeExtensionMessage *) message;
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

  private:

  unsigned char opcode_;
};

#endif /* ShapeExtension_H */
