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

#ifndef Proxy_H
#define Proxy_H

#include <sys/types.h>

#ifdef _AIX
#include <sys/select.h>
#endif

#include "Misc.h"
#include "Timestamp.h"

#include "List.h"
#include "Channel.h"
#include "Transport.h"
#include "EncodeBuffer.h"
#include "ProxyReadBuffer.h"

//
// Forward declaration as we
// need a pointer.
//

class Agent;

//
// Set the verbosity level.
//

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

//
// Log the important tracepoints related
// to writing packets to the peer proxy.
//

#undef  FLUSH

//
// Codes used for control messages in
// proxy-to-proxy protocol.
//
// The following codes are currently
// unused.
//
// code_alert_reply,
// code_reset_request,
// code_reset_reply,
// code_load_reply,
// code_save_reply,
// code_shutdown_reply,
// code_configuration_request,
// code_configuration_reply.
//
// These are for compatibility with
// old versions.
//
// code_sync_request,
// code_sync_reply,
//
// The code_new_aux_connection should not
// be used anymore. Auxiliary X connections
// are treated as normal X channels since
// version 1.5.0.
//

typedef enum
{
  code_new_x_connection,
  code_new_cups_connection,
  code_new_aux_connection,
  code_new_smb_connection,
  code_new_media_connection,
  code_switch_connection,
  code_drop_connection,
  code_finish_connection,
  code_begin_congestion,
  code_end_congestion,
  code_alert_request,
  code_alert_reply,
  code_reset_request,
  code_reset_reply,
  code_load_request,
  code_load_reply,
  code_save_request,
  code_save_reply,
  code_shutdown_request,
  code_shutdown_reply,
  code_control_token_request,
  code_control_token_reply,
  code_configuration_request,
  code_configuration_reply,
  code_statistics_request,
  code_statistics_reply,
  code_new_http_connection,
  code_sync_request,
  code_sync_reply,
  code_new_font_connection,
  code_new_slave_connection,
  code_finish_listeners,
  code_split_token_request,
  code_split_token_reply,
  code_data_token_request,
  code_data_token_reply,
  code_last_tag

} T_proxy_code;

typedef enum
{
  operation_in_negotiation,
  operation_in_messages,
  operation_in_configuration,
  operation_in_statistics,
  operation_last_tag

} T_proxy_operation;

typedef enum
{
  frame_ping,
  frame_data,

} T_frame_type;

typedef enum
{
  token_control,
  token_split,
  token_data

} T_token_type;

typedef enum
{
  load_if_any,
  load_if_first

} T_load_type;

class Proxy
{
  public:

  //
  // Maximum number of supported channels.
  //

  static const int CONNECTIONS_LIMIT  = 256;

  //
  // Numboer of token types.
  //

  static const int TOKEN_TYPES = 3;

  //
  // Lenght of buffer we use to add our
  // control messages plus the length of
  // the data frame.
  //

  static const int CONTROL_CODES_LENGTH = ENCODE_BUFFER_PREFIX_SIZE - 5;

  static const int CONTROL_CODES_THRESHOLD = CONTROL_CODES_LENGTH - 9;

  Proxy(int fd);

  virtual ~Proxy();

  //
  // Inform the proxy that the negotiation phase is
  // completed and that it can start handling binary
  // messages.
  //

  int setOperational();

  int getOperational()
  {
    return (operation_ != operation_in_negotiation);
  }

  int setReadDescriptors(fd_set *fdSet, int &fdMax, T_timestamp &tsMax);

  int setWriteDescriptors(fd_set *fdSet, int &fdMax, T_timestamp &tsMax);

  //
  // Perform the operation on the proxy
  // link or its own channels.
  //

  int handleRead(int &resultFds, fd_set &fdSet);

  int handleFlush(int &resultFds, fd_set &fdSet);

  int handleRead();

  int handleRead(int fd, const char *data = NULL, int size = 0);

  int handleEvents();

  int handleFlush();

  int handleFlush(int fd);

  int handlePing();

  int handleFinish();

  int handleShutdown();

  int handleStatistics(int type, ostream *statofs);

  int handleAlert(int alert);

  int handleRotate()
  {
    activeChannels_.rotate();

    return 1;
  }

