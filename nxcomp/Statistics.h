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

#ifndef Statistics_H
#define Statistics_H

#include "NXproto.h"

#include "Misc.h"
#include "Timestamp.h"

class Proxy;

//
// Opcode 255 is for generic requests, 1 is for
// generic replies (those which haven't a speci-
// fic differential encoding), opcode 0 is for
// generic messages from the auxiliary channels.
//

#define STATISTICS_OPCODE_MAX       256

//
// Maximum length of the buffer allocated for
// the statistics output.
//

#define STATISTICS_LENGTH           16384

//
// Query type.
//

#define TOTAL_STATS                 1
#define PARTIAL_STATS               2
#define NO_STATS                    3

//
// Log level.
//

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

//
// Log the operations related to updating
// the control tokens.
//

#undef  TOKEN

class Statistics
{
  public:

  //
  // Use the proxy class to get access
  // to the message stores.
  //

  Statistics(Proxy *proxy);

  ~Statistics();

  void addIdleTime(unsigned int numMs)
  {
    transportPartial_.idleTime_ += numMs;
    transportTotal_.idleTime_ += numMs;
  }

  void subIdleTime(unsigned int numMs)
  {
    transportPartial_.idleTime_ -= numMs;
    transportTotal_.idleTime_ -= numMs;
  }

  void addReadTime(unsigned int numMs)
  {
    transportPartial_.readTime_ += numMs;
    transportTotal_.readTime_ += numMs;
  }

  void subReadTime(unsigned int numMs)
  {
    transportPartial_.readTime_ -= numMs;
    transportTotal_.readTime_ -= numMs;
  }

  void addWriteTime(unsigned int numMs)
  {
    transportPartial_.writeTime_ += numMs;
    transportTotal_.writeTime_ += numMs;
  }

  void subWriteTime(unsigned int numMs)
  {
    transportPartial_.writeTime_ -= numMs;
    transportTotal_.writeTime_ -= numMs;
  }

  void addBytesIn(unsigned int numBytes)
  {
    transportPartial_.proxyBytesIn_ += numBytes;
    transportTotal_.proxyBytesIn_ += numBytes;
  }

  double getBytesIn()
  {
    return transportTotal_.proxyBytesIn_;
  }

  void addBytesOut(unsigned int numBytes)
  {
    transportPartial_.proxyBytesOut_ += numBytes;
    transportTotal_.proxyBytesOut_ += numBytes;
  }

  double getBytesOut()
  {
    return transportTotal_.proxyBytesOut_;
  }

  void addFrameIn()
  {
    transportPartial_.proxyFramesIn_++;
    transportTotal_.proxyFramesIn_++;

    #ifdef TEST
    *logofs << "Statistics: Updated total proxy frames in to "
            << transportTotal_.proxyFramesIn_ << " at "
            << strMsTimestamp() << ".\n" << logofs_flush;
    #endif
  }

  void addFrameOut()
  {
    transportPartial_.proxyFramesOut_++;
    transportTotal_.proxyFramesOut_++;

    #ifdef TEST
    *logofs << "Statistics: Updated total proxy frames out to "
            << transportTotal_.proxyFramesOut_ << " at "
            << strMsTimestamp() << ".\n"
            << logofs_flush;
    #endif
  }

  void addWriteOut()
  {
    transportPartial_.proxyWritesOut_++;
    transportTotal_.proxyWritesOut_++;

    #ifdef TEST
    *logofs << "Statistics: Updated total proxy writes out to "
            << transportTotal_.proxyWritesOut_ << " at "
            << strMsTimestamp() << ".\n" << logofs_flush;
    #endif
  }

  void addCompressedBytes(unsigned int bytesIn, unsigned int bytesOut);

  void addDecompressedBytes(unsigned int bytesIn, unsigned int bytesOut)
  {
    transportPartial_.decompressedBytesIn_ += bytesIn;
    transportTotal_.decompressedBytesIn_ += bytesIn;

    transportPartial_.decompressedBytesOut_ += bytesOut;
    transportTotal_.decompressedBytesOut_ += bytesOut;
  }

