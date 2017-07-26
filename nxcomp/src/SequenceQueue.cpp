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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "SequenceQueue.h"

static const unsigned int INITIAL_SIZE_    = 16;
static const unsigned int GROWTH_INCREMENT = 16;

SequenceQueue::SequenceQueue()

  : queue_(new RequestSequence[INITIAL_SIZE_]), size_(INITIAL_SIZE_),
        length_(0), start_(0), end_(0)
{
}

SequenceQueue::~SequenceQueue()
{
  delete [] queue_;
}

void SequenceQueue::push(unsigned short int sequence, unsigned char opcode,
                             unsigned int data1, unsigned int data2,
                                 unsigned int data3)
{
  if (length_ == 0)
  {
    start_ = end_ = 0;

    queue_[0].opcode   = opcode;
    queue_[0].sequence = sequence;

    queue_[0].data1 = data1;
    queue_[0].data2 = data2;
    queue_[0].data3 = data3;

    length_ = 1;

    return;
  }

  if (length_ == size_)
  {
    size_ += GROWTH_INCREMENT;

    RequestSequence *newQueue = new RequestSequence[size_];

    for (int i = start_; (unsigned int) i < length_; i++)
    {
      newQueue[i - start_] = queue_[i];
    }

    for (int i1 = 0; (unsigned int) i1 < start_; i1++)
    {
      newQueue[i1 + length_ - start_] = queue_[i1];
    }

    delete [] queue_;

    queue_ = newQueue;

    start_ = 0;

    end_ = length_ - 1;
  }

  end_++;

  if (end_ == size_)
  {
    end_ = 0;
  }

  queue_[end_].opcode   = opcode;
  queue_[end_].sequence = sequence;

  queue_[end_].data1 = data1;
  queue_[end_].data2 = data2;
  queue_[end_].data3 = data3;

  length_++;
}

int SequenceQueue::peek(unsigned short int &sequence,
                            unsigned char &opcode)
{
  if (length_ == 0)
  {
    return 0;
  }
  else
  {
    opcode   = queue_[start_].opcode;
    sequence = queue_[start_].sequence;

    return 1;
  }
}

int SequenceQueue::peek(unsigned short int &sequence, unsigned char &opcode,
                            unsigned int &data1, unsigned int &data2,
                                unsigned int &data3)
{
  if (length_ == 0)
  {
    return 0;
  }
  else
  {
    opcode   = queue_[start_].opcode;
    sequence = queue_[start_].sequence;

    data1 = queue_[start_].data1;
    data2 = queue_[start_].data2;
    data3 = queue_[start_].data3;

    return 1;
  }
}

int SequenceQueue::pop(unsigned short int &sequence, unsigned char &opcode, 
                           unsigned int &data1, unsigned int &data2,
                               unsigned int &data3)
{
  if (length_ == 0)
  {
    return 0;
  }
  else
  {
    opcode   = queue_[start_].opcode;
    sequence = queue_[start_].sequence;

    data1 = queue_[start_].data1;
    data2 = queue_[start_].data2;
    data3 = queue_[start_].data3;

    start_++;

    if (start_ == size_)
    {
      start_ = 0;
    }

    length_--;

    return 1;
  }
}
