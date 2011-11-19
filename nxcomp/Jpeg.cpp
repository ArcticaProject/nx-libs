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

#include <X11/Xmd.h>

#include <unistd.h>
#include <setjmp.h>
#include <zlib.h>

#ifdef __cplusplus

extern "C"
{
  #include <stdio.h>
  #include <jpeglib.h>
}

#else

#include <stdio.h>
#include <jpeglib.h>

#endif

#include "Misc.h"
#include "Jpeg.h"
#include "Unpack.h"

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

#define RGB24_TO_PIXEL(bpp,r,g,b)                               \
   ((((CARD##bpp)(r) & 0xff) * srcRedMax + 127) / 255           \
    << srcRedShift |                                            \
    (((CARD##bpp)(g) & 0xff) * srcGreenMax + 127) / 255         \
    << srcGreenShift |                                          \
    (((CARD##bpp)(b) & 0xff) * srcBlueMax + 127) / 255          \
    << srcBlueShift)

#define RGB24_TO_PIXEL32(r,g,b)                                 \
  (((CARD32)(r) & 0xff) << srcRedShift |                        \
   ((CARD32)(g) & 0xff) << srcGreenShift |                      \
   ((CARD32)(b) & 0xff) << srcBlueShift)

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
// Local functions used for the jpeg decompression.
//

static void JpegSetSrcManager(j_decompress_ptr cinfo, CARD8 *compressedData, int compressedLen);
static void JpegInitSource(j_decompress_ptr cinfo);
static void JpegTermSource(j_decompress_ptr cinfo);
static void JpegSkipInputData(j_decompress_ptr cinfo, long num_bytes);

static boolean JpegFillInputBuffer(j_decompress_ptr cinfo);

static int DecompressJpeg16(unsigned char *compressedData, int compressedLen,
                                unsigned int w, unsigned int h, unsigned char *dstBuf, int byteOrder);

static int DecompressJpeg24(unsigned char *compressedData, int compressedLen,
                                unsigned int w, unsigned int h, unsigned char *dstBuf, int byteOrder);

static int DecompressJpeg32(unsigned char *compressedData, int compressedLen,
                                unsigned int w, unsigned int h, unsigned char *dstBuf, int byteOrder);

void UnpackJpegErrorHandler(j_common_ptr cinfo);

//
// Colormap stuff.
//

CARD16 srcRedMax, srcGreenMax, srcBlueMax;
CARD8  srcRedShift, srcGreenShift, srcBlueShift;

//
// Error handler.
//

static bool jpegError;

jmp_buf UnpackJpegContext;

void UnpackJpegErrorHandler(j_common_ptr cinfo)
{
  #ifdef PANIC
  *logofs << "UnpackJpegErrorHandler: PANIC! Detected error in JPEG decompression.\n"
          << logofs_flush;

  *logofs << "UnpackJpegErrorHandler: PANIC! Trying to revert to the previous context.\n"
          << logofs_flush;
  #endif

  jpegError = 1;

  longjmp(UnpackJpegContext, 1);
}

//
// Attributes used for the jpeg decompression.
//

static struct  jpeg_source_mgr jpegSrcManager;
static JOCTET  *jpegBufferPtr;
static size_t  jpegBufferLen;

static char    *tmpBuf;
static int     tmpBufSize = 0;

int UnpackJpeg(T_geometry *geometry, unsigned char method, unsigned char *srcData,
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
    *logofs << "UnpackJpeg: WARNING! Skipping unpack of dummy data.\n"
            << logofs_flush;
    #endif

    return -1;
  }

  #ifdef DEBUG
  *logofs << "UnpackJpeg: Decompression. Source size "
          << srcSize << " bits per plane " << dstBpp
          << ".\n" << logofs_flush;
  #endif

  srcRedShift   = ffs(geometry -> red_mask)   - 1;
  srcGreenShift = ffs(geometry -> green_mask) - 1;
  srcBlueShift  = ffs(geometry -> blue_mask)  - 1;

  #ifdef DEBUG
  *logofs << "UnpackJpeg: Red shift " << (int) srcRedShift
          << " green shift " << (int) srcGreenShift << " blue shift "
          << (int) srcBlueShift << ".\n" << logofs_flush;
  #endif

  srcRedMax   = geometry -> red_mask   >> srcRedShift;
  srcGreenMax = geometry -> green_mask >> srcGreenShift;
  srcBlueMax  = geometry -> blue_mask  >> srcBlueShift;

  #ifdef DEBUG
  *logofs << "UnpackJpeg: Red mask " << (void *) geometry -> red_mask
          << " green mask " << (void *) geometry -> green_mask
          << " blue mask " << (void *) geometry -> blue_mask
          << ".\n" << logofs_flush;
  #endif

  #ifdef DEBUG
  *logofs << "UnpackJpeg: Red max " << srcRedMax << " green max "
          << srcGreenMax << " blue max " << srcBlueMax
          << ".\n" << logofs_flush;
  #endif

  //
  // Make enough space in the temporary
  // buffer to have one complete row of
  // the image with 3 bytes for a pixel.
  //

  tmpBufSize = dstWidth * 3;
  tmpBuf     = new char[tmpBufSize];

  if (tmpBuf == NULL)
  {
    #ifdef PANIC
    *logofs << "UnpackJpeg: PANIC! Cannot allocate "
            << dstWidth * 3 << " bytes for Jpeg "
            << "decompressed data.\n" << logofs_flush;
    #endif

    delete [] tmpBuf;

    return -1;
  }

  int result = 1;

  switch(dstBpp)
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
        memcpy(dstBuff, srcBuff, dstWidth);

        dstBuff += RoundUp4(dstWidth);
        srcBuff += dstWidth;
      }

      break;
    }
    case 16:
    {
      result = DecompressJpeg16(srcData, srcSize, dstWidth,
                                    dstHeight, dstData, byteOrder);
      break;
    }
    case 24:
    {
      result = DecompressJpeg24(srcData, srcSize, dstWidth,
                                    dstHeight, dstData, byteOrder);
      break;
    }
    case 32:
    {
      result = DecompressJpeg32(srcData, srcSize, dstWidth,
                                    dstHeight, dstData, byteOrder);
      break;
    }
    default:
    {
      #ifdef PANIC
      *logofs << "UnpackJpeg: PANIC! Failed to decode Jpeg image. "
              << " Unsupported Bpp value " << dstBpp
              << " for the Jpeg compression"
              << ".\n" << logofs_flush;
      #endif

      delete [] tmpBuf;

      result = -1;
    }
  }

  #ifdef DEBUG
  *logofs << "UnpackJpeg: Decompression finished with result "
          << result << ".\n" << logofs_flush;
  #endif

  if (result == -1)
  {
    delete [] tmpBuf;

    #ifdef PANIC
    *logofs << "UnpackJpeg: PANIC! Failed to decode Jpeg image using "
            << dstBpp << " Bpp destination.\n" << logofs_flush;
    #endif

    return result;
  }

  //
  // Apply the correction for the brightness.
  //

  int maskMethod;

  switch(method)
  {
    case PACK_JPEG_8_COLORS:
    {
      maskMethod = MASK_8_COLORS;

      break;
    }
    case PACK_JPEG_64_COLORS:
    {
      maskMethod = MASK_64_COLORS;

      break;
    }
    case PACK_JPEG_256_COLORS:
    {
      maskMethod = MASK_256_COLORS;

      break;
    }
    case PACK_JPEG_512_COLORS:
    {
      maskMethod = MASK_512_COLORS;

      break;
    }
    case PACK_JPEG_4K_COLORS:
    {
      maskMethod = MASK_4K_COLORS;

      break;
    }
    case PACK_JPEG_32K_COLORS:
    {
      maskMethod = MASK_32K_COLORS;

      break;
    }
    case PACK_JPEG_64K_COLORS:
    {
      maskMethod = MASK_64K_COLORS;

      break;
    }
    case PACK_JPEG_256K_COLORS:
    {
      maskMethod = MASK_256K_COLORS;

      break;
    }
    case PACK_JPEG_2M_COLORS:
    {
      maskMethod = MASK_2M_COLORS;

      break;
    }
    case PACK_JPEG_16M_COLORS:
    {
      maskMethod = MASK_16M_COLORS;

      break;
    }
    default:
    {
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
      Unpack32To32(colorMask, (unsigned int *) dstBuff, (unsigned int *) dstBuff,
                       (unsigned int *) (dstBuff + dstSize));
      break;
    }
    default:
    {
      delete [] tmpBuf;

      return -1;
    }
  }

  delete [] tmpBuf;

  return 1;
}

//
// Functions that actually do the Jpeg decompression.
//

int DecompressJpeg16(unsigned char *compressedData, int compressedLen,
                         unsigned int w, unsigned int h, unsigned char *dstBuf, int byteOrder)
{
  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr jerr;
  unsigned char *data;
  JSAMPROW rowPointer[1];

  unsigned int dx = 0;
  unsigned int dy = 0;

  #ifdef DEBUG
  *logofs << "DecompressJpeg16: Decompressing with length "
          << compressedLen << " width " << w << " height "
          << h << ".\n" << logofs_flush;
  #endif

  jpegError = 0;

  cinfo.err = jpeg_std_error(&jerr);

  jerr.error_exit = UnpackJpegErrorHandler;

  if (setjmp(UnpackJpegContext) == 1)
  {
    #ifdef TEST
    *logofs << "DecompressJpeg16: Out of the long jump with error '"
            << jpegError << "'.\n" << logofs_flush;
    #endif

    goto AbortDecompressJpeg16;
  }

  jpeg_create_decompress(&cinfo);

  if (jpegError) goto AbortDecompressJpeg16;

  JpegSetSrcManager(&cinfo, compressedData, compressedLen);

  jpeg_read_header(&cinfo, 1);

  if (jpegError) goto AbortDecompressJpeg16;

  cinfo.out_color_space = JCS_RGB;

  jpeg_start_decompress(&cinfo);

  if (jpegError) goto AbortDecompressJpeg16;

  if (cinfo.output_width != w ||
          cinfo.output_height != h ||
              cinfo.output_components != 3)
  {
    #ifdef PANIC
    *logofs << "DecompressJpeg16: PANIC! Wrong JPEG data received.\n"
            << logofs_flush;
    #endif

    jpeg_destroy_decompress(&cinfo);

    return -1;
  }

  //
  // PixelPtr points to dstBuf which is
  // already padded correctly for the final
  // image to put
  //

  data = dstBuf;

  rowPointer[0] = (JSAMPROW) tmpBuf;

  unsigned long pixel;

  while (cinfo.output_scanline < cinfo.output_height)
  {
    jpeg_read_scanlines(&cinfo, rowPointer, 1);

    if (jpegError) goto AbortDecompressJpeg16;

    for (dx = 0; dx < w; dx++)
    {
      pixel = RGB24_TO_PIXEL(16, tmpBuf[dx * 3], tmpBuf[dx * 3 + 1],
                                 tmpBuf[dx * 3 + 2]);

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
    // Move data at the beginning of the
    // next line.
    //

    data = data + (RoundUp4(w * 2) - w * 2);

    dy++;
  }

  AbortDecompressJpeg16:

  if (jpegError == 0)
  {
    jpeg_finish_decompress(&cinfo);
  }

  jpeg_destroy_decompress(&cinfo);

  if (jpegError == 1)
  {
    #ifdef PANIC
    *logofs << "DecompressJpeg16: Failed to decompress JPEG image.\n"
            << logofs_flush;
    #endif

    return -1;
  }

  #ifdef TEST
  *logofs << "DecompressJpeg16: Decompression finished with "
          << dy << " lines handled.\n" << logofs_flush;
  #endif

  return 1;
}

int DecompressJpeg24(unsigned char *compressedData, int compressedLen,
                         unsigned int w, unsigned int h, unsigned char *dstBuf, int byteOrder)
{
  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr jerr;
  CARD8 *pixelPtr = NULL;
  JSAMPROW rowPointer[1];

  unsigned int dx = 0;
  unsigned int dy = 0;

  #ifdef TEST
  *logofs << "DecompressJpeg24: Decompressing with length "
          << compressedLen << " width " << w << " height "
          << h << ".\n" << logofs_flush;
  #endif

  jpegError = 0;

  cinfo.err = jpeg_std_error(&jerr);

  jerr.error_exit = UnpackJpegErrorHandler;

  if (setjmp(UnpackJpegContext) == 1)
  {
    #ifdef TEST
    *logofs << "DecompressJpeg24: Out of the long jump with error '"
            << jpegError << "'.\n" << logofs_flush;
    #endif

    goto AbortDecompressJpeg24;
  }

  jpeg_create_decompress(&cinfo);

  if (jpegError) goto AbortDecompressJpeg24;

  JpegSetSrcManager(&cinfo, compressedData, compressedLen);

  jpeg_read_header(&cinfo, 1);

  if (jpegError) goto AbortDecompressJpeg24;

  cinfo.out_color_space = JCS_RGB;

  jpeg_start_decompress(&cinfo);

  if (jpegError) goto AbortDecompressJpeg24;

  if (cinfo.output_width != w ||
          cinfo.output_height != h ||
              cinfo.output_components != 3)
  {
    #ifdef PANIC
    *logofs << "DecompressJpeg24: PANIC! Wrong JPEG data received.\n"
            << logofs_flush;
    #endif

    jpeg_destroy_decompress(&cinfo);

    return -1;
  }

  //
  // PixelPtr points to dstBuf which is
  // already padded correctly for the final
  // image to put.
  //

  pixelPtr = (CARD8 *) dstBuf;

  rowPointer[0] = (JSAMPROW) tmpBuf;

  while (cinfo.output_scanline < cinfo.output_height)
  {
    jpeg_read_scanlines(&cinfo, rowPointer, 1);

    if (jpegError) goto AbortDecompressJpeg24;

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

    dy++;
  }

  AbortDecompressJpeg24:

  if (jpegError == 0)
  {
    jpeg_finish_decompress(&cinfo);
  }

  jpeg_destroy_decompress(&cinfo);

  if (jpegError == 1)
  {
    #ifdef PANIC
    *logofs << "DecompressJpeg24: Failed to decompress JPEG image.\n"
            << logofs_flush;
    #endif

    return -1;
  }

  #ifdef TEST
  *logofs << "DecompressJpeg24: Decompression finished with "
          << dy << " lines handled.\n" << logofs_flush;
  #endif

  return 1;
}

int DecompressJpeg32(unsigned char *compressedData, int compressedLen,
                         unsigned int w, unsigned int h, unsigned char *dstBuf, int byteOrder)
{
  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr jerr;
  unsigned char *data;
  JSAMPROW rowPointer[1];

  unsigned int dx = 0;
  unsigned int dy = 0;

  #ifdef TEST
  *logofs << "DecompressJpeg32: Decompressing with length "
          << compressedLen << " width " << w << " height "
          << h << ".\n" << logofs_flush;
  #endif

  jpegError = 0;

  cinfo.err = jpeg_std_error(&jerr);

  jerr.error_exit = UnpackJpegErrorHandler;

  if (setjmp(UnpackJpegContext) == 1)
  {
    #ifdef TEST
    *logofs << "DecompressJpeg32: Out of the long jump with error '"
            << jpegError << "'.\n" << logofs_flush;
    #endif

    goto AbortDecompressJpeg32;
  }

  jpeg_create_decompress(&cinfo);

  if (jpegError) goto AbortDecompressJpeg32;

  JpegSetSrcManager(&cinfo, compressedData, compressedLen);

  jpeg_read_header(&cinfo, 1);

  if (jpegError) goto AbortDecompressJpeg32;

  cinfo.out_color_space = JCS_RGB;

  jpeg_start_decompress(&cinfo);

  if (jpegError) goto AbortDecompressJpeg32;

  if (cinfo.output_width != w ||
          cinfo.output_height != h ||
              cinfo.output_components != 3)
  {
    #ifdef PANIC
    *logofs << "DecompressJpeg32 : PANIC! Wrong JPEG data received.\n"
            << logofs_flush;
    #endif

    jpeg_destroy_decompress(&cinfo);

    return -1;
  }

  //
  // PixelPtr points to dstBuf which is
  // already padded correctly for the final
  // image to put
  //

  data = dstBuf;

  rowPointer[0] = (JSAMPROW) tmpBuf;
  
  unsigned long pixel;

  int i;

  while (cinfo.output_scanline < cinfo.output_height)
  {
    jpeg_read_scanlines(&cinfo, rowPointer, 1);

    if (jpegError) goto AbortDecompressJpeg32;

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

    dy++;
  }

  AbortDecompressJpeg32:

  if (jpegError == 0)
  {
    jpeg_finish_decompress(&cinfo);
  }

  jpeg_destroy_decompress(&cinfo);

  if (jpegError == 1)
  {
    #ifdef PANIC
    *logofs << "DecompressJpeg32: Failed to decompress JPEG image.\n"
            << logofs_flush;
    #endif

    return -1;
  }

  #ifdef TEST
  *logofs << "DecompressJpeg32: Decompression finished with "
          << dy << " lines handled.\n" << logofs_flush;
  #endif

  return 1;
}

static void JpegInitSource(j_decompress_ptr cinfo)
{
  jpegError = 0;
}

static boolean JpegFillInputBuffer(j_decompress_ptr cinfo)
{
  jpegError = 1;

  jpegSrcManager.bytes_in_buffer = jpegBufferLen;
  jpegSrcManager.next_input_byte = (JOCTET *)jpegBufferPtr;

  return 1;
}

static void JpegSkipInputData(j_decompress_ptr cinfo, long num_bytes)
{
  if (num_bytes < 0 || (unsigned long) num_bytes > jpegSrcManager.bytes_in_buffer)
  {
    jpegError = 1;

    jpegSrcManager.bytes_in_buffer = jpegBufferLen;
    jpegSrcManager.next_input_byte = (JOCTET *)jpegBufferPtr;
  }
  else
  {
    jpegSrcManager.next_input_byte += (size_t) num_bytes;
    jpegSrcManager.bytes_in_buffer -= (size_t) num_bytes;
  }
}

static void JpegTermSource(j_decompress_ptr cinfo)
{
}

static void JpegSetSrcManager(j_decompress_ptr cinfo,
                                  CARD8 *compressedData,
                                      int compressedLen)
{
  jpegBufferPtr = (JOCTET *) compressedData;
  jpegBufferLen = (size_t) compressedLen;

  jpegSrcManager.init_source       = JpegInitSource;
  jpegSrcManager.fill_input_buffer = JpegFillInputBuffer;
  jpegSrcManager.skip_input_data   = JpegSkipInputData;
  jpegSrcManager.resync_to_restart = jpeg_resync_to_restart;
  jpegSrcManager.term_source       = JpegTermSource;
  jpegSrcManager.next_input_byte   = jpegBufferPtr;
  jpegSrcManager.bytes_in_buffer   = jpegBufferLen;

  cinfo->src = &jpegSrcManager;
}
