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

#include "EncodeBuffer.h"

//
// Set the verbosity level.
//

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG
#undef  DUMP

#define ADVANCE_DEST \
\
if (destShift_ == 0) \
{ \
  destShift_ = 7; nextDest_++; *nextDest_ = 0; \
} \
else \
{ \
  destShift_--; \
}

EncodeBuffer::EncodeBuffer()
{
  size_ = ENCODE_BUFFER_DEFAULT_SIZE;

  buffer_ = new unsigned char[size_ + ENCODE_BUFFER_PREFIX_SIZE +
                                  ENCODE_BUFFER_POSTFIX_SIZE] + ENCODE_BUFFER_PREFIX_SIZE;
  end_ = buffer_ + size_;

  nextDest_  = buffer_;
  *nextDest_ = 0;
  destShift_ = 7;

  lastBits_ = 0;

  initialSize_   = ENCODE_BUFFER_DEFAULT_SIZE;
  thresholdSize_ = ENCODE_BUFFER_DEFAULT_SIZE << 1;
  maximumSize_   = ENCODE_BUFFER_DEFAULT_SIZE << 4;
}

EncodeBuffer::~EncodeBuffer()
{
  delete [] (buffer_ - ENCODE_BUFFER_PREFIX_SIZE);
}

void EncodeBuffer::setSize(unsigned int initialSize, unsigned int thresholdSize,
                               unsigned int maximumSize)
{
  initialSize_   = initialSize;
  thresholdSize_ = thresholdSize;
  maximumSize_   = maximumSize;

  #ifdef TEST
  *logofs << "EncodeBuffer: Set buffer sizes to "
          << initialSize_ << "/" << thresholdSize_
          << "/" << maximumSize_ << ".\n"
          << logofs_flush;
  #endif
}

void EncodeBuffer::fullReset()
{
  if (size_ > initialSize_)
  {
    delete [] (buffer_ - ENCODE_BUFFER_PREFIX_SIZE);

    size_ = initialSize_;

    buffer_ = new unsigned char[size_ + ENCODE_BUFFER_PREFIX_SIZE +
                                    ENCODE_BUFFER_POSTFIX_SIZE] + ENCODE_BUFFER_PREFIX_SIZE;
  }

  end_ = buffer_ + size_;

  nextDest_  = buffer_;
  *nextDest_ = 0;
  destShift_ = 7;

  lastBits_ = 0;
}

void EncodeBuffer::encodeValue(unsigned int value, unsigned int numBits,
                                   unsigned int blockSize)
{
  #ifdef DUMP
  *logofs << "EncodeBuffer: Encoding " << numBits
          << " bits value with block " << blockSize
          << " and " << (nextDest_ - buffer_)
          << " bytes in buffer.\n" << logofs_flush;
  #endif

  value &= IntMask[numBits];

  unsigned int srcMask = 0x1;
  unsigned int bitsWritten = 0;

  if (blockSize == 0)
    blockSize = numBits;

  if (end_ - nextDest_ < 8)
  {
    growBuffer();
  }

  unsigned int numBlocks = 1;

  do
  {
    if (numBlocks == 4)
      blockSize = numBits;

    unsigned int bitsToWrite = (blockSize > numBits - bitsWritten ?
                                    numBits - bitsWritten : blockSize);
    unsigned int count = 0;
    unsigned int lastBit;

    do
    {
      lastBit = (value & srcMask);
      if (lastBit)
        *nextDest_ |= (1 << destShift_);
      ADVANCE_DEST;
      srcMask <<= 1;
    }
    while (bitsToWrite > ++count);

    bitsWritten += bitsToWrite;

    if (bitsWritten < numBits)
    {
      unsigned int tmpMask = srcMask;
      unsigned int i = bitsWritten;

      if (lastBit)
      {
        do
        {
          unsigned int nextBit = (value & tmpMask);

          if (!nextBit)
            break;

          tmpMask <<= 1;
        }
        while (numBits > ++i);
      }
      else
      {
        do
        {
          unsigned int nextBit = (value & tmpMask);

          if (nextBit)
            break;

          tmpMask <<= 1;
        }
        while (numBits > ++i);
      }

      if (i < numBits)
        *nextDest_ |= (1 << destShift_);
      else
        bitsWritten = numBits;

      ADVANCE_DEST;
    }
    blockSize >>= 1;

    if (blockSize < 2)
      blockSize = 2;

    numBlocks++;
  }
  while (numBits > bitsWritten);
}

