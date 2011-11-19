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

#include "NXrender.h"

#include "RenderExtension.h"
#include "RenderGenericRequest.h"

#include "ClientCache.h"

#include "EncodeBuffer.h"
#include "DecodeBuffer.h"

#include "WriteBuffer.h"

//
// Set the verbosity level.
//

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

//
// Here are the methods to handle the messages' content.
//

int RenderGenericRequestStore::encodeMessage(EncodeBuffer &encodeBuffer, const unsigned char *buffer,
                                                 const unsigned int size, int bigEndian,
                                                     ChannelCache *channelCache) const
{
  ClientCache *clientCache = (ClientCache *) channelCache;

  #ifdef DEBUG
  *logofs << name() << ": Encoding full message.\n"
          << logofs_flush;

  unsigned char type = *(buffer + 1);

  #endif

  encodeBuffer.encodeCachedValue(size >> 2, 16,
                     clientCache -> renderLengthCache, 5);

  #ifdef DEBUG
  *logofs << name() << ": Encoding full unhandled message. "
          << "Type is " << (unsigned int) type << " size is "
          << size << ".\n" << logofs_flush;
  #endif

  encodeIntData(encodeBuffer, buffer, 4, size,
                    bigEndian, clientCache);

  #ifdef DEBUG
  *logofs << name() << ": Encoded full message.\n"
          << logofs_flush;
  #endif

  return 1;
}

int RenderGenericRequestStore::decodeMessage(DecodeBuffer &decodeBuffer, unsigned char *&buffer,
                                                 unsigned int &size, unsigned char type, int bigEndian,
                                                     WriteBuffer *writeBuffer, ChannelCache *channelCache) const
{
  ClientCache *clientCache = (ClientCache *) channelCache;

  #ifdef DEBUG
  *logofs << name() << ": Decoding full message.\n"
          << logofs_flush;
  #endif

  decodeBuffer.decodeCachedValue(size, 16,
                     clientCache -> renderLengthCache, 5);

  size <<= 2;

  buffer = writeBuffer -> addMessage(size);

  *(buffer + 1) = type;

  #ifdef DEBUG
  *logofs << name() << ": Decoding full unhandled message. "
          << "Type is " << (unsigned int) type << " size is "
          << size << ".\n" << logofs_flush;
  #endif

  decodeIntData(decodeBuffer, buffer, 4, size,
                    bigEndian, clientCache);

  #ifdef DEBUG
  *logofs << name() << ": Decoded full message.\n"
          << logofs_flush;
  #endif

  return 1;
}

void RenderGenericRequestStore::encodeData(EncodeBuffer &encodeBuffer, const unsigned char *buffer,
                                               unsigned int size, int bigEndian,
                                                   ChannelCache *channelCache) const
{
}

void RenderGenericRequestStore::decodeData(DecodeBuffer &decodeBuffer, unsigned char *buffer,
                                               unsigned int size, int bigEndian,
                                                   ChannelCache *channelCache) const
{
}

int RenderGenericRequestStore::parseIdentity(Message *message, const unsigned char *buffer,
                                                 unsigned int size, int bigEndian) const
{
  #ifdef DEBUG
  *logofs << name() << ": Parsing identity for message at "
          << this << ".\n" << logofs_flush;
  #endif

  RenderExtensionMessage *renderExtension = (RenderExtensionMessage *) message;

  unsigned char type = *(buffer + 1);

  renderExtension -> data.any.type = type;

  #ifdef DEBUG
  *logofs << name() << ": Parsing unhandled identity. "
          << "Type is " << (unsigned int) renderExtension -> data.any.type
          << " size is " << renderExtension -> size_ << ".\n"
          << logofs_flush;
  #endif

  parseIntData(message, buffer, 4, size, bigEndian);

  #ifdef DEBUG
  *logofs << name() << ": Parsed identity for message at "
          << this << ".\n" << logofs_flush;
  #endif

  return 1;
}

int RenderGenericRequestStore::unparseIdentity(const Message *message, unsigned char *buffer,
                                                   unsigned int size, int bigEndian) const
{
  #ifdef DEBUG
  *logofs << name() << ": Unparsing identity for message at "
          << this << ".\n" << logofs_flush;
  #endif

  RenderExtensionMessage *renderExtension = (RenderExtensionMessage *) message;

  unsigned char type = renderExtension -> data.any.type;

  *(buffer + 1) = type;

  #ifdef DEBUG
  *logofs << name() << ": Unparsing unhandled identity. "
          << "Type is " << (unsigned int) renderExtension -> data.any.type
          << " size is " << renderExtension -> size_ << ".\n"
          << logofs_flush;
  #endif

  unparseIntData(message, buffer, 4, size, bigEndian);

  #ifdef DEBUG
  *logofs << name() << ": Unparsed identity for message at "
          << this << ".\n" << logofs_flush;
  #endif

  return 1;
}

void RenderGenericRequestStore::identityChecksum(const Message *message, const unsigned char *buffer,
                                                     unsigned int size, md5_state_t *md5_state,
                                                         int bigEndian) const
{
  //
  // Include the minor opcode in the checksum.
  // Because the data offset can be beyond the
  // real end of the message, we need to include
  // the size or we will match any message whose
  // size is less or equal to the data offset.
  //

  md5_append(md5_state, buffer + 1, 3);
}

void RenderGenericRequestStore::updateIdentity(EncodeBuffer &encodeBuffer, const Message *message,
                                                   const Message *cachedMessage,
                                                       ChannelCache *channelCache) const
{
  //
  // Encode the variant part.
  //

  #ifdef DEBUG
  *logofs << name() << ": Updating identity for message at "
          << this << ".\n" << logofs_flush;
  #endif

  RenderExtensionMessage *renderExtension = (RenderExtensionMessage *) message;

  #ifdef DEBUG
  *logofs << name() << ": Encoding unhandled update. "
          << "Type is " << (unsigned int) renderExtension -> data.any.type
          << " size is " << renderExtension -> size_ << ".\n"
          << logofs_flush;
  #endif

  updateIntData(encodeBuffer, message, cachedMessage, 4,
                    renderExtension -> size_, channelCache);

  #ifdef DEBUG
  *logofs << name() << ": Updated identity for message at "
          << this << ".\n" << logofs_flush;
  #endif
}

void RenderGenericRequestStore::updateIdentity(DecodeBuffer &decodeBuffer, const Message *message,
                                                   ChannelCache *channelCache) const
{
  #ifdef DEBUG
  *logofs << name() << ": Updating identity for message at "
          << this << ".\n" << logofs_flush;
  #endif

  RenderExtensionMessage *renderExtension = (RenderExtensionMessage *) message;

  #ifdef DEBUG
  *logofs << name() << ": Decoding unhandled update. "
          << "Type is " << (unsigned int) renderExtension -> data.any.type
          << " size is " << renderExtension -> size_ << ".\n"
          << logofs_flush;
  #endif

  updateIntData(decodeBuffer, message, 4,
                    renderExtension -> size_, channelCache);

  #ifdef DEBUG
  *logofs << name() << ": Updated identity for message at "
          << this << ".\n" << logofs_flush;
  #endif
}
