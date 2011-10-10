dnl 
dnl  $Id: configure.ac,v 1.3 2005/06/29 18:46:53 daniels Exp $
dnl 
dnl  Copyright © 2003 Keith Packard
dnl 
dnl  Permission to use, copy, modify, distribute, and sell this software and its
dnl  documentation for any purpose is hereby granted without fee, provided that
dnl  the above copyright notice appear in all copies and that both that
dnl  copyright notice and this permission notice appear in supporting
dnl  documentation, and that the name of Keith Packard not be used in
dnl  advertising or publicity pertaining to distribution of the software without
dnl  specific, written prior permission.  Keith Packard makes no
dnl  representations about the suitability of this software for any purpose.  It
dnl  is provided "as is" without express or implied warranty.
dnl 
dnl  KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
dnl  INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
dnl  EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
dnl  CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
dnl  DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
dnl  TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
dnl  PERFORMANCE OF THIS SOFTWARE.
dnl
dnl Process this file with autoconf to create configure.

AC_PREREQ([2.57])
dnl
dnl This is the package version number, not the shared library
dnl version.  This same version number must appear in Xcursor.h
dnl Yes, it is a pain to synchronize version numbers.  Unfortunately, it's
dnl not possible to extract the version number here from Xcursor.h
dnl
AC_INIT([libXcursor],1.1.4,[keithp@keithp.com],[libXcursor])
AM_INIT_AUTOMAKE([dist-bzip2])
AC_CONFIG_SRCDIR([Makefile.am])
AM_MAINTAINER_MODE
AM_CONFIG_HEADER(config.h)

dnl libtool versioning

LT_CURRENT=1
LT_REVISION=2
LT_AGE=0

AC_SUBST(LT_CURRENT)
AC_SUBST(LT_REVISION)
LT_AGE=0

LT_VERSION_INFO="$LT_CURRENT:$LT_REVISION:$LT_AGE"
AC_SUBST(LT_VERSION_INFO)

LT_CURRENT_MINUS_AGE=`expr $LT_CURRENT - $LT_AGE`
AC_SUBST(LT_CURRENT_MINUS_AGE)

# Check for progs
AC_PROG_CC
AC_PROG_LIBTOOL

# Check for X
PKG_CHECK_MODULES(X, x11, 
		  [x_found_with_pkgconfig=yes],
		  [x_found_with_pkgconfig=no])
		  
if test "$x_found_with_pkgconfig" = "no"
then
	AC_PATH_XTRA
	X_LIBS="$X_LIBS -lX11"

	if test "x$no_x" = "xyes"
	then
	        AC_MSG_ERROR([X is required, but it was either disabled or not found.])
	fi
	# Check for XTHREADS
	
	save_LIBS="$LIBS"
	save_CFLAGS="$CFLAGS"
	CFLAGS="$CFLAGS $X_CFLAGS"
	LIBS="$LIBS $X_LIBS"
	
	AC_MSG_CHECKING([for XTHREADS in Xlib])
	AC_RUN_IFELSE(
		      [AC_LANG_PROGRAM([[#include <X11/Xlib.h>]],
			[[return XInitThreads() == 0 ? 0 : 1;]])],
			[xthreads=no],
		[xthreads=yes],
		[xthreads=yes])
	
	AC_MSG_RESULT($xthreads)
	
	LIBS="$save_LIBS"
	CFLAGS="$save_CFLAGS"
	
	if test "x$xthreads" = "xyes"
	then
		X_CFLAGS="$X_CFLAGS -DXTHREADS"
	fi

	if test "x$no_x" = "xyes"
	then
	        AC_MSG_ERROR([X is required, but it was either disabled or not found.])
	fi
fi

AC_SUBST(X_CFLAGS)
AC_SUBST(X_LIBS)

PKG_CHECK_MODULES(XRENDER, xrender >= 0.8.2, [xrender_found_with_pkgconfig=yes], 
                  [xrender_found_with_pkgconfig=no])
case "$xrender_found_with_pkgconfig" in
no)
	PKG_CHECK_MODULES(XRENDER, xrender >= 0.8, [old_xrender_found_with_pkgconfig=yes], 
			  [old_xrender_found_with_pkgconfig=no])
	case "$old_xrender_found_with_pkgconfig" in
	no)
	        # checks for X
	        AC_PATH_X

	        XRENDER_CFLAGS="-I$x_includes"
	        XRENDER_LIBS="-L$x_libraries -lXrender -lXext -lX11"

	        saved_CPPFLAGS="$CPPFLAGS"
	        CPPFLAGS="$CPPFLAGS $XRENDER_CFLAGS"
	        AC_CHECK_HEADERS([X11/extensions/Xrender.h], [], [AC_MSG_ERROR([Xrender.h not found.])])
	        CPPFLAGS="$saved_CPPFLAGS"

	        saved_LIBS="$LIBS"
	        LIBS="$LIBS $XRENDER_LIBS"
	        AC_CHECK_FUNCS([XRenderCreateAnimCursor], [], [AC_MSG_ERROR([libXrender not found.])])
	        LIBS="$saved_LIBS"
		;;
	yes)
		XRENDER_LIBS="$XRENDER_LIBS -lXext -lX11"
		;;
	esac
        ;;
esac
PKG_CHECK_MODULES(XFIXES, xfixes, [AC_DEFINE_UNQUOTED(HAVE_XFIXES, 1, [Define to 1 if you have Xfixes])])

AC_SUBST(XRENDER_LIBS)
AC_SUBST(XRENDER_CFLAGS)

AC_OUTPUT([Makefile
           xcursor.pc])
