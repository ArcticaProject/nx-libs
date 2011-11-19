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

#include "ConfigureWindow.h"

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

int ConfigureWindowStore::parseIdentity(Message *message, const unsigned char *buffer,
                                            unsigned int size, int bigEndian) const
{
  ConfigureWindowMessage *configureWindow = (ConfigureWindowMessage *) message;

  //
  // Here is the fingerprint.
  //

  configureWindow -> window = GetULONG(buffer + 4, bigEndian); 

  configureWindow -> value_mask = GetUINT(buffer + 8, bigEndian);

  //
  // To increase effectiveness of the caching algorithm 
  // we remove the unused bytes carried in the data part.
  //

  if ((int) size > dataOffset)
  {
    #ifdef DEBUG
    *logofs << name() << ": Removing unused bytes from the data payload.\n" << logofs_flush;
    #endif

    configureWindow -> value_mask &= (1 << 7) - 1;

    unsigned int mask = 0x1;
    unsigned char *source = (unsigned char *) buffer + CONFIGUREWINDOW_DATA_OFFSET;
    unsigned long value = 0;

    for (unsigned int i = 0; i < 7; i++)
    {
      if (configureWindow -> value_mask & mask)
      {
        value = GetULONG(source, bigEndian);

        value &= (1 << CONFIGUREWINDOW_FIELD_WIDTH[i]) - 1;

        PutULONG(value, source, bigEndian);

        source += 4;
      }
      mask <<= 1;
    }
  }

  #ifdef DEBUG
  *logofs << name() << ": Parsed Identity for message at " << this << ".\n" << logofs_flush;
  #endif

  return 1;
}

int ConfigureWindowStore::unparseIdentity(const Message *message, unsigned char *buffer,
                                              unsigned int size, int bigEndian) const
{
  ConfigureWindowMessage *configureWindow = (ConfigureWindowMessage *) message;

  //
  // Fill all the message's fields.
  //

  PutULONG(configureWindow -> window, buffer + 4, bigEndian);

  PutUINT(configureWindow -> value_mask, buffer + 8, bigEndian);

  #ifdef DEBUG
  *logofs << name() << ": Unparsed identity for message at " << this << ".\n" << logofs_flush;
  #endif

  return 1;
}

void ConfigureWindowStore::dumpIdentity(const Message *message) const
{
  #ifdef DUMP

  ConfigureWindowMessage *configureWindow = (ConfigureWindowMessage *) message;

  *logofs << "ConfigureWindow: window " << configureWindow -> window  
          << ", value_mask " << configureWindow -> value_mask
          << ", size " << configureWindow -> size_ << ".\n";

  #endif
}

void ConfigureWindowStore::identityChecksum(const Message *message, const unsigned char *buffer,
                                                unsigned int size, int bigEndian) const
{
  md5_append(md5_state_, buffer + 4, 4);
  md5_append(md5_state_, buffer + 8, 2);
}
