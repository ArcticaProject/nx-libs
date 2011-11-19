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

#include "ProxyReadBuffer.h"

#include "Transport.h"

//
// Set the verbosity level. You also
// need to define DUMP in Misc.cpp
// if DUMP is defined here.
//

#define WARNING
#define PANIC
#undef  TEST
#undef  DEBUG
#undef  DUMP

unsigned int ProxyReadBuffer::suggestedLength(unsigned int pendingLength)
{
  //
  // Always read all the data that
  // is available.
  //

  int readable = transport_ -> readable();

  unsigned int readLength = (readable == -1 ? 0 : (unsigned int) readable);

  if (readLength < pendingLength)
  {
    readLength = pendingLength;
  }

  //
  // Even if the readable data is not
  // enough to make a complete message,
  // resize the buffer to accomodate
  // it all.
  //

  if (pendingLength < remaining_)
  {
    readLength = remaining_;
  }

  return readLength;
}  

int ProxyReadBuffer::locateMessage(const unsigned char *start,
                                       const unsigned char *end,
                                           unsigned int &controlLength,
                                               unsigned int &dataLength,
                                                   unsigned int &trailerLength)
{
  unsigned int lengthLength = 0;
  const unsigned char *nextSrc = start;
  unsigned char next;

  dataLength = 0;

  #ifdef TEST
  *logofs << "ProxyReadBuffer: Locating message for FD#"
          << transport_ -> fd() << " with " << end - start
          << " bytes.\n" << logofs_flush;
  #endif

  //
  // Use something like the following if
  // you are looking for errors.
  //

  #ifdef DUMP
  if (control -> ProxyMode == proxy_server && start < end &&
          transport_ -> fd() == 6 || transport_ -> fd() == 11)
  {
    *logofs << "ProxyReadBuffer: Partial checksums are:\n";

    DumpBlockChecksums(start, end - start, 256);

    *logofs << logofs_flush;
  }
  #endif

  do
  {
    if (nextSrc >= end)
    {
      remaining_ = 1;

      #ifdef TEST
      *logofs << "ProxyReadBuffer: No message was located "
              << "with remaining " << remaining_ << ".\n"
              << logofs_flush;
      #endif

      return 0;
    }

    next = *nextSrc++;

    dataLength <<= 7;
    dataLength |= (unsigned int) (next & 0x7f);

    lengthLength++;
  }
  while (next & 0x80);

  unsigned int totalLength;

  if (dataLength == 0)
  {
    trailerLength = 0;
    controlLength = 3;
    totalLength   = controlLength;
  }
  else
  {
    trailerLength = lengthLength;
    controlLength = 0;
    totalLength   = dataLength + trailerLength;
  }

  if (start + totalLength > end)
  {
    //
    // When having to decompress a ZLIB stream,
    // a single byte can be enough to complete
    // the frame.
    //

    if (control -> RemoteStreamCompression == 0)
    {
      remaining_ = totalLength - (end - start);
    }
    else
    {
      remaining_ = 1;
    }

    #ifdef TEST
    *logofs << "ProxyReadBuffer: No message was located "
            << "with remaining " << remaining_ << ".\n"
            << logofs_flush;
    #endif

    return 0;
  }
  else
  {
    #ifdef DUMP
    *logofs << "ProxyReadBuffer: Received " << totalLength << " bytes of data "
            << "with checksum ";

    DumpChecksum(start, totalLength);

    *logofs << " on proxy FD#" << transport_ -> fd() << ".\n" << logofs_flush;
    #endif

    #if defined(TEST) || defined(INFO)
    *logofs << "ProxyReadBuffer: Produced plain input for " << dataLength 
            << "+" << trailerLength << "+" << controlLength << " bytes out of " 
            << totalLength << " bytes.\n" << logofs_flush;
    #endif

    #ifdef DUMP
    *logofs << "ProxyReadBuffer: Partial checksums are:\n";

    DumpBlockChecksums(start, totalLength, 256);

    *logofs << logofs_flush;
    #endif

    remaining_ = 0;

    #ifdef TEST
    *logofs << "ProxyReadBuffer: Located message with "
            << "remaining " << remaining_ << ".\n"
            << logofs_flush;
    #endif

    return 1;
  }
}
