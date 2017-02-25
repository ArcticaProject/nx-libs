/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2011 NoMachine (http://www.nomachine.com)          */
/* Copyright (c) 2008-2014 Oleksandr Shneyder <o.shneyder@phoca-gmbh.de>  */
/* Copyright (c) 2011-2016 Mike Gabriel <mike.gabriel@das-netzwerkteam.de>*/
/* Copyright (c) 2014-2016 Mihai Moldovan <ionic@ionic.de>                */
/* Copyright (c) 2014-2016 Ulrich Sibiller <uli42@gmx.de>                 */
/* Copyright (c) 2015-2016 Qindel Group (http://www.qindel.com)           */
/*                                                                        */
/* NXAGENT, NX protocol compression and NX extensions to this software    */
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

#ifndef __Atoms_H__
#define __Atoms_H__

#include "X.h"
#include "../../include/window.h"
#include "screenint.h"

#define NXAGENT_NUMBER_OF_ATOMS  16

extern Atom nxagentAtoms[NXAGENT_NUMBER_OF_ATOMS];

extern Bool nxagentWMIsRunning;

extern const char * validateString(const char *);

/*
 * Create the required atoms internally
 * to the agent server.
 */

int nxagentInitAtoms(WindowPtr pWin);

/*
 * Query and create all the required atoms
 * on the remote X server using a single
 * round trip.
 */

int nxagentQueryAtoms(ScreenPtr pScreen);

/*
 * Create the atoms on the remote X server
 * and cache the couple local-remote atoms.
 */

Atom nxagentMakeAtom(char *, unsigned, Bool);

/*
 * Converts local atoms in remote atoms and
 * viceversa.
 */

Atom nxagentRemoteToLocalAtom(Atom);
Atom nxagentLocalToRemoteAtom(Atom);

void nxagentResetAtomMap(void);

void nxagentWMDetect(void);

#endif /* __Atoms_H__ */
