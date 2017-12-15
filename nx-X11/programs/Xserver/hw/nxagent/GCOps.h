/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2011 NoMachine (http://www.nomachine.com)          */
/* Copyright (c) 2008-2014 Oleksandr Shneyder <o.shneyder@phoca-gmbh.de>  */
/* Copyright (c) 2011-2016 Mike Gabriel <mike.gabriel@das-netzwerkteam.de>*/
/* Copyright (c) 2014-2016 Mihai Moldovan <ionic@ionic.de>                */
/* Copyright (c) 2014-2016 Ulrich Sibiller <uli42@gmx.de>                 */
/* Copyright (c) 2015-2016 Qindel Group (http://www.qindel.com)           */
/*                                                                        */
/* NXAGENT, NX protocol compression and NX extensions to this software    */
/* are copyright of the aforementioned persons and companies.             */
/*                                                                        */
/* Redistribution and use of the present software is allowed according    */
/* to terms specified in the file LICENSE which comes in the source       */
/* distribution.                                                          */
/*                                                                        */
/* All rights reserved.                                                   */
/*                                                                        */
/* NOTE: This software has received contributions from various other      */
/* contributors, only the core maintainers and supporters are listed as   */
/* copyright holders. Please contact us, if you feel you should be listed */
/* as copyright holder, as well.                                          */
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
                              unsigned int nGlyphs, CharInfoPtr *pCharInfo, void * pGlyphBase);

void nxagentPolyGlyphBlt(DrawablePtr pDrawable, GCPtr pGC, int x, int y,
                             unsigned int nGlyphs, CharInfoPtr *pCharInfo, void * pGlyphBase);

void nxagentPushPixels(GCPtr pGC, PixmapPtr pBitmap, DrawablePtr pDrawable,
                           int width, int height, int x, int y);

#endif /* __GCOps_H__ */

