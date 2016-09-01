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
 
Copyright 1989, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.

*/

/* 
 * This file contains operating system dependencies.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <nx-X11/Xlibint.h>
#include <nx-X11/Xtrans/Xtrans.h>
#include <nx-X11/Xauth.h>
#include <X11/Xdmcp.h>
#include <stdio.h>
#include <ctype.h>

#if !defined(WIN32)
#ifndef Lynx
#include <sys/socket.h>
#else
#include <socket.h>
#endif
#else
#include <X11/Xwindows.h>
#endif

#ifndef X_CONNECTION_RETRIES		/* number retries on ECONNREFUSED */
#define X_CONNECTION_RETRIES 5
#endif

#ifdef LOCALCONN
#include <sys/utsname.h>
#endif

#include "Xintconn.h"

/* prototypes */
static void GetAuthorization(
    XtransConnInfo trans_conn,
    int family,
    char *saddr,
    int saddrlen,
    int idisplay,
    char **auth_namep,
    int *auth_namelenp,
    char **auth_datap,
    int *auth_datalenp);

/* functions */
static char *copystring (char *src, int len)
{
    char *dst = Xmalloc (len + 1);

    if (dst) {
	strncpy (dst, src, len);
	dst[len] = '\0';
    }

    return dst;
}


/* 
 * Attempts to connect to server, given display name. Returns file descriptor
 * (network socket) or -1 if connection fails.  Display names may be of the
 * following format:
 *
 *     [protocol/] [hostname] : [:] displaynumber [.screennumber]
 *
 * A string with exactly two colons seperating hostname from the display
 * indicates a DECnet style name.  Colons in the hostname may occur if an
 * IPv6 numeric address is used as the hostname.  An IPv6 numeric address
 * may also end in a double colon, so three colons in a row indicates an
 * IPv6 address ending in :: followed by :display.  To make it easier for
 * people to read, an IPv6 numeric address hostname may be surrounded by
 * [ ] in a similar fashion to the IPv6 numeric address URL syntax defined
 * by IETF RFC 2732.
 *
 * If no hostname and no protocol is specified, the string is interpreted
 * as the most efficient local connection to a server on the same machine.  
 * This is usually:
 *
 *     o  shared memory
 *     o  local stream
 *     o  UNIX domain socket
 *     o  TCP to local host
 *
 * This function will eventually call the X Transport Interface functions
 * which expects the hostname in the format:
 *
 *	[protocol/] [hostname] : [:] displaynumber
 *
 */
