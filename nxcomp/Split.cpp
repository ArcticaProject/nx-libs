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

#include <unistd.h>
#include <cstring>
#include <sys/stat.h>
#include <sys/types.h>
#include <utime.h>

#include "Misc.h"

#include "Split.h"

#include "Control.h"
#include "Statistics.h"

#include "EncodeBuffer.h"
#include "DecodeBuffer.h"

#include "StaticCompressor.h"

#include "Unpack.h"

//
// Set the verbosity level.
//

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG
#undef  DUMP

//
// Define this to trace elements
// allocated and deallocated.
//

#undef  REFERENCES

//
// Counters used for store control.
//

int SplitStore::totalSplitSize_;
int SplitStore::totalSplitStorageSize_;

//
// This is used for reference count.
//

#ifdef REFERENCES

int Split::references_ = 0;

#endif

Split::Split()
{
  resource_ = nothing;
  position_ = nothing;

  store_ = NULL;

  d_size_ = 0;
  i_size_ = 0;
  c_size_ = 0;
  r_size_ = 0;

  next_ = 0;
  load_ = 0;
  save_ = 0;

  checksum_ = NULL;
  state_    = split_undefined;
  mode_     = split_none;
  action_   = is_discarded;

  #ifdef REFERENCES

  references_++;

  *logofs << "Split: Created new Split at " 
          << this << " out of " << references_ 
          << " allocated references.\n" << logofs_flush;
  #endif
}

Split::~Split()
{
  delete [] checksum_;

  #ifdef REFERENCES

  references_--;

  *logofs << "Split: Deleted Split at " 
          << this << " out of " << references_ 
          << " allocated references.\n" << logofs_flush;
  #endif
}

SplitStore::SplitStore(StaticCompressor *compressor, CommitStore *commits, int resource)

  : compressor_(compressor), commits_(commits), resource_(resource)
{
  splits_ = new T_splits();

  current_ = splits_ -> end();

  splitStorageSize_ = 0;

  #ifdef TEST
  *logofs << "SplitStore: Created new store [";

  if (resource_ != nothing)
  {
    *logofs << resource_;
  }
  else
  {
    *logofs << "commit";
  }
  
  *logofs << "].\n" << logofs_flush;

  *logofs << "SplitStore: Total messages in stores are "
          << totalSplitSize_ << " with total storage size "
          << totalSplitStorageSize_ << ".\n"
          << logofs_flush;
  #endif
}

SplitStore::~SplitStore()
{
  totalSplitSize_ -= splits_ -> size();

  totalSplitStorageSize_ -= splitStorageSize_;

  for (T_splits::iterator i = splits_ -> begin();
           i != splits_ -> end(); i++)
  {
    delete *i;
  }

  delete splits_;

  #ifdef TEST
  *logofs << "SplitStore: Deleted store [";

  if (resource_ != nothing)
  {
    *logofs << resource_;
  }
  else
  {
    *logofs << "commit";
  }
  
  *logofs << "] with storage size " << splitStorageSize_
          << ".\n" << logofs_flush;

  *logofs << "SplitStore: Total messages in stores are "
          << totalSplitSize_ << " with total storage size "
          << totalSplitStorageSize_ << ".\n"
          << logofs_flush;
  #endif
}

//
// This is called at the encoding side.
//

Split *SplitStore::add(MessageStore *store, int resource, T_split_mode mode,
                           int position, T_store_action action, T_checksum checksum,
                               const unsigned char *buffer, const int size)
{
  #ifdef TEST
  *logofs << "SplitStore: Adding message [" << (unsigned int) store ->
             opcode() << "] resource " << resource << " mode " << mode
          << " position " << position << " action [" << DumpAction(action)
          << "] and checksum [" << DumpChecksum(checksum) << "]"
          << ".\n" << logofs_flush;
  #endif

  Split *split = new Split();

  if (split == NULL)
  {
    #ifdef PANIC
    *logofs << "SplitStore: PANIC! Can't allocate "
            << "memory for the split.\n"
            << logofs_flush;
    #endif

    cerr << "Error" << ": Can't allocate memory "
         << "for the split.\n";

    HandleAbort();
  }

  split -> store_    = store;
  split -> resource_ = resource;
  split -> mode_     = mode;
  split -> position_ = position;
  split -> action_   = action;

  split -> store_ -> validateSize(size);

  //
  // The checksum is not provided if the
  // message is cached.
  //

  if (checksum != NULL)
  {
    split -> checksum_ = new md5_byte_t[MD5_LENGTH];

    memcpy(split -> checksum_, checksum, MD5_LENGTH);
  }

  //
  // We don't need the identity data at the
  // encoding side. This qualifies the split
  // as a split generated at the encoding
  // side.
  //

  split -> i_size_ = store -> identitySize(buffer, size);

  split -> d_size_ = size - split -> i_size_;

  if (action == IS_ADDED || action == is_discarded)
  {
    //
    // If the message was added to message
    // store or discarded we need to save
    // the real data so we can transfer it
    // at later time.
    //

    split -> data_.resize(split -> d_size_);

    memcpy(split -> data_.begin(), buffer + split -> i_size_, split -> d_size_);

    //
    // If the message was added, lock it so
    // it will not be used by the encoding
    // side until it is recomposed.
    //

    if (action == IS_ADDED)
    {
      split -> store_ -> lock(split -> position_);

      #ifdef TEST

      commits_ -> validate(split);

      #endif
    }
  }
  #ifdef WARNING
  else
  {
    *logofs << "SplitStore: WARNING! Not copying data for the cached message.\n"
            << logofs_flush;
  }
  #endif

  push(split);

  return split;
}

