/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2011 NoMachine, http://www.nomachine.com/.         */
/*                                                                        */
/* NXAGENT, NX protocol compression and NX extensions to this software    */
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
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "scrnintstr.h"
#include "Agent.h"

#include <X11/Xlib.h>

#include "opaque.h"

#include "Args.h"
#include "Display.h"
#include "Dialog.h"

#include "NX.h"
#include "NXlib.h"
#include "NXalert.h"

/*
 * Set here the required log level.
 */

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

int nxagentKillDialogPid = 0;
int nxagentSuspendDialogPid = 0;
int nxagentRootlessDialogPid = 0;
int nxagentPulldownDialogPid = 0;
int nxagentFontsReplacementDialogPid = 0;
int nxagentEnableRandRModeDialogPid = 0;
int nxagentDisableRandRModeDialogPid = 0;
int nxagentEnableDeferModePid = 0;
int nxagentDisableDeferModePid = 0;
int nxagentDisableXkbPid = 0;

static int nxagentFailedReconnectionDialogPid = 0;

char nxagentPulldownWindow[16];

char nxagentFailedReconnectionMessage[256];

void nxagentResetDialog(int pid)
{
  if (pid == nxagentRootlessDialogPid)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentResetDialog: Resetting rootless dialog pid [%d].\n", nxagentRootlessDialogPid);
    #endif

    nxagentRootlessDialogPid = 0;
  }
  else if (pid == nxagentPulldownDialogPid)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentResetDialog: Resetting pulldown dialog pid [%d].\n", nxagentPulldownDialogPid);
    #endif

    nxagentPulldownDialogPid = 0;
  }
  else if (pid == nxagentFontsReplacementDialogPid)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentFontsReplacementDialog: Resetting fonts replacement dialog pid [%d].\n", 
                nxagentFontsReplacementDialogPid);
    #endif

    nxagentFontsReplacementDialogPid = 0;
  }
  else if (pid == nxagentKillDialogPid)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentResetDialog: Resetting kill dialog pid [%d].\n", nxagentKillDialogPid);
    #endif

    nxagentKillDialogPid = 0;
  }
  else if (pid == nxagentSuspendDialogPid)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentResetDialog: Resetting suspend dialog pid [%d].\n", nxagentSuspendDialogPid);
    #endif

    nxagentSuspendDialogPid = 0;
  }
  else if (pid == nxagentFailedReconnectionDialogPid)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentResetDialog: Resetting Failed Reconnection dialog pid [%d].\n",
                nxagentFailedReconnectionDialogPid);
    #endif

    nxagentFailedReconnectionDialogPid = 0;
  }
  else if (pid == nxagentEnableRandRModeDialogPid)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentResetDialog: Resetting RandR mode dialog pid [%d].\n",
                nxagentEnableRandRModeDialogPid);
    #endif

    nxagentEnableRandRModeDialogPid = 0;
  }
  else if (pid == nxagentDisableRandRModeDialogPid)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentResetDialog: Resetting NoRandR mode dialog pid [%d].\n",
                nxagentDisableRandRModeDialogPid);
    #endif

    nxagentDisableRandRModeDialogPid = 0;
  }
  else if (pid == nxagentEnableDeferModePid)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentResetDialog: Resetting enable defer mode dialog pid [%d].\n",
                nxagentEnableDeferModePid);
    #endif

    nxagentEnableDeferModePid = 0;
  }
  else if (pid == nxagentDisableDeferModePid)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentResetDialog: Resetting disable defer mode dialog pid [%d].\n",
                nxagentDisableDeferModePid);
    #endif

    nxagentDisableDeferModePid = 0;
  }
  else if (pid == nxagentDisableXkbPid)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentResetDialog: Resetting disable XKB dialog pid [%d].\n",
                nxagentDisableXkbPid);
    #endif

    nxagentDisableXkbPid = 0;
  }
}

