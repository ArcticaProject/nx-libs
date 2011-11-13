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

#include "Xlib.h"

#include "NXpack.h"

#include "Mask.h"

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

/*
 * Try first to reduce to a white or black
 * pixel. If not possible, apply the mask.
 * Note that correction is applied at the
 * time pixel is unpacked.
 */

#define MaskPixel(red, green, blue, mask) \
\
if (red > mask -> white_threshold && \
        green > mask -> white_threshold && \
            blue > mask -> white_threshold) \
{ \
    red = green = blue = 0xff; \
} \
else if (red < mask -> black_threshold && \
             green < mask -> black_threshold && \
                 blue < mask -> black_threshold) \
{ \
    red = green = blue = 0x00; \
} \
else \
{ \
    red   = red   & mask -> color_mask; \
    green = green & mask -> color_mask; \
    blue  = blue  & mask -> color_mask; \
}

int MaskImage(const ColorMask *mask, XImage *src_image, XImage *dst_image)
{
  unsigned long pixel;

  register unsigned int red;
  register unsigned int green;
  register unsigned int blue;

  register unsigned long data_size;

  register unsigned int i;

  data_size = (src_image -> bytes_per_line * src_image -> height) >> 2;

  #ifdef TEST
  fprintf(stderr, "******MaskImage: Going to mask image with [%d] bits per pixel.\n",
              src_image -> bits_per_pixel);
  #endif

  if (src_image -> bits_per_pixel == 24 || src_image -> bits_per_pixel == 32)
  {
    register unsigned char *pixel_addr;

    for (i = 0; i < data_size; i++)
    {
      pixel = ((unsigned long *) src_image -> data)[i];

      pixel_addr = (unsigned char *) &pixel;

      red   = pixel_addr[2];
      green = pixel_addr[1];
      blue  = pixel_addr[0];

      #ifdef DEBUG
      fprintf(stderr, "******MaskImage: 24/32 bits original R [%d] G [%d] B [%d] A [%d].\n",
              red, green, blue, pixel_addr[3]);
      #endif

      MaskPixel(red, green, blue, mask);

      #ifdef DEBUG
      fprintf(stderr, "******MaskImage: 24/32 bits masked R [%d] G [%d] B [%d] A [%d].\n",
                  red, green, blue, pixel_addr[3]);
      #endif

      #ifdef DEBUG
      fprintf(stderr, "******MaskImage: 24/32 bits pixel 0x%lx", pixel);
      #endif

      pixel_addr[2] = red;
      pixel_addr[1] = green;
      pixel_addr[0] = blue;

      ((unsigned long*)dst_image -> data)[i] = pixel;

      #ifdef DEBUG
      fprintf(stderr, " -> 0x%lx\n", pixel);
      #endif
    }

    return 1;
  }
  else if (src_image -> bits_per_pixel == 16)
  {
    /* 
     * FIXME: Masking doesn't work in 16 bpp.
     *

    unsigned long src_addr, *dst_addr;
    unsigned short *src_pixels_addr, *dst_pixels_addr;

    for (i = 0; i < data_size; i++)
    {
      src_addr = ((unsigned long *)src_image -> data)[i];
      dst_addr = (unsigned long *)((unsigned long *)dst_image -> data + i);

      src_pixels_addr = ((unsigned short *) &src_addr);
      dst_pixels_addr = ((unsigned short *) dst_addr);

      red   = (src_pixels_addr[0] & src_image -> red_mask)   >> 8;
      green = (src_pixels_addr[0] & src_image -> green_mask) >> 3;
      blue  = (src_pixels_addr[0] & src_image -> blue_mask)  << 3;

      #ifdef DEBUG
      fprintf(stderr, "******MaskImage: 16 bits original R [%d] G [%d] B [%d].\n",
              red, green, blue);
      #endif

      MaskPixel(red, green, blue, mask);

      #ifdef DEBUG
      fprintf(stderr, "******MaskImage: 16 bits masked R [%d] G [%d] B [%d].\n",
              red, green, blue);
      #endif

      dst_pixels_addr[0] = ((red << 8)   & src_image -> red_mask)   |
                           ((green << 3) & src_image -> green_mask) |
                           ((blue >> 3)  & src_image -> blue_mask);

      #ifdef DEBUG
      fprintf(stderr, "******MaskImage: 16 bits pixel 0x%x", dst_pixels_addr[0]);
      #endif

      red   = (src_pixels_addr[1] & src_image -> red_mask)   >> 8;
      green = (src_pixels_addr[1] & src_image -> green_mask) >> 3;
      blue  = (src_pixels_addr[1] & src_image -> blue_mask)  << 3;

      #ifdef DEBUG
      fprintf(stderr, "******MaskImage: 16 bits original R [%d] G [%d] B [%d].\n",
              red, green, blue);
      #endif

      MaskPixel(red, green, blue, mask);

      #ifdef DEBUG
      fprintf(stderr, "******MaskImage: 16 bits masked R [%d] G [%d] B [%d].\n",
              red, green, blue);
      #endif

      dst_pixels_addr[1] = ((red << 8)   & src_image -> red_mask)   |
                           ((green << 3) & src_image -> green_mask) |
                           ((blue >> 3)  & src_image -> blue_mask);

      #ifdef DEBUG
      fprintf(stderr, "******MaskImage: 16 bits pixel 0x%x", dst_pixels_addr[0]);
      #endif
    }

    if (dst_image -> width & 0x00000001)
    {
      int card32_per_line;
      int i;

      card32_per_line = dst_image -> bytes_per_line >> 2;

      for (i = 0; i < dst_image -> height;)
      {
        ((CARD32 *) dst_image -> data)[(++i * card32_per_line) - 1] &= 0x0000ffff;
      }
    }

    *
    * End of FIXME.
    */
  }
  else
  {
    #ifdef TEST
    fprintf(stderr, "******MaskImage: PANIC! Cannot apply mask with [%d] bits per pixel.\n",
                src_image -> bits_per_pixel);
    #endif

    return 0;
  }

  return 1;
}

