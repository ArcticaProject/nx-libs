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

#if !defined(__CYGWIN32__) && !defined(WIN32)

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

#include <X11/Xlibint.h>
#include <X11/Xproto.h>
#include <X11/extensions/XTest.h>
#include <X11/keysym.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "Poller.h"
#include "Logger.h"
#include "Shadow.h"

#define ROUNDUP(nbytes, pad) ((((nbytes) + ((pad)-1)) / (pad)) * ((pad)>>3))

#undef  TRANSLATE_KEYCODES
#define TRANSLATE_ALWAYS

typedef struct {
    KeySym  *map;
    KeyCode minKeyCode,
            maxKeyCode;
    int     mapWidth;
} KeySymsRec, *KeySymsPtr;

extern KeySymsPtr NXShadowKeymap;

typedef struct _KeyPressed
{
  KeyCode keyRcvd;
  KeyCode keySent;
  struct _KeyPressed *next;
} KeyPressedRec;

static KeyPressedRec *shadowKeyPressedPtr = NULL;

static KeySym *shadowKeysyms = NULL;
static KeySym *masterKeysyms = NULL;

static KeySym *shadowKeymap = NULL;

static int shadowMinKey, shadowMaxKey, shadowMapWidth;
static int masterMinKey, masterMaxKey, masterMapWidth;

static int leftShiftOn = 0;
static int rightShiftOn = 0;
static int modeSwitchOn = 0;
static int level3ShiftOn = 0;
static int altROn = 0;

static int sentFakeLShiftPress = 0;
static int sentFakeLShiftRelease = 0;
static int sentFakeRShiftRelease = 0;
static int sentFakeModeSwitchPress = 0;
static int sentFakeModeSwitchRelease = 0;
static int sentFakeLevel3ShiftPress = 0;
static int sentFakeLevel3ShiftRelease = 0;
static int sentFakeAltRRelease = 0;

static int shmInitTrap = 0;

Poller::Poller(Input *input, Display *display, int depth) : CorePoller(input, display)
{
  logTrace("Poller::Poller");

  display_ = NULL;
  shadowDisplayName_ = input -> getShadowDisplayName();

  tmpBuffer_ = NULL;

  xtestExtension_  = -1;
  shmExtension_    = -1;
  randrExtension_  = -1;
  damageExtension_ = -1;

  shadowDisplayUid_ = -1;

  image_ = NULL;

  shminfo_ = NULL;
}

Poller::~Poller()
{
  logTrace("Poller::~Poller");

  if (shmExtension_ == 1)
  {
    XShmDetach(display_, shminfo_);
    XDestroyImage(image_);
    shmdt(shminfo_ -> shmaddr);
    shmctl(shminfo_ -> shmid, IPC_RMID, 0);
  }

  if (shminfo_ != NULL)
  {
    delete shminfo_;

    shminfo_ = NULL;
  }

  if (display_ != NULL)
  {
    XCloseDisplay(display_);
  }

  if (tmpBuffer_ != NULL && shmExtension_ != -1 && damageExtension_ == 1)
  {
    XFree(tmpBuffer_);

    tmpBuffer_ = NULL;
  }
}

int Poller::init()
{
  logTrace("Poller::init");

  if (display_ == NULL)
  {
    display_ = XOpenDisplay(shadowDisplayName_);

    setShadowDisplay(display_);
  }

  logTest("Poller::init:" ,"Shadow display [%p] name [%s].", (Display *) display_, shadowDisplayName_);

  if (display_ == NULL)
  {
    logTest("Poller::init", "Failed to connect to display [%s].", shadowDisplayName_ ? shadowDisplayName_ : "");

    return -1;
  }

  setRootSize();

  logTest("Poller::init", "Screen geometry is [%d, %d] depth is [%d] bpl [%d] bpp [%d].",
              width_, height_, depth_, bpl_, bpp_);

  xtestInit();

  shmInit();

  randrInit();

  damageInit();

  return CorePoller::init();
}

int Poller::updateShadowFrameBuffer(void)
{
  if (shmExtension_ == 1)
  {
    if (XShmGetImage(display_, DefaultRootWindow(display_), image_, 0, 0, AllPlanes) == 0)
    {
      logDebug("Poller::updateShadowFrameBuffer", "XShmGetImage failed!");

      return -1;
    }
  }
  else
  {
    image_ = XGetImage(display_, DefaultRootWindow(display_), 0, 0, width_,
                           height_, AllPlanes, ZPixmap);

    if (image_ == NULL)
    {
      logDebug("Poller::updateShadowFrameBuffer", "XGetImage failed!");

      return -1;
    }
  }

  return 1;
}

char *Poller::getRect(XRectangle r)
{
  logTrace("Poller::getRect");

  logDebug("Poller::getRect", "Going to retrive rectangle [%d, %d, %d, %d].",
              r.x, r.y, r.width, r.height);

  if (shmExtension_ == 1)
  {
    if (damageExtension_ == 1)
    {
      image_ -> width = r.width;
      image_ -> height = r.height;

      image_ -> bytes_per_line = ROUNDUP((image_ -> bits_per_pixel * image_ -> width), image_ -> bitmap_pad);

      if (XShmGetImage(display_, DefaultRootWindow(display_), image_, r.x, r.y, AllPlanes) == 0)
      {
        logDebug("Poller::getRect", "XShmGetImage failed!");

        return NULL;
      }

      tmpBuffer_ = image_ -> data;
    }
    else
    {
      image_ -> width  = r.width;
      image_ -> height = r.height; 

      image_ -> bytes_per_line = ROUNDUP((image_ -> bits_per_pixel * image_ -> width), image_ -> bitmap_pad);

      if (XShmGetImage(display_, DefaultRootWindow(display_), image_, r.x, r.y, AllPlanes) == 0)
      {
        logDebug("Poller::getRect", "XShmGetImage failed!");
      }

      tmpBuffer_ = image_ -> data;
    }
  }
  else
  {
    if (tmpBuffer_)
    {
      XFree(tmpBuffer_);
      tmpBuffer_ = NULL;
    }

    image_ = XGetImage(display_, DefaultRootWindow(display_), r.x, r.y, r.width, r.height, AllPlanes, ZPixmap);

    if (image_ == NULL)
    {
      logError("Poller::getRect", ESET(ENOMSG));

      return NULL;
    }

    tmpBuffer_ = image_ -> data;

    if (image_ -> obdata)
    {
      XFree(image_ -> obdata);
    }

    XFree(image_);

    image_ = NULL;
  }

  return tmpBuffer_;
}

