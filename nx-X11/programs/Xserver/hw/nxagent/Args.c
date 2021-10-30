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

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <libgen.h>

#ifdef __sun
#include <strings.h>
#endif

#include "X.h"
#include "Xproto.h"
#include "screenint.h"
#include "input.h"
#include "misc.h"
#include "globals.h"
#include "scrnintstr.h"
#include "dixstruct.h"
#include "servermd.h"
#include "opaque.h"

#include "Init.h"
#include "Agent.h"
#include "Display.h"
#include "Args.h"
#include "Options.h"
#include "Binder.h"
#include "Trap.h"
#include "Screen.h"
#include "Image.h"
#ifdef RENDER
#include "Render.h"
#endif
#include "Handlers.h"
#include "Error.h"
#include "Reconnect.h"
#include "Utils.h"

/*
 * NX includes and definitions.
 */

#include "compext/Compext.h"
#include <nx/NXpack.h>

/*
 * Set here the required log level.
 */

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG
#undef  WATCH

#ifdef WATCH

#include "unistd.h"

#endif

#ifdef PANORAMIX
  #define PANORAMIX_DISABLED_COND (noPanoramiXExtension || PanoramiXExtensionDisabledHack)
#else
  #define PANORAMIX_DISABLED_COND TRUE
#endif

#ifdef RANDR
  #define RRXINERAMA_DISABLED_COND noRRXineramaExtension
#else
  #define RRXINERAMA_DISABLED_COND TRUE
#endif

/*
 * Define this to force the dispatcher
 * to always use the dumb scheduler.
 */

#undef DISABLE_SMART_SCHEDULE

Bool nxagentUserDefinedFontPath = False;

/*
 * From X11/ImUtil.c.
 */

extern int _XGetBitsPerPixel(Display *dpy, int depth);

extern char dispatchExceptionAtReset;

const char *nxagentProgName = NULL;

char nxagentDisplayName[NXAGENTDISPLAYNAMELENGTH];
Bool nxagentSynchronize = False;
Bool nxagentRealWindowProp = False;

Bool nxagentAutoDPI = False;

char nxagentShadowDisplayName[NXAGENTSHADOWDISPLAYNAMELENGTH] = {0};

char nxagentWindowName[NXAGENTWINDOWNAMELENGTH];
char nxagentDialogName[NXAGENTDIALOGNAMELENGTH];
char nxagentSessionId[NXAGENTSESSIONIDLENGTH] = {0};
char *nxagentOptionsFilenameOrString;

Bool nxagentFullGeneration = False;
int nxagentDefaultClass = TrueColor;
Bool nxagentUserDefaultClass = False;
int nxagentDefaultDepth;
Bool nxagentUserDefaultDepth = False;
struct UserGeometry nxagentUserGeometry = {0, 0, 0, 0, 0};
Bool nxagentUserBorderWidth = False;
int nxagentNumScreens = 0;
Bool nxagentReportWindowIds = False;
Bool nxagentReportPrivateWindowIds = False;
Bool nxagentDoDirectColormaps = False;
Window nxagentParentWindow = 0;

int nxagentLockDeferLevel = 0;

Bool nxagentResizeDesktopAtStartup = False;

Bool nxagentUseNXTrans   = False;
Bool nxagentForceNXTrans = False;

int nxagentMaxAllowedResets = 0;

char *nxagentKeyboard = NULL;

Bool nxagentOnce = True;

int nxagentRemoteMajor = -1;

static void nxagentParseOptionString(char*);

/*
 * Get the caption to be used for helper dialogs
 * from agent's window name passed as parameter.
 */

static int nxagentGetDialogName(void);

Bool nxagentVerbose = False;

char *nxagentKeystrokeFile = NULL;

