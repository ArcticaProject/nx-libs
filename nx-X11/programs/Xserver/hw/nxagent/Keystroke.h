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

/* keep this sorted, do not rely on any numerical value in this enum, and be aware
 * that KEYSTROKE_MAX may be used in a malloc */

/* also be aware that if changing any numerical values, you also need to change values
 * Keystroke.c nxagentSpecialKeystrokeNames */
enum nxagentSpecialKeystroke {
       /* 0 is used as end marker */
       KEYSTROKE_END_MARKER = 0,
       KEYSTROKE_CLOSE_SESSION = 1,
       KEYSTROKE_SWITCH_ALL_SCREENS = 2,
       KEYSTROKE_FULLSCREEN = 3,
       KEYSTROKE_MINIMIZE = 4,
       KEYSTROKE_LEFT = 5,
       KEYSTROKE_UP = 6,
       KEYSTROKE_RIGHT = 7,
       KEYSTROKE_DOWN = 8,
       KEYSTROKE_RESIZE = 9,
       KEYSTROKE_DEFER = 10,
       KEYSTROKE_IGNORE = 11,
       KEYSTROKE_FORCE_SYNCHRONIZATION = 12,

       /* stuff used for debugging, probably not useful for most people */
       KEYSTROKE_DEBUG_TREE = 13,
       KEYSTROKE_REGIONS_ON_SCREEN = 14,
       KEYSTROKE_TEST_INPUT = 15,
       KEYSTROKE_DEACTIVATE_INPUT_DEVICES_GRAB = 16,

       KEYSTROKE_VIEWPORT_MOVE_LEFT = 17,
       KEYSTROKE_VIEWPORT_MOVE_UP = 18,
       KEYSTROKE_VIEWPORT_MOVE_RIGHT = 19,
       KEYSTROKE_VIEWPORT_MOVE_DOWN = 20,

       KEYSTROKE_REREAD_KEYSTROKES = 21,

       KEYSTROKE_NOTHING = 22,

       /* insert more here, increment KEYSTROKE_MAX accordingly.
        * then update string translation below */

       KEYSTROKE_MAX = 23,
};

struct nxagentSpecialKeystrokeMap {
       enum nxagentSpecialKeystroke stroke;
       unsigned int modifierMask; /* everything except alt/meta */
       Bool modifierAltMeta; /* modifier combination should include alt/meta */
       KeySym keysym;
};

#endif /* __Keystroke_H__ */
