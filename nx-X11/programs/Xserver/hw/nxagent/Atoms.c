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

#include "scrnintstr.h"
#include "Agent.h"

#include "Xutil.h"
#include "Xatom.h"
#include "Xlib.h"

#include "misc.h"
#include "scrnintstr.h"
#include "resource.h"

#include <nx/NXpack.h>

#include "Atoms.h"
#include "Args.h"
#include "Image.h"
#include "Display.h"
#include "Screen.h"
#include "Options.h"
#include "Agent.h"
#include "Utils.h"

/*
 * Set here the required log level.
 */

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

/*
 * These values should be moved in
 * the option repository.
 */

Bool nxagentWMIsRunning;

static void startWMDetection(void);

static void nxagentInitAtomMap(char **atomNameList, int count, Atom *atomsRet);

#ifdef DEBUG
static void nxagentPrintAtomMapInfo(char *message);
#else
#define nxagentPrintAtomMapInfo(arg)
#endif

Atom nxagentAtoms[NXAGENT_NUMBER_OF_ATOMS];

/*
 * Careful! Do not change indices here! Some of those are referenced
 * at other places via nxagentAtoms[index].
 */
static char *nxagentAtomNames[NXAGENT_NUMBER_OF_ATOMS + 1] =
{
  "NX_IDENTITY",                 /*  0 */
      /* NX_IDENTITY was used in earlier nx versions to communicate
         the version to NXwin. Got dropped between nxagent 1.5.0-45
         and 1.5.0-112. */
  "WM_PROTOCOLS",                /*  1 */
      /* standard ICCCM Atom */
  "WM_DELETE_WINDOW",            /*  2 */
      /* standard ICCCM Atom */
  "WM_NX_READY",                 /*  3 */
      /* nxagent takes the ownership of the selection with this name
         to signal the nxclient (or any other watching program)
         it is ready. */
  "MCOPGLOBALS",                 /*  4 */
      /* used for artsd support. */
  "NX_CUT_BUFFER_SERVER",        /*  5 */
      /* this is the name of a property on nxagent's window on the
         real X server. This property is used for passing clipboard
         content from clients of the real X server to nxagent's clients

         Unfortunately we cannot rename this to NX_SELTRANS_TO_AGENT
         because nomachine's nxclient and nxwin are using this
         Atom/selection for communication with the nxagent Atom. */
  "TARGETS",                     /*  6 */
      /* used to request a list of supported data formats from the
        selection owner. Standard ICCCM Atom */
  "TEXT",                        /*  7 */
      /* one of the supported data formats for selections. Standard
         ICCCM Atom */
  "NX_AGENT_SIGNATURE",          /*  8 */
      /* this is used to set a property on nxagent's window if nxagent
         is started with the fullscreen option set. Unsure, what this
         is used for. */
  "NXDARWIN",                    /*  9 */
      /* this was an Atom in nxdarwin, nomachine's X server for MacOS. */
  "CLIPBOARD",                   /* 10 */
      /* Atom for the clipboard selection. PRIMARY is fixed in X11 but
         CLIPBOARD is not. Standard ICCCM Atom. */
  "TIMESTAMP",                   /* 11 */
      /* used to request the time a selection has been owned. Standard
         ICCCM Atom */
  "UTF8_STRING",                 /* 12 */
      /* one of the supported data formats for selections. Standard
         ICCCM Atom */
  "_NET_WM_STATE",               /* 13 */
      /* standard ICCCM Atom */
  "_NET_WM_STATE_FULLSCREEN",    /* 14 */
      /* standard ICCCM Atom */
  "NX_SELTRANS_FROM_AGENT",      /* 15 */
      /* this is the name of a property on nxagent's window on the real
         X server. This property is used for passing clipboard content
         from nxagent's clients to clients on the real X server */
  "COMPOUND_TEXT",               /* 16 */
      /* one of the supported data formats for selections. Standard
         ICCCM Atom */
  "INCR",                        /* 17 */
      /* incremental clipboard transfers. Standard
         ICCCM Atom */
  "MULTIPLE",                    /* 18 */
      /* request selection in multiple formats at once. Standard
         ICCCM Atom */
  "DELETE",                      /* 19 */
      /* request to delete selection. Standard ICCCM Atom */
  "INSERT_SELECTION",            /* 20 */
      /* request to insert other selection. Standard ICCCM Atom */
  "INSERT_PROPERTY",             /* 21 */
      /* request to insert content of property into selection. Standard
         ICCCM Atom */
  NULL,
  NULL
};

static XErrorHandler previousErrorHandler = NULL;

