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

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/socket.h>

#include "Transport.h"

#include "Statistics.h"

//
// Set the verbosity level. You also
// need to define DUMP in Misc.cpp
// if DUMP is defined here.
//

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG
#undef  INSPECT
#undef  DUMP

//
// Used to lock and unlock the transport
// buffers before they are accessed by
// different threads.
//

#undef  THREADS

//
// Define this to get logging all the
// operations performed by the parent
// thread, the one that enqueues and
// dequeues data.
//

#define PARENT

//
// Define this to know when a channel
// is created or destroyed.
//

#undef  REFERENCES

//
// Reference count for allocated buffers.
//

#ifdef REFERENCES

int Transport::references_;
int ProxyTransport::references_;
int InternalTransport::references_;

#endif

//
// This is the base class providing methods for read
// and write buffering.
//

Transport::Transport(int fd) : fd_(fd)
{
  #ifdef TEST
  *logofs << "Transport: Going to create base transport "
          << "for FD#" << fd_ << ".\n" << logofs_flush;
  #endif

  type_ = transport_base;

  //
  // Set up the write buffer.
  //

  w_buffer_.length_ = 0;
  w_buffer_.start_  = 0;

  initialSize_   = TRANSPORT_BUFFER_DEFAULT_SIZE;
  thresholdSize_ = TRANSPORT_BUFFER_DEFAULT_SIZE << 1;
  maximumSize_   = TRANSPORT_BUFFER_DEFAULT_SIZE << 4;

  w_buffer_.data_.resize(initialSize_);

  //
  // Set non-blocking IO on socket.
  //

  SetNonBlocking(fd_, 1);

  blocked_ = 0;
  finish_  = 0;

  #ifdef REFERENCES
  *logofs << "Transport: Created new object at " 
          << this << " out of " << ++references_ 
          << " allocated references.\n" << logofs_flush;
  #endif
}

Transport::~Transport()
{
  #ifdef TEST
  *logofs << "Transport: Going to destroy base class "
          << "for FD#" << fd_ << ".\n" << logofs_flush;
  #endif

  ::close(fd_);

  #ifdef REFERENCES
  *logofs << "Transport: Deleted object at " 
          << this << " out of " << --references_ 
          << " allocated references.\n" << logofs_flush;
  #endif
}

//
// Read data from its file descriptor.
//

int Transport::read(unsigned char *data, unsigned int size)
{
  #ifdef DEBUG
  *logofs << "Transport: Going to read " << size << " bytes from " 
          << "FD#" << fd_ << ".\n" << logofs_flush;
  #endif

  //
  // Read the available data from the socket.
  //

  int result = ::read(fd_, data, size);

  //
  // Update the current timestamp as the read
  // can have scheduled some other process.
  //

  getNewTimestamp();

  if (result < 0)
  {
    if (EGET() == EAGAIN)
    {
      #ifdef TEST
      *logofs << "Transport: WARNING! Read of " << size << " bytes from "
              << "FD#" << fd_ << " would block.\n" << logofs_flush;
      #endif

      return 0;
    }
    else if (EGET() == EINTR)
    {
      #ifdef TEST
      *logofs << "Transport: Read of " << size << " bytes from "
              << "FD#" << fd_ << " was interrupted.\n"
              << logofs_flush;
      #endif

      return 0;
    }
    else
    {
      #ifdef TEST
      *logofs << "Transport: Error reading from " 
              << "FD#" << fd_ << ".\n" << logofs_flush;
      #endif

      finish();

      return -1;
    }
  }
  else if (result == 0)
  {
    #ifdef TEST
    *logofs << "Transport: No data read from " 
            << "FD#" << fd_ << ".\n" << logofs_flush;
    #endif

    finish();

    return -1;
  }

  #ifdef TEST
  *logofs << "Transport: Read " << result << " bytes out of "
          << size << " from FD#" << fd_ << ".\n" << logofs_flush;
  #endif

  #ifdef DUMP

  *logofs << "Transport: Dumping content of read data.\n"
          << logofs_flush;

  DumpData(data, result);

  #endif

  return result;
}

//
// Write as many bytes as possible to socket. 
// Append the remaining data bytes to the end 
// of the buffer and update length to reflect
// changes.
//

int Transport::write(T_write type, const unsigned char *data, const unsigned int size)
{
  //
  // If an immediate write was requested then
  // flush the enqueued data first.
  //
  // Alternatively may try to write only if
  // the socket is not blocked.
  //
  // if (w_buffer_.length_ > 0 && blocked_ == 0 &&
  //         type == write_immediate)
  // {
  //   ...
  // }
  //

  if (w_buffer_.length_ > 0 && type == write_immediate)

  {
    #ifdef TEST
    *logofs << "Transport: Writing " << w_buffer_.length_
            << " bytes of previous data to FD#" << fd_ << ".\n"
            << logofs_flush;
    #endif

    int result = Transport::flush();

    if (result < 0)
    {
      return -1;
    }
  }

  //
  // If nothing is remained, write immediately
  // to the socket.
  //

  unsigned int written = 0;

  if (w_buffer_.length_ == 0 && blocked_ == 0 &&
          type == write_immediate)
  {
    //
    // Limit the amount of data sent.
    //

    unsigned int toWrite = size;

    #ifdef DUMP

    *logofs << "Transport: Going to write " << toWrite
            << " bytes to FD#" << fd_ << " with checksum ";

    DumpChecksum(data, size);

    *logofs << ".\n" << logofs_flush;

    #endif

    T_timestamp writeTs;

    int diffTs;

    while (written < toWrite)
    {
      //
      // Trace system time spent writing data.
      //

      writeTs = getTimestamp();

      int result = ::write(fd_, data + written, toWrite - written);

      diffTs = diffTimestamp(writeTs, getNewTimestamp());

      statistics -> addWriteTime(diffTs);

      if (result <= 0)
      {
        if (EGET() == EAGAIN)
        {
          #ifdef TEST
          *logofs << "Transport: Write of " << toWrite - written
                  << " bytes on FD#" << fd_ << " would block.\n"
                  << logofs_flush;
          #endif

          blocked_ = 1;

          break;
        }
        else if (EGET() == EINTR)
        {
          #ifdef TEST
          *logofs << "Transport: Write of " << toWrite - written
                  << " bytes on FD#" << fd_ << " was interrupted.\n"
                  << logofs_flush;
          #endif

          continue;
        }
        else
        {
          #ifdef TEST
          *logofs << "Transport: Write to " << "FD#" 
                  << fd_ << " failed.\n" << logofs_flush;
          #endif

          finish();

          return -1;
        }
      }
      else
      {
        #ifdef TEST
        *logofs << "Transport: Immediately written " << result
                << " bytes on " << "FD#" << fd_ << ".\n"
                << logofs_flush;
        #endif

        written += result;
      }
    }

    #ifdef DUMP

    if (written > 0)
    {
      *logofs << "Transport: Dumping content of immediately written data.\n"
              << logofs_flush;

      DumpData(data, written);
    }

    #endif
  }

  if (written == size)
  {
    //
    // We will not affect the write buffer.
    //

    return written;
  }

  #ifdef DEBUG
  *logofs << "Transport: Going to append " << size - written
          << " bytes to write buffer for " << "FD#" << fd_
          << ".\n" << logofs_flush;
  #endif

  if (resize(w_buffer_, size - written) < 0)
  {
    return -1;
  }

  memmove(w_buffer_.data_.begin() + w_buffer_.start_ + w_buffer_.length_,
              data + written, size - written);

  w_buffer_.length_ += size - written;

  #ifdef TEST
  *logofs << "Transport: Write buffer for FD#" << fd_
          << " has data for " << w_buffer_.length_ << " bytes.\n"
          << logofs_flush;

  *logofs << "Transport: Start is " << w_buffer_.start_ 
          << " length is " << w_buffer_.length_ << " size is "
          << w_buffer_.data_.size() << " capacity is "
          << w_buffer_.data_.capacity() << ".\n"
          << logofs_flush;
  #endif

  //
  // Note that this function always returns the whole
  // size of buffer that was provided, either if not
  // all the data could be actually written.
  //

  return size;
}

//
// Write pending data to its file descriptor.
//

