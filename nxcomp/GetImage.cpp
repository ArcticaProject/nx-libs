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

#include "GetImage.h"

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

int GetImageStore::parseIdentity(Message *message, const unsigned char *buffer,
                                     unsigned int size, int bigEndian) const
{
  GetImageMessage *getImage = (GetImageMessage *) message;

  //
  // Here is the fingerprint.
  //

  getImage -> format = *(buffer + 1);

  #ifdef TEST
  if (getImage -> format != 1 && getImage -> format != 2)
  {
    *logofs << name() << ": WARNING! Dirty value " << getImage -> format 
            << " for field format.\n" << logofs_flush;
  }
  #endif

  getImage -> drawable = GetULONG(buffer + 4, bigEndian);

  getImage -> x      = GetUINT(buffer + 8, bigEndian);
  getImage -> y      = GetUINT(buffer + 10, bigEndian);
  getImage -> width  = GetUINT(buffer + 12, bigEndian);
  getImage -> height = GetUINT(buffer + 14, bigEndian);

  getImage -> plane_mask = GetULONG(buffer + 16, bigEndian);

  #ifdef DEBUG
  *logofs << name() << ": Parsed Identity for message at " << this << ".\n" << logofs_flush;
  #endif

  return 1;
}

int GetImageStore::unparseIdentity(const Message *message, unsigned char *buffer,
                                       unsigned int size, int bigEndian) const
{
  GetImageMessage *getImage = (GetImageMessage *) message;

  //
  // Fill all the message's fields.
  //

  *(buffer + 1) = getImage -> format;

  PutULONG(getImage -> drawable, buffer + 4,  bigEndian);

  PutUINT(getImage -> x,      buffer + 8,  bigEndian);
  PutUINT(getImage -> y,      buffer + 10, bigEndian);
  PutUINT(getImage -> width,  buffer + 12, bigEndian);
  PutUINT(getImage -> height, buffer + 14, bigEndian);

  PutULONG(getImage -> plane_mask, buffer + 16,  bigEndian);

  #ifdef DEBUG
  *logofs << name() << ": Unparsed identity for message at " << this << ".\n" << logofs_flush;
  #endif

  return 1;
}

void GetImageStore::dumpIdentity(const Message *message) const
{
  #ifdef DUMP

  GetImageMessage *getImage = (GetImageMessage *) message;

  *logofs << name() << ": Identity format " << (unsigned) getImage -> format 
          << ", drawable " << getImage -> drawable << ", x " << getImage -> x 
          << ", y " << getImage -> y << ", width " << getImage -> width 
          << ", height " << getImage -> height << ", plane_mask " 
          << getImage -> plane_mask << ", size " << getImage -> size_
          << ".\n" << logofs_flush;

  #endif
}

void GetImageStore::identityChecksum(const Message *message, const unsigned char *buffer,
                                         unsigned int size, int bigEndian) const
{
  md5_append(md5_state_, buffer + 1,  1);
  md5_append(md5_state_, buffer + 8,  2);
  md5_append(md5_state_, buffer + 10, 2);
  md5_append(md5_state_, buffer + 12, 2);
  md5_append(md5_state_, buffer + 14, 2);
  md5_append(md5_state_, buffer + 16, 4);
}

void GetImageStore::updateIdentity(EncodeBuffer &encodeBuffer, const Message *message,
                                       const Message *cachedMessage,
                                           ChannelCache *channelCache) const
{
  GetImageMessage *getImage       = (GetImageMessage *) message;
  GetImageMessage *cachedGetImage = (GetImageMessage *) cachedMessage;

  ClientCache *clientCache = (ClientCache *) channelCache;

  #ifdef TEST
  *logofs << name() << ": Encoding value " << getImage -> drawable
          << " as drawable field.\n" << logofs_flush;
  #endif

  encodeBuffer.encodeXidValue(getImage -> drawable, clientCache -> drawableCache);

  cachedGetImage -> drawable = getImage -> drawable;
}

void GetImageStore::updateIdentity(DecodeBuffer &decodeBuffer, const Message *message,
                                       ChannelCache *channelCache) const
{
  GetImageMessage *getImage = (GetImageMessage *) message;

  ClientCache *clientCache = (ClientCache *) channelCache;

  unsigned int value;

  decodeBuffer.decodeXidValue(value, clientCache -> drawableCache);

  getImage -> drawable = value;

  #ifdef DEBUG
  *logofs << name() << ": Decoded value " << getImage -> drawable
          << " as drawable field.\n" << logofs_flush;
  #endif
}

