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

#include <string.h>

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

#include "Input.h"
#include "Logger.h"

Input::Input()
{
  logTrace("Input::Input");

  eventsHead_ = NULL;
  eventsTail_ = NULL;
  keymap_ = NULL;
}

Input::~Input()
{
  logTrace("Input::~Input");

  Event *head = eventsHead_;

  while (head)
  {
    Event *next = head -> next;

    delete head -> event;
    delete head;

    head = next;
  }

  if (keymap_ != NULL)
  {
    logDebug("Input::~Input", "Delete keymap_ [%p].", keymap_);

    delete [] keymap_;
  }
}

void Input::pushEvent(Display *display, XEvent *event)
{
  Event *tail = new Event;

  if (tail == NULL)
  {
    logError("Input::pushEvent", ESET(ENOMEM));

    return;
  }

  tail -> next = NULL;
  tail -> display = display;
  tail -> event = event;

  if (eventsHead_ == NULL)
  {
    eventsHead_ = tail;
  }
  else
  {
    eventsTail_ -> next = tail;
  }

  eventsTail_ = tail;
}

XEvent *Input::popEvent()
{
  Event *head = eventsHead_;

  if (head == NULL)
  {
    return 0;
  }

  XEvent *event = head -> event;

  eventsHead_ = head -> next;

  delete head;

  if (eventsHead_ == NULL)
  {
    eventsTail_ = NULL;
  }

  return event;
}

int Input::removeAllEvents(Display *display)
{
  logTrace("Input::removeAllEvents");

  int nRemoved = 0;

  Event *current = eventsHead_;

  while (current)
  {
    if (display == current -> display)
    {
      //
      // Update head of list.
      //

      if (current == eventsHead_)
      {
        eventsHead_ = current -> next;
      }

      //
      // Update tail of list.
      //

      if (current == eventsTail_)
      {
        eventsTail_ = eventsHead_;

        while (eventsTail_ && eventsTail_ -> next)
        {
          eventsTail_ = eventsTail_ -> next;
        }
      }

      //
      // Remove event.
      //

      Event *next = current -> next;

      delete current -> event;
      delete current;

      current = next;

      nRemoved++;
    }
    else
    {
      current = current -> next;
    }
  }

  return nRemoved;
}

