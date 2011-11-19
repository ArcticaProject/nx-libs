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

#include "RenderComposite.h"

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

MESSAGE_BEGIN_ENCODE_MESSAGE
{
  ClientCache *clientCache = (ClientCache *) channelCache;

  encodeBuffer.encodeCachedValue(*(buffer + 4), 8,
                     clientCache -> renderOpCache);

  encodeBuffer.encodeXidValue(GetULONG(buffer + 8, bigEndian),
                     clientCache -> renderSrcPictureCache);

  encodeBuffer.encodeXidValue(GetULONG(buffer + 12, bigEndian),
                     clientCache -> renderMaskPictureCache);

  encodeBuffer.encodeXidValue(GetULONG(buffer + 16, bigEndian),
                     clientCache -> renderDstPictureCache);

  //
  // Src X and Y.
  //

  encodeBuffer.encodeDiffCachedValue(GetUINT(buffer + 20, bigEndian),
                     clientCache -> renderLastX, 16,
                         clientCache -> renderXCache, 11);

  encodeBuffer.encodeDiffCachedValue(GetUINT(buffer + 22, bigEndian),
                     clientCache -> renderLastY, 16,
                         clientCache -> renderYCache, 11);
  //
  // Mask X and Y.
  //

  encodeBuffer.encodeDiffCachedValue(GetUINT(buffer + 24, bigEndian),
                     clientCache -> renderLastX, 16,
                         clientCache -> renderXCache, 11);

  encodeBuffer.encodeDiffCachedValue(GetUINT(buffer + 26, bigEndian),
                     clientCache -> renderLastY, 16,
                         clientCache -> renderYCache, 11);

  //
  // Dst X and Y.
  //

  encodeBuffer.encodeDiffCachedValue(GetUINT(buffer + 28, bigEndian),
                     clientCache -> renderLastX, 16,
                         clientCache -> renderXCache, 11);

  encodeBuffer.encodeDiffCachedValue(GetUINT(buffer + 30, bigEndian),
                     clientCache -> renderLastY, 16,
                         clientCache -> renderYCache, 11);

  //
  // Width and height.
  //

  encodeBuffer.encodeCachedValue(GetUINT(buffer + 32, bigEndian), 16,
                     clientCache -> renderWidthCache, 11);

  encodeBuffer.encodeCachedValue(GetUINT(buffer + 34, bigEndian), 16,
                     clientCache -> renderHeightCache, 11);

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

  decodeBuffer.decodeXidValue(value, clientCache -> renderMaskPictureCache);

  PutULONG(value, buffer + 12, bigEndian);

  decodeBuffer.decodeXidValue(value, clientCache -> renderDstPictureCache);

  PutULONG(value, buffer + 16, bigEndian);

  //
  // Src X and Y.
  //

  decodeBuffer.decodeDiffCachedValue(value,
                     clientCache -> renderLastX, 16,
                         clientCache -> renderXCache, 11);

  PutUINT(clientCache -> renderLastX, buffer + 20, bigEndian);

  decodeBuffer.decodeDiffCachedValue(value,
                     clientCache -> renderLastY, 16,
                         clientCache -> renderYCache, 11);

  PutUINT(clientCache -> renderLastY, buffer + 22, bigEndian);

  //
  // Mask X and Y.
  //

  decodeBuffer.decodeDiffCachedValue(value,
                     clientCache -> renderLastX, 16,
                         clientCache -> renderXCache, 11);

  PutUINT(clientCache -> renderLastX, buffer + 24, bigEndian);

  decodeBuffer.decodeDiffCachedValue(value,
                     clientCache -> renderLastY, 16,
                         clientCache -> renderYCache, 11);

  PutUINT(clientCache -> renderLastY, buffer + 26, bigEndian);

  //
  // Dst X and Y.
  //

  decodeBuffer.decodeDiffCachedValue(value,
                     clientCache -> renderLastX, 16,
                         clientCache -> renderXCache, 11);

  PutUINT(clientCache -> renderLastX, buffer + 28, bigEndian);

  decodeBuffer.decodeDiffCachedValue(value,
                     clientCache -> renderLastY, 16,
                         clientCache -> renderYCache, 11);

  PutUINT(clientCache -> renderLastY, buffer + 30, bigEndian);

  //
  // Width and height.
  //

  decodeBuffer.decodeCachedValue(value, 16,
                     clientCache -> renderWidthCache, 11);

  PutUINT(value, buffer + 32, bigEndian);

  decodeBuffer.decodeCachedValue(value, 16,
                     clientCache -> renderHeightCache, 11);

  PutUINT(value, buffer + 34, bigEndian);

  #ifdef TEST
  *logofs << name() << ": Decoded message. Type is "
          << (unsigned int) type << " size is " << size
          << ".\n" << logofs_flush;
  #endif
}
MESSAGE_END_DECODE_MESSAGE

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

  renderExtension -> data.composite.msk_x = GetUINT(buffer + 24, bigEndian);
  renderExtension -> data.composite.msk_y = GetUINT(buffer + 26, bigEndian);

  renderExtension -> data.composite.dst_x = GetUINT(buffer + 28, bigEndian);
  renderExtension -> data.composite.dst_y = GetUINT(buffer + 30, bigEndian);

  renderExtension -> data.composite.width  = GetUINT(buffer + 32, bigEndian);
  renderExtension -> data.composite.height = GetUINT(buffer + 34, bigEndian);

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

  PutUINT(renderExtension -> data.composite.msk_x, buffer + 24, bigEndian);
  PutUINT(renderExtension -> data.composite.msk_y, buffer + 26, bigEndian);

  PutUINT(renderExtension -> data.composite.dst_x, buffer + 28, bigEndian);
  PutUINT(renderExtension -> data.composite.dst_y, buffer + 30, bigEndian);

  PutUINT(renderExtension -> data.composite.width,  buffer + 32, bigEndian);
  PutUINT(renderExtension -> data.composite.height, buffer + 34, bigEndian);

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
  // Include the minor opcode and size in the
  // identity, plus the operator, the x and y
  // of the source and mask and the width and
  // height of the destination.
  //

  md5_append(md5_state, buffer + 1,  4);
  md5_append(md5_state, buffer + 20, 8);
  md5_append(md5_state, buffer + 32, 4);
}
MESSAGE_END_IDENTITY_CHECKSUM

