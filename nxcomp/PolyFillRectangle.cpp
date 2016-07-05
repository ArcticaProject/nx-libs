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

#include "PolyFillRectangle.h"

#include "ClientCache.h"

#include "EncodeBuffer.h"
#include "DecodeBuffer.h"

//
// Set the verbosity level.
//

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG
#undef  DUMP

//
// Here are the methods to handle messages' content.
//

int PolyFillRectangleStore::parseIdentity(Message *message, const unsigned char *buffer,
                                              unsigned int size, int bigEndian) const
{
  PolyFillRectangleMessage *polyFillRectangle = (PolyFillRectangleMessage *) message;

  //
  // Here is the fingerprint.
  //

  polyFillRectangle -> drawable = GetULONG(buffer + 4, bigEndian);
  polyFillRectangle -> gcontext = GetULONG(buffer + 8, bigEndian);

  #ifdef DEBUG
  *logofs << name() << ": Parsed Identity for message at " << this << ".\n" << logofs_flush;
  #endif

  return 1;
}

int PolyFillRectangleStore::unparseIdentity(const Message *message, unsigned char *buffer,
                                                unsigned int size, int bigEndian) const
{
  PolyFillRectangleMessage *polyFillRectangle = (PolyFillRectangleMessage *) message;

  //
  // Fill all the message's fields.
  //

  PutULONG(polyFillRectangle -> drawable, buffer + 4, bigEndian);
  PutULONG(polyFillRectangle -> gcontext, buffer + 8, bigEndian);

  #ifdef DEBUG
  *logofs << name() << ": Unparsed identity for message at " << this << ".\n" << logofs_flush;
  #endif

  return 1;
}

void PolyFillRectangleStore::dumpIdentity(const Message *message) const
{
  #ifdef DUMP

  PolyFillRectangleMessage *polyFillRectangle = (PolyFillRectangleMessage *) message;

  *logofs << name() << ": Identity drawable " << polyFillRectangle -> drawable
                    << ", gcontext " << polyFillRectangle -> gcontext
                    << ", size " << polyFillRectangle -> size_ << ".\n" << logofs_flush;
  #endif
}

void PolyFillRectangleStore::identityChecksum(const Message *message, const unsigned char *buffer,
                                                  unsigned int size, int bigEndian) const
{
}

void PolyFillRectangleStore::updateIdentity(EncodeBuffer &encodeBuffer, const Message *message,
                                                const Message *cachedMessage,
                                                    ChannelCache *channelCache) const
{
  PolyFillRectangleMessage *polyFillRectangle       = (PolyFillRectangleMessage *) message;
  PolyFillRectangleMessage *cachedPolyFillRectangle = (PolyFillRectangleMessage *) cachedMessage;

  ClientCache *clientCache = (ClientCache *) channelCache;

  #ifdef DEBUG
  *logofs << name() << ": Encoding value " << polyFillRectangle -> drawable
          << " as drawable field.\n" << logofs_flush;
  #endif

  encodeBuffer.encodeXidValue(polyFillRectangle -> drawable, clientCache -> drawableCache);

  cachedPolyFillRectangle -> drawable = polyFillRectangle -> drawable;

  #ifdef DEBUG
  *logofs << name() << ": Encoding value " << polyFillRectangle -> gcontext
          << " as gcontext field.\n" << logofs_flush;
  #endif

  encodeBuffer.encodeXidValue(polyFillRectangle -> gcontext, clientCache -> gcCache);

  cachedPolyFillRectangle -> gcontext = polyFillRectangle -> gcontext;
}

void PolyFillRectangleStore::updateIdentity(DecodeBuffer &decodeBuffer, const Message *message,
                                                ChannelCache *channelCache) const
{
  PolyFillRectangleMessage *polyFillRectangle = (PolyFillRectangleMessage *) message;

  ClientCache *clientCache = (ClientCache *) channelCache;

  unsigned int value;

  decodeBuffer.decodeXidValue(value, clientCache -> drawableCache);

  polyFillRectangle -> drawable = value;

  #ifdef DEBUG
  *logofs << name() << ": Decoded value " << polyFillRectangle -> drawable
          << " as drawable field.\n" << logofs_flush;
  #endif

  decodeBuffer.decodeXidValue(value, clientCache -> gcCache);

  polyFillRectangle -> gcontext = value;

  #ifdef DEBUG
  *logofs << name() << ": Decoded value " << polyFillRectangle -> gcontext
          << " as gcontext field.\n" << logofs_flush;
  #endif
}