static void catchAndRedirect(Display* dpy, XErrorEvent* X)
{
  if (X -> error_code == BadAccess &&
      X -> request_code == X_ChangeWindowAttributes &&
      X -> resourceid == DefaultRootWindow(dpy))
  {
    nxagentWMIsRunning = True;
  }
  else
  {
    previousErrorHandler(dpy, X);
  }
}

static void startWMDetection(void)
{
  /*
   * We are trying to detect if is there any client
   * that is listening for 'WM' events on the root
   * window.
   */

  nxagentWMIsRunning = False;

  previousErrorHandler = XSetErrorHandler((XErrorHandler)&catchAndRedirect);

  /*
   * After this request we need to Sync with
   * the X server to be sure we get any error
   * that is generated.
   */

  XSelectInput(nxagentDisplay,
                   RootWindow (nxagentDisplay, 0),
                       SubstructureRedirectMask | ResizeRedirectMask | ButtonPressMask);
}

static void finishWMDetection(Bool verbose)
{
  XSetErrorHandler(previousErrorHandler);

  if (nxagentWMIsRunning)
  {
    if (verbose == 1)
    {
      fprintf(stderr, "Info: Detected window manager running.\n");
    }
  }
  else
  {
    if (verbose == 1)
    {
      fprintf(stderr, "Info: Not found a window manager running.\n");
    }

    /*
     * We are not really interested on root window events.
     */

    XSelectInput(nxagentDisplay, RootWindow (nxagentDisplay, 0), 0);
  }
}

void nxagentWMDetect() 
{
  /* FIXME: verbose is always false, there's no code to set it to true */
  Bool verbose = False;
  Bool windowManagerWasRunning = nxagentWMIsRunning;

  startWMDetection();

  XSync(nxagentDisplay, 0);

  if (windowManagerWasRunning != nxagentWMIsRunning)
  {
    verbose = False;
  }

  finishWMDetection(verbose);
}

void nxagentInitAtoms()
{
  /*
   * Value of nxagentAtoms[8] is "NX_AGENT_SIGNATURE".
   *
   * We don't need to save the atom's value. It will
   * be checked by other agents to find if they are
   * run nested.
   */

  Atom atom = MakeAtom(nxagentAtomNames[8], strlen(nxagentAtomNames[8]), 1);

  if (atom == None)
  {
    #ifdef PANIC
    fprintf(stderr, "%s: PANIC! Could not create [%s] atom.\n", __func__,
                nxagentAtomNames[8]);
    #endif
  }
  else
  {
    #ifdef TEST
    fprintf(stderr, "nxagentInitAtoms: atom [%s] created with value [%d].\n",
                nxagentAtomNames[8], atom);
    #endif
  }
}

int nxagentQueryAtoms(ScreenPtr pScreen)
{
  static unsigned long atomGeneration = 1;

  int num_of_atoms = NXAGENT_NUMBER_OF_ATOMS;
  char *names[NXAGENT_NUMBER_OF_ATOMS];

  #ifdef TEST
  fprintf(stderr, "%s: Going to create the intern atoms on real display.\n", __func__);
  #endif

  nxagentPrintAtomMapInfo("nxagentQueryAtoms: Entering");

  for (int i = 0; i < num_of_atoms; i++)
  {
    names[i] = nxagentAtomNames[i];
    nxagentAtoms[i] = None;
  }

  if (nxagentSessionId[0])
  {
    names[num_of_atoms - 1] = nxagentSessionId;
  }
  else
  {
    num_of_atoms--;
  }

  startWMDetection();

  nxagentInitAtomMap(names, num_of_atoms, nxagentAtoms);

  /*
   * We need to be synchronized with the X server
   * in order to detect the Window Manager, since
   * after a reset the XInternAtom could be cached
   * by Xlib.
   */

  if (atomGeneration != serverGeneration)
  {
    #ifdef WARNING
    fprintf(stderr, "%s: The nxagent has been reset with server %ld atom %ld.\n", __func__,
                serverGeneration, atomGeneration);

    fprintf(stderr, "%s: Forcing a sync to detect the window manager.\n", __func__);
    #endif

    atomGeneration = serverGeneration;

    XSync(nxagentDisplay, 0);
  }

  finishWMDetection(False);

  /*
   * Value of nxagentAtoms[9] is "NXDARWIN".
   *
   * We check if it was created by the NX client.
   */

  if (nxagentAtoms[9] > nxagentAtoms[0])
  {
    nxagentAtoms[9] = None;
  }

  /*
   * Value of nxagentAtoms[8] is "NX_AGENT_SIGNATURE".
   *
   * This atom is created internally by the agent server at
   * startup to let other agents determine if they are run
   * nested. If agent is run nested, in fact, at the time it
   * will create the NX_AGENT_SIGNATURE atom on the real X
   * server it will find the existing atom with a value less
   * than any NX_IDENTITY created but itself.
   */

  if (nxagentAtoms[8] > nxagentAtoms[0])
  {
    nxagentAtoms[8] = None;
  }

  if (nxagentAtoms[8] != None)
  {
    /*
     * We are running nested in another agent
     * server.
     */

    nxagentChangeOption(Nested, 1);

    /*
     * Avoid the image degradation caused by
     * multiple lossy encoding.
     */

    fprintf(stderr, "Warning: Disabling use of lossy encoding in nested mode.\n");

    nxagentPackMethod = nxagentPackLossless;
  }

  #ifdef TEST

  for (int i = 0; i < num_of_atoms; i++)
  {
    fprintf(stderr, "%s: Created intern atom [%s] with id [%ld].\n", __func__,
                names[i], nxagentAtoms[i]);
  }
  #endif

  nxagentPrintAtomMapInfo("nxagentQueryAtoms: Exiting");

  return 1;
}