XtransConnInfo
_X11TransConnectDisplay (
    char *display_name,
    char **fullnamep,			/* RETURN */
    int *dpynump,			/* RETURN */
    int *screenp,			/* RETURN */
    char **auth_namep,			/* RETURN */
    int *auth_namelenp,			/* RETURN */
    char **auth_datap,			/* RETURN */
    int *auth_datalenp)			/* RETURN */
{
    int family;
    int saddrlen;
    Xtransaddr *saddr;
    char *lastp, *lastc, *p;		/* char pointers */
    char *pprotocol = NULL;		/* start of protocol name */
    char *phostname = NULL;		/* start of host of display */
    char *pdpynum = NULL;		/* start of dpynum of display */
    char *pscrnum = NULL;		/* start of screen of display */
    Bool dnet = False;			/* if true, then DECnet format */
    int idisplay = 0;			/* required display number */
    int iscreen = 0;			/* optional screen number */
    /*  int (*connfunc)(); */		/* method to create connection */
    int len, hostlen;			/* length tmp variable */
    int retry;				/* retry counter */
    char addrbuf[128];			/* final address passed to
					   X Transport Interface */
    char* address = addrbuf;
    XtransConnInfo trans_conn = NULL;	/* transport connection object */
    int connect_stat;
#ifdef LOCALCONN
    struct utsname sys;
#endif
#ifdef TCPCONN
    char *tcphostname = NULL;		/* A place to save hostname pointer */
#endif

    p = display_name;

    saddrlen = 0;			/* set so that we can clear later */
    saddr = NULL;

#if defined(NX_TRANS_SOCKET) && defined(NX_TRANS_TEST)
        fprintf(stderr, "_X11TransConnectDisplay: Called with display_name [%s].\n", display_name);
#endif

#ifdef NX_TRANS_SOCKET

    /*
     * Check if user selected the "nx"
     * protocol or an "nx" hostname.
     */

    if (!strncasecmp(p, "nx/", 3) || !strcasecmp(p, "nx") ||
            !strncasecmp(p, "nx:", 3) || !strncasecmp(p, "nx,", 3))
    {
        if (*(display_name + 2) == '/')
        {
          p += 3;
        }

        pprotocol = copystring ("nx", 2);

        if (!pprotocol) goto bad;

#ifdef NX_TRANS_TEST
        fprintf(stderr, "_X11TransConnectDisplay: Forced protocol to [%s].\n", pprotocol);
#endif

    }
    else
    {

#endif

    /*
     * Step 0, find the protocol.  This is delimited by the optional 
     * slash ('/').
     */
    for (lastp = p; *p && *p != ':' && *p != '/'; p++) ;
    if (!*p) return NULL;		/* must have a colon */

    if (p != lastp && *p != ':') {	/* protocol given? */
	pprotocol = copystring (lastp, p - lastp);
	if (!pprotocol) goto bad;	/* no memory */
	p++;				/* skip the '/' */
    } else
	p = display_name;		/* reset the pointer in
					   case no protocol was given */
#ifdef NX_TRANS_SOCKET

    } /* End of step 0. */

    /*
     * Check if user specified the "nx" protocol or
     * hostname is "nx" or in the form "nx,...".
     */

    if (pprotocol && !strcasecmp(pprotocol, "nx"))
    {

#ifdef NX_TRANS_TEST
        fprintf(stderr, "_X11TransConnectDisplay: Checking hostname [%s].\n", p);
#endif

        /*
         * Options can include a "display=" tuple so
         * need to scan right to left.
         */

        lastp = p;
        lastc = NULL;

        for (; *p; p++)
            if (*p == ':')
                lastc = p;

        /*
         * Don't complain if no screen was provided.
         */

        if (lastc)
        {
            phostname = copystring (lastp, lastc - lastp);

            p = lastc;
        }
        else
        {
            phostname = copystring (lastp, strlen(lastp));
        }

        if (!phostname) goto bad;

#ifdef NX_TRANS_TEST
        fprintf(stderr, "_X11TransConnectDisplay: Forced hostname [%s].\n", phostname);
#endif

    }
    else
    {

#endif

    /*
     * Step 1, find the hostname.  This is delimited by either one colon,
     * or two colons in the case of DECnet (DECnet Phase V allows a single
     * colon in the hostname).  (See note above regarding IPv6 numeric 
     * addresses with triple colons or [] brackets.)
     */

    lastp = p;
    lastc = NULL;
    for (; *p; p++)
	if (*p == ':')
	    lastc = p;

    if (!lastc) return NULL;		/* must have a colon */

    if ((lastp != lastc) && (*(lastc - 1) == ':') 
#if defined(IPv6) && defined(AF_INET6)
      && ( ((lastc - 1) == lastp) || (*(lastc - 2) != ':'))
#endif
	) {
	/* DECnet display specified */

#ifndef DNETCONN
	goto bad;
#else
	dnet = True;
	/* override the protocol specified */
	if (pprotocol)
	    Xfree (pprotocol);
	pprotocol = copystring ("dnet", 4);
	hostlen = lastc - 1 - lastp;
#endif
    }
    else
	hostlen = lastc - lastp;

    if (hostlen > 0) {		/* hostname given? */
	phostname = copystring (lastp, hostlen);
	if (!phostname) goto bad;	/* no memory */
    }

    p = lastc;

#ifdef LOCALCONN
    /* check if phostname == localnodename AND protocol not specified */
    if (!pprotocol && phostname && uname(&sys) >= 0 &&
	!strncmp(phostname, sys.nodename, 
	(strlen(sys.nodename) < strlen(phostname) ? 
	strlen(phostname) : strlen(sys.nodename))))
    {
#ifdef TCPCONN
	/*
	 * We'll first attempt to connect using the local transport.  If
	 * this fails (which is the case if sshd X protocol forwarding is
	 * being used), retry using tcp and this hostname.
	 */
	tcphostname = copystring(phostname, strlen(phostname));
#endif
	Xfree (phostname);
	phostname = copystring ("unix", 4);
    }
#endif

#ifdef NX_TRANS_SOCKET

    } /* End of step 1. */

    /*
     * Check if no display was specified. In this case
     * search the "port=n" option in NX host string.
     */

    if (*p)
    {

#endif


    /*
     * Step 2, find the display number.  This field is required and is 
     * delimited either by a nul or a period, depending on whether or not
     * a screen number is present.
     */

    for (lastp = ++p; *p && isascii(*p) && isdigit(*p); p++) ;
    if ((p == lastp) ||			/* required field */
	(*p != '\0' && *p != '.') ||	/* invalid non-digit terminator */
	!(pdpynum = copystring (lastp, p - lastp)))  /* no memory */
      goto bad;
    idisplay = atoi (pdpynum);

#ifdef NX_TRANS_SOCKET

    }
    else
    {
        char *host  = NULL;
        char *name  = NULL;
        char *value = NULL;

#ifdef NX_TRANS_TEST
        fprintf(stderr, "_X11TransConnectDisplay: Searching port in port [%s].\n", phostname);
#endif

        if (!strncasecmp(phostname, "nx,", 3))
        {
            host = copystring(phostname + 3, strlen(phostname) - 3);
        }

        if (!host) goto bad;

        idisplay = -1;

        name = strtok(host, "=");

        while (name)
        {
            value = strtok(NULL, ",");

            if (value == NULL || strstr(value, "=") != NULL ||
                    strstr(name, ",") != NULL || strlen(value) >= 128)
            {
                Xfree(host);

                goto bad;
            }
            else if (strcasecmp(name, "port") == 0)
            {
                idisplay = atoi(value);

                pdpynum = copystring(value, strlen(value));

                if (!pdpynum) goto bad;

                break;
            }

            name = strtok(NULL, "=");
        }

        Xfree(host);

        if (idisplay == -1)
        {
            goto bad;
        }

    } /* End of step 2. */

#endif


    /*
     * Step 3, find the screen number.  This field is optional.  It is 
     * present only if the display number was followed by a period (which
     * we've already verified is the only non-nul character).
     */

    if (*p) {
	for (lastp = ++p; *p && isascii(*p) && isdigit (*p); p++) ;
	if (p != lastp) {
	    if (*p ||			/* non-digits */
		!(pscrnum = copystring (lastp, p - lastp))) /* no memory */
		goto bad;
	    iscreen = atoi (lastp);
	}
    }

    /*
     * At this point, we know the following information:
     *
     *     pprotocol                protocol string or NULL
     *     phostname                hostname string or NULL
     *     idisplay                 display number
     *     iscreen                  screen number
     *     dnet                     DECnet boolean
     * 
     * We can now decide which transport to use based on the ConnectionFlags
     * build parameter the hostname string.  If phostname is NULL or equals
     * the string "local", then choose the best transport.  If phostname
     * is "unix", then choose BSD UNIX domain sockets (if configured).
     */

#ifdef NX_TRANS_SOCKET

    /*
     * If user selected the "nx" protocol
     * force "local" transport.
     */

    if (pprotocol && !strcasecmp(pprotocol, "nx"))
    {
        pprotocol = copystring ("local", 5);

        if (!pprotocol) goto bad;

#ifdef NX_TRANS_TEST
        fprintf(stderr, "_X11TransConnectDisplay: Converted protocol to [%s].\n", pprotocol);
#endif

    }

#endif

#if defined(TCPCONN) || defined(UNIXCONN) || defined(LOCALCONN) || defined(MNX_TCPCONN) || defined(OS2PIPECONN)
    if (!pprotocol) {
	if (!phostname) {
#if defined(UNIXCONN) || defined(LOCALCONN) || defined(OS2PIPECONN)
	    pprotocol = copystring ("local", 5);
#if defined(TCPCONN)
	    tcphostname = copystring("localhost", 9);
#endif
	}
	else
	{
#endif
	    pprotocol = copystring ("tcp", 3);
	}
    }
#endif

#if defined(UNIXCONN) || defined(LOCALCONN) || defined(OS2PIPECONN)
    /*
     * Now that the defaults have been established, see if we have any 
     * special names that we have to override:
     *
     *     :N         =>     if UNIXCONN then unix-domain-socket
     *     ::N        =>     if UNIXCONN then unix-domain-socket
     *     unix:N     =>     if UNIXCONN then unix-domain-socket
     *
     * Note that if UNIXCONN isn't defined, then we can use the default
     * transport connection function set above.
     */

    if (!phostname) {
#ifdef apollo
	;   /* Unix domain sockets are *really* bad on apollos */
#else
	if( pprotocol ) Xfree(pprotocol);
	pprotocol = copystring ("local", 5);
#endif
    }
    else if (strcmp (phostname, "unix") == 0) {
	if( pprotocol ) Xfree(pprotocol);
	pprotocol = copystring ("local", 5);
    }
#endif

#if defined(TCPCONN)
  connect:
#endif
    /*
     * This seems kind of backwards, but we need to put the protocol,
     * host, and port back together to pass to _X11TransOpenCOTSClient().
     */

    {
	int olen = 3 + (pprotocol ? strlen(pprotocol) : 0) + 
		       (phostname ? strlen(phostname) : 0) + 
		       (pdpynum   ? strlen(pdpynum)   : 0);
	if (olen > sizeof addrbuf) address = Xmalloc (olen);
    }
    if (!address) goto bad;

    sprintf(address,"%s/%s:%d",
	pprotocol ? pprotocol : "",
	phostname ? phostname : "",
	idisplay );

    /*
     * Make the connection, also need to get the auth address info for
     * the connection.  Do retries in case server host has hit its
     * backlog (which, unfortunately, isn't distinguishable from there not
     * being a server listening at all, which is why we have to not retry
     * too many times).
     */
#if defined(NX_TRANS_SOCKET) && defined(NX_TRANS_TEST)
        fprintf(stderr, "_X11TransConnectDisplay: Entering connection loop.\n");
#endif
    for(retry=X_CONNECTION_RETRIES; retry>=0; retry-- )
	{
#if defined(NX_TRANS_SOCKET) && defined(NX_TRANS_TEST)
        fprintf(stderr, "_X11TransConnectDisplay: Going to call _X11TransOpenCOTSClient(address) with address [%s].\n", address);
#endif
	if ( (trans_conn = _X11TransOpenCOTSClient(address)) == NULL )
	    {
	    break;
	    }
#if defined(NX_TRANS_SOCKET) && defined(NX_TRANS_TEST)
        fprintf(stderr, "_X11TransConnectDisplay: Going to call _X11TransConnect(trans_conn,address).\n");
#endif
	if ((connect_stat = _X11TransConnect(trans_conn,address)) < 0 )
	    {
#if defined(NX_TRANS_SOCKET) && defined(NX_TRANS_TEST)
            fprintf(stderr, "_X11TransConnectDisplay: Going to call _X11TransClose(trans_conn).\n");
#endif
	    _X11TransClose(trans_conn);
	    trans_conn = NULL;

	    if (connect_stat == TRANS_TRY_CONNECT_AGAIN)
	    {
		sleep(1);
		continue;
	    }
	    else
		break;
	    }

#if defined(NX_TRANS_SOCKET) && defined(NX_TRANS_TEST)
        fprintf(stderr, "_X11TransConnectDisplay: Going to call _X11TransGetPeerAddr(trans_conn, &family, &saddrlen, &saddr).\n");
#endif
	_X11TransGetPeerAddr(trans_conn, &family, &saddrlen, &saddr);

	/*
	 * The family is given in a socket format (ie AF_INET). This
	 * will convert it to the format used by the authorization and
	 * X protocol (ie FamilyInternet).
	 */

#if defined(NX_TRANS_SOCKET) && defined(NX_TRANS_TEST)
        fprintf(stderr, "_X11TransConnectDisplay: Going to call _X11TransConvertAddress(&family, &saddrlen, &saddr).\n");
#endif
	if( _X11TransConvertAddress(&family, &saddrlen, &saddr) < 0 )
	    {
	    _X11TransClose(trans_conn);
	    trans_conn = NULL;
	    sleep(1);
	    if (saddr)
	    {
		free ((char *) saddr);
		saddr = NULL;
	    }
	    continue;
	    }

	break;
	}

#if defined(NX_TRANS_SOCKET) && defined(NX_TRANS_TEST)
        fprintf(stderr, "_X11TransConnectDisplay: Out of connection loop.\n");
#endif
    if (address != addrbuf) Xfree (address);
    address = addrbuf;

    if( trans_conn == NULL )
      goto bad;

    /*
     * Set close-on-exec so that programs that fork() doesn't get confused.
     */

    _X11TransSetOption(trans_conn,TRANS_CLOSEONEXEC,1);

    /*
     * Build the expanded display name:
     *
     *     [host] : [:] dpy . scr \0
     */
    len = ((phostname ? strlen(phostname) : 0) + 1 + (dnet ? 1 : 0) +
	   strlen(pdpynum) + 1 + (pscrnum ? strlen(pscrnum) : 1) + 1);
    *fullnamep = (char *) Xmalloc (len);
    if (!*fullnamep) goto bad;

    sprintf (*fullnamep, "%s%s%d.%d",
	     (phostname ? phostname : ""),
	     (dnet ? "::" : ":"),
	     idisplay, iscreen);

    *dpynump = idisplay;
    *screenp = iscreen;
    if (pprotocol) Xfree (pprotocol);
    if (phostname) Xfree (phostname);
    if (pdpynum) Xfree (pdpynum);
    if (pscrnum) Xfree (pscrnum);
#ifdef TCPCONN
    if (tcphostname) Xfree (tcphostname);
#endif

    GetAuthorization(trans_conn, family, (char *) saddr, saddrlen, idisplay,
		     auth_namep, auth_namelenp, auth_datap, auth_datalenp);
    return trans_conn;


    /*
     * error return; make sure everything is cleaned up.
     */
  bad:
    if (trans_conn) (void)_X11TransClose(trans_conn);
    if (saddr) free ((char *) saddr);
    if (pprotocol) Xfree (pprotocol);
    if (phostname) Xfree (phostname);
    if (address && address != addrbuf) { Xfree(address); address = addrbuf; }

#if defined(TCPCONN)
    if (tcphostname) {
	pprotocol = copystring("tcp", 3);
	phostname = tcphostname;
	tcphostname = NULL;
	goto connect;
    }
#endif

    if (pdpynum) Xfree (pdpynum);
    if (pscrnum) Xfree (pscrnum);
    return NULL;

}

