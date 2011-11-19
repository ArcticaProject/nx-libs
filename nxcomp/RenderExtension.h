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

#ifndef RenderExtension_H
#define RenderExtension_H

#include "Message.h"

//
// Set the verbosity level.
//

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG
#undef  DUMP

//
// Compression of data part is not enabled as
// most messages of this type are smaller than
// the current data size compression threshold.
//

#define RENDEREXTENSION_ENABLE_CACHE               1
#define RENDEREXTENSION_ENABLE_DATA                0
#define RENDEREXTENSION_ENABLE_SPLIT               0
#define RENDEREXTENSION_ENABLE_COMPRESS            0

#define RENDEREXTENSION_DATA_LIMIT                 6144
#define RENDEREXTENSION_DATA_OFFSET                36

#define RENDEREXTENSION_CACHE_SLOTS                6000
#define RENDEREXTENSION_CACHE_THRESHOLD            20
#define RENDEREXTENSION_CACHE_LOWER_THRESHOLD      10

#define RENDEREXTENSION_CACHE_SLOTS_IF_PROTO_STEP_7    8000

//
// Used to build the table of minor opcodes.
//

#define RENDEREXTENSION_MINOR_OPCODE_LIMIT         256

//
// The message class.
//

class RenderMinorExtensionStore;

class RenderExtensionMessage : public Message
{
  friend class RenderExtensionStore;
  friend class RenderMinorExtensionStore;

  friend class RenderGenericRequestStore;
  friend class RenderCreatePictureStore;
  friend class RenderChangePictureStore;
  friend class RenderFreePictureStore;
  friend class RenderPictureClipStore;
  friend class RenderPictureTransformStore;
  friend class RenderPictureFilterStore;
  friend class RenderCreateGlyphSetStore;
  friend class RenderFreeGlyphSetStore;
  friend class RenderAddGlyphsStore;
  friend class RenderCompositeStore;
  friend class RenderCompositeGlyphsStore;
  friend class RenderFillRectanglesStore;
  friend class RenderTrapezoidsStore;
  friend class RenderTrianglesStore;

  friend class RenderCreatePictureCompatStore;
  friend class RenderFreePictureCompatStore;
  friend class RenderPictureClipCompatStore;
  friend class RenderCreateGlyphSetCompatStore;
  friend class RenderCompositeCompatStore;
  friend class RenderCompositeGlyphsCompatStore;

  public:

  RenderExtensionMessage()
  {
  }

  ~RenderExtensionMessage()
  {
  }

  //
  // We consider for this message a data offset of 36,
  // that is size of the biggest among all requests of
  // this extension. The most common requests have a
  // specific differential encoding, others are simply
  // encoded through an array of int or char caches.
  //

  private:

  union
  {
    struct
    {
      unsigned char type;

      unsigned char  char_data[32];
      unsigned short short_data[16];
      unsigned short long_data[8];
    }
    any;

    struct
    {
      unsigned char type;

      unsigned int src_id;
      unsigned int dst_id;

      unsigned int format;
      unsigned int mask;
    }
    create_picture;

    struct
    {
      unsigned char type;

      unsigned int src_id;
    }
    change_picture;

    struct
    {
      unsigned char type;

      unsigned int src_id;
    }
    free_picture;

    struct
    {
      unsigned char type;

      unsigned int src_id;

      unsigned short src_x;
      unsigned short src_y;
    }
    picture_clip;

    struct
    {
      unsigned char type;

      unsigned int src_id;
    }
    picture_transform;

    struct
    {
      unsigned char type;

      unsigned int src_id;
      unsigned int num_elm;
    }
    picture_filter;

    struct
    {
      unsigned char type;

      unsigned int set_id;
      unsigned int format;
    }
    create_set;

    struct
    {
      unsigned char type;

      unsigned int set_id;
    }
    free_set;

    struct
    {
      unsigned char type;