  int handleChannelConfiguration();

  int handleSocketConfiguration();

  int handleLinkConfiguration();

  int handleCacheConfiguration();

  //
  // These must be called just after initialization to
  // tell to the proxy where the network connections
  // have to be forwarded.
  //

  virtual void handleDisplayConfiguration(const char *xServerDisplay, int xServerAddrFamily,
                                              sockaddr * xServerAddr, unsigned int xServerAddrLength) = 0;

  virtual void handlePortConfiguration(int cupsServerPort, int smbServerPort, int mediaServerPort,
                                           int httpServerPort, const char *fontServerPort) = 0;

  //
  // Create new tunneled channels.
  //

  virtual int handleNewConnection(T_channel_type type, int clientFd) = 0;

  virtual int handleNewConnectionFromProxy(T_channel_type type, int channelId) = 0;

  virtual int handleNewAgentConnection(Agent *agent) = 0;

  virtual int handleNewXConnection(int clientFd) = 0;

  virtual int handleNewXConnectionFromProxy(int channelId) = 0;

  int handleNewGenericConnection(int clientFd, T_channel_type type, const char *label);

  int handleNewGenericConnectionFromProxy(int channelId, T_channel_type type,
                                              const char *hostname, int port, const char *label);

  int handleNewGenericConnectionFromProxy(int channelId, T_channel_type type,
                                              const char *hostname, const char *path, const char *label);

  int handleNewSlaveConnection(int clientFd);

  int handleNewSlaveConnectionFromProxy(int channelId);

  //
  // Force closure of channels.
  //

  int handleCloseConnection(int clientFd);

  int handleCloseAllXConnections();

  int handleCloseAllListeners();

  //
  // Called when the loop has replaced
  // or closed a previous alert.
  //

  void handleResetAlert();

  //
  // Handle the persistent cache.
  //

  virtual int handleLoad(T_load_type type) = 0;

  virtual int handleSave() = 0;

  protected:

  //
  // Timeout related data:
  //
  // flush
  // split
  // motion
  //
  // Timeouts in milliseconds after which the
  // proxy will have to perform the operation.
  //
  // readTs, writeTs
  //
  // Timestamp of last packet received or sent
  // to remote proxy. Used to detect lost con-
  // nection.
  //
  // loopTs
  //
  // Timestamp of last loop completed by the
  // proxy
  //
  // pingTs
  //
  // Timestamp of last ping request sent to the
  // remote peer.
  //
  // alertTs
  //
  // Timestamp of last 'no data received' alert
  // dialog shown to the user.
  //
  // loadTs
  //
  // Were message stores populated from data on
  // disk.
  //
  // splitTs
  // motionTs
  //
  // Timestamps of the last operation of this
  // kind handled by the proxy.
  //

  typedef struct
  {
    int split;
    int motion;

    T_timestamp readTs;
    T_timestamp writeTs;

    T_timestamp loopTs;
    T_timestamp pingTs;
    T_timestamp alertTs;
    T_timestamp loadTs;

    T_timestamp splitTs;
    T_timestamp motionTs;

  } T_proxy_timeouts;

  //
  // Bytes accumulated so far while waiting
  // to send the next token, number of tokens
  // remaining for each token type and other
  // token related information.
  //

  typedef struct
  {
    int size;
    int limit;

    int bytes;
    int remaining;

    T_proxy_code request;
    T_proxy_code reply;

    T_token_type type;

  } T_proxy_token;

  int handlePostConnectionFromProxy(int channelId, int serverFd,
                                        T_channel_type type, const char *label);

  int handleDrain();

  int handleFrame(T_frame_type type);

  int handleFinish(int channelId);

  int handleDrop(int channelId);

  int handleFinishFromProxy(int channelId);

  int handleDropFromProxy(int channelId);

  int handleStatisticsFromProxy(int type);

  int handleStatisticsFromProxy(const unsigned char *message, unsigned int length);

  int handleNegotiation(const unsigned char *message, unsigned int length);

  int handleNegotiationFromProxy(const unsigned char *message, unsigned int length);

  int handleToken(T_frame_type type);

  int handleTokenFromProxy(T_proxy_token &token, int count);

