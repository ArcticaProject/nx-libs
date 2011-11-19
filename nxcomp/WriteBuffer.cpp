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

#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "Misc.h"
#include "Control.h"

#include "WriteBuffer.h"

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

WriteBuffer::WriteBuffer()
{
  size_   = WRITE_BUFFER_DEFAULT_SIZE;
  buffer_ = new unsigned char[size_];
  length_ = 0;
  index_  = NULL;

  scratchLength_ = 0;
  scratchBuffer_ = NULL;
  scratchOwner_  = 1;

  initialSize_   = WRITE_BUFFER_DEFAULT_SIZE;
  thresholdSize_ = WRITE_BUFFER_DEFAULT_SIZE << 1;
  maximumSize_   = WRITE_BUFFER_DEFAULT_SIZE << 4;

  #ifdef VALGRIND

  memset(buffer_, '\0', size_);

  #endif
}

WriteBuffer::~WriteBuffer()
{
  if (scratchOwner_ == 1 &&
          scratchBuffer_ != NULL)
  {
    delete [] scratchBuffer_;
  }

  delete [] buffer_;
}

void WriteBuffer::setSize(unsigned int initialSize, unsigned int thresholdSize,
                              unsigned int maximumSize)
{
  initialSize_   = initialSize;
  thresholdSize_ = thresholdSize;
  maximumSize_   = maximumSize;

  #ifdef TEST
  *logofs << "WriteBuffer: Set buffer sizes to "
          << initialSize_ << "/" << thresholdSize_
          << "/" << maximumSize_ << ".\n"
          << logofs_flush;
  #endif
}

void WriteBuffer::partialReset()
{
  if (scratchBuffer_ != NULL)
  {
    if (scratchOwner_)
    {
      #ifdef DEBUG
      *logofs << "WriteBuffer: Going to delete "
              << scratchLength_ << " bytes from the "
              << "scratch buffer.\n" << logofs_flush;
      #endif

      delete [] scratchBuffer_;
    }

    scratchLength_ = 0;
    scratchBuffer_ = NULL;
    scratchOwner_  = 1;
  }

  length_ = 0;
  index_  = NULL;

  #ifdef DEBUG
  *logofs << "WriteBuffer: Performed partial reset with "
          << size_ << " bytes in buffer.\n"
          << logofs_flush;
  #endif
}

void WriteBuffer::fullReset()
{
  if (scratchBuffer_ != NULL)
  {
    if (scratchOwner_ == 1)
    {
      #ifdef DEBUG
      *logofs << "WriteBuffer: Going to delete "
              << scratchLength_ << " bytes from the "
              << "scratch buffer.\n" << logofs_flush;
      #endif

      delete [] scratchBuffer_;
    }

    scratchLength_ = 0;
    scratchBuffer_ = NULL;
    scratchOwner_  = 1;
  }

  length_ = 0;
  index_  = NULL;

  if (size_ > initialSize_)
  {
    #ifdef TEST
    *logofs << "WriteBuffer: Reallocating a new buffer of "
            << initialSize_ << " bytes.\n" << logofs_flush;
    #endif

    delete [] buffer_;

    size_ = initialSize_;

    buffer_ = new unsigned char[size_];

    if (buffer_ == NULL)
    {
      #ifdef PANIC
      *logofs << "WriteBuffer: PANIC! Can't allocate memory for "
              << "X messages in context [A].\n" << logofs_flush;
      #endif

      cerr << "Error" << ": Can't allocate memory for "
           << "X messages in context [A].\n";

      HandleAbort();
    }

    #ifdef VALGRIND

    memset(buffer_, '\0', size_);

    #endif
  }

  #ifdef DEBUG
  *logofs << "WriteBuffer: Performed full reset with "
          << size_ << " bytes in buffer.\n"
          << logofs_flush;
  #endif
}