int Transport::flush()
{
  if (w_buffer_.length_ == 0)
  {
    #ifdef TEST
    *logofs << "Transport: No data to flush on "
            << "FD#" << fd_ << ".\n" << logofs_flush;
    #endif

    #ifdef WARNING
    if (blocked_ != 0)
    {
      *logofs << "Transport: Blocked flag is " << blocked_
              << " with no data to flush on FD#" << fd_
              << ".\n" << logofs_flush;
    }
    #endif

    return 0;
  }

  //
  // It's time to move data from the 
  // write buffer to the real link. 
  //

  int written = 0;

  int toWrite = w_buffer_.length_;

  //
  // We will do our best to write any available
  // data to the socket, so let's say we start
  // from a clean state.
  //

  blocked_ = 0;

  #ifdef TEST
  *logofs << "Transport: Going to flush " << toWrite
          << " bytes on FD#" << fd_ << ".\n"
          << logofs_flush;
  #endif

  T_timestamp writeTs;

  int diffTs;

  while (written < toWrite)
  {
    writeTs = getTimestamp();

    int result = ::write(fd_, w_buffer_.data_.begin() + w_buffer_.start_ +
                             written, toWrite - written);

    diffTs = diffTimestamp(writeTs, getNewTimestamp());

    statistics -> addWriteTime(diffTs);

    if (result <= 0)
    {
      if (EGET() == EAGAIN)
      {
        #ifdef TEST
        *logofs << "Transport: Write of " << toWrite - written
                << " bytes on FD#" << fd_ << " would block.\n"
                << logofs_flush;
        #endif

        blocked_ = 1;

        break;
      }
      else if (EGET() == EINTR)
      {
        #ifdef TEST
        *logofs << "Transport: Write of " << toWrite - written
                << " bytes on FD#" << fd_ << " was interrupted.\n"
                << logofs_flush;
        #endif

        continue;
      }
      else
      {
        #ifdef TEST
        *logofs << "Transport: Write to " << "FD#" 
                << fd_ << " failed.\n" << logofs_flush;
        #endif

        finish();

        return -1;
      }
    }
    else
    {
      #ifdef TEST
      *logofs << "Transport: Flushed " << result << " bytes on " 
              << "FD#" << fd_ << ".\n" << logofs_flush;
      #endif

      written += result;
    }
  }

  if (written > 0)
  {
    #ifdef DUMP

    *logofs << "Transport: Dumping content of flushed data.\n"
            << logofs_flush;

    DumpData(w_buffer_.data_.begin() + w_buffer_.start_, written);

    #endif

    //
    // Update the buffer status.
    //

    w_buffer_.length_ -= written;

    if (w_buffer_.length_ == 0)
    {
      w_buffer_.start_ = 0;
    }
    else
    {
      w_buffer_.start_ += written;
    }
  }

  //
  // It can be that we wrote less bytes than
  // available because of the write limit.
  //

  if (w_buffer_.length_ > 0)
  {
    #ifdef TEST
    *logofs << "Transport: There are still " << w_buffer_.length_ 
            << " bytes in write buffer for " << "FD#" 
            << fd_ << ".\n" << logofs_flush;
    #endif

    blocked_ = 1;
  }

  #ifdef TEST
  *logofs << "Transport: Write buffer for FD#" << fd_
          << " has data for " << w_buffer_.length_ << " bytes.\n"
          << logofs_flush;

  *logofs << "Transport: Start is " << w_buffer_.start_ 
          << " length is " << w_buffer_.length_ << " size is "
          << w_buffer_.data_.size() << " capacity is "
          << w_buffer_.data_.capacity() << ".\n"
          << logofs_flush;
  #endif

  //
  // No new data was produced for the link except
  // any outstanding data from previous writes.
  //

  return 0;
}

int Transport::drain(int limit, int timeout)
{
  if (w_buffer_.length_ <= limit)
  {
    return 1;
  }

  //
  // Write the data accumulated in the write
  // buffer until it is below the limit or
  // the timeout is elapsed.
  //

  int toWrite = w_buffer_.length_;

  int written = 0;

  #ifdef TEST
  *logofs << "Transport: Draining " << toWrite - limit
          << " bytes on FD#" << fd_ << " with limit set to "
          << limit << ".\n" << logofs_flush;
  #endif

  T_timestamp startTs = getNewTimestamp();

  T_timestamp selectTs;
  T_timestamp writeTs;
  T_timestamp idleTs;

  T_timestamp nowTs = startTs;

  int diffTs;

  fd_set writeSet;
  fd_set readSet;

  FD_ZERO(&writeSet);
  FD_ZERO(&readSet);

  int result;
  int ready;

  while (w_buffer_.length_ - written > limit)
  {
    nowTs = getNewTimestamp();

    //
    // Wait for descriptor to become
    // readable or writable.
    //

    FD_SET(fd_, &writeSet);
    FD_SET(fd_, &readSet);

    setTimestamp(selectTs, timeout / 2);

    idleTs = nowTs;

    result = select(fd_ + 1, &readSet, &writeSet, NULL, &selectTs);

    nowTs = getNewTimestamp();

    diffTs = diffTimestamp(idleTs, nowTs);

    statistics -> addIdleTime(diffTs);

    statistics -> subReadTime(diffTs);

    if (result < 0)
    {
      if (EGET() == EINTR)
      {
        #ifdef TEST
        *logofs << "Transport: Select on FD#" << fd_
                << " was interrupted.\n" << logofs_flush;
        #endif

        continue;
      }
      else
      {
        #ifdef TEST
        *logofs << "Transport: Select on FD#" << fd_
                << " failed.\n" << logofs_flush;
        #endif

        finish();

        return -1;
      }
    }
    else if (result > 0)
    {
      ready = result;

      if (FD_ISSET(fd_, &writeSet))
      {
        writeTs = getNewTimestamp();

        result = ::write(fd_, w_buffer_.data_.begin() + w_buffer_.start_ +
                             written, toWrite - written);

        nowTs = getNewTimestamp();

        diffTs = diffTimestamp(writeTs, nowTs);

        statistics -> addWriteTime(diffTs);

        if (result > 0)
        {
          #ifdef TEST
          *logofs << "Transport: Forced flush of " << result
                  << " bytes on " << "FD#" << fd_ << ".\n"
                  << logofs_flush;
          #endif

          written += result;
        }
        else if (result < 0 && EGET() == EINTR)
        {
          #ifdef TEST
          *logofs << "Transport: Write to FD#" << fd_
                  << " was interrupted.\n" << logofs_flush;
          #endif

          continue;
        }
        else
        {
          #ifdef TEST
          *logofs << "Transport: Write to FD#" << fd_
                  << " failed.\n" << logofs_flush;
          #endif

          finish();

          return -1;
        }

        ready--;
      }

      if (ready > 0)
      {
        if (FD_ISSET(fd_, &readSet))
        {
          #ifdef TEST
          *logofs << "Transport: Not draining further "
                  << "due to data readable on FD#" << fd_
                  << ".\n" << logofs_flush;
          #endif

          break;
        }
      }
    }
    #ifdef TEST
    else
    {
      *logofs << "Transport: Timeout encountered "
              << "waiting for FD#" << fd_ << ".\n"
              << logofs_flush;
    }
    #endif

    nowTs = getNewTimestamp();

    diffTs = diffTimestamp(startTs, nowTs);

    if (diffTs >= timeout)
    {
      #ifdef TEST
      *logofs << "Transport: Not draining further "
              << "due to the timeout on FD#" << fd_
              << ".\n" << logofs_flush;
      #endif

      break;
    }
  }

  if (written > 0)
  {
    #ifdef DUMP

    *logofs << "Transport: Dumping content of flushed data.\n"
            << logofs_flush;

    DumpData(w_buffer_.data_.begin() + w_buffer_.start_, written);

    #endif

    //
    // Update the buffer status.
    //

    w_buffer_.length_ -= written;

    if (w_buffer_.length_ == 0)
    {
      w_buffer_.start_ = 0;

      blocked_ = 0;
    }
    else
    {
      w_buffer_.start_ += written;

      #ifdef TEST
      *logofs << "Transport: There are still " << w_buffer_.length_ 
              << " bytes in write buffer for " << "FD#" 
              << fd_ << ".\n" << logofs_flush;
      #endif

      blocked_ = 1;
    }
  }
  #ifdef TEST
  else
  {
    *logofs << "Transport: WARNING! No data written to FD#" << fd_
            << " with " << toWrite  << " bytes to drain and limit "
            << "set to " << limit << ".\n" << logofs_flush;
  }
  #endif

  #ifdef TEST
  *logofs << "Transport: Write buffer for FD#" << fd_
          << " has data for " << w_buffer_.length_ << " bytes.\n"
          << logofs_flush;

  *logofs << "Transport: Start is " << w_buffer_.start_ 
          << " length is " << w_buffer_.length_ << " size is "
          << w_buffer_.data_.size() << " capacity is "
          << w_buffer_.data_.capacity() << ".\n"
          << logofs_flush;
  #endif

  return (w_buffer_.length_ <= limit);
}

