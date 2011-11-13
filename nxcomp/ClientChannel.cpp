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

#include <X11/X.h>
#include <X11/Xatom.h>

#include "NXproto.h"
#include "NXrender.h"

#include "ClientChannel.h"

#include "EncodeBuffer.h"
#include "DecodeBuffer.h"

#include "StaticCompressor.h"

#include "Statistics.h"
#include "Proxy.h"

#include "PutImage.h"
#include "PutPackedImage.h"

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
// Define this to trace the invocations
// of the agent's callbacks.
//

#undef  CALLBACK

//
// By defining this, a simple procedure is activated at
// startup which just allocates and deallocates plenty
// of cache objects. This is used to help determine the
// current memory requirements.
//

#undef  MEMORY

//
// Inspects target of common X operations.
//

#undef  TARGETS

#ifdef TARGETS

#include <set>
#include <map>

typedef set < unsigned int, less<unsigned int> > T_windows;
typedef set < unsigned int, less<unsigned int> > T_pixmaps;
typedef map < unsigned int, unsigned int, less<unsigned int> > T_gcontexts;

T_windows   windows;
T_pixmaps   pixmaps;
T_gcontexts gcontexts;

#endif

//
// Define this to log when a channel
// is created or destroyed.
//

#undef  REFERENCES

//
// Here are the static members.
//

#ifdef REFERENCES

int ClientChannel::references_ = 0;

#endif

ClientChannel::ClientChannel(Transport *transport, StaticCompressor *compressor)

  : Channel(transport, compressor), readBuffer_(transport_, this)
{
  //
  // Sequence number of the next message
  // being encoded or decoded.
  //

  clientSequence_ = 0;
  serverSequence_ = 0;

  //
  // Current sequence known by NX agent.
  //

  lastSequence_ = 0;

  //
  // This is used to test the synchronous
  // flush in the proxy.
  //

  lastRequest_ = 0;

  //
  // Store information about the images
  // being streamed.
  //

  splitState_.resource = nothing;
  splitState_.pending  = 0;
  splitState_.commit   = 0;
  splitState_.mode     = split_none;

  //
  // Disable image streaming if the remote
  // doesn't support our proxy version.
  //

  handleSplitEnable();

  //
  // Number of outstanding tainted replies.
  //

  taintCounter_ = 0;

  #ifdef MEMORY

  *logofs << "ClientChannel: Created 1 ClientCache and 1 ServerCache. "
          << "You have 30 seconds to check the allocated size.\n"
          << logofs_flush;

  sleep(30);

  ClientCache *clientCacheTestArray[100];
  ServerCache *serverCacheTestArray[100];

  for (int i = 0; i < 100; i++)
  {
    clientCacheTestArray[i] = new ClientCache();
  }

  *logofs << "ClientChannel: Created further 100 ClientCache. "
          << "You have 30 seconds to check the allocated size.\n"
          << logofs_flush;

  sleep(30);

  for (int i = 0; i < 100; i++)
  {
    serverCacheTestArray[i] = new ServerCache();
  }

  *logofs << "ClientChannel: Created further 100 ServerCache. "
          << "You have 30 seconds to check the allocated size.\n"
          << logofs_flush;

  sleep(30);

  for (int i = 0; i < 100; i++)
  {
    delete clientCacheTestArray[i];
    delete serverCacheTestArray[i];
  }

  *logofs << "ClientChannel: Deleted 100 ClientCache and 100 ServerCache. "
          << "You have 30 seconds to check the allocated size.\n"
          << logofs_flush;

  sleep(30);

  #endif

  #ifdef REFERENCES
  *logofs << "ClientChannel: Created new object at " 
          << this << " for FD#" << fd_ << " out of " 
          << ++references_ << " allocated channels.\n"
          << logofs_flush;
  #endif
}

ClientChannel::~ClientChannel()
{
  #ifdef REFERENCES
  *logofs << "ClientChannel: Deleted object at " 
          << this << " for FD#" << fd_ << " out of "
          << --references_ << " allocated channels.\n"
          << logofs_flush;
  #endif
}

//
// Beginning of handleRead().
//

