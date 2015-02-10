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

#include <cstdio>
#include <cctype>
#include <cstdlib>
#include <unistd.h>
#include <csignal>

#include <errno.h>
#include <string.h>

#include "NXproto.h"

#include "MD5.h"

#include "Misc.h"
#include "Proxy.h"

//
// Set the verbosity level.
//

#define PANIC
#define WARNING
#define OPCODES
#undef  TEST
#undef  DEBUG

//
// By default nxproxy binds to all network interfaces, setting
// DEFAULT_LOOPBACK_BIND to 1 enables binding to the loopback
// device only.
//

const int DEFAULT_LOOPBACK_BIND = 0;

//
// TCP port offset applied to any NX port specification.
//

const int DEFAULT_NX_PROXY_PORT_OFFSET = 4000;

//
// Default TCP port used by client proxy to listen to  
// X clients and by server proxy to connect to remote.
//

const int DEFAULT_NX_PROXY_PORT = 8;

//
// Default X display number that client proxy imitates.
//

const int DEFAULT_NX_X_PORT = 8;

//
// Default ports used for listening for cups, samba, http,
// multimedia and auxiliary X connections. Arbitrary ports
// can be used by passing the service's port at the proxy
// startup. By default ports are determined by adding the
// offset below to the offset of the proxied display. For
// example, if the proxy is impersonating the display :8,
// SMB tunnels can be created by connecting to port 3008.
//
// Considering that the NX server uses to start the first
// session at display offset 1000, we must lower the CUPS
// and SMB ports to avoid interference with normal X ses-
// sions run on the server.
//
// Font server connections are used to let the X server on
// the client connect to a font server on the NX server.
//
// Slave channels can be originated by both sides so we need
// different offsets in the case the user runs both proxies
// on the same host.
//

const int DEFAULT_NX_CUPS_PORT_OFFSET  = 2000;
const int DEFAULT_NX_SMB_PORT_OFFSET   = 3000;
const int DEFAULT_NX_MEDIA_PORT_OFFSET = 7000;
const int DEFAULT_NX_AUX_PORT_OFFSET   = 8000;
const int DEFAULT_NX_HTTP_PORT_OFFSET  = 9000;
const int DEFAULT_NX_FONT_PORT_OFFSET  = 10000;

const int DEFAULT_NX_SLAVE_PORT_CLIENT_OFFSET = 11000;
const int DEFAULT_NX_SLAVE_PORT_SERVER_OFFSET = 12000;

//
// Usage info and copyright.
//

