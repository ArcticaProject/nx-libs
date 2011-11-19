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

#ifndef XidCache_H
#define XidCache_H

#include "IntCache.h"

class XidCache
{
  friend class EncodeBuffer;
  friend class DecodeBuffer;
  
  public:

  XidCache();
  ~XidCache();

  private:

  IntCache *base_[256];

  unsigned int slot_;
  unsigned int last_;
};

#endif /* XidCache_H */
