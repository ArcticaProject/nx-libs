NULL =

nxexecdir = $(libexecdir)

nxexec_PROGRAMS = nxproxy

nxproxy_SOURCES =					\
    Main.c						\
    $(NULL)

nxproxy_LDADD =						\
    -L$(top_srcdir)/../nxcomp/src/.libs -lXcomp		\
    $(NULL)

nxproxy_CFLAGS =					\
    $(BASE_CFLAGS)					\
    $(NULL)

AM_CPPFLAGS =						\
    -I$(top_srcdir)/../nxcomp/include/			\
    $(NULL)