  void addFramingBits(unsigned int bitsOut)
  {
    transportPartial_.framingBitsOut_ += bitsOut;
    transportTotal_.framingBitsOut_ += bitsOut;

    proxyData_.protocolCount_ += bitsOut;
  }

  void addCachedRequest(unsigned int opcode)
  {
    protocolPartial_.requestCached_[opcode]++;
    protocolTotal_.requestCached_[opcode]++;
  }

  void addRenderCachedRequest(unsigned int opcode)
  {
    protocolPartial_.renderRequestCached_[opcode]++;
    protocolTotal_.renderRequestCached_[opcode]++;
  }

  void addRepliedRequest(unsigned int opcode)
  {
    protocolPartial_.requestReplied_[opcode]++;
    protocolTotal_.requestReplied_[opcode]++;
  }

  void addCachedReply(unsigned int opcode)
  {
    protocolPartial_.replyCached_[opcode]++;
    protocolTotal_.replyCached_[opcode]++;
  }

  void addRequestBits(unsigned int opcode, unsigned int bitsIn, unsigned int bitsOut)
  {
    #ifdef DEBUG
    *logofs << "Statistcs: Added " << bitsIn << " bits in and "
            << bitsOut << " bits out to opcode " << opcode
            << ".\n" << logofs_flush;
    #endif

    protocolPartial_.requestCount_[opcode]++;
    protocolTotal_.requestCount_[opcode]++;

    protocolPartial_.requestBitsIn_[opcode] += bitsIn;
    protocolTotal_.requestBitsIn_[opcode] += bitsIn;

    protocolPartial_.requestBitsOut_[opcode] += bitsOut;
    protocolTotal_.requestBitsOut_[opcode] += bitsOut;

    //
    // Don't account the split bits
    // to the control token.
    //

    if (opcode != X_NXSplitData && opcode != X_NXSplitEvent)
    {
      proxyData_.protocolCount_ += bitsOut;
    }
  }

  void addRenderRequestBits(unsigned int opcode, unsigned int bitsIn, unsigned int bitsOut)
  {
    #ifdef DEBUG
    *logofs << "Statistcs: Added " << bitsIn << " bits in and "
            << bitsOut << " bits out to render opcode " << opcode
            << ".\n" << logofs_flush;
    #endif

    protocolPartial_.renderRequestCount_[opcode]++;
    protocolTotal_.renderRequestCount_[opcode]++;

    protocolPartial_.renderRequestBitsIn_[opcode] += bitsIn;
    protocolTotal_.renderRequestBitsIn_[opcode] += bitsIn;

    protocolPartial_.renderRequestBitsOut_[opcode] += bitsOut;
    protocolTotal_.renderRequestBitsOut_[opcode] += bitsOut;
  }

  void addReplyBits(unsigned int opcode, unsigned int bitsIn, unsigned int bitsOut)
  {
    protocolPartial_.replyCount_[opcode]++;
    protocolTotal_.replyCount_[opcode]++;

    protocolPartial_.replyBitsIn_[opcode] += bitsIn;
    protocolTotal_.replyBitsIn_[opcode] += bitsIn;

    protocolPartial_.replyBitsOut_[opcode] += bitsOut;
    protocolTotal_.replyBitsOut_[opcode] += bitsOut;

    proxyData_.protocolCount_ += bitsOut;
  }

  void addEventBits(unsigned int opcode, unsigned int bitsIn, unsigned int bitsOut)
  {
    protocolPartial_.eventCount_[opcode]++;
    protocolTotal_.eventCount_[opcode]++;

    protocolPartial_.eventBitsIn_[opcode] += bitsIn;
    protocolTotal_.eventBitsIn_[opcode] += bitsIn;

    protocolPartial_.eventBitsOut_[opcode] += bitsOut;
    protocolTotal_.eventBitsOut_[opcode] += bitsOut;

    proxyData_.protocolCount_ += bitsOut;
  }

  void addCupsBits(unsigned int bitsIn, unsigned int bitsOut)
  {
    protocolPartial_.cupsCount_++;
    protocolTotal_.cupsCount_++;

    protocolPartial_.cupsBitsIn_ += bitsIn;
    protocolTotal_.cupsBitsIn_ += bitsIn;

    protocolPartial_.cupsBitsOut_ += bitsOut;
    protocolTotal_.cupsBitsOut_ += bitsOut;
  }

