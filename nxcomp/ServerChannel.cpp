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

#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#ifndef ANDROID
#include <sys/shm.h>
#endif

#include <X11/X.h>
#include <X11/Xatom.h>

#include "NXproto.h"
#include "NXalert.h"
#include "NXpack.h"
#include "NXmitshm.h"

#include "ServerChannel.h"

#include "EncodeBuffer.h"
#include "DecodeBuffer.h"

#include "StaticCompressor.h"

#include "Statistics.h"
#include "Proxy.h"

#include "Auth.h"
#include "Unpack.h"

//
// Available unpack methods.
//

#include "Alpha.h"
#include "Colormap.h"
#include "Bitmap.h"
#include "Jpeg.h"
#include "Pgn.h"
#include "Rgb.h"
#include "Rle.h"

extern Proxy *proxy;

//
// Set the verbosity level. You also
// need to define OPCODES in Misc.cpp
// if you want literals instead of
// opcodes' numbers.
//

#define PANIC
#define WARNING
#undef  OPCODES
#undef  TEST
#undef  DEBUG
#undef  DUMP

//
// Log the important tracepoints related
// to writing packets to the peer proxy.
//

#undef  FLUSH

//
// Log the operations related to splits.
//

#undef  SPLIT

//
// Define this to log when a channel
// is created or destroyed.
//

#undef  REFERENCES

//
// Define this to exit and suspend the
// session after a given number of X
// messages decoded by the proxy.
//

#undef  SUSPEND

//
// Define these to hide the server extensions.
//

#define HIDE_MIT_SHM_EXTENSION
#define HIDE_BIG_REQUESTS_EXTENSION
#define HIDE_XFree86_Bigfont_EXTENSION
#undef  HIDE_SHAPE_EXTENSION
#undef  HIDE_XKEYBOARD_EXTENSION

//
// Known reasons of connection failures.
//

#define INVALID_COOKIE_DATA  "Invalid MIT-MAGIC-COOKIE-1 key"
#define INVALID_COOKIE_SIZE  ((int) sizeof(INVALID_COOKIE_DATA) - 1)

#define NO_AUTH_PROTO_DATA   "No protocol specified"
#define NO_AUTH_PROTO_SIZE   ((int) sizeof(NO_AUTH_PROTO_DATA) - 1)

//
// Here are the static members.
//

#ifdef REFERENCES

int ServerChannel::references_ = 0;

#endif

ServerChannel::ServerChannel(Transport *transport, StaticCompressor *compressor)

  : Channel(transport, compressor), readBuffer_(transport_, this)
{
  //
  // Sequence number of the next message
  // being encoded or decoded.
  //

  clientSequence_ = 0;
  serverSequence_ = 0;

  //
  // Save the last motion event and flush
  // it only when the timeout expires.
  //

  lastMotion_[0] = '\0';

  //
  // Clear the queue of sequence numbers
  // of split commits. Used to mask the
  // errors.
  //

  initCommitQueue();

  //
  // Do we enable or not sending of expose
  // events to the X client.
  //

  enableExpose_         = 1;
  enableGraphicsExpose_ = 1;
  enableNoExpose_       = 1;

  //
  // Track data of image currently being
  // decompressed.
  //

  imageState_ = NULL;

  //
  // Track MIT-SHM resources.
  //

  shmemState_ = NULL;

  //
  // Store the unpack state for each agent
  // resource.
  //

  for (int i = 0; i < CONNECTIONS_LIMIT; i++)
  {
    unpackState_[i] = NULL;
  }

  //
  // Data about the split parameters requested
  // by the encoding side.
  //

  splitState_.resource = nothing;
  splitState_.current  = 0;
  splitState_.save     = 1;
  splitState_.load     = 1;
  splitState_.commit   = 0;

  handleSplitEnable();

  //
  // It will be eventually set by
  // the server proxy.
  //

  fontPort_ = -1;

  #ifdef REFERENCES
  *logofs << "ServerChannel: Created new object at " 
          << this << " for FD#" << fd_ << " out of "
          << ++references_ << " allocated channels.\n"
          << logofs_flush;
  #endif
}

ServerChannel::~ServerChannel()
{
  #ifdef TEST
  *logofs << "ServerChannel: Freeing image state information.\n"
          << logofs_flush;
  #endif

  handleImageStateRemove();

  #ifdef TEST
  *logofs << "ServerChannel: Freeing shared memory information.\n"
          << logofs_flush;
  #endif

  handleShmemStateRemove();

  #ifdef TEST
  *logofs << "ServerChannel: Freeing unpack state information.\n"
          << logofs_flush;
  #endif

  for (int i = 0; i < CONNECTIONS_LIMIT; i++)
  {
    handleUnpackStateRemove(i);
  }

  #ifdef TEST
  *logofs << "ServerChannel: Freeing channel caches.\n"
          << logofs_flush;
  #endif

  #ifdef REFERENCES
  *logofs << "ServerChannel: Deleted object at " 
          << this << " for FD#" << fd_ << " out of "
          << --references_ << " allocated channels.\n"
          << logofs_flush;
  #endif
}

//
// Beginning of handleRead().
//

