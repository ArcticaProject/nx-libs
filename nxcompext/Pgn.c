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

#include "Xutil.h"

#include "NXlib.h"

#include "Mask.h"
#include "Pgn.h"

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

/*
 * Selected ZLIB compression level.
 */

#define PNG_Z_LEVEL   4

/*
 * Local function prototypes.
 */

static void PrepareRowForPng(CARD8 *dst, int y, int count);
static void PrepareRowForPng24(CARD8 *dst, int y, int count);
static void PrepareRowForPng16(CARD8 *dst, int y, int count);
static void PrepareRowForPng32(CARD8 *dst, int y, int count);

static void PngWriteData(png_structp png_ptr, png_bytep data, png_size_t length);
static void PngFlushData(png_structp png_ptr);

/*
 * Image characteristics.
 */

static int bytesPerLine;
static int byteOrder;

static CARD8 bitsPerPixel;
static CARD16 redMax, greenMax, blueMax;
static CARD8 redShift, greenShift, blueShift;

/*
 * Other variables used for the Png
 * encoding.
 */

png_byte    color_type;
png_structp png_ptr;
png_infop   info_ptr;
png_colorp  palette;
static char *pngCompBuf;
static int  pngDataLen;
static char *pngBeforeBuf = NULL;

/*
 * Allocate data for the compressed image.
 * We need to ensure that there is enough
 * space to include the palette and the
 * header.
 */

#define PNG_DEST_SIZE(width, height) ((width) * 3 * (height) + 1024 + 256)

/*
 * Just for debug purposes.
 */

#ifdef DEBUG

static int  pngId;
static char pngName[10];
static FILE *pngFile;

#endif

int PngCompareColorTable(NXColorTable *c1, NXColorTable *c2)
{
  return (c1 -> pixel - c2 -> pixel);
}

#define NB_COLOR_MAX 256

int NXCreatePalette32(XImage *src_image, NXColorTable *color_table, CARD8 *image_index, int nb_max)
{
  int    x, y, t, p;
  CARD8 *fbptr;
  CARD32 pixel;

  fbptr = (CARD8 *) (src_image -> data);

  /*
   * TODO: Find a more intelligent way to
   * estimate the number of colors.
   */

  memset(color_table, 0, nb_max * sizeof(NXColorTable));

  for (x = 0, p = 0; x < src_image -> height; x++)
  {
    for (y = 0; y < src_image -> width; y++)
    {
      if (byteOrder == LSBFirst)
      {
        pixel = (CARD32) *(fbptr + 3);
        pixel = (pixel << 8) | (CARD32) *(fbptr + 2);
        pixel = (pixel << 8) | (CARD32) *(fbptr + 1);
        pixel = (pixel << 8) | (CARD32) *fbptr;
      }
      else
      {
        pixel = (CARD32) *fbptr;
        pixel = (pixel << 8) | (CARD32) *(fbptr + 1);
        pixel = (pixel << 8) | (CARD32) *(fbptr + 2);
        pixel = (pixel << 8) | (CARD32) *(fbptr + 3);
      }

      fbptr += 4;

      for (t = 0; t < nb_max; t++)
      {
        if (color_table[t].found == 0)
        {
          color_table[t].pixel =  pixel;
          color_table[t].found =  1;
          p++;
          image_index[((x * src_image -> width) + y)] = t;

          break;
        }
        else if ((CARD32)(color_table[t].pixel) == pixel)
        {
          image_index[((x * src_image -> width) + y)] = t;

          break;
        }
      }

      if (p == nb_max)
      {
        return nb_max + 1;
      }
    }
  }
  return p;
}

