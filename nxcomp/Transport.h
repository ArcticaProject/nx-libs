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

#ifndef Transport_H
#define Transport_H

#include <zlib.h>
#include <errno.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "Misc.h"
#include "Control.h"

#include "Types.h"
#include "Timestamp.h"
#include "Socket.h"

//
// Set the verbosity level.
//

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

//
// Define this to lock and unlock the
// memory-to-memory transport buffers
// before they are accessed. The code
// is outdated and doesn't work with
// the current pthread library.
//

#undef  THREADS

//
// Define this to know when a socket
// is created or destroyed.
//

#undef  REFERENCES

//
// Size of buffer if not set by user.
//

#define TRANSPORT_BUFFER_DEFAULT_SIZE         16384

//
// Type of transport.
//

typedef enum
{
  transport_base,
  transport_proxy,
  transport_agent,
  transport_last_tag

} T_transport_type;

//
// This class handles the buffered I/O on
// the network sockets.
//

//
// TODO: This class is useful but adds a lot of
// overhead. There are many improvements we can
// make here:
//
// - There should be a generic Buffer class, ac-
//   comodating a list of memory buffers. This
//   would enable the use of the readv() and
//   writev() functions to perform the I/O on
//   the socket.
//
// - The buffering should be moved to the Write-
//   Buffer and ReadBuffer classes. By performing
//   the buffering here and there, we are dupli-
//   cating a lot of code and are adding a lot
//   of useless memory copies.
//
// - Stream compression should be removed. The
//   proxy should compress the frames based on
//   the type and should include the length of
//   the decompressed data in the header of the
//   packet. Besides avoiding the compression
//   of packets that cannot be reduced in size,
//   we would also save the additional memory
//   allocations due to the fact that we don't
//   know the size of the decode buffer at the
//   time we read the packet from the network.
//
// - The other utilities implemented here, like
//   the functions forcing a write on the socket
//   or waiting for more data to become available
//   should be moved to the Proxy or the Channel
//   classes.
//

class Transport
{
  public:

  //
  // Member functions.
  //

  Transport(int fd);

  virtual ~Transport();

  int fd() const
  {
    return fd_;
  }

  T_transport_type getType()
  {
    return type_;
  }

  //
  // Virtual members redefined by proxy
  // and 'memory-to-memory' I/O layers.
  //

  virtual int read(unsigned char *data, unsigned int size);

  virtual int write(T_write type, const unsigned char *data, const unsigned int size);

  virtual int flush();

  virtual int drain(int limit, int timeout);

  virtual void finish()
  {
    fullReset();

    finish_ = 1;
  }

  virtual int length() const
  {
    return w_buffer_.length_;
  }

  virtual int pending() const
  {
    return 0;
  }

  virtual int readable() const
  {
    return GetBytesReadable(fd_);
  }

  virtual int writable() const
  {
    return GetBytesWritable(fd_);
  }

  virtual int queued() const
  {
    return GetBytesQueued(fd_);
  }

  virtual int flushable() const
  {
    return 0;
  }

  virtual int wait(int timeout) const;

  void setSize(unsigned int initialSize,
                   unsigned int thresholdSize,
                       unsigned int maximumSize);

  //
  // Return a pointer to the data
  // in the read buffer.
  //

  virtual unsigned int getPending(unsigned char *&data)
  {
    data = NULL;

    return 0;
  }

  virtual void pendingReset()
  {
  }

  virtual void partialReset()
  {
    partialReset(w_buffer_);
  }

  virtual void fullReset();

  int blocked() const
  {
    return blocked_;
  }

  protected:

  //
  // Make room in the buffer to accomodate 
  // at least size bytes.
  //

  int resize(T_buffer &buffer, const int &size);

  void partialReset(T_buffer &buffer)
  {
    if (buffer.length_ == 0 &&
            (buffer.data_.size() > initialSize_ ||
                 buffer.data_.capacity() > initialSize_))
    {
      fullReset(buffer);
    }
  }

  void fullReset(T_buffer &buffer);

  //
  // Data members.
  //

  int fd_;

  int blocked_;
  int finish_;

  T_buffer w_buffer_;

  unsigned int initialSize_;
  unsigned int thresholdSize_;
  unsigned int maximumSize_;

  T_transport_type type_;

  private:

  #ifdef REFERENCES

  static int references_;

  #endif
};

//
// This class handles buffered I/O and 
// compression of the proxy stream.
//

class ProxyTransport : public Transport
{
  public:

