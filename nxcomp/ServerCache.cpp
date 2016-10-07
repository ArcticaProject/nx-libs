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

#include "ServerCache.h"

//
// Some global caches used to store information
// common to all X connections.
//

BlockCache ServerCache::lastInitReply;
BlockCache ServerCache::lastKeymap;
unsigned char ServerCache::getKeyboardMappingLastKeysymsPerKeycode = 0;
BlockCache ServerCache::getKeyboardMappingLastMap;
BlockCache ServerCache::getModifierMappingLastMap;
BlockCache ServerCache::xResources;
BlockCacheSet ServerCache::queryFontFontCache(16);

ServerCache::ServerCache() :

  replySequenceCache(6), eventSequenceCache(6),
  lastTimestamp(0), visualCache(8), colormapCache(8),

  errorMinorCache(8),

  colormapNotifyWindowCache(8), colormapNotifyColormapCache(8),

  createNotifyWindowCache(8), createNotifyLastWindow(0),

  exposeWindowCache(12),

  focusInWindowCache(8),

  keyPressLastKey(0),

  mapNotifyEventCache(8), mapNotifyWindowCache(8),

  motionNotifyTimestampCache(8), motionNotifyLastRootX(0),
  motionNotifyLastRootY(0), motionNotifyRootXCache(8),
  motionNotifyRootYCache(8), motionNotifyEventXCache(8),
  motionNotifyEventYCache(8), motionNotifyStateCache(8),

  noExposeDrawableCache(8), noExposeMinorCache(8),

  propertyNotifyWindowCache(8), propertyNotifyAtomCache(8),

  reparentNotifyWindowCache(8),

  selectionClearWindowCache(8), selectionClearAtomCache(8),

  visibilityNotifyWindowCache(8),

  getGeometryRootCache(8),

  getInputFocusWindowCache(8),

  getKeyboardMappingKeysymCache(8),

  getPropertyTypeCache(8),

  getSelectionOwnerCache(8),

  getWindowAttributesClassCache(8), getWindowAttributesPlanesCache(8),
  getWindowAttributesPixelCache(8), getWindowAttributesAllEventsCache(8),
  getWindowAttributesYourEventsCache(8),
  getWindowAttributesDontPropagateCache(8),

  queryPointerRootCache(8), queryPointerChildCache(8),

  translateCoordsChildCache(8), translateCoordsXCache(8),
  translateCoordsYCache(8),

  queryTreeWindowCache(8)

{
  unsigned int i;

  for (i = 0; i < 3; i++)
  {
    configureNotifyWindowCache[i] = new IntCache(8);
  }

  for (i = 0; i < 5; i++)
  {
    configureNotifyGeomCache[i] = new IntCache(8);
  }

  for (i = 0; i < 5; i++)
  {
    exposeGeomCache[i] = new IntCache(8);
  }

  for (i = 0; i < 3; i++)
  {
    motionNotifyWindowCache[i] = new IntCache(8);
  }

  for (i = 0; i < 5; i++)
  {
    getGeometryGeomCache[i] = new IntCache(8);
  }

  for (i = 0; i < 23; i++)
  {
    keyPressCache[i] = 0;
  }

  for (i = 0; i < 6; i++)
  {
    queryFontCharInfoCache[i] = new IntCache(8);
    queryFontLastCharInfo[i] = 0;
  }

  for (i = 0; i < 12; i++)
  {
    genericReplyIntCache[i] = new IntCache(8);
  }

  for (i = 0; i < 14; i++)
  {
    genericEventIntCache[i] = new IntCache(8);
  }
}


ServerCache::~ServerCache()
{
  unsigned int i;

  for (i = 0; i < 3; i++)
  {
    delete configureNotifyWindowCache[i];
  }

  for (i = 0; i < 5; i++)
  {
    delete configureNotifyGeomCache[i];
  }

  for (i = 0; i < 5; i++)
  {
    delete exposeGeomCache[i];
  }

  for (i = 0; i < 3; i++)
  {
    delete motionNotifyWindowCache[i];
  }

  for (i = 0; i < 5; i++)
  {
    delete getGeometryGeomCache[i];
  }

  for (i = 0; i < 6; i++)
  {
    delete queryFontCharInfoCache[i];
  }

  for (i = 0; i < 12; i++)
  {
    delete genericReplyIntCache[i];
  }

  for (i = 0; i < 14; i++)
  {
    delete genericEventIntCache[i];
  }
}
