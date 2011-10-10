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

Copyright 1993 by Davor Matic

Permission to use, copy, modify, distribute, and sell this software
and its documentation for any purpose is hereby granted without fee,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation.  Davor Matic makes no representations about
the suitability of this software for any purpose.  It is provided "as
is" without express or implied warranty.

*/

#include "X.h"
#include "Xproto.h"
#include "screenint.h"
#include "input.h"
#include "misc.h"
#include "scrnintstr.h"
#include "servermd.h"
#include "mipointer.h"

#include "Agent.h"
#include "Args.h"
#include "Display.h"
#include "Screen.h"
#include "Pointer.h"
#include "Events.h"
#include "Options.h"

#include "NXlib.h"

/*
 * Set here the required log level.
 */

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

/*
 * The nxagentReversePointerMap array is used to
 * memorize remote display pointer map.
 */

unsigned char nxagentReversePointerMap[MAXBUTTONS];

void nxagentChangePointerControl(DeviceIntPtr pDev, PtrCtrl *ctrl)
{
  /*
   * The original behaviour was to reset the pointer settings
   * (acceleration and alas) to the default values. What the
   * average user expects, on the contrary, is to have agent
   * inheriting whatever value is set on the real X display.
   * Having to reflect changes made inside the agent session,
   * the correct behavior would be saving the original values
   * and restoring them as soon as focus leaves the agent's
   * window.
   */

  if (nxagentOption(DeviceControl) == True)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentChangePointerControl: WARNING! Propagating changes to pointer settings.\n");
    #endif

    XChangePointerControl(nxagentDisplay, True, True, 
                              ctrl->num, ctrl->den, ctrl->threshold);

    return;
  }

  #ifdef TEST
  fprintf(stderr, "nxagentChangePointerControl: WARNING! Not propagating changes to pointer settings.\n");
  #endif
}

int nxagentPointerProc(DeviceIntPtr pDev, int onoff)
{
  CARD8 map[MAXBUTTONS];
  int nmap;
  int i;

  switch (onoff)
  {
    case DEVICE_INIT:

      #ifdef TEST
      fprintf(stderr, "nxagentPointerProc: Called for [DEVICE_INIT].\n");
      #endif

      if (NXDisplayError(nxagentDisplay) == 1)
      {
        return Success;
      }

      nmap = XGetPointerMapping(nxagentDisplay, map, MAXBUTTONS);
      for (i = 0; i <= nmap; i++)
	map[i] = i; /* buttons are already mapped */
      InitPointerDeviceStruct((DevicePtr) pDev, map, nmap,
			      miPointerGetMotionEvents,
			      nxagentChangePointerControl,
			      miPointerGetMotionBufferSize());
      break;
    case DEVICE_ON:

      #ifdef TEST
      fprintf(stderr, "nxagentPointerProc: Called for [DEVICE_ON].\n");
      #endif

      if (NXDisplayError(nxagentDisplay) == 1)
      {
        return Success;
      }

      nxagentInitPointerMap();

      nxagentEnablePointerEvents();

      break;

    case DEVICE_OFF:

      #ifdef TEST
      fprintf(stderr, "nxagentPointerProc: Called for [DEVICE_OFF].\n");
      #endif

      if (NXDisplayError(nxagentDisplay) == 1)
      {
        return Success;
      }

      nxagentDisablePointerEvents();

      break;

    case DEVICE_CLOSE:

      #ifdef TEST
      fprintf(stderr, "nxagentPointerProc: Called for [DEVICE_CLOSE].\n");
      #endif

      break;
    }

  return Success;
}

void nxagentInitPointerMap(void)
{
  int numButtons;

  int i;

  unsigned char pointerMap[MAXBUTTONS];

  #ifdef DEBUG
  fprintf(stderr, "nxagentInitPointerMap: Going to retrieve the "
              "pointer map from remote display.\n");
  #endif

  numButtons = XGetPointerMapping(nxagentDisplay, pointerMap, MAXBUTTONS);

  /*
   * Computing revers pointer map.
   */

  for (i = 1; i <= numButtons; i++)
  {
    nxagentReversePointerMap[pointerMap[i - 1] - 1] = i;
  }
}
