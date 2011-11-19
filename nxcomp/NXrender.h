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

#ifndef NXrender_H
#define NXrender_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Import this from <X11/extensions/render.h>
 * to compile under old XFree86 distributions
 * when render extension was not present yet.
 */

#define X_RenderQueryVersion		    0
#define X_RenderQueryPictFormats	    1
#define X_RenderQueryPictIndexValues	    2
#define X_RenderQueryDithers		    3
#define X_RenderCreatePicture		    4
#define X_RenderChangePicture		    5
#define X_RenderSetPictureClipRectangles    6
#define X_RenderFreePicture		    7
#define X_RenderComposite		    8
#define X_RenderScale			    9
#define X_RenderTrapezoids		    10
#define X_RenderTriangles		    11
#define X_RenderTriStrip		    12
#define X_RenderTriFan			    13
#define X_RenderColorTrapezoids		    14
#define X_RenderColorTriangles		    15
#define X_RenderTransform		    16
#define X_RenderCreateGlyphSet		    17
#define X_RenderReferenceGlyphSet	    18
#define X_RenderFreeGlyphSet		    19
#define X_RenderAddGlyphs		    20
#define X_RenderAddGlyphsFromPicture	    21
#define X_RenderFreeGlyphs		    22
#define X_RenderCompositeGlyphs8	    23
#define X_RenderCompositeGlyphs16	    24
#define X_RenderCompositeGlyphs32	    25
#define X_RenderFillRectangles		    26
/* 0.5 */
#define X_RenderCreateCursor		    27
/* 0.6 */
#define X_RenderSetPictureTransform	    28
#define X_RenderQueryFilters		    29
#define X_RenderSetPictureFilter	    30
#define X_RenderCreateAnimCursor	    31

#ifdef __cplusplus
}
#endif

#endif /* NXrender_H */