//
// This is called at decoding side. If checksum
// is provided, the message can be searched on
// disk, then, if message is found, an event is
// sent to abort the data transfer.
//

Split *SplitStore::add(MessageStore *store, int resource, int position,
                           T_store_action action, T_checksum checksum,
                               unsigned char *buffer, const int size)
{
  #ifdef TEST
  *logofs << "SplitStore: Adding message ["
          << (unsigned int) store -> opcode() << "] resource "
          << resource << " position " << position << " action ["
          << DumpAction(action) << "] and checksum ["
          << DumpChecksum(checksum) << "].\n" << logofs_flush;
  #endif

  Split *split = new Split();

  if (split == NULL)
  {
    #ifdef PANIC
    *logofs << "SplitStore: PANIC! Can't allocate "
            << "memory for the split.\n"
            << logofs_flush;
    #endif

    cerr << "Error" << ": Can't allocate memory "
         << "for the split.\n";

    HandleAbort();
  }

  split -> store_    = store;
  split -> resource_ = resource;
  split -> position_ = position;
  split -> action_   = action;

  split -> store_ -> validateSize(size);

  //
  // Check if the checksum was provided
  // by the remote.
  //

  if (checksum != NULL)
  {
    split -> checksum_ = new md5_byte_t[MD5_LENGTH];

    memcpy(split -> checksum_, checksum, MD5_LENGTH);
  }

  split -> i_size_ = store -> identitySize(buffer, size);

  //
  // Copy the identity so we can expand the
  // message when it is committed.
  //

  split -> identity_.resize(split -> i_size_);

  memcpy(split -> identity_.begin(), buffer, split -> i_size_);

  split -> d_size_ = size - split -> i_size_;

  if (action == IS_ADDED || action == is_discarded)
  {
    //
    // The unpack procedure will check if the
    // first 2 bytes of the buffer contain the
    // pattern and will not try to expand the
    // image.
    //

    split -> data_.resize(2);

    unsigned char *data = split -> data_.begin();

    data[0] = SPLIT_PATTERN;
    data[1] = SPLIT_PATTERN;

    //
    // If the message was added to the store,
    // we don't have the data part, yet, so
    // we need to lock the message until it
    // is recomposed.
    //

    if (action == IS_ADDED)
    {
      split -> store_ -> lock(split -> position_);

      #ifdef TEST

      commits_ -> validate(split);

      #endif
    }
  }
  else
  {
    #ifdef WARNING
    *logofs << "SplitStore: WARNING! Copying data for the cached message.\n"
            << logofs_flush;
    #endif

    //
    // We may optionally take the data from the
    // message store in compressed form, but,
    // as the data has been decompressed in the
    // buffer, we save a further decompression.
    //

    split -> data_.resize(split -> d_size_);

    memcpy(split -> data_.begin(), buffer + split -> i_size_, split -> d_size_);
  }

  push(split);

  return split;
}

void SplitStore::push(Split *split)
{
  splits_ -> push_back(split);

  splitStorageSize_ += getNodeSize(split);

  totalSplitSize_++;

  totalSplitStorageSize_ += getNodeSize(split);

  statistics -> addSplit();

  #ifdef TEST
  *logofs << "SplitStore: There are " << splits_ -> size()
          << " messages in store [" << resource_ << "] with "
          << "storage size " << splitStorageSize_ << ".\n"
          << logofs_flush;

  *logofs << "SplitStore: Total messages in stores are "
          << totalSplitSize_ << " with total storage size "
          << totalSplitStorageSize_ << ".\n"
          << logofs_flush;
  #endif

  split -> state_ = split_added;
}

void SplitStore::dump()
{
  #ifdef DUMP

  int n;

  Split *split;

  *logofs << "SplitStore: DUMP! Dumping content of ";

  if (commits_ == NULL)
  {
    *logofs << "[commits]";
  }
  else
  {
    *logofs << "[splits] for store [" << resource_ << "]";
  }

  *logofs << " with [" << getSize() << "] elements "
          << "in the store.\n" << logofs_flush;

  n = 0;

  for (T_splits::iterator i = splits_ -> begin(); i != splits_ -> end(); i++, n++)
  {
    split = *i;

    *logofs << "SplitStore: DUMP! Split [" << n << "] has action ["
            << DumpAction(split -> action_) << "] state ["
            << DumpState(split -> state_) << "] ";

    if (split -> resource_ >= 0)
    {
      *logofs << "resource " << split -> resource_;
    }

    *logofs << " request " << (unsigned) split -> store_ -> opcode()
            << " position " << split -> position_ << " size is "
            << split -> data_.size() << " (" << split -> d_size_
            << "/" << split -> c_size_ << "/" << split -> r_size_
            << ") with " << split -> data_.size() - split -> next_
            << "] bytes to go.\n" << logofs_flush;
  }

  #endif
}

