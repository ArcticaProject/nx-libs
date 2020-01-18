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
#include "inputstr.h"
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

#include "compext/Compext.h"

/*
 * Set here the required log level.
 */

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

/*
 * The nxagentReversePointerMap array is used to memorize remote
 * display pointer map.
 */

unsigned char nxagentReversePointerMap[MAXBUTTONS];

DeviceIntPtr nxagentPointerDevice = NULL;

void nxagentChangePointerControl(DeviceIntPtr pDev, PtrCtrl *ctrl)
{
  /*
   * The original behaviour was to reset the pointer settings
   * (acceleration and alas) to the default values. What the average
   * user expects, on the contrary, is to have agent inheriting
   * whatever value is set on the real X display.  Having to reflect
   * changes made inside the agent session, the correct behavior would
   * be saving the original values and restoring them as soon as focus
   * leaves the agent's window.
   */

  if (nxagentOption(DeviceControl) == True)
  {
    #ifdef TEST
    fprintf(stderr, "%s: WARNING! Propagating changes to pointer settings.\n", __func__);
    #endif

    XChangePointerControl(nxagentDisplay, True, True, 
                              ctrl->num, ctrl->den, ctrl->threshold);

    return;
  }

  #ifdef TEST
  fprintf(stderr, "%s: WARNING! Not propagating changes to pointer settings.\n", __func__);
  #endif
}

int nxagentPointerProc(DeviceIntPtr pDev, int onoff)
{
  switch (onoff)
  {
    case DEVICE_INIT:

      #ifdef TEST
      fprintf(stderr, "%s: Called for [DEVICE_INIT].\n", __func__);
      #endif

      if (NXDisplayError(nxagentDisplay) == 1)
      {
        return Success;
      }

      CARD8 map[MAXBUTTONS];

      int nmap = XGetPointerMapping(nxagentDisplay, map, MAXBUTTONS);
      for (int i = 0; i <= nmap; i++)
	map[i] = i; /* buttons are already mapped */
      InitPointerDeviceStruct((DevicePtr) pDev, map, nmap,
			      miPointerGetMotionEvents,
			      nxagentChangePointerControl,
			      miPointerGetMotionBufferSize(), 2);
      break;
    case DEVICE_ON:

      #ifdef TEST
      fprintf(stderr, "%s: Called for [DEVICE_ON].\n", __func__);
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
      fprintf(stderr, "%s: Called for [DEVICE_OFF].\n", __func__);
      #endif

      if (NXDisplayError(nxagentDisplay) == 1)
      {
        return Success;
      }

      nxagentDisablePointerEvents();

      break;

    case DEVICE_CLOSE:
      #ifdef TEST
      fprintf(stderr, "%s: Called for [DEVICE_CLOSE].\n", __func__);
      #endif

      break;
    }

  return Success;
}

void nxagentInitPointerMap(void)
{
  unsigned char pointerMap[MAXBUTTONS];

  #ifdef DEBUG
  fprintf(stderr, "%s: Going to retrieve the "
	  "pointer map from remote display.\n", __func__);
  #endif

  int numButtons = XGetPointerMapping(nxagentDisplay, pointerMap, MAXBUTTONS);

  /*
   * Computing reverse pointer map.
   */

  for (int i = 1; i <= numButtons; i++)
  {
    nxagentReversePointerMap[pointerMap[i - 1]] = i;
  }
}
