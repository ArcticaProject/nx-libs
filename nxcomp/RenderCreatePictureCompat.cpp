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

#include "RenderCreatePictureCompat.h"

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
  ClientCache *clientCache = (ClientCache *) channelCache;

  encodeBuffer.encodeCachedValue((size - MESSAGE_OFFSET) >> 2, 16,
                     clientCache -> renderLengthCache, 5);

  #ifdef TEST
  *logofs << name() << ": Encoded size with value "
          << size << ".\n" << logofs_flush;
  #endif
}
MESSAGE_END_ENCODE_SIZE

MESSAGE_BEGIN_DECODE_SIZE
{
  ClientCache *clientCache = (ClientCache *) channelCache;

  decodeBuffer.decodeCachedValue(size, 16,
                     clientCache -> renderLengthCache, 5);

  size = MESSAGE_OFFSET + (size << 2);

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

  encodeBuffer.encodeXidValue(GetULONG(buffer + 8, bigEndian),
                     clientCache -> drawableCache);

  encodeBuffer.encodeCachedValue(GetULONG(buffer + 12, bigEndian), 32,
                     clientCache -> renderFormatCache);

  encodeBuffer.encodeCachedValue(GetULONG(buffer + 16, bigEndian), 32,
                     clientCache -> renderValueMaskCache);

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

  decodeBuffer.decodeXidValue(value,
                     clientCache -> drawableCache);

  PutULONG(value, buffer + 8, bigEndian);

  decodeBuffer.decodeCachedValue(value, 32,
                     clientCache -> renderFormatCache);

  PutULONG(value, buffer + 12, bigEndian);

  decodeBuffer.decodeCachedValue(value, 32,
                     clientCache -> renderValueMaskCache);

  PutULONG(value, buffer + 16, bigEndian);

  #ifdef TEST
  *logofs << name() << ": Decoded message. Type is "
          << (unsigned int) type << " size is " << size
          << ".\n" << logofs_flush;
  #endif
}
MESSAGE_END_DECODE_MESSAGE

MESSAGE_BEGIN_ENCODE_DATA
{
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

  renderExtension -> data.create_picture.type = *(buffer + 1);

  renderExtension -> data.create_picture.src_id = GetULONG(buffer + 4,  bigEndian);
  renderExtension -> data.create_picture.dst_id = GetULONG(buffer + 8,  bigEndian);

  renderExtension -> data.create_picture.format = GetULONG(buffer + 12,  bigEndian);
  renderExtension -> data.create_picture.mask   = GetULONG(buffer + 16,  bigEndian);

  #ifdef TEST
  *logofs << name() << ": Parsed identity. Type is "
          << (unsigned int) renderExtension -> data.create_picture.type
          << " size is " << renderExtension -> size_ << ".\n"
          << logofs_flush;
  #endif
}
MESSAGE_END_PARSE_IDENTITY

MESSAGE_BEGIN_UNPARSE_IDENTITY
{
  RenderExtensionMessage *renderExtension = (RenderExtensionMessage *) message;

  *(buffer + 1) = renderExtension -> data.create_picture.type;

  PutULONG(renderExtension -> data.create_picture.src_id, buffer + 4,  bigEndian);
  PutULONG(renderExtension -> data.create_picture.dst_id, buffer + 8, bigEndian);

  PutULONG(renderExtension -> data.create_picture.format, buffer + 12, bigEndian);
  PutULONG(renderExtension -> data.create_picture.mask,   buffer + 16, bigEndian);

  #ifdef TEST
  *logofs << name() << ": Unparsed identity. Type is "
          << (unsigned int) renderExtension -> data.create_picture.type
          << " size is " << renderExtension -> size_ << ".\n"
          << logofs_flush;
  #endif
}
MESSAGE_END_UNPARSE_IDENTITY

MESSAGE_BEGIN_IDENTITY_CHECKSUM
{
  md5_append(md5_state, buffer + 1,  3);
  md5_append(md5_state, buffer + 12, 8);
}
MESSAGE_END_IDENTITY_CHECKSUM

MESSAGE_BEGIN_ENCODE_UPDATE
{
  RenderExtensionMessage *renderExtension       = (RenderExtensionMessage *) message;
  RenderExtensionMessage *cachedRenderExtension = (RenderExtensionMessage *) cachedMessage;

  ClientCache *clientCache = (ClientCache *) channelCache;

  #ifdef DEBUG
  *logofs << name() << ": Encoding new id value "
          << renderExtension -> data.create_picture.src_id -
             clientCache -> renderLastId << ".\n";
  #endif

  encodeBuffer.encodeDiffCachedValue(renderExtension -> data.create_picture.src_id,
                     clientCache -> renderLastId, 29,
                         clientCache -> renderIdCache);

  cachedRenderExtension -> data.create_picture.src_id =
              renderExtension -> data.create_picture.src_id;

  encodeBuffer.encodeXidValue(renderExtension -> data.create_picture.dst_id,
                     clientCache -> drawableCache);

  cachedRenderExtension -> data.create_picture.dst_id =
              renderExtension -> data.create_picture.dst_id;

  #ifdef TEST
  *logofs << name() << ": Encoded update. Type is "
          << (unsigned int) renderExtension -> data.create_picture.type
          << " size is " << renderExtension -> size_ << ".\n"
          << logofs_flush;
  #endif
}
MESSAGE_END_ENCODE_UPDATE

MESSAGE_BEGIN_DECODE_UPDATE
{
  RenderExtensionMessage *renderExtension = (RenderExtensionMessage *) message;

  ClientCache *clientCache = (ClientCache *) channelCache;

  decodeBuffer.decodeDiffCachedValue(renderExtension -> data.create_picture.src_id,
                     clientCache -> renderLastId, 29,
                         clientCache -> renderIdCache);

  decodeBuffer.decodeXidValue(renderExtension -> data.create_picture.dst_id,
                     clientCache -> drawableCache);

  #ifdef TEST
  *logofs << name() << ": Decoded update. Type is "
          << (unsigned int) renderExtension -> data.create_picture.type
          << " size is " << renderExtension -> size_ << ".\n"
          << logofs_flush;
  #endif
}
MESSAGE_END_DECODE_UPDATE