      unsigned int set_id;
      unsigned int num_elm;
    }
    add_glyphs;

    struct
    {
      unsigned char type;

      unsigned char op;

      unsigned int src_id;
      unsigned int msk_id;
      unsigned int dst_id;

      unsigned short src_x;
      unsigned short src_y;

      unsigned short msk_x;
      unsigned short msk_y;

      unsigned short dst_x;
      unsigned short dst_y;

      unsigned short width;
      unsigned short height;
    }
    composite;

    struct
    {
      unsigned char type;

      unsigned char op;

      unsigned char num_elm;

      unsigned int src_id;
      unsigned int dst_id;

      unsigned int format;
      unsigned int set_id;

      unsigned short src_x;
      unsigned short src_y;

      unsigned short offset_x;
      unsigned short offset_y;
    }
    composite_glyphs;

    struct
    {
      unsigned char type;

      unsigned char op;

      unsigned int dst_id;
    }
    fill_rectangles;

    struct
    {
      unsigned char type;

      unsigned char op;

      unsigned int src_id;
      unsigned int dst_id;

      unsigned int format;

      unsigned short src_x;
      unsigned short src_y;
    }
    trapezoids;

    struct
    {
      unsigned char type;

      unsigned char op;

      unsigned int src_id;
      unsigned int dst_id;

      unsigned int format;

      unsigned short src_x;
      unsigned short src_y;
    }
    triangles;

    struct
    {
      unsigned char type;

      unsigned char op;

      unsigned char num_elm;

      unsigned int src_id;
      unsigned int dst_id;

      unsigned int format;
      unsigned int set_id;

      unsigned short src_x;
      unsigned short src_y;

      unsigned short delta_x;
      unsigned short delta_y;
    }
    composite_glyphs_compat;

  }
  data;
};

class RenderExtensionStore : public MessageStore
{
  public:

  RenderExtensionStore(StaticCompressor *compressor);

  virtual ~RenderExtensionStore();

  virtual const char *name() const
  {
    return "RenderExtension";
  }

  virtual unsigned char opcode() const
  {
    return opcode_;
  }

  virtual unsigned int storage() const
  {
    return sizeof(RenderExtensionMessage);
  }

  //
  // Message handling methods.
  //

  public:

  virtual Message *create() const
  {
    return new RenderExtensionMessage();
  }

  virtual Message *create(const Message &message) const
  {
    return new RenderExtensionMessage((const RenderExtensionMessage &) message);
  }

  virtual void destroy(Message *message) const
  {
    delete (RenderExtensionMessage *) message;
  }

  //
  // Determine if the message must be stored
  // in the cache.
  //

  virtual int validateMessage(const unsigned char *buffer, int size);

  //
  // Since protocol step 5 these methods are
  // specialized in their minor opcode stores.
  //

  virtual int identitySize(const unsigned char *buffer, unsigned int size);

  virtual int encodeIdentity(EncodeBuffer &encodeBuffer, const unsigned char *buffer,
                                 const unsigned int size, int bigEndian,
                                     ChannelCache *channelCache) const;

  virtual int decodeIdentity(DecodeBuffer &decodeBuffer, unsigned char *&buffer,
                                 unsigned int &size, int bigEndian, WriteBuffer *writeBuffer,
                                     ChannelCache *channelCache) const;

  virtual int parseIdentity(Message *message, const unsigned char *buffer,
                                unsigned int size, int bigEndian) const;

  virtual int unparseIdentity(const Message *message, unsigned char *buffer,
                                  unsigned int size, int bigEndian) const;

  virtual void updateIdentity(EncodeBuffer &encodeBuffer, const Message *message,
                                  const Message *cachedMessage,
                                      ChannelCache *channelCache) const;

  virtual void updateIdentity(DecodeBuffer &decodeBuffer, const Message *message,
                                  ChannelCache *channelCache) const;

