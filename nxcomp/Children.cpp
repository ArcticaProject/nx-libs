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

#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <fcntl.h>

#include "NX.h"

#include "Misc.h"

#include "Types.h"
#include "Timestamp.h"

#include "Control.h"
#include "Statistics.h"
#include "Proxy.h"

#include "Keeper.h"
#include "Fork.h"

//
// Set the verbosity level.
//

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

#define DISPLAY_LENGTH_LIMIT  256
#define DEFAULT_STRING_LIMIT  512

//
// These are from the main loop.
//

extern Keeper *keeper;

extern int (*handler)(int);

extern int useUnixSocket;

extern int lastDialog;
extern int lastWatchdog;
extern int lastKeeper;

extern void CleanupListeners();
extern void CleanupSockets();
extern void CleanupAgent();
extern void CleanupGlobal();

extern void InstallSignals();

extern char *GetClientPath();

extern int CheckParent(const char *name, const char *type,
                           int parent);

#ifdef __sun
extern char **environ;
#endif

//
// Close all the unused descriptors and
// install any signal handler that might
// have been disabled in the main process.
//

static void SystemCleanup(const char *name);

//
// Release all objects allocated in the
// heap.

static void MemoryCleanup(const char *name);

//
// Remove 'name' from the environment.
//

static int UnsetEnv(const char *name);

static int NXTransKeeperHandler(int signal);
static void NXTransKeeperCheck();


//
// Start a nxclient process in dialog mode.
//

int NXTransDialog(const char *caption, const char *message,
                      const char *window, const char *type, int local,
                         const char* display)
{
  //
  // Be sure log file is valid.
  //

  if (logofs == NULL)
  {
    logofs = &cerr;
  }

  int pid;

  #ifdef TEST
  *logofs << "NXTransDialog: Going to fork with NX pid '"
          << getpid() << "'.\n" << logofs_flush;
  #endif

  pid = Fork();

  if (pid != 0)
  {
    if (pid < 0)
    {
      #ifdef TEST
      *logofs << "NXTransDialog: WARNING! Function fork failed. "
              << "Error is " << EGET() << " '" << ESTR()
              << "'.\n" << logofs_flush;
      #endif

      cerr << "Warning" << ": Function fork failed. "
           << "Error is " << EGET() << " '" << ESTR()
           << "'.\n";
    }
    #ifdef TEST
    else
    {
      *logofs << "NXTransDialog: Created NX dialog process "
              << "with pid '" << pid << "'.\n"
              << logofs_flush;
    }
    #endif

    return pid;
  }

  #ifdef TEST
  *logofs << "NXTransDialog: Executing child with pid '"
          << getpid() << "' and parent '" << getppid()
          << "'.\n" << logofs_flush;
  #endif

  SystemCleanup("NXTransDialog");

  //
  // Copy the client command before
  // freeing up the control class.
  //

  char command[DEFAULT_STRING_LIMIT];

  if (control != NULL)
  {
    strcpy(command, control -> ClientPath);
  }
  else
  {
    char *path = GetClientPath();

    strcpy(command, path);

    delete [] path;
  }

  //
  // Get rid of the unused resources.
  //

  MemoryCleanup("NXTransDialog");

  #ifdef TEST
  *logofs << "NXTransDialog: Running external NX dialog with caption '"
          << caption << "' message '" << message << "' type '"
          << type << "' local '" << local << "' display '"
          << display << "'.\n"
          << logofs_flush;
  #endif

  int pulldown = (strcmp(type, "pulldown") == 0);

  char parent[DEFAULT_STRING_LIMIT];

  snprintf(parent, DEFAULT_STRING_LIMIT, "%d", getppid());

  parent[DEFAULT_STRING_LIMIT - 1] = '\0';

  UnsetEnv("LD_LIBRARY_PATH");

  for (int i = 0; i < 2; i++)
  {
    if (local != 0)
    {
      if (pulldown)
      {
        execlp(command, command, "--dialog", type, "--caption", caption,
                   "--window", window, "--local", "--parent", parent,
                       "--display", display, NULL);
      }
      else
      {
        execlp(command, command, "--dialog", type, "--caption", caption,
                   "--message", message, "--local", "--parent", parent,
                       "--display", display, NULL);
      }
    }
    else
    {
      if (pulldown)
      {
        execlp(command, command, "--dialog", type, "--caption", caption,
                   "--window", window, "--parent", parent,
                       "--display", display, NULL);
      }
      else
      {
        execlp(command, command, "--dialog", type, "--caption", caption,
                   "--message", message, "--parent", parent,
                       "--display", display, NULL);
      }
    }

    #ifdef WARNING
    *logofs << "NXTransDialog: WARNING! Couldn't start '"
            << command << "'. " << "Error is " << EGET()
            << " '" << ESTR() << "'.\n" << logofs_flush;
    #endif

    cerr << "Warning" << ": Couldn't start '" << command
         << "'. Error is " << EGET() << " '" << ESTR()
         << "'.\n";

    //
    // Retry by looking for the default name
    // in the default NX path.
    //

    strcpy(command, "nxclient");

    char newPath[DEFAULT_STRING_LIMIT];

    strcpy(newPath, "/usr/NX/bin:/opt/NX/bin:/usr/local/NX/bin:");

    #ifdef __APPLE__

    strcat(newPath, "/Applications/NX Client for OSX.app/Contents/MacOS:");

    #endif

    #ifdef __CYGWIN32__

    strcat(newPath, ".:");

    #endif

    int newLength = strlen(newPath);

    char *oldPath = getenv("PATH");

    strncpy(newPath + newLength, oldPath, DEFAULT_STRING_LIMIT - newLength - 1);

    newPath[DEFAULT_STRING_LIMIT - 1] = '\0';

    #ifdef WARNING
    *logofs << "NXTransDialog: WARNING! Trying with path '"
            << newPath << "'.\n" << logofs_flush;
    #endif

    cerr << "Warning" << ": Trying with path '" << newPath
         << "'.\n";

    //
    // Solaris doesn't seem to have
    // function setenv().
    //

    #ifdef __sun

    char newEnv[DEFAULT_STRING_LIMIT + 5];

    sprintf(newEnv,"PATH=%s", newPath);

    putenv(newEnv);

    #else

    setenv("PATH", newPath, 1);

    #endif
  }

  //
  // Hopefully useless.
  //

  exit(0);
}

