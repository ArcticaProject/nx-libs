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

#ifndef Channel_H
#define Channel_H

#include "Transport.h"

#include "WriteBuffer.h"

#include "OpcodeStore.h"

#include "ClientStore.h"
#include "ServerStore.h"

#include "ClientCache.h"
#include "ServerCache.h"

#include "EncodeBuffer.h"
#include "DecodeBuffer.h"

//
// Forward declaration of referenced classes.
//

class List;

class StaticCompressor;

//
// Set the verbosity level.
//

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

//
// Define this to log a line when a channel
// is created or destroyed.
//

#undef  REFERENCES

//
// Type of traffic carried by channel.
//

typedef enum
{
  channel_none = -1,
  channel_x11,
  channel_cups,
  channel_smb,
  channel_media,
  channel_http,
  channel_font,
  channel_slave,
  channel_last_tag

} T_channel_type;

//
// Type of notification event to be sent
// by proxy to the X channel.
//

typedef enum
{
  notify_no_split,
  notify_start_split,
  notify_commit_split,
  notify_end_split,
  notify_empty_split,

} T_notification_type;

class Channel
{
  public:

  //
  // Maximum number of X connections supported.
  //

  static const int CONNECTIONS_LIMIT = 256;

  Channel(Transport *transport, StaticCompressor *compressor);

  virtual ~Channel();

  //
  // Read any X message available on the X
  // connection and encode it to the encode
  // buffer.
  //

  virtual int handleRead(EncodeBuffer &encodeBuffer, const unsigned char *message,
                             unsigned int length) = 0;

  //
  // Decode any X message encoded in the
  // proxy message and write it to the X
  // connection.
  //

  virtual int handleWrite(const unsigned char *message, unsigned int length) = 0;

  //
  // Other methods to be implemented in
  // client, server and generic channel
  // classes.
  //

  virtual int handleSplit(EncodeBuffer &encodeBuffer, MessageStore *store,
                              T_store_action action, int position, const unsigned char opcode,
                                  const unsigned char *buffer, const unsigned int size) = 0;

  virtual int handleSplit(DecodeBuffer &decodeBuffer, MessageStore *store,
                              T_store_action action, int position, unsigned char &opcode,
                                  unsigned char *&buffer, unsigned int &size) = 0;

  virtual int handleSplit(EncodeBuffer &encodeBuffer) = 0;

  virtual int handleSplit(DecodeBuffer &decodeBuffer) = 0;

  virtual int handleSplitEvent(EncodeBuffer &encodeBuffer, Split *split) = 0;

  virtual int handleSplitEvent(DecodeBuffer &decodeBuffer) = 0;

  virtual int handleMotion(EncodeBuffer &encodeBuffer) = 0;

  virtual int handleCompletion(EncodeBuffer &encodeBuffer) = 0;

  virtual int handleConfiguration() = 0;

  virtual int handleFinish() = 0;

  //
  // Interleave reads of the available
  // events while writing data to the
  // channel socket.
  //

  virtual int handleAsyncEvents() = 0;

  //
  // Handle the channel tear down.
  //

  int handleClosing()
  {
    closing_ = 1;

    return 1;
  }

  int handleDrop()
  {
    drop_ = 1;

    return 1;
  }

  //
  // Try to read more data from the socket. In
  // the meanwhile flush any enqueued data if
  // the channel is blocked. Return as soon as
  // more data has been read or the timeout has
  // been exceeded.
  //

  int handleWait(int timeout);

  //
  // Drain the output buffer while handling the
  // data that may become readable.
  //

  int handleDrain(int timeout, int limit);

  //
  // Flush any remaining data in the transport
  // buffer.
  //

  int handleFlush();

  //
  // Called when the loop has replaced or
  // closed a previous alert.
  //

  void handleResetAlert();

  //
  // Initialize all the static members.
  //

  static int setReferences();

  //
  // Set pointer to object mapping opcodes
  // of NX specific messages.
  //

  int setOpcodes(OpcodeStore *opcodeStore);

  //
  // Update pointers to message stores in
  // channels.
  //

  int setStores(ClientStore *clientStore, ServerStore *serverStore);

  //
  // The same for channels caches.
  //

  int setCaches(ClientCache *clientCache, ServerCache *serverCache);

  //
  // Set the port used for tunneling of the
  // font server connections.
  //

  void setPorts(int fontPort)
  {
    fontPort_ = fontPort;
  }

  //
  // Check if there are pending split 
  // to send to the remote side.
  //

  virtual int needSplit() const = 0;

  //
  // Check if there are motion events
  // to flush.
  //

  virtual int needMotion() const = 0;

  //
  // Return the type of traffic carried
  // by this channel.
  //

  virtual T_channel_type getType() const = 0;

  //
  // Check if the channel has been marked
  // as closing down.
  //

