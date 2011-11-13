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

#include "Misc.h"
#include "Unpack.h"

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

//
// Used for the ZLIB decompression
// of RGB, alpha and colormap data.
//

z_stream unpackStream;

static int unpackInitialized;

int Unpack8To8(const T_colormask *colormask, const unsigned char *data,
                   unsigned char *out, unsigned char *end);

int Unpack8To8(T_colormap *colormap, const unsigned char *data,
                   unsigned char *out, unsigned char *end);

int Unpack8To16(const T_colormask *colormask, const unsigned char *data,
                    unsigned char *out, unsigned char *end);

int Unpack8To16(T_colormap *colormap, const unsigned char *data,
                    unsigned char *out, unsigned char *end);

int Unpack8To24(const T_colormask *colormask, const unsigned char *data,
                    unsigned char *out, unsigned char *end);

int Unpack8To24(T_colormap *colormap, const unsigned char *data,
                    unsigned char *out, unsigned char *end);

int Unpack8To32(const T_colormask *colormask, const unsigned char *data,
                    unsigned char *out, unsigned char *end);

int Unpack8To32(T_colormap *colormap, const unsigned char *data,
                    unsigned char *out, unsigned char *end);

int Unpack15To16(const unsigned char *data, unsigned char *out,
                     unsigned char *end);

int Unpack15To24(const unsigned char *data, unsigned char *out,
                     unsigned char *end);

int Unpack15To32(const unsigned char *data, unsigned char *out,
                     unsigned char *end);

int Unpack16To16(const T_colormask *colormask, const unsigned char *data,
                     unsigned char *out, unsigned char *end);

int Unpack16To16(const unsigned char *data, unsigned char *out,
                     unsigned char *end, int imageByteOrder);

int Unpack16To24(const T_colormask *colormask, const unsigned char *data,
                     unsigned char *out, unsigned char *end);

int Unpack16To24(const unsigned char *data, unsigned char *out,
                     unsigned char *end, int imageByteOrder);

int Unpack16To32(const T_colormask *colormask, const unsigned char *data,
                     unsigned char *out, unsigned char *end);

int Unpack16To32(const unsigned char *data, unsigned char *out,
                     unsigned char *end, int imageByteOrder);

int Unpack24To24(const T_colormask *colormask, const unsigned char *data,
                     unsigned char *out, unsigned char *end);

int Unpack24To24(const unsigned char *data, unsigned char *out,
                     unsigned char *end);

int Unpack24To32(const T_colormask *colormask, const unsigned char *data,
                     unsigned char *out, unsigned char *end);

int Unpack24To32(const unsigned char *data, unsigned char *out, unsigned char *end);

int Unpack32To32(const T_colormask *colormask, const unsigned int *data,
                            unsigned int *out, unsigned int *end);


void UnpackInit()
{
  if (unpackInitialized == 0)
  {
    unpackStream.zalloc = (alloc_func) 0;
    unpackStream.zfree  = (free_func) 0;
    unpackStream.opaque = (voidpf) 0;

    unpackStream.next_in = (Bytef *) 0;
    unpackStream.avail_in = 0;

    int result = inflateInit2(&unpackStream, 15);

    if (result != Z_OK)
    {
      #ifdef PANIC
      *logofs << "UnpackInit: PANIC! Cannot initialize the Z stream "
              << "for decompression. Error is '" << zError(result)
              << "'.\n" << logofs_flush;
      #endif

      cerr << "Error" << ": Cannot initialize the Z stream for "
           << "decompression. Error is '" << zError(result)
           << "'.\n";
    }
    else
    {
      unpackInitialized = 1;
    }
  }
}

void UnpackDestroy()
{
  if (unpackInitialized == 1)
  {
    inflateEnd(&unpackStream);

    unpackInitialized = 0;
  }
}

//
// Get bits per pixel set by client
// according to display geometry.
//

int UnpackBitsPerPixel(T_geometry *geometry, unsigned int depth)
{
  switch (depth)
  {
    case 1:
    {
      return geometry -> depth1_bpp;
    }
    case 4:
    {
      return geometry -> depth4_bpp;
    }
    case 8:
    {
      return geometry -> depth8_bpp;
    }
    case 15:
    case 16:
    {
      return geometry -> depth16_bpp;
    }
    case 24:
    {
      return geometry -> depth24_bpp;
    }
    case 32:
    {
      return geometry -> depth32_bpp;
    }
    default:
    {
      return 0;
    }
  }
}

int Unpack8To8(const T_colormask *colormask, const unsigned char *data,
                    unsigned char *out, unsigned char *end)
{
  #ifdef TEST
  *logofs << "Unpack8To8: Unpacking " << end - out
          << " bytes of data.\n" << logofs_flush;
  #endif

  memcpy(out, data, end - out);

  return 1;
}

