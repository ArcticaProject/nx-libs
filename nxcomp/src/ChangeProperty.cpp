/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2011 NoMachine (http://www.nomachine.com)          */
/* Copyright (c) 2008-2017 Oleksandr Shneyder <o.shneyder@phoca-gmbh.de>  */
/* Copyright (c) 2014-2022 Ulrich Sibiller <uli42@gmx.de>                 */
/* Copyright (c) 2014-2019 Mihai Moldovan <ionic@ionic.de>                */
/* Copyright (c) 2011-2022 Mike Gabriel <mike.gabriel@das-netzwerkteam.de>*/
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

#include "ChangeProperty.h"

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

int ChangePropertyStore::parseIdentity(Message *message, const unsigned char *buffer,
                                           unsigned int size, int bigEndian) const
{
  ChangePropertyMessage *changeProperty = (ChangePropertyMessage *) message;

  changeProperty -> mode     = *(buffer + 1);
  changeProperty -> format   = *(buffer + 16);

  changeProperty -> window   = GetULONG(buffer + 4, bigEndian);
  changeProperty -> property = GetULONG(buffer + 8, bigEndian);
  changeProperty -> type     = GetULONG(buffer + 12, bigEndian);
  changeProperty -> length   = GetULONG(buffer + 20, bigEndian);

  //
  // Cleanup the padding bytes.
  //

  unsigned int uiFormat;
  unsigned int uiLengthInBytes;

  if ((int) size > CHANGEPROPERTY_DATA_OFFSET)
  {
    uiFormat = *(buffer + 16);

    uiLengthInBytes = changeProperty -> length;

    #ifdef DEBUG
    *logofs << name() << ": length  " << uiLengthInBytes
            << ", format " << uiFormat << ", size "
            << size << ".\n" << logofs_flush;
    #endif

    if (uiFormat == 16)
    {
      uiLengthInBytes <<= 1;
    }
    else if (uiFormat == 32)
    {
      uiLengthInBytes <<= 2;
    }

    unsigned char *end = ((unsigned char *) buffer) + size;
    unsigned char *pad = ((unsigned char *) buffer) + CHANGEPROPERTY_DATA_OFFSET + uiLengthInBytes;

    CleanData((unsigned char *) pad, end - pad);
  }

  #ifdef DEBUG
  *logofs << name() << ": Parsed identity for message at "
          << message << ".\n" << logofs_flush;
  #endif

  return 1;
}

int ChangePropertyStore::unparseIdentity(const Message *message, unsigned char *buffer,
                                             unsigned int size, int bigEndian) const
{
  ChangePropertyMessage *changeProperty = (ChangePropertyMessage *) message;

  *(buffer + 1)  = changeProperty -> mode;
  *(buffer + 16) = changeProperty -> format;

  PutULONG(changeProperty -> window,   buffer + 4,  bigEndian);
  PutULONG(changeProperty -> property, buffer + 8,  bigEndian);
  PutULONG(changeProperty -> type,     buffer + 12, bigEndian);
  PutULONG(changeProperty -> length,   buffer + 20, bigEndian);

  #ifdef DEBUG
  *logofs << name() << ": Unparsed identity for message at "
          << this << ".\n" << logofs_flush;
  #endif

  return 1;
}

void ChangePropertyStore::dumpIdentity(const Message *message) const
{
  #ifdef DUMP

  ChangePropertyMessage *changeProperty = (ChangePropertyMessage *) message;

  *logofs << name() << ": Identity mode " << (unsigned int) changeProperty -> mode << ", format "
          << (unsigned int) changeProperty -> format << ", window " << changeProperty -> window
          << ", property " << changeProperty -> property << ", type " << changeProperty -> type
          << ", length " << changeProperty -> length << ", size " << changeProperty -> size_
          << ".\n" << logofs_flush;

  #endif
}

void ChangePropertyStore::identityChecksum(const Message *message, const unsigned char *buffer,
                                               unsigned int size, int bigEndian) const
{
  md5_append(md5_state_, buffer + 1,  1);
  md5_append(md5_state_, buffer + 16, 1);

  md5_append(md5_state_, buffer + 8,  4);
  md5_append(md5_state_, buffer + 12, 4);
  md5_append(md5_state_, buffer + 20, 4);
}

void ChangePropertyStore::updateIdentity(EncodeBuffer &encodeBuffer, const Message *message,
                                             const Message *cachedMessage,
                                                 ChannelCache *channelCache) const
{
  ChangePropertyMessage *changeProperty       = (ChangePropertyMessage *) message;
  ChangePropertyMessage *cachedChangeProperty = (ChangePropertyMessage *) cachedMessage;

  ClientCache *clientCache = (ClientCache *) channelCache;

  #ifdef TEST
  *logofs << name() << ": Encoding value " << changeProperty -> window
          << " as window field.\n" << logofs_flush;
  #endif

  encodeBuffer.encodeXidValue(changeProperty -> window, clientCache -> windowCache);

  cachedChangeProperty -> window = changeProperty -> window;
}

void ChangePropertyStore::updateIdentity(DecodeBuffer &decodeBuffer, const Message *message,
                                             ChannelCache *channelCache) const
{
  ChangePropertyMessage *changeProperty = (ChangePropertyMessage *) message;

  ClientCache *clientCache = (ClientCache *) channelCache;

  unsigned int value;

  decodeBuffer.decodeXidValue(value, clientCache -> windowCache);

  changeProperty -> window = value;

  #ifdef DEBUG
  *logofs << name() << ": Decoded value " << changeProperty -> window
          << " as window field.\n" << logofs_flush;
  #endif
}


