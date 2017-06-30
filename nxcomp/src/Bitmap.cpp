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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "Misc.h"
#include "Bitmap.h"

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

int UnpackBitmap(T_geometry *geometry, unsigned char method, unsigned char *src_data,
                  int src_size, int dst_bpp, int dst_width, int dst_height,
                      unsigned char *dst_data, int dst_size)
{
  if (dst_bpp != 32)
  {
    #ifdef TEST
    *logofs << "UnpackBitmap: Nothing to do with "
            << "image of " << dst_bpp << " bits per plane "
            << "and size " << src_size << ".\n"
            << logofs_flush;
    #endif

    if (src_size != dst_size)
    {
      #ifdef PANIC
      *logofs << "UnpackBitmap: PANIC! Size mismatch with "
              << src_size << " bytes in the source and "
              << dst_size << " in the destination.\n"
              << logofs_flush;
      #endif

      return -1;
    }

    memcpy(dst_data, src_data, src_size);

    return 1;
  }
  else if (src_size != dst_width * dst_height * 3 ||
               dst_size != dst_width * dst_height * 4)
  {
    #ifdef PANIC
    *logofs << "UnpackBitmap: PANIC! Size mismatch with "
            << src_size << " bytes in the source and "
            << dst_size << " in the destination.\n"
            << logofs_flush;
    #endif

    return -1;
  }

  /*
   * Insert the 4th byte in the bitmap.
   */

  unsigned char *next_src = src_data;
  unsigned char *next_dst = dst_data;

  if (geometry -> image_byte_order == LSBFirst)
  {
    while (next_src < src_data + src_size)
    {
      *next_dst++ = *next_src++;
      *next_dst++ = *next_src++;
      *next_dst++ = *next_src++;

      next_dst++;
    }
  }
  else
  {
    while (next_src < src_data + src_size)
    {
      next_dst++;

      *next_dst++ = *next_src++;
      *next_dst++ = *next_src++;
      *next_dst++ = *next_src++;
    }
  }

  #ifdef TEST
  *logofs << "UnpackBitmap: Unpacked " << src_size
          << " bytes to a buffer of " << dst_size
          << " with " << dst_bpp << " bits per plane.\n"
          << logofs_flush;
  #endif

  return 1;
}
