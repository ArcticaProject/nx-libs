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

#ifndef GenericReadBuffer_H
#define GenericReadBuffer_H

#include "ReadBuffer.h"
#include "Control.h"

class GenericChannel;

class GenericReadBuffer : public ReadBuffer
{
  public:

  GenericReadBuffer(Transport *transport, GenericChannel *channel)

    : ReadBuffer(transport), channel_(channel)
  {
  }

  virtual ~GenericReadBuffer()
  {
  }

  protected:

  virtual unsigned int suggestedLength(unsigned int pendingLength);

  virtual int locateMessage(const unsigned char *start,
                                const unsigned char *end,
                                    unsigned int &controlLength,
                                        unsigned int &dataLength,
                                            unsigned int &trailerLength);

  GenericChannel *channel_;
};

#endif /* GenericReadBuffer_H */