/*
 * This is gross, but we need it for compatiblity.
 * The test suite relies on the following interface.
 *
 */

int _XConnectDisplay (
    char *display_name,
    char **fullnamep,			/* RETURN */
    int *dpynump,			/* RETURN */
    int *screenp,			/* RETURN */
    char **auth_namep,			/* RETURN */
    int *auth_namelenp,			/* RETURN */
    char **auth_datap,			/* RETURN */
    int *auth_datalenp)			/* RETURN */
{
   XtransConnInfo trans_conn;

   trans_conn = _X11TransConnectDisplay (
		      display_name, fullnamep, dpynump, screenp,
		      auth_namep, auth_namelenp, auth_datap, auth_datalenp);

   if (trans_conn)
   {
       int fd = _X11TransGetConnectionNumber (trans_conn);
       _X11TransFreeConnInfo (trans_conn);
       return (fd);
   }
   else
       return (-1);
}


/*****************************************************************************
 *                                                                           *
 *			  Connection Utility Routines                        *
 *                                                                           *
 *****************************************************************************/

/* 
 * Disconnect from server.
 */

int _XDisconnectDisplay (trans_conn)

XtransConnInfo	trans_conn;

{
    _X11TransDisconnect(trans_conn);
    _X11TransClose(trans_conn);
    return 0;
}



