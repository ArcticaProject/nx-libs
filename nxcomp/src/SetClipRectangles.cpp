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
