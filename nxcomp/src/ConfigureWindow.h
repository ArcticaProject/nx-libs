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