  int getFinish() const
  {
    return finish_;
  }

  int getClosing()
  {
    return closing_;
  }

  int getDrop()
  {
    return drop_;
  }

  int getCongestion()
  {
    return congestion_;
  }

  protected:

  int handleFlush(T_flush type)
  {
    //
    // We could write the data immediately if there
    // is already something queued to the low level
    // TCP buffers.
    //
    // if (... || transport_ -> queued() > 0)
    // {
    //   ...
    // }
    //

    if (writeBuffer_.getScratchLength() > 0 ||
          (type == flush_if_any && writeBuffer_.getLength() > 0) ||
              writeBuffer_.getLength() >= (unsigned int)
                  control -> TransportFlushBufferSize)
    {
      return handleFlush(type, writeBuffer_.getLength(),
                             writeBuffer_.getScratchLength());
    }

    return 0;
  }

  //
  // Actually flush the data to the
  // channel descriptor.
  //

  int handleFlush(T_flush type, int bufferLength, int scratchLength);

  //
  // Handle the congestion changes.
  //

  int handleCongestion();

  //
  // Encode and decode X messages.
  //

  int handleEncode(EncodeBuffer &encodeBuffer, ChannelCache *channelCache,
                       MessageStore *store, const unsigned char opcode,
                           const unsigned char *buffer, const unsigned int size);

  int handleDecode(DecodeBuffer &decodeBuffer, ChannelCache *channelCache,
                       MessageStore *store, unsigned char &opcode,
                           unsigned char *&buffer, unsigned int &size);

  //
  // Encode the message based on its
  // message store.
  //

  int handleEncodeCached(EncodeBuffer &encodeBuffer, ChannelCache *channelCache,
                             MessageStore *store,  const unsigned char *buffer,
                                 const unsigned int size);

  int handleDecodeCached(DecodeBuffer &decodeBuffer, ChannelCache *channelCache,
                             MessageStore *store, unsigned char *&buffer,
                                 unsigned int &size);

  int handleEncodeIdentity(EncodeBuffer &encodeBuffer, ChannelCache *channelCache,
                               MessageStore *store, const unsigned char *buffer,
                                   const unsigned int size, int bigEndian)
  {
    return (store -> encodeIdentity(encodeBuffer, buffer, size,
                                        bigEndian, channelCache));
  }

  int handleDecodeIdentity(DecodeBuffer &decodeBuffer, ChannelCache *channelCache,
                               MessageStore *store, unsigned char *&buffer,
                                   unsigned int &size, int bigEndian,
                                       WriteBuffer *writeBuffer)
  {
    return (store -> decodeIdentity(decodeBuffer, buffer, size, bigEndian,
                                        writeBuffer, channelCache));
  }

  //
  // Other utility functions used by
  // the encoding and decoding methods.
  //

  void handleCopy(EncodeBuffer &encodeBuffer, const unsigned char opcode,
                      const unsigned int offset, const unsigned char *buffer,
                          const unsigned int size)
  {
    if (size > offset)
    {
      encodeBuffer.encodeMemory(buffer + offset, size - offset);
    }
  }

  void handleCopy(DecodeBuffer &decodeBuffer, const unsigned char opcode,
                      const unsigned int offset, unsigned char *buffer,
                          const unsigned int size)
  {
    if (size > offset)
    {
      memcpy(buffer + offset, decodeBuffer.decodeMemory(size - offset), size - offset);
    }
  }

  void handleUpdate(MessageStore *store, const unsigned int dataSize,
                        const unsigned int compressedDataSize)
  {
    if (store -> lastAction == IS_ADDED)
    {
      handleUpdateAdded(store, dataSize, compressedDataSize);
    }
  }

  void handleSave(MessageStore *store, unsigned char *buffer, unsigned int size,
                      const unsigned char *compressedData = NULL,
                          const unsigned int compressedDataSize = 0)
  {
    if (store -> lastAction == IS_ADDED)
    {
      handleSaveAdded(store, 0, buffer, size, compressedData, compressedDataSize);
    }
  }

  void handleSaveSplit(MessageStore *store, unsigned char *buffer,
                          unsigned int size)
  {
    if (store -> lastAction == IS_ADDED)
    {
      return handleSaveAdded(store, 1, buffer, size, 0, 0);
    }
  }

  void handleUpdateAdded(MessageStore *store, const unsigned int dataSize,
                             const unsigned int compressedDataSize);

  void handleSaveAdded(MessageStore *store, int split, unsigned char *buffer,
                           unsigned int size, const unsigned char *compressedData,
                               const unsigned int compressedDataSize);

  //
  // Compress the data part of a message
  // using ZLIB or another compressor
  // and send it over the network.
  //