int ServerChannel::handleRead(EncodeBuffer &encodeBuffer, const unsigned char *message,
                                  unsigned int length)
{
  #ifdef DEBUG
  *logofs << "handleRead: Called for FD#" << fd_
          << ".\n" << logofs_flush;
  #endif

  //
  // Pointer to located message and
  // its size in bytes.
  //

  const unsigned char *inputMessage;
  unsigned int inputLength;

  //
  // Set when message is found in
  // cache.
  //

  int hit;

  #if defined(TEST) || defined(INFO)
  *logofs << "handleRead: Trying to read from FD#"
          << fd_ << " at " << strMsTimestamp() << ".\n"
          << logofs_flush;
  #endif

  int result = readBuffer_.readMessage();

  #if defined(DEBUG) || defined(INFO)
  *logofs << "handleRead: Read result on FD#" << fd_
          << " is " << result << ".\n"
          << logofs_flush;
  #endif

  if (result < 0)
  {
    //
    // Let the proxy close the channel.
    //

    return -1;
  }
  else if (result == 0)
  {
    #if defined(TEST) || defined(INFO)

    //
    // This can happen because we have the descriptor
    // selected in the read set but we already read
    // the data asynchronously, while decoding data
    // read from the proxy.
    //

    *logofs << "handleRead: WARNING! No data read from FD#"
            << fd_ << " while encoding messages.\n"
            << logofs_flush;

    #endif

    return 0;
  }

  #if defined(TEST) || defined(INFO) || defined(FLUSH)
  *logofs << "handleRead: Encoding messages for FD#" << fd_
          << " with " << readBuffer_.getLength() << " bytes "
          << "in the buffer.\n" << logofs_flush;
  #endif

  //
  // Extract any complete message which 
  // is available in the buffer.
  //

  if (proxy -> handleAsyncSwitch(fd_) < 0)
  {
    return -1;
  }

  while ((inputMessage = readBuffer_.getMessage(inputLength)) != NULL)
  {
    hit = 0;

    if (firstReply_)
    {
      //
      // Handle the X server's authorization reply.
      //

      if (handleAuthorization(inputMessage, inputLength) < 0)
      {
        return -1;
      }

      imageByteOrder_ = inputMessage[30];
      bitmapBitOrder_ = inputMessage[31];
      scanlineUnit_   = inputMessage[32];
      scanlinePad_    = inputMessage[33];

      encodeBuffer.encodeValue((unsigned int) inputMessage[0], 8);
      encodeBuffer.encodeValue((unsigned int) inputMessage[1], 8);
      encodeBuffer.encodeValue(GetUINT(inputMessage + 2, bigEndian_), 16);
      encodeBuffer.encodeValue(GetUINT(inputMessage + 4, bigEndian_), 16);
      encodeBuffer.encodeValue(GetUINT(inputMessage + 6, bigEndian_), 16);

      if (ServerCache::lastInitReply.compare(inputLength - 8, inputMessage + 8))
      {
        encodeBuffer.encodeBoolValue(1);
      }
      else
      {
        encodeBuffer.encodeBoolValue(0);

        for (unsigned int i = 8; i < inputLength; i++)
        {
          encodeBuffer.encodeValue((unsigned int) inputMessage[i], 8);
        }
      }

      firstReply_ = 0;

      #if defined(TEST) || defined(OPCODES)

      int bits = encodeBuffer.diffBits();

      *logofs << "handleRead: Handled first reply. " << inputLength
              << " bytes in, " << bits << " bits (" << ((float) bits) / 8
              << " bytes) out.\n" << logofs_flush;

      #endif

      priority_++;

      //
      // Due to the way the loop was implemented
      // we can't encode multiple messages if we
      // are encoding the first request.
      //

      if (control -> isProtoStep7() == 0)
      {
        if (proxy -> handleAsyncInit() < 0)
        {
          return -1;
        }
      }
    }
    else
    {
      //
      // NX client needs this line to consider
      // the initialization phase successfully
      // completed.
      //

      if (firstClient_ == -1)
      {
        cerr << "Info" << ": Established X server connection.\n" ;

        firstClient_ = fd_;
      }

      //
      // Check if this is a reply.
      //

      if (*inputMessage == X_Reply)
      {
        int bits = 0;

        unsigned char inputOpcode = *inputMessage;

        unsigned short int requestSequenceNum;
        unsigned char      requestOpcode;
        unsigned int       requestData[3];

        unsigned int sequenceNum = GetUINT(inputMessage + 2, bigEndian_);

        #ifdef SUSPEND

        if (sequenceNum >= 1000)
        {
          cerr << "Warning" << ": Exiting to test the resilience of the agent.\n";

          sleep(2);

          HandleAbort();
        }

        #endif

        //
        // We managed all the events and errors caused
        // by the previous requests. We can now reset
        // the queue of split commits.
        //

        clearCommitQueue();

        //
        // Encode opcode and difference between
        // current sequence and the last one.
        //

        encodeBuffer.encodeOpcodeValue(inputOpcode, serverCache_ -> opcodeCache);

        unsigned int sequenceDiff = sequenceNum - serverSequence_;

        serverSequence_ = sequenceNum;

        #ifdef DEBUG
        *logofs << "handleRead: Last server sequence number for FD#" 
                << fd_ << " is " << serverSequence_ << " with "
                << "difference " << sequenceDiff << ".\n"
                << logofs_flush;
        #endif

        encodeBuffer.encodeCachedValue(sequenceDiff, 16,
                           serverCache_ -> replySequenceCache, 7);

        //
        // Now handle the data part.
        //

        if (sequenceQueue_.peek(requestSequenceNum, requestOpcode) &&
                                    requestSequenceNum == sequenceNum)
        {
          //
          // We've found the request that generated this reply.
          // It is possible to compress the reply based on the
          // specific request type.
          //

          sequenceQueue_.pop(requestSequenceNum, requestOpcode,
                                 requestData[0], requestData[1], requestData[2]);

          //
          // If differential compression is disabled
          // then use the most simple encoding.
          //

          if (control -> LocalDeltaCompression == 0)
          {
            int result = handleFastReadReply(encodeBuffer, requestOpcode,
                                                 inputMessage, inputLength);
            if (result < 0)
            {
              return -1;
            }
            else if (result > 0)
            {
              continue;
            }
          }

          switch (requestOpcode)
          {
          case X_AllocColor:
            {
              const unsigned char *nextSrc = inputMessage + 8;
              for (unsigned int i = 0; i < 3; i++)
              {
                unsigned int colorValue = GetUINT(nextSrc, bigEndian_);
                nextSrc += 2;
                if (colorValue == requestData[i])
                  encodeBuffer.encodeBoolValue(1);
                else
                {
                  encodeBuffer.encodeBoolValue(0);
                  encodeBuffer.encodeValue(colorValue - colorValue, 16, 6);
                }
              }
              unsigned int pixel = GetULONG(inputMessage + 16, bigEndian_);
              encodeBuffer.encodeValue(pixel, 32, 9);

              priority_++;
            }
            break;
          case X_GetAtomName:
            {
              unsigned int nameLength = GetUINT(inputMessage + 8, bigEndian_);
              encodeBuffer.encodeValue(nameLength, 16, 6);
              const unsigned char *nextSrc = inputMessage + 32;

              if (control -> isProtoStep7() == 1)
              {
                encodeBuffer.encodeTextData(nextSrc, nameLength);
              }
              else
              {
                serverCache_ -> getAtomNameTextCompressor.reset();
                for (unsigned int i = 0; i < nameLength; i++)
                {
                  serverCache_ -> getAtomNameTextCompressor.
                        encodeChar(*nextSrc++, encodeBuffer);
                }
              }

              priority_++;
            }
            break;
          case X_GetGeometry:
            {
              //
              // TODO: This obtains a satisfactory 10:1, but
              // could be cached to leverage the big amount
              // of such requests issued by QT clients.
              //

              encodeBuffer.encodeCachedValue(inputMessage[1], 8,
                                             serverCache_ -> depthCache);
              encodeBuffer.encodeCachedValue(GetULONG(inputMessage + 8, bigEndian_),
                                  29, serverCache_ -> getGeometryRootCache, 9);
              const unsigned char *nextSrc = inputMessage + 12;
              for (unsigned int i = 0; i < 5; i++)
              {
                encodeBuffer.encodeCachedValue(GetUINT(nextSrc, bigEndian_), 16,
                                   *serverCache_ -> getGeometryGeomCache[i], 8);
                nextSrc += 2;
              }

              priority_++;
            }
            break;
          case X_GetInputFocus:
            {
              //
              // Is it a real X_GetInputFocus or a
              // masqueraded reply?
              //

              if (requestData[0] == X_GetInputFocus)
              {
                encodeBuffer.encodeValue((unsigned int) inputMessage[1], 2);
                encodeBuffer.encodeCachedValue(GetULONG(inputMessage + 8, bigEndian_),
                                29, serverCache_ -> getInputFocusWindowCache);

                priority_++;
              }
              else
              {
                //
                // TODO: We are not setting priority in case
                // of replies other than real X_GetInputFocus
                // or X_NXGetUnpackParameters. We should check
                // once again that this is OK.
                //

                #ifdef TEST
                *logofs << "handleRead: Received tainted X_GetInputFocus reply "
                        << "for request OPCODE#" << requestData[0] << " with "
                        << "sequence " << sequenceNum << ".\n"
                        << logofs_flush;
                #endif

                //
                // Don't encode any data in case of sync
                // messages or any other reply for which
                // opcode is enough.
                //

                if (requestData[0] == opcodeStore_ -> getUnpackParameters)
                {
                  for (int i = 0; i < PACK_METHOD_LIMIT; i++)
                  {
                    encodeBuffer.encodeBoolValue(control -> LocalUnpackMethods[i]);
                  }

                  priority_++;
                }
                else if (requestData[0] == opcodeStore_ -> getShmemParameters)
                {
                  if (handleShmemReply(encodeBuffer, requestOpcode, requestData[1],
                                           inputMessage, inputLength) < 0)
                  {
                    return -1;
                  }

                  priority_++;
                }
                else if (requestData[0] == opcodeStore_ -> getFontParameters)
                {
                  if (handleFontReply(encodeBuffer, requestOpcode,
                                          inputMessage, inputLength) < 0)
                  {
                    return -1;
                  }
                }

                //
                // Account this data to the original opcode.
                //

                requestOpcode = requestData[0];
              }
            }
            break;
          case X_GetKeyboardMapping:
            {
              unsigned int keysymsPerKeycode = (unsigned int) inputMessage[1];
              if (ServerCache::getKeyboardMappingLastMap.compare(inputLength - 32,
                        inputMessage + 32) && (keysymsPerKeycode ==
                            ServerCache::getKeyboardMappingLastKeysymsPerKeycode))
              {
                encodeBuffer.encodeBoolValue(1);

                priority_++;

                break;
              }
              ServerCache::getKeyboardMappingLastKeysymsPerKeycode = keysymsPerKeycode;
              encodeBuffer.encodeBoolValue(0);
              unsigned int numKeycodes =
              (((inputLength - 32) / keysymsPerKeycode) >> 2);
              encodeBuffer.encodeValue(numKeycodes, 8);
              encodeBuffer.encodeValue(keysymsPerKeycode, 8, 4);
              const unsigned char *nextSrc = inputMessage + 32;
              unsigned char previous = 0;
              for (unsigned int count = numKeycodes * keysymsPerKeycode;
                   count; --count)
              {
                unsigned int keysym = GetULONG(nextSrc, bigEndian_);
                nextSrc += 4;
                if (keysym == NoSymbol)
                  encodeBuffer.encodeBoolValue(1);
                else
                {
                  encodeBuffer.encodeBoolValue(0);
                  unsigned int first3Bytes = (keysym >> 8);
                  encodeBuffer.encodeCachedValue(first3Bytes, 24,
                             serverCache_ -> getKeyboardMappingKeysymCache, 9);
                  unsigned char lastByte = (unsigned char) (keysym & 0xff);
                  encodeBuffer.encodeCachedValue(lastByte - previous, 8,
                           serverCache_ -> getKeyboardMappingLastByteCache, 5);
                  previous = lastByte;
                }
              }

              priority_++;
            }
            break;
          case X_GetModifierMapping:
            {
              encodeBuffer.encodeValue((unsigned int) inputMessage[1], 8);
              const unsigned char *nextDest = inputMessage + 32;
              if (ServerCache::getModifierMappingLastMap.compare(inputLength - 32,
                                                nextDest))
              {
                encodeBuffer.encodeBoolValue(1);

                priority_++;

                break;
              }
              encodeBuffer.encodeBoolValue(0);
              for (unsigned int count = inputLength - 32; count; count--)
              {
                unsigned char next = *nextDest++;
                if (next == 0)
                  encodeBuffer.encodeBoolValue(1);
                else
                {
                  encodeBuffer.encodeBoolValue(0);
                  encodeBuffer.encodeValue(next, 8);
                }
              }

              priority_++;
            }
            break;
          case X_GetProperty:
            {
              MessageStore *messageStore = serverStore_ ->
                                   getReplyStore(X_GetProperty);

              hit = handleEncode(encodeBuffer, serverCache_, messageStore,
                                     requestOpcode, inputMessage, inputLength);

              priority_++;
            }
            break;
          case X_GetSelectionOwner:
            {
              encodeBuffer.encodeCachedValue(GetULONG(inputMessage + 8, bigEndian_),
                                29, serverCache_ -> getSelectionOwnerCache, 9);
              priority_++;
            }
            break;
          case X_GetWindowAttributes:
            {
              encodeBuffer.encodeValue((unsigned int) inputMessage[1], 2);
              encodeBuffer.encodeCachedValue(GetULONG(inputMessage + 8, bigEndian_),
                                           29, serverCache_ -> visualCache);
              encodeBuffer.encodeCachedValue(GetUINT(inputMessage + 12, bigEndian_),
                         16, serverCache_ -> getWindowAttributesClassCache, 3);
              encodeBuffer.encodeCachedValue(inputMessage[14], 8,
                           serverCache_ -> getWindowAttributesBitGravityCache);
              encodeBuffer.encodeCachedValue(inputMessage[15], 8,
                           serverCache_ -> getWindowAttributesWinGravityCache);
              encodeBuffer.encodeCachedValue(GetULONG(inputMessage + 16, bigEndian_),
                        32, serverCache_ -> getWindowAttributesPlanesCache, 9);
              encodeBuffer.encodeCachedValue(GetULONG(inputMessage + 20, bigEndian_),
                         32, serverCache_ -> getWindowAttributesPixelCache, 9);
              encodeBuffer.encodeBoolValue((unsigned int) inputMessage[24]);
              encodeBuffer.encodeBoolValue((unsigned int) inputMessage[25]);
              encodeBuffer.encodeValue((unsigned int) inputMessage[26], 2);
              encodeBuffer.encodeBoolValue((unsigned int) inputMessage[27]);
              encodeBuffer.encodeCachedValue(GetULONG(inputMessage + 28, bigEndian_),
                                         29, serverCache_ -> colormapCache, 9);
              encodeBuffer.encodeCachedValue(GetULONG(inputMessage + 32, bigEndian_),
                        32, serverCache_ -> getWindowAttributesAllEventsCache);
              encodeBuffer.encodeCachedValue(GetULONG(inputMessage + 36, bigEndian_),
                       32, serverCache_ -> getWindowAttributesYourEventsCache);
              encodeBuffer.encodeCachedValue(GetUINT(inputMessage + 40, bigEndian_),
                    16, serverCache_ -> getWindowAttributesDontPropagateCache);

              priority_++;
            }
            break;
          case X_GrabKeyboard:
          case X_GrabPointer:
            {
              encodeBuffer.encodeValue((unsigned int) inputMessage[1], 3);

              priority_++;
            }
            break;
          case X_InternAtom:
            {
              encodeBuffer.encodeValue(GetULONG(inputMessage + 8, bigEndian_), 29, 9);

              priority_++;
            }
            break;
          case X_ListExtensions:
            {
              encodeBuffer.encodeValue(GetULONG(inputMessage + 4, bigEndian_), 32, 8);
              unsigned int numExtensions = (unsigned int) inputMessage[1];
              encodeBuffer.encodeValue(numExtensions, 8);
              const unsigned char *nextSrc = inputMessage + 32;

              for (; numExtensions; numExtensions--)
              {
                unsigned int length = (unsigned int) (*nextSrc++);

                encodeBuffer.encodeValue(length, 8);

                #ifdef HIDE_MIT_SHM_EXTENSION

                if (!strncmp((char *) nextSrc, "MIT-SHM", 7))
                {
                  #ifdef TEST
                  *logofs << "handleRead: Hiding MIT-SHM extension in reply.\n"
                          << logofs_flush;
                  #endif

                  memcpy((unsigned char *) nextSrc, "NO-MIT-", 7);
                }

                #endif

                #ifdef HIDE_BIG_REQUESTS_EXTENSION

                if (!strncmp((char *) nextSrc, "BIG-REQUESTS", 12))
                {
                  #ifdef TEST
                  *logofs << "handleRead: Hiding BIG-REQUESTS extension in reply.\n"
                          << logofs_flush;
                  #endif

                  memcpy((unsigned char *) nextSrc, "NO-BIG-REQUE", 12);
                }

                #endif

                #ifdef HIDE_XKEYBOARD_EXTENSION

                if (!strncmp((char *) nextSrc, "XKEYBOARD", 9))
                {
                  #ifdef TEST
                  *logofs << "handleRead: Hiding XKEYBOARD extension in reply.\n"
                          << logofs_flush;
                  #endif

                  memcpy((unsigned char *) nextSrc, "NO-XKEYBO", 9);
                }

                #endif

                #ifdef HIDE_XFree86_Bigfont_EXTENSION

                if (!strncmp((char *) nextSrc, "XFree86-Bigfont", 15))
                {
                  #ifdef TEST
                  *logofs << "handleRead: Hiding XFree86-Bigfont extension in reply.\n"
                          << logofs_flush;
                  #endif

                  memcpy((unsigned char *) nextSrc, "NO-XFree86-Bigf", 15);
                }

                #endif

                #ifdef HIDE_SHAPE_EXTENSION

                if (!strncmp((char *) nextSrc, "SHAPE", 5))
                {
                  #ifdef TEST
                  *logofs << "handleRead: Hiding SHAPE extension in reply.\n"
                          << logofs_flush;
                  #endif

                  memcpy((unsigned char *) nextSrc, "NO-SH", 5);
                }

                #endif

                //
                // Check if user disabled RENDER extension.
                //

                if (control -> HideRender == 1 &&
                        !strncmp((char *) nextSrc, "RENDER", 6))
                {
                  #ifdef TEST
                  *logofs << "handleRead: Hiding RENDER extension in reply.\n"
                          << logofs_flush;
                  #endif

                  memcpy((unsigned char *) nextSrc, "NO-REN", 6);
                }

                for (; length; length--)
                {
                  encodeBuffer.encodeValue((unsigned int) (*nextSrc++), 8);
                }
              }

              priority_++;
            }
            break;
          case X_ListFonts:
            {
              MessageStore *messageStore = serverStore_ ->
                                   getReplyStore(X_ListFonts);

              if (handleEncodeCached(encodeBuffer, serverCache_, messageStore,
                                         inputMessage, inputLength))
              {
                priority_++;

                hit = 1;

                break;
              }

              encodeBuffer.encodeValue(GetULONG(inputMessage + 4, bigEndian_), 32, 8);
              unsigned int numFonts = GetUINT(inputMessage + 8, bigEndian_);
              encodeBuffer.encodeValue(numFonts, 16, 6);

              // Differential encoding.
              encodeBuffer.encodeBoolValue(1);

              const unsigned char* nextSrc = inputMessage + 32;
              for (; numFonts; numFonts--)
              {
                unsigned int length = (unsigned int) (*nextSrc++);
                encodeBuffer.encodeValue(length, 8);

                if (control -> isProtoStep7() == 1)
                {
                  encodeBuffer.encodeTextData(nextSrc, length);

                  nextSrc += length;
                }
                else
                {
                  serverCache_ -> getPropertyTextCompressor.reset();
                  for (; length; length--)
                  {
                    serverCache_ -> getPropertyTextCompressor.encodeChar(
                                       *nextSrc++, encodeBuffer);
                  }
                }
              }

              priority_++;
            }
            break;
          case X_LookupColor:
          case X_AllocNamedColor:
            {
              const unsigned char *nextSrc = inputMessage + 8;
              if (requestOpcode == X_AllocNamedColor)
              {
                encodeBuffer.encodeValue(GetULONG(nextSrc, bigEndian_), 32, 9);
                nextSrc += 4;
              }
              unsigned int count = 3;
              do
              {
                unsigned int exactColor = GetUINT(nextSrc, bigEndian_);
                encodeBuffer.encodeValue(exactColor, 16, 9);
                unsigned int visualColor = GetUINT(nextSrc + 6, bigEndian_) -
                exactColor;
                encodeBuffer.encodeValue(visualColor, 16, 5);
                nextSrc += 2;
              }
              while (--count);

              priority_++;
            }
            break;
          case X_QueryBestSize:
            {
              encodeBuffer.encodeValue(GetUINT(inputMessage + 8, bigEndian_), 16, 8);
              encodeBuffer.encodeValue(GetUINT(inputMessage + 10, bigEndian_), 16, 8);

              priority_++;
            }
            break;
          case X_QueryColors:
            {
              // Differential encoding.
              encodeBuffer.encodeBoolValue(1);

              unsigned int numColors = ((inputLength - 32) >> 3);
              const unsigned char *nextSrc = inputMessage + 40;
              unsigned char *nextDest = (unsigned char *) inputMessage + 38;
              for (unsigned int c = 1; c < numColors; c++)
              {
                for (unsigned int i = 0; i < 6; i++)
                  *nextDest++ = *nextSrc++;
                nextSrc += 2;
              }
              unsigned int colorsLength = numColors * 6;
              if (serverCache_ -> queryColorsLastReply.compare(colorsLength,
                                                                   inputMessage + 32))
                encodeBuffer.encodeBoolValue(1);
              else
              {
                const unsigned char *nextSrc = inputMessage + 32;
                encodeBuffer.encodeBoolValue(0);
                encodeBuffer.encodeValue(numColors, 16, 5);
                for (numColors *= 3; numColors; numColors--)
                {
                  encodeBuffer.encodeValue(GetUINT(nextSrc, bigEndian_), 16);
                  nextSrc += 2;
                }
              }

              priority_++;
            }
            break;
          case X_QueryExtension:
            {
              if (requestData[0] == X_QueryExtension)
              {
                //
                // Value in requestData[0] will be nonzero
                // if the request is for an extension that
                // we should hide to the X client.
                //

                if (requestData[1])
                {
                  encodeBuffer.encodeBoolValue(0);
                  encodeBuffer.encodeValue(0, 8);
                }
                else
                {
                  encodeBuffer.encodeBoolValue((unsigned int) inputMessage[8]);
                  encodeBuffer.encodeValue((unsigned int) inputMessage[9], 8);
                }

                encodeBuffer.encodeValue((unsigned int) inputMessage[10], 8);
                encodeBuffer.encodeValue((unsigned int) inputMessage[11], 8);

                if (requestData[2] == X_NXInternalShapeExtension)
                {
                  opcodeStore_ -> shapeExtension = inputMessage[9];

                  #ifdef TEST
                  *logofs << "handleRead: Shape extension opcode for FD#" << fd_
                          << " is " << (unsigned int) opcodeStore_ -> shapeExtension
                          << ".\n" << logofs_flush;
                  #endif
                }
                else if (requestData[2] == X_NXInternalRenderExtension)
                {
                  opcodeStore_ -> renderExtension = inputMessage[9];

                  #ifdef TEST
                  *logofs << "handleRead: Render extension opcode for FD#" << fd_
                          << " is " << (unsigned int) opcodeStore_ -> renderExtension
                          << ".\n" << logofs_flush;
                  #endif
                }

                priority_++;
              }
              else
              {
                #ifdef TEST
                *logofs << "handleRead: Received tainted X_QueryExtension reply "
                        << "for request OPCODE#" << requestData[0] << " with "
                        << "sequence " << sequenceNum << ".\n"
                        << logofs_flush;
                #endif

                if (requestData[0] == opcodeStore_ -> getShmemParameters)
                {
                  if (handleShmemReply(encodeBuffer, requestOpcode, requestData[1],
                                           inputMessage, inputLength) < 0)
                  {
                    return -1;
                  }

                  priority_++;
                }
                //
                // Account this data to the original opcode.
                //

                requestOpcode = requestData[0];
              }
            }
            break;
          case X_QueryFont:
            {
              MessageStore *messageStore = serverStore_ ->
                                   getReplyStore(X_QueryFont);

              if (handleEncodeCached(encodeBuffer, serverCache_, messageStore,
                                         inputMessage, inputLength))
              {
                priority_++;

                hit = 1;

                break;
              }

              // Differential encoding.
              encodeBuffer.encodeBoolValue(1);

              unsigned int numProperties = GetUINT(inputMessage + 46, bigEndian_);
              unsigned int numCharInfos = GetULONG(inputMessage + 56, bigEndian_);
              encodeBuffer.encodeValue(numProperties, 16, 8);
              encodeBuffer.encodeValue(numCharInfos, 32, 10);
              handleEncodeCharInfo(inputMessage + 8, encodeBuffer);
              handleEncodeCharInfo(inputMessage + 24, encodeBuffer);
              encodeBuffer.encodeValue(GetUINT(inputMessage + 40, bigEndian_), 16, 9);
              encodeBuffer.encodeValue(GetUINT(inputMessage + 42, bigEndian_), 16, 9);
              encodeBuffer.encodeValue(GetUINT(inputMessage + 44, bigEndian_), 16, 9);
              encodeBuffer.encodeBoolValue((unsigned int) inputMessage[48]);
              encodeBuffer.encodeValue((unsigned int) inputMessage[49], 8);
              encodeBuffer.encodeValue((unsigned int) inputMessage[50], 8);
              encodeBuffer.encodeBoolValue((unsigned int) inputMessage[51]);
              encodeBuffer.encodeValue(GetUINT(inputMessage + 52, bigEndian_), 16, 9);
              encodeBuffer.encodeValue(GetUINT(inputMessage + 54, bigEndian_), 16, 9);
              const unsigned char *nextSrc = inputMessage + 60;
              unsigned int index;

              int end = 0;

              if (ServerCache::queryFontFontCache.lookup(numProperties * 8 +
                                        numCharInfos * 12, nextSrc, index))
              {
                encodeBuffer.encodeBoolValue(1);
                encodeBuffer.encodeValue(index, 4);

                end = 1;
              }

              if (end == 0)
              {
                encodeBuffer.encodeBoolValue(0);
                for (; numProperties; numProperties--)
                {
                  encodeBuffer.encodeValue(GetULONG(nextSrc, bigEndian_), 32, 9);
                  encodeBuffer.encodeValue(GetULONG(nextSrc + 4, bigEndian_), 32, 9);
                  nextSrc += 8;
                }
                for (; numCharInfos; numCharInfos--)
                {
                  handleEncodeCharInfo(nextSrc, encodeBuffer);

                  nextSrc += 12;
                }
              }

              priority_++;
            }
            break;
          case X_QueryPointer:
            {
              encodeBuffer.encodeBoolValue((unsigned int) inputMessage[1]);
              encodeBuffer.encodeCachedValue(GetULONG(inputMessage + 8, bigEndian_),
                                 29, serverCache_ -> queryPointerRootCache, 9);
              encodeBuffer.encodeCachedValue(GetULONG(inputMessage + 12, bigEndian_),
                                29, serverCache_ -> queryPointerChildCache, 9);
              unsigned int rootX = GetUINT(inputMessage + 16, bigEndian_);
              unsigned int rootY = GetUINT(inputMessage + 18, bigEndian_);
              unsigned int eventX = GetUINT(inputMessage + 20, bigEndian_);
              unsigned int eventY = GetUINT(inputMessage + 22, bigEndian_);
              eventX -= rootX;
              eventY -= rootY;
              encodeBuffer.encodeCachedValue(
                             rootX - serverCache_ -> motionNotifyLastRootX, 16,
                                    serverCache_ -> motionNotifyRootXCache, 8);
              serverCache_ -> motionNotifyLastRootX = rootX;
              encodeBuffer.encodeCachedValue(
                             rootY - serverCache_ -> motionNotifyLastRootY, 16,
                                    serverCache_ -> motionNotifyRootYCache, 8);
              serverCache_ -> motionNotifyLastRootY = rootY;
              encodeBuffer.encodeCachedValue(eventX, 16,
                                   serverCache_ -> motionNotifyEventXCache, 8);
              encodeBuffer.encodeCachedValue(eventY, 16,
                                   serverCache_ -> motionNotifyEventYCache, 8);
              encodeBuffer.encodeCachedValue(GetUINT(inputMessage + 24, bigEndian_),
                                   16, serverCache_ -> motionNotifyStateCache);
              priority_++;
            }
            break;
          case X_QueryTree:
            {
              //
              // This was very inefficient. In practice
              // it just copied data on the output. Now
              // it obtains an average 7:1 compression
              // and could optionally be cached.
              //

              unsigned int children = GetUINT(inputMessage + 16, bigEndian_);

              encodeBuffer.encodeValue(children, 16, 8);

              encodeBuffer.encodeCachedValue(GetULONG(inputMessage + 8, bigEndian_), 29,
                                 serverCache_ -> queryTreeWindowCache);

              encodeBuffer.encodeCachedValue(GetULONG(inputMessage + 12, bigEndian_), 29,
                                 serverCache_ -> queryTreeWindowCache);

              const unsigned char *next = inputMessage + 32;

              for (unsigned int i = 0; i < children; i++)
              {
                encodeBuffer.encodeCachedValue(GetULONG(next + (i * 4), bigEndian_), 29,
                                   serverCache_ -> queryTreeWindowCache);
              }

              priority_++;
            }
            break;
          case X_TranslateCoords:
            {
              encodeBuffer.encodeBoolValue((unsigned int) inputMessage[1]);
              encodeBuffer.encodeCachedValue(GetULONG(inputMessage + 8, bigEndian_),
                             29, serverCache_ -> translateCoordsChildCache, 9);
              encodeBuffer.encodeCachedValue(GetUINT(inputMessage + 12, bigEndian_),
                                 16, serverCache_ -> translateCoordsXCache, 8);
              encodeBuffer.encodeCachedValue(GetUINT(inputMessage + 14, bigEndian_),
                                 16, serverCache_ -> translateCoordsYCache, 8);
              priority_++;
            }
            break;
          case X_GetImage:
            {
              MessageStore *messageStore = serverStore_ ->
                                   getReplyStore(X_GetImage);

              if (handleEncodeCached(encodeBuffer, serverCache_, messageStore,
                                         inputMessage, inputLength))
              {
                priority_++;

                hit = 1;

                break;
              }

              // Depth.
              encodeBuffer.encodeCachedValue(inputMessage[1], 8,
                                 serverCache_ -> depthCache);
              // Reply length.
              encodeBuffer.encodeValue(GetULONG(inputMessage + 4, bigEndian_), 32, 9);

              // Visual.
              encodeBuffer.encodeCachedValue(GetULONG(inputMessage + 8, bigEndian_), 29,
                                 serverCache_ -> visualCache);

              if (control -> isProtoStep8() == 0)
              {
                unsigned int compressedDataSize = 0;
                unsigned char *compressedData   = NULL;

                int compressed = handleCompress(encodeBuffer, requestOpcode, messageStore -> dataOffset,
                                                    inputMessage, inputLength, compressedData,
                                                        compressedDataSize);
                if (compressed < 0)
                {
                  return -1;
                }
                else if (compressed > 0)
                {
                  //
                  // Update size according to result of image compression.
                  //

                  handleUpdate(messageStore, inputLength - messageStore ->
                                   dataOffset, compressedDataSize);
                }
              }
              else
              {
                handleCopy(encodeBuffer, requestOpcode, messageStore ->
                              dataOffset, inputMessage, inputLength);
              }

              priority_++;
            }
            break;
            case X_GetPointerMapping:
            {
              encodeBuffer.encodeValue(inputMessage[1], 8, 4);
              encodeBuffer.encodeValue(GetULONG(inputMessage + 4, bigEndian_), 32, 4);
              for (unsigned int i = 32; i < inputLength; i++)
                encodeBuffer.encodeValue((unsigned int) inputMessage[i], 8, 4);

              priority_++;
            }
            break;
            case X_GetKeyboardControl:
            {
              encodeBuffer.encodeValue(inputMessage[1], 8, 2);
              encodeBuffer.encodeValue(GetULONG(inputMessage + 4, bigEndian_), 32, 8);
              for (unsigned int i = 8; i < inputLength; i++)
                encodeBuffer.encodeValue((unsigned int) inputMessage[i], 8, 4);

              priority_++;
            }
            break;
          default:
            {
              #ifdef PANIC
              *logofs << "ServerChannel: PANIC! No matching request with "
                      << "OPCODE#" << (unsigned int) requestOpcode
                      << " for reply with sequence number "
                      << requestSequenceNum << ".\n"
                      << logofs_flush;
              #endif

              cerr << "Error" << ": No matching request with OPCODE#"
                   << (unsigned int) requestOpcode << " for reply with "
                   << "sequence number " << requestSequenceNum << ".\n";

              return -1;
            }
          }

          bits = encodeBuffer.diffBits();

          #if defined(TEST) || defined(OPCODES)

          const char *cacheString = (hit ? "cached " : "");

          *logofs << "handleRead: Handled " << cacheString << "reply to OPCODE#"
                  << (unsigned int) requestOpcode << " (" << DumpOpcode(requestOpcode)
                  << ") for FD#" << fd_ << " sequence " << serverSequence_
                  << ". " << inputLength << " bytes in, " << bits << " bits ("
                  << ((float) bits) / 8 << " bytes) out.\n" << logofs_flush;

          #endif

        } // End of if (sequenceQueue_.peek(requestSequenceNum, requestOpcode) && ...
        else
        {
          //
          // We didn't push the request opcode.
          // Check if fast encoding is required.
          //

          requestOpcode = X_Reply;

          if (control -> LocalDeltaCompression == 0)
          {
            int result = handleFastReadReply(encodeBuffer, requestOpcode,
                                                 inputMessage, inputLength);
            if (result < 0)
            {
              return -1;
            }
            else if (result > 0)
            {
              continue;
            }
          }

          //
          // Group all replies whose opcode was not
          // pushed in sequence number queue under
          // the category 'generic reply'.
          //

          #ifdef DEBUG
          *logofs << "handleRead: Identified generic reply.\n"
                  << logofs_flush;
          #endif

          MessageStore *messageStore = serverStore_ ->
                               getReplyStore(X_NXInternalGenericReply);

          hit = handleEncode(encodeBuffer, serverCache_, messageStore,
                                 requestOpcode, inputMessage, inputLength);

          priority_++;

          bits = encodeBuffer.diffBits();

          #if defined(TEST) || defined(OPCODES)

          const char *cacheString = (hit ? "cached " : "");

          *logofs << "handleRead: Handled " << cacheString << "generic reply "
                  << "OPCODE#" << X_NXInternalGenericReply << " for FD#" << fd_
                  << " sequence " << serverSequence_ << ". " << inputLength
                  << " bytes in, " << bits << " bits (" << ((float) bits) / 8
                  << " bytes) out.\n" << logofs_flush;

          #endif
        }

        if (hit)
        {
          statistics -> addCachedReply(requestOpcode);
        }

        statistics -> addReplyBits(requestOpcode, inputLength << 3, bits);

      } // End of if (inputMessage[0] == 1) ...
      else
      {
        //
        // Event or error.
        //

        unsigned char inputOpcode = *inputMessage;

        unsigned int inputSequence = GetUINT(inputMessage + 2, bigEndian_);

        //
        // Check if this is an event which we can discard.
        //

        if ((inputOpcode == Expose && enableExpose_ == 0) ||
                (inputOpcode == GraphicsExpose && enableGraphicsExpose_ == 0) ||
                    (inputOpcode == NoExpose && enableNoExpose_ == 0))
        {
          continue;
        }
        else if (shmemState_ != NULL && shmemState_ -> enabled == 1 &&
                     inputOpcode == shmemState_ -> event &&
                         checkShmemEvent(inputOpcode, inputSequence,
                             inputMessage) > 0)
        {
          continue;
        }
        else if (inputOpcode == MotionNotify)
        {
          //
          // Save the motion event and send when another
          // event or error is received or the motion ti-
          // meout is elapsed. If a previous motion event
          // was already saved, we replace it with the
          // new one and don't reset the timeout, so we
          // still have a motion event every given ms.
          //

          memcpy(lastMotion_, inputMessage, 32);

          #ifdef TEST
          *logofs << "handleRead: Saved suppressed motion event for FD#"
                  << fd_ << ".\n" << logofs_flush;
          #endif

          continue;
        }
        else if (inputOpcode == X_Error)
        {
          //
          // Check if this is an error that matches a
          // sequence number for which we are expecting
          // a reply.
          //

          unsigned short int errorSequenceNum;
          unsigned char errorOpcode;

          if (sequenceQueue_.peek(errorSequenceNum, errorOpcode) &&
                  ((unsigned int) errorSequenceNum == inputSequence))
          {
            sequenceQueue_.pop(errorSequenceNum, errorOpcode);
          }

          //
          // Check if error is due to an image commit
          // generated at the end of a split.
          //

          if (checkCommitError(*(inputMessage + 1), inputSequence, inputMessage) > 0)
          {
            #ifdef TEST
            *logofs << "handleRead: Skipping error on image commit for FD#"
                    << fd_ << ".\n" << logofs_flush;
            #endif

            continue;
          }

          //
          // Check if it's an error generated by a request
          // concerning shared memory support.
          //

          else if (shmemState_ != NULL && (shmemState_ -> sequence ==
                        inputSequence || (shmemState_ -> enabled == 1 &&
                            (shmemState_ -> opcode == *(inputMessage + 10) ||
                                shmemState_ -> error == *(inputMessage + 1)))) &&
                                    checkShmemError(*(inputMessage + 1), inputSequence,
                                        inputMessage) > 0)
          {
            #ifdef TEST
            *logofs << "handleRead: Skipping error on shmem operation for FD#"
                    << fd_ << ".\n" << logofs_flush;
            #endif

            continue;
          }
        }
        //
        // Check if user pressed the CTRL+ALT+SHIFT+ESC key
        // sequence because was unable to kill the session
        // through the normal procedure.
        //

        if (inputOpcode == KeyPress)
        {
          if (checkKeyboardEvent(inputOpcode, inputSequence, inputMessage) == 1)
          {
            #ifdef TEST
            *logofs << "handleRead: Removing the key sequence from the "
                    << "event stream for FD#" << fd_ << ".\n"
                    << logofs_flush;
            #endif

            continue;
          }
        }

        //
        // We are going to handle an event or error
        // that's not a mouse motion. Prepend any
        // saved motion to it.
        //

        if (lastMotion_[0] != '\0')
        {
          if (handleMotion(encodeBuffer) < 0)
          {
            #ifdef PANIC
            *logofs << "handleRead: PANIC! Can't encode motion event for FD#"
                    << fd_ << ".\n" << logofs_flush;
            #endif

            cerr << "Error" << ": Can't encode motion event for FD#"
                 << fd_ << ".\n";

            return -1;
          }
        }

        //
        // Encode opcode and difference between
        // current sequence and the last one.
        //

        encodeBuffer.encodeOpcodeValue(inputOpcode, serverCache_ -> opcodeCache);

        unsigned int sequenceDiff = inputSequence - serverSequence_;

        serverSequence_ = inputSequence;

        #ifdef DEBUG
        *logofs << "handleRead: Last server sequence number for FD#" 
                << fd_ << " is " << serverSequence_ << " with "
                << "difference " << sequenceDiff << ".\n"
                << logofs_flush;
        #endif

        encodeBuffer.encodeCachedValue(sequenceDiff, 16,
                           serverCache_ -> eventSequenceCache, 7);

        //
        // If differential compression is disabled
        // then use the most simple encoding.
        //

        if (control -> LocalDeltaCompression == 0)
        {
          int result = handleFastReadEvent(encodeBuffer, inputOpcode,
                                               inputMessage, inputLength);
          if (result < 0)
          {
            return -1;
          }
          else if (result > 0)
          {
            continue;
          }
        }

        switch (inputOpcode)
        {
        case X_Error:
          {
            //
            // Set the priority flag in the case of
            // a X protocol error. This may restart
            // the client if it was waiting for the
            // reply.
            //

            priority_++;

            unsigned char errorCode = *(inputMessage + 1);

            encodeBuffer.encodeCachedValue(errorCode, 8,
                               serverCache_ -> errorCodeCache);

            if (errorCode != 11 && errorCode != 8 &&
                    errorCode != 15 && errorCode != 1)
            {
              encodeBuffer.encodeValue(GetULONG(inputMessage + 4, bigEndian_), 32, 16);
            }

            if (errorCode >= 18)
            {
              encodeBuffer.encodeCachedValue(GetUINT(inputMessage + 8, bigEndian_), 16,
                                 serverCache_ -> errorMinorCache);
            }

            encodeBuffer.encodeCachedValue(inputMessage[10], 8,
                               serverCache_ -> errorMajorCache);

            if (errorCode >= 18)
            {
              const unsigned char *nextSrc = inputMessage + 11;
              for (unsigned int i = 11; i < 32; i++)
                encodeBuffer.encodeValue(*nextSrc++, 8);
            }
          }
          break;
        case ButtonPress:
        case ButtonRelease:
        case KeyPress:
        case KeyRelease:
        case MotionNotify:
        case EnterNotify:
        case LeaveNotify:
          {
            //
            // Set the priority in the case this is
            // an event that the remote side may
            // care to receive as soon as possible.
            //

            switch (inputOpcode)
            {
              case ButtonPress:
              case ButtonRelease:
              case KeyPress:
              case KeyRelease:
              {
                priority_++;
              }
            }

            unsigned char detail = inputMessage[1];
            if (*inputMessage == MotionNotify)
              encodeBuffer.encodeBoolValue((unsigned int) detail);
            else if ((*inputMessage == EnterNotify) || (*inputMessage == LeaveNotify))
              encodeBuffer.encodeValue((unsigned int) detail, 3);
            else if (*inputMessage == KeyRelease)
            {
              if (detail == serverCache_ -> keyPressLastKey)
                encodeBuffer.encodeBoolValue(1);
              else
              {
                encodeBuffer.encodeBoolValue(0);
                encodeBuffer.encodeValue((unsigned int) detail, 8);
              }
            }
            else if ((*inputMessage == ButtonPress) || (*inputMessage == ButtonRelease))
              encodeBuffer.encodeCachedValue(detail, 8,
                                             serverCache_ -> buttonCache);
            else
              encodeBuffer.encodeValue((unsigned int) detail, 8);
            unsigned int timestamp = GetULONG(inputMessage + 4, bigEndian_);
            unsigned int timestampDiff =
            timestamp - serverCache_ -> lastTimestamp;
            serverCache_ -> lastTimestamp = timestamp;
            encodeBuffer.encodeCachedValue(timestampDiff, 32,
                                serverCache_ -> motionNotifyTimestampCache, 9);
            int skipRest = 0;
            if (*inputMessage == KeyRelease)
            {
              skipRest = 1;
              for (unsigned int i = 8; i < 31; i++)
              {
                if (inputMessage[i] != serverCache_ -> keyPressCache[i - 8])
                {
                  skipRest = 0;
                  break;
                }
              }
              encodeBuffer.encodeBoolValue(skipRest);
            }

            if (!skipRest)
            {
              const unsigned char *nextSrc = inputMessage + 8;
              for (unsigned int i = 0; i < 3; i++)
              {
                encodeBuffer.encodeCachedValue(GetULONG(nextSrc, bigEndian_), 29,
                                   *serverCache_ -> motionNotifyWindowCache[i], 6);
                nextSrc += 4;
              }
              unsigned int rootX  = GetUINT(inputMessage + 20, bigEndian_);
              unsigned int rootY  = GetUINT(inputMessage + 22, bigEndian_);
              unsigned int eventX = GetUINT(inputMessage + 24, bigEndian_);
              unsigned int eventY = GetUINT(inputMessage + 26, bigEndian_);
              eventX -= rootX;
              eventY -= rootY;
              encodeBuffer.encodeCachedValue(rootX -
                    serverCache_ -> motionNotifyLastRootX, 16,
                    serverCache_ -> motionNotifyRootXCache, 6);
              serverCache_ -> motionNotifyLastRootX = rootX;
              encodeBuffer.encodeCachedValue(rootY -
                    serverCache_ -> motionNotifyLastRootY, 16,
                    serverCache_ -> motionNotifyRootYCache, 6);
              serverCache_ -> motionNotifyLastRootY = rootY;
              encodeBuffer.encodeCachedValue(eventX, 16,
                                   serverCache_ -> motionNotifyEventXCache, 6);
              encodeBuffer.encodeCachedValue(eventY, 16,
                                   serverCache_ -> motionNotifyEventYCache, 6);
              encodeBuffer.encodeCachedValue(GetUINT(inputMessage + 28, bigEndian_),
                                   16, serverCache_ -> motionNotifyStateCache);
              if ((*inputMessage == EnterNotify) || (*inputMessage == LeaveNotify))
                encodeBuffer.encodeValue((unsigned int) inputMessage[30], 2);
              else
                encodeBuffer.encodeBoolValue((unsigned int) inputMessage[30]);
              if ((*inputMessage == EnterNotify) || (*inputMessage == LeaveNotify))
                encodeBuffer.encodeValue((unsigned int) inputMessage[31], 2);
              else if (*inputMessage == KeyPress)
              {
                serverCache_ -> keyPressLastKey = detail;
                for (unsigned int i = 8; i < 31; i++)
                {
                  serverCache_ -> keyPressCache[i - 8] = inputMessage[i];
                }
              }
            }
          }
          break;
        case ColormapNotify:
          {
            encodeBuffer.encodeCachedValue(GetULONG(inputMessage + 4, bigEndian_),
                             29, serverCache_ -> colormapNotifyWindowCache, 8);
            encodeBuffer.encodeCachedValue(GetULONG(inputMessage + 8, bigEndian_),
                           29, serverCache_ -> colormapNotifyColormapCache, 8);
            encodeBuffer.encodeBoolValue((unsigned int) inputMessage[12]);
            encodeBuffer.encodeBoolValue((unsigned int) inputMessage[13]);
          }
          break;
        case ConfigureNotify:
          {
            const unsigned char *nextSrc = inputMessage + 4;
            for (unsigned int i = 0; i < 3; i++)
            {
              encodeBuffer.encodeCachedValue(GetULONG(nextSrc, bigEndian_), 29,
                                 *serverCache_ -> configureNotifyWindowCache[i], 9);
              nextSrc += 4;
            }
            for (unsigned int j = 0; j < 5; j++)
            {
              encodeBuffer.encodeCachedValue(GetUINT(nextSrc, bigEndian_), 16,
                                 *serverCache_ -> configureNotifyGeomCache[j], 8);
              nextSrc += 2;
            }
            encodeBuffer.encodeBoolValue(*nextSrc);
          }
          break;
        case CreateNotify:
          {
            encodeBuffer.encodeCachedValue(GetULONG(inputMessage + 4, bigEndian_),
                        29, serverCache_ -> createNotifyWindowCache, 9);
            unsigned int window = GetULONG(inputMessage + 8, bigEndian_);
            encodeBuffer.encodeValue(window -
                             serverCache_ -> createNotifyLastWindow, 29, 5);
            serverCache_ -> createNotifyLastWindow = window;
            const unsigned char* nextSrc = inputMessage + 12;
            for (unsigned int i = 0; i < 5; i++)
            {
              encodeBuffer.encodeValue(GetUINT(nextSrc, bigEndian_), 16, 9);
              nextSrc += 2;
            }
            encodeBuffer.encodeBoolValue(*nextSrc);
          }
          break;
        case Expose:
          {
            encodeBuffer.encodeCachedValue(GetULONG(inputMessage + 4, bigEndian_), 29,
                               serverCache_ -> exposeWindowCache, 9);
            const unsigned char *nextSrc = inputMessage + 8;
            for (unsigned int i = 0; i < 5; i++)
            {
              encodeBuffer.encodeCachedValue(GetUINT(nextSrc, bigEndian_), 16,
                                 *serverCache_ -> exposeGeomCache[i], 6);
              nextSrc += 2;
            }
          }
          break;
        case FocusIn:
        case FocusOut:
          {
            encodeBuffer.encodeValue((unsigned int) inputMessage[1], 3);
            encodeBuffer.encodeCachedValue(GetULONG(inputMessage + 4, bigEndian_),
                                    29, serverCache_ -> focusInWindowCache, 9);
            encodeBuffer.encodeValue((unsigned int) inputMessage[8], 2);
          }
          break;
        case KeymapNotify:
          {
          if (ServerCache::lastKeymap.compare(31, inputMessage + 1))
              encodeBuffer.encodeBoolValue(1);
            else
            {
              encodeBuffer.encodeBoolValue(0);
              const unsigned char *nextSrc = inputMessage + 1;
              for (unsigned int i = 1; i < 32; i++)
                encodeBuffer.encodeValue((unsigned int) *nextSrc++, 8);
            }
          }
          break;
        case MapNotify:
        case UnmapNotify:
        case DestroyNotify:
          {
            encodeBuffer.encodeCachedValue(GetULONG(inputMessage + 4, bigEndian_),
                                   29, serverCache_ -> mapNotifyEventCache, 9);
            encodeBuffer.encodeCachedValue(GetULONG(inputMessage + 8, bigEndian_),
                                  29, serverCache_ -> mapNotifyWindowCache, 9);
            if ((*inputMessage == MapNotify) || (*inputMessage == UnmapNotify))
              encodeBuffer.encodeBoolValue((unsigned int) inputMessage[12]);
          }
          break;
        case NoExpose:
          {
            encodeBuffer.encodeCachedValue(GetULONG(inputMessage + 4, bigEndian_),
                                 29, serverCache_ -> noExposeDrawableCache, 9);
            encodeBuffer.encodeCachedValue(GetUINT(inputMessage + 8, bigEndian_), 16,
                                           serverCache_ -> noExposeMinorCache);
            encodeBuffer.encodeCachedValue(inputMessage[10], 8,
                                           serverCache_ -> noExposeMajorCache);
          }
          break;
        case PropertyNotify:
          {
            encodeBuffer.encodeCachedValue(GetULONG(inputMessage + 4, bigEndian_),
                             29, serverCache_ -> propertyNotifyWindowCache, 9);
            encodeBuffer.encodeCachedValue(GetULONG(inputMessage + 8, bigEndian_),
                               29, serverCache_ -> propertyNotifyAtomCache, 9);
            unsigned int timestamp = GetULONG(inputMessage + 12, bigEndian_);
            unsigned int timestampDiff =
            timestamp - serverCache_ -> lastTimestamp;
            serverCache_ -> lastTimestamp = timestamp;
            encodeBuffer.encodeValue(timestampDiff, 32, 9);
            encodeBuffer.encodeBoolValue((unsigned int) inputMessage[16]);
          }
          break;
        case ReparentNotify:
          {
            const unsigned char* nextSrc = inputMessage + 4;
            for (unsigned int i = 0; i < 3; i++)
            {
              encodeBuffer.encodeCachedValue(GetULONG(nextSrc, bigEndian_),
                     29, serverCache_ -> reparentNotifyWindowCache, 9);
              nextSrc += 4;
            }
            encodeBuffer.encodeValue(GetUINT(nextSrc, bigEndian_), 16, 6);
            encodeBuffer.encodeValue(GetUINT(nextSrc + 2, bigEndian_), 16, 6);
            encodeBuffer.encodeBoolValue((unsigned int)inputMessage[20]);
          }
          break;
        case SelectionClear:
          {
            unsigned int timestamp = GetULONG(inputMessage + 4, bigEndian_);
            unsigned int timestampDiff = timestamp - serverCache_ -> lastTimestamp;
            serverCache_ -> lastTimestamp = timestamp;
            encodeBuffer.encodeValue(timestampDiff, 32, 9);
            encodeBuffer.encodeCachedValue(GetULONG(inputMessage + 8, bigEndian_),
                           29, serverCache_ -> selectionClearWindowCache, 9);
            encodeBuffer.encodeCachedValue(GetULONG(inputMessage + 12, bigEndian_),
                           29, serverCache_ -> selectionClearAtomCache, 9);
          }
          break;
        case SelectionRequest:
          {
            unsigned int timestamp = GetULONG(inputMessage + 4, bigEndian_);
            unsigned int timestampDiff = timestamp - serverCache_ -> lastTimestamp;
            serverCache_ -> lastTimestamp = timestamp;
            encodeBuffer.encodeValue(timestampDiff, 32, 9);
            encodeBuffer.encodeCachedValue(GetULONG(inputMessage + 8, bigEndian_),
                           29, serverCache_ -> selectionClearWindowCache, 9);
            encodeBuffer.encodeCachedValue(GetULONG(inputMessage + 12, bigEndian_),
                           29, serverCache_ -> selectionClearWindowCache, 9);
            encodeBuffer.encodeCachedValue(GetULONG(inputMessage + 16, bigEndian_),
                           29, serverCache_ -> selectionClearAtomCache, 9);
            encodeBuffer.encodeCachedValue(GetULONG(inputMessage + 20, bigEndian_),
                           29, serverCache_ -> selectionClearAtomCache, 9);
            encodeBuffer.encodeCachedValue(GetULONG(inputMessage + 24, bigEndian_),
                           29, serverCache_ -> selectionClearAtomCache, 9);
          }
          break;
        case VisibilityNotify:
          {
            encodeBuffer.encodeCachedValue(GetULONG(inputMessage + 4, bigEndian_),
                           29, serverCache_ -> visibilityNotifyWindowCache, 9);
            encodeBuffer.encodeValue((unsigned int) inputMessage[8], 2);
          }
          break;
        default:
          {
            #ifdef TEST
            *logofs << "handleRead: Using generic event compression "
                    << "for OPCODE#" << (unsigned int) inputOpcode
                    << ".\n" << logofs_flush;
            #endif

            encodeBuffer.encodeCachedValue(*(inputMessage + 1), 8,
                         serverCache_ -> genericEventCharCache);

            for (unsigned int i = 0; i < 14; i++)
            {
              encodeBuffer.encodeCachedValue(GetUINT(inputMessage + i * 2 + 4, bigEndian_),
                                 16, *serverCache_ -> genericEventIntCache[i]);
            }
          }

        } // switch (inputOpcode)...

        int bits = encodeBuffer.diffBits();

        #if defined(TEST) || defined(OPCODES)

        if (*inputMessage == X_Error)
        {
          unsigned char code = *(inputMessage + 1);

          *logofs << "handleRead: Handled error ERR_CODE#"
                  << (unsigned int) code << " for FD#" << fd_;

          *logofs << " RES_ID#" << GetULONG(inputMessage + 4, bigEndian_);

          *logofs << " MIN_OP#" << GetUINT(inputMessage + 8, bigEndian_);

          *logofs << " MAJ_OP#" << (unsigned int) *(inputMessage + 10);

          *logofs << " sequence " << inputSequence << ". " << inputLength
                  << " bytes in, " << bits << " bits (" << ((float) bits) / 8
                  << " bytes) out.\n" << logofs_flush;
        }
        else
        {
          *logofs << "handleRead: Handled event OPCODE#"
                  << (unsigned int) *inputMessage << " for FD#" << fd_
                  << " sequence " << inputSequence << ". " << inputLength
                  << " bytes in, " << bits << " bits (" << ((float) bits) / 8
                  << " bytes) out.\n" << logofs_flush;
        }

        #endif

        statistics -> addEventBits(*inputMessage, inputLength << 3, bits);

      } // End of if (inputMessage[0] == X_Reply) ... else ...

    } // End of if (firstReply_) ... else ...

  } // End of while ((inputMessage = readBuffer_.getMessage(inputLength)) != 0) ...

  //
  // Check if we need to flush because of
  // prioritized data.
  //

  if (priority_ > 0)
  {
    #if defined(TEST) || defined(INFO)
    *logofs << "handleRead: WARNING! Requesting flush "
            << "because of " << priority_ << " prioritized "
            << "messages for FD#" << fd_ << ".\n"
            << logofs_flush;
    #endif

    if (proxy -> handleAsyncPriority() < 0)
    {
      return -1;
    }

    //
    // Reset the priority flag.
    //

    priority_ = 0;
  }

  //
  // Flush if we produced enough data.
  //

  if (proxy -> canAsyncFlush() == 1)
  {
    #if defined(TEST) || defined(INFO)
    *logofs << "handleRead: WARNING! Requesting flush "
            << "because of token length exceeded.\n"
            << logofs_flush;
    #endif

    if (proxy -> handleAsyncFlush() < 0)
    {
      return -1;
    }
  }

  #if defined(TEST) || defined(INFO)

  if (transport_ -> pending() != 0 ||
          readBuffer_.checkMessage() != 0)
  {
    *logofs << "handleRead: PANIC! Buffer for X descriptor FD#"
            << fd_ << " has " << transport_ -> pending()
            << " bytes to read.\n" << logofs_flush;

    HandleCleanup();
  }

  #endif

  //
  // Reset the read buffer.
  //

  readBuffer_.fullReset();

  return 1;
}

//
// End of handleRead().
//

//
// Beginning of handleWrite().
//