int Unpack8To16(const T_colormask *colormask, const unsigned char *data,
                    unsigned char *out, unsigned char *end)
{
  #ifdef TEST
  *logofs << "Unpack8To16: Unpacking " << end - out
          << " bytes of data.\n" << logofs_flush;
  #endif

  unsigned short *out16 = (unsigned short *) out;
  unsigned short *end16 = (unsigned short *) end;
  
  while (out16 < end16)
  {
    if (*data == 0)
    {
      *out16 = 0x0;
    }
    else if (*data == 0xff)
    {
      *out16 = 0xffff;
    }
    else
    {
      //
      // Pixel layout:
      //
      // 8bits 00RRGGBB -> 16bits RR000GG0 000BB000.
      //

      *out16 = (((((*data & 0x30) << 2) | colormask -> correction_mask) << 8) & 0xf800) |
                   (((((*data & 0xc) << 4) | colormask -> correction_mask) << 3) & 0x7e0) |
                       (((((*data & 0x3) << 6) | colormask -> correction_mask) >> 3) & 0x1f);
    }

    out16++;
    data++;
  }

  return 1;
}

int Unpack8To24(const T_colormask *colormask, const unsigned char *data,
                    unsigned char *out, unsigned char *end)
{
  #ifdef TEST
  *logofs << "Unpack8To24: Unpacking " << end - out
          << " bytes of data.\n" << logofs_flush;
  #endif

  while (out < (end - 2))
  {
    if (*data == 0x00)
    {
      out[0] = out[1] = out[2] = 0x00;
    }
    else if (*data == 0xff)
    {
      out[0] = out[1] = out[2] = 0xff;
    }
    else
    {
      //
      // Pixel layout:
      //
      // 8bits 00RRGGBB -> 24bits RR000000 GG00000 BB000000.
      //

      out[0] = (((*data & 0x30) << 2) | colormask -> correction_mask);
      out[1] = (((*data & 0x0c) << 4) | colormask -> correction_mask);
      out[2] = (((*data & 0x03) << 6) | colormask -> correction_mask);
    }

    out  += 3;
    data += 1;
  }

  return 1;
}

int Unpack8To32(const T_colormask *colormask, const unsigned char *data,
                    unsigned char *out, unsigned char *end)
{
  #ifdef TEST
  *logofs << "Unpack8To32: Unpacking " << end - out
          << " bytes of data.\n" << logofs_flush;
  #endif

  unsigned int *out32 = (unsigned int *) out;
  unsigned int *end32 = (unsigned int *) end;

  while (out32 < end32)
  {
    if (*data == 0)
    {
      *out32 = 0x0;
    }
    else if (*data == 0xff)
    {
      *out32 = 0xffffff;
    }
    else
    {
      *out32 = ((((*data & 0x30) << 2) | colormask -> correction_mask) << 16) |
                   ((((*data & 0xc) << 4) | colormask -> correction_mask) << 8) |
                       (((*data & 0x3) << 6) | colormask -> correction_mask);
    }

    out32++;
    data++;
  }

  return 1;
}

int Unpack8(T_geometry *geometry, const T_colormask *colormask, int src_depth, int src_width,
                int src_height, unsigned char *src_data, int src_size, int dst_depth,
                    int dst_width, int dst_height, unsigned char *dst_data, int dst_size)
{
  int dst_bpp = UnpackBitsPerPixel(geometry, dst_depth);

  int (*unpack)(const T_colormask *colormask, const unsigned char *data,
                    unsigned char *out, unsigned char *end);

  switch (dst_bpp)
  {
    case 8:
    {
      unpack = Unpack8To8;

      break;
    }
    case 16:
    {
      unpack = Unpack8To16;

      break;
    }
    case 24:
    {
      unpack = Unpack8To24;

      break;
    }
    case 32:
    {
      unpack = Unpack8To32;

      break;
    }
    default:
    {
      #ifdef PANIC
      *logofs << "Unpack8: PANIC! Bad destination bits per pixel "
              << dst_bpp << ". Only 16/24/32 are supported.\n"
              << logofs_flush;
      #endif

      return -1;
    }
  }

  if (dst_bpp == 24)
  {
    unsigned char *dst_end = dst_data;

    #ifdef TEST
    *logofs  << "Unpack8: Handling 24 bits with dst_size "
             << dst_size << ".\n" << logofs_flush;
    #endif

    for (int y = 0; y < dst_height; y++)
    {
      dst_data = dst_end;

      dst_end += RoundUp4(dst_width * 3);

      (*unpack)(colormask, src_data, dst_data, dst_end);

      src_data += src_width;
    }
  }
  else
  {
    unsigned char *dst_end = dst_data + dst_size;

    (*unpack)(colormask, src_data, dst_data, dst_end);
  }

  return 1;
}

