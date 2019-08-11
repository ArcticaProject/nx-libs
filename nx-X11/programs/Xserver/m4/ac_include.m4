dnl dnl
dnl dnl @author Brian Stafford <brian@stafford.uklinux.net>
dnl dnl
dnl dnl based on version by Caolan McNamara <caolan@skynet.ie>
dnl dnl based on David Arnold's autoconf suggestion in the threads faq
dnl dnl
dnl @synopsis ACX_FEATURE(ENABLE_OR_WITH,NAME[,VALUE])
AC_DEFUN([ACX_FEATURE],
         [echo "builtin([substr],[                                  ],len(--$1-$2))--$1-$2: ifelse($3,,[$]translit($1-$2,-,_),$3)"])
