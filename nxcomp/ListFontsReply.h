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

#ifndef ListFontsReply_H
#define ListFontsReply_H

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

#define LISTFONTSREPLY_ENABLE_CACHE                       1
#define LISTFONTSREPLY_ENABLE_DATA                        1
#define LISTFONTSREPLY_ENABLE_SPLIT                       0

#define LISTFONTSREPLY_DATA_LIMIT                         1048576 - 32
#define LISTFONTSREPLY_DATA_OFFSET                        32

#define LISTFONTSREPLY_CACHE_SLOTS                        200
#define LISTFONTSREPLY_CACHE_THRESHOLD                    20
#define LISTFONTSREPLY_CACHE_LOWER_THRESHOLD              5

#define LISTFONTSREPLY_ENABLE_COMPRESS_IF_PROTO_STEP_7    0

//
// The message class.
//

class ListFontsReplyMessage : public Message
{
  friend class ListFontsReplyStore;

  public:

  ListFontsReplyMessage()
  {
  }

  ~ListFontsReplyMessage()
  {
  }

  //
  // Put here the fields which constitute 
  // the 'identity' part of the message.
  //

  private:

  unsigned short int number_of_names;
};

class ListFontsReplyStore : public MessageStore
{
  //
  // Constructors and destructors.
  //

  public:

  ListFontsReplyStore(StaticCompressor *compressor);

  virtual ~ListFontsReplyStore();

  virtual const char *name() const
  {
    return "ListFontsReply";
  }

  virtual unsigned char opcode() const
  {
    return X_ListFonts;
  }

  virtual unsigned int storage() const
  {
    return sizeof(ListFontsReplyMessage);
  }

  //
  // Message handling methods.
  //

  protected:

  virtual Message *create() const
  {
    return new ListFontsReplyMessage();
  }

  virtual Message *create(const Message &message) const
  {
    return new ListFontsReplyMessage((const ListFontsReplyMessage &) message);
  }

  virtual void destroy(Message *message) const
  {
    delete (ListFontsReplyMessage *) message;
  }

  virtual int parseIdentity(Message *message, const unsigned char *buffer, 
                                unsigned int size, int bigEndian) const;

  virtual int unparseIdentity(const Message *message, unsigned char *buffer, 
                                  unsigned int size, int bigEndian) const;

  virtual void identityChecksum(const Message *message, const unsigned char *buffer, 
                                    unsigned int size, int bigEndian) const;

  virtual void dumpIdentity(const Message *message) const;
};

#endif /* ListFontsReply_H */