int ddxProcessArgument(int argc, char *argv[], int i)
{
  /*
   * The flavour can never change, so only set it once.
   *
   * FIXME: ddxProcessArgument() is called once for every command line
   * argument, with argv[0] being the argument and not the program
   * name! We should move this check somewhere else.
   */
  if (nxagentProgName == NULL)
  {
    char *basec = strdup(argv[0]);
    nxagentProgName = strdup(basename(basec));
    SAFE_free(basec);

#ifdef X2GO
    /*
     * Check if we are running as X2Go Agent
     */
    checkX2goAgent(argc, argv);
#endif
  }

  #ifdef TEST
  fprintf(stderr, "%s: argv[0] [%s]  nxagentProgName [%s]\n", __func__, argv[0], nxagentProgName);
  #endif

  /*
   * Ensure that the options are set to their defaults.
   */

  static Bool resetOptions = True;

  if (resetOptions == True)
  {
    char *envOptions = NULL;

    nxagentInitOptions();

    resetOptions = False;

    /*
     * Ensure the correct order of options evaluation: the environment
     * first, then those included in the options file and, last, the
     * command line options.
     */

    char *envDisplay = getenv("DISPLAY");

    if (envDisplay != NULL && strlen(envDisplay) == 0)
    {
      envDisplay = NULL;
    }

    for (int j = 0; j < argc; j++)
    {
      if ((!strcmp(argv[j], "-display")) && (j + 1 < argc))
      {
        envOptions = strdup(argv[j + 1]);

        #ifdef WARNING
        if (envOptions == NULL)
        {
          fprintf(stderr, "ddxProcessArgument: WARNING! failed string allocation.\n");
        }
        #endif

        break;
      }
    }

    if ((envOptions == NULL) && (envDisplay != NULL))
    {
      envOptions = strdup(envDisplay);

      #ifdef WARNING
      if (envOptions == NULL)
      {
        fprintf(stderr, "ddxProcessArgument: WARNING! failed string allocation.\n");
      }
      #endif
    }

    if (envOptions != NULL)
    {
      nxagentParseOptionString(envOptions);
      SAFE_free(envOptions);
    }

    for (int j = 0; j < argc; j++)
    {
      if ((!strcmp(argv[j], "-options") || !strcmp(argv[j], "-option")) && j + 1 < argc)
      {
        SAFE_free(nxagentOptionsFilenameOrString);

        if (-1 == asprintf(&nxagentOptionsFilenameOrString, "%s", argv[j + 1]))
        {
          FatalError("malloc failed");
        }
        break;
      }
    }

    nxagentProcessOptions(nxagentOptionsFilenameOrString);
  }  /* if (resetOptions == True) */

  if (!strcmp(argv[i], "-B"))
  {
    #ifdef TEST
    fprintf(stderr, "ddxProcessArgument: Checking the NX binder option.\n");
    #endif

    if (nxagentCheckBinder(argc, argv, i) > 0)
    {
      /*
       * We are going to run the agent with the 'binder' option. Go
       * straight to the proxy loop.
       */

      #ifdef TEST
      fprintf(stderr, "ddxProcessArgument: Entering the NX proxy binder loop.\n");
      #endif

      nxagentBinderLoop();

      /*
       * This will make an exit.
       */

      nxagentBinderExit(0);
    }
    else
    {
      /*
       * Exit with an error.
       */

      nxagentBinderExit(1);
    }
  }

  if (!strcmp(argv[i], "-display"))
  {
    if (++i < argc)
    {
      snprintf(nxagentDisplayName, NXAGENTDISPLAYNAMELENGTH, "%s", argv[i]);
      return 2;
    }
    return 0;
  }

  if (!strcmp(argv[i], "-id"))
  {
    if (++i < argc)
    {
      snprintf(nxagentSessionId, NXAGENTSESSIONIDLENGTH, "%s", argv[i]);
      return 2;
    }
    return 0;
  }

  if (!strcmp(argv[i], "-version"))
  {
    nxagentShowVersionInfo();
    exit(0);
  }

  /*
   * This had to be '-options' since the beginning
   * but was '-option' by mistake. Now we have to
   * handle the backward compatibility.
   */

  if (!strcmp(argv[i], "-options") || !strcmp(argv[i], "-option"))
  {
    if (++i < argc)
    {
      SAFE_free(nxagentOptionsFilenameOrString);

      if (-1 == asprintf(&nxagentOptionsFilenameOrString, "%s", argv[i]))
      {
        FatalError("malloc failed");
      }
      return 2;
    }
    return 0;
  }

  if (!strcmp(argv[i], "-sync")) {
    nxagentSynchronize = True;
    return 1;
  }

  if (!strcmp(argv[i], "-nxrealwindowprop")) {
    nxagentRealWindowProp = True;
    return 1;
  }

  if (!strcmp(argv[i], "-reportwids")) {
    nxagentReportWindowIds = True;
    return 1;
  }

  if (!strcmp(argv[i], "-reportprivatewids")) {
    nxagentReportPrivateWindowIds = True;
    return 1;
  }

  if (!strcmp(argv[i], "-full")) {
    nxagentFullGeneration = True;
    return 1;
  }

  if (!strcmp(argv[i], "-class")) {
    if (++i < argc) {
      if (!strcmp(argv[i], "StaticGray")) {
        nxagentDefaultClass = StaticGray;
        nxagentUserDefaultClass = True;
        return 2;
      }
      else if (!strcmp(argv[i], "GrayScale")) {
        nxagentDefaultClass = GrayScale;
        nxagentUserDefaultClass = True;
        return 2;
      }
      else if (!strcmp(argv[i], "StaticColor")) {
        nxagentDefaultClass = StaticColor;
        nxagentUserDefaultClass = True;
        return 2;
      }
      else if (!strcmp(argv[i], "PseudoColor")) {
        nxagentDefaultClass = PseudoColor;
        nxagentUserDefaultClass = True;
        return 2;
      }
      else if (!strcmp(argv[i], "TrueColor")) {
        nxagentDefaultClass = TrueColor;
        nxagentUserDefaultClass = True;
        return 2;
      }
      else if (!strcmp(argv[i], "DirectColor")) {
        nxagentDefaultClass = DirectColor;
        nxagentUserDefaultClass = True;
        return 2;
      }
    }
    return 0;
  }

  if (!strcmp(argv[i], "-cc")) {
    if (++i < argc && sscanf(argv[i], "%i", &nxagentDefaultClass) == 1) {
      if (nxagentDefaultClass >= 0 && nxagentDefaultClass <= 5) {
        nxagentUserDefaultClass = True;
        /* let the OS layer process it as well, so return 0 */
      }
    }
    return 0;
  }

  if (!strcmp(argv[i], "-depth")) {
    if (++i < argc && sscanf(argv[i], "%i", &nxagentDefaultDepth) == 1) {
      if (nxagentDefaultDepth > 0) {
        nxagentUserDefaultDepth = True;
        return 2;
      }
    }
    return 0;
  }

  /*
   * These options are now useless and are parsed only for
   * compatibility with older versions.
   */

  if (!strcmp(argv[i], "-fast") ||
          !strcmp(argv[i], "-slow") ||
              !strcmp(argv[i], "-hint") ||
                  !strcmp(argv[i], "-sss"))
  {
    fprintf(stderr, "Warning: Ignoring deprecated command line option '%s'.\n",
                argv[i]);
    return 1;
  }

  if (!strcmp(argv[i], "-backingstore"))
  {
    if (++i < argc)
    {
      if (!strcmp(argv[i], "0"))
      {
        nxagentChangeOption(BackingStore, BackingStoreNever);
      }
      else
      {
        nxagentChangeOption(BackingStore, BackingStoreForce);
      }
      return 2;
    }
    return 0;
  } 

  if (!strcmp(argv[i], "-streaming"))
  {
    if (++i < argc)
    {
      if (!strcmp(argv[i], "0"))
      {
        nxagentChangeOption(Streaming, False);
      }
      else
      {
        nxagentChangeOption(Streaming, True);
      }
      return 2;
    }
    return 0;
  }

  if (!strcmp(argv[i], "-defer"))
  {
    int level;

    if (++i < argc &&
            sscanf(argv[i], "%i", &level) == 1 &&
                level >= 0 && level <= 2)
    {
      if (!nxagentOption(Shadow))
      {
        nxagentChangeOption(DeferLevel, level);

        /*
         * The defer level set with the command line is not changed
         * when the session is resumed.
         */
        nxagentLockDeferLevel = 1;
      }
      return 2;
    }
    return 0;
  }

  if (!strcmp(argv[i], "-irlimit"))
  {
    int limit;

    if (++i < argc &&
            sscanf(argv[i], "%i", &limit) == 1)
    {
      nxagentChangeOption(ImageRateLimit, limit);
      return 2;
    }
    return 0;
  }

  if (!strcmp(argv[i], "-tile"))
  {
    int width, height;

    if (++i < argc &&
            sscanf(argv[i], "%ix%i", &width, &height) == 2 &&
                width >= 32 && height >= 32)
    { 
      nxagentChangeOption(TileWidth,  width);
      nxagentChangeOption(TileHeight, height);
      return 2;
    }
    return 0;
  }

  if (strcmp(argv[i], "-fp") == 0)
  {
    if(++i < argc)
    {
      #ifdef TEST
      fprintf(stderr, "ddxProcessArgument: User defined font path [%s].\n", argv[i]);
      #endif

      nxagentUserDefinedFontPath = True;
      defaultFontPath = argv[i];
    }
    else
    {
      UseMsg();
    }
  }

  if (!strcmp(argv[i], "-geometry"))
  {
    if (++i < argc)
    {
      if (!strcmp(argv[i],"fullscreen") || !strcmp(argv[i],"allscreens"))
      {
        nxagentChangeOption(Fullscreen, True);
        nxagentChangeOption(AllScreens, True);
      }
      else if (!strcmp(argv[i],"onescreen"))
      {
        nxagentChangeOption(Fullscreen, True);
        nxagentChangeOption(AllScreens, False);
      }
      else
      { 
        if (nxagentUserGeometry.flag == 0) 
        {
          nxagentUserGeometry.flag = XParseGeometry(argv[i], 
                                                        &nxagentUserGeometry.X, 
                                                            &nxagentUserGeometry.Y, 
                                                                &nxagentUserGeometry.Width, 
                                                                    &nxagentUserGeometry.Height);
        }
      }

      if (nxagentUserGeometry.flag || (nxagentOption(Fullscreen)))
      {
	return 2;
      }
    }
    return 0;
  }

  if (!strcmp(argv[i], "-bw"))
  {
    int bw;

    if (++i < argc && sscanf(argv[i], "%i", &bw) == 1)
    {
      nxagentChangeOption(BorderWidth, bw);

      if (bw >= 0)
      {
        nxagentUserBorderWidth = True;
        return 2;
      }
    }
    return 0;
  }

  if (!strcmp(argv[i], "-name"))
  {
    if (++i < argc)
    {
      snprintf(nxagentWindowName, NXAGENTWINDOWNAMELENGTH, "%s", argv[i]);
      return 2;
    }
    return 0;
  }

  if (!strcmp(argv[i], "-scrns"))
  {
    if (++i < argc && sscanf(argv[i], "%i", &nxagentNumScreens) == 1)
    {
      if (nxagentNumScreens > 0)
      {
        if (nxagentNumScreens > MAXSCREENS)
	{
          ErrorF("Maximum number of screens is %d.\n", MAXSCREENS);
          nxagentNumScreens = MAXSCREENS;
        }
        return 2;
      }
    }
    return 0;
  }

  if (!strcmp(argv[i], "-install"))
  {
    nxagentDoDirectColormaps = True;
    return 1;
  }

  if (!strcmp(argv[i], "-parent"))
  {
    if (++i < argc)
    {
      nxagentParentWindow = (XID) strtol (argv[i], (char**)NULL, 0);
      return 2;
    }
  }

  if (!strcmp(argv[i], "-forcenx"))
  {
    nxagentForceNXTrans = True;
    return 1;
  }

  if (!strcmp(argv[i], "-norootlessexit"))
  {
    nxagentChangeOption(NoRootlessExit, True);
    return 1;
  }

  if (!strcmp(argv[i], "-nomagicpixel"))
  {
    nxagentChangeOption(MagicPixel, False);
    return 1;
  }

  if (!strcmp(argv[i], "-noonce"))
  {
    nxagentOnce = False;
    return 1;
  }

  if (!strcmp(argv[i], "-kbtype") ||
          !strcmp(argv[i], "-keyboard"))
  {
    if (++i < argc)
    {
      SAFE_free(nxagentKeyboard);

#ifdef X2GO
      if (nxagentX2go && strcmp(argv[i], "null/null") == 0)
      {
        #ifdef TEST
        fprintf(stderr, "%s: changing nxagentKeyboard from [null/null] to [clone].\n", __func__);
        #endif

        SAFE_free(nxagentKeyboard);
        nxagentKeyboard = strdup("clone");
      }
      else
#endif
      {
        nxagentKeyboard = strdup(argv[i]);
      }

      if (nxagentKeyboard == NULL)
      {
        FatalError("malloc failed");
      }
      return 2;
    }
    return 0;
  }

  if (!strcmp(argv[i], "-extensions"))
  {
    return 1;
  }

  #ifdef RENDER
  if (!strcmp(argv[i], "-norender"))
  {
    nxagentRenderEnable = False;
    return 1;
  }
  #endif

  if (!strcmp(argv[i], "-nocomposite"))
  {
    nxagentChangeOption(Composite, False);
    return 1;
  }

  /* the composite extension is disabled by default so we provide a
     way to enable it */
  if (!strcmp(argv[i], "-composite"))
  {
    nxagentChangeOption(Composite, True);
    return 1;
  }

  if (!strcmp(argv[i], "-nodamage"))
  {
    nxagentChangeOption(UseDamage, False);
    return 1;
  }

  if (!strcmp(argv[i], "-autodpi")) {
    nxagentAutoDPI = True;
    return 1;
  }

  /*
   * The original -noreset option, disabling dispatchExceptionAtReset,
   * is the default.  Use this option to restore the original
   * behaviour.
   */

  if (!strcmp(argv[i], "-reset"))
  {
    nxagentChangeOption(Reset, True);
    return 1;
  }

  if (!strcmp(argv[i], "-persistent"))
  {
    nxagentChangeOption(Persistent, True);
    return 1;
  }

  if (!strcmp(argv[i], "-nopersistent"))
  {
    nxagentChangeOption(Persistent, False);
    return 1;
  }

  if (!strcmp(argv[i], "-noshmem"))
  {
    nxagentChangeOption(SharedMemory, False);
    return 1;
  }

  if (!strcmp(argv[i], "-shmem"))
  {
    nxagentChangeOption(SharedMemory, True);
    return 1;
  }

  if (!strcmp(argv[i], "-noignore"))
  {
    nxagentChangeOption(DeviceControl, True);
    nxagentChangeOption(DeviceControlUserDefined, True);
    return 1;
  }

  if (!strcmp(argv[i], "-nokbreset"))
  {
    nxagentChangeOption(ResetKeyboardAtResume, False);
    return 1;
  }

  if (!strcmp(argv[i], "-noxkblock"))
  {
    nxagentChangeOption(InhibitXkb, False);
    return 1;
  }

  /*
   * Enable pseudo-rootless mode.
   */

  if (!strcmp(argv[i], "-R"))
  {
    nxagentChangeOption(Binder, False);
    nxagentChangeOption(Desktop, False);
    nxagentChangeOption(Rootless, True);
    return 1;
  }

  /*
   * Enable the "desktop" mode. This is the default.
   */

  if (!strcmp(argv[i], "-D"))
  {
    nxagentChangeOption(Binder, False);
    nxagentChangeOption(Rootless, False);
    nxagentChangeOption(Desktop, True);
    return 1;
  }

  /*
   * Enable the "shadow" mode.
   */

  if (!strcmp(argv[i], "-S"))
  {
    nxagentChangeOption(Shadow, True);
    nxagentChangeOption(DeferLevel, 0);
    nxagentChangeOption(Persistent, False);
    return 1;
  }

  if (!strcmp(argv[i], "-shadow"))
  {
    if (++i < argc)
    {
      snprintf(nxagentShadowDisplayName, NXAGENTSHADOWDISPLAYNAMELENGTH, "%s", argv[i]);

      if (strcmp(nxagentShadowDisplayName, "") == 0)
      {
        FatalError("Invalid shadow display option");
      }
      return 2;
    }
    return 0;
  }


  if (!strcmp(argv[i], "-shadowmode"))
  {
    if (++i < argc)
    {
      if (!strcmp(argv[i], "0"))
      {
        nxagentChangeOption(ViewOnly, True);
      }
      else
      {
        nxagentChangeOption(ViewOnly, False);
      }
      return 2;
    }
    return 0;
  }

  /*
   * Enable the auto-disconnect timeout.
   */

  if (!strcmp(argv[i], "-timeout"))
  {
    int seconds;

    if (++i < argc && sscanf(argv[i], "%i", &seconds) == 1)
    {
      if (seconds >= 0)
      {
        if (seconds > 0 && seconds < 60)
        {
          seconds = 60;
        }

        nxagentChangeOption(Timeout, seconds);
        return 2;
      }
    }
    return 0;
  }

  /*
   * The return value for -query, -broadcast and -indirect must be 0
   * to let the dix layer process these options.
   */

  if (!strcmp(argv[i], "-query"))

  {
    nxagentChangeOption(Desktop, True);
    nxagentChangeOption(Xdmcp, True);
    nxagentMaxAllowedResets = 0;
    return 0;
  }

  if (!strcmp(argv[i], "-broadcast"))

  {
    nxagentChangeOption(Desktop, True);
    nxagentChangeOption(Xdmcp, True);
    nxagentMaxAllowedResets = 0;
    return 0;
  }

  if (!strcmp(argv[i], "-indirect"))
  {
    nxagentChangeOption(Desktop, True);
    nxagentChangeOption(Xdmcp, True);
    nxagentMaxAllowedResets = 1;
    return 0;
  }

  if (!strcmp(argv[i], "-noshpix"))
  {
    nxagentChangeOption(SharedPixmaps, False);
    return 1;
  }

  if (!strcmp(argv[i], "-shpix"))
  {
    nxagentChangeOption(SharedPixmaps, True);
    return 1;
  }

  if (!strcmp(argv[i], "-clipboard"))
  {
    if ((!strcmp(argv[i+1], "both")) || (!strcmp(argv[i+1], "1")))
    {
      nxagentChangeOption(Clipboard, ClipboardBoth);
    }
    else if (!strcmp(argv[i+1], "client"))
    {
      nxagentChangeOption(Clipboard, ClipboardClient);
    }
    else if (!strcmp(argv[i+1], "server"))
    {
      nxagentChangeOption(Clipboard, ClipboardServer);
    }
    else if ((!strcmp(argv[i+1], "none")) || (!strcmp(argv[i+1], "0")))
    {
      nxagentChangeOption(Clipboard, ClipboardNone);
    }
    else
    {
      nxagentChangeOption(Clipboard, ClipboardBoth);
    }
    return 2;
  }

  if (!strcmp(argv[i], "-textclipboard"))
  {
    nxagentChangeOption(TextClipboard, True);
    return 1;
  }

  if (!strcmp(argv[i], "-bs"))
  {
    nxagentChangeOption(BackingStore, BackingStoreNever);
    return 1;
  }

  if (!strcmp(argv[i], "-verbose"))
  {
    nxagentVerbose = True;
    return 1;
  }

  if (!strcmp(argv[i], "-keystrokefile"))
  {
    if (i + 1 < argc)
    {
      if (NULL != (nxagentKeystrokeFile = strdup(argv[i + 1])))
      {
        return 2;
      } else {
	FatalError("malloc failed");
      }
    }
    return 0;
  }

  if (!strcmp(argv[i], "-autograb"))
  {
    nxagentChangeOption(AutoGrab, True);
    return 1;
  }

  /*
   * Disable Xinerama (i.e. fake it in Screen.c) if somehow Xinerama support
   * has been disabled on the cmdline.
   */
  if (PANORAMIX_DISABLED_COND && RRXINERAMA_DISABLED_COND)
    nxagentChangeOption(Xinerama, False);

  return 0;
}

