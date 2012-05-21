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

#include <signal.h>

#include "X.h"
#include "Xproto.h"
#include "Xpoll.h"
#include "mi.h"
#include "fb.h"
#include "inputstr.h"

#include "Agent.h"
#include "Atoms.h"
#include "Drawable.h"
#include "Client.h"
#include "Reconnect.h"
#include "Display.h"
#include "Dialog.h"
#include "Screen.h"
#include "Windows.h"
#include "Events.h"
#include "Dialog.h"
#include "Args.h"
#include "Font.h"
#include "GCs.h"
#include "Trap.h"
#include "Keyboard.h"
#include "Composite.h"
#include "Millis.h"
#include "Splash.h"
#include "Error.h"

#ifdef XKB
#include "XKBsrv.h"
#endif

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

#define NXAGENT_RECONNECT_DEFAULT_MESSAGE_SIZE  32

extern Bool nxagentReconnectAllCursor(void*);
extern Bool nxagentReconnectAllColormap(void*);
extern Bool nxagentReconnectAllWindows(void*);
extern Bool nxagentReconnectAllGlyphSet(void*);
extern Bool nxagentReconnectAllPictFormat(void*);
extern Bool nxagentReconnectAllPicture(void*);

extern Bool nxagentDisconnectAllPicture(void);
extern Bool nxagentDisconnectAllWindows(void);
extern Bool nxagentDisconnectAllCursor(void);

extern Bool nxagentReconnectFailedFonts(void*);
extern Bool nxagentInstallFontServerPath(void);
extern Bool nxagentUninstallFontServerPath(void);

extern void nxagentRemoveXConnection(void);

extern void nxagentInitPointerMap(void);

static char *nxagentGetReconnectError(void);

void nxagentInitializeRecLossyLevel(void);

static char *nxagentReconnectErrorMessage = NULL;
static int  nxagentReconnectErrorId;

extern Bool nxagentRenderEnable;

extern char *nxagentKeyboard;

enum SESSION_STATE nxagentSessionState = SESSION_STARTING;

struct nxagentExceptionStruct nxagentException = {0, 0};

enum RECONNECTION_STEP
{
  DISPLAY_STEP = 0,
  SCREEN_STEP,
  FONT_STEP,
  PIXMAP_STEP,
  GC_STEP,
  CURSOR_STEP,
  COLORMAP_STEP,
  WINDOW_STEP,
  GLYPHSET_STEP,
  PICTFORMAT_STEP,
  PICTURE_STEP,
  STEP_NONE
};

void *reconnectLossyLevel[STEP_NONE];

static enum RECONNECTION_STEP failedStep;

int nxagentHandleConnectionStates(void)
{
  #ifdef TEST
  fprintf(stderr, "nxagentHandleConnectionStates: Handling Exception with "
              "state [%s] and transport [%d] and generation [%ld].\n",
                  DECODE_SESSION_STATE(nxagentSessionState), NXTransRunning(NX_FD_ANY), serverGeneration);
  fprintf(stderr, "nxagentHandleConnectionStates: Entering with nxagentException.sigHup = [%d], "
              "nxagentException.ioError = [%d]\n",
                  nxagentException.sigHup, nxagentException.ioError);
  #endif

  if (nxagentException.sigHup > 0)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentHandleConnectionStates: Got SIGHUP in the exception flags.\n");
    #endif

    nxagentException.sigHup = 0;

    if (nxagentSessionState == SESSION_UP)
    {
      if (nxagentOption(Persistent))
      {
        nxagentSessionState = SESSION_GOING_DOWN;

        #ifdef TEST
        fprintf(stderr, "nxagentHandleConnectionStates: Handling "
                    "signal [SIGHUP] by disconnecting the agent.\n");

        #endif

      }
      else
      {
        nxagentTerminateSession();
      }
    }
    else if (nxagentSessionState == SESSION_STARTING)
    {
      nxagentTerminateSession();

      #ifdef WARNING
      fprintf(stderr, "nxagentHandleConnectionStates: Handling signal [SIGHUP] by terminating the agent.\n");
      #endif
    }
    else if (nxagentSessionState == SESSION_DOWN &&
                 NXTransRunning(NX_FD_ANY) == 0)
    {
      nxagentSessionState = SESSION_GOING_UP;

      #ifdef TEST
      fprintf(stderr, "nxagentHandleConnectionStates: Handling signal [SIGHUP] by reconnecting the agent.\n");
      #endif
    }
    else
    {
      #ifdef TEST
      fprintf(stderr, "nxagentHandleConnectionStates: Handling signal with state [%s] and exception [%d].\n",
                  DECODE_SESSION_STATE(nxagentSessionState), dispatchException);
      #endif
    }
  }

  if (nxagentNeedConnectionChange() == 1)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentHandleConnectionStates: Calling nxagentHandleConnectionChanges "
                "with ioError [%d] sigHup [%d].\n", nxagentException.ioError, nxagentException.sigHup);
    #endif

    nxagentHandleConnectionChanges();
  }

  if (nxagentException.ioError > 0)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentHandleConnectionStates: Got I/O error in the exception flags.\n");
    #endif
