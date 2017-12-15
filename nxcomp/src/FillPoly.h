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

#ifndef FillPoly_H
#define FillPoly_H

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

#define FILLPOLY_ENABLE_CACHE               1
#define FILLPOLY_ENABLE_DATA                0
#define FILLPOLY_ENABLE_SPLIT               0
#define FILLPOLY_ENABLE_COMPRESS            0

#define FILLPOLY_DATA_LIMIT                 512

#define FILLPOLY_CACHE_SLOTS                2000
#define FILLPOLY_CACHE_THRESHOLD            3
#define FILLPOLY_CACHE_LOWER_THRESHOLD      1

#define FILLPOLY_DATA_OFFSET_IF_PROTO_STEP_8    20

//
// The message class.
//

class FillPolyMessage : public Message
{
  friend class FillPolyStore;

  public:

  FillPolyMessage()
  {
  }

  ~FillPolyMessage()
  {
  }

  //
  // Put here the fields which constitute 
  // the 'identity' part of the message.
  //

  private:

  unsigned char shape;
  unsigned char mode;
  unsigned int  drawable;
  unsigned int  gcontext;

  unsigned short x_origin;
  unsigned short y_origin;
};

class FillPolyStore : public MessageStore
{
  //
  // Constructors and destructors.
  //

  public:

  FillPolyStore() : MessageStore()
  {
    enableCache    = FILLPOLY_ENABLE_CACHE;
    enableData     = FILLPOLY_ENABLE_DATA;
    enableSplit    = FILLPOLY_ENABLE_SPLIT;
    enableCompress = FILLPOLY_ENABLE_COMPRESS;

    dataLimit  = FILLPOLY_DATA_LIMIT;

    // Since ProtoStep8 (#issue 108)
    dataOffset = FILLPOLY_DATA_OFFSET_IF_PROTO_STEP_8;

    cacheSlots          = FILLPOLY_CACHE_SLOTS;
    cacheThreshold      = FILLPOLY_CACHE_THRESHOLD;
    cacheLowerThreshold = FILLPOLY_CACHE_LOWER_THRESHOLD;

    messages_ -> resize(cacheSlots);

    for (T_messages::iterator i = messages_ -> begin();
             i < messages_ -> end(); i++)
    {
      *i = NULL;
    }

    temporary_ = NULL;
  }

  virtual ~FillPolyStore()
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
    return "FillPoly";
  }

  virtual unsigned char opcode() const
  {
    return X_FillPoly;
  }

  virtual unsigned int storage() const
  {
    return sizeof(FillPolyMessage);
  }

  //
  // Message handling methods.
  //

  public:

  virtual Message *create() const
  {
    return new FillPolyMessage();
  }

  virtual Message *create(const Message &message) const
  {
    return new FillPolyMessage((const FillPolyMessage &) message);
  }

  virtual void destroy(Message *message) const
  {
    delete (FillPolyMessage *) message;
  }

  virtual int identitySize(const unsigned char *buffer, unsigned int size)
  {
    // Since ProtoStep8 (#issue 108)
    return (size >= FILLPOLY_DATA_OFFSET_IF_PROTO_STEP_8 ?
                        FILLPOLY_DATA_OFFSET_IF_PROTO_STEP_8 : size);
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

#endif /* FillPoly_H */