int NXCreatePalette16(XImage *src_image, NXColorTable *color_table, CARD8 *image_index, int nb_max)
{
  int    x, y, t, p;
  CARD8 *fbptr;
  CARD16 pixel;

  fbptr = (CARD8 *) (src_image -> data);

  /*
   * TODO: Find a more intelligent way to
   * estimate the number of colors.
   */

  memset(color_table, 0, nb_max * sizeof(NXColorTable));

  for (x = 0, p = 0; x < src_image -> height; x++)
  {
    for (y = 0; y < src_image -> width; y++)
    {
      if (byteOrder == LSBFirst)
      {
        pixel = (CARD16) *(fbptr + 1);
        pixel = (pixel << 8) | (CARD16) *fbptr;
      }
      else
      {
        pixel = (CARD16) *fbptr;
        pixel = (pixel << 8) | (CARD16) *(fbptr + 1);
      }

      fbptr += 2;

      for (t = 0; t < nb_max; t++)
      {
        if (color_table[t].found == 0)
        {
          color_table[t].pixel =  pixel;
          color_table[t].found =  1;
          p++;
          image_index[((x * src_image -> width) + y)] = t;

          break;
        }
        else if ((color_table[t].pixel) == pixel)
        {
          image_index[((x * src_image -> width) + y)] = t;

          break;
        }
      }

      /*
       * In case the number of 16bit words is not even
       * we have 2 padding bytes that we have to skip.
       */

      if ((y == src_image -> width - 1) && (src_image -> width % 2 == 1)) fbptr += 2;

      if (p == nb_max)
      {
        return nb_max + 1;
      }
    }
  }

  return p;
}

