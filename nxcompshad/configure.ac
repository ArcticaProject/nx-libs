dnl ***************************************************************************
dnl *** configure.ac for nxcompshad                                         ***
dnl ***************************************************************************

m4_define([nxcompshad_version], m4_esyscmd([tr -d '\n' < VERSION]))

# Initialize Autoconf
AC_PREREQ(2.60)

AC_INIT([libXcompshad], [nxcompshad_version], [https://github.com/ArcticaProject/nx-libs/issues])
AC_PROG_CXX
AC_CONFIG_SRCDIR([Makefile.am])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([build-aux])

AM_INIT_AUTOMAKE([foreign no-dist-gzip dist-xz])

# Initialize libtool
AC_PROG_LIBTOOL

COMPSHAD_VERSION=nxcompshad_version
AC_SUBST([COMPSHAD_VERSION])

LT_COMPSHAD_VERSION=[`echo $COMPSHAD_VERSION | sed -r -e 's/^([0-9]+\.[0-9]+\.[0-9]+).*$/\1/' -e 's/\./:/g'`] 
AC_SUBST([LT_COMPSHAD_VERSION])

PKG_CHECK_MODULES(Xext, xext)
PKG_CHECK_MODULES(Xdamage, xdamage)
PKG_CHECK_MODULES(Xrandr, xrandr)
PKG_CHECK_MODULES(Xtst, xtst)

# Upstream's pkg.m4 (since 0.27) offers this now, but define our own
# compatible version in case the local version of pkgconfig isn't new enough.
# https://bugs.freedesktop.org/show_bug.cgi?id=48743
m4_ifdef([PKG_INSTALLDIR], [PKG_INSTALLDIR],
         [AC_ARG_WITH([pkgconfigdir],
                      [AS_HELP_STRING([--with-pkgconfigdir],
                      [install directory for nxcompshad.pc pkg-config file])],
                       [],[with_pkgconfigdir='$(libdir)/pkgconfig'])
          AC_SUBST([pkgconfigdir], [${with_pkgconfigdir}])])

AC_LANG([C++])
NX_COMPILER_BRAND
NX_DEFAULT_OPTIONS

AC_ARG_ENABLE([cxx11],
              [AS_HELP_STRING([--enable-cxx11],
                              [enable optional features requiring C++11 support (disabled by default)])],
              [AS_IF([test x$enableval = xyes],
                     [AX_CXX_COMPILE_STDCXX_11([], [mandatory])])])

AC_CONFIG_FILES([
Makefile
src/Makefile
nxcompshad.pc
])

AC_OUTPUT