/* copy from nxcomp's Loop.cpp */
static int hexval(char c)
{
  if ((c >= '0') && (c <= '9'))
    return c - '0';
  if ((c >= 'a') && (c <= 'f'))
    return c - 'a' + 10;
  if ((c >= 'A') && (c <= 'F'))
    return c - 'A' + 10;
  return -1;
}

static void URLDecodeInPlace(char *str)
{
  if (str)
  {
    char *to = str;
    while (str[0])
    {
      if ((str[0] == '%') &&
          (hexval(str[1]) >= 0) &&
          (hexval(str[2]) >= 0))
      {
        *(to++) = hexval(str[1]) * 16 + hexval(str[2]);
        str += 3;
      }
      else
        *(to++) = *(str++);
    }
    *to = '\0';
  }
}

static void nxagentParseSingleOption(char *name, char *value)
{
  int argc;

  char *argv[2] = { NULL, NULL };

  #ifdef TEST
  fprintf(stderr, "nxagentParseSingleOption: Processing option '%s' = '%s'.\n",
              validateString(name), validateString(value));
  #endif

  URLDecodeInPlace(value);

  if (!value)
    value = "";

  if (!strcmp(name, "kbtype") ||
          !strcmp(name, "keyboard") ||
              !strcmp(name, "id") ||
                  !strcmp(name, "display") ||
                      !strcmp(name, "clipboard") ||
                          !strcmp(name, "geometry") ||
                              !strcmp(name, "option") ||
                                  !strcmp(name, "options") ||
                                      !strcmp(name, "shadow") ||
                                          !strcmp(name, "shadowmode") ||
                                              !strcmp(name, "streaming") ||
                                                  !strcmp(name, "defer") ||
                                                      !strcmp(name, "tile"))
  {
    argv[1] = value;
    argc = 2;
  }
  else if (!strcmp(name, "R") && !strcmp(value, "1"))
  {
    argc = 1;
  }
  else if (!strcmp(name, "fast") || !strcmp(name, "slow"))
  {
    fprintf(stderr, "Warning: Ignoring deprecated option '%s'.\n", name);
    return;
  }
  else if (!strcmp(name, "render"))
  {
    if (nxagentReconnectTrap)
    {
      #ifdef DEBUG
      fprintf(stderr, "nxagentParseSingleOption: Ignoring option 'render' at reconnection.\n");
      #endif
    }
    else if (nxagentRenderEnable == UNDEFINED)
    {
      if (!strcmp(value, "1"))
      {
        nxagentRenderEnable = True;
      }
      else if (!strcmp(value, "0"))
      {
        nxagentRenderEnable = False;
      }
      else
      {
        fprintf(stderr, "Warning: Ignoring bad value '%s' for option 'render'.\n",
                    validateString(value));
      }
    }
    return;
  }
  else if (!strcmp(name, "state"))
  {
    setStatePath(value);
    return;
  }
  else if (!strcmp(name, "fullscreen"))
  {
    if (nxagentReconnectTrap)
    {
      #ifdef DEBUG
      fprintf(stderr, "nxagentParseSingleOption: Ignoring option 'fullscreen' at reconnection.\n");
      #endif
    }
    else if (!strcmp(value, "2"))
    {
      nxagentChangeOption(Fullscreen, True);
      nxagentChangeOption(AllScreens, False);
    }
    else if (!strcmp(value, "1"))
    {
      nxagentChangeOption(Fullscreen, True);
      nxagentChangeOption(AllScreens, True);
    }
    else if (!strcmp(value, "0"))
    {
      nxagentChangeOption(Fullscreen, False);
      nxagentChangeOption(AllScreens, False);
    }
    else
    {
      fprintf(stderr, "Warning: Ignoring bad value '%s' for option 'fullscreen'.\n",
                  validateString(value));
    }
    return;
  }
  else if (!strcmp(name, "shpix"))
  {
    if (!strcmp(value, "1"))
    {
      nxagentChangeOption(SharedPixmaps, True);
    }
    else if (!strcmp(value, "0"))
    {
      nxagentChangeOption(SharedPixmaps, False);
    }
    else
    {
      fprintf(stderr, "Warning: Ignoring bad value '%s' for option 'shpix'.\n",
                  validateString(value));
    }
    return;
  }
  else if (!strcmp(name, "shmem"))
  {
    if (!strcmp(value, "1"))
    {
      nxagentChangeOption(SharedMemory, True);
    }
    else if (!strcmp(value, "0"))
    {
      nxagentChangeOption(SharedMemory, False);
    }
    else
    {
      fprintf(stderr, "Warning: Ignoring bad value '%s' for option 'shmem'.\n",
                  validateString(value));
    }
    return;
  }
  else if (!strcmp(name, "composite"))
  {
    if (!strcmp(value, "1"))
    {
      nxagentChangeOption(Composite, True);
    }
    else if (!strcmp(value, "0"))
    {
      nxagentChangeOption(Composite, False);
    }
    else
    {
      fprintf(stderr, "Warning: Ignoring bad value '%s' for option 'composite'.\n",
                  validateString(value));
    }
    return;
  }
  else if (!strcmp(name, "xinerama"))
  {
#if !defined(PANORAMIX) && !defined(RANDR)
    nxagentChangeOption(Xinerama, False);
    fprintf(stderr, "Warning: No Xinerama support compiled into %s.\n", nxagentProgName);
    return;
#else
    if (PANORAMIX_DISABLED_COND && RRXINERAMA_DISABLED_COND)
    {
      nxagentChangeOption(Xinerama, False);
      fprintf(stderr, "Warning: XINERAMA extension has been disabled on %s startup.\n", nxagentProgName);
      return;
    }

    if (!strcmp(value, "1"))
    {
      nxagentChangeOption(Xinerama, True);
      return;
    }
    else if (!strcmp(value, "0"))
    {
      nxagentChangeOption(Xinerama, False);
    }
    else
    {
      fprintf(stderr, "Warning: Ignoring bad value '%s' for option 'xinerama'.\n",
              validateString(value));
    }
    return;
#endif
  }
  else if (!strcmp(name, "resize"))
  {
    if (!nxagentOption(DesktopResize) || strcmp(value, "0") == 0)
    {
      nxagentResizeDesktopAtStartup = False;
    }
    else if (strcmp(value, "1") == 0)
    {
      nxagentResizeDesktopAtStartup = True;
    }
    else
    {
      fprintf(stderr, "Warning: Ignoring bad value '%s' for option 'resize'.\n",
                  validateString(value));
    }
    return;
  }
  else if (!strcmp(name, "backingstore"))
  {
    if (!strcmp(value, "0"))
    {
      nxagentChangeOption(BackingStore, BackingStoreNever);
    }
    else
    {
      nxagentChangeOption(BackingStore, BackingStoreForce);
    }
    return;
  }
  else if (!strcmp(name, "menu"))
  {
    if (!strcmp(value, "0"))
    {
      nxagentChangeOption(Menu, False);
    }
    else
    {
      nxagentChangeOption(Menu, True);
    }
    return;
  }
  else if (!strcmp(name, "magicpixel"))
  {
    if (!strcmp(value, "0"))
    {
      nxagentChangeOption(MagicPixel, False);
    }
    else
    {
      nxagentChangeOption(MagicPixel, True);
    }
    return;
  }
  else if (!strcmp(name, "autodpi"))
  {
    if (nxagentReconnectTrap)
    {
      #ifdef DEBUG
      fprintf(stderr, "nxagentParseSingleOption: Ignoring option 'autodpi' at reconnection.\n");
      #endif
    }
    else if (!strcmp(value, "0"))
    {
      nxagentAutoDPI = False;
    }
    else
    {
      nxagentAutoDPI = True;
    }
    return;
  }
  else if (strcmp(name, "shadowuid") == 0)
  {
    nxagentShadowUid = atoi(value);
    return;
  }
  else if (strcmp(name, "clients") == 0)
  {
    char *new = strdup(value);
    if (new)
    {
      SAFE_free(nxagentClientsLogName);
      nxagentClientsLogName = new;
    }
    else
    {
      fprintf(stderr, "Warning: Ignoring option [%s] because of memory problems\n",
                      validateString(name));
    }
    return;
  }
  else if (strcmp(name, "client") == 0)
  {
    if (strcmp(value, "winnt") == 0 || strcmp(value, "windows") == 0)
    {
      nxagentChangeOption(ClientOs, ClientOsWinnt);
    }
    else if (strcmp(value, "linux") == 0)
    {
      nxagentChangeOption(ClientOs, ClientOsLinux);
    }
    else if (strcmp(value, "solaris") == 0)
    {
      nxagentChangeOption(ClientOs, ClientOsSolaris);
    }
    else if (strcmp(value, "macosx") == 0)
    {
      nxagentChangeOption(ClientOs, ClientOsMac);
    }
    return;
  }
  else if  (strcmp(name, "copysize") == 0)
  {
    nxagentChangeOption(CopyBufferSize, atoi(value));
    return;
  }
  else if (!strcmp(name, "sleep"))
  {
    errno = 0;
    long sleep_parse = strtol(value, NULL, 10);

    if ((errno) && (0 == sleep_parse))
    {
      fprintf(stderr, "Unable to convert value [%s] of option [%s]. "
                      "Ignoring option.\n",
                      validateString(value), validateString(name));
      return;
    }

    if ((long) UINT_MAX < sleep_parse)
    {
      sleep_parse = UINT_MAX;

      fprintf(stderr, "Warning: value [%s] of option [%s] "
                      "out of range, clamped to [%lu].\n",
                      validateString(value), validateString(name), sleep_parse);
    }

    if (0 > sleep_parse)
    {
      sleep_parse = 0;

      fprintf(stderr, "Warning: value [%s] of option [%s] "
                      "out of range, clamped to [%lu].\n",
                      validateString(value), validateString(name), sleep_parse);
    }

    nxagentChangeOption(SleepTimeMillis, sleep_parse);
    return;
  }
  else if (!strcmp(name, "tolerancechecks"))
  {
    if (strcmp(value, "strict") == 0)
    {
      nxagentChangeOption(ReconnectTolerance, ToleranceChecksStrict);
    }
    else if (strcmp(value, "safe") == 0)
    {
      nxagentChangeOption(ReconnectTolerance, ToleranceChecksSafe);
    }
    else if (strcmp(value, "risky") == 0)
    {
      nxagentChangeOption(ReconnectTolerance, ToleranceChecksRisky);
    }
    else if (strcmp(value, "bypass") == 0)
    {
      nxagentChangeOption(ReconnectTolerance, ToleranceChecksBypass);
    }
    else
    {
      /*
       * Check for a matching integer. Or any integer, really.
       */

      errno = 0;
      long tolerance_parse = strtol(value, NULL, 10);

      if ((errno) && (0 == tolerance_parse))
      {
        fprintf(stderr, "Unable to convert value [%s] of option [%s]. "
                        "Ignoring option.\n",
                        validateString(value), validateString(name));
        return;
      }

      if ((long) UINT_MAX < tolerance_parse)
      {
        tolerance_parse = UINT_MAX;

        fprintf(stderr, "Warning: value [%s] of option [%s] "
                        "out of range, clamped to [%lu].\n",
                        validateString(value), validateString(name), tolerance_parse);
      }

      if (0 > tolerance_parse)
      {
        tolerance_parse = 0;

        fprintf(stderr, "Warning: value [%s] of option [%s] "
                        "out of range, clamped to [%lu].\n",
                        validateString(value), validateString(name), tolerance_parse);
      }

      #ifdef TEST
      switch (tolerance_parse) {
        case ToleranceChecksStrict:
        case ToleranceChecksSafe:
        case ToleranceChecksRisky:
        case ToleranceChecksBypass:
          break;
        default:
          fprintf(stderr, "%s: Warning: value [%s] of "
                          "option [%s] unknown, will be mapped to "
                          "\"Bypass\" [%u] value internally.\n",
		  __func__, validateString(value), validateString(name),
		  (unsigned int)ToleranceChecksBypass);
      }
      #endif

      nxagentChangeOption(ReconnectTolerance, tolerance_parse);
    }
    return;
  }
  else if (!strcmp(name, "keyconv"))
  {
    if (!strcmp(value, "off")) {
      nxagentChangeOption(KeycodeConversion, KeycodeConversionOff);
    }
    else if (!strcmp(value, "on")) {
      nxagentChangeOption(KeycodeConversion, KeycodeConversionOn);
    }
    else if (!strcmp(value, "auto")) {
      nxagentChangeOption(KeycodeConversion, KeycodeConversionAuto);
    }
    else
    {
      fprintf(stderr, "Warning: Ignoring bad value '%s' for option 'keyconv'.\n",
              validateString(value));
    }
    return;
  }
  else if (!strcmp(name, "autograb"))
  {
    if (!strcmp(value, "0"))
    {
      nxagentChangeOption(AutoGrab, False);
    }
    else
    {
      nxagentChangeOption(AutoGrab, True);
    }
    return;
  }
  else if (!strcmp(name, "textclipboard"))
  {
    if (!strcmp(value, "0"))
    {
      nxagentChangeOption(TextClipboard, False);
    }
    else
    {
      nxagentChangeOption(TextClipboard, True);
    }
    return;
  }
  else
  {
    #ifdef DEBUG
    fprintf(stderr, "nxagentParseSingleOption: Ignored option [%s] with value [%s].\n",
                validateString(name), validateString(value));
    #endif
    return;
  }

  /*
   * Before passing the not yet evaluated options to
   * ddxProcessArgument(), we have to add a dash as prefix.
   */

  int size = strlen(name) + 1;

  if (size > 1)
  {
    if ((argv[0] = malloc(size + 1)) == NULL)
    {
      fprintf(stderr, "Warning: Ignoring option '%s' due to lack of memory.\n",
                  name);
      return;
    }

    *argv[0] = '-';
    memcpy(argv[0] + 1, name, size);
  }

  ddxProcessArgument(argc, argv, 0);

  SAFE_free(argv[0]);
}

