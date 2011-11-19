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

#include "ClientCache.h"

ClientCache::ClientCache() :

  freeGCCache(16), freeDrawableCache(16), freeWindowCache(16),

  cursorCache(16), colormapCache(16), visualCache(16), lastFont(0),

  changePropertyPropertyCache(16), changePropertyTypeCache(16),
  changePropertyData32Cache(16),
  changePropertyTextCompressor(textCache, CLIENT_TEXT_CACHE_SIZE),

  configureWindowBitmaskCache(4),

  convertSelectionRequestorCache(16),
  convertSelectionLastTimestamp(0),

  copyPlaneBitPlaneCache(8),

  createGCBitmaskCache(8),

  createPixmapIdCache(16), createPixmapLastId(0),
  createPixmapXCache(8), createPixmapYCache(8),

  createWindowBitmaskCache(8),

  fillPolyNumPointsCache(8), fillPolyIndex(0),

  getSelectionOwnerSelectionCache(8),

  grabButtonEventMaskCache(8), grabButtonConfineCache(8),
  grabButtonModifierCache(8),

  grabKeyboardLastTimestamp(0),

  imageTextLengthCache(8),
  imageTextLastX(0), imageTextLastY(0),
  imageTextCacheX(8), imageTextCacheY(8),
  imageTextTextCompressor(textCache, CLIENT_TEXT_CACHE_SIZE),

  internAtomTextCompressor(textCache, CLIENT_TEXT_CACHE_SIZE),

  openFontTextCompressor(textCache, CLIENT_TEXT_CACHE_SIZE),

  polySegmentCacheX(8), polySegmentCacheY(8), polySegmentCacheIndex(0),

  polyTextLastX(0), polyTextLastY(0), polyTextCacheX(8),
  polyTextCacheY(8), polyTextFontCache(8),
  polyTextTextCompressor(textCache, CLIENT_TEXT_CACHE_SIZE),

  putImageWidthCache(8), putImageHeightCache(8), putImageLastX(0),
  putImageLastY(0), putImageXCache(8), putImageYCache(8),

  getImagePlaneMaskCache(8),

  queryColorsLastPixel(0),

  setClipRectanglesXCache(8), setClipRectanglesYCache(8),

  setDashesLengthCache(8), setDashesOffsetCache(8),

  setSelectionOwnerCache(8), setSelectionOwnerTimestampCache(8),

  translateCoordsSrcCache(8), translateCoordsDstCache(8),
  translateCoordsXCache(8), translateCoordsYCache(8),

  sendEventMaskCache(16), sendEventLastSequence(0),
  sendEventIntDataCache(16),

  putPackedImageSrcLengthCache(16), putPackedImageDstLengthCache(16),

  //
  // RenderExtension requests.
  //

  renderFreePictureCache(16),

  renderGlyphSetCache(16),
  renderFreeGlyphSetCache(16),

  renderIdCache(8),

  renderLengthCache(16), renderFormatCache(16),
  renderValueMaskCache(8), renderNumGlyphsCache(8),

  renderXCache(16), renderYCache(16),
  renderLastX(0), renderLastY(0),

  renderWidthCache(16), renderHeightCache(16),

  renderLastId(0),

  renderTextCompressor(textCache, CLIENT_TEXT_CACHE_SIZE),

  renderGlyphXCache(16), renderGlyphYCache(16),
  renderGlyphX(0), renderGlyphY(0),

  renderLastCompositeGlyphsData(0),

  setCacheParametersCache(8),

  lastIdCache(16), lastId(0)

