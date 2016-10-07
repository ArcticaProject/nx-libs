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

#ifndef CorePoller_H
#define CorePoller_H

#include <stdio.h>

#include "Logger.h"
#include "Regions.h"
#include "Input.h"

typedef enum{
  LINE_HAS_CHANGED,
  LINE_NOT_CHECKED,
  LINE_NOT_CHANGED
} LineStatus;

typedef enum{
  HIGHEST_PRIORITY = 0,
  PRIORITY = 30,
  NOT_PRIORITY = 90
} LinePriority;

class CorePoller
{
  public:

  CorePoller(Input*, Display*);

  virtual ~CorePoller();

  virtual int init();

  unsigned int width() const;

  unsigned int height() const;

  unsigned char depth() const;

  int isChanged(int (*)(void*), void *, int *);

  char *getFrameBuffer() const;

  void destroyFrameBuffer();

  void createFrameBuffer();

  Region lastUpdatedRegion();

  Region getLastUpdatedRegion();

  void handleInput();

  void handleEvent(Display *, XEvent *);

  void handleWebKeyEvent(KeySym keysym, Bool isKeyPress);

  Display *getShadowDisplay();

  void setShadowDisplay(Display *shadowDisplay);

  protected:

  unsigned int bpp_;

  unsigned int bpl_;

  unsigned int width_;

  unsigned int height_;

  int depth_;

  char *buffer_;

  unsigned long redMask_;
  unsigned long greenMask_;
  unsigned long blueMask_;
  unsigned long colorMask_[3];

  char mirror_;

  char mirrorChanges_;

  virtual int updateShadowFrameBuffer(void) = 0;

  virtual char *getRect(XRectangle r) = 0;

  int imageByteOrder_;

  #ifdef __CYGWIN32__
  virtual char checkDesktop(void) = 0;
  #endif

  Display *shadowDisplay_;

  void update(char *src, XRectangle r);

  Region lastUpdatedRegion_;

  private:

  virtual void handleKeyboardEvent(Display *, XEvent *) = 0;

  virtual void handleWebKeyboardEvent(KeySym keysym, Bool isKeyPress) = 0;

  virtual void handleMouseEvent(Display *, XEvent *) = 0;

  Input *input_;

  static const int maxSliceHeight_;
  static const int minSliceHeight_;

  LineStatus *lineStatus_;
  int *linePriority_;

  static const char interlace_[];

  int *lefts_;
  int *rights_;

  // FIXME: Make them friend.

  int differ(char *src, XRectangle r);
};

inline unsigned int CorePoller::width() const
{
  return width_;
}

inline unsigned int CorePoller::height() const
{
  return height_;
}

inline unsigned char CorePoller::depth() const
{
  return depth_;
}

inline char *CorePoller::getFrameBuffer() const
{
  return buffer_;
}

inline void CorePoller::destroyFrameBuffer()
{
  if (buffer_ != NULL)
  {
    delete[] buffer_;
    buffer_ = NULL;
  }
}

inline Region CorePoller::lastUpdatedRegion()
{
  Region region = lastUpdatedRegion_;

  lastUpdatedRegion_ = XCreateRegion();

  if (lastUpdatedRegion_ == NULL)
  {
    logError("CorePoller::lastUpdatedRegion", ESET(ENOMEM));

    lastUpdatedRegion_ = region;

    return NULL;
  }

  return region;
}

inline Region CorePoller::getLastUpdatedRegion()
{
  return lastUpdatedRegion_;
}

inline Display *CorePoller::getShadowDisplay()
{
  return shadowDisplay_ ;
}

inline void CorePoller::setShadowDisplay(Display *shadowDisplay)
{
  shadowDisplay_ = shadowDisplay;
}

#endif /* CorePoller_H */
