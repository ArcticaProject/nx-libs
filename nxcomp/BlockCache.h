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

#ifndef BlockCache_H
#define BlockCache_H


// Cache to hold an arbitrary-length block of bytes

class BlockCache
{
  public:
  BlockCache():buffer_(0), size_(0), checksum_(0)
  {
  }
   ~BlockCache()
  {
    delete[]buffer_;
  }
  int compare(unsigned int size, const unsigned char *data,
	      int overwrite = 1);
  void set(unsigned int size, const unsigned char *data);

  unsigned int getLength() const
  {
    return size_;
  }
  unsigned int getChecksum() const
  {
    return checksum_;
  }
  const unsigned char *getData() const
  {
    return buffer_;
  }

  static unsigned int checksum(unsigned int size, const unsigned char *data);

private:
  unsigned char *buffer_;
  unsigned int size_;
  unsigned int checksum_;
};

#endif /* BlockCache_H */