//
// Start a nxclient process in dialog mode.
//

int NXTransClient(const char* display)
{
  //
  // Be sure log file is valid.
  //

  if (logofs == NULL)
  {
    logofs = &cerr;
  }

  int pid;

  #ifdef TEST
  *logofs << "NXTransClient: Going to fork with NX pid '"
          << getpid() << "'.\n" << logofs_flush;
  #endif

  pid = Fork();

  if (pid != 0)
  {
    if (pid < 0)
    {
      #ifdef TEST
      *logofs << "NXTransClient: WARNING! Function fork failed. "
              << "Error is " << EGET() << " '" << ESTR()
              << "'.\n" << logofs_flush;
      #endif

      cerr << "Warning" << ": Function fork failed. "
           << "Error is " << EGET() << " '" << ESTR()
           << "'.\n";
    }
    #ifdef TEST
    else
    {
      *logofs << "NXTransClient: Created NX client process "
              << "with pid '" << pid << "'.\n"
              << logofs_flush;
    }
    #endif

    return pid;
  }

  #ifdef TEST
  *logofs << "NXTransClient: Executing child with pid '"
          << getpid() << "' and parent '" << getppid()
          << "'.\n" << logofs_flush;
  #endif

  SystemCleanup("NXTransClient");

  //
  // Copy the client command before
  // freeing up the control class.
  //

  char command[DEFAULT_STRING_LIMIT];

  if (control != NULL)
  {
    strcpy(command, control -> ClientPath);
  }
  else
  {
    char *path = GetClientPath();

    strcpy(command, path);

    delete [] path;
  }

  //
  // Get rid of unused resources.
  //

  MemoryCleanup("NXTransClient");

  #ifdef TEST
  *logofs << "NXTransClient: Running external NX client with display '"
          << display << "'.\n" << logofs_flush;
  #endif

  //
  // Provide the display in the environment.
  //

  char newDisplay[DISPLAY_LENGTH_LIMIT];

  #ifdef __sun

  snprintf(newDisplay, DISPLAY_LENGTH_LIMIT - 1, "DISPLAY=%s", display);

  newDisplay[DISPLAY_LENGTH_LIMIT - 1] = '\0';

  putenv(newDisplay);

  #else

  strncpy(newDisplay, display, DISPLAY_LENGTH_LIMIT - 1);

  newDisplay[DISPLAY_LENGTH_LIMIT - 1] = '\0';

  setenv("DISPLAY", newDisplay, 1);

  #endif

  UnsetEnv("LD_LIBRARY_PATH");

  for (int i = 0; i < 2; i++)
  {
    execlp(command, command, NULL);

    #ifdef WARNING
    *logofs << "NXTransClient: WARNING! Couldn't start '"
            << command << "'. Error is " << EGET() << " '"
            << ESTR() << "'.\n" << logofs_flush;
    #endif

    cerr << "Warning" << ": Couldn't start '" << command
         << "'. Error is " << EGET() << " '" << ESTR()
         << "'.\n";

    //
    // Retry by looking for the default name
    // in the default NX path.
    //

    strcpy(command, "nxclient");

    char newPath[DEFAULT_STRING_LIMIT];

    strcpy(newPath, "/usr/NX/bin:/opt/NX/bin:/usr/local/NX/bin:");

    #ifdef __APPLE__

    strcat(newPath, "/Applications/NX Client for OSX.app/Contents/MacOS:");

    #endif

    #ifdef __CYGWIN32__

    strcat(newPath, ".:");

    #endif

    int newLength = strlen(newPath);

    char *oldPath = getenv("PATH");

    strncpy(newPath + newLength, oldPath, DEFAULT_STRING_LIMIT - newLength - 1);

    newPath[DEFAULT_STRING_LIMIT - 1] = '\0';

    #ifdef WARNING
    *logofs << "NXTransClient: WARNING! Trying with path '"
            << newPath << "'.\n" << logofs_flush;
    #endif

    cerr << "Warning" << ": Trying with path '" << newPath
         << "'.\n";

    //
    // Solaris doesn't seem to have
    // function setenv().
    //

    #ifdef __sun

    char newEnv[DEFAULT_STRING_LIMIT + 5];

    sprintf(newEnv,"PATH=%s", newPath);

    putenv(newEnv);

    #else

    setenv("PATH", newPath, 1);

    #endif
  }

  //
  // Hopefully useless.
  //

  exit(0);
}

