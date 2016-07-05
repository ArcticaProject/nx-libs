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

#ifndef PolyFillRectangle_H
#define PolyFillRectangle_H

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

#define POLYFILLRECTANGLE_ENABLE_CACHE                           1
#define POLYFILLRECTANGLE_ENABLE_DATA                            0
#define POLYFILLRECTANGLE_ENABLE_SPLIT                           0
#define POLYFILLRECTANGLE_ENABLE_COMPRESS                        0

#define POLYFILLRECTANGLE_DATA_LIMIT                             2048
#define POLYFILLRECTANGLE_DATA_OFFSET                            12

#define POLYFILLRECTANGLE_CACHE_SLOTS                            4000
#define POLYFILLRECTANGLE_CACHE_THRESHOLD                        5
#define POLYFILLRECTANGLE_CACHE_LOWER_THRESHOLD                  1

//
// The message class.
//

class PolyFillRectangleMessage : public Message
{
  friend class PolyFillRectangleStore;

  public:

  PolyFillRectangleMessage()
  {
  }

  ~PolyFillRectangleMessage()
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

class PolyFillRectangleStore : public MessageStore
{
  //
  // Constructors and destructors.
  //

  public:

  PolyFillRectangleStore() : MessageStore()
  {
    enableCache    = POLYFILLRECTANGLE_ENABLE_CACHE;
    enableData     = POLYFILLRECTANGLE_ENABLE_DATA;
    enableSplit    = POLYFILLRECTANGLE_ENABLE_SPLIT;
    enableCompress = POLYFILLRECTANGLE_ENABLE_COMPRESS;

    dataLimit  = POLYFILLRECTANGLE_DATA_LIMIT;
    dataOffset = POLYFILLRECTANGLE_DATA_OFFSET;

    cacheSlots          = POLYFILLRECTANGLE_CACHE_SLOTS;
    cacheThreshold      = POLYFILLRECTANGLE_CACHE_THRESHOLD;
    cacheLowerThreshold = POLYFILLRECTANGLE_CACHE_LOWER_THRESHOLD;

    messages_ -> resize(cacheSlots);

    for (T_messages::iterator i = messages_ -> begin();
             i < messages_ -> end(); i++)
    {
      *i = NULL;
    }

    temporary_ = NULL;
  }

  virtual ~PolyFillRectangleStore()
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
    return "PolyFillRectangle";
  }

  virtual unsigned char opcode() const
  {
    return X_PolyFillRectangle;
  }

  virtual unsigned int storage() const
  {
    return sizeof(PolyFillRectangleMessage);
  }

  //
  // Message handling methods.
  //

  public:

  virtual Message *create() const
  {
    return new PolyFillRectangleMessage();
  }

  virtual Message *create(const Message &message) const
  {
    return new PolyFillRectangleMessage((const PolyFillRectangleMessage &) message);
  }

  virtual void destroy(Message *message) const
  {
    delete (PolyFillRectangleMessage *) message;
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

#endif /* PolyFillRectangle_H */
