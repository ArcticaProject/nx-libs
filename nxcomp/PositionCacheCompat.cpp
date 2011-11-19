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

#include "Control.h"

#include "PositionCacheCompat.h"

PositionCacheCompat::PositionCacheCompat()
{
  if (control -> isProtoStep7() == 0)
  {
    for (int i = 0; i < 32; i++)
    {
      base_[i] = new IntCache(8);
    }

    slot_ = 0;
    last_ = 0;
  }
}

PositionCacheCompat::~PositionCacheCompat()
{
  if (control -> isProtoStep7() == 0)
  {
    for (int i = 0; i < 32; i++)
    {
      delete base_[i];
    }
  }
}
