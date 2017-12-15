/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2011 NoMachine (http://www.nomachine.com)          */
/* Copyright (c) 2008-2014 Oleksandr Shneyder <o.shneyder@phoca-gmbh.de>  */
/* Copyright (c) 2014-2016 Ulrich Sibiller <uli42@gmx.de>                 */
/* Copyright (c) 2014-2016 Mihai Moldovan <ionic@ionic.de>                */
/* Copyright (c) 2011-2016 Mike Gabriel <mike.gabriel@das-netzwerkteam.de>*/
/* Copyright (c) 2015-2016 Qindel Group (http://www.qindel.com)           */
/*                                                                        */
/* NXCOMP, NX protocol compression and NX extensions to this software     */
/* are copyright of the aforementioned persons and companies.             */
/*                                                                        */
/* Redistribution and use of the present software is allowed according    */
/* to terms specified in the file LICENSE.nxcomp which comes in the       */
/* source distribution.                                                   */
/*                                                                        */
/* All rights reserved.                                                   */
/*                                                                        */
/* NOTE: This software has received contributions from various other      */
/* contributors, only the core maintainers and supporters are listed as   */
/* copyright holders. Please contact us, if you feel you should be listed */
/* as copyright holder, as well.                                          */
/*                                                                        */
/**************************************************************************/

#ifndef IntCache_H
#define IntCache_H

class IntCache
{
  public:

  IntCache(unsigned int size);

  ~IntCache()
  {
    delete [] buffer_;
  }

  unsigned int getSize() const
  {
    return length_;
  }

  int lookup(unsigned int &value, unsigned int &index,
                 unsigned int mask, unsigned int &sameDiff);

  //
  // This can be inlined as it is only
  // called by decodeCachedValue().
  //

  unsigned int get(unsigned int index)
  {
    unsigned int result = buffer_[index];

    if (index != 0)
    {
      //
      // Using a memmove() appears to be slower.
      //
      // unsigned int target = index >> 1;
      //
      // memmove((char *) &buffer_[target + 1], (char *) &buffer_[target],
      //             sizeof(unsigned int) * (index - target));
      //
      // buffer_[target] = result;
      //

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

    return result;
  }

  void insert(unsigned int &value, unsigned int mask);

  void push(unsigned int &value, unsigned int mask);

  void dump();

  unsigned int getLastDiff(unsigned int mask) const
  {
    return lastDiff_;
  }

  unsigned int getBlockSize(unsigned int bits) const
  {
    if (bits > 0)
    {
      return (predictedBlockSize_ < bits ? predictedBlockSize_ : bits);
    }

    return predictedBlockSize_;
  }

  private:

  unsigned int size_;
  unsigned int length_;
  unsigned int *buffer_;
  unsigned int lastDiff_;
  unsigned int lastValueInserted_;
  unsigned int predictedBlockSize_;
};

#endif /* IntCache_H */
