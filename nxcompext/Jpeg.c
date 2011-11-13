/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2007 NoMachine, http://www.nomachine.com/.         */
/*                                                                        */
/* NXCOMPEXT, NX protocol compression and NX extensions to this software  */
/* are copyright of NoMachine. Redistribution and use of the present      */
/* software is allowed according to terms specified in the file LICENSE   */
/* which comes in the source distribution.                                */
/*                                                                        */
/* Check http://www.nomachine.com/licensing.html for applicability.       */
/*                                                                        */
/* NX and NoMachine are trademarks of NoMachine S.r.l.                    */
/*                                                                        */
/* All rigths reserved.                                                   */
/*                                                                        */
/**************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "X11/X.h"
#include "X11/Xlib.h"
#include "X11/Xmd.h"

#include <jpeglib.h>

#include "NXlib.h"

#include "Mask.h"
#include "Jpeg.h"

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

#define JPEG_DEST_SIZE(width, height) ((width) * 3 * (height) + 1024)

/*
 * Local function prototypes.
 */

static void PrepareRowForJpeg(CARD8 *dst, int y, int count);
static void PrepareRowForJpeg24(CARD8 *dst, int y, int count);
static void PrepareRowForJpeg16(CARD8 *dst, int y, int count);
static void PrepareRowForJpeg32(CARD8 *dst, int y, int count);

static int JpegEmptyOutputBuffer(j_compress_ptr cinfo);

static void JpegInitDestination(j_compress_ptr cinfo);
static void JpegTermDestination(j_compress_ptr cinfo);
static void JpegSetDstManager(j_compress_ptr cinfo);

/*
 * Quality levels.
 */

static int jpegQuality[10] = {20, 30, 40, 50, 55, 60, 65, 70, 75, 80};

/*
 * Image characteristics.
 */

static int bytesPerLine;

static CARD8 bitsPerPixel;
static CARD16 redMax, greenMax, blueMax;
static CARD8  redShift, greenShift, blueShift;
static int byteOrder;

/*
 * Other variables used for the Jpeg
 * encoding.
 */

static char *jpegBeforeBuf = NULL;
static char *jpegCompBuf;
static int  jpegCompBufSize;
static int  jpegError;
static int  jpegDstDataLen;

static struct jpeg_destination_mgr jpegDstManager;

/*
 * Just for debugging purpose.
 */

#ifdef DEBUG

static int  jpegId;
static char jpegName[10];
static FILE *jpegFile;

#endif

/*
 * Function declarations
 */

