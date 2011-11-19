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

#include <string.h>
#include <sys/time.h>

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

#include "Core.h"
#include "Logger.h"

const int CorePoller::maxSliceHeight_ = 20;
const int CorePoller::minSliceHeight_ = 3;

const char CorePoller::interlace_[] =
{
  0, 16,
  8, 24,
  4, 20, 12, 28,
  2, 18, 10, 26, 6, 22, 14, 30,
  1, 17,
  9, 25,
  5, 21, 13, 29,
  3, 19, 11, 27, 7, 23, 15, 31
};

CorePoller::CorePoller(Input *input, Display *display) : input_(input)
{
  logTrace("CorePoller::CorePoller");

  buffer_ = NULL;
  lastUpdatedRegion_ = NULL;
  lineStatus_ = NULL;
  linePriority_ = NULL;
  lefts_ = NULL;
  rights_ = NULL;
}

CorePoller::~CorePoller()
{
  logTrace("CorePoller::~CorePoller");

  if (buffer_ != NULL)
  {
    delete [] buffer_;

    buffer_ = NULL;
  }

  if (lastUpdatedRegion_ != NULL)
  {
    XDestroyRegion(lastUpdatedRegion_);

    lastUpdatedRegion_ = NULL;
  }

  if (lineStatus_ != NULL)
  {
    delete [] lineStatus_;

    lineStatus_ = NULL;
  }

  if (linePriority_ != NULL)
  {
    delete [] linePriority_;

    linePriority_ = NULL;
  }

  if (lefts_ != NULL)
  {
    delete [] lefts_;

    lefts_ = NULL;
  }

  if (rights_ != NULL)
  {
    delete [] rights_;

    rights_ = NULL;
  }
}

int CorePoller::init()
{
  logTrace("CorePoller::init");

  createFrameBuffer();

  if (buffer_ == NULL)
  {
    logError("CorePoller::init", ESET(ENOMEM));

    return -1;
  }

  logTest("CorePoller::init", "Allocated frame buffer at [%p] for [%d] bytes.",
            buffer_, bpl_ * height_);

  if (lastUpdatedRegion_ != NULL)
  {
    XDestroyRegion(lastUpdatedRegion_);

    lastUpdatedRegion_ = NULL;
  }

  lastUpdatedRegion_ = XCreateRegion();

  if (lineStatus_ != NULL)
  {
    delete[] lineStatus_;
  }

  lineStatus_ = new LineStatus[height_ + 1];

  if (lineStatus_ == NULL)
  {
    logError("CorePoller::init", ESET(ENOMEM));

    return -1;
  }

  //
  // We need this boundary element to
  // speed up the algo.
  //

  if (linePriority_ != NULL)
  {
    delete[] linePriority_;
  }

  linePriority_ = new int [height_ + 1];

  if (linePriority_ == NULL)
  {
    logError("CorePoller::init", ESET(ENOMEM));

    return -1;
  }

  for (unsigned int i = 0; i < height_; i++)
  {
    linePriority_[i] = HIGHEST_PRIORITY;
  }

  if (lefts_ != NULL)
  {
    delete[] lefts_;
  }

  lefts_ = new int [height_];

  if (rights_ != NULL)
  {
    delete[] rights_;
  }

  rights_ = new int [height_];

  for (unsigned int i = 0; i < height_; i++)
  {
    rights_[i] = lefts_[i] = 0;
  }

  return 1;
}

