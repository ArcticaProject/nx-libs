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

#ifndef ServerStore_H
#define ServerStore_H

#include "Message.h"

#include "ChannelStore.h"

class StaticCompressor;

class ServerStore : public ChannelStore
{
  public:

  ServerStore(StaticCompressor *compressor);

  virtual ~ServerStore();

  MessageStore *getReplyStore(unsigned char opcode) const
  {
    return replies_[opcode];
  }

  MessageStore *getEventStore(unsigned char opcode) const
  {
    return events_[opcode];
  }

  //
  // Actually save the message store
  // to disk according to proxy mode.
  //

  int saveReplyStores(ostream *cachefs, md5_state_t *md5StateStream,
                          md5_state_t *md5StateClient, T_checksum_action checksumAction,
                              T_data_action dataAction) const;

  int saveEventStores(ostream *cachefs, md5_state_t *md5StateStream,
                          md5_state_t *md5StateClient, T_checksum_action checksumAction,
                              T_data_action dataAction) const;


  int loadReplyStores(istream *cachefs, md5_state_t *md5StateStream,
                          T_checksum_action checksumAction, T_data_action dataAction) const;

  int loadEventStores(istream *cachefs, md5_state_t *md5StateStream,
                          T_checksum_action checksumAction, T_data_action dataAction) const;

  private:

  //
  // A server store contains replies and events.
  //

  MessageStore *replies_[CHANNEL_STORE_OPCODE_LIMIT];
  MessageStore *events_[CHANNEL_STORE_OPCODE_LIMIT];
};

#endif /* ServerStore_H */
