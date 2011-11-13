/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2011 NoMachine, http://www.nomachine.com/.         */
/*                                                                        */
/* NXCOMPSHAD, NX protocol compression and NX extensions to this software */
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
#include <string.h>

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

#include "Logger.h"
#include "Shadow.h"
#include "Poller.h"
#include "Manager.h"

typedef struct {
    KeySym  *map;
    KeyCode minKeyCode,
            maxKeyCode;
    int     mapWidth;
} KeySymsRec, *KeySymsPtr;

KeySymsPtr NXShadowKeymap = NULL;

ShadowOptions NXShadowOptions = {1, 1, -1};

static int mirrorException = 0;

static UpdateManager *updateManager;
static Poller *poller;
static Input *input;

int NXShadowRemoveAllUpdaters();

inline bool NXShadowNotInitialized()
{
  //
  // updateManager depends on input and poller.
  // So this test seem redundant.
  //
  // return (input == NULL) || (poller == NULL) || (updateManager == NULL);
  //

  return (updateManager == NULL);
}

#ifdef NEED_SIGNAL_HANDLER
static void NXSignalHandler(int signal)
{
  logTest("NXSignalHandler", "Got signal [%d]", signal);

  if (signal == SIGINT)
  {
    mirrorException = 1;
  }
  else if (signal == SIGTERM)
  {
    mirrorException = 1;
  }
}

static int NXInitSignal()
{
  logTrace("NXInitSignal");

  struct sigaction sa;

  sa.sa_handler = NXSignalHandler;
  sigfillset(&sa.sa_mask);
  sa.sa_flags = 0;

  int res;

  while ((res = sigaction(SIGINT, &sa, NULL)) == -1 &&
             errno == EINTR);

  if (res == -1)
  {
    logError("NXInitSignal", EGET());

    return -1;
  }

  return 1;
}
#endif

static void NXHandleException()
{
  if (mirrorException)
  {
    mirrorException = 0;

    NXShadowRemoveAllUpdaters();
  }
}

static int NXCreateInput(char *keymap, char *shadowDisplayName)
{
  logTrace("NXCreateInput");

  input = new Input;

  if (input == NULL)
  {
    logError("NXCreateInput", ESET(ENOMEM));

    return -1;
  }

  input -> setKeymap(keymap);

  input -> setShadowDisplayName(shadowDisplayName);

  return 1;
}

static int NXCreatePoller(Display *display, Display **shadowDisplay)
{
  logTrace("NXCreatePoller");

  if (input == NULL)
  {
    logError("NXCreatePoller", ESET(EBADFD));

    return -1;
  }

  poller = new Poller(input,display);

  if (poller == NULL)
  {
    logError("NXCreatePoller", ESET(ENOMEM));

    return -1;
  }

  if (poller -> init() == -1)
  {
    logWarning("NXCreatePoller", "Failed to initialize poller.");

    return -1;
  }

  *shadowDisplay = poller -> getShadowDisplay();

  logTest("NXCreatePoller", "Poller geometry [%d, %d], ShadowDisplay[%p].", poller -> width(),
              poller -> height(), (Display *) *shadowDisplay);

  return 1;
}

static int NXCreateUpdateManager()
{
  logTrace("NXCreateUpdateManager");

  if (input == NULL || poller == NULL)
  {
    logError("NXCreateUpdateManager", ESET(EBADFD));

    return -1;
  }

  updateManager = new UpdateManager(poller -> width(), poller -> height(),
                                        poller -> getFrameBuffer(), input);

  if (updateManager == NULL)
  {
    logError("NXCreateUpdateManager", ESET(ENOMEM));

    return -1;
  }

  return 1;
}

void  NXShadowResetOptions()
{
  NXShadowOptions.optionShmExtension = 1;
  NXShadowOptions.optionDamageExtension = 1;
}

//
// Exported functions.
//

int NXShadowHasUpdaters()
{
  logTrace("NXShadowHasUpdaters");

  return (updateManager && updateManager -> numberOfUpdaters()) ? 1 : 0;
}

int NXShadowRemoveAllUpdaters()
{
  logTrace("NXShadowRemoveAllUpdaters");

  return updateManager ? updateManager -> removeAllUpdaters() : 0;
}

int NXShadowRemoveUpdater(UpdaterHandle handle)
{
  logTrace("NXShadowRemoveUpdater");

  return updateManager ? updateManager -> removeUpdater(handle) : 0;
}

UpdaterHandle NXShadowAddUpdater(char *displayName)
{
  logTrace("NXShadowAddUpdater");

  return updateManager ? updateManager -> addUpdater(displayName, NULL) : NULL;
}

int NXShadowAddUpdaterDisplay(void *dpy, int *w, int *h, unsigned char *d)
{
  Display *display = reinterpret_cast<Display*>(dpy);

  logTrace("NXShadowAddUpdaterDisplay");

  if ((updateManager ? updateManager -> addUpdater(NULL, display) : NULL) == NULL)
  {
    logTest("NXShadowAddUpdaterDisplay", "Error");

    return 0;
  }

  *w = updateManager -> getWidth();
  *h = updateManager -> getHeight();
  *d = poller -> depth();

  return 1;
}