int ServerChannel::handleWrite(const unsigned char *message, unsigned int length)
{
  #ifdef TEST
  *logofs << "handleWrite: Called for FD#" << fd_
          << ".\n" << logofs_flush;
  #endif

  //
  // Create the buffer from which to 
  // decode messages.
  //

  DecodeBuffer decodeBuffer(message, length);

  #if defined(TEST) || defined(INFO) || defined(FLUSH)
  *logofs << "handleWrite: Decoding messages for FD#" << fd_
          << " with " << length << " bytes in the buffer.\n"
          << logofs_flush;
  #endif

  if (firstRequest_)
  {
    //
    // Need to add the length of the first request
    // because it was not present in the previous
    // versions. Length of the first request was
    // assumed to be the same as the encode buffer
    // but this may be not the case if a different
    // encoding is used.
    //

    if (control -> isProtoStep7() == 1)
    {
      decodeBuffer.decodeValue(length, 8);
    }

    unsigned int nextByte;
    unsigned char *outputMessage = writeBuffer_.addMessage(length);
    unsigned char *nextDest = outputMessage;

    for (unsigned int i = 0; i < length; i++)
    {
      decodeBuffer.decodeValue(nextByte, 8);

      *nextDest++ = (unsigned char) nextByte;
    }

    if (*outputMessage == 0x42)
    {
      setBigEndian(1);
    }
    else
    {
      setBigEndian(0);
    }

    #ifdef TEST
    *logofs << "handleWrite: First request detected.\n" << logofs_flush;
    #endif

    //
    // Handle the fake authorization cookie.
    //

    if (handleAuthorization(outputMessage) < 0)
    {
      return -1;
    }

    firstRequest_ = 0;

  } // End of if (firstRequest_)

  //
  // This was previously in a 'else' block.
  // Due to the way the first request was
  // handled, we could not decode multiple
  // messages in the first frame.
  //

  { // Start of the decoding block.

    unsigned char outputOpcode;

    unsigned char *outputMessage;
    unsigned int outputLength;

    //
    // Set when message is found in cache.
    //

    int hit;

    while (decodeBuffer.decodeOpcodeValue(outputOpcode, clientCache_ -> opcodeCache, 1))
    {
      hit = 0;

      //
      // Splits are sent by client proxy outside the
      // normal read loop. As we 'insert' splits in
      // the real client-server X protocol, we must
      // avoid to increment the sequence number or
      // our clients would get confused.
      //

      if (outputOpcode != opcodeStore_ -> splitData)
      {
        clientSequence_++;
        clientSequence_ &= 0xffff;

        #ifdef DEBUG
        *logofs << "handleWrite: Last client sequence number for FD#" 
                << fd_ << " is " << clientSequence_ << ".\n"
                << logofs_flush;
        #endif
      }
      else
      {
        //
        // It's a split, not a normal
        // burst of proxy data.
        //

        handleSplit(decodeBuffer);

        continue;
      }

      #ifdef SUSPEND

      if (clientSequence_ == 1000)
      {
        cerr << "Warning" << ": Exiting to test the resilience of the agent.\n";

        sleep(2);

        HandleAbort();
      }

      #endif

      //
      // Is differential encoding disabled?
      //

      if (control -> RemoteDeltaCompression == 0)
      {
        int result = handleFastWriteRequest(decodeBuffer, outputOpcode,
                                                outputMessage, outputLength);
        if (result < 0)
        {
          return -1;
        }
        else if (result > 0)
        {
          continue;
        }
      }

      //
      // General-purpose temp variables for
      // decoding ints and chars.
      //

      unsigned int value;
      unsigned char cValue;

      #ifdef DEBUG
      *logofs << "handleWrite: Going to handle request OPCODE#"
              << (unsigned int) outputOpcode << " (" << DumpOpcode(outputOpcode)
              << ") for FD#" << fd_ << " sequence " << clientSequence_
              << ".\n" << logofs_flush;
      #endif

      switch (outputOpcode)
      {
      case X_AllocColor:
        {
          outputLength = 16;
          outputMessage = writeBuffer_.addMessage(outputLength);
          decodeBuffer.decodeCachedValue(value, 29,
                             clientCache_ -> colormapCache);
          PutULONG(value, outputMessage + 4, bigEndian_);
          unsigned char *nextDest = outputMessage + 8;
          unsigned int colorData[3];

          for (unsigned int i = 0; i < 3; i++)
          {
            decodeBuffer.decodeCachedValue(value, 16,
                               *(clientCache_ -> allocColorRGBCache[i]), 4);
            PutUINT(value, nextDest, bigEndian_);
            colorData[i] = value;
            nextDest += 2;
          }

          sequenceQueue_.push(clientSequence_, outputOpcode,
                                  colorData[0], colorData[1], colorData[2]);
        }
        break;
      case X_ReparentWindow:
        {
          outputLength = 16;
          outputMessage = writeBuffer_.addMessage(outputLength);
          decodeBuffer.decodeXidValue(value, clientCache_ -> windowCache);
          PutULONG(value, outputMessage + 4, bigEndian_);
          decodeBuffer.decodeXidValue(value, clientCache_ -> windowCache);
          PutULONG(value, outputMessage + 8, bigEndian_);
          decodeBuffer.decodeValue(value, 16, 11);
          PutUINT(value, outputMessage + 12, bigEndian_);
          decodeBuffer.decodeValue(value, 16, 11);
          PutUINT(value, outputMessage + 14, bigEndian_);
        }
        break;
      case X_ChangeProperty:
        {
          MessageStore *messageStore = clientStore_ ->
                               getRequestStore(X_ChangeProperty);

          if (handleDecodeCached(decodeBuffer, clientCache_, messageStore,
                                     outputMessage, outputLength))
          {
            break;
          }

          unsigned char format;
          decodeBuffer.decodeCachedValue(format, 8,
                             clientCache_ -> changePropertyFormatCache);
          unsigned int dataLength;
          decodeBuffer.decodeValue(dataLength, 32, 6);
          outputLength = 24 + RoundUp4(dataLength * (format >> 3));
          outputMessage = writeBuffer_.addMessage(outputLength);
          decodeBuffer.decodeValue(value, 2);
          outputMessage[1] = (unsigned char) value;
          decodeBuffer.decodeXidValue(value, clientCache_ -> windowCache);
          PutULONG(value, outputMessage + 4, bigEndian_);
          decodeBuffer.decodeCachedValue(value, 29,
                             clientCache_ -> changePropertyPropertyCache, 9);
          PutULONG(value, outputMessage + 8, bigEndian_);
          decodeBuffer.decodeCachedValue(value, 29,
                             clientCache_ -> changePropertyTypeCache, 9);
          PutULONG(value, outputMessage + 12, bigEndian_);
          outputMessage[16] = format;
          PutULONG(dataLength, outputMessage + 20, bigEndian_);
          unsigned char *nextDest = outputMessage + 24;

          if (format == 8)
          {
            if (control -> isProtoStep7() == 1)
            {
              decodeBuffer.decodeTextData(nextDest, dataLength);
            }
            else
            {
              clientCache_ -> changePropertyTextCompressor.reset();
              for (unsigned int i = 0; i < dataLength; i++)
              {
                *nextDest++ = clientCache_ -> changePropertyTextCompressor.
                                                    decodeChar(decodeBuffer);
              }
            }
          }
          else if (format == 32)
          {
            for (unsigned int i = 0; i < dataLength; i++)
            {
              decodeBuffer.decodeCachedValue(value, 32,
                                 clientCache_ -> changePropertyData32Cache);

              PutULONG(value, nextDest, bigEndian_);

              nextDest += 4;
            }
          }
          else
          {
            for (unsigned int i = 0; i < dataLength; i++)
            {
              decodeBuffer.decodeValue(value, 16);

              PutUINT(value, nextDest, bigEndian_);

              nextDest += 2;
            }
          }

          handleSave(messageStore, outputMessage, outputLength);
        }
        break;
      case X_SendEvent:
        {
          MessageStore *messageStore = clientStore_ ->
                               getRequestStore(X_SendEvent);

          if (handleDecodeCached(decodeBuffer, clientCache_, messageStore,
                                     outputMessage, outputLength))
          {
            break;
          }

          outputLength = 44;
          outputMessage = writeBuffer_.addMessage(outputLength);
          decodeBuffer.decodeBoolValue(value);
          *(outputMessage + 1) = value;
          decodeBuffer.decodeBoolValue(value);
          if (value)
          {
            decodeBuffer.decodeBoolValue(value);
          }
          else
          {
            decodeBuffer.decodeXidValue(value, clientCache_ -> windowCache);
          }
          PutULONG(value, outputMessage + 4, bigEndian_);
          decodeBuffer.decodeCachedValue(value, 32,
                       clientCache_ -> sendEventMaskCache, 9);
          PutULONG(value, outputMessage + 8, bigEndian_);
          decodeBuffer.decodeCachedValue(*(outputMessage + 12), 8,
                       clientCache_ -> sendEventCodeCache);
          decodeBuffer.decodeCachedValue(*(outputMessage + 13), 8,
                       clientCache_ -> sendEventByteDataCache);
          decodeBuffer.decodeValue(value, 16, 4);
          clientCache_ -> sendEventLastSequence += value;
          clientCache_ -> sendEventLastSequence &= 0xffff;
          PutUINT(clientCache_ -> sendEventLastSequence, outputMessage + 14, bigEndian_);
          decodeBuffer.decodeCachedValue(value, 32,
                       clientCache_ -> sendEventIntDataCache);
          PutULONG(value, outputMessage + 16, bigEndian_);

          for (unsigned int i = 20; i < 44; i++)
          {
            decodeBuffer.decodeCachedValue(cValue, 8,
                               clientCache_ -> sendEventEventCache);
            *(outputMessage + i) = cValue;
          }

          handleSave(messageStore, outputMessage, outputLength);
        }
        break;
      case X_ChangeWindowAttributes:
        {
          unsigned int numAttrs;
          decodeBuffer.decodeValue(numAttrs, 4);
          outputLength = 12 + (numAttrs << 2);
          outputMessage = writeBuffer_.addMessage(outputLength);
          decodeBuffer.decodeXidValue(value, clientCache_ -> windowCache);
          PutULONG(value, outputMessage + 4, bigEndian_);
          unsigned int bitmask;
          decodeBuffer.decodeCachedValue(bitmask, 15,
                                     clientCache_ -> createWindowBitmaskCache);
          PutULONG(bitmask, outputMessage + 8, bigEndian_);
          unsigned char *nextDest = outputMessage + 12;
          unsigned int mask = 0x1;
          for (unsigned int i = 0; i < 15; i++)
          {
            if (bitmask & mask)
            {
              decodeBuffer.decodeCachedValue(value, 32,
                                 *clientCache_ -> createWindowAttrCache[i]);
              PutULONG(value, nextDest, bigEndian_);
              nextDest += 4;
            }
            mask <<= 1;
          }
        }
        break;
      case X_ClearArea:
        {
          MessageStore *messageStore = clientStore_ ->
                               getRequestStore(X_ClearArea);

          if (handleDecodeCached(decodeBuffer, clientCache_, messageStore,
                                     outputMessage, outputLength))
          {
            break;
          }

          outputLength = 16;
          outputMessage = writeBuffer_.addMessage(outputLength);
          decodeBuffer.decodeBoolValue(value);
          outputMessage[1] = (unsigned char) value;
          decodeBuffer.decodeXidValue(value, clientCache_ -> windowCache);
          PutULONG(value, outputMessage + 4, bigEndian_);
          unsigned char *nextDest = outputMessage + 8;
          for (unsigned int i = 0; i < 4; i++)
          {
            decodeBuffer.decodeCachedValue(value, 16,
                               *clientCache_ -> clearAreaGeomCache[i], 8);
            PutUINT(value, nextDest, bigEndian_);
            nextDest += 2;
          }

          handleSave(messageStore, outputMessage, outputLength);
        }
        break;
      case X_CloseFont:
        {
          outputLength = 8;
          outputMessage = writeBuffer_.addMessage(outputLength);
          decodeBuffer.decodeValue(value, 29, 5);
          clientCache_ -> lastFont += value;
          clientCache_ -> lastFont &= 0x1fffffff;
          PutULONG(clientCache_ -> lastFont, outputMessage + 4, bigEndian_);
        }
        break;
      case X_ConfigureWindow:
        {
          MessageStore *messageStore = clientStore_ ->
                               getRequestStore(X_ConfigureWindow);

          if (handleDecodeCached(decodeBuffer, clientCache_, messageStore,
                                     outputMessage, outputLength))
          {
            break;
          }

          outputLength = 12;
          outputMessage = writeBuffer_.addMessage(outputLength);
          writeBuffer_.registerPointer(&outputMessage);
          decodeBuffer.decodeXidValue(value, clientCache_ -> windowCache);
          PutULONG(value, outputMessage + 4, bigEndian_);
          unsigned int bitmask;
          decodeBuffer.decodeCachedValue(bitmask, 7,
                                  clientCache_ -> configureWindowBitmaskCache);
          PutUINT(bitmask, outputMessage + 8, bigEndian_);
          unsigned int mask = 0x1;
          for (unsigned int i = 0; i < 7; i++)
          {
            if (bitmask & mask)
            {
              unsigned char* nextDest = writeBuffer_.addMessage(4);
              outputLength += 4;
              decodeBuffer.decodeCachedValue(value, CONFIGUREWINDOW_FIELD_WIDTH[i],
                                 *clientCache_ -> configureWindowAttrCache[i], 8);
              PutULONG(value, nextDest, bigEndian_);
              nextDest += 4;
            }
            mask <<= 1;
          }
          writeBuffer_.unregisterPointer();

          handleSave(messageStore, outputMessage, outputLength);
        }
        break;
      case X_ConvertSelection:
        {
          outputLength = 24;
          outputMessage = writeBuffer_.addMessage(outputLength);
          decodeBuffer.decodeCachedValue(value, 29,
                             clientCache_ -> convertSelectionRequestorCache, 9);
          PutULONG(value, outputMessage + 4, bigEndian_);
          unsigned char* nextDest = outputMessage + 8;
          for (unsigned int i = 0; i < 3; i++)
          {
            decodeBuffer.decodeCachedValue(value, 29,
                               *(clientCache_ -> convertSelectionAtomCache[i]), 9);
            PutULONG(value, nextDest, bigEndian_);
            nextDest += 4;
          }
          decodeBuffer.decodeValue(value, 32, 4);
          clientCache_ -> convertSelectionLastTimestamp += value;
          PutULONG(clientCache_ -> convertSelectionLastTimestamp,
                   nextDest, bigEndian_);
        }
        break;
      case X_CopyArea:
        {
          MessageStore *messageStore = clientStore_ ->
                               getRequestStore(X_CopyArea);

          if (handleDecodeCached(decodeBuffer, clientCache_, messageStore,
                                     outputMessage, outputLength))
          {
            break;
          }

          outputLength = 28;
          outputMessage = writeBuffer_.addMessage(outputLength);
          decodeBuffer.decodeXidValue(value, clientCache_ -> drawableCache);
          PutULONG(value, outputMessage + 4, bigEndian_);
          decodeBuffer.decodeXidValue(value, clientCache_ -> drawableCache);
          PutULONG(value, outputMessage + 8, bigEndian_);
          decodeBuffer.decodeXidValue(value, clientCache_ -> gcCache);
          PutULONG(value, outputMessage + 12, bigEndian_);
          unsigned char *nextDest = outputMessage + 16;
          for (unsigned int i = 0; i < 6; i++)
          {
            decodeBuffer.decodeCachedValue(value, 16,
                               *clientCache_ -> copyAreaGeomCache[i], 8);
            PutUINT(value, nextDest, bigEndian_);
            nextDest += 2;
          }

          handleSave(messageStore, outputMessage, outputLength);
        }
        break;
      case X_CopyGC:
        {
          outputLength = 16;
          outputMessage = writeBuffer_.addMessage(outputLength);
          decodeBuffer.decodeXidValue(value, clientCache_ -> gcCache);
          PutULONG(value, outputMessage + 4, bigEndian_);
          decodeBuffer.decodeXidValue(value, clientCache_ -> gcCache);
          PutULONG(value, outputMessage + 8, bigEndian_);
          decodeBuffer.decodeCachedValue(value, 23,
                             clientCache_ -> createGCBitmaskCache);
          PutULONG(value, outputMessage + 12, bigEndian_);
        }
        break;
      case X_CopyPlane:
        {
          outputLength = 32;
          outputMessage = writeBuffer_.addMessage(outputLength);
          decodeBuffer.decodeXidValue(value, clientCache_ -> drawableCache);
          PutULONG(value, outputMessage + 4, bigEndian_);
          decodeBuffer.decodeXidValue(value, clientCache_ -> drawableCache);
          PutULONG(value, outputMessage + 8, bigEndian_);
          decodeBuffer.decodeXidValue(value, clientCache_ -> gcCache);
          PutULONG(value, outputMessage + 12, bigEndian_);
          unsigned char *nextDest = outputMessage + 16;
          for (unsigned int i = 0; i < 6; i++)
          {
            decodeBuffer.decodeCachedValue(value, 16,
                               *clientCache_ -> copyPlaneGeomCache[i], 8);
            PutUINT(value, nextDest, bigEndian_);
            nextDest += 2;
          }
          decodeBuffer.decodeCachedValue(value, 32,
                             clientCache_ -> copyPlaneBitPlaneCache, 10);
          PutULONG(value, outputMessage + 28, bigEndian_);
        }
        break;
      case X_CreateGC:
        {
          MessageStore *messageStore = clientStore_ ->
                               getRequestStore(X_CreateGC);

          if (handleDecodeCached(decodeBuffer, clientCache_, messageStore,
                                     outputMessage, outputLength))
          {
            break;
          }

          outputLength = 16;
          outputMessage = writeBuffer_.addMessage(outputLength);
          writeBuffer_.registerPointer(&outputMessage);

          if (control -> isProtoStep7() == 1)
          {
            decodeBuffer.decodeNewXidValue(value, clientCache_ -> lastId,
                               clientCache_ -> lastIdCache, clientCache_ -> gcCache,
                                   clientCache_ -> freeGCCache);
          }
          else
          {
            decodeBuffer.decodeXidValue(value, clientCache_ -> gcCache);
          }

          PutULONG(value, outputMessage + 4, bigEndian_);
          unsigned int offset = 8;
          decodeBuffer.decodeXidValue(value, clientCache_ -> drawableCache);
          PutULONG(value, outputMessage + offset, bigEndian_);
          offset += 4;
          unsigned int bitmask;
          decodeBuffer.decodeCachedValue(bitmask, 23,
                                         clientCache_ -> createGCBitmaskCache);
          PutULONG(bitmask, outputMessage + offset, bigEndian_);
          unsigned int mask = 0x1;
          for (unsigned int i = 0; i < 23; i++)
          {
            if (bitmask & mask)
            {
              unsigned char* nextDest = writeBuffer_.addMessage(4);
              outputLength += 4;
              unsigned int fieldWidth = CREATEGC_FIELD_WIDTH[i];
              if (fieldWidth <= 4)
                decodeBuffer.decodeValue(value, fieldWidth);
              else
                decodeBuffer.decodeCachedValue(value, fieldWidth,
                                   *clientCache_ -> createGCAttrCache[i]);
              PutULONG(value, nextDest, bigEndian_);
            }
            mask <<= 1;
          }
          writeBuffer_.unregisterPointer();

          handleSave(messageStore, outputMessage, outputLength);
        }
        break;
      case X_ChangeGC:
        {
          MessageStore *messageStore = clientStore_ ->
                               getRequestStore(X_ChangeGC);

          if (handleDecodeCached(decodeBuffer, clientCache_, messageStore,
                                     outputMessage, outputLength))
          {
            break;
          }

          outputLength = 12;
          outputMessage = writeBuffer_.addMessage(outputLength);
          writeBuffer_.registerPointer(&outputMessage);
          decodeBuffer.decodeXidValue(value, clientCache_ -> gcCache);
          PutULONG(value, outputMessage + 4, bigEndian_);
          unsigned int offset = 8;
          unsigned int bitmask;
          decodeBuffer.decodeCachedValue(bitmask, 23,
                                         clientCache_ -> createGCBitmaskCache);
          PutULONG(bitmask, outputMessage + offset, bigEndian_);
          unsigned int mask = 0x1;
          for (unsigned int i = 0; i < 23; i++)
          {
            if (bitmask & mask)
            {
              unsigned char* nextDest = writeBuffer_.addMessage(4);
              outputLength += 4;
              unsigned int fieldWidth = CREATEGC_FIELD_WIDTH[i];
              if (fieldWidth <= 4)
                decodeBuffer.decodeValue(value, fieldWidth);
              else
                decodeBuffer.decodeCachedValue(value, fieldWidth,
                                   *clientCache_ -> createGCAttrCache[i]);
              PutULONG(value, nextDest, bigEndian_);
            }
            mask <<= 1;
          }
          writeBuffer_.unregisterPointer();

          handleSave(messageStore, outputMessage, outputLength);
        }
        break;
      case X_CreatePixmap:
        {
          MessageStore *messageStore = clientStore_ ->
                               getRequestStore(X_CreatePixmap);

          hit = handleDecode(decodeBuffer, clientCache_, messageStore,
                                 outputOpcode, outputMessage, outputLength);
        }
        break;
      case X_CreateWindow:
        {
          outputLength = 32;
          outputMessage = writeBuffer_.addMessage(outputLength);
          writeBuffer_.registerPointer(&outputMessage);
          decodeBuffer.decodeCachedValue(cValue, 8, clientCache_ -> depthCache);
          outputMessage[1] = cValue;
          decodeBuffer.decodeXidValue(value, clientCache_ -> windowCache);
          PutULONG(value, outputMessage + 8, bigEndian_);

          if (control -> isProtoStep7() == 1)
          {
            decodeBuffer.decodeNewXidValue(value, clientCache_ -> lastId,
                               clientCache_ -> lastIdCache, clientCache_ -> windowCache,
                                   clientCache_ -> freeWindowCache);
          }
          else
          {
            decodeBuffer.decodeXidValue(value, clientCache_ -> windowCache);
          }

          PutULONG(value, outputMessage + 4, bigEndian_);
          unsigned char *nextDest = outputMessage + 12;
          unsigned int i;
          for (i = 0; i < 6; i++)
          {
            decodeBuffer.decodeCachedValue(value, 16,
                               *clientCache_ -> createWindowGeomCache[i], 8);
            PutUINT(value, nextDest, bigEndian_);
            nextDest += 2;
          }
          decodeBuffer.decodeCachedValue(value, 29, clientCache_ -> visualCache);
          PutULONG(value, outputMessage + 24, bigEndian_);
          unsigned int bitmask;
          decodeBuffer.decodeCachedValue(bitmask, 15,
                                     clientCache_ -> createWindowBitmaskCache);
          PutULONG(bitmask, outputMessage + 28, bigEndian_);
          unsigned int mask = 0x1;
          for (i = 0; i < 15; i++)
          {
            if (bitmask & mask)
            {
              nextDest = writeBuffer_.addMessage(4);
              outputLength += 4;
              decodeBuffer.decodeCachedValue(value, 32,
                                 *clientCache_ -> createWindowAttrCache[i]);
              PutULONG(value, nextDest, bigEndian_);
            }
            mask <<= 1;
          }
          writeBuffer_.unregisterPointer();
        }
        break;
      case X_DeleteProperty:
        {
          outputLength = 12;
          outputMessage = writeBuffer_.addMessage(outputLength);
          decodeBuffer.decodeXidValue(value, clientCache_ -> windowCache);
          PutULONG(value, outputMessage + 4, bigEndian_);
          decodeBuffer.decodeValue(value, 29, 9);
          PutULONG(value, outputMessage + 8, bigEndian_);
        }
        break;
      case X_FillPoly:
        {
          MessageStore *messageStore = clientStore_ ->
                               getRequestStore(X_FillPoly);

          if (handleDecodeCached(decodeBuffer, clientCache_, messageStore,
                                     outputMessage, outputLength))
          {
            break;
          }

          unsigned int numPoints;

          if (control -> isProtoStep10() == 1)
          {
            decodeBuffer.decodeCachedValue(numPoints, 16,
                               clientCache_ -> fillPolyNumPointsCache, 4);
          }
          else
          {
            decodeBuffer.decodeCachedValue(numPoints, 14,
                               clientCache_ -> fillPolyNumPointsCache, 4);
          }

          outputLength = 16 + (numPoints << 2);
          outputMessage = writeBuffer_.addMessage(outputLength);
          decodeBuffer.decodeXidValue(value, clientCache_ -> drawableCache);
          PutULONG(value, outputMessage + 4, bigEndian_);
          decodeBuffer.decodeXidValue(value, clientCache_ -> gcCache);
          PutULONG(value, outputMessage + 8, bigEndian_);
          decodeBuffer.decodeValue(value, 2);
          outputMessage[12] = (unsigned char) value;
          unsigned int relativeCoordMode;
          decodeBuffer.decodeBoolValue(relativeCoordMode);
          outputMessage[13] = (unsigned char) relativeCoordMode;
          unsigned char *nextDest = outputMessage + 16;
          unsigned int pointIndex = 0;
          for (unsigned int i = 0; i < numPoints; i++)
          {
            if (relativeCoordMode)
            {
              decodeBuffer.decodeCachedValue(value, 16,
                                 *clientCache_ -> fillPolyXRelCache[pointIndex], 8);
              PutUINT(value, nextDest, bigEndian_);
              nextDest += 2;
              decodeBuffer.decodeCachedValue(value, 16,
                                 *clientCache_ -> fillPolyYRelCache[pointIndex], 8);
              PutUINT(value, nextDest, bigEndian_);
              nextDest += 2;
            }
            else
            {
              unsigned int x, y;
              decodeBuffer.decodeBoolValue(value);
              if (value)
              {
                decodeBuffer.decodeValue(value, 3);
                x = clientCache_ -> fillPolyRecentX[value];
                y = clientCache_ -> fillPolyRecentY[value];
              }
              else
              {
                decodeBuffer.decodeCachedValue(x, 16,
                                   *clientCache_ -> fillPolyXAbsCache[pointIndex], 8);
                decodeBuffer.decodeCachedValue(y, 16,
                                   *clientCache_ -> fillPolyYAbsCache[pointIndex], 8);
                clientCache_ -> fillPolyRecentX[clientCache_ -> fillPolyIndex] = x;
                clientCache_ -> fillPolyRecentY[clientCache_ -> fillPolyIndex] = y;
                clientCache_ -> fillPolyIndex++;
                if (clientCache_ -> fillPolyIndex == 8)
                  clientCache_ -> fillPolyIndex = 0;
              }
              PutUINT(x, nextDest, bigEndian_);
              nextDest += 2;
              PutUINT(y, nextDest, bigEndian_);
              nextDest += 2;
            }

            if (++pointIndex == 10) pointIndex = 0;
          }

          handleSave(messageStore, outputMessage, outputLength);
        }
        break;
      case X_FreeColors:
        {
          unsigned int numPixels;
          decodeBuffer.decodeValue(numPixels, 16, 4);
          outputLength = 12 + (numPixels << 2);
          outputMessage = writeBuffer_.addMessage(outputLength);
          decodeBuffer.decodeCachedValue(value, 29,
                             clientCache_ -> colormapCache);
          PutULONG(value, outputMessage + 4, bigEndian_);
          decodeBuffer.decodeValue(value, 32, 4);
          PutULONG(value, outputMessage + 8, bigEndian_);
          unsigned char* nextDest = outputMessage + 12;
          while (numPixels)
          {
            decodeBuffer.decodeValue(value, 32, 8);
            PutULONG(value, nextDest, bigEndian_);
            nextDest += 4;
            numPixels--;
          }
        }
        break;
      case X_FreeCursor:
        {
          outputLength = 8;
          outputMessage = writeBuffer_.addMessage(outputLength);
          decodeBuffer.decodeCachedValue(value, 29, clientCache_ -> cursorCache, 9);
          PutULONG(value, outputMessage + 4, bigEndian_);
        }
        break;
      case X_FreeGC:
        {
          outputLength = 8;
          outputMessage = writeBuffer_.addMessage(outputLength);

          if (control -> isProtoStep7() == 1)
          {
            decodeBuffer.decodeFreeXidValue(value, clientCache_ -> freeGCCache);
          }
          else
          {
            decodeBuffer.decodeXidValue(value, clientCache_ -> gcCache);
          }

          PutULONG(value, outputMessage + 4, bigEndian_);
        }
        break;
      case X_FreePixmap:
        {
          outputLength = 8;
          outputMessage = writeBuffer_.addMessage(outputLength);

          if (control -> isProtoStep7() == 1)
          {
            decodeBuffer.decodeFreeXidValue(value, clientCache_ -> freeDrawableCache);

            PutULONG(value, outputMessage + 4, bigEndian_);
          }
          else
          {
            decodeBuffer.decodeBoolValue(value);
            if (!value)
            {
              decodeBuffer.decodeValue(value, 29, 4);
              clientCache_ -> createPixmapLastId += value;
              clientCache_ -> createPixmapLastId &= 0x1fffffff;
            }
            PutULONG(clientCache_ -> createPixmapLastId, outputMessage + 4, bigEndian_);
          }
        }
        break;
      case X_GetAtomName:
        {
          outputLength = 8;
          outputMessage = writeBuffer_.addMessage(outputLength);
          decodeBuffer.decodeValue(value, 29, 9);
          PutULONG(value, outputMessage + 4, bigEndian_);

          sequenceQueue_.push(clientSequence_, outputOpcode);
        }
        break;
      case X_GetGeometry:
        {
          outputLength = 8;
          outputMessage = writeBuffer_.addMessage(outputLength);
          decodeBuffer.decodeXidValue(value, clientCache_ -> drawableCache);
          PutULONG(value, outputMessage + 4, bigEndian_);

          sequenceQueue_.push(clientSequence_, outputOpcode);
        }
        break;
      case X_GetInputFocus:
        {
          outputLength = 4;
          outputMessage = writeBuffer_.addMessage(outputLength);

          sequenceQueue_.push(clientSequence_, outputOpcode, outputOpcode);
        }
        break;
      case X_GetModifierMapping:
        {
          outputLength = 4;
          outputMessage = writeBuffer_.addMessage(outputLength);

          sequenceQueue_.push(clientSequence_, outputOpcode);
        }
        break;
      case X_GetKeyboardMapping:
        {
          outputLength = 8;
          outputMessage = writeBuffer_.addMessage(outputLength);
          decodeBuffer.decodeValue(value, 8);
          outputMessage[4] = value;
          decodeBuffer.decodeValue(value, 8);
          outputMessage[5] = value;

          sequenceQueue_.push(clientSequence_, outputOpcode);
        }
        break;
      case X_GetProperty:
        {
          MessageStore *messageStore = clientStore_ ->
                               getRequestStore(X_GetProperty);

          if (handleDecodeCached(decodeBuffer, clientCache_, messageStore,
                                     outputMessage, outputLength))
          {
            //
            // Save a reference to identify the reply.
            //

            unsigned int property = GetULONG(outputMessage + 8, bigEndian_);

            sequenceQueue_.push(clientSequence_, outputOpcode, property);

            break;
          }

          outputLength = 24;
          outputMessage = writeBuffer_.addMessage(outputLength);
          decodeBuffer.decodeBoolValue(value);
          outputMessage[1] = (unsigned char) value;
          decodeBuffer.decodeXidValue(value, clientCache_ -> windowCache);
          PutULONG(value, outputMessage + 4, bigEndian_);
          unsigned int property;
          decodeBuffer.decodeValue(property, 29, 9);
          PutULONG(property, outputMessage + 8, bigEndian_);
          decodeBuffer.decodeValue(value, 29, 9);
          PutULONG(value, outputMessage + 12, bigEndian_);
          decodeBuffer.decodeValue(value, 32, 2);
          PutULONG(value, outputMessage + 16, bigEndian_);
          decodeBuffer.decodeValue(value, 32, 8);
          PutULONG(value, outputMessage + 20, bigEndian_);

          sequenceQueue_.push(clientSequence_, outputOpcode, property);

          handleSave(messageStore, outputMessage, outputLength);
        }
        break;
      case X_GetSelectionOwner:
        {
          outputLength = 8;
          outputMessage = writeBuffer_.addMessage(outputLength);
          decodeBuffer.decodeCachedValue(value, 29,
                           clientCache_ -> getSelectionOwnerSelectionCache, 9);
          PutULONG(value, outputMessage + 4, bigEndian_);

          sequenceQueue_.push(clientSequence_, outputOpcode);
        }
        break;
      case X_GrabButton:
      case X_GrabPointer:
        {
          outputLength = 24;
          outputMessage = writeBuffer_.addMessage(outputLength);
          decodeBuffer.decodeBoolValue(value);
          outputMessage[1] = (unsigned char) value;
          decodeBuffer.decodeXidValue(value, clientCache_ -> windowCache);
          PutULONG(value, outputMessage + 4, bigEndian_);
          decodeBuffer.decodeCachedValue(value, 16,
                             clientCache_ -> grabButtonEventMaskCache);
          PutUINT(value, outputMessage + 8, bigEndian_);
          decodeBuffer.decodeBoolValue(value);
          outputMessage[10] = (unsigned char) value;
          decodeBuffer.decodeBoolValue(value);
          outputMessage[11] = (unsigned char) value;
          decodeBuffer.decodeCachedValue(value, 29,
                             clientCache_ -> grabButtonConfineCache, 9);
          PutULONG(value, outputMessage + 12, bigEndian_);
          decodeBuffer.decodeCachedValue(value, 29,
                             clientCache_ -> cursorCache, 9);
          PutULONG(value, outputMessage + 16, bigEndian_);
          if (outputOpcode == X_GrabButton)
          {
            decodeBuffer.decodeCachedValue(cValue, 8,
                               clientCache_ -> grabButtonButtonCache);
            outputMessage[20] = cValue;
            decodeBuffer.decodeCachedValue(value, 16,
                               clientCache_ -> grabButtonModifierCache);
            PutUINT(value, outputMessage + 22, bigEndian_);
          }
          else
          {
            decodeBuffer.decodeValue(value, 32, 4);
            clientCache_ -> grabKeyboardLastTimestamp += value;
            PutULONG(clientCache_ -> grabKeyboardLastTimestamp,
                     outputMessage + 20, bigEndian_);

            sequenceQueue_.push(clientSequence_, outputOpcode);
          }
        }
        break;
      case X_GrabKeyboard:
        {
          outputLength = 16;
          outputMessage = writeBuffer_.addMessage(outputLength);
          decodeBuffer.decodeBoolValue(value);
          outputMessage[1] = (unsigned char) value;
          decodeBuffer.decodeXidValue(value, clientCache_ -> windowCache);
          PutULONG(value, outputMessage + 4, bigEndian_);
          decodeBuffer.decodeValue(value, 32, 4);
          clientCache_ -> grabKeyboardLastTimestamp += value;
          PutULONG(clientCache_ -> grabKeyboardLastTimestamp, outputMessage + 8,
                   bigEndian_);
          decodeBuffer.decodeBoolValue(value);
          outputMessage[12] = (unsigned char) value;
          decodeBuffer.decodeBoolValue(value);
          outputMessage[13] = (unsigned char) value;

          sequenceQueue_.push(clientSequence_, outputOpcode);
        }
        break;
      case X_GrabServer:
      case X_UngrabServer:
      case X_NoOperation:
        {
          #ifdef DEBUG
          *logofs << "handleWrite: Managing (probably tainted) X_NoOperation request for FD#" 
                  << fd_ << ".\n" << logofs_flush;
          #endif

          outputLength = 4;
          outputMessage = writeBuffer_.addMessage(outputLength);
        }
        break;
      case X_PolyText8:
        {
          MessageStore *messageStore = clientStore_ ->
                               getRequestStore(X_PolyText8);

          if (handleDecodeCached(decodeBuffer, clientCache_, messageStore,
                                     outputMessage, outputLength))
          {
            break;
          }

          outputLength = 16;
          outputMessage = writeBuffer_.addMessage(outputLength);
          decodeBuffer.decodeXidValue(value, clientCache_ -> drawableCache);
          PutULONG(value, outputMessage + 4, bigEndian_);
          decodeBuffer.decodeXidValue(value, clientCache_ -> gcCache);
          PutULONG(value, outputMessage + 8, bigEndian_);
          decodeBuffer.decodeCachedValue(value, 16,
                             clientCache_ -> polyTextCacheX);
          clientCache_ -> polyTextLastX += value;
          clientCache_ -> polyTextLastX &= 0xffff;
          PutUINT(clientCache_ -> polyTextLastX, outputMessage + 12, bigEndian_);
          decodeBuffer.decodeCachedValue(value, 16,
                             clientCache_ -> polyTextCacheY);
          clientCache_ -> polyTextLastY += value;
          clientCache_ -> polyTextLastY &= 0xffff;
          PutUINT(clientCache_ -> polyTextLastY, outputMessage + 14, bigEndian_);
          unsigned int addedLength = 0;
          writeBuffer_.registerPointer(&outputMessage);
          for (;;)
          {
            decodeBuffer.decodeBoolValue(value);
            if (!value)
              break;
            unsigned int textLength;
            decodeBuffer.decodeValue(textLength, 8);
            if (textLength == 255)
            {
              addedLength += 5;
              unsigned char *nextSegment = writeBuffer_.addMessage(5);
              *nextSegment = (unsigned char) textLength;
              decodeBuffer.decodeCachedValue(value, 29,
                                 clientCache_ -> polyTextFontCache);
              PutULONG(value, nextSegment + 1, 1);
            }
            else
            {
              addedLength += (textLength + 2);
              unsigned char *nextSegment =
              writeBuffer_.addMessage(textLength + 2);
              *nextSegment = (unsigned char) textLength;
              unsigned char *nextDest = nextSegment + 1;
              decodeBuffer.decodeCachedValue(cValue, 8,
                                 clientCache_ -> polyTextDeltaCache);
              *nextDest++ = cValue;

              if (control -> isProtoStep7() == 1)
              {
                decodeBuffer.decodeTextData(nextDest, textLength);

                nextDest += textLength;
              }
              else
              {
                clientCache_ -> polyTextTextCompressor.reset();
                while (textLength)
                {
                  *nextDest++ = clientCache_ -> polyTextTextCompressor.decodeChar(decodeBuffer);
                  textLength--;
                }
              }
            }
          }
          outputLength += addedLength;
          unsigned int mod4 = (addedLength & 0x3);
          if (mod4)
          {
            unsigned int extra = 4 - mod4;
            unsigned char *nextDest = writeBuffer_.addMessage(extra);
            for (unsigned int i = 0; i < extra; i++)
              *nextDest++ = 0;
            outputLength += extra;
          }
          writeBuffer_.unregisterPointer();

          handleSave(messageStore, outputMessage, outputLength);
        }
        break;
      case X_PolyText16:
        {
          MessageStore *messageStore = clientStore_ ->
                               getRequestStore(X_PolyText16);

          if (handleDecodeCached(decodeBuffer, clientCache_, messageStore,
                                     outputMessage, outputLength))
          {
            break;
          }

          outputLength = 16;
          outputMessage = writeBuffer_.addMessage(outputLength);
          decodeBuffer.decodeXidValue(value, clientCache_ -> drawableCache);
          PutULONG(value, outputMessage + 4, bigEndian_);
          decodeBuffer.decodeXidValue(value, clientCache_ -> gcCache);
          PutULONG(value, outputMessage + 8, bigEndian_);
          decodeBuffer.decodeCachedValue(value, 16,
                             clientCache_ -> polyTextCacheX);
          clientCache_ -> polyTextLastX += value;
          clientCache_ -> polyTextLastX &= 0xffff;
          PutUINT(clientCache_ -> polyTextLastX, outputMessage + 12, bigEndian_);
          decodeBuffer.decodeCachedValue(value, 16,
                             clientCache_ -> polyTextCacheY);
          clientCache_ -> polyTextLastY += value;
          clientCache_ -> polyTextLastY &= 0xffff;
          PutUINT(clientCache_ -> polyTextLastY, outputMessage + 14, bigEndian_);
          unsigned int addedLength = 0;
          writeBuffer_.registerPointer(&outputMessage);
          for (;;)
          {
            decodeBuffer.decodeBoolValue(value);
            if (!value)
              break;
            unsigned int textLength;
            decodeBuffer.decodeValue(textLength, 8);
            if (textLength == 255)
            {
              addedLength += 5;
              unsigned char *nextSegment = writeBuffer_.addMessage(5);
              *nextSegment = (unsigned char) textLength;
              decodeBuffer.decodeCachedValue(value, 29, clientCache_ -> polyTextFontCache);
              PutULONG(value, nextSegment + 1, 1);
            }
            else
            {
              addedLength += (textLength * 2 + 2);
              unsigned char *nextSegment =
              writeBuffer_.addMessage(textLength * 2 + 2);
              *nextSegment = (unsigned char) textLength;
              unsigned char *nextDest = nextSegment + 1;
              decodeBuffer.decodeCachedValue(cValue, 8, clientCache_ -> polyTextDeltaCache);
              *nextDest++ = cValue;

              if (control -> isProtoStep7() == 1)
              {
                decodeBuffer.decodeTextData(nextDest, textLength * 2);

                nextDest += textLength * 2;
              }
              else
              {
                clientCache_ -> polyTextTextCompressor.reset();
                textLength <<= 1;
                while (textLength)
                {
                  *nextDest++ =
                    clientCache_ -> polyTextTextCompressor.decodeChar(decodeBuffer);
                  textLength--;
                }
              }
            }
          }
          outputLength += addedLength;

          unsigned int mod4 = (addedLength & 0x3);
          if (mod4)
          {
            unsigned int extra = 4 - mod4;
            unsigned char *nextDest = writeBuffer_.addMessage(extra);
            for (unsigned int i = 0; i < extra; i++)
              *nextDest++ = 0;
            outputLength += extra;
          }
          writeBuffer_.unregisterPointer();

          handleSave(messageStore, outputMessage, outputLength);
        }
        break;
      case X_ImageText8:
        {
          MessageStore *messageStore = clientStore_ ->
                               getRequestStore(X_ImageText8);

          if (handleDecodeCached(decodeBuffer, clientCache_, messageStore,
                                     outputMessage, outputLength))
          {
            break;
          }

          unsigned int textLength;
          decodeBuffer.decodeCachedValue(textLength, 8,
                       clientCache_ -> imageTextLengthCache, 4);
          outputLength = 16 + RoundUp4(textLength);
          outputMessage = writeBuffer_.addMessage(outputLength);
          outputMessage[1] = (unsigned char) textLength;
          decodeBuffer.decodeXidValue(value, clientCache_ -> drawableCache);
          PutULONG(value, outputMessage + 4, bigEndian_);
          decodeBuffer.decodeXidValue(value, clientCache_ -> gcCache);
          PutULONG(value, outputMessage + 8, bigEndian_);
          decodeBuffer.decodeCachedValue(value, 16,
                       clientCache_ -> imageTextCacheX);
          clientCache_ -> imageTextLastX += value;
          clientCache_ -> imageTextLastX &= 0xffff;
          PutUINT(clientCache_ -> imageTextLastX, outputMessage + 12, bigEndian_);
          decodeBuffer.decodeCachedValue(value, 16,
                       clientCache_ -> imageTextCacheY);
          clientCache_ -> imageTextLastY += value;
          clientCache_ -> imageTextLastY &= 0xffff;
          PutUINT(clientCache_ -> imageTextLastY, outputMessage + 14, bigEndian_);
          unsigned char *nextDest = outputMessage + 16;

          if (control -> isProtoStep7() == 1)
          {
            decodeBuffer.decodeTextData(nextDest, textLength);
          }
          else
          {
            clientCache_ -> imageTextTextCompressor.reset();
            for (unsigned int j = 0; j < textLength; j++)
              *nextDest++ = clientCache_ -> imageTextTextCompressor.decodeChar(decodeBuffer);
          }

          handleSave(messageStore, outputMessage, outputLength);
        }
        break;
      case X_ImageText16:
        {
          MessageStore *messageStore = clientStore_ ->
                               getRequestStore(X_ImageText16);

          if (handleDecodeCached(decodeBuffer, clientCache_, messageStore,
                                     outputMessage, outputLength))
          {
            break;
          }

          unsigned int textLength;
          decodeBuffer.decodeCachedValue(textLength, 8,
                       clientCache_ -> imageTextLengthCache, 4);
          outputLength = 16 + RoundUp4(textLength * 2);
          outputMessage = writeBuffer_.addMessage(outputLength);
          outputMessage[1] = (unsigned char) textLength;
          decodeBuffer.decodeXidValue(value, clientCache_ -> drawableCache);
          PutULONG(value, outputMessage + 4, bigEndian_);
          decodeBuffer.decodeXidValue(value, clientCache_ -> gcCache);
          PutULONG(value, outputMessage + 8, bigEndian_);
          decodeBuffer.decodeCachedValue(value, 16,
                       clientCache_ -> imageTextCacheX);
          clientCache_ -> imageTextLastX += value;
          clientCache_ -> imageTextLastX &= 0xffff;
          PutUINT(clientCache_ -> imageTextLastX, outputMessage + 12, bigEndian_);
          decodeBuffer.decodeCachedValue(value, 16,
                       clientCache_ -> imageTextCacheY);
          clientCache_ -> imageTextLastY += value;
          clientCache_ -> imageTextLastY &= 0xffff;
          PutUINT(clientCache_ -> imageTextLastY, outputMessage + 14, bigEndian_);
          unsigned char *nextDest = outputMessage + 16;

          if (control -> isProtoStep7() == 1)
          {
            decodeBuffer.decodeTextData(nextDest, textLength * 2);
          }
          else
          {
            clientCache_ -> imageTextTextCompressor.reset();
            for (unsigned int j = 0; j < textLength * 2; j++)
              *nextDest++ = clientCache_ -> imageTextTextCompressor.decodeChar(decodeBuffer);
          }

          handleSave(messageStore, outputMessage, outputLength);
        }
        break;
      case X_InternAtom:
        {
          MessageStore *messageStore = clientStore_ ->
                               getRequestStore(X_InternAtom);

          if (handleDecodeCached(decodeBuffer, clientCache_, messageStore,
                                     outputMessage, outputLength))
          {
            sequenceQueue_.push(clientSequence_, outputOpcode);

            break;
          }

          unsigned int nameLength;
          decodeBuffer.decodeValue(nameLength, 16, 6);
          outputLength = RoundUp4(nameLength) + 8;
          outputMessage = writeBuffer_.addMessage(outputLength);
          PutUINT(nameLength, outputMessage + 4, bigEndian_);
          decodeBuffer.decodeBoolValue(value);
          outputMessage[1] = (unsigned char) value;
          unsigned char *nextDest = outputMessage + 8;

          if (control -> isProtoStep7() == 1)
          {
            decodeBuffer.decodeTextData(nextDest, nameLength);
          }
          else
          {
            clientCache_ -> internAtomTextCompressor.reset();
            for (unsigned int i = 0; i < nameLength; i++)
            {
              *nextDest++ = clientCache_ -> internAtomTextCompressor.decodeChar(decodeBuffer);
            }
          }

          sequenceQueue_.push(clientSequence_, outputOpcode);

          handleSave(messageStore, outputMessage, outputLength);
        }
        break;
      case X_ListExtensions:
        {
          outputLength = 4;
          outputMessage = writeBuffer_.addMessage(outputLength);

          sequenceQueue_.push(clientSequence_, outputOpcode);
        }
        break;
      case X_ListFonts:
        {
          unsigned int textLength;
          decodeBuffer.decodeValue(textLength, 16, 6);
          outputLength = 8 + RoundUp4(textLength);
          outputMessage = writeBuffer_.addMessage(outputLength);
          PutUINT(textLength, outputMessage + 6, bigEndian_);
          decodeBuffer.decodeValue(value, 16, 6);
          PutUINT(value, outputMessage + 4, bigEndian_);
          unsigned char* nextDest = outputMessage + 8;

          if (control -> isProtoStep7() == 1)
          {
            decodeBuffer.decodeTextData(nextDest, textLength);
          }
          else
          {
            clientCache_ -> polyTextTextCompressor.reset();
            for (unsigned int i = 0; i < textLength; i++)
            {
              *nextDest++ = clientCache_ -> polyTextTextCompressor.decodeChar(decodeBuffer);
            }
          }

          sequenceQueue_.push(clientSequence_, outputOpcode);
        }
        break;
      case X_LookupColor:
      case X_AllocNamedColor:
        {
          unsigned int textLength;
          decodeBuffer.decodeValue(textLength, 16, 6);
          outputLength = 12 + RoundUp4(textLength);
          outputMessage = writeBuffer_.addMessage(outputLength);
          decodeBuffer.decodeCachedValue(value, 29,
                             clientCache_ -> colormapCache);
          PutULONG(value, outputMessage + 4, bigEndian_);
          PutUINT(textLength, outputMessage + 8, bigEndian_);
          unsigned char *nextDest = outputMessage + 12;

          if (control -> isProtoStep7() == 1)
          {
            decodeBuffer.decodeTextData(nextDest, textLength);
          }
          else
          {
            clientCache_ -> polyTextTextCompressor.reset();
            for (unsigned int i = 0; i < textLength; i++)
            {
              *nextDest++ = clientCache_ -> polyTextTextCompressor.decodeChar(decodeBuffer);
            }
          }

          sequenceQueue_.push(clientSequence_, outputOpcode);
        }
        break;
      case X_MapWindow:
      case X_UnmapWindow:
      case X_MapSubwindows:
      case X_GetWindowAttributes:
      case X_DestroyWindow:
      case X_DestroySubwindows:
      case X_QueryPointer:
      case X_QueryTree:
        {
          outputLength = 8;
          outputMessage = writeBuffer_.addMessage(outputLength);

          if (outputOpcode == X_DestroyWindow && control -> isProtoStep7() == 1)
          {
            decodeBuffer.decodeFreeXidValue(value, clientCache_ -> freeWindowCache);
          }
          else
          {
            decodeBuffer.decodeXidValue(value, clientCache_ -> windowCache);
          }

          PutULONG(value, outputMessage + 4, bigEndian_);
          if (outputOpcode == X_QueryPointer ||
                  outputOpcode == X_GetWindowAttributes ||
                      outputOpcode == X_QueryTree)
          {
            sequenceQueue_.push(clientSequence_, outputOpcode);
          }
        }
        break;
      case X_OpenFont:
        {
          unsigned int nameLength;
          decodeBuffer.decodeValue(nameLength, 16, 7);
          outputLength = RoundUp4(12 + nameLength);
          outputMessage = writeBuffer_.addMessage(outputLength);
          PutUINT(nameLength, outputMessage + 8, bigEndian_);
          decodeBuffer.decodeValue(value, 29, 5);
          clientCache_ -> lastFont += value;
          clientCache_ -> lastFont &= 0x1fffffff;
          PutULONG(clientCache_ -> lastFont, outputMessage + 4, bigEndian_);
          unsigned char *nextDest = outputMessage + 12;

          if (control -> isProtoStep7() == 1)
          {
            decodeBuffer.decodeTextData(nextDest, nameLength);
          }
          else
          {
            clientCache_ -> openFontTextCompressor.reset();
            for (; nameLength; nameLength--)
            {
              *nextDest++ = clientCache_ -> openFontTextCompressor.
                                  decodeChar(decodeBuffer);
            }
          }
        }
        break;
      case X_PolyFillRectangle:
        {
          MessageStore *messageStore = clientStore_ ->
                               getRequestStore(X_PolyFillRectangle);

          if (handleDecodeCached(decodeBuffer, clientCache_, messageStore,
                                     outputMessage, outputLength))
          {
            break;
          }

          outputLength = 12;
          outputMessage = writeBuffer_.addMessage(outputLength);
          writeBuffer_.registerPointer(&outputMessage);
          decodeBuffer.decodeXidValue(value, clientCache_ -> drawableCache);
          PutULONG(value, outputMessage + 4, bigEndian_);
          decodeBuffer.decodeXidValue(value, clientCache_ -> gcCache);
          PutULONG(value, outputMessage + 8, bigEndian_);

          unsigned int index = 0;
          unsigned int lastX = 0, lastY = 0, lastWidth = 0, lastHeight = 0;
          unsigned int numRectangles = 0;

          for (;;)
          {
            outputLength += 8;
            writeBuffer_.addMessage(8);
            unsigned char *nextDest = outputMessage + 12 +
            (numRectangles << 3);
            numRectangles++;
            decodeBuffer.decodeCachedValue(value, 16,
                         *clientCache_ -> polyFillRectangleCacheX[index], 8);
            value += lastX;
            PutUINT(value, nextDest, bigEndian_);
            lastX = value;
            nextDest += 2;
            decodeBuffer.decodeCachedValue(value, 16,
                         *clientCache_ -> polyFillRectangleCacheY[index], 8);
            value += lastY;
            PutUINT(value, nextDest, bigEndian_);
            lastY = value;
            nextDest += 2;
            decodeBuffer.decodeCachedValue(value, 16,
                         *clientCache_ -> polyFillRectangleCacheWidth[index], 8);
            value += lastWidth;
            PutUINT(value, nextDest, bigEndian_);
            lastWidth = value;
            nextDest += 2;
            decodeBuffer.decodeCachedValue(value, 16,
                         *clientCache_ -> polyFillRectangleCacheHeight[index], 8);
            value += lastHeight;
            PutUINT(value, nextDest, bigEndian_);
            lastHeight = value;
            nextDest += 2;

            if (++index == 4) index = 0;

            decodeBuffer.decodeBoolValue(value);

            if (!value) break;
          }
          writeBuffer_.unregisterPointer();

          handleSave(messageStore, outputMessage, outputLength);
        }
        break;
      case X_PolyFillArc:
        {
          MessageStore *messageStore = clientStore_ ->
                               getRequestStore(X_PolyFillArc);

          if (handleDecodeCached(decodeBuffer, clientCache_, messageStore,
                                     outputMessage, outputLength))
          {
            break;
          }

          outputLength = 12;
          outputMessage = writeBuffer_.addMessage(outputLength);
          writeBuffer_.registerPointer(&outputMessage);
          decodeBuffer.decodeXidValue(value, clientCache_ -> drawableCache);
          PutULONG(value, outputMessage + 4, bigEndian_);
          decodeBuffer.decodeXidValue(value, clientCache_ -> gcCache);
          PutULONG(value, outputMessage + 8, bigEndian_);

          unsigned int index = 0;
          unsigned int lastX = 0, lastY = 0,
                       lastWidth = 0, lastHeight = 0,
                       lastAngle1 = 0, lastAngle2 = 0;

          unsigned int numArcs = 0;

          for (;;)
          {
            outputLength += 12;
            writeBuffer_.addMessage(12);

            unsigned char *nextDest = outputMessage + 12 +
            (numArcs * 12);
            numArcs++;

            decodeBuffer.decodeCachedValue(value, 16,
                         *clientCache_ -> polyFillArcCacheX[index], 8);
            value += lastX;
            PutUINT(value, nextDest, bigEndian_);
            lastX = value;
            nextDest += 2;

            decodeBuffer.decodeCachedValue(value, 16,
                         *clientCache_ -> polyFillArcCacheY[index], 8);
            value += lastY;
            PutUINT(value, nextDest, bigEndian_);
            lastY = value;
            nextDest += 2;

            decodeBuffer.decodeCachedValue(value, 16,
                         *clientCache_ -> polyFillArcCacheWidth[index], 8);
            value += lastWidth;
            PutUINT(value, nextDest, bigEndian_);
            lastWidth = value;
            nextDest += 2;

            decodeBuffer.decodeCachedValue(value, 16,
                         *clientCache_ -> polyFillArcCacheHeight[index], 8);
            value += lastHeight;
            PutUINT(value, nextDest, bigEndian_);
            lastHeight = value;
            nextDest += 2;

            decodeBuffer.decodeCachedValue(value, 16,
                         *clientCache_ -> polyFillArcCacheAngle1[index], 8);
            value += lastAngle1;
            PutUINT(value, nextDest, bigEndian_);
            lastAngle1 = value;
            nextDest += 2;

            decodeBuffer.decodeCachedValue(value, 16,
                         *clientCache_ -> polyFillArcCacheAngle2[index], 8);
            value += lastAngle2;
            PutUINT(value, nextDest, bigEndian_);
            lastAngle2 = value;
            nextDest += 2;

            if (++index == 2) index = 0;

            decodeBuffer.decodeBoolValue(value);

            if (!value) break;
          }
          writeBuffer_.unregisterPointer();

          handleSave(messageStore, outputMessage, outputLength);
        }
        break;
      case X_PolyArc:
        {
          MessageStore *messageStore = clientStore_ ->
                               getRequestStore(X_PolyArc);

          if (handleDecodeCached(decodeBuffer, clientCache_, messageStore,
                                     outputMessage, outputLength))
          {
            break;
          }

          outputLength = 12;
          outputMessage = writeBuffer_.addMessage(outputLength);
          writeBuffer_.registerPointer(&outputMessage);
          decodeBuffer.decodeXidValue(value, clientCache_ -> drawableCache);
          PutULONG(value, outputMessage + 4, bigEndian_);
          decodeBuffer.decodeXidValue(value, clientCache_ -> gcCache);
          PutULONG(value, outputMessage + 8, bigEndian_);

          unsigned int index = 0;
          unsigned int lastX = 0, lastY = 0,
                       lastWidth = 0, lastHeight = 0,
                       lastAngle1 = 0, lastAngle2 = 0;

          unsigned int numArcs = 0;

          for (;;)
          {
            outputLength += 12;
            writeBuffer_.addMessage(12);

            unsigned char *nextDest = outputMessage + 12 +
            (numArcs * 12);
            numArcs++;

            decodeBuffer.decodeCachedValue(value, 16,
                           *clientCache_ -> polyArcCacheX[index], 8);
            value += lastX;
            PutUINT(value, nextDest, bigEndian_);
            lastX = value;
            nextDest += 2;

            decodeBuffer.decodeCachedValue(value, 16,
                         *clientCache_ -> polyArcCacheY[index], 8);
            value += lastY;
            PutUINT(value, nextDest, bigEndian_);
            lastY = value;
            nextDest += 2;

            decodeBuffer.decodeCachedValue(value, 16,
                         *clientCache_ -> polyArcCacheWidth[index], 8);
            value += lastWidth;
            PutUINT(value, nextDest, bigEndian_);
            lastWidth = value;
            nextDest += 2;

            decodeBuffer.decodeCachedValue(value, 16,
                         *clientCache_ -> polyArcCacheHeight[index], 8);
            value += lastHeight;
            PutUINT(value, nextDest, bigEndian_);
            lastHeight = value;
            nextDest += 2;

            decodeBuffer.decodeCachedValue(value, 16,
                         *clientCache_ -> polyArcCacheAngle1[index], 8);
            value += lastAngle1;
            PutUINT(value, nextDest, bigEndian_);
            lastAngle1 = value;
            nextDest += 2;

            decodeBuffer.decodeCachedValue(value, 16,
                         *clientCache_ -> polyArcCacheAngle2[index], 8);
            value += lastAngle2;
            PutUINT(value, nextDest, bigEndian_);
            lastAngle2 = value;
            nextDest += 2;

            if (++index == 2) index = 0;

            decodeBuffer.decodeBoolValue(value);

            if (!value) break;
          }
          writeBuffer_.unregisterPointer();

          handleSave(messageStore, outputMessage, outputLength);
        }
        break;
      case X_PolyPoint:
        {
          MessageStore *messageStore = clientStore_ ->
                               getRequestStore(X_PolyPoint);

          if (handleDecodeCached(decodeBuffer, clientCache_, messageStore,
                                     outputMessage, outputLength))
          {
            break;
          }

          unsigned int numPoints;
          decodeBuffer.decodeValue(numPoints, 16, 4);
          outputLength = (numPoints << 2) + 12;
          outputMessage = writeBuffer_.addMessage(outputLength);
          unsigned int relativeCoordMode;
          decodeBuffer.decodeBoolValue(relativeCoordMode);
          outputMessage[1] = (unsigned char) relativeCoordMode;
          decodeBuffer.decodeXidValue(value, clientCache_ -> drawableCache);
          PutULONG(value, outputMessage + 4, bigEndian_);
          decodeBuffer.decodeXidValue(value, clientCache_ -> gcCache);
          PutULONG(value, outputMessage + 8, bigEndian_);
          unsigned char *nextDest = outputMessage + 12;

          unsigned int index = 0;
          unsigned int lastX = 0, lastY = 0;

          for (unsigned int i = 0; i < numPoints; i++)
          {
            decodeBuffer.decodeCachedValue(value, 16,
                               *clientCache_ -> polyPointCacheX[index], 8);
            lastX += value;
            PutUINT(lastX, nextDest, bigEndian_);
            nextDest += 2;
            decodeBuffer.decodeCachedValue(value, 16,
                               *clientCache_ -> polyPointCacheY[index], 8);
            lastY += value;
            PutUINT(lastY, nextDest, bigEndian_);
            nextDest += 2;

            if (++index == 2) index = 0;
          }

          handleSave(messageStore, outputMessage, outputLength);
        }
        break;
      case X_PolyLine:
        {
          MessageStore *messageStore = clientStore_ ->
                               getRequestStore(X_PolyLine);

          if (handleDecodeCached(decodeBuffer, clientCache_, messageStore,
                                     outputMessage, outputLength))
          {
            break;
          }

          unsigned int numPoints;
          decodeBuffer.decodeValue(numPoints, 16, 4);
          outputLength = (numPoints << 2) + 12;
          outputMessage = writeBuffer_.addMessage(outputLength);
          unsigned int relativeCoordMode;
          decodeBuffer.decodeBoolValue(relativeCoordMode);
          outputMessage[1] = (unsigned char) relativeCoordMode;
          decodeBuffer.decodeXidValue(value, clientCache_ -> drawableCache);
          PutULONG(value, outputMessage + 4, bigEndian_);
          decodeBuffer.decodeXidValue(value, clientCache_ -> gcCache);
          PutULONG(value, outputMessage + 8, bigEndian_);
          unsigned char *nextDest = outputMessage + 12;

          unsigned int index = 0;
          unsigned int lastX = 0, lastY = 0;

          for (unsigned int i = 0; i < numPoints; i++)
          {
            decodeBuffer.decodeCachedValue(value, 16,
                               *clientCache_ -> polyLineCacheX[index], 8);
            lastX += value;
            PutUINT(lastX, nextDest, bigEndian_);
            nextDest += 2;
            decodeBuffer.decodeCachedValue(value, 16,
                               *clientCache_ -> polyLineCacheY[index], 8);
            lastY += value;
            PutUINT(lastY, nextDest, bigEndian_);
            nextDest += 2;

            if (++index == 2) index = 0;
          }

          handleSave(messageStore, outputMessage, outputLength);
        }
        break;
      case X_PolyRectangle:
        {
          unsigned int numRectangles;
          decodeBuffer.decodeValue(numRectangles, 16, 3);
          outputLength = (numRectangles << 3) + 12;
          outputMessage = writeBuffer_.addMessage(outputLength);
          decodeBuffer.decodeXidValue(value, clientCache_ -> drawableCache);
          PutULONG(value, outputMessage + 4, bigEndian_);
          decodeBuffer.decodeXidValue(value, clientCache_ -> gcCache);
          PutULONG(value, outputMessage + 8, bigEndian_);
          unsigned char *nextDest = outputMessage + 12;
          for (unsigned int i = 0; i < numRectangles; i++)
            for (unsigned int k = 0; k < 4; k++)
            {
              decodeBuffer.decodeCachedValue(value, 16,
                                *clientCache_ -> polyRectangleGeomCache[k], 8);
              PutUINT(value, nextDest, bigEndian_);
              nextDest += 2;
            }
        }
        break;
      case X_PolySegment:
        {
          MessageStore *messageStore = clientStore_ ->
                               getRequestStore(X_PolySegment);

          if (handleDecodeCached(decodeBuffer, clientCache_, messageStore,
                                     outputMessage, outputLength))
          {
            break;
          }

          unsigned int numSegments;
          decodeBuffer.decodeValue(numSegments, 16, 4);
          outputLength = (numSegments << 3) + 12;
          outputMessage = writeBuffer_.addMessage(outputLength);
          decodeBuffer.decodeXidValue(value, clientCache_ -> drawableCache);
          PutULONG(value, outputMessage + 4, bigEndian_);
          decodeBuffer.decodeXidValue(value, clientCache_ -> gcCache);
          PutULONG(value, outputMessage + 8, bigEndian_);
          unsigned char *nextDest = outputMessage + 12;

          for (numSegments *= 2; numSegments; numSegments--)
          {
            unsigned int index;
            decodeBuffer.decodeBoolValue(index);
            unsigned int x;
            decodeBuffer.decodeCachedValue(x, 16,
                               clientCache_ -> polySegmentCacheX, 6);
            x += clientCache_ -> polySegmentLastX[index];
            PutUINT(x, nextDest, bigEndian_);
            nextDest += 2;

            unsigned int y;
            decodeBuffer.decodeCachedValue(y, 16,
                               clientCache_ -> polySegmentCacheY, 6);
            y += clientCache_ -> polySegmentLastY[index];
            PutUINT(y, nextDest, bigEndian_);
            nextDest += 2;

            clientCache_ -> polySegmentLastX[clientCache_ -> polySegmentCacheIndex] = x;
            clientCache_ -> polySegmentLastY[clientCache_ -> polySegmentCacheIndex] = y;

            if (clientCache_ -> polySegmentCacheIndex == 1)
              clientCache_ -> polySegmentCacheIndex = 0;
            else
              clientCache_ -> polySegmentCacheIndex = 1;
          }

          handleSave(messageStore, outputMessage, outputLength);
        }
        break;
      case X_PutImage:
        {
          MessageStore *messageStore = clientStore_ ->
                               getRequestStore(X_PutImage);

          hit = handleDecode(decodeBuffer, clientCache_, messageStore,
                                 outputOpcode, outputMessage, outputLength);

          if (outputOpcode == X_PutImage)
          {
            handleImage(outputOpcode, outputMessage, outputLength);
          }
        }
        break;
      case X_QueryBestSize:
        {
          outputLength = 12;
          outputMessage = writeBuffer_.addMessage(outputLength);
          decodeBuffer.decodeValue(value, 2);
          outputMessage[1] = (unsigned char)value;
          decodeBuffer.decodeXidValue(value, clientCache_ -> drawableCache);
          PutULONG(value, outputMessage + 4, bigEndian_);
          decodeBuffer.decodeValue(value, 16, 8);
          PutUINT(value, outputMessage + 8, bigEndian_);
          decodeBuffer.decodeValue(value, 16, 8);
          PutUINT(value, outputMessage + 10, bigEndian_);

          sequenceQueue_.push(clientSequence_, outputOpcode);
        }
        break;
      case X_QueryColors:
        {
          // Differential or plain data compression?
          decodeBuffer.decodeBoolValue(value);

          if (value)
          {
            unsigned int numColors;
            decodeBuffer.decodeValue(numColors, 16, 5);
            outputLength = (numColors << 2) + 8;
            outputMessage = writeBuffer_.addMessage(outputLength);
            decodeBuffer.decodeCachedValue(value, 29,
                               clientCache_ -> colormapCache);
            PutULONG(value, outputMessage + 4, bigEndian_);
            unsigned char *nextDest = outputMessage + 8;
            unsigned int predictedPixel = clientCache_ -> queryColorsLastPixel;
            for (unsigned int i = 0; i < numColors; i++)
            {
              unsigned int pixel;
              decodeBuffer.decodeBoolValue(value);
              if (value)
                pixel = predictedPixel;
              else
                decodeBuffer.decodeValue(pixel, 32, 9);
              PutULONG(pixel, nextDest, bigEndian_);
              if (i == 0)
                clientCache_ -> queryColorsLastPixel = pixel;
              predictedPixel = pixel + 1;
              nextDest += 4;
            }
          }
          else
          {
            // Request length.
            unsigned int requestLength;
            decodeBuffer.decodeValue(requestLength, 16, 10);
            outputLength = (requestLength << 2);
            outputMessage = writeBuffer_.addMessage(outputLength);

            const unsigned char *compressedData = NULL;
            unsigned int compressedDataSize = 0;

            int decompressed = handleDecompress(decodeBuffer, outputOpcode, 4,
                                                    outputMessage, outputLength, compressedData,
                                                        compressedDataSize);
            if (decompressed < 0)
            {
              return -1;
            }
          }

          sequenceQueue_.push(clientSequence_, outputOpcode);
        }
        break;
      case X_QueryExtension:
        {
          unsigned int nameLength;
          decodeBuffer.decodeValue(nameLength, 16, 6);
          outputLength = 8 + RoundUp4(nameLength);
          outputMessage = writeBuffer_.addMessage(outputLength);
          PutUINT(nameLength, outputMessage + 4, bigEndian_);
          unsigned char *nextDest = outputMessage + 8;
          for (unsigned int i = 0; i < nameLength; i++)
          {
            decodeBuffer.decodeValue(value, 8);
            *nextDest++ = (unsigned char) value;
          }

          unsigned int hide = 0;

          #ifdef HIDE_MIT_SHM_EXTENSION

          if (!strncmp((char *) outputMessage + 8, "MIT-SHM", 7))
          {
            #ifdef TEST
            *logofs << "handleWrite: Going to hide MIT-SHM extension in reply.\n"
                    << logofs_flush;
            #endif

            hide = 1;
          }

          #endif

          #ifdef HIDE_BIG_REQUESTS_EXTENSION

          if (!strncmp((char *) outputMessage + 8, "BIG-REQUESTS", 12))
          {
            #ifdef TEST
            *logofs << "handleWrite: Going to hide BIG-REQUESTS extension in reply.\n"
                    << logofs_flush;
            #endif

            hide = 1;
          }

          #endif

          #ifdef HIDE_XKEYBOARD_EXTENSION

          else if (!strncmp((char *) outputMessage + 8, "XKEYBOARD", 9))
          {
            #ifdef TEST
            *logofs << "handleWrite: Going to hide XKEYBOARD extension in reply.\n"
                    << logofs_flush;
            #endif

            hide = 1;
          }

          #endif

          #ifdef HIDE_XFree86_Bigfont_EXTENSION

          else if (!strncmp((char *) outputMessage + 8, "XFree86-Bigfont", 15))
          {
            #ifdef TEST
            *logofs << "handleWrite: Going to hide XFree86-Bigfont extension in reply.\n"
                    << logofs_flush;
            #endif

            hide = 1;
          }

          #endif

          //
          // This is if you want to experiment disabling SHAPE extensions.
          //

          #ifdef HIDE_SHAPE_EXTENSION

          if (!strncmp((char *) outputMessage + 8, "SHAPE", 5))
          {
            #ifdef DEBUG
            *logofs << "handleWrite: Going to hide SHAPE extension in reply.\n"
                    << logofs_flush;
            #endif

            hide = 1;
          }

          #endif

          //
          // Check if user disabled RENDER extension.
          //

          if (control -> HideRender == 1 &&
                  strncmp((char *) outputMessage + 8, "RENDER", 6) == 0)
          {
            #ifdef TEST
            *logofs << "handleWrite: Going to hide RENDER extension in reply.\n"
                    << logofs_flush;
            #endif

            hide = 1;
          }

          unsigned int extension = 0;

          if (strncmp((char *) outputMessage + 8, "SHAPE", 5) == 0)
          {
            extension = X_NXInternalShapeExtension;
          }
          else if (strncmp((char *) outputMessage + 8, "RENDER", 6) == 0)
          {
            extension = X_NXInternalRenderExtension;
          }

          sequenceQueue_.push(clientSequence_, outputOpcode,
                                  outputOpcode, hide, extension);
        }
        break;
      case X_QueryFont:
        {
          outputLength = 8;
          outputMessage = writeBuffer_.addMessage(outputLength);
          decodeBuffer.decodeValue(value, 29, 5);
          clientCache_ -> lastFont += value;
          clientCache_ -> lastFont &= 0x1fffffff;
          PutULONG(clientCache_ -> lastFont, outputMessage + 4, bigEndian_);

          sequenceQueue_.push(clientSequence_, outputOpcode);
        }
        break;
      case X_SetClipRectangles:
        {
          MessageStore *messageStore = clientStore_ ->
                               getRequestStore(X_SetClipRectangles);

          if (handleDecodeCached(decodeBuffer, clientCache_, messageStore,
                                       outputMessage, outputLength))
          {
            break;
          }

          unsigned int numRectangles;

          if (control -> isProtoStep9() == 1)
          {
            decodeBuffer.decodeValue(numRectangles, 15, 4);
          }
          else
          {
            decodeBuffer.decodeValue(numRectangles, 13, 4);
          }

          outputLength = (numRectangles << 3) + 12;
          outputMessage = writeBuffer_.addMessage(outputLength);
          decodeBuffer.decodeValue(value, 2);
          outputMessage[1] = (unsigned char) value;
          decodeBuffer.decodeXidValue(value, clientCache_ -> gcCache);
          PutULONG(value, outputMessage + 4, bigEndian_);
          decodeBuffer.decodeCachedValue(value, 16,
                                   clientCache_ -> setClipRectanglesXCache, 8);
          PutUINT(value, outputMessage + 8, bigEndian_);
          decodeBuffer.decodeCachedValue(value, 16,
                                   clientCache_ -> setClipRectanglesYCache, 8);
          PutUINT(value, outputMessage + 10, bigEndian_);
          unsigned char *nextDest = outputMessage + 12;
          for (unsigned int i = 0; i < numRectangles; i++)
          {
            for (unsigned int k = 0; k < 4; k++)
            {
              decodeBuffer.decodeCachedValue(value, 16,
                                 *clientCache_ -> setClipRectanglesGeomCache[k], 8);
              PutUINT(value, nextDest, bigEndian_);
              nextDest += 2;
            }
          }

          handleSave(messageStore, outputMessage, outputLength);
        }
        break;
      case X_SetDashes:
        {
          unsigned int numDashes;
          decodeBuffer.decodeCachedValue(numDashes, 16,
                                      clientCache_ -> setDashesLengthCache, 5);
          outputLength = 12 + RoundUp4(numDashes);
          outputMessage = writeBuffer_.addMessage(outputLength);
          PutUINT(numDashes, outputMessage + 10, bigEndian_);
          decodeBuffer.decodeXidValue(value, clientCache_ -> gcCache);
          PutULONG(value, outputMessage + 4, bigEndian_);
          decodeBuffer.decodeCachedValue(value, 16,
                                      clientCache_ -> setDashesOffsetCache, 5);
          PutUINT(value, outputMessage + 8, bigEndian_);
          unsigned char *nextDest = outputMessage + 12;
          for (unsigned int i = 0; i < numDashes; i++)
          {
            decodeBuffer.decodeCachedValue(cValue, 8,
                                clientCache_ -> setDashesDashCache_[i & 1], 5);
            *nextDest++ = cValue;
          }
        }
        break;
      case X_SetSelectionOwner:
        {
          outputLength = 16;
          outputMessage = writeBuffer_.addMessage(outputLength);
          decodeBuffer.decodeCachedValue(value, 29,
                                    clientCache_ -> setSelectionOwnerCache, 9);
          PutULONG(value, outputMessage + 4, bigEndian_);
          decodeBuffer.decodeCachedValue(value, 29,
                           clientCache_ -> getSelectionOwnerSelectionCache, 9);
          PutULONG(value, outputMessage + 8, bigEndian_);
          decodeBuffer.decodeCachedValue(value, 32,
                           clientCache_ -> setSelectionOwnerTimestampCache, 9);
          PutULONG(value, outputMessage + 12, bigEndian_);
        }
        break;
      case X_TranslateCoords:
        {
          MessageStore *messageStore = clientStore_ ->
                               getRequestStore(X_TranslateCoords);

          if (handleDecodeCached(decodeBuffer, clientCache_, messageStore,
                                     outputMessage, outputLength))
          {
            sequenceQueue_.push(clientSequence_, outputOpcode);

            break;
          }

          outputLength = 16;
          outputMessage = writeBuffer_.addMessage(outputLength);
          decodeBuffer.decodeCachedValue(value, 29,
                                   clientCache_ -> translateCoordsSrcCache, 9);
          PutULONG(value, outputMessage + 4, bigEndian_);
          decodeBuffer.decodeCachedValue(value, 29,
                                  clientCache_ -> translateCoordsDstCache, 9);
          PutULONG(value, outputMessage + 8, bigEndian_);
          decodeBuffer.decodeCachedValue(value, 16,
                                     clientCache_ -> translateCoordsXCache, 8);
          PutUINT(value, outputMessage + 12, bigEndian_);
          decodeBuffer.decodeCachedValue(value, 16,
                                     clientCache_ -> translateCoordsYCache, 8);
          PutUINT(value, outputMessage + 14, bigEndian_);

          sequenceQueue_.push(clientSequence_, outputOpcode);

          handleSave(messageStore, outputMessage, outputLength);
        }
        break;
      case X_GetImage:
        {
          MessageStore *messageStore = clientStore_ ->
                               getRequestStore(X_GetImage);

          if (handleDecodeCached(decodeBuffer, clientCache_, messageStore,
                                     outputMessage, outputLength))
          {
            sequenceQueue_.push(clientSequence_, outputOpcode);

            break;
          }

          outputLength = 20;
          outputMessage = writeBuffer_.addMessage(outputLength);
          // Format.
          unsigned int format;
          decodeBuffer.decodeValue(format, 2);
          outputMessage[1] = (unsigned char) format;
          // Drawable.
          decodeBuffer.decodeXidValue(value, clientCache_ -> drawableCache);
          PutULONG(value, outputMessage + 4, bigEndian_);
          // X.
          decodeBuffer.decodeCachedValue(value, 16,
                             clientCache_ -> putImageXCache, 8);
          clientCache_ -> putImageLastX += value;
          clientCache_ -> putImageLastX &= 0xffff;
          PutUINT(clientCache_ -> putImageLastX, outputMessage + 8, bigEndian_);
          // Y.
          decodeBuffer.decodeCachedValue(value, 16,
                             clientCache_ -> putImageYCache, 8);
          clientCache_ -> putImageLastY += value;
          clientCache_ -> putImageLastY &= 0xffff;
          PutUINT(clientCache_ -> putImageLastY, outputMessage + 10, bigEndian_);
          // Width.
          unsigned int width;
          decodeBuffer.decodeCachedValue(width, 16,
                             clientCache_ -> putImageWidthCache, 8);
          PutUINT(width, outputMessage + 12, bigEndian_);
          // Height.
          unsigned int height;
          decodeBuffer.decodeCachedValue(height, 16,
                             clientCache_ -> putImageHeightCache, 8);
          PutUINT(height, outputMessage + 14, bigEndian_);
          // Plane mask.
          decodeBuffer.decodeCachedValue(value, 32,
                             clientCache_ -> getImagePlaneMaskCache, 5);
          PutULONG(value, outputMessage + 16, bigEndian_);

          sequenceQueue_.push(clientSequence_, outputOpcode);

          handleSave(messageStore, outputMessage, outputLength);
        }
        break;
      case X_GetPointerMapping:
        {
          outputLength = 4;
          outputMessage = writeBuffer_.addMessage(outputLength);

          sequenceQueue_.push(clientSequence_, outputOpcode);
        }
        break;
      case X_GetKeyboardControl:
        {
          outputLength = 4;
          outputMessage = writeBuffer_.addMessage(outputLength);

          sequenceQueue_.push(clientSequence_, outputOpcode);
        }
        break;
      default:
        {
          if (outputOpcode == opcodeStore_ -> renderExtension)
          {
            MessageStore *messageStore = clientStore_ ->
                                 getRequestStore(X_NXInternalRenderExtension);

            hit = handleDecode(decodeBuffer, clientCache_, messageStore,
                                   outputOpcode, outputMessage, outputLength);
          }
          else if (outputOpcode == opcodeStore_ -> shapeExtension)
          {
            MessageStore *messageStore = clientStore_ ->
                                 getRequestStore(X_NXInternalShapeExtension);

            hit = handleDecode(decodeBuffer, clientCache_, messageStore,
                                   outputOpcode, outputMessage, outputLength);
          }
          else if (outputOpcode == opcodeStore_ -> putPackedImage)
          {
            #ifdef DEBUG
            *logofs << "handleWrite: Decoding packed image request for FD#"
                    << fd_ << ".\n" << logofs_flush;
            #endif

            MessageStore *messageStore = clientStore_ ->
                                 getRequestStore(X_NXPutPackedImage);

            hit = handleDecode(decodeBuffer, clientCache_, messageStore,
                                   outputOpcode, outputMessage, outputLength);

            if (outputOpcode == opcodeStore_ -> putPackedImage)
            {
              handleImage(outputOpcode, outputMessage, outputLength);
            }
          }
          else if (outputOpcode == opcodeStore_ -> setUnpackColormap)
          {
            #ifdef DEBUG
            *logofs << "handleWrite: Decoding set unpack colormap request "
                    << "for FD#" << fd_ << ".\n" << logofs_flush;
            #endif

            MessageStore *messageStore = clientStore_ ->
                                 getRequestStore(X_NXSetUnpackColormap);

            hit = handleDecode(decodeBuffer, clientCache_, messageStore,
                                   outputOpcode, outputMessage, outputLength);
            //
            // Message could have been split.
            //

            if (outputOpcode == opcodeStore_ -> setUnpackColormap)
            {
              handleColormap(outputOpcode, outputMessage, outputLength);
            }
          }
          else if (outputOpcode == opcodeStore_ -> setUnpackAlpha)
          {
            #ifdef DEBUG
            *logofs << "handleWrite: Decoding unpack alpha request for FD#"
                    << fd_ << ".\n" << logofs_flush;
            #endif

            MessageStore *messageStore = clientStore_ ->
                                 getRequestStore(X_NXSetUnpackAlpha);

            hit = handleDecode(decodeBuffer, clientCache_, messageStore,
                                   outputOpcode, outputMessage, outputLength);
            //
            // Message could have been split.
            //

            if (outputOpcode == opcodeStore_ -> setUnpackAlpha)
            {
              handleAlpha(outputOpcode, outputMessage, outputLength);
            }
          }
          else if (outputOpcode == opcodeStore_ -> setUnpackGeometry)
          {
            #ifdef DEBUG
            *logofs << "handleWrite: Decoding set unpack geometry request "
                    << "for FD#" << fd_ << ".\n" << logofs_flush;
            #endif

            MessageStore *messageStore = clientStore_ ->
                                 getRequestStore(X_NXSetUnpackGeometry);

            hit = handleDecode(decodeBuffer, clientCache_, messageStore,
                                   outputOpcode, outputMessage, outputLength);

            handleGeometry(outputOpcode, outputMessage, outputLength);
          }
          else if (outputOpcode == opcodeStore_ -> startSplit)
          {
            handleStartSplitRequest(decodeBuffer, outputOpcode,
                                        outputMessage, outputLength);
          }
          else if (outputOpcode == opcodeStore_ -> endSplit)
          {
            handleEndSplitRequest(decodeBuffer, outputOpcode,
                                      outputMessage, outputLength);
          }
          else if (outputOpcode == opcodeStore_ -> commitSplit)
          {
            int result = handleCommitSplitRequest(decodeBuffer, outputOpcode,
                                                      outputMessage, outputLength);

            //
            // Check if message has been successfully
            // extracted from the split store. In this
            // case post-process it in the usual way.
            //

            if (result > 0)
            {
              if (outputOpcode == opcodeStore_ -> putPackedImage ||
                      outputOpcode == X_PutImage)
              {
                handleImage(outputOpcode, outputMessage, outputLength);
              }
              else if (outputOpcode == opcodeStore_ -> setUnpackColormap)
              {
                handleColormap(outputOpcode, outputMessage, outputLength);
              }
              else if (outputOpcode == opcodeStore_ -> setUnpackAlpha)
              {
                handleAlpha(outputOpcode, outputMessage, outputLength);
              }
            }
            else if (result < 0)
            {
              return -1;
            }
          }
          else if (outputOpcode == opcodeStore_ -> abortSplit)
          {
            handleAbortSplitRequest(decodeBuffer, outputOpcode,
                                        outputMessage, outputLength);
          }
          else if (outputOpcode == opcodeStore_ -> finishSplit)
          {
            #ifdef DEBUG
            *logofs << "handleWrite: Decoding finish split request "
                    << "for FD#" << fd_ << ".\n" << logofs_flush;
            #endif

            decodeBuffer.decodeCachedValue(cValue, 8,
                         clientCache_ -> resourceCache);

            handleNullRequest(outputOpcode, outputMessage, outputLength);
          }
          else if (outputOpcode == opcodeStore_ -> freeSplit)
          {
            #ifdef DEBUG
            *logofs << "handleWrite: Decoding free split request "
                    << "for FD#" << fd_ << ".\n" << logofs_flush;
            #endif

            decodeBuffer.decodeCachedValue(cValue, 8,
                         clientCache_ -> resourceCache);

            handleNullRequest(outputOpcode, outputMessage, outputLength);
          }
          else if (outputOpcode == opcodeStore_ -> freeUnpack)
          {
            #ifdef DEBUG
            *logofs << "handleWrite: Decoding free unpack request "
                    << "for FD#" << fd_ << ".\n" << logofs_flush;
            #endif

            decodeBuffer.decodeCachedValue(cValue, 8,
                         clientCache_ -> resourceCache);

            #ifdef DEBUG
            *logofs << "handleWrite: Freeing unpack state for resource "
                    << (unsigned int) cValue << ".\n" << logofs_flush;
            #endif

            handleUnpackStateRemove(cValue);

            handleNullRequest(outputOpcode, outputMessage, outputLength);
          }
          else if (outputOpcode == opcodeStore_ -> setExposeParameters)
          {
            //
            // Send expose events according to agent's wish.
            //

            decodeBuffer.decodeBoolValue(enableExpose_);
            decodeBuffer.decodeBoolValue(enableGraphicsExpose_);
            decodeBuffer.decodeBoolValue(enableNoExpose_);

            handleNullRequest(outputOpcode, outputMessage, outputLength);
          }
          else if (outputOpcode == opcodeStore_ -> getUnpackParameters)
          {
            //
            // Client proxy needs the list of supported
            // unpack methods. We would need an encode
            // buffer, but this is in proxy, not here in
            // channel.
            //

            #ifdef TEST
            *logofs << "handleWrite: Sending X_GetInputFocus request for FD#" 
                    << fd_ << " due to OPCODE#" << (unsigned int) outputOpcode
                    << ".\n" << logofs_flush;
            #endif

            outputOpcode = X_GetInputFocus;

            outputLength  = 4;
            outputMessage = writeBuffer_.addMessage(outputLength);

            sequenceQueue_.push(clientSequence_, outputOpcode,
                                    opcodeStore_ -> getUnpackParameters);
          }
          else if (outputOpcode == opcodeStore_ -> getControlParameters ||
                       outputOpcode == opcodeStore_ -> getCleanupParameters ||
                           outputOpcode == opcodeStore_ -> getImageParameters)
          {
            handleNullRequest(outputOpcode, outputMessage, outputLength);
          }
          else if (outputOpcode == opcodeStore_ -> getShmemParameters)
          {
            if (handleShmemRequest(decodeBuffer, outputOpcode,
                                       outputMessage, outputLength) < 0)
            {
              return -1;
            }
          }
          else if (outputOpcode == opcodeStore_ -> setCacheParameters)
          {
            if (handleCacheRequest(decodeBuffer, outputOpcode,
                                       outputMessage, outputLength) < 0)
            {
              return -1;
            }
          }
          else if (outputOpcode == opcodeStore_ -> getFontParameters)
          {
            if (handleFontRequest(decodeBuffer, outputOpcode,
                                      outputMessage, outputLength) < 0)
            {
              return -1;
            }
          }
          else
          {
            MessageStore *messageStore = clientStore_ ->
                                 getRequestStore(X_NXInternalGenericRequest);

            hit = handleDecode(decodeBuffer, clientCache_, messageStore,
                                   outputOpcode, outputMessage, outputLength);
          }
        }
      } // End of switch on opcode.

      //
      // A packed image request can generate more than just
      // a single X_PutImage. Write buffer is handled inside
      // handleUnpack(). Cannot simply assume that the final
      // opcode and size must be put at the buffer offset as
      // as buffer could have been grown or could have been
      // replaced by a scratch buffer. The same is true in
      // the case of a shared memory image.
      //

      if (outputOpcode != 0)
      {
        //
        // Commit opcode and size to the buffer.
        //

        *outputMessage = (unsigned char) outputOpcode;

        PutUINT(outputLength >> 2, outputMessage + 2, bigEndian_);

        #if defined(TEST) || defined(OPCODES)
        *logofs << "handleWrite: Handled request OPCODE#"
                << (unsigned int) outputOpcode << " ("
                << DumpOpcode(outputOpcode) << ") for FD#"
                << fd_ << " sequence " << clientSequence_
                << ". "  << outputLength << " bytes out.\n"
                << logofs_flush;
        #endif
      }
      #if defined(TEST) || defined(OPCODES)
      else
      {
        //
        // In case of shared memory images the log doesn't
        // reflect the actual opcode of the request that is
        // going to be written. It would be possible to find
        // the opcode of the original request received from
        // the remote proxy in member imageState_ -> opcode,
        // but we have probably already deleted the struct.
        //

        *logofs << "handleWrite: Handled image request for FD#"
                  << fd_ << " new sequence " << clientSequence_
                  << ". " << outputLength << " bytes out.\n"
                  << logofs_flush;
      }
      #endif

      //
      // Check if we produced enough data. We need to
      // decode all the proxy messages or the decode
      // buffer will be left in an inconsistent state,
      // so we just update the finish flag in case of
      // failure.
      //

      handleFlush(flush_if_needed);

    } // End of while (decodeBuffer.decodeOpcodeValue(outputOpcode, 8, ...

  } // End of the decoding block.

  //
  // Write any remaining data to the X connection.
  //

  if (handleFlush(flush_if_any) < 0)
  {
    return -1;
  }

  //
  // Reset offset at which we read the
  // last event looking for the shared
  // memory completion.
  //

  if (shmemState_ != NULL)
  {
    shmemState_ -> checked = 0;
  }

  return 1;
}

