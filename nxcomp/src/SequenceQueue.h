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

#ifndef SequenceQueue_H
#define SequenceQueue_H

//
// List of outstanding request messages which
// are waiting for a reply. This class is used
// in X client and server channels to correlate
// the replies sequence numbers to the original
// request type.
//

class SequenceQueue
{
  public:

  SequenceQueue();

  virtual ~SequenceQueue();

  void push(unsigned short int sequence, unsigned char opcode,
                unsigned int data1 = 0, unsigned int data2 = 0,
                    unsigned int data3 = 0);

  int peek(unsigned short int &sequence, unsigned char &opcode);

  int peek(unsigned short int &sequence, unsigned char &opcode,
               unsigned int &data1, unsigned int &data2,
                   unsigned int &data3);

  int pop(unsigned short int &sequence, unsigned char &opcode, 
              unsigned int &data1, unsigned int &data2,
                  unsigned int &data3);

  int pop(unsigned short int &sequence, unsigned char &opcode)
  {
    unsigned int data1, data2, data3;

    return pop(sequence, opcode, data1, data2, data3);
  }

  int length()
  {
    return length_;
  }

  private:

  struct RequestSequence
  {
    unsigned short int sequence;
    unsigned char      opcode;
    unsigned int       data1;
    unsigned int       data2;
    unsigned int       data3;
  };

  RequestSequence *queue_;

  unsigned int size_;
  unsigned int length_;

  unsigned int start_;
  unsigned int end_;
};

#endif /* SequenceQueue_H */
