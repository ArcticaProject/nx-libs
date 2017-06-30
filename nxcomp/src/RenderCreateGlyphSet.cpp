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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

//
// Include the template for
// this message class.
//

#include "RenderCreateGlyphSet.h"

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

  encodeBuffer.encodeNewXidValue(GetULONG(buffer + 4, bigEndian),
                     clientCache -> lastId, clientCache -> lastIdCache,
                         clientCache -> renderGlyphSetCache,
                             clientCache -> renderFreeGlyphSetCache);

  encodeBuffer.encodeCachedValue(GetULONG(buffer + 8, bigEndian), 32,
                     clientCache -> renderFormatCache);

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

  decodeBuffer.decodeNewXidValue(value,
                     clientCache -> lastId, clientCache -> lastIdCache,
                         clientCache -> renderGlyphSetCache,
                             clientCache -> renderFreeGlyphSetCache);

  PutULONG(value, buffer + 4, bigEndian);

  decodeBuffer.decodeCachedValue(value, 32,
                     clientCache -> renderFormatCache);

  PutULONG(value, buffer + 8, bigEndian);

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

  renderExtension -> data.create_set.type = *(buffer + 1);

  renderExtension -> data.create_set.set_id = GetULONG(buffer + 4, bigEndian);
  renderExtension -> data.create_set.format = GetULONG(buffer + 8, bigEndian);

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
  PutULONG(renderExtension -> data.create_set.format,   buffer + 8,  bigEndian);

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
  md5_append(md5_state, buffer + 8, 4);
}
MESSAGE_END_IDENTITY_CHECKSUM

MESSAGE_BEGIN_ENCODE_UPDATE
{
  RenderExtensionMessage *renderExtension       = (RenderExtensionMessage *) message;
  RenderExtensionMessage *cachedRenderExtension = (RenderExtensionMessage *) cachedMessage;

  ClientCache *clientCache = (ClientCache *) channelCache;

  encodeBuffer.encodeNewXidValue(renderExtension -> data.create_set.set_id,
                     clientCache -> lastId, clientCache -> lastIdCache,
                         clientCache -> renderGlyphSetCache,
                             clientCache -> renderFreeGlyphSetCache);

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

  decodeBuffer.decodeNewXidValue(renderExtension -> data.create_set.set_id,
                     clientCache -> lastId, clientCache -> lastIdCache,
                         clientCache -> renderGlyphSetCache,
                             clientCache -> renderFreeGlyphSetCache);

  #ifdef TEST
  *logofs << name() << ": Decoded update. Type is "
          << (unsigned int) renderExtension -> data.create_set.type
          << " size is " << renderExtension -> size_ << ".\n"
          << logofs_flush;
  #endif
}
MESSAGE_END_DECODE_UPDATE
