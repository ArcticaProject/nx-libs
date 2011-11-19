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

#ifndef PolySegment_H
#define PolySegment_H

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

#define POLYSEGMENT_ENABLE_CACHE               1
#define POLYSEGMENT_ENABLE_DATA                0
#define POLYSEGMENT_ENABLE_SPLIT               0
#define POLYSEGMENT_ENABLE_COMPRESS            0

#define POLYSEGMENT_DATA_LIMIT                 8192
#define POLYSEGMENT_DATA_OFFSET                12

#define POLYSEGMENT_CACHE_SLOTS                3000
#define POLYSEGMENT_CACHE_THRESHOLD            5
#define POLYSEGMENT_CACHE_LOWER_THRESHOLD      1

//
// The message class.
//

class PolySegmentMessage : public Message
{
  friend class PolySegmentStore;

  public:

  PolySegmentMessage()
  {
  }

  ~PolySegmentMessage()
  {
  }

  //
  // Put here the fields which constitute 
  // the 'identity' part of the message.
  //

  private:

  unsigned int drawable;
  unsigned int gcontext;
};

class PolySegmentStore : public MessageStore
{
  //
  // Constructors and destructors.
  //

  public:

  PolySegmentStore() : MessageStore()
  {
    enableCache    = POLYSEGMENT_ENABLE_CACHE;
    enableData     = POLYSEGMENT_ENABLE_DATA;
    enableSplit    = POLYSEGMENT_ENABLE_SPLIT;
    enableCompress = POLYSEGMENT_ENABLE_COMPRESS;

    dataLimit           = POLYSEGMENT_DATA_LIMIT;
    dataOffset          = POLYSEGMENT_DATA_OFFSET;

    cacheSlots          = POLYSEGMENT_CACHE_SLOTS;
    cacheThreshold      = POLYSEGMENT_CACHE_THRESHOLD;
    cacheLowerThreshold = POLYSEGMENT_CACHE_LOWER_THRESHOLD;

    messages_ -> resize(cacheSlots);

    for (T_messages::iterator i = messages_ -> begin();
             i < messages_ -> end(); i++)
    {
      *i = NULL;
    }

    temporary_ = NULL;
  }

  virtual ~PolySegmentStore()
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
    return "PolySegment";
  }

  virtual unsigned char opcode() const
  {
    return X_PolySegment;
  }

  virtual unsigned int storage() const
  {
    return sizeof(PolySegmentMessage);
  }

  //
  // Message handling methods.
  //

  public:

  virtual Message *create() const
  {
    return new PolySegmentMessage();
  }

  virtual Message *create(const Message &message) const
  {
    return new PolySegmentMessage((const PolySegmentMessage &) message);
  }

  virtual void destroy(Message *message) const
  {
    delete (PolySegmentMessage *) message;
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

#endif /* PolySegment_H */
