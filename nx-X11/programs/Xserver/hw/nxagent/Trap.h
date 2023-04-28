/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2011 NoMachine (http://www.nomachine.com)          */
/* Copyright (c) 2008-2017 Oleksandr Shneyder <o.shneyder@phoca-gmbh.de>  */
/* Copyright (c) 2011-2022 Mike Gabriel <mike.gabriel@das-netzwerkteam.de>*/
/* Copyright (c) 2014-2019 Mihai Moldovan <ionic@ionic.de>                */
/* Copyright (c) 2014-2022 Ulrich Sibiller <uli42@gmx.de>                 */
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

#ifndef __Traps_H__
#define __Traps_H__

#include <nx-X11/Xdefs.h>

#ifndef False
#define False 0
#endif

#ifndef True
#define True 1
#endif

/*
 * Set if we are dispatching a render extension request. Used to avoid
 * reentrancy in GC operations.
 */
extern Bool nxagentGCTrap;

/*
 * Set if we are enqueing an internal operation, CreateWindow and
 * Reparent- Window. Used to remove any screen operation.
 */
extern Bool nxagentScreenTrap;

/*
 * Set if we are executing a GC operation only on the X side. Used to
 * avoid reentrancy in FB layer.
 */
extern Bool nxagentFBTrap;

/*
 * Set if we are dispatching a shared memory extension request.
 */
extern Bool nxagentShmTrap;

/*
 * Set if a shared pixmap operation is requested by the client.
 */
extern Bool nxagentShmPixmapTrap;

/*
 * Set if we are dispatching a XVideo extension request.
 */
extern Bool nxagentXvTrap;

/*
 * Set if we are dispatching a GLX extension request.
 */
extern Bool nxagentGlxTrap;

/*
 * Set while we are resuming the session.
 */
extern Bool nxagentReconnectTrap;

/*
 * Set if we need to realize a drawable by using a lossless encoding.
 */
extern Bool nxagentLosslessTrap;

/*
 * Set to force the synchronization of a drawable.
 */
extern Bool nxagentSplitTrap;

/*
 * Set to avoid CapsLock synchronization problems when CapsLock is the
 * first key to be pressed in the session.
 */
extern Bool nxagentXkbCapsTrap;

/*
 * Set to avoid NumLock synchronization problems when NumLock is the
 * first key to be pressed in the session.
 */
extern Bool nxagentXkbNumTrap;

#endif /* __Trap_H__ */