MESSAGE_BEGIN_ENCODE_UPDATE
{
  RenderExtensionMessage *renderExtension       = (RenderExtensionMessage *) message;
  RenderExtensionMessage *cachedRenderExtension = (RenderExtensionMessage *) cachedMessage;

  ClientCache *clientCache = (ClientCache *) channelCache;

  #ifdef DEBUG
  *logofs << name() << ": Source " << renderExtension -> data.composite.src_id
          << " mask " << renderExtension -> data.composite.msk_id
          << " destination " << renderExtension -> data.composite.msk_id
          << ".\n" << logofs_flush;
  #endif

  encodeBuffer.encodeXidValue(renderExtension -> data.composite.src_id,
                     clientCache -> renderSrcPictureCache);

  cachedRenderExtension -> data.composite.src_id =
              renderExtension -> data.composite.src_id;

  encodeBuffer.encodeXidValue(renderExtension -> data.composite.msk_id,
                     clientCache -> renderMaskPictureCache);

  cachedRenderExtension -> data.composite.msk_id =
              renderExtension -> data.composite.msk_id;

  encodeBuffer.encodeXidValue(renderExtension -> data.composite.dst_id,
                     clientCache -> renderDstPictureCache);

  cachedRenderExtension -> data.composite.dst_id =
              renderExtension -> data.composite.dst_id;

  //
  // Dst X and Y.
  //

  unsigned int value;
  unsigned int previous;

  value    = renderExtension -> data.composite.dst_x;
  previous = cachedRenderExtension -> data.composite.dst_x;

  encodeBuffer.encodeDiffCachedValue(value, previous, 16,
                     clientCache -> renderXCache, 11);

  cachedRenderExtension -> data.composite.dst_x = value;

  value    = renderExtension -> data.composite.dst_y;
  previous = cachedRenderExtension -> data.composite.dst_y;

  encodeBuffer.encodeDiffCachedValue(value, previous, 16,
                     clientCache -> renderYCache, 11);

  cachedRenderExtension -> data.composite.dst_y = value;

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
                     clientCache -> renderMaskPictureCache);

  decodeBuffer.decodeXidValue(renderExtension -> data.composite.dst_id,
                     clientCache -> renderDstPictureCache);

  //
  // Dst X and Y.
  //

  unsigned int value;
  unsigned int previous;

  previous = renderExtension -> data.composite.dst_x;

  decodeBuffer.decodeDiffCachedValue(value, previous, 16,
                     clientCache -> renderXCache, 11);

  renderExtension -> data.composite.dst_x = value;

  previous = renderExtension -> data.composite.dst_y;

  decodeBuffer.decodeDiffCachedValue(value, previous, 16,
                     clientCache -> renderYCache, 11);

  renderExtension -> data.composite.dst_y = value;

  #ifdef TEST
  *logofs << name() << ": Decoded update. Type is "
          << (unsigned int) renderExtension -> data.composite.type
          << " size is " << renderExtension -> size_ << ".\n"
          << logofs_flush;
  #endif
}
MESSAGE_END_DECODE_UPDATE
