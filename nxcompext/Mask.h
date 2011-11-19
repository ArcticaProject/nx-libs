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

#ifndef Mask_H
#define Mask_H

#ifdef __cplusplus
extern "C" {
#endif

#include "Xlib.h"

extern int MaskImage(const ColorMask *mask, XImage *src_image, XImage *dst_image);

extern int MaskInPlaceImage(const ColorMask *mask, XImage *image);

extern int PackImage(unsigned int method, unsigned int src_data_size, XImage *src_image,
                         unsigned int dst_data_size, XImage *dst_image);

int FindLSB(int word);

#ifdef __cplusplus
}
#endif

#endif /* Mask_H */
