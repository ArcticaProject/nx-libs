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
