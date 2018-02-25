NULL =

nxexecdir = $(bindir)

nxexec_PROGRAMS = nxproxy

nxproxy_SOURCES =					\
    Main.c						\
    $(NULL)

nxproxy_LDADD =						\
    @PTHREAD_LIBS@					\
    -L$(top_srcdir)/../nxcomp/src/.libs -lXcomp		\
    $(NULL)

nxproxy_LDFLAGS =					\
    $(PTHREAD_LDFLAGS)					\
    $(NULL)

if TARGET_ELF
nxproxy_LDFLAGS +=					\
    -Wl,--enable-new-dtags				\
    $(NULL)
endif TARGET_ELF

nxproxy_CFLAGS =					\
    $(BASE_CFLAGS)					\
    $(PTHREAD_CFLAGS)					\
    $(NULL)

AM_CPPFLAGS =						\
    -I$(top_srcdir)/../nxcomp/include/			\
    $(NULL)