int SplitStore::send(EncodeBuffer &encodeBuffer, int packetSize)
{
  if (splits_ -> size() == 0)
  {
    #ifdef PANIC
    *logofs << "SplitStore: PANIC! Function send called with no splits available.\n" 
            << logofs_flush;
    #endif

    cerr << "Error" << ": Function send called with no splits available.\n";

    HandleAbort();
  }

  //
  // A start operation must always be executed on
  // the split, even in the case the split will be
  // later aborted.
  // 

  if (current_ == splits_ -> end())
  {
    start(encodeBuffer);
  }

  //
  // If we have matched the checksum received from
  // the remote side then we must abort the current
  // split, else we can send another block of data
  // to the remote peer.
  //

  Split *split = *current_;

  unsigned int abort = 0;

  if (split -> state_ == split_loaded)
  {
    abort = 1;
  }

  encodeBuffer.encodeBoolValue(abort);

  if (abort == 1)
  {
    #ifdef TEST
    *logofs << "SplitStore: Aborting split for checksum ["
            << DumpChecksum(split -> checksum_) << "] position "
            << split -> position_ << " with " << (split ->
               data_.size() - split -> next_) << " bytes to go "
            << "out of " << split -> data_.size()
            << ".\n" << logofs_flush;
    #endif

    statistics -> addSplitAborted();

    statistics -> addSplitAbortedBytesOut(split -> data_.size() - split -> next_);

    split -> next_ = split -> data_.size();

    split -> state_ = split_aborted;
  }
  else
  {
    int count = (packetSize <= 0 || split -> next_ +
                     packetSize > (int) split -> data_.size() ?
                         split -> data_.size() - split -> next_ : packetSize);

    #ifdef TEST
    *logofs << "SplitStore: Sending split for checksum ["
            << DumpChecksum(split -> checksum_) << "] count "
            << count << " position " << split -> position_
            << ". Data size is " << split -> data_.size() << " ("
            << split -> d_size_ << "/" << split -> c_size_ << "), "
            << split -> data_.size() - (split -> next_ + count)
            << " to go.\n" << logofs_flush;
    #endif

    encodeBuffer.encodeValue(count, 32, 10);

    encodeBuffer.encodeMemory(split -> data_.begin() + split -> next_, count);

    split -> next_ += count;
  }

  //
  // Was data completely transferred? We are the
  // sending side. We must update the message in
  // store, even if split was aborted.
  //

  if (split -> next_ != ((int) split -> data_.size()))
  {
    return 0;
  }

  //
  // Move the split at the head of the
  // list to the commits.
  //

  remove(split);

  //
  // Reset current position to the
  // end of repository.
  //

  current_ = splits_ -> end();

  #ifdef TEST
  *logofs << "SplitStore: Removed split at head of the list. "
          << "Resource is " << split -> resource_ << " request "
          << (unsigned) split -> store_ -> opcode() << " position "
          << split -> position_ << ".\n" << logofs_flush;
  #endif

  return 1;
}

int SplitStore::start(EncodeBuffer &encodeBuffer)
{
  //
  // Get the element at the top of the
  // list.
  //

  current_ = splits_ -> begin();

  Split *split = *current_;

  #ifdef TEST
  *logofs << "SplitStore: Starting split for checksum ["
          << DumpChecksum(split -> checksum_) << "] position "
          << split -> position_ << " with " << (split ->
             data_.size() - split -> next_) << " bytes to go "
          << "out of " << split -> data_.size()
          << ".\n" << logofs_flush;
  #endif

  //
  // See if compression of the data part is
  // enabled.
  //

  if (split -> store_ -> enableCompress)
  {
    //
    // If the split is going to be aborted don't
    // compress the data and go straight to the
    // send. The new data size will be assumed
    // from the disk cache.
    //

    if (split -> state_ != split_loaded)
    {
      unsigned int compressedSize = 0;
      unsigned char *compressedData = NULL;

      if (control -> LocalDataCompression &&
              (compressor_ -> compressBuffer(split -> data_.begin(), split -> d_size_,
                                                 compressedData, compressedSize)))
      {
        //
        // Replace the data with the one in
        // compressed form.
        //

        #ifdef TEST
        *logofs << "SplitStore: Split data of size " << split -> d_size_
                << " has been compressed to " << compressedSize 
                << " bytes.\n" << logofs_flush;
        #endif

        split -> data_.clear();

        split -> data_.resize(compressedSize);

        memcpy(split -> data_.begin(), compressedData, compressedSize);

        split -> c_size_ = compressedSize;

        //
        // Inform our peer that the data is
        // compressed and send the new size.
        //

        encodeBuffer.encodeBoolValue(1);

        encodeBuffer.encodeValue(compressedSize, 32, 14);

        #ifdef TEST
        *logofs << "SplitStore: Signaled " << split -> c_size_
                << " bytes of compressed data for this message.\n"
                << logofs_flush;
        #endif

        return 1;
      }
    }
    #ifdef TEST
    else
    {
      *logofs << "SplitStore: Not trying to compress the "
              << "loaded message.\n" << logofs_flush;
    }
    #endif

    //
    // Tell to the remote that data will
    // follow uncompressed.
    //

    encodeBuffer.encodeBoolValue(0);
  }

  return 1;
}