static const char UsageInfo[] =
"\n\
  Usage: nxproxy [OPTIONS] host:port\n\
\n\
  -C           Specify that nxproxy has to run on the 'X client'\n\
               side, listening for connections and impersonating\n\
               an X server.\n\
\n\
  -S           Specify that nxproxy has to run in 'X server' mode,\n\
               thus forwarding the connections to daemons running\n\
               on the client.\n\
\n\
  -h           Print this message.\n\
\n\
  -v           Print version information.\n\
\n\
  host:port    Put at the end, specifies the host and port of the\n\
               listening proxy.\n\
\n\
  name=value   Set the NX option to the provided value.\n\
\n\
  Multiple NX options can be specified in the DISPLAY environment\n\
  or on the command line, by using the nx/nx,option=value notation.\n\
\n\
  Options:\n\
\n\
  link=s       An indication of the link speed that is going to be\n\
               used between the proxies. Usually the compression\n\
               and the other link parameters depend on this setting.\n\
               The value can be either 'modem', 'isdn', 'adsl',\n\
               'wan', 'lan', 'local' or a bandwidth specification,\n\
               like for example '56k', '1m', '100m', etc.\n\
\n\
  type=s       Type of session, for example 'windows', 'unix-kde'.\n\
               'unix-application', etc.\n\
\n\
  display=s    Specify the real display where X connections have\n\
               to be forwarded by the proxy running on the client.\n\
\n\
  listen=n     Local port used for accepting the proxy connection.\n\
\n\
  loopback=b   Bind to the loopback device only.\n\
\n\
  accept=s     Name or IP of host that can connect to the proxy.\n\
\n\
  connect=s    Name or IP of host that the proxy will connect to.\n\
\n\
  port=n       Remote port used for the connection.\n\
\n\
  retry=n      Number of connection atempts.\n\
\n\
  root=s       The root directory for the session. Usually is the\n\
               C-* or S-* in the .nx directory in the user's home,\n\
               with '*' being the virtual display.\n\
\n\
  session=s    Name of the session file. The default is the name\n\
               'session' in the session directory.\n\
\n\
  errors=s     Name of the log file used by the proxy. The default\n\
               is the name 'errors' in the session directory.\n\
\n\
  stats=s      Name of the file where are written the proxy stat-\n\
               istics. The default is a file 'stats' in the session\n\
               directory. The proxy replaces the data in the file\n\
               whenever it receives a SIGUSR1 or SIGUSR2 signal:\n\
\n\
               SIGUSR1    Gives total statistics, i.e. statistics\n\
                          collected since the beginning of the\n\
                          session.\n\
\n\
               SIGUSR2    Gives partial statistics, i.e. statist-\n\
                          ics collected since the last time this\n\
                          signal was received.\n\
\n\
  cookie=s     Use the provided cookie for authenticating to the\n\
               remote proxy. The same cookie is used as the fake\n\
               value used for the X authorization. The fake cookie\n\
               is replaced on the X server side with the real cookie\n\
               to be used for the display, so that the real cookie\n\
               doesn't have to travel over the net. When not using\n\
               a proxy cookie, any host will be able to connect to\n\
               the proxy. See also the 'accept' parameter.\n\
\n\
  nodelay=b    A boolean indicating if TCP_NODELAY has to be set\n\
               on the proxy link. Old Linux kernels had problems\n\
               with handling TCP_NODELAY on PPP links.\n\
\n\
  policy=b     Let or not the agent decide when it is the best time\n\
               to flush the proxy link. If set to 0, the proxy will\n\
               flush any encoded data immediately. The option has\n\
               only effect on the X client side proxy.\n\
\n\
  render=b     Enable or disable use of the RENDER extension.\n\
\n\
  taint=b      Try to suppress trivial sources of X roundtrips by\n\
               generating the reply on the X client side.\n\
\n\
  delta=b      Enable X differential compression.\n\
\n\
  data=n       Enable or disable the ZLIB data compression. It is\n\
               possible to specify a value between 0 and 9. Usual-\n\
               ly the value is chosen automatically based on the\n\
               requested link setting.\n\
\n\
  stream=n     Enable or disable the ZLIB stream compression. The\n\
               value, between 0 and 9, is usually determined accor-\n\
               ding to the requested link setting.\n\
\n\
  limit=n      Specify a bitrate limit allowed for this session.\n\
\n\
  memory=n     Trigger memory optimizations used to keep small the\n\
               size of X buffers. This is useful on embedded plat-\n\
               forms, or where memory is scarce.\n\
\n\
  cache=n      Size of the in-memory X message cache. Setting the\n\
               value to 0 will disable the memory cache as well\n\
               as the NX differential compression.\n\
\n\
  images=n     Size of the persistent image cache.\n\
\n\
  shseg=n      Enable the use of the MIT-SHM extension between the\n\
               NX client proxy and the real X server. A value greater\n\
               than 1 is assumed to be the size of requested shared\n\
               memory segment. By default, the size of the segment is\n\
               determined based on the size of the in-memory cache.\n\
\n\
  load=b       Enable loading a persistent X message cache at the\n\
               proxy startup.\n\
\n\
  save=b       Enable saving a persistent X message cache at the\n\
               end of session.\n\
\n\
  cups=n       Enable or disable forwarding of CUPS connections,\n\
               by listening on the optional port 'n'.\n\
\n\
  aux=n        Enable or disable forwarding of the auxiliary X chan-\n\
               nel used for controlling the keyboard. The 'keybd=n'\n\
               form is accepted for backward compatibility.\n\
\n\
  smb=n        Enable or disable forwarding of SMB connections. The\n\
               'samba=n' form is accepted for backward compatibility.\n\
\n\
  media=n      Enable forwarding of audio connections.\n\
\n\
  http=n       Enable forwarding of HTTP connections.\n\
\n\
  font=n       Enable forwarding of reversed connections to a font\n\
               server running on the NX server.\n\
\n\
  file=n       Enable forwarding of file transfer connections.\n\
\n\
  mask=n       Determine the distribution of channel ids between the\n\
               proxies. By default, channels whose ids are multiple\n\
               of 8 (starting from 0) are reserved for the NX client\n\
               side. All the other channels can be allocated by the\n\
               NX server side.\n\
\n\
  timeout=t    Specify the keep-alive timeout used by proxies to\n\
               determine if there is a network problem preventing\n\
               communication with the remote peer. A value of 0\n\
               disables the check.\n\
\n\
  cleanup=t    Specify the number of seconds the proxy has to wait\n\
               at session shutdown before closing all channels.\n\
               The feature is used by the NX server to ensure that\n\
               services are disconnected before shutting down the\n\
               link.\n\
\n\
  pack=s       Determine the method used to compress images.\n\
\n\
  product=s    The product id of the client or server. The value is\n\
               ignored by the proxy, but the client or server can\n\
               provide it to facilitate the support.\n\
\n\
  core=b       Enable production of core dumps when aborting the\n\
               proxy connection.\n\
\n\
  options=s    Specify an additional file containing options that\n\
               has to be merged with option read from the command\n\
               line or the environment.\n\
\n\
  kill=n       Add the given process to the list of daemons that\n\
               must be terminated at session shutdown. Multiple\n\
               'kill=n' options can be specified. The proxy will\n\
               send them a SIGTERM signal just before exiting.\n\
\n\
  strict=b     Optimize for responsiveness, rather than for the best\n\
               use of all the available bandwidth.\n\
\n\
  encryption=b Should be set to 1 if the proxy is running as part of\n\
               a program providing encryption of the point to point\n\
               communication.\n\
\n\
rootless=b\n\
geometry=s\n\
resize=b\n\
fullscreen=b\n\
keyboard=s\n\
clipboard=n\n\
streaming=n\n\
backingstore=n\n\
composite=n\n\
shmem=b\n\
shpix=b\n\
kbtype=s\n\
client=s\n\
shadow=n\n\
shadowuid=n\n\
shadowmode=s\n\
defer=n\n\
tile=s\n\
menu=n         These options are interpreted by the NX agent. They\n\
               are ignored by the proxy.\n\
\n\
  Environment:\n\
\n\
  NX_ROOT      The root NX directory is the place where the session\n\
               directory and the cache files are created. This is\n\
               usually overridden by passing the 'root=' option. By\n\
               default, the root NX directory is assumed to be the\n\
               directory '.nx' in the user's home.\n\
\n\
  NX_SYSTEM    The directory where NX programs and libraries reside.\n\
               If not set, the value is assumed to be '/usr/NX'.\n\
               Programs, libraries and data files are respectedly\n\
               searched in the 'bin', 'lib' and 'share' subdirecto-\n\
               ries.\n\
\n\
  NX_HOME      The NX user's home directory. If NX_ROOT is not set\n\
               or invalid, the user's NX directory is created here.\n\
\n\
  NX_TEMP      The directory where the X11 Unix Domain Sockets and\n\
               all temporary files are to be created.\n\
\n\
  NX_CLIENT    The full path to the nxclient executable. If the va-\n\
               riable is not set, the nxclient executable will be\n\
               run assuming that the program is in the system path.\n\
               This can be useful on platforms like Windows and the\n\
               Mac where nxclient is located in a different direct-\n\
               ory compared to the other programs, to make easier\n\
               for the user to execute the program from the shell.\n\
\n\
  Shell environment:\n\
\n\
  HOME         The variable is checked in the case NX_HOME is not\n\
               set, null or invalid.\n\
\n\
  TEMP         The variable is checked whenever the NX_TEMP direct-\n\
               ory is not set, null or invalid.\n\
\n\
  PATH         The path where all executables are searched, except\n\
               nxclient. If NX_CLIENT is not set, also the client\n\
               executable is searched in the system path.\n\
\n\
  LD_LIBRARY_PATH\n\
               System-wide library search order. This should be set\n\
               by the program invoking the proxy.\n\
\n\
  DISPLAY      On the X server side, the DISPLAY variable indicates\n\
               the location of the X11 server. When nxcomp is used\n\
               as a transport library, the DISPLAY may represent a\n\
               NX transport specification and options can passed in\n\
               the form nx/nx,option=value...\n\
\n\
  XAUTHORITY   This is the file containing the X11 authorization\n\
               cookie. If not set, the file is assumed to be in\n\
               the user's home (either NX_HOME or HOME).\n\
\n\
";

