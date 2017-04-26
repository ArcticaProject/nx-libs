NULL =

nxexecdir = $(libexecdir)

nxexec_PROGRAMS = nxproxy

nxproxy_SOURCES =					\
    Main.c						\
    $(NULL)

nxproxy_LDADD =						\
    -L$(top_srcdir)/../nxcomp/ -lXcomp			\
    $(NULL)

AM_CFLAGS =						\
    -I$(top_srcdir)/../nxcomp/				\
    $(NULL)
