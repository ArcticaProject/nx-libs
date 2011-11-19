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

#include "ClientReadBuffer.h"

#include "ClientChannel.h"

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

unsigned int ClientReadBuffer::suggestedLength(unsigned int pendingLength)
{
  //
  // Even if the pending data is not
  // enough to make a complete message,
  // resize the buffer to accomodate
  // it all.
  //

  unsigned int readLength = pendingLength;

  if (pendingLength < remaining_)
  {
    readLength = remaining_;
  }

  return readLength;
}  

int ClientReadBuffer::locateMessage(const unsigned char *start,
                                        const unsigned char *end,
                                            unsigned int &controlLength,
                                                unsigned int &dataLength,
                                                    unsigned int &trailerLength)
{
  unsigned int size = end - start;

  #ifdef TEST
  *logofs << "ClientReadBuffer: Locating message for FD#"
          << transport_ -> fd() << " with " << size
          << " bytes.\n" << logofs_flush;
  #endif

  if (firstMessage_)
  {
    if (size < 12)
    {
      remaining_ = 12 - size;

      #ifdef TEST
      *logofs << "ClientReadBuffer: No message was located "
              << "with remaining " << remaining_ << ".\n"
              << logofs_flush;
      #endif

      return 0;
    }

    if (*start == 0x42)
    {
      bigEndian_ = 1;
    }
    else
    {
      bigEndian_ = 0;
    }

    channel_ -> setBigEndian(bigEndian_);

    dataLength = 12 + RoundUp4(GetUINT(start + 6, bigEndian_)) +
                          RoundUp4(GetUINT(start + 8, bigEndian_));

    //
    // Send the data immediately if this is unlikely
    // to be a X connection attempt.
    //

    if (dataLength > 4096)
    {
      #ifdef WARNING
      *logofs << "ClientReadBuffer: WARNING! Flushing suspicious X "
              << "connection with first request of " << dataLength
              << " bytes.\n" << logofs_flush;
      #endif

      dataLength = size;
    }
  }
  else
  {
    if (size < 4)
    {
      remaining_ = 4 - size;

      #ifdef TEST
      *logofs << "ClientReadBuffer: No message was located "
              << "with remaining " << remaining_ << ".\n"
              << logofs_flush;
      #endif

      return 0;
    }

    dataLength = (GetUINT(start + 2, bigEndian_) << 2);

    if (dataLength < 4)
    {
      #ifdef TEST
      *logofs << "ClientReadBuffer: WARNING! Assuming length 4 "
              << "for suspicious message of length " << dataLength
              << ".\n" << logofs_flush;
      #endif

      dataLength = 4;
    }
  }

  #ifdef TEST
  *logofs << "ClientReadBuffer: Length of the next message is "
          << dataLength << ".\n" << logofs_flush;
  #endif

  if (size < dataLength)
  {
    remaining_ = dataLength - size;

    #ifdef TEST
    *logofs << "ClientReadBuffer: No message was located "
            << "with remaining " << remaining_ << ".\n"
            << logofs_flush;
    #endif

    return 0;
  }

  firstMessage_ = 0;

  controlLength = 0;
  trailerLength = 0;

  remaining_ = 0;

  #ifdef TEST
  *logofs << "ClientReadBuffer: Located message with "
          << "remaining " << remaining_ << ".\n"
          << logofs_flush;
  #endif

  return 1;
}
