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

#include "NX.h"
#include "NXpack.h"

#include "Control.h"

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

//
// Flush immediately on prioritized messages.
//

#define FLUSH_PRIORITY                           0

//
// Maximum number of bytes sent for each token.
//

#define TOKEN_SIZE                               1536

//
// Maximum number of tokens that can be spent
// by the client proxy before having to block
// waiting for a token reply.
//

#define TOKEN_LIMIT                              24

//
// By default assume the proxy is running as a
// standalone program.
//

#define LINK_ENCRYPTED                           0

//
// Maximum number of pids the proxy will record
// and kill at shutdown.
//

#define KILL_LIMIT                               16

//
// Allocate on the NX client side channels whose
// ids are a multiple of 8 (starting from 0). All
// the other ids can be used to allocate channels
// at the NX server side (X client side).
//

#define CHANNEL_MASK                             0x07

//
// Kill session if control parameters cannot be
// negotiated before this timeout.
//

#define INIT_TIMEOUT                             60000

//
// Enter the congestion state if the remote does
// not reply to a ping within the given amount
// of time.
//

#define PING_TIMEOUT                             5000

//
// Only send one motion event any N milliseconds.
//

#define MOTION_TIMEOUT                           0


//
// Force an update of the congestion counter if
// the proxy is idle for this time.
//

#define IDLE_TIMEOUT                             50

//
// Close X connection if can't write before this
// timeout.
//

#define CHANNEL_TIMEOUT                          10000

//
// Warn user (or close proxy connection) if don't
// receive any data before this timeout.
//

#define PROXY_TIMEOUT                            120000

//
// How many milliseconds to wait for the shared
// memory completion event to become available.
//

#define SHMEM_TIMEOUT                            200
//
// Before closing down the proxy, wait for the
// given amount of miliseconds to let all the
// running applications to close down their
// connections.
//
// A null timeout will cause the proxy to wait
// indefinitely, until the watchdog process is
// killed. This is usually the way the proxy is
// started by the NX server. If on the other
// hand a timeout is given and there no channel
// is remaining, the proxy will be closed down
// using a small timeout, presently of 500 ms.
//

#define CLEANUP_TIMEOUT                          3000

//
// Wait this amount of milliseconds after any
// iteration of the house-keeping process.
//

#define KEEPER_TIMEOUT                           60000

//
// In case of timeout, select can return control
// to program earlier or later of this amount of
// ms. Consider this when calculating if timeout
// is elapsed.
//

#define LATENCY_TIMEOUT                          1

//
// Control memory allocation in transport
// and other classes.
//

#define TRANSPORT_X_BUFFER_SIZE                  131072
#define TRANSPORT_PROXY_BUFFER_SIZE              65536
#define TRANSPORT_GENERIC_BUFFER_SIZE            16384

#define TRANSPORT_X_BUFFER_THRESHOLD             262144
#define TRANSPORT_PROXY_BUFFER_THRESHOLD         131072
#define TRANSPORT_GENERIC_BUFFER_THRESHOLD       32768

//
// Never allow buffers to exceed this limit.
//

#define TRANSPORT_MAXIMUM_BUFFER_SIZE            393216

//
// Immediately flush the accumulated data to
// the X server if the write buffer exceeds
// this size.
//

#define TRANSPORT_FLUSH_BUFFER_SIZE              16384

//
// Defaults used for socket options.
//

#define OPTION_PROXY_KEEP_ALIVE                  0
#define OPTION_PROXY_LOW_DELAY                   1
#define OPTION_PROXY_CLIENT_NO_DELAY             1
#define OPTION_PROXY_SERVER_NO_DELAY             1
#define OPTION_CLIENT_NO_DELAY                   1
#define OPTION_SERVER_NO_DELAY                   1

#define OPTION_PROXY_RECEIVE_BUFFER              -1
#define OPTION_CLIENT_RECEIVE_BUFFER             -1
#define OPTION_SERVER_RECEIVE_BUFFER             -1

#define OPTION_PROXY_SEND_BUFFER                 -1
#define OPTION_CLIENT_SEND_BUFFER                -1
#define OPTION_SERVER_SEND_BUFFER                -1

#define OPTION_PROXY_RETRY_CONNECT               30
#define OPTION_PROXY_RETRY_ACCEPT                3
#define OPTION_SERVER_RETRY_CONNECT              3

//
// Defaults used for cache persistence.
//

