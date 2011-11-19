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

#include "SetClipRectangles.h"

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

int SetClipRectanglesStore::parseIdentity(Message *message, const unsigned char *buffer,
                                              unsigned int size, int bigEndian) const
{
  SetClipRectanglesMessage *setClipRectangles = (SetClipRectanglesMessage *) message;

  //
  // Here is the fingerprint.
  //

  setClipRectangles -> ordering = *(buffer + 1);

  setClipRectangles -> gcontext = GetULONG(buffer + 4, bigEndian);

  setClipRectangles -> x_origin = GetUINT(buffer + 8, bigEndian);
  setClipRectangles -> y_origin = GetUINT(buffer + 10, bigEndian);

  #ifdef DEBUG
  *logofs << name() << ": Parsed Identity for message at " << this << ".\n" << logofs_flush;
  #endif

  return 1;
}

int SetClipRectanglesStore::unparseIdentity(const Message *message, unsigned char *buffer,
                                                unsigned int size, int bigEndian) const
{
  SetClipRectanglesMessage *setClipRectangles = (SetClipRectanglesMessage *) message;

  //
  // Fill all the message's fields.
  //

  *(buffer + 1) = setClipRectangles -> ordering;

  PutULONG(setClipRectangles -> gcontext, buffer + 4, bigEndian);

  PutUINT(setClipRectangles -> x_origin, buffer + 8, bigEndian);
  PutUINT(setClipRectangles -> y_origin, buffer + 10, bigEndian);

  #ifdef DEBUG
  *logofs << name() << ": Unparsed identity for message at " << this << ".\n" << logofs_flush;
  #endif

  return 1;
}

void SetClipRectanglesStore::dumpIdentity(const Message *message) const
{
  #ifdef DUMP

  SetClipRectanglesMessage *setClipRectangles = (SetClipRectanglesMessage *) message;

  *logofs << name() << ": Identity ordering " << (unsigned int) setClipRectangles -> ordering
                    << ", gcontext " << setClipRectangles -> gcontext << ", x_origin " 
                    << setClipRectangles -> x_origin << ", y_origin " 
                    << setClipRectangles -> y_origin << ", size " 
                    << setClipRectangles -> size_ << ".\n" << logofs_flush;
  #endif
}

void SetClipRectanglesStore::identityChecksum(const Message *message, const unsigned char *buffer,
                                                  unsigned int size, int bigEndian) const
{
  md5_append(md5_state_, buffer + 1, 1);
  md5_append(md5_state_, buffer + 8, 4);
}

void SetClipRectanglesStore::updateIdentity(EncodeBuffer &encodeBuffer, const Message *message,
                                                const Message *cachedMessage,
                                                    ChannelCache *channelCache) const
{
  SetClipRectanglesMessage *setClipRectangles       = (SetClipRectanglesMessage *) message;
  SetClipRectanglesMessage *cachedSetClipRectangles = (SetClipRectanglesMessage *) cachedMessage;

  ClientCache *clientCache = (ClientCache *) channelCache;

  #ifdef TEST
  *logofs << name() << ": Encoding value " << setClipRectangles -> gcontext
          << " as gcontext field.\n" << logofs_flush;
  #endif

  encodeBuffer.encodeXidValue(setClipRectangles -> gcontext, clientCache -> gcCache);

  cachedSetClipRectangles -> gcontext = setClipRectangles -> gcontext;
}

void SetClipRectanglesStore::updateIdentity(DecodeBuffer &decodeBuffer, const Message *message,
                                                ChannelCache *channelCache) const
{
  SetClipRectanglesMessage *setClipRectangles = (SetClipRectanglesMessage *) message;

  ClientCache *clientCache = (ClientCache *) channelCache;

  unsigned int value;

  decodeBuffer.decodeXidValue(value, clientCache -> gcCache);

  setClipRectangles -> gcontext = value;

  #ifdef DEBUG
  *logofs << name() << ": Decoded value " << setClipRectangles -> gcontext
          << " as gcontext field.\n" << logofs_flush;
  #endif
}