  ProxyTransport(int fd);

  virtual ~ProxyTransport();

  virtual int read(unsigned char *data, unsigned int size);

  virtual int write(T_write type, const unsigned char *data, const unsigned int size);

  virtual int flush();

  //
  // Same as in the base class.
  //
  // virtual int drain(int limit, int timeout);
  //
  // virtual void finish();
  //

  //
  // Same as in the base class.
  //
  // virtual int length() const
  //

  virtual int pending() const
  {
    return r_buffer_.length_;
  }

  //
  // Same as in the base class.
  //
  // virtual int readable() const;
  //
  // virtual int writable() const;
  //
  // virtual int queued() const;
  //

  virtual int flushable() const
  {
    return flush_;
  }

  //
  // Same as in the base class, but
  // should not be called.
  //
  // int drained() const;
  //
  // Same as in the base class.
  //
  // virtual int wait(int timeout) const;
  //
  // Same as in the base class.
  //
  // void setSize(unsigned int initialSize,
  //                  unsigned int thresholdSize,
  //                      unsigned int maximumSize);
  //

  virtual unsigned int getPending(unsigned char *&data);

  virtual void pendingReset()
  {
    owner_ = 1;
  }

  virtual void partialReset()
  {
    if (owner_ == 1)
    {
      Transport::partialReset(r_buffer_);
    }

    Transport::partialReset(w_buffer_);
  }

  virtual void fullReset();

  //
  // Same as in the base class.
  //
  // int blocked() const;
  //

  protected:

  int flush_;
  int owner_;

  T_buffer r_buffer_;

  z_stream r_stream_;
  z_stream w_stream_;

  private:

  #ifdef REFERENCES

  static int references_;

  #endif
};

//
// Handle memory-to-memory data transfers between
// an agent and the proxy.
//

class AgentTransport : public Transport
{
  public:

  AgentTransport(int fd);

  virtual ~AgentTransport();

  virtual int read(unsigned char *data, unsigned int size);

  virtual int write(T_write type, const unsigned char *data, const unsigned int size);

  //
  // These two should never be called.
  //

  virtual int flush();

  virtual int drain(int limit, int timeout);

  //
  // Same as in the base class.
  //
  // virtual void finish();
  //

  //
  // Same as in the base class.
  //
  // virtual int length() const
  //

  virtual int pending() const
  {
    return r_buffer_.length_;
  }

  //
  // These are intended to operate only
  // on the internal buffers.
  //

  virtual int readable() const
  {
    return r_buffer_.length_;
  }

  virtual int writable() const
  {
    return control -> TransportMaximumBufferSize;
  }

  virtual int queued() const
  {
    return 0;
  }

  //
  // Same as in the base class.
  //
  // virtual int flushable() const;
  //
  // Same as in the base class, but
  // should not be called.
  //
  // int drained() const;
  //

  //
  // Return immediately or will
  // block until the timeout.
  //

  virtual int wait(int timeout) const
  {
    return 0;
  }

  //
  // Same as in the base class.
  //
  // void setSize(unsigned int initialSize,
  //                  unsigned int thresholdSize,
  //                      unsigned int maximumSize);
  //

  virtual unsigned int getPending(unsigned char *&data);

  virtual void pendingReset()
  {
    owner_ = 1;
  }

  virtual void partialReset()
  {
    if (owner_ == 1)
    {
      Transport::partialReset(r_buffer_);
    }

    Transport::partialReset(w_buffer_);
  }

  virtual void fullReset();

  //
  // Same as in the base class.
  //
  // int blocked() const;
  //

  //
  // The following are specific of the
  // memory-to-memory transport.
  //

  int enqueue(const char *data, const int size);

  int dequeue(char *data, int size);

  int queuable()
  {
    //
    // Always allow the agent to enqueue
    // more data.
    //

    return control -> TransportMaximumBufferSize;
  }

  int dequeuable();

  protected:

  //
  // Lock the buffer to handle reads and
  // writes safely.
  //

  #ifdef THREADS

  int lockRead();
  int lockWrite();

  int unlockRead();
  int unlockWrite();

  #endif

  //
  // Data members.
  //

  int owner_;

  T_buffer r_buffer_;

  //
  // Mutexes for safe read and write.
  //

  #ifdef THREADS

  pthread_mutex_t m_read_;
  pthread_mutex_t m_write_;

  #endif

  private:

  #ifdef REFERENCES

  static int references_;

  #endif
};

#endif /* Transport_H */