const char *GetUsageInfo()
{
  return UsageInfo;
}

static const char CopyrightInfo[] =
"\
Copyright (c) 2001, 2010 NoMachine, http://www.nomachine.com/.\n\
\n\
NXCOMP, NX protocol compression and NX extensions to this software \n\
are copyright of NoMachine. Redistribution and use of the present\n\
software is allowed according to terms specified in the file LICENSE\n\
which comes in the source distribution.\n\
\n\
Check http://www.nomachine.com/licensing.html for applicability.\n\
\n\
NX and NoMachine are trademarks of NoMachine S.r.l.\n\
\n\
All rights reserved.\n\
";

const char *GetCopyrightInfo()
{
  return CopyrightInfo;
}

static const char OtherCopyrightInfo[] =
"\
NX protocol compression is derived from DXPC project.\n\
\n\
Copyright (c) 1995,1996 Brian Pane\n\
Copyright (c) 1996,1997 Zachary Vonler and Brian Pane\n\
Copyright (c) 1999 Kevin Vigor and Brian Pane\n\
Copyright (c) 2000,2003 Gian Filippo Pinzari and Brian Pane\n\
\n\
All rights reserved.\n\
";

const char *GetOtherCopyrightInfo()
{
  return OtherCopyrightInfo;
}

int _hostBigEndian  = 0;
int _storeBigEndian = 0;

const unsigned int IntMask[33] =
{
  0x00000000,
  0x00000001,
  0x00000003,
  0x00000007,
  0x0000000f,
  0x0000001f,
  0x0000003f,
  0x0000007f,
  0x000000ff,
  0x000001ff,
  0x000003ff,
  0x000007ff,
  0x00000fff,
  0x00001fff,
  0x00003fff,
  0x00007fff,
  0x0000ffff,
  0x0001ffff,
  0x0003ffff,
  0x0007ffff,
  0x000fffff,
  0x001fffff,
  0x003fffff,
  0x007fffff,
  0x00ffffff,
  0x01ffffff,
  0x03ffffff,
  0x07ffffff,
  0x0fffffff,
  0x1fffffff,
  0x3fffffff,
  0x7fffffff,
  0xffffffff
};

unsigned int GetUINT(unsigned const char *buffer, int bigEndian)
{
  //
  // It doesn't work on SPARCs if the buffer
  // is not aligned to the word boundary. We
  // should check the CPU, not the OS as this
  // surely applies to other architectures.
  //

  #ifndef __sun

  if (_hostBigEndian == bigEndian)
  {
    return *((unsigned short *) buffer);
  }

  #else

  if (_hostBigEndian == bigEndian && ((unsigned int) buffer) & 0x1 == 0)
  {
    return *((unsigned short *) buffer);
  }

  #endif

  unsigned int result;

  if (bigEndian)
  {
    result = *buffer;

    result <<= 8;

    result += buffer[1];
  }
  else
  {
    result = buffer[1];

    result <<= 8;

    result += *buffer;
  }

  return result;
}

