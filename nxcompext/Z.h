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

#ifndef Z_H
#define Z_H

#ifdef __cplusplus
extern "C" {
#endif

int ZInitEncoder(
#if NeedFunctionPrototypes
void
#endif
);

int ZResetEncoder(
#if NeedFunctionPrototypes
void
#endif
);

extern char *ZCompressData(
#if NeedFunctionPrototypes
    const char*     /* data */,
    unsigned int    /* size */,
    int             /* threshold */,
    int             /* level */,
    int             /* strategy */,
    unsigned int*   /* compressed_size */
#endif
);

#ifdef __cplusplus
}
#endif

#endif /* Z_H */
