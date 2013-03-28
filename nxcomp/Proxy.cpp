/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2011 NoMachine, http://www.nomachine.com/.         */
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

#include <cstdio>
#include <unistd.h>
#include <cstdlib>
#include <sys/types.h>
#include <sys/stat.h>

#include "Misc.h"

#if defined(__CYGWIN32__) || defined(__APPLE__) || defined(__FreeBSD__) || defined(__sun)
#include <netinet/in_systm.h>
#endif

#ifndef __CYGWIN32__
#include <sys/un.h>
#endif

#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>

#if defined(__EMX__ ) || defined(__CYGWIN32__)

struct sockaddr_un
{
  u_short sun_family;
  char sun_path[108];
};

#endif

#include "NXalert.h"
#include "NXvars.h"

#include "Proxy.h"

#include "Socket.h"
#include "Channel.h"
#include "Statistics.h"

#include "ClientChannel.h"
#include "ServerChannel.h"
#include "GenericChannel.h"

//
// We need to adjust some values related
// to these messages at the time the mes-
// sage stores are reconfigured.
//

#include "PutImage.h"
#include "ChangeGC.h"
#include "PolyFillRectangle.h"
#include "PutPackedImage.h"

//
// This is from the main loop.
//

extern void CleanupListeners();

//
// Default size of string buffers.
//

#define DEFAULT_STRING_LENGTH  512

//
// Set the verbosity level. You also need
// to define DUMP in Misc.cpp if DUMP is
// defined here.
//

#define WARNING
#define PANIC
#undef  TEST
#undef  DEBUG
#undef  DUMP

//
// Log the important tracepoints related
// to writing packets to the peer proxy.
//

#undef  FLUSH

//
// Log the operations related to splits.
//

#undef  SPLIT

//
// Log the operations related to sending
// and receiving the control tokens.
//

#undef  TOKEN

//
// Log the operations related to setting
// the token limits.
//

#undef  LIMIT

//
// Log a warning if no data is written by
// the proxy within a timeout.
//

#undef  TIME

//
// Log the operation related to generating
// the ping message at idle time.
//

#undef  PING

Proxy::Proxy(int fd)

  : transport_(new ProxyTransport(fd)), fd_(fd), readBuffer_(transport_)
{
  for (int channelId = 0;
           channelId < CONNECTIONS_LIMIT;
               channelId++)
  {
    channels_[channelId]    = NULL;
    transports_[channelId]  = NULL;
    congestions_[channelId] = 0;

    fdMap_[channelId]      = nothing;
    channelMap_[channelId] = nothing;
  }

  inputChannel_  = nothing;
  outputChannel_ = nothing;

  controlLength_ = 0;

  operation_ = operation_in_negotiation;

  draining_   = 0;
  priority_   = 0;
  finish_     = 0;
  shutdown_   = 0;
  congestion_ = 0;

  timer_ = 0;
  alert_ = 0;

  agent_ = nothing;

  //
  // Set null timeouts. This will require
  // a new link configuration.
  //

  timeouts_.split  = 0;
  timeouts_.motion = 0;

  timeouts_.readTs  = getTimestamp();
  timeouts_.writeTs = getTimestamp();

  timeouts_.loopTs  = getTimestamp();
  timeouts_.pingTs  = getTimestamp();
  timeouts_.alertTs = nullTimestamp();
  timeouts_.loadTs  = nullTimestamp();

  timeouts_.splitTs  = nullTimestamp();
  timeouts_.motionTs = nullTimestamp();

  //
  // Initialize the token counters. This
  // will require a new link configuration.
  //

  for (int i = token_control; i <= token_data; i++)
  {
    tokens_[i].size      = 0;
    tokens_[i].limit     = 0;

    tokens_[i].bytes     = 0;
    tokens_[i].remaining = 0;
  }

  tokens_[token_control].request = code_control_token_request;
  tokens_[token_control].reply   = code_control_token_reply;
  tokens_[token_control].type    = token_control;

  tokens_[token_split].request = code_split_token_request;
  tokens_[token_split].reply   = code_split_token_reply;
  tokens_[token_split].type    = token_split;

  tokens_[token_data].request = code_data_token_request;
  tokens_[token_data].reply   = code_data_token_reply;
  tokens_[token_data].type    = token_data;

  currentStatistics_ = NULL;

  //
  // Create compressor and decompressor
  // for image and data payload.
  //

  compressor_ = new StaticCompressor(control -> LocalDataCompressionLevel,
                                         control -> LocalDataCompressionThreshold);

  //
  // Create object storing NX specific
  // opcodes.
  //

  opcodeStore_ = new OpcodeStore();

  //
  // Create the message stores.
  //

  clientStore_ = new ClientStore(compressor_);
  serverStore_ = new ServerStore(compressor_);

  //
  // Older proxies will refuse to store
  // messages bigger than 262144 bytes.
  //

  if (control -> isProtoStep7() == 0)
  {
    #ifdef TEST
    *logofs << "Proxy: WARNING! Limiting the maximum "
            << "message size to " << 262144 << ".\n"
            << logofs_flush;
    #endif

    control -> MaximumMessageSize = 262144;
  }

  clientCache_ = new ClientCache();
  serverCache_ = new ServerCache();

  if (clientCache_ == NULL || serverCache_ == NULL)
  {
    #ifdef PANIC
    *logofs << "Proxy: PANIC! Failed to create the channel cache.\n"
            << logofs_flush;
    #endif

    cerr << "Error" << ": Failed to create the channel cache.\n";

    HandleCleanup();
  }

  //
  // Prepare for image decompression.
  //

  UnpackInit();

  #ifdef DEBUG
  *logofs << "Proxy: Created new object at " << this
          << ".\n" << logofs_flush;
  #endif
}

Proxy::~Proxy()
{
  for (int channelId = 0;
           channelId < CONNECTIONS_LIMIT;
               channelId++)
  {
    if (channels_[channelId] != NULL)
    {
      deallocateTransport(channelId);

      delete channels_[channelId];
      channels_[channelId] = NULL;
    }
  }

  delete transport_;
  delete compressor_;

  //
  // Delete storage shared among channels.
  //

  delete opcodeStore_;

  delete clientStore_;
  delete serverStore_;

  delete clientCache_;
  delete serverCache_;

  //
  // Get rid of the image decompression
  // resources.
  //

  UnpackDestroy();

  #ifdef DEBUG
  *logofs << "Proxy: Deleted proxy object at " << this
          << ".\n" << logofs_flush;
  #endif
}

int Proxy::setOperational()
{
  #ifdef TEST
  *logofs << "Proxy: Entering operational mode.\n"
          << logofs_flush;
  #endif

  operation_ = operation_in_messages;

  return 1;
}

int Proxy::setReadDescriptors(fd_set *fdSet, int &fdMax, T_timestamp &tsMax)
{
  //
  // Set the initial timeout to the time of
  // the next ping. If the congestion count
  // is greater than zero, anyway, use a
  // shorter timeout to force a congestion
  // update.
  //

  if (agent_ != nothing && congestions_[agent_] == 0 &&
          statistics -> getCongestionInFrame() >= 1 &&
              tokens_[token_control].remaining >=
                  (tokens_[token_control].limit - 1))
  {
    setMinTimestamp(tsMax, control -> IdleTimeout);

    #ifdef TEST
    *logofs << "Proxy: Initial timeout is " << tsMax.tv_sec
            << " S and " << (double) tsMax.tv_usec /
               1000 << " Ms with congestion "
            << statistics -> getCongestionInFrame()
            << ".\n" << logofs_flush;
    #endif
  }
  else
  {
    setMinTimestamp(tsMax, control -> PingTimeout);

    #ifdef TEST
    *logofs << "Proxy: Initial timeout is " << tsMax.tv_sec
            << " S and " << (double) tsMax.tv_usec /
               1000 << " Ms.\n" << logofs_flush;
    #endif
  }

  int fd = -1;

  if (isTimeToRead() == 1)
  {
    //
    // If we don't have split tokens available
    // don't set the timeout.
    //

    if (tokens_[token_split].remaining > 0 &&
            isTimestamp(timeouts_.splitTs) == 1)
    {
      int diffTs = getTimeToNextSplit();

      #if defined(TEST) || defined(INFO) || \
              defined(FLUSH) || defined(SPLIT)

      if (diffTimestamp(timeouts_.splitTs,
              getTimestamp()) > timeouts_.split)
      {
        *logofs << "Proxy: FLUSH! SPLIT! WARNING! Running with "
                << diffTimestamp(timeouts_.splitTs, getTimestamp())
                << " Ms elapsed since the last split.\n"
                << logofs_flush;
      }

      *logofs << "Proxy: FLUSH! SPLIT! Requesting timeout of "
              << diffTs << " Ms as there are splits to send.\n"
              << logofs_flush;

      #endif

      setMinTimestamp(tsMax, diffTs);
    }
    #if defined(TEST) || defined(INFO)
    else if (isTimestamp(timeouts_.splitTs) == 1)
    {
      *logofs << "Proxy: WARNING! Not requesting a split "
              << "timeout with " << tokens_[token_split].remaining
              << " split tokens remaining.\n" << logofs_flush;
    }
    #endif

    //
    // Loop through the valid channels and set
    // the descriptors selected for read and
    // the timeout.
    //

    T_list &channelList = activeChannels_.getList();

    for (T_list::iterator j = channelList.begin();
           j != channelList.end(); j++)
    {
      int channelId = *j;

      if (channels_[channelId] == NULL)
      {
        continue;
      }

      fd = getFd(channelId);

      if (channels_[channelId] -> getFinish() == 0 &&
              (channels_[channelId] -> getType() == channel_x11 ||
                  tokens_[token_data].remaining > 0) &&
                      congestions_[channelId] == 0)
      {
        FD_SET(fd, fdSet);

        if (fd >= fdMax)
        {
          fdMax = fd + 1;
        }

        #ifdef TEST
        *logofs << "Proxy: Descriptor FD#" << fd
                << " selected for read with buffer length "
                << transports_[channelId] -> length()
                << ".\n" << logofs_flush;
        #endif

        //
        // Wakeup the proxy if there are motion
        // events to flush.
        //

        if (isTimestamp(timeouts_.motionTs) == 1)
        {
          int diffTs = getTimeToNextMotion();

          #if defined(TEST) || defined(INFO)

          if (diffTimestamp(timeouts_.motionTs,
                  getTimestamp()) > timeouts_.motion)
          {
            *logofs << "Proxy: FLUSH! WARNING! Running with "
                    << diffTimestamp(timeouts_.motionTs, getTimestamp())
                    << " Ms elapsed since the last motion.\n"
                    << logofs_flush;
          }

          *logofs << "Proxy: FLUSH! Requesting timeout of "
                  << diffTs << " Ms as FD#" << fd << " has motion "
                  << "events to send.\n" << logofs_flush;

          #endif

          setMinTimestamp(tsMax, diffTs);
        }
      }
      #if defined(TEST) || defined(INFO)
      else
      {
        if (channels_[channelId] -> getType() != channel_x11 &&
                tokens_[token_data].remaining <= 0)
        {
          *logofs << "Proxy: WARNING! Descriptor FD#" << fd
                  << " not selected for read with "
                  << tokens_[token_data].remaining << " data "
                  << "tokens remaining.\n" << logofs_flush;
        }
      }
      #endif
    }
  }
  #if defined(TEST) || defined(INFO)
  else
  {
    *logofs << "Proxy: WARNING! Disabled reading from channels.\n"
            << logofs_flush;

    *logofs << "Proxy: WARNING! Congestion is " << congestion_
            << " pending " << transport_ -> pending() << " blocked "
            << transport_ -> blocked() << " length " << transport_ ->
               length() << ".\n" << logofs_flush;
  }
  #endif

  //
  // Include the proxy descriptor.
  //

  FD_SET(fd_, fdSet);

  if (fd_ >= fdMax)
  {
    fdMax = fd_ + 1;
  }

  #ifdef TEST
  *logofs << "Proxy: Proxy descriptor FD#" << fd_
          << " selected for read with buffer length "
          << transport_ -> length() << ".\n"
          << logofs_flush;
  #endif

  return 1;
}

//
// Add to the mask the file descriptors of all
// X connections to write to.
//

int Proxy::setWriteDescriptors(fd_set *fdSet, int &fdMax, T_timestamp &tsMax)
{
  int fd = -1;

  T_list &channelList = activeChannels_.getList();

  for (T_list::iterator j = channelList.begin();
           j != channelList.end(); j++)
  {
    int channelId = *j;

    if (channels_[channelId] != NULL)
    {
      fd = getFd(channelId);

      if (transports_[channelId] -> length() > 0)
      {
        FD_SET(fd, fdSet);

        #ifdef TEST
        *logofs << "Proxy: Descriptor FD#" << fd << " selected "
                << "for write with blocked " << transports_[channelId] ->
                   blocked() << " and length " << transports_[channelId] ->
                   length() << ".\n" << logofs_flush;
        #endif

        if (fd >= fdMax)
        {
          fdMax = fd + 1;
        }
      }
      #ifdef TEST
      else
      {
        *logofs << "Proxy: Descriptor FD#" << fd << " not selected "
                << "for write with blocked " << transports_[channelId] ->
                   blocked() << " and length " << transports_[channelId] ->
                   length() << ".\n" << logofs_flush;
      }
      #endif

      #if defined(TEST) || defined(INFO)

      if (transports_[channelId] -> getType() !=
              transport_agent && transports_[channelId] ->
                   length() > 0 && transports_[channelId] ->
                       blocked() != 1)
      {
        *logofs << "Proxy: PANIC! Descriptor FD#" << fd
                << " has data to write but blocked flag is "
                << transports_[channelId] -> blocked()
                << ".\n" << logofs_flush;

        cerr << "Error" << ": Descriptor FD#" << fd
             << " has data to write but blocked flag is "
             << transports_[channelId] -> blocked()
             << ".\n";

        HandleCleanup();
      }

      #endif
    }
  }

  //
  // Check if the proxy transport has data
  // from a previous blocking write.
  //

  if (transport_ -> blocked() == 1)
  {
    FD_SET(fd_, fdSet);

    #ifdef TEST
    *logofs << "Proxy: Proxy descriptor FD#"
            << fd_ << " selected for write. Blocked is "
            << transport_ -> blocked() << " length is "
            << transport_ -> length() << ".\n"
            << logofs_flush;
    #endif

    if (fd_ >= fdMax)
    {
      fdMax = fd_ + 1;
    }
  }
  #ifdef TEST
  else
  {
    *logofs << "Proxy: Proxy descriptor FD#"
            << fd_ << " not selected for write. Blocked is "
            << transport_ -> blocked() << " length is "
            << transport_ -> length() << ".\n"
            << logofs_flush;
  }
  #endif

  //
  // We are entering the main select. Save
  // the timestamp of the last loop so that
  // we can detect the clock drifts.
  //

  timeouts_.loopTs = getTimestamp();

  return 1;
}

int Proxy::getChannels(T_channel_type type)
{
  int channels = 0;

  T_list &channelList = activeChannels_.getList();

  for (T_list::iterator j = channelList.begin();
           j != channelList.end(); j++)
  {
    int channelId = *j;

    if (channels_[channelId] != NULL &&
            (type == channel_none ||
                 type == channels_[channelId] ->
                     getType()))
    {
      channels++;
    }
  }

  return channels;
}

T_channel_type Proxy::getType(int fd)
{
  int channelId = getChannel(fd);

  if (channelId < 0 || channels_[channelId] == NULL)
  {
    return channel_none;
  }

  return channels_[channelId] -> getType();
}

const char *Proxy::getTypeName(T_channel_type type)
{
  switch (type)
  {
    case channel_x11:
    {
      return "X";
    }
    case channel_cups:
    {
      return "CUPS";
    }
    case channel_smb:
    {
      return "SMB";
    }
    case channel_media:
    {
      return "media";
    }
    case channel_http:
    {
      return "HTTP";
    }
    case channel_font:
    {
      return "font";
    }
    case channel_slave:
    {
      return "slave";
    }
    default:
    {
      return "unknown";
    }
  }
}

const char *Proxy::getComputerName()
{
  //
  // Strangely enough, under some Windows OSes SMB
  // service doesn't bind to localhost. Fall back
  // to localhost if can't find computer name in
  // the environment. In future we should try to
  // bind to localhost and then try the other IPs.
  //

  const char *hostname = NULL;

  #ifdef __CYGWIN32__

  hostname = getenv("COMPUTERNAME");

  #endif

  if (hostname == NULL)
  {
    hostname = "localhost";
  }

  return hostname;
}

//
// Handle data from channels selected for read.
//

int Proxy::handleRead(int &resultFds, fd_set &readSet)
{
  #ifdef DEBUG
  *logofs << "Proxy: Checking descriptors selected for read.\n"
          << logofs_flush;
  #endif

  T_list &channelList = activeChannels_.getList();

  for (T_list::iterator j = channelList.begin();
           j != channelList.end(); j++)
  {
    #ifdef DEBUG
    *logofs << "Proxy: Looping with current channel "
            << *j << ".\n" << logofs_flush;
    #endif

    int fd = getFd(*j);

    if (fd >= 0 && resultFds > 0 && FD_ISSET(fd, &readSet))
    {
      #ifdef DEBUG
      *logofs << "Proxy: Going to read messages from FD#"
              << fd << ".\n" << logofs_flush;
      #endif

      int result = handleRead(fd);

      if (result < 0)
      {
        #ifdef TEST
        *logofs << "Proxy: Failure reading messages from FD#"
                << fd << ".\n" << logofs_flush;
        #endif

        return -1;
      }

      #ifdef DEBUG
      *logofs << "Proxy: Clearing the read descriptor "
              << "for FD#" << fd << ".\n" << logofs_flush;
      #endif

      FD_CLR(fd, &readSet);

      resultFds--;
    }
  }

  if (resultFds > 0 && FD_ISSET(fd_, &readSet))
  {
    #ifdef DEBUG
    *logofs << "Proxy: Going to read messages from "
            << "proxy FD#" << fd_ << ".\n"
            << logofs_flush;
    #endif

    if (handleRead() < 0)
    {
      #ifdef TEST
      *logofs << "Proxy: Failure reading from proxy FD#"
              << fd_ << ".\n" << logofs_flush;
      #endif

      return -1;
    }

    #ifdef DEBUG
    *logofs << "Proxy: Clearing the read descriptor "
            << "for proxy FD#" << fd_ << ".\n"
            << logofs_flush;
    #endif

    FD_CLR(fd_, &readSet);

    resultFds--;
  }

  return 1;
}

//
// Perform flush on descriptors selected for write.
//

