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

#ifndef ActionCacheCompat_H
#define ActionCacheCompat_H

#include "CharCache.h"

class ActionCacheCompat
{
  friend class EncodeBuffer;
  friend class DecodeBuffer;
  
  public:

  ActionCacheCompat()
  {
    slot_ = 0;
  }

  ~ActionCacheCompat()
  {
  }

  private:

  CharCache base_[4];
  unsigned char slot_;
};

#endif /* ActionCacheCompat_H */
