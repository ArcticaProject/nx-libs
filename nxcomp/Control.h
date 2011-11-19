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

#ifndef Control_H
#define Control_H

#include "NXpack.h"

#include "Misc.h"
#include "Types.h"
#include "Timestamp.h"
#include "Statistics.h"

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

//
// This is the mode proxy is running.
//

typedef enum
{
  proxy_undefined = -1,
  proxy_client,
  proxy_server,
  proxy_last_tag
}
T_proxy_mode;

//
// Handle advances in the connection
// procedure.
//

typedef enum
{
  stage_undefined,
  stage_initializing,
  stage_connecting,
  stage_connected,
  stage_waiting_forwarder_version,
  stage_waiting_forwarder_options,
  stage_sending_forwarder_options,
  stage_waiting_proxy_version,
  stage_waiting_proxy_options,
  stage_sending_proxy_options,
  stage_waiting_proxy_caches,
  stage_sending_proxy_caches,
  stage_operational,
  stage_terminating,
  stage_terminated
}
T_proxy_stage;

//
// Hint about whether or not the proxy is
// connected to a NX agen.
//

typedef enum
{
  session_undefined = -1,
  session_agent,
  session_shadow,
  session_proxy,
  session_last_tag
}
T_session_mode;

//
// Set how data will be written to the peer
// socket.
//

typedef enum
{
  policy_undefined = -1,
  policy_immediate,
  policy_deferred
}
T_flush_policy;

//
// Link mode, after negotiation, will be set to
// any of the values defined in the NXproto.h.
//

#define link_undefined  -1;

//
// This class collects functioning parameters, 
// to be configurable at run-time. They are for
// the most part regarding timeouts, transport
// and message stores handling.
//

class Control
{
  public:

  //
  // Does proxy run in client mode or server mode?
  // As soon as we'll have gone through parsing of
  // the command line options the current mode will
  // be propagated to the control class.
  //

  T_proxy_mode ProxyMode;

  //
  // Goes from initializing to operational.
  //

  T_proxy_stage ProxyStage;

  //
  // Hint about type of session currently running.
  //

  T_session_mode SessionMode;

  //
  // Either immediate or defferred flushes.
  //

  T_flush_policy FlushPolicy;

  //
  // If set, the channels will try to flush the
  // encoded data whenever there is a prioritized
  // message. Depending on the flush policy, this
  // may determine an immediate flush or an event
  // being generated telling to the agent that it
  // should flush the proxy link.
  //

  int FlushPriority;

  //
  // Id corresponding to link speed negotiated
  // between proxies.
  //

  int LinkMode;

  //
  // Set if the proxy is connected to a program
  // providing the encryption of the point to
  // point communication.
  //

  int LinkEncrypted;

  //
  // Maximum number of bytes sent for each token.
  //

  int TokenSize;

  //
  // Maximum number of tokens that can be spent
  // by the client proxy before having to block
  // waiting for a reply.
  //

  int TokenLimit;

  //
  // Bitmask used to determine the distribution
  // of channel ids between the client and server
  // proxies.
  //

  int ChannelMask;

  //
  // Kill session if control parameters cannot
  // be negotiated before this timeout.
  //

  int InitTimeout;

  //
  // Enter the congestion state if the remote does
  // not reply to the ping within the given amount
  // of time.
  //

  int PingTimeout;

  //
  // Enqueue motion notify events in server channel.
  //

  int MotionTimeout;

  //
  // Force an update of the congestion counter if
  // the proxy is idle for this time.
  //

  int IdleTimeout;

  //
  // Close the connection if can't write before
  // this timeout.
  //

  int ChannelTimeout;

  //
  // Close connection if can't write before
  // this timeout.
  //

  int ProxyTimeout;

  //
  // How many milliseconds to wait for the shared
  // memory completion event to become available.
  //

  int ShmemTimeout;

  //
  // Wait for applications to complete at the time
  // proxy is shut down.
  //

  int CleanupTimeout;

  //
  // Wait this amount of milliseconds before any
  // iteration of the house-keeping process.
  //

  int KeeperTimeout;

  //
  // Adjust timeout calculations.
  //

  int LatencyTimeout;

  //
  // Maximum allowed size of log files.
  //

  int FileSizeLimit;
  int FileSizeCheckTimeout;

  //
  // What do we do at the end of session? If
  // this flag is set we launch a new client
  // letting the user run a new NX session.
  //

