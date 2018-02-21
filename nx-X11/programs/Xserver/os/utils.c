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

Copyright 1987, 1998  The Open Group

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

Except as contained in this notice, the name of The Open Group shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from The Open Group.


Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
Copyright 1994 Quarterdeck Office Systems.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the names of Digital and
Quarterdeck not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.

DIGITAL AND QUARTERDECK DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS, IN NO EVENT SHALL DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT
OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE
OR PERFORMANCE OF THIS SOFTWARE.

*/

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifdef __CYGWIN__
#include <stdlib.h>
#include <signal.h>
#endif

#if defined(WIN32) && !defined(__CYGWIN__)
#include <nx-X11/Xwinsock.h>
#endif
#include <nx-X11/Xos.h>
#include <stdio.h>
#include "misc.h"
#include <nx-X11/X.h>
#define XSERV_t
#define TRANS_SERVER
#define TRANS_REOPEN
#include <nx-X11/Xtrans/Xtrans.h>
#include "input.h"
#include "dixfont.h"
#ifdef HAS_XFONT2
# include <X11/fonts/libxfont2.h>
#else
# include <X11/fonts/fontutil.h>
#endif /* HAS_XFONT2 */
#include "osdep.h"
#ifdef X_POSIX_C_SOURCE
#define _POSIX_C_SOURCE X_POSIX_C_SOURCE
#include <signal.h>
#undef _POSIX_C_SOURCE
#else
#if defined(X_NOT_POSIX) || defined(_POSIX_SOURCE)
#include <signal.h>
#else
#define _POSIX_SOURCE
#include <signal.h>
#undef _POSIX_SOURCE
#endif
#endif
#ifndef WIN32
#include <sys/wait.h>
#endif
#if !defined(SYSV) && !defined(WIN32)
#include <sys/resource.h>
#endif
#include <time.h>
#include <sys/stat.h>
#include <ctype.h>    /* for isspace */
#include <stdarg.h>
#include <sys/types.h>
#include <grp.h>
#include <pwd.h>

#include <stdlib.h>	/* for malloc() */

#if defined(TCPCONN)
# ifndef WIN32
#  include <netdb.h>
# endif
#endif

#include "opaque.h"

#include "dixstruct.h"

#ifdef XKB
#include "xkbsrv.h"
#endif
#ifdef XCSECURITY
#define _SECURITY_SERVER
#include <nx-X11/extensions/security.h>
#endif

#ifdef RENDER
#include "picture.h"
#endif

Bool noTestExtensions;
#ifdef BIGREQS
Bool noBigReqExtension = FALSE;
#endif
#ifdef COMPOSITE
/* COMPOSITE is disabled by default for now until the
 * interface is stable */
Bool noCompositeExtension = TRUE;
#endif
#ifdef DAMAGE
Bool noDamageExtension = FALSE;
#endif
#ifdef DBE
Bool noDbeExtension = FALSE;
#endif
#ifdef DPSEXT
Bool noDPSExtension = FALSE;
#endif
#ifdef DPMSExtension
Bool noDPMSExtension = FALSE;
#endif
#ifdef GLXEXT
Bool noGlxExtension = FALSE;
#endif
#ifdef SCREENSAVER
Bool noScreenSaverExtension = FALSE;
#endif
#ifdef MITSHM
Bool noMITShmExtension = FALSE;
#endif
#ifdef RANDR
Bool noRRExtension = FALSE;
Bool noRRXineramaExtension = FALSE;
#endif
#ifdef RENDER
Bool noRenderExtension = FALSE;
#endif
#ifdef SHAPE
Bool noShapeExtension = FALSE;
#endif
#ifdef XCSECURITY
Bool noSecurityExtension = FALSE;
#endif
#ifdef XSYNC
Bool noSyncExtension = FALSE;
#endif
#ifdef RES
Bool noResExtension = FALSE;
#endif
#ifdef XCMISC
Bool noXCMiscExtension = FALSE;
#endif
#ifdef XF86BIGFONT
Bool noXFree86BigfontExtension = FALSE;
#endif
#ifdef XF86DRI
Bool noXFree86DRIExtension = FALSE;
#endif
#ifdef XFIXES
Bool noXFixesExtension = FALSE;
#endif
/* |noXkbExtension| is defined in xc/programs/Xserver/xkb/xkbInit.c */
#ifdef PANORAMIX
/* Xinerama is disabled by default unless enabled via +xinerama */
Bool noPanoramiXExtension = TRUE;
#endif
#ifdef XINPUT
Bool noXInputExtension = FALSE;
#endif
#ifdef XIDLE
Bool noXIdleExtension = FALSE;
#endif
#ifdef XV
Bool noXvExtension = FALSE;
#endif

#define X_INCLUDE_NETDB_H
#include <nx-X11/Xos_r.h>

#include <errno.h>

#ifdef NX_TRANS_SOCKET

#include <nx/NX.h>
#include <nx/NXvars.h>

#endif

#ifdef NX_TRANS_EXIT

void (*OsVendorStartRedirectErrorFProc)() = NULL;
void (*OsVendorEndRedirectErrorFProc)() = NULL;

#endif

Bool CoreDump;

#ifdef PANORAMIX
Bool PanoramiXVisibilityNotifySent = FALSE;
Bool PanoramiXMapped = FALSE;
Bool PanoramiXWindowExposureSent = FALSE;
Bool PanoramiXOneExposeRequest = FALSE;
Bool PanoramiXExtensionDisabledHack = FALSE;
#endif

int auditTrailLevel = 1;

Bool Must_have_memory = FALSE;

#if defined(SVR4) || defined(__linux__) || defined(CSRG_BASED)
#define HAS_SAVED_IDS_AND_SETEUID
#endif

#ifdef MEMBUG
#define MEM_FAIL_SCALE 100000
long Memory_fail = 0;
#include <stdlib.h>  /* for random() */
#endif

char *dev_tty_from_init = NULL;		/* since we need to parse it anyway */

extern char dispatchExceptionAtReset;

/* Extension enable/disable in miinitext.c */
extern Bool EnableDisableExtension(char *name, Bool enable);
extern void EnableDisableExtensionError(char *name, Bool enable);

OsSigHandlerPtr
OsSignal(sig, handler)
    int sig;
    OsSigHandlerPtr handler;
{
#ifdef X_NOT_POSIX
    return signal(sig, handler);
#else
    struct sigaction act, oact;

    sigemptyset(&act.sa_mask);
    if (handler != SIG_IGN)
	sigaddset(&act.sa_mask, sig);
    act.sa_flags = 0;
    act.sa_handler = handler;
    if (sigaction(sig, &act, &oact))
	perror("sigaction");
    return oact.sa_handler;
#endif
}
	
#ifdef SERVER_LOCK
/*
 * Explicit support for a server lock file like the ones used for UUCP.
 * For architectures with virtual terminals that can run more than one
 * server at a time.  This keeps the servers from stomping on each other
 * if the user forgets to give them different display numbers.
 */
#define LOCK_DIR "/tmp"
#define LOCK_TMP_PREFIX "/.tX"
#define LOCK_PREFIX "/.X"
#define LOCK_SUFFIX "-lock"

#ifndef PATH_MAX
#include <sys/param.h>
#ifndef PATH_MAX
#ifdef MAXPATHLEN
#define PATH_MAX MAXPATHLEN
#else
#define PATH_MAX 1024
#endif
#endif
#endif

static Bool StillLocking = FALSE;
static char LockFile[PATH_MAX];
static Bool nolock = FALSE;

/*
 * LockServer --
 *      Check if the server lock file exists.  If so, check if the PID
 *      contained inside is valid.  If so, then die.  Otherwise, create
 *      the lock file containing the PID.
 */
