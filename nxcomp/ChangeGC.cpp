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

#include "ChangeGC.h"

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

int ChangeGCStore::parseIdentity(Message *message, const unsigned char *buffer,
                                     unsigned int size, int bigEndian) const
{
  ChangeGCMessage *changeGC = (ChangeGCMessage *) message;

  //
  // Here is the fingerprint.
  //

  changeGC -> gcontext   = GetULONG(buffer + 4, bigEndian);
  changeGC -> value_mask = GetULONG(buffer + 8, bigEndian);

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

    changeGC -> value_mask &= (1 << 23) - 1;

    unsigned int mask = 0x1;
    unsigned char *source = (unsigned char *) buffer + CHANGEGC_DATA_OFFSET;
    unsigned long value = 0;

    for (unsigned int i = 0; i < 23; i++)
    {
      if (changeGC -> value_mask & mask)
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
  *logofs << name() << ": Parsed Identity for message at "
          << this << ".\n" << logofs_flush;
  #endif

  return 1;
}

int ChangeGCStore::unparseIdentity(const Message *message, unsigned char *buffer,
                                       unsigned int size, int bigEndian) const
{
  ChangeGCMessage *changeGC = (ChangeGCMessage *) message;

  //
  // Fill all the message's fields.
  //

  PutULONG(changeGC -> gcontext,   buffer + 4, bigEndian);
  PutULONG(changeGC -> value_mask, buffer + 8, bigEndian);

  #ifdef DEBUG
  *logofs << name() << ": Unparsed identity for message at "
          << this << ".\n" << logofs_flush;
  #endif

  return 1;
}

void ChangeGCStore::dumpIdentity(const Message *message) const
{
  #ifdef DUMP

  ChangeGCMessage *changeGC = (ChangeGCMessage *) message;

  *logofs << name() << ": Identity gcontext " << changeGC -> gcontext 
                    << ", mask " << changeGC -> value_mask << ", size "
                    << changeGC -> size_ << ".\n" << logofs_flush;
  #endif
}

void ChangeGCStore::identityChecksum(const Message *message, const unsigned char *buffer,
                                         unsigned int size, int bigEndian) const
{
/*
  md5_append(md5_state_, buffer + 4, 8);
*/
  md5_append(md5_state_, buffer + 8, 4);
}

void ChangeGCStore::updateIdentity(EncodeBuffer &encodeBuffer, const Message *message,
                                       const Message *cachedMessage,
                                           ChannelCache *channelCache) const
{
  ChangeGCMessage *changeGC       = (ChangeGCMessage *) message;
  ChangeGCMessage *cachedChangeGC = (ChangeGCMessage *) cachedMessage;

  ClientCache *clientCache = (ClientCache *) channelCache;

  #ifdef TEST
  *logofs << name() << ": Encoding value " << changeGC -> gcontext
          << " as gcontext field.\n" << logofs_flush;
  #endif

  encodeBuffer.encodeXidValue(changeGC -> gcontext, clientCache -> gcCache);

  cachedChangeGC -> gcontext = changeGC -> gcontext;
}

void ChangeGCStore::updateIdentity(DecodeBuffer &decodeBuffer, const Message *message,
                                       ChannelCache *channelCache) const
{
  ChangeGCMessage *changeGC = (ChangeGCMessage *) message;

  ClientCache *clientCache = (ClientCache *) channelCache;

  unsigned int value;

  decodeBuffer.decodeXidValue(value, clientCache -> gcCache);

  changeGC -> gcontext = value;

  #ifdef DEBUG
  *logofs << name() << ": Decoded value " << changeGC -> gcontext
          << " as gcontext field.\n" << logofs_flush;
  #endif
}
