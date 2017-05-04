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

#ifndef Updater_H
#define Updater_H

#include <nx-X11/Xlib.h>

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
