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

#ifndef Bitmap_H
#define Bitmap_H

#include "Unpack.h"

int UnpackBitmap(T_geometry *geometry, unsigned char method,
                     unsigned char *src_data, int src_size, int dst_bpp,
                         int dst_width, int dst_height, unsigned char *dst_data,
                             int dst_size);

#endif /* Bitmap_H */
