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

#include "PolyText16.h"

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

int PolyText16Store::parseIdentity(Message *message, const unsigned char *buffer,
                                       unsigned int size, int bigEndian) const
{
  PolyText16Message *polyText16 = (PolyText16Message *) message;

  //
  // Here is the fingerprint.
  //

  polyText16 -> drawable = GetULONG(buffer + 4, bigEndian); 
  polyText16 -> gcontext = GetULONG(buffer + 8, bigEndian);

  polyText16 -> x = GetUINT(buffer + 12, bigEndian);
  polyText16 -> y = GetUINT(buffer + 14, bigEndian);

  //
  // Clean up padding bytes.
  //

  #ifdef DUMP

  DumpData(buffer, size);

  *logofs << "\n" << logofs_flush;

  #endif

  if ((int) size > dataOffset)
  {
    int current;
    int length;
    int delta;
    int item;
    
    unsigned int nitem;

    unsigned char *pad = NULL; 
    unsigned char *end = NULL;

    delta = 1;
    nitem = 0;

    #ifdef DUMP
    *logofs << name() << " Size " << size << ".\n" << logofs_flush;
    #endif

    //
    // Data is a list of TextItem where element
    // can be a string or a font shift.
    //

    current = POLYTEXT16_DATA_OFFSET;
    length  = POLYTEXT16_DATA_OFFSET;

    do
    {
      #ifdef DUMP
      *logofs << name() << " Current " << current << ".\n" << logofs_flush;
      #endif

      item = GetUINT(buffer + length , bigEndian);

      if (item < 255)
      {
        //
        // Text element. Number represents
        // the 'Length of CHAR2B string'
        // field.
        //

        length += ((item * 2) + delta + 1);

        nitem++;
      }
      else if (item == 255)
      {
        //
        // Element is a font shift.
        //

        length += 5;

        nitem++;
      }

      #ifdef DUMP
      *logofs << name() << " Item " << item << ".\n" << logofs_flush;
      #endif

      current += length;
    }
    while(current < (int) size && item != 0);

    #ifdef DUMP
    *logofs << name() << " Final length " << length << ".\n" << logofs_flush;
    #endif

    end = ((unsigned char *) buffer) + size;

    pad = ((unsigned char *) buffer) + length;

    for (; pad < end && nitem >= 1; pad++)
    {
      #ifdef DUMP
      *logofs << name() << " Padding " << " .\n" << logofs_flush;
      #endif

      *pad = 0;
    }
  }

  #ifdef DEBUG
  *logofs << name() << ": Parsed Identity for message at " << this << ".\n" << logofs_flush;
  #endif

  return 1;
}

int PolyText16Store::unparseIdentity(const Message *message, unsigned char *buffer,
                                         unsigned int size, int bigEndian) const
{
  PolyText16Message *polyText16 = (PolyText16Message *) message;

  //
  // Fill all the message's fields.
  //

  PutULONG(polyText16 -> drawable, buffer + 4, bigEndian);
  PutULONG(polyText16 -> gcontext, buffer + 8, bigEndian);

  PutUINT(polyText16 -> x, buffer + 12, bigEndian);
  PutUINT(polyText16 -> y, buffer + 14, bigEndian);

  #ifdef DEBUG
  *logofs << name() << ": Unparsed identity for message at " << this << ".\n" << logofs_flush;
  #endif

  return 1;
}

void PolyText16Store::dumpIdentity(const Message *message) const
{
  #ifdef DUMP

  PolyText16Message *polyText16 = (PolyText16Message *) message;

  *logofs << name() << ": Identity drawable " << polyText16 -> drawable 
          << ", gcontext " << polyText16 -> gcontext << ", x " << polyText16 -> x 
          << ", y " << polyText16 -> y << ", size " << polyText16 -> size_ 
          << ".\n";

  #endif
}

void PolyText16Store::identityChecksum(const Message *message, const unsigned char *buffer,
                                           unsigned int size, int bigEndian) const
{
}

void PolyText16Store::updateIdentity(EncodeBuffer &encodeBuffer, const Message *message,
                                         const Message *cachedMessage,
                                             ChannelCache *channelCache) const
{
  PolyText16Message *polyText16       = (PolyText16Message *) message;
  PolyText16Message *cachedPolyText16 = (PolyText16Message *) cachedMessage;

  ClientCache *clientCache = (ClientCache *) channelCache;

  #ifdef TEST
  *logofs << name() << ": Encoding value " << polyText16 -> drawable
          << " as " << "drawable" << " field.\n" << logofs_flush;
  #endif

  encodeBuffer.encodeXidValue(polyText16 -> drawable, clientCache -> drawableCache);

  cachedPolyText16 -> drawable = polyText16 -> drawable;

  #ifdef TEST
  *logofs << name() << ": Encoding value " << polyText16 -> gcontext
          << " as " << "gcontext" << " field.\n" << logofs_flush;
  #endif

  encodeBuffer.encodeXidValue(polyText16 -> gcontext, clientCache -> gcCache);

  cachedPolyText16 -> gcontext = polyText16 -> gcontext;

  #ifdef TEST
  *logofs << name() << ": Encoding value " << polyText16 -> x
          << " as " << "x" << " field.\n" << logofs_flush;
  #endif

  unsigned short int diff_x = polyText16 -> x - cachedPolyText16 -> x;

  encodeBuffer.encodeCachedValue(diff_x, 16,
                     clientCache -> polyTextCacheX);

  cachedPolyText16 -> x = polyText16 -> x;

  #ifdef TEST
  *logofs << name() << ": Encoding value " << polyText16 -> y
          << " as " << "y" << " field.\n" << logofs_flush;
  #endif

  unsigned short int diff_y = polyText16 -> y - cachedPolyText16 -> y;

  encodeBuffer.encodeCachedValue(diff_y, 16,
                     clientCache -> polyTextCacheY);

  cachedPolyText16 -> y = polyText16 -> y;
}

void PolyText16Store::updateIdentity(DecodeBuffer &decodeBuffer, const Message *message,
                                         ChannelCache *channelCache) const
{
  PolyText16Message *polyText16 = (PolyText16Message *) message;

  ClientCache *clientCache = (ClientCache *) channelCache;

  unsigned int value;

  decodeBuffer.decodeXidValue(value, clientCache -> drawableCache);

  polyText16 -> drawable = value;

  #ifdef DEBUG
  *logofs << name() << ": Decoded value " << polyText16 -> drawable
          << " as " << "drawable" << " field.\n" << logofs_flush;
  #endif

  decodeBuffer.decodeXidValue(value, clientCache -> gcCache);

  polyText16 -> gcontext = value;

  #ifdef DEBUG
  *logofs << name() << ": Decoded value " << polyText16 -> gcontext
          << " as gcontext field.\n" << logofs_flush;
  #endif

  decodeBuffer.decodeCachedValue(value, 16,
               clientCache -> polyTextCacheX);

  polyText16 -> x += value;
  polyText16 -> x &= 0xffff;

  #ifdef DEBUG
  *logofs << name() << ": Decoded value " << polyText16 -> x
          << " as x field.\n" << logofs_flush;
  #endif

  decodeBuffer.decodeCachedValue(value, 16,
               clientCache -> polyTextCacheY);

  polyText16 -> y += value;
  polyText16 -> y &= 0xffff;

  #ifdef DEBUG
  *logofs << name() << ": Decoded value " << polyText16 -> y
          << " as y field.\n" << logofs_flush;
  #endif
}


