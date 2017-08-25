dnl nx-macros.m4.  Generated from xorg-macros.m4.in xorgversion.m4 by configure.
dnl
dnl Copyright (c) 2005, 2006, Oracle and/or its affiliates. All rights reserved.
dnl Copyright (c) 2017, Mike Gabriel <mike.gabriel@das-netzwerkteam.de>
dnl
dnl Permission is hereby granted, free of charge, to any person obtaining a
dnl copy of this software and associated documentation files (the "Software"),
dnl to deal in the Software without restriction, including without limitation
dnl the rights to use, copy, modify, merge, publish, distribute, sublicense,
dnl and/or sell copies of the Software, and to permit persons to whom the
dnl Software is furnished to do so, subject to the following conditions:
dnl
dnl The above copyright notice and this permission notice (including the next
dnl paragraph) shall be included in all copies or substantial portions of the
dnl Software.
dnl
dnl THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
dnl IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
dnl FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
dnl THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
dnl LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
dnl FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
dnl DEALINGS IN THE SOFTWARE.

# NX_COMPILER_BRAND
# -------------------
# Checks for various brands of compilers and sets flags as appropriate:
#   GNU gcc - relies on AC_PROG_CC (via AC_PROG_CC_C99) to set GCC to "yes"
#   GNU g++ - relies on AC_PROG_CXX to set GXX to "yes"
#   clang compiler - sets CLANGCC to "yes"
#   Intel compiler - sets INTELCC to "yes"
#   Sun/Oracle Solaris Studio cc - sets SUNCC to "yes"
#
# Derived from https://cgit.freedesktop.org/xorg/util/macros/ and adapted to
# nxcomp{,shad}.

AC_DEFUN([NX_COMPILER_BRAND], [
AC_LANG_CASE(
	[C], [
		AC_REQUIRE([AC_PROG_CC_C99])
	],
	[C++], [
		AC_REQUIRE([AC_PROG_CXX])
	]
)
AC_CHECK_DECL([__clang__], [CLANGCC="yes"], [CLANGCC="no"])
AC_CHECK_DECL([__INTEL_COMPILER], [INTELCC="yes"], [INTELCC="no"])
AC_CHECK_DECL([__SUNPRO_C], [SUNCC="yes"], [SUNCC="no"])
]) # NX_COMPILER_BRAND

# NX_TESTSET_CFLAG(<variable>, <flag>, [<alternative flag>, ...])
# ---------------
# Test if the compiler works when passed the given flag as a command line argument.
# If it succeeds, the flag is appeneded to the given variable.  If not, it tries the
# next flag in the list until there are no more options.
#
# Note that this does not guarantee that the compiler supports the flag as some
# compilers will simply ignore arguments that they do not understand, but we do
# attempt to weed out false positives by using -Werror=unknown-warning-option and
# -Werror=unused-command-line-argument
#
# Derived from https://cgit.freedesktop.org/xorg/util/macros/ and adapted to
# nxcomp{,shad}.