int ClientChannel::handleRead(EncodeBuffer &encodeBuffer, const unsigned char *message,
                                  unsigned int length)
{
  #ifdef TEST
  *logofs << "handleRead: Called for FD#" << fd_
          << " with " << encodeBuffer.getLength()
          << " bytes already encoded.\n"
          << logofs_flush;
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

  //
  // Check if we can borrow the buffer
  // from the caller.
  //

  if (message != NULL && length != 0)
  {
    readBuffer_.readMessage(message, length);
  }
  else
  {
    //
    // Get the data from the transport.
    //

    #if defined(TEST) || defined(INFO)
    *logofs << "handleRead: Trying to read from FD#"
            << fd_ << " at " << strMsTimestamp() << ".\n"
            << logofs_flush;
    #endif

    int result = readBuffer_.readMessage();

    #ifdef DEBUG
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

      *logofs << "handleRead: PANIC! No data read from FD#"
              << fd_ << " while encoding messages.\n"
              << logofs_flush;

      HandleCleanup();

      #endif

      return 0;
    }
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
 
    if (firstRequest_)
    {
      //
      // Need to add the length of the first
      // request as it was not present in
      // previous versions.
      //

      if (control -> isProtoStep7() == 1)
      {
        encodeBuffer.encodeValue(inputLength, 8);
      }

      for (unsigned int i = 0; i < inputLength; i++)
      {
        encodeBuffer.encodeValue((unsigned int) inputMessage[i], 8);
      }

      firstRequest_ = 0;

      #if defined(TEST) || defined(OPCODES)

      int bits = encodeBuffer.diffBits();

      *logofs << "handleRead: Handled first request. " << inputLength
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
      // First of all we get the opcode.
      //

      unsigned char inputOpcode = *inputMessage;

      #if defined(TEST) || defined(INFO)

      //
      // This is used to test the synchronous
      // flush in the parent proxy.
      //

      lastRequest_ = inputOpcode;

      #endif

      //
      // Check if the request is supported by the
      // remote. If not, only handle it locally and
      // taint the opcode as a X_NoOperation. Also
      // try to short-circuit some replies at this
      // side. XSync requests, for example, weight
      // for half of the total round-trips.
      //

      if (handleTaintRequest(inputOpcode, inputMessage,
                                 inputLength) < 0)
      {
        return -1;
      }

      encodeBuffer.encodeOpcodeValue(inputOpcode, clientCache_ -> opcodeCache);

      //
      // Update the current sequence.
      //

      clientSequence_++;
      clientSequence_ &= 0xffff;

      #ifdef DEBUG
      *logofs << "handleRead: Last client sequence number for FD#" 
              << fd_ << " is " << clientSequence_ << ".\n"
              << logofs_flush;
      #endif

      //
      // If differential compression is disabled
      // then use the most simple encoding.
      //

      if (control -> LocalDeltaCompression == 0)
      {
        int result = handleFastReadRequest(encodeBuffer, inputOpcode,
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
      // Go to the message's specific encoding.
      //

      switch (inputOpcode)
      {
      case X_AllocColor:
        {
          encodeBuffer.encodeCachedValue(GetULONG(inputMessage + 4, bigEndian_), 29,
                             clientCache_ -> colormapCache);
          const unsigned char *nextSrc = inputMessage + 8;
          unsigned int colorData[3];
          for (unsigned int i = 0; i < 3; i++)
          {
            unsigned int value = GetUINT(nextSrc, bigEndian_);
            encodeBuffer.encodeCachedValue(value, 16,
                               *(clientCache_ -> allocColorRGBCache[i]), 4);
            colorData[i] = value;
            nextSrc += 2;
          }

          sequenceQueue_.push(clientSequence_, inputOpcode,
                                  colorData[0], colorData[1], colorData[2]);

          priority_++;
        }
        break;
      case X_ReparentWindow:
        {
          encodeBuffer.encodeXidValue(GetULONG(inputMessage + 4, bigEndian_),
                             clientCache_ -> windowCache);
          encodeBuffer.encodeXidValue(GetULONG(inputMessage + 8, bigEndian_),
                             clientCache_ -> windowCache);
          encodeBuffer.encodeValue(GetUINT(inputMessage + 12, bigEndian_), 16, 11);
          encodeBuffer.encodeValue(GetUINT(inputMessage + 14, bigEndian_), 16, 11);
        }
        break;
      case X_ChangeProperty:
        {
          MessageStore *messageStore = clientStore_ ->
                               getRequestStore(X_ChangeProperty);

          if (handleEncodeCached(encodeBuffer, clientCache_, messageStore,
                                     inputMessage, inputLength))
          {
            hit = 1;

            break;
          }

          unsigned char format = inputMessage[16];
          encodeBuffer.encodeCachedValue(format, 8,
                             clientCache_ -> changePropertyFormatCache);
          unsigned int dataLength = GetULONG(inputMessage + 20, bigEndian_);
          encodeBuffer.encodeValue(dataLength, 32, 6);
          encodeBuffer.encodeValue(inputMessage[1], 2);
          encodeBuffer.encodeXidValue(GetULONG(inputMessage + 4, bigEndian_),
                             clientCache_ -> windowCache);
          encodeBuffer.encodeCachedValue(GetULONG(inputMessage + 8, bigEndian_), 29,
                             clientCache_ -> changePropertyPropertyCache, 9);
          encodeBuffer.encodeCachedValue(GetULONG(inputMessage + 12, bigEndian_), 29,
                             clientCache_ -> changePropertyTypeCache, 9);
          const unsigned char *nextSrc = inputMessage + 24;
          if (format == 8)
          {
            if (control -> isProtoStep7() == 1)
            {
              encodeBuffer.encodeTextData(nextSrc, dataLength);
            }
            else
            {
              clientCache_ -> changePropertyTextCompressor.reset();
              for (unsigned int i = 0; i < dataLength; i++)
                clientCache_ -> changePropertyTextCompressor.
                                      encodeChar(*nextSrc++, encodeBuffer);
            }
          }
          else if (format == 32)
          {
            for (unsigned int i = 0; i < dataLength; i++)
            {
              encodeBuffer.encodeCachedValue(GetULONG(nextSrc, bigEndian_), 32,
                                 clientCache_ -> changePropertyData32Cache);
              nextSrc += 4;
            }
          }
          else
          {
            for (unsigned int i = 0; i < dataLength; i++)
            {
              encodeBuffer.encodeValue(GetUINT(nextSrc, bigEndian_), 16);
              nextSrc += 2;
            }
          }
        }
        break;
      case X_SendEvent:
        {
          //
          // TODO: This can be improved. In the worst
          // cases, it appears to provide a poor 1.6:1
          // ratio.
          //

          MessageStore *messageStore = clientStore_ ->
                               getRequestStore(X_SendEvent);

          if (handleEncodeCached(encodeBuffer, clientCache_, messageStore,
                                     inputMessage, inputLength))
          {
            hit = 1;

            break;
          }

          encodeBuffer.encodeBoolValue((unsigned int) inputMessage[1]);
          unsigned int window = GetULONG(inputMessage + 4, bigEndian_);

          if (window == 0 || window == 1)
          {
            encodeBuffer.encodeBoolValue(1);
            encodeBuffer.encodeBoolValue(window);
          }
          else
          {
            encodeBuffer.encodeBoolValue(0);
            encodeBuffer.encodeXidValue(window, clientCache_ -> windowCache);
          }

          encodeBuffer.encodeCachedValue(GetULONG(inputMessage + 8, bigEndian_), 32,
                             clientCache_ -> sendEventMaskCache, 9);
          encodeBuffer.encodeCachedValue(*(inputMessage + 12), 8, 
                             clientCache_ -> sendEventCodeCache);
          encodeBuffer.encodeCachedValue(*(inputMessage + 13), 8, 
                             clientCache_ -> sendEventByteDataCache);

          unsigned int newSeq = GetUINT(inputMessage + 14, bigEndian_);
          unsigned int diffSeq = newSeq - clientCache_ -> sendEventLastSequence;
          clientCache_ -> sendEventLastSequence = newSeq;
          encodeBuffer.encodeValue(diffSeq, 16, 4);
          encodeBuffer.encodeCachedValue(GetULONG(inputMessage + 16, bigEndian_), 32,
                             clientCache_ -> sendEventIntDataCache);

          for (unsigned int i = 20; i < 44; i++)
          {
            encodeBuffer.encodeCachedValue((unsigned int) inputMessage[i], 8,
                               clientCache_ -> sendEventEventCache);
          }
        }
        break;
      case X_ChangeWindowAttributes:
        {
          encodeBuffer.encodeValue((inputLength - 12) >> 2, 4);
          encodeBuffer.encodeXidValue(GetULONG(inputMessage + 4, bigEndian_),
                             clientCache_ -> windowCache);
          unsigned int bitmask = GetULONG(inputMessage + 8, bigEndian_);
          encodeBuffer.encodeCachedValue(bitmask, 15,
                             clientCache_ -> createWindowBitmaskCache);
          const unsigned char *nextSrc = inputMessage + 12;
          unsigned int mask = 0x1;
          for (unsigned int j = 0; j < 15; j++)
          {
            if (bitmask & mask)
            {
              encodeBuffer.encodeCachedValue(GetULONG(nextSrc, bigEndian_), 32,
                                 *clientCache_ -> createWindowAttrCache[j]);
              nextSrc += 4;
            }
            mask <<= 1;
          }
        }
        break;
      case X_ClearArea:
        {
          #ifdef TARGETS

          unsigned int t_id = GetULONG(inputMessage + 4, bigEndian_);

          if (pixmaps.find(t_id) != pixmaps.end())
          {
            *logofs << "handleRead: X_ClearArea target id is pixmap "
                    << t_id << ".\n" << logofs_flush;
          }
          else if (windows.find(t_id) != windows.end())
          {
            *logofs << "handleRead: X_ClearArea target id is window "
                    << t_id << ".\n" << logofs_flush;
          }
          else
          {
            *logofs << "handleRead: X_ClearArea target id " << t_id
                    << " is unrecognized.\n" << logofs_flush;
          }

          #endif

          MessageStore *messageStore = clientStore_ ->
                               getRequestStore(X_ClearArea);

          if (handleEncodeCached(encodeBuffer, clientCache_, messageStore,
                                     inputMessage, inputLength))
          {
            hit = 1;

            break;
          }

          encodeBuffer.encodeBoolValue((unsigned int) inputMessage[1]);
          encodeBuffer.encodeXidValue(GetULONG(inputMessage + 4, bigEndian_),
                             clientCache_ -> windowCache);
          const unsigned char *nextSrc = inputMessage + 8;
          for (unsigned int i = 0; i < 4; i++)
          {
            encodeBuffer.encodeCachedValue(GetUINT(nextSrc, bigEndian_), 16,
                               *clientCache_ -> clearAreaGeomCache[i], 8);
            nextSrc += 2;
          }
        }
        break;
      case X_CloseFont:
        {
          unsigned int font = GetULONG(inputMessage + 4, bigEndian_);
          encodeBuffer.encodeValue(font - clientCache_ -> lastFont, 29, 5);
          clientCache_ -> lastFont = font;
        }
        break;
      case X_ConfigureWindow:
        {
          MessageStore *messageStore = clientStore_ ->
                               getRequestStore(X_ConfigureWindow);

          if (handleEncodeCached(encodeBuffer, clientCache_, messageStore,
                                     inputMessage, inputLength))
          {
            hit = 1;

            break;
          }

          encodeBuffer.encodeXidValue(GetULONG(inputMessage + 4, bigEndian_),
                             clientCache_ -> windowCache);
          unsigned int bitmask = GetUINT(inputMessage + 8, bigEndian_);
          encodeBuffer.encodeCachedValue(bitmask, 7,
                                  clientCache_ -> configureWindowBitmaskCache);
          unsigned int mask = 0x1;
          const unsigned char *nextSrc = inputMessage + 12;
          for (unsigned int i = 0; i < 7; i++)
          {
            if (bitmask & mask)
            {
              encodeBuffer.encodeCachedValue(GetULONG(nextSrc, bigEndian_),
                                 CONFIGUREWINDOW_FIELD_WIDTH[i],
                                     *clientCache_ -> configureWindowAttrCache[i], 8);
              nextSrc += 4;
            }
            mask <<= 1;
          }
        }
        break;
      case X_ConvertSelection:
        {
          encodeBuffer.encodeCachedValue(GetULONG(inputMessage + 4, bigEndian_), 29,
                             clientCache_ -> convertSelectionRequestorCache, 9);
          const unsigned char* nextSrc = inputMessage + 8;
          for (unsigned int i = 0; i < 3; i++)
          {
            encodeBuffer.encodeCachedValue(GetULONG(nextSrc, bigEndian_), 29,
                               *(clientCache_ -> convertSelectionAtomCache[i]), 9);
            nextSrc += 4;
          }
          unsigned int timestamp = GetULONG(nextSrc, bigEndian_);
          encodeBuffer.encodeValue(timestamp -
                             clientCache_ -> convertSelectionLastTimestamp, 32, 4);
          clientCache_ -> convertSelectionLastTimestamp = timestamp;
        }
        break;
      case X_CopyArea:
        {
          #ifdef TARGETS

          unsigned int t_id = GetULONG(inputMessage + 4, bigEndian_);

          if (pixmaps.find(t_id) != pixmaps.end())
          {
            *logofs << "handleRead: X_CopyArea source id is pixmap "
                    << t_id << ".\n" << logofs_flush;
          }
          else if (windows.find(t_id) != windows.end())
          {
            *logofs << "handleRead: X_CopyArea source id is window "
                    << t_id << ".\n" << logofs_flush;
          }
          else
          {
            *logofs << "handleRead: X_CopyArea source id " << t_id
                    << " is unrecognized.\n" << logofs_flush;
          }

          t_id = GetULONG(inputMessage + 8, bigEndian_);

          if (pixmaps.find(t_id) != pixmaps.end())
          {
            *logofs << "handleRead: X_CopyArea target id is pixmap "
                    << t_id << ".\n" << logofs_flush;
          }
          else if (windows.find(t_id) != windows.end())
          {
            *logofs << "handleRead: X_CopyArea target id is window "
                    << t_id << ".\n" << logofs_flush;
          }
          else
          {
            *logofs << "handleRead: X_CopyArea target id " << t_id
                    << " is unrecognized.\n" << logofs_flush;
          }

          #endif

          MessageStore *messageStore = clientStore_ ->
                               getRequestStore(X_CopyArea);

          if (handleEncodeCached(encodeBuffer, clientCache_, messageStore,
                                     inputMessage, inputLength))
          {
            hit = 1;

            break;
          }

          encodeBuffer.encodeXidValue(GetULONG(inputMessage + 4,
                             bigEndian_), clientCache_ -> drawableCache);
          encodeBuffer.encodeXidValue(GetULONG(inputMessage + 8,
                             bigEndian_), clientCache_ -> drawableCache);
          encodeBuffer.encodeXidValue(GetULONG(inputMessage + 12,
                             bigEndian_), clientCache_ -> gcCache);
          const unsigned char *nextSrc = inputMessage + 16;
          for (unsigned int i = 0; i < 6; i++)
          {
            encodeBuffer.encodeCachedValue(GetUINT(nextSrc, bigEndian_), 16,
                               *clientCache_ -> copyAreaGeomCache[i], 8);
            nextSrc += 2;
          }
        }
        break;
      case X_CopyGC:
        {
          #ifdef TARGETS

          unsigned int s_g_id = GetULONG(inputMessage + 4, bigEndian_);
          unsigned int d_g_id = GetULONG(inputMessage + 8, bigEndian_);

          *logofs << "handleRead: X_CopyGC source gcontext id is " << s_g_id  
                  << " destination gcontext id is " << d_g_id << ".\n" 
                  << logofs_flush;

          #endif

          encodeBuffer.encodeXidValue(GetULONG(inputMessage + 4,
                             bigEndian_), clientCache_ -> gcCache);
          encodeBuffer.encodeXidValue(GetULONG(inputMessage + 8,
                             bigEndian_), clientCache_ -> gcCache);
          encodeBuffer.encodeCachedValue(GetULONG(inputMessage + 12,
                             bigEndian_), 23, clientCache_ -> createGCBitmaskCache);
        }
        break;
      case X_CopyPlane:
        {
          encodeBuffer.encodeXidValue(GetULONG(inputMessage + 4,
                             bigEndian_), clientCache_ -> drawableCache);
          encodeBuffer.encodeXidValue(GetULONG(inputMessage + 8,
                             bigEndian_), clientCache_ -> drawableCache);
          encodeBuffer.encodeXidValue(GetULONG(inputMessage + 12,
                             bigEndian_), clientCache_ -> gcCache);
          const unsigned char *nextSrc = inputMessage + 16;
          for (unsigned int i = 0; i < 6; i++)
          {
            encodeBuffer.encodeCachedValue(GetUINT(nextSrc, bigEndian_), 16,
                               *clientCache_ -> copyPlaneGeomCache[i], 8);
            nextSrc += 2;
          }
          encodeBuffer.encodeCachedValue(GetULONG(inputMessage + 28, bigEndian_), 32,
                                         clientCache_ -> copyPlaneBitPlaneCache, 10);
        }
        break;
      case X_CreateGC:
        {
          #ifdef TARGETS

          unsigned int g_id = GetULONG(inputMessage + 4, bigEndian_);
          unsigned int t_id = GetULONG(inputMessage + 8, bigEndian_);

          if (pixmaps.find(t_id) != pixmaps.end())
          {
            *logofs << "handleRead: X_CreateGC id " << g_id
                    << " target id is pixmap " << t_id
                    << ".\n" << logofs_flush;
          }
          else if (windows.find(t_id) != windows.end())
          {
            *logofs << "handleRead: X_CreateGC id " << g_id
                    << " target id is window " << t_id
                    << ".\n" << logofs_flush;
          }
          else
          {
            *logofs << "handleRead: X_CreateGC id " << g_id
                    << " target id is unrecognized.\n"
                    << logofs_flush;
          }

          gcontexts.insert(T_gcontexts::value_type(g_id, t_id));

          #endif

          MessageStore *messageStore = clientStore_ ->
                               getRequestStore(X_CreateGC);

          if (handleEncodeCached(encodeBuffer, clientCache_, messageStore,
                                     inputMessage, inputLength))
          {
            hit = 1;

            break;
          }

          if (control -> isProtoStep7() == 1)
          {
            encodeBuffer.encodeNewXidValue(GetULONG(inputMessage + 4, bigEndian_),
                               clientCache_ -> lastId, clientCache_ -> lastIdCache,
                                   clientCache_ -> gcCache,
                                       clientCache_ -> freeGCCache);
          }
          else
          {
            encodeBuffer.encodeXidValue(GetULONG(inputMessage + 4,
                               bigEndian_), clientCache_ -> gcCache);
          }

          const unsigned char *nextSrc = inputMessage + 8;
          encodeBuffer.encodeXidValue(GetULONG(inputMessage + 8,
                             bigEndian_), clientCache_ -> drawableCache);
          nextSrc += 4;
          unsigned int bitmask = GetULONG(nextSrc, bigEndian_);
          nextSrc += 4;
          encodeBuffer.encodeCachedValue(bitmask, 23,
                             clientCache_ -> createGCBitmaskCache);
          unsigned int mask = 0x1;
          for (unsigned int i = 0; i < 23; i++)
          {
            if (bitmask & mask)
            {
              unsigned int value = GetULONG(nextSrc, bigEndian_);
              nextSrc += 4;
              unsigned int fieldWidth = CREATEGC_FIELD_WIDTH[i];
              if (fieldWidth <= 4)
              {
                encodeBuffer.encodeValue(value, fieldWidth);
              }
              else
              {
                encodeBuffer.encodeCachedValue(value, fieldWidth,
                                   *clientCache_ -> createGCAttrCache[i]);
              }
            }
            mask <<= 1;
          }
        }
        break;
      case X_ChangeGC:
        {
          #ifdef TARGETS

          unsigned int g_id = GetULONG(inputMessage + 4, bigEndian_);

          T_gcontexts::iterator i = gcontexts.find(g_id);

          if (i != gcontexts.end())
          {
            unsigned int t_id = i -> second;

            if (pixmaps.find(t_id) != pixmaps.end())
            {
              *logofs << "handleRead: X_ChangeGC gcontext id is " << g_id  
                      << " target id is pixmap " << t_id << ".\n" 
                      << logofs_flush;
            }
            else if (windows.find(t_id) != windows.end())
            {
              *logofs << "handleRead: X_ChangeGC gcontext id is " << g_id  
                      << " target id is window " << t_id << ".\n" 
                      << logofs_flush;
            }
            else
            {
              *logofs << "handleRead: X_ChangeGC gcontext is " << g_id  
                      << " target id is unrecognized.\n" 
                      << logofs_flush;
            }
          }
          else
          {
            *logofs << "handleRead: X_ChangeGC gcontext id " << g_id  
                    << " is unrecognized.\n" << logofs_flush;
          }

          gcontexts.erase(g_id);

          #endif

          MessageStore *messageStore = clientStore_ ->
                               getRequestStore(X_ChangeGC);

          if (handleEncodeCached(encodeBuffer, clientCache_, messageStore,
                                     inputMessage, inputLength))
          {
            hit = 1;

            break;
          }

          encodeBuffer.encodeXidValue(GetULONG(inputMessage + 4,
                             bigEndian_), clientCache_ -> gcCache);
          const unsigned char *nextSrc = inputMessage + 8;
          unsigned int bitmask = GetULONG(nextSrc, bigEndian_);
          nextSrc += 4;
          encodeBuffer.encodeCachedValue(bitmask, 23,
                             clientCache_ -> createGCBitmaskCache);
          unsigned int mask = 0x1;
          for (unsigned int i = 0; i < 23; i++)
          {
            if (bitmask & mask)
            {
              unsigned int value = GetULONG(nextSrc, bigEndian_);
              nextSrc += 4;
              unsigned int fieldWidth = CREATEGC_FIELD_WIDTH[i];
              if (fieldWidth <= 4)
              {
                encodeBuffer.encodeValue(value, fieldWidth);
              }
              else
              {
                encodeBuffer.encodeCachedValue(value, fieldWidth,
                                   *clientCache_ -> createGCAttrCache[i]);
              }
            }
            mask <<= 1;
          }
        }
        break;
      case X_CreatePixmap:
        {
          #ifdef TARGETS

          *logofs << "handleRead: X_CreatePixmap depth " << (unsigned) inputMessage[1]
                  << ", pixmap id " << GetULONG(inputMessage + 4, bigEndian_)
                  << ", drawable " << GetULONG(inputMessage + 8, bigEndian_)
                  << ", width " << GetUINT(inputMessage + 12, bigEndian_)
                  << ", height " << GetUINT(inputMessage + 14, bigEndian_)
                  << ", size " << GetUINT(inputMessage + 2, bigEndian_) << 2
                  << ".\n" << logofs_flush;

          unsigned int   p_id = GetULONG(inputMessage + 4, bigEndian_);
          unsigned short p_sx = GetUINT(inputMessage + 12, bigEndian_);
          unsigned short p_sy = GetUINT(inputMessage + 14, bigEndian_);

          *logofs << "handleRead: X_CreatePixmap id is " << p_id 
                  << " width is " << p_sx << " height is " << p_sy 
                  << ".\n" << logofs_flush;

          if (p_sx * p_sy <= 64 * 64)
          {
            *logofs << "handleRead: X_CreatePixmap id " << p_id << " of size "
                    << p_sx << "x" << p_sy << "=" << p_sx * p_sy 
                    << " will be painted at client side.\n" << logofs_flush;
          }
          else
          {
            *logofs << "handleRead: X_CreatePixmap id " << p_id << " of size "
                    << p_sx << "x" << p_sy << "=" << p_sx * p_sy 
                    << " will be painted at server side.\n" << logofs_flush;
          }

          pixmaps.insert(p_id);

          #endif

          MessageStore *messageStore = clientStore_ ->
                               getRequestStore(X_CreatePixmap);

          hit = handleEncode(encodeBuffer, clientCache_, messageStore,
                                 inputOpcode, inputMessage, inputLength);
        }
        break;
      case X_CreateWindow:
        {
          #ifdef TARGETS

          unsigned int w_id = GetULONG(inputMessage + 4, bigEndian_);

          *logofs << "handleRead: X_CreateWindow id is " << w_id
                  << ".\n" << logofs_flush;

          windows.insert(w_id);

          #endif

          unsigned bitmask = GetULONG(inputMessage + 28, bigEndian_);
          encodeBuffer.encodeCachedValue((unsigned int) inputMessage[1], 8,
                                         clientCache_ -> depthCache);
          encodeBuffer.encodeXidValue(GetULONG(inputMessage + 8, bigEndian_),
                             clientCache_ -> windowCache);

          if (control -> isProtoStep7() == 1)
          {
            encodeBuffer.encodeNewXidValue(GetULONG(inputMessage + 4, bigEndian_),
                               clientCache_ -> lastId, clientCache_ -> lastIdCache,
                                   clientCache_ -> windowCache,
                                       clientCache_ -> freeWindowCache);
          }
          else
          {
            encodeBuffer.encodeXidValue(GetULONG(inputMessage + 4, bigEndian_),
                               clientCache_ -> windowCache);
          }
          const unsigned char *nextSrc = inputMessage + 12;
          for (unsigned int i = 0; i < 6; i++)
          {
            encodeBuffer.encodeCachedValue(GetUINT(nextSrc, bigEndian_), 16,
                               *clientCache_ -> createWindowGeomCache[i], 8);
            nextSrc += 2;
          }
          encodeBuffer.encodeCachedValue(GetULONG(inputMessage + 24,
                                 bigEndian_), 29, clientCache_ -> visualCache);
          encodeBuffer.encodeCachedValue(bitmask, 15,
                                     clientCache_ -> createWindowBitmaskCache);
          nextSrc = inputMessage + 32;
          unsigned int mask = 0x1;
          for (unsigned int j = 0; j < 15; j++)
          {
            if (bitmask & mask)
            {
              encodeBuffer.encodeCachedValue(GetULONG(nextSrc, bigEndian_), 32,
                                 *clientCache_ -> createWindowAttrCache[j]);
              nextSrc += 4;
            }
            mask <<= 1;
          }
        }
        break;
      case X_DeleteProperty:
        {
          encodeBuffer.encodeXidValue(GetULONG(inputMessage + 4, bigEndian_),
                             clientCache_ -> windowCache);
          encodeBuffer.encodeValue(GetULONG(inputMessage + 8, bigEndian_), 29, 9);
        }
        break;
      case X_FillPoly:
        {
          #ifdef TARGETS

          unsigned int t_id = GetULONG(inputMessage + 4, bigEndian_);

          if (pixmaps.find(t_id) != pixmaps.end())
          {
            *logofs << "handleRead: X_FillPoly target id is pixmap "
                    << t_id << ".\n" << logofs_flush;
          }
          else if (windows.find(t_id) != windows.end())
          {
            *logofs << "handleRead: X_FillPoly target id is window "
                    << t_id << ".\n" << logofs_flush;
          }
          else
          {
            *logofs << "handleRead: X_FillPoly target id " << t_id
                    << " is unrecognized.\n" << logofs_flush;
          }

          #endif

          MessageStore *messageStore = clientStore_ ->
                               getRequestStore(X_FillPoly);

          if (handleEncodeCached(encodeBuffer, clientCache_, messageStore,
                                     inputMessage, inputLength))
          {
            hit = 1;

            break;
          }

          unsigned int numPoints = ((inputLength - 16) >> 2);

          if (control -> isProtoStep10() == 1)
          {
            encodeBuffer.encodeCachedValue(numPoints, 16,
                               clientCache_ -> fillPolyNumPointsCache, 4);
          }
          else
          {
            encodeBuffer.encodeCachedValue(numPoints, 14,
                               clientCache_ -> fillPolyNumPointsCache, 4);
          }

          encodeBuffer.encodeXidValue(GetULONG(inputMessage + 4, bigEndian_),
                             clientCache_ -> drawableCache);
          encodeBuffer.encodeXidValue(GetULONG(inputMessage + 8, bigEndian_),
                             clientCache_ -> gcCache);
          encodeBuffer.encodeValue((unsigned int) inputMessage[12], 2);
          encodeBuffer.encodeBoolValue((unsigned int) inputMessage[13]);
          int relativeCoordMode = (inputMessage[13] != 0);
          const unsigned char *nextSrc = inputMessage + 16;
          unsigned int pointIndex = 0;

          for (unsigned int i = 0; i < numPoints; i++)
          {
            if (relativeCoordMode)
            {
              encodeBuffer.encodeCachedValue(GetUINT(nextSrc, bigEndian_), 16,
                                 *clientCache_ -> fillPolyXRelCache[pointIndex], 8);
              nextSrc += 2;
              encodeBuffer.encodeCachedValue(GetUINT(nextSrc, bigEndian_), 16,
                                 *clientCache_ -> fillPolyYRelCache[pointIndex], 8);
              nextSrc += 2;
            }
            else
            {
              unsigned int x = GetUINT(nextSrc, bigEndian_);
              nextSrc += 2;
              unsigned int y = GetUINT(nextSrc, bigEndian_);
              nextSrc += 2;
              unsigned int j;
              for (j = 0; j < 8; j++)
                if ((x == clientCache_ -> fillPolyRecentX[j]) &&
                    (y == clientCache_ -> fillPolyRecentY[j]))
                  break;
              if (j < 8)
              {
                encodeBuffer.encodeBoolValue(1);
                encodeBuffer.encodeValue(j, 3);
              }
              else
              {
                encodeBuffer.encodeBoolValue(0);
                encodeBuffer.encodeCachedValue(x, 16,
                             *clientCache_ -> fillPolyXAbsCache[pointIndex], 8);
                encodeBuffer.encodeCachedValue(y, 16,
                             *clientCache_ -> fillPolyYAbsCache[pointIndex], 8);
                clientCache_ -> fillPolyRecentX[clientCache_ -> fillPolyIndex] = x;
                clientCache_ -> fillPolyRecentY[clientCache_ -> fillPolyIndex] = y;
                clientCache_ -> fillPolyIndex++;
                if (clientCache_ -> fillPolyIndex == 8)
                  clientCache_ -> fillPolyIndex = 0;
              }
            }

            if (++pointIndex == 10) pointIndex = 0;
          }
        }
        break;
      case X_FreeColors:
        {
          unsigned int numPixels = GetUINT(inputMessage + 2, bigEndian_) - 3;
          encodeBuffer.encodeValue(numPixels, 16, 4);
          encodeBuffer.encodeCachedValue(GetULONG(inputMessage + 4, bigEndian_), 29,
                                         clientCache_ -> colormapCache);
          encodeBuffer.encodeValue(GetULONG(inputMessage + 8, bigEndian_), 32, 4);
          const unsigned char *nextSrc = inputMessage + 12;
          while (numPixels)
          {
            encodeBuffer.encodeValue(GetULONG(nextSrc, bigEndian_), 32, 8);
            nextSrc += 4;
            numPixels--;
          }
        }
        break;
      case X_FreeCursor:
        {
          encodeBuffer.encodeCachedValue(GetULONG(inputMessage + 4, bigEndian_),
                                         29, clientCache_ -> cursorCache, 9);
        }
        break;
      case X_FreeGC:
        {
          #ifdef TARGETS

          unsigned int g_id = GetULONG(inputMessage + 4, bigEndian_);

          T_gcontexts::iterator i = gcontexts.find(g_id);

          if (i != gcontexts.end())
          {
            unsigned int t_id = i -> second;

            if (pixmaps.find(t_id) != pixmaps.end())
            {
              *logofs << "handleRead: X_FreeGC gcontext id is " << g_id  
                      << " target id is pixmap " << t_id << ".\n" 
                      << logofs_flush;
            }
            else if (windows.find(t_id) != windows.end())
            {
              *logofs << "handleRead: X_FreeGC gcontext id is " << g_id  
                      << " target id is window " << t_id << ".\n" 
                      << logofs_flush;
            }
            else
            {
              *logofs << "handleRead: X_FreeGC gcontext id is " << g_id  
                      << " target id is unrecognized.\n" 
                      << logofs_flush;
            }
          }
          else
          {
            *logofs << "handleRead: X_FreeGC gcontext id " << g_id  
                    << " is unrecognized.\n" << logofs_flush;
          }

          gcontexts.erase(g_id);

          #endif

          if (control -> isProtoStep7() == 1)
          {
            encodeBuffer.encodeFreeXidValue(GetULONG(inputMessage + 4, bigEndian_),
                               clientCache_ -> freeGCCache);
          }
          else
          {
            encodeBuffer.encodeXidValue(GetULONG(inputMessage + 4, bigEndian_),
                               clientCache_ -> gcCache);
          }
        }
        break;
      case X_FreePixmap:
        {
          #ifdef TARGETS

          unsigned int p_id = GetULONG(inputMessage + 4, bigEndian_);

          *logofs << "handleRead: X_FreePixmap id is " << p_id << ".\n" << logofs_flush;

          pixmaps.erase(p_id);

          #endif

          if (control -> isProtoStep7() == 1)
          {
            encodeBuffer.encodeFreeXidValue(GetULONG(inputMessage + 4, bigEndian_),
                               clientCache_ -> freeDrawableCache);
          }
          else
          {
            unsigned int pixmap = GetULONG(inputMessage + 4, bigEndian_);
            unsigned int diff = pixmap - clientCache_ -> createPixmapLastId;
            if (diff == 0)
            {
              encodeBuffer.encodeBoolValue(1);
            }
            else
            {
              encodeBuffer.encodeBoolValue(0);
              clientCache_ -> createPixmapLastId = pixmap;
              encodeBuffer.encodeValue(diff, 29, 4);
            }
          }
        }
        break;
      case X_GetAtomName:
        {
          encodeBuffer.encodeValue(GetULONG(inputMessage + 4, bigEndian_), 29, 9);

          sequenceQueue_.push(clientSequence_, inputOpcode);

          priority_++;
        }
        break;
      case X_GetGeometry:
        {
          encodeBuffer.encodeXidValue(GetULONG(inputMessage + 4, bigEndian_),
                             clientCache_ -> drawableCache);

          sequenceQueue_.push(clientSequence_, inputOpcode);

          priority_++;
        }
        break;
      case X_GetInputFocus:
        {
          sequenceQueue_.push(clientSequence_, inputOpcode);

          priority_++;
        }
        break;
      case X_GetModifierMapping:
        {
          sequenceQueue_.push(clientSequence_, inputOpcode);

          priority_++;
        }
        break;
      case X_GetKeyboardMapping:
        {
          encodeBuffer.encodeValue((unsigned int) inputMessage[4], 8);
          encodeBuffer.encodeValue((unsigned int) inputMessage[5], 8);

          sequenceQueue_.push(clientSequence_, inputOpcode);

          priority_++;
        }
        break;
      case X_GetProperty:
        {
          MessageStore *messageStore = clientStore_ ->
                               getRequestStore(X_GetProperty);

          if (handleEncodeCached(encodeBuffer, clientCache_, messageStore,
                                     inputMessage, inputLength))
          {
            unsigned int property = GetULONG(inputMessage + 8, bigEndian_);

            sequenceQueue_.push(clientSequence_, inputOpcode, property);

            priority_++;

            hit = 1;

            break;
          }

          encodeBuffer.encodeBoolValue((unsigned int) inputMessage[1]);
          encodeBuffer.encodeXidValue(GetULONG(inputMessage + 4, bigEndian_),
                             clientCache_ -> windowCache);
          unsigned int property = GetULONG(inputMessage + 8, bigEndian_);
          encodeBuffer.encodeValue(property, 29, 9);
          encodeBuffer.encodeValue(GetULONG(inputMessage + 12, bigEndian_), 29, 9);
          encodeBuffer.encodeValue(GetULONG(inputMessage + 16, bigEndian_), 32, 2);
          encodeBuffer.encodeValue(GetULONG(inputMessage + 20, bigEndian_), 32, 8);

          sequenceQueue_.push(clientSequence_, inputOpcode, property);

          priority_++;
        }
        break;
      case X_GetSelectionOwner:
        {
          encodeBuffer.encodeCachedValue(GetULONG(inputMessage + 4, bigEndian_), 29,
                             clientCache_ -> getSelectionOwnerSelectionCache, 9);

          sequenceQueue_.push(clientSequence_, inputOpcode);

          priority_++;
        }
        break;
      case X_GrabButton:
        {
          encodeBuffer.encodeBoolValue((unsigned int) inputMessage[1]);
          encodeBuffer.encodeXidValue(GetULONG(inputMessage + 4, bigEndian_),
                             clientCache_ -> windowCache);
          encodeBuffer.encodeCachedValue(GetUINT(inputMessage + 8, bigEndian_), 16,
                             clientCache_ -> grabButtonEventMaskCache);
          encodeBuffer.encodeBoolValue((unsigned int) inputMessage[10]);
          encodeBuffer.encodeBoolValue((unsigned int) inputMessage[11]);
          encodeBuffer.encodeCachedValue(GetULONG(inputMessage + 12, bigEndian_), 29,
                             clientCache_ -> grabButtonConfineCache, 9);
          encodeBuffer.encodeCachedValue(GetULONG(inputMessage + 16, bigEndian_), 29,
                             clientCache_ -> cursorCache, 9);
          encodeBuffer.encodeCachedValue(inputMessage[20], 8,
                             clientCache_ -> grabButtonButtonCache);
          encodeBuffer.encodeCachedValue(GetUINT(inputMessage + 22, bigEndian_), 16,
                             clientCache_ -> grabButtonModifierCache);
        }
        break;
      case X_GrabPointer:
        {
          encodeBuffer.encodeBoolValue((unsigned int) inputMessage[1]);
          encodeBuffer.encodeXidValue(GetULONG(inputMessage + 4, bigEndian_),
                             clientCache_ -> windowCache);
          encodeBuffer.encodeCachedValue(GetUINT(inputMessage + 8, bigEndian_), 16,
                             clientCache_ -> grabButtonEventMaskCache);
          encodeBuffer.encodeBoolValue((unsigned int) inputMessage[10]);
          encodeBuffer.encodeBoolValue((unsigned int) inputMessage[11]);
          encodeBuffer.encodeCachedValue(GetULONG(inputMessage + 12,
                           bigEndian_), 29,
                               clientCache_ -> grabButtonConfineCache, 9);
          encodeBuffer.encodeCachedValue(GetULONG(inputMessage + 16,
                           bigEndian_), 29, clientCache_ -> cursorCache, 9);

          unsigned int timestamp = GetULONG(inputMessage + 20, bigEndian_);
          encodeBuffer.encodeValue(timestamp -
                           clientCache_ -> grabKeyboardLastTimestamp, 32, 4);
          clientCache_ -> grabKeyboardLastTimestamp = timestamp;

          sequenceQueue_.push(clientSequence_, inputOpcode);

          priority_++;
        }
        break;
      case X_GrabKeyboard:
        {
          encodeBuffer.encodeBoolValue((unsigned int) inputMessage[1]);
          encodeBuffer.encodeXidValue(GetULONG(inputMessage + 4, bigEndian_),
                             clientCache_ -> windowCache);
          unsigned int timestamp = GetULONG(inputMessage + 8, bigEndian_);
          encodeBuffer.encodeValue(timestamp -
                             clientCache_ -> grabKeyboardLastTimestamp, 32, 4);
          clientCache_ -> grabKeyboardLastTimestamp = timestamp;
          encodeBuffer.encodeBoolValue((unsigned int) inputMessage[12]);
          encodeBuffer.encodeBoolValue((unsigned int) inputMessage[13]);

          sequenceQueue_.push(clientSequence_, inputOpcode);

          priority_++;
        }
        break;
      case X_GrabServer:
      case X_UngrabServer:
      case X_NoOperation:
        {
        }
        break;
      case X_PolyText8:
        {
          #ifdef TARGETS

          unsigned int t_id = GetULONG(inputMessage + 4, bigEndian_);

          if (pixmaps.find(t_id) != pixmaps.end())
          {
            *logofs << "handleRead: X_PolyText8 target id is pixmap "
                    << t_id << ".\n" << logofs_flush;
          }
          else if (windows.find(t_id) != windows.end())
          {
            *logofs << "handleRead: X_PolyText8 target id is window "
                    << t_id << ".\n" << logofs_flush;
          }
          else
          {
            *logofs << "handleRead: X_PolyText8 target id " << t_id
                    << " is unrecognized.\n" << logofs_flush;
          }

          #endif

          MessageStore *messageStore = clientStore_ ->
                               getRequestStore(X_PolyText8);

          if (handleEncodeCached(encodeBuffer, clientCache_, messageStore,
                                     inputMessage, inputLength))
          {
            hit = 1;

            break;
          }

          encodeBuffer.encodeXidValue(GetULONG(inputMessage + 4,
                             bigEndian_), clientCache_ -> drawableCache);
          encodeBuffer.encodeXidValue(GetULONG(inputMessage + 8,
                             bigEndian_), clientCache_ -> gcCache);
          unsigned int x = GetUINT(inputMessage + 12, bigEndian_);
          int xDiff = x - clientCache_ -> polyTextLastX;
          clientCache_ -> polyTextLastX = x;
          encodeBuffer.encodeCachedValue(xDiff, 16,
                             clientCache_ -> polyTextCacheX);
          unsigned int y = GetUINT(inputMessage + 14, bigEndian_);
          int yDiff = y - clientCache_ -> polyTextLastY;
          clientCache_ -> polyTextLastY = y;
          encodeBuffer.encodeCachedValue(yDiff, 16,
                             clientCache_ -> polyTextCacheY);
          const unsigned char *end = inputMessage + inputLength - 1;
          const unsigned char *nextSrc = inputMessage + 16;
          while (nextSrc < end)
          {
            unsigned int textLength = (unsigned int) *nextSrc++;
            encodeBuffer.encodeBoolValue(1);
            encodeBuffer.encodeValue(textLength, 8);
            if (textLength == 255)
            {
              encodeBuffer.encodeCachedValue(GetULONG(nextSrc, 1), 29,
                                 clientCache_ -> polyTextFontCache);
              nextSrc += 4;
            }
            else
            {
              encodeBuffer.encodeCachedValue(*nextSrc++, 8,
                                 clientCache_ -> polyTextDeltaCache);

              if (control -> isProtoStep7() == 1)
              {
                encodeBuffer.encodeTextData(nextSrc, textLength);

                nextSrc += textLength;
              }
              else
              {
                clientCache_ -> polyTextTextCompressor.reset();
                for (unsigned int i = 0; i < textLength; i++)
                  clientCache_ -> polyTextTextCompressor.encodeChar(*nextSrc++, encodeBuffer);
              }
            }
          }
          encodeBuffer.encodeBoolValue(0);
        }
        break;
      case X_PolyText16:
        {
          #ifdef TARGETS

          unsigned int t_id = GetULONG(inputMessage + 4, bigEndian_);

          if (pixmaps.find(t_id) != pixmaps.end())
          {
            *logofs << "handleRead: X_PolyText16 target id is pixmap "
                    << t_id << ".\n" << logofs_flush;
          }
          else if (windows.find(t_id) != windows.end())
          {
            *logofs << "handleRead: X_PolyText16 target id is window "
                    << t_id << ".\n" << logofs_flush;
          }
          else
          {
            *logofs << "handleRead: X_PolyText16 target id " << t_id
                    << " is unrecognized.\n" << logofs_flush;
          }

          #endif

          MessageStore *messageStore = clientStore_ ->
                               getRequestStore(X_PolyText16);

          if (handleEncodeCached(encodeBuffer, clientCache_, messageStore,
                                     inputMessage, inputLength))
          {
            hit = 1;

            break;
          }

          encodeBuffer.encodeXidValue(GetULONG(inputMessage + 4,
                             bigEndian_), clientCache_ -> drawableCache);
          encodeBuffer.encodeXidValue(GetULONG(inputMessage + 8,
                             bigEndian_), clientCache_ -> gcCache);
          unsigned int x = GetUINT(inputMessage + 12, bigEndian_);
          int xDiff = x - clientCache_ -> polyTextLastX;
          clientCache_ -> polyTextLastX = x;
          encodeBuffer.encodeCachedValue(xDiff, 16,
                             clientCache_ -> polyTextCacheX);
          unsigned int y = GetUINT(inputMessage + 14, bigEndian_);
          int yDiff = y - clientCache_ -> polyTextLastY;
          clientCache_ -> polyTextLastY = y;
          encodeBuffer.encodeCachedValue(yDiff, 16,
                             clientCache_ -> polyTextCacheY);
          const unsigned char *end = inputMessage + inputLength - 1;
          const unsigned char *nextSrc = inputMessage + 16;
          while (nextSrc < end)
          {
            unsigned int textLength = (unsigned int) *nextSrc++;
            encodeBuffer.encodeBoolValue(1);
            encodeBuffer.encodeValue(textLength, 8);
            if (textLength == 255)
            {
              encodeBuffer.encodeCachedValue(GetULONG(nextSrc, 1), 29,
                                 clientCache_ -> polyTextFontCache);
              nextSrc += 4;
            }
            else
            {
              encodeBuffer.encodeCachedValue(*nextSrc++, 8,
                                 clientCache_ -> polyTextDeltaCache);

              if (control -> isProtoStep7() == 1)
              {
                encodeBuffer.encodeTextData(nextSrc, textLength * 2);

                nextSrc += textLength * 2;
              }
              else
              {
                clientCache_ -> polyTextTextCompressor.reset();
                for (unsigned int i = 0; i < textLength * 2; i++)
                  clientCache_ -> polyTextTextCompressor.encodeChar(*nextSrc++, encodeBuffer);
              }
            }
          }
          encodeBuffer.encodeBoolValue(0);
        }
        break;
      case X_ImageText8:
        {
          #ifdef TARGETS

          unsigned int t_id = GetULONG(inputMessage + 4, bigEndian_);

          if (pixmaps.find(t_id) != pixmaps.end())
          {
            *logofs << "handleRead: X_ImageText8 target id is pixmap "
                    << t_id << ".\n" << logofs_flush;
          }
          else if (windows.find(t_id) != windows.end())
          {
            *logofs << "handleRead: X_ImageText8 target id is window "
                    << t_id << ".\n" << logofs_flush;
          }
          else
          {
            *logofs << "handleRead: X_ImageText8 target id "
                    << t_id << " is unrecognized.\n"
                    << logofs_flush;
          }

          #endif

          MessageStore *messageStore = clientStore_ ->
                               getRequestStore(X_ImageText8);

          if (handleEncodeCached(encodeBuffer, clientCache_, messageStore,
                                     inputMessage, inputLength))
          {
            hit = 1;

            break;
          }

          unsigned int textLength = (unsigned int) inputMessage[1];
          encodeBuffer.encodeCachedValue(textLength, 8,
                             clientCache_ -> imageTextLengthCache, 4);
          encodeBuffer.encodeXidValue(GetULONG(inputMessage + 4,
                             bigEndian_), clientCache_ -> drawableCache);
          encodeBuffer.encodeXidValue(GetULONG(inputMessage + 8,
                             bigEndian_), clientCache_ -> gcCache);
          unsigned int x = GetUINT(inputMessage + 12, bigEndian_);
          int xDiff = x - clientCache_ -> imageTextLastX;
          clientCache_ -> imageTextLastX = x;
          encodeBuffer.encodeCachedValue(xDiff, 16,
                             clientCache_ -> imageTextCacheX);
          unsigned int y = GetUINT(inputMessage + 14, bigEndian_);
          int yDiff = y - clientCache_ -> imageTextLastY;
          clientCache_ -> imageTextLastY = y;
          encodeBuffer.encodeCachedValue(yDiff, 16,
                             clientCache_ -> imageTextCacheY);
          const unsigned char *nextSrc = inputMessage + 16;

          if (control -> isProtoStep7() == 1)
          {
            encodeBuffer.encodeTextData(nextSrc, textLength);
          }
          else
          {
            clientCache_ -> imageTextTextCompressor.reset();
            for (unsigned int j = 0; j < textLength; j++)
              clientCache_ -> imageTextTextCompressor.encodeChar(*nextSrc++, encodeBuffer);
          }
        }
        break;
      case X_ImageText16:
        {
          #ifdef TARGETS

          unsigned int t_id = GetULONG(inputMessage + 4, bigEndian_);

          if (pixmaps.find(t_id) != pixmaps.end())
          {
            *logofs << "handleRead: X_ImageText16 target id is pixmap "
                    << t_id << ".\n" << logofs_flush;
          }
          else if (windows.find(t_id) != windows.end())
          {
            *logofs << "handleRead: X_ImageText16 target id is window "
                    << t_id << ".\n" << logofs_flush;
          }
          else
          {
            *logofs << "handleRead: X_ImageText16 target id "
                    << t_id << " is unrecognized.\n"
                    << logofs_flush;
          }

          #endif

          MessageStore *messageStore = clientStore_ ->
                               getRequestStore(X_ImageText16);

          if (handleEncodeCached(encodeBuffer, clientCache_, messageStore,
                                     inputMessage, inputLength))
          {
            hit = 1;

            break;
          }

          unsigned int textLength = (unsigned int) inputMessage[1];
          encodeBuffer.encodeCachedValue(textLength, 8,
                             clientCache_ -> imageTextLengthCache, 4);
          encodeBuffer.encodeXidValue(GetULONG(inputMessage + 4,
                             bigEndian_), clientCache_ -> drawableCache);
          encodeBuffer.encodeXidValue(GetULONG(inputMessage + 8,
                             bigEndian_), clientCache_ -> gcCache);
          unsigned int x = GetUINT(inputMessage + 12, bigEndian_);
          int xDiff = x - clientCache_ -> imageTextLastX;
          clientCache_ -> imageTextLastX = x;
          encodeBuffer.encodeCachedValue(xDiff, 16,
                             clientCache_ -> imageTextCacheX);
          unsigned int y = GetUINT(inputMessage + 14, bigEndian_);
          int yDiff = y - clientCache_ -> imageTextLastY;
          clientCache_ -> imageTextLastY = y;
          encodeBuffer.encodeCachedValue(yDiff, 16,
                             clientCache_ -> imageTextCacheY);
          const unsigned char *nextSrc = inputMessage + 16;

          if (control -> isProtoStep7() == 1)
          {
            encodeBuffer.encodeTextData(nextSrc, textLength * 2);
          }
          else
          {
            clientCache_ -> imageTextTextCompressor.reset();
            for (unsigned int j = 0; j < textLength * 2; j++)
              clientCache_ -> imageTextTextCompressor.encodeChar(*nextSrc++, encodeBuffer);
          }
        }
        break;
      case X_InternAtom:
        {
          MessageStore *messageStore = clientStore_ ->
                               getRequestStore(X_InternAtom);

          if (handleEncodeCached(encodeBuffer, clientCache_, messageStore,
                                     inputMessage, inputLength))
          {
            sequenceQueue_.push(clientSequence_, inputOpcode);

            //
            // Set the priority, also if doing so will
            // penalize all the well written clients
            // using XInternAtoms() to pipeline multi-
            // ple replies.
            //

            priority_++;

            hit = 1;

            break;
          }

          unsigned int nameLength = GetUINT(inputMessage + 4, bigEndian_);
          encodeBuffer.encodeValue(nameLength, 16, 6);
          encodeBuffer.encodeBoolValue((unsigned int) inputMessage[1]);
          const unsigned char *nextSrc = inputMessage + 8;

          if (control -> isProtoStep7() == 1)
          {
            encodeBuffer.encodeTextData(nextSrc, nameLength);
          }
          else
          {
            clientCache_ -> internAtomTextCompressor.reset();
            for (unsigned int i = 0; i < nameLength; i++)
            {
              clientCache_ -> internAtomTextCompressor.encodeChar(*nextSrc++, encodeBuffer);
            }
          }

          sequenceQueue_.push(clientSequence_, inputOpcode);

          priority_++;
        }
        break;
      case X_ListExtensions:
        {
          sequenceQueue_.push(clientSequence_, inputOpcode);

          priority_++;
        }
        break;
      case X_ListFonts:
        {
          unsigned int textLength = GetUINT(inputMessage + 6, bigEndian_);
          encodeBuffer.encodeValue(textLength, 16, 6);
          encodeBuffer.encodeValue(GetUINT(inputMessage + 4, bigEndian_), 16, 6);
          const unsigned char* nextSrc = inputMessage + 8;

          if (control -> isProtoStep7() == 1)
          {
            encodeBuffer.encodeTextData(nextSrc, textLength);
          }
          else
          {
            clientCache_ -> polyTextTextCompressor.reset();
            for (unsigned int i = 0; i < textLength; i++)
            {
              clientCache_ -> polyTextTextCompressor.encodeChar(*nextSrc++, encodeBuffer);
            }
          }

          sequenceQueue_.push(clientSequence_, inputOpcode);

          priority_++;
        }
        break;
      case X_LookupColor:
      case X_AllocNamedColor:
        {
          unsigned int textLength = GetUINT(inputMessage + 8, bigEndian_);
          encodeBuffer.encodeValue(textLength, 16, 6);
          encodeBuffer.encodeCachedValue(GetULONG(inputMessage + 4, bigEndian_),
                                         29, clientCache_ -> colormapCache);
          const unsigned char *nextSrc = inputMessage + 12;

          if (control -> isProtoStep7() == 1)
          {
            encodeBuffer.encodeTextData(nextSrc, textLength);
          }
          else
          {
            clientCache_ -> polyTextTextCompressor.reset();
            for (unsigned int i = 0; i < textLength; i++)
            {
              clientCache_ -> polyTextTextCompressor.encodeChar(*nextSrc++, encodeBuffer);
            }
          }

          sequenceQueue_.push(clientSequence_, inputOpcode);

          priority_++;
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
          #ifdef TARGETS

          if (inputOpcode == X_DestroyWindow)
          {
            unsigned int w_id = GetULONG(inputMessage + 4, bigEndian_);

            *logofs << "handleRead: X_DestroyWindow id is "
                    << w_id << ".\n" << logofs_flush;

            windows.erase(w_id);
          }

          #endif

          if (inputOpcode == X_DestroyWindow && control -> isProtoStep7() == 1)
          {
            encodeBuffer.encodeFreeXidValue(GetULONG(inputMessage + 4, bigEndian_),
                               clientCache_ -> freeWindowCache);
          }
          else
          {
            encodeBuffer.encodeXidValue(GetULONG(inputMessage + 4, bigEndian_),
                               clientCache_ -> windowCache);
          }

          if ((inputOpcode == X_QueryPointer) ||
              (inputOpcode == X_GetWindowAttributes) ||
              (inputOpcode == X_QueryTree))
          {
            sequenceQueue_.push(clientSequence_, inputOpcode);

            priority_++;
          }
        }
        break;
      case X_OpenFont:
        {
          unsigned int nameLength = GetUINT(inputMessage + 8, bigEndian_);
          encodeBuffer.encodeValue(nameLength, 16, 7);
          unsigned int font = GetULONG(inputMessage + 4, bigEndian_);
          encodeBuffer.encodeValue(font - clientCache_ -> lastFont, 29, 5);
          clientCache_ -> lastFont = font;
          const unsigned char *nextSrc = inputMessage + 12;

          if (control -> isProtoStep7() == 1)
          {
            encodeBuffer.encodeTextData(nextSrc, nameLength);
          }
          else
          {
            clientCache_ -> openFontTextCompressor.reset();
            for (; nameLength; nameLength--)
            {
              clientCache_ -> openFontTextCompressor.
                    encodeChar(*nextSrc++, encodeBuffer);
            }
          }
        }
        break;
      case X_PolyFillRectangle:
        {
          #ifdef TARGETS

          unsigned int t_id = GetULONG(inputMessage + 4, bigEndian_);

          if (pixmaps.find(t_id) != pixmaps.end())
          {
            *logofs << "handleRead: X_PolyFillRectangle target id is pixmap "
                    << t_id << ".\n" << logofs_flush;
          }
          else if (windows.find(t_id) != windows.end())
          {
            *logofs << "handleRead: X_PolyFillRectangle target id is window "
                    << t_id << ".\n" << logofs_flush;
          }
          else
          {
            *logofs << "handleRead: X_PolyFillRectangle target id "
                    << t_id << " is unrecognized.\n"
                    << logofs_flush;
          }

          #endif

          MessageStore *messageStore = clientStore_ ->
                               getRequestStore(X_PolyFillRectangle);

          if (handleEncodeCached(encodeBuffer, clientCache_, messageStore,
                                     inputMessage, inputLength))
          {
            hit = 1;

            break;
          }

          encodeBuffer.encodeXidValue(GetULONG(inputMessage + 4,
                             bigEndian_), clientCache_ -> drawableCache);
          encodeBuffer.encodeXidValue(GetULONG(inputMessage + 8,
                             bigEndian_), clientCache_ -> gcCache);

          unsigned int index = 0;
          unsigned int lastX = 0, lastY = 0;
          unsigned int lastWidth = 0, lastHeight = 0;

          //
          // TODO: Could send the size at the beginning
          // instead of a bool at each iteration.
          //

          for (unsigned int i = 12; i < inputLength;)
          {
            unsigned int x = GetUINT(inputMessage + i, bigEndian_);
            unsigned int newX = x;
            x -= lastX;
            lastX = newX;
            encodeBuffer.encodeCachedValue(x, 16,
                         *clientCache_ -> polyFillRectangleCacheX[index], 8);
            i += 2;
            unsigned int y = GetUINT(inputMessage + i, bigEndian_);
            unsigned int newY = y;
            y -= lastY;
            lastY = newY;
            encodeBuffer.encodeCachedValue(y, 16,
                         *clientCache_ -> polyFillRectangleCacheY[index], 8);
            i += 2;
            unsigned int width = GetUINT(inputMessage + i, bigEndian_);
            unsigned int newWidth = width;
            width -= lastWidth;
            lastWidth = newWidth;
            encodeBuffer.encodeCachedValue(width, 16,
                         *clientCache_ -> polyFillRectangleCacheWidth[index], 8);
            i += 2;
            unsigned int height = GetUINT(inputMessage + i, bigEndian_);
            unsigned int newHeight = height;
            height -= lastHeight;
            lastHeight = newHeight;
            encodeBuffer.encodeCachedValue(height, 16,
                         *clientCache_ -> polyFillRectangleCacheHeight[index], 8);
            i += 2;

            if (++index == 4) index = 0;

            encodeBuffer.encodeBoolValue((i < inputLength) ? 1 : 0);
          }
        }
        break;
      case X_PolyFillArc:
        {
          #ifdef TARGETS

          unsigned int t_id = GetULONG(inputMessage + 4, bigEndian_);

          if (pixmaps.find(t_id) != pixmaps.end())
          {
            *logofs << "handleRead:  X_PolyFillArc target id is pixmap "
                    << t_id << ".\n" << logofs_flush;
          }
          else if (windows.find(t_id) != windows.end())
          {
            *logofs << "handleRead:  X_PolyFillArc target id is window "
                    << t_id << ".\n" << logofs_flush;
          }
          else
          {
            *logofs << "handleRead:  X_PolyFillArc target id " << t_id
                    << " is unrecognized.\n" << logofs_flush;
          }

          #endif

          MessageStore *messageStore = clientStore_ ->
                               getRequestStore(X_PolyFillArc);

          if (handleEncodeCached(encodeBuffer, clientCache_, messageStore,
                                     inputMessage, inputLength))
          {
            hit = 1;

            break;
          }

          encodeBuffer.encodeXidValue(GetULONG(inputMessage + 4,
                             bigEndian_), clientCache_ -> drawableCache);
          encodeBuffer.encodeXidValue(GetULONG(inputMessage + 8,
                             bigEndian_), clientCache_ -> gcCache);

          unsigned int index = 0;
          unsigned int lastX = 0, lastY = 0;
          unsigned int lastWidth = 0, lastHeight = 0;
          unsigned int lastAngle1 = 0, lastAngle2 = 0;

          //
          // TODO: Could send the size at the beginning
          // instead of a bool at each iteration.
          //

          for (unsigned int i = 12; i < inputLength;)
          {
            unsigned int x = GetUINT(inputMessage + i, bigEndian_);
            unsigned int newX = x;
            x -= lastX;
            lastX = newX;
            encodeBuffer.encodeCachedValue(x, 16,
                         *clientCache_ -> polyFillArcCacheX[index], 8);
            i += 2;
            unsigned int y = GetUINT(inputMessage + i, bigEndian_);
            unsigned int newY = y;
            y -= lastY;
            lastY = newY;
            encodeBuffer.encodeCachedValue(y, 16,
                         *clientCache_ -> polyFillArcCacheY[index], 8);
            i += 2;
            unsigned int width = GetUINT(inputMessage + i, bigEndian_);
            unsigned int newWidth = width;
            width -= lastWidth;
            lastWidth = newWidth;
            encodeBuffer.encodeCachedValue(width, 16,
                         *clientCache_ -> polyFillArcCacheWidth[index], 8);
            i += 2;
            unsigned int height = GetUINT(inputMessage + i, bigEndian_);
            unsigned int newHeight = height;
            height -= lastHeight;
            lastHeight = newHeight;
            encodeBuffer.encodeCachedValue(height, 16,
                         *clientCache_ -> polyFillArcCacheHeight[index], 8);
            i += 2;
            unsigned int angle1 = GetUINT(inputMessage + i, bigEndian_);
            unsigned int newAngle1 = angle1;
            angle1 -= lastAngle1;
            lastAngle1 = newAngle1;
            encodeBuffer.encodeCachedValue(angle1, 16,
                         *clientCache_ -> polyFillArcCacheAngle1[index], 8);
            i += 2;
            unsigned int angle2 = GetUINT(inputMessage + i, bigEndian_);
            unsigned int newAngle2 = angle2;
            angle2 -= lastAngle2;
            lastAngle2 = newAngle2;
            encodeBuffer.encodeCachedValue(angle2, 16,
                         *clientCache_ -> polyFillArcCacheAngle2[index], 8);
            i += 2;

            if (++index == 2) index = 0;

            encodeBuffer.encodeBoolValue((i < inputLength) ? 1 : 0);
          }
        }
        break;
      case X_PolyArc:
        {
          #ifdef TARGETS

          unsigned int t_id = GetULONG(inputMessage + 4, bigEndian_);

          if (pixmaps.find(t_id) != pixmaps.end())
          {
            *logofs << "handleRead: X_PolyArc target id is pixmap "
                    << t_id << ".\n" << logofs_flush;
          }
          else if (windows.find(t_id) != windows.end())
          {
            *logofs << "handleRead: X_PolyArc target id is window "
                    << t_id << ".\n" << logofs_flush;
          }
          else
          {
            *logofs << "handleRead: X_PolyArc target id " << t_id
                    << " is unrecognized.\n" << logofs_flush;
          }

          #endif

          MessageStore *messageStore = clientStore_ ->
                               getRequestStore(X_PolyArc);

          if (handleEncodeCached(encodeBuffer, clientCache_, messageStore,
                                     inputMessage, inputLength))
          {
            hit = 1;

            break;
          }

          encodeBuffer.encodeXidValue(GetULONG(inputMessage + 4,
                             bigEndian_), clientCache_ -> drawableCache);
          encodeBuffer.encodeXidValue(GetULONG(inputMessage + 8,
                             bigEndian_), clientCache_ -> gcCache);

          unsigned int index = 0;
          unsigned int lastX = 0, lastY = 0;
          unsigned int lastWidth = 0, lastHeight = 0;
          unsigned int lastAngle1 = 0, lastAngle2 = 0;

          //
          // TODO: Could send the size at the beginning
          // instead of a bool at each iteration.
          //

          for (unsigned int i = 12; i < inputLength;)
          {
            unsigned int x = GetUINT(inputMessage + i, bigEndian_);
            unsigned int newX = x;
            x -= lastX;
            lastX = newX;
            encodeBuffer.encodeCachedValue(x, 16,
                         *clientCache_ -> polyArcCacheX[index], 8);
            i += 2;
            unsigned int y = GetUINT(inputMessage + i, bigEndian_);
            unsigned int newY = y;
            y -= lastY;
            lastY = newY;
            encodeBuffer.encodeCachedValue(y, 16,
                         *clientCache_ -> polyArcCacheY[index], 8);
            i += 2;
            unsigned int width = GetUINT(inputMessage + i, bigEndian_);
            unsigned int newWidth = width;
            width -= lastWidth;
            lastWidth = newWidth;
            encodeBuffer.encodeCachedValue(width, 16,
                         *clientCache_ -> polyArcCacheWidth[index], 8);
            i += 2;
            unsigned int height = GetUINT(inputMessage + i, bigEndian_);
            unsigned int newHeight = height;
            height -= lastHeight;
            lastHeight = newHeight;
            encodeBuffer.encodeCachedValue(height, 16,
                         *clientCache_ -> polyArcCacheHeight[index], 8);
            i += 2;
            unsigned int angle1 = GetUINT(inputMessage + i, bigEndian_);
            unsigned int newAngle1 = angle1;
            angle1 -= lastAngle1;
            lastAngle1 = newAngle1;
            encodeBuffer.encodeCachedValue(angle1, 16,
                         *clientCache_ -> polyArcCacheAngle1[index], 8);
            i += 2;
            unsigned int angle2 = GetUINT(inputMessage + i, bigEndian_);
            unsigned int newAngle2 = angle2;
            angle2 -= lastAngle2;
            lastAngle2 = newAngle2;
            encodeBuffer.encodeCachedValue(angle2, 16,
                         *clientCache_ -> polyArcCacheAngle2[index], 8);
            i += 2;

            if (++index == 2) index = 0;

            encodeBuffer.encodeBoolValue((i < inputLength) ? 1 : 0);
          }
        }
        break;
      case X_PolyPoint:
        {
          #ifdef TARGETS

          unsigned int t_id = GetULONG(inputMessage + 4, bigEndian_);

          if (pixmaps.find(t_id) != pixmaps.end())
          {
            *logofs << "handleRead: X_PolyPoint target id is pixmap "
                    << t_id << ".\n" << logofs_flush;
          }
          else if (windows.find(t_id) != windows.end())
          {
            *logofs << "handleRead: X_PolyPoint target id is window "
                    << t_id << ".\n" << logofs_flush;
          }
          else
          {
            *logofs << "handleRead: X_PolyPoint target id " << t_id
                    << " is unrecognized.\n" << logofs_flush;
          }

          #endif

          MessageStore *messageStore = clientStore_ ->
                               getRequestStore(X_PolyPoint);

          if (handleEncodeCached(encodeBuffer, clientCache_, messageStore,
                                     inputMessage, inputLength))
          {
            hit = 1;

            break;
          }

          encodeBuffer.encodeValue(GetUINT(inputMessage + 2, bigEndian_) - 3, 16, 4);
          encodeBuffer.encodeBoolValue((unsigned int) inputMessage[1]);
          encodeBuffer.encodeXidValue(GetULONG(inputMessage + 4, bigEndian_),
                             clientCache_ -> drawableCache);
          encodeBuffer.encodeXidValue(GetULONG(inputMessage + 8, bigEndian_),
                             clientCache_ -> gcCache);
          const unsigned char *nextSrc = inputMessage + 12;

          unsigned int index = 0;
          unsigned int lastX = 0, lastY = 0;

          for (unsigned int i = 12; i < inputLength; i += 4)
          {
            unsigned int x = GetUINT(nextSrc, bigEndian_);
            nextSrc += 2;
            unsigned int tmp = x;
            x -= lastX;
            lastX = tmp;
            encodeBuffer.encodeCachedValue(x, 16,
                               *clientCache_ -> polyPointCacheX[index], 8);
            unsigned int y = GetUINT(nextSrc, bigEndian_);
            nextSrc += 2;
            tmp = y;
            y -= lastY;
            lastY = tmp;
            encodeBuffer.encodeCachedValue(y, 16,
                               *clientCache_ -> polyPointCacheY[index], 8);

            if (++index == 2) index = 0;
          }
        }
        break;
      case X_PolyLine:
        {
          #ifdef TARGETS

          unsigned int t_id = GetULONG(inputMessage + 4, bigEndian_);

          if (pixmaps.find(t_id) != pixmaps.end())
          {
            *logofs << "handleRead: X_PolyLine target id is pixmap "
                    << t_id << ".\n" << logofs_flush;
          }
          else if (windows.find(t_id) != windows.end())
          {
            *logofs << "handleRead: X_PolyLine target id is window "
                    << t_id << ".\n" << logofs_flush;
          }
          else
          {
            *logofs << "handleRead: X_PolyLine target id " << t_id
                    << " is unrecognized.\n" << logofs_flush;
          }

          #endif

          MessageStore *messageStore = clientStore_ ->
                               getRequestStore(X_PolyLine);

          if (handleEncodeCached(encodeBuffer, clientCache_, messageStore,
                                     inputMessage, inputLength))
          {
            hit = 1;

            break;
          }

          encodeBuffer.encodeValue(GetUINT(inputMessage + 2, bigEndian_) - 3, 16, 4);
          encodeBuffer.encodeBoolValue((unsigned int) inputMessage[1]);
          encodeBuffer.encodeXidValue(GetULONG(inputMessage + 4,
                             bigEndian_), clientCache_ -> drawableCache);
          encodeBuffer.encodeXidValue(GetULONG(inputMessage + 8,
                             bigEndian_), clientCache_ -> gcCache);
          const unsigned char *nextSrc = inputMessage + 12;

          unsigned int index = 0;
          unsigned int lastX = 0, lastY = 0;

          for (unsigned int i = 12; i < inputLength; i += 4)
          {
            unsigned int x = GetUINT(nextSrc, bigEndian_);
            nextSrc += 2;
            unsigned int tmp = x;
            x -= lastX;
            lastX = tmp;
            encodeBuffer.encodeCachedValue(x, 16,
                               *clientCache_ -> polyLineCacheX[index], 8);
            unsigned int y = GetUINT(nextSrc, bigEndian_);
            nextSrc += 2;
            tmp = y;
            y -= lastY;
            lastY = tmp;
            encodeBuffer.encodeCachedValue(y, 16,
                               *clientCache_ -> polyLineCacheY[index], 8);

            if (++index == 2) index = 0;
          }
        }
        break;
      case X_PolyRectangle:
        {
          encodeBuffer.encodeValue((GetUINT(inputMessage + 2,
                                            bigEndian_) - 3) >> 1, 16, 3);
          encodeBuffer.encodeXidValue(GetULONG(inputMessage + 4,
                             bigEndian_), clientCache_ -> drawableCache);
          encodeBuffer.encodeXidValue(GetULONG(inputMessage + 8,
                             bigEndian_), clientCache_ -> gcCache);
          const unsigned char *end = inputMessage + inputLength;
          const unsigned char *nextSrc = inputMessage + 12;
          while (nextSrc < end)
          {
            for (unsigned int i = 0; i < 4; i++)
            {
              encodeBuffer.encodeCachedValue(GetUINT(nextSrc, bigEndian_), 16,
                                 *clientCache_ -> polyRectangleGeomCache[i], 8);
              nextSrc += 2;
            }
          }
        }
        break;
      case X_PolySegment:
        {
          #ifdef TARGETS

          unsigned int t_id = GetULONG(inputMessage + 4, bigEndian_);

          if (pixmaps.find(t_id) != pixmaps.end())
          {
            *logofs << "handleRead: X_PolySegment target id is pixmap "
                    << t_id << ".\n" << logofs_flush;
          }
          else if (windows.find(t_id) != windows.end())
          {
            *logofs << "handleRead: X_PolySegment target id is window "
                    << t_id << ".\n" << logofs_flush;
          }
          else
          {
            *logofs << "handleRead: X_PolySegment target id " << t_id
                    << " is unrecognized.\n" << logofs_flush;
          }

          #endif

          MessageStore *messageStore = clientStore_ ->
                               getRequestStore(X_PolySegment);

          if (handleEncodeCached(encodeBuffer, clientCache_, messageStore,
                                     inputMessage, inputLength))
          {
            hit = 1;

            break;
          }

          encodeBuffer.encodeValue((GetUINT(inputMessage + 2,
                                            bigEndian_) - 3) >> 1, 16, 4);
          encodeBuffer.encodeXidValue(GetULONG(inputMessage + 4,
                             bigEndian_), clientCache_ -> drawableCache);
          encodeBuffer.encodeXidValue(GetULONG(inputMessage + 8,
                             bigEndian_), clientCache_ -> gcCache);
          const unsigned char *end = inputMessage + inputLength;
          const unsigned char *nextSrc = inputMessage + 12;
          // unsigned int index = 0;
          // unsigned int lastX1, lastY1, lastX2, lastY2;
          while (nextSrc < end)
          {
            unsigned int x = GetUINT(nextSrc, bigEndian_);
            nextSrc += 2;
            unsigned int xDiff0 =
            x - clientCache_ -> polySegmentLastX[0];
            unsigned int xDiff1 =
            x - clientCache_ -> polySegmentLastX[1];
            int xDiff0Abs = (int) xDiff0;
            if (xDiff0Abs < 0)
              xDiff0Abs = -xDiff0Abs;
            int xDiff1Abs = (int) xDiff1;
            if (xDiff1Abs < 0)
              xDiff1Abs = -xDiff1Abs;

            unsigned int y = GetUINT(nextSrc, bigEndian_);
            nextSrc += 2;
            unsigned int yDiff0 =
            y - clientCache_ -> polySegmentLastY[0];
            unsigned int yDiff1 =
            y - clientCache_ -> polySegmentLastY[1];
            int yDiff0Abs = (int) yDiff0;
            if (yDiff0Abs < 0)
              yDiff0Abs = -yDiff0Abs;
            int yDiff1Abs = (int) yDiff1;
            if (yDiff1Abs < 0)
              yDiff1Abs = -yDiff1Abs;

            int diff0 = xDiff0Abs + yDiff0Abs;
            int diff1 = xDiff1Abs + yDiff1Abs;
            if (diff0 < diff1)
            {
              encodeBuffer.encodeBoolValue(0);
              encodeBuffer.encodeCachedValue(xDiff0, 16,
                                         clientCache_ -> polySegmentCacheX, 6);
              encodeBuffer.encodeCachedValue(yDiff0, 16,
                                         clientCache_ -> polySegmentCacheY, 6);
            }
            else
            {
              encodeBuffer.encodeBoolValue(1);
              encodeBuffer.encodeCachedValue(xDiff1, 16,
                                         clientCache_ -> polySegmentCacheX, 6);
              encodeBuffer.encodeCachedValue(yDiff1, 16,
                                         clientCache_ -> polySegmentCacheY, 6);
            }

            clientCache_ -> polySegmentLastX[clientCache_ -> polySegmentCacheIndex] = x;
            clientCache_ -> polySegmentLastY[clientCache_ -> polySegmentCacheIndex] = y;

            clientCache_ -> polySegmentCacheIndex =
                  clientCache_ -> polySegmentCacheIndex == 1 ? 0 : 1;
          }
        }
        break;
      case X_PutImage:
        {
          #ifdef TARGETS

          unsigned int t_id = GetULONG(inputMessage + 4, bigEndian_);

          if (pixmaps.find(t_id) != pixmaps.end())
          {
            *logofs << "handleRead: X_PutImage target id is pixmap "
                    << t_id << ".\n" << logofs_flush;
          }
          else if (windows.find(t_id) != windows.end())
          {
            *logofs << "handleRead: X_PutImage target id is window "
                    << t_id << ".\n" << logofs_flush;
          }
          else
          {
            *logofs << "handleRead: X_PutImage target id " << t_id
                    << " is unrecognized.\n" << logofs_flush;
          }

          #endif

          MessageStore *messageStore = clientStore_ ->
                               getRequestStore(X_PutImage);

          hit = handleEncode(encodeBuffer, clientCache_, messageStore,
                                 inputOpcode, inputMessage, inputLength);
        }
        break;
      case X_QueryBestSize:
        {
          encodeBuffer.encodeValue((unsigned int)inputMessage[1], 2);
          encodeBuffer.encodeXidValue(GetULONG(inputMessage + 4,
                             bigEndian_), clientCache_ -> drawableCache);
          encodeBuffer.encodeValue(GetUINT(inputMessage + 8, bigEndian_), 16, 8);
          encodeBuffer.encodeValue(GetUINT(inputMessage + 10, bigEndian_), 16, 8);

          sequenceQueue_.push(clientSequence_, inputOpcode);

          priority_++;
        }
        break;
      case X_QueryColors:
        {
          // Differential encoding.
          encodeBuffer.encodeBoolValue(1);

          unsigned int numColors = ((inputLength - 8) >> 2);
          encodeBuffer.encodeValue(numColors, 16, 5);
          encodeBuffer.encodeCachedValue(GetULONG(inputMessage + 4, bigEndian_), 29,
                                         clientCache_ -> colormapCache);
          const unsigned char *nextSrc = inputMessage + 8;
          unsigned int predictedPixel = clientCache_ -> queryColorsLastPixel;
          for (unsigned int i = 0; i < numColors; i++)
          {
            unsigned int pixel = GetULONG(nextSrc, bigEndian_);
            nextSrc += 4;
            if (pixel == predictedPixel)
              encodeBuffer.encodeBoolValue(1);
            else
            {
              encodeBuffer.encodeBoolValue(0);
              encodeBuffer.encodeValue(pixel, 32, 9);
            }
            if (i == 0)
              clientCache_ -> queryColorsLastPixel = pixel;
            predictedPixel = pixel + 1;
          }

          sequenceQueue_.push(clientSequence_, inputOpcode);

          priority_++;
        }
        break;
      case X_QueryExtension:
        {
          #ifdef TEST

          char data[256];

          int length = GetUINT(inputMessage + 4, bigEndian_);

          if (length > 256)
          {
            length = 256;
          }

          strncpy(data, (char *) inputMessage + 8, length);

          *(data + length) = '\0';

          *logofs << "handleRead: Going to query extension '"
                  << data << "' for FD#" << fd_ << ".\n"
                  << logofs_flush;
          #endif

          unsigned int nameLength = GetUINT(inputMessage + 4, bigEndian_);
          encodeBuffer.encodeValue(nameLength, 16, 6);
          const unsigned char *nextSrc = inputMessage + 8;

          for (; nameLength; nameLength--)
          {
            encodeBuffer.encodeValue((unsigned int) *nextSrc++, 8);
          }

          unsigned int extension = 0;

          if (strncmp((char *) inputMessage + 8, "SHAPE", 5) == 0)
          {
            extension = X_NXInternalShapeExtension;
          }
          else if (strncmp((char *) inputMessage + 8, "RENDER", 6) == 0)
          {
            extension = X_NXInternalRenderExtension;
          }

          sequenceQueue_.push(clientSequence_, inputOpcode, extension);

          priority_++;
        }
        break;
      case X_QueryFont:
        {
          unsigned int font = GetULONG(inputMessage + 4, bigEndian_);
          encodeBuffer.encodeValue(font - clientCache_ -> lastFont, 29, 5);
          clientCache_ -> lastFont = font;

          sequenceQueue_.push(clientSequence_, inputOpcode);

          priority_++;
        }
        break;
      case X_SetClipRectangles:
        {
          MessageStore *messageStore = clientStore_ ->
                               getRequestStore(X_SetClipRectangles);

          if (handleEncodeCached(encodeBuffer, clientCache_, messageStore,
                                     inputMessage, inputLength))
          {
            hit = 1;

            break;
          }

          unsigned int numRectangles = ((inputLength - 12) >> 3);

          if (control -> isProtoStep9() == 1)
          {
            encodeBuffer.encodeValue(numRectangles, 15, 4);
          }
          else
          {
            encodeBuffer.encodeValue(numRectangles, 13, 4);
          }

          encodeBuffer.encodeValue((unsigned int) inputMessage[1], 2);
          encodeBuffer.encodeXidValue(GetULONG(inputMessage + 4, bigEndian_),
                             clientCache_ -> gcCache);
          encodeBuffer.encodeCachedValue(GetUINT(inputMessage + 8, bigEndian_), 16,
                             clientCache_ -> setClipRectanglesXCache, 8);
          encodeBuffer.encodeCachedValue(GetUINT(inputMessage + 10, bigEndian_), 16,
                             clientCache_ -> setClipRectanglesYCache, 8);
          const unsigned char *nextSrc = inputMessage + 12;
          for (unsigned int i = 0; i < numRectangles; i++)
          {
            for (unsigned int j = 0; j < 4; j++)
            {
              encodeBuffer.encodeCachedValue(GetUINT(nextSrc, bigEndian_), 16,
                                 *clientCache_ -> setClipRectanglesGeomCache[j], 8);
              nextSrc += 2;
            }
          }
        }
        break;
      case X_SetDashes:
        {
          unsigned int numDashes = GetUINT(inputMessage + 10, bigEndian_);
          encodeBuffer.encodeCachedValue(numDashes, 16,
                             clientCache_ -> setDashesLengthCache, 5);
          encodeBuffer.encodeXidValue(GetULONG(inputMessage + 4, bigEndian_),
                             clientCache_ -> gcCache);
          encodeBuffer.encodeCachedValue(GetUINT(inputMessage + 8, bigEndian_), 16,
                             clientCache_ -> setDashesOffsetCache, 5);
          const unsigned char *nextSrc = inputMessage + 12;
          for (unsigned int i = 0; i < numDashes; i++)
            encodeBuffer.encodeCachedValue(*nextSrc++, 8,
                                clientCache_ -> setDashesDashCache_[i & 1], 5);
        }
        break;
      case X_SetSelectionOwner:
        {
          encodeBuffer.encodeCachedValue(GetULONG(inputMessage + 4, bigEndian_), 29,
                                    clientCache_ -> setSelectionOwnerCache, 9);
          encodeBuffer.encodeCachedValue(GetULONG(inputMessage + 8, bigEndian_), 29,
                           clientCache_ -> getSelectionOwnerSelectionCache, 9);
          encodeBuffer.encodeCachedValue(GetULONG(inputMessage + 12, bigEndian_), 32,
                           clientCache_ -> setSelectionOwnerTimestampCache, 9);
        }
        break;
      case X_TranslateCoords:
        {
          #ifdef TARGETS

          unsigned int t_id = GetULONG(inputMessage + 4, bigEndian_);

          if (pixmaps.find(t_id) != pixmaps.end())
          {
            *logofs << "handleRead: X_TranslateCoords source id is pixmap "
                    << t_id << ".\n" << logofs_flush;
          }
          else if (windows.find(t_id) != windows.end())
          {
            *logofs << "handleRead: X_TranslateCoords source id is window "
                    << t_id << ".\n" << logofs_flush;
          }
          else
          {
            *logofs << "handleRead: X_TranslateCoords source id " << t_id
                    << " is unrecognized.\n" << logofs_flush;
          }

          t_id = GetULONG(inputMessage + 8, bigEndian_);

          if (pixmaps.find(t_id) != pixmaps.end())
          {
            *logofs << "handleRead: X_TranslateCoords target id is pixmap "
                    << t_id << ".\n" << logofs_flush;
          }
          else if (windows.find(t_id) != windows.end())
          {
            *logofs << "handleRead: X_TranslateCoords target id is window "
                    << t_id << ".\n" << logofs_flush;
          }
          else
          {
            *logofs << "handleRead: X_TranslateCoords target id " << t_id
                    << " is unrecognized.\n" << logofs_flush;
          }

          #endif

          MessageStore *messageStore = clientStore_ ->
                               getRequestStore(X_TranslateCoords);

          if (handleEncodeCached(encodeBuffer, clientCache_, messageStore,
                                     inputMessage, inputLength))
          {
            sequenceQueue_.push(clientSequence_, inputOpcode);

            priority_++;

            hit = 1;

            break;
          }

          encodeBuffer.encodeCachedValue(GetULONG(inputMessage + 4, bigEndian_), 29,
                                   clientCache_ -> translateCoordsSrcCache, 9);
          encodeBuffer.encodeCachedValue(GetULONG(inputMessage + 8, bigEndian_), 29,
                                   clientCache_ -> translateCoordsDstCache, 9);
          encodeBuffer.encodeCachedValue(GetUINT(inputMessage + 12, bigEndian_), 16,
                                   clientCache_ -> translateCoordsXCache, 8);
          encodeBuffer.encodeCachedValue(GetUINT(inputMessage + 14, bigEndian_), 16,
                                   clientCache_ -> translateCoordsYCache, 8);

          sequenceQueue_.push(clientSequence_, inputOpcode);

          priority_++;
        }
        break;
      case X_GetImage:
        {
          #ifdef TARGETS

          unsigned int t_id = GetULONG(inputMessage + 4, bigEndian_);

          if (pixmaps.find(t_id) != pixmaps.end())
          {
            *logofs << "handleRead: X_GetImage source id is pixmap "
                    << t_id << ".\n" << logofs_flush;
          }
          else if (windows.find(t_id) != windows.end())
          {
            *logofs << "handleRead: X_GetImage source id is window "
                    << t_id << ".\n" << logofs_flush;
          }
          else
          {
            *logofs << "handleRead: X_GetImage source id " << t_id
                    << " is unrecognized.\n" << logofs_flush;
          }

          #endif

          MessageStore *messageStore = clientStore_ ->
                               getRequestStore(X_GetImage);

          if (handleEncodeCached(encodeBuffer, clientCache_, messageStore,
                                     inputMessage, inputLength))
          {
            sequenceQueue_.push(clientSequence_, inputOpcode);

            priority_++;

            hit = 1;

            break;
          }

          // Format.
          encodeBuffer.encodeValue((unsigned int) inputMessage[1], 2);
          // Drawable.
          encodeBuffer.encodeXidValue(GetULONG(inputMessage + 4,
                             bigEndian_), clientCache_ -> drawableCache);
          // X.
          unsigned int x = GetUINT(inputMessage + 8, bigEndian_);
          int xDiff = x - clientCache_ -> putImageLastX;
          clientCache_ -> putImageLastX = x;
          encodeBuffer.encodeCachedValue(xDiff, 16,
                             clientCache_ -> putImageXCache, 8);
          // Y.
          unsigned int y = GetUINT(inputMessage + 10, bigEndian_);
          int yDiff = y - clientCache_ -> putImageLastY;
          clientCache_ -> putImageLastY = y;
          encodeBuffer.encodeCachedValue(yDiff, 16,
                             clientCache_ -> putImageYCache, 8);
          // Width.
          unsigned int width = GetUINT(inputMessage + 12, bigEndian_);
          encodeBuffer.encodeCachedValue(width, 16,
                             clientCache_ -> putImageWidthCache, 8);
          // Height.
          unsigned int height = GetUINT(inputMessage + 14, bigEndian_);
          encodeBuffer.encodeCachedValue(height, 16,
                             clientCache_ -> putImageHeightCache, 8);
          // Plane mask.
          encodeBuffer.encodeCachedValue(GetULONG(inputMessage + 16, bigEndian_), 32,
                             clientCache_ -> getImagePlaneMaskCache, 5);

          sequenceQueue_.push(clientSequence_, inputOpcode);

          priority_++;
        }
        break;
      case X_GetPointerMapping:
        {
          sequenceQueue_.push(clientSequence_, inputOpcode);

          priority_++;
        }
        break;
      case X_GetKeyboardControl:
        {
          sequenceQueue_.push(clientSequence_, inputOpcode);

          priority_++;
        }
        break;
      default:
        {
          if (inputOpcode == opcodeStore_ -> renderExtension)
          {
            MessageStore *messageStore = clientStore_ ->
                                 getRequestStore(X_NXInternalRenderExtension);

            hit = handleEncode(encodeBuffer, clientCache_, messageStore,
                                   inputOpcode, inputMessage, inputLength);
          }
          else if (inputOpcode == opcodeStore_ -> shapeExtension)
          {
            MessageStore *messageStore = clientStore_ ->
                                 getRequestStore(X_NXInternalShapeExtension);

            hit = handleEncode(encodeBuffer, clientCache_, messageStore,
                                   inputOpcode, inputMessage, inputLength);
          }
          else if (inputOpcode == opcodeStore_ -> putPackedImage)
          {
            #ifdef TARGETS

            unsigned int t_id = GetULONG(inputMessage + 4, bigEndian_);

            if (pixmaps.find(t_id) != pixmaps.end())
            {
              *logofs << "handleRead: X_NXPutPackedImage target id is pixmap "
                      << t_id << ".\n" << logofs_flush;
            }
            else if (windows.find(t_id) != windows.end())
            {
              *logofs << "handleRead: X_NXPutPackedImage target id is window "
                      << t_id << ".\n" << logofs_flush;
            }
            else
            {
              *logofs << "handleRead: X_NXPutPackedImage target id " << t_id
                      << " is unrecognized.\n" << logofs_flush;
            }

            #endif

            #ifdef DEBUG
            *logofs << "handleRead: Encoding packed image request for FD#"
                    << fd_ << ".\n" << logofs_flush;
            #endif

            //
            // The field carries the destination data
            // length. We add the request's size of
            // the final X_PutImage.
            //

            unsigned int outputLength = GetULONG(inputMessage + 20, bigEndian_) + 24;

            statistics -> addPackedBytesIn(inputLength);

            statistics -> addPackedBytesOut(outputLength);

            MessageStore *messageStore = clientStore_ ->
                                 getRequestStore(X_NXPutPackedImage);

            hit = handleEncode(encodeBuffer, clientCache_, messageStore,
                                   inputOpcode, inputMessage, inputLength);
          }
          else if (inputOpcode == opcodeStore_ -> setUnpackColormap)
          {
            #ifdef DEBUG
            *logofs << "handleRead: Encoding set unpack colormap request "
                    << "for FD#" << fd_ << " with size " << inputLength
                    << ".\n" << logofs_flush;
            #endif

            MessageStore *messageStore = clientStore_ ->
                                 getRequestStore(X_NXSetUnpackColormap);

            hit = handleEncode(encodeBuffer, clientCache_, messageStore,
                                   inputOpcode, inputMessage, inputLength);
          }
          else if (inputOpcode == opcodeStore_ -> setUnpackAlpha)
          {
            #ifdef DEBUG
            *logofs << "handleRead: Encoding set unpack alpha request "
                    << "for FD#" << fd_ << " with size " << inputLength
                    << ".\n" << logofs_flush;
            #endif

            MessageStore *messageStore = clientStore_ ->
                                 getRequestStore(X_NXSetUnpackAlpha);

            hit = handleEncode(encodeBuffer, clientCache_, messageStore,
                                   inputOpcode, inputMessage, inputLength);
          }
          else if (inputOpcode == opcodeStore_ -> setUnpackGeometry)
          {
            #ifdef DEBUG
            *logofs << "handleRead: Encoding set unpack geometry request "
                    << "for FD#" << fd_ << " with size " << inputLength
                    << ".\n" << logofs_flush;
            #endif

            MessageStore *messageStore = clientStore_ ->
                                 getRequestStore(X_NXSetUnpackGeometry);

            hit = handleEncode(encodeBuffer, clientCache_, messageStore,
                                   inputOpcode, inputMessage, inputLength);
          }
          else if (inputOpcode == opcodeStore_ -> startSplit)
          {
            if (handleStartSplitRequest(encodeBuffer, inputOpcode,
                                            inputMessage, inputLength) < 0)
            {
              return -1;
            }
          }
          else if (inputOpcode == opcodeStore_ -> endSplit)
          {
            if (handleEndSplitRequest(encodeBuffer, inputOpcode,
                                          inputMessage, inputLength) < 0)
            {
              return -1;
            }
          }
          else if (inputOpcode == opcodeStore_ -> commitSplit)
          {
            if (handleCommitSplitRequest(encodeBuffer, inputOpcode,
                                             inputMessage, inputLength) < 0)
            {
              return -1;
            }
          }
          else if (inputOpcode == opcodeStore_ -> abortSplit)
          {
            if (handleAbortSplitRequest(encodeBuffer, inputOpcode,
                                            inputMessage, inputLength) < 0)
            {
              return -1;
            }
          }
          else if (inputOpcode == opcodeStore_ -> finishSplit)
          {
            if (handleFinishSplitRequest(encodeBuffer, inputOpcode,
                                          inputMessage, inputLength) < 0)
            {
              return -1;
            }
          }
          else if (inputOpcode == opcodeStore_ -> freeSplit)
          {
            #ifdef DEBUG
            *logofs << "handleRead: Encoding free split request "
                    << "for FD#" << fd_ << " with size " << inputLength
                    << ".\n" << logofs_flush;
            #endif

            encodeBuffer.encodeCachedValue(*(inputMessage + 1), 8,
                         clientCache_ -> resourceCache);
          }
          else if (inputOpcode == opcodeStore_ -> freeUnpack)
          {
            #ifdef DEBUG
            *logofs << "handleRead: Encoding free unpack request "
                    << "for FD#" << fd_ << " with size " << inputLength
                    << ".\n" << logofs_flush;
            #endif

            encodeBuffer.encodeCachedValue(*(inputMessage + 1), 8,
                         clientCache_ -> resourceCache);
          }
          else if (inputOpcode == opcodeStore_ -> getControlParameters)
          {
            #ifdef DEBUG
            *logofs << "handleRead: Encoding get control parameters "
                    << "request for FD#" << fd_ << " with size "
                    << inputLength << ".\n" << logofs_flush;
            #endif

            //
            // Add the reply to the write buffer. If found
            // to contain a message, it it will be flushed
            // to the X client before leaving the loop.
            //

            unsigned char *reply = writeBuffer_.addMessage(32);

            *(reply + 0) = X_Reply;

            PutUINT(clientSequence_, reply + 2, bigEndian_);

            PutULONG(0, reply + 4, bigEndian_);

            //
            // Save the sequence number we used
            // to auto-generate this reply.
            //

            lastSequence_ = clientSequence_;

            #ifdef TEST
            *logofs << "handleRead: Registered " << lastSequence_
                    << " as last auto-generated sequence number.\n"
                    << logofs_flush;
            #endif

            *(reply + 1) = control -> LinkMode;

            *(reply + 8)  = control -> LocalVersionMajor;
            *(reply + 9)  = control -> LocalVersionMinor;
            *(reply + 10) = control -> LocalVersionPatch;

            *(reply + 11) = control -> RemoteVersionMajor;
            *(reply + 12) = control -> RemoteVersionMinor;
            *(reply + 13) = control -> RemoteVersionPatch;

            PutUINT(control -> SplitTimeout,  reply + 14, bigEndian_);
            PutUINT(control -> MotionTimeout, reply + 16, bigEndian_);

            *(reply + 18) = control -> SplitMode;

            PutULONG(control -> SplitDataThreshold, reply + 20, bigEndian_);

            *(reply + 24) = control -> PackMethod;
            *(reply + 25) = control -> PackQuality;

            *(reply + 26) = control -> LocalDataCompressionLevel;
            *(reply + 27) = control -> LocalStreamCompressionLevel;
            *(reply + 28) = control -> LocalDeltaCompression;

            *(reply + 29) = (control -> LocalDeltaCompression == 1 &&
                                 control -> PersistentCacheEnableLoad == 1);
            *(reply + 30) = (control -> LocalDeltaCompression == 1 &&
                                 control -> PersistentCacheEnableSave == 1);
            *(reply + 31) = (control -> LocalDeltaCompression == 1 &&
                                 control -> PersistentCacheEnableLoad == 1 &&
                                     control -> PersistentCacheName != NULL);

            if (handleFlush(flush_if_any) < 0)
            {
              return -1;
            }
          }
          else if (inputOpcode == opcodeStore_ -> getCleanupParameters)
          {
            #ifdef WARNING
            *logofs << "handleRead: WARNING! Encoding fake get cleanup "
                    << "parameters request for FD#" << fd_ << " with size "
                    << inputLength << ".\n" << logofs_flush;
            #endif
          }
          else if (inputOpcode == opcodeStore_ -> getImageParameters)
          {
            #ifdef WARNING
            *logofs << "handleRead: WARNING! Encoding fake get cleanup "
                    << "parameters request for FD#" << fd_ << " with size "
                    << inputLength << ".\n" << logofs_flush;
            #endif
          }
          else if (inputOpcode == opcodeStore_ -> getUnpackParameters)
          {
            #ifdef DEBUG
            *logofs << "handleRead: Encoding get unpack parameters "
                    << "request for FD#" << fd_ << " with size "
                    << inputLength << ".\n" << logofs_flush;
            #endif

            sequenceQueue_.push(clientSequence_, inputOpcode);
          }
          else if (inputOpcode == opcodeStore_ -> getShmemParameters)
          {
            if (handleShmemRequest(encodeBuffer, inputOpcode,
                                       inputMessage, inputLength) < 0)
            {
              return -1;
            }
          }
          else if (inputOpcode == opcodeStore_ -> setExposeParameters)
          {
            //
            // Enable or disable expose events
            // coming from the real server.
            //

            encodeBuffer.encodeBoolValue(*(inputMessage + 4));
            encodeBuffer.encodeBoolValue(*(inputMessage + 5));
            encodeBuffer.encodeBoolValue(*(inputMessage + 6));
          }
          else if (inputOpcode == opcodeStore_ -> setCacheParameters)
          {
            if (handleCacheRequest(encodeBuffer, inputOpcode,
                                       inputMessage, inputLength) < 0)
            {
              return -1;
            }
          }
          else if (inputOpcode == opcodeStore_ -> getFontParameters)
          {
            if (handleFontRequest(encodeBuffer, inputOpcode,
                                      inputMessage, inputLength) < 0)
            {
              return -1;
            }
          }
          else
          {
            MessageStore *messageStore = clientStore_ ->
                                 getRequestStore(X_NXInternalGenericRequest);

            hit = handleEncode(encodeBuffer, clientCache_, messageStore,
                                   inputOpcode, inputMessage, inputLength);

            //
            // Don't flush if the opcode is unrecognized.
            // We may optionally flush it is an extension
            // but would penalize the well written clients.
            //
            // if (inputOpcode > 127)
            // {
            //   priority_++;
            // }
            //
          }
        }
      } // End of switch on opcode.

      int bits = encodeBuffer.diffBits();

      #if defined(TEST) || defined(OPCODES)

      const char *cacheString = (hit ? "cached " : "");

      *logofs << "handleRead: Handled " << cacheString << "request OPCODE#" 
              << (unsigned int) inputOpcode << " (" << DumpOpcode(inputOpcode)
              << ")" << " for FD#" << fd_ << " sequence " << clientSequence_
              << ". " << inputLength  << " bytes in, " << bits << " bits ("
              << ((float) bits) / 8 << " bytes) out.\n" << logofs_flush;

      #endif

      if (hit)
      {
        statistics -> addCachedRequest(inputOpcode);
      }

      statistics -> addRequestBits(inputOpcode, inputLength << 3, bits);

      if (inputOpcode == opcodeStore_ -> renderExtension)
      {
        if (hit)
        {
          statistics -> addRenderCachedRequest(*(inputMessage + 1));
        }

        statistics -> addRenderRequestBits(*(inputMessage + 1), inputLength << 3, bits);
      }

    }  // End if (firstRequest_)... else ...

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
  // Flush if we exceeded the token length.
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

int ClientChannel::handleWrite(const unsigned char *message, unsigned int length)
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

  if (firstReply_)
  {
    #ifdef TEST
    *logofs << "handleWrite: First reply detected.\n" << logofs_flush;
    #endif

    unsigned int outputOpcode;

    decodeBuffer.decodeValue(outputOpcode, 8);
    unsigned int secondByte;
    decodeBuffer.decodeValue(secondByte, 8);
    unsigned int major;
    decodeBuffer.decodeValue(major, 16);
    unsigned int minor;
    decodeBuffer.decodeValue(minor, 16);
    unsigned int extraLength;
    decodeBuffer.decodeValue(extraLength, 16);
    unsigned int outputLength = 8 + (extraLength << 2);

    unsigned char *outputMessage = writeBuffer_.addMessage(outputLength);
    *outputMessage = (unsigned char) outputOpcode;
    outputMessage[1] = (unsigned char) secondByte;
    PutUINT(major, outputMessage + 2, bigEndian_);
    PutUINT(minor, outputMessage + 4, bigEndian_);
    PutUINT(extraLength, outputMessage + 6, bigEndian_);
    unsigned char *nextDest = outputMessage + 8;
    unsigned int cached;
    decodeBuffer.decodeBoolValue(cached);

    if (cached)
    {
      memcpy(nextDest, ServerCache::lastInitReply.getData(), outputLength - 8);
    }
    else
    {
      for (unsigned i = 8; i < outputLength; i++)
      {
        unsigned int nextByte;
        decodeBuffer.decodeValue(nextByte, 8);
        *nextDest++ = (unsigned char) nextByte;
      }

      ServerCache::lastInitReply.set(outputLength - 8, outputMessage + 8);
    }

    imageByteOrder_ = outputMessage[30];
    bitmapBitOrder_ = outputMessage[31];
    scanlineUnit_   = outputMessage[32];
    scanlinePad_    = outputMessage[33];

    firstReply_ = 0;

  } // End of if (firstReply_)

  //
  // This was previously in a 'else' block.
  // Due to the way the first request was
  // handled, we could not decode multiple
  // messages in the first frame.
  //

  { // Start of the decoding block.

    #ifdef DEBUG
    *logofs << "handleWrite: Starting loop on opcodes.\n"
            << logofs_flush;
    #endif

    unsigned char outputOpcode;

    //
    // NX client needs this line to consider
    // the initialization phase successfully
    // completed.
    //

    if (firstClient_ == -1)
    {
      cerr << "Info" << ": Established X client connection.\n" ;

      firstClient_ = fd_;
    }

    while (decodeBuffer.decodeOpcodeValue(outputOpcode, serverCache_ -> opcodeCache, 1))
    {
      #ifdef DEBUG
      *logofs << "handleWrite: Decoded a new OPCODE#"
              << (unsigned int) outputOpcode << ".\n"
              << logofs_flush;
      #endif

      unsigned char *outputMessage = NULL;
      unsigned int outputLength    = 0;

      //
      // General-purpose temp variables 
      // for decoding ints and chars.
      //

      unsigned int value   = 0;
      unsigned char cValue = 0;

      //
      // Check first if we need to abort any split,
      // then if this is a reply, finally if it is
      // en event or error.
      //

      if (outputOpcode == opcodeStore_ -> splitEvent)
      {
        //
        // It's an abort split, not a normal
        // burst of proxy data.
        //

        handleSplitEvent(decodeBuffer);

        continue;
      }
      else if (outputOpcode == X_Reply)
      {
        #ifdef DEBUG
        *logofs << "handleWrite: Decoding sequence number of reply.\n"
                << logofs_flush;
        #endif

        unsigned int sequenceNum;
        unsigned int sequenceDiff;

        decodeBuffer.decodeCachedValue(sequenceDiff, 16,
                           serverCache_ -> replySequenceCache, 7);

        sequenceNum = (serverSequence_ + sequenceDiff) & 0xffff;

        serverSequence_ = sequenceNum;

        #ifdef DEBUG
        *logofs << "handleWrite: Last server sequence number for FD#" 
                << fd_ << " is " << serverSequence_ << " with "
                << "difference " << sequenceDiff << ".\n"
                << logofs_flush;
        #endif

        //
        // In case of reply we can follow the X server and
        // override any event's sequence number generated
        // by this side.
        //

        #ifdef TEST
        *logofs << "handleWrite: Updating last event's sequence "
                << lastSequence_ << " to reply's sequence number "
                << serverSequence_ << " for FD#" << fd_ << ".\n"
                << logofs_flush;
        #endif

        lastSequence_ = serverSequence_;

        unsigned short int requestSequenceNum;
        unsigned char requestOpcode;

        #ifdef DEBUG

        requestSequenceNum = 0;
        requestOpcode = 0;

        *logofs << "handleWrite: Peek of sequence number returns ";

        *logofs << sequenceQueue_.peek(requestSequenceNum, requestOpcode);

        *logofs << " with sequence " << requestSequenceNum << " and opcode "
                << (unsigned int) requestOpcode << ".\n" << logofs_flush;

        #endif

        if (sequenceQueue_.peek(requestSequenceNum, requestOpcode) == 1 &&
                (requestSequenceNum == sequenceNum))
        {
          unsigned int requestData[3];

          sequenceQueue_.pop(requestSequenceNum, requestOpcode,
                                    requestData[0], requestData[1], requestData[2]);

          #ifdef DEBUG
          *logofs << "handleWrite: Identified reply to OPCODE#"
                  << (unsigned int) requestOpcode << ".\n"
                  << logofs_flush;
          #endif

          //
          // Is differential encoding disabled?
          //

          if (control -> RemoteDeltaCompression == 0)
          {
            int result = handleFastWriteReply(decodeBuffer, requestOpcode,
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

          switch (requestOpcode)
          {
          case X_AllocColor:
            {
              outputLength = 32;
              outputMessage = writeBuffer_.addMessage(outputLength);
              unsigned char *nextDest = outputMessage + 8;
              for (unsigned int i = 0; i < 3; i++)
              {
                decodeBuffer.decodeBoolValue(value);
                if (value)
                {
                  PutUINT(requestData[i], nextDest, bigEndian_);
                }
                else
                {
                  decodeBuffer.decodeValue(value, 16, 6);
                  PutUINT(requestData[i] + value, nextDest, bigEndian_);
                }
                nextDest += 2;
              }
              decodeBuffer.decodeValue(value, 32, 9);
              PutULONG(value, outputMessage + 16, bigEndian_);
            }
            break;
          case X_GetAtomName:
            {
              unsigned int nameLength;
              decodeBuffer.decodeValue(nameLength, 16, 6);
              outputLength = RoundUp4(nameLength) + 32;
              outputMessage = writeBuffer_.addMessage(outputLength);
              PutUINT(nameLength, outputMessage + 8, bigEndian_);
              unsigned char* nextDest = outputMessage + 32;

              if (control -> isProtoStep7() == 1)
              {
                decodeBuffer.decodeTextData(nextDest, nameLength);
              }
              else
              {
                serverCache_ -> getAtomNameTextCompressor.reset();
                for (unsigned int i = 0; i < nameLength; i++)
                {
                  *nextDest++ = serverCache_ -> getAtomNameTextCompressor.
                                      decodeChar(decodeBuffer);
                }
              }
            }
            break;
          case X_GetGeometry:
            {
              outputLength = 32;
              outputMessage = writeBuffer_.addMessage(outputLength);
              decodeBuffer.decodeCachedValue(cValue, 8,
                                 serverCache_ -> depthCache);
              outputMessage[1] = cValue;
              decodeBuffer.decodeCachedValue(value, 29,
                                 serverCache_ -> getGeometryRootCache, 9);
              PutULONG(value, outputMessage + 8, bigEndian_);
              unsigned char *nextDest = outputMessage + 12;
              for (unsigned int i = 0; i < 5; i++)
              {
                decodeBuffer.decodeCachedValue(value, 16,
                                   *serverCache_ -> getGeometryGeomCache[i], 8);
                PutUINT(value, nextDest, bigEndian_);
                nextDest += 2;
              }
            }
            break;
          case X_GetInputFocus:
            {
              outputLength = 32;
              outputMessage = writeBuffer_.addMessage(outputLength);
              decodeBuffer.decodeValue(value, 2);
              outputMessage[1] = (unsigned char) value;
              decodeBuffer.decodeCachedValue(value, 29,
                                 serverCache_ -> getInputFocusWindowCache);
              PutULONG(value, outputMessage + 8, bigEndian_);
            }
            break;
          case X_GetKeyboardMapping:
            {
              decodeBuffer.decodeBoolValue(value);
              if (value)
              {
                unsigned int dataLength =
                ServerCache::getKeyboardMappingLastMap.getLength();
                outputLength = 32 + dataLength;
                outputMessage = writeBuffer_.addMessage(outputLength);
                outputMessage[1] =
                ServerCache::getKeyboardMappingLastKeysymsPerKeycode;
                memcpy(outputMessage + 32,
                           ServerCache::getKeyboardMappingLastMap.getData(),
                               dataLength);
                break;
              }
              unsigned int numKeycodes;
              decodeBuffer.decodeValue(numKeycodes, 8);
              unsigned int keysymsPerKeycode;
              decodeBuffer.decodeValue(keysymsPerKeycode, 8, 4);
              ServerCache::getKeyboardMappingLastKeysymsPerKeycode =
                                  keysymsPerKeycode;
              outputLength = 32 + numKeycodes * keysymsPerKeycode * 4;
              outputMessage = writeBuffer_.addMessage(outputLength);
              outputMessage[1] = (unsigned char) keysymsPerKeycode;
              unsigned char *nextDest = outputMessage + 32;
              unsigned char previous = 0;
              for (unsigned int count = numKeycodes * keysymsPerKeycode;
                       count; --count)
              {
                decodeBuffer.decodeBoolValue(value);
                if (value)
                  PutULONG((unsigned int) NoSymbol, nextDest, bigEndian_);
                else
                {
                  unsigned int keysym;
                  decodeBuffer.decodeCachedValue(keysym, 24,
                             serverCache_ -> getKeyboardMappingKeysymCache, 9);
                  decodeBuffer.decodeCachedValue(cValue, 8,
                           serverCache_ -> getKeyboardMappingLastByteCache, 5);
                  previous += cValue;
                  PutULONG((keysym << 8) | previous, nextDest, bigEndian_);
                }
                nextDest += 4;
              }
              ServerCache::getKeyboardMappingLastMap.set(outputLength - 32,
                                                             outputMessage + 32);
            }
            break;
          case X_GetModifierMapping:
            {
              unsigned int keycodesPerModifier;
              decodeBuffer.decodeValue(keycodesPerModifier, 8);
              outputLength = 32 + (keycodesPerModifier << 3);
              outputMessage = writeBuffer_.addMessage(outputLength);
              outputMessage[1] = (unsigned char) keycodesPerModifier;
              unsigned char *nextDest = outputMessage + 32;
              decodeBuffer.decodeBoolValue(value);
              if (value)
              {
                memcpy(outputMessage + 32,
                           ServerCache::getModifierMappingLastMap.getData(),
                               ServerCache::getModifierMappingLastMap.getLength());
                break;
              }
              for (unsigned int count = outputLength - 32; count; count--)
              {
                decodeBuffer.decodeBoolValue(value);
                if (value)
                  *nextDest++ = 0;
                else
                {
                  decodeBuffer.decodeValue(value, 8);
                  *nextDest++ = value;
                }
              }
              ServerCache::getModifierMappingLastMap.set(outputLength - 32,
                                                             outputMessage + 32);
            }
            break;
          case X_GetProperty:
            {
              MessageStore *messageStore = serverStore_ ->
                                   getReplyStore(X_GetProperty);

              handleDecode(decodeBuffer, serverCache_, messageStore,
                               requestOpcode, outputMessage, outputLength);
            }
            break;
          case X_GetSelectionOwner:
            {
              outputLength = 32;
              outputMessage = writeBuffer_.addMessage(outputLength);
              decodeBuffer.decodeCachedValue(value, 29,
                                    serverCache_ -> getSelectionOwnerCache, 9);
              PutULONG(value, outputMessage + 8, bigEndian_);
            }
            break;
          case X_GetWindowAttributes:
            {
              outputLength = 44;
              outputMessage = writeBuffer_.addMessage(outputLength);
              decodeBuffer.decodeValue(value, 2);
              outputMessage[1] = (unsigned char) value;
              decodeBuffer.decodeCachedValue(value, 29,
                                             serverCache_ -> visualCache);
              PutULONG(value, outputMessage + 8, bigEndian_);
              decodeBuffer.decodeCachedValue(value, 16,
                             serverCache_ -> getWindowAttributesClassCache, 3);
              PutUINT(value, outputMessage + 12, bigEndian_);
              decodeBuffer.decodeCachedValue(cValue, 8,
                           serverCache_ -> getWindowAttributesBitGravityCache);
              outputMessage[14] = cValue;
              decodeBuffer.decodeCachedValue(cValue, 8,
                           serverCache_ -> getWindowAttributesWinGravityCache);
              outputMessage[15] = cValue;
              decodeBuffer.decodeCachedValue(value, 32,
                            serverCache_ -> getWindowAttributesPlanesCache, 9);
              PutULONG(value, outputMessage + 16, bigEndian_);
              decodeBuffer.decodeCachedValue(value, 32,
                             serverCache_ -> getWindowAttributesPixelCache, 9);
              PutULONG(value, outputMessage + 20, bigEndian_);
              decodeBuffer.decodeBoolValue(value);
              outputMessage[24] = (unsigned char) value;
              decodeBuffer.decodeBoolValue(value);
              outputMessage[25] = (unsigned char) value;
              decodeBuffer.decodeValue(value, 2);
              outputMessage[26] = (unsigned char) value;
              decodeBuffer.decodeBoolValue(value);
              outputMessage[27] = (unsigned char) value;
              decodeBuffer.decodeCachedValue(value, 29,
                                             serverCache_ -> colormapCache, 9);
              PutULONG(value, outputMessage + 28, bigEndian_);
              decodeBuffer.decodeCachedValue(value, 32,
                            serverCache_ -> getWindowAttributesAllEventsCache);
              PutULONG(value, outputMessage + 32, bigEndian_);
              decodeBuffer.decodeCachedValue(value, 32,
                           serverCache_ -> getWindowAttributesYourEventsCache);
              PutULONG(value, outputMessage + 36, bigEndian_);
              decodeBuffer.decodeCachedValue(value, 16,
                        serverCache_ -> getWindowAttributesDontPropagateCache);
              PutUINT(value, outputMessage + 40, bigEndian_);
            }
            break;
          case X_GrabKeyboard:
          case X_GrabPointer:
            {
              outputLength = 32;
              outputMessage = writeBuffer_.addMessage(outputLength);
              decodeBuffer.decodeValue(value, 3);
              outputMessage[1] = (unsigned char) value;
            }
            break;
          case X_InternAtom:
            {
              outputLength = 32;
              outputMessage = writeBuffer_.addMessage(outputLength);
              decodeBuffer.decodeValue(value, 29, 9);
              PutULONG(value, outputMessage + 8, bigEndian_);
            }
            break;
          case X_ListExtensions:
            {
              decodeBuffer.decodeValue(value, 32, 8);
              outputLength = 32 + (value << 2);
              outputMessage = writeBuffer_.addMessage(outputLength);
              unsigned int numExtensions;
              decodeBuffer.decodeValue(numExtensions, 8);
              outputMessage[1] = (unsigned char) numExtensions;
              unsigned char *nextDest = outputMessage + 32;
              for (; numExtensions; numExtensions--)
              {
                unsigned int length;
                decodeBuffer.decodeValue(length, 8);
                *nextDest++ = (unsigned char) length;
                for (; length; length--)
                {
                  decodeBuffer.decodeValue(value, 8);
                  *nextDest++ = value;
                }
              }
            }
            break;
          case X_ListFonts:
            {
              //
              // Differential compression can achieve a 12:1 to 14:1
              // ratio, while the best ZLIB compression can achieve
              // a mere 4:1 to 5:1. In the first case, though, the
              // huge amount of data constituting the message would
              // be stored uncompressed at the remote side. We need
              // to find a compromise. The solution is to use diffe-
              // rential compression at startup and ZLIB compression
              // later on.
              //

              MessageStore *messageStore = serverStore_ ->
                                   getReplyStore(X_ListFonts);

              if (handleDecodeCached(decodeBuffer, serverCache_, messageStore,
                                         outputMessage, outputLength))
              {
                break;
              }

              decodeBuffer.decodeValue(value, 32, 8);
              outputLength = 32 + (value << 2);
              outputMessage = writeBuffer_.addMessage(outputLength);
              unsigned int numFonts;
              decodeBuffer.decodeValue(numFonts, 16, 6);
              PutUINT(numFonts, outputMessage + 8, bigEndian_);

              // Differential or plain data compression?
              decodeBuffer.decodeBoolValue(value);

              if (value)
              {
                unsigned char* nextDest = outputMessage + 32;
                for (; numFonts; numFonts--)
                {
                  unsigned int length;
                  decodeBuffer.decodeValue(length, 8);
                  *nextDest++ = (unsigned char)length;

                  if (control -> isProtoStep7() == 1)
                  {
                    decodeBuffer.decodeTextData(nextDest, length);

                    nextDest += length;
                  }
                  else
                  {
                    serverCache_ -> getPropertyTextCompressor.reset();
                    for (; length; length--)
                    {
                      *nextDest++ = serverCache_ -> getPropertyTextCompressor.
                                                       decodeChar(decodeBuffer);
                    }
                  }
                }

                handleSave(messageStore, outputMessage, outputLength);
              }
              else
              {
                const unsigned char *compressedData = NULL;
                unsigned int compressedDataSize = 0;

                int decompressed = handleDecompress(decodeBuffer, requestOpcode, messageStore -> dataOffset,
                                                        outputMessage, outputLength, compressedData,
                                                            compressedDataSize);
                if (decompressed < 0)
                {
                  return -1;
                }
                else if (decompressed > 0)
                {
                  handleSave(messageStore, outputMessage, outputLength,
                                 compressedData, compressedDataSize);
                }
                else
                {
                  handleSave(messageStore, outputMessage, outputLength);
                }
              }
            }
            break;
          case X_LookupColor:
          case X_AllocNamedColor:
            {
              outputLength = 32;
              outputMessage = writeBuffer_.addMessage(outputLength);
              unsigned char *nextDest = outputMessage + 8;
              if (requestOpcode == X_AllocNamedColor)
              {
                decodeBuffer.decodeValue(value, 32, 9);
                PutULONG(value, nextDest, bigEndian_);
                nextDest += 4;
              }
              unsigned int count = 3;
              do
              {
                decodeBuffer.decodeValue(value, 16, 9);
                PutUINT(value, nextDest, bigEndian_);
                unsigned int visualColor;
                decodeBuffer.decodeValue(visualColor, 16, 5);
                visualColor += value;
                visualColor &= 0xffff;
                PutUINT(visualColor, nextDest + 6, bigEndian_);
                nextDest += 2;
              }
              while (--count);
            }
            break;
          case X_QueryBestSize:
            {
              outputLength = 32;
              outputMessage = writeBuffer_.addMessage(outputLength);
              decodeBuffer.decodeValue(value, 16, 8);
              PutUINT(value, outputMessage + 8, bigEndian_);
              decodeBuffer.decodeValue(value, 16, 8);
              PutUINT(value, outputMessage + 10, bigEndian_);
            }
            break;
          case X_QueryColors:
            {
              // Differential or plain data compression?
              decodeBuffer.decodeBoolValue(value);

              if (value)
              {
                decodeBuffer.decodeBoolValue(value);
                if (value)
                {
                  unsigned int numColors =
                  serverCache_ -> queryColorsLastReply.getLength() / 6;
                  outputLength = 32 + (numColors << 3);
                  outputMessage = writeBuffer_.addMessage(outputLength);
                  PutUINT(numColors, outputMessage + 8, bigEndian_);
                  const unsigned char *nextSrc =
                  serverCache_ -> queryColorsLastReply.getData();
                  unsigned char *nextDest = outputMessage + 32;
                  for (; numColors; numColors--)
                  {
                    for (unsigned int i = 0; i < 6; i++)
                      *nextDest++ = *nextSrc++;
                    nextDest += 2;
                  }
                }
                else
                {
                  unsigned int numColors;
                  decodeBuffer.decodeValue(numColors, 16, 5);
                  outputLength = 32 + (numColors << 3);
                  outputMessage = writeBuffer_.addMessage(outputLength);
                  PutUINT(numColors, outputMessage + 8, bigEndian_);
                  unsigned char *nextDest = outputMessage + 32;
                  for (unsigned int c = 0; c < numColors; c++)
                  {
                    for (unsigned int i = 0; i < 3; i++)
                    {
                      decodeBuffer.decodeValue(value, 16);
                      PutUINT(value, nextDest, bigEndian_);
                      nextDest += 2;
                    }
                  }
                  serverCache_ -> queryColorsLastReply.set(numColors * 6,
                                       outputMessage + 32);
                  const unsigned char *nextSrc = nextDest - 1;
                  nextDest = outputMessage + 32 + ((numColors - 1) << 3) + 5;
                  for (; numColors > 1; numColors--)
                  {
                    for (unsigned int i = 0; i < 6; i++)
                      *nextDest-- = *nextSrc--;
                    nextDest -= 2;
                  }
                }
              }
              else
              {
                // Reply length.
                unsigned int numColors;
                decodeBuffer.decodeValue(numColors, 16, 5);
                outputLength = 32 + (numColors << 3);
                outputMessage = writeBuffer_.addMessage(outputLength);
                PutUINT(numColors, outputMessage + 8, bigEndian_);

                const unsigned char *compressedData = NULL;
                unsigned int compressedDataSize = 0;

                int decompressed = handleDecompress(decodeBuffer, requestOpcode, 32,
                                                        outputMessage, outputLength, compressedData,
                                                            compressedDataSize);
                if (decompressed < 0)
                {
                  return -1;
                }
              }
            }
            break;
          case X_QueryExtension:
            {
              outputLength = 32;
              outputMessage = writeBuffer_.addMessage(outputLength);
              decodeBuffer.decodeBoolValue(value);
              outputMessage[8] = (unsigned char) value;
              decodeBuffer.decodeValue(value, 8);
              outputMessage[9] = (unsigned char) value;
              decodeBuffer.decodeValue(value, 8);
              outputMessage[10] = (unsigned char) value;
              decodeBuffer.decodeValue(value, 8);
              outputMessage[11] = (unsigned char) value;

              //
              // We use a predefined opcode to address
              // extensions' message stores, while real
              // opcodes are used for communication with
              // X server and clients.
              //

              if (requestData[0] == X_NXInternalShapeExtension)
              {
                opcodeStore_ -> shapeExtension = outputMessage[9];

                #ifdef TEST
                *logofs << "handleWrite: Shape extension opcode for FD#" << fd_
                        << " is " << (unsigned int) opcodeStore_ -> shapeExtension
                        << ".\n" << logofs_flush;
                #endif
              }
              else if (requestData[0] == X_NXInternalRenderExtension)
              {
                opcodeStore_ -> renderExtension = outputMessage[9];

                #ifdef TEST
                *logofs << "handleWrite: Render extension opcode for FD#" << fd_
                        << " is " << (unsigned int) opcodeStore_ -> renderExtension
                        << ".\n" << logofs_flush;
                #endif
              }
            }
            break;
          case X_QueryFont:
            {
              //
              // Use differential compression at startup and plain
              // data compression later. Check X_ListFonts message
              // for an explaination.
              //

              MessageStore *messageStore = serverStore_ ->
                                   getReplyStore(X_QueryFont);

              if (handleDecodeCached(decodeBuffer, serverCache_, messageStore,
                                         outputMessage, outputLength))
              {
                break;
              }

              // Differential or plain data compression?
              decodeBuffer.decodeBoolValue(value);

              if (value)
              {
                unsigned int numProperties;
                unsigned int numCharInfos;
                decodeBuffer.decodeValue(numProperties, 16, 8);
                decodeBuffer.decodeValue(numCharInfos, 32, 10);
                outputLength = 60 + numProperties * 8 + numCharInfos * 12;
                outputMessage = writeBuffer_.addMessage(outputLength);
                PutUINT(numProperties, outputMessage + 46, bigEndian_);
                PutULONG(numCharInfos, outputMessage + 56, bigEndian_);
                handleDecodeCharInfo(decodeBuffer, outputMessage + 8);
                handleDecodeCharInfo(decodeBuffer, outputMessage + 24);
                decodeBuffer.decodeValue(value, 16, 9);
                PutUINT(value, outputMessage + 40, bigEndian_);
                decodeBuffer.decodeValue(value, 16, 9);
                PutUINT(value, outputMessage + 42, bigEndian_);
                decodeBuffer.decodeValue(value, 16, 9);
                PutUINT(value, outputMessage + 44, bigEndian_);
                decodeBuffer.decodeBoolValue(value);
                outputMessage[48] = (unsigned char) value;
                decodeBuffer.decodeValue(value, 8);
                outputMessage[49] = (unsigned char) value;
                decodeBuffer.decodeValue(value, 8);
                outputMessage[50] = (unsigned char) value;
                decodeBuffer.decodeBoolValue(value);
                outputMessage[51] = (unsigned char) value;
                decodeBuffer.decodeValue(value, 16, 9);
                PutUINT(value, outputMessage + 52, bigEndian_);
                decodeBuffer.decodeValue(value, 16, 9);
                PutUINT(value, outputMessage + 54, bigEndian_);
                unsigned char *nextDest = outputMessage + 60;
                decodeBuffer.decodeBoolValue(value);

                int end = 0;

                if (value == 1)
                {
                  unsigned int index;
                  decodeBuffer.decodeValue(index, 4);
                  unsigned int length;
                  const unsigned char *data;
                  ServerCache::queryFontFontCache.get(index, length, data);
                  memcpy(nextDest, data, length);

                  end = 1;
                }

                if (end == 0)
                {
                  unsigned char *saveDest = nextDest;
                  unsigned int length = numProperties * 8 + numCharInfos * 12;
                  for (; numProperties; numProperties--)
                  {
                    decodeBuffer.decodeValue(value, 32, 9);
                    PutULONG(value, nextDest, bigEndian_);
                    decodeBuffer.decodeValue(value, 32, 9);
                    PutULONG(value, nextDest + 4, bigEndian_);
                    nextDest += 8;
                  }
                  for (; numCharInfos; numCharInfos--)
                  {
                    handleDecodeCharInfo(decodeBuffer, nextDest);

                    nextDest += 12;
                  }
                  ServerCache::queryFontFontCache.set(length, saveDest);
                }

                handleSave(messageStore, outputMessage, outputLength);
              }
              else
              {
                // Reply length.
                unsigned int replyLength;
                decodeBuffer.decodeValue(replyLength, 32, 16);
                outputLength = 32 + (replyLength << 2);
                outputMessage = writeBuffer_.addMessage(outputLength);

                const unsigned char *compressedData = NULL;
                unsigned int compressedDataSize = 0;

                int decompressed = handleDecompress(decodeBuffer, requestOpcode, messageStore -> dataOffset,
                                                        outputMessage, outputLength, compressedData,
                                                            compressedDataSize);
                if (decompressed < 0)
                {
                  return -1;
                }
                else if (decompressed > 0)
                {
                  handleSave(messageStore, outputMessage, outputLength,
                                 compressedData, compressedDataSize);
                }
                else
                {
                  handleSave(messageStore, outputMessage, outputLength);
                }
              }
            }
            break;
          case X_QueryPointer:
            {
              outputLength = 32;
              outputMessage = writeBuffer_.addMessage(outputLength);
              decodeBuffer.decodeBoolValue(value);
              outputMessage[1] = (unsigned char) value;
              decodeBuffer.decodeCachedValue(value, 29,
                                     serverCache_ -> queryPointerRootCache, 9);
              PutULONG(value, outputMessage + 8, bigEndian_);
              decodeBuffer.decodeCachedValue(value, 29,
                                    serverCache_ -> queryPointerChildCache, 9);
              PutULONG(value, outputMessage + 12, bigEndian_);
              decodeBuffer.decodeCachedValue(value, 16,
                                    serverCache_ -> motionNotifyRootXCache, 8);
              serverCache_ -> motionNotifyLastRootX += value;
              PutUINT(serverCache_ -> motionNotifyLastRootX, outputMessage + 16,
                      bigEndian_);
              decodeBuffer.decodeCachedValue(value, 16,
                                    serverCache_ -> motionNotifyRootYCache, 8);
              serverCache_ -> motionNotifyLastRootY += value;
              PutUINT(serverCache_ -> motionNotifyLastRootY, outputMessage + 18,
                      bigEndian_);
              decodeBuffer.decodeCachedValue(value, 16,
                                   serverCache_ -> motionNotifyEventXCache, 8);
              PutUINT(serverCache_ -> motionNotifyLastRootX + value,
                      outputMessage + 20, bigEndian_);
              decodeBuffer.decodeCachedValue(value, 16,
                                   serverCache_ -> motionNotifyEventYCache, 8);
              PutUINT(serverCache_ -> motionNotifyLastRootY + value,
                      outputMessage + 22, bigEndian_);
              decodeBuffer.decodeCachedValue(value, 16,
                                       serverCache_ -> motionNotifyStateCache);
              PutUINT(value, outputMessage + 24, bigEndian_);
            }
            break;
          case X_QueryTree:
            {
              unsigned int children;
              decodeBuffer.decodeValue(children, 16, 8);

              outputLength = 32 + (children << 2);
              outputMessage = writeBuffer_.addMessage(outputLength);

              PutULONG(outputLength, outputMessage + 4, bigEndian_);

              decodeBuffer.decodeCachedValue(value, 29,
                                 serverCache_ -> queryTreeWindowCache);

              PutULONG(value, outputMessage + 8, bigEndian_);

              decodeBuffer.decodeCachedValue(value, 29,
                                 serverCache_ -> queryTreeWindowCache);

              PutULONG(value, outputMessage + 12, bigEndian_);

              unsigned char *next = outputMessage + 32;

              PutUINT(children, outputMessage + 16, bigEndian_);

              for (unsigned int i = 0; i < children; i++)
              {
                decodeBuffer.decodeCachedValue(value, 29,
                                   serverCache_ -> queryTreeWindowCache);

                PutULONG(value, next + (i * 4), bigEndian_);
              }
            }
            break;
          case X_TranslateCoords:
            {
              outputLength = 32;
              outputMessage = writeBuffer_.addMessage(outputLength);
              decodeBuffer.decodeBoolValue(value);
              outputMessage[1] = (unsigned char) value;
              decodeBuffer.decodeCachedValue(value, 29,
                                 serverCache_ -> translateCoordsChildCache, 9);
              PutULONG(value, outputMessage + 8, bigEndian_);
              decodeBuffer.decodeCachedValue(value, 16,
                                     serverCache_ -> translateCoordsXCache, 8);
              PutUINT(value, outputMessage + 12, bigEndian_);
              decodeBuffer.decodeCachedValue(value, 16,
                                     serverCache_ -> translateCoordsYCache, 8);
              PutUINT(value, outputMessage + 14, bigEndian_);
            }
            break;
          case X_GetImage:
            {
              MessageStore *messageStore = serverStore_ ->
                                   getReplyStore(X_GetImage);

              if (handleDecodeCached(decodeBuffer, serverCache_, messageStore,
                                         outputMessage, outputLength))
              {
                break;
              }

              // Depth.
              decodeBuffer.decodeCachedValue(cValue, 8,
                                 serverCache_ -> depthCache);
              // Reply length.
              unsigned int replyLength;
              decodeBuffer.decodeValue(replyLength, 32, 9);
              outputLength = 32 + (replyLength << 2);
              outputMessage = writeBuffer_.addMessage(outputLength);
              outputMessage[1] = (unsigned char) cValue;
              // Visual.
              unsigned int visual;
              decodeBuffer.decodeCachedValue(visual, 29, 
                                 serverCache_ -> visualCache);
              PutULONG(visual, outputMessage + 8, bigEndian_);

              if (control -> isProtoStep8() == 0)
              {
                const unsigned char *compressedData = NULL;
                unsigned int compressedDataSize = 0;

                int decompressed = handleDecompress(decodeBuffer, requestOpcode, messageStore -> dataOffset,
                                                        outputMessage, outputLength, compressedData,
                                                            compressedDataSize);
                if (decompressed < 0)
                {
                  return -1;
                }
                else if (decompressed > 0)
                {
                  handleSave(messageStore, outputMessage, outputLength,
                                 compressedData, compressedDataSize);
                }
                else
                {
                  handleSave(messageStore, outputMessage, outputLength);
                }
              }
              else
              {
                handleCopy(decodeBuffer, requestOpcode, messageStore ->
                               dataOffset, outputMessage, outputLength);

                handleSave(messageStore, outputMessage, outputLength);
              }
            }
            break;
          case X_GetPointerMapping:
            {
              unsigned int nextByte;
              decodeBuffer.decodeValue(nextByte, 8, 4);
              unsigned int replyLength;
              decodeBuffer.decodeValue(replyLength, 32, 4);
              outputLength = 32 + (replyLength << 2);
              outputMessage = writeBuffer_.addMessage(outputLength);
              outputMessage[1] = (unsigned char) nextByte;
              unsigned char *nextDest = outputMessage + 32;
              for (unsigned int i = 32; i < outputLength; i++)
              {
                decodeBuffer.decodeValue(nextByte, 8, 4);
                *nextDest++ = (unsigned char) nextByte;
              }
            }
            break;
          case X_GetKeyboardControl:
            {
              unsigned int nextByte;
              decodeBuffer.decodeValue(nextByte, 8, 2);
              unsigned int replyLength;
              decodeBuffer.decodeValue(replyLength, 32, 8);
              outputLength = 32 + (replyLength << 2);
              outputMessage = writeBuffer_.addMessage(outputLength);
              outputMessage[1] = (unsigned char) nextByte;
              unsigned char *nextDest = outputMessage + 8;
              for (unsigned int i = 8; i < outputLength; i++)
              {
                decodeBuffer.decodeValue(nextByte, 8, 4);
                *nextDest++ = (unsigned char) nextByte;
              }
            }
            break;
          default:
            {
              if (requestOpcode == opcodeStore_ -> getUnpackParameters)
              {
                #ifdef TEST
                *logofs << "handleWrite: Received get unpack parameters reply "
                        << "OPCODE#" << (unsigned int) opcodeStore_ -> getUnpackParameters
                        << ".\n" << logofs_flush;
                #endif

                outputLength = 32 + PACK_METHOD_LIMIT;

                outputMessage = writeBuffer_.addMessage(outputLength);

                unsigned int method;

                //
                // Let agent use only the unpack methods
                // implemented at both sides.
                //

                for (int i = 0; i < PACK_METHOD_LIMIT; i++)
                {
                  decodeBuffer.decodeBoolValue(method);

                  control -> RemoteUnpackMethods[i] = method;

                  *(outputMessage + 32 + i) =
                      (control -> LocalUnpackMethods[i] == 1 &&
                           method == 1);
                }
              }
              else if (requestOpcode == opcodeStore_ -> getShmemParameters)
              {
                if (handleShmemReply(decodeBuffer, requestOpcode,
                                         outputMessage, outputLength) < 0)
                {
                  return -1;
                }
              }
              else if (requestOpcode == opcodeStore_ -> getFontParameters)
              {
                if (handleFontReply(decodeBuffer, requestOpcode,
                                        outputMessage, outputLength) < 0)
                {
                  return -1;
                }
              }
              else
              {
                #ifdef PANIC
                *logofs << "handleWrite: PANIC! No matching request for "
                        << "reply with sequence number " << sequenceNum
                        << ".\n" << logofs_flush;
                #endif

                cerr << "Error" << ": No matching request for "
                     << "reply with sequence number " << sequenceNum
                     << ".\n";

                return -1;
              }
            }
          }

          #if defined(TEST) || defined(OPCODES)
          *logofs << "handleWrite: Handled reply to OPCODE#"
                  << (unsigned) requestOpcode << " (" << DumpOpcode(requestOpcode)
                  << ")" << " for FD#" << fd_ << " with sequence " << serverSequence_
                  << ". Output size is " << outputLength << ".\n" << logofs_flush;
          #endif

          statistics -> addRepliedRequest(requestOpcode);
        }
        else // End of if (sequenceQueue_.peek() && ...)
        {
          //
          // Reply didn't match any request opcode.
          // Check again if differential encoding
          // is disabled.
          //

          #ifdef DEBUG
          *logofs << "handleWrite: Identified generic reply.\n"
                  << logofs_flush;
          #endif

          requestOpcode = X_Reply;

          if (control -> RemoteDeltaCompression == 0)
          {
            int result = handleFastWriteReply(decodeBuffer, requestOpcode,
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
          // All replies whose opcode is not pushed in
          // sequence number queue are cached together.
          // Among such replies are those to extension
          // requests.
          //

          MessageStore *messageStore = serverStore_ ->
                               getReplyStore(X_NXInternalGenericReply);

          handleDecode(decodeBuffer, serverCache_, messageStore,
                           requestOpcode, outputMessage, outputLength);

          #if defined(TEST) || defined(OPCODES)
          *logofs << "handleWrite: Handled generic reply for FD#" << fd_
                  << " with sequence " << serverSequence_ << ". Output size is "
                  << outputLength << ".\n" << logofs_flush;
          #endif

          statistics -> addRepliedRequest(requestOpcode);

        } // End of if (sequenceQueue_.peek() && ...) else ...

        //
        // If any output was produced then write opcode,
        // sequence number and size to the buffer.
        //

        if (outputLength > 0)
        {
          *outputMessage = outputOpcode;

          PutUINT(serverSequence_, outputMessage + 2, bigEndian_);

          PutULONG((outputLength - 32) >> 2, outputMessage + 4, bigEndian_);
        }

      } // End of if (outputOpcode == 1)...
      else
      {
        //
        // It's an event or error.
        //

        unsigned int sequenceNum;
        unsigned int sequenceDiff;

        decodeBuffer.decodeCachedValue(sequenceDiff, 16,
                           serverCache_ -> eventSequenceCache, 7);

        sequenceNum = (serverSequence_ + sequenceDiff) & 0xffff;

        serverSequence_ = sequenceNum;

        #ifdef DEBUG
        *logofs << "handleWrite: Last server sequence number for FD#" 
                << fd_ << " is " << serverSequence_ << " with "
                << "difference " << sequenceDiff << ".\n"
                << logofs_flush;
        #endif

        //
        // Check if this is an error that matches
        // a sequence number for which we were
        // expecting a reply.
        //

        if (outputOpcode == X_Error)
        {
          unsigned short int errorSequenceNum;
          unsigned char errorOpcode;

          if (sequenceQueue_.peek(errorSequenceNum, errorOpcode) &&
                  ((unsigned) errorSequenceNum == serverSequence_))
          {
            //
            // Remove the queued sequence of the reply.
            //

            #ifdef TEST
            *logofs << "handleWrite: WARNING! Removing reply to OPCODE#"
                    << (unsigned) errorOpcode << " sequence "
                    << errorSequenceNum << " for FD#" << fd_
                    << " due to error.\n" << logofs_flush;
            #endif

            sequenceQueue_.pop(errorSequenceNum, errorOpcode);

            //
            // Send to the client the current sequence
            // number, not the number that matched the
            // reply. Because we are generating replies
            // at our side, Xlib can incur in a sequence
            // lost if the error comes after the auto-
            // generated reply.
            //

            if (control -> SessionMode == session_proxy)
            {
              #ifdef TEST
              *logofs << "handleWrite: Updating last event's sequence "
                      << lastSequence_ << " to X server's error sequence "
                      << "number " << serverSequence_ << " for FD#"
                      << fd_ << ".\n" << logofs_flush;
              #endif

              lastSequence_ = serverSequence_;
            }
          }

          //
          // In case of errors always send to client the
          // original X server's sequence associated to
          // the failing request.
          //

          if (control -> SessionMode != session_proxy)
          {
            #ifdef TEST
            *logofs << "handleWrite: Updating last event's sequence "
                    << lastSequence_ << " to X server's error sequence "
                    << "number " << serverSequence_ << " for FD#"
                    << fd_ << ".\n" << logofs_flush;
            #endif

            lastSequence_ = serverSequence_;
          }
        }

        //
        // Check if by producing events at client side we
        // have modified the events' sequence numbering.
        // In this case taint the original sequence to
        // comply with the last one known by client.
        //

/*
FIXME: Recover the sequence number if the proxy
       is not connected to an agent.
*/
        if (serverSequence_ > lastSequence_ ||
                control -> SessionMode != session_proxy)
        {
          #ifdef DEBUG
          *logofs << "handleWrite: Updating last event's sequence "
                  << lastSequence_ << " to X server's sequence number "
                  << serverSequence_ << " for FD#" << fd_
                  << ".\n" << logofs_flush;
          #endif

          lastSequence_ = serverSequence_;
        }
        #ifdef DEBUG
        else if (serverSequence_ < lastSequence_)
        {
          //
          // Use our last auto-generated sequence.
          //

          *logofs << "handleWrite: Tainting sequence number "
                  << serverSequence_ << " to last event's sequence "
                  << lastSequence_ << " for FD#" << fd_ << ".\n"
                  << logofs_flush;
        }
        #endif

        //
        // Check if remote side used fast encoding.
        //

        if (control -> RemoteDeltaCompression == 0)
        {
          int result = handleFastWriteEvent(decodeBuffer, outputOpcode,
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
        // Make space for message in the outgoing buffer
        // and write opcode and sequence number.
        //

        outputLength = 32;
        outputMessage = writeBuffer_.addMessage(outputLength);

        *outputMessage = outputOpcode;

        PutUINT(lastSequence_, outputMessage + 2, bigEndian_);

        #ifdef DEBUG
        *logofs << "handleWrite: Going to handle event or error OPCODE#"
                << (unsigned int) outputOpcode << " for FD#" << fd_
                << " sequence " << lastSequence_ << " (real was "
                << serverSequence_ << ").\n" << logofs_flush;
        #endif

        switch (outputOpcode)
        {
        case X_Error:
          {
            unsigned char code;
            decodeBuffer.decodeCachedValue(code, 8,
                               serverCache_ -> errorCodeCache);
            outputMessage[1] = code;

            #if defined(TEST) || defined(OPCODES)
            *logofs << "handleWrite: Handled error ERR_CODE#"
                    << (unsigned int) code << " for FD#" << fd_;
            #endif

            if ((code != 11) && (code != 8) &&
                    (code != 15) && (code != 1))
            {
              decodeBuffer.decodeValue(value, 32, 16);
              PutULONG(value, outputMessage + 4, bigEndian_);

              #if defined(TEST) || defined(OPCODES)
              *logofs << " RES_ID#" << value;
              #endif
            }

            if (code >= 18)
            {
              decodeBuffer.decodeCachedValue(value, 16,
                                 serverCache_ -> errorMinorCache);
              PutUINT(value, outputMessage + 8, bigEndian_);

              #if defined(TEST) || defined(OPCODES)
              *logofs << " MIN_OP#" << value;
              #endif
            }

            decodeBuffer.decodeCachedValue(cValue, 8,
                               serverCache_ -> errorMajorCache);
            outputMessage[10] = cValue;

            #if defined(TEST) || defined(OPCODES)
            *logofs << " MAJ_OP#" << (unsigned int) cValue;
            #endif

            if (code >= 18)
            {
              unsigned char *nextDest = outputMessage + 11;
              for (unsigned int i = 11; i < 32; i++)
              {
                decodeBuffer.decodeValue(value, 8);
                *nextDest++ = (unsigned char) cValue;
              }
            }

            #if defined(TEST) || defined(OPCODES)
            *logofs << " sequence " << lastSequence_ << " (real was "
                    << serverSequence_ << ") . Size is "
                    << (unsigned int) outputLength << ".\n"
                    << logofs_flush;
            #endif
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
            if (outputOpcode == MotionNotify)
            {
              decodeBuffer.decodeBoolValue(value);
            }
            else if (outputOpcode == EnterNotify || outputOpcode == LeaveNotify)
            {
              decodeBuffer.decodeValue(value, 3);
            }
            else if (outputOpcode == KeyRelease)
            {
              decodeBuffer.decodeBoolValue(value);
              if (value)
              {
                value = serverCache_ -> keyPressLastKey;
              }
              else
              {
                decodeBuffer.decodeValue(value, 8);
              }
            }
            else if (outputOpcode == ButtonPress || outputOpcode == ButtonRelease)
            {
              decodeBuffer.decodeCachedValue(cValue, 8,
                                 serverCache_ -> buttonCache);
              value = (unsigned int) cValue;
            }
            else
            {
              decodeBuffer.decodeValue(value, 8);
            }

            outputMessage[1] = (unsigned char) value;
            decodeBuffer.decodeCachedValue(value, 32,
                               serverCache_ -> motionNotifyTimestampCache, 9);
            serverCache_ -> lastTimestamp += value;
            PutULONG(serverCache_ -> lastTimestamp, outputMessage + 4,
                     bigEndian_);
            unsigned char *nextDest = outputMessage + 8;
            int skipRest = 0;
            if (outputOpcode == KeyRelease)
            {
              decodeBuffer.decodeBoolValue(value);
              if (value)
              {
                for (unsigned int i = 0; i < 23; i++)
                {
                  *nextDest++ = serverCache_ -> keyPressCache[i];
                }
                skipRest = 1;
              }
            }

            if (!skipRest)
            {
              for (unsigned int i = 0; i < 3; i++)
              {
                decodeBuffer.decodeCachedValue(value, 29,
                                   *serverCache_ -> motionNotifyWindowCache[i], 6);
                PutULONG(value, nextDest, bigEndian_);
                nextDest += 4;
              }
              decodeBuffer.decodeCachedValue(value, 16,
                                    serverCache_ -> motionNotifyRootXCache, 6);
              serverCache_ -> motionNotifyLastRootX += value;
              PutUINT(serverCache_ -> motionNotifyLastRootX, outputMessage + 20,
                      bigEndian_);
              decodeBuffer.decodeCachedValue(value, 16,
                                    serverCache_ -> motionNotifyRootYCache, 6);
              serverCache_ -> motionNotifyLastRootY += value;
              PutUINT(serverCache_ -> motionNotifyLastRootY, outputMessage + 22,
                      bigEndian_);
              decodeBuffer.decodeCachedValue(value, 16,
                                   serverCache_ -> motionNotifyEventXCache, 6);
              PutUINT(serverCache_ -> motionNotifyLastRootX + value,
                      outputMessage + 24, bigEndian_);
              decodeBuffer.decodeCachedValue(value, 16,
                                   serverCache_ -> motionNotifyEventYCache, 6);
              PutUINT(serverCache_ -> motionNotifyLastRootY + value,
                      outputMessage + 26, bigEndian_);
              decodeBuffer.decodeCachedValue(value, 16,
                                       serverCache_ -> motionNotifyStateCache);
              PutUINT(value, outputMessage + 28, bigEndian_);
              if (outputOpcode == EnterNotify || outputOpcode == LeaveNotify)
              {
                decodeBuffer.decodeValue(value, 2);
              }
              else
              {
                decodeBuffer.decodeBoolValue(value);
              }
              outputMessage[30] = (unsigned char) value;
              if (outputOpcode == EnterNotify || outputOpcode == LeaveNotify)
              {
                decodeBuffer.decodeValue(value, 2);
                outputMessage[31] = (unsigned char) value;
              }
              else if (outputOpcode == KeyPress)
              {
                serverCache_ -> keyPressLastKey = outputMessage[1];
                for (unsigned int i = 8; i < 31; i++)
                {
                  serverCache_ -> keyPressCache[i - 8] = outputMessage[i];
                }
              }
            }
          }
          break;
        case ColormapNotify:
          {
            decodeBuffer.decodeCachedValue(value, 29,
                                 serverCache_ -> colormapNotifyWindowCache, 8);
            PutULONG(value, outputMessage + 4, bigEndian_);
            decodeBuffer.decodeCachedValue(value, 29,
                               serverCache_ -> colormapNotifyColormapCache, 8);
            PutULONG(value, outputMessage + 8, bigEndian_);
            decodeBuffer.decodeBoolValue(value);
            outputMessage[12] = (unsigned char) value;
            decodeBuffer.decodeBoolValue(value);
            outputMessage[13] = (unsigned char) value;
          }
          break;
        case ConfigureNotify:
          {
            unsigned char *nextDest = outputMessage + 4;
            for (unsigned int i = 0; i < 3; i++)
            {
              decodeBuffer.decodeCachedValue(value, 29,
                                 *serverCache_ -> configureNotifyWindowCache[i], 9);
              PutULONG(value, nextDest, bigEndian_);
              nextDest += 4;
            }
            for (unsigned int j = 0; j < 5; j++)
            {
              decodeBuffer.decodeCachedValue(value, 16,
                                 *serverCache_ -> configureNotifyGeomCache[j], 8);
              PutUINT(value, nextDest, bigEndian_);
              nextDest += 2;
            }
            decodeBuffer.decodeBoolValue(value);
            *nextDest = value;
          }
          break;
        case CreateNotify:
          {
            decodeBuffer.decodeCachedValue(value, 29,
                                 serverCache_ -> createNotifyWindowCache, 9);
            PutULONG(value, outputMessage + 4, bigEndian_);
            decodeBuffer.decodeValue(value, 29, 5);
            serverCache_ -> createNotifyLastWindow += value;
            serverCache_ -> createNotifyLastWindow &= 0x1fffffff;
            PutULONG(serverCache_ -> createNotifyLastWindow, outputMessage + 8,
                     bigEndian_);
            unsigned char* nextDest = outputMessage + 12;
            for (unsigned int i = 0; i < 5; i++)
            {
              decodeBuffer.decodeValue(value, 16, 9);
              PutUINT(value, nextDest, bigEndian_);
              nextDest += 2;
            }
            decodeBuffer.decodeBoolValue(value);
            *nextDest = (unsigned char) value;
          }
          break;
        case Expose:
          {
            decodeBuffer.decodeCachedValue(value, 29,
                               serverCache_ -> exposeWindowCache, 9);
            PutULONG(value, outputMessage + 4, bigEndian_);
            unsigned char *nextDest = outputMessage + 8;
            for (unsigned int i = 0; i < 5; i++)
            {
              decodeBuffer.decodeCachedValue(value, 16,
                                 *serverCache_ -> exposeGeomCache[i], 6);
              PutUINT(value, nextDest, bigEndian_);
              nextDest += 2;
            }
          }
          break;
        case FocusIn:
        case FocusOut:
          {
            decodeBuffer.decodeValue(value, 3);
            outputMessage[1] = (unsigned char) value;
            decodeBuffer.decodeCachedValue(value, 29,
                                        serverCache_ -> focusInWindowCache, 9);
            PutULONG(value, outputMessage + 4, bigEndian_);
            decodeBuffer.decodeValue(value, 2);
            outputMessage[8] = (unsigned char) value;
          }
          break;
        case KeymapNotify:
          {
            decodeBuffer.decodeBoolValue(value);
            if (value)
            memcpy(outputMessage + 1, ServerCache::lastKeymap.getData(), 31);
            else
            {
              unsigned char *nextDest = outputMessage + 1;
              for (unsigned int i = 1; i < 32; i++)
              {
                decodeBuffer.decodeValue(value, 8);
                *nextDest++ = (unsigned char) value;
              }
            ServerCache::lastKeymap.set(31, outputMessage + 1);
            }
          }
          break;
        case MapNotify:
        case UnmapNotify:
        case DestroyNotify:
          {
            decodeBuffer.decodeCachedValue(value, 29,
                                       serverCache_ -> mapNotifyEventCache, 9);
            PutULONG(value, outputMessage + 4, bigEndian_);
            decodeBuffer.decodeCachedValue(value, 29,
                                      serverCache_ -> mapNotifyWindowCache, 9);
            PutULONG(value, outputMessage + 8, bigEndian_);
            if (outputOpcode == MapNotify || outputOpcode == UnmapNotify)
            {
              decodeBuffer.decodeBoolValue(value);
              outputMessage[12] = (unsigned char) value;
            }
          }
          break;
        case NoExpose:
          {
            decodeBuffer.decodeCachedValue(value, 29,
                                     serverCache_ -> noExposeDrawableCache, 9);
            PutULONG(value, outputMessage + 4, bigEndian_);
            decodeBuffer.decodeCachedValue(value, 16,
                                           serverCache_ -> noExposeMinorCache);
            PutUINT(value, outputMessage + 8, bigEndian_);
            decodeBuffer.decodeCachedValue(cValue, 8,
                                           serverCache_ -> noExposeMajorCache);
            outputMessage[10] = cValue;
          }
          break;
        case PropertyNotify:
          {
            decodeBuffer.decodeCachedValue(value, 29,
                                 serverCache_ -> propertyNotifyWindowCache, 9);
            PutULONG(value, outputMessage + 4, bigEndian_);
            decodeBuffer.decodeCachedValue(value, 29,
                                   serverCache_ -> propertyNotifyAtomCache, 9);
            PutULONG(value, outputMessage + 8, bigEndian_);
            decodeBuffer.decodeValue(value, 32, 9);
            serverCache_ -> lastTimestamp += value;
            PutULONG(serverCache_ -> lastTimestamp, outputMessage + 12,
                     bigEndian_);
            decodeBuffer.decodeBoolValue(value);
            outputMessage[16] = (unsigned char) value;
          }
          break;
        case ReparentNotify:
          {
            unsigned char* nextDest = outputMessage + 4;
            for (unsigned int i = 0; i < 3; i++)
            {
              decodeBuffer.decodeCachedValue(value, 29,
                             serverCache_ -> reparentNotifyWindowCache, 9);
              PutULONG(value, nextDest, bigEndian_);
              nextDest += 4;
            }
            decodeBuffer.decodeValue(value, 16, 6);
            PutUINT(value, nextDest, bigEndian_);
            decodeBuffer.decodeValue(value, 16, 6);
            PutUINT(value, nextDest + 2, bigEndian_);
            decodeBuffer.decodeBoolValue(value);
            outputMessage[20] = (unsigned char)value;
          }
          break;
        case SelectionClear:
          {
            decodeBuffer.decodeValue(value, 32, 9);
            serverCache_ -> lastTimestamp += value;
            PutULONG(serverCache_ -> lastTimestamp, outputMessage + 4,
                     bigEndian_);
            decodeBuffer.decodeCachedValue(value, 29,
                                   serverCache_ -> selectionClearWindowCache, 9);
            PutULONG(value, outputMessage + 8, bigEndian_);
            decodeBuffer.decodeCachedValue(value, 29,
                                   serverCache_ -> selectionClearAtomCache, 9);
            PutULONG(value, outputMessage + 12, bigEndian_);
          }
          break;
        case SelectionRequest:
          {
            decodeBuffer.decodeValue(value, 32, 9);
            serverCache_ -> lastTimestamp += value;
            PutULONG(serverCache_ -> lastTimestamp, outputMessage + 4,
                     bigEndian_);
            decodeBuffer.decodeCachedValue(value, 29,
                                   serverCache_ -> selectionClearWindowCache, 9);
            PutULONG(value, outputMessage + 8, bigEndian_);
            decodeBuffer.decodeCachedValue(value, 29,
                                   serverCache_ -> selectionClearWindowCache, 9);
            PutULONG(value, outputMessage + 12, bigEndian_);
            decodeBuffer.decodeCachedValue(value, 29,
                                   serverCache_ -> selectionClearAtomCache, 9);
            PutULONG(value, outputMessage + 16, bigEndian_);
            decodeBuffer.decodeCachedValue(value, 29,
                                   serverCache_ -> selectionClearAtomCache, 9);
            PutULONG(value, outputMessage + 20, bigEndian_);
            decodeBuffer.decodeCachedValue(value, 29,
                                   serverCache_ -> selectionClearAtomCache, 9);
            PutULONG(value, outputMessage + 24, bigEndian_);
          }
          break;
        case VisibilityNotify:
          {
            decodeBuffer.decodeCachedValue(value, 29,
                               serverCache_ -> visibilityNotifyWindowCache, 9);
            PutULONG(value, outputMessage + 4, bigEndian_);
            decodeBuffer.decodeValue(value, 2);
            outputMessage[8] = (unsigned char) value;
          }
          break;
        default:
          {
            #ifdef TEST
            *logofs << "handleWrite: Using generic event compression "
                    << "for OPCODE#" << (unsigned int) outputOpcode
                    << ".\n" << logofs_flush;
            #endif

            decodeBuffer.decodeCachedValue(*(outputMessage + 1), 8,
                         serverCache_ -> genericEventCharCache);

            for (unsigned int i = 0; i < 14; i++)
            {
              decodeBuffer.decodeCachedValue(value, 16,
                           *serverCache_ -> genericEventIntCache[i]);

              PutUINT(value, outputMessage + i * 2 + 4, bigEndian_);
            }
          }
        } // End of switch (outputOpcode)...

        #if defined(TEST) || defined(OPCODES)
        if (outputOpcode != X_Error)
        {
          *logofs << "handleWrite: Handled event OPCODE#"
                  << (unsigned int) outputOpcode << " for FD#"
                  << fd_ << " sequence " << lastSequence_ << " (real was "
                  << serverSequence_ << "). Size is " << outputLength
                  << ".\n" << logofs_flush;
        }
        #endif

        //
        // Check if we need to suppress the error.
        //

        if (outputOpcode == X_Error &&
                handleTaintSyncError(*(outputMessage + 10)) > 0)
        {
          #if defined(TEST) || defined(OPCODES)
          *logofs << "handleWrite: WARNING! Suppressed error OPCODE#"
                  << (unsigned int) outputOpcode << " for FD#"
                  << fd_ << " sequence " << lastSequence_ << ".\n"
                  << logofs_flush;
          #endif

          writeBuffer_.removeMessage(32);
        }

      } // End of if (outputOpcode == 1)... else ...

      //
      // Check if we produced enough data. We need to
      // decode all provided messages. Just update the
      // finish flag in case of failure.
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

  return 1;
}

//
// End of handleWrite().
//

//
// Other members.
//

int ClientChannel::handleSplit(EncodeBuffer &encodeBuffer, MessageStore *store,
                                   T_store_action action, int position, const unsigned char opcode,
                                       const unsigned char *buffer, const unsigned int size)
{
  #if defined(TEST) || defined(SPLIT)

  if (control -> isProtoStep8() == 1)
  {
    *logofs << "handleSplit: PANIC! SPLIT! Split should "
            << "not be enabled for message " << "OPCODE#"
            << (unsigned int) store -> opcode() << ".\n"
            << logofs_flush;

    HandleCleanup();
  }

  #endif

  //
  // Never split the message if connected to
  // an old proxy version. Also refuse the
  // split if we it is not introduced by a
  // start split.
  //

  if (control -> isProtoStep7() == 0)
  {
    #if defined(TEST) || defined(SPLIT)
    *logofs << "handleSplit: SPLIT! Ignoring the split with "
            << "an old proxy version.\n" << logofs_flush;
    #endif

    if (action == IS_ADDED || action == is_discarded)
    {
      encodeBuffer.encodeBoolValue(0);
    }

    return 0;
  }
  else if (splitState_.resource == nothing || enableSplit_ == 0)
  {
    #if defined(TEST) || defined(SPLIT)
    *logofs << "handleSplit: SPLIT! Nothing to do for message "
            << "OPCODE#" << (unsigned int) store -> opcode()
            << " of size " << size << " position " << position
            << " with action [" << DumpAction(action) << "] at "
            << strMsTimestamp() << ".\n" << logofs_flush;
    #endif

    encodeBuffer.encodeBoolValue(0);

    return 0;
  }

  //
  // It's not advisable to allocate the store at
  // the time we receive the start-split because
  // we may process all the splits received and
  // deallocate the store even before we receive
  // the end split. Another message for the same
  // split sequence may then come and we would
  // have a null split store.
  //

  handleSplitStoreAlloc(&splitResources_, splitState_.resource);

  //
  // Check if the split was actually requested by
  // the agent and if the request was saved in the
  // message store. The split can also be refused
  // if the message is smaller than the threshold
  // or if the split store is already full.
  //

  if (mustSplitMessage(splitState_.resource) == 0)
  {
    if (action == IS_HIT || canSplitMessage(splitState_.mode, size) == 0)
    {
      #if defined(TEST) || defined(SPLIT)

      if (splitState_.mode == split_none)
      {
        #ifdef PANIC
        *logofs << "handleSplit: PANIC! SPLIT! Split state has "
                << "mode 'none'.\n" << logofs_flush;
        #endif

        HandleCleanup();
      }

      if (action != IS_HIT && (int) size >=
              control -> SplitDataThreshold)
      {
        #ifdef WARNING
        *logofs << "handleSplit: WARNING! SPLIT! Split stores have "
                << clientStore_ -> getSplitTotalSize() << " messages "
                << "and " << clientStore_ -> getSplitTotalStorageSize()
                << " allocated bytes.\n" << logofs_flush;
        #endif
      }

      #endif

      #if defined(TEST) || defined(SPLIT)
      *logofs << "handleSplit: SPLIT! Message OPCODE#"
              << (unsigned int) store -> opcode() << " of size " << size
              << " [not split] with resource " << splitState_.resource
              << " mode " << splitState_.mode << " position " << position
              << " and action [" << DumpAction(action) << "] at "
              << strMsTimestamp() << ".\n" << logofs_flush;
      #endif

      encodeBuffer.encodeBoolValue(0);

      return 0;
    }
  }

  #if defined(TEST) || defined(SPLIT)
  *logofs << "handleSplit: SPLIT! Message OPCODE#"
          << (unsigned int) store -> opcode() << " of size " << size
          << " [split] with resource " << splitState_.resource
          << " mode " << splitState_.mode << " position " << position
          << " and action [" << DumpAction(action) << "] at "
          << strMsTimestamp() << ".\n" << logofs_flush;
  #endif

  encodeBuffer.encodeBoolValue(1);

  T_checksum checksum = NULL;

  if (action == IS_ADDED)
  {
    checksum = store -> getChecksum(position);
  }
  else if (action == is_discarded)
  {
    //
    // Generate the checksum on the fly.
    //

    checksum = store -> getChecksum(buffer, size, bigEndian_);
  }

  //
  // The method must abort the connection
  // if it can't allocate the split.
  //

  Split *splitMessage = clientStore_ -> getSplitStore(splitState_.resource) ->
                              add(store, splitState_.resource, splitState_.mode,
                                      position, action, checksum, buffer, size);

  //
  // Send the checksum. By using the checksum,
  // the remote end will try to locate the
  // message and load it from disk.
  //

  if (action == IS_HIT)
  {
    splitMessage -> setState(split_loaded);
  }
  else if (handleSplitChecksum(encodeBuffer, checksum) == 0)
  {
    //
    // If the checksum is not sent, for example
    // because loading of messages from disk is
    // disabled, then mark the split as missed.
    //

    #ifdef WARNING
    *logofs << "handleSplit: WARNING! Checksum not sent. "
            << "Marking the split as [missed].\n"
            << logofs_flush;
    #endif

    splitMessage -> setState(split_missed);
  }

  if (action == is_discarded)
  {
    delete [] checksum;
  }

  //
  // Check if we are ready to send a new split
  // for this store.
  //

  handleSplitPending(splitState_.resource);

  #if defined(TEST) || defined(SPLIT)

  *logofs << "handleSplit: SPLIT! There are " << clientStore_ ->
             getSplitTotalSize() << " messages and " << clientStore_ ->
             getSplitTotalStorageSize() << " bytes to send in "
          << "the split stores.\n" << logofs_flush;

  clientStore_ -> dumpSplitStore(splitState_.resource);

  #endif

  return 1;
}

int ClientChannel::handleSplit(EncodeBuffer &encodeBuffer)
{
  //
  // Determine the maximum amount of bytes
  // we can write in this iteration.
  //

  int total = control -> SplitDataPacketLimit;

  int bytes  = total;
  int splits = 0;

  #if defined(TEST) || defined(SPLIT)
  *logofs << "handleSplit: SPLIT! Handling splits "
          << "for FD#" << fd_ << " with " << clientStore_ ->
             getSplitTotalSize() << " elements and " << total
          << " bytes to write at " << strMsTimestamp() << ".\n"
          << logofs_flush;
  #endif

  if (proxy -> handleAsyncSwitch(fd_) < 0)
  {
    return -1;
  }

  #if defined(TEST) || defined(SPLIT)
  *logofs << "handleSplit: SPLIT! Looping to find "
          << "if there is any split to send.\n"
          << logofs_flush;
  #endif

  SplitStore *splitStore;

  Split *splitMessage;

  //
  // Divide the available bandwidth among all the active
  // split stores by implementing a simple round-robin
  // mechanism. This can be extended by using an external
  // function returning the number of bytes to be written
  // based on the state of the split (splits which didn't
  // receive yet a confirmation event could be delayed),
  // the current bitrate, and by letting the agent asso-
  // ciate a priority to the resource in the start split
  // operation.
  //

  splitState_.pending = 0;

  splitResources_.rotate();

  //
  // Copy the list since elements can be removed
  // in the middle of the loop.
  //

  T_list splitList = splitResources_.copyList();

  for (T_list::iterator j = splitList.begin();
           j != splitList.end(); j++)
  {
    int resource = *j;

    #ifdef DEBUG
    *logofs << "handleSplit: SPLIT! Looping with current "
            << "resource " << resource << ".\n"
            << logofs_flush;
    #endif

    splitStore = clientStore_ -> getSplitStore(resource);

    if (splitStore != NULL)
    {
      //
      // Don't send more than the the packet size
      // bytes but ensure that we abort any split
      // found in the disk cache.
      //

      for (;;)
      {
        #if defined(TEST) || defined(SPLIT)

        clientStore_ -> dumpSplitStore(resource);

        #endif

        splitMessage = splitStore -> getFirstSplit();

        if (splitMessage == NULL)
        {
          //
          // We have created the store after a start
          // split but no message was added yet.
          //

          #if defined(TEST) || defined(SPLIT)
          *logofs << "handleSplit: WARNING! SPLIT! The split store "
                  << "is still empty.\n" << logofs_flush;
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
          *logofs << "handleSplit: PANIC! SPLIT! Found an "
                  << "aborted split in store [" << resource
                  << "].\n" << logofs_flush;

          HandleCleanup();
        }

        #endif

        //
        // Check if there are more messages in the
        // store that can be aborted or if we have
        // exceeded the number of bytes we can send
        // for this iteration.
        //

        #if defined(TEST) || defined(SPLIT)
        *logofs << "handleSplit: SPLIT! Checking closure "
                << "of the inner loop with " << bytes
                << " bytes to write and split state ["
                << DumpState(splitMessage -> getState())
                << "].\n" << logofs_flush;
        #endif

        if ((splitMessage -> getMode() == split_sync &&
                splitMessage -> getState() == split_added) ||
                    (bytes <= 0 && splitMessage ->
                        getState() != split_loaded))
        {
          break;
        }

        //
        // If the split was loaded at the remote
        // side abort it immediately.
        //

        if (splitMessage -> getState() == split_loaded)
        {
          #if defined(TEST) || defined(SPLIT)
          *logofs << "handleSplit: SPLIT! Sending more data "
                  << "for store [" << resource << "] with "
                  << "a split to be aborted.\n"
                  << logofs_flush;
          #endif

          if (handleSplitSend(encodeBuffer, resource, splits, bytes) < 0)
          {
            return -1;
          }
        }
        else if (bytes > 0)
        {
          #if defined(TEST) || defined(SPLIT)
          *logofs << "handleSplit: SPLIT! Sending more data "
                  << "for store [" << resource << "] with "
                  << bytes << " bytes to send.\n"
                  << logofs_flush;
          #endif

          if (handleSplitSend(encodeBuffer, resource, splits, bytes) < 0)
          {
            return -1;
          }
        }

        //
        // Check if the split store was deleted.
        //

        splitStore = clientStore_ -> getSplitStore(resource);

        if (splitStore == NULL)
        {
          #if defined(TEST) || defined(SPLIT)
          *logofs << "handleSplit: SPLIT! Exiting from the "
                  << "inner loop with split store [" << resource
                  << "] destroyed.\n" << logofs_flush;
          #endif

          break;
        }
      }

      #if defined(TEST) || defined(SPLIT)
      *logofs << "handleSplit: SPLIT! Completed handling splits "
              << "for store [" << resource << "] with " << bytes
              << " bytes still to send.\n" << logofs_flush;
      #endif

      //
      // Check if there is still a split to
      // send for the store just processed.
      //

      handleSplitPending(resource);
    }
  }

  #if defined(TEST) || defined(SPLIT)

  if (splits == 0)
  {
    #ifdef PANIC
    *logofs << "handleSplit: PANIC! Function called but "
            << "no split message was sent.\n"
            << logofs_flush;
    #endif

    HandleCleanup();
  }

  *logofs << "handleSplit: SPLIT! Sent " << splits
          << " splits and " << total - bytes << " bytes for FD#" << fd_
          << " with " << clientStore_ -> getSplitTotalStorageSize()
          << " bytes and [" << clientStore_ -> getSplitTotalSize()
          << "] splits remaining.\n" << logofs_flush;

  *logofs << "handleSplit: SPLIT! The pending split flag is "
          << splitState_.pending << " with " << clientStore_ ->
             getSplitTotalSize() << " splits in the split stores.\n"
          << logofs_flush;

  clientStore_ -> dumpSplitStores();

  #endif

  return 1;
}

int ClientChannel::handleSplitSend(EncodeBuffer &encodeBuffer, int resource,
                                       int &splits, int &bytes)
{
  #if defined(TEST) || defined(SPLIT) 

  SplitStore *splitStore = clientStore_ -> getSplitStore(resource);

  Split *splitMessage = splitStore -> getFirstSplit();

  if (splitStore -> getResource() != resource ||
          splitMessage -> getResource() != resource)
  {
    #ifdef PANIC
    *logofs << "handleSplitSend: PANIC! The resource doesn't "
            << "match the split store.\n" << logofs_flush;
    #endif

    HandleCleanup();
  }

  *logofs << "handleSplitSend: SPLIT! Sending message "
          << "OPCODE#" << (unsigned) opcodeStore_ -> splitData
          << " for resource " << splitMessage -> getResource()
          << " with request " << splitMessage -> getRequest()
          << " position " << splitMessage -> getPosition()
          << " and " << bytes << " bytes to write.\n"
          << logofs_flush;
  #endif

  //
  // Use a special opcode to signal the other
  // side this is part of a split and not a
  // new message.
  //

  encodeBuffer.encodeOpcodeValue(opcodeStore_ -> splitData,
                                     clientCache_ -> opcodeCache);

  encodeBuffer.encodeCachedValue(resource, 8,
                     clientCache_ -> resourceCache);

  int result = clientStore_ -> getSplitStore(resource) ->
                     send(encodeBuffer, bytes);

  if (result < 0)
  {
    #ifdef PANIC
    *logofs << "handleSplit: PANIC! Error sending splits for FD#"
            << fd_ << ".\n" << logofs_flush;
    #endif

    cerr << "Error" << ": Error sending splits for FD#"
           << fd_ << ".\n";

    return -1;
  }

  //
  // Get the bits written and update the
  // statistics for this special opcode.
  //

  int bits = encodeBuffer.diffBits();

  #if defined(TEST) || defined(SPLIT)|| defined(OPCODES)
  *logofs << "handleSplitSend: SPLIT! Handled request OPCODE#"
          << (unsigned int) opcodeStore_ -> splitData << " ("
          << DumpOpcode(opcodeStore_ -> splitData) << ")" << " for FD#"
          << fd_ << " sequence none. 0 bytes in, " << bits << " bits ("
          << ((float) bits) / 8 << " bytes) out.\n" << logofs_flush;
  #endif

  statistics -> addRequestBits(opcodeStore_ -> splitData, 0, bits);

  bytes -= bits >> 3;

  splits++;

  if (result == 1)
  {
    #if defined(TEST) || defined(SPLIT)
    *logofs << "handleSplitSend: SPLIT! Split at the head "
            << "of the list was completely transferred.\n"
            << logofs_flush;
    #endif

    //
    // The split at the head of the list was
    // completely transferred.
    //

    handleRestart(sequence_deferred, resource);
  }
  #if defined(TEST) || defined(SPLIT)
  else
  {
    *logofs << "handleSplitSend: SPLIT! More data to send "
            << "for the split at the head of the list.\n"
            << logofs_flush;
  }
  #endif

  return result;
}

int ClientChannel::handleSplitChecksum(EncodeBuffer &encodeBuffer, T_checksum checksum)
{
  //
  // Send the checksum only if the loading
  // or the saving of the message to the
  // persistent image cache is enabled.
  //

  if ((control -> ImageCacheEnableLoad == 1 ||
          control -> ImageCacheEnableSave == 1) &&
              (enableLoad_ == 1 || enableSave_ == 1))
  {
    encodeBuffer.encodeBoolValue(1);

    for (unsigned int i = 0; i < MD5_LENGTH; i++)
    {
      encodeBuffer.encodeValue((unsigned int) checksum[i], 8);
    }

    #if defined(TEST) || defined(SPLIT)
    *logofs << "handleSplitChecksum: SPLIT! Sent checksum "
            <<  "[" << DumpChecksum(checksum) << "].\n"
            << logofs_flush;
    #endif

    return 1;
  }
  else
  {
    encodeBuffer.encodeBoolValue(0);

    return 0;
  }
}

void ClientChannel::handleSplitPending()
{
  #if defined(TEST) || defined(SPLIT)

  int previous = splitState_.pending;

  #endif

  if (clientStore_ -> getSplitTotalSize() == 0)
  {
    splitState_.pending = 0;

    #if defined(TEST) || defined(SPLIT)
    *logofs << "handleSplitPending: SPLIT! Set the pending "
            << "split flag to " << splitState_.pending
            << " with split stores empty.\n"
            << logofs_flush;
    #endif
  }
  else
  {
    //
    // Loop through the stores to find if
    // there is any split that has become
    // ready.
    //

    #if defined(TEST) || defined(SPLIT)
    *logofs << "handleSplitPending: WARNING! SPLIT! Looping to "
            << "find if there is any split pending.\n"
            << logofs_flush;
    #endif

    splitState_.pending = 0;

    T_list &splitList = splitResources_.getList();

    for (T_list::iterator j = splitList.begin();
             j != splitList.end(); j++)
    {
      int resource = *j;

      SplitStore *splitStore = clientStore_ -> getSplitStore(resource);

      if (splitStore != NULL)
      {
        #if defined(TEST) || defined(SPLIT)

        clientStore_ -> dumpSplitStore(resource);

        #endif

        Split *splitMessage = splitStore -> getFirstSplit();

        if (splitMessage != NULL && canSendSplit(splitMessage) == 1)
        {
          #if defined(TEST) || defined(SPLIT)
          *logofs << "handleSplitPending: SPLIT! Found a pending "
                  << "split in store [" << resource << "].\n"
                  << logofs_flush;
          #endif

          splitState_.pending = 1;

          #if defined(TEST) || defined(SPLIT)

          if (splitMessage -> getState() == split_loaded)
          {
            *logofs << "handleSplitPending: PANIC! SPLIT! Found a "
                    << "loaded split in store [" << resource
                    << "].\n" << logofs_flush;

            HandleCleanup();
          }

          #endif

          break;
        }
      }
    }

    #if defined(TEST) || defined(SPLIT)
    *logofs << "handleSplitPending: SPLIT! Set the pending "
            << "split flag to " << splitState_.pending
            << " with " << clientStore_ -> getSplitTotalSize()
            << " splits in the split stores.\n"
            << logofs_flush;
    #endif
  }

  #if defined(TEST) || defined(SPLIT)

  if (splitState_.pending != previous)
  {
    *logofs << "handleSplitPending: SPLIT! Pending state "
            << "changed from " << previous << " to "
            << splitState_.pending << ".\n"
            << logofs_flush;
  }

  #endif
}

int ClientChannel::handleSplitEvent(EncodeBuffer &encodeBuffer, Split *splitMessage)
{
  SplitStore *splitStore;

  int resource = splitMessage -> getResource();

  #if defined(TEST) || defined(INFO)

  splitStore = clientStore_ -> getSplitStore(resource);

  if (splitStore == NULL)
  {
    #ifdef PANIC
    *logofs << "handleSplitEvent: PANIC! The split store can't "
            << "be NULL handling abort splits.\n"
            << logofs_flush;
    #endif

    HandleCleanup();
  }
  else if (splitMessage -> getState() != split_loaded)
  {
    *logofs << "handleSplitEvent: PANIC! Can't find the split "
            << "to be aborted.\n" << logofs_flush;

    HandleCleanup();
  }

  #endif

  //
  // Send any split that it is possible to
  // abort until the store is either empty
  // or the next split can't be aborted.
  //

  if (proxy -> handleAsyncSwitch(fd_) < 0)
  {
    return -1;
  }

  while ((splitStore = clientStore_ ->
             getSplitStore(resource)) != NULL &&
                 (splitMessage = splitStore -> getFirstSplit()) != NULL &&
                     splitMessage -> getState() == split_loaded)
  {
    #if defined(TEST) || defined(SPLIT)
    *logofs << "handleSplitEvent: SPLIT! Aborting split with "
            << "checksum [" << DumpChecksum(splitMessage ->
               getChecksum()) << "] for resource " << resource
            << " at " << strMsTimestamp() << ".\n"
            << logofs_flush;
    #endif

    int any = 0;

    if (handleSplitSend(encodeBuffer, resource, any, any) < 0)
    {
      return -1;
    }
  }

  #if defined(TEST) || defined(SPLIT)

  if ((splitStore = clientStore_ ->
          getSplitStore(resource)) == NULL)
  {
    *logofs << "handleSplitEvent: SPLIT! The split store ["
            << resource << "] has been destroyed.\n"
            << logofs_flush;
  }
  else if ((splitMessage = splitStore ->
               getFirstSplit()) == NULL)
  {
    *logofs << "handleSplitEvent: SPLIT! The split store ["
            << resource << "] is empty.\n"
            << logofs_flush;
  }
  else if (splitMessage -> getState() != split_loaded)
  {
    *logofs << "handleSplitEvent: SPLIT! The split at the "
            << "head of store [" << resource << "] doesn't "
            << "need to be aborted.\n" << logofs_flush;
  }

  #endif

  return 1;
}

int ClientChannel::handleSplitEvent(DecodeBuffer &decodeBuffer)
{
  #if defined(TEST) || defined(SPLIT)
  *logofs << "handleSplitEvent: SPLIT! Handling abort "
          << "split messages for FD#" << fd_ << " at "
          << strMsTimestamp() << ".\n" << logofs_flush;
  #endif

  if (control -> isProtoStep7() == 0)
  {
    #ifdef PANIC
    *logofs << "handleSplitEvent: PANIC! The split can't "
            << "be aborted when connected to an old "
            << "proxy version.\n" << logofs_flush;
    #endif

    HandleCleanup();
  }

  //
  // Decode the information about the
  // message to be updated.
  //

  unsigned char resource;

  decodeBuffer.decodeCachedValue(resource, 8,
                     serverCache_ -> resourceCache);

  unsigned int loaded;

  decodeBuffer.decodeBoolValue(loaded);

  unsigned char request;
  unsigned int  size;

  if (loaded == 1)
  {
    decodeBuffer.decodeOpcodeValue(request, serverCache_ -> abortOpcodeCache);

    decodeBuffer.decodeValue(size, 32, 14);
  }
  else
  {
    request = 0;
    size    = 0;
  }

  unsigned int value;

  md5_byte_t checksum[MD5_LENGTH];

  for (unsigned int i = 0; i < MD5_LENGTH; i++)
  {
    decodeBuffer.decodeValue(value, 8);

    checksum[i] = (unsigned char) value;
  }

  #if defined(TEST) || defined(SPLIT)
  *logofs << "handleSplitEvent: SPLIT! Checking split "
          << "with checksum [" << DumpChecksum(checksum)
          << "] loaded " << loaded << " request " << (unsigned int)
             request << " compressed size " << size << " at "
          << strMsTimestamp() << ".\n" << logofs_flush;
  #endif

  Split *splitMessage = handleSplitFind(checksum, resource);

  if (splitMessage != NULL)
  {
    if (loaded == 1)
    {
      #if defined(TEST) || defined(SPLIT)
      *logofs << "handleSplitEvent: SPLIT! Marked split with "
              << "checksum [" << DumpChecksum(checksum) << "] "
              << "as [loaded] at " << strMsTimestamp() << ".\n"
              << logofs_flush;
      #endif

      splitMessage -> setState(split_loaded);

      #if defined(TEST) || defined(SPLIT)

      if (splitMessage -> compressedSize() != (int) size)
      {
        *logofs << "handleSplitEvent: WARNING! SPLIT! Updating "
                << "compressed data size from " << splitMessage ->
                   compressedSize() << " to " << size << ".\n"
                << logofs_flush;
      }

      #endif

      splitMessage -> compressedSize(size);

      //
      // The splits to be aborted are checked by the split
      // store at the time we are going to send a new chunk
      // of split data. The splits must be strictly handled
      // in the same order as they were added to the split
      // store and the split we want to abort here may be
      // not at the head of the list.
      //

      if (splitMessage == clientStore_ ->
              getSplitStore(resource) -> getFirstSplit())
      {
        //
        // We don't need to flush this packet immediately.
        // The abort can be sent at any time to the remote
        // proxy. What's important is that we restart the
        // agent resource as soon as possible.
        //

        #if defined(TEST) || defined(SPLIT)

        T_timestamp startTs = getTimestamp();

        *logofs << "handleSplitEvent: SPLIT! Encoding abort "
                << "split events for FD#" << fd_ << " with "
                << "resource " << (unsigned) resource << " at "
                << strMsTimestamp() << ".\n" << logofs_flush;
        #endif

        if (proxy -> handleAsyncSplit(fd_, splitMessage) < 0)
        {
          return -1;
        }

        #if defined(TEST) || defined(SPLIT)
        *logofs << "handleSplitEvent: SPLIT! Spent "
                << diffTimestamp(startTs, getTimestamp()) << " Ms "
                << "handling abort split events for FD#" << fd_
                << ".\n" << logofs_flush;
        #endif

        //
        // Check if we can clear the pending flag.
        //

        handleSplitPending();
      }
      #if defined(TEST) || defined(SPLIT)
      else
      {
        *logofs << "handleSplitEvent: WARNING! SPLIT! Abort split "
                << "event not sent because not at the head "
                << "of the list.\n" << logofs_flush;
      }
      #endif
    }
    else
    {
      #if defined(TEST) || defined(SPLIT)
      *logofs << "handleSplitEvent: SPLIT! Marked split with "
              << "checksum [" << DumpChecksum(checksum) << "] "
              << "as [missed] at " << strMsTimestamp() << ".\n"
              << logofs_flush;
      #endif

      splitMessage -> setState(split_missed);

      //
      // Check if we can set the pending flag.
      //

      handleSplitPending(resource);
    }
  }
  else
  {
    //
    // The split report came after the split was already
    // sent or the split store deleted. If the message
    // had been loaded from disk by the remote side, we
    // need to update the compressed size in our message
    // store or the checksum will not match at the time
    // we will try to save the message store on disk.
    //

    if (loaded == 1 && size != 0)
    {
      #if defined(TEST) || defined(SPLIT)
      *logofs << "handleSplitEvent: WARNING! SPLIT! Can't find "
              << "the split. Updating in the message store.\n"
              << logofs_flush;
      #endif

      MessageStore *store = clientStore_ -> getRequestStore(request);

      if (store != NULL)
      {
        store -> updateData(checksum, size);
      }
      #if defined(TEST) || defined(SPLIT)
      else
      {
        #ifdef PANIC
        *logofs << "handleSplitEvent: PANIC! The message store "
                << "can't be null.\n" << logofs_flush;
        #endif

        HandleCleanup();
      }
      #endif
    }
    #if defined(TEST) || defined(SPLIT)
    else
    {
      *logofs << "handleSplitEvent: WARNING! SPLIT! No need to "
              << "update the store with loaded " << loaded
              << " and compressed size " << size << ".\n"
              << logofs_flush;
    }
    #endif
  }

  return 1;
}

Split *ClientChannel::handleSplitFind(T_checksum checksum, int resource)
{
  //
  // It can be that we handled all the splits,
  // restarted the resource and deleted the
  // store before the event could even reach
  // our side.
  //

  SplitStore *splitStore = clientStore_ -> getSplitStore(resource);

  if (splitStore != NULL)
  {
    Split *splitMessage;

    T_splits *splitList = splitStore -> getSplits();

    for (T_splits::iterator i = splitList -> begin();
             i != splitList -> end(); i++)
    {
      splitMessage = (*i);

      if (splitMessage -> getChecksum() != NULL)
      {
        #if defined(TEST) || defined(SPLIT)
        *logofs << "handleSplitFind: SPLIT! Comparing with message ["
                << DumpChecksum(splitMessage -> getChecksum())
                << "].\n" << logofs_flush;
        #endif

        if (memcmp(checksum, splitMessage -> getChecksum(), MD5_LENGTH) == 0)
        {
          #if defined(TEST) || defined(SPLIT)
          *logofs << "handleSplitFind: SPLIT! Located split for "
                  << "checksum [" << DumpChecksum(checksum) << "] "
                  << "in store [" << splitStore -> getResource()
                  << "].\n" << logofs_flush;
          #endif

          return splitMessage;
        }
      }
    }
  }
  #if defined(TEST) || defined(SPLIT)
  else
  {
    *logofs << "handleSplitFind: WARNING! SPLIT! The split store "
            << "was already deleted.\n" << logofs_flush;
  }
  #endif

  #if defined(TEST) || defined(SPLIT)
  *logofs << "handleSplitFind: WARNING! SPLIT! Can't find the "
          << "split for checksum [" << DumpChecksum(checksum)
          << "].\n" << logofs_flush;
  #endif

  return NULL;
}

int ClientChannel::handleRestart(T_sequence_mode mode, int resource)
{
  //
  // The agent must send a start-split message, followed by the
  // X messages that may be optionally split by the proxy. Usu-
  // ally, in the middle of a start-split/end-split sequence is
  // a single PutImage() or PutPackedImage(), that, in turn,
  // can generate multiple partial requests, like a SetUnpack-
  // Colormap() and SetUnpackAlpha() followed by the image that
  // must be transferred. Multiple requests may be also genera-
  // ted because the maximum size of a X request has been exce-
  // eded, so that Xlib has divided the single image in multi-
  // ple sub-image requests. The agent doesn't need to take care
  // of that, except tracking the result of the split operation.
  //
  // By monitoring the notify events sent by the proxy, the
  // agent will have to implement its own strategy to deal with
  // its resources (for example its clients). For example:
  //
  // - It will issue a new image request and suspend a client
  //   if the image was not entirely sent in the main X oputput
  //   stream.
  //
  // - It will choose to commit or discard the messages after
  //   they are recomposed at the remote side. The set of mes-
  //   sages that will have to be committed will include all
  //   messages that were part of the split (the colormap, the
  //   alpha channel).
  //
  // - It will restart its own client, in the case it had been
  //   suspended.
  //
  // A more useful strategy would be to replace the original im-
  // age with a tiny 'placeholder' if a split took place, and
  // synchronize the content of the drawable at later time. This
  // is generally referred as 'lazy encoding'.
  //
  // The agent will be able to identify the original split ope-
  // ration (the one marked with the start-spit) by the small
  // integer number (0-255) referred to as the 'resource' field.
  //
  // Before the proxy will be able to report the status of the
  // split, the agent will have to close the sequence by issueing
  // an end-split. The proxy will then report the result of the
  // operation, so that the agent will have the option of suspend-
  // ing the client or marking the drawable as dirty and take
  // care of synchronizing it at later time.
  //
  // One of the following cases may be encountered:
  //
  // notify_no_split:     All messages were sent in the main out-
  //                      put stream, so that no split actually
  //                      took place.
  //
  // notify_start_split:  One or more messages were split, so,
  //                      at discrection of the agent, the client
  //                      may be suspended until the transferral
  //                      is completed.
  //
  // notify_commit_split: One of the requests that made up the
  //                      split was recomposed. The agent should
  //                      either commit the given request or tell
  //                      the proxy to discard it.
  //
  // notify_end_split:    The split was duly completed. The agent
  //                      can restart the client.
  //
  // notify_empty_split:  No more split operation are pending.
  //                      The agent can use this information to
  //                      implement specific strategies requiring
  //                      that all messages have been recomposed
  //                      at the remote end, like updating the
  //                      drawables that were not synchronized
  //                      because of the lazy encoding.
  //
  // By checking the split and commit store we can determine if we
  // need to send a new notification event to the agent. There can
  // be four different cases:
  //
  // - If the split store is not null and not empty, we are still
  //   in the middle of a split.
  //
  // - If the commit store is not empty, we completely recomposed
  //   a full message and can send a new commit notify.
  //
  // - If the split store has become empty, we recomposed all the
  //   messages added for the given resource, and so will be able
  //   to restart the resource.
  //
  // - If no more messages are in the split stores, we can notify
  //   an empty split event to the agent.
  //

  #if defined(TEST) || defined(SPLIT)
  *logofs << "handleRestart: SPLIT! Handling ["
          << (mode == sequence_immediate ? "immediate" : "deferred")
          << "] restart events for resource " << resource << " at "
          << strMsTimestamp() << ".\n" << logofs_flush;
  #endif

  SplitStore *splitStore = clientStore_ -> getSplitStore(resource);

  if (mode == sequence_immediate)
  {
    //
    // We have received an end-split request. If the store
    // was not deleted already, we mark the last split added
    // as the one ending the row for this resource. If the
    // commit() function returns 0 it means that the split
    // store is either empty or that we did not add any split
    // for this resource. This is because when connected to
    // an old proxy version we only have a single store for
    // all the resources.   
    //
    // It can happen that all the split messages that were
    // originally appended to the list were completely sent
    // before our client had the chance of ending the split
    // sequence. In this case the split store will be empty
    // or already deleted and so we will be able to restart
    // the resource.
    //

    #if defined(TEST) || defined(SPLIT)

    if (splitStore == NULL)
    {
      *logofs << "handleRestart: WARNING! SPLIT! Split store ["
              << resource << "] was already deleted.\n"
              << logofs_flush;
    }
    else
    {
      clientStore_ -> dumpSplitStore(resource);
    }

    #endif

    if (splitStore == NULL || splitStore -> getSize() == 0)
    {
      #if defined(TEST) || defined(SPLIT)
      *logofs << "handleRestart: SPLIT! Immediate agent split event "
              << "TYPE#" << (unsigned) opcodeStore_ -> noSplitNotify
              << " [no split] with resource " << resource
              << " at " << strMsTimestamp() << ".\n"
              << logofs_flush;
      #endif

      if (handleNotify(notify_no_split, sequence_immediate,
                           resource, nothing, nothing) < 0)
      {
        return -1;
      }
    }
    else
    {
      #if defined(TEST) || defined(SPLIT)
      *logofs << "handleRestart: SPLIT! Immediate agent split event "
              << "TYPE#" << (unsigned) opcodeStore_ -> startSplitNotify
              << " [start split] with resource " << resource
              << " at " << strMsTimestamp() << ".\n"
              << logofs_flush;
      #endif

      if (handleNotify(notify_start_split, sequence_immediate,
                           resource, nothing, nothing) < 0)
      {
        return -1;
      }
    }
  }
  else
  {
    //
    // We have completely transferred a message
    // that was put in the split store.
    //
    // The id of the resource can be different
    // than the index of the store if we are
    // connected to an old proxy.
    //

    #if defined(TEST) || defined(SPLIT)

    if (splitStore == NULL)
    {
      #ifdef PANIC
      *logofs << "handleRestart: PANIC! The split store can't "
              << "be NULL handling deferred restart events.\n"
              << logofs_flush;
      #endif

      HandleCleanup();
    }
    else
    {
      clientStore_ -> dumpSplitStore(resource);
    }

    #endif

    CommitStore *commitStore = clientStore_ -> getCommitStore();

    #if defined(TEST) || defined(SPLIT)

    clientStore_ -> dumpCommitStore();

    #endif

    //
    // Check if there is any commit to notify.
    //

    Split *split;

    T_splits *commitList = commitStore -> getSplits();

    for (T_splits::iterator i = commitList -> begin();
             i != commitList -> end(); i++)
    {
      split = *i;

      if (split -> getState() != split_notified)
      {
        #if defined(TEST) || defined(SPLIT) 

        if (split -> getResource() != resource)
        {
          #ifdef PANIC
          *logofs << "handleSplitSend: PANIC! The resource doesn't "
                  << "match the split store.\n" << logofs_flush;
          #endif

          HandleCleanup();
        }

        #endif

        int request  = split -> getRequest();
        int position = split -> getPosition();

        #if defined(TEST) || defined(SPLIT)
        *logofs << "handleRestart: SPLIT! Deferred agent split event "
                << "TYPE#" << (unsigned) opcodeStore_ -> commitSplitNotify
                << " [commit split] with resource " << resource << " request "
                << request << " position " << position << " at "
                << strMsTimestamp() << ".\n" << logofs_flush;
        #endif

        if (handleNotify(notify_commit_split, sequence_deferred,
                            resource, request, position) < 0)
        {
          return -1;
        }

        //
        // Don't send the notification again.
        //

        split -> setState(split_notified);
      }
      #if defined(TEST) || defined(SPLIT)
      else
      {
        *logofs << "handleRestart: SPLIT! Split for request "
                << split -> getRequest() << " and position "
                << split -> getPosition() << " was already "
                << "notified.\n" << logofs_flush;
      }
      #endif
    }

    //
    // Don't send the end split if we are still
    // in the middle of a start-split/end-split
    // sequence. We'll send a no-split at the
    // time the end-split is received.
    //

    if (splitStore -> getSize() == 0 &&
            splitStore -> getResource() != splitState_.resource)
    {
      #if defined(TEST) || defined(SPLIT)
      *logofs << "handleRestart: SPLIT! Deferred agent split event "
              << "TYPE#" << (unsigned) opcodeStore_ -> endSplitNotify
              << " [end split] with resource " << resource << " at "
              << strMsTimestamp() << ".\n" << logofs_flush;
      #endif

      if (handleNotify(notify_end_split, sequence_deferred,
                           resource, nothing, nothing) < 0)
      {
        return -1;
      }
    }
    #if defined(TEST) || defined(SPLIT)
    else if (splitStore -> getSize() == 0 &&
                splitStore -> getResource() == splitState_.resource)
    {
      *logofs << "handleRestart: SPLIT! WARNING! The split store "
              << "for resource " << resource << " was emptied in the "
              << "split sequence at " << strMsTimestamp() << ".\n"
              << logofs_flush;
    }
    #endif
  }

  //
  // Remove the split store if it's empty.
  //

  if (splitStore != NULL && splitStore -> getSize() == 0 &&
          splitStore -> getResource() != splitState_.resource)
  {
    #if defined(TEST) || defined(SPLIT)
    *logofs << "handleRestart: SPLIT! Removing the split store ["
            << resource << "] at " << strMsTimestamp()
            << ".\n" << logofs_flush;
    #endif

    handleSplitStoreRemove(&splitResources_, resource);

    if (clientStore_ -> getSplitTotalSize() == 0)
    {
      #if defined(TEST) || defined(SPLIT)
      *logofs << "handleRestart: SPLIT! Deferred agent split event "
              << "TYPE#" << (unsigned) opcodeStore_ -> emptySplitNotify
              << " [empty split] for FD#" << fd_ << " at "
              << strMsTimestamp() << ".\n" << logofs_flush;
      #endif

      if (handleNotify(notify_empty_split, sequence_deferred,
                           nothing, nothing, nothing) < 0)
      {
        return -1;
      }
    }

    #if defined(TEST) || defined(SPLIT)
    *logofs << "handleRestart: SPLIT! There are " << clientStore_ ->
               getSplitTotalSize() << " messages and " << clientStore_ ->
               getSplitTotalStorageSize() << " bytes to send in "
            << "the split stores.\n" << logofs_flush;

    if ((clientStore_ -> getSplitTotalSize() != 0 &&
            clientStore_ -> getSplitTotalStorageSize() == 0) ||
                (clientStore_ -> getSplitTotalSize() == 0 &&
                    clientStore_ -> getSplitTotalStorageSize() != 0))
    {
      #ifdef PANIC
      *logofs << "handleRestart: PANIC! Inconsistency detected "
              << "while handling the split stores.\n"
              << logofs_flush;
      #endif

      HandleCleanup();
    }

    #endif
  }

  return 1;
}

int ClientChannel::handleTaintCacheRequest(unsigned char &opcode, const unsigned char *&buffer,
                                               unsigned int &size)
{
  #ifdef TEST
  *logofs << "handleTaintCacheRequest: Tainting cache request "
          << "for FD#" << fd_ << ".\n" << logofs_flush;
  #endif

  //
  // The save and load flags would affect
  // the decoding side but the decoding
  // side doesn't support the request.
  //

  enableCache_ = *(buffer + 4);
  enableSplit_ = *(buffer + 5);

  handleSplitEnable();

  #ifdef TEST
  *logofs << "handleTaintCacheRequest: Set cache parameters to "
          << "cache " << enableCache_ << " split " << enableSplit_
          << " load " << enableLoad_ << " save " << enableSave_
          << ".\n" << logofs_flush;
  #endif

  //
  // Taint the request to a X_NoOperation.
  //

  opcode = X_NoOperation;

  return 0;
}

int ClientChannel::handleTaintFontRequest(unsigned char &opcode, const unsigned char *&buffer,
                                              unsigned int &size)
{
  //
  // The remote end doesn't support this
  // request so generate an empty reply
  // at the local side.
  //

  #ifdef TEST
  *logofs << "handleTaintFontRequest: Suppressing font "
          << "request for FD#" << fd_ << ".\n"
          << logofs_flush;
  #endif

  //
  // The client sequence number has not
  // been incremented yet in the loop.
  //

  unsigned int sequence = (clientSequence_ + 1) & 0xffff;

  #ifdef TEST
  *logofs << "handleTaintFontRequest: Opcode is " << (unsigned) opcode
          << " expected client sequence is " << sequence
          << ".\n" << logofs_flush;
  #endif

  unsigned char *reply = writeBuffer_.addMessage(36);

  *(reply + 0) = X_Reply;

  PutUINT(sequence, reply + 2, bigEndian_);

  PutULONG(1, reply + 4, bigEndian_);

  //
  // Set the length of the returned
  // path to 0.
  //

  *(reply + 32) = 0;

  //
  // Save the sequence number, not incremented
  // yet, we used to auto-generate this reply.
  //

  lastSequence_ = clientSequence_ + 1;

  #ifdef TEST
  *logofs << "handleTaintFontRequest: Registered " << lastSequence_
          << " as last auto-generated sequence number.\n"
          << logofs_flush;
  #endif

  //
  // Taint the request to a X_NoOperation.
  //

  opcode = X_NoOperation;

  if (handleFlush(flush_if_any) < 0)
  {
    return -1;
  }

  return 1;
}

int ClientChannel::handleTaintSplitRequest(unsigned char &opcode, const unsigned char *&buffer,
                                               unsigned int &size)
{
  #ifdef TEST

  if (opcode == opcodeStore_ -> abortSplit)
  {
    *logofs << "handleTaintSplitRequest: Tainting abort split "
            << "request for FD#" << fd_ << ".\n"
            << logofs_flush;
  }
  else if (opcode == opcodeStore_ -> finishSplit)
  {
    *logofs << "handleTaintSplitRequest: Tainting finish split "
            << "request for FD#" << fd_ << ".\n"
            << logofs_flush;
  }
  else
  {
    *logofs << "handleTaintSplitRequest: Tainting free split "
            << "request for FD#" << fd_ << ".\n"
            << logofs_flush;
  }

  #endif

  //
  // Taint the request to a X_NoOperation.
  //

  opcode = X_NoOperation;

  return 1;
}

int ClientChannel::handleTaintLameRequest(unsigned char &opcode, const unsigned char *&buffer,
                                              unsigned int &size)
{
  //
  // Test the efficiency of the encoding
  // without these RENDER requests.
  //

  if (opcode == opcodeStore_ -> renderExtension &&
          (*(buffer + 1) == X_RenderCompositeGlyphs8 ||
               *(buffer + 1) == X_RenderCompositeGlyphs16 ||
                   *(buffer + 1) == X_RenderCompositeGlyphs32 ||
                       *(buffer + 1) == X_RenderAddGlyphs ||
                           *(buffer + 1) == X_RenderTrapezoids))
  {
    #ifdef TEST
    *logofs << "handleTaintLameRequest: Tainting request "
            << "OPCODE#" << (unsigned int) opcode << " MINOR#"
            << (unsigned int) *(buffer + 1) << " for FD#"
            << fd_ << ".\n" << logofs_flush;
    #endif

    opcode = X_NoOperation;

    return 1;
  }

  return 0;
}

int ClientChannel::handleTaintSyncRequest(unsigned char &opcode, const unsigned char *&buffer,
                                              unsigned int &size)
{
  //
  // Should short-circuit other common replies
  // whose values could be queried only once.
  // Examples are X_InterAtom, X_ListExtension
  // and X_QueryExtension.
  //

  if (taintCounter_ >= control -> TaintThreshold)
  {
    #ifdef DEBUG
    *logofs << "handleTaintSyncRequest: Reset taint counter after "
            << taintCounter_ << " replies managed.\n"
            << logofs_flush;
    #endif

    taintCounter_ = 0;

    return 0;
  }

  //
  // Check if we are rolling the counter.
  // The client sequence number has not
  // been incremented yet in the loop.
  //

  unsigned int sequence = (clientSequence_ + 1) & 0xffff;

  #ifdef DEBUG
  *logofs << "handleTaintSyncRequest: Opcode is " << (unsigned) opcode
          << " expected client sequence is " << sequence
          << ".\n" << logofs_flush;
  #endif

  if (sequence == 0xffff)
  {
    return 0;
  }

  unsigned short t1;
  unsigned char  t2;

  //
  // Check if there is a previous reply
  // pending.
  //

  if (sequenceQueue_.peek(t1, t2) != 0)
  {
    #ifdef DEBUG
    *logofs << "handleTaintSyncRequest: Skipping taint of reply due to "
            << "pending request OPCODE#" << t1 << " with sequence "
            << (unsigned int) t2 << ".\n" << logofs_flush;
    #endif

    return 0;
  }

  #ifdef DEBUG
  *logofs << "handleTaintSyncRequest: Suppressing get input focus "
          << "request for FD#" << fd_ << " with sequence "
          << sequence << ".\n" << logofs_flush;
  #endif

  unsigned char *reply = writeBuffer_.addMessage(32);

  *(reply + 0) = X_Reply;

  PutUINT(sequence, reply + 2, bigEndian_);

  PutULONG(0, reply + 4, bigEndian_);

  //
  // Set revert-to to none.
  //

  *(reply + 1) = 0;

  //
  // Set focus to none.
  //

  PutULONG(0, reply + 8, bigEndian_);

  //
  // Save the sequence number, not incremented
  // yet, we used to auto-generate this reply.
  //

  lastSequence_ = clientSequence_ + 1;

  #ifdef TEST
  *logofs << "handleTaintSyncRequest: Registered " << lastSequence_
          << " as last auto-generated sequence number.\n"
          << logofs_flush;
  #endif

  //
  // Taint the request to a X_NoOperation.
  //

  opcode = X_NoOperation;

  //
  // We may assume that the client has finished
  // drawing and flush immediately, even if this
  // seems to perceively affect the performance.
  //
  // priority_++;
  //

  if (handleFlush(flush_if_any) < 0)
  {
    return -1;
  }

  taintCounter_++;

  return 1;
}

int ClientChannel::handleTaintSyncError(unsigned char opcode)
{
  if (control -> TaintReplies > 0)
  {
    //
    // By enabling short-circuiting of replies
    // some window managers can get confused
    // by some otherwise innocuous X errors.
    //

    if (opcode == X_GrabKey || opcode == X_ReparentWindow ||
            opcode == X_ConfigureWindow)
    {
      #if defined(TEST) || defined(OPCODES)
      *logofs << "handleTaintSyncError: WARNING! Suppressed error "
              << "on OPCODE#" << (unsigned int) opcode << " for FD#"
              << fd_ << " sequence " << lastSequence_ << " (real was "
              << serverSequence_ << ").\n" << logofs_flush;
      #endif

      return 1;
    }
  }

  return 0;
}

int ClientChannel::handleNotify(T_notification_type type, T_sequence_mode mode,
                                    int resource, int request, int position)
{
  if (finish_ == 1)
  {
    #if defined(TEST) || defined(INFO)
    *logofs << "handleNotify: Discarding notification on "
            << "channel for FD#" << fd_ << ".\n"
            << logofs_flush;
    #endif

    return 0;
  }

  //
  // Add a new message to the write buffer.
  //

  unsigned char *event = writeBuffer_.addMessage(32);

  //
  // Event is ClientMessage, atom and
  // window are 0, format is 32.
  //

  *(event + 0) = ClientMessage;

  PutULONG(0, event + 4, bigEndian_);
  PutULONG(0, event + 8, bigEndian_);

  *(event + 1) = 32;

  //
  // If the event follows immediately the request (that is the
  // sequence mode is 'immediate') then the sequence number is
  // the one of the last request, else it should be the last
  // sequence number encoded by peer proxy but, as we are ins-
  // erting events in the stream, we must ensure that the se-
  // quence we send is not less than the last sequence we have
  // auto-generated.
  //

  if (mode == sequence_immediate)
  {
    //
    // Save the sequence number we used
    // to auto-generate this event.
    //

    lastSequence_ = clientSequence_;

    #if defined(TEST) || defined(INFO)
    *logofs << "handleNotify: Registered " << lastSequence_
            << " as last auto-generated sequence number.\n"
            << logofs_flush;
    #endif
  }
  else
  {
    if (serverSequence_ > lastSequence_)
    {
      #ifdef DEBUG
      *logofs << "handleNotify: Updating last event's sequence "
              << lastSequence_ << " to X server's sequence number "
              << serverSequence_ << " for FD#" << fd_ << ".\n"
              << logofs_flush;
      #endif

      lastSequence_ = serverSequence_;
    }
    #ifdef DEBUG
    else if (serverSequence_ < lastSequence_)
    {
      //
      // Use our last auto-generated sequence.
      //

      *logofs << "handleNotify: Tainting sequence number "
              << serverSequence_ << " to last event's sequence "
              << lastSequence_ << " for FD#" << fd_ << ".\n"
              << logofs_flush;
    }
    #endif
  }

  PutUINT(lastSequence_, event + 2, bigEndian_);

  //
  // Be sure we set to void the fields that
  // are not significant for the specific
  // notification message.
  //

  PutULONG(nothing, event + 16, bigEndian_);
  PutULONG(nothing, event + 20, bigEndian_);
  PutULONG(nothing, event + 24, bigEndian_);

  switch (type)
  {
    case notify_no_split:
    {
      PutULONG(opcodeStore_ -> noSplitNotify,
                   event + 12, bigEndian_);

      PutULONG(resource, event + 16, bigEndian_);

      break;
    }
    case notify_start_split:
    {
      PutULONG(opcodeStore_ -> startSplitNotify,
                   event + 12, bigEndian_);

      PutULONG(resource, event + 16, bigEndian_);

      break;
    }
    case notify_commit_split:
    {
      PutULONG(opcodeStore_ -> commitSplitNotify,
                   event + 12, bigEndian_);

      PutULONG(resource, event + 16, bigEndian_);

      PutULONG(request, event + 20, bigEndian_);

      PutULONG(position, event + 24, bigEndian_);

      break;
    }
    case notify_end_split:
    {
      PutULONG(opcodeStore_ -> endSplitNotify,
                   event + 12, bigEndian_);

      PutULONG(resource, event + 16, bigEndian_);

      break;
    }
    case notify_empty_split:
    {
      PutULONG(opcodeStore_ -> emptySplitNotify,
                   event + 12, bigEndian_);
      break;
    }
    default:
    {
      #ifdef PANIC
      *logofs << "handleNotify: PANIC! Unrecognized notify "
              << "TYPE#" << type << ".\n"
              << logofs_flush;
      #endif

      return -1;
    }
  }

  #if defined(TEST) || defined(INFO) || defined (SPLIT)

  *logofs << "handleNotify: Sending "
          << (mode ==  sequence_immediate ? "immediate " : "deferred ")
          << "agent notify event TYPE#" << GetULONG(event + 12, bigEndian_)
          << logofs_flush;

  if (resource != nothing)
  {
    *logofs << " with resource " << GetULONG(event + 16, bigEndian_)
            << logofs_flush;

    if (request != nothing && position != nothing)
    {
      *logofs << " request " << GetULONG(event + 20, bigEndian_)
              << " position " << GetULONG(event + 24, bigEndian_)
              << logofs_flush;
    }
  }

  *logofs << ".\n" << logofs_flush;

  #endif

  //
  // Send the notification now.
  //

  if (handleFlush(flush_if_any) < 0)
  {
    return -1;
  }

  return 1;
}

int ClientChannel::handleCommitSplitRequest(EncodeBuffer &encodeBuffer, const unsigned char opcode,
                                                const unsigned char *buffer, const unsigned int size)
{
  //
  // Get the data of the request to be
  // committed.
  //

  unsigned char request = *(buffer + 5);

  MessageStore *store = clientStore_ -> getRequestStore(request);

  if (store == NULL)
  {
    #ifdef PANIC
    *logofs << "handleCommitSplitRequest: PANIC! Can't commit split for "
            << "request OPCODE#" << (unsigned int) request
            << ". No message store found.\n"
            << logofs_flush;
    #endif

    cerr << "Error" << ": Can't commit split for request "
         << "OPCODE#" << (unsigned int) request
         << ". No message store found.\n";

    return -1;
  }

  //
  // The position in cache of the message
  // to commit. Encode it as difference in
  // respect to the last encoded value.
  //

  unsigned int position = GetULONG(buffer + 8, bigEndian_);

  unsigned char resource = *(buffer + 1);
  unsigned int  commit   = *(buffer + 4);

  #if defined(TEST) || defined(SPLIT)

  if (commit == 1)
  {
    *logofs << "handleCommitSplitRequest: SPLIT! Committing request "
            << "OPCODE#" << (unsigned) request << " at position "
            << position << " for FD#" << fd_ << " with resource "
            << (unsigned) resource << ".\n" << logofs_flush;
  }
  else
  {
    *logofs << "handleCommitSplitRequest: SPLIT! Discarding request "
            << "OPCODE#" << (unsigned) request << " at position "
            << position << " for FD#" << fd_ << " with resource "
            << (unsigned) resource << ".\n" << logofs_flush;
  }

  #endif

  encodeBuffer.encodeOpcodeValue(request, clientCache_ -> opcodeCache);

  int diffCommit = position - splitState_.commit;

  splitState_.commit = position;

  encodeBuffer.encodeValue(diffCommit, 32, 5);

  //
  // Send the resource id and the commit
  // flag.
  //

  encodeBuffer.encodeCachedValue(resource, 8,
                     clientCache_ -> resourceCache);

  encodeBuffer.encodeBoolValue(commit);

  //
  // Remove the split from the split queue.
  //

  Split *split = handleSplitCommitRemove(request, resource, splitState_.commit);

  if (split == NULL)
  {
    return -1;
  }

  clientStore_ -> getCommitStore() -> update(split);

  //
  // Free the split.
  //

  #if defined(TEST) || defined(SPLIT)
  *logofs << "handleCommitSplitRequest: SPLIT! Freeing up the "
          << "committed split.\n" << logofs_flush;
  #endif

  delete split;

  return 1;
}

int ClientChannel::handleAbortSplitRequest(EncodeBuffer &encodeBuffer, const unsigned char opcode,
                                               const unsigned char *buffer, const unsigned int size)
{
  unsigned char resource = *(buffer + 1);

  #if defined(TEST) || defined(SPLIT)
  *logofs << "handleAbortSplitRequest: SPLIT! Handling abort split "
          << "request for FD#"<< fd_ << " and resource "
          << (unsigned int) resource << ".\n"
          << logofs_flush;
  #endif

  encodeBuffer.encodeCachedValue(resource, 8,
                     clientCache_ -> resourceCache);

  SplitStore *splitStore = clientStore_ -> getSplitStore(resource);

  if (splitStore == NULL)
  {
    #ifdef WARNING
    *logofs << "handleAbortSplitRequest: WARNING! SPLIT! The split "
            << "store [" << (unsigned int) resource << "] "
            << "is already empty.\n" << logofs_flush;
    #endif

    return 0;
  }

  //
  // Loop through the messages in the split
  // store and discard from the memory cache
  // the messages that are still incomplete.
  // Then remove the message from the split
  // store.
  //

  #if defined(TEST) || defined(SPLIT)

  clientStore_ -> dumpSplitStore(resource);

  #endif

  int splits = 0;

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
                << "split store [" << (unsigned int) resource
                << "] is unexpectedly empty.\n"
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
                                               use_checksum, discard_data);
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

  //
  // If the start-split/end-split sequence
  // was closed, send the notification now,
  // else wait for the end-split.
  //

  if (resource != splitState_.resource)
  {
    #if defined(TEST) || defined(SPLIT)
    *logofs << "handleAbortSplitRequest: SPLIT! Sending the "
            << "deferred [end split] event.\n"
           << logofs_flush;
    #endif

    handleRestart(sequence_deferred, resource);
  }
  #if defined(TEST) || defined(SPLIT)
  else
  {
    *logofs << "handleAbortSplitRequest: WARNING! SPLIT! Still "
            << "waiting for the closure of the split "
            << "sequence.\n" << logofs_flush;
  }
  #endif

  //
  // Check if there is any other store
  // having splits to send.
  //

  handleSplitPending();

  return (splits > 0);
}

int ClientChannel::handleFinishSplitRequest(EncodeBuffer &encodeBuffer, const unsigned char opcode,
                                                const unsigned char *buffer, const unsigned int size)
{
  unsigned char resource = *(buffer + 1);

  #if defined(TEST) || defined(SPLIT)
  *logofs << "handleFinishSplitRequest: SPLIT! Handling finish split "
          << "request for FD#"<< fd_ << " and resource "
          << (unsigned int) resource << ".\n"
          << logofs_flush;
  #endif

  encodeBuffer.encodeCachedValue(resource, 8,
                     clientCache_ -> resourceCache);

  //
  // We need to get the protocol statistics
  // for the finish message we are handling
  // here because sending a new split will
  // reset the bits counter.
  //

  int bits = encodeBuffer.diffBits();

  statistics -> addRequestBits(opcode, size << 3, bits);

  SplitStore *splitStore = clientStore_ -> getSplitStore(resource);

  if (splitStore == NULL)
  {
    #ifdef WARNING
    *logofs << "handleFinishSplitRequest: WARNING! SPLIT! The split "
            << "store [" << (unsigned int) resource << "] "
            << "is already empty.\n" << logofs_flush;
    #endif

    return 0;
  }

  //
  // Send all the split queued for the given
  // resource until the split store becomes
  // empty.
  //

  #if defined(TEST) || defined(SPLIT)

  clientStore_ -> dumpSplitStore(resource);

  #endif

  Split *splitMessage;

  int total = MESSAGE_DATA_LIMIT;

  int bytes  = total;
  int splits = 0;

  for (;;)
  {
    splitMessage = splitStore -> getFirstSplit();

    if (splitMessage == NULL)
    {
      //
      // We have presumably created the store
      // after a start split but no message
      // was added yet.
      //

      #ifdef WARNING
      *logofs << "handleFinishSplitRequest: WARNING! SPLIT! The "
              << "split store [" << (unsigned int) resource
              << "] is unexpectedly empty.\n"
              << logofs_flush;
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
      *logofs << "handleFinishSplitRequest: PANIC! SPLIT! Found an "
              << "aborted split in store [" << (unsigned int) resource
              << "].\n" << logofs_flush;

      HandleCleanup();
    }

    *logofs << "handleFinishSplitRequest: SPLIT! Sending more "
            << "data for store [" << (unsigned int) resource
            << "].\n" << logofs_flush;
    #endif

    if (handleSplitSend(encodeBuffer, resource, splits, bytes) < 0)
    {
      return -1;
    }

    //
    // Check if the split store was deleted.
    //

    if (clientStore_ -> getSplitStore(resource) == NULL)
    {
      #if defined(TEST) || defined(SPLIT)
      *logofs << "handleFinishSplitRequest: SPLIT! Exiting "
              << "from the finish loop with split store ["
              << (unsigned int) resource << "] destroyed.\n"
              << logofs_flush;
      #endif

      break;
    }
  }

  //
  // Check if there is any other store
  // having splits to send.
  //

  handleSplitPending();

  #if defined(TEST) || defined(SPLIT)
  *logofs << "handleFinishSplitRequest: SPLIT! Sent " << splits
          << " splits and " << total - bytes << " bytes for FD#" << fd_
          << " with " << clientStore_ -> getSplitTotalStorageSize()
          << " bytes and [" << clientStore_ -> getSplitTotalSize()
          << "] splits remaining.\n" << logofs_flush;
  #endif

  return (splits > 0);
}

int ClientChannel::handleConfiguration()
{
  #ifdef TEST
  *logofs << "ClientChannel: Setting new buffer parameters.\n"
          << logofs_flush;
  #endif

  readBuffer_.setSize(control -> ClientInitialReadSize,
                          control -> ClientMaximumBufferSize);

  writeBuffer_.setSize(control -> TransportXBufferSize,
                           control -> TransportXBufferThreshold,
                               control -> TransportMaximumBufferSize);

  transport_ -> setSize(control -> TransportXBufferSize,
                            control -> TransportXBufferThreshold,
                                control -> TransportMaximumBufferSize);

  return 1;
}

int ClientChannel::handleFinish()
{
  #ifdef TEST
  *logofs << "ClientChannel: Finishing channel for FD#"
          << fd_ << ".\n" << logofs_flush;
  #endif

  congestion_ = 0;
  priority_   = 0;

  finish_ = 1;

  taintCounter_ = 0;

  splitState_.resource = nothing;
  splitState_.pending  = 0;
  splitState_.commit   = 0;
  splitState_.mode     = split_none;

  transport_ -> finish();

  return 1;
}

//
// If differential compression is disabled then use the
// most simple encoding but handle the image requests
// and the X_ListExtensions and X_QueryExtension messa-
// ges (needed to detect the opcode of the shape or the
// other extensions) in the usual way.
//

int ClientChannel::handleFastReadRequest(EncodeBuffer &encodeBuffer, const unsigned char &opcode,
                                             const unsigned char *&buffer, const unsigned int &size)
{
  //
  // All the NX requests are handled in the
  // main message loop. The X_PutImage can
  // be handled here only if the split was
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
  *logofs << "handleFastReadRequest: Encoding raw request OPCODE#"
          << (unsigned int) opcode << " for FD#" << fd_
          << " with size " << size << ".\n"
          << logofs_flush;
  #endif

  encodeBuffer.encodeMemory(buffer, size);

  //
  // Put request on the fast track
  // if it needs a reply.
  //

  switch (opcode)
  {
    case X_GetAtomName:
    case X_GetGeometry:
    case X_GetInputFocus:
    case X_GetModifierMapping:
    case X_GetKeyboardMapping:
    case X_GetProperty:
    case X_GetSelectionOwner:
    case X_GrabPointer:
    case X_GrabKeyboard:
    case X_ListExtensions:
    case X_ListFonts:
    case X_LookupColor:
    case X_AllocNamedColor:
    case X_QueryPointer:
    case X_GetWindowAttributes:
    case X_QueryTree:
    case X_QueryBestSize:
    case X_QueryColors:
    case X_QueryFont:
    case X_TranslateCoords:
    case X_GetImage:
    case X_GetPointerMapping:
    case X_GetKeyboardControl:
    case X_InternAtom:
    case X_AllocColor:
    {
      sequenceQueue_.push(clientSequence_, opcode);

      priority_++;

      break;
    }
    default:
    {
      break;
    }
  }

  int bits = encodeBuffer.diffBits();

  #if defined(TEST) || defined(OPCODES)

  *logofs << "handleFastReadRequest: Handled raw request OPCODE#" 
          << (unsigned int) opcode << " (" << DumpOpcode(opcode) << ")"
          << " for FD#" << fd_ << " sequence " << clientSequence_
          << ". " << size << " bytes in, " << bits << " bits ("
          << ((float) bits) / 8 << " bytes) out.\n" << logofs_flush;

  #endif

  statistics -> addRequestBits(opcode, size << 3, bits);

  if (opcode == opcodeStore_ -> renderExtension)
  {
    statistics -> addRenderRequestBits(*(buffer + 1), size << 3, bits);
  }

  return 1;
}

int ClientChannel::handleFastWriteReply(DecodeBuffer &decodeBuffer, unsigned char &opcode,
                                            unsigned char *&buffer, unsigned int &size)
{
  if ((opcode >= X_NXFirstOpcode &&
           opcode <= X_NXLastOpcode) ||
               opcode == X_ListExtensions ||
                   opcode == X_QueryExtension)
  {
    return 0;
  }

  #ifdef DEBUG
  *logofs << "handleFastWriteReply: Decoding raw reply OPCODE#"
          << (unsigned int) opcode << " for FD#" << fd_
          << ".\n" << logofs_flush;
  #endif

  buffer = writeBuffer_.addMessage(8);

  #ifndef __sun

  unsigned int *next = (unsigned int *) decodeBuffer.decodeMemory(8);

  *((unsigned int *) buffer)       = *next++;
  *((unsigned int *) (buffer + 4)) = *next;

  #else /* #ifndef __sun */

  memcpy(buffer, decodeBuffer.decodeMemory(8), 8);

  #endif /* #ifndef __sun */

  size = 32 + (GetULONG(buffer + 4, bigEndian_) << 2);

  writeBuffer_.registerPointer(&buffer);

  if (writeBuffer_.getAvailable() < size - 8 ||
          (int) size >= control -> TransportFlushBufferSize)
  {
    #ifdef DEBUG
    *logofs << "handleFastWriteReply: Using scratch buffer for OPCODE#"
            << (unsigned int) opcode << " with size " << size << " and "
            << writeBuffer_.getLength() << " bytes in buffer.\n"
            << logofs_flush;
    #endif

    writeBuffer_.removeMessage(8);

    buffer = writeBuffer_.addScratchMessage(((unsigned char *)
                             decodeBuffer.decodeMemory(size - 8)) - 8, size);
  }
  else
  {
    writeBuffer_.addMessage(size - 8);

    #ifndef __sun

    if (size == 32)
    {
      next = (unsigned int *) decodeBuffer.decodeMemory(size - 8);

      for (int i = 8; i < 32; i += sizeof(unsigned int))
      {
        *((unsigned int *) (buffer + i)) = *next++;
      }
    }
    else
    {
      memcpy(buffer + 8, decodeBuffer.decodeMemory(size - 8), size - 8);
    }

    #else /* #ifndef __sun */

    memcpy(buffer + 8, decodeBuffer.decodeMemory(size - 8), size - 8);

    #endif /* #ifndef __sun */
  }

  writeBuffer_.unregisterPointer();

  //
  // We don't need to write our local sequence
  // number. Replies are always sent with the
  // original X server's sequence number.
  //

  #if defined(TEST) || defined(OPCODES)
  *logofs << "handleFastWriteReply: Handled raw reply OPCODE#"
          << (unsigned int) opcode << " for FD#" << fd_ << " with sequence "
          << serverSequence_ << ". Output size is " << size << ".\n"
          << logofs_flush;
  #endif

  #ifdef DEBUG
  *logofs << "handleFastWriteReply: Length of sequence queue is "
          << sequenceQueue_.length() << ".\n" << logofs_flush;
  #endif

  statistics -> addRepliedRequest(opcode);

  handleFlush(flush_if_needed);

  return 1;
}

int ClientChannel::handleFastWriteEvent(DecodeBuffer &decodeBuffer, unsigned char &opcode,
                                            unsigned char *&buffer, unsigned int &size)
{
  #ifdef DEBUG
  *logofs << "handleFastWriteEvent: Decoding raw "
          << (opcode == X_Error ? "error" : "event") << " OPCODE#"
          << (unsigned int) opcode << " for FD#" << fd_
          << ".\n" << logofs_flush;
  #endif

  size = 32;

  buffer = writeBuffer_.addMessage(size);

  #ifndef __sun

  unsigned int *next = (unsigned int *) decodeBuffer.decodeMemory(size);

  for (int i = 0; i < 32; i += sizeof(unsigned int))
  {
    *((unsigned int *) (buffer + i)) = *next++;
  }

  #else /* #ifndef __sun */

  memcpy(buffer, decodeBuffer.decodeMemory(size), size);

  #endif /* #ifndef __sun */

  //
  // Use our local sequence number.
  //

  PutUINT(lastSequence_, buffer + 2, bigEndian_);

  #if defined(TEST) || defined(OPCODES)
  *logofs << "handleFastWriteEvent: Handled raw "
          << (opcode == X_Error ? "error" : "event") << " OPCODE#"
          << (unsigned int) opcode << " for FD#" << fd_ << " with sequence "
          << lastSequence_ << ". Output size is " << size << ".\n"
          << logofs_flush;
  #endif

  //
  // Check if we need to suppress the error.
  //

  if (opcode == X_Error && handleTaintSyncError(*(buffer + 10)) > 0)
  {
    #if defined(TEST) || defined(OPCODES)
    *logofs << "handleFastWriteEvent: WARNING! Suppressed error OPCODE#"
            << (unsigned int) opcode << " for FD#" << fd_
            << " with sequence " << lastSequence_ << ".\n"
            << logofs_flush;
    #endif

    writeBuffer_.removeMessage(32);
  }

  handleFlush(flush_if_needed);

  return 1;
}

int ClientChannel::handleShmemRequest(EncodeBuffer &encodeBuffer, const unsigned char opcode,
                                          const unsigned char *buffer, const unsigned int size)
{
  //
  // Will push sequence and set
  // priority according to stage.
  //

  unsigned int stage = *(buffer + 1);

  #ifdef TEST
  *logofs << "handleShmemRequest: Encoding shmem request "
          << "OPCODE#" << (unsigned int) opcode << " for FD#"
          << fd_ << " with size " << size << " at stage "
          << stage << ".\n" << logofs_flush;
  #endif

  encodeBuffer.encodeValue(stage, 2);

  if (stage == 0)
  {
    unsigned int enableClient = 0;
    unsigned int enableServer = 0;

    if (control -> ShmemClient == 1)
    {
      enableClient = *(buffer + 4);
    }

    if (control -> ShmemServer == 1)
    {
      enableServer = *(buffer + 5);
    }

    encodeBuffer.encodeBoolValue(enableClient);
    encodeBuffer.encodeBoolValue(enableServer);

    unsigned int clientSegment = GetULONG(buffer + 8,  bigEndian_);
    unsigned int serverSegment = GetULONG(buffer + 12, bigEndian_);

    encodeBuffer.encodeValue(clientSegment, 29, 9);
    encodeBuffer.encodeValue(serverSegment, 29, 9);

    #ifdef TEST
    *logofs << "handleShmemRequest: Enable client is "
            << enableClient << " enable server is " << enableServer
            << " client segment is " << (void *) clientSegment
            << " server segment is " << (void *) serverSegment
            << ".\n" << logofs_flush;
    #endif

    #ifdef TEST
    *logofs << "handleShmemRequest: Size of the shared memory "
            << "segment will be " << control -> ShmemServerSize
            << ".\n" << logofs_flush;
    #endif
  }

  if (stage != 1)
  {
    sequenceQueue_.push(clientSequence_, opcodeStore_ ->
                            getShmemParameters);

    priority_++;
  }

  return 1;
}

int ClientChannel::handleShmemReply(DecodeBuffer &decodeBuffer, unsigned char &opcode,
                                        unsigned char *&buffer, unsigned int &size)
{
  #ifdef TEST
  *logofs << "handleShmemReply: Received shmem parameters "
          << "reply OPCODE#" << (unsigned int) opcode
          << ".\n" << logofs_flush;
  #endif

  size   = 32;
  buffer = writeBuffer_.addMessage(size);

  unsigned int stage;

  decodeBuffer.decodeValue(stage, 2);

  *(buffer + 1) = stage;

  if (stage == 2)
  {
    unsigned int clientEnabled;
    unsigned int serverEnabled;

    decodeBuffer.decodeBoolValue(clientEnabled);
    decodeBuffer.decodeBoolValue(serverEnabled);

    //
    // Client support is not implemented
    // and not useful. It is here only
    // for compatibility.
    //

    clientEnabled = 0;

    *(buffer + 8) = clientEnabled;
    *(buffer + 9) = serverEnabled;

    PutULONG(0, buffer + 12, bigEndian_);

    if (serverEnabled == 1)
    {
      #ifdef TEST
      *logofs << "handleShmemReply: Enabled shared memory "
              << "support in X server with segment size "
              << control -> ShmemServerSize << ".\n"
              << logofs_flush;
      #endif

      PutULONG(control -> ShmemServerSize, buffer + 16, bigEndian_);
    }
    else
    {
      PutULONG(0, buffer + 16, bigEndian_);
    }
  }
  else
  {
    *(buffer + 8) = 0;
    *(buffer + 9) = 0;

    PutULONG(0, buffer + 12, bigEndian_);
    PutULONG(0, buffer + 16, bigEndian_);
  }

  return 1;
}

int ClientChannel::handleFontRequest(EncodeBuffer &encodeBuffer, const unsigned char opcode,
                                         const unsigned char *buffer, const unsigned int size)
{
  #ifdef TEST
  *logofs << "handleFontRequest: Encoding font request "
          << "OPCODE#" << (unsigned int) opcode << " for FD#"
          << fd_ << " with size " << size << ".\n"
          << logofs_flush;
  #endif

  sequenceQueue_.push(clientSequence_, opcodeStore_ ->
                          getFontParameters);

  return 1;
}

int ClientChannel::handleFontReply(DecodeBuffer &decodeBuffer, unsigned char &opcode,
                                       unsigned char *&buffer, unsigned int &size)
{
  #ifdef TEST
  *logofs << "handleFontReply: Received font operation "
          << "reply OPCODE#" << (unsigned int) opcode
          << ".\n" << logofs_flush;
  #endif

  unsigned int length;

  decodeBuffer.decodeValue(length, 8);

  size   = 32 + RoundUp4(length + 1);
  buffer = writeBuffer_.addMessage(size);

  unsigned char *next = buffer + 32;

  *next++ = length;

  decodeBuffer.decodeTextData(next, length);

  #ifdef TEST

  *logofs << "handleFontReply: Received tunneled font server "
          << "path '";

  for (unsigned int i = 0; i < length; i++)
  {
    *logofs << *(buffer + 32 + 1 + i);
  }

  *logofs << "' for FD#" << fd_ << ".\n" << logofs_flush;

  #endif

  if (fontPort_ == -1)
  {
    //
    // The local side is not going to forward
    // the font server connections.
    //

    #ifdef TEST
    *logofs << "handleFontReply: WARNING! Returning an empty "
            << "font server path.\n" << logofs_flush;
    #endif

    writeBuffer_.removeMessage(size);

    size   = 36;
    buffer = writeBuffer_.addMessage(size);

    //
    // Set the length of the returned
    // path to 0.
    //

    *(buffer + 32) = 0;
  }
  #ifdef TEST
  else
  {
    *logofs << "handleFontReply: Returning the received "
            << "font server path.\n" << logofs_flush;
  }
  #endif

  return 1;
}

int ClientChannel::handleCacheRequest(EncodeBuffer &encodeBuffer, const unsigned char opcode,
                                          const unsigned char *buffer, const unsigned int size)
{
  #ifdef TEST
  *logofs << "handleCacheRequest: Handling cache request "
          << "for FD#" << fd_ << ".\n" << logofs_flush;
  #endif

  enableCache_ = *(buffer + 4);
  enableSplit_ = *(buffer + 5);
  enableSave_  = *(buffer + 6);
  enableLoad_  = *(buffer + 7);

  handleSplitEnable();

  #ifdef TEST
  *logofs << "handleCacheRequest: Set cache parameters to "
          << " cache " << enableCache_ << " split " << enableSplit_
          << " save " << enableSave_ << " load " << enableLoad_
          << ".\n" << logofs_flush;
  #endif

  //
  // Encode all the parameters as a
  // single unsigned int so we can
  // use an int cache.
  //

  unsigned int mask = enableSave_ << 8 | enableLoad_;

  encodeBuffer.encodeCachedValue(mask, 32, clientCache_ ->
                     setCacheParametersCache);
  return 0;
}

int ClientChannel::handleStartSplitRequest(EncodeBuffer &encodeBuffer, const unsigned char opcode,
                                               const unsigned char *buffer, const unsigned int size)
{
  #if defined(TEST) || defined(SPLIT)
  *logofs << "handleStartSplitRequest: SPLIT! Handling start split "
          << "request for FD#"<< fd_ << ".\n" << logofs_flush;
  #endif

  if (splitState_.resource != nothing)
  {
    #ifdef PANIC
    *logofs << "handleStartSplitRequest: PANIC! SPLIT! Split requested "
            << "for resource id " << (unsigned int) *(buffer + 1)
            << " while handling resource " << splitState_.resource
            << ".\n" << logofs_flush;
    #endif

    cerr << "Error" << ": Split requested for "
         << "resource id " << (unsigned int) *(buffer + 1)
         << " while handling resource " << splitState_.resource
         << ".\n";

    return -1;
  }
  else if (fd_ != firstClient_)
  {
    //
    // It can be that an auxiliary channel is the
    // first to connect, then comes the agent that
    // is actually using the NX opcodes.
    //

    #ifdef WARNING
    *logofs << "handleStartSplitRequest: WARNING SPLIT! Split requested "
            << "on FD#" << fd_ << " while expecting FD#" << firstClient_
            << ".\n" << logofs_flush;
    #endif

    firstClient_ = fd_;
  }

  //
  // Set the agent's resource for which we are
  // going to split the request.
  //

  splitState_.resource = *(buffer + 1);

  #if defined(TEST) || defined(SPLIT)

  *logofs << "handleStartSplitRequest: SPLIT! Registered id "
          << splitState_.resource << " as resource "
          << "waiting for a split.\n" << logofs_flush;

  if (clientStore_ -> getSplitStore(splitState_.resource) != NULL)
  {
    *logofs << "handleStartSplitRequest: WARNING! SPLIT! A split "
            << "store for resource id " << splitState_.resource
            << " already exists.\n" << logofs_flush;

    clientStore_ -> dumpSplitStore(splitState_.resource);
  }

  #endif

  //
  // Send the selected resource to the remote.
  //

  if (control -> isProtoStep7() == 1)
  {
    encodeBuffer.encodeCachedValue(splitState_.resource, 8,
                       clientCache_ -> resourceCache);
  }

  splitState_.mode = (T_split_mode) *(buffer + 4);

  if (splitState_.mode != NXSplitModeAsync &&
          splitState_.mode != NXSplitModeSync)
  {
    splitState_.mode = (T_split_mode) control -> SplitMode;

    #if defined(TEST) || defined(SPLIT)
    *logofs << "handleStartSplitRequest: SPLIT! Set split "
            << "mode to '" << splitState_.mode << "' with "
            << "provided value '" << (unsigned) *(buffer + 4)
            << "'.\n" << logofs_flush;
    #endif
  }

  #if defined(TEST) || defined(SPLIT)

  if (splitState_.mode == NXSplitModeAsync)
  {
    *logofs << "handleStartSplitRequest: SPLIT! Selected split "
            << "mode is [split_async].\n" << logofs_flush;
  }
  else if (splitState_.mode == NXSplitModeSync)
  {
    *logofs << "handleStartSplitRequest: SPLIT! Selected split "
            << "mode is [split_sync].\n" << logofs_flush;
  }

  clientStore_ -> dumpSplitStores();

  #endif

  return 1;
}

int ClientChannel::handleEndSplitRequest(EncodeBuffer &encodeBuffer, const unsigned char opcode,
                                             const unsigned char *buffer, const unsigned int size)
{
  #if defined(TEST) || defined(SPLIT)
  *logofs << "handleEndSplitRequest: SPLIT! Handling end split "
          << "request for FD#"<< fd_ << ".\n" << logofs_flush;
  #endif

  //
  // Verify that the agent resource matches.
  //

  if (splitState_.resource == nothing)
  {
    #ifdef PANIC
    *logofs << "handleEndSplitRequest: PANIC! SPLIT! Received an end of "
            << "split for resource id " << (unsigned int) *(buffer + 1)
            << " without a previous start.\n"
            << logofs_flush;
    #endif

    cerr << "Error" << ": Received an end of split "
         << "for resource id " << (unsigned int) *(buffer + 1)
         << " without a previous start.\n";

    return -1;
  }
  else if (splitState_.resource != *(buffer + 1))
  {
    #ifdef PANIC
    *logofs << "handleEndSplitRequest: PANIC! SPLIT! Invalid resource id "
            << (unsigned int) *(buffer + 1) << " received while "
            << "waiting for resource id " << splitState_.resource
            << ".\n" << logofs_flush;
    #endif

    cerr << "Error" << ": Invalid resource id "
         << (unsigned int) *(buffer + 1) << " received while "
         << "waiting for resource id " << splitState_.resource
         << ".\n";

    return -1;
  }

  //
  // Send the selected resource to the remote.
  //

  if (control -> isProtoStep7() == 1)
  {
    encodeBuffer.encodeCachedValue(splitState_.resource, 8,
                       clientCache_ -> resourceCache);
  }

  //
  // Send the split notification events
  // to the agent.
  //

  handleRestart(sequence_immediate, splitState_.resource);

  //
  // Check if we still have splits to send.
  //

  handleSplitPending();

  #if defined(TEST) || defined(SPLIT)
  *logofs << "handleEndSplitRequest: SPLIT! Reset id "
          << splitState_.resource << " as resource "
          << "selected for splits.\n" << logofs_flush;
  #endif

  splitState_.resource = nothing;
  splitState_.mode     = split_none;

  #if defined(TEST) || defined(SPLIT)

  clientStore_ -> dumpSplitStores();

  #endif

  return 1;
}

void ClientChannel::handleDecodeCharInfo(DecodeBuffer &decodeBuffer, unsigned char *nextDest)
{
  unsigned int value;

  decodeBuffer.decodeCachedValue(value, 32,
                     *serverCache_ -> queryFontCharInfoCache[0], 6);

  PutUINT(value & 0xffff, nextDest, bigEndian_);
  PutUINT(value >> 16, nextDest + 10, bigEndian_);

  nextDest += 2;

  for (unsigned int i = 1; i < 5; i++)
  {
    unsigned int value;

    decodeBuffer.decodeCachedValue(value, 16,
                       *serverCache_ -> queryFontCharInfoCache[i], 6);

    PutUINT(value, nextDest, bigEndian_);

    nextDest += 2;
  }
}

int ClientChannel::setBigEndian(int flag)
{
  bigEndian_ = flag;

  return 1;
}

int ClientChannel::setReferences()
{
  #ifdef TEST
  *logofs << "ClientChannel: Initializing the static "
          << "members for the client channels.\n"
          << logofs_flush;
  #endif

  #ifdef REFERENCES

  references_ = 0;

  #endif

  return 1;
}
