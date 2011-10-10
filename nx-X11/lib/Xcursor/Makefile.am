# 
#  $Id: Makefile.am,v 1.3 2005/06/29 18:46:53 daniels Exp $
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
#  CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
#  DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
#  TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
#  PERFORMANCE OF THIS SOFTWARE.

AM_CFLAGS = $(XRENDER_CFLAGS) $(XFIXES_CFLAGS) $(X_CFLAGS)

lib_LTLIBRARIES = libXcursor.la

man_MANS = Xcursor.3

libXcursor_la_SOURCES = xcursorint.h cursor.c display.c file.c library.c xlib.c

libXcursor_la_LIBADD = $(XRENDER_LIBS) $(XFIXES_LIBS) $(X_LIBS)

#
# Shared library version info.  This is not the same as the package version
#
libXcursor_la_LDFLAGS = -version-info @LT_VERSION_INFO@ -no-undefined

libXcursorincludedir = $(includedir)/X11/Xcursor
libXcursorinclude_HEADERS = Xcursor.h 

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = xcursor.pc

EXTRA_DIST = $(man_MANS) xcursor.pc.in autogen.sh