int SplitStore::start(DecodeBuffer &decodeBuffer)
{
  #ifdef TEST
  *logofs << "SplitStore: Going to receive a new split from the remote side.\n"
          << logofs_flush;
  #endif

  //
  // Get the element at the head
  // of the list.
  //

  current_ = splits_ -> begin();

  Split *split = *current_;

  unsigned int compressedSize = 0;

  //
  // Save the data size known by the remote.
  // This information will be needed if the
  // remote will not have a chance to abort
  // the split.
  //

  split -> r_size_ = split -> d_size_;

  //
  // Find out if data was compressed by the
  // remote.
  //

  if (split -> store_ -> enableCompress)
  {
    decodeBuffer.decodeBoolValue(compressedSize);

    if (compressedSize == 1)
    {
      //
      // Get the compressed size.
      //

      if (control -> isProtoStep7() == 1)
      {
        decodeBuffer.decodeValue(compressedSize, 32, 14);
      }
      else
      {
        //
        // As we can't refuse to handle the decoding
        // of the split message when connected to an
        // old proxy version, we need to decode this
        // in a way that is compatible.
        //

        unsigned int diffSize;

        decodeBuffer.decodeValue(diffSize, 32, 14);

        split -> store_ -> lastResize += diffSize;

        compressedSize = split -> store_ -> lastResize;
      }

      split -> store_ -> validateSize(split -> d_size_, compressedSize);

      split -> r_size_ = compressedSize;
    }
  }

  //
  // Update the size if the split
  // was not already loaded.
  //

  if (split -> state_ != split_loaded)
  {
    split -> data_.clear();

    if (compressedSize > 0)
    {
      split -> c_size_ = compressedSize;

      #ifdef TEST
      *logofs << "SplitStore: Split data of size "
              << split -> d_size_ << " was compressed to "
              << split -> c_size_ << " bytes.\n"
              << logofs_flush;
      #endif

      split -> data_.resize(split -> c_size_);
    }
    else
    {
      split -> data_.resize(split -> d_size_);
    }

    unsigned char *data = split -> data_.begin();

    data[0] = SPLIT_PATTERN;
    data[1] = SPLIT_PATTERN;
  }
  #ifdef TEST
  else
  {
    //
    // The message had been already
    // loaded from disk.
    //

    if (compressedSize > 0)
    {
      if ((int) compressedSize != split -> c_size_)
      {
        *logofs << "SplitStore: WARNING! Compressed data size is "
                << "different than the loaded compressed size.\n"
                << logofs_flush;
      }

      *logofs << "SplitStore: Ignoring the new size with "
              << "loaded compressed size " << split -> c_size_
              << ".\n" << logofs_flush;
    }
  }
  #endif

  return 1;
}

int SplitStore::receive(DecodeBuffer &decodeBuffer)
{
  if (splits_ -> size() == 0)
  {
    #ifdef PANIC
    *logofs << "SplitStore: PANIC! Function receive called with no splits available.\n" 
            << logofs_flush;
    #endif

    cerr << "Error" << ": Function receive called with no splits available.\n";

    HandleAbort();
  }

  if (current_ == splits_ -> end())
  {
    start(decodeBuffer);
  }

  //
  // Check first if split was aborted, else add
  // any new data to message being recomposed.
  //

  Split *split = *current_;

  unsigned int abort = 0;

  decodeBuffer.decodeBoolValue(abort);

  if (abort == 1)
  {
    #ifdef TEST
    *logofs << "SplitStore: Aborting split for checksum ["
            << DumpChecksum(split -> checksum_) << "] position "
            << split -> position_ << " with " << (split ->
               data_.size() - split -> next_) << " bytes to go "
            << "out of " << split -> data_.size()
            << ".\n" << logofs_flush;
    #endif

    statistics -> addSplitAborted();

    statistics -> addSplitAbortedBytesOut(split -> r_size_ - split -> next_);

    split -> next_ = split -> r_size_;

    split -> state_ = split_aborted;
  }
  else
  {
    //
    // Get the size of the packet.
    //

    unsigned int count;

    decodeBuffer.decodeValue(count, 32, 10);

    //
    // If the split was not already loaded from
    // disk, decode the packet and update our
    // copy of the data. The encoding side may
    // have not received the abort event, yet,
    // and may be unaware that the message is
    // stored in compressed form at our side.
    //

    #ifdef TEST
    *logofs << "SplitStore: Receiving split for checksum ["
            << DumpChecksum(split -> checksum_) << "] count "
            << count << " position " << split -> position_
            << ". Data size is " << split -> data_.size() << " ("
            << split -> d_size_ << "/" << split -> c_size_ << "/"
            << split -> r_size_ << "), " << split -> r_size_ -
               (split -> next_ + count) << " to go.\n"
            << logofs_flush;
    #endif

    if (split -> next_ + count > (unsigned) split -> r_size_)
    {
      #ifdef PANIC
      *logofs << "SplitStore: PANIC! Invalid data count "
              << count << "provided in the split.\n"
              << logofs_flush;

      *logofs << "SplitStore: PANIC! While receiving split for "
              << "checksum [" << DumpChecksum(split -> checksum_)
              << "] with count " << count << " action ["
              << DumpAction(split -> action_) << "] state ["
              << DumpState(split -> state_) << "]. Data size is "
              << split -> data_.size() << " (" << split -> d_size_
              << "/" << split -> c_size_ << "), " << split ->
                 data_.size() - (split -> next_ + count)
              << " to go.\n" << logofs_flush;
      #endif

      cerr << "Error" << ": Invalid data count "
           << count << "provided in the split.\n";

      HandleAbort();
    }

    if (split -> state_ != split_loaded)
    {
      #ifdef TEST

      if (split -> next_ + count > split -> data_.size())
      {
        #ifdef PANIC
        *logofs << "SplitStore: PANIC! Inconsistent split data size "
                << split -> data_.size() << " with expected size "
                << split -> r_size_ << ".\n"
               << logofs_flush;
        #endif

        HandleAbort();
      }

      #endif

      memcpy(split -> data_.begin() + split -> next_, 
                 decodeBuffer.decodeMemory(count), count);
    }
    else
    {
      #ifdef TEST
      *logofs << "SplitStore: WARNING! Data discarded with split "
              << "loaded from disk.\n" << logofs_flush;
      #endif

      decodeBuffer.decodeMemory(count);
    }

    split -> next_ += count;
  }

  //
  // Is unsplit complete?
  //

  if (split -> next_ != split -> r_size_)
  {
    return 0;
  }

  //
  // If the persistent cache is enabled,
  // we have a valid checksum and the
  // split was not originally retrieved
  // from disk, save the message on disk.
  //

  if (split -> state_ != split_loaded &&
          split -> state_ != split_aborted)
  {
    save(split);
  }

  //
  // Move the split at the head of the
  // list to the commits.
  //

  remove(split);

  //
  // Reset the current position to the
  // end of the repository.
  //

  current_ = splits_ -> end();

  #ifdef TEST
  *logofs << "SplitStore: Removed split at head of the list. "
          << "Resource is " << split -> resource_ << " request "
          << (unsigned) split -> store_ -> opcode() << " position "
          << split -> position_ << ".\n" << logofs_flush;
  #endif

  return 1;
}

