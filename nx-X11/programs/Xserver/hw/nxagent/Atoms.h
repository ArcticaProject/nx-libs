/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2011 NoMachine, http://www.nomachine.com/.         */
/*                                                                        */
/* NXAGENT, NX protocol compression and NX extensions to this software    */
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

#ifndef __Atoms_H__
#define __Atoms_H__

#include "X.h"
#include "../../include/window.h"
#include "screenint.h"

#define NXAGENT_NUMBER_OF_ATOMS  16

extern Atom nxagentAtoms[NXAGENT_NUMBER_OF_ATOMS];

extern Bool nxagentWMIsRunning;

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
