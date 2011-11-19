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

#ifndef Message_H
#define Message_H

#include <X11/Xproto.h>

#include "NXproto.h"

#include "Misc.h"
#include "Control.h"

#include "Types.h"
#include "Timestamp.h"

#include "ActionCache.h"

#include "ActionCacheCompat.h"
#include "PositionCacheCompat.h"

#include "StaticCompressor.h"

//
// Forward class declarations.
//

class ChannelCache;

class EncodeBuffer;
class DecodeBuffer;

class WriteBuffer;

//
// Set the verbosity level.
//

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

//
// Define this to know how many messages
// are allocated and deallocated.
//

#undef  REFERENCES

//
// Set default values.  We limit the maximum
// size of a request to 262144 but we need to
// consider the replies, whose size may be up
// to 4MB.
//

#define MESSAGE_ENABLE_CACHE                        0
#define MESSAGE_ENABLE_DATA                         0
#define MESSAGE_ENABLE_SPLIT                        0
#define MESSAGE_ENABLE_COMPRESS                     0

#define MESSAGE_DATA_LIMIT                          4194304 - 4
#define MESSAGE_DATA_OFFSET                         4

#define MESSAGE_CACHE_SLOTS                         6000
#define MESSAGE_CACHE_THRESHOLD                     50
#define MESSAGE_CACHE_LOWER_THRESHOLD               5

//
// Base message class.
//

class Message
{
  friend class MessageStore;
  friend class RenderExtensionStore;

  public:

  Message()
  {
    hits_  = 0;
    last_  = 0;
    locks_ = 0;

    size_   = 0;
    c_size_ = 0;

    md5_digest_ = NULL;

    #ifdef REFERENCES

    references_++;

    *logofs << "Message: Created new message at "
            << this << " out of " << references_
            << " allocated messages.\n"
            << logofs_flush;

    #endif
  }

  Message(const Message &message)
  {
    size_   = message.size_;
    c_size_ = message.c_size_;
    i_size_ = message.i_size_;

    hits_  = message.hits_;
    last_  = message.last_;
    locks_ = message.locks_;

    data_ = message.data_;

    #ifdef REFERENCES

    references_++;

    *logofs << "Message: Creating new copied message at "
            << this << " out of " << references_
            << " allocated messages.\n"
            << logofs_flush;
    #endif

    if (message.md5_digest_ != NULL)
    {
      md5_digest_ = new md5_byte_t[MD5_LENGTH];

      memcpy(md5_digest_, message.md5_digest_, MD5_LENGTH);

      #ifdef DEBUG
      *logofs << "Message: Created MD5 digest for object at "
              << this << ".\n" << logofs_flush;
      #endif
    }
    else
    {
      md5_digest_ = NULL;
    }
  }

  ~Message()
  {
    #ifdef DEBUG
    if (md5_digest_ != NULL)
    {
      *logofs << "Message: Deleted MD5 digest for object at "
              << this << ".\n" << logofs_flush;
    }
    #endif

    delete [] md5_digest_;

    #ifdef REFERENCES

    references_--;

    *logofs << "Message: Deleted message at "
            << this << " out of " << references_
            << " allocated messages.\n"
            << logofs_flush;
    #endif
  }

  //
  // This is the original message size 
  // including the data part regardless
  // data is still stored in the object.
  //

  int size_;

  //
  // This is the size of the identity.
  //

  int i_size_;

  //
  // This is the size, including identity,
  // after message has been 'updated' to 
  // reflect storage of data in compressed
  // format.
  //

  int c_size_;

  protected:

  //
  // This is the data part.
  //

  T_data data_;

  //
  // Time of last hit.
  //

  time_t last_;

  //
  // This is the number of cache hits 
  // registered for the object.
  //

  short int hits_;

  //
  // This is used to mark messages
  // that have been split. 
  //

  short int locks_;

  //
  // This is the MD5 checksum.
  //

  md5_byte_t *md5_digest_;

  //
  // Keep a reference counter
  // of allocated objects.
  //

  #ifdef REFERENCES

  static int references_;

  #endif
};

//
// Repository of messages.
//

class MessageStore
{
  public:

  //
  // Enable or disable cache of messages in store.
  //

  int enableCache;

  //
  // Does message have a distinct data part.
  //

  int enableData;

  //
  // Enable or disable split of data part.
  //

  int enableSplit;

  //
  // Enable or disable compression of data part.
  //

  int enableCompress;

  //
  // Set starting point of data part in the message.
  //

  int dataOffset;

  //
  // Set maximum size for the data part of each message.
  //

