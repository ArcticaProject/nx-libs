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

#include "Misc.h"
#include "Control.h"

#include "DecodeBuffer.h"

//
// Set the verbosity level.
//

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG
#undef  DUMP

DecodeBuffer::DecodeBuffer(const unsigned char *data, unsigned int length)

  : buffer_(data), end_(buffer_ + length), nextSrc_(buffer_), srcMask_(0x80)
{
  if (control -> isProtoStep7() == 1)
  {
    end_ = buffer_ + length - DECODE_BUFFER_POSTFIX_SIZE;
  }
}

int DecodeBuffer::decodeValue(unsigned int &value, unsigned int numBits,
                                  unsigned int blockSize, int endOkay)
{
  #ifdef DUMP
  *logofs << "DecodeBuffer: Decoding " << numBits
          << " bits value with block " << blockSize
          << " and " << (nextSrc_ - buffer_)
          << " bytes in buffer.\n" << logofs_flush;
  #endif

  unsigned int result = 0;
  unsigned int destMask = 0x1;
  unsigned int bitsRead = 0;

  if (blockSize == 0)
    blockSize = numBits;

  unsigned char nextSrcChar = *nextSrc_;
  unsigned int numBlocks = 1;

  do
  {
    if (numBlocks == 4)
    {
      blockSize = numBits;
    }

    unsigned int bitsToRead = (blockSize > numBits - bitsRead ?
                                   numBits - bitsRead : blockSize);
    unsigned int count = 0;
    unsigned char lastBit;

    do
    {
      if (nextSrc_ >= end_)
      {
        if (!endOkay)
        {
          #ifdef PANIC
          *logofs << "DecodeBuffer: PANIC! Assertion failed. Error [A] "
                  << "in decodeValue() nextSrc_ = " << (nextSrc_ - buffer_)
                  << " end_ = " << (end_ - buffer_) << ".\n"
                  << logofs_flush;
          #endif

          //
          // Label "context" is just used to identify
          // the routine which detected the problem in
          // present source file.
          //

          cerr << "Error" << ": Failure decoding data in context [A].\n";

          HandleAbort();
        }

        #ifdef PANIC
        *logofs << "DecodeBuffer: PANIC! Assertion failed. Error [B] "
                << "in decodeValue() nextSrc_ = " << (nextSrc_ - buffer_)
                << " end_ = " << (end_ - buffer_) << ".\n"
                << logofs_flush;
        #endif

        cerr << "Error" << ": Failure decoding data in context [B].\n";

        HandleAbort();
      }

      lastBit = (nextSrcChar & srcMask_);

      if (lastBit)
        result |= destMask;

      srcMask_ >>= 1;

      if (srcMask_ == 0)
      {
        srcMask_ = 0x80;
        nextSrc_++;
        nextSrcChar = *nextSrc_;
      }

      destMask <<= 1;
    }
    while (bitsToRead > ++count);

    bitsRead += bitsToRead;

    if (bitsRead < numBits)
    {
      if (nextSrc_ >= end_)
      {
        if (!endOkay)
        {
          #ifdef PANIC
          *logofs << "DecodeBuffer: PANIC! Assertion failed. Error [C] "
                  << "in decodeValue() nextSrc_ = " << (nextSrc_ - buffer_)
                  << " end_ = " << (end_ - buffer_) << ".\n"
                  << logofs_flush;
          #endif

          cerr << "Error" << ": Failure decoding data in context [C].\n";

          HandleAbort();
        }

        #ifdef PANIC
        *logofs << "DecodeBuffer: PANIC! Assertion failed. Error [D] "
                << "in decodeValue() nextSrc_ = " << (nextSrc_ - buffer_)
                << " end_ = " << (end_ - buffer_) << ".\n"
                << logofs_flush;
        #endif

        cerr << "Error" << ": Failure decoding data in context [D].\n";

        HandleAbort();
      }

      unsigned char moreData = (nextSrcChar & srcMask_);

      srcMask_ >>= 1;

      if (srcMask_ == 0)
      {
        srcMask_ = 0x80;
        nextSrc_++;
        nextSrcChar = *nextSrc_;
      }

      if (!moreData)
      {
        if (lastBit)
        {
          do
          {
            result |= destMask;
            destMask <<= 1;
          }
          while (numBits > ++bitsRead);
        }
        else
          bitsRead = numBits;
      }
    }

    blockSize >>= 1;

    if (blockSize < 2)
      blockSize = 2;

    numBlocks++;
  }
  while (numBits > bitsRead);

  value = result;

  return 1;
}

