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

#include "ImageText16.h"

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

int ImageText16Store::parseIdentity(Message *message, const unsigned char *buffer,
                                       unsigned int size, int bigEndian) const
{
  ImageText16Message *imageText16 = (ImageText16Message *) message;

  //
  // Here is the fingerprint.
  //

  imageText16 -> len = *(buffer + 1); 

  imageText16 -> drawable = GetULONG(buffer + 4, bigEndian); 
  imageText16 -> gcontext = GetULONG(buffer + 8, bigEndian);

  imageText16 -> x  = GetUINT(buffer + 12, bigEndian);
  imageText16 -> y  = GetUINT(buffer + 14, bigEndian);

  if ((int) size > dataOffset)
  {
    int pad = (size - dataOffset) - (imageText16 -> len * 2);
 
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

int ImageText16Store::unparseIdentity(const Message *message, unsigned char *buffer,
                                         unsigned int size, int bigEndian) const
{
  ImageText16Message *imageText16 = (ImageText16Message *) message;

  //
  // Fill all the message's fields.
  //

  *(buffer + 1) = imageText16 -> len;

  PutULONG(imageText16 -> drawable, buffer + 4, bigEndian);
  PutULONG(imageText16 -> gcontext, buffer + 8, bigEndian);

  PutUINT(imageText16 -> x, buffer + 12, bigEndian);
  PutUINT(imageText16 -> y, buffer + 14, bigEndian);

  #ifdef DEBUG
  *logofs << name() << ": Unparsed identity for message at " << this << ".\n" << logofs_flush;
  #endif

  return 1;
}

void ImageText16Store::dumpIdentity(const Message *message) const
{
  #ifdef DUMP

  ImageText16Message *imageText16 = (ImageText16Message *) message;

  *logofs << name() << ": Identity len " << (unsigned int) imageText16 -> len
          << " drawable " << imageText16 -> drawable << ", gcontext " 
          << imageText16 -> gcontext << ", x " << imageText16 -> x << ", y " 
          << imageText16 -> y << ", size " << imageText16 -> size_ << ".\n";

  #endif
}

void ImageText16Store::identityChecksum(const Message *message, const unsigned char *buffer,
                                           unsigned int size, int bigEndian) const
{
  md5_append(md5_state_, buffer + 1,  1);
}

void ImageText16Store::updateIdentity(EncodeBuffer &encodeBuffer, const Message *message,
                                         const Message *cachedMessage,
                                             ChannelCache *channelCache) const
{
  ImageText16Message *imageText16       = (ImageText16Message *) message;
  ImageText16Message *cachedImageText16 = (ImageText16Message *) cachedMessage;

  ClientCache *clientCache = (ClientCache *) channelCache;

  #ifdef TEST
  *logofs << name() << ": Encoding value " << imageText16 -> drawable
          << " as " << "drawable" << " field.\n" << logofs_flush;
  #endif

  encodeBuffer.encodeXidValue(imageText16 -> drawable, clientCache -> drawableCache);

  cachedImageText16 -> drawable = imageText16 -> drawable;

  #ifdef TEST
  *logofs << name() << ": Encoding value " << imageText16 -> gcontext
          << " as " << "gcontext" << " field.\n" << logofs_flush;
  #endif

  encodeBuffer.encodeXidValue(imageText16 -> gcontext, clientCache -> gcCache);

  cachedImageText16 -> gcontext = imageText16 -> gcontext;

  #ifdef TEST
  *logofs << name() << ": Encoding value " << imageText16 -> x
          << " as " << "x" << " field.\n" << logofs_flush;
  #endif

  unsigned short int diff_x = imageText16 -> x - cachedImageText16 -> x;

  encodeBuffer.encodeCachedValue(diff_x, 16,
                     clientCache -> imageTextCacheX);

  cachedImageText16 -> x = imageText16 -> x;

  #ifdef TEST
  *logofs << name() << ": Encoding value " << imageText16 -> y
          << " as " << "y" << " field.\n" << logofs_flush;
  #endif

  unsigned short int diff_y = imageText16 -> y - cachedImageText16 -> y;

  encodeBuffer.encodeCachedValue(diff_y, 16,
                     clientCache -> imageTextCacheY);

  cachedImageText16 -> y = imageText16 -> y;
}

void ImageText16Store::updateIdentity(DecodeBuffer &decodeBuffer, const Message *message,
                                           ChannelCache *channelCache) const
{
  ImageText16Message *imageText16 = (ImageText16Message *) message;

  ClientCache *clientCache = (ClientCache *) channelCache;

  unsigned int value;

  decodeBuffer.decodeXidValue(value, clientCache -> drawableCache);

  imageText16 -> drawable = value;

  #ifdef DEBUG
  *logofs << name() << ": Decoded value " << imageText16 -> drawable
          << " as " << "drawable" << " field.\n" << logofs_flush;
  #endif

  decodeBuffer.decodeXidValue(value, clientCache -> gcCache);

  imageText16 -> gcontext = value;

  #ifdef DEBUG
  *logofs << name() << ": Decoded value " << imageText16 -> gcontext
          << " as gcontext field.\n" << logofs_flush;
  #endif

  decodeBuffer.decodeCachedValue(value, 16,
               clientCache -> imageTextCacheX);

  imageText16 -> x += value;
  imageText16 -> x &= 0xffff;

  #ifdef DEBUG
  *logofs << name() << ": Decoded value " << imageText16 -> x
          << " as x field.\n" << logofs_flush;
  #endif

  decodeBuffer.decodeCachedValue(value, 16,
               clientCache -> imageTextCacheY);

  imageText16 -> y += value;
  imageText16 -> y &= 0xffff;

  #ifdef DEBUG
  *logofs << name() << ": Decoded value " << imageText16 -> y
          << " as y field.\n" << logofs_flush;
  #endif
}