int Transport::wait(int timeout) const
{
  T_timestamp startTs = getNewTimestamp();

  T_timestamp idleTs;
  T_timestamp selectTs;

  T_timestamp nowTs = startTs;

  long available = 0;
  int  result    = 0;

  int diffTs;

  fd_set readSet;

  FD_ZERO(&readSet);
  FD_SET(fd_, &readSet);

  for (;;)
  {
    available = readable();

    diffTs = diffTimestamp(startTs, nowTs);

    if (available != 0 || timeout == 0 ||
            (diffTs + (timeout / 10)) >= timeout)
    {
      #ifdef TEST
      *logofs << "Transport: There are " << available
              << " bytes on FD#" << fd_ << " after "
              << diffTs << " Ms.\n" << logofs_flush;
      #endif

      return available;
    }
    else if (available == 0 && result > 0)
    {
      #ifdef TEST
      *logofs << "Transport: Read on " << "FD#" 
              << fd_ << " failed.\n" << logofs_flush;
      #endif

      return -1;
    }

    //
    // TODO: Should subtract the time
    // already spent in select.
    //

    selectTs.tv_sec  = 0;
    selectTs.tv_usec = timeout * 1000;

    idleTs = nowTs;

    //
    // Wait for descriptor to become readable.
    //

    result = select(fd_ + 1, &readSet, NULL, NULL, &selectTs);

    nowTs = getNewTimestamp();

    diffTs = diffTimestamp(idleTs, nowTs);

    statistics -> addIdleTime(diffTs);

    statistics -> subReadTime(diffTs);

    if (result < 0)
    {
      if (EGET() == EINTR)
      {
        #ifdef TEST
        *logofs << "Transport: Select on FD#" << fd_
                << " was interrupted.\n" << logofs_flush;
        #endif

        continue;
      }
      else
      {
        #ifdef TEST
        *logofs << "Transport: Select on " << "FD#" 
                << fd_ << " failed.\n" << logofs_flush;
        #endif

        return -1;
      }
    }
    #ifdef TEST
    else if (result == 0)
    {
      *logofs << "Transport: No data available on FD#" << fd_
              << " after " << diffTimestamp(startTs, nowTs)
              << " Ms.\n" << logofs_flush;
    }
    else
    {
      *logofs << "Transport: Data became available on FD#" << fd_
              << " after " << diffTimestamp(startTs, nowTs)
              << " Ms.\n" << logofs_flush;
    }
    #endif
  }
}

void Transport::setSize(unsigned int initialSize, unsigned int thresholdSize,
                            unsigned int maximumSize)
{
  initialSize_   = initialSize;
  thresholdSize_ = thresholdSize;
  maximumSize_   = maximumSize;

  #ifdef TEST
  *logofs << "Transport: Set buffer sizes for FD#" << fd_
          << " to " << initialSize_ << "/" << thresholdSize_
          << "/" << maximumSize_ << ".\n" << logofs_flush;
  #endif
}

void Transport::fullReset()
{
  blocked_ = 0;
  finish_  = 0;

  fullReset(w_buffer_);
}

int Transport::resize(T_buffer &buffer, const int &size)
{
  if ((int) buffer.data_.size() >= (buffer.length_ + size) &&
          (buffer.start_ + buffer.length_ + size) >
               (int) buffer.data_.size())
  {
    if (buffer.length_ > 0)
    {
      //
      // There is enough space in buffer but we need
      // to move existing data at the beginning.
      //

      #ifdef TEST
      *logofs << "Transport: Moving " << buffer.length_
              << " bytes of data for " << "FD#" << fd_
              << " to make room in the buffer.\n"
              << logofs_flush;
      #endif

      memmove(buffer.data_.begin(), buffer.data_.begin() +
                  buffer.start_, buffer.length_);
    }

    buffer.start_ = 0;

    #ifdef DEBUG
    *logofs << "Transport: Made room for " 
            << buffer.data_.size() - buffer.start_
            << " bytes in buffer for " << "FD#" 
            << fd_ << ".\n" << logofs_flush;
    #endif
  }
  else if ((buffer.length_ + size) > (int) buffer.data_.size())
  {
    //
    // Not enough space, so increase
    // the size of the buffer.
    //

    if (buffer.start_ != 0 && buffer.length_ > 0)
    {
      #ifdef TEST
      *logofs << "Transport: Moving " << buffer.length_
              << " bytes of data for " << "FD#" << fd_
              << " to resize the buffer.\n"
              << logofs_flush;
      #endif

      memmove(buffer.data_.begin(), buffer.data_.begin() +
                  buffer.start_, buffer.length_);
    }

    buffer.start_ = 0;

    unsigned int newSize = thresholdSize_;

    while (newSize < (unsigned int) buffer.length_ + size)
    {
      newSize <<= 1;

      if (newSize >= maximumSize_)
      {
        newSize = buffer.length_ + size + initialSize_;
      }
    }

    #ifdef DEBUG
    *logofs << "Transport: Buffer for " << "FD#" << fd_
            << " will be enlarged from " << buffer.data_.size()
            << " to at least " << buffer.length_ + size
            << " bytes.\n" << logofs_flush;
    #endif

    buffer.data_.resize(newSize);

    #ifdef TEST
    if (newSize >= maximumSize_)
    {
      *logofs << "Transport: WARNING! Buffer for FD#" << fd_
              << " grown to reach size of " << newSize
              << " bytes.\n" << logofs_flush;
    }
    #endif

    #ifdef TEST
    *logofs << "Transport: Data buffer for " << "FD#" 
            << fd_ << " has now size " << buffer.data_.size() 
            << " and capacity " << buffer.data_.capacity()
            << ".\n" << logofs_flush;
    #endif
  }

  return (buffer.length_ + size);
}

void Transport::fullReset(T_buffer &buffer)
{
  //
  // Force deallocation and allocation
  // of the initial size.
  //

  #ifdef TEST
  *logofs << "Transport: Resetting buffer for " << "FD#"
          << fd_ << " with size " << buffer.data_.size()
          << " and capacity " << buffer.data_.capacity()
          << ".\n" << logofs_flush;
  #endif

  buffer.start_  = 0;
  buffer.length_ = 0;

  if (buffer.data_.size() > (unsigned int) initialSize_ &&
          buffer.data_.capacity() > (unsigned int) initialSize_)
  {
    buffer.data_.clear();

    buffer.data_.resize(initialSize_);

    #ifdef TEST
    *logofs << "Transport: Data buffer for " << "FD#"
            << fd_ << " shrunk to size " << buffer.data_.size()
            << " and capacity " << buffer.data_.capacity()
            << ".\n" << logofs_flush;
    #endif
  }
}

ProxyTransport::ProxyTransport(int fd) : Transport(fd)
{
  #ifdef TEST
  *logofs << "ProxyTransport: Going to create proxy transport "
          << "for FD#" << fd_ << ".\n" << logofs_flush;
  #endif

  type_ = transport_proxy;

  //
  // Set up the read buffer.
  //

  r_buffer_.length_ = 0;
  r_buffer_.start_  = 0;

  r_buffer_.data_.resize(initialSize_);

  //
  // For now we own the buffer.
  //

  owner_ = 1;

  //
  // Set up ZLIB compression.
  //

  int result;

  r_stream_.zalloc = NULL;
  r_stream_.zfree  = NULL;
  r_stream_.opaque = NULL;

  r_stream_.next_in  = NULL;
  r_stream_.avail_in = 0;

  if ((result = inflateInit2(&r_stream_, 15)) != Z_OK)
  {
    #ifdef PANIC
    *logofs << "ProxyTransport: PANIC! Failed initialization of ZLIB read stream. "
            << "Error is '" << zError(result) << "'.\n" << logofs_flush;
    #endif

    cerr << "Error" << ": Failed initialization of ZLIB read stream. "
         << "Error is '" << zError(result) << "'.\n";

    HandleCleanup();
  }

  if (control -> LocalStreamCompression)
  {
    w_stream_.zalloc = NULL;
    w_stream_.zfree  = NULL;
    w_stream_.opaque = NULL;

    if ((result = deflateInit2(&w_stream_, control -> LocalStreamCompressionLevel, Z_DEFLATED,
                                   15, 9, Z_DEFAULT_STRATEGY)) != Z_OK)
    {
      #ifdef PANIC
      *logofs << "ProxyTransport: PANIC! Failed initialization of ZLIB write stream. "
              << "Error is '" << zError(result) << "'.\n" << logofs_flush;
      #endif

      cerr << "Error" << ": Failed initialization of ZLIB write stream. "
           << "Error is '" << zError(result) << "'.\n";

      HandleCleanup();
    }
  }

  //
  // No ZLIB stream to flush yet.
  //

  flush_ = 0;

  #ifdef REFERENCES
  *logofs << "ProxyTransport: Created new object at " 
          << this << " out of " << ++references_ 
          << " allocated references.\n" << logofs_flush;
  #endif
}

ProxyTransport::~ProxyTransport()
{
  #ifdef TEST
  *logofs << "ProxyTransport: Going to destroy derived class "
          << "for FD#" << fd_ << ".\n" << logofs_flush;
  #endif

  //
  // Deallocate the ZLIB stream state.
  //

  inflateEnd(&r_stream_);

  if (control -> LocalStreamCompression)
  {
    deflateEnd(&w_stream_);
  }

  #ifdef REFERENCES
  *logofs << "ProxyTransport: Deleted object at " 
          << this << " out of " << --references_ 
          << " allocated references.\n" << logofs_flush;
  #endif
}

//
// Read data from its file descriptor.
//

