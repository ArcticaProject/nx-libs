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

#include <sys/types.h>
#include <sys/socket.h>

#include "GenericChannel.h"

#include "EncodeBuffer.h"
#include "DecodeBuffer.h"

#include "StaticCompressor.h"

#include "Statistics.h"
#include "Proxy.h"

extern Proxy *proxy;

//
// Set the verbosity level.
//

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

//
// Log the important tracepoints related
// to writing packets to the peer proxy.
//

#undef  FLUSH

//
// Define this to log when a channel
// is created or destroyed.
//

#undef  REFERENCES

//
// Here are the static members.
//

#ifdef REFERENCES

int GenericChannel::references_ = 0;

#endif

GenericChannel::GenericChannel(Transport *transport, StaticCompressor *compressor)

  : Channel(transport, compressor), readBuffer_(transport_, this)
{
  #ifdef REFERENCES
  *logofs << "GenericChannel: Created new object at " 
          << this << " for FD#" << fd_ << " out of " 
          << ++references_ << " allocated channels.\n"
          << logofs_flush;
  #endif
}

GenericChannel::~GenericChannel()
{
  #ifdef REFERENCES
  *logofs << "GenericChannel: Deleted object at " 
          << this << " for FD#" << fd_ << " out of "
          << --references_ << " allocated channels.\n"
          << logofs_flush;
  #endif
}

//
// Beginning of handleRead().
//

