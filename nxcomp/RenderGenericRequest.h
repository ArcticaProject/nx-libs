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

#ifndef RenderGenericRequest_H
#define RenderGenericRequest_H

#include "Message.h"

//
// Set the verbosity level.
//

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG
#undef  DUMP

class RenderGenericRequestStore : public RenderMinorExtensionStore
{
  public:

  virtual const char *name() const
  {
    return "RenderGenericRequest";
  }

  virtual int identitySize(const unsigned char *buffer, unsigned int size)
  {
    return RENDEREXTENSION_DATA_OFFSET;
  }

  virtual int encodeMessage(EncodeBuffer &encodeBuffer, const unsigned char *buffer,
                                const unsigned int size, int bigEndian,
                                    ChannelCache *channelCache) const;

  virtual int decodeMessage(DecodeBuffer &decodeBuffer, unsigned char *&buffer,
                                unsigned int &size, unsigned char type, int bigEndian,
                                    WriteBuffer *writeBuffer, ChannelCache *channelCache) const;

  virtual void encodeData(EncodeBuffer &encodeBuffer, const unsigned char *buffer,
                              unsigned int size, int bigEndian,
                                  ChannelCache *channelCache) const;

  virtual void decodeData(DecodeBuffer &decodeBuffer, unsigned char *buffer,
                              unsigned int size, int bigEndian,
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
                                    unsigned int size, md5_state_t *md5_state,
                                        int bigEndian) const;
};

#endif /* RenderGenericRequest_H */
