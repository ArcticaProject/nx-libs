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

#ifndef Alpha_H
#define Alpha_H

#ifdef __cplusplus
extern "C" {
#endif

extern char *AlphaCompressData(
#if NeedFunctionPrototypes
    const char*       /* data */,
    unsigned int      /* size */,
    unsigned int*     /* compressed_size */
#endif
);

#ifdef __cplusplus
}
#endif

#endif /* Alpha_H */
