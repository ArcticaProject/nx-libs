/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2011 NoMachine (http://www.nomachine.com)          */
/* Copyright (c) 2008-2017 Oleksandr Shneyder <o.shneyder@phoca-gmbh.de>  */
/* Copyright (c) 2011-2022 Mike Gabriel <mike.gabriel@das-netzwerkteam.de>*/
/* Copyright (c) 2014-2019 Mihai Moldovan <ionic@ionic.de>                */
/* Copyright (c) 2014-2022 Ulrich Sibiller <uli42@gmx.de>                 */
/* Copyright (c) 2015-2016 Qindel Group (http://www.qindel.com)           */
/*                                                                        */
/* NXAGENT, NX protocol compression and NX extensions to this software    */
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

Copyright 1993 by Davor Matic

Permission to use, copy, modify, distribute, and sell this software
and its documentation for any purpose is hereby granted without fee,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation.  Davor Matic makes no representations about
the suitability of this software for any purpose.  It is provided "as
is" without express or implied warranty.

*/

#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <stdbool.h>

#include <nx-X11/X.h>
#include <nx-X11/Xproto.h>
#include "screenint.h"
#include "input.h"
#include "misc.h"
#include "scrnintstr.h"
#include "servermd.h"
#include "windowstr.h"
#include "dixstruct.h"

#ifdef WATCH
#include "unistd.h"
#endif

#include <nx/NXalert.h>

#include "Agent.h"
#include "Display.h"
#include "Visual.h"
#include "Options.h"
#include "Error.h"
#include "Init.h"
#include "Args.h"
#include "Image.h"
#include "Utils.h"

#define Pixmap XlibPixmap
#include "Icons.h"
#undef Pixmap

#include "Render.h"
#include "Font.h"
#include "Reconnect.h"
#include "Events.h"
#include "Dialog.h"
#include "Client.h"
#include "Splash.h"
#include "Screen.h"
#include "Handlers.h"
#include "Split.h"

#include <nx/NX.h>
#include "compext/Compext.h"

#include NXAGENT_ICON_NAME
#ifdef X2GO
#include X2GOAGENT_ICON_NAME
#endif

/*
 * Set here the required log level.
 */

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG
#undef  WATCH

Display *nxagentDisplay = NULL;
XVisualInfo *nxagentVisuals = NULL;
Bool nxagentTrue24 = False;

int nxagentNumVisuals;
int nxagentXConnectionNumber;

int nxagentIOErrorHandler(Display *disp);

static Bool nxagentDisplayInfoSaved = False;
static Display *nxagentDisplayBackup = NULL;
static XlibGC nxagentBitmapGCBackup = NULL;
static XVisualInfo *nxagentVisualsRecBackup;
static int nxagentNumVisualsRecBackup;
static int nxagentNumDefaultColormapsRecBackup;
static int *nxagentDepthsRecBackup;
static int nxagentNumDepthsRecBackup;
static int nxagentDefaultDepthRecBackup;
static int nxagentDisplayWidthRecBackup;
static int nxagentDisplayHeightRecBackup;
static Bool nxagentRenderEnableRecBackup;
static Bool *nxagentVisualHasBeenIgnored;

static enum
{
  NOTHING = 0,
  OPENED,
  GOT_VISUAL_INFO,
  ALLOC_DEF_COLORMAP,
  GOT_DEPTH_LIST,
  GOT_PIXMAP_FORMAT_LIST,
  EVERYTHING_DONE
} reconnectDisplayState;

int nxagentDefaultVisualIndex;
Colormap *nxagentDefaultColormaps = NULL;
int nxagentNumDefaultColormaps;
int *nxagentDepths = NULL;
int nxagentNumDepths;
XPixmapFormatValues *nxagentPixmapFormats = NULL;
XPixmapFormatValues *nxagentRemotePixmapFormats = NULL;
int nxagentNumPixmapFormats;
int nxagentRemoteNumPixmapFormats;
Pixel nxagentBlackPixel;
Pixel nxagentWhitePixel;
Drawable nxagentDefaultDrawables[MAXDEPTH + 1];
Pixmap nxagentScreenSaverPixmap;

/*
 * Also used in Cursor.c. There are huge problems using GC
 * definition. This is to be reworked.
 */

XlibGC nxagentBitmapGC;

#ifdef NX_CONFINE_WINDOW
/*
 * The "confine" window is used in the nxagentConstrainCursor
 * procedure. We are currently overriding the original Xnest
 * behaviour. It is unclear what this window is used for.
 */

Window nxagentConfineWindow;
#endif

Pixmap nxagentIconPixmap;
Pixmap nxagentIconShape;
Bool useXpmIcon = False;

Bool nxagentMakeIcon(Display *disp, Pixmap *nxIcon, Pixmap *nxMask);


static void nxagentInitVisuals(void);
static void nxagentSetDefaultVisual(void);
static void nxagentInitDepths(void);
static void nxagentInitPixmapFormats(void);

static int nxagentCheckForDefaultDepthCompatibility(void);
static int nxagentCheckForDepthsCompatibility(void);
static int nxagentCheckForPixmapFormatsCompatibility(void);
static int nxagentInitAndCheckVisuals(int flexibility);
static int nxagentCheckForColormapsCompatibility(int flexibility);

/*
 * Save Internal implementation Also called in Reconnect.c.
 */

Display *nxagentInternalOpenDisplay(char *disp);

#ifdef NXAGENT_TIMESTAMP
unsigned long startTime;
#endif

/*
 * This is located in connection.c.
 */

extern void RejectWellKnownSockets(void);
extern Bool nxagentReportWindowIds;

int nxagentServerOrder(void)
{
  int whichbyte = 1;

  if (*((char *) &whichbyte))
      return LSBFirst;

  return MSBFirst;
}

/*
 * FIXME: This error handler is not printing anything in the session
 * log. This is OK once the session is started, because the error is
 * handled by the other layers, but not before that point, as the
 * agent would die without giving any feedback to the user (or, worse,
 * to the NX server). We should check how many requests have been
 * handled for this display and print a message if the display dies
 * before the session is up and running.
 */

/*
 * FIXME: This should be moved to Error.c, The other handlers should
 * be probably moved to Handlers.c.
 */

int nxagentIOErrorHandler(Display *disp)
{
  #ifdef TEST
  fprintf(stderr, "nxagentIOErrorHandler: Got I/O error with nxagentException.ioError [%d].\n",
              nxagentException.ioError);
  #endif

  nxagentException.ioError++;

  #ifdef TEST
  fprintf(stderr, "nxagentIOErrorHandler: Set nxagentException.ioError to [%d].\n",
              nxagentException.ioError);
  #endif

  return 1;
}

/*
 * Force a shutdown of any connection attempt while connecting to the
 * remote display.  This is needed to avoid a hang up in case of
 * loopback connections to our own listening sockets.
 */

static void nxagentRejectConnection(int signal)
{
  #ifdef TEST
  fprintf(stderr, "nxagentRejectConnection: Going to reject client connections.\n");
  #endif

  RejectWellKnownSockets();

  #ifdef TEST
  fprintf(stderr, "nxagentRejectConnection: Setting new alarm to 5 seconds from now.\n");
  #endif

  /*
   * A further timeout is unlikely to happen in the case of loopback
   * connections.
   */

  alarm(5);
}

/*
 * Ignore the signal if the NX transport is not running.
 */

static void nxagentSigusrHandler(int signal)
{
  #ifdef TEST
  fprintf(stderr, "nxagentSigusrHandler: Nothing to do with signal [%d].\n",
              signal);
  #endif
}

static void nxagentSighupHandler(int signal)
{
  #ifdef TEST
  fprintf(stderr, "nxagentSighupHandler: Handling signal with state [%s] transport [%d] server "
              "generation [%ld].\n", DECODE_SESSION_STATE(nxagentSessionState),
                  NXTransRunning(NX_FD_ANY), serverGeneration);
  #endif

  if (signal != SIGHUP)
  {
    #ifdef PANIC
    fprintf(stderr, "nxagentSighupHandler: PANIC! Invalid signal [%d] received in state [%s].\n",
                signal, DECODE_SESSION_STATE(nxagentSessionState));
    #endif

    return;
  }

  if (dispatchException & DE_TERMINATE)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentSighupHandler: Ignoring the signal while terminating the session.\n");
    #endif

    return;
  }
  else if (nxagentSessionState == SESSION_UP)
  {
    if (nxagentOption(Persistent))
    {
      #ifdef TEST
      fprintf(stderr, "nxagentSighupHandler: Handling the signal by disconnecting the agent.\n");
      #endif

      nxagentException.sigHup++;
    }
    else
    {
      #ifdef TEST
      fprintf(stderr, "nxagentSighupHandler: Ignoring the signal with persistency disabled.\n");
      #endif
    }

    return;
  }
  else if (nxagentSessionState == SESSION_STARTING)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentSighupHandler: Handling the signal by aborting the session.\n");
    #endif

    nxagentException.sigHup++;

    return;
  }
  else if (nxagentSessionState == SESSION_DOWN)
  {
    if (NXTransRunning(NX_FD_ANY) == 1)
    {
      #ifdef TEST
      fprintf(stderr, "nxagentSighupHandler: Handling the signal by aborting the reconnection.\n");
      #endif
    }
    else
    {
      #ifdef TEST
      fprintf(stderr, "nxagentSighupHandler: Handling the signal by resuming the session.\n");
      #endif
    }

    nxagentException.sigHup++;

    return;
  }

  #ifdef WARNING
  fprintf(stderr, "nxagentSighupHandler: WARNING! Ignoring the signal in state [%s].\n",
              DECODE_SESSION_STATE(nxagentSessionState));
  #endif
}

