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

#ifndef ActionCache_H
#define ActionCache_H

#include "IntCache.h"

class ActionCache
{
  friend class EncodeBuffer;
  friend class DecodeBuffer;
  
  public:

  ActionCache();
  ~ActionCache();

  private:

  IntCache *base_[256];

  unsigned int slot_;
  unsigned short last_;
};

#endif /* ActionCache_H */
