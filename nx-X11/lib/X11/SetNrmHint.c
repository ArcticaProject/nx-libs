
/***********************************************************
Copyright 1988 by Wyse Technology, Inc., San Jose, Ca,
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL AND WYSE DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
EVENT SHALL DIGITAL OR WYSE BE LIABLE FOR ANY SPECIAL, INDIRECT OR
CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

******************************************************************/
/*

Copyright 1987, 1988, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from The Open Group.

*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <nx-X11/Xlibint.h>
#include <nx-X11/Xutil.h>
#include "Xatomtype.h"
#include <X11/Xatom.h>
#include <nx-X11/Xos.h>

void XSetWMSizeHints (dpy, w, hints, prop)
    Display *dpy;
    Window w;
    XSizeHints *hints;
    Atom prop;
{
    xPropSizeHints data;

    data.flags = (hints->flags & 
		  (USPosition|USSize|PPosition|PSize|PMinSize|PMaxSize|
		   PResizeInc|PAspect|PBaseSize|PWinGravity));

    /*
     * The x, y, width, and height fields are obsolete; but, applications
     * that want to work with old window managers might set them.
     */
    data.x = hints->x;
    data.y = hints->y;
    data.width = hints->width;
    data.height = hints->height;

    data.minWidth = hints->min_width;
    data.minHeight = hints->min_height;
    data.maxWidth  = hints->max_width;
    data.maxHeight = hints->max_height;
    data.widthInc = hints->width_inc;
    data.heightInc = hints->height_inc;
    data.minAspectX = hints->min_aspect.x;
    data.minAspectY = hints->min_aspect.y;
    data.maxAspectX = hints->max_aspect.x;
    data.maxAspectY = hints->max_aspect.y;
    data.baseWidth = hints->base_width;
    data.baseHeight = hints->base_height;
    data.winGravity = hints->win_gravity;
   
    XChangeProperty (dpy, w, prop, XA_WM_SIZE_HINTS, 32,
		     PropModeReplace, (unsigned char *) &data,
		     NumPropSizeElements);
}


void XSetWMNormalHints (dpy, w, hints)
    Display *dpy;
    Window w;
    XSizeHints *hints;
{
    XSetWMSizeHints (dpy, w, hints, XA_WM_NORMAL_HINTS);
}