int ProxyTransport::read(unsigned char *data, unsigned int size)
{
  //
  // If the remote peer is not compressing
  // the stream then just return any byte
  // read from the socket.
  //

  if (control -> RemoteStreamCompression == 0)
  {
    int result = Transport::read(data, size);

    if (result <= 0)
    {
      return result;
    }

    statistics -> addBytesIn(result);

    return result;
  }

  //
  // Return any pending data first.
  //

  if (r_buffer_.length_ > 0)
  {
    //
    // If the size of the buffer doesn't
    // match the amount of data pending,
    // force the caller to retry.
    //

    if ((int) size < r_buffer_.length_)
    {
      #ifdef TEST
      *logofs << "ProxyTransport: WARNING! Forcing a retry with "
              << r_buffer_.length_ << " bytes pending and "
              << size << " in the buffer.\n"
              << logofs_flush;
      #endif

      ESET(EAGAIN);

      return -1;
    }

    int copied = (r_buffer_.length_ > ((int) size) ?
                      ((int) size) : r_buffer_.length_);

    memcpy(data, r_buffer_.data_.begin() + r_buffer_.start_, copied);

    //
    // Update the buffer status.
    //

    #ifdef DEBUG
    *logofs << "ProxyTransport: Going to immediately return " << copied
            << " bytes from proxy FD#" << fd_ << ".\n"
            << logofs_flush;
    #endif

    r_buffer_.length_ -= copied;

    if (r_buffer_.length_ == 0)
    {
      r_buffer_.start_ = 0;
    }
    else
    {
      r_buffer_.start_ += copied;

      #ifdef TEST
      *logofs << "ProxyTransport: There are still " << r_buffer_.length_ 
              << " bytes in read buffer for proxy " << "FD#" 
              << fd_ << ".\n" << logofs_flush;
        #endif
    }

    return copied;
  }

  //
  // Read data in the user buffer.
  //

  int result = Transport::read(data, size);

  if (result <= 0)
  {
    return result;
  }

  statistics -> addBytesIn(result);

  //
  // Decompress the data into the read
  // buffer.
  //

  #ifdef DEBUG
  *logofs << "ProxyTransport: Going to decompress data for " 
          << "proxy FD#" << fd_ << ".\n" << logofs_flush;
  #endif

  int saveTotalIn  = r_stream_.total_in;
  int saveTotalOut = r_stream_.total_out;

  int oldTotalIn  = saveTotalIn;
  int oldTotalOut = saveTotalOut;

  int diffTotalIn;
  int diffTotalOut;

  #ifdef INSPECT
  *logofs << "ProxyTransport: oldTotalIn = " << oldTotalIn
          << ".\n" << logofs_flush;
  #endif

  #ifdef INSPECT
  *logofs << "ProxyTransport: oldTotalOut = " << oldTotalOut
          << ".\n" << logofs_flush;
  #endif

  r_stream_.next_in  = (Bytef *) data;
  r_stream_.avail_in = result;

  //
  // Let ZLIB use all the space already
  // available in the buffer.
  //

  unsigned int newAvailOut = r_buffer_.data_.size() - r_buffer_.start_ -
                                 r_buffer_.length_;

  #ifdef TEST
  *logofs << "ProxyTransport: Initial decompress buffer is "
          << newAvailOut << " bytes.\n" << logofs_flush;
  #endif

  for (;;)
  {
    #ifdef INSPECT
    *logofs << "\nProxyTransport: Running the decompress loop.\n"
            << logofs_flush;
    #endif

    #ifdef INSPECT
    *logofs << "ProxyTransport: r_buffer_.length_ = " << r_buffer_.length_
            << ".\n" << logofs_flush;
    #endif

    #ifdef INSPECT
    *logofs << "ProxyTransport: r_buffer_.data_.size() = " << r_buffer_.data_.size()
            << ".\n" << logofs_flush;
    #endif

    #ifdef INSPECT
    *logofs << "ProxyTransport: newAvailOut = " << newAvailOut
            << ".\n" << logofs_flush;
    #endif

    if (resize(r_buffer_, newAvailOut) < 0)
    {
      return -1;
    }

    #ifdef INSPECT
    *logofs << "ProxyTransport: r_buffer_.data_.size() = "
            << r_buffer_.data_.size() << ".\n"
            << logofs_flush;
    #endif

    #ifdef INSPECT
    *logofs << "ProxyTransport: r_stream_.next_in = "
            << (void *) r_stream_.next_in << ".\n"
            << logofs_flush;
    #endif

    #ifdef INSPECT
    *logofs << "ProxyTransport: r_stream_.avail_in = "
            << r_stream_.avail_in << ".\n"
            << logofs_flush;
    #endif

    r_stream_.next_out  = r_buffer_.data_.begin() + r_buffer_.start_ +
                              r_buffer_.length_;

    r_stream_.avail_out = newAvailOut;

    #ifdef INSPECT
    *logofs << "ProxyTransport: r_stream_.next_out = "
            << (void *) r_stream_.next_out << ".\n"
            << logofs_flush;
    #endif

    #ifdef INSPECT
    *logofs << "ProxyTransport: r_stream_.avail_out = "
            << r_stream_.avail_out << ".\n"
            << logofs_flush;
    #endif

    int result = inflate(&r_stream_, Z_SYNC_FLUSH);

    #ifdef INSPECT
    *logofs << "ProxyTransport: Called inflate() result is "
            << result << ".\n" << logofs_flush;
    #endif

    #ifdef INSPECT
    *logofs << "ProxyTransport: r_stream_.avail_in = "
            << r_stream_.avail_in << ".\n"
            << logofs_flush;
    #endif

    #ifdef INSPECT
    *logofs << "ProxyTransport: r_stream_.avail_out = "
            << r_stream_.avail_out << ".\n"
            << logofs_flush;
    #endif

    #ifdef INSPECT
    *logofs << "ProxyTransport: r_stream_.total_in = "
            << r_stream_.total_in << ".\n"
            << logofs_flush;
    #endif

    #ifdef INSPECT
    *logofs << "ProxyTransport: r_stream_.total_out = "
            << r_stream_.total_out << ".\n"
            << logofs_flush;
    #endif

    diffTotalIn  = r_stream_.total_in  - oldTotalIn;
    diffTotalOut = r_stream_.total_out - oldTotalOut;

    #ifdef INSPECT
    *logofs << "ProxyTransport: diffTotalIn = "
            << diffTotalIn << ".\n"
            << logofs_flush;
    #endif

    #ifdef INSPECT
    *logofs << "ProxyTransport: diffTotalOut = "
            << diffTotalOut << ".\n"
            << logofs_flush;
    #endif

    #ifdef INSPECT
    *logofs << "ProxyTransport: r_buffer_.length_ = "
            << r_buffer_.length_ << ".\n"
            << logofs_flush;
    #endif

    r_buffer_.length_ += diffTotalOut;

    #ifdef INSPECT
    *logofs << "ProxyTransport: r_buffer_.length_ = "
            << r_buffer_.length_ << ".\n"
            << logofs_flush;
    #endif

    oldTotalIn  = r_stream_.total_in;
    oldTotalOut = r_stream_.total_out;

    if (result == Z_OK)
    {
      if (r_stream_.avail_in == 0 && r_stream_.avail_out > 0)
      {
        break;
      }
    }
    else if (result == Z_BUF_ERROR && r_stream_.avail_out > 0 &&
                 r_stream_.avail_in == 0)
    {
      #ifdef TEST
      *logofs << "ProxyTransport: WARNING! Raised Z_BUF_ERROR decompressing data.\n"
              << logofs_flush;
      #endif

      break;
    }
    else
    {
      #ifdef PANIC
      *logofs << "ProxyTransport: PANIC! Decompression of data failed. "
              << "Error is '" << zError(result) << "'.\n"
              << logofs_flush;
      #endif

      cerr << "Error" << ": Decompression of data failed. Error is '"
           << zError(result) << "'.\n";

      finish();

      return -1;
    }

    //
    // Add more bytes to the buffer.
    //

    if (newAvailOut < thresholdSize_)
    {
      newAvailOut = thresholdSize_;
    }

    #ifdef TEST
    *logofs << "ProxyTransport: Need to add " << newAvailOut
            << " bytes to the decompress buffer in read.\n"
            << logofs_flush;
    #endif
  }

  diffTotalIn  = r_stream_.total_in  - saveTotalIn;
  diffTotalOut = r_stream_.total_out - saveTotalOut;

  #ifdef DEBUG
  *logofs << "ProxyTransport: Decompressed data from "
          << diffTotalIn << " to " << diffTotalOut
          << " bytes.\n" << logofs_flush;
  #endif

  statistics -> addDecompressedBytes(diffTotalIn, diffTotalOut);

  //
  // Check if the size of the buffer
  // matches the produced data.
  //

  if ((int) size < r_buffer_.length_)
  {
    #ifdef TEST
    *logofs << "ProxyTransport: WARNING! Forcing a retry with "
            << r_buffer_.length_ << " bytes pending and "
            << size << " in the buffer.\n"
            << logofs_flush;
    #endif

    ESET(EAGAIN);

    return -1;
  }

  //
  // Copy the decompressed data to the
  // provided buffer.
  //

  int copied = (r_buffer_.length_ > ((int) size) ?
                    ((int) size) : r_buffer_.length_);

  #ifdef DEBUG
  *logofs << "ProxyTransport: Going to return " << copied
          << " bytes from proxy FD#" << fd_ << ".\n"
          << logofs_flush;
  #endif

  memcpy(data, r_buffer_.data_.begin() + r_buffer_.start_, copied);

  //
  // Update the buffer status.
  //

  r_buffer_.length_ -= copied;

  if (r_buffer_.length_ == 0)
  {
    r_buffer_.start_ = 0;
  }
  else
  {
    r_buffer_.start_ += copied;

    #ifdef TEST
    *logofs << "ProxyTransport: There are still " << r_buffer_.length_ 
            << " bytes in read buffer for proxy FD#" << fd_
            << ".\n" << logofs_flush;
    #endif
  }

  return copied;
}