  int dataLimit;

  //
  // Set maximum elements in cache.
  //

  int cacheSlots;

  //
  // Set the percentage of total cache memory which
  // a given type of message is allowed to occupy.
  // When threshold is exceeded store is cleaned to
  // make room for a new message of the same type.
  //

  int cacheThreshold;

  //
  // Don't clean the store if percentage of cache
  // memory occupied by messages of this type is
  // below the threshold.
  //

  int cacheLowerThreshold;

  //
  // Last operation performed on cache.
  //

  T_store_action lastAction;

  //
  // Position of last element stored in cache.
  //

  short int lastAdded;

  //
  // Positions of last element found in cache.
  //

  short int lastHit;

  //
  // Position of last element erased.
  //

  short int lastRemoved;

  //
  // Used to encode the the action to
  // perform on the store and the slot
  // involved.
  //

  ActionCache lastActionCache;

  //
  // Used in old protocol versions.
  //

  ActionCacheCompat lastActionCacheCompat;

  PositionCacheCompat lastAddedCacheCompat;
  PositionCacheCompat lastHitCacheCompat;
  PositionCacheCompat lastRemovedCacheCompat;

  //
  // Position in cache where next insertion
  // is going to take place.
  //

  short int lastRated;

  //
  // Size of data part of last split message
  // once compressed. This is used only for
  // compatibility with older proxies.
  //

  int lastResize;

  //
  // Constructors and destructors.
  //

  public:

  MessageStore(StaticCompressor *compressor = NULL);

  virtual ~MessageStore();

  virtual const char *name() const = 0;

  virtual unsigned char opcode() const = 0;

  virtual unsigned int storage() const = 0;

  //
  // These are members that must be specialized.
  //

  public:

  virtual Message *create() const = 0;

  virtual Message *create(const Message &message) const = 0;

  virtual void destroy(Message *message) const = 0;

  void validateSize(int size)
  {
    if (size < control -> MinimumMessageSize ||
            size > control -> MaximumMessageSize)
    {
      *logofs << name() << ": PANIC! Invalid size " << size
              << " for message.\n" << logofs_flush;

      cerr << "Error" << ": Invalid size " << size
           << " for message opcode " << opcode() << ".\n";

      HandleAbort();
    }
  }

  void validateSize(int dataSize, int compressedDataSize)
  {
    if (dataSize < 0 || dataSize > control ->
            MaximumMessageSize - 4 || compressedDataSize < 0 ||
                compressedDataSize >= dataSize)
    {
      *logofs << name() << ": PANIC! Invalid data size "
              << dataSize << " and compressed data size "
              << compressedDataSize << " for message.\n"
              << logofs_flush;

      cerr << "Error" << ": Invalid data size "
           << dataSize << " and compressed data size "
           << compressedDataSize << " for message "
           << "opcode " << (unsigned) opcode() << ".\n";

      HandleAbort();
    }
  }

  //
  // Determine if the message can be stored
  // in the cache.
  //

  virtual int validateMessage(const unsigned char *buffer, int size)
  {
    return (size >= control -> MinimumMessageSize &&
                size <= control -> MaximumMessageSize);
  }

  //
  // Get data offset based on major and minor
  // opcode of the message.
  //

  virtual int identitySize(const unsigned char *buffer, unsigned int size)
  {
    return dataOffset;
  }

  //
  // Encode identity and data using the
  // specific message encoding.
  //
  // Some messages do not implement these
  // methods because the encoding is done
  // directly in the channel loop. Should
  // move the encoding methods in in the
  // message classes.
  //

  virtual int encodeIdentity(EncodeBuffer &encodeBuffer, const unsigned char *buffer,
                                 unsigned int size, int bigEndian,
                                     ChannelCache *channelCache) const
  {
    return 1;
  }

  virtual int decodeIdentity(DecodeBuffer &decodeBuffer, unsigned char *&buffer,
                                 unsigned int &size, int bigEndian, WriteBuffer *writeBuffer,
                                      ChannelCache *channelCache) const
  {
    return 1;
  }

  //
  // Encode differences between message
  // in cache and the one to be encoded.
  //

  virtual void updateIdentity(EncodeBuffer &encodeBuffer, const Message *message,
                                  const Message *cachedMessage, ChannelCache *channelCache) const
  {
  }

  //
  // Decode differences and update the
  // cached version of the same message.
  //

  virtual void updateIdentity(DecodeBuffer &decodeBuffer, const Message *message,
                                  ChannelCache *channelCache) const
  {
  }