int Proxy::handleFlush(int &resultFds, fd_set &writeSet)
{
  #ifdef DEBUG
  *logofs << "Proxy: Checking descriptors selected for write.\n"
          << logofs_flush;
  #endif

  if (resultFds > 0 && FD_ISSET(fd_, &writeSet))
  {
    #ifdef TEST
    *logofs << "Proxy: FLUSH! Proxy descriptor FD#" << fd_
            << " reported to be writable.\n"
            << logofs_flush;
    #endif

    if (handleFlush() < 0)
    {
      #ifdef TEST
      *logofs << "Proxy: Failure flushing the writable "
              << "proxy FD#" << fd_ << ".\n"
              << logofs_flush;
      #endif

      return -1;
    }

    #ifdef DEBUG
    *logofs << "Proxy: Clearing the write descriptor "
            << "for proxy FD#" << fd_ << ".\n"
            << logofs_flush;
    #endif

    FD_CLR(fd_, &writeSet);

    resultFds--;
  }

  T_list &channelList = activeChannels_.getList();

  for (T_list::iterator j = channelList.begin();
           resultFds > 0 && j != channelList.end(); j++)
  {
    #ifdef DEBUG
    *logofs << "Proxy: Looping with current channel "
            << *j << ".\n" << logofs_flush;
    #endif

    int fd = getFd(*j);

    if (fd >= 0 && FD_ISSET(fd, &writeSet))
    {
      #ifdef TEST
      *logofs << "Proxy: X descriptor FD#" << fd
              << " reported to be writable.\n"
              << logofs_flush;
      #endif

      //
      // It can happen that, in handling reads, we
      // have destroyed the buffer associated to a
      // closed socket, so don't complain about
      // the errors.
      //

      handleFlush(fd);

      //
      // Clear the descriptor from the mask so
      // we don't confuse the agent if it's
      // not checking only its own descriptors.
      //

      #ifdef DEBUG
      *logofs << "Proxy: Clearing the write descriptor "
              << "for FD#" << fd << ".\n"
              << logofs_flush;
      #endif

      FD_CLR(fd, &writeSet);

      resultFds--;
    }
  }

  return 1;
}

int Proxy::handleRead()
{
  #if defined(TEST) || defined(INFO)
  *logofs << "Proxy: Decoding data from proxy FD#"
          << fd_ << ".\n" << logofs_flush;
  #endif

  //
  // Decode all the available messages from
  // the remote proxy until is not possible
  // to read more.
  //

  for (;;)
  {
    int result = readBuffer_.readMessage();

    #if defined(TEST) || defined(DEBUG) || defined(INFO)
    *logofs << "Proxy: Read result on proxy FD#" << fd_
            << " is " << result << ".\n"
            << logofs_flush;
    #endif

    if (result < 0)
    {
      if (shutdown_ == 0)
      {
        if (finish_ == 0)
        {
          #ifdef PANIC
          *logofs << "Proxy: PANIC! Failure reading from the "
                  << "peer proxy on FD#" << fd_ << ".\n"
                  << logofs_flush;
          #endif

          cerr << "Error" << ": Failure reading from the "
               << "peer proxy.\n";
        }
      }
      #ifdef TEST
      else
      {
        *logofs << "Proxy: Closure of the proxy link detected "
                << "after clean shutdown.\n" << logofs_flush;
      }
      #endif

      priority_   = 0;
      finish_     = 1;
      congestion_ = 0;

      return -1;
    }
    else if (result == 0)
    {
      #if defined(TEST) || defined(DEBUG) || defined(INFO)
      *logofs << "Proxy: No data read from proxy FD#"
              << fd_ << "\n" << logofs_flush;
      #endif

      return 0;
    }

    //
    // We read some data from the remote. If we set
    // the congestion flag because we couldn't read
    // before the timeout and have tokens available,
    // then reset the congestion flag.
    //

    if (congestion_ == 1 &&
            tokens_[token_control].remaining > 0)
    {
      #if defined(TEST) || defined(INFO)
      *logofs << "Proxy: Exiting congestion due to "
              << "proxy data with " << tokens_[token_control].remaining
              << " tokens.\n" << logofs_flush;
      #endif

      congestion_ = 0;
    }

    //
    // Set the timestamp of the last read
    // operation from the remote proxy and
    // enable again showing the 'no data
    // received' dialog at the next timeout.
    //

    timeouts_.readTs  = getTimestamp();

    if (alert_ != 0)
    {
      #if defined(TEST) || defined(INFO)
      *logofs << "Proxy: Displacing the dialog "
              << "for proxy FD#" << fd_ << ".\n"
              << logofs_flush;
      #endif

      HandleAlert(DISPLACE_MESSAGE_ALERT, 1);
    }

    timeouts_.alertTs = nullTimestamp();

    #if defined(TEST) || defined(INFO)
    *logofs << "Proxy: Getting messages from proxy FD#" << fd_
            << " with " << readBuffer_.getLength() << " bytes "
            << "in the read buffer.\n" << logofs_flush;
    #endif

    unsigned int controlLength;
    unsigned int dataLength;

    const unsigned char *message;

    while ((message = readBuffer_.getMessage(controlLength, dataLength)) != NULL)
    {
      statistics -> addFrameIn();

      if (controlLength == 3 && *message == 0 &&
              *(message + 1) < code_last_tag)
      {
        if (handleControlFromProxy(message) < 0)
        {
          return -1;
        }
      }
      else if (operation_ == operation_in_messages)
      {
        int channelId = inputChannel_;

        #if defined(TEST) || defined(INFO)
        *logofs << "Proxy: Identified message of " << dataLength
                << " bytes for FD#" << getFd(channelId) << " channel ID#"
                 << channelId << ".\n" << logofs_flush;
        #endif

        if (channelId >= 0 && channelId < CONNECTIONS_LIMIT &&
                channels_[channelId] != NULL)
        {
          int finish = channels_[channelId] -> getFinish();

          #ifdef WARNING

          if (finish == 1)
          {
            *logofs << "Proxy: WARNING! Handling data for finishing "
                    << "FD#" << getFd(channelId) << " channel ID#"
                    << channelId << ".\n" << logofs_flush;
          }

          #endif

          //
          // We need to decode all the data to preserve
          // the consistency of the cache, so can't re-
          // turn as soon as the first error is encount-
          // ered. Check if this is the first time that
          // the failure is detected.
          //

          int result = channels_[channelId] -> handleWrite(message, dataLength);

          if (result < 0 && finish == 0)
          {
            #ifdef TEST
            *logofs << "Proxy: Failed to write proxy data to FD#"
                    << getFd(channelId) << " channel ID#"
                    << channelId << ".\n" << logofs_flush;
            #endif

            if (handleFinish(channelId) < 0)
            {
              return -1;
            }
          }

          //
          // Check if we have splits or motion
          // events to send.
          //

          setSplitTimeout(channelId);
          setMotionTimeout(channelId);
        }
        #ifdef WARNING
        else
        {
          *logofs << "Proxy: WARNING! Received data for "
                  << "invalid channel ID#" << channelId
                  << ".\n" << logofs_flush;
        }
        #endif
      }
      else if (operation_ == operation_in_statistics)
      {
        #ifdef TEST
        *logofs << "Proxy: Received statistics data from remote proxy.\n"
                << logofs_flush;
        #endif

        if (handleStatisticsFromProxy(message, dataLength) < 0)
        {
          return -1;
        }

        operation_ = operation_in_messages;
      }
      else if (operation_ == operation_in_negotiation)
      {
        #ifdef TEST
        *logofs << "Proxy: Received new negotiation data from remote proxy.\n"
                << logofs_flush;
        #endif

        if (handleNegotiationFromProxy(message, dataLength) < 0)
        {
          return -1;
        }
      }

      //
      // if (controlLength == 3 && *message == 0 && ...) ...
      // else if (operation_ == operation_in_statistics) ...
      // else if (operation_ == operation_in_messages) ...
      // else if (operation_ == operation_in_negotiation) ...
      // else ...
      //

      else
      {
        #ifdef PANIC
        *logofs << "Proxy: PANIC! Unrecognized message received on proxy FD#"
                << fd_ << ".\n" << logofs_flush;
        #endif

        cerr << "Error" << ": Unrecognized message received on proxy FD#"
             << fd_ << ".\n";

        return -1;
      }

    } // while ((message = readBuffer_.getMessage(controlLength, dataLength)) != NULL) ...

    //
    // Reset the read buffer.
    //

    readBuffer_.fullReset();

    //
    // Give up if no data is readable.
    //

    if (transport_ -> readable() == 0)
    {
      break;
    }

  } // End of for (;;) ...

  return 1;
}

int Proxy::handleControlFromProxy(const unsigned char *message)
{
  #if defined(TEST) || defined(INFO)
  *logofs << "Proxy: Received message '" << DumpControl(*(message + 1))
          << "' at " << strMsTimestamp() << " with data ID#"
          << (int) *(message + 2) << ".\n" << logofs_flush;
  #endif

  T_channel_type channelType = channel_none;

  switch (*(message + 1))
  {
    case code_switch_connection:
    {
      int channelId = *(message + 2);

      //
      // If channel is invalid further messages will
      // be ignored. The acknowledged shutdown of
      // channels should prevent this.
      //

      inputChannel_ = channelId;

      break;
    }
    case code_begin_congestion:
    {
      //
      // Set the congestion state for the
      // channel reported by the remote.
      //

      int channelId = *(message + 2);

      if (channels_[channelId] != NULL)
      {
        congestions_[channelId] = 1;

        #if defined(TEST) || defined(INFO)
        *logofs << "Proxy: Received a begin congestion "
                << "for channel id ID#" << channelId
                << ".\n" << logofs_flush;
        #endif

        if (channelId == agent_ && congestions_[agent_] != 0)
        {
          #if defined(TEST) || defined(INFO)
          *logofs << "Proxy: Forcing an update of the congestion "
                  << "counter with agent congested.\n"
                  << logofs_flush;
          #endif

          statistics -> updateCongestion(-tokens_[token_control].remaining,
                                             tokens_[token_control].limit);
        }
      }
      #ifdef WARNING
      else
      {
        *logofs << "Proxy: WARNING! Received a begin congestion "
                << "for invalid channel id ID#" << channelId
                << ".\n" << logofs_flush;
      }
      #endif

      break;
    }
    case code_end_congestion:
    {
      //
      // Attend again to the channel.
      //

      int channelId = *(message + 2);

      if (channels_[channelId] != NULL)
      {
        congestions_[channelId] = 0;

        #if defined(TEST) || defined(INFO)
        *logofs << "Proxy: Received an end congestion "
                << "for channel id ID#" << channelId
                << ".\n" << logofs_flush;
        #endif

        if (channelId == agent_ && congestions_[agent_] != 0)
        {
          #if defined(TEST) || defined(INFO)
          *logofs << "Proxy: Forcing an update of the congestion "
                  << "counter with agent decongested.\n"
                  << logofs_flush;
          #endif

          statistics -> updateCongestion(tokens_[token_control].remaining,
                                             tokens_[token_control].limit);
        }
      }
      #ifdef WARNING
      else
      {
        *logofs << "Proxy: WARNING! Received an end congestion "
                << "for invalid channel id ID#" << channelId
                << ".\n" << logofs_flush;
      }
      #endif

      break;
    }
    case code_control_token_request:
    {
      T_proxy_token &token = tokens_[token_control];

      if (handleTokenFromProxy(token, *(message + 2)) < 0)
      {
        return -1;
      }

      break;
    }
    case code_split_token_request:
    {
      T_proxy_token &token = tokens_[token_split];

      if (handleTokenFromProxy(token, *(message + 2)) < 0)
      {
        return -1;
      }

      break;
    }
    case code_data_token_request:
    {
      T_proxy_token &token = tokens_[token_data];

      if (handleTokenFromProxy(token, *(message + 2)) < 0)
      {
        return -1;
      }

      break;
    }
    case code_control_token_reply:
    {
      T_proxy_token &token = tokens_[token_control];

      if (handleTokenReplyFromProxy(token, *(message + 2)) < 0)
      {
        return -1;
      }

      break;
    }
    case code_split_token_reply:
    {
      T_proxy_token &token = tokens_[token_split];

      if (handleTokenReplyFromProxy(token, *(message + 2)) < 0)
      {
        return -1;
      }

      break;
    }
    case code_data_token_reply:
    {
      T_proxy_token &token = tokens_[token_data];

      if (handleTokenReplyFromProxy(token, *(message + 2)) < 0)
      {
        return -1;
      }

      break;
    }
    case code_new_x_connection:
    {
      //
      // Opening the channel is handled later.
      //

      channelType = channel_x11;

      break;
    }
    case code_new_cups_connection:
    {
      channelType = channel_cups;

      break;
    }
    case code_new_aux_connection:
    {
      //
      // Starting from version 1.5.0 we create real X
      // connections for the keyboard channel. We need
      // to refuse old auxiliary X connections because
      // they would be unable to leverage the new fake
      // authorization cookie.
      //

      #ifdef WARNING
      *logofs << "Proxy: WARNING! Can't open outdated auxiliary X "
              << "channel for code " << *(message + 1) << ".\n"
              << logofs_flush;
      #endif

      cerr << "Warning" << ": Can't open outdated auxiliary X "
           << "channel for code " << *(message + 1) << ".\n";

      if (handleControl(code_drop_connection, *(message + 2)) < 0)
      {
        return -1;
      }

      break;
    }
    case code_new_smb_connection:
    {
      channelType = channel_smb;

      break;
    }
    case code_new_media_connection:
    {
      channelType = channel_media;

      break;
    }
    case code_new_http_connection:
    {
      channelType = channel_http;

      break;
    }
    case code_new_font_connection:
    {
      channelType = channel_font;

      break;
    }
    case code_new_slave_connection:
    {
      channelType = channel_slave;

      break;
    }
    case code_drop_connection:
    {
      int channelId = *(message + 2);

      if (channelId >= 0 && channelId < CONNECTIONS_LIMIT &&
              channels_[channelId] != NULL)
      {
        handleDropFromProxy(channelId);
      }
      #ifdef WARNING
      else
      {
        *logofs << "Proxy: WARNING! Received a drop message "
                << "for invalid channel id ID#" << channelId
                << ".\n" << logofs_flush;
      }
      #endif

      break;
    }
    case code_finish_connection:
    {
      int channelId = *(message + 2);

      if (channelId >= 0 && channelId < CONNECTIONS_LIMIT &&
              channels_[channelId] != NULL)
      {
        //
        // Force the finish state on the channel.
        // We can receive this message while in
        // the read loop, so we only mark the
        // channel for deletion.
        //

        #ifdef TEST
        *logofs << "Proxy: Received a finish message for FD#"
                << getFd(channelId) << " channel ID#"
                << channelId << ".\n" << logofs_flush;
        #endif

        handleFinishFromProxy(channelId);
      }
      #ifdef WARNING
      else
      {
        *logofs << "Proxy: WARNING! Received a finish message "
                << "for invalid channel id ID#" << channelId
                << ".\n" << logofs_flush;
      }
      #endif

      break;
    }
    case code_finish_listeners:
    {
      //
      // This is from the main loop.
      //

      #ifdef TEST
      *logofs << "Proxy: Closing down all local listeners.\n"
              << logofs_flush;
      #endif

      CleanupListeners();

      finish_ = 1;

      break;
    }
    case code_reset_request:
    {
      #ifdef PANIC
      *logofs << "Proxy: PANIC! Proxy reset not supported "
              << "in this version.\n" << logofs_flush;
      #endif

      cerr << "Error" << ": Proxy reset not supported "
           << "in this version.\n";

      HandleCleanup();
    }
    case code_shutdown_request:
    {
      //
      // Time to rest in peace.
      //

      shutdown_ = 1;

      break;
    }
    case code_load_request:
    {
      if (handleLoadFromProxy() < 0)
      {
        return -1;
      }

      break;
    }
    case code_save_request:
    {
      //
      // Don't abort the connection
      // if can't write to disk.
      //

      handleSaveFromProxy();

      break;
    }
    case code_statistics_request:
    {
      int type = *(message + 2);

      if (handleStatisticsFromProxy(type) < 0)
      {
        return -1;
      }

      break;
    }
    case code_statistics_reply:
    {
      operation_ = operation_in_statistics;

      break;
    }
    case code_alert_request:
    {
      HandleAlert(*(message + 2), 1);

      break;
    }
    case code_sync_request:
    {
      int channelId = *(message + 2);

      if (handleSyncFromProxy(channelId) < 0)
      {
        return -1;
      }

      break;
    }
    case code_sync_reply:
    {
      //
      // We are not the one that issued
      // the request.
      //

      #if defined(TEST) || defined(INFO)
      *logofs << "Proxy: PANIC! Received an unexpected "
              << "synchronization reply.\n"
              << logofs_flush;
      #endif

      cerr << "Error" << ": Received an unexpected "
           << "synchronization reply.\n";

      HandleCleanup();
    }
    default:
    {
      #ifdef PANIC
      *logofs << "Proxy: PANIC! Received bad control message number "
              << (unsigned int) *(message + 1) << " with attribute "
              << (unsigned int) *(message + 2) << ".\n" << logofs_flush;
      #endif

      cerr << "Error" << ": Received bad control message number "
           << (unsigned int) *(message + 1) << " with attribute "
           << (unsigned int) *(message + 2) << ".\n";

      HandleCleanup();
    }

  } // End of switch (*(message + 1)) ...

  if (channelType == channel_none)
  {
    return 1;
  }

  //
  // Handle the channel allocation that we
  // left from the main switch case.
  //

  int channelId = *(message + 2);

  //
  // Check if the channel has been dropped.
  //

  if (channels_[channelId] != NULL &&
          (channels_[channelId] -> getDrop() == 1 ||
              channels_[channelId] -> getClosing() == 1))
  {
    #ifdef TEST
    *logofs << "Proxy: Dropping the descriptor FD#"
            << getFd(channelId) << " channel ID#"
            << channelId << ".\n" << logofs_flush;
    #endif

    handleDrop(channelId);
  }

  //
  // Check if the channel is in the valid
  // range.
  //

  int result = checkChannelMap(channelId);

  if (result >= 0)
  {
    result = handleNewConnectionFromProxy(channelType, channelId);
  }

  if (result < 0)
  {
    //
    // Realization of new channel failed.
    // Send channel shutdown message to
    // the peer proxy.
    //

    if (handleControl(code_drop_connection, channelId) < 0)
    {
      return -1;
    }
  }
  else
  {
    int fd =  getFd(channelId);

    if (getReadable(fd) > 0)
    {
      #ifdef TEST
      *logofs << "Proxy: Trying to read immediately "
              << "from descriptor FD#" << fd << ".\n"
              << logofs_flush;
      #endif

      if (handleRead(fd) < 0)
      {
        return -1;
      }
    }
    #ifdef TEST
    *logofs << "Proxy: Nothing to read immediately "
            << "from descriptor FD#" << fd << ".\n"
            << logofs_flush;
    #endif
  }

  return 1;
}

