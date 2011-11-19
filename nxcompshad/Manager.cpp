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

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <string.h>

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

#include "Manager.h"
#include "Logger.h"

UpdateManager::UpdateManager(int w, int h, char *f, Input *i)
  : width_(w), height_(h), frameBuffer_(f), input_(i)
{
  logTrace("UpdateManager::UpdateManager");

  nUpdater = 0;
  updaterVector = NULL;
  updateManagerRegion_ = NULL;
}

UpdateManager::~UpdateManager()
{
  logTrace("UpdateManager::~UpdateManager");

  for (int i = 0; i < nUpdater; i++)
  {
    delete updaterVector[i];
  }

  delete [] updaterVector;
}

Updater *UpdateManager::createUpdater(char *displayName, Display *display)
{
  Updater *updater = new Updater(displayName, display);

  if (updater == NULL)
  {
    logError("UpdateManager::createUpdater", ESET(ENOMEM));

    return NULL;
  }

  if (updater -> init(width_, height_, frameBuffer_, input_) == -1)
  {
    logError("UpdateManager::createUpdater", EGET());

    delete updater;

    return NULL;
  }

  return updater;
}

UpdaterHandle UpdateManager::addUpdater(char *displayName, Display *display)
{
  Updater *newUpdater = createUpdater(displayName, display);

  if (newUpdater == NULL)
  {
    logError("UpdateManager::addUpdater", EGET());

    return NULL;
  }

  Updater **newUpdaterVector = new Updater*[nUpdater + 1];

  if (newUpdaterVector == NULL)
  {
    logError("UpdateManager::addUpdater", ESET(ENOMEM));

    delete newUpdater;

    return NULL;
  }

  for (int i = 0; i < nUpdater; i++)
  {
    newUpdaterVector[i] = updaterVector[i];
  }

  newUpdaterVector[nUpdater] = newUpdater;

  delete [] updaterVector;

  updaterVector = newUpdaterVector;

  nUpdater++;

  logTest("UpdateManager::AddUpdater", "Number of updaters [%d].", nUpdater);

  return reinterpret_cast<UpdaterHandle>(newUpdater);
}

int UpdateManager::removeAllUpdaters()
{
  logTest("UpdateManager::removeAllUpdaters", "Number of updaters [%d].", nUpdater);

  int nullUpdaters = 0;

  for (int i = nUpdater; i > 0; i--)
  {
    if (removeUpdater(reinterpret_cast<UpdaterHandle>(updaterVector[i - 1])) == 0)
    {
      nullUpdaters++;
    }
  }

  if (nUpdater == 0)
  {
    return 1;
  }

  if (nUpdater == nullUpdaters)
  {
    logTest("UpdateManager::removeAllUpdaters", "Ignored null records in Updater vector.");

    return 0;
  }

  logTest("UpdateManager::removeAllUpdaters", "Failed to remove some updaters.");

  return -1;
}

int UpdateManager::removeUpdater(UpdaterHandle handle)
{
  Updater * const updater = (Updater*) handle;

  logTest("UpdateManager::removeUpdater", "Removing Updater [%p].", updater);

  if (updater == NULL)
  {
    return 0;
  }

  for (int i = 0; i < nUpdater; i++)
  {
    if (updater == updaterVector[i])
    {
      updaterVector[i] = updaterVector[nUpdater - 1];

      nUpdater--;

      delete updater;

      return 1;
    }
  }

  logTest("UpdateManager::removeUpdater", "Couldn't find Updater [%p].", updater);

  return -1;
}

void UpdateManager::addRegion(Region region)
{
  logTrace("UpdateManager::addRegion");

  for (int i = 0; i < nUpdater; i++)
  {
    updaterVector[i] -> addRegion(region);
  }

  XDestroyRegion(region);
}

void UpdateManager::update()
{
  logTrace("UpdateManager::update");

  for (int i = 0; i < nUpdater; i++)
  {
    /*updaterVector[i] -> update();*/
    if (updaterVector[i] -> getUpdateRegion())
    {
      logDebug("UpdateManager::update", "pRegion [%p] rect[%ld].",
                   updaterVector[i] -> getUpdateRegion(), (updaterVector[i] -> getUpdateRegion()) -> numRects);

      updateManagerRegion_ = updaterVector[i] -> getUpdateRegion();
      //
      // FIXME: Remove me.
      //
      for (int j = 0; j < updateManagerRegion_ -> numRects; j++)
      {
        int x = updateManagerRegion_ -> rects[j].x1;
        int y = updateManagerRegion_ -> rects[j].y1;
        unsigned int width = updateManagerRegion_ -> rects[j].x2 - updateManagerRegion_ -> rects[j].x1;
        unsigned int height = updateManagerRegion_ -> rects[j].y2 - updateManagerRegion_ -> rects[j].y1;
        logDebug("UpdateManager::update", "x[%d]y[%d]width[%u]height[%u], updateManagerRegion_[%p]",
                  x, y, width, height, updateManagerRegion_);
      }
    }
  }
}

void UpdateManager::handleInput()
{
  logTrace("UpdateManager::handleInput");

  for (int i = 0; i < nUpdater; i++)
  {
    try
    {
      updaterVector[i] -> handleInput();
    }
    catch (UpdaterClosing u)
    {
      logTest("UpdateManager::handleInput", "Catched exception UpdaterClosing().");

      removeUpdater((UpdaterHandle)updaterVector[i]);

      //
      // Now the i-element of the updaterVector
      // is changed. We don't want to skip it.
      //

      i--;
    }
  }
}

void UpdateManager::newRegion()
{
  logTrace("UpdateManager::newRegion");

  for (int i = 0; i < nUpdater; i++)
  {
    updaterVector[i] -> newRegion();
  }
}