//
// Wait until the timeout is expired.
// The timeout is expressed in milli-
// seconds.
//

int NXTransWatchdog(int timeout)
{
  //
  // Be sure log file is valid.
  //

  if (logofs == NULL)
  {
    logofs = &cerr;
  }

  int pid;

  #ifdef TEST
  *logofs << "NXTransWatchdog: Going to fork with NX pid '"
          << getpid() << "'.\n" << logofs_flush;
  #endif

  pid = Fork();

  if (pid != 0)
  {
    if (pid < 0)
    {
      #ifdef TEST
      *logofs << "NXTransWatchdog: WARNING! Function fork failed. "
              << "Error is " << EGET() << " '" << ESTR()
              << "'.\n" << logofs_flush;
      #endif

      cerr << "Warning" << ": Function fork failed. "
           << "Error is " << EGET() << " '" << ESTR()
           << "'.\n";
    }
    #ifdef TEST
    else
    {
      *logofs << "NXTransWatchdog: Created NX watchdog process "
              << "with pid '" << pid << "'.\n" << logofs_flush;
    }
    #endif

    return pid;
  }

  int parent = getppid();

  #ifdef TEST
  *logofs << "NXTransWatchdog: Executing child with pid '"
          << getpid() << "' and parent '" << parent
          << "'.\n" << logofs_flush;
  #endif

  SystemCleanup("NXTransWatchdog");

  //
  // Get rid of unused resources.
  //

  MemoryCleanup("NXTransWatchdog");

  //
  // Run until the timeout is expired
  // or forever, if no timeout is
  // provided.
  //

  T_timestamp startTs = getTimestamp();

  int diffTs = 0;

  for (;;)
  {
    //
    // Complain if the parent is dead.
    //

    if (CheckParent("NXTransWatchdog", "watchdog", parent) == 0)
    {
      #ifdef TEST
      *logofs << "NXTransWatchdog: Exiting with no parent "
              << "running.\n" << logofs_flush;
      #endif

      HandleCleanup();
    }

    if (timeout > 0)
    {
      if (diffTs >= timeout)
      {
        #ifdef TEST
        *logofs << "NXTransWatchdog: Timeout of " << timeout
                << " Ms raised in watchdog.\n" << logofs_flush;
        #endif

        //
        // We will just exit. Our parent should be
        // monitoring us and detect that the process
        // is gone.
        //

        HandleCleanup();
      }
    }

    if (timeout > 0)
    {
      #ifdef TEST
      *logofs << "NXTransWatchdog: Waiting for the timeout "
              << "with " << timeout - diffTs << " Ms to run.\n"
              << logofs_flush;
      #endif

      usleep((timeout - diffTs) * 1000);

      diffTs = diffTimestamp(startTs, getNewTimestamp());
    }
    else
    {
      #ifdef TEST
      *logofs << "NXTransWatchdog: Waiting for a signal.\n"
              << logofs_flush;
      #endif

      sleep(10);
    }
  }

  //
  // Hopefully useless.
  //

  exit(0);
}