char *JpegCompressData(XImage *image, int level, int *compressed_size)
{
  struct jpeg_compress_struct cinfo;
  struct jpeg_error_mgr jerr;

  CARD8 *srcBuf;
  JSAMPROW rowPointer[1];

  int dy, w, h;

  *compressed_size = 0;

  /*
   * Initialize the image stuff
   */

  bitsPerPixel = image -> bits_per_pixel;
  bytesPerLine = image -> bytes_per_line;
  byteOrder = image -> byte_order;

  #ifdef TEST
  fprintf(stderr, "******JpegCompressData: Image byte order [%d] bitmap bit order [%d].\n",
              image -> byte_order, image -> bitmap_bit_order);

  fprintf(stderr, "******JpegCompressData: Bits per pixel [%d] bytes per line [%d].\n",
              bitsPerPixel, bytesPerLine);
  #endif

  redShift   = FindLSB(image -> red_mask)   - 1;
  greenShift = FindLSB(image -> green_mask) - 1;
  blueShift  = FindLSB(image -> blue_mask)  - 1;

  #ifdef TEST
  fprintf(stderr, "******JpegCompressData: Red mask [0x%lx] green mask [0x%lx] blue mask [0x%lx].\n",
              image -> red_mask, image -> green_mask, image -> blue_mask);

  fprintf(stderr, "******JpegCompressData: Red shift [%d] green shift [%d] blue shift [%d].\n",
              redShift, greenShift, blueShift);
  #endif

  redMax   = image -> red_mask   >> redShift;
  greenMax = image -> green_mask >> greenShift;
  blueMax  = image -> blue_mask  >> blueShift;

  #ifdef TEST
  fprintf(stderr, "******JpegCompressData: Red max [0x%x] green max [0x%x] blue max [0x%x].\n",
              redMax, greenMax, blueMax);
  #endif

  w = image -> width;
  h = image -> height;

  jpegBeforeBuf = image -> data;

  #ifdef DEBUG
  fprintf(stderr, "******JpegCompressData: Width [%d] height [%d] level [%d].\n",
              w, h, level);
  #endif

  if (bitsPerPixel == 1 ||
          bitsPerPixel == 8)
  {
    #ifdef PANIC
    fprintf(stderr, "******JpegCompressData: PANIC! Invalid bits per pixel [%d].\n",
                bitsPerPixel);
    #endif

    return NULL;
  }

  /*
   * Allocate space for one line of the
   * resulting image, 3 bytes per pixel.
   */

  #ifdef DEBUG
  fprintf(stderr, "******JpegCompressData: Allocating [%d] bytes for the scanline.\n",
              w * 3);
  #endif

  srcBuf = (CARD8 *) malloc(w * 3);

  if (srcBuf == NULL)
  {
    #ifdef PANIC
    fprintf(stderr, "******JpegCompressData: PANIC! Cannot allocate [%d] bytes.\n",
                w * 3);
    #endif

    return NULL;
  }

  rowPointer[0] = srcBuf;

  cinfo.err = jpeg_std_error(&jerr);

  jpeg_create_compress(&cinfo);

  cinfo.image_width = w;
  cinfo.image_height = h;
  cinfo.input_components = 3;
  cinfo.in_color_space = JCS_RGB;

  jpeg_set_defaults(&cinfo);
  jpeg_set_quality(&cinfo, jpegQuality[level], 1);

  /*
   * Allocate memory for the destination
   * buffer.
   */

  jpegCompBufSize = JPEG_DEST_SIZE(w, h);

  #ifdef TEST
  fprintf(stderr, "******JpegCompressData: Allocating [%d] bytes for the destination data.\n",
              jpegCompBufSize);
  #endif

  jpegCompBuf = malloc(jpegCompBufSize);

  if (jpegCompBuf == NULL)
  {
    #ifdef PANIC
    fprintf(stderr, "******JpegCompressData: PANIC! Error allocating [%d] bytes for the Jpeg data.\n",
                jpegCompBufSize);
    #endif

    return NULL;
  }

  JpegSetDstManager(&cinfo);

  jpeg_start_compress(&cinfo, 1);

  #ifdef DEBUG
  fprintf(stderr, "******JpegCompressedData: Initialization finished.\n");
  #endif

  for (dy = 0; dy < h; dy++)
  {
    PrepareRowForJpeg(srcBuf, dy, w);

    jpeg_write_scanlines(&cinfo, rowPointer, 1);

    if (jpegError != 0)
    {
      break;
    }
  }

  #ifdef DEBUG
  fprintf(stderr, "******JpegCompressedData: Compression finished. Lines handled [%d,%d]. Error is [%d].\n",
              dy, h, jpegError);
  #endif

  if (jpegError == 0)
  {
    jpeg_finish_compress(&cinfo);
  }

  jpeg_destroy_compress(&cinfo);

  free((char *) srcBuf);

  if (jpegError != 0)
  {
    #ifdef PANIC
    fprintf(stderr, "******JpegCompressedData: PANIC! Compression failed. Error is [%d].\n",
                jpegError);
    #endif

    free(jpegCompBuf);

    return NULL;
  }

  /*
   * Check the size of the resulting data.
   */

  if (jpegDstDataLen > 0)
  {
    /*
     * Save the image on disk to help with
     * the debug.
     */

    #ifdef DEBUG

    int i = 0;

    fprintf(stderr, "******JpegCompressedData: Compressed size [%d].\n",
                jpegDstDataLen);

    jpegId++;

    sprintf(jpegName, "jpeg%d", jpegId);

    jpegFile = fopen(jpegName, "w");

    for (i = 0; i < jpegDstDataLen; i++)
    {
      fprintf(jpegFile, "%c", *(jpegCompBuf + i));
    }

    fclose(jpegFile);

    #endif

    *compressed_size = jpegDstDataLen;

    return jpegCompBuf;
  }
  else
  {
    #ifdef PANIC
    fprintf(stderr, "******JpegCompressedData: PANIC! Invalid size of the compressed data [%d].\n",
                jpegDstDataLen);
    #endif

    free(jpegCompBuf);

    return NULL;
  }
}

