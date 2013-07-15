/*
 * Copyright © 2013 Keith Packard
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
 */

#ifndef _PRESENT_PROTO_H_
#define _PRESENT_PROTO_H_

#include <X11/extensions/presenttokens.h>

#define Region CARD32
#define XSyncFence CARD32

typedef struct {
    CARD8   reqType;
    CARD8   presentReqType;
    CARD16  length B16;
    CARD32  majorVersion B32;
    CARD32  minorVersion B32;
} xPresentQueryVersionReq;
#define sz_xPresentQueryVersionReq   12

typedef struct {
    BYTE    type;   /* X_Reply */
    BYTE    pad1;
    CARD16  sequenceNumber B16;
    CARD32  length B32;
    CARD32  majorVersion B32;
    CARD32  minorVersion B32;
    CARD32  pad2 B32;
    CARD32  pad3 B32;
    CARD32  pad4 B32;
    CARD32  pad5 B32;
} xPresentQueryVersionReply;
#define sz_xPresentQueryVersionReply	32

typedef struct {
    CARD8   reqType;
    CARD8   presentReqType;
    CARD16  length B16;
    Window  window B32;

    Pixmap  pixmap B32;
    CARD32  serial B32;

    Region  valid B32;
    Region  update B32;

    INT16   x_off B16;
    INT16   y_off B16;
    XSyncFence idle_fence B32;

    CARD64  target_msc;
    CARD64  divisor;
    CARD64  remainder;
} xPresentRegionReq;
#define sz_xPresentRegionReq	56

typedef struct {
    CARD8   reqType;
    CARD8   presentReqType;
    CARD16  length B16;
    Window  window B32;

    CARD32  serial B32;
    CARD32  pad0 B32;

    CARD64  target_msc;
    CARD64  divisor;
    CARD64  remainder;
} xPresentNotifyMSCReq;
#define sz_xPresentNotifyMSCReq	40

typedef struct {
    CARD8   reqType;
    CARD8   presentReqType;
    CARD16  length B16;
    CARD32  eid B32;
    CARD32  window B32;
    CARD32  eventMask B32;
} xPresentSelectInputReq;
#define sz_xPresentSelectInputReq   12

/*
 * Events
 *
 * All Present events are X Generic Events
 */

typedef struct {
    CARD8 type;
    CARD8 extension;
    CARD16 sequenceNumber B16;
    CARD32 length;
    CARD16 evtype B16;
    CARD16 pad2;
    CARD32 eid B32;
    CARD32 window B32;
    INT16  x B16;
    INT16  y B16;
    CARD16 width B16;
    CARD16 height B16;
    INT16  off_x B16;
    INT16  off_y B16;

    CARD16 pixmap_width B16;
    CARD16 pixmap_height B16;
    CARD32 pixmap_flags B32;
} xPresentConfigureNotify;
#define sz_xPresentConfigureNotify 40

typedef struct {
    CARD8 type;
    CARD8 extension;
    CARD16 sequenceNumber B16;
    CARD32 length;
    CARD16 evtype B16;
    CARD16 kind B16;
    CARD32 eid B32;
    Window window B32;
    CARD32 serial B32;
    CARD64 ust;

    CARD64 msc;
} xPresentCompleteNotify;
#define sz_xPresentCompleteNotify 40

typedef struct {
    CARD8 type;
    CARD8 extension;
    CARD16 sequenceNumber B16;
    CARD32 length;
    CARD16 evtype B16;
    CARD16 pad2;
    CARD32 eid B32;

    Window event_window B32;
    Window window B32;
    Pixmap pixmap B32;
    Region valid_region B32;

    xRectangle valid_rect;
    xRectangle update_rect;

    Region update_region B32;
    INT16 x_off B16;
    INT16 y_off B16;
    CARD64 target_msc;

    CARD64 divisor;
    CARD64 remainder;

    XSyncFence idle_fence;
} xPresentRedirectNotify;

#define sz_xPresentRedirectNotify 84

#undef Region
#undef XSyncFence

#endif