unsigned int GetULONG(unsigned const char *buffer, int bigEndian)
{
  //
  // It doesn't work on SPARCs if the buffer
  // is not aligned to word the boundary.
  //

  #ifndef __sun

  if (_hostBigEndian == bigEndian)
  {
    return *((unsigned int *) buffer);
  }

  #else

  if (_hostBigEndian == bigEndian && ((unsigned int) buffer) & 0x3 == 0)
  {
    return *((unsigned int *) buffer);
  }

  #endif

  const unsigned char *next = (bigEndian ? buffer : buffer + 3);

  unsigned int result = 0;

  for (int i = 0; i < 4; i++)
  {
    result <<= 8;

    result += *next;

    if (bigEndian)
    {
      next++;
    }
    else
    {
      next--;
    }
  }

  return result;
}

void PutUINT(unsigned int value, unsigned char *buffer, int bigEndian)
{
  if (_hostBigEndian == bigEndian)
  {
    *((unsigned short *) buffer) = value;

    return;
  }

  if (bigEndian)
  {
    buffer[1] = (unsigned char) (value & 0xff);

    value >>= 8;

    *buffer = (unsigned char) value;
  }
  else
  {
    *buffer = (unsigned char) (value & 0xff);

    value >>= 8;

    buffer[1] = (unsigned char) value;
  }
}

void PutULONG(unsigned int value, unsigned char *buffer, int bigEndian)
{
  if (_hostBigEndian == bigEndian)
  {
    *((unsigned int *) buffer) = value;

    return;
  }

  if (bigEndian)
  {
    buffer += 3;

    for (int i = 4; i > 0; i--)
    {
      *buffer-- = (unsigned char) (value & 0xff);

      value >>= 8;
    }
  }
  else
  {
    for (int i = 4; i > 0; i--)
    {
      *buffer++ = (unsigned char) (value & 0xff);

      value >>= 8;
    }
  }
}

int CheckData(istream *fs)
{
  if (fs == NULL || fs -> fail())
  {
    return -1;
  }

  return 1;
}

int CheckData(ostream *fs)
{
  if (fs == NULL || fs -> fail())
  {
    return -1;
  }

  return 1;
}

int PutData(ostream *fs, const unsigned char *buffer, int size)
{
  fs -> write((char *) buffer, size);

  #ifdef DEBUG
  *logofs << "PutData: Written " << size << " bytes with eof "
          << fs -> eof() << " fail " << fs -> fail() << " and bad "
          << fs -> bad() << ".\n" << logofs_flush;
  #endif

  if (fs -> fail())
  {
    return -1;
  }

  return size;
}

int GetData(istream *fs, unsigned char *buffer, int size)
{
  fs -> read((char *) buffer, size);

  #ifdef DEBUG
  *logofs << "GetData: Read " << size << " bytes with eof "
          << fs -> eof() << " fail " << fs -> fail()
          << " and bad " << fs -> bad() << ".\n"
          << logofs_flush;
  #endif

  #ifdef __APPLE__

  if (fs -> bad())
  {
    return -1;
  }

  #else

  if (fs -> fail())
  {
    return -1;
  }

  #endif

  return size;
}

int FlushData(ostream *fs)
{
  fs -> flush();

  if (fs -> fail())
  {
    return -1;
  }

  return 1;
}

unsigned int RoundUp2(unsigned int x)
{
  unsigned int y = x / 2;

  y *= 2;

  if (y != x)
  {
    y += 2;
  }

  return y;
}

unsigned int RoundUp4(unsigned int x)
{
  unsigned int y = x / 4;

  y *= 4;

  if (y != x)
  {
    y += 4;
  }

  return y;
}

unsigned int RoundUp8(unsigned int x)
{
  unsigned int y = x / 8;

  y *= 8;

  if (y != x)
  {
    y += 8;
  }

  return y;
}

const char *DumpSignal(int signal)
{
  switch (signal)
  {
    case SIGCHLD:
    {
      return "SIGCHLD";
    }
    case SIGUSR1:
    {
      return "SIGUSR1";
    }
    case SIGUSR2:
    {
      return "SIGUSR2";
    }
    case SIGHUP:
    {
      return "SIGHUP";
    }
    case SIGINT:
    {
      return "SIGINT";
    }
    case SIGTERM:
    {
      return "SIGTERM";
    }
    case SIGPIPE:
    {
      return "SIGPIPE";
    }
    case SIGALRM:
    {
      return "SIGALRM";
    }
    case SIGVTALRM:
    {
      return "SIGVTALRM";
    }
    case SIGWINCH:
    {
      return "SIGWINCH";
    }
    case SIGIO:
    {
      return "SIGIO";
    }
    case SIGTSTP:
    {
      return "SIGTSTP";
    }
    case SIGTTIN:
    {
      return "SIGTTIN";
    }
    case SIGTTOU:
    {
      return "SIGTTOU";
    }
    case SIGSEGV:
    {
      return "SIGSEGV";
    }
    case SIGABRT:
    {
      return "SIGABRT";
    }
    default:
    {
      return "Unknown";
    }
  }
}

const char *DumpPolicy(int type)
{
  switch ((T_flush_policy) type)
  {
    case policy_immediate:
    {
      return "immediate";
    }
    case policy_deferred:
    {
      return "deferred";
    }
    default:
    {
      #ifdef PANIC
      *logofs << "Misc: PANIC! Unknown policy type '"
              << type << "'.\n" << logofs_flush;
      #endif

      cerr << "Error" << ": Unknown policy type '"
           << type << "'.\n";

      HandleCleanup();
    }
  }
}

