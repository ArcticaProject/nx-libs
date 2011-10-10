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

/*
 * Initialize the additional extensions.
 */

void nxagentInitGlxExtension(VisualPtr *visuals, DepthPtr *depths,
                                 int *numVisuals, int *numDepths, int *rootDepth,
                                     VisualID *defaultVisual);

void nxagentInitRandRExtension(ScreenPtr pScreen);

/*
 * Basic interface to the RandR extension.
 */

int nxagentRandRGetInfo(ScreenPtr pScreen, Rotation *pRotations);

int nxagentRandRSetConfig(ScreenPtr pScreen, Rotation rotation,
                              int rate, RRScreenSizePtr pSize);
