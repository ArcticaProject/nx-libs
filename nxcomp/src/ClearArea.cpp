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

  *logofs << name() << ": Identity exposures " << (unsigned int) clearArea -> exposures 
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
