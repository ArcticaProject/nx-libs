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
//
#include <stdio.h>
//
#include "Updater.h"
#include "Logger.h"

Updater::Updater(char *displayName, Display *display)
{
  logTrace("Updater::Updater");

  displayName_ = displayName;
  display_ = display;
  closeDisplay_ = false;
  image_ = NULL;
  updateRegion_ = NULL;
  buffer_ = NULL;
}

Updater::~Updater()
{
  logTrace("Updater::~Updater");

  if (input_)
  {
    int removedEvents = input_ -> removeAllEvents(display_);

    logTest("Updater::~Updater", "Removed events in input queue is [%d].", removedEvents);
  }

  if (display_)
  {
    XDestroyWindow(display_, window_);
    XFreePixmap(display_, pixmap_);

    if (closeDisplay_)
    {
      XCloseDisplay(display_);
    }
  }

  if (image_)
  {
    image_ -> data = NULL;

    XDestroyImage(image_);
  }

  if (updateRegion_)
  {
    XDestroyRegion(updateRegion_);
  }
}

int Updater::init(int width, int height, char *fb, Input *input)
{
  logTrace("Updater::init");

  if (fb == NULL || input == NULL || width <= 0 || height <= 0)
  {
    logError("Updater::init", ESET(EINVAL));

    return -1;
  }

  width_ = width;
  height_ = height;
  buffer_ = fb;
  input_ = input;
/*
  if (display_ == NULL)
  {
    display_ = XOpenDisplay(displayName_);

    closeDisplay_ = true;

    if (display_ == NULL)
    {
      logError("Updater::init", ESET(ENOMSG));

      return -1;
    }
  }
*/
  depth_ = DefaultDepth(display_, DefaultScreen(display_));

  if (depth_ == 8) bpl_ = width_;
  else if (depth_ == 16) bpl_ = width_ * 2;
  else bpl_ = width_ * 4;

  logTest("Updater::init", "Server geometry [%d, %d] depth [%d] bpl [%d].", width_, height_, depth_, bpl_);

/*  int bitmap_pad = 8;

  image_ = XCreateImage(display_, DefaultVisual(display_, DefaultScreen(display_)), depth_, ZPixmap, 0,
                            buffer_, width_, height_, bitmap_pad, 0);

  if (image_ == NULL)
  {
    logError("Updater::init", ESET(ENOMSG));

    logTest("Updater::init", "Failed to create default image.");

    return -1;
  }

  pixmap_ = XCreatePixmap(display_, DefaultRootWindow(display_), width_, height_, depth_);

  unsigned int mask = CWBackPixmap | CWBorderPixel | CWEventMask;

  XSetWindowAttributes attributes;

  attributes.background_pixmap = pixmap_;
  attributes.border_pixel = WhitePixel(display_, DefaultScreen(display_));
  attributes.event_mask = KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask;

  window_ = XCreateWindow(display_, DefaultRootWindow(display_),
                              0, 0, width_, height_, 0, depth_, InputOutput,
                                  DefaultVisual(display_, DefaultScreen(display_)), mask, &attributes);

  if (window_ == None)
  {
    logError("Updater::init", ESET(ENOMSG));

    return -1;
  }

  XSizeHints *size_hints;

  if ((size_hints = XAllocSizeHints()) == NULL)
  {
    logError("Updater::init", ESET(ENOMEM));

    return -1;
  }

  size_hints -> flags = PMinSize | PMaxSize;
  size_hints -> min_width = width_;
  size_hints -> max_width = width_;
  size_hints -> min_height = height_;
  size_hints -> max_height = height_;

  XSetWMNormalHints(display_, window_, size_hints);

  XFree(size_hints);

  Atom deleteWMatom = XInternAtom(display_, "WM_DELETE_WINDOW", 1);

  XSetWMProtocols(display_, window_, &deleteWMatom, 1);

  XMapWindow(display_, window_);*/

  updateRegion_ = XCreateRegion();

  logTest("Updater::init", "updateRegion_[%p]", updateRegion_);
  return 1;
}

void Updater::addRegion(Region region)
{
  //
  // FIXME: Is this too paranoid ?
  //

  if (updateRegion_ == NULL)
  {
    logError("Updater::addRegion", ESET(EINVAL));

    return;
  }

  XUnionRegion(region, updateRegion_, updateRegion_);
}