int CorePoller::isChanged(int (*checkIfInputCallback)(void *), void *arg, int *suspended)
{
  logTrace("CorePoller::isChanged");

#if defined(__CYGWIN32__) || defined(WIN32)

  checkDesktop();

#endif

#if !defined(__CYGWIN32__) && !defined(WIN32)

  if (mirror_ == 1)
  {
    int result = mirrorChanges_;

    mirrorChanges_ = 0;

    return result;
  }

#endif

  logDebug("CorePoller:isChanged", "Going to use default polling algorithm.\n");

  //
  // In order to allow this function to
  // be suspended and resumed later, we
  // need to save these two status vars.
  // 

  static int idxIlace = 0;
  static int curLine = 0;


  const long timeout = 50;
  long oldTime;
  long newTime;
  struct timeval ts;

  gettimeofday(&ts, NULL);

  oldTime = ts.tv_sec * 1000 + ts.tv_usec / 1000;

  if (curLine == 0) // && idxIlace == 0 ?
  {
    for (unsigned int i = 0; i < height_; i++)
    {
      lineStatus_[i] = LINE_NOT_CHECKED;
    }
  }

  int foundChanges = 0;

  foundChanges = 0;

  int curIlace = interlace_[idxIlace];

  bool moveBackward = false;

  logDebug("CorePoller::isChanged", "Interlace index [%d] interlace [%d].", idxIlace, curIlace);

  for (; curLine < (int) height_; curLine++)
  {
    logDebug("CorePoller::isChanged", "Analizing line [%d] move backward [%d] status [%d] priority [%d].",
                curLine, moveBackward, lineStatus_[curIlace], linePriority_[curLine]);

    //
    // Ask the caller if the polling have to be suspended.
    //

    if ((*checkIfInputCallback)(arg) == 1)
    {
      *suspended = 1;

      break;
    }

    //
    // Suspend if too much time is elapsed.
    //

    gettimeofday(&ts, NULL);

    newTime = ts.tv_sec * 1000 + ts.tv_usec / 1000;

    if (newTime - oldTime >= timeout)
    {
      *suspended = 1;

      break;
    }

    oldTime = newTime;

    if (lineStatus_[curLine] != LINE_NOT_CHECKED)
    {
      continue;
    }

    if (moveBackward)
    {
      moveBackward = false;
    }
    else
    {
      switch (linePriority_[curLine])
      {
        case 1:
        case 29:
        {
          //
          // It was a priority,
          // but now it may not be.
          //
        }
        case 31:
        {
          //
          // Not a priority, still isn't.
          //

          linePriority_[curLine] = NOT_PRIORITY;

          break;
        }
        case 0:
        {
          //
          // Make it a priority.
          //

          linePriority_[curLine] = PRIORITY;

          break;
        }
        default:
        {
          linePriority_[curLine]--;

          break;
        }
      }

      if ((linePriority_[curLine] > PRIORITY) && ((curLine & 31) != curIlace))
      {
        continue;
      }
    }

    XRectangle rect = {0, curLine, width_, 1};

    char *buffer;

    logDebug("CorePoller::isChanged", "Checking line [%d].", curLine);

    if ((buffer = getRect(rect)) == NULL)
    {
      logDebug("CorePoller::isChanged", "Failed to retrieve line [%d].", curLine);

      return -1;
    }

    if (memcmp(buffer, buffer_ + curLine * bpl_, bpl_) == 0 || differ(buffer, rect) == 0)
    {
      logDebug("CorePoller::isChanged", "Data buffer didn't change.");

      lineStatus_[curLine] = LINE_NOT_CHANGED;

      continue;
    }

    rect.x = lefts_[rect.y];
    rect.width = rights_[rect.y] - lefts_[rect.y] + 1;

    update(buffer + rect.x * bpp_, rect);

    foundChanges = 1;

    lineStatus_[curLine] = LINE_HAS_CHANGED;

    //
    // Wake up the next line.
    //

    if (linePriority_[curLine + 1] > PRIORITY)
    {
      linePriority_[curLine + 1] = HIGHEST_PRIORITY;
    }

    //
    // Give this line priority.
    //

    linePriority_[curLine] = HIGHEST_PRIORITY;

    //
    // Wake up previous line.
    //

    if (curLine > 0 && lineStatus_[curLine - 1] == LINE_NOT_CHECKED)
    {
      moveBackward = true;
      curLine -= 2;
    }
  }

  //
  // Execution reached the end of loop.
  //

  if (curLine == (int) height_)
  {
    idxIlace = (idxIlace + 1) % 32;

    curLine = 0;
  }

  //
  // Create the region of changed pixels.
  //

  if (foundChanges)
  {
    int start, last, curLine, left, right;

    for (curLine = 0; curLine < (int) height_; curLine++)
    {
      if (lineStatus_[curLine] == LINE_HAS_CHANGED)
      {
        break;
      }
    }

    start = curLine;
    last = curLine;

    left = lefts_[curLine];
    right = rights_[curLine];
    curLine++;

    while (1)
    {
      for (; curLine < (int) height_; curLine++)
      {
        if (lineStatus_[curLine] == LINE_HAS_CHANGED)
        {
          break;
        }
      }

      if (curLine == (int) height_)
      {
        break;
      }

      if ((curLine - last > minSliceHeight_) || (last - start > maxSliceHeight_))
      {
        XRectangle rect = {left, start, right - left + 1, last - start + 1};

        XUnionRectWithRegion(&rect, lastUpdatedRegion_, lastUpdatedRegion_);

        start = curLine;
        left = lefts_[curLine];
        right = rights_[curLine];
      }
      else
      {
        if (lefts_[curLine] < left)
        {
          left = lefts_[curLine];
        }

        if (rights_[curLine] > right)
        {
          right = rights_[curLine];
        }
      }

      last = curLine;

      curLine++;
    }

    //
    // Send last block.
    //

    if (last >= start)
    {
      XRectangle rect = {left, start, right - left + 1, last - start + 1};

      XUnionRectWithRegion(&rect, lastUpdatedRegion_, lastUpdatedRegion_);
    }
  }

  return foundChanges;
}