  int handleTokenReplyFromProxy(T_proxy_token &token, int count);

  int handleSyncFromProxy(int channelId);

  int handleSwitch(int channelId);

  int handleControl(T_proxy_code code, int data = -1);

  int handleControlFromProxy(const unsigned char *message);

  //
  // Interleave reads of the X server
  // events while writing data to the
  // remote proxy.
  //

  virtual int handleAsyncEvents() = 0;

  //
  // Timer related functions.
  //

  protected:

  void setTimer(int value)
  {
    SetTimer(value);
  }

  void resetTimer()
  {
    ResetTimer();

    timer_ = 0;
  }

  public:

  void handleTimer()
  {
    timer_ = 1;
  }

  int getTimer()
  {
    return timer_;
  }

  //
  // Can the channel consume data and the
  // proxy produce more output?
  //

  int canRead(int fd) const
  {
    return (isTimeToRead() == 1 &&
                isTimeToRead(getChannel(fd)) == 1);
  }

  //
  // Can the proxy read more data from its
  // peer?
  //

  int canRead() const
  {
    return (transport_ -> readable() != 0);
  }

  int canFlush() const
  {
    return (encodeBuffer_.getLength() +
                controlLength_ + transport_ -> length() +
                    transport_ -> flushable() > 0);
  }

  int needFlush(int channelId) const
  {
    return (outputChannel_ == channelId &&
                encodeBuffer_.getLength() > 0);
  }

  int needFlush() const
  {
    return (encodeBuffer_.getLength() > 0);
  }

  int shouldFlush() const
  {
    if ((int) ((encodeBuffer_.getLength() +
            controlLength_) / statistics ->
                getStreamRatio()) >= control -> TokenSize)
    {
      #if defined(TEST) || defined(INFO) || defined(FLUSH)
      *logofs << "Proxy: FLUSH! Requesting a flush with "
              << (encodeBuffer_.getLength() + controlLength_)
              << " bytes and stream ratio " << (double) statistics ->
                 getStreamRatio() << ".\n" << logofs_flush;
      #endif

      return 1;
    }

    #if defined(TEST) || defined(INFO) || defined(FLUSH)
    *logofs << "Proxy: Not requesting a flush with "
            << (encodeBuffer_.getLength() + controlLength_)
            << " bytes and stream ratio " << (double) statistics ->
               getStreamRatio() << ".\n" << logofs_flush;
    #endif

    return 0;
  }

  int needDrain() const
  {
    return (congestion_ == 1 || transport_ -> length() >
                control -> TransportProxyBufferThreshold);
  }

  int getFd() const
  {
    return fd_;
  }

  int getFlushable(int fd) const
  {
    if (fd == fd_)
    {
      return (encodeBuffer_.getLength() + controlLength_ +
                  transport_ -> flushable());
    }

    return 0;
  }

  int getSplitSize()
  {
    return (clientStore_ != NULL ? clientStore_ ->
                getSplitTotalSize() : 0);
  }

  int getSplitStorageSize()
  {
    return (clientStore_ != NULL ? clientStore_ ->
                getSplitTotalStorageSize() : 0);
  }

  //
  // Returns the number of active channels
  // that currently managed by this proxy.
  //

  int getChannels(T_channel_type type = channel_none);

  //
  // If descriptor corresponds to a valid
  // channel, returns the type of traffic
  // carried by it.
  //

  T_channel_type getType(int fd);

  //
  // Given a channel type, returns the
  // literal name.
  //

  const char *getTypeName(T_channel_type type);

  //
  // Get a convenient name for 'localhost'.
  //

  const char *getComputerName();

  //
  // Set if we have initiated the shutdown
  // procedure and if the shutdown request
  // has been received from the remote.
  //

  int getFinish() const
  {
    return finish_;
  }

  int getShutdown() const
  {
    return shutdown_;
  }

  //
  // Interfaces to the transport buffers.
  //

  int getLength(int fd) const
  {
    if (fd == fd_)
    {
      return transport_ -> length();
    }

    int channelId = getChannel(fd);

    if (channelId < 0 || channels_[channelId] == NULL)
    {
      return 0;
    }

    return transports_[channelId] -> length();
  }

