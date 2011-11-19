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

#ifndef CopyArea_H
#define CopyArea_H

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

#define COPYAREA_ENABLE_CACHE               1
#define COPYAREA_ENABLE_DATA                0
#define COPYAREA_ENABLE_SPLIT               0
#define COPYAREA_ENABLE_COMPRESS            0

#define COPYAREA_DATA_LIMIT                 0
#define COPYAREA_DATA_OFFSET                28

#define COPYAREA_CACHE_SLOTS                3000
#define COPYAREA_CACHE_THRESHOLD            5
#define COPYAREA_CACHE_LOWER_THRESHOLD      1

//
// The message class.
//

class CopyAreaMessage : public Message
{
  friend class CopyAreaStore;

  public:

  CopyAreaMessage()
  {
  }

  ~CopyAreaMessage()
  {
  }

  //
  // Put here the fields which constitute 
  // the 'identity' part of the message.
  //

  private:

  unsigned int   src_drawable;
  unsigned int   dst_drawable;
  unsigned int   gcontext;
  unsigned short src_x;
  unsigned short src_y;
  unsigned short dst_x;
  unsigned short dst_y;
  unsigned short width;
  unsigned short height;
};

class CopyAreaStore : public MessageStore
{
  //
  // Constructors and destructors.
  //

  public:

  CopyAreaStore() : MessageStore()
  {
    enableCache    = COPYAREA_ENABLE_CACHE;
    enableData     = COPYAREA_ENABLE_DATA;
    enableSplit    = COPYAREA_ENABLE_SPLIT;
    enableCompress = COPYAREA_ENABLE_COMPRESS;

    dataLimit  = COPYAREA_DATA_LIMIT;
    dataOffset = COPYAREA_DATA_OFFSET;

    cacheSlots          = COPYAREA_CACHE_SLOTS;
    cacheThreshold      = COPYAREA_CACHE_THRESHOLD;
    cacheLowerThreshold = COPYAREA_CACHE_LOWER_THRESHOLD;

    messages_ -> resize(cacheSlots);

    for (T_messages::iterator i = messages_ -> begin();
             i < messages_ -> end(); i++)
    {
      *i = NULL;
    }

    temporary_ = NULL;
  }

  virtual ~CopyAreaStore()
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
    return "CopyArea";
  }

  virtual unsigned char opcode() const
  {
    return X_CopyArea;
  }

  virtual unsigned int storage() const
  {
    return sizeof(CopyAreaMessage);
  }

  //
  // Message handling methods.
  //

  public:

  virtual Message *create() const
  {
    return new CopyAreaMessage();
  }

  virtual Message *create(const Message &message) const
  {
    return new CopyAreaMessage((const CopyAreaMessage &) message);
  }

  virtual void destroy(Message *message) const
  {
    delete (CopyAreaMessage *) message;
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

#endif /* CopyArea_H */
