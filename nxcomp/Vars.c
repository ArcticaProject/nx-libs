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

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>

#include "NXvars.h"

/*
 * Allocate here instances of variables and
 * pointers declared in NXvars.h.
 */
 
int _NXHandleDisplayError = 0;

NXDisplayErrorPredicate _NXDisplayErrorFunction = NULL;

int _NXUnsetLibraryPath = 0;

NXLostSequenceHandler _NXLostSequenceFunction = NULL;

NXDisplayBlockHandler _NXDisplayBlockFunction = NULL;
NXDisplayWriteHandler _NXDisplayWriteFunction = NULL;
NXDisplayFlushHandler _NXDisplayFlushFunction = NULL;
NXDisplayStatisticsHandler _NXDisplayStatisticsFunction = NULL;

#ifdef __cplusplus
}
#endif
