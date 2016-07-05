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

#include "Misc.h"
#include "BlockCacheSet.h"


BlockCacheSet::BlockCacheSet(unsigned int numCaches):
  caches_(new BlockCache *[numCaches]), size_(numCaches), 
  length_(0)
{
  for (unsigned int i = 0; i < numCaches; i++)
    caches_[i] = new BlockCache();
}


BlockCacheSet::~BlockCacheSet()
{
  //
  // TODO: There is still a strange segfault occurring
  // at random time under Cygwin, when proxy is being
  // shutdown. Problem appeared just after upgrading
  // to the latest version of the Cygwin DLL. A stack
  // trace, obtained at the last minute, reveals that
  // failure happens in this destructor.
  //

  #ifndef __CYGWIN32__

  for (unsigned int i = 0; i < size_; i++)
    delete caches_[i];
  delete[]caches_;

  #endif /* ifdef __CYGWIN32__ */
}


int
BlockCacheSet::lookup(unsigned int dataLength, const unsigned char *data,
		      unsigned int &index)
{
  unsigned int checksum = BlockCache::checksum(dataLength, data);
  for (unsigned int i = 0; i < length_; i++)
    if ((caches_[i]->getChecksum() == checksum) &&
	(caches_[i]->compare(dataLength, data, 0)))
    {
      // match
      index = i;
      if (i)
      {
	BlockCache *save = caches_[i];
	unsigned int target = (i >> 1);
	do
	{
	  caches_[i] = caches_[i - 1];
	  i--;
	}
	while (i > target);
	caches_[target] = save;
      }
      return 1;
    }
  // no match
  unsigned int insertionPoint = (length_ >> 1);
  unsigned int start;
  if (length_ >= size_)
    start = size_ - 1;
  else
  {
    start = length_;
    length_++;
  }
  BlockCache *save = caches_[start];
  for (unsigned int k = start; k > insertionPoint; k--)
    caches_[k] = caches_[k - 1];
  caches_[insertionPoint] = save;
  save->set(dataLength, data);
  return 0;
}


void
BlockCacheSet::get(unsigned index, unsigned int &size,
		   const unsigned char *&data)
{
  size = caches_[index]->getLength();
  data = caches_[index]->getData();
  if (index)
  {
    BlockCache *save = caches_[index];
    unsigned int target = (index >> 1);
    do
    {
      caches_[index] = caches_[index - 1];
      index--;
    }
    while (index > target);
    caches_[target] = save;
  }
}



void
BlockCacheSet::set(unsigned int dataLength, const unsigned char *data)
{
  unsigned int insertionPoint = (length_ >> 1);
  unsigned int start;
  if (length_ >= size_)
    start = size_ - 1;
  else
  {
    start = length_;
    length_++;
  }
  BlockCache *save = caches_[start];
  for (unsigned int k = start; k > insertionPoint; k--)
    caches_[k] = caches_[k - 1];
  caches_[insertionPoint] = save;
  save->set(dataLength, data);
}
