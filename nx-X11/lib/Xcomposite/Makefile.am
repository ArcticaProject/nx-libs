# 
#  $Id: Makefile.am,v 1.1 2004/07/31 05:50:39 anholt Exp $
# 
#  Copyright © 2003 Keith Packard, Noah Levitt
# 
#  Permission to use, copy, modify, distribute, and sell this software and its
#  documentation for any purpose is hereby granted without fee, provided that
#  the above copyright notice appear in all copies and that both that
#  copyright notice and this permission notice appear in supporting
#  documentation, and that the name of Keith Packard not be used in
#  advertising or publicity pertaining to distribution of the software without
#  specific, written prior permission.  Keith Packard makes no
#  representations about the suitability of this software for any purpose.  It
#  is provided "as is" without express or implied warranty.
# 
#  KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
#  INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
#  EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
#  CONSEQUENTIAL COMPOSITES OR ANY COMPOSITES WHATSOEVER RESULTING FROM LOSS OF USE,
#  DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
#  TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
#  PERFORMANCE OF THIS SOFTWARE.

AM_CFLAGS = $(XCOMPOSITE_CFLAGS) $(X_CFLAGS)

lib_LTLIBRARIES = libXcomposite.la

libXcomposite_la_SOURCES = \
	xcompositeint.h	\
	Xcomposite.c

libXcomposite_la_LIBADD = $(XCOMPOSITE_LIBS) $(X_LIBS)

#
# Library version info.  Check the libtool docs for
# instructions on when and how to change this value
#
# bump when the ABI changes
XCOMPOSITE_CURRENT=2
# bump for non-ABI changes, reset to zero when CURRENT changes
XCOMPOSITE_REVISION=0
# bump when the ABI changes in backward-compatible fashion
XCOMPOSITE_AGE=1
libXcomposite_la_LDFLAGS = -version-info ${XCOMPOSITE_CURRENT}:${XCOMPOSITE_REVISION}:${XCOMPOSITE_AGE} -no-undefined

libXcompositeincludedir = $(includedir)/X11/extensions
libXcompositeinclude_HEADERS = Xcomposite.h 

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = xcomposite.pc

EXTRA_DIST = xcomposite.pc.in autogen.sh
