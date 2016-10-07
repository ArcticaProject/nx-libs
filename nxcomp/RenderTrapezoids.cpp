/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2011 NoMachine (http://www.nomachine.com)          */
/* Copyright (c) 2008-2014 Oleksandr Shneyder <o.shneyder@phoca-gmbh.de>  */
/* Copyright (c) 2014-2016 Ulrich Sibiller <uli42@gmx.de>                 */
/* Copyright (c) 2014-2016 Mihai Moldovan <ionic@ionic.de>                */
/* Copyright (c) 2011-2016 Mike Gabriel <mike.gabriel@das-netzwerkteam.de>*/
/* Copyright (c) 2015-2016 Qindel Group (http://www.qindel.com)           */
/*                                                                        */
/* NXCOMP, NX protocol compression and NX extensions to this software     */
/* are copyright of the aforementioned persons and companies.             */
/*                                                                        */
/* Redistribution and use of the present software is allowed according    */
/* to terms specified in the file LICENSE.nxcomp which comes in the       */
/* source distribution.                                                   */
/*                                                                        */
/* All rights reserved.                                                   */
/*                                                                        */
/* NOTE: This software has received contributions from various other      */
/* contributors, only the core maintainers and supporters are listed as   */
/* copyright holders. Please contact us, if you feel you should be listed */
/* as copyright holder, as well.                                          */
/*                                                                        */
/**************************************************************************/

//
// Include the template for
// this message class.
//

#include "RenderTrapezoids.h"

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
  // The trapezoid data is made up of a structure
  // containing a top and bottom coordinate in 4
  // bytes format, plus two lines, each represent-
  // ed as four points in 4 bytes format. Thus
  // each trapezoid is 4 * 2 + (4 * 4) * 2 = 40
  // bytes. Bytes are all padded to an long int,
  // so we don't need to clean the message.
  //

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

  decodeBuffer.decodeXidValue(value,
                     clientCache -> renderSrcPictureCache);

  PutULONG(value, buffer + 8, bigEndian);

  decodeBuffer.decodeXidValue(value,
                     clientCache -> renderDstPictureCache);

  PutULONG(value, buffer + 12, bigEndian);

  decodeBuffer.decodeCachedValue(value, 32,
                     clientCache -> renderFormatCache);

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
  if (size > MESSAGE_OFFSET)
  {
    encodeLongData(encodeBuffer, buffer, MESSAGE_OFFSET,
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
  if (size > MESSAGE_OFFSET)
  {
    decodeLongData(decodeBuffer, buffer, MESSAGE_OFFSET,
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

  renderExtension -> data.trapezoids.type = *(buffer + 1);
  renderExtension -> data.trapezoids.op   = *(buffer + 4);

  renderExtension -> data.trapezoids.src_id = GetULONG(buffer + 8,  bigEndian);
  renderExtension -> data.trapezoids.dst_id = GetULONG(buffer + 12, bigEndian);

  renderExtension -> data.trapezoids.format = GetULONG(buffer + 16, bigEndian);

  renderExtension -> data.trapezoids.src_x = GetUINT(buffer + 20, bigEndian);
  renderExtension -> data.trapezoids.src_y = GetUINT(buffer + 22, bigEndian);

  #ifdef TEST
  *logofs << name() << ": Parsed identity. Type is "
          << (unsigned int) renderExtension -> data.trapezoids.type
          << " size is " << renderExtension -> size_ << " identity size "
          << renderExtension -> i_size_ << ".\n" << logofs_flush;
  #endif
}
MESSAGE_END_PARSE_IDENTITY

MESSAGE_BEGIN_UNPARSE_IDENTITY
{
  RenderExtensionMessage *renderExtension = (RenderExtensionMessage *) message;

  *(buffer + 1) = renderExtension -> data.trapezoids.type;
  *(buffer + 4) = renderExtension -> data.trapezoids.op;

  PutULONG(renderExtension -> data.trapezoids.src_id, buffer + 8,  bigEndian);
  PutULONG(renderExtension -> data.trapezoids.dst_id, buffer + 12, bigEndian);

  PutULONG(renderExtension -> data.trapezoids.format, buffer + 16, bigEndian);

  PutUINT(renderExtension -> data.trapezoids.src_x, buffer + 20, bigEndian);
  PutUINT(renderExtension -> data.trapezoids.src_y, buffer + 22, bigEndian);

  #ifdef TEST
  *logofs << name() << ": Unparsed identity. Type is "
          << (unsigned int) renderExtension -> data.trapezoids.type
          << " size is " << renderExtension -> size_  << " identity size "
          << renderExtension -> i_size_ <<  ".\n" << logofs_flush;
  #endif
}
MESSAGE_END_UNPARSE_IDENTITY

MESSAGE_BEGIN_IDENTITY_CHECKSUM
{
  //
  // Include minor opcode, size and the
  // operator in the identity.
  //

  md5_append(md5_state, buffer + 1, 4);

  //
  // Also include the format but not the
  // x and y source.
  //

  md5_append(md5_state, buffer + 16, 4);
}
MESSAGE_END_IDENTITY_CHECKSUM

MESSAGE_BEGIN_ENCODE_UPDATE
{
  RenderExtensionMessage *renderExtension       = (RenderExtensionMessage *) message;
  RenderExtensionMessage *cachedRenderExtension = (RenderExtensionMessage *) cachedMessage;

  ClientCache *clientCache = (ClientCache *) channelCache;

  encodeBuffer.encodeXidValue(renderExtension -> data.trapezoids.src_id,
                     clientCache -> renderSrcPictureCache);

  cachedRenderExtension -> data.trapezoids.src_id =
              renderExtension -> data.trapezoids.src_id;

  encodeBuffer.encodeXidValue(renderExtension -> data.trapezoids.dst_id,
                     clientCache -> renderDstPictureCache);

  cachedRenderExtension -> data.trapezoids.dst_id =
              renderExtension -> data.trapezoids.dst_id;

  //
  // The source x and y coordinates are
  // encoded as differerences in respect
  // to the previous cached value.
  //

  unsigned int value;
  unsigned int previous;

  value    = renderExtension -> data.trapezoids.src_x;
  previous = cachedRenderExtension -> data.trapezoids.src_x;

  encodeBuffer.encodeDiffCachedValue(value, previous, 16,
                     clientCache -> renderXCache, 11);

  cachedRenderExtension -> data.trapezoids.src_x = value;

  value    = renderExtension -> data.trapezoids.src_y;
  previous = cachedRenderExtension -> data.trapezoids.src_y;

  encodeBuffer.encodeDiffCachedValue(value, previous, 16,
                     clientCache -> renderYCache, 11);

  cachedRenderExtension -> data.trapezoids.src_y = value;

  #ifdef TEST
  *logofs << name() << ": Encoded update. Type is "
          << (unsigned int) renderExtension -> data.trapezoids.type
          << " size is " << renderExtension -> size_ << " source x "
          << renderExtension -> data.trapezoids.src_x << " y "
          << renderExtension -> data.trapezoids.src_y << ".\n"
          << logofs_flush;
  #endif
}
MESSAGE_END_ENCODE_UPDATE

MESSAGE_BEGIN_DECODE_UPDATE
{
  RenderExtensionMessage *renderExtension = (RenderExtensionMessage *) message;

  ClientCache *clientCache = (ClientCache *) channelCache;

  decodeBuffer.decodeXidValue(renderExtension -> data.trapezoids.src_id,
                     clientCache -> renderSrcPictureCache);

  decodeBuffer.decodeXidValue(renderExtension -> data.trapezoids.dst_id,
                     clientCache -> renderDstPictureCache);

  unsigned int value;
  unsigned int previous;

  previous = renderExtension -> data.trapezoids.src_x;

  decodeBuffer.decodeDiffCachedValue(value, previous, 16,
                     clientCache -> renderXCache, 11);

  renderExtension -> data.trapezoids.src_x = value;

  previous = renderExtension -> data.trapezoids.src_y;

  decodeBuffer.decodeDiffCachedValue(value, previous, 16,
                     clientCache -> renderYCache, 11);

  renderExtension -> data.trapezoids.src_y = value;

  #ifdef TEST
  *logofs << name() << ": Decoded update. Type is "
          << (unsigned int) renderExtension -> data.trapezoids.type
          << " size is " << renderExtension -> size_ << " source x "
          << renderExtension -> data.trapezoids.src_x << " y "
          << renderExtension -> data.trapezoids.src_y << ".\n"
          << logofs_flush;
  #endif
}
MESSAGE_END_DECODE_UPDATE
