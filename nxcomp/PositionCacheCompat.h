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

#ifndef PositionCacheCompat_H
#define PositionCacheCompat_H

#include "IntCache.h"

class PositionCacheCompat
{
  friend class EncodeBuffer;
  friend class DecodeBuffer;
  
  public:

  PositionCacheCompat();
  ~PositionCacheCompat();

  private:

  IntCache *base_[32];

  unsigned int slot_;
  short int last_;
};

#endif /* PositionCacheCompat_H */
