/*
 * Copyright 1998 by Sebastien Marineau <sebastien@qnx.com>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of David Wexelblat not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission. Sebastien Marineau makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * SEBASTIEN MARINEAU DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL DAVID WEXELBLAT BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * $XFree86: xc/programs/Xserver/hw/xfree86/os-support/nto/nto_ioperm.c,v 1.2 1999/12/27 00:45:45 robin Exp $
 */

/* I/O functions to enable access to I/O ports under Neutrino */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include <sys/neutrino.h>
#include <errno.h>


Bool xf86EnableIO()
{
	ErrorF("xf86EnableIO: enabling I/O access\n");
	if(ThreadCtl(_NTO_TCTL_IO, 0)) {
		ErrorF("xf86EnableIO: could not set I/O privilege, errno %d\n",errno);
		return FALSE;
	}
	return TRUE;
}

void xf86DisableIO()
{
	return;
}
