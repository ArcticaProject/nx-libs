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

#include "FillPoly.h"

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

int FillPolyStore::parseIdentity(Message *message, const unsigned char *buffer,
                                     unsigned int size, int bigEndian) const
{
  FillPolyMessage *fillPoly = (FillPolyMessage *) message;

  //
  // Here is the fingerprint.
  //

  fillPoly -> drawable = GetULONG(buffer + 4, bigEndian);
  fillPoly -> gcontext = GetULONG(buffer + 8, bigEndian);

  fillPoly -> shape = *(buffer + 12);
  fillPoly -> mode  = *(buffer + 13);

  // Since ProtoStep8 (#issue 108)
  if (size >= (unsigned int) dataOffset)
  {
    fillPoly -> x_origin = GetUINT(buffer + 16, bigEndian);
    fillPoly -> y_origin = GetUINT(buffer + 18, bigEndian);
  }
  else
  {
    fillPoly -> x_origin = 0;
    fillPoly -> y_origin = 0;
  }

  #ifdef DEBUG
  *logofs << name() << ": Parsed Identity for message at " << this << ".\n" << logofs_flush;
  #endif

  return 1;
}

int FillPolyStore::unparseIdentity(const Message *message, unsigned char *buffer,
                                       unsigned int size, int bigEndian) const
{
  FillPolyMessage *fillPoly = (FillPolyMessage *) message;

  //
  // Fill all the message's fields.
  //

  PutULONG(fillPoly -> drawable, buffer + 4, bigEndian);
  PutULONG(fillPoly -> gcontext, buffer + 8, bigEndian);

  *(buffer + 12) = fillPoly -> shape;
  *(buffer + 13) = fillPoly -> mode;

  // Since ProtoStep8 (#issue 108)
  if (size >= (unsigned int) dataOffset)
  {
    PutUINT(fillPoly -> x_origin, buffer + 16, bigEndian);
    PutUINT(fillPoly -> y_origin, buffer + 18, bigEndian);
  }

  #ifdef DEBUG
  *logofs << name() << ": Unparsed identity for message at " << this << ".\n" << logofs_flush;
  #endif

  return 1;
}

void FillPolyStore::dumpIdentity(const Message *message) const
{
  #ifdef DUMP

  FillPolyMessage *fillPoly = (FillPolyMessage *) message;

  *logofs << name() << ": Identity drawable " << fillPoly -> drawable
                    << ", gcontext " << fillPoly -> gcontext << ", shape "
                    << fillPoly -> shape << ", mode " << fillPoly -> mode
                    << fillPoly -> size_ << ", x_origin " << fillPoly -> x_origin
                    << ", y_origin " << fillPoly -> y_origin << ".\n"
                    << logofs_flush;
  #endif
}

void FillPolyStore::identityChecksum(const Message *message, const unsigned char *buffer,
                                         unsigned int size, int bigEndian) const
{
  //
  // Fields shape, mode.
  //

  md5_append(md5_state_, buffer + 12, 2);
}

void FillPolyStore::updateIdentity(EncodeBuffer &encodeBuffer, const Message *message,
                                       const Message *cachedMessage,
                                           ChannelCache *channelCache) const
{
  FillPolyMessage *fillPoly       = (FillPolyMessage *) message;
  FillPolyMessage *cachedFillPoly = (FillPolyMessage *) cachedMessage;

  ClientCache *clientCache = (ClientCache *) channelCache;

  #ifdef TEST
  *logofs << name() << ": Encoding value " << fillPoly -> drawable
          << " as drawable field.\n" << logofs_flush;
  #endif

  encodeBuffer.encodeXidValue(fillPoly -> drawable, clientCache -> drawableCache);

  cachedFillPoly -> drawable = fillPoly -> drawable;

  #ifdef TEST
  *logofs << name() << ": Encoding value " << fillPoly -> gcontext
          << " as gcontext field.\n" << logofs_flush;
  #endif

  encodeBuffer.encodeXidValue(fillPoly -> gcontext, clientCache -> gcCache);

  cachedFillPoly -> gcontext = fillPoly -> gcontext;

  // Since ProtoStep8 (#issue 108)
  if (fillPoly -> size_ >= dataOffset)
  {
    #ifdef TEST
    *logofs << name() << ": Encoding value " << fillPoly -> x_origin
        << " as x_origin field.\n" << logofs_flush;
    #endif

    encodeBuffer.encodeCachedValue(fillPoly -> x_origin, 16,
                 *clientCache -> fillPolyXAbsCache[0], 8);

    cachedFillPoly -> x_origin = fillPoly -> x_origin;

    #ifdef TEST
    *logofs << name() << ": Encoding value " << fillPoly -> y_origin
            << " as y_origin field.\n" << logofs_flush;
    #endif

    encodeBuffer.encodeCachedValue(fillPoly -> y_origin, 16,
                 *clientCache -> fillPolyYAbsCache[0], 8);

    cachedFillPoly -> y_origin = fillPoly -> y_origin;
  }
}

void FillPolyStore::updateIdentity(DecodeBuffer &decodeBuffer, const Message *message,
                                       ChannelCache *channelCache) const
{
  FillPolyMessage *fillPoly = (FillPolyMessage *) message;

  ClientCache *clientCache = (ClientCache *) channelCache;

  decodeBuffer.decodeXidValue(fillPoly -> drawable, clientCache -> drawableCache);

  #ifdef TEST
  *logofs << name() << ": Decoded value " << fillPoly -> drawable
          << " as drawable field.\n" << logofs_flush;
  #endif

  decodeBuffer.decodeXidValue(fillPoly -> gcontext, clientCache -> gcCache);

  #ifdef TEST
  *logofs << name() << ": Decoded value " << fillPoly -> gcontext
          << " as gcontext field.\n" << logofs_flush;
  #endif

  // Since ProtoStep8 (#issue 108)
  if (fillPoly -> size_ >= dataOffset)
  {
    unsigned int value;

    decodeBuffer.decodeCachedValue(value, 16,
                 *clientCache -> fillPolyXAbsCache[0], 8);

    fillPoly -> x_origin = value;

    #ifdef TEST
    *logofs << name() << ": Decoded value " << fillPoly -> x_origin
            << " as x_origin field.\n" << logofs_flush;
    #endif

    decodeBuffer.decodeCachedValue(value, 16,
                 *clientCache -> fillPolyYAbsCache[0], 8);

    fillPoly -> y_origin = value;

    #ifdef TEST
    *logofs << name() << ": Decoded value " << fillPoly -> y_origin
            << " as y_origin field.\n" << logofs_flush;
    #endif
  }
}


