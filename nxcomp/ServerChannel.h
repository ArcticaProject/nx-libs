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

#ifndef ServerChannel_H
#define ServerChannel_H

#include "List.h"
#include "Channel.h"

#include "SequenceQueue.h"

#include "ServerReadBuffer.h"

#include "Unpack.h"

//
// Set the verbosity level.
//

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

//
// How many sequence numbers of split commit
// requests we are going to save in order to
// mask errors.
//

#define MAX_COMMIT_SEQUENCE_QUEUE         16

//
// Define this to know when a channel
// is created or destroyed.
//

#undef  REFERENCES

//
// This class implements the X server
// side compression of X protocol.
//

class ServerChannel : public Channel
{
  public:

  ServerChannel(Transport *transport, StaticCompressor *compressor);

  virtual ~ServerChannel();

  virtual int handleRead(EncodeBuffer &encodeBuffer, const unsigned char *message,
                             unsigned int length);

  virtual int handleWrite(const unsigned char *message, unsigned int length);

  virtual int handleSplit(EncodeBuffer &encodeBuffer, MessageStore *store,
                              T_store_action action, int position, const unsigned char opcode,
                                  const unsigned char *buffer, const unsigned int size)
  {
    return 0;
  }

  virtual int handleSplit(DecodeBuffer &decodeBuffer, MessageStore *store,
                                T_store_action action, int position, unsigned char &opcode,
                                    unsigned char *&buffer, unsigned int &size);

  virtual int handleSplit(EncodeBuffer &encodeBuffer)
  {
    return 0;
  }

  virtual int handleSplit(DecodeBuffer &decodeBuffer);

  virtual int handleSplitEvent(EncodeBuffer &encodeBuffer, Split *split);

  virtual int handleSplitEvent(DecodeBuffer &decodeBuffer)
  {
    return 0;
  }

  //
  // Send the last motion notify event
  // received from the X server to the
  // remote proxy.
  //

  virtual int handleMotion(EncodeBuffer &encodeBuffer);

  virtual int handleCompletion(EncodeBuffer &encodeBuffer)
  {
    return 0;
  }

  virtual int handleConfiguration();

  virtual int handleFinish();

  virtual int handleAsyncEvents();

  virtual int needSplit() const
  {
    return 0;
  }

