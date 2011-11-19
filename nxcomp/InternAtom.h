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

#ifndef InternAtom_H
#define InternAtom_H

#include "Message.h"

//
// Set the verbosity level.
//

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG
#undef  DUMP

//
// Set default values.
//

#define INTERNATOM_ENABLE_CACHE               1
#define INTERNATOM_ENABLE_DATA                0
#define INTERNATOM_ENABLE_SPLIT               0
#define INTERNATOM_ENABLE_COMPRESS            0

#define INTERNATOM_DATA_LIMIT                 80
#define INTERNATOM_DATA_OFFSET                8

#define INTERNATOM_CACHE_SLOTS                2000
#define INTERNATOM_CACHE_THRESHOLD            2
#define INTERNATOM_CACHE_LOWER_THRESHOLD      1

//
// The message class.
//

class InternAtomMessage : public Message
{
  friend class InternAtomStore;

  public:

  InternAtomMessage()
  {
  }

  ~InternAtomMessage()
  {
  }

  //
  // Put here the fields which constitute 
  // the 'identity' part of the message.
  //

  private:

  unsigned char  only_if_exists;
  unsigned short name_length;
};

class InternAtomStore : public MessageStore
{
  //
  // Constructors and destructors.
  //

  public:

  InternAtomStore() : MessageStore()
  {
    enableCache    = INTERNATOM_ENABLE_CACHE;
    enableData     = INTERNATOM_ENABLE_DATA;
    enableSplit    = INTERNATOM_ENABLE_SPLIT;
    enableCompress = INTERNATOM_ENABLE_COMPRESS;

    dataLimit  = INTERNATOM_DATA_LIMIT;
    dataOffset = INTERNATOM_DATA_OFFSET;

    cacheSlots          = INTERNATOM_CACHE_SLOTS;
    cacheThreshold      = INTERNATOM_CACHE_THRESHOLD;
    cacheLowerThreshold = INTERNATOM_CACHE_LOWER_THRESHOLD;

    messages_ -> resize(cacheSlots);

    for (T_messages::iterator i = messages_ -> begin();
             i < messages_ -> end(); i++)
    {
      *i = NULL;
    }

    temporary_ = NULL;
  }

  virtual ~InternAtomStore()
  {
    for (T_messages::iterator i = messages_ -> begin();
             i < messages_ -> end(); i++)
    {
      destroy(*i);
    }

    destroy(temporary_);
  }

  virtual const char *name() const
  {
    return "InternAtom";
  }

  virtual unsigned char opcode() const
  {
    return X_InternAtom;
  }

  virtual unsigned int storage() const
  {
    return sizeof(InternAtomMessage);
  }

  //
  // Message handling methods.
  //

  protected:

  virtual Message *create() const
  {
    return new InternAtomMessage();
  }

  virtual Message *create(const Message &message) const
  {
    return new InternAtomMessage((const InternAtomMessage &) message);
  }

  virtual void destroy(Message *message) const
  {
    delete (InternAtomMessage *) message;
  }

  virtual int parseIdentity(Message *message, const unsigned char *buffer,
                                unsigned int size, int bigEndian) const;

  virtual int unparseIdentity(const Message *message, unsigned char *buffer,
                                  unsigned int size, int bigEndian) const;

  virtual void identityChecksum(const Message *message, const unsigned char *buffer,
                                    unsigned int size, int bigEndian) const;

  virtual void dumpIdentity(const Message *message) const;
};

#endif /* InternAtom_H */
