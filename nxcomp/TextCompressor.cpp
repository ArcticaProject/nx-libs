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

#include "TextCompressor.h"

#include "EncodeBuffer.h"
#include "DecodeBuffer.h"

//
// The compression obtained by this class is
// very poor. In newer versions the text is
// simply appended to the encode buffer and
// compressed by leveraging the final stream
// compression.
//

void
TextCompressor::encodeChar(unsigned char ch, EncodeBuffer& encodeBuffer)
{
  // encode each successive character of text using
  // a predictive model where most of the last 3 characters
  // (low order 7 bits of the previous character, plus the
  // low order 5 bits of the character before that, plus
  // the low order 3 bits of the character before that)
  // are used to find the right cache...

  CharCache& cache =  cache_[key_ % cacheSize_];
  if ((key_ >= 128) && (cache.getSize() == 0))
  {
    // 3rd-order model doesn't have any statistics yet,
    // so use the 1st-order one instead
    CharCache& cache2 =  cache_[(key_ & 0x7f) % cacheSize_];
    encodeBuffer.encodeCachedValue((unsigned int) ch, 8, cache2);
    cache.insert(ch);
  }
  else
  {
    encodeBuffer.encodeCachedValue((unsigned int) ch, 8, cache);
  }

  key_ = (((key_ & 0x1f) << 7) | ((key_ & 0x380) << 5) | (ch & 0x7f));
}


unsigned char
TextCompressor::decodeChar(DecodeBuffer& decodeBuffer)
{
  unsigned char nextChar;
  CharCache& cache =  cache_[key_ % cacheSize_];
  if ((key_ >= 128) && (cache.getSize() == 0))
  {
    CharCache& cache2 =  cache_[(key_ & 0x7f) % cacheSize_];
    decodeBuffer.decodeCachedValue(nextChar, 8, cache2);
    cache.insert(nextChar);
  }
  else
  {
    decodeBuffer.decodeCachedValue(nextChar, 8, cache);
  }

  key_ = (((key_ & 0x1f) << 7) | ((key_ & 0x380) << 5) | (nextChar & 0x7f));
  return nextChar;
}
