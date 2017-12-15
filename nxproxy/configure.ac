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
NX_DEFAULT_OPTIONS

dnl This is a workaround for a nasty libtool bug.
dnl We actually compile libXcomp with pthread support, but libtool uses g++ ... -nostdlib ... -pthread
dnl on Linux. -nostdlib causes -pthread to be ignored.
dnl According to GCC upstream, this is not a bug: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=25460
dnl GCC upstream insists that libtool needs to handle -pthread in a special way when using -nostdlib.
dnl A patch has been submitted to the libtool project: http://lists.gnu.org/archive/html/libtool-patches/2013-11/msg00015.html
dnl but never applied to the code base. Boo.
dnl We will work around this bug by using AX_PTHREAD and linking with pthread support in all consumers of
dnl libXcomp for now.
AX_PTHREAD([], AC_MSG_ERROR([no POSIX threads support detected]))

AC_ARG_ENABLE([cxx11],
              [AS_HELP_STRING([--enable-cxx11],
                              [enable optional features requiring C++11 support (disabled by default)])],
              [AS_IF([test x$enableval = xyes],
                     [AX_CXX_COMPILE_STDCXX_11([], [mandatory])])])

AC_CONFIG_FILES([
Makefile
man/Makefile
src/Makefile
])

AC_OUTPUT