int Unpack16To16(const T_colormask *colormask, const unsigned char *data,
                     unsigned char *out, unsigned char *end)
{
  #ifdef TEST
  *logofs << "Unpack16To16: Unpacking " << end - out
          << " bytes of data.\n" << logofs_flush;
  #endif

  if (colormask -> correction_mask)
  {
    unsigned short *data16 = (unsigned short *) data;

    unsigned short *out16 = (unsigned short *) out;
    unsigned short *end16 = (unsigned short *) end;

    while (out16 < end16)
    {
      if (*data16 == 0x0000)
      {
        *out16 = 0x0000;
      }
      else if (*data16 == 0xffff)
      {
        *out16 = 0xffff;
      }
      else
      {
        //
        // Pixel layout:
        //
        // 16bit RRRRRGGG GG0BBBBB  -> RRRRRGGG GGGBBBBB.
        //

        *out16 = (((((*data16 & 0xf100) >> 8) | colormask -> correction_mask) << 8) & 0xf800) |
                     (((((*data16 & 0x7c0) >> 3) | colormask -> correction_mask) << 3) & 0x7e0) |
                         (((((*data16 & 0x1f) << 3) | colormask -> correction_mask) >> 3) & 0x1f);
      }

      out16++;
      data16++;
    }
  }
  else
  {
    #ifdef TEST
    *logofs << "Unpack16To16: Using bitwise copy due to null correction mask.\n"
            << logofs_flush;
    #endif

    memcpy((unsigned char *) out, (unsigned char *) data, end - out);
  }

  return 1;
}

int Unpack16To24(const T_colormask *colormask, const unsigned char *data,
                     unsigned char *out, unsigned char *end)
{
  #ifdef TEST
  *logofs << "Unpack16To24: Unpacking " << end - out
          << " bytes of data.\n" << logofs_flush;
  #endif

  unsigned short *data16 = (unsigned short *) data;


  while (out < end - 2)
  {
    if (*data16 == 0x0)
    {
      out[0] = 0x00;
      out[1] = 0x00;
      out[2] = 0x00;
    }
    else if (*data16 == 0xffff)
    {
      out[0] = 0xff;
      out[1] = 0xff;
      out[2] = 0xff;
    }
    else
    {
      #ifdef TEST
      *logofs << "Unpack16To24: Pixel [" << *data16 << "]\n"
              << logofs_flush;
      #endif

      //
      // Pixel layout:
      //
      //  16bit 0RRRRRGG GGGBBBBB  -> 24 bit RRRRR000 GGGGG000 BBBBB000
      //

      out[0] = (((*data16 & 0x7c00) >> 7) | colormask -> correction_mask);
      out[1] = (((*data16 & 0x03e0) >> 2) | colormask -> correction_mask);
      out[2] = (((*data16 & 0x001f) << 3) | colormask -> correction_mask);
    }

    out    += 3;
    data16 += 1;
  }

  return 1;
}

int Unpack16To32(const T_colormask *colormask, const unsigned char *data,
                     unsigned char *out, unsigned char *end)
{
  #ifdef TEST
  *logofs << "Unpack16To32: Unpacking " << end - out
          << " bytes of data.\n" << logofs_flush;
  #endif

  unsigned short *data16 = (unsigned short *) data;

  unsigned int *out32 = (unsigned int *) out;
  unsigned int *end32 = (unsigned int *) end;

  while (out32 < end32)
  {
    if (*data16 == 0x0)
    {
      *out32 = 0x0;
    }
    else if (*data16 == 0xffff)
    {
      *out32 = 0xffffff;
    }
    else
    {
      *out32 = ((((*data16 & 0x7c00) >> 7) | colormask -> correction_mask) << 16) |
                   ((((*data16 & 0x3e0) >> 2) | colormask -> correction_mask) << 8) |
                       (((*data16 & 0x1f) << 3) | colormask -> correction_mask);
    }

    out32++;
    data16++;
  }

  return 1;
}

int Unpack16(T_geometry *geometry, const T_colormask *colormask, int src_depth, int src_width,
                 int src_height, unsigned char *src_data, int src_size, int dst_depth,
                     int dst_width, int dst_height, unsigned char *dst_data, int dst_size)
{
  int dst_bpp = UnpackBitsPerPixel(geometry, dst_depth);

  int (*unpack)(const T_colormask *colormask, const unsigned char *data,
                    unsigned char *out, unsigned char *end);

  switch (dst_bpp)
  {
    case 16:
    {
      unpack = Unpack16To16;

      break;
    }
    case 24:
    {
      unpack = Unpack16To24;

      break;
    }
    case 32:
    {
      unpack = Unpack16To32;

      break;
    }
    default:
    {
      #ifdef PANIC
      *logofs << "Unpack16: PANIC! Bad destination bits per pixel "
              << dst_bpp << ". Only 24/32 are supported.\n"
              << logofs_flush;
      #endif

      return -1;
    }
  }

  if (dst_bpp == 24)
  {
    unsigned char *dst_end = dst_data;

    for (int y = 0; y < dst_height; y++)
    {
      dst_data = dst_end;

      dst_end += RoundUp4(dst_width * 3);

      (*unpack)(colormask, src_data, dst_data, dst_end);

      src_data += (src_width * 2);
    }

  }
  else
  {
    unsigned char *dst_end = dst_data + dst_size;

    (*unpack)(colormask, src_data, dst_data, dst_end);
  }
  
  return 1;
}