  int EnableRestartOnShutdown;

  //
  // The client can request the proxy to kill
  // a number of processes before exiting.
  //

  int *KillDaemonOnShutdown;
  int KillDaemonOnShutdownNumber;
  int KillDaemonOnShutdownLimit;

  //
  // Do we generate a core dump and exit in
  // case of program errors?
  //

  int EnableCoreDumpOnAbort;

  //
  // Is statistic output enabled?
  //

  int EnableStatistics;

  //
  // Version number of local and remote proxy.
  //

  int LocalVersionMajor;
  int LocalVersionMinor;
  int LocalVersionPatch;

  int RemoteVersionMajor;
  int RemoteVersionMinor;
  int RemoteVersionPatch;

  int CompatVersionMajor;
  int CompatVersionMinor;
  int CompatVersionPatch;

  //
  // Which unpack methods are implemented in proxy?
  //

  unsigned char *LocalUnpackMethods;
  unsigned char *RemoteUnpackMethods;

  //
  // Memory restriction imposed by user.
  //

  int LocalMemoryLevel;

  //
  // Use or not differential compression
  // and caching of X protocol messages.
  //

  int LocalDeltaCompression;
  int RemoteDeltaCompression;

  //
  // Compression of images and replies.
  //

  int LocalDataCompression;
  int LocalDataCompressionLevel;

  int RemoteDataCompression;
  int RemoteDataCompressionLevel;

  //
  // Minimum packet size to be compressed.
  //

  int LocalDataCompressionThreshold;

  //
  // Compress or not data flowing through the proxy
  // link. Level should be one of the ZLIB level as
  // Z_DEFAULT_COMPRESSION or Z_BEST_COMPRESSION.
  //

  int LocalStreamCompression;
  int LocalStreamCompressionLevel;

  int RemoteStreamCompression;
  int RemoteStreamCompressionLevel;

  //
  // Size of read operations in read buffer classes.
  //

  int ClientInitialReadSize;
  int ClientMaximumBufferSize;

  int ServerInitialReadSize;
  int ServerMaximumBufferSize;

  int ProxyInitialReadSize;
  int ProxyMaximumBufferSize;

  int GenericInitialReadSize;
  int GenericMaximumBufferSize;

  //
  // Set initial size and resize policy of
  // transport buffers. If maximum size is
  // exceeded, print a warning.
  //

  int TransportXBufferSize;
  int TransportProxyBufferSize;
  int TransportGenericBufferSize;

  int TransportXBufferThreshold;
  int TransportProxyBufferThreshold;
  int TransportGenericBufferThreshold;

  int TransportMaximumBufferSize;

  //
  // Flush the data produced for the channel
  // connection if it exceeds this size.
  //

  int TransportFlushBufferSize;

  //
  // Socket options.
  //

  int OptionProxyKeepAlive;
  int OptionProxyLowDelay;
  int OptionProxyClientNoDelay;
  int OptionProxyServerNoDelay;
  int OptionClientNoDelay;
  int OptionServerNoDelay;

  int OptionProxyReceiveBuffer;
  int OptionClientReceiveBuffer;
  int OptionServerReceiveBuffer;

  int OptionProxySendBuffer;
  int OptionClientSendBuffer;
  int OptionServerSendBuffer;

  int OptionProxyRetryAccept;
  int OptionProxyRetryConnect;
  int OptionServerRetryConnect;

  //
  // Calculate current bitrate on proxy link
  // using these observation periods. Value
  // is in milliseconds.  
  //

  int ShortBitrateTimeFrame;
  int LongBitrateTimeFrame;

  //
  // Limit the bandwidth usage of the proxy
  // link.
  //

  int LocalBitrateLimit;

  int ClientBitrateLimit;
  int ServerBitrateLimit;
  
  //
  // This is the limit imposed by user on
  // total cache size.
  //

  int ClientTotalStorageSize;
  int ServerTotalStorageSize;

  int LocalTotalStorageSize;
  int RemoteTotalStorageSize;

  //
  // Discard messages in store older than
  // this amount of seconds.
  //

  int StoreTimeLimit;

  //
  // Any new message in store starts with
  // this amount of hits.
  //

  int StoreHitsAddBonus;

  //
  // Unless it is loaded from persistent
  // cache.
  //

  int StoreHitsLoadBonus;

  //
  // Stop increasing hits at this threshold.
  //

  int StoreHitsLimit;