int DecodeBuffer::decodeCachedValue(unsigned int &value, unsigned int numBits,
                                        IntCache &cache, unsigned int blockSize,
                                            int endOkay)
{
  #ifdef DUMP
  *logofs << "DecodeBuffer: Decoding " << numBits
          << " bits cached value with block " << blockSize
          << " and " << (nextSrc_ - buffer_)
          << " bytes in buffer.\n" << logofs_flush;
  #endif

  if (nextSrc_ >= end_)
  {
    #ifdef PANIC
    *logofs << "DecodeBuffer: PANIC! Assertion failed. Error [E] "
            << "in decodeValue() nextSrc_ = " << (nextSrc_ - buffer_)
            << " end_ = " << (end_ - buffer_) << ".\n"
            << logofs_flush;
    #endif

    cerr << "Error" << ": Failure decoding data in context [E].\n";

    HandleAbort();
  }

  unsigned int index = 0;
  unsigned char nextSrcChar = *nextSrc_;

  while (!(nextSrcChar & srcMask_))
  {
    index++;
    srcMask_ >>= 1;
    if (srcMask_ == 0)
    {
      srcMask_ = 0x80;
      nextSrc_++;
      if (nextSrc_ >= end_)
      {
        if (!endOkay)
        {
          #ifdef PANIC
          *logofs << "DecodeBuffer: PANIC! Assertion failed. Error [F] "
                  << "in decodeCachedValue() nextSrc_ = "
                  << (nextSrc_ - buffer_) << " end_ = "
                  << (end_ - buffer_) << ".\n" << logofs_flush;
          #endif

          cerr << "Error" << ": Failure decoding data in context [F].\n";

          HandleAbort();
        }

        #ifdef PANIC
        *logofs << "DecodeBuffer: PANIC! Assertion failed. Error [G] "
                << "in decodeValue() nextSrc_ = " << (nextSrc_ - buffer_)
                << " end_ = " << (end_ - buffer_) << ".\n"
                << logofs_flush;
        #endif

        cerr << "Error" << ": Failure decoding data in context [G].\n";

        HandleAbort();
      }

      nextSrcChar = *nextSrc_;
    }
  }

  srcMask_ >>= 1;

  if (srcMask_ == 0)
  {
    srcMask_ = 0x80;
    nextSrc_++;
  }

  if (index == 2)
  {
    if (control -> isProtoStep8() == 1)
    {
      blockSize = cache.getBlockSize(blockSize);

      if (decodeValue(value, numBits, blockSize, endOkay))
      {
        cache.insert(value, IntMask[numBits]);

        return 1;
      }

      #ifdef PANIC
      *logofs << "DecodeBuffer: PANIC! Assertion failed. Error [H] "
              << "in decodeCacheValue() with no value found.\n"
              << logofs_flush;
      #endif

      cerr << "Error" << ": Failure decoding data in context [H].\n";

      HandleAbort();
    }
    else
    {
      unsigned int sameDiff;

      decodeBoolValue(sameDiff);

      if (sameDiff)
      {
        value = cache.getLastDiff(IntMask[numBits]);

        cache.insert(value, IntMask[numBits]);

        return 1;
      }
      else
      {
        blockSize = cache.getBlockSize(blockSize);

        if (decodeValue(value, numBits, blockSize, endOkay))
        {
          cache.insert(value, IntMask[numBits]);

          return 1;
        }

        #ifdef PANIC
        *logofs << "DecodeBuffer: PANIC! Assertion failed. Error [H] "
                << "in decodeCacheValue() with no value found.\n"
                << logofs_flush;
        #endif

        cerr << "Error" << ": Failure decoding data in context [H].\n";

        HandleAbort();
      }
    }
  }
  else
  {
    if (index > 2)
    {
      index--;
    }

    if (index > cache.getSize())
    {
      #ifdef PANIC
      *logofs << "DecodeBuffer: PANIC! Assertion failed. Error [I] "
              << "in decodeCachedValue() index = " << index
              << " cache size = " << cache.getSize() << ".\n"
              << logofs_flush;
      #endif

      cerr << "Error" << ": Failure decoding data in context [I].\n";

      HandleAbort();
    }

    value = cache.get(index);

    return 1;
  }
}

