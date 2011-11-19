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

#include "Channel.h"

#include "List.h"
#include "Proxy.h"
#include "Statistics.h"

#include "StaticCompressor.h"

#include "NXalert.h"

extern Proxy *proxy;

//
// Set the verbosity level.
//

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG
#undef  DUMP

//
// Log the operations related to splits.
//

#undef  SPLIT

#undef  COUNT

#define COUNT_MAJOR_OPCODE     154

#undef  MONITOR

#define MONITOR_MAJOR_OPCODE   154
#define MONITOR_MINOR_OPCODE   23

#undef  CLEAR

#define CLEAR_MAJOR_OPCODE     154
#define CLEAR_MINOR_OPCODE     23

//
// Define this to know how many messages
// are allocated and deallocated.
//

#undef  REFERENCES

//
// Set to the descriptor of the first X
// channel successfully connected.
//

int Channel::firstClient_ = -1;

//
// Port used for font server connections.
//

int Channel::fontPort_ = -1;

//
// This is used for reference count.
//

#ifdef REFERENCES

int Channel::references_ = 0;

#endif

Channel::Channel(Transport *transport, StaticCompressor *compressor)

    : transport_(transport), compressor_(compressor)
{
  fd_ = transport_ -> fd();

  finish_     = 0;
  closing_    = 0;
  drop_       = 0;
  congestion_ = 0;
  priority_   = 0;

  alert_ = 0;

  firstRequest_ = 1;
  firstReply_   = 1;

  enableCache_ = 1;
  enableSplit_ = 1;
  enableSave_  = 1;
  enableLoad_  = 1;

  //
  // Must be set by proxy.
  //

  opcodeStore_ = NULL;
 
  clientStore_ = NULL;
  serverStore_ = NULL;

  clientCache_ = NULL;
  serverCache_ = NULL;

  #ifdef REFERENCES
  *logofs << "Channel: Created new Channel at " 
          << this << " out of " << ++references_ 
          << " allocated references.\n" << logofs_flush;
  #endif
}

Channel::~Channel()
{
  if (firstClient_ == fd_)
  {
    firstClient_ = -1;
  }

  #ifdef REFERENCES
  *logofs << "Channel: Deleted Channel at " 
          << this << " out of " << --references_ 
          << " allocated references.\n" << logofs_flush;
  #endif
}

int Channel::handleEncode(EncodeBuffer &encodeBuffer, ChannelCache *channelCache,
                              MessageStore *store, const unsigned char opcode,
                                  const unsigned char *buffer, const unsigned int size)
{
  #ifdef MONITOR

  static float totalMessages = 0;
  static float totalBits     = 0;

  int bits;
  int diff;

  bits = encodeBuffer.getBits();

  #endif

  //
  // Check if message can be differentially
  // encoded using a similar message in the
  // message store.
  // 

  #ifdef COUNT

  if (*(buffer) == COUNT_MAJOR_OPCODE)
  {
    if (*(buffer) < 128)
    {
      *logofs << "handleEncode: Handling OPCODE#" << (unsigned int) *(buffer)
              << ".\n" << logofs_flush;
    }
    else
    {
      *logofs << "handleEncode: Handling OPCODE#" << (unsigned int) *(buffer)
              << " MINOR#" << (unsigned int) *(buffer + 1) << ".\n"
              << logofs_flush;
    }
  }

  #endif

  #ifdef CLEAR

  if (*(buffer) == CLEAR_MAJOR_OPCODE &&
          (CLEAR_MINOR_OPCODE == -1 || *(buffer + 1) == CLEAR_MINOR_OPCODE))
  {
    *((unsigned char *) buffer) = X_NoOperation;

    *((unsigned char *) buffer + 1) = '\0';

    CleanData((unsigned char *) buffer + 4, size - 4);
  }

  #endif

  if (handleEncodeCached(encodeBuffer, channelCache,
                             store, buffer, size) == 1)
  {
    #ifdef MONITOR

    diff = encodeBuffer.getBits() - bits;

    if (*(buffer) == MONITOR_MAJOR_OPCODE &&
            (MONITOR_MINOR_OPCODE == -1 || *(buffer + 1) == MONITOR_MINOR_OPCODE))
    {
      totalMessages++;

      totalBits += diff;

      *logofs << "handleEncode: Handled cached OPCODE#" << (unsigned int) *(buffer)
              << " MINOR#" << (unsigned int) *(buffer + 1) << ". " << size
              << " bytes in, " << diff << " bits (" << ((float) diff) / 8
              << " bytes) out. Average " << totalBits / totalMessages
              << "/1.\n" << logofs_flush;
    }

    #endif

    //
    // Let the channel update the split store
    // and notify the agent in the case of a
    // cache hit.
    //

    if (store -> enableSplit)
    {
      handleSplit(encodeBuffer, store, store -> lastAction,
                      store -> lastHit, opcode, buffer, size);
    }

    return 1;
  }

  //
  // A similar message could not be found in
  // cache or message must be discarded. Must
  // transmit the message using the field by
  // field differential encoding.
  //

  handleEncodeIdentity(encodeBuffer, channelCache,
                           store, buffer, size, bigEndian_);

  //
  // Check if message has a distinct data part.
  //

  if (store -> enableData)
  {
    //
    // If message split was requested by agent then send data
    // out-of-band, dividing it in small chunks. Until message
    // is completely transferred, keep in the split store a
    // dummy version of the message, with data replaced with a
    // pattern.
    //
    // While data is being transferred, agent should have put
    // the resource (for example its client) asleep. It can
    // happen, though, that a different client would reference
    // the same message. We cannot issue a cache hit for images
    // being split (such images are put in store in 'incomplete'
    // state), so we need to handle this case.
    //

    if (store -> enableSplit == 1)
    {
      //
      // Let the channel decide what to do with the
      // message. If the split can't take place be-
      // cause the split store is full, the channel
      // will tell the remote side that the data is
      // going to follow.
      //

      if (handleSplit(encodeBuffer, store, store -> lastAction,
                          (store -> lastAction == IS_ADDED ? store -> lastAdded : 0),
                              opcode, buffer, size) == 1)
      {
        #ifdef MONITOR

        diff = encodeBuffer.getBits() - bits;

        if (*(buffer) == MONITOR_MAJOR_OPCODE &&
                (MONITOR_MINOR_OPCODE == -1 || *(buffer + 1) == MONITOR_MINOR_OPCODE))
        {
          totalMessages++;

          totalBits += diff;

          *logofs << "handleEncode: Handled split OPCODE#" << (unsigned int) *(buffer)
                  << " MINOR#" << (unsigned int) *(buffer + 1) << ". " << size
                  << " bytes in, " << diff << " bits (" << ((float) diff) / 8
                  << " bytes) out. Average " << totalBits / totalMessages
                  << "/1.\n" << logofs_flush;
        }

        #endif

        return 0;
      }
    }

    //
    // The split did not take place and we are going
    // to transfer the data part. Check if the static
    // compression of the data section is enabled.
    // This is the case of all messages not having a
    // special differential encoding or messages that
    // we want to store in cache in compressed form.
    //

    unsigned int offset = store -> identitySize(buffer, size);

    if (store -> enableCompress)
    {
      unsigned char *data   = NULL;
      unsigned int dataSize = 0;

      int compressed = handleCompress(encodeBuffer, opcode, offset,
                                          buffer, size, data, dataSize);
      if (compressed < 0)
      {
        return -1;
      }
      else if (compressed > 0)
      {
        //
        // Update the size of the message according
        // to the result of the data compression.
        //

        handleUpdate(store, size - offset, dataSize);
      }
    }
    else
    {
      handleCopy(encodeBuffer, opcode, offset, buffer, size);
    }
  }

  #ifdef MONITOR

  diff = encodeBuffer.getBits() - bits;

  if (*(buffer) == MONITOR_MAJOR_OPCODE &&
          (MONITOR_MINOR_OPCODE == -1 || *(buffer + 1) == MONITOR_MINOR_OPCODE))
  {
    totalMessages++;

    totalBits += diff;

    *logofs << "handleEncode: Handled OPCODE#" << (unsigned int) *(buffer)
            << " MINOR#" << (unsigned int) *(buffer + 1) << ". " << size
            << " bytes in, " << diff << " bits (" << ((float) diff) / 8
            << " bytes) out. Average " << totalBits / totalMessages
            << "/1.\n" << logofs_flush;
  }

  #endif

  return 0;
}