static void nxagentSigchldHandler(int signal)
{
  int pid = 0;
  int status;

  #ifdef TEST
  fprintf(stderr, "nxagentSigchldHandler: Going to check the children processes.\n");
  #endif

  int options = WNOHANG | WUNTRACED;

  /*
   * Try with the pid of the dialog process.  Leave the other children
   * unaffected.
   */

  if (nxagentRootlessDialogPid)
  {
    pid = waitpid(nxagentRootlessDialogPid, &status, options);

    if (pid == -1 && errno == ECHILD)
    {
      #ifdef WARNING
      fprintf(stderr, "nxagentSigchldHandler: Got ECHILD waiting for child %d (Rootless dialog).\n",
                  nxagentRootlessDialogPid);
      #endif

      pid = nxagentRootlessDialogPid = 0;
    }
  }

  if (pid == 0 && nxagentPulldownDialogPid)
  {
    pid = waitpid(nxagentPulldownDialogPid, &status, options);

    if (pid == -1 && errno == ECHILD)
    {
      #ifdef WARNING
      fprintf(stderr, "nxagentSigchldHandler: Got ECHILD waiting for child %d (Pulldown dialog).\n",
                  nxagentPulldownDialogPid);
      #endif

      pid = nxagentPulldownDialogPid = 0;
    }
  }

  if (pid == 0 && nxagentKillDialogPid)
  {
    pid = waitpid(nxagentKillDialogPid, &status, options);

    if (pid == -1 && errno == ECHILD)
    {
      #ifdef WARNING
      fprintf(stderr, "nxagentSigchldHandler: Got ECHILD waiting for child %d (Kill dialog).\n",
                  nxagentKillDialogPid);
      #endif

      pid = nxagentKillDialogPid = 0;
    }
  }

  if (pid == 0 && nxagentSuspendDialogPid)
  {
    pid = waitpid(nxagentSuspendDialogPid, &status, options);

    if (pid == -1 && errno == ECHILD)
    {
      #ifdef WARNING
      fprintf(stderr, "nxagentSigchldHandler: Got ECHILD waiting for child %d (Suspend dialog).\n",
                  nxagentSuspendDialogPid);
      #endif

      pid = nxagentSuspendDialogPid = 0;
    }
  }

  if (pid == 0 && nxagentFontsReplacementDialogPid)
  {
    pid = waitpid(nxagentFontsReplacementDialogPid, &status, options);

    if (pid == -1 && errno == ECHILD)
    {
      #ifdef WARNING
      fprintf(stderr, "nxagentSigchldHandler: Got ECHILD waiting for child %d (Fonts replacement).\n",
                  nxagentFontsReplacementDialogPid);
      #endif

      pid = nxagentFontsReplacementDialogPid = 0;
    }
  }

  if (pid == 0 && nxagentEnableRandRModeDialogPid)
  {
    pid = waitpid(nxagentEnableRandRModeDialogPid, &status, options);

    if (pid == -1 && errno == ECHILD)
    {
      #ifdef WARNING
      fprintf(stderr, "nxagentSigchldHandler: Got ECHILD waiting for child %d (EnableRandRMode dialog).\n",
                  nxagentEnableRandRModeDialogPid);
      #endif

      pid = nxagentEnableRandRModeDialogPid = 0;
    }
  }

  if (pid == 0 && nxagentDisableRandRModeDialogPid)
  {
    pid = waitpid(nxagentDisableRandRModeDialogPid, &status, options);

    if (pid == -1 && errno == ECHILD)
    {
      #ifdef WARNING
      fprintf(stderr, "nxagentSigchldHandler: Got ECHILD waiting for child %d (DisableRandRMode dialog).\n",
                  nxagentDisableRandRModeDialogPid);
      #endif

      pid = nxagentDisableRandRModeDialogPid = 0;
    }
  }

  if (pid == 0 && nxagentEnableDeferModePid)
  {
    pid = waitpid(nxagentEnableDeferModePid, &status, options);

    if (pid == -1 && errno == ECHILD)
    {
      #ifdef WARNING
      fprintf(stderr, "nxagentSigchldHandler: Got ECHILD waiting for child %d (EnableDeferMode dialog).\n",
                  nxagentEnableDeferModePid);
      #endif

      pid = nxagentEnableDeferModePid = 0;
    }
  }

  if (pid == 0 && nxagentDisableDeferModePid)
  {
    pid = waitpid(nxagentDisableDeferModePid, &status, options);

    if (pid == -1 && errno == ECHILD)
    {
      #ifdef WARNING
      fprintf(stderr, "nxagentSigchldHandler: Got ECHILD waiting for child %d (DisableDeferMode dialog).\n",
                  nxagentDisableDeferModePid);
      #endif

      pid = nxagentDisableDeferModePid = 0;
    }
  }

  if (pid == -1)
  {
    FatalError("Got error '%s' waiting for the child.\n", strerror(errno));
  }

  if (pid > 0)
  {
    if (WIFSTOPPED(status))
    {
      #ifdef WARNING
      fprintf(stderr, "nxagentSigchldHandler: Child process [%d] was stopped "
                  "with signal [%d].\n", pid, (WSTOPSIG(status)));
      #endif
    }
    else
    {
      #ifdef TEST

      if (WIFEXITED(status))
      {
        fprintf(stderr, "nxagentSigchldHandler: Child process [%d] exited "
                    "with status [%d].\n", pid, (WEXITSTATUS(status)));
      }
      else if (WIFSIGNALED(status))
      {
        fprintf(stderr, "nxagentSigchldHandler: Child process [%d] died "
                    "because of signal [%d].\n", pid, (WTERMSIG(status)));
      }

      #endif

      nxagentResetDialog(pid);
    }
  }
  else if (pid == 0)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentSigchldHandler: Forwarding the signal to the NX transport.\n");
    #endif

    NXTransSignal(SIGCHLD, NX_SIGNAL_RAISE);
  }

  return;
}

Display *nxagentInternalOpenDisplay(char *disp)
{
  /*
   * Stop the smart schedule timer since it uses SIGALRM as we do.
   */

  nxagentStopTimer();

  /*
   * Install the handler rejecting a possible loopback connection.
   */
/*
FIXME: Should print a warning if the user tries to let the agent
       impersonate the same display as the display where the agent is
       supposed to connect.  We actually handle this by means of
       RejectWellKnownSockets() but without giving a friendly
       explanation for the error to the user.
*/

  struct sigaction newAction = {
    .sa_handler = nxagentRejectConnection
  };

  sigfillset(&newAction.sa_mask);

  newAction.sa_flags = 0;

  int result;
  struct sigaction oldAction;

  while (((result = sigaction(SIGALRM, &newAction,
             &oldAction)) == -1) && (errno == EINTR));

  if (result == -1)
  {
    FatalError("Can't set alarm for rejecting connections.");
  }

  alarm(10);

  #ifdef TEST
  fprintf(stderr, "nxagentInternalOpenDisplay: Going to open the display [%s].\n",
              disp);
  #endif

  Display *newDisplay = XOpenDisplay(disp);

  alarm(0);

  while (((result = sigaction(SIGALRM, &oldAction,
             NULL)) == -1) && (errno == EINTR));

  if (result == -1)
  {
    FatalError("Can't restore alarm for rejecting connections.");
  }

  #ifdef TEST
  fprintf(stderr, "nxagentInternalOpenDisplay: Setting the NX flush policy to immediate.\n");
  #endif

  NXSetDisplayPolicy(nxagentDisplay, NXPolicyImmediate);

  #ifdef TEST
  fprintf(stderr, "nxagentInternalOpenDisplay: Function returned display at [%p].\n",
              (void *) newDisplay);
  #endif

  return newDisplay;
}

static void nxagentDisplayBlockHandler(Display *disp, int reason)
{
  if (nxagentDisplay != NULL)
  {
    /*
     * Don't allow the smart schedule to interrupt the agent while
     * waiting for the remote display.
     */

    #ifdef DEBUG
    fprintf(stderr, "nxagentDisplayBlockHandler: BLOCK! Stopping the smart schedule timer.\n");
    #endif

    nxagentStopTimer();

    if (reason == NXBlockRead)
    {
      #ifdef DEBUG
      fprintf(stderr, "nxagentDisplayBlockHandler: BLOCK! Display is blocking for [read].\n");
      #endif
    }
    else
    {
      #ifdef DEBUG
      fprintf(stderr, "nxagentDisplayBlockHandler: BLOCK! Display is blocking for [write].\n");
      #endif

      nxagentBlocking = True;

      if (!SmartScheduleSignalEnable)
      {

        /*
         * Let the dispatch attend the next client.
         */

        #ifdef DEBUG
        fprintf(stderr, "nxagentDisplayBlockHandler: BLOCK! Yielding with agent blocked.\n");
        #endif

        nxagentDispatch.start = GetTimeInMillis();

        nxagentDispatch.in  = nxagentBytesIn;
        nxagentDispatch.out = nxagentBytesOut;

      }

      /*
       * Give a chance to the next client.
       */

      isItTimeToYield = 1;
    }
  }
}

static void nxagentDisplayWriteHandler(Display *disp, int length)
{
  if (nxagentDisplay != NULL)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentDisplayWriteHandler: WRITE! Called with [%d] bytes written.\n",
                length);
    #endif

    /*
     * Notify the dispatch handler.
     */

    nxagentDispatchHandler(NULL, 0, length);

    if (nxagentOption(LinkType) == LINK_TYPE_NONE)
    {
      nxagentFlush = GetTimeInMillis();
    }
  }
}

static CARD32 nxagentRateTime = 5000;
static CARD32 nxagentLastTime;
static unsigned int nxagentRate = 0;

int nxagentGetDataRate(void)
{
  return nxagentRate;
}

static void nxagentDisplayFlushHandler(Display *disp, int length)
{
  if (nxagentDisplay != NULL)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentDisplayFlushHandler: FLUSH! Called with [%d] bytes flushed.\n",
                length);
    #endif

    nxagentCongestion = NXDisplayCongestion(nxagentDisplay);

    #ifdef TEST
    fprintf(stderr, "nxagentDisplayFlushHandler: FLUSH! Current congestion level is [%d].\n",
                nxagentCongestion);
    #endif

    if (nxagentOption(LinkType) != LINK_TYPE_NONE)
    {
      nxagentFlush = GetTimeInMillis();

      CARD32 time = nxagentFlush - nxagentLastTime;

      if (time < nxagentRateTime)
      {
        nxagentRate = ((nxagentRate * (nxagentRateTime - time) +
                          length) * 1000) / nxagentRateTime;
      }
      else
      {
        nxagentRate = (length * 1000) / nxagentRateTime;
      }

      nxagentLastTime = nxagentFlush;
    }
  }
}

/*
 * From the changelog for nx-X11-3.0.0-4: "Added the
 * _NXDisplayErrorPredicate function in XlibInt.c. It is actually a
 * pointer to a function called whenever Xlib is going to perform a
 * network operation. If the function returns true, the call will be
 * aborted and Xlib will return the control to the application. It is
 * up to the application to set the XlibDisplayIO- Error flag after
 * the _NXDisplayErrorPredicate returns true. The function can be used
 * to activate additional checks, besides the normal failures detected
 * by Xlib on the display socket. For example, the application can set
 * the function to verify if an interrupt was received or if any other
 * event occurred mandating the end of the session."
 */
