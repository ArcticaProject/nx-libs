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

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#include "NXpack.h"

const ColorMask Mask8TrueColor    = { 128, 63, 240, 7 };
const ColorMask Mask64TrueColor   = { 192, 7,  240, 4 };
const ColorMask Mask256TrueColor  = { 255, 0,  255, 0 };
const ColorMask Mask512TrueColor  = { 224, 5,  240, 4 };
const ColorMask Mask4KTrueColor   = { 240, 4,  240, 2 };
const ColorMask Mask32KTrueColor  = { 248, 3,  248, 2 };
const ColorMask Mask64KTrueColor  = { 255, 0,  255, 0 };
const ColorMask Mask256KTrueColor = { 252, 1,  252, 1 };
const ColorMask Mask2MTrueColor   = { 255, 0,  254, 1 };
const ColorMask Mask16MTrueColor  = { 255, 0,  255, 0 };

const ColorMask *MethodColorMask(unsigned int method)
{
  switch (method)
  {
    case MASK_8_COLORS:
    {
      return &Mask8TrueColor;
    }
    case MASK_64_COLORS:
    {
      return &Mask64TrueColor;
    }
    case MASK_256_COLORS:
    {
      return &Mask256TrueColor;
    }
    case MASK_512_COLORS:
    {
      return &Mask512TrueColor;
    }
    case MASK_4K_COLORS:
    {
      return &Mask4KTrueColor;
    }
    case MASK_32K_COLORS:
    {
      return &Mask32KTrueColor;
    }
    case MASK_64K_COLORS:
    {
      return &Mask64KTrueColor;
    }
    case MASK_256K_COLORS:
    {
      return &Mask256KTrueColor;
    }
    case MASK_2M_COLORS:
    {
      return &Mask2MTrueColor;
    }
    case MASK_16M_COLORS:
    {
      return &Mask16MTrueColor;
    }
    default:
    {
      return NULL;
    }
  }
}

int MethodBitsPerPixel(unsigned int method)
{
  switch (method)
  {
    case PACK_MASKED_8_COLORS:
    case PACK_JPEG_8_COLORS:
    case PACK_PNG_8_COLORS:
    {
      return 8;
    }
    case PACK_MASKED_64_COLORS:
    case PACK_JPEG_64_COLORS:
    case PACK_PNG_64_COLORS:
    {
      return 8;
    }
    case PACK_MASKED_256_COLORS:
    case PACK_JPEG_256_COLORS:
    case PACK_PNG_256_COLORS:
    {
      return 8;
    }
    case PACK_MASKED_512_COLORS:
    case PACK_JPEG_512_COLORS:
    case PACK_PNG_512_COLORS:
    {
      return 16;
    }
    case PACK_MASKED_4K_COLORS:
    case PACK_JPEG_4K_COLORS:
    case PACK_PNG_4K_COLORS:
    {
      return 16;
    }
    case PACK_MASKED_32K_COLORS:
    case PACK_JPEG_32K_COLORS:
    case PACK_PNG_32K_COLORS:
    {
      return 16;
    }
    case PACK_MASKED_64K_COLORS:
    case PACK_JPEG_64K_COLORS:
    case PACK_PNG_64K_COLORS:
    {
      return 16;
    }
    case PACK_MASKED_256K_COLORS:
    case PACK_JPEG_256K_COLORS:
    case PACK_PNG_256K_COLORS:
    {
      return 24;
    }
    case PACK_MASKED_2M_COLORS:
    case PACK_JPEG_2M_COLORS:
    case PACK_PNG_2M_COLORS:
    {
      return 24;
    }
    case PACK_MASKED_16M_COLORS:
    case PACK_JPEG_16M_COLORS:
    case PACK_PNG_16M_COLORS:
    {
      return 24;
    }
    case PACK_BITMAP_16M_COLORS:
    case PACK_RGB_16M_COLORS:
    case PACK_RLE_16M_COLORS:
    {
      return 24;
    }
    default:
    {
      return 0;
    }
  }
}

#ifdef __cplusplus
}
#endif
