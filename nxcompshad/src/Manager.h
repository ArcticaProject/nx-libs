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

#ifndef UpdateManager_H
#define UpdateManager_H

#include <nx-X11/Xlib.h>

#include "Updater.h"
#include "Regions.h"
#include "Input.h"

typedef char* UpdaterHandle;

class UpdateManager
{
  public:

  UpdateManager(int, int, char *, Input *);

  ~UpdateManager();

  void handleInput();

  void addRegion(Region);

  void update();

  UpdaterHandle addUpdater(char *displayName, Display *display);

  int removeUpdater(UpdaterHandle);

  int removeAllUpdaters();

  int numberOfUpdaters();

  int getWidth();

  int getHeight();

  char *getBuffer();

  Region getUpdateManagerRegion();

  void destroyUpdateManagerRegion();

  void newRegion();

  private:

  Updater *createUpdater(char *displayName, Display *display);

  int width_;
  int height_;
  char *frameBuffer_;
  Input *input_;

  int nUpdater;

  Updater **updaterVector;

  Region updateManagerRegion_;

};

inline int UpdateManager::numberOfUpdaters()
{
  return nUpdater;
}

inline int UpdateManager::getWidth()
{
  return width_;
}

inline int UpdateManager::getHeight()
{
  return height_;
}

inline char *UpdateManager::getBuffer()
{
  return frameBuffer_;
}

inline Region UpdateManager::getUpdateManagerRegion()
{
  return updateManagerRegion_;
}

inline void UpdateManager::destroyUpdateManagerRegion()
{
  if (updateManagerRegion_ != NULL)
  {
    XDestroyRegion(updateManagerRegion_);

    updateManagerRegion_ = NULL;
  }
}

#endif /* UpdateManager_H */
