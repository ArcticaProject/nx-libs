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

#ifndef PolyFillArc_H
#define PolyFillArc_H

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

#define POLYFILLARC_ENABLE_CACHE               1
#define POLYFILLARC_ENABLE_DATA                0
#define POLYFILLARC_ENABLE_SPLIT               0
#define POLYFILLARC_ENABLE_COMPRESS            0

#define POLYFILLARC_DATA_LIMIT                 6144
#define POLYFILLARC_DATA_OFFSET                12

#define POLYFILLARC_CACHE_SLOTS                2000
#define POLYFILLARC_CACHE_THRESHOLD            2
#define POLYFILLARC_CACHE_LOWER_THRESHOLD      1

//
// The message class.
//

class PolyFillArcMessage : public Message
{
  friend class PolyFillArcStore;

  public:

  PolyFillArcMessage()
  {
  }

  ~PolyFillArcMessage()
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

class PolyFillArcStore : public MessageStore
{
  //
  // Constructors and destructors.
  //

  public:

  PolyFillArcStore() : MessageStore()
  {
    enableCache    = POLYFILLARC_ENABLE_CACHE;
    enableData     = POLYFILLARC_ENABLE_DATA;
    enableSplit    = POLYFILLARC_ENABLE_SPLIT;
    enableCompress = POLYFILLARC_ENABLE_COMPRESS;

    dataLimit  = POLYFILLARC_DATA_LIMIT;
    dataOffset = POLYFILLARC_DATA_OFFSET;

    cacheSlots          = POLYFILLARC_CACHE_SLOTS;
    cacheThreshold      = POLYFILLARC_CACHE_THRESHOLD;
    cacheLowerThreshold = POLYFILLARC_CACHE_LOWER_THRESHOLD;

    messages_ -> resize(cacheSlots);

    for (T_messages::iterator i = messages_ -> begin();
             i < messages_ -> end(); i++)
    {
      *i = NULL;
    }

    temporary_ = NULL;
  }

  virtual ~PolyFillArcStore()
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
    return "PolyFillArc";
  }

  virtual unsigned char opcode() const
  {
    return X_PolyFillArc;
  }

  virtual unsigned int storage() const
  {
    return sizeof(PolyFillArcMessage);
  }

  //
  // Message handling methods.
  //

  public:

  virtual Message *create() const
  {
    return new PolyFillArcMessage();
  }

  virtual Message *create(const Message &message) const
  {
    return new PolyFillArcMessage((const PolyFillArcMessage &) message);
  }

  virtual void destroy(Message *message) const
  {
    delete (PolyFillArcMessage *) message;
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

#endif /* PolyFillArc_H */
