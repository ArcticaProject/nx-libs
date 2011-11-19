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

#ifndef ClientStore_H
#define ClientStore_H

#include "Message.h"
#include "Split.h"

#include "ChannelStore.h"

class StaticCompressor;

class ClientStore : public ChannelStore
{
  public:

  ClientStore(StaticCompressor *compressor);

  virtual ~ClientStore();

  //
  // Get the store based on the index.
  //

  MessageStore *getRequestStore(unsigned char opcode) const
  {
    return requests_[opcode];
  }

  SplitStore *getSplitStore(int resource) const
  {
    return splits_[resource];
  }

  int getSplitTotalSize() const
  {
    return SplitStore::getTotalSize();
  }

  int getSplitTotalStorageSize() const
  {
    return SplitStore::getTotalStorageSize();
  }

  CommitStore *getCommitStore() const
  {
    return commits_;
  }

  int getCommitSize() const
  {
    return commits_ -> getSize();
  }

  void dumpSplitStore(int resource) const
  {
    splits_[resource] -> dump();
  }

  void dumpCommitStore() const
  {
    commits_ -> dump();
  }

  void dumpSplitStores() const;

  SplitStore *createSplitStore(int resource)
  {
    splits_[resource] = new SplitStore(compressor_, commits_, resource);

    return splits_[resource];
  }

  void destroySplitStore(int resource)
  {
    delete splits_[resource];

    splits_[resource] = NULL;
  }

  //
  // Actually save the message store
  // to disk according to proxy mode.
  //

  int saveRequestStores(ostream *cachefs, md5_state_t *md5StateStream,
                            md5_state_t *md5StateClient, T_checksum_action checksumAction,
                                T_data_action dataAction) const;

  int loadRequestStores(istream *cachefs, md5_state_t *md5StateStream,
                            T_checksum_action checksumAction, T_data_action dataAction) const;

  private:

  //
  // A client store contains requests.
  //

  MessageStore *requests_[CHANNEL_STORE_OPCODE_LIMIT];

  //
  // Client messages being split.
  //

  SplitStore *splits_[CHANNEL_STORE_RESOURCE_LIMIT];

  //
  // Messages having been recomposed.
  //

  CommitStore *commits_;

  //
  // Passed forward to the other stores.
  //

  StaticCompressor *compressor_;
};

#endif /* ClientStore_H */
