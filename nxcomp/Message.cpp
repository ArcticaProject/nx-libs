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

#include <cstdio>
#include <unistd.h>
#include <cstring>

#include <algorithm>

#include "Misc.h"

//
// We need channel's cache data.
//

#include "Message.h"

#include "EncodeBuffer.h"
#include "DecodeBuffer.h"

//
// Set the verbosity level. You also
// need to define DUMP in Misc.cpp
// if DUMP is defined here.
//

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG
#undef  DUMP

//
// Define this to log when messages
// are allocated and deallocated.
//

#undef  REFERENCES

//
// Keep track of how many bytes are
// occupied by cache.
//

int MessageStore::totalLocalStorageSize_  = 0;
int MessageStore::totalRemoteStorageSize_ = 0;

//
// These are used for reference count.
//

#ifdef REFERENCES

int Message::references_        = 0;
int MessageStore::references_   = 0;

#endif

//
// Here are the methods to handle cached messages.
//

MessageStore::MessageStore(StaticCompressor *compressor)

  : compressor_(compressor)
{
  //
  // Public members.
  //

  enableCache    = MESSAGE_ENABLE_CACHE;
  enableData     = MESSAGE_ENABLE_DATA;
  enableSplit    = MESSAGE_ENABLE_SPLIT;
  enableCompress = MESSAGE_ENABLE_COMPRESS;

  dataLimit  = MESSAGE_DATA_LIMIT;
  dataOffset = MESSAGE_DATA_OFFSET;

  cacheSlots          = MESSAGE_CACHE_SLOTS;
  cacheThreshold      = MESSAGE_CACHE_THRESHOLD;
  cacheLowerThreshold = MESSAGE_CACHE_LOWER_THRESHOLD;

  #ifdef TEST
  *logofs << "MessageStore: Static compressor is at "
          << compressor_ << ".\n" << logofs_flush;
  #endif

  md5_state_ = new md5_state_t();

  #ifdef DEBUG
  *logofs << "MessageStore: Created MD5 state for object at " 
          << this << ".\n" << logofs_flush;
  #endif

  lastAdded   = cacheSlots;
  lastHit     = 0;
  lastRemoved = 0;
  lastRated   = nothing;
  lastAction  = is_discarded;

  //
  // This is used only for compatibility
  // with older proxies.
  //

  if (control -> isProtoStep7() == 1)
  {
    lastResize = -1;
  }
  else
  {
    lastResize = 0;
  }

  //
  // Private members.
  //

  localStorageSize_  = 0;
  remoteStorageSize_ = 0;

  #ifdef TEST
  *logofs << "MessageStore: Size of total cache is " 
          << totalLocalStorageSize_ << " bytes at local side and " 
          << totalRemoteStorageSize_ << " bytes at remote side.\n" 
          << logofs_flush;
  #endif

  messages_  = new T_messages();
  checksums_ = new T_checksums();

  temporary_ = NULL;

  #ifdef REFERENCES

  references_++;

  *logofs << "MessageStore: Created new store at " 
          << this << "out of " << references_
          << " allocated stores.\n" << logofs_flush;

  #endif
}

MessageStore::~MessageStore()
{
  //
  // The virtual destructor of specialized class
  // must get rid of both messages in container
  // and temporary.
  //

  #ifdef DEBUG
  *logofs << "MessageStore: Deleting MD5 state for object at "
          << this << ".\n" << logofs_flush;
  #endif

  delete md5_state_;

  delete messages_;
  delete checksums_;

  //
  // Update the static members tracking
  // size of total memory allocated for
  // all stores.
  //

  totalLocalStorageSize_  -= localStorageSize_;
  totalRemoteStorageSize_ -= remoteStorageSize_;

  #ifdef TEST
  *logofs << "MessageStore: Size of total cache is " 
          << totalLocalStorageSize_ << " bytes at local side and " 
          << totalRemoteStorageSize_ << " bytes at remote side.\n" 
          << logofs_flush;
  #endif

  #ifdef REFERENCES

  references_--;

  *logofs << "MessageStore: Deleted store at " 
          << this << " out of " << references_
          << " allocated stores.\n" << logofs_flush;

  #endif
}

//
// Here are the methods to parse and cache
// messages in the message stores.
//

int MessageStore::parse(Message *message, int split, const unsigned char *buffer,
                            unsigned int size, T_checksum_action checksumAction,
                                T_data_action dataAction, int bigEndian)
{
  //
  // Save the message size as received on the link.
  // This information will be used to create an ap-
  // propriate buffer at the time the message will
  // be unparsed.
  //

  message -> size_   = size;
  message -> i_size_ = identitySize(buffer, size);
  message -> c_size_ = 0;

  validateSize(size);

  if (checksumAction == use_checksum)
  {
    beginChecksum(message);

    parseIdentity(message, buffer, size, bigEndian);

    identityChecksum(message, buffer, size, bigEndian);

    parseData(message, split, buffer, size, checksumAction, dataAction, bigEndian);

    endChecksum(message);
  }
  else
  {
    parseIdentity(message, buffer, size, bigEndian);

    parseData(message, split, buffer, size, checksumAction, dataAction, bigEndian);
  }

  return 1;
}

int MessageStore::parse(Message *message, const unsigned char *buffer,
                            unsigned int size, const unsigned char *compressedData,
                                const unsigned int compressedDataSize,
                                    T_checksum_action checksumAction,
                                        T_data_action dataAction, int bigEndian)
{
  int offset = identitySize(buffer, size);

  message -> size_   = size;
  message -> i_size_ = offset;
  message -> c_size_ = compressedDataSize + offset;

  validateSize(message -> size_ - offset, compressedDataSize);

  if (checksumAction == use_checksum)
  {
    beginChecksum(message);

    parseIdentity(message, buffer, size, bigEndian);

    identityChecksum(message, buffer, size, bigEndian);

    parseData(message, buffer, size, compressedData, compressedDataSize,
                  checksumAction, dataAction, bigEndian);

    endChecksum(message);
  }
  else
  {
    parseIdentity(message, buffer, size, bigEndian);

    parseData(message, buffer, size, compressedData, compressedDataSize,
                  checksumAction, dataAction, bigEndian);
  }

  return 1;
}

