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

#include "ServerReadBuffer.h"
#include "ServerChannel.h"

//
// Set the verbosity level.
//

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

unsigned int ServerReadBuffer::suggestedLength(unsigned int pendingLength)
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

int ServerReadBuffer::locateMessage(const unsigned char *start,
                                        const unsigned char *end,
                                            unsigned int &controlLength,
                                                unsigned int &dataLength,
                                                    unsigned int &trailerLength)
{
  unsigned int size = end - start;

  #ifdef TEST
  *logofs << "ServerReadBuffer: Locating message for FD#"
          << transport_ -> fd() << " with " << size
          << " bytes.\n" << logofs_flush;
  #endif

  if (firstMessage_)
  {
    if (size < 8)
    {
      remaining_ = 8 - size;

      #ifdef TEST
      *logofs << "ServerReadBuffer: No message was located "
              << "with remaining " << remaining_ << ".\n"
              << logofs_flush;
      #endif

      return 0;
    }

    dataLength = 8 + (GetUINT(start + 6, bigEndian_) << 2);
  }
  else
  {
    if (size < 32)
    {
      remaining_ = 32 - size;

      #ifdef TEST
      *logofs << "ServerReadBuffer: No message was located "
              << "with remaining " << remaining_ << ".\n"
              << logofs_flush;
      #endif

      return 0;
    }

    if (*start == 1)
    {
      dataLength = 32 + (GetULONG(start + 4, bigEndian_) << 2);
    }
    else
    {
      dataLength = 32;
    }

    if (dataLength < 32)
    {
      #ifdef TEST
      *logofs << "ServerReadBuffer: WARNING! Assuming length 32 "
              << "for suspicious message of length " << dataLength
              << ".\n" << logofs_flush;
      #endif

      dataLength = 32;
    }
  }

  #ifdef TEST
  *logofs << "ServerReadBuffer: Length of the next message is "
          << dataLength << ".\n" << logofs_flush;
  #endif

  if (size < dataLength)
  {
    remaining_ = dataLength - size;

    #ifdef TEST
    *logofs << "ServerReadBuffer: No message was located "
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
  *logofs << "ServerReadBuffer: Located message with "
          << "remaining " << remaining_ << ".\n"
          << logofs_flush;
  #endif

  return 1;
}

//
// Check if the data already read contains a
// message matching the opcode and sequence,
// starting at the given offset.
//

unsigned char *ServerReadBuffer::peekMessage(unsigned int &offset, unsigned char opcode,
                                                 unsigned short sequence)
{
  #ifdef TEST
  *logofs << "ServerReadBuffer: Peeking message "
          << "for FD#" << transport_ -> fd() << " with size "
          << length_ << " offset " << offset << " opcode "
          << (unsigned int) opcode << " and sequence "
          << sequence << ".\n" << logofs_flush;
  #endif

  if (firstMessage_)
  {
    return NULL;
  }

  unsigned char *next = buffer_ + start_ + offset;
  unsigned char *end  = buffer_ + start_ + length_;

  int found = 0;

  while (end - next >= 32)
  {
    #ifdef DEBUG
    *logofs << "ServerReadBuffer: Checking opcode "
            << (unsigned int) *next << " sequence "
            << GetUINT(next + 2, bigEndian_)
            << " at " << next - buffer_ + start_
            << ".\n" << logofs_flush;
    #endif

    if (*next == opcode && GetUINT(next + 2, bigEndian_) == sequence)
    {
      found = 1;

      break;
    }
    else if (*next == 1)
    {
      next += (32 + (GetULONG(next + 4, bigEndian_) << 2));
    }
    else
    {
      next += 32;
    }
  }

  offset = next - buffer_ + start_;

  if (found == 1)
  {
    #ifdef TEST
    *logofs << "ServerReadBuffer: Found message at "
            << "offset " << next - buffer_ + start_
            << ".\n" << logofs_flush;
    #endif

    return next;
  }

  #ifdef TEST
  *logofs << "ServerReadBuffer: Quitting loop at "
          << "offset " << next - buffer_ + start_
          << ".\n" << logofs_flush;
  #endif

  return NULL;
}
