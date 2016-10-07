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

#ifndef GetProperty_H
#define GetProperty_H

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

#define GETPROPERTY_ENABLE_CACHE               1
#define GETPROPERTY_ENABLE_DATA                0
#define GETPROPERTY_ENABLE_SPLIT               0
#define GETPROPERTY_ENABLE_COMPRESS            0

#define GETPROPERTY_DATA_LIMIT                 0
#define GETPROPERTY_DATA_OFFSET                24

#define GETPROPERTY_CACHE_SLOTS                2000
#define GETPROPERTY_CACHE_THRESHOLD            2
#define GETPROPERTY_CACHE_LOWER_THRESHOLD      1

//
// The message class.
//

class GetPropertyMessage : public Message
{
  friend class GetPropertyStore;

  public:

  GetPropertyMessage()
  {
  }

  ~GetPropertyMessage()
  {
  }

  //
  // Put here the fields which constitute 
  // the 'identity' part of the message.
  //

  private:

  unsigned char  property_delete;
  unsigned long  window;
  unsigned long  property;
  unsigned long  type;
  unsigned long  long_offset;
  unsigned long  long_length;
};

class GetPropertyStore : public MessageStore
{

  //
  // Constructors and destructors.
  //

  public:

  GetPropertyStore() : MessageStore()
  {
    enableCache    = GETPROPERTY_ENABLE_CACHE;
    enableData     = GETPROPERTY_ENABLE_DATA;
    enableSplit    = GETPROPERTY_ENABLE_SPLIT;
    enableCompress = GETPROPERTY_ENABLE_COMPRESS;

    dataLimit  = GETPROPERTY_DATA_LIMIT;
    dataOffset = GETPROPERTY_DATA_OFFSET;

    cacheSlots          = GETPROPERTY_CACHE_SLOTS;
    cacheThreshold      = GETPROPERTY_CACHE_THRESHOLD;
    cacheLowerThreshold = GETPROPERTY_CACHE_LOWER_THRESHOLD;

    messages_ -> resize(cacheSlots);

    for (T_messages::iterator i = messages_ -> begin();
             i < messages_ -> end(); i++)
    {
      *i = NULL;
    }

    temporary_ = NULL;
  }

  virtual ~GetPropertyStore()
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
    return "GetProperty";
  }

  virtual unsigned char opcode() const
  {
    return X_GetProperty;
  }

  virtual unsigned int storage() const
  {
    return sizeof(GetPropertyMessage);
  }

  //
  // Message handling methods.
  //

  protected:

  virtual Message *create() const
  {
    return new GetPropertyMessage();
  }

  virtual Message *create(const Message &message) const
  {
    return new GetPropertyMessage((const GetPropertyMessage &) message);
  }

  virtual void destroy(Message *message) const
  {
    delete (GetPropertyMessage *) message;
  }

  virtual int parseIdentity(Message *message, const unsigned char *buffer,
                                unsigned int size, int bigEndian) const;

  virtual int unparseIdentity(const Message *message, unsigned char *buffer,
                                  unsigned int size, int bigEndian) const;

  virtual void identityChecksum(const Message *message, const unsigned char *buffer,
                                    unsigned int size, int bigEndian) const;

  virtual void dumpIdentity(const Message *message) const;
};

#endif /* GetProperty_H */
