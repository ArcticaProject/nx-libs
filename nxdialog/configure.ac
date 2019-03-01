dnl ***************************************************************************
dnl *** configure.ac for nxdialog                                           ***
dnl ***************************************************************************

m4_define([nxdialog_version], m4_esyscmd([tr -d '\n' < VERSION]))

# Initialize Autoconf
AC_PREREQ(2.60)

AC_INIT([NX Dialog], [nxdialog_version], [https://github.com/ArcticaProject/nx-libs/issues])
AC_CONFIG_AUX_DIR([build-aux])
AC_PREFIX_DEFAULT([/usr/local])

AM_INIT_AUTOMAKE([foreign no-dist-gzip dist-bzip2])

AC_CONFIG_FILES([
Makefile
bin/Makefile
man/Makefile
])

AC_OUTPUT
