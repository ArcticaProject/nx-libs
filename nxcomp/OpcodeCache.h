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

#ifndef OpcodeCache_H
#define OpcodeCache_H

#include "CharCache.h"

class OpcodeCache
{
  friend class EncodeBuffer;
  friend class DecodeBuffer;
  
  public:

  OpcodeCache()
  {
    slot_ = 0;
  }

  ~OpcodeCache()
  {
  }

  private:

  CharCache base_[256];
  unsigned char slot_;
};

#endif /* OpcodeCache_H */
