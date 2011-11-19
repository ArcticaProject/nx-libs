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

#ifndef ImageText8_H
#define ImageText8_H

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

#define IMAGETEXT8_ENABLE_CACHE               1
#define IMAGETEXT8_ENABLE_DATA                0
#define IMAGETEXT8_ENABLE_SPLIT               0
#define IMAGETEXT8_ENABLE_COMPRESS            0

#define IMAGETEXT8_DATA_LIMIT                 256
#define IMAGETEXT8_DATA_OFFSET                16

#define IMAGETEXT8_CACHE_SLOTS                3000
#define IMAGETEXT8_CACHE_THRESHOLD            5
#define IMAGETEXT8_CACHE_LOWER_THRESHOLD      1

//
// The message class.
//

class ImageText8Message : public Message
{
  friend class ImageText8Store;

  public:

  ImageText8Message()
  {
  }

  ~ImageText8Message()
  {
  }

  //
  // Put here the fields which constitute 
  // the 'identity' part of the message.
  //

  private:

  unsigned char len;

  unsigned int drawable;
  unsigned int gcontext;

  unsigned short x;
  unsigned short y;
};

class ImageText8Store : public MessageStore
{
  //
  // Constructors and destructors.
  //

  public:

  ImageText8Store() : MessageStore()
  {
    enableCache    = IMAGETEXT8_ENABLE_CACHE;
    enableData     = IMAGETEXT8_ENABLE_DATA;
    enableSplit    = IMAGETEXT8_ENABLE_SPLIT;
    enableCompress = IMAGETEXT8_ENABLE_COMPRESS;

    dataLimit            = IMAGETEXT8_DATA_LIMIT;
    dataOffset           = IMAGETEXT8_DATA_OFFSET;

    cacheSlots           = IMAGETEXT8_CACHE_SLOTS;
    cacheThreshold       = IMAGETEXT8_CACHE_THRESHOLD;
    cacheLowerThreshold  = IMAGETEXT8_CACHE_LOWER_THRESHOLD;

    messages_ -> resize(cacheSlots);

    for (T_messages::iterator i = messages_ -> begin();
             i < messages_ -> end(); i++)
    {
      *i = NULL;
    }

    temporary_ = NULL;
  }

  virtual ~ImageText8Store()
  {
    for (T_messages::iterator i = messages_ -> begin();
             i < messages_ -> end(); i++)
    {
      destroy(*i);
    }

    destroy(temporary_);
  }

  virtual const char *name() const
  {
    return "ImageText8";
  }

  virtual unsigned char opcode() const
  {
    return X_ImageText8;
  }

  virtual unsigned int storage() const
  {
    return sizeof(ImageText8Message);
  }

  //
  // Message handling methods.
  //

  public:

  virtual Message *create() const
  {
    return new ImageText8Message();
  }

  virtual Message *create(const Message &message) const
  {
    return new ImageText8Message((const ImageText8Message &) message);
  }

  virtual void destroy(Message *message) const
  {
    delete (ImageText8Message *) message;
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

#endif /* ImageText8_H */
