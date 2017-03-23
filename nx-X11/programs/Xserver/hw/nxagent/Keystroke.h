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

#ifndef __Keystroke_H__
#define __Keystroke_H__

#include "Events.h"

extern Bool nxagentCheckSpecialKeystroke(XKeyEvent*, enum HandleEventResult*);
extern void nxagentDumpKeystrokes(void);

/* keep this sorted, do not rely on any numerical value in this enum, and be aware
 * that KEYSTROKE_MAX may be used in a malloc */

/* also be aware that if changing any numerical values, you also need to change values
 * Keystroke.c nxagentSpecialKeystrokeNames */
enum nxagentSpecialKeystroke {
       /* 0 is used as end marker */
       KEYSTROKE_END_MARKER,
       KEYSTROKE_CLOSE_SESSION,
       KEYSTROKE_SWITCH_ALL_SCREENS,
       KEYSTROKE_FULLSCREEN,
       KEYSTROKE_MINIMIZE,
       KEYSTROKE_LEFT,
       KEYSTROKE_UP,
       KEYSTROKE_RIGHT,
       KEYSTROKE_DOWN,
       KEYSTROKE_RESIZE,
       KEYSTROKE_DEFER,
       KEYSTROKE_IGNORE,
       KEYSTROKE_FORCE_SYNCHRONIZATION,

       /* stuff used for debugging, probably not useful for most people */
#ifdef DEBUG_TREE
       KEYSTROKE_DEBUG_TREE,
#endif
#ifdef DUMP
       KEYSTROKE_REGIONS_ON_SCREEN,
#endif
#ifdef NX_DEBUG_INPUT
       KEYSTROKE_TEST_INPUT,
       KEYSTROKE_DEACTIVATE_INPUT_DEVICES_GRAB,
#endif

       KEYSTROKE_VIEWPORT_MOVE_LEFT,
       KEYSTROKE_VIEWPORT_MOVE_UP,
       KEYSTROKE_VIEWPORT_MOVE_RIGHT,
       KEYSTROKE_VIEWPORT_MOVE_DOWN,

       KEYSTROKE_REREAD_KEYSTROKES,

       KEYSTROKE_NOTHING,

       /* insert more here and in the string translation */

       KEYSTROKE_MAX,
};

struct nxagentSpecialKeystrokeMap {
       enum nxagentSpecialKeystroke stroke;
       unsigned int modifierMask; /* everything except alt/meta */
       Bool modifierAltMeta; /* modifier combination should include alt/meta */
       KeySym keysym;
};

#endif /* __Keystroke_H__ */