int GenericChannel::handleRead(EncodeBuffer &encodeBuffer, const unsigned char *message,
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
  // Tag message as generic data in compression
  // routine. Opcode is not actually transferred
  // over the network.
  //

  unsigned char inputOpcode = X_NXInternalGenericData;

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

  #if defined(TEST) || defined(INFO) || defined(FLUSH)
  *logofs << "handleRead: Encoding messages for FD#" << fd_
          << " with " << readBuffer_.getLength() << " bytes "
          << "in the buffer.\n" << logofs_flush;
  #endif

  //
  // Divide the available data in multiple
  // messages and encode them one by one.
  //

  if (proxy -> handleAsyncSwitch(fd_) < 0)
  {
    return -1;
  }

  while ((inputMessage = readBuffer_.getMessage(inputLength)) != NULL)
  {
    encodeBuffer.encodeValue(inputLength, 32, 14);

    if (isCompressed() == 1)
    {
      unsigned int compressedDataSize = 0;
      unsigned char *compressedData   = NULL;

      if (handleCompress(encodeBuffer, inputOpcode, 0,
                             inputMessage, inputLength, compressedData,
                                 compressedDataSize) < 0)
      {
        return -1;
      }
    }
    else
    {
      encodeBuffer.encodeMemory(inputMessage, inputLength);
    }

    int bits = encodeBuffer.diffBits();

    #if defined(TEST) || defined(OPCODES)
    *logofs << "handleRead: Handled generic data for FD#" << fd_
            << ". " << inputLength << " bytes in, " << bits << " bits ("
            << ((float) bits) / 8 << " bytes) out.\n" << logofs_flush;
    #endif

    addProtocolBits(inputLength << 3, bits);

    if (isPrioritized() == 1)
    {
      priority_++;
    }

  } // End of while ((inputMessage = readBuffer_.getMessage(inputLength)) != NULL) ...

  //
  // All data has been read from the read buffer.
  // We still need to mark the end of the encode
  // buffer just before sending the frame. This
  // allows us to accomodate multiple reads in
  // a single frame.
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
            << "because of enough data or timeout on the "
            << "proxy link.\n" << logofs_flush;
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

int GenericChannel::handleWrite(const unsigned char *message, unsigned int length)
{
  #ifdef TEST
  *logofs << "handleWrite: Called for FD#" << fd_ << ".\n" << logofs_flush;
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

  unsigned char *outputMessage;
  unsigned int outputLength;

  //
  // Tag message as generic data
  // in decompression.
  //

  unsigned char outputOpcode = X_NXInternalGenericData;

  for (;;)
  {
    decodeBuffer.decodeValue(outputLength, 32, 14);

    if (outputLength == 0)
    {
      break;
    }

    if (isCompressed() == 1)
    {
      if (writeBuffer_.getAvailable() < outputLength ||
              (int) outputLength >= control -> TransportFlushBufferSize)
      {
        #ifdef DEBUG
        *logofs << "handleWrite: Using scratch buffer for "
                << "generic data with size " << outputLength << " and "
                << writeBuffer_.getLength() << " bytes in buffer.\n"
                << logofs_flush;
        #endif

        outputMessage = writeBuffer_.addScratchMessage(outputLength);
      }
      else
      {
        outputMessage = writeBuffer_.addMessage(outputLength);
      }

      const unsigned char *compressedData = NULL;
      unsigned int compressedDataSize = 0;

      int decompressed = handleDecompress(decodeBuffer, outputOpcode, 0,
                                              outputMessage, outputLength, compressedData,
                                                  compressedDataSize);
      if (decompressed < 0)
      {
        return -1;
      }
    }
    else
    {
      #ifdef DEBUG
      *logofs << "handleWrite: Using scratch buffer for "
              << "generic data with size " << outputLength << " and "
              << writeBuffer_.getLength() << " bytes in buffer.\n"
              << logofs_flush;
      #endif

      writeBuffer_.addScratchMessage((unsigned char *)
                       decodeBuffer.decodeMemory(outputLength), outputLength);
    }

    #if defined(TEST) || defined(OPCODES)
    *logofs << "handleWrite: Handled generic data for FD#" << fd_
            << ". "  << outputLength << " bytes out.\n"
            << logofs_flush;
    #endif

    handleFlush(flush_if_needed);
  }

  //
  // Write any remaining data to socket.
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

int GenericChannel::handleCompletion(EncodeBuffer &encodeBuffer)
{
  //
  // Add the bits telling to the remote
  // that all data in the frame has been
  // encoded.
  //

  if (encodeBuffer.getLength() > 0)
  {
    #if defined(TEST) || defined(INFO)
    *logofs << "handleCompletion: Writing completion bits with "
            << encodeBuffer.getLength() << " bytes encoded "
            << "for FD#" << fd_ << ".\n" << logofs_flush;
    #endif

    encodeBuffer.encodeValue(0, 32, 14);

    return 1;
  }
  #if defined(TEST) || defined(INFO)
  else
  {
    *logofs << "handleCompletion: PANIC! No completion to write "
            << "for FD#" << fd_ << ".\n" << logofs_flush;

    HandleCleanup();
  }
  #endif

  return 0;
}

int GenericChannel::handleConfiguration()
{
  #ifdef TEST
  *logofs << "GenericChannel: Setting new buffer parameters.\n"
          << logofs_flush;
  #endif

  readBuffer_.setSize(control -> GenericInitialReadSize,
                          control -> GenericMaximumBufferSize);

  writeBuffer_.setSize(control -> TransportGenericBufferSize,
                           control -> TransportGenericBufferThreshold,
                               control -> TransportMaximumBufferSize);

  transport_ -> setSize(control -> TransportGenericBufferSize,
                            control -> TransportGenericBufferThreshold,
                                control -> TransportMaximumBufferSize);

  return 1;
}

int GenericChannel::handleFinish()
{
  #ifdef TEST
  *logofs << "GenericChannel: Finishing channel for FD#"
          << fd_ << ".\n" << logofs_flush;
  #endif

  congestion_ = 0;
  priority_   = 0;

  finish_ = 1;

  transport_ -> fullReset();

  return 1;
}

int GenericChannel::setReferences()
{
  #ifdef TEST
  *logofs << "GenericChannel: Initializing the static "
          << "members for the generic channels.\n"
          << logofs_flush;
  #endif

  #ifdef REFERENCES

  references_ = 0;

  #endif

  return 1;
}
