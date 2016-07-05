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