  void addSmbBits(unsigned int bitsIn, unsigned int bitsOut)
  {
    protocolPartial_.smbCount_++;
    protocolTotal_.smbCount_++;

    protocolPartial_.smbBitsIn_ += bitsIn;
    protocolTotal_.smbBitsIn_ += bitsIn;

    protocolPartial_.smbBitsOut_ += bitsOut;
    protocolTotal_.smbBitsOut_ += bitsOut;
  }

  void addMediaBits(unsigned int bitsIn, unsigned int bitsOut)
  {
    protocolPartial_.mediaCount_++;
    protocolTotal_.mediaCount_++;

    protocolPartial_.mediaBitsIn_ += bitsIn;
    protocolTotal_.mediaBitsIn_ += bitsIn;

    protocolPartial_.mediaBitsOut_ += bitsOut;
    protocolTotal_.mediaBitsOut_ += bitsOut;
  }

  void addHttpBits(unsigned int bitsIn, unsigned int bitsOut)
  {
    protocolPartial_.httpCount_++;
    protocolTotal_.httpCount_++;

    protocolPartial_.httpBitsIn_ += bitsIn;
    protocolTotal_.httpBitsIn_ += bitsIn;

    protocolPartial_.httpBitsOut_ += bitsOut;
    protocolTotal_.httpBitsOut_ += bitsOut;
  }

  void addFontBits(unsigned int bitsIn, unsigned int bitsOut)
  {
    protocolPartial_.fontCount_++;
    protocolTotal_.fontCount_++;

    protocolPartial_.fontBitsIn_ += bitsIn;
    protocolTotal_.fontBitsIn_ += bitsIn;

    protocolPartial_.fontBitsOut_ += bitsOut;
    protocolTotal_.fontBitsOut_ += bitsOut;
  }

  void addSlaveBits(unsigned int bitsIn, unsigned int bitsOut)
  {
    protocolPartial_.slaveCount_++;
    protocolTotal_.slaveCount_++;

    protocolPartial_.slaveBitsIn_ += bitsIn;
    protocolTotal_.slaveBitsIn_ += bitsIn;

    protocolPartial_.slaveBitsOut_ += bitsOut;
    protocolTotal_.slaveBitsOut_ += bitsOut;
  }

  void addPackedBytesIn(unsigned int numBytes)
  {
    packedPartial_.packedBytesIn_ += numBytes;
    packedTotal_.packedBytesIn_ += numBytes;
  }

  void addPackedBytesOut(unsigned int numBytes)
  {
    packedPartial_.packedBytesOut_ += numBytes;
    packedTotal_.packedBytesOut_ += numBytes;
  }

  void addSplit()
  {
    splitPartial_.splitCount_++;
    splitTotal_.splitCount_++;
  }

  void addSplitAborted()
  {
    splitPartial_.splitAborted_++;
    splitTotal_.splitAborted_++;
  }

  void addSplitAbortedBytesOut(unsigned int numBytes)
  {
    splitPartial_.splitAbortedBytesOut_ += numBytes;
    splitTotal_.splitAbortedBytesOut_ += numBytes;
  }

  //
  // Add the recorded protocol data to the proxy
  // token counters. We want to send bpth the token
  // request message and the data payload using a
  // single system write, so we must guess how many
  // output bytes we will generate.
  //

  void updateControlToken(int &count)
  {
    //
    // Total is the total number of protocol bytes
    // generated so far. We have saved the number
    // of bytes generated the last time the function
    // was called so we can calculate the difference.
    //
    // The number of protocol bits is updated as soon
    // as new bits are accumulated, to avoid summing
    // up all the opcodes in this routine. We add a
    // byte to the control bytes difference to account
    // for the framing bits that are very likely to
    // be added to the payload.
    //

    double total = proxyData_.protocolCount_ / 8;

    double diff = total - proxyData_.controlCount_ + 1;

    #if defined(TEST) || defined(TOKEN)
    *logofs << "Statistics: TOKEN! Protocol bytes are "
            << total  << " control bytes are "
            << proxyData_.controlCount_ << " difference is "
            << diff << ".\n" << logofs_flush;
    #endif

    count += (int) (diff / proxyData_.streamRatio_);

    #if defined(TEST) || defined(TOKEN)
    *logofs << "Statistics: TOKEN! Adding "
            << (int) (diff / proxyData_.streamRatio_)
            << " bytes to the control token with ratio "
            << proxyData_.streamRatio_ << ".\n"
            << logofs_flush;
    #endif

    proxyData_.controlCount_ = total;

    #if defined(TEST) || defined(TOKEN)
    *logofs << "Statistics: TOKEN! New control token has "
            << count << " bytes with total control bytes "
            << proxyData_.controlCount_ << ".\n"
            << logofs_flush;
    #endif
  }