#define PERSISTENT_CACHE_THRESHOLD               102400

#define PERSISTENT_CACHE_ENABLE_LOAD             1
#define PERSISTENT_CACHE_ENABLE_SAVE             1

#define PERSISTENT_CACHE_CHECK_ON_SHUTDOWN       0

#define PERSISTENT_CACHE_LOAD_PACKED             1
#define PERSISTENT_CACHE_LOAD_RENDER             1

#define PERSISTENT_CACHE_DISK_LIMIT              33554432

//
// Defaults used for image cache.
//

#define IMAGE_CACHE_ENABLE_LOAD                  0
#define IMAGE_CACHE_ENABLE_SAVE                  0

#define IMAGE_CACHE_DISK_LIMIT                   33554432

//
// Suggested defaults for read length parameters
// used by read buffer classes.
//

#define CLIENT_INITIAL_READ_SIZE                 8192
#define CLIENT_MAXIMUM_BUFFER_SIZE               262144

#define SERVER_INITIAL_READ_SIZE                 8192
#define SERVER_MAXIMUM_BUFFER_SIZE               65536

#define PROXY_INITIAL_READ_SIZE                  65536
#define PROXY_MAXIMUM_BUFFER_SIZE                262144 + 1024

#define GENERIC_INITIAL_READ_SIZE                8192
#define GENERIC_MAXIMUM_BUFFER_SIZE              8192

//
// Calculate bitrate in given time frames.
// Values are in milliseconds.
//

#define SHORT_BITRATE_TIME_FRAME                 5000
#define LONG_BITRATE_TIME_FRAME                  30000

//
// Bandwidth control. A value of 0 means no
// limit. Values are stored internally in
// bytes per second.
//

#define CLIENT_BITRATE_LIMIT                     0
#define SERVER_BITRATE_LIMIT                     0

//
// Default values for cache control. We limit
// the maximum size of a request to 262144 but
// we need to consider the replies, whose size
// may be up to 4MB.
//

#define MINIMUM_MESSAGE_SIZE                     4
#define MAXIMUM_MESSAGE_SIZE                     4194304
#define MAXIMUM_REQUEST_SIZE                     262144

#define CLIENT_TOTAL_STORAGE_SIZE                8388608
#define SERVER_TOTAL_STORAGE_SIZE                8388608

#define STORE_TIME_LIMIT                         3600

#define STORE_HITS_LOAD_BONUS                    10
#define STORE_HITS_ADD_BONUS                     20
#define STORE_HITS_LIMIT                         100

#define STORE_HITS_TOUCH                         1
#define STORE_HITS_UNTOUCH                       2

//
// Default parameters for message splitting.
//

#define SPLIT_MODE                               1
#define SPLIT_TIMEOUT                            50
#define SPLIT_TOTAL_SIZE                         128
#define SPLIT_TOTAL_STORAGE_SIZE                 1048576
#define SPLIT_DATA_THRESHOLD                     65536
#define SPLIT_DATA_PACKET_LIMIT                  24576

//
// Agent related parameters.
//

#define PACK_METHOD                              63
#define PACK_QUALITY                             9
#define HIDE_RENDER                              0
#define TAINT_REPLIES                            1
#define TAINT_THRESHOLD                          8

//
// In current version only X server support is
// implemented. Note that use of shared memory
// is negotiated according to options provided
// by the user.
//

#define SHMEM_CLIENT                             0
#define SHMEM_SERVER                             1

//
// Default size of shared memory segments used
// in MIT-SHM support.
//

#define SHMEM_CLIENT_SIZE                        0
#define SHMEM_SERVER_SIZE                        2097152

//
// What do we do at the end of session? If this
// flag is set, we launch a new client letting
// the user run a new NX session.
//

#define ENABLE_RESTART_ON_SHUTDOWN               0

//
// Do we produce a core dump on fatal errors?
//

#define ENABLE_CORE_DUMP_ON_ABORT                0

//
// Reopen the log file if it exceeds this size.
//

#define FILE_SIZE_LIMIT                          60000000

//
// Check periodically if we need to truncate the
// log file. By default check every minute.
//

#define FILE_SIZE_CHECK_TIMEOUT                  60000

//
// Set defaults for control. They should be what
// you get in case of 'local' connection.
//

