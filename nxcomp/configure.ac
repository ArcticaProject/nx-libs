# ***************************************************************************
# *** configure.ac for nxcomp                                         ***
# ***************************************************************************

m4_define([nxcomp_version], m4_esyscmd([tr -d '\n' < VERSION]))

# Initialize Autoconf
AC_PREREQ(2.60)

AC_INIT([libXcomp], [nxcomp_version], [https://github.com/ArcticaProject/nx-libs/issues])
AC_CONFIG_AUX_DIR([build-aux])
AC_PROG_CXX
AC_CONFIG_SRCDIR([Makefile.am])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([foreign no-dist-gzip dist-xz])

# Initialize libtool
AC_PROG_LIBTOOL

COMP_VERSION=nxcomp_version
AC_SUBST([COMP_VERSION])

LT_COMP_VERSION=[`echo $COMP_VERSION | sed -r -e 's/^([0-9]+\.[0-9]+\.[0-9]+).*$/\1/' -e 's/\./:/g'`] 
AC_SUBST([LT_COMP_VERSION])

PKG_CHECK_MODULES(JPEG, libjpeg)
PKG_CHECK_MODULES(PNG, libpng)
PKG_CHECK_MODULES(Z, zlib)

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
NX_COMPILER_FLAGS

NX_BUILD_ON_CYGWIN32
NX_BUILD_ON_AMD64
NX_BUILD_ON_DARWIN
NX_BUILD_ON_SUN
NX_BUILD_ON_FreeBSD

# Build PIC libraries.

if test "$CYGWIN32" != yes -a "$DARWIN" != yes; then
  CXXFLAGS="$CXXFLAGS -fPIC"
  CFLAGS="$CFLAGS -fPIC"
fi

# On FreeBSD search libraries and includes under /usr/local.

if test "$FreeBSD" = yes; then
  LIBS="$LIBS -L/usr/local/lib"
  CPPFLAGS="$CPPFLAGS -I/usr/local/include"
fi

NX_HAS_INADDRT

# If in_addr_t is not defined use unsigned int.

if test "$INADDRT" != yes ; then
  echo -e "using unsigned int for type in_addr_t"
  CPPFLAGS="$CPPFLAGS -DIN_ADDR_T=unsigned"
else
  CPPFLAGS="$CPPFLAGS -DIN_ADDR_T=in_addr_t"
fi

AC_CONFIG_FILES([
Makefile
src/Makefile
nxcomp.pc
])

AC_OUTPUT