void nxagentLaunchDialog(DialogType dialogType)
{
  char dialogDisplay[256];
  sigset_t set, oldSet;
  int *pid;
  char *type;
  char *message;
  int local;
  const char *window = NULL;

  switch (dialogType)
  {
    case DIALOG_KILL_SESSION:
    {
      message = DIALOG_KILL_SESSION_MESSAGE;
      type = DIALOG_KILL_SESSION_TYPE;
      local = DIALOG_KILL_SESSION_LOCAL;
      pid = &nxagentKillDialogPid;

      break;
    }
    case DIALOG_SUSPEND_SESSION:
    {
      message = DIALOG_SUSPEND_SESSION_MESSAGE;
      type = DIALOG_SUSPEND_SESSION_TYPE;
      local = DIALOG_SUSPEND_SESSION_LOCAL;
      pid = &nxagentSuspendDialogPid;

      break;
    }
    case DIALOG_ROOTLESS:
    {
      message = DIALOG_ROOTLESS_MESSAGE;
      type = DIALOG_ROOTLESS_TYPE;
      local = DIALOG_ROOTLESS_LOCAL;
      pid = &nxagentRootlessDialogPid;

      break;
    }
    case DIALOG_PULLDOWN:
    {
      message = DIALOG_PULLDOWN_MESSAGE;
      type = DIALOG_PULLDOWN_TYPE;
      local = DIALOG_PULLDOWN_LOCAL;
      pid = &nxagentPulldownDialogPid;
      window = nxagentPulldownWindow;

      break;
    }
    case DIALOG_FONT_REPLACEMENT:
    {
      message = DIALOG_FONT_REPLACEMENT_MESSAGE;
      type = DIALOG_FONT_REPLACEMENT_TYPE;
      local = DIALOG_FONT_REPLACEMENT_LOCAL;
      pid = &nxagentFontsReplacementDialogPid;

      break;
    }
    case DIALOG_FAILED_RECONNECTION:
    {
      message = DIALOG_FAILED_RECONNECTION_MESSAGE;
      type = DIALOG_FAILED_RECONNECTION_TYPE;
      local = DIALOG_FAILED_RECONNECTION_LOCAL;
      pid = &nxagentFailedReconnectionDialogPid;

      break;
    }
    case DIALOG_ENABLE_DESKTOP_RESIZE_MODE:
    {
      message = DIALOG_ENABLE_DESKTOP_RESIZE_MODE_MESSAGE;
      type = DIALOG_ENABLE_DESKTOP_RESIZE_MODE_TYPE;
      local = DIALOG_ENABLE_DESKTOP_RESIZE_MODE_LOCAL;
      pid = &nxagentEnableRandRModeDialogPid;

      break;
    }
    case DIALOG_DISABLE_DESKTOP_RESIZE_MODE:
    {
      message = DIALOG_DISABLE_DESKTOP_RESIZE_MODE_MESSAGE;
      type = DIALOG_DISABLE_DESKTOP_RESIZE_MODE_TYPE;
      local = DIALOG_DISABLE_DESKTOP_RESIZE_MODE_LOCAL;
      pid = &nxagentDisableRandRModeDialogPid;

      break;
    }
    case DIALOG_ENABLE_DEFER_MODE:
    {
      message = DIALOG_ENABLE_DEFER_MODE_MESSAGE;
      type = DIALOG_ENABLE_DEFER_MODE_TYPE;
      local = DIALOG_ENABLE_DEFER_MODE_LOCAL;
      pid = &nxagentEnableDeferModePid;

      break;
    }
    case DIALOG_DISABLE_DEFER_MODE:
    {
      message = DIALOG_DISABLE_DEFER_MODE_MESSAGE;
      type = DIALOG_DISABLE_DEFER_MODE_TYPE;
      local = DIALOG_DISABLE_DEFER_MODE_LOCAL;
      pid = &nxagentDisableDeferModePid;

      break;
    }
    case DIALOG_DISABLE_XKB:
    {
      message = DIALOG_DISABLE_XKB_MESSAGE;
      type = DIALOG_DISABLE_XKB_TYPE;
      local = DIALOG_DISABLE_XKB_LOCAL;
      pid = &nxagentDisableXkbPid;

      break;
    }
    default:
    {
      #ifdef WARNING
      fprintf(stderr, "nxagentLaunchDialog: Unknown Dialog type [%d].\n", dialogType);
      #endif

      return;
    }
  }

  #ifdef TEST
  fprintf(stderr, "nxagentLaunchDialog: Launching dialog type [%d] message [%s].\n", type, message);
  #endif

  if (dialogType == DIALOG_FAILED_RECONNECTION)
  {
    strncpy(dialogDisplay, nxagentDisplayName, 255);
  }
  else
  {
    strcpy(dialogDisplay, ":");
    strncat(dialogDisplay, display, 254);
  }

  *(dialogDisplay + 255) = '\0';

  /*
   * We don't want to receive SIGCHLD
   * before we store the child pid.
   */

  sigemptyset(&set);

  sigaddset(&set, SIGCHLD);

  sigprocmask(SIG_BLOCK, &set, &oldSet);

  *pid = NXTransDialog(nxagentDialogName, message, window,
                           type, local, dialogDisplay);

  #ifdef TEST
  fprintf(stderr, "nxagentLaunchDialog: Launched dialog %s with pid [%d] on display %s.\n",
              DECODE_DIALOG_TYPE(dialogType), *pid, dialogDisplay);
  #endif

  *dialogDisplay = '\0';

  /*
   * Restore the previous set of
   * blocked signal. 
   */

  sigprocmask(SIG_SETMASK, &oldSet, NULL);
}

