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
#define LISTFONTSREPLY_ENABLE_COMPRESS                    1

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
