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

#ifndef Pgn_H
#define Pgn_H

#ifdef __cplusplus
extern "C" {
#endif

#include <X11/X.h>
#include <nx-X11/Xlib.h>
#include <X11/Xmd.h>

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

