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

#ifndef Split_H
#define Split_H

#include "Types.h"
#include "Timestamp.h"
#include "Message.h"

//
// Set the verbosity level.
//

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

//
// Define this to know how many splits
// are allocated and deallocated.
//

#undef  REFERENCES

//
// Size of header of messages saved on
// disk.
//

#define SPLIT_HEADER_SIZE            12

//
// This class is used to divide big messages
// in smaller chunks and send them at idle
// time.
//

class EncodeBuffer;
class DecodeBuffer;

class SplitStore;
class CommitStore;

//
// Preferred message streaming policy.
//

typedef enum
{
  split_none = -1,
  split_async = 1,
  split_sync

} T_split_mode;

//
// Current state of the split. Used to
// implement the state machine.
//

typedef enum
{
  split_undefined = -1,
  split_added,
  split_missed,
  split_loaded,
  split_aborted,
  split_notified

} T_split_state;

class Split
{
  friend class SplitStore;
  friend class CommitStore;

  public:

  Split();

  ~Split();

  //
  // Note that, differently from the message
  // store, the split store doesn't account
  // for the data offset when dealing with
  // the data. This means that both the size_
  // and c_size members represent the actual
  // size of the data part.
  //

  void compressedSize(int size)
  {
    c_size_ = size;

    store_ -> validateSize(d_size_, c_size_);
  }

  int compressedSize()
  {
    return c_size_;
  }

  int plainSize()
  {
    return i_size_ + d_size_;
  }

  T_checksum getChecksum()
  {
    return checksum_;
  }

  MessageStore *getStore()
  {
    return store_;
  }
  
  T_split_state getState()
  {
    return state_;
  }

  T_store_action getAction()
  {
    return action_;
  }

  //
  // We may need to find the resource
  // associated to the split message
  // because old protocol version use
  // a single store for all splits.
  //

  int getResource()
  {
    return resource_;
  }

  int getRequest()
  {
    return store_ -> opcode();
  }

  int getPosition()
  {
    return position_;
  }

  T_split_mode getMode()
  {
    return mode_;
  }

  void setPolicy(int load, int save)
  {
    load_ = load;
    save_ = save;
  }

  void setState(T_split_state state)
  {
    state_ = state;
  }

  private:

  //
  // The agent's resource which is splitting
  // the message.
  //

  int resource_;

  //
  // Where to find the message in the message
  // store or the X sequence number of the
  // original request, in recent versions.
  //

  int position_;

  //
  // Which store is involved.
  //

  MessageStore *store_;

  //
  // Identity size of the message.
  //

  int i_size_;

  //
  // This is the uncompressed data size of the
  // original message.
  //

  int d_size_;

  //
  // This is the size of the compressed data,
  // if the data is stored in this form.
  //

  int c_size_;

  //
  // Size of the data buffer, as known by the
  // encoding side. This field is only used at
  // the decoding side. The remote size can be
  // different from the actual data size, if
  // the encoding side did not confirm that it
  // received the abort split event.
  //

  int r_size_;
  
  //
  // Position in the data buffer that will be
  // the target of the next send or receive
  // operation while streaming the message.
  //

  int next_;

  //
  // Load or save the split to disk.
  //

  int load_;
  int save_;

  //
  // Checksum of the original message.
  //

  T_checksum checksum_;

  //
  // Was this split confirmed or aborted?
  //

  T_split_state state_;

  //
  // What's the policy for sending this split?
  //

  T_split_mode mode_;

  //
  // Operation that had been performed on the
  // store at the time the split was added.
  //

  T_store_action action_;

  //
  // Container for the identity and data part
  // of the X message.
  //

  T_data identity_;
  T_data data_;

  #ifdef REFERENCES

  static int references_;

  #endif
};

class SplitStore
{
  public:

  SplitStore(StaticCompressor *compressor, CommitStore *commits, int resource);

  ~SplitStore();

