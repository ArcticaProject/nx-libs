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

#ifndef RenderMinorExtensionTags_H
#define RenderMinorExtensionTags_H

//
// Set in the message header file.
//

#if MESSAGE_HAS_SIZE

#define MESSAGE_ENCODE_SIZE  encodeSize(encodeBuffer, buffer, size, bigEndian, channelCache)
#define MESSAGE_DECODE_SIZE  decodeSize(decodeBuffer, buffer, size, type, bigEndian, writeBuffer, channelCache)

#else

#define MESSAGE_ENCODE_SIZE
#define MESSAGE_DECODE_SIZE  size = MESSAGE_OFFSET; buffer = writeBuffer -> addMessage(size);

#endif

#if MESSAGE_HAS_DATA

#define MESSAGE_ENCODE_DATA  encodeData(encodeBuffer, buffer, size, bigEndian, channelCache)
#define MESSAGE_DECODE_DATA  decodeData(decodeBuffer, buffer, size, bigEndian, channelCache)

#else

#define MESSAGE_ENCODE_DATA
#define MESSAGE_DECODE_DATA

#endif

//
// Prologue an epilogue of the message
// handling functions.
//

#define MESSAGE_BEGIN_ENCODE_SIZE \
\
void MESSAGE_STORE::encodeSize(EncodeBuffer &encodeBuffer, const unsigned char *buffer, \
                                   const unsigned int size, int bigEndian, \
                                       ChannelCache *channelCache) const \
{

#define MESSAGE_END_ENCODE_SIZE \
\
}

#define MESSAGE_BEGIN_DECODE_SIZE \
\
void MESSAGE_STORE::decodeSize(DecodeBuffer &decodeBuffer, unsigned char *&buffer, \
                                   unsigned int &size, unsigned char type, int bigEndian, \
                                       WriteBuffer *writeBuffer, ChannelCache *channelCache) const \
{

#define MESSAGE_END_DECODE_SIZE \
\
}

#define MESSAGE_BEGIN_ENCODE_MESSAGE \
\
int MESSAGE_STORE::encodeMessage(EncodeBuffer &encodeBuffer, const unsigned char *buffer, \
                                     const unsigned int size, int bigEndian, \
                                         ChannelCache *channelCache) const \
{ \
  MESSAGE_ENCODE_SIZE;


#define MESSAGE_END_ENCODE_MESSAGE \
\
  MESSAGE_ENCODE_DATA; \
\
  return 1; \
}

#define MESSAGE_BEGIN_DECODE_MESSAGE \
\
int MESSAGE_STORE::decodeMessage(DecodeBuffer &decodeBuffer, unsigned char *&buffer, \
                                     unsigned int &size, unsigned char type, int bigEndian, \
                                         WriteBuffer *writeBuffer, ChannelCache *channelCache) const \
{ \
  MESSAGE_DECODE_SIZE;


#define MESSAGE_END_DECODE_MESSAGE \
\
  MESSAGE_DECODE_DATA; \
\
  return 1; \
}

#define MESSAGE_BEGIN_ENCODE_DATA \
\
void MESSAGE_STORE::encodeData(EncodeBuffer &encodeBuffer, const unsigned char *buffer, \
                                   unsigned int size, int bigEndian, \
                                       ChannelCache *channelCache) const \
{

#define MESSAGE_END_ENCODE_DATA \
\
}

#define MESSAGE_BEGIN_DECODE_DATA \
\
void MESSAGE_STORE::decodeData(DecodeBuffer &decodeBuffer, unsigned char *buffer, \
                                   unsigned int size, int bigEndian, \
                                       ChannelCache *channelCache) const \
{

#define MESSAGE_END_DECODE_DATA \
\
}

#define MESSAGE_BEGIN_PARSE_IDENTITY \
\
int MESSAGE_STORE::parseIdentity(Message *message, const unsigned char *buffer, \
                                     unsigned int size, int bigEndian) const \
{

#define MESSAGE_END_PARSE_IDENTITY \
\
  return 1; \
\
}

#define MESSAGE_BEGIN_UNPARSE_IDENTITY \
\
int MESSAGE_STORE::unparseIdentity(const Message *message, unsigned char *buffer, \
                                       unsigned int size, int bigEndian) const \
{

#define MESSAGE_END_UNPARSE_IDENTITY \
\
  return 1; \
\
}

#define MESSAGE_BEGIN_IDENTITY_CHECKSUM \
\
void MESSAGE_STORE::identityChecksum(const Message *message, const unsigned char *buffer, \
                                         unsigned int size, md5_state_t *md5_state, \
                                             int bigEndian) const \
{

#define MESSAGE_END_IDENTITY_CHECKSUM \
\
}

#define MESSAGE_BEGIN_ENCODE_UPDATE \
\
void MESSAGE_STORE::updateIdentity(EncodeBuffer &encodeBuffer, const Message *message, \
                                       const Message *cachedMessage, \
                                           ChannelCache *channelCache) const \
{

#define MESSAGE_END_ENCODE_UPDATE \
\
}

#define MESSAGE_BEGIN_DECODE_UPDATE \
\
void MESSAGE_STORE::updateIdentity(DecodeBuffer &decodeBuffer, const Message *message, \
                                       ChannelCache *channelCache) const \
{

#define MESSAGE_END_DECODE_UPDATE \
\
}

#endif /* RenderMinorExtensionTags_H */