void
LockServer(void)
{
  char tmp[PATH_MAX], pid_str[12];
  int lfd, i, haslock, l_pid, t;
  char *tmppath = NULL;
  int len;
  char port[20];

  if (nolock || NoListenAll) return;
  /*
   * Path names
   */
  tmppath = LOCK_DIR;

  sprintf(port, "%d", atoi(display));
  len = strlen(LOCK_PREFIX) > strlen(LOCK_TMP_PREFIX) ? strlen(LOCK_PREFIX) :
						strlen(LOCK_TMP_PREFIX);
  len += strlen(tmppath) + strlen(port) + strlen(LOCK_SUFFIX) + 1;
  if (len > sizeof(LockFile))
    FatalError("Display name `%s' is too long\n", port);
  (void)sprintf(tmp, "%s" LOCK_TMP_PREFIX "%s" LOCK_SUFFIX, tmppath, port);
  (void)sprintf(LockFile, "%s" LOCK_PREFIX "%s" LOCK_SUFFIX, tmppath, port);

  /*
   * Create a temporary file containing our PID.  Attempt three times
   * to create the file.
   */
  StillLocking = TRUE;
  i = 0;
  do {
    i++;
    lfd = open(tmp, O_CREAT | O_EXCL | O_WRONLY, 0644);
    if (lfd < 0)
       sleep(2);
    else
       break;
  } while (i < 3);
  if (lfd < 0) {
    unlink(tmp);
    i = 0;
    do {
      i++;
      lfd = open(tmp, O_CREAT | O_EXCL | O_WRONLY, 0644);
      if (lfd < 0)
         sleep(2);
      else
         break;
    } while (i < 3);
  }
  if (lfd < 0)
    FatalError("Could not create lock file in %s\n", tmp);
  (void) sprintf(pid_str, "%10ld\n", (long)getpid());
  if (write(lfd, pid_str, 11) != 11)
    FatalError("Could not write pid to lock file in %s\n", tmp);

#ifndef USE_CHMOD
  (void) fchmod(lfd, 0444);
#else
  (void) chmod(tmp, 0444);
#endif
  (void) close(lfd);

  /*
   * OK.  Now the tmp file exists.  Try three times to move it in place
   * for the lock.
   */
  i = 0;
  haslock = 0;
  while ((!haslock) && (i++ < 3)) {
    haslock = (link(tmp,LockFile) == 0);
    if (haslock) {
      /*
       * We're done.
       */
      break;
    }
    else {
      /*
       * Read the pid from the existing file
       */
      lfd = open(LockFile, O_RDONLY|O_NOFOLLOW);
      if (lfd < 0) {
        unlink(tmp);
        FatalError("Can't read lock file %s\n", LockFile);
      }
      pid_str[0] = '\0';
      if (read(lfd, pid_str, 11) != 11) {
        /*
         * Bogus lock file.
         */
        unlink(LockFile);
        close(lfd);
        continue;
      }
      pid_str[11] = '\0';
      sscanf(pid_str, "%d", &l_pid);
      close(lfd);

      /*
       * Now try to kill the PID to see if it exists.
       */
      errno = 0;
      t = kill(l_pid, 0);
      if ((t< 0) && (errno == ESRCH)) {
        /*
         * Stale lock file.
         */
        unlink(LockFile);
        continue;
      }
      else if (((t < 0) && (errno == EPERM)) || (t == 0)) {
        /*
         * Process is still active.
         */
        unlink(tmp);
	FatalError("Server is already active for display %s\n%s %s\n%s\n",
		   port, "\tIf this server is no longer running, remove",
		   LockFile, "\tand start again.");
      }
    }
  }
  unlink(tmp);
  if (!haslock)
    FatalError("Could not create server lock file: %s\n", LockFile);
  StillLocking = FALSE;
}

/*
 * UnlockServer --
 *      Remove the server lock file.
 */
void
UnlockServer(void)
{
  if (nolock || NoListenAll) return;

  if (!StillLocking){

  (void) unlink(LockFile);
  }
}
#endif /* SERVER_LOCK */

/* Force connections to close on SIGHUP from init */

/*ARGSUSED*/
SIGVAL
AutoResetServer (int sig)
{
    int olderrno = errno;

    dispatchException |= DE_RESET;
    isItTimeToYield = TRUE;
#ifdef GPROF
    chdir ("/tmp");
    exit (0);
#endif
#if defined(SYSV) && defined(X_NOT_POSIX)
    OsSignal (SIGHUP, AutoResetServer);
#endif
    errno = olderrno;
}

/* Force connections to close and then exit on SIGTERM, SIGINT */

/*ARGSUSED*/
SIGVAL
GiveUp(int sig)
{
    int olderrno = errno;

#if defined(NX_TRANS_SOCKET) && defined(NX_TRANS_TEST)
    fprintf(stderr, "GiveUp: Called with signal [%d].\n", sig);
#endif

    dispatchException |= DE_TERMINATE;
    isItTimeToYield = TRUE;
#if defined(SYSV) && defined(X_NOT_POSIX)
    if (sig)
	OsSignal(sig, SIG_IGN);
#endif
    errno = olderrno;
}

#ifndef DDXTIME
CARD32
GetTimeInMillis(void)
{
    struct timeval  tp;

    X_GETTIMEOFDAY(&tp);
    return(tp.tv_sec * 1000) + (tp.tv_usec / 1000);
}
#endif

void
AdjustWaitForDelay (void * waitTime, unsigned long newdelay)
{
    static struct timeval   delay_val;
    struct timeval	    **wt = (struct timeval **) waitTime;
    unsigned long	    olddelay;

    if (*wt == NULL)
    {
	delay_val.tv_sec = newdelay / 1000;
	delay_val.tv_usec = 1000 * (newdelay % 1000);
	*wt = &delay_val;
    }
    else
    {
	olddelay = (*wt)->tv_sec * 1000 + (*wt)->tv_usec / 1000;
	if (newdelay < olddelay)
	{
	    (*wt)->tv_sec = newdelay / 1000;
	    (*wt)->tv_usec = 1000 * (newdelay % 1000);
	}
    }
}