void nxagentPulldownDialog(Window wid)
{
  snprintf(nxagentPulldownWindow, 15, "%ld", (long int) wid);
  nxagentPulldownWindow[15] = 0;

  #ifdef TEST
  fprintf(stderr, "nxagentPulldownDialog: Going to launch pulldown "
              "dialog on window [%s].\n", nxagentPulldownWindow);
  #endif

  nxagentLaunchDialog(DIALOG_PULLDOWN);

  nxagentPulldownWindow[0] = 0;
}

void nxagentFailedReconnectionDialog(int alert, char *error)
{
  if (alert == 0)
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentFailedReconnectionDialog: WARNING! No valid alert provided. "
                "Using the default.\n");
    #endif

    alert = FAILED_RESUME_DISPLAY_ALERT;
  }

  if (NXDisplayError(nxagentDisplay) == 0 &&
          NXTransRunning(NX_FD_ANY) == 1)
  {
    NXTransAlert(alert, NX_ALERT_REMOTE);

    /*
     * Make it possible to interrupt the
     * loop with a signal.
     */

    while (NXDisplayError(nxagentDisplay) == 0 &&
               NXTransRunning(NX_FD_ANY) == 1)
    {
      struct timeval timeout;

      timeout.tv_sec  = 30;
      timeout.tv_usec = 0;

      NXTransContinue(&timeout);
    }
  }
  else
  {
    int pid;
    int status;
    int options = 0;

    snprintf(nxagentFailedReconnectionMessage, 255, "Reconnection failed: %s", error);

    *(nxagentFailedReconnectionMessage + 255) = '\0';

    nxagentLaunchDialog(DIALOG_FAILED_RECONNECTION);

    while ((pid = waitpid(nxagentFailedReconnectionDialogPid, &status, options)) == -1 &&
               errno == EINTR);

    if (pid == -1)
    {
      if (errno == ECHILD)
      {
        #ifdef WARNING
        fprintf(stderr, "nxagentFailedReconnectionDialog: Got ECHILD waiting for child [%d].\n",
                    nxagentFailedReconnectionDialogPid);
        #endif

        nxagentFailedReconnectionDialogPid = 0;
      }
      else
      {
        fprintf(stderr, "nxagentFailedReconnectionDialog: PANIC! Got unexpected error [%s] waiting "
                    "for child [%d].\n", strerror(errno), nxagentFailedReconnectionDialogPid);
      }
    }
    else if (pid > 0)
    {
      if (WIFSTOPPED(status))
      {
        #ifdef WARNING
        fprintf(stderr, "nxagentFailedReconnectionDialog: Child process [%d] was stopped "
                    "with signal [%d].\n", pid, (WSTOPSIG(status)));
        #endif
      }
      else
      {
        #ifdef WARNING

        if (WIFEXITED(status))
        {
          fprintf(stderr, "nxagentFailedReconnectionDialog: Child process [%d] exited "
                      "with status [%d].\n", pid, (WEXITSTATUS(status)));
        }
        else if (WIFSIGNALED(status))
        {
          fprintf(stderr, "nxagentFailedReconnectionDialog: Child process [%d] died "
                      "because of signal [%d].\n", pid, (WTERMSIG(status)));
        }

        #endif

        nxagentResetDialog(pid);
      }
    }
    #ifdef WARNING
    else if (pid == 0)
    {
      fprintf(stderr, "nxagentFailedReconnectionDialog: No own child process exited.\n");
    }
    #endif
  }
}

