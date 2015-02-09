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

//
// This file obviously supports PNG
// decompression. It was renamed to
// avoid name clashes on Windows.
//

#include <X11/Xmd.h>

#include <unistd.h>
#include <stdio.h>
#include <png.h>

#include "Unpack.h"
#include "Pgn.h"

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

#define RGB24_TO_PIXEL(bpp,r,g,b)                         \
   ((((CARD##bpp)(r) & 0xFF) * srcRedMax2 + 127) / 255    \
    << srcRedShift2 |                                     \
    (((CARD##bpp)(g) & 0xFF) * srcGreenMax2 + 127) / 255  \
    << srcGreenShift2 |                                   \
    (((CARD##bpp)(b) & 0xFF) * srcBlueMax2 + 127) / 255   \
    << srcBlueShift2)

#define RGB24_TO_PIXEL32(r,g,b)                           \
  (((CARD32)(r) & 0xFF) << srcRedShift2 |                 \
   ((CARD32)(g) & 0xFF) << srcGreenShift2 |               \
   ((CARD32)(b) & 0xFF) << srcBlueShift2)

//
// Functions from Unpack.cpp
//

extern int Unpack32To32(const T_colormask *colormask, const unsigned int *data,
                            unsigned int *out, unsigned int *end);

extern int Unpack24To24(const T_colormask *colormask, const unsigned char *data,
                            unsigned char *out, unsigned char *end);

extern int Unpack16To16(const T_colormask *colormask, const unsigned char *data,
                            unsigned char *out, unsigned char *end);

//
// Local functions used for the png decompression.
//

static int DecompressPng16(unsigned char *compressedData, int compressedLen,
                                unsigned int w, unsigned int h, unsigned char *dstBuf,
                                    int byteOrder);

static int DecompressPng24(unsigned char *compressedData, int compressedLen,
                                unsigned int w, unsigned int h, unsigned char *dstBuf,
                                    int byteOrder);

static int DecompressPng32(unsigned char *compressedData, int compressedLen,
                                unsigned int w, unsigned int h, unsigned char *dstBuf,
                                    int byteOrder);

static void PngReadData(png_structp png_ptr, png_bytep data, png_size_t length);

//
// Colormap stuff.
//

CARD16 srcRedMax2, srcGreenMax2, srcBlueMax2;
CARD8  srcRedShift2, srcGreenShift2, srcBlueShift2;

//
// Attributes used for the png decompression.
//

static char *tmpBuf;
static int  tmpBufSize = 0;
static int  streamPos;

int UnpackPng(T_geometry *geometry, unsigned char method, unsigned char *srcData,
                  int srcSize, int dstBpp, int dstWidth, int dstHeight,
                      unsigned char *dstData, int dstSize)
{
  int byteOrder = geometry -> image_byte_order;

  //
  // Check if data is coming from a failed unsplit.
  //

  if (srcSize < 2 || (srcData[0] == SPLIT_PATTERN &&
          srcData[1] == SPLIT_PATTERN))
  {
    #ifdef WARNING
    *logofs << "UnpackPng: WARNING! Skipping unpack of dummy data.\n"
            << logofs_flush;
    #endif

    return -1;
  }

  #ifdef DEBUG
  *logofs << "UnpackPng: Decompressing image with "
          << "srcSize " << srcSize << " and bpp "
          << dstBpp  << ".\n" << logofs_flush;
  #endif

  srcRedShift2   = ffs(geometry -> red_mask) - 1;
  srcGreenShift2 = ffs(geometry -> green_mask) - 1;
  srcBlueShift2  = ffs(geometry -> blue_mask) - 1;
  srcRedMax2     = geometry -> red_mask >> srcRedShift2;
  srcGreenMax2   = geometry -> green_mask >> srcGreenShift2;
  srcBlueMax2    = geometry -> blue_mask >> srcBlueShift2;

  //
  // Make enough space in the temporary
  // buffer to have one complete row of
  // the image with 3 bytes per pixel.
  //

  tmpBufSize = dstWidth * 3;
  tmpBuf = new char [tmpBufSize];

  if (tmpBuf == NULL)
  {
    #ifdef PANIC
    *logofs << "UnpackPng: PANIC! Cannot allocate "
            << dstWidth * 3 << " bytes for PNG "
            << "decompressed data.\n" << logofs_flush;
    #endif

    delete [] tmpBuf;

    return -1;
  }

  int result = 1;

  switch (dstBpp)
  {
    case 8:
    {
      //
      // Simply move the data from srcData to dstData
      // taking into consideration the correct padding.
      //

      int row;

      unsigned char * dstBuff = dstData;
      unsigned char * srcBuff = srcData;

      for (row = 0; row < dstHeight; row++)
      {
        memcpy(dstBuff, srcBuff, dstWidth );

        dstBuff += RoundUp4(dstWidth);
        srcBuff += dstWidth;
      }
    }
    case 16:
    {
      result = DecompressPng16(srcData, srcSize, dstWidth,
                                    dstHeight, dstData, byteOrder);
      break;
    }
    case 24:
    {
      result = DecompressPng24(srcData, srcSize, dstWidth,
                                    dstHeight, dstData, byteOrder);
      break;
    }
    case 32:
    {
      result = DecompressPng32(srcData, srcSize, dstWidth,
                                    dstHeight, dstData, byteOrder);
      break;
    }
    default:
    {
      #ifdef PANIC
      *logofs << "UnpackPng: PANIC! Error in PNG compression. "
              << " Unsupported Bpp value " << dstBpp
              << " for the PNG compression"
              << ".\n" << logofs_flush;
      #endif

      delete [] tmpBuf;

      result = -1;
    }
  }

  if (result == -1)
  {
    delete [] tmpBuf;

    return result;
  }

  //
  // Apply the correction for the brightness
  //

  int maskMethod;

  switch (method)
  {
    case PACK_PNG_8_COLORS:
    {
      maskMethod = MASK_8_COLORS;

      break;
    }
    case PACK_PNG_64_COLORS:
    {
      maskMethod = MASK_64_COLORS;

      break;
    }
    case PACK_PNG_256_COLORS:
    {
      maskMethod = MASK_256_COLORS;

      break;
    }
    case PACK_PNG_512_COLORS:
    {
      maskMethod = MASK_512_COLORS;

      break;
    }
    case PACK_PNG_4K_COLORS:
    {
      maskMethod = MASK_4K_COLORS;

      break;
    }
    case PACK_PNG_32K_COLORS:
    {
      maskMethod = MASK_32K_COLORS;

      break;
    }
    case PACK_PNG_64K_COLORS:
    {
      maskMethod = MASK_64K_COLORS;

      break;
    }
    case PACK_PNG_256K_COLORS:
    {
      maskMethod = MASK_256K_COLORS;

      break;
    }
    case PACK_PNG_2M_COLORS:
    {
      maskMethod = MASK_2M_COLORS;

      break;
    }
    case PACK_PNG_16M_COLORS:
    {
      maskMethod = MASK_16M_COLORS;

      break;
    }
    default:
    {
      #ifdef PANIC
      *logofs << "DecompressPng16: PANIC! "
              << " No matching decompression method.\n"
              << logofs_flush;
      #endif

      delete [] tmpBuf;

      return -1;
    }
  }

  const T_colormask *colorMask = MethodColorMask(maskMethod);

  unsigned char *dstBuff = dstData;

  switch (dstBpp)
  {
    case 16:
    {
      Unpack16To16(colorMask, dstBuff, dstBuff, dstBuff + dstSize);

      break;
    }
    case 24:
    {
      break;
    }
    case 32:
    {
      Unpack32To32(colorMask, (unsigned int *)dstBuff, (unsigned int *)dstBuff,
                       (unsigned int *)(dstBuff + dstSize));
      break;
    }
    default:
    {
      #ifdef PANIC
      *logofs << "DecompressPng16: PANIC! "
              << " No matching destination bits per plane.\n"
              << logofs_flush;
      #endif

      delete [] tmpBuf;

      return -1;
    }
  }

  delete [] tmpBuf;

  return 1;
}


//
// Functions that actually do
// the PNG decompression.
//

int DecompressPng16(unsigned char *compressedData, int compressedLen,
                         unsigned int w, unsigned int h, unsigned char *dstBuf, int byteOrder)
{
  unsigned char *data;
  unsigned int dx, dy;

  png_structp pngPtr;
  png_infop   infoPtr;
  png_bytep   rowPointers;


  streamPos = 0;

  pngPtr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

  if (!pngPtr)
  {
    #ifdef PANIC
    *logofs << "DecompressPng16: PANIC! "
            << " Failed png_create_read_struct operation"
            << ".\n" << logofs_flush;
    #endif

    return -1;
  }

  infoPtr = png_create_info_struct(pngPtr);

  if (!infoPtr)
  {
    #ifdef PANIC
    *logofs << "DecompressPng16: PANIC! "
            << "Failed png_create_info_struct operation"
            << ".\n" << logofs_flush;
    #endif

    png_destroy_read_struct(&pngPtr, NULL, NULL);

    return -1;
  }

  if (setjmp(png_jmpbuf(pngPtr)))
  {
    #ifdef PANIC
    *logofs << "DecompressPng16: PANIC! "
            << "Error during IO initialization"
            << ".\n" << logofs_flush;
    #endif

    png_destroy_read_struct(&pngPtr, &infoPtr, NULL);

    return -1;
  }

  png_set_read_fn(pngPtr, (void *)compressedData, PngReadData);

  if (setjmp(png_jmpbuf(pngPtr)))
  {
    #ifdef PANIC
    *logofs << "DecompressPng16: PANIC! "
            << "Error during read of PNG header"
            << ".\n" << logofs_flush;
    #endif

    png_destroy_read_struct(&pngPtr, &infoPtr, NULL);

    return -1;
  }

  png_read_info(pngPtr, infoPtr);

  if (png_get_color_type(pngPtr, infoPtr) == PNG_COLOR_TYPE_PALETTE)
  {
    png_set_expand(pngPtr);
  }

  //
  // data points to dstBuf which is
  // already padded correctly for the final
  // image to put
  //

  data = dstBuf;
  rowPointers = (png_byte *) tmpBuf;

  //
  // We use setjmp() to save our context.
  // The PNG library will call longjmp()
  // in case of error.
  //

  if (setjmp(png_jmpbuf(pngPtr)))
  {
    #ifdef PANIC
    *logofs << "DecompressPng16: PANIC! "
            << "Error during read of PNG rows"
            << ".\n" << logofs_flush;
    #endif

    png_destroy_read_struct(&pngPtr, &infoPtr, NULL);

    return -1;
  }

  unsigned long pixel;

  for (dy = 0; dy < h; dy++)
  {
    png_read_row(pngPtr, rowPointers, NULL);

    for (dx = 0; dx < w; dx++)
    {
      pixel = RGB24_TO_PIXEL(16, tmpBuf[dx*3], tmpBuf[dx*3+1], tmpBuf[dx*3+2]);

      //
      // Follow the server byte order when arranging data.
      //
 
      if (byteOrder == LSBFirst)
      {
        data[0] = (unsigned char) (pixel & 0xff);
        data[1] = (unsigned char) ((pixel >> 8) & 0xff);
      }
      else
      {
        data[1] = (unsigned char) (pixel & 0xff);
        data[0] = (unsigned char) ((pixel >> 8) & 0xff);
      }
 
      data += 2; 
    }

    //
    // Move pixelPtr at the beginning of the
    // next line.
    //

    data = data + (RoundUp4(w * 2) - w * 2);
  }

  png_destroy_read_struct(&pngPtr, &infoPtr,NULL);

  #ifdef DEBUG
  *logofs << "DecompressPng16: Decompression finished."
          << dy << " lines handled.\n"
          << logofs_flush;
  #endif

  return 1;
}

int DecompressPng24(unsigned char *compressedData, int compressedLen,
                        unsigned int w, unsigned int h, unsigned char *dstBuf, int byteOrder)
{
  static CARD8 *pixelPtr = NULL;
  unsigned int dx, dy;

  png_structp pngPtr;
  png_infop   infoPtr;
  png_bytep   rowPointers;


  streamPos = 0;

  pngPtr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

  if (!pngPtr)
  {
    #ifdef PANIC
    *logofs << "DecompressPng24: PANIC! "
            << "Failed png_create_read_struct operation"
            << ".\n" << logofs_flush;
    #endif

    return -1;
  }

  infoPtr = png_create_info_struct(pngPtr);

  if (!infoPtr)
  {
    #ifdef PANIC
    *logofs << "DecompressPng24: PANIC! "
            << "Failed png_create_info_struct operation"
            << ".\n" << logofs_flush;
    #endif

    png_destroy_read_struct(&pngPtr, NULL, NULL);

    return -1;
  }

  if (setjmp(png_jmpbuf(pngPtr)))
  {
    #ifdef PANIC
    *logofs << "DecompressPng24: PANIC! "
            << "Error during IO initialization"
            << ".\n" << logofs_flush;
    #endif

    png_destroy_read_struct(&pngPtr, &infoPtr, NULL);

    return -1;
  }

  png_set_read_fn(pngPtr, (void *)compressedData, PngReadData);

  if (setjmp(png_jmpbuf(pngPtr)))
  {
    #ifdef PANIC
    *logofs << "DecompressPng24: PANIC! "
            << "Error during read of PNG header"
            << ".\n" << logofs_flush;
    #endif

    png_destroy_read_struct(&pngPtr, &infoPtr, NULL);

    return -1;
  }

  png_read_info( pngPtr, infoPtr ) ;

  if (png_get_color_type(pngPtr, infoPtr) == PNG_COLOR_TYPE_PALETTE)
  {
    png_set_expand(pngPtr);
  }

  //
  // PixelPtr points to dstBuf which is
  // already padded correctly for the final
  // image to put
  //

  pixelPtr = (CARD8 *) dstBuf;

  rowPointers = (png_byte *)tmpBuf;
  
  if (setjmp(png_jmpbuf(pngPtr)))
  {
    #ifdef PANIC
    *logofs << "DecompressPng24: PANIC! "
            << "Error during read of PNG rows"
            << ".\n" << logofs_flush;
    #endif

    png_destroy_read_struct(&pngPtr, &infoPtr, NULL);

    return -1;
  }

  for (dy = 0; dy < h; dy++)
  {
    png_read_row(pngPtr, rowPointers, NULL);

    for (dx = 0; dx < w; dx++)
    {
      //
      // Follow the server byte order when arranging data.
      //

      if (byteOrder == LSBFirst)
      {
        pixelPtr[0] = tmpBuf[dx * 3];
        pixelPtr[1] = tmpBuf[dx * 3 + 1];
        pixelPtr[2] = tmpBuf[dx * 3 + 2];
      }
      else
      {
        pixelPtr[2] = tmpBuf[dx * 3];
        pixelPtr[1] = tmpBuf[dx * 3 + 1];
        pixelPtr[0] = tmpBuf[dx * 3 + 2];
      }

      pixelPtr += 3;
    }

    //
    // Go to the next line.
    //

    pixelPtr = (CARD8 *) (((char *) pixelPtr) + (RoundUp4(w * 3) - w * 3));
  }

  png_destroy_read_struct(&pngPtr, &infoPtr,NULL);

  #ifdef DEBUG
  *logofs << "DecompressPng24: Decompression finished."
          << dy << " lines handled.\n"
          << logofs_flush;
  #endif

  return 1;
}

int DecompressPng32(unsigned char *compressedData, int compressedLen,
                         unsigned int w, unsigned int h, unsigned char *dstBuf, int byteOrder)
{
  unsigned char *data;

  unsigned int dx, dy;

  png_structp pngPtr;
  png_infop   infoPtr;
  png_bytep   rowPointers;

  streamPos = 0;

  pngPtr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

  if (!pngPtr)
  { 
    #ifdef PANIC
    *logofs << "DecompressPng32: PANIC! "
            << "Failed png_create_read_struct operation"
            << ".\n" << logofs_flush;
    #endif

    return -1;
  }

  infoPtr = png_create_info_struct(pngPtr);

  if (!infoPtr)
  { 
    #ifdef PANIC
    *logofs << "DecompressPng32: PANIC! "
            << "Failed png_create_info_struct operation."
            << ".\n" << logofs_flush;
    #endif
    
    png_destroy_read_struct(&pngPtr, NULL, NULL);

    return -1;
  }

  if (setjmp(png_jmpbuf(pngPtr)))
  {
    #ifdef PANIC
    *logofs << "DecompressPng32: PANIC! "
            << "Error during IO initialization"
            << ".\n" << logofs_flush;
    #endif

    png_destroy_read_struct(&pngPtr, &infoPtr, NULL);

    return -1;
  }

  png_set_read_fn(pngPtr, (void *)compressedData, PngReadData);
 
  if (setjmp(png_jmpbuf(pngPtr)))
  {
    #ifdef PANIC
    *logofs << "DecompressPng32: PANIC! "
            << "Error during read of PNG header"
            << ".\n" << logofs_flush;
    #endif

    png_destroy_read_struct(&pngPtr, &infoPtr, NULL);

    return -1;
  }

  png_read_info(pngPtr, infoPtr) ;


  if (png_get_color_type(pngPtr, infoPtr) == PNG_COLOR_TYPE_PALETTE)
  {
    png_set_expand(pngPtr);
  }

  //
  // data points to dstBuf which is
  // already padded correctly for the final
  // image to put
  //

  data = dstBuf;

  rowPointers = (png_byte *) tmpBuf;
  
  if (setjmp(png_jmpbuf(pngPtr)))
  {
    #ifdef PANIC
    *logofs << "DecompressPng32: PANIC! "
            << "Error during read of PNG rows"
            << ".\n" << logofs_flush;
    #endif

    png_destroy_read_struct(&pngPtr, &infoPtr, NULL);

    return -1;
  }

  unsigned long pixel;

  int i;

  for (dy = 0; dy < h; dy++)
  {
    png_read_row(pngPtr, rowPointers, NULL);

    for (dx = 0; dx < w; dx++)
    {
      pixel = RGB24_TO_PIXEL(32, tmpBuf[dx * 3], tmpBuf[dx * 3 + 1],
                                 tmpBuf[dx * 3 + 2]);

      //
      // Follow the server byte order when arranging data.
      //

      if (byteOrder == LSBFirst)
      {
        for (i = 0; i < 4; i++)
        {
          data[i] = (unsigned char)(pixel & 0xff);
          pixel >>= 8;
        }
      }
      else
      {
        for (i = 3; i >= 0; i--)
        {
          data[i] = (unsigned char) (pixel & 0xff);
          pixel >>= 8;
        }
      }

      data += 4;
    }
  }

  png_destroy_read_struct(&pngPtr, &infoPtr,NULL);

  #ifdef DEBUG
  *logofs << "DecompressPng32: Decompression finished."
          << dy << " lines handled.\n"
          << logofs_flush;
  #endif

  return 1;
}

static void PngReadData(png_structp png_ptr, png_bytep data, png_size_t length)
{
  memcpy((char *) data, (char *) png_get_io_ptr(png_ptr) + streamPos, length);

  streamPos += length;
}
