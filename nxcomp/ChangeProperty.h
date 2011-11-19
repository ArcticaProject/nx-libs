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

#ifndef ChangeProperty_H
#define ChangeProperty_H

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

#define CHANGEPROPERTY_ENABLE_CACHE               1
#define CHANGEPROPERTY_ENABLE_DATA                0
#define CHANGEPROPERTY_ENABLE_SPLIT               0
#define CHANGEPROPERTY_ENABLE_COMPRESS            0

#define CHANGEPROPERTY_DATA_LIMIT                 28688
#define CHANGEPROPERTY_DATA_OFFSET                24

#define CHANGEPROPERTY_CACHE_SLOTS                2000
#define CHANGEPROPERTY_CACHE_THRESHOLD            2
#define CHANGEPROPERTY_CACHE_LOWER_THRESHOLD      1

//
// The message class.
//

class ChangePropertyMessage : public Message
{
  friend class ChangePropertyStore;

  public:

  ChangePropertyMessage()
  {
  }

  ~ChangePropertyMessage()
  {
  }

  //
  // Put here the fields which constitute 
  // the 'identity' part of the message.
  //

  private:

  unsigned char mode;
  unsigned char format;
  unsigned int  window;
  unsigned int  property;
  unsigned int  type;
  unsigned int  length;
};

class ChangePropertyStore : public MessageStore
{
  //
  // Constructors and destructors.
  //

  public:

  ChangePropertyStore() : MessageStore()
  {
    enableCache    = CHANGEPROPERTY_ENABLE_CACHE;
    enableData     = CHANGEPROPERTY_ENABLE_DATA;
    enableSplit    = CHANGEPROPERTY_ENABLE_SPLIT;
    enableCompress = CHANGEPROPERTY_ENABLE_COMPRESS;

    dataLimit  = CHANGEPROPERTY_DATA_LIMIT;
    dataOffset = CHANGEPROPERTY_DATA_OFFSET;

    cacheSlots          = CHANGEPROPERTY_CACHE_SLOTS;
    cacheThreshold      = CHANGEPROPERTY_CACHE_THRESHOLD;
    cacheLowerThreshold = CHANGEPROPERTY_CACHE_LOWER_THRESHOLD;

    messages_ -> resize(cacheSlots);

    for (T_messages::iterator i = messages_ -> begin();
             i < messages_ -> end(); i++)
    {
      *i = NULL;
    }

    temporary_ = NULL;
  }

  virtual ~ChangePropertyStore()
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
    return "ChangeProperty";
  }

  virtual unsigned char opcode() const
  {
    return X_ChangeProperty;
  }

  virtual unsigned int storage() const
  {
    return sizeof(ChangePropertyMessage);
  }

  //
  // Message handling methods.
  //

  public:

  virtual Message *create() const
  {
    return new ChangePropertyMessage();
  }

  virtual Message *create(const Message &message) const
  {
    return new ChangePropertyMessage((const ChangePropertyMessage &) message);
  }

  virtual void destroy(Message *message) const
  {
    delete (ChangePropertyMessage *) message;
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

#endif /* ChangeProperty_H */
