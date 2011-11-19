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