AC_DEFUN([NX_TESTSET_CFLAG], [
m4_if([$#], 0, [m4_fatal([NX_TESTSET_CFLAG was given with an unsupported number of arguments])])
m4_if([$#], 1, [m4_fatal([NX_TESTSET_CFLAG was given with an unsupported number of arguments])])

AC_LANG_COMPILER_REQUIRE

AC_LANG_CASE(
	[C], [
		AC_REQUIRE([AC_PROG_CC_C99])
		define([PREFIX], [C])
		define([CACHE_PREFIX], [cc])
		define([COMPILER], [$CC])
	],
	[C++], [
		define([PREFIX], [CXX])
		define([CACHE_PREFIX], [cxx])
		define([COMPILER], [$CXX])
	]
)

[nx_testset_save_]PREFIX[FLAGS]="$PREFIX[FLAGS]"

if test "x$[nx_testset_]CACHE_PREFIX[_unknown_warning_option]" = "x" ; then
	PREFIX[FLAGS]="$PREFIX[FLAGS] -Werror=unknown-warning-option"
	AC_CACHE_CHECK([if ]COMPILER[ supports -Werror=unknown-warning-option],
			[nx_cv_]CACHE_PREFIX[_flag_unknown_warning_option],
			AC_COMPILE_IFELSE([AC_LANG_SOURCE([int i;])],
					  [nx_cv_]CACHE_PREFIX[_flag_unknown_warning_option=yes],
					  [nx_cv_]CACHE_PREFIX[_flag_unknown_warning_option=no]))
	[nx_testset_]CACHE_PREFIX[_unknown_warning_option]=$[nx_cv_]CACHE_PREFIX[_flag_unknown_warning_option]
	PREFIX[FLAGS]="$[nx_testset_save_]PREFIX[FLAGS]"
fi

if test "x$[nx_testset_]CACHE_PREFIX[_unused_command_line_argument]" = "x" ; then
	if test "x$[nx_testset_]CACHE_PREFIX[_unknown_warning_option]" = "xyes" ; then
		PREFIX[FLAGS]="$PREFIX[FLAGS] -Werror=unknown-warning-option"
	fi
	PREFIX[FLAGS]="$PREFIX[FLAGS] -Werror=unused-command-line-argument"
	AC_CACHE_CHECK([if ]COMPILER[ supports -Werror=unused-command-line-argument],
			[nx_cv_]CACHE_PREFIX[_flag_unused_command_line_argument],
			AC_COMPILE_IFELSE([AC_LANG_SOURCE([int i;])],
					  [nx_cv_]CACHE_PREFIX[_flag_unused_command_line_argument=yes],
					  [nx_cv_]CACHE_PREFIX[_flag_unused_command_line_argument=no]))
	[nx_testset_]CACHE_PREFIX[_unused_command_line_argument]=$[nx_cv_]CACHE_PREFIX[_flag_unused_command_line_argument]
	PREFIX[FLAGS]="$[nx_testset_save_]PREFIX[FLAGS]"
fi

found="no"
m4_foreach([flag], m4_cdr($@), [
	if test $found = "no" ; then
		if test "x$nx_testset_]CACHE_PREFIX[_unknown_warning_option" = "xyes" ; then
			PREFIX[FLAGS]="$PREFIX[FLAGS] -Werror=unknown-warning-option"
		fi

		if test "x$nx_testset_]CACHE_PREFIX[_unused_command_line_argument" = "xyes" ; then
			PREFIX[FLAGS]="$PREFIX[FLAGS] -Werror=unused-command-line-argument"
		fi

		PREFIX[FLAGS]="$PREFIX[FLAGS] ]flag["

dnl Some hackery here since AC_CACHE_VAL can't handle a non-literal varname
		AC_MSG_CHECKING([if ]COMPILER[ supports ]flag[])
		cacheid=AS_TR_SH([nx_cv_]CACHE_PREFIX[_flag_]flag[])
		AC_CACHE_VAL($cacheid,
			     [AC_LINK_IFELSE([AC_LANG_PROGRAM([int i;])],
					     [eval $cacheid=yes],
					     [eval $cacheid=no])])

		PREFIX[FLAGS]="$[nx_testset_save_]PREFIX[FLAGS]"

		eval supported=\$$cacheid
		AC_MSG_RESULT([$supported])
		if test "$supported" = "yes" ; then
			$1="$$1 ]flag["
			found="yes"
		fi
	fi
])
]) # NX_TESTSET_CFLAG

# NX_COMPILER_FLAGS
# ---------------
# Defines BASE_CFLAGS or BASE_CXXFLAGS to contain a set of command line
# arguments supported by the selected compiler which do NOT alter the generated
# code.  These arguments will cause the compiler to print various warnings
# during compilation AND turn a conservative set of warnings into errors.
#
# The set of flags supported by BASE_CFLAGS and BASE_CXXFLAGS will grow in
# future versions of util-macros as options are added to new compilers.
#
# Derived from https://cgit.freedesktop.org/xorg/util/macros/ and adapted to
# nxcomp{,shad}.

AC_DEFUN([NX_COMPILER_FLAGS], [
AC_REQUIRE([NX_COMPILER_BRAND])

AC_ARG_ENABLE(selective-werror,
              AS_HELP_STRING([--disable-selective-werror],
                             [Turn off selective compiler errors. (default: enabled)]),
              [SELECTIVE_WERROR=$enableval],
              [SELECTIVE_WERROR=yes])

AC_LANG_CASE(
        [C], [
                define([PREFIX], [C])
        ],
        [C++], [
                define([PREFIX], [CXX])
        ]
)
# -v is too short to test reliably with NX_TESTSET_CFLAG
if test "x$SUNCC" = "xyes"; then
    [BASE_]PREFIX[FLAGS]="-v"
else
    [BASE_]PREFIX[FLAGS]=""
fi

# This chunk of warnings were those that existed in the legacy CWARNFLAGS
NX_TESTSET_CFLAG([[BASE_]PREFIX[FLAGS]], [-Wall])
NX_TESTSET_CFLAG([[BASE_]PREFIX[FLAGS]], [-Wpointer-arith])
NX_TESTSET_CFLAG([[BASE_]PREFIX[FLAGS]], [-Wmissing-declarations])
NX_TESTSET_CFLAG([[BASE_]PREFIX[FLAGS]], [-Wformat=2], [-Wformat])

AC_LANG_CASE(
	[C], [
		NX_TESTSET_CFLAG([[BASE_]PREFIX[FLAGS]], [-Wstrict-prototypes])
		NX_TESTSET_CFLAG([[BASE_]PREFIX[FLAGS]], [-Wmissing-prototypes])
		NX_TESTSET_CFLAG([[BASE_]PREFIX[FLAGS]], [-Wnested-externs])
		NX_TESTSET_CFLAG([[BASE_]PREFIX[FLAGS]], [-Wbad-function-cast])
		NX_TESTSET_CFLAG([[BASE_]PREFIX[FLAGS]], [-Wold-style-definition], [-fd])
		NX_TESTSET_CFLAG([[BASE_]PREFIX[FLAGS]], [-Wdeclaration-after-statement])
	]
)

# This chunk adds additional warnings that could catch undesired effects.
NX_TESTSET_CFLAG([[BASE_]PREFIX[FLAGS]], [-Wunused])
NX_TESTSET_CFLAG([[BASE_]PREFIX[FLAGS]], [-Wuninitialized])
NX_TESTSET_CFLAG([[BASE_]PREFIX[FLAGS]], [-Wshadow])
NX_TESTSET_CFLAG([[BASE_]PREFIX[FLAGS]], [-Wmissing-noreturn])
NX_TESTSET_CFLAG([[BASE_]PREFIX[FLAGS]], [-Wmissing-format-attribute])
NX_TESTSET_CFLAG([[BASE_]PREFIX[FLAGS]], [-Wredundant-decls])
NX_TESTSET_CFLAG([[BASE_]PREFIX[FLAGS]], [-Wlogical-op])

# These are currently disabled because they are noisy.  They will be enabled
# in the future once the codebase is sufficiently modernized to silence
# them.  For now, I don't want them to drown out the other warnings.
# NX_TESTSET_CFLAG([[BASE_]PREFIX[FLAGS]], [-Wparentheses])
# NX_TESTSET_CFLAG([[BASE_]PREFIX[FLAGS]], [-Wcast-align])
# NX_TESTSET_CFLAG([[BASE_]PREFIX[FLAGS]], [-Wcast-qual])

# Turn some warnings into errors, so we don't accidently get successful builds
# when there are problems that should be fixed.

if test "x$SELECTIVE_WERROR" = "xyes" ; then
NX_TESTSET_CFLAG([[BASE_]PREFIX[FLAGS]], [-Werror=implicit], [-errwarn=E_NO_EXPLICIT_TYPE_GIVEN -errwarn=E_NO_IMPLICIT_DECL_ALLOWED])
NX_TESTSET_CFLAG([[BASE_]PREFIX[FLAGS]], [-Werror=nonnull])
NX_TESTSET_CFLAG([[BASE_]PREFIX[FLAGS]], [-Werror=init-self])
NX_TESTSET_CFLAG([[BASE_]PREFIX[FLAGS]], [-Werror=main])
NX_TESTSET_CFLAG([[BASE_]PREFIX[FLAGS]], [-Werror=missing-braces])
NX_TESTSET_CFLAG([[BASE_]PREFIX[FLAGS]], [-Werror=sequence-point])
NX_TESTSET_CFLAG([[BASE_]PREFIX[FLAGS]], [-Werror=return-type], [-errwarn=E_FUNC_HAS_NO_RETURN_STMT])
NX_TESTSET_CFLAG([[BASE_]PREFIX[FLAGS]], [-Werror=trigraphs])
NX_TESTSET_CFLAG([[BASE_]PREFIX[FLAGS]], [-Werror=array-bounds])
NX_TESTSET_CFLAG([[BASE_]PREFIX[FLAGS]], [-Werror=write-strings])
NX_TESTSET_CFLAG([[BASE_]PREFIX[FLAGS]], [-Werror=address])
NX_TESTSET_CFLAG([[BASE_]PREFIX[FLAGS]], [-Werror=int-to-pointer-cast], [-errwarn=E_BAD_PTR_INT_COMBINATION])
NX_TESTSET_CFLAG([[BASE_]PREFIX[FLAGS]], [-Werror=pointer-to-int-cast]) # Also -errwarn=E_BAD_PTR_INT_COMBINATION
else
AC_MSG_WARN([You have chosen not to turn some select compiler warnings into errors.  This should not be necessary.  Please report why you needed to do so in a bug report at $PACKAGE_BUGREPORT])
NX_TESTSET_CFLAG([[BASE_]PREFIX[FLAGS]], [-Wimplicit])
NX_TESTSET_CFLAG([[BASE_]PREFIX[FLAGS]], [-Wnonnull])
NX_TESTSET_CFLAG([[BASE_]PREFIX[FLAGS]], [-Winit-self])
NX_TESTSET_CFLAG([[BASE_]PREFIX[FLAGS]], [-Wmain])
NX_TESTSET_CFLAG([[BASE_]PREFIX[FLAGS]], [-Wmissing-braces])
NX_TESTSET_CFLAG([[BASE_]PREFIX[FLAGS]], [-Wsequence-point])
NX_TESTSET_CFLAG([[BASE_]PREFIX[FLAGS]], [-Wreturn-type])
NX_TESTSET_CFLAG([[BASE_]PREFIX[FLAGS]], [-Wtrigraphs])
NX_TESTSET_CFLAG([[BASE_]PREFIX[FLAGS]], [-Warray-bounds])
NX_TESTSET_CFLAG([[BASE_]PREFIX[FLAGS]], [-Wwrite-strings])
NX_TESTSET_CFLAG([[BASE_]PREFIX[FLAGS]], [-Waddress])
NX_TESTSET_CFLAG([[BASE_]PREFIX[FLAGS]], [-Wint-to-pointer-cast])
NX_TESTSET_CFLAG([[BASE_]PREFIX[FLAGS]], [-Wpointer-to-int-cast])
fi

AC_SUBST([BASE_]PREFIX[FLAGS])
]) # NX_COMPILER_FLAGS

# NX_STRICT_OPTION
# -----------------------
#
# Add configure option to enable strict compilation flags, such as treating
# warnings as fatal errors.
# If --enable-strict-compilation is passed to configure, adds strict flags to
# $BASE_CFLAGS or $BASE_CXXFLAGS.
#
# Also exports $STRICT_CFLAGS for use in other tests or when strict compilation
# is unconditionally desired.
AC_DEFUN([NX_STRICT_OPTION], [
AC_REQUIRE([NX_COMPILER_FLAGS])

AC_ARG_ENABLE(strict-compilation,
                          AS_HELP_STRING([--enable-strict-compilation],
                          [Enable all warnings from compiler and make them errors (default: disabled)]),
                          [STRICT_COMPILE=$enableval], [STRICT_COMPILE=no])

AC_LANG_CASE(
        [C], [
                define([PREFIX], [C])
        ],
        [C++], [
                define([PREFIX], [CXX])
        ]
)

[STRICT_]PREFIX[FLAGS]=""
NX_TESTSET_CFLAG([[STRICT_]PREFIX[FLAGS]], [-pedantic])
NX_TESTSET_CFLAG([[STRICT_]PREFIX[FLAGS]], [-Werror], [-errwarn])

# Earlier versions of gcc (eg: 4.2) support -Werror=attributes, but do not
# activate it with -Werror, so we add it here explicitly.
NX_TESTSET_CFLAG([[STRICT_]PREFIX[FLAGS]], [-Werror=attributes])

if test "x$STRICT_COMPILE" = "xyes"; then
    [BASE_]PREFIX[FLAGS]="$[BASE_]PREFIX[FLAGS] $[STRICT_]PREFIX[FLAGS]"
fi
AC_SUBST([STRICT_]PREFIX[FLAGS])
AC_SUBST([BASE_]PREFIX[FLAGS])
]) # NX_STRICT_OPTION

# NX_DEFAULT_OPTIONS
# --------------------
#
# Defines default options for X.Org-like modules.
#
AC_DEFUN([NX_DEFAULT_OPTIONS], [
AC_REQUIRE([AC_PROG_INSTALL])
NX_COMPILER_FLAGS
NX_STRICT_OPTION
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])],
    [AC_SUBST([AM_DEFAULT_VERBOSITY], [1])])
]) # NX_DEFAULT_OPTIONS