int Proxy::handleRead(int fd, const char *data, int size)
{
  #if defined(TEST) || defined(INFO)
  *logofs << "Proxy: Handling data for connection on FD#"
          << fd << ".\n" << logofs_flush;
  #endif

  if (canRead(fd) == 0)
  {
    #if defined(TEST) || defined(INFO)

    if (getChannel(fd) < 0)
    {
      *logofs << "Proxy: PANIC! Can't read from invalid FD#"
              << fd << ".\n" << logofs_flush;

      HandleCleanup();
    }
    else
    {
      *logofs << "Proxy: WARNING! Read method called for FD#"
              << fd << " but operation is not possible.\n"
              << logofs_flush;
    }

    #endif

    return 0;
  }

  int channelId = getChannel(fd);

  //
  // Let the channel object read all the new data from
  // its file descriptor, isolate messages, compress
  // those messages, and append the compressed form to
  // the encode buffer.
  //

  #if defined(TEST) || defined(INFO)
  *logofs << "Proxy: Reading messages from FD#" << fd
          << " channel ID#" << channelId << ".\n"
          << logofs_flush;
  #endif

  int result = channels_[channelId] -> handleRead(encodeBuffer_, (const unsigned char *) data,
                                                      (unsigned int) size);

  //
  // Even in the case of a failure, write the produced
  // data to the proxy connection. To keep the stores
  // synchronized, the remote side needs to decode any
  // message encoded by this side, also if the X socket
  // was closed in the meanwhile. If this is the case,
  // the decompressed output will be silently discarded.
  //

  if (result < 0)
  {
    #ifdef TEST
    *logofs << "Proxy: Failed to read data from connection FD#"
            << fd << " channel ID#" << channelId << ".\n"
            << logofs_flush;
    #endif

    if (handleFinish(channelId) < 0)
    {
      return -1;
    }
  }

  //
  // Check if there are new splits or
  // motion events to send.
  //

  setSplitTimeout(channelId);
  setMotionTimeout(channelId);

  return 1;
}

int Proxy::handleEvents()
{
  #ifdef TEST
  *logofs << "Proxy: Going to check the events on channels.\n"
          << logofs_flush;
  #endif

  //
  // Check if we can safely write to the
  // proxy link.
  //

  int read = isTimeToRead();

  //
  // Loop on channels and send the pending
  // events. We must copy the list because
  // channels can be removed in the middle
  // of the loop.
  //

  T_list channelList = activeChannels_.copyList();

  for (T_list::iterator j = channelList.begin();
           j != channelList.end(); j++)
  {
    int channelId = *j;

    if (channels_[channelId] == NULL)
    {
      continue;
    }

    //
    // Check if we need to drop the channel.
    //

    if (channels_[channelId] -> getDrop() == 1 ||
            channels_[channelId] -> getClosing() == 1)
    {
      #ifdef TEST
      *logofs << "Proxy: Dropping the descriptor FD#"
              << getFd(channelId) << " channel ID#"
              << channelId << ".\n" << logofs_flush;
      #endif

      if (handleDrop(channelId) < 0)
      {
        return -1;
      }

      continue;
    }
    else if (channels_[channelId] -> getFinish() == 1)
    {
      #ifdef TEST
      *logofs << "Proxy: Skipping finishing "
              << "descriptor FD#" << getFd(channelId)
              << " channel ID#" << channelId << ".\n"
              << logofs_flush;
      #endif

      continue;
    }

    //
    // If the proxy link or the channel is
    // in congestion state, don't handle
    // the further events.
    //

    if (read == 0 || congestions_[channelId] == 1)
    {
      #ifdef TEST

      if (read == 0)
      {
        *logofs << "Proxy: Can't handle events for FD#"
                << getFd(channelId) << " channel ID#"
                << channelId << " with proxy not available.\n"
                << logofs_flush;
      }
      else
      {
        *logofs << "Proxy: Can't handle events for FD#"
                << getFd(channelId) << " channel ID#"
                << channelId << " with channel congested.\n"
                << logofs_flush;
      }

      #endif

      continue;
    }

    //
    // Handle the timeouts on the channel
    // operations.
    //

    int result = 0;

    //
    // Handle the motion events.
    //

    if (result >= 0 && channels_[channelId] -> needMotion() == 1)
    {
      if (isTimeToMotion() == 1)
      {
        #if defined(TEST) || defined(INFO) || defined(FLUSH)

        *logofs << "Proxy: FLUSH! Motion timeout expired after "
                << diffTimestamp(timeouts_.motionTs, getTimestamp())
                << " Ms.\n" << logofs_flush;

        #endif

        result = channels_[channelId] -> handleMotion(encodeBuffer_);

        #ifdef TEST

        if (result < 0)
        {
          *logofs << "Proxy: Failed to handle motion events for FD#"
                  << getFd(channelId) << " channel ID#" << channelId
                  << ".\n" << logofs_flush;
        }

        #endif

        timeouts_.motionTs = nullTimestamp();

        setMotionTimeout(channelId);
      }
      #if defined(TEST) || defined(INFO)
      else if (isTimestamp(timeouts_.motionTs) == 1)
      {
        *logofs << "Proxy: Running with "
                << diffTimestamp(timeouts_.motionTs, getTimestamp())
                << " Ms elapsed since the last motion.\n"
                << logofs_flush;
      }
      #endif
    }

    if (result >= 0 && channels_[channelId] -> needSplit() == 1)
    {
      //
      // Check if it is time to send more splits
      // and how many bytes are going to be sent.
      //

      if (isTimeToSplit() == 1)
      {
        #if defined(TEST) || defined(INFO) || defined(SPLIT)
        *logofs << "Proxy: SPLIT! Split timeout expired after "
                << diffTimestamp(timeouts_.splitTs, getTimestamp())
                << " Ms.\n" << logofs_flush;
        #endif

        #if defined(TEST) || defined(INFO) || defined(SPLIT)

        *logofs << "Proxy: SPLIT! Encoding splits for FD#"
                << getFd(channelId) << " at " << strMsTimestamp()
                << " with " << clientStore_ -> getSplitTotalStorageSize()
                << " total bytes and " << control -> SplitDataPacketLimit
                << " bytes " << "to write.\n"
                << logofs_flush;

        #endif

        result = channels_[channelId] -> handleSplit(encodeBuffer_);

        #ifdef TEST

        if (result < 0)
        {
          *logofs << "Proxy: Failed to handle splits for FD#"
                  << getFd(channelId) << " channel ID#" << channelId
                  << ".\n" << logofs_flush;
        }

        #endif

        timeouts_.splitTs = nullTimestamp();

        setSplitTimeout(channelId);
      }
      #if defined(TEST) || defined(INFO) || defined(SPLIT)
      else if (channels_[channelId] -> needSplit() == 1 &&
                   isTimestamp(timeouts_.splitTs) == 0)
      {
        *logofs << "Proxy: SPLIT! WARNING! Channel for FD#"
                << getFd(channelId) << " has split to send but "
                << "there is no timeout.\n" << logofs_flush;
      }
      else if (isTimestamp(timeouts_.splitTs) == 1)
      {
        *logofs << "Proxy: SPLIT! Running with "
                << diffTimestamp(timeouts_.splitTs, getTimestamp())
                << " Ms elapsed since the last split.\n"
                << logofs_flush;
      }
      #endif
    }

    if (result < 0)
    {
      #ifdef TEST
      *logofs << "Proxy: Error handling events for FD#"
              << getFd(channelId) << " channel ID#"
              << channelId << ".\n" << logofs_flush;
      #endif

      if (handleFinish(channelId) < 0)
      {
        return -1;
      }
    }
  }

  return 1;
}

int Proxy::handleFrame(T_frame_type type)
{
  //
  // Write any outstanding control message, followed by the
  // content of the encode buffer, to the proxy transport.
  //
  // This code assumes that the encode buffer data is at an
  // offset several bytes from start of the buffer, so that
  // the length header and any necessary control bytes can
  // be inserted in front of the data already in the buffer.
  // This is the easiest way to encapsulate header and data
  // together in a single frame.
  //
  // The way framing is implemented is very efficient but
  // inherently limited and does not allow for getting the
  // best performance, especially when running over a fast
  // link. Framing should be rewritten to include the length
  // of the packets in a fixed size header and, possibly,
  // to incapsulate the control messages and the channel's
  // data in a pseudo X protocol message, so that the proxy
  // itself would be treated like any other channel.
  //

  #if defined(TEST) || defined(INFO)

  if (congestion_ == 1)
  {
    //
    // This can happen because there may be control
    // messages to send, like a proxy shutdown mes-
    // sage or a statistics request. All the other
    // cases should be considered an error.
    //

    #ifdef WARNING
    *logofs << "Proxy: WARNING! Data is to be sent while "
            << "congestion is " << congestion_ << ".\n"
            << logofs_flush;
    #endif
  }

  #endif

  //
  // Check if there is any data available on
  // the socket. Recent Linux kernels are very
  // picky. They require that we read often or
  // they assume that the process is non-inter-
  // active.
  //

  if (handleAsyncEvents() < 0)
  {
    return -1;
  }

  //
  // Check if this is a ping, not a data frame.
  //

  if (type == frame_ping)
  {
    if (handleToken(frame_ping) < 0)
    {
      return -1;
    }
  }

  unsigned int dataLength = encodeBuffer_.getLength();

  #ifdef DEBUG
  *logofs << "Proxy: Data length is " << dataLength
          << " control length is " << controlLength_
          << ".\n" << logofs_flush;
  #endif

  if (dataLength > 0)
  {
    //
    // If this is a generic channel we need
    // to add the completion bits. Data can
    // also have been encoded because of a
    // statistics request, even if no output
    // channel was currently selected.
    //

    if (outputChannel_ != -1)
    {
      #if defined(TEST) || defined(INFO)

      if (channels_[outputChannel_] == NULL)
      {
        *logofs << "Proxy: PANIC! A new frame was requested "
                << "but the channel is invalid.\n"
                << logofs_flush;

        HandleCleanup();
      }

      #endif

      channels_[outputChannel_] -> handleCompletion(encodeBuffer_);

      dataLength = encodeBuffer_.getLength();
    }
  }
  else if (controlLength_ == 0)
  {
    #if defined(TEST) || defined(INFO)

    *logofs << "Proxy: PANIC! A new frame was requested "
            << "but there is no data to write.\n"
            << logofs_flush;

    HandleCleanup();

    #endif

    return 0;
  }

  #ifdef DEBUG
  *logofs << "Proxy: Data length is now " << dataLength
          << " control length is " << controlLength_
          << ".\n" << logofs_flush;
  #endif

  //
  // Check if this frame needs to carry a new
  // token request.
  //

  if (type == frame_data)
  {
    if (handleToken(frame_data) < 0)
    {
      return -1;
    }
  }

  #ifdef DEBUG
  *logofs << "Proxy: Adding a new frame for the remote proxy.\n"
          << logofs_flush;
  #endif

  unsigned char temp[5];

  unsigned int lengthLength = 0;
  unsigned int shift = dataLength;

  while (shift)
  {
    temp[lengthLength++] = (unsigned char) (shift & 0x7f);

    shift >>= 7;
  }

  unsigned char *data = encodeBuffer_.getData();

  unsigned char *outputMessage = data - (controlLength_ + lengthLength);

  unsigned char *nextDest = outputMessage;

  for (int i = 0; i < controlLength_; i++)
  {
    *nextDest++ = controlCodes_[i];
  }

  for (int j = lengthLength - 1; j > 0; j--)
  {
    *nextDest++ = (temp[j] | 0x80);
  }

  if (lengthLength)
  {
    *nextDest++ = temp[0];
  }

  unsigned int outputLength = dataLength + controlLength_ + lengthLength;

  #if defined(TEST) || defined(INFO)
  *logofs << "Proxy: Produced plain output for " << dataLength << "+"
          << controlLength_ << "+" << lengthLength << " out of "
          << outputLength << " bytes.\n" << logofs_flush;
  #endif

  #if defined(TEST) || defined(INFO) || defined(FLUSH) || defined(TIME)

  T_timestamp nowTs = getTimestamp();

  *logofs << "Proxy: FLUSH! Immediate with blocked " << transport_ ->
             blocked() << " length " << transport_ -> length()
          << " new " << outputLength << " flushable " << transport_ ->
             flushable() << " tokens " << tokens_[token_control].remaining
          << " after " << diffTimestamp(timeouts_.writeTs, nowTs)
          << " Ms.\n" << logofs_flush;

  *logofs << "Proxy: FLUSH! Immediate flush to proxy FD#" << fd_
          << " of " << outputLength << " bytes at " << strMsTimestamp()
          << " with priority " << priority_ << ".\n" << logofs_flush;

  *logofs << "Proxy: FLUSH! Current bitrate is "
          << statistics -> getBitrateInShortFrame() << " with "
          << statistics -> getBitrateInLongFrame() << " in the "
          << "long frame and top " << statistics ->
             getTopBitrate() << ".\n" << logofs_flush;
  #endif

  statistics -> addWriteOut();

  int result = transport_ -> write(write_immediate, outputMessage, outputLength);

  #ifdef TIME

  if (diffTimestamp(timeouts_.writeTs, nowTs) > 50)
  {
    *logofs << "Proxy: WARNING! TIME! Data written to proxy FD#"
            << fd_ << " at " << strMsTimestamp() << " after "
            << diffTimestamp(timeouts_.writeTs, nowTs)
            << " Ms.\n" << logofs_flush;
  }

  #endif

  #ifdef DUMP
  *logofs << "Proxy: Sent " << outputLength << " bytes of data "
          << "with checksum ";

  DumpChecksum(outputMessage, outputLength);

  *logofs << " on proxy FD#" << fd_ << ".\n" << logofs_flush;
  #endif

  #ifdef DUMP
  *logofs << "Proxy: Partial checksums are:\n";

  DumpBlockChecksums(outputMessage, outputLength, 256);

  *logofs << logofs_flush;
  #endif

  //
  // Clean up the encode buffer and
  // bring it to the initial size.
  //

  encodeBuffer_.fullReset();

  //
  // Close the connection if we got
  // an error.
  //

  if (result < 0)
  {
    #ifdef TEST
    *logofs << "Proxy: Failed write to proxy FD#"
            << fd_ << ".\n" << logofs_flush;
    #endif

    return -1;
  }

  //
  // Account for the data frame and the
  // framing overhead.
  //

  if (dataLength > 0)
  {
    statistics -> addFrameOut();
  }

  statistics -> addFramingBits((controlLength_ + lengthLength) << 3);

  controlLength_ = 0;

  //
  // Reset all buffers, counters and the
  // priority flag.
  //

  handleResetFlush();

  //
  // Check if more data became available
  // after writing.
  //

  if (handleAsyncEvents() < 0)
  {
    return -1;
  }

  //
  // Drain the proxy link if we are in
  // congestion state.
  //
  // if (needDrain() == 1 && draining_ == 0)
  // {
  //   if (handleDrain() < 0)
  //   {
  //     return -1;
  //   }
  // }
  //

  return result;
}

int Proxy::handleFlush()
{
  //
  // We can have data in the encode buffer or
  // control bytes to send. In the case make
  // up a new frame.
  //

  if (encodeBuffer_.getLength() + controlLength_ > 0)
  {
    #if defined(TEST) || defined(INFO)
    *logofs << "Proxy: Flushing data in the encode buffer.\n"
            << logofs_flush;
    #endif

    priority_ = 1;

    if (handleFrame(frame_data) < 0)
    {
      return -1;
    }
  }

  //
  // Check if we have something to write.
  //

  if (transport_ -> length() + transport_ -> flushable() == 0)
  {
    #if defined(TEST) || defined(INFO)
    *logofs << "Proxy: Nothing else to flush for proxy FD#"
            << fd_ << ".\n" << logofs_flush;
    #endif

    return 0;
  }

  #if defined(TEST) || defined(INFO)

  if (transport_ -> blocked() == 0)
  {
    #ifdef PANIC
    *logofs << "Proxy: PANIC! Proxy descriptor FD#" << fd_
            << " has data to flush but the transport "
            << "is not blocked.\n" << logofs_flush;
    #endif

    cerr << "Error" << ": Proxy descriptor FD#" << fd_
         << " has data to flush but the transport "
         << "is not blocked.\n";

    HandleCleanup();
  }

  #endif

  #if defined(TEST) || defined(INFO) || defined(FLUSH)
  *logofs << "Proxy: FLUSH! Deferred with blocked " << transport_ ->
             blocked() << " length " << transport_ -> length()
          << " flushable " << transport_ -> flushable() << " tokens "
          << tokens_[token_control].remaining << ".\n"
          << logofs_flush;

  *logofs << "Proxy: FLUSH! Deferred flush to proxy FD#" << fd_
          << " of " << transport_ -> length() + transport_ ->
             flushable() << " bytes at " << strMsTimestamp()
          << " with priority " << priority_ << ".\n"
          << logofs_flush;

  *logofs << "Proxy: FLUSH! Current bitrate is "
          << statistics -> getBitrateInShortFrame() << " with "
          << statistics -> getBitrateInLongFrame() << " in the "
          << "long frame and top " << statistics ->
             getTopBitrate() << ".\n" << logofs_flush;
  #endif

  statistics -> addWriteOut();

  int result = transport_ -> flush();

  if (result < 0)
  {
    return -1;
  }

  //
  // Reset the counters and update the
  // timestamp of the last write.
  //

  handleResetFlush();

  return result;
}