int Channel::handleDecode(DecodeBuffer &decodeBuffer, ChannelCache *channelCache,
                              MessageStore *store, unsigned char &opcode,
                                  unsigned char *&buffer, unsigned int &size)
{
  //
  // Check first if the message is in the
  // message store.
  //

  unsigned int split = 0;

  if (handleDecodeCached(decodeBuffer, channelCache,
                             store, buffer, size) == 1)
  {
    //
    // Let the channel update the split store
    // in the case of a message being cached.
    //

    if (store -> enableSplit == 1)
    {
      if (control -> isProtoStep7() == 1)
      {
        #ifdef DEBUG
        *logofs << "handleDecode: " << store -> name() 
                << ": Checking if the message was split.\n" 
                << logofs_flush;
        #endif

        decodeBuffer.decodeBoolValue(split);
      }

      if (split == 1)
      {
        handleSplit(decodeBuffer, store, store -> lastAction,
                        store -> lastHit, opcode, buffer, size);

        handleCleanAndNullRequest(opcode, buffer, size);
      }
    }

    return 1;
  }

  //
  // Decode the full identity.
  //

  handleDecodeIdentity(decodeBuffer, channelCache, store, buffer,
                           size, bigEndian_, &writeBuffer_);

  //
  // Check if the message has a distinct
  // data part.
  //

  if (store -> enableData)
  {
    //
    // Check if message has been split.
    //

    if (store -> enableSplit)
    {
      #ifdef DEBUG
      *logofs << "handleDecode: " << store -> name() 
              << ": Checking if the message was split.\n" 
              << logofs_flush;
      #endif
      
      decodeBuffer.decodeBoolValue(split);

      if (split == 1)
      {
        //
        // If the message was added to the store,
        // create the entry without the data part.
        //

        handleSaveSplit(store, buffer, size);

        handleSplit(decodeBuffer, store, store -> lastAction,
                        (store -> lastAction == IS_ADDED ? store -> lastAdded : 0),
                            opcode, buffer, size);

        handleCleanAndNullRequest(opcode, buffer, size);

        return 0;
      }
    }

    //
    // Decode the data part.
    //

    unsigned int offset = store -> identitySize(buffer, size);

    if (store -> enableCompress)
    {
      const unsigned char *data = NULL;
      unsigned int dataSize = 0;

      int decompressed = handleDecompress(decodeBuffer, opcode, offset,
                                              buffer, size, data, dataSize);
      if (decompressed < 0)
      {
        return -1;
      }
      else if (decompressed > 0)
      {
        //
        // The message has been transferred
        // in compressed format.
        //

        handleSave(store, buffer, size, data, dataSize);

        if (store -> enableSplit)
        {
          if (split == 1)
          {
            handleSplit(decodeBuffer, store, store -> lastAction,
                            (store -> lastAction == IS_ADDED ? store -> lastAdded : 0),
                                opcode, buffer, size);

            handleCleanAndNullRequest(opcode, buffer, size);
          }
        }

        return 0;
      }
    }
    else
    {
      //
      // Static compression of the data part
      // was not enabled for this message.
      //

      handleCopy(decodeBuffer, opcode, offset, buffer, size);
    }
  }

  //
  // The message doesn't have a data part
  // or the data was not compressed.
  //

  handleSave(store, buffer, size);

  if (store -> enableSplit)
  {
    if (split == 1)
    {
      handleSplit(decodeBuffer, store, store -> lastAction,
                      (store -> lastAction == IS_ADDED ? store -> lastAdded : 0),
                          opcode, buffer, size);

      handleCleanAndNullRequest(opcode, buffer, size);
    }
  }

  return 0;
}

