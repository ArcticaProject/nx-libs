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

#ifndef PolyArc_H
#define PolyArc_H

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

#define POLYARC_ENABLE_CACHE               1
#define POLYARC_ENABLE_DATA                0
#define POLYARC_ENABLE_SPLIT               0
#define POLYARC_ENABLE_COMPRESS            0

#define POLYARC_DATA_LIMIT                 1980
#define POLYARC_DATA_OFFSET                12

#define POLYARC_CACHE_SLOTS                2000
#define POLYARC_CACHE_THRESHOLD            2
#define POLYARC_CACHE_LOWER_THRESHOLD      1

//
// The message class.
//

class PolyArcMessage : public Message
{
  friend class PolyArcStore;

  public:

  PolyArcMessage()
  {
  }

  ~PolyArcMessage()
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

class PolyArcStore : public MessageStore
{
  //
  // Constructors and destructors.
  //

  public:

  PolyArcStore() : MessageStore()
  {
    enableCache    = POLYARC_ENABLE_CACHE;
    enableData     = POLYARC_ENABLE_DATA;
    enableSplit    = POLYARC_ENABLE_SPLIT;
    enableCompress = POLYARC_ENABLE_COMPRESS;

    dataLimit  = POLYARC_DATA_LIMIT;
    dataOffset = POLYARC_DATA_OFFSET;

    cacheSlots          = POLYARC_CACHE_SLOTS;
    cacheThreshold      = POLYARC_CACHE_THRESHOLD;
    cacheLowerThreshold = POLYARC_CACHE_LOWER_THRESHOLD;

    messages_ -> resize(cacheSlots);

    for (T_messages::iterator i = messages_ -> begin();
             i < messages_ -> end(); i++)
    {
      *i = NULL;
    }

    temporary_ = NULL;
  }

  virtual ~PolyArcStore()
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
    return "PolyArc";
  }

  virtual unsigned char opcode() const
  {
    return X_PolyArc;
  }

  virtual unsigned int storage() const
  {
    return sizeof(PolyArcMessage);
  }

  //
  // Message handling methods.
  //

  public:

  virtual Message *create() const
  {
    return new PolyArcMessage();
  }

  virtual Message *create(const Message &message) const
  {
    return new PolyArcMessage((const PolyArcMessage &) message);
  }

  virtual void destroy(Message *message) const
  {
    delete (PolyArcMessage *) message;
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

#endif /* PolyArc_H */