void Poller::shmInit(void)
{
  int major, minor;
  int pixmaps;

  logTest("Poller::shmInit", "Added shmExtension_ [%d].", shmExtension_);

  if (shmExtension_ >= 0)
  {
    logDebug("Poller::shmInit", "Called with shared memory already initialized.");

    if (shmInitTrap == 0)
    {
      return;
    }
  }

  if (shmExtension_ < 0 && NXShadowOptions.optionShmExtension == 0)
  {
    shmExtension_ = 0;

    logUser("Poller::shmInit: Disabling use of MIT-SHM extension.\n");

    return;
  }

  if (XShmQueryVersion(display_, &major, &minor, &pixmaps) == 0)
  {
    logDebug("Poller::shmInit", "MIT_SHM: Shared memory extension not available.");

    shmExtension_ = 0;
  }
  else
  {
    logDebug("Poller::shmInit", "MIT_SHM: Shared memory extension available.");

    if (shminfo_ != NULL)
    {
      destroyShmImage();
    }

    shminfo_ = (XShmSegmentInfo* ) new XShmSegmentInfo;

    if (shminfo_ == NULL)
    {
      logError("Poller::shmInit", ESET(ENOMEM));

      shmExtension_ = 0;

      return;
    }

    image_ = (XImage *)XShmCreateImage(display_, display_ -> screens[0].root_visual, depth_, ZPixmap,
                                           NULL,  shminfo_, width_, height_);

    if (image_ == NULL)
    {
      logError("Poller::shmInit", ESET(ENOMSG));

      shmExtension_ = 0;

      return;
    }

    shadowDisplayUid_ = NXShadowOptions.optionShadowDisplayUid;

    logDebug("Poller::shmInit", "Master X server uid [%d].", NXShadowOptions.optionShadowDisplayUid);

    shminfo_ -> shmid = shmget(IPC_PRIVATE, image_ -> bytes_per_line * image_ -> height, IPC_CREAT | 0666);

    if (shminfo_ -> shmid < 0)
    {
      logDebug("Poller::shmInit", "kernel id error.");

      shmExtension_ = 0;

      return;
    }

    logDebug("Poller::shmInit", "Created shm segment with shmid [%d].", shminfo_ -> shmid);

    shminfo_ -> shmaddr = (char *)shmat(shminfo_ -> shmid, 0, 0);

    if (shminfo_ -> shmaddr < 0)
    {
      logWarning("Poller::shmInit", "Couldn't attach to shm segment.");
    }

    logDebug("Poller::shmInit", "shminfo_ -> shmaddr [%p].", shminfo_ -> shmaddr);

    image_ -> data = shminfo_ -> shmaddr;

    shminfo_ -> readOnly = 0;

    if (XShmAttach(display_, shminfo_) == 0)
    {
      logDebug("Poller::shmInit", "XShmAttach failed.");

      shmExtension_ = 0;

      return;
    }

    //
    // Mark the shm segment to be destroyed after
    // the last process detach. Let the X server
    // complete the X_ShmAttach request, before.
    //

    XSync(display_, 0);

    struct shmid_ds ds;

    shmctl(shminfo_ -> shmid, IPC_STAT, &ds);

    if (shadowDisplayUid_ != -1)
    {
      ds.shm_perm.uid = (ushort) shadowDisplayUid_;
    }
    else
    {
      logWarning("Poller::shmInit", "Couldn't set uid for shm segment.");
    }

    ds.shm_perm.mode = 0600;

    shmctl(shminfo_ -> shmid, IPC_SET, &ds);

    shmctl(shminfo_ -> shmid, IPC_STAT, &ds);

    shmctl(shminfo_ -> shmid, IPC_RMID, 0);

    logDebug("Poller::shmInit", "Number of attaches to shm segment [%d] are [%d].\n",
                 shminfo_ -> shmid, (int) ds.shm_nattch);

    if (ds.shm_nattch > 2)
    {
      logWarning("Poller::shmInit", "More than two attaches to the shm segment.");

      destroyShmImage();

      shmExtension_ = 0;

      return;
    }

    shmExtension_ = 1;
  }
}

void Poller::keymapShadowInit(Display *display)
{
  int i, len;
  CARD32 *map;

  if (NXShadowKeymap != NULL)
  {
    shadowMinKey   = NXShadowKeymap -> minKeyCode;
    shadowMaxKey   = NXShadowKeymap -> maxKeyCode;
    shadowMapWidth = NXShadowKeymap -> mapWidth;

    len = (shadowMaxKey - shadowMinKey + 1) * shadowMapWidth;

    map = (CARD32 *) NXShadowKeymap -> map;

    if (shadowKeymap != NULL)
    {
      free(shadowKeymap);
    }

    shadowKeymap = (KeySym *) malloc(len * sizeof(KeySym));

    if (shadowKeymap != NULL)
    {
      for (i = 0; i < len; i++)
      {
        shadowKeymap[i] = map[i];
      }

      shadowKeysyms = shadowKeymap;
    }
  }

  if (shadowKeysyms == NULL)
  {
    XDisplayKeycodes(display, &shadowMinKey, &shadowMaxKey);

    shadowKeysyms = XGetKeyboardMapping(display, shadowMinKey, shadowMaxKey - shadowMinKey + 1,
                                            &shadowMapWidth);
  }

  #ifdef DEBUG
  if (shadowKeysyms != NULL)
  {
    for (i = 0; i < (shadowMaxKey - shadowMinKey + 1) * shadowMapWidth; i++)
    {
      if (i % shadowMapWidth == 0)
      {
        logDebug("Poller::keymapShadowInit", "keycode [%d]", (int) (i / shadowMapWidth));
      }

      logDebug("\tkeysym", " [%x] [%s]", (unsigned int) shadowKeysyms[i], XKeysymToString(shadowKeysyms[i]));
    }
  }
  #endif
}

