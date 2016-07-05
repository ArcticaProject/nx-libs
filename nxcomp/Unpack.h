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

#ifndef Unpack_H
#define Unpack_H

#include "NXpack.h"

#include "Z.h"

#define LSBFirst  0
#define MSBFirst  1

#define SPLIT_PATTERN  0x88

typedef ColorMask T_colormask;

//
// Pixel geometry of channel's display.
//

typedef struct
{
  unsigned int depth1_bpp;
  unsigned int depth4_bpp;
  unsigned int depth8_bpp;
  unsigned int depth16_bpp;
  unsigned int depth24_bpp;
  unsigned int depth32_bpp;

  unsigned int red_mask;
  unsigned int green_mask;
  unsigned int blue_mask;

  unsigned int image_byte_order;
  unsigned int bitmap_bit_order;
  unsigned int scanline_unit;
  unsigned int scanline_pad;

} T_geometry;

//
// Colormap is used to remap colors
// from source to destination depth.
//
 
typedef struct
{
  unsigned int entries;
  unsigned int *data;

} T_colormap;

//
// Alpha channel data is added to 32
// bits images at the time they are
// unpacked.
//
 
typedef struct
{
  unsigned int entries;
  unsigned char *data;

} T_alpha;

//
// The ZLIB stream structure used for
// the decompression.
//

extern z_stream unpackStream;

//
// Initialize the ZLIB stream used for
// decompression.
//

void UnpackInit();

//
// Free the ZLIB stream.
//

void UnpackDestroy();

//
// Get the destination bits per pixel
// based on the drawable depth.
//

int UnpackBitsPerPixel(T_geometry *geometry, unsigned int depth);

//
// Unpack the source data into the X
// bitmap.
//

int Unpack8(T_geometry *geometry, const T_colormask *colormask, int src_depth, int src_width,
                int src_height, unsigned char *src_data, int src_size, int dst_depth,
                    int dst_width, int dst_height, unsigned char *dst_data, int dst_size);

int Unpack16(T_geometry *geometry, const T_colormask *colormask, int src_depth, int src_width,
                 int src_height, unsigned char *src_data, int src_size, int dst_depth,
                     int dst_width, int dst_height, unsigned char *dst_data, int dst_size);

int Unpack24(T_geometry *geometry, const T_colormask *colormask, int src_depth, int src_width,
                 int src_height, unsigned char *src_data, int src_size, int dst_depth,
                     int dst_width, int dst_height, unsigned char *dst_data, int dst_size);

int Unpack8(T_geometry *geometry, T_colormap *colormap, int src_depth, int src_width,
                int src_height, unsigned char *src_data, int src_size, int dst_depth,
                    int dst_width, int dst_height, unsigned char *dst_data, int dst_size);

int Unpack15(T_geometry *geometry, int src_depth, int src_width,
                int src_height, unsigned char *src_data, int src_size, int dst_depth,
                    int dst_width, int dst_height, unsigned char *dst_data, int dst_size);

int Unpack16(T_geometry *geometry, int src_depth, int src_width,
                int src_height, unsigned char *src_data, int src_size, int dst_depth,
                    int dst_width, int dst_height, unsigned char *dst_data, int dst_size);

int Unpack24(T_geometry *geometry, int src_depth, int src_width,
                int src_height, unsigned char *src_data, int src_size, int dst_depth,
                    int dst_width, int dst_height, unsigned char *dst_data, int dst_size);

#endif /* Unpack_H */
