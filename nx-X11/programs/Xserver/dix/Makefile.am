noinst_LTLIBRARIES = libdix.la libxpstubs.la

AM_CFLAGS = $(DIX_CFLAGS) \
	-DVENDOR_STRING=\""@VENDOR_STRING@"\" \
	-DVENDOR_RELEASE="@VENDOR_RELEASE@"


if NXAGENT
SPECIAL =
else
SPECIAL = dispatch.c dixfonts.c events.c extension.c glyphcurs.c property.c resource.c window.c
endif

libdix_la_SOURCES = 	\
	atom.c		\
	colormap.c	\
	cursor.c	\
	devices.c	\
	dispatch.h	\
	dixutils.c	\
	ffs.c		\
	gc.c		\
	globals.c	\
	grabs.c		\
	initatoms.c	\
	main.c		\
	pixmap.c	\
	privates.c	\
	region.c	\
	swaprep.c	\
	swapreq.c	\
	tables.c

libxpstubs_la_SOURCES =	\
	xpstubs.c

AM_CPPFLAGS = -I$(top_srcdir)/Xprint

EXTRA_DIST = buildatoms BuiltInAtoms CHANGES
