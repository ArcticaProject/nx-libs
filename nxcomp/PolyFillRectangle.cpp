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
