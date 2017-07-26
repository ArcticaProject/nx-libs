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
  // Since ProtoStep8 (#issue 108)
  md5_append(md5_state_, buffer + 1, 1);
}

void PolyLineStore::updateIdentity(EncodeBuffer &encodeBuffer, const Message *message,
                                       const Message *cachedMessage,
                                           ChannelCache *channelCache) const
{
  PolyLineMessage *polyLine       = (PolyLineMessage *) message;
  PolyLineMessage *cachedPolyLine = (PolyLineMessage *) cachedMessage;

  ClientCache *clientCache = (ClientCache *) channelCache;

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