const char *DumpAction(int type)
{
  T_store_action action = (T_store_action) type;

  if (action == IS_HIT)
  {
    return "is_hit";
  }
  else if (action == IS_ADDED)
  {
    return "is_added";
  }
  else if (action == is_discarded)
  {
    return "is_discarded";
  }
  else if (action == is_removed)
  {
    return "is_removed";
  }
  else
  {
    #ifdef PANIC
    *logofs << "Misc: PANIC! Unknown store action '"
            << type << "'.\n" << logofs_flush;
    #endif

    cerr << "Error" << ": Unknown store action '"
         << type << "'.\n";

    HandleCleanup();
  }
}

const char *DumpState(int type)
{
  switch ((T_split_state) type)
  {
    case split_added:
    {
      return "split_added";
    }
    case split_missed:
    {
      return "split_missed";
    }
    case split_loaded:
    {
      return "split_loaded";
    }
    case split_aborted:
    {
      return "split_aborted";
    }
    case split_notified:
    {
      return "split_notified";
    }
    default:
    {
      #ifdef PANIC
      *logofs << "Misc: PANIC! Unknown split state '"
              << type << "'.\n" << logofs_flush;
      #endif

      cerr << "Error" << ": Unknown split state '"
           << type << "'.\n";

      HandleCleanup();
    }
  }
}

const char *DumpControl(int code)
{
  switch ((T_proxy_code) code)
  {
    case code_new_x_connection:
    {
      return "code_new_x_connection";
    }
    case code_new_cups_connection:
    {
      return "code_new_cups_connection";
    }
    case code_new_aux_connection:
    {
      return "code_new_aux_connection";
    }
    case code_new_smb_connection:
    {
      return "code_new_smb_connection";
    }
    case code_new_media_connection:
    {
      return "code_new_media_connection";
    }
    case code_switch_connection:
    {
      return "code_switch_connection";
    }
    case code_drop_connection:
    {
      return "code_drop_connection";
    }
    case code_finish_connection:
    {
      return "code_finish_connection";
    }
    case code_begin_congestion:
    {
      return "code_begin_congestion";
    }
    case code_end_congestion:
    {
      return "code_end_congestion";
    }
    case code_alert_request:
    {
      return "code_alert_request";
    }
    case code_alert_reply:
    {
      return "code_alert_reply";
    }
    case code_reset_request:
    {
      return "code_reset_request";
    }
    case code_reset_reply:
    {
      return "code_reset_reply";
    }
    case code_load_request:
    {
      return "code_load_request";
    }
    case code_load_reply:
    {
      return "code_load_reply";
    }
    case code_save_request:
    {
      return "code_save_request";
    }
    case code_save_reply:
    {
      return "code_save_reply";
    }
    case code_shutdown_request:
    {
      return "code_shutdown_request";
    }
    case code_shutdown_reply:
    {
      return "code_shutdown_reply";
    }
    case code_control_token_request:
    {
      return "code_control_token_request";
    }
    case code_control_token_reply:
    {
      return "code_control_token_reply";
    }
    case code_configuration_request:
    {
      return "code_configuration_request";
    }
    case code_configuration_reply:
    {
      return "code_configuration_reply";
    }
    case code_statistics_request:
    {
      return "code_statistics_request";
    }
    case code_statistics_reply:
    {
      return "code_statistics_reply";
    }
    case code_new_http_connection:
    {
      return "code_new_http_connection";
    }
    case code_sync_request:
    {
      return "code_sync_request";
    }
    case code_sync_reply:
    {
      return "code_sync_reply";
    }
    case code_new_font_connection:
    {
      return "code_new_font_connection";
    }
    case code_new_slave_connection:
    {
      return "code_new_slave_connection";
    }
    case code_finish_listeners:
    {
      return "code_finish_listeners";
    }
    case code_split_token_request:
    {
      return "code_split_token_request";
    }
    case code_split_token_reply:
    {
      return "code_split_token_reply";
    }
    case code_data_token_request:
    {
      return "code_data_token_request";
    }
    case code_data_token_reply:
    {
      return "code_data_token_reply";
    }
    default:
    {
      #ifdef WARNING
      *logofs << "Misc: WARNING! Unknown control code '"
              << code << "'.\n" << logofs_flush;
      #endif

      cerr << "Warning" << ": Unknown control code '"
           << code << "'.\n";

      return "unknown";
    }
  }
}

const char *DumpSession(int code)
{
  switch ((T_session_mode) code)
  {
    case session_agent:
    {
      return "session_agent";
    }
    case session_shadow:
    {
      return "session_shadow";
    }
    case session_proxy:
    {
      return "session_proxy";
    }
    default:
    {
      #ifdef WARNING
      *logofs << "Misc: WARNING! Unknown session type '"
              << code << "'.\n" << logofs_flush;
      #endif

      cerr << "Warning" << ": Unknown session type '"
           << code << "'.\n";

      return "unknown";
    }
  }
}

const char *DumpToken(int type)
{
  switch ((T_token_type) type)
  {
    case token_control:
    {
      return "token_control";
    }
    case token_split:
    {
      return "token_split";
    }
    case token_data:
    {
      return "token_data";
    }
    default:
    {
      #ifdef WARNING
      *logofs << "Misc: WARNING! Unknown token type '"
              << type << "'.\n" << logofs_flush;
      #endif

      cerr << "Warning" << ": Unknown token type '"
           << type << "'.\n";

      return "unknown";
    }
  }
}

//
// Always include this in code as it is generally
// needed to test channels and split store.
//

