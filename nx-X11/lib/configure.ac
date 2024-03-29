# Initialize Autoconf
AC_PREREQ([2.60])
AC_INIT([libNX_X11], [1.7.3.1],
        [https://github.com/ArcticaProject/nx-libs/issues], [libNX_X11])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_SRCDIR([Makefile.am])
AC_CONFIG_HEADERS([src/config.h include/X11/XlibConf.h])
AC_CONFIG_MACRO_DIR([m4])
AC_CANONICAL_BUILD
AC_CANONICAL_HOST
# Set common system defines for POSIX extensions, such as _GNU_SOURCE
# Must be called before any macros that run the compiler (like AC_PROG_LIBTOOL)
# to avoid autoconf errors.
AC_USE_SYSTEM_EXTENSIONS

# Initialize Automake
AM_INIT_AUTOMAKE([foreign dist-xz])

# Initialize libtool
AC_PROG_LIBTOOL

# Upstream's pkg.m4 (since 0.27) offers this now, but define our own
# compatible version in case the local version of pkgconfig isn't new enough.
# https://bugs.freedesktop.org/show_bug.cgi?id=48743
m4_ifdef([PKG_INSTALLDIR], [PKG_INSTALLDIR],
         [AC_ARG_WITH([pkgconfigdir],
                      [AS_HELP_STRING([--with-pkgconfigdir],
                      [install directory for nx-x11.pc pkg-config file])],
                       [],[with_pkgconfigdir='$(libdir)/pkgconfig'])
          AC_SUBST([pkgconfigdir], [${with_pkgconfigdir}])])

NX_COMPILER_BRAND
NX_DEFAULT_OPTIONS

# Required when PKG_CHECK_MODULES called within an if statement
PKG_PROG_PKG_CONFIG

if test x"$CC_FOR_BUILD" = x; then
	if test x"$cross_compiling" = xyes; then
	       AC_CHECK_PROGS(CC_FOR_BUILD, gcc cc)
	else
	       CC_FOR_BUILD="$CC"
	fi
fi
AC_SUBST([CC_FOR_BUILD])

if test x"$CPPFLAGS_FOR_BUILD" = x; then
	if test ! x"$cross_compiling" = xyes; then
		CPPFLAGS_FOR_BUILD=${CPPFLAGS}
	fi
fi
AC_SUBST(CPPFLAGS_FOR_BUILD)

if test x"$CFLAGS_FOR_BUILD" = x; then
	if test ! x"$cross_compiling" = xyes; then
		CFLAGS_FOR_BUILD=${CFLAGS}
	fi
fi
AC_SUBST(CFLAGS_FOR_BUILD)

if test x"$LDFLAGS_FOR_BUILD" = x; then
	if test ! x"$cross_compiling" = xyes; then
		LDFLAGS_FOR_BUILD=${LDFLAGS}
	fi
fi
AC_SUBST(LDFLAGS_FOR_BUILD)

if test x"$EXEEXT_FOR_BUILD" = x; then
	if test x"$cross_compiling" = xno; then
		EXEEXT_FOR_BUILD=${EXEEXT}
	fi
	# If cross-compiling and not provided, assume EXEEXT_FOR_BUILD is empty
fi
AC_SUBST([EXEEXT_FOR_BUILD])

# Checks for pkg-config packages

# Always required
X11_REQUIRES='nx-xproto nx-xextproto nx-xtrans'
X11_EXTRA_DEPS=""

X11_REQUIRES="${X11_REQUIRES} xau xcmiscproto bigreqsproto"
X11_EXTRA_DEPS="xau"
PKG_CHECK_MODULES(XDMCP, xdmcp,
	AC_CHECK_LIB(Xdmcp, XdmcpWrap,
		[
		AC_CHECK_LIB(Xdmcp, XdmcpWrap, [xdmauth="yes"], [xdmauth="no"], [$XDMCP_LIBS])
		X11_EXTRA_DEPS="$X11_EXTRA_DEPS xdmcp"
		],
		[
		XDMCP_CFLAGS=
		XDMCP_LIBS=
		], [$XDMCP_LIBS]),
	[AC_MSG_RESULT(no)])
AC_SUBST(X11_EXTRA_DEPS)

# Silence warning: ar: 'u' modifier ignored since 'D' is the default
AC_SUBST(AR_FLAGS, [cr])

# Issue an error if nx-xtrans.m4 was not found and NX_XTRANS_CONNECTION_FLAGS macro
# was not expanded, since libNX_X11 with no transport types is rather useless.
#
# If you're seeing an error here, be sure you installed the nx-xtrans module
# first and if it's not in the default location, that you set the ACLOCAL
# environment variable to find it, such as:
#	ACLOCAL="aclocal -I ${PREFIX}/share/aclocal"
m4_pattern_forbid([^NX_XTRANS_CONNECTION_FLAGS$])

# Transport selection macro from nx-xtrans.m4
NX_XTRANS_CONNECTION_FLAGS

# Secure RPC detection macro from xtrans.m4
NX_XTRANS_SECURE_RPC_FLAGS

# Original comments regarding FAIL_HARD from libxtrans:
# "fail hard if socket directories cannot be chowned to root or
# chmod'ed to the requested mode if 'sticky' bit is requested for this
# directory instead of just print a warning that will remain unnoticed
# most of the times.
# Set the define XtransFailSoft to restore the old
# behavior to fail softly when socket directory cannot chowned/chmod
# to correct user/group."
#
# -> we want to always fail in that scenario.
AC_DEFINE(FAIL_HARD,1,[Fail if the xtrans socket directory cannot be chowned/chmod'ed])

# Actually disable all of the FD passing code, NX does not support it.
AC_DEFINE(XTRANS_SEND_FDS,0,[Disable xtrans FD passing code.])

# Check for dlopen
AC_MSG_CHECKING([if run-time linking is supported])
AC_SEARCH_LIBS(dlopen,[dl svld])
if test "x$ac_cv_search_dlopen" = xno; then
	AC_SEARCH_LIBS(shl_load,[dld])
	if test "x$ac_cv_search_shl_load" != xno; then
		AC_DEFINE(HAVE_SHL_LOAD,1,
			  [Use shl_load to load shared libraries])
		AC_CHECK_HEADERS([dl.h])
	fi
else
	AC_DEFINE(HAVE_DLOPEN,1,[Use dlopen to load shared libraries])
	AC_CHECK_HEADERS([dlfcn.h])
fi
if test "x$ac_cv_header_dlfcn_h" = xyes -o "x$ac_cv_header_dl_h" = xyes; then
	HAVE_LOADABLE_MODULES=yes
else
	HAVE_LOADABLE_MODULES=no
fi
AC_MSG_RESULT($HAVE_LOADABLE_MODULES)

AC_MSG_CHECKING([if loadable Xcursor library support should be enabled])
AC_ARG_ENABLE(loadable-xcursor,
	      AS_HELP_STRING([--disable-loadable-xcursor],
	       [Controls loadable xcursor library support]),
	       [XLIB_LOADABLE_XCURSOR=$enableval],
	       [XLIB_LOADABLE_XCURSOR=$HAVE_LOADABLE_MODULES])
if test x$XLIB_LOADABLE_XCURSOR = xyes; then
	AC_DEFINE(USE_DYNAMIC_XCURSOR,1,
		  [Use the X cursor library to load cursors])
fi
AC_MSG_RESULT($XLIB_LOADABLE_XCURSOR)

# Checks for header files.
AC_CHECK_HEADERS([sys/filio.h sys/select.h sys/ioctl.h sys/socket.h])

# Checks for typedefs, structures, and compiler characteristics.
AX_GCC_BUILTIN([__builtin_popcountl])

# Checks for library functions.
AC_CHECK_FUNCS([strtol seteuid])
AC_REPLACE_FUNCS([reallocarray])
# Used in lcFile.c (see also --enable-xlocaledir settings below)
XLOCALEDIR_IS_SAFE="no"
AC_CHECK_DECL([issetugid])
AC_CHECK_FUNC([issetugid])
if test "$ac_cv_have_decl_issetugid" = yes && test "$ac_cv_func_issetugid" = yes; then
    XLOCALEDIR_IS_SAFE="yes"
    AC_DEFINE(HASSETUGID,1,[Has issetugid() function])
fi
AC_CHECK_FUNC([getresuid], [XLOCALEDIR_IS_SAFE="yes"]
	AC_DEFINE(HASGETRESUID,1,[Has getresuid() & getresgid() functions]))
# Used in Font.c
AC_CHECK_FUNC([shmat], AC_DEFINE(HAS_SHM,1,[Has shm*() functions]))

# Checks for system services
# AC_PATH_XTRA

# Threading support...

AC_ARG_ENABLE(xthreads,
              AS_HELP_STRING([--disable-xthreads],
                [Disable Xlib support for Multithreading]),
              [xthreads=$enableval],[xthreads=yes])

AC_CHECK_LIB(c, getpwuid_r, [mtsafeapi="yes"], [mtsafeapi="no"])

case x$xthreads in
xyes)
        AC_DEFINE(XTHREADS,1,[Whether libNX_X11 is compiled with thread support])
	if test x$mtsafeapi = xyes
	then
        AC_DEFINE(XUSE_MTSAFE_API,1,[Whether libNX_X11 needs to use MT safe API's])
	fi
	;;
*)
	;;
esac

AC_CHECK_LIB(c, pthread_self, [thrstubs="no"], [thrstubs="yes"])
AM_CONDITIONAL(THRSTUBS, test x$thrstubs = xyes)

# XXX incomplete, please fill this in
if test x$xthreads = xyes ; then
    case $host_os in
    linux*|gnu*|k*bsd*-gnu)
        XTHREADLIB=-lpthread ;;
    netbsd*)
	XTHREAD_CFLAGS="-D_POSIX_THREAD_SAFE_FUNCTIONS"
	XTHREADLIB="-lpthread" ;;
    freebsd*)
        XTHREAD_CFLAGS="-D_THREAD_SAFE"
        XTHREADLIB="-pthread" ;;
    dragonfly*|openbsd*)
        XTHREADLIB="-pthread" ;;
    solaris*)
	XTHREAD_CFLAGS="-D_REENTRANT -D_POSIX_PTHREAD_SEMANTICS" ;;
    esac
