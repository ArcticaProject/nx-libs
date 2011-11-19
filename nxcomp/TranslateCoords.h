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

#ifndef TranslateCoords_H
#define TranslateCoords_H

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

#define TRANSLATECOORDS_ENABLE_CACHE               1
#define TRANSLATECOORDS_ENABLE_DATA                0
#define TRANSLATECOORDS_ENABLE_SPLIT               0
#define TRANSLATECOORDS_ENABLE_COMPRESS            0

#define TRANSLATECOORDS_DATA_LIMIT                 0
#define TRANSLATECOORDS_DATA_OFFSET                16

#define TRANSLATECOORDS_CACHE_SLOTS                3000
#define TRANSLATECOORDS_CACHE_THRESHOLD            3
#define TRANSLATECOORDS_CACHE_LOWER_THRESHOLD      1

//
// The message class.
//

class TranslateCoordsMessage : public Message
{
  friend class TranslateCoordsStore;

  public:

  TranslateCoordsMessage()
  {
  }

  ~TranslateCoordsMessage()
  {
  }

  //
  // Put here the fields which constitute 
  // the 'identity' part of the message.
  //

  private:

  unsigned int  src_window;
  unsigned int  dst_window;
  unsigned int  src_x;
  unsigned int  src_y;

  unsigned char r_same_screen;
  unsigned int  r_child_window;
  unsigned int  r_dst_x;
  unsigned int  r_dst_y;
};

class TranslateCoordsStore : public MessageStore
{
  //
  // Constructors and destructors.
  //

  public:

  TranslateCoordsStore() : MessageStore()
  {
    enableCache    = TRANSLATECOORDS_ENABLE_CACHE;
    enableData     = TRANSLATECOORDS_ENABLE_DATA;
    enableSplit    = TRANSLATECOORDS_ENABLE_SPLIT;
    enableCompress = TRANSLATECOORDS_ENABLE_COMPRESS;

    dataLimit  = TRANSLATECOORDS_DATA_LIMIT;
    dataOffset = TRANSLATECOORDS_DATA_OFFSET;

    cacheSlots          = TRANSLATECOORDS_CACHE_SLOTS;
    cacheThreshold      = TRANSLATECOORDS_CACHE_THRESHOLD;
    cacheLowerThreshold = TRANSLATECOORDS_CACHE_LOWER_THRESHOLD;

    messages_ -> resize(cacheSlots);

    for (T_messages::iterator i = messages_ -> begin();
             i < messages_ -> end(); i++)
    {
      *i = NULL;
    }

    temporary_ = NULL;
  }

  virtual ~TranslateCoordsStore()
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
    return "TranslateCoords";
  }

  virtual unsigned char opcode() const
  {
    return X_TranslateCoords;
  }

  virtual unsigned int storage() const
  {
    return sizeof(TranslateCoordsMessage);
  }

  //
  // Message handling methods.
  //

  public:

  virtual Message *create() const
  {
    return new TranslateCoordsMessage();
  }

  virtual Message *create(const Message &message) const
  {
    return new TranslateCoordsMessage((const TranslateCoordsMessage &) message);
  }

  virtual void destroy(Message *message) const
  {
    delete (TranslateCoordsMessage *) message;
  }

  virtual int parseIdentity(Message *message, const unsigned char *buffer,
                                unsigned int size, int bigEndian) const;

  virtual int unparseIdentity(const Message *message, unsigned char *buffer,
                                  unsigned int size, int bigEndian) const;

  virtual void identityChecksum(const Message *message, const unsigned char *buffer,
                                    unsigned int size, int bigEndian) const;

  virtual void dumpIdentity(const Message *message) const;
};

#endif /* TranslateCoords_H */