void UseMsg(void)
{
#ifdef NXAGENT_SERVER
    extern const char *__progname;
    ErrorF("Usage: %s [<options>] [:<display>]\n\n", __progname);
#else
    ErrorF("use: X [:<display>] [option]\n");
#endif
    ErrorF("-a #                   mouse acceleration (pixels)\n");
    ErrorF("-ac                    disable access control restrictions\n");
#ifdef MEMBUG
    ErrorF("-alloc int             chance alloc should fail\n");
#endif
    ErrorF("-audit int             set audit trail level\n");	
    ErrorF("-auth file             select authorization file\n");	
    ErrorF("bc                     enable bug compatibility\n");
    ErrorF("-br                    create root window with black background\n");
    ErrorF("+bs                    enable any backing store support\n");
    ErrorF("-bs                    disable any backing store support\n");
    ErrorF("-c                     turns off key-click\n");
    ErrorF("c #                    key-click volume (0-100)\n");
    ErrorF("-cc int                default color visual class\n");
#ifdef NXAGENT_SERVER
    ErrorF("-co file               deprecated, has no effect\n");
#endif
#ifdef COMMANDLINE_CHALLENGED_OPERATING_SYSTEMS
    ErrorF("-config file           read options from file\n");
#endif
    ErrorF("-core                  generate core dump on fatal error\n");
    ErrorF("-dpi int               screen resolution in dots per inch\n");
#ifdef DPMSExtension
    ErrorF("dpms                   enables VESA DPMS monitor control\n");
    ErrorF("-dpms                  disables VESA DPMS monitor control\n");
#endif
    ErrorF("-deferglyphs [none|all|16] defer loading of [no|all|16-bit] glyphs\n");
    ErrorF("-f #                   bell base (0-100)\n");
    ErrorF("-fc string             cursor font\n");
    ErrorF("-fn string             default font name\n");
    ErrorF("-fp string             default font path\n");
    ErrorF("-help                  prints message with these options\n");
    ErrorF("-I                     ignore all remaining arguments\n");
#ifdef RLIMIT_DATA
    ErrorF("-ld int                limit data space to N Kb\n");
#endif
#ifdef RLIMIT_NOFILE
    ErrorF("-lf int                limit number of open files to N\n");
#endif
#ifdef RLIMIT_STACK
    ErrorF("-ls int                limit stack space to N Kb\n");
#endif
#ifdef SERVER_LOCK
    ErrorF("-nolock                disable the locking mechanism\n");
#endif
#ifndef NOLOGOHACK
    ErrorF("-logo                  enable logo in screen saver\n");
    ErrorF("nologo                 disable logo in screen saver\n");
#endif
    ErrorF("-nolisten string       don't listen on protocol\n");
    ErrorF("-noreset               don't reset after last client exists\n");
    ErrorF("-reset                 reset after last client exists\n");
    ErrorF("-p #                   screen-saver pattern duration (minutes)\n");
    ErrorF("-pn                    accept failure to listen on all ports\n");
    ErrorF("-nopn                  reject failure to listen on all ports\n");
    ErrorF("-r                     turns off auto-repeat\n");
    ErrorF("r                      turns on auto-repeat \n");
#ifdef RENDER
    ErrorF("-render [default|mono|gray|color] set render color alloc policy\n");
#endif
    ErrorF("-s #                   screen-saver timeout (minutes)\n");
#ifdef XCSECURITY
    ErrorF("-sp file               security policy file\n");
#endif
    ErrorF("-su                    disable any save under support\n");
    ErrorF("-t #                   mouse threshold (pixels)\n");
    ErrorF("-terminate             terminate at server reset\n");
    ErrorF("-to #                  connection time out\n");
    ErrorF("-tst                   disable testing extensions\n");
    ErrorF("ttyxx                  server started from init on /dev/ttyxx\n");
    ErrorF("v                      video blanking for screen-saver\n");
    ErrorF("-v                     screen-saver without video blanking\n");
    ErrorF("-wm                    WhenMapped default backing-store\n");
    ErrorF("-maxbigreqsize         set maximal bigrequest size \n");
#ifdef PANORAMIX
    ErrorF("+xinerama              Enable XINERAMA (PanoramiX) extension\n");
    ErrorF("-xinerama              Disable XINERAMA (PanoramiX) extension (default)\n");
    ErrorF("-disablexineramaextension Disable XINERAMA extension\n");
#endif
#ifdef RANDR
    ErrorF("+rrxinerama            Enable XINERAMA (via RandR) extension (default)\n");
    ErrorF("-rrxinerama            Disable XINERAMA (via RandR) extension\n");
#endif
    ErrorF("-dumbSched             Disable smart scheduling, enable old behavior\n");
    ErrorF("-schedInterval int     Set scheduler interval in msec\n");
    ErrorF("+extension name        Enable extension\n");
    ErrorF("-extension name        Disable extension\n");
#ifdef XDMCP
    XdmcpUseMsg();
#endif
#ifdef XKB
    XkbUseMsg();
#endif
    ddxUseMsg();
}

/*  This function performs a rudimentary sanity check
 *  on the display name passed in on the command-line,
 *  since this string is used to generate filenames.
 *  It is especially important that the display name
 *  not contain a "/" and not start with a "-".
 *                                            --kvajk
 */
static int 
VerifyDisplayName(const char *d)
{
    if ( d == (char *)0 ) return( 0 );  /*  null  */
    if ( *d == '\0' ) return( 0 );  /*  empty  */
    if ( *d == '-' ) return( 0 );  /*  could be confused for an option  */
    if ( *d == '.' ) return( 0 );  /*  must not equal "." or ".."  */
    if ( strchr(d, '/') != (char *)0 ) return( 0 );  /*  very important!!!  */
    return( 1 );
}

/*
 * This function is responsible for doing initalisation of any global
 * variables at an very early point of server startup (even before
 * |ProcessCommandLine()|. 
 */
void InitGlobals(void)
{
    ddxInitGlobals();
}


/*
 * This function parses the command line. Handles device-independent fields
 * and allows ddx to handle additional fields.  It is not allowed to modify
 * argc or any of the strings pointed to by argv.
 */