Bool
_XSendClientPrefix (dpy, client, auth_proto, auth_string, prefix)
     Display *dpy;
     xConnClientPrefix *client;		/* contains count for auth_* */
     char *auth_proto, *auth_string;	/* NOT null-terminated */
     xConnSetupPrefix *prefix;		/* prefix information */
{
    int auth_length = client->nbytesAuthProto;
    int auth_strlen = client->nbytesAuthString;
    static char padbuf[3];		/* for padding to 4x bytes */
    int pad;
    struct iovec iovarray[5], *iov = iovarray;
    int niov = 0;
    int len = 0;

#define add_to_iov(b,l) \
  { iov->iov_base = (b); iov->iov_len = (l); iov++, niov++; len += (l); }

    add_to_iov ((caddr_t) client, SIZEOF(xConnClientPrefix));

    /*
     * write authorization protocol name and data
     */
    if (auth_length > 0) {
	add_to_iov (auth_proto, auth_length);
	pad = -auth_length & 3; /* pad auth_length to a multiple of 4 */
	if (pad) add_to_iov (padbuf, pad);
    }
    if (auth_strlen > 0) {
	add_to_iov (auth_string, auth_strlen);
	pad = -auth_strlen & 3; /* pad auth_strlen to a multiple of 4 */
	if (pad) add_to_iov (padbuf, pad);
    }

#undef add_to_iov

    len -= _X11TransWritev (dpy->trans_conn, iovarray, niov);

    /*
     * Set the connection non-blocking since we use select() to block.
     */

    _X11TransSetOption(dpy->trans_conn, TRANS_NONBLOCKING, 1);

    if (len != 0)
	return -1;
#ifdef NX_TRANS_SOCKET
    if (_NXDisplayWriteFunction != NULL) {
        (*_NXDisplayWriteFunction)(dpy, len);
    }
#ifdef NX_TRANS_CHANGE
    if (_NXDisplayCongestionFunction != NULL &&
            _X11TransSocketCongestionChange(dpy->trans_conn, &congestion) == 1) {
        (*_NXDisplayCongestionFunction)(dpy, congestion);
    }
#endif
#endif

#ifdef K5AUTH
    if (auth_length == 14 &&
	!strncmp(auth_proto, "MIT-KERBEROS-5", 14))
    {
	return k5_clientauth(dpy, prefix);
    } else
#endif
    return 0;
}