int MaskInPlaceImage(const ColorMask *mask, XImage *image)
{
  unsigned long pixel;

  register unsigned int red;
  register unsigned int green;
  register unsigned int blue;

  register unsigned int i;

  register unsigned long data_size;

  data_size = (image -> bytes_per_line * image -> height)>>2;

  #ifdef TEST
  fprintf(stderr, "******MaskInPlaceImage: Going to mask image with [%d] bits per pixel.\n",
              image -> bits_per_pixel);
  #endif

  if (image -> bits_per_pixel == 24 || image -> bits_per_pixel == 32)
  {
    register unsigned char *pixel_addr;

    for (i = 0; i < data_size; i++)
    {
      pixel = ((unsigned long *) image -> data)[i];

      pixel_addr = (unsigned char *) &pixel;

      red   = pixel_addr[2];
      green = pixel_addr[1];
      blue  = pixel_addr[0];

      #ifdef DEBUG
      fprintf(stderr, "******MaskInPlaceImage: 24/32 bits original R [%d] G [%d] B [%d] A [%d].\n",
                  red, green, blue, pixel_addr[3]);
      #endif

      MaskPixel(red, green, blue, mask);

      #ifdef DEBUG
      fprintf(stderr, "******MaskInPlaceImage: 24/32 bits masked R [%d] G [%d] B [%d] A [%d].\n",
                  red, green, blue, pixel_addr[3]);
      #endif

      #ifdef DEBUG
      fprintf(stderr, "******MaskInPlaceImage: 24/32 bits pixel 0x%lx", pixel);
      #endif

      pixel_addr[2] = red;
      pixel_addr[1] = green;
      pixel_addr[0] = blue;

      ((unsigned long *) image -> data)[i] = pixel;

      #ifdef DEBUG
      fprintf(stderr, " -> 0x%lx\n", pixel);
      #endif
    }

    return 1;
  }
  else if (image -> bits_per_pixel == 16)
  {
    /*
     * FIXME: Mask doesn't still work for 16 bits.
     *

    unsigned long addr;
    register unsigned short *pixels_addr;

    for (i = 0; i < data_size; i++)
    {
      addr = ((unsigned long *) image -> data)[i];

      pixels_addr = ((unsigned short *) &addr);

      red   = (pixels_addr[0] & image -> red_mask)   >> 8;
      green = (pixels_addr[0] & image -> green_mask) >> 3;
      blue  = (pixels_addr[0] & image -> blue_mask)  << 3;

      #ifdef DEBUG
      fprintf(stderr, "******MaskInPlaceImage: 16 bits original R [%d] G [%d] B [%d].\n",
                  red, green, blue);
      #endif

      MaskPixel(red, green, blue, mask);

      #ifdef DEBUG
      fprintf(stderr, "******MaskInPlaceImage: 16 bits masked R [%d] G [%d] B [%d].\n",
                  red, green, blue);
      #endif

      pixels_addr[0] = ((red << 8)   & image -> red_mask) |
                           ((green << 3) & image -> green_mask) |
                               ((blue >> 3)  & image -> blue_mask);

      #ifdef DEBUG
      fprintf(stderr, "******MaskInPlaceImage: 16 bits pixel 0x%x", pixels_addr[0]);
      #endif

      red   = (pixels_addr[1] & image -> red_mask)   >> 8;
      green = (pixels_addr[1] & image -> green_mask) >> 3;
      blue  = (pixels_addr[1] & image -> blue_mask)  << 3;

      #ifdef DEBUG
      fprintf(stderr, "******MaskInPlaceImage: 16 bits original R [%d] G [%d] B [%d].\n",
                  red, green, blue);
      #endif

      MaskPixel(red, green, blue, mask);

      #ifdef DEBUG
      fprintf(stderr, "******MaskInPlaceImage: 16 bits masked R [%d] G [%d] B [%d].\n",
                  red, green, blue);
      #endif

      pixels_addr[1] = ((red << 8)   & image -> red_mask) |
                           ((green << 3) & image -> green_mask) |
                               ((blue >> 3)  & image -> blue_mask);

      #ifdef DEBUG
      fprintf(stderr, "******MaskInPlaceImage: 16 bits pixel 0x%x", pixels_addr[1]);
      #endif
    }

    if (image -> width & 0x00000001)
    {
      int card32_per_line;
      int i;

      card32_per_line = image -> bytes_per_line >> 2;

      for (i = 0; i < image -> height;)
      {
        ((CARD32 *) image -> data)[(++i * card32_per_line) - 1] &= 0x0000ffff;
      }
    }

    *
    * End of FIXME.
    */
  }
  else
  {
    #ifdef TEST
    fprintf(stderr, "******MaskImage: PANIC! Cannot apply mask with [%d] bits per pixel.\n",
                image -> bits_per_pixel);
    #endif

    return 0;
  }

  return 1;
}

