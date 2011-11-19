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

#include "RenderCompositeGlyphsCompat.h"

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

  //
  // The offset points 8 bytes after
  // the beginning of the data part.
  //

  #ifdef DEBUG
  *logofs << name() << ": Encoding value "
          << ((size - (MESSAGE_OFFSET - 8)) >> 2)
          << ".\n" << logofs_flush;
  #endif

  encodeBuffer.encodeCachedValue((size - (MESSAGE_OFFSET - 8)) >> 2, 16,
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

  size = (MESSAGE_OFFSET - 8) + (size << 2);

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

  encodeBuffer.encodeCachedValue(GetULONG(buffer + 16, bigEndian), 32,
                     clientCache -> renderFormatCache);

  encodeBuffer.encodeCachedValue(GetULONG(buffer + 20, bigEndian), 29,
                     clientCache -> renderGlyphSetCache);

  encodeBuffer.encodeDiffCachedValue(GetUINT(buffer + 24, bigEndian),
                     clientCache -> renderLastX, 16,
                         clientCache -> renderXCache, 11);

  encodeBuffer.encodeDiffCachedValue(GetUINT(buffer + 26, bigEndian),
                     clientCache -> renderLastY, 16,
                         clientCache -> renderYCache, 11);

  //
  // Try to save as many bits as possible by
  // encoding the information about the first
  // set of glyphs.
  //

  if (size >= MESSAGE_OFFSET)
  {
    unsigned int numGlyphs = *(buffer + 28);

    encodeBuffer.encodeCachedValue(numGlyphs, 8,
                       clientCache -> renderNumGlyphsCache);

    encodeBuffer.encodeCachedValue(GetUINT(buffer + 32, bigEndian), 16,
                       clientCache -> renderWidthCache, 11);

    encodeBuffer.encodeCachedValue(GetUINT(buffer + 34, bigEndian), 16,
                       clientCache -> renderHeightCache, 11);

    //
    // Only manage the first set of glyphs,
    // that is in most cases the only one.
    //

    switch (*(buffer + 1))
    {
      case X_RenderCompositeGlyphs8:
      {
        if (numGlyphs & 0x03)
        {
          memset((unsigned char *) buffer + MESSAGE_OFFSET + numGlyphs, '\0',
                     RoundUp4(numGlyphs) - numGlyphs);
        }

        break;
      }
      case X_RenderCompositeGlyphs16:
      {
        if (numGlyphs & 0x01)
        {
          memset((unsigned char *) buffer + MESSAGE_OFFSET + (numGlyphs * 2), '\0',
                     RoundUp4(numGlyphs * 2) - numGlyphs * 2);
        }

        break;
      }
    }

    #ifdef TEST
    if (*(buffer + (size - 1)) != '\0')
    {
      *logofs << name() << ": WARNING! Final byte is non-zero with size "
              << size << " and " << (unsigned int) *(buffer + 28)
              << " glyphs.\n" << logofs_flush;
    }
    else
    {
      *logofs << name() << ": Final byte is zero with size "
              << size << " and " << (unsigned int) *(buffer + 28)
              << " glyphs.\n" << logofs_flush;
    }
    #endif
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
                     clientCache -> renderSrcPictureCache);

  PutULONG(value, buffer + 12, bigEndian);

  decodeBuffer.decodeCachedValue(value, 32,
                     clientCache -> renderFormatCache);

  PutULONG(value, buffer + 16, bigEndian);

  decodeBuffer.decodeCachedValue(value, 29,
                     clientCache -> renderGlyphSetCache);

  PutULONG(value, buffer + 20, bigEndian);

  decodeBuffer.decodeDiffCachedValue(value,
                     clientCache -> renderLastX, 16,
                         clientCache -> renderXCache, 11);

  PutUINT(clientCache -> renderLastX, buffer + 24, bigEndian);

  decodeBuffer.decodeDiffCachedValue(value,
                     clientCache -> renderLastY, 16,
                         clientCache -> renderYCache, 11);

  PutUINT(clientCache -> renderLastY, buffer + 26, bigEndian);

  if (size >= MESSAGE_OFFSET)
  {
    decodeBuffer.decodeCachedValue(value, 8,
                       clientCache -> renderNumGlyphsCache);

    *(buffer + 28) = value;

    decodeBuffer.decodeCachedValue(value, 16,
                       clientCache -> renderWidthCache, 11);

    PutUINT(value, buffer + 32, bigEndian);

    decodeBuffer.decodeCachedValue(value, 16,
                       clientCache -> renderHeightCache, 11);

    PutUINT(value, buffer + 34, bigEndian);
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
  ClientCache *clientCache = (ClientCache *) channelCache;

  unsigned int value;

  switch (*(buffer + 1))
  {
    case X_RenderCompositeGlyphs8:
    {
      clientCache -> renderTextCompressor.reset();

      const unsigned char *next = buffer + MESSAGE_OFFSET;

      for (unsigned int i = MESSAGE_OFFSET; i < size; i++)
      {
        #ifdef DEBUG
        *logofs << name() << ": Encoding char with i = " << i
                << ".\n" << logofs_flush;
        #endif

        clientCache -> renderTextCompressor.
              encodeChar(*next++, encodeBuffer);
      }

      break;
    }
    case X_RenderCompositeGlyphs16:
    {
      for (unsigned int i = MESSAGE_OFFSET; i < size; i += 2)
      {
        value = GetUINT(buffer + i, bigEndian);

        #ifdef DEBUG
        *logofs << name() << ": Encoding short with i = " << i
                << ".\n" << logofs_flush;
        #endif

        encodeBuffer.encodeCachedValue(value, 16,
                           *clientCache -> renderCompositeGlyphsDataCache[clientCache ->
                                renderLastCompositeGlyphsData]);

        clientCache -> renderLastCompositeGlyphsData = value % 16;
      }

      break;
    }
    default:
    {
      for (unsigned int i = MESSAGE_OFFSET; i < size; i += 4)
      {
        value = GetULONG(buffer + i, bigEndian);

        #ifdef DEBUG
        *logofs << name() << ": Encoding long with i = " << i
                << ".\n" << logofs_flush;
        #endif

        encodeBuffer.encodeCachedValue(value, 32,
                           *clientCache -> renderCompositeGlyphsDataCache[clientCache ->
                                renderLastCompositeGlyphsData]);

        clientCache -> renderLastCompositeGlyphsData = value % 16;
      }

      break;
    }
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

  switch (*(buffer + 1))
  {
    case X_RenderCompositeGlyphs8:
    {
      clientCache -> renderTextCompressor.reset();

      unsigned char *next = buffer + MESSAGE_OFFSET;

      for (unsigned int i = MESSAGE_OFFSET; i < size; i++)
      {
        #ifdef DEBUG
        *logofs << name() << ": Decoding char with i = " << i
                << ".\n" << logofs_flush;
        #endif

        *next++ = clientCache -> renderTextCompressor.
                        decodeChar(decodeBuffer);
      }

      break;
    }
    case X_RenderCompositeGlyphs16:
    {
      for (unsigned int i = MESSAGE_OFFSET; i < size; i += 2)
      {
        #ifdef DEBUG
        *logofs << name() << ": Decoding short with i = " << i
                << ".\n" << logofs_flush;
        #endif

        decodeBuffer.decodeCachedValue(value, 16,
                           *clientCache -> renderCompositeGlyphsDataCache[clientCache ->
                                  renderLastCompositeGlyphsData]);

        PutUINT(value, buffer + i, bigEndian);

        clientCache -> renderLastCompositeGlyphsData = value % 16;
      }

      break;
    }
    default:
    {
      for (unsigned int i = MESSAGE_OFFSET; i < size; i += 4)
      {
        #ifdef DEBUG
        *logofs << name() << ": Decoding long with i = " << i
                << ".\n" << logofs_flush;
        #endif

        decodeBuffer.decodeCachedValue(value, 32,
                           *clientCache -> renderCompositeGlyphsDataCache[clientCache ->
                                  renderLastCompositeGlyphsData]);

        PutULONG(value, buffer + i, bigEndian);

        clientCache -> renderLastCompositeGlyphsData = value % 16;
      }

      break;
    }
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

  renderExtension -> data.composite_glyphs_compat.type = *(buffer + 1);
  renderExtension -> data.composite_glyphs_compat.op   = *(buffer + 4);

  renderExtension -> data.composite_glyphs_compat.src_id = GetULONG(buffer + 8,  bigEndian);
  renderExtension -> data.composite_glyphs_compat.dst_id = GetULONG(buffer + 12, bigEndian);

  renderExtension -> data.composite_glyphs_compat.format = GetULONG(buffer + 16, bigEndian);
  renderExtension -> data.composite_glyphs_compat.set_id = GetULONG(buffer + 20, bigEndian);

  renderExtension -> data.composite_glyphs_compat.src_x = GetUINT(buffer + 24, bigEndian);
  renderExtension -> data.composite_glyphs_compat.src_y = GetUINT(buffer + 26, bigEndian);

  if (size >= MESSAGE_OFFSET)
  {
    renderExtension -> data.composite_glyphs_compat.num_elm = *(buffer + 28);

    renderExtension -> data.composite_glyphs_compat.delta_x = GetUINT(buffer + 32, bigEndian);
    renderExtension -> data.composite_glyphs_compat.delta_y = GetUINT(buffer + 34, bigEndian);
  }

  #ifdef TEST
  *logofs << name() << ": Parsed identity. Type is "
          << (unsigned int) renderExtension -> data.composite_glyphs_compat.type
          << " size is " << renderExtension -> size_ << " identity size "
          << renderExtension -> i_size_ << ".\n"
          << logofs_flush;
  #endif
}
MESSAGE_END_PARSE_IDENTITY

MESSAGE_BEGIN_UNPARSE_IDENTITY
{
  RenderExtensionMessage *renderExtension = (RenderExtensionMessage *) message;

  *(buffer + 1) = renderExtension -> data.composite_glyphs_compat.type;
  *(buffer + 4) = renderExtension -> data.composite_glyphs_compat.op;

  PutULONG(renderExtension -> data.composite_glyphs_compat.src_id, buffer + 8,  bigEndian);
  PutULONG(renderExtension -> data.composite_glyphs_compat.dst_id, buffer + 12, bigEndian);

  PutULONG(renderExtension -> data.composite_glyphs_compat.format, buffer + 16, bigEndian);
  PutULONG(renderExtension -> data.composite_glyphs_compat.set_id, buffer + 20, bigEndian);

  PutUINT(renderExtension -> data.composite_glyphs_compat.src_x, buffer + 24, bigEndian);
  PutUINT(renderExtension -> data.composite_glyphs_compat.src_y, buffer + 26, bigEndian);

  if (size >= MESSAGE_OFFSET)
  {
    *(buffer + 28) = renderExtension -> data.composite_glyphs_compat.num_elm;

    PutUINT(renderExtension -> data.composite_glyphs_compat.delta_x, buffer + 32, bigEndian);
    PutUINT(renderExtension -> data.composite_glyphs_compat.delta_y, buffer + 34, bigEndian);

    #ifdef DEBUG
    *logofs << name() << ": Len is " << (unsigned int) *(buffer + 28)
            << " delta X is " << GetUINT(buffer + 32, bigEndian)
            << " delta Y is " << GetUINT(buffer + 34, bigEndian)
            << ".\n" << logofs_flush;

    *logofs << name() << ": Pad 1 is " << (unsigned int) *(buffer + 29)
            << " pad 2 and 3 are " << GetUINT(buffer + 30, bigEndian)
            << ".\n" << logofs_flush;
    #endif
  }

  #ifdef TEST
  *logofs << name() << ": Unparsed identity. Type is "
          << (unsigned int) renderExtension -> data.composite_glyphs_compat.type
          << " size is " << renderExtension -> size_  << " identity size "
          << renderExtension -> i_size_ << ".\n" << ".\n"
          << logofs_flush;
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
  // Include the format and the source
  // x and y fields.
  //

  md5_append(md5_state, buffer + 16, 4);
  md5_append(md5_state, buffer + 24, 4);

  //
  // Include the number of glyphs.
  //

  if (size >= MESSAGE_OFFSET)
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

  encodeBuffer.encodeXidValue(renderExtension -> data.composite_glyphs_compat.src_id,
                     clientCache -> renderSrcPictureCache);

  cachedRenderExtension -> data.composite_glyphs_compat.src_id =
              renderExtension -> data.composite_glyphs_compat.src_id;

  encodeBuffer.encodeXidValue(renderExtension -> data.composite_glyphs_compat.dst_id,
                     clientCache -> renderSrcPictureCache);

  cachedRenderExtension -> data.composite_glyphs_compat.dst_id =
              renderExtension -> data.composite_glyphs_compat.dst_id;

  encodeBuffer.encodeCachedValue(renderExtension -> data.composite_glyphs_compat.set_id, 29,
                     clientCache -> renderGlyphSetCache);

  cachedRenderExtension -> data.composite_glyphs_compat.set_id =
              renderExtension -> data.composite_glyphs_compat.set_id;

  if (renderExtension -> size_ >= MESSAGE_OFFSET)
  {
    encodeBuffer.encodeCachedValue(renderExtension -> data.composite_glyphs_compat.delta_x, 16,
                       clientCache -> renderWidthCache, 11);

    cachedRenderExtension -> data.composite_glyphs_compat.delta_x =
                renderExtension -> data.composite_glyphs_compat.delta_x;

    encodeBuffer.encodeCachedValue(renderExtension -> data.composite_glyphs_compat.delta_y, 16,
                       clientCache -> renderHeightCache, 11);

    cachedRenderExtension -> data.composite_glyphs_compat.delta_y =
                renderExtension -> data.composite_glyphs_compat.delta_y;
  }

  #ifdef TEST
  *logofs << name() << ": Encoded update. Type is "
          << (unsigned int) renderExtension -> data.composite_glyphs_compat.type
          << " size is " << renderExtension -> size_ << ".\n"
          << logofs_flush;
  #endif
}
MESSAGE_END_ENCODE_UPDATE

MESSAGE_BEGIN_DECODE_UPDATE
{
  RenderExtensionMessage *renderExtension = (RenderExtensionMessage *) message;

  ClientCache *clientCache = (ClientCache *) channelCache;

  decodeBuffer.decodeXidValue(renderExtension -> data.composite_glyphs_compat.src_id,
                     clientCache -> renderSrcPictureCache);

  decodeBuffer.decodeXidValue(renderExtension -> data.composite_glyphs_compat.dst_id,
                     clientCache -> renderSrcPictureCache);

  decodeBuffer.decodeCachedValue(renderExtension -> data.composite_glyphs_compat.set_id, 29,
                     clientCache -> renderGlyphSetCache);

  if (renderExtension -> size_ >= MESSAGE_OFFSET)
  {
    unsigned int value;

    decodeBuffer.decodeCachedValue(value, 16,
                       clientCache -> renderWidthCache, 11);

    renderExtension -> data.composite_glyphs_compat.delta_x = value;

    decodeBuffer.decodeCachedValue(value, 16,
                       clientCache -> renderHeightCache, 11);

    renderExtension -> data.composite_glyphs_compat.delta_y = value;
  }

  #ifdef TEST
  *logofs << name() << ": Decoded update. Type is "
          << (unsigned int) renderExtension -> data.composite_glyphs_compat.type
          << " size is " << renderExtension -> size_ << ".\n"
          << logofs_flush;
  #endif
}
MESSAGE_END_DECODE_UPDATE
