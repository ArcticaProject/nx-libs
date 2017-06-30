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