int Channel::handleEncodeCached(EncodeBuffer &encodeBuffer, ChannelCache *channelCache,
                                    MessageStore *store,  const unsigned char *buffer,
                                        const unsigned int size)
{
  if (control -> LocalDeltaCompression == 0 ||
          enableCache_ ==  0 || store -> enableCache == 0)
  {
    if (control -> isProtoStep7() == 1)
    {
      encodeBuffer.encodeActionValue(is_discarded,
                         store -> lastActionCache);
    }
    else
    {
      encodeBuffer.encodeActionValueCompat(is_discarded,
                         store -> lastActionCacheCompat);
    }

    store -> lastAction = is_discarded;

    return 0;
  }

  #ifdef DEBUG
  *logofs << "handleEncodeCached: " << store -> name() 
          << ": Going to handle a new message of this class.\n" 
          << logofs_flush;
  #endif

  //
  // Check if the estimated size of cache is greater
  // than the requested limit. If it is the case make
  // some room by deleting one or more messages.
  //

  int position;

  while (mustCleanStore(store) == 1 && canCleanStore(store) == 1)
  {
    #ifdef DEBUG
    *logofs << "handleEncodeCached: " << store -> name() 
            << ": Trying to reduce size of message store.\n"
            << logofs_flush;
    #endif

    position = store -> clean(use_checksum);

    if (position == nothing)
    {
      #ifdef TEST
      *logofs << "handleEncodeCached: " << store -> name() 
              << ": WARNING! No message found to be "
              << "actually removed.\n" << logofs_flush;
      #endif

      break;
    }

    #ifdef DEBUG
    *logofs << "handleEncodeCached: " << store -> name() 
            << ": Message at position " << position
            << " will be removed.\n" << logofs_flush;
    #endif

    //
    // Encode the position of message to
    // be discarded.
    //

    store -> lastRemoved = position;

    if (control -> isProtoStep7() == 1)
    {
      encodeBuffer.encodeActionValue(is_removed, store -> lastRemoved,
                         store -> lastActionCache);
    }
    else
    {
      encodeBuffer.encodeActionValueCompat(is_removed,
                         store -> lastActionCacheCompat);

      encodeBuffer.encodePositionValueCompat(store -> lastRemoved,
                         store -> lastRemovedCacheCompat);
    }

    #ifdef DEBUG
    *logofs << "handleEncodeCached: " << store -> name() << ": Going to "
            << "clean up message at position " << position << ".\n" 
            << logofs_flush;
    #endif

    store -> remove(position, use_checksum, discard_data);

    #ifdef DEBUG
    *logofs << "handleEncodeCached: " << store -> name() << ": There are " 
            << store -> getSize() << " messages in the store out of " 
            << store -> cacheSlots << " slots.\n" << logofs_flush;

    *logofs << "handleEncodeCached: " << store -> name() 
            << ": Size of store is " << store -> getLocalStorageSize() 
            << " bytes locally and " << store -> getRemoteStorageSize() 
            << " bytes remotely.\n" << logofs_flush;

    *logofs << "handleEncodeCached: " << store -> name() 
            << ": Size of total cache is " << store -> getLocalTotalStorageSize()
            << " bytes locally and " << store -> getRemoteTotalStorageSize()
            << " bytes remotely.\n" << logofs_flush;
    #endif
  }

  #ifdef DEBUG

  if (mustCleanStore(store) == 1 && canCleanStore(store) == 0)
  {
    *logofs << "handleEncodeCached: " << store -> name() 
            << ": Store would need a clean but operation will be delayed.\n" 
            << logofs_flush;

    *logofs << "handleEncodeCached: " << store -> name() << ": There are " 
            << store -> getSize() << " messages in the store out of " 
            << store -> cacheSlots << " slots.\n"  << logofs_flush;

    *logofs << "handleEncodeCached: " << store -> name() 
            << ": Size of store is " << store -> getLocalStorageSize() 
            << " bytes locally and " << store -> getRemoteStorageSize() 
            << " bytes remotely.\n" << logofs_flush;

    *logofs << "handleEncodeCached: " << store -> name() 
            << ": Size of total cache is " << store -> getLocalTotalStorageSize()
            << " bytes locally and " << store -> getRemoteTotalStorageSize()
            << " bytes remotely.\n" << logofs_flush;
  }

  #endif

  //
  // If 'on the wire' size of message exceeds the
  // allowed limit then avoid to store it in the
  // cache.
  //

  if (store -> validateMessage(buffer, size) == 0)
  {
    #ifdef TEST
    *logofs << "handleEncodeCached: " << store -> name() 
            << ": Message with size " << size << " ignored.\n" 
            << logofs_flush;
    #endif

    if (control -> isProtoStep7() == 1)
    {
      encodeBuffer.encodeActionValue(is_discarded,
                         store -> lastActionCache);
    }
    else
    {
      encodeBuffer.encodeActionValueCompat(is_discarded,
                         store -> lastActionCacheCompat);
    }

    store -> lastAction = is_discarded;

    return 0;
  }

  //
  // Fill the message object with the
  // received data.
  //

  Message *message = store -> getTemporary();

  if (message == NULL)
  {
    #ifdef PANIC
    *logofs << "handleEncodeCached: " << store -> name()
            << ": PANIC! Can't allocate memory for "
            << "a new message.\n" << logofs_flush;
    #endif

    cerr << "Error" << ": Can't allocate memory for "
         << "a new message in context [D].\n";

    HandleCleanup();
  }

  //
  // As we are at encoding side, it is enough to store the
  // checksum for the object while data can be erased. Both
  // the identity and the data will never be sent through
  // the wire again as long as they are stored in the cache
  // at the decoding side. The split parameter is always
  // set to 0 as the data will not be stored in any case.
  //

  store -> parse(message, 0, buffer, size, use_checksum,
                     discard_data, bigEndian_);

  #ifdef DUMP

  store -> dump(message);

  #endif

  //
  // Search the object in the message
  // store. If found get the position.
  //

  #ifdef DEBUG
  *logofs << "handleEncodeCached: " << store -> name() 
          << ": Searching object of size " << size
          << " in the cache.\n" << logofs_flush;
  #endif

  int added;
  int locked;

  position = store -> findOrAdd(message, use_checksum,
                                    discard_data, added, locked);

  if (position == nothing)
  {
    #ifdef WARNING
    *logofs << "handleEncodeCached: " << store -> name() 
            << ": WARNING! Can't store object in the cache.\n"
            << logofs_flush;
    #endif

    if (control -> isProtoStep7() == 1)
    {
      encodeBuffer.encodeActionValue(is_discarded,
                         store -> lastActionCache);
    }
    else
    {
      encodeBuffer.encodeActionValueCompat(is_discarded,
                         store -> lastActionCacheCompat);
    }

    store -> lastAction = is_discarded;

    return 0;
  }
  else if (locked == 1)
  {
    //
    // We can't issue a cache hit. Encoding identity
    // differences while message it's being split
    // would later result in agent to commit a wrong
    // version of message.
    //

    #ifdef WARNING
    *logofs << "handleEncodeCached: " << store -> name() 
            << ": WARNING! Message of size " << store -> plainSize(position)
            << " at position " << position << " is locked.\n"
            << logofs_flush;
    #endif

    cerr << "Warning" << ": Message of size " << store -> plainSize(position)
         << " at position " << position << " is locked.\n";

    if (control -> isProtoStep7() == 1)
    {
      encodeBuffer.encodeActionValue(is_discarded,
                         store -> lastActionCache);
    }
    else
    {
      encodeBuffer.encodeActionValueCompat(is_discarded,
                         store -> lastActionCacheCompat);
    }

    store -> lastAction = is_discarded;

    return 0;
  }
  else if (added == 1)
  {
    store -> resetTemporary();

    #ifdef DEBUG
    *logofs << "handleEncodeCached: " << store -> name() << ": Message of size "
            << store -> plainSize(position) << " has been stored at position "
            << position << ".\n" << logofs_flush;

    *logofs << "handleEncodeCached: " << store -> name() << ": There are " 
            << store -> getSize() << " messages in the store out of " 
            << store -> cacheSlots << " slots.\n" << logofs_flush;

    *logofs << "handleEncodeCached: " << store -> name() 
            << ": Size of store is " << store -> getLocalStorageSize() 
            << " bytes locally and " << store -> getRemoteStorageSize() 
            << " bytes remotely.\n" << logofs_flush;

    *logofs << "handleEncodeCached: " << store -> name() 
            << ": Size of total cache is " << store -> getLocalTotalStorageSize()
            << " bytes locally and " << store -> getRemoteTotalStorageSize()
            << " bytes remotely.\n" << logofs_flush;
    #endif

    //
    // Inform the decoding side that message 
    // must be inserted in cache and encode
    // the position where the insertion took
    // place.
    //

    store -> lastAction = IS_ADDED;

    store -> lastAdded = position;

    if (control -> isProtoStep7() == 1)
    {
      encodeBuffer.encodeActionValue(IS_ADDED, store -> lastAdded,
                         store -> lastActionCache);

    }
    else
    {
      encodeBuffer.encodeActionValueCompat(IS_ADDED,
                         store -> lastActionCacheCompat);

      encodeBuffer.encodePositionValueCompat(store -> lastAdded,
                         store -> lastAddedCacheCompat);
    }

    return 0;
  }
  else
  {
    #ifdef DEBUG
    *logofs << "handleEncodeCached: " << store -> name()
            << ": Cache hit. Found object at position " 
            << position << ".\n" << logofs_flush;
    #endif

    //
    // Must abort the connection if the
    // the position is invalid.
    //

    Message *cachedMessage = store -> get(position);

    //
    // Increase the rating of the cached
    // message.
    //

    store -> touch(cachedMessage);

    #ifdef DEBUG
    *logofs << "handleEncodeCached: " << store -> name() << ": Hits for "
            << "object at position " << position << " are now " 
            << store -> getTouches(position) << ".\n"
            << logofs_flush;
    #endif

    //
    // Send to the decoding side position
    // where object can be found in cache.
    //

    store -> lastAction = IS_HIT;

    store -> lastHit = position;

    if (control -> isProtoStep7() == 1)
    {
      encodeBuffer.encodeActionValue(IS_HIT, store -> lastHit,
                         store -> lastActionCache);
    }
    else
    {
      encodeBuffer.encodeActionValueCompat(IS_HIT,
                         store -> lastActionCacheCompat);

      encodeBuffer.encodePositionValueCompat(store -> lastHit,
                         store -> lastHitCacheCompat);
    }

    //
    // Send the field by field differences in
    // respect to the original message stored
    // in cache.
    //

    store -> updateIdentity(encodeBuffer, message, cachedMessage, channelCache);

    return 1;
  }
}

