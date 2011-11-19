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

#include <sys/types.h>
#include <sys/utsname.h>

#if defined(__CYGWIN32__) || defined(__APPLE__) || defined(__FreeBSD__) || defined(__sun)
#include <netinet/in_systm.h>
#endif

#ifdef __sun
#include <unistd.h>
#include <sys/termios.h>
#endif

#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#include <netdb.h>
#include <fcntl.h>

//
// System specific defines.
//

#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__sun)
#define SOL_IP  IPPROTO_IP
#endif

#ifdef __sun
#define INADDR_NONE  ((unsigned int) -1)
#endif

//
// The TIOCOUTQ ioctl is not implemented on Cygwin.
// Note also that TIOCOUTQ and IPTOS_LOWDELAY are
// disabled when running on MacOS/X.
//

#ifdef __CYGWIN32__
#define TIOCOUTQ  ((unsigned int) -1)
#endif

//
// NX includes.
//

#include "Misc.h"
#include "Socket.h"

//
// Set verbosity level.
//

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

//
// Set this only once by querying OS details.
//

static int _kernelStep = -1;

int GetKernelStep()
{
  if (_kernelStep < 0)
  {
    //
    // At the moment only NX clients run on Win32
    // and MacOS/X so we are not really interested
    // in the relevant OS dependent functions.
    //

    #if defined(__CYGWIN32__) || defined(__APPLE__)

    _kernelStep = 0;

    #else

    struct utsname buffer;

    if (uname(&buffer) < 0)
    {
      #ifdef WARNING
      *logofs << "Socket: WARNING! Failed to get system info. Error is "
              << EGET() << " '" << ESTR() << "'.\n" << logofs_flush;

      *logofs << "Socket: WARNING! Assuming lowest system support.\n"
              << logofs_flush;
      #endif

      cerr << "Warning" << ": Failed to get system info. Error is "
           << EGET() << " '" << ESTR() << "'.\n";

      cerr << "Warning" << ": Assuming lowest system support.\n";

      _kernelStep = 0;
    }
    else
    {
      #ifdef TEST
      *logofs << "Socket: System is '" << buffer.sysname
              << "' nodename '" << buffer.nodename << "' release '"
              << buffer.release << "'.\n" << logofs_flush;

      *logofs << "Socket: Version is '" << buffer.version << "' machine '"
              << buffer.machine << "'.\n" << logofs_flush;
      #endif

      //
      // Should test support on other operating systems.
      //

      if (strcmp(buffer.sysname, "Linux") == 0)
      {
        if (strncmp(buffer.release, "2.0.", 4) == 0 ||
                strncmp(buffer.release, "2.2.", 4) == 0)
        {
          #ifdef TEST
          *logofs << "Socket: Assuming level 2 system support.\n"
                  << logofs_flush;
          #endif

          _kernelStep = 2;
        }
        else
        {
          #ifdef TEST
          *logofs << "Socket: Assuming level 3 system support.\n"
                  << logofs_flush;
          #endif

          _kernelStep = 3;
        }
      }
      else if (strcmp(buffer.sysname, "SunOS") == 0)
      {
        #ifdef TEST
        *logofs << "Socket: Assuming level 1 system support.\n"
                << logofs_flush;
        #endif

        _kernelStep = 1;
      }
      else
      {
        #ifdef TEST
        *logofs << "Socket: Assuming level 0 system support.\n"
                << logofs_flush;
        #endif

        _kernelStep = 0;
      }
    }

    #endif /* #if defined(__CYGWIN32__) || defined(__APPLE__) */
  }

  return _kernelStep;
}
  
int SetReuseAddress(int fd)
{
  int flag = 1;

  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
                     (char *) &flag, sizeof(flag)) < 0)
  {
    #ifdef PANIC
    *logofs << "Socket: PANIC! Failed to set SO_REUSEADDR flag on FD#"
            << fd << ". Error is " << EGET() << " '" << ESTR() << "'.\n"
            << logofs_flush;
    #endif

    cerr << "Error" << ": Failed to set SO_REUSEADDR flag on FD#"
            << fd << ". Error is " << EGET() << " '" << ESTR()
            << "'.\n";

    return -1;
  }
  #ifdef TEST
  else
  {
    *logofs << "Socket: Set SO_REUSEADDR flag on FD#"
            << fd << ".\n" << logofs_flush;
  }
  #endif

  return 1;
}

