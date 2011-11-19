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
#include "BlockCache.h"


int BlockCache::compare(unsigned int size, const unsigned char *data,
                            int overwrite)
{
  int match = 0;
  if (size == size_)
  {
    match = 1;
    for (unsigned int i = 0; i < size_; i++)
      if (data[i] != buffer_[i])
      {
        match = 0;
        break;
      }
  }
  if (!match && overwrite)
    set(size, data);
  return match;
}


void BlockCache::set(unsigned int size, const unsigned char *data)
{
  if (size_ < size)
  {
    delete[]buffer_;
    buffer_ = new unsigned char[size];
  }
  size_ = size;
  memcpy(buffer_, data, size);
  checksum_ = checksum(size, data);
}


unsigned int BlockCache::checksum(unsigned int size, const unsigned char *data)
{
  unsigned int sum = 0;
  unsigned int shift = 0;
  const unsigned char *next = data;
  for (unsigned int i = 0; i < size; i++)
  {
    unsigned int value = (unsigned int) *next++;
    sum += (value << shift);
    shift++;
    if (shift == 8)
      shift = 0;
  }
  return sum;
}