  void updateSplitToken(int &count)
  {
    double total = (protocolTotal_.requestBitsOut_[X_NXSplitData] +
                        protocolTotal_.eventBitsOut_[X_NXSplitEvent]) / 8;

    double diff = total - proxyData_.splitCount_;

    #if defined(TEST) || defined(TOKEN)
    *logofs << "Statistics: TOKEN! Protocol bytes are "
            << total  << " split bytes are "
            << proxyData_.splitCount_ << " difference is "
            << diff << ".\n" << logofs_flush;
    #endif

    count += (int) (diff / proxyData_.streamRatio_);

    #if defined(TEST) || defined(TOKEN)
    *logofs << "Statistics: TOKEN! Adding "
            << (int) (diff / proxyData_.streamRatio_)
            << " bytes to the split token with ratio "
            << proxyData_.streamRatio_ << ".\n"
            << logofs_flush;
    #endif

    proxyData_.splitCount_ = total;

    #if defined(TEST) || defined(TOKEN)
    *logofs << "Statistics: TOKEN! New split token has "
            << count << " bytes with total split bytes "
            << proxyData_.splitCount_ << ".\n"
            << logofs_flush;
    #endif
  }

  void updateDataToken(int &count)
  {
    double total = (protocolTotal_.cupsBitsOut_ +
                        protocolTotal_.smbBitsOut_ +
                            protocolTotal_.mediaBitsOut_ +
                                protocolTotal_.httpBitsOut_ +
                                    protocolTotal_.fontBitsOut_ +
                                        protocolTotal_.slaveBitsOut_) / 8;

    double diff = total - proxyData_.dataCount_;

    #if defined(TEST) || defined(TOKEN)
    *logofs << "Statistics: TOKEN! Protocol bytes are "
            << total  << " data bytes are "
            << proxyData_.dataCount_ << " difference is "
            << diff << ".\n" << logofs_flush;
    #endif

    count += (int) (diff / proxyData_.streamRatio_);

    #if defined(TEST) || defined(TOKEN)
    *logofs << "Statistics: TOKEN! Adding "
            << (int) (diff / proxyData_.streamRatio_)
            << " bytes to the data token with ratio "
            << proxyData_.streamRatio_ << ".\n"
            << logofs_flush;
    #endif

    proxyData_.dataCount_ = total;

    #if defined(TEST) || defined(TOKEN)
    *logofs << "Statistics: TOKEN! New data token has "
            << count << " bytes with total data bytes "
            << proxyData_.dataCount_ << ".\n"
            << logofs_flush;
    #endif
  }

  //
  // Update the current bitrate.
  //

  void updateBitrate(int bytes);

  //
  // Return the current bitrate.
  //

  int getBitrateInShortFrame()
  {
    return bitrateInShortFrame_;
  }

  int getBitrateInLongFrame()
  {
    return bitrateInLongFrame_;
  }

  int getTopBitrate()
  {
    return topBitrate_;
  }

  void resetTopBitrate()
  {
    topBitrate_ = 0;
  }

  double getStreamRatio()
  {
    return proxyData_.streamRatio_;
  }

  //
  // Manage the congestion level.
  //

  void updateCongestion(int remaining, int limit);

  double getCongestionInFrame()
  {
    return congestionInFrame_;
  }

  //
  // Produce a dump of the statistics on
  // the provided buffer.
  //

  int getClientCacheStats(int type, char *&buffer);
  int getClientProtocolStats(int type, char *&buffer);
  int getClientOverallStats(int type, char *&buffer);