/*
TODO: This should be reset only when
      the state became SESSION_DOWN.
*/
    nxagentException.ioError = 0;

    if (nxagentOption(Persistent) == 1 && nxagentSessionState != SESSION_STARTING)
    {
      if (nxagentSessionState == SESSION_UP)
      {
        if ((dispatchException & DE_TERMINATE) == 0)
        {
          fprintf(stderr, "Session: Display failure detected at '%s'.\n", GetTimeAsString());

          fprintf(stderr, "Session: Suspending session at '%s'.\n", GetTimeAsString());
        }

        nxagentDisconnectSession();
      }
      else if (nxagentSessionState == SESSION_GOING_DOWN)
      {
        #ifdef TEST
        fprintf(stderr, "nxagentHandleConnectionStates: Got I/O error with session "
                    "[SESSION_GOING_DOWN].\n");
        #endif
      }
      else if (nxagentSessionState == SESSION_GOING_UP)
      {
        #ifdef TEST
        fprintf(stderr, "nxagentHandleConnectionStates: Got I/O error with session "
                    "[SESSION_GOING_UP].\n");
        #endif

        nxagentSessionState = SESSION_GOING_DOWN;

        nxagentSetReconnectError(FAILED_RESUME_DISPLAY_BROKEN_ALERT,
                                     "Got I/O error during reconnect.");

        nxagentChangeOption(Fullscreen, False);

        return 1;
      }
      else if (nxagentSessionState == SESSION_DOWN)
      {
        #ifdef TEST
        fprintf(stderr, "nxagentHandleConnectionStates: Got I/O error with session "
                    "[SESSION_DOWN]. Ignoring.\n");
        #endif

        return 1;
      }
      else
      {
        #ifdef TEST
        fprintf(stderr, "nxagentHandleConnectionStates: Got I/O error with session "
                    "[%d].\n", nxagentSessionState);
        #endif
      }

      nxagentSessionState = SESSION_DOWN;

      if ((dispatchException & DE_TERMINATE) == 0)
      {
        #ifdef NX_DEBUG_INPUT
        fprintf(stderr, "Session: Session suspended at '%s' timestamp [%lu].\n", GetTimeAsString(), GetTimeInMillis());
        #else
        fprintf(stderr, "Session: Session suspended at '%s'.\n", GetTimeAsString());
        #endif
      }

      nxagentResetDisplayHandlers();

      return 1;
    }

    fprintf(stderr, "Info: Disconnected from display '%s'.\n", nxagentDisplayName);

    nxagentTerminateSession();

    return -1;
  }

  return 0;
}

void nxagentInitializeRecLossyLevel()
{
  *(int *)reconnectLossyLevel[DISPLAY_STEP]    = 0;
  *(int *)reconnectLossyLevel[SCREEN_STEP]     = 0;
  *(int *)reconnectLossyLevel[FONT_STEP]       = 0;
  *(int *)reconnectLossyLevel[PIXMAP_STEP]     = 0;
  *(int *)reconnectLossyLevel[GC_STEP]         = 0;
  *(int *)reconnectLossyLevel[CURSOR_STEP]     = 0;
  *(int *)reconnectLossyLevel[COLORMAP_STEP]   = 0;
  *(int *)reconnectLossyLevel[WINDOW_STEP]     = 0;
  *(int *)reconnectLossyLevel[GLYPHSET_STEP]   = 0;
  *(int *)reconnectLossyLevel[PICTFORMAT_STEP] = 0;
  *(int *)reconnectLossyLevel[PICTURE_STEP]    = 0;
}