void EncodeBuffer::encodeCachedValue(unsigned int value, unsigned int numBits,
                                         IntCache &cache, unsigned int blockSize)
{
  #ifdef DUMP
  *logofs << "EncodeBuffer: Encoding " << numBits
          << " bits cached value with block " << blockSize
          << " and " << (nextDest_ - buffer_)
          << " bytes in buffer.\n" << logofs_flush;
  #endif

  value &= IntMask[numBits];

  if (end_ - nextDest_ < 8)
  {
    growBuffer();
  }

  blockSize = cache.getBlockSize(blockSize);

  unsigned int index;
  unsigned int sameDiff;

  #ifdef DUMP

  diffBits();

  #endif

  if (cache.lookup(value, index, IntMask[numBits], sameDiff))
  {
    if (index > 1)
      index++;

    while (destShift_ < index)
    {
      index -= destShift_;
      index--;
      destShift_ = 7;
      nextDest_++;
      *nextDest_ = 0;
    }

    destShift_ -= index;
    *nextDest_ |= (1 << destShift_);
    ADVANCE_DEST;

    #ifdef DUMP
    *logofs << "EncodeBuffer: Encoded cached int using "
            << diffBits() << " bits out of " << numBits
            << ".\n" << logofs_flush;
    #endif
  }
  else
  {
    ADVANCE_DEST;
    ADVANCE_DEST;
    *nextDest_ |= (1 << destShift_);
    ADVANCE_DEST;

    //
    // The attempt is very seldom successful.
    // Avoid to encode the additional bool.
    //

    if (control -> isProtoStep8() == 1)
    {
      #ifdef DUMP
      *logofs << "EncodeBuffer: Encoded missed int using "
              << diffBits() << " bits out of " << numBits
              << ".\n" << logofs_flush;
      #endif

      encodeValue(value, numBits, blockSize);
    }
    else
    {
      if (sameDiff)
      {
        #ifdef DUMP
        *logofs << "EncodeBuffer: Matched difference with block size "
                << cache.getBlockSize(blockSize) << ".\n"
                << logofs_flush;
        #endif

        encodeBoolValue(1);
      }
      else
      {
        #ifdef DUMP
        *logofs << "EncodeBuffer: Missed difference with block size "
                << cache.getBlockSize(blockSize) << ".\n"
                << logofs_flush;
        #endif

        encodeBoolValue(0);

        encodeValue(value, numBits, blockSize);
      }

      #ifdef DUMP
      *logofs << "EncodeBuffer: Encoded missed int using "
              << diffBits() << " bits out of " << numBits
              << ".\n" << logofs_flush;
      #endif
    }
  }
}

void EncodeBuffer::encodeCachedValue(unsigned char value, unsigned int numBits,
                                         CharCache &cache, unsigned int blockSize)
{
  #ifdef DUMP
  *logofs << "EncodeBuffer: Encoding " << numBits
          << " bits char cached value with block " << blockSize
          << " and " << (nextDest_ - buffer_)
          << " bytes in buffer.\n" << logofs_flush;
  #endif

  value &= IntMask[numBits];

  if (end_ - nextDest_ < 8)
  {
    growBuffer();
  }

  unsigned int index;

  #ifdef DUMP

  diffBits();

  #endif

  if (cache.lookup(value, index))
  {
    if (index > 1)
      index++;

    while (destShift_ < index)
    {
      index -= destShift_;
      index--;
      destShift_ = 7;
      nextDest_++;
      *nextDest_ = 0;
    }

    destShift_ -= index;
    *nextDest_ |= (1 << destShift_);
    ADVANCE_DEST;

    #ifdef DUMP
    *logofs << "EncodeBuffer: Encoded cached char using "
            << diffBits() << " bits out of " << numBits
            << ".\n" << logofs_flush;
    #endif
  }
  else
  {
    ADVANCE_DEST;
    ADVANCE_DEST;
    *nextDest_ |= (1 << destShift_);
    ADVANCE_DEST;

    encodeValue(value, numBits, blockSize);

    #ifdef DUMP
    *logofs << "EncodeBuffer: Encoded missed char using "
            << diffBits() << " bits out of " << numBits
            << ".\n" << logofs_flush;
    #endif
  }
}

