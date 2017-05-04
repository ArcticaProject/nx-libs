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

NX_COMPILER_BRAND
NX_COMPILER_FLAGS

AC_CONFIG_FILES([
Makefile
src/Makefile
nxcompshad.pc
])

AC_OUTPUT