//
// End of handleWrite().
//

//
// Other members.
//

int ServerChannel::handleSplit(DecodeBuffer &decodeBuffer, MessageStore *store,
                                   T_store_action action, int position, unsigned char &opcode,
                                       unsigned char *&buffer, unsigned int &size)
{
  if (control -> isProtoStep7() == 1)
  {
    splitState_.current = splitState_.resource;
  }

  handleSplitStoreAlloc(&splitResources_, splitState_.current);

  #if defined(TEST) || defined(SPLIT)
  *logofs << "handleSplit: SPLIT! Message OPCODE#"
          << (unsigned int) store -> opcode() << " of size " << size
          << " [split] with resource " << splitState_.current
          << " position " << position << " and action ["
          << DumpAction(action) << "] at " << strMsTimestamp()
          << ".\n" << logofs_flush;
  #endif

  //
  // Get the MD5 of the message being
  // split.
  //

  T_checksum checksum = NULL;

  if (action != IS_HIT)
  {
    handleSplitChecksum(decodeBuffer, checksum);
  }

  //
  // The method must abort the connection
  // if it can't allocate the split.
  //

  Split *splitMessage = clientStore_ -> getSplitStore(splitState_.current) ->
                              add(store, splitState_.current, position,
                                      action, checksum, buffer, size);

  //
  // If we are connected to an old proxy
  // version or the encoding side didn't
  // provide a checksum, then don't send
  // the split report.
  //

  if (control -> isProtoStep7() == 0 ||
          checksum == NULL)
  {
    if (action == IS_HIT)
    {
      splitMessage -> setState(split_loaded);
    }
    else
    {
      splitMessage -> setState(split_missed);
    }

    #if defined(TEST) || defined(SPLIT)

    *logofs << "handleSplit: SPLIT! There are " << clientStore_ ->
               getSplitTotalSize() << " messages and " << clientStore_ ->
               getSplitTotalStorageSize() << " bytes to send in "
            << "the split stores.\n" << logofs_flush;

    clientStore_ -> dumpSplitStore(splitState_.current);

    #endif

    delete [] checksum;

    return 1;
  }

  delete [] checksum;

  //
  // Tell the split store if it must use
  // the disk cache to retrieve and save
  // the message.
  //

  splitMessage -> setPolicy(splitState_.load, splitState_.save);

  //
  // Try to locate the message on disk.
  //

  if (clientStore_ -> getSplitStore(splitState_.current) ->
          load(splitMessage) == 1)
  {
    #if defined(TEST) || defined(SPLIT)
    *logofs << "handleSplit: SPLIT! Loaded the message "
            << "from the image cache.\n" << logofs_flush;
    #endif

    splitMessage -> setState(split_loaded);
  }
  else
  {
    #if defined(TEST) || defined(SPLIT)
    *logofs << "handleSplit: WARNING! SPLIT! Can't find the message "
            << "in the image cache.\n" << logofs_flush;
    #endif

    splitMessage -> setState(split_missed);
  }

  #if defined(TEST) || defined(SPLIT)

  T_timestamp startTs = getTimestamp();

  *logofs << "handleSplit: SPLIT! Encoding abort "
          << "split events for FD#" << fd_ << " at "
          << strMsTimestamp() << ".\n" << logofs_flush;
  #endif

  if (proxy -> handleAsyncSplit(fd_, splitMessage) < 0)
  {
    return -1;
  }

  //
  // Send the encoded data immediately. We
  // want the abort split message to reach
  // the remote proxy as soon as possible.
  //

  if (proxy -> handleAsyncFlush() < 0)
  {
    return -1;
  }

  #if defined(TEST) || defined(SPLIT)

  *logofs << "handleSplit: SPLIT! Spent "
          << diffTimestamp(startTs, getTimestamp()) << " Ms "
          << "handling abort split events for FD#" << fd_
          << ".\n" << logofs_flush;

  *logofs << "handleSplit: SPLIT! There are " << clientStore_ ->
             getSplitTotalSize() << " messages and " << clientStore_ ->
             getSplitTotalStorageSize() << " bytes to send in "
          << "the split stores.\n" << logofs_flush;

  clientStore_ -> dumpSplitStore(splitState_.current);

  #endif

  return 1;
}