# NX_CHECK_MALLOC_ZERO
# ----------------------
# Minimum version: 1.0.0
#
# Defines {MALLOC,XMALLOC,XTMALLOC}_ZERO_CFLAGS appropriately if
# malloc(0) returns NULL.  Packages should add one of these cflags to
# their AM_CFLAGS (or other appropriate *_CFLAGS) to use them.
AC_DEFUN([NX_CHECK_MALLOC_ZERO],[
AC_ARG_ENABLE(malloc0returnsnull,
        AS_HELP_STRING([--enable-malloc0returnsnull],
                       [malloc(0) returns NULL (default: auto)]),
        [MALLOC_ZERO_RETURNS_NULL=$enableval],
        [MALLOC_ZERO_RETURNS_NULL=auto])

AC_MSG_CHECKING([whether malloc(0) returns NULL])
if test "x$MALLOC_ZERO_RETURNS_NULL" = xauto; then
        AC_RUN_IFELSE([AC_LANG_PROGRAM([
#include <stdlib.h>
],[
    char *m0, *r0, *c0, *p;
    m0 = malloc(0);
    p = malloc(10);
    r0 = realloc(p,0);
    c0 = calloc(0,10);
    exit((m0 == 0 || r0 == 0 || c0 == 0) ? 0 : 1);
])],
                [MALLOC_ZERO_RETURNS_NULL=yes],
                [MALLOC_ZERO_RETURNS_NULL=no],
                [MALLOC_ZERO_RETURNS_NULL=yes])
fi
AC_MSG_RESULT([$MALLOC_ZERO_RETURNS_NULL])

if test "x$MALLOC_ZERO_RETURNS_NULL" = xyes; then
        MALLOC_ZERO_CFLAGS="-DMALLOC_0_RETURNS_NULL"
        XMALLOC_ZERO_CFLAGS=$MALLOC_ZERO_CFLAGS
        XTMALLOC_ZERO_CFLAGS="$MALLOC_ZERO_CFLAGS -DXTMALLOC_BC"
else
        MALLOC_ZERO_CFLAGS=""
        XMALLOC_ZERO_CFLAGS=""
        XTMALLOC_ZERO_CFLAGS=""
fi

AC_SUBST([MALLOC_ZERO_CFLAGS])
AC_SUBST([XMALLOC_ZERO_CFLAGS])
AC_SUBST([XTMALLOC_ZERO_CFLAGS])
]) # NX_CHECK_MALLOC_ZERO