unsigned char *WriteBuffer::addMessage(unsigned int numBytes)
{
  #ifdef DEBUG
  *logofs << "WriteBuffer: Adding " << numBytes << " bytes to "
          << length_ << " bytes already in buffer.\n"
          << logofs_flush;
  #endif

  if (numBytes > WRITE_BUFFER_OVERFLOW_SIZE)
  {
    #ifdef PANIC
    *logofs << "WriteBuffer: PANIC! Can't add a message of "
            << numBytes << " bytes.\n" << logofs_flush;

    *logofs << "WriteBuffer: PANIC! Assuming error handling "
            << "data in context [B].\n" << logofs_flush;
    #endif

    cerr << "Error" << ": Can't add a message of "
         << numBytes << " bytes to write buffer.\n";

    cerr << "Error" << ": Assuming error handling "
         << "data in context [B].\n";

    HandleAbort();
  }
  else if (length_ + numBytes > size_)
  {
    unsigned int newSize = thresholdSize_;

    while (newSize < length_ + numBytes)
    {
      newSize <<= 1;

      if (newSize > maximumSize_)
      {
        newSize = length_ + numBytes + initialSize_;
      }
    }

    #ifdef TEST
    *logofs << "WriteBuffer: Growing buffer from "
            << size_ << " to " << newSize << " bytes.\n"
            << logofs_flush;
    #endif

    unsigned int indexOffset = 0;

    if (index_ && *index_)
    {
      indexOffset = *index_ - buffer_;
    }

    size_ = newSize;

    unsigned char *newBuffer = new unsigned char[size_];

    if (newBuffer == NULL)
    {
      #ifdef PANIC
      *logofs << "WriteBuffer: PANIC! Can't allocate memory for "
              << "X messages in context [C].\n" << logofs_flush;
      #endif

      cerr << "Error" << ": Can't allocate memory for "
           << "X messages in context [C].\n";

      HandleAbort();
    }

    #ifdef TEST
    if (newSize >= maximumSize_)
    {
      *logofs << "WriteBuffer: WARNING! Buffer grown to reach "
              << "size of " << newSize << " bytes.\n"
              << logofs_flush;
    }
    #endif

    #ifdef VALGRIND

    memset(newBuffer, '\0', size_);

    #endif

    memcpy(newBuffer, buffer_, length_);

    #ifdef DEBUG
    *logofs << "WriteBuffer: Going to delete the "
            << "old buffer with new size " << size_
            << ".\n" << logofs_flush;
    #endif

    delete [] buffer_;

    buffer_ = newBuffer;

    if (index_ && *index_)
    {
      *index_ = buffer_ + indexOffset;
    }
  }

  unsigned char *result = buffer_ + length_;

  length_ += numBytes;

  #ifdef DEBUG
  *logofs << "WriteBuffer: Bytes in buffer are "
          << length_ << " while size is " << size_
          << ".\n" << logofs_flush;
  #endif

  return result;
}

unsigned char *WriteBuffer::removeMessage(unsigned int numBytes)
{
  #ifdef TEST
  *logofs << "WriteBuffer: Removing " << numBytes
          << " bytes from buffer.\n" << logofs_flush;
  #endif

  if (numBytes > length_)
  {
    #ifdef PANIC
    *logofs << "WriteBuffer: PANIC! Can't remove "
            << numBytes << " bytes with only " << length_
            << " bytes in buffer.\n" << logofs_flush;
    #endif

    cerr << "Error" << ": Buffer underflow handling "
         << "write buffer in context [D].\n";

    HandleAbort();
  }

  length_ -= numBytes;

  #ifdef TEST
  *logofs << "WriteBuffer: Bytes in buffer are now "
          << length_ << " while size is "
          << size_ << ".\n" << logofs_flush;
  #endif

  return (buffer_ + length_);
}