//
// If required compress data, else write it to socket.
//

int ProxyTransport::write(T_write type, const unsigned char *data, const unsigned int size)
{
  #ifdef TEST
  if (size == 0)
  {
    *logofs << "ProxyTransport: WARNING! Write called for FD#" 
            << fd_ << " without any data to write.\n"
            << logofs_flush;

    return 0;
  }
  #endif

  //
  // If there is no compression revert to
  // plain socket management.
  //

  if (control -> LocalStreamCompression == 0)
  {
    int result = Transport::write(type, data, size);

    if (result <= 0)
    {
      return result;
    }

    statistics -> addBytesOut(result);

    statistics -> updateBitrate(result);

    FlushCallback(result);

    return result;
  }

  #ifdef DEBUG
  *logofs << "ProxyTransport: Going to compress " << size 
          << " bytes to write buffer for proxy FD#" << fd_
          << ".\n" << logofs_flush;
  #endif

  //
  // Compress data into the write buffer.
  //

  int saveTotalIn  = w_stream_.total_in;
  int saveTotalOut = w_stream_.total_out;

  int oldTotalIn  = saveTotalIn;
  int oldTotalOut = saveTotalOut;

  int diffTotalIn;
  int diffTotalOut;

  #ifdef INSPECT
  *logofs << "ProxyTransport: oldTotalIn = " << oldTotalIn
          << ".\n" << logofs_flush;
  #endif

  #ifdef INSPECT
  *logofs << "ProxyTransport: oldTotalOut = " << oldTotalOut
          << ".\n" << logofs_flush;
  #endif

  w_stream_.next_in  = (Bytef *) data;
  w_stream_.avail_in = size;

  //
  // Let ZLIB use all the space already
  // available in the buffer.
  //

  unsigned int newAvailOut = w_buffer_.data_.size() - w_buffer_.start_ -
                                 w_buffer_.length_;

  #ifdef TEST
  *logofs << "ProxyTransport: Initial compress buffer is "
          << newAvailOut << " bytes.\n" << logofs_flush;
  #endif

  for (;;)
  {
    #ifdef INSPECT
    *logofs << "\nProxyTransport: Running the compress loop.\n"
            << logofs_flush;
    #endif

    #ifdef INSPECT
    *logofs << "ProxyTransport: w_buffer_.length_ = "
            << w_buffer_.length_ << ".\n"
            << logofs_flush;
    #endif

    #ifdef INSPECT
    *logofs << "ProxyTransport: w_buffer_.data_.size() = "
            << w_buffer_.data_.size() << ".\n"
            << logofs_flush;
    #endif

    #ifdef INSPECT
    *logofs << "ProxyTransport: newAvailOut = "
            << newAvailOut << ".\n"
            << logofs_flush;
    #endif

    if (resize(w_buffer_, newAvailOut) < 0)
    {
      return -1;
    }

    #ifdef INSPECT
    *logofs << "ProxyTransport: w_buffer_.data_.size() = "
            << w_buffer_.data_.size() << ".\n"
            << logofs_flush;
    #endif

    #ifdef INSPECT
    *logofs << "ProxyTransport: w_stream_.next_in = "
            << (void *) w_stream_.next_in << ".\n"
            << logofs_flush;
    #endif

    #ifdef INSPECT
    *logofs << "ProxyTransport: w_stream_.avail_in = "
            << w_stream_.avail_in << ".\n"
            << logofs_flush;
    #endif

    w_stream_.next_out  = w_buffer_.data_.begin() + w_buffer_.start_ +
                              w_buffer_.length_;

    w_stream_.avail_out = newAvailOut;

    #ifdef INSPECT
    *logofs << "ProxyTransport: w_stream_.next_out = "
            << (void *) w_stream_.next_out << ".\n"
            << logofs_flush;
    #endif

    #ifdef INSPECT
    *logofs << "ProxyTransport: w_stream_.avail_out = "
            << w_stream_.avail_out << ".\n"
            << logofs_flush;
    #endif

    int result = deflate(&w_stream_, (type == write_delayed ?
                             Z_NO_FLUSH : Z_SYNC_FLUSH));

    #ifdef INSPECT
    *logofs << "ProxyTransport: Called deflate() result is "
            << result << ".\n" << logofs_flush;
    #endif

    #ifdef INSPECT
    *logofs << "ProxyTransport: w_stream_.avail_in = "
            << w_stream_.avail_in << ".\n"
            << logofs_flush;
    #endif

    #ifdef INSPECT
    *logofs << "ProxyTransport: w_stream_.avail_out = "
            << w_stream_.avail_out << ".\n"
            << logofs_flush;
    #endif

    #ifdef INSPECT
    *logofs << "ProxyTransport: w_stream_.total_in = "
            << w_stream_.total_in << ".\n"
            << logofs_flush;
    #endif

    #ifdef INSPECT
    *logofs << "ProxyTransport: w_stream_.total_out = "
            << w_stream_.total_out << ".\n"
            << logofs_flush;
    #endif

    diffTotalOut = w_stream_.total_out - oldTotalOut;
    diffTotalIn  = w_stream_.total_in  - oldTotalIn;

    #ifdef INSPECT
    *logofs << "ProxyTransport: diffTotalIn = "
            << diffTotalIn << ".\n"
            << logofs_flush;
    #endif

    #ifdef INSPECT
    *logofs << "ProxyTransport: diffTotalOut = "
            << diffTotalOut << ".\n"
            << logofs_flush;
    #endif

    #ifdef INSPECT
    *logofs << "ProxyTransport: w_buffer_.length_ = "
            << w_buffer_.length_ << ".\n"
            << logofs_flush;
    #endif

    w_buffer_.length_ += diffTotalOut;

    #ifdef INSPECT
    *logofs << "ProxyTransport: w_buffer_.length_ = "
            << w_buffer_.length_ << ".\n"
            << logofs_flush;
    #endif

    oldTotalOut = w_stream_.total_out;
    oldTotalIn  = w_stream_.total_in;

    if (result == Z_OK)
    {
      if (w_stream_.avail_in == 0 && w_stream_.avail_out > 0)
      {
        break;
      }
    }
    else if (result == Z_BUF_ERROR && w_stream_.avail_out > 0 &&
                 w_stream_.avail_in == 0)
    {
      #ifdef TEST
      *logofs << "ProxyTransport: WARNING! Raised Z_BUF_ERROR compressing data.\n"
              << logofs_flush;
      #endif

      break;
    }
    else
    {
      #ifdef PANIC
      *logofs << "ProxyTransport: PANIC! Compression of data failed. "
              << "Error is '" << zError(result) << "'.\n"
              << logofs_flush;
      #endif

      cerr << "Error" << ": Compression of data failed. Error is '"
           << zError(result) << "'.\n";

      finish();

      return -1;
    }

    //
    // Add more bytes to the buffer.
    //

    if (newAvailOut < thresholdSize_)
    {
      newAvailOut = thresholdSize_;
    }

    #ifdef TEST
    *logofs << "ProxyTransport: Need to add " << newAvailOut
            << " bytes to the compress buffer in write.\n"
            << logofs_flush;
    #endif
  }

  diffTotalIn  = w_stream_.total_in  - saveTotalIn;
  diffTotalOut = w_stream_.total_out - saveTotalOut;

  #ifdef TEST

  *logofs << "ProxyTransport: Compressed data from "
          << diffTotalIn << " to " << diffTotalOut
          << " bytes.\n" << logofs_flush;

  if (diffTotalIn != (int) size)
  {
    #ifdef PANIC
    *logofs << "ProxyTransport: PANIC! Bytes provided to ZLIB stream "
            << "should be " << size << " but they look to be "
            << diffTotalIn << ".\n" << logofs_flush;
    #endif
  }

  #endif

  //
  // Find out what we have to do with the
  // produced data.
  //

  if (type == write_immediate)
  {
    //
    // If user requested an immediate write we
    // flushed the ZLIB buffer. We can now reset
    // the counter and write data to socket.
    //

    flush_ = 0;

    #ifdef TEST
    *logofs << "ProxyTransport: Write buffer for proxy FD#" << fd_
            << " has data for " << w_buffer_.length_ << " bytes.\n"
            << logofs_flush;

    *logofs << "ProxyTransport: Start is " << w_buffer_.start_ 
            << " length is " << w_buffer_.length_ << " flush is "
            << flush_ << " size is " << w_buffer_.data_.size()
            << " capacity is " << w_buffer_.data_.capacity()
            << ".\n" << logofs_flush;
    #endif

    //
    // Alternatively may try to write only if
    // the socket is not blocked.
    //
    // if (w_buffer_.length_ > 0 && blocked_ == 0)
    // {
    //   ...
    // }
    //

    if (w_buffer_.length_ > 0)

    {
      #ifdef TEST
      *logofs << "ProxyTransport: Writing " << w_buffer_.length_
              << " bytes of produced data to FD#" << fd_ << ".\n"
              << logofs_flush;
      #endif

      int result = Transport::flush();

      if (result < 0)
      {
        return -1;
      }
    }
  }
  else
  {
    //
    // We haven't flushed the ZLIB compression
    // buffer, so user will have to call proxy
    // transport's flush explicitly.
    //

    flush_ += diffTotalIn;
  }

  //
  // We either wrote the data or added it to the
  // write buffer. It's convenient to update the
  // counters at this stage to get the current
  // bitrate earlier.
  //

  statistics -> addCompressedBytes(diffTotalIn, diffTotalOut);

  statistics -> addBytesOut(diffTotalOut);

  statistics -> updateBitrate(diffTotalOut);

  FlushCallback(diffTotalOut);

  #ifdef TEST
  *logofs << "ProxyTransport: Write buffer for proxy FD#" << fd_
          << " has data for " << w_buffer_.length_ << " bytes.\n"
          << logofs_flush;

  *logofs << "ProxyTransport: Start is " << w_buffer_.start_ 
          << " length is " << w_buffer_.length_ << " flush is "
          << flush_ << " size is " << w_buffer_.data_.size()
          << " capacity is " << w_buffer_.data_.capacity()
          << ".\n" << logofs_flush;
  #endif

  return size;
}

