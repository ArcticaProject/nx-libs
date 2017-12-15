NULL =

AM_CPPFLAGS =					\
	-I$(top_srcdir)/include			\
	-I$(top_srcdir)/include/X11		\
	-I$(top_builddir)/include		\
	-I$(top_builddir)/include/X11		\
	-I$(top_srcdir)/src/xcms		\
	-I$(top_srcdir)/src/xkb			\
	-I$(top_srcdir)/src/xlibi18n		\
	-I$(top_srcdir)/src			\
	-I$(top_srcdir)/../exports/include	\
	-D_GNU_SOURCE				\
	$(NULL)

AM_CFLAGS =					\
	$(X11_CFLAGS)				\
	$(BIGFONT_CFLAGS)			\
	$(MALLOC_ZERO_CFLAGS)			\
	$(CWARNFLAGS)				\
	$(NULL)

noinst_LTLIBRARIES = libxomGeneric.la

xomGeneric_la_SOURCES =				\
	omDefault.c				\
	omGeneric.c				\
	omImText.c				\
	omText.c				\
	omTextEsc.c				\
	omTextExt.c				\
	omTextPer.c				\
	omXChar.c				\
	$(NULL)

libxomGeneric_la_SOURCES = $(xomGeneric_la_SOURCES)