  virtual int needMotion() const
  {
    return (lastMotion_[0] != '\0');
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

  int handleFastReadReply(EncodeBuffer &encodeBuffer, const unsigned char &opcode,
                              const unsigned char *&buffer, const unsigned int &size);

  int handleFastReadEvent(EncodeBuffer &encodeBuffer, const unsigned char &opcode,
                              const unsigned char *&buffer, const unsigned int &size);

  int handleFastWriteRequest(DecodeBuffer &decodeBuffer, unsigned char &opcode,
                                 unsigned char *&buffer, unsigned int &size);

  //
  // Handle the fake authorization cookie
  // and the X server's reply.
  //

  int handleAuthorization(unsigned char *buffer);
  int handleAuthorization(const unsigned char *buffer, int size);

  //
  // Set the unpack colormap and the alpha
  // blending data to be used to unpack
  // images.
  //

  int handleGeometry(unsigned char &opcode, unsigned char *&buffer,
                         unsigned int &size);

  int handleColormap(unsigned char &opcode, unsigned char *&buffer,
                         unsigned int &size);

  int handleAlpha(unsigned char &opcode, unsigned char *&buffer,
                      unsigned int &size);

  //
  // Manage the decoded buffer to unpack
  // the image and move the data to the
  // shared memory segment.
  //

  int handleImage(unsigned char &opcode, unsigned char *&buffer,
                      unsigned int &size);

  //
  // Uncompress a packed image in one
  // or more graphic X requests.
  //

  int handleUnpack(unsigned char &opcode, unsigned char *&buffer,
                       unsigned int &size);

  //
  // Move the image to the shared
  // memory buffer.
  //

  int handleShmem(unsigned char &opcode, unsigned char *&buffer,
                      unsigned int &size);

  //
  // Handle suppression of error on
  // commit of image splits.
  //

  void initCommitQueue();

  void updateCommitQueue(unsigned short sequence);

  int checkCommitError(unsigned char error, unsigned short sequence,
                           const unsigned char *buffer);

  void clearCommitQueue()
  {
    if (commitSequenceQueue_[0] != 0)
    {
      initCommitQueue();
    }
  }

  //
  // Check if the user pressed the
  // CTRL+ALT+SHIFT+ESC keystroke.
  //

  int checkKeyboardEvent(unsigned char event, unsigned short sequence,
                             const unsigned char *buffer);

  //
  // Other utilities.
  //

  void handleEncodeCharInfo(const unsigned char *nextSrc, EncodeBuffer &encodeBuffer);

  //
  // Handle the MIT-SHM initialization
  // messages exchanged with the remote
  // proxy.
  //

  int handleShmemRequest(DecodeBuffer &decodeBuffer, unsigned char &opcode,
                             unsigned char *&buffer, unsigned int &size);


  int handleShmemReply(EncodeBuffer &encodeBuffer, const unsigned char opcode,
                           const unsigned int stage, const unsigned char *buffer,
                               const unsigned int size);

  //
  // Try to read more events in the attempt to
  // get the MIT-SHM image completion event
  // from the X server.
  //

  int handleShmemEvent();

  //
  // Handle the MIT-SHM events as they are read
  // from the socket.
  //

  int checkShmemEvent(unsigned char event, unsigned short sequence,
                          const unsigned char *buffer);

  int checkShmemError(unsigned char error, unsigned short sequence,
                          const unsigned char *buffer);

  //
  // Query the port used to tunnel
  // the font server connections.
  //

  int handleFontRequest(DecodeBuffer &decodeBuffer, unsigned char &opcode,
                            unsigned char *&buffer, unsigned int &size);

  int handleFontReply(EncodeBuffer &encodeBuffer, const unsigned char opcode,
                          const unsigned char *buffer, const unsigned int size);

  //
  // Set the cache policy for image
  // requests.
  //

  int handleCacheRequest(DecodeBuffer &decodeBuffer, unsigned char &opcode,
                             unsigned char *&buffer, unsigned int &size);

  //
  // Decode the start and end split
  // requests.
  //

  int handleStartSplitRequest(DecodeBuffer &decodeBuffer, unsigned char &opcode,
                                  unsigned char *&buffer, unsigned int &size);

  int handleEndSplitRequest(DecodeBuffer &decodeBuffer, unsigned char &opcode,
                                unsigned char *&buffer, unsigned int &size);

  //
  // Remove the split store and the
  // incomplete messages from the
  // memory cache.
  //

  int handleAbortSplitRequest(DecodeBuffer &decodeBuffer, unsigned char &opcode,
                                  unsigned char *&buffer, unsigned int &size);

  //
  // Send the split requests to the
  // X server once they have been
  // recomposed.
  //

  int handleCommitSplitRequest(DecodeBuffer &decodeBuffer, unsigned char &opcode,
                                   unsigned char *&buffer, unsigned int &size);

  int handleSplitChecksum(DecodeBuffer &decodeBuffer, T_checksum &checksum);

  void handleSplitEnable()
  {
    if (control -> isProtoStep7() == 0)
    {
      #if defined(TEST) || defined(SPLIT)
      *logofs << "handleSplitEnable: WARNING! Disabling load "
              << "and save with an old proxy version.\n"
              << logofs_flush;
      #endif

      splitState_.save = 0;
      splitState_.load = 0;
    }
  }

  //
  // Allocate and free the shared memory
  // support resources.
  //

  void handleShmemStateAlloc();
  void handleShmemStateRemove();

  //
  // Temporary storage for the image info.
  //

  void handleImageStateAlloc(unsigned char opcode)
  {
    if (imageState_ == NULL)
    {
      imageState_ = new T_image_state();
    }

    imageState_ -> opcode = opcode;
  }

  void handleImageStateRemove()
  {
    if (imageState_ != NULL)
    {
      delete imageState_;

      imageState_ = NULL;
    }
  }

  //
  // Store the information needed to unpack
  // images per each known agent's client.
  //

  void handleUnpackStateInit(int resource);

  void handleUnpackAllocGeometry(int resource);
  void handleUnpackAllocColormap(int resource);
  void handleUnpackAllocAlpha(int resource);

  void handleUnpackStateRemove(int resource);

  typedef struct
  {
    T_geometry *geometry;
    T_colormap *colormap;
    T_alpha    *alpha;

  } T_unpack_state;

  T_unpack_state *unpackState_[256];

  //
  // Own read buffer. It is able to identify
  // full messages read from X descriptor.
  //

  ServerReadBuffer readBuffer_;

  //
  // Sequence number of last request coming
  // from X client or X server.
  //

  unsigned int clientSequence_;
  unsigned int serverSequence_;

  //
  // Used to identify replies based on sequence
  // number of original request.
  //

  SequenceQueue sequenceQueue_;

  //
  // Last motion notify read from the X server.
  //

  unsigned char lastMotion_[32];

  //
  // Sequence numbers of last auto-generated
  // put image requests. Needed to intercept
  // and suppress errors generated by such
  // requests.
  //

  unsigned int commitSequenceQueue_[MAX_COMMIT_SEQUENCE_QUEUE];

  //
  // Let agent select which expose
  // events is going to receive.
  //

  unsigned int enableExpose_;
  unsigned int enableGraphicsExpose_;
  unsigned int enableNoExpose_;

  //
  // Used in initialization and handling
  // of MIT-SHM shared memory put images.
  //

  typedef struct
  {
    int           stage;
    int           present;
    int           enabled;
    int           segment;
    int           id;
    void          *address;
    unsigned int  size;

    unsigned char opcode;
    unsigned char event;
    unsigned char error;

    unsigned int  sequence;
    unsigned int  offset;
    T_timestamp   last;

    unsigned int  checked;

  } T_shmem_state;

  T_shmem_state *shmemState_;

  //
  // Used to pass current image data between
  // the different decompression stages.
  //

  typedef struct
  {
    unsigned char  opcode;

    unsigned int   drawable;
    unsigned int   gcontext;

    unsigned char  method;

    unsigned char  format;
    unsigned char  srcDepth;
    unsigned char  dstDepth;

    unsigned int   srcLength;
    unsigned int   dstLength;
    unsigned int   dstLines;

    short int      srcX;
    short int      srcY;
    unsigned short srcWidth;
    unsigned short srcHeight;

    short int      dstX;
    short int      dstY;
    unsigned short dstWidth;
    unsigned short dstHeight;

    unsigned char  leftPad;

  } T_image_state;

  T_image_state *imageState_;

  //
  // The flags is set according to the
  // split load and save policy set by
  // the encoding side.
  //

  typedef struct
  {
    int resource;
    int current;
    int load;
    int save;
    int commit;

  } T_split_state;

  T_split_state splitState_;

  //
  // List of agent resources.
  //

  List splitResources_;

  //
  // Keep track of object creation and
  // deletion.
  //

  private:

  #ifdef REFERENCES

  static int references_;

  #endif
};

#endif /* ServerChannel_H */