fi
AC_SUBST(XTHREADLIB)
AC_SUBST(XTHREAD_CFLAGS)

case x$xdmauth in
xyes)
	XDMCP_CFLAGS="$XDMCP_CFLAGS -DHASXDMAUTH"
	;;
xno)
	XDMCP_LIBS=""
	;;
esac
AC_SUBST(XDMCP_CFLAGS)
AC_SUBST(XDMCP_LIBS)

AC_CHECK_FUNC(poll, [has_poll="yes"], [has_poll="no"])

AC_ARG_ENABLE([poll],
        AS_HELP_STRING([--disable-poll],
          [Disable poll() usage in XlibInt]),
        AS_IF([test "x$enable_poll" = "xno"],
          [want_poll="no"], [want_poll="yes"]),
          [want_poll="yes"])

if test x"$has_poll" = "xyes" && test x"$want_poll" = "xyes"; then
        use_poll="yes"
        AC_DEFINE(USE_POLL,1,[Enable using the poll() function in XlibInt])
else
        use_poll="no"
fi

#
# Find keysymdef.h
#
AC_MSG_CHECKING([keysym definitions])
AC_ARG_WITH([keysymdefdir],
        [AC_HELP_STRING([--with-keysymdefdir=DIR], [The location of keysymdef.h (defaults to xproto include dir)])],
        [KEYSYMDEFDIR=$withval],
        [KEYSYMDEFDIR=`pwd`/../include])

