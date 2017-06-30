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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

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