void nxagentInitReconnector(void)
{
  nxagentReconnectTrap = 0;

  reconnectLossyLevel[DISPLAY_STEP]    = xalloc(sizeof(int));
  reconnectLossyLevel[SCREEN_STEP]     = xalloc(sizeof(int));
  reconnectLossyLevel[FONT_STEP]       = xalloc(sizeof(int));
  reconnectLossyLevel[PIXMAP_STEP]     = xalloc(sizeof(int));
  reconnectLossyLevel[GC_STEP]         = xalloc(sizeof(int));
  reconnectLossyLevel[CURSOR_STEP]     = xalloc(sizeof(int));
  reconnectLossyLevel[COLORMAP_STEP]   = xalloc(sizeof(int));
  reconnectLossyLevel[WINDOW_STEP]     = xalloc(sizeof(int));
  reconnectLossyLevel[GLYPHSET_STEP]   = xalloc(sizeof(int));
  reconnectLossyLevel[PICTFORMAT_STEP] = xalloc(sizeof(int));
  reconnectLossyLevel[PICTURE_STEP]    = xalloc(sizeof(int));
}

void nxagentDisconnectSession(void)
{
  #ifdef TEST
  fprintf(stderr, "nxagentDisconnectSession: Disconnecting session with state [%s].\n",
              DECODE_SESSION_STATE(nxagentSessionState));
  #endif

  /*
   * Force an I/O error on the display
   * and wait until the NX transport
   * is gone.
   */

  #ifdef TEST
  fprintf(stderr, "nxagentDisconnectSession: Disconnecting the X display.\n");
  #endif

  nxagentWaitDisplay();

  /*
   * Prepare for the next reconnection.
   */

  #ifdef TEST
  fprintf(stderr, "nxagentDisconnectSession: Disconnecting all X resources.\n");
  #endif

  nxagentInitializeRecLossyLevel();

  nxagentBackupDisplayInfo();

  if (nxagentOption(Rootless))
  {
    nxagentFreePropertyList();
  }

  if (nxagentRenderEnable)
  {
    nxagentDisconnectAllPicture();
  }

  nxagentEmptyAllBackingStoreRegions();

  nxagentDisconnectAllWindows();
  nxagentDisconnectAllCursor();
  nxagentDisconnectAllPixmaps();
  nxagentDisconnectAllGCs();
  nxagentDisconnectDisplay();

  nxagentWMIsRunning = 0;

  #ifdef TEST
  fprintf(stderr, "nxagentDisconnectSession: Disconnection completed. SigHup is [%d]. IoError is [%d].\n",
              nxagentException.sigHup, nxagentException.ioError);
  #endif
}

