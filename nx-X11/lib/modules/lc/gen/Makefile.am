NULL =

noinst_LTLIBRARIES = libxlibi18n.la

AM_CPPFLAGS =					\
	-I$(top_srcdir)/include			\
	-I$(top_srcdir)/include/X11		\
	-I$(top_builddir)/include		\
	-I$(top_builddir)/include/X11		\
	-I$(top_srcdir)/src/xcms		\
	-I$(top_srcdir)/src/xkb			\
	-I$(top_srcdir)/src/xlibi18n		\
	-I$(top_srcdir)/../exports/include	\
	-D_GNU_SOURCE				\
	$(NULL)

AM_CFLAGS =					\
	$(X11_CFLAGS)				\
	$(BIGFONT_CFLAGS)			\
	$(CWARNFLAGS)				\
	$(NULL)

xlibi18n_la_SOURCES =				\
	lcGenConv.c				\
	$(NULL)

libxlibi18n_la_SOURCES = $(xlibi18n_la_SOURCES)