Control::Control()
{
  ProxyMode   = proxy_undefined;
  ProxyStage  = stage_undefined;
  SessionMode = session_undefined;
  FlushPolicy = policy_undefined;
  LinkMode    = link_undefined;

  LinkEncrypted = LINK_ENCRYPTED;
  FlushPriority = FLUSH_PRIORITY;

  TokenSize  = TOKEN_SIZE;
  TokenLimit = TOKEN_LIMIT;

  ChannelMask = CHANNEL_MASK;

  InitTimeout   = INIT_TIMEOUT;
  PingTimeout   = PING_TIMEOUT;
  MotionTimeout = MOTION_TIMEOUT;
  IdleTimeout   = IDLE_TIMEOUT;

  ChannelTimeout = CHANNEL_TIMEOUT;
  ProxyTimeout   = PROXY_TIMEOUT;
  ShmemTimeout   = SHMEM_TIMEOUT;

  CleanupTimeout  = CLEANUP_TIMEOUT;
  KeeperTimeout   = KEEPER_TIMEOUT;
  LatencyTimeout  = LATENCY_TIMEOUT;

  FileSizeLimit        = FILE_SIZE_LIMIT;
  FileSizeCheckTimeout = FILE_SIZE_CHECK_TIMEOUT;

  EnableRestartOnShutdown = ENABLE_RESTART_ON_SHUTDOWN;

  KillDaemonOnShutdownLimit = KILL_LIMIT;

  KillDaemonOnShutdown = new int[KillDaemonOnShutdownLimit];

  for (int i = 0; i < KILL_LIMIT; i++)
  {
    KillDaemonOnShutdown[i] = -1;
  }

  KillDaemonOnShutdownNumber = 0;

  EnableCoreDumpOnAbort = ENABLE_CORE_DUMP_ON_ABORT;

  //
  // Collect statistics by default.
  //

  EnableStatistics = 1;

  //
  // Memory restrictions if any.
  //

  LocalMemoryLevel = -1;

  //
  // Compression must be negotiated between proxies.
  //

  LocalDeltaCompression   = -1;
  RemoteDeltaCompression  = -1;

  LocalDataCompression    = -1;
  LocalStreamCompression  = -1;

  RemoteDataCompression   = -1;
  RemoteStreamCompression = -1;

  LocalDataCompressionLevel     = -1;
  LocalDataCompressionThreshold = -1;
  LocalStreamCompressionLevel   = -1;

  RemoteDataCompressionLevel    = -1;
  RemoteStreamCompressionLevel  = -1;

  //
  // Transport buffers' allocation parameters.
  // 

  TransportXBufferSize            = TRANSPORT_X_BUFFER_SIZE;
  TransportProxyBufferSize        = TRANSPORT_PROXY_BUFFER_SIZE;
  TransportGenericBufferSize      = TRANSPORT_GENERIC_BUFFER_SIZE;

  TransportXBufferThreshold       = TRANSPORT_X_BUFFER_THRESHOLD;
  TransportProxyBufferThreshold   = TRANSPORT_PROXY_BUFFER_THRESHOLD;
  TransportGenericBufferThreshold = TRANSPORT_GENERIC_BUFFER_THRESHOLD;

  TransportMaximumBufferSize      = TRANSPORT_MAXIMUM_BUFFER_SIZE;

  //
  // Flush the write buffer if it exceeds
  // this size.
  //

  TransportFlushBufferSize = TRANSPORT_FLUSH_BUFFER_SIZE;

  //
  // Socket options.
  //

  OptionProxyKeepAlive      = OPTION_PROXY_KEEP_ALIVE;
  OptionProxyLowDelay       = OPTION_PROXY_LOW_DELAY;
  OptionProxyClientNoDelay  = OPTION_PROXY_CLIENT_NO_DELAY;
  OptionProxyServerNoDelay  = OPTION_PROXY_SERVER_NO_DELAY;
  OptionClientNoDelay       = OPTION_CLIENT_NO_DELAY;
  OptionServerNoDelay       = OPTION_SERVER_NO_DELAY;

  OptionProxyReceiveBuffer  = OPTION_PROXY_RECEIVE_BUFFER;
  OptionClientReceiveBuffer = OPTION_CLIENT_RECEIVE_BUFFER;
  OptionServerReceiveBuffer = OPTION_SERVER_RECEIVE_BUFFER;

  OptionProxySendBuffer     = OPTION_PROXY_SEND_BUFFER;
  OptionClientSendBuffer    = OPTION_CLIENT_SEND_BUFFER;
  OptionServerSendBuffer    = OPTION_SERVER_SEND_BUFFER;

  OptionProxyRetryAccept    = OPTION_PROXY_RETRY_ACCEPT;
  OptionProxyRetryConnect   = OPTION_PROXY_RETRY_CONNECT;
  OptionServerRetryConnect  = OPTION_SERVER_RETRY_CONNECT;

  //
  // Base NX directories.
  //

  HomePath   = NULL;
  RootPath   = NULL;
  SystemPath = NULL;
  TempPath   = NULL;
  ClientPath = NULL;

  //
  // Set defaults for handling persistent cache.
  //

  PersistentCachePath = NULL;
  PersistentCacheName = NULL;

  PersistentCacheThreshold = PERSISTENT_CACHE_THRESHOLD;

  PersistentCacheEnableLoad = PERSISTENT_CACHE_ENABLE_LOAD;
  PersistentCacheEnableSave = PERSISTENT_CACHE_ENABLE_SAVE;

  PersistentCacheCheckOnShutdown = PERSISTENT_CACHE_CHECK_ON_SHUTDOWN;

  PersistentCacheLoadPacked = PERSISTENT_CACHE_LOAD_PACKED;
  PersistentCacheLoadRender = PERSISTENT_CACHE_LOAD_RENDER;

  PersistentCacheDiskLimit = PERSISTENT_CACHE_DISK_LIMIT;

  //
  // Set defaults for image cache.
  //

  ImageCachePath = NULL;

  ImageCacheEnableLoad = IMAGE_CACHE_ENABLE_LOAD;
  ImageCacheEnableSave = IMAGE_CACHE_ENABLE_SAVE;

  ImageCacheDiskLimit = IMAGE_CACHE_DISK_LIMIT;

  //
  // Set defaults for the read buffers.
  //

  ClientInitialReadSize    = CLIENT_INITIAL_READ_SIZE;
  ClientMaximumBufferSize  = CLIENT_MAXIMUM_BUFFER_SIZE;

  ServerInitialReadSize    = SERVER_INITIAL_READ_SIZE;
  ServerMaximumBufferSize  = SERVER_MAXIMUM_BUFFER_SIZE;

  ProxyInitialReadSize     = PROXY_INITIAL_READ_SIZE;
  ProxyMaximumBufferSize   = PROXY_MAXIMUM_BUFFER_SIZE;

  GenericInitialReadSize   = GENERIC_INITIAL_READ_SIZE;
  GenericMaximumBufferSize = GENERIC_MAXIMUM_BUFFER_SIZE;

  ShortBitrateTimeFrame = SHORT_BITRATE_TIME_FRAME;
  LongBitrateTimeFrame  = LONG_BITRATE_TIME_FRAME;

  //
  // Bandwidth control.
  //

  LocalBitrateLimit   = -1;

  ClientBitrateLimit = CLIENT_BITRATE_LIMIT;
  ServerBitrateLimit = SERVER_BITRATE_LIMIT;

  //
  // Default parameters for message handling.
  //

  ClientTotalStorageSize = CLIENT_TOTAL_STORAGE_SIZE;
  ServerTotalStorageSize = SERVER_TOTAL_STORAGE_SIZE;

  LocalTotalStorageSize  = -1;
  RemoteTotalStorageSize = -1;

  StoreTimeLimit = STORE_TIME_LIMIT;

  StoreHitsLoadBonus = STORE_HITS_LOAD_BONUS;
  StoreHitsAddBonus  = STORE_HITS_ADD_BONUS;
  StoreHitsLimit     = STORE_HITS_LIMIT;

  StoreHitsTouch   = STORE_HITS_TOUCH;
  StoreHitsUntouch = STORE_HITS_UNTOUCH;

  MinimumMessageSize = MINIMUM_MESSAGE_SIZE;
  MaximumMessageSize = MAXIMUM_MESSAGE_SIZE;
  MaximumRequestSize = MAXIMUM_REQUEST_SIZE;

  SplitMode             = SPLIT_MODE;
  SplitTimeout          = SPLIT_TIMEOUT;
  SplitTotalSize        = SPLIT_TOTAL_SIZE;
  SplitTotalStorageSize = SPLIT_TOTAL_STORAGE_SIZE;
  SplitDataThreshold    = SPLIT_DATA_THRESHOLD;
  SplitDataPacketLimit  = SPLIT_DATA_PACKET_LIMIT;

  PackMethod     = PACK_METHOD;
  PackQuality    = PACK_QUALITY;
  HideRender     = HIDE_RENDER;
  TaintReplies   = TAINT_REPLIES;
  TaintThreshold = TAINT_THRESHOLD;

  ShmemClient = SHMEM_CLIENT;
  ShmemServer = SHMEM_SERVER;

  ShmemClientSize = SHMEM_CLIENT_SIZE;
  ShmemServerSize = SHMEM_SERVER_SIZE;

  //
  // Get local version number from compile time
  // settings. Version of remote proxy will be
  // checked at connection time.
  //

  RemoteVersionMajor = -1;
  RemoteVersionMinor = -1;
  RemoteVersionPatch = -1;
  RemoteVersionMaintenancePatch = -1;

  CompatVersionMajor = -1;
  CompatVersionMinor = -1;
  CompatVersionPatch = -1;
  CompatVersionMaintenancePatch = -1;

  LocalVersionMajor = NXMajorVersion();
  LocalVersionMinor = NXMinorVersion();
  LocalVersionPatch = NXPatchVersion();
  LocalVersionMaintenancePatch = NXMaintenancePatchVersion();

  #ifdef TEST
  *logofs << "Control: Major version is " << LocalVersionMajor
          << " minor is " << LocalVersionMinor << " patch is "
          << LocalVersionPatch << ".\n" << logofs_flush;
  #endif

  //
  // Initialize local implemented methods later
  // and negotiate remote methods at connection
  // time.
  //

  LocalUnpackMethods  = NULL;
  RemoteUnpackMethods = NULL;

  //
  // Set to 1 those methods which are implemented.
  //

  setLocalUnpackMethods();

  //
  // Set the protocol version at the
  // time the session is negotiated.
  //

  protoStep6_  = 0;
  protoStep7_  = 0;
  protoStep8_  = 0;
  protoStep9_  = 0;
  protoStep10_ = 0;
}

