/*
 * Copyright © 2003 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * This file is a reduced version of the header file of
 * <X11/extensions/Xdamaga.h>
 *
 * This copy of code has been introduced to allow a clear namespace
 * separation between <X11/...> and <nx-X11/...> header files.
 *
 * This version of the Xdamage library header file only contains symbols
 * required by nxcompshad and strictly avoids indirectly including
 * from an X11 library that is also shipped in nx-X11/lib/.
 *
 * When using <X11/extensions/Xdamage.h> instead for inclusion in
 * nxcompshad, it will attempt pulling in the <X11/extensions/Xfixes.h>
 * header which in turn will include <X11/Xlib.h>. However, the headers of
 * the same name from <nx-X11/...> should be used instead.
 *
 * FIXME: Once the nxagent Xserver starts using libXfixes from X.Org, this
 * hack can be removed.
 *
 * 2015/06/26, Mike Gabriel <mike.gabriel@das-netzwerkteam.de>
 */


#ifndef _XDAMAGE_H_
#define _XDAMAGE_H_

#include <X11/extensions/damagewire.h>
#include <nx-X11/Xfuncproto.h>

/* from <X11/extensions/Xfixes.h> */
typedef XID XserverRegion;

#define XDAMAGE_1_1_INTERFACE

typedef XID Damage;

typedef struct {
    int type;			/* event base */
    unsigned long serial;
    Bool send_event;
    Display *display;
    Drawable drawable;
    Damage damage;
    int level;
    Bool more;			/* more events will be delivered immediately */
    Time timestamp;
    XRectangle area;
    XRectangle geometry;
} XDamageNotifyEvent;

_XFUNCPROTOBEGIN

Bool XDamageQueryExtension (Display *dpy,
                            int *event_base_return,
                            int *error_base_return);

Status XDamageQueryVersion (Display *dpy,
			    int     *major_version_return,
			    int     *minor_version_return);

Damage
XDamageCreate (Display	*dpy, Drawable drawable, int level);

void
XDamageSubtract (Display *dpy, Damage damage,
		 XserverRegion repair, XserverRegion parts);

_XFUNCPROTOEND

#endif /* _XDAMAGE_H_ */
