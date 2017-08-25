NULL =

noinst_LTLIBRARIES = libxkb.la

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
	$(XMALLOC_ZERO_CFLAGS)			\
	$(CWARNFLAGS)				\
	$(NULL)

libxkb_la_SOURCES =				\
	XKB.c					\
	XKBBind.c				\
	XKBCompat.c				\
	XKBCtrls.c				\
	XKBCvt.c				\
	XKBGetMap.c				\
	XKBGetByName.c				\
	XKBNames.c				\
	XKBRdBuf.c				\
	XKBSetMap.c				\
	XKBUse.c				\
	XKBleds.c				\
	XKBBell.c				\
	XKBGeom.c				\
	XKBSetGeom.c				\
	XKBExtDev.c				\
	XKBList.c				\
	XKBMisc.c				\
	XKBMAlloc.c				\
	XKBGAlloc.c				\
	XKBAlloc.c				\
	$(NULL)
