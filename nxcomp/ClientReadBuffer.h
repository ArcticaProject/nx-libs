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

#ifndef ClientReadBuffer_H
#define ClientReadBuffer_H

#include "Control.h"
#include "ReadBuffer.h"

class ClientChannel;

class ClientReadBuffer : public ReadBuffer
{
  public:

  ClientReadBuffer(Transport *transport, ClientChannel *channel)

    : ReadBuffer(transport), firstMessage_(1), channel_(channel)
  {
  }

  virtual ~ClientReadBuffer()
  {
  }

  protected:

  virtual unsigned int suggestedLength(unsigned int pendingLength);

  virtual int locateMessage(const unsigned char *start,
                                const unsigned char *end,
                                    unsigned int &controlLength,
                                        unsigned int &dataLength,
                                            unsigned int &trailerLength);

  int bigEndian_;

  int firstMessage_;

  ClientChannel *channel_;
};

#endif /* ClientReadBuffer_H */