static int nxagentDisplayErrorPredicate(Display *disp, int error)
{
  #ifdef TEST
  fprintf(stderr, "nxagentDisplayErrorPredicate: CHECK! Error is [%d] with [%d][%d][%d][%d][%d].\n",
              ((error == 1) || (dispatchException & DE_RESET) != 0 ||
                  (dispatchException & DE_TERMINATE) != 0 || nxagentException.sigHup > 0 ||
                      nxagentException.ioError > 0), error, (dispatchException & DE_RESET) != 0,
                          (dispatchException & DE_TERMINATE), nxagentException.sigHup > 0,
                              nxagentException.ioError > 0);
  #endif

  if (error == 0)
  {
    if ((dispatchException & DE_RESET) != 0 ||
            (dispatchException & DE_TERMINATE))
    {
      return 1;
    }
    else if (nxagentException.sigHup > 0 ||
                 nxagentException.ioError > 0)
    {
      NXForceDisplayError(disp);

      return 1;
    }
  }

  return error;
}

void nxagentInstallDisplayHandlers(void)
{
  /*
   * If the display was already opened, be sure all structures are
   * freed.
   */

  nxagentResetDisplayHandlers();

  /*
   * We want the Xlib I/O error handler to return, instead of quitting
   * the application. Using setjmp()/longjmp() leaves the door open to
   * unexpected bugs when dealing with interaction with the other X
   * server layers.
   */

  NXHandleDisplayError(1);

  NXSetDisplayBlockHandler(nxagentDisplayBlockHandler);

  NXSetDisplayWriteHandler(nxagentDisplayWriteHandler);

  NXSetDisplayFlushHandler(nxagentDisplayFlushHandler, NULL);

  /*
   * Override the default Xlib error handler.
   */

  XSetIOErrorHandler(nxagentIOErrorHandler);

  /*
   * Let Xlib become aware of our interrupts. In theory we don't need
   * to have the error handler installed during the normal operations
   * and could simply let the dispatcher handle the interrupts. In
   * practice it's better to have Xlib invalidating the display as
   * soon as possible rather than incurring in the risk of entering a
   * loop that doesn't care checking the display errors explicitly.
   */

  #ifdef TEST
  fprintf(stderr, "nxagentInstallDisplayHandlers: Installing the error function predicate.\n");
  #endif

  NXSetDisplayErrorPredicate(nxagentDisplayErrorPredicate);
}

void nxagentPostInstallDisplayHandlers(void)
{
  /*
   * This is executed after having opened the display, once we know
   * the display address.
   */

  if (nxagentDisplay != NULL)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentPostInstallDisplayHandlers: Initializing the NX display internals.\n");
    #endif

    NXInitDisplay(nxagentDisplay);
/*
FIXME: What is the most appropriate number of elements?

    NXInitCache(nxagentDisplay, 128);
*/
    NXInitCache(nxagentDisplay, 256);

    NXSetDisplayFlushHandler(nxagentDisplayFlushHandler, nxagentDisplay);
  }

  /*
   * Handler for the Xlib protocol errors.
   */

  XSetErrorHandler(nxagentErrorHandler);
}

void nxagentResetDisplayHandlers(void)
{
  if (nxagentDisplay != NULL)
  {
    /*
     * Free the internal nxcompext structures.
     */

    NXResetDisplay(nxagentDisplay);

    /*
     * Remove the display descriptor from the listened sockets.
     */

    nxagentRemoveXConnection();

    /*
     * Restart the suspended clients.
     */

    nxagentWakeupByReconnect();

    nxagentReleaseAllSplits();
  }

  /*
   * Reset the display to a healty state.
   */

  nxagentBuffer     = 0;
  nxagentBlocking   = False;
  nxagentCongestion = 0;

  /*
   * Reset the counter of synchronization requests pending.
   */

  nxagentTokens.soft    = 0;
  nxagentTokens.hard    = 0;
  nxagentTokens.pending = 0;

  /*
   * Reset the current dispatch information.
   */

  nxagentDispatch.client = UNDEFINED;
  nxagentDispatch.in     = 0;
  nxagentDispatch.out    = 0;
  nxagentDispatch.start  = 0;
}

void nxagentInstallSignalHandlers(void)
{
  #ifdef TEST
  fprintf(stderr, "nxagentInstallSignalHandlers: Installing the agent signal handlers.\n");
  #endif

  /*
   * Keep the default X server's handlers for SIGINT and SIGTERM and
   * restore the other signals of interest to our defaults.
   */

  struct sigaction newAction;

  /*
   * By default nxcomp installs its signal handlers.  We need to
   * ensure that SIGUSR1 and SIGUSR2 are ignored if the NX transport
   * is not running.
   */

  newAction.sa_handler = nxagentSigusrHandler;

  sigfillset(&newAction.sa_mask);

  newAction.sa_flags = 0;

  int result;

  while (((result = sigaction(SIGUSR1, &newAction,
             NULL)) == -1) && (errno == EINTR));

  if (result == -1)
  {
    FatalError("Can't set the handler for user signal 1.");
  }

  while (((result = sigaction(SIGUSR2, &newAction,
             NULL)) == -1) && (errno == EINTR));

  if (result == -1)
  {
    FatalError("Can't set the handler for user signal 2.");
  }

  /*
   * Reset the SIGALRM to the default.
   */

  nxagentStopTimer();

  newAction.sa_handler = SIG_DFL;

  sigfillset(&newAction.sa_mask);

  while (((result = sigaction(SIGALRM, &newAction,
             NULL)) == -1) && (errno == EINTR));

  if (result == -1)
  {
    FatalError("Can't set the handler for alarm signal.");
  }

  /*
   * Let the smart schedule set the SIGALRM handler again.
   */

  nxagentInitTimer();

  /*
   * Install our own handler for the SIGHUP.
   */

  newAction.sa_handler = nxagentSighupHandler;

  sigfillset(&newAction.sa_mask);

  newAction.sa_flags = 0;

  while (((result = sigaction(SIGHUP, &newAction,
             NULL)) == -1) && (errno == EINTR));

  if (result == -1)
  {
    FatalError("Can't set the handler for session suspend.");
  }

  /*
   * We need to be notified about our children.
   */

  newAction.sa_handler = nxagentSigchldHandler;

  sigfillset(&newAction.sa_mask);

  newAction.sa_flags = 0;

  while (((result = sigaction(SIGCHLD, &newAction,
             NULL)) == -1) && (errno == EINTR));

  if (result == -1)
  {
    FatalError("Can't set the handler for children.");
  }
}

void nxagentPostInstallSignalHandlers(void)
{
  #ifdef TEST
  fprintf(stderr, "nxagentPostInstallSignalHandlers: Dealing with the proxy signal handlers.\n");
  #endif

  /*
   * Reconfigure our signal handlers to work well with the NX
   * transport.
   *
   * Let our handlers manage the SIGINT and SIGTERM.  The following
   * calls will tell the NX transport to restore the old handlers
   * (those originally installed by us or the X server).
   */

  NXTransSignal(SIGINT,  NX_SIGNAL_DISABLE);
  NXTransSignal(SIGTERM, NX_SIGNAL_DISABLE);

  /*
   * Also tell the proxy to ignore the SIGHUP.
   */

  NXTransSignal(SIGHUP, NX_SIGNAL_DISABLE);

  /*
   * Both the proxy and the agent need to catch their children, so
   * we'll have to send the signal to transport.
   */

  NXTransSignal(SIGCHLD, NX_SIGNAL_DISABLE);

  /*
   * Let the NX transport take care of SIGUSR1 and SIGUSR2.
   */
}

void nxagentResetSignalHandlers(void)
{
  /*
   * Reset the signal handlers to a well known state.
   */

  #ifdef TEST
  fprintf(stderr, "nxagentResetSignalHandlers: Resetting the agent the signal handlers.\n");
  #endif

  /*
   * Reset the SIGALRM to the default.
   */

  nxagentStopTimer();

  struct sigaction newAction = {
    .sa_handler = SIG_DFL
  };

  sigfillset(&newAction.sa_mask);

  int result;
  while (((result = sigaction(SIGALRM, &newAction,
             NULL)) == -1) && (errno == EINTR));

  if (result == -1)
  {
    FatalError("Can't set the handler for alarm signal.");
  }

  /*
   * Let the smart schedule set the SIGALRM handler again.
   */

  nxagentInitTimer();
}

/*
 * currently unused it seems
 */
void nxagentOpenConfineWindow(void)
{
#ifdef NX_CONFINE_WINDOW
  nxagentConfineWindow = XCreateWindow(nxagentDisplay,
                                       DefaultRootWindow(nxagentDisplay),
                                       0, 0, 1, 1, 0, 0,
                                       InputOnly,
                                       CopyFromParent,
                                       0L, NULL);

  if (nxagentReportWindowIds) {
    fprintf(stderr, "NXAGENT_WINDOW_ID: CONFINEMENT_WINDOW,WID:[0x%x]\n",
              nxagentConfineWindow);
  }

  #ifdef DEBUG
  {
    char *winname = NULL;
    if (-1 != asprintf(&winname, "%s Confine", nxagentWindowName))
    {
      Xutf8SetWMProperties(nxagentDisplay, nxagentConfineWindow,
                               winname, winname, NULL , 0 , NULL, NULL, NULL);
      SAFE_free(winname);
    }
  }
  #endif

  #ifdef TEST
  fprintf(stderr, "%s: Created agent's confine window with id [0x%x].\n", __func__, nxagentConfineWindow);
  #endif
#endif
}

