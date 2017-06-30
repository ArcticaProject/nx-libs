/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2011 NoMachine (http://www.nomachine.com)          */
/* Copyright (c) 2008-2014 Oleksandr Shneyder <o.shneyder@phoca-gmbh.de>  */
/* Copyright (c) 2014-2016 Ulrich Sibiller <uli42@gmx.de>                 */
/* Copyright (c) 2014-2016 Mihai Moldovan <ionic@ionic.de>                */
/* Copyright (c) 2011-2016 Mike Gabriel <mike.gabriel@das-netzwerkteam.de>*/
/* Copyright (c) 2015-2016 Qindel Group (http://www.qindel.com)           */
/*                                                                        */
/* NXCOMP, NX protocol compression and NX extensions to this software     */
/* are copyright of the aforementioned persons and companies.             */
/*                                                                        */
/* Redistribution and use of the present software is allowed according    */
/* to terms specified in the file LICENSE.nxcomp which comes in the       */
/* source distribution.                                                   */
/*                                                                        */
/* All rights reserved.                                                   */
/*                                                                        */
/* NOTE: This software has received contributions from various other      */
/* contributors, only the core maintainers and supporters are listed as   */
/* copyright holders. Please contact us, if you feel you should be listed */
/* as copyright holder, as well.                                          */
/*                                                                        */
/**************************************************************************/

//
// This file is included multiple times,
// one for each message inheriting the
// parent class.
//

public:

#if MESSAGE_HAS_SIZE

virtual void encodeSize(EncodeBuffer &encodeBuffer, const unsigned char *buffer,
                            const unsigned int size, int bigEndian,
                                ChannelCache *channelCache) const;

virtual void decodeSize(DecodeBuffer &decodeBuffer, unsigned char *&buffer,
                            unsigned int &size, unsigned char type, int bigEndian,
                                WriteBuffer *writeBuffer, ChannelCache *channelCache) const;

#endif

#if MESSAGE_HAS_DATA

virtual void encodeData(EncodeBuffer &encodeBuffer, const unsigned char *buffer,
                            unsigned int size, int bigEndian,
                                ChannelCache *channelCache) const;

virtual void decodeData(DecodeBuffer &decodeBuffer, unsigned char *buffer,
                            unsigned int size, int bigEndian,
                                ChannelCache *channelCache) const;

#endif

virtual int encodeMessage(EncodeBuffer &encodeBuffer, const unsigned char *buffer,
                              const unsigned int size, int bigEndian,
                                  ChannelCache *channelCache) const;

virtual int decodeMessage(DecodeBuffer &decodeBuffer, unsigned char *&buffer,
                              unsigned int &size, unsigned char type, int bigEndian,
                                  WriteBuffer *writeBuffer, ChannelCache *channelCache) const;

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
                                  unsigned int size, md5_state_t *md5_state,
                                      int bigEndian) const;