char *PngCompressData(XImage *image, int *compressed_size)
{
  unsigned int num = 0;
  CARD8        *srcBuf;

  int dy, w, h;

  int nb_colors;

  NXColorTable color_table[NB_COLOR_MAX];
  CARD8       *image_index;

  image_index = (CARD8 *) malloc((image -> height) * (image -> width) * sizeof(CARD8));

  /*
   * TODO: Be sure the padded bytes are cleaned.
   * It would be better to set to zero the bytes
   * that are not alligned to the word boundary
   * at the end of the procedure.
   */

  memset(image_index, 0, (image -> height) * (image -> width) * sizeof(CARD8));

  *compressed_size = 0;

  pngDataLen = 0;

  /*
   * Initialize the image stuff.
   */

  bitsPerPixel = image -> bits_per_pixel;
  bytesPerLine = image -> bytes_per_line;
  byteOrder = image -> byte_order;

  if (bitsPerPixel < 15)
  {
    #ifdef PANIC
    fprintf(stderr, "******PngCompressData: PANIC! Can't compress images with [%d] bits per pixel.\n",
                bitsPerPixel);
    #endif

    return NULL;
  }

  redShift   = FindLSB(image -> red_mask)   - 1;
  greenShift = FindLSB(image -> green_mask) - 1;
  blueShift  = FindLSB(image -> blue_mask)  - 1;

  redMax   = image -> red_mask   >> redShift;
  greenMax = image -> green_mask >> greenShift;
  blueMax  = image -> blue_mask  >> blueShift;

  w             = image -> width;
  h             = image -> height;
  pngBeforeBuf  = image -> data;

  #ifdef DEBUG
  fprintf(stderr, "******PngCompressData: Compressing image with width [%d] height [%d].\n",
              w, h );
  #endif

  /*
   * Initialize the PNG stuff.
   */

  png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

  if (png_ptr == NULL)
  {
    #ifdef PANIC
    fprintf(stderr, "******PngCompressData: PANIC! Failed creating the png_create_write_struct.\n");
    #endif

    return NULL;
  }

  info_ptr = png_create_info_struct(png_ptr);

  if (info_ptr == NULL)
  {
    #ifdef PANIC
    fprintf(stderr, "******PngCompressData: PANIC! Failed creating the png_create_info_struct.\n");
    #endif

    png_destroy_write_struct(&png_ptr, NULL);

    return NULL;
  }

  if (setjmp(png_jmpbuf(png_ptr)))
  {
    #ifdef PANIC
    fprintf(stderr, "******PngCompressData: PANIC! Error during compression initialization.\n");
    #endif

    png_destroy_write_struct(&png_ptr, &info_ptr);

    return NULL;
  }

  /*
   * Be sure we allocate enough data.
   */

  #ifdef TEST
  fprintf(stderr, "******PngCompressData: Allocating [%d] bytes for the destination data.\n",
              PNG_DEST_SIZE(w, h));
  #endif

  pngCompBuf = malloc(PNG_DEST_SIZE(w, h));

  if (pngCompBuf == NULL)
  {
    #ifdef PANIC
    fprintf(stderr, "******PngCompressData: PANIC! Error allocating [%d] bytes for the Png data.\n",
                PNG_DEST_SIZE(w, h));
    #endif

    return NULL;
  }

  png_set_write_fn(png_ptr, (void *) pngCompBuf, PngWriteData, PngFlushData);

  if (setjmp(png_jmpbuf(png_ptr)))
  {
    #ifdef PANIC
    fprintf(stderr, "******PngCompressData: PANIC! Error writing the header.\n");
    #endif

    png_destroy_write_struct(&png_ptr, &info_ptr);

    free(pngCompBuf);

    return NULL;
  }

  png_set_compression_level(png_ptr, PNG_Z_LEVEL);

  if (bitsPerPixel == 16)
  {
    nb_colors = NXCreatePalette16(image, color_table, image_index, NB_COLOR_MAX);
  }
  else
  {
    nb_colors = NXCreatePalette32(image, color_table, image_index, NB_COLOR_MAX);
  }

  if (nb_colors <= NB_COLOR_MAX)
  {
    color_type = PNG_COLOR_TYPE_PALETTE;
  }
  else
  {
    color_type = PNG_COLOR_TYPE_RGB;
  }

  png_set_IHDR(png_ptr, info_ptr, w, h,
                   8, color_type, PNG_INTERLACE_NONE,
                       PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

  if (color_type == PNG_COLOR_TYPE_PALETTE)
  {
    palette = png_malloc(png_ptr, sizeof(*palette) * 256);

    /*
     * TODO: Do we need to clean these bytes?
     *
     * memset(palette, 0, sizeof(*palette) * 256);
     */

    for (num = 0; num < 256 && color_table[num].found != 0; num++)
    {
      if (bitsPerPixel == 24)
      {
        palette[num].red = (color_table[num].pixel >> redShift) & redMax;
        palette[num].green = (color_table[num].pixel >> greenShift) & greenMax;
        palette[num].blue = color_table[num].pixel >> blueShift & blueMax;
      }
      else
      {
        int inRed, inGreen, inBlue;

        inRed = (color_table[num].pixel >> redShift) & redMax;
        inGreen = (color_table[num].pixel >> greenShift) & greenMax;
        inBlue = color_table[num].pixel >> blueShift & blueMax;

        palette[num].red = (CARD8)((inRed * 255 + redMax / 2) / redMax);
        palette[num].green = (CARD8)((inGreen * 255 + greenMax / 2) / greenMax);
        palette[num].blue = (CARD8)((inBlue * 255 + blueMax / 2) / blueMax);
      }

      #ifdef DEBUG
      fprintf(stderr, "******PngCompressData: pixel[%d] r[%d] g[%d] b[%d].\n",
                  (int) color_table[num].pixel,palette[num].red,palette[num].green,palette[num].blue);
      #endif
    }

    png_set_PLTE(png_ptr, info_ptr, palette, num);

    #ifdef DEBUG
    fprintf(stderr, "******PngCompressedData: Setting palette.\n");
    #endif
  }

  /*
   * End of palette.
   */

  png_write_info(png_ptr, info_ptr);

  /*
   * Allocate space for one line of
   * the image, 3 bytes per pixel.
   */

  #ifdef DEBUG
  fprintf(stderr, "******PngCompressedData: Initialization finished.\n");
  #endif

  if (setjmp(png_jmpbuf(png_ptr)))
  {
    #ifdef PANIC
    fprintf(stderr, "******PngCompressData: PANIC! Error while writing the image rows.\n");
    #endif

    png_destroy_write_struct(&png_ptr, &info_ptr);

    free(pngCompBuf);

    return NULL;
  }

  if (color_type == PNG_COLOR_TYPE_PALETTE)
  {
    srcBuf = (CARD8 *) malloc(w * sizeof(CARD8));

    if (srcBuf == NULL)
    {
      #ifdef PANIC
      fprintf(stderr, "******PngCompressData: PANIC! Cannot allocate [%d] bytes.\n",
                  (int) (w * sizeof(CARD8)));
      #endif

      return NULL;
    }

    /*
     * TODO: Be sure the padded bytes are cleaned.
     * It would be better to set to zero the bytes
     * that are not alligned to the word boundary
     * at the end of the procedure.
     */

    memset(srcBuf, 0, w * sizeof(CARD8));
  }
  else
  {
    srcBuf = (CARD8 *) malloc(w * 3 * sizeof(CARD8));

    /*
     * TODO: See above.
     */

    memset(srcBuf, 0, w * 3 * sizeof(CARD8));
  }

  if (srcBuf == NULL)
  {
    #ifdef PANIC
    fprintf(stderr, "******PngCompressData: PANIC! Cannot allocate [%d] bytes.\n",
                w * 3);
    #endif

    free(pngCompBuf);

    return NULL;
  }

  for (dy = 0; dy < h; dy++)
  {
    if (color_type == PNG_COLOR_TYPE_RGB)
    {
      PrepareRowForPng(srcBuf, dy, w);
    }
    else
    {
      memcpy(srcBuf, image_index + (dy * w), w);
    }

    png_write_row(png_ptr, srcBuf);
  }

  #ifdef DEBUG
  fprintf(stderr, "******PngCompressedData: Compression finished. Lines handled [%d,%d].\n",
              dy, h);
  #endif

  free(srcBuf);
  free(image_index);

  if (setjmp(png_jmpbuf(png_ptr)))
  {
    #ifdef PANIC
    fprintf(stderr, "******PngCompressData: PANIC! error during end of write.\n");
    #endif

    png_destroy_write_struct(&png_ptr, &info_ptr);

    free(pngCompBuf);

    return NULL;
  }

  png_write_end(png_ptr, NULL);

  if (color_type == PNG_COLOR_TYPE_PALETTE)
  {
    png_free(png_ptr, palette);
  }

  png_destroy_write_struct(&png_ptr, &info_ptr);

  /*
   * Check the size of the resulting data.
   */

  if (pngDataLen > 0)
  {
    #ifdef DEBUG

    int i = 0;

    fprintf(stderr, "******PngCompressedData: Compressed size [%d].\n",
                pngDataLen);

    pngId++;
    sprintf(pngName, "png%d", pngId);
    pngFile = fopen(pngName, "w");

    for (i = 0; i < pngDataLen; i++)
    {
      fprintf(pngFile, "%c", *(pngCompBuf + i));
    }

    fclose(pngFile);

    #endif

    *compressed_size = pngDataLen;

    return pngCompBuf;
  }
  else
  {
    #ifdef DEBUG
    fprintf(stderr, "******PngCompressedData: PANIC! Invalid size of the compressed data [%d].\n",
                pngDataLen);
    #endif

    free(pngCompBuf);

    return NULL;
  }
}

static void PngWriteData(png_structp png_ptr, png_bytep data, png_size_t length)
{
  memcpy(((char *) png_get_io_ptr(png_ptr) + pngDataLen), data, length);

  pngDataLen += length;
}

static void PngFlushData(png_structp png_ptr)
{
}

void PrepareRowForPng(CARD8 *dst, int y, int count)
{
  if (bitsPerPixel == 32)
  {
    if (redMax == 0xff &&
           greenMax == 0xff &&
               blueMax == 0xff)
    {
      PrepareRowForPng24(dst, y, count);
    }
    else
    {
      PrepareRowForPng32(dst, y, count);
    }
  }
  else if (bitsPerPixel == 24)
  {
    memcpy(dst, pngBeforeBuf + y * bytesPerLine, count * 3);
  }
  else
  {
    /*
     * 16 bpp assumed.
     */

    PrepareRowForPng16(dst, y, count);
  }
}



void PrepareRowForPng24(CARD8 *dst, int y, int count)
{
  CARD8 *fbptr;
  CARD32 pix;

  fbptr = (CARD8 *) (pngBeforeBuf + y * bytesPerLine);

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

#define DEFINE_PNG_GET_ROW_FUNCTION(bpp)                                    \
                                                                            \
void PrepareRowForPng##bpp(CARD8 *dst, int y, int count)                    \
{                                                                           \
  CARD8 *fbptr;                                                             \
  CARD##bpp pix;                                                            \
  int inRed, inGreen, inBlue;                                               \
  int i;                                                                    \
                                                                            \
  fbptr = (CARD8 *) (pngBeforeBuf + y * bytesPerLine);                      \
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
    fbptr += (bpp >> 3);                                                    \
                                                                            \
    inRed = (int)                                                           \
            (pix >> redShift   & redMax);                                   \
    inGreen = (int)                                                         \
            (pix >> greenShift & greenMax);                                 \
    inBlue  = (int)                                                         \
            (pix >> blueShift  & blueMax);                                  \
    *dst++ = (CARD8)((inRed   * 255 + redMax / 2) /                         \
                         redMax);                                           \
    *dst++ = (CARD8)((inGreen * 255 + greenMax / 2) /                       \
                         greenMax);                                         \
    *dst++ = (CARD8)((inBlue  * 255 + blueMax / 2) /                        \
                         blueMax);                                          \
  }                                                                         \
}

DEFINE_PNG_GET_ROW_FUNCTION(16)
DEFINE_PNG_GET_ROW_FUNCTION(32)
