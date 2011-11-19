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

#ifndef ServerReadBuffer_H
#define ServerReadBuffer_H

#include "ReadBuffer.h"
#include "Control.h"

class ServerChannel;

class ServerReadBuffer : public ReadBuffer
{
  public:

  ServerReadBuffer(Transport *transport, ServerChannel *channel)

    : ReadBuffer(transport), firstMessage_(1), channel_(channel)
  {
  }

  virtual ~ServerReadBuffer()
  {
  }

  void setBigEndian(int flag)
  {
    bigEndian_ = flag;
  }

  unsigned char *peekMessage(unsigned int &offset, unsigned char opcode,
                                 unsigned short sequence);

  protected:

  virtual unsigned int suggestedLength(unsigned int pendingLength);

  virtual int locateMessage(const unsigned char *start,
                                const unsigned char *end,
                                    unsigned int &controlLength,
                                        unsigned int &dataLength,
                                            unsigned int &trailerLength);

  int bigEndian_;

  int firstMessage_;

  ServerChannel *channel_;
};

#endif /* ServerReadBuffer_H */
