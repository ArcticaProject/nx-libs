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

#include "InternAtom.h"

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

int InternAtomStore::parseIdentity(Message *message, const unsigned char *buffer,
                                       unsigned int size, int bigEndian) const
{
  InternAtomMessage *internAtom = (InternAtomMessage *) message;

  //
  // Here is the fingerprint.
  //

  internAtom -> only_if_exists = *(buffer + 1);
  internAtom -> name_length    = GetUINT(buffer + 4, bigEndian);

  #ifdef DEBUG
  *logofs << name() << ": Parsed identity for message at " << message << ".\n" << logofs_flush;
  #endif

  //
  // Clean up padding bytes.
  //

  if ((int) size > dataOffset)
  {
    unsigned char *end = ((unsigned char *) buffer) + size;

    for (unsigned char *pad = ((unsigned char *) buffer) + 8 + 
                                  internAtom -> name_length; pad < end; pad++)
    {
      *pad = 0;
    }
  }

  return 1;
}

int InternAtomStore::unparseIdentity(const Message *message, unsigned char *buffer,
                                         unsigned int size, int bigEndian) const
{
  InternAtomMessage *internAtom = (InternAtomMessage *) message;

  //
  // Fill all the message's fields.
  //

  *(buffer + 1) = internAtom -> only_if_exists;

  PutUINT(internAtom -> name_length, buffer + 4, bigEndian);

  #ifdef DEBUG
  *logofs << name() << ": Unparsed identity for message at " << message << ".\n" << logofs_flush;
  #endif

  return 1;
}

void InternAtomStore::dumpIdentity(const Message *message) const
{
  #ifdef DUMP

  InternAtomMessage *internAtom = (InternAtomMessage *) message;

  *logofs << name() << ": Identity only_if_exists " 
          << (unsigned int) internAtom -> only_if_exists
          << ", name_length " << internAtom -> name_length
          << ", name '";

          for (int i = 0; i < internAtom -> name_length; i++)
          {
            *logofs << internAtom -> data_[i];
          }

          *logofs << "'.\n" << logofs_flush;

  #endif
}

void InternAtomStore::identityChecksum(const Message *message, const unsigned char *buffer,
                                           unsigned int size, int bigEndian) const
{
  md5_append(md5_state_, buffer + 1, 1);
  md5_append(md5_state_, buffer + 4, 2);
}
