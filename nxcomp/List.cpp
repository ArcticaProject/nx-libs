/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2011 NoMachine (http://www.nomachine.com)          */
/* Copyright (c) 2008-2014 Oleksandr Shneyder <o.shneyder@phoca-gmbh.de>  */
/* Copyright (c) 2014-2016 Ulrich Sibiller <uli42@gmx.de>                 */
/* Copyright (c) 2014-2016 Mihai Moldovan <ionic@ionic.de>                */
/* Copyright (c) 2011-2016 Mike Gabriel <mike.gabriel@das-netzwerkteam.de>*/
/* Copyright (c) 2015-2016 Qindel Group (http://www.qindel.com)           */
/*                                                                        */
/* NXCOMP, NX protocol compression and NX extensions to this software     */
/* are copyright of the aforementioned persons and companies.             */
/*                                                                        */
/* Redistribution and use of the present software is allowed according    */
/* to terms specified in the file LICENSE.nxcomp which comes in the       */
/* source distribution.                                                   */
/*                                                                        */
/* All rights reserved.                                                   */
/*                                                                        */
/* NOTE: This software has received contributions from various other      */
/* contributors, only the core maintainers and supporters are listed as   */
/* copyright holders. Please contact us, if you feel you should be listed */
/* as copyright holder, as well.                                          */
/*                                                                        */
/**************************************************************************/

#include "List.h"

//
// Set the verbosity level.
//

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

//
// Define this to know how many instances
// are allocated and deallocated.
//

#undef  REFERENCES

#ifdef REFERENCES

int List::references_ = 0;

#endif

List::List()
{
  #ifdef REFERENCES

  references_++;

  *logofs << "List: Created new List at " 
          << this << " out of " << references_ 
          << " allocated references.\n" << logofs_flush;
  #endif
}

List::~List()
{
  #ifdef REFERENCES

  references_--;

  *logofs << "List: Deleted List at " 
          << this << " out of " << references_ 
          << " allocated references.\n" << logofs_flush;
  #endif
}

void List::remove(int value)
{
  for (T_list::iterator i = list_.begin(); i != list_.end(); i++)
  {
    if (*i == value)
    {
      list_.erase(i);

      return;
    }
  }

  #ifdef PANIC
  *logofs << "List: PANIC! Should not try to remove "
          << "an element not found in the list.\n"
          << logofs_flush;
  #endif

  cerr << "Error" << ": Should not try to remove "
       << "an element not found in the list.\n";

  HandleAbort();
}

void List::rotate()
{
  if (list_.size() > 1)
  {
    int value = *(list_.begin());

    list_.pop_front();

    list_.push_back(value);
  }
}