unsigned char *WriteBuffer::addScratchMessage(unsigned int numBytes)
{
  if (numBytes > WRITE_BUFFER_OVERFLOW_SIZE)
  {
    #ifdef PANIC
    *logofs << "WriteBuffer: PANIC! Can't add a message of "
            << numBytes << " bytes.\n" << logofs_flush;

    *logofs << "WriteBuffer: PANIC! Assuming error handling "
            << "data in context [E].\n" << logofs_flush;
    #endif

    cerr << "Error" << ": Can't add a message of "
         << numBytes << " bytes to write buffer.\n";

    cerr << "Error" << ": Assuming error handling "
         << "data in context [E].\n";

    HandleAbort();
  }
  else if (scratchBuffer_ != NULL)
  {
    #ifdef PANIC
    *logofs << "WriteBuffer: PANIC! Can't add a message of "
            << numBytes << " bytes with " << scratchLength_
            << " bytes already in scratch buffer.\n"
            << logofs_flush;

    *logofs << "WriteBuffer: PANIC! Assuming error handling "
            << "data in context [F].\n" << logofs_flush;
    #endif

    cerr << "Error" << ": Can't add a message of "
         << numBytes << " bytes with " << scratchLength_
         << " bytes already in scratch buffer.\n";

    cerr << "Error" << ": Assuming error handling "
         << "data in context [F].\n";

    HandleAbort();
  }

  #ifdef DEBUG
  *logofs << "WriteBuffer: Adding " << numBytes << " bytes "
          << "to scratch buffer.\n" << logofs_flush;
  #endif

  unsigned char *newBuffer = new unsigned char[numBytes];

  if (newBuffer == NULL)
  {
    #ifdef PANIC
    *logofs << "WriteBuffer: PANIC! Can't allocate memory for "
            << "X messages in context [G].\n" << logofs_flush;
    #endif

    cerr << "Error" << ": Can't allocate memory for "
         << "X messages in context [G].\n";

    HandleAbort();
  }

  #ifdef VALGRIND

  memset(newBuffer, '\0', numBytes);

  #endif

  scratchBuffer_ = newBuffer;
  scratchOwner_  = 1;
  scratchLength_ = numBytes;

  return newBuffer;
}

unsigned char *WriteBuffer::addScratchMessage(unsigned char *newBuffer, unsigned int numBytes)
{
  if (numBytes > WRITE_BUFFER_OVERFLOW_SIZE)
  {
    #ifdef PANIC
    *logofs << "WriteBuffer: PANIC! Can't add a message of "
            << numBytes << " bytes.\n" << logofs_flush;

    *logofs << "WriteBuffer: PANIC! Assuming error handling "
            << "data in context [H].\n" << logofs_flush;
    #endif

    cerr << "Error" << ": Can't add a message of "
         << numBytes << " bytes to write buffer.\n";

    cerr << "Error" << ": Assuming error handling "
         << "data in context [H].\n";

    HandleAbort();
  }
  else if (scratchBuffer_ != NULL)
  {
    #ifdef PANIC
    *logofs << "WriteBuffer: PANIC! Can't add a foreign "
            << "message of " << numBytes << " bytes with "
            << scratchLength_ << " bytes already in "
            << "scratch buffer.\n" << logofs_flush;

    *logofs << "WriteBuffer: PANIC! Assuming error handling "
            << "data in context [I].\n" << logofs_flush;
    #endif

    cerr << "Error" << ": Can't add a foreign message of "
         << numBytes << " bytes with " << scratchLength_
         << " bytes already in scratch buffer.\n";

    cerr << "Error" << ": Assuming error handling "
         << "data in context [I].\n";

    HandleAbort();
  }

  #ifdef DEBUG
  *logofs << "WriteBuffer: Adding " << numBytes << " bytes "
          << "from a foreign message to scratch buffer.\n"
          << logofs_flush;
  #endif

  scratchBuffer_ = newBuffer;
  scratchLength_ = numBytes;
  scratchOwner_  = 0;

  return newBuffer;
}

void WriteBuffer::removeScratchMessage()
{
  #ifdef TEST

  if (scratchLength_ == 0 || scratchBuffer_ == NULL)
  {
    #ifdef PANIC
    *logofs << "WriteBuffer: PANIC! Can't remove non existent scratch message.\n"
            << logofs_flush;
    #endif

    cerr << "Error" << ": Can't remove non existent scratch message.\n";

    HandleAbort();
  }

  *logofs << "WriteBuffer: Removing " << scratchLength_
          << " bytes from scratch buffer.\n"
          << logofs_flush;

  #endif

  if (scratchOwner_ == 1)
  {
    #ifdef DEBUG
    *logofs << "WriteBuffer: Going to delete "
           << scratchLength_ << " bytes from the "
            << "scratch buffer.\n" << logofs_flush;
    #endif

    delete [] scratchBuffer_;
  }

  scratchLength_ = 0;
  scratchBuffer_ = NULL;
  scratchOwner_  = 1;
}
