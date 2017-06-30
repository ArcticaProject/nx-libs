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


