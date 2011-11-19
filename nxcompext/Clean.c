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
#include <signal.h>

#include "os.h"

#include "NXlib.h"

#include "Clean.h"

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

int CleanXYImage(XImage *image)
{
  int i, j, k, plane;

  int bitsToClean = (image -> bytes_per_line << 3) - image -> width - image -> xoffset;

  unsigned int bytesToClean = bitsToClean >> 3;

  bitsToClean &= 7;

  for (k = 0; k < image -> depth; k++)
  {
    plane = k * (image -> bytes_per_line * image -> height);

    for (i = 1; i <= image -> height; i++)
    {
      if (image -> byte_order == image -> bitmap_bit_order)
      {
        for (j = 1; j <= bytesToClean; j++)
        {
          image -> data[plane + i * (image -> bytes_per_line) - j] = 0x00;
        }
      }
      else
      {
        for (j = bytesToClean; j >= 1; j--)
        {
          image -> data[plane + i * (image -> bytes_per_line) - j] = 0x00;
        }
      }

      if (image -> bitmap_bit_order == MSBFirst)
      {
        image -> data[plane + i * (image -> bytes_per_line) - j] &= 0xff << bitsToClean;
      }
      else
      {
        image -> data[plane + i * (image -> bytes_per_line) - j] &= 0xff >> bitsToClean;
      }
    }
  }

  return 1;
}

int CleanZImage(XImage *image)
{
  unsigned int bytesToClean;
  unsigned int j;
  unsigned int imageLength;

  #ifdef TEST
  fprintf(stderr, "*****CleanZImage: Going to clean image of [%d] bits per pixel.\n",
              image -> bits_per_pixel);
  #endif

  switch (image -> bits_per_pixel)
  {
    case 32:
    {
      /*
       * The caller should pay attention at extracting
       * the alpha channel prior to cleaning the image.
       * Cleaning an image which is carrying the alpha
       * channel will result in the image being treated
       * as fully transparent.
       */

      register int i;

      bytesToClean = image -> bytes_per_line * image -> height;

      #ifdef DEBUG
      fprintf(stderr, "*****CleanZImage: Cleaning [%d] bytes with bits per pixel [%d] "
                  "width [%d] bytes per line [%d] height [%d].\n", bytesToClean,
                      image -> bits_per_pixel, image -> width, image ->
                          bytes_per_line, image -> height);
      #endif

      if (image -> byte_order == LSBFirst)
      {
        for (i = 3; i < bytesToClean; i += 4)
        {
          ((unsigned char *) image -> data)[i] = 0x00;
        }
      }
      else
      {
        for (i = 0; i < bytesToClean; i += 4)
        {
          ((unsigned char *) image -> data)[i] = 0x00;
        }
      }

      break;
    }
    case 24:
    case 15:
    case 16:
    case 8:
    {
      register int i, j;

      bytesToClean = image -> bytes_per_line -
                           ((image -> width * image -> bits_per_pixel) >> 3);

      for (i = 1; i <= image -> height; i++)
      {
        for (j = bytesToClean; j > 0; j--)
        {
          ((unsigned char *) image -> data)[(i * image -> bytes_per_line) - j] = 0x00;
        }
      }

      break;
    }
    default:
    {
      #ifdef PANIC
      fprintf(stderr, "*****CleanZImage: PANIC! Cannot clean image with [%d] bits per pixel.\n",
                  image -> bits_per_pixel);
      #endif
    }
  }

  /* 
   * Clean the padding bytes at the real
   * end of the buffer.
   */

  imageLength = image -> bytes_per_line * image -> height;

  bytesToClean = imageLength % 4;

  for (j = 0; j < bytesToClean; j++)
  {
    ((unsigned char *)image -> data)[(imageLength + j)] = 0x00;
  }

  return 1;
}

/*
 * Copy a clean version of src_image into dst_image.
 * This code is not taking care of the image format.
 * The agent doesn't use it and you have to consider
 * it unsupported.
 */

int CopyAndCleanImage(XImage *src_image, XImage *dst_image)
{
  register long data_size;
  register int i;

  data_size = (src_image -> bytes_per_line * src_image -> height) >> 2;

  #ifdef WARNING
  fprintf(stderr, "******CleanImage: WARNING! Function called with image of [%d] bits per pixel.\n",
              src_image -> bits_per_pixel);
  #endif

  switch (src_image -> bits_per_pixel)
  {
    case 32:
    {
      unsigned int mask;

      if (src_image -> byte_order == MSBFirst)
      {
        mask = 0xffffff00;
      }
      else
      {
        mask = 0x00ffffff;
      }
      for (i = 0; i < data_size; i++)
      {
        ((unsigned int *)dst_image -> data)[i] = ((unsigned int *)src_image -> data)[i] & mask;
      }

      break;
    }

    case 24:
    {
      unsigned int bytes_to_clean;

      for (i = 0; i < data_size; i++)
      {
        ((unsigned int *)dst_image -> data)[i] = ((unsigned int *)src_image -> data)[i];
      }

      bytes_to_clean = dst_image -> bytes_per_line - ((dst_image -> width *
                           dst_image -> bits_per_pixel) >> 3);

      if (bytes_to_clean)
      {
        register unsigned int mask = 0xffffffff;
        register int line_size;
        register int i;
                
        line_size = dst_image -> bytes_per_line >> 2;

        if (dst_image -> byte_order == MSBFirst)
        {
          mask = mask << (bytes_to_clean << 3);
        }
        else
        {
          mask = mask >> (bytes_to_clean << 3);
        }

        for (i = 0; i < dst_image -> height;)
        {
          ((unsigned char *)dst_image -> data)[(++i * line_size) -1] &= mask;
        }
      }

      break;
    }

    case 15:
    case 16:
    {
      for (i = 0; i < data_size; i++)
      {
        ((unsigned int *) dst_image -> data)[i] = ((unsigned int *) src_image -> data)[i];
      }

      if (src_image -> width & 0x00000001)
      {
        int card32_per_line = dst_image -> bytes_per_line >> 2;

        for (i = 0; i < dst_image -> height;)
        {
          ((unsigned int *) dst_image -> data)[(++i * card32_per_line) -1] &= 0x0000ffff;
        }
      }

      break;
    }

    case 8:
    {
      unsigned int mask = 0x00000000;

      switch (dst_image -> width % 4)
      {
        case 3:
        {
          mask = 0x00ffffff;

          break;
        }
        case 2:
        {
          mask = 0x0000ffff;

          break;
        }
        case 1:
        {
          mask = 0x000000ff;

          break;
        }
        default:
        {
          /*
           * Nothing to clean.
           */

          break;
        }
      }

      for (i = 0; i < data_size; i++)
      {
        ((unsigned int *) dst_image -> data)[i] = ((unsigned int *) src_image -> data)[i];
      }

      if (mask)
      {
        int card32_per_line;
        int i;

        card32_per_line = dst_image -> bytes_per_line >> 2;

        for (i = 0; i < dst_image -> height; i++)
        {
          ((unsigned int *) dst_image -> data)[(++i * card32_per_line) -1] &= mask;
        }
      }

      break;
    }

    default:
    {
      #ifdef PANIC
      fprintf(stderr, "******CleanImage: PANIC! Cannot clean image of [%d] bits per pixel.\n",
                  src_image -> bits_per_pixel);
      #endif

      return 0;
    }
  }

  return 1;
}
