NULL =

noinst_LTLIBRARIES = libxlcDef.la

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
	$(CWARNFLAG)				\
	$(NULL)

xlcDef_la_SOURCES =				\
	lcDefConv.c				\
	$(NULL)

libxlcDef_la_SOURCES = $(xlcDef_la_SOURCES)
