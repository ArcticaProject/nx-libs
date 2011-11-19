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

#include <stdio.h>

#include "Statistics.h"

#include "Control.h"

#include "Proxy.h"

#include "ClientStore.h"
#include "ServerStore.h"

//
// Length of temporary buffer
// used to format output.
//

#define FORMAT_LENGTH          1024

//
// Log level.
//

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

//
// Note that when presenting statistics we invert the
// correct semantics of X client and server entities.
// This is questionable, but matches the user's pers-
// pective of running remote X applications on its
// local client.
//

Statistics::Statistics(Proxy *proxy) : proxy_(proxy)
{
  transportPartial_.idleTime_             = 0;
  transportPartial_.readTime_             = 0;
  transportPartial_.writeTime_            = 0;
  transportPartial_.proxyBytesIn_         = 0;
  transportPartial_.proxyBytesOut_        = 0;
  transportPartial_.proxyFramesIn_        = 0;
  transportPartial_.proxyFramesOut_       = 0;
  transportPartial_.proxyWritesOut_       = 0;
  transportPartial_.compressedBytesIn_    = 0;
  transportPartial_.compressedBytesOut_   = 0;
  transportPartial_.decompressedBytesIn_  = 0;
  transportPartial_.decompressedBytesOut_ = 0;
  transportPartial_.framingBitsOut_       = 0;

  transportTotal_.idleTime_             = 0;
  transportTotal_.readTime_             = 0;
  transportTotal_.writeTime_            = 0;
  transportTotal_.proxyBytesIn_         = 0;
  transportTotal_.proxyBytesOut_        = 0;
  transportTotal_.proxyFramesIn_        = 0;
  transportTotal_.proxyFramesOut_       = 0;
  transportTotal_.proxyWritesOut_       = 0;
  transportTotal_.compressedBytesIn_    = 0;
  transportTotal_.compressedBytesOut_   = 0;
  transportTotal_.decompressedBytesIn_  = 0;
  transportTotal_.decompressedBytesOut_ = 0;
  transportTotal_.framingBitsOut_       = 0;

  for (int i = 0; i < STATISTICS_OPCODE_MAX; i++)
  {
    protocolPartial_.requestCached_[i]  = 0;
    protocolPartial_.requestReplied_[i] = 0;
    protocolPartial_.requestCount_[i]   = 0;
    protocolPartial_.requestBitsIn_[i]  = 0;
    protocolPartial_.requestBitsOut_[i] = 0;

    protocolPartial_.renderRequestCached_[i]  = 0;
    protocolPartial_.renderRequestCount_[i]   = 0;
    protocolPartial_.renderRequestBitsIn_[i]  = 0;
    protocolPartial_.renderRequestBitsOut_[i] = 0;

    protocolPartial_.replyCached_[i]  = 0;
    protocolPartial_.replyCount_[i]   = 0;
    protocolPartial_.replyBitsIn_[i]  = 0;
    protocolPartial_.replyBitsOut_[i] = 0;

    protocolPartial_.eventCached_[i]  = 0;
    protocolPartial_.eventCount_[i]   = 0;
    protocolPartial_.eventBitsIn_[i]  = 0;
    protocolPartial_.eventBitsOut_[i] = 0;

    protocolTotal_.requestCached_[i]  = 0;
    protocolTotal_.requestReplied_[i] = 0;
    protocolTotal_.requestCount_[i]   = 0;
    protocolTotal_.requestBitsIn_[i]  = 0;
    protocolTotal_.requestBitsOut_[i] = 0;

    protocolTotal_.renderRequestCached_[i]  = 0;
    protocolTotal_.renderRequestCount_[i]   = 0;
    protocolTotal_.renderRequestBitsIn_[i]  = 0;
    protocolTotal_.renderRequestBitsOut_[i] = 0;

    protocolTotal_.replyCached_[i]  = 0;
    protocolTotal_.replyCount_[i]   = 0;
    protocolTotal_.replyBitsIn_[i]  = 0;
    protocolTotal_.replyBitsOut_[i] = 0;

    protocolTotal_.eventCached_[i]  = 0;
    protocolTotal_.eventCount_[i]   = 0;
    protocolTotal_.eventBitsIn_[i]  = 0;
    protocolTotal_.eventBitsOut_[i] = 0;
  }

  protocolPartial_.cupsCount_    = 0;
  protocolPartial_.cupsBitsIn_   = 0;
  protocolPartial_.cupsBitsOut_  = 0;

  protocolPartial_.smbCount_     = 0;
  protocolPartial_.smbBitsIn_    = 0;
  protocolPartial_.smbBitsOut_   = 0;

  protocolPartial_.mediaCount_   = 0;
  protocolPartial_.mediaBitsIn_  = 0;
  protocolPartial_.mediaBitsOut_ = 0;

  protocolPartial_.httpCount_    = 0;
  protocolPartial_.httpBitsIn_   = 0;
  protocolPartial_.httpBitsOut_  = 0;

  protocolPartial_.fontCount_    = 0;
  protocolPartial_.fontBitsIn_   = 0;
  protocolPartial_.fontBitsOut_  = 0;

  protocolPartial_.slaveCount_   = 0;
  protocolPartial_.slaveBitsIn_  = 0;
  protocolPartial_.slaveBitsOut_ = 0;

  protocolTotal_.cupsCount_    = 0;
  protocolTotal_.cupsBitsIn_   = 0;
  protocolTotal_.cupsBitsOut_  = 0;

  protocolTotal_.smbCount_     = 0;
  protocolTotal_.smbBitsIn_    = 0;
  protocolTotal_.smbBitsOut_   = 0;

  protocolTotal_.mediaCount_   = 0;
  protocolTotal_.mediaBitsIn_  = 0;
  protocolTotal_.mediaBitsOut_ = 0;

  protocolTotal_.httpCount_    = 0;
  protocolTotal_.httpBitsIn_   = 0;
  protocolTotal_.httpBitsOut_  = 0;

  protocolTotal_.fontCount_    = 0;
  protocolTotal_.fontBitsIn_   = 0;
  protocolTotal_.fontBitsOut_  = 0;

  protocolTotal_.slaveCount_   = 0;
  protocolTotal_.slaveBitsIn_  = 0;
  protocolTotal_.slaveBitsOut_ = 0;

  packedPartial_.packedBytesIn_  = 0;
  packedPartial_.packedBytesOut_ = 0;

  packedTotal_.packedBytesIn_    = 0;
  packedTotal_.packedBytesOut_   = 0;

  splitPartial_.splitCount_           = 0;
  splitPartial_.splitAborted_         = 0;
  splitPartial_.splitAbortedBytesOut_ = 0;

  splitTotal_.splitCount_             = 0;
  splitTotal_.splitAborted_           = 0;
  splitTotal_.splitAbortedBytesOut_   = 0;

  overallPartial_.overallBytesIn_  = 0;
  overallPartial_.overallBytesOut_ = 0;

  overallTotal_.overallBytesIn_    = 0;
  overallTotal_.overallBytesOut_   = 0;

  proxyData_.protocolCount_ = 0;
  proxyData_.controlCount_  = 0;
  proxyData_.splitCount_    = 0;
  proxyData_.dataCount_     = 0;

  proxyData_.streamRatio_ = 1;

  startShortFrameTs_ = getTimestamp();
  startLongFrameTs_  = getTimestamp();
  startFrameTs_      = getTimestamp();

  bytesInShortFrame_ = 0;
  bytesInLongFrame_  = 0;

  bitrateInShortFrame_ = 0;
  bitrateInLongFrame_  = 0;

  topBitrate_ = 0;

  congestionInFrame_ = 0;
}