Bool nxagentReconnectSession(void)
{
  char *nxagentOldKeyboard = NULL;

  nxagentResizeDesktopAtStartup = False;

  /*
   * Propagate device settings if explicitly asked for.
   */

  nxagentChangeOption(DeviceControl, nxagentOption(DeviceControlUserDefined));

  /*
   * We need to zero out every new XID
   * created by the disconnected display.
   */

  nxagentDisconnectSession();

  /*
   * Set this in order to let the screen
   * function to behave differently at
   * reconnection time.
   */

  nxagentReconnectTrap = True;

  nxagentSetReconnectError(0, NULL);

  if (nxagentKeyboard != NULL)
  {
    int size;

    size = strlen(nxagentKeyboard);

    if ((nxagentOldKeyboard = xalloc(size + 1)) != NULL)
    {
      strncpy(nxagentOldKeyboard, nxagentKeyboard, size);

      nxagentOldKeyboard[size] = '\0';
    }
  }

  if (nxagentKeyboard)
  {
    xfree(nxagentKeyboard);

    nxagentKeyboard = NULL;
  }

  nxagentSaveOptions();

  nxagentResetOptions();

  nxagentProcessOptionsFile();

  if (nxagentReconnectDisplay(reconnectLossyLevel[DISPLAY_STEP]) == 0)
  {
    failedStep = DISPLAY_STEP;

    #ifdef TEST
    fprintf(stderr, "nxagentReconnect: WARNING! Failed display reconnection.\n");
    #endif

    goto nxagentReconnectError;
  }

  if (nxagentReconnectScreen(reconnectLossyLevel[SCREEN_STEP]) == 0)
  {
    failedStep = SCREEN_STEP;

    goto nxagentReconnectError;
  }

  nxagentDisconnectAllFonts();

  nxagentListRemoteFonts("*", nxagentMaxFontNames);

  if (nxagentReconnectAllFonts(reconnectLossyLevel[FONT_STEP]) == 0)
  {
    if (nxagentReconnectFailedFonts(reconnectLossyLevel[FONT_STEP]) == 0)
    {
      failedStep = FONT_STEP;

      goto nxagentReconnectError;
    }
    else
    {
      #ifdef WARNING
      fprintf(stderr, "nxagentReconnect: WARNING! Unable to retrieve all the fonts currently in use. "
                  "Missing fonts have been replaced.\n");
      #endif

      nxagentLaunchDialog(DIALOG_FONT_REPLACEMENT);
    }
  }

  /*
   * Map the main window and send a
   * SetSelectionOwner request to
   * notify of the agent start.
   */

  nxagentMapDefaultWindows();

  /*
   * Ensure that the SetSelectionOwner
   * request is sent through the link.
   */

  XFlush(nxagentDisplay);

  NXTransContinue(NULL);

  nxagentEmptyBSPixmapList();

  if (nxagentReconnectAllPixmaps(reconnectLossyLevel[PIXMAP_STEP]) == 0)
  {
    failedStep = PIXMAP_STEP;

    goto nxagentReconnectError;
  }

  if (nxagentReconnectAllGCs(reconnectLossyLevel[GC_STEP]) == 0)
  {
    failedStep = GC_STEP;

    goto nxagentReconnectError;
  }

  if (nxagentReconnectAllColormap(reconnectLossyLevel[COLORMAP_STEP]) == 0)
  {
    failedStep = COLORMAP_STEP;

    goto nxagentReconnectError;
  }

  if (nxagentReconnectAllWindows(reconnectLossyLevel[WINDOW_STEP]) == 0)
  {
    failedStep = WINDOW_STEP;

    goto nxagentReconnectError;
  }

  if (nxagentRenderEnable)
  {
    if (nxagentReconnectAllGlyphSet(reconnectLossyLevel[GLYPHSET_STEP]) == 0)
    {
      failedStep = GLYPHSET_STEP;

      goto nxagentReconnectError;
    }

    if (nxagentReconnectAllPictFormat(reconnectLossyLevel[PICTFORMAT_STEP]) == 0)
    {
      failedStep = PICTFORMAT_STEP;

      goto nxagentReconnectError;
    }

    if (nxagentReconnectAllPicture(reconnectLossyLevel[PICTURE_STEP]) == 0)
    {
      failedStep = PICTURE_STEP;

      goto nxagentReconnectError;
    }
  }

  if (nxagentReconnectAllCursor(reconnectLossyLevel[CURSOR_STEP]) == 0)
  {
    failedStep = CURSOR_STEP;

    goto nxagentReconnectError;
  }

  if (nxagentSetWindowCursors(reconnectLossyLevel[WINDOW_STEP]) == 0)
  {
    failedStep = WINDOW_STEP;

    goto nxagentReconnectError;
  }

  if (nxagentOption(ResetKeyboardAtResume) == 1 &&
         (nxagentKeyboard  == NULL || nxagentOldKeyboard == NULL ||
             strcmp(nxagentKeyboard, nxagentOldKeyboard) != 0 ||
                 strcmp(nxagentKeyboard, "query") == 0))
  {
    if (nxagentResetKeyboard() == 0)
    {
      #ifdef WARNING
      if (nxagentVerbose == 1)
      {
        fprintf(stderr, "nxagentReconnect: Failed to reset keyboard device.\n");
      }
      #endif

      failedStep = WINDOW_STEP;

      goto nxagentReconnectError;
    }
  }
  else
  {
    nxagentResetKeycodeConversion();
  }

  nxagentXkbState.Initialized = 0;

  if (nxagentOldKeyboard != NULL)
  {
    xfree(nxagentOldKeyboard);

    nxagentOldKeyboard = NULL;
  }

  nxagentInitPointerMap();

  nxagentDeactivatePointerGrab();

  nxagentWakeupByReconnect();

  nxagentFreeGCList();

  nxagentRedirectDefaultWindows();

  if (nxagentResizeDesktopAtStartup || nxagentOption(Rootless) == True)
  {
    nxagentChangeScreenConfig(0, nxagentOption(RootWidth),
                                  nxagentOption(RootHeight), 0, 0);

    nxagentResizeDesktopAtStartup = False;
  }

  nxagentReconnectTrap = False;

  nxagentExposeArrayIsInitialized = False;

  if (nxagentSessionState != SESSION_GOING_UP)
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentReconnect: WARNING! Unexpected session state [%s] while reconnecting.\n",
                DECODE_SESSION_STATE(nxagentSessionState));
    #endif

    goto nxagentReconnectError;
  }

  #ifdef NX_DEBUG_INPUT
  fprintf(stderr, "Session: Session resumed at '%s' timestamp [%lu].\n", GetTimeAsString(), GetTimeInMillis());
  #else
  fprintf(stderr, "Session: Session resumed at '%s'.\n", GetTimeAsString());
  #endif

  nxagentRemoveSplashWindow(NULL);

  /*
   * We let the proxy flush the link on our behalf
   * after having opened the display. We are now
   * entering again the dispatcher so can flush
   * the link explicitly.
   */

  #ifdef TEST
  fprintf(stderr, "nxagentReconnect: Setting the NX flush policy to deferred.\n");
  #endif

  NXSetDisplayPolicy(nxagentDisplay, NXPolicyDeferred);

  nxagentCleanupBackupDisplayInfo();

  return 1;

