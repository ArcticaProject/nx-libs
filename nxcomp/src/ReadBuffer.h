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

#ifndef ReadBuffer_H
#define ReadBuffer_H

#include "Misc.h"
#include "Timestamp.h"
#include "Transport.h"

#define READ_BUFFER_DEFAULT_SIZE  8192

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

class ReadBuffer
{
  public:

  ReadBuffer(Transport *transport);

  virtual ~ReadBuffer();

  int readMessage();

  void readMessage(const unsigned char *message, unsigned int length);

  const unsigned char *getMessage(unsigned int &dataLength)
  {
    unsigned int controlLength;

    return getMessage(controlLength, dataLength);
  }

  const unsigned char *getMessage(unsigned int &controlLength, unsigned int &dataLength);

  unsigned int getLength() const
  {
    return length_;
  }

  unsigned int getRemaining() const
  {
    return remaining_;
  }

  int setSize(int initialReadSize, int initialbufferSize);

  void fullReset();

  //
  // Check if there is a complete
  // message in the buffer.
  //

  int checkMessage()
  {
    if (length_ == 0)
    {
      return 0;
    }
    else
    {
      unsigned int controlLength;
      unsigned int dataLength;
      unsigned int trailerLength;

      return (locateMessage(buffer_ + start_, buffer_ + start_ + length_,
                  controlLength, dataLength, trailerLength));
    }
  }

  protected:

  virtual unsigned int suggestedLength(unsigned int pendingLength) = 0;

  virtual int locateMessage(const unsigned char *start,
                                const unsigned char *end,
                                    unsigned int &controlLength,
                                        unsigned int &dataLength,
                                            unsigned int &trailerLength) = 0;

  unsigned char *allocateBuffer(unsigned int newSize);

  void appendBuffer(const unsigned char *message, unsigned int length);

  void convertBuffer();

  Transport *transport_;

  unsigned char *buffer_;

  unsigned int length_;
  unsigned int size_;
  unsigned int start_;
  unsigned int remaining_;

  int owner_;

  unsigned int initialReadSize_;
  unsigned int maximumBufferSize_;
};

#endif /* ReadBuffer_H */
