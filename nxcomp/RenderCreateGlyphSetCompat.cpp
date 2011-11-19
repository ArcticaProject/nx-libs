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

//
// Include the template for
// this message class.
//

#include "RenderCreateGlyphSetCompat.h"

//
// Set the verbosity level.
//

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

#include MESSAGE_TAGS

//
// Message handling methods.
//

MESSAGE_BEGIN_ENCODE_SIZE
{
  //
  // Strictly speaking this request doesn't have
  // a data part. We encode the fields past the
  // offset as they were data. An improvement
  // would be to encode the format field using
  // the cache.
  //

  #ifdef TEST
  *logofs << name() << ": Encoded size with value "
          << size << ".\n" << logofs_flush;
  #endif
}
MESSAGE_END_ENCODE_SIZE

MESSAGE_BEGIN_DECODE_SIZE
{
  size = MESSAGE_OFFSET + 4;

  buffer = writeBuffer -> addMessage(size);

  #ifdef TEST
  *logofs << name() << ": Decoded size with value "
          << size << ".\n" << logofs_flush;
  #endif
}
MESSAGE_END_DECODE_SIZE

MESSAGE_BEGIN_ENCODE_MESSAGE
{
  ClientCache *clientCache = (ClientCache *) channelCache;

  encodeBuffer.encodeDiffCachedValue(GetULONG(buffer + 4, bigEndian),
                     clientCache -> renderLastId, 29,
                         clientCache -> renderIdCache);

  #ifdef TEST
  *logofs << name() << ": Encoded message. Type is "
          << (unsigned int) *(buffer + 1) << " size is "
          << size << ".\n" << logofs_flush;
  #endif
}
MESSAGE_END_ENCODE_MESSAGE

MESSAGE_BEGIN_DECODE_MESSAGE
{
  ClientCache *clientCache = (ClientCache *) channelCache;

  unsigned int value;

  *(buffer + 1) = type;

  decodeBuffer.decodeDiffCachedValue(value,
                     clientCache -> renderLastId, 29,
                         clientCache -> renderIdCache);

  PutULONG(value, buffer + 4, bigEndian);

  #ifdef TEST
  *logofs << name() << ": Decoded message. Type is "
          << (unsigned int) type << " size is " << size
          << ".\n" << logofs_flush;
  #endif
}
MESSAGE_END_DECODE_MESSAGE

MESSAGE_BEGIN_ENCODE_DATA
{
  #ifdef DEBUG

  *logofs << name() << ": Glyphset is " << GetULONG(buffer + 4, bigEndian)
          << ".\n" << logofs_flush;

  if (size > MESSAGE_OFFSET)
  {
    *logofs << name() << ": Format is " << GetULONG(buffer + 8, bigEndian)
            << ".\n" << logofs_flush;
  }

  if (size > MESSAGE_OFFSET + 4)
  {
    *logofs << name() << ": WARNING! Unexpected size " << size
            << ".\n" << logofs_flush;
  }

  #endif

  encodeLongData(encodeBuffer, buffer, MESSAGE_OFFSET,
                     size, bigEndian, channelCache);

  #ifdef TEST
  *logofs << name() << ": Encoded " << size - MESSAGE_OFFSET
          << " bytes of data.\n" << logofs_flush;
  #endif
}
MESSAGE_END_ENCODE_DATA

MESSAGE_BEGIN_DECODE_DATA
{
  decodeLongData(decodeBuffer, buffer, MESSAGE_OFFSET,
                     size, bigEndian, channelCache);

  #ifdef TEST
  *logofs << name() << ": Decoded " << size - MESSAGE_OFFSET
          << " bytes of data.\n" << logofs_flush;
  #endif
}
MESSAGE_END_DECODE_DATA

MESSAGE_BEGIN_PARSE_IDENTITY
{
  RenderExtensionMessage *renderExtension = (RenderExtensionMessage *) message;

  renderExtension -> data.create_set.type = *(buffer + 1);

  renderExtension -> data.create_set.set_id = GetULONG(buffer + 4, bigEndian);

  #ifdef TEST
  *logofs << name() << ": Parsed identity. Type is "
          << (unsigned int) renderExtension -> data.create_set.type
          << " size is " << renderExtension -> size_ << ".\n"
          << logofs_flush;
  #endif
}
MESSAGE_END_PARSE_IDENTITY

MESSAGE_BEGIN_UNPARSE_IDENTITY
{
  RenderExtensionMessage *renderExtension = (RenderExtensionMessage *) message;

  *(buffer + 1) = renderExtension -> data.create_set.type;

  PutULONG(renderExtension -> data.create_set.set_id, buffer + 4,  bigEndian);

  #ifdef TEST
  *logofs << name() << ": Unparsed identity. Type is "
          << (unsigned int) renderExtension -> data.create_set.type
          << " size is " << renderExtension -> size_ << ".\n"
          << logofs_flush;
  #endif
}
MESSAGE_END_UNPARSE_IDENTITY

MESSAGE_BEGIN_IDENTITY_CHECKSUM
{
  md5_append(md5_state, buffer + 1, 3);
}
MESSAGE_END_IDENTITY_CHECKSUM

MESSAGE_BEGIN_ENCODE_UPDATE
{
  RenderExtensionMessage *renderExtension       = (RenderExtensionMessage *) message;
  RenderExtensionMessage *cachedRenderExtension = (RenderExtensionMessage *) cachedMessage;

  ClientCache *clientCache = (ClientCache *) channelCache;

  encodeBuffer.encodeDiffCachedValue(renderExtension -> data.create_set.set_id,
                     clientCache -> renderLastId, 29,
                         clientCache -> renderIdCache);

  cachedRenderExtension -> data.create_set.set_id =
              renderExtension -> data.create_set.set_id;

  #ifdef TEST
  *logofs << name() << ": Encoded update. Type is "
          << (unsigned int) renderExtension -> data.create_set.type
          << " size is " << renderExtension -> size_ << ".\n"
          << logofs_flush;
  #endif
}
MESSAGE_END_ENCODE_UPDATE

MESSAGE_BEGIN_DECODE_UPDATE
{
  RenderExtensionMessage *renderExtension = (RenderExtensionMessage *) message;

  ClientCache *clientCache = (ClientCache *) channelCache;

  decodeBuffer.decodeDiffCachedValue(renderExtension -> data.create_set.set_id,
                     clientCache -> renderLastId, 29,
                         clientCache -> renderIdCache);

  #ifdef TEST
  *logofs << name() << ": Decoded update. Type is "
          << (unsigned int) renderExtension -> data.create_set.type
          << " size is " << renderExtension -> size_ << ".\n"
          << logofs_flush;
  #endif
}
MESSAGE_END_DECODE_UPDATE