nxagentReconnectError:

  if (failedStep == DISPLAY_STEP)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentReconnect: Reconnection failed in display step. Restoring options.\n");
    #endif

    nxagentRestoreOptions();
  }
  else
  {
    nxagentCleanupBackupDisplayInfo();
  }

  if (*nxagentGetReconnectError() == '\0')
  {
    #ifdef WARNING
    if (nxagentVerbose == 1)
    {
      fprintf(stderr, "nxagentReconnect: WARNING! The reconnect error message is not set. Failed step is [%d].\n",
                  failedStep);
    }
    #endif

    #ifdef TEST
    fprintf(stderr, "nxagentReconnect: Reconnection failed due to a display error.\n");
    #endif
  }
  else
  {
    #ifdef TEST
    fprintf(stderr, "nxagentReconnect: Reconnection failed with reason '%s'\n",
                nxagentGetReconnectError());
    #endif
  }

  if (NXDisplayError(nxagentDisplay) == 0)
  {
    nxagentUnmapWindows();

    nxagentFailedReconnectionDialog(nxagentReconnectErrorId, nxagentGetReconnectError());
  }
  #ifdef TEST
  else
  {
    fprintf(stderr, "nxagentReconnect: Cannot launch the dialog without a valid display.\n");
  }
  #endif

  if (failedStep == FONT_STEP)
  {
    *((int *) reconnectLossyLevel[FONT_STEP]) = 1;
  }

  if (nxagentDisplay == NULL)
  {
    nxagentDisconnectDisplay();
  }

  if (nxagentOldKeyboard != NULL)
  {
    xfree(nxagentOldKeyboard);

    nxagentOldKeyboard = NULL;
  }

  return 0;
}

void nxagentSetReconnectError(int id, char *format, ...)
{
  static int size = 0;

  va_list ap;
  int n;

  if (format == NULL)
  {
    nxagentSetReconnectError(id, "");

    return;
  }

  nxagentReconnectErrorId = id;

  while (1)
  {
    va_start (ap, format);

    n = vsnprintf(nxagentReconnectErrorMessage, size, format, ap);

    va_end(ap);

    if (n > -1 && n < size)
    {
      break;
    }
    if (n > -1)
    {
      size = n + 1;
    }
    else
    {
      /*
       * The vsnprintf() in glibc 2.0.6 would return
       * -1 when the output was truncated. See section
       * NOTES on printf(3).
       */

      size = (size ? size * 2 : NXAGENT_RECONNECT_DEFAULT_MESSAGE_SIZE);
    }

    nxagentReconnectErrorMessage = realloc(nxagentReconnectErrorMessage, size);

    if (nxagentReconnectErrorMessage == NULL)
    {
      FatalError("realloc failed");
    }
  }

  return;
}

static char* nxagentGetReconnectError()
{
  if (nxagentReconnectErrorMessage == NULL)
  {
    nxagentSetReconnectError(nxagentReconnectErrorId, "");
  }

  return nxagentReconnectErrorMessage;
}

void nxagentHandleConnectionChanges()
{
  #ifdef TEST
  fprintf(stderr, "nxagentHandleConnectionChanges: Called.\n");
  #endif

  if (nxagentSessionState == SESSION_GOING_DOWN)
  {
    fprintf(stderr, "Session: Suspending session at '%s'.\n", GetTimeAsString());

    nxagentDisconnectSession();
  }
  else if (nxagentSessionState == SESSION_GOING_UP)
  {
    fprintf(stderr, "Session: Resuming session at '%s'.\n", GetTimeAsString());

    if (nxagentReconnectSession())
    {
      nxagentSessionState = SESSION_UP;
    }
    else
    {
      nxagentSessionState = SESSION_GOING_DOWN;

      fprintf(stderr, "Session: Display failure detected at '%s'.\n", GetTimeAsString());

      fprintf(stderr, "Session: Suspending session at '%s'.\n", GetTimeAsString());

      nxagentDisconnectSession();
    }
  }
}

