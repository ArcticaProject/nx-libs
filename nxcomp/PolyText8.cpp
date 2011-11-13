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

#include "PolyText8.h"

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

int PolyText8Store::parseIdentity(Message *message, const unsigned char *buffer,
                                      unsigned int size, int bigEndian) const
{
  PolyText8Message *polyText8 = (PolyText8Message *) message;

  //
  // Here is the fingerprint.
  //

  polyText8 -> drawable = GetULONG(buffer + 4, bigEndian); 
  polyText8 -> gcontext = GetULONG(buffer + 8, bigEndian);

  polyText8 -> x = GetUINT(buffer + 12, bigEndian);
  polyText8 -> y = GetUINT(buffer + 14, bigEndian);

  //
  // Clean up padding bytes.
  //

  #ifdef DUMP

  DumpData(buffer, size);

  *logofs << "\n\n" << logofs_flush;

  #endif

  if ((int) size > dataOffset)
  { 
    int length;
    int current;
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

    current = POLYTEXT8_DATA_OFFSET;
    length  = POLYTEXT8_DATA_OFFSET;

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
        // the 'Length of string' field.
        //
        
        length += (item + delta + 1);

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

int PolyText8Store::unparseIdentity(const Message *message, unsigned char *buffer,
                                        unsigned int size, int bigEndian) const
{
  PolyText8Message *polyText8 = (PolyText8Message *) message;

  //
  // Fill all the message's fields.
  //

  PutULONG(polyText8 -> drawable, buffer + 4, bigEndian);
  PutULONG(polyText8 -> gcontext, buffer + 8, bigEndian);

  PutUINT(polyText8 -> x, buffer + 12, bigEndian);
  PutUINT(polyText8 -> y, buffer + 14, bigEndian);

  #ifdef DEBUG
  *logofs << name() << ": Unparsed identity for message at " << this << ".\n" << logofs_flush;
  #endif

  return 1;
}

void PolyText8Store::dumpIdentity(const Message *message) const
{
  #ifdef DUMP

  PolyText8Message *polyText8 = (PolyText8Message *) message;

  *logofs << name() << ": Identity drawable " << polyText8 -> drawable 
          << ", gcontext " << polyText8 -> gcontext << ", x " << polyText8 -> x 
          << ", y " << polyText8 -> y << ", size " << polyText8 -> size_ 
          << ".\n";

  #endif
}

void PolyText8Store::identityChecksum(const Message *message, const unsigned char *buffer,
                                          unsigned int size, int bigEndian) const
{
}

void PolyText8Store::updateIdentity(EncodeBuffer &encodeBuffer, const Message *message,
                                        const Message *cachedMessage,
                                            ChannelCache *channelCache) const
{
  PolyText8Message *polyText8       = (PolyText8Message *) message;
  PolyText8Message *cachedPolyText8 = (PolyText8Message *) cachedMessage;

  ClientCache *clientCache = (ClientCache *) channelCache;

  #ifdef TEST
  *logofs << name() << ": Encoding value " << polyText8 -> drawable
          << " as " << "drawable" << " field.\n" << logofs_flush;
  #endif

  encodeBuffer.encodeXidValue(polyText8 -> drawable, clientCache -> drawableCache);

  cachedPolyText8 -> drawable = polyText8 -> drawable;

  #ifdef TEST
  *logofs << name() << ": Encoding value " << polyText8 -> gcontext
          << " as " << "gcontext" << " field.\n" << logofs_flush;
  #endif

  encodeBuffer.encodeXidValue(polyText8 -> gcontext, clientCache -> gcCache);

  cachedPolyText8 -> gcontext = polyText8 -> gcontext;

  #ifdef TEST
  *logofs << name() << ": Encoding value " << polyText8 -> x
          << " as " << "x" << " field.\n" << logofs_flush;
  #endif

  unsigned short int diff_x = polyText8 -> x - cachedPolyText8 -> x;

  encodeBuffer.encodeCachedValue(diff_x, 16,
                     clientCache -> polyTextCacheX);

  cachedPolyText8 -> x = polyText8 -> x;

  #ifdef TEST
  *logofs << name() << ": Encoding value " << polyText8 -> y
          << " as " << "y" << " field.\n" << logofs_flush;
  #endif

  unsigned short int diff_y = polyText8 -> y - cachedPolyText8 -> y;

  encodeBuffer.encodeCachedValue(diff_y, 16,
                     clientCache -> polyTextCacheY);

  cachedPolyText8 -> y = polyText8 -> y;
}

void PolyText8Store::updateIdentity(DecodeBuffer &decodeBuffer, const Message *message,
                                        ChannelCache *channelCache) const
{
  PolyText8Message *polyText8 = (PolyText8Message *) message;

  ClientCache *clientCache = (ClientCache *) channelCache;

  unsigned int value;

  decodeBuffer.decodeXidValue(value, clientCache -> drawableCache);

  polyText8 -> drawable = value;

  #ifdef DEBUG
  *logofs << name() << ": Decoded value " << polyText8 -> drawable
          << " as " << "drawable" << " field.\n" << logofs_flush;
  #endif

  decodeBuffer.decodeXidValue(value, clientCache -> gcCache);

  polyText8 -> gcontext = value;

  #ifdef DEBUG
  *logofs << name() << ": Decoded value " << polyText8 -> gcontext
          << " as gcontext field.\n" << logofs_flush;
  #endif

  decodeBuffer.decodeCachedValue(value, 16,
               clientCache -> polyTextCacheX);

  polyText8 -> x += value;
  polyText8 -> x &= 0xffff;

  #ifdef DEBUG
  *logofs << name() << ": Decoded value " << polyText8 -> x
          << " as x field.\n" << logofs_flush;
  #endif

  decodeBuffer.decodeCachedValue(value, 16,
               clientCache -> polyTextCacheY);

  polyText8 -> y += value;
  polyText8 -> y &= 0xffff;

  #ifdef DEBUG
  *logofs << name() << ": Decoded value " << polyText8 -> y
          << " as y field.\n" << logofs_flush;
  #endif
}