static void nxagentParseOptionString(char *string)
{
  char *value     = NULL;
  char *option    = NULL;

  /*
   * we must not modify string, but strtok will insert \0. So let's
   * work with a copy
   */
  char *dup = strdup(string);

  /*
   * Remove the port specification.
   */

  char *delimiter = rindex(dup, ':');

  if (delimiter)
  {
    #ifdef DEBUG
    fprintf(stderr, "%s: stripping port specification [%s]\n", __func__, delimiter);
    #endif
    *delimiter = '\0';
  }
  else
  {
    fprintf(stderr, "Warning: Option file doesn't contain a port specification.\n");
  }

  while ((option = strtok(option ? NULL : dup, ",")))
  {
    delimiter = rindex(option, '=');

    if (delimiter)
    {
      *delimiter = '\0';
      value = delimiter + 1;
    }
    else
    {
      value = NULL;
    }

    nxagentParseSingleOption(option, value);
  }
  SAFE_free(dup);
}

char *nxagentSkipNXMarker(char *string)
{
  if (strncasecmp(string, "nx/nx,", 6) == 0 ||
      strncasecmp(string, "nx/nx:", 6) == 0)
  {
    #ifdef DEBUG
    fprintf(stderr, "%s: skipping [%6.6s]\n", __func__, string);
    #endif
    return string + 6;
  }
  else if (strncasecmp(string, "nx,", 3) == 0 ||
           strncasecmp(string, "nx:", 3) == 0)
  {
    #ifdef DEBUG
    fprintf(stderr, "%s: skipping [%3.3s]\n", __func__, string);
    #endif
    return string + 3;
  }
  else
  {
    return string;
  }
}