int SetNonBlocking(int fd, int value)
{
  int flags = fcntl(fd, F_GETFL);

  if (flags >= 0)
  {
    if (value == 0)
    {
      flags &= ~O_NONBLOCK;
    }
    else
    {
      flags |= O_NONBLOCK;
    }
  }

  if (flags < 0 || fcntl(fd, F_SETFL, flags) < 0)
  {
    #ifdef PANIC
    *logofs << "Socket: PANIC! Failed to set O_NONBLOCK flag on FD#"
            << fd << " to " << value << ". Error is " << EGET()
            << " '" << ESTR() << "'.\n" << logofs_flush;
    #endif

    cerr << "Error" << ": Failed to set O_NONBLOCK flag on FD#"
         << fd << " to " << value << ". Error is " << EGET()
         << " '" << ESTR() << "'.\n";

    return -1;
  }
  #ifdef TEST
  else
  {
    *logofs << "Socket: Set O_NONBLOCK flag on FD#"
            << fd << " to " << value << ".\n"
            << logofs_flush;
  }
  #endif

  return 1;
}

int SetLingerTimeout(int fd, int timeout)
{
  struct linger linger_value;

  if (timeout > 0)
  {
    linger_value.l_onoff  = 1;
    linger_value.l_linger = timeout;
  }
  else
  {
    linger_value.l_onoff  = 0;
    linger_value.l_linger = 0;
  }

  if (setsockopt(fd, SOL_SOCKET, SO_LINGER, &linger_value, sizeof(linger_value)) < 0)
  {
    #ifdef PANIC
    *logofs << "Socket: PANIC! Failed to set SO_LINGER values to "
            << linger_value.l_onoff << " and " << linger_value.l_linger
            << " on FD#" << fd << ". Error is " << EGET() << " '"
            << ESTR() << "'.\n" << logofs_flush;
    #endif

    cerr << "Error" << ": Failed to set SO_LINGER values to "
         << linger_value.l_onoff << " and " << linger_value.l_linger
         << " on FD#" << fd << ". Error is " << EGET() << " '"
         << ESTR() << "'.\n";

    return -1;
  }
  #ifdef TEST
  else
  {
    *logofs << "Socket: Set SO_LINGER values to "
            << linger_value.l_onoff << " and " << linger_value.l_linger
            << " on FD#" << fd << ".\n" << logofs_flush;
  }
  #endif

  return 1;
}

int SetSendBuffer(int fd, int size)
{
  if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &size, sizeof(size)) < 0)
  {
    #ifdef PANIC
    *logofs << "Socket: PANIC! Failed to set SO_SNDBUF size to "
            << size << " on FD#" << fd << ". Error is "
            << EGET() << " '" << ESTR() << "'.\n"
            << logofs_flush;
    #endif

    cerr << "Error" << ": Failed to set SO_SNDBUF size to "
         << size << " on FD#" << fd << ". Error is "
         << EGET() << " '" << ESTR() << "'.\n";

    return -1;
  }
  #ifdef TEST
  else
  {
    *logofs << "Socket: Set SO_SNDBUF on FD#" << fd
            << " to " << size << " bytes.\n"
            << logofs_flush;
  }
  #endif

  return 1;
}

int SetReceiveBuffer(int fd, int size)
{
  if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size)) < 0)
  {
    #ifdef PANIC
    *logofs << "Socket: PANIC! Failed to set SO_RCVBUF size to "
            << size << " on FD#" << fd << ". Error is "
            << EGET() << " '" << ESTR() << "'.\n"
            << logofs_flush;
    #endif

    cerr << "Error" << ": Failed to set SO_RCVBUF size to "
         << size << " on FD#" << fd << ". Error is "
         << EGET() << " '" << ESTR() << "'.\n";

    return -1;
  }
  #ifdef TEST
  else
  {
    *logofs << "Socket: Set SO_RCVBUF on FD#" << fd
            << " to " << size << " bytes.\n"
            << logofs_flush;
  }
  #endif

  return 1;
}