int MessageStore::parseData(Message *message, int split, const unsigned char *buffer,
                                unsigned int size, T_checksum_action checksumAction,
                                    T_data_action dataAction, int bigEndian)
{
  if ((int) size > message -> i_size_)
  {
    unsigned int dataSize = size - message -> i_size_;

    if (checksumAction == use_checksum)
    {
      #ifdef DEBUG
      *logofs << name() << ": Calculating checksum of object at "
              << message << " with data size " << dataSize
              << ".\n" << logofs_flush;
      #endif

      dataChecksum(message, buffer, size, bigEndian);
    }

    if (dataAction == discard_data)
    {
      #ifdef DEBUG
      *logofs << name() << ": Discarded " << dataSize
              << " bytes of plain data. Real size is "
              << message -> size_ << " compressed size is "
              << message -> c_size_ << ".\n" << logofs_flush;
      #endif

      return 1;
    }

    //
    // Accept anyway data beyond the
    // expected limit.
    //

    #ifdef TEST

    if (dataSize > (unsigned int) dataLimit)
    {
      *logofs << name() << ": WARNING! Data is " << dataSize
              << " bytes. Ignoring the established limit.\n"
              << logofs_flush;
    }

    #endif

    if (dataSize != message -> data_.size())
    {
      #ifdef DEBUG
      *logofs << name() << ": Data will be resized from "
              << message -> data_.size() << " to hold a plain buffer of "
              << dataSize << " bytes.\n" << logofs_flush;
      #endif

      message -> data_.clear();

      message -> data_.resize(dataSize);
    }

    if (split == 0)
    {
      memcpy(message -> data_.begin(), buffer + message -> i_size_, dataSize);
    }
    #ifdef TEST
    else
    {
      *logofs << name() << ": Not copied " << dataSize
              << " bytes of fake data for the split message.\n"
              << logofs_flush;
    }
    #endif

    #ifdef DEBUG
    *logofs << name() << ": Parsed " << dataSize
            << " bytes of plain data. Real size is "
            << message -> size_ << " compressed size is "
            << message -> c_size_ << ".\n" << logofs_flush;
    #endif
  }

  return 1;
}

//
// Store the data part in compressed format.
//

int MessageStore::parseData(Message *message, const unsigned char *buffer,
                                unsigned int size, const unsigned char *compressedData,
                                    const unsigned int compressedDataSize,
                                        T_checksum_action checksumAction,
                                             T_data_action dataAction, int bigEndian)
{
  if ((int) size > message -> i_size_)
  {
    unsigned int dataSize = size - message -> i_size_;

    if (checksumAction == use_checksum)
    {
      #ifdef DEBUG
      *logofs << name() << ": Calculating checksum of object at "
              << message << " with data size " << dataSize
              << ".\n" << logofs_flush;
      #endif

      dataChecksum(message, buffer, size, bigEndian);
    }

    if (dataAction == discard_data)
    {
      #ifdef DEBUG
      *logofs << name() << ": Discarded " << dataSize
              << " bytes of compressed data. Real size is "
              << message -> size_ << " compressed size is "
              << message -> c_size_ << ".\n" << logofs_flush;
      #endif

      return 1;
    }

    #ifdef WARNING
    if (dataSize > (unsigned int) dataLimit)
    {
      *logofs << name() << ": WARNING! Data is " << dataSize
              << " bytes. Ignoring the established limit!\n"
              << logofs_flush;
    }
    #endif

    dataSize = compressedDataSize;

    if (dataSize != message -> data_.size())
    {
      #ifdef DEBUG
      *logofs << name() << ": Data will be resized from "
              << message -> data_.size() << " to hold a compressed buffer of "
              << dataSize << " bytes.\n" << logofs_flush;
      #endif

      message -> data_.clear();

      message -> data_.resize(compressedDataSize);
    }

    memcpy(message -> data_.begin(), compressedData, compressedDataSize);

    #ifdef DEBUG
    *logofs << name() << ": Parsed " << dataSize
            << " bytes of compressed data. Real size is "
            << message -> size_ << " compressed size is "
            << message -> c_size_ << ".\n" << logofs_flush;
    #endif
  }

  return 1;
}

int MessageStore::unparseData(const Message *message, unsigned char *buffer,
                                  unsigned int size, int bigEndian)
{
  //
  // Copy data, if any, to the buffer.
  //

  if ((int) size > message -> i_size_)
  {
    //
    // Check if message has been stored
    // in compressed format.
    //

    if (message -> c_size_ == 0)
    {
      memcpy(buffer + message -> i_size_, message -> data_.begin(), size - message -> i_size_);

      #ifdef DEBUG
      *logofs << name() << ": Unparsed " << message -> size_ - message -> i_size_
              << " bytes of data to a buffer of " << message -> size_ - message -> i_size_
              << ".\n" << logofs_flush;
      #endif
    }
    else
    {
      #ifdef DEBUG
      *logofs << name() << ": Using static compressor at " << (void *) compressor_
              << ".\n" << logofs_flush;
      #endif

      if (compressor_ ->
              decompressBuffer(buffer + message -> i_size_,
                                   size - message -> i_size_,
                                       message -> data_.begin(),
                                           message -> c_size_ - message -> i_size_) < 0)
      {
        #ifdef PANIC
        *logofs << name() << ": PANIC! Data decompression failed.\n"
                << logofs_flush;
        #endif

        cerr << "Error" << ": Data decompression failed.\n";

        return -1;
      }

      #ifdef DEBUG
      *logofs << name() << ": Unparsed " << message -> c_size_ - message -> i_size_
              << " bytes of compressed data to a buffer of "
              << message -> size_ - message -> i_size_ << ".\n" << logofs_flush;
      #endif
    }
  }

  //
  // We could write size to the buffer but this
  // is something the channel class is doing by
  // itself.
  //
  // PutUINT(size >> 2, buffer + 2, bigEndian);
  //

  return 1;
}

void MessageStore::dumpData(const Message *message) const
{
  #ifdef DUMP

  *logofs << name() << ": Dumping enumerated data:\n" << logofs_flush; 

  DumpData(message -> data_.begin(), message -> data_.size());

  #endif

  #ifdef DUMP

  *logofs << name() << ": Dumping checksum data:\n" << logofs_flush; 

  DumpData(message -> md5_digest_, MD5_LENGTH);

  #endif
}

T_checksum MessageStore::getChecksum(const unsigned char *buffer,
                                         unsigned int size, int bigEndian)
{
  Message *message = getTemporary();

  message -> size_   = size;
  message -> i_size_ = identitySize(buffer, size);
  message -> c_size_ = 0;

  validateSize(size);

  beginChecksum(message);

  //
  // We don't need to extract the identity
  // data from the buffer.
  //
  // parseIdentity(message, buffer, size, bigEndian);
  //

  identityChecksum(message, buffer, size, bigEndian);

  parseData(message, 0, buffer, size, use_checksum, discard_data, bigEndian);

  endChecksum(message);

  //
  // The caller will have to explicitly
  // deallocated the memory after use.
  //

  T_checksum checksum = new md5_byte_t[MD5_LENGTH];

  memcpy(checksum, message -> md5_digest_, MD5_LENGTH);

  return checksum;
}

