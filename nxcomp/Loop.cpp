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

#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "Misc.h"

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

#ifndef __CYGWIN32__
#include <netinet/tcp.h>
#include <sys/un.h>
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

//
// System specific defines.
//

#if defined(__EMX__) || defined(__CYGWIN32__)

struct sockaddr_un
{
  u_short sun_family;
  char sun_path[108];
};

#endif

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

#define WE_INITIATE_CONNECTION    (*connectHost != '\0')

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
                                           listenPort != -1)

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

int (*handler)(int) = NULL;

//
// Signal handling functions.
//

void DisableSignals();
void EnableSignals();
void InstallSignals();

static void RestoreSignals();
static void HandleSignal(int signal);

//
// Signal handling utilities.
//

static void InstallSignal(int signal, int action);
static void RestoreSignal(int signal);

static int HandleChildren();

static int HandleChild(int child);
static int CheckChild(int pid, int status);
static int WaitChild(int child, const char *label, int force);

int CheckParent(const char *name, const char *type, int parent);

void RegisterChild(int child);

static int CheckAbort();

//
// Timer handling utilities.
//

void SetTimer(int timeout);
void ResetTimer();

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

static int StartKeeper();

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

static int SetupTcpSocket();
static int SetupUnixSocket();
static int SetupServiceSockets();
static int SetupDisplaySocket(int &xServerAddrFamily, sockaddr *&xServerAddr,
                                  unsigned int &xServerAddrLength);

//
// Setup a listening socket and accept
// a new connection.
//

static int ListenConnection(int port, const char *label);
static int AcceptConnection(int fd, int domain, const char *label);

//
// Other convenience functions.
//

static int WaitForRemote(int portNum);
static int ConnectToRemote(const char *const hostName, int portNum);

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

static int ParseHostOption(const char *opt, char *host, int &port);

//
// Translate a font server port specification
// to the corresponding Unix socket path.
//

static int ParseFontPath(char *path);

//
// Determine the interface where to listen for
// the remote proxy connection or the local
// forwarder.
//

static int ParseListenOption(int &interface);

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

#if defined(TEST) || defined(INFO)
static void handleCheckSelectInLoop(int &setFDs, fd_set &readSet,
                                        fd_set &writeSet, T_timestamp selectTs);
static void handleCheckResultInLoop(int &resultFDs, int &errorFDs, int &setFDs, fd_set &readSet,
                                        fd_set &writeSet, struct timeval &selectTs,
                                            struct timeval &startTs);
static void handleCheckStateInLoop(int &setFDs);
#endif

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

static void handleLogReopenInLoop(T_timestamp &logsTs, T_timestamp &nowTs);

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
// the place where we print also debug informations.
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

static char connectHost[DEFAULT_STRING_LENGTH] = { 0 };
static char acceptHost[DEFAULT_STRING_LENGTH]  = { 0 };
static char listenHost[DEFAULT_STRING_LENGTH]  = { 0 };
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
// The port where the local proxy will await
// the peer connection or where the remote
// proxy will be contacted.
//

static int listenPort  = -1;
static int connectPort = -1;

//
// Helper channels are disabled by default.
//

static int cupsPort  = -1;
static int auxPort   = -1;
static int smbPort   = -1;
static int mediaPort = -1;
static int httpPort  = -1;
static int slavePort = -1;

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

int diffTs;

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
    #ifdef TEST
    *logofs << "NXTransProxy: Out of the long jump with pid '"
            << lastProxy << "'.\n" << logofs_flush;
    #endif
    
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

  #ifdef TEST
  *logofs << "NXTransProxy: Main process started with pid '"
          << lastProxy << "'.\n" << logofs_flush;
  #endif

  SetMode(mode);

  if (mode == NX_MODE_CLIENT)
  {
    if (fd != NX_FD_ANY)
    {
      #ifdef TEST

      *logofs << "NXTransProxy: Agent descriptor for X client connections is FD#"
              << fd << ".\n" << logofs_flush;

      *logofs << "NXTransProxy: Disabling listening on further X client connections.\n"
              << logofs_flush;

      #endif

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
      #ifdef TEST
      *logofs << "NXTransProxy: PANIC! Agent descriptor for X server connections "
              << "not supported yet.\n" << logofs_flush;
      #endif

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

  #ifdef TEST
  *logofs << "NXTransProxy: Going to run the NX transport loop.\n"
          << logofs_flush;
  #endif

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
    #ifdef TEST
    *logofs << "NXTransExit: Aborting process with pid '"
            << getpid() << "' due to recursion through "
            << "exit.\n" << logofs_flush;
    #endif

    abort();
  }

  #ifdef TEST
  *logofs << "NXTransExit: Process with pid '"
          << getpid() << "' called exit with code '"
          << code << "'.\n" << logofs_flush;
  #endif

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
    #ifdef PANIC
    *logofs << "NXTransCreate: PANIC! The NX transport seems "
            << "to be already running.\n" << logofs_flush;
    #endif

    cerr << "Error" << ": The NX transport seems "
         << "to be already running.\n";

    return -1;
  }

  control = new Control();

  if (control == NULL)
  {
    #ifdef PANIC
    *logofs << "Loop: PANIC! Error creating the NX transport.\n"
            << logofs_flush;
    #endif

    cerr << "Error" << ": Error creating the NX transport.\n";

    return -1;
  }

  lastProxy = getpid();

  #ifdef TEST
  *logofs << "NXTransCreate: Caller process running with pid '"
          << lastProxy << "'.\n" << logofs_flush;
  #endif

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

  #ifdef TEST
  *logofs << "NXTransCreate: Called with NX proxy descriptor '"
          << proxyFD << "'.\n" << logofs_flush;
  #endif

  #ifdef TEST
  *logofs << "NXTransCreate: Creation of the NX transport completed.\n"
          << logofs_flush;
  #endif

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
    #ifdef PANIC
    *logofs << "NXTransAgent: Invalid mode while setting the NX agent.\n"
            << logofs_flush;
    #endif

    cerr << "Error" << ": Invalid mode while setting the NX agent.\n\n";

    return -1;
  }

  useTcpSocket   = 0;
  useUnixSocket  = 0;
  useAgentSocket = 1;

  agentFD[0] = fd[0];
  agentFD[1] = fd[1];

  #ifdef TEST

  *logofs << "NXTransAgent: Internal descriptors for agent are FD#"
          << agentFD[0] << " and FD#" << agentFD[1] << ".\n"
          << logofs_flush;

  *logofs << "NXTransAgent: Disabling listening for further X client "
          << "connections.\n" << logofs_flush;

  #endif

  agent = new Agent(agentFD);

  if (agent == NULL || agent -> isValid() != 1)
  {
    #ifdef PANIC
    *logofs << "Loop: PANIC! Error creating the NX memory transport .\n"
            << logofs_flush;
    #endif

    cerr << "Error" << ": Error creating the NX memory transport.\n";

    HandleCleanup();
  }

  #ifdef TEST
  *logofs << "NXTransAgent: Enabling memory-to-memory transport.\n"
          << logofs_flush;
  #endif

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
      #ifdef TEST
      *logofs << "NXTransClose: Closing down all the X connections.\n"
              << logofs_flush;
      #endif

      CleanupConnections();
    }
  }
  #ifdef TEST
  else
  {
    *logofs << "NXTransClose: The NX transport is not running.\n"
            << logofs_flush;
  }
  #endif

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
      #ifdef TEST
      *logofs << "NXTransDestroy: Closing down all the X connections.\n"
              << logofs_flush;
      #endif

      CleanupConnections();
    }

    #ifdef TEST
    *logofs << "NXTransDestroy: Waiting for the NX transport to terminate.\n"
            << logofs_flush;
    #endif

    lastDestroy = 1;

    WaitCleanup();
  }
  #ifdef TEST
  else
  {
    *logofs << "NXTransDestroy: The NX transport is not running.\n"
            << logofs_flush;
  }
  #endif

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
    #ifdef TEST
    *logofs << "NXTransSignal: Raising signal '" << DumpSignal(signal)
            << "' in the proxy handler.\n" << logofs_flush;
    #endif

    HandleSignal(signal);

    return 1;
  }
  else if (signal == NX_SIGNAL_ANY)
  {
    #ifdef TEST
    *logofs << "NXTransSignal: Setting action of all signals to '"
            << action << "'.\n" << logofs_flush;
    #endif

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
    #ifdef TEST
    *logofs << "NXTransSignal: Setting action of signal '"
            << DumpSignal(signal) << "' to '" << action
            << "'.\n" << logofs_flush;
    #endif

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

  #ifdef WARNING
  *logofs << "NXTransSignal: WARNING! Unable to perform action '"
          << action << "' on signal '" << DumpSignal(signal)
          << "'.\n" << logofs_flush;
  #endif

  cerr << "Warning" << ": Unable to perform action '" << action
       << "' on signal '" << DumpSignal(signal)
       << "'.\n";

  return -1;
}

int NXTransCongestion(int fd)
{
  if (control != NULL && proxy != NULL)
  {
    #ifdef DUMP

    int congestion = proxy -> getCongestion(proxyFD);

    *logofs << "NXTransCongestion: Returning " << congestion
            << " as current congestion level.\n" << logofs_flush;

    return congestion;

    #endif

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
      #ifdef TEST
      *logofs << "NXTransHandler: WARNING! Failed to set "
              << "the NX callback for event '" << type << "' to '"
              << (void *) handler << "' and parameter '"
              << parameter << "'.\n" << logofs_flush;
      #endif

      return 0;
    }
  }

  #ifdef TEST
  *logofs << "NXTransHandler: Set the NX "
          << "callback for event '" << type << "' to '"
          << (void *) handler << "' and parameter '"
          << parameter << "'.\n" << logofs_flush;
  #endif

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
    #ifdef DUMP
    *logofs << "NXTransRead: Dequeuing " << size << " bytes "
            << "from FD#" << agentFD[0] << ".\n" << logofs_flush;
    #endif

    int result = agent -> dequeueData(data, size);

    #ifdef DUMP

    if (result < 0 && EGET() == EAGAIN)
    {
      *logofs << "NXTransRead: WARNING! Dequeuing from FD#"
              << agentFD[0] << " would block.\n" << logofs_flush;
    }
    else
    {
      *logofs << "NXTransRead: Dequeued " << result << " bytes "
              << "to FD#" << agentFD[0] << ".\n" << logofs_flush;
    }

    #endif

    return result;
  }
  else
  {
    #ifdef DUMP
    *logofs << "NXTransRead: Reading " << size << " bytes "
            << "from FD#" << fd << ".\n" << logofs_flush;
    #endif

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
    #if defined(DUMP)

    if (control -> ProxyStage >= stage_operational &&
            agent -> localReadable() > 0)
    {
      *logofs << "NXTransReadVector: WARNING! Agent has data readable.\n"
              << logofs_flush;
    }

    #endif

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
        #ifdef DUMP
        *logofs << "NXTransReadVector: Dequeuing " << length
                << " bytes " << "from FD#" << agentFD[0] << ".\n"
                << logofs_flush;
        #endif

        result = agent -> dequeueData(base, length);

        #ifdef DUMP

        if (result < 0 && EGET() == EAGAIN)
        {
          *logofs << "NXTransReadVector: WARNING! Dequeuing from FD#"
                  << agentFD[0] << " would block.\n" << logofs_flush;
        }
        else
        {
          *logofs << "NXTransReadVector: Dequeued " << result
                  << " bytes " << "from FD#" << agentFD[0] << ".\n"
                  << logofs_flush;
        }

        #endif

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
    #ifdef DUMP
    *logofs << "NXTransReadVector: Reading vector with "
            << iovsize << " elements from FD#" << fd << ".\n"
            << logofs_flush;
    #endif

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
    #ifdef DUMP

    int result = GetBytesReadable(fd, readable);

    if (result == -1)
    {
      *logofs << "NXTransReadable: Error detected on FD#"
              << fd << ".\n" << logofs_flush;
    }
    else
    {
      *logofs << "NXTransReadable: Returning " << *readable
              << " bytes as readable from FD#" << fd
              << ".\n" << logofs_flush;
    }

    return result;

    #else

    return GetBytesReadable(fd, readable);

    #endif
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
        #if defined(TEST) || defined(INFO)
        *logofs << "NXTransReadable: WARNING! Trying to "
                << "read to generate new agent data.\n"
                << logofs_flush;
        #endif

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
          #if defined(TEST) || defined(INFO)
          *logofs << "NXTransReadable: Failure reading "
                  << "messages from proxy FD#" << proxyFD
                  << ".\n" << logofs_flush;
          #endif

          HandleShutdown();
        }

        //
        // Call again the routine. By reading
        // new control messages from the proxy
        // the agent channel may be gone.
        //

        return NXTransReadable(fd, readable);
      }

      #ifdef DUMP
      *logofs << "NXTransReadable: Returning " << 0
              << " bytes as readable from FD#" << fd
              << " with result 0.\n" << logofs_flush;
      #endif

      *readable = 0;

      return 0;
    }
    case -1:
    {
      #ifdef DUMP
      *logofs << "NXTransReadable: Returning " << 0
              << " bytes as readable from FD#" << fd
              << " with result -1.\n" << logofs_flush;
      #endif

      *readable = 0;

      return -1;
    }
    default:
    {
      #ifdef DUMP
      *logofs << "NXTransReadable: Returning " << result
              << " bytes as readable from FD#" << fd
              << " with result 0.\n" << logofs_flush;
      #endif

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
        #if defined(DUMP) || defined(TEST)
        *logofs << "NXTransWrite: WARNING! Delayed enqueuing to FD#"
                << agentFD[0] << " with proxy unable to read.\n"
                << logofs_flush;
        #endif

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

      #ifdef DUMP
      *logofs << "NXTransWrite: Letting the channel borrow "
              << size << " bytes from FD#" << agentFD[0]
              << ".\n" << logofs_flush;
      #endif

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

      #ifdef DUMP
      *logofs << "NXTransWrite: Enqueuing " << size << " bytes "
              << "to FD#" << agentFD[0] << ".\n" << logofs_flush;
      #endif

      result = agent -> enqueueData(data, size);
    }

    #ifdef DUMP

    if (result < 0)
    {
      if (EGET() == EAGAIN)
      {
        *logofs << "NXTransWrite: WARNING! Enqueuing to FD#"
                << agentFD[0] << " would block.\n"
                << logofs_flush;
      }
      else
      {
        *logofs << "NXTransWrite: WARNING! Error enqueuing to FD#"
                << agentFD[0] << ".\n" << logofs_flush;
      }
    }
    else
    {
      *logofs << "NXTransWrite: Enqueued " << result << " bytes "
              << "to FD#" << agentFD[0] << ".\n" << logofs_flush;
    }

    #endif

    return result;
  }
  else
  {
    #ifdef DUMP
    *logofs << "NXTransWrite: Writing " << size << " bytes "
            << "to FD#" << fd << ".\n" << logofs_flush;
    #endif

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
        #if defined(DUMP) || defined(TEST)
        *logofs << "NXTransWriteVector: WARNING! Delayed enqueuing to FD#"
                << agentFD[0] << " with proxy unable to read.\n"
                << logofs_flush;
        #endif

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

          #ifdef DUMP
          *logofs << "NXTransWriteVector: Letting the channel borrow "
                  << length << " bytes from FD#" << agentFD[0]
                  << ".\n" << logofs_flush;
          #endif

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

          #ifdef DUMP
          *logofs << "NXTransWriteVector: Enqueuing " << length
                  << " bytes " << "to FD#" << agentFD[0] << ".\n"
                  << logofs_flush;
          #endif

          result = agent -> enqueueData(base, length);
        }

        #ifdef DUMP

        if (result < 0)
        {
          if (EGET() == EAGAIN)
          {
            *logofs << "NXTransWriteVector: WARNING! Enqueuing to FD#"
                    << agentFD[0] << " would block.\n"
                    << logofs_flush;
          }
          else
          {
            *logofs << "NXTransWriteVector: WARNING! Error enqueuing to FD#"
                    << agentFD[0] << ".\n" << logofs_flush;
          }
        }
        else
        {
          *logofs << "NXTransWriteVector: Enqueued " << result
                  << " bytes " << "to FD#" << agentFD[0] << ".\n"
                  << logofs_flush;
        }

        #endif

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
    #ifdef DUMP
    *logofs << "NXTransWriteVector: Writing vector with "
            << iovsize << " elements to FD#" << fd << ".\n"
            << logofs_flush;
    #endif

    return writev(fd, iovdata, iovsize);
  }
}

int NXTransPolicy(int fd, int type)
{
  if (control != NULL)
  {
    if (usePolicy == -1)
    {
      #if defined(TEST) || defined(INFO)
      *logofs << "NXTransPolicy: Setting flush policy on "
              << "proxy FD#" << proxyFD << " to '"
              << DumpPolicy(type == NX_POLICY_DEFERRED ?
                     policy_deferred : policy_immediate)
              << "'.\n" << logofs_flush;
      #endif

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
      #if defined(TEST) || defined(INFO)
      *logofs << "NXTransPolicy: Ignoring the agent "
              << "setting with user policy set to '"
              << DumpPolicy(control -> FlushPolicy)
              << "'.\n" << logofs_flush;
      #endif

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
    #ifdef DUMP
    *logofs << "NXTransFlushable: Returning 0 bytes as "
            << "flushable for unrecognized FD#" << fd
            << ".\n" << logofs_flush;
    #endif

    return 0;
  }
  else
  {
    #if defined(DUMP) || defined(INFO)
    *logofs << "NXTransFlushable: Returning " << proxy ->
               getFlushable(proxyFD) << " as bytes flushable on "
            << "proxy FD#" << proxyFD << ".\n"
            << logofs_flush;
    #endif

    return proxy -> getFlushable(proxyFD);
  }
}

int NXTransFlush(int fd)
{
  if (proxy != NULL)
  {
    #if defined(TEST) || defined(INFO)
    *logofs << "NXTransFlush: Requesting an immediate flush of "
            << "proxy FD#" << proxyFD << ".\n"
            << logofs_flush;
    #endif

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

    #if defined(TEST) || defined(INFO)
    *logofs << "NXTransChannel: Going to create a new channel "
            << "with type '" << type << "' on FD#" << channelFd
            << ".\n" << logofs_flush;
    #endif

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
        #ifdef WARNING
        *logofs << "NXTransChannel: WARNING! Unrecognized channel "
                << "type '" << type << "'.\n" << logofs_flush;
        #endif

        break;
      }
    }

    #ifdef WARNING

    if (result != 1)
    {
      *logofs << "NXTransChannel: WARNING! Could not create the "
                << "new channel with type '" << type << "' on FD#"
                << channelFd << ".\n" << logofs_flush;
    }

    #endif

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
    #if defined(DUMP) || defined(INFO)
    *logofs << "NXTransAlert: Requesting a NX dialog with code "
            << code << " and local " << local << ".\n"
            << logofs_flush;
    #endif

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
  #if defined(DUMP) || defined(INFO)
  else
  {
    if (logofs == NULL)
    {
      logofs = &cerr;
    }

    *logofs << "NXTransAlert: Can't request an alert without "
            << "a valid NX transport.\n" << logofs_flush;
  }
  #endif

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

  #if defined(TEST) || defined(INFO)
  *logofs << "\nNXTransPrepare: Going to prepare the NX transport.\n"
          << logofs_flush;
  #endif

  if (control -> ProxyStage < stage_operational)
  {
    handleNegotiationInLoop(*setFDs, *readSet, *writeSet, *selectTs);
  }
  else
  {
    #if defined(TEST) || defined(INFO)

    if (isTimestamp(*selectTs) == 0)
    {
      *logofs << "Loop: WARNING! Preparing the select with requested "
              << "timeout of " << selectTs -> tv_sec << " S and "
              << (double) selectTs -> tv_usec / 1000 << " Ms.\n"
              << logofs_flush;
    }
    else
    {
      *logofs << "Loop: Preparing the select with requested "
              << "timeout of " << selectTs -> tv_sec << " S and "
              << (double) selectTs -> tv_usec / 1000 << " Ms.\n"
              << logofs_flush;
    }

    #endif

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

  diffTs = diffTimestamp(startTs, nowTs);

  #ifdef TEST
  *logofs << "Loop: Mark - 0 - at " << strMsTimestamp()
          << " with " << diffTs << " Ms elapsed.\n"
          << logofs_flush;
  #endif

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

  #ifdef DEBUG
  *logofs << "Loop: New timestamp is " << strMsTimestamp(startTs)
          << ".\n" << logofs_flush;
  #endif

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
  #ifdef TIME

  static T_timestamp lastTs;

  #endif

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

  #if defined(TEST) || defined(INFO)
  *logofs << "\nNXTransSelect: Going to select the NX descriptors.\n"
          << logofs_flush;
  #endif

  #if defined(TEST) || defined(INFO)

  handleCheckSelectInLoop(*setFDs, *readSet, *writeSet, *selectTs);

  #endif

  #ifdef TIME

  diffTs = diffTimestamp(lastTs, getNewTimestamp());

  if (diffTs > 20)
  {
    *logofs << "Loop: TIME! Spent " << diffTs
            << " Ms handling messages for proxy FD#"
            << proxyFD << ".\n" << logofs_flush;
  }

  lastTs = getNewTimestamp();

  #endif

  #if defined(TEST) || defined(INFO)

  if (isTimestamp(*selectTs) == 0)
  {
    *logofs << "Loop: WARNING! Executing the select with requested "
            << "timeout of " << selectTs -> tv_sec << " S and "
            << (double) selectTs -> tv_usec / 1000 << " Ms.\n"
            << logofs_flush;
  }
  else if (proxy != NULL && proxy -> getFlushable(proxyFD) > 0)
  {
    *logofs << "Loop: WARNING! Proxy FD#" << proxyFD
            << " has " << proxy -> getFlushable(proxyFD)
            << " bytes to write but timeout is "
            << selectTs -> tv_sec << " S and "
            << selectTs -> tv_usec / 1000 << " Ms.\n"
            << logofs_flush;
  }

  #endif

  //
  // Wait for the selected sockets
  // or the timeout.
  //

  ESET(0);

  *resultFDs = select(*setFDs, readSet, writeSet, NULL, selectTs);

  *errorFDs = EGET();

  #ifdef TIME

  diffTs = diffTimestamp(lastTs, getNewTimestamp());

  if (diffTs > 100)
  {
    *logofs << "Loop: TIME! Spent " << diffTs
            << " Ms waiting for new data for proxy FD#"
            << proxyFD << ".\n" << logofs_flush;
  }

  lastTs = getNewTimestamp();

  #endif

  //
  // Check the result of the select.
  //

  #if defined(TEST) || defined(INFO)

  handleCheckResultInLoop(*resultFDs, *errorFDs, *setFDs, *readSet, *writeSet, *selectTs, startTs);

  #endif

  //
  // Get time spent in select. The accouting is done
  // in milliseconds. This is a real problem on fast
  // machines where each loop is unlikely to take
  // more than 500 us, so consider that the results
  // can be inaccurate.
  //

  nowTs = getNewTimestamp();

  diffTs = diffTimestamp(startTs, nowTs);

  #ifdef TEST
  *logofs << "Loop: Out of select after " << diffTs << " Ms "
          << "at " << strMsTimestamp(nowTs) << " with result "
          << *resultFDs << ".\n" << logofs_flush;
  #endif

  startTs = nowTs;

  #ifdef DEBUG
  *logofs << "Loop: New timestamp is " << strMsTimestamp(startTs)
          << ".\n" << logofs_flush;
  #endif

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
      #ifdef TEST

      if (*errorFDs == EINTR)
      {
        *logofs << "Loop: Select failed due to EINTR error.\n"
                << logofs_flush;
      }
      else
      {
        *logofs << "Loop: WARNING! Call to select failed. Error is "
                << EGET() << " '" << ESTR() << "'.\n"
                << logofs_flush;
      }

      #endif
    }
    else
    {
      #ifdef PANIC
      *logofs << "Loop: PANIC! Call to select failed. Error is "
              << EGET() << " '" << ESTR() << "'.\n"
              << logofs_flush;
      #endif

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

  #if defined(TEST) || defined(INFO)
  *logofs << "\nNXTransExecute: Going to execute I/O on the NX descriptors.\n"
          << logofs_flush;
  #endif

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

    #ifdef TEST
    *logofs << "Loop: Mark - 1 - at " << strMsTimestamp()
            << " with " << diffTimestamp(startTs, getTimestamp())
            << " Ms elapsed.\n" << logofs_flush;
    #endif

    //
    // Rotate the channel that will be handled
    // first.
    //

    handleRotateInLoop();

    //
    // Flush any data on newly writable sockets.
    //

    handleWritableInLoop(*resultFDs, *writeSet);

    #ifdef TEST
    *logofs << "Loop: Mark - 2 - at " << strMsTimestamp()
            << " with " << diffTimestamp(startTs, getTimestamp())
            << " Ms elapsed.\n" << logofs_flush;
    #endif

    //
    // Check if any socket has become readable.
    //

    handleReadableInLoop(*resultFDs, *readSet);

    #ifdef TEST
    *logofs << "Loop: Mark - 3 - at " << strMsTimestamp()
            << " with " << diffTimestamp(startTs, getTimestamp())
            << " Ms elapsed.\n" << logofs_flush;
    #endif

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

    #ifdef TEST
    *logofs << "Loop: Mark - 4 - at " << strMsTimestamp()
            << " with " << diffTimestamp(startTs, getTimestamp())
            << " Ms elapsed.\n" << logofs_flush;
    #endif

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

    #ifdef TEST
    *logofs << "Loop: Mark - 5 - at " << strMsTimestamp()
            << " with " << diffTimestamp(startTs, getTimestamp())
            << " Ms elapsed.\n" << logofs_flush;
    #endif

    //
    // Check if there is any data to flush.
    // Agents should flush the proxy link
    // explicitly.
    //

    handleFlushInLoop();

    #ifdef TEST
    *logofs << "Loop: Mark - 6 - at " << strMsTimestamp()
            << " with " << diffTimestamp(startTs, getTimestamp())
            << " Ms elapsed.\n" << logofs_flush;
    #endif
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

    #if defined(TEST) || defined(INFO)

    handleCheckStateInLoop(*setFDs);

    #endif

    #ifdef TEST
    *logofs << "Loop: Mark - 7 - at " << strMsTimestamp()
            << " with " << diffTimestamp(startTs, getTimestamp())
            << " Ms elapsed.\n" << logofs_flush;
    #endif
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

  #ifdef TEST
  *logofs << "Loop: INIT! Taking mark for initialization at "
          << strMsTimestamp(initTs) << ".\n"
          << logofs_flush;
  #endif

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
    #ifdef TEST
    *logofs << "Loop: Starting watchdog process with timeout of "
            << control -> InitTimeout / 1000 << " seconds.\n"
            << logofs_flush;
    #endif

    lastWatchdog = NXTransWatchdog(control -> InitTimeout);

    if (IsFailed(lastWatchdog))
    {
      #ifdef PANIC
      *logofs << "Loop: PANIC! Can't start the NX watchdog process.\n"
              << logofs_flush;
      #endif

      SetNotRunning(lastWatchdog);
    }
    #ifdef TEST
    else
    {
      *logofs << "Loop: Watchdog started with pid '"
              << lastWatchdog << "'.\n" << logofs_flush;
    }
    #endif
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
  // Set local endianess.
  //

  unsigned int test = 1;

  setHostBigEndian(*((unsigned char *) (&test)) == 0);

  #ifdef TEST
  *logofs << "Loop: Local host is "
          << (hostBigEndian() ? "big endian" : "little endian")
          << ".\n" << logofs_flush;
  #endif

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
    if (WE_INITIATE_CONNECTION)
    {
      if (connectPort < 0)
      {
        connectPort = DEFAULT_NX_PROXY_PORT_OFFSET + proxyPort;
      }

      #ifdef TEST
      *logofs << "Loop: Going to connect to " << connectHost
              << ":" << connectPort << ".\n" << logofs_flush;
      #endif

      proxyFD = ConnectToRemote(connectHost, connectPort);

      #ifdef TEST
      *logofs << "Loop: Connected to remote proxy on FD#"
              << proxyFD << ".\n" << logofs_flush;
      #endif

      cerr << "Info" << ": Connection to remote proxy '" << connectHost
           << ":" << connectPort << "' established.\n";
    }
    else
    {
      if (listenPort < 0)
      {
        listenPort = DEFAULT_NX_PROXY_PORT_OFFSET + proxyPort;
      }

      #ifdef TEST
      *logofs << "Loop: Going to wait for connection on port " 
              << listenPort << ".\n" << logofs_flush;
      #endif

      proxyFD = WaitForRemote(listenPort);

      #ifdef TEST

      if (WE_LISTEN_FORWARDER)
      {
        *logofs << "Loop: Connected to remote forwarder on FD#"
                << proxyFD << ".\n" << logofs_flush;
      }
      else
      {
        *logofs << "Loop: Connected to remote proxy on FD#"
                << proxyFD << ".\n" << logofs_flush;
      }

      #endif
    }
  }
  #ifdef TEST
  else
  {
    *logofs << "Loop: Using the inherited connection on FD#"
            << proxyFD << ".\n" << logofs_flush;
  }
  #endif

  //
  // Set TCP_NODELAY on proxy descriptor
  // to reduce startup time. Option will
  // later be disabled if needed.
  //

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
  #ifdef TEST
  *logofs << "Loop: Connection with remote proxy completed.\n"
          << logofs_flush;
  #endif

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

  #ifdef TEST
  *logofs << "Loop: INIT! Completed initialization at "
          << strMsTimestamp(nowTs) << " with "
          << diffTimestamp(initTs, nowTs) << " Ms "
          << "since the init mark.\n" << logofs_flush;
  #endif

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
      #ifdef TEST
      *logofs << "Loop: INIT! Initializing with mode "
              << "NX_MODE_CLIENT at " << strMsTimestamp()
              << ".\n" << logofs_flush;
      #endif

      control -> ProxyMode = proxy_client;
    }
    else if (mode == NX_MODE_SERVER)
    {
      #ifdef TEST
      *logofs << "Loop: INIT! Initializing with mode "
              << "NX_MODE_SERVER at " << strMsTimestamp()
              << ".\n" << logofs_flush;
      #endif

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
    #ifdef PANIC
    *logofs << "Loop: PANIC! Error creating the NX proxy.\n"
            << logofs_flush;
    #endif

    cerr << "Error" << ": Error creating the NX proxy.\n";

    HandleCleanup();
  }

  //
  // Create the statistics repository.
  //

  statistics = new Statistics(proxy);

  if (statistics == NULL)
  {
    #ifdef PANIC
    *logofs << "Loop: PANIC! Error creating the NX statistics.\n"
            << logofs_flush;
    #endif

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
    #ifdef PANIC
    *logofs << "Loop: PANIC! Error configuring the NX transport.\n"
            << logofs_flush;
    #endif

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

  #if defined(TEST) || defined(INFO)

  if (proxy -> getFlushable(proxyFD) > 0)
  {
    *logofs << "Loop: WARNING! Proxy FD#" << proxyFD << " has data "
            << "to flush after setup of the NX transport.\n"
            << logofs_flush;
  }

  #endif

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
    if (authCookie != NULL && *authCookie != '\0')
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

        const int launchdAddrNameLength = 108;

        int success = -1;

        strncpy(launchdAddrUnix.sun_path, displayHost, launchdAddrNameLength);

        *(launchdAddrUnix.sun_path + launchdAddrNameLength - 1) = '\0';

        #ifdef TEST
        *logofs << "Loop: Connecting to launchd service "
                << "on Unix port '" << displayHost << "'.\n" << logofs_flush;
        #endif

        int launchdFd = socket(launchdAddrFamily, SOCK_STREAM, PF_UNSPEC);

        if (launchdFd < 0)
        {
          #ifdef PANIC
          *logofs << "Loop: PANIC! Call to socket failed. "
                  << "Error is " << EGET() << " '" << ESTR()
                  << "'.\n" << logofs_flush;
          #endif
        }
        else if ((success = connect(launchdFd, (sockaddr *) &launchdAddrUnix, launchdAddrLength)) < 0)
        {
          #ifdef WARNING
          *logofs << "Loop: WARNING! Connection to launchd service "
                  << "on Unix port '" << displayHost << "' failed "
                  << "with error " << EGET() << ", '" << ESTR() << "'.\n"
                  << logofs_flush;
          #endif
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
        #ifdef PANIC
        *logofs << "Loop: PANIC! Error creating the X authorization.\n"
                << logofs_flush;
        #endif

        cerr << "Error" << ": Error creating the X authorization.\n";

        HandleCleanup();
      }
      else if (auth -> isFake() == 1)
      {
        #ifdef WARNING
        *logofs << "Loop: WARNING! Could not retrieve the X server "
                << "authentication cookie.\n"
                << logofs_flush;
        #endif

        cerr << "Warning" << ": Failed to read data from the X "
             << "auth command.\n";

        cerr << "Warning" << ": Generated a fake cookie for X "
             << "authentication.\n";
      }
    }
    else
    {
      #ifdef TEST
      *logofs << "Loop: No proxy cookie was provided for "
              << "authentication.\n" << logofs_flush;
      #endif

      cerr << "Info" << ": No proxy cookie was provided for "
           << "authentication.\n";

      #ifdef TEST
      *logofs << "Loop: Forwarding the real X authorization "
              << "cookie.\n" << logofs_flush;
      #endif

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
      #ifdef PANIC
      *logofs << "Loop: PANIC! Error creating the NX agent connection.\n"
              << logofs_flush;
      #endif

      cerr << "Error" << ": Error creating the NX agent connection.\n";

      HandleCleanup();
    }
  }

  return 1;
}

