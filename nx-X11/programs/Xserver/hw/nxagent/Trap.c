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

#include "Trap.h"

/*
 * Set if we are dispatching a render
 * extension request. Used to avoid
 * reentrancy in GC operations.
 */

int nxagentGCTrap = 0;

/*
 * Set if we are enqueing an internal
 * operation, CreateWindow and Reparent-
 * Window. Used to remove any screen operation.
 */

int nxagentScreenTrap = 0;

/*
 * Set if we detected that our RENDER
 * implementation is faulty.
 */

int nxagentRenderTrap = 0;

/*
 * Set if we are executing a GC operation
 * only on the X side. Used to avoid
 * reentrancy in FB layer.
 */

int nxagentFBTrap = 0;

/*
 * Set if we are dispatching a shared
 * memory extension request.
 */

int nxagentShmTrap = 0;

/*
 * Set if a shared pixmap operation is
 * requested by the client.
 */

int nxagentShmPixmapTrap = 0;

/*
 * Set if we are dispatching a XVideo
 * extension request.
 */

int nxagentXvTrap = 0;

/*
 * Set if we are dispatching a GLX
 * extension request.
 */

int nxagentGlxTrap = 0;

/*
 * Set while we are resuming the session.
 */

int nxagentReconnectTrap = 0;

/*
 * Set if we need to realize a drawable
 * by using a lossless encoding.
 */

int nxagentLosslessTrap = 0;

/*
 * Set to force the synchronization of
 * a drawable.
 */

int nxagentSplitTrap = 0;

/*
 * Set to avoid CapsLock synchronization
 * problems when CapsLock is the first
 * key to be pressed in the session.
 */

int nxagentXkbCapsTrap = 0;

/*
 * Set to avoid NumLock synchronization
 * problems when NumLock is the first
 * key to be pressed in the session.
 */

int nxagentXkbNumTrap = 0;

