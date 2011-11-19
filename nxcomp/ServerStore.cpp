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

#include "ServerStore.h"

//
// Cached reply classes.
//

#include "GetImageReply.h"
#include "ListFontsReply.h"
#include "QueryFontReply.h"
#include "GetPropertyReply.h"
#include "GenericReply.h"

//
// Set the verbosity level.
//

#define WARNING
#define PANIC
#undef  TEST

ServerStore::ServerStore(StaticCompressor *compressor)
{
  if (logofs == NULL)
  {
    logofs = &cout;
  }

  for (int i = 0; i < CHANNEL_STORE_OPCODE_LIMIT; i++)
  {
    replies_[i] = NULL;
    events_[i]  = NULL;
  }

  replies_[X_ListFonts]   = new ListFontsReplyStore(compressor);
  replies_[X_QueryFont]   = new QueryFontReplyStore(compressor);
  replies_[X_GetImage]    = new GetImageReplyStore(compressor);
  replies_[X_GetProperty] = new GetPropertyReplyStore(compressor);

  replies_[X_NXInternalGenericReply] = new GenericReplyStore(compressor);
}

ServerStore::~ServerStore()
{
  if (logofs == NULL)
  {
    logofs = &cout;
  }

  for (int i = 0; i < CHANNEL_STORE_OPCODE_LIMIT; i++)
  {
    delete replies_[i];
    delete events_[i];
  }
}

int ServerStore::saveReplyStores(ostream *cachefs, md5_state_t *md5StateStream,
                                     md5_state_t *md5StateClient, T_checksum_action checksumAction,
                                         T_data_action dataAction) const
{
  for (int i = 0; i < CHANNEL_STORE_OPCODE_LIMIT; i++)
  {
    if (replies_[i] != NULL &&
            replies_[i] -> saveStore(cachefs, md5StateStream, md5StateClient,
                                         checksumAction, dataAction,
                                             storeBigEndian()) < 0)
    {
      #ifdef PANIC
      *logofs << "ServerStore: PANIC! Error saving reply store "
              << "for OPCODE#" << (unsigned int) i << ".\n"
              << logofs_flush;
      #endif

      cerr << "Error" << ": Error saving reply store "
           << "for opcode '" << (unsigned int) i << "'.\n";

      return -1;
    }
  }

  return 1;
}

int ServerStore::saveEventStores(ostream *cachefs, md5_state_t *md5StateStream,
                                     md5_state_t *md5StateClient, T_checksum_action checksumAction,
                                         T_data_action dataAction) const
{
  for (int i = 0; i < CHANNEL_STORE_OPCODE_LIMIT; i++)
  {
    if (events_[i] != NULL &&
            events_[i] -> saveStore(cachefs, md5StateStream, md5StateClient,
                                        checksumAction, dataAction,
                                            storeBigEndian()) < 0)
    {
      #ifdef PANIC
      *logofs << "ServerStore: PANIC! Error saving event store "
              << "for OPCODE#" << (unsigned int) i << ".\n"
              << logofs_flush;
      #endif

      cerr << "Error" << ": Error saving event store "
           << "for opcode '" << (unsigned int) i << "'.\n";

      return -1;
    }
  }

  return 1;
}

int ServerStore::loadReplyStores(istream *cachefs, md5_state_t *md5StateStream,
                                     T_checksum_action checksumAction, T_data_action dataAction) const
{
  for (int i = 0; i < CHANNEL_STORE_OPCODE_LIMIT; i++)
  {
    if (replies_[i] != NULL &&
            replies_[i] -> loadStore(cachefs, md5StateStream,
                                         checksumAction, dataAction,
                                             storeBigEndian()) < 0)
    {
      #ifdef PANIC
      *logofs << "ServerStore: PANIC! Error loading reply store "
              << "for OPCODE#" << (unsigned int) i << ".\n"
              << logofs_flush;
      #endif

      return -1;
    }
  }

  return 1;
}

int ServerStore::loadEventStores(istream *cachefs, md5_state_t *md5StateStream,
                                     T_checksum_action checksumAction, T_data_action dataAction) const
{
  for (int i = 0; i < CHANNEL_STORE_OPCODE_LIMIT; i++)
  {
    if (events_[i] != NULL &&
            events_[i] -> loadStore(cachefs, md5StateStream,
                                        checksumAction, dataAction,
                                            storeBigEndian()) < 0)
    {
      #ifdef PANIC
      *logofs << "ServerStore: PANIC! Error loading event store "
              << "for OPCODE#" << (unsigned int) i << ".\n"
              << logofs_flush;
      #endif

      return -1;
    }
  }

  return 1;
}