void nxagentProcessOptions(char * string)
{
  if (!string)
    return;

  #ifdef DEBUG
  fprintf(stderr, "%s: Going to process option string/filename [%s].\n",
          __func__, validateString(string));
  #endif

  /* if the "filename" starts with an nx marker treat it
     as an option _string_ instead of a filename */
  char *skipped = nxagentSkipNXMarker(string);
  if (skipped != string)
  {
    nxagentParseOptionString(skipped);
  }
  else
  {
    nxagentProcessOptionsFile(string);
  }
}

void nxagentProcessOptionsFile(char * filename)
{
  FILE *file = NULL;
  char *data = NULL;

  int sizeOfFile;
  int maxFileSize = 1024;

  #ifdef DEBUG
  fprintf(stderr, "nxagentProcessOptionsFile: Going to process option file [%s].\n",
          validateString(filename));
  #endif

  /*
   * Init statePath
   */
  setStatePath("");

  if (filename == NULL)
  {
    return;
  }

  if ((file = fopen(filename, "r")) == NULL)
  {
    fprintf(stderr, "Warning: Couldn't open option file '%s'. Error is '%s'.\n",
                validateString(filename), strerror(errno));
    goto nxagentProcessOptionsFileExit;
  }

  if (fseek(file, 0, SEEK_END) != 0)
  {
    fprintf(stderr, "Warning: Couldn't position inside option file '%s'. Error is '%s'.\n",
                validateString(filename), strerror(errno));
    goto nxagentProcessOptionsFileExit;
  }

  if ((sizeOfFile = ftell(file)) == -1)
  {
    fprintf(stderr, "Warning: Couldn't get the size of option file '%s'. Error is '%s'.\n",
                validateString(filename), strerror(errno));
    goto nxagentProcessOptionsFileExit;
  }

  #ifdef DEBUG
  fprintf(stderr, "nxagentProcessOptionsFile: Processing option file [%s].\n",
              validateString(filename));
  #endif

  rewind(file);

  if (sizeOfFile > maxFileSize)
  {
    fprintf(stderr, "Warning: Maximum file size exceeded for options '%s'.\n",
                validateString(filename));
    goto nxagentProcessOptionsFileExit;
  }

  if ((data = malloc(sizeOfFile + 1)) == NULL)
  {
    fprintf(stderr, "Warning: Memory allocation failed processing file '%s'.\n",
                validateString(filename));
    goto nxagentProcessOptionsFileExit;
  }

  int offset = 0;
  int size = 0;

  for (;;)
  {
    size_t result = fread(data + offset, 1, sizeOfFile, file);

    if (ferror(file) != 0)
    {
      fprintf(stderr, "Warning: Error reading the option file '%s'.\n",
                validateString(filename));
      goto nxagentProcessOptionsFileExit;
    }

    size   += result;
    offset += result;

    if (feof(file) != 0 || (size == sizeOfFile))
    {
      break;
    }
  }

  if (size != sizeOfFile)
  {
    fprintf(stderr, "Warning: Premature end of option file '%s' while reading.\n",
              validateString(filename));
    goto nxagentProcessOptionsFileExit;
  }

  /*
   * Truncate the buffer to the first line.
   */

  for (offset = 0; (offset < sizeOfFile) && (data[offset] != '\n'); offset++);

  data[offset] = '\0';

  #ifdef DEBUG
  fprintf(stderr, "%s: first line of options file [%s]\n", __func__, data);
  #endif

  nxagentParseOptionString(nxagentSkipNXMarker(data));

nxagentProcessOptionsFileExit:

  SAFE_free(data);

  if (file)
  {
    if (fclose(file) != 0)
    {
      fprintf(stderr, "Warning: Couldn't close option file '%s'. Error is '%s'.\n",
              validateString(filename), strerror(errno));
    }
  }

  return;
}