static int Pack16To8(unsigned int src_data_size, XImage *src_image, XImage *dst_image)
{
  unsigned short *src_pixel = (unsigned short *) src_image -> data;
  unsigned char  *dst_pixel = (unsigned char *)  dst_image -> data;

  #ifdef DEBUG
  unsigned int counter = 0;
  #endif

  while (src_pixel < ((unsigned short *) (src_image -> data + src_data_size)))
  {
    #ifdef DEBUG
    fprintf(stderr, "******PackImage: Pixel [%d] value [0x%x] red [0x%x] green [0x%x] blue [0x%x].\n",
                counter, *src_pixel, (*src_pixel & 0xc000) >> 8,
                    ((*src_pixel & 0x600) >> 3), (*src_pixel & 0x18) << 3);
    #endif

    if (*src_pixel == 0x0)
    {
      *dst_pixel = 0x0;
    }
    else if (*src_pixel == 0xffff)
    {
      *dst_pixel = 0xff;
    }
    else
    {
      *dst_pixel = ((*src_pixel & 0xc000) >> 10) |
                        ((*src_pixel & 0x600) >> 7) |
                             ((*src_pixel & 0x18) >> 3);
    }

    #ifdef DEBUG
    fprintf(stderr, "******PackImage: Pixel [%d] destination [0x%x].\n",
                counter++, *dst_pixel);
    #endif

    src_pixel++;
    dst_pixel++;
  }

  return 1;
}

