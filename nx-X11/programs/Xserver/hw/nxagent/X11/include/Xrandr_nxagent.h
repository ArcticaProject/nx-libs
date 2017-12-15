/*
 * Copyright © 2000 Compaq Computer Corporation, Inc.
 * Copyright © 2002 Hewlett-Packard Company, Inc.
 * Copyright © 2006 Intel Corporation
 * Copyright © 2008 Red Hat, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 *
 * Author:  Jim Gettys, HP Labs, Hewlett-Packard, Inc.
 *	    Keith Packard, Intel Corporation
 */

/*
 * This file is a reduced version of the header file of
 * <X11/extensions/Xrandr.h>
 *
 * This copy of code has been introduced to allow a clear namespace
 * separation between <X11/...> and <nx-X11/...> header files.
 *
 * This version of the Xrandr library header file only contains symbols
 * required by nxagent and strictly avoids indirectly including
 * from an X11 library that is also shipped in nx-X11/lib/.
 *
 * When using <X11/extensions/Xrandr.h> instead for inclusion in
 * nxagent, it will attempt pulling in the <X11/extensions/Xrender.h>
 * header which in turn will include <X11/Xlib.h>. However, the headers of
 * the same name from <nx-X11/...> should be used instead.
 *
 * FIXME: Once the nxagent Xserver starts using libXrender from X.Org, this
 * hack can be removed.
 *
 * 2015/06/26, Mike Gabriel <mike.gabriel@das-netzwerkteam.de>
 */

#ifndef _XRANDR_H_
#define _XRANDR_H_

#include <nx-X11/extensions/randr.h>
#include <nx-X11/Xfuncproto.h>

_XFUNCPROTOBEGIN

/*
 *  Events.
 */

typedef struct {
    int type;                   /* event base */
    unsigned long serial;       /* # of last request processed by server */
    Bool send_event;            /* true if this came from a SendEvent request */
    Display *display;           /* Display the event was read from */
    Window window;              /* window which selected for this event */
    Window root;                /* Root window for changed screen */
    Time timestamp;             /* when the screen change occurred */
    Time config_timestamp;      /* when the last configuration change */
    SizeID size_index;
    SubpixelOrder subpixel_order;
    Rotation rotation;
    int width;
    int height;
    int mwidth;
    int mheight;
} XRRScreenChangeNotifyEvent;

_XFUNCPROTOEND

#endif /* _XRANDR_H_ */