int MessageStore::clean(T_checksum_action checksumAction)
{
  int position = lastRemoved + 1;

  if (position >= cacheSlots)
  {
    position = 0;
  }

  #ifdef DEBUG
  *logofs << name() << ": Searching a message to remove "
          << "starting at position " << position
          << " with " << checksums_ -> size()
          << " elements in cache.\n"
          << logofs_flush;
  #endif

  while (position != lastRemoved)
  {
    #ifdef DEBUG
    *logofs << name() << ": Examining position "
            << position << ".\n" << logofs_flush;
    #endif

    if ((*messages_)[position] != NULL)
    {
      if (getRating((*messages_)[position], rating_for_clean) == 0)
      {
        break;
      }
      else
      {
        untouch((*messages_)[position]);
      }
    }

    if (++position == cacheSlots)
    {
      #ifdef DEBUG
      *logofs << name() << ": Rolled position at "
              << strMsTimestamp() << ".\n"
              << logofs_flush;
      #endif

      position = 0;
    }
  }

  //
  // If no message is a good candidate,
  // then try the object at the next slot
  // in respect to last element removed.
  //

  if (position == lastRemoved)
  {
    position = lastRemoved + 1;

    if (position >= cacheSlots)
    {
      position = 0;
    }

    if ((*messages_)[position] == NULL ||
            (*messages_)[position] -> locks_ != 0)
    {
      #ifdef DEBUG
      *logofs << name() << ": WARNING! No message found "
            << "to be actually removed.\n"
            << logofs_flush;
      #endif

      return nothing;
    }

    #ifdef DEBUG
    *logofs << name() << ": WARNING! Assuming object "
            << "at position " << position << ".\n"
            << logofs_flush;
    #endif
  }

  return position;
}

//
// This is the insertion method used at local side 
// side. Cache at remote side side will be kept in
// sync by telling the to other party where to 
// store the message. 
//

int MessageStore::findOrAdd(Message *message, T_checksum_action checksumAction,
                                T_data_action dataAction, int &added, int &locked)
{
  if (checksumAction != use_checksum)
  {
    #ifdef PANIC
    *logofs << name() << ": PANIC! Internal error in context [A]. "
            << "Cannot find or add message to repository "
            << "without using checksum.\n" << logofs_flush;
    #endif

    cerr << "Error" << ": Internal error in context [A]. "
         << "Cannot find or add message to repository "
         << "without using checksum.\n";

    HandleAbort();
  }

  //
  // Set added to true only if message
  // is inserted in cache.
  //

  added  = 0;
  locked = 0;

  //
  // First of all figure out where to
  // store this object.
  //

  #ifdef DEBUG
  *logofs << name() << ": Searching an empty slot "
          << "with last rated " << lastRated << " and "
          << "last added " << lastAdded << ".\n"
          << logofs_flush;
  #endif

  int position = lastRated;

  if (position == nothing)
  {
    position = lastAdded + 1;

    if (position >= cacheSlots)
    {
      position = 0;
    }

    #ifdef DEBUG
    *logofs << name() << ": Searching an empty slot "
            << "starting at position " << position
            << " with " << checksums_ -> size()
            << " elements in cache.\n"
            << logofs_flush;
    #endif

    while (position != lastAdded)
    {
      #ifdef DEBUG
      *logofs << name() << ": Examining position "
              << position << ".\n" << logofs_flush;
      #endif

      if ((*messages_)[position] == NULL)
      {
        break;
      }
      else if (getRating((*messages_)[position], rating_for_insert) == 0)
      {
        break;
      }
      else
      {
        untouch((*messages_)[position]);
      }

      if (++position == cacheSlots)
      {
        #ifdef DEBUG
        *logofs << name() << ": Rolled position at "
                << strMsTimestamp() << ".\n"
                << logofs_flush;
        #endif

        position = 0;
      }
    }
  }
  #ifdef DEBUG
  else
  {
    *logofs << name() << ": Using last rated position "
            << position << ".\n" << logofs_flush;
  }
  #endif

  //
  // If we made an extensive check but did not
  // find neither a free slot or a message to
  // replace, assume slot at next position in
  // respect to last added. This can happen if
  // all objects in repository have got an hit
  // recently.
  //

  if (position == lastAdded)
  {
    position = lastAdded + 1;

    if (position >= cacheSlots)
    {
      position = 0;
    }

    #ifdef DEBUG
    *logofs << name() << ": WARNING! Assuming slot "
            << "at position " << position << ".\n"
            << logofs_flush;
    #endif
  }
  #ifdef DEBUG
  else
  {
    *logofs << name() << ": Found candidate slot "
            << "at position " << position << ".\n"
            << logofs_flush;
  }
  #endif

  //
  // Save the search result so if the message
  // is found in cache, we can use the slot
  // at next run.
  //

  lastRated = position;

  if ((*messages_)[position] != NULL &&
          (*messages_)[position] -> locks_ != 0)
  {
    #ifdef WARNING
    *logofs << name() << ": WARNING! Insertion at position "
            << position << " would replace a locked message. "
            << "Forcing channel to discard the message.\n"
            << logofs_flush;
    #endif

    #ifdef TEST
    *logofs << name() << ": Invalidating rating of object "
            << "at position " << position << ".\n"
            << logofs_flush;
    #endif

    return (lastRated = nothing);
  }

  if (checksumAction == use_checksum)
  {
    T_checksum checksum = getChecksum(message);

    #ifdef TEST
    *logofs << name() << ": Searching checksum ["
            << DumpChecksum(checksum) << "] in repository.\n"
            << logofs_flush;

    #endif

    pair<T_checksums::iterator, bool> result;

    result = checksums_ -> insert(T_checksums::value_type(checksum, position));

    //
    // Message was found in cache or
    // insertion couldn't take place.
    //

    if (result.second == 0)
    {
      if (result.first == checksums_ -> end())
      {
        #ifdef PANIC
        *logofs << name() << ": PANIC! Failed to insert object "
                << "in the cache.\n" << logofs_flush;
        #endif

        cerr << "Error" << ": Failed to insert object of type "
             << name() << " in the cache.\n";

        return nothing;
      }

      //
      // Message is in cache.
      //

      #ifdef TEST
      *logofs << name() << ": Object is already in cache "
              << "at position " << (result.first) -> second
              << ".\n" << logofs_flush;
      #endif

      #ifdef DEBUG

      printStorageSize();

      #endif

      //
      // Message is locked, probably because
      // it has not completely recomposed at
      // remote side after a split.
      //

      if ((*messages_)[(result.first) -> second] -> locks_ != 0)
      {
        #ifdef TEST
        *logofs << name() << ": WARNING! Object at position "
                << (result.first) -> second << " is locked.\n"
                << logofs_flush;
        #endif

        locked = 1;
      }

      //
      // Object got a hit, so prevent
      // its removal.
      //

      if (lastRated == (result.first) -> second)
      {
        #ifdef TEST
        *logofs << name() << ": Resetting rating of object "
                << "at position " << (result.first) -> second
                << ".\n" << logofs_flush;
        #endif

        lastRated = nothing;
      }

      return (result.first) -> second;
    }

    #ifdef DEBUG
    *logofs << name() << ": Could not find message in cache.\n"
            << logofs_flush;
    #endif
  }

  //
  // Message not found in hash table (or insertion
  // of checksum in hash table was not requested).
  // Message was added to cache.
  //

  added = 1;

  //
  // Log data about the missed message.
  //

  #ifdef TEST

  if (opcode() == X_PutImage || opcode() == X_NXPutPackedImage)
  {
    #ifdef WARNING
    *logofs << name() << ": WARNING! Dumping identity of "
            << "missed image object of type " << name()
            << ".\n" << logofs_flush;
    #endif

    dumpIdentity(message);
  }

  #endif

  if ((*messages_)[position] != NULL)
  {
    #ifdef DEBUG
    *logofs << name() << ": The message replaces "
            << "the old one at position " << position
            << ".\n" << logofs_flush;
    #endif

    remove(position, checksumAction, dataAction);
  }

  (*messages_)[position] = message;

  //
  // We used the slot. Perform a new
  // search at next run.
  //

  lastRated = nothing;

  #ifdef TEST
  *logofs << name() << ": Stored message object of size "
          << plainSize(position) << " (" << message -> size_
          << "/" << message -> c_size_ << ") at position "
          << position << ".\n" << logofs_flush;
  #endif

  unsigned int localSize;
  unsigned int remoteSize;

  storageSize(message, localSize, remoteSize);

  localStorageSize_  += localSize;
  remoteStorageSize_ += remoteSize;

  totalLocalStorageSize_  += localSize;
  totalRemoteStorageSize_ += remoteSize;

  #ifdef DEBUG

  printStorageSize();

  #endif

  //
  // Set hits and timestamp at insertion in cache.
  //

  message -> hits_ = control -> StoreHitsAddBonus;
  message -> last_ = (getTimestamp()).tv_sec;

  message -> locks_ = 0;

  #ifdef DEBUG
  *logofs << name() << ": Set last hit of object at " 
          << strMsTimestamp() << " with a bonus of "
          << message -> hits_ << ".\n" << logofs_flush;
  #endif

  return position;
}

