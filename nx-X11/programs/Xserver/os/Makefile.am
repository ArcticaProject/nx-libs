AUTOMAKE_OPTIONS = subdir-objects

noinst_LTLIBRARIES = libos.la libcwrapper.la

AM_CFLAGS = $(DIX_CFLAGS) @SERVER_DEFINES@ @LOADER_DEFINES@

# FIXME: Add support for these in configure.ac
SECURERPC_SRCS = rpcauth.c

XCSECURITY_SRCS = secauth.c
XDMCP_SRCS = xdmcp.c
STRLCAT_SRCS = strlcat.c strlcpy.c
XORG_SRCS = log.c

libos_la_SOURCES = 	\
	WaitFor.c	\
	access.c	\
	auth.c		\
	connection.c	\
	io.c		\
	mitauth.c	\
	oscolor.c	\
	osdep.h		\
	osinit.c	\
	utils.c		\
	xdmauth.c	\
	xstrans.c	\
	xprintf.c	\
	$(XORG_SRCS)

if XCSECURITY
libos_la_SOURCES += $(XCSECURITY_SRCS)
endif

if XDMCP
libos_la_SOURCES += $(XDMCP_SRCS)
endif

if NEED_STRLCAT
libos_la_SOURCES += $(STRLCAT_SRCS)
endif

libcwrapper_la_SOURCES = \
	$(top_srcdir)/hw/xfree86/os-support/shared/libc_wrapper.c
libcwrapper_la_CFLAGS = \
	-DSELF_CONTAINED_WRAPPER \
	-I$(top_srcdir)/hw/xfree86/os-support \
	$(AM_CFLAGS)

EXTRA_DIST = $(SECURERPC_SRCS) $(XCSECURITY_SRCS) $(XDMCP_SRCS) $(STRLCAT_SRCS)
