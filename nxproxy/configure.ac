dnl ***************************************************************************
dnl *** configure.ac for nxproxy                                         ***
dnl ***************************************************************************

m4_define([nxproxy_version], m4_esyscmd([tr -d '\n' < VERSION]))

# Initialize Autoconf
AC_PREREQ(2.60)

AC_INIT([NX Proxy], [nxproxy_version], [https://github.com/ArcticaProject/nx-libs/issues])
AC_CONFIG_AUX_DIR([build-aux])
AC_PROG_CC
AC_CONFIG_SRCDIR([Makefile.am])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([foreign no-dist-gzip dist-xz])

PROXY_VERSION=nxproxy_version
AC_SUBST([PROXY_VERSION])

AC_LANG([C])
NX_COMPILER_BRAND
NX_COMPILER_FLAGS

AC_CONFIG_FILES([
Makefile
man/Makefile
src/Makefile
])

AC_OUTPUT
