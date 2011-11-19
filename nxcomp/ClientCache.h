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

#ifndef ClientCache_H
#define ClientCache_H

#include "Misc.h"

#include "IntCache.h"
#include "CharCache.h"
#include "OpcodeCache.h"
#include "XidCache.h"
#include "FreeCache.h"

#include "TextCompressor.h"

#include "ChannelCache.h"

class ClientCache : public ChannelCache
{
  public:

  ClientCache();

  ~ClientCache();

  //
  // Opcode prediction caches.
  //

  OpcodeCache opcodeCache;

  //
  // GC and drawables caches.
  //

  XidCache gcCache;
  FreeCache freeGCCache;

  XidCache drawableCache;
  FreeCache freeDrawableCache;

  XidCache windowCache;
  FreeCache freeWindowCache;

  //
  // General-purpose caches.
  //

  CharCache textCache[CLIENT_TEXT_CACHE_SIZE];
  IntCache cursorCache;
  IntCache colormapCache;
  IntCache visualCache;
  CharCache depthCache;
  CharCache resourceCache;
  CharCache methodCache;

  unsigned int lastFont;

  //
  // AllocColor request.
  //

  IntCache *allocColorRGBCache[3];

  //
  // ChangeProperty request.
  //

  CharCache changePropertyFormatCache;
  IntCache changePropertyPropertyCache;
  IntCache changePropertyTypeCache;
  IntCache changePropertyData32Cache;
  TextCompressor changePropertyTextCompressor;

  //
  // ClearArea request.
  //

  IntCache *clearAreaGeomCache[4];

  //
  // ConfigureWindow request.
  //

  IntCache configureWindowBitmaskCache;
  IntCache *configureWindowAttrCache[7];

  //
  // ConvertSelection request.
  //

  IntCache convertSelectionRequestorCache;
  IntCache* convertSelectionAtomCache[3];
  unsigned int convertSelectionLastTimestamp;

  //
  // CopyArea request.
  //

  IntCache *copyAreaGeomCache[6];

  //
  // CopyPlane request.
  //

  IntCache *copyPlaneGeomCache[6];
  IntCache copyPlaneBitPlaneCache;

  //
  // CreateGC request.
  //

  IntCache createGCBitmaskCache;
  IntCache *createGCAttrCache[23];

  //
  // CreatePixmap request.
  //

  IntCache createPixmapIdCache;
  unsigned int createPixmapLastId;
  IntCache createPixmapXCache;
  IntCache createPixmapYCache;

  //
  // CreateWindow request.
  //

  IntCache *createWindowGeomCache[6];
  IntCache createWindowBitmaskCache;
  IntCache *createWindowAttrCache[15];

  //
  // FillPoly request.
  //

  IntCache fillPolyNumPointsCache;
  IntCache *fillPolyXRelCache[10];
  IntCache *fillPolyXAbsCache[10];
  IntCache *fillPolyYRelCache[10];
  IntCache *fillPolyYAbsCache[10];
  unsigned int fillPolyRecentX[8];
  unsigned int fillPolyRecentY[8];
  unsigned int fillPolyIndex;

  //
  // GetSelectionOwner request.
  //

  IntCache getSelectionOwnerSelectionCache;

  //
  // GrabButton request (also used for GrabPointer).
  //

  IntCache grabButtonEventMaskCache;
  IntCache grabButtonConfineCache;
  CharCache grabButtonButtonCache;
  IntCache grabButtonModifierCache;

  //
  // GrabKeyboard request.
  //

  unsigned int grabKeyboardLastTimestamp;

  //
  // ImageText8/16 request.
  //

  IntCache imageTextLengthCache;
  unsigned int imageTextLastX;
  unsigned int imageTextLastY;
  IntCache imageTextCacheX;
  IntCache imageTextCacheY;
  TextCompressor imageTextTextCompressor;

  //
  // InternAtom request.
  //

  TextCompressor internAtomTextCompressor;

  //
  // OpenFont request.
  //

  TextCompressor openFontTextCompressor;

  //
  // PolyFillRectangle request.
  //

  IntCache *polyFillRectangleCacheX[4];
  IntCache *polyFillRectangleCacheY[4];
  IntCache *polyFillRectangleCacheWidth[4];
  IntCache *polyFillRectangleCacheHeight[4];

  //
  // PolyLine request.
  //

  IntCache *polyLineCacheX[2];
  IntCache *polyLineCacheY[2];

  //
  // PolyPoint request.
  //

  IntCache *polyPointCacheX[2];
  IntCache *polyPointCacheY[2];

  //
  // PolyRectangle request.
  //

  IntCache *polyRectangleGeomCache[4];