Statistics::~Statistics()
{
}

int Statistics::resetPartialStats()
{
  transportPartial_.idleTime_             = 0;
  transportPartial_.readTime_             = 0;
  transportPartial_.writeTime_            = 0;
  transportPartial_.proxyBytesIn_         = 0;
  transportPartial_.proxyBytesOut_        = 0;
  transportPartial_.proxyFramesIn_        = 0;
  transportPartial_.proxyFramesOut_       = 0;
  transportPartial_.proxyWritesOut_       = 0;
  transportPartial_.compressedBytesIn_    = 0;
  transportPartial_.compressedBytesOut_   = 0;
  transportPartial_.decompressedBytesIn_  = 0;
  transportPartial_.decompressedBytesOut_ = 0;
  transportPartial_.framingBitsOut_       = 0;

  for (int i = 0; i < STATISTICS_OPCODE_MAX; i++)
  {
    protocolPartial_.requestCached_[i]  = 0;
    protocolPartial_.requestReplied_[i] = 0;
    protocolPartial_.requestCount_[i]   = 0;
    protocolPartial_.requestBitsIn_[i]  = 0;
    protocolPartial_.requestBitsOut_[i] = 0;

    protocolPartial_.renderRequestCached_[i]  = 0;
    protocolPartial_.renderRequestCount_[i]   = 0;
    protocolPartial_.renderRequestBitsIn_[i]  = 0;
    protocolPartial_.renderRequestBitsOut_[i] = 0;

    protocolPartial_.replyCached_[i]  = 0;
    protocolPartial_.replyCount_[i]   = 0;
    protocolPartial_.replyBitsIn_[i]  = 0;
    protocolPartial_.replyBitsOut_[i] = 0;

    protocolPartial_.eventCached_[i]  = 0;
    protocolPartial_.eventCount_[i]   = 0;
    protocolPartial_.eventBitsIn_[i]  = 0;
    protocolPartial_.eventBitsOut_[i] = 0;
  }

  protocolPartial_.cupsCount_    = 0;
  protocolPartial_.cupsBitsIn_   = 0;
  protocolPartial_.cupsBitsOut_  = 0;

  protocolPartial_.smbCount_     = 0;
  protocolPartial_.smbBitsIn_    = 0;
  protocolPartial_.smbBitsOut_   = 0;

  protocolPartial_.mediaCount_   = 0;
  protocolPartial_.mediaBitsIn_  = 0;
  protocolPartial_.mediaBitsOut_ = 0;

  protocolPartial_.httpCount_    = 0;
  protocolPartial_.httpBitsIn_   = 0;
  protocolPartial_.httpBitsOut_  = 0;

  protocolPartial_.fontCount_    = 0;
  protocolPartial_.fontBitsIn_   = 0;
  protocolPartial_.fontBitsOut_  = 0;

  protocolPartial_.slaveCount_   = 0;
  protocolPartial_.slaveBitsIn_  = 0;
  protocolPartial_.slaveBitsOut_ = 0;

  packedPartial_.packedBytesIn_  = 0;
  packedPartial_.packedBytesOut_ = 0;

  splitPartial_.splitCount_           = 0;
  splitPartial_.splitAborted_         = 0;
  splitPartial_.splitAbortedBytesOut_ = 0;

  overallPartial_.overallBytesIn_  = 0;
  overallPartial_.overallBytesOut_ = 0;

  return 1;
}

void Statistics::addCompressedBytes(unsigned int bytesIn, unsigned int bytesOut)
{
  transportPartial_.compressedBytesIn_ += bytesIn;
  transportTotal_.compressedBytesIn_ += bytesIn;

  transportPartial_.compressedBytesOut_ += bytesOut;
  transportTotal_.compressedBytesOut_ += bytesOut;

  double ratio = bytesIn / bytesOut;

  if (ratio < 1)
  {
    ratio = 1;
  }

  #if defined(TEST) || defined(TOKEN)
  *logofs << "Statistics: TOKEN! Old ratio was "
          << proxyData_.streamRatio_ << " current is "
          << (double) ratio << " new ratio is " << (double)
             ((proxyData_.streamRatio_ * 2) + ratio) / 3 << ".\n"
          << logofs_flush;
  #endif

  proxyData_.streamRatio_ = ((proxyData_.streamRatio_ * 2) + ratio) / 3;

  #if defined(TEST) || defined(TOKEN)
  *logofs << "Statistics: TOKEN! Updated compressed bytes "
          << "with " << bytesIn << " in " << bytesOut << " out "
          << "and ratio " << (double) proxyData_.streamRatio_
          << ".\n" << logofs_flush;
  #endif
}

//
// Recalculate the current bitrate. The bytes written
// are accounted at the time the transport actually
// writes the data to the network, not at the time it
// receives the data from the upper layers. The reason
// is that data can be compressed by the stream com-
// pressor, so we can become aware of the new bitrate
// only afer having flushed the ZLIB stream. This also
// means that, to have a reliable estimate, we need to
// flush the link often.
//

void Statistics::updateBitrate(int bytes)
{
  T_timestamp thisFrameTs = getNewTimestamp();

  int diffFramesInMs = diffTimestamp(startFrameTs_, thisFrameTs);

  #ifdef DEBUG
  *logofs << "Statistics: Difference since previous timestamp is "
          << diffFramesInMs << " Ms.\n" << logofs_flush;
  #endif

  if (diffFramesInMs > 0)
  {
    #ifdef DEBUG
    *logofs << "Statistics: Removing " << diffFramesInMs
            << " Ms in short and long time frame.\n"
            << logofs_flush;
    #endif

    int shortBytesToRemove = (int) (((double) bytesInShortFrame_ * (double) diffFramesInMs) /
                                       (double) control -> ShortBitrateTimeFrame);

    int longBytesToRemove = (int) (((double) bytesInLongFrame_ * (double) diffFramesInMs) /
                                      (double) control -> LongBitrateTimeFrame);

    #ifdef DEBUG
    *logofs << "Statistics: Removing " << shortBytesToRemove
            << " bytes from " << bytesInShortFrame_
            << " in the short frame.\n" << logofs_flush;
    #endif

    bytesInShortFrame_ -= shortBytesToRemove;

    if (bytesInShortFrame_ < 0)
    {
      #ifdef TEST
      *logofs << "Statistics: Bytes in short frame are "
              << bytesInShortFrame_ << ". Set to 0.\n"
              << logofs_flush;
      #endif

      bytesInShortFrame_ = 0;
    }

    #ifdef DEBUG
    *logofs << "Statistics: Removing " << longBytesToRemove
            << " bytes from " << bytesInLongFrame_
            << " in the long frame.\n" << logofs_flush;
    #endif

    bytesInLongFrame_ -= longBytesToRemove;

    if (bytesInLongFrame_ < 0)
    {
      #ifdef TEST
      *logofs << "Statistics: Bytes in long frame are "
              << bytesInLongFrame_ << ". Set to 0.\n"
              << logofs_flush;
      #endif

      bytesInLongFrame_ = 0;
    }

    int diffStartInMs;

    diffStartInMs = diffTimestamp(thisFrameTs, startShortFrameTs_);

    if (diffStartInMs > control -> ShortBitrateTimeFrame)
    {
      addMsTimestamp(startShortFrameTs_, diffStartInMs);
    }

    diffStartInMs = diffTimestamp(thisFrameTs, startLongFrameTs_);

    if (diffStartInMs > control -> LongBitrateTimeFrame)
    {
      addMsTimestamp(startLongFrameTs_, diffStartInMs);
    }

    startFrameTs_ = thisFrameTs;
  }

  #ifdef DEBUG
  *logofs << "Statistics: Adding " << bytes << " bytes to "
          << bytesInShortFrame_ << " in the short frame.\n"
          << logofs_flush;
  #endif

  bytesInShortFrame_ = bytesInShortFrame_ + bytes;

  #ifdef DEBUG
  *logofs << "Statistics: Adding " << bytes << " bytes to "
          << bytesInLongFrame_ << " in the long frame.\n"
          << logofs_flush;
  #endif

  bytesInLongFrame_ = bytesInLongFrame_ + bytes;

  bitrateInShortFrame_ = (int) ((double) bytesInShortFrame_ /
                                   ((double) control -> ShortBitrateTimeFrame / 1000));

  bitrateInLongFrame_ = (int) ((double) bytesInLongFrame_ /
                                   ((double) control -> LongBitrateTimeFrame / 1000));

  if (bitrateInShortFrame_ > topBitrate_)
  {
    topBitrate_ = bitrateInShortFrame_;
  }

  #ifdef TEST
  *logofs << "Statistics: Current bitrate is short " << bitrateInShortFrame_
          << " long " << bitrateInLongFrame_ << " top " << topBitrate_
          << ".\n" << logofs_flush;
  #endif
}

