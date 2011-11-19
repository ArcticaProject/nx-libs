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

#ifndef ClientChannel_H
#define ClientChannel_H

#include "List.h"
#include "Channel.h"

#include "SequenceQueue.h"

#include "ClientReadBuffer.h"

//
// Set the verbosity level.
//

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

//
// If defined, the client channel will
// have the chance of suppressing more
// opcodes for test purposes.
//

#undef  LAME

//
// Define this to log a line when a
// channel is created or destroyed.
//

#undef  REFERENCES

//
// This class implements the X client
// side compression of the protocol.
//

class ClientChannel : public Channel
{
  public:

  ClientChannel(Transport *transport, StaticCompressor *compressor);

  virtual ~ClientChannel();

  virtual int handleRead(EncodeBuffer &encodeBuffer, const unsigned char *message,
                             unsigned int length);

  virtual int handleWrite(const unsigned char *message, unsigned int length);

  virtual int handleSplit(EncodeBuffer &encodeBuffer, MessageStore *store,
                              T_store_action action, int position, const unsigned char opcode,
                                  const unsigned char *buffer, const unsigned int size);

  virtual int handleSplit(DecodeBuffer &decodeBuffer, MessageStore *store,
                              T_store_action action, int position, unsigned char &opcode,
                                  unsigned char *&buffer, unsigned int &size)
  {
    return 0;
  }

  virtual int handleSplit(EncodeBuffer &encodeBuffer);

  virtual int handleSplit(DecodeBuffer &decodeBuffer)
  {
    return 0;
  }

  virtual int handleSplitEvent(EncodeBuffer &encodeBuffer, Split *split);

  virtual int handleSplitEvent(DecodeBuffer &decodeBuffer);

  virtual int handleMotion(EncodeBuffer &encodeBuffer)
  {
    return 0;
  }

  virtual int handleCompletion(EncodeBuffer &encodeBuffer)
  {
    return 0;
  }

  virtual int handleConfiguration();

  virtual int handleFinish();

  virtual int handleAsyncEvents()
  {
    return 0;
  }

  virtual int needSplit() const
  {
    #if defined(TEST) || defined(SPLIT)
    *logofs << "needSplit: SPLIT! Returning pending split "
            << "flag " << splitState_.pending << " with "
            << clientStore_ -> getSplitTotalSize()
            << " splits in the split stores.\n"
            << logofs_flush;
    #endif

    return splitState_.pending;
  }

  virtual int needMotion() const
  {
    return 0;
  }

  virtual T_channel_type getType() const
  {
    return channel_x11;
  }

  int setBigEndian(int flag);

  //
  // Initialize the static members.
  //

  static int setReferences();

  private:

  int handleFastReadRequest(EncodeBuffer &encodeBuffer, const unsigned char &opcode,
                                const unsigned char *&buffer, const unsigned int &size);

  int handleFastWriteReply(DecodeBuffer &decodeBuffer, unsigned char &opcode,
                               unsigned char *&buffer, unsigned int &size);

  int handleFastWriteEvent(DecodeBuffer &decodeBuffer, unsigned char &opcode,
                               unsigned char *&buffer, unsigned int &size);

  //
  // Intercept the request before the opcode
  // is encoded.
  //

  int handleTaintRequest(unsigned char &opcode, const unsigned char *&buffer,
                             unsigned int &size)
  {
    if (control -> isProtoStep7() == 0)
    {
      if (opcode == X_NXFreeSplit || opcode == X_NXAbortSplit ||
              opcode == X_NXFinishSplit)
      {
        return handleTaintSplitRequest(opcode, buffer, size);
      }
      else if (opcode == X_NXSetCacheParameters)
      {
        return handleTaintCacheRequest(opcode, buffer, size);
      }
      else if (opcode == X_NXGetFontParameters)
      {
        return handleTaintFontRequest(opcode, buffer, size);
      }
    }

    if (control -> TaintReplies > 0 &&
            opcode == X_GetInputFocus)
    {
      return handleTaintSyncRequest(opcode, buffer, size);
    }

    #ifdef LAME

    return handleTaintLameRequest(opcode, buffer, size);

    #endif

    return 0;
  }

  int handleTaintCacheRequest(unsigned char &opcode, const unsigned char *&buffer,
                                  unsigned int &size);

  int handleTaintFontRequest(unsigned char &opcode, const unsigned char *&buffer,
                                 unsigned int &size);

  int handleTaintSplitRequest(unsigned char &opcode, const unsigned char *&buffer,
                                  unsigned int &size);

  int handleTaintLameRequest(unsigned char &opcode, const unsigned char *&buffer,
                                 unsigned int &size);

  int handleTaintSyncRequest(unsigned char &opcode, const unsigned char *&buffer,
                                 unsigned int &size);

  int handleTaintSyncError(unsigned char opcode);

  //
  // How to handle sequence counter
  // in notification event.
  //

  enum T_sequence_mode
  {
    sequence_immediate,
    sequence_deferred
  };

  //
  // Send split notifications to the
  // agent.
  //

  int handleRestart(T_sequence_mode mode, int resource);

  int handleNotify(T_notification_type type, T_sequence_mode mode,
                       int resource, int request, int position);

  //
  // Other utility functions used in
  // handling of the image streaming.
  //

  int mustSplitMessage(int resource)
  {
    return (clientStore_ -> getSplitStore(resource) ->
                getSize() != 0);
  }

