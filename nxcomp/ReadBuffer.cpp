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

#include "ReadBuffer.h"

#include "Transport.h"

//
// Set the verbosity level.
//

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

ReadBuffer::ReadBuffer(Transport *transport)

  : transport_(transport)
{
  //
  // The read buffer will grow until
  // reaching the maximum buffer size
  // and then will remain stable at
  // that size.
  //

  initialReadSize_   = READ_BUFFER_DEFAULT_SIZE;
  maximumBufferSize_ = READ_BUFFER_DEFAULT_SIZE;

  size_   = 0;
  buffer_ = NULL;

  owner_  = 1;
  length_ = 0;
  start_  = 0;

  remaining_ = 0;
}

ReadBuffer::~ReadBuffer()
{
  if (owner_ == 1)
  {
    delete [] buffer_;
  }
}

void ReadBuffer::readMessage(const unsigned char *message, unsigned int length)
{
  //
  // To be here we must be the real owner
  // of the buffer and there must not be
  // pending bytes in the transport.
  //

  #ifdef TEST

  if (owner_ == 0)
  {
    *logofs << "ReadBuffer: PANIC! Class for FD#"
            << transport_ -> fd() << " doesn't "
            << "appear to be the owner of the buffer "
            << "while borrowing from the caller.\n"
            << logofs_flush;

    HandleCleanup();
  }

  #endif

  //
  // Be sure that any outstanding data from
  // the transport is appended to our own
  // byffer.
  //

  if (transport_ -> pending() != 0)
  {
    #ifdef WARNING
    *logofs << "ReadBuffer: WARNING! Class for FD#"
            << transport_ -> fd() << " has pending "
            << "data in the transport while "
            << "borrowing from the caller.\n"
            << logofs_flush;
    #endif

    readMessage();

    if (owner_ == 0)
    {
      convertBuffer();
    }
  }

  //
  // Can't borrow the buffer if there is data
  // from a partial message. In this case add
  // the new data to the end of our buffer.
  //

  if (length_ == 0)
  {
    #ifdef TEST
    *logofs << "ReadBuffer: Borrowing " << length
            << " bytes from the caller for FD#"
            << transport_ -> fd() << " with "
            << length_ << " bytes in the buffer.\n"
            << logofs_flush;
    #endif

    delete [] buffer_;

    buffer_ = (unsigned char *) message;
    size_   = length;

    length_ = length;

    owner_ = 0;
    start_ = 0;
  }
  else
  {
    #ifdef TEST
    *logofs << "ReadBuffer: Appending " << length
            << " bytes from the caller for FD#"
            << transport_ -> fd() << " with "
            << length_ << " bytes in the buffer.\n"
            << logofs_flush;
    #endif

    appendBuffer(message, length);
  }
}

