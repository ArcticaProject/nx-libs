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

#include "ClearArea.h"

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

int ClearAreaStore::parseIdentity(Message *message, const unsigned char *buffer,
                                      unsigned int size, int bigEndian) const
{
  ClearAreaMessage *clearArea = (ClearAreaMessage *) message;

  //
  // Here is the fingerprint.
  //

  clearArea -> exposures = *(buffer + 1); 

  clearArea -> window = GetULONG(buffer + 4, bigEndian); 

  clearArea -> x      = GetUINT(buffer + 8, bigEndian);
  clearArea -> y      = GetUINT(buffer + 10, bigEndian);
  clearArea -> width  = GetUINT(buffer + 12, bigEndian);
  clearArea -> height = GetUINT(buffer + 14, bigEndian);

  #ifdef DEBUG
  *logofs << name() << ": Parsed Identity for message at " << this << ".\n" << logofs_flush;
  #endif

  return 1;
}

int ClearAreaStore::unparseIdentity(const Message *message, unsigned char *buffer,
                                        unsigned int size, int bigEndian) const
{
  ClearAreaMessage *clearArea = (ClearAreaMessage *) message;

  //
  // Fill all the message's fields.
  //

  *(buffer + 1) = clearArea -> exposures;

  PutULONG(clearArea -> window, buffer + 4, bigEndian);

  PutUINT(clearArea -> x,      buffer + 8, bigEndian);
  PutUINT(clearArea -> y,      buffer + 10, bigEndian);
  PutUINT(clearArea -> width,  buffer + 12, bigEndian);
  PutUINT(clearArea -> height, buffer + 14, bigEndian);

  #ifdef DEBUG
  *logofs << name() << ": Unparsed identity for message at " << this << ".\n" << logofs_flush;
  #endif

  return 1;
}

void ClearAreaStore::dumpIdentity(const Message *message) const
{
  #ifdef DUMP

  ClearAreaMessage *clearArea = (ClearAreaMessage *) message;

  *logofs << name() << ": Identity exposures " << clearArea -> (unsigned int) exposures 
          << ", window " << clearArea -> window  << ", x " << clearArea -> x 
          << ", y " << clearArea -> y << ", width  " << clearArea -> width 
          << ", height " << clearArea -> height << ", size " << clearArea -> size_ 
          << ".\n";

  #endif
}

void ClearAreaStore::identityChecksum(const Message *message, const unsigned char *buffer,
                                          unsigned int size, int bigEndian) const
{
  md5_append(md5_state_, buffer + 1,  1);
  md5_append(md5_state_, buffer + 4,  4);
  md5_append(md5_state_, buffer + 8,  2);
  md5_append(md5_state_, buffer + 10, 2);
  md5_append(md5_state_, buffer + 12, 2);
  md5_append(md5_state_, buffer + 14, 2);
}
