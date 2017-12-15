/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2011 NoMachine (http://www.nomachine.com)          */
/* Copyright (c) 2008-2014 Oleksandr Shneyder <o.shneyder@phoca-gmbh.de>  */
/* Copyright (c) 2014-2016 Ulrich Sibiller <uli42@gmx.de>                 */
/* Copyright (c) 2014-2016 Mihai Moldovan <ionic@ionic.de>                */
/* Copyright (c) 2011-2016 Mike Gabriel <mike.gabriel@das-netzwerkteam.de>*/
/* Copyright (c) 2015-2016 Qindel Group (http://www.qindel.com)           */
/*                                                                        */
/* NXCOMPSHAD, NX protocol compression and NX extensions to this software */
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

#ifndef Input_H
#define Input_H

#include <nx-X11/Xlib.h>

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
