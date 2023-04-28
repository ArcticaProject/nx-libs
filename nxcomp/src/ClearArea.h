/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2011 NoMachine (http://www.nomachine.com)          */
/* Copyright (c) 2008-2017 Oleksandr Shneyder <o.shneyder@phoca-gmbh.de>  */
/* Copyright (c) 2014-2022 Ulrich Sibiller <uli42@gmx.de>                 */
/* Copyright (c) 2014-2019 Mihai Moldovan <ionic@ionic.de>                */
/* Copyright (c) 2011-2022 Mike Gabriel <mike.gabriel@das-netzwerkteam.de>*/
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

#ifndef ClearArea_H
#define ClearArea_H

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

#define CLEARAREA_ENABLE_CACHE               1
#define CLEARAREA_ENABLE_DATA                0
#define CLEARAREA_ENABLE_SPLIT               0
#define CLEARAREA_ENABLE_COMPRESS            0

#define CLEARAREA_DATA_LIMIT                 0
#define CLEARAREA_DATA_OFFSET                16

#define CLEARAREA_CACHE_SLOTS                3000
#define CLEARAREA_CACHE_THRESHOLD            5
#define CLEARAREA_CACHE_LOWER_THRESHOLD      1

//
// The message class.
//

class ClearAreaMessage : public Message
{
  friend class ClearAreaStore;

  public:

  ClearAreaMessage()
  {
  }

  ~ClearAreaMessage()
  {
  }

  //
  // Put here the fields which constitute
  // the 'identity' part of the message.
  //

  private:

  unsigned char  exposures;
  unsigned int   window;
  unsigned short x;
  unsigned short y;
  unsigned short width;
  unsigned short height;
};

class ClearAreaStore : public MessageStore
{
  //
  // Constructors and destructors.
  //

  public:

  ClearAreaStore() : MessageStore()
  {
    enableCache    = CLEARAREA_ENABLE_CACHE;
    enableData     = CLEARAREA_ENABLE_DATA;
    enableSplit    = CLEARAREA_ENABLE_SPLIT;
    enableCompress = CLEARAREA_ENABLE_COMPRESS;

    dataLimit  = CLEARAREA_DATA_LIMIT;
    dataOffset = CLEARAREA_DATA_OFFSET;

    cacheSlots          = CLEARAREA_CACHE_SLOTS;
    cacheThreshold      = CLEARAREA_CACHE_THRESHOLD;
    cacheLowerThreshold = CLEARAREA_CACHE_LOWER_THRESHOLD;

    messages_ -> resize(cacheSlots);

    for (T_messages::iterator i = messages_ -> begin();
             i < messages_ -> end(); i++)
    {
      *i = NULL;
    }

    temporary_ = NULL;
  }

  virtual ~ClearAreaStore()
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
    return "ClearArea";
  }

  virtual unsigned char opcode() const
  {
    return X_ClearArea;
  }

  virtual unsigned int storage() const
  {
    return sizeof(ClearAreaMessage);
  }

  //
  // Message handling methods.
  //

  public:

  virtual Message *create() const
  {
    return new ClearAreaMessage();
  }

  virtual Message *create(const Message &message) const
  {
    return new ClearAreaMessage((const ClearAreaMessage &) message);
  }

  virtual void destroy(Message *message) const
  {
    delete (ClearAreaMessage *) message;
  }

  virtual int parseIdentity(Message *message, const unsigned char *buffer,
                                unsigned int size, int bigEndian) const;

  virtual int unparseIdentity(const Message *message, unsigned char *buffer,
                                  unsigned int size, int bigEndian) const;

  virtual void identityChecksum(const Message *message, const unsigned char *buffer,
                                    unsigned int size, int bigEndian) const;

  virtual void dumpIdentity(const Message *message) const;
};

#endif /* ClearArea_H */
