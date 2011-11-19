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