void Statistics::updateCongestion(int remaining, int limit)
{
  #ifdef TEST
  *logofs << "Statistics: Updating the congestion "
          << "counters at " << strMsTimestamp()
          << ".\n" << logofs_flush;
  #endif

  double current = remaining;

  if (current < 0)
  {
    current = 0;
  }

  current = 9 * (limit - current) / limit;

  #ifdef TEST
  *logofs << "Statistics: Current congestion is "
          << current << " with " << limit << " tokens "
          << "and " << remaining << " remaining.\n"
          << logofs_flush;
  #endif

  //
  // If the current congestion counter is greater
  // than the previous, take the current value,
  // otherwise ramp down the value by calculating
  // the average of the last 8 updates.
  //

  #ifdef TEST
  *logofs << "Statistics: Old congestion was "
          << congestionInFrame_;
  #endif

  if (current >= congestionInFrame_)
  {
    congestionInFrame_ = current;
  }
  else
  {
    congestionInFrame_ = ((congestionInFrame_ * 7) + current) / 8;
  }

  #ifdef TEST
  *logofs << " new congestion is "
          << ((congestionInFrame_ * 7) + current) / 8
          << ".\n" << logofs_flush;
  #endif

  //
  // Call the function with 0 bytes flushed
  // so the agent can update its congestion
  // counter.
  //

  FlushCallback(0);
}

int Statistics::getClientCacheStats(int type, char *&buffer)
{
  if (type != PARTIAL_STATS && type != TOTAL_STATS)
  {
    #ifdef PANIC
    *logofs << "Statistics: PANIC! Cannot produce statistics "
            << "with qualifier '" << type << "'.\n"
            << logofs_flush;
    #endif

    return -1;
  }

  //
  // Print message cache data according
  // to local and remote view.
  //

  MessageStore *currentStore = NULL;
  MessageStore *anyStore     = NULL;

  char format[FORMAT_LENGTH];

  strcat(buffer, "\nNX Cache Statistics\n");
  strcat(buffer, "-------------------\n\n");

  for (int m = proxy_client; m <= proxy_server; m++)
  {
    if (m == proxy_client)
    {
      strcat(buffer, "Request\tCached\tSize at Server\t\tSize at Client\t\tCache limit\n");
      strcat(buffer, "-------\t------\t--------------\t\t--------------\t\t-----------\n");
    }
    else
    {
      strcat(buffer, "\nReply\tCached\tSize at Server\t\tSize at Client\t\tCache limit\n");
      strcat(buffer, "-----\t------\t--------------\t\t--------------\t\t-----------\n");
    }

    for (int i = 0; i < CHANNEL_STORE_OPCODE_LIMIT; i++)
    {
      if (m == proxy_client)
      {
        currentStore = proxy_ -> getClientStore() -> getRequestStore(i);
      }
      else
      {
        currentStore = proxy_ -> getServerStore() -> getReplyStore(i);
      }

      if (currentStore != NULL &&
              (currentStore -> getLocalStorageSize() ||
                   currentStore -> getRemoteStorageSize()))
      {
        anyStore = currentStore;

        sprintf(format, "#%d\t%d\t", i, currentStore -> getSize());

        strcat(buffer, format);

        sprintf(format, "%d (%.0f KB)\t\t", currentStore -> getLocalStorageSize(),
                    ((double) currentStore -> getLocalStorageSize()) / 1024);

        strcat(buffer, format);

        sprintf(format, "%d (%.0f KB)\t\t", currentStore -> getRemoteStorageSize(),
                    ((double) currentStore -> getRemoteStorageSize()) / 1024);

        strcat(buffer, format);

        sprintf(format, "%d/%.0f KB\n", currentStore -> cacheSlots,
                    ((double) control -> getUpperStorageSize() / 100 *
                          currentStore -> cacheThreshold) / 1024);

        strcat(buffer, format);
      }
    }

    if (anyStore == NULL)
    {
      strcat(buffer, "N/A\n");
    }
  }

  if (anyStore != NULL)
  {
    sprintf(format, "\ncache: %d bytes (%d KB) available at server.\n",
                control -> ClientTotalStorageSize,
                    control -> ClientTotalStorageSize / 1024);

    strcat(buffer, format);

    sprintf(format, "       %d bytes (%d KB) available at client.\n\n",
                control -> ServerTotalStorageSize,
                    control -> ServerTotalStorageSize / 1024);

    strcat(buffer, format);

    sprintf(format, "       %d bytes (%d KB) allocated at server.\n",
                anyStore -> getLocalTotalStorageSize(),
                    anyStore -> getLocalTotalStorageSize() / 1024);

    strcat(buffer, format);

    sprintf(format, "       %d bytes (%d KB) allocated at client.\n\n\n",
                anyStore -> getRemoteTotalStorageSize(),
                    anyStore -> getRemoteTotalStorageSize() / 1024);

    strcat(buffer, format);
  }
  else
  {
    strcat(buffer, "\ncache: N/A\n\n");
  }

  return 1;
}