static int Pack24To8(unsigned int src_data_size, XImage *src_image, XImage *dst_image)
{
  unsigned char *src_pixel = (unsigned char *) src_image -> data;
  unsigned char *dst_pixel = (unsigned char *) dst_image -> data;

  int i;

  unsigned int bytes_per_line = src_image -> bytes_per_line;

  unsigned char *end_of_line = (unsigned char *) (src_pixel + bytes_per_line);


  #ifdef DEBUG
  unsigned int counter = 0;
  #endif

  for (i = 0; i < src_image -> height; i++ )
  {
    #ifdef DEBUG
    fprintf(stderr, "******PackImage: Pixel [%d] value [0x%x%x%x] red [0x%x] green [0x%x] blue [0x%x].\n",
                counter, src_pixel[0], src_pixel[1], src_pixel[2], src_pixel[0] & 0xc0,
                    src_pixel[1] & 0xc0, src_pixel[2] & 0xc0);
    #endif

    while(src_pixel < end_of_line - 2)
    {
      if (src_pixel[0] == 0x00 &&
              src_pixel[1] == 0x00 &&
                  src_pixel[2] == 0x00)
      {
        *dst_pixel = 0x0;
      }
      else if (src_pixel[0] == 0xff &&
                   src_pixel[1] == 0xff &&
                       src_pixel[2] == 0xff)
      {
        *dst_pixel = 0xff;
      }
      else
      {
        /*
         * Pixel layout:
         *
         * 24 bit RRRRR000 GGGGG000 BBBBB000 -> 8 bit 00RRGGBB
         */

        *dst_pixel = (src_pixel[0] & 0xc0) >> 2 |
                         ((src_pixel[1] & 0xc0) >> 4) |
                              ((src_pixel[2] & 0xc0) >> 6);
      }

      #ifdef DEBUG
      fprintf(stderr, "******PackImage: Pixel [%d] destination [0x%x].\n",
                  counter++, *dst_pixel);
      #endif

      src_pixel += 3;
      dst_pixel += 1;
    }

    src_pixel = end_of_line;
    end_of_line += bytes_per_line;
  }
  
  return 1;
}

static int Pack24To16(unsigned int src_data_size, XImage *src_image, XImage *dst_image)
{
  unsigned char  *src_pixel = (unsigned char  *) src_image -> data;
  unsigned short *dst_pixel = (unsigned short *) dst_image -> data;

  int i;

  unsigned int bytes_per_line = src_image -> bytes_per_line;

  unsigned char *end_of_line = (unsigned char *) (src_pixel + bytes_per_line);

  #ifdef DEBUG
  unsigned int counter = 0;
  #endif

  for (i = 0; i < src_image -> height; i++ )
  {
    while(src_pixel < end_of_line - 2)
    {
      #ifdef DEBUG
      fprintf(stderr, "******PackImage: Pixel [%d] value [0x%x%x%x] red [0x%x] green [0x%x] blue [0x%x].\n",
                  counter, src_pixel[0], src_pixel[1], src_pixel[2], src_pixel[0] & 0xf8,
                      src_pixel[1] & 0xf8, src_pixel[2] & 0xf8);
      #endif

      if (src_pixel[0] == 0x00 &&
              src_pixel[1] == 0x00 &&
                  src_pixel[2] == 0x00)
      {
        *dst_pixel = 0x0;
      }
      else if (src_pixel[0] == 0xff &&
                   src_pixel[1] == 0xff &&
                       src_pixel[2] == 0xff)
      {
        *dst_pixel = 0xffff;
      }
      else
      {
        /*
         * Pixel layout:
         *
         * 24 bit RRRRR000 GGGGG000 BBBBB000 -> 16 bit 0RRRRRGG GGGBBBBB
         */

        *dst_pixel = ((src_pixel[0] & 0xf8) << 7) |
                         ((src_pixel[1] & 0xf8) << 2) |
                             ((src_pixel[2] & 0xf8) >> 3);
      }

      #ifdef DEBUG
      fprintf(stderr, "******PackImage: Pixel [%d] destination [0x%x].\n",
                  counter++, *dst_pixel);
      #endif

      src_pixel += 3;
      dst_pixel += 1;
    }

    src_pixel = end_of_line;
    end_of_line += bytes_per_line;
  }

  return 1;
}