//
// Add a parsed message to repository. It is normally used
// at decoding side or at encoding side when we load store
// from disk. To handle messages coming from network, the
// encoding side uses the optimized method findOrAdd().
//

int MessageStore::add(Message *message, const int position,
                          T_checksum_action checksumAction, T_data_action dataAction)
{
  if (position < 0 || position >= cacheSlots)
  {
    #ifdef PANIC
    *logofs << name() << ": PANIC! Cannot add a message "
            << "at non existing position " << position
            << ".\n" << logofs_flush;
    #endif

    cerr << "Error" << ": Cannot add a message "
         << "at non existing position " << position
         << ".\n";

    HandleAbort();
  }

  if ((*messages_)[position] != NULL)
  {
    #ifdef DEBUG
    *logofs << name() << ": The message will replace "
            << "the old one at position " << position
            << ".\n" << logofs_flush;
    #endif

    remove(position, checksumAction, dataAction);
  }

  #ifdef DEBUG
  *logofs << name() << ": Inserting object in repository at position "
          << position << ".\n" << logofs_flush;
  #endif

  (*messages_)[position] = message;

  //
  // Get the object's checksum value
  // and insert it in the table.
  //

  if (checksumAction == use_checksum)
  {
    #ifdef DEBUG
    *logofs << name() << ": Inserting object's checksum in repository.\n";
    #endif

    T_checksum checksum = getChecksum(message);

    checksums_ -> insert(T_checksums::value_type(checksum, position));
  }

  #ifdef DEBUG
  *logofs << name() << ": Stored message object of size "
          << plainSize(position) << " (" << message -> size_ 
          << "/" << message -> c_size_ << ") at position "
          << position << ".\n" << logofs_flush;
  #endif

  unsigned int localSize;
  unsigned int remoteSize;

  storageSize(message, localSize, remoteSize);

  localStorageSize_  += localSize;
  remoteStorageSize_ += remoteSize;

  totalLocalStorageSize_  += localSize;
  totalRemoteStorageSize_ += remoteSize;

  #ifdef DEBUG

  printStorageSize();

  #endif

  //
  // Set hits and timestamp at insertion in cache.
  //

  message -> hits_ = control -> StoreHitsAddBonus;
  message -> last_ = (getTimestamp()).tv_sec;

  message -> locks_ = 0;

  #ifdef DEBUG
  *logofs << name() << ": Set last hit of object at " 
          << strMsTimestamp() << " with a bonus of "
          << message -> hits_ << ".\n" << logofs_flush;
  #endif

  return position;
}

//
// The following functions don't modify data,
// so they are supposed to be called only at
// the encoding side.
//

void MessageStore::updateData(const int position, unsigned int dataSize,
                                  unsigned int compressedDataSize)
{
  Message *message = (*messages_)[position];

  validateSize(dataSize, compressedDataSize);

  if (compressedDataSize != 0)
  {
    unsigned int localSize;
    unsigned int remoteSize;

    storageSize(message, localSize, remoteSize);

    localStorageSize_  -= localSize;
    remoteStorageSize_ -= remoteSize;

    totalLocalStorageSize_  -= localSize;
    totalRemoteStorageSize_ -= remoteSize;

    message -> c_size_ = compressedDataSize + message -> i_size_;

    #ifdef TEST

    if (message -> size_ != (int) (dataSize + message -> i_size_))
    {
      #ifdef PANIC
      *logofs << name() << ": PANIC! Size of object looks "
              << message -> size_ << " bytes while it "
              << "should be " << dataSize + message -> i_size_
              << ".\n" << logofs_flush;
      #endif

      cerr << "Error" << ": Size of object looks "
           << message -> size_ << " bytes while it "
           << "should be " << dataSize + message -> i_size_
           << ".\n";

      HandleAbort();
    }

    #endif

    storageSize(message, localSize, remoteSize);

    localStorageSize_  += localSize;
    remoteStorageSize_ += remoteSize;

    totalLocalStorageSize_  += localSize;
    totalRemoteStorageSize_ += remoteSize;

    #ifdef DEBUG

    printStorageSize();

    #endif
  }
}

void MessageStore::updateData(const T_checksum checksum, unsigned int compressedDataSize)
{
  #ifdef TEST
  *logofs << name() << ": Searching checksum ["
          << DumpChecksum(checksum) << "] in repository.\n"
          << logofs_flush;
  #endif

  T_checksums::iterator found = checksums_ -> find(checksum);

  if (found != checksums_ -> end())
  {
    Message *message = (*messages_)[found -> second];

    #ifdef TEST
    *logofs << name() << ": Message found in cache at "
            << "position " << found -> second << " with size "
            << message -> size_ << " and compressed size "
            << message -> c_size_ << ".\n" << logofs_flush;
    #endif

    updateData(found -> second, message -> size_ -
                   message -> i_size_, compressedDataSize);
  }
  #ifdef TEST
  else if (checksums_ -> size() > 0)
  {
    *logofs << name() << ": WARNING! Can't locate the "
            << "checksum [" << DumpChecksum(checksum)
            << "] for the update.\n" << logofs_flush;
  }
  #endif
}