int ServerChannel::handleSplit(DecodeBuffer &decodeBuffer)
{
  #if defined(TEST) || defined(SPLIT)
  *logofs << "handleSplit: SPLIT! Going to handle splits "
          << "for FD#" << fd_ << " at " << strMsTimestamp()
          << ".\n" << logofs_flush;
  #endif

  unsigned char resource;

  if (control -> isProtoStep7() == 1)
  {
    decodeBuffer.decodeCachedValue(resource, 8,
                       clientCache_ -> resourceCache);

    splitState_.current = resource;
  }

  handleSplitStoreAlloc(&splitResources_, splitState_.current);

  SplitStore *splitStore = clientStore_ -> getSplitStore(splitState_.current);

  #if defined(TEST) || defined(SPLIT)
  *logofs << "handleSplit: SPLIT! Handling splits for "
          << "resource [" << splitState_.current << "] with "
          << splitStore -> getSize() << " elements "
          << "in the split store.\n" << logofs_flush;
  #endif

  int result = splitStore -> receive(decodeBuffer);

  if (result < 0)
  {
    #ifdef PANIC
    *logofs << "handleSplit: PANIC! Receive of split for FD#" << fd_
            << " failed.\n" << logofs_flush;
    #endif

    cerr << "Error" << ": Receive of split for FD#" << fd_
         << " failed.\n";

    return -1;
  }
  else if (result == 0)
  {
    //
    // The split is still incomplete. It's time
    // to check if we need to start the house-
    // keeping process to take care of the image
    // cache.
    //

    if (proxy -> handleAsyncKeeperCallback() < 0)
    {
      return -1;
    }
  }
  else
  {
    //
    // Note that we don't need the resource id at the
    // X server side and, thus, we don't provide it
    // at the time we add split to the split store.
    //

    #if defined(TEST) || defined(SPLIT)
    *logofs << "handleSplit: SPLIT! Remote agent should "
            << "now commit a new split for resource ["
            << splitState_.current << "].\n"
            << logofs_flush;

    clientStore_ -> dumpCommitStore();

    #endif

    if (splitStore -> getSize() == 0)
    {
      #if defined(TEST) || defined(SPLIT)
      *logofs << "handleSplit: SPLIT! Removing split store "
              << "for resource [" << splitState_.current
              << "] at " << strMsTimestamp() << ".\n"
              << logofs_flush;
      #endif

      handleSplitStoreRemove(&splitResources_, splitState_.current);

      #if defined(TEST) || defined(SPLIT)
      *logofs << "handleSplit: SPLIT! There are [" << clientStore_ ->
                 getSplitTotalSize() << "] messages and " << clientStore_ ->
                 getSplitTotalStorageSize() << " bytes to send in "
              << "the split stores.\n" << logofs_flush;
      #endif
    }
    else
    {
      //
      // If the next split is discarded, it can be
      // that, since the beginning of the split, we
      // have saved the message on the disk, due to
      // a more recent split operation. This is also
      // the case when we had to discard the message
      // because it was locked but, since then, we
      // completed the transferral of the split.
      //

      Split *splitMessage = splitStore -> getFirstSplit();

      if (splitMessage -> getAction() == is_discarded &&
            splitMessage -> getState() == split_missed &&
                splitStore -> load(splitMessage) == 1)
      {
        #if defined(TEST) || defined(SPLIT)
        *logofs << "handleSplit: WARNING! SPLIT! Asynchronously "
                << "loaded the message from the image cache.\n"
                << logofs_flush;
        #endif

        splitMessage -> setState(split_loaded);

        #if defined(TEST) || defined(SPLIT)

        T_timestamp startTs = getTimestamp();

        *logofs << "handleSplit: WARNING! SPLIT! Asynchronously "
                << "encoding abort split events for FD#" << fd_
                << " at " << strMsTimestamp() << ".\n"
                << logofs_flush;
        #endif

        if (proxy -> handleAsyncSplit(fd_, splitMessage) < 0)
        {
          return -1;
        }

        //
        // Send the encoded data immediately. We
        // want the abort split message to reach
        // the remote proxy as soon as possible.
        //

        if (proxy -> handleAsyncFlush() < 0)
        {
          return -1;
        }

        #if defined(TEST) || defined(SPLIT)

        *logofs << "handleSplit: WARNING! SPLIT! Spent "
                << diffTimestamp(startTs, getTimestamp()) << " Ms "
                << "handling asynchronous abort split events for "
                << "FD#" << fd_ << ".\n" << logofs_flush;

        *logofs << "handleSplit: SPLIT! There are " << clientStore_ ->
                   getSplitTotalSize() << " messages and " << clientStore_ ->
                   getSplitTotalStorageSize() << " bytes to send in "
                << "the split stores.\n" << logofs_flush;

        clientStore_ -> dumpSplitStore(splitState_.current);

        #endif
      }
    }
  }

  return 1;
}

