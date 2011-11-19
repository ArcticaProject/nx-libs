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

#include "SendEvent.h"

#include "ClientCache.h"

#include "EncodeBuffer.h"
#include "DecodeBuffer.h"

#include "IntCache.h"

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

int SendEventStore::parseIdentity(Message *message, const unsigned char *buffer,
                                      unsigned int size, int bigEndian) const
{
  SendEventMessage *sendEvent = (SendEventMessage *) message;

  //
  // Here is the fingerprint.
  //

  sendEvent -> propagate = *(buffer + 1);

  sendEvent -> window = GetULONG(buffer + 4, bigEndian);
  sendEvent -> mask   = GetULONG(buffer + 8, bigEndian);

  sendEvent -> code      = *(buffer + 12);
  sendEvent -> byte_data = *(buffer + 13);

  sendEvent -> sequence = GetUINT(buffer + 14, bigEndian);

  sendEvent -> int_data = GetULONG(buffer + 16, bigEndian);

  #ifdef DEBUG
  *logofs << name() << ": Parsed Identity for message at "
           << this << ".\n" << logofs_flush;
  #endif

  return 1;
}

int SendEventStore::unparseIdentity(const Message *message, unsigned char *buffer,
                                        unsigned int size, int bigEndian) const
{
  SendEventMessage *sendEvent = (SendEventMessage *) message;

  //
  // Fill all the message's fields.
  //

  *(buffer + 1) = sendEvent -> propagate;

  PutULONG(sendEvent -> window, buffer + 4,  bigEndian);
  PutULONG(sendEvent -> mask,   buffer + 8,  bigEndian);

  *(buffer + 12) = sendEvent -> code;
  *(buffer + 13) = sendEvent -> byte_data;

  PutUINT(sendEvent -> sequence, buffer + 14, bigEndian);

  PutULONG(sendEvent -> int_data, buffer + 16, bigEndian);

  #ifdef DEBUG
  *logofs << name() << ": Unparsed identity for message at "
          << this << ".\n" << logofs_flush;
  #endif

  return 1;
}

void SendEventStore::dumpIdentity(const Message *message) const
{
  #ifdef DUMP

  SendEventMessage *sendEvent = (SendEventMessage *) message;

  *logofs << name() << ": Identity propagate " << (unsigned int) sendEvent -> propagate 
          << ", window " << sendEvent -> window << ", mask " << sendEvent -> mask
          << ", code " << (unsigned int) sendEvent -> code << ", byte_data "
          << (unsigned int) sendEvent -> byte_data << ", sequence "
          << sendEvent -> sequence << ", int_data " << sendEvent -> int_data 
          << ", size " << sendEvent -> size_ << ".\n" << logofs_flush;

  #endif
}

void SendEventStore::identityChecksum(const Message *message, const unsigned char *buffer,
                                          unsigned int size, int bigEndian) const
{
}

