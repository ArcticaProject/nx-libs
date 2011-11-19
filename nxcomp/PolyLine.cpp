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

#include "PolyLine.h"

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

int PolyLineStore::parseIdentity(Message *message, const unsigned char *buffer,
                                     unsigned int size, int bigEndian) const
{
  PolyLineMessage *polyLine = (PolyLineMessage *) message;

  //
  // Here is the fingerprint.
  //

  polyLine -> mode = *(buffer + 1);

  polyLine -> drawable = GetULONG(buffer + 4, bigEndian);
  polyLine -> gcontext = GetULONG(buffer + 8, bigEndian);

  #ifdef DEBUG
  *logofs << name() << ": Parsed Identity for message at " << this << ".\n" << logofs_flush;
  #endif

  return 1;
}

int PolyLineStore::unparseIdentity(const Message *message, unsigned char *buffer,
                                       unsigned int size, int bigEndian) const
{
  PolyLineMessage *polyLine = (PolyLineMessage *) message;

  //
  // Fill all the message's fields.
  //

  *(buffer + 1) = polyLine -> mode;

  PutULONG(polyLine -> drawable, buffer + 4, bigEndian);
  PutULONG(polyLine -> gcontext, buffer + 8, bigEndian);

  #ifdef DEBUG
  *logofs << name() << ": Unparsed identity for message at " << this << ".\n" << logofs_flush;
  #endif

  return 1;
}

void PolyLineStore::dumpIdentity(const Message *message) const
{
  #ifdef DUMP

  PolyLineMessage *polyLine = (PolyLineMessage *) message;

  *logofs << name() << ": Identity drawable " << polyLine -> drawable
                    << ", gcontext " << polyLine -> gcontext
                    << ", size " << polyLine -> size_ << ".\n" << logofs_flush;
  #endif
}

void PolyLineStore::identityChecksum(const Message *message, const unsigned char *buffer,
                                         unsigned int size, int bigEndian) const
{
  if (control -> isProtoStep8() == 1)
  {
    md5_append(md5_state_, buffer + 1, 1);
  }
}

void PolyLineStore::updateIdentity(EncodeBuffer &encodeBuffer, const Message *message,
                                       const Message *cachedMessage,
                                           ChannelCache *channelCache) const
{
  PolyLineMessage *polyLine       = (PolyLineMessage *) message;
  PolyLineMessage *cachedPolyLine = (PolyLineMessage *) cachedMessage;

  ClientCache *clientCache = (ClientCache *) channelCache;

  if (control -> isProtoStep8() == 0)
  {
    encodeBuffer.encodeBoolValue((unsigned int) polyLine -> mode);
  }

  #ifdef TEST
  *logofs << name() << ": Encoding value " << polyLine -> drawable
          << " as drawable field.\n" << logofs_flush;
  #endif

  encodeBuffer.encodeXidValue(polyLine -> drawable, clientCache -> drawableCache);

  cachedPolyLine -> drawable = polyLine -> drawable;

  #ifdef TEST
  *logofs << name() << ": Encoding value " << polyLine -> gcontext
          << " as gcontext field.\n" << logofs_flush;
  #endif

  encodeBuffer.encodeXidValue(polyLine -> gcontext, clientCache -> gcCache);

  cachedPolyLine -> gcontext = polyLine -> gcontext;
}

void PolyLineStore::updateIdentity(DecodeBuffer &decodeBuffer, const Message *message,
                                       ChannelCache *channelCache) const
{
  PolyLineMessage *polyLine = (PolyLineMessage *) message;

  ClientCache *clientCache = (ClientCache *) channelCache;

  unsigned int value;

  if (control -> isProtoStep8() == 0)
  {
    decodeBuffer.decodeBoolValue(value);

    polyLine -> mode = value;
  }

  decodeBuffer.decodeXidValue(value, clientCache -> drawableCache);

  polyLine -> drawable = value;

  #ifdef DEBUG
  *logofs << name() << ": Decoded value " << polyLine -> drawable
          << " as drawable field.\n" << logofs_flush;
  #endif

  decodeBuffer.decodeXidValue(value, clientCache -> gcCache);

  polyLine -> gcontext = value;

  #ifdef DEBUG
  *logofs << name() << ": Decoded value " << polyLine -> gcontext
          << " as gcontext field.\n" << logofs_flush;
  #endif
}