dnl Check to see if we're running under Cygwin32.

AC_DEFUN([NX_BUILD_ON_CYGWIN32],
[AC_CACHE_CHECK([for Cygwin32 environment], nxconf_cv_cygwin32,
[AC_TRY_COMPILE(,[return __CYGWIN32__;],
nxconf_cv_cygwin32=yes, nxconf_cv_cygwin32=no)
rm -f conftest*])
CYGWIN32=
test "$nxconf_cv_cygwin32" = yes && CYGWIN32=yes
]) # NX_BUILD_ON_CYGWIN32

dnl Check whether we're building on a AMD64.

AC_DEFUN([NX_BUILD_ON_AMD64],
[AC_CACHE_CHECK([for Amd64 environment], nxconf_cv_amd64,
[AC_TRY_COMPILE(,[return (__amd64__ || __x86_64__);],
nxconf_cv_amd64=yes, nxconf_cv_amd64=no)
rm -f conftest*])
AMD64=
test "$nxconf_cv_amd64" = yes && AMD64=yes
]) # NX_BUILD_ON_AMD64

dnl Check for Darwin environment.

AC_DEFUN([NX_BUILD_ON_DARWIN],
[AC_CACHE_CHECK([for Darwin environment], nxconf_cv_darwin,
[AC_TRY_COMPILE(,[return __APPLE__;],
nxconf_cv_darwin=yes, nxconf_cv_darwin=no)
rm -f conftest*])
DARWIN=
test "$nxconf_cv_darwin" = yes && DARWIN=yes
]) # NX_BUILD_ON_DARWIN

