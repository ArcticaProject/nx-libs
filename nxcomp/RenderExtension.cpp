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

#include "NXrender.h"

#include "ClientCache.h"

#include "EncodeBuffer.h"
#include "DecodeBuffer.h"

#include "WriteBuffer.h"

#include "RenderExtension.h"

#include "RenderGenericRequest.h"
#include "RenderCreatePicture.h"
#include "RenderChangePicture.h"
#include "RenderFreePicture.h"
#include "RenderPictureClip.h"
#include "RenderPictureTransform.h"
#include "RenderPictureFilter.h"
#include "RenderCreateGlyphSet.h"
#include "RenderFreeGlyphSet.h"
#include "RenderAddGlyphs.h"
#include "RenderComposite.h"
#include "RenderCompositeGlyphs.h"
#include "RenderFillRectangles.h"
#include "RenderTrapezoids.h"
#include "RenderTriangles.h"

#include "RenderCreatePictureCompat.h"
#include "RenderFreePictureCompat.h"
#include "RenderPictureClipCompat.h"
#include "RenderCreateGlyphSetCompat.h"
#include "RenderCompositeCompat.h"
#include "RenderCompositeGlyphsCompat.h"

//
// Set the verbosity level.
//

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

//
// Constructor and destructor.
//

RenderExtensionStore::RenderExtensionStore(StaticCompressor *compressor)

  : MessageStore(compressor)
{
  enableCache    = RENDEREXTENSION_ENABLE_CACHE;
  enableData     = RENDEREXTENSION_ENABLE_DATA;
  enableSplit    = RENDEREXTENSION_ENABLE_SPLIT;
  enableCompress = RENDEREXTENSION_ENABLE_COMPRESS;

  generic_ = new RenderGenericRequestStore();

  for (int i = 0; i < RENDEREXTENSION_MINOR_OPCODE_LIMIT; i++)
  {
    minors_[i] = generic_;
  }

  minors_[X_RenderChangePicture]  = new RenderChangePictureStore();
  minors_[X_RenderFillRectangles] = new RenderFillRectanglesStore();
  minors_[X_RenderAddGlyphs]      = new RenderAddGlyphsStore();

  if (control -> isProtoStep7() == 1)
  {
    minors_[X_RenderCreatePicture]            = new RenderCreatePictureStore();
    minors_[X_RenderFreePicture]              = new RenderFreePictureStore();
    minors_[X_RenderSetPictureClipRectangles] = new RenderPictureClipStore();
    minors_[X_RenderCreateGlyphSet]           = new RenderCreateGlyphSetStore();
    minors_[X_RenderComposite]                = new RenderCompositeStore();
    minors_[X_RenderCompositeGlyphs8]         = new RenderCompositeGlyphsStore();
    minors_[X_RenderCompositeGlyphs16]        = new RenderCompositeGlyphsStore();
    minors_[X_RenderCompositeGlyphs32]        = new RenderCompositeGlyphsStore();

    minors_[X_RenderSetPictureTransform] = new RenderPictureTransformStore();
    minors_[X_RenderSetPictureFilter]    = new RenderPictureFilterStore();
    minors_[X_RenderFreeGlyphSet]        = new RenderFreeGlyphSetStore();
    minors_[X_RenderTrapezoids]          = new RenderTrapezoidsStore();
    minors_[X_RenderTriangles]           = new RenderTrianglesStore();
  }
  else
  {
    minors_[X_RenderCreatePicture]            = new RenderCreatePictureCompatStore();
    minors_[X_RenderFreePicture]              = new RenderFreePictureCompatStore();
    minors_[X_RenderSetPictureClipRectangles] = new RenderPictureClipCompatStore();
    minors_[X_RenderCreateGlyphSet]           = new RenderCreateGlyphSetCompatStore();
    minors_[X_RenderComposite]                = new RenderCompositeCompatStore();
    minors_[X_RenderCompositeGlyphs8]         = new RenderCompositeGlyphsCompatStore();
    minors_[X_RenderCompositeGlyphs16]        = new RenderCompositeGlyphsCompatStore();
    minors_[X_RenderCompositeGlyphs32]        = new RenderCompositeGlyphsCompatStore();
  }

  dataLimit  = RENDEREXTENSION_DATA_LIMIT;
  dataOffset = RENDEREXTENSION_DATA_OFFSET;

  if (control -> isProtoStep7() == 1)
  {
    cacheSlots = RENDEREXTENSION_CACHE_SLOTS_IF_PROTO_STEP_7;
  }
  else
  {
    cacheSlots = RENDEREXTENSION_CACHE_SLOTS;
  }

  cacheThreshold      = RENDEREXTENSION_CACHE_THRESHOLD;
  cacheLowerThreshold = RENDEREXTENSION_CACHE_LOWER_THRESHOLD;

  opcode_ = X_NXInternalRenderExtension;

  messages_ -> resize(cacheSlots);

  for (T_messages::iterator i = messages_ -> begin();
           i < messages_ -> end(); i++)
  {
    *i = NULL;
  }

  temporary_ = NULL;
}