void
ProcessCommandLine(int argc, char *argv[])
{
    int i, skip;

    defaultKeyboardControl.autoRepeat = TRUE;

#ifdef NO_PART_NET
    PartialNetwork = FALSE;
#else
    PartialNetwork = TRUE;
#endif

    for ( i = 1; i < argc; i++ )
    {
	/* call ddx first, so it can peek/override if it wants */
        if((skip = ddxProcessArgument(argc, argv, i)))
	{
	    i += (skip - 1);
	}
	else if(argv[i][0] ==  ':')  
	{
	    /* initialize display */
	    display = argv[i];
	    explicit_display = TRUE;
	    display++;
            if( ! VerifyDisplayName( display ) ) {
                ErrorF("Bad display name: %s\n", display);
                UseMsg();
		FatalError("Bad display name, exiting: %s\n", display);
            }
	}
	else if ( strcmp( argv[i], "-a") == 0)
	{
	    if(++i < argc)
	        defaultPointerControl.num = atoi(argv[i]);
	    else
		UseMsg();
	}
	else if ( strcmp( argv[i], "-ac") == 0)
	{
	    defeatAccessControl = TRUE;
	}
#ifdef MEMBUG
	else if ( strcmp( argv[i], "-alloc") == 0)
	{
	    if(++i < argc)
	        Memory_fail = atoi(argv[i]);
	    else
		UseMsg();
	}
#endif
	else if ( strcmp( argv[i], "-audit") == 0)
	{
	    if(++i < argc)
	        auditTrailLevel = atoi(argv[i]);
	    else
		UseMsg();
	}
	else if ( strcmp( argv[i], "-auth") == 0)
	{
	    if(++i < argc)
	        InitAuthorization (argv[i]);
	    else
		UseMsg();
	}
	else if ( strcmp( argv[i], "-br") == 0)
	    blackRoot = TRUE;
	else if ( strcmp( argv[i], "+bs") == 0)
	    enableBackingStore = TRUE;
	else if ( strcmp( argv[i], "-bs") == 0)
	    disableBackingStore = TRUE;
	else if ( strcmp( argv[i], "c") == 0)
	{
	    if(++i < argc)
	        defaultKeyboardControl.click = atoi(argv[i]);
	    else
		UseMsg();
	}
	else if ( strcmp( argv[i], "-c") == 0)
	{
	    defaultKeyboardControl.click = 0;
	}
	else if ( strcmp( argv[i], "-cc") == 0)
	{
	    if(++i < argc)
	        defaultColorVisualClass = atoi(argv[i]);
	    else
		UseMsg();
	}
#ifdef NXAGENT_SERVER
	else if ( strcmp( argv[i], "-co") == 0)
	{
	    fprintf(stderr, "Warning: Ignoring deprecated command line option '%s'.\n", argv[i]);
	    if(++i >= argc)
	        UseMsg();
	}
#endif
	else if ( strcmp( argv[i], "-core") == 0)
	    CoreDump = TRUE;
	else if ( strcmp( argv[i], "-dpi") == 0)
	{
	    if(++i < argc)
	        monitorResolution = atoi(argv[i]);
	    else
		UseMsg();
	}
	else if (strcmp(argv[i], "-displayfd") == 0) {
	    if (++i < argc) {
		displayfd = atoi(argv[i]);
		nolock = TRUE;
	    }
	    else
		UseMsg();
	}

#ifdef DPMSExtension
	else if ( strcmp( argv[i], "dpms") == 0)
	    DPMSEnabledSwitch = TRUE;
	else if ( strcmp( argv[i], "-dpms") == 0)
	    DPMSDisabledSwitch = TRUE;
#endif
	else if ( strcmp( argv[i], "-deferglyphs") == 0)
	{
#ifdef HAS_XFONT2
	    if(++i >= argc || !!xfont2_parse_glyph_caching_mode(argv[i]))
#else
	    if(++i >= argc || !ParseGlyphCachingMode(argv[i]))
#endif /* HAS_XFONT2 */
		UseMsg();
	}
	else if ( strcmp( argv[i], "-f") == 0)
	{
	    if(++i < argc)
	        defaultKeyboardControl.bell = atoi(argv[i]);
	    else
		UseMsg();
	}
	else if ( strcmp( argv[i], "-fc") == 0)
	{
	    if(++i < argc)
	        defaultCursorFont = argv[i];
	    else
		UseMsg();
	}
	else if ( strcmp( argv[i], "-fn") == 0)
	{
	    if(++i < argc)
	        defaultTextFont = argv[i];
	    else
		UseMsg();
	}
	else if ( strcmp( argv[i], "-fp") == 0)
	{
	    if(++i < argc)
	    {
	        defaultFontPath = argv[i];
	    }
	    else
		UseMsg();
	}
	else if ( strcmp( argv[i], "-help") == 0)
	{
	    UseMsg();
	    exit(0);
	}
#ifdef XKB
        else if ( (skip=XkbProcessArguments(argc,argv,i))!=0 ) {
	    if (skip>0)
		 i+= skip-1;
	    else UseMsg();
	}
#endif
#ifdef RLIMIT_DATA
	else if ( strcmp( argv[i], "-ld") == 0)
	{
	    if(++i < argc)
	    {
	        limitDataSpace = atoi(argv[i]);
		if (limitDataSpace > 0)
		    limitDataSpace *= 1024;
	    }
	    else
		UseMsg();
	}
#endif
#ifdef RLIMIT_NOFILE
	else if ( strcmp( argv[i], "-lf") == 0)
	{
	    if(++i < argc)
	        limitNoFile = atoi(argv[i]);
	    else
		UseMsg();
	}
#endif
#ifdef RLIMIT_STACK
	else if ( strcmp( argv[i], "-ls") == 0)
	{
	    if(++i < argc)
	    {
	        limitStackSpace = atoi(argv[i]);
		if (limitStackSpace > 0)
		    limitStackSpace *= 1024;
	    }
	    else
		UseMsg();
	}
#endif
#ifdef SERVER_LOCK
	else if ( strcmp ( argv[i], "-nolock") == 0)
	{
#if !defined(WIN32) && !defined(__CYGWIN__)
	  if (getuid() != 0)
	    ErrorF("Warning: the -nolock option can only be used by root\n");
	  else
#endif
	    nolock = TRUE;
	}
#endif
#ifndef NOLOGOHACK
	else if ( strcmp( argv[i], "-logo") == 0)
	{
	    logoScreenSaver = 1;
	}
	else if ( strcmp( argv[i], "nologo") == 0)
	{
	    logoScreenSaver = 0;
	}
#endif
	else if ( strcmp( argv[i], "-nolisten") == 0)
	{
            if(++i < argc) {
#ifdef NXAGENT_SERVER
		if (strcmp( argv[i], "ANY" ) == 0)
		    NoListenAll = TRUE;
		else
#endif /* NXAGENT_SERVER */
		if (_XSERVTransNoListen(argv[i])) 
		    FatalError ("Failed to disable listen for %s transport",
				argv[i]);
	   } else
		UseMsg();
	}
	else if ( strcmp( argv[i], "-noreset") == 0)
	{
	    dispatchExceptionAtReset = 0;
	}
	else if ( strcmp( argv[i], "-reset") == 0)
	{
	    dispatchExceptionAtReset = DE_RESET;
	}
	else if ( strcmp( argv[i], "-p") == 0)
	{
	    if(++i < argc)
	        defaultScreenSaverInterval = ((CARD32)atoi(argv[i])) *
					     MILLI_PER_MIN;
	    else
		UseMsg();
	}
	else if ( strcmp( argv[i], "-pn") == 0)
	    PartialNetwork = TRUE;
	else if ( strcmp( argv[i], "-nopn") == 0)
	    PartialNetwork = FALSE;
	else if ( strcmp( argv[i], "r") == 0)
	    defaultKeyboardControl.autoRepeat = TRUE;
	else if ( strcmp( argv[i], "-r") == 0)
	    defaultKeyboardControl.autoRepeat = FALSE;
	else if ( strcmp( argv[i], "-s") == 0)
	{
	    if(++i < argc)
	        defaultScreenSaverTime = ((CARD32)atoi(argv[i])) *
					 MILLI_PER_MIN;
	    else
		UseMsg();
	}
	else if ( strcmp( argv[i], "-su") == 0)
	    disableSaveUnders = TRUE;
	else if ( strcmp( argv[i], "-t") == 0)
	{
	    if(++i < argc)
	        defaultPointerControl.threshold = atoi(argv[i]);
	    else
		UseMsg();
	}
	else if ( strcmp( argv[i], "-terminate") == 0)
	{
	    dispatchExceptionAtReset = DE_TERMINATE;
	}
	else if ( strcmp( argv[i], "-to") == 0)
	{
	    if(++i < argc)
		TimeOutValue = ((CARD32)atoi(argv[i])) * MILLI_PER_SECOND;
	    else
		UseMsg();
	}
	else if ( strcmp( argv[i], "-tst") == 0)
	{
	    noTestExtensions = TRUE;
	}
	else if ( strcmp( argv[i], "v") == 0)
	    defaultScreenSaverBlanking = PreferBlanking;
	else if ( strcmp( argv[i], "-v") == 0)
	    defaultScreenSaverBlanking = DontPreferBlanking;
	else if ( strcmp( argv[i], "-wm") == 0)
	    defaultBackingStore = WhenMapped;
        else if ( strcmp( argv[i], "-maxbigreqsize") == 0) {
             if(++i < argc) {
                 long reqSizeArg = atol(argv[i]);

                 /* Request size > 128MB does not make much sense... */
                 if( reqSizeArg > 0L && reqSizeArg < 128L ) {
                     maxBigRequestSize = (reqSizeArg * 1048576L) - 1L;
                 }
                 else
                 {
                     UseMsg();
                 }
             }
             else
             {
                 UseMsg();
             }
         }
#ifdef PANORAMIX
	else if ( strcmp( argv[i], "+xinerama") == 0){
	    noPanoramiXExtension = FALSE;
	}
	else if ( strcmp( argv[i], "-xinerama") == 0){
	    noPanoramiXExtension = TRUE;
	}
	else if ( strcmp( argv[i], "-disablexineramaextension") == 0){
	    PanoramiXExtensionDisabledHack = TRUE;
	}
#endif
#ifdef RANDR
	else if ( strcmp( argv[i], "+rrxinerama") == 0){
	    noRRXineramaExtension = FALSE;
	}
	else if ( strcmp( argv[i], "-rrxinerama") == 0){
	    noRRXineramaExtension = TRUE;
	}
#endif
	else if ( strcmp( argv[i], "-I") == 0)
	{
	    /* ignore all remaining arguments */
	    break;
	}
	else if (strncmp (argv[i], "tty", 3) == 0)
	{
	    /* just in case any body is interested */
	    dev_tty_from_init = argv[i];
	}
#ifdef XDMCP
	else if ((skip = XdmcpOptions(argc, argv, i)) != i)
	{
	    i = skip - 1;
	}
#endif
#ifdef XCSECURITY
	else if ((skip = XSecurityOptions(argc, argv, i)) != i)
	{
	    i = skip - 1;
	}
#endif
#if HAVE_SETITIMER
	else if ( strcmp( argv[i], "-dumbSched") == 0)
	{
	    SmartScheduleSignalEnable = FALSE;
	}
#endif
	else if ( strcmp( argv[i], "-schedInterval") == 0)
	{
	    if (++i < argc)
	    {
		SmartScheduleInterval = atoi(argv[i]);
		SmartScheduleSlice = SmartScheduleInterval;
	    }
	    else
		UseMsg();
	}
	else if ( strcmp( argv[i], "-schedMax") == 0)
	{
	    if (++i < argc)
	    {
		SmartScheduleMaxSlice = atoi(argv[i]);
	    }
	    else
		UseMsg();
	}
#ifdef RENDER
	else if ( strcmp( argv[i], "-render" ) == 0)
	{
	    if (++i < argc)
	    {
		int policy = PictureParseCmapPolicy (argv[i]);

		if (policy != PictureCmapPolicyInvalid)
		    PictureCmapPolicy = policy;
		else
		    UseMsg ();
	    }
	    else
		UseMsg ();
	}
#endif
	else if ( strcmp( argv[i], "+extension") == 0)
	{
	    if (++i < argc)
	    {
		if (!EnableDisableExtension(argv[i], TRUE))
		    EnableDisableExtensionError(argv[i], TRUE);
	    }
	    else
		UseMsg();
	}
	else if ( strcmp( argv[i], "-extension") == 0)
	{
	    if (++i < argc)
	    {
		if (!EnableDisableExtension(argv[i], FALSE))
		    EnableDisableExtensionError(argv[i], FALSE);
	    }
	    else
		UseMsg();
	}
 	else
 	{
	    ErrorF("Unrecognized option: %s\n", argv[i]);
	    UseMsg();
	    FatalError("Unrecognized option: %s\n", argv[i]);
        }
    }
}