# Check to see if we're running under Solaris.

AC_DEFUN([NX_BUILD_ON_SUN],
[AC_CACHE_CHECK([for Solaris environment], nxconf_cv_sun,
[AC_TRY_COMPILE(,[return __sun;],
nxconf_cv_sun=yes, nxconf_cv_sun=no)
rm -f conftest*])
SUN=
test "$nxconf_cv_sun" = yes && SUN=yes
]) # NX_BUILD_ON_SUN

# Check to see if we're running under FreeBSD.

AC_DEFUN([NX_BUILD_ON_FreeBSD],
[AC_CACHE_CHECK([for FreeBSD environment], nxconf_cv_freebsd,
[AC_TRY_COMPILE(,[return __FreeBSD__;],
nxconf_cv_freebsd=yes, nxconf_cv_freebsd=no)
rm -f conftest*])
FreeBSD=
test "$nxconf_cv_freebsd" = yes && FreeBSD=yes
]) # NX_BUILD_ON_FreeBSD

AC_DEFUN([LIBJPEG_FALLBACK_CHECK],[
AC_MSG_CHECKING([for libjpeg shared libary file and headers])
AC_CHECK_LIB([jpeg], [jpeg_destroy_compress],
    [have_jpeg_lib=yes], [have_jpeg_lib=no])
AC_CHECK_HEADERS([jpeglib.h],
    [have_jpeg_headers=yes], [have_jpeg_headers=no])

if test x"$have_jpeg_lib" = "xyes" && test x"$have_jpeg_headers" = "xyes"; then
    AC_MSG_RESULT([yes])
    JPEG_CFLAGS=""
    JPEG_LIBS="-ljpeg"
else
    AC_MSG_RESULT([no])
    AC_MSG_FAILURE([Could not find libjpeg on your system, make sure
the JPEG shared library and header files are installed.])
fi
]) # LIBJPEG_FALLBACK_CHECK