  Split *getFirstSplit() const
  {
    if (splits_ -> size() > 0)
    {
      return (*(splits_ -> begin()));
    }

    return NULL;
  }

  Split *getLastSplit() const
  {
    if (splits_ -> size() > 0)
    {
      return (*(--(splits_ -> end())));
    }

    return NULL;
  }

  int getNodeSize(const Split *split) const
  {
    //
    // Take in account 64 bytes of overhead
    // for each node.
    //

    return (sizeof(class Split) + 64 +
                split -> i_size_ + split -> d_size_);
  }

  int getStorageSize()
  {
    return splitStorageSize_;
  }

  static int getTotalSize()
  {
    return totalSplitSize_;
  }

  static int getTotalStorageSize()
  {
    return totalSplitStorageSize_;
  }

  int getResource()
  {
    return resource_;
  }

  int getSize()
  {
    return splits_ -> size();
  }

  T_splits *getSplits()
  {
    return splits_;
  }

  //
  // Used, respectively, at the encoding
  // and decoding side.
  //

  Split *add(MessageStore *store, int resource, T_split_mode mode,
                 int position, T_store_action action, T_checksum checksum,
                     const unsigned char *buffer, const int size);

  Split *add(MessageStore *store, int resource, int position,
                T_store_action action, T_checksum checksum,
                     unsigned char *buffer, const int size);

  //
  // Handle the streaming of the message data.
  //

  int send(EncodeBuffer &encodeBuffer, int packetSize);

  int receive(DecodeBuffer &decodeBuffer);

  //
  // Remove the top element of the split store
  // and update the storage size.
  //

  void remove(Split *split);

  //
  // Load the message from disk and replace the
  // message in the store with the new copy.
  //

  int load(Split *split);

  //
  // Save the data to disk after the message has
  // been recomposed at the local side.
  //

  int save(Split *split);

  //
  // Find the message on disk and update the last
  // modification time. This is currently unused.
  //

  int find(Split *split);

  //
  // Remove the element on top of the queue and
  // discard any split data that still needs to
  // be transferred.
  //

  Split *pop();

  //
  // Dump the content of the store.
  //

  void dump();

  protected:

  //
  // Repository where to add the splits.
  //

  T_splits *splits_;

  //
  // Compress and decompress the data payload.
  //

  StaticCompressor *compressor_;

  private:

  int start(EncodeBuffer &encodeBuffer);

  int start(DecodeBuffer &decodeBuffer);

  void push(Split *split);

  //
  // Determine the name of the file object based
  // on the checksum.
  //

  const char *name(const T_checksum checksum);

  //
  // The number of elements and data bytes
  // in the repository.
  //

  int splitStorageSize_;

  static int totalSplitSize_;
  static int totalSplitStorageSize_;

  //
  // Current element being transferred.
  //

  T_splits::iterator current_;

  //
  // Repository where to move the splits
  // after they are completely recomposed.
  //

  CommitStore *commits_;

  //
  // Index in the client store or none,
  // if this is a commit store.
  //

  int resource_;

  #ifdef REFERENCES

  static int references_;

  #endif
};

class CommitStore : public SplitStore
{
  //
  // This is just a split store.
  //

  public:

  CommitStore(StaticCompressor *compressor)

    : SplitStore(compressor, NULL, nothing)
  {
  }

  //
  // Move identity and data of the split to the
  // provided buffer, uncompressing the message,
  // if needed.
  //

  int expand(Split *split, unsigned char *buffer, const int size);

  //
  // We recomposed the data part. If the message
  // was originally added to the message store,
  // replace the data and/or update the size.
  //

  int update(Split *split);

  //
  // Remove the split from the commit queue.
  //

  Split *pop();

  //
  // This is just used for debug. It checks
  // if any message in the message store has
  // an invalid number of locks.
  //

  int validate(Split *split);
};

#endif /* Split_H */