int NXTransKeeperHandler(int signal)
{
  if (keeper != NULL)
  {
    switch (signal)
    {
      case SIGTERM:
      case SIGINT:
      case SIGHUP:
      {
        #ifdef TEST
        *logofs << "NXTransKeeperHandler: Requesting giveup "
                << "because of signal " << signal << " ,'"
                << DumpSignal(signal) << "'.\n"
                << logofs_flush;
        #endif

        keeper -> setSignal(signal);

        return 0;
      }
    }
  }

  return 1;
}

void NXTransKeeperCheck()
{
  if (CheckParent("NXTransKeeper", "keeper",
          keeper -> getParent()) == 0 || keeper -> getSignal() != 0)
  {
    #ifdef TEST
    *logofs << "NXTransKeeperCheck: Exiting because of signal "
            << "or no parent running.\n" << logofs_flush;
    #endif

    HandleCleanup();
  }
}

int NXTransKeeper(int caches, int images, const char *root)
{
  //
  // Be sure log file is valid.
  //

  if (logofs == NULL)
  {
    logofs = &cerr;
  }

  if (caches == 0 && images == 0)
  {
    #ifdef TEST
    *logofs << "NXTransKeeper: No NX cache house-keeping needed.\n"
            << logofs_flush;
    #endif

    return 0;
  }

  int pid;

  #ifdef TEST
  *logofs << "NXTransKeeper: Going to fork with NX pid '"
          << getpid() << "'.\n" << logofs_flush;
  #endif

  pid = Fork();

  if (pid != 0)
  {
    if (pid < 0)
    {
      #ifdef TEST
      *logofs << "NXTransKeeper: WARNING! Function fork failed. "
              << "Error is " << EGET() << " '" << ESTR()
              << "'.\n" << logofs_flush;
      #endif

      cerr << "Warning" << ": Function fork failed. "
           << "Error is " << EGET() << " '" << ESTR()
           << "'.\n";
    }
    #ifdef TEST
    else
    {
      *logofs << "NXTransKeeper: Created NX keeper process "
              << "with pid '" << pid << "'.\n"
              << logofs_flush;
    }
    #endif

    return pid;
  }

  int parent = getppid();

  #ifdef TEST
  *logofs << "NXTransKeeper: Executing child with pid '"
          << getpid() << "' and parent '" << parent
          << "'.\n" << logofs_flush;
  #endif

  SystemCleanup("NXTransKeeper");

  #ifdef TEST
  *logofs << "NXTransKeeper: Going to run with caches " << caches
          << " images " << images << " and root " << root
          << " at " << strMsTimestamp() << ".\n" << logofs_flush;
  #endif

  //
  // Create the house-keeper class.
  //

  int timeout = control -> KeeperTimeout;

  keeper = new Keeper(caches, images, root, 100, parent);

  handler = NXTransKeeperHandler;

  if (keeper == NULL)
  {
    #ifdef PANIC
    *logofs << "NXTransKeeper: PANIC! Failed to create the keeper object.\n"
            << logofs_flush;
     #endif

     cerr << "Error" << ": Failed to create the keeper object.\n";

     HandleCleanup();
  }

  //
  // Get rid of unused resources. Root path
  // must be copied in keeper's constructor
  // before control is deleted.
  //
  
  MemoryCleanup("NXTransKeeper");

  //
  // Decrease the priority of this process.
  //
  // The following applies to Cygwin: "Cygwin processes can be 
  // set to IDLE_PRIORITY_CLASS, NORMAL_PRIORITY_CLASS, HIGH_-
  // PRIORITY_CLASS, or REALTIME_PRIORITY_CLASS with the nice
  // call. If you pass a positive number to nice(), then the
  // priority level will decrease by one (within the above list
  // of priorities). A negative number would make it increase
  // by one. It is not possible to change it by more than one
  // at a time without making repeated calls".
  //

  if (nice(5) < 0 && errno != 0)
  {
    #ifdef WARNING
    *logofs << "NXTransKeeper: WARNING! Failed to renice process to +5. "
            << "Error is " << EGET() << " '" << ESTR()
            << "'.\n" << logofs_flush;
     #endif

     cerr << "Warning" << ": Failed to renice process to +5. "
          << "Error is " << EGET() << " '" << ESTR()
          << "'.\n";
  }

  //
  // Delay a bit the first run to give
  // a boost to the session startup.
  //

  #ifdef TEST
  *logofs << "NXTransKeeper: Going to sleep for "
          << timeout / 20 << " Ms.\n" << logofs_flush;
  #endif

  usleep(timeout / 20 * 1000);

  NXTransKeeperCheck();

  //
  // The house keeping of the persistent
  // caches is performed only once.
  //

  if (caches != 0)
  {
    #ifdef TEST
    *logofs << "NXTransKeeper: Going to cleanup the NX cache "
            << "directories at " << strMsTimestamp() << ".\n"
            << logofs_flush;
    #endif

    keeper -> cleanupCaches();

    #ifdef TEST
    *logofs << "NXTransKeeper: Completed cleanup of NX cache "
            << "directories at " << strMsTimestamp() << ".\n"
            << logofs_flush;
    #endif
  }
  #ifdef TEST
  else
  {
    *logofs << "NXTransKeeper: Nothing to do for the "
            << "persistent caches.\n" << logofs_flush;
  }
  #endif

  if (images == 0)
  {
    #ifdef TEST
    *logofs << "NXTransKeeper: Nothing to do for the "
            << "persistent images.\n" << logofs_flush;
    #endif

    HandleCleanup();
  }

  //
  // Take care of the persisten image cache.
  // Run a number of iterations and then exit,
  // so we can keep the memory consumption
  // low. The parent will check our exit code
  // and will eventually restart us.
  //

  for (int iterations = 0; iterations < 100; iterations++)
  {
    #ifdef TEST
    *logofs << "NXTransKeeper: Running iteration " << iterations
            << " at " << strMsTimestamp() << ".\n"
            << logofs_flush;
    #endif

    NXTransKeeperCheck();

    #ifdef TEST
    *logofs << "NXTransKeeper: Going to cleanup the NX images "
            << "directories at " << strMsTimestamp() << ".\n"
            << logofs_flush;
    #endif

    if (keeper -> cleanupImages() < 0)
    {
      #ifdef TEST
      *logofs << "NXTransKeeper: Exiting because of error "
              << "handling the image cache.\n" << logofs_flush;
      #endif

      HandleCleanup();
    }

    #ifdef TEST
    *logofs << "NXTransKeeper: Completed cleanup of NX images "
            << "directories at " << strMsTimestamp() << ".\n"
            << logofs_flush;
    #endif

    NXTransKeeperCheck();

    #ifdef TEST
    *logofs << "NXTransKeeper: Going to sleep for " << timeout
            << " Ms.\n" << logofs_flush;
    #endif

    usleep(timeout * 1000);
  }

  HandleCleanup(2);

  //
  // Hopefully useless.
  //

  exit(0);
}

