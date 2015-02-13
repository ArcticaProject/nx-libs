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

#ifndef __Keystroke_H__
#define __Keystroke_H__

#include "Events.h"

extern int nxagentCheckSpecialKeystroke(XKeyEvent*, enum HandleEventResult*);

unsigned int nxagentAltMetaMask;

/* keep this sorted, do not rely on any numerical value in this enum, and be aware
 * that KEYSTROKE_MAX may be used in a malloc */

/* also be aware that if changing any numerical values, you also need to change values
 * Keystroke.c nxagentSpecialKeystrokeNames */
enum nxagentSpecialKeystroke {
       /* 0 is used as end marker */
       KEYSTROKE_END_MARKER = 0,
       KEYSTROKE_CLOSE_SESSION = 1,
       KEYSTROKE_SWITCH_ALL_SCREENS = 2,
       KEYSTROKE_MINIMIZE = 3,
       KEYSTROKE_LEFT = 4,
       KEYSTROKE_UP = 5,
       KEYSTROKE_RIGHT = 6,
       KEYSTROKE_DOWN = 7,
       KEYSTROKE_RESIZE = 8,
       KEYSTROKE_DEFER = 9,
       KEYSTROKE_IGNORE = 10,
       KEYSTROKE_FORCE_SYNCHRONIZATION = 11,

       /* stuff used for debugging, probably not useful for most people */
       KEYSTROKE_DEBUG_TREE = 12,
       KEYSTROKE_REGIONS_ON_SCREEN = 13,
       KEYSTROKE_TEST_INPUT = 14,
       KEYSTROKE_DEACTIVATE_INPUT_DEVICES_GRAB = 15,

       KEYSTROKE_FULLSCREEN = 16,
       KEYSTROKE_VIEWPORT_MOVE_LEFT = 17,
       KEYSTROKE_VIEWPORT_MOVE_UP = 18,
       KEYSTROKE_VIEWPORT_MOVE_RIGHT = 19,
       KEYSTROKE_VIEWPORT_MOVE_DOWN = 20,

       KEYSTROKE_NOTHING = 21,

       /* insert more here, increment KEYSTROKE_MAX accordingly.
        * then update string translation below */

       KEYSTROKE_MAX=22,
};

struct nxagentSpecialKeystrokeMap {
       enum nxagentSpecialKeystroke stroke;
       unsigned int modifierMask; /* everything except alt/meta */
       int modifierAltMeta; /* modifier combination should include alt/meta */
       KeySym keysym;
};

#endif /* __Keystroke_H__ */