void Channel::handleUpdateAdded(MessageStore *store, unsigned int dataSize,
                                    unsigned int compressedDataSize)
{
  #ifdef TEST

  if (store -> lastAction != IS_ADDED)
  {
    #ifdef PANIC
    *logofs << "handleUpdateAdded: " << store -> name()
            << ": PANIC! Function called for action '"
            << store -> lastAction << "'.\n"
            << logofs_flush;
    #endif

    cerr << "Error" << ": Update function called for "
         << "store '" << store -> name() << "' with "
         << "action '" << store -> lastAction
         << "'.\n";

    HandleCleanup();
  }

  #endif

  #ifdef DEBUG
  *logofs << "handleUpdateAdded: " << store -> name() << ": Updating "
          << "object at position " << store -> lastAdded << " of size "
          << store -> plainSize(store -> lastAdded) << " (" << dataSize
          << "/" << compressedDataSize << ").\n" << logofs_flush;
  #endif

  store -> updateData(store -> lastAdded, dataSize, compressedDataSize);

  #ifdef DEBUG
  *logofs << "handleUpdateAdded: " << store -> name() << ": There are " 
          << store -> getSize() << " messages in the store out of " 
          << store -> cacheSlots << " slots.\n" << logofs_flush;

  *logofs << "handleUpdateAdded: " << store -> name() 
          << ": Size of store is " << store -> getLocalStorageSize() 
          << " bytes locally and " << store -> getRemoteStorageSize() 
          << " bytes remotely.\n" << logofs_flush;

  *logofs << "handleUpdateAdded: " << store -> name() 
          << ": Size of total cache is " << store -> getLocalTotalStorageSize()
          << " bytes locally and " << store -> getRemoteTotalStorageSize()
          << " bytes remotely.\n" << logofs_flush;
  #endif
}

