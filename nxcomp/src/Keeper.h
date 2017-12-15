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

#ifndef Keeper_H
#define Keeper_H

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
// Define this to check how many file
// nodes are allocated and deallocated.
//

#undef  REFERENCES

class Keeper;

class File
{
  friend class Keeper;

  public:

  File();

  ~File();

  //
  // Allow sort by time and size. If time
  // is the same, keep the bigger element.
  //

  bool compare(File *b) const;

  private:

  char *name_;

  int    size_;
  time_t time_;

  #ifdef REFERENCES

  static int references_;

  #endif
};

class Keeper
{
  public:

  Keeper(int caches, int images, const char *root,
             int sleep, int parent);

  ~Keeper();

  //
  // Call this just once.
  //

  int cleanupCaches();

  //
  // Call this at any given interval.
  //

  int cleanupImages();

  //
  // Call this if it's time to exit.
  //

  void setSignal(int signal)
  {
    signal_ = signal;
  }

  int getSignal()
  {
    return signal_;
  }

  int getParent()
  {
    return parent_;
  }

  private:

  //
  // Get a list of files in directory.
  //
  
  int collect(const char *path);

  //
  // Sort the collected files according to
  // last modification time and delete the
  // older ones until disk size is below
  // the threshold.
  //

  int cleanup(int threshold);

  //
  // Empty the files repository.
  //

  void empty();

  //
  // Size in bytes of total allowed
  // storage for persistent caches.
  //

  int caches_;

  //
  // Size in bytes of total allowed
  // storage for images cache.
  //

  int images_;

  //
  // Path of the NX root directory.
  //

  char *root_;

  //
  // The little delay to be introduced
  // before reading a new entry.
  //

  int sleep_;

  //
  // Total size of files in repository.
  //

  int total_;

  //
  // The parent process, so we can exit
  // if it is gone.
  //

  int parent_;

  //
  // Set if we need to give up because
  // of a signal.
  //

  int signal_;

  //
  // Repository where to collect files.
  //

  T_files *files_;
};

#endif /* Keeper_H */