//
// Write data to its file descriptor.
//

int ProxyTransport::flush()
{
  //
  // If there is no compression or we already compressed
  // outgoing data and just need to write it to socket
  // because of previous incomplete writes then revert
  // to plain socket management.
  //

  if (flush_ == 0 || control -> LocalStreamCompression == 0)
  {
    int result = Transport::flush();

    if (result < 0)
    {
      return -1;
    }

    return result;
  }

  #ifdef DEBUG
  *logofs << "ProxyTransport: Going to flush compression on "
          << "proxy FD#" << fd_ << ".\n" << logofs_flush;
  #endif

  #ifdef TEST
  *logofs << "ProxyTransport: Flush counter for proxy FD#" << fd_
          << " is " << flush_ << " bytes.\n" << logofs_flush;
  #endif

  //
  // Flush ZLIB stream into the write buffer.
  //

  int saveTotalIn  = w_stream_.total_in;
  int saveTotalOut = w_stream_.total_out;

  int oldTotalIn  = saveTotalIn;
  int oldTotalOut = saveTotalOut;

  int diffTotalOut;
  int diffTotalIn;

  #ifdef INSPECT
  *logofs << "ProxyTransport: oldTotalIn = " << oldTotalIn
          << ".\n" << logofs_flush;
  #endif

  #ifdef INSPECT
  *logofs << "ProxyTransport: oldTotalOut = " << oldTotalOut
          << ".\n" << logofs_flush;
  #endif

  w_stream_.next_in  = w_buffer_.data_.begin() + w_buffer_.start_ + w_buffer_.length_;
  w_stream_.avail_in = 0;

  //
  // Let ZLIB use all the space already
  // available in the buffer.
  //

  unsigned int newAvailOut = w_buffer_.data_.size() - w_buffer_.start_ -
                                 w_buffer_.length_;

  #ifdef DEBUG
  *logofs << "ProxyTransport: Initial flush buffer is "
          << newAvailOut << " bytes.\n" << logofs_flush;
  #endif

  for (;;)
  {
    #ifdef INSPECT
    *logofs << "\nProxyTransport: Running the flush loop.\n"
            << logofs_flush;
    #endif

    #ifdef INSPECT
    *logofs << "ProxyTransport: w_buffer_.length_ = "
            << w_buffer_.length_ << ".\n"
            << logofs_flush;
    #endif

    #ifdef INSPECT
    *logofs << "ProxyTransport: w_buffer_.data_.size() = "
            << w_buffer_.data_.size() << ".\n"
            << logofs_flush;
    #endif

    #ifdef INSPECT
    *logofs << "ProxyTransport: newAvailOut = "
            << newAvailOut << ".\n"
            << logofs_flush;
    #endif

    if (resize(w_buffer_, newAvailOut) < 0)
    {
      return -1;
    }

    #ifdef INSPECT
    *logofs << "ProxyTransport: w_buffer_.data_.size() = "
            << w_buffer_.data_.size() << ".\n"
            << logofs_flush;
    #endif

    #ifdef INSPECT
    *logofs << "ProxyTransport: w_stream_.next_in = "
            << (void *) w_stream_.next_in << ".\n"
            << logofs_flush;
    #endif

    #ifdef INSPECT
    *logofs << "ProxyTransport: w_stream_.avail_in = "
            << w_stream_.avail_in << ".\n"
            << logofs_flush;
    #endif

    w_stream_.next_out  = w_buffer_.data_.begin() + w_buffer_.start_ +
                              w_buffer_.length_;

    w_stream_.avail_out = newAvailOut;

    #ifdef INSPECT
    *logofs << "ProxyTransport: w_stream_.next_out = "
            << (void *) w_stream_.next_out << ".\n"
            << logofs_flush;
    #endif

    #ifdef INSPECT
    *logofs << "ProxyTransport: w_stream_.avail_out = "
            << w_stream_.avail_out << ".\n"
            << logofs_flush;
    #endif

    int result = deflate(&w_stream_, Z_SYNC_FLUSH);

    #ifdef INSPECT
    *logofs << "ProxyTransport: Called deflate() result is "
            << result << ".\n" << logofs_flush;
    #endif

    #ifdef INSPECT
    *logofs << "ProxyTransport: w_stream_.avail_in = "
            << w_stream_.avail_in << ".\n"
            << logofs_flush;
    #endif

    #ifdef INSPECT
    *logofs << "ProxyTransport: w_stream_.avail_out = "
            << w_stream_.avail_out << ".\n"
            << logofs_flush;
    #endif

    #ifdef INSPECT
    *logofs << "ProxyTransport: w_stream_.total_in = "
            << w_stream_.total_in << ".\n"
            << logofs_flush;
    #endif

    #ifdef INSPECT
    *logofs << "ProxyTransport: w_stream_.total_out = "
            << w_stream_.total_out << ".\n"
            << logofs_flush;
    #endif

    diffTotalOut = w_stream_.total_out - oldTotalOut;
    diffTotalIn  = w_stream_.total_in  - oldTotalIn;

    #ifdef INSPECT
    *logofs << "ProxyTransport: diffTotalIn = "
            << diffTotalIn << ".\n"
            << logofs_flush;
    #endif

    #ifdef INSPECT
    *logofs << "ProxyTransport: diffTotalOut = "
            << diffTotalOut << ".\n"
            << logofs_flush;
    #endif

    #ifdef INSPECT
    *logofs << "ProxyTransport: w_buffer_.length_ = "
            << w_buffer_.length_ << ".\n"
            << logofs_flush;
    #endif

    w_buffer_.length_ += diffTotalOut;

    #ifdef INSPECT
    *logofs << "ProxyTransport: w_buffer_.length_ = "
            << w_buffer_.length_ << ".\n"
            << logofs_flush;
    #endif

    oldTotalOut = w_stream_.total_out;
    oldTotalIn  = w_stream_.total_in;

    if (result == Z_OK)
    {
      if (w_stream_.avail_in == 0 && w_stream_.avail_out > 0)
      {
        break;
      }
    }
    else if (result == Z_BUF_ERROR && w_stream_.avail_out > 0 &&
                 w_stream_.avail_in == 0)
    {
      #ifdef TEST
      *logofs << "ProxyTransport: WARNING! Raised Z_BUF_ERROR flushing data.\n"
              << logofs_flush;
      #endif

      break;
    }
    else
    {
      #ifdef PANIC
      *logofs << "ProxyTransport: PANIC! Flush of compressed data failed. "
              << "Error is '" << zError(result) << "'.\n"
              << logofs_flush;
      #endif

      cerr << "Error" << ": Flush of compressed data failed. Error is '"
           << zError(result) << "'.\n";

      finish();

      return -1;
    }

    //
    // Add more bytes to the buffer.
    //

    if (newAvailOut < thresholdSize_)
    {
      newAvailOut = thresholdSize_;
    }

    #ifdef TEST
    *logofs << "ProxyTransport: Need to add " << newAvailOut
            << " bytes to the compress buffer in flush.\n"
            << logofs_flush;
    #endif
  }

  diffTotalIn  = w_stream_.total_in  - saveTotalIn;
  diffTotalOut = w_stream_.total_out - saveTotalOut;

  #ifdef TEST
  *logofs << "ProxyTransport: Compressed flush data from "
          << diffTotalIn << " to " << diffTotalOut
          << " bytes.\n" << logofs_flush;
  #endif

  //
  // Time to move data from the write
  // buffer to the real link. 
  //

  #ifdef DEBUG
  *logofs << "ProxyTransport: Reset flush counter for proxy FD#"
          << fd_ << ".\n" << logofs_flush;
  #endif

  flush_ = 0;

  #ifdef TEST
  *logofs << "ProxyTransport: Write buffer for proxy FD#" << fd_
          << " has data for " << w_buffer_.length_ << " bytes.\n"
          << logofs_flush;

  *logofs << "ProxyTransport: Start is " << w_buffer_.start_ 
          << " length is " << w_buffer_.length_ << " flush is "
          << flush_ << " size is " << w_buffer_.data_.size()
          << " capacity is " << w_buffer_.data_.capacity()
          << ".\n" << logofs_flush;
  #endif

  int result = Transport::flush();

  if (result < 0)
  {
    return -1;
  }

  //
  // Update all the counters.
  //

  statistics -> addCompressedBytes(diffTotalIn, diffTotalOut);

  statistics -> addBytesOut(diffTotalOut);

  statistics -> updateBitrate(diffTotalOut);

  FlushCallback(diffTotalOut);

  return result;
}

