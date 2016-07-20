/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2011 NoMachine (http://www.nomachine.com)          */
/* Copyright (c) 2008-2014 Oleksandr Shneyder <o.shneyder@phoca-gmbh.de>  */
/* Copyright (c) 2011-2016 Mike Gabriel <mike.gabriel@das-netzwerkteam.de>*/
/* Copyright (c) 2014-2016 Mihai Moldovan <ionic@ionic.de>                */
/* Copyright (c) 2014-2016 Ulrich Sibiller <uli42@gmx.de>                 */
/* Copyright (c) 2015-2016 Qindel Group (http://www.qindel.com)           */
/*                                                                        */
/* nx-X11, NX protocol compression and NX extensions to this software     */
/* are copyright of the aforementioned persons and companies.             */
/*                                                                        */
/* Redistribution and use of the present software is allowed according    */
/* to terms specified in the file LICENSE which comes in the source       */
/* distribution.                                                          */
/*                                                                        */
/* All rights reserved.                                                   */
/*                                                                        */
/* NOTE: This software has received contributions from various other      */
/* contributors, only the core maintainers and supporters are listed as   */
/* copyright holders. Please contact us, if you feel you should be listed */
/* as copyright holder, as well.                                          */
/*                                                                        */
/**************************************************************************/

/*

Copyright 1993, 1994, 1998  The Open Group
Copyright 2002 Sun Microsystems, Inc.  All rights reserved.

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the copyright holders shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from the copyright holders.

*/

/* Copyright 1993, 1994 NCR Corporation - Dayton, Ohio, USA
 *
 * All Rights Reserved
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name NCR not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission.  NCR makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * NCR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL NCR BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifdef NX_TRANS_SOCKET

#ifdef NX_TRANS_DEBUG
#define XTRANSDEBUG 5
#endif

#ifndef PF_LOCAL
#define PF_LOCAL PF_UNIX
#endif

#endif

#include <ctype.h>
#ifdef XTHREADS
#include <X11/Xthreads.h>
#endif

#ifndef WIN32

#if defined(TCPCONN) || defined(UNIXCONN)
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#if defined(TCPCONN) || defined(UNIXCONN)
#define X_INCLUDE_NETDB_H
#define XOS_USE_NO_LOCKING
#include <nx-X11/Xos_r.h>
#endif

#ifdef UNIXCONN
#ifndef X_NO_SYS_UN
#ifndef Lynx
#include <sys/un.h>
#else
#include <un.h>
#endif
#endif
#include <sys/stat.h>
#endif

#if defined(hpux) || (defined(MOTOROLA) && defined(SYSV))
#define NO_TCP_H
#endif 

#ifndef NO_TCP_H
#if defined(__osf__) || defined(linux) || defined(__GLIBC__) || defined(AIXV5)
#include <sys/param.h>
#endif /* osf */
#if defined(__NetBSD__) || defined(__OpenBSD__) || defined(__FreeBSD__) || defined(__DragonFly__)
#include <sys/param.h>
#include <machine/endian.h>
#endif /* __NetBSD__ || __OpenBSD__ || __FreeBSD__ || __DragonFly__ */
#include <netinet/tcp.h>
#endif /* !NO_TCP_H */

#include <sys/ioctl.h>
#if defined(SVR4) && !defined(DGUX) && !defined(_SEQUENT_)
#include <sys/filio.h>
#endif

#if (defined(i386) && defined(SYSV)) && !defined(SCO325) && !defined(sun)
#include <net/errno.h>
#endif 

#if (defined(i386) && defined(SYSV)) && (!defined(ISC) || !defined(I_NREAD) || defined(SCO325)) || defined(_SEQUENT_)
#include <sys/stropts.h>
#endif 

#else /* !WIN32 */

#include <nx-X11/Xwinsock.h>
#include <nx-X11/Xwindows.h>
#include <X11/Xw32defs.h>
#undef close
#define close closesocket
#define ECONNREFUSED WSAECONNREFUSED
#define EADDRINUSE WSAEADDRINUSE
#define EPROTOTYPE WSAEPROTOTYPE
#undef EWOULDBLOCK
#define EWOULDBLOCK WSAEWOULDBLOCK
#define EINPROGRESS WSAEINPROGRESS
#undef EINTR
#define EINTR WSAEINTR
#define X_INCLUDE_NETDB_H
#define XOS_USE_MTSAFE_NETDBAPI
#include <nx-X11/Xos_r.h>
#endif /* WIN32 */

#if defined(SO_DONTLINGER) && defined(SO_LINGER)
#undef SO_DONTLINGER
#endif

#if defined(__UNIXOS2__)
#if defined(NOT_EMX09A)
static int IBMsockInit = 0;
#define SocketInitOnce()\
    if (!IBMsockInit) {\
	sock_init();\
	IBMsockInit = 1;\
    }


#undef EINTR
#define EINTR SOCEINTR
#undef EINVAL
#define EINVAL SOCEINVAL
#undef errno
#define errno sock_errno()
#undef close
#define close soclose
#undef ioctl
#define ioctl sockioctl
#else
#define SocketInitOnce() /**/
#endif
/* this is still not there */
#define SOCKET int
#else
/* others don't need this */
#define SocketInitOnce() /**/
#endif

#if defined(linux)
#define HAVE_ABSTRACT_SOCKETS
#endif

#define MIN_BACKLOG 128
#ifdef SOMAXCONN
#if SOMAXCONN > MIN_BACKLOG
#define BACKLOG SOMAXCONN
#endif
#endif
#ifndef BACKLOG
#define BACKLOG MIN_BACKLOG
#endif
/*
 * This is the Socket implementation of the X Transport service layer
 *
 * This file contains the implementation for both the UNIX and INET domains,
 * and can be built for either one, or both.
 *
 */

typedef struct _Sockettrans2dev {      
    char	*transname;
    int		family;
    int		devcotsname;
    int		devcltsname;
    int		protocol;
} Sockettrans2dev;

static Sockettrans2dev Sockettrans2devtab[] = {
#ifdef TCPCONN
    {"inet",AF_INET,SOCK_STREAM,SOCK_DGRAM,0},
#if !defined(IPv6) || !defined(AF_INET6)
    {"tcp",AF_INET,SOCK_STREAM,SOCK_DGRAM,0},
#else /* IPv6 */
    {"tcp",AF_INET6,SOCK_STREAM,SOCK_DGRAM,0},
    {"tcp",AF_INET,SOCK_STREAM,SOCK_DGRAM,0}, /* fallback */
    {"inet6",AF_INET6,SOCK_STREAM,SOCK_DGRAM,0},
#endif
#endif /* TCPCONN */
#ifdef UNIXCONN
    {"unix",AF_UNIX,SOCK_STREAM,SOCK_DGRAM,0},
#if !defined(LOCALCONN)
    {"local",AF_UNIX,SOCK_STREAM,SOCK_DGRAM,0},
#endif /* !LOCALCONN */
#endif /* UNIXCONN */
};

#define NUMSOCKETFAMILIES (sizeof(Sockettrans2devtab)/sizeof(Sockettrans2dev))

#ifdef TCPCONN
static int TRANS(SocketINETClose) (XtransConnInfo ciptr);
#endif

#ifdef UNIXCONN

#ifdef hpux

#if defined(X11_t)
#define UNIX_PATH "/usr/spool/sockets/X11/"
#define UNIX_DIR "/usr/spool/sockets/X11"
#define OLD_UNIX_PATH "/tmp/.X11-unix/X"
#endif /* X11_t */
#if defined(XIM_t)
#define UNIX_PATH "/usr/spool/sockets/XIM/"
#define UNIX_DIR "/usr/spool/sockets/XIM"
#define OLD_UNIX_PATH "/tmp/.XIM-unix/XIM"
#endif /* XIM_t */
#if defined(FS_t) || defined(FONT_t)
#define UNIX_PATH "/usr/spool/sockets/fontserv/"
#define UNIX_DIR "/usr/spool/sockets/fontserv"
#endif /* FS_t || FONT_t */
#if defined(ICE_t)
#define UNIX_PATH "/usr/spool/sockets/ICE/"
#define UNIX_DIR "/usr/spool/sockets/ICE"
#endif /* ICE_t */
#if defined(TEST_t)
#define UNIX_PATH "/usr/spool/sockets/xtrans_test/"
#define UNIX_DIR "/usr/spool/sockets/xtrans_test"
#endif
#if defined(LBXPROXY_t)
#define UNIX_PATH "/usr/spool/sockets/X11/"
#define UNIX_DIR  "/usr/spool/sockets/X11"
#endif

#else /* !hpux */

#if defined(X11_t)
#define UNIX_PATH "/tmp/.X11-unix/X"
#define UNIX_DIR "/tmp/.X11-unix"
#endif /* X11_t */
#if defined(XIM_t)
#define UNIX_PATH "/tmp/.XIM-unix/XIM"
#define UNIX_DIR "/tmp/.XIM-unix"
#endif /* XIM_t */
#if defined(FS_t) || defined(FONT_t)
#define UNIX_PATH "/tmp/.font-unix/fs"
#define UNIX_DIR "/tmp/.font-unix"
#endif /* FS_t || FONT_t */
#if defined(ICE_t)
#define UNIX_PATH "/tmp/.ICE-unix/"
#define UNIX_DIR "/tmp/.ICE-unix"
#endif /* ICE_t */
#if defined(TEST_t)
#define UNIX_PATH "/tmp/.Test-unix/test"
#define UNIX_DIR "/tmp/.Test-unix"
#endif
#if defined(LBXPROXY_t)
#define UNIX_PATH "/tmp/.X11-unix/X"
#define UNIX_DIR  "/tmp/.X11-unix"
#endif

#endif /* hpux */

#endif /* UNIXCONN */

#define PORTBUFSIZE	32

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 255
#endif

/*
 * This provides compatibility for apps linked against system libraries
 * that don't have IPv6 support.
 */
#if defined(IPv6) && defined(AF_INET6)
static const struct in6_addr local_in6addr_any = IN6ADDR_ANY_INIT;
#pragma weak in6addr_any = local_in6addr_any
#ifndef __USLC__
#pragma weak getaddrinfo
#endif
static int haveIPv6 = 1;
#endif

#ifndef X11_t

/*
 * No NX changes if this is not
 * compiled as a X11 transport.
 */

#undef NX_TRANS_SOCKET

#endif

#ifdef NX_TRANS_SOCKET

#ifdef TRANS_CLIENT

#include <nx/NX.h>

typedef struct
{
    XtransConnInfo info;
    int            local;
    int            remote;
    int            congestion;

} _NXProxyConnInfo;

#define NX_PROXY_CONN_LIMIT     256

static _NXProxyConnInfo *_NXProxyConnInfoTab[NX_PROXY_CONN_LIMIT];

#endif /* #ifdef TRANS_CLIENT */

/*
 * Override the UNIX_DIR and UNIX_PATH settings and
 * make them configurable, based on the NX_TEMP or
 * the TEMP environment.
 *
 * We must be careful as the same defines are used
 * for different directories, based on the subsystem
 * that is compiling this, while we want to override
 * only the '/tmp/.X11-unix' and '/tmp/.X11-unix/X'
 * settings.
 */

static char _NXUnixDir[1024];
static char _NXUnixPath[1024];

static char *_NXGetUnixDir(char *dir)
{
    const char *tempDir;

    PRMSG (3, "_NXGetUnixDir(%s)\n", dir, 0, 0);

    if (strcmp(dir, UNIX_DIR) != 0)
    {
#ifdef NX_TRANS_TEST
        fprintf(stderr, "_NXGetUnixDir: Returning other Unix directory [%s].\n", dir);
#endif
        return dir;
    }

    /*
     * Check the environment only once.
     */

    if (*_NXUnixDir != '\0')
    {
        return _NXUnixDir;
    }

#ifdef NX_TRANS_TEST
    fprintf(stderr, "_NXGetUnixDir: Trying with the NX_TEMP environment.\n");
#endif

    tempDir = getenv("NX_TEMP");

    if (tempDir == NULL || *tempDir == '\0')
    {
#ifdef NX_TRANS_TEST
        fprintf(stderr, "_NXGetUnixDir: Trying with the TEMP environment.\n");
#endif

        tempDir = getenv("TEMP");
    }

    if (tempDir != NULL && *tempDir != '\0')
    {
        if (strlen(tempDir) + strlen("/.X11-unix") + 1 > 1024)
        {
#ifdef NX_TRANS_TEST
            fprintf(stderr, "_NXGetUnixDir: WARNING! Maximum length of X11 Unix directory exceeded.\n");
#endif
            goto _NXGetUnixDirError;
        }

        strcpy(_NXUnixDir, tempDir);
        strcat(_NXUnixDir, "/.X11-unix");

#ifdef NX_TRANS_TEST
        fprintf(stderr, "_NXGetUnixDir: Using X11 Unix directory [%s].\n", _NXUnixDir);
#endif

        return _NXUnixDir;
    }

_NXGetUnixDirError:

    strcpy(_NXUnixDir, dir);
 
#ifdef NX_TRANS_TEST
    fprintf(stderr, "_NXGetUnixDir: Returning default X11 Unix directory [%s].\n", _NXUnixDir);
#endif

    return _NXUnixDir;
}

static char *_NXGetUnixPath(char *path)
{
    const char *unixDir;

    PRMSG (3, "_NXGetUnixPath(%s)\n", path, 0, 0);

    if (strcmp(path, UNIX_PATH) != 0)
    {
#ifdef NX_TRANS_TEST
        fprintf(stderr, "_NXGetUnixPath: Returning other X11 Unix path [%s].\n", path);
#endif
        return path;
    }

    /*
     * Check the environment only once.
     */

    if (*_NXUnixPath != '\0')
    {
        return _NXUnixPath;
    }

    unixDir = _NXGetUnixDir(UNIX_DIR);

#ifdef NX_TRANS_TEST
    fprintf(stderr, "_NXGetUnixPath: Got X11 Unix directory [%s].\n", unixDir);
#endif

    if (strlen(unixDir) + strlen("/X") + 1 > 1024)
    {
#ifdef NX_TRANS_TEST
        fprintf(stderr, "_NXGetUnixPath: WARNING! Maximum length of X11 Unix path exceeded.\n");
#endif

        goto _NXGetUnixPathError;
    }

    strcpy(_NXUnixPath, unixDir);
    strcat(_NXUnixPath, "/X");

#ifdef NX_TRANS_TEST
    fprintf(stderr, "_NXGetUnixPath: Returning X11 Unix path [%s].\n", _NXUnixPath);
#endif

    return _NXUnixPath;

_NXGetUnixPathError:

    strcpy(_NXUnixPath, path);

#ifdef NX_TRANS_TEST
    fprintf(stderr, "_NXGetUnixPath: Returning default X11 Unix path [%s].\n", _NXUnixPath);
#endif

    return _NXUnixPath;
}

#ifdef hpux

static char *_NXGetOldUnixPath(char *path)
{
    PRMSG (3, "_NXGetOldUnixPath(%s)\n", path, 0, 0);

    if (strcmp(path, OLD_UNIX_PATH) == 0)
    {
#ifdef NX_TRANS_TEST
        fprintf(stderr, "_NXGetOldUnixPath: Returning X11 Unix path [%s].\n",
                    _NXGetUnixPath(path));
#endif

        return _NXGetUnixPath(path);
    }

#ifdef NX_TRANS_TEST
    fprintf(stderr, "_NXGetOldUnixPath: Returning other old X11 Unix path [%s].\n", path);
#endif

    return path;
}

#endif /* #ifdef hpux */

/*
 * Forcibly close any connection attempt on the
 * listening socket. Need this to avoid loopback
 * connections to the agent server.
 */

#ifdef TRANS_SERVER

void TRANS(SocketRejectConnection) (XtransConnInfo ciptr)
{
    socklen_t sa_l = sizeof(struct sockaddr);
    struct sockaddr sa;
    fd_set fs;
    struct timeval t;
    int f;

    PRMSG (3, "SocketRejectConnection(%x)\n", ciptr, 0, 0);

    FD_ZERO(&fs);
    FD_SET(ciptr -> fd, &fs);

    t.tv_sec  = 0;
    t.tv_usec = 0;

    /*
     * Check if there is an awaiting connection.
     */

    if (select(ciptr -> fd + 1, &fs, NULL, NULL, &t) == 1)
    {
#ifdef NX_TRANS_TEST
        fprintf(stderr, "SocketRejectConnection: Accepting connection attempt on fd [%d].\n",
                    ciptr -> fd);
#endif
        /*
         * If there is one, close it.
         */
 
        if ((f = accept(ciptr -> fd, &sa, &sa_l)) >= 0)
        {
#ifdef NX_TRANS_TEST
            fprintf(stderr, "SocketRejectConnection: Closing connection attempt on fd [%d].\n",
                        ciptr -> fd);
#endif
            close(f);
        }
    }
}

#endif /* #ifdef TRANS_SERVER */

#ifdef TRANS_CLIENT

void *TRANS(SocketProxyConnInfo) (XtransConnInfo ciptr)
{
    if (_NXProxyConnInfoTab[ciptr->fd] != NULL)
    {
      return ciptr->priv;
    }

    return NULL;
}

static XtransConnInfo TRANS(SocketCreateConnInfo) ()
{
    XtransConnInfo ciptr;

    int fds[2];

#ifdef NX_TRANS_TEST
    fprintf(stderr, "SocketCreateConnInfo: Going to create the NX connection info.\n");
#endif

    if ((ciptr = (XtransConnInfo) calloc (1, sizeof(struct _XtransConnInfo))) == NULL)
    {
        PRMSG (1, "SocketCreateConnInfo: malloc failed\n", 0, 0, 0);
        return NULL;
    }

    /*
     * Create a pair of sockets. We'll communicate with
     * the NX proxy by reading and writing to our end.
     */

#ifdef NX_TRANS_TEST
    fprintf(stderr, "SocketCreateConnInfo: Going to create the NX socketpair.\n");
#endif

    if (socketpair(PF_LOCAL, SOCK_STREAM, 0, fds) < 0)
    {
        PRMSG (1, "SocketCreateConnInfo: socketpair() failed.\n", 0, 0, 0);
        free ((char *) ciptr);
        return NULL;
    }

#ifdef NX_TRANS_TEST
    fprintf(stderr, "SocketCreateConnInfo: X socket end is [%d] NX proxy end is [%d].\n",
                fds[0], fds[1]);
#endif

    /*
     * Save in _NXProxyConnInfoTab the local and remote end of
     * the socketpair. The remote end will be used by the proxy.
     * When the memory-to-memory transport is activated, the
     * agent and the proxy don't read or write to the real des-
     * criptors but the communication takes place by reading
     * and writing to the proxy's buffers.
     */

    ciptr->fd = fds[0];

    if (ciptr->fd >= NX_PROXY_CONN_LIMIT)
    {
        PRMSG (1, "SocketCreateConnInfo: No space for a new _NXProxyConnInfo for [%d].\n",
                  ciptr->fd, 0, 0);
        free ((char *) ciptr);
        return NULL;
    }
    else if (_NXProxyConnInfoTab[ciptr->fd] != NULL)
    {
        PRMSG (1, "SocketCreateConnInfo: _NXProxyConnInfo for [%d] is not NULL. Exiting.\n",
                   ciptr->fd, 0, 0);
        exit(1);
    }

    _NXProxyConnInfoTab[ciptr->fd] = (_NXProxyConnInfo *) calloc(1, sizeof(_NXProxyConnInfo));

    if (_NXProxyConnInfoTab[ciptr->fd] == NULL)
    {
        PRMSG (1, "SocketCreateConnInfo: Alloc of _NXProxyConnInfo failed.\n", 0, 0, 0);
        free ((char *) ciptr);
        return NULL;
    }

#ifdef NX_TRANS_TEST
    fprintf(stderr, "SocketCreateConnInfo: Allocated new _NXProxyConnInfo for [%d].\n",
                ciptr->fd);
#endif

    _NXProxyConnInfoTab[ciptr->fd]->info       = ciptr;
    _NXProxyConnInfoTab[ciptr->fd]->local      = fds[0];
    _NXProxyConnInfoTab[ciptr->fd]->remote     = fds[1];
    _NXProxyConnInfoTab[ciptr->fd]->congestion = 0;

    ciptr->priv = (char *) _NXProxyConnInfoTab[ciptr->fd];

    return ciptr;
}

static int TRANS(SocketConnectConnInfo) (XtransConnInfo ciptr, char *host, char *port)
{
    int fds[2];
    char display[1024];

    _NXProxyConnInfo *proxy_conn;

#ifdef NX_TRANS_TEST
    fprintf(stderr, "SocketConnectConnInfo: Going to connect NX fd [%d] to host [%s] port [%s].\n",
                ciptr->fd, host, port);
#endif

    /*
     * We should have already created the socket pair.
     */

    proxy_conn = (_NXProxyConnInfo *) ciptr->priv;

    if (proxy_conn == NULL)
    {
        PRMSG (1, "SocketConnectConnInfo: Pointer to _NXProxyConnInfo is NULL. Exiting.\n", 0, 0, 0);

        exit(1);
    }
    else if (_NXProxyConnInfoTab[ciptr->fd] != (_NXProxyConnInfo *) ciptr->priv)
    {
        PRMSG (1, "SocketConnectConnInfo: Can't find _NXProxyConnInfo in table. Exiting.\n",
                   0, 0, 0);

        exit(1);
    }

    if (strlen(host) + strlen(port) + 1 >= 1023)
    {
        PRMSG (1, "SocketConnectConnInfo: Length of NX display string '%s:%s' would exceed %d characters.\n",
                   host, port, 1023);

        return TRANS_CONNECT_FAILED;
    }

    sprintf(display, "%s:%s", host, port);

#ifdef NX_TRANS_TEST
    fprintf(stderr, "SocketConnectConnInfo: Setting close-on-exec flag on local NX descriptor [%d].\n",
                proxy_conn -> local);
#endif

#ifdef F_SETFD
#ifdef FD_CLOEXEC
    if (fcntl(proxy_conn -> local, F_SETFD, FD_CLOEXEC) != 0)
#else
    if (fcntl(proxy_conn -> local, F_SETFD, 1) != 0)
#endif
#endif
    {
        PRMSG (1, "SocketConnectConnInfo: Cannot set close-on-exec on local NX descriptor [%d].\n",
                   proxy_conn -> local, 0, 0);

        return TRANS_CONNECT_FAILED;
    }

#ifdef NX_TRANS_TEST
    fprintf(stderr, "SocketConnectConnInfo: Creating the NX transport with display [%s].\n",
                display);
#endif

    if (NXTransCreate(NX_FD_ANY, NX_MODE_CLIENT, display) < 0)
    {
        PRMSG (1, "SocketConnectConnInfo: Cannot create the NX transport.\n",
                   0, 0, 0);

        return TRANS_CONNECT_FAILED;
    }

#ifdef NX_TRANS_TEST
    fprintf(stderr, "SocketConnectConnInfo: Starting the NX agent with descriptor [%d].\n",
                proxy_conn -> remote);
#endif

    fds[0] = proxy_conn -> local;
    fds[1] = proxy_conn -> remote;

    NXTransAgent(fds);

    return 0;
}

static void TRANS(SocketCloseConnInfo) (XtransConnInfo ciptr)
{
    _NXProxyConnInfo *proxy_conn;

#ifdef NX_TRANS_TEST
    fprintf(stderr, "SocketCloseConnInfo: Going to close the NX fd [%d].\n", ciptr->fd);
#endif

    proxy_conn = (_NXProxyConnInfo *) ciptr->priv;

    if (proxy_conn == NULL)
    {
        PRMSG (1, "SocketCloseConnInfo: Pointer to _NXProxyConnInfo is NULL. Exiting.\n", 0, 0, 0);

        exit(1);
    }
    else if (ciptr->fd >= NX_PROXY_CONN_LIMIT ||
                 _NXProxyConnInfoTab[ciptr->fd] != (_NXProxyConnInfo *) ciptr->priv)
    {
        PRMSG (1, "SocketCloseConnInfo: Can't find _NXProxyConnInfo in table. Exiting.\n",
                   0, 0, 0);
        exit(1);
    }
    else if (_NXProxyConnInfoTab[ciptr->fd] -> info != ciptr ||
                 _NXProxyConnInfoTab[ciptr->fd] -> local != ciptr->fd)
    {
        PRMSG (1, "SocketCloseConnInfo: Invalid _NXProxyConnInfo structure for [%d]. Exiting.\n",
                   ciptr->fd, 0, 0);
        exit(1);
    }
    else if (proxy_conn->local < 0 || proxy_conn->remote < 0)
    {
        PRMSG (1, "SocketCloseConnInfo: Invalid socket pair in NX connection for [%d]. Exiting.\n",
                   ciptr->fd, 0, 0);
        exit(1);
    }

    NXTransClose(ciptr->fd);

    /*
     *  Get rid of the _NXProxyConnInfo structure.
     */

#ifdef NX_TRANS_TEST
    fprintf(stderr, "SocketCloseConnInfo: Freeing _NXProxyConnInfo structure for [%d].\n",
                ciptr->fd);
#endif

    free((char *) _NXProxyConnInfoTab[ciptr->fd]);

    _NXProxyConnInfoTab[ciptr->fd] = NULL;

    ciptr->priv = NULL;

#ifdef NX_TRANS_TEST
    fprintf(stderr, "SocketCloseConnInfo: Should now close the local descriptor [%d].\n",
                ciptr->fd);
#endif
}

#endif /* #ifdef TRANS_CLIENT */

#if defined(TRANS_CLIENT) && defined(NX_TRANS_CHANGE)

/*
 * Check the congestion state of the NX transport
 * and return 1 if there has been a change. This
 * can be extended by adding a few counters track-
 * ing the bandwidth usage of the X11 connection.
 */

int TRANS(SocketCongestionChange) (XtransConnInfo ciptr, int *state)
{
    int congestion;

    _NXProxyConnInfo *proxy_conn;

    PRMSG (3, "SocketCongestionChange(%x)\n", ciptr, 0, 0);

    proxy_conn = (_NXProxyConnInfo *) ciptr->priv;

    if (proxy_conn == NULL)
    {
#ifdef NX_TRANS_DEBUG
        fprintf(stderr, "SocketCongestionChange: Descriptor [%d] doesn't appear to be a NX connection.\n",
                    ciptr->fd);
#endif
        return 0;
    }

#ifdef NX_TRANS_DEBUG
    fprintf(stderr, "SocketCongestionChange: Checking congestion on fd [%d] with old state [%d].\n",
                ciptr->fd, proxy_conn->congestion);
#endif

    congestion = NXTransCongestion(ciptr->fd);

    if (congestion != proxy_conn->congestion)
    {
        proxy_conn->congestion = congestion;

#ifdef NX_TRANS_TEST
        fprintf(stderr, "SocketCongestionChange: Change detected on fd [%d] with new state [%d].\n",
                    ciptr->fd, proxy_conn->congestion);
#endif
        return 1;
    }

#ifdef NX_TRANS_TEST
    fprintf(stderr, "SocketCongestionChange: No change on fd [%d] with current state [%d].\n",
                ciptr->fd, congestion);
#endif
    return 0;
}

#endif /* #if defined(TRANS_CLIENT) && defined(NX_TRANS_CHANGE) */

#endif /* #ifdef NX_TRANS_SOCKET */

/*
 * These are some utility function used by the real interface function below.
 */

static int
TRANS(SocketSelectFamily) (int first, char *family)

{
    int     i;

    PRMSG (3,"SocketSelectFamily(%s)\n", family, 0, 0);

    for (i = first + 1; i < NUMSOCKETFAMILIES;i++)
    {
        if (!strcmp (family, Sockettrans2devtab[i].transname))
	    return i;
    }

    return (first == -1 ? -2 : -1);
}


/*
 * This function gets the local address of the socket and stores it in the
 * XtransConnInfo structure for the connection.
 */

static int
TRANS(SocketINETGetAddr) (XtransConnInfo ciptr)

{
#if defined(IPv6) && defined(AF_INET6)
    struct sockaddr_storage socknamev6;
#endif
    struct sockaddr_in socknamev4;
    void *socknamePtr;
#if defined(SVR4) || defined(__SCO__)
#   define SOCKLEN_T_PTR void*
    size_t namelen;
#else
#   define SOCKLEN_T_PTR socklen_t*
    socklen_t namelen;
#endif

    PRMSG (3,"SocketINETGetAddr(%p)\n", ciptr, 0, 0);

#if defined(IPv6) && defined(AF_INET6)
    if (haveIPv6)
    {
	namelen = sizeof(socknamev6);
	socknamePtr = &socknamev6;
    }
    else
#endif
    {
	namelen = sizeof(socknamev4);
	socknamePtr = &socknamev4;
    }

    if (getsockname (ciptr->fd,(struct sockaddr *) socknamePtr,
		     (SOCKLEN_T_PTR)&namelen) < 0)
    {
#ifdef WIN32
	errno = WSAGetLastError();
#endif
	PRMSG (1,"SocketINETGetAddr: getsockname() failed: %d\n",
	    EGET(),0, 0);
	return -1;
    }

    /*
     * Everything looks good: fill in the XtransConnInfo structure.
     */

    if ((ciptr->addr = (char *) malloc (namelen)) == NULL)
    {
        PRMSG (1,
	    "SocketINETGetAddr: Can't allocate space for the addr\n",
	    0, 0, 0);
        return -1;
    }

#if defined(IPv6) && defined(AF_INET6)
    if (haveIPv6)
    {
	ciptr->family = ((struct sockaddr *)socknamePtr)->sa_family;
    }
    else
#endif
    {
	ciptr->family = socknamev4.sin_family;
    }
    ciptr->addrlen = namelen;
    memcpy (ciptr->addr, socknamePtr, ciptr->addrlen);

    return 0;
}


/*
 * This function gets the remote address of the socket and stores it in the
 * XtransConnInfo structure for the connection.
 */

static int
TRANS(SocketINETGetPeerAddr) (XtransConnInfo ciptr)

{
#if defined(IPv6) && defined(AF_INET6)
    struct sockaddr_storage socknamev6;
#endif
    struct sockaddr_in 	socknamev4;
    void *socknamePtr;
#if defined(SVR4) || defined(__SCO__)
    size_t namelen;
#else
    int namelen;
#endif

#if defined(IPv6) && defined(AF_INET6)
    if (haveIPv6 && ciptr->family == AF_INET6)
    {
	namelen = sizeof(socknamev6);
	socknamePtr = &socknamev6;
    }
    else
#endif
    {
	namelen = sizeof(socknamev4);
	socknamePtr = &socknamev4;
    }

    PRMSG (3,"SocketINETGetPeerAddr(%p)\n", ciptr, 0, 0);

    if (getpeername (ciptr->fd, (struct sockaddr *) socknamePtr,
		     (void *)&namelen) < 0)
    {
#ifdef WIN32
	errno = WSAGetLastError();
#endif
	PRMSG (1,"SocketINETGetPeerAddr: getpeername() failed: %d\n",
	    EGET(), 0, 0);
	return -1;
    }

    /*
     * Everything looks good: fill in the XtransConnInfo structure.
     */

    if ((ciptr->peeraddr = (char *) malloc (namelen)) == NULL)
    {
        PRMSG (1,
	   "SocketINETGetPeerAddr: Can't allocate space for the addr\n",
	   0, 0, 0);
        return -1;
    }

    ciptr->peeraddrlen = namelen;
    memcpy (ciptr->peeraddr, socknamePtr, ciptr->peeraddrlen);

    return 0;
}


static XtransConnInfo
TRANS(SocketOpen) (int i, int type)

{
    XtransConnInfo	ciptr;

    PRMSG (3,"SocketOpen(%d,%d)\n", i, type, 0);

#if defined(IPv6) && defined(AF_INET6)
    if (getaddrinfo == NULL)
	haveIPv6 = 0;

    if (!haveIPv6 && Sockettrans2devtab[i].family == AF_INET6)
	return NULL;
#endif

    if ((ciptr = (XtransConnInfo) calloc (
	1, sizeof(struct _XtransConnInfo))) == NULL)
    {
	PRMSG (1, "SocketOpen: malloc failed\n", 0, 0, 0);
	return NULL;
    }

    if ((ciptr->fd = socket(Sockettrans2devtab[i].family, type,
	Sockettrans2devtab[i].protocol)) < 0
#ifndef WIN32
#if (defined(X11_t) && !defined(USE_POLL)) || defined(FS_t) || defined(FONT_t)
       || ciptr->fd >= TRANS_OPEN_MAX
#endif
#endif
      ) {
#ifdef WIN32
	errno = WSAGetLastError();
#endif
	PRMSG (2, "SocketOpen: socket() failed for %s\n",
	    Sockettrans2devtab[i].transname, 0, 0);

	free ((char *) ciptr);
	return NULL;
    }

#ifdef TCP_NODELAY
    if (Sockettrans2devtab[i].family == AF_INET
#if defined(IPv6) && defined(AF_INET6)
      || Sockettrans2devtab[i].family == AF_INET6
#endif
    )
    {
	/*
	 * turn off TCP coalescence for INET sockets
	 */

	int tmp = 1;
	setsockopt (ciptr->fd, IPPROTO_TCP, TCP_NODELAY,
	    (char *) &tmp, sizeof (int));
    }
#endif

    return ciptr;
}


#ifdef TRANS_REOPEN

static XtransConnInfo
TRANS(SocketReopen) (int i, int type, int fd, char *port)

{
    XtransConnInfo	ciptr;

    PRMSG (3,"SocketReopen(%d,%d,%s)\n", type, fd, port);

    if ((ciptr = (XtransConnInfo) calloc (
	1, sizeof(struct _XtransConnInfo))) == NULL)
    {
	PRMSG (1, "SocketReopen: malloc failed\n", 0, 0, 0);
	return NULL;
    }

    ciptr->fd = fd;

    return ciptr;
}

#endif /* TRANS_REOPEN */


/*
 * These functions are the interface supplied in the Xtransport structure
 */

#ifdef TRANS_CLIENT

static XtransConnInfo
TRANS(SocketOpenCOTSClientBase) (char *transname, char *protocol,
				char *host, char *port, int previndex)
{
    XtransConnInfo	ciptr;
    int			i = previndex;

    PRMSG (2, "SocketOpenCOTSClient(%s,%s,%s)\n",
	protocol, host, port);

    SocketInitOnce();

    while ((i = TRANS(SocketSelectFamily) (i, transname)) >= 0) {

#if defined(NX_TRANS_SOCKET) && defined(TRANS_CLIENT)

        if ((!strcmp(protocol, "local") || !strcmp(protocol, "nx")) &&
                (!strcasecmp(host, "nx") || !strncasecmp(host, "nx,", 3)))
        {
            ciptr = TRANS(SocketCreateConnInfo) ();

            if (ciptr == NULL)
            {
                PRMSG (1, "SocketOpenCOTSClient: Unable to create the NX connection info for %s.\n",
                           transname, 0, 0);

                return NULL;
            }

            ciptr->index = i;

            return ciptr;
        }

#endif /* #if defined(NX_TRANS_SOCKET) && defined(TRANS_CLIENT) */

	if ((ciptr = TRANS(SocketOpen) (
		 i, Sockettrans2devtab[i].devcotsname)) != NULL)
	    break;
    }
    if (i < 0) {
	if (i == -1)
	    PRMSG (1,"SocketOpenCOTSClient: Unable to open socket for %s\n",
		   transname, 0, 0);
	else
	    PRMSG (1,"SocketOpenCOTSClient: Unable to determine socket type for %s\n",
		   transname, 0, 0);
	return NULL;
    }

#if defined(NX_TRANS_SOCKET) && defined(TRANS_CLIENT)

    ciptr->priv = NULL;

#endif

    /* Save the index for later use */

    ciptr->index = i;

    return ciptr;
}

static XtransConnInfo
TRANS(SocketOpenCOTSClient) (Xtransport *thistrans, char *protocol, 
			     char *host, char *port)
{
    return TRANS(SocketOpenCOTSClientBase)(
			thistrans->TransName, protocol, host, port, -1);
}


#endif /* TRANS_CLIENT */


#ifdef TRANS_SERVER

static XtransConnInfo
TRANS(SocketOpenCOTSServer) (Xtransport *thistrans, char *protocol, 
			     char *host, char *port)

{
    XtransConnInfo	ciptr;
    int	i = -1;

    PRMSG (2,"SocketOpenCOTSServer(%s,%s,%s)\n", protocol, host, port);

    SocketInitOnce();

    while ((i = TRANS(SocketSelectFamily) (i, thistrans->TransName)) >= 0) {
	if ((ciptr = TRANS(SocketOpen) (
		 i, Sockettrans2devtab[i].devcotsname)) != NULL)
	    break;
    }
    if (i < 0) {
	if (i == -1)
	    PRMSG (1,"SocketOpenCOTSServer: Unable to open socket for %s\n",
		   thistrans->TransName, 0, 0);
	else
	    PRMSG (1,"SocketOpenCOTSServer: Unable to determine socket type for %s\n",
		   thistrans->TransName, 0, 0);
	return NULL;
    }

    /*
     * Using this prevents the bind() check for an existing server listening
     * on the same port, but it is required for other reasons.
     */
#ifdef SO_REUSEADDR

    /*
     * SO_REUSEADDR only applied to AF_INET && AF_INET6
     */

    if (Sockettrans2devtab[i].family == AF_INET
#if defined(IPv6) && defined(AF_INET6)
      || Sockettrans2devtab[i].family == AF_INET6
#endif
    )
    {
	int one = 1;
	setsockopt (ciptr->fd, SOL_SOCKET, SO_REUSEADDR,
		    (char *) &one, sizeof (int));
    }
#endif
#ifdef IPV6_V6ONLY
    if (Sockettrans2devtab[i].family == AF_INET6)
    {
	int one = 1;
	setsockopt(ciptr->fd, IPPROTO_IPV6, IPV6_V6ONLY, &one, sizeof(int));
    }
#endif
    /* Save the index for later use */

    ciptr->index = i;

    return ciptr;
}

#endif /* TRANS_SERVER */


#ifdef TRANS_CLIENT

static XtransConnInfo
TRANS(SocketOpenCLTSClient) (Xtransport *thistrans, char *protocol, 
			     char *host, char *port)

{
    XtransConnInfo	ciptr;
    int			i = -1;

    PRMSG (2,"SocketOpenCLTSClient(%s,%s,%s)\n", protocol, host, port);

    SocketInitOnce();

    while ((i = TRANS(SocketSelectFamily) (i, thistrans->TransName)) >= 0) {

#if defined(NX_TRANS_SOCKET) && defined(TRANS_CLIENT)

        if ((!strcmp(protocol, "local") || !strcmp(protocol, "nx")) &&
                (!strcasecmp(host, "nx") || !strncasecmp(host, "nx,", 3)))
        {
            ciptr = TRANS(SocketCreateConnInfo) ();

            if (ciptr == NULL)
            {
                PRMSG (1, "SocketOpenCLTSClient: Unable to create the NX connection info for %s.\n",
                           thistrans->TransName, 0, 0);

                return NULL;
            }

            ciptr->index = i;

            return ciptr;
        }

#endif /* #if defined(NX_TRANS_SOCKET) && defined(TRANS_CLIENT) */

	if ((ciptr = TRANS(SocketOpen) (
		 i, Sockettrans2devtab[i].devcotsname)) != NULL)
	    break;
    }
    if (i < 0) {
	if (i == -1)
	    PRMSG (1,"SocketOpenCLTSClient: Unable to open socket for %s\n",
		   thistrans->TransName, 0, 0);
	else
	    PRMSG (1,"SocketOpenCLTSClient: Unable to determine socket type for %s\n",
		   thistrans->TransName, 0, 0);
	return NULL;
    }

#if defined(NX_TRANS_SOCKET) && defined(TRANS_CLIENT)

    ciptr->priv = NULL;

#endif /* #if defined(NX_TRANS_SOCKET) && defined(TRANS_CLIENT) */

    /* Save the index for later use */

    ciptr->index = i;

    return ciptr;
}

#endif /* TRANS_CLIENT */


#ifdef TRANS_SERVER

static XtransConnInfo
TRANS(SocketOpenCLTSServer) (Xtransport *thistrans, char *protocol, 
			     char *host, char *port)

{
    XtransConnInfo	ciptr;
    int	i = -1;

    PRMSG (2,"SocketOpenCLTSServer(%s,%s,%s)\n", protocol, host, port);

    SocketInitOnce();

    while ((i = TRANS(SocketSelectFamily) (i, thistrans->TransName)) >= 0) {
	if ((ciptr = TRANS(SocketOpen) (
		 i, Sockettrans2devtab[i].devcotsname)) != NULL)
	    break;
    }
    if (i < 0) {
	if (i == -1)
	    PRMSG (1,"SocketOpenCLTSServer: Unable to open socket for %s\n",
		   thistrans->TransName, 0, 0);
	else
	    PRMSG (1,"SocketOpenCLTSServer: Unable to determine socket type for %s\n",
		   thistrans->TransName, 0, 0);
	return NULL;
    }

#ifdef IPV6_V6ONLY
    if (Sockettrans2devtab[i].family == AF_INET6)
    {
	int one = 1;
	setsockopt(ciptr->fd, IPPROTO_IPV6, IPV6_V6ONLY, &one, sizeof(int));
    }
#endif
    /* Save the index for later use */

    ciptr->index = i;

    return ciptr;
}

#endif /* TRANS_SERVER */


#ifdef TRANS_REOPEN

static XtransConnInfo
TRANS(SocketReopenCOTSServer) (Xtransport *thistrans, int fd, char *port)

{
    XtransConnInfo	ciptr;
    int			i = -1;

    PRMSG (2,
	"SocketReopenCOTSServer(%d, %s)\n", fd, port, 0);

    SocketInitOnce();

    while ((i = TRANS(SocketSelectFamily) (i, thistrans->TransName)) >= 0) {
	if ((ciptr = TRANS(SocketReopen) (
		 i, Sockettrans2devtab[i].devcotsname, fd, port)) != NULL)
	    break;
    }
    if (i < 0) {
	if (i == -1)
	    PRMSG (1,"SocketReopenCOTSServer: Unable to open socket for %s\n",
		   thistrans->TransName, 0, 0);
	else
	    PRMSG (1,"SocketReopenCOTSServer: Unable to determine socket type for %s\n",
		   thistrans->TransName, 0, 0);
	return NULL;
    }

    /* Save the index for later use */

    ciptr->index = i;

    return ciptr;
}

static XtransConnInfo
TRANS(SocketReopenCLTSServer) (Xtransport *thistrans, int fd, char *port)

{
    XtransConnInfo	ciptr;
    int			i = -1;

    PRMSG (2,
	"SocketReopenCLTSServer(%d, %s)\n", fd, port, 0);

    SocketInitOnce();

    while ((i = TRANS(SocketSelectFamily) (i, thistrans->TransName)) >= 0) {
	if ((ciptr = TRANS(SocketReopen) (
		 i, Sockettrans2devtab[i].devcotsname, fd, port)) != NULL)
	    break;
    }
    if (i < 0) {
	if (i == -1)
	    PRMSG (1,"SocketReopenCLTSServer: Unable to open socket for %s\n",
		   thistrans->TransName, 0, 0);
	else
	    PRMSG (1,"SocketReopenCLTSServer: Unable to determine socket type for %s\n",
		   thistrans->TransName, 0, 0);
	return NULL;
    }

    /* Save the index for later use */

    ciptr->index = i;

    return ciptr;
}

#endif /* TRANS_REOPEN */


static int
TRANS(SocketSetOption) (XtransConnInfo ciptr, int option, int arg)

{
    PRMSG (2,"SocketSetOption(%d,%d,%d)\n", ciptr->fd, option, arg);

#ifdef NX_TRANS_TEST
    fprintf(stderr, "SocketSetOption: WARNING! Not setting option [%d] with value [%d] on descriptor [%d].\n",
                option, arg, ciptr -> fd);
#endif

    return -1;
}

#ifdef UNIXCONN
static int
set_sun_path(const char *port, const char *upath, char *path, int abstract)
{
    struct sockaddr_un s;
    int maxlen = sizeof(s.sun_path) - 1;
    const char *at = "";

    if (!port || !*port || !path)
	return -1;

#ifdef HAVE_ABSTRACT_SOCKETS
    if (port[0] == '@')
	upath = "";
    else if (abstract)
	at = "@";
#endif

    if (*port == '/') /* a full pathname */
	upath = "";

    if (strlen(port) + strlen(upath) > maxlen)
	return -1;
    snprintf(path, sizeof(s.sun_path), "%s%s%s", at, upath, port);
    return 0;
}
#endif

#ifdef TRANS_SERVER

static int
TRANS(SocketCreateListener) (XtransConnInfo ciptr, 
			     struct sockaddr *sockname,
			     int socknamelen, unsigned int flags)

{
    int	namelen = socknamelen;
    int	fd = ciptr->fd;
    int	retry;

    PRMSG (3, "SocketCreateListener(%x,%p)\n", ciptr, fd, 0);

    if (Sockettrans2devtab[ciptr->index].family == AF_INET
#if defined(IPv6) && defined(AF_INET6)
      || Sockettrans2devtab[ciptr->index].family == AF_INET6
#endif
	)
	retry = 20;
    else
	retry = 0;

#if defined(NX_TRANS_SOCKET) && defined(NX_TRANS_TEST)
    fprintf(stderr, "SocketCreateListener: Creating listener for ciptr at [%p] on path [%s].\n",
                (void *) ciptr, ((struct sockaddr_un *) sockname)->sun_family == AF_UNIX ?
                    ((struct sockaddr_un *) sockname)->sun_path : "TCP");
#endif
    while (bind (fd, (struct sockaddr *) sockname, namelen) < 0)
    {
	if (errno == EADDRINUSE) {
	    if (flags & ADDR_IN_USE_ALLOWED)
		break;
	    else
		return TRANS_ADDR_IN_USE;
	}
	
	if (retry-- == 0) {
	    PRMSG (1, "SocketCreateListener: failed to bind listener\n",
		0, 0, 0);
	    close (fd);
	    return TRANS_CREATE_LISTENER_FAILED;
	}
#ifdef SO_REUSEADDR
	sleep (1);
#else
	sleep (10);
#endif /* SO_REUSEDADDR */
    }

    if (Sockettrans2devtab[ciptr->index].family == AF_INET
#if defined(IPv6) && defined(AF_INET6)
      || Sockettrans2devtab[ciptr->index].family == AF_INET6
#endif
	) {
#ifdef SO_DONTLINGER
	setsockopt (fd, SOL_SOCKET, SO_DONTLINGER, (char *) NULL, 0);
#else
#ifdef SO_LINGER
    {
	static int linger[2] = { 0, 0 };
	setsockopt (fd, SOL_SOCKET, SO_LINGER,
		(char *) linger, sizeof (linger));
    }
#endif
#endif
}

    if (listen (fd, BACKLOG) < 0)
    {
	PRMSG (1, "SocketCreateListener: listen() failed\n", 0, 0, 0);
	close (fd);
	return TRANS_CREATE_LISTENER_FAILED;
    }
	
    /* Set a flag to indicate that this connection is a listener */

    ciptr->flags = 1 | (ciptr->flags & TRANS_KEEPFLAGS);

#if defined(NX_TRANS_SOCKET) && defined(NX_TRANS_TEST)
    fprintf(stderr, "SocketCreateListener: Set flags to [%d] for ciptr [%p].\n",
                ciptr->flags, (void *) ciptr);
#endif

    return 0;
}

#ifdef TCPCONN
static int
TRANS(SocketINETCreateListener) (XtransConnInfo ciptr, char *port, unsigned int flags)

{
#if defined(IPv6) && defined(AF_INET6)
    struct sockaddr_storage sockname;
#else
    struct sockaddr_in	    sockname;
#endif
    unsigned short	    sport;
    int		namelen = sizeof(sockname);
    int		status;
    long	tmpport;
#ifdef XTHREADS_NEEDS_BYNAMEPARAMS
    _Xgetservbynameparams sparams;
#endif
    struct servent *servp;

#ifdef X11_t
    char	portbuf[PORTBUFSIZE];
#endif
    
    PRMSG (2, "SocketINETCreateListener(%s)\n", port, 0, 0);

#ifdef X11_t
    /*
     * X has a well known port, that is transport dependent. It is easier
     * to handle it here, than try and come up with a transport independent
     * representation that can be passed in and resolved the usual way.
     *
     * The port that is passed here is really a string containing the idisplay
     * from ConnectDisplay().
     */

    if (is_numeric (port))
    {
	/* fixup the server port address */
	tmpport = X_TCP_PORT + strtol (port, (char**)NULL, 10);
	sprintf (portbuf,"%lu", tmpport);
	port = portbuf;
    }
#endif

    if (port && *port)
    {
	/* Check to see if the port string is just a number (handles X11) */

	if (!is_numeric (port))
	{
	    if ((servp = _XGetservbyname (port,"tcp",sparams)) == NULL)
	    {
		PRMSG (1,
	     "SocketINETCreateListener: Unable to get service for %s\n",
		      port, 0, 0);
		return TRANS_CREATE_LISTENER_FAILED;
	    }
	    /* we trust getservbyname to return a valid number */
	    sport = servp->s_port;
	}
	else
	{
	    tmpport = strtol (port, (char**)NULL, 10);
	    /* 
	     * check that somehow the port address isn't negative or in
	     * the range of reserved port addresses. This can happen and
	     * be very bad if the server is suid-root and the user does 
	     * something (dumb) like `X :60049`. 
	     */
	    if (tmpport < 1024 || tmpport > USHRT_MAX)
		return TRANS_CREATE_LISTENER_FAILED;

	    sport = (unsigned short) tmpport;
	}
    }
    else
	sport = 0;

    bzero(&sockname, sizeof(sockname));
#if defined(IPv6) && defined(AF_INET6)
    if (Sockettrans2devtab[ciptr->index].family == AF_INET) {
	namelen = sizeof (struct sockaddr_in);
#ifdef BSD44SOCKETS
	((struct sockaddr_in *)&sockname)->sin_len = namelen;
#endif
	((struct sockaddr_in *)&sockname)->sin_family = AF_INET;
	((struct sockaddr_in *)&sockname)->sin_port = htons(sport);
	((struct sockaddr_in *)&sockname)->sin_addr.s_addr = htonl(INADDR_ANY);
    } else {
	namelen = sizeof (struct sockaddr_in6);
#ifdef SIN6_LEN
	((struct sockaddr_in6 *)&sockname)->sin6_len = sizeof(sockname);
#endif
	((struct sockaddr_in6 *)&sockname)->sin6_family = AF_INET6;
	((struct sockaddr_in6 *)&sockname)->sin6_port = htons(sport);
	((struct sockaddr_in6 *)&sockname)->sin6_addr = in6addr_any;
    }
#else
#ifdef BSD44SOCKETS
    sockname.sin_len = sizeof (sockname);
#endif
    sockname.sin_family = AF_INET;
    sockname.sin_port = htons (sport);
    sockname.sin_addr.s_addr = htonl (INADDR_ANY);
#endif

    if ((status = TRANS(SocketCreateListener) (ciptr,
	(struct sockaddr *) &sockname, namelen, flags)) < 0)
    {
	PRMSG (1,
    "SocketINETCreateListener: ...SocketCreateListener() failed\n",
	    0, 0, 0);
	return status;
    }

    if (TRANS(SocketINETGetAddr) (ciptr) < 0)
    {
	PRMSG (1,
       "SocketINETCreateListener: ...SocketINETGetAddr() failed\n",
	    0, 0, 0);
	return TRANS_CREATE_LISTENER_FAILED;
    }

    return 0;
}

#endif /* SOCKCONN */


#ifdef UNIXCONN

static int
TRANS(SocketUNIXCreateListener) (XtransConnInfo ciptr, char *port,
				 unsigned int flags)

{
    struct sockaddr_un	sockname;
    int			namelen;
    int			oldUmask;
    int			status;
    unsigned int	mode;
    char		tmpport[108];

    int			abstract = 0;
#ifdef HAVE_ABSTRACT_SOCKETS
    abstract = ciptr->transptr->flags & TRANS_ABSTRACT;
#endif

    PRMSG (2, "SocketUNIXCreateListener(%s)\n",
	port ? port : "NULL", 0, 0);

    /* Make sure the directory is created */

    oldUmask = umask (0);

#ifdef UNIX_DIR
#ifdef HAS_STICKY_DIR_BIT
    mode = 01777;
#else
    mode = 0777;
#endif
#ifdef NX_TRANS_SOCKET
    if (!abstract && trans_mkdir(_NXGetUnixDir(UNIX_DIR), mode) == -1) {
        PRMSG (1, "SocketUNIXCreateListener: mkdir(%s) failed, errno = %d\n",
               _NXGetUnixDir(UNIX_DIR), errno, 0);
#else
    if (!abstract && trans_mkdir(UNIX_DIR, mode) == -1) {
	PRMSG (1, "SocketUNIXCreateListener: mkdir(%s) failed, errno = %d\n",
	       UNIX_DIR, errno, 0);
#endif
	(void) umask (oldUmask);
	return TRANS_CREATE_LISTENER_FAILED;
    }
#endif

    memset(&sockname, 0, sizeof(sockname));
    sockname.sun_family = AF_UNIX;

    if (!(port && *port)) {
	snprintf (tmpport, sizeof(tmpport), "%s%ld", UNIX_PATH, (long)getpid());
	port = tmpport;
    }
    if (port && *port) {
#ifdef NX_TRANS_SOCKET
        if (set_sun_path(port, _NXGetUnixPath(UNIX_PATH), sockname.sun_path, abstract) != 0) {
#else
	if (set_sun_path(port, UNIX_PATH, sockname.sun_path, abstract) != 0) {
#endif
	    PRMSG (1, "SocketUNIXCreateListener: path too long\n", 0, 0, 0);
	    return TRANS_CREATE_LISTENER_FAILED;
	}
    } else {
#ifdef NX_TRANS_SOCKET
	sprintf (sockname.sun_path, "%s%ld", _NXGetUnixPath(UNIX_PATH), (long)getpid());
#else
	sprintf (sockname.sun_path, "%s%ld", UNIX_PATH, (long)getpid());
#endif
    }

#if (defined(BSD44SOCKETS) || defined(__UNIXWARE__)) && !defined(Lynx)
    sockname.sun_len = strlen(sockname.sun_path);
    namelen = SUN_LEN(&sockname);
#else
    namelen = strlen(sockname.sun_path) + sizeof(sockname.sun_family);
#endif

#if defined(NX_TRANS_SOCKET) && defined(NX_TRANS_TEST)
    fprintf(stderr, "SocketUNIXCreateListener: Unlinking path [%s] for ciptr at [%p].\n",
                sockname.sun_path, (void *) ciptr);
#endif
    if (abstract) {
	sockname.sun_path[0] = '\0';
	namelen = offsetof(struct sockaddr_un, sun_path) + 1 + strlen(&sockname.sun_path[1]);
    }
    else
	unlink (sockname.sun_path);

    if ((status = TRANS(SocketCreateListener) (ciptr,
	(struct sockaddr *) &sockname, namelen, flags)) < 0)
    {
	PRMSG (1,
    "SocketUNIXCreateListener: ...SocketCreateListener() failed\n",
	    0, 0, 0);
	(void) umask (oldUmask);
	return status;
    }

    /*
     * Now that the listener is esablished, create the addr info for
     * this connection. getpeername() doesn't work for UNIX Domain Sockets
     * on some systems (hpux at least), so we will just do it manually, instead
     * of calling something like TRANS(SocketUNIXGetAddr).
     */

    namelen = sizeof (sockname); /* this will always make it the same size */

    if ((ciptr->addr = (char *) malloc (namelen)) == NULL)
    {
        PRMSG (1,
        "SocketUNIXCreateListener: Can't allocate space for the addr\n",
	    0, 0, 0);
	(void) umask (oldUmask);
        return TRANS_CREATE_LISTENER_FAILED;
    }

    if (abstract)
	sockname.sun_path[0] = '@';

    ciptr->family = sockname.sun_family;
    ciptr->addrlen = namelen;
    memcpy (ciptr->addr, &sockname, ciptr->addrlen);

    (void) umask (oldUmask);

    return 0;
}

static int
TRANS(SocketUNIXResetListener) (XtransConnInfo ciptr)

{
    /*
     * See if the unix domain socket has disappeared.  If it has, recreate it.
     */

    struct sockaddr_un 	*unsock = (struct sockaddr_un *) ciptr->addr;
    struct stat		statb;
    int 		status = TRANS_RESET_NOOP;
    unsigned int	mode;
    int abstract = 0;
#ifdef HAVE_ABSTRACT_SOCKETS
    abstract = ciptr->transptr->flags & TRANS_ABSTRACT;
#endif

    PRMSG (3, "SocketUNIXResetListener(%p,%d)\n", ciptr, ciptr->fd, 0);

    if (!abstract && (
	stat (unsock->sun_path, &statb) == -1 ||
        ((statb.st_mode & S_IFMT) !=
#if (defined (sun) && defined(SVR4)) || defined(NCR) || defined(SCO325) || !defined(S_IFSOCK)
	  		S_IFIFO))
#else
			S_IFSOCK)))
#endif
    {
	int oldUmask = umask (0);

#ifdef UNIX_DIR
#ifdef HAS_STICKY_DIR_BIT
	mode = 01777;
#else
	mode = 0777;
#endif
#ifdef NX_TRANS_SOCKET
        if (trans_mkdir(_NXGetUnixDir(UNIX_DIR), mode) == -1) {
            PRMSG (1, "SocketUNIXResetListener: mkdir(%s) failed, errno = %d\n",
            _NXGetUnixDir(UNIX_DIR), errno, 0);
#else
        if (trans_mkdir(UNIX_DIR, mode) == -1) {
            PRMSG (1, "SocketUNIXResetListener: mkdir(%s) failed, errno = %d\n",
	    UNIX_DIR, errno, 0);
#endif
	    (void) umask (oldUmask);
	    return TRANS_RESET_FAILURE;
        }
#endif

	close (ciptr->fd);
	unlink (unsock->sun_path);

	if ((ciptr->fd = socket (AF_UNIX, SOCK_STREAM, 0)) < 0)
	{
	    TRANS(FreeConnInfo) (ciptr);
	    (void) umask (oldUmask);
	    return TRANS_RESET_FAILURE;
	}

	if (bind (ciptr->fd, (struct sockaddr *) unsock, ciptr->addrlen) < 0)
	{
	    close (ciptr->fd);
	    TRANS(FreeConnInfo) (ciptr);
	    return TRANS_RESET_FAILURE;
	}

	if (listen (ciptr->fd, BACKLOG) < 0)
	{
	    close (ciptr->fd);
	    TRANS(FreeConnInfo) (ciptr);
	    (void) umask (oldUmask);
	    return TRANS_RESET_FAILURE;
	}

	umask (oldUmask);

	status = TRANS_RESET_NEW_FD;
    }

    return status;
}

#endif /* UNIXCONN */


#ifdef TCPCONN

static XtransConnInfo
TRANS(SocketINETAccept) (XtransConnInfo ciptr, int *status)

{
    XtransConnInfo	newciptr;
    struct sockaddr_in	sockname;
    int			namelen = sizeof(sockname);

    PRMSG (2, "SocketINETAccept(%p,%d)\n", ciptr, ciptr->fd, 0);

    if ((newciptr = (XtransConnInfo) calloc (
	1, sizeof(struct _XtransConnInfo))) == NULL)
    {
	PRMSG (1, "SocketINETAccept: malloc failed\n", 0, 0, 0);
	*status = TRANS_ACCEPT_BAD_MALLOC;
	return NULL;
    }

    if ((newciptr->fd = accept (ciptr->fd,
	(struct sockaddr *) &sockname, (void *)&namelen)) < 0)
    {
#ifdef WIN32
	errno = WSAGetLastError();
#endif
	PRMSG (1, "SocketINETAccept: accept() failed\n", 0, 0, 0);
	free (newciptr);
	*status = TRANS_ACCEPT_FAILED;
	return NULL;
    }

#ifdef TCP_NODELAY
    {
	/*
	 * turn off TCP coalescence for INET sockets
	 */

	int tmp = 1;
	setsockopt (newciptr->fd, IPPROTO_TCP, TCP_NODELAY,
	    (char *) &tmp, sizeof (int));
    }
#endif

    /*
     * Get this address again because the transport may give a more 
     * specific address now that a connection is established.
     */

    if (TRANS(SocketINETGetAddr) (newciptr) < 0)
    {
	PRMSG (1,
	    "SocketINETAccept: ...SocketINETGetAddr() failed:\n",
	    0, 0, 0);
	close (newciptr->fd);
	free (newciptr);
	*status = TRANS_ACCEPT_MISC_ERROR;
        return NULL;
    }

    if (TRANS(SocketINETGetPeerAddr) (newciptr) < 0)
    {
	PRMSG (1,
	  "SocketINETAccept: ...SocketINETGetPeerAddr() failed:\n",
		0, 0, 0);
	close (newciptr->fd);
	if (newciptr->addr) free (newciptr->addr);
	free (newciptr);
	*status = TRANS_ACCEPT_MISC_ERROR;
        return NULL;
    }

    *status = 0;

    return newciptr;
}

#endif /* TCPCONN */


#ifdef UNIXCONN
static XtransConnInfo
TRANS(SocketUNIXAccept) (XtransConnInfo ciptr, int *status)

{
    XtransConnInfo	newciptr;
    struct sockaddr_un	sockname;
#if defined(SVR4) || defined(__SCO__)
    size_t namelen = sizeof sockname;
#else
    int namelen = sizeof sockname;
#endif

    PRMSG (2, "SocketUNIXAccept(%p,%d)\n", ciptr, ciptr->fd, 0);

    if ((newciptr = (XtransConnInfo) calloc (
	1, sizeof(struct _XtransConnInfo))) == NULL)
    {
	PRMSG (1, "SocketUNIXAccept: malloc() failed\n", 0, 0, 0);
	*status = TRANS_ACCEPT_BAD_MALLOC;
	return NULL;
    }

    if ((newciptr->fd = accept (ciptr->fd,
	(struct sockaddr *) &sockname, (void *)&namelen)) < 0)
    {
	PRMSG (1, "SocketUNIXAccept: accept() failed\n", 0, 0, 0);
	free (newciptr);
	*status = TRANS_ACCEPT_FAILED;
	return NULL;
    }

    /*
     * Get the socket name and the peer name from the listener socket,
     * since this is unix domain.
     */

    if ((newciptr->addr = (char *) malloc (ciptr->addrlen)) == NULL)
    {
        PRMSG (1,
        "SocketUNIXAccept: Can't allocate space for the addr\n",
	      0, 0, 0);
	close (newciptr->fd);
	free (newciptr);
	*status = TRANS_ACCEPT_BAD_MALLOC;
        return NULL;
    }


    /*
     * if the socket is abstract, we already modified the address to have a
     * @ instead of the initial NUL, so no need to do that again here.
     */

    newciptr->addrlen = ciptr->addrlen;
    memcpy (newciptr->addr, ciptr->addr, newciptr->addrlen);

    if ((newciptr->peeraddr = (char *) malloc (ciptr->addrlen)) == NULL)
    {
        PRMSG (1,
	      "SocketUNIXAccept: Can't allocate space for the addr\n",
	      0, 0, 0);
	close (newciptr->fd);
	if (newciptr->addr) free (newciptr->addr);
	free (newciptr);
	*status = TRANS_ACCEPT_BAD_MALLOC;
        return NULL;
    }
    
    newciptr->peeraddrlen = ciptr->addrlen;
    memcpy (newciptr->peeraddr, ciptr->addr, newciptr->addrlen);

    newciptr->family = AF_UNIX;

    *status = 0;

    return newciptr;
}

#endif /* UNIXCONN */

#endif /* TRANS_SERVER */


#ifdef TRANS_CLIENT

#ifdef TCPCONN

#if defined(IPv6) && defined(AF_INET6)
struct addrlist {
    struct addrinfo *	addr;
    struct addrinfo *	firstaddr; 
    char 		port[PORTBUFSIZE];
    char 		host[MAXHOSTNAMELEN];
};
static struct addrlist  *addrlist = NULL;
#endif


static int
TRANS(SocketINETConnect) (XtransConnInfo ciptr, char *host, char *port)

{
    struct sockaddr *	socketaddr = NULL;
    int			socketaddrlen = 0;
    int			res;
#if defined(IPv6) && defined(AF_INET6)
    struct addrinfo 	hints;
    char		ntopbuf[INET6_ADDRSTRLEN];
    int			resetonce = 0;
#endif
    struct sockaddr_in	sockname;
#ifdef XTHREADS_NEEDS_BYNAMEPARAMS
    _Xgethostbynameparams hparams;
    _Xgetservbynameparams sparams;
#endif
    struct hostent	*hostp;
    struct servent	*servp;
    unsigned long 	tmpaddr;
#ifdef X11_t
    char	portbuf[PORTBUFSIZE];
#endif

    long		tmpport;
    char 		hostnamebuf[256];		/* tmp space */

    PRMSG (2,"SocketINETConnect(%d,%s,%s)\n", ciptr->fd, host, port);

    if (!host)
    {
	hostnamebuf[0] = '\0';
	(void) TRANS(GetHostname) (hostnamebuf, sizeof hostnamebuf);
	host = hostnamebuf;
    }

#ifdef X11_t
    /*
     * X has a well known port, that is transport dependent. It is easier
     * to handle it here, than try and come up with a transport independent
     * representation that can be passed in and resolved the usual way.
     *
     * The port that is passed here is really a string containing the idisplay
     * from ConnectDisplay().
     */

    if (is_numeric (port))
    {
	tmpport = X_TCP_PORT + strtol (port, (char**)NULL, 10);
	sprintf (portbuf, "%lu", tmpport);
	port = portbuf;
    }
#endif

#if defined(IPv6) && defined(AF_INET6)
    if (haveIPv6) {
	if (addrlist != NULL) {
	    if (strcmp(host,addrlist->host) || strcmp(port,addrlist->port)) {
		if (addrlist->firstaddr)
		    freeaddrinfo(addrlist->firstaddr);
		addrlist->firstaddr = NULL;
	    }
	} else {
	    addrlist = malloc(sizeof(struct addrlist));
	    addrlist->firstaddr = NULL;
	}

	if (addrlist->firstaddr == NULL) {
	    strncpy(addrlist->port, port, sizeof(addrlist->port));
	    addrlist->port[sizeof(addrlist->port) - 1] = '\0';
	    strncpy(addrlist->host, host, sizeof(addrlist->host));
	    addrlist->host[sizeof(addrlist->host) - 1] = '\0';

	    bzero(&hints,sizeof(hints));
	    hints.ai_socktype = Sockettrans2devtab[ciptr->index].devcotsname;

	    res = getaddrinfo(host,port,&hints,&addrlist->firstaddr);
	    if (res != 0) {
		PRMSG (1, "SocketINETConnect() can't get address "
			"for %s:%s: %s\n", host, port, gai_strerror(res));
		ESET(EINVAL);
		return TRANS_CONNECT_FAILED;
	    }
	    for (res = 0, addrlist->addr = addrlist->firstaddr;
		 addrlist->addr ; res++) {
		addrlist->addr = addrlist->addr->ai_next;
	    }
	    PRMSG(4,"Got New Address list with %d addresses\n", res, 0, 0);
	    res = 0;
	    addrlist->addr = NULL;
	}

	while (socketaddr == NULL) {
	    if (addrlist->addr == NULL) {
		if (resetonce) { 
		    /* Already checked entire list - no usable addresses */
		    PRMSG (1, "SocketINETConnect() no usable address "
			   "for %s:%s\n", host, port, 0);
		    return TRANS_CONNECT_FAILED;
		} else {
		    /* Go back to beginning of list */
		    resetonce = 1;
		    addrlist->addr = addrlist->firstaddr;
		}
	    } 

	    socketaddr = addrlist->addr->ai_addr;
	    socketaddrlen = addrlist->addr->ai_addrlen;

	    if (addrlist->addr->ai_family == AF_INET) {
		struct sockaddr_in *sin = (struct sockaddr_in *) socketaddr;

		PRMSG (4,"SocketINETConnect() sockname.sin_addr = %s\n",
			inet_ntop(addrlist->addr->ai_family,&sin->sin_addr,
			ntopbuf,sizeof(ntopbuf)), 0, 0); 

		PRMSG (4,"SocketINETConnect() sockname.sin_port = %d\n",
			ntohs(sin->sin_port), 0, 0); 

		if (Sockettrans2devtab[ciptr->index].family == AF_INET6) {
		    if (strcmp(Sockettrans2devtab[ciptr->index].transname,
				"tcp") == 0) {
			XtransConnInfo newciptr;

			/*
			 * Our socket is an IPv6 socket, but the address is
			 * IPv4.  Close it and get an IPv4 socket.  This is
			 * needed for IPv4 connections to work on platforms
			 * that don't allow IPv4 over IPv6 sockets.
			 */
			TRANS(SocketINETClose)(ciptr);
			newciptr = TRANS(SocketOpenCOTSClientBase)(
					"tcp", "tcp", host, port, ciptr->index);
			if (newciptr)
			    ciptr->fd = newciptr->fd;
			if (!newciptr ||
			    Sockettrans2devtab[newciptr->index].family !=
				AF_INET) {
			    socketaddr = NULL;
			    PRMSG (4,"SocketINETConnect() Cannot get IPv4 "
					" socketfor IPv4 address\n", 0,0,0);
			}
			if (newciptr)
			    free(newciptr);
		    } else {
			socketaddr = NULL;
			PRMSG (4,"SocketINETConnect Skipping IPv4 address\n",
				0,0,0);
		    }
		}
	    } else if (addrlist->addr->ai_family == AF_INET6) {
		struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *) socketaddr;
	
		PRMSG (4,"SocketINETConnect() sockname.sin6_addr = %s\n",
			inet_ntop(addrlist->addr->ai_family,
				  &sin6->sin6_addr,ntopbuf,sizeof(ntopbuf)),
			0, 0); 
		PRMSG (4,"SocketINETConnect() sockname.sin6_port = %d\n",
			ntohs(sin6->sin6_port), 0, 0); 

		if (Sockettrans2devtab[ciptr->index].family == AF_INET) {
		    if (strcmp(Sockettrans2devtab[ciptr->index].transname,
				"tcp") == 0) {
			XtransConnInfo newciptr;

			/*
			 * Close the IPv4 socket and try to open an IPv6 socket.
			 */
			TRANS(SocketINETClose)(ciptr);
			newciptr = TRANS(SocketOpenCOTSClientBase)(
					"tcp", "tcp", host, port, -1);
			if (newciptr)
			    ciptr->fd = newciptr->fd;
			if (!newciptr ||
			    Sockettrans2devtab[newciptr->index].family !=
					AF_INET6) {
			    socketaddr = NULL;
			    PRMSG (4,"SocketINETConnect() Cannot get IPv6 "
				   "socket for IPv6 address\n", 0,0,0);
			}
			if (newciptr)
			    free(newciptr);
		    }
		    else
		    {
			socketaddr = NULL;
			PRMSG (4,"SocketINETConnect() Skipping IPv6 address\n",
				0,0,0);
		    }
		}
	    } else {
		socketaddr = NULL; /* Unsupported address type */
	    }
	    if (socketaddr == NULL) {
		addrlist->addr = addrlist->addr->ai_next;
	    }
	} 
    } else
#endif
    {
	/*
	 * Build the socket name.
	 */

#ifdef BSD44SOCKETS
	sockname.sin_len = sizeof (struct sockaddr_in);
#endif
	sockname.sin_family = AF_INET;

	/*
	 * fill in sin_addr
	 */

#ifndef INADDR_NONE
#define INADDR_NONE ((in_addr_t) 0xffffffff)
#endif

	/* check for ww.xx.yy.zz host string */

	if (isascii (host[0]) && isdigit (host[0])) {
	    tmpaddr = inet_addr (host); /* returns network byte order */
	} else {
	    tmpaddr = INADDR_NONE;
	}

	PRMSG (4,"SocketINETConnect() inet_addr(%s) = %x\n", host, tmpaddr, 0);

	if (tmpaddr == INADDR_NONE) {
	    if ((hostp = _XGethostbyname(host,hparams)) == NULL) {
		PRMSG (1,"SocketINETConnect: Can't get address for %s\n",
			host, 0, 0);
		ESET(EINVAL);
		return TRANS_CONNECT_FAILED;
	    }
	    if (hostp->h_addrtype != AF_INET) {  /* is IP host? */
		PRMSG (1,"SocketINETConnect: not INET host%s\n", host, 0, 0);
		ESET(EPROTOTYPE);
		return TRANS_CONNECT_FAILED;
	    }
	    memcpy ((char *) &sockname.sin_addr, (char *) hostp->h_addr,
		    sizeof (sockname.sin_addr));
	} else {
	    sockname.sin_addr.s_addr = tmpaddr;
        }

	/*
	 * fill in sin_port
	 */

	/* Check for number in the port string */

	if (!is_numeric (port)) {
	    if ((servp = _XGetservbyname (port,"tcp",sparams)) == NULL) {
		PRMSG (1,"SocketINETConnect: can't get service for %s\n",
			port, 0, 0);
		return TRANS_CONNECT_FAILED;
	    }
	    sockname.sin_port = htons (servp->s_port);
	} else {
	    tmpport = strtol (port, (char**)NULL, 10);
	    if (tmpport < 1024 || tmpport > USHRT_MAX)
		return TRANS_CONNECT_FAILED;
	    sockname.sin_port = htons (((unsigned short) tmpport));
	}

	PRMSG (4,"SocketINETConnect: sockname.sin_port = %d\n",
		ntohs(sockname.sin_port), 0, 0);
	socketaddr = (struct sockaddr *) &sockname;
	socketaddrlen = sizeof(sockname);
    }

    /*
     * Turn on socket keepalive so the client process will eventually
     * be notified with a SIGPIPE signal if the display server fails
     * to respond to a periodic transmission of messages
     * on the connected socket.
     * This is useful to avoid hung application processes when the
     * processes are not spawned from the xdm session and
     * the display server terminates abnormally.
     * (Someone turned off the power switch.)
     */

    {
	int tmp = 1;
	setsockopt (ciptr->fd, SOL_SOCKET, SO_KEEPALIVE,
		(char *) &tmp, sizeof (int));
    }

    /*
     * Do the connect()
     */

    if (connect (ciptr->fd, socketaddr, socketaddrlen ) < 0)
    {
#ifdef WIN32
	int olderrno = WSAGetLastError();
#else
	int olderrno = errno;
#endif

	/*
	 * If the error was ECONNREFUSED, the server may be overloaded
	 * and we should try again.
	 *
	 * If the error was EWOULDBLOCK or EINPROGRESS then the socket
	 * was non-blocking and we should poll using select
	 *
	 * If the error was EINTR, the connect was interrupted and we
	 * should try again.
	 *
	 * If multiple addresses are found for a host then we should
	 * try to connect again with a different address for a larger
	 * number of errors that made us quit before, since those
	 * could be caused by trying to use an IPv6 address to contact
	 * a machine with an IPv4-only server or other reasons that
	 * only affect one of a set of addresses.  
	 */

	if (olderrno == ECONNREFUSED || olderrno == EINTR
#if defined(IPv6) && defined(AF_INET6)
	  || (haveIPv6 && ((addrlist->addr->ai_next != NULL) || 
	        (addrlist->addr != addrlist->firstaddr)) &&
               (olderrno == ENETUNREACH || olderrno == EAFNOSUPPORT ||
		 olderrno == EADDRNOTAVAIL || olderrno == ETIMEDOUT
#if defined(EHOSTDOWN)
		   || olderrno == EHOSTDOWN
#endif
	       ))
#endif
	    )
	    res = TRANS_TRY_CONNECT_AGAIN;
	else if (olderrno == EWOULDBLOCK || olderrno == EINPROGRESS)
	    res = TRANS_IN_PROGRESS;
	else
	{
	    PRMSG (2,"SocketINETConnect: Can't connect: errno = %d\n",
		   olderrno,0, 0);

	    res = TRANS_CONNECT_FAILED;	
	}
    } else {
	res = 0;
    

	/*
	 * Sync up the address fields of ciptr.
	 */
    
	if (TRANS(SocketINETGetAddr) (ciptr) < 0)
	{
	    PRMSG (1,
	     "SocketINETConnect: ...SocketINETGetAddr() failed:\n",
	      0, 0, 0);
	    res = TRANS_CONNECT_FAILED;
	}

	else if (TRANS(SocketINETGetPeerAddr) (ciptr) < 0)
	{
	    PRMSG (1,
	      "SocketINETConnect: ...SocketINETGetPeerAddr() failed:\n",
	      0, 0, 0);
	    res = TRANS_CONNECT_FAILED;
	}
    }

#if defined(IPv6) && defined(AF_INET6)
   if (haveIPv6 && res != 0) { 
	addrlist->addr = addrlist->addr->ai_next;
   }
#endif

    return res;
}

#endif /* TCPCONN */



#ifdef UNIXCONN

/*
 * Make sure 'host' is really local.
 */

static int
UnixHostReallyLocal (char *host)

{
    char hostnamebuf[256];

#if defined(IPv6) && defined(AF_INET6)
    if (getaddrinfo == NULL)
	haveIPv6 = 0;
#endif

    TRANS(GetHostname) (hostnamebuf, sizeof (hostnamebuf));

    if (strcmp (hostnamebuf, host) == 0)
    {
	return (1);
    }
#if defined(IPv6) && defined(AF_INET6)
    else if (haveIPv6)
    {
	struct addrinfo *localhostaddr;
	struct addrinfo *otherhostaddr;
	struct addrinfo *i, *j;
	int equiv = 0;

	if (getaddrinfo(hostnamebuf, NULL, NULL, &localhostaddr) != 0)
	    return 0;
	if (getaddrinfo(host, NULL, NULL, &otherhostaddr) != 0) {
	    freeaddrinfo(localhostaddr);
	    return 0;
	}

	for (i = localhostaddr; i != NULL && equiv == 0; i = i->ai_next) {
	    for (j = otherhostaddr; j != NULL && equiv == 0; j = j->ai_next) {
		if (i->ai_family == j->ai_family) {
		    if (i->ai_family == AF_INET) {
			struct sockaddr_in *sinA 
			  = (struct sockaddr_in *) i->ai_addr;
			struct sockaddr_in *sinB
			  = (struct sockaddr_in *) j->ai_addr;
			struct in_addr *A = &sinA->sin_addr;
			struct in_addr *B = &sinB->sin_addr;

			if (memcmp(A,B,sizeof(struct in_addr)) == 0) {
			    equiv = 1;
			}
		    } else if (i->ai_family == AF_INET6) {
			struct sockaddr_in6 *sinA 
			  = (struct sockaddr_in6 *) i->ai_addr;
			struct sockaddr_in6 *sinB 
			  = (struct sockaddr_in6 *) j->ai_addr;
			struct in6_addr *A = &sinA->sin6_addr;
			struct in6_addr *B = &sinB->sin6_addr;

			if (memcmp(A,B,sizeof(struct in6_addr)) == 0) {
			    equiv = 1;
			}
		    }
		}
	    }
	}
	
	freeaddrinfo(localhostaddr);
	freeaddrinfo(otherhostaddr);
	return equiv;
    }
#endif
    else
    {
	/*
	 * A host may have more than one network address.  If any of the
	 * network addresses of 'host' (specified to the connect call)
	 * match any of the network addresses of 'hostname' (determined
	 * by TRANS(GetHostname)), then the two hostnames are equivalent,
	 * and we know that 'host' is really a local host.
	 */
	char specified_local_addr_list[10][4];
	int scount, equiv, i, j;
#ifdef XTHREADS_NEEDS_BYNAMEPARAMS
	_Xgethostbynameparams hparams;
#endif
	struct hostent *hostp;

	if ((hostp = _XGethostbyname (host,hparams)) == NULL)
	    return (0);

	scount = 0;
	while (hostp->h_addr_list[scount] && scount <= 8)
	{
	    /*
	     * The 2nd call to gethostname() overrides the data
	     * from the 1st call, so we must save the address list.
	     */

	    specified_local_addr_list[scount][0] = 
				hostp->h_addr_list[scount][0];
	    specified_local_addr_list[scount][1] = 
				hostp->h_addr_list[scount][1];
	    specified_local_addr_list[scount][2] = 
				hostp->h_addr_list[scount][2];
	    specified_local_addr_list[scount][3] = 
				hostp->h_addr_list[scount][3];
	    scount++;
	}
	if ((hostp = _XGethostbyname (hostnamebuf,hparams)) == NULL)
	    return (0);

	equiv = 0;
	i = 0;

	while (i < scount && !equiv)
	{
	    j = 0;

	    while (hostp->h_addr_list[j])
	    {
		if ((specified_local_addr_list[i][0] == 
					hostp->h_addr_list[j][0]) &&
		    (specified_local_addr_list[i][1] == 
					hostp->h_addr_list[j][1]) &&
		    (specified_local_addr_list[i][2] == 
					hostp->h_addr_list[j][2]) &&
		    (specified_local_addr_list[i][3] == 
					hostp->h_addr_list[j][3]))
		{
		    /* They're equal, so we're done */
		    
		    equiv = 1;
		    break;
		}

		j++;
	    }

	    i++;
	}
	return (equiv);
    }
}

static int
TRANS(SocketUNIXConnect) (XtransConnInfo ciptr, char *host, char *port)

{
    struct sockaddr_un	sockname;
    int			namelen;


    int abstract = 0;
#ifdef HAVE_ABSTRACT_SOCKETS
    abstract = ciptr->transptr->flags & TRANS_ABSTRACT;
#endif

#if defined(hpux) && defined(X11_t)
    struct sockaddr_un	old_sockname;
    int			old_namelen;
#endif


    PRMSG (2,"SocketUNIXConnect(%d,%s,%s)\n", ciptr->fd, host, port);
    
    /*
     * Make sure 'host' is really local.  If not, we return failure.
     * The reason we make this check is because a process may advertise
     * a "local" network ID for which it can accept connections, but if
     * a process on a remote machine tries to connect to this network ID,
     * we know for sure it will fail.
     */

#if defined(NX_TRANS_SOCKET) && defined(TRANS_CLIENT)
    if (strcmp(host, "unix") != 0 && strcasecmp(host, "nx") != 0 &&
            strncasecmp(host, "nx,", 3) != 0 && !UnixHostReallyLocal (host))
#else
    if (strcmp (host, "unix") != 0 && !UnixHostReallyLocal (host))
#endif
    {
	PRMSG (1,
	   "SocketUNIXConnect: Cannot connect to non-local host %s\n",
	       host, 0, 0);
	return TRANS_CONNECT_FAILED;
    }


    /*
     * Check the port.
     */

    if (!port || !*port)
    {
	PRMSG (1,"SocketUNIXConnect: Missing port specification\n",
	      0, 0, 0);
	return TRANS_CONNECT_FAILED;
    }

    /*
     * Build the socket name.
     */
    
    sockname.sun_family = AF_UNIX;

#ifdef NX_TRANS_SOCKET
    if (set_sun_path(port, _NXGetUnixPath(UNIX_PATH), sockname.sun_path, abstract) != 0) {
#else
    if (set_sun_path(port, UNIX_PATH, sockname.sun_path, abstract) != 0) {
#endif
	PRMSG (1, "SocketUNIXConnect: path too long\n", 0, 0, 0);
	return TRANS_CONNECT_FAILED;
    }

#if (defined(BSD44SOCKETS) || defined(__UNIXWARE__)) && !defined(Lynx)
    sockname.sun_len = strlen (sockname.sun_path);
    namelen = SUN_LEN (&sockname);
#else
    namelen = strlen (sockname.sun_path) + sizeof (sockname.sun_family);
#endif


#if defined(hpux) && defined(X11_t)
    /*
     * This is gross, but it was in Xlib
     */
    old_sockname.sun_family = AF_UNIX;
#ifdef NX_TRANS_SOCKET
    if (set_sun_path(port, _NXGetOldUnixPath(OLD_UNIX_PATH), old_sockname.sun_path) != 0) {
#else
    if (set_sun_path(port, OLD_UNIX_PATH, old_sockname.sun_path) != 0) {
#endif
	PRMSG (1, "SocketUNIXConnect: path too long\n", 0, 0, 0);
	return TRANS_CONNECT_FAILED;
    }
    old_namelen = strlen (old_sockname.sun_path) +
	sizeof (old_sockname.sun_family);
#endif

#if defined(NX_TRANS_SOCKET) && defined(TRANS_CLIENT)

    if (ciptr->priv != NULL)
    {
        if (TRANS(SocketConnectConnInfo) (ciptr, host, port) != 0)
        {
            return TRANS_CONNECT_FAILED;
        }

        goto SocketUNIXConnectPost;
    }

#endif

    /*
     * Adjust the socket path if using abstract sockets.
     * Done here because otherwise all the strlen() calls above would fail.
     */

    if (abstract)
	sockname.sun_path[0] = '\0';

    /*
     * Do the connect()
     */

    if (connect (ciptr->fd, (struct sockaddr *) &sockname, namelen) < 0)
    {
	int olderrno = errno;
	int connected = 0;
	
#if defined(hpux) && defined(X11_t)
	if (olderrno == ENOENT)
	{
	    if (connect (ciptr->fd,
		(struct sockaddr *) &old_sockname, old_namelen) >= 0)
	    {
		connected = 1;
	    }
	    else
		olderrno = errno;
	}
#endif
	if (!connected)
	{
	    errno = olderrno;
	    
	    /*
	     * If the error was ENOENT, the server may be starting up
	     * and we should try again.
	     *
	     * If the error was EWOULDBLOCK or EINPROGRESS then the socket
	     * was non-blocking and we should poll using select
	     *
	     * If the error was EINTR, the connect was interrupted and we
	     * should try again.
	     */

	    if (olderrno == EWOULDBLOCK || olderrno == EINPROGRESS)
		return TRANS_IN_PROGRESS;
	    else if (olderrno == EINTR)
		return TRANS_TRY_CONNECT_AGAIN;
	    else if (olderrno == ENOENT || olderrno == ECONNREFUSED)
		/* If opening as abstract failed, try again normally */
		if (abstract) {
		    ciptr->transptr->flags &= ~(TRANS_ABSTRACT);
		    return TRANS_TRY_CONNECT_AGAIN;
		} else {
		    return TRANS_CONNECT_FAILED;
	    } else {
		PRMSG (2,"SocketUNIXConnect: Can't connect: errno = %d\n",
		       EGET(),0, 0);

		return TRANS_CONNECT_FAILED;
	    }
	}
    }

#if defined(NX_TRANS_SOCKET) && defined(TRANS_CLIENT)

SocketUNIXConnectPost:

#endif

    /*
     * Get the socket name and the peer name from the connect socket,
     * since this is unix domain.
     */

    if ((ciptr->addr = (char *) malloc(namelen)) == NULL ||
       (ciptr->peeraddr = (char *) malloc(namelen)) == NULL)
    {
        PRMSG (1,
	"SocketUNIXCreateListener: Can't allocate space for the addr\n",
	      0, 0, 0);
        return TRANS_CONNECT_FAILED;
    }

    if (abstract)
	sockname.sun_path[0] = '@';

    ciptr->family = AF_UNIX;
    ciptr->addrlen = namelen;
    ciptr->peeraddrlen = namelen;
    memcpy (ciptr->addr, &sockname, ciptr->addrlen);
    memcpy (ciptr->peeraddr, &sockname, ciptr->peeraddrlen);
    
    return 0;
}

#endif /* UNIXCONN */

#endif /* TRANS_CLIENT */


static int
TRANS(SocketBytesReadable) (XtransConnInfo ciptr, BytesReadable_t *pend)

{
    PRMSG (2,"SocketBytesReadable(%p,%d,%p)\n",
	ciptr, ciptr->fd, pend);

#if defined(NX_TRANS_SOCKET) && defined(TRANS_CLIENT)

    if (ciptr->priv)
    {
        if (NXTransRunning(ciptr->fd) == 0)
        {
            /*
             * Force the application to shut down the
             * socket if the NX transport is gone. We
             * may probably save this additional call.
             */

#ifdef NX_TRANS_DEBUG
            fprintf(stderr, "SocketBytesReadable: NX transport not running for descriptor [%d].\n",
                        ciptr->fd);
#endif
            ESET(EPIPE);

            return -1;
        }
        else
        {
            /*
             * Emulate BytesReadable. Some X applications may use the system
             * select() in their main loop, instead of the _XSelect() that is
             * replaced by NX. Still these applications use _XEventsQueued to
             * poll events from the X connection, and _XEventsQueued uses the
             * NX _XSelect(), so it is generally possible to let the client
             * yield the control to NX and let it handle the I/O on the proxy
             * descriptors even if the application is not explicitly designed
             * to work as a NX agent.         
             */

#ifdef NX_TRANS_DEBUG

            int result;

            result = NXTransReadable(ciptr->fd, (int *) pend);

            fprintf(stderr, "SocketBytesReadable: Descriptor [%d] result [%d] readable [%ld].\n",
                        ciptr->fd, result, *pend);

            return result;
#else
            return NXTransReadable(ciptr->fd, (int *) pend);
#endif
        }
    }

#endif /* #if defined(NX_TRANS_SOCKET) && defined(TRANS_CLIENT) */

#if defined(QNX4)
    *pend = 0L; /* FIONREAD only returns a short. Zero out upper bits */
#endif
#ifdef WIN32
    {
	int ret = ioctlsocket ((SOCKET) ciptr->fd, FIONREAD, (u_long *) pend);
	errno = WSAGetLastError();
	return ret;
    }
#else
#if (defined(i386) && defined(SYSV) && !defined(SCO325)) || (defined(_SEQUENT_) && _SOCKET_VERSION == 1)
    return ioctl (ciptr->fd, I_NREAD, (char *) pend);
#else
#if defined(__UNIXOS2__)
    return ioctl (ciptr->fd, FIONREAD, (char*) pend, sizeof(int));
#else
    return ioctl (ciptr->fd, FIONREAD, (char *) pend);
#endif /* __UNIXOS2__ */
#endif /* i386 && SYSV || _SEQUENT_ && _SOCKET_VERSION == 1 */
#endif /* WIN32 */
}


static int
TRANS(SocketRead) (XtransConnInfo ciptr, char *buf, int size)

{
    PRMSG (2,"SocketRead(%d,%p,%d)\n", ciptr->fd, buf, size);

#if defined(NX_TRANS_SOCKET) && defined(TRANS_CLIENT)

    /*
     * If we have a valid priv pointer then this
     * is an internal connection to the proxy.
     * In this case we should emulate the read.
     */

    if (ciptr->priv)
    {
        int result;

        result = NXTransRead(ciptr->fd, buf, size);

#ifdef NX_TRANS_DEBUG
        if (result < 0 && EGET() == EAGAIN)
        {
            fprintf(stderr, "SocketRead: Read from descriptor [%d] would block.\n",
                     ciptr->fd);
        }
        else
        {
            fprintf(stderr, "SocketRead: Read [%d] bytes from descriptor [%d].\n",
                     result, ciptr->fd);
        }
#endif
        return result;
    }
    else
    {
        return read (ciptr->fd, buf, size);
    }

#else /* #if defined(NX_TRANS_SOCKET) && defined(TRANS_CLIENT) */

#if defined(WIN32) || defined(__UNIXOS2__)
    {
	int ret = recv ((SOCKET)ciptr->fd, buf, size, 0);
#ifdef WIN32
	errno = WSAGetLastError();
#endif
	return ret;
    }
#else
    return read (ciptr->fd, buf, size);
#endif /* WIN32 */

#endif /* #if defined(NX_TRANS_SOCKET) && defined(TRANS_CLIENT) */
}


static int
TRANS(SocketWrite) (XtransConnInfo ciptr, char *buf, int size)

{
    PRMSG (2,"SocketWrite(%d,%p,%d)\n", ciptr->fd, buf, size);

#if defined(NX_TRANS_SOCKET) && defined(TRANS_CLIENT)

    /*
     * If we have a valid priv pointer then this
     * is an internal connection to the proxy.
     * In this case we should emulate the write.
     */

    if (ciptr->priv)
    {
        int result;

        result = NXTransWrite(ciptr->fd, buf, size);

#ifdef NX_TRANS_DEBUG
        if (result < 0 && EGET() == EAGAIN)
        {
            fprintf(stderr, "SocketWrite: Write on descriptor [%d] would block.\n",
                     ciptr->fd);
        }
        else
        {
            fprintf(stderr, "SocketWrite: Written [%d] bytes on descriptor [%d].\n",
                     result, ciptr->fd);
        }
#endif
        return result;
    }
    else
    {
        return write (ciptr->fd, buf, size);
    }

#else /* #if defined(NX_TRANS_SOCKET) && defined(TRANS_CLIENT) */

#if defined(WIN32) || defined(__UNIXOS2__)
    {
	int ret = send ((SOCKET)ciptr->fd, buf, size, 0);
#ifdef WIN32
	errno = WSAGetLastError();
#endif
	return ret;
    }
#else
    return write (ciptr->fd, buf, size);
#endif /* WIN32 */

#endif /* #if defined(NX_TRANS_SOCKET) && defined(TRANS_CLIENT) */
}


static int
TRANS(SocketReadv) (XtransConnInfo ciptr, struct iovec *buf, int size)

{
    PRMSG (2,"SocketReadv(%d,%p,%d)\n", ciptr->fd, buf, size);

#if defined(NX_TRANS_SOCKET) && defined(TRANS_CLIENT)

    /*
     * If we have a valid priv pointer then this
     * is an internal connection to the proxy.
     * In this case we should emulate the readv.
     */

    if (ciptr->priv)
    {
        return NXTransReadVector(ciptr->fd, buf, size);
    }
    else
    {
        return READV (ciptr, buf, size);
    }

#else /* #if defined(NX_TRANS_SOCKET) && defined(TRANS_CLIENT) */

    return READV (ciptr, buf, size);

#endif /* #if defined(NX_TRANS_SOCKET) && defined(TRANS_CLIENT) */
}


static int
TRANS(SocketWritev) (XtransConnInfo ciptr, struct iovec *buf, int size)

{
    PRMSG (2,"SocketWritev(%d,%p,%d)\n", ciptr->fd, buf, size);

#if defined(NX_TRANS_SOCKET) && defined(TRANS_CLIENT)

    /*
     * If we have a valid priv pointer then this
     * is an internal connection to the proxy.
     * In this case we should emulate the writev.
     */

    if (ciptr->priv)
    {
        return NXTransWriteVector(ciptr->fd, buf, size);
    }
    else
    {
        return WRITEV (ciptr, buf, size);
    }

#else /* #if defined(NX_TRANS_SOCKET) && defined(TRANS_CLIENT) */

    return WRITEV (ciptr, buf, size);

#endif /* #if defined(NX_TRANS_SOCKET) && defined(TRANS_CLIENT) */
}


static int
TRANS(SocketDisconnect) (XtransConnInfo ciptr)

{
    PRMSG (2,"SocketDisconnect(%p,%d)\n", ciptr, ciptr->fd, 0);

#ifdef WIN32
    { 
	int ret = shutdown (ciptr->fd, 2);
	errno = WSAGetLastError();
	return ret;
    }
#else
    return shutdown (ciptr->fd, 2); /* disallow further sends and receives */
#endif
}


#ifdef TCPCONN
static int
TRANS(SocketINETClose) (XtransConnInfo ciptr)

{
    PRMSG (2,"SocketINETClose(%p,%d)\n", ciptr, ciptr->fd, 0);

#ifdef WIN32
    {
	int ret = close (ciptr->fd);
	errno = WSAGetLastError();
	return ret;
    }
#else
    return close (ciptr->fd);
#endif
}

#endif /* TCPCONN */


#ifdef UNIXCONN
static int
TRANS(SocketUNIXClose) (XtransConnInfo ciptr)

{
    /*
     * If this is the server side, then once the socket is closed,
     * it must be unlinked to completely close it
     */

    struct sockaddr_un	*sockname = (struct sockaddr_un *) ciptr->addr;
    int ret;

    PRMSG (2,"SocketUNIXClose(%x,%d)\n", ciptr, ciptr->fd, 0);

#if defined(NX_TRANS_SOCKET) && defined(TRANS_CLIENT)

    if (ciptr->priv)
    {
        TRANS(SocketCloseConnInfo) (ciptr);
    }

#endif

    ret = close(ciptr->fd);

#if defined(NX_TRANS_SOCKET) && defined(NX_TRANS_TEST)
    fprintf(stderr, "SocketUNIXClose: Flags are [%d] for ciptr at [%p] check is [%d].\n",
                ciptr->flags, (void *) ciptr, (ciptr->flags && sockname
                    && sockname->sun_family == AF_UNIX && sockname->sun_path[0]));
#endif

    if (ciptr->flags
       && sockname
       && sockname->sun_family == AF_UNIX
       && sockname->sun_path[0])
    {
#if defined(NX_TRANS_SOCKET) && defined(NX_TRANS_TEST)
        if (!(ciptr->flags & TRANS_NOUNLINK))
        {
                fprintf(stderr, "SocketUNIXClose: Unlinking path [%s] for ciptr at [%p].\n",
                            sockname->sun_path, (void *) ciptr);
                unlink (sockname->sun_path);
        }
#else
	if (!(ciptr->flags & TRANS_NOUNLINK))
		unlink (sockname->sun_path);
#endif
    }

    return ret;
}

static int
TRANS(SocketUNIXCloseForCloning) (XtransConnInfo ciptr)

{
    /*
     * Don't unlink path.
     */

    int ret;

    PRMSG (2,"SocketUNIXCloseForCloning(%p,%d)\n",
	ciptr, ciptr->fd, 0);

#if defined(NX_TRANS_SOCKET) && defined(TRANS_CLIENT)

    if (ciptr->priv)
    {
        TRANS(SocketCloseConnInfo) (ciptr);
    }

#endif

    ret = close(ciptr->fd);

    return ret;
}

#endif /* UNIXCONN */


#ifdef TCPCONN
# ifdef TRANS_SERVER
static char* tcp_nolisten[] = {
	"inet",
#if defined(IPv6) && defined(AF_INET6)
	"inet6",
#endif
	NULL
};
# endif

Xtransport	TRANS(SocketTCPFuncs) = {
	/* Socket Interface */
	"tcp",
        TRANS_ALIAS,
#ifdef TRANS_CLIENT
	TRANS(SocketOpenCOTSClient),
#endif /* TRANS_CLIENT */
#ifdef TRANS_SERVER
	tcp_nolisten,
	TRANS(SocketOpenCOTSServer),
#endif /* TRANS_SERVER */
#ifdef TRANS_CLIENT
	TRANS(SocketOpenCLTSClient),
#endif /* TRANS_CLIENT */
#ifdef TRANS_SERVER
	TRANS(SocketOpenCLTSServer),
#endif /* TRANS_SERVER */
#ifdef TRANS_REOPEN
	TRANS(SocketReopenCOTSServer),
	TRANS(SocketReopenCLTSServer),
#endif
	TRANS(SocketSetOption),
#ifdef TRANS_SERVER
	TRANS(SocketINETCreateListener),
	NULL,		       			/* ResetListener */
	TRANS(SocketINETAccept),
#endif /* TRANS_SERVER */
#ifdef TRANS_CLIENT
	TRANS(SocketINETConnect),
#endif /* TRANS_CLIENT */
	TRANS(SocketBytesReadable),
	TRANS(SocketRead),
	TRANS(SocketWrite),
	TRANS(SocketReadv),
	TRANS(SocketWritev),
	TRANS(SocketDisconnect),
	TRANS(SocketINETClose),
	TRANS(SocketINETClose),
	};

Xtransport	TRANS(SocketINETFuncs) = {
	/* Socket Interface */
	"inet",
	0,
#ifdef TRANS_CLIENT
	TRANS(SocketOpenCOTSClient),
#endif /* TRANS_CLIENT */
#ifdef TRANS_SERVER
	NULL,
	TRANS(SocketOpenCOTSServer),
#endif /* TRANS_SERVER */
#ifdef TRANS_CLIENT
	TRANS(SocketOpenCLTSClient),
#endif /* TRANS_CLIENT */
#ifdef TRANS_SERVER
	TRANS(SocketOpenCLTSServer),
#endif /* TRANS_SERVER */
#ifdef TRANS_REOPEN
	TRANS(SocketReopenCOTSServer),
	TRANS(SocketReopenCLTSServer),
#endif
	TRANS(SocketSetOption),
#ifdef TRANS_SERVER
	TRANS(SocketINETCreateListener),
	NULL,		       			/* ResetListener */
	TRANS(SocketINETAccept),
#endif /* TRANS_SERVER */
#ifdef TRANS_CLIENT
	TRANS(SocketINETConnect),
#endif /* TRANS_CLIENT */
	TRANS(SocketBytesReadable),
	TRANS(SocketRead),
	TRANS(SocketWrite),
	TRANS(SocketReadv),
	TRANS(SocketWritev),
	TRANS(SocketDisconnect),
	TRANS(SocketINETClose),
	TRANS(SocketINETClose),
	};

#if defined(IPv6) && defined(AF_INET6)
Xtransport     TRANS(SocketINET6Funcs) = {
	/* Socket Interface */
	"inet6",
	0,
#ifdef TRANS_CLIENT
	TRANS(SocketOpenCOTSClient),
#endif /* TRANS_CLIENT */
#ifdef TRANS_SERVER
	NULL,
	TRANS(SocketOpenCOTSServer),
#endif /* TRANS_SERVER */
#ifdef TRANS_CLIENT
	TRANS(SocketOpenCLTSClient),
#endif /* TRANS_CLIENT */
#ifdef TRANS_SERVER
	TRANS(SocketOpenCLTSServer),
#endif /* TRANS_SERVER */
#ifdef TRANS_REOPEN
	TRANS(SocketReopenCOTSServer),
	TRANS(SocketReopenCLTSServer),
#endif
	TRANS(SocketSetOption),
#ifdef TRANS_SERVER
	TRANS(SocketINETCreateListener),
	NULL,					/* ResetListener */
	TRANS(SocketINETAccept),
#endif /* TRANS_SERVER */
#ifdef TRANS_CLIENT
	TRANS(SocketINETConnect),
#endif /* TRANS_CLIENT */
	TRANS(SocketBytesReadable),
	TRANS(SocketRead),
	TRANS(SocketWrite),
	TRANS(SocketReadv),
	TRANS(SocketWritev),
	TRANS(SocketDisconnect),
	TRANS(SocketINETClose),
	TRANS(SocketINETClose),
	};
#endif /* IPv6 */
#endif /* TCPCONN */

#ifdef UNIXCONN
#if !defined(LOCALCONN)
Xtransport	TRANS(SocketLocalFuncs) = {
	/* Socket Interface */
	"local",
#ifdef HAVE_ABSTRACT_SOCKETS
	TRANS_ABSTRACT,
#else
	0,
#endif
#ifdef TRANS_CLIENT
	TRANS(SocketOpenCOTSClient),
#endif /* TRANS_CLIENT */
#ifdef TRANS_SERVER
	NULL,
	TRANS(SocketOpenCOTSServer),
#endif /* TRANS_SERVER */
#ifdef TRANS_CLIENT
	TRANS(SocketOpenCLTSClient),
#endif /* TRANS_CLIENT */
#ifdef TRANS_SERVER
	TRANS(SocketOpenCLTSServer),
#endif /* TRANS_SERVER */
#ifdef TRANS_REOPEN
	TRANS(SocketReopenCOTSServer),
	TRANS(SocketReopenCLTSServer),
#endif
	TRANS(SocketSetOption),
#ifdef TRANS_SERVER
	TRANS(SocketUNIXCreateListener),
	TRANS(SocketUNIXResetListener),
	TRANS(SocketUNIXAccept),
#endif /* TRANS_SERVER */
#ifdef TRANS_CLIENT
	TRANS(SocketUNIXConnect),
#endif /* TRANS_CLIENT */
	TRANS(SocketBytesReadable),
	TRANS(SocketRead),
	TRANS(SocketWrite),
	TRANS(SocketReadv),
	TRANS(SocketWritev),
	TRANS(SocketDisconnect),
	TRANS(SocketUNIXClose),
	TRANS(SocketUNIXCloseForCloning),
	};
#endif /* !LOCALCONN */
# ifdef TRANS_SERVER
#  if !defined(LOCALCONN)
static char* unix_nolisten[] = { "local" , NULL };
#  endif
# endif
	    
Xtransport	TRANS(SocketUNIXFuncs) = {
	/* Socket Interface */
	"unix",
#if !defined(LOCALCONN) && !defined(HAVE_ABSTRACT_SOCKETS)
        TRANS_ALIAS,
#else
	0,
#endif
#ifdef TRANS_CLIENT
	TRANS(SocketOpenCOTSClient),
#endif /* TRANS_CLIENT */
#ifdef TRANS_SERVER
#if !defined(LOCALCONN)
	unix_nolisten,
#else
	NULL,
#endif
	TRANS(SocketOpenCOTSServer),
#endif /* TRANS_SERVER */
#ifdef TRANS_CLIENT
	TRANS(SocketOpenCLTSClient),
#endif /* TRANS_CLIENT */
#ifdef TRANS_SERVER
	TRANS(SocketOpenCLTSServer),
#endif /* TRANS_SERVER */
#ifdef TRANS_REOPEN
	TRANS(SocketReopenCOTSServer),
	TRANS(SocketReopenCLTSServer),
#endif
	TRANS(SocketSetOption),
#ifdef TRANS_SERVER
	TRANS(SocketUNIXCreateListener),
	TRANS(SocketUNIXResetListener),
	TRANS(SocketUNIXAccept),
#endif /* TRANS_SERVER */
#ifdef TRANS_CLIENT
	TRANS(SocketUNIXConnect),
#endif /* TRANS_CLIENT */
	TRANS(SocketBytesReadable),
	TRANS(SocketRead),
	TRANS(SocketWrite),
	TRANS(SocketReadv),
	TRANS(SocketWritev),
	TRANS(SocketDisconnect),
	TRANS(SocketUNIXClose),
	TRANS(SocketUNIXCloseForCloning),
	};

#endif /* UNIXCONN */