int Statistics::getClientProtocolStats(int type, char *&buffer)
{
  if (type != PARTIAL_STATS && type != TOTAL_STATS)
  {
    #ifdef PANIC
    *logofs << "Statistics: PANIC! Cannot produce statistics "
            << "with qualifier '" << type << "'.\n"
            << logofs_flush;
    #endif

    return -1;
  }

  struct T_transportData *transportData;
  struct T_protocolData  *protocolData;
  struct T_overallData   *overallData;

  if (type == PARTIAL_STATS)
  {
    transportData = &transportPartial_;
    protocolData = &protocolPartial_;
    overallData = &overallPartial_;
  }
  else
  {
    transportData = &transportTotal_;
    protocolData = &protocolTotal_;
    overallData = &overallTotal_;
  }

  char format[FORMAT_LENGTH];

  double countRequestIn        = 0;
  double countCachedRequestIn  = 0;
  double countRepliedRequestIn = 0;

  double countRequestBitsIn  = 0;
  double countRequestBitsOut = 0;

  double countAnyIn   = 0;
  double countBitsIn  = 0;
  double countBitsOut = 0;

  //
  // Print request data.
  //

  strcat(buffer, "NX Server Side Protocol Statistics\n");
  strcat(buffer, "----------------------------------\n\n");

  //
  // Print render data.
  //

  strcat(buffer, "Render  Total\tCached\tBits In\t\tBits Out\tBits/Request\t\tRatio\n");
  strcat(buffer, "------- -----\t------\t-------\t\t--------\t------------\t\t-----\n");

  for (int i = 0; i < STATISTICS_OPCODE_MAX; i++)
  {
    if (protocolData -> renderRequestCount_[i])
    {
      sprintf(format, "#%d ", i);

      while (strlen(format) < 8)
      {
        strcat(format, " ");
      }

      strcat(buffer, format);

      if (protocolData -> renderRequestCached_[i] > 0)
      {
        sprintf(format, "%.0f\t%.0f", protocolData -> renderRequestCount_[i],
                    protocolData -> renderRequestCached_[i]);
      }
      else
      {
        sprintf(format, "%.0f\t", protocolData -> renderRequestCount_[i]);
      }

      strcat(buffer, format);

      sprintf(format, "\t%.0f (%.0f KB)\t%.0f (%.0f KB)\t%.0f/1 -> %.0f/1     \t",
                  protocolData -> renderRequestBitsIn_[i], protocolData -> renderRequestBitsIn_[i] / 8192,
                      protocolData -> renderRequestBitsOut_[i], protocolData -> renderRequestBitsOut_[i] / 8192,
                          protocolData -> renderRequestBitsIn_[i] / protocolData -> renderRequestCount_[i],
                              protocolData -> renderRequestBitsOut_[i] / protocolData -> renderRequestCount_[i]);

      strcat(buffer, format);

      if (protocolData -> renderRequestBitsOut_[i] > 0)
      {
        sprintf(format, "%5.3f:1\n", protocolData -> renderRequestBitsIn_[i] /
                                         protocolData -> renderRequestBitsOut_[i]);

        strcat(buffer, format);
      }
      else
      {
        strcat(buffer, "1:1\n");
      }
    }

    countRequestIn        += protocolData -> renderRequestCount_[i];
    countCachedRequestIn  += protocolData -> renderRequestCached_[i];

    countRequestBitsIn  += protocolData -> renderRequestBitsIn_[i];
    countRequestBitsOut += protocolData -> renderRequestBitsOut_[i];

    countAnyIn   += protocolData -> renderRequestCount_[i];
    countBitsIn  += protocolData -> renderRequestBitsIn_[i];
    countBitsOut += protocolData -> renderRequestBitsOut_[i];
  }

  if (countRequestIn > 0)
  {
    if (countCachedRequestIn > 0)
    {
      sprintf(format, "\ntotal:  %.0f\t%.0f", countRequestIn, countCachedRequestIn);
    }
    else
    {
      sprintf(format, "\ntotal:  %.0f\t", countRequestIn);
    }

    strcat(buffer, format);

    sprintf(format, "\t%.0f (%.0f KB)\t%.0f (%.0f KB)\t%.0f/1 -> %.0f/1     \t",
                countRequestBitsIn, countRequestBitsIn / 8192, countRequestBitsOut,
                    countRequestBitsOut / 8192, countRequestBitsIn / countRequestIn,
                        countRequestBitsOut / countRequestIn);

    strcat(buffer, format);

    if (countRequestBitsOut > 0)
    {
      sprintf(format, "%5.3f:1\n", countRequestBitsIn / countRequestBitsOut);
    }
    else
    {
      sprintf(format, "1:1\n");
    }

    strcat(buffer, format);
  }
  else
  {
    strcat(buffer, "N/A\n\n");
  }

  countRequestIn        = 0;
  countCachedRequestIn  = 0;

  countRequestBitsIn  = 0;
  countRequestBitsOut = 0;

  countAnyIn   = 0;
  countBitsIn  = 0;
  countBitsOut = 0;

  //
  // Print other requests' data.
  //

  strcat(buffer, "\nRequest Total\tCached\tBits In\t\tBits Out\tBits/Request\t\tRatio\n");
  strcat(buffer, "------- -----\t------\t-------\t\t--------\t------------\t\t-----\n");

  for (int i = 0; i < STATISTICS_OPCODE_MAX; i++)
  {
    if (protocolData -> requestCount_[i])
    {
      sprintf(format, "#%d ", i);

      while (strlen(format) < 5)
      {
        strcat(format, " ");
      }

      //
      // Mark NX agent-related requests, those
      // having a reply and finally those that
      // have been probably tainted by client
      // side.
      //

      if (i >= X_NXFirstOpcode && i <= X_NXLastOpcode)
      {
        strcat(format, "A");
      }

      if (i != X_NXInternalGenericReply && protocolData -> requestReplied_[i] > 0)
      {
        strcat(format, "R");
      }

      if (i == X_NoOperation && control -> TaintReplies)
      {
        strcat(format, "T");
      }

      while (strlen(format) < 8)
      {
        strcat(format, " ");
      }

      strcat(buffer, format);

      if (protocolData -> requestCached_[i] > 0)
      {
        sprintf(format, "%.0f\t%.0f", protocolData -> requestCount_[i],
                    protocolData -> requestCached_[i]);
      }
      else
      {
        sprintf(format, "%.0f\t", protocolData -> requestCount_[i]);
      }

      strcat(buffer, format);

      sprintf(format, "\t%.0f (%.0f KB)\t%.0f (%.0f KB)\t%.0f/1 -> %.0f/1     \t",
                  protocolData -> requestBitsIn_[i], protocolData -> requestBitsIn_[i] / 8192,
                      protocolData -> requestBitsOut_[i], protocolData -> requestBitsOut_[i] / 8192,
                          protocolData -> requestBitsIn_[i] / protocolData -> requestCount_[i],
                              protocolData -> requestBitsOut_[i] / protocolData -> requestCount_[i]);

      strcat(buffer, format);

      if (protocolData -> requestBitsOut_[i] > 0)
      {
        sprintf(format, "%5.3f:1\n", protocolData -> requestBitsIn_[i] /
                                         protocolData -> requestBitsOut_[i]);

        strcat(buffer, format);
      }
      else
      {
        strcat(buffer, "1:1\n");
      }
    }

    countRequestIn        += protocolData -> requestCount_[i];
    countCachedRequestIn  += protocolData -> requestCached_[i];
    countRepliedRequestIn += protocolData -> requestReplied_[i];

    countRequestBitsIn  += protocolData -> requestBitsIn_[i];
    countRequestBitsOut += protocolData -> requestBitsOut_[i];

    countAnyIn   += protocolData -> requestCount_[i];
    countBitsIn  += protocolData -> requestBitsIn_[i];
    countBitsOut += protocolData -> requestBitsOut_[i];
  }

  if (countRequestIn > 0)
  {
    if (countCachedRequestIn > 0)
    {
      sprintf(format, "\ntotal:  %.0f\t%.0f", countRequestIn, countCachedRequestIn);
    }
    else
    {
      sprintf(format, "\ntotal:  %.0f\t", countRequestIn);
    }

    strcat(buffer, format);

    sprintf(format, "\t%.0f (%.0f KB)\t%.0f (%.0f KB)\t%.0f/1 -> %.0f/1     \t",
                countRequestBitsIn, countRequestBitsIn / 8192, countRequestBitsOut,
                    countRequestBitsOut / 8192, countRequestBitsIn / countRequestIn,
                        countRequestBitsOut / countRequestIn);

    strcat(buffer, format);

    if (countRequestBitsOut > 0)
    {
      sprintf(format, "%5.3f:1\n", countRequestBitsIn / countRequestBitsOut);
    }
    else
    {
      sprintf(format, "1:1\n");
    }

    strcat(buffer, format);
  }
  else
  {
    strcat(buffer, "N/A\n\n");
  }

  //
  // Print transport data.
  //

  getTimeStats(type, buffer);

  countAnyIn   += protocolData -> cupsCount_;
  countBitsIn  += protocolData -> cupsBitsIn_;
  countBitsOut += protocolData -> cupsBitsOut_;

  countAnyIn   += protocolData -> smbCount_;
  countBitsIn  += protocolData -> smbBitsIn_;
  countBitsOut += protocolData -> smbBitsOut_;

  countAnyIn   += protocolData -> mediaCount_;
  countBitsIn  += protocolData -> mediaBitsIn_;
  countBitsOut += protocolData -> mediaBitsOut_;

  countAnyIn   += protocolData -> httpCount_;
  countBitsIn  += protocolData -> httpBitsIn_;
  countBitsOut += protocolData -> httpBitsOut_;

  countAnyIn   += protocolData -> fontCount_;
  countBitsIn  += protocolData -> fontBitsIn_;
  countBitsOut += protocolData -> fontBitsOut_;

  countAnyIn   += protocolData -> slaveCount_;
  countBitsIn  += protocolData -> slaveBitsIn_;
  countBitsOut += protocolData -> slaveBitsOut_;

  //
  // Save the overall amount of bytes
  // coming from X clients.
  //

  overallData -> overallBytesIn_ = countBitsIn / 8;

  //
  // Print performance data.
  //

  if (transportData -> readTime_ > 0)
  {
    sprintf(format, "      %.0f messages (%.0f KB) encoded per second.\n\n",
                countAnyIn / (transportData -> readTime_ / 1000),
                    (countBitsIn + transportData -> framingBitsOut_) / 8192 /
                         (transportData -> readTime_ / 1000));
  }
  else
  {
    sprintf(format, "      %.0f messages (%.0f KB) encoded per second.\n\n",
                countAnyIn, (countBitsIn + transportData ->
                    framingBitsOut_) / 8192);
  }

  strcat(buffer, format);

  strcat(buffer, "link: ");

  //
  // ZLIB compression stats.
  //

  getStreamStats(type, buffer);

  //
  // Save the overall amount of bytes
  // sent on NX proxy link.
  //

  if (transportData -> compressedBytesOut_ > 0)
  {
    overallData -> overallBytesOut_ = transportData -> compressedBytesOut_;
  }
  else
  {
    overallData -> overallBytesOut_ = countBitsOut / 8;
  }

  //
  // Print info on multiplexing overhead.
  //

  getFramingStats(type, buffer);

  //
  // Print stats about additional channels.
  //

  getServicesStats(type, buffer);

  //
  // Compression summary.
  //

  double ratio = 1;

  if (transportData -> compressedBytesOut_ / 1024 > 0)
  {
    ratio = ((countBitsIn + transportData -> framingBitsOut_) / 8192) /
                  (transportData -> compressedBytesOut_ / 1024);

  }
  else if (countBitsOut > 0)
  {
    ratio = (countBitsIn + transportData -> framingBitsOut_) /
                 countBitsOut;
  }

  sprintf(format, "      Protocol compression ratio is %5.3f:1.\n\n",
              ratio);

  strcat(buffer, format);

  getBitrateStats(type, buffer);

  getSplitStats(type, buffer);

  sprintf(format, "      %.0f total handled replies (%.0f unmatched).\n\n\n",
              countRepliedRequestIn, protocolData -> requestReplied_[X_NXInternalGenericReply]);

  strcat(buffer, format);

  return 1;
}

