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

#include "TranslateCoords.h"

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

int TranslateCoordsStore::parseIdentity(Message *message, const unsigned char *buffer,
                                            unsigned int size, int bigEndian) const
{
  TranslateCoordsMessage *translateCoords = (TranslateCoordsMessage *) message;

  //
  // Here is the fingerprint.
  //

  translateCoords -> src_window = GetULONG(buffer + 4, bigEndian);
  translateCoords -> dst_window = GetULONG(buffer + 8, bigEndian);

  translateCoords -> src_x = GetUINT(buffer + 12, bigEndian);
  translateCoords -> src_y = GetUINT(buffer + 14, bigEndian);

  #ifdef DEBUG
  *logofs << name() << ": Parsed Identity for message at " << this << ".\n" << logofs_flush;
  #endif

  return 1;
}

int TranslateCoordsStore::unparseIdentity(const Message *message, unsigned char *buffer,
                                              unsigned int size, int bigEndian) const
{
  TranslateCoordsMessage *translateCoords = (TranslateCoordsMessage *) message;

  //
  // Fill all the message's fields.
  //

  PutULONG(translateCoords -> src_window, buffer + 4,  bigEndian);
  PutULONG(translateCoords -> dst_window, buffer + 8,  bigEndian);

  PutUINT(translateCoords -> src_x, buffer + 12, bigEndian);
  PutUINT(translateCoords -> src_y, buffer + 14, bigEndian);

  #ifdef DEBUG
  *logofs << name() << ": Unparsed identity for message at " << this << ".\n" << logofs_flush;
  #endif

  return 1;
}

void TranslateCoordsStore::dumpIdentity(const Message *message) const
{
  #ifdef DUMP

  TranslateCoordsMessage *translateCoords = (TranslateCoordsMessage *) message;

  *logofs << name() << ": Identity src_window " << translateCoords -> src_window << ", dst_window " 
          << translateCoords -> dst_window << ", src_x " << translateCoords -> src_x << ", src_y " 
          << translateCoords -> src_y << ", size " << translateCoords -> size_ << ".\n" << logofs_flush;

  #endif
}

void TranslateCoordsStore::identityChecksum(const Message *message, const unsigned char *buffer,
                                                unsigned int size, int bigEndian) const
{
  md5_append(md5_state_, buffer + 4,  4);
  md5_append(md5_state_, buffer + 8,  4);
  md5_append(md5_state_, buffer + 12, 2);
  md5_append(md5_state_, buffer + 14, 2);
}
