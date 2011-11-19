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

#ifndef SendEvent_H
#define SendEvent_H

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

#define SENDEVENT_ENABLE_CACHE               1
#define SENDEVENT_ENABLE_DATA                0
#define SENDEVENT_ENABLE_SPLIT               0
#define SENDEVENT_ENABLE_COMPRESS            0

#define SENDEVENT_DATA_LIMIT                 24
#define SENDEVENT_DATA_OFFSET                20

#define SENDEVENT_CACHE_SLOTS                2000
#define SENDEVENT_CACHE_THRESHOLD            2
#define SENDEVENT_CACHE_LOWER_THRESHOLD      1

//
// The message class.
//

class SendEventMessage : public Message
{
  friend class SendEventStore;

  public:

  SendEventMessage()
  {
  }

  ~SendEventMessage()
  {
  }

  //
  // Put here the fields which constitute 
  // the 'identity' part of the message.
  //

  private:

  unsigned char  propagate;
  unsigned int   window;
  unsigned int   mask;

  //
  // These are part of the event data.
  //

  unsigned char  code;
  unsigned char  byte_data;
  unsigned short sequence;
  unsigned int   int_data;
};

class SendEventStore : public MessageStore
{
  //
  // Constructors and destructors.
  //

  public:

  SendEventStore() : MessageStore()
  {
    enableCache    = SENDEVENT_ENABLE_CACHE;
    enableData     = SENDEVENT_ENABLE_DATA;
    enableSplit    = SENDEVENT_ENABLE_SPLIT;
    enableCompress = SENDEVENT_ENABLE_COMPRESS;

    dataLimit  = SENDEVENT_DATA_LIMIT;
    dataOffset = SENDEVENT_DATA_OFFSET;

    cacheSlots           = SENDEVENT_CACHE_SLOTS;
    cacheThreshold       = SENDEVENT_CACHE_THRESHOLD;
    cacheLowerThreshold  = SENDEVENT_CACHE_LOWER_THRESHOLD;

    messages_ -> resize(cacheSlots);

    for (T_messages::iterator i = messages_ -> begin();
             i < messages_ -> end(); i++)
    {
      *i = NULL;
    }

    temporary_ = NULL;
  }

  virtual ~SendEventStore()
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
    return "SendEvent";
  }

  virtual unsigned char opcode() const
  {
    return X_SendEvent;
  }

  virtual unsigned int storage() const
  {
    return sizeof(SendEventMessage);
  }

  //
  // Message handling methods.
  //

  public:

  virtual Message *create() const
  {
    return new SendEventMessage();
  }

  virtual Message *create(const Message &message) const
  {
    return new SendEventMessage((const SendEventMessage &) message);
  }

  virtual void destroy(Message *message) const
  {
    delete (SendEventMessage *) message;
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

#endif /* SendEvent_H */
