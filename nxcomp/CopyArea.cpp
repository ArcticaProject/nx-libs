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

#include "CopyArea.h"

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

int CopyAreaStore::parseIdentity(Message *message, const unsigned char *buffer,
                                     unsigned int size, int bigEndian) const
{
  CopyAreaMessage *copyArea = (CopyAreaMessage *) message;

  //
  // Here is the fingerprint.
  //

  copyArea -> src_drawable = GetULONG(buffer + 4, bigEndian); 
  copyArea -> dst_drawable = GetULONG(buffer + 8, bigEndian); 
  copyArea -> gcontext     = GetULONG(buffer + 12, bigEndian);

  copyArea -> src_x  = GetUINT(buffer + 16, bigEndian);
  copyArea -> src_y  = GetUINT(buffer + 18, bigEndian);
  copyArea -> dst_x  = GetUINT(buffer + 20, bigEndian);
  copyArea -> dst_y  = GetUINT(buffer + 22, bigEndian);

  copyArea -> width  = GetUINT(buffer + 24, bigEndian);
  copyArea -> height = GetUINT(buffer + 26, bigEndian);

  #ifdef DEBUG
  *logofs << name() << ": Parsed Identity for message at " << this << ".\n" << logofs_flush;
  #endif

  return 1;
}

int CopyAreaStore::unparseIdentity(const Message *message, unsigned char *buffer,
                                       unsigned int size, int bigEndian) const
{
  CopyAreaMessage *copyArea = (CopyAreaMessage *) message;

  //
  // Fill all the message's fields.
  //

  PutULONG(copyArea -> src_drawable, buffer + 4, bigEndian);
  PutULONG(copyArea -> dst_drawable, buffer + 8, bigEndian);
  PutULONG(copyArea -> gcontext,     buffer + 12, bigEndian);

  PutUINT(copyArea -> src_x, buffer + 16, bigEndian);
  PutUINT(copyArea -> src_y, buffer + 18, bigEndian);
  PutUINT(copyArea -> dst_x, buffer + 20, bigEndian);
  PutUINT(copyArea -> dst_y, buffer + 22, bigEndian);

  PutUINT(copyArea -> width,  buffer + 24, bigEndian);
  PutUINT(copyArea -> height, buffer + 26, bigEndian);

  #ifdef DEBUG
  *logofs << name() << ": Unparsed identity for message at " << this << ".\n" << logofs_flush;
  #endif

  return 1;
}

void CopyAreaStore::dumpIdentity(const Message *message) const
{
  #ifdef DUMP

  CopyAreaMessage *copyArea = (CopyAreaMessage *) message;

  *logofs << name() << ": Identity src_drawable " << copyArea -> src_drawable 
          << ", dst_drawable " << copyArea -> dst_drawable << ", gcontext " << copyArea -> gcontext
          << ", src_x " << copyArea -> src_x << ", src_y " << copyArea -> src_y
          << ", dst_x " << copyArea -> dst_x << ", dst_y " << copyArea -> dst_y
          << ", width  " << copyArea -> width << ", height " << copyArea -> height
          << ", size " << copyArea -> size_ << ".\n";

  #endif
}

void CopyAreaStore::identityChecksum(const Message *message, const unsigned char *buffer,
                                         unsigned int size, int bigEndian) const
{
  md5_append(md5_state_, buffer + 16, 12);
}

void CopyAreaStore::updateIdentity(EncodeBuffer &encodeBuffer, const Message *message,
                                       const Message *cachedMessage,
                                           ChannelCache *channelCache) const
{
  CopyAreaMessage *copyArea       = (CopyAreaMessage *) message;
  CopyAreaMessage *cachedCopyArea = (CopyAreaMessage *) cachedMessage;

  ClientCache *clientCache = (ClientCache *) channelCache;

  #ifdef TEST
  *logofs << name() << ": Encoding value " << copyArea -> src_drawable
          << " as " << "src_drawable" << " field.\n" << logofs_flush;
  #endif

  encodeBuffer.encodeXidValue(copyArea -> src_drawable, clientCache -> drawableCache);

  cachedCopyArea -> src_drawable = copyArea -> src_drawable;

  #ifdef TEST
  *logofs << name() << ": Encoding value " << copyArea -> dst_drawable
          << " as " << "dst_drawable" << " field.\n" << logofs_flush;
  #endif

  encodeBuffer.encodeXidValue(copyArea -> dst_drawable, clientCache -> drawableCache);

  cachedCopyArea -> dst_drawable = copyArea -> dst_drawable;

  #ifdef TEST
  *logofs << name() << ": Encoding value " << copyArea -> gcontext
          << " as " << "gcontext" << " field.\n" << logofs_flush;
  #endif

  encodeBuffer.encodeXidValue(copyArea -> gcontext, clientCache -> gcCache);

  cachedCopyArea -> gcontext = copyArea -> gcontext;
}

void CopyAreaStore::updateIdentity(DecodeBuffer &decodeBuffer, const Message *message,
                                       ChannelCache *channelCache) const
{
  CopyAreaMessage *copyArea = (CopyAreaMessage *) message;

  ClientCache *clientCache = (ClientCache *) channelCache;

  unsigned int value;

  decodeBuffer.decodeXidValue(value, clientCache -> drawableCache);

  copyArea -> src_drawable = value;

  #ifdef DEBUG
  *logofs << name() << ": Decoded value " << copyArea -> src_drawable
          << " as " << "src_drawable" << " field.\n" << logofs_flush;
  #endif

  decodeBuffer.decodeXidValue(value, clientCache -> drawableCache);

  copyArea -> dst_drawable = value;

  #ifdef DEBUG
  *logofs << name() << ": Decoded value " << copyArea -> dst_drawable
          << " as " << "dst_drawable" << " field.\n" << logofs_flush;
  #endif

  decodeBuffer.decodeXidValue(value, clientCache -> gcCache);

  copyArea -> gcontext = value;

  #ifdef DEBUG
  *logofs << name() << ": Decoded value " << copyArea -> gcontext
          << " as gcontext field.\n" << logofs_flush;
  #endif
}


