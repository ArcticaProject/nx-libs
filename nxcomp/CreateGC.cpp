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

#include "CreateGC.h"

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

int CreateGCStore::parseIdentity(Message *message, const unsigned char *buffer,
                                     unsigned int size, int bigEndian) const
{
  CreateGCMessage *createGC = (CreateGCMessage *) message;

  //
  // Here is the fingerprint.
  //

  createGC -> gcontext   = GetULONG(buffer + 4, bigEndian);
  createGC -> drawable   = GetULONG(buffer + 8, bigEndian);
  createGC -> value_mask = GetULONG(buffer + 12, bigEndian);

  //
  // Clear the unused bytes carried in the
  // payload to increase the effectiveness
  // of the caching algorithm.
  //

  if ((int) size > dataOffset)
  {
    #ifdef DEBUG
    *logofs << name() << ": Removing unused bytes from the "
            << "data payload.\n" << logofs_flush;
    #endif

    createGC -> value_mask &= (1 << 23) - 1;

    unsigned int mask = 0x1;
    unsigned char *source = (unsigned char *) buffer + CREATEGC_DATA_OFFSET;
    unsigned long value = 0;

    for (unsigned int i = 0; i < 23; i++)
    {
      if (createGC -> value_mask & mask)
      {
        value = GetULONG(source, bigEndian);

        value &= (0xffffffff >> (32 - CREATEGC_FIELD_WIDTH[i]));

        PutULONG(value, source, bigEndian);

        source += 4;
      }

      mask <<= 1;
    }
  }

  #ifdef DEBUG
  *logofs << name() << ": Parsed identity for message at "
          << this << ".\n" << logofs_flush;
  #endif

  return 1;
}

int CreateGCStore::unparseIdentity(const Message *message, unsigned char *buffer,
                                       unsigned int size, int bigEndian) const
{
  CreateGCMessage *createGC = (CreateGCMessage *) message;

  //
  // Fill all the message's fields.
  //

  PutULONG(createGC -> gcontext,   buffer + 4, bigEndian);
  PutULONG(createGC -> drawable,   buffer + 8, bigEndian);
  PutULONG(createGC -> value_mask, buffer + 12, bigEndian);

  #ifdef DEBUG
  *logofs << name() << ": Unparsed identity for message at "
          << this << ".\n" << logofs_flush;
  #endif

  return 1;
}

void CreateGCStore::dumpIdentity(const Message *message) const
{
  #ifdef DUMP

  CreateGCMessage *createGC = (CreateGCMessage *) message;

  *logofs << name() << ": Identity gcontext " << createGC -> gcontext << ", drawable "
                    << createGC -> drawable << ", value_mask " << createGC -> value_mask
                    << ", size " << createGC -> size_ << ".\n" << logofs_flush;
  #endif
}

void CreateGCStore::identityChecksum(const Message *message, const unsigned char *buffer,
                                         unsigned int size, int bigEndian) const
{
  //
  // This didn't include the drawable
  // in previous versions.
  //

  md5_append(md5_state_, buffer + 8, 8);
}

void CreateGCStore::updateIdentity(EncodeBuffer &encodeBuffer, const Message *message,
                                       const Message *cachedMessage,
                                           ChannelCache *channelCache) const
{
  CreateGCMessage *createGC       = (CreateGCMessage *) message;
  CreateGCMessage *cachedCreateGC = (CreateGCMessage *) cachedMessage;

  ClientCache *clientCache = (ClientCache *) channelCache;

  if (control -> isProtoStep7() == 1)
  {
    #ifdef TEST
    *logofs << name() << ": Encoding value " << createGC -> gcontext
            << " as gcontext field.\n" << logofs_flush;
    #endif

    encodeBuffer.encodeNewXidValue(createGC -> gcontext, clientCache -> lastId,
                       clientCache -> lastIdCache, clientCache -> gcCache,
                           clientCache -> freeGCCache);

    cachedCreateGC -> gcontext = createGC -> gcontext;
  }
  else
  {
    #ifdef TEST
    *logofs << name() << ": Encoding value " << createGC -> drawable
            << " as drawable field.\n" << logofs_flush;
    #endif

    encodeBuffer.encodeXidValue(createGC -> drawable, clientCache -> drawableCache);

    cachedCreateGC -> drawable = createGC -> drawable;

    #ifdef TEST
    *logofs << name() << ": Encoding value " << createGC -> gcontext
            << " as gcontext field.\n" << logofs_flush;
    #endif

    encodeBuffer.encodeXidValue(createGC -> gcontext, clientCache -> gcCache);

    cachedCreateGC -> gcontext = createGC -> gcontext;
  }
}

void CreateGCStore::updateIdentity(DecodeBuffer &decodeBuffer, const Message *message,
                                       ChannelCache *channelCache) const
{
  CreateGCMessage *createGC = (CreateGCMessage *) message;

  ClientCache *clientCache = (ClientCache *) channelCache;

  unsigned int value;

  if (control -> isProtoStep7() == 1)
  {
    decodeBuffer.decodeNewXidValue(value, clientCache -> lastId,
                       clientCache -> lastIdCache, clientCache -> gcCache,
                           clientCache -> freeGCCache);

    createGC -> gcontext = value;

    #ifdef TEST
    *logofs << name() << ": Decoded value " << createGC -> gcontext
            << " as gcontext field.\n" << logofs_flush;
    #endif
  }
  else
  {
    decodeBuffer.decodeXidValue(value, clientCache -> drawableCache);

    createGC -> drawable = value;

    #ifdef TEST
    *logofs << name() << ": Decoded value " << createGC -> drawable
            << " as drawable field.\n" << logofs_flush;
    #endif

    decodeBuffer.decodeXidValue(value, clientCache -> gcCache);

    createGC -> gcontext = value;

    #ifdef TEST
    *logofs << name() << ": Decoded value " << createGC -> gcontext
            << " as gcontext field.\n" << logofs_flush;
    #endif
  }
}