void nxagentOpenDisplay(int argc, char *argv[])
{
  if (!nxagentDoFullGeneration)
    return;

  #ifdef NXAGENT_TIMESTAMP
  startTime = GetTimeInMillis();
  fprintf(stderr, "Display: Opening the display on real X server with time [%d] ms.\n",
          GetTimeInMillis() - startTime);
  #endif

  /*
   * Initialize the reconnector only in the case of persistent
   * sessions.
   */

  if (nxagentOption(Persistent))
  {
    nxagentInitReconnector();
  }

  if (*nxagentDisplayName == '\0')
  {
    snprintf(nxagentDisplayName, NXAGENTDISPLAYNAMELENGTH, "%s", XDisplayName(NULL));
  }

  nxagentCloseDisplay();

  nxagentInstallSignalHandlers();

  nxagentInstallDisplayHandlers();

  nxagentDisplay = nxagentInternalOpenDisplay(nxagentDisplayName);

  nxagentPostInstallSignalHandlers();

  nxagentPostInstallDisplayHandlers();

  if (nxagentDisplay == NULL)
  {
/*
FIXME: The agent should never exit the program with a FatalError() but
       rather use a specific function that may eventually call
       FatalError() on its turn.
*/
    FatalError("Unable to open display '%s'.\n", nxagentDisplayName);
  }

  if (nxagentSynchronize)
    XSynchronize(nxagentDisplay, True);

  nxagentXConnectionNumber = XConnectionNumber(nxagentDisplay);

  #ifdef TEST
  fprintf(stderr, "nxagentOpenDisplay: Display image order is [%d] bitmap order is [%d].\n",
              ImageByteOrder(nxagentDisplay), BitmapBitOrder(nxagentDisplay));

  fprintf(stderr, "nxagentOpenDisplay: Display scanline unit is [%d] scanline pad is [%d].\n",
              BitmapUnit(nxagentDisplay), BitmapPad(nxagentDisplay));
  #endif

  #ifdef WATCH

  fprintf(stderr, "nxagentOpenDisplay: Watchpoint 1.\n");

/*
Reply   Total	Cached	Bits In			Bits Out		Bits/Reply	  Ratio
------- -----	------	-------			--------		----------	  -----
#1   U  1	1	256 bits (0 KB) ->	150 bits (0 KB) ->	256/1 -> 150/1	= 1.707:1
#20     1	1	119104 bits (15 KB) ->	28 bits (0 KB) ->	119104/1 -> 28/1	= 4253.714:1
#98     2		512 bits (0 KB) ->	84 bits (0 KB) ->	256/1 -> 42/1	= 6.095:1
*/

  sleep(60);

  #endif

  #ifdef NXAGENT_TIMESTAMP
  fprintf(stderr, "Display: Display on real X server opened with time [%d] ms.\n",
          GetTimeInMillis() - startTime);
  #endif

  nxagentUseNXTrans =
      nxagentPostProcessArgs(nxagentDisplayName, nxagentDisplay,
                                 DefaultScreenOfDisplay(nxagentDisplay));

  /*
   * Processing the arguments all the timeouts have been set. Now we
   * have to change the screen-saver timeout.
   */

  nxagentSetScreenSaverTime();

  nxagentInitVisuals();

  nxagentNumDefaultColormaps = nxagentNumVisuals;
  nxagentDefaultColormaps = (Colormap *)malloc(nxagentNumDefaultColormaps *
                                             sizeof(Colormap));

  for (int i = 0; i < nxagentNumDefaultColormaps; i++)
  {
    nxagentDefaultColormaps[i] = XCreateColormap(nxagentDisplay,
                                               DefaultRootWindow(nxagentDisplay),
                                               nxagentVisuals[i].visual,
                                               AllocNone);
  }

  #ifdef WATCH

  fprintf(stderr, "nxagentOpenDisplay: Watchpoint 4.\n");

/*
Reply   Total	Cached	Bits In			Bits Out		Bits/Reply	  Ratio
------- -----	------	-------			--------		----------	  -----
N/A
*/

  sleep(30);

  #endif

  nxagentBlackPixel = BlackPixel(nxagentDisplay, DefaultScreen(nxagentDisplay));
  nxagentWhitePixel = WhitePixel(nxagentDisplay, DefaultScreen(nxagentDisplay));

  #ifdef WATCH

  fprintf(stderr, "nxagentOpenDisplay: Watchpoint 5.\n");

/*
Reply   Total	Cached	Bits In			Bits Out		Bits/Reply	  Ratio
------- -----	------	-------			--------		----------	  -----
N/A
*/

  sleep(30);

  #endif

  /*
   * Initialize the agent's event mask that will be requested for the
   * root and all the top level windows. If the nested window is a
   * child of an existing window, we will need to receive
   * StructureNotify events. If we are going to manage the changes in
   * root window's visibility we'll also need VisibilityChange events.
   */

/*
FIXME: Use of nxagentParentWindow is strongly deprecated.  We need
       also to clarify which events are selected in the different
       operating modes.
*/

  nxagentInitDefaultEventMask();

  /*
   * Initialize the pixmap depths and formats.
   */

  nxagentInitDepths();

  nxagentInitPixmapFormats();
  (void) nxagentCheckForPixmapFormatsCompatibility();

  /*
   * Create a pixmap for each depth matching the local supported
   * formats with format available on the remote display.
   */

  nxagentSetDefaultDrawables();

  #ifdef RENDER
  if (nxagentRenderEnable)
  {
    nxagentRenderExtensionInit();
  }
  #endif

  /*
   * This GC is referenced in Cursor.c. It can be probably removed.
   */

  nxagentBitmapGC = XCreateGC(nxagentDisplay, nxagentDefaultDrawables[1], 0L, NULL);

  /*
   * Note that this "confine window" is useless at the moment as we
   * reimplement nxagentConstrainCursor() to skip the "constrain"
   * stuff.
   */

  #ifdef TEST
  fprintf(stderr, "nxagentOpenDisplay: Going to create agent's confine window.\n");
  #endif

  nxagentOpenConfineWindow();

  if (!(nxagentUserGeometry.flag & XValue))
  {
    nxagentChangeOption(RootX, 0);
  }

  if (!(nxagentUserGeometry.flag & YValue))
  {
    nxagentChangeOption(RootY, 0);
  }

  if (nxagentParentWindow == 0)
  {
    if (!(nxagentUserGeometry.flag & WidthValue))
    {
      if (nxagentOption(Fullscreen) || nxagentOption(Rootless))
      {
        nxagentChangeOption(RootWidth, DisplayWidth(nxagentDisplay, DefaultScreen(nxagentDisplay)));
      }
      else
      {
        nxagentChangeOption(RootWidth, 3 * DisplayWidth(nxagentDisplay,
                                           DefaultScreen(nxagentDisplay)) / 4);
      }
    }

    if (!(nxagentUserGeometry.flag & HeightValue))
    {
      if (nxagentOption(Fullscreen) || nxagentOption(Rootless))
      {
        nxagentChangeOption(RootHeight, DisplayHeight(nxagentDisplay, DefaultScreen(nxagentDisplay)));
      }
      else
      {
        nxagentChangeOption(RootHeight, 3 * DisplayHeight(nxagentDisplay,
                                            DefaultScreen(nxagentDisplay)) / 4);
      }
    }
  }

  if (!nxagentUserBorderWidth)
  {
    nxagentChangeOption(BorderWidth, 1);
  }

  #ifdef WATCH

  fprintf(stderr, "nxagentOpenDisplay: Watchpoint 5.1.\n");

/*
Reply   Total	Cached	Bits In			Bits Out		Bits/Reply	  Ratio
------- -----	------	-------			--------		----------	  -----
N/A
*/

  sleep(30);

  #endif

  useXpmIcon = nxagentMakeIcon(nxagentDisplay, &nxagentIconPixmap, &nxagentIconShape);

  #ifdef WATCH

  fprintf(stderr, "nxagentOpenDisplay: Watchpoint 5.2.\n");

/*
Reply   Total	Cached	Bits In			Bits Out		Bits/Reply	  Ratio
------- -----	------	-------			--------		----------	  -----
#84     2		512 bits (0 KB) ->	76 bits (0 KB) ->	256/1 -> 38/1	= 6.737:1
*/

  sleep(30);

  #endif

  #ifdef WATCH

  fprintf(stderr, "nxagentOpenDisplay: Watchpoint 6.\n");

/*
Reply   Total	Cached	Bits In			Bits Out		Bits/Reply	  Ratio
------- -----	------	-------			--------		----------	  -----
N/A
*/

  sleep(30);

  #endif

  #ifdef NXAGENT_TIMESTAMP
  fprintf(stderr, "Display: Open of the display finished with time [%d] ms.\n",
          GetTimeInMillis() - startTime);
  #endif

  if (nxagentOption(Persistent))
  {
    reconnectDisplayState = EVERYTHING_DONE;
  }
}

void nxagentSetDefaultVisual(void)
{
  if (nxagentUserDefaultClass || nxagentUserDefaultDepth)
  {
    nxagentDefaultVisualIndex = UNDEFINED;

    for (int i = 0; i < nxagentNumVisuals; i++)
    {
      if ((!nxagentUserDefaultClass ||
           nxagentVisuals[i].class == nxagentDefaultClass)
          &&
          (!nxagentUserDefaultDepth ||
           nxagentVisuals[i].depth == nxagentDefaultDepth))
      {
        nxagentDefaultVisualIndex = i;
        break;
      }
    }

    if (nxagentDefaultVisualIndex == UNDEFINED)
    {
      FatalError("Unable to find desired default visual.\n");
    }
  }
  else
  {
    XVisualInfo vi = {
      .visualid = XVisualIDFromVisual(DefaultVisual(nxagentDisplay,
                                          DefaultScreen(nxagentDisplay)))
    };

    nxagentDefaultVisualIndex = 0;

    for (int i = 0; i < nxagentNumVisuals; i++)
    {
      if (vi.visualid == nxagentVisuals[i].visualid)
      {
        nxagentDefaultVisualIndex = i;
      }
    }
  }
}