void nxagentTerminateDialog(DialogType type)
{
  int pid;

  switch (type)
  {
    case DIALOG_KILL_SESSION:
    {
      pid = nxagentKillDialogPid;

      break;
    }
    case DIALOG_SUSPEND_SESSION:
    {
      pid = nxagentSuspendDialogPid;

      break;
    }
    case DIALOG_ROOTLESS:
    {
      pid = nxagentRootlessDialogPid;

      break;
    }
    case DIALOG_PULLDOWN:
    {
      pid = nxagentPulldownDialogPid;

      break;
    }
    case DIALOG_FONT_REPLACEMENT:
    {
      pid = nxagentFontsReplacementDialogPid;

      break;
    }
    case DIALOG_FAILED_RECONNECTION:
    {
      pid = nxagentFailedReconnectionDialogPid;

      break;
    }
    case DIALOG_ENABLE_DESKTOP_RESIZE_MODE:
    {
      pid = nxagentEnableRandRModeDialogPid;

      break;
    }
    case DIALOG_DISABLE_DESKTOP_RESIZE_MODE:
    {
      pid = nxagentDisableRandRModeDialogPid;

      break;
    }
    case DIALOG_ENABLE_DEFER_MODE:
    {
      pid = nxagentEnableDeferModePid;

      break;
    }
    case DIALOG_DISABLE_DEFER_MODE:
    {
      pid = nxagentDisableDeferModePid;

      break;
    }
    case DIALOG_DISABLE_XKB:
    {
      pid = nxagentDisableXkbPid;

      break;
    }
    default:
    {
      #ifdef WARNING
      fprintf(stderr, "nxagentTerminateDialog: Unknown dialog type [%d].\n", type);
      #endif

      return;
    }
  }

  if (pid > 0)
  {
    if (kill(pid, SIGTERM) == -1)
    {
      #ifdef WARNING
      fprintf(stderr, "nxagentTerminateDialog: Failed to terminate dialog pid [%d]: %s.\n",
                  pid, strerror(errno));
      #endif
    }
  }
  #ifdef DEBUG
  else
  {
    fprintf(stderr, "nxagentTerminateDialog: Dialog type [%d] is not running.\n",
                type);
  }
  #endif
}

void nxagentTerminateDialogs()
{
  DialogType type;

  #ifdef DEBUG
  fprintf(stderr, "nxagentTerminateDialogs: Terminating all the running dialogs.\n");
  #endif

  for (type = DIALOG_FIRST_TAG; type < DIALOG_LAST_TAG; type++)
  {
    nxagentTerminateDialog(type);
  }
}