/*
 * FIXME: Transport initialization, shouldn't depend upon
 *        argv[], because we call it at every reconnection.
 */

Bool nxagentPostProcessArgs(char* name, Display* dpy, Screen* scr)
{
    Bool useNXTrans = False;

    #ifdef WATCH

    fprintf(stderr, "nxagentPostProcessArgs: Watchpoint 2.\n");

/*
Reply   Total	Cached	Bits In			Bits Out		Bits/Reply	  Ratio
------- -----	------	-------			--------		----------	  -----
N/A
*/

    sleep(30);

    #endif

    if (nxagentOption(Rootless) && nxagentOption(Fullscreen))
    {
      #ifdef TEST
      fprintf(stderr, "WARNING: Ignoring fullscreen option for rootless session.\n");
      #endif

      nxagentChangeOption(Fullscreen, False);
      nxagentChangeOption(AllScreens, False);
    }

    /*
     * Ensure we have a valid name for children dialogs.
     */

    nxagentGetDialogName();

    /*
     * Ensure we have a valid name for window name.
     */

    if (*nxagentWindowName == '\0')
    {
#ifdef X2GO
      if (nxagentX2go)
      {
        snprintf(nxagentWindowName, NXAGENTWINDOWNAMELENGTH, "X2Go Agent");
      }
      else
#endif
      {
        snprintf(nxagentWindowName, NXAGENTWINDOWNAMELENGTH, "NX Agent");
      }
    }

    /*
     * Note that use of NX packed images as well as
     * render extension could be later disabled due
     * to the fact that session is running nested
     * in a nxagent server.
     */

    if (nxagentForceNXTrans)
    {
      useNXTrans = True;
    }
    else if ((strncasecmp(name, "nx/", 3) == 0) ||
                 (strncasecmp(name, "nx:", 3) == 0) ||
                     (strncasecmp(name, "nx,", 3) == 0) ||
                         (strcasecmp(name, "nx") == 0))
    {
      useNXTrans = True;
    }

    if (useNXTrans == True)
    {
      unsigned int linkType = LINK_TYPE_NONE;

      unsigned int localMajor = 0;
      unsigned int localMinor = 0;
      unsigned int localPatch = 0;

      unsigned int remoteMajor = 0;
      unsigned int remoteMinor = 0;
      unsigned int remotePatch = 0;

      int splitTimeout  = 0;
      int motionTimeout = 0;

      int splitMode = 0;
      int splitSize = 0;

      unsigned int packMethod = PACK_NONE;
      unsigned int packQuality = 9;

      int dataLevel   = 0;
      int streamLevel = 0;
      int deltaLevel  = 0;

      unsigned int loadCache    = 0;
      unsigned int saveCache    = 0;
      unsigned int startupCache = 0;

      unsigned int clientSegment = 0;
      unsigned int serverSegment = 0;

      unsigned int clientSize = 0;
      unsigned int serverSize = 0;

      if (NXGetControlParameters(dpy, &linkType, &localMajor, &localMinor,
                                     &localPatch, &remoteMajor, &remoteMinor, &remotePatch,
                                         &splitTimeout, &motionTimeout, &splitMode, &splitSize,
                                             &packMethod, &packQuality, &dataLevel, &streamLevel,
                                                 &deltaLevel, &loadCache, &saveCache, &startupCache) == 0)
      {
        fprintf(stderr, "Warning: Failed to get the control parameters.\n");
      }

      nxagentChangeOption(LinkType, linkType);

      #ifdef TEST
      fprintf(stderr, "nxagentPostProcessArgs: Got local version [%d.%d.%d] remote version [%d.%d.%d].\n",
                  localMajor, localMinor, localPatch, remoteMajor, remoteMinor, remotePatch);

      fprintf(stderr, "nxagentPostProcessArgs: Got split timeout [%d] motion timeout [%d].\n",
                  splitTimeout, motionTimeout);

      fprintf(stderr, "nxagentPostProcessArgs: Got split mode [%d] split size [%d].\n",
                  splitMode, splitSize);

      fprintf(stderr, "nxagentPostProcessArgs: Got preferred pack method [%d] and quality [%d].\n",
                  packMethod, packQuality);
      #endif

      if (remoteMajor < 2)
      {
        #ifdef TEST
        fprintf(stderr, "nxagentPostProcessArgs: WARNING! Using backward compatible alpha encoding.\n");
        #endif

        nxagentAlphaCompat = True;
      }
      else
      {
        nxagentAlphaCompat = False;
      }

      nxagentRemoteMajor = remoteMajor;

      if (nxagentPackMethod == -1)
      {
        nxagentPackMethod = packMethod;
      }

      if (nxagentPackQuality == -1)
      {
        nxagentPackQuality = packQuality;
      }

      /*
       * Set the minimum size of images being streamed.
       */

      if (nxagentSplitThreshold == -1)
      {
        nxagentSplitThreshold = splitSize;
      }

      /*
       * Let the remote proxy use the shared memory extension, if
       * supported by the X server.  The client part is not useful and
       * not implemented. The size of the segment is chosen by the
       * user. The only purpose of the message is to reserve the XID
       * that will be used by the remote.
       */

      unsigned int enableClient = 0;
      unsigned int enableServer = 1;

      if (NXGetShmemParameters(dpy, &enableClient, &enableServer, &clientSegment,
                                   &serverSegment, &clientSize, &serverSize) == 0)
      {
        fprintf(stderr, "Warning: Failed to get the shared memory parameters.\n");
      }

      if (enableServer == 1)
      {
        fprintf(stderr, "Info: Using shared memory parameters %d/%d/%d/%dK.\n",
                    nxagentOption(SharedMemory), nxagentOption(SharedPixmaps),
                         enableServer, serverSize / 1024);
      }
      else
      {
        fprintf(stderr, "Info: Using shared memory parameters %d/%d/0/0K.\n",
                    nxagentOption(SharedMemory), nxagentOption(SharedPixmaps));
      }

      /*
       * We don't need the NoExpose events. Block them at the proxy
       * side.
       */

      NXSetExposeParameters(nxagentDisplay, 1, 1, 0);
    }
    else
    {
      /*
       * We don't have a proxy on the remote side.
       */

      nxagentChangeOption(LinkType, LINK_TYPE_NONE);
    }

    /*
     * Set the lossless and lossy pack methods based on the user's
     * preferences and the selected link type.
     */

    nxagentSetPackMethod();

    /*
     * If not set, set the defer level and the synchronization timeout
     * based on the link type.
     */

    nxagentSetDeferLevel();

    /*
     * Also set the display output buffer size.
     */

    nxagentSetBufferSize();

    /*
     * Select the preferred scheduler.
     */

    nxagentSetScheduler();

    /*
     * Select the buffer coalescence timeout.
     */

    nxagentSetCoalescence();

    /*
     * The enableBackingStore flag is defined
     * in window.c in the dix.
     */
/*
FIXME: In rootless mode the backing-store support is not functional yet.
*/
    if (nxagentOption(Rootless))
    {
      enableBackingStore = FALSE;
    }
    else if (nxagentOption(BackingStore) == BackingStoreUndefined ||
                 nxagentOption(BackingStore) == BackingStoreForce)
    {
      enableBackingStore = TRUE;
    }
    else if (nxagentOption(BackingStore) == BackingStoreNever)
    {
      enableBackingStore = FALSE;
    }

    /*
     * need to check if this was set on the command line as this has
     * the priority over the option file.
     */
 
    if (nxagentRenderEnable == UNDEFINED)
    {
      nxagentRenderEnable = True;
    }
    
    if (nxagentRenderEnable == True)
    {
      nxagentAlphaEnabled = True;
    }
    else
    {
      nxagentAlphaEnabled = False;
    }

    if (nxagentOption(Rootless) && nxagentOption(Xdmcp))
    {
      FatalError("PANIC! Cannot start a XDMCP session in rootless mode.\n");
    }

    /*
     * We enable server reset only for indirect XDMCP sessions.
     */

    if (nxagentOption(Reset) && nxagentMaxAllowedResets == 0)
    {
      #ifdef WARNING
      fprintf(stderr, "nxagentPostProcessArgs: Disabling the server reset.\n");
      #endif

      nxagentChangeOption(Reset, False);

      dispatchExceptionAtReset = 0;
    }

    /*
     * We skip server reset by default. This should be equivalent to
     * passing the -noreset option to a standard XFree86 server.
     */

    if (!nxagentOption(Reset))
    {
      #ifdef TEST
      fprintf(stderr, "nxagentPostProcessArgs: Disabling dispatch of exception at server reset.\n");
      #endif

      dispatchExceptionAtReset = 0;
    }

    /*
     * Check if the user activated the auto-disconect feature.
     */

    if (nxagentOption(Timeout) > 0)
    {
      fprintf(stderr, "Info: Using auto-disconnect timeout of %d seconds.\n",
                  nxagentOption(Timeout));
    }

    #ifdef WATCH

    fprintf(stderr, "nxagentPostProcessArgs: Watchpoint 3.\n");

/*
Reply   Total	Cached	Bits In			Bits Out		Bits/Reply	  Ratio
------- -----	------	-------			--------		----------	  -----
#16     1		256 bits (0 KB) ->	12 bits (0 KB) ->	256/1 -> 12/1	= 21.333:1
#233 A  1		256 bits (0 KB) ->	131 bits (0 KB) ->	256/1 -> 131/1	= 1.954:1
#245 A  2		512 bits (0 KB) ->	19 bits (0 KB) ->	256/1 -> 10/1	= 26.947:1
*/

    sleep(30);

    #endif

    return useNXTrans;
}