Split *SplitStore::pop()
{
  if (splits_ -> size() == 0)
  {
    #ifdef TEST
    *logofs << "SplitStore: The split store is empty.\n"
            << logofs_flush;
    #endif

    return NULL;
  }

  //
  // Move the pointer at the end of the list.
  // The next send operation will eventually
  // start a new split.
  // 

  current_ = splits_ -> end();

  Split *split = *(splits_ -> begin());

  splits_ -> pop_front();

  #ifdef TEST
  *logofs << "SplitStore: Removed split at the head of the "
          << "list with resource " << split -> resource_
          << " request " << (unsigned) split -> store_ ->
             opcode() << " position " << split -> position_
          << ".\n" << logofs_flush;
  #endif

  splitStorageSize_ -= getNodeSize(split);

  totalSplitSize_--;

  totalSplitStorageSize_ -= getNodeSize(split);

  #ifdef TEST
  *logofs << "SplitStore: There are " << splits_ -> size()
          << " messages in store [" << resource_ << "] with "
          << "storage size " << splitStorageSize_ << ".\n"
          << logofs_flush;

  *logofs << "SplitStore: Total messages in stores are "
          << totalSplitSize_ << " with total storage size "
          << totalSplitStorageSize_ << ".\n"
          << logofs_flush;
  #endif

  return split;
}

void SplitStore::remove(Split *split)
{
  #ifdef TEST
  *logofs << "SplitStore: Going to remove the split from the list.\n"
          << logofs_flush;
  #endif

  #ifdef TEST

  if (split != getFirstSplit())
  {
    #ifdef PANIC
    *logofs << "SplitStore: PANIC! Trying to remove a split "
            << "not at the head of the list.\n"
            << logofs_flush;
    #endif

    cerr << "Error" << ": Trying to remove a split "
         << "not at the head of the list.\n";

    HandleAbort();
  }

  #endif

  //
  // Move the split to the commit store.
  //

  splits_ -> pop_front();

  commits_ -> splits_ -> push_back(split);

  splitStorageSize_ -= getNodeSize(split);

  totalSplitSize_--;

  totalSplitStorageSize_ -= getNodeSize(split);

  #ifdef TEST
  *logofs << "SplitStore: There are " << splits_ -> size()
          << " messages in store [" << resource_ << "] with "
          << "storage size " << splitStorageSize_ << ".\n"
          << logofs_flush;

  *logofs << "SplitStore: Total messages in stores are "
          << totalSplitSize_ << " with total storage size "
          << totalSplitStorageSize_ << ".\n"
          << logofs_flush;
  #endif

  #ifdef TEST

  if (splits_ -> size() == 0)
  {
    if (splitStorageSize_ != 0)
    {
      #ifdef PANIC
      *logofs << "SplitStore: PANIC! Internal error calculating "
              << "split data size. It is " << splitStorageSize_ 
              << " while should be 0.\n" << logofs_flush;
      #endif

      cerr << "Error" << ": Internal error calculating "
           << "split data size. It is " << splitStorageSize_
           << " while should be 0.\n";

      HandleAbort();
    }
  }

  #endif
}

