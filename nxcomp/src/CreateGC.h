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

#ifndef CreateGC_H
#define CreateGC_H

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

#define CREATEGC_ENABLE_CACHE               1
#define CREATEGC_ENABLE_DATA                0
#define CREATEGC_ENABLE_SPLIT               0
#define CREATEGC_ENABLE_COMPRESS            0

#define CREATEGC_DATA_LIMIT                 144
#define CREATEGC_DATA_OFFSET                16

#define CREATEGC_CACHE_SLOTS                2000
#define CREATEGC_CACHE_THRESHOLD            2
#define CREATEGC_CACHE_LOWER_THRESHOLD      1

//
// The message class.
//

class CreateGCMessage : public Message
{
  friend class CreateGCStore;

  public:

  CreateGCMessage()
  {
  }

  ~CreateGCMessage()
  {
  }

  //
  // Put here the fields which constitute
  // the 'identity' part of the message.
  //

  private:

  unsigned int gcontext;
  unsigned int drawable;
  unsigned int value_mask;
};

class CreateGCStore : public MessageStore
{
  //
  // Constructors and destructors.
  //

  public:

  CreateGCStore() : MessageStore()
  {
    enableCache    = CREATEGC_ENABLE_CACHE;
    enableData     = CREATEGC_ENABLE_DATA;
    enableSplit    = CREATEGC_ENABLE_SPLIT;
    enableCompress = CREATEGC_ENABLE_COMPRESS;

    dataLimit  = CREATEGC_DATA_LIMIT;
    dataOffset = CREATEGC_DATA_OFFSET;

    cacheSlots          = CREATEGC_CACHE_SLOTS;
    cacheThreshold      = CREATEGC_CACHE_THRESHOLD;
    cacheLowerThreshold = CREATEGC_CACHE_LOWER_THRESHOLD;

    messages_ -> resize(cacheSlots);

    for (T_messages::iterator i = messages_ -> begin();
             i < messages_ -> end(); i++)
    {
      *i = NULL;
    }

    temporary_ = NULL;
  }

  virtual ~CreateGCStore()
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
    return "CreateGC";
  }

  virtual unsigned char opcode() const
  {
    return X_CreateGC;
  }

  virtual unsigned int storage() const
  {
    return sizeof(CreateGCMessage);
  }

  //
  // Message handling methods.
  //

  public:

  virtual Message *create() const
  {
    return new CreateGCMessage();
  }

  virtual Message *create(const Message &message) const
  {
    return new CreateGCMessage((const CreateGCMessage &) message);
  }

  virtual void destroy(Message *message) const
  {
    delete (CreateGCMessage *) message;
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

#endif /* CreateGC_H */
