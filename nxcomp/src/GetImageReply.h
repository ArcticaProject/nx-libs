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

#ifndef GetImageReply_H
#define GetImageReply_H

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

#define GETIMAGEREPLY_ENABLE_CACHE                       1
#define GETIMAGEREPLY_ENABLE_DATA                        1
#define GETIMAGEREPLY_ENABLE_SPLIT                       0

#define GETIMAGEREPLY_DATA_LIMIT                         1048576 - 32
#define GETIMAGEREPLY_DATA_OFFSET                        32

#define GETIMAGEREPLY_CACHE_SLOTS                        1000
#define GETIMAGEREPLY_CACHE_THRESHOLD                    20
#define GETIMAGEREPLY_CACHE_LOWER_THRESHOLD              2

#define GETIMAGEREPLY_ENABLE_COMPRESS_IF_PROTO_STEP_7    0

//
// The message class.
//

class GetImageReplyMessage : public Message
{
  friend class GetImageReplyStore;

  public:

  GetImageReplyMessage()
  {
  }

  ~GetImageReplyMessage()
  {
  }

  //
  // Put here the fields which constitute 
  // the 'identity' part of the message.
  //

  private:

  unsigned char depth;
  unsigned int  visual;
};

class GetImageReplyStore : public MessageStore
{
  public:

  GetImageReplyStore(StaticCompressor *compressor);

  virtual ~GetImageReplyStore();

  virtual const char *name() const
  {
    return "GetImageReply";
  }

  virtual unsigned char opcode() const
  {
    return X_GetImage;
  }

  virtual unsigned int storage() const
  {
    return sizeof(GetImageReplyMessage);
  }

  //
  // Message handling methods.
  //

  protected:

  virtual Message *create() const
  {
    return new GetImageReplyMessage();
  }

  virtual Message *create(const Message &message) const
  {
    return new GetImageReplyMessage((const GetImageReplyMessage &) message);
  }

  virtual void destroy(Message *message) const
  {
    delete (GetImageReplyMessage *) message;
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

#endif /* GetImageReply_H */