int CorePoller::differ(char *buffer, XRectangle r)
{
  logTrace("CorePoller::differ");

  int bpl = bpp_ * r.width;
  int i;
  char *pBuf;
  char *pFb;

  pBuf = (buffer);
  pFb = (buffer_ + r.x + r.y * bpl_);

  for (i = 0; i < bpl; i++)
  {
    if (*pFb++ != *pBuf++)
    {
      lefts_[r.y] = i / bpp_;

      break;
    }
  }

  if (i == bpl)
  {
    return 0;
  }

  pBuf = (buffer) + bpl - 1;
  pFb = (buffer_ + r.x + r.y * bpl_) + bpl - 1;

  int j = i - 1;

  for (i = bpl - 1; i > j; i--)
  {
    if (*pFb-- != *pBuf--)
    {
      rights_[r.y] = i / bpp_;

      break;
    }
  }

  return 1;
}

void CorePoller::update(char *src, XRectangle r)
{
  logTrace("CorePoller::update");

  char *dst = buffer_ + r.x * bpp_ + r.y * bpl_;
  int bpl = bpp_ * r.width;

  for (unsigned int i = 0; i < r.height; i++)
  {
    if(((r.x * bpp_ + r.y * bpl_) + bpl) > (bpl_ * height_))
    {
      //
      // Out of bounds. Maybe a resize is going on.
      //

      continue;
    }

    memcpy(dst, src, bpl);

    src += bpl;

    dst += bpl_;
  }
}

void CorePoller::handleEvent(Display *display, XEvent *event)
{
  logTrace("CorePoller::handleEvent");

  switch (event -> type)
  {
    case KeyPress:
    case KeyRelease:
    {
      handleKeyboardEvent(display, event);
      break;
    }
    case ButtonPress:
    case ButtonRelease:
    case MotionNotify:
    {
      handleMouseEvent(display, event);
      break;
    }
    default:
    {
      logTest("CorePoller::handleEvent", "Handling unexpected event [%d] from display [%p].",
                  event -> type, display);
      break;
    }
  }
}

void CorePoller::handleWebKeyEvent(KeySym keysym, Bool isKeyPress)
{
  logTrace("CorePoller::handleWebKeyEvent");

  handleWebKeyboardEvent(keysym, isKeyPress);
}

void CorePoller::handleInput()
{
  while (input_ -> checkIfEvent())
  {
    Display *display = input_ -> currentDisplay();
    XEvent *event = input_ -> popEvent();

    handleEvent(display, event);

    delete event;
  }
}

void CorePoller::createFrameBuffer()
{
  logTrace("CorePoller::createFrameBuffer");

  if (buffer_ == NULL)
  {
    buffer_ = new char[bpl_ * height_];
  }
}