if test ! -d "$KEYSYMDEFDIR"; then
	AC_MSG_ERROR([$KEYSYMDEFDIR doesn't exist or isn't a directory])
fi

FILES="keysymdef.h XF86keysym.h Sunkeysym.h DECkeysym.h HPkeysym.h"
for i in $FILES; do
    if test -f "$KEYSYMDEFDIR/$i"; then
            KEYSYMDEFS="$KEYSYMDEFS $KEYSYMDEFDIR/$i"
    elif test "x$i" = "xkeysymdef.h"; then
	    AC_MSG_ERROR([Cannot find keysymdef.h])
    fi
done
AC_MSG_RESULT([$KEYSYMDEFS])
AC_SUBST(KEYSYMDEFS)

AC_ARG_ENABLE(xcms,
              AS_HELP_STRING([--disable-xcms],
                [Disable Xlib support for CMS *EXPERIMENTAL*]),
              [XCMS=$enableval],[XCMS=yes])
AM_CONDITIONAL(XCMS, [test x$XCMS = xyes ])
if test x"$XCMS" = "xyes"; then
	AC_DEFINE(XCMS,1,[Include support for XCMS])
fi

AC_ARG_ENABLE(xlocale,
              AS_HELP_STRING([--disable-xlocale],
                [Disable Xlib locale implementation *EXPERIMENTAL*]),
              [XLOCALE=$enableval],[XLOCALE=yes])

