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

#include "windowstr.h"
#include "scrnintstr.h"

#ifdef _XSERVER64

#include "Agent.h"

#define GC XlibGC

#endif /* _XSERVER64 */

#include "Xlib.h"
#include "Xutil.h"

#include "Display.h"
#include "Splash.h"
#include "Screen.h"
#include "Windows.h"
#include "Atoms.h"
#include "Trap.h"

/*
 * Set here the required log level.
 */

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

/*
 * Colors used to paint the splash screen.
 */

int nxagentLogoDepth;
int nxagentLogoWhite;
int nxagentLogoRed;
int nxagentLogoBlack;

void nxagentPaintLogo(Window win, GC gc, int scale, int width, int height);

/*
 * From Screen.c.
 */

extern Atom nxagentWMStart;

/*
 * From Clipboard.c.
 */

extern Atom serverCutProperty;

int nxagentShowSplashWindow(Window parentWindow)
{
  XWindowAttributes getAttributes;
  XWindowChanges    values;
  XSetWindowAttributes attributes;
  GC gc;

  #ifdef TEST
  fprintf(stderr, "nxagentShowSplash: Got called.\n");
  #endif

  #ifdef NXAGENT_TIMESTAMP
  {
    extern unsigned long startTime;

    fprintf(stderr, "nxagentShowSplash: Initializing splash start at [%d] milliseconds.\n",
            GetTimeInMillis() - startTime);
  }
  #endif

  XSetSelectionOwner(nxagentDisplay, nxagentWMStart, None, CurrentTime);

  nxagentWMPassed = False;

  /*
   * This would cause a GetWindowAttributes and a
   * GetGeometry (asynchronous) reply. We use instead
   * the geometry requested by the user for the agent
   * window.
   *
   * XGetWindowAttributes(nxagentDisplay, parentWindow, &getAttributes);
   */

  /*
   * During reconnection we draw the splash over
   * the default window and not over the root
   * window because it would be hidden  by other
   * windows.
   */

  if (nxagentReconnectTrap)
  {
    getAttributes.x = nxagentOption(RootX);
    getAttributes.y = nxagentOption(RootY);
  }
  else
  {
    getAttributes.x = 0;
    getAttributes.y = 0;
  }

  getAttributes.width  = nxagentOption(RootWidth);
  getAttributes.height = nxagentOption(RootHeight);

  #ifdef TEST
  fprintf(stderr, "nxagentShowSplash: Going to create new splash window.\n");
  #endif

  nxagentSplashWindow =
      XCreateSimpleWindow(nxagentDisplay,
                          parentWindow,
                          getAttributes.x, getAttributes.y,
                          getAttributes.width, getAttributes.height,
                          0,
                          WhitePixel (nxagentDisplay, 0),
                          BlackPixel (nxagentDisplay, 0));

  #ifdef TEST
  fprintf(stderr, "nxagentShowSplash: Created new splash window with id [%ld].\n",
              nxagentSplashWindow);
  #endif

  gc = XCreateGC(nxagentDisplay, nxagentSplashWindow, 0, NULL);
  nxagentPaintLogo(nxagentSplashWindow, gc, 1, getAttributes.width, getAttributes.height);
  XMapRaised (nxagentDisplay, nxagentSplashWindow);
  values.stack_mode = Above;
  XConfigureWindow(nxagentDisplay, nxagentSplashWindow, CWStackMode, &values);
  attributes.override_redirect = True;
  XChangeWindowAttributes(nxagentDisplay, nxagentSplashWindow, CWOverrideRedirect, &attributes);
  XFreeGC(nxagentDisplay, gc);

  #ifdef NXAGENT_TIMESTAMP
  {
    extern unsigned long startTime;
    fprintf(stderr, "nxagentShowSplash: Splash ends [%d] milliseconds.\n",
            GetTimeInMillis() - startTime);
  }
  #endif

  return True;
}