int SetupTcpSocket()
{
  //
  // Open TCP socket emulating local display.
  //

  tcpFD = socket(AF_INET, SOCK_STREAM, PF_UNSPEC);

  if (tcpFD == -1)
  {
    #ifdef PANIC
    *logofs << "Loop: PANIC! Call to socket failed for TCP socket"
            << ". Error is " << EGET() << " '" << ESTR() << "'.\n"
            << logofs_flush;
    #endif

    cerr << "Error" << ": Call to socket failed for TCP socket"
         << ". Error is " << EGET() << " '" << ESTR() << "'.\n";

    HandleCleanup();
  }
  else if (SetReuseAddress(tcpFD) < 0)
  {
    HandleCleanup();
  }

  unsigned int proxyPortTCP = X_TCP_PORT + proxyPort;

  sockaddr_in tcpAddr;

  tcpAddr.sin_family = AF_INET;
  tcpAddr.sin_port = htons(proxyPortTCP);
  if ( loopbackBind )
  {
    tcpAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  }
  else
  {
    tcpAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  }

  if (bind(tcpFD, (sockaddr *) &tcpAddr, sizeof(tcpAddr)) == -1)
  {
    #ifdef PANIC
    *logofs << "Loop: PANIC! Call to bind failed for TCP port "
            << proxyPortTCP << ". Error is " << EGET() << " '" << ESTR()
            << "'.\n" << logofs_flush;
    #endif

    cerr << "Error" << ": Call to bind failed for TCP port "
         << proxyPortTCP << ". Error is " << EGET() << " '" << ESTR()
         << "'.\n";

    HandleCleanup();
  }

  if (listen(tcpFD, 8) == -1)
  {
    #ifdef PANIC
    *logofs << "Loop: PANIC! Call to listen failed for TCP port "
            << proxyPortTCP << ". Error is " << EGET() << " '" << ESTR()
            << "'.\n" << logofs_flush;
    #endif

    cerr << "Error" << ": Call to listen failed for TCP port "
         << proxyPortTCP << ". Error is " << EGET() << " '" << ESTR()
         << "'.\n";

    HandleCleanup();
  }

  return 1;
}

int SetupUnixSocket()
{
  //
  // Open UNIX domain socket for display.
  //

  unixFD = socket(AF_UNIX, SOCK_STREAM, PF_UNSPEC);

  if (unixFD == -1)
  {
    #ifdef PANIC
    *logofs << "Loop: PANIC! Call to socket failed for UNIX domain"
            << ". Error is " << EGET() << " '" << ESTR() << "'.\n"
            << logofs_flush;
    #endif

    cerr << "Error" << ": Call to socket failed for UNIX domain"
         << ". Error is " << EGET() << " '" << ESTR() << "'.\n";

    HandleCleanup();
  }

  sockaddr_un unixAddr;
  unixAddr.sun_family = AF_UNIX;

  char dirName[DEFAULT_STRING_LENGTH];

  snprintf(dirName, DEFAULT_STRING_LENGTH - 1, "%s/.X11-unix",
               control -> TempPath);

  *(dirName + DEFAULT_STRING_LENGTH - 1) = '\0';

  struct stat dirStat;

  if ((stat(dirName, &dirStat) == -1) && (EGET() == ENOENT))
  {
    mkdir(dirName, (0777 | S_ISVTX));
    chmod(dirName, (0777 | S_ISVTX));
  }

  snprintf(unixSocketName,  DEFAULT_STRING_LENGTH - 1, "%s/X%d",
               dirName, proxyPort);

  strncpy(unixAddr.sun_path, unixSocketName, 108);

  #ifdef TEST
  *logofs << "Loop: Assuming Unix socket with name '"
          << unixAddr.sun_path << "'.\n"
          << logofs_flush;
  #endif

  *(unixAddr.sun_path + 107) = '\0';

  if (bind(unixFD, (sockaddr *) &unixAddr, sizeof(unixAddr)) == -1)
  {
    #ifdef PANIC
    *logofs << "Loop: PANIC! Call to bind failed for UNIX domain socket "
            << unixSocketName << ". Error is " << EGET() << " '" << ESTR()
            << "'.\n" << logofs_flush;
    #endif

    cerr << "Error" << ":  Call to bind failed for UNIX domain socket "
         << unixSocketName << ". Error is " << EGET() << " '" << ESTR()
         << "'.\n";

    HandleCleanup();
  }

  if (listen(unixFD, 8) == -1)
  {
    #ifdef PANIC
    *logofs << "Loop: PANIC! Call to listen failed for UNIX domain socket "
            << unixSocketName << ". Error is " << EGET() << " '" << ESTR()
            << "'.\n" << logofs_flush;
    #endif

    cerr << "Error" << ":  Call to listen failed for UNIX domain socket "
         << unixSocketName << ". Error is " << EGET() << " '" << ESTR()
         << "'.\n";

    HandleCleanup();
  }

  //
  // Let any local user to gain access to socket.
  //

  chmod(unixSocketName, 0777);

  return 1;
}

//
// The following is a dumb copy-paste. The
// nxcompsh library should offer a better
// implementation.
//

int SetupDisplaySocket(int &xServerAddrFamily, sockaddr *&xServerAddr,
                           unsigned int &xServerAddrLength)
{
  xServerAddrFamily = AF_INET;
  xServerAddr = NULL;
  xServerAddrLength = 0;

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
      #ifdef PANIC
      *logofs << "Loop: PANIC! Host X server DISPLAY is not set.\n"
              << logofs_flush;
      #endif

      cerr << "Error" << ": Host X server DISPLAY is not set.\n";

      HandleCleanup();
    }
    else if (strncasecmp(display, "nx/nx,", 6) == 0 ||
                 strncasecmp(display, "nx,", 3) == 0 ||
                     strncasecmp(display, "nx:", 3) == 0)
    {
      #ifdef PANIC
      *logofs << "Loop: PANIC! NX transport on host X server '"
              << display << "' not supported.\n" << logofs_flush;
      #endif

      cerr << "Error" << ": NX transport on host X server '"
           << display << "' not supported.\n";

      cerr << "Error" << ": Please run the local proxy specifying "
           << "the host X server to connect to.\n";

      HandleCleanup();
    }
    else if (strlen(display) >= DEFAULT_STRING_LENGTH)
    {
      #ifdef PANIC
      *logofs << "Loop: PANIC! Host X server DISPLAY cannot exceed "
              << DEFAULT_STRING_LENGTH << " characters.\n"
              << logofs_flush;
      #endif

      cerr << "Error" << ": Host X server DISPLAY cannot exceed "
           << DEFAULT_STRING_LENGTH << " characters.\n";

      HandleCleanup();
    }

    strcpy(displayHost, display);
  }

  display = new char[strlen(displayHost) + 1];

  if (display == NULL)
  {
    #ifdef PANIC
    *logofs << "Loop: PANIC! Out of memory handling DISPLAY variable.\n"
            << logofs_flush;
    #endif

    cerr << "Error" << ": Out of memory handling DISPLAY variable.\n";

    HandleCleanup();
  }

  strcpy(display, displayHost);

  #ifdef __APPLE__

  if (strncasecmp(display, "/tmp/launch", 11) == 0)
  {
    #ifdef TEST
    *logofs << "Loop: Using launchd service on socket '"
            << display << "'.\n" << logofs_flush;
    #endif

    useLaunchdSocket = 1;
  }

  #endif

  char *separator = rindex(display, ':');

  if ((separator == NULL) || !isdigit(*(separator + 1)))
  {
    #ifdef PANIC
    *logofs << "Loop: PANIC! Invalid display '" << display << "'.\n"
            << logofs_flush;
    #endif

    cerr << "Error" << ": Invalid display '" << display << "'.\n";

    HandleCleanup();
  }

  *separator = '\0';

  xPort = atoi(separator + 1);

  #ifdef TEST
  *logofs << "Loop: Using local X display '" << displayHost
          << "' with host '" << display << "' and port '"
          << xPort << "'.\n" << logofs_flush;
  #endif

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

    #ifdef TEST
    *logofs << "Loop: Using real X server on UNIX domain socket.\n"
            << logofs_flush;
    #endif

    sockaddr_un *xServerAddrUNIX = new sockaddr_un;

    xServerAddrFamily = AF_UNIX;
    xServerAddrUNIX -> sun_family = AF_UNIX;

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

    struct stat statInfo;

    char unixSocketDir[DEFAULT_STRING_LENGTH];

    snprintf(unixSocketDir, DEFAULT_STRING_LENGTH - 1, "%s/.X11-unix",
                 control -> TempPath);

    #ifdef __APPLE__

    if (useLaunchdSocket == 1)
    {
      char *slash = rindex(display, '/');

      if (slash != NULL)
      {
        *slash = '\0';
      }

      snprintf(unixSocketDir, DEFAULT_STRING_LENGTH - 1, "%s", display);
    }

    #endif

    *(unixSocketDir + DEFAULT_STRING_LENGTH - 1) = '\0';

    #ifdef TEST
    *logofs << "Loop: Assuming X socket in directory '"
            << unixSocketDir << "'.\n" << logofs_flush;
    #endif

    if (stat(unixSocketDir, &statInfo) < 0)
    {
      #ifdef PANIC
      *logofs << "Loop: PANIC! Can't determine the location of "
              << "the X display socket.\n" << logofs_flush;
      #endif

      cerr << "Error" << ": Can't determine the location of "
           << "the X display socket.\n";

      #ifdef PANIC
      *logofs << "Loop: PANIC! Error " << EGET() << " '" << ESTR()
              << "' checking '" << unixSocketDir << "'.\n"
              << logofs_flush;
      #endif

      cerr << "Error" << ": Error " << EGET() << " '" << ESTR()
           << "' checking '" << unixSocketDir << "'.\n";

      HandleCleanup();
    }

    sprintf(unixSocketName, "%s/X%d", unixSocketDir, xPort);

    #ifdef __APPLE__

    if (useLaunchdSocket == 1)
    {
      strncpy(unixSocketName, displayHost, DEFAULT_STRING_LENGTH - 1);
    }

    #endif

    #ifdef TEST
    *logofs << "Loop: Assuming X socket name '" << unixSocketName
            << "'.\n" << logofs_flush;
    #endif

    strcpy(xServerAddrUNIX -> sun_path, unixSocketName);

    xServerAddr = (sockaddr *) xServerAddrUNIX;
    xServerAddrLength = sizeof(sockaddr_un);
  }
  else
  {
    //
    // TCP port.
    //

    #ifdef TEST
    *logofs << "Loop: Using real X server on TCP port.\n"
            << logofs_flush;
    #endif

    xServerAddrFamily = AF_INET;

    int xServerIPAddr = GetHostAddress(display);

    if (xServerIPAddr == 0)
    {
      #ifdef PANIC
      *logofs << "Loop: PANIC! Unknown display host '" << display
              << "'.\n" << logofs_flush;
      #endif

      cerr << "Error" << ": Unknown display host '" << display
           << "'.\n";

      HandleCleanup();
    }

    sockaddr_in *xServerAddrTCP = new sockaddr_in;

    xServerAddrTCP -> sin_family = AF_INET;
    xServerAddrTCP -> sin_port = htons(X_TCP_PORT + xPort);
    xServerAddrTCP -> sin_addr.s_addr = xServerIPAddr;

    xServerAddr = (sockaddr *) xServerAddrTCP;
    xServerAddrLength = sizeof(sockaddr_in);
  }

  delete [] display;

  return 1;
}

int SetupServiceSockets()
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
    // Disable the font server connections if
    // they are not supported by the remote
    // proxy.
    //

    if (useFontSocket)
    {
      if (control -> isProtoStep7() == 1)
      {
        int port = atoi(fontPort);

        if ((fontFD = ListenConnection(port, "font")) < 0)
        {
          useFontSocket = 0;
        }
      }
      else
      {
        #ifdef WARNING
        *logofs << "Loop: WARNING! Font server connections not supported "
                << "by the remote proxy.\n" << logofs_flush;
        #endif

        cerr << "Warning" << ": Font server connections not supported "
             << "by the remote proxy.\n";

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
    if (control -> isProtoStep7() == 1)
    {
      if ((slaveFD = ListenConnection(slavePort, "slave")) < 0)
      {
        useSlaveSocket = 0;
      }
    }
    else
    {
      #ifdef WARNING
      *logofs << "Loop: WARNING! Slave connections not supported "
                << "by the remote proxy.\n" << logofs_flush;
      #endif

      cerr << "Warning" << ": Slave connections not supported "
           << "by the remote proxy.\n";

      useSlaveSocket = 0;
    }
  }

  return 1;
}

int ListenConnection(int port, const char *label)
{
  sockaddr_in tcpAddr;

  unsigned int portTCP = port;

  int newFD = socket(AF_INET, SOCK_STREAM, PF_UNSPEC);

  if (newFD == -1)
  {
    #ifdef PANIC
    *logofs << "Loop: PANIC! Call to socket failed for " << label
            << " TCP socket. Error is " << EGET() << " '"
            << ESTR() << "'.\n" << logofs_flush;
    #endif

    cerr << "Error" << ": Call to socket failed for " << label
         << " TCP socket. Error is " << EGET() << " '"
         << ESTR() << "'.\n";

    goto SetupSocketError;
  }
  else if (SetReuseAddress(newFD) < 0)
  {
    goto SetupSocketError;
  }

  tcpAddr.sin_family = AF_INET;
  tcpAddr.sin_port = htons(portTCP);
  if ( loopbackBind )
  {
    tcpAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  }
  else
  {
    tcpAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  }

  if (bind(newFD, (sockaddr *) &tcpAddr, sizeof(tcpAddr)) == -1)
  {
    #ifdef PANIC
    *logofs << "Loop: PANIC! Call to bind failed for " << label
            << " TCP port " << port << ". Error is " << EGET()
            << " '" << ESTR() << "'.\n" << logofs_flush;
    #endif

    cerr << "Error" << ": Call to bind failed for " << label
         << " TCP port " << port << ". Error is " << EGET()
         << " '" << ESTR() << "'.\n";

    goto SetupSocketError;
  }

  if (listen(newFD, 8) == -1)
  {
    #ifdef PANIC
    *logofs << "Loop: PANIC! Call to bind failed for " << label
            << " TCP port " << port << ". Error is " << EGET()
            << " '" << ESTR() << "'.\n" << logofs_flush;
    #endif

    cerr << "Error" << ": Call to bind failed for " << label
         << " TCP port " << port << ". Error is " << EGET()
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
}

static int AcceptConnection(int fd, int domain, const char *label)
{
  struct sockaddr newAddr;

  socklen_t addrLen = sizeof(newAddr);

  #ifdef TEST

  if (domain == AF_UNIX)
  {
    *logofs << "Loop: Going to accept new Unix " << label
            << " connection on FD#" << fd << ".\n"
            << logofs_flush;
  }
  else
  {
    *logofs << "Loop: Going to accept new TCP " << label
            << " connection on FD#" << fd << ".\n"
            << logofs_flush;
  }

  #endif

  int newFD = accept(fd, &newAddr, &addrLen);

  if (newFD < 0)
  {
    #ifdef PANIC
    *logofs << "Loop: PANIC! Call to accept failed for "
            << label << " connection. Error is " << EGET()
            << " '" << ESTR() << "'.\n" << logofs_flush;
    #endif

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
    #ifdef PANIC
    *logofs << "Loop: PANIC! No shutdown of proxy link "
            << "performed by remote proxy.\n"
            << logofs_flush;
    #endif

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

    #ifdef TEST
    *logofs << "Loop: Bytes received so far are "
            << (unsigned long long) statistics -> getBytesIn()
            << ".\n" << logofs_flush;
    #endif

    cerr << "Error" << ": Please check the state of your "
         << "network and retry.\n";

    handleTerminatingInLoop();

    if (control -> ProxyMode == proxy_server)
    {
      #ifdef TEST
      *logofs << "Loop: Showing the proxy abort dialog.\n"
              << logofs_flush;
      #endif

      HandleAlert(ABORT_PROXY_CONNECTION_ALERT, 1);

      handleAlertInLoop();
    }
  }
  #ifdef TEST
  else
  {
    *logofs << "Loop: Finalized the remote proxy shutdown.\n"
            << logofs_flush;
  }
  #endif

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
    #if defined(TEST) || defined(INFO)
    *logofs << "Loop: Killing the " << label << " process '"
            << pid << "' from process with pid '" << getpid()
            << "' with signal '" << DumpSignal(signal)
            << "'.\n" << logofs_flush;
    #endif

    signal = (signal == 0 ? SIGTERM : signal);

    if (kill(pid, signal) < 0 && EGET() != ESRCH)
    {
      #ifdef PANIC
      *logofs << "Loop: PANIC! Couldn't kill the " << label
              << " process with pid '" << pid << "'.\n"
              << logofs_flush;
      #endif

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
    #ifdef TEST
    *logofs << "Loop: No " << label << " process "
            << "to kill with pid '" << pid
            << "'.\n" << logofs_flush;
    #endif

    return 0;
  }
}

int CheckProcess(int pid, const char *label)
{
  #if defined(TEST) || defined(INFO)
  *logofs << "Loop: Checking the " << label << " process '"
          << pid << "' from process with pid '" << getpid()
          << "'.\n" << logofs_flush;
  #endif

  if (kill(pid, SIGCONT) < 0 && EGET() == ESRCH)
  {
    #ifdef WARNING
    *logofs << "Loop: WARNING! The " << label << " process "
            << "with pid '" << pid << "' has exited.\n"
            << logofs_flush;
    #endif

    cerr << "Warning" << ": The " << label << " process "
         << "with pid '" << pid << "' has exited.\n";

    return 0;
  }

  return 1;
}

int StartKeeper()
{
  #if defined(TEST) || defined(INFO)

  if (IsRunning(lastKeeper) == 1 ||
          IsRestarting(lastKeeper) == 1)
  {
    #ifdef PANIC
    *logofs << "Loop: PANIC! The house-keeping process is "
            << "alreay running with pid '" << lastKeeper
            << "'.\n" << logofs_flush;
    #endif

    HandleCleanup();
  }

  #endif

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
    #ifdef TEST
    *logofs << "Loop: Starting the house-keeping process with "
            << "storage size " << control -> PersistentCacheDiskLimit
            << ".\n" << logofs_flush;
    #endif

    lastKeeper = NXTransKeeper(control -> PersistentCacheDiskLimit,
                                   0, control -> RootPath);

    if (IsFailed(lastKeeper))
    {
      #ifdef WARNING
      *logofs << "Loop: WARNING! Failed to start the NX keeper process.\n"
              << logofs_flush;
      #endif

      cerr << "Warning" << ": Failed to start the NX keeper process.\n";

      SetNotRunning(lastKeeper);
    }
    #ifdef TEST
    else
    {
      *logofs << "Loop: Keeper started with pid '"
              << lastKeeper << "'.\n" << logofs_flush;
    }
    #endif
  }
  #ifdef TEST
  else
  {
    *logofs << "Loop: Nothing to do for the keeper process "
            << "with persistent cache not enabled.\n"
            << logofs_flush;
  }
  #endif

  return 1;
}

