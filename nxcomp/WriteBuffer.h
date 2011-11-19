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
