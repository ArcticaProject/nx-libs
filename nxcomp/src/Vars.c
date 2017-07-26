/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2011 NoMachine (http://www.nomachine.com)          */
/* Copyright (c) 2008-2014 Oleksandr Shneyder <o.shneyder@phoca-gmbh.de>  */
/* Copyright (c) 2014-2016 Ulrich Sibiller <uli42@gmx.de>                 */
/* Copyright (c) 2014-2016 Mihai Moldovan <ionic@ionic.de>                */
/* Copyright (c) 2011-2016 Mike Gabriel <mike.gabriel@das-netzwerkteam.de>*/
/* Copyright (c) 2015-2016 Qindel Group (http://www.qindel.com)           */
/*                                                                        */
/* NXCOMP, NX protocol compression and NX extensions to this software     */
/* are copyright of the aforementioned persons and companies.             */
/*                                                                        */
/* Redistribution and use of the present software is allowed according    */
/* to terms specified in the file LICENSE.nxcomp which comes in the       */
/* source distribution.                                                   */
/*                                                                        */
/* All rights reserved.                                                   */
/*                                                                        */
/* NOTE: This software has received contributions from various other      */
/* contributors, only the core maintainers and supporters are listed as   */
/* copyright holders. Please contact us, if you feel you should be listed */
/* as copyright holder, as well.                                          */
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