#define NXAGENT_ATOM_MAP_SIZE_INCREMENT 256

typedef struct {
    Atom local;
    XlibAtom remote;
    char *string;
    int  length;
} AtomMap;

static AtomMap *privAtomMap = NULL;
static unsigned int privAtomMapSize = 0;
static unsigned int privLastAtom = 0;

static void nxagentExpandCache(void);
static void nxagentWriteAtom(Atom, XlibAtom, const char*);
static AtomMap* nxagentFindAtomByRemoteValue(XlibAtom);
static AtomMap* nxagentFindAtomByLocalValue(Atom);
static AtomMap* nxagentFindAtomByName(char*, unsigned);

static void nxagentExpandCache(void)
{
  privAtomMapSize += NXAGENT_ATOM_MAP_SIZE_INCREMENT;

  privAtomMap = realloc(privAtomMap, privAtomMapSize * sizeof(AtomMap));

  if (privAtomMap == NULL)
  {
    FatalError("nxagentExpandCache: realloc failed\n");
  }
}

/*
 * Check if there is space left on the map and manage the possible
 * consequent allocation, then cache the atom-couple.
 */

static void nxagentWriteAtom(Atom local, XlibAtom remote, const char *string)
{
  char *s = strdup(string);

  #ifdef WARNING
  if (s == NULL)
  {
    fprintf(stderr, "nxagentWriteAtom: Malloc failed.\n");
  }
  #endif

  if (privLastAtom == privAtomMapSize)
  {
    nxagentExpandCache();
  }

  privAtomMap[privLastAtom].local = local;
  privAtomMap[privLastAtom].remote = remote;
  privAtomMap[privLastAtom].string = s;
  privAtomMap[privLastAtom].length = strlen(s);

  privLastAtom++;
}

/*
 * Clean up the atom map at nxagent reset, in order to cancel all the
 * local atoms but still maintaining the Xserver values and the atom
 * names. This is called from Dispatch()
 */
void nxagentResetAtomMap(void)
{
  nxagentPrintAtomMapInfo("nxagentResetAtomMap: Entering");

  for (unsigned int i = 0; i < privLastAtom; i++)
  {
    privAtomMap[i].local = None;
  }

  nxagentPrintAtomMapInfo("nxagentResetAtomMap: Exiting");
}

void nxagentFreeAtomMap(void)
{
  for (unsigned int i = 0; i < privLastAtom; i++)
  {
    SAFE_free(privAtomMap[i].string);
  }

  SAFE_free(privAtomMap);
  privLastAtom = privAtomMapSize = 0;
}

/*
 * Init map.
 * Initializing the atomNameList all in one.
 */

