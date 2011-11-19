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