void HandleCleanup(int code)
{
  #ifdef TEST
  *logofs << "Loop: Going to clean up system resources "
          << "in process '" << getpid() << "'.\n"
          << logofs_flush;
  #endif

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

  #ifdef TEST

  if (getpid() == lastProxy)
  {
    *logofs << "Loop: Reverting to loop context in process with "
            << "pid '" << getpid() << "' at " << strMsTimestamp()
            << ".\n" << logofs_flush;
  }
  else
  {
    *logofs << "Loop: Exiting from child process with pid '"
            << getpid() << "' at " << strMsTimestamp()
            << ".\n" << logofs_flush;
  }

  #endif

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
    #ifdef TEST
    *logofs << "Loop: Freeing up keeper in process "
            << "with pid '" << getpid() << "'.\n"
            << logofs_flush;
    #endif

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

  #ifndef __CYGWIN32__

  #ifdef TEST
  *logofs << "Loop: Freeing up streams in process "
          << "with pid '" << getpid() << "'.\n"
          << logofs_flush;
  #endif

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

  #endif /* #ifndef __CYGWIN32__ */

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
    #if defined(TEST) || defined(INFO)
    *logofs << "Loop: WARNING! Leaving the dialog process '"
            << lastDialog << "' running in process "
            << "with pid '" << getpid() << "'.\n"
            << logofs_flush;
    #endif

    SetNotRunning(lastDialog);
  }

  //
  // Give user a chance to start a new session.
  //

  if (control -> EnableRestartOnShutdown == 1)
  {
    #ifdef WARNING
    *logofs << "Loop: WARNING! Respawning the NX client "
            << "on display '" << displayHost << "'.\n"
            << logofs_flush;
    #endif

    NXTransClient(displayHost);
  }

  for (int i = 0; i < control -> KillDaemonOnShutdownNumber; i++)
  {
    #ifdef WARNING
    *logofs << "Loop: WARNING! Killing the NX daemon with "
            << "pid '" << control -> KillDaemonOnShutdown[i]
            << "'.\n" << logofs_flush;
    #endif

    KillProcess(control -> KillDaemonOnShutdown[i], "daemon", SIGTERM, 0);
  }    
}

void CleanupGlobal()
{
  if (proxy != NULL)
  {
    #ifdef TEST
    *logofs << "Loop: Freeing up proxy in process "
            << "with pid '" << getpid() << "'.\n"
            << logofs_flush;
    #endif

    delete proxy;

    proxy = NULL;
  }

  if (agent != NULL)
  {
    #ifdef TEST
    *logofs << "Loop: Freeing up agent in process "
            << "with pid '" << getpid() << "'.\n"
            << logofs_flush;
    #endif

    delete agent;

    agent = NULL;
  }

  if (auth != NULL)
  {
    #ifdef TEST
    *logofs << "Loop: Freeing up auth data in process "
            << "with pid '" << getpid() << "'.\n"
            << logofs_flush;
    #endif

    delete auth;

    auth = NULL;
  }

  if (statistics != NULL)
  {
    #ifdef TEST
    *logofs << "Loop: Freeing up statistics in process "
            << "with pid '" << getpid() << "'.\n"
            << logofs_flush;
    #endif

    delete statistics;

    statistics = NULL;
  }

  if (control != NULL)
  {
    #ifdef TEST
    *logofs << "Loop: Freeing up control in process "
            << "with pid '" << getpid() << "'.\n"
            << logofs_flush;
    #endif

    delete control;

    control = NULL;
  }
}

void CleanupConnections()
{
  if (proxy -> getChannels(channel_x11) != 0)
  {
    #ifdef TEST
    *logofs << "Loop: Closing any remaining X connections.\n"
            << logofs_flush;
    #endif

    proxy -> handleCloseAllXConnections();

    #ifdef TEST
    *logofs << "Loop: Closing any remaining listener.\n"
            << logofs_flush;
    #endif

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
      #ifdef TEST
      *logofs << "Loop: Closing TCP listener in process "
              << "with pid '" << getpid() << "'.\n"
              << logofs_flush;
      #endif

      close(tcpFD);

      tcpFD = -1;
    }

    useTcpSocket = 0;
  }

  if (useUnixSocket == 1)
  {
    if (unixFD != -1)
    {
      #ifdef TEST
      *logofs << "Loop: Closing UNIX listener in process "
              << "with pid '" << getpid() << "'.\n"
              << logofs_flush;
      #endif

      close(unixFD);

      unixFD = -1;
    }

    if (*unixSocketName != '\0')
    {
      #ifdef TEST
      *logofs << "Loop: Going to remove the Unix domain socket '"
              << unixSocketName << "' in process " << "with pid '"
              << getpid() << "'.\n" << logofs_flush;
      #endif

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
      #ifdef TEST
      *logofs << "Loop: Closing CUPS listener in process "
              << "with pid '" << getpid() << "'.\n"
              << logofs_flush;
      #endif

      close(cupsFD);

      cupsFD = -1;
    }

    useCupsSocket = 0;
  }

  if (useAuxSocket == 1)
  {
    if (auxFD != -1)
    {
      #ifdef TEST
      *logofs << "Loop: Closing auxiliary X11 listener "
              << "in process " << "with pid '" << getpid()
              << "'.\n" << logofs_flush;
      #endif

      close(auxFD);

      auxFD = -1;
    }

    useAuxSocket = 0;
  }

  if (useSmbSocket == 1)
  {
    if (smbFD != -1)
    {
      #ifdef TEST
      *logofs << "Loop: Closing SMB listener in process "
              << "with pid '" << getpid() << "'.\n"
              << logofs_flush;
      #endif

      close(smbFD);

      smbFD = -1;
    }

    useSmbSocket = 0;
  }

  if (useMediaSocket == 1)
  {
    if (mediaFD != -1)
    {
      #ifdef TEST
      *logofs << "Loop: Closing multimedia listener in process "
              << "with pid '" << getpid() << "'.\n"
              << logofs_flush;
      #endif

      close(mediaFD);

      mediaFD = -1;
    }

    useMediaSocket = 0;
  }

  if (useHttpSocket == 1)
  {
    if (httpFD != -1)
    {
      #ifdef TEST
      *logofs << "Loop: Closing http listener in process "
              << "with pid '" << getpid() << "'.\n"
              << logofs_flush;
      #endif

      close(httpFD);

      httpFD = -1;
    }

    useHttpSocket = 0;
  }

  if (useFontSocket == 1)
  {
    if (fontFD != -1)
    {
      #ifdef TEST
      *logofs << "Loop: Closing font server listener in process "
              << "with pid '" << getpid() << "'.\n"
              << logofs_flush;
      #endif

      close(fontFD);

      fontFD = -1;
    }

    useFontSocket = 0;
  }

  if (useSlaveSocket == 1)
  {
    if (slaveFD != -1)
    {
      #ifdef TEST
      *logofs << "Loop: Closing slave listener in process "
              << "with pid '" << getpid() << "'.\n"
              << logofs_flush;
      #endif

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
    #ifdef TEST
    *logofs << "Loop: Closing proxy FD in process "
            << "with pid '" << getpid() << "'.\n"
            << logofs_flush;
    #endif

    close(proxyFD);

    proxyFD = -1;
  }

  if (agentFD[1] != -1)
  {
    #ifdef TEST
    *logofs << "Loop: Closing agent FD in process "
            << "with pid '" << getpid() << "'.\n"
            << logofs_flush;
    #endif

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

  *connectHost = '\0';
  *acceptHost  = '\0';
  *listenHost  = '\0';
  *displayHost = '\0';
  *authCookie  = '\0';

  proxyPort = DEFAULT_NX_PROXY_PORT;
  xPort     = DEFAULT_NX_X_PORT;

  xServerAddrFamily = -1;
  xServerAddrLength = 0;

  delete xServerAddr;

  xServerAddr = NULL;

  listenPort  = -1;
  connectPort = -1;

  cupsPort  = -1;
  auxPort   = -1;
  smbPort   = -1;
  mediaPort = -1;
  httpPort  = -1;
  slavePort = -1;

  *fontPort = '\0';

  *bindHost = '\0';
  bindPort = -1;

  initTs  = nullTimestamp();
  startTs = nullTimestamp();
  logsTs  = nullTimestamp();
  nowTs   = nullTimestamp();

  diffTs = 0;

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
    #ifdef TEST
    *logofs << "Loop: Aborting the procedure due to signal '"
            << lastSignal << "', '" << DumpSignal(lastSignal)
            << "'.\n" << logofs_flush;
    #endif

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

  #ifdef TEST
  *logofs << "Loop: Showing the proxy abort dialog.\n"
          << logofs_flush;
  #endif

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
    #if defined(TEST) || defined(INFO)
    *logofs << "Loop: Requesting an alert dialog with code "
            << code << " and local " << local << ".\n"
            << logofs_flush;
    #endif

    lastAlert.code  = code;
    lastAlert.local = local;
  }
  #if defined(TEST) || defined(INFO)
  else
  {
    *logofs << "Loop: WARNING! Alert dialog already requested "
            << "with code " << lastAlert.code << ".\n"
            << logofs_flush;
  }
  #endif

  return;
}

void FlushCallback(int length)
{
  if (flushCallback != NULL)
  {
    #if defined(TEST) || defined(INFO)
    *logofs << "Loop: Reporting a flush request at "
            << strMsTimestamp() << " with " << length
            << " bytes written.\n" << logofs_flush;
    #endif

    (*flushCallback)(flushParameter, length);
  }
  #if defined(TEST) || defined(INFO)
  else if (control -> ProxyMode == proxy_client)
  {
    *logofs << "Loop: WARNING! Can't find a flush "
            << "callback in process with pid '" << getpid()
            << "'.\n" << logofs_flush;
  }
  #endif
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
      #ifdef TEST
      *logofs << "Loop: Starting the house-keeping process with "
              << "image storage size " << control -> ImageCacheDiskLimit
              << ".\n" << logofs_flush;
      #endif

      lastKeeper = NXTransKeeper(0, control -> ImageCacheDiskLimit,
                                     control -> RootPath);

      if (IsFailed(lastKeeper))
      {
        #ifdef WARNING
        *logofs << "Loop: WARNING! Can't start the NX keeper process.\n"
                << logofs_flush;
        #endif

        SetNotRunning(lastKeeper);
      }
      #ifdef TEST
      else
      {
        *logofs << "Loop: Keeper started with pid '"
                << lastKeeper << "'.\n" << logofs_flush;
      }
      #endif
    }
    #ifdef TEST
    else
    {
      *logofs << "Loop: Nothing to do for the keeper process "
              << "with image cache not enabled.\n"
              << logofs_flush;
    }
    #endif
  }
  #ifdef TEST
  else
  {
    *logofs << "Loop: Nothing to do with the keeper process "
            << "already running.\n" << logofs_flush;
  }
  #endif
}

void InstallSignals()
{
  #ifdef TEST
  *logofs << "Loop: Installing signals in process with pid '"
          << getpid() << "'.\n" << logofs_flush;
  #endif

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
  #ifdef TEST
  *logofs << "Loop: Restoring signals in process with pid '"
          << getpid() << "'.\n" << logofs_flush;
  #endif

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
        #ifdef DUMP
        *logofs << "Loop: Disabling signal " << i << " '"
                << DumpSignal(i) << "' in process with pid '"
                << getpid() << "'.\n" << logofs_flush;
        #endif

        sigaddset(&newMask, i);
      }
    }

    sigprocmask(SIG_BLOCK, &newMask, &lastMasks.saved);

    lastMasks.blocked++;
  }
  #ifdef TEST
  else
  {
    *logofs << "Loop: WARNING! Signals were already blocked in "
            << "process with pid '" << getpid() << "'.\n"
            << logofs_flush;
  }
  #endif
}

void EnableSignals()
{
  if (lastMasks.blocked == 1)
  {
    #ifdef TEST
    *logofs << "Loop: Enabling signals in process with pid '"
            << getpid() << "'.\n" << logofs_flush;
    #endif

    sigprocmask(SIG_SETMASK, &lastMasks.saved, NULL);

    lastMasks.blocked = 0;
  }
  else
  {
    #ifdef WARNING
    *logofs << "Loop: WARNING! Signals were not blocked in "
            << "process with pid '" << getpid() << "'.\n"
            << logofs_flush;
    #endif

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
      #ifdef TEST
      *logofs << "Loop: Forwarding handler for signal " << signal
              << " '" << DumpSignal(signal) << "' in process "
              << "with pid '" << getpid() << "'.\n"
              << logofs_flush;
      #endif

      lastMasks.forward[signal] = 1;

      return;
    }
    #ifdef TEST
    else
    {
      *logofs << "Loop: Reinstalling handler for signal " << signal
              << " '" << DumpSignal(signal) << "' in process "
              << "with pid '" << getpid() << "'.\n"
              << logofs_flush;
    }
    #endif
  }
  #ifdef TEST
  else
  {
    *logofs << "Loop: Installing handler for signal " << signal
            << " '" << DumpSignal(signal) << "' in process "
            << "with pid '" << getpid() << "'.\n"
            << logofs_flush;
  }
  #endif

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
    #ifdef WARNING
    *logofs << "Loop: WARNING! Signal '" << DumpSignal(signal)
            << " not installed in process with pid '"
            << getpid() << "'.\n" << logofs_flush;
    #endif

    cerr << "Warning" << ": Signal '" << DumpSignal(signal)
         << " not installed in process with pid '"
         << getpid() << "'.\n";

    return;
  }

  #ifdef TEST
  *logofs << "Loop: Restoring handler for signal " << signal
          << " '" << DumpSignal(signal) << "' in process "
          << "with pid '" << getpid() << "'.\n"
          << logofs_flush;
  #endif

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

  #if defined(UNSAFE) && (defined(TEST) || defined(INFO))

  if (lastSignal != 0)
  {
    *logofs << "Loop: WARNING! Last signal is '" << lastSignal
            << "', '" << DumpSignal(signal) << "' and not zero "
            << "in process with pid '" << getpid() << "'.\n"
            << logofs_flush;
  }

  *logofs << "Loop: Signal '" << signal << "', '"
          << DumpSignal(signal) << "' received in process "
          << "with pid '" << getpid() << "'.\n" << logofs_flush;

  #endif

  if (getpid() != lastProxy && handler != NULL)
  {
    #if defined(UNSAFE) && (defined(TEST) || defined(INFO))
    *logofs << "Loop: Calling slave handler in process "
            << "with pid '" << getpid() << "'.\n"
            << logofs_flush;
    #endif

    if ((*handler)(signal) == 0)
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
      //        << "Closing agent conection.\n";
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

    #ifdef __CYGWIN32__

    case 12:
    {
      //
      // Nothing to do. This signal is what is delivered
      // by the Cygwin library when trying use a shared
      // memory function if the daemon is not running.
      //

      #ifdef TEST
      *logofs << "Loop: WARNING! Received signal '12' in "
              << "process with pid '" << getpid() << "'.\n"
              << logofs_flush;

      *logofs << "Loop: WARNING! Please check that the "
              << "cygserver daemon is running.\n"
              << logofs_flush;
      #endif

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
        #if defined(UNSAFE) && defined(TEST)
        *logofs << "Loop: Registering end of session request "
                << "due to signal '" << signal << "', '"
                << DumpSignal(signal) << "'.\n"
                << logofs_flush;
        #endif

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
      #if defined(UNSAFE) && defined(TEST)
      *logofs << "Loop: Forwarding signal '" << signal << "', '"
              << DumpSignal(signal) << "' to previous handler.\n"
              << logofs_flush;
      #endif

      lastMasks.action[signal].sa_handler(signal);
    }
    #ifdef WARNING
    else if (lastMasks.action[signal].sa_handler == NULL)
    {
      *logofs << "Loop: WARNING! Parent requested to forward "
              << "signal '" << signal << "', '" << DumpSignal(signal)
              << "' but didn't set a handler.\n" << logofs_flush;
    }
    #endif
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
    #if defined(UNSAFE) && defined(TEST)
    *logofs << "Loop: Resetting pid of last dialog process "
            << "in handler.\n" << logofs_flush;
    #endif

    SetNotRunning(lastDialog);

    if (proxy != NULL)
    {
      proxy -> handleResetAlert();
    }

    return 1;
  }

  if (IsRunning(lastWatchdog) && HandleChild(lastWatchdog) == 1)
  {
    #if defined(UNSAFE) && defined(TEST)
    *logofs << "Loop: Watchdog is gone. Setting the last "
            << "signal to SIGHUP.\n" << logofs_flush;
    #endif

    lastSignal = SIGHUP;

    #if defined(UNSAFE) && defined(TEST)
    *logofs << "Loop: Resetting pid of last watchdog process "
            << "in handler.\n" << logofs_flush;
    #endif

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
    #if defined(UNSAFE) && defined(TEST)
    *logofs << "Loop: Resetting pid of last house-keeping "
            << "process in handler.\n" << logofs_flush;
    #endif

    SetNotRunning(lastKeeper);

    return 1;
  }

  //
  // The pid will be checked by the code
  // that registered the child.
  //

  if (IsRunning(lastChild))
  {
    #if defined(UNSAFE) && defined(TEST)
    *logofs << "Loop: Resetting pid of last child process "
            << "in handler.\n" << logofs_flush;
    #endif

    SetNotRunning(lastChild);

    return 1;
  }

  //
  // This can actually happen either because we
  // reset the pid of the child process as soon
  // as we kill it, or because of a child process
  // of our parent.
  //

  #if defined(UNSAFE) && (defined(TEST) || defined(INFO))
  *logofs << "Loop: Ignoring signal received for the "
          << "unregistered child.\n" << logofs_flush;
  #endif

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
    #if defined(TEST) || defined(INFO)
    *logofs << "Loop: Waiting for the " << label
            << " process '" << child << "' to die.\n"
            << logofs_flush;
    #endif

    pid = waitpid(child, &status, options);

    if (pid == -1 && EGET() == EINTR)
    {
      if (force == 0)
      {
        return 0;
      }

      #ifdef WARNING
      *logofs << "Loop: WARNING! Ignoring signal while "
              << "waiting for the " << label << " process '"
              << child << "' to die.\n"
              << logofs_flush;
      #endif

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
      #if defined(UNSAFE) && defined(TEST)
      *logofs << "Loop: Child process '" << pid << "' was stopped "
              << "with signal " << (WSTOPSIG(status)) << ".\n"
              << logofs_flush;
      #endif

      return 0;
    }
    else
    {
      if (WIFEXITED(status))
      {
        #if defined(UNSAFE) && defined(TEST)
        *logofs << "Loop: Child process '" << pid << "' exited "
                << "with status '" << (WEXITSTATUS(status))
                << "'.\n" << logofs_flush;
        #endif

        lastStatus = WEXITSTATUS(status);
      }
      else if (WIFSIGNALED(status))
      {
        if (CheckSignal(WTERMSIG(status)) != 1)
        {
          #ifdef WARNING
          *logofs << "Loop: WARNING! Child process '" << pid
                  << "' died because of signal " << (WTERMSIG(status))
                  << ", '" << DumpSignal(WTERMSIG(status)) << "'.\n"
                  << logofs_flush;
          #endif

          cerr << "Warning" << ": Child process '" << pid
               << "' died because of signal " << (WTERMSIG(status))
               << ", '" << DumpSignal(WTERMSIG(status)) << "'.\n";
        }
        #if defined(UNSAFE) && defined(TEST)
        else
        {
          *logofs << "Loop: Child process '" << pid
                  << "' died because of signal " << (WTERMSIG(status))
                  << ", '" << DumpSignal(WTERMSIG(status)) << "'.\n"
                  << logofs_flush;
        }
        #endif

        lastStatus = 1;
      }

      return 1;
    }
  }
  else if (pid < 0)
  {
    if (EGET() != ECHILD)
    {
      #ifdef PANIC
      *logofs << "Loop: PANIC! Call to waitpid failed. "
              << "Error is " << EGET() << " '" << ESTR()
              << "'.\n" << logofs_flush;
      #endif

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

    #ifdef TEST
    *logofs << "Loop: No more children processes running.\n"
            << logofs_flush;
    #endif

    return 1;
  }

  return 0;
}

void RegisterChild(int child)
{
  #if defined(TEST) || defined(INFO)

  if (IsNotRunning(lastChild))
  {
    *logofs << "Loop: Registering child process '" << child
            << "' in process with pid '" << getpid()
            << "'.\n" << logofs_flush;
  }
  else
  {
    *logofs << "Loop: WARNING! Overriding registered child '"
            << lastChild << "' with new child '" << child
            << "' in process with pid '" << getpid()
            << "'.\n" << logofs_flush;
  }

  #endif

  lastChild = child;
}

int CheckParent(const char *name, const char *type, int parent)
{
  if (parent != getppid() || parent == 1)
  {
    #ifdef WARNING
    *logofs << name << ": WARNING! Parent process appears "
            << "to be dead. Exiting " << type << ".\n"
            << logofs_flush;
    #endif

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
      #if defined(UNSAFE) && defined(TEST)
      *logofs << "Loop: Timer expired at " << strMsTimestamp()
              << " in process with pid '" << getpid() << "'.\n"
              << logofs_flush;
      #endif

      if (proxy != NULL)
      {
        proxy -> handleTimer();
      }

      ResetTimer();
    }
    else
    {
      #ifdef PANIC
      *logofs << "Loop: PANIC! Inconsistent timer state "
              << " in process with pid '" << getpid() << "'.\n"
              << logofs_flush;
      #endif

      cerr << "Error" << ": Inconsistent timer state "
           << " in process with pid '" << getpid() << "'.\n";
    }
  }
  else
  {
    #ifdef PANIC
    *logofs << "Loop: PANIC! Inconsistent signal '"
            << signal << "', '" << DumpSignal(signal)
            << "' received in process with pid '"
            << getpid() << "'.\n" << logofs_flush;
    #endif

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
      #ifdef WARNING
      *logofs << "Loop: WARNING! Timer missed to expire at "
              << strMsTimestamp() << " in process with pid '"
              << getpid() << "'.\n" << logofs_flush;
      #endif

      cerr << "Warning" << ": Timer missed to expire at "
           << strMsTimestamp() << " in process with pid '"
           << getpid() << "'.\n";

      HandleTimer(SIGALRM);
    }
    else
    {
      #ifdef TEST
      *logofs << "Loop: Timer already running at "
              << strMsTimestamp() << " in process with pid '"
              << getpid() << "'.\n" << logofs_flush;
      #endif

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

  #ifdef TEST
  *logofs << "Loop: Timer set to " << lastTimer.next.tv_sec
          << " S and " << lastTimer.next.tv_usec / 1000
          << " Ms at " << strMsTimestamp() << " in process "
          << "with pid '" << getpid() << "'.\n"
          << logofs_flush;
  #endif

  if (setitimer(ITIMER_REAL, &timer, &lastTimer.value) < 0)
  {
    #ifdef PANIC
    *logofs << "Loop: PANIC! Call to setitimer failed. "
            << "Error is " << EGET() << " '" << ESTR()
            << "'.\n" << logofs_flush;
    #endif

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
    #if defined(UNSAFE) && defined(TEST)
    *logofs << "Loop: Timer not running in process "
            << "with pid '" << getpid() << "'.\n"
            << logofs_flush;
    #endif

    return;
  }

  #if defined(UNSAFE) && defined(TEST)
  *logofs << "Loop: Timer reset at " << strMsTimestamp()
          << " in process with pid '" << getpid()
          << "'.\n" << logofs_flush;
  #endif

  //
  // Restore the old signal mask and timer.
  //

  if (setitimer(ITIMER_REAL, &lastTimer.value, NULL) < 0)
  {
    #ifdef PANIC
    *logofs << "Loop: PANIC! Call to setitimer failed. "
            << "Error is " << EGET() << " '" << ESTR()
            << "'.\n" << logofs_flush;
    #endif

    cerr << "Error" << ": Call to setitimer failed. "
         << "Error is " << EGET() << " '" << ESTR()
         << "'.\n";
  }

  if (sigaction(SIGALRM, &lastTimer.action, NULL) < 0)
  {
    #ifdef PANIC
    *logofs << "Loop: PANIC! Call to sigaction failed. "
            << "Error is " << EGET() << " '" << ESTR()
            << "'.\n" << logofs_flush;
    #endif

    cerr << "Error" << ": Call to sigaction failed. "
         << "Error is " << EGET() << " '" << ESTR()
         << "'.\n";
  }

  lastTimer.start = lastTimer.next = nullTimestamp();
}

