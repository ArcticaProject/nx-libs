/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2010 NoMachine, http://www.nomachine.com/.         */
/*                                                                        */
/* NXCOMP, NX protocol compression and NX extensions to this software     */
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
