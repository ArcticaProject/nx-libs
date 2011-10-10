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

#ifndef __Composite_H__
#define __Composite_H__

/*
 * Set if the extension is present and
 * its use is enabled.
 */

extern int nxagentCompositeEnable;

/*
 * Query the composite extension on the
 * remote display and set the flag if
 * it is supported.
 */

void nxagentCompositeExtensionInit(void);

/*
 * Let the X server redirect the window
 * on the off-screen memory.
 */

void nxagentRedirectDefaultWindows(void);

/*
 * Enable or disabel the redirection of
 * the given window.
 */

void nxagentRedirectWindow(WindowPtr pWin);
void nxagentUnredirectWindow(WindowPtr pWin);

#endif /* __Composite_H__ */
