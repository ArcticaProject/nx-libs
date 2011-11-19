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

#ifndef EncodeBuffer_H
#define EncodeBuffer_H

#include "IntCache.h"
#include "CharCache.h"
#include "XidCache.h"
#include "FreeCache.h"
#include "OpcodeCache.h"
#include "ActionCache.h"

#include "ActionCacheCompat.h"
#include "PositionCacheCompat.h"

#define ENCODE_BUFFER_DEFAULT_SIZE        16384

//
// This should match the maximum size of
// a single message added to write buffer
// (see WriteBuffer.h).
//

#define ENCODE_BUFFER_OVERFLOW_SIZE       4194304

//
// Adjust for the control messages and the
// frame length added by the proxy.
//

#define ENCODE_BUFFER_PREFIX_SIZE         64

//
// The encode routines may write one byte
// past the nominal end of the encode buffer.
// This additional byte is included in the
// payload. This is actually a harmless bug.
//

#define ENCODE_BUFFER_POSTFIX_SIZE        1

class EncodeBuffer
{
  public:

  EncodeBuffer();

  ~EncodeBuffer();

  void setSize(unsigned int initialSize, unsigned int thresholdSize,
                   unsigned int maximumSize);

  void encodeValue(unsigned int value, unsigned int numBits,
                       unsigned int blockSize = 0);

  void encodeCachedValue(unsigned int value, unsigned int numBits,
                             IntCache &cache, unsigned int blockSize = 0);

  void encodeCachedValue(unsigned char value, unsigned int numBits,
                             CharCache &cache, unsigned int blockSize = 0);

  void encodeDiffCachedValue(const unsigned int value, unsigned int &previous,
                                 unsigned int numBits, IntCache &cache,
                                     unsigned int blockSize = 0)
  {
    encodeCachedValue((value - 1) - previous, numBits, cache, blockSize);

    previous = value;
  }

  void encodeBoolValue(unsigned int value)
  {
    encodeValue(value, 1);
  }

  void encodeOpcodeValue(unsigned char value, OpcodeCache &cache)
  {
    encodeCachedValue(value, 8, cache.base_[cache.slot_], 8);

    cache.slot_ = value;
  }

  void encodeActionValue(unsigned char value, ActionCache &cache)
  {
    unsigned short position = 0;

    encodeActionValue(value, position, cache);
  }

  void encodeActionValue(unsigned char value, unsigned short position,
                             ActionCache &cache);

  void encodeNewXidValue(unsigned int value, unsigned int &lastId,
                             IntCache &lastIdCache, IntCache &cache,
                                 FreeCache &freeCache);

  void encodeNewXidValue(unsigned int value, unsigned int &lastId,
                             IntCache &lastIdCache, XidCache &cache,
                                 FreeCache &freeCache);

  void encodeXidValue(unsigned int value, XidCache &cache);

  void encodeFreeXidValue(unsigned int value, FreeCache &cache);

  void encodeActionValueCompat(unsigned char value, ActionCacheCompat &cache)
  {
    encodeCachedValue(value, 2, cache.base_[cache.slot_]);

    cache.slot_ = value;
  }

  void encodePositionValueCompat(short int value, PositionCacheCompat &cache);

  void encodeTextData(const unsigned char *buffer, unsigned int numBytes)
  {
    encodeMemory(buffer, numBytes);
  }

  void encodeIntData(const unsigned char *buffer, unsigned int numBytes)
  {
    encodeMemory(buffer, numBytes);
  }

  void encodeLongData(const unsigned char *buffer, unsigned int numBytes)
  {
    encodeMemory(buffer, numBytes);
  }

  void encodeMemory(const unsigned char *buffer, unsigned int numBytes);

  unsigned char *getData()
  {
    return buffer_;
  }

  unsigned int getLength() const;

  unsigned int getBits() const
  {
    return ((nextDest_ - buffer_) << 3) + (7 - destShift_);
  }

  unsigned int diffBits();

  void fullReset();

  private:

  void growBuffer(unsigned int numBytes = 0);

  void alignBuffer();

  unsigned int size_;
  unsigned char *buffer_;

  //
  // This points to the first byte
  // just beyond end of the buffer.
  //

  const unsigned char *end_;

  unsigned char *nextDest_;
  unsigned int destShift_;
  unsigned int lastBits_;

  unsigned int initialSize_;
  unsigned int thresholdSize_;
  unsigned int maximumSize_;
};

#endif /* EncodeBuffer_H */
