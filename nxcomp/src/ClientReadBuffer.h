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