  //
  // Give a special weight to messages put or
  // taken from cache during startup time.
  //

  int StoreHitsStartup;

  //
  // Weight of touch and untoch operations.
  //

  int StoreHitsTouch;
  int StoreHitsUntouch;

  //
  // Directives on size of messages to cache.
  //

  int MinimumMessageSize;
  int MaximumMessageSize;

  //
  // Maximum size of a single X request.
  //

  int MaximumRequestSize;

  //
  // Currently selected streaming mode.
  //

  int SplitMode;

  //
  // Send new split data any given amount of
  // milliseconds.
  //

  int SplitTimeout;

  //
  // Maximum number of distinct messages and
  // maximum size in bytes of the temporary
  // storage.
  //

  int SplitTotalSize;
  int SplitTotalStorageSize;

  //
  // Don't split messages smaller that this
  // threshold and send no more than the
  // given amount of bytes in a single data
  // shot when streaming the split messages.
  //

  int SplitDataThreshold;
  int SplitDataPacketLimit;

  //
  // Agent related parameters. These values apply
  // to the agent which, at startup, must query
  // the user's settings.
  //

  int PackMethod;
  int PackQuality;
  int HideRender;
  int TaintReplies;
  int TaintThreshold;

  //
  // Do we allow shared memory image support in
  // client and or server?
  //

  int ShmemClient;
  int ShmemServer;

  //
  // Default size of shared memory segments used
  // in MIT-SHM support.
  //

  int ShmemClientSize;
  int ShmemServerSize;

  //
  // The user's home directory.
  //

  char *HomePath;

  //
  // The ".nx" directory, usually in
  // the user's home.
  //

  char *RootPath;

  //
  // Usually the /usr/NX" directory.
  //

  char *SystemPath;

  //
  // Usually the "/tmp" directory.
  //

  char *TempPath;

  //
  // The complete path to the client.
  //

  char *ClientPath;

  //
  // String containing path of cache
  // file selected for load or save.
  //

  char *PersistentCachePath;

  //
  // Name of selected cache file.
  //

  char *PersistentCacheName;

  //
  // Minimum size of cache in memory
  // to proceed to its storage on disk.
  //

  int PersistentCacheThreshold;

  //
  // Is persistent cache enabled?
  //

  int PersistentCacheEnableLoad;
  int PersistentCacheEnableSave;

  //
  // This is used just for test because
  // it requires that client and server
  // reside on the same machine.
  //

  int PersistentCacheCheckOnShutdown;

  //
  // Load packed image and render extension
  // message stores. This currently depends
  // on the type of session.
  //

  int PersistentCacheLoadPacked;
  int PersistentCacheLoadRender;

  //
  // Maximum disk consumption of message
  // caches on disk.
  //

  int PersistentCacheDiskLimit;

  //
  // String containing the base path
  // of image cache files.
  //

  char *ImageCachePath;

  //
  // Is image cache enabled?
  //

  int ImageCacheEnableLoad;
  int ImageCacheEnableSave;

  //
  // Maximum disk consumption of image
  // caches on disk.
  //

  int ImageCacheDiskLimit;

  //
  // Only constructor, destructor
  // and a few utility functions.
  //

  Control();

  ~Control();

  //
  // Should not leverage control to find channel
  // stores' size limits. As most of values in
  // control, this info must be moved elsewhere.
  //

  int getUpperStorageSize() const
  {
    return (ClientTotalStorageSize >
                ServerTotalStorageSize ?
                    ClientTotalStorageSize :
                        ServerTotalStorageSize);
  }

  int getLowerStorageSize() const
  {
    return (ClientTotalStorageSize <
                ServerTotalStorageSize ?
                    ClientTotalStorageSize :
                        ServerTotalStorageSize);
  }

  void setProtoStep(int step);

  int getProtoStep();

  int isProtoStep7()
  {
    return protoStep7_;
  }

  int isProtoStep8()
  {
    return protoStep8_;
  }

  int isProtoStep9()
  {
    return protoStep9_;
  }

  int isProtoStep10()
  {
    return protoStep10_;
  }

  private:

  //
  // Look in Control.cpp.
  //

  void setLocalUnpackMethods();

  //
  // Manage the encoding according
  // to the protocol version.
  //

  int protoStep6_;
  int protoStep7_;
  int protoStep8_;
  int protoStep9_;
  int protoStep10_;
};

#endif /* Control_H */
