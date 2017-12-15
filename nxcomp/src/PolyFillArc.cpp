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

#include "PolyFillArc.h"

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

int PolyFillArcStore::parseIdentity(Message *message, const unsigned char *buffer,
                                        unsigned int size, int bigEndian) const
{
  PolyFillArcMessage *polyFillArc = (PolyFillArcMessage *) message;

  //
  // Here is the fingerprint.
  //

  polyFillArc -> drawable = GetULONG(buffer + 4, bigEndian);
  polyFillArc -> gcontext = GetULONG(buffer + 8, bigEndian);

  #ifdef DEBUG
  *logofs << name() << ": Parsed Identity for message at " << this << ".\n" << logofs_flush;
  #endif

  return 1;
}

int PolyFillArcStore::unparseIdentity(const Message *message, unsigned char *buffer,
                                          unsigned int size, int bigEndian) const
{
  PolyFillArcMessage *polyFillArc = (PolyFillArcMessage *) message;

  //
  // Fill all the message's fields.
  //

  PutULONG(polyFillArc -> drawable, buffer + 4, bigEndian);
  PutULONG(polyFillArc -> gcontext, buffer + 8, bigEndian);

  #ifdef DEBUG
  *logofs << name() << ": Unparsed identity for message at " << this << ".\n" << logofs_flush;
  #endif

  return 1;
}

void PolyFillArcStore::dumpIdentity(const Message *message) const
{
  #ifdef DUMP

  PolyFillArcMessage *polyFillArc = (PolyFillArcMessage *) message;

  *logofs << name() << ": Identity drawable " << polyFillArc -> drawable
                    << ", gcontext " << polyFillArc -> gcontext
                    << ", size " << polyFillArc -> size_ << ".\n" << logofs_flush;
  #endif
}

void PolyFillArcStore::identityChecksum(const Message *message, const unsigned char *buffer,
                                            unsigned int size, int bigEndian) const
{
}

void PolyFillArcStore::updateIdentity(EncodeBuffer &encodeBuffer, const Message *message,
                                          const Message *cachedMessage,
                                              ChannelCache *channelCache) const
{
  PolyFillArcMessage *polyFillArc       = (PolyFillArcMessage *) message;
  PolyFillArcMessage *cachedPolyFillArc = (PolyFillArcMessage *) cachedMessage;

  ClientCache *clientCache = (ClientCache *) channelCache;

  #ifdef TEST
  *logofs << name() << ": Encoding value " << polyFillArc -> drawable
          << " as drawable field.\n" << logofs_flush;
  #endif

  encodeBuffer.encodeXidValue(polyFillArc -> drawable, clientCache -> drawableCache);

  cachedPolyFillArc -> drawable = polyFillArc -> drawable;

  #ifdef TEST
  *logofs << name() << ": Encoding value " << polyFillArc -> gcontext
          << " as gcontext field.\n" << logofs_flush;
  #endif

  encodeBuffer.encodeXidValue(polyFillArc -> gcontext, clientCache -> gcCache);

  cachedPolyFillArc -> gcontext = polyFillArc -> gcontext;
}

void PolyFillArcStore::updateIdentity(DecodeBuffer &decodeBuffer, const Message *message,
                                          ChannelCache *channelCache) const
{
  PolyFillArcMessage *polyFillArc = (PolyFillArcMessage *) message;

  ClientCache *clientCache = (ClientCache *) channelCache;

  unsigned int value;

  decodeBuffer.decodeXidValue(value, clientCache -> drawableCache);

  polyFillArc -> drawable = value;

  #ifdef DEBUG
  *logofs << name() << ": Decoded value " << polyFillArc -> drawable
          << " as drawable field.\n" << logofs_flush;
  #endif

  decodeBuffer.decodeXidValue(value, clientCache -> gcCache);

  polyFillArc -> gcontext = value;

  #ifdef DEBUG
  *logofs << name() << ": Decoded value " << polyFillArc -> gcontext
          << " as gcontext field.\n" << logofs_flush;
  #endif
}