Control::~Control()
{
  if (KillDaemonOnShutdown != NULL)
  {
    delete [] KillDaemonOnShutdown;
  }

  if (HomePath != NULL)
  {
    delete [] HomePath;
  }

  if (RootPath != NULL)
  {
    delete [] RootPath;
  }

  if (SystemPath != NULL)
  {
    delete [] SystemPath;
  }

  if (TempPath != NULL)
  {
    delete [] TempPath;
  }

  if (ClientPath != NULL)
  {
    delete [] ClientPath;
  }

  if (PersistentCachePath != NULL)
  {
    delete [] PersistentCachePath;
  }

  if (PersistentCacheName != NULL)
  {
    delete [] PersistentCacheName;
  }

  if (LocalUnpackMethods != NULL)
  {
    delete [] LocalUnpackMethods;
  }

  if (RemoteUnpackMethods != NULL)
  {
    delete [] RemoteUnpackMethods;
  }

  if (ImageCachePath != NULL)
  {
    delete [] ImageCachePath;
  }
}

//
// Set the protocol step based on the
// remote version.
//

void Control::setProtoStep(int step)
{
  switch (step)
  {
    case 6:
    {
      protoStep6_  = 1;
      protoStep7_  = 0;
      protoStep8_  = 0;
      protoStep9_  = 0;
      protoStep10_ = 0;

      break;
    }
    case 7:
    {
      protoStep6_  = 1;
      protoStep7_  = 1;
      protoStep8_  = 0;
      protoStep9_  = 0;
      protoStep10_ = 0;

      break;
    }
    case 8:
    {
      protoStep6_  = 1;
      protoStep7_  = 1;
      protoStep8_  = 1;
      protoStep9_  = 0;
      protoStep10_ = 0;

      break;
    }
    case 9:
    {
      protoStep6_  = 1;
      protoStep7_  = 1;
      protoStep8_  = 1;
      protoStep9_  = 1;
      protoStep10_ = 0;

      break;
    }
    case 10:
    {
      protoStep6_  = 1;
      protoStep7_  = 1;
      protoStep8_  = 1;
      protoStep9_  = 1;
      protoStep10_ = 1;

      break;
    }
    default:
    {
      #ifdef PANIC
      *logofs << "Control: PANIC! Invalid protocol step "
              << "with value " << step << ".\n"
              << logofs_flush;
      #endif

      HandleCleanup();
    }
  }
}

