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

#ifndef TextCompressor_H
#define TextCompressor_H

#include "CharCache.h"

class EncodeBuffer;
class DecodeBuffer;

class TextCompressor
{
 public:
  TextCompressor(CharCache* cache, unsigned int cacheSize):
    cache_(cache),
    cacheSize_(cacheSize),
    key_(0)
  {
  }

  void encodeChar(unsigned char ch, EncodeBuffer &);
  unsigned char decodeChar(DecodeBuffer &);
  void reset(unsigned int newKey = 0)
  {
    key_ = newKey;
  }

 private:
  CharCache* cache_;
  unsigned int cacheSize_;
  unsigned int key_;
};

#endif /* TextCompressor_H */
