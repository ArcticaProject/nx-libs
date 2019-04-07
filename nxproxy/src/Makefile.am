NULL =

nxexecdir = $(bindir)

nxexec_PROGRAMS = nxproxy

nxproxy_SOURCES =					\
    Main.c						\
    $(NULL)

nxproxy_LDADD =						\
    -L$(top_srcdir)/../nxcomp/src/.libs -lXcomp		\
    $(NULL)

nxproxy_LDFLAGS =					\
    $(NULL)

if TARGET_ELF
nxproxy_LDFLAGS +=					\
    -Wl,--enable-new-dtags				\
    $(NULL)
endif TARGET_ELF

nxproxy_CFLAGS =					\
    $(BASE_CFLAGS)					\
    $(NULL)

AM_CPPFLAGS =						\
    -I$(top_srcdir)/../nxcomp/include/			\
    $(NULL)
