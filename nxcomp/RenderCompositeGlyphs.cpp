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

#include "RenderCompositeGlyphs.h"

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

  #ifdef DEBUG
  *logofs << name() << ": Encoding value "
          << ((size - MESSAGE_OFFSET) >> 2) << ".\n"
          << logofs_flush;
  #endif

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

  #ifdef DEBUG
  *logofs << name() << ": Decoded value " << size
          << ".\n" << logofs_flush;
  #endif

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

  encodeBuffer.encodeCachedValue(*(buffer + 4), 8,
                     clientCache -> renderOpCache);

  encodeBuffer.encodeXidValue(GetULONG(buffer + 8, bigEndian),
                     clientCache -> renderSrcPictureCache);

  encodeBuffer.encodeXidValue(GetULONG(buffer + 12, bigEndian),
                     clientCache -> renderDstPictureCache);

  encodeBuffer.encodeCachedValue(GetULONG(buffer + 16, bigEndian), 32,
                     clientCache -> renderFormatCache);

  encodeBuffer.encodeCachedValue(GetULONG(buffer + 20, bigEndian), 29,
                     clientCache -> renderGlyphSetCache);

  unsigned int src_x = GetUINT(buffer + 24, bigEndian);
  unsigned int src_y = GetUINT(buffer + 26, bigEndian);

  if (control -> isProtoStep8() == 1)
  {
    encodeBuffer.encodeDiffCachedValue(src_x,
                       clientCache -> renderGlyphX, 16,
                           clientCache -> renderGlyphXCache, 11);

    encodeBuffer.encodeDiffCachedValue(src_y,
                       clientCache -> renderGlyphY, 16,
                           clientCache -> renderGlyphYCache, 11);
  }
  else
  {
    encodeBuffer.encodeDiffCachedValue(src_x,
                       clientCache -> renderLastX, 16,
                           clientCache -> renderXCache, 11);

    encodeBuffer.encodeDiffCachedValue(src_y,
                       clientCache -> renderLastY, 16,
                           clientCache -> renderYCache, 11);
  }

  #ifdef TEST
  *logofs << name() << ": Encoded source X "
          << GetUINT(buffer + 24, bigEndian) << " source Y "
          << GetUINT(buffer + 26, bigEndian) << ".\n"
          << logofs_flush;
  #endif

  //
  // Bytes from 28 to 36 contain in the order:
  //
  // 1 byte for the length of the first string.
  // 3 bytes of padding.
  // 2 bytes for the X offset.
  // 2 bytes for the Y offset.
  //
  // Encode these bytes differentially to match
  // all the strings that have equal glyphs.
  //
  // Only manage the first string of glyphs. The
  // others strings should match, if they contain
  // the same glyphs, since the offset are rela-
  // tive to the first offset coordinates.
  //

  if (control -> isProtoStep8() == 1 &&
          size >= MESSAGE_OFFSET_IF_PROTO_STEP_8)
  {
    unsigned int numGlyphs = *(buffer + 28);

    encodeBuffer.encodeCachedValue(numGlyphs, 8,
                       clientCache -> renderNumGlyphsCache);

    unsigned int offset_x = GetUINT(buffer + 32, bigEndian);
    unsigned int offset_y = GetUINT(buffer + 34, bigEndian);

    if (offset_x == src_x && offset_y == src_y)
    {
      encodeBuffer.encodeBoolValue(0);

      #ifdef TEST
      *logofs << name() << ": Matched offset X "
              << GetUINT(buffer + 32, bigEndian) << " offset Y "
              << GetUINT(buffer + 34, bigEndian) << ".\n"
              << logofs_flush;
      #endif
    }
    else
    {
      encodeBuffer.encodeBoolValue(1);

      encodeBuffer.encodeDiffCachedValue(offset_x,
                         clientCache -> renderGlyphX, 16,
                             clientCache -> renderGlyphXCache, 11);

      encodeBuffer.encodeDiffCachedValue(offset_y,
                         clientCache -> renderGlyphY, 16,
                             clientCache -> renderGlyphYCache, 11);

      #ifdef TEST
      *logofs << name() << ": Missed offset X "
              << GetUINT(buffer + 32, bigEndian) << " offset Y "
              << GetUINT(buffer + 34, bigEndian) << ".\n"
              << logofs_flush;
      #endif
    }
  }

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

  decodeBuffer.decodeXidValue(value,
                     clientCache -> renderSrcPictureCache);

  PutULONG(value, buffer + 8, bigEndian);

  decodeBuffer.decodeXidValue(value,
                     clientCache -> renderDstPictureCache);

  PutULONG(value, buffer + 12, bigEndian);

  decodeBuffer.decodeCachedValue(value, 32,
                     clientCache -> renderFormatCache);

  PutULONG(value, buffer + 16, bigEndian);

  decodeBuffer.decodeCachedValue(value, 29,
                     clientCache -> renderGlyphSetCache);

  PutULONG(value, buffer + 20, bigEndian);

  unsigned int src_x;
  unsigned int src_y;

  if (control -> isProtoStep8() == 1)
  {
    decodeBuffer.decodeDiffCachedValue(src_x,
                       clientCache -> renderGlyphX, 16,
                           clientCache -> renderGlyphXCache, 11);

    decodeBuffer.decodeDiffCachedValue(src_y,
                       clientCache -> renderGlyphY, 16,
                           clientCache -> renderGlyphYCache, 11);
  }
  else
  {
    decodeBuffer.decodeDiffCachedValue(src_x,
                       clientCache -> renderLastX, 16,
                           clientCache -> renderXCache, 11);

    decodeBuffer.decodeDiffCachedValue(src_y,
                       clientCache -> renderLastY, 16,
                           clientCache -> renderYCache, 11);
  }

  PutUINT(src_x, buffer + 24, bigEndian);
  PutUINT(src_y, buffer + 26, bigEndian);

  if (control -> isProtoStep8() == 1 &&
          size >= MESSAGE_OFFSET_IF_PROTO_STEP_8)
  {
    decodeBuffer.decodeCachedValue(value, 8,
                       clientCache -> renderNumGlyphsCache);

    *(buffer + 28) = value;

    decodeBuffer.decodeBoolValue(value);

    if (value == 0)
    {
      PutUINT(src_x, buffer + 32, bigEndian);
      PutUINT(src_y, buffer + 34, bigEndian);
    }
    else
    {
      decodeBuffer.decodeDiffCachedValue(src_x,
                         clientCache -> renderGlyphX, 16,
                             clientCache -> renderGlyphXCache, 11);

      PutUINT(src_x, buffer + 32, bigEndian);

      decodeBuffer.decodeDiffCachedValue(src_y,
                         clientCache -> renderGlyphY, 16,
                             clientCache -> renderGlyphYCache, 11);

      PutUINT(src_y, buffer + 34, bigEndian);
    }
  }

  #ifdef TEST
  *logofs << name() << ": Decoded message. Type is "
          << (unsigned int) type << " size is " << size
          << ".\n" << logofs_flush;
  #endif
}
MESSAGE_END_DECODE_MESSAGE

