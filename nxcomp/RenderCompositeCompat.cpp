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

#include "RenderCompositeCompat.h"

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
  // a data part. We just encode the field from
  // offset 24 to 36 as they were data using an
  // int cache.
  //

  #ifdef TEST
  *logofs << name() << ": Encoded size with value "
          << size << ".\n" << logofs_flush;
  #endif
}
MESSAGE_END_ENCODE_SIZE

MESSAGE_BEGIN_DECODE_SIZE
{
  size = MESSAGE_OFFSET + 12;

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

  encodeBuffer.encodeCachedValue(*(buffer + 4), 8,
                     clientCache -> renderOpCache);

  encodeBuffer.encodeXidValue(GetULONG(buffer + 8, bigEndian),
                     clientCache -> renderSrcPictureCache);

  encodeBuffer.encodeXidValue(GetULONG(buffer + 12, bigEndian),
                     clientCache -> renderSrcPictureCache);

  encodeBuffer.encodeXidValue(GetULONG(buffer + 16, bigEndian),
                     clientCache -> renderSrcPictureCache);

  encodeBuffer.encodeDiffCachedValue(GetUINT(buffer + 20, bigEndian),
                     clientCache -> renderLastX, 16,
                         clientCache -> renderXCache, 11);

  encodeBuffer.encodeDiffCachedValue(GetUINT(buffer + 22, bigEndian),
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

  decodeBuffer.decodeCachedValue(*(buffer + 4), 8,
                     clientCache -> renderOpCache);

  decodeBuffer.decodeXidValue(value, clientCache -> renderSrcPictureCache);

  PutULONG(value, buffer + 8, bigEndian);

  decodeBuffer.decodeXidValue(value, clientCache -> renderSrcPictureCache);

  PutULONG(value, buffer + 12, bigEndian);

  decodeBuffer.decodeXidValue(value, clientCache -> renderSrcPictureCache);

  PutULONG(value, buffer + 16, bigEndian);

  decodeBuffer.decodeDiffCachedValue(value,
                     clientCache -> renderLastX, 16,
                         clientCache -> renderXCache, 11);

  PutUINT(clientCache -> renderLastX, buffer + 20, bigEndian);

  decodeBuffer.decodeDiffCachedValue(value,
                     clientCache -> renderLastY, 16,
                         clientCache -> renderYCache, 11);

  PutUINT(clientCache -> renderLastY, buffer + 22, bigEndian);

  #ifdef TEST
  *logofs << name() << ": Decoded message. Type is "
          << (unsigned int) type << " size is " << size
          << ".\n" << logofs_flush;
  #endif
}
MESSAGE_END_DECODE_MESSAGE

MESSAGE_BEGIN_ENCODE_DATA
{
  ClientCache *clientCache = (ClientCache *) channelCache;

  for (unsigned int i = MESSAGE_OFFSET, c = 0; i < size; i += 4)
  {
    #ifdef DEBUG
    *logofs << name() << ": Encoding long value "
            << GetULONG(buffer + i, bigEndian) << " with i = "
            << i << " c = "  << c << ".\n" << logofs_flush;
    #endif

    encodeBuffer.encodeCachedValue(GetULONG(buffer + i, bigEndian), 32,
                       *clientCache -> renderCompositeDataCache[c]);

    if (++c == 3) c = 0;
  }

  #ifdef TEST
  *logofs << name() << ": Encoded " << size - MESSAGE_OFFSET
          << " bytes of data.\n" << logofs_flush;
  #endif
}
MESSAGE_END_ENCODE_DATA

MESSAGE_BEGIN_DECODE_DATA
{
  ClientCache *clientCache = (ClientCache *) channelCache;

  unsigned int value;

  for (unsigned int i = MESSAGE_OFFSET, c = 0; i < size; i += 4)
  {
    decodeBuffer.decodeCachedValue(value, 32,
                       *clientCache -> renderCompositeDataCache[c]);

    #ifdef DEBUG
    *logofs << name() << ": Decoded long value " << value
            << " with i = " << i << " c = " << c << ".\n"
            << logofs_flush;
    #endif

    PutULONG(value, buffer + i, bigEndian);

    if (++c == 3) c = 0;
  }

  #ifdef TEST
  *logofs << name() << ": Decoded " << size - MESSAGE_OFFSET
          << " bytes of data.\n" << logofs_flush;
  #endif
}
MESSAGE_END_DECODE_DATA

MESSAGE_BEGIN_PARSE_IDENTITY
{
  RenderExtensionMessage *renderExtension = (RenderExtensionMessage *) message;

  renderExtension -> data.composite.type = *(buffer + 1);
  renderExtension -> data.composite.op   = *(buffer + 4);

  renderExtension -> data.composite.src_id = GetULONG(buffer + 8,  bigEndian);
  renderExtension -> data.composite.msk_id = GetULONG(buffer + 12, bigEndian);
  renderExtension -> data.composite.dst_id = GetULONG(buffer + 16, bigEndian);

  renderExtension -> data.composite.src_x = GetUINT(buffer + 20, bigEndian);
  renderExtension -> data.composite.src_y = GetUINT(buffer + 22, bigEndian);

  #ifdef TEST
  *logofs << name() << ": Parsed identity. Type is "
          << (unsigned int) renderExtension -> data.composite.type
          << " size is " << renderExtension -> size_ << ".\n"
          << logofs_flush;
  #endif
}
MESSAGE_END_PARSE_IDENTITY

MESSAGE_BEGIN_UNPARSE_IDENTITY
{
  RenderExtensionMessage *renderExtension = (RenderExtensionMessage *) message;

  *(buffer + 1) = renderExtension -> data.composite.type;
  *(buffer + 4) = renderExtension -> data.composite.op;

  PutULONG(renderExtension -> data.composite.src_id, buffer + 8,  bigEndian);
  PutULONG(renderExtension -> data.composite.msk_id, buffer + 12, bigEndian);
  PutULONG(renderExtension -> data.composite.dst_id, buffer + 16, bigEndian);

  PutUINT(renderExtension -> data.composite.src_x, buffer + 20, bigEndian);
  PutUINT(renderExtension -> data.composite.src_y, buffer + 22, bigEndian);

  #ifdef TEST
  *logofs << name() << ": Unparsed identity. Type is "
          << (unsigned int) renderExtension -> data.composite.type
          << " size is " << renderExtension -> size_ << ".\n"
          << logofs_flush;
  #endif
}
MESSAGE_END_UNPARSE_IDENTITY

MESSAGE_BEGIN_IDENTITY_CHECKSUM
{
  //
  // Include minor opcode, size and
  // operator in the identity, plus
  // the x and y of the source.
  //

  md5_append(md5_state, buffer + 1,  4);
  md5_append(md5_state, buffer + 20, 4);
}
MESSAGE_END_IDENTITY_CHECKSUM

MESSAGE_BEGIN_ENCODE_UPDATE
{
  RenderExtensionMessage *renderExtension       = (RenderExtensionMessage *) message;
  RenderExtensionMessage *cachedRenderExtension = (RenderExtensionMessage *) cachedMessage;

  ClientCache *clientCache = (ClientCache *) channelCache;

  #ifdef DEBUG
  *logofs << name() << ": Source " << renderExtension ->
             data.composite.src_id << " mask " << renderExtension ->
             data.composite.msk_id << " destination " << renderExtension ->
             data.composite.msk_id << ".\n" << logofs_flush;
  #endif

  encodeBuffer.encodeXidValue(renderExtension -> data.composite.src_id,
                     clientCache -> renderSrcPictureCache);

  cachedRenderExtension -> data.composite.src_id =
              renderExtension -> data.composite.src_id;

  encodeBuffer.encodeXidValue(renderExtension -> data.composite.msk_id,
                     clientCache -> renderSrcPictureCache);

  cachedRenderExtension -> data.composite.msk_id =
              renderExtension -> data.composite.msk_id;

  encodeBuffer.encodeXidValue(renderExtension -> data.composite.dst_id,
                     clientCache -> renderSrcPictureCache);

  cachedRenderExtension -> data.composite.dst_id =
              renderExtension -> data.composite.dst_id;

  #ifdef TEST
  *logofs << name() << ": Encoded update. Type is "
          << (unsigned int) renderExtension -> data.composite.type
          << " size is " << renderExtension -> size_ << ".\n"
          << logofs_flush;
  #endif
}
MESSAGE_END_ENCODE_UPDATE

MESSAGE_BEGIN_DECODE_UPDATE
{
  RenderExtensionMessage *renderExtension = (RenderExtensionMessage *) message;

  ClientCache *clientCache = (ClientCache *) channelCache;

  decodeBuffer.decodeXidValue(renderExtension -> data.composite.src_id,
                     clientCache -> renderSrcPictureCache);

  decodeBuffer.decodeXidValue(renderExtension -> data.composite.msk_id,
                     clientCache -> renderSrcPictureCache);

  decodeBuffer.decodeXidValue(renderExtension -> data.composite.dst_id,
                     clientCache -> renderSrcPictureCache);

  #ifdef TEST
  *logofs << name() << ": Decoded update. Type is "
          << (unsigned int) renderExtension -> data.composite.type
          << " size is " << renderExtension -> size_ << ".\n"
          << logofs_flush;
  #endif
}
MESSAGE_END_DECODE_UPDATE
