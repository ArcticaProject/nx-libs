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

#ifndef WriteBuffer_H
#define WriteBuffer_H

#include "Misc.h"

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

#define WRITE_BUFFER_DEFAULT_SIZE         16384

//
// Adjust for the biggest reply that we could receive.
// This is likely to be a reply to a X_ListFonts where
// user has a large amount of installed fonts.
//

#define WRITE_BUFFER_OVERFLOW_SIZE        4194304

class WriteBuffer
{
  public:

  WriteBuffer();

  ~WriteBuffer();

  void setSize(unsigned int initialSize, unsigned int thresholdSize,
                   unsigned int maximumSize);

  unsigned char *addMessage(unsigned int numBytes);

  unsigned char *removeMessage(unsigned int numBytes);

  unsigned char *addScratchMessage(unsigned int numBytes);

  //
  // This form allows user to provide its own
  // buffer as write buffer's scratch area.
  //

  unsigned char *addScratchMessage(unsigned char *newBuffer, unsigned int numBytes);

  void removeScratchMessage();

  void partialReset();

  void fullReset();

  unsigned char *getData() const
  {
    return buffer_;
  }

  unsigned int getLength() const
  {
    return length_;
  }

  unsigned int getAvailable() const
  {
    return (size_ - length_);
  }

  unsigned char *getScratchData() const
  {
    return scratchBuffer_;
  }

  unsigned int getScratchLength() const
  {
    return scratchLength_;
  }

  unsigned int getTotalLength() const
  {
    return (length_ + scratchLength_);
  }

  void registerPointer(unsigned char **pointer)
  {
    index_ = pointer;
  }

  void unregisterPointer()
  {
    index_ = 0;
  }

  private:

  unsigned int size_;
  unsigned int length_;

  unsigned char *buffer_;
  unsigned char **index_;

  unsigned int  scratchLength_;
  unsigned char *scratchBuffer_;

  int scratchOwner_;

  unsigned int initialSize_;
  unsigned int thresholdSize_;
  unsigned int maximumSize_;
};

#endif /* WriteBuffer_H */