//
// Open TCP socket to listen for remote proxy and
// block until remote connects. If successful close
// the listening socket and return FD on which the
// other party is connected.
//

int WaitForRemote(int portNum)
{
  char hostLabel[DEFAULT_STRING_LENGTH] = { 0 };

  int retryAccept  = -1;
  int listenIPAddr = -1;

  int proxyFD = -1;
  int newFD   = -1;

  //
  // Get IP address of host to be awaited.
  //

  int acceptIPAddr = 0;

  if (*acceptHost != '\0')
  {
    acceptIPAddr = GetHostAddress(acceptHost);

    if (acceptIPAddr == 0)
    {
      #ifdef PANIC
      *logofs << "Loop: PANIC! Cannot accept connections from unknown host '"
              << acceptHost << "'.\n" << logofs_flush;
      #endif

      cerr << "Error" << ": Cannot accept connections from unknown host '"
           << acceptHost << "'.\n";

      goto WaitForRemoteError;
    }
  }

  proxyFD = socket(AF_INET, SOCK_STREAM, PF_UNSPEC);

  if (proxyFD == -1)
  {
    #ifdef PANIC
    *logofs << "Loop: PANIC! Call to socket failed for TCP socket. "
            << "Error is " << EGET() << " '" << ESTR() << "'.\n"
            << logofs_flush;
    #endif

    cerr << "Error" << ": Call to socket failed for TCP socket. "
         << "Error is " << EGET() << " '" << ESTR() << "'.\n";

    goto WaitForRemoteError;
  }
  else if (SetReuseAddress(proxyFD) < 0)
  {
    goto WaitForRemoteError;
  }

  listenIPAddr = 0;

  ParseListenOption(listenIPAddr);

  sockaddr_in tcpAddr;

  tcpAddr.sin_family = AF_INET;
  tcpAddr.sin_port = htons(portNum);

  //
  // Quick patch to run on MacOS/X where inet_addr("127.0.0.1")
  // alone seems to fail to return a valid interface. It probably
  // just needs a htonl() or something like that.
  // 
  // TODO: We have to give another look at inet_addr("127.0.0.1")
  // on the Mac.
  //

  #ifdef __APPLE__

  if ( loopbackBind )
  {
    tcpAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  }
  else
  {
    tcpAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  }

  #else

  tcpAddr.sin_addr.s_addr = listenIPAddr;

  #endif

  if (bind(proxyFD, (sockaddr *) &tcpAddr, sizeof(tcpAddr)) == -1)
  {
    #ifdef PANIC
    *logofs << "Loop: PANIC! Call to bind failed for TCP port "
            << portNum << ". Error is " << EGET() << " '" << ESTR()
            << "'.\n" << logofs_flush;
    #endif

    cerr << "Error" << ": Call to bind failed for TCP port "
         << portNum << ". Error is " << EGET() << " '" << ESTR()
         << "'.\n";

    goto WaitForRemoteError;
  }

  if (listen(proxyFD, 4) == -1)
  {
    #ifdef PANIC
    *logofs << "Loop: PANIC! Call to listen failed for TCP port "
            << portNum << ". Error is " << EGET() << " '" << ESTR()
            << "'.\n" << logofs_flush;
    #endif

    cerr << "Error" << ": Call to listen failed for TCP port "
         << portNum << ". Error is " << EGET() << " '" << ESTR()
         << "'.\n";

    goto WaitForRemoteError;
  }

  if (*acceptHost != '\0')
  {
    strcat(hostLabel, "'");
    strcat(hostLabel, acceptHost);
    strcat(hostLabel, "'");
  }
  else
  {
    strcpy(hostLabel, "any host");
  }

  #ifdef TEST
  *logofs << "Loop: Waiting for connection from "
          << hostLabel  << " on port '" << portNum
          << "'.\n" << logofs_flush;
  #endif

  cerr << "Info" << ": Waiting for connection from "
       << hostLabel << " on port '" << portNum
       << "'.\n";

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
    FD_SET(proxyFD, &readSet);

    T_timestamp selectTs;

    selectTs.tv_sec  = 20;
    selectTs.tv_usec = 0;

    int result = select(proxyFD + 1, &readSet, NULL, NULL, &selectTs);

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
 
      #ifdef PANIC
      *logofs << "Loop: PANIC! Call to select failed. Error is "
              << EGET() << " '" << ESTR() << "'.\n"
              << logofs_flush;
      #endif

      cerr << "Error" << ": Call to select failed. Error is "
           << EGET() << " '" << ESTR() << "'.\n";

      goto WaitForRemoteError;
    }
    else if (result > 0 && FD_ISSET(proxyFD, &readSet))
    {
      sockaddr_in newAddr;

      size_t addrLen = sizeof(sockaddr_in);

      newFD = accept(proxyFD, (sockaddr *) &newAddr, (socklen_t *) &addrLen);

      if (newFD == -1)
      {
        #ifdef PANIC
        *logofs << "Loop: PANIC! Call to accept failed. Error is "
                << EGET() << " '" << ESTR() << "'.\n"
                << logofs_flush;
        #endif

        cerr << "Error" << ": Call to accept failed. Error is "
             << EGET() << " '" << ESTR() << "'.\n";

        goto WaitForRemoteError;
      }

      char *connectedHost = inet_ntoa(newAddr.sin_addr);

      if (*acceptHost == '\0' || (int) newAddr.sin_addr.s_addr == acceptIPAddr)
      {
        #ifdef TEST

        unsigned int connectedPort = ntohs(newAddr.sin_port);

        *logofs << "Loop: Accepted connection from '" << connectedHost
                << "' with port '" << connectedPort << "'.\n"
                << logofs_flush;
        #endif

        cerr << "Info" << ": Accepted connection from '"
             << connectedHost << "'.\n";

        break;
      }
      else
      {
        #ifdef PANIC
        *logofs << "Loop: WARNING! Refusing connection from '" << connectedHost
                << "' on port '" << portNum << "'.\n" << logofs_flush;
        #endif

        cerr << "Warning" << ": Refusing connection from '"
             << connectedHost << "'.\n";
      }

      //
      // Not the best way to elude a DOS attack...
      //

      sleep(5);

      close(newFD);
    }

    if (--retryAccept == 0)
    {
      if (*acceptHost == '\0')
      {
        #ifdef PANIC
        *logofs << "Loop: PANIC! Connection with remote host "
                << "could not be established.\n"
                << logofs_flush;
        #endif

        cerr << "Error" << ": Connection with remote host "
             << "could not be established.\n";
      }
      else
      {
        #ifdef PANIC
        *logofs << "Loop: PANIC! Connection with remote host '"
                << acceptHost << "' could not be established.\n"
                << logofs_flush;
        #endif

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

  close(proxyFD);

  return newFD;

WaitForRemoteError:

  close(proxyFD);

  HandleCleanup();
}

//
// Connect to remote proxy. If successful
// return FD of connection, else return -1.
//

int ConnectToRemote(const char *const hostName, int portNum)
{
  int proxyFD = -1;

  int remoteIPAddr = GetHostAddress(hostName);

  if (remoteIPAddr == 0)
  {
    #ifdef PANIC
    *logofs << "Loop: PANIC! Unknown remote host '"
            << hostName << "'.\n" << logofs_flush;
    #endif

    cerr << "Error" << ": Unknown remote host '"
         << hostName << "'.\n";

    HandleCleanup();
  }

  #ifdef TEST
  *logofs << "Loop: Connecting to remote host '" 
          << hostName << ":" << portNum << "'.\n"
          << logofs_flush;
  #endif

  cerr << "Info" << ": Connecting to remote host '"
       << hostName << ":" << portNum << "'.\n"
       << logofs_flush;

  //
  // How many times we retry to connect to remote
  // host in case of failure?
  //

  int retryConnect = control -> OptionProxyRetryConnect;

  //
  // Show an alert after 20 seconds and use the
  // same timeout to interrupt the connect. The
  // retry timeout is incremental, starting from
  // 100 miliseconds up to 1 second.
  //

  int alertTimeout   = 20000;
  int connectTimeout = 20000;
  int retryTimeout   = 100;

  T_timestamp lastRetry = getNewTimestamp();

  sockaddr_in addr;

  addr.sin_family = AF_INET;
  addr.sin_port = htons(portNum);
  addr.sin_addr.s_addr = remoteIPAddr;

  for (;;)
  {
    proxyFD = socket(AF_INET, SOCK_STREAM, PF_UNSPEC);

    if (proxyFD == -1)
    {
      #ifdef PANIC
      *logofs << "Loop: PANIC! Call to socket failed. "
              << "Error is " << EGET() << " '" << ESTR()
              << "'.\n" << logofs_flush;
      #endif

      cerr << "Error" << ": Call to socket failed. "
           << "Error is " << EGET() << " '" << ESTR()
           << "'.\n";

      goto ConnectToRemoteError;
    }
    else if (SetReuseAddress(proxyFD) < 0)
    {
      goto ConnectToRemoteError;
    }

    //
    // Ensure operation is timed out
    // if there is a network problem.
    //

    #ifdef DEBUG
    *logofs << "Loop: Timer set to " << connectTimeout / 1000
            << " S " << "with retry set to " << retryConnect
            << " in process with pid '" << getpid()
            << "'.\n" << logofs_flush;
    #endif

    SetTimer(connectTimeout);

    int result = connect(proxyFD, (sockaddr *) &addr, sizeof(sockaddr_in));

    int reason = EGET();

    ResetTimer();

    if (result < 0)
    {
      close(proxyFD);

      if (CheckAbort() != 0)
      {
        goto ConnectToRemoteError;
      }
      else if (--retryConnect == 0)
      {
        ESET(reason);

        #ifdef PANIC
        *logofs << "Loop: PANIC! Connection to '" << hostName
                << ":" << portNum << "' failed. Error is "
                << EGET() << " '" << ESTR() << "'.\n"
                << logofs_flush;
        #endif

        cerr << "Error" << ": Connection to '" << hostName
             << ":" << portNum << "' failed. Error is "
             << EGET() << " '" << ESTR() << "'.\n";

        goto ConnectToRemoteError;
      }
      else
      {
        #ifdef TEST
        *logofs << "Loop: Sleeping " << retryTimeout 
                << " Ms before retrying.\n"
                << logofs_flush;
        #endif

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

              #ifdef __CYGWIN32__

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
      #ifdef TEST
      {
        *logofs << "Loop: Not showing the dialog with "
                << (diffTimestamp(lastRetry, getTimestamp()) / 1000)
                << " seconds elapsed.\n" << logofs_flush;
      }
      #endif

      ESET(reason);

      #ifdef TEST
      *logofs << "Loop: Connection to '" << hostName
              << ":" << portNum << "' failed with error '"
              << ESTR() << "'. Retrying.\n"
              << logofs_flush;
      #endif
    }
    else
    {
      //
      // Connection was successful.
      //

      break;
    }
  }

  return proxyFD;

ConnectToRemoteError:

  if (proxyFD != -1)
  {
    close(proxyFD);
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

  sprintf(options, "NXPROXY-1.5.0-%i.%i.%i", control -> LocalVersionMajor,
              control -> LocalVersionMinor, control -> LocalVersionPatch);

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

    if (control -> isProtoStep7() == 1 &&
            useStrict != -1)
    {
      sprintf(options + strlen(options), "strict=%d,", useStrict);
    }

    //
    // Tell the remote the size of the shared
    // memory segment.
    //

    if (control -> isProtoStep7() == 1 &&
            *shsegSizeName != '\0')
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

  #ifdef TEST
  *logofs << "Loop: Sending remote options '"
          << options << "'.\n" << logofs_flush;
  #endif

  return WriteLocalData(fd, options, strlen(options));
}

int ReadProxyVersion(int fd)
{
  #ifdef TEST
  *logofs << "Loop: Going to read the remote proxy version "
          << "from FD#" << fd << ".\n" << logofs_flush;
  #endif

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

  #ifdef TEST
  *logofs << "Loop: Received remote version string '"
          << options << "' from FD#" << fd << ".\n"
          << logofs_flush;
  #endif

  if (strncmp(options, "NXPROXY-", strlen("NXPROXY-")) != 0)
  {
    #ifdef PANIC
    *logofs << "Loop: PANIC! Parse error in remote options string '"
            << options << "'.\n" << logofs_flush;
    #endif

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
    #ifdef TEST
    *logofs << "Loop: Read trailing remote version '" << major
            << "." << minor << "." << patch << "'.\n"
            << logofs_flush;
    #endif

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

  #ifdef TEST
  *logofs << "Loop: Identified remote version '" << control -> RemoteVersionMajor
          << "." << control -> RemoteVersionMinor << "." << control -> RemoteVersionPatch
          << "'.\n" << logofs_flush;

  *logofs << "Loop: Remote compatibility version '" << control -> CompatVersionMajor
          << "." << control -> CompatVersionMinor << "." << control -> CompatVersionPatch
          << "'.\n" << logofs_flush;

  *logofs << "Loop: Local version '" << control -> LocalVersionMajor
          << "." << control -> LocalVersionMinor << "." << control -> LocalVersionPatch
          << "'.\n" << logofs_flush;
  #endif

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
  #ifdef TEST
  *logofs << "Loop: Going to read the remote proxy options "
          << "from FD#" << fd << ".\n" << logofs_flush;
  #endif

  char options[DEFAULT_REMOTE_OPTIONS_LENGTH];

  int result = ReadRemoteData(fd, options, sizeof(options), ' ');

  if (result <= 0)
  {
    return result;
  }

  #ifdef TEST
  *logofs << "Loop: Received remote options string '"
          << options << "' from FD#" << fd << ".\n"
          << logofs_flush;
  #endif

  //
  // Get the remote options, delimited by a space character.
  // Note that there will be a further initialization phase
  // at the time proxies negotiate cache file to restore.
  //

  if (ParseRemoteOptions(options) < 0)
  {
    #ifdef PANIC
    *logofs << "Loop: PANIC! Couldn't negotiate a valid "
            << "session with remote NX proxy.\n"
            << logofs_flush;
    #endif

    cerr << "Error" << ": Couldn't negotiate a valid "
         << "session with remote NX proxy.\n";

    return -1;
  }

  return 1;
}

int SendProxyCaches(int fd)
{
  #ifdef TEST
  *logofs << "Loop: Synchronizing local and remote caches.\n"
          << logofs_flush;
  #endif

  if (control -> ProxyMode == proxy_client)
  {
    //
    // Prepare a list of caches matching this
    // session type and send it to the remote.
    //

    #ifdef TEST
    *logofs << "Loop: Going to send the list of local caches.\n"
            << logofs_flush;
    #endif

    SetCaches();

    int entries = DEFAULT_REMOTE_CACHE_ENTRIES;

    const char prefix = 'C';

    if (control -> LocalDeltaCompression == 0 ||
            control -> PersistentCacheEnableLoad == 0)
    {
      #ifdef TEST
      *logofs << "Loop: Writing an empty list to FD#" << fd
              << ".\n" << logofs_flush;
      #endif

      return WriteLocalData(fd, "cachelist=none ", strlen("cachelist=none "));
    }

    int count = 0;

    #ifdef TEST
    *logofs << "Loop: Looking for cache files in directory '"
            << control -> PersistentCachePath << "'.\n" << logofs_flush;
    #endif

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

          #ifdef TEST
          *logofs << "Loop: Writing entry '" << control -> PersistentCachePath
                  << "/" << dirEntry -> d_name << "' to FD#" << fd
                  << ".\n" << logofs_flush;
          #endif

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
      #ifdef TEST
      *logofs << "Loop: Writing an empty list to FD#" << fd
              << ".\n" << logofs_flush;
      #endif

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

    #ifdef TEST
    *logofs << "Loop: Going to send the selected cache.\n"
            << logofs_flush;
    #endif

    char buffer[DEFAULT_STRING_LENGTH];

    if (control -> PersistentCacheName != NULL)
    {
      #ifdef TEST
      *logofs << "Loop: Name of selected cache file is '"
              << control -> PersistentCacheName << "'.\n"
              << logofs_flush;
      #endif

      sprintf(buffer, "cachefile=%s%s ",
                  *(control -> PersistentCacheName) == 'C' ? "S-" : "C-",
                      control -> PersistentCacheName + 2);
    }
    else
    {
      #ifdef TEST
      *logofs << "Loop: No valid cache file was selected.\n"
              << logofs_flush;
      #endif

      sprintf(buffer, "cachefile=none ");
    }

    #ifdef TEST
    *logofs << "Loop: Sending string '" << buffer
            << "' as selected cache file.\n"
            << logofs_flush;
    #endif

    return WriteLocalData(fd, buffer, strlen(buffer));
  }
}

int ReadProxyCaches(int fd)
{
  if (control -> ProxyMode == proxy_client)
  {
    #ifdef TEST
    *logofs << "Loop: Going to receive the selected proxy cache.\n"
            << logofs_flush;
    #endif

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
      #ifdef PANIC
      *logofs << "Loop: PANIC! Invalid cache file option '"
              << buffer << "' provided by remote proxy.\n"
              << logofs_flush;
      #endif

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
      #ifdef TEST
      *logofs << "Loop: No cache file selected by remote proxy.\n"
              << logofs_flush;
      #endif
    }
    else if (strlen(cacheName) != MD5_LENGTH * 2 + 3 ||
                 *(cacheName + MD5_LENGTH * 2 + 2) != ' ')
    {
      #ifdef PANIC
      *logofs << "Loop: PANIC! Invalid cache file name '"
              << cacheName << "' provided by remote proxy.\n"
              << logofs_flush;
      #endif

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

      #ifdef TEST
      *logofs << "Loop: Cache file '" << control -> PersistentCacheName
              << "' selected by remote proxy.\n" << logofs_flush;
      #endif
    }
  }
  else
  {
    #ifdef TEST
    *logofs << "Loop: Going to receive the list of remote caches.\n"
            << logofs_flush;
    #endif

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

    #ifdef TEST
    *logofs << "Loop: Read list of caches from remote side as '"
            << buffer << "'.\n" << logofs_flush;
    #endif

    //
    // Prepare the buffer. What we want is a list
    // like "cache1,cache2,cache2" terminated by
    // null.
    //

    *(buffer + strlen(buffer) - 1) = '\0';

    if (strncasecmp(buffer, "cachelist=", strlen("cachelist=")) != 0)
    {
      #ifdef PANIC
      *logofs << "Loop: Wrong format for list of cache files "
              << "read from FD#" << fd << ".\n" << logofs_flush;
      #endif

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
  #ifdef TEST
  *logofs << "Loop: Going to negotiate the forwarder version.\n"
          << logofs_flush;
  #endif

  //
  // Check if we actually expect the session cookie.
  //

  if (*authCookie == '\0')
  {
    #ifdef TEST
    *logofs << "Loop: No authentication cookie required "
            << "from FD#" << fd << ".\n" << logofs_flush;
    #endif

    return 1;
  }

  char options[DEFAULT_REMOTE_OPTIONS_LENGTH];

  int result = ReadRemoteData(fd, options, sizeof(options), ' ');

  if (result <= 0)
  {
    return result;
  }

  #ifdef TEST
  *logofs << "Loop: Received forwarder version string '" << options
          << "' from FD#" << fd << ".\n" << logofs_flush;
  #endif

  if (strncmp(options, "NXSSH-", strlen("NXSSH-")) != 0)
  {
    #ifdef PANIC
    *logofs << "Loop: PANIC! Parse error in forwarder options string '"
            << options << "'.\n" << logofs_flush;
    #endif

    cerr << "Error" << ": Parse error in forwarder options string '"
         << options << "'.\n";

    return -1;
  }

  //
  // Accept whatever forwarder version.
  //

  sscanf(options, "NXSSH-%i.%i.%i", &(control -> RemoteVersionMajor),
             &(control -> RemoteVersionMinor), &(control -> RemoteVersionPatch));

  #ifdef TEST
  *logofs << "Loop: Read forwarder version '" << control -> RemoteVersionMajor
          << "." << control -> RemoteVersionMinor << "." << control -> RemoteVersionPatch
          << "'.\n" << logofs_flush;
  #endif

  return 1;
}

int ReadForwarderOptions(int fd)
{
  //
  // Get the forwarder cookie.
  //

  if (*authCookie == '\0')
  {
    #ifdef TEST
    *logofs << "Loop: No authentication cookie required "
            << "from FD#" << fd << ".\n" << logofs_flush;
    #endif

    return 1;
  }

  char options[DEFAULT_REMOTE_OPTIONS_LENGTH];

  int result = ReadRemoteData(fd, options, sizeof(options), ' ');

  if (result <= 0)
  {
    return result;
  }

  #ifdef TEST
  *logofs << "Loop: Received forwarder options string '"
          << options << "' from FD#" << fd << ".\n"
          << logofs_flush;
  #endif

  if (ParseForwarderOptions(options) < 0)
  {
    #ifdef PANIC
    *logofs << "Loop: PANIC! Couldn't negotiate a valid "
            << "cookie with the NX forwarder.\n"
            << logofs_flush;
    #endif

    cerr << "Error" << ": Couldn't negotiate a valid "
         << "cookie with the NX forwarder.\n";

    return -1;
  }

  return 1;
}

int ReadRemoteData(int fd, char *buffer, int size, char stop)
{
  #ifdef TEST
  *logofs << "Loop: Going to read remote data from FD#"
          << fd << ".\n" << logofs_flush;
  #endif

  if (size >= MAXIMUM_REMOTE_OPTIONS_LENGTH)
  {
    #ifdef PANIC
    *logofs << "Loop: PANIC! Maximum remote options buffer "
            << "limit exceeded.\n" << logofs_flush;
    #endif

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
          #ifdef TEST
          *logofs << "Loop: Reading data from FD#" << fd
                  << " would block.\n" << logofs_flush;
          #endif

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

      #ifdef PANIC
      *logofs << "Loop: PANIC! The remote NX proxy closed "
              << "the connection.\n" << logofs_flush;
      #endif

      cerr << "Error" << ": The remote NX proxy closed "
           << "the connection.\n";

      return -1;
    }
    else if (*(remoteData + remotePosition) == stop)
    {
      #ifdef TEST
      *logofs << "Loop: Read stop character from FD#"
              << fd << ".\n" << logofs_flush;
      #endif

      remotePosition++;

      //
      // Copy the fake terminating null
      // in the buffer.
      //

      *(remoteData + remotePosition) = '\0';

      memcpy(buffer, remoteData, remotePosition + 1);

      #ifdef TEST
      *logofs << "Loop: Remote string '" << remoteData
              << "' read from FD#" << fd << ".\n"
              << logofs_flush;
      #endif

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
        #ifdef WARNING
        *logofs << "Loop: WARNING! Non printable character decimal '"
                << (unsigned int) *(remoteData + remotePosition)
                << "' received in remote data from FD#"
                << fd << ".\n" << logofs_flush;
        #endif

        cerr << "Warning" << ": Non printable character decimal '"
                << (unsigned int) *(remoteData + remotePosition)
                << "' received in remote data from FD#"
                << fd << ".\n" << logofs_flush;

        *(remoteData + remotePosition) = ' ';
      }

      #ifdef DEBUG
      *logofs << "Loop: Read a further character "
              << "from FD#" << fd << ".\n"
              << logofs_flush;
      #endif

      remotePosition++;
    }
  }

  *(remoteData + remotePosition) = '\0';

  #ifdef PANIC
  *logofs << "Loop: PANIC! Stop character missing "
          << "from FD#" << fd << " after " << remotePosition 
          << " characters read in string '" << remoteData
          << "'.\n" << logofs_flush;
  #endif

  cerr << "Error" << ": Stop character missing "
       << "from FD#" << fd << " after " << remotePosition 
       << " characters read in string '" << remoteData
       << "'.\n";

  memcpy(buffer, remoteData, remotePosition);

  remotePosition = 0;

  return -1;
}

int WriteLocalData(int fd, const char *buffer, int size)
{
  int position = 0;

  while (position < size)
  {
    int result = write(fd, buffer + position, size - position);

    getNewTimestamp();

    if (result <= 0)
    {
      if (result < 0 && EGET() == EINTR)
      {
        continue;
      }

      #ifdef TEST
      *logofs << "Loop: Error writing data to FD#"
              << fd << ".\n" << logofs_flush;
      #endif

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
    #ifdef TEST
    *logofs << "Loop: Out of the long jump while parsing "
            << "the environment options.\n"
            << logofs_flush;
    #endif
    
    return -1;
  }

  if (force == 0 && parsedOptions == 1)
  {
    #ifdef TEST
    *logofs << "Loop: Skipping a further parse of environment "
            << "options string '" << (env != NULL ? env : "")
            << "'.\n" << logofs_flush;
    #endif

    return 1;
  }

  if (env == NULL || *env == '\0')
  {
    #ifdef TEST
    *logofs << "Loop: Nothing to do with empty environment "
            << "options string '" << (env != NULL ? env : "")
            << "'.\n" << logofs_flush;
    #endif

    return 0;
  }

  #ifdef TEST
  *logofs << "Loop: Going to parse the environment options "
          << "string '" << env << "'.\n"
          << logofs_flush;
  #endif

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
    #ifdef PANIC
    *logofs << "Loop: PANIC! Environment options string '" << env
            << "' exceeds length of " << DEFAULT_DISPLAY_OPTIONS_LENGTH
            << " characters.\n" << logofs_flush;
    #endif

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
    #ifdef PANIC
    *logofs << "Loop: PANIC! Parse error in options string '"
            << opts << "' at 'nx,:'.\n" << logofs_flush;
    #endif

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
    #ifdef TEST
    *logofs << "Loop: Ignoring host X server display string '"
            << opts << "'.\n" << logofs_flush;
    #endif

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

  value = rindex(nextOpts, ':');

  if (value != NULL)
  {
    char *check = value + 1;

    if (*check == '\0' || isdigit(*check) == 0)
    {
      #ifdef PANIC
      *logofs << "Loop: PANIC! Can't identify NX port in string '"
              << value << "'.\n" << logofs_flush;
      #endif

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

    #ifdef PANIC
    *logofs << "Loop: PANIC! Can't identify NX port in string '"
            << opts << "'.\n" << logofs_flush;
    #endif

    cerr << "Error" << ": Can't identify NX port in string '"
         << opts << "'.\n";

    return -1;
  }

  #ifdef TEST
  *logofs << "Loop: Parsing options string '"
          << nextOpts << "'.\n" << logofs_flush;
  #endif

  //
  // Now all the other optional parameters.
  //

  name = strtok(nextOpts, "=");

  while (name)
  {
    value = strtok(NULL, ",");

    if (CheckArg("environment", name, value) < 0)
    {
      return -1;
    }

    if (strcasecmp(name, "options") == 0)
    {
      strncpy(fileOptions, value, DEFAULT_STRING_LENGTH - 1);
    }
    else if (strcasecmp(name, "display") == 0)
    {
      strncpy(displayHost, value, DEFAULT_STRING_LENGTH - 1);
    }
    else if (strcasecmp(name, "link") == 0)
    {
      if (control -> ProxyMode == proxy_server)
      {
        PrintOptionIgnored("local", name, value);
      }
      else if (ParseLinkOption(value) < 0)
      {
        #ifdef PANIC
        *logofs << "Loop: PANIC! Can't identify 'link' option in string '"
                << value << "'.\n" << logofs_flush;
        #endif

        cerr << "Error" << ": Can't identify 'link' option in string '"
             << value << "'.\n";

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
        #ifdef PANIC
        *logofs << "Loop: PANIC! Can't identify option 'limit' in string '"
                << value << "'.\n" << logofs_flush;
        #endif

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
          strncpy(sessionType, value, DEFAULT_STRING_LENGTH - 1);
        }
      }
    }
    else if (strcasecmp(name, "listen") == 0)
    {
      if (*connectHost != '\0')
      {
        #ifdef PANIC
        *logofs << "Loop: PANIC! Can't handle 'listen' and 'connect' parameters "
                << "at the same time.\n" << logofs_flush;

        *logofs << "Loop: PANIC! Refusing 'listen' parameter with 'connect' being '"
                << connectHost << "'.\n" << logofs_flush;
        #endif

        cerr << "Error" << ": Can't handle 'listen' and 'connect' parameters "
             << "at the same time.\n";

        cerr << "Error" << ": Refusing 'listen' parameter with 'connect' being '"
             << connectHost << "'.\n";

        return -1;
      }

      listenPort = ValidateArg("local", name, value);
    }
    else if (strcasecmp(name, "loopback") == 0)
    {
      loopbackBind = ValidateArg("local", name, value);
    }
    else if (strcasecmp(name, "accept") == 0)
    {
      if (*connectHost != '\0')
      {
        #ifdef PANIC
        *logofs << "Loop: PANIC! Can't handle 'accept' and 'connect' parameters "
                << "at the same time.\n" << logofs_flush;

        *logofs << "Loop: PANIC! Refusing 'accept' parameter with 'connect' being '"
                << connectHost << "'.\n" << logofs_flush;
        #endif

        cerr << "Error" << ": Can't handle 'accept' and 'connect' parameters "
             << "at the same time.\n";

        cerr << "Error" << ": Refusing 'accept' parameter with 'connect' being '"
             << connectHost << "'.\n";

        return -1;
      }

      strncpy(acceptHost, value, DEFAULT_STRING_LENGTH - 1);
    }
    else if (strcasecmp(name, "connect") == 0)
    {
      if (*acceptHost != '\0')
      {
        #ifdef PANIC
        *logofs << "Loop: PANIC! Can't handle 'connect' and 'accept' parameters "
                << "at the same time.\n" << logofs_flush;

        *logofs << "Loop: PANIC! Refusing 'connect' parameter with 'accept' being '"
                << acceptHost << "'.\n" << logofs_flush;
        #endif

        cerr << "Error" << ": Can't handle 'connect' and 'accept' parameters "
             << "at the same time.\n";

        cerr << "Error" << ": Refusing 'connect' parameter with 'accept' being '"
             << acceptHost << "'.\n";

        return -1;
      }

      strncpy(connectHost, value, DEFAULT_STRING_LENGTH - 1);
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
      strncpy(sessionFileName, value, DEFAULT_STRING_LENGTH - 1);
    }
    else if (strcasecmp(name, "errors") == 0)
    {
      //
      // The old name of the parameter was 'log'
      // but the default name for the file is
      // 'errors' so it is more logical to use
      // the same name.
      //

      strncpy(errorsFileName, value, DEFAULT_STRING_LENGTH - 1);
    }
    else if (strcasecmp(name, "root") == 0)
    {
      strncpy(rootDir, value, DEFAULT_STRING_LENGTH - 1);
    }
    else if (strcasecmp(name, "id") == 0)
    {
      strncpy(sessionId, value, DEFAULT_STRING_LENGTH - 1);
    }
    else if (strcasecmp(name, "stats") == 0)
    {
      control -> EnableStatistics = 1;

      strncpy(statsFileName, value, DEFAULT_STRING_LENGTH - 1);
    }
    else if (strcasecmp(name, "cookie") == 0)
    {
      LowercaseArg("local", name, value);

      strncpy(authCookie, value, DEFAULT_STRING_LENGTH - 1);
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
        #ifdef PANIC
        *logofs << "Loop: PANIC! Can't identify cache size for string '"
                << value << "'.\n" << logofs_flush;
        #endif

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
        #ifdef PANIC
        *logofs << "Loop: PANIC! Can't identify images cache size for string '"
                << value << "'.\n" << logofs_flush;
        #endif

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
        #ifdef PANIC
        *logofs << "Loop: PANIC! Can't identify size of shared memory "
                << "segment in string '" << value << "'.\n"
                << logofs_flush;
        #endif

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
      cupsPort = ValidateArg("local", name, value);
    }
    else if (strcasecmp(name, "sync") == 0)
    {
      #ifdef WARNING
      *logofs << "Loop: WARNING! No 'sync' channel in current version. "
              << "Assuming 'cups' channel.\n" << logofs_flush;
      #endif

      cerr << "Warning" << ": No 'sync' channel in current version. "
           << "Assuming 'cups' channel.\n";

      cupsPort = ValidateArg("local", name, value);
    }
    else if (strcasecmp(name, "keybd") == 0 ||
                 strcasecmp(name, "aux") == 0)
    {
      auxPort = ValidateArg("local", name, value);
    }
    else if (strcasecmp(name, "samba") == 0 ||
                 strcasecmp(name, "smb") == 0)
    {
      smbPort = ValidateArg("local", name, value);
    }
    else if (strcasecmp(name, "media") == 0)
    {
      mediaPort = ValidateArg("local", name, value);
    }
    else if (strcasecmp(name, "http") == 0)
    {
      httpPort = ValidateArg("local", name, value);
    }
    else if (strcasecmp(name, "font") == 0)
    {
      strncpy(fontPort, value, DEFAULT_STRING_LENGTH - 1);
    }
    else if (strcasecmp(name, "slave") == 0)
    {
      slavePort = ValidateArg("local", name, value);
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
        #ifdef TEST
        *logofs << "Loop: Disabling timeout on broken "
                << "proxy connection.\n" << logofs_flush;
        #endif

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
        #ifdef TEST
        *logofs << "Loop: Disabling grace timeout on "
                << "proxy shutdown.\n" << logofs_flush;
        #endif

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
        #ifdef PANIC
        *logofs << "Loop: PANIC! Can't identify pack method for string '"
                << value << "'.\n" << logofs_flush;
        #endif

        cerr << "Error" << ": Can't identify pack method for string '"
             << value << "'.\n";

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
        #ifdef TEST
        *logofs << "Loop: WARNING! Adding process with pid '"
                << ValidateArg("local", name, value) << " to the "
                << "daemons to kill at shutdown.\n"
                << logofs_flush;
        #endif

        control -> KillDaemonOnShutdown[control ->
                       KillDaemonOnShutdownNumber] =
                           ValidateArg("local", name, value);

        control -> KillDaemonOnShutdownNumber++;
      }
      else
      {
        #ifdef WARNING
        *logofs << "Loop: WARNING! Number of daemons to kill "
                << "at shutdown exceeded.\n" << logofs_flush;
        #endif

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
      strncpy(productName, value, DEFAULT_STRING_LENGTH - 1);
    }
    else if (strcasecmp(name, "rootless") == 0 ||
                 strcasecmp(name, "geometry") == 0 ||
                     strcasecmp(name, "resize") == 0 ||
                         strcasecmp(name, "fullscreen") == 0 ||
                             strcasecmp(name, "keyboard") == 0 ||
                                 strcasecmp(name, "clipboard") == 0 ||
                                     strcasecmp(name, "streaming") == 0 ||
                                         strcasecmp(name, "backingstore") == 0)
    {
      #ifdef DEBUG
      *logofs << "Loop: Ignoring agent option '" << name
              << "' with value '" << value << "'.\n"
              << logofs_flush;
      #endif
    }
    else if (strcasecmp(name, "composite") == 0 ||
                 strcasecmp(name, "shmem") == 0 ||
                     strcasecmp(name, "shpix") == 0 ||
                         strcasecmp(name, "kbtype") == 0 ||
                             strcasecmp(name, "client") == 0 ||
                                 strcasecmp(name, "shadow") == 0 ||
                                     strcasecmp(name, "shadowuid") == 0 ||
                                         strcasecmp(name, "shadowmode") == 0 ||
                                             strcasecmp(name, "clients") == 0)
    {
      #ifdef DEBUG
      *logofs << "Loop: Ignoring agent option '" << name
              << "' with value '" << value << "'.\n"
              << logofs_flush;
      #endif
    }
    else if (strcasecmp(name, "defer") == 0 ||
                 strcasecmp(name, "tile") == 0 ||
                     strcasecmp(name, "menu") == 0)
    {
      #ifdef DEBUG
      *logofs << "Loop: Ignoring agent option '" << name
              << "' with value '" << value << "'.\n"
              << logofs_flush;
      #endif
    }
    else
    {
      #ifdef WARNING
      *logofs << "Loop: WARNING! Ignoring unknown option '"
              << name << "' with value '" << value << "'.\n"
              << logofs_flush;
      #endif

      cerr << "Warning" << ": Ignoring unknown option '"
           << name << "' with value '" << value << "'.\n";
    }

    name = strtok(NULL, "=");

  } // End of while (name) ...

  #ifdef TEST
  *logofs << "Loop: Completed parsing of string '"
          << env << "'.\n" << logofs_flush;
  #endif

  if (*fileOptions != '\0')
  {
    if (strcmp(fileOptions, optionsFileName) != 0)
    {
      #ifdef TEST
      *logofs << "Loop: Reading options from '" << fileOptions
              << "'.\n" << logofs_flush;
      #endif

      if (ParseFileOptions(fileOptions) < 0)
      {
        return -1;
      }
    }
    #ifdef WARNING
    else
    {
      *logofs << "Loop: WARNING! Name of the options file "
              << "specified multiple times. Not parsing "
              << "again.\n" << logofs_flush;
    }
    #endif

    if (*optionsFileName == '\0')
    {
      strncpy(optionsFileName, value, DEFAULT_STRING_LENGTH - 1);

      #ifdef TEST
      *logofs << "Loop: Assuming name of options file '"
              << optionsFileName << "'.\n"
              << logofs_flush;
      #endif
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
    #ifdef TEST
    *logofs << "Loop: Out of the long jump while parsing "
            << "the command line options.\n"
            << logofs_flush;
    #endif
    
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
    #ifdef TEST
    *logofs << "Loop: Skipping a further parse of command line options.\n"
            << logofs_flush;
    #endif

    return 1;
  }

  #ifdef TEST
  *logofs << "Loop: Going to parse the command line options.\n"
          << logofs_flush;
  #endif

  parsedCommand = 1;

  //
  // Print out arguments.
  //

  #ifdef TEST

  *logofs << "Loop: Argc is " << argc << ".\n" << logofs_flush;

  for (int argi = 0; argi < argc; argi++)
  {
    *logofs << "Loop: Argv[" << argi << "] is " << argv[argi]
            << ".\n" << logofs_flush;
  }

  #endif

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
            #ifdef TEST
            *logofs << "Loop: Setting local proxy mode to proxy_client.\n"
                    << logofs_flush;
            #endif

            control -> ProxyMode = proxy_client;
          }
          else if (control -> ProxyMode != proxy_client)
          {
            #ifdef PANIC
            *logofs << "Loop: PANIC! Can't redefine local proxy to "
                    << "client mode.\n" << logofs_flush;
            #endif

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
            #ifdef TEST
            *logofs << "Loop: Setting local proxy mode to proxy_server.\n"
                    << logofs_flush;
            #endif

            control -> ProxyMode = proxy_server;
          }
          else if (control -> ProxyMode != proxy_server)
          {
            #ifdef PANIC
            *logofs << "Loop: PANIC! Can't redefine local proxy to "
                    << "server mode.\n" << logofs_flush;
            #endif

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

        if (ParseHostOption(nextArg, connectHost, connectPort) > 0)
        {
          //
          // Assume port is at a proxied display offset.
          //

          proxyPort = connectPort;

          connectPort += DEFAULT_NX_PROXY_PORT_OFFSET;
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

  #ifdef TEST
  *logofs << "Loop: Going to read options from file '"
          << fileName << "'.\n" << logofs_flush;
  #endif

  FILE *filePtr = fopen(fileName, "r");

  if (filePtr == NULL)
  {
    #ifdef PANIC
    *logofs << "Loop: PANIC! Can't open options file '" << fileName
            << "'. Error is " << EGET() << " '" << ESTR() << "'.\n"
            << logofs_flush;
    #endif

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
    #ifdef PANIC
    *logofs << "Loop: PANIC! Can't read options from file '" << fileName
            << "'. Error is " << EGET() << " '" << ESTR() << "'.\n"
            << logofs_flush;
    #endif

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

  #ifdef TEST
  *logofs << "Loop: Read options '" << options << "' from file '"
          << fileName << "'.\n" << logofs_flush;
  #endif

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
  #ifdef TEST
  *logofs << "Loop: Going to parse the remote options "
          << "string '" << opts << "'.\n"
          << logofs_flush;
  #endif

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
  int hasLimit  = 0;
  int hasRender = 0;
  int hasTaint  = 0;
  int hasType   = 0;
  int hasStrict = 0;
  int hasShseg  = 0;

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
        #ifdef WARNING
        *logofs << "Loop: WARNING! Ignoring remote option 'cookie' "
                << "with value '" << value << "' when initiating "
                << "connection.\n" << logofs_flush;
        #endif

        cerr << "Warning" << ": Ignoring remote option 'cookie' "
             << "with value '" << value << "' when initiating "
             << "connection.\n";
      }
      else if (strncasecmp(authCookie, value, strlen(authCookie)) != 0)
      {
        #ifdef PANIC
        *logofs << "Loop: PANIC! Authentication cookie '" << value
                << "' doesn't match '" << authCookie << "'.\n"
                << logofs_flush;
        #endif

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
          #ifdef WARNING
          *logofs << "Loop: WARNING! Overriding option 'link' "
                  << "with new value '" << value << "'.\n"
                  << logofs_flush;
          #endif

          cerr << "Warning" << ": Overriding option 'link' "
               << "with new value '" << value << "'.\n";
        }

        if (ParseLinkOption(value) < 0)
        {
          #ifdef PANIC
          *logofs << "Loop: PANIC! Can't identify remote 'link' "
                  << "option in string '" << value << "'.\n"
                  << logofs_flush;
          #endif

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
          #ifdef WARNING
          *logofs << "Loop: WARNING! Overriding option 'pack' "
                  << "with remote value '" << value << "'.\n"
                  << logofs_flush;
          #endif

          cerr << "Warning" << ": Overriding option 'pack' "
               << "with remote value '" << value << "'.\n";
        }

        if (ParsePackOption(value) < 0)
        {
          #ifdef PANIC
          *logofs << "Loop: PANIC! Invalid pack option '"
                  << value << "' requested by remote.\n"
                  << logofs_flush;
          #endif

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
          #ifdef PANIC
          *logofs << "Loop: PANIC! Can't identify remote 'cache' "
                  << "option in string '" << value << "'.\n"
                  << logofs_flush;
          #endif

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
          #ifdef PANIC
          *logofs << "Loop: PANIC! Can't identify remote 'images' "
                  << "option in string '" << value << "'.\n"
                  << logofs_flush;
          #endif

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
          #ifdef WARNING
          *logofs << "Loop: WARNING! Overriding option 'limit' "
                  << "with new value '" << value << "'.\n"
                  << logofs_flush;
          #endif

          cerr << "Warning" << ": Overriding option 'limit' "
               << "with new value '" << value << "'.\n";
        }

        if (ParseBitrateOption(value) < 0)
        {
          #ifdef PANIC
          *logofs << "Loop: PANIC! Can't identify 'limit' "
                  << "option in string '" << value << "'.\n"
                  << logofs_flush;
          #endif

          cerr << "Error" << ": Can't identify 'limit' "
               << "option in string '" << value << "'.\n";

          return -1;
        }
      }

      hasLimit = 1;
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

      hasRender = 1;
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

      hasTaint = 1;
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
          strncpy(sessionType, value, DEFAULT_STRING_LENGTH - 1);
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

      hasStrict = 1;
    }
    else if (strcasecmp(name, "shseg") == 0)
    {
      if (control -> ProxyMode == proxy_client)
      {
        PrintOptionIgnored("remote", name, value);
      }
      else if (ParseShmemOption(value) < 0)
      {
        #ifdef PANIC
        *logofs << "Loop: PANIC! Can't identify size of shared memory "
                << "segment in string '" << value << "'.\n"
               << logofs_flush;
        #endif

        cerr << "Error" << ": Can't identify size of shared memory "
             << "segment in string '" << value << "'.\n";

        return -1;
      }

      hasShseg = 1;
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

      #ifdef DEBUG
      *logofs << "Loop: Ignoring obsolete remote option '"
              << name << "' with value '" << value
              << "'.\n" << logofs_flush;
      #endif
    }
    else
    {
      #ifdef WARNING
      *logofs << "Loop: WARNING! Ignoring unknown remote option '"
              << name << "' with value '" << value << "'.\n"
              << logofs_flush;
      #endif

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
    #ifdef PANIC
    *logofs << "Loop: PANIC! The remote peer didn't specify the option '"
            << missing << "'.\n" << logofs_flush;
    #endif

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
  #ifdef TEST
  *logofs << "Loop: Going to parse the forwarder options "
          << "string '" << opts << "'.\n"
          << logofs_flush;
  #endif

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
        #ifdef PANIC
        *logofs << "Loop: PANIC! The NX forwarder cookie '" << value
                << "' doesn't match '" << authCookie << "'.\n"
                << logofs_flush;
        #endif

        cerr << "Error" << ": The NX forwarder cookie '" << value
             << "' doesn't match '" << authCookie << "'.\n";

        return -1;
      }

      hasCookie = 1;
    }
    else
    {
      #ifdef WARNING
      *logofs << "Loop: WARNING! Ignoring unknown forwarder option '"
              << name << "' with value '" << value << "'.\n"
              << logofs_flush;
      #endif

      cerr << "Warning" << ": Ignoring unknown forwarder option '"
           << name << "' with value '" << value << "'.\n";
    }

    name = strtok(NULL, "=");

  } // End of while (name) ...

  if (hasCookie == 0)
  {
    #ifdef PANIC
    *logofs << "Loop: PANIC! The NX forwarder didn't provide "
            << "the authentication cookie.\n" << logofs_flush;
    #endif

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
    #ifdef TEST
    *logofs << "Cannot read RLIMIT_CORE. Error is '"
            << ESTR() << "'.\n" << logofs_flush;
    #endif

    return -1;
  }

  if (rlim.rlim_cur < rlim.rlim_max)
  {
    rlim.rlim_cur = rlim.rlim_max;

    if (setrlimit(RLIMIT_CORE, &rlim))
    {
      #ifdef TEST
      *logofs << "Loop: Cannot read RLIMIT_CORE. Error is '"
              << ESTR() << "'.\n" << logofs_flush;
      #endif

      return -2;
    }
  }

  #ifdef TEST
  *logofs << "Loop: Set RLIMIT_CORE to "<< rlim.rlim_max
          << ".\n" << logofs_flush;
  #endif

  #endif // #ifdef COREDUMPS

  return 1;
}

char *GetLastCache(char *listBuffer, const char *searchPath)
{
  if (listBuffer == NULL || searchPath == NULL ||
          strncmp(listBuffer, "cachelist=", strlen("cachelist=")) != 0)
  {
    #ifdef TEST
    *logofs << "Loop: Invalid parameters '" << listBuffer << "' and '"
            << (searchPath != NULL ? searchPath : "")
            << "'. Can't select any cache.\n" << logofs_flush;
    #endif

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
      #ifdef TEST
      *logofs << "Loop: No cache files seem to be available.\n"
              << logofs_flush;
      #endif

      delete [] selectedName;

      return NULL;
    }
    else if (strlen(fileName) != MD5_LENGTH * 2 + 2 ||
                 strncmp(fileName, remotePrefix, 2) != 0)
    {
      #ifdef PANIC
      *logofs << "Loop: PANIC! Bad cache file name '"
               << fileName << "'.\n" << logofs_flush;
      #endif

      cerr << "Error" << ": Bad cache file name '"
           << fileName << "'.\n";

      delete [] selectedName;

      HandleCleanup();
    }

    #ifdef TEST
    *logofs << "Loop: Parsing remote cache name '"
            << fileName << "'.\n" << logofs_flush;
    #endif

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
      #ifdef TEST
      *logofs << "Loop: Found a matching cache '"
              << fullPath << "'.\n" << logofs_flush;
      #endif

      if (fileStat.st_mtime >= selectedTime)
      {
        strcpy(selectedName, fileName);

        selectedTime = fileStat.st_mtime;
      }
    }
    #ifdef TEST
    else
    {
      *logofs << "Loop: Can't get stats of file '"
              << fullPath << "'.\n" << logofs_flush;
    }
    #endif

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
      #ifdef TEST
      *logofs << "Loop: WARNING! No environment for NX_TEMP.\n"
              << logofs_flush;
      #endif

      tempEnv = getenv("TEMP");

      if (tempEnv == NULL || *tempEnv == '\0')
      {
        #ifdef TEST
        *logofs << "Loop: WARNING! No environment for TEMP.\n"
                << logofs_flush;
        #endif

        tempEnv = "/tmp";
      }
    }

    if (strlen(tempEnv) > DEFAULT_STRING_LENGTH - 1)
    {
      #ifdef PANIC
      *logofs << "Loop: PANIC! Invalid value for the NX "
              << "temporary directory '" << tempEnv
              << "'.\n" << logofs_flush;
      #endif

      cerr << "Error" << ": Invalid value for the NX "
           << "temporary directory '" << tempEnv
           << "'.\n";

      HandleCleanup();
    }

    strcpy(tempDir, tempEnv);

    #ifdef TEST
    *logofs << "Loop: Assuming temporary NX directory '"
            << tempDir << "'.\n" << logofs_flush;
    #endif
  }

  char *tempPath = new char[strlen(tempDir) + 1];

  if (tempPath == NULL)
  {
    #ifdef PANIC
    *logofs << "Loop: PANIC! Can't allocate memory "
            << "for the temp path.\n" << logofs_flush;
    #endif

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
      #ifdef TEST
      *logofs << "Loop: WARNING! No environment for NX_CLIENT.\n"
              << logofs_flush;
      #endif

      //
      // Try to guess the location of the client.
      //

      clientEnv = "/usr/NX/bin/nxclient";

      #ifdef __APPLE__

      clientEnv = "/Applications/NX Client for OSX.app/Contents/MacOS/nxclient";

      #endif

      #ifdef __CYGWIN32__

      clientEnv = "C:\\Program Files\\NX Client for Windows\\nxclient";

      #endif
    }

    if (strlen(clientEnv) > DEFAULT_STRING_LENGTH - 1)
    {
      #ifdef PANIC
      *logofs << "Loop: PANIC! Invalid value for the NX "
              << "client directory '" << clientEnv
              << "'.\n" << logofs_flush;
      #endif

      cerr << "Error" << ": Invalid value for the NX "
           << "client directory '" << clientEnv
           << "'.\n";

      HandleCleanup();
    }

    strcpy(clientDir, clientEnv);

    #ifdef TEST
    *logofs << "Loop: Assuming NX client location '"
            << clientDir << "'.\n" << logofs_flush;
    #endif
  }

  char *clientPath = new char[strlen(clientDir) + 1];

  if (clientPath == NULL)
  {
    #ifdef PANIC
    *logofs << "Loop: PANIC! Can't allocate memory "
            << "for the client path.\n" << logofs_flush;
    #endif

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
      #ifdef TEST
      *logofs << "Loop: WARNING! No environment for NX_SYSTEM.\n"
              << logofs_flush;
      #endif

      systemEnv = "/usr/NX";
    }

    if (strlen(systemEnv) > DEFAULT_STRING_LENGTH - 1)
    {
      #ifdef PANIC
      *logofs << "Loop: PANIC! Invalid value for the NX "
              << "system directory '" << systemEnv
              << "'.\n" << logofs_flush;
      #endif

      cerr << "Error" << ": Invalid value for the NX "
           << "system directory '" << systemEnv
           << "'.\n";

      HandleCleanup();
    }

    strcpy(systemDir, systemEnv);

    #ifdef TEST
    *logofs << "Loop: Assuming system NX directory '"
            << systemDir << "'.\n" << logofs_flush;
    #endif
  }

  char *systemPath = new char[strlen(systemDir) + 1];

  if (systemPath == NULL)
  {
    #ifdef PANIC
    *logofs << "Loop: PANIC! Can't allocate memory "
            << "for the system path.\n" << logofs_flush;
    #endif

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
      #ifdef TEST
      *logofs << "Loop: WARNING! No environment for NX_HOME.\n"
              << logofs_flush;
      #endif

      homeEnv = getenv("HOME");

      if (homeEnv == NULL || *homeEnv == '\0')
      {
        #ifdef PANIC
        *logofs << "Loop: PANIC! No environment for HOME.\n"
                << logofs_flush;
        #endif

        cerr << "Error" << ": No environment for HOME.\n";

        HandleCleanup();
      }
    }

    if (strlen(homeEnv) > DEFAULT_STRING_LENGTH - 1)
    {
      #ifdef PANIC
      *logofs << "Loop: PANIC! Invalid value for the NX "
              << "home directory '" << homeEnv
              << "'.\n" << logofs_flush;
      #endif

      cerr << "Error" << ": Invalid value for the NX "
           << "home directory '" << homeEnv
           << "'.\n";

      HandleCleanup();
    }

    strcpy(homeDir, homeEnv);

    #ifdef TEST
    *logofs << "Loop: Assuming NX user's home directory '"
            << homeDir << "'.\n" << logofs_flush;
    #endif
  }

  char *homePath = new char[strlen(homeDir) + 1];

  if (homePath == NULL)
  {
    #ifdef PANIC
    *logofs << "Loop: PANIC! Can't allocate memory "
            << "for the home path.\n" << logofs_flush;
    #endif

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
      #ifdef TEST
      *logofs << "Loop: WARNING! No environment for NX_ROOT.\n"
              << logofs_flush;
      #endif

      //
      // We will determine the root NX directory
      // based on the NX_HOME or HOME directory
      // settings.
      //

      const char *homeEnv = GetHomePath();

      if (strlen(homeEnv) > DEFAULT_STRING_LENGTH -
              strlen("/.nx") - 1)
      {
        #ifdef PANIC
        *logofs << "Loop: PANIC! Invalid value for the NX "
                << "home directory '" << homeEnv
                << "'.\n" << logofs_flush;
        #endif

        cerr << "Error" << ": Invalid value for the NX "
             << "home directory '" << homeEnv
             << "'.\n";

        HandleCleanup();
      }

      #ifdef TEST
      *logofs << "Loop: Assuming NX root directory in "
              << "the user's home '" << homeEnv
              << "'.\n" << logofs_flush;
      #endif

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
          #ifdef PANIC
          *logofs << "Loop: PANIC! Can't create directory '"
                  << rootDir << ". Error is " << EGET() << " '"
                  << ESTR() << "'.\n" << logofs_flush;
          #endif

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
        #ifdef PANIC
        *logofs << "Loop: PANIC! Invalid value for the NX "
                << "root directory '" << rootEnv
                << "'.\n" << logofs_flush;
        #endif

        cerr << "Error" << ": Invalid value for the NX "
             << "root directory '" << rootEnv
             << "'.\n";

        HandleCleanup();
      }

      strcpy(rootDir, rootEnv);
    }
        
    #ifdef TEST
    *logofs << "Loop: Assuming NX root directory '"
            << rootDir << "'.\n" << logofs_flush;
    #endif
  }

  char *rootPath = new char[strlen(rootDir) + 1];

  if (rootPath == NULL)
  {
    #ifdef PANIC
    *logofs << "Loop: PANIC! Can't allocate memory "
            << "for the root path.\n" << logofs_flush;
    #endif

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
      #ifdef PANIC
      *logofs << "Loop: PANIC! Can't create directory '" << cachePath
              << ". Error is " << EGET() << " '" << ESTR() << "'.\n"
              << logofs_flush;
      #endif

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
      #ifdef PANIC
      *logofs << "Loop: PANIC! Can't create directory '" << imagesPath
              << ". Error is " << EGET() << " '" << ESTR() << "'.\n"
              << logofs_flush;
      #endif

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
        #ifdef PANIC
        *logofs << "Loop: PANIC! Can't create directory '" << digitPath
                << ". Error is " << EGET() << " '" << ESTR() << "'.\n"
                << logofs_flush;
        #endif

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
        #ifdef PANIC
        *logofs << "Loop: PANIC! Can't create directory '" << sessionDir
                << ". Error is " << EGET() << " '" << ESTR() << "'.\n"
                << logofs_flush;
        #endif

        cerr << "Error" << ": Can't create directory '" << sessionDir
             << ". Error is " << EGET() << " '" << ESTR() << "'.\n";

        delete [] rootPath;

        return NULL;
      }
    }

    #ifdef TEST
    *logofs << "Loop: Root of NX session is '" << sessionDir
            << "'.\n" << logofs_flush;
    #endif

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
  #ifdef DEBUG
  *logofs << "Loop: Pack method is " << packMethod
          << " quality is " << packQuality << ".\n"
          << logofs_flush;
  #endif

  #ifdef DEBUG
  *logofs << "Loop: Parsing pack method '" << opt
          << "'.\n" << logofs_flush;
  #endif

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
    const char *dash = rindex(opt, '-');

    if (dash != NULL && strlen(dash) == 2 &&
            *(dash + 1) >= '0' && *(dash + 1) <= '9')
    {
      packQuality = atoi(dash + 1);

      #ifdef DEBUG
      *logofs << "Loop: Using pack quality '"
              << packQuality << "'.\n" << logofs_flush;
      #endif
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

    #ifdef TEST
    *logofs << "Loop: Assuming default statistics file '"
            << statsFileName << "'.\n" << logofs_flush;
    #endif
  }
  #ifdef TEST
  else
  {
    *logofs << "Loop: Name selected for statistics is '"
            << statsFileName << "'.\n" << logofs_flush;
  }
  #endif

  if (OpenLogFile(statsFileName, statofs) < 0)
  {
    HandleCleanup();
  }

  #ifndef MIXED

  if (*errorsFileName == '\0')
  {
    strcpy(errorsFileName, "errors");

    #ifdef TEST
    *logofs << "Loop: Assuming default log file name '"
            << errorsFileName << "'.\n" << logofs_flush;
    #endif
  }
  #ifdef TEST
  else
  {
    *logofs << "Loop: Name selected for log file is '"
            << errorsFileName << "'.\n" << logofs_flush;
  }
  #endif

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
    #ifdef TEST
    *logofs << "Loop: Name selected for session file is '"
            << sessionFileName << "'.\n" << logofs_flush;
    #endif

    if (errofs != NULL)
    {
      #ifdef WARNING
      *logofs << "Loop: WARNING! Unexpected value for stream errofs.\n"
              << logofs_flush;
      #endif

      cerr << "Warning" << ": Unexpected value for stream errofs.\n";
    }

    if (errsbuf != NULL)
    {
      #ifdef WARNING
      *logofs << "Loop: WARNING! Unexpected value for buffer errsbuf.\n"
              << logofs_flush;
      #endif

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

  if (cupsPort <= 0)
  {
    #ifdef TEST
    *logofs << "Loop: Disabling cups connections.\n"
            << logofs_flush;
    #endif

    cupsPort = 0;

    useCupsSocket = 0;
  }
  else
  {
    if (control -> ProxyMode == proxy_client)
    {
      if (cupsPort == 1)
      {
        cupsPort = DEFAULT_NX_CUPS_PORT_OFFSET + proxyPort;
      }

      useCupsSocket = 1;
    }
    else
    {
      if (cupsPort == 1)
      {
        //
        // Use the well-known 631/tcp port of the
        // Internet Printing Protocol.
        //

        cupsPort = 631;
      }

      useCupsSocket = 0;
    }

    #ifdef TEST
    *logofs << "Loop: Using cups port '" << cupsPort
            << "'.\n" << logofs_flush;
    #endif
  }

  if (auxPort <= 0)
  {
    #ifdef TEST
    *logofs << "Loop: Disabling auxiliary X11 connections.\n"
            << logofs_flush;
    #endif

    auxPort = 0;

    useAuxSocket = 0;
  }
  else
  {
    if (control -> ProxyMode == proxy_client)
    {
      if (auxPort == 1)
      {
        auxPort = DEFAULT_NX_AUX_PORT_OFFSET + proxyPort;
      }

      useAuxSocket = 1;
    }
    else
    {
      //
      // Auxiliary X connections are always forwarded
      // to the display where the session is running.
      // The only value accepted is 1.
      //

      if (auxPort != 1)
      {
        #ifdef WARNING
        *logofs << "Loop: WARNING! Overriding auxiliary X11 "
                << "port with new value '" << 1 << "'.\n"
                << logofs_flush;
        #endif

        cerr << "Warning" << ": Overriding auxiliary X11 "
             << "port with new value '" << 1 << "'.\n";

        auxPort = 1;
      }

      useAuxSocket = 0;
    }

    #ifdef TEST
    *logofs << "Loop: Using auxiliary X11 port '" << auxPort
            << "'.\n" << logofs_flush;
    #endif
  }

  if (smbPort <= 0)
  {
    #ifdef TEST
    *logofs << "Loop: Disabling SMB connections.\n"
            << logofs_flush;
    #endif

    smbPort = 0;

    useSmbSocket = 0;
  }
  else
  {
    if (control -> ProxyMode == proxy_client)
    {
      if (smbPort == 1)
      {
        smbPort = DEFAULT_NX_SMB_PORT_OFFSET + proxyPort;
      }

      useSmbSocket = 1;
    }
    else
    {
      if (smbPort == 1)
      {
        //
        // Assume the 139/tcp port used for SMB
        // over NetBIOS over TCP.
        //

        smbPort = 139;
      }

      useSmbSocket = 0;
    }

    #ifdef TEST
    *logofs << "Loop: Using SMB port '" << smbPort
            << "'.\n" << logofs_flush;
    #endif
  }

  if (mediaPort <= 0)
  {
    #ifdef TEST
    *logofs << "Loop: Disabling multimedia connections.\n"
            << logofs_flush;
    #endif

    mediaPort = 0;

    useMediaSocket = 0;
  }
  else
  {
    if (control -> ProxyMode == proxy_client)
    {
      if (mediaPort == 1)
      {
        mediaPort = DEFAULT_NX_MEDIA_PORT_OFFSET + proxyPort;
      }

      useMediaSocket = 1;
    }
    else
    {
      if (mediaPort == 1)
      {
        //
        // We don't have a well-known port to
        // be used for media connections.
        //

        #ifdef PANIC
        *logofs << "Loop: PANIC! No port specified for multimedia connections.\n"
                << logofs_flush;
        #endif

        cerr << "Error" << ": No port specified for multimedia connections.\n";

        HandleCleanup();
      }

      useMediaSocket = 0;
    }

    #ifdef TEST
    *logofs << "Loop: Using multimedia port '" << mediaPort
            << "'.\n" << logofs_flush;
    #endif
  }

  if (httpPort <= 0)
  {
    #ifdef TEST
    *logofs << "Loop: Disabling HTTP connections.\n"
            << logofs_flush;
    #endif

    httpPort = 0;

    useHttpSocket = 0;
  }
  else
  {
    if (control -> ProxyMode == proxy_client)
    {
      if (httpPort == 1)
      {
        httpPort = DEFAULT_NX_HTTP_PORT_OFFSET + proxyPort;
      }

      useHttpSocket = 1;
    }
    else
    {
      if (httpPort == 1)
      {
        //
        // Use the well-known 80/tcp port.
        //

        httpPort = 80;
      }

      useHttpSocket = 0;
    }

    #ifdef TEST
    *logofs << "Loop: Using HTTP port '" << httpPort
            << "'.\n" << logofs_flush;
    #endif
  }

  if (ParseFontPath(fontPort) <= 0)
  {
    #ifdef TEST
    *logofs << "Loop: Disabling font server connections.\n"
            << logofs_flush;
    #endif

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

    #ifdef TEST
    *logofs << "Loop: Using font server port '" << fontPort
            << "'.\n" << logofs_flush;
    #endif
  }

  if (slavePort <= 0)
  {
    #ifdef TEST
    *logofs << "Loop: Disabling slave connections.\n"
            << logofs_flush;
    #endif

    slavePort = 0;

    useSlaveSocket = 0;
  }
  else
  {
    //
    // File transfer connections can
    // be originated by both sides.
    //

    if (slavePort == 1)
    {
      if (control -> ProxyMode == proxy_client)
      {
        slavePort = DEFAULT_NX_SLAVE_PORT_CLIENT_OFFSET + proxyPort;
      }
      else
      {
        slavePort = DEFAULT_NX_SLAVE_PORT_SERVER_OFFSET + proxyPort;
      }
    }

    useSlaveSocket = 1;

    #ifdef TEST
    *logofs << "Loop: Using slave port '" << slavePort
            << "'.\n" << logofs_flush;
    #endif
  }

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
    #ifdef PANIC
    *logofs << "Loop: PANIC! Cannot determine number of available "
            << "file descriptors.\n" << logofs_flush;
    #endif

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
    #ifdef PANIC
    *logofs << "Loop: PANIC! Error getting or creating the cache path.\n"
            << logofs_flush;
    #endif

    cerr << "Error" << ": Error getting or creating the cache path.\n";

    HandleCleanup();
  }

  #ifdef TEST
  *logofs << "Loop: Path of cache files is '" << control -> PersistentCachePath
          << "'.\n" << logofs_flush;
  #endif

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

    if (*sessionType != '\0' &&
            (control -> isProtoStep8() == 1 ||
                strncmp(sessionType, "unix-", strlen("unix-")) != 0))
    {
      #ifdef WARNING
      *logofs << "Loop: WARNING! Unrecognized session type '"
              << sessionType << "'. Assuming agent session.\n"
              << logofs_flush;
      #endif

      cerr << "Warning" << ": Unrecognized session type '"
           << sessionType << "'. Assuming agent session.\n";
    }

    control -> SessionMode = session_agent;
  }

  #if defined(TEST) || defined(INFO)
  *logofs << "Loop: Assuming session type '"
          << DumpSession(control -> SessionMode) << "' with "
          << "string '" << sessionType << "'.\n"
          << logofs_flush;
  #endif

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

    #if defined(TEST) || defined(INFO)
    *logofs << "Loop: WARNING! Forcing flush policy to '"
            << DumpPolicy(control -> FlushPolicy)
            << ".\n" << logofs_flush;
    #endif
  }
  else
  {
    control -> FlushPolicy = policy_immediate;

    #if defined(TEST) || defined(INFO)
    *logofs << "Loop: Setting initial flush policy to '"
            << DumpPolicy(control -> FlushPolicy)
            << "'.\n" << logofs_flush;
    #endif
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
    #if defined(TEST) || defined(INFO)
    *logofs << "Loop: Proxy running as part of an "
            << "encrypting client.\n"
            << logofs_flush;
    #endif
  }
  else
  {
    #if defined(TEST) || defined(INFO)
    *logofs << "Loop: Assuming proxy running as a "
            << "standalone program.\n"
            << logofs_flush;
    #endif
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
      #ifdef TEST
      *logofs << "Loop: Enabling respawn of client at session shutdown.\n"
              << logofs_flush;
      #endif

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

  #ifdef DEBUG
  *logofs << "Loop: Storage size limit is "
          << control -> ClientTotalStorageSize
          << " at client and "
          << control -> ServerTotalStorageSize
          << " at server.\n"
          << logofs_flush;
  #endif

  #ifdef DEBUG
  *logofs << "Loop: Storage local limit set to "
          << control -> LocalTotalStorageSize
          << " remote limit set to "
          << control -> RemoteTotalStorageSize
          << ".\n" << logofs_flush;
  #endif

  //
  // Never reserve for split store more than
  // half the memory available for messages.
  //

  if (size > 0 && control ->
          SplitTotalStorageSize > size / 2)
  {
    #ifdef TEST
    *logofs << "Loop: Reducing size of split store to "
            << size / 2 << " bytes.\n"
            << logofs_flush;
    #endif

    control -> SplitTotalStorageSize = size / 2;
  }

  //
  // Don't load render from persistent
  // cache if extension is hidden or
  // not supported by agent.
  //

  if (control -> HideRender == 1)
  {
    #ifdef TEST
    *logofs << "Loop: Not loading render extension "
            << "from persistent cache.\n"
            << logofs_flush;
    #endif

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

    #if defined(TEST) || defined(INFO)
    *logofs << "Loop: Set initial shared memory size "
            << "to " << control -> ShmemServerSize
            << " bytes.\n" << logofs_flush;
    #endif
  }
  else
  {
    #if defined(TEST) || defined(INFO)
    *logofs << "Loop: Disabled use of the shared memory "
            << "extension.\n" << logofs_flush;
    #endif

    control -> ShmemServer = 0;
  }

  return 1;
}

