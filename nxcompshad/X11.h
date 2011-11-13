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

  void handleKeyboardEvent(Display *display, XEvent *);

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
