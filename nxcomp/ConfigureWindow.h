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

#ifndef ConfigureWindow_H
#define ConfigureWindow_H

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

#define CONFIGUREWINDOW_ENABLE_CACHE               1
#define CONFIGUREWINDOW_ENABLE_DATA                0
#define CONFIGUREWINDOW_ENABLE_SPLIT               0
#define CONFIGUREWINDOW_ENABLE_COMPRESS            0

#define CONFIGUREWINDOW_DATA_LIMIT                 32
#define CONFIGUREWINDOW_DATA_OFFSET                12

#define CONFIGUREWINDOW_CACHE_SLOTS                3000
#define CONFIGUREWINDOW_CACHE_THRESHOLD            5
#define CONFIGUREWINDOW_CACHE_LOWER_THRESHOLD      1

//
// The message class.
//

class ConfigureWindowMessage : public Message
{
  friend class ConfigureWindowStore;

  public:

  ConfigureWindowMessage()
  {
  }

  ~ConfigureWindowMessage()
  {
  }

  //
  // Put here the fields which constitute 
  // the 'identity' part of the message.
  //

  private:

  unsigned int   window;
  unsigned short value_mask;
};

class ConfigureWindowStore : public MessageStore
{
  //
  // Constructors and destructors.
  //

  public:

  ConfigureWindowStore() : MessageStore()
  {
    enableCache    = CONFIGUREWINDOW_ENABLE_CACHE;
    enableData     = CONFIGUREWINDOW_ENABLE_DATA;
    enableSplit    = CONFIGUREWINDOW_ENABLE_SPLIT;
    enableCompress = CONFIGUREWINDOW_ENABLE_COMPRESS;

    dataLimit  = CONFIGUREWINDOW_DATA_LIMIT;
    dataOffset = CONFIGUREWINDOW_DATA_OFFSET;

    cacheSlots          = CONFIGUREWINDOW_CACHE_SLOTS;
    cacheThreshold      = CONFIGUREWINDOW_CACHE_THRESHOLD;
    cacheLowerThreshold = CONFIGUREWINDOW_CACHE_LOWER_THRESHOLD;

    messages_ -> resize(cacheSlots);

    for (T_messages::iterator i = messages_ -> begin();
             i < messages_ -> end(); i++)
    {
      *i = NULL;
    }

    temporary_ = NULL;
  }

  virtual ~ConfigureWindowStore()
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
    return "ConfigureWindow";
  }

  virtual unsigned char opcode() const
  {
    return X_ConfigureWindow;
  }

  virtual unsigned int storage() const
  {
    return sizeof(ConfigureWindowMessage);
  }

  //
  // Message handling methods.
  //

  public:

  virtual Message *create() const
  {
    return new ConfigureWindowMessage();
  }

  virtual Message *create(const Message &message) const
  {
    return new ConfigureWindowMessage((const ConfigureWindowMessage &) message);
  }

  virtual void destroy(Message *message) const
  {
    delete (ConfigureWindowMessage *) message;
  }

  virtual int parseIdentity(Message *message, const unsigned char *buffer,
                                unsigned int size, int bigEndian) const;

  virtual int unparseIdentity(const Message *message, unsigned char *buffer,
                                  unsigned int size, int bigEndian) const;

  virtual void identityChecksum(const Message *message, const unsigned char *buffer,
                                    unsigned int size, int bigEndian) const;

  virtual void dumpIdentity(const Message *message) const;
};

#endif /* ConfigureWindow_H */