int Statistics::getClientOverallStats(int type, char *&buffer)
{
  if (type != PARTIAL_STATS && type != TOTAL_STATS)
  {
    #ifdef PANIC
    *logofs << "Statistics: PANIC! Cannot produce statistics "
            << "with qualifier '" << type << "'.\n"
            << logofs_flush;
    #endif

    return -1;
  }

  struct T_overallData *overallData;
  struct T_packedData  *packedData;

  if (type == PARTIAL_STATS)
  {
    overallData = &overallPartial_;
    packedData  = &packedPartial_;
  }
  else
  {
    overallData = &overallTotal_;
    packedData = &packedTotal_;
  }

  char format[FORMAT_LENGTH];

  //
  // Print header including link type,
  // followed by info on packed images.
  //

  strcat(buffer, "NX Compression Summary\n");
  strcat(buffer, "----------------------\n\n");

  char label[FORMAT_LENGTH];

  switch (control -> LinkMode)
  {
    case LINK_TYPE_NONE:
    {
      strcpy(label, "NONE");

      break;
    }
    case LINK_TYPE_MODEM:
    {
      strcpy(label, "MODEM");

      break;
    }
    case LINK_TYPE_ISDN:
    {
      strcpy(label, "ISDN");

      break;
    }
    case LINK_TYPE_ADSL:
    {
      strcpy(label, "ADSL");

      break;
    }
    case LINK_TYPE_WAN:
    {
      strcpy(label, "WAN");

      break;
    }
    case LINK_TYPE_LAN:
    {
      strcpy(label, "LAN");

      break;
    }
    default:
    {
      strcpy(label, "Unknown");

      break;
    }
  }

  sprintf(format, "link:    %s", label);

  if (control -> LocalDeltaCompression == 1)
  {
    strcat(format, " with protocol compression enabled.");
  }
  else
  {
    strcat(format, " with protocol compression disabled.");
  }

  strcat(format, "\n\n");

  strcat(buffer, format);

  if (packedData -> packedBytesIn_ > 0)
  {
    sprintf(format, "images:  %.0f bytes (%.0f KB) packed to %.0f (%.0f KB).\n\n",
                packedData -> packedBytesOut_, packedData -> packedBytesOut_ / 1024,
                    packedData -> packedBytesIn_, packedData -> packedBytesIn_ / 1024);

    strcat(buffer, format);

    sprintf(format, "         Images compression ratio is %5.3f:1.\n\n",
                packedData -> packedBytesOut_ / packedData -> packedBytesIn_);

    strcat(buffer, format);
  }

  double overallIn = overallData -> overallBytesIn_ - packedData -> packedBytesIn_ +
                         packedData -> packedBytesOut_;

  double overallOut = overallData -> overallBytesOut_;

  sprintf(format, "overall: %.0f bytes (%.0f KB) in, %.0f bytes (%.0f KB) out.\n\n",
              overallIn, overallIn / 1024, overallOut, overallOut / 1024);

  strcat(buffer, format);

  if (overallData -> overallBytesOut_ > 0)
  {
    sprintf(format, "         Overall NX server compression ratio is %5.3f:1.\n\n\n",
                overallIn / overallOut);
  }
  else
  {
    sprintf(format, "         Overall NX server compression ratio is 1:1.\n\n\n");
  }

  strcat(buffer, format);

  return 1;
}