//
// This function replaces the data part of the message
// and updates the information about its size. Split
// messages are advertised to the decoding side with
// their uncompressed size, data is then compressed
// before sending the first chunk. This function is
// called by the decoding side after the split message
// is fully recomposed to replace the dummy data and
// set the real size.
//

void MessageStore::updateData(const int position, const unsigned char *newData,
                                  unsigned int dataSize, unsigned int compressedDataSize)
{
  Message *message = (*messages_)[position];

  validateSize(dataSize, compressedDataSize);

  #ifdef TEST

  if (message -> size_ != (int) (dataSize + message -> i_size_))
  {
    #ifdef PANIC
    *logofs << name() << ": PANIC! Data of object looks "
            << dataSize << " bytes while it " << "should be "
            << message -> size_ - message -> i_size_
            << ".\n" << logofs_flush;
    #endif

    cerr << "Error" << ": Data of object looks "
         << dataSize << " bytes while it " << "should be "
         << message -> size_ - message -> i_size_
         << ".\n";

    HandleAbort();
  }

  #endif

  //
  // A compressed data size of 0 means that
  // message's data was not compressed.
  //

  if (compressedDataSize != 0)
  {
    unsigned int localSize;
    unsigned int remoteSize;

    storageSize(message, localSize, remoteSize);

    localStorageSize_  -= localSize;
    remoteStorageSize_ -= remoteSize;

    totalLocalStorageSize_  -= localSize;
    totalRemoteStorageSize_ -= remoteSize;

    if (message -> c_size_ != (int) compressedDataSize +
            message -> i_size_)
    {
      #ifdef TEST
      *logofs << name() << ": Resizing data of message at "
              << "position " << position << " from " << message ->
                  c_size_ << " to " << compressedDataSize +
                  message -> i_size_ << " bytes.\n"
              << logofs_flush;
      #endif

      message -> data_.clear();

      message -> data_.resize(compressedDataSize);
    }

    memcpy(message -> data_.begin(), newData, compressedDataSize);

    #ifdef TEST
    *logofs << name() << ": Data of message at position "
            << position << " has size " << message -> data_.size()
            << " and capacity " << message -> data_.capacity()
            << ".\n" << logofs_flush;
    #endif

    message -> c_size_ = compressedDataSize + message -> i_size_;

    storageSize(message, localSize, remoteSize);

    localStorageSize_  += localSize;
    remoteStorageSize_ += remoteSize;

    totalLocalStorageSize_  += localSize;
    totalRemoteStorageSize_ += remoteSize;

    #ifdef DEBUG

    printStorageSize();

    #endif
  }
  else
  {
    #ifdef TEST
    *logofs << name() << ": No changes to data size for message "
            << "at position " << position << ".\n" << logofs_flush;
    #endif

    memcpy(message -> data_.begin(), newData, dataSize);
  }
}

int MessageStore::remove(const int position, T_checksum_action checksumAction,
                             T_data_action dataAction)
{
  Message *message;

  if (position < 0 || position >= cacheSlots ||
          (message = (*messages_)[position]) == NULL)
  {
    #ifdef PANIC
    *logofs << name() << ": PANIC! Cannot remove "
            << "a non existing message at position "
            << position << ".\n" << logofs_flush;
    #endif

    cerr << "Error" << ": Cannot remove "
         << "a non existing message at position "
         << position << ".\n";

    HandleAbort();
  }

  #if defined(TEST) || defined(INFO)

  if (opcode() == X_PutImage || opcode() == X_NXPutPackedImage)
  {
    #ifdef WARNING
    *logofs << name() << ": WARNING! Discarding image object "
            << "of type " << name() << " at position "
            << position << ".\n" << logofs_flush;
    #endif
  }

  #endif

  //
  // The checksum is only stored at the encoding
  // side.
  //

  if (checksumAction == use_checksum)
  {
    #ifdef DEBUG
    *logofs << name() << ": Removing checksum for object at "
            << "position " << position << ".\n" << logofs_flush;
    #endif

    //
    // TODO: If we had stored the iterator and
    // not the pointer to the message, we could
    // have removed the message without having
    // to look up the checksum.
    //

    T_checksum checksum = getChecksum(message);

    #ifdef TEST
    *logofs << name() << ": Searching checksum ["
            << DumpChecksum(checksum) << "] in repository.\n"
            << logofs_flush;
    #endif

    T_checksums::iterator found = checksums_ -> find(checksum);

    if (found == checksums_ -> end())
    {
      #ifdef PANIC
      *logofs << name() << ": PANIC! No checksum found for "
              << "object at position " << position << ".\n"
              << logofs_flush;
      #endif

      cerr << "Error" << ": No checksum found for "
           << "object at position " << position << ".\n";

      HandleAbort();
    }

    #ifdef TEST

    else if (position != found -> second)
    {
      #ifdef PANIC
      *logofs << name() << ": PANIC! Value of position for object "
              << "doesn't match position " << position << ".\n"
              << logofs_flush;
      #endif

      cerr << "Error" << ": Value of position for object "
           << "doesn't match position " << position << ".\n";

      HandleAbort();
    }

    #endif

    checksums_ -> erase(found);
  }

  #ifdef DEBUG
  *logofs << name() << ": Removing message at position "
          << position << " of size " << plainSize(position)
          << " (" << message -> size_ << "/" << message -> c_size_
          << ").\n" << logofs_flush;
  #endif

  unsigned int localSize;
  unsigned int remoteSize;

  storageSize(message, localSize, remoteSize);

  localStorageSize_  -= localSize;
  remoteStorageSize_ -= remoteSize;

  totalLocalStorageSize_  -= localSize;
  totalRemoteStorageSize_ -= remoteSize;

  recycle(message);

  (*messages_)[position] = NULL;

  #ifdef DEBUG

  printStorageSize();

  #endif

  return position;
}

//
// This should only be called at encoding side.
// The decoding side can't rely on the counter
// as it is decremented by the encoding side
// every time the repository is searched for a
// message to be removed.
//