  int getPending(int fd) const
  {
    if (fd == fd_)
    {
      return transport_ -> pending();
    }

    int channelId = getChannel(fd);

    if (channelId < 0 || channels_[channelId] == NULL)
    {
      return 0;
    }

    return transports_[channelId] -> pending();
  }

  //
  // Check if the proxy or the given channel
  // has data in the buffer because of a
  // blocking write.
  //

  int getBlocked(int fd) const
  {
    if (fd == fd_)
    {
      return transport_ -> blocked();
    }

    int channelId = getChannel(fd);

    if (channelId < 0 || channels_[channelId] == NULL)
    {
      return 0;
    }

    return transports_[channelId] -> blocked();
  }

  //
  // Check if the proxy or the given channel has
  // data to read.
  //

  int getReadable(int fd) const
  {
    if (fd == fd_)
    {
      return transport_ -> readable();
    }

    int channelId = getChannel(fd);

    if (channelId < 0 || channels_[channelId] == NULL)
    {
      return 0;
    }

    return transports_[channelId] -> readable();
  }

  //
  // Return a vale between 0 and 9 in the case
  // of the proxy descriptor.
  //

  int getCongestion(int fd) const
  {
    if (fd == fd_)
    {
      return (agent_ != nothing && congestions_[agent_] == 1 ? 9 :
                  (int) statistics -> getCongestionInFrame());
    }

    int channelId = getChannel(fd);

    if (channelId < 0 || channels_[channelId] == NULL)
    {
      return 0;
    }

    return channels_[channelId] -> getCongestion();
  }

  //
  // Let the statistics class get info
  // from the message stores.
  //

  const ClientStore * const getClientStore() const
  {
    return clientStore_;
  }

  const ServerStore * const getServerStore() const
  {
    return serverStore_;
  }

  //
  // These can be called asynchronously by
  // channels during their read or write
  // loop.
  //

  int handleAsyncRead(int fd)
  {
    return handleRead(fd);
  }

  int handleAsyncCongestion(int fd)
  {
    int channelId = getChannel(fd);

    return handleControl(code_begin_congestion, channelId);
  }

  int handleAsyncDecongestion(int fd)
  {
    int channelId = getChannel(fd);

    return handleControl(code_end_congestion, channelId);
  }

  int handleAsyncSplit(int fd, Split *split)
  {
    return channels_[getChannel(fd)] ->
               handleSplitEvent(encodeBuffer_, split);
  }

  int handleAsyncInit()
  {
    return handleFlush();
  }

  int handleAsyncPriority()
  {
    if (control -> FlushPriority == 1)
    {
      return handleFlush();
    }

    return 0;
  }

  int canAsyncFlush() const
  {
    return shouldFlush();
  }

  int handleAsyncFlush()
  {
    return handleFlush();
  }

  int handleAsyncSwitch(int fd)
  {
    return handleSwitch(getChannel(fd));
  }

  int handleAsyncKeeperCallback()
  {
    KeeperCallback();

    return 1;
  }

  //
  // Internal interfaces used to verify the
  // availability of channels and the proxy
  // link.
  //

  protected:

  int isTimeToRead() const
  {
    if (congestion_ == 0 && transport_ ->
            blocked() == 0)
    {
      return 1;
    }
    else
    {
      #if defined(TEST) || defined(INFO)
      *logofs << "Proxy: Can't read from channels "
              << "with congestion " << congestion_
              << " and blocked " << transport_ ->
                 blocked() << ".\n" << logofs_flush;
      #endif

      return 0;
    }
  }

