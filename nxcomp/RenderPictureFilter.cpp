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

#include "RenderPictureFilter.h"

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

  encodeBuffer.encodeXidValue(GetULONG(buffer + 4, bigEndian),
                     clientCache -> renderSrcPictureCache);

  encodeBuffer.encodeCachedValue(GetUINT(buffer + 8, bigEndian), 16,
                     clientCache -> renderLengthCache, 5);

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

  decodeBuffer.decodeCachedValue(value, 16,
                     clientCache -> renderLengthCache, 5);

  PutUINT(value, buffer + 8, bigEndian);

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

  renderExtension -> data.picture_filter.type = *(buffer + 1);

  renderExtension -> data.picture_filter.src_id = GetULONG(buffer + 4, bigEndian);
  renderExtension -> data.picture_filter.num_elm = GetUINT(buffer + 8, bigEndian);

  //
  // Clean the padding bytes. This
  // should be the purpose of the
  // filter.
  //

  #ifdef TEST
  *logofs << name() << ": Cleaning "
          << RoundUp4(renderExtension -> data.picture_filter.num_elm) -
             renderExtension -> data.picture_filter.num_elm << " bytes "
          << "at offset " << MESSAGE_OFFSET + renderExtension ->
             data.picture_filter.num_elm << " with " << renderExtension ->
             data.picture_filter.num_elm << " elements and size "
          << renderExtension -> size_ << ".\n" << logofs_flush;
  #endif

  if (size >= MESSAGE_OFFSET + renderExtension ->
          data.picture_filter.num_elm)
  {
    unsigned char *next = (unsigned char *) buffer +
                              MESSAGE_OFFSET + renderExtension ->
                                  data.picture_filter.num_elm;

    while (next < buffer + size)
    {
      *next++ = '\0';
    }
  }

  #ifdef TEST
  *logofs << name() << ": Parsed identity. Type is "
          << (unsigned int) renderExtension -> data.picture_filter.type
          << " size is " << renderExtension -> size_ << ".\n"
          << logofs_flush;
  #endif
}
MESSAGE_END_PARSE_IDENTITY

MESSAGE_BEGIN_UNPARSE_IDENTITY
{
  RenderExtensionMessage *renderExtension = (RenderExtensionMessage *) message;

  *(buffer + 1) = renderExtension -> data.picture_filter.type;

  PutULONG(renderExtension -> data.picture_filter.src_id, buffer + 4, bigEndian);
  PutUINT(renderExtension -> data.picture_filter.num_elm, buffer + 8, bigEndian);

  #ifdef TEST
  *logofs << name() << ": Unparsed identity. Type is "
          << (unsigned int) renderExtension -> data.picture_filter.type
          << " size is " << renderExtension -> size_ << ".\n"
          << logofs_flush;
  #endif
}
MESSAGE_END_UNPARSE_IDENTITY

MESSAGE_BEGIN_IDENTITY_CHECKSUM
{
  //
  // Include the length of the filter name
  // in the checksum.
  //

  md5_append(md5_state, buffer + 1, 3);
  md5_append(md5_state, buffer + 8, 2);
}
MESSAGE_END_IDENTITY_CHECKSUM

MESSAGE_BEGIN_ENCODE_UPDATE
{
  RenderExtensionMessage *renderExtension       = (RenderExtensionMessage *) message;
  RenderExtensionMessage *cachedRenderExtension = (RenderExtensionMessage *) cachedMessage;

  ClientCache *clientCache = (ClientCache *) channelCache;

  encodeBuffer.encodeXidValue(renderExtension -> data.picture_filter.src_id,
                     clientCache -> renderSrcPictureCache);

  cachedRenderExtension -> data.picture_filter.src_id =
              renderExtension -> data.picture_filter.src_id;

  #ifdef TEST
  *logofs << name() << ": Encoded update. Type is "
          << (unsigned int) renderExtension -> data.picture_filter.type
          << " size is " << renderExtension -> size_ << ".\n"
          << logofs_flush;
  #endif
}
MESSAGE_END_ENCODE_UPDATE

MESSAGE_BEGIN_DECODE_UPDATE
{
  RenderExtensionMessage *renderExtension = (RenderExtensionMessage *) message;

  ClientCache *clientCache = (ClientCache *) channelCache;

  decodeBuffer.decodeXidValue(renderExtension -> data.picture_filter.src_id,
                     clientCache -> renderSrcPictureCache);

  #ifdef TEST
  *logofs << name() << ": Decoded update. Type is "
          << (unsigned int) renderExtension -> data.picture_filter.type
          << " size is " << renderExtension -> size_ << ".\n"
          << logofs_flush;
  #endif
}
MESSAGE_END_DECODE_UPDATE