void Poller::keymapMasterInit()
{
  XDisplayKeycodes(display_, &masterMinKey, &masterMaxKey);

  masterKeysyms = XGetKeyboardMapping(display_, masterMinKey, masterMaxKey - masterMinKey + 1,
                                          &masterMapWidth);

  #ifdef DEBUG
  if (masterKeysyms != NULL)
  {
    for (int i = 0; i < (masterMaxKey - masterMinKey + 1) * masterMapWidth; i++)
    {
      if (i % masterMapWidth == 0)
      {
        logDebug("Poller::keymapMasterInit", "keycode [%d]", (int) (i / masterMapWidth));
      }

      logDebug("\tkeysym", " [%x] [%s]", (unsigned int) masterKeysyms[i], XKeysymToString(masterKeysyms[i]));
    }
  }
  #endif
}

KeySym Poller::keymapKeycodeToKeysym(KeyCode keycode, KeySym *keysyms,
                                         int minKey, int mapWidth, int col)
{
  int index = ((keycode - minKey) * mapWidth) + col;
  return keysyms[index];
}

KeyCode Poller::keymapKeysymToKeycode(KeySym keysym, KeySym *keysyms,
                                          int minKey, int maxKey, int mapWidth, int *col)
{
  for (int i = 0; i < (maxKey - minKey + 1) * mapWidth; i++)
  {
    if (keysyms[i] == keysym)
    {
      *col = i % mapWidth;
      return i / mapWidth + minKey;
    }
  }
  return 0;
}

KeyCode Poller::translateKeysymToKeycode(KeySym keysym, int *col)
{
  KeyCode keycode;

  keycode = keymapKeysymToKeycode(keysym, masterKeysyms, masterMinKey,
                                      masterMaxKey, masterMapWidth, col);

  if (keycode == 0)
  {
    if (((keysym >> 8) == 0) && (keysym >= XK_a) && (keysym <= XK_z))
    {
      /*
       * The master session has a Solaris keyboard.
       */

      keysym -= XK_a - XK_A;

      keycode = keymapKeysymToKeycode(keysym, masterKeysyms, masterMinKey,
                                          masterMaxKey, masterMapWidth, col);
    }
    else if (keysym == XK_Shift_R)
    {
      keysym = XK_Shift_L;

      keycode = keymapKeysymToKeycode(keysym, masterKeysyms, masterMinKey,
                                          masterMaxKey, masterMapWidth, col);
    }
    else if (keysym == XK_Shift_L)
    {
      keysym = XK_Shift_R;

      keycode = keymapKeysymToKeycode(keysym, masterKeysyms, masterMinKey,
                                          masterMaxKey, masterMapWidth, col);
    }
    else if (keysym == XK_ISO_Level3_Shift)
    {
      keysym = XK_Mode_switch;

      if ((keycode = keymapKeysymToKeycode(keysym, masterKeysyms, masterMinKey,
                                               masterMaxKey, masterMapWidth, col)) == 0)
      {
        keysym = XK_Alt_R;

        keycode = keymapKeysymToKeycode(keysym, masterKeysyms, masterMinKey,
                                            masterMaxKey, masterMapWidth, col);
      }
    }
    else if (keysym == XK_Alt_R)
    {
      keysym = XK_ISO_Level3_Shift;

      if ((keycode = keymapKeysymToKeycode(keysym, masterKeysyms, masterMinKey,
                                               masterMaxKey, masterMapWidth, col)) == 0)
      {
        keysym = XK_Mode_switch;

        keycode = keymapKeysymToKeycode(keysym, masterKeysyms, masterMinKey,
                                            masterMaxKey, masterMapWidth, col);
      }
    }
  }
  return keycode;
}

Bool Poller::checkModifierKeys(KeySym keysym, Bool isKeyPress)
{
  switch (keysym)
  {
    case XK_Shift_L:
      leftShiftOn = isKeyPress;
      return True;
    case XK_Shift_R:
      rightShiftOn = isKeyPress;
      return True;
    case XK_Mode_switch:
      modeSwitchOn = isKeyPress;
      return True;
    case XK_ISO_Level3_Shift:
      level3ShiftOn = isKeyPress;
      return True;
    case XK_Alt_R:
      altROn = isKeyPress;
      return True;
    default:
      return False;
  }
}