AM_CONDITIONAL(XLOCALE, [ test x$XLOCALE = xyes ])
if test x"$XLOCALE" = "xyes"; then
	AC_DEFINE(XLOCALE,1,[support for X Locales])
fi

# This disables XLOCALEDIR.  Set it if you're using BuildLoadableXlibI18n,
# don't have either issetugid() or getresuid(), and you need to protect
# clients that are setgid or setuid to an id other than 0.
AC_MSG_CHECKING([if XLOCALEDIR support should be enabled])
AC_ARG_ENABLE(xlocaledir,
	      AS_HELP_STRING([--enable-xlocaledir],
		[Enable XLOCALEDIR environment variable support]),
	      [ENABLE_XLOCALEDIR=$enableval],[ENABLE_XLOCALEDIR=$XLOCALEDIR_IS_SAFE])
if test "x$ENABLE_XLOCALEDIR" = "xno"; then
	AC_DEFINE(NO_XLOCALEDIR,1,[Disable XLOCALEDIR environment variable])
fi
AC_MSG_RESULT($ENABLE_XLOCALEDIR)

AC_ARG_ENABLE(xf86bigfont,
	      AS_HELP_STRING([--disable-xf86bigfont],
		[Disable XF86BigFont extension support]),
	      [XF86BIGFONT=$enableval],[XF86BIGFONT="yes"])
if test "x$XF86BIGFONT" = "xyes"; then
# FIXME: Skip check for proto headers for now... Requires .pc files for all the proto
# header packages we have. Keeping that in mind for later...
#    PKG_CHECK_MODULES(BIGFONT, [xf86bigfontproto >= 1.2.0],
        AC_DEFINE(XF86BIGFONT,1,[Enable XF86BIGFONT extension])
#   ,XF86BIGFONT="no")
fi

AC_ARG_ENABLE(xkb,
              AS_HELP_STRING([--disable-xkb],
                [Disable XKB support *EXPERIMENTAL*]),
              [XKB=$enableval],[XKB=yes])

AM_CONDITIONAL(XKB, [ test x$XKB = xyes ])
if test x"$XKB" = "xyes"; then
   XKBPROTO_REQUIRES="kbproto"
   X11_REQUIRES="${X11_REQUIRES} kbproto inputproto"
   AC_DEFINE(XKB,1,[Use XKB])
else
   XKBPROTO_REQUIRES=""
fi
AC_SUBST(XKBPROTO_REQUIRES)

AC_FUNC_MMAP()
composecache_default=$ac_cv_func_mmap_fixed_mapped
AC_CHECK_FUNC(nl_langinfo, , [composecache_default=no])
AC_ARG_ENABLE(composecache,
              AS_HELP_STRING([--disable-composecache],
                [Disable compose table cache support]),
              [COMPOSECACHE=$enableval],[COMPOSECACHE=$composecache_default])