#ifdef COMMANDLINE_CHALLENGED_OPERATING_SYSTEMS
static void
InsertFileIntoCommandLine(
    int *resargc, char ***resargv, 
    int prefix_argc, char **prefix_argv,
    char *filename, 
    int suffix_argc, char **suffix_argv)
{
    struct stat     st;
    FILE           *f;
    char           *p;
    char           *q;
    int             insert_argc;
    char           *buf;
    int             len;
    int             i;

    f = fopen(filename, "r");
    if (!f)
	FatalError("Can't open option file %s\n", filename);

    fstat(fileno(f), &st);

    buf = (char *) malloc((unsigned) st.st_size + 1);
    if (!buf)
	FatalError("Out of Memory\n");

    len = fread(buf, 1, (unsigned) st.st_size, f);

    fclose(f);

    if (len < 0)
	FatalError("Error reading option file %s\n", filename);

    buf[len] = '\0';

    p = buf;
    q = buf;
    insert_argc = 0;

    while (*p)
    {
	while (isspace(*p))
	    p++;
	if (!*p)
	    break;
	if (*p == '#')
	{
	    while (*p && *p != '\n')
		p++;
	} else
	{
	    while (*p && !isspace(*p))
		*q++ = *p++;
	    /* Since p and q might still be pointing at the same place, we	 */
	    /* need to step p over the whitespace now before we add the null.	 */
	    if (*p)
		p++;
	    *q++ = '\0';
	    insert_argc++;
	}
    }

    buf = (char *) realloc(buf, q - buf);
    if (!buf)
	FatalError("Out of memory reallocing option buf\n");

    *resargc = prefix_argc + insert_argc + suffix_argc;
    *resargv = (char **) malloc((*resargc + 1) * sizeof(char *));
    if (!*resargv)
	FatalError("Out of Memory\n");

    memcpy(*resargv, prefix_argv, prefix_argc * sizeof(char *));

    p = buf;
    for (i = 0; i < insert_argc; i++)
    {
	(*resargv)[prefix_argc + i] = p;
	p += strlen(p) + 1;
    }

    memcpy(*resargv + prefix_argc + insert_argc,
	   suffix_argv, suffix_argc * sizeof(char *));

    (*resargv)[*resargc] = NULL;
} /* end InsertFileIntoCommandLine */


void
ExpandCommandLine(int *pargc, char ***pargv)
{
    int i;

#if !defined(WIN32) && !defined(__CYGWIN__)
    if (getuid() != geteuid())
	return;
#endif

    for (i = 1; i < *pargc; i++)
    {
	if ( (0 == strcmp((*pargv)[i], "-config")) && (i < (*pargc - 1)) )
	{
	    InsertFileIntoCommandLine(pargc, pargv,
					  i, *pargv,
					  (*pargv)[i+1], /* filename */
					  *pargc - i - 2, *pargv + i + 2);
	    i--;
	}
    }
} /* end ExpandCommandLine */
#endif

/* Implement a simple-minded font authorization scheme.  The authorization
   name is "hp-hostname-1", the contents are simply the host name. */
int
set_font_authorizations(char **authorizations, int *authlen, void * client)
{
#define AUTHORIZATION_NAME "hp-hostname-1"
#if defined(TCPCONN)
    static char *result = NULL;
    static char *p = NULL;

    if (p == NULL)
    {
	char hname[1024], *hnameptr;
	unsigned int len;
#if defined(IPv6) && defined(AF_INET6)
	struct addrinfo hints, *ai = NULL;
#else
	struct hostent *host;
#ifdef XTHREADS_NEEDS_BYNAMEPARAMS
	_Xgethostbynameparams hparams;
#endif
#endif

	gethostname(hname, 1024);
#if defined(IPv6) && defined(AF_INET6)
	bzero(&hints, sizeof(hints));
	hints.ai_flags = AI_CANONNAME;
	if (getaddrinfo(hname, NULL, &hints, &ai) == 0) {
	    hnameptr = ai->ai_canonname;
	} else {
	    hnameptr = hname;
	}
#else
	host = _XGethostbyname(hname, hparams);
	if (host == NULL)
	    hnameptr = hname;
	else
	    hnameptr = host->h_name;
#endif

	len = strlen(hnameptr) + 1;
	result = malloc(len + sizeof(AUTHORIZATION_NAME) + 4);

	p = result;
        *p++ = sizeof(AUTHORIZATION_NAME) >> 8;
        *p++ = sizeof(AUTHORIZATION_NAME) & 0xff;
        *p++ = (len) >> 8;
        *p++ = (len & 0xff);

	memmove(p, AUTHORIZATION_NAME, sizeof(AUTHORIZATION_NAME));
	p += sizeof(AUTHORIZATION_NAME);
	memmove(p, hnameptr, len);
	p += len;
#if defined(IPv6) && defined(AF_INET6)
	if (ai) {
	    freeaddrinfo(ai);
	}
#endif
    }
    *authlen = p - result;
    *authorizations = result;
    return 1;
#else /* TCPCONN */
    return 0;
#endif /* TCPCONN */
}

/*****************
 * XNFalloc 
 * "no failure" alloc
 *****************/

void *
XNFalloc(unsigned long amount)
{
    void *ptr = malloc(amount);
    if (!ptr)
    {
        FatalError("Out of memory");
    }
    return ptr;
}

/*****************
 * XNFcalloc
 *****************/

void *
XNFcalloc(unsigned long amount)
{
    void *ret = calloc(1, amount);
    if (!ret)
	FatalError("XNFcalloc: Out of memory");
    return ret;
}

/*****************
 * XNFrealloc 
 * "no failure" realloc
 *****************/

void *
XNFrealloc(void * ptr, unsigned long amount)
{
    void *ret = realloc(ptr, amount);
    if (!ret)
       FatalError("XNFrealloc: Out of memory");
    return ret;
}

void
OsInitAllocator (void)
{
#ifdef MEMBUG
    static int	been_here;

    /* Check the memory system after each generation */
    if (been_here)
	CheckMemory ();
    else
	been_here = 1;
#endif
}