int Channel::handleDecodeCached(DecodeBuffer &decodeBuffer, ChannelCache *channelCache,
                                    MessageStore *store, unsigned char *&buffer,
                                        unsigned int &size)
{
  //
  // Create a new message object and 
  // fill it with received data.
  //

  #ifdef DEBUG
  *logofs << "handleDecodeCached: " << store -> name() 
          << ": Going to handle a new message of this class.\n" 
          << logofs_flush;
  #endif

  //
  // Decode bits telling how to handle
  // this message.
  //

  unsigned char action;
  unsigned short int position;

  if (control -> isProtoStep7() == 1)
  {
    decodeBuffer.decodeActionValue(action, position,
                       store -> lastActionCache);
  }
  else
  {
    decodeBuffer.decodeActionValueCompat(action,
                       store -> lastActionCacheCompat);
  }

  //
  // Clean operations must always come 
  // before any operation on message.
  //

  while (action == is_removed)
  {
    if (control -> isProtoStep7() == 1)
    {
      store -> lastRemoved = position;
    }
    else
    {
      decodeBuffer.decodePositionValueCompat(store -> lastRemoved,
                         store -> lastRemovedCacheCompat);
    }

    #ifdef DEBUG

    if (store -> get(store -> lastRemoved))
    {
      *logofs << "handleDecodeCached: " << store -> name() << ": Cleaning up "
              << "object at position " << store -> lastRemoved
              << " of size " << store -> plainSize(store -> lastRemoved)
              << " (" << store -> plainSize(store -> lastRemoved) << "/"
              << store -> compressedSize(store -> lastRemoved) << ").\n"
              << logofs_flush;
    }

    #endif

    //
    // If the message can't be found we
    // will abort the connection.
    //

    store -> remove(store -> lastRemoved, discard_checksum, use_data);

    if (control -> isProtoStep7() == 1)
    {
      decodeBuffer.decodeActionValue(action, position,
                         store -> lastActionCache);
    }
    else
    {
      decodeBuffer.decodeActionValueCompat(action,
                         store -> lastActionCacheCompat);
    }
  }

  //
  // If it's a cache hit, the position
  // where object can be found follows.
  //

  if ((T_store_action) action == IS_HIT)
  {
    if (control -> isProtoStep7() == 1)
    {
      store -> lastHit = position;
    }
    else
    {
      decodeBuffer.decodePositionValueCompat(store -> lastHit,
                         store -> lastHitCacheCompat);
    }

    //
    // Get data from the cache at given position.
    //

    #ifdef DEBUG

    if (store -> get(store -> lastHit))
    {
      *logofs << "handleDecodeCached: " << store -> name() << ": Retrieving "
              << "object at position " << store -> lastHit
              << " of size " << store -> plainSize(store -> lastHit)
              << " (" << store -> plainSize(store -> lastHit)  << "/"
              << store -> compressedSize(store -> lastHit) << ").\n"
              << logofs_flush;
    }

    #endif

    //
    // Must abort the connection if the
    // the position is invalid.
    //

    Message *message = store -> get(store -> lastHit);

    //
    // Make room for the outgoing message.
    //

    size = store -> plainSize(store -> lastHit);

    buffer = writeBuffer_.addMessage(size);

    #ifdef DEBUG
    *logofs << "handleDecodeCached: " << store -> name() 
            << ": Prepared an outgoing buffer of " 
            << size << " bytes.\n" << logofs_flush;
    #endif

    //
    // Decode the variant part. Pass client
    // or server cache to the message store.
    //

    store -> updateIdentity(decodeBuffer, message, channelCache);

    //
    // Write each field in the outgoing buffer.
    //

    store -> unparse(message, buffer, size, bigEndian_);

    #ifdef DUMP

    store -> dump(message);

    #endif

    store -> lastAction = IS_HIT;

    return 1;
  }
  else if ((T_store_action) action == IS_ADDED)
  {
    if (control -> isProtoStep7() == 1)
    {
      store -> lastAdded = position;
    }
    else
    {
      decodeBuffer.decodePositionValueCompat(store -> lastAdded,
                         store -> lastAddedCacheCompat);
    }

    #ifdef DEBUG
    *logofs << "handleDecodeCached: " << store -> name() 
            << ": Message will be later stored at position "
             << store -> lastAdded << ".\n" << logofs_flush;
    #endif

    store -> lastAction = IS_ADDED;

    return 0;
  }
  else
  {
    #ifdef DEBUG
    *logofs << "handleDecodeCached: " << store -> name() 
            << ": Message will be later discarded.\n"
            << logofs_flush;
    #endif

    store -> lastAction = is_discarded;

    return 0;
  }
}

void Channel::handleSaveAdded(MessageStore *store, int split, unsigned char *buffer,
                                  unsigned int size, const unsigned char *compressedData,
                                      const unsigned int compressedDataSize)
{
  #ifdef TEST

  if (store -> lastAction != IS_ADDED)
  {
    #ifdef PANIC
    *logofs << "handleSaveAdded: " << store -> name()
            << ": PANIC! Function called for action '"
            << store -> lastAction << "'.\n"
            << logofs_flush;
    #endif

    cerr << "Error" << ": Save function called for "
         << "store '" << store -> name() << "' with "
         << "action '" << store -> lastAction
         << "'.\n";

    HandleCleanup();
  }

  #endif

  Message *message = store -> getTemporary();

  if (message == NULL)
  {
    #ifdef PANIC
    *logofs << "handleSaveAdded: " << store -> name()
            << ": PANIC! Can't access temporary storage "
            << "for message at position " << store -> lastAdded
            << ".\n" << logofs_flush;
    #endif

    cerr << "Error" << ": Can't access temporary storage "
         << "for message  at position " << store -> lastAdded
         << ".\n";

    HandleCleanup();
  }

  if (compressedData == NULL)
  {
    //
    // If the data part has been split
    // avoid to copy it into the message.
    //

    store -> parse(message, split, buffer, size, discard_checksum,
                       use_data, bigEndian_);
  }
  else
  {
    store -> parse(message, buffer, size, compressedData, 
                       compressedDataSize, discard_checksum,
                           use_data, bigEndian_);
  }

  if (store -> add(message, store -> lastAdded,
          discard_checksum, use_data) == nothing)
  {
    #ifdef PANIC
    *logofs << "handleSaveAdded: " << store -> name()
            << ": PANIC! Can't store message in the cache "
            << "at position " << store -> lastAdded << ".\n"
            << logofs_flush;
    #endif

    cerr << "Error" << ": Can't store message of type "
         << store -> name() << "in the cache at position "
         << store -> lastAdded << ".\n";

    HandleCleanup();
  }
  else
  {
    store -> resetTemporary();

    #ifdef DEBUG
    *logofs << "handleSaveAdded: " << store -> name() << ": Stored "
            << (compressedData == NULL ? "plain" : "compressed")
            << " object at position " << store -> lastAdded
            << " of size " << store -> plainSize(store -> lastAdded)
            << " (" << store -> plainSize(store -> lastAdded) << "/"
            << store -> compressedSize(store -> lastAdded) << ").\n"
            << logofs_flush;
    #endif
  }

  #ifdef DEBUG
  *logofs << "handleSaveAdded: " << store -> name()
          << ": Size of store is " << store -> getLocalStorageSize()
          << " bytes locally and " << store -> getRemoteStorageSize()
          << " bytes remotely.\n" << logofs_flush;

  *logofs << "handleSaveAdded: " << store -> name()
          << ": Size of total cache is " << store -> getLocalTotalStorageSize()
          << " bytes locally and " << store -> getRemoteTotalStorageSize()
          << " bytes remotely.\n" << logofs_flush;
  #endif
}