int MessageStore::getRating(Message *message, T_rating type) const
{
  if (message -> locks_ != 0)
  {
    #ifdef TEST
    *logofs << name() << ": Rate set to -1 as locks of object are "
            << (int) message -> locks_ << ".\n"
            << logofs_flush;
    #endif

    return -1;
  }
  else if ((type == rating_for_clean ||
                (int) checksums_ -> size() == cacheSlots) &&
                    message -> hits_ <= control -> StoreHitsLoadBonus)
  {
    //
    // We don't have any free slot or we exceeded the
    // available storage size. This is likely to happen
    // after having loaded objects from persistent cache.
    // It's not a bad idea to discard some messages that
    // were restored but never referenced.
    //

    #ifdef TEST

    if (type == rating_for_clean)
    {
      *logofs << name() << ": Rate set to 0 with hits "
              << message -> hits_ << " as maximum storage size "
              << "was exceeded.\n" << logofs_flush;
    }
    else
    {
      *logofs << name() << ": Rate set to 0 with hits "
              << message -> hits_ << " as there are no available "
              << "slots in store.\n" << logofs_flush;
    }

    #endif

    return 0;
  }
  else if (type == rating_for_clean &&
               (getTimestamp()).tv_sec - message -> last_ >=
                   control -> StoreTimeLimit)
  {
    #ifdef TEST
    *logofs << name() << ": Rate set to 0 as last hit of object was "
            << (getTimestamp()).tv_sec - message -> last_
            << " seconds ago with limit set to " << control ->
               StoreTimeLimit << ".\n" << logofs_flush;
    #endif

    return 0;
  }
  else
  {
    #ifdef TEST
    if (message -> hits_ < 0)
    {
      *logofs << name() << ": PANIC! Rate of object shouldn't be "
              << message -> hits_ << ".\n" << logofs_flush;

      cerr << "Error" << ": Rate of object of type " << name()
           << " shouldn't be " << message -> hits_ << ".\n";

      HandleAbort();
    }
    #endif

    #ifdef TEST
    *logofs << name() << ": Rate of object is " << message -> hits_
            << " with last hit " << (getTimestamp()).tv_sec -
               message -> last_ << " seconds ago.\n"
            << logofs_flush;
    #endif

    return message -> hits_;
  }
}

int MessageStore::touch(Message *message) const
{
  message -> last_ = (getTimestamp()).tv_sec;

  message -> hits_ += control -> StoreHitsTouch;

  if (message -> hits_ > control -> StoreHitsLimit)
  {
    message -> hits_ = control -> StoreHitsLimit;
  }

  #ifdef TEST
  *logofs << name() << ": Increased hits of object to "
          << message -> hits_ << " at " << strMsTimestamp()
          << ".\n" << logofs_flush;
  #endif

  return message -> hits_;
}

int MessageStore::untouch(Message *message) const
{
  message -> hits_ -= control -> StoreHitsUntouch;

  if (message -> hits_ < 0)
  {
    message -> hits_ = 0;
  }

  #ifdef TEST
  *logofs << name() << ": Decreased hits of object to " 
          << message -> hits_ << ".\n"
          << logofs_flush;
  #endif

  return message -> hits_;
}

int MessageStore::lock(const int position) const
{
  Message *message = (*messages_)[position];

  if (message == NULL)
  {
    #ifdef PANIC
    *logofs << name() << ": PANIC! Can't lock the null "
            << "object at position " << position
            << ".\n" << logofs_flush;
    #endif

    return -1;
  }

  #ifdef DEBUG
  *logofs << name() << ": Increasing locks of object to " 
          << (int) message -> locks_ + 1 << ".\n"
          << logofs_flush;
  #endif

  return ++(message -> locks_);
}

int MessageStore::unlock(const int position) const
{
  Message *message = (*messages_)[position];

  if (message == NULL)
  {
    #ifdef PANIC
    *logofs << name() << ": PANIC! Can't unlock the null "
            << "object at position " << position
            << ".\n" << logofs_flush;
    #endif

    return -1;
  }

  #ifdef DEBUG
  *logofs << name() << ": Decreasing locks of object to " 
          << (int) message -> locks_ - 1 << ".\n"
          << logofs_flush;
  #endif

  return --(message -> locks_);
}

int MessageStore::saveStore(ostream *cachefs, md5_state_t *md5StateStream,
                                md5_state_t *md5StateClient, T_checksum_action checksumAction,
                                    T_data_action dataAction, int bigEndian)
{
  Message *message;

  #ifdef TEST
  *logofs << name() << ": Opcode of this store is "
          << (unsigned int) opcode() << " default size of "
          << "identity is " << dataOffset << ".\n"
          << logofs_flush;
  #endif

  unsigned char *identityBuffer = new unsigned char[dataOffset];
  unsigned char *sizeBuffer     = new unsigned char[4 * 2];
  unsigned char *positionBuffer = new unsigned char[4];
  unsigned char *opcodeBuffer   = new unsigned char[4];

  #ifdef DUMP

  char *md5ClientDump = new char[dataOffset * 2 + 128];

  #endif

  unsigned char value;

  int offset;

  int failed = 0;

  for (int position = 0; position < cacheSlots; position++)
  {
    message = (*messages_)[position];

    //
    // Don't save split messages.
    //

    if (message != NULL && message -> locks_ == 0)
    {
      //
      // Use the total size if offset is
      // beyond the real end of message.
      //

      offset = dataOffset;

      if (offset > message -> size_)
      {
        offset = message -> size_;
      }
        
      #ifdef TEST
      *logofs << name() << ": Going to save message at position "
              << position << ".\n" << logofs_flush;
      #endif

      value = 1;

      PutULONG(position, positionBuffer, bigEndian);
      PutULONG(opcode(), opcodeBuffer, bigEndian);

      md5_append(md5StateClient, positionBuffer, 4);
      md5_append(md5StateClient, opcodeBuffer, 4);

      #ifdef DUMP

      *logofs << "Name=" << name() << logofs_flush;

      sprintf(md5ClientDump," Pos=%d Op=%d\n", position, opcode());

      *logofs << md5ClientDump << logofs_flush;

      #endif

      if (PutData(cachefs, &value, 1) < 0)
      {
        #ifdef DEBUG
        *logofs << name() << ": PANIC! Failure writing " << 1
                << " bytes.\n" << logofs_flush;
        #endif

        failed = 1;

        break;
      }

      md5_append(md5StateStream, &value, 1);

      PutULONG(message -> size_, sizeBuffer, bigEndian);
      PutULONG(message -> c_size_, sizeBuffer + 4, bigEndian);

      //
      // Note that the identity size is not saved with
      // the message and will be determined from the
      // data read when restoring the identity.
      //

      if (PutData(cachefs, sizeBuffer, 4 * 2) < 0)
      {
        #ifdef DEBUG
        *logofs << name() << ": PANIC! Failure writing " << 4 * 2
                << " bytes.\n" << logofs_flush;
        #endif

        failed = 1;

        break;
      }

      md5_append(md5StateStream, sizeBuffer, 4 * 2);
      md5_append(md5StateClient, sizeBuffer, 4 * 2);

      #ifdef DUMP

      sprintf(md5ClientDump, "size = %d c_size = %d\n",
                  message -> size_, message -> c_size_);

      *logofs << md5ClientDump << logofs_flush;

      #endif

      //
      // Prepare a clean buffer for unparse.
      //

      CleanData(identityBuffer, offset);

      unparseIdentity(message, identityBuffer, offset, bigEndian);

      if (PutData(cachefs, identityBuffer, offset) < 0)
      {
        #ifdef DEBUG
        *logofs << name() << ": PANIC! Failure writing " << offset
                << " bytes.\n" << logofs_flush;
        #endif

        failed = 1;

        break;
      }

      md5_append(md5StateStream, identityBuffer, offset);
      md5_append(md5StateClient, identityBuffer, offset);

      #ifdef DUMP

      for (int i = 0; i < offset; i++)
      {
        sprintf(md5ClientDump + (i * 2), "%02X", identityBuffer[i]);
      }

      *logofs << "Identity = " << md5ClientDump << "\n" << logofs_flush;

      #endif

      //
      // Set the real identity size before
      // saving the data.
      //

      offset = message -> i_size_;

      if (offset > message -> size_)
      {
        offset = message -> size_;
      }

      if (checksumAction == use_checksum)
      {
        if (PutData(cachefs, message -> md5_digest_, MD5_LENGTH) < 0)
        {
          #ifdef DEBUG
          *logofs << name() << ": PANIC! Failure writing " << MD5_LENGTH
                  << " bytes.\n" << logofs_flush;
          #endif

          failed = 1;

          break;
        }

        md5_append(md5StateStream, message -> md5_digest_, MD5_LENGTH);
      }
      else if (dataAction == use_data)
      {
        int dataSize = (message -> c_size_ == 0 ?
                             message -> size_ - offset :
                                 message -> c_size_ - offset);
        if (dataSize > 0)
        {
          if (PutData(cachefs, message -> data_.begin(), dataSize) < 0)
          {
            #ifdef DEBUG
            *logofs << name() << ": PANIC! Failure writing " << dataSize
                    << " bytes.\n" << logofs_flush;
            #endif

            failed = 1;

            break;
          }

          md5_append(md5StateStream, message -> data_.begin(), dataSize);
        }
      }
    }
    else
    {
      #ifdef TEST
      *logofs << name() << ": Not saving message at position "
              << position << ".\n" << logofs_flush;
      #endif

      value = 0;

      if (PutData(cachefs, &value, 1) < 0)
      {
        #ifdef DEBUG
        *logofs << name() << ": PANIC! Failure writing " << 1
                << " bytes.\n" << logofs_flush;
        #endif

        failed = 1;

        break;
      }

      md5_append(md5StateStream, &value, 1);
    }
  }

  if (failed == 1)
  {
    #ifdef PANIC
    *logofs << name() << ": PANIC! Write to persistent cache file failed.\n"
            << logofs_flush;
    #endif

    cerr << "Error" << ": Write to persistent cache file failed.\n";
  }

  delete [] identityBuffer;
  delete [] sizeBuffer;
  delete [] positionBuffer;
  delete [] opcodeBuffer;

  #ifdef DUMP

  delete [] md5ClientDump;

  #endif

  return (failed == 0 ? 1 : -1);
}

