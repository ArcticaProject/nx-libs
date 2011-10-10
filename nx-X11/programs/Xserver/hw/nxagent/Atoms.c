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

#include "scrnintstr.h"
#include "Agent.h"

#include "Xutil.h"
#include "Xatom.h"
#include "Xlib.h"

#include "misc.h"
#include "scrnintstr.h"
#include "resource.h"

#include "NXpack.h"

#include "Atoms.h"
#include "Args.h"
#include "Image.h"
#include "Display.h"
#include "Screen.h"
#include "Options.h"
#include "Agent.h"

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

static int nxagentInitAtomMap(char **atomNameList, int count, Atom *atomsRet);

#ifdef DEBUG
static void nxagentPrintAtomMapInfo(char *message);
#else
#define nxagentPrintAtomMapInfo(arg)
#endif

Atom nxagentAtoms[NXAGENT_NUMBER_OF_ATOMS];

static char *nxagentAtomNames[NXAGENT_NUMBER_OF_ATOMS + 1] =
{
  "NX_IDENTITY",                 /* 0  */
  "WM_PROTOCOLS",                /* 1  */
  "WM_DELETE_WINDOW",            /* 2  */
  "WM_NX_READY",                 /* 3  */
  "MCOPGLOBALS",                 /* 4  */
  "NX_CUT_BUFFER_SERVER",        /* 5  */
  "TARGETS",                     /* 6  */
  "TEXT",                        /* 7  */
  "NX_AGENT_SIGNATURE",          /* 8  */
  "NXDARWIN",                    /* 9  */
  "CLIPBOARD",                   /* 10 */
  "TIMESTAMP",                   /* 11 */
  "UTF8_STRING",                 /* 12 */
  "_NET_WM_STATE",               /* 13 */
  "_NET_WM_STATE_FULLSCREEN",    /* 14 */
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
    nxagentWMIsRunning = TRUE;
  }
  else
  {
    previousErrorHandler(dpy, X);
  }
}

static void startWMDetection()
{
  /*
   * We are trying to detect if is there any client
   * that is listening for 'WM' events on the root
   * window.
   */

  nxagentWMIsRunning = FALSE;

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
  Bool verbose = False;
  int windowManagerWasRunning = nxagentWMIsRunning;

  startWMDetection();

  XSync(nxagentDisplay, 0);

  if (windowManagerWasRunning != nxagentWMIsRunning)
  {
    verbose = False;
  }

  finishWMDetection(verbose);
}

int nxagentInitAtoms(WindowPtr pWin)
{
  Atom atom;

  /*
   * Value of nxagentAtoms[8] is "NX_AGENT_SIGNATURE".
   *
   * We don't need to save the atom's value. It will
   * be checked by other agents to find if they are
   * run nested.
   */

  atom = MakeAtom(nxagentAtomNames[8], strlen(nxagentAtomNames[8]), 1);

  if (atom == None)
  {
    #ifdef PANIC
    fprintf(stderr, "nxagentInitAtoms: PANIC! Could not create [%s] atom.\n",
                nxagentAtomNames[8]);
    #endif

    return -1;
  }

  return 1;
}

int nxagentQueryAtoms(ScreenPtr pScreen)
{
  int i;
  static unsigned long atomGeneration = 1;

  int num_of_atoms = NXAGENT_NUMBER_OF_ATOMS;
  char *names[NXAGENT_NUMBER_OF_ATOMS];

  unsigned long int startingTime = GetTimeInMillis();

  #ifdef TEST
  fprintf(stderr, "nxagentQueryAtoms: Going to create the intern atoms on real display.\n");

  fprintf(stderr, "nxagentQueryAtoms: Starting time is [%ld].\n", startingTime);
  #endif

  nxagentPrintAtomMapInfo("nxagentQueryAtoms: Entering");

  for (i = 0; i < num_of_atoms; i++)
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
    fprintf(stderr, "nxagentQueryAtoms: The nxagent has been reset with server %ld atom %ld.\n",
                serverGeneration, atomGeneration);

    fprintf(stderr, "nxagentQueryAtoms: Forcing a sync to detect the window manager.\n");
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

  for (i = 0; i < num_of_atoms; i++)
  {
    fprintf(stderr, "nxagentQueryAtoms: Created intern atom [%s] with id [%ld].\n",
                names[i], nxagentAtoms[i]);
  }

  #endif

  nxagentChangeOption(DisplayLatency, GetTimeInMillis() - startingTime);

  #ifdef TEST
  fprintf(stderr, "nxagentQueryAtoms: Ending time is [%ld] reference latency is [%d] Ms.\n",
              GetTimeInMillis(), nxagentOption(DisplayLatency));
  #endif

  nxagentPrintAtomMapInfo("nxagentQueryAtoms: Exiting");

  return 1;
}