int Unpack24To24(const T_colormask *colormask, const unsigned char *data,
                     unsigned char *out, unsigned char *end)
{
  #ifdef TEST
  *logofs << "Unpack24To24: Unpacking " << end - out
          << " bytes of data.\n" << logofs_flush;
  #endif

  if (colormask -> correction_mask)
  {
    while (out < end)
    {
      if (data[0] == 0x00 &&
              data[1] == 0x00 &&
                  data[2] == 0x00)
      {
        out[0] = out[1] = out[2] = 0x00;
      }
      else if (data[0] == 0xff &&
                   data[1] == 0xff &&
                       data[2] == 0xff)
      {
        out[0] = out[1] = out[2] = 0xff;
      }
      else
      {
        out[0] = (data[0] | colormask -> correction_mask);
        out[1] = (data[1] | colormask -> correction_mask);
        out[2] = (data[2] | colormask -> correction_mask);
      }

      out  += 3;
      data += 3;
    }
  }
  else
  {
    #ifdef TEST
    *logofs << "Unpack24To24: Using bitwise copy due to null correction mask.\n"
            << logofs_flush;
    #endif

    memcpy((unsigned char *) out, (unsigned char *) data, end - out);
  }

  return 1;
}

int Unpack24To32(const T_colormask *colormask, const unsigned char *data,
                     unsigned char *out, unsigned char *end)
{
  #ifdef TEST
  *logofs << "Unpack24To32: Unpacking " << end - out
          << " bytes of data.\n" << logofs_flush;
  #endif

  unsigned int *out32 = (unsigned int *) out;
  unsigned int *end32 = (unsigned int *) end;

  while (out32 < end32)
  {
    if (colormask -> color_mask == 0xff)
    {
      *out32 = (data[0] << 16) | (data[1] << 8) | data[2];
    }
    else
    {
      if (data[0] == 0x0 && data[1] == 0x0 && data[2] == 0x0)
      {
        *out32 = 0x0;
      }
      else if (data[0] == 0xff && data[1] == 0xff && data[2] == 0xff)
      {
        *out32 = 0xffffff;
      }
      else
      {
        *out32 = (((unsigned int) data[0] | colormask -> correction_mask) << 16) |
                   (((unsigned int) data[1] | colormask -> correction_mask) << 8) |
                       ((unsigned int) data[2] | colormask -> correction_mask);
      }
    }

    out32 += 1;
    data  += 3;
  }

  return 1;
}

int Unpack24(T_geometry *geometry, const T_colormask *colormask, int src_depth, int src_width,
                 int src_height, unsigned char *src_data, int src_size, int dst_depth,
                     int dst_width, int dst_height, unsigned char *dst_data, int dst_size)
{
  int dst_bpp = UnpackBitsPerPixel(geometry, dst_depth);

  int (*unpack)(const T_colormask *colormask, const unsigned char *data,
                    unsigned char *out, unsigned char *end);

  switch (dst_bpp)
  {
    case 24:
    {
      unpack = Unpack24To24;

      break;
    }
    case 32:
    {
      unpack = Unpack24To32;

      break;
    }
    default:
    {
      #ifdef PANIC
      *logofs << "Unpack24: PANIC! Bad destination bits per pixel "
              << dst_bpp << ". Only 32 is supported.\n"
              << logofs_flush;
      #endif

      return -1;
    }
  }

  if (dst_bpp == 24)
  {
    unsigned char *dst_end;
    unsigned long scanline_size = RoundUp4(dst_width * dst_bpp / 8);

    dst_end = dst_data;

    #ifdef TEST
    *logofs  << "Unpack24: Handling 24 bits with dst_height "
             << dst_height << " scanline_size " << scanline_size
             << " dst_size " << dst_size << ".\n" << logofs_flush;
    #endif

    for (int y = 0; y < dst_height; y++)
    {
      dst_data = dst_end;

      dst_end += scanline_size;

      (*unpack)(colormask, src_data, dst_data, dst_end);

      src_data += scanline_size;
    }
  }
  else
  {
    unsigned char *dst_end = dst_data + dst_size;

    (*unpack)(colormask, src_data, dst_data, dst_end);
  }

  return 1;
}

