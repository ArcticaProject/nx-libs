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

#ifndef GetImage_H
#define GetImage_H

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

#define GETIMAGE_ENABLE_CACHE               1
#define GETIMAGE_ENABLE_DATA                0
#define GETIMAGE_ENABLE_SPLIT               0
#define GETIMAGE_ENABLE_COMPRESS            0

#define GETIMAGE_DATA_LIMIT                 0
#define GETIMAGE_DATA_OFFSET                20

#define GETIMAGE_CACHE_SLOTS                200
#define GETIMAGE_CACHE_THRESHOLD            1
#define GETIMAGE_CACHE_LOWER_THRESHOLD      0

//
// The message class.
//

class GetImageMessage : public Message
{
  friend class GetImageStore;

  public:

  GetImageMessage()
  {
  }

  ~GetImageMessage()
  {
  }

  //
  // Put here the fields which constitute
  // the 'identity' part of the message.
  //

  private:

  unsigned char      format;
  unsigned int       drawable;
  unsigned short int x;
  unsigned short int y;
  unsigned short int width;
  unsigned short int height;
  unsigned int       plane_mask;
};

class GetImageStore : public MessageStore
{
  //
  // Constructors and destructors.
  //

  public:

  GetImageStore() : MessageStore()
  {
    enableCache    = GETIMAGE_ENABLE_CACHE;
    enableData     = GETIMAGE_ENABLE_DATA;
    enableSplit    = GETIMAGE_ENABLE_SPLIT;
    enableCompress = GETIMAGE_ENABLE_COMPRESS;

    dataLimit  = GETIMAGE_DATA_LIMIT;
    dataOffset = GETIMAGE_DATA_OFFSET;

    cacheSlots          = GETIMAGE_CACHE_SLOTS;
    cacheThreshold      = GETIMAGE_CACHE_THRESHOLD;
    cacheLowerThreshold = GETIMAGE_CACHE_LOWER_THRESHOLD;

    messages_ -> resize(cacheSlots);

    for (T_messages::iterator i = messages_ -> begin();
             i < messages_ -> end(); i++)
    {
      *i = NULL;
    }

    temporary_ = NULL;
  }

  virtual ~GetImageStore()
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
    return "GetImage";
  }

  virtual unsigned char opcode() const
  {
    return X_GetImage;
  }

  virtual unsigned int storage() const
  {
    return sizeof(GetImageMessage);
  }

  //
  // Message handling methods.
  //

  public:

  virtual Message *create() const
  {
    return new GetImageMessage();
  }

  virtual Message *create(const Message &message) const
  {
    return new GetImageMessage((const GetImageMessage &) message);
  }

  virtual void destroy(Message *message) const
  {
    delete (GetImageMessage *) message;
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

#endif /* GetImage_H */