static void nxagentInitAtomMap(char **atomNameList, int count, Atom *atomsRet)
{
  int list_size = count + privLastAtom;

  nxagentPrintAtomMapInfo("nxagentInitAtomMap: Entering");

  XlibAtom *atom_list = malloc((list_size) * sizeof(*atom_list));
  char **name_list = malloc((list_size) * sizeof(char*));

  if ((atom_list == NULL) || (name_list == NULL))
  {
    SAFE_free(atom_list);
    SAFE_free(name_list);
    FatalError("nxagentInitAtomMap: malloc failed\n");
  }

  for (unsigned int i = 0; i < count; i++)
  {
    name_list[i] = atomNameList[i];
    atom_list[i] = None;
  }
  
  for (unsigned int i = 0; i < privLastAtom; i++)
  {
    name_list[count + i] = (char *)privAtomMap[i].string;
    atom_list[count + i] = None;
  }

  /*
   * Ask X-Server for our Atoms
   * ... if successful cache them too.
   */

  int ret_value = XInternAtoms(nxagentDisplay, name_list, list_size, False, atom_list);

  if (ret_value == 0)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentInitAtomMap: WARNING! XInternAtoms request failed.\n");
    #endif

    SAFE_free(atom_list);
    SAFE_free(name_list);

    return;
  }

  for (unsigned int i = 0; i < list_size; i++)
  {
    AtomMap *aMap = nxagentFindAtomByName(name_list[i], strlen(name_list[i]));

    if (aMap == NULL)
    {
      Atom local = MakeAtom(name_list[i], strlen(name_list[i]), True);

      if (ValidAtom(local))
      {
        nxagentWriteAtom(local, atom_list[i], name_list[i]);
      }
      else
      {
        #ifdef WARNING
        fprintf(stderr, "nxagentInitAtomMap: WARNING MakeAtom failed.\n");
        #endif
      }
    }
    else
    {
      aMap -> remote = atom_list[i];

      if (i < count && aMap -> local == None)
      {
        aMap -> local = MakeAtom(name_list[i], strlen(name_list[i]), True);
      }
    }

    if (i < count)
    {
      atomsRet[i] = atom_list[i];
    }
  }

  SAFE_free(atom_list);
  SAFE_free(name_list);

  nxagentPrintAtomMapInfo("nxagentInitAtomMap: Exiting");

  return;
}

/*
 * If the nxagent has been reset, the local value of the atoms stored
 * in cache could have the value None, do not call this function with
 * None.
 */

static AtomMap* nxagentFindAtomByLocalValue(Atom local)
{
  if (!ValidAtom(local))
  {
    return NULL;
  }

  for (unsigned int i = 0; i < privLastAtom; i++)
  {
    if (local == privAtomMap[i].local)
    {
      return (privAtomMap + i);
    }
  }

  return NULL;
}

static AtomMap* nxagentFindAtomByRemoteValue(XlibAtom remote)
{
  if (remote == None || remote == BAD_RESOURCE)
  {
    return NULL;
  }

  for (unsigned int i = 0; i < privLastAtom; i++)
  {
    if (remote == privAtomMap[i].remote)
    {
      return (privAtomMap + i);
    }
  }

  return NULL;
}

static AtomMap* nxagentFindAtomByName(char *string, unsigned int length)
{
  for (unsigned int i = 0; i < privLastAtom; i++)
  {
    if ((length == privAtomMap[i].length) && 
            (strcmp(string, privAtomMap[i].string) == 0))
    {
      return (privAtomMap + i);
    }
  }

  return NULL;
}

/*
 * Convert local atom's name to X-server value.
 * Reading them from map, if they have been already cached or
 * really asking to X-server and caching them.
 * FIXME: I don't really know if is better to allocate
 *        an automatic variable like ret_value and write it, instead of make all
 *        these return!, perhaps this way the code is a little bit easier to read.
 *        I think this and the 2 .*Find.* are the only functions to look for performances.
 */

XlibAtom nxagentMakeAtom(char *string, unsigned int length, Bool Makeit)
{
  /*
   * Surely MakeAtom is faster than our nxagentFindAtomByName.
   */

  Atom local = MakeAtom(string, length, Makeit);

  if (!ValidAtom(local))
  {
    return None;
  }

  if (local <= XA_LAST_PREDEFINED)
  {
    return local;
  }

  AtomMap *current;

  if ((current = nxagentFindAtomByLocalValue(local)))
  {
    /*
     * Found cached by value.
     */

    return current->remote;
  }
  else if ((current = nxagentFindAtomByName(string, length)))
  {
    /*
     * Found cached by name.
     * It means that nxagent has been reset,
     * but not the xserver so we still have cached its atoms.
     */

    current->local = local;

    return current->remote;
  }
  else
  {
    /*
     * We really have to ask the Xserver for it.
     */

    /* FIXME: why is Makeit inverted here? */
    XlibAtom remote = XInternAtom(nxagentDisplay, string, !Makeit);

    if (remote == None)
    {
      #ifdef WARNING
      fprintf(stderr, "nxagentMakeAtom: WARNING XInternAtom(.., %s, ..) failed.\n", string);
      #endif

      return None;
    }
    else
    {
      nxagentWriteAtom(local, remote, string);

      return remote;
    }
  }
}

