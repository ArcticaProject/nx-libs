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

#include <string.h>

#include "Misc.h"
#include "IntCache.h"

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG
#undef  DUMP

IntCache::IntCache(unsigned int size)

  : size_(size), length_(0), buffer_(new unsigned int[size]),
        lastDiff_(0), lastValueInserted_(0), predictedBlockSize_(0)
{
}

int IntCache::lookup(unsigned int &value, unsigned int &index,
                         unsigned int mask, unsigned int &sameDiff)
{
  for (unsigned int i = 0; i < length_; i++)
  {
    if (value == buffer_[i])
    {
      index = i;
      if (i)
      {
        unsigned int target = (i >> 1);
        do
        {
          buffer_[i] = buffer_[i - 1];
          i--;
        }
        while (i > target);
        buffer_[target] = value;
      }
      return 1;
    }
  }
  unsigned int insertionPoint;
  if (2 >= length_)
    insertionPoint = length_;
  else
    insertionPoint = 2;
  unsigned int start;
  if (length_ >= size_)
    start = size_ - 1;
  else
  {
    start = length_;
    length_++;
  }
  for (unsigned int k = start; k > insertionPoint; k--)
    buffer_[k] = buffer_[k - 1];
  buffer_[insertionPoint] = value;
  unsigned int diff = value - lastValueInserted_;

  lastValueInserted_ = (value & mask);
  value = (diff & mask);
  sameDiff = (value == lastDiff_);
  if (!sameDiff)
  {
    lastDiff_ = value;

    unsigned int lastChangeIndex = 0;
    unsigned int lastBitIsOne = (lastDiff_ & 0x1);
    unsigned int j = 1;
    for (unsigned int nextMask = 0x2; nextMask & mask; nextMask <<= 1)
    {
      unsigned int nextBitIsOne = (lastDiff_ & nextMask);
      if (nextBitIsOne)
      {
        if (!lastBitIsOne)
        {
          lastChangeIndex = j;
          lastBitIsOne = nextBitIsOne;
        }
      }
      else
      {
        if (lastBitIsOne)
        {
          lastChangeIndex = j;
          lastBitIsOne = nextBitIsOne;
        }
      }
      j++;
    }
    predictedBlockSize_ = lastChangeIndex + 1;
    if (predictedBlockSize_ < 2)
      predictedBlockSize_ = 2;
  }
  return 0;
}

void IntCache::insert(unsigned int &value, unsigned int mask)
{
  unsigned int insertionPoint;
  if (2 >= length_)
    insertionPoint = length_;
  else
    insertionPoint = 2;
  unsigned int start;
  if (length_ >= size_)
    start = size_ - 1;
  else
  {
    start = length_;
    length_++;
  }
  for (unsigned int k = start; k > insertionPoint; k--)
    buffer_[k] = buffer_[k - 1];
  if (lastDiff_ != value)
  {
    lastDiff_ = value;
    unsigned int lastChangeIndex = 0;
    unsigned int lastBitIsOne = (lastDiff_ & 0x1);
    unsigned int j = 1;
    for (unsigned int nextMask = 0x2; nextMask & mask; nextMask <<= 1)
    {
      unsigned int nextBitIsOne = (lastDiff_ & nextMask);
      if (nextBitIsOne)
      {
        if (!lastBitIsOne)
        {
          lastChangeIndex = j;
          lastBitIsOne = nextBitIsOne;
        }
      }
      else
      {
        if (lastBitIsOne)
        {
          lastChangeIndex = j;
          lastBitIsOne = nextBitIsOne;
        }
      }
      j++;
    }
    predictedBlockSize_ = lastChangeIndex + 1;
    if (predictedBlockSize_ < 2)
      predictedBlockSize_ = 2;
  }
  lastValueInserted_ += value;
  lastValueInserted_ &= mask;
  buffer_[insertionPoint] = lastValueInserted_;
  value = lastValueInserted_;
}

void IntCache::push(unsigned int &value, unsigned int mask)
{
  //
  // Using a memmove() appears to be slower.
  //
  // memmove((char *) &buffer_[1], (char *) &buffer_[0],
  //             sizeof(unsigned int) * (size_ - 1));
  //
  // if (length_ < size_)
  // {
  //   length_++;
  // }
  //

  unsigned int start;

  if (length_ >= size_)
  {
    start = size_ - 1;
  }
  else
  {
    start = length_;

    length_++;
  }

  for (unsigned int k = start; k > 0; k--)
  {
    buffer_[k] = buffer_[k - 1];
  }

  value &= mask;

  buffer_[0] = value;
}

void IntCache::dump()
{
  #ifdef DUMP

  *logofs << "IntCache: Dumping content of cache at "
          << (void *) this << ":\n" << logofs_flush;

  for (unsigned int i = 0; i < length_; i++)
  {
    *logofs << "IntCache: [" << i << "][" << buffer_[i] << "]\n";
  }

  #endif
}