int NXShadowCreate(void *dpy, char *keymap, char* shadowDisplayName, void **shadowDpy)
{
  logTrace("NXShadowCreate");

  Display *display = reinterpret_cast<Display*>(dpy);
  Display **shadowDisplay = reinterpret_cast<Display**>(shadowDpy);

/*  if (NXInitSignal() != 1)
  {
    logError("NXShadowCreate", EGET());

    return -1;
  }*/

  if (NXCreateInput(keymap, shadowDisplayName) != 1)
  {
    logError("NXShadowCreate", EGET());

    return -1;
  }

  if (NXCreatePoller(display, shadowDisplay) != 1)
  {
    logWarning("NXShadowCreate", "NXCreatePoller failed.");

    return -1;
  }

  if (NXCreateUpdateManager() != 1)
  {
    logError("NXShadowCreate", EGET());

    return -1;
  }

  return 1;
}

#if !defined(__CYGWIN32__) && !defined(WIN32)

void NXShadowSetDisplayUid(int uid)
{
  NXShadowOptions.optionShadowDisplayUid = uid;
}

void NXShadowDisableShm(void)
{
  logUser("NXShadowDisableShm: Disabling SHM.\n");

  NXShadowOptions.optionShmExtension = 0;
}

void NXShadowDisableDamage(void)
{
  NXShadowOptions.optionDamageExtension = 0;
}

void NXShadowGetScreenSize(int *w, int *h)
{
  poller -> getScreenSize(w, h);
}

void NXShadowSetScreenSize(int *w, int *h)
{
  poller -> setScreenSize(w, h);
}

#endif

void NXShadowDestroy()
{
  if (poller)
  {
    delete poller;

    poller = NULL;
  }

  if (updateManager)
  {
    delete updateManager;

    updateManager = NULL;
  }

  if (input)
  {
    delete input;

    input = NULL;
  }
}

void NXShadowHandleInput()
{
  logTrace("NXShadowHandleInput");

  if (NXShadowNotInitialized())
  {
    logError("NXShadowHandleInput - NXShadow not properly initialized.", ESET(EBADFD));

    return;
  }

  NXHandleException();

  updateManager -> handleInput();

  poller -> handleInput();
}

int NXShadowHasChanged(int (*callback)(void *), void *arg, int *suspended)
{
  int result;

  logTrace("NXShadowHasChanged");

  if (NXShadowNotInitialized())
  {
    logError("NXShadowHasChanged - NXShadow not properly initialized.", ESET(EBADFD));

    return -1;
  }

  //
  // FIXME
  //updateManager -> destroyUpdateManagerRegion();
  //

  updateManager -> newRegion();

#if !defined(__CYGWIN32__) && !defined(WIN32)
  poller -> getEvents();
#endif

  result = poller -> isChanged(callback, arg, suspended);

  if (result == 1)
  {
    updateManager -> addRegion(poller -> lastUpdatedRegion());

    return 1;
  }
  else if (result == -1)
  {
    logTest("NXShadowHasChanged", "Scanline error.");
    return -1;
  }

  return 0;
}

void NXShadowExportChanges(long *numRects, char **pBox)
{
  Region pReg;

  logTrace("NXShadowExportChanges");

  if (NXShadowNotInitialized())
  {
    logError("NXShadowExportChanges - NXShadow not properly initialized.", ESET(EBADFD));
  }

  updateManager -> update();
  pReg = updateManager -> getUpdateManagerRegion();
  *numRects = pReg -> numRects;
  *pBox = (char *)pReg -> rects;

  logTest("NXShadowExportChanges", "numRects [%ld] pBox[%p], pReg->numRects[%ld], rects[%p], size[%lu]",
              *numRects, *pBox, pReg -> numRects, &(pReg -> rects -> x2),
                  (unsigned long) sizeof(pReg -> rects -> x2));
}

void NXShadowEvent(Display *display, XEvent event)
{
  poller -> handleEvent(display, &event);
}

void NXShadowWebKeyEvent(KeySym keysym, Bool isKeyPress)
{
  poller -> handleWebKeyEvent(keysym, isKeyPress);
}

#ifdef __CYGWIN32__

int NXShadowCaptureCursor(unsigned int wnd, void *vis)
{
  Window window = (Window)wnd;
  Visual *visual = reinterpret_cast<Visual*>(vis);

  logTrace("NXShadowCaptureCursor");

  logTest("NXShadowCaptureCursor","Init");

  return poller -> updateCursor(window, visual);
}

#endif

void NXShadowUpdateBuffer(void **buffer)
{
  char **fBuffer = reinterpret_cast<char **>(buffer);

  if (*fBuffer != NULL)
  {
    poller -> destroyFrameBuffer();

    poller -> init();
  }

  *fBuffer = poller -> getFrameBuffer();

  logTest("NXShadowUpdateBuffer","New frame buffer [0x%p]", (void *)*fBuffer);
}

void NXShadowInitKeymap(void *keysyms)
{
  NXShadowKeymap = (KeySymsPtr) keysyms;

  logTest("NXShadowInitKeymap","KeySyms pointer [0x%p]", (void *)NXShadowKeymap);
}
