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

#ifndef __Image_H__
#define __Image_H__

/*
 * Graphic operations.
 */

void nxagentPutImage(DrawablePtr pDrawable, GCPtr pGC, int depth,
                         int dstX, int dstY, int dstWidth, int dstHeight,
                             int leftPad, int format, char *data);

void nxagentRealizeImage(DrawablePtr pDrawable, GCPtr pGC, int depth,
                             int x, int y, int w, int h, int leftPad,
                                 int format, char *data);

void nxagentPutSubImage(DrawablePtr pDrawable, GCPtr pGC, int depth,
                            int x, int y, int w, int h, int leftPad, int format,
                                char *data, Visual *pVisual);

void nxagentGetImage(DrawablePtr pDrawable, int x, int y, int w, int h,
                         unsigned int format, unsigned long planeMask, char *data);

/*
 * Pack and split parameters we get
 * from the NX transport.
 */

extern int nxagentPackLossless;
extern int nxagentPackMethod;
extern int nxagentPackQuality;
extern int nxagentSplitThreshold;

/*
 * Set if images can use the alpha
 * channel and if the alpha channel
 * can be sent in compressed form.
 */

extern int nxagentAlphaEnabled;
extern int nxagentAlphaCompat;

/*
 * Reset the visual and alpha cache
 * before closing the screen or con-
 * necting to a different display.
 */

void nxagentResetVisualCache(void);
void nxagentResetAlphaCache(void);

/*
 * Always use the default visual for the
 * image related functions.
 */

#define nxagentImageVisual(pDrawable, depth) \
    ((depth) == 32 ? &nxagentAlphaVisual : \
         nxagentDefaultVisual(((pDrawable) -> pScreen)))

/*
 * Byte swap the image if the display
 * uses a different endianess.
 */

#define nxagentImageNormalize(image) \
    ((image) -> byte_order != IMAGE_BYTE_ORDER || \
         (image) -> bitmap_bit_order != BITMAP_BIT_ORDER ? \
             nxagentImageReformat((image) -> data, (image) -> bytes_per_line * \
                 (image) -> height * ((image) -> format == XYPixmap ? (image) -> depth : 1), \
                     ((image) -> format == ZPixmap ? \
                         BitsPerPixel((image) -> depth) : 1), \
                             (image) -> byte_order) : 0)

/*
 * Other image related functions.
 */

int nxagentImageLength(int width, int height, int format, int leftPad, int depth);

int nxagentImagePad(int width, int format, int leftPad, int depth);

int nxagentImageReformat(char *base, int nbytes, int bpp, int order);

void nxagentImageStatisticsHandler(char **buffer, int type);

int nxagentScaleImage(int x, int y, unsigned xRatio, unsigned yRatio, XImage **pImage, int *scaledx, int *scaledy);

char *nxagentAllocateImageData(int width, int height, int depth, int *length, int *format);

#endif /* __Image_H__ */