int Proxy::handleDrain()
{
  //
  // If the proxy is run in the same process
  // as SSH, we can't block or the program
  // would not have a chance to read or write
  // its data.
  //

  if (control -> LinkEncrypted == 1)
  {
    return 0;
  }

  if (needDrain() == 0 || draining_ == 1)
  {
    #if defined(TEST) || defined(INFO)

    if (draining_ == 1)
    {
      *logofs << "Proxy: WARNING! Already draining proxy FD#"
              << fd_ << " at " << strMsTimestamp() << ".\n"
              << logofs_flush;
    }
    else
    {
      *logofs << "Proxy: WARNING! No need to drain proxy FD#"
              << fd_ << " with congestion " << congestion_
              << " length " << transport_ -> length()
              << " and blocked " << transport_ -> blocked()
              << ".\n" << logofs_flush;
    }

    #endif

    return 0;
  }

  draining_ = 1;

  #if defined(TEST) || defined(INFO)
  *logofs << "Proxy: Going to drain the proxy FD#" << fd_
          << " at " << strMsTimestamp() << ".\n"
          << logofs_flush;
  #endif

  int timeout = control -> PingTimeout / 2;

  T_timestamp startTs = getNewTimestamp();

  T_timestamp nowTs = startTs;

  int remaining;
  int result;

  //
  // Keep draining the proxy socket while
  // reading the incoming messages until
  // the timeout is expired.
  // 

  for (;;)
  {
    remaining = timeout - diffTimestamp(startTs, nowTs);

    if (remaining <= 0)
    {
      #if defined(TEST) || defined(INFO)
      *logofs << "Proxy: Timeout raised while draining "
              << "FD#" << fd_ << " at " << strMsTimestamp()
              << " after " << diffTimestamp(startTs, nowTs)
              << " Ms.\n" << logofs_flush;
      #endif

      result = 0;

      goto ProxyDrainEnd;
    }

    if (transport_ -> length() > 0)
    {
      #if defined(TEST) || defined(INFO)
      *logofs << "Proxy: Trying to write to FD#" << fd_
              << " at " << strMsTimestamp() << " with length "
              << transport_ -> length() << " and "
              << remaining << " Ms remaining.\n"
              << logofs_flush;
      #endif

      result = transport_ -> drain(0, remaining);

      if (result == -1)
      {
        result = -1;

        goto ProxyDrainEnd;
      }
      else if (result == 0 && transport_ -> readable() > 0)
      {
        #if defined(TEST) || defined(INFO)
        *logofs << "Proxy: Decoding more data from proxy FD#"
                << fd_ << " at " << strMsTimestamp() << " with "
                << transport_ -> length() << " bytes to write and "
                << transport_ -> readable() << " readable.\n"
                << logofs_flush;
        #endif

        if (handleRead() < 0)
        {
          result = -1;

          goto ProxyDrainEnd;
        }
      }
      #if defined(TEST) || defined(INFO)
      else if (result == 1)
      {
        *logofs << "Proxy: Transport for proxy FD#" << fd_
                << " drained down to " << transport_ -> length()
                << " bytes.\n" << logofs_flush;
      }
      #endif
    }
    else
    {
      #if defined(TEST) || defined(INFO)
      *logofs << "Proxy: Waiting for more data from proxy "
              << "FD#" << fd_ << " at " << strMsTimestamp()
              << " with " << remaining << " Ms remaining.\n"
              << logofs_flush;
      #endif


      result = transport_ -> wait(remaining);

      if (result == -1)
      {
        result = -1;

        goto ProxyDrainEnd;
      }
      else if (result > 0)
      {
        #if defined(TEST) || defined(INFO)
        *logofs << "Proxy: Decoding more data from proxy FD#"
                << fd_ << " at " << strMsTimestamp() << " with "
                << transport_ -> readable() << " bytes readable.\n"
                << logofs_flush;
        #endif

        if (handleRead() < 0)
        {
          result = -1;

          goto ProxyDrainEnd;
        }
      }
    }

    //
    // Check if we finally got the tokens
    // that would allow us to come out of
    // the congestion state.
    //

    if (needDrain() == 0)
    {
      #if defined(TEST) || defined(INFO)
      *logofs << "Proxy: Got decongestion for proxy FD#"
              << fd_ << " at " << strMsTimestamp() << " after "
              << diffTimestamp(startTs, getTimestamp())
              << " Ms.\n" << logofs_flush;
      #endif

      result = 1;

      goto ProxyDrainEnd;
    }

    nowTs = getNewTimestamp();
  }

ProxyDrainEnd:

  draining_ = 0;

  return result;
}

int Proxy::handleFlush(int fd)
{
  int channelId = getChannel(fd);

  if (channelId < 0 || channels_[channelId] == NULL)
  {
    #ifdef TEST
    *logofs << "Proxy: WARNING! Skipping flush on invalid "
            << "descriptor FD#" << fd << " channel ID#"
            << channelId << ".\n" << logofs_flush;
    #endif

    return 0;
  }
  else if (channels_[channelId] -> getFinish() == 1)
  {
    #ifdef TEST
    *logofs << "Proxy: Skipping flush on finishing "
            << "descriptor FD#" << fd << " channel ID#"
            << channelId << ".\n" << logofs_flush;
    #endif

    return 0;
  }

  #ifdef TEST
  *logofs << "Proxy: Going to flush FD#" << fd
          << " with blocked " << transports_[channelId] -> blocked()
          << " length " << transports_[channelId] -> length()
          << ".\n" << logofs_flush;
  #endif

  if (channels_[channelId] -> handleFlush() < 0)
  {
    #ifdef TEST
    *logofs << "Proxy: Failed to flush data to FD#"
            << getFd(channelId) << " channel ID#"
            << channelId << ".\n" << logofs_flush;
    #endif

    handleFinish(channelId);

    return -1;
  }

  return 1;
}

int Proxy::handleStatistics(int type, ostream *stream)
{
  if (stream == NULL || control -> EnableStatistics == 0)
  {
    #ifdef WARNING
    *logofs << "Proxy: WARNING! Cannot produce statistics "
            << " for proxy FD#" << fd_ << ". Invalid settings "
            << "for statistics or stream.\n" << logofs_flush;
    #endif

    return 0;
  }
  else if (currentStatistics_ != NULL)
  {
    //
    // Need to update the stream pointer as the
    // previous one could have been destroyed.
    //
 
    #ifdef WARNING
    *logofs << "Proxy: WARNING! Replacing stream while producing "
            << "statistics in stream at " << currentStatistics_
            << " for proxy FD#" << fd_ << ".\n"
            << logofs_flush;
    #endif
  }

  currentStatistics_ = stream;

  //
  // Get statistics of remote peer.
  //

  if (handleControl(code_statistics_request, type) < 0)
  {
    return -1;
  }

  return 1;
}

int Proxy::handleStatisticsFromProxy(int type)
{
  if (needFlush() == 1)
  {
    #if defined(TEST) || defined(INFO) || defined(FLUSH)
    *logofs << "Proxy: WARNING! Data for the previous "
            << "channel ID#" << outputChannel_
            << " flushed in statistics.\n"
            << logofs_flush;
    #endif

    if (handleFrame(frame_data) < 0)
    {
      return -1;
    }
  }

  if (control -> EnableStatistics == 1)
  {
    //
    // Allocate a buffer for the output.
    //

    char *buffer = new char[STATISTICS_LENGTH];
 
    *buffer = '\0';

    if (control -> ProxyMode == proxy_client)
    {
      #ifdef TEST
      *logofs << "Proxy: Producing "
              << (type == TOTAL_STATS ? "total" : "partial")
              << " client statistics for proxy FD#"
              << fd_ << ".\n" << logofs_flush;
      #endif

      statistics -> getClientProtocolStats(type, buffer);

      statistics -> getClientOverallStats(type, buffer);
    }
    else
    {
      #ifdef TEST
      *logofs << "Proxy: Producing "
              << (type == TOTAL_STATS ? "total" : "partial")
              << " server statistics for proxy FD#"
              << fd_ << ".\n" << logofs_flush;
      #endif

      statistics -> getServerProtocolStats(type, buffer);
    }

    if (type == PARTIAL_STATS)
    {
      statistics -> resetPartialStats();
    }

    unsigned int length = strlen((char *) buffer) + 1;

    encodeBuffer_.encodeValue(type, 8);

    encodeBuffer_.encodeValue(length, 32);

    #ifdef TEST
    *logofs << "Proxy: Encoding " << length
            << " bytes of statistics data for proxy FD#"
            << fd_ << ".\n" << logofs_flush;
    #endif

    encodeBuffer_.encodeMemory((unsigned char *) buffer, length);

    //
    // Account statistics data as framing bits.
    //

    statistics -> addFramingBits(length << 3);

    delete [] buffer;
  }
  else
  {
    #ifdef WARNING
    *logofs << "Proxy: WARNING! Got statistics request "
            << "but local statistics are disabled.\n"
            << logofs_flush;
    #endif

    cerr << "Warning" << ": Got statistics request "
         << "but local statistics are disabled.\n";

    type = NO_STATS;

    encodeBuffer_.encodeValue(type, 8);

    #ifdef TEST
    *logofs << "Proxy: Sending error code to remote proxy on FD#"
            << fd_ << ".\n" << logofs_flush;
    #endif
  }

  //
  // The next write will flush the statistics
  // data and the control message.
  //

  if (handleControl(code_statistics_reply, type) < 0)
  {
    return -1;
  }

  return 1;
}

int Proxy::handleStatisticsFromProxy(const unsigned char *message, unsigned int length)
{
  if (currentStatistics_ == NULL)
  {
    #ifdef WARNING
    *logofs << "Proxy: WARNING! Unexpected statistics data received "
            << "from remote proxy on FD#" << fd_ << ".\n"
            << logofs_flush;
    #endif

    cerr << "Warning" << ": Unexpected statistics data received "
         << "from remote proxy.\n";

    return 0;
  }

  //
  // Allocate the decode buffer and at least
  // the 'type' field to see if there was an
  // error.
  //

  DecodeBuffer decodeBuffer(message, length);

  unsigned int type;

  decodeBuffer.decodeValue(type, 8);

  if (type == NO_STATS)
  {
    #ifdef PANIC
    *logofs << "Proxy: PANIC! Couldn't get statistics from remote "
            << "proxy on FD#" << fd_ << ".\n" << logofs_flush;
    #endif

    cerr << "Error" << ": Couldn't get statistics from remote proxy.\n";
  }
  else if (type != TOTAL_STATS && type != PARTIAL_STATS)
  {
    #ifdef PANIC
    *logofs << "Proxy: PANIC! Cannot produce statistics "
            << "with qualifier '" << type << "'.\n"
            << logofs_flush;
    #endif

    cerr << "Error" << ": Cannot produce statistics "
         << "with qualifier '" << type << "'.\n";

    return -1;
  }
  else
  {
    unsigned int size;

    decodeBuffer.decodeValue(size, 32);

    char *buffer = new char[STATISTICS_LENGTH];

    *buffer = '\0';

    if (control -> EnableStatistics == 1)
    {
      if (control -> ProxyMode == proxy_client)
      {
        #ifdef TEST
        *logofs << "Proxy: Finalizing "
                << (type == TOTAL_STATS ? "total" : "partial")
                << " client statistics for proxy FD#"
                << fd_ << ".\n" << logofs_flush;
        #endif

        statistics -> getClientCacheStats(type, buffer);

        #ifdef TEST
        *logofs << "Proxy: Decoding " << size
                << " bytes of statistics data for proxy FD#"
                << fd_ << ".\n" << logofs_flush;
        #endif

        strncat(buffer, (char *) decodeBuffer.decodeMemory(size), size);

        statistics -> getClientProtocolStats(type, buffer);

        statistics -> getClientOverallStats(type, buffer);
      }
      else
      {
        #ifdef TEST
        *logofs << "Proxy: Finalizing "
                << (type == TOTAL_STATS ? "total" : "partial")
                << " server statistics for proxy FD#"
                << fd_ << ".\n" << logofs_flush;
        #endif

        statistics -> getServerCacheStats(type, buffer);

        statistics -> getServerProtocolStats(type, buffer);

        #ifdef TEST
        *logofs << "Proxy: Decoding " << size
                << " bytes of statistics data for proxy FD#"
                << fd_ << ".\n" << logofs_flush;
        #endif

        strncat(buffer, (char *) decodeBuffer.decodeMemory(size), size);
      }

      if (type == PARTIAL_STATS)
      {
        statistics -> resetPartialStats();
      }

      *currentStatistics_ << buffer;

      //
      // Mark the end of text to help external parsing.
      //

      *currentStatistics_ << '\4';

      *currentStatistics_ << flush;
    }
    else
    {
      //
      // It can be that statistics were enabled at the time
      // we issued the request (otherwise we could not have
      // set the stream), but now they have been disabled
      // by user. We must decode statistics data if we want
      // to keep the connection.
      //

      #ifdef TEST
      *logofs << "Proxy: Discarding " << size
              << " bytes of statistics data for proxy FD#"
              << fd_ << ".\n" << logofs_flush;
      #endif

      strncat(buffer, (char *) decodeBuffer.decodeMemory(size), size);
    }

    delete [] buffer;
  }

  currentStatistics_ = NULL;

  return 1;
}

int Proxy::handleNegotiation(const unsigned char *message, unsigned int length)
{
  #ifdef PANIC
  *logofs << "Proxy: PANIC! Writing data during proxy "
          << "negotiation is not implemented.\n"
          << logofs_flush;
  #endif

  cerr << "Error" << ": Writing data during proxy "
       << "negotiation is not implemented.\n";

  return -1;
}

int Proxy::handleNegotiationFromProxy(const unsigned char *message, unsigned int length)
{
  #ifdef PANIC
  *logofs << "Proxy: PANIC! Reading data during proxy "
          << "negotiation is not implemented.\n"
          << logofs_flush;
  #endif

  cerr << "Error" << ": Reading data during proxy "
       << "negotiation is not implemented.\n";

  return -1;
}

int Proxy::handleAlert(int alert)
{
  if (handleControl(code_alert_request, alert) < 0)
  {
    return -1;
  }

  return 1;
}

int Proxy::handleCloseConnection(int clientFd)
{
  int channelId = getChannel(clientFd);

  if (channels_[channelId] != NULL &&
          channels_[channelId] -> getFinish() == 0)
  {
    #ifdef TEST
    *logofs << "Proxy: Closing down the channel for FD#"
            << clientFd << ".\n" << logofs_flush;
    #endif

    if (handleFinish(channelId) < 0)
    {
      return -1;
    }

    return 1;
  }

  return 0;
}

int Proxy::handleCloseAllXConnections()
{
  #ifdef TEST
  *logofs << "Proxy: Closing down any remaining X channel.\n"
          << logofs_flush;
  #endif

  T_list &channelList = activeChannels_.getList();

  for (T_list::iterator j = channelList.begin();
           j != channelList.end(); j++)
  {
    int channelId = *j;

    if (channels_[channelId] != NULL &&
            channels_[channelId] -> getType() == channel_x11 &&
                channels_[channelId] -> getFinish() == 0)
    {
      #ifdef TEST
      *logofs << "Proxy: Closing down the channel for FD#"
              << getFd(channelId) << ".\n" << logofs_flush;
      #endif

      if (handleFinish(channelId) < 0)
      {
        return -1;
      }
    }
  }

  return 1;
}

int Proxy::handleCloseAllListeners()
{
  if (control -> isProtoStep7() == 1)
  {
    if (finish_ == 0)
    {
      #ifdef TEST
      *logofs << "Proxy: Closing down all remote listeners.\n"
              << logofs_flush;
      #endif

      if (handleControl(code_finish_listeners) < 0)
      {
        return -1;
      }

      finish_ = 1;
    }
  }
  else
  {
    #ifdef TEST
    *logofs << "Proxy: WARNING! Not sending unsupported "
            << "'code_finish_listeners' message.\n"
            << logofs_flush;
    #endif

    finish_ = 1;
  }

  return 1;
}

void Proxy::handleResetAlert()
{
  if (alert_ != 0)
  {
    #ifdef TEST
    *logofs << "Proxy: The proxy alert '" << alert_
            << "' was displaced.\n" << logofs_flush;
    #endif

    alert_ = 0;
  }

  T_list &channelList = activeChannels_.getList();

  for (T_list::iterator j = channelList.begin();
           j != channelList.end(); j++)
  {
    int channelId = *j;

    if (channels_[channelId] != NULL)
    {
      channels_[channelId] -> handleResetAlert();
    }
  }
}

int Proxy::handleFinish(int channelId)
{
  //
  // Send any outstanding encoded data and
  // do any finalization needed on the
  // channel.
  //

  if (needFlush(channelId) == 1)
  {
    if (channels_[channelId] -> getFinish() == 1)
    {
      #ifdef WARNING
      *logofs << "Proxy: WARNING! The finishing channel ID#"
              << channelId << " has data to flush.\n"
              << logofs_flush;
      #endif
    }

    #if defined(TEST) || defined(INFO) || defined(FLUSH)
    *logofs << "Proxy: WARNING! Flushing data for the "
            << "finishing channel ID#" << channelId
            << ".\n" << logofs_flush;
    #endif

    if (handleFrame(frame_data) < 0)
    {
      return -1;
    }
  }

  //
  // Reset the congestion state and the
  // timeouts, if needed.
  //

  congestions_[channelId] = 0;

  setSplitTimeout(channelId);
  setMotionTimeout(channelId);

  if (channels_[channelId] -> getFinish() == 0)
  {
    channels_[channelId] -> handleFinish();

    //
    // Force a failure in the case somebody
    // would try to read from the channel.
    //

    shutdown(getFd(channelId), SHUT_RD);

    //
    // If the failure was not originated by
    // the remote, send a channel shutdown
    // message.
    //

    if (channels_[channelId] -> getClosing() == 0)
    {
      #ifdef TEST
      *logofs << "Proxy: Finishing channel for FD#"
              << getFd(channelId) << " channel ID#"
              << channelId << " because of failure.\n"
              << logofs_flush;
      #endif

      if (handleControl(code_finish_connection, channelId) < 0)
      {
        return -1;
      }
    }
  }

  return 1;
}

int Proxy::handleFinishFromProxy(int channelId)
{
  //
  // Check if this channel has pending
  // data to send.
  //

  if (needFlush(channelId) == 1)
  {
    #if defined(TEST) || defined(INFO) || defined(FLUSH)
    *logofs << "Proxy: WARNING! Flushing data for the "
            << "finishing channel ID#" << channelId
            << ".\n" << logofs_flush;
    #endif

    if (handleFrame(frame_data) < 0)
    {
      return -1;
    }
  }

  //
  // Mark the channel. We will free its
  // resources at the next loop and will
  // send the drop message to the remote.
  //

  if (channels_[channelId] -> getClosing() == 0)
  {
    #ifdef TEST
    *logofs << "Proxy: Marking channel for FD#"
            << getFd(channelId) << " channel ID#"
            << channelId << " as closing.\n"
            << logofs_flush;
    #endif

    channels_[channelId] -> handleClosing();
  }

  if (channels_[channelId] -> getFinish() == 0)
  {
    #ifdef TEST
    *logofs << "Proxy: Finishing channel for FD#"
            << getFd(channelId) << " channel ID#"
            << channelId << " because of proxy.\n"
            << logofs_flush;
    #endif

    channels_[channelId] -> handleFinish();
  }

  if (handleFinish(channelId) < 0)
  {
    return -1;
  }

  return 1;
}

int Proxy::handleDropFromProxy(int channelId)
{
  //
  // Only mark the channel.
  //

  #ifdef TEST
  *logofs << "Proxy: Marking channel for FD#"
          << getFd(channelId) << " channel ID#"
          << channelId << " as being dropped.\n"
          << logofs_flush;
  #endif

  if (channels_[channelId] -> getDrop() == 0)
  {
    channels_[channelId] -> handleDrop();
  }

  return 1;
}

//
// Close the channel and deallocate all its
// resources.
//

