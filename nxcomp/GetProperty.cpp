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

#include "GetProperty.h"

#include "ClientCache.h"

#include "EncodeBuffer.h"
#include "DecodeBuffer.h"

//
// Set the verbosity level.
//

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG
#undef  DUMP

//
// Here are the methods to handle messages' content.
//

int GetPropertyStore::parseIdentity(Message *message, const unsigned char *buffer,
                                        unsigned int size, int bigEndian) const
{
  GetPropertyMessage *getProperty = (GetPropertyMessage *) message;

  //
  // Here is the fingerprint.
  //

  getProperty -> property_delete = *(buffer + 1);

  getProperty -> window          = GetULONG(buffer + 4,  bigEndian);
  getProperty -> property        = GetULONG(buffer + 8,  bigEndian);
  getProperty -> type            = GetULONG(buffer + 12, bigEndian);
  getProperty -> long_offset     = GetULONG(buffer + 16, bigEndian);
  getProperty -> long_length     = GetULONG(buffer + 20, bigEndian);

  #ifdef DEBUG
  *logofs << name() << ": Parsed identity for message at " << message << ".\n" << logofs_flush;
  #endif

  return 1;
}

int GetPropertyStore::unparseIdentity(const Message *message, unsigned char *buffer,
                                          unsigned int size, int bigEndian) const
{

  GetPropertyMessage *getProperty = (GetPropertyMessage *) message;

  //
  // Fill all the message's fields.
  //

  *(buffer + 1) = getProperty -> property_delete;

  PutULONG(getProperty -> window,      buffer + 4,  bigEndian);
  PutULONG(getProperty -> property,    buffer + 8,  bigEndian);
  PutULONG(getProperty -> type,        buffer + 12, bigEndian);
  PutULONG(getProperty -> long_offset, buffer + 16, bigEndian);
  PutULONG(getProperty -> long_length, buffer + 20, bigEndian);

  #ifdef DEBUG
  *logofs << name() << ": Unparsed identity for message at " << message << ".\n" << logofs_flush;
  #endif

  return 1;
}

void GetPropertyStore::dumpIdentity(const Message *message) const
{
  #ifdef DUMP

  GetPropertyMessage *getProperty = (GetPropertyMessage *) message;

  *logofs << name() << ": Identity property_delete " << (unsigned int) getProperty -> property_delete
          << ", window " << getProperty -> window << ", property " << getProperty -> property
          << ", type " << getProperty -> type << ", long-offset " << getProperty -> long_offset
          << ", long-length " << getProperty -> long_length << ".\n" << logofs_flush;

  #endif
}

void GetPropertyStore::identityChecksum(const Message *message, const unsigned char *buffer,
                                            unsigned int size, int bigEndian) const
{
  md5_append(md5_state_, buffer + 1, 1);
  md5_append(md5_state_, buffer + 4, 20);
}
