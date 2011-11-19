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

#ifndef OpcodeStore_H
#define OpcodeStore_H

#include "NXproto.h"

class OpcodeStore
{
  public:

  OpcodeStore();

  ~OpcodeStore();

  //
  // Map NX protocol messages. At the moment mapping is hard-
  // coded. Opcodes should be instead agreed with the proxied
  // X server (by excluding all opcodes used for extensions)
  // and exported by the proxy class to channels.
  //
  // Some toolkits query the server only once for extensions'
  // opcodes and share the same settings across all channels.
  // This could be a problem as channels needed to monitor the
  // traffic to find out the extensions' opcodes themselves,
  // so it is important that proxy passes an instance of this
  // class to new channels.
  //

  unsigned char getControlParameters;
  unsigned char getCleanupParameters;
  unsigned char getImageParameters;
  unsigned char getUnpackParameters;
  unsigned char getShmemParameters;
  unsigned char getFontParameters;

  unsigned char startSplit;
  unsigned char endSplit;
  unsigned char commitSplit;
  unsigned char finishSplit;
  unsigned char abortSplit;

  unsigned char splitData;
  unsigned char splitEvent;

  unsigned char setCacheParameters;
  unsigned char setExposeParameters;

  unsigned char setUnpackGeometry;
  unsigned char setUnpackColormap;
  unsigned char setUnpackAlpha;

  unsigned char putPackedImage;

  unsigned char freeUnpack;
  unsigned char freeSplit;

  unsigned char shapeExtension;
  unsigned char renderExtension;

  unsigned char noSplitNotify;
  unsigned char startSplitNotify;
  unsigned char commitSplitNotify;
  unsigned char endSplitNotify;
  unsigned char emptySplitNotify;
};

#endif /* OpcodeStore_H */