void ddxUseMsg(void)
{
  ErrorF("-full                  utilize full regeneration\n");
  ErrorF("-class string          default visual class\n");
  ErrorF("-depth int             default depth\n");
  ErrorF("-geometry string       window size and position (WXH+X+Y) or 'allscreens' / 'onescreen'\n");
  ErrorF("-bw int                window border width\n");
  ErrorF("-name string           window name\n");
  ErrorF("-scrns int             number of screens to generate\n");
  ErrorF("-install               install colormaps directly\n");

  ErrorF("\nThe NX system adds the following arguments:\n");
  ErrorF("-options file|string   file or string containing nx/nx options\n");
  ErrorF("-forcenx               force use of NX protocol messages assuming communication through nxproxy\n");
  ErrorF("-timeout int           auto-disconnect timeout in seconds (minimum allowed: 60)\n");
  ErrorF("-norootlessexit        don't exit if there are no clients in rootless mode\n");
  ErrorF("-nomagicpixel          disable nxagent's magic pixel\n");
  ErrorF("-autodpi               detect real server's DPI and use that in the session\n");
  ErrorF("-display string        display name of the real server\n");
  ErrorF("-sync                  synchronize with the real server\n");
  ErrorF("-nxrealwindowprop      set property NX_REAL_WINDOW for each X11 window inside nxagent\n");
  ErrorF("-reportwids            report externally exposed X11 window IDs to the session log\n");
  ErrorF("-reportprivatewids     report internal X11 window ID to the session log\n");
#ifdef RENDER
  ErrorF("-norender              disable the use of the render extension\n");
  ErrorF("-nocomposite           disable the use of the composite extension (default)\n");
  ErrorF("-composite             enable the use of the composite extension\n");
#endif
  ErrorF("-nopersistent          disable disconnection/reconnection to the X display on SIGHUP\n");
  ErrorF("-noshmem               disable use of shared memory extension\n");
  ErrorF("-shmem                 enable use of shared memory extension\n");
  ErrorF("-noshpix               disable use of shared pixmaps\n");
  ErrorF("-shpix                 enable use of shared pixmaps\n");
  ErrorF("-noignore              don't ignore pointer and keyboard configuration changes mandated by clients\n");
  ErrorF("-nokbreset             don't reset keyboard device if the session is resumed\n");
  ErrorF("-noxkblock             always allow applications to change layout through XKEYBOARD\n");
  ErrorF("-autograb              enable autograb\n");
  ErrorF("-textclipboard         limit clipboard data to text only\n");
  ErrorF("-irlimit               maximum image data rate to the encoder input in kB/s.\n");
  ErrorF("-tile WxH              maximum size of image tiles (minimum allowed: 32x32)\n");
  ErrorF("-keystrokefile file    file with keyboard shortcut definitions\n");
  ErrorF("-options file|string   path to an options file or an option string (for testing/debugging)\n");
  ErrorF("-verbose               print more warning and error messages\n");
  ErrorF("-D                     enable desktop mode\n");
  ErrorF("-R                     enable rootless mode\n");
  ErrorF("-S                     enable shadow mode\n");
  ErrorF("-B                     enable proxy binding mode\n");
  ErrorF("-version               show version information and exit\n");
}

static int nxagentGetDialogName(void)
{
  if (*nxagentSessionId != '\0')
  {
    int length = strlen(nxagentSessionId);

    /* if the session id contains an MD5 hash in a well-known format cut it off */
    if (length > (MD5_LENGTH * 2 + 1) &&
           *(nxagentSessionId + (length - (MD5_LENGTH * 2 + 1))) == '-')
    {
      length -= (MD5_LENGTH * 2 + 1);
    }

    snprintf(nxagentDialogName, NXAGENTDIALOGNAMELENGTH, "NX - %.*s", length, nxagentSessionId);

    return 1;
  }

  snprintf(nxagentDialogName, NXAGENTDIALOGNAMELENGTH, "NX");

  return 0;
}