const char *DumpChecksum(const void *checksum)
{
  static char string[MD5_LENGTH * 2 + 1];

  if (checksum != NULL)
  {
    for (unsigned int i = 0; i < MD5_LENGTH; i++)
    {
      sprintf(string + (i * 2), "%02X", ((unsigned char *) checksum)[i]);
    }
  }
  else
  {
    strcpy(string, "null");
  }

  return string;
}

//
// Define OPCODES here and in the channel
// if you want to log the opcode literal.
//

#ifdef OPCODES

const char *DumpOpcode(const int &opcode)
{
  switch (opcode)
  {
    case X_CreateWindow:
    {
      return "X_CreateWindow";
    }
    case X_ChangeWindowAttributes:
    {
      return "X_ChangeWindowAttributes";
    }
    case X_GetWindowAttributes:
    {
      return "X_GetWindowAttributes";
    }
    case X_DestroyWindow:
    {
      return "X_DestroyWindow";
    }
    case X_DestroySubwindows:
    {
      return "X_DestroySubwindows";
    }
    case X_ChangeSaveSet:
    {
      return "X_ChangeSaveSet";
    }
    case X_ReparentWindow:
    {
      return "X_ReparentWindow";
    }
    case X_MapWindow:
    {
      return "X_MapWindow";
    }
    case X_MapSubwindows:
    {
      return "X_MapSubwindows";
    }
    case X_UnmapWindow:
    {
      return "X_UnmapWindow";
    }
    case X_UnmapSubwindows:
    {
      return "X_UnmapSubwindows";
    }
    case X_ConfigureWindow:
    {
      return "X_ConfigureWindow";
    }
    case X_CirculateWindow:
    {
      return "X_CirculateWindow";
    }
    case X_GetGeometry:
    {
      return "X_GetGeometry";
    }
    case X_QueryTree:
    {
      return "X_QueryTree";
    }
    case X_InternAtom:
    {
      return "X_InternAtom";
    }
    case X_GetAtomName:
    {
      return "X_GetAtomName";
    }
    case X_ChangeProperty:
    {
      return "X_ChangeProperty";
    }
    case X_DeleteProperty:
    {
      return "X_DeleteProperty";
    }
    case X_GetProperty:
    {
      return "X_GetProperty";
    }
    case X_ListProperties:
    {
      return "X_ListProperties";
    }
    case X_SetSelectionOwner:
    {
      return "X_SetSelectionOwner";
    }
    case X_GetSelectionOwner:
    {
      return "X_GetSelectionOwner";
    }
    case X_ConvertSelection:
    {
      return "X_ConvertSelection";
    }
    case X_SendEvent:
    {
      return "X_SendEvent";
    }
    case X_GrabPointer:
    {
      return "X_GrabPointer";
    }
    case X_UngrabPointer:
    {
      return "X_UngrabPointer";
    }
    case X_GrabButton:
    {
      return "X_GrabButton";
    }
    case X_UngrabButton:
    {
      return "X_UngrabButton";
    }
    case X_ChangeActivePointerGrab:
    {
      return "X_ChangeActivePointerGrab";
    }
    case X_GrabKeyboard:
    {
      return "X_GrabKeyboard";
    }
    case X_UngrabKeyboard:
    {
      return "X_UngrabKeyboard";
    }
    case X_GrabKey:
    {
      return "X_GrabKey";
    }
    case X_UngrabKey:
    {
      return "X_UngrabKey";
    }
    case X_AllowEvents:
    {
      return "X_AllowEvents";
    }
    case X_GrabServer:
    {
      return "X_GrabServer";
    }
    case X_UngrabServer:
    {
      return "X_UngrabServer";
    }
    case X_QueryPointer:
    {
      return "X_QueryPointer";
    }
    case X_GetMotionEvents:
    {
      return "X_GetMotionEvents";
    }
    case X_TranslateCoords:
    {
      return "X_TranslateCoords";
    }
    case X_WarpPointer:
    {
      return "X_WarpPointer";
    }
    case X_SetInputFocus:
    {
      return "X_SetInputFocus";
    }
    case X_GetInputFocus:
    {
      return "X_GetInputFocus";
    }
    case X_QueryKeymap:
    {
      return "X_QueryKeymap";
    }
    case X_OpenFont:
    {
      return "X_OpenFont";
    }
    case X_CloseFont:
    {
      return "X_CloseFont";
    }
    case X_QueryFont:
    {
      return "X_QueryFont";
    }
    case X_QueryTextExtents:
    {
      return "X_QueryTextExtents";
    }
    case X_ListFonts:
    {
      return "X_ListFonts";
    }
    case X_ListFontsWithInfo:
    {
      return "X_ListFontsWithInfo";
    }
    case X_SetFontPath:
    {
      return "X_SetFontPath";
    }
    case X_GetFontPath:
    {
      return "X_GetFontPath";
    }
    case X_CreatePixmap:
    {
      return "X_CreatePixmap";
    }
    case X_FreePixmap:
    {
      return "X_FreePixmap";
    }
    case X_CreateGC:
    {
      return "X_CreateGC";
    }
    case X_ChangeGC:
    {
      return "X_ChangeGC";
    }
    case X_CopyGC:
    {
      return "X_CopyGC";
    }
    case X_SetDashes:
    {
      return "X_SetDashes";
    }
    case X_SetClipRectangles:
    {
      return "X_SetClipRectangles";
    }
    case X_FreeGC:
    {
      return "X_FreeGC";
    }
    case X_ClearArea:
    {
      return "X_ClearArea";
    }
    case X_CopyArea:
    {
      return "X_CopyArea";
    }
    case X_CopyPlane:
    {
      return "X_CopyPlane";
    }
    case X_PolyPoint:
    {
      return "X_PolyPoint";
    }
    case X_PolyLine:
    {
      return "X_PolyLine";
    }
    case X_PolySegment:
    {
      return "X_PolySegment";
    }
    case X_PolyRectangle:
    {
      return "X_PolyRectangle";
    }
    case X_PolyArc:
    {
      return "X_PolyArc";
    }
    case X_FillPoly:
    {
      return "X_FillPoly";
    }
    case X_PolyFillRectangle:
    {
      return "X_PolyFillRectangle";
    }
    case X_PolyFillArc:
    {
      return "X_PolyFillArc";
    }
    case X_PutImage:
    {
      return "X_PutImage";
    }
    case X_GetImage:
    {
      return "X_GetImage";
    }
    case X_PolyText8:
    {
      return "X_PolyText8";
    }
    case X_PolyText16:
    {
      return "X_PolyText16";
    }
    case X_ImageText8:
    {
      return "X_ImageText8";
    }
    case X_ImageText16:
    {
      return "X_ImageText16";
    }
    case X_CreateColormap:
    {
      return "X_CreateColormap";
    }
    case X_FreeColormap:
    {
      return "X_FreeColormap";
    }
    case X_CopyColormapAndFree:
    {
      return "X_CopyColormapAndFree";
    }
    case X_InstallColormap:
    {
      return "X_InstallColormap";
    }
    case X_UninstallColormap:
    {
      return "X_UninstallColormap";
    }
    case X_ListInstalledColormaps:
    {
      return "X_ListInstalledColormaps";
    }
    case X_AllocColor:
    {
      return "X_AllocColor";
    }
    case X_AllocNamedColor:
    {
      return "X_AllocNamedColor";
    }
    case X_AllocColorCells:
    {
      return "X_AllocColorCells";
    }
    case X_AllocColorPlanes:
    {
      return "X_AllocColorPlanes";
    }
    case X_FreeColors:
    {
      return "X_FreeColors";
    }
    case X_StoreColors:
    {
      return "X_StoreColors";
    }
    case X_StoreNamedColor:
    {
      return "X_StoreNamedColor";
    }
    case X_QueryColors:
    {
      return "X_QueryColors";
    }
    case X_LookupColor:
    {
      return "X_LookupColor";
    }
    case X_CreateCursor:
    {
      return "X_CreateCursor";
    }
    case X_CreateGlyphCursor:
    {
      return "X_CreateGlyphCursor";
    }
    case X_FreeCursor:
    {
      return "X_FreeCursor";
    }
    case X_RecolorCursor:
    {
      return "X_RecolorCursor";
    }
    case X_QueryBestSize:
    {
      return "X_QueryBestSize";
    }
    case X_QueryExtension:
    {
      return "X_QueryExtension";
    }
    case X_ListExtensions:
    {
      return "X_ListExtensions";
    }
    case X_ChangeKeyboardMapping:
    {
      return "X_ChangeKeyboardMapping";
    }
    case X_GetKeyboardMapping:
    {
      return "X_GetKeyboardMapping";
    }
    case X_ChangeKeyboardControl:
    {
      return "X_ChangeKeyboardControl";
    }
    case X_GetKeyboardControl:
    {
      return "X_GetKeyboardControl";
    }
    case X_Bell:
    {
      return "X_Bell";
    }
    case X_ChangePointerControl:
    {
      return "X_ChangePointerControl";
    }
    case X_GetPointerControl:
    {
      return "X_GetPointerControl";
    }
    case X_SetScreenSaver:
    {
      return "X_SetScreenSaver";
    }
    case X_GetScreenSaver:
    {
      return "X_GetScreenSaver";
    }
    case X_ChangeHosts:
    {
      return "X_ChangeHosts";
    }
    case X_ListHosts:
    {
      return "X_ListHosts";
    }
    case X_SetAccessControl:
    {
      return "X_SetAccessControl";
    }
    case X_SetCloseDownMode:
    {
      return "X_SetCloseDownMode";
    }
    case X_KillClient:
    {
      return "X_KillClient";
    }
    case X_RotateProperties:
    {
      return "X_RotateProperties";
    }
    case X_ForceScreenSaver:
    {
      return "X_ForceScreenSaver";
    }
    case X_SetPointerMapping:
    {
      return "X_SetPointerMapping";
    }
    case X_GetPointerMapping:
    {
      return "X_GetPointerMapping";
    }
    case X_SetModifierMapping:
    {
      return "X_SetModifierMapping";
    }
    case X_GetModifierMapping:
    {
      return "X_GetModifierMapping";
    }
    case X_NoOperation:
    {
      return "X_NoOperation";
    }
    case X_NXInternalGenericData:
    {
      return "X_NXInternalGenericData";
    }
    //
    // case X_NXInternalGenericReply:
    // {
    //   return "X_NXInternalGenericReply";
    // }
    //
    case X_NXInternalGenericRequest:
    {
      return "X_NXInternalGenericRequest";
    }
    case X_NXInternalShapeExtension:
    {
      return "X_NXInternalShapeExtension";
    }
    case X_NXGetControlParameters:
    {
      return "X_NXGetControlParameters";
    }
    case X_NXGetCleanupParameters:
    {
      return "X_NXGetCleanupParameters";
    }
    case X_NXGetImageParameters:
    {
      return "X_NXGetImageParameters";
    }
    case X_NXGetUnpackParameters:
    {
      return "X_NXGetUnpackParameters";
    }
    case X_NXGetShmemParameters:
    {
      return "X_NXGetShmemParameters";
    }
    case X_NXGetFontParameters:
    {
      return "X_NXGetFontParameters";
    }
    case X_NXSetExposeParameters:
    {
      return "X_NXSetExposeParameters";
    }
    case X_NXSetCacheParameters:
    {
      return "X_NXSetCacheParameters";
    }
    case X_NXStartSplit:
    {
      return "X_NXStartSplit";
    }
    case X_NXEndSplit:
    {
      return "X_NXEndSplit";
    }
    case X_NXSplitData:
    {
      return "X_NXSplitData";
    }
    case X_NXSplitEvent:
    {
      return "X_NXSplitEvent";
    }
    case X_NXCommitSplit:
    {
      return "X_NXCommitSplit";
    }
    case X_NXFinishSplit:
    {
      return "X_NXFinishSplit";
    }
    case X_NXAbortSplit:
    {
      return "X_NXAbortSplit";
    }
    case X_NXFreeSplit:
    {
      return "X_NXFreeSplit";
    }
    case X_NXSetUnpackGeometry:
    {
      return "X_NXSetUnpackGeometry";
    }
    case X_NXSetUnpackColormap:
    {
      return "X_NXSetUnpackColormap";
    }
    case X_NXSetUnpackAlpha:
    {
      return "X_NXSetUnpackAlpha";
    }
    case X_NXPutPackedImage:
    {
      return "X_NXPutPackedImage";
    }
    case X_NXFreeUnpack:
    {
      return "X_NXFreeUnpack";
    }
    default:
    {
      if (opcode > 127)
      {
        return "Extension";
      }
      else
      {
        return "?";
      }
    }
  }
}