int ServerChannel::handleSplitEvent(EncodeBuffer &encodeBuffer, Split *splitMessage)
{
  int resource = splitMessage -> getResource();

  #if defined(TEST) || defined(SPLIT)
  *logofs << "handleSplitEvent: SPLIT! Going to send a "
          << "split report for resource " << resource
          << ".\n" << logofs_flush;
  #endif

  //
  // This function is called only after the message
  // has been searched in the disk cache. We need to
  // inform the other side if the data transfer can
  // start or it must be aborted to let the local
  // side use the copy that was found on the disk.
  //

  #if defined(TEST) || defined(INFO)

  if (splitMessage -> getState() != split_loaded &&
          splitMessage -> getState() != split_missed)
  {
    *logofs << "handleSplitEvent: PANIC! Can't find the split to be aborted.\n"
            << logofs_flush;

    HandleCleanup();
  }

  #endif

  //
  // We need to send a boolean telling if the split
  // was found or not, followed by the checksum of
  // message we are referencing.
  //

  T_checksum checksum = splitMessage -> getChecksum();

  #if defined(TEST) || defined(SPLIT)
  *logofs << "handleSplitEvent: SPLIT! Sending split report for "
          << "checksum [" << DumpChecksum(checksum) << "].\n"
          << logofs_flush;
  #endif

  if (proxy -> handleAsyncSwitch(fd_) < 0)
  {
    return -1;
  }

  encodeBuffer.encodeOpcodeValue(opcodeStore_ -> splitEvent,
                                     serverCache_ -> opcodeCache);

  //
  // The encoding in older protocol versions
  // is different but we will never try to
  // send a split report if the remote does
  // not support our version.
  //

  encodeBuffer.encodeCachedValue(resource, 8,
                     serverCache_ -> resourceCache);

  if (splitMessage -> getState() == split_loaded)
  {
    encodeBuffer.encodeBoolValue(1);

    encodeBuffer.encodeOpcodeValue(splitMessage -> getStore() -> opcode(),
                                       serverCache_ -> abortOpcodeCache);

    encodeBuffer.encodeValue(splitMessage -> compressedSize(), 32, 14);
  }
  else
  {
    encodeBuffer.encodeBoolValue(0);
  }

  for (unsigned int i = 0; i < MD5_LENGTH; i++)
  {
    encodeBuffer.encodeValue((unsigned int) checksum[i], 8);
  }

  //
  // Update statistics for this special opcode.
  //

  int bits = encodeBuffer.diffBits();

  #if defined(TEST) || defined(OPCODES) || defined(INFO) || defined(SPLIT)
  *logofs << "handleSplitEvent: SPLIT! Handled event OPCODE#"
          << (unsigned int) opcodeStore_ -> splitEvent << " ("
          << DumpOpcode(opcodeStore_ -> splitEvent) << ")" << " for FD#"
          << fd_ << " sequence none. 0 bytes in, " << bits << " bits ("
          << ((float) bits) / 8 << " bytes) out.\n" << logofs_flush;
  #endif

  statistics -> addEventBits(opcodeStore_ -> splitEvent, 0, bits);

  return 1;
}

int ServerChannel::handleAbortSplitRequest(DecodeBuffer &decodeBuffer, unsigned char &opcode,
                                               unsigned char *&buffer, unsigned int &size)
{
  unsigned char resource;

  decodeBuffer.decodeCachedValue(resource, 8,
                     clientCache_ -> resourceCache);

  #if defined(TEST) || defined(SPLIT)
  *logofs << "handleAbortSplitRequest: SPLIT! Handling abort split "
          << "request for FD#" << fd_ << " and resource "
          << (unsigned) resource << ".\n"
          << logofs_flush;
  #endif

  int splits = 0;

  SplitStore *splitStore = clientStore_ -> getSplitStore(resource);

  if (splitStore != NULL)
  {
    //
    // Discard from the memory cache the messages
    // that are still incomplete and then get rid
    // of the splits in the store.
    //

    #if defined(TEST) || defined(SPLIT)

    clientStore_ -> dumpSplitStore(resource);

    #endif

    Split *splitMessage;

    for (;;)
    {
      splitMessage = splitStore -> getFirstSplit();

      if (splitMessage == NULL)
      {
        //
        // Check if we had created the store
        // but no message was added yet.
        //

        #ifdef WARNING

        if (splits == 0)
        {
          *logofs << "handleAbortSplitRequest: WARNING! SPLIT! The "
                  << "split store for resource [" << (unsigned int)
                     resource << "] is unexpectedly empty.\n"
                  << logofs_flush;
        }

        #endif

        break;
      }

      //
      // Splits already aborted can't be in the
      // split store.
      //

      #if defined(TEST) || defined(SPLIT)

      if (splitMessage -> getState() == split_aborted)
      {
        *logofs << "handleAbortSplitRequest: PANIC! SPLIT! Found an "
                << "aborted split in store [" << (unsigned int) resource
                << "].\n" << logofs_flush;

        HandleCleanup();
      }

      #endif

      if (splitMessage -> getAction() == IS_HIT)
      {
        #if defined(TEST) || defined(SPLIT)
        *logofs << "handleAbortSplitRequest: SPLIT! Removing the "
                << "split from the memory cache.\n"
                << logofs_flush;
        #endif

        splitMessage -> getStore() -> remove(splitMessage -> getPosition(),
                                                 discard_checksum, use_data);
      }

      #if defined(TEST) || defined(SPLIT)
      *logofs << "handleAbortSplitRequest: SPLIT! Removing the "
              << "split from the split store.\n"
              << logofs_flush;
      #endif

      splitMessage = splitStore -> pop();

      #if defined(TEST) || defined(SPLIT)
      *logofs << "handleAbortSplitRequest: SPLIT! Freeing up the "
              << "aborted split.\n" << logofs_flush;
      #endif

      delete splitMessage;

      splits++;
    }
  }
  #ifdef WARNING
  else
  {
    *logofs << "handleAbortSplitRequest: WARNING! SPLIT! The "
            << "split store for resource [" << (unsigned int)
               resource << "] is already empty.\n"
            << logofs_flush;
  }
  #endif

  handleNullRequest(opcode, buffer, size);

  return (splits > 0);
}

int ServerChannel::handleCommitSplitRequest(DecodeBuffer &decodeBuffer, unsigned char &opcode,
                                                unsigned char *&buffer, unsigned int &size)
{
  //
  // Get request type and position of the image
  // to commit.
  //

  unsigned char request;

  decodeBuffer.decodeOpcodeValue(request, clientCache_ -> opcodeCache);

  unsigned int diffCommit;

  decodeBuffer.decodeValue(diffCommit, 32, 5);

  splitState_.commit += diffCommit;

  unsigned char resource = 0;
  unsigned int commit = 1;

  //
  // Send the resource id and the commit flag.
  // The resource id is ignored at the moment.
  // The message will be handled based on the
  // resource id that was sent together with
  // the original message.
  //

  decodeBuffer.decodeCachedValue(resource, 8,
                     clientCache_ -> resourceCache);

  decodeBuffer.decodeBoolValue(commit);

  Split *split = handleSplitCommitRemove(request, resource, splitState_.commit);

  if (split == NULL)
  {
    return -1;
  }

  clientStore_ -> getCommitStore() -> update(split);

  if (commit == 1)
  {
    #if defined(TEST) || defined(SPLIT)
    *logofs << "handleCommitSplitRequest: SPLIT! Handling split commit "
            << "for FD#" << fd_ << " with commit " << commit
            << " request " << (unsigned) request << " resource "
            << (unsigned) resource << " and position "
            << splitState_.commit << ".\n"
            << logofs_flush;
    #endif

    //
    // Allocate as many bytes in the write
    // buffer as the final length of the
    // message in uncompressed form.
    //

    size = split -> plainSize();

    buffer = writeBuffer_.addMessage(size);

    #if defined(TEST) || defined(SPLIT)
    *logofs << "handleCommitSplitRequest: SPLIT! Prepared an "
            << "outgoing buffer of " << size << " bytes.\n"
            << logofs_flush;
    #endif

    if (clientStore_ -> getCommitStore() -> expand(split, buffer, size) < 0)
    {
      writeBuffer_.removeMessage(size);

      commit = 0;
    }
  }

  //
  // Free the split.
  //

  #if defined(TEST) || defined(SPLIT)
  *logofs << "handleCommitSplitRequest: SPLIT! Freeing up the "
          << "committed split.\n" << logofs_flush;
  #endif

  delete split;

  //
  // Discard the operation and send a null
  // message.
  //

  if (commit == 0)
  {
    handleNullRequest(opcode, buffer, size);
  }
  else
  {
    //
    // Save the sequence number to be able
    // to mask any error generated by the
    // request.
    //

    updateCommitQueue(clientSequence_);

    //
    // Now in the write buffer there is
    // a copy of this request.
    //

    opcode = request;
  }

  return commit;
}

int ServerChannel::handleGeometry(unsigned char &opcode, unsigned char *&buffer,
                                      unsigned int &size)
{
  //
  // Replace the old geometry and taint
  // the message into a X_NoOperation.
  //

  int resource = *(buffer + 1);

  #ifdef TEST
  *logofs << "handleGeometry: Setting new unpack geometry "
          << "for resource " << resource << ".\n"
          << logofs_flush;
  #endif

  handleUnpackStateInit(resource);

  handleUnpackAllocGeometry(resource);

  unpackState_[resource] -> geometry -> depth1_bpp  = *(buffer + 4);
  unpackState_[resource] -> geometry -> depth4_bpp  = *(buffer + 5);
  unpackState_[resource] -> geometry -> depth8_bpp  = *(buffer + 6);
  unpackState_[resource] -> geometry -> depth16_bpp = *(buffer + 7);
  unpackState_[resource] -> geometry -> depth24_bpp = *(buffer + 8);
  unpackState_[resource] -> geometry -> depth32_bpp = *(buffer + 9);

  unpackState_[resource] -> geometry -> red_mask   = GetULONG(buffer + 12, bigEndian_);
  unpackState_[resource] -> geometry -> green_mask = GetULONG(buffer + 16, bigEndian_);
  unpackState_[resource] -> geometry -> blue_mask  = GetULONG(buffer + 20, bigEndian_);

  handleCleanAndNullRequest(opcode, buffer, size);

  return 1;
}

int ServerChannel::handleColormap(unsigned char &opcode, unsigned char *&buffer,
                                      unsigned int &size)
{
  //
  // Replace the old colormap and taint
  // the message into a X_NoOperation.
  //

  int resource = *(buffer + 1);

  #ifdef TEST
  *logofs << "handleColormap: Setting new unpack colormap "
          << "for resource " << resource << ".\n"
          << logofs_flush;
  #endif

  handleUnpackStateInit(resource);

  handleUnpackAllocColormap(resource);

  //
  // New protocol versions send the alpha
  // data in compressed form.
  //

  if (control -> isProtoStep7() == 1)
  {
    unsigned int packed   = GetULONG(buffer + 8, bigEndian_);
    unsigned int unpacked = GetULONG(buffer + 12, bigEndian_);

    validateSize("colormap", packed, unpacked, 16, size);

    if (unpackState_[resource] -> colormap -> entries != unpacked >> 2 &&
            unpackState_[resource] -> colormap -> data != NULL)
    {
      #ifdef TEST
      *logofs << "handleColormap: Freeing previously allocated "
              << "unpack colormap data.\n" << logofs_flush;
      #endif

      delete [] unpackState_[resource] -> colormap -> data;

      unpackState_[resource] -> colormap -> data    = NULL;
      unpackState_[resource] -> colormap -> entries = 0;
    }

    #ifdef TEST
    *logofs << "handleColormap: Setting " << unpacked
            << " bytes of unpack colormap data for resource "
            << resource << ".\n" << logofs_flush;
    #endif

    if (unpackState_[resource] -> colormap -> data == NULL)
    {
      unpackState_[resource] -> colormap -> data =
            (unsigned int *) new unsigned char[unpacked];

      if (unpackState_[resource] -> colormap -> data == NULL)
      {
        #ifdef PANIC
        *logofs << "handleColormap: PANIC! Can't allocate "
                << unpacked << " entries for unpack colormap data "
                << "for FD#" << fd_ << ".\n" << logofs_flush;
        #endif

        goto handleColormapEnd;
      }

      #ifdef DEBUG
      *logofs << "handleColormap: Size of new colormap data is "
              << unpacked << ".\n" << logofs_flush;
      #endif
    }

    unsigned int method = *(buffer + 4);

    if (method == PACK_COLORMAP)
    {
      if (UnpackColormap(method, buffer + 16, packed,
                             (unsigned char *) unpackState_[resource] ->
                                 colormap -> data, unpacked) < 0)
      {
        #ifdef PANIC
        *logofs << "handleColormap: PANIC! Can't unpack " << packed
                << " bytes to " << unpacked << " entries for FD#"
                << fd_ << ".\n" << logofs_flush;
        #endif

        delete [] unpackState_[resource] -> colormap -> data;

        unpackState_[resource] -> colormap -> data    = NULL;
        unpackState_[resource] -> colormap -> entries = 0;

        goto handleColormapEnd;
      }
    }
    else
    {
      memcpy((unsigned char *) unpackState_[resource] ->
                 colormap -> data, buffer + 16, unpacked);
    }

    unpackState_[resource] -> colormap -> entries = unpacked >> 2;

    #if defined(DEBUG) && defined(DUMP)

    *logofs << "handleColormap: Dumping colormap entries:\n"
            << logofs_flush;

    const unsigned char *p = (const unsigned char *) unpackState_[resource] -> colormap -> data;

    for (unsigned int i = 0; i < unpackState_[resource] ->
             colormap -> entries; i++)
    {
      *logofs << "handleColormap: [" << i << "] ["
              << (void *) ((int) p[i]) << "].\n"
              << logofs_flush;
    }

    #endif
  }
  else
  {
    unsigned int entries = GetULONG(buffer + 4, bigEndian_);

    if (size == entries * 4 + 8)
    {
      if (unpackState_[resource] -> colormap -> entries != entries &&
              unpackState_[resource] -> colormap -> data != NULL)
      {
        #ifdef TEST
        *logofs << "handleColormap: Freeing previously "
                << "allocated unpack colormap.\n"
                << logofs_flush;
        #endif

        delete [] unpackState_[resource] -> colormap -> data;

        unpackState_[resource] -> colormap -> data    = NULL;
        unpackState_[resource] -> colormap -> entries = 0;
      }

      if (entries > 0)
      {
        if (unpackState_[resource] -> colormap -> data == NULL)
        {
          unpackState_[resource] ->
                colormap -> data = new unsigned int[entries];
        }

        if (unpackState_[resource] -> colormap -> data != NULL)
        {
          unpackState_[resource] -> colormap -> entries = entries;

          #ifdef DEBUG
          *logofs << "handleColormap: Size of new colormap "
                  << "data is " << (entries << 2) << ".\n"
                  << logofs_flush;
          #endif

          memcpy((unsigned char *) unpackState_[resource] ->
                     colormap -> data, buffer + 8, entries << 2);

          #if defined(DEBUG) && defined(DUMP)

          *logofs << "handleColormap: Dumping colormap entries:\n"
                  << logofs_flush;

          const unsigned int *p = (unsigned int *) buffer + 8;

          for (unsigned int i = 0; i < entries; i++)
          {
            *logofs << "handleColormap: [" << i << "] ["
                    << (void *) p[i] << "].\n"
                    << logofs_flush;
          }

          #endif
        }
        else
        {
          #ifdef PANIC
          *logofs << "handleColormap: PANIC! Can't allocate "
                  << entries << " entries for unpack colormap "
                  << "for FD#" << fd_ << ".\n" << logofs_flush;
          #endif
        }
      }
    }
    else
    {
      #ifdef PANIC
      *logofs << "handleColormap: PANIC! Bad size " << size
              << " for set unpack colormap message for FD#"
              << fd_ << " with " << entries << " entries.\n"
              << logofs_flush;
      #endif
    }
  }

handleColormapEnd:

  handleCleanAndNullRequest(opcode, buffer, size);

  return 1;
}

int ServerChannel::handleAlpha(unsigned char &opcode, unsigned char *&buffer,
                                      unsigned int &size)
{
  int resource = *(buffer + 1);

  #ifdef TEST
  *logofs << "handleAlpha: Setting new unpack alpha "
          << "for resource " << resource << ".\n"
          << logofs_flush;
  #endif

  handleUnpackStateInit(resource);

  handleUnpackAllocAlpha(resource);

  //
  // New protocol versions send the alpha
  // data in compressed form.
  //

  if (control -> isProtoStep7() == 1)
  {
    unsigned int packed   = GetULONG(buffer + 8, bigEndian_);
    unsigned int unpacked = GetULONG(buffer + 12, bigEndian_);

    validateSize("alpha", packed, unpacked, 16, size);

    if (unpackState_[resource] -> alpha -> entries != unpacked &&
            unpackState_[resource] -> alpha -> data != NULL)
    {
      #ifdef TEST
      *logofs << "handleAlpha: Freeing previously allocated "
              << "unpack alpha data.\n" << logofs_flush;
      #endif

      delete [] unpackState_[resource] -> alpha -> data;

      unpackState_[resource] -> alpha -> data    = NULL;
      unpackState_[resource] -> alpha -> entries = 0;
    }

    #ifdef TEST
    *logofs << "handleAlpha: Setting " << unpacked
            << " bytes of unpack alpha data for resource "
            << resource << ".\n" << logofs_flush;
    #endif

    if (unpackState_[resource] -> alpha -> data == NULL)
    {
      unpackState_[resource] -> alpha -> data = new unsigned char[unpacked];

      if (unpackState_[resource] -> alpha -> data == NULL)
      {
        #ifdef PANIC
        *logofs << "handleAlpha: PANIC! Can't allocate "
                << unpacked << " entries for unpack alpha data "
                << "for FD#" << fd_ << ".\n" << logofs_flush;
        #endif

        goto handleAlphaEnd;
      }

      #ifdef DEBUG
      *logofs << "handleAlpha: Size of new alpha data is "
              << unpacked << ".\n" << logofs_flush;
      #endif
    }

    unsigned int method = *(buffer + 4);

    if (method == PACK_ALPHA)
    {
      if (UnpackAlpha(method, buffer + 16, packed,
                          unpackState_[resource] -> alpha ->
                              data, unpacked) < 0)
      {
        #ifdef PANIC
        *logofs << "handleAlpha: PANIC! Can't unpack " << packed
                << " bytes to " << unpacked << " entries for FD#"
                << fd_ << ".\n" << logofs_flush;
        #endif

        delete [] unpackState_[resource] -> alpha -> data;

        unpackState_[resource] -> alpha -> data    = NULL;
        unpackState_[resource] -> alpha -> entries = 0;

        goto handleAlphaEnd;
      }
    }
    else
    {
      memcpy((unsigned char *) unpackState_[resource] ->
                 alpha -> data, buffer + 16, unpacked);
    }

    unpackState_[resource] -> alpha -> entries = unpacked;

    #if defined(DEBUG) && defined(DUMP)

    *logofs << "handleAlpha: Dumping alpha entries:\n"
            << logofs_flush;

    const unsigned char *p = unpackState_[resource] -> alpha -> data;

    for (unsigned int i = 0; i < unpackState_[resource] ->
             alpha -> entries; i++)
    {
      *logofs << "handleAlpha: [" << i << "] ["
              << (void *) ((int) p[i]) << "].\n"
              << logofs_flush;
    }

    #endif
  }
  else
  {
    unsigned int entries = GetULONG(buffer + 4, bigEndian_);

    if (size == RoundUp4(entries) + 8)
    {
      if (unpackState_[resource] -> alpha -> entries != entries &&
              unpackState_[resource] -> alpha -> data != NULL)
      {
        #ifdef TEST
        *logofs << "handleAlpha: Freeing previously allocated "
                << "unpack alpha data.\n" << logofs_flush;
        #endif

        delete [] unpackState_[resource] -> alpha -> data;

        unpackState_[resource] -> alpha -> data    = NULL;
        unpackState_[resource] -> alpha -> entries = 0;
      }

      if (entries > 0)
      {
        if (unpackState_[resource] -> alpha -> data == NULL)
        {
          unpackState_[resource] -> alpha -> data = new unsigned char[entries];
        }

        if (unpackState_[resource] -> alpha -> data != NULL)
        {
          unpackState_[resource] -> alpha -> entries = entries;

          #ifdef DEBUG
          *logofs << "handleAlpha: Size of new alpha data is "
                  << entries << ".\n" << logofs_flush;
          #endif

          memcpy((unsigned char *) unpackState_[resource] ->
                     alpha -> data, buffer + 8, entries);

          #if defined(DEBUG) && defined(DUMP)

          *logofs << "handleAlpha: Dumping alpha entries:\n"
                  << logofs_flush;

          const unsigned char *p = buffer + 8;

          for (unsigned int i = 0; i < entries; i++)
          {
            *logofs << "handleAlpha: [" << i << "] ["
                    << (void *) ((int) p[i]) << "].\n"
                    << logofs_flush;
          }

          #endif
        }
        else
        {
          #ifdef PANIC
          *logofs << "handleAlpha: PANIC! Can't allocate "
                  << entries << " entries for unpack alpha data "
                  << "for FD#" << fd_ << ".\n" << logofs_flush;
          #endif
        }
      }
    }
    #ifdef PANIC
    else
    {
      *logofs << "handleAlpha: PANIC! Bad size " << size
              << " for set unpack alpha message for FD#"
              << fd_ << " with " << entries << " entries.\n"
              << logofs_flush;
    }
    #endif
  }

handleAlphaEnd:

  handleCleanAndNullRequest(opcode, buffer, size);

  return 1;
}

int ServerChannel::handleImage(unsigned char &opcode, unsigned char *&buffer,
                                   unsigned int &size)
{
  int result = 1;

  //
  // Save the original opcode together with
  // the image state so we can later find if
  // this is a plain or a packed image when
  // moving data to the shared memory area.
  //

  handleImageStateAlloc(opcode);

  if (opcode == opcodeStore_ -> putPackedImage)
  {
    //
    // Unpack the image and put a X_PutImage in a
    // new buffer. Save the expected output size,
    // so, in the case of a decoding error we can
    // still update the statistics.
    //

    int length = GetULONG(buffer + 20, bigEndian_);

    #ifdef TEST
    *logofs << "handleImage: Sending image for FD#" << fd_
            << " due to OPCODE#" << (unsigned int) opcode << " with "
            << GetULONG(buffer + 16, bigEndian_) << " bytes packed "
            << "and " << GetULONG(buffer + 20, bigEndian_)
            << " bytes unpacked.\n" << logofs_flush;
    #endif

    statistics -> addPackedBytesIn(size);

    result = handleUnpack(opcode, buffer, size);

    if (result < 0)
    {
      //
      // Recover from the error. Send a X_NoOperation
      // to keep the sequence counter in sync with
      // the remote peer.
      //

      size   = 4;
      buffer = writeBuffer_.addMessage(size);

      *buffer = X_NoOperation;

      PutUINT(size >> 2, buffer + 2, bigEndian_);

      #ifdef PANIC
      *logofs << "handleImage: PANIC! Sending X_NoOperation for FD#"
              << fd_ << " to recover from failed unpack.\n"
              << logofs_flush;
      #endif

      //
      // Set the output length to reflect the amount of
      // data that would have been produced by unpacking
      // the image. This is advisable to keep the count-
      // ers in sync with those at remote proxy. Setting
      // the size here doesn't have any effect on the
      // size of data sent to the X server as the actual
      // size will be taken from the content of the write
      // buffer.
      //

      size = length;
    }

    statistics -> addPackedBytesOut(size);

    //
    // Refrain the write loop from putting
    // opcode and size in the output buffer.
    //

    opcode = 0;
  }

  //
  // Now image is unpacked as a X_PutImage
  // in write buffer. Check if we can send
  // the image using the MIT-SHM extension.
  //

  if (result > 0)
  {
    result = handleShmem(opcode, buffer, size);

    //
    // We already put opcode and size in
    // the resulting buffer.
    //

    if (result > 0)
    {
      opcode = 0;
    }
  }

  return 1;
}

int ServerChannel::handleMotion(EncodeBuffer &encodeBuffer)
{
  #if defined(TEST) || defined(INFO)

  if (lastMotion_[0] == '\0')
  {
    *logofs << "handleMotion: PANIC! No motion events to send "
            << "for FD#" << fd_ << ".\n" << logofs_flush;

    HandleCleanup();
  }

  #endif

  #if defined(TEST) || defined(INFO)
  *logofs << "handleMotion: Sending motion events for FD#"
          << fd_ << " at " << strMsTimestamp() << ".\n"
          << logofs_flush;
  #endif

  //
  // Replicate code from read loop. When have
  // time and wish, try to split everything
  // in functions.
  //

  if (proxy -> handleAsyncSwitch(fd_) < 0)
  {
    return -1;
  }

  const unsigned char *buffer = lastMotion_;
  unsigned char opcode = *lastMotion_;
  unsigned int size = 32;

  if (GetUINT(buffer + 2, bigEndian_) < serverSequence_)
  {
    PutUINT(serverSequence_, (unsigned char *) buffer + 2, bigEndian_);
  }

  encodeBuffer.encodeOpcodeValue(opcode, serverCache_ -> opcodeCache);

  unsigned int sequenceNum = GetUINT(buffer + 2, bigEndian_);

  unsigned int sequenceDiff = sequenceNum - serverSequence_;

  serverSequence_ = sequenceNum;

  #ifdef DEBUG
  *logofs << "handleMotion: Last server sequence number for FD#" 
          << fd_ << " is " << serverSequence_ << " with "
          << "difference " << sequenceDiff << ".\n"
          << logofs_flush;
  #endif

  encodeBuffer.encodeCachedValue(sequenceDiff, 16,
                     serverCache_ -> eventSequenceCache, 7);

  //
  // If we fast encoded the message
  // then skip the rest.
  //

  if (control -> LocalDeltaCompression == 0)
  {
    int result = handleFastReadEvent(encodeBuffer, opcode,
                                         buffer, size);

    #ifdef DEBUG
    *logofs << "handleMotion: Sent saved motion event for FD#"
            << fd_ << ".\n" << logofs_flush;
    #endif

    lastMotion_[0] = '\0';

    #ifdef DEBUG
    *logofs << "handleMotion: Reset last motion event for FD#"
            << fd_ << ".\n" << logofs_flush;
    #endif

    if (result < 0)
    {
      return -1;
    }
    else if (result > 0)
    {
      return 1;
    }
  }

  //
  // This should be just the part specific
  // for motion events but is currently a
  // copy-paste of code from the read loop.
  //

  unsigned char detail = buffer[1];
  if (*buffer == MotionNotify)
    encodeBuffer.encodeBoolValue((unsigned int) detail);
  else if ((*buffer == EnterNotify) || (*buffer == LeaveNotify))
    encodeBuffer.encodeValue((unsigned int) detail, 3);
  else if (*buffer == KeyRelease)
  {
    if (detail == serverCache_ -> keyPressLastKey)
      encodeBuffer.encodeBoolValue(1);
    else
    {
      encodeBuffer.encodeBoolValue(0);
      encodeBuffer.encodeValue((unsigned int) detail, 8);
    }
  }
  else if ((*buffer == ButtonPress) || (*buffer == ButtonRelease))
    encodeBuffer.encodeCachedValue(detail, 8,
                                   serverCache_ -> buttonCache);
  else
    encodeBuffer.encodeValue((unsigned int) detail, 8);
  unsigned int timestamp = GetULONG(buffer + 4, bigEndian_);
  unsigned int timestampDiff = timestamp - serverCache_ -> lastTimestamp;
  serverCache_ -> lastTimestamp = timestamp;
  encodeBuffer.encodeCachedValue(timestampDiff, 32,
                      serverCache_ -> motionNotifyTimestampCache, 9);
  int skipRest = 0;
  if (*buffer == KeyRelease)
  {
    skipRest = 1;
    for (unsigned int i = 8; i < 31; i++)
    {
      if (buffer[i] != serverCache_ -> keyPressCache[i - 8])
      {
        skipRest = 0;
        break;
      }
    }
    encodeBuffer.encodeBoolValue(skipRest);
  }
  if (!skipRest)
  {
    const unsigned char *nextSrc = buffer + 8;
    for (unsigned int i = 0; i < 3; i++)
    {
      encodeBuffer.encodeCachedValue(GetULONG(nextSrc, bigEndian_), 29,
                         *serverCache_ -> motionNotifyWindowCache[i], 6);
      nextSrc += 4;
    }
    unsigned int rootX = GetUINT(buffer + 20, bigEndian_);
    unsigned int rootY = GetUINT(buffer + 22, bigEndian_);
    unsigned int eventX = GetUINT(buffer + 24, bigEndian_);
    unsigned int eventY = GetUINT(buffer + 26, bigEndian_);
    eventX -= rootX;
    eventY -= rootY;
    encodeBuffer.encodeCachedValue(rootX -
          serverCache_ -> motionNotifyLastRootX, 16,
          serverCache_ -> motionNotifyRootXCache, 6);
    serverCache_ -> motionNotifyLastRootX = rootX;
    encodeBuffer.encodeCachedValue(rootY -
          serverCache_ -> motionNotifyLastRootY, 16,
          serverCache_ -> motionNotifyRootYCache, 6);
    serverCache_ -> motionNotifyLastRootY = rootY;
    encodeBuffer.encodeCachedValue(eventX, 16,
                         serverCache_ -> motionNotifyEventXCache, 6);
    encodeBuffer.encodeCachedValue(eventY, 16,
                         serverCache_ -> motionNotifyEventYCache, 6);
    encodeBuffer.encodeCachedValue(GetUINT(buffer + 28, bigEndian_),
                         16, serverCache_ -> motionNotifyStateCache);
    if ((*buffer == EnterNotify) || (*buffer == LeaveNotify))
      encodeBuffer.encodeValue((unsigned int) buffer[30], 2);
    else
      encodeBuffer.encodeBoolValue((unsigned int) buffer[30]);
    if ((*buffer == EnterNotify) || (*buffer == LeaveNotify))
      encodeBuffer.encodeValue((unsigned int) buffer[31], 2);
    else if (*buffer == KeyPress)
    {
      serverCache_ -> keyPressLastKey = detail;
      for (unsigned int i = 8; i < 31; i++)
      {
        serverCache_ -> keyPressCache[i - 8] = buffer[i];
      }
    }
  }

  //
  // Print info about achieved compression
  // and update the statistics.
  //

  int bits = encodeBuffer.diffBits();

  #if defined(TEST) || defined(OPCODES)
  *logofs << "handleMotion: Handled event OPCODE#" << (unsigned int) buffer[0]
          << " for FD#" << fd_ << " sequence " << sequenceNum << ". "
          << size << " bytes in, " << bits << " bits (" << ((float) bits) / 8
          << " bytes) out.\n" << logofs_flush;
  #endif

  statistics -> addEventBits(*buffer, size << 3, bits);

  #ifdef DEBUG
  *logofs << "handleMotion: Sent saved motion event for FD#"
          << fd_ << ".\n" << logofs_flush;
  #endif

  lastMotion_[0] = '\0';

  #ifdef DEBUG
  *logofs << "handleMotion: Reset last motion event for FD#"
          << fd_ << ".\n" << logofs_flush;
  #endif

  return 1;
}