const char *SplitStore::name(const T_checksum checksum)
{
  if (checksum == NULL)
  {
    return NULL;
  }

  char *pathName = control -> ImageCachePath;

  if (pathName == NULL)
  {
    #ifdef PANIC
    *logofs << "SplitStore: PANIC! Cannot determine directory of "
            << "NX image files.\n" << logofs_flush;
    #endif

    return NULL;
  }

  int pathSize = strlen(pathName);

  //
  // File name is "[path][/I-c/I-][checksum][\0]",
  // where c is the first hex digit of checksum.
  //

  int nameSize = pathSize + 7 + MD5_LENGTH * 2 + 1;

  char *fileName = new char[nameSize];

  if (fileName == NULL)
  {
    #ifdef PANIC
    *logofs << "SplitStore: PANIC! Cannot allocate space for "
            << "NX image file name.\n" << logofs_flush;
    #endif

    return NULL;
  }

  strcpy(fileName, pathName);

  sprintf(fileName + pathSize, "/I-%1X/I-",
              *((unsigned char *) checksum) >> 4);

  for (unsigned int i = 0; i < MD5_LENGTH; i++)
  {
    sprintf(fileName + pathSize + 7 + (i * 2), "%02X",
                ((unsigned char *) checksum)[i]);
  }

  return fileName;
}

int SplitStore::save(Split *split)
{
  //
  // Check if saving the message on the
  // persistent cache is enabled.
  //

  if (split -> save_ == 0)
  {
    return 0;
  }

  T_checksum checksum = split -> checksum_;

  const char *fileName = name(checksum);

  if (fileName == NULL)
  {
    return 0;
  }

  unsigned int splitSize;

  ostream *fileStream = NULL;

  unsigned char *fileHeader = NULL;

  //
  // Get the other data from the split.
  //

  unsigned char opcode = split -> store_ -> opcode();

  unsigned char *data = split -> data_.begin();

  int dataSize = split -> d_size_;
  int compressedSize = split -> c_size_;

  #ifdef DEBUG
  *logofs << "SplitStore: Going to save split OPCODE#"
          << (unsigned int) opcode << " to file '" << fileName
          << "' with size " << dataSize << " and compressed size "
          << compressedSize << ".\n" << logofs_flush;
  #endif

  DisableSignals();

  //
  // Change the mask to make the file only
  // readable by the user, then restore the
  // old mask.
  //

  mode_t fileMode;

  //
  // Check if the file already exists. We try to
  // load the message when the split is started
  // and save it only if it is not found. Still
  // the remote side may send the same image mul-
  // tiple time and we may not have the time to
  // notify the abort.
  //

  struct stat fileStat;

  if (stat(fileName, &fileStat) == 0)
  {
    #ifdef TEST
    *logofs << "SplitStore: Image file '" << fileName
            << "' already present on disk.\n"
            << logofs_flush;
    #endif

    goto SplitStoreSaveError;
  }

  fileMode = umask(0077);

  fileStream = new ofstream(fileName, ios::out | ios::binary);

  umask(fileMode);

  if (CheckData(fileStream) < 0)
  {
    #ifdef PANIC
    *logofs << "SplitStore: PANIC! Cannot open file '" << fileName
            << "' for output.\n" << logofs_flush;
    #endif

    goto SplitStoreSaveError;
  }

  fileHeader = new unsigned char[SPLIT_HEADER_SIZE];

  if (fileHeader == NULL)
  {
    #ifdef PANIC
    *logofs << "SplitStore: PANIC! Cannot allocate space for "
            << "NX image header.\n" << logofs_flush;
    #endif

    goto SplitStoreSaveError;
  }

  //
  // Leave 3 bytes for future use. Please note
  // that, on some CPUs, we can't use PutULONG()
  // to write integers that are not aligned to
  // the word boundary.
  //

  *fileHeader = opcode;

  *(fileHeader + 1) = 0;
  *(fileHeader + 2) = 0;
  *(fileHeader + 3) = 0;

  PutULONG(dataSize, fileHeader + 4, false);
  PutULONG(compressedSize, fileHeader + 8, false);

  splitSize = (compressedSize > 0 ? compressedSize : dataSize);

  if (PutData(fileStream, fileHeader, SPLIT_HEADER_SIZE) < 0 ||
          PutData(fileStream, data, splitSize) < 0)
  {
    #ifdef PANIC
    *logofs << "SplitStore: PANIC! Cannot write to NX "
            << "image file '" << fileName << "'.\n"
            << logofs_flush;
    #endif

    goto SplitStoreSaveError;
  }

  //
  // Check if all the data was written on the
  // disk and, if not, remove the faulty copy.
  //

  FlushData(fileStream);

  if (CheckData(fileStream) < 0)
  {
    #ifdef PANIC
    *logofs << "SplitStore: PANIC! Failed to write NX "
            << "image file '" << fileName << "'.\n"
            << logofs_flush;
    #endif

    cerr << "Warning" << ": Failed to write NX "
         << "image file '" << fileName << "'.\n";

    goto SplitStoreSaveError;
  }

  #ifdef TEST
  *logofs << "SplitStore: Saved split to file '" << fileName
          << "' with data size " << dataSize << " and "
          << "compressed data size " << compressedSize
          << ".\n" << logofs_flush;
  #endif

  delete fileStream;

  delete [] fileName;
  delete [] fileHeader;

  EnableSignals();

  //
  // Update the timestamp as the operation
  // may have taken some time.
  //

  getNewTimestamp();

  return 1;

SplitStoreSaveError:

  delete fileStream;

  if (fileName != NULL)
  {
    unlink(fileName);
  }

  delete [] fileName;
  delete [] fileHeader;

  EnableSignals();

  return -1;
}

