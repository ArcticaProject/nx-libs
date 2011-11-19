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

#ifndef BlockCacheSet_H
#define BlockCacheSet_H

#include "BlockCache.h"


class BlockCacheSet
{
  public:
  BlockCacheSet(unsigned int numCaches);
   ~BlockCacheSet();

  int lookup(unsigned int size, const unsigned char *data,
	     unsigned int &index);
  void get(unsigned int index, unsigned int &size, const unsigned char *&data);
  void set(unsigned int size, const unsigned char *data);

    private:
    BlockCache ** caches_;
  unsigned int size_;
  unsigned int length_;
};

#endif /* BlockCacheSet_H */
