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

#ifndef Agent_H
#define Agent_H

#include <unistd.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>

#include "Misc.h"
#include "Transport.h"
#include "Proxy.h"

extern Proxy *proxy;

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

class Agent
{
  public:

  //
  // Must be created by passing the fake descriptor that
  // will be used for simulating socket communication
  // betwen the agent and the proxy. I/O will take place
  // by copying data to the agent's read and write buf-
  // fers.
  //

  Agent(int fd[2]);

  ~Agent();

  AgentTransport *getTransport() const
  {
    return transport_;
  }

  void saveReadMask(fd_set *readSet)
  {
    saveRead_ = *readSet;
  }

  void saveWriteMask(fd_set *writeSet)
  {
    saveWrite_ = *writeSet;
  }

  void clearReadMask(fd_set *readSet)
  {
    FD_CLR(remoteFd_, readSet);
    FD_CLR(localFd_,  readSet);
  }

  void clearWriteMask(fd_set *writeSet)
  {
    FD_CLR(remoteFd_, writeSet);
    FD_CLR(localFd_,  writeSet);
  }

  void setLocalRead(fd_set *readSet, int *result)
  {
    (*result)++;

    FD_SET(localFd_, readSet);
  }

  void setRemoteRead(fd_set *readSet, int *result)
  {
    (*result)++;

    FD_SET(remoteFd_, readSet);
  }

  void setRemoteWrite(fd_set *writeSet, int *result)
  {
    (*result)++;

    FD_SET(remoteFd_, writeSet);
  }

  fd_set *getSavedReadMask()
  {
    return &saveRead_;
  }

  fd_set *getSavedWriteMask()
  {
    return &saveWrite_;
  }

  int getRemoteFd() const
  {
    return remoteFd_;
  }

  int getLocalFd() const
  {
    return localFd_;
  }

  int getProxyFd() const
  {
    return proxy -> getFd();
  }

  int isValid() const
  {
    return (transport_ != NULL);
  }

  int localReadable()
  {
    return (transport_ -> readable() != 0);
  }

  //
  // Check if we can process more data from
  // the agent descriptor and cache the result
  // to avoid multiple calls. This must be
  // always called before querying the other
  // functions.
  //

  void saveChannelState()
  {
    canRead_ = (proxy != NULL ? proxy -> canRead(localFd_) : 0);
  }

  int remoteCanRead(const fd_set * const readSet)
  {
    // OS X 10.5 requires the second argument to be non-const, so copy readSet.
    // It's safe though, as FD_ISSET does not operate on it.
    fd_set readWorkSet = *readSet;

    #if defined(TEST) || defined(INFO)
    *logofs << "Agent: remoteCanRead() is " <<
               (FD_ISSET(remoteFd_, &readWorkSet) && transport_ -> dequeuable() != 0)
            << " with FD_ISSET() " << (int) FD_ISSET(remoteFd_, &readWorkSet)
            << " and dequeuable " << transport_ -> dequeuable()
            << ".\n" << logofs_flush;
    #endif

    return (FD_ISSET(remoteFd_, &readWorkSet) &&
                transport_ -> dequeuable() != 0);
  }

  int remoteCanWrite(const fd_set * const writeSet)
  {
    // OS X 10.5 requires the second argument to be non-const, so copy writeSet.
    // It's safe though, as FD_ISSET does not operate on it.
    fd_set writeWorkSet = *writeSet;

    #if defined(TEST) || defined(INFO)
    *logofs << "Agent: remoteCanWrite() is " <<
               (FD_ISSET(remoteFd_, &writeWorkSet) && transport_ ->
               queuable() != 0 && canRead_ == 1) << " with FD_ISSET() "
            << (int) FD_ISSET(remoteFd_, &writeWorkSet) << " queueable "
            << transport_ -> queuable() << " channel can read "
            << canRead_ << ".\n" << logofs_flush;
    #endif

    return (FD_ISSET(remoteFd_, &writeWorkSet) &&
                transport_ -> queuable() != 0 &&
                    canRead_ == 1);
  }

  int localCanRead()
  {
    #if defined(TEST) || defined(INFO)
    *logofs << "Agent: localCanRead() is " <<
               (transport_ -> readable() != 0 && canRead_ == 1)
            << " with readable " << transport_ -> readable()
            << " channel can read " << canRead_ << ".\n"
            << logofs_flush;
    #endif

    return (transport_ -> readable() != 0 &&
                canRead_ == 1);
  }

  int proxyCanRead()
  {
    #if defined(TEST) || defined(INFO)
    *logofs << "Agent: proxyCanRead() is " << proxy -> canRead()
            << ".\n" << logofs_flush;
    #endif

    return (proxy -> canRead());
  }

  int proxyCanRead(const fd_set * const readSet)
  {
    // OS X 10.5 requires the second argument to be non-const, so copy readSet.
    // It's safe though, as FD_ISSET does not operate on it.
    fd_set readWorkSet = *readSet;

    #if defined(TEST) || defined(INFO)
    *logofs << "Agent: proxyCanRead() is "
            << ((int) FD_ISSET(proxy -> getFd(), &readWorkSet)
            << ".\n" << logofs_flush;
    #endif

    return (FD_ISSET(proxy -> getFd(), &readWorkSet));
  }

  int enqueueData(const char *data, const int size) const
  {
    return transport_ -> enqueue(data, size);
  }

  int dequeueData(char *data, int size) const
  {
    return transport_ -> dequeue(data, size);
  }

  int dequeuableData() const
  {
    return transport_ -> dequeuable();
  }

  private:

  int remoteFd_;
  int localFd_;

  fd_set saveRead_;
  fd_set saveWrite_;

  int canRead_;

  AgentTransport *transport_;
};

#endif /* Agent_H */