char *
Xstrdup(const char *s)
{
    if (s == NULL)
	return NULL;
    return strdup(s);
}


char *
XNFstrdup(const char *s)
{
    char *ret;

    if (s == NULL)
	return NULL;

    ret = strdup(s);
    if (!ret)
       FatalError("XNFstrdup: Out of memory");
    return ret;
}

void
SmartScheduleStopTimer (void)
{
#if HAVE_SETITIMER
    struct itimerval	timer;

    if (!SmartScheduleSignalEnable)
	return;

    #ifdef NX_TRANS_TEST
    fprintf(stderr, "SmartScheduleStopTimer: Stopping timer.\n");
    #endif

    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 0;
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 0;
    (void) setitimer (ITIMER_REAL, &timer, 0);
#endif
}

void
SmartScheduleStartTimer (void)
{
#if HAVE_SETITIMER
    struct itimerval	timer;

    if (!SmartScheduleSignalEnable)
      return;

    #ifdef NX_TRANS_TEST
    fprintf(stderr, "SmartScheduleStartTimer: Starting timer with [%ld] ms.\n",
                SmartScheduleInterval);
    #endif

    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = SmartScheduleInterval * 1000;
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = SmartScheduleInterval * 1000;
    setitimer (ITIMER_REAL, &timer, 0);
#endif
}

#if HAVE_SETITIMER
static void
SmartScheduleTimer (int sig)
{
    SmartScheduleTime += SmartScheduleInterval;

    #ifdef NX_TRANS_TEST
    fprintf(stderr, "SmartScheduleTimer: Got timer with time [%ld] ms.\n",
                SmartScheduleTime);
    #endif
}

int
SmartScheduleEnable (void)
{
    int ret = 0;
    struct sigaction	act;

    if (!SmartScheduleSignalEnable)
	return 0;

    #ifdef NX_TRANS_TEST
    fprintf(stderr, "SmartScheduleEnable: Enabling the smart scheduler.\n");
    #endif

    memset((char *) &act, 0, sizeof(struct sigaction));

    /* Set up the timer signal function */
    act.sa_flags = SA_RESTART;
    act.sa_handler = SmartScheduleTimer;
    sigemptyset (&act.sa_mask);
    sigaddset (&act.sa_mask, SIGALRM);
    ret = sigaction(SIGALRM, &act, 0);
    return ret;
}

static int
SmartSchedulePause(void)
{
    int ret = 0;
    struct sigaction act;

    if (!SmartScheduleSignalEnable)
	return 0;

    #ifdef NX_TRANS_TEST
    fprintf(stderr, "SmartSchedulePause: Pausing the smart scheduler.\n");
    #endif

    memset((char *) &act, 0, sizeof(struct sigaction));

    act.sa_handler = SIG_IGN;
    sigemptyset(&act.sa_mask);
    ret = sigaction(SIGALRM, &act, 0);
    return ret;
}
#endif

void
SmartScheduleInit(void)
{
#if HAVE_SETITIMER
    #ifdef NX_TRANS_TEST
    fprintf(stderr, "SmartScheduleInit: Initializing the smart scheduler.\n");
    #endif

    if (SmartScheduleEnable() < 0) {
	perror("sigaction for smart scheduler");
	SmartScheduleSignalEnable = FALSE;
    }
#endif
}

#ifdef SIG_BLOCK
static sigset_t	PreviousSignalMask;
static int	BlockedSignalCount;
#endif

void
OsBlockSignals (void)
{
#ifdef SIG_BLOCK
    if (BlockedSignalCount++ == 0)
    {
	sigset_t    set;
	
	sigemptyset (&set);
#ifdef SIGALRM
	sigaddset (&set, SIGALRM);
#endif
#ifdef SIGVTALRM
	sigaddset (&set, SIGVTALRM);
#endif
#ifdef SIGWINCH
	sigaddset (&set, SIGWINCH);
#endif
#ifdef SIGIO
	sigaddset (&set, SIGIO);
#endif
#ifdef SIGTSTP
	sigaddset (&set, SIGTSTP);
#endif
#ifdef SIGTTIN
	sigaddset (&set, SIGTTIN);
#endif
#ifdef SIGTTOU
	sigaddset (&set, SIGTTOU);
#endif
#ifdef SIGCHLD
	sigaddset (&set, SIGCHLD);
#endif
	sigprocmask (SIG_BLOCK, &set, &PreviousSignalMask);
    }
#endif
}

void
OsReleaseSignals (void)
{
#ifdef SIG_BLOCK
    if (--BlockedSignalCount == 0)
    {
	sigprocmask (SIG_SETMASK, &PreviousSignalMask, 0);
    }
#endif
}

#if !defined(WIN32)
/*
 * "safer" versions of system(3), popen(3) and pclose(3) which give up
 * all privs before running a command.
 *
 * This is based on the code in FreeBSD 2.2 libc.
 *
 * XXX It'd be good to redirect stderr so that it ends up in the log file
 * as well.  As it is now, xkbcomp messages don't end up in the log file.
 */

int
System(char *command)
{
    int pid, p;
#ifdef SIGCHLD
    void (*csig)(int);
#endif
    int status;
    struct passwd *pwent;

    if (!command)
	return(1);

#ifdef SIGCHLD
    csig = OsSignal(SIGCHLD, SIG_DFL);
    if (csig == SIG_ERR) {
	perror("signal");
	return -1;
    }
#endif

#ifdef DEBUG
    ErrorF("System: `%s'\n", command);
#endif

#ifdef NX_TRANS_EXIT
    if (OsVendorStartRedirectErrorFProc != NULL) {
        OsVendorStartRedirectErrorFProc();
    }
#endif
    switch (pid = fork()) {
    case -1:	/* error */
	p = -1;
    case 0:	/* child */
	pwent = getpwuid(getuid());
	if (initgroups(pwent->pw_name,getgid()) == -1)
	    _exit(127);
	if (setgid(getgid()) == -1)
	    _exit(127);
	if (setuid(getuid()) == -1)
	    _exit(127);
	execl("/bin/sh", "sh", "-c", command, (char *)NULL);
	_exit(127);
    default:	/* parent */
	do {
	    p = waitpid(pid, &status, 0);
	} while (p == -1 && errno == EINTR);
	
    }
#ifdef NX_TRANS_EXIT
    if (OsVendorEndRedirectErrorFProc != NULL) {
        OsVendorEndRedirectErrorFProc();
    }
#endif

#ifdef SIGCHLD
    if (OsSignal(SIGCHLD, csig) == SIG_ERR) {
	perror("signal");
	return -1;
    }
#endif

    return p == -1 ? -1 : status;
}

static struct pid {
    struct pid *next;
    FILE *fp;
    int pid;
} *pidlist;