int ReadBuffer::readMessage()
{
  int pendingLength = transport_ -> pending();

  if (pendingLength > 0)
  {
    //
    // Can't move the data in the borrowed buffer,
    // so use the tansport buffer only if we don't
    // have any partial message. This can happen
    // with the proxy where we need to deflate the
    // stream.
    //

    if (length_ == 0)
    {
      unsigned char *newBuffer;

      length_ = transport_ -> getPending(newBuffer);

      if (newBuffer == NULL)
      {
        #ifdef PANIC
        *logofs << "ReadBuffer: PANIC! Failed to borrow "
                << length_ << " bytes of memory for buffer "
                << "in context [A].\n" << logofs_flush;
        #endif

        cerr << "Error" << ": Failed to borrow memory for "
             << "read buffer in context [A].\n";

        HandleCleanup();
      }

      delete [] buffer_;

      buffer_ = newBuffer;
      size_   = length_;

      owner_ = 0;
      start_ = 0;

      #ifdef TEST
      *logofs << "ReadBuffer: Borrowed " << length_
              << " pending bytes for FD#" << transport_ ->
                 fd() << ".\n" << logofs_flush;
      #endif

      return length_;
    }
    #ifdef TEST
    else
    {
      *logofs << "ReadBuffer: WARNING! Cannot borrow "
              << pendingLength << " bytes for FD#"
              << transport_ -> fd() << " with "
              << length_ << " bytes in the buffer.\n"
              << logofs_flush;
    }
    #endif
  }

  unsigned int readLength = suggestedLength(pendingLength);

  #ifdef DEBUG
  *logofs << "ReadBuffer: Requested " << readLength
          << " bytes for FD#" << transport_ -> fd()
          << " with readable " << transport_ -> readable()
          << " remaining " << remaining_ << " pending "
          << transport_ -> pending() << ".\n"
          << logofs_flush;
  #endif

  if (readLength < initialReadSize_)
  {
    readLength = initialReadSize_;
  }

  #ifdef DEBUG
  *logofs << "ReadBuffer: Buffer size is " << size_
          << " length " << length_ << " and start "
          << start_  << ".\n" << logofs_flush;
  #endif

  //
  // We can't use the transport buffer
  // to read our own data in it.
  //

  #ifdef TEST

  if (owner_ == 0)
  {
    *logofs << "ReadBuffer: PANIC! Class for FD#"
            << transport_ -> fd() << " doesn't "
            << "appear to be the owner of the buffer "
            << "while reading.\n" << logofs_flush;

    HandleCleanup();
  }

  #endif

  //
  // Be sure that we have enough space
  // to store all the requested data.
  //

  if (buffer_ == NULL || length_ + readLength > size_)
  {
    unsigned int newSize = length_ + readLength;

    #ifdef TEST
    *logofs << "ReadBuffer: Resizing buffer for FD#"
            << transport_ -> fd() << " in read from "
            << size_ << " to " << newSize << " bytes.\n"
            << logofs_flush;
    #endif

    unsigned char *newBuffer = allocateBuffer(newSize);

    memcpy(newBuffer, buffer_ + start_, length_);

    delete [] buffer_;

    buffer_ = newBuffer;
    size_   = newSize;

    transport_ -> pendingReset();

    owner_ = 1;
  }
  else if (start_ != 0 && length_ != 0)
  {
    //
    // If any bytes are left due to a partial
    // message, shift them to the beginning
    // of the buffer.
    //

    #ifdef TEST
    *logofs << "ReadBuffer: Moving " << length_
            << " bytes of data " << "at beginning of "
            << "the buffer for FD#" << transport_ -> fd() 
            << ".\n" << logofs_flush;
    #endif

    memmove(buffer_, buffer_ + start_, length_);
  }

  start_ = 0;

  #ifdef DEBUG
  *logofs << "ReadBuffer: Buffer size is now " << size_ 
          << " length is " << length_ << " and start is " 
          << start_ << ".\n" << logofs_flush;
  #endif

  unsigned char *readData = buffer_ + length_;

  #ifdef DEBUG
  *logofs << "ReadBuffer: Going to read " << readLength 
          << " bytes from FD#" << transport_ -> fd() << ".\n"
          << logofs_flush;
  #endif

  int bytesRead = transport_ -> read(readData, readLength);

  if (bytesRead > 0)
  {
    #ifdef TEST
    *logofs << "ReadBuffer: Read " << bytesRead
            << " bytes from FD#" << transport_ -> fd()
            << ".\n" << logofs_flush;
    #endif

    length_ += bytesRead;
  }
  else if (bytesRead < 0)
  {
    //
    // Check if there is more data pending than the
    // size of the provided buffer. After reading
    // the requested amount, in fact, the transport
    // may have decompressed the data and produced
    // more input. This trick allows us to always
    // borrow the buffer from the transport, even
    // when the partial read would have prevented
    // that.
    //

    if (transport_ -> pending() > 0)
    {
      #ifdef TEST
      *logofs << "ReadBuffer: WARNING! Trying to read some "
              << "more with " << transport_ -> pending()
              << " bytes pending for FD#" << transport_ ->
                 fd() << ".\n" << logofs_flush;
      #endif

      return readMessage();
    }

    #ifdef TEST
    *logofs << "ReadBuffer: Error detected reading "
            << "from FD#" << transport_ -> fd()
            << ".\n" << logofs_flush;
    #endif

    return -1;
  }
  #ifdef TEST
  else
  {
    *logofs << "ReadBuffer: No data read from FD#"
            << transport_ -> fd() << " with remaining "
            << remaining_ << ".\n" << logofs_flush;
  }
  #endif

  return bytesRead;
}