int Statistics::getServerCacheStats(int type, char *&buffer)
{
  if (type != PARTIAL_STATS && type != TOTAL_STATS)
  {
    #ifdef PANIC
    *logofs << "Statistics: PANIC! Cannot produce statistics "
            << "with qualifier '" << type << "'.\n"
            << logofs_flush;
    #endif

    return -1;
  }

  //
  // Print message cache data according
  // to local and remote view.
  //

  MessageStore *currentStore = NULL;
  MessageStore *anyStore     = NULL;

  char format[FORMAT_LENGTH];

  strcat(buffer, "\nNX Cache Statistics\n");
  strcat(buffer, "-------------------\n\n");

  for (int m = proxy_client; m <= proxy_server; m++)
  {
    if (m == proxy_client)
    {
      strcat(buffer, "Request\tCached\tSize at Server\t\tSize at Client\t\tCache limit\n");
      strcat(buffer, "-------\t------\t--------------\t\t--------------\t\t-----------\n");
    }
    else
    {
      strcat(buffer, "\nReply\tCached\tSize at Server\t\tSize at Client\t\tCache limit\n");
      strcat(buffer, "-----\t------\t--------------\t\t--------------\t\t-----------\n");
    }

    for (int i = 0; i < CHANNEL_STORE_OPCODE_LIMIT; i++)
    {
      if (m == proxy_client)
      {
        currentStore = proxy_ -> getClientStore() -> getRequestStore(i);
      }
      else
      {
        currentStore = proxy_ -> getServerStore() -> getReplyStore(i);
      }

      if (currentStore != NULL &&
              (currentStore -> getLocalStorageSize() ||
                   currentStore -> getRemoteStorageSize()))
      {
        anyStore = currentStore;

        sprintf(format, "#%d\t%d\t", i, currentStore -> getSize());

        strcat(buffer, format);

        sprintf(format, "%d (%.0f KB)\t\t", currentStore -> getRemoteStorageSize(),
                    ((double) currentStore -> getRemoteStorageSize()) / 1024);

        strcat(buffer, format);

        sprintf(format, "%d (%.0f KB)\t\t", currentStore -> getLocalStorageSize(),
                    ((double) currentStore -> getLocalStorageSize()) / 1024);

        strcat(buffer, format);

        sprintf(format, "%d/%.0f KB\n", currentStore -> cacheSlots,
                    ((double) control -> getUpperStorageSize() / 100 *
                          currentStore -> cacheThreshold) / 1024);

        strcat(buffer, format);
      }
    }

    if (anyStore == NULL)
    {
      strcat(buffer, "N/A\n");
    }
  }

  if (anyStore != NULL)
  {
    sprintf(format, "\ncache: %d bytes (%d KB) available at server.\n",
                control -> ClientTotalStorageSize,
                    control -> ClientTotalStorageSize / 1024);

    strcat(buffer, format);

    sprintf(format, "       %d bytes (%d KB) available at client.\n\n",
                control -> ServerTotalStorageSize,
                    control -> ServerTotalStorageSize / 1024);

    strcat(buffer, format);

    sprintf(format, "       %d bytes (%d KB) allocated at server.\n",
                anyStore -> getRemoteTotalStorageSize(),
                    anyStore -> getRemoteTotalStorageSize() / 1024);

    strcat(buffer, format);

    sprintf(format, "       %d bytes (%d KB) allocated at client.\n\n\n",
                anyStore -> getLocalTotalStorageSize(),
                    anyStore -> getLocalTotalStorageSize() / 1024);

    strcat(buffer, format);
  }
  else
  {
    strcat(buffer, "\ncache: N/A\n\n");
  }

  return 1;
}

