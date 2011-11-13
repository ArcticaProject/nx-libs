/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2007 NoMachine, http://www.nomachine.com/.         */
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
#include <X11/extensions/XTest.h>
#include <X11/keysym.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "Poller.h"
#include "Logger.h"
#include "Shadow.h"

#define ROUNDUP(nbytes, pad) ((((nbytes) + ((pad)-1)) / (pad)) * ((pad)>>3))

#define TRANSLATE_KEYCODES

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

  if (tmpBuffer_ != NULL && shmExtension_ == 1 && damageExtension_ == 1)
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

    return 1;
  }
  else
  {
    return 0;
  }
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

    return;
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

void Poller::handleKeyboardEvent(Display *display, XEvent *event)
{
  if (xtestExtension_ == 0 || display_ == 0)
  {
    return;
  }

  logTest("Poller::handleKeyboardEvent", "Handling event at [%p]", event);

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

  randrExtension_ = 0;

  XRRSelectInput(display_, DefaultRootWindow(display_), RRScreenChangeNotifyMask);

  if (XRRQueryExtension(display_, &randrEventBase, &randrErrorBase) == 0)
  {
    #ifdef PANIC
    fprintf(stderr, "nxagentShadowInit: Randr extension not supported on this display.\n");
    #endif
  }

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
      XRRUpdateConfiguration (&X);

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
  if (shmExtension_ == 1)
  {
    BOX *boxPtr;

    XRectangle rectangle;

    int i;
    int y;

    for (i = 0; i < lastUpdatedRegion_ -> numRects; i++)
    {
      boxPtr = lastUpdatedRegion_ -> rects + i;

      image_ -> width  = boxPtr -> x2 - boxPtr -> x1;
      image_ -> height = boxPtr -> y2 - boxPtr -> y1; 

      image_ -> bytes_per_line = ROUNDUP((image_ -> bits_per_pixel * image_ -> width), image_ -> bitmap_pad);

      if (XShmGetImage(display_, DefaultRootWindow(display_), image_, boxPtr -> x1, boxPtr -> y1, AllPlanes) == 0)
      {
        logDebug("Poller::getRect", "XShmGetImage failed!");

        return;
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
    }
  }

  return;
}

int anyEventPredicate(Display *display, XEvent *event, XPointer parameter)
{
  return 1;
}

#endif /* !defined(__CYGWIN32__) && !defined(WIN32) */