int Proxy::handleDrop(int channelId)
{
  //
  // Check if this channel has pending
  // data to send.
  //

  if (needFlush(channelId) == 1)
  {
    if (channels_[channelId] -> getFinish() == 1)
    {
      #ifdef WARNING
      *logofs << "Proxy: WARNING! The dropping channel ID#"
              << channelId << " has data to flush.\n"
              << logofs_flush;
      #endif
    }

    #if defined(TEST) || defined(INFO) || defined(FLUSH)
    *logofs << "Proxy: WARNING! Flushing data for the "
            << "dropping channel ID#" << channelId
            << ".\n" << logofs_flush;
    #endif

    if (handleFrame(frame_data) < 0)
    {
      return -1;
    }
  }

  #ifdef TEST
  *logofs << "Proxy: Dropping channel for FD#"
          << getFd(channelId) << " channel ID#"
          << channelId << ".\n" << logofs_flush;
  #endif

  if (channels_[channelId] -> getFinish() == 0)
  {
    #ifdef WARNING
    *logofs << "Proxy: WARNING! The channel for FD#"
            << getFd(channelId) << " channel ID#"
            << channelId << " was not marked as "
            << "finishing.\n" << logofs_flush;
    #endif

    cerr << "Warning" << ": The channel for FD#"
         << getFd(channelId) << " channel ID#"
         << channelId << " was not marked as "
         << "finishing.\n";

    channels_[channelId] -> handleFinish();
  }

  //
  // Send the channel shutdown message
  // to the peer proxy.
  //

  if (channels_[channelId] -> getClosing() == 1)
  {
    if (handleControl(code_drop_connection, channelId) < 0)
    {
      return -1;
    }
  }

  //
  // Get rid of the channel.
  //

  if (channels_[channelId] -> getType() != channel_x11)
  {
    #ifdef TEST
    *logofs << "Proxy: Closed connection to "
            << getTypeName(channels_[channelId] -> getType())
            << " server.\n" << logofs_flush;
    #endif

    cerr << "Info" << ": Closed connection to "
         << getTypeName(channels_[channelId] -> getType())
         << " server.\n";
  }

  delete channels_[channelId];
  channels_[channelId] = NULL;

  cleanupChannelMap(channelId);

  //
  // Get rid of the transport.
  //

  deallocateTransport(channelId);

  congestions_[channelId] = 0;

  decreaseChannels(channelId);

  //
  // Check if the channel was the
  // one currently selected for
  // output.
  //

  if (outputChannel_ == channelId)
  {
    outputChannel_ = -1;
  }
  
  return 1;
}

//
// Send an empty message to the remote peer
// to verify if the link is alive and let
// the remote proxy detect a congestion.
//

int Proxy::handlePing()
{
  T_timestamp nowTs = getTimestamp();

  #if defined(DEBUG) || defined(PING)

  *logofs << "Proxy: Checking ping at "
          << strMsTimestamp(nowTs) << logofs_flush;

  *logofs << " with last loop at "
          << strMsTimestamp(timeouts_.loopTs) << ".\n"
          << logofs_flush;

  *logofs << "Proxy: Last bytes in at "
          << strMsTimestamp(timeouts_.readTs) << logofs_flush;

  *logofs << " last bytes out at "
          << strMsTimestamp(timeouts_.writeTs) << ".\n"
          << logofs_flush;

  *logofs << "Proxy: Last ping at "
          << strMsTimestamp(timeouts_.pingTs) << ".\n"
          << logofs_flush;

  #endif

  //
  // Be sure we take into account any clock drift. This
  // can be caused by the user changing the system timer
  // or by small adjustments introduced by the operating
  // system making the clock go backward.
  //

  if (checkDiffTimestamp(timeouts_.loopTs, nowTs) == 0)
  {
    #ifdef WARNING
    *logofs << "Proxy: WARNING! Detected drift in system "
            << "timer. Resetting to current time.\n"
            << logofs_flush;
    #endif

    timeouts_.pingTs  = nowTs;
    timeouts_.readTs  = nowTs;
    timeouts_.writeTs = nowTs;
  }

  //
  // Check timestamp of last read from remote proxy. It can
  // happen that we stayed in the main loop long enough to
  // have idle timeout expired, for example if the proxy was
  // stopped and restarted or because of an extremely high
  // load of the system. In this case we don't complain if
  // there is something new to read from the remote.
  //

  int diffIn = diffTimestamp(timeouts_.readTs, nowTs);

  if (diffIn >= (control -> PingTimeout * 2) -
          control -> LatencyTimeout)
  {
    //
    // Force a read to detect whether the remote proxy
    // aborted the connection.
    //

    int result = handleRead();

    if (result < 0)
    {
      #if defined(TEST) || defined(INFO) || defined(PING)
      *logofs << "Proxy: WARNING! Detected shutdown waiting "
              << "for the ping after " << diffIn / 1000
              << " seconds.\n" << logofs_flush;
      #endif

      return -1;
    }
    else if (result > 0)
    {
      diffIn = diffTimestamp(timeouts_.readTs, nowTs);

      if (handleFlush() < 0)
      {
        return -1;
      }
    }
  }

  if (diffIn >= (control -> PingTimeout * 2) -
          control -> LatencyTimeout)
  {
    #if defined(TEST) || defined(INFO) || defined(PING)
    *logofs << "Proxy: Detected congestion at "
            << strMsTimestamp() << " with " << diffIn / 1000
            << " seconds since the last read.\n"
            << logofs_flush;
    #endif

    //
    // There are two types of proxy congestion. The first,
    // affecting the ability of the proxy to write the
    // encoded data to the network, is controlled by the
    // congestion_ flag. The flag is raised when no data
    // is received from the remote proxy within a timeout.
    // On the X client side, the flag is also raised when
    // the proxy runs out of tokens.
    //

    if (control -> ProxyMode == proxy_server)
    {
      //
      // At X server side we must return to read data
      // from the channels after a while, because we
      // need to give a chance to the channel to read
      // the key sequence CTRL+ALT+SHIFT+ESC.
      //

      if (congestion_ == 0)
      {
        #if defined(TEST) || defined(INFO)
        *logofs << "Proxy: Forcibly entering congestion due to "
                << "timeout with " << tokens_[token_control].remaining
                << " tokens.\n" << logofs_flush;
        #endif

        congestion_ = 1;
      }
      else
      {
        #if defined(TEST) || defined(INFO)
        *logofs << "Proxy: Forcibly exiting congestion due to "
                << "timeout with " << tokens_[token_control].remaining
                << " tokens.\n" << logofs_flush;
        #endif

        congestion_ = 0;
      }
    }
    else
    {
      #if defined(TEST) || defined(INFO)

      if (congestion_ == 0)
      {
        *logofs << "Proxy: Entering congestion due to timeout "
                << "with " << tokens_[token_control].remaining
                << " tokens.\n" << logofs_flush;
      }

      #endif

      congestion_ = 1;
    }

    if (control -> ProxyTimeout > 0 &&
            diffIn >= (control -> ProxyTimeout -
                control -> LatencyTimeout))
    {
      #ifdef PANIC
      *logofs << "Proxy: PANIC! No data received from "
              << "remote proxy on FD#" << fd_ << " within "
              << (diffIn + control -> LatencyTimeout) / 1000
              << " seconds.\n" << logofs_flush;
      #endif

      cerr << "Error" << ": No data received from remote "
           << "proxy within " << (diffIn + control ->
              LatencyTimeout) / 1000 << " seconds.\n";

      HandleAbort();
    }
    else
    {
      #if defined(TEST) || defined(INFO)
      *logofs << "Proxy: WARNING! No data received from "
              << "remote proxy on FD#" << fd_ << " since "
              << diffIn << " Ms.\n" << logofs_flush;
      #endif

      if (control -> ProxyTimeout > 0 &&
              isTimestamp(timeouts_.alertTs) == 0 &&
                  diffIn >= (control -> ProxyTimeout -
                      control -> LatencyTimeout) / 4)
      {
        //
        // If we are in the middle of a shutdown
        // procedure but the remote is not resp-
        // onding, force the closure of the link.
        //

        if (finish_ != 0)
        {
          #ifdef PANIC
          *logofs << "Proxy: PANIC! No response received from "
                  << "the remote proxy on FD#" << fd_ << " while "
                  << "waiting for the shutdown.\n"
                  << logofs_flush;
          #endif

          cerr << "Error" << ": No response received from remote "
               << "proxy while waiting for the shutdown.\n";

          HandleAbort();
        }
        else
        {
          cerr << "Warning" << ": No data received from remote "
               << "proxy within " << (diffIn + control ->
                  LatencyTimeout) / 1000 << " seconds.\n";

          if (alert_ == 0)
          {
            if (control -> ProxyMode == proxy_client)
            {
              alert_ = CLOSE_DEAD_PROXY_CONNECTION_CLIENT_ALERT;
            }
            else
            {
              alert_ = CLOSE_DEAD_PROXY_CONNECTION_SERVER_ALERT;
            }

            HandleAlert(alert_, 1);
          }

          timeouts_.alertTs = nowTs;
        }
      }
    }
  }

  //
  // Check if we need to update the congestion
  // counter.
  //

  int diffOut = diffTimestamp(timeouts_.writeTs, nowTs);

  if (agent_ != nothing && congestions_[agent_] == 0 &&
          statistics -> getCongestionInFrame() >= 1 &&
              diffOut >= (control -> IdleTimeout -
                  control -> LatencyTimeout * 5))
  {
    #if defined(TEST) || defined(INFO) || defined(PING)
    *logofs << "Proxy: Forcing an update of the "
            << "congestion counter after timeout.\n"
            << logofs_flush;
    #endif

    statistics -> updateCongestion(tokens_[token_control].remaining,
                                       tokens_[token_control].limit);
  }

  //
  // Send a new token if we didn't send any data to
  // the remote for longer than the ping timeout.
  // The client side sends a token, the server side
  // responds with a token reply.
  //
  // VMWare virtual machines can have the system
  // timer deadly broken. Try to send a ping regard-
  // less we are the client or the server proxy to
  // force a write by the remote.
  //

  if (control -> ProxyMode == proxy_client ||
          diffIn >= (control -> PingTimeout * 4) -
              control -> LatencyTimeout)
  {
    //
    // We need to send a new ping even if we didn't
    // receive anything from the remote within the
    // ping timeout. The server side will respond
    // to our ping, so we use the ping to force the
    // remote end to send some data.
    //

    if (diffIn >= (control -> PingTimeout -
            control -> LatencyTimeout * 5) ||
                diffOut >= (control -> PingTimeout -
                    control -> LatencyTimeout * 5))
    {
      int diffPing = diffTimestamp(timeouts_.pingTs, nowTs);

      if (diffPing < 0 || diffPing >= (control -> PingTimeout -
              control -> LatencyTimeout * 5))
      {
        #if defined(TEST) || defined(INFO) || defined(PING)
        *logofs << "Proxy: Sending a new ping at " << strMsTimestamp()
                << " with " << tokens_[token_control].remaining
                << " tokens and elapsed in " << diffIn << " out "
                << diffOut << " ping " << diffPing
                << ".\n" << logofs_flush;
        #endif

        if (handleFrame(frame_ping) < 0)
        {
          return -1;
        }

        timeouts_.pingTs = nowTs;
      }
      #if defined(TEST) || defined(INFO) || defined(PING)
      else
      {
        *logofs << "Proxy: Not sending a new ping with "
                << "elapsed in " << diffIn << " out "
                << diffOut << " ping " << diffPing
                << ".\n" << logofs_flush;
      }
      #endif
    }
  }

  return 1;
}

int Proxy::handleSyncFromProxy(int channelId)
{
  #if defined(TEST) || defined(INFO)
  *logofs << "Proxy: WARNING! Received a synchronization "
          << "request from the remote proxy.\n"
          << logofs_flush;
  #endif

  if (handleControl(code_sync_reply, channelId) < 0)
  {
    return -1;
  }

  return 1;
}

int Proxy::handleResetStores()
{
  //
  // Recreate the message stores.
  //

  delete clientStore_;
  delete serverStore_;

  clientStore_ = new ClientStore(compressor_);
  serverStore_ = new ServerStore(compressor_);

  timeouts_.loadTs = nullTimestamp();

  //
  // Replace message stores in channels.
  //

  T_list &channelList = activeChannels_.getList();

  for (T_list::iterator j = channelList.begin();
           j != channelList.end(); j++)
  {
    int channelId = *j;

    if (channels_[channelId] != NULL)
    {
      if (channels_[channelId] -> setStores(clientStore_, serverStore_) < 0)
      {
        #ifdef PANIC
        *logofs << "Proxy: PANIC! Failed to replace message stores in "
                << "channel for FD#" << getFd(channelId) << ".\n"
                << logofs_flush;
        #endif

        cerr << "Error" << ": Failed to replace message stores in "
             << "channel for FD#" << getFd(channelId) << ".\n";

        return -1;
      }
      #ifdef TEST
      else
      {
        *logofs << "Proxy: Replaced message stores in channel "
                << "for FD#" << getFd(channelId) << ".\n"
                << logofs_flush;
      }
      #endif
    }
  }

  return 1;
}

int Proxy::handleResetPersistentCache()
{
  char *fullName = new char[strlen(control -> PersistentCachePath) +
                                strlen(control -> PersistentCacheName) + 2];

  strcpy(fullName, control -> PersistentCachePath);
  strcat(fullName, "/");
  strcat(fullName, control -> PersistentCacheName);

  #ifdef TEST
  *logofs << "Proxy: Going to remove persistent cache file '"
          << fullName << "'\n" << logofs_flush;
  #endif

  unlink(fullName);

  delete [] fullName;

  delete [] control -> PersistentCacheName;

  control -> PersistentCacheName = NULL;

  return 1;
}

void Proxy::handleResetFlush()
{
  #ifdef TEST
  *logofs << "Proxy: Going to reset flush counters "
          << "for proxy FD#" << fd_ << ".\n"
          << logofs_flush;
  #endif

  //
  // Reset the proxy priority flag.
  //

  priority_ = 0;

  //
  // Restore buffers to their initial
  // size.
  //

  transport_ -> partialReset();

  //
  // Update the timestamp of the last
  // write operation performed on the
  // socket.
  //

  timeouts_.writeTs = getTimestamp();
}

int Proxy::handleFinish()
{
  //
  // Reset the timestamps to give the proxy
  // another chance to show the 'no response'
  // dialog if the shutdown message doesn't
  // come in time.
  //

  timeouts_.readTs = getTimestamp();

  timeouts_.alertTs = nullTimestamp();

  finish_ = 1;

  return 1;
}

int Proxy::handleShutdown()
{
  //
  // Send shutdown message to remote proxy.
  //

  shutdown_ = 1;

  handleControl(code_shutdown_request);

  #ifdef TEST
  *logofs << "Proxy: Starting shutdown procedure "
          << "for proxy FD#" << fd_ << ".\n"
          << logofs_flush;
  #endif

  //
  // Ensure that all the data accumulated
  // in the transport buffer is flushed
  // to the network layer.
  //

  for (int i = 0; i < 100; i++)
  {
    if (canFlush() == 1)
    {
      handleFlush();
    }
    else
    {
      break;
    }

    usleep(100000);
  }

  //
  // Now wait for the network layers to
  // consume all the data.
  //

  for (int i = 0; i < 100; i++)
  {
    if (transport_ -> queued() <= 0)
    {
      break;
    }

    usleep(100000);
  }

  //
  // Give time to the remote end to read
  // the shutdown message and close the
  // connection.
  //

  transport_ -> wait(10000);

  #ifdef TEST
  *logofs << "Proxy: Ending shutdown procedure "
          << "for proxy FD#" << fd_ << ".\n"
          << logofs_flush;
  #endif

  return 1;
}

int Proxy::handleChannelConfiguration()
{
  if (activeChannels_.getSize() == 0)
  {
    #ifdef TEST
    *logofs << "Proxy: Going to initialize the static "
            << "members in channels for proxy FD#"
            << fd_ << ".\n" << logofs_flush;
    #endif

    Channel::setReferences();

    ClientChannel::setReferences();
    ServerChannel::setReferences();

    GenericChannel::setReferences();
  }

  return 1;
}

int Proxy::handleSocketConfiguration()
{
  //
  // Set linger mode on proxy to correctly
  // get shutdown notification.
  //

  SetLingerTimeout(fd_, 30);

  //
  // Set keep-alive on socket so that if remote link
  // terminates abnormally (as killed hard or because
  // of a power-off) process will get a SIGPIPE. In
  // practice this is useless as proxies already ping
  // each other every few seconds.
  //

  if (control -> OptionProxyKeepAlive == 1)
  {
    SetKeepAlive(fd_);
  }

  //
  // Set 'priority' flag at TCP layer for path
  // proxy-to-proxy. Look at IPTOS_LOWDELAY in
  // man 7 ip.
  //

  if (control -> OptionProxyLowDelay == 1)
  {
    SetLowDelay(fd_);
  }

  //
  // Update size of TCP send and receive buffers.
  //

  if (control -> OptionProxySendBuffer != -1)
  {
    SetSendBuffer(fd_, control -> OptionProxySendBuffer);
  }

  if (control -> OptionProxyReceiveBuffer != -1)
  {
    SetReceiveBuffer(fd_, control -> OptionProxyReceiveBuffer);
  }

  //
  // Update TCP_NODELAY settings. Note that on old Linux
  // kernels turning off the Nagle algorithm didn't work
  // when proxy was run through a PPP link. Trying to do
  // so caused the kernel to stop delivering data to us
  // if a serious network congestion was encountered.
  //

  if (control -> ProxyMode == proxy_client)
  {
    if (control -> OptionProxyClientNoDelay != -1)
    {
      SetNoDelay(fd_, control -> OptionProxyClientNoDelay);
    }
  }
  else
  {
    if (control -> OptionProxyServerNoDelay != -1)
    {
      SetNoDelay(fd_, control -> OptionProxyServerNoDelay);
    }
  }

  return 1;
}

