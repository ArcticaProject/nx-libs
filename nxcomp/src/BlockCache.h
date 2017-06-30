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
