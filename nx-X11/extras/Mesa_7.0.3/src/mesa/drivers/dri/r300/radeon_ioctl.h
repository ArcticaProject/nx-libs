/*
Copyright (C) The Weather Channel, Inc.  2002.  All Rights Reserved.

The Weather Channel (TM) funded Tungsten Graphics to develop the
initial release of the Radeon 8500 driver under the XFree86 license.
This notice must be preserved.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Keith Whitwell <keith@tungstengraphics.com>
 */

#ifndef __RADEON_IOCTL_H__
#define __RADEON_IOCTL_H__

#include "simple_list.h"
#include "radeon_dri.h"
#include "radeon_lock.h"

#include "xf86drm.h"
#include "drm.h"
#if 0
#include "r200_context.h"
#endif
#include "radeon_drm.h"

extern void radeonCopyBuffer(const __DRIdrawablePrivate * drawable,
			     const drm_clip_rect_t	* rect);
extern void radeonPageFlip(const __DRIdrawablePrivate * drawable);
extern void radeonFlush(GLcontext * ctx);
extern void radeonFinish(GLcontext * ctx);
extern void radeonWaitForIdleLocked(radeonContextPtr radeon);
extern uint32_t radeonGetAge(radeonContextPtr radeon);

#endif				/* __RADEON_IOCTL_H__ */