RenderExtensionStore::~RenderExtensionStore()
{
  for (int i = 0; i < RENDEREXTENSION_MINOR_OPCODE_LIMIT; i++)
  {
    if (minors_[i] != generic_)
    {
      delete minors_[i];
    }
  }

  delete generic_;

  for (T_messages::iterator i = messages_ -> begin();
           i < messages_ -> end(); i++)
  {
    destroy(*i);
  }

  destroy(temporary_);
}

int RenderExtensionStore::validateMessage(const unsigned char *buffer, int size)
{
  #ifdef TEST
  *logofs << name() << ": Encoding message OPCODE#"
          << (unsigned) *buffer << " MINOR#" << (unsigned)
             *(buffer + 1) << " with size " << size
          << ".\n" << logofs_flush;
  #endif

  return (size >= control -> MinimumMessageSize &&
              size <= control -> MaximumMessageSize);
}

//
// Here are the methods to handle the messages' content.
//

int RenderExtensionStore::identitySize(const unsigned char *buffer, unsigned int size)
{
  return minors_[*(buffer + 1)] -> identitySize(buffer, size);
}

int RenderExtensionStore::encodeIdentity(EncodeBuffer &encodeBuffer, const unsigned char *buffer,
                                             const unsigned int size, int bigEndian,
                                                 ChannelCache *channelCache) const
{
  encodeBuffer.encodeOpcodeValue(*(buffer + 1),
                     ((ClientCache *) channelCache) -> renderOpcodeCache);

  minors_[*(buffer + 1)] -> encodeMessage(encodeBuffer, buffer, size,
                                              bigEndian, channelCache);

  return 1;
}

int RenderExtensionStore::decodeIdentity(DecodeBuffer &decodeBuffer, unsigned char *&buffer,
                                             unsigned int &size, int bigEndian, WriteBuffer *writeBuffer,
                                                 ChannelCache *channelCache) const
{
  unsigned char type;

  decodeBuffer.decodeOpcodeValue(type,
                     ((ClientCache *) channelCache) -> renderOpcodeCache);

  minors_[type] -> decodeMessage(decodeBuffer, buffer, size, type,
                                     bigEndian, writeBuffer, channelCache);

  return 1;
}

int RenderExtensionStore::parseIdentity(Message *message, const unsigned char *buffer,
                                            unsigned int size, int bigEndian) const
{
  return minors_[*(buffer + 1)] -> parseIdentity(message, buffer, size, bigEndian);
}

int RenderExtensionStore::unparseIdentity(const Message *message, unsigned char *buffer,
                                              unsigned int size, int bigEndian) const
{
  return minors_[((RenderExtensionMessage *) message) -> data.any.type] ->
                     unparseIdentity(message, buffer, size, bigEndian);
}

void RenderExtensionStore::identityChecksum(const Message *message, const unsigned char *buffer,
                                               unsigned int size, int bigEndian) const
{
  minors_[*(buffer + 1)] -> identityChecksum(message, buffer, size, md5_state_, bigEndian);
}

void RenderExtensionStore::updateIdentity(EncodeBuffer &encodeBuffer, const Message *message,
                                              const Message *cachedMessage,
                                                   ChannelCache *channelCache) const
{
  minors_[((RenderExtensionMessage *) message) -> data.any.type] ->
              updateIdentity(encodeBuffer, message, cachedMessage, channelCache);
}

void RenderExtensionStore::updateIdentity(DecodeBuffer &decodeBuffer, const Message *message,
                                              ChannelCache *channelCache) const
{
  minors_[((RenderExtensionMessage *) message) -> data.any.type] ->
              updateIdentity(decodeBuffer, message, channelCache);
}

void RenderExtensionStore::dumpIdentity(const Message *message) const
{
  #ifdef DUMP

  #ifdef WARNING
  *logofs << name() << ": WARNING! Dump of identity not implemented.\n"
          << logofs_flush;
  #endif

  #endif
}

//
// TODO: The following encoding and decoding functions
// could be generalized further, for example by passing
// the pointer to the data cache, the number of caches
// made available by the caller and the first cache to
// iterate through.
//