int DecodeBuffer::decodeCachedValue(unsigned char &value, unsigned int numBits,
                                        CharCache &cache, unsigned int blockSize,
                                            int endOkay)
{
  #ifdef DUMP
  *logofs << "DecodeBuffer: Decoding " << numBits
          << " bits char cached value with block " << blockSize
          << " and " << nextSrc_ - buffer_ << " bytes read out of "
          << end_ - buffer_ << ".\n" << logofs_flush;
  #endif

  if (nextSrc_ >= end_)
  {
    #ifdef TEST
    *logofs << "DecodeBuffer: End of buffer reached in context [J] with "
            << nextSrc_ - buffer_ << " bytes read out of "
            << end_ - buffer_ << ".\n" << logofs_flush;
    #endif

    return 0;
  }

  unsigned int index = 0;
  unsigned char nextSrcChar = *nextSrc_;

  while (!(nextSrcChar & srcMask_))
  {
    index++;
    srcMask_ >>= 1;

    if (srcMask_ == 0)
    {
      srcMask_ = 0x80;
      nextSrc_++;

      if (nextSrc_ >= end_)
      {
        if (!endOkay)
        {
          #ifdef PANIC
          *logofs << "DecodeBuffer: PANIC! Assertion failed. Error [K] "
                  << "in decodeCachedValue() nextSrc_ "
                  << (nextSrc_ - buffer_) << " end_ " << (end_ - buffer_)
                  << ".\n" << logofs_flush;
          #endif

          cerr << "Error" << ": Failure decoding data in context [K].\n";

          HandleAbort();
        }

        #ifdef TEST
        *logofs << "DecodeBuffer: End of buffer reached in context [L] with "
                << nextSrc_ - buffer_ << " bytes read out of "
                << end_ - buffer_ << ".\n" << logofs_flush;
        #endif

        return 0;
      }

      nextSrcChar = *nextSrc_;
    }
  }

  srcMask_ >>= 1;

  if (srcMask_ == 0)
  {
    srcMask_ = 0x80;
    nextSrc_++;
  }

  if (index == 2)
  {
    unsigned int temp;

    if (decodeValue(temp, numBits, blockSize, endOkay))
    {
      value = (unsigned char) temp;

      cache.insert(value);
    }
    else
    {
      #ifdef PANIC
      *logofs << "DecodeBuffer: PANIC! Assertion failed. Error [M] "
              << "in decodeValue() with index = 2.\n"
              << logofs_flush;
      #endif

      cerr << "Error" << ": Failure decoding data in context [M].\n";

      HandleAbort();
    }
  }
  else
  {
    if (index > 2)
    {
      index--;
    }

    if (index > cache.getSize())
    {
      #ifdef PANIC
      *logofs << "DecodeBuffer: PANIC! Assertion failed. Error [N] "
              << "in decodeCachedValue() " << "index = " << index
              << " cache size = " << cache.getSize() << ".\n"
              << logofs_flush;
      #endif

      cerr << "Error" << ": Failure decoding data in context [N].\n";

      HandleAbort();
    }

    value = cache.get(index);
  }

  return 1;
}

//
// Simply returns a pointer to the correct spot in
// the internal buffer. If the caller needs this
// data to last beyond the lifetime of the internal
// buffer, it must copy the data in its own memory.
//

const unsigned char *DecodeBuffer::decodeMemory(unsigned int numBytes)
{
  #ifdef DUMP
  *logofs << "DecodeBuffer: Decoding " << numBytes
          << " bytes of memory with " << (nextSrc_ - buffer_)
          << " bytes in buffer.\n" << logofs_flush;
  #endif

  const unsigned char *result;     

  //
  // Force ourselves to a byte boundary.
  // Is up to application to ensure data
  // is word alligned when needed.
  //

  if (srcMask_ != 0x80)
  {
    srcMask_ = 0x80;
    nextSrc_++;
  }

  result = nextSrc_;

  if (numBytes > DECODE_BUFFER_OVERFLOW_SIZE)
  {
    #ifdef PANIC
    *logofs << "DecodeBuffer: PANIC! Can't decode a buffer of "
            << numBytes << " bytes with limit set to "
            << DECODE_BUFFER_OVERFLOW_SIZE << ".\n"
            << logofs_flush;

    *logofs << "DecodeBuffer: PANIC! Assuming failure decoding "
            << "data in context [O].\n" << logofs_flush;
    #endif

    cerr << "Error" << ": Should never decode buffer of size "
         << "greater than " << DECODE_BUFFER_OVERFLOW_SIZE
         << " bytes.\n";

    cerr << "Error" << ": Assuming failure decoding data in "
         << "context [O].\n";

    HandleAbort();
  }
  else if (end_ - nextSrc_ < (int) numBytes)
  {
    #ifdef PANIC
    *logofs << "DecodeBuffer: PANIC! Assertion failed. Error [P] "
            << "in decodeMemory() " << "with length " << numBytes
            << " and " << (end_ - nextSrc_)
            << " bytes remaining.\n" << logofs_flush;
    #endif

    cerr << "Error" << ": Failure decoding data in context [P].\n";

    HandleAbort();
  }

  nextSrc_ += numBytes;

  return result;
}