int Statistics::getServerProtocolStats(int type, char *&buffer)
{
  if (type != PARTIAL_STATS && type != TOTAL_STATS)
  {
    #ifdef PANIC
    *logofs << "Statistics: PANIC! Cannot produce statistics "
            << "with qualifier '" << type << "'.\n"
            << logofs_flush;
    #endif

    return -1;
  }

  struct T_transportData *transportData;
  struct T_protocolData  *protocolData;
  struct T_overallData   *overallData;

  if (type == PARTIAL_STATS)
  {
    transportData = &transportPartial_;
    protocolData = &protocolPartial_;
    overallData = &overallPartial_;
  }
  else
  {
    transportData = &transportTotal_;
    protocolData = &protocolTotal_;
    overallData = &overallTotal_;
  }

  char format[FORMAT_LENGTH];

  double countReplyBitsIn  = 0;
  double countReplyBitsOut = 0;

  double countReplyIn        = 0;
  double countCachedReplyIn  = 0;

  double countEventBitsIn  = 0;
  double countEventBitsOut = 0;

  double countEventIn        = 0;
  double countCachedEventIn  = 0;

  double countAnyIn   = 0;
  double countBitsIn  = 0;
  double countBitsOut = 0;

  //
  // Print reply data.
  //

  strcat(buffer, "NX Client Side Protocol Statistics\n");
  strcat(buffer, "----------------------------------\n\n");

  strcat(buffer, "Reply   Total\tCached\tBits In\t\tBits Out\tBits/Reply\t\tRatio\n");
  strcat(buffer, "------- -----\t------\t-------\t\t--------\t----------\t\t-----\n");

  for (int i = 0; i < STATISTICS_OPCODE_MAX; i++)
  {
    if (protocolData -> replyCount_[i])
    {
      sprintf(format, "#%d ", i);

      while (strlen(format) < 5)
      {
        strcat(format, " ");
      }

      //
      // Mark replies originated
      // by NX agent requests.
      //

      if (i >= X_NXFirstOpcode && i <= X_NXLastOpcode)
      {
        strcat(format, "A");
      }

      //
      // Mark replies that we didn't
      // match against a request.
      //

      if (i == 1)
      {
        strcat(format, "U");
      }

      while (strlen(format) < 8)
      {
        strcat(format, " ");
      }

      strcat(buffer, format);

      if (protocolData -> replyCached_[i] > 0)
      {
        sprintf(format, "%.0f\t%.0f", protocolData -> replyCount_[i],
                    protocolData -> replyCached_[i]);
      }
      else
      {
        sprintf(format, "%.0f\t", protocolData -> replyCount_[i]);
      }

      strcat(buffer, format);

      sprintf(format, "\t%.0f (%.0f KB)\t%.0f (%.0f KB)\t%.0f/1 -> %.0f/1     \t",
                  protocolData -> replyBitsIn_[i], protocolData -> replyBitsIn_[i] / 8192,
                      protocolData -> replyBitsOut_[i], protocolData -> replyBitsOut_[i] / 8192,
                          protocolData -> replyBitsIn_[i] / protocolData -> replyCount_[i],
                              protocolData -> replyBitsOut_[i] / protocolData -> replyCount_[i]);

      strcat(buffer, format);

      if (protocolData -> replyBitsOut_[i] > 0)
      {
        sprintf(format, "%5.3f:1\n", protocolData -> replyBitsIn_[i] /
                                         protocolData -> replyBitsOut_[i]);
      }
      else
      {
        sprintf(format, "1:1\n");
      }

      strcat(buffer, format);
    }

    countReplyIn       += protocolData -> replyCount_[i];
    countCachedReplyIn += protocolData -> replyCached_[i];

    countReplyBitsIn  += protocolData -> replyBitsIn_[i];
    countReplyBitsOut += protocolData -> replyBitsOut_[i];

    countAnyIn   += protocolData -> replyCount_[i];
    countBitsIn  += protocolData -> replyBitsIn_[i];
    countBitsOut += protocolData -> replyBitsOut_[i];
  }

  if (countReplyIn > 0)
  {
    if (countCachedReplyIn > 0)
    {
      sprintf(format, "\ntotal:  %.0f\t%.0f", countReplyIn, countCachedReplyIn);
    }
    else
    {
      sprintf(format, "\ntotal:  %.0f\t", countReplyIn);
    }

    strcat(buffer, format);

    sprintf(format, "\t%.0f (%.0f KB)\t%.0f (%.0f KB)\t%.0f/1 -> %.0f/1     \t",
                countReplyBitsIn, countReplyBitsIn / 8192, countReplyBitsOut,
                    countReplyBitsOut / 8192, countReplyBitsIn / countReplyIn,
                        countReplyBitsOut / countReplyIn);

    strcat(buffer, format);

    if (countReplyBitsOut > 0)
    {
      sprintf(format, "%5.3f:1\n", countReplyBitsIn / countReplyBitsOut);
    }
    else
    {
      sprintf(format, "1:1\n");
    }

    strcat(buffer, format);
  }
  else
  {
    strcat(buffer, "N/A\n");
  }

  strcat(buffer, "\n");

  //
  // Print event and error data.
  //

  strcat(buffer, "Event   Total\tCached\tBits In\t\tBits Out\tBits/Event\t\tRatio\n");
  strcat(buffer, "------- -----\t------\t-------\t\t--------\t----------\t\t-----\n");

  for (int i = 0; i < STATISTICS_OPCODE_MAX; i++)
  {
    if (protocolData -> eventCount_[i])
    {
      sprintf(format, "#%d ", i);

      while (strlen(format) < 8)
      {
        strcat(format, " ");
      }

      strcat(buffer, format);

      if (protocolData -> eventCached_[i] > 0)
      {
        sprintf(format, "%.0f\t%.0f", protocolData -> eventCount_[i],
                    protocolData -> eventCached_[i]);
      }
      else
      {
        sprintf(format, "%.0f\t", protocolData -> eventCount_[i]);
      }

      strcat(buffer, format);

      sprintf(format, "\t%.0f (%.0f KB)\t%.0f (%.0f KB)\t%.0f/1 -> %.0f/1     \t",
                  protocolData -> eventBitsIn_[i], protocolData -> eventBitsIn_[i] / 8192,
                      protocolData -> eventBitsOut_[i], protocolData -> eventBitsOut_[i] / 8192,
                          protocolData -> eventBitsIn_[i] / protocolData -> eventCount_[i],
                              protocolData -> eventBitsOut_[i] / protocolData -> eventCount_[i]);

      strcat(buffer, format);

      if (protocolData -> eventBitsOut_[i] > 0)
      {
        sprintf(format, "%5.3f:1\n", protocolData -> eventBitsIn_[i] /
                                         protocolData -> eventBitsOut_[i]);
      }
      else
      {
        sprintf(format, "1:1\n");
      }

      strcat(buffer, format);
    }

    countEventIn       += protocolData -> eventCount_[i];
    countCachedEventIn += protocolData -> eventCached_[i];

    countEventBitsIn  += protocolData -> eventBitsIn_[i];
    countEventBitsOut += protocolData -> eventBitsOut_[i];

    countAnyIn   += protocolData -> eventCount_[i];
    countBitsIn  += protocolData -> eventBitsIn_[i];
    countBitsOut += protocolData -> eventBitsOut_[i];
  }

  if (countEventIn > 0)
  {
    if (countCachedEventIn > 0)
    {
      sprintf(format, "\ntotal:  %.0f\t%.0f", countEventIn, countCachedEventIn);
    }
    else
    {
      sprintf(format, "\ntotal:  %.0f\t", countEventIn);
    }

    strcat(buffer, format);

    sprintf(format, "\t%.0f (%.0f KB)\t%.0f (%.0f KB)\t%.0f/1 -> %.0f/1     \t",
                countEventBitsIn, countEventBitsIn / 8192, countEventBitsOut,
                    countEventBitsOut / 8192, countEventBitsIn / countEventIn,
                        countEventBitsOut / countEventIn);

    strcat(buffer, format);

    if (countEventBitsOut > 0)
    {
      sprintf(format, "%5.3f:1\n", countEventBitsIn / countEventBitsOut);
    }
    else
    {
      sprintf(format, "1:1\n");
    }

    strcat(buffer, format);
  }
  else
  {
    strcat(buffer, "N/A\n\n");
  }

  //
  // Print transport data.
  //

  getTimeStats(type, buffer);

  countAnyIn   += protocolData -> cupsCount_;
  countBitsIn  += protocolData -> cupsBitsIn_;
  countBitsOut += protocolData -> cupsBitsOut_;

  countAnyIn   += protocolData -> smbCount_;
  countBitsIn  += protocolData -> smbBitsIn_;
  countBitsOut += protocolData -> smbBitsOut_;

  countAnyIn   += protocolData -> mediaCount_;
  countBitsIn  += protocolData -> mediaBitsIn_;
  countBitsOut += protocolData -> mediaBitsOut_;

  countAnyIn   += protocolData -> httpCount_;
  countBitsIn  += protocolData -> httpBitsIn_;
  countBitsOut += protocolData -> httpBitsOut_;

  countAnyIn   += protocolData -> fontCount_;
  countBitsIn  += protocolData -> fontBitsIn_;
  countBitsOut += protocolData -> fontBitsOut_;

  countAnyIn   += protocolData -> slaveCount_;
  countBitsIn  += protocolData -> slaveBitsIn_;
  countBitsOut += protocolData -> slaveBitsOut_;

  //
  // Save the overall amount of bytes
  // coming from X clients.
  //

  overallData -> overallBytesIn_ = countBitsIn / 8;

  //
  // Print performance data.
  //

  if (transportData -> readTime_ > 0)
  {
    sprintf(format, "      %.0f messages (%.0f KB) encoded per second.\n\n",
                countAnyIn / (transportData -> readTime_ / 1000),
                    (countBitsIn + transportData -> framingBitsOut_) / 8192 /
                         (transportData -> readTime_ / 1000));
  }
  else
  {
    sprintf(format, "      %.0f messages (%.0f KB) encoded per second.\n\n",
                countAnyIn, (countBitsIn + transportData ->
                    framingBitsOut_) / 8192);
  }

  strcat(buffer, format);

  strcat(buffer, "link: ");

  //
  // ZLIB compression stats.
  //

  getStreamStats(type, buffer);

  //
  // Save the overall amount of bytes
  // sent on NX proxy link.
  //

  if (transportData -> compressedBytesOut_ > 0)
  {
    overallData -> overallBytesOut_ = transportData -> compressedBytesOut_;
  }
  else
  {
    overallData -> overallBytesOut_ = countBitsOut / 8;
  }

  //
  // Print info on multiplexing overhead.
  //

  getFramingStats(type, buffer);

  //
  // Print stats about additional channels.
  //

  getServicesStats(type, buffer);

  //
  // Compression summary.
  //

  double ratio = 1;

  if (transportData -> compressedBytesOut_ / 1024 > 0)
  {
    ratio = ((countBitsIn + transportData -> framingBitsOut_) / 8192) /
                  (transportData -> compressedBytesOut_ / 1024);

  }
  else if (countBitsOut > 0)
  {
    ratio = (countBitsIn + transportData -> framingBitsOut_) /
                 countBitsOut;
  }

  sprintf(format, "      Protocol compression ratio is %5.3f:1.\n\n",
              ratio);

  strcat(buffer, format);

  getBitrateStats(type, buffer);

  //
  // These are not included in output.
  //
  // getSplitStats(type, buffer);
  //

  strcat(buffer, "\n");

  //
  // These statistics are not included in output.
  // You can check it anyway to get the effective
  // amount of bytes produced by unpack procedure.
  //
  // getClientOverallStats(type, buffer);
  //

  return 1;
}

int Statistics::getServerOverallStats(int type, char *&buffer)
{
  return 1;
}

int Statistics::getTimeStats(int type, char *&buffer)
{
  struct T_transportData *transportData;

  if (type == PARTIAL_STATS)
  {
    transportData = &transportPartial_;
  }
  else
  {
    transportData = &transportTotal_;
  }

  char format[FORMAT_LENGTH];

  sprintf(format, "\ntime: %.0f Ms idle, %.0f Ms (%.0f Ms in read, %.0f Ms in write) running.\n\n",
              transportData -> idleTime_, transportData -> readTime_,
                  transportData -> readTime_ - transportData -> writeTime_,
                      transportData -> writeTime_);

  strcat(buffer, format);

  return 1;
}