//
// Adjust the pack method according to the
// type of the session.
//

int SetPack()
{
  #ifdef TEST
  *logofs << "Loop: Setting pack with initial method "
          << packMethod << " and quality " << packQuality
          << ".\n" << logofs_flush;
  #endif

  //
  // Check if this is a proxy session and, in
  // this case, set the pack method to none.
  // Packed images are not supported by plain
  // X applications.
  //

  if (control -> SessionMode == session_proxy)
  {
    #ifdef TEST
    *logofs << "Loop: WARNING! Disabling pack with proxy session.\n"
            << logofs_flush;
    #endif

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

    #ifdef TEST
    *logofs << "Loop: Not loading packed images "
            << "from persistent cache.\n"
            << logofs_flush;
    #endif
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
    #ifdef TEST
    *logofs << "Loop: Disabling image cache with "
            << "session '" << DumpSession(control ->
               SessionMode) << "'.\n" << logofs_flush;
    #endif

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
        #ifdef PANIC
        *logofs << "Loop: PANIC! Error getting or creating image cache path.\n"
                << logofs_flush;
        #endif

        cerr << "Error" << ": Error getting or creating image cache path.\n";

        HandleCleanup();
      }

      #ifdef TEST
      *logofs << "Loop: Path of image cache files is '" << control -> ImageCachePath
              << "'.\n" << logofs_flush;
      #endif
    }
  }
  else
  {
    #ifdef TEST
    *logofs << "Loop: Disabling the persistent image cache.\n"
            << logofs_flush;
    #endif

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

    #ifdef TEST
    *logofs << "Loop: Using compatibility version '"
            << major << "." << minor << "." << patch
            << "'.\n" << logofs_flush;
    #endif
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

    #ifdef TEST
    *logofs << "Loop: Using remote version '"
            << major << "." << minor << "." << patch
            << "'.\n" << logofs_flush;
    #endif
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

      #ifdef TEST
      *logofs << "Loop: Using remote version '"
              << major << "." << minor << "." << patch
              << "'.\n" << logofs_flush;
      #endif
    }
    else
    {
      major = control -> LocalVersionMajor;
      minor = control -> LocalVersionMinor;
      patch = control -> LocalVersionPatch;

      #ifdef TEST
      *logofs << "Loop: Using local version '"
              << major << "." << minor << "." << patch
              << "'.\n" << logofs_flush;
      #endif
    }
  }

  //
  // Handle the 1.5.0 versions. The protocol
  // step 6 is the minimum supported version.
  //

  int step = 0;

  if (major == 1)
  {
    if (minor == 5)
    {
      step = 6;
    }
  }
  else if (major == 2)
  {
    step = 7;
  }
  else if (major == 3)
  {
    if (minor >= 2)
    {
      step = 10;
    }
    else if (minor > 0 || patch > 0)
    {
      step = 9;
    }
    else
    {
      step = 8;
    }
  }
  else if (major > 3)
  {
    step = 10;
  }

  if (step == 0)
  {
    #ifdef PANIC
    *logofs << "Loop: PANIC! Incompatible remote version "
            << control -> RemoteVersionMajor << "." << control -> RemoteVersionMinor
            << "." << control -> RemoteVersionPatch << " with local version "
            << control -> LocalVersionMajor << "." << control -> LocalVersionMinor
            << "." << control -> LocalVersionPatch << ".\n" << logofs_flush;
    #endif

    cerr << "Error" << ": Incompatible remote version "
         << control -> RemoteVersionMajor << "." << control -> RemoteVersionMinor
         << "." << control -> RemoteVersionPatch << " with local version "
         << control -> LocalVersionMajor << "." << control -> LocalVersionMinor
         << "." << control -> LocalVersionPatch << ".\n";

    return -1;
  }

  #ifdef TEST
  *logofs << "Loop: Using NX protocol step "
          << step << ".\n" << logofs_flush;
  #endif

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
    #ifdef WARNING
    *logofs << "Loop: WARNING! Connected to remote version "
            << control -> RemoteVersionMajor << "." << control -> RemoteVersionMinor
            << "." << control -> RemoteVersionPatch << " with local version "
            << control -> LocalVersionMajor << "." << control -> LocalVersionMinor
            << "." << control -> LocalVersionPatch << ".\n" << logofs_flush;
    #endif

    cerr << "Warning" << ": Connected to remote version "
         << control -> RemoteVersionMajor << "." << control -> RemoteVersionMinor
         << "." << control -> RemoteVersionPatch << " with local version "
         << control -> LocalVersionMajor << "." << control -> LocalVersionMinor
         << "." << control -> LocalVersionPatch << ".\n" << logofs_flush;
  }

  if (local < remote)
  {
    cerr << "Warning" << ": Consider checking http://www.nomachine.com/ for updates.\n";
  }

  //
  // Now that we are aware of the remote
  // version, let's adjust the options to
  // be compatible with the remote proxy.
  //

  if (control -> ProxyMode == proxy_client)
  {
    if (control -> isProtoStep8() == 0)
    {
      if (strncmp(sessionType, "shadow", strlen("shadow")) == 0 ||
              strncmp(sessionType, "application", strlen("application")) == 0 ||
                  strncmp(sessionType, "console", strlen("console")) == 0 ||
                      strncmp(sessionType, "default", strlen("default")) == 0 ||
                          strncmp(sessionType, "gnome", strlen("gnome")) == 0 ||
                              strncmp(sessionType, "kde", strlen("kde")) == 0 ||
                                  strncmp(sessionType, "cde", strlen("cde")) == 0 ||
                                      strncmp(sessionType, "xdm", strlen("xdm")) == 0)

      {
        #if defined(TEST) || defined(INFO)
        *logofs << "Loop: WARNING! Prepending 'unix-' to the "
                << "name of the session.\n" << logofs_flush;
        #endif

        char buffer[DEFAULT_STRING_LENGTH];

        snprintf(buffer, DEFAULT_STRING_LENGTH - 1, "unix-%s", sessionType);

        strcpy(sessionType, buffer);
      }
    }

    //
    // Check if the remote is able to handle
    // the selected pack method.
    //

    if (control -> isProtoStep8() == 0)
    {
      if (packMethod == PACK_ADAPTIVE || packMethod == PACK_LOSSY)
      {
        #ifdef TEST
        *logofs << "Loop: WARNING! Assuming a lossy encoding with "
                << "an old proxy version.\n" << logofs_flush;
        #endif

        packMethod = PACK_JPEG_16M_COLORS;
      }
      else if (packMethod == PACK_LOSSLESS)
      {
        #ifdef TEST
        *logofs << "Loop: WARNING! Assuming a lossless encoding with "
                << "an old proxy version.\n" << logofs_flush;
        #endif

        if (control -> isProtoStep7() == 1)
        {
          packMethod = PACK_RLE_16M_COLORS;
        }
        else
        {
          packMethod = PACK_PNG_16M_COLORS;
        }
      }
    }

    //
    // If the remote doesn't support the
    // selected method use something that
    // is compatible.
    //

    if ((packMethod == PACK_RGB_16M_COLORS ||
            packMethod == PACK_RLE_16M_COLORS ||
                packMethod == PACK_BITMAP_16M_COLORS) &&
                    control -> isProtoStep7() == 0)
    {
      #ifdef TEST
      *logofs << "Loop: WARNING! Setting the pack method to '"
              << PACK_PNG_16M_COLORS << "' with '" << packMethod
              << "' unsupported.\n" << logofs_flush;
      #endif

      packMethod  = PACK_PNG_16M_COLORS;
      packQuality = 9;
    }
    else if (packMethod == PACK_BITMAP_16M_COLORS &&
                 control -> isProtoStep8() == 0)
    {
      #ifdef TEST
      *logofs << "Loop: WARNING! Setting the pack method to '"
              << PACK_RLE_16M_COLORS << "' with '" << packMethod
              << "' unsupported.\n" << logofs_flush;
      #endif

      packMethod  = PACK_RLE_16M_COLORS;
      packQuality = 9;
    }

    //
    // Update the pack method name.
    //

    ParsePackMethod(packMethod, packQuality);
  }

  //
  // At the moment the image cache is not used by the
  // agent but we need to take care of the compatibi-
  // lity with old versions. Proxy versions older than
  // the 3.0.0 assume that it is enabled and will send
  // specific bits as part of the encoding. Conversely,
  // it is advisable to disable the cache right now.
  // By not enabling the image cache, the house-keep-
  // ing process will only take care of cleaning up
  // the "cache-" directories.
  //

  if (control -> isProtoStep8() == 1)
  {
    #ifdef TEST
    *logofs << "Loop: Disabling image cache with protocol "
            << "step '" << control -> getProtoStep()
            << "'.\n"  << logofs_flush;
    #endif

    sprintf(imagesSizeName, "0");

    control -> ImageCacheEnableLoad = 0;
    control -> ImageCacheEnableSave = 0;
  }

  return 1;
}