#ifdef STREAMSCONN
#ifdef SVR4
#include <tiuser.h>
#else
#undef HASXDMAUTH
#endif
#endif

#ifdef SECURE_RPC
#include <rpc/rpc.h>
#ifdef ultrix
#include <time.h>
#include <rpc/auth_des.h>
#endif
#endif

#ifdef HASXDMAUTH
#include <time.h>
#define Time_t time_t
#endif

/*
 * First, a routine for setting authorization data
 */
static int xauth_namelen = 0;
static char *xauth_name = NULL;	 /* NULL means use default mechanism */
static int xauth_datalen = 0;
static char *xauth_data = NULL;	 /* NULL means get default data */

/*
 * This is a list of the authorization names which Xlib currently supports.
 * Xau will choose the file entry which matches the earliest entry in this
 * array, allowing us to prioritize these in terms of the most secure first
 */

static char *default_xauth_names[] = {
#ifdef K5AUTH
    "MIT-KERBEROS-5",
#endif
#ifdef SECURE_RPC
    "SUN-DES-1",
#endif
#ifdef HASXDMAUTH
    "XDM-AUTHORIZATION-1",
#endif
    "MIT-MAGIC-COOKIE-1"
};

static _Xconst int default_xauth_lengths[] = {
#ifdef K5AUTH
    14,     /* strlen ("MIT-KERBEROS-5") */
#endif
#ifdef SECURE_RPC
    9,	    /* strlen ("SUN-DES-1") */
#endif
#ifdef HASXDMAUTH
    19,	    /* strlen ("XDM-AUTHORIZATION-1") */
#endif
    18	    /* strlen ("MIT-MAGIC-COOKIE-1") */
};

#define NUM_DEFAULT_AUTH    (sizeof (default_xauth_names) / sizeof (default_xauth_names[0]))
    
static char **xauth_names = default_xauth_names;
static _Xconst int  *xauth_lengths = default_xauth_lengths;

static int  xauth_names_length = NUM_DEFAULT_AUTH;

void XSetAuthorization (name, namelen, data, datalen)
    int namelen, datalen;		/* lengths of name and data */
    char *name, *data;			/* NULL or arbitrary array of bytes */
{
    char *tmpname, *tmpdata;

    _XLockMutex(_Xglobal_lock);
    if (xauth_name) Xfree (xauth_name);	 /* free any existing data */
    if (xauth_data) Xfree (xauth_data);

    xauth_name = xauth_data = NULL;	/* mark it no longer valid */
    xauth_namelen = xauth_datalen = 0;
    _XUnlockMutex(_Xglobal_lock);

    if (namelen < 0) namelen = 0;	/* check for bogus inputs */
    if (datalen < 0) datalen = 0;	/* maybe should return? */

    if (namelen > 0)  {			/* try to allocate space */
	tmpname = Xmalloc ((unsigned) namelen);
	if (!tmpname) return;
	memcpy (tmpname, name, namelen);
    } else {
	tmpname = NULL;
    }

    if (datalen > 0)  {
	tmpdata = Xmalloc ((unsigned) datalen);
	if (!tmpdata) {
	    if (tmpname) (void) Xfree (tmpname);
	    return;
	}
	memcpy (tmpdata, data, datalen);
    } else {
	tmpdata = NULL;
    }

    _XLockMutex(_Xglobal_lock);
    xauth_name = tmpname;		/* and store the suckers */
    xauth_namelen = namelen;
    if (tmpname)
    {
	xauth_names = &xauth_name;
	xauth_lengths = &xauth_namelen;
	xauth_names_length = 1;
    }
    else
    {
	xauth_names = default_xauth_names;
	xauth_lengths = default_xauth_lengths;
	xauth_names_length = NUM_DEFAULT_AUTH;
    }
    xauth_data = tmpdata;
    xauth_datalen = datalen;
    _XUnlockMutex(_Xglobal_lock);
    return;
}

#ifdef SECURE_RPC
/*
 * Create a credential that we can send to the X server.
 */
static int
auth_ezencode(servername, window, cred_out, len)
        char           *servername;
        int             window;
	char	       *cred_out;
        int            *len;
{
        AUTH           *a;
        XDR             xdr;

#if defined(SVR4) && defined(sun)
        a = authdes_seccreate(servername, window, NULL, NULL);
#else
        a = (AUTH *)authdes_create(servername, window, NULL, NULL);
#endif
        if (a == (AUTH *)NULL) {
                perror("auth_create");
                return 0;
        }
        xdrmem_create(&xdr, cred_out, *len, XDR_ENCODE);
        if (AUTH_MARSHALL(a, &xdr) == FALSE) {
                perror("auth_marshall");
                AUTH_DESTROY(a);
                return 0;
        }
        *len = xdr_getpos(&xdr);
        AUTH_DESTROY(a);
	return 1;
}
#endif