int ServerChannel::handleConfiguration()
{
  #ifdef TEST
  *logofs << "ServerChannel: Setting new buffer parameters "
          << "for FD#" << fd_ << ".\n" << logofs_flush;
  #endif

  readBuffer_.setSize(control -> ServerInitialReadSize,
                          control -> ServerMaximumBufferSize);

  writeBuffer_.setSize(control -> TransportXBufferSize,
                           control -> TransportXBufferThreshold,
                               control -> TransportMaximumBufferSize);

  transport_ -> setSize(control -> TransportXBufferSize,
                            control -> TransportXBufferThreshold,
                                control -> TransportMaximumBufferSize);
  return 1;
}

int ServerChannel::handleFinish()
{
  #ifdef TEST
  *logofs << "ServerChannel: Finishing connection for FD#"
          << fd_ << ".\n" << logofs_flush;
  #endif

  congestion_ = 0;
  priority_   = 0;

  finish_ = 1;

  //
  // Reset the motion event.
  //

  lastMotion_[0] = '\0';

  transport_ -> fullReset();

  return 1;
}

int ServerChannel::handleAsyncEvents()
{
  //
  // Encode more events while decoding the
  // proxy messages.
  //

  if (transport_ -> readable() > 0)
  {
    #if defined(TEST) || defined(INFO)
    *logofs << "handleAsyncEvents: WARNING! Encoding events "
            << "for FD#" << fd_ << " at " << strMsTimestamp()
            << ".\n" << logofs_flush;
    #endif

    #if defined(TEST) || defined(INFO)

    T_timestamp startTs = getTimestamp();

    #endif

    if (proxy -> handleAsyncRead(fd_) < 0)
    {
      return -1;
    }

    #if defined(TEST) || defined(INFO)
    *logofs << "handleAsyncEvents: Spent " << diffTimestamp(startTs,
               getTimestamp()) << " Ms handling events for FD#"
            << fd_ << ".\n" << logofs_flush;
    #endif

    return 1;
  }

  return 0;
}

int ServerChannel::handleUnpack(unsigned char &opcode, unsigned char *&buffer,
                                    unsigned int &size)
{
  int resource = *(buffer + 1);

  #ifdef DEBUG
  *logofs << "handleUnpack: Unpacking image for resource " << resource
          << " with method " << (unsigned int) *(buffer + 12)
          << ".\n" << logofs_flush;
  #endif

  handleUnpackStateInit(resource);

  T_geometry *geometryState = unpackState_[resource] -> geometry;
  T_colormap *colormapState = unpackState_[resource] -> colormap;
  T_alpha    *alphaState    = unpackState_[resource] -> alpha;

  if (geometryState == NULL)
  {
    #ifdef PANIC
    *logofs << "handleUnpack: PANIC! Missing geometry unpacking "
            << "image for resource " << resource << ".\n"
            << logofs_flush;
    #endif

    cerr << "Error" << ": Missing geometry unpacking "
         << "image for resource " << resource << ".\n";

    return -1;
  }

  //
  // Get the image data from the buffer.
  //

  imageState_ -> drawable  = GetULONG(buffer + 4,  bigEndian_);
  imageState_ -> gcontext  = GetULONG(buffer + 8,  bigEndian_);

  imageState_ -> method    = *(buffer + 12);

  imageState_ -> format    = *(buffer + 13);
  imageState_ -> srcDepth  = *(buffer + 14);
  imageState_ -> dstDepth  = *(buffer + 15);

  imageState_ -> srcLength = GetULONG(buffer + 16,  bigEndian_);
  imageState_ -> dstLength = GetULONG(buffer + 20,  bigEndian_);

  imageState_ -> srcX      = GetUINT(buffer + 24, bigEndian_);
  imageState_ -> srcY      = GetUINT(buffer + 26, bigEndian_);
  imageState_ -> srcWidth  = GetUINT(buffer + 28, bigEndian_);
  imageState_ -> srcHeight = GetUINT(buffer + 30, bigEndian_);

  imageState_ -> dstX      = GetUINT(buffer + 32, bigEndian_);
  imageState_ -> dstY      = GetUINT(buffer + 34, bigEndian_);
  imageState_ -> dstWidth  = GetUINT(buffer + 36, bigEndian_);
  imageState_ -> dstHeight = GetUINT(buffer + 38, bigEndian_);

  #ifdef TEST
  *logofs << "handleUnpack: Source X is " << imageState_ -> srcX
          << " Y is " << imageState_ -> srcY << " width is "
          << imageState_ -> srcWidth << " height is "
          << imageState_ -> srcHeight << ".\n"
          << logofs_flush;
  #endif

  #ifdef TEST
  *logofs << "handleUnpack: Destination X is " << imageState_ -> dstX
          << " Y is " << imageState_ -> dstY << " width is "
          << imageState_ -> dstWidth << " height is "
          << imageState_ -> dstHeight << ".\n"
          << logofs_flush;
  #endif

  if (imageState_ -> srcX != 0 || imageState_ -> srcY != 0)
  {
    #ifdef PANIC
    *logofs << "handleUnpack: PANIC! Unsupported source coordinates "
            << "in unpack request.\n" << logofs_flush;
    #endif

    return -1;
  }
  else if (imageState_ -> method == PACK_COLORMAP_256_COLORS &&
               (colormapState == NULL || colormapState -> data == NULL))
  {
    #ifdef PANIC
    *logofs << "handleUnpack: PANIC! Cannot find any unpack colormap.\n"
            << logofs_flush;
    #endif

    return -1;
  }

  //
  // Field srcLength carries size of image data in
  // packed format. Field dstLength is size of the
  // image in the original X bitmap format.
  //

  unsigned int srcDataOffset = 40;

  unsigned int srcSize = imageState_ -> srcLength;

  unsigned int removeSize = size;

  unsigned char *srcData = buffer + srcDataOffset;

  //
  // Get source and destination bits per pixel.
  //

  int srcBitsPerPixel = MethodBitsPerPixel(imageState_ -> method);

  if (srcBitsPerPixel <= 0)
  {
    #ifdef PANIC
    *logofs << "handleUnpack: PANIC! Can't identify source "
            << "bits per pixel for method " << (unsigned int)
               imageState_ -> method << ".\n" << logofs_flush;
    #endif

    cerr << "Error" << ": Can't identify source bits "
         << "per pixel for method " << (unsigned int)
            imageState_ -> method << ".\n";

    writeBuffer_.removeMessage(removeSize);

    return -1;
  }

  #ifdef TEST
  *logofs << "handleUnpack: Source bits per pixel are "
          << srcBitsPerPixel << " source data size is "
          << srcSize << ".\n" << logofs_flush;
  #endif

  int dstBitsPerPixel = UnpackBitsPerPixel(geometryState, imageState_ -> dstDepth);

  if (dstBitsPerPixel <= 0)
  {
    #ifdef PANIC
    *logofs << "handleUnpack: PANIC! Can't identify "
            << "destination bits per pixel for depth "
            << (unsigned int) imageState_ -> dstDepth
            << ".\n" << logofs_flush;
    #endif

    cerr << "Error" << ": Can't identify "
         << "destination bits per pixel for depth "
         << (unsigned int) imageState_ -> dstDepth
         << ".\n";

    writeBuffer_.removeMessage(removeSize);

    return -1;
  }

  //
  // Destination is a PutImage request.
  //

  unsigned int dstDataOffset = 24;

  //
  // Output buffer size must match the number of input
  // pixels multiplied by the number of bytes per pixel
  // of current geometry.
  //

  size = (RoundUp4(imageState_ -> dstWidth * dstBitsPerPixel / 8) *
                       imageState_ -> dstHeight) + dstDataOffset;

  #ifdef TEST
  *logofs << "handleUnpack: Destination bits per pixel are "
          << dstBitsPerPixel << " destination data size is "
          << size - dstDataOffset << ".\n" << logofs_flush;
  #endif

  unsigned int dstSize = size - dstDataOffset;

  imageState_ -> dstLines = imageState_ -> dstHeight;

  unsigned char *dstData;

  //
  // Size of the final output buffer had to be stored
  // in the offset field of XImage/NXPackedImage.
  //

  #ifdef WARNING

  if (dstSize != imageState_ -> dstLength)
  {
    *logofs << "handleUnpack: WARNING! Destination size mismatch "
            << "with reported " << imageState_ -> dstLength
            << " and actual " << dstSize << ".\n"
            << logofs_flush;
  }

  #endif

  //
  // The decoding algorithm has put the packed image
  // in the plain write buffer. Let's use the scratch
  // buffer to uncompress the image.
  //

  buffer = writeBuffer_.addScratchMessage(size);

  dstData = buffer + dstDataOffset;

  //
  // Unpack image into the buffer.
  //

  *buffer = (unsigned char) X_PutImage;

  *(buffer + 1) = imageState_ -> format;

  PutUINT(size >> 2, buffer + 2, bigEndian_);

  PutULONG(imageState_ -> drawable, buffer + 4,  bigEndian_);
  PutULONG(imageState_ -> gcontext, buffer + 8,  bigEndian_);

  PutUINT(imageState_ -> dstWidth, buffer + 12, bigEndian_);
  PutUINT(imageState_ -> dstLines, buffer + 14, bigEndian_);

  PutUINT(imageState_ -> dstX, buffer + 16, bigEndian_);
  PutUINT(imageState_ -> dstY, buffer + 18, bigEndian_);

  *(buffer + 20) = 0;
  *(buffer + 21) = imageState_ -> dstDepth;

  #ifdef TEST
  *logofs << "handleUnpack: Write buffer size is "
          << writeBuffer_.getLength() << " scratch size is "
          << writeBuffer_.getScratchLength() << ".\n"
          << logofs_flush;
  #endif

  int result = 0;

  switch (imageState_ -> method)
  {
    case PACK_JPEG_8_COLORS:
    case PACK_JPEG_64_COLORS:
    case PACK_JPEG_256_COLORS:
    case PACK_JPEG_512_COLORS:
    case PACK_JPEG_4K_COLORS:
    case PACK_JPEG_32K_COLORS:
    case PACK_JPEG_64K_COLORS:
    case PACK_JPEG_256K_COLORS:
    case PACK_JPEG_2M_COLORS:
    case PACK_JPEG_16M_COLORS:
    {
      result = UnpackJpeg(geometryState, imageState_ -> method, srcData,
                              srcSize, dstBitsPerPixel, imageState_ -> dstWidth,
                                  imageState_ -> dstHeight, dstData, dstSize);
      break;
    }
    case PACK_PNG_8_COLORS:
    case PACK_PNG_64_COLORS:
    case PACK_PNG_256_COLORS:
    case PACK_PNG_512_COLORS:
    case PACK_PNG_4K_COLORS:
    case PACK_PNG_32K_COLORS:
    case PACK_PNG_64K_COLORS:
    case PACK_PNG_256K_COLORS:
    case PACK_PNG_2M_COLORS:
    case PACK_PNG_16M_COLORS:
    {
      result = UnpackPng(geometryState, imageState_ -> method, srcData,
                             srcSize, dstBitsPerPixel, imageState_ -> dstWidth,
                                 imageState_ -> dstHeight, dstData, dstSize);
      break;
    }
    case PACK_RGB_16M_COLORS:
    {
      result = UnpackRgb(geometryState, imageState_ -> method, srcData,
                             srcSize, dstBitsPerPixel, imageState_ -> dstWidth,
                                 imageState_ -> dstHeight, dstData, dstSize);
      break;
    }
    case PACK_RLE_16M_COLORS:
    {
      result = UnpackRle(geometryState, imageState_ -> method, srcData,
                             srcSize, dstBitsPerPixel, imageState_ -> dstWidth,
                                 imageState_ -> dstHeight, dstData, dstSize);
      break;
    }
    case PACK_BITMAP_16M_COLORS:
    {
      result = UnpackBitmap(geometryState, imageState_ -> method, srcData,
                                srcSize, dstBitsPerPixel, imageState_ -> dstWidth,
                                    imageState_ -> dstHeight, dstData, dstSize);
      break;
    }
    case PACK_COLORMAP_256_COLORS:
    {
      result = Unpack8(geometryState, colormapState, srcBitsPerPixel,
                           imageState_ -> srcWidth, imageState_ -> srcHeight, srcData,
                               srcSize, dstBitsPerPixel, imageState_ -> dstWidth,
                                   imageState_ -> dstHeight, dstData, dstSize);

      break;
    }
    default:
    {
      const T_colormask *colorMask = MethodColorMask(imageState_ -> method);

      switch (imageState_ -> method)
      {
        case PACK_MASKED_8_COLORS:
        case PACK_MASKED_64_COLORS:
        case PACK_MASKED_256_COLORS:
        {
          result = Unpack8(geometryState, colorMask, imageState_ -> srcDepth,
                               imageState_ -> srcWidth, imageState_ -> srcHeight,
                                   srcData, srcSize, imageState_ -> dstDepth,
                                       imageState_ -> dstWidth, imageState_ -> dstHeight,
                                           dstData, dstSize);
          break;
        }
        case PACK_MASKED_512_COLORS:
        case PACK_MASKED_4K_COLORS:
        case PACK_MASKED_32K_COLORS:
        case PACK_MASKED_64K_COLORS:
        {
          result = Unpack16(geometryState, colorMask, imageState_ -> srcDepth,
                                imageState_ -> srcWidth, imageState_ -> srcHeight,
                                    srcData, srcSize, imageState_ -> dstDepth,
                                        imageState_ -> dstWidth, imageState_ -> dstHeight,
                                            dstData, dstSize);
          break;
        }
        case PACK_MASKED_256K_COLORS:
        case PACK_MASKED_2M_COLORS:
        case PACK_MASKED_16M_COLORS:
        {
          result = Unpack24(geometryState, colorMask, imageState_ -> srcDepth,
                                imageState_ -> srcWidth, imageState_ -> srcHeight,
                                    srcData, srcSize, imageState_ -> dstDepth,
                                        imageState_ -> dstWidth, imageState_ -> dstHeight,
                                            dstData, dstSize);
          break;
        }
        default:
        {
          break;
        }
      }
    }
  }

  writeBuffer_.removeMessage(removeSize);

  if (result <= 0)
  {
    #ifdef PANIC
    *logofs << "handleUnpack: PANIC! Failed to unpack image "
            << "with method '" << (unsigned int) imageState_ -> method
            << "'.\n" << logofs_flush;
    #endif

    cerr << "Warning" << ": Failed to unpack image "
         << "with method '" << (unsigned int) imageState_ -> method
         << "'.\n";

    //
    // TODO: We should mark the image somehow,
    // and force the remote to remove it from
    // the cache.
    //

    writeBuffer_.removeScratchMessage();

    return -1;
  }

  //
  // Alpha channel is used only on some 32 bits pixmaps
  // and only if render extension is in use. Presently
  // we don't have an efficient way to know in advance
  // if mask must be applied or not to the image. If an
  // alpha channel is set, the function will check if
  // the size of the alpha data matches the size of the
  // image. In the worst case we'll create an useless
  // alpha plane for a pixmap that doesn't need it.
  //

  if (alphaState != NULL && alphaState -> data != NULL &&
          imageState_ -> dstDepth == 32)
  {
    UnpackAlpha(alphaState, dstData, dstSize, imageByteOrder_);
  }

  return 1;
}

int ServerChannel::handleAuthorization(unsigned char *buffer)
{
  //
  // At the present moment we don't support more than
  // a single display for each proxy, so authorization
  // data is shared among all the channels.
  //
  // Use the following code to simulate authentication
  // failures on a LSB machine:
  // 
  // memcpy(buffer + 12 + (((buffer[6] + 256 *
  //            buffer[7]) + 3) & ~3), "1234567890123456", 16);
  //

  if (auth == NULL)
  {
    #if defined(TEST) || defined(INFO)
    *logofs << "handleAuthorization: Forwarding the real cookie "
            << "for FD#" << fd_ << ".\n" << logofs_flush;
    #endif

    return 0;
  }
  else if (auth -> checkCookie(buffer) == 1)
  {
    #if defined(TEST) || defined(INFO)
    *logofs << "handleAuthorization: Matched the fake cookie "
            << "for FD#" << fd_ << ".\n" << logofs_flush;
    #endif

    return 1;
  }
  else
  {
    #if defined(TEST) || defined(INFO)
    *logofs << "handleAuthorization: WARNING! Failed to match "
            << "the fake cookie for FD#" << fd_ << ".\n"
            << logofs_flush;
    #endif

    return -1;
  }
}

int ServerChannel::handleAuthorization(const unsigned char *buffer, int size)
{
  //
  // Check the X server's response and, in the case of
  // an error, print the textual information reported
  // by the server.
  //

  if (*buffer != 1)
  {
    const char *reason = NULL;

    //
    // At the moment we don't take into account the end-
    // ianess of the reply. This should work in any case
    // because we simply try to match a few well-known
    // error strings.
    //

    if (size >= INVALID_COOKIE_SIZE + 8 &&
            memcmp(buffer + 8, INVALID_COOKIE_DATA,
                       INVALID_COOKIE_SIZE) == 0)
    {
      reason = INVALID_COOKIE_DATA;
    }
    else if (size >= NO_AUTH_PROTO_SIZE + 8 &&
                 memcmp(buffer + 8, NO_AUTH_PROTO_DATA,
                            NO_AUTH_PROTO_SIZE) == 0)
    {
      reason = NO_AUTH_PROTO_DATA;
    }
    else
    {
      reason = "Unknown";
    }

    #ifdef WARNING
    *logofs << "handleAuthorization: WARNING! X connection failed "
            << "with error '" << reason << "' on FD#" << fd_
            << ".\n" << logofs_flush;
    #endif

    cerr << "Warning" << ": X connection failed "
         << "with error '" << reason << "'.\n";
  }
  #if defined(TEST) || defined(INFO)
  else
  {
    *logofs << "handleAuthorization: X connection successful "
            << "on FD#" << fd_ << ".\n" << logofs_flush;
  }
  #endif

  return 1;
}

//
// Use a simple encoding. Need to handle the image
// requests in the usual way and the X_ListExtensions
// and X_QueryExtension to hide MIT-SHM and RENDER
// in the reply.
//

int ServerChannel::handleFastWriteRequest(DecodeBuffer &decodeBuffer, unsigned char &opcode,
                                              unsigned char *&buffer, unsigned int &size)
{
  //
  // All the NX requests are handled in the
  // main message loop. The X_PutImage can
  // be handled here only if a split was
  // not requested.
  //

  if ((opcode >= X_NXFirstOpcode && opcode <= X_NXLastOpcode) ||
          (control -> isProtoStep7() == 1 && opcode == X_PutImage &&
               splitState_.resource != nothing) || opcode == X_ListExtensions ||
                   opcode == X_QueryExtension)
  {
    return 0;
  }

  #ifdef DEBUG
  *logofs << "handleFastWriteRequest: Decoding raw request OPCODE#"
          << (unsigned int) opcode << " for FD#" << fd_
          << ".\n" << logofs_flush;
  #endif

  buffer = writeBuffer_.addMessage(4);

  #ifndef __sun

  unsigned int *next = (unsigned int *) decodeBuffer.decodeMemory(4);

  *((unsigned int *) buffer) = *next;

  #else /* #ifndef __sun */

  memcpy(buffer, decodeBuffer.decodeMemory(4), 4);

  #endif /* #ifndef __sun */

  size = GetUINT(buffer + 2, bigEndian_) << 2;

  if (size < 4)
  {
    #ifdef WARNING
    *logofs << "handleFastWriteRequest: WARNING! Assuming size 4 "
            << "for suspicious message of size " << size
            << ".\n" << logofs_flush;
    #endif

    size = 4;
  }

  writeBuffer_.registerPointer(&buffer);

  if (writeBuffer_.getAvailable() < size - 4 ||
          (int) size >= control -> TransportFlushBufferSize)
  {
    #ifdef DEBUG
    *logofs << "handleFastWriteRequest: Using scratch buffer for OPCODE#"
            << (unsigned int) opcode << " with size " << size << " and "
            << writeBuffer_.getLength() << " bytes in buffer.\n"
            << logofs_flush;
    #endif

    //
    // The procedure moving data to shared memory
    // assumes that the full message is stored in
    // the scratch buffer. We can safely let the
    // scratch buffer inherit the decode buffer
    // at the next offset.
    //

    writeBuffer_.removeMessage(4);

    buffer = writeBuffer_.addScratchMessage(((unsigned char *)
                             decodeBuffer.decodeMemory(size - 4)) - 4, size);
  }
  else
  {
    writeBuffer_.addMessage(size - 4);

    #ifndef __sun

    if (size <= 32)
    {
      next = (unsigned int *) decodeBuffer.decodeMemory(size - 4);

      for (unsigned int i = 4; i < size; i += sizeof(unsigned int))
      {
        *((unsigned int *) (buffer + i)) = *next++;
      }
    }
    else
    {
      memcpy(buffer + 4, decodeBuffer.decodeMemory(size - 4), size - 4);
    }

    #else /* #ifndef __sun */

    memcpy(buffer + 4, decodeBuffer.decodeMemory(size - 4), size - 4);

    #endif /* #ifndef __sun */
  }

  //
  // Opcode could have been tainted by the client
  // proxy. Replace the original opcode with the
  // one sent in the decode buffer.
  //

  *buffer = opcode;

  writeBuffer_.unregisterPointer();

  if (opcode == X_PutImage)
  {
    handleImage(opcode, buffer, size);
  }

  #if defined(TEST) || defined(OPCODES)

  if (opcode != 0)
  {
    *logofs << "handleFastWriteRequest: Handled request "
            << "OPCODE#" << (unsigned int) opcode << " ("
            << DumpOpcode(opcode) << ") for FD#" << fd_
            << " sequence " << clientSequence_ << ". "
            << size << " bytes out.\n" << logofs_flush;
  }
  else
  {
    *logofs << "handleFastWriteRequest: Handled image or "
            << "other request for FD#" << fd_
            << " sequence " << clientSequence_ << ". "
            << size << " bytes out.\n" << logofs_flush;
  }

  #endif

  handleFlush(flush_if_needed);

  return 1;
}

//
// Use the simplest encoding except for replies that
// need to be managed some way.
//

int ServerChannel::handleFastReadReply(EncodeBuffer &encodeBuffer, const unsigned char &opcode,
                                           const unsigned char *&buffer, const unsigned int &size)
{
  //
  // If we pushed a X_GetInputFocus in the sequence
  // queue this means that the original message was
  // a NX request for which we have to provide a NX
  // reply.
  //

  if ((opcode >= X_NXFirstOpcode &&
           opcode <= X_NXLastOpcode) ||
               opcode == X_QueryExtension ||
                   opcode == X_ListExtensions ||
                       opcode == X_GetInputFocus)
  {
    return 0;
  }

  #ifdef DEBUG
  *logofs << "handleFastReadReply: Encoding raw reply OPCODE#"
          << (unsigned int) opcode << " for FD#" << fd_
          << " with size " << size << ".\n"
          << logofs_flush;
  #endif

  encodeBuffer.encodeMemory(buffer, size);

  //
  // Send back the reply as soon
  // as possible.
  //

  priority_++;

  int bits = encodeBuffer.diffBits();

  #if defined(TEST) || defined(OPCODES)
  *logofs << "handleFastReadReply: Handled raw reply OPCODE#"
          << (unsigned int) opcode << " for FD#" << fd_ << " sequence "
          << serverSequence_ << ". " << size << " bytes in, "
          << bits << " bits (" << ((float) bits) / 8
          << " bytes) out.\n" << logofs_flush;
  #endif

  statistics -> addReplyBits(opcode, size << 3, bits);

  return 1;
}

int ServerChannel::handleFastReadEvent(EncodeBuffer &encodeBuffer, const unsigned char &opcode,
                                           const unsigned char *&buffer, const unsigned int &size)
{
  #ifdef DEBUG
  *logofs << "handleFastReadEvent: Encoding raw "
          << (opcode == X_Error ? "error" : "event") << " OPCODE#"
          << (unsigned int) opcode << " for FD#" << fd_
          << " with size " << size << ".\n"
          << logofs_flush;
  #endif

  encodeBuffer.encodeMemory(buffer, size);

  switch (opcode)
  {
    case X_Error:
    case ButtonPress:
    case ButtonRelease:
    case KeyPress:
    case KeyRelease:
    {
      priority_++;
    }
  }

  int bits = encodeBuffer.diffBits();

  #if defined(TEST) || defined(OPCODES)

  if (opcode == X_Error)
  {
    unsigned char code = *(buffer + 1);

    *logofs << "handleFastReadEvent: Handled error ERR_CODE#"
            << (unsigned int) code << " for FD#" << fd_;

    *logofs << " RES_ID#" << GetULONG(buffer + 4, bigEndian_);

    *logofs << " MIN_OP#" << GetUINT(buffer + 8, bigEndian_);

    *logofs << " MAJ_OP#" << (unsigned int) *(buffer + 10);

    *logofs << " sequence " << serverSequence_ << ". " << size
            << " bytes in, " << bits << " bits (" << ((float) bits) / 8
            << " bytes) out.\n" << logofs_flush;
  }
  else
  {
    *logofs << "handleFastReadEvent: Handled event OPCODE#"
            << (unsigned int) *buffer << " for FD#" << fd_
            << " sequence " << serverSequence_ << ". " << size
            << " bytes in, " << bits << " bits (" << ((float) bits) / 8
            << " bytes) out.\n" << logofs_flush;
  }

  #endif

  statistics -> addEventBits(opcode, size << 3, bits);

  return 1;
}

void ServerChannel::initCommitQueue()
{
  #ifdef TEST
  *logofs << "initCommitQueue: Resetting the queue of split commits "
          << "for FD#" << fd_ << ".\n" << logofs_flush;
  #endif

  for (int i = 0; i < MAX_COMMIT_SEQUENCE_QUEUE; i++)
  {
    commitSequenceQueue_[i] = 0;
  }
}

void ServerChannel::updateCommitQueue(unsigned short sequence)
{
  for (int i = 0; i < MAX_COMMIT_SEQUENCE_QUEUE - 1; i++)
  {
    commitSequenceQueue_[i + 1] = commitSequenceQueue_[i];
  }

  #ifdef TEST
  *logofs << "updateCommitQueue: Saved " << sequence
          << " as last sequence number of image to commit.\n"
          << logofs_flush;
  #endif

  commitSequenceQueue_[0] = sequence;
}

int ServerChannel::checkCommitError(unsigned char error, unsigned short sequence,
                                        const unsigned char *buffer)
{
  //
  // Check if error is due to an image commit
  // generated at the end of a split.
  //
  // TODO: It should zero the head of the list
  // when an event comes with a sequence number
  // greater than the value of the last element
  // added.
  //

  for (int i = 0; i < MAX_COMMIT_SEQUENCE_QUEUE &&
           commitSequenceQueue_[i] != 0; i++)
  {
    #ifdef TEST
    *logofs << "checkCommitError: Checking committed image's "
            << "sequence number " << commitSequenceQueue_[i]
            << " with input sequence " << sequence << ".\n"
            << logofs_flush;
    #endif

    if (commitSequenceQueue_[i] == sequence)
    {
      #ifdef WARNING

      *logofs << "checkCommitError: WARNING! Failed operation for "
              << "FD#" << fd_ << " with ERR_CODE#"
              << (unsigned int) *(buffer + 1);

      *logofs << " RES_ID#" << GetULONG(buffer + 4, bigEndian_);

      *logofs << " MIN_OP#" << GetUINT(buffer + 8, bigEndian_);

      *logofs << " MAJ_OP#" << (unsigned int) *(buffer + 10);

      *logofs << " sequence " << sequence << ".\n";

      *logofs << logofs_flush;

      #endif

      cerr << "Warning" << ": Failed commit operation "
           << "with ERR_CODE#" << (unsigned int) error;

      cerr << " RES_ID#" << GetULONG(buffer + 4, bigEndian_);

      cerr << " MIN_OP#" << GetUINT(buffer + 8, bigEndian_);

      cerr << " MAJ_OP#" << (unsigned int) *(buffer + 10);

      cerr << ".\n";

      #ifdef WARNING
      *logofs << "checkCommitError: WARNING! Suppressing error on "
              << "OPCODE#" << (unsigned int) opcodeStore_ -> commitSplit
              << " for FD#" << fd_ << " with sequence " << sequence
              << " at position " << i << ".\n" << logofs_flush;
      #endif

      return 0;
    }
  }

  return 0;
}

//
// Check if the user pressed the CTRL+ALT+SHIFT+ESC
// keystroke. At the present moment it uses different
// keycodes based on the client OS. This  should be
// implemented in a way that is platform independent
// (that's not an easy task, considered that we don't
// have access to the higher level X libraries).
//

int ServerChannel::checkKeyboardEvent(unsigned char event, unsigned short sequence,
                                          const unsigned char *buffer)
{
  #ifdef TEST
  *logofs << "checkKeyboardEvent: Checking escape sequence with byte [1] "
          << (void *) ((unsigned) *(buffer + 1)) << " and bytes [28-29] "
          << (void *) ((unsigned) GetUINT(buffer + 28, bigEndian_))
          << " for FD#" << fd_ << ".\n" << logofs_flush;
  #endif

  #ifdef __APPLE__

  int alert = (*(buffer + 1) == 0x3d &&
                   GetUINT(buffer + 28, bigEndian_) == 0x2005);

  #else

  int alert = (*(buffer + 1) == 0x09 &&
                   ((GetUINT(buffer + 28, bigEndian_) &
                        0x0d) == 0x0d));

  #endif

  if (alert == 1)
  {
    #ifdef PANIC
    *logofs << "checkKeyboardEvent: PANIC! Received sequence "
            << "CTRL+ALT+SHIFT+ESC " << "for FD#"<< fd_
            << ". Showing the abort dialog.\n" << logofs_flush;
    #endif

    cerr << "Warning" << ": Received sequence CTRL+ALT+SHIFT+ESC. "
         << "Showing the abort dialog.\n";

    HandleAlert(CLOSE_UNRESPONSIVE_X_SERVER_ALERT, 1);
  }

  return alert;
}

//
// Handle the MIT-SHM initialization
// messages exchanged with the remote
// proxy. 
//

int ServerChannel::handleShmemReply(EncodeBuffer &encodeBuffer, const unsigned char opcode,
                                        const unsigned int stage, const unsigned char *buffer,
                                            const unsigned int size)
{
  #ifdef TEST
  *logofs << "handleShmemReply: Returning shmem reply for "
          << "stage " << stage << ".\n" << logofs_flush;
  #endif

  if (opcode == X_QueryExtension)
  {
    encodeBuffer.encodeValue(stage, 2);

#ifndef ANDROID
    shmemState_ -> present = *(buffer + 8);
#else
    shmemState_ -> present = 0;
    cerr << "Info: handleShmemReply: In android no shared memory. Setting present to 0 hardcoded\n";
#endif
    shmemState_ -> opcode  = *(buffer + 9);
    shmemState_ -> event   = *(buffer + 10);
    shmemState_ -> error   = *(buffer + 11);

    #ifdef TEST
    *logofs << "handleShmemReply: Extension present is "
            << shmemState_ -> present << " with base OPCODE#"
            << (unsigned int) shmemState_ -> opcode << " base event "
            << (unsigned int) shmemState_ -> event << " base error "
            << (unsigned int) shmemState_ -> error << ".\n"
            << logofs_flush;
    #endif
  }
  else if (opcode == X_GetInputFocus)
  {
    encodeBuffer.encodeValue(stage, 2);

    encodeBuffer.encodeBoolValue(0);

    if (shmemState_ -> present == 1 &&
            shmemState_ -> address != NULL &&
                shmemState_ -> segment > 0 &&
                    shmemState_ -> id > 0)
    {
      cerr << "Info" << ": Using shared memory parameters 1/"
           << (shmemState_ -> size / 1024) << "K.\n";

#ifndef ANDROID
      shmemState_ -> enabled = 1;
#else
      cerr << "Info: handleShmemReply: In android no shared memory. Setting enabled to -1. This should not be displayed\n";
      shmemState_ -> enabled = -1;
#endif

      encodeBuffer.encodeBoolValue(1);
    }
    else
    {
      #ifdef TEST
      *logofs << "handleShmemReply: WARNING! Not using shared memory "
              << "support in X server for FD#" << fd_ << ".\n"
              << logofs_flush;
      #endif

      cerr << "Info" << ": Using shared memory parameters 0/0K.\n";

      handleShmemStateRemove();

      encodeBuffer.encodeBoolValue(0);
    }
  }
  else
  {
    #ifdef PANIC
    *logofs << "handleShmemReply: PANIC! Conversation error "
            << "handling shared memory support for FD#"
            << fd_ << ".\n" << logofs_flush;
    #endif

    cerr << "Error" << ": Conversation error handling "
         << "shared memory support.\n";

    return -1;
  }

  return 1;
}

