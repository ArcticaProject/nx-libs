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

#ifndef GenericChannel_H
#define GenericChannel_H

#include "Channel.h"

#include "Statistics.h"

#include "GenericReadBuffer.h"

//
// Set the verbosity level.
//

#define PANIC
#undef  TEST
#undef  DEBUG

//
// Define this to log a line when a channel
// is created or destroyed.
//

#undef  REFERENCES

//
// This class implements the client
// side compression of X protocol.
//

class GenericChannel : public Channel
{
  public:

  GenericChannel(Transport *transport, StaticCompressor *compressor);

  virtual ~GenericChannel();

  virtual int handleRead(EncodeBuffer &encodeBuffer, const unsigned char *message,
                             unsigned int length);

  virtual int handleWrite(const unsigned char *message, unsigned int length);


  virtual int handleSplit(EncodeBuffer &encodeBuffer, MessageStore *store,
                              T_store_action action, int position, const unsigned char opcode,
                                  const unsigned char *buffer, const unsigned int size)
  {
    return 0;
  }

  virtual int handleSplit(DecodeBuffer &decodeBuffer, MessageStore *store,
                              T_store_action action, int position, unsigned char &opcode,
                                  unsigned char *&buffer, unsigned int &size)
  {
    return 0;
  }


  virtual int handleSplit(EncodeBuffer &encodeBuffer)
  {
    return 0;
  }

  virtual int handleSplit(DecodeBuffer &decodeBuffer)
  {
    return 0;
  }

  virtual int handleSplitEvent(EncodeBuffer &encodeBuffer, Split *split)
  {
    return 0;
  }

  virtual int handleSplitEvent(DecodeBuffer &decodeBuffer)
  {
    return 0;
  }

  virtual int handleMotion(EncodeBuffer &encodeBuffer)
  {
    return 0;
  }

  virtual int handleCompletion(EncodeBuffer &encodeBuffer);

  virtual int handleConfiguration();

  virtual int handleFinish();

  virtual int handleAsyncEvents()
  {
    return 0;
  }

  virtual int needSplit() const
  {
    return 0;
  }

  virtual int needMotion() const
  {
    return 0;
  }

  virtual T_channel_type getType() const = 0;

  //
  // Initialize the static members.
  //

  static int setReferences();

  protected:

  //
  // Generic channels are considered to be
  // in congestion state as soon as the
  // socket is blocked for write.
  //

  virtual int isCongested()
  {
    return (transport_ -> blocked() == 1);
  }

  virtual int isReliable()
  {
    return 0;
  }

  //
  // Model generic channels' encoding and
  // decoding policy.
  //

  virtual int isCompressed() = 0;

  //
  // Return true if the channel contains
  // time sensitive data.
  //

  virtual int isPrioritized() = 0;

  //
  // Record the protocol bits for the
  // specific service.
  //

  virtual void addProtocolBits(unsigned int bitsIn, unsigned int bitsOut) = 0;

  //
  // Channel's own read buffer.
  //

  GenericReadBuffer readBuffer_;

  private:

  //
  // Keep track of object's creation
  // and deletion.
  //

  #ifdef REFERENCES

  static int references_;

  #endif
};

class CupsChannel : public GenericChannel
{
  public:

  CupsChannel(Transport *transport, StaticCompressor *compressor)

    : GenericChannel(transport, compressor)
  {
  }

  virtual ~CupsChannel()
  {
  }

  protected:

  virtual T_channel_type getType() const
  {
    return channel_cups;
  }

  virtual int isCompressed()
  {
    // Since ProtoStep8 (#issue 108)
    return 0;
  }

  virtual int isPrioritized()
  {
    return 0;
  }

  virtual void addProtocolBits(unsigned int bitsIn,
                                   unsigned int bitsOut)
  {
    statistics -> addCupsBits(bitsIn, bitsOut);
  }
};

class SmbChannel : public GenericChannel
{
  public:

  SmbChannel(Transport *transport, StaticCompressor *compressor)

    : GenericChannel(transport, compressor)
  {
  }

  virtual ~SmbChannel()
  {
  }

  protected:

  virtual T_channel_type getType() const
  {
    return channel_smb;
  }

  virtual int isCompressed()
  {
    // Since ProtoStep8 (#issue 108)
    return 0;
  }

  virtual int isPrioritized()
  {
    return 0;
  }

  virtual void addProtocolBits(unsigned int bitsIn,
                                   unsigned int bitsOut)
  {
    statistics -> addSmbBits(bitsIn, bitsOut);
  }
};

class MediaChannel : public GenericChannel
{
  public:

  MediaChannel(Transport *transport, StaticCompressor *compressor)

    : GenericChannel(transport, compressor)
  {
  }

  virtual ~MediaChannel()
  {
  }

  protected:

  virtual T_channel_type getType() const
  {
    return channel_media;
  }

  //
  // Don't try to compress the media data.
  //

  virtual int isCompressed()
  {
    return 0;
  }

  //
  // Reduce the latency of media channels
  // by setting them as prioritized, even
  // if this will take away bandwidth from
  // the X channels.
  //

  virtual int isPrioritized()
  {
    return 1;
  }

  virtual void addProtocolBits(unsigned int bitsIn,
                                   unsigned int bitsOut)
  {
    statistics -> addMediaBits(bitsIn, bitsOut);
  }
};

class HttpChannel : public GenericChannel
{
  public:

  HttpChannel(Transport *transport, StaticCompressor *compressor)

    : GenericChannel(transport, compressor)
  {
  }

  virtual ~HttpChannel()
  {
  }

  protected:

  virtual T_channel_type getType() const
  {
    return channel_http;
  }

  virtual int isCompressed()
  {
    // Since ProtoStep8 (#issue 108)
    return 0;
  }

  virtual int isPrioritized()
  {
    return 0;
  }

  virtual void addProtocolBits(unsigned int bitsIn,
                                   unsigned int bitsOut)
  {
    statistics -> addHttpBits(bitsIn, bitsOut);
  }
};

class FontChannel : public GenericChannel
{
  public:

  FontChannel(Transport *transport, StaticCompressor *compressor)

    : GenericChannel(transport, compressor)
  {
  }

  virtual ~FontChannel()
  {
  }

  protected:

  virtual T_channel_type getType() const
  {
    return channel_font;
  }

  virtual int isCompressed()
  {
    // Since ProtoStep8 (#issue 108)
    return 0;
  }

  virtual int isPrioritized()
  {
    return 1;
  }

  virtual void addProtocolBits(unsigned int bitsIn,
                                   unsigned int bitsOut)
  {
    statistics -> addFontBits(bitsIn, bitsOut);
  }
};

class SlaveChannel : public GenericChannel
{
  public:

  SlaveChannel(Transport *transport, StaticCompressor *compressor)

    : GenericChannel(transport, compressor)
  {
  }

  virtual ~SlaveChannel()
  {
  }

  protected:

  virtual T_channel_type getType() const
  {
    return channel_slave;
  }

  virtual int isCompressed()
  {
    return 0;
  }

  virtual int isPrioritized()
  {
    return 0;
  }

  virtual void addProtocolBits(unsigned int bitsIn,
                                   unsigned int bitsOut)
  {
    statistics -> addSlaveBits(bitsIn, bitsOut);
  }
};

#endif /* GenericChannel_H */
