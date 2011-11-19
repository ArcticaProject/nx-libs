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

#ifndef ChannelCache_H
#define ChannelCache_H

//
// Elements in array of caches used in TextCompressor.
//

const unsigned int CLIENT_TEXT_CACHE_SIZE = 9999;
const unsigned int SERVER_TEXT_CACHE_SIZE = 9999;

//
// Sizes of optional fields for ConfigureWindow
// request.
//

extern const unsigned int CONFIGUREWINDOW_FIELD_WIDTH[7];

//
// Sizes of optional fields for CreateGC request.
//

extern const unsigned int CREATEGC_FIELD_WIDTH[23];

//
// This is just needed to provide a pointer
// to the base cache class in encoding and
// decoding procedures of message stores.
//

class ChannelCache
{
  public:

  ChannelCache()
  {
  }

  ~ChannelCache()
  {
  }
};

#endif /* ChannelCache_H */