void PrepareRowForJpeg(CARD8 *dst, int y, int count)
{
  if (bitsPerPixel == 32)
  {
    if (redMax == 0xff &&
            greenMax == 0xff &&
                blueMax == 0xff)
    {
      PrepareRowForJpeg24(dst, y, count);
    }
    else
    {
      PrepareRowForJpeg32(dst, y, count);
    }
  }
  else if (bitsPerPixel == 24)
  {
    memcpy(dst, jpegBeforeBuf + y * bytesPerLine, count * 3);
  }
  else
  {
    /*
     * 16 bpp assumed.
     */

    PrepareRowForJpeg16(dst, y, count);
  }
}

void PrepareRowForJpeg24(CARD8 *dst, int y, int count)
{
  CARD8 *fbptr;
  CARD32 pix;

  fbptr = (CARD8 *) (jpegBeforeBuf + y * bytesPerLine);

  while (count--)
  {
    if (byteOrder == LSBFirst)
    {
      pix = (CARD32) *(fbptr + 2);
      pix = (pix << 8) | (CARD32) *(fbptr+1);
      pix = (pix << 8) | (CARD32) *fbptr;
    }
    else
    {
      pix = (CARD32) *(fbptr + 1);
      pix = (pix << 8) | (CARD32) *(fbptr + 2);
      pix = (pix << 8) | (CARD32) *(fbptr + 3);
    }

    *dst++ = (CARD8)(pix >> redShift);
    *dst++ = (CARD8)(pix >> greenShift);
    *dst++ = (CARD8)(pix >> blueShift);

    fbptr+=4;
  }
}

#define DEFINE_JPEG_GET_ROW_FUNCTION(bpp)                                   \
                                                                            \
void PrepareRowForJpeg##bpp(CARD8 *dst, int y, int count)                   \
{                                                                           \
  CARD8 *fbptr;                                                             \
  CARD##bpp pix;                                                            \
  int inRed, inGreen, inBlue;                                               \
  int i;                                                                    \
                                                                            \
  fbptr = (CARD8 *) (jpegBeforeBuf + y * bytesPerLine);                     \
                                                                            \
  while (count--)                                                           \
  {                                                                         \
    pix = 0;                                                                \
                                                                            \
    if (byteOrder == LSBFirst)                                              \
    {                                                                       \
      for (i = (bpp >> 3) - 1; i >= 0; i--)                                 \
      {                                                                     \
        pix = (pix << 8) | (CARD32) *(fbptr + i);                           \
      }                                                                     \
    }                                                                       \
    else                                                                    \
    {                                                                       \
      for (i = 0; i < (bpp >> 3); i++)                                      \
      {                                                                     \
        pix = (pix << 8) | (CARD32) *(fbptr + i);                           \
      }                                                                     \
    }                                                                       \
                                                                            \
    fbptr += bpp >> 3;                                                      \
                                                                            \
    inRed = (int)                                                           \
            (pix >> redShift   & redMax);                                   \
    inGreen = (int)                                                         \
            (pix >> greenShift & greenMax);                                 \
    inBlue  = (int)                                                         \
            (pix >> blueShift  & blueMax);                                  \
                                                                            \
    *dst++ = (CARD8)((inRed   * 255 + redMax / 2) /                         \
                         redMax);                                           \
    *dst++ = (CARD8)((inGreen * 255 + greenMax / 2) /                       \
                         greenMax);                                         \
    *dst++ = (CARD8)((inBlue  * 255 + blueMax / 2) /                        \
                         blueMax);                                          \
  }                                                                         \
}

DEFINE_JPEG_GET_ROW_FUNCTION(16)
DEFINE_JPEG_GET_ROW_FUNCTION(32)

/*
 * Destination manager implementation for JPEG library.
 */

void JpegInitDestination(j_compress_ptr cinfo)
{
  jpegError = 0;

  jpegDstManager.next_output_byte = (JOCTET *) jpegCompBuf;
  jpegDstManager.free_in_buffer = (size_t) jpegCompBufSize;
}

int JpegEmptyOutputBuffer(j_compress_ptr cinfo)
{
  jpegError = 1;

  jpegDstManager.next_output_byte = (JOCTET *) jpegCompBuf;
  jpegDstManager.free_in_buffer = (size_t) jpegCompBufSize;

  return 1;
}

void JpegTermDestination(j_compress_ptr cinfo)
{
  jpegDstDataLen = jpegCompBufSize - jpegDstManager.free_in_buffer;
}

void JpegSetDstManager(j_compress_ptr cinfo)
{
  jpegDstManager.init_destination = JpegInitDestination;
  jpegDstManager.empty_output_buffer = JpegEmptyOutputBuffer;
  jpegDstManager.term_destination = JpegTermDestination;

  cinfo -> dest = &jpegDstManager;
}