int Channel::handleWait(int timeout)
{
  #ifdef TEST
  *logofs << "handleWait: Going to wait for more data "
          << "on FD#" << fd_ << " at " << strMsTimestamp()
          << ".\n" << logofs_flush;
  #endif

  T_timestamp startTs = getNewTimestamp();

  T_timestamp nowTs = startTs;

  int readable;
  int remaining;

  for (;;)
  {
    remaining = timeout - diffTimestamp(startTs, nowTs);

    if (transport_ -> blocked() == 1)
    {
      #ifdef WARNING
      *logofs << "handleWait: WARNING! Having to drain with "
              << "channel " << "for FD#" << fd_ << " blocked.\n"
              << logofs_flush;
      #endif

      handleDrain(0, remaining);

      continue;
    }

    if (remaining <= 0)
    {
      #ifdef TEST
      *logofs << "handleWait: Timeout raised while waiting "
              << "for more data for FD#" << fd_ << " at "
              << strMsTimestamp() << ".\n"
              << logofs_flush;
      #endif

      return 0;
    }

    #ifdef TEST
    *logofs << "handleWait: Waiting " << remaining << " Ms "
            << "for a new message on  FD#" << fd_
            << ".\n" << logofs_flush;
    #endif

    readable = transport_ -> wait(remaining);

    if (readable > 0)
    {
      #ifdef TEST
      *logofs << "handleWait: WARNING! Encoding more data "
              << "for FD#" << fd_ << " at " << strMsTimestamp()
              << ".\n" << logofs_flush;
      #endif

      if (proxy -> handleAsyncRead(fd_) < 0)
      {
        return -1;
      }

      return 1;
    }
    else if (readable == -1)
    {
      return -1;
    }

    nowTs = getNewTimestamp();
  }
}

int Channel::handleDrain(int limit, int timeout)
{
  #ifdef TEST
  *logofs << "handleDrain: Going to drain FD#" << fd_
          << " with a limit of " << limit << " bytes "
          << "at " << strMsTimestamp() << ".\n"
          << logofs_flush;
  #endif

  T_timestamp startTs = getNewTimestamp();

  T_timestamp nowTs = startTs;

  int drained;
  int remaining;

  int result;

  for (;;)
  {
    remaining = timeout - diffTimestamp(startTs, nowTs);

    if (remaining <= 0)
    {
      #ifdef TEST
      *logofs << "handleDrain: Timeout raised while draining "
              << "FD#" << fd_ << " at " << strMsTimestamp()
              << ".\n" << logofs_flush;
      #endif

      result = 0;

      goto ChannelDrainEnd;
    }

    #ifdef TEST
    *logofs << "handleDrain: Trying to write to FD#"
            << fd_ << " with " << remaining << " Ms "
            << "remaining.\n" << logofs_flush;
    #endif

    drained = transport_ -> drain(limit, remaining);

    if (drained == 1)
    {
      #ifdef TEST
      *logofs << "handleDrain: Transport for FD#" << fd_
              << " drained to " << transport_ -> length()
              << " bytes at " << strMsTimestamp() << ".\n"
              << logofs_flush;
      #endif

      result = 1;

      goto ChannelDrainEnd;
    }
    else if (drained == 0 && transport_ -> readable() > 0)
    {
      #ifdef TEST
      *logofs << "handleDrain: WARNING! Encoding more data "
              << "for FD#" << fd_ << " at " << strMsTimestamp()
              << ".\n" << logofs_flush;
      #endif

      if (proxy -> handleAsyncRead(fd_) < 0)
      {
        goto ChannelDrainError;
      }
    }
    else if (drained == -1)
    {
      goto ChannelDrainError;
    }

    nowTs = getNewTimestamp();

    if (diffTimestamp(startTs, nowTs) >= control -> ChannelTimeout)
    {
      int seconds = (remaining + control -> LatencyTimeout * 10) / 1000;

      #ifdef WARNING
      *logofs << "handleDrain: WARNING! Could not drain FD#"
              << fd_ << " within " << seconds << " seconds.\n"
              << logofs_flush;
      #endif

      cerr << "Warning" << ": Can't write to connection on FD#"
           << fd_ << " since " << seconds << " seconds.\n";

      if (alert_ == 0)
      {
        if (control -> ProxyMode == proxy_client)
        {
          alert_ = CLOSE_DEAD_X_CONNECTION_CLIENT_ALERT;
        }
        else
        {
          alert_ = CLOSE_DEAD_X_CONNECTION_SERVER_ALERT;
        }

        HandleAlert(alert_, 1);
      }
    }
  }

ChannelDrainEnd:

  //
  // Maybe we drained the channel and are
  // now out of the congestion state.
  //

  handleCongestion();

  return result;

ChannelDrainError:

  finish_ = 1;

  return -1;
}

int Channel::handleCongestion()
{
  //
  // Send a begin congestion control code
  // if the local end of the channel does
  // not consume its data.
  //

  if (isCongested() == 1)
  {
    if (congestion_ == 0)
    {
      #if defined(TEST) || defined(INFO)
      *logofs << "handleCongestion: Sending congestion for FD#"
              << fd_ << " with length " << transport_ -> length()
              << " at " << strMsTimestamp() << ".\n"
              << logofs_flush;
      #endif

      congestion_ = 1;

      //
      // Use the callback to send the control
      // code immediately.
      //

      if (proxy -> handleAsyncCongestion(fd_) < 0)
      {
        finish_ = 1;

        return -1;
      }
    }
  }
  else
  {
    //
    // If the channel was in congestion state
    // send an end congestion control code.
    //

    if (congestion_ == 1)
    {
      #if defined(TEST) || defined(INFO)
      *logofs << "handleCongestion: Sending decongestion for FD#"
              << fd_ << " with length " << transport_ -> length()
              << " at " << strMsTimestamp() << ".\n"
              << logofs_flush;
      #endif

      congestion_ = 0;

      if (proxy -> handleAsyncDecongestion(fd_) < 0)
      {
        finish_ = 1;

        return -1;
      }
    }

    //
    // Remove the "channel unresponsive"
    // dialog.
    //

    if (alert_ != 0)
    {
      #if defined(TEST) || defined(INFO)
      *logofs << "handleCongestion: Displacing the dialog "
              << "for FD#" << fd_ << ".\n" << logofs_flush;
      #endif

      HandleAlert(DISPLACE_MESSAGE_ALERT, 1);
    }
  }

  return 1;
}

