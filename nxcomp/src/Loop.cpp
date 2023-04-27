/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2011 NoMachine (http://www.nomachine.com)          */
/* Copyright (c) 2008-2014 Oleksandr Shneyder <o.shneyder@phoca-gmbh.de>  */
/* Copyright (c) 2014-2016 Ulrich Sibiller <uli42@gmx.de>                 */
/* Copyright (c) 2014-2016 Mihai Moldovan <ionic@ionic.de>                */
/* Copyright (c) 2011-2016 Mike Gabriel <mike.gabriel@das-netzwerkteam.de>*/
/* Copyright (c) 2015-2016 Qindel Group (http://www.qindel.com)           */
/*                                                                        */
/* NXCOMP, NX protocol compression and NX extensions to this software     */
/* are copyright of the aforementioned persons and companies.             */
/*                                                                        */
/* Redistribution and use of the present software is allowed according    */
/* to terms specified in the file LICENSE.nxcomp which comes in the       */
/* source distribution.                                                   */
/*                                                                        */
/* All rights reserved.                                                   */
/*                                                                        */
/* NOTE: This software has received contributions from various other      */
/* contributors, only the core maintainers and supporters are listed as   */
/* copyright holders. Please contact us, if you feel you should be listed */
/* as copyright holder, as well.                                          */
/*                                                                        */
/**************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <errno.h>
#include <signal.h>
#include <setjmp.h>

#include <math.h>
#include <ctype.h>
#include <string.h>
#include <dirent.h>
#include <pwd.h>

#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/utsname.h>
#include <sys/un.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "Misc.h"

#include <cstddef>

#ifdef __sun
#include <strings.h>
#endif

//
// MacOSX 10.4 defines socklen_t. This is
// intended to ensure compatibility with
// older versions.
//

#ifdef __APPLE__
#include <AvailabilityMacros.h>
#if MAC_OS_X_VERSION_MAX_ALLOWED <= MAC_OS_X_VERSION_10_3
typedef int socklen_t;
#endif
#endif

#ifdef _AIX
#include <strings.h>
#include <sys/select.h>
#endif

#if !(defined(__CYGWIN__) || defined(__CYGWIN32__))
#include <netinet/tcp.h>
#endif

//
// NX include files.
//

#include "NX.h"
#include "NXalert.h"

#include "Misc.h"
#include "Control.h"
#include "Socket.h"
#include "Statistics.h"
#include "Auth.h"
#include "Keeper.h"
#include "Agent.h"

#include "ClientProxy.h"
#include "ServerProxy.h"

#include "Message.h"
#include "ChannelEndPoint.h"
#include "Log.h"

//
// System specific defines.
//


//
// HP-UX hides this define.
//

#if defined(hpux) && !defined(RLIM_INFINITY)

#define RLIM_INFINITY  0x7fffffff

#endif

//
// Set the verbosity level.
//

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG
#undef  DUMP

//
// Enable log output in signal handler.
// This is likely to hang the proxy at
// random, at least on Linux.
//

#undef UNSAFE

//
// Let all logs go to the standard error.
// This is useful to interleave the Xlib
// log output with the proxy output in a
// single file.
//

#undef  MIXED

//
// Define this to check if the client and
// server caches match at shutdown. This
// is a test facility as it requires that
// both proxies are running on the same
// host.
//

#undef  MATCH

//
// If defined, reduce the size of the log
// file and be sure it never exceeds the
// limit.
//

#undef  QUOTA

//
// If defined, force very strict limits for
// the proxy tokens and force the proxy to
// enter often in congestion state.
//

#undef  STRICT

//
// Print a line in the log if the time we
// spent inside the select or handling the
// messages exceeded a given time value.
//

#undef  TIME

//
// This can be useful when testing the forwarding
// of the SSHD connection by nxssh to the agent.
// The debug output will go to a well known file
// that will be opened also by nxssh when BINDER
// is enabled there.
//

#undef  BINDER

//
// Define this to override the limits on
// the core dump size.
//

#define COREDUMPS

//
// Upper limit of pre-allocated buffers
// for string parameters.
//

#define DEFAULT_STRING_LENGTH              256

//
// Maximum length of remote options data
// passed by peer proxy at startup.
//

#define DEFAULT_REMOTE_OPTIONS_LENGTH      512

//
// Maximum length of NX display string.
//

#define DEFAULT_DISPLAY_OPTIONS_LENGTH     1024

//
// Maximum number of cache file names to
// send to the server side.
//

#define DEFAULT_REMOTE_CACHE_ENTRIES       100

//
// Maximum length of remote options string
// that can be received from the peer proxy.
//

#define MAXIMUM_REMOTE_OPTIONS_LENGTH      4096

//
// Macro is true if we determined our proxy
// mode.
//

#define WE_SET_PROXY_MODE         (control -> ProxyMode != proxy_undefined)

//
// Macro is true if our side is the one that
// should connect to remote.
//

#define WE_INITIATE_CONNECTION    (connectSocket.enabled())

//
// Is true if we must provide our credentials
// to the remote peer.
//

#define WE_PROVIDE_CREDENTIALS    (control -> ProxyMode == proxy_server)

//
// Is true if we listen for a local forwarder
// that will tunnel the traffic through a SSH
// or HTTP link.
//

#define WE_LISTEN_FORWARDER       (control -> ProxyMode == proxy_server && \
                                           listenSocket.enabled())

//
// You must define FLUSH in Misc.h if
// you want an immediate flush of the
// log output.
//

ostream *logofs = NULL;

//
// Other stream destriptors used for
// logging.
//

ostream *statofs = NULL;
ostream *errofs  = NULL;

//
// Save standard error's rdbuf here
// and restore it when exiting.
//

static streambuf *errsbuf = NULL;

//
// Allow faults to be recovered by
// jumping back into the main loop.
//

jmp_buf context;

//
// Provide operational parameters.
//

Control *control = NULL;

//
// Collect and print statistics.
//

Statistics *statistics = NULL;

//
// Keep data for X11 authentication.
//

Auth *auth = NULL;

//
// This class makes the hard work.
//

Proxy *proxy = NULL;

//
// Used to handle memory-to-memory
// transport to the X agent.
//

Agent *agent = NULL;

//
// The image cache house-keeping class.
//

Keeper *keeper = NULL;

//
// Callback set by the child process
// to be notified about signals.
//

int (*signalHandler)(int) = NULL;

//
// Signal handling functions (that are not already mentioned in Misc.h).
//

void InstallSignals();

static void RestoreSignals();
static void HandleSignal(int signal);

//
// Signal handling utilities.
//

static void InstallSignal(int signal, int action);
static void RestoreSignal(int signal);

static int HandleChildren();

int HandleChild(int child);
static int CheckChild(int pid, int status);
static int WaitChild(int child, const char *label, int force);

int CheckParent(const char *name, const char *type, int parent);

void RegisterChild(int child);

static int CheckAbort();

//
// Timer handling utilities (that are not already mentioned in Misc.h).
//

static void HandleTimer(int signal);

//
// Kill or check a running child.
//

static int KillProcess(int pid, const char *label, int signal, int wait);
static int CheckProcess(int pid, const char *label);

//
// Macros used to test the pid of a child.
//

#define IsFailed(pid)       ((pid) < 0)
#define IsRunning(pid)      ((pid) > 1)
#define IsNotRunning(pid)   ((pid) == 0)
#define IsRestarting(pid)   ((pid) == 1)

#define SetNotRunning(pid)  ((pid) = 0)
#define SetRestarting(pid)  ((pid) = 1)

//
// Start or restart the house-keeper process.
//

static void StartKeeper();

//
// Cleanup functions.
//

void CleanupConnections();
void CleanupListeners();
void CleanupSockets();
void CleanupGlobal();

static void CleanupChildren();
static void CleanupLocal();
static void CleanupKeeper();
static void CleanupStreams();

//
// Loop forever until the connections
// to the peer proxy is dropped.
//

static void WaitCleanup();

//
// Initialization functions.
//

static int InitBeforeNegotiation();
static int SetupProxyConnection();
static int InitAfterNegotiation();
static int SetupProxyInstance();
static int SetupAuthInstance();
static int SetupAgentInstance();

static void SetupTcpSocket();
static void SetupUnixSocket();
static void SetupServiceSockets();
static void SetupDisplaySocket(int &addr_family, sockaddr *&addr,
                                  unsigned int &addr_length);

//
// Setup a listening socket and accept
// a new connection.
//

static int ListenConnection(ChannelEndPoint &endPoint, const char *label);
static int ListenConnectionTCP(const char *host, long port, const char *label);
static int ListenConnectionUnix(const char *path, const char *label);
static int ListenConnectionAny(sockaddr *addr, socklen_t addrlen, const char *label);
static int AcceptConnection(int fd, int domain, const char *label);

//
// Other convenience functions.
//

static int PrepareProxyConnectionTCP(char** hostName, long int* portNum, int* timeout, int* proxyFileDescriptor, int* reason);
static int PrepareProxyConnectionUnix(char** path, int* timeout, int* proxyFileDescriptor, int* reason);

static int WaitForRemote(ChannelEndPoint &socketAddress);
static int ConnectToRemote(ChannelEndPoint &socketAddress);

static int SendProxyOptions(int fd);
static int SendProxyCaches(int fd);
static int ReadProxyVersion(int fd);
static int ReadProxyOptions(int fd);
static int ReadProxyCaches(int fd);
static int ReadForwarderVersion(int fd);
static int ReadForwarderOptions(int fd);

static int ReadRemoteData(int fd, char *buffer, int size, char stop);
static int WriteLocalData(int fd, const char *buffer, int size);

static void PrintVersionInfo();
static void PrintProcessInfo();
static void PrintConnectionInfo();
static void PrintUsageInfo(const char *option, const int error);
static void PrintOptionIgnored(const char *type, const char *name, const char *value);

//
// This is not static to avoid a warning.
//

void PrintCopyrightInfo();

static const char *GetOptions(const char *options);
static const char *GetArg(int &argi, int argc, const char **argv);
static int CheckArg(const char *type, const char *name, const char *value);
static int ParseArg(const char *type, const char *name, const char *value);
static int ValidateArg(const char *type, const char *name, const char *value);
static void SetAndValidateChannelEndPointArg(const char *type, const char *name, const char *value,
                                             ChannelEndPoint &endPoint);
static int LowercaseArg(const char *type, const char *name, char *value);
static int CheckSignal(int signal);

extern "C"
{
  int ParseCommandLineOptions(int argc, const char **argv);
  int ParseEnvironmentOptions(const char *env, int force);
  int ParseBindOptions(char **host, int *port);
}

static int ParseFileOptions(const char *file);
static int ParseRemoteOptions(char *opts);
static int ParseForwarderOptions(char *opts);

//
// These functions are used to parse literal
// values provided by the user and set the
// control parameters accordingly.
//

static int ParseLinkOption(const char *opt);
static int ParseBitrateOption(const char *opt);
static int ParseCacheOption(const char *opt);
static int ParseShmemOption(const char *opt);
static int ParseImagesOption(const char *opt);
static int ParsePackOption(const char *opt);

//
// Set host and port where NX proxy is supposed
// to be listening in case such parameters are
// given on the command line.
//

static int ParseHostOption(const char *opt, char *host, long &port);

//
// Translate a font server port specification
// to the corresponding Unix socket path.
//

static int ParseFontPath(char *path);

//
// Translate a pack method id in a literal.
//

static int ParsePackMethod(const int method, const int quality);

//
// Try to increase the size of the allowed
// core dumps.
//

static int SetCore();

//
// Set the proxy mode to either client or
// server.
//

static int SetMode(int mode);

//
// Determine the path of the NX_* directories
// from the environment.
//

static int SetDirectories();

//
// Set the defaults used for the log file and
// statistics.
//

static int SetLogs();

//
// Check if local and remote protocol versions
// are compatible and, eventually, downgrade
// local version to the minimum level that is
// known to work.
//

static int SetVersion();

//
// Setup the listening TCP ports used for the
// additional channels according to user's
// wishes.
//

static int SetPorts();

//
// Set the maximum number of open descriptors.
//

static int SetDescriptors();

//
// Set the path used for choosing the cache.
// It must be selected after determining the
// session type.
//

static int SetCaches();

//
// Initialize, one after the other, all the
// configuration parameters.
//

static int SetParameters();

//
// Set the specific configuration parameter.
//

static int SetSession();
static int SetStorage();
static int SetShmem();
static int SetPack();
static int SetImages();
static int SetLimits();

//
// Set up the control parameters based on
// the link speed negotiated between the
// proxies.
//

static int SetLink();

static int SetLinkModem();
static int SetLinkIsdn();
static int SetLinkAdsl();
static int SetLinkWan();
static int SetLinkLan();

//
// Adjust the compression parameters.
//

static int SetCompression();

static int SetCompressionModem();
static int SetCompressionIsdn();
static int SetCompressionAdsl();
static int SetCompressionWan();
static int SetCompressionLan();

//
// Determine the NX paths based on the
// user's parameters or the environment.
//

char *GetClientPath();

static char *GetSystemPath();
static char *GetHomePath();
static char *GetTempPath();
static char *GetRootPath();
static char *GetCachePath();
static char *GetImagesPath();
static char *GetSessionPath();
static char *GetLastCache(char *list, const char *path);

static int OpenLogFile(char *name, ostream *&stream);
static int ReopenLogFile(char *name, ostream *&stream, int limit);

//
// Perform operations on the managed
// descriptors in the main loop.
//

static void handleCheckSessionInLoop();
static void handleCheckBitrateInLoop();

static void handleCheckSelectInLoop(int &setFDs, fd_set &readSet,
                                        fd_set &writeSet, T_timestamp selectTs);
static void handleCheckResultInLoop(int &resultFDs, int &errorFDs, int &setFDs, fd_set &readSet,
                                        fd_set &writeSet, struct timeval &selectTs,
                                            struct timeval &startTs);
static void handleCheckStateInLoop(int &setFDs);
static void handleCheckSessionInConnect();

static inline void handleSetReadInLoop(fd_set &readSet, int &setFDs, struct timeval &selectTs);
static inline void handleSetWriteInLoop(fd_set &writeSet, int &setFDs, struct timeval &selectTs);
static inline void handleSetListenersInLoop(fd_set &writeSet, int &setFDs);
static inline void handleSetAgentInLoop(int &setFDs, fd_set &readSet, fd_set &writeSet,
                                            struct timeval &selectTs);

static void handleAlertInLoop();
static void handleStatisticsInLoop();

static inline void handleAgentInLoop(int &resultFDs, int &errorFDs, int &setFDs, fd_set &readSet,
                                         fd_set &writeSet, struct timeval &selectTs);
static inline void handleAgentLateInLoop(int &resultFDs, int &errorFDs, int &setFDs, fd_set &readSet,
                                             fd_set &writeSet, struct timeval &selectTs);

static inline void handleReadableInLoop(int &resultFDs, fd_set &readSet);
static inline void handleWritableInLoop(int &resultFDs, fd_set &writeSet);

static inline void handleRotateInLoop();
static inline void handleEventsInLoop();
static inline void handleFlushInLoop();

//
// Manage the proxy link during the negotiation
// phase.
//

static void handleNegotiationInLoop(int &setFDs, fd_set &readSet,
                                        fd_set &writeSet, T_timestamp &selectTs);

//
// Print the 'terminating' messages in the
// session log.
//

static inline void handleTerminatingInLoop();
static inline void handleTerminatedInLoop();

//
// Monitor the size of the log file.
//

static void handleLogReopenInLoop(T_timestamp &lTs, T_timestamp &nTs);

//
// Directory where the NX binaries and libraries reside.
//

static char systemDir[DEFAULT_STRING_LENGTH] = { 0 };

//
// Directory used for temporary files.
//

static char tempDir[DEFAULT_STRING_LENGTH] = { 0 };

//
// Actually the full path to the client.
//

static char clientDir[DEFAULT_STRING_LENGTH] = { 0 };

//
// User's home directory.
//

static char homeDir[DEFAULT_STRING_LENGTH] = { 0 };

//
// Root of directory structure to be created by proxy.
//

static char rootDir[DEFAULT_STRING_LENGTH] = { 0 };

//
// Root of statistics and log files to be created by proxy.
//

static char sessionDir[DEFAULT_STRING_LENGTH] = { 0 };

//
// Log files for errors and statistics. Error log is
// the place where we print also debug information.
// Both files are closed, deleted and reopened as
// their size exceed the limit set in control class.
// The session log is not reopened, as it is used by
// the NX client and server to track the advance of
// the session.
//

static char errorsFileName[DEFAULT_STRING_LENGTH]  = { 0 };
static char statsFileName[DEFAULT_STRING_LENGTH]   = { 0 };
static char sessionFileName[DEFAULT_STRING_LENGTH] = { 0 };
static char optionsFileName[DEFAULT_STRING_LENGTH] = { 0 };

//
// String literal representing selected link speed
// parameter. Value is translated in control values
// used by proxies to stay synchronized.
//

static char linkSpeedName[DEFAULT_STRING_LENGTH] = { 0 };

//
// String literal representing selected
// cache size.
//

static char cacheSizeName[DEFAULT_STRING_LENGTH] = { 0 };

//
// String literal representing selected
// shared memory segment size.
//

static char shsegSizeName[DEFAULT_STRING_LENGTH] = { 0 };

//
// String literal of images cache size.
//

static char imagesSizeName[DEFAULT_STRING_LENGTH] = { 0 };

//
// String literal for bandwidth limit.
//

static char bitrateLimitName[DEFAULT_STRING_LENGTH] = { 0 };

//
// String literal for image packing method.
//

static char packMethodName[DEFAULT_STRING_LENGTH] = { 0 };

//
// Product name provided by the server or client.
//

static char productName[DEFAULT_STRING_LENGTH] = { 0 };

//
// Its corresponding value from NXpack.h.
//

static int packMethod  = -1;
static int packQuality = -1;

//
// String literal for session type. Persistent caches
// are searched in directory whose name matches this
// parameter.
//

static char sessionType[DEFAULT_STRING_LENGTH] = { 0 };

//
// Unique id assigned to session. It is used as
// name of directory where all files are placed.
//

static char sessionId[DEFAULT_STRING_LENGTH] = { 0 };

//
// Set if we already parsed the options.
//

static int parsedOptions = 0;
static int parsedCommand = 0;

//
// Buffer data received from the remote proxy at
// session negotiation.
//

static char remoteData[MAXIMUM_REMOTE_OPTIONS_LENGTH] = { 0 };
static int  remotePosition = 0;

//
// Main loop file descriptors.
//

static int tcpFD   = -1;
static int unixFD  = -1;
static int cupsFD  = -1;
static int auxFD   = -1;
static int smbFD   = -1;
static int mediaFD = -1;
static int httpFD  = -1;
static int fontFD  = -1;
static int slaveFD = -1;
static int proxyFD = -1;

//
// Used for internal communication
// with the X agent.
//

static int agentFD[2] = { -1, -1 };

//
// Flags determining which protocols and
// ports are forwarded.
//

int useUnixSocket = 1;

static int useTcpSocket   = 1;
static int useCupsSocket  = 0;
static int useAuxSocket   = 0;
static int useSmbSocket   = 0;
static int useMediaSocket = 0;
static int useHttpSocket  = 0;
static int useFontSocket  = 0;
static int useSlaveSocket = 0;
static int useAgentSocket = 0;

//
// Set if the launchd service is running
// and its socket must be used as X socket.
//

static int useLaunchdSocket = 0;

//
// Set by user if he/she wants to modify
// the default TCP_NODELAY option as set
// in control.
//

static int useNoDelay = -1;

//
// Set if user wants to override default
// flush timeout set according to link.
//

static int usePolicy = -1;

//
// Set if user wants to hide the RENDER
// extension or wants to short-circuit
// some simple replies at client side.
//

static int useRender = -1;
static int useTaint  = -1;

//
// Set if the user wants to reduce the
// nominal size of the token messages
// exchanged between the proxies.
//

static int useStrict = -1;

//
// Set if the proxy is running as part
// of SSH on the client.
//

static int useEncryption = -1;

//
// Name of Unix socket created by the client proxy to
// accept client connections. File must be unlinked
// by cleanup function.
//

static char unixSocketName[DEFAULT_STRING_LENGTH] = { 0 };

//
// Other parameters.
//

static char acceptHost[DEFAULT_STRING_LENGTH]  = { 0 };
static char displayHost[DEFAULT_STRING_LENGTH] = { 0 };
static char authCookie[DEFAULT_STRING_LENGTH]  = { 0 };

static int loopbackBind = DEFAULT_LOOPBACK_BIND;
static int proxyPort = DEFAULT_NX_PROXY_PORT;
static int xPort     = DEFAULT_NX_X_PORT;

//
// Used to setup the connection the real
// X display socket.
//

static int xServerAddrFamily          = -1;
static sockaddr *xServerAddr          = NULL;
static unsigned int xServerAddrLength = 0;

//
// The representation of a Unix socket path or
// a bind address, denoting where the local proxy
// will await the peer connection.
//

static ChannelEndPoint listenSocket;

//
// The TCP host and port or Unix file socket where
// the remote proxy will be contacted.
//

static ChannelEndPoint connectSocket;

//
// Helper channels are disabled by default.
//

static ChannelEndPoint cupsPort;
static ChannelEndPoint auxPort;
static ChannelEndPoint smbPort;
static ChannelEndPoint mediaPort;
static ChannelEndPoint httpPort;
static ChannelEndPoint slavePort;

//
// Can be either a port number or a Unix
// socket.
//

static char fontPort[DEFAULT_STRING_LENGTH] = { 0 };

//
// Host and port where the existing proxy
// is running.
//

static char bindHost[DEFAULT_STRING_LENGTH] = { 0 };
static int  bindPort = -1;

//
// Pointers to the callback functions and
// parameter set by the agent
//

static void (*flushCallback)(void *, int) = NULL;
static void *flushParameter = NULL;

static void (*statisticsCallback)(void *, int) = NULL;
static void *statisticsParameter = NULL;

//
// State variables shared between the init
// function and the main loop.
//

T_timestamp initTs;
T_timestamp startTs;
T_timestamp logsTs;
T_timestamp nowTs;

//
// This is set to the main proxy process id.
//

int lastProxy = 0;

//
// Set to last dialog process launched by proxy.
//

int lastDialog = 0;

//
// Set to watchdog process launched by proxy.
//

int lastWatchdog = 0;

//
// Set if a cache house-keeper process is running.
//

int lastKeeper = 0;

//
// Let an inner routine register the pid of a slave
// process.
//

static int lastChild = 0;

//
// Exit code of the last child process exited.
//

static int lastStatus = 0;

//
// Set if shutdown was requested through a signal.
//

static int lastKill = 0;

//
// Set if the agent confirmed the destruction of
// the NX transport.
//

static int lastDestroy = 0;

//
// This is set to the code and local flag of the
// last requested alert.
//

static struct
{
  int code;
  int local;

} lastAlert;

//
// Manage the current signal masks.
//

static struct
{
  sigset_t saved;

  int blocked;
  int installed;

  int enabled[32];
  int forward[32];

  struct sigaction action[32];

} lastMasks;

//
// Manage the current timer.
//

static struct
{
  struct sigaction action;
  struct itimerval value;
  struct timeval   start;
  struct timeval   next;

} lastTimer;

//
// This is set to last signal received in handler.
//

static int lastSignal = 0;

//
// Set to the last time bytes readable were queried
// by the agent.
//

static T_timestamp lastReadableTs = nullTimestamp();

//
// Here are interfaces declared in NX.h.
//

int NXTransProxy(int fd, int mode, const char* options)
{
  //
  // Let the log temporarily go to the standard
  // error. Be also sure we have a jump context,
  // in the case any subsequent operation will
  // cause a cleanup.
  //

  if (logofs == NULL)
  {
    logofs = &cerr;
  }

  if (setjmp(context) == 1)
  {
    nxinfo << "NXTransProxy: Out of the long jump with pid '"
           << lastProxy << "'.\n" << std::flush;

    return -1;
  }

  //
  // Check if have already performed a parsing of
  // parameters, as in the case we are running as
  // a stand-alone process. If needed create the
  // parameters repository
  //

  if (control == NULL)
  {
    control = new Control();
  }

  lastProxy = getpid();

  nxinfo << "NXTransProxy: Main process started with pid '"
         << lastProxy << "'.\n" << std::flush;

  SetMode(mode);

  if (mode == NX_MODE_CLIENT)
  {
    if (fd != NX_FD_ANY)
    {

      nxinfo << "NXTransProxy: Agent descriptor for X client connections is FD#"
             << fd << ".\n" << std::flush;

      nxinfo << "NXTransProxy: Disabling listening on further X client connections.\n"
             << std::flush;


      useTcpSocket   = 0;
      useUnixSocket  = 0;
      useAgentSocket = 1;

      agentFD[1] = fd;
    }
  }
  else if (mode == NX_MODE_SERVER)
  {
    if (fd != NX_FD_ANY)
    {
      nxinfo << "NXTransProxy: PANIC! Agent descriptor for X server connections "
             << "not supported yet.\n" << std::flush;

      cerr << "Error" << ": Agent descriptor for X server connections "
           << "not supported yet.\n";

      return -1;
    }
  }

  const char *env = GetOptions(options);

  if (ParseEnvironmentOptions(env, 0) < 0)
  {
    cerr << "Error" << ": Parsing of NX transport options failed.\n";

    return -1;
  }

  //
  // Set the path of the NX directories.
  //

  SetDirectories();

  //
  // Open the log files.
  //

  SetLogs();

  nxinfo << "NXTransProxy: Going to run the NX transport loop.\n"
         << std::flush;

  WaitCleanup();

  //
  // This function should never return.
  //

  exit(0);
}

void NXTransExit(int code)
{
  if (logofs == NULL)
  {
    logofs = &cerr;
  }

  static int recurse;

  if (++recurse > 1)
  {
    nxinfo << "NXTransExit: Aborting process with pid '"
           << getpid() << "' due to recursion through "
           << "exit.\n" << std::flush;

    abort();
  }

  nxinfo << "NXTransExit: Process with pid '"
         << getpid() << "' called exit with code '"
         << code << "'.\n" << std::flush;

  if (control != NULL)
  {
    //
    // Be sure that there we can detect the
    // termination of the watchdog.
    //

    EnableSignals();

    //
    // Close the NX transport if it was not
    // shut down already.
    //

    NXTransDestroy(NX_FD_ANY);
  }

  exit(code);
}

int NXTransParseCommandLine(int argc, const char **argv)
{
  return ParseCommandLineOptions(argc, argv);
}

int NXTransParseEnvironment(const char *env, int force)
{
  return ParseEnvironmentOptions(env, force);
}

void NXTransCleanup()
{
  HandleCleanup();
}

void NXTransCleanupForReconnect()
{
  HandleCleanupForReconnect();
}

//
// Check the parameters for subsequent
// initialization of the NX transport.
//

int NXTransCreate(int fd, int mode, const char* options)
{
  if (logofs == NULL)
  {
    logofs = &cerr;
  }

  //
  // Be sure we have a jump context, in the
  // case a subsequent operation will cause
  // a cleanup.
  //

  if (setjmp(context) == 1)
  {
    return -1;
  }

  //
  // Create the parameters repository
  //

  if (control != NULL)
  {
    nxfatal << "NXTransCreate: PANIC! The NX transport seems "
            << "to be already running.\n" << std::flush;

    cerr << "Error" << ": The NX transport seems "
         << "to be already running.\n";

    return -1;
  }

  control = new Control();

  if (control == NULL)
  {
    nxfatal << "Loop: PANIC! Error creating the NX transport.\n"
            << std::flush;

    cerr << "Error" << ": Error creating the NX transport.\n";

    return -1;
  }

  lastProxy = getpid();

  nxinfo << "NXTransCreate: Caller process running with pid '"
         << lastProxy << "'.\n" << std::flush;

  //
  // Set the local proxy mode an parse the
  // display NX options.
  //

  SetMode(mode);

  const char *env = GetOptions(options);

  if (ParseEnvironmentOptions(env, 0) < 0)
  {
    cerr << "Error" << ": Parsing of NX transport options failed.\n";

    return -1;
  }

  //
  // Set the path of the NX directories.
  //

  SetDirectories();

  //
  // Open the log files.
  //

  SetLogs();

  //
  // Use the provided descriptor.
  //

  proxyFD = fd;

  nxinfo << "NXTransCreate: Called with NX proxy descriptor '"
         << proxyFD << "'.\n" << std::flush;

  nxinfo << "NXTransCreate: Creation of the NX transport completed.\n"
         << std::flush;

  return 1;
}

//
// Tell the proxy to use the descriptor as the internal
// connection to the X client side NX agent. This will
// have the side effect of disabling listening for add-
// itional X client connections.
//

int NXTransAgent(int fd[2])
{
  //
  // Be sure we have a jump context, in the
  // case a subsequent operation will cause
  // a cleanup.
  //

  if (logofs == NULL)
  {
    logofs = &cerr;
  }

  if (setjmp(context) == 1)
  {
    return -1;
  }

  if (control == NULL)
  {
    cerr << "Error" << ": Can't set the NX agent without a NX transport.\n";

    return -1;
  }
  else if (control -> ProxyMode != proxy_client)
  {
    nxfatal << "NXTransAgent: Invalid mode while setting the NX agent.\n"
            << std::flush;

    cerr << "Error" << ": Invalid mode while setting the NX agent.\n\n";

    return -1;
  }

  useTcpSocket   = 0;
  useUnixSocket  = 0;
  useAgentSocket = 1;

  agentFD[0] = fd[0];
  agentFD[1] = fd[1];


  nxinfo << "NXTransAgent: Internal descriptors for agent are FD#"
         << agentFD[0] << " and FD#" << agentFD[1] << ".\n"
         << std::flush;

  nxinfo << "NXTransAgent: Disabling listening for further X client "
         << "connections.\n" << std::flush;


  agent = new Agent(agentFD);

  if (agent == NULL || agent -> isValid() != 1)
  {
    nxfatal << "Loop: PANIC! Error creating the NX memory transport .\n"
            << std::flush;

    cerr << "Error" << ": Error creating the NX memory transport.\n";

    HandleCleanup();
  }

  nxinfo << "NXTransAgent: Enabling memory-to-memory transport.\n"
         << std::flush;

  return 1;
}

int NXTransClose(int fd)
{
  if (logofs == NULL)
  {
    logofs = &cerr;
  }

  /*
   * Only handle the proxy connection. The X
   * transport will take care of closing its
   * end of the socket pair.
   */

  if (control != NULL && ((agent != NULL &&
          (fd == agentFD[0] || fd == NX_FD_ANY)) ||
              (fd == proxyFD || fd == NX_FD_ANY)))
  {
    if (proxy != NULL)
    {
      nxinfo << "NXTransClose: Closing down all the X connections.\n"
             << std::flush;

      CleanupConnections();
    }
  }
  else
  {
    nxinfo << "NXTransClose: The NX transport is not running.\n"
           << std::flush;
  }

  return 1;
}

//
// Close down the transport and free the
// allocated NX resources.
//

int NXTransDestroy(int fd)
{
  if (logofs == NULL)
  {
    logofs = &cerr;
  }

  if (control != NULL && ((agent != NULL &&
          (fd == agentFD[0] || fd == NX_FD_ANY)) ||
              (fd == proxyFD || fd == NX_FD_ANY)))
  {
    //
    // Shut down the X connections and
    // wait the cleanup to complete.
    //

    if (proxy != NULL)
    {
      nxinfo << "NXTransDestroy: Closing down all the X connections.\n"
             << std::flush;

      CleanupConnections();
    }

    nxinfo << "NXTransDestroy: Waiting for the NX transport to terminate.\n"
           << std::flush;

    lastDestroy = 1;

    WaitCleanup();
  }
  else
  {
    nxinfo << "NXTransDestroy: The NX transport is not running.\n"
           << std::flush;
  }

  return 1;
}

//
// Assume that the NX transport is valid
// as long as the control class has not
// been destroyed.
//

int NXTransRunning(int fd)
{
  return (control != NULL);
}

//
// FIXME: why timeval? Passing milliseconds would be more convenient,
// the timeval struct/T_timestamp could be built on demand.
//
int NXTransContinue(struct timeval *selectTs)
{
  if (control != NULL)
  {
    //
    // If no timeout is provided use
    // the default.
    //

    T_timestamp newTs;

    if (selectTs == NULL)
    {
      setTimestamp(newTs, control -> PingTimeout);

      selectTs = &newTs;
    }

    //
    // Use empty masks and only get the
    // descriptors set by the proxy.
    //

    fd_set readSet;
    fd_set writeSet;

    int setFDs;
    int errorFDs;
    int resultFDs;

    setFDs = 0;

    FD_ZERO(&readSet);
    FD_ZERO(&writeSet);

    //
    // Run a new loop. If the transport
    // is gone avoid sleeping until the
    // timeout.
    //

    if (NXTransPrepare(&setFDs, &readSet, &writeSet, selectTs) != 0)
    {
      NXTransSelect(&resultFDs, &errorFDs, &setFDs, &readSet, &writeSet, selectTs);

      NXTransExecute(&resultFDs, &errorFDs, &setFDs, &readSet, &writeSet, selectTs);
    }
  }

  return (control != NULL);
}

int NXTransSignal(int signal, int action)
{
  if (logofs == NULL)
  {
    logofs = &cerr;
  }

  if (control == NULL)
  {
    return 0;
  }

  if (action == NX_SIGNAL_RAISE)
  {
    nxinfo << "NXTransSignal: Raising signal '" << DumpSignal(signal)
           << "' in the proxy handler.\n" << std::flush;

    HandleSignal(signal);

    return 1;
  }
  else if (signal == NX_SIGNAL_ANY)
  {
    nxinfo << "NXTransSignal: Setting action of all signals to '"
           << action << "'.\n" << std::flush;

    for (int i = 0; i < 32; i++)
    {
      if (CheckSignal(i) == 1)
      {
        NXTransSignal(i, action);
      }
    }

    return 1;
  }
  else if (CheckSignal(signal) == 1)
  {
    nxinfo << "NXTransSignal: Setting action of signal '"
           << DumpSignal(signal) << "' to '" << action
           << "'.\n" << std::flush;

    if (action == NX_SIGNAL_ENABLE ||
            action == NX_SIGNAL_FORWARD)
    {
      InstallSignal(signal, action);

      return 1;
    }
    else if (action == NX_SIGNAL_DISABLE)
    {
      RestoreSignal(signal);

      return 1;
    }
  }

  nxwarn << "NXTransSignal: WARNING! Unable to perform action '"
         << action << "' on signal '" << DumpSignal(signal)
         << "'.\n" << std::flush;

  cerr << "Warning" << ": Unable to perform action '" << action
       << "' on signal '" << DumpSignal(signal)
       << "'.\n";

  return -1;
}

int NXTransCongestion(int fd)
{
  if (control != NULL && proxy != NULL)
  {

    int congestion = proxy -> getCongestion(proxyFD);
    nxdbg << "NXTransCongestion: Returning " << congestion
          << " as current congestion level.\n" << std::flush;

    return congestion;

    return (proxy -> getCongestion(proxyFD));
  }

  return 0;
}

int NXTransHandler(int fd, int type, void (*handler)(void *parameter,
                       int reason), void *parameter)
{
  if (logofs == NULL)
  {
    logofs = &cerr;
  }

  switch (type)
  {
    case NX_HANDLER_FLUSH:
    {
      flushCallback  = handler;
      flushParameter = parameter;

      break;
    }
    case NX_HANDLER_STATISTICS:
    {
      //
      // Reporting of statistics by the agent
      // still needs to be implemented.
      //

      statisticsCallback  = handler;
      statisticsParameter = parameter;

      break;
    }
    default:
    {
      nxinfo << "NXTransHandler: WARNING! Failed to set "
             << "the NX callback for event '" << type << "' to '"
             << (void *) handler << "' and parameter '"
             << parameter << "'.\n" << std::flush;

      return 0;
    }
  }

  nxinfo << "NXTransHandler: Set the NX "
         << "callback for event '" << type << "' to '"
         << (void *) handler << "' and parameter '"
         << parameter << "'.\n" << std::flush;

  return 1;
}

int NXTransRead(int fd, char *data, int size)
{
  if (logofs == NULL)
  {
    logofs = &cerr;
  }

  if (control != NULL && agent != NULL &&
          fd == agentFD[0])
  {
    nxdbg << "NXTransRead: Dequeuing " << size << " bytes "
          << "from FD#" << agentFD[0] << ".\n" << std::flush;

    int result = agent -> dequeueData(data, size);


    if (result < 0 && EGET() == EAGAIN)
    {
      nxdbg << "NXTransRead: WARNING! Dequeuing from FD#"
            << agentFD[0] << " would block.\n" << std::flush;
    }
    else
    {
      nxdbg << "NXTransRead: Dequeued " << result << " bytes "
            << "to FD#" << agentFD[0] << ".\n" << std::flush;
    }


    return result;
  }
  else
  {
    nxdbg << "NXTransRead: Reading " << size << " bytes "
          << "from FD#" << fd << ".\n" << std::flush;

    return read(fd, data, size);
  }
}

int NXTransReadVector(int fd, struct iovec *iovdata, int iovsize)
{
  if (logofs == NULL)
  {
    logofs = &cerr;
  }

  if (control != NULL && agent != NULL &&
          fd == agentFD[0])
  {

    if (control -> ProxyStage >= stage_operational &&
            agent -> localReadable() > 0)
    {
      nxdbg << "NXTransReadVector: WARNING! Agent has data readable.\n"
            << std::flush;
    }


    char *base;

    int length;
    int result;

    struct iovec *vector = iovdata;
    int count = iovsize;

    ESET(0);

    int i = 0;
    int total = 0;

    for (;  i < count;  i++, vector++)
    {
      length = vector -> iov_len;
      base = (char *) vector -> iov_base;

      while (length > 0)
      {
        nxdbg << "NXTransReadVector: Dequeuing " << length
              << " bytes " << "from FD#" << agentFD[0] << ".\n"
              << std::flush;

        result = agent -> dequeueData(base, length);


        if (result < 0 && EGET() == EAGAIN)
        {
          nxdbg << "NXTransReadVector: WARNING! Dequeuing from FD#"
                << agentFD[0] << " would block.\n" << std::flush;
        }
        else
        {
          nxdbg << "NXTransReadVector: Dequeued " << result
                << " bytes " << "from FD#" << agentFD[0] << ".\n"
                << std::flush;
        }


        if (result < 0 && total == 0)
        {
          return result;
        }
        else if (result <= 0)
        {
          return total;
        }

        ESET(0);

        length -= result;
        total  += result;
        base   += result;
      }
    }

    return total;
  }
  else
  {
    nxdbg << "NXTransReadVector: Reading vector with "
          << iovsize << " elements from FD#" << fd << ".\n"
          << std::flush;

    return readv(fd, iovdata, iovsize);
  }
}

int NXTransReadable(int fd, int *readable)
{
  if (logofs == NULL)
  {
    logofs = &cerr;
  }

  if (control == NULL || agent == NULL ||
          fd != agentFD[0])
  {

    int result = GetBytesReadable(fd, readable);
    if (result == -1)
    {
      nxdbg << "NXTransReadable: Error detected on FD#"
            << fd << ".\n" << std::flush;
    }
    else
    {
      nxdbg << "NXTransReadable: Returning " << *readable
            << " bytes as readable from FD#" << fd
            << ".\n" << std::flush;
    }

    return result;
  }

  int result = agent -> dequeuableData();

  switch (result)
  {
    case 0:
    {
      //
      // The client might have enqueued data to our side
      // and is now checking for the available events. As
      // _XEventsQueued() may omit to call _XSelect(), we
      // handle here the new data that is coming from the
      // proxy to avoid spinning through this function
      // again.
      //

      if (proxy != NULL && proxy -> canRead() == 1)
      {
        nxinfo << "NXTransReadable: WARNING! Trying to "
               << "read to generate new agent data.\n"
               << std::flush;

        //
        // Set the context as the function
        // can cause a cleanup.
        //

        if (setjmp(context) == 1)
        {
          return -1;
        }

        if (proxy -> handleRead() < 0)
        {
          nxinfo << "NXTransReadable: Failure reading "
                 << "messages from proxy FD#" << proxyFD
                 << ".\n" << std::flush;

          HandleShutdown();
        }

        //
        // Call again the routine. By reading
        // new control messages from the proxy
        // the agent channel may be gone.
        //

        return NXTransReadable(fd, readable);
      }

      nxdbg << "NXTransReadable: Returning " << 0
            << " bytes as readable from FD#" << fd
            << " with result 0.\n" << std::flush;

      *readable = 0;

      return 0;
    }
    case -1:
    {
      nxdbg << "NXTransReadable: Returning " << 0
            << " bytes as readable from FD#" << fd
            << " with result -1.\n" << std::flush;

      *readable = 0;

      return -1;
    }
    default:
    {
      nxdbg << "NXTransReadable: Returning " << result
            << " bytes as readable from FD#" << fd
            << " with result 0.\n" << std::flush;

      *readable = result;

      return 0;
    }
  }
}

int NXTransWrite(int fd, char *data, int size)
{
  //
  // Be sure we have a valid log file.
  //

  if (logofs == NULL)
  {
    logofs = &cerr;
  }

  if (control != NULL && agent != NULL &&
          fd == agentFD[0])
  {
    int result;

    if (proxy != NULL)
    {
      if (proxy -> canRead(agentFD[1]) == 0)
      {
        nxdbg << "NXTransWrite: WARNING! Delayed enqueuing to FD#"
              << agentFD[0] << " with proxy unable to read.\n"
              << std::flush;

        ESET(EAGAIN);

        return -1;
      }

      //
      // Set the context as the function
      // can cause a cleanup.
      //

      if (setjmp(context) == 1)
      {
        return -1;
      }

      //
      // Don't enqueue the data to the transport
      // but let the channel borrow the buffer.
      //

      nxdbg << "NXTransWrite: Letting the channel borrow "
            << size << " bytes from FD#" << agentFD[0]
            << ".\n" << std::flush;

      result = proxy -> handleRead(agentFD[1], data, size);

      if (result == 1)
      {
        result = size;
      }
      else
      {
        if (result == 0)
        {
          ESET(EAGAIN);
        }
        else
        {
          ESET(EPIPE);
        }

        result = -1;
      }
    }
    else
    {
      //
      // We don't have a proxy connection, yet.
      // Enqueue the data to the agent transport.
      //

      nxdbg << "NXTransWrite: Enqueuing " << size << " bytes "
            << "to FD#" << agentFD[0] << ".\n" << std::flush;

      result = agent -> enqueueData(data, size);
    }


    if (result < 0)
    {
      if (EGET() == EAGAIN)
      {
        nxdbg << "NXTransWrite: WARNING! Enqueuing to FD#"
              << agentFD[0] << " would block.\n"
              << std::flush;
      }
      else
      {
        nxdbg << "NXTransWrite: WARNING! Error enqueuing to FD#"
              << agentFD[0] << ".\n" << std::flush;
      }
    }
    else
    {
      nxdbg << "NXTransWrite: Enqueued " << result << " bytes "
            << "to FD#" << agentFD[0] << ".\n" << std::flush;
    }


    return result;
  }
  else
  {
    nxdbg << "NXTransWrite: Writing " << size << " bytes "
          << "to FD#" << fd << ".\n" << std::flush;

    return write(fd, data, size);
  }
}

int NXTransWriteVector(int fd, struct iovec *iovdata, int iovsize)
{
  //
  // Be sure we have a valid log file and a
  // jump context because we will later call
  // functions that can perform a cleanup.
  //

  if (logofs == NULL)
  {
    logofs = &cerr;
  }

  int result = 0;

  if (control != NULL && agent != NULL &&
          fd == agentFD[0])
  {
    //
    // See the comment in NXTransWrite().
    //

    if (proxy != NULL)
    {
      if (proxy -> canRead(agentFD[1]) == 0)
      {
        nxdbg << "NXTransWriteVector: WARNING! Delayed enqueuing to FD#"
              << agentFD[0] << " with proxy unable to read.\n"
              << std::flush;

        ESET(EAGAIN);

        return -1;
      }
    }

    //
    // Set the context as the function
    // can cause a cleanup.
    //

    if (setjmp(context) == 1)
    {
      return -1;
    }

    char *base;

    int length;

    struct iovec *vector = iovdata;
    int count = iovsize;

    ESET(0);

    int i = 0;
    int total = 0;

    for (;  i < count;  i++, vector++)
    {
      length = vector -> iov_len;
      base = (char *) vector -> iov_base;

      while (length > 0)
      {
        if (proxy != NULL)
        {
          //
          // Don't enqueue the data to the transport
          // but let the channel borrow the buffer.
          //

          nxdbg << "NXTransWriteVector: Letting the channel borrow "
                << length << " bytes from FD#" << agentFD[0]
                << ".\n" << std::flush;

          result = proxy -> handleRead(agentFD[1], base, length);

          if (result == 1)
          {
            result = length;
          }
          else
          {
            if (result == 0)
            {
              ESET(EAGAIN);
            }
            else
            {
              ESET(EPIPE);
            }

            result = -1;
          }
        }
        else
        {
          //
          // We don't have a proxy connection, yet.
          // Enqueue the data to the agent transport.
          //

          nxdbg << "NXTransWriteVector: Enqueuing " << length
                << " bytes " << "to FD#" << agentFD[0] << ".\n"
                << std::flush;

          result = agent -> enqueueData(base, length);
        }


        if (result < 0)
        {
          if (EGET() == EAGAIN)
          {
            nxdbg << "NXTransWriteVector: WARNING! Enqueuing to FD#"
                  << agentFD[0] << " would block.\n"
                  << std::flush;
          }
          else
          {
            nxdbg << "NXTransWriteVector: WARNING! Error enqueuing to FD#"
                  << agentFD[0] << ".\n" << std::flush;
          }
        }
        else
        {
          nxdbg << "NXTransWriteVector: Enqueued " << result
                << " bytes " << "to FD#" << agentFD[0] << ".\n"
                << std::flush;
        }


        if (result < 0 && total == 0)
        {
          return result;
        }
        else if (result <= 0)
        {
          return total;
        }

        ESET(0);

        length -= result;
        total  += result;
        base   += result;
      }
    }

    return total;
  }
  else
  {
    nxdbg << "NXTransWriteVector: Writing vector with "
          << iovsize << " elements to FD#" << fd << ".\n"
          << std::flush;

    return writev(fd, iovdata, iovsize);
  }
}

int NXTransPolicy(int fd, int type)
{
  if (control != NULL)
  {
    if (usePolicy == -1)
    {
      nxinfo << "NXTransPolicy: Setting flush policy on "
             << "proxy FD#" << proxyFD << " to '"
             << DumpPolicy(type == NX_POLICY_DEFERRED ?
                           policy_deferred : policy_immediate)
             << "'.\n" << std::flush;

      control -> FlushPolicy = (type == NX_POLICY_DEFERRED ?
                                    policy_deferred : policy_immediate);

      if (proxy != NULL)
      {
        proxy -> handleFlush();
      }

      return 1;
    }
    else
    {
      nxinfo << "NXTransPolicy: Ignoring the agent "
             << "setting with user policy set to '"
             << DumpPolicy(control -> FlushPolicy)
             << "'.\n" << std::flush;

      return 0;
    }
  }

  return 0;
}

int NXTransFlushable(int fd)
{
  if (proxy == NULL || agent == NULL ||
          fd != agentFD[0])
  {
    nxdbg << "NXTransFlushable: Returning 0 bytes as "
          << "flushable for unrecognized FD#" << fd
          << ".\n" << std::flush;

    return 0;
  }
  else
  {
    nxdbg << "NXTransFlushable: Returning " << proxy ->
             getFlushable(proxyFD) << " as bytes flushable on "
          << "proxy FD#" << proxyFD << ".\n"
          << std::flush;

    return proxy -> getFlushable(proxyFD);
  }
}

int NXTransFlush(int fd)
{
  if (proxy != NULL)
  {
    nxinfo << "NXTransFlush: Requesting an immediate flush of "
           << "proxy FD#" << proxyFD << ".\n"
           << std::flush;

    return proxy -> handleFlush();
  }

  return 0;
}

int NXTransChannel(int fd, int channelFd, int type)
{
  if (proxy != NULL)
  {
    //
    // Set the context as the function
    // can cause a cleanup.
    //

    if (setjmp(context) == 1)
    {
      return -1;
    }

    nxinfo << "NXTransChannel: Going to create a new channel "
           << "with type '" << type << "' on FD#" << channelFd
           << ".\n" << std::flush;

    int result = -1;

    switch (type)
    {
      case NX_CHANNEL_X11:
      {
        if (useUnixSocket == 1 || useTcpSocket == 1 ||
                useAgentSocket == 1 || useAuxSocket == 1)
        {
          result = proxy -> handleNewConnection(channel_x11, channelFd);
        }

        break;
      }
      case NX_CHANNEL_CUPS:
      {
        if (useCupsSocket == 1)
        {
          result = proxy -> handleNewConnection(channel_cups, channelFd);
        }

        break;
      }
      case NX_CHANNEL_SMB:
      {
        if (useSmbSocket == 1)
        {
          result = proxy -> handleNewConnection(channel_smb, channelFd);
        }

        break;
      }
      case NX_CHANNEL_MEDIA:
      {
        if (useMediaSocket == 1)
        {
          result = proxy -> handleNewConnection(channel_media, channelFd);
        }

        break;
      }
      case NX_CHANNEL_HTTP:
      {
        if (useHttpSocket == 1)
        {
          result = proxy -> handleNewConnection(channel_http, channelFd);
        }

        break;
      }
      case NX_CHANNEL_FONT:
      {
        if (useFontSocket == 1)
        {
          result = proxy -> handleNewConnection(channel_font, channelFd);
        }

        break;
      }
      case NX_CHANNEL_SLAVE:
      {
        if (useSlaveSocket == 1)
        {
          result = proxy -> handleNewConnection(channel_slave, channelFd);
        }

        break;
      }
      default:
      {
        nxwarn << "NXTransChannel: WARNING! Unrecognized channel "
               << "type '" << type << "'.\n" << std::flush;

        break;
      }
    }


    if (result != 1)
    {
      nxwarn << "NXTransChannel: WARNING! Could not create the "
             << "new channel with type '" << type << "' on FD#"
             << channelFd << ".\n" << std::flush;
    }


    return result;
  }

  return 0;
}

const char *NXTransFile(int type)
{
  char *name = NULL;

  switch (type)
  {
    case NX_FILE_SESSION:
    {
      name = sessionFileName;

      break;
    }
    case NX_FILE_ERRORS:
    {
      name = errorsFileName;

      break;
    }
    case NX_FILE_OPTIONS:
    {
      name = optionsFileName;

      break;
    }
    case NX_FILE_STATS:
    {
      name = statsFileName;

      break;
    }
  }

  if (name != NULL && *name != '\0')
  {
    return name;
  }

  return NULL;
}

long NXTransTime()
{
  static T_timestamp last = getTimestamp();

  T_timestamp now = getTimestamp();

  long diff = diffTimestamp(last, now);

  last = now;

  return diff;
}

int NXTransAlert(int code, int local)
{
  if (proxy != NULL)
  {
    nxdbg << "NXTransAlert: Requesting a NX dialog with code "
          << code << " and local " << local << ".\n"
          << std::flush;

    if (local == 0)
    {
      //
      // Set the context as the function
      // can cause a cleanup.
      //

      if (setjmp(context) == 1)
      {
        return -1;
      }

      proxy -> handleAlert(code);
    }
    else
    {
      //
      // Show the alert at the next loop.
      //

      HandleAlert(code, local);
    }

    return 1;
  }
  else
  {
    if (logofs == NULL)
    {
      logofs = &cerr;
    }

    nxdbg << "NXTransAlert: Can't request an alert without "
          << "a valid NX transport.\n" << std::flush;
  }

  return 0;
}

//
// Prepare the file sets and the timeout
// for a later execution of the select().
//

int NXTransPrepare(int *setFDs, fd_set *readSet,
                       fd_set *writeSet, struct timeval *selectTs)
{
  if (logofs == NULL)
  {
    logofs = &cerr;
  }

  //
  // Control is NULL if the NX transport was
  // reset or was never created. If control
  // is valid then prepare to jump back when
  // the transport is destroyed.
  //

  if (control == NULL || setjmp(context) == 1)
  {
    return 0;
  }

  nxinfo << "NXTransPrepare: Going to prepare the NX transport.\n"
         << std::flush;

  if (control -> ProxyStage < stage_operational)
  {
    handleNegotiationInLoop(*setFDs, *readSet, *writeSet, *selectTs);
  }
  else
  {

    if (isTimestamp(*selectTs) == 0)
    {
      nxinfo << "Loop: WARNING! Preparing the select with requested "
             << "timeout of " << selectTs -> tv_sec << " s and "
             << (double) selectTs -> tv_usec / 1000 << " ms.\n"
             << std::flush;
    }
    else
    {
      nxinfo << "Loop: Preparing the select with requested "
             << "timeout of " << selectTs -> tv_sec << " s and "
             << (double) selectTs -> tv_usec / 1000 << " ms.\n"
             << std::flush;
    }


    //
    // Set descriptors of listening sockets.
    //

    handleSetListenersInLoop(*readSet, *setFDs);

    //
    // Set descriptors of both proxy and X
    // connections.
    //

    handleSetReadInLoop(*readSet, *setFDs, *selectTs);

    //
    // Find out which file descriptors have
    // data to write.
    //

    handleSetWriteInLoop(*writeSet, *setFDs, *selectTs);
  }

  //
  // Prepare the masks for handling the memory-
  // to-memory transport. This is required even
  // during session negotiation.
  //

  if (agent != NULL)
  {
    handleSetAgentInLoop(*setFDs, *readSet, *writeSet, *selectTs);
  }

  //
  // Register time spent handling messages.
  //

  nowTs = getNewTimestamp();

  int diffTs = diffTimestamp(startTs, nowTs);

  nxinfo << "Loop: Mark - 0 - at " << strMsTimestamp()
         << " with " << diffTs << " ms elapsed.\n"
         << std::flush;

  //
  // TODO: Should add the read time in two
  // parts otherwise the limits are checked
  // before the counters are updated with
  // time spent in the last loop.
  //

  if (control -> ProxyStage >= stage_operational)
  {
    statistics -> addReadTime(diffTs);
  }

  startTs = nowTs;

  nxdbg << "Loop: New timestamp is " << strMsTimestamp(startTs)
        << ".\n" << std::flush;

  return 1;
}

//
// Let's say that we call select() to find out
// if any of the handled descriptors has data,
// but actually things are a bit more complex
// than that.
//

int NXTransSelect(int *resultFDs, int *errorFDs, int *setFDs, fd_set *readSet,
                      fd_set *writeSet, struct timeval *selectTs)
{
  int diffTs;

  static T_timestamp lastTs;

  if (logofs == NULL)
  {
    logofs = &cerr;
  }

  //
  // Control is NULL if the NX transport was
  // reset or never created. If control is
  // valid then prepare for jumping back in
  // the case of an error.
  //

  if (control == NULL || setjmp(context) == 1)
  {
    *resultFDs = select(*setFDs, readSet, writeSet, NULL, selectTs);

    *errorFDs = errno;

    return 0;
  }

  nxinfo << "NXTransSelect: Going to select the NX descriptors.\n"
         << std::flush;


  handleCheckSelectInLoop(*setFDs, *readSet, *writeSet, *selectTs);


  diffTs = diffTimestamp(lastTs, getNewTimestamp());
  if (diffTs > 20)
  {
    nxdbg << "Loop: TIME! Spent " << diffTs
          << " ms handling messages for proxy FD#"
          << proxyFD << ".\n" << std::flush;
  }

  lastTs = getNewTimestamp();


  if (isTimestamp(*selectTs) == 0)
  {
    nxinfo << "Loop: WARNING! Executing the select with requested "
           << "timeout of " << selectTs -> tv_sec << " s and "
           << (double) selectTs -> tv_usec / 1000 << " ms.\n"
           << std::flush;
  }
  else if (proxy != NULL && proxy -> getFlushable(proxyFD) > 0)
  {
    nxinfo << "Loop: WARNING! Proxy FD#" << proxyFD
           << " has " << proxy -> getFlushable(proxyFD)
           << " bytes to write but timeout is "
           << selectTs -> tv_sec << " s and "
           << selectTs -> tv_usec / 1000 << " ms.\n"
           << std::flush;
  }


  //
  // Wait for the selected sockets
  // or the timeout.
  //

  ESET(0);

  *resultFDs = select(*setFDs, readSet, writeSet, NULL, selectTs);

  *errorFDs = EGET();


  diffTs = diffTimestamp(lastTs, getNewTimestamp());
  if (diffTs > 100)
  {
    nxdbg << "Loop: TIME! Spent " << diffTs
          << " ms waiting for new data for proxy FD#"
          << proxyFD << ".\n" << std::flush;
  }

  lastTs = getNewTimestamp();

  //
  // Check the result of the select.
  //


  handleCheckResultInLoop(*resultFDs, *errorFDs, *setFDs, *readSet, *writeSet, *selectTs, startTs);

  //
  // Get time spent in select. The accounting is done
  // in milliseconds. This is a real problem on fast
  // machines where each loop is unlikely to take
  // more than 500 us, so consider that the results
  // can be inaccurate.
  //

  nowTs = getNewTimestamp();

  diffTs = diffTimestamp(startTs, nowTs);

  nxinfo << "Loop: Out of select after " << diffTs << " ms "
         << "at " << strMsTimestamp(nowTs) << " with result "
         << *resultFDs << ".\n" << std::flush;

  startTs = nowTs;

  nxdbg << "Loop: New timestamp is " << strMsTimestamp(startTs)
        << ".\n" << std::flush;

  if (control -> ProxyStage >= stage_operational)
  {
    statistics -> addIdleTime(diffTs);
  }

  if (*resultFDs < 0)
  {
    //
    // Check if the call was interrupted or if any of the
    // managed descriptors has become invalid. This can
    // happen to the X11 code, before the descriptor is
    // removed from the managed set.
    //

    #ifdef __sun

    if (*errorFDs == EINTR || *errorFDs == EBADF ||
            *errorFDs == EINVAL)

    #else

    if (*errorFDs == EINTR || *errorFDs == EBADF)

    #endif

    {

      if (*errorFDs == EINTR)
      {
        nxinfo << "Loop: Select failed due to EINTR error.\n"
               << std::flush;
      }
      else
      {
        nxinfo << "Loop: WARNING! Call to select failed. Error is "
               << EGET() << " '" << ESTR() << "'.\n"
               << std::flush;
      }

    }
    else
    {
      nxfatal << "Loop: PANIC! Call to select failed. Error is "
              << EGET() << " '" << ESTR() << "'.\n"
              << std::flush;

      cerr << "Error" << ": Call to select failed. Error is "
           << EGET() << " '" << ESTR() << "'.\n";

      HandleCleanup();
    }
  }

  return 1;
}

//
// Perform the required actions on all
// the descriptors having I/O pending.
//

int NXTransExecute(int *resultFDs, int *errorFDs, int *setFDs, fd_set *readSet,
                       fd_set *writeSet, struct timeval *selectTs)
{
  if (logofs == NULL)
  {
    logofs = &cerr;
  }

  //
  // Control is NULL if the NX transport was
  // reset or never created. If control is
  // valid then prepare for jumping back in
  // the case of an error.
  //

  if (control == NULL || setjmp(context) == 1)
  {
    return 0;
  }

  nxinfo << "NXTransExecute: Going to execute I/O on the NX descriptors.\n"
         << std::flush;

  if (control -> ProxyStage >= stage_operational)
  {
    //
    // Check if I/O is possible on the proxy and
    // local agent descriptors.
    //

    if (agent != NULL)
    {
      handleAgentInLoop(*resultFDs, *errorFDs, *setFDs, *readSet, *writeSet, *selectTs);
    }

    nxinfo << "Loop: Mark - 1 - at " << strMsTimestamp()
           << " with " << diffTimestamp(startTs, getTimestamp())
           << " ms elapsed.\n" << std::flush;

    //
    // Rotate the channel that will be handled
    // first.
    //

    handleRotateInLoop();

    //
    // Flush any data on newly writable sockets.
    //

    handleWritableInLoop(*resultFDs, *writeSet);

    nxinfo << "Loop: Mark - 2 - at " << strMsTimestamp()
           << " with " << diffTimestamp(startTs, getTimestamp())
           << " ms elapsed.\n" << std::flush;

    //
    // Check if any socket has become readable.
    //

    handleReadableInLoop(*resultFDs, *readSet);

    nxinfo << "Loop: Mark - 3 - at " << strMsTimestamp()
           << " with " << diffTimestamp(startTs, getTimestamp())
           << " ms elapsed.\n" << std::flush;

    //
    // Handle the scheduled events on channels.
    //
    // - Restart, if possible, any client that was
    //   put to sleep.
    //
    // - Check if there are pointer motion events to
    //   flush. This applies only to X server side.
    //
    // - Check if any channel has exited the conges-
    //   tion state.
    //
    // - Check if there are images that are currently
    //   being streamed.
    //

    handleEventsInLoop();

    nxinfo << "Loop: Mark - 4 - at " << strMsTimestamp()
           << " with " << diffTimestamp(startTs, getTimestamp())
           << " ms elapsed.\n" << std::flush;

    //
    // Check if user sent a signal to produce
    // statistics.
    //

    handleStatisticsInLoop();

    //
    // We may have flushed the proxy link or
    // handled data coming from the remote.
    // Post-process the masks and set the
    // selected agent descriptors as ready.
    //

    if (agent != NULL)
    {
      handleAgentLateInLoop(*resultFDs, *errorFDs, *setFDs, *readSet, *writeSet, *selectTs);
    }

    nxinfo << "Loop: Mark - 5 - at " << strMsTimestamp()
           << " with " << diffTimestamp(startTs, getTimestamp())
           << " ms elapsed.\n" << std::flush;

    //
    // Check if there is any data to flush.
    // Agents should flush the proxy link
    // explicitly.
    //

    handleFlushInLoop();

    nxinfo << "Loop: Mark - 6 - at " << strMsTimestamp()
           << " with " << diffTimestamp(startTs, getTimestamp())
           << " ms elapsed.\n" << std::flush;
  }

  //
  // Check if we have an alert to show.
  //

  handleAlertInLoop();

  if (control -> ProxyStage >= stage_operational)
  {
    //
    // Check if it's time to give up.
    //

    handleCheckSessionInLoop();

    //
    // Check if local proxy is consuming
    // too many resources.
    //

    handleCheckBitrateInLoop();

    //
    // Check coherency of internal state.
    //


    handleCheckStateInLoop(*setFDs);

    nxinfo << "Loop: Mark - 7 - at " << strMsTimestamp()
           << " with " << diffTimestamp(startTs, getTimestamp())
           << " ms elapsed.\n" << std::flush;
  }

  //
  // Truncate the logs if needed.
  //

  handleLogReopenInLoop(logsTs, nowTs);

  return 1;
}

//
// Initialize the connection parameters and
// prepare for negotiating the link with the
// remote proxy.
//

int InitBeforeNegotiation()
{
  //
  // Disable limits on core dumps.
  //

  SetCore();

  //
  // Install the signal handlers.
  //

  InstallSignals();

  //
  // Track how much time we spent in initialization.
  //

  nowTs = getNewTimestamp();

  startTs = nowTs;
  initTs  = nowTs;

  nxinfo << "Loop: INIT! Taking mark for initialization at "
         << strMsTimestamp(initTs) << ".\n"
         << std::flush;

  //
  // If not explicitly specified, determine if local
  // mode is client or server according to parameters
  // provided so far.
  //

  if (WE_SET_PROXY_MODE == 0)
  {
    cerr << "Error" << ": Please specify either the -C or -S option.\n";

    HandleCleanup();
  }

  //
  // Start a watchdog. If initialization cannot
  // be completed before timeout, then clean up
  // everything and exit.
  //

  if (control -> ProxyMode == proxy_client)
  {
    nxinfo << "Loop: Starting watchdog process with timeout of "
           << control -> InitTimeout / 1000 << " seconds.\n"
           << std::flush;

    lastWatchdog = NXTransWatchdog(control -> InitTimeout);

    if (IsFailed(lastWatchdog))
    {
      nxfatal << "Loop: PANIC! Can't start the NX watchdog process.\n"
              << std::flush;

      SetNotRunning(lastWatchdog);
    }
    else
    {
      nxinfo << "Loop: Watchdog started with pid '"
             << lastWatchdog << "'.\n" << std::flush;
    }
  }

  //
  // Print preliminary info.
  //

  PrintProcessInfo();

  //
  // Set cups, multimedia and other
  // auxiliary ports.
  //

  SetPorts();

  //
  // Increase the number of maximum open
  // file descriptors for this process.
  //

  SetDescriptors();

  //
  // Set local endianness.
  //

  unsigned int test = 1;

  setHostBigEndian(*((unsigned char *) (&test)) == 0);

  nxinfo << "Loop: Local host is "
         << (hostBigEndian() ? "big endian" : "little endian")
         << ".\n" << std::flush;

  if (control -> ProxyMode == proxy_client)
  {
    //
    // Listen on sockets that mimic an X display to
    // which X clients will be able to connect (e.g.
    // unix:8 and/or localhost:8).
    //

    if (useTcpSocket == 1)
    {
      SetupTcpSocket();
    }

    if (useUnixSocket == 1)
    {
      SetupUnixSocket();
    }
  }
  else
  {
    //
    // Don't listen for X connections.
    //

    useUnixSocket  = 0;
    useTcpSocket   = 0;
    useAgentSocket = 0;

    //
    // Get ready to open the local display.
    //

    delete xServerAddr;
    xServerAddr = NULL;

    SetupDisplaySocket(xServerAddrFamily, xServerAddr, xServerAddrLength);
  }

  //
  // If we are the NX server-side proxy we need to
  // complete our initializazion. We will mandate
  // our parameters at the time the NX client will
  // connect.
  //

  if (control -> ProxyMode == proxy_client)
  {
    SetParameters();
  }

  return 1;
}

int SetupProxyConnection()
{

  if (proxyFD == -1)
  {

    char *socketUri = NULL;

    // Let's make sure, the default value for listenSocket is properly set. Doing this
    // here, because we have to make sure that we call it after the connectSocket
    // declaration is really really complete.

    if (listenSocket.disabled() && connectSocket.disabled())
    {
      char listenPortValue[20] = { 0 };
      sprintf(listenPortValue, "%ld", (long)(proxyPort + DEFAULT_NX_PROXY_PORT_OFFSET));

      SetAndValidateChannelEndPointArg("local", "listen", listenPortValue, listenSocket);
    }

    connectSocket.getSpec(&socketUri);
    nxinfo << "Loop: connectSocket is "<< ( connectSocket.enabled() ? "enabled" : "disabled") << ". "
           << "The socket URI is '"<< ( socketUri != NULL ? socketUri : "<unset>") << "'.\n" << std::flush;

    SAFE_FREE(socketUri);

    listenSocket.getSpec(&socketUri);
    nxinfo << "Loop: listenSocket is "<< ( listenSocket.enabled() ? "enabled" : "disabled") << ". "
           << "The socket URI is '"<< ( socketUri != NULL ? socketUri : "<unset>") << "'.\n" << std::flush;

    SAFE_FREE(socketUri);

    if (WE_INITIATE_CONNECTION)
    {
      if (connectSocket.getSpec(&socketUri))
      {
        nxinfo << "Loop: Going to connect to '" << socketUri
               << "'.\n" << std::flush;
        SAFE_FREE(socketUri);

        proxyFD = ConnectToRemote(connectSocket);

        nxinfo << "Loop: Connected to remote proxy on FD#"
               << proxyFD << ".\n" << std::flush;

        cerr << "Info" << ": Connected to remote proxy on FD#"
             << proxyFD << ".\n";
      }
    }
    else
    {

      if (listenSocket.isTCPSocket() && (listenSocket.getTCPPort() < 0))
      {
        listenSocket.setSpec(DEFAULT_NX_PROXY_PORT_OFFSET + proxyPort);
      }

      if (listenSocket.getSpec(&socketUri))
      {
        nxinfo << "Loop: Going to wait for connection at '"
               << socketUri << "'.\n" << std::flush;
        SAFE_FREE(socketUri);

        proxyFD = WaitForRemote(listenSocket);

        if (WE_LISTEN_FORWARDER)
        {
          nxinfo << "Loop: Connected to remote forwarder on FD#"
                 << proxyFD << ".\n" << std::flush;
        }
        else
        {
          nxinfo << "Loop: Connected to remote proxy on FD#"
                 << proxyFD << ".\n" << std::flush;
        }

      }
    }
  }
  else
  {
    nxinfo << "Loop: Using the inherited connection on FD#"
           << proxyFD << ".\n" << std::flush;
  }

  //
  // Set TCP_NODELAY on proxy descriptor
  // to reduce startup time. Option will
  // later be disabled if needed.
  //
  // either listenSocket or connectSocket is used here...

  if(listenSocket.isTCPSocket() || connectSocket.isTCPSocket())

    SetNoDelay(proxyFD, 1);

  //
  // We need non-blocking input since the
  // negotiation phase.
  //

  SetNonBlocking(proxyFD, 1);

  return 1;
}

//
// Create the required proxy and channel classes
// and get ready for handling the encoded traffic.
//

int InitAfterNegotiation()
{
  nxinfo << "Loop: Connection with remote proxy completed.\n"
         << std::flush;

  cerr << "Info" << ": Connection with remote proxy completed.\n"
        << logofs_flush;

  //
  // If we are the server proxy we completed
  // our initializazion phase according to
  // the values provided by the client side.
  //

  if (control -> ProxyMode == proxy_server)
  {
    SetParameters();
  }

  //
  // Set up the listeners for the additional
  // services.
  //

  SetupServiceSockets();

  //
  // Create the proxy class and the statistics
  // repository and pass all the configuration
  // data we negotiated with the remote peer.
  //

  SetupProxyInstance();

  //
  // We completed both parsing of user's parameters
  // and handlshaking with remote proxy. Now print
  // a brief summary including the most significant
  // control values.
  //

  PrintConnectionInfo();

  //
  // Cancel the initialization watchdog.
  //

  if (IsRunning(lastWatchdog))
  {
    KillProcess(lastWatchdog, "watchdog", SIGTERM, 1);

    SetNotRunning(lastWatchdog);

    lastSignal = 0;
  }

  //
  // Start the house-keeper process. It will
  // remove the oldest persistent caches, if
  // the amount of storage exceed the limits
  // set by the user.
  //

  StartKeeper();

  //
  // Set the log size check timestamp.
  //

  nowTs = getNewTimestamp();

  logsTs = nowTs;

  //
  // TODO: Due to the way the new NX transport is working,
  // the accounting of time spent handling messages must
  // be rewritten. In particular, at the moment it only
  // shows the time spent encoding and decoding messages
  // in the main loop, after executing a select. It doesn't
  // take into account the time spent in the NXTrans* calls
  // where messages can be encoded and decoded implicitly,
  // on demand of the agent. When the agent transport is
  // in use, these calls constitute the vast majority of
  // the encoding activity. The result is that the number
  // of KB encoded per second shown by the proxy statistics
  // is actually much lower than the real throughput gene-
  // rated by the proxy.
  //

  nxinfo << "Loop: INIT! Completed initialization at "
         << strMsTimestamp(nowTs) << " with "
         << diffTimestamp(initTs, nowTs) << " ms "
         << "since the init mark.\n" << std::flush;

  initTs = getNewTimestamp();

  //
  // We can now start handling binary data from
  // our peer proxy.
  //

  if (agent == NULL)
  {
    cerr << "Session" << ": Session started at '"
         << strTimestamp() << "'.\n";
  }

  return 1;
}

int SetMode(int mode)
{
  //
  // Set the local proxy mode.
  //

  if (control -> ProxyMode == proxy_undefined)
  {
    if (mode == NX_MODE_CLIENT)
    {
      nxinfo << "Loop: INIT! Initializing with mode "
             << "NX_MODE_CLIENT at " << strMsTimestamp()
             << ".\n" << std::flush;

      control -> ProxyMode = proxy_client;
    }
    else if (mode == NX_MODE_SERVER)
    {
      nxinfo << "Loop: INIT! Initializing with mode "
             << "NX_MODE_SERVER at " << strMsTimestamp()
             << ".\n" << std::flush;

      control -> ProxyMode = proxy_server;
    }
    else
    {
      cerr << "Error" << ": Please specify either "
           << "the -C or -S option.\n";

      HandleCleanup();
    }
  }

  return 1;
}

int SetupProxyInstance()
{
  if (control -> ProxyMode == proxy_client)
  {
    proxy = new ClientProxy(proxyFD);
  }
  else
  {
    proxy = new ServerProxy(proxyFD);
  }

  if (proxy == NULL)
  {
    nxfatal << "Loop: PANIC! Error creating the NX proxy.\n"
            << std::flush;

    cerr << "Error" << ": Error creating the NX proxy.\n";

    HandleCleanup();
  }

  //
  // Create the statistics repository.
  //

  statistics = new Statistics(proxy);

  if (statistics == NULL)
  {
    nxfatal << "Loop: PANIC! Error creating the NX statistics.\n"
            << std::flush;

    cerr << "Error" << ": Error creating the NX statistics.\n";

    HandleCleanup();
  }

  //
  // If user gave us a proxy cookie than create the
  // X11 authorization repository and find the real
  // cookie to be used for our X display.
  //

  SetupAuthInstance();

  //
  // Reset the static members in channels.
  //

  proxy -> handleChannelConfiguration();

  //
  // Inform the proxies about the ports where they
  // will have to forward the network connections.
  //

  proxy -> handleDisplayConfiguration(displayHost, xServerAddrFamily,
                                          xServerAddr, xServerAddrLength);

  proxy -> handlePortConfiguration(cupsPort, smbPort, mediaPort,
                                       httpPort, fontPort);

  //
  // We handed over the sockaddr structure we
  // created when we set up the display socket
  // to the proxy.
  //

  xServerAddr = NULL;

  //
  // Set socket options on proxy link, then propagate link
  // configuration to proxy. This includes translating some
  // control parameters in 'local' and 'remote'. Finally
  // adjust cache parameters according to pack method and
  // session type selected by user.
  //

  if (proxy -> handleSocketConfiguration() < 0 ||
          proxy -> handleLinkConfiguration() < 0 ||
              proxy -> handleCacheConfiguration() < 0)
  {
    nxfatal << "Loop: PANIC! Error configuring the NX transport.\n"
            << std::flush;

    cerr << "Error" << ": Error configuring the NX transport.\n";

    HandleCleanup();
  }

  //
  // Load the message stores from the persistent
  // cache.
  //

  proxy -> handleLoad(load_if_first);

  //
  // Inform the proxy that from now on it can
  // start handling the encoded data.
  //

  proxy -> setOperational();

  //
  // Are we going to use an internal IPC connection
  // with an agent? In this case create the channel
  // by using the socket descriptor provided by the
  // caller at the proxy initialization.
  //

  SetupAgentInstance();

  //
  // Check if we need to verify the existence of
  // a matching client cache at shutdown.
  //

  #ifdef MATCH

  control -> PersistentCacheCheckOnShutdown = 1;

  #endif

  //
  // Flush any data produced so far.
  //

  proxy -> handleFlush();


  if (proxy -> getFlushable(proxyFD) > 0)
  {
    nxinfo << "Loop: WARNING! Proxy FD#" << proxyFD << " has data "
           << "to flush after setup of the NX transport.\n"
           << std::flush;
  }


  return 1;
}

int SetupAuthInstance()
{
  //
  // If user gave us a proxy cookie, then create the
  // X11 authorization repository and find the real
  // cookie to be used for our X display.
  //

  if (control -> ProxyMode == proxy_server)
  {
    if (*authCookie != '\0')
    {
      if (useLaunchdSocket == 1)
      {
        //
        // If we are going to retrieve the X11 autho-
        // rization through the launchd service, make
        // a connection to its socket to trigger the
        // X server starting.
        //

        sockaddr_un launchdAddrUnix;

        unsigned int launchdAddrLength = sizeof(sockaddr_un);

        int launchdAddrFamily = AF_UNIX;

        launchdAddrUnix.sun_family = AF_UNIX;

        // determine the maximum number of characters that fit into struct
        // sockaddr_un's sun_path member
        std::size_t launchdAddrNameLength =
          sizeof(struct sockaddr_un) - offsetof(struct sockaddr_un, sun_path);

        int success = -1;

        snprintf(launchdAddrUnix.sun_path, launchdAddrNameLength, "%s", displayHost);

        *(launchdAddrUnix.sun_path + launchdAddrNameLength - 1) = '\0';

        nxinfo << "Loop: Connecting to launchd service "
               << "on Unix port '" << displayHost << "'.\n" << std::flush;

        int launchdFd = socket(launchdAddrFamily, SOCK_STREAM, PF_UNSPEC);

        if (launchdFd < 0)
        {
          nxfatal << "Loop: PANIC! Call to socket failed. "
                  << "Error is " << EGET() << " '" << ESTR()
                  << "'.\n" << std::flush;
        }
        else if ((success = connect(launchdFd, (sockaddr *) &launchdAddrUnix, launchdAddrLength)) < 0)
        {
          nxwarn << "Loop: WARNING! Connection to launchd service "
                 << "on Unix port '" << displayHost << "' failed "
                 << "with error " << EGET() << ", '" << ESTR() << "'.\n"
                 << std::flush;
        }

        if (launchdFd >= 0)
        {
          close(launchdFd);
        }

        //
        // The real cookie will not be available
        // until the X server starts. Query for the
        // cookie in a loop, unless the connection
        // to the launchd service failed.
        //

        int attempts = (success < 0 ? 1 : 10);

        for (int i = 0; i < attempts; i++)
        {
          delete auth;

          auth = new Auth(displayHost, authCookie);

          if (auth != NULL && auth -> isFake() == 1)
          {
            usleep(200000);

            continue;
          }

          break;
        }
      }
      else
      {
        auth = new Auth(displayHost, authCookie);
      }

      if (auth == NULL || auth -> isValid() != 1)
      {
        nxfatal << "Loop: PANIC! Error creating the X authorization.\n"
                << std::flush;

        cerr << "Error" << ": Error creating the X authorization.\n";

        HandleCleanup();
      }
      else if (auth -> isFake() == 1)
      {
        nxwarn << "Loop: WARNING! Could not retrieve the X server "
               << "authentication cookie.\n"
               << std::flush;

        cerr << "Warning" << ": Failed to read data from the X "
             << "auth command.\n";

        cerr << "Warning" << ": Generated a fake cookie for X "
             << "authentication.\n";
      }
    }
    else
    {
      nxinfo << "Loop: No proxy cookie was provided for "
             << "authentication.\n" << std::flush;

      cerr << "Info" << ": No proxy cookie was provided for "
           << "authentication.\n";

      nxinfo << "Loop: Forwarding the real X authorization "
             << "cookie.\n" << std::flush;

      cerr << "Info" << ": Forwarding the real X authorization "
           << "cookie.\n";
    }
  }

  return 1;
}

int SetupAgentInstance()
{
  if (control -> ProxyMode == proxy_client &&
          useAgentSocket == 1)
  {
    //
    // This will temporarily disable signals to safely
    // load the cache, then will send a control packet
    // to the remote end, telling that cache has to be
    // loaded, so it's important that proxy is already
    // set in operational state.
    //

    int result;

    if (agent != NULL)
    {
      result = proxy -> handleNewAgentConnection(agent);
    }
    else
    {
      result = proxy -> handleNewConnection(channel_x11, agentFD[1]);
    }

    if (result < 0)
    {
      nxfatal << "Loop: PANIC! Error creating the NX agent connection.\n"
              << std::flush;

      cerr << "Error" << ": Error creating the NX agent connection.\n";

      HandleCleanup();
    }
  }

  return 1;
}

void SetupTcpSocket()
{
  //
  // Open TCP socket emulating local display.
  //

  tcpFD =  ListenConnectionTCP((loopbackBind ? "localhost" : "*"), X_TCP_PORT + proxyPort, "X11");
}

void SetupUnixSocket()
{
  //
  // Open UNIX domain socket for display.
  //

  unsigned int required = snprintf(unixSocketName, DEFAULT_STRING_LENGTH, "/tmp/.X11-unix");
  if (required < sizeof(unixSocketName)) {

    // No need to execute the following actions conditionally
    mkdir(unixSocketName, (0777 | S_ISVTX));
    chmod(unixSocketName, (0777 | S_ISVTX));

    required = snprintf(unixSocketName, DEFAULT_STRING_LENGTH, "/tmp/.X11-unix/X%d", proxyPort);
    if (required < sizeof(unixSocketName)) {

      unixFD = ListenConnectionUnix(unixSocketName, "x11");
      if (unixFD >= 0)
        chmod(unixSocketName, 0777);
      return;
    }
  }

  unixSocketName[0] = '\0'; // Just in case!

  nxfatal << "Loop: PANIC! path for unix socket is too long.\n" << std::flush;

  cerr << "Error" << ": path for Unix socket is too long.\n";
  HandleCleanup();
}

//
// The following is a dumb copy-paste. The
// nxcompsh library should offer a better
// implementation.
// addr is assumed to have been freed outside
//

void SetupDisplaySocket(int &addr_family, sockaddr *&addr,
                           unsigned int &addr_length)
{
  addr_family = AF_INET;
  addr_length = 0;

  char *display;

  if (*displayHost == '\0')
  {
    //
    // Assume DISPLAY as the X server to which
    // we will forward the proxied connections.
    // This means that NX parameters have been
    // passed through other means.
    //

    display = getenv("DISPLAY");

    if (display == NULL || *display == '\0')
    {
      nxfatal << "Loop: PANIC! Host X server DISPLAY is not set.\n"
              << std::flush;

      cerr << "Error" << ": Host X server DISPLAY is not set.\n";

      HandleCleanup();
    }
    else if (strncasecmp(display, "nx/nx,", 6) == 0 ||
                 strncasecmp(display, "nx,", 3) == 0 ||
                     strncasecmp(display, "nx/nx:", 6) == 0 ||
                         strncasecmp(display, "nx:", 3) == 0)
    {
      nxfatal << "Loop: PANIC! NX transport on host X server '"
              << display << "' not supported.\n" << std::flush;

      cerr << "Error" << ": NX transport on host X server '"
           << display << "' not supported.\n";

      cerr << "Error" << ": Please run the local proxy specifying "
           << "the host X server to connect to.\n";

      HandleCleanup();
    }
    else if (strlen(display) >= DEFAULT_STRING_LENGTH)
    {
      nxfatal << "Loop: PANIC! Host X server DISPLAY cannot exceed "
              << DEFAULT_STRING_LENGTH << " characters.\n"
              << std::flush;

      cerr << "Error" << ": Host X server DISPLAY cannot exceed "
           << DEFAULT_STRING_LENGTH << " characters.\n";

      HandleCleanup();
    }

    strcpy(displayHost, display);
  }

  display = new char[strlen(displayHost) + 1];

  if (display == NULL)
  {
    nxfatal << "Loop: PANIC! Out of memory handling DISPLAY variable.\n"
            << std::flush;

    cerr << "Error" << ": Out of memory handling DISPLAY variable.\n";

    HandleCleanup();
  }

  strcpy(display, displayHost);

  #ifdef __APPLE__

  if ((strncasecmp(display, "/tmp/launch", 11) == 0) ||
      (strncasecmp(display, "/private/tmp/com.apple.launchd", 30) == 0))
  {
    nxinfo << "Loop: Using launchd service on socket '"
           << display << "'.\n" << std::flush;

    useLaunchdSocket = 1;
  }

  #endif

  char *separator = strrchr(display, ':');

  if ((separator == NULL) || !isdigit(*(separator + 1)))
  {
    nxfatal << "Loop: PANIC! Invalid display '" << display << "'.\n"
            << std::flush;

    cerr << "Error" << ": Invalid display '" << display << "'.\n";

    delete [] display;

    HandleCleanup();
  }

  *separator = '\0';

  xPort = atoi(separator + 1);

  nxinfo << "Loop: Using local X display '" << displayHost
         << "' with host '" << display << "' and port '"
         << xPort << "'.\n" << std::flush;

  #ifdef __APPLE__

  if (separator == display || strcmp(display, "unix") == 0 ||
          useLaunchdSocket == 1)

  #else

  if (separator == display || strcmp(display, "unix") == 0)

  #endif
  {
    //
    // UNIX domain port.
    //

    // determine the maximum number of characters that fit into struct
    // sockaddr_un's sun_path member
    std::size_t maxlen_un =
      sizeof(struct sockaddr_un) - offsetof(struct sockaddr_un, sun_path);

    nxinfo << "Loop: Using real X server on UNIX domain socket.\n"
           << std::flush;

    addr_family = AF_UNIX;

    //
    // The scope of this function is to fill either the sockaddr_un
    // (when the display is set to the Unix Domain socket) or the
    // sockaddr_in structure (when connecting by TCP) only once, so
    // that the structure will be later used at the time the server
    // proxy will try to forward the connection to the X server. We
    // don't need to verify that the socket does exist at the pre-
    // sent moment. The method that forwards the connection will
    // perform the required checks and will retry, if needed. Anyway
    // we need to select the name of the socket, so we check if the
    // well-known directory exists and take that as an indication of
    // where the socket will be created.
    //

    // Try abstract X11 socket first (via a test connect), if that fails
    // fall back to Unix domain socket file.

    #ifdef __linux__
    int testSocketFD = socket(addr_family, SOCK_STREAM, PF_UNSPEC);

    // this name cannot be changed as it is defined this way by the
    // local X server
    int len = snprintf(unixSocketName + 1, DEFAULT_STRING_LENGTH - 1,
                       "/tmp/.X11-unix/X%d", xPort);
    unixSocketName[0] = '\0';

    sockaddr_un *xServerAddrABSTRACT = new sockaddr_un;
    memset(xServerAddrABSTRACT, 0, sizeof(struct sockaddr_un));
    xServerAddrABSTRACT -> sun_family = AF_UNIX;

    if (maxlen_un < (unsigned int)len + 1)
    {
      nxfatal << "Loop: PANIC! Abstract socket name '" << unixSocketName + 1
              << "' is too long!" << std::flush;

      delete [] display;
      delete xServerAddrABSTRACT;

      HandleCleanup();
    }

    // copy including the leading '\0'
    memcpy(xServerAddrABSTRACT -> sun_path, unixSocketName, len + 1);

    // man 7 unix:
    // "an abstract socket address is distinguished (from a
    // pathname socket) by the fact that sun_path[0] is a null byte
    // ('\0').  The socket's address in this namespace is given by the
    // additional bytes in sun_path that are covered by the specified
    // length of the address structure."
    addr_length = offsetof(struct sockaddr_un, sun_path) + len + 1;

    int ret = connect(testSocketFD,
                      (struct sockaddr *) xServerAddrABSTRACT,
                      addr_length);
    close(testSocketFD);

    if (ret == 0) {

        cerr << "Info" << ": Using abstract X11 socket in kernel namespace "
             << "for accessing DISPLAY=:" << xPort << ".\n";

        addr = (sockaddr *) xServerAddrABSTRACT;
        delete [] display;
        return;

    }

    cerr << "Info" << ": Falling back to file system X11 socket "
         << "for accessing DISPLAY=:" << xPort << ".\n";

    delete xServerAddrABSTRACT;

#endif

    struct stat statInfo;

    char unixSocketDir[DEFAULT_STRING_LENGTH];

    snprintf(unixSocketDir, DEFAULT_STRING_LENGTH, "/tmp/.X11-unix");

    #ifdef __APPLE__

    if (useLaunchdSocket == 1)
    {
      char *slash = rindex(display, '/');

      if (slash != NULL)
      {
        *slash = '\0';
      }

      snprintf(unixSocketDir, DEFAULT_STRING_LENGTH, "%s", display);
    }

    #endif

    *(unixSocketDir + DEFAULT_STRING_LENGTH - 1) = '\0';

    nxinfo << "Loop: Assuming X socket in directory '"
           << unixSocketDir << "'.\n" << std::flush;

    if (stat(unixSocketDir, &statInfo) < 0)
    {
      nxfatal << "Loop: PANIC! Can't determine the location of "
              << "the X display socket.\n" << std::flush;

      cerr << "Error" << ": Can't determine the location of "
           << "the X display socket.\n";

      nxfatal << "Loop: PANIC! Error " << EGET() << " '" << ESTR()
              << "' checking '" << unixSocketDir << "'.\n"
              << std::flush;

      cerr << "Error" << ": Error " << EGET() << " '" << ESTR()
           << "' checking '" << unixSocketDir << "'.\n";

      delete [] display;
      HandleCleanup();
    }

    snprintf(unixSocketName, DEFAULT_STRING_LENGTH, "%s/X%d",
             unixSocketDir, xPort);

    #ifdef __APPLE__

    if (useLaunchdSocket == 1)
    {
      snprintf(unixSocketName, DEFAULT_STRING_LENGTH, "%s", displayHost);
    }

    #endif

    nxinfo << "Loop: Assuming X socket name '" << unixSocketName
           << "'.\n" << std::flush;

    if (maxlen_un < strlen(unixSocketName) + 1)
    {
      nxfatal << "Loop: PANIC! Socket name '" << unixSocketName
              << "' is too long!" << std::flush;

      delete [] display;

      HandleCleanup();
    }

    sockaddr_un *xServerAddrUNIX = new sockaddr_un;
    xServerAddrUNIX -> sun_family = AF_UNIX;
    strcpy(xServerAddrUNIX -> sun_path, unixSocketName);

    addr = (sockaddr *) xServerAddrUNIX;
    addr_length = sizeof(sockaddr_un);

  }
  else
  {
    //
    // TCP port.
    //

    nxinfo << "Loop: Using real X server on TCP port.\n"
           << std::flush;

    addr_family = AF_INET;

    int xServerIPAddr = GetHostAddress(display);

    if (xServerIPAddr == 0)
    {
      nxfatal << "Loop: PANIC! Unknown display host '" << display
              << "'.\n" << std::flush;

      cerr << "Error" << ": Unknown display host '" << display
           << "'.\n";

      delete [] display;
      HandleCleanup();
    }

    sockaddr_in *xServerAddrTCP = new sockaddr_in;

    xServerAddrTCP -> sin_family = AF_INET;
    xServerAddrTCP -> sin_port = htons(X_TCP_PORT + xPort);
    xServerAddrTCP -> sin_addr.s_addr = xServerIPAddr;

    addr = (sockaddr *) xServerAddrTCP;
    addr_length = sizeof(sockaddr_in);
  }

  delete [] display;
}

void SetupServiceSockets()
{
  if (control -> ProxyMode == proxy_client)
  {
    if (useCupsSocket)
    {
      if ((cupsFD = ListenConnection(cupsPort, "CUPS")) < 0)
      {
        useCupsSocket = 0;
      }
    }

    if (useAuxSocket)
    {
      if ((auxFD = ListenConnection(auxPort, "auxiliary X11")) < 0)
      {
        useAuxSocket = 0;
      }
    }

    if (useSmbSocket)
    {
      if ((smbFD = ListenConnection(smbPort, "SMB")) < 0)
      {
        useSmbSocket = 0;
      }
    }

    if (useMediaSocket)
    {
      if ((mediaFD = ListenConnection(mediaPort, "media")) < 0)
      {
        useMediaSocket = 0;
      }
    }

    if (useHttpSocket)
    {
      if ((httpFD = ListenConnection(httpPort, "http")) < 0)
      {
        useHttpSocket = 0;
      }
    }

    useFontSocket = 0;
  }
  else
  {
    //
    // Get ready to listen for the font server connections
    //

    if (useFontSocket)
    {
      // Since ProtoStep7 (#issue 108)
      int port = atoi(fontPort);

      if ((fontFD = ListenConnectionTCP("localhost", port, "font")) < 0)
      {
        useFontSocket = 0;
      }
    }

    useCupsSocket  = 0;
    useAuxSocket   = 0;
    useSmbSocket   = 0;
    useMediaSocket = 0;
    useHttpSocket  = 0;
  }

  //
  // Slave channels can be originated
  // by both sides.
  //

  if (useSlaveSocket)
  {
    // Since ProtoStep7 (#issue 108)
    if ((slaveFD = ListenConnection(slavePort, "slave")) < 0)
    {
      useSlaveSocket = 0;
    }
  }
}

int ListenConnectionAny(sockaddr *addr, socklen_t addrlen, const char *label)
{
  int newFD = socket(addr->sa_family, SOCK_STREAM, PF_UNSPEC);

  if (newFD == -1)
  {
    nxfatal << "Loop: PANIC! Call to socket failed for " << label
            << " socket. Error is " << EGET() << " '"
            << ESTR() << "'.\n" << std::flush;

    cerr << "Error" << ": Call to socket failed for " << label
         << " socket. Error is " << EGET() << " '"
         << ESTR() << "'.\n";

    goto SetupSocketError;
  }

  if (addr->sa_family == AF_INET)
    if (SetReuseAddress(newFD) < 0)
    {
      // SetReuseAddress already warns with an error
      goto SetupSocketError;
    }

  if (bind(newFD, addr, addrlen) == -1)
  {
    nxfatal << "Loop: PANIC! Call to bind failed for " << label
            << ". Error is " << EGET()
            << " '" << ESTR() << "'.\n" << std::flush;

    cerr << "Error" << ": Call to bind failed for " << label
         << ". Error is " << EGET()
         << " '" << ESTR() << "'.\n";
    goto SetupSocketError;
  }

  if (listen(newFD, 8) == -1)
  {
    nxfatal << "Loop: PANIC! Call to listen failed for " << label
            << ". Error is " << EGET()
            << " '" << ESTR() << "'.\n" << std::flush;

    cerr << "Error" << ": Call to listen failed for " << label
         << ". Error is " << EGET()
         << " '" << ESTR() << "'.\n";

    goto SetupSocketError;
  }

  return newFD;

SetupSocketError:

  if (newFD != -1)
  {
    close(newFD);
  }

  //
  // May optionally return. The session would
  // continue without the service. The problem
  // with this approach is that it would make
  // harder to track problems with allocation
  // of ports in clients and server.
  //

  HandleCleanup();
  return -1;
}

int ListenConnectionUnix(const char *path, const char *label)
{

  sockaddr_un unixAddr;
  unixAddr.sun_family = AF_UNIX;
#ifdef UNIX_PATH_MAX
  if (strlen(path) >= UNIX_PATH_MAX)
#else
  if (strlen(path) >= sizeof(unixAddr.sun_path))
#endif
  {
    nxfatal << "Loop: PANIC! Socket path \"" << path << "\" for "
            << label << " is too long.\n" << std::flush;

    cerr << "Error" << ": Socket path \"" << path << "\" for "
         << label << " is too long.\n";

    HandleCleanup();
    return -1;
  }

  strcpy(unixAddr.sun_path, path);
  return ListenConnectionAny((sockaddr *)&unixAddr, sizeof(unixAddr), label);
}

int ListenConnectionTCP(const char *host, long port, const char *label)
{
  sockaddr_in tcpAddr;
  tcpAddr.sin_family = AF_INET;
  tcpAddr.sin_port = htons(port);

  if (loopbackBind ||
      !host ||
      !strcmp(host, "") ||
      !strcmp(host, "localhost")) {
    tcpAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  }
  else if(strcmp(host, "*") == 0) {
    tcpAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  }
  else {
    int ip = tcpAddr.sin_addr.s_addr = GetHostAddress(host);
    if (ip == 0)
    {
      nxfatal << "Loop: PANIC! Unknown " << label << " host '" << host
              << "'.\n" << std::flush;

      cerr << "Error" << ": Unknown " << label << " host '" << host
           << "'.\n";

      HandleCleanup();
      return -1;
    }
  }

  return ListenConnectionAny((sockaddr *)&tcpAddr, sizeof(tcpAddr), label);
}

int ListenConnection(ChannelEndPoint &endpoint, const char *label)
{
  char *unixPath = NULL, *host = NULL;
  long port;
  int result = -1;
  if (endpoint.getUnixPath(&unixPath)) {
    result = ListenConnectionUnix(unixPath, label);
  }
  else if (endpoint.getTCPHostAndPort(&host, &port)) {
    result = ListenConnectionTCP(host, port, label);
  }
  SAFE_FREE(unixPath);
  SAFE_FREE(host);
  return result;
}

static int AcceptConnection(int fd, int domain, const char *label)
{
  struct sockaddr newAddr;

  socklen_t addrLen = sizeof(newAddr);


  if (domain == AF_UNIX)
  {
    nxinfo << "Loop: Going to accept new Unix " << label
           << " connection on FD#" << fd << ".\n"
           << std::flush;
  }
  else
  {
    nxinfo << "Loop: Going to accept new TCP " << label
           << " connection on FD#" << fd << ".\n"
           << std::flush;
  }


  int newFD = accept(fd, &newAddr, &addrLen);

  if (newFD < 0)
  {
    nxfatal << "Loop: PANIC! Call to accept failed for "
            << label << " connection. Error is " << EGET()
            << " '" << ESTR() << "'.\n" << std::flush;

    cerr << "Error" << ": Call to accept failed for "
         << label << " connection. Error is " << EGET()
         << " '" << ESTR() << "'.\n";
  }

  return newFD;
}

void HandleShutdown()
{
  if (proxy -> getShutdown() == 0)
  {
    nxfatal << "Loop: PANIC! No shutdown of proxy link "
            << "performed by remote proxy.\n"
            << std::flush;

    //
    // Close the socket before showing the alert.
    // It seems that the closure of the socket can
    // sometimes take several seconds, even after
    // the connection is broken. The result is that
    // the dialog can be shown long after the user
    // has gone after the failed session. Note that
    // disabling the linger timeout does not seem
    // to make any difference.
    //

    CleanupSockets();

    cerr << "Error" << ": Connection with remote peer broken.\n";

    nxinfo << "Loop: Bytes received so far are "
           << (unsigned long long) statistics -> getBytesIn()
           << ".\n" << std::flush;

    cerr << "Error" << ": Please check the state of your "
         << "network and retry.\n";

    handleTerminatingInLoop();

    if (control -> ProxyMode == proxy_server)
    {
      nxinfo << "Loop: Showing the proxy abort dialog.\n"
             << std::flush;

      HandleAlert(ABORT_PROXY_CONNECTION_ALERT, 1);

      handleAlertInLoop();
    }
  }
  else
  {
    nxinfo << "Loop: Finalized the remote proxy shutdown.\n"
           << std::flush;
  }

  HandleCleanup();
}


void WaitCleanup()
{
  T_timestamp selectTs;

  while (NXTransRunning(NX_FD_ANY))
  {
    setTimestamp(selectTs, control -> PingTimeout);

    NXTransContinue(&selectTs);
  }
}

int KillProcess(int pid, const char *label, int signal, int wait)
{
  if (pid > 0)
  {
    nxinfo << "Loop: Killing the " << label << " process '"
           << pid << "' from process with pid '" << getpid()
           << "' with signal '" << DumpSignal(signal)
           << "'.\n" << std::flush;

    signal = (signal == 0 ? SIGTERM : signal);

    if (kill(pid, signal) < 0 && EGET() != ESRCH)
    {
      nxfatal << "Loop: PANIC! Couldn't kill the " << label
              << " process with pid '" << pid << "'.\n"
              << std::flush;

      cerr << "Error" << ": Couldn't kill the " << label
           << " process with pid '" << pid << "'.\n";
    }

    if (wait == 1)
    {
      WaitChild(pid, label, 1);
    }

    return 1;
  }
  else
  {
    nxinfo << "Loop: No " << label << " process "
           << "to kill with pid '" << pid
           << "'.\n" << std::flush;

    return 0;
  }
}

int CheckProcess(int pid, const char *label)
{
  nxinfo << "Loop: Checking the " << label << " process '"
         << pid << "' from process with pid '" << getpid()
         << "'.\n" << std::flush;

  if (kill(pid, SIGCONT) < 0 && EGET() == ESRCH)
  {
    nxwarn << "Loop: WARNING! The " << label << " process "
           << "with pid '" << pid << "' has exited.\n"
           << std::flush;

    cerr << "Warning" << ": The " << label << " process "
         << "with pid '" << pid << "' has exited.\n";

    return 0;
  }

  return 1;
}

void StartKeeper()
{

  if (IsRunning(lastKeeper) == 1 ||
          IsRestarting(lastKeeper) == 1)
  {
    nxfatal << "Loop: PANIC! The house-keeping process is "
            << "already running with pid '" << lastKeeper
            << "'.\n" << std::flush;

    HandleCleanup();
  }


  //
  // Don't care harvesting the persistent caches if
  // the memory cache is not enabled. If the memory
  // cache is not enabled neither we produced per-
  // sistent caches. The user can still delete any
  // persistent cache produced by the previous runs
  // by using the client GUI.
  //
  // TODO: At the moment the user doesn't have a way
  // to specify the amount of disk space to use for
  // the persistent caches, but only the amount of
  // space to use for images.
  //

  if (control -> LocalTotalStorageSize > 0)
  {
    nxinfo << "Loop: Starting the house-keeping process with "
           << "storage size " << control -> PersistentCacheDiskLimit
           << ".\n" << std::flush;

    lastKeeper = NXTransKeeper(control -> PersistentCacheDiskLimit,
                                   0, control -> RootPath);

    if (IsFailed(lastKeeper))
    {
      nxwarn << "Loop: WARNING! Failed to start the NX keeper process.\n"
             << std::flush;

      cerr << "Warning" << ": Failed to start the NX keeper process.\n";

      SetNotRunning(lastKeeper);
    }
    else
    {
      nxinfo << "Loop: Keeper started with pid '"
             << lastKeeper << "'.\n" << std::flush;
    }
  }
  else
  {
    nxinfo << "Loop: Nothing to do for the keeper process "
           << "with persistent cache not enabled.\n"
           << std::flush;
  }
}

void HandleCleanupForReconnect()
{
  nxinfo << "Loop: Going to clean up system resources for Reconnect "
         << "in process '" << getpid() << "'.\n"
         << std::flush;
  handleTerminatedInLoop();
  DisableSignals();
  if (control)
    CleanupChildren();
  CleanupListeners();
  CleanupSockets();
  CleanupKeeper();
  CleanupStreams();
  CleanupLocal();
  CleanupGlobal();
  RestoreSignals();
  ServerCache::lastInitReply.set(0,NULL);
  ServerCache::lastKeymap.set(0,NULL);
  ServerCache::getKeyboardMappingLastMap.set(0,NULL);
}
void HandleCleanup(int code)
{
  nxinfo << "Loop: Going to clean up system resources "
         << "in process '" << getpid() << "'.\n"
         << std::flush;

  handleTerminatedInLoop();

  //
  // Suspend any signal while cleaning up.
  //

  DisableSignals();

  if (getpid() == lastProxy)
  {
    //
    // Terminate all the children.
    //

    CleanupChildren();

    //
    // Close all listeners.
    //

    CleanupListeners();

    //
    // Close all sockets.
    //

    CleanupSockets();

    //
    // Release the global objects.
    //

    CleanupGlobal();

    //
    // Restore the original signal handlers.
    //

    RestoreSignals();
  }

  //
  // This is our last chance to print a message. If this
  // is the process which created the transport we will
  // jump back into the loop, letting the caller find out
  // that the connection is broken, otherwise we assume
  // that this is a child of the proxy and so we will
  // safely exit.
  //


  if (getpid() == lastProxy)
  {
    nxinfo << "Loop: Reverting to loop context in process with "
           << "pid '" << getpid() << "' at " << strMsTimestamp()
           << ".\n" << std::flush;
  }
  else
  {
    nxinfo << "Loop: Exiting from child process with pid '"
           << getpid() << "' at " << strMsTimestamp()
           << ".\n" << std::flush;
  }


  if (getpid() == lastProxy)
  {
    //
    // Reset all values to their default.
    //

    CleanupLocal();

    CleanupStreams();

    longjmp(context, 1);
  }
  else
  {
    //
    // Give a last chance to the process
    // to cleanup the ancillary classes.
    //

    CleanupKeeper();

    CleanupStreams();

    exit(code);
  }
}

void CleanupKeeper()
{
  if (keeper != NULL)
  {
    nxinfo << "Loop: Freeing up keeper in process "
           << "with pid '" << getpid() << "'.\n"
           << std::flush;

    delete keeper;

    keeper = NULL;
  }
}

void CleanupStreams()
{
  //
  // TODO: The cleanup procedure skips deletion of
  // the I/O streams under Windows. This is intended
  // to avoid a strange segfault occurring randomly,
  // at the time the proxy is being shut down.
  //

  #if !(defined(__CYGWIN__) || defined(__CYGWIN32__))

  nxinfo << "Loop: Freeing up streams in process "
         << "with pid '" << getpid() << "'.\n"
         << std::flush;

  if (logofs != NULL && logofs != &cerr &&
          *errorsFileName != '\0')
  {
    *logofs << flush;

    delete logofs;

    //
    // Let the log go again to the standard
    // error.
    //

    logofs = &cerr;
  }

  if (statofs != NULL && statofs != &cerr &&
          *statsFileName != '\0')
  {
    *statofs << flush;

    delete statofs;

    statofs = NULL;
  }

  if (errofs != NULL)
  {
    *errofs << flush;

    if (errofs == &cerr)
    {
      errofs = NULL;
    }
    else if (errsbuf != NULL)
    {
      cerr.rdbuf(errsbuf);

      errsbuf = NULL;

      delete errofs;
    }

    errofs = NULL;
  }

  #endif /* #if !(defined(__CYGWIN__) || defined(__CYGWIN32__)) */

  //
  // Reset these as they can't be reset
  // in CleanupLocal().
  //

  *sessionFileName = '\0';
  *errorsFileName  = '\0';
  *optionsFileName = '\0';
  *statsFileName   = '\0';
}

void CleanupChildren()
{
  //
  // Remove any watchdog.
  //

  if (IsRunning(lastWatchdog))
  {
    KillProcess(lastWatchdog, "watchdog", SIGTERM, 1);

    SetNotRunning(lastWatchdog);

    lastSignal = 0;
  }

  //
  // Kill the cache house-keeping process.
  //

  if (IsRunning(lastKeeper))
  {
    KillProcess(lastKeeper, "house-keeping", SIGTERM, 1);

    SetNotRunning(lastKeeper);
  }

  //
  // Let any running dialog to continue until it is
  // closed by the user. In general this is the exp-
  // ected behaviour, as for example when we are
  // exiting because the link was abrouptedly shut
  // down.
  //

  if (IsRunning(lastDialog))
  {
    nxinfo << "Loop: WARNING! Leaving the dialog process '"
           << lastDialog << "' running in process "
           << "with pid '" << getpid() << "'.\n"
           << std::flush;

    SetNotRunning(lastDialog);
  }

  //
  // Give user a chance to start a new session.
  //

  if (control -> EnableRestartOnShutdown == 1)
  {
    nxwarn << "Loop: WARNING! Respawning the NX client "
           << "on display '" << displayHost << "'.\n"
           << std::flush;

    NXTransClient(displayHost);
  }

  for (int i = 0; i < control -> KillDaemonOnShutdownNumber; i++)
  {
    nxwarn << "Loop: WARNING! Killing the NX daemon with "
           << "pid '" << control -> KillDaemonOnShutdown[i]
           << "'.\n" << std::flush;

    KillProcess(control -> KillDaemonOnShutdown[i], "daemon", SIGTERM, 0);
  }
}

void CleanupGlobal()
{
  if (proxy != NULL)
  {
    nxinfo << "Loop: Freeing up proxy in process "
           << "with pid '" << getpid() << "'.\n"
           << std::flush;

    delete proxy;

    proxy = NULL;
  }

  if (agent != NULL)
  {
    nxinfo << "Loop: Freeing up agent in process "
           << "with pid '" << getpid() << "'.\n"
           << std::flush;

    delete agent;

    agent = NULL;
  }

  if (auth != NULL)
  {
    nxinfo << "Loop: Freeing up auth data in process "
           << "with pid '" << getpid() << "'.\n"
           << std::flush;

    delete auth;

    auth = NULL;
  }

  if (statistics != NULL)
  {
    nxinfo << "Loop: Freeing up statistics in process "
           << "with pid '" << getpid() << "'.\n"
           << std::flush;

    delete statistics;

    statistics = NULL;
  }

  if (control != NULL)
  {
    nxinfo << "Loop: Freeing up control in process "
           << "with pid '" << getpid() << "'.\n"
           << std::flush;

    delete control;

    control = NULL;
  }
}

void CleanupConnections()
{
  if (proxy -> getChannels(channel_x11) != 0)
  {
    nxinfo << "Loop: Closing any remaining X connections.\n"
           << std::flush;

    proxy -> handleCloseAllXConnections();

    nxinfo << "Loop: Closing any remaining listener.\n"
           << std::flush;

    proxy -> handleCloseAllListeners();
  }

  proxy -> handleFinish();
}

void CleanupListeners()
{
  if (useTcpSocket == 1)
  {
    if (tcpFD != -1)
    {
      nxinfo << "Loop: Closing TCP listener in process "
             << "with pid '" << getpid() << "'.\n"
             << std::flush;

      close(tcpFD);

      tcpFD = -1;
    }

    useTcpSocket = 0;
  }

  if (useUnixSocket == 1)
  {
    if (unixFD != -1)
    {
      nxinfo << "Loop: Closing UNIX listener in process "
             << "with pid '" << getpid() << "'.\n"
             << std::flush;

      close(unixFD);

      unixFD = -1;
    }

    if (*unixSocketName != '\0')
    {
      nxinfo << "Loop: Going to remove the Unix domain socket '"
             << unixSocketName << "' in process " << "with pid '"
             << getpid() << "'.\n" << std::flush;

      unlink(unixSocketName);
    }

    useUnixSocket = 0;
  }

  if (useAgentSocket == 1)
  {
    //
    // There is no listener for the
    // agent descriptor.
    //

    useAgentSocket = 0;
  }

  if (useCupsSocket == 1)
  {
    if (cupsFD != -1)
    {
      nxinfo << "Loop: Closing CUPS listener in process "
             << "with pid '" << getpid() << "'.\n"
             << std::flush;

      close(cupsFD);

      cupsFD = -1;
    }

    useCupsSocket = 0;
  }

  if (useAuxSocket == 1)
  {
    if (auxFD != -1)
    {
      nxinfo << "Loop: Closing auxiliary X11 listener "
             << "in process " << "with pid '" << getpid()
             << "'.\n" << std::flush;

      close(auxFD);

      auxFD = -1;
    }

    useAuxSocket = 0;
  }

  if (useSmbSocket == 1)
  {
    if (smbFD != -1)
    {
      nxinfo << "Loop: Closing SMB listener in process "
             << "with pid '" << getpid() << "'.\n"
             << std::flush;

      close(smbFD);

      smbFD = -1;
    }

    useSmbSocket = 0;
  }

  if (useMediaSocket == 1)
  {
    if (mediaFD != -1)
    {
      nxinfo << "Loop: Closing multimedia listener in process "
             << "with pid '" << getpid() << "'.\n"
             << std::flush;

      close(mediaFD);

      mediaFD = -1;
    }

    useMediaSocket = 0;
  }

  if (useHttpSocket == 1)
  {
    if (httpFD != -1)
    {
      nxinfo << "Loop: Closing http listener in process "
             << "with pid '" << getpid() << "'.\n"
             << std::flush;

      close(httpFD);

      httpFD = -1;
    }

    useHttpSocket = 0;
  }

  if (useFontSocket == 1)
  {
    if (fontFD != -1)
    {
      nxinfo << "Loop: Closing font server listener in process "
             << "with pid '" << getpid() << "'.\n"
             << std::flush;

      close(fontFD);

      fontFD = -1;
    }

    useFontSocket = 0;
  }

  if (useSlaveSocket == 1)
  {
    if (slaveFD != -1)
    {
      nxinfo << "Loop: Closing slave listener in process "
             << "with pid '" << getpid() << "'.\n"
             << std::flush;

      close(slaveFD);

      slaveFD = -1;
    }

    useSlaveSocket = 0;
  }
}

void CleanupSockets()
{
  if (proxyFD != -1)
  {
    nxinfo << "Loop: Closing proxy FD in process "
           << "with pid '" << getpid() << "'.\n"
           << std::flush;

    close(proxyFD);

    proxyFD = -1;
  }

  if (agentFD[1] != -1)
  {
    nxinfo << "Loop: Closing agent FD in process "
           << "with pid '" << getpid() << "'.\n"
           << std::flush;

    close(agentFD[1]);

    agentFD[0] = -1;
    agentFD[1] = -1;
  }
}

void CleanupLocal()
{
  *homeDir    = '\0';
  *rootDir    = '\0';
  *tempDir    = '\0';
  *systemDir  = '\0';
  *sessionDir = '\0';

  *linkSpeedName    = '\0';
  *cacheSizeName    = '\0';
  *shsegSizeName    = '\0';
  *imagesSizeName   = '\0';
  *bitrateLimitName = '\0';
  *packMethodName   = '\0';
  *productName      = '\0';

  packMethod  = -1;
  packQuality = -1;

  *sessionType = '\0';
  *sessionId   = '\0';

  parsedOptions = 0;
  parsedCommand = 0;

  *remoteData = '\0';
  remotePosition = 0;

  tcpFD   = -1;
  unixFD  = -1;
  cupsFD  = -1;
  auxFD   = -1;
  smbFD   = -1;
  mediaFD = -1;
  httpFD  = -1;
  fontFD  = -1;
  slaveFD = -1;
  proxyFD = -1;

  agentFD[0] = -1;
  agentFD[1] = -1;

  useUnixSocket  = 1;
  useTcpSocket   = 1;
  useCupsSocket  = 0;
  useAuxSocket   = 0;
  useSmbSocket   = 0;
  useMediaSocket = 0;
  useHttpSocket  = 0;
  useFontSocket  = 0;
  useSlaveSocket = 0;
  useAgentSocket = 0;

  useNoDelay = -1;
  usePolicy  = -1;
  useRender  = -1;
  useTaint   = -1;

  *unixSocketName = '\0';

  *acceptHost  = '\0';
  *displayHost = '\0';
  *authCookie  = '\0';

  proxyPort = DEFAULT_NX_PROXY_PORT;
  xPort     = DEFAULT_NX_X_PORT;

  xServerAddrFamily = -1;
  xServerAddrLength = 0;

  delete xServerAddr;

  xServerAddr = NULL;

  listenSocket.disable();
  connectSocket.disable();

  cupsPort.disable();
  auxPort.disable();
  smbPort.disable();
  mediaPort.disable();
  httpPort.disable();
  slavePort.disable();

  *fontPort = '\0';

  *bindHost = '\0';
  bindPort = -1;

  initTs  = nullTimestamp();
  startTs = nullTimestamp();
  logsTs  = nullTimestamp();
  nowTs   = nullTimestamp();

  lastProxy    = 0;
  lastDialog   = 0;
  lastWatchdog = 0;
  lastKeeper   = 0;
  lastStatus   = 0;
  lastKill     = 0;
  lastDestroy  = 0;

  lastReadableTs = nullTimestamp();

  lastAlert.code  = 0;
  lastAlert.local = 0;

  lastMasks.blocked   = 0;
  lastMasks.installed = 0;

  memset(&lastMasks.saved, 0, sizeof(sigset_t));

  for (int i = 0; i < 32; i++)
  {
    lastMasks.enabled[i] = 0;
    lastMasks.forward[i] = 0;

    memset(&lastMasks.action[i], 0, sizeof(struct sigaction));
  }

  lastSignal = 0;

  memset(&lastTimer.action, 0, sizeof(struct sigaction));
  memset(&lastTimer.value,  0, sizeof(struct itimerval));

  lastTimer.start = nullTimestamp();
  lastTimer.next  = nullTimestamp();
}

int CheckAbort()
{
  if (lastSignal != 0)
  {
    nxinfo << "Loop: Aborting the procedure due to signal '"
           << lastSignal << "', '" << DumpSignal(lastSignal)
           << "'.\n" << std::flush;

    cerr << "Info" << ": Aborting the procedure due to signal '"
         << lastSignal << "'.\n";

    lastSignal = 0;

    return 1;
  }

  return 0;
}

void HandleAbort()
{
  if (logofs == NULL)
  {
    logofs = &cerr;
  }

  *logofs << flush;

  handleTerminatingInLoop();

  if (lastSignal == SIGHUP)
  {
    lastSignal = 0;
  }

  //
  // The current default is to just quit the program.
  // Code has not been updated to deal with the new
  // NX transport loop.
  //

  if (control -> EnableCoreDumpOnAbort == 1)
  {
    if (agent != NULL)
    {
      cerr << "Session" << ": Terminating session at '"
           << strTimestamp() << "'.\n";
    }

    cerr << "Error" << ": Generating a core file to help "
         << "the investigations.\n";

    cerr << "Session" << ": Session terminated at '"
         << strTimestamp() << "'.\n";

    cerr << flush;

    signal(SIGABRT, SIG_DFL);

    raise(SIGABRT);
  }

  nxinfo << "Loop: Showing the proxy abort dialog.\n"
         << std::flush;

  if (control -> ProxyMode == proxy_server)
  {
    //
    // Close the socket before showing the alert.
    // It seems that the closure of the socket can
    // sometimes take several seconds, even after
    // the connection is broken.
    //

    CleanupSockets();

    if (lastKill == 0)
    {
      HandleAlert(ABORT_PROXY_CONNECTION_ALERT, 1);
    }
    else
    {
      HandleAlert(ABORT_PROXY_SHUTDOWN_ALERT, 1);
    }

    handleAlertInLoop();
  }

  HandleCleanup();
}

void HandleAlert(int code, int local)
{
  if (lastAlert.code == 0)
  {
    nxinfo << "Loop: Requesting an alert dialog with code "
           << code << " and local " << local << ".\n"
           << std::flush;

    lastAlert.code  = code;
    lastAlert.local = local;
  }
  else
  {
    nxinfo << "Loop: WARNING! Alert dialog already requested "
           << "with code " << lastAlert.code << ".\n"
           << std::flush;
  }

  return;
}

void FlushCallback(int length)
{
  if (flushCallback != NULL)
  {
    nxinfo << "Loop: Reporting a flush request at "
           << strMsTimestamp() << " with " << length
           << " bytes written.\n" << std::flush;

    (*flushCallback)(flushParameter, length);
  }
  else if (control -> ProxyMode == proxy_client)
  {
    nxinfo << "Loop: WARNING! Can't find a flush "
           << "callback in process with pid '" << getpid()
           << "'.\n" << std::flush;
  }
}

void KeeperCallback()
{
  if (IsRunning(lastKeeper) == 0)
  {
    //
    // Let the house-keeping process take care
    // of the persistent image cache.
    //

    if (control -> ImageCacheEnableLoad == 1 ||
            control -> ImageCacheEnableSave == 1)
    {
      nxinfo << "Loop: Starting the house-keeping process with "
             << "image storage size " << control -> ImageCacheDiskLimit
             << ".\n" << std::flush;

      lastKeeper = NXTransKeeper(0, control -> ImageCacheDiskLimit,
                                     control -> RootPath);

      if (IsFailed(lastKeeper))
      {
        nxwarn << "Loop: WARNING! Can't start the NX keeper process.\n"
               << std::flush;

        SetNotRunning(lastKeeper);
      }
      else
      {
        nxinfo << "Loop: Keeper started with pid '"
               << lastKeeper << "'.\n" << std::flush;
      }
    }
    else
    {
      nxinfo << "Loop: Nothing to do for the keeper process "
             << "with image cache not enabled.\n"
             << std::flush;
    }
  }
  else
  {
    nxinfo << "Loop: Nothing to do with the keeper process "
           << "already running.\n" << std::flush;
  }
}

void InstallSignals()
{
  nxinfo << "Loop: Installing signals in process with pid '"
         << getpid() << "'.\n" << std::flush;

  for (int i = 0; i < 32; i++)
  {
    if (CheckSignal(i) == 1 &&
          lastMasks.enabled[i] == 0)
    {
      InstallSignal(i, NX_SIGNAL_ENABLE);
    }
  }

  lastMasks.installed = 1;
}

void RestoreSignals()
{
  nxinfo << "Loop: Restoring signals in process with pid '"
         << getpid() << "'.\n" << std::flush;

  if (lastMasks.installed == 1)
  {
    //
    // Need to keep monitoring the children.
    //

    for (int i = 0; i < 32; i++)
    {
      if (lastMasks.enabled[i] == 1)
      {
        RestoreSignal(i);
      }
    }
  }

  lastMasks.installed = 0;

  if (lastMasks.blocked == 1)
  {
    EnableSignals();
  }
}

void DisableSignals()
{
  if (lastMasks.blocked == 0)
  {
    sigset_t newMask;

    sigemptyset(&newMask);

    //
    // Block also the other signals that may be
    // installed by the agent, that are those
    // signals for which the function returns 2.
    //

    for (int i = 0; i < 32; i++)
    {
      if (CheckSignal(i) > 0)
      {
        nxdbg << "Loop: Disabling signal " << i << " '"
              << DumpSignal(i) << "' in process with pid '"
              << getpid() << "'.\n" << std::flush;

        sigaddset(&newMask, i);
      }
    }

    sigprocmask(SIG_BLOCK, &newMask, &lastMasks.saved);

    lastMasks.blocked++;
  }
  else
  {
    nxinfo << "Loop: WARNING! Signals were already blocked in "
           << "process with pid '" << getpid() << "'.\n"
           << std::flush;
  }
}

void EnableSignals()
{
  if (lastMasks.blocked == 1)
  {
    nxinfo << "Loop: Enabling signals in process with pid '"
           << getpid() << "'.\n" << std::flush;

    sigprocmask(SIG_SETMASK, &lastMasks.saved, NULL);

    lastMasks.blocked = 0;
  }
  else
  {
    nxwarn << "Loop: WARNING! Signals were not blocked in "
           << "process with pid '" << getpid() << "'.\n"
           << std::flush;

    cerr << "Warning" << ": Signals were not blocked in "
         << "process with pid '" << getpid() << "'.\n";
  }
}

void InstallSignal(int signal, int action)
{
  if (lastMasks.enabled[signal] == 1)
  {
    if (action == NX_SIGNAL_FORWARD)
    {
      nxinfo << "Loop: Forwarding handler for signal " << signal
             << " '" << DumpSignal(signal) << "' in process "
             << "with pid '" << getpid() << "'.\n"
             << std::flush;

      lastMasks.forward[signal] = 1;

      return;
    }
    else
    {
      nxinfo << "Loop: Reinstalling handler for signal " << signal
             << " '" << DumpSignal(signal) << "' in process "
             << "with pid '" << getpid() << "'.\n"
             << std::flush;
    }
  }
  else
  {
    nxinfo << "Loop: Installing handler for signal " << signal
           << " '" << DumpSignal(signal) << "' in process "
           << "with pid '" << getpid() << "'.\n"
           << std::flush;
  }

  if (signal == SIGALRM && isTimestamp(lastTimer.start))
  {
    ResetTimer();
  }

  struct sigaction newAction;

  memset(&newAction, 0, sizeof(newAction));

  newAction.sa_handler = HandleSignal;

  sigemptyset(&(newAction.sa_mask));

  if (signal == SIGCHLD)
  {
    newAction.sa_flags = SA_NOCLDSTOP;
  }
  else
  {
    newAction.sa_flags = 0;
  }

  sigaction(signal, &newAction, &lastMasks.action[signal]);

  lastMasks.enabled[signal] = 1;

  if (action == NX_SIGNAL_FORWARD)
  {
    lastMasks.forward[signal] = 1;
  }
}

void RestoreSignal(int signal)
{
  if (lastMasks.enabled[signal] == 0)
  {
    nxwarn << "Loop: WARNING! Signal '" << DumpSignal(signal)
           << "' not installed in process with pid '"
           << getpid() << "'.\n" << std::flush;

    cerr << "Warning" << ": Signal '" << DumpSignal(signal)
         << "' not installed in process with pid '"
         << getpid() << "'.\n";

    return;
  }

  nxinfo << "Loop: Restoring handler for signal " << signal
         << " '" << DumpSignal(signal) << "' in process "
         << "with pid '" << getpid() << "'.\n"
         << std::flush;

  if (signal == SIGALRM && isTimestamp(lastTimer.start))
  {
    ResetTimer();
  }

  sigaction(signal, &lastMasks.action[signal], NULL);

  lastMasks.enabled[signal] = 0;
  lastMasks.forward[signal] = 0;
}

void HandleSignal(int signal)
{
  if (logofs == NULL)
  {
    logofs = &cerr;
  }


  if (lastSignal != 0)
  {
    nxinfo << "Loop: WARNING! Last signal is '" << lastSignal
           << "', '" << DumpSignal(signal) << "' and not zero "
           << "in process with pid '" << getpid() << "'.\n"
           << std::flush;
  }

  nxinfo << "Loop: Signal '" << signal << "', '"
         << DumpSignal(signal) << "' received in process "
         << "with pid '" << getpid() << "'.\n" << std::flush;


  if (getpid() != lastProxy && signalHandler != NULL)
  {
    nxinfo << "Loop: Calling slave handler in process "
           << "with pid '" << getpid() << "'.\n"
           << std::flush;

    if ((*signalHandler)(signal) == 0)
    {
      return;
    }
  }

  switch (signal)
  {
    case SIGUSR1:
    {
      if (proxy != NULL && lastSignal == 0)
      {
        lastSignal = SIGUSR1;
      }

      break;
    }
    case SIGUSR2:
    {
      if (proxy != NULL && lastSignal == 0)
      {
        lastSignal = SIGUSR2;
      }

      break;
    }
    case SIGPIPE:
    {
      //
      // It can happen that SIGPIPE is delivered
      // to the proxy even in the case some other
      // descriptor is unexpectedly closed.
      //
      // if (agentFD[1] != -1)
      // {
      //   cerr << "Info" << ": Received signal 'SIGPIPE'. "
      //        << "Closing agent connection.\n";
      //
      //   shutdown(agentFD[1], SHUT_RDWR);
      // }
      //

      break;
    }
    case SIGALRM:
    {
      //
      // Nothing to do. Just wake up the
      // process on blocking operations.
      //

      break;
    }
    case SIGCHLD:
    {
      //
      // Check if any of our children has exited.
      //

      if (HandleChildren() != 0)
      {
        signal = 0;
      }

      //
      // Don't save this signal or it will override
      // any previous signal sent by child before
      // exiting.
      //

      break;
    }

    #if defined(__CYGWIN__) || defined(__CYGWIN32__)

    case 12:
    {
      //
      // Nothing to do. This signal is what is delivered
      // by the Cygwin library when trying use a shared
      // memory function if the daemon is not running.
      //

      nxinfo << "Loop: WARNING! Received signal '12' in "
             << "process with pid '" << getpid() << "'.\n"
             << std::flush;

      nxinfo << "Loop: WARNING! Please check that the "
             << "cygserver daemon is running.\n"
             << std::flush;

      break;
    }

    #endif

    default:
    {
      //
      // Register the signal so we can handle it
      // inside the main loop. We will probably
      // dispose any resource and exit.
      //

      if (getpid() == lastProxy)
      {
        nxinfo << "Loop: Registering end of session request "
               << "due to signal '" << signal << "', '"
               << DumpSignal(signal) << "'.\n"
               << std::flush;

        lastSignal = signal;
      }
      else
      {
        //
        // This is a child, so exit immediately.
        //

        HandleCleanup();
      }
    }
  }

  if (signal != 0 && lastMasks.forward[signal] == 1)
  {
    if (lastMasks.action[signal].sa_handler != NULL &&
            lastMasks.action[signal].sa_handler != HandleSignal)
    {
      nxinfo << "Loop: Forwarding signal '" << signal << "', '"
             << DumpSignal(signal) << "' to previous handler.\n"
             << std::flush;

      lastMasks.action[signal].sa_handler(signal);
    }
    else if (lastMasks.action[signal].sa_handler == NULL)
    {
      nxwarn << "Loop: WARNING! Parent requested to forward "
             << "signal '" << signal << "', '" << DumpSignal(signal)
             << "' but didn't set a handler.\n" << std::flush;
    }
  }
}

int HandleChildren()
{
  //
  // Try to waitpid() for each child because the
  // call might have return ECHILD and so we may
  // have lost any of the processes.
  //

  if (IsRunning(lastDialog) && HandleChild(lastDialog) == 1)
  {
    nxinfo << "Loop: Resetting pid of last dialog process "
           << "in handler.\n" << std::flush;

    SetNotRunning(lastDialog);

    if (proxy != NULL)
    {
      proxy -> handleResetAlert();
    }

    return 1;
  }

  if (IsRunning(lastWatchdog) && HandleChild(lastWatchdog) == 1)
  {
    nxinfo << "Loop: Watchdog is gone. Setting the last "
           << "signal to SIGHUP.\n" << std::flush;

    lastSignal = SIGHUP;

    nxinfo << "Loop: Resetting pid of last watchdog process "
           << "in handler.\n" << std::flush;

    SetNotRunning(lastWatchdog);

    return 1;
  }

  //
  // The house-keeping process exits after a
  // number of iterations to keep the memory
  // pollution low. It is restarted on demand
  // by the lower layers, using the callback
  // function.
  //

  if (IsRunning(lastKeeper) && HandleChild(lastKeeper) == 1)
  {
    nxinfo << "Loop: Resetting pid of last house-keeping "
           << "process in handler.\n" << std::flush;

    SetNotRunning(lastKeeper);

    return 1;
  }

  //
  // The pid will be checked by the code
  // that registered the child.
  //

  if (IsRunning(lastChild))
  {
    nxinfo << "Loop: Resetting pid of last child process "
           << "in handler.\n" << std::flush;

    SetNotRunning(lastChild);

    return 1;
  }

  proxy->checkSlaves();

  //
  // This can actually happen either because we
  // reset the pid of the child process as soon
  // as we kill it, or because of a child process
  // of our parent.
  //

  nxinfo << "Loop: Ignoring signal received for the "
         << "unregistered child.\n" << std::flush;

  return 0;
}

int HandleChild(int child)
{
  int pid;

  int status  = 0;
  int options = WNOHANG | WUNTRACED;

  while ((pid = waitpid(child, &status, options)) &&
             pid == -1 && EGET() == EINTR);

  return CheckChild(pid, status);
}

int WaitChild(int child, const char* label, int force)
{
  int pid;

  int status  = 0;
  int options = WUNTRACED;

  for (;;)
  {
    nxinfo << "Loop: Waiting for the " << label
           << " process '" << child << "' to die.\n"
           << std::flush;

    pid = waitpid(child, &status, options);

    if (pid == -1 && EGET() == EINTR)
    {
      if (force == 0)
      {
        return 0;
      }

      nxwarn << "Loop: WARNING! Ignoring signal while "
             << "waiting for the " << label << " process '"
             << child << "' to die.\n"
             << std::flush;

      continue;
    }

    break;
  }

  return (EGET() == ECHILD ? 1 : CheckChild(pid, status));
}

int CheckChild(int pid, int status)
{
  lastStatus = 0;

  if (pid > 0)
  {
    if (WIFSTOPPED(status))
    {
      nxinfo << "Loop: Child process '" << pid << "' was stopped "
             << "with signal " << (WSTOPSIG(status)) << ".\n"
             << std::flush;

      return 0;
    }
    else
    {
      if (WIFEXITED(status))
      {
        nxinfo << "Loop: Child process '" << pid << "' exited "
               << "with status '" << (WEXITSTATUS(status))
               << "'.\n" << std::flush;

        lastStatus = WEXITSTATUS(status);
      }
      else if (WIFSIGNALED(status))
      {
        if (CheckSignal(WTERMSIG(status)) != 1)
        {
          nxwarn << "Loop: WARNING! Child process '" << pid
                 << "' died because of signal " << (WTERMSIG(status))
                 << ", '" << DumpSignal(WTERMSIG(status)) << "'.\n"
                 << std::flush;

          cerr << "Warning" << ": Child process '" << pid
               << "' died because of signal " << (WTERMSIG(status))
               << ", '" << DumpSignal(WTERMSIG(status)) << "'.\n";
        }
        else
        {
          nxinfo << "Loop: Child process '" << pid
                 << "' died because of signal " << (WTERMSIG(status))
                 << ", '" << DumpSignal(WTERMSIG(status)) << "'.\n"
                 << std::flush;
        }

        lastStatus = 1;
      }

      return 1;
    }
  }
  else if (pid < 0)
  {
    if (EGET() != ECHILD)
    {
      nxfatal << "Loop: PANIC! Call to waitpid failed. "
              << "Error is " << EGET() << " '" << ESTR()
              << "'.\n" << std::flush;

      cerr << "Error" << ": Call to waitpid failed. "
           << "Error is " << EGET() << " '" << ESTR()
           << "'.\n";

      HandleCleanup();
    }

    //
    // This can happen when the waitpid() is
    // blocking, as the SIGCHLD is received
    // within the call.
    //

    nxinfo << "Loop: No more children processes running.\n"
           << std::flush;

    return 1;
  }

  return 0;
}

void RegisterChild(int child)
{

  if (IsNotRunning(lastChild))
  {
    nxinfo << "Loop: Registering child process '" << child
           << "' in process with pid '" << getpid()
           << "'.\n" << std::flush;
  }
  else
  {
    nxinfo << "Loop: WARNING! Overriding registered child '"
           << lastChild << "' with new child '" << child
           << "' in process with pid '" << getpid()
           << "'.\n" << std::flush;
  }


  lastChild = child;
}

int CheckParent(const char *name, const char *type, int parent)
{
  if (parent != getppid() || parent == 1)
  {
    nxwarn << name << ": WARNING! Parent process appears "
           << "to be dead. Exiting " << type << ".\n"
           << std::flush;

    cerr << "Warning" << ": Parent process appears "
         << "to be dead. Exiting " << type << ".\n";

    return 0;
  }

  return 1;
}

void HandleTimer(int signal)
{
  if (signal == SIGALRM)
  {
    if (isTimestamp(lastTimer.start))
    {
      nxinfo << "Loop: Timer expired at " << strMsTimestamp()
             << " in process with pid '" << getpid() << "'.\n"
             << std::flush;

      if (proxy != NULL)
      {
        proxy -> handleTimer();
      }

      ResetTimer();
    }
    else
    {
      nxfatal << "Loop: PANIC! Inconsistent timer state "
              << " in process with pid '" << getpid() << "'.\n"
              << std::flush;

      cerr << "Error" << ": Inconsistent timer state "
           << " in process with pid '" << getpid() << "'.\n";
    }
  }
  else
  {
    nxfatal << "Loop: PANIC! Inconsistent signal '"
            << signal << "', '" << DumpSignal(signal)
            << "' received in process with pid '"
            << getpid() << "'.\n" << std::flush;

    cerr << "Error" << ": Inconsistent signal '"
         << signal << "', '" << DumpSignal(signal)
         << "' received in process with pid '"
         << getpid() << "'.\n";
  }
}

void SetTimer(int value)
{
  getNewTimestamp();

  if (isTimestamp(lastTimer.start))
  {
    int diffTs = diffTimestamp(lastTimer.start, getTimestamp());

    if (diffTs > lastTimer.next.tv_usec / 1000 * 2)
    {
      nxwarn << "Loop: WARNING! Timer missed to expire at "
             << strMsTimestamp() << " in process with pid '"
             << getpid() << "'.\n" << std::flush;

      cerr << "Warning" << ": Timer missed to expire at "
           << strMsTimestamp() << " in process with pid '"
           << getpid() << "'.\n";

      HandleTimer(SIGALRM);
    }
    else
    {
      nxinfo << "Loop: Timer already running at "
             << strMsTimestamp() << " in process with pid '"
             << getpid() << "'.\n" << std::flush;

      return;
    }
  }

  //
  // Save the former handler.
  //

  struct sigaction action;

  memset(&action, 0, sizeof(action));

  action.sa_handler = HandleTimer;

  sigemptyset(&action.sa_mask);

  action.sa_flags = 0;

  sigaction(SIGALRM, &action, &lastTimer.action);

  //
  // Start the timer.
  //

  lastTimer.next = getTimestamp(value);

  struct itimerval timer;

  timer.it_interval = lastTimer.next;
  timer.it_value    = lastTimer.next;

  nxinfo << "Loop: Timer set to " << lastTimer.next.tv_sec
         << " s and " << lastTimer.next.tv_usec / 1000
         << " ms at " << strMsTimestamp() << " in process "
         << "with pid '" << getpid() << "'.\n"
         << std::flush;

  if (setitimer(ITIMER_REAL, &timer, &lastTimer.value) < 0)
  {
    nxfatal << "Loop: PANIC! Call to setitimer failed. "
            << "Error is " << EGET() << " '" << ESTR()
            << "'.\n" << std::flush;

    cerr << "Error" << ": Call to setitimer failed. "
         << "Error is " << EGET() << " '" << ESTR()
         << "'.\n";

    lastTimer.next = nullTimestamp();

    return;
  }

  lastTimer.start = getTimestamp();
}

void ResetTimer()
{
  if (isTimestamp(lastTimer.start) == 0)
  {
    nxinfo << "Loop: Timer not running in process "
           << "with pid '" << getpid() << "'.\n"
           << std::flush;

    return;
  }

  nxinfo << "Loop: Timer reset at " << strMsTimestamp()
         << " in process with pid '" << getpid()
         << "'.\n" << std::flush;

  //
  // Restore the old signal mask and timer.
  //

  if (setitimer(ITIMER_REAL, &lastTimer.value, NULL) < 0)
  {
    nxfatal << "Loop: PANIC! Call to setitimer failed. "
            << "Error is " << EGET() << " '" << ESTR()
            << "'.\n" << std::flush;

    cerr << "Error" << ": Call to setitimer failed. "
         << "Error is " << EGET() << " '" << ESTR()
         << "'.\n";
  }

  if (sigaction(SIGALRM, &lastTimer.action, NULL) < 0)
  {
    nxfatal << "Loop: PANIC! Call to sigaction failed. "
            << "Error is " << EGET() << " '" << ESTR()
            << "'.\n" << std::flush;

    cerr << "Error" << ": Call to sigaction failed. "
         << "Error is " << EGET() << " '" << ESTR()
         << "'.\n";
  }

  lastTimer.start = lastTimer.next = nullTimestamp();
}

//
// Open TCP or UNIX file socket to listen for remote proxy
// and block until remote connects. If successful close
// the listening socket and return FD on which the other
// party is connected.
//

int WaitForRemote(ChannelEndPoint &socketAddress)
{
  char hostLabel[DEFAULT_STRING_LENGTH] = { 0 };
  char *socketUri = NULL;

  int retryAccept  = -1;

  int pFD = -1;
  int newFD   = -1;

  int acceptIPAddr = 0;

  if (socketAddress.isTCPSocket())
  {

    //
    // Get IP address of host to be awaited.
    //

    if (*acceptHost != '\0')
    {
      acceptIPAddr = GetHostAddress(acceptHost);

      if (acceptIPAddr == 0)
      {
        nxfatal << "Loop: PANIC! Cannot accept connections from unknown host '"
                << acceptHost << "'.\n" << std::flush;

        cerr << "Error" << ": Cannot accept connections from unknown host '"
             << acceptHost << "'.\n";

        goto WaitForRemoteError;
      }
      snprintf(hostLabel, sizeof(hostLabel), "'%s'", acceptHost);
    }
    else
    {
      strcpy(hostLabel, "any host");
    }

    long _bindPort;
    if (socketAddress.getTCPHostAndPort(NULL, &_bindPort))
    {
      socketAddress.setSpec(loopbackBind ? "localhost" : "*", _bindPort);
    }
    else
    {
      // This should never happen
      cerr << "Error" << ": Unable to change bind host\n";
    }
  }
  else if (socketAddress.isUnixSocket())
    strcpy(hostLabel, "this host");
  else
    strcpy(hostLabel, "unknown origin (something went wrong!!!)");


  pFD = ListenConnection(socketAddress, "NX");

  SAFE_FREE(socketUri);

  socketAddress.getSpec(&socketUri);
  nxinfo << "Loop: Waiting for connection from "
         << hostLabel  << " on socket '" << socketUri
         << "'.\n" << std::flush;
  cerr << "Info" << ": Waiting for connection from "
       << hostLabel << " on socket '" << socketUri
       << "'.\n";
  SAFE_FREE(socketUri);

  //
  // How many times to loop waiting for connections
  // from the selected host? Each loop wait for at
  // most 20 seconds so a default value of 3 gives
  // a timeout of 1 minute.
  //
  // TODO: Handling of timeouts and retry attempts
  // must be rewritten.
  //

  retryAccept = control -> OptionProxyRetryAccept;

  for (;;)
  {
    fd_set readSet;

    FD_ZERO(&readSet);
    FD_SET(pFD, &readSet);

    T_timestamp selectTs;

    selectTs.tv_sec  = 20;
    selectTs.tv_usec = 0;

    int result = select(pFD + 1, &readSet, NULL, NULL, &selectTs);

    getNewTimestamp();

    if (result == -1)
    {
      if (EGET() == EINTR)
      {
        if (CheckAbort() != 0)
        {
          goto WaitForRemoteError;
        }

        continue;
      }

      nxfatal << "Loop: PANIC! Call to select failed. Error is "
              << EGET() << " '" << ESTR() << "'.\n"
              << std::flush;

      cerr << "Error" << ": Call to select failed. Error is "
           << EGET() << " '" << ESTR() << "'.\n";

      goto WaitForRemoteError;
    }
    else if (result > 0 && FD_ISSET(pFD, &readSet))
    {

      sockaddr_in newAddrINET;

      if (socketAddress.isUnixSocket())
      {
        socklen_t addrLen = sizeof(sockaddr_un);
        newFD = accept(pFD, NULL, &addrLen);
      }
      else if (socketAddress.isTCPSocket())
      {
        socklen_t addrLen = sizeof(sockaddr_in);
        newFD = accept(pFD, (sockaddr *) &newAddrINET, &addrLen);
      }
      if (newFD == -1)
      {
        nxfatal << "Loop: PANIC! Call to accept failed. Error is "
                << EGET() << " '" << ESTR() << "'.\n"
                << std::flush;

        cerr << "Error" << ": Call to accept failed. Error is "
             << EGET() << " '" << ESTR() << "'.\n";

        goto WaitForRemoteError;
      }

      if (socketAddress.isUnixSocket())
      {

        char * unixPath = NULL;
        socketAddress.getUnixPath(&unixPath);
        nxinfo << "Loop: Accepted connection from this host on Unix file socket '"
               << unixPath << "'.\n"
               << std::flush;

        cerr << "Info" << ": Accepted connection from this host on Unix file socket '"
             << unixPath << "'.\n";
        SAFE_FREE(unixPath);

        break;
      }
      else if (socketAddress.isTCPSocket())
      {

        char *connectedHost = inet_ntoa(newAddrINET.sin_addr);

        if (*acceptHost == '\0' || (int) newAddrINET.sin_addr.s_addr == acceptIPAddr)
        {


          unsigned int connectedPort = ntohs(newAddrINET.sin_port);
          nxinfo << "Loop: Accepted connection from '" << connectedHost
                 << "' with port '" << connectedPort << "'.\n"
                 << std::flush;

          cerr << "Info" << ": Accepted connection from '"
               << connectedHost << "'.\n";

          break;
        }
        else
        {
          nxfatal << "Loop: WARNING! Refusing connection from '" << connectedHost
                  << "' on port '" << socketAddress.getTCPPort() << "'.\n" << std::flush;

          cerr << "Warning" << ": Refusing connection from '"
               << connectedHost << "'.\n";
        }

        //
        // Not the best way to elude a DOS attack...
        //

        sleep(5);

        close(newFD);

      }

    }

    if (--retryAccept == 0)
    {
      if (socketAddress.isUnixSocket())
      {
        nxfatal << "Loop: PANIC! Connection via Unix file socket from this host "
                << "could not be established.\n"
                << std::flush;

        cerr << "Error" << ": Connection via Unix file socket from this host "
             << "could not be established.\n";
      }
      else if (*acceptHost == '\0')
      {
        nxfatal << "Loop: PANIC! Connection with remote host "
                << "could not be established.\n"
                << std::flush;

        cerr << "Error" << ": Connection with remote host "
             << "could not be established.\n";
      }
      else
      {
        nxfatal << "Loop: PANIC! Connection with remote host '"
                << acceptHost << "' could not be established.\n"
                << std::flush;

        cerr << "Error" << ": Connection with remote host '"
             << acceptHost << "' could not be established.\n";
      }

      goto WaitForRemoteError;
    }
    else
    {
      handleCheckSessionInConnect();
    }
  }

  close(pFD);

  return newFD;

WaitForRemoteError:

  close(pFD);

  HandleCleanup();
}

int PrepareProxyConnectionTCP(char** hostName, long int* portNum, int* timeout, int* proxyFileDescriptor, int* reason)
{

  if (!proxyFileDescriptor)
  {
    nxfatal << "Loop: PANIC! Implementation error (PrepareProxyConnectionTCP). "
            << "'proxyFileDescriptor' must not be a NULL pointer.\n" << std::flush;

    cerr << "Error" << ":  Implementation error (PrepareProxyConnectionTCP). "
            << "'proxyFileDescriptor' must not be a NULL pointer.\n";

    return -1;
  }

  if (!reason)
  {
    nxfatal << "Loop: PANIC! Implementation error (PrepareProxyConnectionTCP). "
            << "'reason' must not be a NULL pointer.\n" << std::flush;

    cerr << "Error" << ":  Implementation error (PrepareProxyConnectionTCP). "
            << "'reason' must not be a NULL pointer.\n";

    return -1;
  }

  int remoteIPAddr = GetHostAddress(*hostName);
  if (remoteIPAddr == 0)
  {
    nxfatal << "Loop: PANIC! Unknown remote host '"
            << *hostName << "'.\n" << std::flush;
        cerr << "Error" << ": Unknown remote host '"
         << *hostName << "'.\n";

    SAFE_FREE(*hostName);
    HandleCleanup();
  }

  nxinfo << "Loop: Connecting to remote host '"
         << *hostName << ":" << *portNum << "'.\n"
         << std::flush;

  cerr << "Info" << ": Connecting to remote host '"
       << *hostName << ":" << *portNum << "'.\n"
       << logofs_flush;

  *proxyFileDescriptor = -1;
  *reason = -1;

  sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(*portNum);
  addr.sin_addr.s_addr = remoteIPAddr;

  *proxyFileDescriptor = socket(AF_INET, SOCK_STREAM, PF_UNSPEC);
  *reason = EGET();

  if (*proxyFileDescriptor == -1)
  {
    nxfatal << "Loop: PANIC! Call to socket failed. "
            << "Error is " << *reason << " '" << ESTR()
            << "'.\n" << std::flush;

    cerr << "Error" << ": Call to socket failed. "
         << "Error is " << *reason << " '" << ESTR()
         << "'.\n";
    return -1;

  }
  else if (SetReuseAddress(*proxyFileDescriptor) < 0)
  {
    return -1;
  }

  //
  // Ensure operation is timed out
  // if there is a network problem.
  //

  if (timeout)
    SetTimer(*timeout);
  else
    SetTimer(20000);

  int result = connect(*proxyFileDescriptor, (sockaddr *) &addr, sizeof(sockaddr_in));

  *reason = EGET();

  ResetTimer();

  return result;

}

int PrepareProxyConnectionUnix(char** path, int* timeout, int* proxyFileDescriptor, int* reason)
{

  if (!proxyFileDescriptor)
  {
    nxfatal << "Loop: PANIC! Implementation error (PrepareProxyConnectionUnix). "
            << "proxyFileDescriptor must not be a NULL pointer.\n" << std::flush;

    cerr << "Error" << ":  Implementation error (PrepareProxyConnectionUnix). "
            << "proxyFileDescriptor must not be a NULL pointer.\n";

    return -1;
  }

  if (!reason)
  {
    nxfatal << "Loop: PANIC! Implementation error (PrepareProxyConnectionUnix). "
            << "'reason' must not be a NULL pointer.\n" << std::flush;

    cerr << "Error" << ":  Implementation error (PrepareProxyConnectionUnix). "
            << "'reason' must not be a NULL pointer.\n";

    return -1;
  }

  /* FIXME: Add socket file existence and permission checks */


  *proxyFileDescriptor = -1;
  *reason = -1;

  // determine the maximum number of characters that fit into struct
  // sockaddr_un's sun_path member
  const std::size_t sockpathlen =
    sizeof(struct sockaddr_un) - offsetof(struct sockaddr_un, sun_path);

  sockaddr_un addr;
  addr.sun_family = AF_UNIX;
  snprintf(addr.sun_path, sockpathlen, "%s", *path);

  *proxyFileDescriptor = socket(AF_UNIX, SOCK_STREAM, PF_UNSPEC);
  *reason = EGET();

  if (*proxyFileDescriptor == -1)
  {
    nxfatal << "Loop: PANIC! Call to socket failed. "
            << "Error is " << *reason << " '" << ESTR()
            << "'.\n" << std::flush;

    cerr << "Error" << ": Call to socket failed. "
         << "Error is " << *reason << " '" << ESTR()
         << "'.\n";

    return -1;
  }

  //
  // Ensure operation is timed out
  // if there is a network problem.
  //

  if (timeout)
    SetTimer(*timeout);
  else
    SetTimer(20000);

  int result = connect(*proxyFileDescriptor, (sockaddr *) &addr, sizeof(sockaddr_un));

  *reason = EGET();

  ResetTimer();

  return result;
}

//
// Connect to remote proxy. If successful
// return FD of connection, else return -1.
//

int ConnectToRemote(ChannelEndPoint &socketAddress)
{

  //
  // How many times we retry to connect to remote
  // host / Unix domain socket in case of failure?
  //

  int retryConnect = control -> OptionProxyRetryConnect;

  //
  // Show an alert after 20 seconds and use the
  // same timeout to interrupt the connect. The
  // retry timeout is incremental, starting from
  // 100 milliseconds up to 1 second.
  //

  int alertTimeout   = 20000;
  int connectTimeout = 20000;
  int retryTimeout   = 100;

  T_timestamp lastRetry = getNewTimestamp();

  int result = -1;
  int reason = -1;
  int pFD = -1;

  char *hostName = NULL;
  long int portNum = -1;
  char *unixPath = NULL;

  for (;;)
  {

    nxdbg << "Loop: Timer set to " << connectTimeout / 1000
          << " s " << "with retry set to " << retryConnect
          << " in process with pid '" << getpid()
          << "'.\n" << std::flush;

    SAFE_FREE(hostName);
    SAFE_FREE(unixPath);

    if (socketAddress.getUnixPath(&unixPath))
      result = PrepareProxyConnectionUnix(&unixPath, &connectTimeout, &pFD, &reason);
    else if (socketAddress.getTCPHostAndPort(&hostName, &portNum))
      result = PrepareProxyConnectionTCP(&hostName, &portNum, &connectTimeout, &pFD, &reason);

    if (result < 0)
    {
      close(pFD);

      if (CheckAbort() != 0)
      {
        goto ConnectToRemoteError;
      }
      else if (--retryConnect == 0)
      {
        ESET(reason);

        if (socketAddress.isUnixSocket())
        {
          nxfatal << "Loop: PANIC! Connection to Unix file socket '"
                  << unixPath << "' failed. Error is "
                  << EGET() << " '" << ESTR() << "'.\n"
                  << std::flush;

          cerr << "Error" << ": Connection to Unix file socket '"
                  << unixPath << "' failed. Error is "
                  << EGET() << " '" << ESTR() << "'.\n";
        }
        else
        {

          nxfatal << "Loop: PANIC! Connection to '" << hostName
                  << ":" << portNum << "' failed. Error is "
                  << EGET() << " '" << ESTR() << "'.\n"
                  << std::flush;

          cerr << "Error" << ": Connection to '" << hostName
               << ":" << portNum << "' failed. Error is "
               << EGET() << " '" << ESTR() << "'.\n";
        }
        goto ConnectToRemoteError;
      }
      else
      {
        nxinfo << "Loop: Sleeping " << retryTimeout
               << " ms before retrying.\n"
               << std::flush;

        usleep(retryTimeout * 1000);

        retryTimeout <<= 1;

        if (retryTimeout > 1000 * 1000)
        {
          retryTimeout = 1000 * 1000;
        }
      }

      //
      // Check if it is time to show an alert dialog.
      //

      if (diffTimestamp(lastRetry, getNewTimestamp()) >=
              (alertTimeout - control -> LatencyTimeout))
      {
        if (IsNotRunning(lastDialog))
        {
          handleCheckSessionInConnect();

          //
          // Wait for the dialog process to die
          // unless a signal is received.
          //

          while (IsRunning(lastDialog))
          {
            WaitChild(lastDialog, "dialog", 0);

            if (CheckAbort() != 0)
            {
              //
              // The client ignores the TERM signal
              // on Windows.
              //

              #if defined(__CYGWIN__) || defined(__CYGWIN32__)

              KillProcess(lastDialog, "dialog", SIGKILL, 1);

              #else

              KillProcess(lastDialog, "dialog", SIGTERM, 1);

              #endif

              goto ConnectToRemoteError;
            }
          }

          lastRetry = getTimestamp();
        }
      }
      {
        nxinfo << "Loop: Not showing the dialog with "
               << (diffTimestamp(lastRetry, getTimestamp()) / 1000)
               << " seconds elapsed.\n" << std::flush;
      }

      ESET(reason);

      if (unixPath && unixPath[0] != '\0' )
      {
        nxinfo << "Loop: Connection to Unix socket file '"
               << unixPath << "' failed with error '"
               << ESTR() << "'. Retrying.\n"
               << std::flush;
      }
      else
      {
        nxinfo << "Loop: Connection to '" << hostName
               << ":" << portNum << "' failed with error '"
               << ESTR() << "'. Retrying.\n"
               << std::flush;
      }
    }
    else
    {
      //
      // Connection was successful.
      //

      break;
    }
  }

  SAFE_FREE(unixPath);
  SAFE_FREE(hostName);

  return pFD;

ConnectToRemoteError:

  SAFE_FREE(unixPath);
  SAFE_FREE(hostName);

  if (pFD != -1)
  {
    close(pFD);
  }

  HandleCleanup();
}

//
// Make a string of options for the remote
// proxy and write it to the descriptor.
// The string includes the local version.
//

int SendProxyOptions(int fd)
{
  char options[DEFAULT_REMOTE_OPTIONS_LENGTH];

  //
  // Send the "compatibility" version first, then our
  // actual version. Old proxies will take the first
  // value and ignore the second.
  //

  sprintf(options, "NXPROXY-%s-%i.%i.%i",
              control -> NXPROXY_COMPATIBILITY_VERSION,
                  control -> LocalVersionMajor,
                      control -> LocalVersionMinor,
                          control -> LocalVersionPatch);

  //
  // If you want to send options from proxy
  // initiating the connection use something
  // like this:
  //
  // if (WE_PROVIDE_CREDENTIALS)
  // {
  //   sprintf(options + strlen(options), "%s=%s", option, value);
  // }
  //
  // If you want to send options according to
  // local proxy mode use something like this:
  //
  // if (control -> ProxyMode == proxy_client)
  // {
  //   sprintf(options + strlen(options), "%s=%s", option, value);
  // }
  //

  //
  // Send the authorization cookie if any. We assume
  // user can choose to not provide any auth cookie
  // and allow any connection to be accepted.
  //

  if (WE_PROVIDE_CREDENTIALS && *authCookie != '\0')
  {
    sprintf(options + strlen(options), " cookie=%s,", authCookie);
  }
  else
  {
    sprintf(options + strlen(options), " ");
  }

  //
  // Now link characteristics and compression
  // options. Delta compression, as well as
  // preferred pack method, are imposed by
  // client proxy.
  //

  if (control -> ProxyMode == proxy_client)
  {
    sprintf(options + strlen(options), "link=%s,pack=%s,cache=%s,",
                linkSpeedName, packMethodName, cacheSizeName);

    if (*bitrateLimitName != '\0')
    {
      sprintf(options + strlen(options), "limit=%s,",
                  bitrateLimitName);
    }

    //
    // Let the user disable the render extension
    // and let the X client proxy know if it can
    // short-circuit the X replies. Also pass
    // along the session type to ensure that the
    // remote proxy gets the right value.
    //

    sprintf(options + strlen(options), "render=%d,taint=%d,",
                (control -> HideRender == 0),
                    control -> TaintReplies);

    if (*sessionType != '\0')
    {
      sprintf(options + strlen(options), "type=%s,", sessionType);
    }
    else
    {
      sprintf(options + strlen(options), "type=default,");
    }

    //
    // Add the 'strict' option, if needed.
    //

    // Since ProtoStep7 (#issue 108)
    if (useStrict != -1)
    {
      sprintf(options + strlen(options), "strict=%d,", useStrict);
    }

    //
    // Tell the remote the size of the shared
    // memory segment.
    //

    // Since ProtoStep7 (#issue 108)
    if (*shsegSizeName != '\0')
    {
      sprintf(options + strlen(options), "shseg=%s,", shsegSizeName);
    }

    //
    // Send image cache parameters.
    //

    sprintf(options + strlen(options), "images=%s,", imagesSizeName);

    sprintf(options + strlen(options), "delta=%d,stream=%d,data=%d ",
                control -> LocalDeltaCompression,
                    control -> LocalStreamCompressionLevel,
                        control -> LocalDataCompressionLevel);
  }
  else
  {
    //
    // If no special compression level was selected,
    // server side will use compression levels set
    // by client.
    //

    if (control -> LocalStreamCompressionLevel < 0)
    {
      sprintf(options + strlen(options), "stream=default,");
    }
    else
    {
      sprintf(options + strlen(options), "stream=%d,",
                  control -> LocalStreamCompressionLevel);
    }

    if (control -> LocalDataCompressionLevel < 0)
    {
      sprintf(options + strlen(options), "data=default ");
    }
    else
    {
      sprintf(options + strlen(options), "data=%d ",
                  control -> LocalDataCompressionLevel);
    }
  }

  nxinfo << "Loop: Sending remote options '"
         << options << "'.\n" << std::flush;

  return WriteLocalData(fd, options, strlen(options));
}

int ReadProxyVersion(int fd)
{
  nxinfo << "Loop: Going to read the remote proxy version "
         << "from FD#" << fd << ".\n" << std::flush;

  //
  // Read until the first space in string.
  // We expect the remote version number.
  //

  char options[DEFAULT_REMOTE_OPTIONS_LENGTH];

  int result = ReadRemoteData(fd, options, sizeof(options), ' ');

  if (result <= 0)
  {
    if (result < 0)
    {
      if (control -> ProxyMode == proxy_server)
      {
        HandleAlert(ABORT_PROXY_NEGOTIATION_ALERT, 1);
      }

      handleAlertInLoop();
    }

    return result;
  }

  nxinfo << "Loop: Received remote version string '"
         << options << "' from FD#" << fd << ".\n"
         << std::flush;

  if (strncmp(options, "NXPROXY-", strlen("NXPROXY-")) != 0)
  {
    nxfatal << "Loop: PANIC! Parse error in remote options string '"
            << options << "'.\n" << std::flush;

    cerr << "Error" << ": Parse error in remote options string '"
         << options << "'.\n";

    return -1;
  }

  //
  // Try to determine if this is a pre-2.0.0
  // version advertising itself as compatible
  // with the 1.2.2.
  //

  int major = -1;
  int minor = -1;
  int patch = -1;

  sscanf(options, "NXPROXY-%i.%i.%i-%i.%i.%i", &(control -> RemoteVersionMajor),
             &(control -> RemoteVersionMinor), &(control -> RemoteVersionPatch),
                 &major, &minor, &patch);

  if (control -> RemoteVersionMajor == 1 &&
          control -> RemoteVersionMinor == 2 &&
              control -> RemoteVersionPatch == 2 &&
                  major != -1 && minor != -1 && patch != -1)
  {
    nxinfo << "Loop: Read trailing remote version '" << major
           << "." << minor << "." << patch << "'.\n"
           << std::flush;

    control -> CompatVersionMajor = major;
    control -> CompatVersionMinor = minor;
    control -> CompatVersionPatch = patch;

    control -> RemoteVersionMajor = major;
    control -> RemoteVersionMinor = minor;
    control -> RemoteVersionPatch = patch;
  }
  else
  {
    //
    // We read the remote version at the first
    // round. If the second version is missing,
    // we will retain the values read before.
    //

    sscanf(options, "NXPROXY-%i.%i.%i-%i.%i.%i", &(control -> CompatVersionMajor),
               &(control -> CompatVersionMinor), &(control -> CompatVersionPatch),
                   &(control -> RemoteVersionMajor), &(control -> RemoteVersionMinor),
                       &(control -> RemoteVersionPatch));
  }

  *logofs << "Loop: Identified remote version '" << control -> RemoteVersionMajor
          << "." << control -> RemoteVersionMinor << "." << control -> RemoteVersionPatch
          << "'.\n" << logofs_flush;

  *logofs << "Loop: Remote compatibility version '" << control -> CompatVersionMajor
          << "." << control -> CompatVersionMinor << "." << control -> CompatVersionPatch
          << "'.\n" << logofs_flush;

  *logofs << "Loop: Local version '" << control -> LocalVersionMajor
          << "." << control -> LocalVersionMinor << "." << control -> LocalVersionPatch
          << "'.\n" << logofs_flush;

  if (SetVersion() < 0)
  {
    if (control -> ProxyMode == proxy_server)
    {
      HandleAlert(WRONG_PROXY_VERSION_ALERT, 1);
    }

    handleAlertInLoop();

    return -1;
  }

  return 1;
}

int ReadProxyOptions(int fd)
{
  nxinfo << "Loop: Going to read the remote proxy options "
         << "from FD#" << fd << ".\n" << std::flush;

  char options[DEFAULT_REMOTE_OPTIONS_LENGTH];

  int result = ReadRemoteData(fd, options, sizeof(options), ' ');

  if (result <= 0)
  {
    return result;
  }

  nxinfo << "Loop: Received remote options string '"
         << options << "' from FD#" << fd << ".\n"
         << std::flush;

  //
  // Get the remote options, delimited by a space character.
  // Note that there will be a further initialization phase
  // at the time proxies negotiate cache file to restore.
  //

  if (ParseRemoteOptions(options) < 0)
  {
    nxfatal << "Loop: PANIC! Couldn't negotiate a valid "
            << "session with remote NX proxy.\n"
            << std::flush;

    cerr << "Error" << ": Couldn't negotiate a valid "
         << "session with remote NX proxy.\n";

    return -1;
  }

  return 1;
}

int SendProxyCaches(int fd)
{
  nxinfo << "Loop: Synchronizing local and remote caches.\n"
         << std::flush;

  if (control -> ProxyMode == proxy_client)
  {
    //
    // Prepare a list of caches matching this
    // session type and send it to the remote.
    //

    nxinfo << "Loop: Going to send the list of local caches.\n"
           << std::flush;

    SetCaches();

    int entries = DEFAULT_REMOTE_CACHE_ENTRIES;

    const char prefix = 'C';

    if (control -> LocalDeltaCompression == 0 ||
            control -> PersistentCacheEnableLoad == 0)
    {
      nxinfo << "Loop: Writing an empty list to FD#" << fd
             << ".\n" << std::flush;

      return WriteLocalData(fd, "cachelist=none ", strlen("cachelist=none "));
    }

    int count = 0;

    nxinfo << "Loop: Looking for cache files in directory '"
           << control -> PersistentCachePath << "'.\n" << std::flush;

    DIR *cacheDir = opendir(control -> PersistentCachePath);

    if (cacheDir != NULL)
    {
      dirent *dirEntry;

      int prologue = 0;

      while (((dirEntry = readdir(cacheDir)) != NULL) && (count < entries))
      {
        if (*dirEntry -> d_name == prefix &&
                strlen(dirEntry -> d_name) == (MD5_LENGTH * 2 + 2))
        {
          if (prologue == 0)
          {
            WriteLocalData(fd, "cachelist=", strlen("cachelist="));

            prologue = 1;
          }
          else
          {
            WriteLocalData(fd, ",", strlen(","));
          }

          nxinfo << "Loop: Writing entry '" << control -> PersistentCachePath
                 << "/" << dirEntry -> d_name << "' to FD#" << fd
                 << ".\n" << std::flush;

          //
          // Write cache file name to the socket,
          // including leading 'C-' or 'S-'.
          //

          WriteLocalData(fd, dirEntry -> d_name, MD5_LENGTH * 2 + 2);

          count++;
        }
      }

      closedir(cacheDir);
    }

    if (count == 0)
    {
      nxinfo << "Loop: Writing an empty list to FD#" << fd
             << ".\n" << std::flush;

      return WriteLocalData(fd, "cachelist=none ", strlen("cachelist=none "));
    }
    else
    {
      return WriteLocalData(fd, " ", 1);
    }
  }
  else
  {
    //
    // Send back the selected cache name.
    //

    nxinfo << "Loop: Going to send the selected cache.\n"
           << std::flush;

    char buffer[DEFAULT_STRING_LENGTH];

    if (control -> PersistentCacheName != NULL)
    {
      nxinfo << "Loop: Name of selected cache file is '"
             << control -> PersistentCacheName << "'.\n"
             << std::flush;

      sprintf(buffer, "cachefile=%s%s ",
                  *(control -> PersistentCacheName) == 'C' ? "S-" : "C-",
                      control -> PersistentCacheName + 2);
    }
    else
    {
      nxinfo << "Loop: No valid cache file was selected.\n"
             << std::flush;

      sprintf(buffer, "cachefile=none ");
    }

    nxinfo << "Loop: Sending string '" << buffer
           << "' as selected cache file.\n"
           << std::flush;

    return WriteLocalData(fd, buffer, strlen(buffer));
  }
}

int ReadProxyCaches(int fd)
{
  if (control -> ProxyMode == proxy_client)
  {
    nxinfo << "Loop: Going to receive the selected proxy cache.\n"
           << std::flush;

    //
    // We will read the name of cache plus the stop character.
    //

    char buffer[DEFAULT_STRING_LENGTH];

    //
    // Leave space for a trailing null.
    //

    int result = ReadRemoteData(fd, buffer, sizeof("cachefile=") + MD5_LENGTH * 2 + 3, ' ');

    if (result <= 0)
    {
      return result;
    }

    char *cacheName = strstr(buffer, "cachefile=");

    if (cacheName == NULL)
    {
      nxfatal << "Loop: PANIC! Invalid cache file option '"
              << buffer << "' provided by remote proxy.\n"
              << std::flush;

      cerr << "Error" << ": Invalid cache file option '"
           << buffer << "' provided by remote proxy.\n";

      HandleCleanup();
    }

    cacheName += strlen("cachefile=");

    if (control -> PersistentCacheName != NULL)
    {
      delete [] control -> PersistentCacheName;
    }

    control -> PersistentCacheName = NULL;

    if (strncasecmp(cacheName, "none", strlen("none")) == 0)
    {
      nxinfo << "Loop: No cache file selected by remote proxy.\n"
             << std::flush;
    }
    else if (strlen(cacheName) != MD5_LENGTH * 2 + 3 ||
                 *(cacheName + MD5_LENGTH * 2 + 2) != ' ')
    {
      nxfatal << "Loop: PANIC! Invalid cache file name '"
              << cacheName << "' provided by remote proxy.\n"
              << std::flush;

      cerr << "Error" << ": Invalid cache file name '"
           << cacheName << "' provided by remote proxy.\n";

      HandleCleanup();
    }
    else
    {
      //
      // It is "C-" + 32 + "\0".
      //

      control -> PersistentCacheName = new char[MD5_LENGTH * 2 + 3];

      *(cacheName + MD5_LENGTH * 2 + 2) = '\0';

      strcpy(control -> PersistentCacheName, cacheName);

      nxinfo << "Loop: Cache file '" << control -> PersistentCacheName
             << "' selected by remote proxy.\n" << std::flush;
    }
  }
  else
  {
    nxinfo << "Loop: Going to receive the list of remote caches.\n"
           << std::flush;

    SetCaches();

    int size = ((MD5_LENGTH * 2 + 2) + strlen(",")) * DEFAULT_REMOTE_CACHE_ENTRIES +
                   strlen("cachelist=") + strlen(" ") + 1;

    char *buffer = new char[size];

    int result = ReadRemoteData(fd, buffer, size - 1, ' ');

    if (result <= 0)
    {
      delete [] buffer;

      return result;
    }

    nxinfo << "Loop: Read list of caches from remote side as '"
           << buffer << "'.\n" << std::flush;

    //
    // Prepare the buffer. What we want is a list
    // like "cache1,cache2,cache2" terminated by
    // null.
    //

    *(buffer + strlen(buffer) - 1) = '\0';

    if (strncasecmp(buffer, "cachelist=", strlen("cachelist=")) != 0)
    {
      nxfatal << "Loop: Wrong format for list of cache files "
              << "read from FD#" << fd << ".\n" << std::flush;

      cerr << "Error" << ": Wrong format for list of cache files.\n";

      delete [] buffer;

      return -1;
    }

    control -> PersistentCacheName = GetLastCache(buffer, control -> PersistentCachePath);

    //
    // Get rid of list of caches.
    //

    delete [] buffer;
  }

  return 1;
}

int ReadForwarderVersion(int fd)
{
  nxinfo << "Loop: Going to negotiate the forwarder version.\n"
         << std::flush;

  //
  // Check if we actually expect the session cookie.
  //

  if (*authCookie == '\0')
  {
    nxinfo << "Loop: No authentication cookie required "
           << "from FD#" << fd << ".\n" << std::flush;

    return 1;
  }

  char options[DEFAULT_REMOTE_OPTIONS_LENGTH];

  int result = ReadRemoteData(fd, options, sizeof(options), ' ');

  if (result <= 0)
  {
    return result;
  }

  nxinfo << "Loop: Received forwarder version string '" << options
         << "' from FD#" << fd << ".\n" << std::flush;

  if (strncmp(options, "NXSSH-", strlen("NXSSH-")) != 0)
  {
    nxfatal << "Loop: PANIC! Parse error in forwarder options string '"
            << options << "'.\n" << std::flush;

    cerr << "Error" << ": Parse error in forwarder options string '"
         << options << "'.\n";

    return -1;
  }

  //
  // Accept whatever forwarder version.
  //

  sscanf(options, "NXSSH-%i.%i.%i", &(control -> RemoteVersionMajor),
             &(control -> RemoteVersionMinor), &(control -> RemoteVersionPatch));

  nxinfo << "Loop: Read forwarder version '" << control -> RemoteVersionMajor
         << "." << control -> RemoteVersionMinor << "." << control -> RemoteVersionPatch
         << "'.\n" << std::flush;

  return 1;
}

int ReadForwarderOptions(int fd)
{
  //
  // Get the forwarder cookie.
  //

  if (*authCookie == '\0')
  {
    nxinfo << "Loop: No authentication cookie required "
           << "from FD#" << fd << ".\n" << std::flush;

    return 1;
  }

  char options[DEFAULT_REMOTE_OPTIONS_LENGTH];

  int result = ReadRemoteData(fd, options, sizeof(options), ' ');

  if (result <= 0)
  {
    return result;
  }

  nxinfo << "Loop: Received forwarder options string '"
         << options << "' from FD#" << fd << ".\n"
         << std::flush;

  if (ParseForwarderOptions(options) < 0)
  {
    nxfatal << "Loop: PANIC! Couldn't negotiate a valid "
            << "cookie with the NX forwarder.\n"
            << std::flush;

    cerr << "Error" << ": Couldn't negotiate a valid "
         << "cookie with the NX forwarder.\n";

    return -1;
  }

  return 1;
}

int ReadRemoteData(int fd, char *buffer, int size, char stop)
{
  nxinfo << "Loop: Going to read remote data from FD#"
         << fd << ".\n" << std::flush;

  if (size >= MAXIMUM_REMOTE_OPTIONS_LENGTH)
  {
    nxfatal << "Loop: PANIC! Maximum remote options buffer "
            << "limit exceeded.\n" << std::flush;

    cerr << "Error" << ": Maximum remote options buffer "
         << "limit exceeded.\n";

    HandleCleanup();
  }

  while (remotePosition < (size - 1))
  {
    int result = read(fd, remoteData + remotePosition, 1);

    getNewTimestamp();

    if (result <= 0)
    {
      if (result == -1)
      {
        if (EGET() == EAGAIN)
        {
          nxinfo << "Loop: Reading data from FD#" << fd
                 << " would block.\n" << std::flush;

          return 0;
        }
        else if (EGET() == EINTR)
        {
          if (CheckAbort() != 0)
          {
            return -1;
          }

          continue;
        }
      }

      nxfatal << "Loop: PANIC! The remote NX proxy closed "
              << "the connection.\n" << std::flush;

      cerr << "Error" << ": The remote NX proxy closed "
           << "the connection.\n";

      return -1;
    }
    else if (*(remoteData + remotePosition) == stop)
    {
      nxinfo << "Loop: Read stop character from FD#"
             << fd << ".\n" << std::flush;

      remotePosition++;

      //
      // Copy the fake terminating null
      // in the buffer.
      //

      *(remoteData + remotePosition) = '\0';

      memcpy(buffer, remoteData, remotePosition + 1);

      nxinfo << "Loop: Remote string '" << remoteData
             << "' read from FD#" << fd << ".\n"
             << std::flush;

      int t = remotePosition;

      remotePosition = 0;

      return t;
    }
    else
    {
      //
      // Make sure string received
      // from far end is printable.
      //

      if (isgraph(*(remoteData + remotePosition)) == 0)
      {
        nxwarn << "Loop: WARNING! Non printable character decimal '"
               << (unsigned int) *(remoteData + remotePosition)
               << "' received in remote data from FD#"
               << fd << ".\n" << std::flush;

        cerr << "Warning" << ": Non printable character decimal '"
                << (unsigned int) *(remoteData + remotePosition)
                << "' received in remote data from FD#"
                << fd << ".\n" << logofs_flush;

        *(remoteData + remotePosition) = ' ';
      }

      nxdbg << "Loop: Read a further character "
            << "from FD#" << fd << ".\n"
            << std::flush;

      remotePosition++;
    }
  }

  *(remoteData + remotePosition) = '\0';

  nxfatal << "Loop: PANIC! Stop character missing "
          << "from FD#" << fd << " after " << remotePosition
          << " characters read in string '" << remoteData
          << "'.\n" << std::flush;

  cerr << "Error" << ": Stop character missing "
       << "from FD#" << fd << " after " << remotePosition
       << " characters read in string '" << remoteData
       << "'.\n";

  memcpy(buffer, remoteData, remotePosition);

  remotePosition = 0;

  return -1;
}

static int
hexval(char c) {
  if ((c >= '0') && (c <= '9'))
    return c - '0';
  if ((c >= 'a') && (c <= 'f'))
    return c - 'a' + 10;
  if ((c >= 'A') && (c <= 'F'))
    return c - 'A' + 10;
  return -1;
}

static void
URLDecodeInPlace(char *str) {
  if (str) {
    char *to = str;
    while (str[0]) {
      if ((str[0] == '%') &&
          (hexval(str[1]) >= 0) &&
          (hexval(str[2]) >= 0)) {
        *(to++) = hexval(str[1]) * 16 + hexval(str[2]);
        str += 3;
      }
      else
        *(to++) = *(str++);
    }
    *to = '\0';
  }
}

int WriteLocalData(int fd, const char *buffer, int size)
{
  int position = 0;
  int ret = 0;
  fd_set writeSet;
  struct timeval selectTs = {30, 0};

  while (position < size)
  {

    // A write to a non-blocking socket may fail with EAGAIN. The problem is
    // that cache data is done in several writes, and there's no easy way
    // to handle failure without rewriting a significant amount of code.
    //
    // Bailing out of the outer loop would result in restarting the sending
    // of the entire cache list, which would confuse the other side.

    FD_ZERO(&writeSet);
    FD_SET(fd, &writeSet);

    ret = select(fd+1, NULL, &writeSet, NULL, &selectTs);

    nxdbg << "Loop: WriteLocalData: select() returned with a code of " << ret << " and remaining timeout of "
          << selectTs.tv_sec << " sec, " << selectTs.tv_usec << "usec\n" << std::flush;

    if ( ret < 0 )
    {
      *logofs << "Loop: Error in select() when writing data to FD#" << fd << ": " << strerror(EGET()) << "\n" << logofs_flush;

      if ( EGET() == EINTR )
        continue;

      return -1;
    }
    else if ( ret == 0 )
    {
      *logofs << "Loop: Timeout expired in select() when writing data to FD#" << fd << ": " << strerror(EGET()) << "\n" << logofs_flush;
      return -1;
    }

    int result = write(fd, buffer + position, size - position);

    getNewTimestamp();

    if (result <= 0)
    {
      if (result < 0 && (EGET() == EINTR || EGET() == EAGAIN || EGET() == EWOULDBLOCK))
      {
        continue;
      }

      nxinfo << "Loop: Error writing data to FD#"
             << fd << ".\n" << std::flush;

      return -1;
    }

    position += result;
  }

  return position;
}

//
// Parse the string passed by calling process in
// the environment. This is not necessarily the
// content of DISPLAY variable, but can be the
// parameters passed when creating the process
// or thread.
//

int ParseEnvironmentOptions(const char *env, int force)
{
  //
  // Be sure log file is valid.
  //

  if (logofs == NULL)
  {
    logofs = &cerr;
  }

  //
  // Be sure we have a parameters repository
  // and a context to jump into because this
  // can be called before creating the proxy.
  //

  if (control == NULL)
  {
    control = new Control();
  }

  if (setjmp(context) == 1)
  {
    nxinfo << "Loop: Out of the long jump while parsing "
           << "the environment options.\n"
           << std::flush;

    return -1;
  }

  if (force == 0 && parsedOptions == 1)
  {
    nxinfo << "Loop: Skipping a further parse of environment "
           << "options string '" << (env != NULL ? env : "")
           << "'.\n" << std::flush;

    return 1;
  }

  if (env == NULL || *env == '\0')
  {
    nxinfo << "Loop: Nothing to do with empty environment "
           << "options string '" << (env != NULL ? env : "")
           << "'.\n" << std::flush;

    return 0;
  }

  nxinfo << "Loop: Going to parse the environment options "
         << "string '" << env << "'.\n"
         << std::flush;

  parsedOptions = 1;

  //
  // Copy the string passed as parameter
  // because we need to modify it.
  //

  char opts[DEFAULT_DISPLAY_OPTIONS_LENGTH];

  #ifdef VALGRIND

  memset(opts, '\0', DEFAULT_DISPLAY_OPTIONS_LENGTH);

  #endif

  if (strlen(env) >= DEFAULT_DISPLAY_OPTIONS_LENGTH)
  {
    nxfatal << "Loop: PANIC! Environment options string '" << env
            << "' exceeds length of " << DEFAULT_DISPLAY_OPTIONS_LENGTH
            << " characters.\n" << std::flush;

    cerr << "Error" << ": Environment options string '" << env
         << "' exceeds length of " << DEFAULT_DISPLAY_OPTIONS_LENGTH
         << " characters.\n";

    return -1;
  }

  strcpy(opts, env);

  char *nextOpts = opts;

  //
  // Ensure that DISPLAY environment variable
  // (roughly) follows the X convention for
  // transport notation.
  //

  if (strncasecmp(opts, "nx/nx,:", 7) == 0 ||
          strncasecmp(opts, "nx,:", 4) == 0)
  {
    nxfatal << "Loop: PANIC! Parse error in options string '"
            << opts << "' at 'nx,:'.\n" << std::flush;

    cerr << "Error" << ": Parse error in options string '"
         << opts << "' at 'nx,:'.\n";

    return -1;
  }
  else if (strncasecmp(opts, "nx/nx,", 6) == 0)
  {
    nextOpts += 6;
  }
  else if (strncasecmp(opts, "nx,", 3) == 0)
  {
    nextOpts += 3;
  }
  else if (strncasecmp(opts, "nx:", 3) == 0)
  {
    nextOpts += 3;
  }
  else if (force == 0)
  {
    nxinfo << "Loop: Ignoring host X server display string '"
           << opts << "'.\n" << std::flush;

    return 0;
  }

  //
  // Save here the name of the options file and
  // parse it after all the other options.
  //

  char fileOptions[DEFAULT_STRING_LENGTH] = { 0 };

  //
  // The options string is intended to be a series
  // of name/value tuples in the form name=value
  // separated by the ',' character ended by a ':'
  // followed by remote NX proxy port.
  //

  char *name;
  char *value;

  value = strrchr(nextOpts, ':');

  if (value != NULL)
  {
    char *check = value + 1;

    if (*check == '\0' || isdigit(*check) == 0)
    {
      nxfatal << "Loop: PANIC! Can't identify NX port in string '"
              << value << "'.\n" << std::flush;

      cerr << "Error" << ": Can't identify NX port in string '"
           << value << "'.\n";

      return -1;
    }

    proxyPort = atoi(check);

    //
    // Get rid of the port specification.
    //

    *value = '\0';
  }
  else if (proxyPort == DEFAULT_NX_PROXY_PORT && force == 0)
  {
    //
    // Complain only if user didn't specify
    // the port on the command line.
    //

    nxfatal << "Loop: PANIC! Can't identify NX port in string '"
            << opts << "'.\n" << std::flush;

    cerr << "Error" << ": Can't identify NX port in string '"
         << opts << "'.\n";

    return -1;
  }

  nxinfo << "Loop: Parsing options string '"
         << nextOpts << "'.\n" << std::flush;

  //
  // Now all the other optional parameters.
  //

  name = strtok(nextOpts, "=");

  char connectHost[DEFAULT_STRING_LENGTH] = { 0 };
  long connectPort = -1;

  while (name)
  {
    value = strtok(NULL, ",");
    URLDecodeInPlace(value);

    if (CheckArg("environment", name, value) < 0)
    {
      return -1;
    }

    if (strcasecmp(name, "options") == 0)
    {
      snprintf(fileOptions, DEFAULT_STRING_LENGTH, "%s", value);
    }
    else if (strcasecmp(name, "display") == 0)
    {
      snprintf(displayHost, DEFAULT_STRING_LENGTH, "%s", value);
    }
    else if (strcasecmp(name, "link") == 0)
    {

      if (control -> ProxyMode == proxy_server)
      {
        PrintOptionIgnored("local", name, value);
      }
      else if (ParseLinkOption(value) < 0)
      {
        nxfatal << "Loop: PANIC! Can't identify 'link' option in string '"
                << value << "'.\n" << std::flush;

        cerr << "Error" << ": Can't identify 'link' option in string '"
             << value << "'.\n";
        if (ParseLinkOption("adsl") < 0)
           return -1;
      }
    }
    else if (strcasecmp(name, "limit") == 0)
    {
      if (control -> ProxyMode == proxy_server)
      {
        PrintOptionIgnored("local", name, value);
      }
      else if (ParseBitrateOption(value) < 0)
      {
        nxfatal << "Loop: PANIC! Can't identify option 'limit' in string '"
                << value << "'.\n" << std::flush;

        cerr << "Error" << ": Can't identify option 'limit' in string '"
             << value << "'.\n";

        return -1;
      }
    }
    else if (strcasecmp(name, "type") == 0)
    {
      //
      // Type of session, for example "desktop",
      // "application", "windows", etc.
      //

      if (control -> ProxyMode == proxy_server)
      {
        PrintOptionIgnored("local", name, value);
      }
      else
      {
        if (strcasecmp(value, "default") == 0)
        {
          *sessionType = '\0';
        }
        else
        {
          snprintf(sessionType, DEFAULT_STRING_LENGTH, "%s", value);
        }
      }
    }
    else if (strcasecmp(name, "listen") == 0)
    {
      char *socketUri = NULL;
      if (connectSocket.getSpec(&socketUri))
      {
        nxfatal << "Loop: PANIC! Can't handle 'listen' and 'connect' parameters "
                << "at the same time.\n" << std::flush;

        nxfatal << "Loop: PANIC! Refusing 'listen' parameter with 'connect' being '"
                << socketUri << "'.\n" << std::flush;

        cerr << "Error" << ": Can't handle 'listen' and 'connect' parameters "
             << "at the same time.\n";

        cerr << "Error" << ": Refusing 'listen' parameter with 'connect' being '"
             << socketUri << "'.\n";

        SAFE_FREE(socketUri);
        return -1;
      }

      SetAndValidateChannelEndPointArg("local", name, value, listenSocket);

    }
    else if (strcasecmp(name, "loopback") == 0)
    {
      loopbackBind = ValidateArg("local", name, value);
    }
    else if (strcasecmp(name, "accept") == 0)
    {
      char *socketUri = NULL;
      if (connectSocket.getSpec(&socketUri))
      {
        nxfatal << "Loop: PANIC! Can't handle 'accept' and 'connect' parameters "
                << "at the same time.\n" << std::flush;

        nxfatal << "Loop: PANIC! Refusing 'accept' parameter with 'connect' being '"
                << socketUri << "'.\n" << std::flush;

        cerr << "Error" << ": Can't handle 'accept' and 'connect' parameters "
             << "at the same time.\n";

        cerr << "Error" << ": Refusing 'accept' parameter with 'connect' being '"
             << socketUri << "'.\n";

        SAFE_FREE(socketUri);
        return -1;
      }

      snprintf(acceptHost, DEFAULT_STRING_LENGTH, "%s", value);
    }
    else if (strcasecmp(name, "connect") == 0)
    {
      if (*acceptHost != '\0')
      {
        nxfatal << "Loop: PANIC! Can't handle 'connect' and 'accept' parameters "
                << "at the same time.\n" << std::flush;

        nxfatal << "Loop: PANIC! Refusing 'connect' parameter with 'accept' being '"
                << acceptHost << "'.\n" << std::flush;

        cerr << "Error" << ": Can't handle 'connect' and 'accept' parameters "
             << "at the same time.\n";

        cerr << "Error" << ": Refusing 'connect' parameter with 'accept' being '"
             << acceptHost << "'.\n";

        return -1;
      }
      if ((strncmp(value, "tcp:", 4) == 0) || (strncmp(value, "unix:", 5) == 0))
        SetAndValidateChannelEndPointArg("local", name, value, connectSocket);
      else
        // if the "connect" parameter does not start with "unix:" or "tcp:" assume
        // old parameter usage style (providing hostname string only).
        strcpy(connectHost, value);
    }
    else if (strcasecmp(name, "port") == 0)
    {
      connectPort = ValidateArg("local", name, value);
    }
    else if (strcasecmp(name, "retry") == 0)
    {
      control -> OptionProxyRetryConnect  = ValidateArg("local", name, value);
      control -> OptionServerRetryConnect = ValidateArg("local", name, value);
    }
    else if (strcasecmp(name, "session") == 0)
    {
      snprintf(sessionFileName, DEFAULT_STRING_LENGTH, "%s", value);
    }
    else if (strcasecmp(name, "errors") == 0)
    {
      //
      // The old name of the parameter was 'log'
      // but the default name for the file is
      // 'errors' so it is more logical to use
      // the same name.
      //

      snprintf(errorsFileName, DEFAULT_STRING_LENGTH, "%s", value);
    }
    else if (strcasecmp(name, "root") == 0)
    {
      snprintf(rootDir, DEFAULT_STRING_LENGTH, "%s", value);
    }
    else if (strcasecmp(name, "id") == 0)
    {
      snprintf(sessionId, DEFAULT_STRING_LENGTH, "%s", value);
    }
    else if (strcasecmp(name, "stats") == 0)
    {
      control -> EnableStatistics = 1;

      snprintf(statsFileName, DEFAULT_STRING_LENGTH, "%s", value);
    }
    else if (strcasecmp(name, "cookie") == 0)
    {
      LowercaseArg("local", name, value);

      snprintf(authCookie, DEFAULT_STRING_LENGTH, "%s", value);
    }
    else if (strcasecmp(name, "nodelay") == 0)
    {
      useNoDelay = ValidateArg("local", name, value);
    }
    else if (strcasecmp(name, "policy") == 0)
    {
      if (control -> ProxyMode == proxy_server)
      {
        PrintOptionIgnored("local", name, value);
      }
      else
      {
        usePolicy = ValidateArg("local", name, value);
      }
    }
    else if (strcasecmp(name, "render") == 0)
    {
      if (control -> ProxyMode == proxy_server)
      {
        PrintOptionIgnored("local", name, value);
      }
      else
      {
        useRender = ValidateArg("local", name, value);
      }
    }
    else if (strcasecmp(name, "taint") == 0)
    {
      if (control -> ProxyMode == proxy_server)
      {
        PrintOptionIgnored("local", name, value);
      }
      else
      {
        useTaint = ValidateArg("local", name, value);
      }
    }
    else if (strcasecmp(name, "delta") == 0)
    {
      if (control -> ProxyMode == proxy_server)
      {
        PrintOptionIgnored("local", name, value);
      }
      else
      {
        control -> LocalDeltaCompression = ValidateArg("local", name, value);
      }
    }
    else if (strcasecmp(name, "data") == 0)
    {
      control -> LocalDataCompressionLevel = ValidateArg("local", name, value);

      if (control -> LocalDataCompressionLevel == 0)
      {
        control -> LocalDataCompression = 0;
      }
      else
      {
        control -> LocalDataCompression = 1;
      }
    }
    else if (strcasecmp(name, "stream") == 0)
    {
      control -> LocalStreamCompressionLevel = ValidateArg("local", name, value);

      if (control -> LocalStreamCompressionLevel == 0)
      {
        control -> LocalStreamCompression = 0;
      }
      else
      {
        control -> LocalStreamCompression = 1;
      }
    }
    else if (strcasecmp(name, "memory") == 0)
    {
      control -> LocalMemoryLevel = ValidateArg("local", name, value);
    }
    else if (strcasecmp(name, "cache") == 0)
    {
      if (control -> ProxyMode == proxy_server)
      {
        PrintOptionIgnored("local", name, value);
      }
      else if (ParseCacheOption(value) < 0)
      {
        nxfatal << "Loop: PANIC! Can't identify cache size for string '"
                << value << "'.\n" << std::flush;

        cerr << "Error" << ": Can't identify cache size for string '"
             << value << "'.\n";

        return -1;
      }
    }
    else if (strcasecmp(name, "images") == 0)
    {
      if (control -> ProxyMode == proxy_server)
      {
        PrintOptionIgnored("local", name, value);
      }
      else if (ParseImagesOption(value) < 0)
      {
        nxfatal << "Loop: PANIC! Can't identify images cache size for string '"
                << value << "'.\n" << std::flush;

        cerr << "Error" << ": Can't identify images cache size for string '"
             << value << "'.\n";

        return -1;
      }
    }
    else if (strcasecmp(name, "shseg") == 0)
    {
      //
      // The 'shmem' option is used by the agent, together
      // with 'shpix' literal. We make the 'shseg' option
      // specific to the proxy and use it to determine the
      // size of the shared memory segment, or otherwise 0,
      // if the use of the shared memory extension should
      // not be enabled on the real X server.
      //

      if (control -> ProxyMode == proxy_server)
      {
        PrintOptionIgnored("local", name, value);
      }
      else if (ParseShmemOption(value) < 0)
      {
        nxfatal << "Loop: PANIC! Can't identify size of shared memory "
                << "segment in string '" << value << "'.\n"
                << std::flush;

        cerr << "Error" << ": Can't identify size of shared memory "
             << "segment in string '" << value << "'.\n";

        return -1;
      }
    }
    else if (strcasecmp(name, "load") == 0)
    {
      if (control -> ProxyMode == proxy_server)
      {
        PrintOptionIgnored("local", name, value);
      }
      else
      {
        control -> PersistentCacheEnableLoad = ValidateArg("local", name, value);

        if (control -> PersistentCacheEnableLoad > 0)
        {
          control -> PersistentCacheEnableLoad = 1;
        }
        else
        {
          if (control -> PersistentCacheName != NULL)
          {
            delete [] control -> PersistentCacheName;
          }

          control -> PersistentCacheName = NULL;

          control -> PersistentCacheEnableLoad = 0;
        }
      }
    }
    else if (strcasecmp(name, "save") == 0)
    {
      if (control -> ProxyMode == proxy_server)
      {
        PrintOptionIgnored("local", name, value);
      }
      else
      {
        control -> PersistentCacheEnableSave = ValidateArg("local", name, value);

        if (control -> PersistentCacheEnableSave > 0)
        {
          control -> PersistentCacheEnableSave = 1;
        }
        else
        {
          if (control -> PersistentCacheName != NULL)
          {
            delete [] control -> PersistentCacheName;
          }

          control -> PersistentCacheName = NULL;

          control -> PersistentCacheEnableSave = 0;
        }
      }
    }
    else if (strcasecmp(name, "cups") == 0)
    {
      SetAndValidateChannelEndPointArg("local", name, value, cupsPort);
    }
    else if (strcasecmp(name, "sync") == 0)
    {
      nxwarn << "Loop: WARNING! No 'sync' channel in current version. "
             << "Assuming 'cups' channel.\n" << std::flush;

      cerr << "Warning" << ": No 'sync' channel in current version. "
           << "Assuming 'cups' channel.\n";

      SetAndValidateChannelEndPointArg("local", name, value, cupsPort);
    }
    else if (strcasecmp(name, "keybd") == 0 ||
                 strcasecmp(name, "aux") == 0)
    {
      SetAndValidateChannelEndPointArg("local", name, value, auxPort);
    }
    else if (strcasecmp(name, "samba") == 0 ||
                 strcasecmp(name, "smb") == 0)
    {
      SetAndValidateChannelEndPointArg("local", name, value, smbPort);
    }
    else if (strcasecmp(name, "media") == 0)
    {
      SetAndValidateChannelEndPointArg("local", name, value, mediaPort);
    }
    else if (strcasecmp(name, "http") == 0)
    {
      SetAndValidateChannelEndPointArg("local", name, value, httpPort);
    }
    else if (strcasecmp(name, "font") == 0)
    {
      snprintf(fontPort, DEFAULT_STRING_LENGTH, "%s", value);
    }
    else if (strcasecmp(name, "slave") == 0)
    {
      SetAndValidateChannelEndPointArg("local", name, value, slavePort);
    }
    else if (strcasecmp(name, "mask") == 0)
    {
      control -> ChannelMask = ValidateArg("local", name, value);
    }
    else if (strcasecmp(name, "timeout") == 0)
    {
      int timeout = ValidateArg("local", name, value);

      if (timeout == 0)
      {
        nxinfo << "Loop: Disabling timeout on broken "
               << "proxy connection.\n" << std::flush;

        control -> ProxyTimeout = 0;
      }
      else
      {
        control -> ProxyTimeout = timeout * 1000;
      }
    }
    else if (strcasecmp(name, "cleanup") == 0)
    {
      int cleanup = ValidateArg("local", name, value);

      if (cleanup == 0)
      {
        nxinfo << "Loop: Disabling grace timeout on "
               << "proxy shutdown.\n" << std::flush;

        control -> CleanupTimeout = 0;
      }
      else
      {
        control -> CleanupTimeout = cleanup * 1000;
      }
    }
    else if (strcasecmp(name, "pack") == 0)
    {
      if (control -> ProxyMode == proxy_server)
      {
        PrintOptionIgnored("local", name, value);
      }
      else if (ParsePackOption(value) < 0)
      {
        nxfatal << "Loop: PANIC! Can't identify pack method for string '"
                << value << "'.\n" << std::flush;

        cerr << "Error" << ": Can't identify pack method for string '"
             << value << "'.\n";
        if (ParsePackOption("nopack")<0)
           return -1;
      }
    }
    else if (strcasecmp(name, "core") == 0)
    {
      control -> EnableCoreDumpOnAbort = ValidateArg("local", name, value);
    }
    else if (strcasecmp(name, "kill") == 0)
    {
      if (control -> KillDaemonOnShutdownNumber <
              control -> KillDaemonOnShutdownLimit)
      {
        nxinfo << "Loop: WARNING! Adding process with pid '"
               << ValidateArg("local", name, value) << " to the "
               << "daemons to kill at shutdown.\n"
               << std::flush;

        control -> KillDaemonOnShutdown[control ->
                       KillDaemonOnShutdownNumber] =
                           ValidateArg("local", name, value);

        control -> KillDaemonOnShutdownNumber++;
      }
      else
      {
        nxwarn << "Loop: WARNING! Number of daemons to kill "
               << "at shutdown exceeded.\n" << std::flush;

        cerr << "Warning" << ": Number of daemons to kill "
             << "at shutdown exceeded.\n";
      }
    }
    else if (strcasecmp(name, "strict") == 0)
    {
      if (control -> ProxyMode == proxy_server)
      {
        PrintOptionIgnored("local", name, value);
      }
      else
      {
        useStrict = ValidateArg("local", name, value);
      }
    }
    else if (strcasecmp(name, "encryption") == 0)
    {
      useEncryption = ValidateArg("local", name, value);
    }
    else if (strcasecmp(name, "product") == 0)
    {
      snprintf(productName, DEFAULT_STRING_LENGTH, "%s", value);
    }
    else if (strcasecmp(name, "rootless") == 0 ||
                 strcasecmp(name, "geometry") == 0 ||
                     strcasecmp(name, "resize") == 0 ||
                         strcasecmp(name, "fullscreen") == 0 ||
                             strcasecmp(name, "keyboard") == 0 ||
                                 strcasecmp(name, "clipboard") == 0 ||
                                     strcasecmp(name, "streaming") == 0 ||
                                         strcasecmp(name, "backingstore") == 0 ||
                                             strcasecmp(name, "sleep") == 0 ||
                                                 strcasecmp(name, "keyconv") == 0 ||
                                                     strcasecmp(name, "tolerancechecks") == 0)
    {
      nxdbg << "Loop: Ignoring agent option '" << name
            << "' with value '" << value << "'.\n"
            << std::flush;
    }
    else if (strcasecmp(name, "composite") == 0 ||
                 strcasecmp(name, "shmem") == 0 ||
                     strcasecmp(name, "shpix") == 0 ||
                         strcasecmp(name, "kbtype") == 0 ||
                             strcasecmp(name, "client") == 0 ||
                                 strcasecmp(name, "shadow") == 0 ||
                                     strcasecmp(name, "shadowuid") == 0 ||
                                         strcasecmp(name, "shadowmode") == 0 ||
                                             strcasecmp(name, "clients") == 0 ||
                                                 strcasecmp(name, "xinerama") == 0)
    {
      nxdbg << "Loop: Ignoring agent option '" << name
            << "' with value '" << value << "'.\n"
            << std::flush;
    }
    else if (strcasecmp(name, "defer") == 0 ||
                 strcasecmp(name, "tile") == 0 ||
                     strcasecmp(name, "menu") == 0 ||
                        strcasecmp(name, "magicpixel") == 0 ||
                          strcasecmp(name, "autodpi") == 0 ||
                            strcasecmp(name, "state") == 0 )
    {
      nxdbg << "Loop: Ignoring agent option '" << name
            << "' with value '" << value << "'.\n"
            << std::flush;
    }
    else
    {
      nxwarn << "Loop: WARNING! Ignoring unknown option '"
             << name << "' with value '" << value << "'.\n"
             << std::flush;

      cerr << "Warning" << ": Ignoring unknown option '"
           << name << "' with value '" << value << "'.\n";
    }

    name = strtok(NULL, "=");

  } // End of while (name) ...

  // Assemble the connectSocket channel end point if parameter values have been old-school...
  if (connectSocket.disabled() && (connectHost[0] != '\0') && (proxyPort > 0 || connectPort > 0))
  {
    if (connectPort < 0)
      connectPort = proxyPort + DEFAULT_NX_PROXY_PORT_OFFSET;

    char tcpHostAndPort[DEFAULT_STRING_LENGTH] = { 0 };
    sprintf(tcpHostAndPort, "tcp:%s:%ld", connectHost, connectPort);
    SetAndValidateChannelEndPointArg("local", name, tcpHostAndPort, connectSocket);
  }

  nxinfo << "Loop: Completed parsing of string '"
         << env << "'.\n" << std::flush;

  if ((*fileOptions != '\0') && (strncmp(fileOptions, "/dev/", 5) != 0) && (strncmp(fileOptions, "/proc/", 6) != 0) && (strncmp(fileOptions, "/sys/", 5) != 0))
  {
    if (strcmp(fileOptions, optionsFileName) != 0)
    {
      nxinfo << "Loop: Reading options from '" << fileOptions
             << "'.\n" << std::flush;

      if (ParseFileOptions(fileOptions) < 0)
      {
        return -1;
      }
    }
    else
    {
      nxwarn << "Loop: WARNING! Name of the options file "
             << "specified multiple times. Not parsing "
             << "again.\n" << std::flush;
    }

    if (*optionsFileName == '\0')
    {
      snprintf(optionsFileName, DEFAULT_STRING_LENGTH, "%s", value);

      nxinfo << "Loop: Assuming name of options file '"
             << optionsFileName << "'.\n"
             << std::flush;
    }
  }

  //
  // If port where proxy is acting as an X server
  // was not specified assume the same port where
  // proxy is listening for the remote peer.
  //

  if (xPort == DEFAULT_NX_X_PORT)
  {
    xPort = proxyPort;
  }

  return 1;
}

//
// Parse the command line options passed by user when
// running proxy in stand alone mode. Note that passing
// parameters this way is strongly discouraged. These
// command line switch can change (and they do often).
// Please, use the form "option=value" instead and set
// the DISPLAY environment variable.
//

int ParseCommandLineOptions(int argc, const char **argv)
{
  //
  // Be sure log file is valid.
  //

  if (logofs == NULL)
  {
    logofs = &cerr;
  }

  if (setjmp(context) == 1)
  {
    nxinfo << "Loop: Out of the long jump while parsing "
           << "the command line options.\n"
           << std::flush;

    return -1;
  }

  //
  // Be sure we have a parameters repository
  //

  if (control == NULL)
  {
    control = new Control();
  }

  if (parsedCommand == 1)
  {
    nxinfo << "Loop: Skipping a further parse of command line options.\n"
           << std::flush;

    return 1;
  }

  nxinfo << "Loop: Going to parse the command line options.\n"
         << std::flush;

  parsedCommand = 1;

  //
  // Print out arguments.
  //


  nxinfo << "Loop: Argc is " << argc << ".\n" << std::flush;

  for (int argi = 0; argi < argc; argi++)
  {
    nxinfo << "Loop: Argv[" << argi << "] is " << argv[argi]
           << ".\n" << std::flush;
  }


  //
  // Shall use getopt here.
  //

  for (int argi = 1; argi < argc; argi++)
  {
    const char *nextArg = argv[argi];

    if (*nextArg == '-')
    {
      switch (*(nextArg + 1))
      {
        case 'h':
        {
          PrintUsageInfo(nextArg, 0);

          return -1;
        }
        case 'C':
        {
          //
          // Start proxy in CLIENT mode.
          //

          if (WE_SET_PROXY_MODE == 0)
          {
            nxinfo << "Loop: Setting local proxy mode to proxy_client.\n"
                   << std::flush;

            control -> ProxyMode = proxy_client;
          }
          else if (control -> ProxyMode != proxy_client)
          {
            nxfatal << "Loop: PANIC! Can't redefine local proxy to "
                    << "client mode.\n" << std::flush;

            cerr << "Error" << ": Can't redefine local proxy to "
                 << "client mode.\n";

            return -1;
          }

          break;
        }
        case 'S':
        {
          //
          // Start proxy in SERVER mode.
          //

          if (WE_SET_PROXY_MODE == 0)
          {
            nxinfo << "Loop: Setting local proxy mode to proxy_server.\n"
                   << std::flush;

            control -> ProxyMode = proxy_server;
          }
          else if (control -> ProxyMode != proxy_server)
          {
            nxfatal << "Loop: PANIC! Can't redefine local proxy to "
                    << "server mode.\n" << std::flush;

            cerr << "Error" << ": Can't redefine local proxy to "
                 << "server mode.\n";

            return -1;
          }

          break;
        }
        case 'v':
        {
          PrintVersionInfo();

          return -1;
        }
        case 'd':
        {
          if ( argi+1 >= argc )
          {
            PrintUsageInfo(nextArg, 0);
            return -1;
          }

          int level = 0;
          errno = 0;
          level = strtol(argv[argi+1], NULL, 10);

          if ( errno && (level == 0) )
          {
            cerr << "Warning: Failed to parse log level. Ignoring option." << std::endl;
          }
          if ( level < 0 )
          {
            cerr << "Warning: Log level must be a positive integer. Ignoring option." << std::endl;
            level = nx_log.level();
          }
          else if ( level >= NXLOG_LEVEL_COUNT )
          {
            cerr << "Warning: Log level is greater than the maximum " << NXLOG_LEVEL_COUNT-1 << ". Setting to the maximum." << std::endl;
            level = NXLOG_LEVEL_COUNT-1;
          }

          nx_log.level( (NXLogLevel)level );

          argi++;
          break;

        }
        case 'o':
        {
          if ( argi + 1 >= argc )
          {
            PrintUsageInfo(nextArg, 0);
            return -1;
          }

          std::ofstream *logfile = new std::ofstream();

          // Unbuffered output
          logfile->rdbuf()->pubsetbuf(0, 0);
          logfile->open(argv[argi+1], std::ofstream::app);

          if ( logfile->is_open() )
          {
            nx_log.stream(logfile);
          }
          else
          {
            cerr << "Failed to open log file " << argv[argi+1] << endl;
            return -1;
          }

          argi++;
          break;
        }
        case 'f':
        {
          if ( argi + 1 >= argc )
          {
            PrintUsageInfo(nextArg, 0);
            return -1;
          }

          const char *format = argv[argi+1];
          size_t pos = 0;

          nx_log.log_level(false);
          nx_log.log_time(false);
          nx_log.log_unix_time(false);
          nx_log.log_location(false);
          nx_log.log_thread_id(false);

          for(pos =0;pos<strlen(format);pos++)
          {
            switch(format[pos])
            {
              case '0': break;
              case 't': nx_log.log_time(true); break;
              case 'u': nx_log.log_time(true); nx_log.log_unix_time(true); break;
              case 'l': nx_log.log_level(true); break;
              case 'T': nx_log.log_thread_id(true); break;
              case 'L': nx_log.log_location(true); break;
              default : cerr << "Unrecognized format specifier: " << format[pos] << endl; break;
            }
          }

          argi++;
          break;
        }


        default:
        {
          PrintUsageInfo(nextArg, 1);

          //
          // Function GetArg() is not used anymore.
          // Add a dummy call to avoid the warning.
          //

          if (0)
          {
            GetArg(argi, argc, argv);
          }

          return -1;
        }
      }
    }
    else
    {
      if (nextArg)
      {
        //
        // Try to parse the option as a remote host:port
        // specification as in 'localhost:8'. Such a
        // parameter can be specified at the end of the
        // command line at the connecting side.
        //

        char cHost[DEFAULT_STRING_LENGTH] = { '\0' };
        long cPort = 0;

        if (ParseHostOption(nextArg, cHost, cPort) > 0)
          {
            //
            // Assume port is at a proxied display offset.
            //

            proxyPort = cPort;

            cPort += DEFAULT_NX_PROXY_PORT_OFFSET;
            connectSocket.setSpec(cHost, cPort);

          }
        else if (ParseEnvironmentOptions(nextArg, 1) < 0)
        {
          return -1;
        }
      }
    }
  }

  return 1;
}

//
// Set the variable to the values of host and
// port where this proxy is going to hook to
// an existing proxy.
//

int ParseBindOptions(char **host, int *port)
{
  if (*bindHost != '\0')
  {
    *host = bindHost;
    *port = bindPort;

    return 1;
  }
  else
  {
    return 0;
  }
}

//
// Read options from file and merge with environment.
//

int ParseFileOptions(const char *file)
{
  char *fileName;

  if (*file != '/' && *file != '.')
  {
    char *filePath = GetSessionPath();

    if (filePath == NULL)
    {
      cerr << "Error" << ": Cannot determine directory for NX option file.\n";

      HandleCleanup();
    }

    fileName = new char[strlen(filePath) + strlen("/") +
                            strlen(file) + 1];

    strcpy(fileName, filePath);

    strcat(fileName, "/");
    strcat(fileName, file);

    delete [] filePath;
  }
  else
  {
    fileName = new char[strlen(file) + 1];

    strcpy(fileName, file);
  }

  nxinfo << "Loop: Going to read options from file '"
         << fileName << "'.\n" << std::flush;

  FILE *filePtr = fopen(fileName, "r");

  if (filePtr == NULL)
  {
    nxfatal << "Loop: PANIC! Can't open options file '" << fileName
            << "'. Error is " << EGET() << " '" << ESTR() << "'.\n"
            << std::flush;

    cerr << "Error" << ": Can't open options file '" << fileName
         << "'. Error is " << EGET() << " '" << ESTR() << "'.\n";

    delete [] fileName;

    return -1;
  }

  char options[DEFAULT_DISPLAY_OPTIONS_LENGTH];

  #ifdef VALGRIND

  memset(options, '\0', DEFAULT_DISPLAY_OPTIONS_LENGTH);

  #endif

  if (fgets(options, DEFAULT_DISPLAY_OPTIONS_LENGTH, filePtr) == NULL)
  {
    nxfatal << "Loop: PANIC! Can't read options from file '" << fileName
            << "'. Error is " << EGET() << " '" << ESTR() << "'.\n"
            << std::flush;

    cerr << "Error" << ": Can't read options from file '" << fileName
         << "'. Error is " << EGET() << " '" << ESTR() << "'.\n";

    fclose(filePtr);

    delete [] fileName;

    return -1;
  }

  fclose(filePtr);

  //
  // Purge the newline and the other non-
  // printable characters in the string.
  //

  char *next = options;

  while (*next != '\0')
  {
    if (isprint(*next) == 0)
    {
      *next = '\0';
    }

    next++;
  }

  nxinfo << "Loop: Read options '" << options << "' from file '"
         << fileName << "'.\n" << std::flush;

  if (ParseEnvironmentOptions(options, 1) < 0)
  {
    delete [] fileName;

    return -1;
  }

  delete [] fileName;

  return 1;
}

//
// Parse the option string passed from the
// remote proxy at startup.
//

int ParseRemoteOptions(char *opts)
{
  nxinfo << "Loop: Going to parse the remote options "
         << "string '" << opts << "'.\n"
         << std::flush;

  char *name;
  char *value;

  //
  // The options string is intended to be a series
  // of name/value tuples in the form name=value
  // separated by the ',' character.
  //

  int hasCookie = 0;
  int hasLink   = 0;
  int hasPack   = 0;
  int hasCache  = 0;
  int hasImages = 0;
  int hasDelta  = 0;
  int hasStream = 0;
  int hasData   = 0;
  int hasType   = 0;

  //
  // Get rid of the terminating space.
  //

  if (*(opts + strlen(opts) - 1) == ' ')
  {
    *(opts + strlen(opts) - 1) = '\0';
  }

  name = strtok(opts, "=");

  while (name)
  {
    value = strtok(NULL, ",");

    if (CheckArg("remote", name, value) < 0)
    {
      return -1;
    }

    if (strcasecmp(name, "cookie") == 0)
    {
      if (WE_PROVIDE_CREDENTIALS)
      {
        nxwarn << "Loop: WARNING! Ignoring remote option 'cookie' "
               << "with value '" << value << "' when initiating "
               << "connection.\n" << std::flush;

        cerr << "Warning" << ": Ignoring remote option 'cookie' "
             << "with value '" << value << "' when initiating "
             << "connection.\n";
      }
      else if (strncasecmp(authCookie, value, strlen(authCookie)) != 0)
      {
        nxfatal << "Loop: PANIC! Authentication cookie '" << value
                << "' doesn't match '" << authCookie << "'.\n"
                << std::flush;

        cerr << "Error" << ": Authentication cookie '" << value
             << "' doesn't match '" << authCookie << "'.\n";

        return -1;
      }

      hasCookie = 1;
    }
    else if (strcasecmp(name, "link") == 0)
    {
      if (control -> ProxyMode == proxy_client)
      {
        PrintOptionIgnored("remote", name, value);
      }
      else
      {
        if (*linkSpeedName != '\0' && strcasecmp(linkSpeedName, value) != 0)
        {
          nxwarn << "Loop: WARNING! Overriding option 'link' "
                 << "with new value '" << value << "'.\n"
                 << std::flush;

          cerr << "Warning" << ": Overriding option 'link' "
               << "with new value '" << value << "'.\n";
        }

        if (ParseLinkOption(value) < 0)
        {
          nxfatal << "Loop: PANIC! Can't identify remote 'link' "
                  << "option in string '" << value << "'.\n"
                  << std::flush;

          cerr << "Error" << ": Can't identify remote 'link' "
               << "option in string '" << value << "'.\n";

          return -1;
        }
      }

      hasLink = 1;
    }
    else if (strcasecmp(name, "pack") == 0)
    {
      if (control -> ProxyMode == proxy_client)
      {
        PrintOptionIgnored("remote", name, value);
      }
      else
      {
        if (*packMethodName != '\0' && strcasecmp(packMethodName, value) != 0)
        {
          nxwarn << "Loop: WARNING! Overriding option 'pack' "
                 << "with remote value '" << value << "'.\n"
                 << std::flush;

          cerr << "Warning" << ": Overriding option 'pack' "
               << "with remote value '" << value << "'.\n";
        }

        if (ParsePackOption(value) < 0)
        {
          nxfatal << "Loop: PANIC! Invalid pack option '"
                  << value << "' requested by remote.\n"
                  << std::flush;

          cerr << "Error" << ": Invalid pack option '"
               << value << "' requested by remote.\n";

          return -1;
        }
      }

      hasPack = 1;
    }
    else if (strcasecmp(name, "cache") == 0)
    {
      if (control -> ProxyMode == proxy_client)
      {
        PrintOptionIgnored("remote", name, value);
      }
      else
      {
        //
        // Cache size is sent as a hint of how much memory
        // the remote proxy is going to consume. A very low
        // powered thin client could choose to refuse the
        // connection.
        //

        if (ParseCacheOption(value) < 0)
        {
          nxfatal << "Loop: PANIC! Can't identify remote 'cache' "
                  << "option in string '" << value << "'.\n"
                  << std::flush;

          cerr << "Error" << ": Can't identify remote 'cache' "
               << "option in string '" << value << "'.\n";

          return -1;
        }
      }

      hasCache = 1;
    }
    else if (strcasecmp(name, "images") == 0)
    {
      if (control -> ProxyMode == proxy_client)
      {
        PrintOptionIgnored("remote", name, value);
      }
      else
      {
        //
        // Images cache size is sent as a hint.
        // There is no obbligation for the local
        // proxy to use the persistent cache.
        //

        if (ParseImagesOption(value) < 0)
        {
          nxfatal << "Loop: PANIC! Can't identify remote 'images' "
                  << "option in string '" << value << "'.\n"
                  << std::flush;

          cerr << "Error" << ": Can't identify remote 'images' "
               << "option in string '" << value << "'.\n";

          return -1;
        }
      }

      hasImages = 1;
    }
    else if (strcasecmp(name, "limit") == 0)
    {
      if (control -> ProxyMode == proxy_client)
      {
        PrintOptionIgnored("remote", name, value);
      }
      else
      {
        if (*bitrateLimitName != '\0' &&
                strcasecmp(bitrateLimitName, value) != 0)
        {
          nxwarn << "Loop: WARNING! Overriding option 'limit' "
                 << "with new value '" << value << "'.\n"
                 << std::flush;

          cerr << "Warning" << ": Overriding option 'limit' "
               << "with new value '" << value << "'.\n";
        }

        if (ParseBitrateOption(value) < 0)
        {
          nxfatal << "Loop: PANIC! Can't identify 'limit' "
                  << "option in string '" << value << "'.\n"
                  << std::flush;

          cerr << "Error" << ": Can't identify 'limit' "
               << "option in string '" << value << "'.\n";

          return -1;
        }
      }

    }
    else if (strcasecmp(name, "render") == 0)
    {
      if (control -> ProxyMode == proxy_client)
      {
        PrintOptionIgnored("remote", name, value);
      }
      else
      {
        useRender = ValidateArg("remote", name, value);
      }

    }
    else if (strcasecmp(name, "taint") == 0)
    {
      if (control -> ProxyMode == proxy_client)
      {
        PrintOptionIgnored("remote", name, value);
      }
      else
      {
        useTaint = ValidateArg("remote", name, value);
      }

    }
    else if (strcasecmp(name, "type") == 0)
    {
      if (control -> ProxyMode == proxy_client)
      {
        PrintOptionIgnored("remote", name, value);
      }
      else
      {
        if (strcasecmp(value, "default") == 0)
        {
          *sessionType = '\0';
        }
        else
        {
          snprintf(sessionType, DEFAULT_STRING_LENGTH, "%s", value);
        }
      }

      hasType = 1;
    }
    else if (strcasecmp(name, "strict") == 0)
    {
      if (control -> ProxyMode == proxy_client)
      {
        PrintOptionIgnored("remote", name, value);
      }
      else
      {
        useStrict = ValidateArg("remote", name, value);
      }

    }
    else if (strcasecmp(name, "shseg") == 0)
    {
      if (control -> ProxyMode == proxy_client)
      {
        PrintOptionIgnored("remote", name, value);
      }
      else if (ParseShmemOption(value) < 0)
      {
        nxfatal << "Loop: PANIC! Can't identify size of shared memory "
                << "segment in string '" << value << "'.\n"
                << std::flush;

        cerr << "Error" << ": Can't identify size of shared memory "
             << "segment in string '" << value << "'.\n";

        return -1;
      }

    }
    else if (strcasecmp(name, "delta") == 0)
    {
      if (control -> ProxyMode == proxy_client)
      {
        PrintOptionIgnored("remote", name, value);
      }
      else
      {
        control -> RemoteDeltaCompression = ValidateArg("remote", name, value);

        //
        // Follow for delta compression the
        // same settings as the client proxy.
        //

        control -> LocalDeltaCompression  = control -> RemoteDeltaCompression;
      }

      hasDelta = 1;
    }
    else if (strcasecmp(name, "stream") == 0)
    {
      //
      // If remote side didn't choose its own
      // stream compression level then assume
      // local settings.
      //

      if (strcasecmp(value, "default") == 0)
      {
        //
        // This applies only at client side.
        //

        control -> RemoteStreamCompression =
            control -> LocalStreamCompression;

        control -> RemoteStreamCompressionLevel =
            control -> LocalStreamCompressionLevel;
      }
      else
      {
        control -> RemoteStreamCompressionLevel = ValidateArg("remote", name, value);

        if (control -> RemoteStreamCompressionLevel > 0)
        {
          control -> RemoteStreamCompression = 1;
        }
        else
        {
          control -> RemoteStreamCompression = 0;
        }

        if (control -> LocalStreamCompressionLevel < 0)
        {
          control -> LocalStreamCompressionLevel = ValidateArg("remote", name, value);

          if (control -> LocalStreamCompressionLevel > 0)
          {
            control -> LocalStreamCompression = 1;
          }
          else
          {
            control -> LocalStreamCompression = 0;
          }
        }
      }

      hasStream = 1;
    }
    else if (strcasecmp(name, "data") == 0)
    {
      //
      // Apply the same to data compression level.
      //

      if (strcasecmp(value, "default") == 0)
      {
        control -> RemoteDataCompression =
            control -> LocalDataCompression;

        control -> RemoteDataCompressionLevel =
            control -> LocalDataCompressionLevel;
      }
      else
      {
        control -> RemoteDataCompressionLevel = ValidateArg("remote", name, value);

        if (control -> RemoteDataCompressionLevel > 0)
        {
          control -> RemoteDataCompression = 1;
        }
        else
        {
          control -> RemoteDataCompression = 0;
        }

        if (control -> LocalDataCompressionLevel < 0)
        {
          control -> LocalDataCompressionLevel = ValidateArg("remote", name, value);

          if (control -> LocalDataCompressionLevel > 0)
          {
            control -> LocalDataCompression = 1;
          }
          else
          {
            control -> LocalDataCompression = 0;
          }
        }
      }

      hasData = 1;
    }
    else if (strcasecmp(name, "flush") == 0)
    {
      //
      // This option has no effect in recent
      // versions.
      //

      nxdbg << "Loop: Ignoring obsolete remote option '"
            << name << "' with value '" << value
            << "'.\n" << std::flush;
    }
    else
    {
      nxwarn << "Loop: WARNING! Ignoring unknown remote option '"
             << name << "' with value '" << value << "'.\n"
             << std::flush;

      cerr << "Warning" << ": Ignoring unknown remote option '"
           << name << "' with value '" << value << "'.\n";
    }

    name = strtok(NULL, "=");

  } // End of while (name) ...

  //
  // If we are client side, we need remote 'stream'
  // and 'data' options. If we are server, we need
  // all the above plus 'link' and some others.
  //

  char missing[DEFAULT_STRING_LENGTH];

  *missing = '\0';

  if (control -> ProxyMode == proxy_client)
  {
    if (hasStream == 0)
    {
      strcpy(missing, "stream");
    }
    else if (hasData == 0)
    {
      strcpy(missing, "data");
    }
  }
  else
  {
    //
    // Don't complain if the optional 'flush',
    // 'render' and 'taint' options are not
    // provided.
    //

    if (hasLink == 0)
    {
      strcpy(missing, "link");
    }
    else if (hasCache == 0)
    {
      strcpy(missing, "cache");
    }
    else if (hasPack == 0)
    {
      strcpy(missing, "pack");
    }
    else if (hasDelta == 0)
    {
      strcpy(missing, "delta");
    }
    else if (hasStream == 0)
    {
      strcpy(missing, "stream");
    }
    else if (hasData == 0)
    {
      strcpy(missing, "data");
    }
    else if (hasType == 0)
    {
      strcpy(missing, "type");
    }
    else if (hasImages == 0)
    {
      strcpy(missing, "images");
    }
  }

  if (WE_PROVIDE_CREDENTIALS == 0)
  {
    //
    // Can be that user doesn't have requested to
    // check the authorization cookie provided by
    // the connecting peer.
    //

    if (hasCookie == 0 && *authCookie != '\0')
    {
      strcpy(missing, "cookie");
    }
  }

  if (*missing != '\0')
  {
    nxfatal << "Loop: PANIC! The remote peer didn't specify the option '"
            << missing << "'.\n" << std::flush;

    cerr << "Error" << ": The remote peer didn't specify the option '"
         << missing << "'.\n";

    return -1;
  }

  return 1;
}

//
// Parse the cookie provided by the NX proxy
// connection forwarder.
//

int ParseForwarderOptions(char *opts)
{
  nxinfo << "Loop: Going to parse the forwarder options "
         << "string '" << opts << "'.\n"
         << std::flush;

  char *name;
  char *value;

  int hasCookie = 0;

  //
  // Get rid of the terminating space.
  //

  if (*(opts + strlen(opts) - 1) == ' ')
  {
    *(opts + strlen(opts) - 1) = '\0';
  }

  name = strtok(opts, "=");

  while (name)
  {
    value = strtok(NULL, ",");

    if (CheckArg("forwarder", name, value) < 0)
    {
      return -1;
    }

    if (strcasecmp(name, "cookie") == 0)
    {
      if (strncasecmp(authCookie, value, strlen(authCookie)) != 0)
      {
        nxfatal << "Loop: PANIC! The NX forwarder cookie '" << value
                << "' doesn't match '" << authCookie << "'.\n"
                << std::flush;

        cerr << "Error" << ": The NX forwarder cookie '" << value
             << "' doesn't match '" << authCookie << "'.\n";

        return -1;
      }

      hasCookie = 1;
    }
    else
    {
      nxwarn << "Loop: WARNING! Ignoring unknown forwarder option '"
             << name << "' with value '" << value << "'.\n"
             << std::flush;

      cerr << "Warning" << ": Ignoring unknown forwarder option '"
           << name << "' with value '" << value << "'.\n";
    }

    name = strtok(NULL, "=");

  } // End of while (name) ...

  if (hasCookie == 0)
  {
    nxfatal << "Loop: PANIC! The NX forwarder didn't provide "
            << "the authentication cookie.\n" << std::flush;

    cerr << "Error" << ": The NX forwarder didn't provide "
         << "the authentication cookie.\n";

    return -1;
  }

  return 1;
}

int SetCore()
{
  #ifdef COREDUMPS

  rlimit rlim;

  if (getrlimit(RLIMIT_CORE, &rlim))
  {
    nxinfo << "Cannot read RLIMIT_CORE. Error is '"
           << ESTR() << "'.\n" << std::flush;

    return -1;
  }

  if (rlim.rlim_cur < rlim.rlim_max)
  {
    rlim.rlim_cur = rlim.rlim_max;

    if (setrlimit(RLIMIT_CORE, &rlim))
    {
      nxinfo << "Loop: Cannot read RLIMIT_CORE. Error is '"
             << ESTR() << "'.\n" << std::flush;

      return -2;
    }
  }

  nxinfo << "Loop: Set RLIMIT_CORE to "<< rlim.rlim_max
         << ".\n" << std::flush;

  #endif // #ifdef COREDUMPS

  return 1;
}

char *GetLastCache(char *listBuffer, const char *searchPath)
{
  if (listBuffer == NULL || searchPath == NULL ||
          strncmp(listBuffer, "cachelist=", strlen("cachelist=")) != 0)
  {
    nxinfo << "Loop: Invalid parameters '" << listBuffer << "' and '"
           << (searchPath != NULL ? searchPath : "")
           << "'. Can't select any cache.\n" << std::flush;

    return NULL;
  }

  char *selectedName = new char[MD5_LENGTH * 2 + 3];

  *selectedName = '\0';

  const char *localPrefix;
  const char *remotePrefix;

  if (control -> ProxyMode == proxy_client)
  {
    localPrefix  = "C-";
    remotePrefix = "S-";
  }
  else
  {
    localPrefix  = "S-";
    remotePrefix = "C-";
  }

  //
  // Get rid of prefix.
  //

  listBuffer += strlen("cachelist=");

  char *fileName;

  fileName = strtok(listBuffer, ",");

  //
  // It is "/path/to/file" + "/" + "C-" + 32 + "\0".
  //

  char fullPath[strlen(searchPath) + MD5_LENGTH * 2 + 4];

  time_t selectedTime = 0;

  struct stat fileStat;

  while (fileName)
  {
    if (strncmp(fileName, "none", strlen("none")) == 0)
    {
      nxinfo << "Loop: No cache files seem to be available.\n"
             << std::flush;

      delete [] selectedName;

      return NULL;
    }
    else if (strlen(fileName) != MD5_LENGTH * 2 + 2 ||
                 strncmp(fileName, remotePrefix, 2) != 0)
    {
      nxfatal << "Loop: PANIC! Bad cache file name '"
              << fileName << "'.\n" << std::flush;

      cerr << "Error" << ": Bad cache file name '"
           << fileName << "'.\n";

      delete [] selectedName;

      HandleCleanup();
    }

    nxinfo << "Loop: Parsing remote cache name '"
           << fileName << "'.\n" << std::flush;

    //
    // Prefix, received as "S-", becomes
    // "C-" and viceversa.
    //

    *fileName = *localPrefix;

    strcpy(fullPath, searchPath);
    strcat(fullPath, "/");
    strcat(fullPath, fileName);

    if (stat(fullPath, &fileStat) == 0)
    {
      nxinfo << "Loop: Found a matching cache '"
             << std::string(fullPath) << "'.\n" << std::flush;

      if (fileStat.st_mtime >= selectedTime)
      {
        strcpy(selectedName, fileName);

        selectedTime = fileStat.st_mtime;
      }
    }
    else
    {
      nxinfo << "Loop: Can't get stats of file '"
             << std::string(fullPath) << "'.\n" << std::flush;
    }

    fileName = strtok(NULL, ",");
  }

  if (*selectedName != '\0')
  {
    return selectedName;
  }
  else
  {
    delete [] selectedName;

    return NULL;
  }
}

char *GetTempPath()
{
  if (*tempDir == '\0')
  {
    //
    // Check the NX_TEMP environment, first,
    // then the TEMP variable.
    //

    const char *tempEnv = getenv("NX_TEMP");

    if (tempEnv == NULL || *tempEnv == '\0')
    {
      nxinfo << "Loop: WARNING! No environment for NX_TEMP.\n"
             << std::flush;

      tempEnv = getenv("TEMP");

      if (tempEnv == NULL || *tempEnv == '\0')
      {
        nxinfo << "Loop: WARNING! No environment for TEMP.\n"
               << std::flush;

        tempEnv = "/tmp";
      }
    }

    if (strlen(tempEnv) > DEFAULT_STRING_LENGTH - 1)
    {
      nxfatal << "Loop: PANIC! Invalid value for the NX "
              << "temporary directory '" << tempEnv
              << "'.\n" << std::flush;

      cerr << "Error" << ": Invalid value for the NX "
           << "temporary directory '" << tempEnv
           << "'.\n";

      HandleCleanup();
    }

    strcpy(tempDir, tempEnv);

    nxinfo << "Loop: Assuming temporary NX directory '"
           << tempDir << "'.\n" << std::flush;
  }

  char *tempPath = new char[strlen(tempDir) + 1];

  if (tempPath == NULL)
  {
    nxfatal << "Loop: PANIC! Can't allocate memory "
            << "for the temp path.\n" << std::flush;

    cerr << "Error" << ": Can't allocate memory "
         << "for the temp path.\n";

    HandleCleanup();
  }

  strcpy(tempPath, tempDir);

  return tempPath;
}

char *GetClientPath()
{
  if (*clientDir == '\0')
  {
    //
    // Check the NX_CLIENT environment.
    //

    const char *clientEnv = getenv("NX_CLIENT");

    if (clientEnv == NULL || *clientEnv == '\0')
    {
      nxinfo << "Loop: WARNING! No environment for NX_CLIENT.\n"
             << std::flush;

      //
      // Try to guess the location of the client.
      //
      // FIXME: replace hardcoded paths by built-time variables if possible

      #ifdef __APPLE__

      clientEnv = "/Applications/NX Client for OSX.app/Contents/MacOS/nxclient";

      #else
      #  if defined(__CYGWIN__) || defined(__CYGWIN32__)

      clientEnv = "C:\\Program Files\\NX Client for Windows\\nxclient";

      #  else

      clientEnv = "/usr/NX/bin/nxclient";

      struct stat fileStat;

      if ((stat(clientEnv, &fileStat) == -1) && (EGET() == ENOENT))
      {
        clientEnv = "/usr/bin/nxdialog";
      }

      #  endif
      #endif
    }

    if (strlen(clientEnv) > DEFAULT_STRING_LENGTH - 1)
    {
      nxfatal << "Loop: PANIC! Invalid value for the NX "
              << "client directory '" << clientEnv
              << "'.\n" << std::flush;

      cerr << "Error" << ": Invalid value for the NX "
           << "client directory '" << clientEnv
           << "'.\n";

      HandleCleanup();
    }

    strcpy(clientDir, clientEnv);

    nxinfo << "Loop: Assuming NX client location '"
           << clientDir << "'.\n" << std::flush;
  }

  char *clientPath = new char[strlen(clientDir) + 1];

  if (clientPath == NULL)
  {
    nxfatal << "Loop: PANIC! Can't allocate memory "
            << "for the client path.\n" << std::flush;

    cerr << "Error" << ": Can't allocate memory "
         << "for the client path.\n";

    HandleCleanup();
  }

  strcpy(clientPath, clientDir);

  return clientPath;
}

char *GetSystemPath()
{
  if (*systemDir == '\0')
  {
    //
    // Check the NX_SYSTEM environment.
    //

    const char *systemEnv = getenv("NX_SYSTEM");

    if (systemEnv == NULL || *systemEnv == '\0')
    {
      nxinfo << "Loop: WARNING! No environment for NX_SYSTEM.\n"
             << std::flush;

      systemEnv = "/usr/NX";
    }

    if (strlen(systemEnv) > DEFAULT_STRING_LENGTH - 1)
    {
      nxfatal << "Loop: PANIC! Invalid value for the NX "
              << "system directory '" << systemEnv
              << "'.\n" << std::flush;

      cerr << "Error" << ": Invalid value for the NX "
           << "system directory '" << systemEnv
           << "'.\n";

      HandleCleanup();
    }

    strcpy(systemDir, systemEnv);

    nxinfo << "Loop: Assuming system NX directory '"
           << systemDir << "'.\n" << std::flush;
  }

  char *systemPath = new char[strlen(systemDir) + 1];

  if (systemPath == NULL)
  {
    nxfatal << "Loop: PANIC! Can't allocate memory "
            << "for the system path.\n" << std::flush;

    cerr << "Error" << ": Can't allocate memory "
         << "for the system path.\n";

    HandleCleanup();
  }

  strcpy(systemPath, systemDir);

  return systemPath;
}

char *GetHomePath()
{
  if (*homeDir == '\0')
  {
    //
    // Check the NX_HOME environment.
    //

    const char *homeEnv = getenv("NX_HOME");

    if (homeEnv == NULL || *homeEnv == '\0')
    {
      nxinfo << "Loop: WARNING! No environment for NX_HOME.\n"
             << std::flush;

      homeEnv = getenv("HOME");

      if (homeEnv == NULL || *homeEnv == '\0')
      {
        nxfatal << "Loop: PANIC! No environment for HOME.\n"
                << std::flush;

        cerr << "Error" << ": No environment for HOME.\n";

        HandleCleanup();
      }
    }

    if (strlen(homeEnv) > DEFAULT_STRING_LENGTH - 1)
    {
      nxfatal << "Loop: PANIC! Invalid value for the NX "
              << "home directory '" << homeEnv
              << "'.\n" << std::flush;

      cerr << "Error" << ": Invalid value for the NX "
           << "home directory '" << homeEnv
           << "'.\n";

      HandleCleanup();
    }

    strcpy(homeDir, homeEnv);

    nxinfo << "Loop: Assuming NX user's home directory '"
           << homeDir << "'.\n" << std::flush;
  }

  char *homePath = new char[strlen(homeDir) + 1];

  if (homePath == NULL)
  {
    nxfatal << "Loop: PANIC! Can't allocate memory "
            << "for the home path.\n" << std::flush;

    cerr << "Error" << ": Can't allocate memory "
         << "for the home path.\n";

    HandleCleanup();
  }

  strcpy(homePath, homeDir);

  return homePath;
}

char *GetRootPath()
{
  if (*rootDir == '\0')
  {
    //
    // Check the NX_ROOT environment.
    //

    const char *rootEnv = getenv("NX_ROOT");

    if (rootEnv == NULL || *rootEnv == '\0')
    {
      nxinfo << "Loop: WARNING! No environment for NX_ROOT.\n"
             << std::flush;

      //
      // We will determine the root NX directory
      // based on the NX_HOME or HOME directory
      // settings.
      //

      const char *homeEnv = GetHomePath();

      if (strlen(homeEnv) > DEFAULT_STRING_LENGTH -
              strlen("/.nx") - 1)
      {
        nxfatal << "Loop: PANIC! Invalid value for the NX "
                << "home directory '" << homeEnv
                << "'.\n" << std::flush;

        cerr << "Error" << ": Invalid value for the NX "
             << "home directory '" << homeEnv
             << "'.\n";

        HandleCleanup();
      }

      nxinfo << "Loop: Assuming NX root directory in "
             << "the user's home '" << homeEnv
              << "'.\n" << std::flush;

      strcpy(rootDir, homeEnv);
      strcat(rootDir, "/.nx");

      delete [] homeEnv;

      //
      // Create the NX root directory.
      //

      struct stat dirStat;

      if ((stat(rootDir, &dirStat) == -1) && (EGET() == ENOENT))
      {
        if (mkdir(rootDir, 0700) < 0 && (EGET() != EEXIST))
        {
          nxfatal << "Loop: PANIC! Can't create directory '"
                  << rootDir << ". Error is " << EGET() << " '"
                  << ESTR() << "'.\n" << std::flush;

          cerr << "Error" << ": Can't create directory '"
               << rootDir << ". Error is " << EGET() << " '"
               << ESTR() << "'.\n";

          HandleCleanup();
        }
      }
    }
    else
    {
      if (strlen(rootEnv) > DEFAULT_STRING_LENGTH - 1)
      {
        nxfatal << "Loop: PANIC! Invalid value for the NX "
                << "root directory '" << rootEnv
                << "'.\n" << std::flush;

        cerr << "Error" << ": Invalid value for the NX "
             << "root directory '" << rootEnv
             << "'.\n";

        HandleCleanup();
      }

      strcpy(rootDir, rootEnv);
    }

    nxinfo << "Loop: Assuming NX root directory '"
           << rootDir << "'.\n" << std::flush;
  }

  char *rootPath = new char[strlen(rootDir) + 1];

  if (rootPath == NULL)
  {
    nxfatal << "Loop: PANIC! Can't allocate memory "
            << "for the root path.\n" << std::flush;

    cerr << "Error" << ": Can't allocate memory "
         << "for the root path.\n";

    HandleCleanup();
  }

  strcpy(rootPath, rootDir);

  return rootPath;
}

char *GetCachePath()
{
  char *rootPath = GetRootPath();

  char *cachePath;

  if (*sessionType != '\0')
  {
    cachePath = new char[strlen(rootPath) + strlen("/cache-") +
                             strlen(sessionType) + 1];
  }
  else
  {
    cachePath = new char[strlen(rootPath) + strlen("/cache") + 1];
  }

  strcpy(cachePath, rootPath);

  if (*sessionType != '\0')
  {
    strcat(cachePath, "/cache-");

    strcat(cachePath, sessionType);
  }
  else
  {
    strcat(cachePath, "/cache");
  }

  //
  // Create the cache directory if needed.
  //

  struct stat dirStat;

  if ((stat(cachePath, &dirStat) == -1) && (EGET() == ENOENT))
  {
    if (mkdir(cachePath, 0700) < 0 && (EGET() != EEXIST))
    {
      nxfatal << "Loop: PANIC! Can't create directory '" << cachePath
              << ". Error is " << EGET() << " '" << ESTR() << "'.\n"
              << std::flush;

      cerr << "Error" << ": Can't create directory '" << cachePath
           << ". Error is " << EGET() << " '" << ESTR() << "'.\n";

      delete [] rootPath;
      delete [] cachePath;

      return NULL;
    }
  }

  delete [] rootPath;

  return cachePath;
}

char *GetImagesPath()
{
  char *rootPath = GetRootPath();

  char *imagesPath = new char[strlen(rootPath) + strlen("/images") + 1];

  strcpy(imagesPath, rootPath);

  strcat(imagesPath, "/images");

  //
  // Create the cache directory if needed.
  //

  struct stat dirStat;

  if ((stat(imagesPath, &dirStat) == -1) && (EGET() == ENOENT))
  {
    if (mkdir(imagesPath, 0700) < 0 && (EGET() != EEXIST))
    {
      nxfatal << "Loop: PANIC! Can't create directory '" << imagesPath
              << ". Error is " << EGET() << " '" << ESTR() << "'.\n"
              << std::flush;

      cerr << "Error" << ": Can't create directory '" << imagesPath
           << ". Error is " << EGET() << " '" << ESTR() << "'.\n";

      delete [] rootPath;
      delete [] imagesPath;

      return NULL;
    }
  }

  //
  // Create 16 directories in the path to
  // hold the images whose name begins with
  // the corresponding hexadecimal digit.
  //

  char *digitPath = new char[strlen(imagesPath) + 5];

  strcpy(digitPath, imagesPath);

  //
  // Image paths have format "[path][/I-c][\0]",
  // where c is the first digit of the checksum.
  //

  for (char digit = 0; digit < 16; digit++)
  {
    sprintf(digitPath + strlen(imagesPath), "/I-%01X", digit);

    if ((stat(digitPath, &dirStat) == -1) && (EGET() == ENOENT))
    {
      if (mkdir(digitPath, 0700) < 0 && (EGET() != EEXIST))
      {
        nxfatal << "Loop: PANIC! Can't create directory '" << digitPath
                << ". Error is " << EGET() << " '" << ESTR() << "'.\n"
                << std::flush;

        cerr << "Error" << ": Can't create directory '" << digitPath
             << ". Error is " << EGET() << " '" << ESTR() << "'.\n";

        delete [] rootPath;
        delete [] imagesPath;
        delete [] digitPath;

        return NULL;
      }
    }
  }

  delete [] rootPath;
  delete [] digitPath;

  return imagesPath;
}

char *GetSessionPath()
{
  if (*sessionDir == '\0')
  {
    char *rootPath = GetRootPath();

    strcpy(sessionDir, rootPath);

    if (control -> ProxyMode == proxy_client)
    {
      strcat(sessionDir, "/C-");
    }
    else
    {
      strcat(sessionDir, "/S-");
    }

    if (*sessionId == '\0')
    {
      char port[DEFAULT_STRING_LENGTH];

      sprintf(port, "%d", proxyPort);

      strcpy(sessionId, port);
    }

    strcat(sessionDir, sessionId);

    struct stat dirStat;

    if ((stat(sessionDir, &dirStat) == -1) && (EGET() == ENOENT))
    {
      if (mkdir(sessionDir, 0700) < 0 && (EGET() != EEXIST))
      {
        nxfatal << "Loop: PANIC! Can't create directory '" << sessionDir
                << ". Error is " << EGET() << " '" << ESTR() << "'.\n"
                << std::flush;

        cerr << "Error" << ": Can't create directory '" << sessionDir
             << ". Error is " << EGET() << " '" << ESTR() << "'.\n";

        delete [] rootPath;

        return NULL;
      }
    }

    nxinfo << "Loop: Root of NX session is '" << sessionDir
           << "'.\n" << std::flush;

    delete [] rootPath;
  }

  char *sessionPath = new char[strlen(sessionDir) + 1];

  strcpy(sessionPath, sessionDir);

  return sessionPath;
}

//
// Identify requested link characteristics
// and set control parameters accordingly.
//

int ParseLinkOption(const char *opt)
{
  //
  // Normalize the user input.
  //

  if (strcasecmp(opt, "modem") == 0 ||
          strcasecmp(opt, "33k") == 0 ||
              strcasecmp(opt, "56k") == 0)
  {
    strcpy(linkSpeedName, "MODEM");
  }
  else if (strcasecmp(opt, "isdn")  == 0 ||
               strcasecmp(opt, "64k")  == 0 ||
                   strcasecmp(opt, "128k") == 0)
  {
    strcpy(linkSpeedName, "ISDN");
  }
  else if (strcasecmp(opt, "adsl") == 0 ||
               strcasecmp(opt, "256k") == 0 ||
                   strcasecmp(opt, "640k") == 0)
  {
    strcpy(linkSpeedName, "ADSL");
  }
  else if (strcasecmp(opt, "wan")  == 0 ||
               strcasecmp(opt, "1m")  == 0 ||
                   strcasecmp(opt, "2m")  == 0 ||
                       strcasecmp(opt, "34m") == 0)
  {
    strcpy(linkSpeedName, "WAN");
  }
  else if (strcasecmp(opt, "lan")   == 0 ||
               strcasecmp(opt, "10m")   == 0 ||
                   strcasecmp(opt, "100m")  == 0 ||
                       strcasecmp(opt, "local") == 0)
  {
    strcpy(linkSpeedName, "LAN");
  }

  if (strcasecmp(linkSpeedName, "modem") != 0 &&
          strcasecmp(linkSpeedName, "isdn")  != 0 &&
              strcasecmp(linkSpeedName, "adsl")  != 0 &&
                  strcasecmp(linkSpeedName, "wan")   != 0 &&
                      strcasecmp(linkSpeedName, "lan")   != 0)
  {
    return -1;
  }

  return 1;
}

int ParsePackOption(const char *opt)
{
  nxdbg << "Loop: Pack method is " << packMethod
        << " quality is " << packQuality << ".\n"
        << std::flush;

  nxdbg << "Loop: Parsing pack method '" << opt
        << "'.\n" << std::flush;

  if (strcasecmp(opt, "0") == 0 ||
          strcasecmp(opt, "none") == 0 ||
              strcasecmp(opt, "nopack") == 0 ||
                  strcasecmp(opt, "no-pack") == 0)
  {
    packMethod = PACK_NONE;
  }
  else if (strcasecmp(opt, "8") == 0)
  {
    packMethod = PACK_MASKED_8_COLORS;
  }
  else if (strcasecmp(opt, "64") == 0)
  {
    packMethod = PACK_MASKED_64_COLORS;
  }
  else if (strcasecmp(opt, "256") == 0)
  {
    packMethod = PACK_MASKED_256_COLORS;
  }
  else if (strcasecmp(opt, "512") == 0)
  {
    packMethod = PACK_MASKED_512_COLORS;
  }
  else if (strcasecmp(opt, "4k") == 0)
  {
    packMethod = PACK_MASKED_4K_COLORS;
  }
  else if (strcasecmp(opt, "32k") == 0)
  {
    packMethod = PACK_MASKED_32K_COLORS;
  }
  else if (strcasecmp(opt, "64k") == 0)
  {
    packMethod = PACK_MASKED_64K_COLORS;
  }
  else if (strcasecmp(opt, "256k") == 0)
  {
    packMethod = PACK_MASKED_256K_COLORS;
  }
  else if (strcasecmp(opt, "2m") == 0)
  {
    packMethod = PACK_MASKED_2M_COLORS;
  }
  else if (strcasecmp(opt, "16m") == 0)
  {
    packMethod = PACK_MASKED_16M_COLORS;
  }
  else if (strncasecmp(opt, "8-jpeg", strlen("8-jpeg")) == 0)
  {
    packMethod = PACK_JPEG_8_COLORS;
  }
  else if (strncasecmp(opt, "64-jpeg", strlen("64-jpeg")) == 0)
  {
    packMethod = PACK_JPEG_64_COLORS;
  }
  else if (strncasecmp(opt, "256-jpeg", strlen("256-jpeg")) == 0)
  {
    packMethod = PACK_JPEG_256_COLORS;
  }
  else if (strncasecmp(opt, "512-jpeg", strlen("512-jpeg")) == 0)
  {
    packMethod = PACK_JPEG_512_COLORS;
  }
  else if (strncasecmp(opt, "4k-jpeg", strlen("4k-jpeg")) == 0)
  {
    packMethod = PACK_JPEG_4K_COLORS;
  }
  else if (strncasecmp(opt, "32k-jpeg", strlen("32k-jpeg")) == 0)
  {
    packMethod = PACK_JPEG_32K_COLORS;
  }
  else if (strncasecmp(opt, "64k-jpeg", strlen("64k-jpeg")) == 0)
  {
    packMethod = PACK_JPEG_64K_COLORS;
  }
  else if (strncasecmp(opt, "256k-jpeg", strlen("256k-jpeg")) == 0)
  {
    packMethod = PACK_JPEG_256K_COLORS;
  }
  else if (strncasecmp(opt, "2m-jpeg", strlen("2m-jpeg")) == 0)
  {
    packMethod = PACK_JPEG_2M_COLORS;
  }
  else if (strncasecmp(opt, "16m-jpeg", strlen("16m-jpeg")) == 0)
  {
    packMethod = PACK_JPEG_16M_COLORS;
  }
  else if (strncasecmp(opt, "8-png", strlen("8-png")) == 0)
  {
    packMethod = PACK_PNG_8_COLORS;
  }
  else if (strncasecmp(opt, "64-png", strlen("64-png")) == 0)
  {
    packMethod = PACK_PNG_64_COLORS;
  }
  else if (strncasecmp(opt, "256-png", strlen("256-png")) == 0)
  {
    packMethod = PACK_PNG_256_COLORS;
  }
  else if (strncasecmp(opt, "512-png", strlen("512-png")) == 0)
  {
    packMethod = PACK_PNG_512_COLORS;
  }
  else if (strncasecmp(opt, "4k-png", strlen("4k-png")) == 0)
  {
    packMethod = PACK_PNG_4K_COLORS;
  }
  else if (strncasecmp(opt, "32k-png", strlen("32k-png")) == 0)
  {
    packMethod = PACK_PNG_32K_COLORS;
  }
  else if (strncasecmp(opt, "64k-png", strlen("64k-png")) == 0)
  {
    packMethod = PACK_PNG_64K_COLORS;
  }
  else if (strncasecmp(opt, "256k-png", strlen("256k-png")) == 0)
  {
    packMethod = PACK_PNG_256K_COLORS;
  }
  else if (strncasecmp(opt, "2m-png", strlen("2m-png")) == 0)
  {
    packMethod = PACK_PNG_2M_COLORS;
  }
  else if (strncasecmp(opt, "16m-png", strlen("16m-png")) == 0)
  {
    packMethod = PACK_PNG_16M_COLORS;
  }
  else if (strncasecmp(opt, "16m-rgb", strlen("16m-rgb")) == 0 ||
               strncasecmp(opt, "rgb", strlen("rgb")) == 0)
  {
    packMethod = PACK_RGB_16M_COLORS;
  }
  else if (strncasecmp(opt, "16m-rle", strlen("16m-rle")) == 0 ||
               strncasecmp(opt, "rle", strlen("rle")) == 0)
  {
    packMethod = PACK_RLE_16M_COLORS;
  }
  else if (strncasecmp(opt, "16m-bitmap", strlen("16m-bitmap")) == 0 ||
               strncasecmp(opt, "bitmap", strlen("bitmap")) == 0)
  {
    packMethod = PACK_BITMAP_16M_COLORS;
  }
  else if (strncasecmp(opt, "lossy", strlen("lossy")) == 0)
  {
    packMethod = PACK_LOSSY;
  }
  else if (strncasecmp(opt, "lossless", strlen("lossless")) == 0)
  {
    packMethod = PACK_LOSSLESS;
  }
  else if (strncasecmp(opt, "adaptive", strlen("adaptive")) == 0)
  {
    packMethod = PACK_ADAPTIVE;
  }
  else
  {
    return -1;
  }

  if (packMethod == PACK_NONE)
  {
    strcpy(packMethodName, "none");
  }
  else
  {
    strcpy(packMethodName, opt);
  }

  if (packMethod == PACK_RGB_16M_COLORS ||
          packMethod == PACK_RLE_16M_COLORS ||
              packMethod == PACK_BITMAP_16M_COLORS ||
                  (packMethod >= PACK_JPEG_8_COLORS &&
                      packMethod <= PACK_JPEG_16M_COLORS) ||
                          (packMethod >= PACK_PNG_8_COLORS &&
                              packMethod <= PACK_PNG_16M_COLORS) ||
                                  packMethod == PACK_LOSSY ||
                                      packMethod == PACK_LOSSLESS ||
                                          packMethod == PACK_ADAPTIVE)
  {
    const char *dash = strrchr(opt, '-');

    if (dash != NULL && strlen(dash) == 2 &&
            *(dash + 1) >= '0' && *(dash + 1) <= '9')
    {
      packQuality = atoi(dash + 1);

      nxdbg << "Loop: Using pack quality '"
            << packQuality << "'.\n" << std::flush;
    }
  }
  else
  {
    packQuality = 0;
  }

  return 1;
}

int ParsePackMethod(const int method, const int quality)
{
  switch (method)
  {
    case PACK_NONE:
    {
      strcpy(packMethodName, "none");

      break;
    }
    case PACK_MASKED_8_COLORS:
    {
      strcpy(packMethodName, "8");

      break;
    }
    case PACK_MASKED_64_COLORS:
    {
      strcpy(packMethodName, "64");

      break;
    }
    case PACK_MASKED_256_COLORS:
    {
      strcpy(packMethodName, "256");

      break;
    }
    case PACK_MASKED_512_COLORS:
    {
      strcpy(packMethodName, "512");

      break;
    }
    case PACK_MASKED_4K_COLORS:
    {
      strcpy(packMethodName, "4k");

      break;
    }
    case PACK_MASKED_32K_COLORS:
    {
      strcpy(packMethodName, "32k");

      break;
    }
    case PACK_MASKED_64K_COLORS:
    {
      strcpy(packMethodName, "64k");

      break;
    }
    case PACK_MASKED_256K_COLORS:
    {
      strcpy(packMethodName, "256k");

      break;
    }
    case PACK_MASKED_2M_COLORS:
    {
      strcpy(packMethodName, "2m");

      break;
    }
    case PACK_MASKED_16M_COLORS:
    {
      strcpy(packMethodName, "16m");

      break;
    }
    case PACK_JPEG_8_COLORS:
    {
      strcpy(packMethodName, "8-jpeg");

      break;
    }
    case PACK_JPEG_64_COLORS:
    {
      strcpy(packMethodName, "64-jpeg");

      break;
    }
    case PACK_JPEG_256_COLORS:
    {
      strcpy(packMethodName, "256-jpeg");

      break;
    }
    case PACK_JPEG_512_COLORS:
    {
      strcpy(packMethodName, "512-jpeg");

      break;
    }
    case PACK_JPEG_4K_COLORS:
    {
      strcpy(packMethodName, "4k-jpeg");

      break;
    }
    case PACK_JPEG_32K_COLORS:
    {
      strcpy(packMethodName, "32k-jpeg");

      break;
    }
    case PACK_JPEG_64K_COLORS:
    {
      strcpy(packMethodName, "64k-jpeg");

      break;
    }
    case PACK_JPEG_256K_COLORS:
    {
      strcpy(packMethodName, "256k-jpeg");

      break;
    }
    case PACK_JPEG_2M_COLORS:
    {
      strcpy(packMethodName, "2m-jpeg");

      break;
    }
    case PACK_JPEG_16M_COLORS:
    {
      strcpy(packMethodName, "16m-jpeg");

      break;
    }
    case PACK_PNG_8_COLORS:
    {
      strcpy(packMethodName, "8-png");

      break;
    }
    case PACK_PNG_64_COLORS:
    {
      strcpy(packMethodName, "64-png");

      break;
    }
    case PACK_PNG_256_COLORS:
    {
      strcpy(packMethodName, "256-png");

      break;
    }
    case PACK_PNG_512_COLORS:
    {
      strcpy(packMethodName, "512-png");

      break;
    }
    case PACK_PNG_4K_COLORS:
    {
      strcpy(packMethodName, "4k-png");

      break;
    }
    case PACK_PNG_32K_COLORS:
    {
      strcpy(packMethodName, "32k-png");

      break;
    }
    case PACK_PNG_64K_COLORS:
    {
      strcpy(packMethodName, "64k-png");

      break;
    }
    case PACK_PNG_256K_COLORS:
    {
      strcpy(packMethodName, "256k-png");

      break;
    }
    case PACK_PNG_2M_COLORS:
    {
      strcpy(packMethodName, "2m-png");

      break;
    }
    case PACK_PNG_16M_COLORS:
    {
      strcpy(packMethodName, "16m-png");

      break;
    }
    case PACK_RGB_16M_COLORS:
    {
      strcpy(packMethodName, "16m-rgb");

      break;
    }
    case PACK_RLE_16M_COLORS:
    {
      strcpy(packMethodName, "16m-rle");

      break;
    }
    case PACK_BITMAP_16M_COLORS:
    {
      strcpy(packMethodName, "16m-bitmap");

      break;
    }
    case PACK_LOSSY:
    {
      strcpy(packMethodName, "lossy");

      break;
    }
    case PACK_LOSSLESS:
    {
      strcpy(packMethodName, "lossless");

      break;
    }
    case PACK_ADAPTIVE:
    {
      strcpy(packMethodName, "adaptive");

      break;
    }
    default:
    {
      return -1;
    }
  }

  if (quality < 0 || quality > 9)
  {
    return -1;
  }

  if (packMethod == PACK_RGB_16M_COLORS ||
          packMethod == PACK_RLE_16M_COLORS ||
              packMethod == PACK_BITMAP_16M_COLORS ||
                  (packMethod >= PACK_JPEG_8_COLORS &&
                      packMethod <= PACK_JPEG_16M_COLORS) ||
                          (packMethod >= PACK_PNG_8_COLORS &&
                              packMethod <= PACK_PNG_16M_COLORS) ||
                                  packMethod == PACK_LOSSY ||
                                      packMethod == PACK_LOSSLESS ||
                                          packMethod == PACK_ADAPTIVE)
  {
    sprintf(packMethodName + strlen(packMethodName),
                "-%d", quality);
  }

  packMethod  = method;
  packQuality = quality;

  control -> PackMethod  = packMethod;
  control -> PackQuality = packQuality;

  return 1;
}

int SetDirectories()
{
  //
  // Determine the location of the user's NX
  // directory and the other relevant paths.
  // The functions below will check the pa-
  // rameters passed to the program and will
  // query the environment, if needed.
  //

  control -> HomePath   = GetHomePath();
  control -> RootPath   = GetRootPath();
  control -> SystemPath = GetSystemPath();
  control -> TempPath   = GetTempPath();
  control -> ClientPath = GetClientPath();

  return 1;
}

int SetLogs()
{
  //
  // So far we used stderr (or stdout under
  // WIN32). Now use the files selected by
  // the user.
  //

  if (*statsFileName == '\0')
  {
    strcpy(statsFileName, "stats");

    nxinfo << "Loop: Assuming default statistics file '"
           << statsFileName << "'.\n" << std::flush;
  }
  else
  {
    nxinfo << "Loop: Name selected for statistics is '"
           << statsFileName << "'.\n" << std::flush;
  }

  if (OpenLogFile(statsFileName, statofs) < 0)
  {
    HandleCleanup();
  }

  #ifndef MIXED

  if (*errorsFileName == '\0')
  {
    strcpy(errorsFileName, "errors");

    nxinfo << "Loop: Assuming default log file name '"
           << errorsFileName << "'.\n" << std::flush;
  }
  else
  {
    nxinfo << "Loop: Name selected for log file is '"
           << errorsFileName << "'.\n" << std::flush;
  }

  //
  // Share the bebug output with the nxssh binder
  // process. The file must be made writable by
  // everybody because the nxssh process is run by
  // nxserver as the nx user.
  //

  #ifdef BINDER

  strcpy(errorsFileName, "/tmp/errors");

  ostream *tmpfs = new ofstream(errorsFileName, ios::out);

  delete tmpfs;

  chmod(errorsFileName, S_IRUSR | S_IWUSR | S_IRGRP |
            S_IWGRP | S_IROTH | S_IWOTH);

  #endif

  if (OpenLogFile(errorsFileName, logofs) < 0)
  {
    HandleCleanup();
  }

  //
  // By default the session log is the standard error
  // of the process. It is anyway required to set the
  // option when running inside SSH, otherwise the
  // output will go to the same file as the SSH log,
  // depending where the NX client has redirected the
  // output.
  //

  if (*sessionFileName != '\0')
  {
    nxinfo << "Loop: Name selected for session file is '"
           << sessionFileName << "'.\n" << std::flush;

    if (errofs != NULL)
    {
      nxwarn << "Loop: WARNING! Unexpected value for stream errofs.\n"
             << std::flush;

      cerr << "Warning" << ": Unexpected value for stream errofs.\n";
    }

    if (errsbuf != NULL)
    {
      nxwarn << "Loop: WARNING! Unexpected value for buffer errsbuf.\n"
             << std::flush;

      cerr << "Warning" << ": Unexpected value for buffer errsbuf.\n";
    }

    errofs  = NULL;
    errsbuf = NULL;

    if (OpenLogFile(sessionFileName, errofs) < 0)
    {
      HandleCleanup();
    }

    //
    // Redirect the standard error to the file.
    //

    errsbuf = cerr.rdbuf(errofs -> rdbuf());
  }

  #endif

  return 1;
}

int SetPorts()
{
  //
  // Depending on the proxy side, we need to determine on which
  // port to listen for the given protocol or to which port we
  // will have to forward the connection. Three possibilities
  // are given for each supported protocol:
  //
  // Port <= 0: Disable port forwarding.
  // Port == 1: Use the default port.
  // Port >  1: Use the specified port.
  //
  // At the connectiong side the user should always explicitly
  // set the ports where the connections will be forwarded. This
  // is both for security reasons and because, when running both
  // proxies on the same host, there is a concrete possibility
  // that, by using the default ports, the connection will be
  // forwarded to the same port where the peer proxy is listen-
  // ing, causing a loop.
  //

  if (control -> ProxyMode == proxy_client) {
    // ChannelEndPoint::enabled() implements the logic described above,
    // and takes the default port into consideration. If cups=1, and
    // there is a default port, then enabled() will return true.
    //
    // Therefore, we must set the default port before calling this
    // function.
    cupsPort.setDefaultTCPPort(DEFAULT_NX_CUPS_PORT_OFFSET + proxyPort);
    useCupsSocket = cupsPort.enabled();
  } else {
    cupsPort.setDefaultTCPPort(631);
  }



  nxinfo << "Loop: cups port: '" << cupsPort
         << "'.\n" << std::flush;

  if (control -> ProxyMode == proxy_client) {
    auxPort.setDefaultTCPPort(DEFAULT_NX_AUX_PORT_OFFSET + proxyPort);
    useAuxSocket = auxPort.enabled();
  } else {
    auxPort.setDefaultTCPPort(1);

    if ( auxPort.getTCPPort() != 1 ) {
        nxwarn << "Loop: WARNING! Overriding auxiliary X11 "
               << "port with new value '" << 1 << "'.\n"
               << std::flush;

      cerr << "Warning" << ": Overriding auxiliary X11 "
           << "port with new value '" << 1 << "'.\n";

      auxPort.setSpec("1");
    }
  }

  nxinfo << "Loop: aux port: '" << auxPort
         << "'.\n" << std::flush;

  if (control -> ProxyMode == proxy_client) {
    smbPort.setDefaultTCPPort(DEFAULT_NX_SMB_PORT_OFFSET + proxyPort);
    useSmbSocket = smbPort.enabled();
  } else {
    smbPort.setDefaultTCPPort(139);
  }


  nxinfo << "Loop: smb port: '" << smbPort
         << "'.\n" << std::flush;

  if ( mediaPort.configured() ) {
    if (control -> ProxyMode == proxy_client) {
      mediaPort.setDefaultTCPPort(DEFAULT_NX_MEDIA_PORT_OFFSET + proxyPort);
      useMediaSocket = mediaPort.enabled();
    } else {

      if ( mediaPort.getTCPPort() < 0 ) {
        nxfatal << "Loop: PANIC! No port specified for multimedia connections.\n"
                << std::flush;

        cerr << "Error" << ": No port specified for multimedia connections.\n";

        HandleCleanup();
      }
    }
  }

  nxinfo << "Loop: Using multimedia port '" << mediaPort
         << "'.\n" << std::flush;

  if (control -> ProxyMode == proxy_client) {
      httpPort.setDefaultTCPPort(DEFAULT_NX_HTTP_PORT_OFFSET + proxyPort);
      useHttpSocket = httpPort.enabled();
  } else {
      httpPort.setDefaultTCPPort(80);
  }

  nxinfo << "Loop: Using HTTP port '" << httpPort
         << "'.\n" << std::flush;

  if (ParseFontPath(fontPort) <= 0)
  {
    nxinfo << "Loop: Disabling font server connections.\n"
           << std::flush;

    *fontPort = '\0';

    useFontSocket = 0;
  }
  else
  {
    //
    // We don't know yet if the remote proxy supports
    // the font server connections. If needed, we will
    // disable the font server connections at later
    // time.
    //

    if (control -> ProxyMode == proxy_server)
    {
      useFontSocket = 1;
    }
    else
    {
      useFontSocket = 0;
    }

    nxinfo << "Loop: Using font server port '" << fontPort
           << "'.\n" << std::flush;
  }

  if (control -> ProxyMode == proxy_client) {
    slavePort.setDefaultTCPPort(DEFAULT_NX_SLAVE_PORT_CLIENT_OFFSET + proxyPort);
  } else {
    slavePort.setDefaultTCPPort(DEFAULT_NX_SLAVE_PORT_SERVER_OFFSET + proxyPort);
  }

  useSlaveSocket = slavePort.enabled();

  nxinfo << "Loop: Using slave port '" << slavePort
         << "'.\n" << std::flush;

  return 1;
}

int SetDescriptors()
{
  unsigned int limit = 0;

  #ifdef RLIMIT_NOFILE

  rlimit limits;

  if (getrlimit(RLIMIT_NOFILE, &limits) == 0)
  {
    if (limits.rlim_max == RLIM_INFINITY)
    {
      limit = 0;
    }
    else
    {
      limit = (unsigned int) limits.rlim_max;
    }
  }

  #endif

  #ifdef _SC_OPEN_MAX

  if (limit == 0)
  {
    limit = sysconf(_SC_OPEN_MAX);
  }

  #endif

  #ifdef FD_SETSIZE

  if (limit > FD_SETSIZE)
  {
    limit = FD_SETSIZE;
  }

  #endif

  #ifdef RLIMIT_NOFILE

  if (limits.rlim_cur < limit)
  {
    limits.rlim_cur = limit;

    setrlimit(RLIMIT_NOFILE, &limits);
  }

  #endif

  if (limit == 0)
  {
    nxfatal << "Loop: PANIC! Cannot determine number of available "
            << "file descriptors.\n" << std::flush;

    cerr << "Error" << ": Cannot determine number of available "
         << "file descriptors.\n";

    return -1;
  }

  return 1;
}

//
// Find the directory containing the caches
// matching the session type.
//

int SetCaches()
{
  if ((control -> PersistentCachePath = GetCachePath()) == NULL)
  {
    nxfatal << "Loop: PANIC! Error getting or creating the cache path.\n"
            << std::flush;

    cerr << "Error" << ": Error getting or creating the cache path.\n";

    HandleCleanup();
  }

  nxinfo << "Loop: Path of cache files is '" << control -> PersistentCachePath
         << "'.\n" << std::flush;

  return 1;
}

//
// Initialize all configuration parameters.
//

int SetParameters()
{
  //
  // Find out the type of session.
  //

  SetSession();

  //
  // Initialize the network and compression
  // parameters according to the settings
  // suggested by the user.
  //

  SetLink();

  //
  // Set compression according to link speed.
  //

  SetCompression();

  //
  // Be sure that we have a literal for current
  // cache size. Value will reflect control's
  // default unless we already parsed a 'cache'
  // option. Server side has no control on size
  // of cache but is informed at session nego-
  // tiation about how much memory is going to
  // be used.
  //

  SetStorage();

  //
  // Set size of shared memory segments.
  //

  SetShmem();

  //
  // Make adjustments to cache based
  // on the pack method.
  //

  SetPack();

  //
  // Set disk-based image cache.
  //

  SetImages();

  //
  // Set CPU and bandwidth limits.
  //

  SetLimits();

  return 1;
}

//
// According to session literal determine
// the type of traffic that is going to be
// transported. Literals should be better
// standardized in future NX versions.
//

int SetSession()
{
  if (strncmp(sessionType, "agent", strlen("agent")) == 0 ||
          strncmp(sessionType, "desktop", strlen("desktop")) == 0 ||
              strncmp(sessionType, "rootless", strlen("rootless")) == 0 ||
                  strncmp(sessionType, "console", strlen("console")) == 0 ||
                      strncmp(sessionType, "default", strlen("default")) == 0 ||
                          strncmp(sessionType, "gnome", strlen("gnome")) == 0 ||
                              strncmp(sessionType, "kde", strlen("kde")) == 0 ||
                                  strncmp(sessionType, "cde", strlen("cde")) == 0 ||
                                      strncmp(sessionType, "xdm", strlen("xdm")) == 0)
  {
    control -> SessionMode = session_agent;
  }
  else if (strncmp(sessionType, "win", strlen("win")) == 0 ||
               strncmp(sessionType, "vnc", strlen("vnc")) == 0)
  {
    control -> SessionMode = session_agent;
  }
  else if (strncmp(sessionType, "shadow", strlen("shadow")) == 0)
  {
    control -> SessionMode = session_shadow;
  }
  else if (strncmp(sessionType, "proxy", strlen("proxy")) == 0 ||
               strncmp(sessionType, "application", strlen("application")) == 0 ||
                   strncmp(sessionType, "raw", strlen("raw")) == 0)
  {
    control -> SessionMode = session_proxy;
  }
  else
  {
    //
    // If the session type is not passed or
    // it is not among the recognized strings,
    // we assume that the proxy is connected
    // to the agent.
    //

    //
    // Since ProtoStep8 (#issue 108) and also
    // with older "unix-" sessions
    //

    if (*sessionType != '\0')
    {
      nxwarn << "Loop: WARNING! Unrecognized session type '"
             << sessionType << "'. Assuming agent session.\n"
             << std::flush;

      cerr << "Warning" << ": Unrecognized session type '"
           << sessionType << "'. Assuming agent session.\n";
    }

    control -> SessionMode = session_agent;
  }

  nxinfo << "Loop: Assuming session type '"
         << DumpSession(control -> SessionMode) << "' with "
         << "string '" << sessionType << "'.\n"
         << std::flush;

  //
  // By default the policy is immediate. Agents
  // will set a different policy, if they like.
  // Anyway we need to check if the user has
  // provided a custom flush policy.
  //

  if (usePolicy != -1)
  {
    if (usePolicy > 0)
    {
      control -> FlushPolicy = policy_deferred;
    }
    else
    {
      control -> FlushPolicy = policy_immediate;
    }

    nxinfo << "Loop: WARNING! Forcing flush policy to '"
           << DumpPolicy(control -> FlushPolicy)
           << ".\n" << std::flush;
  }
  else
  {
    control -> FlushPolicy = policy_immediate;

    nxinfo << "Loop: Setting initial flush policy to '"
           << DumpPolicy(control -> FlushPolicy)
           << "'.\n" << std::flush;
  }

  //
  // Check if the proxy library is run inside
  // another program providing encryption, as
  // it is the case of the SSH client.
  //

  if (useEncryption != -1)
  {
    if (useEncryption > 0)
    {
      control -> LinkEncrypted = 1;
    }
    else
    {
      control -> LinkEncrypted = 0;
    }
  }

  if (control -> LinkEncrypted == 1)
  {
    nxinfo << "Loop: Proxy running as part of an "
           << "encrypting client.\n"
           << std::flush;
  }
  else
  {
    nxinfo << "Loop: Assuming proxy running as a "
           << "standalone program.\n"
           << std::flush;
  }

  //
  // Check if the system administrator has
  // enabled the respawn of the client at
  // the end of session.
  //

  if (control -> ProxyMode == proxy_server)
  {
    struct stat fileStat;

    char fileName[DEFAULT_STRING_LENGTH];

    snprintf(fileName, DEFAULT_STRING_LENGTH - 1,
                 "%s/share/noexit", control -> SystemPath);

    *(fileName + DEFAULT_STRING_LENGTH - 1) = '\0';

    if (stat(fileName, &fileStat) == 0)
    {
      nxinfo << "Loop: Enabling respawn of client at session shutdown.\n"
             << std::flush;

      control -> EnableRestartOnShutdown = 1;
    }
  }

  return 1;
}

int SetStorage()
{
  //
  // If differential compression is disabled
  // we don't need a cache at all.
  //

  if (control -> LocalDeltaCompression == 0)
  {
    control -> ClientTotalStorageSize = 0;
    control -> ServerTotalStorageSize = 0;
  }

  //
  // Set a a cache size literal.
  //

  int size = control -> getUpperStorageSize();

  if (size / 1024 > 0)
  {
    sprintf(cacheSizeName, "%dk", size / 1024);
  }
  else
  {
    sprintf(cacheSizeName, "%d", size);
  }

  if (control -> ProxyMode == proxy_client)
  {
    control -> LocalTotalStorageSize =
        control -> ClientTotalStorageSize;

    control -> RemoteTotalStorageSize =
        control -> ServerTotalStorageSize;
  }
  else
  {
    control -> LocalTotalStorageSize =
        control -> ServerTotalStorageSize;

    control -> RemoteTotalStorageSize =
        control -> ClientTotalStorageSize;
  }

  nxdbg << "Loop: Storage size limit is "
        << control -> ClientTotalStorageSize
        << " at client and "
        << control -> ServerTotalStorageSize
        << " at server.\n"
        << std::flush;

  nxdbg << "Loop: Storage local limit set to "
        << control -> LocalTotalStorageSize
        << " remote limit set to "
        << control -> RemoteTotalStorageSize
        << ".\n" << std::flush;

  //
  // Never reserve for split store more than
  // half the memory available for messages.
  //

  if (size > 0 && control ->
          SplitTotalStorageSize > size / 2)
  {
    nxinfo << "Loop: Reducing size of split store to "
           << size / 2 << " bytes.\n"
           << std::flush;

    control -> SplitTotalStorageSize = size / 2;
  }

  //
  // Don't load render from persistent
  // cache if extension is hidden or
  // not supported by agent.
  //

  if (control -> HideRender == 1)
  {
    nxinfo << "Loop: Not loading render extension "
           << "from persistent cache.\n"
           << std::flush;

    control -> PersistentCacheLoadRender = 0;
  }

  return 1;
}

int SetShmem()
{
  //
  // If not set, adjust the size of the shared
  // memory segment according to size of the
  // message cache.
  //

  if (*shsegSizeName == '\0')
  {
    int size = control -> getUpperStorageSize();

    const int mega = 1048576;

    if (size > 0)
    {
      if (size <= 1 * mega)
      {
        size = 0;
      }
      else if (size <= 2 * mega)
      {
        size = 524288;
      }
      else if (size < 4 * mega)
      {
        size = 1048576;
      }
      else
      {
        size = size / 4;
      }

      if (size > 4194304)
      {
        size = 4194304;
      }

      control -> ShmemClientSize = size;
      control -> ShmemServerSize = size;
    }
    else
    {
      //
      // The delta compression is disabled.
      // Use a default segment size of 2 MB.
      //

      control -> ShmemServerSize = 2 * mega;
    }
  }

  //
  // Client side shared memory support is
  // not useful and not implemented.
  //

  if (control -> ShmemServerSize >= 524288)
  {
    control -> ShmemServer = 1;

    nxinfo << "Loop: Set initial shared memory size "
           << "to " << control -> ShmemServerSize
           << " bytes.\n" << std::flush;
  }
  else
  {
    nxinfo << "Loop: Disabled use of the shared memory "
           << "extension.\n" << std::flush;

    control -> ShmemServer = 0;
  }

  // For android, no shared memory available
  control -> ShmemServer = 0;
  control -> ShmemClientSize = 0;

  return 1;
}

//
// Adjust the pack method according to the
// type of the session.
//

int SetPack()
{
  nxinfo << "Loop: Setting pack with initial method "
         << packMethod << " and quality " << packQuality
         << ".\n" << std::flush;

  //
  // Check if this is a proxy session and, in
  // this case, set the pack method to none.
  // Packed images are not supported by plain
  // X applications.
  //

  if (control -> SessionMode == session_proxy)
  {
    nxinfo << "Loop: WARNING! Disabling pack with proxy session.\n"
           << std::flush;

    packMethod = PACK_NONE;
  }

  //
  // Adjust the internal settings according
  // to the newly selected pack method.
  //

  ParsePackMethod(packMethod, packQuality);

  //
  // Don't load messages from persistent
  // cache if packed images are disabled.
  //

  if (control -> PackMethod == PACK_NONE)
  {
    control -> PersistentCacheLoadPacked = 0;

    nxinfo << "Loop: Not loading packed images "
           << "from persistent cache.\n"
           << std::flush;
  }

  return 1;
}

//
// Set the disk-based image cache parameters
// according to the user's wishes.
//

int SetImages()
{
  //
  // Be sure we disable the image cache if we
  // are connecting to plain X clients.
  //

  if (control -> SessionMode == session_proxy)
  {
    nxinfo << "Loop: Disabling image cache with "
           << "session '" << DumpSession(control ->
              SessionMode) << "'.\n" << std::flush;

    sprintf(imagesSizeName, "0");

    control -> ImageCacheEnableLoad = 0;
    control -> ImageCacheEnableSave = 0;

    return 1;
  }

  int size = control -> ImageCacheDiskLimit;

  if (size / 1024 > 0)
  {
    sprintf(imagesSizeName, "%dk", size / 1024);
  }
  else
  {
    sprintf(imagesSizeName, "%d", size);
  }

  if (size > 0)
  {
    control -> ImageCacheEnableLoad = 1;
    control -> ImageCacheEnableSave = 1;

    if (control -> ProxyMode == proxy_server)
    {
      if ((control -> ImageCachePath = GetImagesPath()) == NULL)
      {
        nxfatal << "Loop: PANIC! Error getting or creating image cache path.\n"
                << std::flush;

        cerr << "Error" << ": Error getting or creating image cache path.\n";

        HandleCleanup();
      }

      nxinfo << "Loop: Path of image cache files is '" << control -> ImageCachePath
             << "'.\n" << std::flush;
    }
  }
  else
  {
    nxinfo << "Loop: Disabling the persistent image cache.\n"
           << std::flush;

    control -> ImageCacheEnableLoad = 0;
    control -> ImageCacheEnableSave = 0;
  }

  return 1;
}

int SetVersion()
{
  //
  // Normalize the different proxy versions.
  //

  int local = (control -> LocalVersionMajor << 24) |
                  (control -> LocalVersionMinor << 16) |
                      control -> LocalVersionPatch;

  int remote = (control -> RemoteVersionMajor << 24) |
                   (control -> RemoteVersionMinor << 16) |
                       control -> RemoteVersionPatch;

  int major = -1;
  int minor = -1;
  int patch = -1;

  if (control -> RemoteVersionMajor <= 1)
  {
    //
    // The remote proxy uses a different
    // logic to determine the version so
    // we default to the compatibility
    // version.
    //

    major = control -> CompatVersionMajor;
    minor = control -> CompatVersionMinor;
    patch = control -> CompatVersionPatch;

    nxinfo << "Loop: Using compatibility version '"
           << major << "." << minor << "." << patch
           << "'.\n" << std::flush;
  }
  else if (control -> LocalVersionMajor >
               control -> RemoteVersionMajor)
  {
    //
    // We use a more recent version. Let's
    // negotiate the version based on the
    // version supported by the remote.
    //

    major = control -> RemoteVersionMajor;
    minor = control -> RemoteVersionMinor;
    patch = control -> RemoteVersionPatch;

    nxinfo << "Loop: Using remote version '"
           << major << "." << minor << "." << patch
           << "'.\n" << std::flush;
  }
  else
  {
    //
    // We support a major version that is
    // equal or older than the remote. We
    // assume the smaller version between
    // the two, including the minor and
    // the patch numbers.
    //

    if (local > remote)
    {
      major = control -> RemoteVersionMajor;
      minor = control -> RemoteVersionMinor;
      patch = control -> RemoteVersionPatch;

      nxinfo << "Loop: Using remote version '"
             << major << "." << minor << "." << patch
             << "'.\n" << std::flush;
    }
    else
    {
      major = control -> LocalVersionMajor;
      minor = control -> LocalVersionMinor;
      patch = control -> LocalVersionPatch;

      nxinfo << "Loop: Using local version '"
             << major << "." << minor << "." << patch
             << "'.\n" << std::flush;
    }
  }

  //
  // Handle versions from 3.5.0. The protocol
  // step 10 is the minimum supported version.
  //

  int step = 0;

  if (major == 3)
  {
    if (minor >= 5)
    {
      step = 10;
    }
  }
  else if (major > 3)
  {
    step = 10;
  }

  if (step == 0)
  {
    nxfatal << "Loop: PANIC! Unable to set the protocol step value from "
            << "the negotiated protocol version " << major << "." << minor
            << "." << patch << ".\n" << std::flush;

    cerr << "Error" << ": Unable to set the protocol step value from "
         << "the negotiated protocol version " << major << "." << minor
         << "." << patch << ".\n";

    nxfatal << "Loop: PANIC! Incompatible remote version "
            << control -> RemoteVersionMajor << "." << control -> RemoteVersionMinor
            << "." << control -> RemoteVersionPatch << " with local version "
            << control -> LocalVersionMajor << "." << control -> LocalVersionMinor
            << "." << control -> LocalVersionPatch << ".\n" << std::flush;

    cerr << "Error" << ": Incompatible remote version "
         << control -> RemoteVersionMajor << "." << control -> RemoteVersionMinor
         << "." << control -> RemoteVersionPatch << " with local version "
         << control -> LocalVersionMajor << "." << control -> LocalVersionMinor
         << "." << control -> LocalVersionPatch << ".\n";

    return -1;
  }

  nxinfo << "Loop: Using NX protocol step "
         << step << ".\n" << std::flush;

  control -> setProtoStep(step);

  //
  // Ignore the differences in patch version
  // and print a warning if the local version
  // is different or obsolete compared to
  // the remote.
  //

  local  &= 0xffff0000;
  remote &= 0xffff0000;

  if (local != remote)
  {
    nxwarn << "Loop: WARNING! Connected to remote version "
           << control -> RemoteVersionMajor << "." << control -> RemoteVersionMinor
           << "." << control -> RemoteVersionPatch << " with local version "
           << control -> LocalVersionMajor << "." << control -> LocalVersionMinor
           << "." << control -> LocalVersionPatch << ".\n" << std::flush;

    cerr << "Warning" << ": Connected to remote version "
         << control -> RemoteVersionMajor << "." << control -> RemoteVersionMinor
         << "." << control -> RemoteVersionPatch << " with local version "
         << control -> LocalVersionMajor << "." << control -> LocalVersionMinor
         << "." << control -> LocalVersionPatch << ".\n" << logofs_flush;
  }

  if (local < remote)
  {
    nxerr << "Warning" << ": Consider checking https://github.com/ArcticaProject/nx-libs/releases for updates.\n";
  }

  //
  // Now that we are aware of the remote
  // version, let's adjust the options to
  // be compatible with the remote proxy.
  //

  if (control -> ProxyMode == proxy_client)
  {
    //
    // Since ProtoStep8 (#issue 108)
    //
    // Now it's assumed that the remote is
    // able to handle the selected pack
    // method
    //

    nxinfo << __FILE__ << " : " << __LINE__ << " - "
           << "step = " << control -> getProtoStep()
           << " packMethod = " << packMethod
           << " packQuality = " << packQuality
           << ".\n"  << std::flush;

    //
    // Update the pack method name.
    //

    ParsePackMethod(packMethod, packQuality);
  }

  //
  // At the moment the image cache is not used by the
  // agent. Proxy versions older than 3.0.0 assumed
  // that it was enabled and sent specific bits as part
  // of the encoding. Conversely, it is advisable to
  // disable the cache right now. By not enabling the
  // the image cache, the house-keeping process will
  // only take care of cleaning up the "cache-" direc-
  // tories.
  //

  //
  // Considering that compatibility with older versions
  // has been set to cover as far as 3.5.0, the cache can
  // be disabled at this point without any concern
  //

  // Since ProtoStep8 (#issue 108)
  nxinfo << "Loop: Disabling image cache with protocol "
         << "step '" << control -> getProtoStep()
         << "'.\n"  << std::flush;

  sprintf(imagesSizeName, "0");

  control -> ImageCacheEnableLoad = 0;
  control -> ImageCacheEnableSave = 0;

  return 1;
}

//
// Identify the requested link settings
// and update the control parameters
// accordingly.
//

int SetLink()
{
  nxinfo << "Loop: Setting link with initial value "
         << linkSpeedName << ".\n" << std::flush;

  if (*linkSpeedName == '\0')
  {
    strcpy(linkSpeedName, "lan");
  }

  nxinfo << "Loop: Link speed is " << linkSpeedName
         << ".\n" << std::flush;

  if (strcasecmp(linkSpeedName, "modem") == 0)
  {
    SetLinkModem();
  }
  else if (strcasecmp(linkSpeedName, "isdn") == 0)
  {
    SetLinkIsdn();
  }
  else if (strcasecmp(linkSpeedName, "adsl") == 0)
  {
    SetLinkAdsl();
  }
  else if (strcasecmp(linkSpeedName, "wan") == 0)
  {
    SetLinkWan();
  }
  else if (strcasecmp(linkSpeedName, "lan") == 0)
  {
    SetLinkLan();
  }
  else
  {
    return -1;
  }

  //
  // Set TCP_NODELAY according to the user's
  // wishes.
  //

  if (useNoDelay != -1)
  {
    control -> OptionProxyClientNoDelay = useNoDelay;
    control -> OptionProxyServerNoDelay = useNoDelay;
  }

  //
  // Select the image compression method.
  //

  if (packMethod == -1)
  {
    packMethod = control -> PackMethod;
  }

  if (packQuality == -1)
  {
    packQuality = control -> PackQuality;
  }

  if (ParsePackMethod(packMethod, packQuality) < 0)
  {
    nxfatal << "Loop: PANIC! Unrecognized pack method id "
            << packMethod << " with quality " << packQuality
            << ".\n" << std::flush;

    cerr << "Error" << ": Unrecognized pack method id "
         << packMethod << " with quality " << packQuality
         << ".\n";

    HandleCleanup();
  }

  //
  // Check if the user disabled the ability
  // to generate simple replies at the client
  // side.
  //

  if (control -> SessionMode == session_proxy)
  {
    if (useTaint != -1)
    {
      control -> TaintReplies = (useTaint == 1);
    }
    else
    {
      nxwarn << "Loop: WARNING! Forcing taint of replies "
             << "with a proxy session.\n"
             << std::flush;

      control -> TaintReplies = 1;
    }
  }
  else
  {
    //
    // There is no need to taint the
    // replies if we have an agent.
    //

    control -> TaintReplies = 0;
  }

  //
  // Be sure that the requests needing a reply
  // are flushed immediately. Normal X clients
  // use so many replies to make the queuing
  // completely useless.
  //

  if (control -> SessionMode == session_proxy)
  {
    nxwarn << "Loop: WARNING! Forcing flush on priority "
           << "with a proxy session.\n"
           << std::flush;

    control -> FlushPriority = 1;
  }

  return 1;
}

//
// Parameters for MODEM 28.8/33.6/56 Kbps.
//

int SetLinkModem()
{
  nxinfo << "Loop: Setting parameters for MODEM.\n"
         << std::flush;

  control -> LinkMode = LINK_TYPE_MODEM;

  control -> TokenSize  = 256;
  control -> TokenLimit = 24;

  control -> SplitMode             = 1;
  control -> SplitTotalSize        = 128;
  control -> SplitTotalStorageSize = 1048576;

  control -> SplitTimeout   = 50;
  control -> MotionTimeout  = 50;
  control -> IdleTimeout    = 50;

  control -> PackMethod  = PACK_ADAPTIVE;
  control -> PackQuality = 3;

  return 1;
}

//
// Parameters for ISDN 64/128 Kbps.
//

int SetLinkIsdn()
{
  nxinfo << "Loop: Setting parameters for ISDN.\n"
         << std::flush;

  control -> LinkMode = LINK_TYPE_ISDN;

  control -> TokenSize  = 384;
  control -> TokenLimit = 24;

  control -> SplitMode             = 1;
  control -> SplitTotalSize        = 128;
  control -> SplitTotalStorageSize = 1048576;

  control -> SplitTimeout   = 50;
  control -> MotionTimeout  = 20;
  control -> IdleTimeout    = 50;

  control -> PackMethod  = PACK_ADAPTIVE;
  control -> PackQuality = 5;

  return 1;
}

//
// Parameters for ADSL 256 Kbps.
//

int SetLinkAdsl()
{
  nxinfo << "Loop: Setting parameters for ADSL.\n"
         << std::flush;

  control -> LinkMode = LINK_TYPE_ADSL;

  control -> TokenSize  = 1408;
  control -> TokenLimit = 24;

  control -> SplitMode             = 1;
  control -> SplitTotalSize        = 128;
  control -> SplitTotalStorageSize = 1048576;

  control -> SplitTimeout   = 50;
  control -> MotionTimeout  = 10;
  control -> IdleTimeout    = 50;

  control -> PackMethod  = PACK_ADAPTIVE;
  control -> PackQuality = 7;

  return 1;
}

//
// Parameters for XDSL/FDDI/ATM 1/2/34 Mbps WAN.
//

int SetLinkWan()
{
  nxinfo << "Loop: Setting parameters for WAN.\n"
         << std::flush;

  control -> LinkMode = LINK_TYPE_WAN;

  control -> TokenSize  = 1408;
  control -> TokenLimit = 24;

  control -> SplitMode             = 1;
  control -> SplitTotalSize        = 128;
  control -> SplitTotalStorageSize = 1048576;

  control -> SplitTimeout   = 50;
  control -> MotionTimeout  = 5;
  control -> IdleTimeout    = 50;

  control -> PackMethod  = PACK_ADAPTIVE;
  control -> PackQuality = 9;

  return 1;
}

//
// Parameters for LAN 10/100 Mbps.
//

int SetLinkLan()
{
  nxinfo << "Loop: Setting parameters for LAN.\n"
         << std::flush;

  control -> LinkMode = LINK_TYPE_LAN;

  control -> TokenSize  = 1536;
  control -> TokenLimit = 24;

  control -> SplitMode             = 1;
  control -> SplitTotalSize        = 128;
  control -> SplitTotalStorageSize = 1048576;

  control -> SplitTimeout   = 50;
  control -> MotionTimeout  = 0;
  control -> IdleTimeout    = 50;

  control -> PackMethod  = PACK_ADAPTIVE;
  control -> PackQuality = 9;

  return 1;
}

//
// Identify the requested link type and set
// the control parameters accordingly.
//

int SetCompression()
{
  if (strcasecmp(linkSpeedName, "modem") == 0)
  {
    SetCompressionModem();
  }
  else if (strcasecmp(linkSpeedName, "isdn") == 0)
  {
    SetCompressionIsdn();
  }
  else if (strcasecmp(linkSpeedName, "adsl") == 0)
  {
    SetCompressionAdsl();
  }
  else if (strcasecmp(linkSpeedName, "wan") == 0)
  {
    SetCompressionWan();
  }
  else if (strcasecmp(linkSpeedName, "lan") == 0)
  {
    SetCompressionLan();
  }
  else
  {
    return -1;
  }

  if (control -> LocalDeltaCompression < 0)
  {
    control -> LocalDeltaCompression = 1;
  }

  //
  // If we didn't set remote delta compression
  // (as it should always be the case at client
  // side) assume value of local side.
  //

  if (control -> RemoteDeltaCompression < 0)
  {
    control -> RemoteDeltaCompression =
        control -> LocalDeltaCompression;
  }

  //
  // If we didn't set remote compression levels
  // assume values of local side.
  //

  if (control -> RemoteStreamCompression < 0)
  {
    control -> RemoteStreamCompressionLevel =
        control -> LocalStreamCompressionLevel;

    if (control -> RemoteStreamCompressionLevel > 0)
    {
      control -> RemoteStreamCompression = 1;
    }
    else
    {
      control -> RemoteStreamCompression = 0;
    }
  }

  if (control -> RemoteDataCompression < 0)
  {
    control -> RemoteDataCompressionLevel =
        control -> LocalDataCompressionLevel;

    if (control -> RemoteDataCompressionLevel > 0)
    {
      control -> RemoteDataCompression = 1;
    }
    else
    {
      control -> RemoteDataCompression = 0;
    }
  }

  return 1;
}

//
// Compression for MODEM.
//

int SetCompressionModem()
{
  if (control -> LocalDataCompression < 0)
  {
    control -> LocalDataCompression      = 1;
    control -> LocalDataCompressionLevel = 1;
  }

  if (control -> LocalDataCompressionThreshold < 0)
  {
    control -> LocalDataCompressionThreshold = 32;
  }

  if (control -> LocalStreamCompression < 0)
  {
    control -> LocalStreamCompression      = 1;
    control -> LocalStreamCompressionLevel = 9;
  }

  return 1;
}

//
// Compression for ISDN.
//

int SetCompressionIsdn()
{
  if (control -> LocalDataCompression < 0)
  {
    control -> LocalDataCompression      = 1;
    control -> LocalDataCompressionLevel = 1;
  }

  if (control -> LocalDataCompressionThreshold < 0)
  {
    control -> LocalDataCompressionThreshold = 32;
  }

  if (control -> LocalStreamCompression < 0)
  {
    control -> LocalStreamCompression      = 1;
    control -> LocalStreamCompressionLevel = 6;
  }

  return 1;
}

//
// Compression for ADSL.
//

int SetCompressionAdsl()
{
  if (control -> LocalDataCompression < 0)
  {
    control -> LocalDataCompression      = 1;
    control -> LocalDataCompressionLevel = 1;
  }

  if (control -> LocalDataCompressionThreshold < 0)
  {
    control -> LocalDataCompressionThreshold = 32;
  }

  if (control -> LocalStreamCompression < 0)
  {
    control -> LocalStreamCompression      = 1;
    control -> LocalStreamCompressionLevel = 4;
  }

  return 1;
}

//
// Compression for WAN.
//

int SetCompressionWan()
{
  if (control -> LocalDataCompression < 0)
  {
    control -> LocalDataCompression      = 1;
    control -> LocalDataCompressionLevel = 1;
  }

  if (control -> LocalDataCompressionThreshold < 0)
  {
    control -> LocalDataCompressionThreshold = 32;
  }

  if (control -> LocalStreamCompression < 0)
  {
    control -> LocalStreamCompression      = 1;
    control -> LocalStreamCompressionLevel = 1;
  }

  return 1;
}

//
// Compression for LAN.
//

int SetCompressionLan()
{
  //
  // Disable delta compression if not
  // explicitly enabled.
  //

  if (control -> LocalDeltaCompression < 0)
  {
    control -> LocalDeltaCompression = 0;
  }

  if (control -> LocalDataCompression < 0)
  {
    control -> LocalDataCompression      = 0;
    control -> LocalDataCompressionLevel = 0;
  }

  if (control -> LocalDataCompressionThreshold < 0)
  {
    control -> LocalDataCompressionThreshold = 0;
  }

  if (control -> LocalStreamCompression < 0)
  {
    control -> LocalStreamCompression      = 0;
    control -> LocalStreamCompressionLevel = 0;
  }

  return 1;
}

int SetLimits()
{
  //
  // Check if the user requested strict
  // control flow parameters.
  //

  if (useStrict == 1)
  {
    nxinfo << "Loop: LIMIT! Decreasing the token limit "
           << "to " << control -> TokenLimit / 2
           << " with option 'strict'.\n"
           << std::flush;

    control -> TokenLimit /= 2;
  }

  #ifdef STRICT

  control -> TokenLimit = 1;

  nxinfo << "Loop: WARNING! LIMIT! Setting the token limit "
         << "to " << control -> TokenLimit
         << " to simulate the proxy congestion.\n"
         << std::flush;

  #endif

  //
  // Reduce the size of the log file.
  //

  #ifdef QUOTA

  control -> FileSizeLimit = 8388608;

  #endif

  //
  // Check the bitrate limits.
  //

  if (control -> LocalBitrateLimit == -1)
  {
    if (control -> ProxyMode == proxy_client)
    {
      control -> LocalBitrateLimit =
          control -> ClientBitrateLimit;
    }
    else
    {
      control -> LocalBitrateLimit =
          control -> ServerBitrateLimit;
    }
  }

  nxinfo << "Loop: LIMIT! Setting client bitrate limit "
         << "to " << control -> ClientBitrateLimit
         << " server bitrate limit to " << control ->
            ServerBitrateLimit << " with local limit "
         << control -> LocalBitrateLimit << ".\n"
         << std::flush;

  return 1;
}

//
// These functions are used to parse literal
// values provided by the user and set the
// control parameters accordingly.
//

int ParseCacheOption(const char *opt)
{
  int size = ParseArg("", "cache", opt);

  if (size < 0)
  {
    nxfatal << "Loop: PANIC! Invalid value '"
            << opt << "' for option 'cache'.\n"
            << std::flush;

    cerr << "Error" << ": Invalid value '"
         << opt << "' for option 'cache'.\n";

    return -1;
  }

  nxinfo << "Loop: Setting size of cache to "
         << size << " bytes.\n" << std::flush;

  control -> ClientTotalStorageSize = size;
  control -> ServerTotalStorageSize = size;

  strcpy(cacheSizeName, opt);

  if (size == 0)
  {
    nxwarn << "Loop: WARNING! Disabling NX delta compression.\n"
           << std::flush;

    control -> LocalDeltaCompression = 0;

    nxwarn << "Loop: WARNING! Disabling use of NX persistent cache.\n"
           << std::flush;

    control -> PersistentCacheEnableLoad = 0;
    control -> PersistentCacheEnableSave = 0;
  }

  return 1;
}

int ParseImagesOption(const char *opt)
{
  int size = ParseArg("", "images", opt);

  if (size < 0)
  {
    nxfatal << "Loop: PANIC! Invalid value '"
            << opt << "' for option 'images'.\n"
            << std::flush;

    cerr << "Error" << ": Invalid value '"
         << opt << "' for option 'images'.\n";

    return -1;
  }

  nxinfo << "Loop: Setting size of images cache to "
         << size << " bytes.\n" << std::flush;

  control -> ImageCacheDiskLimit = size;

  strcpy(imagesSizeName, opt);

  return 1;
}

int ParseShmemOption(const char *opt)
{
  int size = ParseArg("", "shseg", opt);

  if (size < 0)
  {
    nxfatal << "Loop: PANIC! Invalid value '"
            << opt << "' for option 'shseg'.\n"
            << std::flush;

    cerr << "Error" << ": Invalid value '"
         << opt << "' for option 'shseg'.\n";

    return -1;
  }

  control -> ShmemClientSize = size;
  control -> ShmemServerSize = size;

  nxinfo << "Loop: Set shared memory size to "
         << control -> ShmemServerSize << " bytes.\n"
         << std::flush;

  strcpy(shsegSizeName, opt);

  return 1;
}

int ParseBitrateOption(const char *opt)
{
  int bitrate = ParseArg("", "limit", opt);

  if (bitrate < 0)
  {
    nxfatal << "Loop: PANIC! Invalid value '"
            << opt << "' for option 'limit'.\n"
            << std::flush;

    cerr << "Error" << ": Invalid value '"
         << opt << "' for option 'limit'.\n";

    return -1;
  }

  strcpy(bitrateLimitName, opt);

  if (bitrate == 0)
  {
    nxinfo << "Loop: Disabling bitrate limit on proxy link.\n"
           << std::flush;

    control -> LocalBitrateLimit = 0;
  }
  else
  {
    nxinfo << "Loop: Setting bitrate to " << bitrate
           << " bits per second.\n" << std::flush;

    //
    // Internal representation is in bytes
    // per second.
    //

    control -> LocalBitrateLimit = bitrate >> 3;
  }

  return 1;
}

int ParseHostOption(const char *opt, char *host, long &port)
{
  nxinfo << "Loop: Trying to parse options string '" << opt
         << "' as a remote NX host.\n" << std::flush;

  if (opt == NULL || *opt == '\0')
  {
    nxfatal << "Loop: PANIC! No host parameter provided.\n"
            << std::flush;

    return 0;
  }
  else if (strlen(opt) >= DEFAULT_STRING_LENGTH)
  {
    nxfatal << "Loop: PANIC! Host parameter exceeds length of "
            << DEFAULT_STRING_LENGTH << " characters.\n"
            << std::flush;

    return 0;
  }

  //
  // Look for a host name followed
  // by a colon followed by port.
  //

  int newPort = port;

  const char *separator = strrchr(opt, ':');

  if (separator != NULL)
  {
    const char *check = separator + 1;

    while (*check != '\0' && *check != ',' &&
               *check != '=' && isdigit(*check) != 0)
    {
      check++;
    }

    newPort = atoi(separator + 1);

    if (newPort < 0 || *check != '\0')
    {
      nxinfo << "Loop: Can't identify remote NX port in string '"
             << separator << "'.\n" << std::flush;

      return 0;
    }
  }
  else if (newPort < 0)
  {
    //
    // Complain if port was not passed
    // by other means.
    //

    nxinfo << "Loop: Can't identify remote NX port in string '"
           << opt << "'.\n" << std::flush;

    return 0;
  }
  else
  {
    separator = opt + strlen(opt);
  }

  char newHost[DEFAULT_STRING_LENGTH] = { 0 };

  // opt cannot be longer than DEFAULT_STRING_LENGTH, this is checked above
  strncpy(newHost, opt, strlen(opt) - strlen(separator));

  *(newHost + strlen(opt) - strlen(separator)) = '\0';

  const char *check = newHost;

  while (*check != '\0' && *check != ',' &&
             *check != '=')
  {
    check++;
  }

  if (*check != '\0')
  {
    nxinfo << "Loop: Can't identify remote NX host in string '"
           << newHost << "'.\n" << std::flush;

    return 0;
  }
  else if (*acceptHost != '\0')
  {
    nxfatal << "Loop: PANIC! Can't manage to connect and accept connections "
            << "at the same time.\n" << std::flush;

    nxfatal << "Loop: PANIC! Refusing remote NX host with string '"
            << opt << "'.\n" << std::flush;

    cerr << "Error" << ": Can't manage to connect and accept connections "
         << "at the same time.\n";

    cerr << "Error" << ": Refusing remote NX host with string '"
         << opt << "'.\n";

    return -1;
  }

  if (*host != '\0' && strcmp(host, newHost) != 0)
  {
    nxwarn << "Loop: WARNING! Overriding remote NX host '"
           << host << "' with new value '" << newHost
           << "'.\n" << std::flush;
  }

  strcpy(host, newHost);

  if (port != -1 && port != newPort)
  {
    nxwarn << "Loop: WARNING! Overriding remote NX port '"
           << port << "' with new value '" << newPort
           << "'.\n" << std::flush;
  }

  nxinfo << "Loop: Parsed options string '" << opt
         << "' with host '" << newHost << "' and port '"
         << newPort << "'.\n" << std::flush;

  port = newPort;

  return 1;
}

int ParseFontPath(char *path)
{
  char oldPath[DEFAULT_STRING_LENGTH];

  strcpy(oldPath, path);

  if (path == NULL || *path == '\0' || strcmp(path, "0") == 0)
  {
    return 0;
  }

  nxinfo << "Loop: Parsing font server option '" << path
         << "'.\n" << std::flush;

  //
  // Convert the value to our default port.
  //

  if (strcmp(fontPort, "1") == 0)
  {
    if (control -> ProxyMode == proxy_server)
    {
      snprintf(fontPort, DEFAULT_STRING_LENGTH - 1, "%d",
                   DEFAULT_NX_FONT_PORT_OFFSET + proxyPort);
    }
    else
    {
      //
      // Let the client use the well-known
      // "unix/:7100" font path.
      //

      snprintf(fontPort, DEFAULT_STRING_LENGTH - 1, "unix/:7100");
    }
  }

  //
  // Check if a simple numaric value was given.
  //

  if (atoi(path) > 0)
  {
    nxinfo << "Loop: Assuming numeric TCP port '" << atoi(path)
           << "' for font server.\n" << std::flush;

    return 1;
  }

  //
  // Let's assume that a port specification "unix/:7100"
  // corresponds to "/tmp/.font-unix/fs7100" and a port
  // "unix/:-1" corresponds to "/tmp/.font-unix/fs-1".
  //

  if (strncmp("unix/:", path, 6) == 0)
  {
    snprintf(path, DEFAULT_STRING_LENGTH - 1, "/tmp/.font-unix/fs%s",
                 oldPath + 6);

    *(path + DEFAULT_STRING_LENGTH - 1) = '\0';

    nxinfo << "Loop: Assuming Unix socket '" << path
           << "' for font server.\n" << std::flush;
  }
  else if (strncmp("tcp/:", path, 5) == 0)
  {
    snprintf(path, DEFAULT_STRING_LENGTH - 1, "%d", atoi(oldPath + 5));

    *(path + DEFAULT_STRING_LENGTH - 1) = '\0';

    if (atoi(path) <= 0)
    {
      goto ParseFontPathError;
    }

    nxinfo << "Loop: Assuming TCP port '" << atoi(path)
           << "' for font server.\n" << std::flush;
  }
  else
  {
    //
    // Accept an absolute file path as
    // a valid Unix socket.
    //

    if (*path != '/')
    {
      goto ParseFontPathError;
    }

    nxinfo << "Loop: Assuming Unix socket '" << path
           << "' for font server.\n" << std::flush;
  }

  return 1;

ParseFontPathError:

  nxinfo << "Loop: Unable to determine the font server "
         << "port in string '" << path << "'.\n"
         << std::flush;

  return -1;
}

int OpenLogFile(char *name, ostream *&stream)
{
  if (name == NULL || *name == '\0')
  {
    nxinfo << "Loop: WARNING! No name provided for output. Using standard error.\n"
           << std::flush;

    if (stream == NULL)
    {
      stream = &cerr;
    }

    return 1;
  }

  if (stream == NULL || stream == &cerr)
  {
    if (*name != '/' && *name != '.')
    {
      char *filePath = GetSessionPath();

      if (filePath == NULL)
      {
        nxfatal << "Loop: PANIC! Cannot determine directory of NX session file.\n"
                << std::flush;

        cerr << "Error" << ": Cannot determine directory of NX session file.\n";

        return -1;
      }

      if (strlen(filePath) + strlen("/") +
              strlen(name) + 1 > DEFAULT_STRING_LENGTH)
      {
        nxfatal << "Loop: PANIC! Full name of NX file '" << name
                << " would exceed length of " << DEFAULT_STRING_LENGTH
                << " characters.\n" << std::flush;

        cerr << "Error" << ": Full name of NX file '" << name
             << " would exceed length of " << DEFAULT_STRING_LENGTH
             << " characters.\n";

        return -1;
      }

      char *file = new char[strlen(filePath) + strlen("/") +
                                strlen(name) + 1];

      //
      // Transform name in a fully qualified name.
      //

      strcpy(file, filePath);
      strcat(file, "/");
      strcat(file, name);

      strcpy(name, file);

      delete [] filePath;
      delete [] file;
    }

    mode_t fileMode = umask(0077);

    for (;;)
    {
      if ((stream = new ofstream(name, ios::app)) != NULL)
      {
        break;
      }

      usleep(200000);
    }

    umask(fileMode);
  }
  else
  {
    nxfatal << "Loop: PANIC! Bad stream provided for output.\n"
            << std::flush;

    cerr << "Error" << ": Bad stream provided for output.\n";

    return -1;
  }

  return 1;
}

int ReopenLogFile(char *name, ostream *&stream, int limit)
{
  if (*name != '\0' && limit >= 0)
  {
    struct stat fileStat;

    if (limit > 0)
    {
      //
      // This is used for the log file, if the
      // size exceeds the limit.
      //

      if (stat(name, &fileStat) != 0)
      {
        nxwarn << "Loop: WARNING! Can't get stats of file '"
               << name  << "'. Error is " << EGET()
               << " '" << ESTR() << "'.\n" << std::flush;

        return 0;
      }
      else if (fileStat.st_size < (long) limit)
      {
        return 0;
      }
    }

    nxinfo << "Loop: Deleting file '" << name
           << "' with size " << fileStat.st_size
           << ".\n" << std::flush;

    //
    // Create a new stream over the previous
    // file. Trying to delete the file fails
    // to work on recent Cygwin installs.
    //

    *stream << flush;

    delete stream;

    mode_t fileMode = umask(0077);

    for (;;)
    {
      if ((stream = new ofstream(name, ios::out)) != NULL)
      {
        break;
      }

      usleep(200000);
    }

    umask(fileMode);

    nxinfo << "Loop: Reopened file '" << name
           << "'.\n" << std::flush;
  }

  return 1;
}

void PrintProcessInfo()
{
  if (agent == NULL)
  {

    cerr << endl;

    PrintVersionInfo();

    cerr << endl;

    cerr << GetCopyrightInfo()
         << endl
         << GetOtherCopyrightInfo()
         << endl
         << "See https://github.com/ArcticaProject/nx-libs for more information." << endl << endl;
  }

  //
  // People get confused by the fact that client
  // mode is running on NX server and viceversa.
  // Let's adopt an user-friendly naming conven-
  // tion here.
  //

  cerr << "Info: Proxy running in "
       << (control -> ProxyMode == proxy_client ? "client" : "server")
       << " mode with pid '" << getpid() << "'.\n";

  if (agent == NULL)
  {
    cerr << "Session" << ": Starting session at '"
         << strTimestamp() << "'.\n";
  }


  if (*errorsFileName != '\0')
  {
    cerr << "Info" << ": Using errors file '" << errorsFileName << "'.\n";
  }

  if (*statsFileName != '\0')
  {
    cerr << "Info" << ": Using stats file '" << statsFileName << "'.\n";
  }

}

void PrintConnectionInfo()
{
  cerr << "Info" << ": Using "
       << linkSpeedName << " link parameters "
       << control -> TokenSize
       << "/" << control -> TokenLimit
       << "/" << control -> FlushPolicy + 1
       << "/" << control -> FlushPriority
       << ".\n";

  if (control -> ProxyMode == proxy_client)
  {
    cerr << "Info" << ": Using agent parameters "
         << control -> PingTimeout
         << "/" << control -> MotionTimeout
         << "/" << control -> IdleTimeout
         << "/" << control -> TaintReplies
         << "/" << control -> HideRender
         << ".\n";
  }

  if (control -> LocalDeltaCompression == 1)
  {
    cerr << "Info" << ": Using cache parameters "
         << control -> MinimumMessageSize
         << "/" << control -> MaximumMessageSize / 1024 << "KB"
         << "/" << control -> ClientTotalStorageSize / 1024 << "KB"
         << "/" << control -> ServerTotalStorageSize / 1024 << "KB"
         << ".\n";
  }

  if (control -> ImageCacheEnableLoad == 1 ||
          control -> ImageCacheEnableSave == 1)
  {
    cerr << "Info" << ": Using image streaming parameters "
         << control -> SplitTimeout
         << "/" << control -> SplitTotalSize
         << "/" << control -> SplitTotalStorageSize / 1024 << "KB"
         << "/" << control -> SplitDataThreshold
         << "/" << control -> SplitDataPacketLimit
         << ".\n";

    cerr << "Info" << ": Using image cache parameters "
         << control -> ImageCacheEnableLoad
         << "/" << control -> ImageCacheEnableSave
         << "/" << control -> ImageCacheDiskLimit / 1024 << "KB"
         << ".\n";
  }

  cerr << "Info" << ": Using pack method '"
       << packMethodName << "' with session '"
       << sessionType << "'.\n";

  if (*productName != '\0')
  {
    cerr << "Info" << ": Using product '" << productName
         << "'.\n" << logofs_flush;
  }

  if (control -> LocalDeltaCompression == 0)
  {
    cerr << "Info" << ": Not using NX delta compression.\n";
  }

  if (control -> LocalDataCompression == 1 ||
          control -> RemoteDataCompression == 1)
  {
    cerr << "Info" << ": Using ZLIB data compression "
         << control -> LocalDataCompressionLevel
         << "/" << control -> RemoteDataCompressionLevel
         << "/" << control -> LocalDataCompressionThreshold
         << ".\n";
  }
  else
  {
    cerr << "Info" << ": Not using ZLIB data compression.\n";
  }

  if (control -> LocalStreamCompression == 1 ||
          control -> RemoteStreamCompression == 1)
  {
    cerr << "Info" << ": Using ZLIB stream compression "
         << control -> LocalStreamCompressionLevel
         << "/" << control -> RemoteStreamCompressionLevel
         << ".\n";
  }
  else
  {
    cerr << "Info" << ": Not using ZLIB stream compression.\n";
  }

  if (control -> LocalBitrateLimit > 0)
  {
    cerr << "Info" << ": Using bandwidth limit of "
         << bitrateLimitName << " bits per second.\n";
  }

  if (control -> PersistentCacheName != NULL)
  {
    cerr << "Info" << ": Using cache file '"
         << control -> PersistentCachePath << "/"
         << control -> PersistentCacheName << "'.\n";
  }
  else
  {
    if (control -> PersistentCacheEnableLoad == 0 ||
            control -> LocalDeltaCompression == 0)
    {
      cerr << "Info" << ": Not using a persistent cache.\n";
    }
    else
    {
      cerr << "Info" << ": No suitable cache file found.\n";
    }
  }

  if (control -> ProxyMode == proxy_client &&
          (useUnixSocket > 0 || useTcpSocket > 0 ||
              useAgentSocket > 0))
  {
    cerr << "Info" << ": Listening to X11 connections "
         << "on display ':" << xPort << "'.\n";
  }
  else if (control -> ProxyMode == proxy_server)
  {
    cerr << "Info" << ": Forwarding X11 connections "
         << "to display '" << displayHost << "'.\n";
  }

  if (control -> ProxyMode == proxy_client &&
      useCupsSocket > 0 && cupsPort.enabled())
  {
    cerr << "Info" << ": Listening to CUPS connections "
         << "on port '" << cupsPort << "'.\n";
  }
  else if (control -> ProxyMode == proxy_server &&
           cupsPort.enabled())
  {
    cerr << "Info" << ": Forwarding CUPS connections "
         << "to port '" << cupsPort << "'.\n";
  }

  if (control -> ProxyMode == proxy_client &&
      useAuxSocket > 0 && auxPort.enabled())
  {
    cerr << "Info" << ": Listening to auxiliary X11 connections "
         << "on port '" << auxPort << "'.\n";
  }
  else if (control -> ProxyMode == proxy_server &&
           auxPort.enabled())
  {
    cerr << "Info" << ": Forwarding auxiliary X11 connections "
         << "to display '" << displayHost << "'.\n";
  }

  if (control -> ProxyMode == proxy_client &&
      useSmbSocket > 0 && smbPort.enabled())
  {
    cerr << "Info" << ": Listening to SMB connections "
         << "on port '" << smbPort << "'.\n";
  }
  else if (control -> ProxyMode == proxy_server &&
           smbPort.enabled())
  {
    cerr << "Info" << ": Forwarding SMB connections "
         << "to port '" << smbPort << "'.\n";
  }

  if (control -> ProxyMode == proxy_client &&
      useMediaSocket > 0 && mediaPort.enabled())
  {
    cerr << "Info" << ": Listening to multimedia connections "
         << "on port '" << mediaPort << "'.\n";
  }
  else if (control -> ProxyMode == proxy_server &&
           mediaPort.enabled())
  {
    cerr << "Info" << ": Forwarding multimedia connections "
         << "to port '" << mediaPort << "'.\n";
  }

  if (control -> ProxyMode == proxy_client &&
      useHttpSocket > 0 && httpPort.enabled())
  {
    cerr << "Info" << ": Listening to HTTP connections "
         << "on port '" << httpPort << "'.\n";
  }
  else if (control -> ProxyMode == proxy_server &&
           httpPort.enabled())
  {
    cerr << "Info" << ": Forwarding HTTP connections "
         << "to port '" << httpPort << "'.\n";
  }

  if (control -> ProxyMode == proxy_server &&
          useFontSocket > 0 && *fontPort != '\0')
  {
    cerr << "Info" << ": Listening to font server connections "
         << "on port '" << fontPort << "'.\n";
  }
  else if (control -> ProxyMode == proxy_client &&
               *fontPort != '\0')
  {
    cerr << "Info" << ": Forwarding font server connections "
         << "to port '" << fontPort << "'.\n";
  }

  if (useSlaveSocket > 0 && slavePort.enabled())
  {
    cerr << "Info" << ": Listening to slave connections "
         << "on port '" << slavePort << "'.\n";
  }
}

void PrintVersionInfo()
{
  cerr << "NXPROXY - Version "
#ifdef NX_VERSION_CUSTOM
       << NX_VERSION_CUSTOM << " ("
#endif
       << control -> LocalVersionMajor << "."
       << control -> LocalVersionMinor << "."
       << control -> LocalVersionPatch << "."
       << control -> LocalVersionMaintenancePatch
#ifdef NX_VERSION_CUSTOM
       << ")"
#endif
       << endl;
}

void PrintCopyrightInfo()
{
  cerr << endl;

  PrintVersionInfo();

  cerr << endl;

  cerr << GetCopyrightInfo();

  //
  // Print third party's copyright info.
  //

  cerr << endl;

  cerr << GetOtherCopyrightInfo();

  cerr << endl;
}

void PrintOptionIgnored(const char *type, const char *name, const char *value)
{
  if (control -> ProxyMode == proxy_server)
  {
    nxwarn << "Loop: WARNING! Ignoring " << type
           << " option '" << name << "' with value '"
           << value << "' at " << "NX client side.\n"
           << std::flush;

    cerr << "Warning" << ": Ignoring " << type
         << " option '" << name << "' with value '"
         << value << "' at " << "NX client side.\n";
  }
  else
  {
    nxwarn << "Loop: WARNING! Ignoring " << type
           << " option '" << name << "' with value '"
           << value << "' at " << "NX server side.\n"
           << std::flush;

    cerr << "Warning" << ": Ignoring " << type
         << " option '" << name << "' with value '"
         << value << "' at " << "NX server side.\n";
  }
}

const char *GetOptions(const char *options)
{
  if (options != NULL)
  {
    if (strncasecmp(options, "nx/nx,", 6) != 0 &&
            strncasecmp(options, "nx,", 3) != 0 &&
                strncasecmp(options, "nx:", 3) != 0)
    {
      nxinfo << "Loop: PANIC! Display options string '" << options
             << "' must start with 'nx' or 'nx/nx' prefix.\n"
             << std::flush;

      cerr << "Error" << ": Display options string '" << options
           << "' must start with 'nx' or 'nx/nx' prefix.\n";

      HandleCleanup();
    }
  }
  else
  {
    options = getenv("DISPLAY");
  }

  return options;
}

const char *GetArg(int &argi, int argc, const char **argv)
{
  //
  // Skip "-" and flag character.
  //

  const char *arg = argv[argi] + 2;

  if (*arg == 0)
  {
    if (argi + 1 == argc)
    {
      return NULL;
    }
    else
    {
      argi++;

      return (*argv[argi] == '-' ? NULL : argv[argi]);
    }
  }
  else
  {
    return (*arg == '-' ? NULL : arg);
  }
}

int CheckArg(const char *type, const char *name, const char *value)
{
  nxinfo << "Loop: Parsing " << type << " option '" << name
         << "' with value '" << (value ? value : "(null)")
         << "'.\n" << std::flush;

  if (value == NULL || strstr(value, "=") != NULL)
  {
    nxfatal << "Loop: PANIC! Error in " << type << " option '"
            << name << "'. No value found.\n"
            << std::flush;

    cerr << "Error" << ": Error in " << type << " option '"
         << name << "'. No value found.\n";

    return -1;
  }
  else if (strstr(name, ",") != NULL)
  {
    nxfatal << "Loop: PANIC! Parse error at " << type << " option '"
            << name << "'.\n" << std::flush;

    cerr << "Error" << ": Parse error at " << type << " option '"
         << name << "'.\n";

    return -1;
  }
  else if (strlen(value) >= DEFAULT_STRING_LENGTH)
  {
    nxfatal << "Loop: PANIC! Value '" << value << "' of "
            << type << " option '" << name << "' exceeds length of "
            << DEFAULT_STRING_LENGTH << " characters.\n"
            << std::flush;

    cerr << "Error" << ": Value '" << value << "' of "
         << type << " option '" << name << "' exceeds length of "
         << DEFAULT_STRING_LENGTH << " characters.\n";

    return -1;
  }

  return 1;
}

int ParseArg(const char *type, const char *name, const char *value)
{
  if (strcasecmp(value, "0") == 0)
  {
    return 0;
  }

  //
  // Find the base factor.
  //

  double base;

  const char *id = value + strlen(value) - 1;

  if (strcasecmp(id, "g") == 0)
  {
    base = 1024 * 1024 * 1024;
  }
  else if (strcasecmp(id, "m") == 0)
  {
    base = 1024 * 1024;
  }
  else if (strcasecmp(id, "k") == 0)
  {
    base = 1024;
  }
  else if (strcasecmp(id, "b") == 0 || isdigit(*id) == 1)
  {
    base = 1;
  }
  else
  {
    return -1;
  }

  char *string = new char[strlen(value)];

  // copy value but cut off the last character
  snprintf(string, strlen(value), "%s", value);

  nxinfo << "Loop: Parsing integer option '" << name
         << "' from string '" << string << "' with base set to ";

  switch (tolower(*id))
  {
    case 'k':
    case 'm':
    case 'g':
    {
      nxinfo_append << (char) toupper(*id);
      break;
    }
  }

  nxinfo_append << ".\n" << std::flush;

  double result = atof(string) * base;

  delete [] string;

  if (result < 0 || result > (((unsigned) -1) >> 1))
  {
    return -1;
  }

  nxinfo << "Loop: Integer option parsed to '"
         << (int) result << "'.\n" << std::flush;

  return (int) result;
}

void SetAndValidateChannelEndPointArg(const char *type, const char *name, const char *value,
                                      ChannelEndPoint &endPoint) {
  endPoint.setSpec(value);
  if (!endPoint.validateSpec()) {
    nxfatal << "Loop: PANIC! Invalid " << type
            << " option '" << name << "' with value '"
            << value << "'.\n" << std::flush;

    cerr << "Error" << ": Invalid " << type
         << " option '" << name << "' with value '"
         << value << "'.\n";

    HandleCleanup();
  }
}


int ValidateArg(const char *type, const char *name, const char *value)
{
  int number = atoi(value);

  if (number < 0)
  {
    nxfatal << "Loop: PANIC! Invalid " << type
            << " option '" << name << "' with value '"
            << value << "'.\n" << std::flush;

    cerr << "Error" << ": Invalid " << type
         << " option '" << name << "' with value '"
         << value << "'.\n";

    HandleCleanup();
  }

  return number;
}

int LowercaseArg(const char *type, const char *name, char *value)
{
  char *next = value;

  while (*next != '\0')
  {
    *next = tolower(*next);

    next++;
  }

  return 1;
}

int CheckSignal(int signal)
{
  //
  // Return 1 if the signal needs to be handled
  // by the proxy, 2 if the signal just needs to
  // be blocked to avoid interrupting a system
  // call.
  //

  switch (signal)
  {
    case SIGCHLD:
    case SIGUSR1:
    case SIGUSR2:
    case SIGHUP:
    case SIGINT:
    case SIGTERM:
    case SIGPIPE:
    case SIGALRM:
    {
      return 1;
    }
    case SIGVTALRM:
    case SIGWINCH:
    case SIGIO:
    case SIGTSTP:
    case SIGTTIN:
    case SIGTTOU:
    {
      return 2;
    }
    default:
    {
      #if defined(__CYGWIN__) || defined(__CYGWIN32__)

      //
      // This signal can be raised by the Cygwin
      // library.
      //

      if (signal == 12)
      {
        return 1;
      }

      #endif

      return 0;
    }
  }
}

static void PrintUsageInfo(const char *option, int error)
{
  if (error == 1)
  {
    cerr << "Error" << ": Invalid command line option '" << option << "'.\n";
  }

  cerr << GetUsageInfo();

  if (error == 1)
  {
    cerr << "Error" << ": NX transport initialization failed.\n";
  }
}

static void handleCheckSessionInLoop()
{
  //
  // Check if we completed the shutdown procedure
  // and the remote confirmed the shutdown. The
  // tear down should be always initiated by the
  // agent, but the X server side may unilateral-
  // ly shut down the link without our permission.
  //

  if (proxy -> getShutdown() > 0)
  {
    nxinfo << "Loop: End of NX transport requested "
           << "by remote.\n" << std::flush;

    handleTerminatingInLoop();

    if (control -> ProxyMode == proxy_server)
    {
      nxinfo << "Loop: Bytes received so far are "
             << (unsigned long long) statistics -> getBytesIn()
             << ".\n" << std::flush;

      if (statistics -> getBytesIn() < 1024)
      {
        cerr << "Info" << ": Your session was closed before reaching "
             << "a usable state.\n";
        cerr << "Info" << ": This can be due to the local X server "
             << "refusing access to the client.\n";
        cerr << "Info" << ": Please check authorization provided "
             << "by the remote X application.\n";
      }
    }

    nxinfo << "Loop: Shutting down the NX transport.\n"
           << std::flush;

    HandleCleanup();
  }
  else if (proxy -> handlePing() < 0)
  {
    nxinfo << "Loop: Failure handling the ping for "
           << "proxy FD#" << proxyFD << ".\n"
           << std::flush;

    HandleShutdown();
  }

  //
  // Check if the watchdog has exited and we didn't
  // get the SIGCHLD. This can happen if the parent
  // has overridden our signal handlers.
  //

  if (IsRunning(lastWatchdog) && CheckProcess(lastWatchdog, "watchdog") == 0)
  {
    nxwarn << "Loop: WARNING! Watchdog is gone unnoticed. "
           << "Setting the last signal to SIGTERM.\n"
           << std::flush;

    lastSignal = SIGTERM;

    nxwarn << "Loop: WARNING! Resetting pid of last "
           << "watchdog process.\n" << std::flush;

    SetNotRunning(lastWatchdog);
  }

  //
  // Let the client proxy find out if the agent's
  // channel is gone. This is the normal shutdown
  // procedure in the case of an internal connect-
  // ion to the agent.
  //

  int cleanup = 0;

  if (control -> ProxyMode == proxy_client &&
          agent != NULL && proxy -> getType(agentFD[1]) ==
              channel_none && lastKill == 0 && lastDestroy == 1)
  {
    nxinfo << "Loop: End of NX transport requested "
           << "by agent.\n" << std::flush;

    nxinfo << "Loop: Bytes sent so far are "
           << (unsigned long long) statistics -> getBytesOut()
           << ".\n" << std::flush;

    if (statistics -> getBytesOut() < 1024)
    {
      cerr << "Info" << ": Your session has died before reaching "
           << "an usable state.\n";
      cerr << "Info" << ": This can be due to the remote X server "
           << "refusing access to the client.\n";
      cerr << "Info" << ": Please check the authorization provided "
           << "by your X application.\n";
    }

    cleanup = 1;
  }

  //
  // Check if the user requested the end of the
  // session by sending a signal to the proxy.
  // All signals are handled in the main loop
  // so we need to reset the value to get ready
  // for the next iteration.
  //

  int signal = 0;

  if (lastSignal != 0)
  {
    switch (lastSignal)
    {
      case SIGCHLD:
      case SIGUSR1:
      case SIGUSR2:
      {
        break;
      }
      default:
      {
        signal = lastSignal;

        cleanup = 1;

        break;
      }
    }

    lastSignal = 0;
  }

  if (cleanup == 1)
  {
    //
    // The first time termination signal is received
    // disable all further connections, close down any
    // X channel and wait for a second signal.
    //

    if (lastKill == 0)
    {
      //
      // Don't print a message if cleanup is
      // due to normal termination of agent.
      //

      if (signal != 0)
      {
        nxinfo << "Loop: End of NX transport requested by signal '"
               << signal << "' '" << DumpSignal(signal)
               << "'.\n" << std::flush;

        handleTerminatingInLoop();
      }

      //
      // Disable any further connection.
      //

      CleanupListeners();

      //
      // Close all the remaining X channels and
      // let proxies save their persistent cache
      // on disk.
      //

      CleanupConnections();

      //
      // We'll need to wait for the X channels
      // to be shut down before waiting for the
      // cleanup signal.
      //

      lastKill = 1;
    }
    else if (lastKill == 2)
    {
      nxinfo << "Loop: Shutting down the NX transport.\n"
             << std::flush;

      proxy -> handleShutdown();

      HandleCleanup();
    }
  }

  if (lastKill == 1 && proxy -> getChannels(channel_x11) == 0)
  {
    //
    // Save the message stores to the
    // persistent cache.
    //

    proxy -> handleSave();

    //
    // Run a watchdog process so we can finally
    // give up at the time the watchdog exits.
    //

    if (IsNotRunning(lastWatchdog))
    {
      int timeout = control -> CleanupTimeout;

      if (timeout > 0)
      {
        if (proxy -> getChannels() == 0)
        {
          timeout = 500;
        }

        nxinfo << "Loop: Starting watchdog process with timeout "
               << "of " << timeout << " ms.\n"
               << std::flush;
      }
      else
      {
        nxinfo << "Loop: Starting watchdog process without "
               << "a timeout.\n" << std::flush;
      }

      lastWatchdog = NXTransWatchdog(timeout);

      if (IsFailed(lastWatchdog))
      {
        nxfatal << "Loop: PANIC! Can't start the NX watchdog "
                << "process in shutdown.\n" << std::flush;

        cerr << "Error" << ": Can't start the NX watchdog "
             << "process in shutdown.\n";

        HandleCleanup();
      }
      else
      {
        nxinfo << "Loop: Watchdog started with pid '"
               << lastWatchdog << "'.\n" << std::flush;
      }
    }
    else
    {
      nxfatal << "Loop: PANIC! Previous watchdog detected "
              << "in shutdown with pid '" << lastWatchdog
              << "'.\n" << std::flush;

      cerr << "Error" << ": Previous watchdog detected "
           << "in shutdown with pid '" << lastWatchdog
           << "'.\n";

      HandleCleanup();
    }

    if (control -> CleanupTimeout > 0)
    {
      nxinfo << "Loop: Waiting the cleanup timeout to complete.\n"
             << std::flush;

      cerr << "Info" << ": Waiting the cleanup timeout to complete.\n";
    }
    else
    {
      //
      // The NX server will kill the watchdog
      // process after having shut down the
      // service channels.
      //

      cerr << "Info" << ": Watchdog running with pid '" << lastWatchdog
           << "'.\n";

      nxinfo << "Loop: Waiting the watchdog process to complete.\n"
             << std::flush;

      cerr << "Info" << ": Waiting the watchdog process to complete.\n";
    }

    lastKill = 2;
  }
}

static void handleCheckBitrateInLoop()
{
  static long int slept = 0;

  nxinfo << "Loop: Bitrate is " << statistics -> getBitrateInShortFrame()
         << " B/s and " << statistics -> getBitrateInLongFrame()
         << " B/s in " << control -> ShortBitrateTimeFrame / 1000
         << "/" << control -> LongBitrateTimeFrame / 1000
         << " seconds timeframes.\n" << std::flush;

  //
  // This can be improved. We may not jump out
  // of the select often enough to guarantee
  // the necessary accuracy.
  //

  if (control -> LocalBitrateLimit > 0)
  {
    nxinfo << "Loop: Calculating bandwidth usage with limit "
           << control -> LocalBitrateLimit << ".\n"
           << std::flush;

    int reference = (statistics -> getBitrateInLongFrame() +
                         statistics -> getBitrateInShortFrame()) / 2;

    if (reference > control -> LocalBitrateLimit)
    {
      double ratio = ((double) reference) /
                         ((double) control -> LocalBitrateLimit);

      if (ratio > 1.2)
      {
        ratio = 1.2;
      }

      slept += (unsigned int) (pow(50000, ratio) / 1000);

      if (slept > 2000)
      {
        nxwarn << "Loop: WARNING! Sleeping due to "
               << "reference bitrate of " << reference
               << " B/s.\n" << std::flush;

        cerr << "Warning" << ": Sleeping due to "
             << "reference bitrate of " << reference
             << " B/s.\n";

        slept %= 2000;
      }

      T_timestamp idleTs = getNewTimestamp();

      usleep((unsigned int) pow(50000, ratio));

      int diffTs = diffTimestamp(idleTs, getNewTimestamp());

      statistics -> addIdleTime(diffTs);

      statistics -> subReadTime(diffTs);
    }
  }
}


static void handleCheckStateInLoop(int &setFDs)
{
  int fdLength;
  int fdPending;
  int fdSplits;

  for (int j = 0; j < setFDs; j++)
  {
    if (j != proxyFD)
    {
      fdPending = proxy -> getPending(j);
      if (fdPending > 0)
      {
        nxfatal << "Loop: PANIC! Buffer for descriptor FD#"
                << j << " has pending bytes to read.\n"
                << std::flush;

        HandleCleanup();
      }

      fdLength = proxy -> getLength(j);

      if (fdLength > 0)
      {
        nxinfo << "Loop: WARNING! Buffer for descriptor FD#"
               << j << " has " << fdLength << " bytes to write.\n"
               << std::flush;
      }
    }
  }

  fdPending = proxy -> getPending(proxyFD);

  if (fdPending > 0)
  {
    nxfatal << "Loop: PANIC! Buffer for proxy descriptor FD#"
            << proxyFD << " has pending bytes to read.\n"
            << std::flush;

    HandleCleanup();
  }

  fdLength = proxy -> getFlushable(proxyFD);

  if (fdLength > 0)
  {
    if (control -> FlushPolicy == policy_immediate &&
            proxy -> getBlocked(proxyFD) == 0)
    {
      nxfatal << "Loop: PANIC! Buffer for proxy descriptor FD#"
              << proxyFD << " has " << fdLength << " bytes "
              << "to write with policy 'immediate'.\n"
              << std::flush;

      HandleCleanup();
    }
    else
    {
      nxinfo << "Loop: WARNING! Buffer for proxy descriptor FD#"
             << proxyFD << " has " << fdLength << " bytes "
             << "to write.\n" << std::flush;
    }
  }

  fdSplits = proxy -> getSplitSize();

  if (fdSplits > 0)
  {
    nxwarn << "Loop: WARNING! Proxy descriptor FD#" << proxyFD
           << " has " << fdSplits << " splits to send.\n"
           << std::flush;
  }
}

static void handleCheckSelectInLoop(int &setFDs, fd_set &readSet,
                                        fd_set &writeSet, T_timestamp selectTs)
{
  nxinfo << "Loop: Maximum descriptors is ["
         << setFDs << "] at " << strMsTimestamp()
         << ".\n" << std::flush;

  int i;

  if (setFDs > 0)
  {
    i = 0;

    nxinfo << "Loop: Selected for read are";

    for (int j = 0; j < setFDs; j++)
    {
      if (FD_ISSET(j, &readSet))
      {
        nxinfo_append << " [" << j << "]";

        i++;
      }
    }

    if (i > 0)
    {
      nxinfo_append << ".\n" << std::flush;
    }
    else
    {
      nxinfo_append << " [none].\n" << std::flush;
    }

    i = 0;

    nxinfo << "Loop: Selected for write are";

    for (int j = 0; j < setFDs; j++)
    {
      if (FD_ISSET(j, &writeSet))
      {
        nxinfo_append << " [" << j << "]";

        i++;
      }
    }

    if (i > 0)
    {
      nxinfo_append << ".\n" << std::flush;
    }
    else
    {
      nxinfo_append << " [none].\n" << std::flush;
    }
  }

  nxinfo << "Loop: Select timeout is "
         << selectTs.tv_sec << " s and "
         << (double) selectTs.tv_usec / 1000
         << " ms.\n" << std::flush;
}

static void handleCheckResultInLoop(int &resultFDs, int &errorFDs, int &setFDs, fd_set &readSet,
                                        fd_set &writeSet, struct timeval &selectTs,
                                            struct timeval &pstartTs)
{
  int diffTs = diffTimestamp(pstartTs, getNewTimestamp());


  if (diffTs >= (control -> PingTimeout -
                     (control -> LatencyTimeout * 4)))
  {
    nxinfo << "Loop: Select result is [" << resultFDs
           << "] at " << strMsTimestamp() << " with no "
           << "communication within " << diffTs
           << " ms.\n" << std::flush;
  }
  else
  {
    nxinfo << "Loop: Select result is [" << resultFDs
           << "] error is [" << errorFDs << "] at "
           << strMsTimestamp() << " after " << diffTs
           << " ms.\n" << std::flush;
  }


  int i;

  if (resultFDs > 0)
  {
    i = 0;

    nxinfo << "Loop: Selected for read are";

    for (int j = 0; j < setFDs; j++)
    {
      if (FD_ISSET(j, &readSet))
      {
        nxinfo_append << " [" << j << "]";

        i++;
      }
    }

    if (i > 0)
    {
      nxinfo_append << ".\n" << std::flush;
    }
    else
    {
      nxinfo_append << " [none].\n" << std::flush;
    }

    i = 0;

    nxinfo << "Loop: Selected for write are";

    for (int j = 0; j < setFDs; j++)
    {
      if (FD_ISSET(j, &writeSet))
      {
        nxinfo_append << " [" << j << "]";

        i++;
      }
    }

    if (i > 0)
    {
      nxinfo_append << ".\n" << std::flush;
    }
    else
    {
      nxinfo_append << " [none].\n" << std::flush;
    }
  }
}


static void handleCheckSessionInConnect()
{
  nxinfo << "Loop: Going to check session in connect.\n"
         << std::flush;

  if (control -> ProxyMode == proxy_client)
  {
    HandleAlert(FAILED_PROXY_CONNECTION_CLIENT_ALERT, 1);
  }
  else if (IsNotRunning(lastDialog))
  {
    HandleAlert(FAILED_PROXY_CONNECTION_SERVER_ALERT, 1);
  }

  handleAlertInLoop();
}

static void handleStatisticsInLoop()
{
  if (lastSignal == 0)
  {
    return;
  }

  int mode = NO_STATS;

  if (control -> EnableStatistics == 1)
  {
    if (lastSignal == SIGUSR1)
    {
      //
      // Print overall statistics.
      //

      mode = TOTAL_STATS;
    }
    else if (lastSignal == SIGUSR2)
    {
      //
      // Print partial statistics.
      //

      mode = PARTIAL_STATS;
    }

    if (mode == TOTAL_STATS || mode == PARTIAL_STATS)
    {
      nxinfo << "Loop: Going to request proxy statistics "
             << "with signal '" << DumpSignal(lastSignal)
             << "'.\n" << std::flush;

      if (proxy != NULL)
      {
        if (ReopenLogFile(statsFileName, statofs, 0) < 0)
        {
          HandleCleanup();
        }

        proxy -> handleStatistics(mode, statofs);
      }
    }
  }
}

static void handleNegotiationInLoop(int &setFDs, fd_set &readSet,
                                        fd_set &writeSet, T_timestamp &selectTs)
{
  int yield = 0;

  while (yield == 0)
  {
    nxinfo << "Loop: Going to run a new negotiation loop "
           << "with stage " << control -> ProxyStage
           << " at " << strMsTimestamp() << ".\n"
           << std::flush;

    switch (control -> ProxyStage)
    {
      case stage_undefined:
      {
        nxinfo << "Loop: Handling negotiation with '"
               << "stage_undefined" << "'.\n"
               << std::flush;

        control -> ProxyStage = stage_initializing;

        break;
      }
      case stage_initializing:
      {
        nxinfo << "Loop: Handling negotiation with '"
               << "stage_initializing" << "'.\n"
               << std::flush;

        InitBeforeNegotiation();

        control -> ProxyStage = stage_connecting;

        break;
      }
      case stage_connecting:
      {
        nxinfo << "Loop: Handling negotiation with '"
               << "stage_connecting" << "'.\n"
               << std::flush;

        SetupProxyConnection();

        control -> ProxyStage = stage_connected;

        break;
      }
      case stage_connected:
      {
        nxinfo << "Loop: Handling negotiation with '"
               << "stage_connected" << "'.\n"
               << std::flush;

        //
        // Server side proxy must always be the one that
        // sends its version and options first, so, in
        // some way, client side can be the the one that
        // has the last word on the matter.
        //

        if (control -> ProxyMode == proxy_server)
        {
          //
          // Check if we have been listening for a
          // forwarder. In this case it will have to
          // authenticate itself.
          //

          if (WE_LISTEN_FORWARDER)
          {
            control -> ProxyStage = stage_waiting_forwarder_version;

            break;
          }

          control -> ProxyStage = stage_sending_proxy_options;
        }
        else
        {
          //
          // The X client side is the side that has to wait
          // for the authorization cookie and any remote
          // option.
          //

          control -> ProxyStage = stage_waiting_proxy_version;
        }

        break;
      }
      case stage_sending_proxy_options:
      {
        nxinfo << "Loop: Handling negotiation with '"
               << "stage_sending_proxy_options" << "'.\n"
               << std::flush;

        if (SendProxyOptions(proxyFD) < 0)
        {
          goto handleNegotiationInLoopError;
        }

        if (control -> ProxyMode == proxy_server)
        {
          control -> ProxyStage = stage_waiting_proxy_version;
        }
        else
        {
          control -> ProxyStage = stage_sending_proxy_caches;
        }

        break;
      }
      case stage_waiting_forwarder_version:
      {
        nxinfo << "Loop: Handling negotiation with '"
               << "stage_waiting_forwarder_version" << "'.\n"
               << std::flush;

        int result = ReadForwarderVersion(proxyFD);

        if (result == 0)
        {
          yield = 1;
        }
        else if (result == 1)
        {
          control -> ProxyStage = stage_waiting_forwarder_options;
        }
        else
        {
          goto handleNegotiationInLoopError;
        }

        break;
      }
      case stage_waiting_forwarder_options:
      {
        nxinfo << "Loop: Handling negotiation with '"
               << "stage_waiting_forwarder_options" << "'.\n"
               << std::flush;

        int result = ReadForwarderOptions(proxyFD);

        if (result == 0)
        {
          yield = 1;
        }
        else if (result == 1)
        {
          control -> ProxyStage = stage_sending_proxy_options;
        }
        else
        {
          goto handleNegotiationInLoopError;
        }

        break;
      }
      case stage_waiting_proxy_version:
      {
        nxinfo << "Loop: Handling negotiation with '"
               << "stage_waiting_proxy_version" << "'.\n"
               << std::flush;

        int result = ReadProxyVersion(proxyFD);

        if (result == 0)
        {
          yield = 1;
        }
        else if (result == 1)
        {
          control -> ProxyStage = stage_waiting_proxy_options;
        }
        else
        {
          goto handleNegotiationInLoopError;
        }

        break;
      }
      case stage_waiting_proxy_options:
      {
        nxinfo << "Loop: Handling negotiation with '"
               << "stage_waiting_proxy_options" << "'.\n"
               << std::flush;

        int result = ReadProxyOptions(proxyFD);

        if (result == 0)
        {
          yield = 1;
        }
        else if (result == 1)
        {
          if (control -> ProxyMode == proxy_server)
          {
            control -> ProxyStage = stage_waiting_proxy_caches;
          }
          else
          {
            control -> ProxyStage = stage_sending_proxy_options;
          }
        }
        else
        {
          goto handleNegotiationInLoopError;
        }

        break;
      }
      case stage_sending_proxy_caches:
      {
        nxinfo << "Loop: Handling negotiation with '"
               << "stage_sending_proxy_caches" << "'.\n"
               << std::flush;

        if (SendProxyCaches(proxyFD) < 0)
        {
          goto handleNegotiationInLoopError;
        }

        if (control -> ProxyMode == proxy_server)
        {
          control -> ProxyStage = stage_operational;
        }
        else
        {
          control -> ProxyStage = stage_waiting_proxy_caches;
        }

        break;
      }
      case stage_waiting_proxy_caches:
      {
        nxinfo << "Loop: Handling negotiation with '"
               << "stage_waiting_proxy_caches" << "'.\n"
               << std::flush;

        int result = ReadProxyCaches(proxyFD);

        if (result == 0)
        {
          yield = 1;
        }
        else if (result == 1)
        {
          if (control -> ProxyMode == proxy_server)
          {
            control -> ProxyStage = stage_sending_proxy_caches;
          }
          else
          {
            control -> ProxyStage = stage_operational;
          }
        }
        else
        {
          goto handleNegotiationInLoopError;
        }

        break;
      }
      case stage_operational:
      {
        nxinfo << "Loop: Handling negotiation with '"
               << "stage_operational" << "'.\n"
               << std::flush;

        InitAfterNegotiation();

        yield = 1;

        break;
      }
      default:
      {
        nxfatal << "Loop: PANIC! Unmanaged case '" << control -> ProxyStage
                << "' while handling negotiation.\n" << std::flush;

        cerr << "Error" << ": Unmanaged case '" << control -> ProxyStage
             << "' while handling negotiation.\n";

        HandleCleanup();
      }
    }
  }

  //
  // Check if the user requested the end of
  // the session.
  //

  if (CheckAbort() != 0)
  {
    HandleCleanup();
  }

  //
  // Select the proxy descriptor so that we
  // can proceed negotiating the session.
  //

  FD_SET(proxyFD, &readSet);

  if (proxyFD >= setFDs)
  {
    setFDs = proxyFD + 1;
  }

  setMinTimestamp(selectTs, control -> PingTimeout);

  nxinfo << "Loop: Selected proxy FD#" << proxyFD << " in negotiation "
         << "phase with timeout of " << selectTs.tv_sec << " s and "
         << selectTs.tv_usec << " ms.\n" << std::flush;

  return;

handleNegotiationInLoopError:

  nxfatal << "Loop: PANIC! Failure negotiating the session in stage '"
          << control -> ProxyStage << "'.\n" << std::flush;

  cerr << "Error" << ": Failure negotiating the session in stage '"
       << control -> ProxyStage << "'.\n";


  if (control -> ProxyMode == proxy_server &&
          control -> ProxyStage == stage_waiting_proxy_version)
  {
    nxfatal << "Loop: PANIC! Wrong version or invalid session "
            << "authentication cookie.\n" << std::flush;

    cerr << "Error" << ": Wrong version or invalid session "
         << "authentication cookie.\n";
  }

  handleTerminatingInLoop();

  HandleCleanup();
}

static void handleTerminatingInLoop()
{
  if (getpid() == lastProxy)
  {
    if (control -> ProxyStage < stage_terminating)
    {
      if (agent == NULL)
      {
        cerr << "Session" << ": Terminating session at '"
             << strTimestamp() << "'.\n";
      }

      control -> ProxyStage = stage_terminating;
    }
  }
}

static void handleTerminatedInLoop()
{
  if (getpid() == lastProxy)
  {
    if (control -> ProxyStage < stage_terminated)
    {
      if (agent == NULL)
      {
        cerr << "Session" << ": Session terminated at '"
             << strTimestamp() << "'.\n";
      }

      control -> ProxyStage = stage_terminated;
    }
  }
}

static void handleAlertInLoop()
{
  if (lastAlert.code == 0)
  {
    return;
  }

  //
  // Since ProtoStep7 (#issue 108)
  //
  // Now the remote proxy should always
  // be able to handle the alert
  //

  if (lastAlert.local == 0)
  {
    if (proxy != NULL)
    {
      nxinfo << "Loop: Requesting a remote alert with code '"
             << lastAlert.code << "'.\n" << std::flush;

      if (proxy -> handleAlert(lastAlert.code) < 0)
      {
        HandleShutdown();
      }
    }
  }
  else
  {
    nxinfo << "Loop: Handling a local alert with code '"
           << lastAlert.code << "'.\n" << std::flush;

    if (control -> ProxyMode == proxy_client)
    {
      //
      // If we are at X client side and server
      // proxy is not responding, we don't have
      // any possibility to interact with user.
      //

      if (lastAlert.code != CLOSE_DEAD_PROXY_CONNECTION_CLIENT_ALERT &&
              lastAlert.code != RESTART_DEAD_PROXY_CONNECTION_CLIENT_ALERT &&
                  lastAlert.code != FAILED_PROXY_CONNECTION_CLIENT_ALERT)
      {
        //
        // Let the server proxy show the dialog.
        //

        if (proxy != NULL &&
                proxy -> handleAlert(lastAlert.code) < 0)
        {
          HandleShutdown();
        }
      }
    }
    else
    {
      char caption[DEFAULT_STRING_LENGTH];

      strcpy(caption, ALERT_CAPTION_PREFIX);

      int length = strlen(sessionId);

      //
      // Get rid of the trailing MD5 from session id.
      //

      if (length > (MD5_LENGTH * 2 + 1) &&
              *(sessionId + (length - (MD5_LENGTH * 2 + 1))) == '-')
      {
        strncat(caption, sessionId, length - (MD5_LENGTH * 2 + 1));
      }
      else
      {
        strcat(caption, sessionId);
      }

      //
      // Use the display to which we are forwarding
      // the remote X connections.
      //

      char *display = displayHost;

      int replace = 1;
      int local   = 1;

      const char *message;
      const char *type;

      switch (lastAlert.code)
      {
        case CLOSE_DEAD_X_CONNECTION_CLIENT_ALERT:
        {
          message = CLOSE_DEAD_X_CONNECTION_CLIENT_ALERT_STRING;
          type    = CLOSE_DEAD_X_CONNECTION_CLIENT_ALERT_TYPE;

          break;
        }
        case CLOSE_DEAD_X_CONNECTION_SERVER_ALERT:
        {
          message = CLOSE_DEAD_X_CONNECTION_SERVER_ALERT_STRING;
          type    = CLOSE_DEAD_X_CONNECTION_SERVER_ALERT_TYPE;

          break;
        }
        case CLOSE_DEAD_PROXY_CONNECTION_SERVER_ALERT:
        {
          message = CLOSE_DEAD_PROXY_CONNECTION_SERVER_ALERT_STRING;
          type    = CLOSE_DEAD_PROXY_CONNECTION_SERVER_ALERT_TYPE;

          break;
        }
        case RESTART_DEAD_PROXY_CONNECTION_SERVER_ALERT:
        {
          message = RESTART_DEAD_PROXY_CONNECTION_SERVER_ALERT_STRING;
          type    = RESTART_DEAD_PROXY_CONNECTION_SERVER_ALERT_TYPE;

          break;
        }
        case CLOSE_UNRESPONSIVE_X_SERVER_ALERT:
        {
          message = CLOSE_UNRESPONSIVE_X_SERVER_ALERT_STRING;
          type    = CLOSE_UNRESPONSIVE_X_SERVER_ALERT_TYPE;

          break;
        }
        case WRONG_PROXY_VERSION_ALERT:
        {
          message = WRONG_PROXY_VERSION_ALERT_STRING;
          type    = WRONG_PROXY_VERSION_ALERT_TYPE;

          break;
        }
        case FAILED_PROXY_CONNECTION_SERVER_ALERT:
        {
          message = FAILED_PROXY_CONNECTION_SERVER_ALERT_STRING;
          type    = FAILED_PROXY_CONNECTION_SERVER_ALERT_TYPE;

          break;
        }
        case MISSING_PROXY_CACHE_ALERT:
        {
          message = MISSING_PROXY_CACHE_ALERT_STRING;
          type    = MISSING_PROXY_CACHE_ALERT_TYPE;

          break;
        }
        case ABORT_PROXY_CONNECTION_ALERT:
        {
          message = ABORT_PROXY_CONNECTION_ALERT_STRING;
          type    = ABORT_PROXY_CONNECTION_ALERT_TYPE;

          break;
        }
        case DISPLACE_MESSAGE_ALERT:
        {
          message = DISPLACE_MESSAGE_ALERT_STRING;
          type    = DISPLACE_MESSAGE_ALERT_TYPE;

          break;
        }
        case GREETING_MESSAGE_ALERT:
        {
          message = GREETING_MESSAGE_ALERT_STRING;
          type    = GREETING_MESSAGE_ALERT_TYPE;

          break;
        }
        case START_RESUME_SESSION_ALERT:
        {
          message = START_RESUME_SESSION_ALERT_STRING;
          type    = START_RESUME_SESSION_ALERT_TYPE;

          break;
        }
        case FAILED_RESUME_DISPLAY_ALERT:
        {
          message = FAILED_RESUME_DISPLAY_ALERT_STRING;
          type    = FAILED_RESUME_DISPLAY_ALERT_TYPE;

          break;
        }
        case FAILED_RESUME_DISPLAY_BROKEN_ALERT:
        {
          message = FAILED_RESUME_DISPLAY_BROKEN_STRING;
          type    = FAILED_RESUME_DISPLAY_BROKEN_TYPE;

          break;
        }
        case FAILED_RESUME_VISUALS_ALERT:
        {
          message = FAILED_RESUME_VISUALS_ALERT_STRING;
          type    = FAILED_RESUME_VISUALS_ALERT_TYPE;

          break;
        }
        case FAILED_RESUME_COLORMAPS_ALERT:
        {
          message = FAILED_RESUME_COLORMAPS_ALERT_STRING;
          type    = FAILED_RESUME_COLORMAPS_ALERT_TYPE;

          break;
        }
        case FAILED_RESUME_PIXMAPS_ALERT:
        {
          message = FAILED_RESUME_PIXMAPS_ALERT_STRING;
          type    = FAILED_RESUME_PIXMAPS_ALERT_TYPE;

          break;
        }
        case FAILED_RESUME_DEPTHS_ALERT:
        {
          message = FAILED_RESUME_DEPTHS_ALERT_STRING;
          type    = FAILED_RESUME_DEPTHS_ALERT_TYPE;

          break;
        }
        case FAILED_RESUME_RENDER_ALERT:
        {
          message = FAILED_RESUME_RENDER_ALERT_STRING;
          type    = FAILED_RESUME_RENDER_ALERT_TYPE;

          break;
        }
        case FAILED_RESUME_FONTS_ALERT:
        {
          message = FAILED_RESUME_FONTS_ALERT_STRING;
          type    = FAILED_RESUME_FONTS_ALERT_TYPE;

          break;
        }
        case INTERNAL_ERROR_ALERT:
        {
          message = INTERNAL_ERROR_ALERT_STRING;
          type    = INTERNAL_ERROR_ALERT_TYPE;

          break;
        }
        case ABORT_PROXY_NEGOTIATION_ALERT:
        {
          message = ABORT_PROXY_NEGOTIATION_ALERT_STRING;
          type    = ABORT_PROXY_NEGOTIATION_ALERT_TYPE;

          break;
        }
        case ABORT_PROXY_SHUTDOWN_ALERT:
        {
          message = ABORT_PROXY_SHUTDOWN_ALERT_STRING;
          type    = ABORT_PROXY_SHUTDOWN_ALERT_TYPE;

          break;
        }
        case FAILED_XDMCP_CONNECTION_ALERT:
        {
          message = FAILED_XDMCP_CONNECTION_ALERT_STRING;
          type    = FAILED_XDMCP_CONNECTION_ALERT_TYPE;

          break;
        }
        default:
        {
          if (lastAlert.code > LAST_PROTO_STEP_7_ALERT)
          {
            nxwarn << "Loop: WARNING! An unrecognized alert type '"
                   << lastAlert.code << "' was requested.\n"
                   << std::flush;

            cerr << "Warning" << ": An unrecognized alert type '"
                 << lastAlert.code << "' was requested.\n";
          }
          else
          {
            nxwarn << "Loop: WARNING! Ignoring obsolete alert type '"
                   << lastAlert.code << "'.\n" << std::flush;
          }

          message = NULL;
          type    = NULL;

          replace = 0;

          break;
        }
      }

      if (replace == 1 && IsRunning(lastDialog))
      {
        nxinfo << "Loop: Killing the previous dialog with pid '"
               << lastDialog << "'.\n" << std::flush;

        //
        // The client ignores the TERM signal
        // on Windows.
        //

        #if defined(__CYGWIN__) || defined(__CYGWIN32__)

        KillProcess(lastDialog, "dialog", SIGKILL, 0);

        #else

        KillProcess(lastDialog, "dialog", SIGTERM, 0);

        #endif

        SetNotRunning(lastDialog);

        if (proxy != NULL)
        {
          proxy -> handleResetAlert();
        }
      }

      if (message != NULL && type != NULL)
      {
        lastDialog = NXTransDialog(caption, message, 0, type, local, display);

        if (IsFailed(lastDialog))
        {
          nxfatal << "Loop: PANIC! Can't start the NX dialog process.\n"
                  << std::flush;

          SetNotRunning(lastDialog);
        }
        else
        {
          nxinfo << "Loop: Dialog started with pid '"
                 << lastDialog << "'.\n" << std::flush;
        }
      }
      else
      {
        nxinfo << "Loop: No new dialog required for code '"
               << lastAlert.code << "'.\n" << std::flush;
      }
    }
  }

  //
  // Reset state.
  //

  lastAlert.code  = 0;
  lastAlert.local = 0;
}

static inline void handleSetAgentInLoop(int &setFDs, fd_set &readSet,
                                            fd_set &writeSet, struct timeval &selectTs)
{
  nxinfo << "Loop: Preparing the masks for the agent descriptors.\n"
         << std::flush;

  agent -> saveChannelState();

  agent -> saveReadMask(&readSet);
  agent -> saveWriteMask(&writeSet);

  if (control -> ProxyStage >= stage_operational)
  {
    if (agent -> remoteCanRead(&readSet) ||
            agent -> remoteCanWrite(&writeSet) ||
                agent -> localCanRead() ||
                    agent -> proxyCanRead())
    {
      nxinfo << "Loop: Setting a null timeout with agent descriptors ready.\n"
             << std::flush;

      //
      // Force a null timeout so we'll bail out
      // of the select immediately. We will ac-
      // comodate the result code later.
      //

      selectTs.tv_sec  = 0;
      selectTs.tv_usec = 0;
    }
  }

  nxinfo << "Loop: Clearing the read and write agent descriptors.\n"
         << std::flush;

  agent -> clearReadMask(&readSet);
  agent -> clearWriteMask(&writeSet);
}

static inline void handleAgentInLoop(int &resultFDs, int &errorFDs, int &setFDs, fd_set &readSet,
                                         fd_set &writeSet, struct timeval &selectTs)
{
  nxinfo << "Loop: Setting proxy and local agent descriptors.\n"
         << std::flush;

  //
  // Check if I/O is possible on the local
  // agent or the proxy descriptor.
  //

  if (resultFDs >= 0)
  {
    //
    // Save if the proxy can read from the
    // the agent descriptor.
    //

    agent -> saveChannelState();

    nxinfo << "Loop: Values were resultFDs " << resultFDs
           << " errorFDs " << errorFDs << " setFDs "
           << setFDs << ".\n" << std::flush;

    if (agent -> localCanRead() == 1)
    {
      nxinfo << "Loop: Setting agent descriptor FD#" << agent ->
                getLocalFd() << " as ready to read.\n"
             << std::flush;

      agent -> setLocalRead(&readSet, &resultFDs);
    }


    if (agent -> proxyCanRead(&readSet) == 0 &&
            agent -> proxyCanRead() == 1)
    {
      nxinfo << "Loop: WARNING! Can read from proxy FD#"
             << proxyFD << " but the descriptor "
             << "is not selected.\n" << std::flush;
    }

    if (agent -> proxyCanRead(&readSet) == 1)
    {
      nxinfo << "Loop: Setting proxy descriptor FD#" << agent ->
                getProxyFd() << " as ready to read.\n"
             << std::flush;
    }


    nxinfo << "Loop: Values are now resultFDs " << resultFDs
           << " errorFDs " << errorFDs << " setFDs "
           << setFDs << ".\n" << std::flush;
  }
}

static inline void handleAgentLateInLoop(int &resultFDs, int &errorFDs, int &setFDs, fd_set &readSet,
                                             fd_set &writeSet, struct timeval &selectTs)
{
  nxinfo << "Loop: Setting remote agent descriptors.\n"
         << std::flush;

  //
  // We reset the masks before calling our select.
  // We now set the descriptors that are ready but
  // only if they were set in the original mask.
  // We do this after having executed our loop as
  // we may have produced more data and the agent
  // descriptors may have become readable or writ-
  // able in the meanwhile.
  //

  if (resultFDs >= 0)
  {
    //
    // Save if the proxy can read from the
    // the agent descriptor.
    //

    agent -> saveChannelState();

    nxinfo << "Loop: Values were resultFDs " << resultFDs
           << " errorFDs " << errorFDs << " setFDs "
           << setFDs << ".\n" << std::flush;

    if (agent -> remoteCanRead(agent ->
            getSavedReadMask()) == 1)
    {
      nxinfo << "Loop: Setting agent descriptor FD#" << agent ->
                getRemoteFd() << " as ready to read.\n"
             << std::flush;

      agent -> setRemoteRead(&readSet, &resultFDs);
    }

    if (agent -> remoteCanWrite(agent ->
            getSavedWriteMask()) == 1)
    {
      nxinfo << "Loop: Setting agent descriptor FD#" << agent ->
                getRemoteFd() << " as ready to write.\n"
             << std::flush;

      agent -> setRemoteWrite(&writeSet, &resultFDs);
    }

    nxinfo << "Loop: Values are now resultFDs " << resultFDs
           << " errorFDs " << errorFDs << " setFDs "
           << setFDs << ".\n" << std::flush;
  }
}

static inline void handleReadableInLoop(int &resultFDs, fd_set &readSet)
{
  if (resultFDs > 0)
  {
    T_channel_type type = channel_none;

    const char *label = NULL;
    int domain  = -1;
    int fd      = -1;

    if (tcpFD != -1 && FD_ISSET(tcpFD, &readSet))
    {
      type   = channel_x11;
      label  = "X";
      domain = AF_INET;
      fd     = tcpFD;

      resultFDs--;
    }

    if (unixFD != -1 && FD_ISSET(unixFD, &readSet))
    {
      type   = channel_x11;
      label  = "X";
      domain = AF_UNIX;
      fd     = unixFD;

      resultFDs--;
    }

    if (cupsFD != -1 && FD_ISSET(cupsFD, &readSet))
    {
      type   = channel_cups;
      label  = "CUPS";
      domain = AF_INET;
      fd     = cupsFD;

      resultFDs--;
    }

    if (auxFD != -1 && FD_ISSET(auxFD, &readSet))
    {
      //
      // Starting from version 1.5.0 we create real X
      // connections for the keyboard channel, so they
      // can use the fake authorization cookie. This
      // means that there is not such a thing like a
      // channel_aux anymore.
      //

      type   = channel_x11;
      label  = "auxiliary X11";
      domain = AF_INET;
      fd     = auxFD;

      resultFDs--;
    }

    if (smbFD != -1 && FD_ISSET(smbFD, &readSet))
    {
      type   = channel_smb;
      label  = "SMB";
      domain = AF_INET;
      fd     = smbFD;

      resultFDs--;
    }

    if (mediaFD != -1 && FD_ISSET(mediaFD, &readSet))
    {
      type   = channel_media;
      label  = "media";
      domain = AF_INET;
      fd     = mediaFD;

      resultFDs--;
    }

    if (httpFD != -1 && FD_ISSET(httpFD, &readSet))
    {
      type   = channel_http;
      label  = "HTTP";
      domain = AF_INET;
      fd     = httpFD;

      resultFDs--;
    }

    if (fontFD != -1 && FD_ISSET(fontFD, &readSet))
    {
      type   = channel_font;
      label  = "font server";
      domain = AF_INET;
      fd     = fontFD;

      resultFDs--;
    }

    if (slaveFD != -1 && FD_ISSET(slaveFD, &readSet))
    {
      type   = channel_slave;
      label  = "slave";
      domain = AF_INET;
      fd     = slaveFD;

      resultFDs--;
    }

    if (type != channel_none)
    {
      int newFD = AcceptConnection(fd, domain, label);

      if (newFD != -1)
      {
        if (proxy -> handleNewConnection(type, newFD) < 0)
        {
          nxfatal << "Loop: PANIC! Error creating new " << label
                  << " connection.\n" << std::flush;

          cerr << "Error" << ": Error creating new " << label
               << " connection.\n";

          close(newFD);

          //
          // Don't kill the proxy in the case of an error.
          //
          // HandleCleanup();
          //
        }
        else if (proxy -> getReadable(newFD) > 0)
        {
          //
          // Add the descriptor, so we can try
          // to read immediately.
          //

          nxinfo << "Loop: Trying to read immediately "
                 << "from descriptor FD#" << newFD
                 << ".\n" << std::flush;

          FD_SET(newFD, &readSet);

          resultFDs++;
        }
        else
        {
          nxinfo << "Loop: Nothing to read immediately "
                 << "from descriptor FD#" << newFD
                 << ".\n" << std::flush;
        }
      }
    }
  }

  nxdbg << "Loop: Going to check the readable descriptors.\n"
        << std::flush;

  if (proxy -> handleRead(resultFDs, readSet) < 0)
  {
    nxinfo << "Loop: Failure reading from descriptors "
           << "for proxy FD#" << proxyFD << ".\n"
           << std::flush;

    HandleShutdown();
  }
}

static inline void handleWritableInLoop(int &resultFDs, fd_set &writeSet)
{
  nxdbg << "Loop: Going to check the writable descriptors.\n"
        << std::flush;

  if (resultFDs > 0 && proxy -> handleFlush(resultFDs, writeSet) < 0)
  {
    nxinfo << "Loop: Failure writing to descriptors "
           << "for proxy FD#" << proxyFD << ".\n"
           << std::flush;

    HandleShutdown();
  }
}

static inline void handleFlushInLoop()
{
  nxdbg << "Loop: Going to flush any data to the proxy.\n"
        << std::flush;

  if (agent == NULL || control ->
          FlushPolicy == policy_immediate)
  {

    if (usePolicy == -1 && control ->
            ProxyMode == proxy_client)
    {
      nxinfo << "Loop: WARNING! Flushing the proxy link "
             << "on behalf of the agent.\n" << std::flush;
    }


    if (proxy -> handleFlush() < 0)
    {
      nxinfo << "Loop: Failure flushing the proxy FD#"
             << proxyFD << ".\n" << std::flush;

      HandleShutdown();
    }
  }
}

static inline void handleRotateInLoop()
{
  nxdbg << "Loop: Going to rotate channels "
        << "for proxy FD#" << proxyFD << ".\n"
        << std::flush;

  proxy -> handleRotate();
}

static inline void handleEventsInLoop()
{
  nxdbg << "Loop: Going to check channel events "
        << "for proxy FD#" << proxyFD << ".\n"
        << std::flush;

  if (proxy -> handleEvents() < 0)
  {
    nxinfo << "Loop: Failure handling channel events "
           << "for proxy FD#" << proxyFD << ".\n"
           << std::flush;

    HandleShutdown();
  }
}

static void handleLogReopenInLoop(T_timestamp &lTs, T_timestamp &nTs)
{
  //
  // If need to limit the size of the
  // log file, check the size at each
  // loop.
  //

  #ifndef QUOTA

  if (diffTimestamp(lTs, nTs) > control -> FileSizeCheckTimeout)

  #endif
  {
    nxdbg << "Loop: Checking size of log file '"
          << errorsFileName << "'.\n" << std::flush;

    #ifndef MIXED

    if (ReopenLogFile(errorsFileName, logofs, control -> FileSizeLimit) < 0)
    {
      HandleShutdown();
    }

    #endif

    //
    // Reset to current timestamp.
    //

    lTs = nTs;
  }
}

static inline void handleSetReadInLoop(fd_set &readSet, int &setFDs, struct timeval &selectTs)
{
  proxy -> setReadDescriptors(&readSet, setFDs, selectTs);
}

static inline void handleSetWriteInLoop(fd_set &writeSet, int &setFDs, struct timeval &selectTs)
{
  proxy -> setWriteDescriptors(&writeSet, setFDs, selectTs);
}

static void handleSetListenersInLoop(fd_set &readSet, int &setFDs)
{
  //
  // Set descriptors of listening sockets.
  //

  if (control -> ProxyMode == proxy_client)
  {
    if (useTcpSocket == 1)
    {
      FD_SET(tcpFD, &readSet);

      if (tcpFD >= setFDs)
      {
        setFDs = tcpFD + 1;
      }

      nxdbg << "Loop: Selected listener tcpFD " << tcpFD
            << " with setFDs " << setFDs << ".\n"
            << std::flush;
    }

    if (useUnixSocket == 1)
    {
      FD_SET(unixFD, &readSet);

      if (unixFD >= setFDs)
      {
        setFDs = unixFD + 1;
      }

      nxdbg << "Loop: Selected listener unixFD " << unixFD
            << " with setFDs " << setFDs << ".\n"
            << std::flush;
    }

    if (useCupsSocket == 1)
    {
      FD_SET(cupsFD, &readSet);

      if (cupsFD >= setFDs)
      {
        setFDs = cupsFD + 1;
      }

      nxdbg << "Loop: Selected listener cupsFD " << cupsFD
            << " with setFDs " << setFDs << ".\n"
            << std::flush;
    }

    if (useAuxSocket == 1)
    {
      FD_SET(auxFD, &readSet);

      if (auxFD >= setFDs)
      {
        setFDs = auxFD + 1;
      }

      nxdbg << "Loop: Selected listener auxFD " << auxFD
            << " with setFDs " << setFDs << ".\n"
            << std::flush;
    }

    if (useSmbSocket == 1)
    {
      FD_SET(smbFD, &readSet);

      if (smbFD >= setFDs)
      {
        setFDs = smbFD + 1;
      }

      nxdbg << "Loop: Selected listener smbFD " << smbFD
            << " with setFDs " << setFDs << ".\n"
            << std::flush;
    }

    if (useMediaSocket == 1)
    {
      FD_SET(mediaFD, &readSet);

      if (mediaFD >= setFDs)
      {
        setFDs = mediaFD + 1;
      }

      nxdbg << "Loop: Selected listener mediaFD " << mediaFD
            << " with setFDs " << setFDs << ".\n"
            << std::flush;
    }

    if (useHttpSocket == 1)
    {
      FD_SET(httpFD, &readSet);

      if (httpFD >= setFDs)
      {
        setFDs = httpFD + 1;
      }

      nxdbg << "Loop: Selected listener httpFD " << httpFD
            << " with setFDs " << setFDs << ".\n"
            << std::flush;
    }
  }
  else
  {
    if (useFontSocket == 1)
    {
      FD_SET(fontFD, &readSet);

      if (fontFD >= setFDs)
      {
        setFDs = fontFD + 1;
      }

      nxdbg << "Loop: Selected listener fontFD " << fontFD
            << " with setFDs " << setFDs << ".\n"
            << std::flush;
    }
  }

  if (useSlaveSocket == 1)
  {
    FD_SET(slaveFD, &readSet);

    if (slaveFD >= setFDs)
    {
      setFDs = slaveFD + 1;
    }

    nxdbg << "Loop: Selected listener slaveFD " << slaveFD
          << " with setFDs " << setFDs << ".\n"
          << std::flush;
  }
}
