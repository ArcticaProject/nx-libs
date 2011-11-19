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

#include "CharCache.h"

int CharCache::lookup(unsigned char value, unsigned int &index)
{
  for (unsigned int i = 0; i < length_; i++)
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
  insert(value);
  return 0;
}

void CharCache::insert(unsigned char value)
{
  unsigned int insertionPoint = 0;
  if (2 >= length_)
    insertionPoint = length_;
  else
    insertionPoint = 2;
  unsigned int start;
  if (length_ >= 7)
    start = 7 - 1;
  else
  {
    start = length_;
    length_++;
  }
  for (unsigned int k = start; k > insertionPoint; k--)
    buffer_[k] = buffer_[k - 1];
  buffer_[insertionPoint] = value;
}