int Channel::handleFlush(T_flush type, int bufferLength, int scratchLength)
{
  if (finish_ == 1)
  {
    #ifdef TEST
    *logofs << "handleFlush: Not flushing data for "
            << "finishing channel for FD#" << fd_
            << ".\n" << logofs_flush;
    #endif

    writeBuffer_.fullReset();

    return -1;
  }

  #ifdef TEST
  *logofs << "handleFlush: Flushing " << bufferLength
          << " + " << scratchLength << " bytes "
          << "to FD#" << fd_ << ".\n"
          << logofs_flush;
  #endif

  //
  // Check if the channel has data available.
  // Recent Linux kernels are very picky.
  // They require that we read often or they
  // assume that the process is non-interact-
  // ive.
  //

  int result = 0;

  if (handleAsyncEvents() < 0)
  {
    goto ChannelFlushError;
  }

  //
  // Write the data in the main buffer first,
  // followed by the data in the scratch buffer.
  //

  if (bufferLength > 0)
  {
    result = transport_ -> write(write_immediate,
                                     writeBuffer_.getData(), bufferLength);
  }

  if (result >= 0 && scratchLength > 0)
  {
    result = transport_ -> write(write_immediate,
                                     writeBuffer_.getScratchData(), scratchLength);
  }

  if (type == flush_if_any)
  {
    writeBuffer_.fullReset();
  }
  else
  {
    writeBuffer_.partialReset();
  }

  //
  // If we failed to write to the X connection then
  // set the finish flag. The caller should continue
  // to handle all the remaining messages or it will
  // corrupt the decode buffer. At the real end, an
  // error will be propagated to the upper layers
  // which will perform any needed cleanup.
  //

  if (result < 0)
  {
    goto ChannelFlushError;
  }

  //
  // Reset transport buffers.
  //

  transport_ -> partialReset();

  //
  // Check if the X server has generated
  // any event in response to our data.
  //

  if (handleAsyncEvents() < 0)
  {
    goto ChannelFlushError;
  }

  //
  // Check if the channel has entered in
  // congestion state and, in this case,
  // send an immediate congestion control
  // code to the remote.
  //

  handleCongestion();

  //
  // We could optionally drain the output
  // buffer if this is X11 channel.
  //
  // if (isCongested() == 1 && isReliable() == 1)
  // {
  //   if (handleDrain(0, control -> ChannelTimeout) < 0)
  //   {
  //     goto ChannelFlushError;
  //   }
  // }
  //

  return 1;

ChannelFlushError:

  finish_ = 1;

  return -1;
}

int Channel::handleFlush()
{
  #ifdef TEST
  *logofs << "handleFlush: Flushing "
          << transport_ -> length() << " bytes to FD#"
          << fd_ << " with descriptor writable.\n"
          << logofs_flush;
  #endif

  //
  // Check if there is anything to read
  // before anf after having written to
  // the socket.
  //

  if (handleAsyncEvents() < 0)
  {
    goto ChannelFlushError;
  }

  if (transport_ -> flush() < 0)
  {
    #ifdef TEST
    *logofs << "handleFlush: Failure detected "
            << "flushing data to FD#" << fd_
            << ".\n" << logofs_flush;
    #endif

    goto ChannelFlushError;
  }

  if (handleAsyncEvents() < 0)
  {
    goto ChannelFlushError;
  }

  //
  // Reset channel's transport buffers.
  //

  transport_ -> partialReset();

  //
  // Check if the channel went out of the
  // congestion state.
  //

  handleCongestion();

  return 1;

ChannelFlushError:

  finish_ = 1;

  return -1;
}

void Channel::handleResetAlert()
{
  if (alert_ != 0)
  {
    #ifdef TEST
    *logofs << "handleResetAlert: The channel alert '"
            << alert_ << "' was displaced.\n"
            << logofs_flush;
    #endif

    alert_ = 0;
  }
}

int Channel::handleCompress(EncodeBuffer &encodeBuffer, const unsigned char opcode,
                                const unsigned int offset, const unsigned char *buffer,
                                    const unsigned int size, unsigned char *&compressedData,
                                        unsigned int &compressedDataSize)
{
  if (size <= offset)
  {
    #ifdef DEBUG
    *logofs << "handleCompress: Not compressing data for FD#" << fd_
            << " as offset is " << offset << " with data size "
            << size << ".\n" << logofs_flush;
    #endif

    return 0;
  }

  #ifdef DEBUG
  *logofs << "handleCompress: Compressing data for FD#" << fd_
          << " with data size " << size << " and offset "
          << offset << ".\n" << logofs_flush;
  #endif

  //
  // It is responsibility of the compressor to
  // mark the buffer as such if the compression
  // couldn't take place.
  //

  if (compressor_ -> compressBuffer(buffer + offset, size - offset, compressedData,
                                        compressedDataSize, encodeBuffer) <= 0)
  {
    #ifdef DEBUG
    *logofs << "handleCompress: Sent " << size - offset
            << " bytes of plain data for FD#" << fd_
            << ".\n" << logofs_flush;
    #endif

    return 0;
  }
  else
  {
    #ifdef DEBUG
    *logofs << "handleCompress: Sent " << compressedDataSize
            << " bytes of compressed data for FD#"
            << fd_ << ".\n" << logofs_flush;
    #endif

    return 1;
  }
}

int Channel::handleDecompress(DecodeBuffer &decodeBuffer, const unsigned char opcode,
                                  const unsigned int offset, unsigned char *buffer,
                                      const unsigned int size, const unsigned char *&compressedData,
                                          unsigned int &compressedDataSize)
{
  if (size <= offset)
  {
    return 0;
  }

  int result = compressor_ -> decompressBuffer(buffer + offset, size - offset,
                                                   compressedData, compressedDataSize,
                                                       decodeBuffer);
  if (result < 0)
  {
    #ifdef PANIC
    *logofs << "handleDecompress: PANIC! Failed to decompress "
            << size - offset << " bytes of data for FD#" << fd_
            << " with OPCODE#" << (unsigned int) opcode << ".\n"
            << logofs_flush;
    #endif

    cerr << "Error" << ": Data decompression failed for OPCODE#"
         << (unsigned int) opcode << ".\n";

    return -1;
  }
  else if (result == 0)
  {
    #ifdef DEBUG
    *logofs << "handleDecompress: Received " << size - offset
            << " bytes of plain data for FD#" << fd_
            << ".\n" << logofs_flush;
    #endif

    return 0;
  }
  else
  {
    #ifdef DEBUG
    *logofs << "handleDecompress: Received " << compressedDataSize
            << " bytes of compressed data for FD#" << fd_
            << ".\n" << logofs_flush;
    #endif

    return 1;
  }
}

int Channel::handleCleanAndNullRequest(unsigned char &opcode, unsigned char *&buffer,
                                           unsigned int &size)
{
  #ifdef TEST
  *logofs << "handleCleanAndNullRequest: Removing the previous data "
          << "and sending an X_NoOperation " << "for FD#" << fd_
          << " due to OPCODE#" << (unsigned int) opcode << " ("
          << DumpOpcode(opcode) << ").\n" << logofs_flush;
  #endif

  writeBuffer_.removeMessage(size - 4);

  size   = 4;
  opcode = X_NoOperation;

  return 1;
}

