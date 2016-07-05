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
