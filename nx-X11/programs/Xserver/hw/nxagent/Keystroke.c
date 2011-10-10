/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2009 NoMachine, http://www.nomachine.com/.         */
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

#include "X.h"
#include "keysym.h"

#include "screenint.h"
#include "scrnintstr.h"

#include "Agent.h"
#include "Display.h"
#include "Events.h"
#include "Options.h"
#include "Keystroke.h"
#include "Drawable.h"

extern Bool nxagentWMIsRunning;
extern Bool nxagentIpaq;

#ifdef NX_DEBUG_INPUT
int nxagentDebugInputDevices = 0;
unsigned long nxagentLastInputDevicesDumpTime = 0;
extern void nxagentDeactivateInputDevicesGrabs();
#endif

/*
 * Set here the required log level.
 */

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG
#undef  DUMP

int nxagentCheckSpecialKeystroke(XKeyEvent *X, enum HandleEventResult *result)
{
  KeySym sym;
  int index = 0;

  *result = doNothing;

  /*
   * I don't know how much hard work is doing this operation.
   * Do we need a cache ?
   */

  sym = XKeycodeToKeysym(nxagentDisplay, X -> keycode, index);

  if (sym == XK_VoidSymbol || sym == NoSymbol)
  {
    return 0;
  }

  #ifdef TEST
  fprintf(stderr, "nxagentCheckSpecialKeystroke: got code %x - state %x - sym %lx\n",
              X -> keycode, X -> state, sym);
  #endif

  /*
   * Check special keys.
   */

  /*
   * FIXME: We should use the keysym instead that the keycode
   *        here.
   */

  if (X -> keycode == 130 && nxagentIpaq)
  {
    *result = doStartKbd;

    return 1;
  }

  if ((X -> state & nxagentAltMetaMask) &&
          ((X -> state & (ControlMask | ShiftMask)) == ControlMask))
  {
    switch (sym)
    {
      case XK_t:
      case XK_T:
      {
        *result = doCloseSession;

        break;
      }
      case XK_f:
      case XK_F:
      {
        if (nxagentOption(Rootless) == False)
        {
          *result = doSwitchFullscreen;
        }

        break;
      }
      case XK_m:
      case XK_M:
      {
        if (nxagentOption(Rootless) == False)
        {
          *result = doMinimize;
        }

        break;
      }
      case XK_Left:
      case XK_KP_Left:
      {
        if (nxagentOption(Rootless) == False &&
                nxagentOption(DesktopResize) == False)
        {
          *result = doViewportLeft;
        }

        break;
      }
      case XK_Up:
      case XK_KP_Up:
      {
        if (nxagentOption(Rootless) == False &&
                nxagentOption(DesktopResize) == False)
        {
          *result = doViewportUp;
        }

        break;
      }
      case XK_Right:
      case XK_KP_Right:
      {
        if (nxagentOption(Rootless) == False &&
                nxagentOption(DesktopResize) == False)
        {
          *result = doViewportRight;
        }

        break;
      }
      case XK_Down:
      case XK_KP_Down:
      {
        if (nxagentOption(Rootless) == 0 &&
                nxagentOption(DesktopResize) == 0)
        {
          *result = doViewportDown;
        }

        break;
      }
      case XK_R:
      case XK_r:
      {
        if (nxagentOption(Rootless) == 0)
        {
          *result = doSwitchResizeMode;
        }

        break;
      }
      case XK_E:
      case XK_e:
      {
        *result = doSwitchDeferMode;

        break;
      }
      case XK_BackSpace:
      case XK_Terminate_Server:
      {
        /*
         * Discard Ctrl-Alt-BackSpace key.
         */

        return 1;

        break;
      }

      case XK_J:
      case XK_j:
      {
        nxagentForceSynchronization = 1;

        return 1;
      }

      #ifdef DUMP

      case XK_A:
      case XK_a:
      {
        /*
         * Used to test the lazy encoding.
         */

        nxagentRegionsOnScreen();

        return 1;
      }

      #endif

      #ifdef NX_DEBUG_INPUT

      case XK_X:
      case XK_x:
      {
        /*
         * Used to test the input devices state.
         */

        if (X -> type == KeyPress)
        {
          if (nxagentDebugInputDevices == 0)
          {
            fprintf(stderr, "Info: Turning input devices debug ON.\n");
    
            nxagentDebugInputDevices = 1;
          }
          else
          {
            fprintf(stderr, "Info: Turning input devices debug OFF.\n");
    
            nxagentDebugInputDevices = 0;
    
            nxagentLastInputDevicesDumpTime = 0;
          }
        }

        return 1;
      }

      case XK_Y:
      case XK_y:
      {
        /*
         * Used to deactivate input devices grab.
         */

        if (X -> type == KeyPress)
        {
          nxagentDeactivateInputDevicesGrabs();
        }

        return 1;
      }

      #endif
    }
  }

  return (*result == doNothing) ? 0 : 1;
}
