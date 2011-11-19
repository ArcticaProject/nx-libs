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

#ifndef NXpack_H
#define NXpack_H

#ifdef __cplusplus
extern "C" {
#endif

#define MASK_METHOD_LIMIT                       10

#define NO_MASK                                 0

#define MASK_8_COLORS                           1
#define MASK_64_COLORS                          2
#define MASK_256_COLORS                         3
#define MASK_512_COLORS                         4
#define MASK_4K_COLORS                          5
#define MASK_32K_COLORS                         6
#define MASK_64K_COLORS                         7
#define MASK_256K_COLORS                        8
#define MASK_2M_COLORS                          9
#define MASK_16M_COLORS                         10

#define PACK_METHOD_LIMIT                       128

#define NO_PACK                                 0

#define PACK_MASKED_8_COLORS                    1
#define PACK_MASKED_64_COLORS                   2
#define PACK_MASKED_256_COLORS                  3
#define PACK_MASKED_512_COLORS                  4
#define PACK_MASKED_4K_COLORS                   5
#define PACK_MASKED_32K_COLORS                  6
#define PACK_MASKED_64K_COLORS                  7
#define PACK_MASKED_256K_COLORS                 8
#define PACK_MASKED_2M_COLORS                   9
#define PACK_MASKED_16M_COLORS                  10

#define PACK_RAW_8_BITS                         3
#define PACK_RAW_16_BITS                        7
#define PACK_RAW_24_BITS                        10

#define PACK_COLORMAP_256_COLORS                11

#define PACK_JPEG_8_COLORS                      26
#define PACK_JPEG_64_COLORS                     27
#define PACK_JPEG_256_COLORS                    28
#define PACK_JPEG_512_COLORS                    29
#define PACK_JPEG_4K_COLORS                     30
#define PACK_JPEG_32K_COLORS                    31
#define PACK_JPEG_64K_COLORS                    32
#define PACK_JPEG_256K_COLORS                   33
#define PACK_JPEG_2M_COLORS                     34
#define PACK_JPEG_16M_COLORS                    35

#define PACK_PNG_8_COLORS                       37
#define PACK_PNG_64_COLORS                      38
#define PACK_PNG_256_COLORS                     39
#define PACK_PNG_512_COLORS                     40
#define PACK_PNG_4K_COLORS                      41
#define PACK_PNG_32K_COLORS                     42
#define PACK_PNG_64K_COLORS                     43
#define PACK_PNG_256K_COLORS                    44
#define PACK_PNG_2M_COLORS                      45
#define PACK_PNG_16M_COLORS                     46

#define PACK_RGB_16M_COLORS                     63
#define PACK_RLE_16M_COLORS                     64

#define PACK_ALPHA                              65
#define PACK_COLORMAP                           66

#define PACK_BITMAP_16M_COLORS                  67

/*
 * Not really pack methods. These values
 * allow dynamic selection of the pack
 * method by the agent.
 */

#define PACK_NONE                               0
#define PACK_LOSSY                              253
#define PACK_LOSSLESS                           254
#define PACK_ADAPTIVE                           255

/*
 * Reduce the number of colors in the
 * image by applying a mask.
 */

typedef struct
{
  unsigned int color_mask;
  unsigned int correction_mask;
  unsigned int white_threshold;
  unsigned int black_threshold;

} ColorMask;

extern const ColorMask Mask8TrueColor;
extern const ColorMask Mask64TrueColor;
extern const ColorMask Mask512TrueColor;
extern const ColorMask Mask4KTrueColor;
extern const ColorMask Mask32KTrueColor;
extern const ColorMask Mask256KTrueColor;
extern const ColorMask Mask2MTrueColor;
extern const ColorMask Mask16MTrueColor;

const ColorMask *MethodColorMask(unsigned int method);

int MethodBitsPerPixel(unsigned int method);

#ifdef __cplusplus
}
#endif

#endif /* NXpack_H */