int Control::getProtoStep()
{
  if (protoStep10_ == 1)
  {
    return 10;
  }
  else if (protoStep9_ == 1)
  {
    return 9;
  }
  else if (protoStep8_ == 1)
  {
    return 8;
  }
  else if (protoStep7_ == 1)
  {
    return 7;
  }
  else if (protoStep6_ == 1)
  {
    return 6;
  }
  else
  {
    #ifdef PANIC
    *logofs << "Control: PANIC! Can't identify the "
            << "protocol step.\n" << logofs_flush;
    #endif

    HandleCleanup();
  }
}

//
// Set here the pack/unpack methods that are
// implemented by this NX proxy.
//

void Control::setLocalUnpackMethods()
{
  LocalUnpackMethods  = new unsigned char[PACK_METHOD_LIMIT];
  RemoteUnpackMethods = new unsigned char[PACK_METHOD_LIMIT];

  for (int i = 0; i < PACK_METHOD_LIMIT; i++)
  {
    LocalUnpackMethods[i]  = 0;
    RemoteUnpackMethods[i] = 0;
  }

  LocalUnpackMethods[NO_PACK]                         = 1;

  LocalUnpackMethods[PACK_MASKED_8_COLORS]            = 1;
  LocalUnpackMethods[PACK_MASKED_64_COLORS]           = 1;
  LocalUnpackMethods[PACK_MASKED_256_COLORS]          = 1;
  LocalUnpackMethods[PACK_MASKED_512_COLORS]          = 1;
  LocalUnpackMethods[PACK_MASKED_4K_COLORS]           = 1;
  LocalUnpackMethods[PACK_MASKED_32K_COLORS]          = 1;
  LocalUnpackMethods[PACK_MASKED_64K_COLORS]          = 1;
  LocalUnpackMethods[PACK_MASKED_256K_COLORS]         = 1;
  LocalUnpackMethods[PACK_MASKED_2M_COLORS]           = 1;
  LocalUnpackMethods[PACK_MASKED_16M_COLORS]          = 1;

  LocalUnpackMethods[PACK_RAW_8_BITS]                 = 1;
  LocalUnpackMethods[PACK_RAW_16_BITS]                = 1;
  LocalUnpackMethods[PACK_RAW_24_BITS]                = 1;

  LocalUnpackMethods[PACK_COLORMAP_256_COLORS]        = 1;

  LocalUnpackMethods[PACK_JPEG_8_COLORS]              = 1;
  LocalUnpackMethods[PACK_JPEG_64_COLORS]             = 1;
  LocalUnpackMethods[PACK_JPEG_256_COLORS]            = 1;
  LocalUnpackMethods[PACK_JPEG_512_COLORS]            = 1;
  LocalUnpackMethods[PACK_JPEG_4K_COLORS]             = 1;
  LocalUnpackMethods[PACK_JPEG_32K_COLORS]            = 1;
  LocalUnpackMethods[PACK_JPEG_64K_COLORS]            = 1;
  LocalUnpackMethods[PACK_JPEG_256K_COLORS]           = 1;
  LocalUnpackMethods[PACK_JPEG_2M_COLORS]             = 1;
  LocalUnpackMethods[PACK_JPEG_16M_COLORS]            = 1;

  LocalUnpackMethods[PACK_PNG_8_COLORS]               = 1;
  LocalUnpackMethods[PACK_PNG_64_COLORS]              = 1;
  LocalUnpackMethods[PACK_PNG_256_COLORS]             = 1;
  LocalUnpackMethods[PACK_PNG_512_COLORS]             = 1;
  LocalUnpackMethods[PACK_PNG_4K_COLORS]              = 1;
  LocalUnpackMethods[PACK_PNG_32K_COLORS]             = 1;
  LocalUnpackMethods[PACK_PNG_64K_COLORS]             = 1;
  LocalUnpackMethods[PACK_PNG_256K_COLORS]            = 1;
  LocalUnpackMethods[PACK_PNG_2M_COLORS]              = 1;
  LocalUnpackMethods[PACK_PNG_16M_COLORS]             = 1;

  LocalUnpackMethods[PACK_RGB_16M_COLORS]             = 1;
  LocalUnpackMethods[PACK_RLE_16M_COLORS]             = 1;

  LocalUnpackMethods[PACK_ALPHA]                      = 1;
  LocalUnpackMethods[PACK_COLORMAP]                   = 1;

  LocalUnpackMethods[PACK_BITMAP_16M_COLORS]          = 1;
}