int SplitStore::find(Split *split)
{
  const char *fileName = name(split -> checksum_);

  if (fileName == NULL)
  {
    return 0;
  }

  #ifdef DEBUG
  *logofs << "SplitStore: Going to find split OPCODE#"
          << (unsigned) split -> store_ -> opcode()
          << " in file '" << fileName << "'.\n"
          << logofs_flush;
  #endif

  //
  // Check if the file exists and, at the
  // same time, update the modification
  // time to prevent its deletion.
  //

  if (utime(fileName, NULL) == 0)
  {
    #ifdef TEST
    *logofs << "SplitStore: Found split OPCODE#"
            << (unsigned) split -> store_ -> opcode()
            << " in file '" << fileName << "'.\n"
            << logofs_flush;
    #endif

    delete [] fileName;

    return 1;
  }

  #ifdef TEST
  *logofs << "SplitStore: WARNING! Can't find split "
          << "OPCODE#" << (unsigned) split -> store_ ->
             opcode() << " in file '" << fileName
          << "'.\n" << logofs_flush;
  #endif

  delete [] fileName;

  return 0;
}

int SplitStore::load(Split *split)
{
  //
  // Check if loading the image is enabled.
  //

  if (split -> load_ == 0)
  {
    return 0;
  }

  const char *fileName = name(split -> checksum_);

  if (fileName == NULL)
  {
    return 0;
  }

  unsigned char fileOpcode;

  int fileSize;
  int fileCSize;

  istream *fileStream = NULL;

  unsigned char *fileHeader = NULL;

  DisableSignals();

  #ifdef DEBUG
  *logofs << "SplitStore: Going to load split OPCODE#"
          << (unsigned int) split -> store_ -> opcode()
          << " from file '" << fileName << "'.\n"
          << logofs_flush;
  #endif

  fileStream = new ifstream(fileName, ios::in | ios::binary);

  if (CheckData(fileStream) < 0)
  {
    #ifdef TEST
    *logofs << "SplitStore: WARNING! Can't open image file '"
            << fileName  << "' on disk.\n" << logofs_flush;
    #endif

    goto SplitStoreLoadError;
  }

  fileHeader = new unsigned char[SPLIT_HEADER_SIZE];

  if (fileHeader == NULL)
  {
    #ifdef PANIC
    *logofs << "SplitStore: PANIC! Cannot allocate space for "
            << "NX image header.\n" << logofs_flush;
    #endif

    cerr << "Error" << ": Cannot allocate space for "
         << "NX image header.\n";

    goto SplitStoreLoadError;
  }

  if (GetData(fileStream, fileHeader, SPLIT_HEADER_SIZE) < 0)
  {
    #ifdef PANIC
    *logofs << "SplitStore: PANIC! Cannot read header from "
            << "NX image file '" << fileName << "'.\n"
            << logofs_flush;
    #endif

    cerr << "Warning" << ": Cannot read header from "
         << "NX image file '" << fileName << "'.\n";

    goto SplitStoreLoadError;
  }

  fileOpcode = *fileHeader;

  fileSize  = GetULONG(fileHeader + 4, false);
  fileCSize = GetULONG(fileHeader + 8, false);

  //
  // Don't complain if we find that data was saved
  // in compressed form even if we were not aware
  // of the compressed data size. The remote side
  // compresses the data only at the time it starts
  // the transferral of the split. We replace our
  // copy of the data with whatever we find on the
  // disk.
  //

  if (fileOpcode != split -> store_ -> opcode() ||
          fileSize != split -> d_size_ ||
              fileSize > control -> MaximumRequestSize ||
                  fileCSize > control -> MaximumRequestSize)

  {
    #ifdef TEST
    *logofs << "SplitStore: PANIC! Corrupted image file '" << fileName
            << "'. Expected " << (unsigned int) split -> store_ -> opcode()
            << "/" << split -> d_size_ << "/" << split -> c_size_ << " found "
            << (unsigned int) fileOpcode << "/" << fileSize << "/"
            << fileCSize << ".\n" << logofs_flush;
    #endif

    cerr << "Warning" << ": Corrupted image file '" << fileName
         << "'. Expected " << (unsigned int) split -> store_ -> opcode()
         << "/" << split -> d_size_ << "/" << split -> c_size_ << " found "
         << (unsigned int) fileOpcode << "/" << fileSize << "/"
         << fileCSize << ".\n";

    goto SplitStoreLoadError;
  }

  //
  // Update the data size with the size
  // we got from the disk record.
  //

  split -> d_size_ = fileSize;
  split -> c_size_ = fileCSize;

  unsigned int splitSize;

  if (fileCSize > 0)
  {
    splitSize = fileCSize;
  }
  else
  {
    splitSize = fileSize;
  }

  //
  // Allocate a new buffer if we didn't
  // do that already or if the size is
  // different.
  //

  if (split -> data_.size() != splitSize)
  {
    split -> data_.clear();

    split -> data_.resize(splitSize);
  }

  if (GetData(fileStream, split -> data_.begin(), splitSize) < 0)
  {
    #ifdef PANIC
    *logofs << "SplitStore: PANIC! Cannot read data from "
            << "NX image file '" << fileName << "'.\n"
            << logofs_flush;
    #endif

    cerr << "Warning" << ": Cannot read data from "
         << "NX image file '" << fileName << "'.\n";

    goto SplitStoreLoadError;
  }

  delete fileStream;

  delete [] fileHeader;
  delete [] fileName;

  EnableSignals();

  //
  // Update the timestamp as the operation
  // may have taken some time.
  //

  getNewTimestamp();

  return 1;

SplitStoreLoadError:

  delete fileStream;

  unlink(fileName);

  delete [] fileName;
  delete [] fileHeader;

  EnableSignals();

  return -1;
}