void Poller::sendFakeModifierEvents(int pos, Bool skip)
{
  KeySym fakeKeysym;
  int col;

  if ((!leftShiftOn && !rightShiftOn) &&
          (!modeSwitchOn && !level3ShiftOn && !altROn))
  {
    if (pos == 1 || pos == 3)
    {
      fakeKeysym = keymapKeysymToKeycode(XK_Shift_L, masterKeysyms, masterMinKey,
                                             masterMaxKey, masterMapWidth, &col);
      XTestFakeKeyEvent(display_, fakeKeysym, 1, 0);
      sentFakeLShiftPress = 1;
    }
    if (pos == 2 || pos == 3)
    {
      fakeKeysym = keymapKeysymToKeycode(XK_ISO_Level3_Shift, masterKeysyms, masterMinKey,
                                             masterMaxKey, masterMapWidth, &col);

      if (fakeKeysym == 0)
      {
        fakeKeysym = keymapKeysymToKeycode(XK_Mode_switch, masterKeysyms, masterMinKey,
                                               masterMaxKey, masterMapWidth, &col);
        sentFakeModeSwitchPress = 1;
      }
      else
      {
        sentFakeLevel3ShiftPress = 1;
      }

      XTestFakeKeyEvent(display_, fakeKeysym, 1, 0);
    }
  }

  else if ((leftShiftOn || rightShiftOn) &&
               (!modeSwitchOn && !level3ShiftOn && !altROn))
  {
    if ((pos == 0 && !skip) || pos == 2)
    {
      if (leftShiftOn)
      {
        fakeKeysym = keymapKeysymToKeycode(XK_Shift_L, masterKeysyms, masterMinKey,
                                               masterMaxKey, masterMapWidth, &col);
        XTestFakeKeyEvent(display_, fakeKeysym, 0, 0);
        sentFakeLShiftRelease = 1;
      }
      if (rightShiftOn)
      {
        fakeKeysym = keymapKeysymToKeycode(XK_Shift_R, masterKeysyms, masterMinKey,
                                               masterMaxKey, masterMapWidth, &col);
        XTestFakeKeyEvent(display_, fakeKeysym, 0, 0);
        sentFakeRShiftRelease = 1;
      }
    }
    if (pos == 2 || pos ==3)
    {
      fakeKeysym = keymapKeysymToKeycode(XK_ISO_Level3_Shift, masterKeysyms, masterMinKey,
                                             masterMaxKey, masterMapWidth, &col);

      if (fakeKeysym == 0)
      {
        fakeKeysym = keymapKeysymToKeycode(XK_Mode_switch, masterKeysyms, masterMinKey,
                                               masterMaxKey, masterMapWidth, &col);
        sentFakeModeSwitchPress = 1;
      }
      else
      {
        sentFakeLevel3ShiftPress = 1;
      }

      XTestFakeKeyEvent(display_, fakeKeysym, 1, 0);
    }
  }

  else if ((!leftShiftOn && !rightShiftOn) &&
               (modeSwitchOn || level3ShiftOn || altROn))
  {
    if (pos == 1 || pos == 3)
    {
      fakeKeysym = keymapKeysymToKeycode(XK_Shift_L, masterKeysyms, masterMinKey,
                                             masterMaxKey, masterMapWidth, &col);
      XTestFakeKeyEvent(display_, fakeKeysym, 1, 0);
      sentFakeLShiftPress = 1;
    }
    if (pos == 0 || pos == 1)
    {
      if (modeSwitchOn)
      {
        fakeKeysym = keymapKeysymToKeycode(XK_Mode_switch, masterKeysyms, masterMinKey,
                                               masterMaxKey, masterMapWidth, &col);
        XTestFakeKeyEvent(display_, fakeKeysym, 0, 0);
        sentFakeModeSwitchRelease = 1;
      }
      if (level3ShiftOn)
      {
        fakeKeysym = keymapKeysymToKeycode(XK_ISO_Level3_Shift, masterKeysyms, masterMinKey,
                                               masterMaxKey, masterMapWidth, &col);
        XTestFakeKeyEvent(display_, fakeKeysym, 0, 0);
        sentFakeLevel3ShiftRelease = 1;
      }
      if (altROn)
      {
        fakeKeysym = keymapKeysymToKeycode(XK_Alt_R, masterKeysyms, masterMinKey,
                                               masterMaxKey, masterMapWidth, &col);
        XTestFakeKeyEvent(display_, fakeKeysym, 0, 0);
        sentFakeAltRRelease = 1;
      }
    }
  }

  else if ((leftShiftOn || rightShiftOn) &&
               (modeSwitchOn || level3ShiftOn || altROn))
  {
    if (pos == 0 || pos == 2)
    {
      if (leftShiftOn)
      {
        fakeKeysym = keymapKeysymToKeycode(XK_Shift_L, masterKeysyms, masterMinKey,
                                               masterMaxKey, masterMapWidth, &col);
        XTestFakeKeyEvent(display_, fakeKeysym, 0, 0);
        sentFakeLShiftRelease = 1;
      }
      if (rightShiftOn)
      {
        fakeKeysym = keymapKeysymToKeycode(XK_Shift_R, masterKeysyms, masterMinKey,
                                               masterMaxKey, masterMapWidth, &col);
        XTestFakeKeyEvent(display_, fakeKeysym, 0, 0);
        sentFakeRShiftRelease = 1;
      }
    }
    if (pos == 0 || pos == 1)
    {
      if (modeSwitchOn)
      {
        fakeKeysym = keymapKeysymToKeycode(XK_Mode_switch, masterKeysyms, masterMinKey,
                                               masterMaxKey, masterMapWidth, &col);
        XTestFakeKeyEvent(display_, fakeKeysym, 0, 0);
        sentFakeModeSwitchRelease = 1;
      }
      if (level3ShiftOn)
      {
        fakeKeysym = keymapKeysymToKeycode(XK_ISO_Level3_Shift, masterKeysyms, masterMinKey,
                                               masterMaxKey, masterMapWidth, &col);
        XTestFakeKeyEvent(display_, fakeKeysym, 0, 0);
        sentFakeLevel3ShiftRelease = 1;
      }
      if (altROn)
      {
        fakeKeysym = keymapKeysymToKeycode(XK_Alt_R, masterKeysyms, masterMinKey,
                                               masterMaxKey, masterMapWidth, &col);
        XTestFakeKeyEvent(display_, fakeKeysym, 0, 0);
        sentFakeAltRRelease = 1;
      }
    }
  }
}

