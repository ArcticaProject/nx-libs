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

#ifndef _PRESENT_TOKENS_H_
#define _PRESENT_TOKENS_H_

#define PRESENT_NAME			"Present"
#define PRESENT_MAJOR			1
#define PRESENT_MINOR			0

#define PresentNumberErrors		0
#define PresentNumberEvents		0

#define X_PresentQueryVersion		0
#define X_PresentRegion			1
#define X_PresentSelectInput		2

#define PresentNumberRequests		3

#define PresentConfigureNotify	0
#define PresentCompleteNotify	1
#define PresentRedirectNotify	2

#define PresentAllEvents   ((1 << PresentConfigureNotify) |     \
                            (1 << PresentCompleteNotify) |      \
                            (1 << PresentRedirectNotify))

#endif