#define NXAGENT_ATOM_MAP_SIZE_INCREMENT 256

typedef struct {
    Atom local;
    Atom remote;
    char *string;
    int  length;
} AtomMap;

static AtomMap *privAtomMap = NULL;
static unsigned int privAtomMapSize = 0;
static unsigned int privLastAtom = 0;

static void nxagentExpandCache(void);
static void nxagentWriteAtom(Atom, Atom, char*, Bool);
static AtomMap* nxagentFindAtomByRemoteValue(Atom);
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
 * Check if there is space left on the map
 * and manage the possible consequent allocation,
 * then cache the atom-couple.
 */

static void nxagentWriteAtom(Atom local, Atom remote, char *string, Bool duplicate)
{
  char *s;

  /*
   * We could remove this string duplication if
   * we know for sure that the server will not
   * reset, since only at reset the dix layer
   * free all the atom names.
   */

  if (duplicate)
  {
    s = strdup(string);

    #ifdef WARNING
    if (s == NULL)
    {
      fprintf(stderr, "nxagentWriteAtom: Malloc failed.\n");
    }
    #endif
  }
  else
  {
    s = string;
  }

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
 * FIXME: We should clean up the atom map
 * at nxagent reset, in order to cancel 
 * all the local atoms but still mantaining 
 * the Xserver values and the atom names.
 */

void nxagentResetAtomMap()
{
  unsigned i;

  nxagentPrintAtomMapInfo("nxagentResetAtomMap: Entering");

  for (i = 0; i < privLastAtom; i++)
  {
    privAtomMap[i].local = None;
  }

  nxagentPrintAtomMapInfo("nxagentResetAtomMap: Exiting");
}

/*
 * Init map.
 * Initializing the atomNameList all in one.
 */

static int nxagentInitAtomMap(char **atomNameList, int count, Atom *atomsRet)
{
  XlibAtom *atom_list;
  char **name_list;
  unsigned int i;
  int ret_value = 0;
  int list_size = count + privLastAtom;

  nxagentPrintAtomMapInfo("nxagentInitAtomMap: Entering");

  atom_list = malloc((list_size) * sizeof(*atom_list));
  name_list = malloc((list_size) * sizeof(char*));

  if ((atom_list == NULL) || (name_list == NULL))
  {
    FatalError("nxagentInitAtomMap: malloc failed\n");
  }

  for (i = 0; i < count; i++)
  {
    name_list[i] = atomNameList[i];
    atom_list[i] = None;
  }
  
  for (i = 0; i < privLastAtom; i++)
  {
    name_list[count + i] = privAtomMap[i].string;
    atom_list[count + i] = None;
  }

  /*
   * Ask X-Server for ours Atoms
   * ... if successfull cache them too.
   */

  ret_value = XInternAtoms(nxagentDisplay, name_list, list_size, False, atom_list);

  if (ret_value == 0)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentInitAtomMap: WARNING! XInternAtoms request failed.\n");
    #endif

    free(atom_list);
    free(name_list);

    return 0;
  }

  for (i = 0; i < list_size; i++)
  {
    AtomMap *aMap = nxagentFindAtomByName(name_list[i], strlen(name_list[i]));

    if (aMap == NULL)
    {
      Atom local = MakeAtom(name_list[i], strlen(name_list[i]), True);

      if (ValidAtom(local))
      {
        nxagentWriteAtom(local, atom_list[i], name_list[i], False);
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

  free(atom_list);
  free(name_list);

  nxagentPrintAtomMapInfo("nxagentInitAtomMap: Exiting");

  return 1;
}

/*
 * If the nxagent has been resetted,
 * the local value of the atoms stored
 * in cache could have the value None, 
 * do not call this function with None.
 */

static AtomMap* nxagentFindAtomByLocalValue(Atom local)
{
  unsigned i;

  if (!ValidAtom(local))
  {
    return NULL;
  }

  for (i = 0; i < privLastAtom; i++)
  {
    if (local == privAtomMap[i].local)
    {
      return (privAtomMap + i);
    }
  }

  return NULL;
}

static AtomMap* nxagentFindAtomByRemoteValue(Atom remote)
{
  unsigned i;

  if (remote == None || remote == BAD_RESOURCE)
  {
    return NULL;
  }

  for (i = 0; i < privLastAtom; i++)
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
  unsigned i;

  for (i = 0; i < privLastAtom; i++)
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
 *        these return!, perhaps this way the code is a little bit easyer to read.
 *        I think this and the 2 .*Find.* are the only functions to look for performances.
 */

Atom nxagentMakeAtom(char *string, unsigned int length, Bool Makeit)
{
  Atom local;
  AtomMap *current;

  /*
   * Surely MakeAtom is faster than
   * our nxagentFindAtomByName.
   */

  local = MakeAtom(string, length, Makeit);

  if (!ValidAtom(local))
  {
    return None;
  }

  if (local <= XA_LAST_PREDEFINED)
  {
    return local;
  }

  if ((current = nxagentFindAtomByLocalValue(local)))
  {
    /*
     * Found cached by value.
     */

    return current->remote;
  }

  if ((current = nxagentFindAtomByName(string, length)))
  {
    /*
     * Found Cached by name.
     * It means that nxagent has been resetted,
     * but not the xserver so we still have cached its atoms.
     */

    current->local = local;

    return current->remote;
  }

  /*
   * We really have to ask Xserver for it.
   */

  {
    Atom remote;

    remote = XInternAtom(nxagentDisplay, string, !Makeit);

    if (remote == None)
    {
      #ifdef WARNING
      fprintf(stderr, "nxagentMakeAtom: WARNING XInternAtom failed.\n");
      #endif

      return None;
    }

    nxagentWriteAtom(local, remote, string, True);

    return remote;
  }
}

Atom nxagentLocalToRemoteAtom(Atom local)
{
  AtomMap *current;
  char    *string;
  Atom    remote;

  if (!ValidAtom(local))
  {
    return None;
  }

  if (local <= XA_LAST_PREDEFINED)
  {
    return local;
  }

  if ((current = nxagentFindAtomByLocalValue(local)))
  {
    return current->remote;
  }

  string = NameForAtom(local);

  remote = XInternAtom(nxagentDisplay, string, False);

  if (remote == None)
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentLocalToRemoteAtom: WARNING XInternAtom failed.\n");
    #endif

    return None;
  }

  nxagentWriteAtom(local, remote, string, True);

  return remote;
}

Atom nxagentRemoteToLocalAtom(Atom remote)
{
  AtomMap *current;
  char    *string;
  Atom    local;

  if (remote == None || remote == BAD_RESOURCE)
  {
    return None;
  }

  if (remote <= XA_LAST_PREDEFINED)
  {
    return remote;
  }

  if ((current = nxagentFindAtomByRemoteValue(remote)))
  {
    if (!ValidAtom(current->local))
    {
      local = MakeAtom(current->string, current->length, True);

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

    return current->local;
  }

  if ((string = XGetAtomName(nxagentDisplay, remote)))
  {
    local = MakeAtom(string, strlen(string), True);

    if (!ValidAtom(local))
    {
      #ifdef WARNING
      fprintf(stderr, "nxagentRemoteToLocalAtom: WARNING MakeAtom failed.\n");
      #endif

      local = None;
    }

    nxagentWriteAtom(local, remote, string, True);

    XFree(string);

    return local;
  }

  #ifdef WARNING
  fprintf(stderr, "nxagentRemoteToLocalAtom: WARNING failed to get name from remote atom.\n");
  #endif

  return None;
}

#ifdef DEBUG

static void nxagentPrintAtomMapInfo(char *message)
{
  unsigned i;

  fprintf(stderr, "--------------- Atom map in context [%s] ----------------------\n", message);
  fprintf(stderr, "nxagentPrintAtomMapInfo: Map at [%p] size [%d] number of entry [%d] auto increment [%d].\n",
              (void*) privAtomMap, privLastAtom, privAtomMapSize, NXAGENT_ATOM_MAP_SIZE_INCREMENT);

  for (i = 0; i < privLastAtom; i++)
  {
    fprintf(stderr, "[%5.1d] local: %6.1lu - remote: %6.1lu - [%p] %s\n", i,
                privAtomMap[i].local,
                    privAtomMap[i].remote,
                        privAtomMap[i].string, validateString(privAtomMap[i].string));
  }

  fprintf(stderr, "---------------------------------------------\n");
}

#endif
