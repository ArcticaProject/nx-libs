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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

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