void SystemCleanup(const char *name)
{
  #ifdef TEST
  *logofs << name << ": Performing system cleanup in process "
          << "with pid '" << getpid() << "'.\n"
          << logofs_flush;
  #endif

  //
  // Reinstall signals that might
  // have been restored by agents.
  //

  InstallSignals();
}

void MemoryCleanup(const char *name)
{
  #ifdef TEST
  *logofs << name << ": Performing memory cleanup in process "
          << "with pid '" << getpid() << "'.\n"
          << logofs_flush;
  #endif

  DisableSignals();

  //
  // Prevent deletion of unix socket
  // and lock file.
  //

  useUnixSocket = 0;

  //
  // Don't let cleanup kill other
  // children.
  //

  lastDialog   = 0;
  lastWatchdog = 0;
  lastKeeper   = 0;

  CleanupListeners();

  CleanupSockets();

  CleanupGlobal();

  EnableSignals();
}

int UnsetEnv(const char *name)
{
  int result;

  #ifdef __sun

  char **pEnv = environ;

  int nameLen = strlen(name) + 1;

  char *varName = new char[nameLen + 1];

  strcpy(varName, name);

  strcat(varName, "=");

  pEnv = environ;

  while (*pEnv != NULL)
  {
    if (!strncmp(varName, *pEnv, nameLen))
    {
      break;
    }

    *pEnv++;
  }

  while (*pEnv != NULL)
  {
    *pEnv = *(pEnv + 1);

    pEnv++;
  }

  result = 0;

  #else

  #ifdef __APPLE__

  unsetenv(name);
  result = 0;

  #else

  result = unsetenv(name);

  #endif

  #endif

  return result;
}