void Poller::cancelFakeModifierEvents()
{
  KeySym fakeKeysym;
  int col;

  if (sentFakeLShiftPress)
  {
    logTest("Poller::handleKeyboardEvent", "Fake Shift_L key press event has been sent");
    logTest("Poller::handleKeyboardEvent", "Sending fake Shift_L key release event");

    fakeKeysym = keymapKeysymToKeycode(XK_Shift_L, masterKeysyms, masterMinKey,
                                           masterMaxKey, masterMapWidth, &col);
    XTestFakeKeyEvent(display_, fakeKeysym, 0, 0);

    sentFakeLShiftPress = 0;
  }

  if (sentFakeLShiftRelease)
  {
    logTest("Poller::handleKeyboardEvent", "Fake Shift_L key release event has been sent");
    logTest("Poller::handleKeyboardEvent", "Sending fake Shift_L key press event");

    fakeKeysym = keymapKeysymToKeycode(XK_Shift_L, masterKeysyms, masterMinKey,
                                           masterMaxKey, masterMapWidth, &col);
    XTestFakeKeyEvent(display_, fakeKeysym, 1, 0);

    sentFakeLShiftRelease = 0;
  }

  if (sentFakeRShiftRelease)
  {
    logTest("Poller::handleKeyboardEvent", "Fake Shift_R key release event has been sent");
    logTest("Poller::handleKeyboardEvent", "Sending fake Shift_R key press event");

    fakeKeysym = keymapKeysymToKeycode(XK_Shift_R, masterKeysyms, masterMinKey,
                                           masterMaxKey, masterMapWidth, &col);
    XTestFakeKeyEvent(display_, fakeKeysym, 1, 0);

    sentFakeRShiftRelease = 0;
  }

  if (sentFakeModeSwitchPress)
  {
    logTest("Poller::handleKeyboardEvent", "Fake Mode_switch key press event has been sent");
    logTest("Poller::handleKeyboardEvent", "Sending fake Mode_switch key release event");

    fakeKeysym = keymapKeysymToKeycode(XK_Mode_switch, masterKeysyms, masterMinKey,
                                           masterMaxKey, masterMapWidth, &col);
    XTestFakeKeyEvent(display_, fakeKeysym, 0, 0);

    sentFakeModeSwitchPress = 0;
  }

  if (sentFakeModeSwitchRelease)
  {
    logTest("Poller::handleKeyboardEvent", "Fake Mode_switch key release event has been sent");
    logTest("Poller::handleKeyboardEvent", "Sending Mode_switch key press event");

    fakeKeysym = keymapKeysymToKeycode(XK_Mode_switch, masterKeysyms, masterMinKey,
                                           masterMaxKey, masterMapWidth, &col);
    XTestFakeKeyEvent(display_, fakeKeysym, 1, 0);

    sentFakeModeSwitchRelease = 0;
  }

  if (sentFakeLevel3ShiftPress)
  {
    logTest("Poller::handleKeyboardEvent", "Fake ISO_Level3_Shift key press event has been sent");
    logTest("Poller::handleKeyboardEvent", "Sending fake ISO_Level3_Shift key release event");

    fakeKeysym = keymapKeysymToKeycode(XK_ISO_Level3_Shift, masterKeysyms, masterMinKey,
                                           masterMaxKey, masterMapWidth, &col);
    XTestFakeKeyEvent(display_, fakeKeysym, 0, 0);

    sentFakeLevel3ShiftPress = 0;
  }

  if (sentFakeLevel3ShiftRelease)
  {
    logTest("Poller::handleKeyboardEvent", "Fake ISO_Level3_Shift key release event has been sent");
    logTest("Poller::handleKeyboardEvent", "Sending fake ISO_Level3_Shift key press event");

    fakeKeysym = keymapKeysymToKeycode(XK_ISO_Level3_Shift, masterKeysyms, masterMinKey,
                                           masterMaxKey, masterMapWidth, &col);
    XTestFakeKeyEvent(display_, fakeKeysym, 1, 0);

    sentFakeLevel3ShiftRelease = 0;
  }

  if (sentFakeAltRRelease)
  {
    logTest("Poller::handleKeyboardEvent", "Fake XK_Alt_R key release event has been sent");
    logTest("Poller::handleKeyboardEvent", "Sending fake XK_Alt_R key press event");

    fakeKeysym = keymapKeysymToKeycode(XK_Alt_R, masterKeysyms, masterMinKey,
                                           masterMaxKey, masterMapWidth, &col);
    XTestFakeKeyEvent(display_, fakeKeysym, 1, 0);

    sentFakeAltRRelease = 0;
  }
}

Bool Poller::keyIsDown(KeyCode keycode)
{
  KeyPressedRec *downKey;

  downKey = shadowKeyPressedPtr;

  while (downKey)
  {
    if (downKey -> keyRcvd == keycode)
    {
      return True;
    }
    downKey = downKey -> next;
  }

  return False;
}

void Poller::addKeyPressed(KeyCode received, KeyCode sent)
{
  KeyPressedRec *downKey;

  if (!keyIsDown(received))
  {
    if (shadowKeyPressedPtr == NULL)
    {
      shadowKeyPressedPtr = (KeyPressedRec *) malloc(sizeof(KeyPressedRec));

      shadowKeyPressedPtr -> keyRcvd = received;
      shadowKeyPressedPtr -> keySent = sent;
      shadowKeyPressedPtr -> next = NULL;
    }
    else
    {
      downKey = shadowKeyPressedPtr;

      while (downKey -> next != NULL)
      {
        downKey = downKey -> next;
      }

      downKey -> next = (KeyPressedRec *) malloc(sizeof(KeyPressedRec));

      downKey -> next -> keyRcvd = received;
      downKey -> next -> keySent = sent;
      downKey -> next -> next = NULL;
    }
  }
}