int Proxy::handleLinkConfiguration()
{
  #ifdef TEST
  *logofs << "Proxy: Propagating parameters to "
          << "channels' read buffers.\n"
          << logofs_flush;
  #endif

  T_list &channelList = activeChannels_.getList();

  for (T_list::iterator j = channelList.begin();
           j != channelList.end(); j++)
  {
    int channelId = *j;

    if (channels_[channelId] != NULL)
    {
      channels_[channelId] -> handleConfiguration();
    }
  }

  #ifdef TEST
  *logofs << "Proxy: Propagating parameters to "
          << "proxy buffers.\n"
          << logofs_flush;
  #endif

  readBuffer_.setSize(control -> ProxyInitialReadSize,
                          control -> ProxyMaximumBufferSize);

  encodeBuffer_.setSize(control -> TransportProxyBufferSize,
                            control -> TransportProxyBufferThreshold,
                                control -> TransportMaximumBufferSize);

  transport_ -> setSize(control -> TransportProxyBufferSize,
                            control -> TransportProxyBufferThreshold,
                                control -> TransportMaximumBufferSize);

  #ifdef TEST
  *logofs << "Proxy: Configuring the proxy timeouts.\n"
          << logofs_flush;
  #endif

  timeouts_.split  = control -> SplitTimeout;
  timeouts_.motion = control -> MotionTimeout;

  #ifdef TEST
  *logofs << "Proxy: Configuring the proxy tokens.\n"
          << logofs_flush;
  #endif

  tokens_[token_control].size  = control -> TokenSize;
  tokens_[token_control].limit = control -> TokenLimit;

  if (tokens_[token_control].limit < 1)
  {
    tokens_[token_control].limit = 1;
  }

  #if defined(TEST) || defined(INFO) || defined(LIMIT)
  *logofs << "Proxy: TOKEN! LIMIT! Setting token ["
          << DumpToken(token_control) << "] size to "
          << tokens_[token_control].size << " and limit to "
          << tokens_[token_control].limit << ".\n"
          << logofs_flush;
  #endif

  tokens_[token_split].size  = control -> TokenSize;
  tokens_[token_split].limit = control -> TokenLimit / 2;

  if (tokens_[token_split].limit < 1)
  {
    tokens_[token_split].limit = 1;
  }

  #if defined(TEST) || defined(INFO) || defined(LIMIT)
  *logofs << "Proxy: TOKEN! LIMIT! Setting token ["
          << DumpToken(token_split) << "] size to "
          << tokens_[token_split].size << " and limit to "
          << tokens_[token_split].limit << ".\n"
          << logofs_flush;
  #endif

  tokens_[token_data].size  = control -> TokenSize;
  tokens_[token_data].limit = control -> TokenLimit / 4;

  if (tokens_[token_data].limit < 1)
  {
    tokens_[token_data].limit = 1;
  }

  #if defined(TEST) || defined(INFO) || defined(LIMIT)
  *logofs << "Proxy: TOKEN! LIMIT! Setting token ["
          << DumpToken(token_data) << "] size to "
          << tokens_[token_data].size << " and limit to "
          << tokens_[token_data].limit << ".\n"
          << logofs_flush;
  #endif

  for (int i = token_control; i <= token_data; i++)
  {
    tokens_[i].remaining = tokens_[i].limit;
  }

  #if defined(TEST) || defined(INFO) || defined(LIMIT)
  *logofs << "Proxy: LIMIT! Using client bitrate "
          << "limit " << control -> ClientBitrateLimit
          << " server bitrate limit " << control ->
             ServerBitrateLimit << " with local limit "
          << control -> LocalBitrateLimit << ".\n"
          << logofs_flush;
  #endif

  //
  // Set the other parameters based on
  // the token size.
  //

  int base = control -> TokenSize;

  control -> SplitDataThreshold   = base * 4;
  control -> SplitDataPacketLimit = base / 2;

  #if defined(TEST) || defined(INFO)
  *logofs << "Proxy: LIMIT! Setting split data threshold "
          << "to " << control -> SplitDataThreshold
          << " split packet limit to " << control ->
             SplitDataPacketLimit << " with base "
          << base << ".\n" << logofs_flush;
  #endif

  //
  // Set the number of bytes read from the
  // data channels at each loop. This will
  // basically determine the maximum band-
  // width available for the generic chan-
  // nels.
  //

  control -> GenericInitialReadSize   = base / 2;
  control -> GenericMaximumBufferSize = base / 2;

  #if defined(TEST) || defined(INFO)
  *logofs << "Proxy: LIMIT! Setting generic channel "
          << "initial read size to " << control ->
             GenericInitialReadSize << " maximum read "
          << "size to " << control -> GenericMaximumBufferSize
          << " with base " << base << ".\n"
          << logofs_flush;
  #endif

  return 1;
}

int Proxy::handleCacheConfiguration()
{
  #ifdef TEST
  *logofs << "Proxy: Configuring cache according to pack parameters.\n"
          << logofs_flush;
  #endif

  //
  // Further adjust the cache parameters. If
  // packing of the images is enabled, reduce
  // the size available for plain images.
  //

  if (control -> SessionMode == session_agent)
  {
    if (control -> PackMethod != NO_PACK)
    {
      clientStore_ -> getRequestStore(X_PutImage) ->
          cacheThreshold = PUTIMAGE_CACHE_THRESHOLD_IF_PACKED;

      clientStore_ -> getRequestStore(X_PutImage) ->
          cacheLowerThreshold = PUTIMAGE_CACHE_LOWER_THRESHOLD_IF_PACKED;
    }
  }

  //
  // If this is a shadow session increase the
  // size of the image cache.
  //

  if (control -> SessionMode == session_shadow)
  {
    if (control -> PackMethod != NO_PACK)
    {
      clientStore_ -> getRequestStore(X_NXPutPackedImage) ->
          cacheThreshold = PUTPACKEDIMAGE_CACHE_THRESHOLD_IF_PACKED_SHADOW;

      clientStore_ -> getRequestStore(X_NXPutPackedImage) ->
          cacheLowerThreshold = PUTPACKEDIMAGE_CACHE_LOWER_THRESHOLD_IF_PACKED_SHADOW;
    }
    else
    {
      clientStore_ -> getRequestStore(X_PutImage) ->
          cacheThreshold = PUTIMAGE_CACHE_THRESHOLD_IF_SHADOW;

      clientStore_ -> getRequestStore(X_PutImage) ->
          cacheLowerThreshold = PUTIMAGE_CACHE_LOWER_THRESHOLD_IF_SHADOW;
    }
  }

  return 1;
}

int Proxy::handleSaveStores()
{
  //
  // Save content of stores on disk.
  //

  char *cacheToAdopt = NULL;

  if (control -> PersistentCacheEnableSave)
  {
    #ifdef TEST
    *logofs << "Proxy: Going to save content of client store.\n"
            << logofs_flush;
    #endif

    cacheToAdopt = handleSaveAllStores(control -> PersistentCachePath);
  }
  #ifdef TEST
  else
  {
    if (control -> ProxyMode == proxy_client)
    {
      *logofs << "Proxy: Saving persistent cache to disk disabled.\n"
              << logofs_flush;
    }
    else
    {
      *logofs << "Proxy: PANIC! Protocol violation in command save.\n"
              << logofs_flush;

      cerr << "Error" << ": Protocol violation in command save.\n";

      HandleCleanup();
    }
  }
  #endif

  if (cacheToAdopt != NULL)
  {
    //
    // Do we have a cache already?
    //

    if (control -> PersistentCacheName != NULL)
    {
      //
      // Check if old and new cache are the same.
      // In this case don't remove the old cache.
      //

      if (strcasecmp(control -> PersistentCacheName, cacheToAdopt) != 0)
      {
        handleResetPersistentCache();
      }

      delete [] control -> PersistentCacheName;
    }

    #ifdef TEST
    *logofs << "Proxy: Setting current persistent cache file to '"
            << cacheToAdopt << "'\n" << logofs_flush;
    #endif

    control -> PersistentCacheName = cacheToAdopt;

    return 1;
  }
  #ifdef TEST
  else
  {
    *logofs << "Proxy: No cache file produced from message stores.\n"
            << logofs_flush;
  }
  #endif

  //
  // It can be that we didn't generate a new cache
  // because store was too small or persistent cache
  // was disabled. This is not an error.
  //

  return 0;
}

int Proxy::handleLoadStores()
{
  //
  // Restore the content of the client store
  // from disk if a valid cache was negotiated
  // at session startup.
  //

  if (control -> PersistentCacheEnableLoad == 1 &&
          control -> PersistentCachePath != NULL &&
              control -> PersistentCacheName != NULL)
  {
    #ifdef TEST
    *logofs << "Proxy: Going to load content of client store.\n"
            << logofs_flush;
    #endif

    //
    // Returns the same string passed as name of
    // the cache, or NULL if it was not possible
    // to load the cache from disk.
    //

    if (handleLoadAllStores(control -> PersistentCachePath,
                                control -> PersistentCacheName) == NULL)
    {
      //
      // The corrupted cache should have been
      // removed from disk. Get rid of the
      // reference so we don't try to delete
      // it once again.
      //

      if (control -> PersistentCacheName != NULL)
      {
        delete [] control -> PersistentCacheName;
      }

      control -> PersistentCacheName = NULL;
 
      return -1;
    }

    //
    // Set timestamp of last time cache
    // was loaded from data on disk.
    //

    timeouts_.loadTs = getTimestamp();

    return 1;
  }
  #ifdef TEST
  else
  {
    if (control -> ProxyMode == proxy_client)
    {
      *logofs << "Proxy: Loading of cache disabled or no cache file selected.\n"
              << logofs_flush;
    }
    else
    {
      *logofs << "Proxy: PANIC! Protocol violation in command load.\n"
              << logofs_flush;

      cerr << "Error" << ": Protocol violation in command load.\n";

      HandleCleanup();
    }
  }
  #endif

  return 0;
}

int Proxy::handleControl(T_proxy_code code, int data)
{
  //
  // Send the given control messages
  // to the remote proxy.
  //

  #if defined(TEST) || defined(INFO)

  if (data != -1)
  {
    if (code == code_control_token_reply ||
            code == code_split_token_reply ||
                code == code_data_token_reply)
    {
      *logofs << "Proxy: TOKEN! Sending message '" << DumpControl(code)
              << "' at " << strMsTimestamp() << " with count "
              << data << ".\n" << logofs_flush;
    }
    else
    {
      *logofs << "Proxy: Sending message '" << DumpControl(code)
              << "' at " << strMsTimestamp() << " with data ID#"
              << data << ".\n" << logofs_flush;
    }
  }
  else
  {
    *logofs << "Proxy: Sending message '" << DumpControl(code)
            << "' at " << strMsTimestamp() << ".\n"
            << logofs_flush;
  }

  #endif

  //
  // Add the control message and see if the
  // data has to be flushed immediately.
  //

  if (addControlCodes(code, data) < 0)
  {
    return -1;
  }

  switch (code)
  {
    //
    // Append the first data read from the opened
    // channel to the control code.
    // 

    case code_new_x_connection:
    case code_new_cups_connection:
    case code_new_aux_connection:
    case code_new_smb_connection:
    case code_new_media_connection:
    case code_new_http_connection:
    case code_new_font_connection:
    case code_new_slave_connection:

    //
    // Do we send the token reply immediately?
    // The control messages are put at the begin-
    // ning of the of the encode buffer, so we may
    // reply to multiple tokens before having the
    // chance of handling the actual frame data.
    // On the other hand, the sooner we reply, the
    // sooner the remote proxy is restarted.
    //

    case code_control_token_reply:
    case code_split_token_reply:
    case code_data_token_reply:
    {
      break;
    }

    //
    // Also send the congestion control codes
    // immediately.
    //
    // case code_begin_congestion:
    // case code_end_congestion:
    //

    default:
    {
      priority_ = 1;

      break;
    }
  }

  if (priority_ == 1)
  {
    if (handleFrame(frame_data) < 0)
    {
      return -1;
    }
  }

  return 1;
}

int Proxy::handleSwitch(int channelId)
{
  //
  // If data is for a different channel than last
  // selected for output, prepend to the data the
  // new channel id.
  //

  #ifdef DEBUG
  *logofs << "Proxy: Requested a switch with "
          << "current channel ID#" << outputChannel_
          << " new channel ID#" << channelId << ".\n"
          << logofs_flush;
  #endif

  if (channelId != outputChannel_)
  {
    if (needFlush() == 1)
    {
      #if defined(TEST) || defined(INFO) || defined(FLUSH)
      *logofs << "Proxy: WARNING! Flushing data for the "
              << "previous channel ID#" << outputChannel_
              << ".\n" << logofs_flush;
      #endif

      if (handleFrame(frame_data) < 0)
      {
        return -1;
      }
    }

    #if defined(TEST) || defined(INFO)
    *logofs << "Proxy: Sending message '"
            << DumpControl(code_switch_connection) << "' at "
            << strMsTimestamp() << " with FD#" << getFd(channelId)
            << " channel ID#" << channelId << ".\n"
            << logofs_flush;
    #endif

    if (addControlCodes(code_switch_connection, channelId) < 0)
    {
      return -1;
    }

    outputChannel_ = channelId;
  }

  return 1;
}

int Proxy::addTokenCodes(T_proxy_token &token)
{
  #if defined(TEST) || defined(INFO) || defined(TOKEN)
  *logofs << "Proxy: TOKEN! Sending token ["
          << DumpToken(token.type) << "] with "
          << token.bytes << " bytes accumulated size "
          << token.size << " and " << token.remaining
          << " available.\n" << logofs_flush;
  #endif

  //
  // Give a 'weight' to the token. The tokens
  // remaining can become negative if we sent
  // a packet that was exceptionally big.
  //

  int count = 0;

  if (control -> isProtoStep7() == 1)
  {
    count = token.bytes / token.size;

    if (count > 255)
    {
      count = 255;
    }
  }

  //
  // Force a count of 1, for example
  // if this is a ping.
  //

  if (count < 1)
  {
    count = 1;

    token.bytes = 0;
  }
  else
  {
    //
    // Let the next token account for the
    // remaining bytes.
    //

    token.bytes %= token.size;
  }

  #if defined(TEST) || defined(INFO) || defined(TOKEN)
  *logofs << "Proxy: Sending message '"
          << DumpControl(token.request) << "' at "
          << strMsTimestamp() << " with count " << count
          << ".\n" << logofs_flush;
  #endif

  controlCodes_[controlLength_++] = 0;
  controlCodes_[controlLength_++] = (unsigned char) token.request;
  controlCodes_[controlLength_++] = (unsigned char) count;

  statistics -> addFrameOut();

  token.remaining -= count;

  return 1;
}

int Proxy::handleToken(T_frame_type type)
{
  #if defined(TEST) || defined(INFO) || defined(TOKEN)
  *logofs << "Proxy: TOKEN! Checking tokens with "
          << "frame type [";

  *logofs << (type == frame_ping ? "frame_ping" : "frame_data");

  *logofs << "] with stream ratio " << statistics ->
             getStreamRatio() << ".\n" << logofs_flush;
  #endif

  if (type == frame_data)
  {
    if (control -> isProtoStep7() == 1)
    {
      //
      // Send a distinct token for each data type.
      // We don't want to slow down the sending of
      // the X events, X replies and split confir-
      // mation events on the X server side, so
      // take care only of the generic data token.
      //

      if (control -> ProxyMode == proxy_client)
      {
        statistics -> updateControlToken(tokens_[token_control].bytes);

        if (tokens_[token_control].bytes > tokens_[token_control].size)
        {
          if (addTokenCodes(tokens_[token_control]) < 0)
          {
            return -1;
          }

          #if defined(TEST) || defined(INFO) || defined(TOKEN)

          T_proxy_token &token = tokens_[token_control];

          *logofs << "Proxy: TOKEN! Token class ["
                  << DumpToken(token.type) << "] has now "
                  << token.bytes << " bytes accumulated and "
                  << token.remaining << " tokens remaining.\n"
                  << logofs_flush;
          #endif
        }

        statistics -> updateSplitToken(tokens_[token_split].bytes);

        if (tokens_[token_split].bytes > tokens_[token_split].size)
        {
          if (addTokenCodes(tokens_[token_split]) < 0)
          {
            return -1;
          }

          #if defined(TEST) || defined(INFO) || defined(TOKEN)

          T_proxy_token &token = tokens_[token_split];

          *logofs << "Proxy: TOKEN! Token class ["
                  << DumpToken(token.type) << "] has now "
                  << token.bytes << " bytes accumulated and "
                  << token.remaining << " tokens remaining.\n"
                  << logofs_flush;
          #endif
        }
      }

      statistics -> updateDataToken(tokens_[token_data].bytes);

      if (tokens_[token_data].bytes > tokens_[token_data].size)
      {
        if (addTokenCodes(tokens_[token_data]) < 0)
        {
          return -1;
        }

        #if defined(TEST) || defined(INFO) || defined(TOKEN)

        T_proxy_token &token = tokens_[token_data];

        *logofs << "Proxy: TOKEN! Token class ["
                << DumpToken(token.type) << "] has now "
                << token.bytes << " bytes accumulated and "
                << token.remaining << " tokens remaining.\n"
                << logofs_flush;
        #endif
      }
    }
    else
    {
      //
      // Sum everything to the control token.
      //

      if (control -> ProxyMode == proxy_client)
      {
        statistics -> updateControlToken(tokens_[token_control].bytes);
        statistics -> updateSplitToken(tokens_[token_control].bytes);
        statistics -> updateDataToken(tokens_[token_control].bytes);

        if (tokens_[token_control].bytes > tokens_[token_control].size)
        {
          if (addTokenCodes(tokens_[token_control]) < 0)
          {
            return -1;
          }

          #if defined(TEST) || defined(INFO) || defined(TOKEN)

          T_proxy_token &token = tokens_[token_control];

          *logofs << "Proxy: TOKEN! Token class ["
                  << DumpToken(token.type) << "] has now "
                  << token.bytes << " bytes accumulated and "
                  << token.remaining << " tokens remaining.\n"
                  << logofs_flush;
          #endif
        }
      }
    }
  }
  else
  {
    if (addTokenCodes(tokens_[token_control]) < 0)
    {
      return -1;
    }

    //
    // Reset all counters on a ping.
    //

    tokens_[token_control].bytes = 0;
    tokens_[token_split].bytes   = 0;
    tokens_[token_data].bytes    = 0;

    #if defined(TEST) || defined(INFO) || defined(TOKEN)

    T_proxy_token &token = tokens_[token_control];

    *logofs << "Proxy: TOKEN! Token class ["
            << DumpToken(token.type) << "] has now "
            << token.bytes << " bytes accumulated and "
            << token.remaining << " tokens remaining.\n"
            << logofs_flush;
    #endif
  }

  //
  // Check if we have entered in
  // congestion state.
  //

  if (congestion_ == 0 &&
          tokens_[token_control].remaining <= 0)
  {
    #if defined(TEST) || defined(INFO)
    *logofs << "Proxy: Entering congestion with "
            << tokens_[token_control].remaining
            << " tokens remaining.\n" << logofs_flush;
    #endif

    congestion_ = 1;
  }

  statistics -> updateCongestion(tokens_[token_control].remaining,
                                     tokens_[token_control].limit);

  return 1;
}

int Proxy::handleTokenFromProxy(T_proxy_token &token, int count)
{
  #if defined(TEST) || defined(INFO) || defined(TOKEN)
  *logofs << "Proxy: TOKEN! Received token ["
          << DumpToken(token.type) << "] request at "
          << strMsTimestamp() << " with count "
          << count << ".\n" << logofs_flush;
  #endif

  if (control -> isProtoStep7() == 0)
  {
    if (control -> ProxyMode == proxy_client ||
            token.request != code_control_token_request)
    {
      #ifdef PANIC
      *logofs << "Proxy: PANIC! Invalid token request received from remote.\n"
              << logofs_flush;
      #endif

      cerr << "Error" << ": Invalid token request received from remote.\n";

      HandleCleanup();
    }
  }

  //
  // Add our token reply.
  //

  if (handleControl(token.reply, count) < 0)
  {
    return -1;
  }

  return 1;
}

