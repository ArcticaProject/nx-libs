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

#ifndef PutPackedImage_H
#define PutPackedImage_H

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

#define PUTPACKEDIMAGE_ENABLE_CACHE                            1
#define PUTPACKEDIMAGE_ENABLE_DATA                             1
#define PUTPACKEDIMAGE_ENABLE_COMPRESS                         0

//
// We can't exceed a length of 262144 bytes.
//

#define PUTPACKEDIMAGE_DATA_LIMIT                              262144 - 40
#define PUTPACKEDIMAGE_DATA_OFFSET                             40

#define PUTPACKEDIMAGE_CACHE_SLOTS                             6000
#define PUTPACKEDIMAGE_CACHE_THRESHOLD                         70
#define PUTPACKEDIMAGE_CACHE_LOWER_THRESHOLD                   50

#define PUTPACKEDIMAGE_CACHE_THRESHOLD_IF_PACKED_SHADOW        97
#define PUTPACKEDIMAGE_CACHE_LOWER_THRESHOLD_IF_PACKED_SHADOW  90

#define PUTPACKEDIMAGE_ENABLE_SPLIT_IF_PROTO_STEP_8            0

//
// The message class.
//

class PutPackedImageMessage : public Message
{
  friend class PutPackedImageStore;

  public:

  PutPackedImageMessage()
  {
  }

  ~PutPackedImageMessage()
  {
  }

  //
  // Here are the fields which constitute
  // the 'identity' part of the message.
  //

  private:

  unsigned char  client;

  unsigned int   drawable;
  unsigned int   gcontext;

  unsigned char  format;
  unsigned char  method;

  unsigned char  src_depth;
  unsigned char  dst_depth;

  unsigned int   src_length;
  unsigned int   dst_length;

  short int      src_x;
  short int      src_y;
  unsigned short src_width;
  unsigned short src_height;

  short int      dst_x;
  short int      dst_y;
  unsigned short dst_width;
  unsigned short dst_height;
};

class PutPackedImageStore : public MessageStore
{
  //
  // Constructors and destructors.
  //

  public:

  PutPackedImageStore(StaticCompressor *compressor);

  virtual ~PutPackedImageStore();

  virtual const char *name() const
  {
    return "PutPackedImage";
  }

  virtual unsigned char opcode() const
  {
    return X_NXPutPackedImage;
  }

  virtual unsigned int storage() const
  {
    return sizeof(PutPackedImageMessage);
  }

  //
  // Very special of this class.
  //

  int getPackedSize(const int position) const
  {
    PutPackedImageMessage *message = (PutPackedImageMessage *) (*messages_)[position];

    if (message == NULL)
    {
      return 0;
    }

    return dataOffset + message -> src_length;
  }

  int getUnpackedSize(const int position) const
  {
    PutPackedImageMessage *message = (PutPackedImageMessage *) (*messages_)[position];

    if (message == NULL)
    {
      return 0;
    }

    return dataOffset + message -> dst_length;
  }

  //
  // Message handling methods.
  //

  protected:

  virtual Message *create() const
  {
    return new PutPackedImageMessage();
  }

  virtual Message *create(const Message &message) const
  {
    return new PutPackedImageMessage((const PutPackedImageMessage &) message);
  }

  virtual void destroy(Message *message) const
  {
    delete (PutPackedImageMessage *) message;
  }

  virtual int encodeIdentity(EncodeBuffer &encodeBuffer, const unsigned char *buffer,
                                 const unsigned int size, int bigEndian,
                                     ChannelCache *channelCache) const;

  virtual int decodeIdentity(DecodeBuffer &decodeBuffer, unsigned char *&buffer,
                                 unsigned int &size, int bigEndian, WriteBuffer *writeBuffer,
                                     ChannelCache *channelCache) const;

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

#endif /* PutPackedImage_H */
