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

#include "PolyPoint.h"

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

int PolyPointStore::parseIdentity(Message *message, const unsigned char *buffer,
                                      unsigned int size, int bigEndian) const
{
  PolyPointMessage *polyPoint = (PolyPointMessage *) message;

  //
  // Here is the fingerprint.
  //

  polyPoint -> mode = *(buffer + 1);

  polyPoint -> drawable = GetULONG(buffer + 4, bigEndian);
  polyPoint -> gcontext = GetULONG(buffer + 8, bigEndian);

  #ifdef DEBUG
  *logofs << name() << ": Parsed Identity for message at " << this << ".\n" << logofs_flush;
  #endif

  return 1;
}

int PolyPointStore::unparseIdentity(const Message *message, unsigned char *buffer,
                                        unsigned int size, int bigEndian) const
{
  PolyPointMessage *polyPoint = (PolyPointMessage *) message;

  //
  // Fill all the message's fields.
  //

  *(buffer + 1) = polyPoint -> mode;

  PutULONG(polyPoint -> drawable, buffer + 4, bigEndian);
  PutULONG(polyPoint -> gcontext, buffer + 8, bigEndian);

  #ifdef DEBUG
  *logofs << name() << ": Unparsed identity for message at " << this << ".\n" << logofs_flush;
  #endif

  return 1;
}

void PolyPointStore::dumpIdentity(const Message *message) const
{
  #ifdef DUMP

  PolyPointMessage *polyPoint = (PolyPointMessage *) message;

  *logofs << name() << ": Identity drawable " << polyPoint -> drawable
                    << ", gcontext " << polyPoint -> gcontext
                    << ", size " << polyPoint -> size_ << ".\n" << logofs_flush;
  #endif
}

void PolyPointStore::identityChecksum(const Message *message, const unsigned char *buffer,
                                          unsigned int size, int bigEndian) const
{
  if (control -> isProtoStep8() == 1)
  {
    md5_append(md5_state_, buffer + 1, 1);
  }
}

void PolyPointStore::updateIdentity(EncodeBuffer &encodeBuffer, const Message *message,
                                        const Message *cachedMessage,
                                            ChannelCache *channelCache) const
{
  PolyPointMessage *polyPoint       = (PolyPointMessage *) message;
  PolyPointMessage *cachedPolyPoint = (PolyPointMessage *) cachedMessage;

  ClientCache *clientCache = (ClientCache *) channelCache;

  if (control -> isProtoStep8() == 0)
  {
    encodeBuffer.encodeBoolValue((unsigned int) polyPoint -> mode);
  }

  #ifdef TEST
  *logofs << name() << ": Encoding value " << polyPoint -> drawable
          << " as drawable field.\n" << logofs_flush;
  #endif

  encodeBuffer.encodeXidValue(polyPoint -> drawable, clientCache -> drawableCache);

  cachedPolyPoint -> drawable = polyPoint -> drawable;

  #ifdef TEST
  *logofs << name() << ": Encoding value " << polyPoint -> gcontext
          << " as gcontext field.\n" << logofs_flush;
  #endif

  encodeBuffer.encodeXidValue(polyPoint -> gcontext, clientCache -> gcCache);

  cachedPolyPoint -> gcontext = polyPoint -> gcontext;
}

void PolyPointStore::updateIdentity(DecodeBuffer &decodeBuffer, const Message *message,
                                        ChannelCache *channelCache) const
{
  PolyPointMessage *polyPoint = (PolyPointMessage *) message;

  ClientCache *clientCache = (ClientCache *) channelCache;

  unsigned int value;

  if (control -> isProtoStep8() == 0)
  {
    decodeBuffer.decodeBoolValue(value);

    polyPoint -> mode = value;
  }

  decodeBuffer.decodeXidValue(value, clientCache -> drawableCache);

  polyPoint -> drawable = value;

  #ifdef DEBUG
  *logofs << name() << ": Decoded value " << polyPoint -> drawable
          << " as drawable field.\n" << logofs_flush;
  #endif

  decodeBuffer.decodeXidValue(value, clientCache -> gcCache);

  polyPoint -> gcontext = value;

  #ifdef DEBUG
  *logofs << name() << ": Decoded value " << polyPoint -> gcontext
          << " as gcontext field.\n" << logofs_flush;
  #endif
}


