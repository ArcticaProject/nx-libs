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

#include "GenericReadBuffer.h"

#include "GenericChannel.h"

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

unsigned int GenericReadBuffer::suggestedLength(unsigned int pendingLength)
{
  //
  // Always read the initial read size.
  //

  return 0;
}  

int GenericReadBuffer::locateMessage(const unsigned char *start,
                                         const unsigned char *end,
                                             unsigned int &controlLength,
                                                 unsigned int &dataLength,
                                                     unsigned int &trailerLength)
{
  //
  // We don't care about the endianess
  // in generic channels.
  //

  unsigned int size = end - start;

  #ifdef TEST
  *logofs << "GenericReadBuffer: Locating message for FD#"
          << transport_ -> fd() << " with " << size
          << " bytes.\n" << logofs_flush;
  #endif

  if (size == 0)
  {
    remaining_ = 1;

    return 0;
  }

  dataLength = size;

  controlLength = 0;
  trailerLength = 0;

  remaining_ = 0;

  return 1;
}