void nxagentPaintLogo(Window win, GC gc, int scale, int width, int height)
{
  XPoint    rect[4];
  XPoint    m[12];
  int w, h, c, w2, h2;

  #ifdef DEBUG
  fprintf(stderr, "nxagenShowtLogo: Got called.\n");
  #endif

  #ifdef NXAGENT_LOGO_DEBUG
  fprintf(stderr, "nxagentPaintLogo: begin\n");
  fprintf(stderr, "nxagentPaintLogo: gen params are: w=%d h=%d d=%d r=%x w=%x b=%x\n",width, height,
          nxagentLogoDepth, nxagentLogoRed,
          nxagentLogoWhite, nxagentLogoBlack);
  #endif

  w = width/scale;
  h = height/scale;

  w2 = w/2;
  h2 = h/2;

  if (height > width)
  {
    c = w/30;
  }
  else
  {
    c = w/48;
  }

  rect[0].x = 0;               rect[0].y = 0;
  rect[1].x = 0;               rect[1].y = h;
  rect[2].x = w;               rect[2].y = h;
  rect[3].x = w;               rect[3].y = 0;

  XSetFunction(nxagentDisplay, gc, GXcopy);
  XSetFillStyle(nxagentDisplay, gc, FillSolid);
  XSetForeground(nxagentDisplay, gc, nxagentLogoBlack);
  XSetBackground(nxagentDisplay, gc, nxagentLogoRed);

  nxagentPixmapLogo = XCreatePixmap(nxagentDisplay, win, width, height, nxagentLogoDepth);

  if (!nxagentPixmapLogo)
  {
    return;
  }

  XFillPolygon(nxagentDisplay, nxagentPixmapLogo, gc, rect, 4, Convex, CoordModeOrigin);

  #ifdef NXAGENT_LOGO_DEBUG
  fprintf(stderr, "filled first poly\n");
  #endif

  XSetForeground(nxagentDisplay, gc, nxagentLogoRed);
  XSetBackground(nxagentDisplay, gc, nxagentLogoWhite);

  rect[0].x = w2-10*c;               rect[0].y = h2-8*c;
  rect[1].x = w2-10*c;               rect[1].y = h2+8*c;
  rect[2].x = w2+10*c;               rect[2].y = h2+8*c;
  rect[3].x = w2+10*c;               rect[3].y = h2-8*c;

  XFillPolygon(nxagentDisplay, nxagentPixmapLogo, gc, rect, 4, Convex, CoordModeOrigin);

  #ifdef NXAGENT_LOGO_DEBUG
  fprintf(stderr, "filled red rect\n");
  #endif

  rect[0].x = w2-9*c;               rect[0].y = h2-7*c;
  rect[1].x = w2-9*c;               rect[1].y = h2+7*c;
  rect[2].x = w2+9*c;               rect[2].y = h2+7*c;
  rect[3].x = w2+9*c;               rect[3].y = h2-7*c;

  XSetForeground(nxagentDisplay, gc, nxagentLogoWhite);
  XSetBackground(nxagentDisplay, gc, nxagentLogoRed);

  XFillPolygon(nxagentDisplay, nxagentPixmapLogo, gc, rect, 4, Convex, CoordModeOrigin);

  /*
   * Begin 'M'.
   */

  m[0].x = w2-3*c;  m[0].y = h2-5*c;
  m[1].x = w2+7*c;  m[1].y = h2-5*c;
  m[2].x = w2+7*c;  m[2].y = h2+5*c;
  m[3].x = w2+5*c;  m[3].y = h2+5*c;
  m[4].x = w2+5*c;  m[4].y = h2-3*c;
  m[5].x = w2+3*c;  m[5].y = h2-3*c;
  m[6].x = w2+3*c;  m[6].y = h2+5*c;
  m[7].x = w2+1*c;  m[7].y = h2+5*c;
  m[8].x = w2+1*c;  m[8].y = h2-3*c;
  m[9].x = w2-1*c;  m[9].y = h2-3*c;
  m[10].x = w2-1*c; m[10].y = h2+5*c;
  m[11].x = w2-3*c; m[11].y = h2+5*c;

  XSetForeground(nxagentDisplay, gc, nxagentLogoRed);
  XSetBackground(nxagentDisplay, gc, nxagentLogoWhite);

  XFillPolygon(nxagentDisplay, nxagentPixmapLogo, gc, m, 12, Nonconvex, CoordModeOrigin);

  /*
   * End 'M'.
   */

  /*
   * Begin '!'.
   */

  rect[0].x = w2-7*c;               rect[0].y = h2-5*c;
  rect[1].x = w2-5*c;               rect[1].y = h2-5*c;
  rect[2].x = w2-5*c;               rect[2].y = h2+2*c;
  rect[3].x = w2-7*c;               rect[3].y = h2+2*c;

  XFillPolygon(nxagentDisplay, nxagentPixmapLogo, gc, rect, 4, Convex, CoordModeOrigin);

  rect[0].x = w2-7*c;               rect[0].y = h2+3*c;
  rect[1].x = w2-5*c;               rect[1].y = h2+3*c;
  rect[2].x = w2-5*c;               rect[2].y = h2+5*c;
  rect[3].x = w2-7*c;               rect[3].y = h2+5*c;

  XFillPolygon(nxagentDisplay, nxagentPixmapLogo, gc, rect, 4, Convex, CoordModeOrigin);

  /*
   * End 'M'.
   */

  XSetWindowBackgroundPixmap(nxagentDisplay, win, nxagentPixmapLogo);

  #ifdef NXAGENT_LOGO_DEBUG
  fprintf(stderr, "nxagentPaintLogo: end\n");
  #endif
}

void nxagentRemoveSplashWindow(WindowPtr pWin)
{
  if (nxagentReconnectTrap) return;

  #ifdef TEST
  fprintf(stderr, "nxagentRemoveSplashWindow: Destroying the splash window.\n");
  #endif

  if (!nxagentWMPassed)
  {
    XSetSelectionOwner(nxagentDisplay, nxagentWMStart,
                          nxagentDefaultWindows[0], CurrentTime);

    nxagentWMPassed = True;
  }

  if (nxagentSplashWindow != None)
  {
    XDestroyWindow(nxagentDisplay, nxagentSplashWindow);

    nxagentSplashWindow = None;
    nxagentRefreshWindows(WindowTable[0]);

    #ifdef TEST
    fprintf(stderr, "nxagentRemoveSplashWindow: setting the ownership of %s (%d) on window 0x%lx\n",
                "NX_CUT_BUFFER_SERVER", (int)serverCutProperty, nxagentWindow(WindowTable[0]));
    #endif

    XSetSelectionOwner(nxagentDisplay, serverCutProperty,
                           nxagentWindow(WindowTable[0]), CurrentTime);
  }

  if (nxagentPixmapLogo)
  {
    XFreePixmap(nxagentDisplay, nxagentPixmapLogo);

    nxagentPixmapLogo = (Pixmap) 0;
  }
}