int Proxy::handleTokenReplyFromProxy(T_proxy_token &token, int count)
{
  #if defined(TEST) || defined(INFO) || defined(TOKEN)
  *logofs << "Proxy: TOKEN! Received token ["
          << DumpToken(token.type) << "] reply at "
          << strMsTimestamp() << " with count " << count
          << ".\n" << logofs_flush;
  #endif

  //
  // Increment the available tokens.
  //

  if (control -> isProtoStep7() == 0)
  {
    if (token.reply != code_control_token_reply)
    {
      #ifdef PANIC
      *logofs << "Proxy: PANIC! Invalid token reply received from remote.\n"
              << logofs_flush;
      #endif

      cerr << "Error" << ": Invalid token reply received from remote.\n";

      HandleCleanup();
    }

    count = 1;
  }

  token.remaining += count;

  if (token.remaining > token.limit)
  {
    #ifdef PANIC
    *logofs << "Proxy: PANIC! Token overflow handling messages.\n"
            << logofs_flush;
    #endif

    cerr << "Error" << ": Token overflow handling messages.\n";

    HandleCleanup();
  }

  #if defined(TEST) || defined(INFO) || defined(TOKEN)
  *logofs << "Proxy: TOKEN! Token class ["
          << DumpToken(token.type) << "] has now " << token.bytes
          << " bytes accumulated and " << token.remaining
          << " tokens remaining.\n" << logofs_flush;
  #endif

  //
  // Check if we can jump out of the
  // congestion state.
  //

  if (congestion_ == 1 &&
          tokens_[token_control].remaining > 0)
  {
    #if defined(TEST) || defined(INFO)
    *logofs << "Proxy: Exiting congestion with "
            << tokens_[token_control].remaining
            << " tokens remaining.\n" << logofs_flush;
    #endif

    congestion_ = 0;
  }

  statistics -> updateCongestion(tokens_[token_control].remaining,
                                     tokens_[token_control].limit);

  return 1;
}

void Proxy::handleFailOnSave(const char *fullName, const char *failContext) const
{
  #ifdef WARNING
  *logofs << "Proxy: WARNING! Error saving stores to cache file "
          << "in context [" << failContext << "].\n" << logofs_flush;
  #endif

  cerr << "Warning" << ": Error saving stores to cache file "
       << "in context [" << failContext << "].\n";

  #ifdef WARNING
  *logofs << "Proxy: WARNING! Removing invalid cache '"
          << fullName << "'.\n" << logofs_flush;
  #endif

  cerr << "Warning" << ": Removing invalid cache '"
       << fullName << "'.\n";

  unlink(fullName);
}

void Proxy::handleFailOnLoad(const char *fullName, const char *failContext) const
{
  #ifdef WARNING
  *logofs << "Proxy: WARNING! Error loading stores from cache file "
          << "in context [" << failContext << "].\n" << logofs_flush;
  #endif

  cerr << "Warning" << ": Error loading stores from cache file "
       << "in context [" << failContext << "].\n";

  #ifdef WARNING
  *logofs << "Proxy: WARNING! Removing invalid cache '"
          << fullName << "'.\n" << logofs_flush;
  #endif

  cerr << "Warning" << ": Removing invalid cache '"
       << fullName << "'.\n";

  unlink(fullName);
}

int Proxy::handleSaveVersion(unsigned char *buffer, int &major,
                                 int &minor, int &patch) const
{
  if (control -> isProtoStep8() == 1)
  {
    major = 3;
    minor = 0;
    patch = 0;
  }
  else if (control -> isProtoStep7() == 1)
  {
    major = 2;
    minor = 0;
    patch = 0;
  }
  else
  {
    major = 1;
    minor = 4;
    patch = 0;
  }

  *(buffer + 0) = major;
  *(buffer + 1) = minor;

  PutUINT(patch, buffer + 2, storeBigEndian());

  return 1;
}

int Proxy::handleLoadVersion(const unsigned char *buffer, int &major,
                                 int &minor, int &patch) const
{
  major = *(buffer + 0);
  minor = *(buffer + 1);

  patch = GetUINT(buffer + 2, storeBigEndian());

  //
  // Force the proxy to discard the
  // incompatible caches.
  // 

  if (control -> isProtoStep8() == 1)
  {
    if (major < 3)
    {
      return -1;
    }
  }
  else if (control -> isProtoStep7() == 1)
  {
    if (major < 2)
    {
      return -1;
    }
  }
  else
  {
    if (major != 1 && minor != 4)
    {
      return -1;
    }
  }

  return 1;
}

char *Proxy::handleSaveAllStores(const char *savePath) const
{
  int cumulativeSize = MessageStore::getCumulativeTotalStorageSize();

  if (cumulativeSize < control -> PersistentCacheThreshold)
  {
    #ifdef TEST
    *logofs << "Proxy: Cache not saved as size is "
            << cumulativeSize << " with threshold set to "
            << control -> PersistentCacheThreshold
            << ".\n" << logofs_flush;
    #endif

    return NULL;
  }
  else if (savePath == NULL)
  {
    #ifdef PANIC
    *logofs << "Proxy: PANIC! No name provided for save path.\n"
            << logofs_flush;
    #endif

    cerr << "Error" << ": No name provided for save path.\n";

    return NULL;
  }

  #ifdef TEST
  *logofs << "Proxy: Going to save content of message stores.\n"
          << logofs_flush;
  #endif

  //
  // Our parent process is likely going to terminate.
  // Until we finish saving cache we must ignore its
  // SIGIPE.
  //

  DisableSignals();

  ofstream *cachefs = NULL;

  md5_state_t *md5StateStream = NULL;
  md5_byte_t  *md5DigestStream = NULL;

  md5_state_t *md5StateClient = NULL;
  md5_byte_t  *md5DigestClient = NULL;

  char *tempName = NULL;

  char md5String[MD5_LENGTH * 2 + 2];

  char fullName[strlen(savePath) + MD5_LENGTH * 2 + 4];

  if (control -> ProxyMode == proxy_client)
  {
    tempName = tempnam(savePath, "Z-C-");
  }
  else
  {
    tempName = tempnam(savePath, "Z-S-");
  }

  //
  // Change the mask to make the file only
  // readable by the user, then restore the
  // old mask.
  //

  mode_t fileMode = umask(0077);

  cachefs = new ofstream(tempName, ios::out | ios::binary);

  umask(fileMode);

  if (tempName == NULL || cachefs == NULL)
  {
    #ifdef PANIC
    *logofs << "Proxy: PANIC! Can't create temporary file in '"
            << savePath << "'.\n" << logofs_flush;
    #endif

    cerr << "Error" << ": Can't create temporary file in '"
         << savePath << "'.\n";

    if (tempName != NULL)
    {
      free(tempName);
    }

    if (cachefs != NULL)
    {
      delete cachefs;
    }

    EnableSignals();

    return NULL;
  }

  md5StateStream  = new md5_state_t();
  md5DigestStream = new md5_byte_t[MD5_LENGTH];

  md5_init(md5StateStream);

  //
  // First write the proxy version.
  //

  unsigned char version[4];

  int major;
  int minor;
  int patch;

  handleSaveVersion(version, major, minor, patch);

  #ifdef TEST
  *logofs << "Proxy: Saving cache using version '"
          << major << "." << minor << "." << patch
          << "'.\n" << logofs_flush;
  #endif

  if (PutData(cachefs, version, 4) < 0)
  {
    handleFailOnSave(tempName, "A");

    delete cachefs;

    delete md5StateStream;
    delete [] md5DigestStream;

    free(tempName);

    EnableSignals();

    return NULL;
  }

  //
  // Make space for the calculated MD5 so we
  // can later rewind the file and write it
  // at this position.
  //

  if (PutData(cachefs, md5DigestStream, MD5_LENGTH) < 0)
  {
    handleFailOnSave(tempName, "B");

    delete cachefs;

    delete md5StateStream;
    delete [] md5DigestStream;

    free(tempName);

    EnableSignals();

    return NULL;
  }

  md5StateClient  = new md5_state_t();
  md5DigestClient = new md5_byte_t[MD5_LENGTH];

  md5_init(md5StateClient);

  #ifdef DUMP

  ofstream *cacheDump = NULL;

  ofstream *tempfs = (ofstream*) logofs;

  char cacheDumpName[DEFAULT_STRING_LENGTH];

  if (control -> ProxyMode == proxy_client)
  {
    snprintf(cacheDumpName, DEFAULT_STRING_LENGTH - 1,
                 "%s/client-cache-dump", control -> TempPath);
  }
  else
  {
    snprintf(cacheDumpName, DEFAULT_STRING_LENGTH - 1,
                 "%s/server-cache-dump", control -> TempPath);
  }

  *(cacheDumpName + DEFAULT_STRING_LENGTH - 1) = '\0';

  mode_t fileMode = umask(0077);

  cacheDump = new ofstream(cacheDumpName, ios::out);

  umask(fileMode);

  logofs = cacheDump;

  #endif

  //
  // Use the virtual method of the concrete proxy class.
  //

  int allSaved = handleSaveAllStores(cachefs, md5StateStream, md5StateClient);

  #ifdef DUMP

  logofs = tempfs;

  delete cacheDump;

  #endif

  if (allSaved == 0)
  {
    handleFailOnSave(tempName, "C");

    delete cachefs;

    delete md5StateStream;
    delete [] md5DigestStream;

    delete md5StateClient;
    delete [] md5DigestClient;

    free(tempName);

    EnableSignals();

    return NULL;
  }

  md5_finish(md5StateClient, md5DigestClient);

  for (unsigned int i = 0; i < MD5_LENGTH; i++)
  {
    sprintf(md5String + (i * 2), "%02X", md5DigestClient[i]);
  }

  strcpy(fullName, (control -> ProxyMode == proxy_client) ? "C-" : "S-");

  strcat(fullName, md5String);

  md5_append(md5StateStream, (const md5_byte_t *) fullName, strlen(fullName));
  md5_finish(md5StateStream, md5DigestStream);

  //
  // Go to the beginning of file plus
  // the integer where we wrote our
  // proxy version.
  //

  cachefs -> seekp(4);

  if (PutData(cachefs, md5DigestStream, MD5_LENGTH) < 0)
  {
    handleFailOnSave(tempName, "D");

    delete cachefs;

    delete md5StateStream;
    delete [] md5DigestStream;

    delete md5StateClient;
    delete [] md5DigestClient;

    free(tempName);

    EnableSignals();

    return NULL;
  }

  delete cachefs;

  //
  // Copy the resulting cache name without
  // the path so that we can return it to
  // the caller.
  //

  char *cacheName = new char[MD5_LENGTH * 2 + 4];

  strcpy(cacheName, fullName);

  //
  // Add the path to the full name and move
  // the cache into the path.
  //

  strcpy(fullName, savePath);
  strcat(fullName, (control -> ProxyMode == proxy_client) ? "/C-" : "/S-");
  strcat(fullName, md5String);

  #ifdef TEST
  *logofs << "Proxy: Renaming cache file from '"
          << tempName << "' to '" << fullName
          << "'.\n" << logofs_flush;
  #endif

  rename(tempName, fullName);

  delete md5StateStream;
  delete [] md5DigestStream;

  delete md5StateClient;
  delete [] md5DigestClient;

  free(tempName);

  //
  // Restore the original handlers.
  //

  EnableSignals();

  #ifdef TEST
  *logofs << "Proxy: Successfully saved cache file '"
          << cacheName << "'.\n" << logofs_flush;
  #endif

  //
  // This must be enabled only for test
  // because it requires that client and
  // server reside on the same machine.
  //

  if (control -> PersistentCacheCheckOnShutdown == 1 &&
          control -> ProxyMode == proxy_server)
  {
    #if defined(TEST) || defined(INFO)
    *logofs << "Proxy: MATCH! Checking if the file '"
            << fullName << "' matches a client cache.\n"
            << logofs_flush;
    #endif

    strcpy(fullName, savePath);
    strcat(fullName, "/C-");
    strcat(fullName, md5String);

    struct stat fileStat;

    if (stat(fullName, &fileStat) != 0)
    {
      #ifdef PANIC
      *logofs << "Proxy: PANIC! Can't find a client cache "
              << "with name '" << fullName << "'.\n"
              << logofs_flush;
      #endif

      cerr << "Error" << ": Can't find a client cache "
           << "with name '" << fullName << "'.\n";

      HandleShutdown();
    }

    #if defined(TEST) || defined(INFO)
    *logofs << "Proxy: MATCH! Client cache '" << fullName
            << "' matches the local cache.\n"
            << logofs_flush;
    #endif
  }

  return cacheName;
}

const char *Proxy::handleLoadAllStores(const char *loadPath, const char *loadName) const
{
  #ifdef TEST
  *logofs << "Proxy: Going to load content of message stores.\n"
          << logofs_flush;
  #endif

  //
  // Until we finish loading cache we
  // must at least ignore any SIGIPE.
  //

  DisableSignals();

  if (loadPath == NULL || loadName == NULL)
  {
    #ifdef PANIC
    *logofs << "Proxy: PANIC! No path or no file name provided for cache to restore.\n"
            << logofs_flush;
    #endif

    cerr << "Error" << ": No path or no file name provided for cache to restore.\n";

    EnableSignals();

    return NULL;
  }
  else if (strlen(loadName) != MD5_LENGTH * 2 + 2)
  {
    #ifdef PANIC
    *logofs << "Proxy: PANIC! Bad file name provided for cache to restore.\n"
            << logofs_flush;
    #endif

    cerr << "Error" << ": Bad file name provided for cache to restore.\n";

    EnableSignals();

    return NULL;
  }

  istream *cachefs = NULL;
  char md5String[(MD5_LENGTH * 2) + 2];
  md5_byte_t md5FromFile[MD5_LENGTH];

  char *cacheName = new char[strlen(loadPath) + strlen(loadName) + 3];

  strcpy(cacheName, loadPath);
  strcat(cacheName, "/");
  strcat(cacheName, loadName);

  #ifdef TEST
  *logofs << "Proxy: Name of cache file is '"
          << cacheName << "'.\n" << logofs_flush;
  #endif

  cachefs = new ifstream(cacheName, ios::in | ios::binary);

  unsigned char version[4];

  if (cachefs == NULL || GetData(cachefs, version, 4) < 0)
  {
    #ifdef PANIC
    *logofs << "Proxy: PANIC! Can't read cache file '"
            << cacheName << "'.\n" << logofs_flush;;
    #endif

    handleFailOnLoad(cacheName, "A");

    delete cachefs;

    delete [] cacheName;

    EnableSignals();

    return NULL;
  }

  int major;
  int minor;
  int patch;

  if (handleLoadVersion(version, major, minor, patch) < 0)
  {
    #ifdef PANIC
    *logofs << "Proxy: WARNING! Incompatible version '"
            << major << "." << minor << "." << patch
            << "' in cache file '" << cacheName
            << "'.\n" << logofs_flush;
    #endif

    cerr << "Warning" << ": Incompatible version '"
         << major << "." << minor << "." << patch
         << "' in cache file '" << cacheName
         << "'.\n" << logofs_flush;

    if (control -> ProxyMode == proxy_server)
    {
      handleFailOnLoad(cacheName, "B");
    }
    else
    {
      //
      // Simply remove the cache file.
      //

      unlink(cacheName);
    }

    delete cachefs;

    delete [] cacheName;

    EnableSignals();

    return NULL;
  }

  #ifdef TEST
  *logofs << "Proxy: Reading from cache file version '"
          << major << "." << minor << "." << patch
          << "'.\n" << logofs_flush;
  #endif

  if (GetData(cachefs, md5FromFile, MD5_LENGTH) < 0)
  {
    #ifdef PANIC
    *logofs << "Proxy: PANIC! No checksum in cache file '"
            << loadName << "'.\n" << logofs_flush;
    #endif

    handleFailOnLoad(cacheName, "C");

    delete cachefs;

    delete [] cacheName;

    EnableSignals();

    return NULL;
  }

  md5_state_t *md5StateStream = NULL;
  md5_byte_t  *md5DigestStream = NULL;

  md5StateStream  = new md5_state_t();
  md5DigestStream = new md5_byte_t[MD5_LENGTH];

  md5_init(md5StateStream);

  //
  // Use the virtual method of the proxy class.
  //

  if (handleLoadAllStores(cachefs, md5StateStream) < 0)
  {
    handleFailOnLoad(cacheName, "D");

    delete cachefs;

    delete md5StateStream;
    delete [] md5DigestStream;

    delete [] cacheName;

    EnableSignals();

    return NULL;
  }

  md5_append(md5StateStream, (const md5_byte_t *) loadName, strlen(loadName));
  md5_finish(md5StateStream, md5DigestStream);

  for (int i = 0; i < MD5_LENGTH; i++)
  {
    if (md5DigestStream[i] != md5FromFile[i])
    {
      #ifdef PANIC

      *logofs << "Proxy: PANIC! Bad checksum for cache file '"
              << cacheName << "'.\n" << logofs_flush;

      for (unsigned int i = 0; i < MD5_LENGTH; i++)
      {
        sprintf(md5String + (i * 2), "%02X", md5FromFile[i]);
      }

      *logofs << "Proxy: PANIC! Saved checksum is '"
              << md5String << "'.\n" << logofs_flush;

      for (unsigned int i = 0; i < MD5_LENGTH; i++)
      {
        sprintf(md5String + (i * 2),"%02X", md5DigestStream[i]);
      }

      *logofs << "Proxy: PANIC! Calculated checksum is '"
              << md5String << "'.\n" << logofs_flush;

      #endif

      handleFailOnLoad(cacheName, "E");

      delete cachefs;

      delete md5StateStream;
      delete [] md5DigestStream;

      delete [] cacheName;

      EnableSignals();

      return NULL;
    }
  }

  delete cachefs;

  delete md5StateStream;
  delete [] md5DigestStream;

  delete [] cacheName;

  //
  // Restore the original handlers.
  //

  EnableSignals();

  #ifdef TEST
  *logofs << "Proxy: Successfully loaded cache file '"
          << loadName << "'.\n" << logofs_flush;
  #endif

  //
  // Return the string provided by caller.
  //

  return loadName;
}

int Proxy::allocateChannelMap(int fd)
{
  //
  // We assume that the fd is lower than
  // the maximum allowed number. This is
  // checked at the time the connection
  // is accepted.
  //

  if (fd < 0 || fd >= CONNECTIONS_LIMIT)
  {
    #ifdef PANIC
    *logofs << "Proxy: PANIC! Internal error allocating "
            << "new channel with FD#" << fd_ << ".\n"
            << logofs_flush;
    #endif

    cerr << "Error" << ": Internal error allocating "
         << "new channel with FD#" << fd_ << ".\n";

    HandleCleanup();
  }

  for (int channelId = 0;
           channelId < CONNECTIONS_LIMIT;
               channelId++)
  {
    if (checkLocalChannelMap(channelId) == 1 &&
            fdMap_[channelId] == -1)
    {
      fdMap_[channelId] = fd;
      channelMap_[fd] = channelId;

      #ifdef TEST
      *logofs << "Proxy: Allocated new channel ID#"
              << channelId << " with FD#" << fd << ".\n"
              << logofs_flush;
      #endif

      return channelId;
    }
  }

  //
  // No available channel is remaining.
  //

  #ifdef TEST
  *logofs << "Proxy: WARNING! Can't allocate a new channel "
          << "for FD#" << fd_ << ".\n" << logofs_flush;
  #endif

  return -1;
}