{
  unsigned int i;

  for (i = 0; i < 3; i++)
  {
    allocColorRGBCache[i] = new IntCache(8);

    convertSelectionAtomCache[i] = new IntCache(8);
  }

  for (i = 0; i < 4; i++)
  {
    clearAreaGeomCache[i] = new IntCache(8);
  }

  for (i = 0; i < 7; i++)
  {
    configureWindowAttrCache[i] = new IntCache(8);
  }

  for (i = 0; i < 6; i++)
  {
    copyAreaGeomCache[i] = new IntCache(8);
    copyPlaneGeomCache[i] = new IntCache(8);
  }

  for (i = 0; i < 23; i++)
  {
    if (CREATEGC_FIELD_WIDTH[i] > 16)
    {
      createGCAttrCache[i] = new IntCache(16);
    }
    else
    {
      createGCAttrCache[i] = new IntCache(CREATEGC_FIELD_WIDTH[i]);
    }
  }

  for (i = 0; i < 6; i++)
  {
    createWindowGeomCache[i] = new IntCache(8);
  }

  for (i = 0; i < 15; i++)
  {
    createWindowAttrCache[i] = new IntCache(8);
  }

  for (i = 0; i < 10; i++)
  {
    fillPolyXRelCache[i] = new IntCache(8);
    fillPolyXAbsCache[i] = new IntCache(8);
    fillPolyYRelCache[i] = new IntCache(8);
    fillPolyYAbsCache[i] = new IntCache(8);
  }

  for (i = 0; i < 8; i++)
  {
    fillPolyRecentX[i] = 0;
    fillPolyRecentY[i] = 0;
  }

  for (i = 0; i < 4; i++)
  {
    polyFillRectangleCacheX[i] = new IntCache(8);
    polyFillRectangleCacheY[i] = new IntCache(8);
    polyFillRectangleCacheWidth[i] = new IntCache(8);
    polyFillRectangleCacheHeight[i] = new IntCache(8);
  }

  for (i = 0; i < 2; i++)
  {
    polyLineCacheX[i] = new IntCache(8);
    polyLineCacheY[i] = new IntCache(8);
  }

  for (i = 0; i < 2; i++)
  {
    polyPointCacheX[i] = new IntCache(8);
    polyPointCacheY[i] = new IntCache(8);
  }

  for (i = 0; i < 4; i++)
  {
    polyRectangleGeomCache[i] = new IntCache(8);
  }

  for (i = 0; i < 2; i++)
  {
    polySegmentLastX[i] = 0;
    polySegmentLastY[i] = 0;
  }

  for (i = 0; i < 4; i++)
  {
    setClipRectanglesGeomCache[i] = new IntCache(8);
  }

  for (i = 0; i < 2; i++)
  {
    polyFillArcCacheX[i] = new IntCache(8);
    polyFillArcCacheY[i] = new IntCache(8);
    polyFillArcCacheWidth[i] = new IntCache(8);
    polyFillArcCacheHeight[i] = new IntCache(8);
    polyFillArcCacheAngle1[i] = new IntCache(8);
    polyFillArcCacheAngle2[i] = new IntCache(8);
  }

  for (i = 0; i < 2; i++)
  {
    polyArcCacheX[i] = new IntCache(8);
    polyArcCacheY[i] = new IntCache(8);
    polyArcCacheWidth[i] = new IntCache(8);
    polyArcCacheHeight[i] = new IntCache(8);
    polyArcCacheAngle1[i] = new IntCache(8);
    polyArcCacheAngle2[i] = new IntCache(8);
  }

  for (i = 0; i < 8; i++)
  {
    shapeDataCache[i] = new IntCache(8);
  }

  for (i = 0; i < 8; i++)
  {
    genericRequestDataCache[i] = new IntCache(8);
  }

  for (i = 0; i < 16; i++)
  {
    renderDataCache[i] = new IntCache(16);
  }

  for (i = 0; i < 16; i++)
  {
    renderCompositeGlyphsDataCache[i] = new IntCache(16);
  }

  for (i = 0; i < 3; i++)
  {
    renderCompositeDataCache[i] = new IntCache(16);
  }
}


ClientCache::~ClientCache()
{
  unsigned int i;

  for (i = 0; i < 3; i++)
  {
    delete allocColorRGBCache[i];
    delete convertSelectionAtomCache[i];
  }

  for (i = 0; i < 4; i++)
  {
    delete clearAreaGeomCache[i];
  }

  for (i = 0; i < 7; i++)
  {
    delete configureWindowAttrCache[i];
  }

  for (i = 0; i < 6; i++)
  {
    delete copyAreaGeomCache[i];
    delete copyPlaneGeomCache[i];
  }

  for (i = 0; i < 23; i++)
  {
    delete createGCAttrCache[i];
  }

  for (i = 0; i < 6; i++)
  {
    delete createWindowGeomCache[i];
  }

  for (i = 0; i < 15; i++)
  {
    delete createWindowAttrCache[i];
  }

  for (i = 0; i < 10; i++)
  {
    delete fillPolyXRelCache[i];
    delete fillPolyXAbsCache[i];
    delete fillPolyYRelCache[i];
    delete fillPolyYAbsCache[i];
  }

  for (i = 0; i < 4; i++)
  {
    delete polyFillRectangleCacheX[i];
    delete polyFillRectangleCacheY[i];
    delete polyFillRectangleCacheWidth[i];
    delete polyFillRectangleCacheHeight[i];
  }

  for (i = 0; i < 2; i++)
  {
    delete polyLineCacheX[i];
    delete polyLineCacheY[i];
  }

  for (i = 0; i < 2; i++)
  {
    delete polyPointCacheX[i];
    delete polyPointCacheY[i];
  }

  for (i = 0; i < 4; i++)
  {
    delete polyRectangleGeomCache[i];
  }

  for (i = 0; i < 4; i++)
  {
    delete setClipRectanglesGeomCache[i];
  }

  for (i = 0; i < 2; i++)
  {
    delete polyFillArcCacheX[i];
    delete polyFillArcCacheY[i];
    delete polyFillArcCacheWidth[i];
    delete polyFillArcCacheHeight[i];
    delete polyFillArcCacheAngle1[i];
    delete polyFillArcCacheAngle2[i];
  }

  for (i = 0; i < 2; i++)
  {
    delete polyArcCacheX[i];
    delete polyArcCacheY[i];
    delete polyArcCacheWidth[i];
    delete polyArcCacheHeight[i];
    delete polyArcCacheAngle1[i];
    delete polyArcCacheAngle2[i];
  }

  for (i = 0; i < 8; i++)
  {
    delete shapeDataCache[i];
  }

  for (i = 0; i < 8; i++)
  {
    delete genericRequestDataCache[i];
  }

  for (i = 0; i < 16; i++)
  {
    delete renderDataCache[i];
  }

  for (i = 0; i < 16; i++)
  {
    delete renderCompositeGlyphsDataCache[i];
  }

  for (i = 0; i < 3; i++)
  {
    delete renderCompositeDataCache[i];
  }
}