#else /* #ifdef OPCODES */

const char *DumpOpcode(const int &opcode)
{
  return "?";
}

#endif /* #ifdef OPCODES */

void DumpData(const unsigned char *buffer, unsigned int size)
{
  if (buffer != NULL)
  {
    unsigned int i = 0;

    while (i < size)
    {
      *logofs << "[" << i << "]\t";

      for (unsigned int ii = 0; i < size && ii < 8; i++, ii++)
      {
        *logofs << (unsigned int) (buffer[i]) << "\t";
      }

      *logofs << "\n" << logofs_flush;
    }
  }
}

void DumpChecksum(const unsigned char *buffer, unsigned int size)
{
  if (buffer != NULL)
  {
    md5_byte_t md5_digest[MD5_LENGTH];

    md5_state_t md5_state;

    md5_init(&md5_state);

    md5_append(&md5_state, buffer, size);

    md5_finish(&md5_state, md5_digest);

    char md5_string[MD5_LENGTH * 2 + 1];

    for (unsigned int i = 0; i < MD5_LENGTH; i++)
    {
      sprintf(md5_string + (i * 2), "%02X", md5_digest[i]);
    }

    *logofs << "[" << md5_string << "]" << logofs_flush;
  }
}

void DumpBlockChecksums(const unsigned char *buffer,
                            unsigned int size, unsigned int block)
{
  for (unsigned int i = 0; i < (size / block); i++)
  {
    *logofs << "[" << i * block << "]";

    DumpChecksum(buffer + (i * block), block);

    *logofs << "\n";
  }

  if (size % block > 0)
  {
    *logofs << "[" << size / block * block << "]";

    DumpChecksum(buffer + (size / block * block), size % block);

    *logofs << "\n";
  }
}