unsigned int ProxyTransport::getPending(unsigned char *&data)
{
  //
  // Return a pointer to the data in the
  // read buffer. It is up to the caller
  // to ensure that the data is consumed
  // before the read buffer is reused.
  //

  if (r_buffer_.length_ > 0)
  {
    unsigned int size = r_buffer_.length_;

    data = r_buffer_.data_.begin() + r_buffer_.start_;

    #ifdef DEBUG
    *logofs << "ProxyTransport: Returning " << size
            << " pending bytes from proxy FD#" << fd_
            << ".\n" << logofs_flush;
    #endif

    r_buffer_.length_ = 0;
    r_buffer_.start_  = 0;

    //
    // Prevent the deletion of the buffer.
    //

    owner_ = 0;

    return size;
  }

  #ifdef TEST
  *logofs << "ProxyTransport: WARNING! No pending data "
          << "for proxy FD#" << fd_ << ".\n"
          << logofs_flush;
  #endif

  data = NULL;

  return 0;
}

void ProxyTransport::fullReset()
{
  blocked_ = 0;
  finish_  = 0;
  flush_   = 0;

  if (control -> RemoteStreamCompression)
  {
    inflateReset(&r_stream_);
  }

  if (control -> LocalStreamCompression)
  {
    deflateReset(&w_stream_);
  }

  if (owner_ == 1)
  {
    Transport::fullReset(r_buffer_);
  }

  Transport::fullReset(w_buffer_);
}

AgentTransport::AgentTransport(int fd) : Transport(fd)
{
  #ifdef TEST
  *logofs << "AgentTransport: Going to create agent transport "
          << "for FD#" << fd_ << ".\n" << logofs_flush;
  #endif

  type_ = transport_agent;

  //
  // Set up the read buffer.
  //

  r_buffer_.length_ = 0;
  r_buffer_.start_  = 0;

  r_buffer_.data_.resize(initialSize_);

  //
  // For now we own the buffer.
  //

  owner_ = 1;

  //
  // Set up the mutexes.
  //

  #ifdef THREADS

  pthread_mutexattr_t m_attributes;

  pthread_mutexattr_init(&m_attributes);

  //
  // Interfaces in pthread to handle mutex
  // type do not work in current version. 
  //

  m_attributes.__mutexkind = PTHREAD_MUTEX_ERRORCHECK_NP;

  if (pthread_mutex_init(&m_read_, &m_attributes) != 0)
  {
    #ifdef TEST
    *logofs << "AgentTransport: Child: Creation of read mutex failed. "
            << "Error is " << EGET() << " '" << ESTR()
            << "'.\n" << logofs_flush;
    #endif
  }

  if (pthread_mutex_init(&m_write_, &m_attributes) != 0)
  {
    #ifdef TEST
    *logofs << "AgentTransport: Child: Creation of write mutex failed. "
            << "Error is " << EGET() << " '" << ESTR()
            << "'.\n" << logofs_flush;
    #endif
  }

  #endif

  #ifdef REFERENCES
  *logofs << "AgentTransport: Child: Created new object at " 
          << this << " out of " << ++references_ 
          << " allocated references.\n" << logofs_flush;
  #endif
}

AgentTransport::~AgentTransport()
{
  #ifdef TEST
  *logofs << "AgentTransport: Going to destroy derived class "
          << "for FD#" << fd_ << ".\n" << logofs_flush;
  #endif

  //
  // Unlock and free all mutexes.
  //

  #ifdef THREADS

  pthread_mutex_unlock(&m_read_);
  pthread_mutex_unlock(&m_write_);

  pthread_mutex_destroy(&m_read_);
  pthread_mutex_destroy(&m_write_);

  #endif

  #ifdef REFERENCES
  *logofs << "AgentTransport: Child: Deleted object at " 
          << this << " out of " << --references_ 
          << " allocated references.\n" << logofs_flush;
  #endif
}

//
// Read data enqueued by the other thread.
//

int AgentTransport::read(unsigned char *data, unsigned int size)
{
  #ifdef THREADS

  lockRead();

  #endif

  #ifdef DEBUG
  *logofs << "AgentTransport: Child: Going to read " << size
          << " bytes from " << "FD#" << fd_ << ".\n"
          << logofs_flush;
  #endif

  int copied = -1;

  if (r_buffer_.length_ > 0)
  {
    if ((int) size < r_buffer_.length_)
    {
      #ifdef TEST
      *logofs << "AgentTransport: WARNING! Forcing a retry with "
              << r_buffer_.length_ << " bytes pending and "
              << size << " in the buffer.\n"
              << logofs_flush;
      #endif

      ESET(EAGAIN);
    }
    else
    {
      copied = (r_buffer_.length_ > ((int) size) ?
                    ((int) size) : r_buffer_.length_);

      memcpy(data, r_buffer_.data_.begin() + r_buffer_.start_, copied);

      //
      // Update the buffer status.
      //

      #ifdef TEST
      *logofs << "AgentTransport: Child: Going to immediately return "
              << copied << " bytes from FD#" << fd_ << ".\n"
              << logofs_flush;
      #endif

      #ifdef DUMP

      *logofs << "AgentTransport: Child: Dumping content of read data.\n"
              << logofs_flush;

      DumpData(data, copied);

      #endif

      r_buffer_.length_ -= copied;

      if (r_buffer_.length_ == 0)
      {
        r_buffer_.start_ = 0;
      }
      else
      {
        r_buffer_.start_ += copied;

        #ifdef TEST
        *logofs << "AgentTransport: Child: There are still "
                << r_buffer_.length_  << " bytes in read buffer for "
                << "FD#" << fd_ << ".\n" << logofs_flush;
        #endif
      }
    }
  }
  else
  {
    #ifdef DEBUG
    *logofs << "AgentTransport: Child: No data can be got "
            << "from read buffer for FD#" << fd_ << ".\n"
            << logofs_flush;
    #endif

    ESET(EAGAIN);
  }

  #ifdef THREADS

  unlockRead();

  #endif

  return copied;
}

//
// Write data to buffer so that the other
// thread can get it.
//

int AgentTransport::write(T_write type, const unsigned char *data, const unsigned int size)
{
  #ifdef THREADS

  lockWrite();

  #endif

  //
  // Just append data to socket's write buffer.
  // Note that we don't care if buffer exceeds
  // the size limits set for this type of
  // transport.
  //

  #ifdef TEST
  *logofs << "AgentTransport: Child: Going to append " << size
          << " bytes to write buffer for " << "FD#" << fd_
          << ".\n" << logofs_flush;
  #endif

  int copied = -1;

  if (resize(w_buffer_, size) < 0)
  {
    finish();

    ESET(EPIPE);
  }
  else
  {
    memmove(w_buffer_.data_.begin() + w_buffer_.start_ + w_buffer_.length_, data, size);

    w_buffer_.length_ += size;

    #ifdef DUMP

    *logofs << "AgentTransport: Child: Dumping content of written data.\n"
            << logofs_flush;

    DumpData(data, size);

    #endif

    #ifdef TEST
    *logofs << "AgentTransport: Child: Write buffer for FD#" << fd_
            << " has data for " << w_buffer_.length_ << " bytes.\n"
            << logofs_flush;

    *logofs << "AgentTransport: Child: Start is " << w_buffer_.start_ 
            << " length is " << w_buffer_.length_ << " size is "
            << w_buffer_.data_.size() << " capacity is "
            << w_buffer_.data_.capacity() << ".\n"
            << logofs_flush;
    #endif

    copied = size;
  }

  //
  // Let child access again the read buffer.
  //

  #ifdef THREADS

  unlockWrite();

  #endif

  return copied;
}

int AgentTransport::flush()
{
  //
  // In case of memory-to-memory transport
  // this function should never be called.
  //

  #ifdef PANIC
  *logofs << "AgentTransport: Child: PANIC! Called flush() for "
          << "memory to memory transport on " << "FD#"
          << fd_ << ".\n" << logofs_flush;
  #endif

  cerr << "Error" << ": Called flush() for "
       << "memory to memory transport on " << "FD#"
       << fd_ << ".\n";

  HandleAbort();
}

int AgentTransport::drain(int limit, int timeout)
{
  //
  // We can't drain the channel in the case
  // of the memory-to-memory transport. Data
  // is enqueued for the agent to read but
  // the agent could require multiple loops
  // to read it all.
  //

  //
  // In case of memory-to-memory transport
  // this function should never be called.
  //

  #ifdef PANIC
  *logofs << "AgentTransport: Child: PANIC! Called drain() for "
          << "memory to memory transport on " << "FD#"
          << fd_ << ".\n" << logofs_flush;
  #endif

  cerr << "Error" << ": Called drain() for "
       << "memory to memory transport on " << "FD#"
       << fd_ << ".\n";

  HandleAbort();
}

