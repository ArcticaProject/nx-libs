/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2011 NoMachine (http://www.nomachine.com)          */
/* Copyright (c) 2008-2014 Oleksandr Shneyder <o.shneyder@phoca-gmbh.de>  */
/* Copyright (c) 2011-2016 Mike Gabriel <mike.gabriel@das-netzwerkteam.de>*/
/* Copyright (c) 2014-2016 Mihai Moldovan <ionic@ionic.de>                */
/* Copyright (c) 2014-2016 Ulrich Sibiller <uli42@gmx.de>                 */
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

#include <sys/types.h>
#include <unistd.h>
#include <stdarg.h>
#include <signal.h>

#include "X.h"
#include "Xproto.h"
#include "screenint.h"
#include "input.h"
#include "inputstr.h"
#include "misc.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "servermd.h"
#include "mi.h"
#include <X11/fonts/fontstruct.h>
#include "dixfontstr.h"
#include "dixstruct.h"

#include "Agent.h"
#include "Display.h"
#include "Screen.h"
#include "Pointer.h"
#include "Keyboard.h"
#include "Handlers.h"
#include "Events.h"
#include "Init.h"
#include "Args.h"
#include "Client.h"
#include "Options.h"
#include "Drawable.h"
#include "Pixmaps.h"
#include "GCs.h"
#include "Font.h"
#include "Millis.h"
#include "Error.h"
#include "Keystroke.h"
#include "Atoms.h"
#include "Client.h"

#include <nx/NX.h>
#include "compext/Compext.h"
#include "Reconnect.h"
/*
 * Set here the required log level.
 */

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG
#undef  DUMP

/*
 * ProcVector array defined in tables.c.
 */
extern int (*ProcVector[256])(ClientPtr);

/*
 * From the fb code.
 */
extern int fbGCPrivateIndex;

/*
 * Our error logging function.
 */
void OsVendorVErrorFFunction(const char *f, va_list args);

/*
 * True if this is a fatal error.
 */
extern int OsVendorVErrorFFatal;

/*
 * Redirect the error output to a different file
 */
extern void (*OsVendorStartRedirectErrorFProc)();
extern void (*OsVendorEndRedirectErrorFProc)();

extern void SetVendorRelease(int release);

void OsVendorStartRedirectErrorFFunction();
void OsVendorEndRedirectErrorFFunction();

/*
 * Called by InitGlobals() in the new X server tree.
 */
static void nxagentGrabServerCallback(CallbackListPtr *callbacks, void *data,
                                   void *args);

#ifdef NXAGENT_CLIPBOARD
extern void nxagentSetSelectionCallback(CallbackListPtr *callbacks, void *data,
                                   void *args);
#endif

OsTimerPtr nxagentTimeoutTimer = NULL;

extern const char *nxagentProgName;

void ddxInitGlobals(void)
{
  /*
   * Install our error logging function.
   */

  OsVendorVErrorFProc = OsVendorVErrorFFunction;

  OsVendorStartRedirectErrorFProc = OsVendorStartRedirectErrorFFunction;
  OsVendorEndRedirectErrorFProc   = OsVendorEndRedirectErrorFFunction;
}

/*
 * Set if the remote display supports backing store.
 */
/*
FIXME: These, if not removed, should at least be moved to Display.h
       and Display.c.
*/
int nxagentBackingStore;
Bool nxagentSaveUnder;

/*
 * This is true at startup and set to the value of
 * nxagentFullGeneration at the end of InitInput.
 *
 *   InitOutput
 *      nxagentOpenDisplay (if nxagentDoFullGeneration)
 *         nxagentCloseDisplay (if (nxagentDoFullGeneration && nxagentDisplay))
 *            nxagentFree*
 *      nxagentListRemoteFonts
 *      AddScreen
 *         nxagentOpenScreen
 *   InitInput
 */
Bool nxagentDoFullGeneration = True;

#ifdef X2GO
/*
 * True if agent is running as X2goAgent
 * False if agent is running as NXAgent
 */
Bool nxagentX2go;

/*
 * Check if agent is X2goAgent
 */
void checkX2goAgent(int argc, char * argv[])
{
  #ifdef TEST
  fprintf(stderr, "%s: nxagentProgName [%s]\n", __func__, nxagentProgName);
  #endif

  nxagentX2go = False;

  if (strcasecmp(nxagentProgName,"x2goagent") == 0)
  {
    nxagentX2go = True;
  }
  else
  {
    char *envDisplay = getenv("DISPLAY");

    /* x2go DISPLAY variable contains ".x2go" */
    if (envDisplay && strstr(envDisplay, ".x2go-"))
    {
      nxagentX2go = True;
    }
    else
    {
      for (int i = 1; i < argc; i++)
      {
        /* x2go session names are passed with -name and start with "X2GO-" */
        if (argv[i] && strncmp(argv[i], "-name", 5)  == 0)
        {
          if (i < argc - 1)
          {
            if (strstr(argv[i+1], "X2GO-"))
            {
              nxagentX2go = True;
              break;
            }
          }
        }
      }
    }
  }

  if (nxagentX2go)
    fprintf(stderr, "\nrunning as X2Go Agent\n");
}


#endif

/*
 * Called at X server's initialization.
 */
void InitOutput(ScreenInfo *scrInfo, int argc, char *argv[])
{
  /*
   * Print our pid and version information.
   */

  if (serverGeneration <= 1)
  {
    fprintf(stderr, "\nNXAGENT - Version " NX_VERSION_CURRENT_STRING "\n\n");
    fprintf(stderr, "Copyright (c) 2001, 2011 NoMachine (http://www.nomachine.com)\n");
    fprintf(stderr, "Copyright (c) 2008-2014 Oleksandr Shneyder <o.shneyder@phoca-gmbh.de>\n");
    fprintf(stderr, "Copyright (c) 2011-2016 Mike Gabriel <mike.gabriel@das-netzwerkteam.de>\n");
    fprintf(stderr, "Copyright (c) 2014-2016 Ulrich Sibiller <uli42@gmx.de>\n");
    fprintf(stderr, "Copyright (c) 2014-2016 Mihai Moldovan <ionic@ionic.de>\n");
    fprintf(stderr, "Copyright (c) 2015-2016 Qindel Group (http://www.qindel.com)\n");
    fprintf(stderr, "See https://github.com/ArcticaProject/nx-libs for more information.\n\n");

    fprintf(stderr, "Info: Agent running with pid '%d'.\n", getpid());

    fprintf(stderr, "Session: Starting session at '%s'.\n", GetTimeAsString());
    saveAgentState("STARTING");
  }

  /*
   * Avoid slowness due to buggy_repeat workaround in libcairo
   * versions >= 1.10.
   */

  SetVendorRelease(70000000);

  /*
   * Init the time count for image rate.
   */

  if (nxagentOption(ImageRateLimit) != 0)
  { 
    fprintf(stderr, "Info: Image rate limit set to %u kB/s.\n", nxagentOption(ImageRateLimit));
  }

  /*
   * Unset the LD_LIBRARY_PATH variable in Popen() before calling
   * execl() in the child process.
   */

  NXUnsetLibraryPath(1);

  if (serverGeneration == 1)
  {
    AddCallback(&ServerGrabCallback, nxagentGrabServerCallback, NULL);
  }

  if (!nxagentUserDefinedFontPath)
  {
    #ifdef TEST
    fprintf(stderr, "InitOutput: Calling nxagentVerifyDefaultFontPath.\n");
    #endif

    nxagentVerifyDefaultFontPath();
  }
  #ifdef TEST
  else
  {
    fprintf(stderr, "InitOutput: User defined font path. Skipping the check on the fonts dir.\n");
  }
  #endif

  char *authority = getenv("NX_XAUTHORITY");

  if (authority)
  {
    #ifdef __sun

    char *environment = malloc(15 + strlen(authority));

    sprintf(environment, "XAUTHORITY=%s", authority);

    if (putenv(environment) < 0)

    #else

    if (setenv("XAUTHORITY", authority, True) < 0)

    #endif

    {
      fprintf(stderr, "Warning: Couldn't set the XAUTHORITY environment to [%s]\n",
                  authority);
    }
  }

  nxagentInitBSPixmapList();

  /*
   * Open the display. We are at the early startup and the information
   * we'll get from the remote X server will mandate some of the
   * characteristics of the session, like the screen depth. Note that
   * this reliance on the remote display at session startup should be
   * removed. We should always operate at 32 bpp, internally, and do
   * the required translations as soon as the graphic operation needs
   * to be realized on the remote display.
   */

  nxagentOpenDisplay(argc, argv);

/*
FIXME: These variables, if not removed at all because have probably
       become useless, should be moved to Display.h and Display.c.
*/
  nxagentBackingStore = XDoesBackingStore(DefaultScreenOfDisplay(nxagentDisplay));

  #ifdef TEST
  fprintf(stderr, "InitOutput: Remote display backing store support [%d].\n",
              nxagentBackingStore);
  #endif

  nxagentSaveUnder = XDoesSaveUnders(DefaultScreenOfDisplay(nxagentDisplay));

  #ifdef TEST
  fprintf(stderr, "InitOutput: Remote display save under support [%d].\n",
              nxagentSaveUnder);
  #endif

  /*
   * Initialize the basic screen info.
   */

  nxagentSetScreenInfo(scrInfo);

  /*
   * Initialize pixmap formats for this screen.
   */

  nxagentSetPixmapFormats(scrInfo);

  /*
   * Get our own privates' index.
   */

  nxagentWindowPrivateIndex = AllocateWindowPrivateIndex();
  nxagentGCPrivateIndex = AllocateGCPrivateIndex();
  RT_NX_GC = CreateNewResourceType(nxagentDestroyNewGCResourceType);
#ifdef HAS_XFONT2
  nxagentFontPrivateIndex = xfont2_allocate_font_private_index();
#else
  nxagentFontPrivateIndex = AllocateFontPrivateIndex();
#endif /* HAS_XFONT2 */
  RT_NX_FONT = CreateNewResourceType(nxagentDestroyNewFontResourceType); 
  nxagentClientPrivateIndex = AllocateClientPrivateIndex();
  nxagentPixmapPrivateIndex = AllocatePixmapPrivateIndex();
  RT_NX_PIXMAP = CreateNewResourceType(nxagentDestroyNewPixmapResourceType); 

  RT_NX_CORR_BACKGROUND = CreateNewResourceType(nxagentDestroyCorruptedBackgroundResource);
  RT_NX_CORR_WINDOW = CreateNewResourceType(nxagentDestroyCorruptedWindowResource);
  RT_NX_CORR_PIXMAP = CreateNewResourceType(nxagentDestroyCorruptedPixmapResource);

  fbGCPrivateIndex = AllocateGCPrivateIndex();

  if (nxagentNumScreens == 0)
  {
    nxagentNumScreens = 1;
  }

  for (int i = 0; i < nxagentNumScreens; i++)
  {
    AddScreen(nxagentOpenScreen, argc, argv);
  }

  nxagentNumScreens = scrInfo->numScreens;

  /*
   * Initialize the GCs used by the synchronization put images. We do
   * it here because we use the nxagentDefaultScreen.
   */

  nxagentAllocateGraphicContexts();

  nxagentDoFullGeneration = nxagentFullGeneration;

  /*
   * Use a solid black root window background.
   */

  if (!whiteRoot)
    blackRoot = TRUE;

  nxagentInitKeystrokes(False);

#ifdef NXAGENT_CLIPBOARD
  /* FIXME: we need to call DeleteCallback at shutdown, but where? */
  AddCallback(&SelectionCallback, nxagentSetSelectionCallback, NULL);
#endif

  /* FIXME: we need to call DeleteCallback at shutdown, but where? */
  AddCallback(&ClientStateCallback, nxagentClientStateCallback, NULL);

  nxagentInitAtoms();
}

void nxagentNotifyConnection(int fd, int ready, void *data)
{
    nxagentDispatchEvents(NULL);
}

void InitInput(int argc, char *argv[])
{
  void *ptr = AddInputDevice(nxagentPointerProc, True);
  void *kbd = AddInputDevice(nxagentKeyboardProc, True);

  RegisterPointerDevice(ptr);
  RegisterKeyboardDevice(kbd);

  mieqInit(kbd, ptr);

  /*
   * Add the display descriptor to the set of descriptors awaited by
   * the dispatcher.
   */

  nxagentAddXConnection();

  if (nxagentOption(Shadow))
  {
    RegisterBlockAndWakeupHandlers(nxagentShadowBlockHandler, nxagentShadowWakeupHandler, NULL);
  }
  else
  {
    RegisterBlockAndWakeupHandlers(nxagentBlockHandler, nxagentWakeupHandler, NULL);
  }

  /*
   * We let the proxy flush the link on our behalf after having opened
   * the display. We are now entering the dispatcher. From now on
   * we'll flush the proxy link explicitly.
   */

  #ifdef TEST
  fprintf(stderr, "InitInput: Setting the NX flush policy to deferred.\n");
  #endif

  NXSetDisplayPolicy(nxagentDisplay, NXPolicyDeferred);
}

/*
 * DDX specific abort routine. This is called by AbortServer() that,
 * in turn, is called by FatalError().
 */
void AbortDDX(void)
{
  nxagentDoFullGeneration = True;

  nxagentCloseDisplay();

  /*
   * Do the required finalization if we are not going through the
   * normal X server shutdown.
   */

  if ((dispatchException & DE_TERMINATE) == 0)
  {
    nxagentAbortDisplay();
  }
}

/*
 * Called by GiveUp().
 */
void ddxGiveUp(void)
{
  AbortDDX();
}

void ddxBeforeReset(void)
{
}

CARD32 nxagentTimeoutCallback(OsTimerPtr timer, CARD32 now, void *arg)
{
  CARD32 idle = now - lastDeviceEventTime.milliseconds;

  #ifdef TEST
  fprintf(stderr, "%s: called, idle [%d] timeout [%d]\n", __func__, idle, nxagentOption(Timeout) * MILLI_PER_SECOND);
  #endif

  /* Set the time to exactly match the remaining time until timeout */
  if (idle < nxagentOption(Timeout) * MILLI_PER_SECOND)
  {
    return nxagentOption(Timeout) * MILLI_PER_SECOND - idle;
  }

   /*
   * The lastDeviceEventTime is updated every time a device event is
   * received, and it is used by WaitForSomething() to know when the
   * SaveScreens() function should be called. This solution doesn't
   * take care of a pointer button not being released, so we have to
   * handle this case by ourselves.
   */

/*
FIXME: Do we need to check the key grab if the
       autorepeat feature is disabled?
*/
  if (inputInfo.pointer -> button -> buttonsDown > 0)
  {
    #ifdef TEST
    fprintf(stderr, "%s: Prolonging timeout - there is a pointer button down.\n", __func__);
    #endif

    /* wait 10s more */
    return 10 * MILLI_PER_SECOND;
  }

  if (nxagentSessionState == SESSION_UP )
  {
    if (nxagentClients == 0)
    {
      fprintf(stderr, "Info: Auto-terminating session with no client running.\n");
      raise(SIGTERM);
    }
    else if (!nxagentOption(Persistent))
    {
      fprintf(stderr, "Info: Auto-terminating session with persistence not allowed.\n");
      raise(SIGTERM);
    }
    else
    {
      fprintf(stderr, "Info: Auto-suspending session with %d clients running.\n",
                  nxagentClients);
      raise(SIGHUP);
    }
  }

  /*
   * we do not need the timer anymore, so do not set a new time. The
   * signals above will either terminate or suspend the session. At
   * resume we will re-init the timer.
   */
  return 0;
}

void nxagentSetTimeoutTimer(unsigned int millis)
{
  if (nxagentOption(Timeout) > 0)
  {
    if (millis == 0)
    {
      millis = nxagentOption(Timeout) * MILLI_PER_SECOND;
    }

    #ifdef TEST
    fprintf(stderr, "%s: Setting auto-disconnect timeout to [%dms]\n", __func__, millis);
    #endif
    nxagentTimeoutTimer = TimerSet(nxagentTimeoutTimer, 0, millis, nxagentTimeoutCallback, NULL);
  }
}

void nxagentFreeTimeoutTimer(void)
{
  #ifdef TEST
  fprintf(stderr, "%s: freeing timeout timer\n", __func__);
  #endif
  TimerFree(nxagentTimeoutTimer);
  nxagentTimeoutTimer = NULL;
}


void OsVendorInit(void)
{
  return;
}

void OsVendorFatalError(void)
{
  /*
   * Let the session terminate gracely from an user's standpoint.
   */

  fprintf(stderr, "Session: Aborting session at '%s'.\n", GetTimeAsString());
  fprintf(stderr, "Session: Session aborted at '%s'.\n", GetTimeAsString());
}

void OsVendorVErrorFFunction(const char *f, va_list args)
{
  if (OsVendorVErrorFFatal == 0)
  {
    char buffer[1024];

    vsnprintf(buffer, sizeof(buffer), f, args);

    nxagentStartRedirectToClientsLog();

    fprintf(stderr, "%s", buffer);

    nxagentEndRedirectToClientsLog();
  }
  else
  {
    LogVWrite(-1, f, args);
  }
}

void OsVendorStartRedirectErrorFFunction(void)
{
  nxagentStartRedirectToClientsLog();
}

void OsVendorEndRedirectErrorFFunction(void)
{
  nxagentEndRedirectToClientsLog();
}


/*
 * In an uninialized nxagentGrabServerInfo .grabstate is 0 which is the
 * value of SERVER_GRABBED. Therefore we need to initialize .grabstate
 * with SERVER_UNGRABBED. A check in Screen.c would go wrong
 * otherwise.
 */
ServerGrabInfoRec nxagentGrabServerInfo = {.grabstate = SERVER_UNGRABBED, .client = NULL};

static void nxagentGrabServerCallback(CallbackListPtr *callbacks, void *data,
                                   void *args)
{
    ServerGrabInfoRec *grab = (ServerGrabInfoRec*)args;

    nxagentGrabServerInfo.client = grab->client;
    nxagentGrabServerInfo.grabstate = grab->grabstate;
}