  int isTimeToRead(int channelId) const
  {
    if (channelId >= 0 && channelId < CONNECTIONS_LIMIT &&
            channels_[channelId] != NULL &&
                congestions_[channelId] == 0)
    {
      if (channels_[channelId] -> getType() == channel_x11 ||
              tokens_[token_data].remaining > 0 ||
                  channels_[channelId] ->
                      getFinish() == 1)
      {
        return 1;
      }

      #if defined(TEST) || defined(INFO)
      *logofs << "Proxy: Can't read from generic "
              << "descriptor FD#" << getFd(channelId)
              << " channel ID#" << channelId << " with "
              << tokens_[token_data].remaining
              << " data tokens remaining.\n"
              << logofs_flush;
      #endif

      return 0;
    }

    #if defined(TEST) || defined(INFO)

    if (channelId < 0 || channelId >= CONNECTIONS_LIMIT ||
            channels_[channelId] == NULL)
    {
      *logofs << "Proxy: WARNING! No valid channel for ID#"
              << channelId << ".\n" << logofs_flush;
    }
    else if (channels_[channelId] -> getFinish())
    {
      *logofs << "Proxy: Can't read from finishing "
              << "descriptor FD#" << getFd(channelId)
              << " channel ID#" << channelId << ".\n"
              << logofs_flush;
    }
    else if (congestions_[channelId] == 1)
    {
      *logofs << "Proxy: Can't read from congested "
              << "descriptor FD#" << getFd(channelId)
              << " channel ID#" << channelId << ".\n"
              << logofs_flush;
    }

    #endif

    return 0;
  }

  //
  // Handle the flush and split timeouts.
  // All these functions should round up
  // to the value of the latency timeout
  // to save a further loop.
  //

  protected:

  int isTimeToSplit() const
  {
    if (isTimestamp(timeouts_.splitTs) &&
            getTimeToNextSplit() <= control ->
                LatencyTimeout)
    {
      if (tokens_[token_split].remaining > 0)
      {
        return 1;
      }

      #if defined(TEST) || defined(INFO)
      *logofs << "Proxy: WARNING! Can't encode splits "
              << "with " << tokens_[token_split].remaining
              << " split tokens remaining.\n"
              << logofs_flush;
      #endif
    }

    return 0;
  }

  int isTimeToMotion() const
  {
    return (isTimestamp(timeouts_.motionTs) &&
                getTimeToNextMotion() <= control ->
                    LatencyTimeout);
  }

  int getTimeToNextSplit() const
  {
    #if defined(TEST) || defined(INFO) || defined(FLUSH)

    if (isTimestamp(timeouts_.splitTs) == 0)
    {
      #ifdef PANIC
      *logofs << "Proxy: PANIC! No split timeout was set "
              << "for proxy FD#" << fd_ << ".\n"
              << logofs_flush;
      #endif

      cerr << "Error" << ": No split timeout was set "
           << "for proxy FD#" << fd_ << ".\n";

      HandleCleanup();
    }

    #endif

    int diffTs = timeouts_.split -
                     diffTimestamp(timeouts_.splitTs,
                         getTimestamp());

    return (diffTs > 0 ? diffTs : 0);
  }

  int getTimeToNextMotion() const
  {
    #if defined(TEST) || defined(INFO) || defined(FLUSH)

    if (isTimestamp(timeouts_.motionTs) == 0)
    {
      #ifdef PANIC
      *logofs << "Proxy: PANIC! No motion timeout was set "
              << "for proxy FD#" << fd_ << ".\n"
              << logofs_flush;
      #endif

      cerr << "Error" << ": No motion timeout was set "
           << "for proxy FD#" << fd_ << ".\n";

      HandleCleanup();
    }

    #endif

    int diffTs = timeouts_.motion -
                     diffTimestamp(timeouts_.motionTs,
                         getTimestamp());

    return (diffTs > 0 ? diffTs : 0);
  }

  protected:

  //
  // Implement persistence of cache on disk.
  //

  virtual int handleLoadFromProxy() = 0;
  virtual int handleSaveFromProxy() = 0;

  int handleLoadStores();
  int handleSaveStores();

  char *handleSaveAllStores(const char *savePath) const;

  virtual int handleSaveAllStores(ostream *cachefs, md5_state_t *md5StateStream,
                                      md5_state_t *md5StateClient) const = 0;

  int handleSaveVersion(unsigned char *buffer, int &major, int &minor, int &patch) const;

  void handleFailOnSave(const char *fullName, const char *failContext) const;

  const char *handleLoadAllStores(const char *loadPath, const char *loadName) const;

  virtual int handleLoadAllStores(istream *cachefs, md5_state_t *md5StateStream) const = 0;

  int handleLoadVersion(const unsigned char *buffer, int &major, int &minor, int &patch) const;

  void handleFailOnLoad(const char *fullName, const char *failContext) const;

