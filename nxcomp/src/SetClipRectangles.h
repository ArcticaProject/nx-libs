/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2011 NoMachine (http://www.nomachine.com)          */
/* Copyright (c) 2008-2017 Oleksandr Shneyder <o.shneyder@phoca-gmbh.de>  */
/* Copyright (c) 2014-2022 Ulrich Sibiller <uli42@gmx.de>                 */
/* Copyright (c) 2014-2019 Mihai Moldovan <ionic@ionic.de>                */
/* Copyright (c) 2011-2022 Mike Gabriel <mike.gabriel@das-netzwerkteam.de>*/
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

#ifndef SetClipRectangles_H
#define SetClipRectangles_H

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

#define SETCLIPRECTANGLES_ENABLE_CACHE               1
#define SETCLIPRECTANGLES_ENABLE_DATA                0
#define SETCLIPRECTANGLES_ENABLE_SPLIT               0
#define SETCLIPRECTANGLES_ENABLE_COMPRESS            0

#define SETCLIPRECTANGLES_DATA_LIMIT                 2048
#define SETCLIPRECTANGLES_DATA_OFFSET                12

#define SETCLIPRECTANGLES_CACHE_SLOTS                3000
#define SETCLIPRECTANGLES_CACHE_THRESHOLD            3
#define SETCLIPRECTANGLES_CACHE_LOWER_THRESHOLD      1

//
// The message class.
//

class SetClipRectanglesMessage : public Message
{
  friend class SetClipRectanglesStore;

  public:

  SetClipRectanglesMessage()
  {
  }

  ~SetClipRectanglesMessage()
  {
  }

  //
  // Put here the fields which constitute
  // the 'identity' part of the message.
  //

  private:

  unsigned char  ordering;
  unsigned int   gcontext;
  unsigned short x_origin;
  unsigned short y_origin;
};

class SetClipRectanglesStore : public MessageStore
{
  //
  // Constructors and destructors.
  //

  public:

  SetClipRectanglesStore() : MessageStore()
  {
    enableCache    = SETCLIPRECTANGLES_ENABLE_CACHE;
    enableData     = SETCLIPRECTANGLES_ENABLE_DATA;
    enableSplit    = SETCLIPRECTANGLES_ENABLE_SPLIT;
    enableCompress = SETCLIPRECTANGLES_ENABLE_COMPRESS;

    dataLimit  = SETCLIPRECTANGLES_DATA_LIMIT;
    dataOffset = SETCLIPRECTANGLES_DATA_OFFSET;

    cacheSlots          = SETCLIPRECTANGLES_CACHE_SLOTS;
    cacheThreshold      = SETCLIPRECTANGLES_CACHE_THRESHOLD;
    cacheLowerThreshold = SETCLIPRECTANGLES_CACHE_LOWER_THRESHOLD;

    messages_ -> resize(cacheSlots);

    for (T_messages::iterator i = messages_ -> begin();
             i < messages_ -> end(); i++)
    {
      *i = NULL;
    }

    temporary_ = NULL;
  }

  virtual ~SetClipRectanglesStore()
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
    return "SetClipRectangles";
  }

  virtual unsigned char opcode() const
  {
    return X_SetClipRectangles;
  }

  virtual unsigned int storage() const
  {
    return sizeof(SetClipRectanglesMessage);
  }

  //
  // Message handling methods.
  //

  public:

  virtual Message *create() const
  {
    return new SetClipRectanglesMessage();
  }

  virtual Message *create(const Message &message) const
  {
    return new SetClipRectanglesMessage((const SetClipRectanglesMessage &) message);
  }

  virtual void destroy(Message *message) const
  {
    delete (SetClipRectanglesMessage *) message;
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

#endif /* SetClipRectangles_H */
