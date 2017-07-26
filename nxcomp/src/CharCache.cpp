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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

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
