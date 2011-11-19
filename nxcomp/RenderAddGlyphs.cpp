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

#include "RenderAddGlyphs.h"

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

  encodeBuffer.encodeCachedValue(GetULONG(buffer + 4, bigEndian), 29,
                     clientCache -> renderGlyphSetCache);

  encodeBuffer.encodeCachedValue(GetULONG(buffer + 8, bigEndian), 32,
                     clientCache -> renderNumGlyphsCache, 8);

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

  decodeBuffer.decodeCachedValue(value, 29,
                     clientCache -> renderGlyphSetCache);

  PutULONG(value, buffer + 4, bigEndian);

  decodeBuffer.decodeCachedValue(value, 32,
                     clientCache -> renderNumGlyphsCache, 8);

  PutULONG(value, buffer + 8, bigEndian);

  #ifdef TEST
  *logofs << name() << ": Decoded message. Type is "
          << (unsigned int) type << " size is " << size
          << ".\n" << logofs_flush;
  #endif
}
MESSAGE_END_DECODE_MESSAGE

MESSAGE_BEGIN_ENCODE_DATA
{
  encodeCharData(encodeBuffer, buffer, MESSAGE_OFFSET,
                     size, bigEndian, channelCache);

  #ifdef TEST
  *logofs << name() << ": Encoded " << size - MESSAGE_OFFSET
          << " bytes of data.\n" << logofs_flush;
  #endif
}
MESSAGE_END_ENCODE_DATA

MESSAGE_BEGIN_DECODE_DATA
{
  decodeCharData(decodeBuffer, buffer, MESSAGE_OFFSET,
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

  renderExtension -> data.add_glyphs.type = *(buffer + 1);

  renderExtension -> data.add_glyphs.set_id  = GetULONG(buffer + 4, bigEndian);
  renderExtension -> data.add_glyphs.num_elm = GetULONG(buffer + 8,  bigEndian);

  #ifdef TEST
  *logofs << name() << ": Parsed identity. Type is "
          << (unsigned int) renderExtension -> data.add_glyphs.type
          << " size is " << renderExtension -> size_ << ".\n"
          << logofs_flush;
  #endif
}
MESSAGE_END_PARSE_IDENTITY

MESSAGE_BEGIN_UNPARSE_IDENTITY
{
  RenderExtensionMessage *renderExtension = (RenderExtensionMessage *) message;

  *(buffer + 1) = renderExtension -> data.add_glyphs.type;

  PutULONG(renderExtension -> data.add_glyphs.set_id,  buffer + 4,  bigEndian);
  PutULONG(renderExtension -> data.add_glyphs.num_elm, buffer + 8,  bigEndian);

  #ifdef TEST
  *logofs << name() << ": Unparsed identity. Type is "
          << (unsigned int) renderExtension -> data.add_glyphs.type
          << " size is " << renderExtension -> size_ << ".\n"
          << logofs_flush;
  #endif
}
MESSAGE_END_UNPARSE_IDENTITY

MESSAGE_BEGIN_IDENTITY_CHECKSUM
{
  md5_append(md5_state, buffer + 1, 3);
  md5_append(md5_state, buffer + 8, 4);
}
MESSAGE_END_IDENTITY_CHECKSUM

MESSAGE_BEGIN_ENCODE_UPDATE
{
  RenderExtensionMessage *renderExtension       = (RenderExtensionMessage *) message;
  RenderExtensionMessage *cachedRenderExtension = (RenderExtensionMessage *) cachedMessage;

  ClientCache *clientCache = (ClientCache *) channelCache;

  encodeBuffer.encodeCachedValue(renderExtension -> data.add_glyphs.set_id, 29,
                     clientCache -> renderGlyphSetCache);

  cachedRenderExtension -> data.add_glyphs.set_id =
              renderExtension -> data.add_glyphs.set_id;

  #ifdef TEST
  *logofs << name() << ": Encoded update. Type is "
          << (unsigned int) renderExtension -> data.add_glyphs.type
          << " size is " << renderExtension -> size_ << ".\n"
          << logofs_flush;
  #endif
}
MESSAGE_END_ENCODE_UPDATE

MESSAGE_BEGIN_DECODE_UPDATE
{
  RenderExtensionMessage *renderExtension = (RenderExtensionMessage *) message;

  ClientCache *clientCache = (ClientCache *) channelCache;

  decodeBuffer.decodeCachedValue(renderExtension -> data.add_glyphs.set_id, 29,
                     clientCache -> renderGlyphSetCache);

  #ifdef TEST
  *logofs << name() << ": Decoded update. Type is "
          << (unsigned int) renderExtension -> data.add_glyphs.type
          << " size is " << renderExtension -> size_ << ".\n"
          << logofs_flush;
  #endif
}
MESSAGE_END_DECODE_UPDATE
