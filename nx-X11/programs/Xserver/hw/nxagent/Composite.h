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