//
// Identify the requested link settings
// and update the control parameters
// accordingly.
//

int SetLink()
{
  #ifdef TEST
  *logofs << "Loop: Setting link with initial value "
          << linkSpeedName << ".\n" << logofs_flush;
  #endif

  if (*linkSpeedName == '\0')
  {
    strcpy(linkSpeedName, "lan");
  }
  
  #ifdef TEST
  *logofs << "Loop: Link speed is " << linkSpeedName
          << ".\n" << logofs_flush;
  #endif

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
    #ifdef PANIC
    *logofs << "Loop: PANIC! Unrecognized pack method id "
            << packMethod << " with quality " << packQuality
            << ".\n" << logofs_flush;
    #endif

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
      #ifdef WARNING
      *logofs << "Loop: WARNING! Forcing taint of replies "
              << "with a proxy session.\n"
              << logofs_flush;
      #endif

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
    #ifdef WARNING
    *logofs << "Loop: WARNING! Forcing flush on priority "
            << "with a proxy session.\n"
            << logofs_flush;
    #endif

    control -> FlushPriority = 1;
  }

  return 1;
}

//
// Parameters for MODEM 28.8/33.6/56 Kbps.
//

int SetLinkModem()
{
  #ifdef TEST
  *logofs << "Loop: Setting parameters for MODEM.\n"
          << logofs_flush;
  #endif

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
  #ifdef TEST
  *logofs << "Loop: Setting parameters for ISDN.\n"
          << logofs_flush;
  #endif

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
  #ifdef TEST
  *logofs << "Loop: Setting parameters for ADSL.\n"
          << logofs_flush;
  #endif

  control -> LinkMode = LINK_TYPE_ADSL;

  control -> TokenSize  = 512;
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
  #ifdef TEST
  *logofs << "Loop: Setting parameters for WAN.\n"
          << logofs_flush;
  #endif

  control -> LinkMode = LINK_TYPE_WAN;

  control -> TokenSize  = 768;
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
  #ifdef TEST
  *logofs << "Loop: Setting parameters for LAN.\n"
          << logofs_flush;
  #endif

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
    #if defined(TEST) || defined(INFO)
    *logofs << "Loop: LIMIT! Decreasing the token limit "
            << "to " << control -> TokenLimit / 2
            << " with option 'strict'.\n"
            << logofs_flush;
    #endif

    control -> TokenLimit /= 2;
  }

  #ifdef STRICT

  control -> TokenLimit = 1;

  #if defined(TEST) || defined(INFO)
  *logofs << "Loop: WARNING! LIMIT! Setting the token limit "
          << "to " << control -> TokenLimit
          << " to simulate the proxy congestion.\n"
          << logofs_flush;
  #endif

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

  #if defined(TEST) || defined(INFO)
  *logofs << "Loop: LIMIT! Setting client bitrate limit "
          << "to " << control -> ClientBitrateLimit
          << " server bitrate limit to " << control ->
             ServerBitrateLimit << " with local limit "
          << control -> LocalBitrateLimit << ".\n"
          << logofs_flush;
  #endif

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
    #ifdef PANIC
    *logofs << "Loop: PANIC! Invalid value '"
            << opt << "' for option 'cache'.\n"
            << logofs_flush;
    #endif

    cerr << "Error" << ": Invalid value '"
         << opt << "' for option 'cache'.\n";

    return -1;
  }

  #ifdef TEST
  *logofs << "Loop: Setting size of cache to "
          << size << " bytes.\n" << logofs_flush;
  #endif

  control -> ClientTotalStorageSize = size;
  control -> ServerTotalStorageSize = size;

  strcpy(cacheSizeName, opt);

  if (size == 0)
  {
    #ifdef WARNING
    *logofs << "Loop: WARNING! Disabling NX delta compression.\n"
            << logofs_flush;
    #endif

    control -> LocalDeltaCompression = 0;

    #ifdef WARNING
    *logofs << "Loop: WARNING! Disabling use of NX persistent cache.\n"
            << logofs_flush;
    #endif

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
    #ifdef PANIC
    *logofs << "Loop: PANIC! Invalid value '"
            << opt << "' for option 'images'.\n"
            << logofs_flush;
    #endif

    cerr << "Error" << ": Invalid value '"
         << opt << "' for option 'images'.\n";

    return -1;
  }

  #ifdef TEST
  *logofs << "Loop: Setting size of images cache to "
          << size << " bytes.\n" << logofs_flush;
  #endif

  control -> ImageCacheDiskLimit = size;

  strcpy(imagesSizeName, opt);

  return 1;
}