void RenderMinorExtensionStore::encodeLongData(EncodeBuffer &encodeBuffer, const unsigned char *buffer,
                                                   unsigned int offset, unsigned int size, int bigEndian,
                                                       ChannelCache *channelCache) const
{
  if (control -> isProtoStep7() == 1)
  {
    encodeBuffer.encodeLongData(buffer + offset, size - offset);

    #ifdef TEST
    *logofs << name() << ": Encoded " << size - offset
            << " bytes of long data.\n" << logofs_flush;
    #endif

    return;
  }

  ClientCache *clientCache = (ClientCache *) channelCache;

  for (unsigned int i = offset, c = (offset - 4) % 16; i < size; i += 4)
  {
    #ifdef DEBUG
    *logofs << name() << ": Encoding int with i = " << i << " c = "
            << c << ".\n" << logofs_flush;
    #endif

    encodeBuffer.encodeCachedValue(GetULONG(buffer + i, bigEndian), 32,
                       *clientCache -> renderDataCache[c]);

    if (++c == 16) c = 0;
  }
}

void RenderMinorExtensionStore::encodeIntData(EncodeBuffer &encodeBuffer, const unsigned char *buffer,
                                                  unsigned int offset, unsigned int size, int bigEndian,
                                                      ChannelCache *channelCache) const
{
  if (control -> isProtoStep7() == 1)
  {
    encodeBuffer.encodeIntData(buffer + offset, size - offset);

    #ifdef TEST
    *logofs << name() << ": Encoded " << size - offset
            << " bytes of int data.\n" << logofs_flush;
    #endif

    return;
  }

  ClientCache *clientCache = (ClientCache *) channelCache;

  for (unsigned int i = offset, c = (offset - 4) % 16; i < size; i += 2)
  {
    #ifdef DEBUG
    *logofs << name() << ": Encoding int with i = " << i << " c = "
            << c << ".\n" << logofs_flush;
    #endif

    encodeBuffer.encodeCachedValue(GetUINT(buffer + i, bigEndian), 16,
                       *clientCache -> renderDataCache[c]);

    if (++c == 16) c = 0;
  }
}

void RenderMinorExtensionStore::encodeCharData(EncodeBuffer &encodeBuffer, const unsigned char *buffer,
                                                   unsigned int offset, unsigned int size, int bigEndian,
                                                       ChannelCache *channelCache) const
{
  if (control -> isProtoStep7() == 1)
  {
    encodeBuffer.encodeTextData(buffer + offset, size - offset);

    #ifdef TEST
    *logofs << name() << ": Encoded " << size - offset
            << " bytes of text data.\n" << logofs_flush;
    #endif

    return;
  }

  ClientCache *clientCache = (ClientCache *) channelCache;

  clientCache -> renderTextCompressor.reset();

  const unsigned char *next = buffer + offset;

  for (unsigned int i = offset; i < size; i++)
  {
    #ifdef DEBUG
    *logofs << name() << ": Encoding char with i = " << i
            << ".\n" << logofs_flush;
    #endif

    clientCache -> renderTextCompressor.
          encodeChar(*next++, encodeBuffer);
  }
}

void RenderMinorExtensionStore::decodeLongData(DecodeBuffer &decodeBuffer, unsigned char *buffer,
                                                   unsigned int offset, unsigned int size, int bigEndian,
                                                       ChannelCache *channelCache) const
{
  if (control -> isProtoStep7() == 1)
  {
    decodeBuffer.decodeLongData(buffer + offset, size - offset);

    #ifdef TEST
    *logofs << name() << ": Decoded " << size - offset
            << " bytes of long data.\n" << logofs_flush;
    #endif

    return;
  }

  ClientCache *clientCache = (ClientCache *) channelCache;

  unsigned int value;

  for (unsigned int i = offset, c = (offset - 4) % 16; i < size; i += 4)
  {
    #ifdef DEBUG
    *logofs << name() << ": Decoding int with i = " << i << " c = "
            << c << ".\n" << logofs_flush;
    #endif

    decodeBuffer.decodeCachedValue(value, 32,
                       *clientCache -> renderDataCache[c]);

    PutULONG(value, buffer + i, bigEndian);

    if (++c == 16) c = 0;
  }
}

void RenderMinorExtensionStore::decodeIntData(DecodeBuffer &decodeBuffer, unsigned char *buffer,
                                                  unsigned int offset, unsigned int size, int bigEndian,
                                                      ChannelCache *channelCache) const
{
  if (control -> isProtoStep7() == 1)
  {
    decodeBuffer.decodeIntData(buffer + offset, size - offset);

    #ifdef TEST
    *logofs << name() << ": Decoded " << size - offset
            << " bytes of int data.\n" << logofs_flush;
    #endif

    return;
  }

  ClientCache *clientCache = (ClientCache *) channelCache;

  unsigned int value;

  for (unsigned int i = offset, c = (offset - 4) % 16; i < size; i += 2)
  {
    #ifdef DEBUG
    *logofs << name() << ": Decoding int with i = " << i << " c = "
            << c << ".\n" << logofs_flush;
    #endif

    decodeBuffer.decodeCachedValue(value, 16,
                       *clientCache -> renderDataCache[c]);

    PutUINT(value, buffer + i, bigEndian);

    if (++c == 16) c = 0;
  }
}