  int canSplitMessage(T_split_mode mode, unsigned int size)
  {
    return ((int) size >= control -> SplitDataThreshold &&
                (clientStore_ -> getSplitTotalStorageSize() < control ->
                     SplitTotalStorageSize && clientStore_ ->
                         getSplitTotalSize() < control -> SplitTotalSize));
  }

  int canSendSplit(Split *split)
  {
    return (split -> getMode() != split_sync ||
                split -> getState() == split_missed ||
                    split -> getState() == split_loaded);
  }

  int handleSplitSend(EncodeBuffer &encodeBuffer, int resource,
                          int &total, int &bytes);

  Split *handleSplitFind(T_checksum checksum, int resource);

  int handleSplitChecksum(EncodeBuffer &encodeBuffer, T_checksum checksum);

  void handleSplitEnable()
  {
    if (control -> isProtoStep7() == 0)
    {
      #if defined(TEST) || defined(SPLIT)
      *logofs << "handleSplitEnable: WARNING! Disabling split "
              << "with an old proxy version.\n"
              << logofs_flush;
      #endif

      enableSplit_ = 0;
    }
  }

  void handleSplitPending(int resource)
  {
    if (splitState_.pending == 0)
    {
      if (clientStore_ -> getSplitStore(resource) != NULL &&
              clientStore_ -> getSplitStore(resource) ->
                  getFirstSplit() != NULL)
      {
        splitState_.pending = canSendSplit(clientStore_ ->
            getSplitStore(resource) -> getFirstSplit());

        #if defined(TEST) || defined(SPLIT)
        *logofs << "handleSplitPending: SPLIT! Set the pending "
                << "split flag to " << splitState_.pending
                << " with " << clientStore_ -> getSplitTotalSize()
                << " splits in the split stores.\n"
                << logofs_flush;
        #endif
      }
    }
  }

  //
  // Scan all the split stores to find
  // if there is any split to send.
  //

  void handleSplitPending();

  //
  // Handle the MIT-SHM initialization
  // messages exchanged with the remote
  // proxy.
  //

  int handleShmemRequest(EncodeBuffer &encodeBuffer, const unsigned char opcode,
                             const unsigned char *buffer, const unsigned int size);

  int handleShmemReply(DecodeBuffer &decodeBuffer, unsigned char &opcode,
                           unsigned char *&buffer, unsigned int &size);

  //
  // Query the port used to tunnel
  // the font server connections.
  //

  int handleFontRequest(EncodeBuffer &encodeBuffer, const unsigned char opcode,
                            const unsigned char *buffer, const unsigned int size);

  int handleFontReply(DecodeBuffer &decodeBuffer, unsigned char &opcode,
                          unsigned char *&buffer, unsigned int &size);

  //
  // Let the agent set the cache
  // policy for image requests.
  //

  int handleCacheRequest(EncodeBuffer &encodeBuffer, const unsigned char opcode,
                             const unsigned char *buffer, const unsigned int size);

  //
  // Encode the start and end split
  // requests.
  //

  int handleStartSplitRequest(EncodeBuffer &encodeBuffer, const unsigned char opcode,
                                  const unsigned char *buffer, const unsigned int size);

  int handleEndSplitRequest(EncodeBuffer &encodeBuffer, const unsigned char opcode,
                                const unsigned char *buffer, const unsigned int size);

  //
  // Empty a split store and send the
  // restart event.
  //

  int handleAbortSplitRequest(EncodeBuffer &encodeBuffer, const unsigned char opcode,
                                  const unsigned char *buffer, const unsigned int size);

  //
  // Force the proxy to finalize all
  // the pending split operations and
  // restart a resource.
  //
  
  int handleFinishSplitRequest(EncodeBuffer &encodeBuffer, const unsigned char opcode,
                                   const unsigned char *buffer, const unsigned int size);

  //
  // Tell the remote peer to send the
  // split requests to the X server.
  //

  int handleCommitSplitRequest(EncodeBuffer &encodeBuffer, const unsigned char opcode,
                                   const unsigned char *buffer, const unsigned int size);

  //
  // Other utilities.
  //

  void handleDecodeCharInfo(DecodeBuffer &, unsigned char *);

  //
  // Own read buffer. It is able to identify
  // full messages read from the X descriptor.
  //

  ClientReadBuffer readBuffer_;

  //
  // Sequence number of last request coming
  // from the X client or the X server.
  //

  unsigned int clientSequence_;
  unsigned int serverSequence_;

  //
  // Last sequence number known by client. It can
  // be the real sequence generated by server or
  // the one of the last auto-generated event.
  //

  unsigned int lastSequence_;

  //
  // Used to identify replies based on sequence
  // number of original request.
  //

  SequenceQueue sequenceQueue_;

  //
  // This is used to test the synchronous flush
  // in the proxy.
  //

  int lastRequest_;

  //
  // Current resource id selected as target and
  // other information related to the image split.
  // The pending and abort flags are set when we
  // want the proxy to give us a chance to send
  // more split data. We also save the position
  // of the last commit operation performed by
  // channel so we can differentially encode the
  // position of next message to commit.
  //

  typedef struct
  {
    int resource;
    int pending;
    int commit;
    T_split_mode mode;

  } T_split_state;

  T_split_state splitState_;

  //
  // List of agent resources.
  //

  List splitResources_;

  //
  // How many sync requests we
  // have tainted so far.
  //

  int taintCounter_;

  private:

  //
  // Keep track of object 
  // creation and deletion.
  //

  #ifdef REFERENCES

  static int references_;

  #endif
};

#endif /* ClientChannel_H */