void nxagentInitVisuals(void)
{
  long mask = VisualScreenMask;
  XVisualInfo vi = {
    .screen = DefaultScreen(nxagentDisplay),
    .depth = DefaultDepth(nxagentDisplay, DefaultScreen(nxagentDisplay))
  };
  int viNumList;
  XVisualInfo *viList = XGetVisualInfo(nxagentDisplay, mask, &vi, &viNumList);
  nxagentVisuals = (XVisualInfo *) malloc(viNumList * sizeof(XVisualInfo));
  nxagentNumVisuals = 0;

  for (int i = 0; i < viNumList; i++)
  {
    if (viList[i].depth == vi.depth)
    {
      if (nxagentVisuals != NULL)
      {
        memcpy(nxagentVisuals + nxagentNumVisuals, viList + i, sizeof(XVisualInfo));
      }

      #ifdef DEBUG
      fprintf(stderr, "nxagentInitVisuals: Visual:\n");
      fprintf(stderr, "\tdepth = %d\n", nxagentVisuals[nxagentNumVisuals].depth);
      fprintf(stderr, "\tclass = %d\n", nxagentVisuals[nxagentNumVisuals].class);
      fprintf(stderr, "\tmask = (%lu,%lu,%lu)\n",
                  nxagentVisuals[nxagentNumVisuals].red_mask,
                      nxagentVisuals[nxagentNumVisuals].green_mask,
                          nxagentVisuals[nxagentNumVisuals].blue_mask);
      fprintf(stderr, "\tcolormap size = %d\n", nxagentVisuals[nxagentNumVisuals].colormap_size);
      fprintf(stderr, "\tbits_per_rgb = %d\n", nxagentVisuals[nxagentNumVisuals].bits_per_rgb);
      #endif

      nxagentNumVisuals++;
    }
  }

  if (nxagentVisuals != NULL)
  {
    XVisualInfo *new = (XVisualInfo *) realloc(nxagentVisuals,
                                                   nxagentNumVisuals * sizeof(XVisualInfo));
    /* nxagentVisuals being NULL is covered below */
    if (new)
      nxagentVisuals = new;
    else
      SAFE_free(nxagentVisuals);
  }

  SAFE_XFree(viList);

  if (nxagentNumVisuals == 0 || nxagentVisuals == NULL)
  {
    FatalError("Unable to find any visuals.\n");
  }

  nxagentSetDefaultVisual();
}

void nxagentInitDepths(void)
{
  nxagentDepths = XListDepths(nxagentDisplay, DefaultScreen(nxagentDisplay),
                                  &nxagentNumDepths);

  if (nxagentDepths == NULL)
  {
    #ifdef PANIC
    fprintf(stderr, "nxagentInitDepths: PANIC! Failed to get available depths.\n");
    #endif

    FatalError("Failed to get available depths and pixmap formats.");
  }
  #ifdef TEST
  else
  {
    fprintf(stderr, "nxagentInitDepths: Got [%d] available depths:\n",
                nxagentNumDepths);

    for (int i = 0; i < nxagentNumDepths; i++)
    {
      fprintf(stderr, " [%d]", nxagentDepths[i]);
    }

    fprintf(stderr, ".\n");
  }
  #endif
}

void nxagentInitPixmapFormats(void)
{
  /*
   * Formats are created with no care of which are supported on the
   * real display. Creating only formats supported by the remote end
   * makes troublesome handling migration of session from a display to
   * another.
   */

  nxagentNumPixmapFormats = 0;
  nxagentRemoteNumPixmapFormats = 0;

/*
XXX: Some X server doesn't list 1 among available depths...
*/

  nxagentPixmapFormats = malloc((nxagentNumDepths + 1) * sizeof(XPixmapFormatValues));

  for (int i = 1; i <= MAXDEPTH; i++)
  {
    int depth = 0;

    if (i == 1)
    {
      depth = 1;
    }
    else
    {
      for (int j = 0; j < nxagentNumDepths; j++)
      {
        if (nxagentDepths[j] == i)
        {
          depth = i;
          break;
        }
      }
    }

    if (depth != 0)
    {
      if (nxagentNumPixmapFormats >= MAXFORMATS)
      {
        FatalError("nxagentInitPixmapFormats: MAXFORMATS is too small for this server.\n");
      }

      nxagentPixmapFormats[nxagentNumPixmapFormats].depth = depth;
      nxagentPixmapFormats[nxagentNumPixmapFormats].bits_per_pixel = nxagentBitsPerPixel(depth);
      nxagentPixmapFormats[nxagentNumPixmapFormats].scanline_pad = BITMAP_SCANLINE_PAD;

      #ifdef TEST
      fprintf(stderr, "nxagentInitPixmapFormats: Set format [%d] to depth [%d] "
                  "bits per pixel [%d] scanline pad [%d].\n", nxagentNumPixmapFormats,
                      depth, nxagentPixmapFormats[nxagentNumPixmapFormats].bits_per_pixel,
                          BITMAP_SCANLINE_PAD);
      #endif

      nxagentNumPixmapFormats++;
    }
  }

  /*
   * we need to filter the list of remote pixmap formats by our
   * supported depths, just like above. If we do not perform this step
   * nxagentCheckForPixmapFormatsCompatibility will fail when
   * tolerance is "strict" (the default). This becomes evident when
   * Xephyr or Xnest are used as the real X server. They normally show
   * only two supported depths but 7 supported pixmap formats (which
   * could be a bug there).
   */

  int tmpnum = 0;
  XPixmapFormatValues *tmp = XListPixmapFormats(nxagentDisplay, &tmpnum);

  if (tmp == NULL)
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentInitPixmapFormats: WARNING! Failed to get available remote pixmap formats.\n");
    #endif

    nxagentRemotePixmapFormats = NULL;
  }
  else
  {
    #ifdef TEST
    fprintf(stderr, "nxagentInitPixmapFormats: Got [%d] available remote pixmap formats:\n",
                tmpnum);

    for (int i = 0; i < tmpnum; i++)
    {
      fprintf(stderr, "nxagentInitPixmapFormats: Found remote pixmap format [%d]: depth [%d] "
                  "bits_per_pixel [%d] scanline_pad [%d].\n", i, tmp[i].depth,
                      tmp[i].bits_per_pixel, tmp[i].scanline_pad);

    }
    #endif

    SAFE_XFree(tmp);

    nxagentRemotePixmapFormats = malloc((nxagentNumDepths + 1) * sizeof(XPixmapFormatValues));

    for (int i = 1; i <= MAXDEPTH; i++)
    {
      int depth = 0;

      if (i == 1)
      {
        depth = 1;
      }
      else
      {
        for (int j = 0; j < nxagentNumDepths; j++)
        {
          if (nxagentDepths[j] == i)
          {
            depth = i;
            break;
          }
        }
      }

      if (depth != 0)
      {
        if (nxagentRemoteNumPixmapFormats >= MAXFORMATS)
        {
          FatalError("nxagentInitPixmapFormats: MAXFORMATS is too small for this remote server.\n");
        }

        nxagentRemotePixmapFormats[nxagentRemoteNumPixmapFormats].depth = depth;
        nxagentRemotePixmapFormats[nxagentRemoteNumPixmapFormats].bits_per_pixel = nxagentBitsPerPixel(depth);
        nxagentRemotePixmapFormats[nxagentRemoteNumPixmapFormats].scanline_pad = BITMAP_SCANLINE_PAD;

        #ifdef TEST
        fprintf(stderr, "nxagentInitPixmapFormats: Suitable remote format [%d] to depth [%d] "
                    "bits per pixel [%d] scanline pad [%d].\n", nxagentRemoteNumPixmapFormats,
                        depth, nxagentRemotePixmapFormats[nxagentRemoteNumPixmapFormats].bits_per_pixel,
                            BITMAP_SCANLINE_PAD);
        #endif

        nxagentRemoteNumPixmapFormats++;
      }
    }

    #ifdef TEST
    for (int i = 0; i < nxagentRemoteNumPixmapFormats; i++)
    {
      fprintf(stderr, "nxagentInitPixmapFormats: Remote pixmap format [%d]: depth [%d] "
                  "bits_per_pixel [%d] scanline_pad [%d].\n", i, nxagentRemotePixmapFormats[i].depth,
                      nxagentRemotePixmapFormats[i].bits_per_pixel, nxagentRemotePixmapFormats[i].scanline_pad);
    }
    #endif
  }
}

void nxagentSetDefaultDrawables(void)
{
  for (int i = 0; i <= MAXDEPTH; i++)
  {
    nxagentDefaultDrawables[i] = None;
  }

  for (int i = 0; i < nxagentNumPixmapFormats; i++)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentSetDefaultDrawables: Checking remote pixmap format [%d] with depth [%d] "
                "bits per pixel [%d] scanline pad [%d].\n", i, nxagentPixmapFormats[i].depth,
                    nxagentPixmapFormats[i].bits_per_pixel, nxagentPixmapFormats[i].scanline_pad);
    #endif

    if (nxagentPixmapFormats[i].depth == 24)
    {
      if (nxagentPixmapFormats[i].bits_per_pixel == 24)
      {
        #ifdef TEST
        fprintf(stderr, "nxagentSetDefaultDrawables: WARNING! Assuming remote pixmap "
                    "format [%d] as true 24 bits.\n", i);
        #endif

        nxagentTrue24 = True;
      }
    }

    for (int j = 0; j < nxagentNumDepths; j++)
    {
      #ifdef TEST
      fprintf(stderr, "nxagentSetDefaultDrawables: Checking depth at index [%d] with pixmap depth [%d] "
                  "and display depth [%d].\n", j, nxagentPixmapFormats[i].depth, nxagentDepths[j]);
      #endif

      if ((nxagentPixmapFormats[i].depth == 1 ||
              nxagentPixmapFormats[i].depth == nxagentDepths[j]) &&
                  nxagentDefaultDrawables[nxagentPixmapFormats[i].depth] == None)
      {
        nxagentDefaultDrawables[nxagentPixmapFormats[i].depth] =
            XCreatePixmap(nxagentDisplay, DefaultRootWindow(nxagentDisplay),
                              1, 1, nxagentPixmapFormats[i].depth);

        #ifdef TEST
        fprintf(stderr, "nxagentSetDefaultDrawables: Created default drawable [%lu] for depth [%d].\n",
                    nxagentDefaultDrawables[nxagentPixmapFormats[i].depth], nxagentPixmapFormats[i].depth);
        #endif
      }
    }

    if (nxagentDefaultDrawables[nxagentPixmapFormats[i].depth] == None)
    {
      #ifdef TEST
      fprintf(stderr, "nxagentSetDefaultDrawables: WARNING! Forcing default drawable for depth [%d].\n",
                  nxagentPixmapFormats[i].depth);
      #endif

      nxagentDefaultDrawables[nxagentPixmapFormats[i].depth] =
          XCreatePixmap(nxagentDisplay, DefaultRootWindow(nxagentDisplay),
                            1, 1, nxagentPixmapFormats[i].depth);

      #ifdef TEST
      fprintf(stderr, "nxagentSetDefaultDrawables: Created default drawable [%lu] for depth [%d].\n",
                  nxagentDefaultDrawables[nxagentPixmapFormats[i].depth], nxagentPixmapFormats[i].depth);
      #endif
    }
  }
}

