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

#ifndef SetUnpackGeometry_H
#define SetUnpackGeometry_H

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

#define SETUNPACKGEOMETRY_ENABLE_CACHE               1
#define SETUNPACKGEOMETRY_ENABLE_DATA                0
#define SETUNPACKGEOMETRY_ENABLE_SPLIT               0
#define SETUNPACKGEOMETRY_ENABLE_COMPRESS            0

#define SETUNPACKGEOMETRY_DATA_LIMIT                 24
#define SETUNPACKGEOMETRY_DATA_OFFSET                24

#define SETUNPACKGEOMETRY_CACHE_SLOTS                20
#define SETUNPACKGEOMETRY_CACHE_THRESHOLD            1
#define SETUNPACKGEOMETRY_CACHE_LOWER_THRESHOLD      0

//
// The message class.
//

class SetUnpackGeometryMessage : public Message
{
  friend class SetUnpackGeometryStore;

  public:

  SetUnpackGeometryMessage()
  {
  }

  ~SetUnpackGeometryMessage()
  {
  }

  //
  // Put here the fields which constitute 
  // the 'identity' part of the message.
  //

  private:

  unsigned char client;

  unsigned char depth_1_bpp;
  unsigned char depth_4_bpp;
  unsigned char depth_8_bpp;
  unsigned char depth_16_bpp;
  unsigned char depth_24_bpp;
  unsigned char depth_32_bpp;

  unsigned int red_mask;
  unsigned int green_mask;
  unsigned int blue_mask;
};

class SetUnpackGeometryStore : public MessageStore
{
  public:

  SetUnpackGeometryStore(StaticCompressor *compressor);

  virtual ~SetUnpackGeometryStore();

  virtual const char *name() const
  {
    return "SetUnpackGeometry";
  }

  virtual unsigned char opcode() const
  {
    return X_NXSetUnpackGeometry;
  }

  virtual unsigned int storage() const
  {
    return sizeof(SetUnpackGeometryMessage);
  }

  //
  // Message handling methods.
  //

  protected:

  virtual Message *create() const
  {
    return new SetUnpackGeometryMessage();
  }

  virtual Message *create(const Message &message) const
  {
    return new SetUnpackGeometryMessage((const SetUnpackGeometryMessage &) message);
  }

  virtual void destroy(Message *message) const
  {
    delete (SetUnpackGeometryMessage *) message;
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

#endif /* SetUnpackGeometry_H */