KeyCode Poller::getKeyPressed(KeyCode received)
{
  KeyCode sent;
  KeyPressedRec *downKey;
  KeyPressedRec *tempKey;

  if (shadowKeyPressedPtr != NULL)
  {
    if (shadowKeyPressedPtr -> keyRcvd == received)
    {
      sent = shadowKeyPressedPtr -> keySent;

      tempKey = shadowKeyPressedPtr;
      shadowKeyPressedPtr = shadowKeyPressedPtr -> next;
      free(tempKey);

      return sent;
    }
    else
    {
      downKey = shadowKeyPressedPtr;

      while (downKey -> next != NULL)
      {
        if (downKey -> next -> keyRcvd == received)
        {
          sent = downKey -> next -> keySent;

          tempKey = downKey -> next;
          downKey -> next = downKey -> next -> next;
          free(tempKey);

          return sent;
        }
        else
        {
          downKey = downKey -> next;
        }
      }
    }
  }
  return 0;
}

void Poller::handleKeyboardEvent(Display *display, XEvent *event)
{
  if (xtestExtension_ == 0 || display_ == 0)
  {
    return;
  }

  logTest("Poller::handleKeyboardEvent", "Handling event at [%p]", event);

#ifdef TRANSLATE_ALWAYS

  KeyCode keycode;
  KeySym keysym;

  int col = 0;

  Bool isKeyPress = False;
  Bool isModifier = False;
  Bool isShiftComb = False;
  Bool skip = False;

  if (event -> type == KeyPress)
  {
    isKeyPress = True;
  }

  if (shadowKeysyms == NULL)
  {
    keymapShadowInit(event -> xkey.display);
  }

  if (masterKeysyms == NULL)
  {
    keymapMasterInit();
  }

  if (shadowKeysyms == NULL || masterKeysyms == NULL)
  {
    logTest("Poller::handleKeyboardEvent", "Unable to initialize keymaps. Do not translate");

    keycode = event -> xkey.keycode;

    goto SendKeycode;
  }

  keysym = keymapKeycodeToKeysym(event -> xkey.keycode, shadowKeysyms,
                                   shadowMinKey, shadowMapWidth, 0);

  isModifier = checkModifierKeys(keysym, isKeyPress);

  if (event -> type == KeyRelease)
  {
    KeyCode keycodeToSend;

    keycodeToSend = getKeyPressed(event -> xkey.keycode);

    if (keycodeToSend)
    {
      keycode = keycodeToSend;

      goto SendKeycode;
    }
  }

  /*
   * Convert case for Solaris keyboard.
   */

  if (((keysym >> 8) == 0) && (keysym >= XK_A) && (keysym <= XK_Z))
  {
    if (!leftShiftOn && !rightShiftOn)
    {
      keysym += XK_a - XK_A;
    }
    else
    {
      skip = True;
    }
  }

  if (!isModifier)
  {
    if ((leftShiftOn || rightShiftOn) &&
            (!modeSwitchOn && !level3ShiftOn && !altROn) &&
                !skip)
    {
      KeySym tempKeysym = keymapKeycodeToKeysym(event -> xkey.keycode, shadowKeysyms,
                                                    shadowMinKey, shadowMapWidth, 1);

      if (tempKeysym == 0)
      {
        isShiftComb = True;
      }
      else
      {
        keysym = tempKeysym;
      }
    }
    else if ((!leftShiftOn && !rightShiftOn) &&
                 (modeSwitchOn || level3ShiftOn || altROn))
    {
      keysym = keymapKeycodeToKeysym(event -> xkey.keycode, shadowKeysyms,
                                         shadowMinKey, shadowMapWidth, 2);
    }
    else if ((leftShiftOn || rightShiftOn) &&
                 (modeSwitchOn || level3ShiftOn || altROn))
    {
      keysym = keymapKeycodeToKeysym(event -> xkey.keycode, shadowKeysyms,
                                       shadowMinKey, shadowMapWidth, 3);
    }
  }

  if (keysym == 0)
  {
    logTest("Poller::handleKeyboardEvent", "Null keysym. Return");

    return;
  }

  logTest("Poller::handleKeyboardEvent", "keysym [%x] [%s]",
              (unsigned int)keysym, XKeysymToString(keysym));

  if (keysym == XK_Mode_switch)
  {
    keysym = XK_ISO_Level3_Shift;
  }

  keycode = translateKeysymToKeycode(keysym, &col);

  if (keycode == 0)
  {
    logTest("Poller::handleKeyboardEvent", "No keycode found for keysym [%x] [%s]. Return",
                (unsigned int)keysym, XKeysymToString(keysym));
    return;
  }

  logTest("Poller::handleKeyboardEvent", "keycode [%d] translated into keycode [%d]",
              (int)event -> xkey.keycode, (unsigned int)keycode);

  if (event -> type == KeyPress)
  {
    addKeyPressed(event -> xkey.keycode, keycode);
  }

  /*
   * Send fake modifier events.
   */

  if (!isModifier && isShiftComb == False)
  {
    sendFakeModifierEvents(col, ((keysym >> 8) == 0) && (keysym >= XK_A) && (keysym <= XK_Z));
  }

SendKeycode:

  /*
   * Send the event.
   */

  XTestFakeKeyEvent(display_, keycode, isKeyPress, 0);

  /*
   * Check if fake modifier events have been sent.
   */

  cancelFakeModifierEvents();

#else // TRANSLATE_ALWAYS

  //
  // Use keysyms to translate keycodes across different
  // keyboard models. Unuseful when both keyboards have
  // same keycodes (e.g. both are pc keyboards).
  //

  #ifdef TRANSLATE_KEYCODES

  KeyCode keycode = XKeysymToKeycode(display_, XK_A);

  if (XKeysymToKeycode(event -> xkey.display, XK_A) != keycode)
  {
    KeySym keysym = XKeycodeToKeysym(event -> xkey.display, event -> xkey.keycode, 0);
  
    if (keysym == XK_Mode_switch || keysym == XK_ISO_Level3_Shift)
    {
      logUser("Poller::handleKeyboardEvent: keysym [%x].\n", (unsigned int)keysym);
  
      if (XKeycodeToKeysym(display_, 113, 0) == XK_ISO_Level3_Shift ||
             (XKeycodeToKeysym(display_, 124, 0) == XK_ISO_Level3_Shift))
      {
        event -> xkey.keycode = 113;
      }
      else
      {
        event -> xkey.keycode = XKeysymToKeycode(display_, XK_Mode_switch);
      }
  
      logUser("Poller::handleKeyboardEvent: keycode translated to [%x].\n", (unsigned int)event -> xkey.keycode);
    }
    else
    {
      event -> xkey.keycode = XKeysymToKeycode(display_, keysym);
    }
  }

  #endif // TRANSLATE_KEYCODES

  if (event -> type == KeyPress)
  {
    XTestFakeKeyEvent(display_, event -> xkey.keycode, 1, 0);
  }
  else if (event -> type == KeyRelease)
  {
    XTestFakeKeyEvent(display_, event -> xkey.keycode, 0, 0);
  }

#endif // TRANSLATE_ALWAYS
}