XlibAtom nxagentLocalToRemoteAtom(Atom local)
{
  #ifdef TEST
  fprintf(stderr, "%s: entering\n", __func__);
  #endif

  if (!ValidAtom(local))
  {
    #ifdef DEBUG
    fprintf(stderr, "%s: local [%d] is no valid - returning None\n", __func__, local);
    #endif
    return None;
  }

  /* no mapping required for built-in atoms */
  if (local <= XA_LAST_PREDEFINED)
  {
    #ifdef DEBUG
    fprintf(stderr, "%s: local [%d] is < XA_LAST_PREDEFINED [%d]\n", __func__, local, XA_LAST_PREDEFINED);
    #endif
    return local;
  }

  AtomMap *current = nxagentFindAtomByLocalValue(local);

  if (current)
  {
    #ifdef DEBUG
    if (current->string)
      fprintf(stderr, "%s: local [%d] -> remote [%d (%s)]\n", __func__, local, current->remote, current->string);
    else
      fprintf(stderr, "%s: local [%d] -> remote [%d]\n", __func__, local, current->remote);
    #endif

    return current->remote;
  }
  else
  {
    const char *string = NameForAtom(local);

    /* FIXME: why False? */
    XlibAtom remote = XInternAtom(nxagentDisplay, string, False);

    if (remote == None)
    {
      #ifdef WARNING
      fprintf(stderr, "nxagentLocalToRemoteAtom: WARNING XInternAtom failed.\n");
      #endif

      return None;
    }

    nxagentWriteAtom(local, remote, string);

    #ifdef TEST
    fprintf(stderr, "%s: local [%d (%s)] -> remote [%d]\n", __func__, local, string, remote);
    #endif

    return remote;
  }
}

Atom nxagentRemoteToLocalAtom(XlibAtom remote)
{
  if (remote == None || remote == BAD_RESOURCE)
  {
    #ifdef DEBUG
    fprintf(stderr, "%s: remote [%d] is None or BAD_RESOURCE\n", __func__, remote);
    #endif
    return None;
  }

  /* no mapping required for built-in atoms */
  if (remote <= XA_LAST_PREDEFINED)
  {
    #ifdef DEBUG
    fprintf(stderr, "%s: remote [%d] is <= XA_LAST_PREDEFINED [%d]\n", __func__, remote, XA_LAST_PREDEFINED);
    #endif
    return remote;
  }

  AtomMap *current = nxagentFindAtomByRemoteValue(remote);

  if (current)
  {
    if (!ValidAtom(current->local))
    {
      Atom local = MakeAtom(current->string, current->length, True);

      if (ValidAtom(local))
      {
        current->local = local;
      }
      else
      {
        #ifdef WARNING
        fprintf(stderr, "nxagentRemoteToLocalAtom: WARNING MakeAtom failed.\n");
        #endif

        current->local = None;
      }
    }

    #ifdef DEBUG
    if (current->string)
      fprintf(stderr, "%s: remote [%d] -> local [%d (%s)]\n", __func__, remote, current->local, current->string);
    else
      fprintf(stderr, "%s: remote [%d] -> local [%d]\n", __func__, remote, current->local);
    #endif

    return current->local;
  }
  else
  {
    char *string = XGetAtomName(nxagentDisplay, remote);

    if (string)
    {
      Atom local = MakeAtom(string, strlen(string), True);

      if (!ValidAtom(local))
      {
        #ifdef WARNING
        fprintf(stderr, "%s: WARNING MakeAtom failed.\n", __func__);
        #endif

        local = None;
      }

      nxagentWriteAtom(local, remote, string);

      #ifdef TEST
      fprintf(stderr, "%s: remote [%d (%s)] -> local [%d]\n", __func__, remote, string, local);
      #endif
      SAFE_XFree(string);

      return local;
    }

    #ifdef WARNING
    fprintf(stderr, "%s: WARNING failed to get name from remote atom.\n", __func__);
    #endif

    return None;
  }
}

#ifdef DEBUG

static void nxagentPrintAtomMapInfo(char *message)
{
  fprintf(stderr, "--------------- Atom map in context [%s] ----------------------\n", message);
  fprintf(stderr, "nxagentPrintAtomMapInfo: Map at [%p] size [%d] number of entry [%d] auto increment [%d].\n",
              (void*) privAtomMap, privLastAtom, privAtomMapSize, NXAGENT_ATOM_MAP_SIZE_INCREMENT);

  for (unsigned int i = 0; i < privLastAtom; i++)
  {
    fprintf(stderr, "[%5.1d] local: %6.1u - remote: %6.1u - [%p] %s\n", i,
                privAtomMap[i].local,
                    privAtomMap[i].remote,
                        privAtomMap[i].string, validateString(privAtomMap[i].string));
  }

  fprintf(stderr, "---------------------------------------------\n");
}

#endif