int Unpack8To8(T_colormap *colormap, const unsigned char *data,
                   unsigned char *out, unsigned char *end)
{
  #ifdef TEST
  *logofs << "Unpack8To8: Unpacking " << end - out
          << " bytes of colormapped data.\n"
          << logofs_flush;
  #endif

  while (out < end)
  {
    *(out++) = (unsigned char) colormap -> data[*(data++)];
  }

  return 1;
}

int Unpack8To16(T_colormap *colormap, const unsigned char *data,
                    unsigned char *out, unsigned char *end)
{
  #ifdef TEST
  *logofs << "Unpack8To16: Unpacking " << end - out
          << " bytes of colormapped data.\n"
          << logofs_flush;
  #endif

  unsigned short *out16 = (unsigned short *) out;
  unsigned short *end16 = (unsigned short *) end;

  while (out16 < end16)
  {
    *(out16++) = (unsigned short) colormap -> data[*(data++)];
  }

  return 1;
}

int Unpack8To24(T_colormap *colormap, const unsigned char *data,
                    unsigned char *out, unsigned char *end)
{
  #ifdef TEST
  *logofs << "Unpack8To24: Unpacking " << end - out
          << " bytes of colormapped data.\n"
          << logofs_flush;
  #endif

  unsigned int value;

  while (out < end)
  {
    value = colormap -> data[*(data++)];

    *(out++) = value;
    *(out++) = value >> 8;
    *(out++) = value >> 16;
  }

  return 1;
}

int Unpack8To32(T_colormap *colormap, const unsigned char *data,
                    unsigned char *out, unsigned char *end)
{
  #ifdef TEST
  *logofs << "Unpack8To32: Unpacking " << end - out
          << " bytes of colormapped data.\n"
          << logofs_flush;
  #endif

  unsigned int *out32 = (unsigned int *) out;
  unsigned int *end32 = (unsigned int *) end;

  while (out32 < end32)
  {
    *(out32++) = colormap -> data[*(data++)];
  }

  return 1;
}

int Unpack8(T_geometry *geometry, T_colormap *colormap, int src_depth, int src_width, int src_height,
                 unsigned char *src_data, int src_size, int dst_depth, int dst_width,
                     int dst_height, unsigned char *dst_data, int dst_size)
{
  if (src_depth != 8)
  {
    #ifdef PANIC
    *logofs << "Unpack8: PANIC! Cannot unpack colormapped image of source depth "
            << src_depth << ".\n" << logofs_flush;
    #endif

    return -1;
  }

  int (*unpack)(T_colormap *colormap, const unsigned char *data,
                    unsigned char *out, unsigned char *end);

  int dst_bpp = UnpackBitsPerPixel(geometry, dst_depth);

  switch (dst_bpp)
  {
    case 8:
    {
      unpack = Unpack8To8;

      break;
    }
    case 16:
    {
      unpack = Unpack8To16;

      break;
    }
    case 24:
    {
      unpack = Unpack8To24;

      break;
    }
    case 32:
    {
      unpack = Unpack8To32;

      break;
    }
    default:
    {
      #ifdef PANIC
      *logofs << "Unpack8: PANIC! Bad destination bits per pixel "
              << dst_bpp << ". Only 8/16/24/32 are supported.\n"
              << logofs_flush;
      #endif

      return -1;
    }
  }

  if (src_width == dst_width &&
          src_height == dst_height)
  {
    unsigned char *dst_end = dst_data + dst_size;

    (*unpack)(colormap, src_data, dst_data, dst_end);
  }
  else if (src_width >= dst_width &&
             src_height >= dst_height)
  {
    unsigned char *dst_end = dst_data;

    for (int y = 0; y < dst_height; y++)
    {
      dst_data = dst_end;

      dst_end += RoundUp4(dst_width * dst_bpp / 8);

      (*unpack)(colormap, src_data, dst_data, dst_end);

      src_data += src_width;
    }
  }
  else
  {
    #ifdef PANIC
    *logofs << "Unpack8: PANIC! Cannot unpack image. "
            << "Destination area " << dst_width << "x"
            << dst_height << " is not fully contained in "
            << src_width << "x" << src_height << " source.\n"
            << logofs_flush;
    #endif

    return -1;
  }

  return 1;
}

