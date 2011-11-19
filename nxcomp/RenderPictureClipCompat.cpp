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

#include "RenderPictureClipCompat.h"

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

  encodeBuffer.encodeXidValue(GetULONG(buffer + 4, bigEndian),
                     clientCache -> renderSrcPictureCache);

  encodeBuffer.encodeDiffCachedValue(GetUINT(buffer + 8, bigEndian),
                     clientCache -> renderLastX, 16,
                         clientCache -> renderXCache, 11);

  encodeBuffer.encodeDiffCachedValue(GetUINT(buffer + 10, bigEndian),
                     clientCache -> renderLastY, 16,
                         clientCache -> renderYCache, 11);

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

  decodeBuffer.decodeXidValue(value,
                     clientCache -> renderSrcPictureCache);

  PutULONG(value, buffer + 4, bigEndian);

  decodeBuffer.decodeDiffCachedValue(value,
                     clientCache -> renderLastX, 16,
                         clientCache -> renderXCache, 11);

  PutUINT(clientCache -> renderLastX, buffer + 8, bigEndian);

  decodeBuffer.decodeDiffCachedValue(value,
                     clientCache -> renderLastY, 16,
                         clientCache -> renderYCache, 11);

  PutUINT(clientCache -> renderLastY, buffer + 10, bigEndian);

  #ifdef TEST
  *logofs << name() << ": Decoded message. Type is "
          << (unsigned int) type << " size is " << size
          << ".\n" << logofs_flush;
  #endif
}
MESSAGE_END_DECODE_MESSAGE

MESSAGE_BEGIN_ENCODE_DATA
{
  encodeIntData(encodeBuffer, buffer, MESSAGE_OFFSET,
                    size, bigEndian, channelCache);

  #ifdef TEST
  *logofs << name() << ": Encoded " << size - MESSAGE_OFFSET
          << " bytes of data.\n" << logofs_flush;
  #endif
}
MESSAGE_END_ENCODE_DATA

MESSAGE_BEGIN_DECODE_DATA
{
  decodeIntData(decodeBuffer, buffer, MESSAGE_OFFSET,
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

  renderExtension -> data.picture_clip.type = *(buffer + 1);

  renderExtension -> data.picture_clip.src_id = GetULONG(buffer + 4,  bigEndian);

  renderExtension -> data.picture_clip.src_x = GetUINT(buffer + 8,  bigEndian);
  renderExtension -> data.picture_clip.src_y = GetUINT(buffer + 10, bigEndian);

  #ifdef TEST
  *logofs << name() << ": Parsed identity. Type is "
          << (unsigned int) renderExtension -> data.picture_clip.type
          << " size is " << renderExtension -> size_ << ".\n"
          << logofs_flush;
  #endif
}
MESSAGE_END_PARSE_IDENTITY

MESSAGE_BEGIN_UNPARSE_IDENTITY
{
  RenderExtensionMessage *renderExtension = (RenderExtensionMessage *) message;

  *(buffer + 1) = renderExtension -> data.picture_clip.type;

  PutULONG(renderExtension -> data.picture_clip.src_id, buffer + 4,  bigEndian);

  PutUINT(renderExtension -> data.picture_clip.src_x, buffer + 8,  bigEndian);
  PutUINT(renderExtension -> data.picture_clip.src_y, buffer + 10, bigEndian);

  #ifdef TEST
  *logofs << name() << ": Unparsed identity. Type is "
          << (unsigned int) renderExtension -> data.picture_clip.type
          << " size is " << renderExtension -> size_ << ".\n"
          << logofs_flush;
  #endif
}
MESSAGE_END_UNPARSE_IDENTITY

MESSAGE_BEGIN_IDENTITY_CHECKSUM
{
  md5_append(md5_state, buffer + 1,  3);
  md5_append(md5_state, buffer + 8,  4);
}
MESSAGE_END_IDENTITY_CHECKSUM

MESSAGE_BEGIN_ENCODE_UPDATE
{
  RenderExtensionMessage *renderExtension       = (RenderExtensionMessage *) message;
  RenderExtensionMessage *cachedRenderExtension = (RenderExtensionMessage *) cachedMessage;

  ClientCache *clientCache = (ClientCache *) channelCache;

  encodeBuffer.encodeXidValue(renderExtension -> data.picture_clip.src_id,
                     clientCache -> renderSrcPictureCache);

  cachedRenderExtension -> data.picture_clip.src_id =
              renderExtension -> data.picture_clip.src_id;

  #ifdef TEST
  *logofs << name() << ": Encoded update. Type is "
          << (unsigned int) renderExtension -> data.picture_clip.type
          << " size is " << renderExtension -> size_ << ".\n"
          << logofs_flush;
  #endif
}
MESSAGE_END_ENCODE_UPDATE

MESSAGE_BEGIN_DECODE_UPDATE
{
  RenderExtensionMessage *renderExtension = (RenderExtensionMessage *) message;

  ClientCache *clientCache = (ClientCache *) channelCache;

  decodeBuffer.decodeXidValue(renderExtension -> data.picture_clip.src_id,
                     clientCache -> renderSrcPictureCache);

  #ifdef TEST
  *logofs << name() << ": Decoded update. Type is "
          << (unsigned int) renderExtension -> data.picture_clip.type
          << " size is " << renderExtension -> size_ << ".\n"
          << logofs_flush;
  #endif
}
MESSAGE_END_DECODE_UPDATE