  virtual void identityChecksum(const Message *message, const unsigned char *buffer,
                                    unsigned int size, int bigEndian) const;

  virtual void dumpIdentity(const Message *message) const;

  private:

  unsigned char opcode_;

  //
  // Keep pointers to specialized classes.
  //

  RenderMinorExtensionStore *minors_[RENDEREXTENSION_MINOR_OPCODE_LIMIT];

  RenderMinorExtensionStore *generic_;
};

class RenderMinorExtensionStore : public MinorMessageStore
{
  public:

  virtual const char *name() const = 0;

  virtual int identitySize(const unsigned char *buffer, unsigned int size) = 0;

  virtual int encodeMessage(EncodeBuffer &encodeBuffer, const unsigned char *buffer,
                                const unsigned int size, int bigEndian,
                                    ChannelCache *channelCache) const = 0;

  virtual int decodeMessage(DecodeBuffer &decodeBuffer, unsigned char *&buffer,
                                unsigned int &size, unsigned char type, int bigEndian,
                                    WriteBuffer *writeBuffer, ChannelCache *channelCache) const = 0;

  virtual int parseIdentity(Message *message, const unsigned char *buffer,
                                unsigned int size, int bigEndian) const = 0;

  virtual int unparseIdentity(const Message *message, unsigned char *buffer,
                                  unsigned int size, int bigEndian) const = 0;

  virtual void updateIdentity(EncodeBuffer &encodeBuffer, const Message *message,
                                  const Message *cachedMessage,
                                      ChannelCache *channelCache) const = 0;

  virtual void updateIdentity(DecodeBuffer &decodeBuffer, const Message *message,
                                  ChannelCache *channelCache) const = 0;

  virtual void identityChecksum(const Message *message, const unsigned char *buffer,
                                    unsigned int size, md5_state_t *md5_state,
                                        int bigEndian) const = 0;

  //
  // Internal encode and decode utilities.
  //

  protected:

  void encodeLongData(EncodeBuffer &encodeBuffer, const unsigned char *buffer,
                          unsigned int offset, unsigned int size, int bigEndian,
                              ChannelCache *channelCache) const;
 
  void encodeIntData(EncodeBuffer &encodeBuffer, const unsigned char *buffer,
                         unsigned int offset, unsigned int size, int bigEndian,
                             ChannelCache *channelCache) const;
 
  void encodeCharData(EncodeBuffer &encodeBuffer, const unsigned char *buffer,
                          unsigned int offset, unsigned int size, int bigEndian,
                              ChannelCache *channelCache) const;

  void decodeLongData(DecodeBuffer &decodeBuffer, unsigned char *buffer,
                          unsigned int offset, unsigned int size, int bigEndian,
                              ChannelCache *channelCache) const;

  void decodeIntData(DecodeBuffer &decodeBuffer, unsigned char *buffer,
                         unsigned int offset, unsigned int size, int bigEndian,
                             ChannelCache *channelCache) const;

  void decodeCharData(DecodeBuffer &decodeBuffer, unsigned char *buffer,
                          unsigned int offset, unsigned int size, int bigEndian,
                              ChannelCache *channelCache) const;

  /*
   * The following methods are only used in the
   * encoding of the generic render request. To
   * be removed in future.
   */

  void parseIntData(const Message *message, const unsigned char *buffer,
                        unsigned int offset, unsigned int size,
                            int bigEndian) const;

  void unparseIntData(const Message *message, unsigned char *buffer,
                          unsigned int offset, unsigned int size,
                              int bigEndian) const;

  void updateIntData(EncodeBuffer &encodeBuffer, const Message *message,
                         const Message *cachedMessage, unsigned int offset,
                             unsigned int size, ChannelCache *channelCache) const;

  void updateIntData(DecodeBuffer &decodeBuffer, const Message *message,
                         unsigned int offset, unsigned int size,
                             ChannelCache *channelCache) const;
};

#endif /* RenderExtension_H */