int MessageStore::loadStore(istream *cachefs, md5_state_t *md5StateStream,
                                T_checksum_action checksumAction, T_data_action dataAction,
                                    int bigEndian)
{
  Message *message;

  #ifdef TEST
  *logofs << name() << ": Opcode of this store is "
          << (unsigned int) opcode() << " default size of "
          << "identity is " << dataOffset << " slots are "
          << cacheSlots << ".\n" << logofs_flush;
  #endif

  //
  // If packed images or the render extension has been
  // disabled we don't need to restore these messages
  // in the cache. Encoding of RENDER in 1.4.0 is also
  // changed so we want to skip messages saved using
  // the old format. We want to restore all the other
  // messages so we'll need to skip these one by one.
  //

  int skip = 0;

  if ((opcode() == X_NXPutPackedImage &&
          control -> PersistentCacheLoadPacked == 0) ||
              (opcode() == X_NXInternalRenderExtension &&
                  control -> PersistentCacheLoadRender == 0))
  {
    #ifdef TEST
    *logofs << name() << ": All messages for OPCODE#"
            << (unsigned int) opcode() << " will be discarded.\n"
            << logofs_flush;
    #endif

    skip = 1;
  }

  unsigned char *identityBuffer = new unsigned char[dataOffset];
  unsigned char *sizeBuffer     = new unsigned char[4 * 2];

  unsigned char value;

  int offset;

  int failed = 0;

  for (int position = 0; position < cacheSlots; position++)
  {
    if (GetData(cachefs, &value, 1) < 0)
    {
      #ifdef DEBUG
      *logofs << name() << ": PANIC! Failure reading " << 1
              << " bytes.\n" << logofs_flush;
      #endif

      failed = 1;

      break;
    }

    md5_append(md5StateStream, &value, 1);

    if (value == 1)
    {
      #ifdef TEST
      *logofs << name() << ": Going to load message at position "
              << position << ".\n" << logofs_flush;
      #endif

      if (GetData(cachefs, sizeBuffer, 4 * 2) < 0)
      {
        #ifdef DEBUG
        *logofs << name() << ": PANIC! Failure reading " << 4 * 2
                << " bytes.\n" << logofs_flush;
        #endif

        failed = 1;

        break;
      }

      md5_append(md5StateStream, sizeBuffer, 4 * 2);

      message = getTemporary();

      if (message == NULL)
      {
        #ifdef PANIC
        *logofs << name() << ": PANIC! Can't access temporary storage "
                << "for message in context [B].\n" << logofs_flush;
        #endif

        cerr << "Error" << ": Can't access temporary storage "
             << "for message in context [B].\n";

        failed = 1;

        break;
      }

      message -> size_   = GetULONG(sizeBuffer, bigEndian);
      message -> c_size_ = GetULONG(sizeBuffer + 4, bigEndian);

      #ifdef DEBUG
      *logofs << name() << ": Size is " << message -> size_
              << " compressed size is " << message -> c_size_
              << ".\n" << logofs_flush;
      #endif

      //
      // Use the total size if offset is
      // beyond the real end of message.
      //

      offset = dataOffset;

      if (offset > message -> size_)
      {
        offset = message -> size_;
      }

      if (GetData(cachefs, identityBuffer, offset) < 0)
      {
        #ifdef DEBUG
        *logofs << name() << ": PANIC! Failure reading " << offset
                << " bytes.\n" << logofs_flush;
        #endif

        failed = 1;

        break;
      }

      md5_append(md5StateStream, identityBuffer, offset);

      //
      // Get the real identity size based on the value
      // reported by the message store. The dataOffset
      // value is guaranteed to be greater or equal to
      // the maximum identity size of the messages in
      // the major store.
      //

      offset = identitySize(identityBuffer, offset);

      if (offset > message -> size_)
      {
        offset = message -> size_;
      }

      message -> i_size_ = offset;

      //
      // Get identity of message from the buffer we just
      // created. Don't calculate neither checksum nor
      // data, restore them from stream. Don't pass the
      // message's size but the default size of identity.
      //

      parseIdentity(message, identityBuffer, offset, bigEndian);

      if (checksumAction == use_checksum)
      {
        if (message -> md5_digest_ == NULL)
        {
          message -> md5_digest_ = new md5_byte_t[MD5_LENGTH];
        }

        if (GetData(cachefs, message -> md5_digest_, MD5_LENGTH) < 0)
        {
          #ifdef DEBUG
          *logofs << name() << ": PANIC! Failure reading " << MD5_LENGTH
                  << " bytes.\n" << logofs_flush;
          #endif

          failed = 1;

          break;
        }

        //
        // Add message's checksum to checksum that will
        // be saved together with this cache. Checksum
        // will be verified when cache file is restored
        // to ensure file is not corrupted.
        //

        md5_append(md5StateStream, message -> md5_digest_, MD5_LENGTH);

        if (skip == 1)
        {
          #ifdef TEST
          *logofs << name() << ": Discarding message for OPCODE#"
                  << (unsigned int) opcode() << ".\n"
                  << logofs_flush;
          #endif
          
          continue;
        }
      }
      else if (dataAction == use_data)
      {
        //
        // Restore the data part.
        //

        int dataSize = (message -> c_size_ == 0 ?
                             message -> size_ - offset :
                                 message -> c_size_ - offset);

        if (dataSize < 0 || dataSize > control -> MaximumMessageSize)
        {
          #ifdef PANIC
          *logofs << name() << ": PANIC! Bad data size "
                  << dataSize << " loading persistent cache.\n"
                  << logofs_flush;
          #endif

          cerr << "Error" << ": Bad data size " << dataSize
               << " loading persistent cache.\n";

          failed = 1;

          break;
        }
        else if (dataSize > 0)
        {
          //
          // If need to skip the message let anyway
          // it to be part of the calculated MD5.
          //

          if (skip == 1)
          {
            unsigned char *dummy = new unsigned char[dataSize];

            if (dummy == NULL)
            {
              #ifdef PANIC
              *logofs << name() << ": PANIC! Can't allocate dummy buffer "
                      << "of size " << dataSize << " loading cache.\n"
                      << logofs_flush;
              #endif

              cerr << "Error" << ": Can't allocate dummy buffer "
                   << "of size " << dataSize << " loading cache.\n";

              failed = 1;

              break;
            }

            if (GetData(cachefs, dummy, dataSize) < 0)
            {
              #ifdef DEBUG
              *logofs << name() << ": PANIC! Failure reading " << dataSize
                      << " bytes.\n" << logofs_flush;
              #endif

              failed = 1;

              break;
            }

            md5_append(md5StateStream, dummy, dataSize);

            delete [] dummy;

            #ifdef TEST
            *logofs << name() << ": Discarding message for OPCODE#"
                    << (unsigned int) opcode() << ".\n"
                    << logofs_flush;
            #endif

            continue;
          }
          else
          {
            message -> data_.clear();

            message -> data_.resize(dataSize);

            if (GetData(cachefs, message -> data_.begin(), dataSize) < 0)
            {
              #ifdef DEBUG
              *logofs << name() << ": PANIC! Failure reading " << dataSize
                      << " bytes.\n" << logofs_flush;
              #endif

              failed = 1;

              break;
            }

            //
            // Add message's data to cache checksum.
            //

            md5_append(md5StateStream, message -> data_.begin(), dataSize);
          }
        }
        else
        {
          //
          // We are here if data part is zero.
          //

          if (skip == 1)
          {
            #ifdef TEST
            *logofs << name() << ": Discarding message for OPCODE#"
                    << (unsigned int) opcode() << ".\n"
                    << logofs_flush;
            #endif

            continue;
          }
        }
      }

      int added;

      added = add(message, position, checksumAction, dataAction);

      if (added != position)
      {
        #ifdef PANIC
        *logofs << name() << ": PANIC! Can't store message "
                << "in the cache at position " << position
                << ".\n" << logofs_flush;
        #endif

        cerr << "Error" << ": Can't store message "
             << "in the cache at position " << position
             << ".\n";

        failed = 1;

        break;
      }
      else
      {
        //
        // Replace default value of hits set by add
        // function. Messages read from cache start
        // with a lower bonus than fresh messages
        // inserted.
        //

        message -> hits_ = control -> StoreHitsLoadBonus;

        #ifdef DEBUG
        *logofs << name() << ": Updated last hit of object at " 
                << strMsTimestamp() << " with a bonus of "
                << message -> hits_ << ".\n" << logofs_flush;
        #endif

        resetTemporary();
      }
    }
    else if ((*messages_)[position] != NULL)
    {
      #ifdef TEST
      *logofs << name() << ": Going to remove message at position "
              << position << ".\n" << logofs_flush;
      #endif

      int removed;

      removed = remove(position, checksumAction, dataAction);

      if (removed != position)
      {
        #ifdef PANIC
        *logofs << name() << ": PANIC! Can't remove message from cache "
                << "at position " << position << ".\n"
                << logofs_flush;
        #endif

        cerr << "Error" << ": Can't remove message from cache "
             << "at position " << position << ".\n";

        failed = 1;

        break;
      }
    }
    #ifdef TEST
    else
    {
      *logofs << name() << ": Not loading message at position "
              << position << ".\n" << logofs_flush;
    }
    #endif
  }

  #ifdef WARNING

  if (failed == 1)
  {
    *logofs << name() << ": WARNING! Read from persistent cache file failed.\n"
            << logofs_flush;
  }

  #endif

  delete [] identityBuffer;
  delete [] sizeBuffer;

  return (failed == 0 ? 1 : -1);
}