#ifdef K5AUTH
#include <com_err.h>

extern krb5_flags krb5_kdc_default_options;

/*
 * k5_clientauth
 *
 * Returns non-zero if the setup prefix has been read,
 * so we can tell XOpenDisplay to not bother looking for it by
 * itself.
 */
static int k5_clientauth(dpy, sprefix)
    Display *dpy;
    xConnSetupPrefix *sprefix;
{
    krb5_error_code retval;
    xReq prefix;
    char *buf;
    CARD16 plen, tlen;
    krb5_data kbuf;
    krb5_ccache cc;
    krb5_creds creds;
    krb5_principal cprinc, sprinc;
    krb5_ap_rep_enc_part *repl;

    krb5_init_ets();
    /*
     * stage 0: get encoded principal and tgt from server
     */
    _XRead(dpy, (char *)&prefix, sz_xReq);
    if (prefix.reqType != 2 && prefix.reqType != 3)
	/* not an auth packet... so deal */
	if (prefix.reqType == 0 || prefix.reqType == 1)
	{
	    memcpy((char *)sprefix, (char *)&prefix, sz_xReq);
	    _XRead(dpy, (char *)sprefix + sz_xReq,
		   sz_xConnSetupPrefix - sz_xReq); /* ewww... gross */
	    return 1;
	}
	else
	{
	    fprintf(stderr,
		    "Xlib: Krb5 stage 0: got illegal connection setup success code %d\n",
		    prefix.reqType);
	    return -1;
	}
    if (prefix.data != 0)
    {
	fprintf(stderr, "Xlib: got out of sequence (%d) packet in Krb5 auth\n",
		prefix.data);
	return -1;
    }
    buf = (char *)malloc((prefix.length << 2) - sz_xReq);
    if (buf == NULL)		/* malloc failed.  Run away! */
    {
	fprintf(stderr, "Xlib: malloc bombed in Krb5 auth\n");
	return -1;
    }
    tlen = (prefix.length << 2) - sz_xReq;
    _XRead(dpy, buf, tlen);
    if (prefix.reqType == 2 && tlen < 6)
    {
	fprintf(stderr, "Xlib: Krb5 stage 0 reply from server too short\n");
	free(buf);
	return -1;
    }
    if (prefix.reqType == 2)
    {
	plen = *(CARD16 *)buf;
	kbuf.data = buf + 2;
	kbuf.length = (plen > tlen) ? tlen : plen;
    }
    else
    {
	kbuf.data = buf;
	kbuf.length = tlen;
    }
    if (XauKrb5Decode(kbuf, &sprinc))
    {
	free(buf);
	fprintf(stderr, "Xlib: XauKrb5Decode bombed\n");
	return -1;
    }
    if (prefix.reqType == 3)	/* do some special stuff here */
    {
	char *sname, *hostname = NULL;

	sname = (char *)malloc(krb5_princ_component(sprinc, 0)->length + 1);
	if (sname == NULL)
	{
	    free(buf);
	    krb5_free_principal(sprinc);
	    fprintf(stderr, "Xlib: malloc bombed in Krb5 auth\n");
	    return -1;
	}
	memcpy(sname, krb5_princ_component(sprinc, 0)->data,
	       krb5_princ_component(sprinc, 0)->length);
	sname[krb5_princ_component(sprinc, 0)->length] = '\0';
	krb5_free_principal(sprinc);
	if (dpy->display_name[0] != ':') /* hunt for a hostname */
	{
	    char *t;

	    if ((hostname = (char *)malloc(strlen(dpy->display_name)))
		== NULL)
	    {
		free(buf);
		free(sname);
		fprintf(stderr, "Xlib: malloc bombed in Krb5 auth\n");
		return -1;
	    }
	    strcpy(hostname, dpy->display_name);
	    t = strchr(hostname, ':');
	    if (t == NULL)
	    {
		free(buf);
		free(sname);
		free(hostname);
		fprintf(stderr,
			"Xlib: shouldn't get here! malformed display name.");
		return -1;
	    }
	    if ((t - hostname + 1 < strlen(hostname)) && t[1] == ':')
		t++;
	    *t = '\0';		/* truncate the dpy number out */
	}
	retval = krb5_sname_to_principal(hostname, sname,
					 KRB5_NT_SRV_HST, &sprinc);
	free(sname);
	if (hostname)
	    free(hostname);
	if (retval)
	{
	    free(buf);
	    fprintf(stderr, "Xlib: krb5_sname_to_principal failed: %s\n",
		    error_message(retval));
	    return -1;
	}
    }
    if (retval = krb5_cc_default(&cc))
    {
	free(buf);
	krb5_free_principal(sprinc);
	fprintf(stderr, "Xlib: krb5_cc_default failed: %s\n",
		error_message(retval));
	return -1;
    }
    if (retval = krb5_cc_get_principal(cc, &cprinc))
    {
	free(buf);
	krb5_free_principal(sprinc);
	fprintf(stderr, "Xlib: cannot get Kerberos principal from \"%s\": %s\n",
		krb5_cc_default_name(), error_message(retval));
	return -1;
    }
    bzero((char *)&creds, sizeof(creds));
    creds.server = sprinc;
    creds.client = cprinc;
    if (prefix.reqType == 2)
    {
	creds.second_ticket.length = tlen - plen - 2;
	creds.second_ticket.data = buf + 2 + plen;
	retval = krb5_get_credentials(KRB5_GC_USER_USER |
				      krb5_kdc_default_options,
				      cc, &creds);
    }
    else
	retval = krb5_get_credentials(krb5_kdc_default_options,
				      cc, &creds);
    if (retval)
    {
	free(buf);
	krb5_free_cred_contents(&creds);
	fprintf(stderr, "Xlib: cannot get Kerberos credentials: %s\n",
		error_message(retval));
	return -1;
    }
    /*
     * now format the ap_req to send to the server
     */
    if (prefix.reqType == 2)
	retval = krb5_mk_req_extended(AP_OPTS_USE_SESSION_KEY |
				      AP_OPTS_MUTUAL_REQUIRED, NULL,
				      0, 0, NULL, cc,
				      &creds, NULL, &kbuf);
    else
	retval = krb5_mk_req_extended(AP_OPTS_MUTUAL_REQUIRED, NULL,
				      0, 0, NULL, cc, &creds, NULL,
				      &kbuf);
    free(buf);
    if (retval)			/* Some manner of Kerberos lossage */
    {
	krb5_free_cred_contents(&creds);
	fprintf(stderr, "Xlib: krb5_mk_req_extended failed: %s\n",
		error_message(retval));
	return -1;
    }
    prefix.reqType = 1;
    prefix.data = 0;
    prefix.length = (kbuf.length + sz_xReq + 3) >> 2;
    /*
     * stage 1: send ap_req to server
     */
    _XSend(dpy, (char *)&prefix, sz_xReq);
    _XSend(dpy, (char *)kbuf.data, kbuf.length);
    free(kbuf.data);
    /*
     * stage 2: get ap_rep from server to mutually authenticate
     */
    _XRead(dpy, (char *)&prefix, sz_xReq);
    if (prefix.reqType != 2)
	if (prefix.reqType == 0 || prefix.reqType == 1)
	{
	    memcpy((char *)sprefix, (char *)&prefix, sz_xReq);
	    _XRead(dpy, (char *)sprefix + sz_xReq,
		   sz_xConnSetupPrefix - sz_xReq);
	    return 1;
	}
	else
	{
	    fprintf(stderr,
		    "Xlib: Krb5 stage 2: got illegal connection setup success code %d\n",
		    prefix.reqType);
	    return -1;
	}
    if (prefix.data != 2)
	return -1;
    kbuf.length = (prefix.length << 2) - sz_xReq;
    kbuf.data = (char *)malloc(kbuf.length);
    if (kbuf.data == NULL)
    {
	fprintf(stderr, "Xlib: malloc bombed in Krb5 auth\n");
	return -1;
    }
    _XRead(dpy, (char *)kbuf.data, kbuf.length);
    retval = krb5_rd_rep(&kbuf, &creds.keyblock, &repl);
    if (retval)
    {
	free(kbuf.data);
	fprintf(stderr, "Xlib: krb5_rd_rep failed: %s\n",
		error_message(retval));
	return -1;
    }
    free(kbuf.data);
    /*
     * stage 3: send a short ack to the server and return
     */
    prefix.reqType = 3;
    prefix.data = 0;
    prefix.length = sz_xReq >> 2;
    _XSend(dpy, (char *)&prefix, sz_xReq);
    return 0;
}
#endif /* K5AUTH */

