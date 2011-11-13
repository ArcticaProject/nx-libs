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

#ifndef Pgn_H
#define Pgn_H

#ifdef __cplusplus
extern "C" {
#endif

#include "X11/X.h"
#include "X11/Xlib.h"
#include "X11/Xmd.h"

#include <png.h> 

extern int PngCompareColorTable(
#if NeedFunctionPrototypes
  NXColorTable*     /* color_table_1 */,
  NXColorTable*     /* color_table_2 */
#endif
);

extern char *PngCompressData(
#if NeedFunctionPrototypes
    XImage*          /* image */,
    int*             /* compressed_size */
#endif
);

int NXCreatePalette16(
#if NeedFunctionPrototypes
    XImage*          /* src_image */,
    NXColorTable*    /* color_table */,
    CARD8*           /* image_index */,
    int              /* nb_max */
#endif
);

int NXCreatePalette32(
#if NeedFunctionPrototypes
    XImage*          /* src_image */,
    NXColorTable*    /* color_table */,
    CARD8*           /* image_index */,
    int              /* nb_max */
#endif
);

#ifdef __cplusplus
}
#endif

#endif /* Pgn_H */

