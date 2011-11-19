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

#ifndef ChangeGCCompat_H
#define ChangeGCCompat_H

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

#define CHANGEGC_ENABLE_CACHE                           1
#define CHANGEGC_ENABLE_DATA                            0
#define CHANGEGC_ENABLE_SPLIT                           0
#define CHANGEGC_ENABLE_COMPRESS                        0

#define CHANGEGC_DATA_LIMIT                             144
#define CHANGEGC_DATA_OFFSET                            12

#define CHANGEGC_CACHE_SLOTS                            3000
#define CHANGEGC_CACHE_THRESHOLD                        3
#define CHANGEGC_CACHE_LOWER_THRESHOLD                  1

//
// The message class.
//

class ChangeGCCompatMessage : public Message
{
  friend class ChangeGCCompatStore;

  public:

  ChangeGCCompatMessage()
  {
  }

  ~ChangeGCCompatMessage()
  {
  }

  //
  // Put here the fields which constitute 
  // the 'identity' part of the message.
  //

  private:

  unsigned int gcontext;
  unsigned int value_mask;
};

class ChangeGCCompatStore : public MessageStore
{
  //
  // Constructors and destructors.
  //

  public:

  ChangeGCCompatStore() : MessageStore()
  {
    enableCache    = CHANGEGC_ENABLE_CACHE;
    enableData     = CHANGEGC_ENABLE_DATA;
    enableSplit    = CHANGEGC_ENABLE_SPLIT;
    enableCompress = CHANGEGC_ENABLE_COMPRESS;

    dataLimit  = CHANGEGC_DATA_LIMIT;
    dataOffset = CHANGEGC_DATA_OFFSET;

    cacheSlots          = CHANGEGC_CACHE_SLOTS;
    cacheThreshold      = CHANGEGC_CACHE_THRESHOLD;
    cacheLowerThreshold = CHANGEGC_CACHE_LOWER_THRESHOLD;

    messages_ -> resize(cacheSlots);

    for (T_messages::iterator i = messages_ -> begin();
             i < messages_ -> end(); i++)
    {
      *i = NULL;
    }

    temporary_ = NULL;
  }

  virtual ~ChangeGCCompatStore()
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
    return "ChangeGCCompat";
  }

  virtual unsigned char opcode() const
  {
    return X_ChangeGC;
  }

  virtual unsigned int storage() const
  {
    return sizeof(ChangeGCCompatMessage);
  }

  //
  // Message handling methods.
  //

  public:

  virtual Message *create() const
  {
    return new ChangeGCCompatMessage();
  }

  virtual Message *create(const Message &message) const
  {
    return new ChangeGCCompatMessage((const ChangeGCCompatMessage &) message);
  }

  virtual void destroy(Message *message) const
  {
    delete (ChangeGCCompatMessage *) message;
  }

  virtual int parseIdentity(Message *message, const unsigned char *buffer,
                                unsigned int size, int bigEndian) const;

  virtual int unparseIdentity(const Message *message, unsigned char *buffer,
                                  unsigned int size, int bigEndian) const;

  virtual void identityChecksum(const Message *message, const unsigned char *buffer,
                                    unsigned int size, int bigEndian) const;

  virtual void dumpIdentity(const Message *message) const;
};

#endif /* ChangeGCCompat_H */
