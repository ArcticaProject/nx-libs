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
  // Since ProtoStep8 (#issue 108)
  md5_append(md5_state_, buffer + 1, 1);
}

void PolyPointStore::updateIdentity(EncodeBuffer &encodeBuffer, const Message *message,
                                        const Message *cachedMessage,
                                            ChannelCache *channelCache) const
{
  PolyPointMessage *polyPoint       = (PolyPointMessage *) message;
  PolyPointMessage *cachedPolyPoint = (PolyPointMessage *) cachedMessage;

  ClientCache *clientCache = (ClientCache *) channelCache;

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