void MessageStore::storageSize(const Message *message, unsigned int &local,
                                   unsigned int &remote) const
{
  local = remote = storage();

  //
  // Encoding side includes 48 bytes for
  // the map of checksums and 24 bytes
  // of adjustment for total overhead.
  //

  local += MD5_LENGTH + 48 + 24;

  //
  // At decoding side we include size of
  // data part and 24 bytes of adjustment
  // for total overhead.
  //

  if (message -> c_size_ == 0)
  {
    remote += message -> size_ + 24;
  }
  else
  {
    remote += message -> c_size_ + 24;
  }

  //
  // Check if we are the encoding or the
  // decoding side and, if needed, swap
  // the values.
  //

  if (message -> md5_digest_ == NULL)
  {
    unsigned int t = local;

    local = remote;

    remote = t;
  }
}

void MessageStore::printStorageSize()
{
  #ifdef TEST

  *logofs << name() << ": There are "
          << checksums_ -> size() << " checksums in this store "
          << "out of " << cacheSlots << " slots.\n"
          << logofs_flush;

  *logofs << name() << ": Size of this store is "
          << localStorageSize_ << " bytes at local side and "
          << remoteStorageSize_ << " bytes at remote side.\n"
          << logofs_flush;

  *logofs << name() << ": Size of total cache is "
          << totalLocalStorageSize_ << " bytes at local side and "
          << totalRemoteStorageSize_ << " bytes at remote side.\n"
          << logofs_flush;

  #endif
}