void EncodeBuffer::encodeMemory(const unsigned char *buffer, unsigned int numBytes)
{
  #ifdef DUMP
  *logofs << "EncodeBuffer: Encoding " << numBytes
          << " bytes of memory with " << (nextDest_ - buffer_)
          << " bytes in buffer.\n" << logofs_flush;
  #endif

  if (numBytes > ENCODE_BUFFER_OVERFLOW_SIZE)
  {
    #ifdef PANIC
    *logofs << "EncodeBuffer: PANIC! Should never encode buffer "
            << "of size greater than " << ENCODE_BUFFER_OVERFLOW_SIZE
            << " bytes.\n" << logofs_flush;

    *logofs << "EncodeBuffer: PANIC! Assuming failure encoding data "
            << "in context [A].\n" << logofs_flush;
    #endif

    //
    // Label "context" is just used to identify
    // the routine which detected the problem in
    // present source file.
    //

    cerr << "Error" << ": Should never encode buffer of size "
         << "greater than " << ENCODE_BUFFER_OVERFLOW_SIZE
         << " bytes.\n";

    cerr << "Error" << ": Assuming failure encoding data "
         << "in context [A].\n" ;

    HandleAbort();
  }

  alignBuffer();

  if (end_ - nextDest_ < (int) numBytes)
  {
    growBuffer(numBytes);
  }

  memcpy(nextDest_, buffer, numBytes);

  nextDest_ += numBytes;

  if (nextDest_ == end_)
  {
    growBuffer();
  }
  else if (nextDest_ > end_)
  {
    #ifdef PANIC
    *logofs << "EncodeBuffer: PANIC! Assertion failed. Error [B] "
            << "in encodeMemory() nextDest_ " << (nextDest_ - buffer)
            << " end_ " << (end_ - buffer) << ".\n"
            << logofs_flush;
    #endif

    //
    // Label "context" is just used to identify
    // the routine which detected the problem in
    // present source file.
    //

    cerr << "Error" << ": Failure encoding raw data "
         << "in context [B].\n" ;

    HandleAbort();
  }

  *nextDest_ = 0; 
}

unsigned int EncodeBuffer::getLength() const
{
  unsigned int length = nextDest_ - buffer_;

  if (destShift_ != 7)
  {
    length++;
  }

  if (length > 0 && control -> isProtoStep7() == 1)
  {
    return length + ENCODE_BUFFER_POSTFIX_SIZE;
  }

  return length;
}

unsigned int EncodeBuffer::diffBits()
{
  unsigned int bits = ((nextDest_ - buffer_) << 3);

  bits += (7 - destShift_);

  unsigned int diff = bits - lastBits_;

  lastBits_ = bits;

  return diff;
}