  //
  // Prepare for a new persistent cache.
  //

  int handleResetPersistentCache();

  //
  // Reset the stores in the case of a
  // failure loading the cache.
  //

  int handleResetStores();

  //
  // Reset the transport buffer and the
  // other counters.
  //

  void handleResetFlush();

  //
  // Utility functions used to map file
  // descriptors to channel ids.
  //

  protected:

  int allocateChannelMap(int fd);
  int checkChannelMap(int channelId);
  int assignChannelMap(int channelId, int fd);

  void cleanupChannelMap(int channelId);

  virtual int checkLocalChannelMap(int channelId) = 0;

  int addControlCodes(T_proxy_code code, int data);
  int addTokenCodes(T_proxy_token &token);

  int getFd(int channelId) const
  {
    if (channelId >= 0 && channelId < CONNECTIONS_LIMIT)
    {
      return fdMap_[channelId];
    }

    return -1;
  }

  int getChannel(int fd) const
  {
    if (fd >= 0 && fd < CONNECTIONS_LIMIT)
    {
      return channelMap_[fd];
    }

    return -1;
  }

  protected:

  void setSplitTimeout(int channelId);
  void setMotionTimeout(int channelId);

  void increaseChannels(int channelId);
  void decreaseChannels(int channelId);

  int allocateTransport(int channelFd, int channelId);
  int deallocateTransport(int channelId);

  //
  // The proxy has its own transport.
  //

  ProxyTransport *transport_;

  //
  // The static compressor is shared among
  // channels and all the message stores.
  //

  StaticCompressor *compressor_;

  //
  // Map NX specific opcodes.
  //

  OpcodeStore *opcodeStore_;

  //
  // Stores are shared between channels.
  //

  ClientStore *clientStore_;
  ServerStore *serverStore_;

  //
  // Client and server caches are shared
  // between channels.
  //

  ClientCache *clientCache_;
  ServerCache *serverCache_;

  //
  // The proxy's file descriptor.
  //

  int fd_;

  //
  // Channels currently selected for I/O
  // operations.
  //

  int inputChannel_;
  int outputChannel_;

  //
  // List of active channels.
  //

  List activeChannels_;

  //
  // Used to read data sent from peer proxy.
  //

  ProxyReadBuffer readBuffer_;

  //
  // Used to send data to peer proxy.
  //

  EncodeBuffer encodeBuffer_;

  //
  // Control messages' array.
  //

  int controlLength_;

  unsigned char controlCodes_[CONTROL_CODES_LENGTH];

  //
  // Table of channel classes taking
  // care of open X connections.
  //

  Channel *channels_[CONNECTIONS_LIMIT];

  //
  // Table of open sockets.
  //

  Transport *transports_[CONNECTIONS_LIMIT];

  //
  // Timeout related data.
  //

  T_proxy_timeouts timeouts_;

  //
  // Proxy can be decoding messages,
  // handling a link reconfiguration,
  // or decoding statistics.
  //

  int operation_;

  //
  // True if we are currently draining
  // the proxy link.
  //

  int draining_;

  //
  // Force flush because of prioritized
  // control messages to send.
  //

  int priority_;

  //
  // Set if we have initiated the close
  // down procedure.
  //

  int finish_;

  //
  // Remote peer requested the shutdown.
  //

  int shutdown_;

  //
  // We are in the middle of a network
  // congestion in the path to remote
  // proxy.
  //

  int congestion_;

  //
  // Channels at the remote end that
  // are not consuming their data.
  //

  int congestions_[CONNECTIONS_LIMIT];

  //
  // Is the timer expired?
  //

  int timer_;

  //
  // Did the proxy request an alert?
  //

  int alert_;

  //
  // The channel id of the agent.
  //

  int agent_;

  //
  // Token related data.
  //

  T_proxy_token tokens_[TOKEN_TYPES];

  //
  // Pointer to stream descriptor where
  // proxy is producing statistics.
  //

  ostream *currentStatistics_;

  private:

  //
  // Map channel ids to file descriptors.
  //

  int channelMap_[CONNECTIONS_LIMIT];
  int fdMap_[CONNECTIONS_LIMIT];
};

#endif /* Proxy_H */