void *
Popen(char *command, char *type)
{
    struct pid *cur;
    FILE *iop;
    int pdes[2], pid;

    if (command == NULL || type == NULL)
	return NULL;

    if ((*type != 'r' && *type != 'w') || type[1])
	return NULL;

    if ((cur = (struct pid *)malloc(sizeof(struct pid))) == NULL)
	return NULL;

    if (pipe(pdes) < 0) {
	free(cur);
	return NULL;
    }

    /* Ignore the smart scheduler while this is going on */
#if HAVE_SETITIMER
    if (SmartSchedulePause() < 0) {
	close(pdes[0]);
	close(pdes[1]);
	free(cur);
	perror("signal");
	return NULL;
    }
#endif

#ifdef NX_TRANS_EXIT
    if (OsVendorStartRedirectErrorFProc != NULL) {
        OsVendorStartRedirectErrorFProc();
    }
    OsBlockSignals ();
#endif
    switch (pid = fork()) {
    case -1: 	/* error */
	close(pdes[0]);
	close(pdes[1]);
	free(cur);
#if HAVE_SETITIMER
	if (SmartScheduleEnable() < 0)
	    perror("signal");
#endif
#ifdef NX_TRANS_EXIT
	if (OsVendorEndRedirectErrorFProc != NULL) {
	    OsVendorEndRedirectErrorFProc();
	}
	OsReleaseSignals ();
#endif
	return NULL;
    case 0:	/* child */
	if (setgid(getgid()) == -1)
	    _exit(127);
	if (setuid(getuid()) == -1)
	    _exit(127);
	if (*type == 'r') {
	    if (pdes[1] != 1) {
		/* stdout */
		dup2(pdes[1], 1);
		close(pdes[1]);
	    }
	    close(pdes[0]);
	} else {
	    if (pdes[0] != 0) {
		/* stdin */
		dup2(pdes[0], 0);
		close(pdes[0]);
	    }
	    close(pdes[1]);
	}

        #ifdef NX_TRANS_SOCKET

        /*
         * Check if the child process should not
         * use the parent's libraries.
         */

        if (_NXUnsetLibraryPath)
        {
          #ifndef __sun

          unsetenv ("LD_LIBRARY_PATH");

          #else

          extern char **environ;

          char **ep = environ;

          ep = environ;

          while (*ep)
          {
            if (!strncmp("LD_LIBRARY_PATH=", *ep, strlen("LD_LIBRARY_PATH=")))
            {
              break;
            }

            *ep++;
          }

          while (*ep)
          {
            *ep = *(ep + 1);
            ep++;
          }

          #endif
        }

        #endif

        #ifdef NX_TRANS_EXIT
	OsReleaseSignals ();
        #endif

#if HAVE_SETITIMER
	if (SmartScheduleEnable() < 0) {
	    perror("signal");
	    return NULL;
	}
#endif

	execl("/bin/sh", "sh", "-c", command, (char *)NULL);
	_exit(127);
    }

#ifndef NX_TRANS_EXIT
    /* Avoid EINTR during stdio calls */
    OsBlockSignals ();
#endif
    
    /* parent */
    if (*type == 'r') {
	iop = fdopen(pdes[0], type);
	close(pdes[1]);
    } else {
	iop = fdopen(pdes[1], type);
	close(pdes[0]);
    }

    cur->fp = iop;
    cur->pid = pid;
    cur->next = pidlist;
    pidlist = cur;

#ifdef DEBUG
    ErrorF("Popen: `%s', fp = %p\n", command, iop);
#endif

    return iop;
}

/* fopen that drops privileges */
void *
Fopen(char *file, char *type)
{
    FILE *iop;
#ifndef HAS_SAVED_IDS_AND_SETEUID
    struct pid *cur;
    int pdes[2], pid;

    if (file == NULL || type == NULL)
	return NULL;

    if ((*type != 'r' && *type != 'w') || type[1])
	return NULL;

    if ((cur = (struct pid *)malloc(sizeof(struct pid))) == NULL)
	return NULL;

    if (pipe(pdes) < 0) {
	free(cur);
	return NULL;
    }

    switch (pid = fork()) {
    case -1: 	/* error */
	close(pdes[0]);
	close(pdes[1]);
	free(cur);
	return NULL;
    case 0:	/* child */
	if (setgid(getgid()) == -1)
	    _exit(127);
	if (setuid(getuid()) == -1)
	    _exit(127);
	if (*type == 'r') {
	    if (pdes[1] != 1) {
		/* stdout */
		dup2(pdes[1], 1);
		close(pdes[1]);
	    }
	    close(pdes[0]);
	} else {
	    if (pdes[0] != 0) {
		/* stdin */
		dup2(pdes[0], 0);
		close(pdes[0]);
	    }
	    close(pdes[1]);
	}
	execl("/bin/cat", "cat", file, (char *)NULL);
	_exit(127);
    }

    /* Avoid EINTR during stdio calls */
    OsBlockSignals ();
    
    /* parent */
    if (*type == 'r') {
	iop = fdopen(pdes[0], type);
	close(pdes[1]);
    } else {
	iop = fdopen(pdes[1], type);
	close(pdes[0]);
    }

    cur->fp = iop;
    cur->pid = pid;
    cur->next = pidlist;
    pidlist = cur;

#ifdef DEBUG
    ErrorF("Popen: `%s', fp = %p\n", command, iop);
#endif

    return iop;
#else
    int ruid, euid;

    ruid = getuid();
    euid = geteuid();
    
    if (seteuid(ruid) == -1) {
	    return NULL;
    }
    iop = fopen(file, type);

    if (seteuid(euid) == -1) {
	    fclose(iop);
	    return NULL;
    }
    return iop;
#endif /* HAS_SAVED_IDS_AND_SETEUID */
}

int
Pclose(void * iop)
{
    struct pid *cur, *last;
    int pstat;
    int pid;

#ifdef DEBUG
    ErrorF("Pclose: fp = %p\n", iop);
#endif

    fclose(iop);

    for (last = NULL, cur = pidlist; cur; last = cur, cur = cur->next)
	if (cur->fp == iop)
	    break;
    if (cur == NULL)
	return -1;

    do {
	pid = waitpid(cur->pid, &pstat, 0);
    } while (pid == -1 && errno == EINTR);

    if (last == NULL)
	pidlist = cur->next;
    else
	last->next = cur->next;
    free(cur);

    /* allow EINTR again */
    OsReleaseSignals ();
    
#ifdef NX_TRANS_EXIT
    if (OsVendorEndRedirectErrorFProc != NULL) {
        OsVendorEndRedirectErrorFProc();
    }
#endif
    return pid == -1 ? -1 : pstat;
}

int 
Fclose(void * iop)
{
#ifdef HAS_SAVED_IDS_AND_SETEUID
    return fclose(iop);
#else
    return Pclose(iop);
#endif
}

#endif /* !WIN32 */


/*
 * CheckUserParameters: check for long command line arguments and long
 * environment variables.  By default, these checks are only done when
 * the server's euid != ruid.  In 3.3.x, these checks were done in an
 * external wrapper utility.
 */

/* Consider LD* variables insecure? */
#ifndef REMOVE_ENV_LD
#define REMOVE_ENV_LD 1
#endif

/* Remove long environment variables? */
#ifndef REMOVE_LONG_ENV
#define REMOVE_LONG_ENV 1
#endif

/*
 * Disallow stdout or stderr as pipes?  It's possible to block the X server
 * when piping stdout+stderr to a pipe.
 *
 * Don't enable this because it looks like it's going to cause problems.
 */
#ifndef NO_OUTPUT_PIPES
#define NO_OUTPUT_PIPES 0
#endif


/* Check args and env only if running setuid (euid == 0 && euid != uid) ? */
#ifndef CHECK_EUID
#ifndef WIN32
#define CHECK_EUID 1
#else
#define CHECK_EUID 0
#endif
#endif

/*
 * Maybe the locale can be faked to make isprint(3) report that everything
 * is printable?  Avoid it by default.
 */
#ifndef USE_ISPRINT
#define USE_ISPRINT 0
#endif

#define MAX_ARG_LENGTH          128
#define MAX_ENV_LENGTH          256
#define MAX_ENV_PATH_LENGTH     2048	/* Limit for *PATH and TERMCAP */

#if USE_ISPRINT
#include <ctype.h>
#define checkPrintable(c) isprint(c)
#else
#define checkPrintable(c) (((c) & 0x7f) >= 0x20 && ((c) & 0x7f) != 0x7f)
#endif

enum BadCode {
    NotBad = 0,
    UnsafeArg,
    ArgTooLong,
    UnprintableArg,
    EnvTooLong,
    OutputIsPipe,
    InternalError
};