static int Pack32To8(unsigned int src_data_size, XImage *src_image, XImage *dst_image)
{
  unsigned int  *src_pixel = (unsigned int  *) src_image -> data;
  unsigned char *dst_pixel = (unsigned char *) dst_image -> data;

  #ifdef DEBUG
  unsigned int counter = 0;
  #endif

  while (src_pixel < ((unsigned int  *) (src_image -> data + src_data_size)))
  {
    #ifdef DEBUG
    fprintf(stderr, "******PackImage: Pixel [%d] value [0x%x] red [0x%x] green [0x%x] blue [0x%x].\n",
                counter, *src_pixel, (*src_pixel & 0xc00000),
                    (*src_pixel & 0xc000), (*src_pixel & 0xc0));
    #endif

    if (*src_pixel == 0x0)
    {
      *dst_pixel = 0x0;
    }
    else if (*src_pixel == 0xffffff)
    {
      *dst_pixel = 0xff;
    }
    else
    {
      *dst_pixel = ((*src_pixel & 0xc00000) >> 18) |
                        ((*src_pixel & 0xc000) >> 12) |
                             ((*src_pixel & 0xc0) >> 6);
    }

    #ifdef DEBUG
    fprintf(stderr, "******PackImage: Pixel [%d] destination [0x%x].\n",
                counter++, *dst_pixel);
    #endif

    src_pixel++;
    dst_pixel++;
  }

  return 1;
}

static int Pack32To16(unsigned int src_data_size, XImage *src_image, XImage *dst_image)
{
  unsigned int   *src_pixel = (unsigned int  *)  src_image -> data;
  unsigned short *dst_pixel = (unsigned short *) dst_image -> data;

  #ifdef DEBUG
  unsigned int counter = 0;
  #endif

  while (src_pixel < ((unsigned int  *) (src_image -> data + src_data_size)))
  {
    #ifdef DEBUG
    fprintf(stderr, "******PackImage: Pixel [%d] value [0x%x] red [0x%x] green [0x%x] blue [0x%x].\n",
                counter, *src_pixel, (*src_pixel & 0xf80000),
                    (*src_pixel & 0xf800), (*src_pixel & 0xf8));
    #endif

    if (*src_pixel == 0x0)
    {
      *dst_pixel = 0x0;
    }
    else if (*src_pixel == 0xffffff)
    {
      *dst_pixel = 0xffff;
    }
    else
    {
      *dst_pixel = ((*src_pixel & 0xf80000) >> 9) |
                        ((*src_pixel & 0xf800) >> 6) |
                             ((*src_pixel & 0xf8) >> 3);
    }

    #ifdef DEBUG
    fprintf(stderr, "******PackImage: Pixel [%d] destination [0x%x].\n",
                counter++, *dst_pixel);
    #endif

    src_pixel++;
    dst_pixel++;
  }

  return 1;
}