  int handleCompress(EncodeBuffer &encodeBuffer, const unsigned char opcode,
                         const unsigned int offset, const unsigned char *buffer,
                             const unsigned int size, unsigned char *&compressedData,
                                 unsigned int &compressedDataSize);

  int handleDecompress(DecodeBuffer &decodeBuffer, const unsigned char opcode,
                           const unsigned int offset, unsigned char *buffer,
                               const unsigned int size, const unsigned char *&compressedData,
                                   unsigned int &compressedDataSize);

  //
  // Send an X_NoOperation to the X server.
  // The second version also removes any
  // previous data in the write buffer.
  //

  int handleNullRequest(unsigned char &opcode, unsigned char *&buffer,
                            unsigned int &size);

  int handleCleanAndNullRequest(unsigned char &opcode, unsigned char *&buffer,
                                    unsigned int &size);

  //
  // X11 channels are considered to be in
  // congestion state when there was a
  // blocking write and, since then, the
  // local end didn't consume all the data.
  //

  virtual int isCongested()
  {
    return (transport_ -> getType() !=
                transport_agent && transport_ -> length() >
                    control -> TransportFlushBufferSize);
  }

  virtual int isReliable()
  {
    return 1;
  }

  //
  // Determine how to handle allocation
  // of new messages in the message
  // stores.
  //

  int mustCleanStore(MessageStore *store)
  {
    return (store -> getRemoteTotalStorageSize() > control ->
                RemoteTotalStorageSize || store -> getLocalTotalStorageSize() >
                    control -> LocalTotalStorageSize || (store -> getRemoteStorageSize() >
                        (control -> RemoteTotalStorageSize / 100 * store ->
                            cacheThreshold)) || (store -> getLocalStorageSize() >
                                (control -> LocalTotalStorageSize / 100 * store ->
                                    cacheThreshold)));
  }

  int canCleanStore(MessageStore *store)
  {
    return ((store -> getSize() > 0 && (store -> getRemoteStorageSize() >
                (control -> RemoteTotalStorageSize / 100 * store ->
                    cacheLowerThreshold))) || (store -> getLocalStorageSize() >
                        (control -> LocalTotalStorageSize / 100 *  store ->
                            cacheLowerThreshold)));
  }

  protected:

  //
  // Set up the split stores.
  //

  void handleSplitStoreError(int resource);

  void handleSplitStoreAlloc(List *list, int resource);
  void handleSplitStoreRemove(List *list, int resource);

  Split *handleSplitCommitRemove(int request, int resource, int position);

  void validateSize(const char *name, int input, int output,
                        int offset, int size)
  {
    if (size < offset || size > control -> MaximumMessageSize ||
           size != (int) RoundUp4(input) + offset ||
               output > control -> MaximumMessageSize)
    {
      *logofs << "Channel: PANIC! Invalid size " << size
              << " for " << name << " output with data "
              << input << "/" << output << "/" << offset
              << "/" << size << ".\n" << logofs_flush;

      cerr << "Error" << ": Invalid size " << size
           << " for " << name << " output.\n";

      HandleAbort();
    }
  }

  //
  // Is the X client big endian?
  //

  int bigEndian() const
  {
    return bigEndian_;
  }

  int bigEndian_;

  //
  // Other X server's features
  // saved at session startup.
  //

  unsigned int imageByteOrder_;
  unsigned int bitmapBitOrder_;
  unsigned int scanlineUnit_;
  unsigned int scanlinePad_;

  int firstRequest_;
  int firstReply_;

  //
  // Use this class for IO operations.
  //

  Transport *transport_;

  //
  // The static compressor is created by the
  // proxy and shared among channels.
  //

  StaticCompressor *compressor_;

  //
  // Map NX operations to opcodes. Propagated
  // by proxy to all channels on the same X
  // server.
  //

  OpcodeStore *opcodeStore_;
 
  //
  // Also stores are shared between channels.
  //

  ClientStore *clientStore_;
  ServerStore *serverStore_;

  //
  // Caches are specific for each channel.
  //

  ClientCache *clientCache_;
  ServerCache *serverCache_;

  //
  // Data going to X connection.
  //

  WriteBuffer writeBuffer_;

  //
  // Other data members.
  //

  int fd_;

  int finish_;
  int closing_;
  int drop_;
  int congestion_;
  int priority_;

  int alert_;

  //
  // It will be set to the descriptor of the
  // first X channel that is successfully con-
  // nected and will print an info message on
  // standard error.
  //

  static int firstClient_;

  //
  // Port used for font server connections.
  //

  static int fontPort_;

  //
  // Track which cache operations have been
  // enabled by the agent.
  //

  int enableCache_;
  int enableSplit_;
  int enableSave_;
  int enableLoad_;

  //
  // Keep track of object creation and
  // deletion.
  //

  #ifdef REFERENCES

  static int references_;

  #endif
};

#endif /* Channel_H */