void EncodeBuffer::growBuffer(unsigned int numBytes)
{
  if (numBytes == 0)
  {
    numBytes = initialSize_;
  }

  unsigned int bytesInBuffer = nextDest_ - buffer_;

  unsigned int newSize = thresholdSize_;

  while (newSize < bytesInBuffer + numBytes)
  {
    newSize <<= 1;

    if (newSize > maximumSize_)
    {
      newSize = bytesInBuffer + numBytes + initialSize_;
    }
  }

  unsigned char *newBuffer;

  newBuffer = new unsigned char[newSize + ENCODE_BUFFER_PREFIX_SIZE +
                                    ENCODE_BUFFER_POSTFIX_SIZE] + ENCODE_BUFFER_PREFIX_SIZE;

  if (newBuffer == NULL)
  {
    #ifdef PANIC
    *logofs << "EncodeBuffer: PANIC! Error in context [C] "
            << "growing buffer to accomodate " << numBytes
            << " bytes .\n" << logofs_flush;
    #endif

    cerr << "Error" << ": Error in context [C] "
         << "growing encode buffer to accomodate "
         << numBytes << " bytes.\n";

    HandleAbort();
  }

  #ifdef TEST
  if (newSize >= maximumSize_)
  {
    *logofs << "EncodeBuffer: WARNING! Buffer grown to reach "
            << "size of " << newSize << " bytes.\n"
            << logofs_flush;
  }
  #endif

  //
  // Prefix should not contain any valid data.
  // It is proxy that will fill it with control
  // messages and data length at the time a new
  // frame is written to socket.
  //

  memcpy(newBuffer, buffer_, bytesInBuffer + 1);

  newBuffer[bytesInBuffer + 1] = 0;

  delete [] (buffer_ - ENCODE_BUFFER_PREFIX_SIZE);

  buffer_ = newBuffer;
  size_   = newSize;
  end_    = buffer_ + size_;

  nextDest_ = buffer_ + bytesInBuffer;
}

void EncodeBuffer::alignBuffer()
{
  if (destShift_ != 7)
  {
    destShift_ = 7;
    nextDest_++;

    if (nextDest_ >= end_)
    {
      growBuffer();
    }
    
    *nextDest_ = 0;
  }
}

void EncodeBuffer::encodeActionValue(unsigned char value, unsigned short position,
                                         ActionCache &cache)
{
  unsigned int v = (value << 13) | position;

  unsigned int t = (v - cache.last_);

  encodeCachedValue(t, 15, *(cache.base_[cache.slot_]));

  cache.last_ = v;

  #ifdef DEBUG
  *logofs << "EncodeBuffer: Encoded value "
          << (unsigned) value << " and position "
          << position << " with base " << cache.slot_
          << ".\n" << logofs_flush;
  #endif

  cache.slot_ = (cache.last_ & 0xff);
}

void EncodeBuffer::encodeNewXidValue(unsigned int value, unsigned int &lastId,
                                         IntCache &lastIdCache, IntCache &cache,
                                             FreeCache &freeCache)
{
  encodeCachedValue((value - 1) - lastId, 29, lastIdCache);

  lastId = value;

  cache.push(value, 0x1fffffff);

  freeCache.push(value, 0x1fffffff);
}

void EncodeBuffer::encodeNewXidValue(unsigned int value, unsigned int &lastId,
                                         IntCache &lastIdCache, XidCache &cache,
                                             FreeCache &freeCache)
{
  encodeCachedValue((value - 1) - lastId, 29, lastIdCache);

  lastId = value;

  unsigned int t = (value - cache.last_);

  cache.last_ = value;

  #ifdef DEBUG
  *logofs << "EncodeBuffer: Encoded new Xid " << value
          << " with base " << cache.slot_ << ".\n"
          << logofs_flush;
  #endif

  cache.slot_ = (value & 0xff);

  cache.base_[cache.slot_] -> push(t, 0x1fffffff);

  freeCache.push(value, IntMask[29]);
}

void EncodeBuffer::encodeXidValue(unsigned int value, XidCache &cache)
{
  unsigned int t = (value - cache.last_);

  encodeCachedValue(t, 29, *(cache.base_[cache.slot_]));

  cache.last_ = value;

  #ifdef DEBUG
  *logofs << "EncodeBuffer: Encoded Xid " << value
          << " with base " << cache.slot_ << ".\n"
          << logofs_flush;
  #endif

  cache.slot_ = (value & 0xff);
}

void EncodeBuffer::encodeFreeXidValue(unsigned int value, FreeCache &cache)
{
  encodeCachedValue(value, 29, cache);
}

void EncodeBuffer::encodePositionValueCompat(short int value, PositionCacheCompat &cache)
{
  unsigned int t = (value - cache.last_);

  encodeCachedValue(t, 13, *(cache.base_[cache.slot_]));

  cache.last_ = value;

  #ifdef DEBUG
  *logofs << "EncodeBuffer: Encoded position "
          << value << " with base " << cache.slot_
          << ".\n" << logofs_flush;
  #endif

  cache.slot_ = (value & 0x1f);
}