  //
  // PolySegment request.
  //

  IntCache polySegmentCacheX;
  IntCache polySegmentCacheY;
  unsigned int polySegmentLastX[2];
  unsigned int polySegmentLastY[2];
  unsigned int polySegmentCacheIndex;

  //
  // PolyText8/16 request.
  //

  unsigned int polyTextLastX;
  unsigned int polyTextLastY;
  IntCache polyTextCacheX;
  IntCache polyTextCacheY;
  IntCache polyTextFontCache;
  CharCache polyTextDeltaCache;
  TextCompressor polyTextTextCompressor;

  //
  // PutImage request.
  //

  IntCache putImageWidthCache;
  IntCache putImageHeightCache;
  unsigned int putImageLastX;
  unsigned int putImageLastY;
  IntCache putImageXCache;
  IntCache putImageYCache;
  CharCache putImageLeftPadCache;

  //
  // GetImage request.
  //

  IntCache getImagePlaneMaskCache;

  //
  // QueryColors request.
  //

  unsigned int queryColorsLastPixel;

  //
  // SetClipRectangles request.
  //

  IntCache setClipRectanglesXCache;
  IntCache setClipRectanglesYCache;
  IntCache *setClipRectanglesGeomCache[4];

  //
  // SetDashes request.
  //

  IntCache setDashesLengthCache;
  IntCache setDashesOffsetCache;
  CharCache setDashesDashCache_[2];

  //
  // SetSelectionOwner request.
  //

  IntCache setSelectionOwnerCache;
  IntCache setSelectionOwnerTimestampCache;

  //
  // TranslateCoords request.
  //

  IntCache translateCoordsSrcCache;
  IntCache translateCoordsDstCache;
  IntCache translateCoordsXCache;
  IntCache translateCoordsYCache;

  //
  // SendEvent request.
  //

  IntCache     sendEventMaskCache;
  CharCache    sendEventCodeCache;
  CharCache    sendEventByteDataCache;
  unsigned int sendEventLastSequence;
  IntCache     sendEventIntDataCache;
  CharCache    sendEventEventCache;

  //
  // PolyFillArc request.
  //

  IntCache *polyFillArcCacheX[2];
  IntCache *polyFillArcCacheY[2];
  IntCache *polyFillArcCacheWidth[2];
  IntCache *polyFillArcCacheHeight[2];
  IntCache *polyFillArcCacheAngle1[2];
  IntCache *polyFillArcCacheAngle2[2];

  //
  // PolyArc request.
  //

  IntCache *polyArcCacheX[2];
  IntCache *polyArcCacheY[2];
  IntCache *polyArcCacheWidth[2];
  IntCache *polyArcCacheHeight[2];
  IntCache *polyArcCacheAngle1[2];
  IntCache *polyArcCacheAngle2[2];

  //
  // PutPackedImage request.
  //

  IntCache  putPackedImageSrcLengthCache;
  IntCache  putPackedImageDstLengthCache;

  //
  // Shape extension requests.
  //

  CharCache shapeOpcodeCache;
  IntCache  *shapeDataCache[8];

  //
  // Generic requests.
  //

  CharCache genericRequestOpcodeCache;
  IntCache  *genericRequestDataCache[8];

  //
  // Render extension requests.
  //

  OpcodeCache renderOpcodeCache;

  CharCache renderOpCache;

  XidCache  renderSrcPictureCache;
  XidCache  renderMaskPictureCache;
  XidCache  renderDstPictureCache;
  FreeCache renderFreePictureCache;

  IntCache  renderGlyphSetCache;
  FreeCache renderFreeGlyphSetCache;

  IntCache renderIdCache;
  IntCache renderLengthCache;
  IntCache renderFormatCache;
  IntCache renderValueMaskCache;
  IntCache renderNumGlyphsCache;

  IntCache renderXCache;
  IntCache renderYCache;

  unsigned int renderLastX;
  unsigned int renderLastY;

  IntCache renderWidthCache;
  IntCache renderHeightCache;

  unsigned int renderLastId;

  IntCache *renderDataCache[16];

  TextCompressor renderTextCompressor;

  IntCache renderGlyphXCache;
  IntCache renderGlyphYCache;

  unsigned int renderGlyphX;
  unsigned int renderGlyphY;

  IntCache *renderCompositeGlyphsDataCache[16];
  unsigned int renderLastCompositeGlyphsData;

  IntCache *renderCompositeDataCache[3];

  //
  // SetCacheParameters request.
  //

  IntCache setCacheParametersCache;

  //
  // Encode new XID values based
  // on the last value encoded.
  //

  IntCache lastIdCache;
  unsigned int lastId;
};

#endif /* ClientCache_H */
