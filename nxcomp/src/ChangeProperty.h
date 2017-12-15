/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2011 NoMachine (http://www.nomachine.com)          */
/* Copyright (c) 2008-2014 Oleksandr Shneyder <o.shneyder@phoca-gmbh.de>  */
/* Copyright (c) 2014-2016 Ulrich Sibiller <uli42@gmx.de>                 */
/* Copyright (c) 2014-2016 Mihai Moldovan <ionic@ionic.de>                */
/* Copyright (c) 2011-2016 Mike Gabriel <mike.gabriel@das-netzwerkteam.de>*/
/* Copyright (c) 2015-2016 Qindel Group (http://www.qindel.com)           */
/*                                                                        */
/* NXCOMP, NX protocol compression and NX extensions to this software     */
/* are copyright of the aforementioned persons and companies.             */
/*                                                                        */
/* Redistribution and use of the present software is allowed according    */
/* to terms specified in the file LICENSE.nxcomp which comes in the       */
/* source distribution.                                                   */
/*                                                                        */
/* All rights reserved.                                                   */
/*                                                                        */
/* NOTE: This software has received contributions from various other      */
/* contributors, only the core maintainers and supporters are listed as   */
/* copyright holders. Please contact us, if you feel you should be listed */
/* as copyright holder, as well.                                          */
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