int SetNoDelay(int fd, int value)
{
  int result = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &value, sizeof(value));

  if (result == 0)
  {
    result = 1;
  }
  else if (result < 0)
  {
    //
    // Is it become a different error on
    // Mac OSX 10.4?
    //

    #if defined(__APPLE__)

    result = 0;

    #endif

    #if defined(__sun)

    if (EGET() == ENOPROTOOPT)
    {
      result = 0;
    }

    #endif

    #if !defined(__APPLE__) && !defined(__sun)

    if (EGET() == EOPNOTSUPP)
    {
      result = 0;
    }

    #endif
  }

  if (result < 0)
  {
    #ifdef PANIC
    *logofs << "Socket: PANIC! Failed to set TCP_NODELAY flag on "
            << "FD#" << fd << " to " << value << ". Error is "
            << EGET() << " '" << ESTR() << "'.\n" << logofs_flush;
    #endif

    cerr << "Error" << ": Failed to set TCP_NODELAY flag on "
         << "FD#" << fd << " to " << value << ". Error is "
         << EGET() << " '" << ESTR() << "'.\n";
  }
  #ifdef TEST
  else if (result == 0)
  {
    #ifdef TEST
    *logofs << "Socket: Option TCP_NODELAY not supported "
            << "on FD#" << fd << ".\n" << logofs_flush;
    #endif
  }
  else
  {
    *logofs << "Socket: Set TCP_NODELAY flag on FD#"
            << fd << " to " << value << ".\n"
            << logofs_flush;
  }
  #endif

  return result;
}

int SetKeepAlive(int fd)
{
  int flag = 1;

  if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &flag, sizeof(flag)) < 0)
  {
    #ifdef PANIC
    *logofs << "Socket: PANIC! Failed to set SO_KEEPALIVE flag on "
            << "FD#" << fd << ". Error is " << EGET() << " '"
            << ESTR() << "'.\n" << logofs_flush;
    #endif

    cerr << "Error" << ": Failed to set SO_KEEPALIVE flag on "
         << "FD#" << fd << ". Error is " << EGET() << " '"
         << ESTR() << "'.\n";

    return -1;
  }
  #ifdef TEST
  else
  {
    *logofs << "Socket: Set SO_KEEPALIVE flag on FD#"
            << fd << ".\n" << logofs_flush;
  }
  #endif

  return 1;
}

int SetLowDelay(int fd)
{
  if (_kernelStep < 0)
  {
    GetKernelStep();
  }

  switch (_kernelStep)
  {
    case 3:
    case 2:
    case 1:
    {
      int flag = IPTOS_LOWDELAY;

      if (setsockopt(fd, SOL_IP, IP_TOS, &flag, sizeof(flag)) < 0)
      {
        if (EGET() == EOPNOTSUPP)
        {
          #ifdef TEST
          *logofs << "Socket: Option IPTOS_LOWDELAY not supported "
                  << "on FD#" << fd << ".\n" << logofs_flush;
          #endif

          return 0;
        }
        else
        {
          #ifdef WARNING
          *logofs << "Socket: WARNING! Failed to set IPTOS_LOWDELAY flag on "
                  << "FD#" << fd << ". Error is " << EGET() << " '" << ESTR()
                  << "'.\n" << logofs_flush;
          #endif

          cerr << "Warning" << ": Failed to set IPTOS_LOWDELAY flag on "
               << "FD#" << fd << ". Error is " << EGET() << " '" << ESTR()
               << "'.\n";

          return -1;
        }
      }
      #ifdef TEST
      else
      {
        *logofs << "Socket: Set IPTOS_LOWDELAY flag on FD#"
                << fd << ".\n" << logofs_flush;
      }
      #endif

      return 1;
    }
    default:
    {
      #ifdef TEST
      *logofs << "Socket: Option IPTOS_LOWDELAY not "
              << "supported on FD#" << fd << ".\n"
              << logofs_flush;
      #endif

      return 0;
    }
  }
}

int SetCloseOnExec(int fd)
{
  if (fcntl(fd, F_SETFD, 1) != 0)
  {
    #ifdef TEST
    *logofs << "NXClient: PANIC! Cannot set close-on-exec "
            << "on FD#" << fd << ". Error is " << EGET()
            << " '" << ESTR() << "'.\n" << logofs_flush;
    #endif

    cerr << "Error" << ": Cannot set close-on-exec on FD#"
         << fd << ". Error is " << EGET() << " '" << ESTR()
         << "'.\n";

    return -1;
  }

  return 1;
}

