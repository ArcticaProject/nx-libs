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

#include "Misc.h"
#include "Agent.h"
#include "Proxy.h"

extern Proxy *proxy;

//
// Set the verbosity level.
//

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

Agent::Agent(int fd[2])
{
  remoteFd_ = fd[0];
  localFd_  = fd[1];

  transport_ = new AgentTransport(localFd_);

  if (transport_ == NULL)
  {
    #ifdef PANIC
    *logofs << "Agent: PANIC! Can't create the memory-to-memory transport "
            << "for FD#" << localFd_ << ".\n" << logofs_flush;
    #endif

    cerr << "Error" << ": Can't create the memory-to-memory transport "
         << "for FD#" << localFd_ << ".\n";

    HandleCleanup();
  }

  FD_ZERO(&saveRead_);
  FD_ZERO(&saveWrite_);

  canRead_ = 0;

  #ifdef DEBUG
  *logofs << "Agent: Created agent object at " << this
          << ".\n" << logofs_flush;
  #endif
}

Agent::~Agent()
{
  delete transport_;

  #ifdef DEBUG
  *logofs << "Agent: Deleted agent object at " << this
          << ".\n" << logofs_flush;
  #endif
}