void nxagentCloseDisplay(void)
{
  #ifdef TEST
  fprintf(stderr, "nxagentCloseDisplay: Called with full generation [%d] and display [%p].\n",
              nxagentDoFullGeneration, (void *) nxagentDisplay);
  #endif

  if (!nxagentDoFullGeneration ||
          nxagentDisplay == NULL)
  {
    return;
  }

  /*
   * If nxagentDoFullGeneration is true, all the X resources will be
   * destroyed upon closing the display connection, so there is no
   * real need to generate additional traffic
   */

  SAFE_free(nxagentDefaultColormaps);
  SAFE_free(nxagentDepths);
  SAFE_XFree(nxagentVisuals);
  SAFE_XFree(nxagentPixmapFormats);
  SAFE_XFree(nxagentRemotePixmapFormats);

  nxagentFreeFontCache();
/*
FIXME: Is this needed?

  nxagentFreeFontMatchStuff();
*/

  /*
   * Free the image cache. This is useful for detecting memory leaks.
   */

  if (nxagentDisplay != NULL)
  {
    NXFreeCache(nxagentDisplay);
    NXResetDisplay(nxagentDisplay);
  }

  /*
   * Kill all the running dialogs.
   */

  nxagentTerminateDialogs();

  #ifdef TEST
  fprintf(stderr, "nxagentCloseDisplay: Setting the display to NULL.\n");
  #endif

  XCloseDisplay(nxagentDisplay);

  nxagentDisplay = NULL;
}

Bool nxagentMakeIcon(Display *disp, Pixmap *nxIcon, Pixmap *nxMask)
{
  char** agentIconData;

#ifdef X2GO
  /*
   * selecting x2go icon when running as X2Go agent
   */
  if (nxagentX2go)
  {
    agentIconData = x2goagentIconData;
  }
  else
#endif
  {
    agentIconData = nxagentIconData;
  }

  XlibPixmap IconPixmap;
  XlibPixmap IconShape;
  if (XpmSuccess == XpmCreatePixmapFromData(disp,
                                            DefaultRootWindow(disp),
                                            agentIconData,
                                            &IconPixmap,
                                            &IconShape,
                                            NULL))
  {
    *nxIcon = IconPixmap;
    *nxMask = IconShape;

    return True;
  }
  else
  {
    #ifdef TEST
    fprintf(stderr, "%s: Xpm operation failed.\n", __func__);
    #endif

    return False;
  }
}

Bool nxagentXServerGeometryChanged(void)
{
  return  (WidthOfScreen(DefaultScreenOfDisplay(nxagentDisplay)) !=
               WidthOfScreen(DefaultScreenOfDisplay(nxagentDisplayBackup))) ||
                    (HeightOfScreen(DefaultScreenOfDisplay(nxagentDisplay)) !=
                         HeightOfScreen(DefaultScreenOfDisplay(nxagentDisplayBackup)));
}

void nxagentBackupDisplayInfo(void)
{
  if (nxagentDisplayInfoSaved)
  {
    return;
  }

  /*
   * Since we need the display structure in order to behave correctly
   * when no X connection is available, we must always have a good
   * display record.  It can be discarded only when a new X connection
   * is available, so we store it in order to destroy whenever the
   * reconnection succeeds.
   */

  nxagentDisplayBackup = nxagentDisplay;
  nxagentBitmapGCBackup = nxagentBitmapGC;
  nxagentDepthsRecBackup = nxagentDepths;
  nxagentNumDepthsRecBackup = nxagentNumDepths;
  nxagentNumDefaultColormapsRecBackup = nxagentNumDefaultColormaps;
  nxagentVisualsRecBackup = nxagentVisuals;
  nxagentNumVisualsRecBackup = nxagentNumVisuals;
  SAFE_free(nxagentVisualHasBeenIgnored);
  nxagentVisualHasBeenIgnored = malloc(nxagentNumVisuals * sizeof(Bool));
  nxagentDefaultDepthRecBackup = DefaultDepth(nxagentDisplay, DefaultScreen(nxagentDisplay));
  nxagentDisplayWidthRecBackup = DisplayWidth(nxagentDisplay, DefaultScreen(nxagentDisplay));
  nxagentDisplayHeightRecBackup = DisplayHeight(nxagentDisplay, DefaultScreen(nxagentDisplay));
  nxagentRenderEnableRecBackup = nxagentRenderEnable;

  nxagentDisplayInfoSaved = True;
}

void nxagentCleanupBackupDisplayInfo(void)
{
  SAFE_free(nxagentDepthsRecBackup);
  SAFE_free(nxagentVisualsRecBackup);
  SAFE_free(nxagentVisualHasBeenIgnored);

  nxagentNumDefaultColormapsRecBackup = 0;
  nxagentDefaultDepthRecBackup = 0;
  nxagentDisplayWidthRecBackup = 0;
  nxagentDisplayHeightRecBackup = 0;

  if (nxagentDisplayBackup)
  {
    XCloseDisplay(nxagentDisplayBackup);
    nxagentDisplayBackup = NULL;
  }

  if (nxagentBitmapGCBackup)
  {
    if (nxagentDisplayBackup)
    {
      XFreeGC(nxagentDisplayBackup, nxagentBitmapGCBackup);
    }
    else
    {
      SAFE_free(nxagentBitmapGCBackup);
    }

    nxagentBitmapGCBackup = NULL;
  }

  nxagentDisplayInfoSaved = False;
}

void nxagentDisconnectDisplay(void)
{
  switch (reconnectDisplayState)
  {
    case EVERYTHING_DONE:
      if (nxagentBitmapGC &&
              nxagentBitmapGCBackup &&
                  (nxagentBitmapGC != nxagentBitmapGCBackup))
      {
        XFreeGC(nxagentDisplay, nxagentBitmapGC);
      }

      nxagentBitmapGC = nxagentBitmapGCBackup;
    case GOT_PIXMAP_FORMAT_LIST:
    case GOT_DEPTH_LIST:
    case ALLOC_DEF_COLORMAP:
      if (nxagentDefaultColormaps)
      {
        for (int i = 0; i < nxagentNumDefaultColormaps; i++)
        {
          nxagentDefaultColormaps[i] = None;
        }
      }
    case GOT_VISUAL_INFO:
    case OPENED:
      /*
       * Actually we need the nxagentDisplay structure in order to let
       * the agent go when no X connection is available.
       */

      if (nxagentDisplay &&
              nxagentDisplayBackup &&
                  (nxagentDisplay != nxagentDisplayBackup))
      {
        XCloseDisplay(nxagentDisplay);
      }
    case NOTHING:
      nxagentDisplay = nxagentDisplayBackup;
      break;
    default:
      FatalError("Display is in unknown state. Can't continue.");
  }

  reconnectDisplayState = NOTHING;
}

static int nxagentCheckForDefaultDepthCompatibility(void)
{
  /*
   * Depending on the (reconnect) tolerance checks value, this
   * function checks stricter or looser:
   *   - "Strict" means that the old and new default depth values
   *     must match exactly.
   *   - "Safe" or "Risky" means that the default depth values might differ,
   *     but the new default depth value must be at least as
   *     high as the former default depth value. This is
   *     recommended, because it allows clients with a
   *     higher default depth value to still connect, but
   *     not lose functionality.
   *   - "Bypass" means that all of these checks are essentially
   *     deactivated. This is probably a very bad idea.
   */

  int dDepth = DefaultDepth(nxagentDisplay, DefaultScreen(nxagentDisplay));

  const unsigned int tolerance = nxagentOption(ReconnectTolerance);

  if (ToleranceChecksBypass <= tolerance)
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentCheckForDefaultDepthCompatibility: WARNING! Not proceeding with any checks, "
                    "because tolerance [%u] higher than or equal [%u]. New default depth value "
                    "is [%d], former default depth value is [%d].\n", tolerance,
            ToleranceChecksBypass, dDepth, nxagentDefaultDepthRecBackup);
    #endif
    return 1;
  }

  if (nxagentDefaultDepthRecBackup == dDepth)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentCheckForDefaultDepthCompatibility: New default depth [%d] "
                "matches with old default depth.\n", dDepth);
    #endif
    return 1;
  }
  else if ((ToleranceChecksSafe <= tolerance) && (nxagentDefaultDepthRecBackup < dDepth))
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentCheckForDefaultDepthCompatibility: WARNING! New default depth [%d] "
                    "higher than the old default depth [%d] at tolerance [%u].\n", dDepth,
            nxagentDefaultDepthRecBackup, tolerance);
    #endif
    return 1;
  }
  else
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentCheckForDefaultDepthCompatibility: WARNING! New default depth [%d] "
                "doesn't match with old default depth [%d] at tolerance [%u].\n", dDepth,
            nxagentDefaultDepthRecBackup, tolerance);
    #endif
    return 0;
  }
}

