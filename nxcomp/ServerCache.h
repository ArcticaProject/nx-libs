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

#ifndef ServerCache_H
#define ServerCache_H

#include "Misc.h"

#include "IntCache.h"
#include "CharCache.h"
#include "OpcodeCache.h"
#include "TextCompressor.h"
#include "BlockCache.h"
#include "BlockCacheSet.h"

#include "ChannelCache.h"

class ServerCache : public ChannelCache
{
  public:

  ServerCache();

  ~ServerCache();

  //
  // Opcode prediction caches.
  //

  OpcodeCache opcodeCache;

  //
  // General-purpose caches.
  //

  CharCache textCache[SERVER_TEXT_CACHE_SIZE];
  IntCache replySequenceCache;
  IntCache eventSequenceCache;
  unsigned int lastTimestamp;
  CharCache depthCache;
  IntCache visualCache;
  IntCache colormapCache;
  CharCache resourceCache;

  //
  // X connection startup.
  //

  static BlockCache lastInitReply;

  //
  // X errors.
  //

  CharCache errorCodeCache;
  IntCache errorMinorCache;
  CharCache errorMajorCache;

  //
  // ButtonPress and ButtonRelease events.
  //

  CharCache buttonCache;

  //
  // ColormapNotify event.
  //

  IntCache colormapNotifyWindowCache;
  IntCache colormapNotifyColormapCache;

  //
  // ConfigureNotify event.
  //

  IntCache *configureNotifyWindowCache[3];
  IntCache *configureNotifyGeomCache[5];

  //
  // CreateNotify event.
  //

  IntCache createNotifyWindowCache;
  unsigned int createNotifyLastWindow;

  //
  // Expose event.
  //

  IntCache exposeWindowCache;
  IntCache *exposeGeomCache[5];

  //
  // FocusIn event (also used for FocusOut).
  //

  IntCache focusInWindowCache;

  //
  // KeymapNotify event.
  //

  static BlockCache lastKeymap;

  //
  // KeyPress event.
  //

  unsigned char keyPressLastKey;
  unsigned char keyPressCache[23];

  //
  // MapNotify event (also used for UnmapNotify).
  //

  IntCache mapNotifyEventCache;
  IntCache mapNotifyWindowCache;

  //
  // MotionNotify event (also used for KeyPress, 
  // KeyRelease, ButtonPress, ButtonRelease,
  // EnterNotify, and LeaveNotify events and
  // QueryPointer reply).
  //

  IntCache motionNotifyTimestampCache;
  unsigned int motionNotifyLastRootX;
  unsigned int motionNotifyLastRootY;
  IntCache motionNotifyRootXCache;
  IntCache motionNotifyRootYCache;
  IntCache motionNotifyEventXCache;
  IntCache motionNotifyEventYCache;
  IntCache motionNotifyStateCache;
  IntCache *motionNotifyWindowCache[3];

  //
  // NoExpose event.
  //

  IntCache noExposeDrawableCache;
  IntCache noExposeMinorCache;
  CharCache noExposeMajorCache;

  //
  // PropertyNotify event.
  //

  IntCache propertyNotifyWindowCache;
  IntCache propertyNotifyAtomCache;

  //
  // ReparentNotify event.
  //

  IntCache reparentNotifyWindowCache;

  //
  // SelectionClear event.
  //

  IntCache selectionClearWindowCache;
  IntCache selectionClearAtomCache;

  //
  // VisibilityNotify event.
  //

  IntCache visibilityNotifyWindowCache;

  //
  // GetGeometry reply.
  //

  IntCache getGeometryRootCache;
  IntCache *getGeometryGeomCache[5];

  //
  // GetInputFocus reply.
  //

  IntCache getInputFocusWindowCache;

  //
  // GetKeyboardMapping reply.
  //

  static unsigned char getKeyboardMappingLastKeysymsPerKeycode;
  static BlockCache getKeyboardMappingLastMap;
  IntCache getKeyboardMappingKeysymCache;
  CharCache getKeyboardMappingLastByteCache;

  //
  // GetModifierMapping reply.
  //

  static BlockCache getModifierMappingLastMap;

  //
  // GetProperty reply.
  //

  CharCache getPropertyFormatCache;
  IntCache getPropertyTypeCache;
  TextCompressor getPropertyTextCompressor;
  static BlockCache xResources;

  //
  // GetSelection reply.
  //

  IntCache getSelectionOwnerCache;

  //
  // GetWindowAttributes reply.
  //

  IntCache getWindowAttributesClassCache;
  CharCache getWindowAttributesBitGravityCache;
  CharCache getWindowAttributesWinGravityCache;
  IntCache getWindowAttributesPlanesCache;
  IntCache getWindowAttributesPixelCache;
  IntCache getWindowAttributesAllEventsCache;
  IntCache getWindowAttributesYourEventsCache;
  IntCache getWindowAttributesDontPropagateCache;

  //
  // QueryColors reply.
  //

  BlockCache queryColorsLastReply;

  //
  // QueryFont reply.
  //

  static BlockCacheSet queryFontFontCache;
  IntCache *queryFontCharInfoCache[6];
  unsigned int queryFontLastCharInfo[6];

  //
  // QueryPointer reply.
  //

  IntCache queryPointerRootCache;
  IntCache queryPointerChildCache;

  //
  // TranslateCoords reply.
  //

  IntCache translateCoordsChildCache;
  IntCache translateCoordsXCache;
  IntCache translateCoordsYCache;

  //
  // QueryTree reply.
  //

  IntCache queryTreeWindowCache;

  //
  // GetAtomName reply in protocol
  // versions >= 3.
  //

  TextCompressor getAtomNameTextCompressor;

  //
  // Generic reply. Use short data
  // in protocol versions >= 3.
  //

  CharCache genericReplyCharCache;
  IntCache  *genericReplyIntCache[12];

  //
  // Generic event. Only in protocol
  // versions >= 3.
  //

  CharCache genericEventCharCache;
  IntCache  *genericEventIntCache[14];

  //
  // Used in the abort split events.
  //

  OpcodeCache abortOpcodeCache;
};

#endif /* ServerCache_H */