void DumpHexData(const unsigned char *buffer, unsigned int size)
{
  char message [65536];
  char ascii   [17];

  unsigned int index = 0;
  unsigned int linescan = 0;
  unsigned int index_ascii = 0;

  sprintf  (message,"\n####  Start Dump Buffer of [%.5d] Bytes ####\n\n",size);

  *logofs << message << logofs_flush;

  // 
  // "Index    0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f  Ascii           "
  // "-----   -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --  ----------------"
  // "00000 : 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................"
  // 

  sprintf (message,"Index   0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f  Ascii           \n");
  *logofs << message << logofs_flush;
  sprintf (message,"-----  -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --  ----------------\n");
  *logofs << message << logofs_flush;

  index = 0;

  while (index < size)
  {
    memset (ascii, ' ', sizeof(ascii));

    ascii[16] = '\0';

    sprintf  (message,"%.5d  ", index);

    for (index_ascii = 0, linescan = index;
             ((index < (linescan + 16)) && (index < size)); 
                  index++, index_ascii++)
    {
      if (isprint(buffer [index]))
      {
        ascii[index_ascii] = buffer [index];
      }
      else
      {
        ascii[index_ascii] = '.';
      }

      sprintf  (&message [strlen (message)],"%.2x ", (unsigned char) buffer [index]);
    }

    for (linescan = index_ascii; linescan < 16; linescan++)
    {
      strcat (&message [strlen (message)], "   ");
    }

    sprintf  (&message [strlen (message)]," %s\n", ascii);

    *logofs << message << logofs_flush;
  } 

  sprintf  (message,"\n####  End Dump Buffer ####\n\n");

  *logofs << message << logofs_flush;
}