unsigned int AgentTransport::getPending(unsigned char *&data)
{
  #ifdef THREADS

  lockRead();

  #endif

  if (r_buffer_.length_ > 0)
  {
    unsigned int size = r_buffer_.length_;

    data = r_buffer_.data_.begin() + r_buffer_.start_;

    #ifdef DEBUG
    *logofs << "AgentTransport: Child: Returning " << size
            << " pending bytes from FD#" << fd_
            << ".\n" << logofs_flush;
    #endif

    r_buffer_.length_ = 0;
    r_buffer_.start_  = 0;

    #ifdef THREADS

    unlockRead();

    #endif

    //
    // Prevent the deletion of the buffer.
    //

    owner_ = 0;

    return size;
  }

  #ifdef TEST
  *logofs << "AgentTransport: WARNING! No pending data "
          << "for FD#" << fd_ << ".\n" << logofs_flush;
  #endif

  #ifdef THREADS

  unlockRead();

  #endif

  data = NULL;

  return 0;
}

void AgentTransport::fullReset()
{
  #ifdef THREADS

  lockRead();
  lockWrite();

  #endif

  #ifdef TEST
  *logofs << "AgentTransport: Child: Resetting transport "
          << "for FD#" << fd_ << ".\n" << logofs_flush;
  #endif

  blocked_ = 0;
  finish_  = 0;

  if (owner_ == 1)
  {
    Transport::fullReset(r_buffer_);
  }

  Transport::fullReset(w_buffer_);
}

int AgentTransport::enqueue(const char *data, const int size)
{
  #ifdef THREADS

  lockRead();

  #endif

  if (finish_ == 1)
  {
    #if defined(PARENT) && defined(TEST)
    *logofs << "AgentTransport: Parent: Returning EPIPE in "
            << "write for finishing FD#" << fd_ << ".\n"
           << logofs_flush;
    #endif

    ESET(EPIPE);

    return -1;
  }

  //
  // Always allow the agent to write
  // all its data.
  //

  int toPut = size;

  #if defined(PARENT) && defined(TEST)
  *logofs << "AgentTransport: Parent: Going to put " << toPut
          << " bytes into read buffer for FD#" << fd_
          << ". Buffer length is " << r_buffer_.length_
          << ".\n" << logofs_flush;
  #endif

  if (resize(r_buffer_, toPut) < 0)
  {
    finish();

    #ifdef THREADS

    unlockRead();

    #endif

    return -1;
  }

  memcpy(r_buffer_.data_.begin() + r_buffer_.start_ + r_buffer_.length_, data, toPut);

  r_buffer_.length_ += toPut;

  #if defined(DUMP) && defined(PARENT)

  *logofs << "AgentTransport: Parent: Dumping content of enqueued data.\n"
          << logofs_flush;

  DumpData((const unsigned char *) data, toPut);

  #endif

  #if defined(PARENT) && defined(TEST)
  *logofs << "AgentTransport: Parent: Read buffer for FD#" << fd_
          << " has now data for " << r_buffer_.length_
          << " bytes.\n" << logofs_flush;

  *logofs << "AgentTransport: Parent: Start is " << r_buffer_.start_ 
          << " length is " << r_buffer_.length_ << " size is "
          << r_buffer_.data_.size() << " capacity is "
          << r_buffer_.data_.capacity() << ".\n"
          << logofs_flush;
  #endif

  #ifdef THREADS

  unlockRead();

  #endif

  return toPut;
}

int AgentTransport::dequeue(char *data, int size)
{
  #ifdef THREADS

  lockWrite();

  #endif

  if (w_buffer_.length_ == 0)
  {
    if (finish_ == 1)
    {
      #if defined(PARENT) && defined(TEST)
      *logofs << "AgentTransport: Parent: Returning 0 in read "
              << "for finishing FD#" << fd_ << ".\n"
              << logofs_flush;
      #endif

      return 0;
    }

    #if defined(PARENT) && defined(TEST)
    *logofs << "AgentTransport: Parent: No data can be read "
            << "from write buffer for FD#" << fd_ << ".\n"
            << logofs_flush;
    #endif

    ESET(EAGAIN);

    #ifdef THREADS

    unlockWrite();

    #endif

    return -1;
  }

  //
  // Return as many bytes as possible.
  //

  int toGet = ((int) size > w_buffer_.length_ ? w_buffer_.length_ : size);

  #if defined(PARENT) && defined(TEST)
  *logofs << "AgentTransport: Parent: Going to get " << toGet
          << " bytes from write buffer for FD#" << fd_ << ".\n"
          << logofs_flush;
  #endif

  memcpy(data, w_buffer_.data_.begin() + w_buffer_.start_, toGet);

  w_buffer_.start_  += toGet;
  w_buffer_.length_ -= toGet;

  #if defined(DUMP) && defined(PARENT)

  *logofs << "AgentTransport: Parent: Dumping content of dequeued data.\n"
          << logofs_flush;

  DumpData((const unsigned char *) data, toGet);

  #endif

  #if defined(PARENT) && defined(TEST)
  *logofs << "AgentTransport: Parent: Write buffer for FD#" << fd_
          << " has now data for " << length() << " bytes.\n"
          << logofs_flush;

  *logofs << "AgentTransport: Parent: Start is " << w_buffer_.start_ 
          << " length is " << w_buffer_.length_ << " size is "
          << w_buffer_.data_.size() << " capacity is "
          << w_buffer_.data_.capacity() << ".\n"
          << logofs_flush;
  #endif

  #ifdef THREADS

  unlockWrite();

  #endif

  return toGet;
}

int AgentTransport::dequeuable()
{
  if (finish_ == 1)
  {
    #if defined(PARENT) && defined(TEST)
    *logofs << "AgentTransport: Parent: Returning EPIPE in "
            << "readable for finishing FD#" << fd_
            << ".\n" << logofs_flush;
    #endif

    ESET(EPIPE);

    return -1;
  }

  #if defined(PARENT) && defined(TEST)
  *logofs << "AgentTransport: Parent: Returning "
          << w_buffer_.length_ << " as data readable "
          << "from read buffer for FD#" << fd_ << ".\n"
          << logofs_flush;
  #endif

  return w_buffer_.length_;
}

#ifdef THREADS

int AgentTransport::lockRead()
{
  for (;;)
  {
    int result = pthread_mutex_lock(&m_read_);

    if (result == 0)
    {
      #ifdef DEBUG
      *logofs << "AgentTransport: Read mutex locked by thread id "
              << pthread_self() << ".\n" << logofs_flush;
      #endif

      return 0;
    }
    else if (EGET() == EINTR)
    {
      continue;
    }
    else
    {
      #ifdef WARNING
      *logofs << "AgentTransport: WARNING! Locking of read mutex by thread id "
              << pthread_self() << " returned " << result << ". Error is '"
              << ESTR() << "'.\n" << logofs_flush;
      #endif

      return result;
    }
  }
}

int AgentTransport::lockWrite()
{
  for (;;)
  {
    int result = pthread_mutex_lock(&m_write_);

    if (result == 0)
    {
      #ifdef DEBUG
      *logofs << "AgentTransport: Write mutex locked by thread id "
              << pthread_self() << ".\n" << logofs_flush;
      #endif

      return 0;
    }
    else if (EGET() == EINTR)
    {
      continue;
    }
    else
    {
      #ifdef WARNING
      *logofs << "AgentTransport: WARNING! Locking of write mutex by thread id "
              << pthread_self() << " returned " << result << ". Error is '"
              << ESTR() << "'.\n" << logofs_flush;
      #endif

      return result;
    }
  }
}

int AgentTransport::unlockRead()
{
  for (;;)
  {
    int result = pthread_mutex_unlock(&m_read_);

    if (result == 0)
    {
      #ifdef DEBUG
      *logofs << "AgentTransport: Read mutex unlocked by thread id "
              << pthread_self() << ".\n" << logofs_flush;
      #endif

      return 0;
    }
    else if (EGET() == EINTR)
    {
      continue;
    }
    else
    {
      #ifdef WARNING
      *logofs << "AgentTransport: WARNING! Unlocking of read mutex by thread id "
              << pthread_self() << " returned " << result << ". Error is '"
              << ESTR() << "'.\n" << logofs_flush;
      #endif

      return result;
    }
  }
}

int AgentTransport::unlockWrite()
{
  for (;;)
  {
    int result = pthread_mutex_unlock(&m_write_);

    if (result == 0)
    {
      #ifdef DEBUG
      *logofs << "AgentTransport: Write mutex unlocked by thread id "
              << pthread_self() << ".\n" << logofs_flush;
      #endif

      return 0;
    }
    else if (EGET() == EINTR)
    {
      continue;
    }
    else
    {
      #ifdef WARNING
      *logofs << "AgentTransport: WARNING! Unlocking of write mutex by thread id "
              << pthread_self() << " returned " << result << ". Error is '"
              << ESTR() << "'.\n" << logofs_flush;
      #endif

      return result;
    }
  }
}

#endif
