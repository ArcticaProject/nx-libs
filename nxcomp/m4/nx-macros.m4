# NX_COMPILER_BRAND
# -------------------
# Checks for various brands of compilers and sets flags as appropriate:
#   GNU gcc - relies on AC_PROG_CC (via AC_PROG_CC_C99) to set GCC to "yes"
#   GNU g++ - relies on AC_PROG_CXX to set GXX to "yes"
#   clang compiler - sets CLANGCC to "yes"
#   Intel compiler - sets INTELCC to "yes"
#   Sun/Oracle Solaris Studio cc - sets SUNCC to "yes"
#
# Obtained from https://cgit.freedesktop.org/xorg/util/macros/ and adapted to
# nxcomp{,shad}.

AC_DEFUN([NX_COMPILER_BRAND], [
AC_REQUIRE([AC_PROG_CXX])
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
# Obtained from https://cgit.freedesktop.org/xorg/util/macros/ and adapted to
# nxcomp{,shad}.

AC_DEFUN([NX_TESTSET_CFLAG], [
m4_if([$#], 0, [m4_fatal([NX_TESTSET_CFLAG was given with an unsupported number of arguments])])
m4_if([$#], 1, [m4_fatal([NX_TESTSET_CFLAG was given with an unsupported number of arguments])])

AC_LANG_COMPILER_REQUIRE

[nx_testset_save_CXXFLAGS]="$CXXFLAGS"

if test "x$nx_testset_CXX_unknown_warning_option" = "x" ; then
        CXXFLAGS="$CXXFLAGS -Werror=unknown-warning-option"
        AC_CACHE_CHECK([if $CXX supports -Werror=unknown-warning-option],
                        [nx_cv_cxx_flag_unknown_warning_option],
                        AC_COMPILE_IFELSE([AC_LANG_SOURCE([int i;])],
                                          [nx_cv_cxx_flag_unknown_warning_option=yes],
                                          [nx_cv_cxx_flag_unknown_warning_option=no]))
        nx_testset_CXX_unknown_warning_option=$nx_cv_cxx_flag_unknown_warning_option
        CXXFLAGS="$nx_testset_save_CXXFLAGS"
fi

if test "x$nx_testset_CXX_unused_command_line_argument" = "x" ; then
        if test "x$nx_testset_CXX_unknown_warning_option" = "xyes" ; then
                CXXFLAGS="$CXXFLAGS -Werror=unknown-warning-option"
        fi
        CXXFLAGS="$CXXFLAGS -Werror=unused-command-line-argument"
        AC_CACHE_CHECK([if $CXX supports -Werror=unused-command-line-argument],
                        [nx_cv_cxx_flag_unused_command_line_argument],
                        AC_COMPILE_IFELSE([AC_LANG_SOURCE([int i;])],
                                          [nx_cv_cxx_flag_unused_command_line_argument=yes],
                                          [nx_cv_cxx_flag_unused_command_line_argument=no]))
        nx_testset_CXX_unused_command_line_argument=$nx_cv_cxx_flag_unused_command_line_argument
        CXXFLAGS="$nx_testset_save_CXXFLAGS"
fi

found="no"
m4_foreach([flag], m4_cdr($@), [
        if test $found = "no" ; then
                if test "x$nx_testset_unknown_warning_option" = "xyes" ; then
                        CXXFLAGS="$CXXFLAGS -Werror=unknown-warning-option"
                fi

                if test "x$nx_testset_unused_command_line_argument" = "xyes" ; then
                        CXXFLAGS="$CXXFLAGS -Werror=unused-command-line-argument"
                fi

                CXXFLAGS="$[CXXFLAGS] ]flag["

dnl Some hackery here since AC_CACHE_VAL can't handle a non-literal varname
                AC_MSG_CHECKING([if $CXX supports ]flag[])
                cacheid=AS_TR_SH([nx_cv_cxx_flag_]flag[])
                AC_CACHE_VAL($cacheid,
                             [AC_LINK_IFELSE([AC_LANG_PROGRAM([int i;])],
                                             [eval $cacheid=yes],
                                             [eval $cacheid=no])])

                CXXFLAGS="$nx_testset_save_CXXFLAGS"

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
# Obtained from https://cgit.freedesktop.org/xorg/util/macros/ and adapted to
# nxcomp{,shad}.

AC_DEFUN([NX_COMPILER_FLAGS], [
AC_REQUIRE([NX_COMPILER_BRAND])

AC_ARG_ENABLE(selective-werror,
              AS_HELP_STRING([--disable-selective-werror],
                             [Turn off selective compiler errors. (default: enabled)]),
              [SELECTIVE_WERROR=$enableval],
              [SELECTIVE_WERROR=yes])

# -v is too short to test reliably with NX_TESTSET_CFLAG
if test "x$SUNCC" = "xyes"; then
    [BASE_CXXFLAGS]="-v"
else
    [BASE_CXXFLAGS]=""
fi

# This chunk of warnings were those that existed in the legacy CWARNFLAGS
NX_TESTSET_CFLAG([BASE_CXXFLAGS], [-Wall])
NX_TESTSET_CFLAG([BASE_CXXFLAGS], [-Wpointer-arith])
NX_TESTSET_CFLAG([BASE_CXXFLAGS], [-Wmissing-declarations])
NX_TESTSET_CFLAG([BASE_CXXFLAGS], [-Wformat=2], [-Wformat])

# This chunk adds additional warnings that could catch undesired effects.
NX_TESTSET_CFLAG([BASE_CXXFLAGS], [-Wunused])
NX_TESTSET_CFLAG([BASE_CXXFLAGS], [-Wuninitialized])
NX_TESTSET_CFLAG([BASE_CXXFLAGS], [-Wshadow])
NX_TESTSET_CFLAG([BASE_CXXFLAGS], [-Wcast-qual])
NX_TESTSET_CFLAG([BASE_CXXFLAGS], [-Wmissing-noreturn])
NX_TESTSET_CFLAG([BASE_CXXFLAGS], [-Wmissing-format-attribute])
NX_TESTSET_CFLAG([BASE_CXXFLAGS], [-Wredundant-decls])

# These are currently disabled because they are noisy.  They will be enabled
# in the future once the codebase is sufficiently modernized to silence
# them.  For now, I don't want them to drown out the other warnings.
# NX_TESTSET_CFLAG([BASE_CXXFLAGS], [-Wlogical-op])
# NX_TESTSET_CFLAG([BASE_CXXFLAGS], [-Wparentheses])
# NX_TESTSET_CFLAG([BASE_CXXFLAGS], [-Wcast-align])

# Turn some warnings into errors, so we don't accidently get successful builds
# when there are problems that should be fixed.

if test "x$SELECTIVE_WERROR" = "xyes" ; then
NX_TESTSET_CFLAG([BASE_CXXFLAGS], [-Werror=implicit], [-errwarn=E_NO_EXPLICIT_TYPE_GIVEN -errwarn=E_NO_IMPLICIT_DECL_ALLOWED])
NX_TESTSET_CFLAG([BASE_CXXFLAGS], [-Werror=nonnull])
NX_TESTSET_CFLAG([BASE_CXXFLAGS], [-Werror=init-self])
NX_TESTSET_CFLAG([BASE_CXXFLAGS], [-Werror=main])
NX_TESTSET_CFLAG([BASE_CXXFLAGS], [-Werror=missing-braces])
NX_TESTSET_CFLAG([BASE_CXXFLAGS], [-Werror=sequence-point])
NX_TESTSET_CFLAG([BASE_CXXFLAGS], [-Werror=return-type], [-errwarn=E_FUNC_HAS_NO_RETURN_STMT])
NX_TESTSET_CFLAG([BASE_CXXFLAGS], [-Werror=trigraphs])
NX_TESTSET_CFLAG([BASE_CXXFLAGS], [-Werror=array-bounds])
NX_TESTSET_CFLAG([BASE_CXXFLAGS], [-Werror=write-strings])
NX_TESTSET_CFLAG([BASE_CXXFLAGS], [-Werror=address])
NX_TESTSET_CFLAG([BASE_CXXFLAGS], [-Werror=int-to-pointer-cast], [-errwarn=E_BAD_PTR_INT_COMBINATION])
NX_TESTSET_CFLAG([BASE_CXXFLAGS], [-Werror=pointer-to-int-cast]) # Also -errwarn=E_BAD_PTR_INT_COMBINATION
else
AC_MSG_WARN([You have chosen not to turn some select compiler warnings into errors.  This should not be necessary.  Please report why you needed to do so in a bug report at $PACKAGE_BUGREPORT])
NX_TESTSET_CFLAG([BASE_CXXFLAGS], [-Wimplicit])
NX_TESTSET_CFLAG([BASE_CXXFLAGS], [-Wnonnull])
NX_TESTSET_CFLAG([BASE_CXXFLAGS], [-Winit-self])
NX_TESTSET_CFLAG([BASE_CXXFLAGS], [-Wmain])
NX_TESTSET_CFLAG([BASE_CXXFLAGS], [-Wmissing-braces])
NX_TESTSET_CFLAG([BASE_CXXFLAGS], [-Wsequence-point])
NX_TESTSET_CFLAG([BASE_CXXFLAGS], [-Wreturn-type])
NX_TESTSET_CFLAG([BASE_CXXFLAGS], [-Wtrigraphs])
NX_TESTSET_CFLAG([BASE_CXXFLAGS], [-Warray-bounds])
NX_TESTSET_CFLAG([BASE_CXXFLAGS], [-Wwrite-strings])
NX_TESTSET_CFLAG([BASE_CXXFLAGS], [-Waddress])
NX_TESTSET_CFLAG([BASE_CXXFLAGS], [-Wint-to-pointer-cast])
NX_TESTSET_CFLAG([BASE_CXXFLAGS], [-Wpointer-to-int-cast])
fi

AC_SUBST([BASE_CXXFLAGS])
]) # NX_COMPILER_FLAGS
