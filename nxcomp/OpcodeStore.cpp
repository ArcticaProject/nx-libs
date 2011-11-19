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

#include "OpcodeStore.h"

OpcodeStore::OpcodeStore()
{
  //
  // Assign values of the specific
  // NX opcodes.
  //

  getControlParameters = X_NXGetControlParameters;
  getCleanupParameters = X_NXGetCleanupParameters;
  getImageParameters   = X_NXGetImageParameters;
  getUnpackParameters  = X_NXGetUnpackParameters;
  getShmemParameters   = X_NXGetShmemParameters;
  getFontParameters    = X_NXGetFontParameters;

  startSplit  = X_NXStartSplit;
  endSplit    = X_NXEndSplit;
  commitSplit = X_NXCommitSplit;
  finishSplit = X_NXFinishSplit;
  abortSplit  = X_NXAbortSplit;

  splitData  = X_NXSplitData;
  splitEvent = X_NXSplitEvent;

  setCacheParameters  = X_NXSetCacheParameters;
  setExposeParameters = X_NXSetExposeParameters;

  setUnpackGeometry = X_NXSetUnpackGeometry;
  setUnpackColormap = X_NXSetUnpackColormap;
  setUnpackAlpha    = X_NXSetUnpackAlpha;

  putPackedImage = X_NXPutPackedImage;

  freeUnpack = X_NXFreeUnpack;
  freeSplit  = X_NXFreeSplit;

  //
  // These values must be fetched
  // from the X server.
  //

  shapeExtension  = 0;
  renderExtension = 0;

  //
  // Events sent as ClientMessage.
  //

  noSplitNotify     = NXNoSplitNotify;
  startSplitNotify  = NXStartSplitNotify;
  commitSplitNotify = NXCommitSplitNotify;
  endSplitNotify    = NXEndSplitNotify;
  emptySplitNotify  = NXEmptySplitNotify;
}

OpcodeStore::~OpcodeStore()
{
}