int Proxy::checkChannelMap(int channelId)
{
  //
  // To be acceptable, the channel id must
  // be an id that is not possible to use
  // to allocate channels at this side.
  //

  if (checkLocalChannelMap(channelId) == 1)
  {
    #ifdef PANIC
    *logofs << "Proxy: PANIC! Can't open a new channel "
            << "with invalid ID#" << channelId << ".\n"
            << logofs_flush;
    #endif

    cerr << "Error" << ": Can't open a new channel "
         << "with invalid ID#" << channelId << ".\n";

    return -1;
  }
  else if (channels_[channelId] != NULL)
  {
    #ifdef PANIC
    *logofs << "Proxy: PANIC! Can't open a new channel "
            << "over an existing ID#" << channelId
            << " with FD#" << getFd(channelId)
            << ".\n" << logofs_flush;
    #endif

    cerr << "Error" << ": Can't open a new channel "
         << "over an existing ID#" << channelId
         << " with FD#" << getFd(channelId)
         << ".\n";

    return -1;
  }

  return 1;
}

int Proxy::assignChannelMap(int channelId, int fd)
{
  //
  // We assume that the fd is lower than
  // the maximum allowed number. This is
  // checked at the time the connection
  // is accepted.
  //

  if (channelId < 0 || channelId >= CONNECTIONS_LIMIT ||
          fd < 0 || fd >= CONNECTIONS_LIMIT)
  {
    #ifdef PANIC
    *logofs << "Proxy: PANIC! Internal error assigning "
            << "new channel with FD#" << fd_ << ".\n"
            << logofs_flush;
    #endif

    cerr << "Error" << ": Internal error assigning "
         << "new channel with FD#" << fd_ << ".\n";

    HandleCleanup();
  }

  fdMap_[channelId] = fd;
  channelMap_[fd] = channelId;

  return 1;
}

void Proxy::cleanupChannelMap(int channelId)
{
  int fd = fdMap_[channelId];

  if (fd != -1)
  {
    fdMap_[channelId] = -1;
    channelMap_[fd] = -1;
  }
}

int Proxy::addControlCodes(T_proxy_code code, int data)
{
  //
  // Flush the encode buffer plus all the outstanding
  // control codes if there is not enough space for
  // the new control message. We need to ensure that
  // there are further bytes available, in the case
  // we will need to add more token control messages.
  //

  if (controlLength_ + 3 > CONTROL_CODES_THRESHOLD)
  {
    #ifdef WARNING
    *logofs << "Proxy: WARNING! Flushing control messages "
            << "while sending code '" << DumpControl(code)
            << "'.\n" << logofs_flush;
    #endif

    if (handleFlush() < 0)
    {
      return -1;
    }
  }

  controlCodes_[controlLength_++] = 0;
  controlCodes_[controlLength_++] = (unsigned char) code;
  controlCodes_[controlLength_++] = (unsigned char) (data == -1 ? 0 : data);

  //
  // Account for the control frame.
  //

  statistics -> addFrameOut();

  return 1;
}

void Proxy::setSplitTimeout(int channelId)
{
  int needed = channels_[channelId] -> needSplit();

  if (needed != isTimestamp(timeouts_.splitTs))
  {
    if (needed == 1)
    {
      #if defined(TEST) || defined(INFO) || defined(SPLIT)
      *logofs << "Proxy: SPLIT! Allocating split timestamp at "
              << strMsTimestamp() << ".\n" << logofs_flush;
      #endif

      timeouts_.splitTs = getTimestamp();
    }
    else
    {
      T_list &channelList = activeChannels_.getList();

      for (T_list::iterator j = channelList.begin();
               j != channelList.end(); j++)
      {
        int channelId = *j;

        if (channels_[channelId] != NULL &&
                channels_[channelId] -> needSplit() == 1)
        {
          #ifdef TEST
          *logofs << "Proxy: SPLIT! Channel for FD#"
                  << getFd(channelId) << " still needs splits.\n"
                  << logofs_flush;
          #endif

          return;
        }
      }

      #if defined(TEST) || defined(INFO) || defined(SPLIT)
      *logofs << "Proxy: SPLIT! Resetting split timestamp at "
              << strMsTimestamp() << ".\n" << logofs_flush;
      #endif

      timeouts_.splitTs = nullTimestamp();
    }
  }
}

void Proxy::setMotionTimeout(int channelId)
{
  int needed = channels_[channelId] -> needMotion();

  if (needed != isTimestamp(timeouts_.motionTs))
  {
    if (channels_[channelId] -> needMotion() == 1)
    {
      #if defined(TEST) || defined(INFO) || defined(SPLIT)
      *logofs << "Proxy: Allocating motion timestamp at "
              << strMsTimestamp() << ".\n" << logofs_flush;
      #endif

      timeouts_.motionTs = getTimestamp();
    }
    else
    {
      T_list &channelList = activeChannels_.getList();

      for (T_list::iterator j = channelList.begin();
               j != channelList.end(); j++)
      {
        int channelId = *j;

        if (channels_[channelId] != NULL &&
                channels_[channelId] -> needMotion() == 1)
        {
          #ifdef TEST
          *logofs << "Proxy: SPLIT! Channel for FD#"
                  << getFd(channelId) << " still needs motions.\n"
                  << logofs_flush;
          #endif

          return;
        }
      }

      #if defined(TEST) || defined(INFO) || defined(SPLIT)
      *logofs << "Proxy: Resetting motion timestamp at "
              << strMsTimestamp() << ".\n" << logofs_flush;
      #endif

      timeouts_.motionTs = nullTimestamp();
    }
  }
}

void Proxy::increaseChannels(int channelId)
{
  #ifdef TEST
  *logofs << "Proxy: Adding channel " << channelId
          << " to the list of active channels.\n"
          << logofs_flush;
  #endif

  activeChannels_.add(channelId);

  #ifdef TEST
  *logofs << "Proxy: There are " << activeChannels_.getSize()
          << " allocated channels for proxy FD#" << fd_
          << ".\n" << logofs_flush;
  #endif
}

void Proxy::decreaseChannels(int channelId)
{
  #ifdef TEST
  *logofs << "Proxy: Removing channel " << channelId
          << " from the list of active channels.\n"
          << logofs_flush;
  #endif

  activeChannels_.remove(channelId);

  #ifdef TEST
  *logofs << "Proxy: There are " << activeChannels_.getSize()
          << " allocated channels for proxy FD#" << fd_
          << ".\n" << logofs_flush;
  #endif
}

int Proxy::allocateTransport(int channelFd, int channelId)
{
  if (transports_[channelId] == NULL)
  {
    transports_[channelId] = new Transport(channelFd);

    if (transports_[channelId] == NULL)
    {
      #ifdef PANIC
      *logofs << "Proxy: PANIC! Can't allocate transport for "
              << "channel id " << channelId << ".\n"
              << logofs_flush;
      #endif

      cerr << "Error" << ": Can't allocate transport for "
           << "channel id " << channelId << ".\n";

      return -1;
    }
  }
  else if (transports_[channelId] ->
               getType() != transport_agent)
  {
    #ifdef PANIC
    *logofs << "Proxy: PANIC! Transport for channel id "
            << channelId << " should be null.\n"
            << logofs_flush;
    #endif

    cerr << "Error" << ": Transport for channel id "
         << channelId << " should be null.\n";

    return -1;
  }

  return 1;
}

int Proxy::deallocateTransport(int channelId)
{
  //
  // Transport for the agent connection
  // is passed from the outside when
  // creating the channel.
  //

  if (transports_[channelId] ->
          getType() != transport_agent)
  {
    delete transports_[channelId];
  }

  transports_[channelId] = NULL;

  return 1;
}

int Proxy::handleNewGenericConnection(int clientFd, T_channel_type type, const char *label)
{
  int channelId = allocateChannelMap(clientFd);

  if (channelId == -1)
  {
    #ifdef PANIC
    *logofs << "Proxy: PANIC! Maximum mumber of available "
            << "channels exceeded.\n" << logofs_flush;
    #endif

    cerr << "Error" << ": Maximum mumber of available "
         << "channels exceeded.\n";

    return -1;
  }

  #ifdef TEST
  *logofs << "Proxy: Channel for " << label << " descriptor "
          << "FD#" << clientFd << " mapped to ID#"
          << channelId << ".\n"
          << logofs_flush;
  #endif

  //
  // Turn queuing off for path server-to-proxy.
  //

  SetNoDelay(clientFd, 1);

  if (allocateTransport(clientFd, channelId) < 0)
  {
    return -1;
  }

  switch (type)
  {
    case channel_cups:
    {
      channels_[channelId] = new CupsChannel(transports_[channelId], compressor_);

      break;
    }
    case channel_smb:
    {
      channels_[channelId] = new SmbChannel(transports_[channelId], compressor_);

      break;
    }
    case channel_media:
    {
      channels_[channelId] = new MediaChannel(transports_[channelId], compressor_);

      break;
    }
    case channel_http:
    {
      channels_[channelId] = new HttpChannel(transports_[channelId], compressor_);

      break;
    }
    case channel_font:
    {
      channels_[channelId] = new FontChannel(transports_[channelId], compressor_);

      break;
    }
    default:
    {
      channels_[channelId] = new SlaveChannel(transports_[channelId], compressor_);

      break;
    }
  }

  if (channels_[channelId] == NULL)
  {
    deallocateTransport(channelId);

    return -1;
  }

  #ifdef TEST
  *logofs << "Proxy: Accepted new connection to "
          << label << " server.\n" << logofs_flush;
  #endif

  cerr << "Info" << ": Accepted new connection to "
       << label << " server.\n";

  increaseChannels(channelId);

  switch (type)
  {
    case channel_cups:
    {
      if (handleControl(code_new_cups_connection, channelId) < 0)
      {
        return -1;
      }

      break;
    }
    case channel_smb:
    {
      if (handleControl(code_new_smb_connection, channelId) < 0)
      {
        return -1;
      }

      break;
    }
    case channel_media:
    {
      if (handleControl(code_new_media_connection, channelId) < 0)
      {
        return -1;
      }

      break;
    }
    case channel_http:
    {
      if (handleControl(code_new_http_connection, channelId) < 0)
      {
        return -1;
      }

      break;
    }
    case channel_font:
    {
      if (handleControl(code_new_font_connection, channelId) < 0)
      {
        return -1;
      }

      break;
    }
    default:
    {
      if (handleControl(code_new_slave_connection, channelId) < 0)
      {
        return -1;
      }

      break;
    }
  }

  channels_[channelId] -> handleConfiguration();

  return 1;
}

int Proxy::handleNewSlaveConnection(int clientFd)
{
  if (control -> isProtoStep7() == 1)
  {
    return handleNewGenericConnection(clientFd, channel_slave, "slave");
  }
  else
  {
    #ifdef TEST
    *logofs << "Proxy: WARNING! Not sending unsupported "
            << "'code_new_slave_connection' message.\n"
            << logofs_flush;
    #endif

    return -1;
  }
}

int Proxy::handleNewGenericConnectionFromProxy(int channelId, T_channel_type type,
                                                   const char *hostname, int port, const char *label)
{
  if (port <= 0)
  {
    //
    // This happens when user has disabled
    // forwarding of the specific service.
    //

    #ifdef WARNING
    *logofs << "Proxy: WARNING! Refusing attempted connection "
            << "to " << label << " server.\n" << logofs_flush;
    #endif

    cerr << "Warning" << ": Refusing attempted connection "
         << "to " << label << " server.\n";

    return -1;
  }

  const char *serverHost = hostname;
  int serverAddrFamily = AF_INET;
  sockaddr *serverAddr = NULL;
  unsigned int serverAddrLength = 0;

  #ifdef TEST
  *logofs << "Proxy: Connecting to " << label
          << " server '" << serverHost << "' on TCP port '"
          << port << "'.\n" << logofs_flush;
  #endif

  int serverIPAddr = GetHostAddress(serverHost);

  if (serverIPAddr == 0)
  {
    #ifdef PANIC
    *logofs << "Proxy: PANIC! Unknown " << label
            << " server host '" << serverHost << "'.\n"
            << logofs_flush;
    #endif

    cerr << "Error" << ": Unknown " << label
         << " server host '" << serverHost
         << "'.\n";

    return -1;
  }

  sockaddr_in *serverAddrTCP = new sockaddr_in;

  serverAddrTCP -> sin_family = AF_INET;
  serverAddrTCP -> sin_port = htons(port);
  serverAddrTCP -> sin_addr.s_addr = serverIPAddr;

  serverAddr = (sockaddr *) serverAddrTCP;
  serverAddrLength = sizeof(sockaddr_in);

  //
  // Connect to the requested server.
  //

  int serverFd = socket(serverAddrFamily, SOCK_STREAM, PF_UNSPEC);

  if (serverFd < 0)
  {
    #ifdef PANIC
    *logofs << "Proxy: PANIC! Call to socket failed. "
            << "Error is " << EGET() << " '" << ESTR()
            << "'.\n" << logofs_flush;
    #endif

    cerr << "Error" << ": Call to socket failed. "
         << "Error is " << EGET() << " '" << ESTR()
         << "'.\n";

    delete serverAddrTCP;

    return -1;
  }
  else if (connect(serverFd, serverAddr, serverAddrLength) < 0)
  {
    #ifdef WARNING
    *logofs << "Proxy: WARNING! Connection to " << label
            << " server '" << serverHost << ":" << port
            << "' failed with error '" << ESTR() << "'.\n"
            << logofs_flush;
    #endif

    cerr << "Warning" << ": Connection to " << label
         << " server '" << serverHost << ":" << port
         << "' failed with error '" << ESTR() << "'.\n";

    close(serverFd);

    delete serverAddrTCP;

    return -1;
  }

  delete serverAddrTCP;

  if (handlePostConnectionFromProxy(channelId, serverFd, type, label) < 0)
  {
    return -1;
  }

  #ifdef TEST
  *logofs << "Proxy: Forwarded new connection to "
          << label << " server on port '" << port
          << "'.\n" << logofs_flush;
  #endif

  cerr << "Info" << ": Forwarded new connection to "
       << label << " server on port '" << port
       << "'.\n";

  return 1;
}

int Proxy::handleNewGenericConnectionFromProxy(int channelId, T_channel_type type,
                                                   const char *hostname, const char *path, const char *label)
{
  if (path == NULL || *path == '\0' )
  {
    //
    // This happens when user has disabled
    // forwarding of the specific service.
    //

    #ifdef WARNING
    *logofs << "Proxy: WARNING! Refusing attempted connection "
            << "to " << label << " server.\n" << logofs_flush;
    #endif

    cerr << "Warning" << ": Refusing attempted connection "
         << "to " << label << " server.\n";

    return -1;
  }

  sockaddr_un serverAddrUnix;

  unsigned int serverAddrLength = sizeof(sockaddr_un);

  int serverAddrFamily = AF_UNIX;

  serverAddrUnix.sun_family = AF_UNIX;

  const int serverAddrNameLength = 108;

  strncpy(serverAddrUnix.sun_path, path, serverAddrNameLength);

  *(serverAddrUnix.sun_path + serverAddrNameLength - 1) = '\0';

  #ifdef TEST
  *logofs << "Proxy: Connecting to " << label << " server "
          << "on Unix port '" << path << "'.\n" << logofs_flush;
  #endif

  //
  // Connect to the requested server.
  //

  int serverFd = socket(serverAddrFamily, SOCK_STREAM, PF_UNSPEC);

  if (serverFd < 0)
  {
    #ifdef PANIC
    *logofs << "Proxy: PANIC! Call to socket failed. "
            << "Error is " << EGET() << " '" << ESTR()
            << "'.\n" << logofs_flush;
    #endif

    cerr << "Error" << ": Call to socket failed. "
         << "Error is " << EGET() << " '" << ESTR()
         << "'.\n";

    return -1;
  }
  else if (connect(serverFd, (sockaddr *) &serverAddrUnix, serverAddrLength) < 0)
  {
    #ifdef WARNING
    *logofs << "Proxy: WARNING! Connection to " << label
            << " server on Unix port '" << path << "' failed "
            << "with error " << EGET() << ", '" << ESTR() << "'.\n"
            << logofs_flush;
    #endif

    cerr << "Warning" << ": Connection to " << label
         << " server on Unix port '" << path << "' failed "
        << "with error " << EGET() << ", '" << ESTR() << "'.\n";

    close(serverFd);

    return -1;
  }

  if (handlePostConnectionFromProxy(channelId, serverFd, type, label) < 0)
  {
    return -1;
  }

  #ifdef TEST
  *logofs << "Proxy: Forwarded new connection to "
          << label << " server on Unix port '" << path
          << "'.\n" << logofs_flush;
  #endif

  cerr << "Info" << ": Forwarded new connection to "
       << label << " server on Unix port '" << path
       << "'.\n";

  return 1;
}

int Proxy::handleNewSlaveConnectionFromProxy(int channelId)
{
  //
  // Implementation is incomplete. Opening a
  // slave channel should let the proxy fork
  // a new client and pass to it the channel
  // descriptors. For now we make the channel
  // fail immediately.
  //
  // #include <fcntl.h>
  // #include <sys/types.h>
  // #include <sys/stat.h>
  //
  // char *slaveServer = "/dev/null";
  //
  // #ifdef TEST
  // *logofs << "Proxy: Opening file '" << slaveServer
  //         << "'.\n" << logofs_flush;
  // #endif
  //
  // int serverFd = open(slaveServer, O_RDWR);
  //
  // if (handlePostConnectionFromProxy(channelId, serverFd, channel_slave, "slave") < 0)
  // {
  //   return -1;
  // }
  //

  #ifdef WARNING
  *logofs << "Proxy: Refusing new slave connection for "
          << "channel ID#" << channelId << "\n"
          << logofs_flush;
  #endif

  cerr << "Warning" << ": Refusing new slave connection for "
          << "channel ID#" << channelId << "\n";

  return -1;
}

int Proxy::handlePostConnectionFromProxy(int channelId, int serverFd,
                                             T_channel_type type, const char *label)
{
  //
  // Turn queuing off for path proxy-to-server.
  //

  SetNoDelay(serverFd, 1);

  assignChannelMap(channelId, serverFd);

  #ifdef TEST
  *logofs << "Proxy: Descriptor FD#" << serverFd 
          << " mapped to channel ID#" << channelId << ".\n"
          << logofs_flush;
  #endif

  if (allocateTransport(serverFd, channelId) < 0)
  {
    return -1;
  }

  switch (type)
  {
    case channel_cups:
    {
      channels_[channelId] = new CupsChannel(transports_[channelId], compressor_);
      break;
    }
    case channel_smb:
    {
      channels_[channelId] = new SmbChannel(transports_[channelId], compressor_);

      break;
    }
    case channel_media:
    {
      channels_[channelId] = new MediaChannel(transports_[channelId], compressor_);

      break;
    }
    case channel_http:
    {
      channels_[channelId] = new HttpChannel(transports_[channelId], compressor_);

      break;
    }
    case channel_font:
    {
      channels_[channelId] = new FontChannel(transports_[channelId], compressor_);

      break;
    }
    default:
    {
      channels_[channelId] = new SlaveChannel(transports_[channelId], compressor_);

      break;
    }
  }

  if (channels_[channelId] == NULL)
  {
    deallocateTransport(channelId);

    return -1;
  }

  increaseChannels(channelId);

  channels_[channelId] -> handleConfiguration();

  return 1;
}