static void
GetAuthorization(
    XtransConnInfo trans_conn,
    int family,
    char *saddr,
    int saddrlen,
    int idisplay,
    char **auth_namep,			/* RETURN */
    int *auth_namelenp,			/* RETURN */
    char **auth_datap,			/* RETURN */
    int *auth_datalenp)			/* RETURN */
{
#ifdef SECURE_RPC
    char rpc_cred[MAX_AUTH_BYTES];
#endif
#ifdef HASXDMAUTH
    unsigned char xdmcp_data[192/8];
#endif
    char *auth_name;
    int auth_namelen;
    unsigned char *auth_data;
    int auth_datalen;
    Xauth *authptr = NULL;

/*
 * Look up the authorization protocol name and data if necessary.
 */
    if (xauth_name && xauth_data) {
	auth_namelen = xauth_namelen;
	auth_name = xauth_name;
	auth_datalen = xauth_datalen;
	auth_data = (unsigned char *) xauth_data;
    } else {
	char dpynumbuf[40];		/* big enough to hold 2^64 and more */
	(void) sprintf (dpynumbuf, "%d", idisplay);

	authptr = XauGetBestAuthByAddr ((unsigned short) family,
				    (unsigned short) saddrlen,
				    saddr,
				    (unsigned short) strlen (dpynumbuf),
				    dpynumbuf,
				    xauth_names_length,
				    xauth_names,
				    xauth_lengths);
	if (authptr) {
	    auth_namelen = authptr->name_length;
	    auth_name = (char *)authptr->name;
	    auth_datalen = authptr->data_length;
	    auth_data = (unsigned char *) authptr->data;
	} else {
	    auth_namelen = 0;
	    auth_name = NULL;
	    auth_datalen = 0;
	    auth_data = NULL;
	}
    }
#ifdef HASXDMAUTH
    /*
     * build XDM-AUTHORIZATION-1 data
     */
    if (auth_namelen == 19 && !strncmp (auth_name, "XDM-AUTHORIZATION-1", 19))
    {
	int i, j;
	Time_t  now;
	int family, addrlen;
	Xtransaddr *addr = NULL;

	for (j = 0; j < 8; j++)
	    xdmcp_data[j] = auth_data[j];

	_X11TransGetMyAddr(trans_conn, &family, &addrlen, &addr);

	switch( family )
	{
#ifdef AF_INET
	case AF_INET:
	{
	    /*
	     * addr will contain a sockaddr_in with all
	     * of the members already in network byte order.
	     */

	    for(i=4; i<8; i++)	/* do sin_addr */
		xdmcp_data[j++] = ((char *)addr)[i];
	    for(i=2; i<4; i++)	/* do sin_port */
		xdmcp_data[j++] = ((char *)addr)[i];
	    break;
	}
#endif /* AF_INET */
#if defined(IPv6) && defined(AF_INET6)
	case AF_INET6:
	  /* XXX This should probably never happen */
	{
	    unsigned char ipv4mappedprefix[] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff };
	    
	    /* In the case of v4 mapped addresses send the v4 
	       part of the address - addr is already in network byte order */
	    if (memcmp(addr+8, ipv4mappedprefix, 12) == 0) {
		for (i = 20 ; i < 24; i++)
		    xdmcp_data[j++] = ((char *)addr)[i];
	    
		/* Port number */
		for (i=2; i<4; i++)
		    xdmcp_data[j++] = ((char *)addr)[i];
	    } else {
		/* Fake data to keep the data aligned. Otherwise the 
		   the server will bail about incorrect timing data */
		for (i = 0; i < 6; i++) {
		    xdmcp_data[j++] = 0;
		}
	    }
	    break;
	}
#endif /* AF_INET6 */
#ifdef AF_UNIX
	case AF_UNIX:
	{
	    /*
	     * We don't use the sockaddr_un for this encoding.
	     * Instead, we create a sockaddr_in filled with
	     * a decreasing counter for the address, and the
	     * pid for the port.
	     */

	    static unsigned long    unix_addr = 0xFFFFFFFF;
	    unsigned long	the_addr;
	    unsigned short	the_port;
	    unsigned long	the_utime;
	    struct timeval      tp;
	    
	    X_GETTIMEOFDAY(&tp);
	    _XLockMutex(_Xglobal_lock);
	    the_addr = unix_addr--;
	    _XUnlockMutex(_Xglobal_lock);
	    the_utime = (unsigned long) tp.tv_usec;
	    the_port = getpid ();
	    
	    xdmcp_data[j++] = (the_utime >> 24) & 0xFF;
	    xdmcp_data[j++] = (the_utime >> 16) & 0xFF;
	    xdmcp_data[j++] = ((the_utime >>  8) & 0xF0)
		| ((the_addr >>  8) & 0x0F);
	    xdmcp_data[j++] = (the_addr >>  0) & 0xFF;
	    xdmcp_data[j++] = (the_port >>  8) & 0xFF;
	    xdmcp_data[j++] = (the_port >>  0) & 0xFF;
	    break;
	}
#endif /* AF_UNIX */
#ifdef AF_DECnet
	case AF_DECnet:
	    /*
	     * What is the defined encoding for this?
	     */
	    break;
#endif /* AF_DECnet */
	default:
	    /*
	     * Need to return some kind of errro status here.
	     * maybe a NULL auth??
	     */
	    break;
	} /* switch */

	if (addr)
	    free ((char *) addr);

	time (&now);
	xdmcp_data[j++] = (now >> 24) & 0xFF;
	xdmcp_data[j++] = (now >> 16) & 0xFF;
	xdmcp_data[j++] = (now >>  8) & 0xFF;
	xdmcp_data[j++] = (now >>  0) & 0xFF;
	while (j < 192 / 8)
	    xdmcp_data[j++] = 0;
	_XLockMutex(_Xglobal_lock);
	/* this function might use static data, hence the lock around it */
	XdmcpWrap (xdmcp_data, auth_data + 8,
		      xdmcp_data, j);
	_XUnlockMutex(_Xglobal_lock);
	auth_data = xdmcp_data;
	auth_datalen = j;
    }
