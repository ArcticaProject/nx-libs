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

#ifndef DecodeBuffer_H
#define DecodeBuffer_H

#include <string.h>

#include "IntCache.h"
#include "CharCache.h"
#include "XidCache.h"
#include "FreeCache.h"
#include "OpcodeCache.h"
#include "ActionCache.h"

#include "ActionCacheCompat.h"
#include "PositionCacheCompat.h"

#define DECODE_BUFFER_OVERFLOW_SIZE        4194304

#define DECODE_BUFFER_POSTFIX_SIZE         1

class DecodeBuffer
{
  public:

  DecodeBuffer(const unsigned char *data, unsigned int length);

  ~DecodeBuffer()
  {
  }

  int decodeValue(unsigned int &value, unsigned int numBits,
                      unsigned int blockSize = 0, int endOkay = 0);

  int decodeCachedValue(unsigned int &value, unsigned int numBits,
                            IntCache &cache, unsigned int blockSize = 0,
                                int endOkay = 0);

  int decodeCachedValue(unsigned char &value, unsigned int numBits,
                            CharCache &cache, unsigned int blockSize = 0,
                                int endOkay = 0);

  void decodeDiffCachedValue(unsigned int &value, unsigned int &previous,
                                 unsigned int numBits, IntCache &cache,
                                     unsigned int blockSize = 0)
  {
    decodeCachedValue(value, numBits, cache, blockSize);

    previous += (value + 1);
    previous &= (0xffffffff >> (32 - numBits));

    value = previous;
  }

  void decodeBoolValue(unsigned int &value)
  {
    decodeValue(value, 1);
  }

  int decodeOpcodeValue(unsigned char &value, OpcodeCache &cache, int endOkay = 0)
  {
    int result = decodeCachedValue(value, 8, cache.base_[cache.slot_], 8, endOkay);

    if (result == 1)
    {
      cache.slot_ = value;
    }

    return result;
  }

  void decodeActionValue(unsigned char &value, unsigned short &position,
                             ActionCache &cache);

  void decodeNewXidValue(unsigned int &value, unsigned int &lastId,
                             IntCache &lastIdCache, IntCache &cache,
                                 FreeCache &freeCache);

  void decodeNewXidValue(unsigned int &value, unsigned int &lastId,
                             IntCache &lastIdCache, XidCache &cache,
                                 FreeCache &freeCache);

  void decodeXidValue(unsigned int &value, XidCache &cache);

  void decodeFreeXidValue(unsigned int &value, FreeCache &cache);

  void decodeActionValueCompat(unsigned char &value, ActionCacheCompat &cache)
  {
    decodeCachedValue(value, 2, cache.base_[cache.slot_]);

    cache.slot_ = value;
  }

  void decodePositionValueCompat(short int &value, PositionCacheCompat &cache);

  void decodeTextData(unsigned char *buffer, unsigned int numBytes)
  {
    decodeMemory(buffer, numBytes);
  }

  void decodeIntData(unsigned char *buffer, unsigned int numBytes)
  {
    decodeMemory(buffer, numBytes);
  }

  void decodeLongData(unsigned char *buffer, unsigned int numBytes)
  {
    decodeMemory(buffer, numBytes);
  }

  const unsigned char *decodeMemory(unsigned int numBytes);

  void decodeMemory(unsigned char *buffer, unsigned int numBytes)
  {
    memcpy(buffer, decodeMemory(numBytes), numBytes);
  }

  private:

  const unsigned char *buffer_;
  const unsigned char *end_;
  const unsigned char *nextSrc_;

  unsigned char srcMask_;
};

#endif /* DecodeBuffer_H */