#if defined(VENDORSUPPORT)
#define BUGADDRESS VENDORSUPPORT
#elif defined(BUILDERADDR)
#define BUGADDRESS BUILDERADDR
#else
#define BUGADDRESS "xorg@freedesktop.org"
#endif

#define ARGMSG \
    "\nIf the arguments used are valid, and have been rejected incorrectly\n" \
      "please send details of the arguments and why they are valid to\n" \
      "%s.  In the meantime, you can start the Xserver as\n" \
      "the \"super user\" (root).\n"   

#define ENVMSG \
    "\nIf the environment is valid, and have been rejected incorrectly\n" \
      "please send details of the environment and why it is valid to\n" \
      "%s.  In the meantime, you can start the Xserver as\n" \
      "the \"super user\" (root).\n"

void
CheckUserParameters(int argc, char **argv, char **envp)
{
    enum BadCode bad = NotBad;
    int i = 0, j;
    char *a, *e = NULL;

#if CHECK_EUID
    if (geteuid() == 0 && getuid() != geteuid())
#endif
    {
	/* Check each argv[] */
	for (i = 1; i < argc; i++) {
	    if (strcmp(argv[i], "-fp") == 0)
	    {
		i++; /* continue with next argument. skip the length check */
		if (i >= argc)
		    break;
	    } else
	    {
		if (strlen(argv[i]) > MAX_ARG_LENGTH) {
		    bad = ArgTooLong;
		    break;
		}
	    }
	    a = argv[i];
	    while (*a) {
		if (checkPrintable(*a) == 0) {
		    bad = UnprintableArg;
		    break;
		}
		a++;
	    }
	    if (bad)
		break;
	}
	if (!bad) {
	    /* Check each envp[] */
	    for (i = 0; envp[i]; i++) {

		/* Check for bad environment variables and values */
#if REMOVE_ENV_LD
		while (envp[i] && (strncmp(envp[i], "LD", 2) == 0)) {
#ifdef ENVDEBUG
		    ErrorF("CheckUserParameters: removing %s from the "
			   "environment\n", strtok(envp[i], "="));
#endif
		    for (j = i; envp[j]; j++) {
			envp[j] = envp[j+1];
		    }
		}
#endif   
		if (envp[i] && (strlen(envp[i]) > MAX_ENV_LENGTH)) {
#if REMOVE_LONG_ENV
#ifdef ENVDEBUG
		    ErrorF("CheckUserParameters: removing %s from the "
			   "environment\n", strtok(envp[i], "="));
#endif
		    for (j = i; envp[j]; j++) {
			envp[j] = envp[j+1];
		    }
		    i--;
#else
		    char *eq;
		    int len;

		    eq = strchr(envp[i], '=');
		    if (!eq)
			continue;
		    len = eq - envp[i];
		    e = malloc(len + 1);
		    if (!e) {
			bad = InternalError;
			break;
		    }
		    strncpy(e, envp[i], len);
		    e[len] = 0;
		    if (len >= 4 &&
			(strcmp(e + len - 4, "PATH") == 0 ||
			 strcmp(e, "TERMCAP") == 0)) {
			if (strlen(envp[i]) > MAX_ENV_PATH_LENGTH) {
			    bad = EnvTooLong;
			    break;
			} else {
			    free(e);
			}
		    } else {
			bad = EnvTooLong;
			break;
		    }
#endif
		}
	    }
	}
#if NO_OUTPUT_PIPES
	if (!bad) {
	    struct stat buf;

	    if (fstat(fileno(stdout), &buf) == 0 && S_ISFIFO(buf.st_mode))
		bad = OutputIsPipe;
	    if (fstat(fileno(stderr), &buf) == 0 && S_ISFIFO(buf.st_mode))
		bad = OutputIsPipe;
	}
#endif
    }
    switch (bad) {
    case NotBad:
	return;
    case UnsafeArg:
	ErrorF("Command line argument number %d is unsafe\n", i);
	ErrorF(ARGMSG, BUGADDRESS);
	break;
    case ArgTooLong:
	ErrorF("Command line argument number %d is too long\n", i);
	ErrorF(ARGMSG, BUGADDRESS);
	break;
    case UnprintableArg:
	ErrorF("Command line argument number %d contains unprintable"
		" characters\n", i);
	ErrorF(ARGMSG, BUGADDRESS);
	break;
    case EnvTooLong:
	ErrorF("Environment variable `%s' is too long\n", e);
	ErrorF(ENVMSG, BUGADDRESS);
	break;
    case OutputIsPipe:
	ErrorF("Stdout and/or stderr is a pipe\n");
	break;
    case InternalError:
	ErrorF("Internal Error\n");
	break;
    default:
	ErrorF("Unknown error\n");
	ErrorF(ARGMSG, BUGADDRESS);
	ErrorF(ENVMSG, BUGADDRESS);
	break;
    }
    FatalError("X server aborted because of unsafe environment\n");
}

/*
 * CheckUserAuthorization: check if the user is allowed to start the
 * X server.  This usually means some sort of PAM checking, and it is
 * usually only done for setuid servers (uid != euid).
 */

#ifdef USE_PAM
#include <security/pam_appl.h>
#include <security/pam_misc.h>
#include <pwd.h>
#endif /* USE_PAM */

void
CheckUserAuthorization(void)
{
#ifdef USE_PAM
    static struct pam_conv conv = {
	misc_conv,
	NULL
    };

    pam_handle_t *pamh = NULL;
    struct passwd *pw;
    int retval;

    if (getuid() != geteuid()) {
	pw = getpwuid(getuid());
	if (pw == NULL)
	    FatalError("getpwuid() failed for uid %d\n", getuid());

	retval = pam_start("xserver", pw->pw_name, &conv, &pamh);
	if (retval != PAM_SUCCESS)
	    FatalError("pam_start() failed.\n"
			"\tMissing or mangled PAM config file or module?\n");

	retval = pam_authenticate(pamh, 0);
	if (retval != PAM_SUCCESS) {
	    pam_end(pamh, retval);
	    FatalError("PAM authentication failed, cannot start X server.\n"
			"\tPerhaps you do not have console ownership?\n");
	}

	retval = pam_acct_mgmt(pamh, 0);
	if (retval != PAM_SUCCESS) {
	    pam_end(pamh, retval);
	    FatalError("PAM authentication failed, cannot start X server.\n"
			"\tPerhaps you do not have console ownership?\n");
	}

	/* this is not a session, so do not do session management */
	pam_end(pamh, PAM_SUCCESS);
    }
#endif
}

#ifdef __SCO__
#include <fcntl.h>

static void
lockit (int fd, short what)
{
  struct flock lck;

  lck.l_whence = 0;
  lck.l_start = 0;
  lck.l_len = 1;
  lck.l_type = what;

  (void)fcntl (fd, F_SETLKW, &lck);
}

/* SCO OpenServer 5 lacks pread/pwrite. Emulate them. */
ssize_t
pread (int fd, void *buf, size_t nbytes, off_t offset)
{
  off_t saved;
  ssize_t ret;

  lockit (fd, F_RDLCK);
  saved = lseek (fd, 0, SEEK_CUR);
  lseek (fd, offset, SEEK_SET);
  ret = read (fd, buf, nbytes);
  lseek (fd, saved, SEEK_SET);
  lockit (fd, F_UNLCK);

  return ret;
}

ssize_t
pwrite (int fd, const void *buf, size_t nbytes, off_t offset)
{
  off_t saved;
  ssize_t ret;

  lockit (fd, F_WRLCK);
  saved = lseek (fd, 0, SEEK_CUR);
  lseek (fd, offset, SEEK_SET);
  ret = write (fd, buf, nbytes);
  lseek (fd, saved, SEEK_SET);
  lockit (fd, F_UNLCK);

  return ret;
}
#endif /* __SCO__ */
