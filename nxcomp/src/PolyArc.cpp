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

#include "PolyArc.h"

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

int PolyArcStore::parseIdentity(Message *message, const unsigned char *buffer,
                                    unsigned int size, int bigEndian) const
{
  PolyArcMessage *polyArc = (PolyArcMessage *) message;

  //
  // Here is the fingerprint.
  //

  polyArc -> drawable = GetULONG(buffer + 4, bigEndian);
  polyArc -> gcontext = GetULONG(buffer + 8, bigEndian);

  #ifdef DEBUG
  *logofs << name() << ": Parsed Identity for message at " << this << ".\n" << logofs_flush;
  #endif

  return 1;
}

int PolyArcStore::unparseIdentity(const Message *message, unsigned char *buffer,
                                      unsigned int size, int bigEndian) const
{
  PolyArcMessage *polyArc = (PolyArcMessage *) message;

  //
  // Fill all the message's fields.
  //

  PutULONG(polyArc -> drawable, buffer + 4, bigEndian);
  PutULONG(polyArc -> gcontext, buffer + 8, bigEndian);

  #ifdef DEBUG
  *logofs << name() << ": Unparsed identity for message at " << this << ".\n" << logofs_flush;
  #endif

  return 1;
}

void PolyArcStore::dumpIdentity(const Message *message) const
{
  #ifdef DUMP

  PolyArcMessage *polyArc = (PolyArcMessage *) message;

  *logofs << name() << ": Identity drawable " << polyArc -> drawable
                    << ", gcontext " << polyArc -> gcontext
                    << ", size " << polyArc -> size_ << ".\n" << logofs_flush;
  #endif
}

void PolyArcStore::identityChecksum(const Message *message, const unsigned char *buffer,
                                        unsigned int size, int bigEndian) const
{
}

void PolyArcStore::updateIdentity(EncodeBuffer &encodeBuffer, const Message *message,
                                      const Message *cachedMessage,
                                          ChannelCache *channelCache) const
{
  PolyArcMessage *polyArc       = (PolyArcMessage *) message;
  PolyArcMessage *cachedPolyArc = (PolyArcMessage *) cachedMessage;

  ClientCache *clientCache = (ClientCache *) channelCache;

  #ifdef TEST
  *logofs << name() << ": Encoding value " << polyArc -> drawable
          << " as drawable field.\n" << logofs_flush;
  #endif

  encodeBuffer.encodeXidValue(polyArc -> drawable, clientCache -> drawableCache);

  cachedPolyArc -> drawable = polyArc -> drawable;

  #ifdef TEST
  *logofs << name() << ": Encoding value " << polyArc -> gcontext
          << " as gcontext field.\n" << logofs_flush;
  #endif

  encodeBuffer.encodeXidValue(polyArc -> gcontext, clientCache -> gcCache);

  cachedPolyArc -> gcontext = polyArc -> gcontext;
}

void PolyArcStore::updateIdentity(DecodeBuffer &decodeBuffer, const Message *message,
                                      ChannelCache *channelCache) const
{
  PolyArcMessage *polyArc = (PolyArcMessage *) message;

  ClientCache *clientCache = (ClientCache *) channelCache;

  unsigned int value;

  decodeBuffer.decodeXidValue(value, clientCache -> drawableCache);

  polyArc -> drawable = value;

  #ifdef DEBUG
  *logofs << name() << ": Decoded value " << polyArc -> drawable
          << " as drawable field.\n" << logofs_flush;
  #endif

  decodeBuffer.decodeXidValue(value, clientCache -> gcCache);

  polyArc -> gcontext = value;

  #ifdef DEBUG
  *logofs << name() << ": Decoded value " << polyArc -> gcontext
          << " as gcontext field.\n" << logofs_flush;
  #endif
}


