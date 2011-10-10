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

#include <stdio.h>
#include <stdlib.h>

#include "Xmd.h"
#include "Xlib.h"

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

#define PIXEL_ELEMENTS  256
#define PIXEL_THRESHOLD 8
#define PIXEL_STEP      7

unsigned int Get16(const char *buffer, int order);
unsigned int Get24(const char *buffer, int order);
unsigned int Get32(const char *buffer, int order);

void Put16(unsigned int value, char *buffer, int order);
void Put24(unsigned int value, char *buffer, int order);
void Put32(unsigned int value, char *buffer, int order);

static int nxagentComparePixels(const void *p1, const void *p2)
{
  int pixel1 = *((int *) p1);
  int pixel2 = *((int *) p2);

  return (pixel1 < pixel2 ? -1 : (pixel1 == pixel2 ? 0 : 1));
}

int nxagentUniquePixels(XImage *image)
{
  int i = 0;

  int pixels[PIXEL_ELEMENTS];

  int elements = PIXEL_ELEMENTS;
  int unique   = 0;

  int total;
  int ratio;
  int step;

  int last = -1;

  const char *next = image -> data;

  #ifdef TEST
  fprintf(stderr, "nxagentUniquePixels: Image geometry [%d,%d] depth [%d] bits per pixel [%d].\n",
              image -> width, image -> height, image -> depth, image -> bits_per_pixel);
  #endif

  /*
   * Take at most 256 pixels from the image.
   */

  total = image -> width * image -> height;
  
  step = total / elements;

  if (step < PIXEL_STEP)
  {
    step = PIXEL_STEP;
  }

  #ifdef TEST
  fprintf(stderr, "nxagentUniquePixels: Step is [%d] with [%d] pixels and [%d] elements.\n",
              step, total, elements);
  #endif

  /*
   * Shift at the left after each scanline.
   */
   
  if (image -> bytes_per_line % step == 0)
  {
    step++;

    #ifdef TEST
    fprintf(stderr, "nxagentUniquePixels: Increasing step to [%d] with [%d] bytes per line.\n",
                step, image -> bytes_per_line);
    #endif
  }

  elements = total / step;

  if (elements > PIXEL_ELEMENTS)
  {
    elements = PIXEL_ELEMENTS;
  }

  #ifdef TEST
  fprintf(stderr, "nxagentUniquePixels: Step is now [%d] with [%d] elements.\n",
              step, elements);
  #endif

  if (elements < PIXEL_THRESHOLD)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentUniquePixels: Assuming ratio [100] with only [%d] elements.\n",
                elements);
    #endif

    return 100;
  }

  #ifdef TEST
  fprintf(stderr, "nxagentUniquePixels: Scanning [%d] pixels out of [%d] with step [%d].\n",
              elements, total, step);
  #endif

  /*
   * Take one pixel every n from the image and
   * add it to the array.
   */

  switch (image -> bits_per_pixel)
  {
    case 32:
    {
      for (i = 0; i < elements; i++)
      {
        pixels[i] = Get32(next, image -> byte_order);

        next += (4 * step);

        #ifdef DEBUG
        fprintf(stderr, "nxagentUniquePixels: pixels[%d][0x%08x].\n",
                    i, pixels[i]);
        #endif
      }

      break;
    }
    case 24:
    {
      for (i = 0; i < elements; i++)
      {
        pixels[i] = Get24(next, image -> byte_order);

        next += (3 * step);

        #ifdef DEBUG
        fprintf(stderr, "nxagentUniquePixels: pixels[%d][0x%08x].\n",
                    i, pixels[i]);
        #endif
      }

      break;
    }
    case 16:
    case 15:
    {
      /*
       * Note that the padding bytes at the end
       * of the scanline are included in the set.
       * This is not a big problem. What we want
       * to find out is just how compressible is
       * the image data.
       */

      for (i = 0; i < elements; i++)
      {
        pixels[i] = Get16(next, image -> byte_order);

        next += (2 * step);

        #ifdef DEBUG
        fprintf(stderr, "nxagentUniquePixels: pixels[%d][0x%08x].\n",
                    i, pixels[i]);
        #endif
      }

      break;
    }
    default:
    {
      #ifdef PANIC
      fprintf(stderr, "nxagentUniquePixels: PANIC! Assuming ratio [100] with [%d] bits per pixel.\n",
                  image -> bits_per_pixel);
      #endif

      return 100;
    }
  }

  #ifdef TEST
  fprintf(stderr, "nxagentUniquePixels: Sorting [%d] elements in the list.\n", i);
  #endif

  qsort(pixels, elements, sizeof(int), nxagentComparePixels);

  for (i = 0; i < elements; i++)
  {
    if (last != pixels[i])
    {
      unique++;

      last = pixels[i];
    }

    #ifdef DEBUG
    fprintf(stderr, "nxagentUniquePixels: pixels[%d][0x%08x].\n",
                i, pixels[i]);
    #endif
  }

  ratio = unique * 100 / elements;

  #ifdef TEST
  fprintf(stderr, "nxagentUniquePixels: Found [%d] unique pixels out of [%d] with ratio [%d%%].\n",
              unique, elements, ratio);
  #endif

  return ratio;
}

unsigned int Get16(const char *buffer, int order)
{
  unsigned int result;

  if (order == MSBFirst)
  {
    result = *buffer;

    result <<= 8;

    result += buffer[1];
  }
  else
  {
    result = buffer[1];

    result <<= 8;

    result += *buffer;
  }

  return result;
}

unsigned int Get24(const char *buffer, int order)
{
  int i;

  const char *next = (order == MSBFirst ? buffer : buffer + 2);

  unsigned int result = 0;

  for (i = 0; i < 3; i++)
  {
    result <<= 8;

    result += *next;

    if (order == MSBFirst)
    {
      next++;
    }
    else
    {
      next--;
    }
  }

  return result;
}

unsigned int Get32(const char *buffer, int order)
{
  int i;

  const char *next = (order == MSBFirst ? buffer : buffer + 3);

  unsigned int result = 0;

  for (i = 0; i < 4; i++)
  {
    result <<= 8;

    result += *next;

    if (order == MSBFirst)
    {
      next++;
    }
    else
    {
      next--;
    }
  }

  return result;
}

void Put16(unsigned int value, char *buffer, int order)
{
  if (order == MSBFirst)
  {
    buffer[1] = (unsigned char) (value & 0xff);

    value >>= 8;

    *buffer = (unsigned char) value;
  }
  else
  {
    *buffer = (unsigned char) (value & 0xff);

    value >>= 8;

    buffer[1] = (unsigned char) value;
  }
}

void Put24(unsigned int value, char *buffer, int order)
{
  int i;

  if (order == MSBFirst)
  {
    buffer += 2;

    for (i = 3; i > 0; i--)
    {
      *buffer-- = (unsigned char) (value & 0xff);

      value >>= 8;
    }
  }
  else
  {
    for (i = 3; i > 0; i--)
    {
      *buffer++ = (unsigned char) (value & 0xff);

      value >>= 8;
    }
  }
}

void Put32(unsigned int value, char *buffer, int order)
{
  int i;

  if (order == MSBFirst)
  {
    buffer += 3;

    for (i = 4; i > 0; i--)
    {
      *buffer-- = (unsigned char) (value & 0xff);

      value >>= 8;
    }
  }
  else
  {
    for (i = 4; i > 0; i--)
    {
      *buffer++ = (unsigned char) (value & 0xff);

      value >>= 8;
    }
  }
}
