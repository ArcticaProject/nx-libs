/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2011 NoMachine, http://www.nomachine.com/.         */
/*                                                                        */
/* NXAGENT, NX protocol compression and NX extensions to this software    */
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

/*

Copyright 1993 by Davor Matic

Permission to use, copy, modify, distribute, and sell this software
and its documentation for any purpose is hereby granted without fee,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation.  Davor Matic makes no representations about
the suitability of this software for any purpose.  It is provided "as
is" without express or implied warranty.

*/

#ifndef __GCOps_H__
#define __GCOps_H__

/*
 * Graphic operations.
 */

void nxagentFillSpans(DrawablePtr pDrawable, GCPtr pGC, int nSpans,
                          xPoint *pPoints, int *pWidths, int fSorted);

void nxagentSetSpans(DrawablePtr pDrawable, GCPtr pGC, char *pSrc,
                         xPoint *pPoints, int *pWidths, int nSpans, int fSorted);

void nxagentGetSpans(DrawablePtr pDrawable, int maxWidth, xPoint *pPoints,
                         int *pWidths, int nSpans, char *pBuffer);

RegionPtr nxagentCopyArea(DrawablePtr pSrcDrawable, DrawablePtr pDstDrawable,
                              GCPtr pGC, int srcx, int srcy, int width,
                                  int height, int dstx, int dsty);

RegionPtr nxagentCopyPlane(DrawablePtr pSrcDrawable, DrawablePtr pDstDrawable,
                               GCPtr pGC, int srcx, int srcy, int width, int height,
                                   int dstx, int dsty, unsigned long plane);

void nxagentQueryBestSize(int class, unsigned short *pwidth,
                              unsigned short *pheight, ScreenPtr pScreen);

void nxagentPolyPoint(DrawablePtr pDrawable, GCPtr pGC, int mode,
                          int nPoints, xPoint *pPoints);

void nxagentPolyLines(DrawablePtr pDrawable, GCPtr pGC, int mode,
                          int nPoints, xPoint *pPoints);

void nxagentPolySegment(DrawablePtr pDrawable, GCPtr pGC,
                            int nSegments, xSegment *pSegments);

void nxagentPolyRectangle(DrawablePtr pDrawable, GCPtr pGC,
                              int nRectangles, xRectangle *pRectangles);

void nxagentPolyArc(DrawablePtr pDrawable, GCPtr pGC,
                        int nArcs, xArc *pArcs);

void nxagentFillPolygon(DrawablePtr pDrawable, GCPtr pGC, int shape,
                            int mode, int nPoints, xPoint *pPoints);

void nxagentPolyFillRect(DrawablePtr pDrawable, GCPtr pGC,
                             int nRectangles, xRectangle *pRectangles);

void nxagentPolyFillArc(DrawablePtr pDrawable, GCPtr pGC,
                            int nArcs, xArc *pArcs);

int nxagentPolyText8(DrawablePtr pDrawable, GCPtr pGC, int x,
                         int y, int count, char *string);

int nxagentPolyText16(DrawablePtr pDrawable, GCPtr pGC, int x,
                          int y, int count, unsigned short *string);

void nxagentImageText8(DrawablePtr pDrawable, GCPtr pGC, int x,
                           int y, int count, char *string);

void nxagentImageText16(DrawablePtr pDrawable, GCPtr pGC, int x,
                            int y, int count, unsigned short *string);

void nxagentImageGlyphBlt(DrawablePtr pDrawable, GCPtr pGC, int x, int y,
                              unsigned int nGlyphs, CharInfoPtr *pCharInfo, pointer pGlyphBase);

void nxagentPolyGlyphBlt(DrawablePtr pDrawable, GCPtr pGC, int x, int y,
                             unsigned int nGlyphs, CharInfoPtr *pCharInfo, pointer pGlyphBase);

void nxagentPushPixels(GCPtr pGC, PixmapPtr pBitmap, DrawablePtr pDrawable,
                           int width, int height, int x, int y);

#endif /* __GCOps_H__ */

