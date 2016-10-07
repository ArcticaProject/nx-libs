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