int ParseShmemOption(const char *opt)
{
  int size = ParseArg("", "shseg", opt);

  if (size < 0)
  {
    #ifdef PANIC
    *logofs << "Loop: PANIC! Invalid value '"
            << opt << "' for option 'shseg'.\n"
            << logofs_flush;
    #endif

    cerr << "Error" << ": Invalid value '"
         << opt << "' for option 'shseg'.\n";

    return -1;
  }

  control -> ShmemClientSize = size;
  control -> ShmemServerSize = size;

  #if defined(TEST) || defined(INFO)
  *logofs << "Loop: Set shared memory size to "
          << control -> ShmemServerSize << " bytes.\n"
          << logofs_flush;
  #endif

  strcpy(shsegSizeName, opt);

  return 1;
}

int ParseBitrateOption(const char *opt)
{
  int bitrate = ParseArg("", "limit", opt);

  if (bitrate < 0)
  {
    #ifdef PANIC
    *logofs << "Loop: PANIC! Invalid value '"
            << opt << "' for option 'limit'.\n"
            << logofs_flush;
    #endif

    cerr << "Error" << ": Invalid value '"
         << opt << "' for option 'limit'.\n";

    return -1;
  }

  strcpy(bitrateLimitName, opt);

  if (bitrate == 0)
  {
    #ifdef TEST
    *logofs << "Loop: Disabling bitrate limit on proxy link.\n"
            << logofs_flush;
    #endif

    control -> LocalBitrateLimit = 0;
  }
  else
  {
    #ifdef TEST
    *logofs << "Loop: Setting bitrate to " << bitrate
            << " bits per second.\n" << logofs_flush;
    #endif

    //
    // Internal representation is in bytes
    // per second.
    //

    control -> LocalBitrateLimit = bitrate >> 3;
  }

  return 1;
}

int ParseHostOption(const char *opt, char *host, int &port)
{
  #ifdef TEST
  *logofs << "Loop: Trying to parse options string '" << opt
          << "' as a remote NX host.\n" << logofs_flush;
  #endif

  if (opt == NULL || *opt == '\0')
  {
    #ifdef PANIC
    *logofs << "Loop: PANIC! No host parameter provided.\n"
            << logofs_flush;
    #endif

    return 0;
  }
  else if (strlen(opt) >= DEFAULT_STRING_LENGTH)
  {
    #ifdef PANIC
    *logofs << "Loop: PANIC! Host parameter exceeds length of "
            << DEFAULT_STRING_LENGTH << " characters.\n"
            << logofs_flush;
    #endif

    return 0;
  }

  //
  // Look for a host name followed
  // by a colon followed by port.
  //

  int newPort = port;

  const char *separator = rindex(opt, ':');

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
      #ifdef TEST
      *logofs << "Loop: Can't identify remote NX port in string '"
              << separator << "'.\n" << logofs_flush;
      #endif

      return 0;
    }
  }
  else if (newPort < 0)
  {
    //
    // Complain if port was not passed
    // by other means.
    //

    #ifdef TEST
    *logofs << "Loop: Can't identify remote NX port in string '"
            << opt << "'.\n" << logofs_flush;
    #endif

    return 0;
  }
  else
  {
    separator = opt + strlen(opt);
  }

  char newHost[DEFAULT_STRING_LENGTH] = { 0 };

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
    #ifdef TEST
    *logofs << "Loop: Can't identify remote NX host in string '"
            << newHost << "'.\n" << logofs_flush;
    #endif

    return 0;
  }
  else if (*acceptHost != '\0')
  {
    #ifdef PANIC
    *logofs << "Loop: PANIC! Can't manage to connect and accept connections "
            << "at the same time.\n" << logofs_flush;

    *logofs << "Loop: PANIC! Refusing remote NX host with string '"
            << opt << "'.\n" << logofs_flush;
    #endif

    cerr << "Error" << ": Can't manage to connect and accept connections "
         << "at the same time.\n";

    cerr << "Error" << ": Refusing remote NX host with string '"
         << opt << "'.\n";

    return -1;
  }

  if (*host != '\0' && strcmp(host, newHost) != 0)
  {
    #ifdef WARNING
    *logofs << "Loop: WARNING! Overriding remote NX host '"
            << host << "' with new value '" << newHost
            << "'.\n" << logofs_flush;
    #endif
  }

  strcpy(host, newHost);

  if (port != -1 && port != newPort)
  {
    #ifdef WARNING
    *logofs << "Loop: WARNING! Overriding remote NX port '"
            << port << "' with new value '" << newPort
            << "'.\n" << logofs_flush;
    #endif
  }

  #ifdef TEST
  *logofs << "Loop: Parsed options string '" << opt
          << "' with host '" << newHost << "' and port '"
          << newPort << "'.\n" << logofs_flush;
  #endif

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

  #ifdef TEST
  *logofs << "Loop: Parsing font server option '" << path
          << "'.\n" << logofs_flush;
  #endif

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
    #ifdef TEST
    *logofs << "Loop: Assuming numeric TCP port '" << atoi(path)
            << "' for font server.\n" << logofs_flush;
    #endif

    return 1;
  }

  //
  // Let's assume that a port specification "unix/:7100"
  // corresponds to "$TEMP/.font-unix/fs7100" and a port
  // "unix/:-1" corresponds to "$TEMP/.font-unix/fs-1".
  //

  if (strncmp("unix/:", path, 6) == 0)
  {
    snprintf(path, DEFAULT_STRING_LENGTH - 1, "%s/.font-unix/fs%s",
                 control -> TempPath, oldPath + 6);

    *(path + DEFAULT_STRING_LENGTH - 1) = '\0';

    #ifdef TEST
    *logofs << "Loop: Assuming Unix socket '" << path
            << "' for font server.\n" << logofs_flush;
    #endif
  }
  else if (strncmp("tcp/:", path, 5) == 0)
  {
    snprintf(path, DEFAULT_STRING_LENGTH - 1, "%d", atoi(oldPath + 5));

    *(path + DEFAULT_STRING_LENGTH - 1) = '\0';

    if (atoi(path) <= 0)
    {
      goto ParseFontPathError;
    }

    #ifdef TEST
    *logofs << "Loop: Assuming TCP port '" << atoi(path)
            << "' for font server.\n" << logofs_flush;
    #endif
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

    #ifdef TEST
    *logofs << "Loop: Assuming Unix socket '" << path
            << "' for font server.\n" << logofs_flush;
    #endif
  }

  return 1;

ParseFontPathError:

  #ifdef TEST
  *logofs << "Loop: Unable to determine the font server "
          << "port in string '" << path << "'.\n"
          << logofs_flush;
  #endif

  return -1;
}

int ParseListenOption(int &address)
{
  if (*listenHost == '\0')
  {
    //
    // On the X client side listen on any address.
    // On the X server side listen to the forwarder
    // on localhost.
    //

    if (control -> ProxyMode == proxy_server)
    {
      address = (int) inet_addr("127.0.0.1");
    }
    else
    {
      if ( loopbackBind )
      {
        address = htonl(INADDR_LOOPBACK);
      }
      else
      {
        address = htonl(INADDR_ANY);
      }
    }
  }
  else
  {
    address = inet_addr(listenHost);
  }

  return 1;
}

int OpenLogFile(char *name, ostream *&stream)
{
  if (name == NULL || *name == '\0')
  {
    #ifdef TEST
    *logofs << "Loop: WARNING! No name provided for output. Using standard error.\n"
            << logofs_flush;
    #endif

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
        #ifdef PANIC
        *logofs << "Loop: PANIC! Cannot determine directory of NX session file.\n"
                << logofs_flush;
        #endif

        cerr << "Error" << ": Cannot determine directory of NX session file.\n";

        return -1;
      }

      if (strlen(filePath) + strlen("/") +
              strlen(name) + 1 > DEFAULT_STRING_LENGTH)
      {
        #ifdef PANIC
        *logofs << "Loop: PANIC! Full name of NX file '" << name
                << " would exceed length of " << DEFAULT_STRING_LENGTH
                << " characters.\n" << logofs_flush;
        #endif

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
    #ifdef PANIC
    *logofs << "Loop: PANIC! Bad stream provided for output.\n"
            << logofs_flush;
    #endif

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
        #ifdef WARNING
        *logofs << "Loop: WARNING! Can't get stats of file '"
                << name  << "'. Error is " << EGET() 
                << " '" << ESTR() << "'.\n" << logofs_flush;
        #endif

        return 0;
      }
      else if (fileStat.st_size < (long) limit)
      {
        return 0;
      }
    }

    #ifdef TEST
    *logofs << "Loop: Deleting file '" << name
            << "' with size " << fileStat.st_size
            << ".\n" << logofs_flush;
    #endif

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

    #ifdef TEST
    *logofs << "Loop: Reopened file '" << name
            << "'.\n" << logofs_flush;
    #endif
  }

  return 1;
}

void PrintProcessInfo()
{
  if (agent == NULL)
  {
    cerr << "\nNXPROXY - Version " << control -> LocalVersionMajor
         << "." << control -> LocalVersionMinor << "."
         << control -> LocalVersionPatch << "\n\n";

    cerr << "Copyright (C) 2001, 2010 NoMachine.\n"
         << "See http://www.nomachine.com/ for more information.\n\n";
  }

  //
  // People get confused by the fact that client
  // mode is running on NX server and viceversa.
  // Let's adopt an user-friendly naming conven-
  // tion here.
  //

  cerr << "Info: Proxy running in "
       << (control -> ProxyMode == proxy_client ? "server" : "client")
       << " mode with pid '" << getpid() << "'.\n";

  if (agent == NULL)
  {
    cerr << "Session" << ": Starting session at '"
         << strTimestamp() << "'.\n";
  }

  #ifdef TEST

  if (*errorsFileName != '\0')
  {
    cerr << "Info" << ": Using errors file '" << errorsFileName << "'.\n";
  }

  if (*statsFileName != '\0')
  {
    cerr << "Info" << ": Using stats file '" << statsFileName << "'.\n";
  }

  #endif
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
          useCupsSocket > 0 && cupsPort > 0)
  {
    cerr << "Info" << ": Listening to CUPS connections "
         << "on port '" << cupsPort << "'.\n";
  }
  else if (control -> ProxyMode == proxy_server &&
               cupsPort > 0)
  {
    cerr << "Info" << ": Forwarding CUPS connections "
         << "to port '" << cupsPort << "'.\n";
  }

  if (control -> ProxyMode == proxy_client &&
          useAuxSocket > 0 && auxPort > 0)
  {
    cerr << "Info" << ": Listening to auxiliary X11 connections "
         << "on port '" << auxPort << "'.\n";
  }
  else if (control -> ProxyMode == proxy_server &&
               auxPort > 0)
  {
    cerr << "Info" << ": Forwarding auxiliary X11 connections "
         << "to display '" << displayHost << "'.\n";
  }

  if (control -> ProxyMode == proxy_client &&
          useSmbSocket > 0 && smbPort > 0)
  {
    cerr << "Info" << ": Listening to SMB connections "
         << "on port '" << smbPort << "'.\n";
  }
  else if (control -> ProxyMode == proxy_server &&
               smbPort > 0)
  {
    cerr << "Info" << ": Forwarding SMB connections "
         << "to port '" << smbPort << "'.\n";
  }

  if (control -> ProxyMode == proxy_client &&
          useMediaSocket > 0 && mediaPort > 0)
  {
    cerr << "Info" << ": Listening to multimedia connections "
         << "on port '" << mediaPort << "'.\n";
  }
  else if (control -> ProxyMode == proxy_server &&
               mediaPort > 0)
  {
    cerr << "Info" << ": Forwarding multimedia connections "
         << "to port '" << mediaPort << "'.\n";
  }

  if (control -> ProxyMode == proxy_client &&
          useHttpSocket > 0 && httpPort > 0)
  {
    cerr << "Info" << ": Listening to HTTP connections "
         << "on port '" << httpPort << "'.\n";
  }
  else if (control -> ProxyMode == proxy_server &&
               httpPort > 0)
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

  if (useSlaveSocket > 0 && slavePort > 0)
  {
    cerr << "Info" << ": Listening to slave connections "
         << "on port '" << slavePort << "'.\n";
  }
}

void PrintVersionInfo()
{
  cerr << "NXPROXY - " << "Version "
       << control -> LocalVersionMajor << "."
       << control -> LocalVersionMinor << "."
       << control -> LocalVersionPatch;

  cerr << endl;
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
    #ifdef WARNING
    *logofs << "Loop: WARNING! Ignoring " << type
            << " option '" << name << "' with value '"
            << value << "' at " << "NX client side.\n"
            << logofs_flush;
    #endif

    cerr << "Warning" << ": Ignoring " << type
         << " option '" << name << "' with value '"
         << value << "' at " << "NX client side.\n";
  }
  else
  {
    #ifdef WARNING
    *logofs << "Loop: WARNING! Ignoring " << type
            << " option '" << name << "' with value '"
            << value << "' at " << "NX server side.\n"
            << logofs_flush;
    #endif

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
      #ifdef TEST
      *logofs << "Loop: PANIC! Display options string '" << options
              << "' must start with 'nx' or 'nx/nx' prefix.\n"
              << logofs_flush;
      #endif

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
  #ifdef TEST
  *logofs << "Loop: Parsing " << type << " option '" << name
          << "' with value '" << (value ? value : "(null)")
          << "'.\n" << logofs_flush;
  #endif

  if (value == NULL || strstr(value, "=") != NULL)
  {
    #ifdef PANIC
    *logofs << "Loop: PANIC! Error in " << type << " option '"
            << name << "'. No value found.\n"
            << logofs_flush;
    #endif

    cerr << "Error" << ": Error in " << type << " option '"
         << name << "'. No value found.\n";

    return -1;
  }
  else if (strstr(name, ",") != NULL)
  {
    #ifdef PANIC
    *logofs << "Loop: PANIC! Parse error at " << type << " option '"
            << name << "'.\n" << logofs_flush;
    #endif

    cerr << "Error" << ": Parse error at " << type << " option '"
         << name << "'.\n";

    return -1;
  }
  else if (strlen(value) >= DEFAULT_STRING_LENGTH)
  {
    #ifdef PANIC
    *logofs << "Loop: PANIC! Value '" << value << "' of "
            << type << " option '" << name << "' exceeds length of "
            << DEFAULT_STRING_LENGTH << " characters.\n"
            << logofs_flush;
    #endif

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

  strncpy(string, value, strlen(value) - 1);

  *(string + (strlen(value) - 1)) = '\0';

  #ifdef TEST

  *logofs << "Loop: Parsing integer option '" << name
          << "' from string '" << string << "' with base set to ";

  switch (tolower(*id))
  {
    case 'k':
    case 'm':
    case 'g':
    {
      *logofs << (char) toupper(*id);
    }
    break;
  }

  *logofs << ".\n" << logofs_flush;

  #endif

  double result = atof(string) * base;

  if (result < 0 || result > (((unsigned) -1) >> 1))
  {
    delete [] string;

    return -1;
  }

  delete [] string;

  #ifdef TEST
  *logofs << "Loop: Integer option parsed to '"
          << (int) result << "'.\n" << logofs_flush;
  #endif

  return (int) result;
}

int ValidateArg(const char *type, const char *name, const char *value)
{
  int number = atoi(value);

  if (number < 0)
  {
    #ifdef PANIC
    *logofs << "Loop: PANIC! Invalid " << type
            << " option '" << name << "' with value '"
            << value << "'.\n" << logofs_flush;
    #endif

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
      #ifdef __CYGWIN32__

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
    #ifdef TEST
    *logofs << "Loop: End of NX transport requested "
            << "by remote.\n" << logofs_flush;
    #endif

    handleTerminatingInLoop();

    if (control -> ProxyMode == proxy_server)
    {
      #ifdef TEST
      *logofs << "Loop: Bytes received so far are "
              << (unsigned long long) statistics -> getBytesIn()
              << ".\n" << logofs_flush;
      #endif

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

    #ifdef TEST
    *logofs << "Loop: Shutting down the NX transport.\n"
            << logofs_flush;
    #endif
 
    HandleCleanup();
  }
  else if (proxy -> handlePing() < 0)
  {
    #ifdef TEST
    *logofs << "Loop: Failure handling the ping for "
            << "proxy FD#" << proxyFD << ".\n"
            << logofs_flush;
    #endif

    HandleShutdown();
  }

  //
  // Check if the watchdog has exited and we didn't
  // get the SIGCHLD. This can happen if the parent
  // has overridden our signal handlers.
  //

  if (IsRunning(lastWatchdog) && CheckProcess(lastWatchdog, "watchdog") == 0)
  {
    #ifdef WARNING
    *logofs << "Loop: WARNING! Watchdog is gone unnoticed. "
            << "Setting the last signal to SIGTERM.\n"
            << logofs_flush;
    #endif

    lastSignal = SIGTERM;

    #ifdef WARNING
    *logofs << "Loop: WARNING! Resetting pid of last "
            << "watchdog process.\n" << logofs_flush;
    #endif

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
    #ifdef TEST
    *logofs << "Loop: End of NX transport requested "
            << "by agent.\n" << logofs_flush;
    #endif

    #ifdef TEST
    *logofs << "Loop: Bytes sent so far are "
            << (unsigned long long) statistics -> getBytesOut()
            << ".\n" << logofs_flush;
    #endif

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
        #ifdef TEST
        *logofs << "Loop: End of NX transport requested by signal '"
                << signal << "' '" << DumpSignal(signal)
                << "'.\n" << logofs_flush;
        #endif

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
      #ifdef TEST
      *logofs << "Loop: Shutting down the NX transport.\n"
              << logofs_flush;
      #endif

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

        #ifdef TEST
        *logofs << "Loop: Starting watchdog process with timeout "
                << "of " << timeout << " Ms.\n"
                << logofs_flush;
        #endif
      }
      #ifdef TEST
      else
      {
        *logofs << "Loop: Starting watchdog process without "
                << "a timeout.\n" << logofs_flush;
      }
      #endif

      lastWatchdog = NXTransWatchdog(timeout);

      if (IsFailed(lastWatchdog))
      {
        #ifdef PANIC
        *logofs << "Loop: PANIC! Can't start the NX watchdog "
                << "process in shutdown.\n" << logofs_flush;
        #endif

        cerr << "Error" << ": Can't start the NX watchdog "
             << "process in shutdown.\n";

        HandleCleanup();
      }
      #ifdef TEST
      else
      {
        *logofs << "Loop: Watchdog started with pid '"
                << lastWatchdog << "'.\n" << logofs_flush;
      }
      #endif
    }
    else
    {
      #ifdef PANIC
      *logofs << "Loop: PANIC! Previous watchdog detected "
              << "in shutdown with pid '" << lastWatchdog
              << "'.\n" << logofs_flush;
      #endif

      cerr << "Error" << ": Previous watchdog detected "
           << "in shutdown with pid '" << lastWatchdog
           << "'.\n";

      HandleCleanup();
    }

    if (control -> CleanupTimeout > 0)
    {
      #ifdef TEST
      *logofs << "Loop: Waiting the cleanup timeout to complete.\n"
              << logofs_flush;
      #endif

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

      #ifdef TEST
      *logofs << "Loop: Waiting the watchdog process to complete.\n"
              << logofs_flush;
      #endif

      cerr << "Info" << ": Waiting the watchdog process to complete.\n";
    }

    lastKill = 2;
  }
}

