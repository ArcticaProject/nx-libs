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

#include "PolySegment.h"

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

int PolySegmentStore::parseIdentity(Message *message, const unsigned char *buffer,
                                        unsigned int size, int bigEndian) const
{
  PolySegmentMessage *polySegment = (PolySegmentMessage *) message;

  //
  // Here is the fingerprint.
  //

  polySegment -> drawable = GetULONG(buffer + 4, bigEndian);
  polySegment -> gcontext = GetULONG(buffer + 8, bigEndian);

  #ifdef DEBUG
  *logofs << name() << ": Parsed Identity for message at " << this << ".\n" << logofs_flush;
  #endif

  return 1;
}

int PolySegmentStore::unparseIdentity(const Message *message, unsigned char *buffer,
                                          unsigned int size, int bigEndian) const
{
  PolySegmentMessage *polySegment = (PolySegmentMessage *) message;

  //
  // Fill all the message's fields.
  //

  PutULONG(polySegment -> drawable, buffer + 4, bigEndian);
  PutULONG(polySegment -> gcontext, buffer + 8, bigEndian);

  #ifdef DEBUG
  *logofs << name() << ": Unparsed identity for message at " << this << ".\n" << logofs_flush;
  #endif

  return 1;
}

void PolySegmentStore::dumpIdentity(const Message *message) const
{
  #ifdef DUMP

  PolySegmentMessage *polySegment = (PolySegmentMessage *) message;

  *logofs << name() << ": Identity drawable " << polySegment -> drawable
                    << ", gcontext " << polySegment -> gcontext
                    << ", size " << polySegment -> size_ << ".\n" << logofs_flush;
  #endif
}

void PolySegmentStore::identityChecksum(const Message *message, const unsigned char *buffer,
                                            unsigned int size, int bigEndian) const
{
}

void PolySegmentStore::updateIdentity(EncodeBuffer &encodeBuffer, const Message *message,
                                          const Message *cachedMessage,
                                              ChannelCache *channelCache) const
{
  PolySegmentMessage *polySegment       = (PolySegmentMessage *) message;
  PolySegmentMessage *cachedPolySegment = (PolySegmentMessage *) cachedMessage;

  ClientCache *clientCache = (ClientCache *) channelCache;

  #ifdef TEST
  *logofs << name() << ": Encoding value " << polySegment -> drawable
          << " as drawable field.\n" << logofs_flush;
  #endif

  encodeBuffer.encodeXidValue(polySegment -> drawable, clientCache -> drawableCache);

  cachedPolySegment -> drawable = polySegment -> drawable;

  #ifdef TEST
  *logofs << name() << ": Encoding value " << polySegment -> gcontext
          << " as gcontext field.\n" << logofs_flush;
  #endif

  encodeBuffer.encodeXidValue(polySegment -> gcontext, clientCache -> gcCache);

  cachedPolySegment -> gcontext = polySegment -> gcontext;
}

void PolySegmentStore::updateIdentity(DecodeBuffer &decodeBuffer, const Message *message,
                                          ChannelCache *channelCache) const
{
  PolySegmentMessage *polySegment = (PolySegmentMessage *) message;

  ClientCache *clientCache = (ClientCache *) channelCache;

  unsigned int value;

  decodeBuffer.decodeXidValue(value, clientCache -> drawableCache);

  polySegment -> drawable = value;

  #ifdef DEBUG
  *logofs << name() << ": Decoded value " << polySegment -> drawable
          << " as drawable field.\n" << logofs_flush;
  #endif

  decodeBuffer.decodeXidValue(value, clientCache -> gcCache);

  polySegment -> gcontext = value;

  #ifdef DEBUG
  *logofs << name() << ": Decoded value " << polySegment -> gcontext
          << " as gcontext field.\n" << logofs_flush;
  #endif
}