  //
  // Post process the message information
  // contained in the store by either up-
  // dating the size record or the actual
  // data part once the message has been
  // completely sent to our peer.
  //

  void updateData(const int position, unsigned int dataSize,
                      unsigned int compressedDataSize);

  void updateData(const T_checksum checksum, unsigned int compressedDataSize);

  void updateData(const int position, const unsigned char *newData,
                      unsigned int dataSize, unsigned int compressedDataSize);

  //
  // These members, used internally
  // in the message store class, are
  // mandatory.
  //

  protected:

  virtual int parseIdentity(Message *message, const unsigned char *buffer,
                                unsigned int size, int bigEndian) const = 0;

  virtual int unparseIdentity(const Message *message, unsigned char *buffer,
                                  unsigned int size, int bigEndian) const = 0;

  virtual void identityChecksum(const Message *message, const unsigned char *buffer,
                                    unsigned int size, int bigEndian) const = 0;

  virtual void dumpIdentity(const Message *message) const = 0;

  //
  // Design should preserve these from being
  // virtual.
  //

  int parseData(Message *message, int split, const unsigned char *buffer,
                    unsigned int size, T_checksum_action checksumAction,
                        T_data_action dataAction, int bigEndian);

  int parseData(Message *message, const unsigned char *buffer,
                    unsigned int size, const unsigned char *compressedData,
                        const unsigned int compressedDataSize, T_checksum_action checksumAction,
                            T_data_action dataAction, int bigEndian);

  int unparseData(const Message *message, unsigned char *buffer,
                      unsigned int size, int bigEndian);

  //
  // Manage efficient allocation of messages
  // in the heap.
  //

  void recycle(Message *message)
  {
    #ifdef TEST

    if (message == NULL)
    {
      *logofs << name() << ": PANIC! Cannot recycle a null message.\n"
              << logofs_flush;

      cerr << "Error" << ": Cannot recycle a null message.\n";

      HandleAbort();
    }

    #endif

    if (temporary_ == NULL)
    {
      //
      // Be careful when reusing the message as
      // it can contain valid data that must be
      // explicitly deallocated if not needed.
      // Note also that you cannot count on the
      // initialization made in costructor.
      //

      temporary_ = message;
    }
    else
    {
      destroy(message);
    }
  }

  void beginChecksum(Message *message)
  {
    if (message -> md5_digest_ == NULL)
    {
      message -> md5_digest_ = new md5_byte_t[MD5_LENGTH];

      #ifdef DEBUG
      *logofs << name() << ": Created MD5 digest structure "
              << "for object at " << message << ".\n"
              << logofs_flush;
      #endif
    }
    #ifdef DEBUG
    else
    {
      *logofs << name() << ": Using existing MD5 digest structure "
              << "for object at " << message << ".\n"
              << logofs_flush;
    }
    #endif

    #ifdef DEBUG
    *logofs << name() << ": Prepared MD5 digest for object at "
            << message << ".\n" << logofs_flush;
    #endif

    md5_init(md5_state_);
  }

  void endChecksum(Message *message)
  {
    md5_finish(md5_state_, message -> md5_digest_);

    #ifdef DEBUG
    *logofs << name() << ": Calculated checksum for object at "
            << message << ".\n" << logofs_flush;
    #endif
  }

  void dataChecksum(Message *message, const unsigned char *buffer,
                        unsigned int size, int bigEndian)
  {
    //
    // Messages that have a data part starting
    // at an offset possibly beyond the end of
    // the message, must include the size in
    // the identity checksum.
    //

    if ((int) size > message -> i_size_)
    {
      md5_append(md5_state_, buffer + message -> i_size_,
                     size - message -> i_size_);
    }
  }

  //
  // Repository handling methods.
  //

  public:

  //
  // Extract identity and data from buffer.
  // The size field will be updated at the
  // time of data parsing.
  //

  int parse(Message *message, int split, const unsigned char *buffer, unsigned int size,
                T_checksum_action checksumAction, T_data_action dataAction, int bigEndian);

  int parse(Message *message, const unsigned char *buffer, unsigned int size,
                const unsigned char *compressedData, const unsigned int compressedDataSize,
                     T_checksum_action checksumAction, T_data_action dataAction, int bigEndian);

  //
  // From identity and data write the
  // final message to the raw buffer.
  //

  int unparse(const Message *message, unsigned char *buffer,
                  unsigned int size, int bigEndian)
  {
    return (unparseData(message, buffer, size, bigEndian) &&
                unparseIdentity(message, buffer, size, bigEndian));
  }

