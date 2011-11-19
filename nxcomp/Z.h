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

#ifndef Z_H
#define Z_H

#include <zlib.h>

int ZCompress(z_stream *stream, unsigned char *dest, unsigned int *destLen,
                  const unsigned char *source, unsigned int sourceLen);

int ZDecompress(z_stream *stream, unsigned char *dest, unsigned int *destLen,
                    const unsigned char *source, unsigned int sourceLen);

#endif /* Z_H */