int Unpack15To16(const unsigned char *data, unsigned char *out, unsigned char *end)
{
  #ifdef TEST
  *logofs << "Unpack15To16: Unpacking " << end - out
          << " bytes of colormapped data.\n"
          << logofs_flush;
  #endif
  
  unsigned short *data16 = (unsigned short *) data;
  unsigned short *out16 = (unsigned short *) out;
  unsigned short *end16 = (unsigned short *) end;

  while (out16 < end16)
  {
    if (*data16 == 0x0)
    {
      *out16 = 0x0;
    }
    else if (*data16 == 0x7fff)
    {
      *out16 = 0xffff;
    }
    else
    {
      #ifdef TEST
      *logofs << "Unpack15To16: Pixel [" << *data16 << "]\n"
              << logofs_flush;
      #endif
      
     *out16 = ((*data16 & 0x7ff0) << 1) |
               (*data16 & 0x001f);
    }

    out16  += 1;
    data16 += 1;
  }
  
  return 1;
}

int Unpack15To24(const unsigned char *data, unsigned char *out, unsigned char *end)

{
  #ifdef TEST
  *logofs << "Unpack15To24: Unpacking " << end - out
          << " bytes of data.\n" << logofs_flush;
  #endif

  unsigned short *data16 = (unsigned short *) data;

  while (out < end - 2)
  {
    if (*data16 == 0x0)
    {
      out[0] = 0x00;
      out[1] = 0x00;
      out[2] = 0x00;
    }
    else if (*data16 == 0x7fff)
    {
      out[0] = 0xff;
      out[1] = 0xff;
      out[2] = 0xff;
    }
    else
    {
      #ifdef TEST
      *logofs << "Unpack15To24: Pixel [" << *data16 << "]\n"
              << logofs_flush;
      #endif

      out[0] = ((*data16 >> 7) & 0xf8) | ((*data16 >> 12) & 0x07);
      out[1] = ((*data16 >> 2) & 0xf8) | ((*data16 >> 8) & 0x07);
      out[2] = ((*data16 << 3) & 0xf8) | ((*data16 >> 2) & 0x07);
    }

    out    += 3;
    data16 += 1;
  }

  return 1;
}

int Unpack15To32(const unsigned char *data, unsigned char *out, unsigned char *end)
{
  #ifdef TEST
  *logofs << "Unpack15To32: Unpacking " << end - out
          << " bytes of data.\n"
          << logofs_flush;
  #endif

  unsigned short *data16 = (unsigned short *) data;
  unsigned int *out32 = (unsigned int *) out;
  unsigned int *end32 = (unsigned int *) end;

  while (out32 < end32)
  {
    if (*data16 == 0x0)
    {
      *out32 = 0x0;
    }
    else if (*data16 == 0xffff)
    {
      *out32 = 0xffffff;
    }
    else
    {
        *out32 = ((((*data16 >> 7) & 0xf8) | ((*data16 >> 12) & 0x07)) << 16) | 
                 ((((*data16 >> 2) & 0xf8) | ((*data16 >> 8) & 0x07)) << 8) | 
                  (((*data16 << 3) & 0xf8) | ((*data16 >> 2) & 0x07));
    }

    out32++;
    data16++;
  }

  return 1;
}

int Unpack15(T_geometry *geometry, int src_depth, int src_width, int src_height,
                 unsigned char *src_data, int src_size, int dst_depth, int dst_width,
                     int dst_height, unsigned char *dst_data, int dst_size)
{
  if (src_depth != 16)
  {
    #ifdef PANIC
    *logofs << "Unpack15: PANIC! Cannot unpack colormapped image of source depth "
            << src_depth << ".\n" << logofs_flush;
    #endif

    return -1;
  }

  int (*unpack)(const unsigned char *data, unsigned char *out, unsigned char *end);

  int dst_bpp = UnpackBitsPerPixel(geometry, dst_depth);

  switch (dst_bpp)
  {
    case 16:
    {
      unpack = Unpack15To16;

      break;
    }
    case 24:
    {
      unpack = Unpack15To24;

      break;
    }
    case 32:
    {
      unpack = Unpack15To32;

      break;
    }
    default:
    {
      #ifdef PANIC
      *logofs << "Unpack15: PANIC! Bad destination bits per pixel "
              << dst_bpp << ". Only 16/24/32 are supported.\n"
              << logofs_flush;
      #endif

      return -1;
    }
  }

  if (src_width == dst_width && src_height == dst_height)
  {
    unsigned char *dst_end = dst_data + dst_size;

    (*unpack)(src_data, dst_data, dst_end);
  }
  else if (src_width >= dst_width && src_height >= dst_height)
  {
    unsigned char *dst_end = dst_data;

    for (int y = 0; y < dst_height; y++)
    {
      dst_data = dst_end;
      dst_end += RoundUp4(dst_width * dst_bpp / 8);

      (*unpack)(src_data, dst_data, dst_end);

      src_data += src_width * 2;
    }
  }
  else
  {
    #ifdef PANIC
    *logofs << "Unpack15: PANIC! Cannot unpack image. "
            << "Destination area " << dst_width << "x"
            << dst_height << " is not fully contained in "
            << src_width << "x" << src_height << " source.\n"
            << logofs_flush;
    #endif

    return -1;
  }

  return 1;
}