  int getServerCacheStats(int type, char *&buffer);
  int getServerProtocolStats(int type, char *&buffer);
  int getServerOverallStats(int type, char *&buffer);

  int resetPartialStats();

  private:

  int getTimeStats(int type, char *&buffer);
  int getServicesStats(int type, char *&buffer);
  int getFramingStats(int type, char *&buffer);
  int getBitrateStats(int type, char *&buffer);
  int getStreamStats(int type, char *&buffer);
  int getSplitStats(int type, char *&buffer);

  struct T_protocolData
  {
    double requestCached_[STATISTICS_OPCODE_MAX];
    double requestReplied_[STATISTICS_OPCODE_MAX];
    double requestCount_[STATISTICS_OPCODE_MAX];
    double requestBitsIn_[STATISTICS_OPCODE_MAX];
    double requestBitsOut_[STATISTICS_OPCODE_MAX];

    double renderRequestCached_[STATISTICS_OPCODE_MAX];
    double renderRequestCount_[STATISTICS_OPCODE_MAX];
    double renderRequestBitsIn_[STATISTICS_OPCODE_MAX];
    double renderRequestBitsOut_[STATISTICS_OPCODE_MAX];

    double replyCached_[STATISTICS_OPCODE_MAX];
    double replyCount_[STATISTICS_OPCODE_MAX];
    double replyBitsIn_[STATISTICS_OPCODE_MAX];
    double replyBitsOut_[STATISTICS_OPCODE_MAX];

    double eventCached_[STATISTICS_OPCODE_MAX];
    double eventCount_[STATISTICS_OPCODE_MAX];
    double eventBitsIn_[STATISTICS_OPCODE_MAX];
    double eventBitsOut_[STATISTICS_OPCODE_MAX];

    double cupsCount_;
    double cupsBitsIn_;
    double cupsBitsOut_;

    double smbCount_;
    double smbBitsIn_;
    double smbBitsOut_;

    double mediaCount_;
    double mediaBitsIn_;
    double mediaBitsOut_;

    double httpCount_;
    double httpBitsIn_;
    double httpBitsOut_;

    double fontCount_;
    double fontBitsIn_;
    double fontBitsOut_;

    double slaveCount_;
    double slaveBitsIn_;
    double slaveBitsOut_;
  };

  struct T_transportData
  {
    double idleTime_;
    double readTime_;
    double writeTime_;

    double proxyBytesIn_;
    double proxyBytesOut_;

    double proxyFramesIn_;
    double proxyFramesOut_;
    double proxyWritesOut_;

    double compressedBytesIn_;
    double compressedBytesOut_;

    double decompressedBytesIn_;
    double decompressedBytesOut_;

    double framingBitsOut_;
  };

  struct T_packedData
  {
    double packedBytesIn_;
    double packedBytesOut_;
  };

  struct T_splitData
  {
    double splitCount_;
    double splitAborted_;
    double splitAbortedBytesOut_;
  };

  struct T_overallData
  {
    double overallBytesIn_;
    double overallBytesOut_;
  };

  struct T_proxyData
  {
    double protocolCount_;
    double controlCount_;
    double splitCount_;
    double dataCount_;

    double streamRatio_;
  };

  T_protocolData protocolPartial_;
  T_protocolData protocolTotal_;

  T_transportData transportPartial_;
  T_transportData transportTotal_;

  T_packedData packedPartial_;
  T_packedData packedTotal_;

  T_splitData splitPartial_;
  T_splitData splitTotal_;

  T_overallData overallPartial_;
  T_overallData overallTotal_;

  T_proxyData proxyData_;

  //
  // Used to calculate the bandwidth usage
  // of the proxy link.
  //

  T_timestamp startShortFrameTs_;
  T_timestamp startLongFrameTs_;
  T_timestamp startFrameTs_;

  int bytesInShortFrame_;
  int bytesInLongFrame_;

  int bitrateInShortFrame_;
  int bitrateInLongFrame_;

  int topBitrate_;

  double congestionInFrame_;

  //
  // Need the proxy pointer to print the
  // statistics related to the client and
  // server stores and to add the protocol
  // data to the proxy token accumulators.
  //

  Proxy *proxy_;
};

#endif /* Statistics_H */