#endif /* HASXDMAUTH */
#ifdef SECURE_RPC
    /*
     * The SUN-DES-1 authorization protocol uses the
     * "secure RPC" mechanism in SunOS 4.0+.
     */
    if (auth_namelen == 9 && !strncmp(auth_name, "SUN-DES-1", 9)) {
	char servernetname[MAXNETNAMELEN + 1];

	/*
	 * Copy over the server's netname from the authorization
	 * data field filled in by XauGetAuthByAddr().
	 */
	if (auth_datalen > MAXNETNAMELEN) {
	    auth_datalen = 0;
	    auth_data = NULL;
	} else {
	    memcpy(servernetname, auth_data, auth_datalen);
	    servernetname[auth_datalen] = '\0';

	    auth_datalen = sizeof (rpc_cred);
	    if (auth_ezencode(servernetname, 100, rpc_cred,
			      &auth_datalen))
		auth_data = (unsigned char *) rpc_cred;
	    else {
		auth_datalen = 0;
		auth_data = NULL;
	    }
	}
    }
#endif
    if (saddr) free ((char *) saddr);
    if ((*auth_namelenp = auth_namelen))
    {
	if ((*auth_namep = Xmalloc(auth_namelen)))
	    memcpy(*auth_namep, auth_name, auth_namelen);
	else
	    *auth_namelenp = 0;
    }
    else
	*auth_namep = NULL;
    if ((*auth_datalenp = auth_datalen))
    {
	if ((*auth_datap = Xmalloc(auth_datalen)))
	    memcpy(*auth_datap, auth_data, auth_datalen);
	else
	    *auth_datalenp = 0;
    }
    else
	*auth_datap = NULL;
    if (authptr) XauDisposeAuth (authptr);
}