const unsigned char *ReadBuffer::getMessage(unsigned int &controlLength,
                                                unsigned int &dataLength)
{
  #ifdef TEST

  if (transport_ -> pending() > 0)
  {
    *logofs << "ReadBuffer: PANIC! The transport "
            << "appears to have data pending.\n"
            << logofs_flush;

    HandleCleanup();
  }

  #endif

  if (length_ == 0)
  {
    #ifdef DEBUG
    *logofs << "ReadBuffer: No message can be located "
            << "for FD#" << transport_ -> fd() << ".\n"
            << logofs_flush;
    #endif

    if (owner_ == 0)
    {
      buffer_ = NULL;
      size_   = 0;

      transport_ -> pendingReset();

      owner_ = 1;
      start_ = 0;
    }

    return NULL;
  }

  unsigned int trailerLength;

  #ifdef DEBUG
  *logofs << "ReadBuffer: Going to locate message with "
          << "start at " << start_ << " and length "
          << length_ << " for FD#" << transport_ -> fd()
          << ".\n" << logofs_flush;
  #endif

  int located = locateMessage(buffer_ + start_, buffer_ + start_ + length_,
                                  controlLength, dataLength, trailerLength);

  if (located == 0)
  {
    //
    // No more complete messages are in
    // the buffer.
    //

    #ifdef DEBUG
    *logofs << "ReadBuffer: No message was located "
            << "for FD#" << transport_ -> fd()
            << ".\n" << logofs_flush;
    #endif

    if (owner_ == 0)
    {
      //
      // Must move the remaining bytes in
      // our own buffer.
      //

      convertBuffer();
    }
  
    return NULL;
  }
  else
  {
    const unsigned char *result = buffer_ + start_;

    if (dataLength > 0)
    {
      //
      // Message contains data, so go to the
      // first byte of payload.
      //

      result += trailerLength;

      start_  += (dataLength + trailerLength);
      length_ -= (dataLength + trailerLength);
    }
    else
    {
      //
      // It is a control message.
      //

      start_  += (controlLength + trailerLength);
      length_ -= (controlLength + trailerLength);
    }

    #ifdef DEBUG
    *logofs << "ReadBuffer: Located message for FD#"
            << transport_ -> fd() << " with control length "
            << controlLength << " and data length "
            << dataLength << ".\n" << logofs_flush;
    #endif

    remaining_ = 0;

    return result;
  }
}

int ReadBuffer::setSize(int initialReadSize, int maximumBufferSize)
{
  initialReadSize_   = initialReadSize;
  maximumBufferSize_ = maximumBufferSize;

  #ifdef TEST
  *logofs << "ReadBuffer: WARNING! Set buffer parameters to "
          << initialReadSize_ << "/" << maximumBufferSize_
          << " for object at "<< this << ".\n"
          << logofs_flush;
  #endif

  return 1;
}

void ReadBuffer::fullReset()
{
  #ifdef TEST

  if (owner_ == 0)
  {
    *logofs << "ReadBuffer: PANIC! Class for FD#"
            << transport_ -> fd() << " doesn't "
            << "appear to be the owner of the buffer "
            << "in reset.\n" << logofs_flush;

    HandleCleanup();
  }

  #endif

  if (length_ == 0 && size_ > maximumBufferSize_)
  {
    #ifdef TEST
    *logofs << "ReadBuffer: Resizing buffer for FD#"
            << transport_ -> fd() << " in reset from "
            << size_ << " to " << maximumBufferSize_
            << " bytes.\n" << logofs_flush;
    #endif

    delete [] buffer_;

    int newSize = maximumBufferSize_;

    unsigned char *newBuffer = allocateBuffer(newSize);

    buffer_ = newBuffer;
    size_   = newSize;

    transport_ -> pendingReset();

    owner_ = 1;
    start_ = 0;
  }
}

unsigned char *ReadBuffer::allocateBuffer(unsigned int newSize)
{
  unsigned char *newBuffer = new unsigned char[newSize];

  if (newBuffer == NULL)
  {
    #ifdef PANIC
    *logofs << "ReadBuffer: PANIC! Can't allocate "
            << newSize << " bytes of memory for buffer "
            << "in context [B].\n" << logofs_flush;
    #endif

    cerr << "Error" << ": Can't allocate memory for "
         << "read buffer in context [B].\n";

    HandleCleanup();
  }

  #ifdef VALGRIND

  memset(newBuffer, '\0', newSize);

  #endif

  return newBuffer;
}

void ReadBuffer::appendBuffer(const unsigned char *message, unsigned int length)
{
  if (start_ + length_ + length > size_)
  {
    unsigned int newSize = length_ + length + initialReadSize_;

    #ifdef TEST
    *logofs << "ReadBuffer: WARNING! Resizing buffer "
            << "for FD#" << transport_ -> fd()
            << " from " << size_ << " to " << newSize
            << " bytes.\n" << logofs_flush;
    #endif

    unsigned char *newBuffer = allocateBuffer(newSize);

    memcpy(newBuffer, buffer_ + start_, length_);

    delete [] buffer_;

    buffer_ = newBuffer;
    size_   = newSize;

    start_ = 0;
  }

  memcpy(buffer_ + start_ + length_, message, length);

  length_ += length;

  transport_ -> pendingReset();

  owner_ = 1;
}

void ReadBuffer::convertBuffer()
{
  unsigned int newSize = length_ + initialReadSize_;

  #ifdef TEST
  *logofs << "ReadBuffer: WARNING! Converting "
          << length_ << " bytes to own buffer "
          << "for FD#" << transport_ -> fd()
          << " with new size " << newSize
          << " bytes.\n" << logofs_flush;
  #endif

  unsigned char *newBuffer = allocateBuffer(newSize);

  memcpy(newBuffer, buffer_ + start_, length_);

  buffer_ = newBuffer;
  size_   = newSize;

  transport_ -> pendingReset();

  owner_ = 1;
  start_ = 0;
}