void Poller::handleWebKeyboardEvent(KeySym keysym, Bool isKeyPress)
{
  KeyCode keycode;
  int col;

  if (masterKeysyms == NULL)
  {
    keymapMasterInit();
  }

  if (masterKeysyms == NULL)
  {
    logTest("Poller::handleWebKeyboardEvent", "Unable to initialize keymap");

    return;
  }

  keycode = translateKeysymToKeycode(keysym, &col);

  if (keycode == 0)
  {
    logTest("Poller::handleKeyboardEvent", "No keycode found for keysym [%x] [%s]. Return",
                (unsigned int)keysym, XKeysymToString(keysym));
    return;
  }

  logTest("Poller::handleKeyboardEvent", "keysym [%x] [%s] translated into keycode [%x]",
              (unsigned int)keysym, XKeysymToString(keysym), (unsigned int)keycode);

  /*
   * Send fake modifier events.
   */

  if (!checkModifierKeys(keysym, isKeyPress))
  {
    sendFakeModifierEvents(col, False);
  }

  /*
   * Send the event.
   */

  XTestFakeKeyEvent(display_, keycode, isKeyPress, 0);

  /*
   * Check if fake modifier events have been sent.
   */

  cancelFakeModifierEvents();

}

void Poller::handleMouseEvent(Display *display, XEvent *event)
{
  if (xtestExtension_ == 0 || display_ == 0)
  {
    return;
  }

  if (event -> type == MotionNotify)
  {
    XTestFakeMotionEvent(display_, 0, event -> xmotion.x, event -> xmotion.y, 0);
  }
  else if (event -> type == ButtonPress)
  {
    XTestFakeButtonEvent(display_, event -> xbutton.button, True, 0);
  }
  else if (event -> type == ButtonRelease)
  {
    XTestFakeButtonEvent(display_, event -> xbutton.button, False, 0);
  }

  XFlush(display_);
}

void Poller::setRootSize(void)
{
  width_ = WidthOfScreen(DefaultScreenOfDisplay(display_));
  height_ = HeightOfScreen(DefaultScreenOfDisplay(display_));
  depth_ = DefaultDepth(display_, DefaultScreen(display_));

  if (depth_ == 8) bpp_ = 1;
  else if (depth_ == 16) bpp_ = 2;
  else bpp_ = 4;

  bpl_ = width_ * bpp_;
}

void Poller::destroyShmImage(void)
{
    XShmDetach(display_, shminfo_);
    XDestroyImage(image_);
    image_ = NULL;

    shmdt(shminfo_ -> shmaddr);
    shmctl(shminfo_ -> shmid, IPC_RMID, 0);

    delete shminfo_;
    shminfo_ = NULL;
}

void Poller::xtestInit(void)
{
  int eventBase;
  int errorBase;
  int versionMajor;
  int versionMinor;
  int result;

  xtestExtension_ = 0;

  result = XTestQueryExtension(display_, &eventBase, &errorBase, &versionMajor, &versionMinor);

  if (result == 0)
  {
    xtestExtension_ = 0;

    logWarning("Poller::xtestInit", "Failed while querying for XTEST extension.");
  }
  else
  {
    logDebug("Poller::xtestInit", "XTEST version %d.%d.", versionMajor, versionMinor);

    xtestExtension_ = 1;
  }

  //
  // Make this client impervious to grabs.
  //

  if (xtestExtension_ == 1)
  {
    XTestGrabControl(display_, 1);
  }
}

void Poller::randrInit(void)
{
  int randrEventBase;
  int randrErrorBase;

  if (XRRQueryExtension(display_, &randrEventBase, &randrErrorBase) == 0)
  {
    logWarning("Poller::randrInit", "Randr extension not supported on this "
                   "display.");

    randrExtension_ = 0;

    return;
  }

  XRRSelectInput(display_, DefaultRootWindow(display_),
                     RRScreenChangeNotifyMask);

  randrEventBase_ = randrEventBase;

  randrExtension_ = 1;

  return;
}