void Updater::update()
{
  logTrace("Updater::update");

  if (updateRegion_ == NULL)
  {
    logError("Updater::update", ESET(EINVAL));

    return;
  }

  logTest("Updater::update", "Number of rectangles [%ld].", updateRegion_ -> numRects);

/*  for (; updateRegion_ -> numRects > 0; updateRegion_ -> numRects--)
  {
    int n = updateRegion_ -> numRects - 1;

    int x = updateRegion_ -> rects[n].x1;
    int y = updateRegion_ -> rects[n].y1;
    unsigned int width = updateRegion_ -> rects[n].x2 - updateRegion_ -> rects[n].x1;
    unsigned int height = updateRegion_ -> rects[n].y2 - updateRegion_ -> rects[n].y1;

    logDebug("Updater::update", "Sending rectangle: [%d, %d, %d, %d].", x, y, width, height);

    //
    // We need to update the extents.
    //

    int bitmap_pad;

    if (depth_ == 32 || depth_ == 24)
    {
      bitmap_pad = 32;
    }
    else if (depth_ == 16)
    {
      if ((width & 1) == 0)
      {
        bitmap_pad = 32;
      }
      else
      {
        bitmap_pad = 16;
      }
    }
    else if ((width & 3) == 0)
    {
      bitmap_pad = 32;
    }
    else if ((width & 1) == 0)
    {
      bitmap_pad = 16;
    }
    else
    {
      bitmap_pad = 8;
    }*/

/*    image_ -> bitmap_pad = bitmap_pad;*/

   /* NXShadowCorrectColor(x, y, width, height);*/

/*    XPutImage(display_, pixmap_, DefaultGC(display_, DefaultScreen(display_)),
                  image_, x, y, x, y, width, height);

    XClearArea(display_, window_, x, y, width, height, 0);
  }*/

  //
  // Should we reduces the box vector ?
  //
  // BOX *box = Xrealloc(updateRegion_ -> rects,
  //                         updateRegion_ -> numRects == 0 ? sizeof(BOX) :
  //                             updateRegion_ -> numRects * sizeof(BOX));
  //
  // if (box)
  // {
  //   updateRegion_ -> rects = box;
  //   updateRegion_ -> size = 1;
  // }
  //

  if (updateRegion_ -> numRects == 0)
  {
    updateRegion_ -> extents.x1 = 0;
    updateRegion_ -> extents.y1 = 0;
    updateRegion_ -> extents.x2 = 0;
    updateRegion_ -> extents.y2 = 0;
  }
  else
  {
    //
    // FIXME: We have to update the region extents.
    //

    logTest("Updater::update", "Region extents has not been updated.");
  }
}

void Updater::handleInput()
{
  logTrace("Updater::handleInput");

  XEvent *event = new XEvent;

  if (event == NULL)
  {
    logError("Updater::handleInput", ESET(ENOMEM));

    return;
  }

  while (XCheckIfEvent(display_, event, anyEventPredicate, NULL))
  {
    switch (event -> type)
    {
     /* case ClientMessage:
      {
        Atom wmProtocols = XInternAtom(display_, "WM_PROTOCOLS", 0);
        Atom wmDeleteWindow = XInternAtom(display_, "WM_DELETE_WINDOW", 0);

        if (event -> xclient.message_type == wmProtocols &&
                (Atom)event -> xclient.data.l[0] == wmDeleteWindow)
        {
          logTest("Updater::handleInput", "Got client message of type WM_PROTOCOLS and value WM_DELETE_WINDOW,"
                      " throwing exception UpdaterClosing.");

          delete event;

          throw UpdaterClosing();
        }
        else
        {
          logTest("Updater::handleInput", "Unexpected client message type [%ld] format [%d] first value [%ld]",
                      event -> xclient.message_type, event -> xclient.format, event -> xclient.data.l[0]);
        }

        break;
      }*/
      case KeyPress:
      case KeyRelease:
      case ButtonPress:
      case ButtonRelease:
      case MotionNotify:
      {
        input_ -> pushEvent(display_, event);

        event = new XEvent;

        if (event == NULL)
        {
          logError("Updater::handleInput", ESET(ENOMEM));

          return;
        }

        break;
      }
      default:
      {
        logTest("Updater::handleInput", "Handling unexpected event [%d].", event -> type);

        break;
      }
    }
  }

  delete event;
}

void Updater::newRegion()
{
  if (updateRegion_ != NULL)
  {
    XDestroyRegion(updateRegion_);
  }

  updateRegion_ = XCreateRegion();

  logTest("Updater::newRegion", "updateRegion_ [%p].", updateRegion_);
}
//
// Private functions.
//