void DecodeBuffer::decodeActionValue(unsigned char &value, unsigned short &position,
                                         ActionCache &cache)
{
  unsigned int t;

  decodeCachedValue(t, 15, *(cache.base_[cache.slot_]));

  cache.last_ += t;
  cache.last_ &= 0x7fff;

  value = cache.last_ >> 13;

  position = cache.last_ & 0x1fff;

  #ifdef DEBUG
  *logofs << "DecodeBuffer: Decoded value "
          << (unsigned) value << " and position "
          << position << " with base " << cache.slot_
          << ".\n" << logofs_flush;
  #endif

  #ifdef DEBUG
  *logofs << "DecodeBuffer: Action block prediction is "
          << (*(cache.base_[cache.slot_])).getBlockSize(15)
          << ".\n" << logofs_flush;
  #endif

  cache.slot_ = (cache.last_ & 0xff);
}

void DecodeBuffer::decodeNewXidValue(unsigned int &value, unsigned int &lastId,
                                         IntCache &lastIdCache, IntCache &cache,
                                             FreeCache &freeCache)
{
  decodeCachedValue(value, 29, lastIdCache);

  lastId += (value + 1);
  lastId &= 0x1fffffff;

  value = lastId;

  cache.push(value, 0x1fffffff);

  freeCache.push(value, 0x1fffffff);
}

void DecodeBuffer::decodeNewXidValue(unsigned int &value, unsigned int &lastId,
                                         IntCache &lastIdCache, XidCache &cache,
                                             FreeCache &freeCache)
{
  decodeCachedValue(value, 29, lastIdCache);

  #ifdef DEBUG
  *logofs << "DecodeBuffer: Decoded new Xid difference "
          << value << ".\n" << logofs_flush;
  #endif

  lastId += (value + 1);
  lastId &= 0x1fffffff;

  value = lastId;

  unsigned int t = (value - cache.last_);

  cache.last_ = value;

  #ifdef DEBUG
  *logofs << "DecodeBuffer: Decoded new Xid " << value
          << " with base " << cache.slot_ << ".\n"
          << logofs_flush;
  #endif

  cache.slot_ = (value & 0xff);

  cache.base_[cache.slot_] -> push(t, 0x1fffffff);

  freeCache.push(value, 0x1fffffff);
}

void DecodeBuffer::decodeXidValue(unsigned int &value, XidCache &cache)
{
  unsigned int t;

  decodeCachedValue(t, 29, *(cache.base_[cache.slot_]));

  cache.last_ += t;
  cache.last_ &= 0x1fffffff;

  value = cache.last_;

  #ifdef DEBUG
  *logofs << "DecodeBuffer: Decoded Xid " << value
          << " with base " << cache.slot_ << ".\n"
          << logofs_flush;
  #endif

  cache.slot_ = (value & 0xff);

  #ifdef DEBUG
  *logofs << "DecodeBuffer: Xid block prediction is "
          << (*(cache.base_[cache.slot_])).getBlockSize(29)
          << ".\n" << logofs_flush;
  #endif
}

void DecodeBuffer::decodeFreeXidValue(unsigned int &value, FreeCache &cache)
{
  decodeCachedValue(value, 29, cache);
}

void DecodeBuffer::decodePositionValueCompat(short int &value, PositionCacheCompat &cache)
{
  unsigned int t;

  decodeCachedValue(t, 13, *(cache.base_[cache.slot_]));

  cache.last_ += t;
  cache.last_ &= 0x1fff;

  value = cache.last_;

  #ifdef DEBUG
  *logofs << "DecodeBuffer: Decoded position "
          << value << " with base " << cache.slot_
          << ".\n" << logofs_flush;
  #endif

  #ifdef DEBUG
  *logofs << "DecodeBuffer: Position block prediction is "
          << (*(cache.base_[cache.slot_])).getBlockSize(13)
          << ".\n" << logofs_flush;
  #endif

  cache.slot_ = (value & 0x1f);
}