void Poller::damageInit(void)
{
  int damageMajorVersion = 0;
  int damageMinorVersion = 0;

  int damageEventBase = 0;
  int damageErrorBase = 0;

  if (damageExtension_ >= 0)
  {
    logDebug("Poller::damageInit", "Called with damage already initialized.");
  }

  if (damageExtension_ == 0)
  {
    logDebug("Poller::damageInit", "Damage disabled. Skip initialization.");

    return;
  }

  if (damageExtension_ < 0 && NXShadowOptions.optionDamageExtension == 0)
  {
    damageExtension_ = 0;

    logUser("Poller::damageInit: Disabling use of DAMAGE extension.\n");

    return;
  }

  damageExtension_ = 0;

  mirrorChanges_ = 0;

  if (XDamageQueryExtension(display_, &damageEventBase, &damageErrorBase) == 0)
  {
    logUser("Poller::damageInit: DAMAGE not supported.\n");

    return;
  }
  #ifdef DEBUG
  else
  {
    fprintf(stderr, "Poller::damageInit: DAMAGE supported. "
                "Event base [%d] error base [%d].\n", damageEventBase, damageErrorBase);
  }
  #endif

  damageEventBase_ = damageEventBase;

  if (XDamageQueryVersion(display_, &damageMajorVersion, &damageMinorVersion) == 0)
  {
    logWarning("Poller::damageInit", "Error on querying DAMAGE version.\n");

    damageExtension_ = 0;

    return;
  }
  #ifdef DEBUG
  else
  {
    fprintf(stderr, "Poller::damageInit: DAMAGE version %d.%d.\n",
                damageMajorVersion, damageMinorVersion);
  }
  #endif

  damage_ = XDamageCreate(display_, DefaultRootWindow(display_), XDamageReportRawRectangles);

  damageExtension_= 1;

  mirror_ = 1;

  return;
}

void Poller::getEvents(void)
{
  XEvent X;

  if (damageExtension_ == 1)
  {
    XDamageSubtract(display_, damage_, None, None);
  }

  XSync(display_, 0);

  while (XCheckIfEvent(display_, &X, anyEventPredicate, NULL) == 1)
  {
    if (randrExtension_ == 1 && (X.type == randrEventBase_ + RRScreenChangeNotify || X.type == ConfigureNotify))
    {
      XRRUpdateConfiguration(&X);

      handleRRScreenChangeNotify(&X);

      continue;
    }

    if (damageExtension_ == 1 && X.type == damageEventBase_ + XDamageNotify)
    {
      handleDamageNotify(&X);
    }
  }

  if (damageExtension_ == 1)
  {
    updateDamagedAreas();
  }

  XFlush(display_);
}

void Poller::handleRRScreenChangeNotify(XEvent *X)
{
  return;
}

void Poller::handleDamageNotify(XEvent *X)
{
  XDamageNotifyEvent *e = (XDamageNotifyEvent *) X;

  //
  //  e->drawable is the window ID of the damaged window
  //  e->geometry is the geometry of the damaged window
  //  e->area     is the bounding rect for the damaged area
  //  e->damage   is the damage handle returned by XDamageCreate()
  //

  #ifdef DEBUG
  fprintf(stderr, "handleDamageNotify: drawable [%d] damage [%d] geometry [%d][%d][%d][%d] area [%d][%d][%d][%d].\n",
              (int) e -> drawable, (int) e -> damage, e -> geometry.x, e -> geometry.y,
                  e -> geometry.width, e -> geometry.height, e -> area.x, e -> area.y,
                      e -> area.width, e -> area.height);
  #endif

  XRectangle rectangle = {e -> area.x, e -> area.y, e -> area.width, e -> area.height};

  XUnionRectWithRegion(&rectangle, lastUpdatedRegion_, lastUpdatedRegion_);

  mirrorChanges_ = 1;

  return;
}

void Poller::updateDamagedAreas(void)
{
  BOX *boxPtr;

  XRectangle rectangle;

  int i;
  int y;
  
  for (i = 0; i < lastUpdatedRegion_ -> numRects; i++)
  {
    boxPtr = lastUpdatedRegion_ -> rects + i;

    if (shmExtension_ == 1)
    {
      image_ -> width  = boxPtr -> x2 - boxPtr -> x1;
      image_ -> height = boxPtr -> y2 - boxPtr -> y1;
      image_ -> bytes_per_line =
          ROUNDUP((image_ -> bits_per_pixel * image_ -> width),
                      image_ -> bitmap_pad);
      
      if (XShmGetImage(display_, DefaultRootWindow(display_), image_,
                           boxPtr -> x1, boxPtr -> y1, AllPlanes) == 0)
      {
        logDebug("Poller::updateDamagedAreas", "XShmGetImage failed!");

        return;
      }
    }
    else if (shmExtension_ == 0)
    {
      image_ = XGetImage(display_, DefaultRootWindow(display_), boxPtr -> x1,
                             boxPtr -> y1, boxPtr -> x2 - boxPtr -> x1,
                                 boxPtr -> y2 - boxPtr -> y1, AllPlanes,
                                     ZPixmap);

      if (image_ == NULL)
      {
        logDebug("Poller::updateDamagedAreas", "XGetImage failed!");

        return;
      }

      image_ -> width  = boxPtr -> x2 - boxPtr -> x1;
      image_ -> height = boxPtr -> y2 - boxPtr -> y1;
      image_ -> bytes_per_line =
          ROUNDUP((image_ -> bits_per_pixel * image_ -> width),
                      image_ -> bitmap_pad);
    }

    rectangle.height = 1;
    rectangle.width = image_ -> width;
    rectangle.x = boxPtr -> x1;
    rectangle.y = boxPtr -> y1;

    for (y = 0; y < image_ -> height; y++)
    {
      update(image_ -> data + y * image_ -> bytes_per_line, rectangle);

      rectangle.y++; 
    }

    if (shmExtension_ != 1)
    {
       XDestroyImage(image_);

      image_ = NULL;
    }
  }

  return;
}

void Poller::getScreenSize(int *w, int *h)
{
  *w = WidthOfScreen(DefaultScreenOfDisplay(display_));
  *h = HeightOfScreen(DefaultScreenOfDisplay(display_));
}

void Poller::setScreenSize(int *w, int *h)
{
  setRootSize();

  shmInitTrap = 1;
  shmInit();
  shmInitTrap = 0;

  *w = width_;
  *h = height_;

  logDebug("Poller::setScreenSize", "New size of screen [%d, %d]", width_, height_);
}

int anyEventPredicate(Display *display, XEvent *event, XPointer parameter)
{
  return 1;
}

#endif /* !defined(__CYGWIN32__) && !defined(WIN32) */