Split *CommitStore::pop()
{
  if (splits_ -> size() == 0)
  {
    #ifdef TEST
    *logofs << "CommitStore: The commit store is empty.\n"
            << logofs_flush;
    #endif

    return NULL;
  }

  Split *split = *(splits_ -> begin());

  splits_ -> pop_front();

  #ifdef TEST
  *logofs << "CommitStore: Removed commit split at the head "
          << "of the list with resource " << split -> resource_
          << " request " << (unsigned) split -> store_ ->
             opcode() << " position " << split -> position_
          << ".\n" << logofs_flush;
  #endif

  return split;
}

int CommitStore::expand(Split *split, unsigned char *buffer, const int size)
{
  #ifdef TEST
  *logofs << "CommitStore: Expanding split data with "
          << size << " bytes to write.\n"
          << logofs_flush;
  #endif

  #ifdef TEST

  if (size < split -> i_size_ + split -> d_size_)
  {
    #ifdef PANIC
    *logofs << "CommitStore: PANIC! Wrong size of the provided "
            << "buffer. It should be " << split -> i_size_ +
               split -> d_size_ << " instead of " << size
            << ".\n" << logofs_flush;
    #endif

    cerr << "Error" << ": Wrong size of the provided "
         << "buffer. It should be " << split -> i_size_ +
            split -> d_size_ << " instead of " << size
         << ".\n";

    HandleAbort();
  }

  #endif

  #ifdef DEBUG
  *logofs << "CommitStore: Copying " << split -> i_size_
          << " bytes of identity.\n" << logofs_flush;
  #endif

  memcpy(buffer, split -> identity_.begin(), split -> i_size_);

  //
  // Copy data, if any, to the buffer.
  //

  if (size > split -> i_size_)
  {
    //
    // Check if message has been stored
    // in compressed format.
    //

    if (split -> c_size_ == 0)
    {
      #ifdef DEBUG
      *logofs << "CommitStore: Copying " << split -> d_size_
              << " bytes of plain data.\n" << logofs_flush;
      #endif

      memcpy(buffer + split -> i_size_, split -> data_.begin(), split -> d_size_);
    }
    else
    {
      #ifdef DEBUG
      *logofs << "CommitStore: Decompressing " << split -> c_size_
              << " bytes and copying " << split -> d_size_
              << " bytes of data.\n" << logofs_flush;
      #endif

      if (compressor_ ->
              decompressBuffer(buffer + split -> i_size_,
                                   split -> d_size_, split -> data_.begin(),
                                       split -> c_size_) < 0)
      {
        #ifdef PANIC
        *logofs << "CommitStore: PANIC! Split data decompression failed.\n"
                << logofs_flush;
        #endif

        cerr << "Error" << ": Split data decompression failed.\n";

        return -1;
      }
    }
  }

  return 1;
}

int CommitStore::update(Split *split)
{
  if (split -> action_ != IS_ADDED)
  {
    return 0;
  }

  //
  // We don't need the identity data at
  // the encoding side.
  //

  if (split -> identity_.size() == 0)
  {
    #ifdef TEST
    *logofs << "SplitStore: Going to update the size "
            << "for object at position " << split -> position_
            << " with data size " << split -> d_size_
            << " and compressed data size " << split ->
               c_size_ << ".\n" << logofs_flush;
    #endif

    split -> store_ -> updateData(split -> position_, split -> d_size_,
                                      split -> c_size_);
  }
  else
  {
    #ifdef TEST
    *logofs << "SplitStore: Going to update data and size "
            << "for object at position " << split -> position_
            << " with data size " << split -> d_size_
            << " and compressed data size " << split ->
               c_size_ << ".\n" << logofs_flush;
    #endif

    split -> store_ -> updateData(split -> position_, split -> data_.begin(), 
                                      split -> d_size_, split -> c_size_);
  }

  //
  // Unlock message so that we can remove
  // or save it on disk at shutdown.
  //

  if (split -> action_ == IS_ADDED)
  {
    split -> store_ -> unlock(split -> position_);

    #ifdef TEST

    validate(split);

    #endif
  }

  return 1;
}

int CommitStore::validate(Split *split)
{
  MessageStore *store = split -> store_;

  int p, n, s;

  s = store -> cacheSlots;

  for (p = 0, n = 0; p < s; p++)
  {
    if (store -> getLocks(p) == 1)
    {
      n++;
    }
    else if (store -> getLocks(p) != 0)
    {
      #ifdef PANIC
      *logofs << "CommitStore: PANIC! Repository for OPCODE#"
              << (unsigned int) store -> opcode() << " has "
              << store -> getLocks(p) << " locks for message "
              << "at position " << p << ".\n" << logofs_flush;
      #endif

      cerr << "Error" << ": Repository for OPCODE#"
           << (unsigned int) store -> opcode() << " has "
           << store -> getLocks(p) << " locks for message "
           << "at position " << p << ".\n";

      HandleAbort();
    }
  }

  #ifdef TEST
  *logofs << "CommitStore: Repository for OPCODE#"
          << (unsigned int) store -> opcode()
          << " has " << n << " locked messages.\n"
          << logofs_flush;
  #endif

  return 1;
}