int Channel::handleNullRequest(unsigned char &opcode, unsigned char *&buffer,
                                   unsigned int &size)
{
  #ifdef TEST
  *logofs << "handleNullRequest: Sending an X_NoOperation for FD#"
          << fd_ << " due to OPCODE#" << (unsigned int) opcode
          << " (" << DumpOpcode(opcode) << ").\n"
          << logofs_flush;
  #endif

  size   = 4;
  buffer = writeBuffer_.addMessage(size);
  opcode = X_NoOperation;

  return 1;
}

void Channel::handleSplitStoreError(int resource)
{
  if (resource < 0 || resource >= CONNECTIONS_LIMIT)
  {
    #ifdef PANIC
    *logofs << "handleSplitStoreError: PANIC! Resource "
            << resource << " is out of range with limit "
            << "set to " << CONNECTIONS_LIMIT << ".\n"
            << logofs_flush;
    #endif

    cerr << "Error" << ": Resource " << resource
         << " is out of range with limit set to "
         << CONNECTIONS_LIMIT << ".\n";

    HandleCleanup();
  }
  else
  {
    #ifdef PANIC
    *logofs << "handleSplitStoreError: PANIC! Cannot "
            << "allocate the split store for resource "
            << resource << ".\n" << logofs_flush;
    #endif

    cerr << "Error" << ": Cannot allocate the "
         << "split store for resource " << resource
         << ".\n";

    HandleCleanup();
  }
}

void Channel::handleSplitStoreAlloc(List *list, int resource)
{
  if (resource < 0 || resource >= CONNECTIONS_LIMIT)
  {
    handleSplitStoreError(resource);
  }

  if (clientStore_ -> getSplitStore(resource) == NULL)
  {
    #if defined(TEST) || defined(SPLIT)
    *logofs << "handleSplitStoreAlloc: Allocating a new "
            << "split store for resource " << resource
            << ".\n" << logofs_flush;
    #endif

    SplitStore *splitStore = clientStore_ -> createSplitStore(resource);

    if (splitStore == NULL)
    {
      handleSplitStoreError(resource);
    }

    list -> add(resource);
  }
  #if defined(TEST) || defined(SPLIT)
  else
  {
    //
    // Old proxy versions only use a single
    // split store.
    //

    if (resource != 0)
    {
      *logofs << "handleSplitStoreAlloc: WARNING! A split "
              << "store for resource " << resource
              << " already exists.\n" << logofs_flush;
    }
  }
  #endif
}

void Channel::handleSplitStoreRemove(List *list, int resource)
{
  if (resource < 0 || resource >= CONNECTIONS_LIMIT)
  {
    handleSplitStoreError(resource);
  }

  SplitStore *splitStore = clientStore_ -> getSplitStore(resource);

  if (splitStore != NULL)
  {
    #if defined(TEST) || defined(SPLIT)
    *logofs << "handleSplitStoreRemove: Deleting the "
            << "split store for resource " << resource
            << ".\n" << logofs_flush;
    #endif

    clientStore_ -> destroySplitStore(resource);

    #if defined(TEST) || defined(SPLIT)
    *logofs << "handleSplitStoreRemove: Deleting resource "
            << resource << " from the list " << ".\n"
            << logofs_flush;
    #endif

    list -> remove(resource);
  }
  #if defined(TEST) || defined(SPLIT)
  else
  {
    *logofs << "handleSplitStoreRemove: WARNING! A split "
            << "store for resource " << resource
            << " does not exist.\n" << logofs_flush;
  }
  #endif
}

Split *Channel::handleSplitCommitRemove(int request, int resource, int position)
{
  #if defined(TEST) || defined(SPLIT)
  *logofs << "handleSplitCommitRemove: SPLIT! Checking split "
          << "commit with resource " << resource << " request "
          << request << " and position " << position
          << ".\n" << logofs_flush;
  #endif

  //
  // Remove the split from the split queue.
  //

  CommitStore *commitStore = clientStore_ -> getCommitStore();

  Split *split = commitStore -> pop();

  if (split == NULL)
  {
    #ifdef PANIC
    *logofs << "handleSplitCommitRemove: PANIC! Can't "
            << "find the split in the commit queue.\n"
            << logofs_flush;
    #endif

    cerr << "Error" << ": Can't find the "
         << "split in the commit queue.\n";

    HandleCleanup();
  }

  #if defined(TEST) || defined(SPLIT)
  *logofs << "handleSplitCommitRemove: SPLIT! Element from "
          << "the queue has resource " << split -> getResource()
          << " request " << split -> getRequest() << " and "
          << "position " << split -> getPosition()
          << ".\n" << logofs_flush;
  #endif

  if ((control -> isProtoStep7() == 1 &&
          (resource != split -> getResource() ||
               request != split -> getRequest() ||
                   position != split -> getPosition())) ||
                       (request != split -> getRequest() ||
                           position != split -> getPosition()))
  {
    #ifdef PANIC
    *logofs << "handleSplitCommitRemove: PANIC! The data in "
            << "the split doesn't match the commit request.\n"
            << logofs_flush;
    #endif

    cerr << "Error" << ": The data in the split doesn't "
         << "match the commit request.\n";

    return NULL;
  }

  #if defined(TEST) || defined(SPLIT)

  commitStore -> dump();

  #endif

  return split;
}

int Channel::setReferences()
{
  #ifdef TEST
  *logofs << "Channel: Initializing the static "
          << "members for the base class.\n"
          << logofs_flush;
  #endif

  firstClient_ = -1;

  fontPort_ = -1;

  #ifdef REFERENCES

  references_ = 0;

  #endif

  return 1;
}

int Channel::setOpcodes(OpcodeStore *opcodeStore)
{
  opcodeStore_ = opcodeStore;

  #ifdef TEST
  *logofs << "setOpcodes: Propagated opcodes store to channel "
          << "for FD#" << fd_ << ".\n" << logofs_flush;
  #endif

  return 1;
}

int Channel::setStores(ClientStore *clientStore, ServerStore *serverStore)
{
  clientStore_ = clientStore;
  serverStore_ = serverStore;

  #ifdef TEST
  *logofs << "setStores: Propagated message stores to channel "
          << "for FD#" << fd_ << ".\n" << logofs_flush;
  #endif

  return 1;
}

int Channel::setCaches(ClientCache *clientCache, ServerCache *serverCache)
{
  clientCache_ = clientCache;
  serverCache_ = serverCache;

  #ifdef TEST
  *logofs << "setCaches: Propagated encode caches to channel "
          << "for FD#" << fd_ << ".\n" << logofs_flush;
  #endif

  return 1;
}