if test x"$COMPOSECACHE" = "xyes"; then
	AC_DEFINE(COMPOSECACHE,1,[Include compose table cache support])
fi

X11_DATADIR="${datadir}/nx/X11"
AX_DEFINE_DIR(X11_DATADIR, X11_DATADIR, [Location of libNX_X11 data])
AC_SUBST(X11_DATADIR)

X11_LIBDIR="${libdir}"
AX_DEFINE_DIR(X11_LIBDIR, X11_LIBDIR, [Location of libNX_X11 library data])
AC_SUBST(X11_LIBDIR)

# FIXME: Skip check for proto headers for now... Requires .pc files for all the proto
# header packages we have. Keeping that in mind for later...
#PKG_CHECK_MODULES(X11, [$X11_REQUIRES])
X11_CFLAGS="$X11_CFLAGS $XTHREAD_CFLAGS $BASE_CFLAGS"

#
# Yes, it would be nice to put the locale data in
# /usr/share, but the locale stuff includes loadable
# libraries which must be located in the same directory
# as the other locale data, so for now, everything lives
# in ${libdir}
#

AC_ARG_WITH(locale-lib-dir,     AS_HELP_STRING([--with-locale-lib-dir=DIR],
 [Directory where locale libraries files are installed (default: $libdir/X11/locale)]),
                                [ X11_LOCALELIBDIR="$withval" ],
                                [ X11_LOCALELIBDIR="${X11_LIBDIR}/locale" ])
AX_DEFINE_DIR(XLOCALELIBDIR, X11_LOCALELIBDIR, [Location of libNX_X11 locale libraries])
AC_SUBST(X11_LOCALELIBDIR)

AC_ARG_WITH(locale-data-dir,     AS_HELP_STRING([--with-locale-data-dir=DIR],
 [Directory where locale data files are installed (default: /usr/share/X11/locale)]),
                                [ X11_LOCALEDATADIR="$withval" ],
                                [ X11_LOCALEDATADIR="/usr/share/X11/locale" ])
AX_DEFINE_DIR(XLOCALEDATADIR, X11_LOCALEDATADIR, [Location of libNX_X11 locale data files])
AC_SUBST(X11_LOCALEDATADIR)

X11_LOCALEDIR="${X11_LOCALEDATADIR}"
AX_DEFINE_DIR(XLOCALEDIR, X11_LOCALEDIR, [Location of libNX_X11 locale data])
AC_SUBST(X11_LOCALEDIR)

XERRORDB="${X11_DATADIR}/XErrorDB"
AX_DEFINE_DIR(XERRORDB, XERRORDB, [Location of error message database])

NX_CHECK_MALLOC_ZERO

AC_CONFIG_FILES([include/Makefile
		 modules/im/ximcp/Makefile
		 modules/im/Makefile
		 modules/lc/gen/Makefile
		 modules/lc/def/Makefile
		 modules/lc/Utf8/Makefile
		 modules/lc/Makefile
		 modules/om/generic/Makefile
		 modules/om/Makefile
		 modules/Makefile
		 src/util/Makefile
		 src/xcms/Makefile
		 src/xkb/Makefile
		 src/xlibi18n/Makefile
		 src/Makefile
		 Makefile
		 nx-x11.pc])
AC_OUTPUT

echo ""
echo "NX_X11 will be built with the following settings:"
echo " Loadable xcursor library support:        "$XLIB_LOADABLE_XCURSOR
echo " Threading support:                       "$xthreads
echo " Use Threads safe API:                    "$mtsafeapi
echo " Threads stubs in libNX_X11:              "$thrstubs
echo " USE_POLL:                                "$use_poll
echo " XCMS:                                    "$XCMS
echo " Internationalization support:            "$XLOCALE
echo " XF86BigFont support:                     "$XF86BIGFONT
echo " XKB support:                             "$XKB
echo " XLOCALEDIR environment variable support: "$ENABLE_XLOCALEDIR
echo " Compose table cache enabled:             "$COMPOSECACHE
echo ""