void nxagentSetPackMethod(void)
{
  unsigned char supportedMethods[NXNumberOfPackMethods];
  unsigned int entries = NXNumberOfPackMethods;

  if (nxagentOption(LinkType) == LINK_TYPE_NONE)
  {
    nxagentChangeOption(Streaming, False);

    nxagentPackMethod   = PACK_NONE;
    nxagentPackLossless = PACK_NONE;

    nxagentSplitThreshold = 0;

    return;
  }

  /*
   * Check if we need to select the lossy and lossless pack methods
   * based on the link type.
   */

  int method = nxagentPackMethod;

  if (method == PACK_ADAPTIVE)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentSetPackMethod: Using adaptive mode for image compression.\n");
    #endif

    nxagentChangeOption(Adaptive, True);
  }
  else
  {
    #ifdef TEST
    fprintf(stderr, "nxagentSetPackMethod: Not using adaptive mode for image compression.\n");
    #endif

    nxagentChangeOption(Adaptive, False);
  }
  
  if (method == PACK_LOSSY || method == PACK_ADAPTIVE)
  {
    nxagentPackMethod = PACK_JPEG_16M_COLORS;
  }
  else if (method == PACK_LOSSLESS)
  {
    switch (nxagentOption(LinkType))
    {
      case LINK_TYPE_MODEM:
      case LINK_TYPE_ISDN:
      case LINK_TYPE_ADSL:
      case LINK_TYPE_WAN:
      {
        nxagentPackMethod = PACK_BITMAP_16M_COLORS;
        break;
      }
      case LINK_TYPE_LAN:
      {
        nxagentPackMethod = PACK_RLE_16M_COLORS;
        break;
      }
      default:
      {
        fprintf(stderr, "Warning: Unknown link type '%d' while setting the pack method.\n",
                    nxagentOption(LinkType));
        break;
      }
    }
  }

  /*
   * Query the remote proxy to determine whether the selected methods
   * are supported.
   */

  if (NXGetUnpackParameters(nxagentDisplay, &entries, supportedMethods) == 0 ||
          entries != NXNumberOfPackMethods)
  {
    fprintf(stderr, "Warning: Unable to retrieve the supported pack methods.\n");

    nxagentPackMethod   = PACK_NONE;
    nxagentPackLossless = PACK_NONE;
  }
  else
  {
    if (nxagentPackMethod == PACK_BITMAP_16M_COLORS ||
            nxagentPackMethod == PACK_RLE_16M_COLORS ||
                nxagentPackMethod == PACK_RGB_16M_COLORS ||
                    nxagentPackMethod == PACK_NONE)
    {
      nxagentPackLossless = nxagentPackMethod;
    }
    else
    {
      if (nxagentOption(LinkType) == LINK_TYPE_LAN)
      {
        nxagentPackLossless = PACK_RLE_16M_COLORS;
      }
      else
      {
        nxagentPackLossless = PACK_BITMAP_16M_COLORS;
      }
    }

    if (supportedMethods[nxagentPackLossless] == 0)
    {
      nxagentPackLossless = PACK_NONE;
    }

    #ifdef TEST
    fprintf(stderr, "nxagentSetPackMethod: Using method [%d] for lossless compression.\n",
                nxagentPackLossless);
    #endif

    if (supportedMethods[nxagentPackMethod] == 0)
    {
      fprintf(stderr, "Warning: Pack method '%d' not supported by the proxy.\n",
                  nxagentPackMethod);

      fprintf(stderr, "Warning: Replacing with lossless pack method '%d'.\n",
                  nxagentPackLossless);

      nxagentPackMethod = nxagentPackLossless;
    }
  }

  if (nxagentPackMethod == nxagentPackLossless)
  {
    nxagentPackQuality = 9;
  }

  #ifdef TEST
  fprintf(stderr, "nxagentSetPackMethod: Assuming pack methods [%d] and [%d] with "
              "quality [%d].\n", nxagentPackMethod, nxagentPackLossless, nxagentPackQuality);
  #endif
}

/*
 * Each defer level adds the following rules to the previous ones:
 *
 * Level 0  Eager encoding.
 *
 * Level 1  No data is put or copied on pixmaps, marking them always
 *          as corrupted and synchronizing them on demand, i.e. when
 *          a copy area to a window is requested, the source is syn-
 *          chronized before copying it.
 *
 * Level 2  The put images over the windows are skipped marking the
 *          destination as corrupted. The same happens for copy area
 *          and composite operations, spreading the corrupted regions
 *          of involved drawables.
 */

void nxagentSetDeferLevel(void)
{
  /* defaults */
  int deferLevel = 0;
  int tileWidth = 64;
  int tileHeight = 64;
  int deferTimeout = 200;

  /*
   * Streaming is only partly implemented and is not available in this
   * version of the agent.
   */

  if (nxagentOption(Streaming))
  {
    fprintf(stderr, "Warning: Streaming of images not available in this agent.\n");

    nxagentChangeOption(Streaming, False);
  }

  switch (nxagentOption(LinkType))
  {
    case LINK_TYPE_MODEM:
    case LINK_TYPE_ISDN:  { deferLevel = 2; tileWidth =   64; tileHeight =   64; break; }
    case LINK_TYPE_ADSL:  { deferLevel = 2; tileWidth = 4096; tileHeight = 4096; break; }
    case LINK_TYPE_WAN:   { deferLevel = 1; tileWidth = 4096; tileHeight = 4096; break; }
    case LINK_TYPE_NONE:
    case LINK_TYPE_LAN:   { deferLevel = 0; tileWidth = 4096; tileHeight = 4096; break; }
    default:
    {
      fprintf(stderr, "Warning: Unknown link type '%d' processing the defer option.\n",
                  nxagentOption(LinkType));
      break;
    }
  }

  /*
   * Set the defer timeout.
   */

  if (nxagentOption(Shadow))
  {
    #ifdef TEST
    fprintf(stderr, "nxagentSetDeferLevel: Ignoring defer timeout parameter in shadow mode.\n");
    #endif
  }
  else
  {
    nxagentChangeOption(DeferTimeout, deferTimeout);
  }

  /*
   * Set the defer level.
   */

  if (nxagentOption(Shadow))
  {
    #ifdef TEST
    fprintf(stderr, "nxagentSetDeferLevel: Ignoring defer parameter in shadow mode.\n");
    #endif
  }
  else if (nxagentOption(DeferLevel) != UNDEFINED)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentSetDeferLevel: Not overriding the [defer] option "
                "with value [%d]. Defer timeout is [%ld] ms.\n", nxagentOption(DeferLevel),
                    nxagentOption(DeferTimeout));
    #endif
  }
  else
  {
    nxagentChangeOption(DeferLevel, deferLevel);

    #ifdef TEST
    fprintf(stderr, "nxagentSetDeferLevel: Assuming defer level [%d] with timeout of [%ld] ms.\n",
                nxagentOption(DeferLevel), nxagentOption(DeferTimeout));
    #endif
  }

  /*
   * Set the tile width.
   */

  if (nxagentOption(TileWidth) != UNDEFINED)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentSetDeferLevel: Not overriding the [tile] option "
                "width value [%d].\n", nxagentOption(TileWidth));
    #endif
  }
  else
  {
    nxagentChangeOption(TileWidth, tileWidth);

    #ifdef TEST
    fprintf(stderr, "nxagentSetDeferLevel: Assuming tile width [%d].\n",
                 nxagentOption(TileWidth));
    #endif
  }

  /*
   * Set the tile height.
   */
  
  if (nxagentOption(TileHeight) != UNDEFINED)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentSetDeferLevel: Not overriding the [tile] option "
                "height value [%d].\n", nxagentOption(TileHeight));
    #endif
  }
  else
  {
    nxagentChangeOption(TileHeight, tileHeight);

    #ifdef TEST
    fprintf(stderr, "nxagentSetDeferLevel: Assuming tile height [%d].\n",
                 nxagentOption(TileHeight));
    #endif
  }
}

void nxagentSetBufferSize(void)
{
  int size = 16384;

  switch (nxagentOption(LinkType))
  {
    case LINK_TYPE_MODEM: { size =  4096; break; }
    case LINK_TYPE_ISDN:  { size =  4096; break; }
    case LINK_TYPE_ADSL:  { size =  8192; break; }
    case LINK_TYPE_WAN:   { size = 16384; break; }
    case LINK_TYPE_NONE:
    case LINK_TYPE_LAN:   { size = 16384; break; }
    default:
    {
      fprintf(stderr, "Warning: Unknown link type '%d' while setting the display buffer size.\n",
                  nxagentOption(LinkType));
      break;
    }
  }

  nxagentChangeOption(DisplayBuffer, size);

  nxagentBuffer = size;

  if (NXSetDisplayBuffer(nxagentDisplay, nxagentBuffer) < 0)
  {
    fprintf(stderr, "Warning: Can't set the display buffer size to [%d].\n",
                nxagentBuffer);
  }
}

void nxagentSetScheduler(void)
{
  /*
   * The smart scheduler is the default.
   */

  if (nxagentOption(Shadow))
  {
    #ifdef TEST
    fprintf(stderr, "nxagentSetScheduler: Using the dumb scheduler in shadow mode.\n");
    #endif

    nxagentDisableTimer();
  }
}

void nxagentSetCoalescence(void)
{
  int timeout = 0;

  switch (nxagentOption(LinkType))
  {
    case LINK_TYPE_MODEM: { timeout = 50; break; }
    case LINK_TYPE_ISDN:  { timeout = 20; break; }
    case LINK_TYPE_ADSL:  { timeout = 10; break; }
    case LINK_TYPE_WAN:   { timeout = 5;  break; }
    case LINK_TYPE_NONE:
    case LINK_TYPE_LAN:   { timeout = 0;  break; }
    default:
    {
      fprintf(stderr, "Warning: Unknown link type '%d' while setting the display coalescence.\n",
                  nxagentOption(LinkType));
      break;
    }
  }

  #ifdef TEST
  fprintf(stderr, "nxagentSetCoalescence: Using coalescence timeout of [%d] ms.\n",
              timeout);
  #endif

  nxagentChangeOption(DisplayCoalescence, timeout);
}

void nxagentShowVersionInfo(void)
{
  ErrorF("NXAGENT - Version " NX_VERSION_CURRENT_STRING "\n");
}
