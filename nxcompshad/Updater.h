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

#ifndef Updater_H
#define Updater_H

#include <X11/Xlib.h>

#include "Regions.h"
#include "Input.h"

class UpdaterClosing {};

class Updater
{
  public:

  Updater(char *displayName, Display *display);

  ~Updater();

  int init(int, int, char *, Input*);

  void addRegion(Region r);

  void update();

  void handleInput();

  XImage *getImage();

  Region getUpdateRegion();

  void newRegion();

  private:

  Input *input_;

  static inline Bool anyEventPredicate(Display*, XEvent*, XPointer);

  void handleKeyboardEvent(XEvent &event);

  char *displayName_;

  char *buffer_;

  bool closeDisplay_;

  Display *display_;

  int depth_;

  int width_;
  int height_;

  int bpl_;

  Window window_;
  XImage *image_;

  Pixmap pixmap_;

  Region updateRegion_;

};

Bool Updater::anyEventPredicate(Display*, XEvent*, XPointer)
{
  return true;
}

inline XImage* Updater::getImage()
{
  return image_;
}
inline Region Updater::getUpdateRegion()
{
  return updateRegion_;
}
#endif /* Updater_H */