int Unpack16To16(const unsigned char *data, unsigned char *out,
                    unsigned char *end, int imageByteOrder)
{
  #ifdef TEST
  *logofs << "Unpack16To16: Unpacking " << end - out
          << " bytes of colormapped data.\n"
          << logofs_flush;
  #endif
  
  memcpy((unsigned char *) out, (unsigned char *) data, end - out);

  return 1;
}

int Unpack16To24(const unsigned char *data, unsigned char *out, 
                    unsigned char *end, int imageByteOrder)

{
  #ifdef TEST
  *logofs << "Unpack16To24: Unpacking " << end - out
          << " bytes of data.\n" << logofs_flush;
  #endif

  unsigned short *data16 = (unsigned short *) data;

  while (out < end - 2)
  {
    if (*data16 == 0x0)
    {
      out[0] = 0x00;
      out[1] = 0x00;
      out[2] = 0x00;
    }
    else if (*data16 == 0xffff)
    {
      out[0] = 0xff;
      out[1] = 0xff;
      out[2] = 0xff;
    }
    else
    {
      #ifdef TEST
      *logofs << "Unpack16To24: Pixel [" << *data16 << "]\n"
              << logofs_flush;
      #endif

      out[0] = ((*data16 >> 8) & 0xf8) | ((*data16 >> 13) & 0x07);
      out[1] = ((*data16 >> 3) & 0xfc) | ((*data16 >> 9) & 0x03);
      out[2] = ((*data16 << 3) & 0xf8) | ((*data16 >> 2) & 0x07);
    }

    out    += 3;
    data16 += 1;
  }

  return 1;
}


int Unpack16To32(const unsigned char *data, unsigned char *out, 
                    unsigned char *end, int imageByteOrder)
{
  #ifdef TEST
  *logofs << "Unpack16To32: Unpacking " << end - out
          << " bytes of data.\n"
          << logofs_flush;
  #endif

  unsigned short *data16 = (unsigned short *) data;
  unsigned int *out32 = (unsigned int *) out;
  unsigned int *end32 = (unsigned int *) end;

  unsigned short pixel16;
  unsigned int pixel32;

  while (out32 < end32)
  {

    pixel16 = GetUINT((unsigned char *)data16, 0);

    if (pixel16 == 0x0)
    {
      PutULONG(0x0, (unsigned char *) out32, imageByteOrder); 
    }
    else if (pixel16 == 0xffff)
    {
      PutULONG(0xffffff, (unsigned char *) out32, imageByteOrder);
    }
    else
    {
      pixel32 = ((((pixel16 >> 8) & 0xf8) | ((pixel16 >> 13) & 0x07)) << 16) | 
               ((((pixel16 >> 3) & 0xfc) | ((pixel16 >> 9) & 0x03)) << 8) | 
               (((pixel16 << 3) & 0xf8) | ((pixel16 >> 2) & 0x07));
 
      PutULONG(pixel32, (unsigned char *) out32, imageByteOrder); 
    }

    out32++;
    data16++;
  }
  return 1;
}

int Unpack16(T_geometry *geometry, int src_depth, int src_width, int src_height,
                 unsigned char *src_data, int src_size, int dst_depth, int dst_width,
                     int dst_height, unsigned char *dst_data, int dst_size)
{
  int imageByteOrder = geometry -> image_byte_order;

  if (src_depth != 16)
  {
    #ifdef PANIC
    *logofs << "Unpack16: PANIC! Cannot unpack colormapped image of source depth "
            << src_depth << ".\n" << logofs_flush;
    #endif

    return -1;
  }

  int (*unpack)(const unsigned char *data, unsigned char *out, 
                    unsigned char *end, int imageByteOrder);

  int dst_bpp = UnpackBitsPerPixel(geometry, dst_depth);
  
  switch (dst_bpp)
  {
    case 16:
    {
      unpack = Unpack16To16;

      break;
    }
    case 24:
    {
      unpack = Unpack16To24;

      break;
    }
    case 32:
    {
      unpack = Unpack16To32;

      break;
    }
    default:
    {
      #ifdef PANIC
      *logofs << "Unpack16: PANIC! Bad destination bits per pixel "
              << dst_bpp << ". Only 16/24/32 are supported.\n"
              << logofs_flush;
      #endif

      return -1;
    }
  }

  if (src_width == dst_width && src_height == dst_height)
  {
    unsigned char *dst_end = dst_data + dst_size;

    (*unpack)(src_data, dst_data, dst_end, imageByteOrder);
  }
  else if (src_width >= dst_width && src_height >= dst_height)
  {
    unsigned char *dst_end = dst_data;

    for (int y = 0; y < dst_height; y++)
    {
      dst_data = dst_end;
      dst_end += RoundUp4(dst_width * dst_bpp / 8);

      (*unpack)(src_data, dst_data, dst_end, imageByteOrder);

      src_data += src_width * 2;
    }
  }
  else
  {
    #ifdef PANIC
    *logofs << "Unpack16: PANIC! Cannot unpack image. "
            << "Destination area " << dst_width << "x"
            << dst_height << " is not fully contained in "
            << src_width << "x" << src_height << " source.\n"
            << logofs_flush;
    #endif

    return -1;
  }

  return 1;
}

