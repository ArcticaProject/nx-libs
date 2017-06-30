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

#ifndef ChangeGC_H
#define ChangeGC_H

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

#define CHANGEGC_ENABLE_CACHE                           1
#define CHANGEGC_ENABLE_DATA                            0
#define CHANGEGC_ENABLE_SPLIT                           0
#define CHANGEGC_ENABLE_COMPRESS                        0

#define CHANGEGC_DATA_LIMIT                             144
#define CHANGEGC_DATA_OFFSET                            12

#define CHANGEGC_CACHE_SLOTS                            3000
#define CHANGEGC_CACHE_THRESHOLD                        3
#define CHANGEGC_CACHE_LOWER_THRESHOLD                  1

//
// The message class.
//

class ChangeGCMessage : public Message
{
  friend class ChangeGCStore;

  public:

  ChangeGCMessage()
  {
  }

  ~ChangeGCMessage()
  {
  }

  //
  // Put here the fields which constitute 
  // the 'identity' part of the message.
  //

  private:

  unsigned int gcontext;
  unsigned int value_mask;
};

class ChangeGCStore : public MessageStore
{
  //
  // Constructors and destructors.
  //

  public:

  ChangeGCStore() : MessageStore()
  {
    enableCache    = CHANGEGC_ENABLE_CACHE;
    enableData     = CHANGEGC_ENABLE_DATA;
    enableSplit    = CHANGEGC_ENABLE_SPLIT;
    enableCompress = CHANGEGC_ENABLE_COMPRESS;

    dataLimit  = CHANGEGC_DATA_LIMIT;
    dataOffset = CHANGEGC_DATA_OFFSET;

    cacheSlots          = CHANGEGC_CACHE_SLOTS;
    cacheThreshold      = CHANGEGC_CACHE_THRESHOLD;
    cacheLowerThreshold = CHANGEGC_CACHE_LOWER_THRESHOLD;

    messages_ -> resize(cacheSlots);

    for (T_messages::iterator i = messages_ -> begin();
             i < messages_ -> end(); i++)
    {
      *i = NULL;
    }

    temporary_ = NULL;
  }

  virtual ~ChangeGCStore()
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
    return "ChangeGC";
  }

  virtual unsigned char opcode() const
  {
    return X_ChangeGC;
  }

  virtual unsigned int storage() const
  {
    return sizeof(ChangeGCMessage);
  }

  //
  // Message handling methods.
  //

  public:

  virtual Message *create() const
  {
    return new ChangeGCMessage();
  }

  virtual Message *create(const Message &message) const
  {
    return new ChangeGCMessage((const ChangeGCMessage &) message);
  }

  virtual void destroy(Message *message) const
  {
    delete (ChangeGCMessage *) message;
  }

  virtual int parseIdentity(Message *message, const unsigned char *buffer,
                                unsigned int size, int bigEndian) const;

  virtual int unparseIdentity(const Message *message, unsigned char *buffer,
                                  unsigned int size, int bigEndian) const;

  virtual void identityChecksum(const Message *message, const unsigned char *buffer,
                                    unsigned int size, int bigEndian) const;

  virtual void dumpIdentity(const Message *message) const;

  virtual void updateIdentity(EncodeBuffer &encodeBuffer, const Message *message,
                                  const Message *cachedMessage,
                                      ChannelCache *channelCache) const;

  virtual void updateIdentity(DecodeBuffer &decodeBuffer, const Message *message,
                                  ChannelCache *channelCache) const;
};

#endif /* ChangeGC_H */