static void handleCheckBitrateInLoop()
{
  static long int slept = 0;

  #ifdef TEST
  *logofs << "Loop: Bitrate is " << statistics -> getBitrateInShortFrame()
          << " B/s and " << statistics -> getBitrateInLongFrame()
          << " B/s in " << control -> ShortBitrateTimeFrame / 1000
          << "/" << control -> LongBitrateTimeFrame / 1000
          << " seconds timeframes.\n" << logofs_flush;
  #endif

  //
  // This can be improved. We may not jump out
  // of the select often enough to guarantee
  // the necessary accuracy.
  //

  if (control -> LocalBitrateLimit > 0)
  {
    #ifdef TEST
    *logofs << "Loop: Calculating bandwidth usage with limit "
            << control -> LocalBitrateLimit << ".\n"
            << logofs_flush;
    #endif

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
        #ifdef WARNING
        *logofs << "Loop: WARNING! Sleeping due to "
                << "reference bitrate of " << reference
                << " B/s.\n" << logofs_flush;
        #endif

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

#if defined(TEST) || defined(INFO)

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
        #ifdef PANIC
        *logofs << "Loop: PANIC! Buffer for descriptor FD#"
                << j << " has pending bytes to read.\n"
                << logofs_flush;
        #endif

        HandleCleanup();
      }

      fdLength = proxy -> getLength(j);

      if (fdLength > 0)
      {
        #ifdef TEST
        *logofs << "Loop: WARNING! Buffer for descriptor FD#"
                << j << " has " << fdLength << " bytes to write.\n"
                << logofs_flush;
        #endif
      }
    }
  }

  fdPending = proxy -> getPending(proxyFD);

  if (fdPending > 0)
  {
    #ifdef PANIC
    *logofs << "Loop: PANIC! Buffer for proxy descriptor FD#"
            << proxyFD << " has pending bytes to read.\n"
            << logofs_flush;
    #endif

    HandleCleanup();
  }

  fdLength = proxy -> getFlushable(proxyFD);

  if (fdLength > 0)
  {
    if (control -> FlushPolicy == policy_immediate &&
            proxy -> getBlocked(proxyFD) == 0)
    {
      #ifdef PANIC
      *logofs << "Loop: PANIC! Buffer for proxy descriptor FD#"
              << proxyFD << " has " << fdLength << " bytes "
              << "to write with policy 'immediate'.\n"
              << logofs_flush;
      #endif

      HandleCleanup();
    }
    else
    {
      #ifdef TEST
      *logofs << "Loop: WARNING! Buffer for proxy descriptor FD#"
              << proxyFD << " has " << fdLength << " bytes "
              << "to write.\n" << logofs_flush;
      #endif
    }
  }

  fdSplits = proxy -> getSplitSize();

  if (fdSplits > 0)
  {
    #ifdef WARNING
    *logofs << "Loop: WARNING! Proxy descriptor FD#" << proxyFD
            << " has " << fdSplits << " splits to send.\n"
            << logofs_flush;
    #endif
  }
}

static void handleCheckSelectInLoop(int &setFDs, fd_set &readSet,
                                        fd_set &writeSet, T_timestamp selectTs)
{
  #if defined(TEST) || defined(INFO)
  *logofs << "Loop: Maximum descriptors is ["
          << setFDs << "] at " << strMsTimestamp()
          << ".\n" << logofs_flush;
  #endif

  int i;

  if (setFDs > 0)
  {
    i = 0;

    #if defined(TEST) || defined(INFO)
    *logofs << "Loop: Selected for read are ";
    #endif

    for (int j = 0; j < setFDs; j++)
    {
      if (FD_ISSET(j, &readSet))
      {
        #if defined(TEST) || defined(INFO)
        *logofs << "[" << j << "]" << logofs_flush;
        #endif

        i++;
      }
    }

    if (i > 0)
    {
      #if defined(TEST) || defined(INFO)
      *logofs << ".\n" << logofs_flush;
      #endif
    }
    else
    {
      #if defined(TEST) || defined(INFO)
      *logofs << "[none].\n" << logofs_flush;
      #endif
    }

    i = 0;

    #if defined(TEST) || defined(INFO)
    *logofs << "Loop: Selected for write are ";
    #endif

    for (int j = 0; j < setFDs; j++)
    {
      if (FD_ISSET(j, &writeSet))
      {
        #if defined(TEST) || defined(INFO)
        *logofs << "[" << j << "]" << logofs_flush;
        #endif

        i++;
      }
    }

    if (i > 0)
    {
      #if defined(TEST) || defined(INFO)
      *logofs << ".\n" << logofs_flush;
      #endif
    }
    else
    {
      #if defined(TEST) || defined(INFO)
      *logofs << "[none].\n" << logofs_flush;
      #endif
    }
  }

  #if defined(TEST) || defined(INFO)
  *logofs << "Loop: Select timeout is "
          << selectTs.tv_sec << " S and "
          << (double) selectTs.tv_usec / 1000
          << " Ms.\n" << logofs_flush;
  #endif
}

static void handleCheckResultInLoop(int &resultFDs, int &errorFDs, int &setFDs, fd_set &readSet,
                                        fd_set &writeSet, struct timeval &selectTs,
                                            struct timeval &startTs)
{
  int diffTs = diffTimestamp(startTs, getNewTimestamp());

  #if defined(TEST) || defined(INFO)

  if (diffTs >= (control -> PingTimeout -
                     (control -> LatencyTimeout * 4)))
  {
    *logofs << "Loop: Select result is [" << resultFDs
            << "] at " << strMsTimestamp() << " with no "
            << "communication within " << diffTs
            << " Ms.\n" << logofs_flush;
  }
  else
  {
    *logofs << "Loop: Select result is [" << resultFDs
            << "] error is [" << errorFDs << "] at "
            << strMsTimestamp() << " after " << diffTs
            << " Ms.\n" << logofs_flush;
  }

  #endif

  int i;

  if (resultFDs > 0)
  {
    i = 0;

    #if defined(TEST) || defined(INFO)
    *logofs << "Loop: Selected for read are ";
    #endif

    for (int j = 0; j < setFDs; j++)
    {
      if (FD_ISSET(j, &readSet))
      {
        #if defined(TEST) || defined(INFO)
        *logofs << "[" << j << "]" << logofs_flush;
        #endif

        i++;
      }
    }

    if (i > 0)
    {
      #if defined(TEST) || defined(INFO)
      *logofs << ".\n" << logofs_flush;
      #endif
    }
    else
    {
      #if defined(TEST) || defined(INFO)
      *logofs << "[none].\n" << logofs_flush;
      #endif
    }

    i = 0;

    #if defined(TEST) || defined(INFO)
    *logofs << "Loop: Selected for write are ";
    #endif

    for (int j = 0; j < setFDs; j++)
    {
      if (FD_ISSET(j, &writeSet))
      {
        #if defined(TEST) || defined(INFO)
        *logofs << "[" << j << "]" << logofs_flush;
        #endif

        i++;
      }
    }

    if (i > 0)
    {
      #if defined(TEST) || defined(INFO)
      *logofs << ".\n" << logofs_flush;
      #endif
    }
    else
    {
      #if defined(TEST) || defined(INFO)
      *logofs << "[none].\n" << logofs_flush;
      #endif
    }
  }
}

#endif

static void handleCheckSessionInConnect()
{
  #ifdef TEST
  *logofs << "Loop: Going to check session in connect.\n"
          << logofs_flush;
  #endif

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
      #ifdef TEST
      *logofs << "Loop: Going to request proxy statistics "
              << "with signal '" << DumpSignal(lastSignal)
              << "'.\n" << logofs_flush;
      #endif

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
    #ifdef TEST
    *logofs << "Loop: Going to run a new negotiation loop "
            << "with stage " << control -> ProxyStage
            << " at " << strMsTimestamp() << ".\n"
            << logofs_flush;
    #endif

    switch (control -> ProxyStage)
    {
      case stage_undefined:
      {
        #ifdef TEST
        *logofs << "Loop: Handling negotiation with '"
                << "stage_undefined" << "'.\n"
                << logofs_flush;
        #endif

        control -> ProxyStage = stage_initializing;

        break;
      }
      case stage_initializing:
      {
        #ifdef TEST
        *logofs << "Loop: Handling negotiation with '"
                << "stage_initializing" << "'.\n"
                << logofs_flush;
        #endif

        InitBeforeNegotiation();

        control -> ProxyStage = stage_connecting;

        break;
      }
      case stage_connecting:
      {
        #ifdef TEST
        *logofs << "Loop: Handling negotiation with '"
                << "stage_connecting" << "'.\n"
                << logofs_flush;
        #endif

        SetupProxyConnection();

        control -> ProxyStage = stage_connected;

        break;
      }
      case stage_connected:
      {
        #ifdef TEST
        *logofs << "Loop: Handling negotiation with '"
                << "stage_connected" << "'.\n"
                << logofs_flush;
        #endif

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
        #ifdef TEST
        *logofs << "Loop: Handling negotiation with '"
                << "stage_sending_proxy_options" << "'.\n"
                << logofs_flush;
        #endif

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
        #ifdef TEST
        *logofs << "Loop: Handling negotiation with '"
                << "stage_waiting_forwarder_version" << "'.\n"
                << logofs_flush;
        #endif

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
        #ifdef TEST
        *logofs << "Loop: Handling negotiation with '"
                << "stage_waiting_forwarder_options" << "'.\n"
                << logofs_flush;
        #endif

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
        #ifdef TEST
        *logofs << "Loop: Handling negotiation with '"
                << "stage_waiting_proxy_version" << "'.\n"
                << logofs_flush;
        #endif

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
        #ifdef TEST
        *logofs << "Loop: Handling negotiation with '"
                << "stage_waiting_proxy_options" << "'.\n"
                << logofs_flush;
        #endif

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
        #ifdef TEST
        *logofs << "Loop: Handling negotiation with '"
                << "stage_sending_proxy_caches" << "'.\n"
                << logofs_flush;
        #endif

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
        #ifdef TEST
        *logofs << "Loop: Handling negotiation with '"
                << "stage_waiting_proxy_caches" << "'.\n"
                << logofs_flush;
        #endif

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
        #ifdef TEST
        *logofs << "Loop: Handling negotiation with '"
                << "stage_operational" << "'.\n"
                << logofs_flush;
        #endif

        InitAfterNegotiation();

        yield = 1;

        break;
      }
      default:
      {
        #ifdef PANIC
        *logofs << "Loop: PANIC! Unmanaged case '" << control -> ProxyStage
                << "' while handling negotiation.\n" << logofs_flush;
        #endif

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

  #ifdef TEST
  *logofs << "Loop: Selected proxy FD#" << proxyFD << " in negotiation "
          << "phase with timeout of " << selectTs.tv_sec << " S and "
          << selectTs.tv_usec << " Ms.\n" << logofs_flush;
  #endif

  return;

handleNegotiationInLoopError:

  #ifdef PANIC
  *logofs << "Loop: PANIC! Failure negotiating the session in stage '"
          << control -> ProxyStage << "'.\n" << logofs_flush;
  #endif

  cerr << "Error" << ": Failure negotiating the session in stage '"
       << control -> ProxyStage << "'.\n";


  if (control -> ProxyMode == proxy_server &&
          control -> ProxyStage == stage_waiting_proxy_version)
  {
    #ifdef PANIC
    *logofs << "Loop: PANIC! Wrong version or invalid session "
            << "authentication cookie.\n" << logofs_flush;
    #endif

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

  if (lastAlert.local == 0 &&
          (lastAlert.code > LAST_PROTO_STEP_6_ALERT &&
               control -> isProtoStep7() == 0))
  {
    //
    // The remote proxy would be unable
    // to handle the alert.
    //

    #ifdef WARNING
    *logofs << "Loop: WARNING! Ignoring unsupported alert "
            << "with code '" << lastAlert.code << "'.\n"
            << logofs_flush;
    #endif
  }
  else if (lastAlert.local == 0)
  {
    if (proxy != NULL)
    {
      #if defined(TEST) || defined(INFO)
      *logofs << "Loop: Requesting a remote alert with code '"
              << lastAlert.code << "'.\n" << logofs_flush;
      #endif

      if (proxy -> handleAlert(lastAlert.code) < 0)
      {
        HandleShutdown();
      }
    }
  }
  else
  {
    #if defined(TEST) || defined(INFO)
    *logofs << "Loop: Handling a local alert with code '"
            << lastAlert.code << "'.\n" << logofs_flush;
    #endif

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
            #ifdef WARNING
            *logofs << "Loop: WARNING! An unrecognized alert type '"
                    << lastAlert.code << "' was requested.\n"
                    << logofs_flush;
            #endif

            cerr << "Warning" << ": An unrecognized alert type '"
                 << lastAlert.code << "' was requested.\n";
          }
          #ifdef WARNING
          else
          {
            *logofs << "Loop: WARNING! Ignoring obsolete alert type '"
                    << lastAlert.code << "'.\n" << logofs_flush;
          }
          #endif

          message = NULL;
          type    = NULL;

          replace = 0;

          break;
        }
      }

      if (replace == 1 && IsRunning(lastDialog))
      {
        #if defined(TEST) || defined(INFO)
        *logofs << "Loop: Killing the previous dialog with pid '"
                << lastDialog << "'.\n" << logofs_flush;
        #endif

        //
        // The client ignores the TERM signal
        // on Windows.
        //

        #ifdef __CYGWIN32__

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
          #ifdef PANIC
          *logofs << "Loop: PANIC! Can't start the NX dialog process.\n"
                  << logofs_flush;
          #endif

          SetNotRunning(lastDialog);
        }
        #if defined(TEST) || defined(INFO)
        else
        {
          *logofs << "Loop: Dialog started with pid '"
                  << lastDialog << "'.\n" << logofs_flush;
        }
        #endif
      }
      #if defined(TEST) || defined(INFO)
      else
      {
        *logofs << "Loop: No new dialog required for code '"
                << lastAlert.code << "'.\n" << logofs_flush;
      }
      #endif
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
  #ifdef TEST
  *logofs << "Loop: Preparing the masks for the agent descriptors.\n"
          << logofs_flush;
  #endif

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
      #ifdef TEST
      *logofs << "Loop: Setting a null timeout with agent descriptors ready.\n"
              << logofs_flush;
      #endif

      //
      // Force a null timeout so we'll bail out
      // of the select immediately. We will ac-
      // comodate the result code later.
      //

      selectTs.tv_sec  = 0;
      selectTs.tv_usec = 0;
    }
  }

  #ifdef TEST
  *logofs << "Loop: Clearing the read and write agent descriptors.\n"
          << logofs_flush;
  #endif

  agent -> clearReadMask(&readSet);
  agent -> clearWriteMask(&writeSet);
}

static inline void handleAgentInLoop(int &resultFDs, int &errorFDs, int &setFDs, fd_set &readSet,
                                         fd_set &writeSet, struct timeval &selectTs)
{
  #if defined(TEST) || defined(INFO)
  *logofs << "Loop: Setting proxy and local agent descriptors.\n"
          << logofs_flush;
  #endif

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

    #if defined(TEST) || defined(INFO)
    *logofs << "Loop: Values were resultFDs " << resultFDs
            << " errorFDs " << errorFDs << " setFDs "
            << setFDs << ".\n" << logofs_flush;
    #endif

    if (agent -> localCanRead() == 1)
    {
      #if defined(TEST) || defined(INFO)
      *logofs << "Loop: Setting agent descriptor FD#" << agent ->
                 getLocalFd() << " as ready to read.\n"
              << logofs_flush;
      #endif

      agent -> setLocalRead(&readSet, &resultFDs);
    }

    #if defined(TEST) || defined(INFO)

    if (agent -> proxyCanRead(&readSet) == 0 &&
            agent -> proxyCanRead() == 1)
    {
      *logofs << "Loop: WARNING! Can read from proxy FD#"
              << proxyFD << " but the descriptor "
              << "is not selected.\n" << logofs_flush;
    }

    if (agent -> proxyCanRead(&readSet) == 1)
    {
      *logofs << "Loop: Setting proxy descriptor FD#" << agent ->
                 getProxyFd() << " as ready to read.\n"
              << logofs_flush;
    }

    #endif

    #if defined(TEST) || defined(INFO)
    *logofs << "Loop: Values are now resultFDs " << resultFDs
            << " errorFDs " << errorFDs << " setFDs "
            << setFDs << ".\n" << logofs_flush;
    #endif
  }
}

static inline void handleAgentLateInLoop(int &resultFDs, int &errorFDs, int &setFDs, fd_set &readSet,
                                             fd_set &writeSet, struct timeval &selectTs)
{
  #if defined(TEST) || defined(INFO)
  *logofs << "Loop: Setting remote agent descriptors.\n"
          << logofs_flush;
  #endif

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

    #if defined(TEST) || defined(INFO)
    *logofs << "Loop: Values were resultFDs " << resultFDs
            << " errorFDs " << errorFDs << " setFDs "
            << setFDs << ".\n" << logofs_flush;
    #endif

    if (agent -> remoteCanRead(agent ->
            getSavedReadMask()) == 1)
    {
      #if defined(TEST) || defined(INFO)
      *logofs << "Loop: Setting agent descriptor FD#" << agent ->
                 getRemoteFd() << " as ready to read.\n"
              << logofs_flush;
      #endif

      agent -> setRemoteRead(&readSet, &resultFDs);
    }

    if (agent -> remoteCanWrite(agent ->
            getSavedWriteMask()) == 1)
    {
      #if defined(TEST) || defined(INFO)
      *logofs << "Loop: Setting agent descriptor FD#" << agent ->
                 getRemoteFd() << " as ready to write.\n"
              << logofs_flush;
      #endif

      agent -> setRemoteWrite(&writeSet, &resultFDs);
    }

    #if defined(TEST) || defined(INFO)
    *logofs << "Loop: Values are now resultFDs " << resultFDs
            << " errorFDs " << errorFDs << " setFDs "
            << setFDs << ".\n" << logofs_flush;
    #endif
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
          #ifdef PANIC
          *logofs << "Loop: PANIC! Error creating new " << label
                  << " connection.\n" << logofs_flush;
          #endif

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

          #ifdef TEST
          *logofs << "Loop: Trying to read immediately "
                  << "from descriptor FD#" << newFD
                  << ".\n" << logofs_flush;
          #endif

          FD_SET(newFD, &readSet);

          resultFDs++;
        }
        #ifdef TEST
        else
        {
          *logofs << "Loop: Nothing to read immediately "
                  << "from descriptor FD#" << newFD
                  << ".\n" << logofs_flush;
        }
        #endif
      }
    }
  }

  #ifdef DEBUG
  *logofs << "Loop: Going to check the readable descriptors.\n"
          << logofs_flush;
  #endif

  if (proxy -> handleRead(resultFDs, readSet) < 0)
  {
    #ifdef TEST
    *logofs << "Loop: Failure reading from descriptors "
            << "for proxy FD#" << proxyFD << ".\n"
            << logofs_flush;
    #endif

    HandleShutdown();
  }
}

static inline void handleWritableInLoop(int &resultFDs, fd_set &writeSet)
{
  #ifdef DEBUG
  *logofs << "Loop: Going to check the writable descriptors.\n"
          << logofs_flush;
  #endif

  if (resultFDs > 0 && proxy -> handleFlush(resultFDs, writeSet) < 0)
  {
    #ifdef TEST
    *logofs << "Loop: Failure writing to descriptors "
            << "for proxy FD#" << proxyFD << ".\n"
            << logofs_flush;
    #endif

    HandleShutdown();
  }
}

static inline void handleFlushInLoop()
{
  #ifdef DEBUG
  *logofs << "Loop: Going to flush any data to the proxy.\n"
          << logofs_flush;
  #endif

  if (agent == NULL || control ->
          FlushPolicy == policy_immediate)
  {
    #if defined(TEST) || defined(INFO)

    if (usePolicy == -1 && control ->
            ProxyMode == proxy_client)
    {
      *logofs << "Loop: WARNING! Flushing the proxy link "
              << "on behalf of the agent.\n" << logofs_flush;
    }

    #endif

    if (proxy -> handleFlush() < 0)
    {
      #ifdef TEST
      *logofs << "Loop: Failure flushing the proxy FD#"
              << proxyFD << ".\n" << logofs_flush;
      #endif

      HandleShutdown();
    }
  }
}

static inline void handleRotateInLoop()
{
  #ifdef DEBUG
  *logofs << "Loop: Going to rotate channels "
          << "for proxy FD#" << proxyFD << ".\n"
          << logofs_flush;
  #endif

  proxy -> handleRotate();
}

static inline void handleEventsInLoop()
{
  #ifdef DEBUG
  *logofs << "Loop: Going to check channel events "
          << "for proxy FD#" << proxyFD << ".\n"
          << logofs_flush;
  #endif

  if (proxy -> handleEvents() < 0)
  {
    #ifdef TEST
    *logofs << "Loop: Failure handling channel events "
            << "for proxy FD#" << proxyFD << ".\n"
            << logofs_flush;
    #endif

    HandleShutdown();
  }
}

static void handleLogReopenInLoop(T_timestamp &logsTs, T_timestamp &nowTs)
{
  //
  // If need to limit the size of the
  // log file, check the size at each
  // loop.
  //

  #ifndef QUOTA

  if (diffTimestamp(logsTs, nowTs) > control -> FileSizeCheckTimeout)

  #endif
  {
    #ifdef DEBUG
    *logofs << "Loop: Checking size of log file '"
            << errorsFileName << "'.\n" << logofs_flush;
    #endif

    #ifndef MIXED

    if (ReopenLogFile(errorsFileName, logofs, control -> FileSizeLimit) < 0)
    {
      HandleShutdown();
    }

    #endif

    //
    // Reset to current timestamp.
    //

    logsTs = nowTs;
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

      #ifdef DEBUG
      *logofs << "Loop: Selected listener tcpFD " << tcpFD
              << " with setFDs " << setFDs << ".\n"
              << logofs_flush;
      #endif
    }

    if (useUnixSocket == 1)
    {
      FD_SET(unixFD, &readSet);

      if (unixFD >= setFDs)
      {
        setFDs = unixFD + 1;
      }

      #ifdef DEBUG
      *logofs << "Loop: Selected listener unixFD " << unixFD
              << " with setFDs " << setFDs << ".\n"
              << logofs_flush;
      #endif
    }

    if (useCupsSocket == 1)
    {
      FD_SET(cupsFD, &readSet);

      if (cupsFD >= setFDs)
      {
        setFDs = cupsFD + 1;
      }

      #ifdef DEBUG
      *logofs << "Loop: Selected listener cupsFD " << cupsFD
              << " with setFDs " << setFDs << ".\n"
              << logofs_flush;
      #endif
    }

    if (useAuxSocket == 1)
    {
      FD_SET(auxFD, &readSet);

      if (auxFD >= setFDs)
      {
        setFDs = auxFD + 1;
      }

      #ifdef DEBUG
      *logofs << "Loop: Selected listener auxFD " << auxFD
              << " with setFDs " << setFDs << ".\n"
              << logofs_flush;
      #endif
    }

    if (useSmbSocket == 1)
    {
      FD_SET(smbFD, &readSet);

      if (smbFD >= setFDs)
      {
        setFDs = smbFD + 1;
      }

      #ifdef DEBUG
      *logofs << "Loop: Selected listener smbFD " << smbFD
              << " with setFDs " << setFDs << ".\n"
              << logofs_flush;
      #endif
    }

    if (useMediaSocket == 1)
    {
      FD_SET(mediaFD, &readSet);

      if (mediaFD >= setFDs)
      {
        setFDs = mediaFD + 1;
      }

      #ifdef DEBUG
      *logofs << "Loop: Selected listener mediaFD " << mediaFD
              << " with setFDs " << setFDs << ".\n"
              << logofs_flush;
      #endif
    }

    if (useHttpSocket == 1)
    {
      FD_SET(httpFD, &readSet);

      if (httpFD >= setFDs)
      {
        setFDs = httpFD + 1;
      }

      #ifdef DEBUG
      *logofs << "Loop: Selected listener httpFD " << httpFD
              << " with setFDs " << setFDs << ".\n"
              << logofs_flush;
      #endif
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

      #ifdef DEBUG
      *logofs << "Loop: Selected listener fontFD " << fontFD
              << " with setFDs " << setFDs << ".\n"
              << logofs_flush;
      #endif
    }
  }

  if (useSlaveSocket == 1)
  {
    FD_SET(slaveFD, &readSet);

    if (slaveFD >= setFDs)
    {
      setFDs = slaveFD + 1;
    }

    #ifdef DEBUG
    *logofs << "Loop: Selected listener slaveFD " << slaveFD
            << " with setFDs " << setFDs << ".\n"
            << logofs_flush;
    #endif
  }
}
