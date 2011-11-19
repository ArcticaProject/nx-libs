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

#include "ChangeGCCompat.h"

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

int ChangeGCCompatStore::parseIdentity(Message *message, const unsigned char *buffer,
                                           unsigned int size, int bigEndian) const
{
  ChangeGCCompatMessage *changeGC = (ChangeGCCompatMessage *) message;

  //
  // Here is the fingerprint.
  //

  changeGC -> gcontext   = GetULONG(buffer + 4, bigEndian);
  changeGC -> value_mask = GetULONG(buffer + 8, bigEndian);

  //
  // Clear the unused bytes carried in the
  // payload to increase the effectiveness
  // of the caching algorithm.
  //

  if ((int) size > dataOffset)
  {
    #ifdef DEBUG
    *logofs << name() << ": Removing unused bytes from the "
            << "data payload.\n" << logofs_flush;
    #endif

    changeGC -> value_mask &= (1 << 23) - 1;

    unsigned int mask = 0x1;
    unsigned char *source = (unsigned char *) buffer + CHANGEGC_DATA_OFFSET;
    unsigned long value = 0;

    for (unsigned int i = 0; i < 23; i++)
    {
      if (changeGC -> value_mask & mask)
      {
        value = GetULONG(source, bigEndian);

        value &= (0xffffffff >> (32 - CREATEGC_FIELD_WIDTH[i]));

        PutULONG(value, source, bigEndian);

        source += 4;
      }

      mask <<= 1;
    }
  }

  #ifdef DEBUG
  *logofs << name() << ": Parsed Identity for message at "
          << this << ".\n" << logofs_flush;
  #endif

  return 1;
}

int ChangeGCCompatStore::unparseIdentity(const Message *message, unsigned char *buffer,
                                             unsigned int size, int bigEndian) const
{
  ChangeGCCompatMessage *changeGC = (ChangeGCCompatMessage *) message;

  //
  // Fill all the message's fields.
  //

  PutULONG(changeGC -> gcontext,   buffer + 4, bigEndian);
  PutULONG(changeGC -> value_mask, buffer + 8, bigEndian);

  #ifdef DEBUG
  *logofs << name() << ": Unparsed identity for message at "
          << this << ".\n" << logofs_flush;
  #endif

  return 1;
}

void ChangeGCCompatStore::dumpIdentity(const Message *message) const
{
  #ifdef DUMP

  ChangeGCCompatMessage *changeGC = (ChangeGCCompatMessage *) message;

  *logofs << name() << ": Identity gcontext " << changeGC -> gcontext 
                    << ", mask " << changeGC -> value_mask << ", size "
                    << changeGC -> size_ << ".\n" << logofs_flush;
  #endif
}

void ChangeGCCompatStore::identityChecksum(const Message *message, const unsigned char *buffer,
                                               unsigned int size, int bigEndian) const
{
  md5_append(md5_state_, buffer + 4, 8);
}