void RenderMinorExtensionStore::decodeCharData(DecodeBuffer &decodeBuffer, unsigned char *buffer,
                                                   unsigned int offset, unsigned int size, int bigEndian,
                                                       ChannelCache *channelCache) const
{
  if (control -> isProtoStep7() == 1)
  {
    decodeBuffer.decodeTextData(buffer + offset, size - offset);

    #ifdef TEST
    *logofs << name() << ": Decoded " << size - offset
            << " bytes of text data.\n" << logofs_flush;
    #endif

    return;
  }

  ClientCache *clientCache = (ClientCache *) channelCache;

  clientCache -> renderTextCompressor.reset();

  unsigned char *next = buffer + offset;

  for (unsigned int i = offset; i < size; i++)
  {
    #ifdef DEBUG
    *logofs << name() << ": Decoding char with i = " << i
            << ".\n" << logofs_flush;
    #endif

    *next++ = clientCache -> renderTextCompressor.
                    decodeChar(decodeBuffer);
  }
}

void RenderMinorExtensionStore::parseIntData(const Message *message, const unsigned char *buffer,
                                                 unsigned int offset, unsigned int size,
                                                     int bigEndian) const
{
  RenderExtensionMessage *renderExtension = (RenderExtensionMessage *) message;

  unsigned int last = ((unsigned) message -> i_size_ > size ? size : message -> i_size_);

  for (unsigned int i = offset, c = (offset - 4) % 16; i < last; i += 2)
  {
    #ifdef DEBUG
    *logofs << name() << ": Parsing int with i = " << i << " c = "
            << c << ".\n" << logofs_flush;
    #endif

    renderExtension -> data.any.short_data[c] = GetUINT(buffer + i, bigEndian);

    if (++c == 16) c = 0;
  }
}

void RenderMinorExtensionStore::unparseIntData(const Message *message, unsigned char *buffer,
                                                   unsigned int offset, unsigned int size,
                                                       int bigEndian) const
{
  RenderExtensionMessage *renderExtension = (RenderExtensionMessage *) message;

  unsigned int last = ((unsigned) message -> i_size_ > size ? size : message -> i_size_);

  for (unsigned int i = offset, c = (offset - 4) % 16; i < last; i += 2)
  {
    #ifdef DEBUG
    *logofs << name() << ": Unparsing int with i = " << i << " c = "
            << c << ".\n" << logofs_flush;
    #endif

    PutUINT(renderExtension -> data.any.short_data[c], buffer + i, bigEndian);

    if (++c == 16) c = 0;
  }
}

void RenderMinorExtensionStore::updateIntData(EncodeBuffer &encodeBuffer, const Message *message,
                                                  const Message *cachedMessage, unsigned int offset,
                                                      unsigned int size, ChannelCache *channelCache) const
{
  RenderExtensionMessage *renderExtension       = (RenderExtensionMessage *) message;
  RenderExtensionMessage *cachedRenderExtension = (RenderExtensionMessage *) cachedMessage;

  ClientCache *clientCache = (ClientCache *) channelCache;

  unsigned int last = ((unsigned) message -> i_size_ > size ? size : message -> i_size_);

  for (unsigned int i = offset, c = (offset - 4) % 16; i < last; i += 2)
  {
    #ifdef DEBUG
    *logofs << name() << ": Encoding int update with i = " << i
            << " c = " << c << ".\n" << logofs_flush;
    #endif

    encodeBuffer.encodeCachedValue(renderExtension -> data.any.short_data[c], 16,
                       *clientCache -> renderDataCache[c]);

    cachedRenderExtension -> data.any.short_data[c] =
                renderExtension -> data.any.short_data[c];

    if (++c == 16) c = 0;
  }
}

void RenderMinorExtensionStore::updateIntData(DecodeBuffer &decodeBuffer, const Message *message,
                                                  unsigned int offset, unsigned int size,
                                                      ChannelCache *channelCache) const
{
  RenderExtensionMessage *renderExtension = (RenderExtensionMessage *) message;

  ClientCache *clientCache = (ClientCache *) channelCache;

  unsigned int last = ((unsigned) message -> i_size_ > size ? size : message -> i_size_);

  unsigned int value;

  for (unsigned int i = offset, c = (offset - 4) % 16; i < last; i += 2)
  {
    #ifdef DEBUG
    *logofs << name() << ": Decoding int update with i = " << i
            << " c = " << c << ".\n" << logofs_flush;
    #endif

    decodeBuffer.decodeCachedValue(value, 16,
                       *clientCache -> renderDataCache[c]);

    renderExtension -> data.any.short_data[c] = value;

    if (++c == 16) c = 0;
  }
}