static int nxagentCheckForDepthsCompatibility(void)
{
  /*
   * Depending on the (reconnect) tolerance checks value, this
   * function checks stricter or looser:
   *   - "Strict" means that the number of old and new depths must
   *     match exactly and every old depth value must be
   *     available in the new depth array.
   *   - "Safe" means that the number of depths might diverge,
   *     but all former depth must also be included in the
   *     new depth array. This is recommended, because
   *     it allows clients with more depths to still
   *     connect, but not lose functionality.
   *   - "Risky" means that the new depths array is allowed to be
   *     smaller than the old depths array, but at least
   *     one depth value must be included in both.
   *     This is potentially unsafe.
   *   - "Bypass" or higher means that all of these checks are
   *     essentially deactivated. This is a very bad idea.
   */

  const unsigned int tolerance = nxagentOption(ReconnectTolerance);

  if (ToleranceChecksBypass <= tolerance)
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentCheckForDepthsCompatibility: WARNING! Not proceeding with any checks, "
                    "because tolerance [%u] higher than or equal [%u]. Number of newly available depths "
                    "is [%d], number of old depths is [%d].\n", tolerance, ToleranceChecksBypass,
            nxagentNumDepths, nxagentNumDepthsRecBackup);
    #endif
    return 1;
  }

  if ((ToleranceChecksStrict == tolerance) && (nxagentNumDepths != nxagentNumDepthsRecBackup))
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentCheckForDepthsCompatibility: WARNING! No tolerance allowed and "
                    "number of new available depths [%d] doesn't match with number of old "
                    "depths [%d].\n", nxagentNumDepths,
            nxagentNumDepthsRecBackup);
    #endif
    return 0;
  }

  if ((ToleranceChecksSafe == tolerance) && (nxagentNumDepths < nxagentNumDepthsRecBackup))
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentCheckForDepthsCompatibility: WARNING! Tolerance [%u] not "
                    "high enough and number of new available depths [%d] "
                    "lower than number of old depths [%d].\n", tolerance,
            nxagentNumDepths, nxagentNumDepthsRecBackup);
    #endif
    return 0;
  }

  /*
   * By now the tolerance is either:
   *   - "Strict" and both depth numbers match
   *   - "Safe" and:
   *     o the number of old and new depths matches exactly, or
   *     o the number of old depths is lower than the number
   *       of new depths
   *   - "Risky"
   */

  bool compatible = true;
  bool one_match = false;
  int total_matches = 0;

  /*
   * FIXME: within this loop, we try to match all "new" depths
   *        against the "old" depths. Depending upon the flexibility
   *        value, either all "new" depths must have a corresponding
   *        counterpart in the "old" array, or at least one value
   *        must be included in both.
   *        Is this safe enough though?
   *        Shouldn't we better try to match entries in the "old"
   *        depths array against the "new" depths array, such that
   *        we know that all "old" values are covered by "new"
   *        values? Or is it more important that "new" values are
   *        covered by "old" ones, with potentially more "old"
   *        values lingering around that cannot be displayed by the
   *        connected client?
   *
   * This section probably needs a revisit at some point in time.
   */
  for (int i = 0; i < nxagentNumDepths; ++i)
  {
    bool matched = false;

    for (int j = 0; j < nxagentNumDepthsRecBackup; ++j)
    {
      if (nxagentDepths[i] == nxagentDepthsRecBackup[j])
      {
        matched = true;
        one_match = true;
        ++total_matches;

        break;
      }
    }

    if ((ToleranceChecksRisky > tolerance) && (!matched))
    {
      #ifdef WARNING
      fprintf(stderr, "nxagentCheckForDepthsCompatibility: WARNING! Tolerance [%u] too low and "
                      "failed to match available depth [%d].\n", tolerance, nxagentDepths[i]);
      #endif

      compatible = false;

      break;
    }
  }

  /*
   * At Risky tolerance, only one match is necessary to be "compatible".
   */
  if (ToleranceChecksRisky == tolerance)
  {
    compatible = one_match;
  }

  int ret = (!(!compatible));

  if (compatible)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentCheckForDepthsCompatibility: Internal depths match with "
                "remote depths at tolerance [%u].\n", tolerance);
    #endif

    if (total_matches != nxagentNumDepths)
    {
      #ifdef WARNING
      fprintf(stderr, "nxagentCheckForDepthsCompatibility: only some [%d] of the new depths [%d] "
                  "match with old depths [%d] at tolerance [%u].\n", total_matches, nxagentNumDepths,
              nxagentNumDepthsRecBackup, tolerance);
      #endif
    }
  }
  else
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentCheckForDepthsCompatibility: WARNING! New available depths [%d] don't match "
                    "with old depths [%d] at tolerance [%u]. Only [%d] depth values matched.\n",
            nxagentNumDepths, nxagentNumDepthsRecBackup, tolerance, total_matches);
    #endif
  }

  return (ret);
}

static int nxagentCheckForPixmapFormatsCompatibility(void)
{
  /*
   * Depending on the (reconnect) tolerance checks value, this
   * function checks stricter or looser:
   *   - "Strict" means that the number of internal and external
   *     pixmap formats must match exactly and every
   *     internal pixmap format must be available in the
   *     external pixmap format array.
   *   - "Safe" means that the number of pixmap formats might
   *     diverge, but all internal pixmap formats must
   *     also be included in the external pixmap formats
   *     array. This is recommended, because it allows
   *     clients with more pixmap formats to still connect,
   *     but not lose functionality.
   *   - "Risky" means that the internal pixmap formats array is
   *     allowed to be smaller than the external pixmap
   *     formats array, but at least one pixmap format must
   *     be included in both. This is potentially unsafe.
   *   - "Bypass" or higher means that all of these checks are
   *     essentially deactivated. This is a very bad idea.
   */

  const unsigned int tolerance = nxagentOption(ReconnectTolerance);

  if (ToleranceChecksBypass <= tolerance)
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentCheckForPixmapFormatsCompatibility: WARNING! Not proceeding with any checks, "
                    "because tolerance [%u] higher than or equal [%u]. Number of internally available "
                    "pixmap formats is [%d], number of externally available pixmap formats is [%d].\n",
            tolerance, ToleranceChecksBypass, nxagentNumPixmapFormats, nxagentRemoteNumPixmapFormats);
    #endif
    return 1;
  }

  if ((ToleranceChecksStrict == tolerance) && (nxagentNumPixmapFormats != nxagentRemoteNumPixmapFormats))
  {
    #ifdef DEBUG
    fprintf(stderr, "nxagentCheckForPixmapFormatsCompatibility: WARNING! No tolerance allowed and number "
                    "of internal pixmap formats [%d] doesn't match with number of remote formats [%d].\n",
            nxagentNumPixmapFormats, nxagentRemoteNumPixmapFormats);
    #endif
    return 0;
  }

  if ((ToleranceChecksSafe == tolerance) && (nxagentNumPixmapFormats > nxagentRemoteNumPixmapFormats))
  {
    #ifdef DEBUG
    fprintf(stderr, "nxagentCheckForPixmapFormatsCompatibility: WARNING! Tolerance [%u] too low "
                    "and number of internal pixmap formats [%d] higher than number of external formats [%d].\n",
            tolerance, nxagentNumPixmapFormats, nxagentRemoteNumPixmapFormats);
    #endif
    return 0;
  }

  /*
   * By now the tolerance is either:
   *   - "Strict"
   *   - "Safe" and:
   *     o the number of internal and external pixmap formats
   *       matches exactly, or
   *     o the number of external pixmap formats is higher than
   *       the number of internal pixmap formats,
   *   - "Risky"
   */

  bool compatible = true;
  int total_matches = 0;

  for (int i = 0; i < nxagentNumPixmapFormats; ++i)
  {
    bool matched = false;

    for (int j = 0; j < nxagentRemoteNumPixmapFormats; ++j)
    {
      if (nxagentPixmapFormats[i].depth == nxagentRemotePixmapFormats[j].depth &&
          nxagentPixmapFormats[i].bits_per_pixel == nxagentRemotePixmapFormats[j].bits_per_pixel &&
          nxagentPixmapFormats[i].scanline_pad == nxagentRemotePixmapFormats[j].scanline_pad)
      {
        matched = true;
        ++total_matches;

        break;
      }
    }

    if ((ToleranceChecksRisky > tolerance) && (!matched))
    {
      #ifdef WARNING
      fprintf(stderr, "nxagentCheckForPixmapFormatsCompatibility: WARNING! Tolerance [%u] too low "
                      "and failed to match internal pixmap format (depth [%d] bpp [%d] pad [%d]).\n",
              tolerance, nxagentPixmapFormats[i].depth, nxagentPixmapFormats[i].bits_per_pixel,
              nxagentPixmapFormats[i].scanline_pad);
      #endif

      compatible = false;
    }
  }

  int ret = !(!(compatible));

  if (compatible)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentCheckForPixmapFormatsCompatibility: Internal pixmap formats match with "
                    "remote pixmap formats at tolerance [%u].\n", tolerance);
    #endif

    if (total_matches != nxagentNumPixmapFormats)
    {
      #ifdef WARNING
      fprintf(stderr, "nxagentCheckForPixmapFormatsCompatibility: Only some [%d] of the internal "
                      "pixmap formats [%d] match with external pixmap formats [%d] at tolerance [%u].\n",
              total_matches, nxagentNumPixmapFormats, nxagentRemoteNumPixmapFormats, tolerance);
      #endif
    }
  }
  else
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentCheckForPixmapFormatsCompatibility: WARNING! Internally available "
                    "pixmap formats [%d] don't match with external pixmap formats [%d] "
                    "at tolerance [%u]. Only [%d] depth values matched.\n",
            nxagentNumPixmapFormats, nxagentRemoteNumPixmapFormats, tolerance, total_matches);
    #endif
  }

  return (ret);
}

static int nxagentInitAndCheckVisuals(int flexibility)
{
  /* FIXME: does this also need work? */

  bool compatible = true;

  long viMask = VisualScreenMask;
  XVisualInfo viTemplate = {
    .screen = DefaultScreen(nxagentDisplay),
    .depth = DefaultDepth(nxagentDisplay, DefaultScreen(nxagentDisplay)),
  };
  int viNumList;
  XVisualInfo *viList = XGetVisualInfo(nxagentDisplay, viMask, &viTemplate, &viNumList);

  XVisualInfo *newVisuals = malloc(sizeof(XVisualInfo) * nxagentNumVisuals);

  for (int i = 0; i < nxagentNumVisuals; i++)
  {
    bool matched = false;

    for (int n = 0; n < viNumList; n++)
    {
      if (nxagentCompareVisuals(nxagentVisuals[i], viList[n]) == 1)
      {
/*
FIXME: Should the visual be ignored in this case?  We can flag the
       visuals with inverted masks, and use this information to switch
       the masks when contacting the remote X server.
*/
        if (nxagentVisuals[i].red_mask == viList[n].blue_mask &&
                nxagentVisuals[i].blue_mask == viList[n].red_mask)
        {
          #ifdef WARNING
          fprintf(stderr, "nxagentInitAndCheckVisuals: WARNING! Red and blue mask inverted. "
                      "Forcing matching.\n");
          #endif
        }

        matched = true;

        nxagentVisualHasBeenIgnored[i] = FALSE;

        memcpy(newVisuals + i, viList + n, sizeof(XVisualInfo));

        break;
      }
    }

    if (!matched)
    {
      if (nxagentVisuals[i].class == DirectColor)
      {
        #ifdef WARNING
        fprintf(stderr, "nxagentInitAndCheckVisuals: WARNING! Ignoring not matched DirectColor visual.\n");
        #endif

        nxagentVisualHasBeenIgnored[i] = TRUE;

        memcpy(newVisuals + i, nxagentVisuals + i, sizeof(XVisualInfo));
      }
      else
      {
        #ifdef DEBUG
        fprintf(stderr, "nxagentInitAndCheckVisuals: WARNING! Failed to match this visual:\n");
        fprintf(stderr, "\tdepth = %d\n", nxagentVisuals[i].depth);
        fprintf(stderr, "\tclass = %d\n", nxagentVisuals[i].class);
        fprintf(stderr, "\tmask = (%ld,%ld,%ld)\n",
                    nxagentVisuals[i].red_mask,
                        nxagentVisuals[i].green_mask,
                            nxagentVisuals[i].blue_mask);
        fprintf(stderr, "\tcolormap size = %d\n", nxagentVisuals[i].colormap_size);
        fprintf(stderr, "\tbits_per_rgb = %d\n", nxagentVisuals[i].bits_per_rgb);
        #endif

        compatible = false;

        break;
      }
    }
  }

  SAFE_XFree(viList);

  if (compatible)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentInitAndCheckVisuals: New visuals match with old visuals.\n");
    #endif

    nxagentVisuals = newVisuals;
  }
  else
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentInitAndCheckVisuals: New visuals don't match with old visuals.\n");
    #endif

    SAFE_free(newVisuals);
  }

  return compatible;
}

