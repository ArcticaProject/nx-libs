/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2011 NoMachine (http://www.nomachine.com)          */
/* Copyright (c) 2008-2014 Oleksandr Shneyder <o.shneyder@phoca-gmbh.de>  */
/* Copyright (c) 2014-2016 Ulrich Sibiller <uli42@gmx.de>                 */
/* Copyright (c) 2014-2016 Mihai Moldovan <ionic@ionic.de>                */
/* Copyright (c) 2011-2016 Mike Gabriel <mike.gabriel@das-netzwerkteam.de>*/
/* Copyright (c) 2015-2016 Qindel Group (http://www.qindel.com)           */
/*                                                                        */
/* NXCOMPEXT, NX protocol compression and NX extensions to this software  */
/* are copyright of the aforementioned persons and companies.             */
/*                                                                        */
/* Redistribution and use of the present software is allowed according    */
/* to terms specified in the file LICENSE which comes in the source       */
/* distribution.                                                          */
/*                                                                        */
/* All rights reserved.                                                   */
/*                                                                        */
/* NOTE: This software has received contributions from various other      */
/* contributors, only the core maintainers and supporters are listed as   */
/* copyright holders. Please contact us, if you feel you should be listed */
/* as copyright holder, as well.                                          */
/*                                                                        */
/**************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "Compext.h"

#include "Bitmap.h"

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

char *BitmapCompressData(XImage *image, unsigned int *size)
{
  if (image -> bits_per_pixel != 32)
  {
    #ifdef TEST
    fprintf(stderr, "******BitmapCompressData: Nothing to do with image of [%d] bpp and size [%d].\n",
                image -> bits_per_pixel, image -> bytes_per_line * image -> height);
    #endif

    *size = image -> bytes_per_line * image -> height;

    return image -> data;
  }
  else
  {
    /*
     * Remove the 4th byte from the bitmap.
     */

    char *data;

    char *next_src;
    char *next_dst;

    #ifdef TEST

    if (image -> bytes_per_line != 4 * image -> width)
    {
      fprintf(stderr, "******BitmapCompressData: PANIC! Image as [%d] bytes per line with expected [%d].\n",
                  image -> bytes_per_line, 4 * image -> width);

      return NULL;
    }

    #endif

    *size = image -> width * image -> height * 3;

    data = malloc(*size);

    if (data == NULL)
    {
      #ifdef PANIC
      fprintf(stderr, "******BitmapCompressData: PANIC! Failed to allocate [%d] bytes for the destination.\n",
                  *size);
      #endif

      *size = image -> bytes_per_line * image -> height;

      return image -> data;
    }

    next_src = image -> data;
    next_dst = data;

    if (image -> byte_order == LSBFirst)
    {
      while (next_src < image -> data +
                 image -> bytes_per_line * image -> height)
      {
        *next_dst++ = *next_src++;
        *next_dst++ = *next_src++;
        *next_dst++ = *next_src++;

        next_src++;
      }
    }
    else
    {
      while (next_src < image -> data +
                 image -> bytes_per_line * image -> height)
      {
        next_src++;

        *next_dst++ = *next_src++;
        *next_dst++ = *next_src++;
        *next_dst++ = *next_src++;
      }
    }

    return data;
  }
}