void SendEventStore::updateIdentity(EncodeBuffer &encodeBuffer, const Message *message,
                                        const Message *cachedMessage,
                                            ChannelCache *channelCache) const
{
  SendEventMessage *sendEvent       = (SendEventMessage *) message;
  SendEventMessage *cachedSendEvent = (SendEventMessage *) cachedMessage;

  ClientCache *clientCache = (ClientCache *) channelCache;

  #ifdef TEST
  *logofs << name() << ": Encoding value " << (unsigned int) sendEvent -> propagate
          << " as propagate field.\n" << logofs_flush;
  #endif

  encodeBuffer.encodeBoolValue(sendEvent -> propagate);

  cachedSendEvent -> propagate = sendEvent -> propagate;

  #ifdef TEST
  *logofs << name() << ": Encoding value " << sendEvent -> window
          << " as window field.\n" << logofs_flush;
  #endif

  if (sendEvent -> window == 0 || sendEvent -> window == 1)
  {
    encodeBuffer.encodeBoolValue(1);

    encodeBuffer.encodeBoolValue(sendEvent -> window);
  }
  else
  {
    encodeBuffer.encodeBoolValue(0);

    encodeBuffer.encodeXidValue(sendEvent -> window, clientCache -> windowCache);
  }

  cachedSendEvent -> window = sendEvent -> window;

  #ifdef TEST
  *logofs << name() << ": Encoding value " << sendEvent -> mask
          << " as mask field.\n" << logofs_flush;
  #endif

  encodeBuffer.encodeCachedValue(sendEvent -> mask, 32,
                     clientCache -> sendEventMaskCache);

  cachedSendEvent -> mask = sendEvent -> mask;

  #ifdef TEST
  *logofs << name() << ": Encoding value " << sendEvent -> code
          << " as code field.\n" << logofs_flush;
  #endif

  encodeBuffer.encodeCachedValue(sendEvent -> code, 8, 
                     clientCache -> sendEventCodeCache);

  cachedSendEvent -> code = sendEvent -> code;

  #ifdef TEST
  *logofs << name() << ": Encoding value " << sendEvent -> byte_data
          << " as byte_data field.\n" << logofs_flush;
  #endif

  encodeBuffer.encodeCachedValue(sendEvent -> byte_data, 8, 
                     clientCache -> sendEventByteDataCache);

  cachedSendEvent -> byte_data = sendEvent -> byte_data;

  #ifdef TEST
  *logofs << name() << ": Encoding value " << sendEvent -> sequence
          << " as sequence field.\n" << logofs_flush;
  #endif

  unsigned int diffSeq = sendEvent -> sequence -
                             clientCache -> sendEventLastSequence;

  clientCache -> sendEventLastSequence = sendEvent -> sequence;

  encodeBuffer.encodeValue(diffSeq, 16, 4);

  cachedSendEvent -> sequence = sendEvent -> sequence;

  #ifdef TEST
  *logofs << name() << ": Encoding value " << sendEvent -> int_data
          << " as int_data field.\n" << logofs_flush;
  #endif

  encodeBuffer.encodeCachedValue(sendEvent -> int_data, 32,
                     clientCache -> sendEventIntDataCache);

  cachedSendEvent -> int_data = sendEvent -> int_data;
}

void SendEventStore::updateIdentity(DecodeBuffer &decodeBuffer, const Message *message,
                                        ChannelCache *channelCache) const
{
  SendEventMessage *sendEvent = (SendEventMessage *) message;

  ClientCache *clientCache = (ClientCache *) channelCache;

  unsigned int  value;

  decodeBuffer.decodeBoolValue(value);

  sendEvent -> propagate = value;

  #ifdef DEBUG
  *logofs << name() << ": Decoded value " << (unsigned int) sendEvent -> propagate
          << " as propagate field.\n" << logofs_flush;
  #endif

  decodeBuffer.decodeBoolValue(value);

  if (value)
  {
    decodeBuffer.decodeBoolValue(value);
  }
  else
  {
    decodeBuffer.decodeXidValue(value, clientCache -> windowCache);
  }

  sendEvent -> window = value;

  #ifdef DEBUG
  *logofs << name() << ": Decoded value " << sendEvent -> window
          << " as window field.\n" << logofs_flush;
  #endif

  decodeBuffer.decodeCachedValue(sendEvent -> mask, 32,
                     clientCache -> sendEventMaskCache);

  #ifdef DEBUG
  *logofs << name() << ": Decoded value " << sendEvent -> mask
          << " as mask field.\n" << logofs_flush;
  #endif

  decodeBuffer.decodeCachedValue(sendEvent -> code, 8,
                     clientCache -> sendEventCodeCache);

  #ifdef DEBUG
  *logofs << name() << ": Decoded value " << sendEvent -> code
          << " as code field.\n" << logofs_flush;
  #endif

  decodeBuffer.decodeCachedValue(sendEvent -> byte_data, 8,
                     clientCache -> sendEventByteDataCache);

  #ifdef DEBUG
  *logofs << name() << ": Decoded value " << sendEvent -> byte_data
          << " as byte_data field.\n" << logofs_flush;
  #endif

  decodeBuffer.decodeValue(value, 16, 4);

  clientCache -> sendEventLastSequence += value;
  clientCache -> sendEventLastSequence &= 0xffff;

  sendEvent -> sequence = clientCache -> sendEventLastSequence;

  #ifdef DEBUG
  *logofs << name() << ": Decoded value " << sendEvent -> sequence
          << " as sequence field.\n" << logofs_flush;
  #endif

  decodeBuffer.decodeCachedValue(sendEvent -> int_data, 32,
                     clientCache -> sendEventIntDataCache);

  #ifdef DEBUG
  *logofs << name() << ": Decoded value " << sendEvent -> int_data
          << " as int_data field.\n" << logofs_flush;
  #endif
}
