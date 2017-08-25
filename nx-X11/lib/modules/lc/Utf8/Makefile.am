NULL =

noinst_LTLIBRARIES = libxlcUTF8Load.la

AM_CPPFLAGS =					\
	-I$(top_srcdir)/include			\
	-I$(top_srcdir)/include/X11		\
	-I$(top_builddir)/include		\
	-I$(top_builddir)/include/X11 		\
	-I$(top_srcdir)/src/xcms 		\
	-I$(top_srcdir)/src/xkb			\
	-I$(top_srcdir)/src/xlibi18n		\
	-I$(top_srcdir)/../exports/include	\
	-D_GNU_SOURCE				\
	$(NULL)

AM_CFLAGS =					\
	$(X11_CFLAGS)				\
	$(BIGFONT_CFLAGS)			\
	$(MALLOC_ZERO_CFLAGS)			\
	$(CWARNFLAGS)				\
	$(NULL)

xlcUTF8Load_la_SOURCES =			\
	lcUTF8Load.c				\
	$(NULL)

libxlcUTF8Load_la_SOURCES = $(xlcUTF8Load_la_SOURCES)
