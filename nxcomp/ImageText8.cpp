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

#include "ImageText8.h"

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

int ImageText8Store::parseIdentity(Message *message, const unsigned char *buffer,
                                       unsigned int size, int bigEndian) const
{
  ImageText8Message *imageText8 = (ImageText8Message *) message;

  //
  // Here is the fingerprint.
  //

  imageText8 -> len = *(buffer + 1); 

  imageText8 -> drawable = GetULONG(buffer + 4, bigEndian); 
  imageText8 -> gcontext = GetULONG(buffer + 8, bigEndian);

  imageText8 -> x  = GetUINT(buffer + 12, bigEndian);
  imageText8 -> y  = GetUINT(buffer + 14, bigEndian);

  if ((int) size > dataOffset)
  {
    int pad = (size - dataOffset) - imageText8 -> len;
 
    if (pad > 0)
    {
      CleanData((unsigned char *) buffer + size - pad, pad);
    }
  }

  #ifdef DEBUG
  *logofs << name() << ": Parsed Identity for message at " << this << ".\n" << logofs_flush;
  #endif

  return 1;
}

int ImageText8Store::unparseIdentity(const Message *message, unsigned char *buffer,
                                         unsigned int size, int bigEndian) const
{
  ImageText8Message *imageText8 = (ImageText8Message *) message;

  //
  // Fill all the message's fields.
  //

  *(buffer + 1) = imageText8 -> len;

  PutULONG(imageText8 -> drawable, buffer + 4, bigEndian);
  PutULONG(imageText8 -> gcontext, buffer + 8, bigEndian);

  PutUINT(imageText8 -> x, buffer + 12, bigEndian);
  PutUINT(imageText8 -> y, buffer + 14, bigEndian);

  #ifdef DEBUG
  *logofs << name() << ": Unparsed identity for message at " << this << ".\n" << logofs_flush;
  #endif

  return 1;
}

void ImageText8Store::dumpIdentity(const Message *message) const
{
  #ifdef DUMP

  ImageText8Message *imageText8 = (ImageText8Message *) message;

  *logofs << name() << ": Identity len " << (unsigned int) imageText8 -> len
          << " drawable " << imageText8 -> drawable << ", gcontext " 
          << imageText8 -> gcontext << ", x " << imageText8 -> x << ", y " 
          << imageText8 -> y << ", size " << imageText8 -> size_ << ".\n";

  #endif
}

void ImageText8Store::identityChecksum(const Message *message, const unsigned char *buffer,
                                           unsigned int size, int bigEndian) const
{
  md5_append(md5_state_, buffer + 1,  1);
}

void ImageText8Store::updateIdentity(EncodeBuffer &encodeBuffer, const Message *message,
                                         const Message *cachedMessage,
                                             ChannelCache *channelCache) const
{
  ImageText8Message *imageText8       = (ImageText8Message *) message;
  ImageText8Message *cachedImageText8 = (ImageText8Message *) cachedMessage;

  ClientCache *clientCache = (ClientCache *) channelCache;

  #ifdef TEST
  *logofs << name() << ": Encoding value " << imageText8 -> drawable
          << " as " << "drawable" << " field.\n" << logofs_flush;
  #endif

  encodeBuffer.encodeXidValue(imageText8 -> drawable, clientCache -> drawableCache);

  cachedImageText8 -> drawable = imageText8 -> drawable;

  #ifdef TEST
  *logofs << name() << ": Encoding value " << imageText8 -> gcontext
          << " as " << "gcontext" << " field.\n" << logofs_flush;
  #endif

  encodeBuffer.encodeXidValue(imageText8 -> gcontext, clientCache -> gcCache);

  cachedImageText8 -> gcontext = imageText8 -> gcontext;

  #ifdef TEST
  *logofs << name() << ": Encoding value " << imageText8 -> x
          << " as " << "x" << " field.\n" << logofs_flush;
  #endif

  unsigned short int diff_x = imageText8 -> x - cachedImageText8 -> x;

  encodeBuffer.encodeCachedValue(diff_x, 16,
                     clientCache -> imageTextCacheX);

  cachedImageText8 -> x = imageText8 -> x;

  #ifdef TEST
  *logofs << name() << ": Encoding value " << imageText8 -> y
          << " as " << "y" << " field.\n" << logofs_flush;
  #endif

  unsigned short int diff_y = imageText8 -> y - cachedImageText8 -> y;

  encodeBuffer.encodeCachedValue(diff_y, 16,
                     clientCache -> imageTextCacheY);

  cachedImageText8 -> y = imageText8 -> y;
}

void ImageText8Store::updateIdentity(DecodeBuffer &decodeBuffer, const Message *message,
                                           ChannelCache *channelCache) const
{
  ImageText8Message *imageText8 = (ImageText8Message *) message;

  ClientCache *clientCache = (ClientCache *) channelCache;

  unsigned int value;

  decodeBuffer.decodeXidValue(value, clientCache -> drawableCache);

  imageText8 -> drawable = value;

  #ifdef DEBUG
  *logofs << name() << ": Decoded value " << imageText8 -> drawable
          << " as " << "drawable" << " field.\n" << logofs_flush;
  #endif

  decodeBuffer.decodeXidValue(value, clientCache -> gcCache);

  imageText8 -> gcontext = value;

  #ifdef DEBUG
  *logofs << name() << ": Decoded value " << imageText8 -> gcontext
          << " as gcontext field.\n" << logofs_flush;
  #endif

  decodeBuffer.decodeCachedValue(value, 16,
               clientCache -> imageTextCacheX);

  imageText8 -> x += value;
  imageText8 -> x &= 0xffff;

  #ifdef DEBUG
  *logofs << name() << ": Decoded value " << imageText8 -> x
          << " as x field.\n" << logofs_flush;
  #endif

  decodeBuffer.decodeCachedValue(value, 16,
               clientCache -> imageTextCacheY);

  imageText8 -> y += value;
  imageText8 -> y &= 0xffff;

  #ifdef DEBUG
  *logofs << name() << ": Decoded value " << imageText8 -> y
          << " as y field.\n" << logofs_flush;
  #endif
}


