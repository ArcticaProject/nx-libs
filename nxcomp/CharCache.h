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

#ifndef CharCache_H
#define CharCache_H

//
// CharCache is a counterpart of IntCache that is
// optimized for use in compressing text composed
// of 8-bit characters.
//

class CharCache
{
  public:

  CharCache() : length_(0)
  {
  }

  ~CharCache()
  {
  }

  unsigned int getSize() const
  {
    return (unsigned int) length_;
  }

  int lookup(unsigned char value, unsigned int &index);

  //
  // This can be inlined as it is only
  // called by decodeCachedValue().
  //

  unsigned int get(unsigned int index)
  {
    unsigned char result = buffer_[index];

    if (index != 0)
    {
      unsigned int i = index;
      unsigned int target = (i >> 1);

      do
      {
        buffer_[i] = buffer_[i - 1];

        i--;
      }
      while (i > target);

      buffer_[target] = result;
    }

    return (unsigned int) result;
  }

  void insert(unsigned char value);

  private:

  unsigned char length_;

  unsigned char buffer_[7];
};

#endif /* CharCache_H */