MESSAGE_BEGIN_ENCODE_DATA
{
  if (control -> isProtoStep8() == 1 &&
          size >= MESSAGE_OFFSET_IF_PROTO_STEP_8)
  {
    encodeCharData(encodeBuffer, buffer, MESSAGE_OFFSET_IF_PROTO_STEP_8,
                       size, bigEndian, channelCache);
  }
  else if (size > MESSAGE_OFFSET)
  {
    encodeCharData(encodeBuffer, buffer, MESSAGE_OFFSET,
                       size, bigEndian, channelCache);
  }

  #ifdef TEST
  *logofs << name() << ": Encoded " << size - MESSAGE_OFFSET
          << " bytes of text data.\n" << logofs_flush;
  #endif
}
MESSAGE_END_ENCODE_DATA

MESSAGE_BEGIN_DECODE_DATA
{
  if (control -> isProtoStep8() == 1 &&
          size >= MESSAGE_OFFSET_IF_PROTO_STEP_8)
  {
    decodeCharData(decodeBuffer, buffer, MESSAGE_OFFSET_IF_PROTO_STEP_8,
                       size, bigEndian, channelCache);
  }
  else if (size > MESSAGE_OFFSET)
  {
    decodeCharData(decodeBuffer, buffer, MESSAGE_OFFSET,
                       size, bigEndian, channelCache);
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

  renderExtension -> data.composite_glyphs.type = *(buffer + 1);
  renderExtension -> data.composite_glyphs.op   = *(buffer + 4);

  renderExtension -> data.composite_glyphs.src_id = GetULONG(buffer + 8,  bigEndian);
  renderExtension -> data.composite_glyphs.dst_id = GetULONG(buffer + 12, bigEndian);

  renderExtension -> data.composite_glyphs.format = GetULONG(buffer + 16, bigEndian);
  renderExtension -> data.composite_glyphs.set_id = GetULONG(buffer + 20, bigEndian);

  renderExtension -> data.composite_glyphs.src_x = GetUINT(buffer + 24, bigEndian);
  renderExtension -> data.composite_glyphs.src_y = GetUINT(buffer + 26, bigEndian);

  if (control -> isProtoStep8() == 1 &&
          size >= MESSAGE_OFFSET_IF_PROTO_STEP_8)
  {
    renderExtension -> data.composite_glyphs.num_elm = *(buffer + 28);

    renderExtension -> data.composite_glyphs.offset_x = GetUINT(buffer + 32, bigEndian);
    renderExtension -> data.composite_glyphs.offset_y = GetUINT(buffer + 34, bigEndian);
  }

  #ifdef TEST
  *logofs << name() << ": Parsed identity. Type is "
          << (unsigned int) renderExtension -> data.composite_glyphs.type
          << " size is " << renderExtension -> size_ << " identity size "
          << renderExtension -> i_size_ << ".\n" << logofs_flush;
  #endif
}
MESSAGE_END_PARSE_IDENTITY

MESSAGE_BEGIN_UNPARSE_IDENTITY
{
  RenderExtensionMessage *renderExtension = (RenderExtensionMessage *) message;

  *(buffer + 1) = renderExtension -> data.composite_glyphs.type;
  *(buffer + 4) = renderExtension -> data.composite_glyphs.op;

  PutULONG(renderExtension -> data.composite_glyphs.src_id, buffer + 8,  bigEndian);
  PutULONG(renderExtension -> data.composite_glyphs.dst_id, buffer + 12, bigEndian);

  PutULONG(renderExtension -> data.composite_glyphs.format, buffer + 16, bigEndian);
  PutULONG(renderExtension -> data.composite_glyphs.set_id, buffer + 20, bigEndian);

  PutUINT(renderExtension -> data.composite_glyphs.src_x, buffer + 24, bigEndian);
  PutUINT(renderExtension -> data.composite_glyphs.src_y, buffer + 26, bigEndian);

  if (control -> isProtoStep8() == 1 &&
          size >= MESSAGE_OFFSET_IF_PROTO_STEP_8)
  {
    *(buffer + 28) = renderExtension -> data.composite_glyphs.num_elm;

    PutUINT(renderExtension -> data.composite_glyphs.offset_x, buffer + 32, bigEndian);
    PutUINT(renderExtension -> data.composite_glyphs.offset_y, buffer + 34, bigEndian);
  }

  #ifdef TEST
  *logofs << name() << ": Unparsed identity. Type is "
          << (unsigned int) renderExtension -> data.composite_glyphs.type
          << " size is " << renderExtension -> size_  << " identity size "
          << renderExtension -> i_size_ <<  ".\n" << logofs_flush;
  #endif
}
MESSAGE_END_UNPARSE_IDENTITY

MESSAGE_BEGIN_IDENTITY_CHECKSUM
{
  //
  // Include minor opcode, size and
  // the composite operator in the
  // identity.
  //

  md5_append(md5_state, buffer + 1, 4);

  //
  // Include the format.
  //

  md5_append(md5_state, buffer + 16, 4);

  //
  // Also include the length of the
  // first string.
  //

  if (control -> isProtoStep8() == 1 &&
          size >= MESSAGE_OFFSET_IF_PROTO_STEP_8)
  {
    md5_append(md5_state, buffer + 28, 1);
  }
}
MESSAGE_END_IDENTITY_CHECKSUM

MESSAGE_BEGIN_ENCODE_UPDATE
{
  RenderExtensionMessage *renderExtension       = (RenderExtensionMessage *) message;
  RenderExtensionMessage *cachedRenderExtension = (RenderExtensionMessage *) cachedMessage;

  ClientCache *clientCache = (ClientCache *) channelCache;

  encodeBuffer.encodeXidValue(renderExtension -> data.composite_glyphs.src_id,
                     clientCache -> renderSrcPictureCache);

  cachedRenderExtension -> data.composite_glyphs.src_id =
              renderExtension -> data.composite_glyphs.src_id;

  encodeBuffer.encodeXidValue(renderExtension -> data.composite_glyphs.dst_id,
                     clientCache -> renderDstPictureCache);

  cachedRenderExtension -> data.composite_glyphs.dst_id =
              renderExtension -> data.composite_glyphs.dst_id;

  encodeBuffer.encodeCachedValue(renderExtension -> data.composite_glyphs.set_id, 29,
                     clientCache -> renderGlyphSetCache);

  cachedRenderExtension -> data.composite_glyphs.set_id =
              renderExtension -> data.composite_glyphs.set_id;

  //
  // Src X and Y.
  //
  // The source X and Y coordinates are
  // encoded as differerences in respect
  // to the cached message.
  //

  unsigned int value;
  unsigned int previous;

  if (control -> isProtoStep8() == 1)
  {
    value    = renderExtension -> data.composite_glyphs.src_x;
    previous = cachedRenderExtension -> data.composite_glyphs.src_x;

    encodeBuffer.encodeDiffCachedValue(value, previous, 16,
                       clientCache -> renderGlyphXCache, 11);

    cachedRenderExtension -> data.composite_glyphs.src_x = value;

    value    = renderExtension -> data.composite_glyphs.src_y;
    previous = cachedRenderExtension -> data.composite_glyphs.src_y;

    encodeBuffer.encodeDiffCachedValue(value, previous, 16,
                       clientCache -> renderGlyphYCache, 11);

    cachedRenderExtension -> data.composite_glyphs.src_y = value;
  }
  else
  {
    value    = renderExtension -> data.composite_glyphs.src_x;
    previous = cachedRenderExtension -> data.composite_glyphs.src_x;

    encodeBuffer.encodeDiffCachedValue(value, previous, 16,
                       clientCache -> renderXCache, 11);

    cachedRenderExtension -> data.composite_glyphs.src_x = value;

    value    = renderExtension -> data.composite_glyphs.src_y;
    previous = cachedRenderExtension -> data.composite_glyphs.src_y;

    encodeBuffer.encodeDiffCachedValue(value, previous, 16,
                       clientCache -> renderYCache, 11);

    cachedRenderExtension -> data.composite_glyphs.src_y = value;
  }

  #ifdef TEST
  *logofs << name() << ": Encoded source X "
          << renderExtension -> data.composite_glyphs.src_x << " source Y "
          << renderExtension -> data.composite_glyphs.src_y << ".\n"
          << logofs_flush;
  #endif

  if (control -> isProtoStep8() == 1 &&
          renderExtension -> size_ >= MESSAGE_OFFSET_IF_PROTO_STEP_8)
  {
    //
    // Offset X and Y.
    //

    if (renderExtension -> data.composite_glyphs.offset_x ==
            renderExtension -> data.composite_glyphs.src_x &&
                renderExtension -> data.composite_glyphs.offset_y ==
                    renderExtension -> data.composite_glyphs.src_y)
    {
      encodeBuffer.encodeBoolValue(0);

      cachedRenderExtension -> data.composite_glyphs.offset_x =
            renderExtension -> data.composite_glyphs.offset_x;

      cachedRenderExtension -> data.composite_glyphs.offset_y =
            renderExtension -> data.composite_glyphs.offset_y;

      #ifdef TEST
      *logofs << name() << ": Matched offset X "
              << renderExtension -> data.composite_glyphs.offset_x << " offset Y "
              << renderExtension -> data.composite_glyphs.offset_y << ".\n"
              << logofs_flush;
      #endif
    }
    else
    {
      encodeBuffer.encodeBoolValue(1);

      value    = renderExtension -> data.composite_glyphs.offset_x;
      previous = cachedRenderExtension -> data.composite_glyphs.offset_x;

      encodeBuffer.encodeDiffCachedValue(value, previous, 16,
                         clientCache -> renderGlyphXCache, 11);

      cachedRenderExtension -> data.composite_glyphs.offset_x = value;

      value    = renderExtension -> data.composite_glyphs.offset_y;
      previous = cachedRenderExtension -> data.composite_glyphs.offset_y;

      encodeBuffer.encodeDiffCachedValue(value, previous, 16,
                         clientCache -> renderGlyphYCache, 11);

      cachedRenderExtension -> data.composite_glyphs.offset_y = value;

      #ifdef TEST
      *logofs << name() << ": Missed offset X "
              << renderExtension -> data.composite_glyphs.offset_x << " offset Y "
              << renderExtension -> data.composite_glyphs.offset_y << ".\n"
              << logofs_flush;
      #endif
    }
  }

  #ifdef TEST
  *logofs << name() << ": Encoded update. Type is "
          << (unsigned int) renderExtension -> data.composite_glyphs.type
          << " size is " << renderExtension -> size_ << ".\n"
          << logofs_flush;
  #endif
}
MESSAGE_END_ENCODE_UPDATE

MESSAGE_BEGIN_DECODE_UPDATE
{
  RenderExtensionMessage *renderExtension = (RenderExtensionMessage *) message;

  ClientCache *clientCache = (ClientCache *) channelCache;

  decodeBuffer.decodeXidValue(renderExtension -> data.composite_glyphs.src_id,
                     clientCache -> renderSrcPictureCache);

  decodeBuffer.decodeXidValue(renderExtension -> data.composite_glyphs.dst_id,
                     clientCache -> renderDstPictureCache);

  decodeBuffer.decodeCachedValue(renderExtension -> data.composite_glyphs.set_id, 29,
                     clientCache -> renderGlyphSetCache);

  //
  // Src X and Y.
  //

  unsigned int value;
  unsigned int previous;

  if (control -> isProtoStep8() == 1)
  {
    previous = renderExtension -> data.composite_glyphs.src_x;

    decodeBuffer.decodeDiffCachedValue(value, previous, 16,
                       clientCache -> renderGlyphXCache, 11);

    renderExtension -> data.composite_glyphs.src_x = value;

    previous = renderExtension -> data.composite_glyphs.src_y;

    decodeBuffer.decodeDiffCachedValue(value, previous, 16,
                       clientCache -> renderGlyphYCache, 11);

    renderExtension -> data.composite_glyphs.src_y = value;
  }
  else
  {
    previous = renderExtension -> data.composite_glyphs.src_x;

    decodeBuffer.decodeDiffCachedValue(value, previous, 16,
                       clientCache -> renderXCache, 11);

    renderExtension -> data.composite_glyphs.src_x = value;

    previous = renderExtension -> data.composite_glyphs.src_y;

    decodeBuffer.decodeDiffCachedValue(value, previous, 16,
                       clientCache -> renderYCache, 11);

    renderExtension -> data.composite_glyphs.src_y = value;
  }

  if (control -> isProtoStep8() == 1 &&
          renderExtension -> size_ >= MESSAGE_OFFSET_IF_PROTO_STEP_8)
  {
    //
    // Offset X and Y.
    //

    decodeBuffer.decodeBoolValue(value);

    if (value == 0)
    {
      renderExtension -> data.composite_glyphs.offset_x =
            renderExtension -> data.composite_glyphs.src_x;

      renderExtension -> data.composite_glyphs.offset_y =
            renderExtension -> data.composite_glyphs.src_y;
    }
    else
    {
      previous = renderExtension -> data.composite_glyphs.offset_x;

      decodeBuffer.decodeDiffCachedValue(value, previous, 16,
                         clientCache -> renderGlyphXCache, 11);

      renderExtension -> data.composite_glyphs.offset_x = value;

      previous = renderExtension -> data.composite_glyphs.offset_y;

      decodeBuffer.decodeDiffCachedValue(value, previous, 16,
                         clientCache -> renderGlyphYCache, 11);

      renderExtension -> data.composite_glyphs.offset_y = value;
    }
  }

  #ifdef TEST
  *logofs << name() << ": Decoded update. Type is "
          << (unsigned int) renderExtension -> data.composite_glyphs.type
          << " size is " << renderExtension -> size_ << ".\n"
          << logofs_flush;
  #endif
}
MESSAGE_END_DECODE_UPDATE
