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

#include "X.h"
#include "Xproto.h"
#include "screenint.h"
#include "input.h"
#include "misc.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "servermd.h"
#include "mi.h"
#include <X11/fonts/fontstruct.h>
#include "dixfontstr.h"

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
 * Stubs for the DPMS extension.
 */

#ifdef DPMSExtension

void DPMSSet(int level);
int DPMSGet(int *level);
Bool DPMSSupported(void);

#endif

/*
 * Our error logging function.
 */

void OsVendorVErrorFFunction(const char *f, va_list args);

/*
 * True if this is a fatal error.
 */

extern int OsVendorVErrorFFatal;

/*
 * Redirect the error output to a
 * different file
 */

extern void (*OsVendorStartRedirectErrorFProc)();
extern void (*OsVendorEndRedirectErrorFProc)();

extern void SetVendorRelease(int release);

void OsVendorStartRedirectErrorFFunction();
void OsVendorEndRedirectErrorFFunction();

/*
 * Called by InitGlobals() in the
 * new X server tree.
 */


static void nxagentGrabServerCallback(CallbackListPtr *callbacks, void *data,
                                   void *args);

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
 * Set if the remote display supports
 * backing store.
 */
/*
FIXME: These, if not removed, should at least
       be moved to Display.h and Display.c.
*/
int nxagentBackingStore;
int nxagentSaveUnder;

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

int nxagentDoFullGeneration = 1;

 /*
 * 1 if agent running as X2goAgent
 * 0 if NX Agent
 */
int nxagentX2go;

/*
 * Checking if agent is x2go agent
 */

void checkX2goAgent()
{
  extern const char *nxagentProgName;
  if( strcasecmp(nxagentProgName,"x2goagent") == 0)
  {
    fprintf(stderr, "\nrunning as X2Go Agent\n");
    nxagentX2go=1;
  }
  else
    nxagentX2go=0;
}


/*
 * Called at X server's initialization.
 */

void InitOutput(ScreenInfo *screenInfo, int argc, char *argv[])
{
  char *authority;
  int i;

  #ifdef __sun

  char *environment;

  #endif

  /*
   * Check if we running as X2Go Agent
   */
  checkX2goAgent();

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
   * Avoid slowness due to buggy_repeat workaround
   * in libcairo versions >= 1.10.
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
   * Unset the LD_LIBRARY_PATH variable in
   * Popen() before calling execl() in the
   * child process.
   */

  NXUnsetLibraryPath(1);

  if (serverGeneration == 1)
  {
    AddCallback(&ServerGrabCallback, nxagentGrabServerCallback, NULL);
  }

  if (nxagentUserDefinedFontPath == 0)
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

  if ((authority = getenv("NX_XAUTHORITY")))
  {
    #ifdef __sun

    environment = malloc(15 + strlen(authority));

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
   * Open the display. We are at the early startup and
   * the information we'll get from the remote X server
   * will mandate some of the characteristics of the
   * session, like the screen depth. Note that this re-
   * liance on the remote display at session startup
   * should be removed. We should always operate at 32
   * bpp, internally, and do the required translations
   * as soon as the graphic operation needs to be real-
   * ized on the remote display.
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

  nxagentSetScreenInfo(screenInfo);

  /*
   * Initialize pixmap formats for this screen.
   */

  nxagentSetPixmapFormats(screenInfo);

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

  for (i = 0; i < nxagentNumScreens; i++)
  {
    AddScreen(nxagentOpenScreen, argc, argv);
  }

  nxagentNumScreens = screenInfo->numScreens;

  /*
   * Initialize the GCs used by the synchro-
   * nization put images. We do it here beca-
   * use we use the nxagentDefaultScreen.
   */

  nxagentAllocateGraphicContexts();

  nxagentDoFullGeneration = nxagentFullGeneration;

  /*
   * Use a solid black root window
   * background.
   */

  blackRoot = TRUE;

  nxagentInitKeystrokes(False);
}

void
nxagentNotifyConnection(int fd, int ready, void *data)
{
    nxagentDispatchEvents(NULL);
}

void InitInput(argc, argv)
     int argc;
     char *argv[];
{
  void *ptr, *kbd;

  ptr = AddInputDevice(nxagentPointerProc, True);
  kbd = AddInputDevice(nxagentKeyboardProc, True);

  RegisterPointerDevice(ptr);
  RegisterKeyboardDevice(kbd);

  mieqInit(kbd, ptr);

  /*
   * Add the display descriptor to the
   * set of descriptors awaited by the
   * dispatcher.
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
   * We let the proxy flush the link on our behalf
   * after having opened the display. We are now
   * entering the dispatcher. From now on we'll
   * flush the proxy link explicitly.
   */

  #ifdef TEST
  fprintf(stderr, "InitInput: Setting the NX flush policy to deferred.\n");
  #endif

  NXSetDisplayPolicy(nxagentDisplay, NXPolicyDeferred);
}

/*
 * DDX specific abort routine. This is called
 * by AbortServer() that, in turn, is called
 * by FatalError().
 */

void AbortDDX(void)
{
  nxagentDoFullGeneration = True;

  nxagentCloseDisplay();

  /*
   * Do the required finalization if we
   * are not going through the normal
   * X server shutdown.
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

void OsVendorInit(void)
{
  return;
}

void OsVendorFatalError(void)
{
  /*
   * Let the session terminate gracely
   * from an user's standpoint.
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

ServerGrabInfoRec nxagentGrabServerInfo;

static void nxagentGrabServerCallback(CallbackListPtr *callbacks, void *data,
                                   void *args)
{
    ServerGrabInfoRec *grab = (ServerGrabInfoRec*)args;

    nxagentGrabServerInfo.client = grab->client;
    nxagentGrabServerInfo.grabstate = grab->grabstate;
}

#ifdef DPMSExtension

void DPMSSet(int level)
{
}

int DPMSGet(int *level)
{
  return -1;
}

Bool DPMSSupported(void)
{
  return 1;
}

#endif