static int Pack32To24(unsigned int src_data_size, XImage *src_image, XImage *dst_image)
{
  unsigned int  *src_pixel = (unsigned int  *) src_image -> data;
  unsigned char *dst_pixel = (unsigned char *) dst_image -> data;

  #ifdef DEBUG
  unsigned int counter = 0;
  #endif

  while (src_pixel < ((unsigned int  *) (src_image -> data + src_data_size)))
  {
    #ifdef DEBUG
    fprintf(stderr, "******PackImage: Pixel [%d] value [0x%x] red [0x%x] green [0x%x] blue [0x%x].\n",
                counter, *src_pixel, ((*src_pixel & 0xff0000) >> 16),
                    ((*src_pixel & 0x00ff00) >> 8), (*src_pixel & 0xff));
    #endif

    if (*src_pixel == 0x0)
    {
      dst_pixel[0] = dst_pixel[1] = dst_pixel[2] = 0x0;
    }
    else if (*src_pixel == 0xffffff)
    {
      dst_pixel[0] = dst_pixel[1] = dst_pixel[2] = 0xff;
    }
    else
    {
      dst_pixel[0] = (*src_pixel & 0xff0000) >> 16;
      dst_pixel[1] = (*src_pixel & 0x00ff00) >> 8;
      dst_pixel[2] = (*src_pixel & 0x0000ff);
    }

    #ifdef DEBUG
    fprintf(stderr, "******PackImage: Pixel [%d] destination [0x%x], [0x%x], [0x%x].\n",
                counter++, dst_pixel[0], dst_pixel[1], dst_pixel[2]);
    #endif

    src_pixel += 1;
    dst_pixel += 3;
  }

  return 1;
}

int PackImage(unsigned int method, unsigned int src_data_size, XImage *src_image,
                  unsigned int dst_data_size, XImage *dst_image)
{
  unsigned int src_bits_per_pixel;
  unsigned int dst_bits_per_pixel;

  src_bits_per_pixel = src_image -> bits_per_pixel;
  dst_bits_per_pixel = MethodBitsPerPixel(method);

  #ifdef TEST
  fprintf(stderr, "******PackImage: Source bits per pixel [%d], destination bits per pixel [%d].\n",
              src_bits_per_pixel, dst_bits_per_pixel);

  fprintf(stderr, "******PackImage: Source data size [%d], destination data size [%d].\n",
              src_data_size, dst_data_size);
  #endif

  if (dst_bits_per_pixel >= src_bits_per_pixel)
  {
    #ifdef PANIC
    fprintf(stderr, "******PackImage: PANIC! Cannot pack image from [%d] to [%d] bytes per pixel.\n",
                src_bits_per_pixel, dst_bits_per_pixel);
    #endif

    return 0;
  }

  switch (src_bits_per_pixel)
  {
    case 16:
    {
      switch (dst_bits_per_pixel)
      {
        case 8:
        {
          return Pack16To8(src_data_size, src_image, dst_image);
        }
        default:
        {
          return 0;
        }
      }
    }
    case 24:
    {
      switch (dst_bits_per_pixel)
      {
        case 8:
        {
          return Pack24To8(src_data_size, src_image, dst_image);
        }
        case 16:
        {
          return Pack24To16(src_data_size, src_image, dst_image);
        }
        default:
        {
          return 0;
        }
      }
    }
    case 32:
    {
      switch (dst_bits_per_pixel)
      {
        case 8:
        {
          return Pack32To8(src_data_size, src_image, dst_image);
        }
        case 16:
        {
          return Pack32To16(src_data_size, src_image, dst_image);
        }
        case 24:
        {
          return Pack32To24(src_data_size, src_image, dst_image);
        }
        default:
        {
          return 0;
        }
      }
    }
    default:
    {
      return 0;
    }
  }
}

/*
 * Replace the ffs() call that may be not
 * present on some systems.
 */

int FindLSB(int word)
{
  int t = word;

  int m = 1;
  int i = 0;

  for (; i < sizeof(word) << 3; i++, m <<= 1)
  {
    if (t & m)
    {
      return i + 1;
    }
  }

  return 0;
}

