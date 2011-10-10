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

#include <signal.h>
#include <stdio.h>

#ifdef _XSERVER64

#include "scrnintstr.h"
#include "Agent.h"
#define GC XlibGC
#define PIXEL_ALREADY_TYPEDEFED

#endif /* _XSERVER64 */

#include "pixmapstr.h"
#include "regionstr.h"
#include "resource.h"
#include "../../include/gc.h"
#include "../../include/window.h"

#include "xpm.h"

#include "Agent.h"
#include "Pixmaps.h"
#include "Display.h"
#include "Holder.h"
#include "Icons.h"

#include NXAGENT_PLACEHOLDER_NAME

#define MAXDEPTH 32

#define PLACEHOLDER_WIDTH 14
#define PLACEHOLDER_HEIGHT 16

#define PLACEHOLDER_BORDER_COLOR_DARK 0x000000
#define PLACEHOLDER_BORDER_COLOR_LIGHT 0xB2B2B2

/*
 * Set here the required log level.
 */

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG
#undef  DUMP

static Pixmap nxagentPlaceholderPixmaps[MAXDEPTH + 1];

void nxagentMarkPlaceholderNotLoaded(int depth)
{
  nxagentPlaceholderPixmaps[depth] = 0;
}

void nxagentInitPlaceholder(int depth)
{
  int status;
  XpmAttributes attributes;

  attributes.valuemask = XpmDepth | XpmSize;
  attributes.depth = depth;

  status = XpmCreatePixmapFromData(nxagentDisplay, DefaultRootWindow(nxagentDisplay),
                                       placeholderXpm, nxagentPlaceholderPixmaps + depth, NULL, &attributes);

  if (status != Success)
  {
    FatalError("Error: Failed to create the placeholder pixmap.\n");
  }

  #ifdef TEST
  fprintf(stderr, "nxagentInitPlaceholder: Created pixmap [0x%lx] with geometry [%d,%d] for depth [%d].\n",
              nxagentPlaceholderPixmaps[depth], attributes.width, attributes.height, depth);
  #endif
}

void nxagentApplyPlaceholder(Drawable drawable, int x, int y,
                                 int w, int h, int depth)
{
  /*
   * Instead of the image, a white rectangle that
   * covers the pixmap area is drawn, alongside
   * with a black and grey line that outlines the
   * boundaries of the affected area.
   */

  GC gc;
  XGCValues value;
  XPoint points[3];

  value.foreground = 0xffffffff;
  value.background = 0x00000000;
  value.plane_mask = 0xffffffff;
  value.fill_style = FillSolid;

  /*
   * FIXME: Should we use a gc cache to save
   *        some bandwidth?
   */

  gc = XCreateGC(nxagentDisplay, drawable, GCBackground |
           GCForeground | GCFillStyle | GCPlaneMask, &value);

  XFillRectangle(nxagentDisplay, drawable, gc, x, y, w, h);

  if (depth == 1)
  {
    return;
  }

  value.foreground = PLACEHOLDER_BORDER_COLOR_DARK;
  value.line_style = LineSolid;
  value.line_width = 1;

  points[0].x = x;
  points[0].y = y + h - 1;
  points[1].x = x;
  points[1].y = y;
  points[2].x = x + w - 1;
  points[2].y = y;

  XChangeGC(nxagentDisplay, gc, GCForeground | GCLineWidth | GCLineStyle, &value);
  XDrawLines(nxagentDisplay, drawable, gc, points, 3, CoordModeOrigin);

  value.foreground = PLACEHOLDER_BORDER_COLOR_LIGHT;
  value.line_style = LineSolid;
  value.line_width = 1;

  points[0].x = x;
  points[0].y = y + h - 1;
  points[1].x = x + w - 1;
  points[1].y = y + h - 1;
  points[2].x = x + w - 1;
  points[2].y = y;

  XChangeGC(nxagentDisplay, gc, GCForeground | GCLineWidth | GCLineStyle, &value);
  XDrawLines(nxagentDisplay, drawable, gc, points, 3, CoordModeOrigin);

  /*
   * We are going to apply place holder only if on region
   * we have enough space for the placeholder plus three
   * pixel for spacing and one for region border.
   */

  if ((w >= PLACEHOLDER_WIDTH + 8) && (h >= PLACEHOLDER_HEIGHT + 8))
  {
    #ifdef TEST
    fprintf(stderr, "nxagentApplyPlaceholder: drawable %lx placeholder %lx from %d %d  pixmap size is %d %d "
                        "depth %d\n", drawable, nxagentPlaceholderPixmaps[depth], x, y, w, h, depth);
    #endif

    if (nxagentPlaceholderPixmaps[depth] == 0)
    {
      nxagentInitPlaceholder(depth);
    }

    XCopyArea(nxagentDisplay, nxagentPlaceholderPixmaps[depth],
                  drawable, gc, 0, 0, PLACEHOLDER_WIDTH, PLACEHOLDER_HEIGHT, x + 4, y + 4);

  }

  XFreeGC(nxagentDisplay, gc);

  return;
}

#ifdef DUMP

static char hexdigit(char c)
{
  char map[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', '?'};

  return map[c];
}

/*
FIXME: Please, check the implementation of the same
       function in nxcomp.
*/
char *nxagentChecksum(char *string, int length)
{
  static char md5_output[MD5_DIGEST_LENGTH * 2 + 1];
  static char md5[MD5_DIGEST_LENGTH];
  char * ret;
  int i;

  memset(md5, 0, sizeof(md5));
  memset(md5_output, 0, sizeof(md5_output));

  ret = MD5(string, length, md5);

  for (i = 0; i < MD5_DIGEST_LENGTH; i++)
  {
    char c = md5[i];

    md5_output[i * 2 + 0] = hexdigit((c >> 0) & 0xF);
    md5_output[i * 2 + 1] = hexdigit((c >> 4) & 0xF);
  }

  return md5_output;
}

#else

const char *nxagentChecksum(char *data, int size)
{
  return "";
}

#endif
