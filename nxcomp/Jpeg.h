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

#ifndef Jpeg_H
#define Jpeg_H

#include "Misc.h"
#include "Unpack.h"

int UnpackJpeg(T_geometry *geometry, unsigned char method, unsigned char *srcData,
                   int srcSize, int dstBpp, int dstWidth, int dstHeight,
                       unsigned char *dstData, int dstSize);

#endif /* Jpeg_H */