int GetBytesReadable(int fd)
{
  long readable = 0;

  //
  // It may fail, for example at session
  // shutdown.
  //

  if (ioctl(fd, FIONREAD, &readable) < 0)
  {
    #ifdef TEST
    *logofs << "Socket: PANIC! Failed to get bytes readable "
            << "from FD#" << fd << ". Error is " << EGET()
            << " '" << ESTR() << "'.\n" << logofs_flush;
    #endif

    return -1;
  }

  #ifdef TEST
  *logofs << "Socket: Returning " << (int) readable
          << " bytes readable on FD#" << fd << ".\n"
           << logofs_flush;
  #endif

  return (int) readable;
}

int GetBytesWritable(int fd)
{
  if (_kernelStep < 0)
  {
    GetKernelStep();
  }

  long writable;

  switch (_kernelStep)
  {
    case 3:
    {
      //
      // TODO: Should query the real size
      // of the TCP write buffer.
      //

      writable = 16384 - GetBytesQueued(fd);

      if (writable < 0)
      {
        writable = 0;
      }

      break;
    }
    case 2:
    {
      if (ioctl(fd, TIOCOUTQ, (void *) &writable) < 0)
      {
        #ifdef PANIC
        *logofs << "Socket: PANIC! Failed to get bytes writable "
                << "on FD#" << fd << ". Error is " << EGET()
                << " '" << ESTR() << "'.\n" << logofs_flush;
        #endif

        cerr << "Error" << ": Failed to get bytes writable "
             << "on FD#" << fd << ". Error is " << EGET()
             << " '" << ESTR() << "'.\n";

        return -1;
      }

      break;
    }
    default:
    {
      #ifdef TEST
      *logofs << "Socket: Option TIOCOUTQ not supported "
              << "on FD#" << fd << ",\n" << logofs_flush;
      #endif

      //
      // TODO: Should query the real size
      // of the TCP write buffer.
      //

      writable = 16384;

      break;
    }
  }

  #ifdef TEST
  *logofs << "Socket: Returning " << writable
          << " bytes writable on FD#" << fd
          << ".\n" << logofs_flush;
  #endif

  return (int) writable;
}

int GetBytesQueued(int fd)
{
  //
  // The TIOCOUTQ ioctl is not implemented on Cygwin
  // and returns the space available on Linux Kernels
  // 2.0 and 2.2 (like current MIPS for PS/2).
  //

  if (_kernelStep < 0)
  {
    GetKernelStep();
  }

  long queued;

  switch (_kernelStep)
  {
    case 3:
    {
      if (ioctl(fd, TIOCOUTQ, (void *) &queued) < 0)
      {
        #ifdef PANIC
        *logofs << "Socket: PANIC! Failed to get bytes queued "
                << "on FD#" << fd << ". Error is " << EGET()
                << " '" << ESTR() << "'.\n" << logofs_flush;
        #endif

        cerr << "Error" << ": Failed to get bytes queued "
             << "on FD#" << fd << ". Error is " << EGET()
             << " '" << ESTR() << "'.\n";

        return -1;
      }

      break;
    }
    case 2:
    {
      //
      // TODO: Should query the real size
      // of the TCP write buffer.
      //

      queued = 16384 - GetBytesWritable(fd);

      if (queued < 0)
      {
        queued = 0;
      }

      break;
    }
    default:
    {
      #ifdef TEST
      *logofs << "Socket: Option TIOCOUTQ not supported "
              << "on FD#" << fd << ",\n" << logofs_flush;
      #endif

      queued = 0;

      break;
    }
  }

  #ifdef TEST
  *logofs << "Socket: Returning " << queued
          << " bytes queued on FD#" << fd
          << ".\n" << logofs_flush;
  #endif

  return (int) queued;
}

int GetHostAddress(const char *name)
{
  hostent *host = gethostbyname(name);

  if (host == NULL)
  {
    //
    // On some Unices gethostbyname() doesn't
    // accept IP addresses, so try inet_addr.
    //

    IN_ADDR_T address = inet_addr(name);

    if (address == INADDR_NONE)
    {
      #ifdef PANIC
      *logofs << "Socket: PANIC! Failed to resolve address of '"
              << name << "'.\n" << logofs_flush;
      #endif

      cerr << "Error" << ": Failed to resolve address of '"
           << name << "'.\n";

      return 0;
    }

    return (int) address;
  }
  else
  {
    return (*((int *) host -> h_addr_list[0]));
  }
}