int Unpack24To24(const unsigned char *data,
                    unsigned char *out, unsigned char *end)
{
  #ifdef TEST
  *logofs << "Unpack124To24: Unpacking " << end - out
          << " bytes of colormapped data.\n"
          << logofs_flush;
  #endif

  while (out < end)
  {
    *(out++) = *(data++);
  }

  return 1;
}

int Unpack24To32(const unsigned char *data, unsigned char *out, unsigned char *end)
{
  #ifdef TEST
  *logofs << "Unpack24To32: Unpacking " << end - out
          << " bytes of colormapped data.\n"
          << logofs_flush;
  #endif

  unsigned int *out32 = (unsigned int *) out;
  unsigned int *end32 = (unsigned int *) end;

  while (out32 < end32)
  {
    if (data[0] == 0x0 && data[1] == 0x0 && data[2] == 0x0)
    {
      *out32 = 0x0;
    }
    else if (data[0] == 0xff && data[1] == 0xff && data[2] == 0xff)
    {
      *out32 = 0xffffff;
    }
    else
    {
      *out32 = (data[2] << 16) | (data[1] << 8) | data[0];  
    }

    out32 += 1;
    data  += 3;
  }

  return 1;
}

int Unpack24(T_geometry *geometry, int src_depth, int src_width, int src_height,
                 unsigned char *src_data, int src_size, int dst_depth, int dst_width,
                     int dst_height, unsigned char *dst_data, int dst_size)
                     
{
  if (src_depth != 24)
  {
    #ifdef PANIC
    *logofs << "Unpack24: PANIC! Cannot unpack colormapped image of source depth "
            << src_depth << ".\n" << logofs_flush;
    #endif

    return -1;
  }

  int (*unpack)(const unsigned char *data, unsigned char *out, unsigned char *end);

  int dst_bpp = UnpackBitsPerPixel(geometry, dst_depth);

  switch (dst_bpp)
  {
    case 24:
    {
      unpack = Unpack24To24;

      break;
    }
    case 32:
    {
      unpack = Unpack24To32;

      break;
    }
    default:
    {
      #ifdef PANIC
      *logofs << "Unpack24: PANIC! Bad destination bits per pixel "
              << dst_bpp << ". Only 24/32 are supported.\n"
              << logofs_flush;
      #endif

      return -1;
    }
  }

  if (src_width == dst_width && src_height == dst_height)
  {
    unsigned char *dst_end = dst_data + dst_size;

    (*unpack)(src_data, dst_data, dst_end);
  }
  else if (src_width >= dst_width && src_height >= dst_height)
  {
    unsigned char *dst_end = dst_data;

    for (int y = 0; y < dst_height; y++)
    {
      dst_data = dst_end;
      dst_end += RoundUp4(dst_width * dst_bpp / 8);

      (*unpack)(src_data, dst_data, dst_end);

      src_data += src_width * 3;
    }
  }
  else
  {
    #ifdef PANIC
    *logofs << "Unpack24: PANIC! Cannot unpack image. "
            << "Destination area " << dst_width << "x"
            << dst_height << " is not fully contained in "
            << src_width << "x" << src_height << " source.\n"
            << logofs_flush;
    #endif

    return -1;
  }

  return 1;
}

int Unpack32To32(const T_colormask *colormask, const unsigned int *data,
                     unsigned int *out, unsigned int *end)
{
  #ifdef TEST
  *logofs << "Unpack32To32: Unpacking " << end - out
          << " bytes of data.\n" << logofs_flush;
  #endif

  if (colormask -> correction_mask)
  {
    while (out < end)
    {
      if (*data == 0x00000000)
      {
        *out = 0x00000000;
      }
      else if (*data == 0xFFFFFFFF)
      {
        *out = 0xFFFFFFFF;
      }
      else
      {
        *out = *data | ((colormask -> correction_mask << 16) |
                            (colormask -> correction_mask << 8) |
                                colormask -> correction_mask);
      }

      out  += 1;
      data += 1;
    }
  }
  else
  {
    #ifdef TEST
    *logofs << "Unpack32To32: Using bitwise copy due to null correction mask.\n"
            << logofs_flush;
    #endif

    memcpy((unsigned char *) out, (unsigned char *) data, end - out);
  }

  return 1;
}
