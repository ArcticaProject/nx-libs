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

#include "ListFontsReply.h"

#include "ServerCache.h"

#include "EncodeBuffer.h"
#include "DecodeBuffer.h"

//
// Set the verbosity level.
//

#define PANIC
#define WARNING
#undef  DUMP
#undef  TEST
#undef  DEBUG

ListFontsReplyStore::ListFontsReplyStore(StaticCompressor *compressor)

  : MessageStore(compressor)
{
  enableCache    = LISTFONTSREPLY_ENABLE_CACHE;
  enableData     = LISTFONTSREPLY_ENABLE_DATA;
  enableSplit    = LISTFONTSREPLY_ENABLE_SPLIT;

  // Since ProtoStep7 (#issue 108)
  enableCompress = LISTFONTSREPLY_ENABLE_COMPRESS_IF_PROTO_STEP_7;

  dataLimit  = LISTFONTSREPLY_DATA_LIMIT;
  dataOffset = LISTFONTSREPLY_DATA_OFFSET;

  cacheSlots          = LISTFONTSREPLY_CACHE_SLOTS;
  cacheThreshold      = LISTFONTSREPLY_CACHE_THRESHOLD;
  cacheLowerThreshold = LISTFONTSREPLY_CACHE_LOWER_THRESHOLD;

  messages_ -> resize(cacheSlots);

  for (T_messages::iterator i = messages_ -> begin();
           i < messages_ -> end(); i++)
  {
    *i = NULL;
  }

  temporary_ = NULL;
}

ListFontsReplyStore::~ListFontsReplyStore()
{
  for (T_messages::iterator i = messages_ -> begin();
           i < messages_ -> end(); i++)
  {
    destroy(*i);
  }

  destroy(temporary_);
}

//
// Here are the methods to handle messages' content.
//

int ListFontsReplyStore::parseIdentity(Message *message, const unsigned char *buffer,
                                           unsigned int size, int bigEndian) const
{
  ListFontsReplyMessage *listFontsReply = (ListFontsReplyMessage *) message;

  //
  // Here is the fingerprint.
  //

  listFontsReply -> number_of_names = GetUINT(buffer + 8, bigEndian);

  //
  // Clean up padding bytes.
  //

  if ((int) size > dataOffset)
  {
    unsigned int current;
    unsigned int length;
    unsigned int nstringInNames; 

    unsigned char *end = NULL;
    unsigned char *pad = NULL;

    #ifdef DUMP

    *logofs << "\n" << logofs_flush;

    *logofs << "Number of STRING8 " << listFontsReply -> number_of_names << ".\n" << logofs_flush;

    *logofs << "Size " << size << ".\n" << logofs_flush;

    DumpHexData(buffer, size);

    *logofs << "\n" << logofs_flush;

    #endif

    length = LISTFONTSREPLY_DATA_OFFSET;

    for (nstringInNames = 0; 
             nstringInNames < listFontsReply -> number_of_names &&
                 listFontsReply -> number_of_names > 0;
                     nstringInNames++)
    {
      //
      // Start with offset LISTFONTSREPLY_DATA_OFFSET 32.
      //

      current = buffer[length];

      length += current + 1;

      #ifdef DUMP
      *logofs << "\nString number : " << nstringInNames << " Current length : "
              << current << "\n" << logofs_flush;
      #endif
    }
    
    #ifdef DUMP
    *logofs << "\nFinal length " << length << "\n" << logofs_flush;
    #endif

    end = ((unsigned char *) buffer) + size;

    for (pad = ((unsigned char *) buffer) + length; pad < end; pad++)
    {
      *pad = 0;

      #ifdef DUMP
      *logofs << "\nPadding ." << "\n" << logofs_flush;
      #endif
    }
  }

  #ifdef DEBUG
  *logofs << name() << ": Parsed identity for message at " << message << ".\n" << logofs_flush;
  #endif

  return 1;
}

int ListFontsReplyStore::unparseIdentity(const Message *message, unsigned char *buffer,
                                            unsigned int size, int bigEndian) const
{
  ListFontsReplyMessage *listFontsReply = (ListFontsReplyMessage *) message;

  //
  // Fill all the message's fields.
  //

  PutUINT(listFontsReply -> number_of_names, buffer + 8, bigEndian);

  #ifdef DEBUG
  *logofs << name() << ": Unparsed identity for message at "
          << message << ".\n" << logofs_flush;
  #endif

  return 1;
}

void ListFontsReplyStore::dumpIdentity(const Message *message) const
{
  #ifdef DUMP

  ListFontsReplyMessage *listFontsReply = (ListFontsReplyMessage *) message;

  *logofs << name() << ": Identity number_of_names "
          << listFontsReply -> number_of_names << ", size "
          << listFontsReply -> size_ << ".\n";

  #endif
}

void ListFontsReplyStore::identityChecksum(const Message *message, const unsigned char *buffer,
                                               unsigned int size, int bigEndian) const
{
  //
  // Field number_of_names.
  //

  md5_append(md5_state_, buffer + 8,  2);
}