int ServerChannel::handleShmemRequest(DecodeBuffer &decodeBuffer, unsigned char &opcode,
                                          unsigned char *&buffer, unsigned int &size)
{
  //
  // We need to query and initialize MIT-SHM on
  // the real X server. To do this we'll need 3
  // requests. At the end we'll have to encode
  // the final reply for the X client side.
  //

  handleShmemStateAlloc();

  unsigned int stage;

  decodeBuffer.decodeValue(stage, 2);

  unsigned int expected = shmemState_ -> stage + 1;

  if (stage != expected || stage > 2)
  {
    #ifdef PANIC
    *logofs << "handleShmemRequest: PANIC! Unexpected stage "
            << stage << " in handling shared memory "
            << "support for FD#" << fd_ << ".\n"
            << logofs_flush;
    #endif

    cerr << "Error" << ": Unexpected stage "
         << stage << " in handling shared memory "
         << "support for FD#" << fd_ << ".\n";

    return -1;
  }

  switch (stage)
  {
    case 0:
    {
      unsigned int enableClient;
      unsigned int enableServer;

      decodeBuffer.decodeBoolValue(enableClient);
      decodeBuffer.decodeBoolValue(enableServer);

      unsigned int clientSegment;
      unsigned int serverSegment;

      decodeBuffer.decodeValue(clientSegment, 29, 9);
      decodeBuffer.decodeValue(serverSegment, 29, 9);

      shmemState_ -> segment = serverSegment;

      #ifdef TEST
      *logofs << "handleShmemRequest: Size of the shared memory "
              << "segment will be " << control -> ShmemServerSize
              << ".\n" << logofs_flush;
      #endif

      #ifdef TEST
      *logofs << "handleShmemRequest: Sending X_QueryExtension request "
              << "for FD#" << fd_ << " due to OPCODE#" << (unsigned int)
                 opcodeStore_ -> getShmemParameters << " in stage "
              << stage << ".\n" << logofs_flush;
      #endif

      opcode = X_QueryExtension;

      size   = 16;
      buffer = writeBuffer_.addMessage(size);

      PutUINT(7, buffer + 4, bigEndian_);

      //
      // Simply make the query fail if shared
      // memory support is disabled by the
      // user.
      //
#ifndef ANDROID
      if (control -> ShmemServer == 1 &&
              control -> ShmemServerSize > 0 &&
                  enableServer == 1)
      {
        memcpy(buffer + 8, "MIT-SHM", 7);
      }
      else
      {
        memcpy(buffer + 8, "NO-MIT-", 7);
      }
#else
      cerr << "Info: handleShmemRequest: In android no shared memory. Returning NO-MIT- answer\n";

        memcpy(buffer + 8, "NO-MIT-", 7);
#endif
     sequenceQueue_.push(clientSequence_, opcode,
                              opcodeStore_ -> getShmemParameters, stage);

      //
      // Save the sequence number so we can
      // later identify any matching X error
      // received from server.
      //

      shmemState_ -> sequence = clientSequence_;

      break;
    }
    case 1:
    {
      if (shmemState_ -> present == 1)
      {
        //
        // Make the segment read-write for everybody on
        // Cygwin (to avoid any lack of support or any
        // performance issue) and on MacOS/X (where the
        // 0600 mask doesn't seem to work).
        //

        #if defined(__CYGWIN32__) || defined(__APPLE__)

        int permissions = 0777;

        #else

        int permissions = 0600;

        #endif

        shmemState_ -> size = control -> ShmemServerSize;

#ifndef ANDROID
        shmemState_ -> id = shmget(IPC_PRIVATE, shmemState_ -> size,
                                       IPC_CREAT | permissions);
#else
	cerr << "Info: handleShmemReqyest: In android no shared memory (shmget). This message should not be displayed present should never be 1 in android\n";
	shmemState_ -> id = -1;
#endif
        if (shmemState_ -> id >= 0)
        {
          #if defined(TEST) || defined(INFO)
          *logofs << "handleShmemRequest: Allocated shared memory "
                  << "segment of " << shmemState_ -> size
                  << " bytes with id " << shmemState_ -> id
                  << ".\n" << logofs_flush;
          #endif


#ifndef ANDROID
          shmemState_ -> address = shmat(shmemState_ -> id, 0, 0);
#else
	  cerr << "Info: handleShmemReqyest: In android no shared memory (shmat). This message should not be displayed. present should never be 1 in android\n";
	  shmemState_ -> address = NULL;
#endif
          if (shmemState_ -> address != NULL)
          {
            #ifdef TEST
            *logofs << "handleShmemRequest: Sending X_ShmAttach request "
                    << "for FD#" << fd_ << " due to OPCODE#" << (unsigned int)
                       opcodeStore_ -> getShmemParameters << " in stage "
                    << stage << ".\n" << logofs_flush;
            #endif

            opcode = shmemState_ -> opcode;

            size   = 16;
            buffer = writeBuffer_.addMessage(size);

            *(buffer + 1) = X_ShmAttach;

            PutULONG(shmemState_ -> segment, buffer + 4, bigEndian_);
            PutULONG(shmemState_ -> id, buffer + 8, bigEndian_);

            *(buffer + 12) = 1;

            shmemState_ -> sequence = clientSequence_;

            break;
          }
          else
          {
            #ifdef WARNING
            *logofs << "handleShmemRequest: WARNING! Can't attach the shared "
                    << "memory segment. Error is " << EGET() << " '"
                    << ESTR() << "'.\n" << logofs_flush;
            #endif

            cerr << "Warning" << ": Can't attach the shared memory "
                 << "segment. Error is " << EGET() << " '"
                 << ESTR() << "'.\n";
          }
        }
        else
        {
          #ifndef __CYGWIN32__

          #ifdef WARNING
          *logofs << "handleShmemRequest: WARNING! Can't create the shared "
                  << "memory segment. Error is " << EGET() << " '"
                  << ESTR() << "'.\n" << logofs_flush;
          #endif

          cerr << "Warning" << ": Can't create the shared memory "
               << "segment. Error is " << EGET() << " '"
               << ESTR() << "'.\n";

          #else

          #ifdef TEST
          *logofs << "handleShmemRequest: WARNING! Can't create the shared "
                  << "memory segment. Error is " << EGET() << " '"
                  << ESTR() << "'.\n" << logofs_flush;
          #endif

          #endif
        }
      }

      if (shmemState_ -> present != 0)
      {
        #ifdef TEST
        *logofs << "handleShmemRequest: Resetting shared memory "
                << "presence flag for FD#" << fd_ << ".\n"
                << logofs_flush;
        #endif

        shmemState_ -> present = 0;
      }

      handleNullRequest(opcode, buffer, size);

      break;
    }
    default:
    {
      #ifdef TEST
      *logofs << "handleShmemRequest: Sending X_GetInputFocus request "
              << "for FD#" << fd_ << " due to OPCODE#" << (unsigned int)
                 opcodeStore_ -> getShmemParameters << " in stage "
              << stage << ".\n" << logofs_flush;
      #endif

      opcode = X_GetInputFocus;

      size   = 4;
      buffer = writeBuffer_.addMessage(size);

      sequenceQueue_.push(clientSequence_, opcode,
                              opcodeStore_ -> getShmemParameters, stage);
      break;
    }
  }

  shmemState_ -> stage += 1;

  return 1;
}

//
// Handling of MIT-SHM extension has been plugged late in
// the design, so we have to make some assumptions. Image
// is a X_PutImage request contained either in the scratch
// buffer or in the normal write buffer. We need to move
// the image data to the shared memory segment and replace
// the X_PutImage request with a X_ShmPutImage.
//

int ServerChannel::handleShmem(unsigned char &opcode, unsigned char *&buffer,
                                   unsigned int &size)
{
  if (shmemState_ == NULL || shmemState_ -> enabled != 1)
  {
    #ifdef TEST

    if (shmemState_ != NULL)
    {
      *logofs << "handleShmem: PANIC! Shared memory "
              << "state found but support is not enabled "
              << "for FD#" << fd_ << " in stage "
              << shmemState_ -> stage << ".\n"
              << logofs_flush;
    }

    #endif

    return 0;
  }
#ifdef ANDROID
  cerr << "Info: handleShmem: In android no shared memory.  enabled should never be 1. This should not be displayed\n";
  return 0;
#endif

  //
  // Ignore null requests and requests that will not result
  // in a single X_PutImage. To conform with the other func-
  // tions, we get the opcode passed as a parameter. It can
  // be zero if we don't want the write loop to put opcode
  // and length in the resulting buffer. Anyway we are only
  // interested in the original opcode of the request, that
  // is stored in the image state.
  //

  unsigned char *dstData    = buffer + 24;
  unsigned int  dstDataSize = size   - 24;

  if (dstDataSize == 0 || dstDataSize >
          (unsigned int) control -> MaximumRequestSize)
  {
    #ifdef TEST
    *logofs << "handleShmem: Ignoring image with opcode "
            << (unsigned int) imageState_ -> opcode
            << " and size " << size << " for FD#" << fd_
            << ".\n" << logofs_flush;
    #endif

    return 0;
  }

  #ifdef TEST
  *logofs << "handleShmem: Handling image with opcode "
          << (unsigned int) imageState_ -> opcode
          << " and size " << size << " for FD#" << fd_
          << ".\n" << logofs_flush;
  #endif

  //
  // Get image data from buffer.
  //

  if (imageState_ -> opcode == X_PutImage)
  {
    //
    // We still need to get the image's data.
    //

    imageState_ -> format    = *(buffer + 1);

    imageState_ -> drawable  = GetULONG(buffer + 4,  bigEndian_);
    imageState_ -> gcontext  = GetULONG(buffer + 8,  bigEndian_);

    imageState_ -> dstWidth  = GetUINT(buffer + 12, bigEndian_);
    imageState_ -> dstHeight = GetUINT(buffer + 14, bigEndian_);

    imageState_ -> srcX      = 0;
    imageState_ -> srcY      = 0;

    imageState_ -> srcWidth  = imageState_ -> dstWidth;
    imageState_ -> srcHeight = imageState_ -> dstHeight;

    imageState_ -> dstX      = GetUINT(buffer + 16, bigEndian_);
    imageState_ -> dstY      = GetUINT(buffer + 18, bigEndian_);

    imageState_ -> leftPad   = *(buffer + 20);
    imageState_ -> dstDepth  = *(buffer + 21);

    imageState_ -> dstLines  = imageState_ -> dstHeight;

    imageState_ -> dstLength = size - 24;
  }

  //
  // Skip the MIT-SHM operation if the image
  // is 1 bits-per-plane.
  //

  if (imageState_ -> dstDepth == 1)
  {
    #if defined(TEST) || defined(INFO)
    *logofs << "handleShmem: Ignoring image with opcode "
            << (unsigned int) imageState_ -> opcode << " depth "
            << (unsigned int) imageState_ -> dstDepth << " and "
            << "size " << size << " for FD#" << fd_
            << ".\n" << logofs_flush;
    #endif

    return 0;
  }

  //
  // If the image can't fit in the available
  // space, check if the completion event is
  // arrived.
  //

  #if defined(TEST) || defined(INFO)

  if (isTimestamp(shmemState_ -> last) == 0 &&
          shmemState_ -> offset != 0)
  {
    *logofs << "handleShmem: PANIC! No timestamp for sequence "
            << shmemState_ -> sequence << " with offset "
            << shmemState_ -> offset << ".\n"
            << logofs_flush;
  }

  #endif

  if (shmemState_ -> offset + imageState_ -> dstLength >
          shmemState_ -> size)
  {
    if (imageState_ -> dstLength > shmemState_ -> size)
    {
      #if defined(TEST) || defined(INFO)
      *logofs << "handleShmem: WARNING! Can't fit the image "
              << "in the available memory segment for FD#"
              << fd_ << ".\n" << logofs_flush;
      #endif

      return 0;
    }
    else if (handleShmemEvent() <= 0)
    {
      #if defined(TEST) || defined(INFO)
      *logofs << "handleShmem: WARNING! Missing completion "
              << "after " << diffTimestamp(shmemState_ -> last,
                 getTimestamp()) << " Ms for shared memory "
              << "for FD#" << fd_ << ".\n" << logofs_flush;
      #endif

      return 0;
    }
  }

  //
  // Let image start at current offset
  // in the shared segment.
  //

  #ifdef TEST
  *logofs << "handleShmem: Copying " << dstDataSize
          << " bytes to shared memory at offset "
          << shmemState_ -> offset << " for FD#"
          << fd_ << ".\n" << logofs_flush;
  #endif

  memcpy((unsigned char *) shmemState_ -> address +
             shmemState_ -> offset, dstData, dstDataSize);

  //
  // Get rid of the original X_PutImage
  // request.
  //

  if (writeBuffer_.getScratchData() != NULL)
  {
    writeBuffer_.removeScratchMessage();
  }
  else
  {
    writeBuffer_.removeMessage(size);
  }

  //
  // Add a X_ShmPutImage request to the
  // write buffer.
  //

  buffer = writeBuffer_.addMessage(40);

  *buffer = shmemState_ -> opcode;

  *(buffer + 1) = X_ShmPutImage;

  PutUINT(40 >> 2, buffer + 2, bigEndian_);

  PutULONG(imageState_ -> drawable, buffer + 4,  bigEndian_);
  PutULONG(imageState_ -> gcontext, buffer + 8,  bigEndian_);

  PutUINT(imageState_ -> dstWidth, buffer + 12, bigEndian_);
  PutUINT(imageState_ -> dstLines, buffer + 14, bigEndian_);

  PutUINT(imageState_ -> srcX, buffer + 16, bigEndian_);
  PutUINT(imageState_ -> srcY, buffer + 18, bigEndian_);

  PutUINT(imageState_ -> dstWidth,  buffer + 20, bigEndian_);
  PutUINT(imageState_ -> dstLines,  buffer + 22, bigEndian_);

  PutUINT(imageState_ -> dstX, buffer + 24, bigEndian_);
  PutUINT(imageState_ -> dstY, buffer + 26, bigEndian_);

  *(buffer + 28) = imageState_ -> dstDepth;
  *(buffer + 29) = imageState_ -> format;
  *(buffer + 30) = 1;

  PutULONG(shmemState_ -> segment, buffer + 32, bigEndian_);
  PutULONG(shmemState_ -> offset,  buffer + 36, bigEndian_);

  shmemState_ -> offset += dstDataSize;

  shmemState_ -> sequence = clientSequence_;
  shmemState_ -> last     = getTimestamp();

  #ifdef TEST
  *logofs << "handleShmem: Saved shared memory sequence "
          << shmemState_ -> sequence << " for FD#" << fd_
          << " with offset " << shmemState_ -> offset
          << " at " << strMsTimestamp() << ".\n"
          << logofs_flush;
  #endif

  //
  // Make the X server read immediately
  // from the shared memory buffer and
  // produce the completion event.
  //

  handleFlush(flush_if_any);

  return 1;
}

//
// Try to read more events from the socket in the
// attempt to get the completion event required
// to reset the MIT-SHM segment.
//

int ServerChannel::handleShmemEvent()
{
  #if defined(TEST) || defined(INFO)
  *logofs << "handleShmemEvent: Waiting for shared memory "
          << "sequence " << shmemState_ -> sequence
          << " for X server FD#" << fd_ << ".\n"
          << logofs_flush;

  T_timestamp startTs = getTimestamp();

  #endif

  while (isTimestamp(shmemState_ -> last) != 0)
  {
    if (handleWait(control -> ShmemTimeout) <= 0)
    {
      break;
    }
    #if defined(TEST) || defined(INFO)
    else
    {
      *logofs << "handleShmemEvent: WARNING! Encoded events "
              << "for FD#" << fd_ << " at " << strMsTimestamp()
              << ".\n" << logofs_flush;
    }
    #endif
  }

  if (isTimestamp(shmemState_ -> last) == 0)
  {
    #if defined(TEST) || defined(INFO)
    *logofs << "handleShmemEvent: Spent "
            << diffTimestamp(startTs, getTimestamp()) << " Ms "
            << "waiting for shared memory sequence for FD#"
            << fd_ << ".\n" << logofs_flush;
    #endif

    return 1;
  }

  #if defined(TEST) || defined(INFO)
  *logofs << "handleShmemEvent: WARNING! Can't reset shared "
          << "memory sequence for FD#" << fd_ << " after "
          << diffTimestamp(shmemState_ -> last, getTimestamp())
          << " Ms.\n" << logofs_flush;
  #endif

  return 0;
}

int ServerChannel::checkShmemEvent(unsigned char event, unsigned short sequence,
                                       const unsigned char *buffer)
{
  if (isTimestamp(shmemState_ -> last) == 1 &&
          sequence == shmemState_ -> sequence)
  {
    #ifdef TEST
    *logofs << "checkShmemEvent: Reset shared memory sequence "
            << shmemState_ -> sequence << " for FD#" << fd_
            << " after " << diffTimestamp(shmemState_ -> last,
               getTimestamp()) << " Ms.\n" << logofs_flush;
    #endif

    shmemState_ -> sequence = 0;
    shmemState_ -> offset   = 0;
    shmemState_ -> last     = nullTimestamp();
  }
  #ifdef TEST
  else
  {
    *logofs << "checkShmemEvent: Skipping past shared memory "
            << "image sequence " << sequence << " for FD#"
            << fd_ << ".\n" << logofs_flush;
  }
  #endif

  return 1;
}

int ServerChannel::checkShmemError(unsigned char error, unsigned short sequence,
                                       const unsigned char *buffer)
{
  #ifdef TEST

  *logofs << "checkShmemError: WARNING! Failed operation for "
          << "FD#" << fd_ << " in stage " << shmemState_ -> stage
          << " with ERR_CODE#" << (unsigned int) *(buffer + 1);

  *logofs << " RES_ID#" << GetULONG(buffer + 4, bigEndian_);

  *logofs << " MIN_OP#" << GetUINT(buffer + 8, bigEndian_);

  *logofs << " MAJ_OP#" << (unsigned int) *(buffer + 10);

  *logofs << " sequence " << sequence << ".\n";

  *logofs << logofs_flush;

  #endif

  //
  // If enabled flag is <= 0 we are still
  // in the inizialization phase. In this
  // case force presence to false.
  //

  if (shmemState_ -> enabled != 1)
  {
    if (shmemState_ -> present != 0)
    {
      #ifdef TEST
      *logofs << "checkShmemError: Resetting shared memory "
              << "presence flag for FD#" << fd_ << ".\n"
              << logofs_flush;
      #endif

      shmemState_ -> present = 0;
    }

    return 0;
  }

  if (shmemState_ -> sequence == sequence)
  {
    //
    // Reset the sequence and timestamp.
    //

    shmemState_ -> sequence = 0;
    shmemState_ -> offset   = 0;
    shmemState_ -> last     = nullTimestamp();
  }

  return 1;
}

int ServerChannel::handleFontRequest(DecodeBuffer &decodeBuffer, unsigned char &opcode,
                                         unsigned char *&buffer, unsigned int &size)
{
  //
  // Send a synchronization request and use
  // the reply to return the requested font
  // path.
  //

  #ifdef TEST
  *logofs << "handleFontRequest: Sending X_GetInputFocus request "
          << "for FD#" << fd_ << " due to OPCODE#" << (unsigned int)
             opcodeStore_ -> getFontParameters << ".\n"
          << logofs_flush;
  #endif

  opcode = X_GetInputFocus;

  size   = 4;
  buffer = writeBuffer_.addMessage(size);

  sequenceQueue_.push(clientSequence_, X_GetInputFocus,
                          opcodeStore_ -> getFontParameters);

  return 1;
}

int ServerChannel::handleFontReply(EncodeBuffer &encodeBuffer, const unsigned char opcode,
                                       const unsigned char *buffer, const unsigned int size)
{
  #ifdef TEST
  *logofs << "handleFontReply: Encoding font operation "
          << "reply with size " << size << ".\n"
          << logofs_flush;
  #endif

  char data[256];

  if (fontPort_ != -1)
  {
    sprintf(data + 1, "tcp/localhost:%d", fontPort_);
  }
  else
  {
    *(data + 1) = '\0';
  }

  *data = strlen(data + 1);

  unsigned char *next = (unsigned char *) data;

  unsigned int length = (unsigned int) (*next++);

  encodeBuffer.encodeValue(length, 8);

  encodeBuffer.encodeTextData(next, length);

  return 1;
}

int ServerChannel::handleCacheRequest(DecodeBuffer &decodeBuffer, unsigned char &opcode,
                                          unsigned char *&buffer, unsigned int &size)
{
  unsigned int mask;

  decodeBuffer.decodeCachedValue(mask, 32, clientCache_ ->
                     setCacheParametersCache);

  splitState_.save = (mask >> 8) & 0xff;
  splitState_.load = mask & 0xff;

  //
  // Just to be sure. We should never
  // receive this request if connected
  // to an old proxy version.
  //

  handleSplitEnable();

  #ifdef TEST
  *logofs << "handleCacheRequest: Set cache parameters to "
          << "save " << splitState_.save << " load "
          << splitState_.load << ".\n" << logofs_flush;
  #endif

  handleNullRequest(opcode, buffer, size);

  return 1;
}

int ServerChannel::handleStartSplitRequest(DecodeBuffer &decodeBuffer, unsigned char &opcode,
                                               unsigned char *&buffer, unsigned int &size)
{
  //
  // Prepare for the split for the selected
  // resource. Old proxy versions only use
  // the split store at position 0.
  //

  if (control -> isProtoStep7() == 1)
  {
    unsigned char resource;

    decodeBuffer.decodeCachedValue(resource, 8,
                       clientCache_ -> resourceCache);

    splitState_.resource = resource;

    splitState_.current = splitState_.resource;

    #if defined(TEST) || defined(SPLIT)
    *logofs << "handleStartSplitRequest: SPLIT! Registered id "
            << splitState_.resource << " as resource "
            << "waiting for a split.\n" << logofs_flush;
    #endif
  }
  #if defined(TEST) || defined(SPLIT)
  else
  {
    *logofs << "handleStartSplitRequest: SPLIT! Assuming fake id "
            << splitState_.current << " as resource "
            << "waiting for a split.\n" << logofs_flush;
  }
  #endif

  handleNullRequest(opcode, buffer, size);

  return 1;
}

int ServerChannel::handleEndSplitRequest(DecodeBuffer &decodeBuffer, unsigned char &opcode,
                                             unsigned char *&buffer, unsigned int &size)
{
  //
  // Verify that the agent resource matches.
  //

  if (control -> isProtoStep7() == 1)
  {
    unsigned char resource;

    decodeBuffer.decodeCachedValue(resource, 8,
                       clientCache_ -> resourceCache);

    #ifdef TEST

    if (splitState_.resource == nothing)
    {
      #ifdef PANIC
      *logofs << "handleEndSplitRequest: PANIC! SPLIT! Received an end of "
              << "split for resource id " << (unsigned int) *(buffer + 1)
              << " without a previous start.\n"
              << logofs_flush;
      #endif

      HandleCleanup();
    }
    else if (resource != splitState_.resource)
    {
      #ifdef PANIC
      *logofs << "handleEndSplitRequest: PANIC! SPLIT! Invalid resource id "
              << resource << " received while waiting for resource id "
              << splitState_.resource << ".\n" << logofs_flush;
      #endif

      HandleCleanup();
    }

    #endif
  }

  #if defined(TEST) || defined(SPLIT)
  *logofs << "handleEndSplitRequest: SPLIT! Reset id "
          << splitState_.resource << " as resource "
          << "selected for splits.\n" << logofs_flush;
  #endif

  splitState_.resource = nothing;

  handleNullRequest(opcode, buffer, size);

  return 1;
}

int ServerChannel::handleSplitChecksum(DecodeBuffer &decodeBuffer, T_checksum &checksum)
{
  unsigned int receive;

  if (control -> isProtoStep7() == 1)
  {
    decodeBuffer.decodeBoolValue(receive);
  }
  else
  {
    receive = (control -> ImageCacheEnableLoad == 1 ||
                   control -> ImageCacheEnableSave == 1);
  }

  if (receive == 1)
  {
    checksum = new md5_byte_t[MD5_LENGTH];

    for (unsigned int i = 0; i < MD5_LENGTH; i++)
    {
      decodeBuffer.decodeValue(receive, 8);

      if (checksum != NULL)
      {
        checksum[i] = (unsigned char) receive;
      }
    }

    #if defined(TEST) || defined(SPLIT)
    *logofs << "handleSplitChecksum: SPLIT! Received checksum "
            <<  "[" << DumpChecksum(checksum) << "].\n"
            << logofs_flush;
    #endif

    return 1;
  }

  return 0;
}

void ServerChannel::handleShmemStateAlloc()
{
  if (shmemState_ == NULL)
  {
    shmemState_ = new T_shmem_state();

    shmemState_ -> stage   = -1;
    shmemState_ -> present = -1;
    shmemState_ -> enabled = -1;

    shmemState_ -> segment = -1;
    shmemState_ -> id      = -1;
    shmemState_ -> address = NULL;
    shmemState_ -> size    = 0;

    shmemState_ -> opcode = 0xff;
    shmemState_ -> event  = 0xff;
    shmemState_ -> error  = 0xff;

    shmemState_ -> sequence = 0;
    shmemState_ -> offset   = 0;
    shmemState_ -> last     = nullTimestamp();

    shmemState_ -> checked  = 0;
  }
}

void ServerChannel::handleShmemStateRemove()
{
  if (shmemState_ != NULL)
  {
   if (shmemState_ -> address != NULL)
    {
#ifndef ANDROID
       shmdt((char *) shmemState_ -> address);
#else
    cerr << "Info: handleShmemStateRemove: In android no shared memory. This should not be displayed. address should always be NULL\n";
#endif
    }

    if (shmemState_ -> id > 0)
    {
#ifndef ANDROID
      shmctl(shmemState_ -> id, IPC_RMID, 0);
#else
      cerr << "Info: handleShmemStateRemove: In android no shared memory. This should not be displayed. id should always be 0\n";
#endif
    }

    delete shmemState_;

    shmemState_ = NULL;
  }
}

void ServerChannel::handleUnpackStateInit(int resource)
{
  if (unpackState_[resource] == NULL)
  {
    unpackState_[resource] = new T_unpack_state();

    if (unpackState_[resource] == NULL)
    {
      #ifdef PANIC
      *logofs << "handleUnpackStateInit: PANIC! Can't allocate "
              << "memory for unpack state in context [A].\n"
              << logofs_flush;
      #endif

      cerr << "Error" << ": Can't allocate memory for "
           << "unpack state in context [A].\n";

      HandleAbort();
    }

    unpackState_[resource] -> geometry = NULL;
    unpackState_[resource] -> colormap = NULL;
    unpackState_[resource] -> alpha    = NULL;
  }
}

void ServerChannel::handleUnpackAllocGeometry(int resource)
{
  if (unpackState_[resource] -> geometry == NULL)
  {
    unpackState_[resource] -> geometry = new T_geometry();

    if (unpackState_[resource] -> geometry == NULL)
    {
      #ifdef PANIC
      *logofs << "handleUnpackAllocGeometry: PANIC! Can't allocate "
              << "memory for unpack state in context [B].\n"
              << logofs_flush;
      #endif

      cerr << "Error" << ": Can't allocate memory for "
           << "unpack state in context [B].\n";

      HandleAbort();
    }

    unpackState_[resource] -> geometry -> depth1_bpp  = 4;
    unpackState_[resource] -> geometry -> depth4_bpp  = 4;
    unpackState_[resource] -> geometry -> depth8_bpp  = 8;
    unpackState_[resource] -> geometry -> depth16_bpp = 16;
    unpackState_[resource] -> geometry -> depth24_bpp = 32;
    unpackState_[resource] -> geometry -> depth32_bpp = 32;

    unpackState_[resource] -> geometry -> red_mask   = 0xff0000;
    unpackState_[resource] -> geometry -> green_mask = 0x00ff00;
    unpackState_[resource] -> geometry -> blue_mask  = 0x0000ff;

    unpackState_[resource] -> geometry -> image_byte_order = imageByteOrder_;
    unpackState_[resource] -> geometry -> bitmap_bit_order = bitmapBitOrder_;
    unpackState_[resource] -> geometry -> scanline_unit    = scanlineUnit_;
    unpackState_[resource] -> geometry -> scanline_pad     = scanlinePad_;
  }
}

void ServerChannel::handleUnpackAllocColormap(int resource)
{
  if (unpackState_[resource] -> colormap == NULL)
  {
    unpackState_[resource] -> colormap = new T_colormap();

    if (unpackState_[resource] -> colormap == NULL)
    {
      #ifdef PANIC
      *logofs << "handleUnpackAllocColormap: PANIC! Can't allocate "
              << "memory for unpack state in context [C].\n"
              << logofs_flush;
      #endif

      cerr << "Error" << ": Can't allocate memory for "
           << "unpack state in context [C].\n";

      HandleAbort();
    }

    unpackState_[resource] -> colormap -> entries = 0;
    unpackState_[resource] -> colormap -> data    = NULL;
  }
}

void ServerChannel::handleUnpackAllocAlpha(int resource)
{
  if (unpackState_[resource] -> alpha == NULL)
  {
    unpackState_[resource] -> alpha = new T_alpha();

    if (unpackState_[resource] -> alpha == NULL)
    {
      #ifdef PANIC
      *logofs << "handleUnpackAllocAlpha: PANIC! Can't allocate "
              << "memory for unpack state in context [D].\n"
              << logofs_flush;
      #endif

      cerr << "Error" << ": Can't allocate memory for "
           << "unpack state in context [D].\n";

      HandleAbort();
    }

    unpackState_[resource] -> alpha -> entries = 0;
    unpackState_[resource] -> alpha -> data    = NULL;
  }
}

void ServerChannel::handleUnpackStateRemove(int resource)
{
  if (unpackState_[resource] != NULL)
  {
    delete unpackState_[resource] -> geometry;

    if (unpackState_[resource] -> colormap != NULL)
    {
      delete [] unpackState_[resource] -> colormap -> data;
    }

    delete unpackState_[resource] -> colormap;

    if (unpackState_[resource] -> alpha != NULL)
    {
      delete [] unpackState_[resource] -> alpha -> data;
    }

    delete unpackState_[resource] -> alpha;

    delete unpackState_[resource];

    unpackState_[resource] = NULL;
  }
}

void ServerChannel::handleEncodeCharInfo(const unsigned char *nextSrc, EncodeBuffer &encodeBuffer)
{
  unsigned int value = GetUINT(nextSrc, bigEndian_) |
                           (GetUINT(nextSrc + 10, bigEndian_) << 16);

  encodeBuffer.encodeCachedValue(value, 32,
                     *serverCache_ -> queryFontCharInfoCache[0], 6);

  nextSrc += 2;

  for (unsigned int i = 1; i < 5; i++)
  {
    unsigned int value = GetUINT(nextSrc, bigEndian_);

    nextSrc += 2;

    encodeBuffer.encodeCachedValue(value, 16,
                       *serverCache_ -> queryFontCharInfoCache[i], 6);
  }
}

int ServerChannel::setBigEndian(int flag)
{
  bigEndian_ = flag;

  readBuffer_.setBigEndian(flag);

  return 1;
}

int ServerChannel::setReferences()
{
  #ifdef TEST
  *logofs << "ServerChannel: Initializing the static "
          << "members for the server channels.\n"
          << logofs_flush;
  #endif

  #ifdef REFERENCES

  references_ = 0;

  #endif

  return 1;
}
