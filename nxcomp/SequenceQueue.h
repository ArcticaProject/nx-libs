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
