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

#ifndef X11Poller_H
#define X11Poller_H

#include <X11/Xlib.h>
#include <X11/extensions/XShm.h>
#include <X11/extensions/Xdamage.h>
#include <X11/extensions/Xrandr.h>

#include "Core.h"

class Poller : public CorePoller
{
  public:

  Poller(Input *, Display *display, int = 0);

  ~Poller();

  int init();

  void setRootSize();

  void destroyShmImage();

  void getEvents(void);

  void getScreenSize(int *width, int *height);

  void setScreenSize(int *width, int *height);

  private:

  Display *display_;

  char *shadowDisplayName_;

  int shadowDisplayUid_;

  char *tmpBuffer_;

  char xtestExtension_;

  char shmExtension_;

  char randrExtension_;

  int randrEventBase_;

  char damageExtension_;

  int damageEventBase_;

  Damage damage_;

  Region repair_;

  char damageChanges_;

  XShmSegmentInfo *shminfo_;

  XImage *image_;

  int updateShadowFrameBuffer(void);

  char *getRect(XRectangle);

  void keymapShadowInit(Display *display);

  void keymapMasterInit();

  KeySym keymapKeycodeToKeysym(KeyCode keycode, KeySym *keysyms,
                                   int minKey, int per, int col);

  KeyCode keymapKeysymToKeycode(KeySym keysym, KeySym *keysyms,
                                    int minKey, int maxKey, int per, int *col);

  KeyCode translateKeysymToKeycode(KeySym keysym, int *col);

  Bool checkModifierKeys(KeySym keysym, Bool isKeyPress);

  void sendFakeModifierEvents(int pos, Bool skip);

  void cancelFakeModifierEvents();

  Bool keyIsDown(KeyCode keycode);

  void addKeyPressed(KeyCode received, KeyCode sent);

  KeyCode getKeyPressed(KeyCode received);

  void handleKeyboardEvent(Display *display, XEvent *);

  void handleWebKeyboardEvent(KeySym keysym, Bool isKeyPress);

  void handleMouseEvent(Display *, XEvent *);

  void xtestInit(void);

  void shmInit(void);

  void randrInit(void);

  void damageInit(void);

  void handleRRScreenChangeNotify(XEvent *);

  void handleDamageNotify(XEvent *);

  void updateDamagedAreas(void);
};

int anyEventPredicate(Display *display, XEvent *event, XPointer parameter);

#endif /* X11Poller_H */