  void dump(const Message *message) const
  {
    dumpIdentity(message);

    dumpData(message);
  }

  void dumpData(const Message *message) const;

  //
  // This returns the original message size as it
  // was received on the link. It takes in account
  // the data part, regardless data is still stored
  // in the message object. This information will
  // be used at the time message is unparsed.
  //

  int plainSize(const int position) const
  {
    return (*messages_)[position] -> size_;
  }

  //
  // This returns either the size of identity plus
  // the compressed data part or 0 if message is
  // stored in uncompressed format.
  //

  int compressedSize(const int position) const
  {
    return (*messages_)[position] -> c_size_;
  }

  //
  // Returns a pointer to message
  // given its position in cache.
  //

  Message *get(const int position) const
  {
    if (position < 0 || position >= cacheSlots)
    {
      #ifdef PANIC
      *logofs << name() << ": PANIC! Requested position "
              << position << " is not inside the "
              << "container.\n" << logofs_flush;
      #endif

      cerr << "Error" << ": Requested position "
           << position << " is not inside the"
           << "container.\n";

      HandleAbort();
    }
    else if ((*messages_)[position] == NULL)
    {
      #ifdef PANIC
      *logofs << name() << ": PANIC! Message at position "
              << position << " is NULL.\n"
              << logofs_flush;
      #endif

      cerr << "Error" << ": Message at position "
           << position << " is NULL.\n";

      HandleAbort();
    }

    return (*messages_)[position];
  }

  //
  // This is the method called at encoding
  // side to add a message to cache.
  //

  int findOrAdd(Message *message, T_checksum_action checksumAction,
                    T_data_action dataAction, int &added, int &locked);

  //
  // Utility interfaces to message insertion
  // and deletion.
  //

  int add(Message *message, const int position,
              T_checksum_action checksumAction, T_data_action dataAction);

  int remove(const int position, T_checksum_action checksumAction,
                 T_data_action dataAction);

  //
  // Make space in the repository by remove
  // the first suitable message object.
  //

  int clean(T_checksum_action checksumAction);

  //
  // Increase or decrease the "rating" of
  // the message object.
  //

  int touch(Message *message) const;
  int untouch(Message *message) const;

  int getTouches(const int position) const
  {
    Message *message = (*messages_)[position];

    if (message == NULL)
    {
      return 0;
    }

    return message -> hits_;
  }

  //
  // Gives a 'weight' to the cached message. A zero
  // value means object can be safely removed. A value
  // greater than zero means it is advisable to retain
  // the object. A negative result means it is mandato-
  // ry to keep object in cache.
  //

  int getRating(Message *message, T_rating type) const;

  //
  // Increase or decrease locks of message at given
  // position. A locked message will not be removed
  // from the message store until the lock counter
  // is zero.
  //

  int lock(const int position) const;
  int unlock(const int position) const;

  int getLocks(const int position) const
  {
    Message *message = (*messages_)[position];

    if (message == NULL)
    {
      return 0;
    }

    return message -> locks_;
  }

  T_checksum const getChecksum(const int position) const
  {
    return getChecksum(get(position));
  }

  T_checksum const getChecksum(const Message *message) const
  {
    if (message -> md5_digest_ == NULL)
    {
      #ifdef PANIC
      *logofs << name() << ": PANIC! Checksum not initialized "
              << "for object at " << message << ".\n"
              << logofs_flush;
      #endif

      cerr << "Error" << ": Checksum not initialized "
           << "for object at " << message << ".\n";

      HandleAbort();
    }

    #ifdef DEBUG
    *logofs << name() << ": Got checksum for object at " 
            << message << ".\n" << logofs_flush;
    #endif

    return message -> md5_digest_;
  }

  //
  // Calculate the checksum on the fly based the
  // opcode in the buffer. Useful in the case a
  // message was not processed or was not stored
  // in the cache. The returned checksum must be
  // explicitly deallocated by the caller, after
  // use.
  //

  T_checksum getChecksum(const unsigned char *buffer,
                             unsigned int size, int bigEndian);

  const unsigned char *getData(const Message *message) const
  {
    return message -> data_.begin();
  }

  int plainSize(const Message *message) const
  {
    return message -> size_;
  }

  int identitySize(Message *message)
  {
    return message -> i_size_;
  }

  int compressedSize(const Message *message) const
  {
    return message -> c_size_;
  }

  Message *getTemporary()
  {
    if (temporary_ == NULL)
    {
      temporary_ = create();
    }

    return temporary_;
  }

  void resetTemporary()
  {
    temporary_ = NULL;
  }

