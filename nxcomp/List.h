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

#ifndef List_H
#define List_H

#include "Misc.h"
#include "Types.h"

//
// Set the verbosity level.
//

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

//
// Define this to log when lists are
// allocated and deallocated.
//

#undef  REFERENCES

class List
{
  public:

  List();

  ~List();

  int getSize()
  {
    return list_.size();
  }

  T_list &getList()
  {
    return list_;
  }

  T_list copyList()
  {
    return list_;
  }

  void add(int value)
  {
    list_.push_back(value);
  }

  void remove(int value);

  void rotate();

  private:

  //
  // The list container.
  //

  T_list list_;

  #ifdef REFERENCES

  static int references_;

  #endif
};

#endif /* List_H */
