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

#ifndef Input_H
#define Input_H

#include <X11/Xlib.h>

typedef struct Event
{
  struct Event *next;
  Display *display;
  XEvent *event;
} Event;

class Input
{
  public:

  Input();

  ~Input();

  int checkIfEvent();

  void pushEvent(Display *, XEvent *);

  XEvent *popEvent();
  Display *currentDisplay();

  int removeAllEvents(Display *);

  void setKeymap(char *keymap);
  char *getKeymap();

  void setShadowDisplayName(char *shadowDisplayName);
  char *getShadowDisplayName();

  private:

  Event *eventsHead_;
  Event *eventsTail_;
  char *keymap_;
  char *shadowDisplayName_;
};

inline Display *Input::currentDisplay()
{
  return eventsHead_ ? eventsHead_ -> display : NULL;
}

inline int Input::checkIfEvent()
{
  return (eventsHead_ != NULL);
}

inline void Input::setKeymap(char *keymap)
{
  keymap_ = keymap;
}

inline char *Input::getKeymap()
{
  return keymap_;
}

inline void Input::setShadowDisplayName(char *shadowDisplayName)
{
  shadowDisplayName_ = shadowDisplayName;
}

inline char *Input::getShadowDisplayName()
{
  return shadowDisplayName_;
}

#endif /* Input_H */