static int nxagentCheckForColormapsCompatibility(int flexibility)
{
  /* FIXME: does this also need work? */
  if (nxagentNumDefaultColormaps == nxagentNumDefaultColormapsRecBackup)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentCheckForColormapsCompatibility: Number of new colormaps [%d] "
                "matches with old colormaps.\n", nxagentNumDefaultColormaps);
    #endif
    return 1;
  }
  else
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentCheckForColormapsCompatibility: WARNING! Number of new colormaps [%d] "
                "doesn't match with old colormaps [%d].\n", nxagentNumDefaultColormaps,
                    nxagentNumDefaultColormapsRecBackup);
    #endif
    return 0;
  }
}

Bool nxagentReconnectDisplay(void *p0)
{
  int flexibility = *(int*)p0;

  #if defined(NXAGENT_RECONNECT_DEBUG) || defined(NXAGENT_RECONNECT_DISPLAY_DEBUG)
  fprintf(stderr, "nxagentReconnectDisplay\n");
  #endif

  if (reconnectDisplayState)
  {
    fprintf(stderr, "nxagentReconnectDisplay: Trying to reconnect a session "
                "uncleanly disconnected\n");

    return False;
  }

  /*
   * Reset the values to their defaults.
   */

  nxagentPackMethod     = -1;
  nxagentPackQuality    = -1;
  nxagentSplitThreshold = -1;

  nxagentRemoteMajor = -1;

  nxagentInstallSignalHandlers();

  nxagentInstallDisplayHandlers();

  nxagentDisplay = nxagentInternalOpenDisplay(nxagentDisplayName);

  nxagentPostInstallSignalHandlers();

  nxagentPostInstallDisplayHandlers();

  if (nxagentDisplay == NULL)
  {
    nxagentSetReconnectError(FAILED_RESUME_DISPLAY_ALERT,
                                 "Couldn't open the display.");

    return False;
  }

  nxagentAddXConnection();

  /*
   * Display is now open.
   */

  reconnectDisplayState = OPENED;

  #ifdef NXAGENT_TIMESTAMP

  fprintf(stderr, "Display: Open of the display finished with time [%d] ms.\n",
          GetTimeInMillis() - startTime);

  #endif

  if (!nxagentCheckForDefaultDepthCompatibility())
  {
    nxagentSetReconnectError(FAILED_RESUME_DISPLAY_ALERT,
                                 "Default display depth doesn't match.");

    return False;
  }

  nxagentUseNXTrans = nxagentPostProcessArgs(nxagentDisplayName, nxagentDisplay,
                                                 DefaultScreenOfDisplay(nxagentDisplay));

  /*
   * After processing the arguments all the timeout values have been
   * set. Now we have to change the screen-saver timeout.
   */

  nxagentSetScreenSaverTime();

  /*
   * Init and compare the visuals.
   */

  if (!nxagentInitAndCheckVisuals(flexibility))
  {
    nxagentSetReconnectError(FAILED_RESUME_VISUALS_ALERT,
                                 "Couldn't restore the required visuals.");

    return False;
  }

  reconnectDisplayState = GOT_VISUAL_INFO;

  nxagentSetDefaultVisual();

  nxagentInitAlphaVisual();

  /*
   * Re-allocate the colormaps.
   */

  nxagentNumDefaultColormaps = nxagentNumVisuals;

  {
    Colormap * tmp = (Colormap *) realloc(nxagentDefaultColormaps,
                                              nxagentNumDefaultColormaps * sizeof(Colormap));
    if (tmp == NULL)
    {
      SAFE_free(nxagentDefaultColormaps);
      FatalError("Can't allocate memory for the default colormaps\n");
    }
    else
    {
      nxagentDefaultColormaps = tmp;
    }
  }

  reconnectDisplayState = ALLOC_DEF_COLORMAP;

  for (int i = 0; i < nxagentNumDefaultColormaps; i++)
  {
    if (nxagentVisualHasBeenIgnored[i])
    {
      nxagentDefaultColormaps[i] = (XID)0;
    }
    else
    {
      nxagentDefaultColormaps[i] = XCreateColormap(nxagentDisplay,
                                                   DefaultRootWindow(nxagentDisplay),
                                                   nxagentVisuals[i].visual,
                                                   AllocNone);
    }
  }

  nxagentCheckForColormapsCompatibility(flexibility);

  /*
   * Check the display depth.
   */

  nxagentInitDepths();

  reconnectDisplayState = GOT_DEPTH_LIST;

  if (!nxagentCheckForDepthsCompatibility())
  {
    nxagentSetReconnectError(FAILED_RESUME_DEPTHS_ALERT,
                                 "Couldn't restore all the required depths.");

    return False;
  }

  /*
   * nxagentPixmapFormats and nxagentRemotePixmapFormats
   * will be reallocated in nxagentInitPixmapFormats().
   */

  SAFE_XFree(nxagentPixmapFormats);
  SAFE_XFree(nxagentRemotePixmapFormats);

  /*
   * Check if all the required pixmap formats are supported.
   */

  nxagentInitPixmapFormats();

  if (!nxagentCheckForPixmapFormatsCompatibility())
  {
    nxagentSetReconnectError(FAILED_RESUME_PIXMAPS_ALERT,
                                 "Couldn't restore all the required pixmap formats.");

    return False;
  }

  reconnectDisplayState = GOT_PIXMAP_FORMAT_LIST;

  /*
   * Create a pixmap for each depth matching the local supported
   * formats with format available on the remote display.
   */

  nxagentSetDefaultDrawables();

  #ifdef RENDER

  if (nxagentRenderEnable)
  {
    nxagentRenderExtensionInit();
  }

  if (nxagentRenderEnableRecBackup != nxagentRenderEnable)
  {
    nxagentRenderEnable = nxagentRenderEnableRecBackup;

    nxagentSetReconnectError(FAILED_RESUME_RENDER_ALERT,
                                 "Render extension not available or incompatible version.");

    return False;
  }

  #endif

  nxagentBlackPixel = BlackPixel(nxagentDisplay, DefaultScreen(nxagentDisplay));
  nxagentWhitePixel = WhitePixel(nxagentDisplay, DefaultScreen(nxagentDisplay));

  /*
   * Initialize the agent's event mask that will be requested for the
   * root or all the top level windows. If the nested window is a
   * child of an existing window we will need to receive
   * StructureNotify events. If we are going to manage the changes in
   * root window's visibility we'll also need VisibilityChange events.
   */

  nxagentInitDefaultEventMask();

  nxagentBitmapGC = XCreateGC(nxagentDisplay, nxagentDefaultDrawables[1], 0L, NULL);

  #ifdef TEST
  fprintf(stderr, "nxagentReconnectDisplay: Going to create agent's confine window.\n");
  #endif

  nxagentOpenConfineWindow();

  useXpmIcon = nxagentMakeIcon(nxagentDisplay, &nxagentIconPixmap, &nxagentIconShape);

  /*
   * Everything went fine. We can continue handling our clients.
   */

  reconnectDisplayState = EVERYTHING_DONE;

  return True;
}

void nxagentAddXConnection(void)
{
  nxagentXConnectionNumber = XConnectionNumber(nxagentDisplay);

  #ifdef TEST
  fprintf(stderr, "nxagentAddXConnection: Adding the X connection [%d] "
              "to the device set.\n", nxagentXConnectionNumber);
  #endif

  SetNotifyFd(XConnectionNumber(nxagentDisplay), nxagentNotifyConnection, X_NOTIFY_READ, NULL);
}

void nxagentRemoveXConnection(void)
{
  #ifdef TEST
  fprintf(stderr, "nxagentRemoveXConnection: Removing the X connection [%d] "
              "from the device set.\n", nxagentXConnectionNumber);
  #endif

  RemoveNotifyFd(nxagentXConnectionNumber);
}

/*
 * Force an I/O error and wait until the NX transport is gone. It must
 * be called before suspending or terminating a session to ensure that
 * the NX transport is terminated first.
 */

void nxagentWaitDisplay(void)
{
  /*
   * Disable the smart scheduler's interrupts.
   */

  #ifdef DEBUG
  fprintf(stderr, "nxagentWaitDisplay: Stopping the smart schedule timer.\n");
  #endif

  nxagentStopTimer();

  if (nxagentDisplay != NULL)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentWaitDisplay: Going to shutdown the X connection [%d].\n",
                nxagentXConnectionNumber);
    #endif

    NXForceDisplayError(nxagentDisplay);

    XSync(nxagentDisplay, 0);
  }

  #ifdef TEST
  fprintf(stderr, "nxagentWaitDisplay: Going to wait for the NX transport.\n");
  #endif

  NXTransDestroy(NX_FD_ANY);

  #ifdef TEST
  fprintf(stderr, "nxagentWaitDisplay: The NX transport is not running.\n");
  #endif

  /*
   * Be sure the signal handlers are in a known state.
   */

  nxagentResetSignalHandlers();
}

/*
 * This has not to do with the remote display but with the X server
 * that the agent is impersonating.  We have it here to be consistent
 * with the other cleanup procedures which have mainly to do with the
 * Xlib display connection.
 */

void nxagentAbortDisplay(void)
{
  /*
   * Be sure the X server socket in .X11-unix is deleted otherwise
   * other users may to become unable to run a session on the same
   * display.
   */

  #ifdef TEST
  fprintf(stderr, "nxagentAbortDisplay: Cleaning up the X server sockets.\n");
  #endif

  CloseWellKnownConnections();
}