  //
  // On side where we don't have checksums, we
  // count how many messages are in the array.
  // This is obviously expensive and should be
  // only performed when reporting statistics.
  //

  int getSize() const
  {
    int size = checksums_ -> size();

    if (size == 0)
    {
      for (int i = 0; i < cacheSlots; i++)
      {
        if ((*messages_)[i] != NULL)
        {
          size++;
        }
      }
    }

    return size;
  }

  int getLocalStorageSize() const
  {
    return localStorageSize_;
  }

  int getRemoteStorageSize() const
  {
    return remoteStorageSize_;
  }

  int getLocalTotalStorageSize() const
  {
    return totalLocalStorageSize_;
  }

  int getRemoteTotalStorageSize() const
  {
    return totalRemoteStorageSize_;
  }

  static int getCumulativeTotalStorageSize()
  {
    return (totalLocalStorageSize_ > totalRemoteStorageSize_ ?
                totalLocalStorageSize_ : totalRemoteStorageSize_);
  }

  int saveStore(ostream *cachefs, md5_state_t *md5_state_stream,
                    md5_state_t *md5_state_client, T_checksum_action checksumAction,
                        T_data_action dataAction, int bigEndian);

  int loadStore(istream *cachefs, md5_state_t *md5_state_stream,
                    T_checksum_action checksumAction, T_data_action dataAction,
                        int bigEndian);

  protected:

  //
  // Estimate the memory requirements of given
  // instance of message. Size includes memory
  // allocated from heap to store checksum and
  // data.
  //

  void storageSize(const Message *message, unsigned int &local,
                       unsigned int &remote) const;

  //
  // Just used for debug.
  //

  void printStorageSize();

  //
  // Repositories where to save cached messages.
  // First is a vector of pointers, the second
  // is a hash table used for fast lookups.
  //

  T_messages  *messages_;
  T_checksums *checksums_;

  //
  // A message object to be used as a temporary.
  // Reuse the temporary object if possible, if
  // not, create a new instance.
  //

  Message *temporary_;

  //
  // Used to calculate message's checksum.
  //

  md5_state_t *md5_state_;

  private:

  //
  // Used to compress data payload.
  //

  StaticCompressor *compressor_;

  //
  // Keep track of how many bytes 
  // are taken by cache.
  //

  int localStorageSize_;
  int remoteStorageSize_;

  static int totalLocalStorageSize_;
  static int totalRemoteStorageSize_;

  //
  // Used to track object allocation and deallocation.
  //

  #ifdef REFERENCES

  static int references_;

  #endif
};

//
// This is an ancillary class of the message
// store, used to encode extensions based on
// the minor opcode.
//

class MinorMessageStore
{
  public:

  virtual ~MinorMessageStore()
  {
  }

  virtual const char *name() const = 0;

  virtual int identitySize(const unsigned char *buffer, unsigned int size) = 0;

  virtual int encodeMessage(EncodeBuffer &encodeBuffer, const unsigned char *buffer,
                                const unsigned int size, int bigEndian,
                                    ChannelCache *channelCache) const
  {
    return 1;
  }

  virtual int decodeMessage(DecodeBuffer &decodeBuffer, unsigned char *&buffer,
                                unsigned int &size, unsigned char type, int bigEndian,
                                    WriteBuffer *writeBuffer, ChannelCache *channelCache) const
  {
    return 1;
  }

  virtual void encodeData(EncodeBuffer &encodeBuffer, const unsigned char *buffer,
                              unsigned int size, int bigEndian,
                                  ChannelCache *channelCache) const
  {
  }

  virtual void decodeData(DecodeBuffer &decodeBuffer, unsigned char *buffer,
                              unsigned int size, int bigEndian,
                                  ChannelCache *channelCache) const
  {
  }

  virtual int parseIdentity(Message *message, const unsigned char *buffer,
                                unsigned int size, int bigEndian) const = 0;

  virtual int unparseIdentity(const Message *message, unsigned char *buffer,
                                  unsigned int size, int bigEndian) const = 0;

  virtual void updateIdentity(EncodeBuffer &encodeBuffer, const Message *message,
                                  const Message *cachedMessage,
                                      ChannelCache *channelCache) const
  {
  }

  virtual void updateIdentity(DecodeBuffer &decodeBuffer, const Message *message,
                                  ChannelCache *channelCache) const
  {
  }

  virtual void identityChecksum(const Message *message, const unsigned char *buffer,
                                    unsigned int size, md5_state_t *md5_state,
                                        int bigEndian) const = 0;
};

#endif /* Message_H */

