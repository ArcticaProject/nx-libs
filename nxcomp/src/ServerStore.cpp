/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2011 NoMachine (http://www.nomachine.com)          */
/* Copyright (c) 2008-2017 Oleksandr Shneyder <o.shneyder@phoca-gmbh.de>  */
/* Copyright (c) 2014-2022 Ulrich Sibiller <uli42@gmx.de>                 */
/* Copyright (c) 2014-2019 Mihai Moldovan <ionic@ionic.de>                */
/* Copyright (c) 2011-2022 Mike Gabriel <mike.gabriel@das-netzwerkteam.de>*/
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