int Statistics::getStreamStats(int type, char *&buffer)
{
  struct T_transportData *transportData;

  if (type == PARTIAL_STATS)
  {
    transportData = &transportPartial_;
  }
  else
  {
    transportData = &transportTotal_;
  }

  char format[FORMAT_LENGTH];

  if (transportData -> compressedBytesOut_ > 0)
  {
    sprintf(format, "%.0f bytes (%.0f KB) compressed to %.0f (%.0f KB).\n",
                transportData -> compressedBytesIn_, transportData -> compressedBytesIn_ / 1024,
                    transportData -> compressedBytesOut_, transportData -> compressedBytesOut_ / 1024);

    strcat(buffer, format);

    sprintf(format, "      %5.3f:1 stream compression ratio.\n\n",
                transportData -> compressedBytesIn_ / transportData -> compressedBytesOut_);

    strcat(buffer, format);
  }

  if (transportData -> decompressedBytesOut_ > 0)
  {
    if (transportData -> compressedBytesOut_ > 0)
    {
      strcat(buffer, "      ");
    }

    sprintf(format, "%.0f bytes (%.0f KB) decompressed to %.0f (%.0f KB).\n",
                transportData -> decompressedBytesIn_, transportData -> decompressedBytesIn_ / 1024,
                    transportData -> decompressedBytesOut_, transportData -> decompressedBytesOut_ / 1024);

    strcat(buffer, format);

    sprintf(format, "      %5.3f:1 stream compression ratio.\n\n",
                transportData -> decompressedBytesOut_ / transportData -> decompressedBytesIn_);

    strcat(buffer, format);
  }

  if (transportData -> compressedBytesOut_ > 0 ||
          transportData -> decompressedBytesOut_ > 0)
  {
    strcat(buffer, "      ");
  }

  return 1;
}

int Statistics::getServicesStats(int type, char *&buffer)
{
  struct T_protocolData *protocolData;

  if (type == PARTIAL_STATS)
  {
    protocolData = &protocolPartial_;
  }
  else
  {
    protocolData = &protocolTotal_;
  }

  char format[FORMAT_LENGTH];

  if (protocolData -> cupsBitsOut_ > 0)
  {
    sprintf(format, "      %.0f CUPS messages, %.0f bytes (%.0f KB) in, %.0f bytes (%.0f KB) out.\n\n",
                protocolData -> cupsCount_ , protocolData -> cupsBitsIn_ / 8,
                    protocolData -> cupsBitsIn_ / 8192, protocolData -> cupsBitsOut_ / 8,
                        protocolData -> cupsBitsOut_ / 8192);

    strcat(buffer, format);
  }

  if (protocolData -> smbBitsOut_ > 0)
  {
    sprintf(format, "      %.0f SMB messages, %.0f bytes (%.0f KB) in, %.0f bytes (%.0f KB) out.\n\n",
                protocolData -> smbCount_ , protocolData -> smbBitsIn_ / 8,
                    protocolData -> smbBitsIn_ / 8192, protocolData -> smbBitsOut_ / 8,
                        protocolData -> smbBitsOut_ / 8192);

    strcat(buffer, format);
  }

  if (protocolData -> mediaBitsOut_ > 0)
  {
    sprintf(format, "      %.0f multimedia messages, %.0f bytes (%.0f KB) in, %.0f bytes (%.0f KB) out.\n\n",
                protocolData -> mediaCount_ , protocolData -> mediaBitsIn_ / 8,
                    protocolData -> mediaBitsIn_ / 8192, protocolData -> mediaBitsOut_ / 8,
                        protocolData -> mediaBitsOut_ / 8192);

    strcat(buffer, format);
  }

  if (protocolData -> httpBitsOut_ > 0)
  {
    sprintf(format, "      %.0f HTTP messages, %.0f bytes (%.0f KB) in, %.0f bytes (%.0f KB) out.\n\n",
                protocolData -> httpCount_ , protocolData -> httpBitsIn_ / 8,
                    protocolData -> httpBitsIn_ / 8192, protocolData -> httpBitsOut_ / 8,
                        protocolData -> httpBitsOut_ / 8192);

    strcat(buffer, format);
  }

  if (protocolData -> fontBitsOut_ > 0)
  {
    sprintf(format, "      %.0f font server messages, %.0f bytes (%.0f KB) in, %.0f bytes (%.0f KB) out.\n\n",
                protocolData -> fontCount_ , protocolData -> fontBitsIn_ / 8,
                    protocolData -> fontBitsIn_ / 8192, protocolData -> fontBitsOut_ / 8,
                        protocolData -> fontBitsOut_ / 8192);

    strcat(buffer, format);
  }

  if (protocolData -> slaveBitsOut_ > 0)
  {
    sprintf(format, "      %.0f slave messages, %.0f bytes (%.0f KB) in, %.0f bytes (%.0f KB) out.\n\n",
                protocolData -> slaveCount_ , protocolData -> slaveBitsIn_ / 8,
                    protocolData -> slaveBitsIn_ / 8192, protocolData -> slaveBitsOut_ / 8,
                        protocolData -> slaveBitsOut_ / 8192);

    strcat(buffer, format);
  }

  return 1;
}

int Statistics::getFramingStats(int type, char *&buffer)
{
  struct T_transportData *transportData;

  if (type == PARTIAL_STATS)
  {
    transportData = &transportPartial_;
  }
  else
  {
    transportData = &transportTotal_;
  }

  char format[FORMAT_LENGTH];

  //
  // Print info on multiplexing overhead.
  //

  sprintf(format, "%.0f frames in, %.0f frames out, %.0f writes out.\n\n",
              transportData -> proxyFramesIn_, transportData -> proxyFramesOut_,
                  transportData -> proxyWritesOut_);

  strcat(buffer, format);

  sprintf(format, "      %.0f bytes (%.0f KB) used for framing and multiplexing.\n\n",
              transportData -> framingBitsOut_ / 8, transportData -> framingBitsOut_ / 8192);

  strcat(buffer, format);

  return 1;
}

int Statistics::getBitrateStats(int type, char *&buffer)
{
  struct T_transportData *transportData;
  struct T_overallData   *overallData;

  if (type == PARTIAL_STATS)
  {
    transportData = &transportPartial_;
    overallData = &overallPartial_;
  }
  else
  {
    transportData = &transportTotal_;
    overallData = &overallTotal_;
  }

  double total = 0;

  if (transportData -> idleTime_ + transportData -> readTime_ > 0)
  {
    total = overallData -> overallBytesOut_ /
                ((transportData -> idleTime_ + transportData -> readTime_) / 1000);
  }

  char format[FORMAT_LENGTH];

  sprintf(format, "      %.0f B/s average, %d B/s %ds, %d B/s %ds, %d B/s maximum.\n\n",
              total, getBitrateInShortFrame(), control -> ShortBitrateTimeFrame / 1000,
                  getBitrateInLongFrame(), control -> LongBitrateTimeFrame / 1000,
                      getTopBitrate());

  strcat(buffer, format);

  resetTopBitrate();

  return 1;
}

int Statistics::getSplitStats(int type, char *&buffer)
{
  //
  // Don't print these statistics if persistent
  // cache of images is disabled.
  //

  if (control -> ImageCacheEnableLoad == 0 &&
          control -> ImageCacheEnableSave == 0)
  {
    return 0;
  }

  struct T_splitData *splitData;

  if (type == PARTIAL_STATS)
  {
    splitData = &splitPartial_;
  }
  else
  {
    splitData = &splitTotal_;
  }

  char format[FORMAT_LENGTH];

  //
  // Print info on split messages restored from disk.
  //

  sprintf(format, "      %.0f images streamed, %.0f restored, %.0f bytes (%.0f KB) cached.\n\n",
              splitData -> splitCount_, splitData -> splitAborted_, splitData -> splitAbortedBytesOut_,
                  splitData -> splitAbortedBytesOut_ / 1024);

  strcat(buffer, format);

  return 1;
}
