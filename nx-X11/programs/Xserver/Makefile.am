AUTOMAKE_OPTIONS=dist-bzip2 foreign nostdinc

NULL =

if COMPOSITE
COMPOSITE_DIR=composite
endif

if GLX
GLX_DIR=GL
endif

SUBDIRS =			\
	include			\
	dix 			\
	fb			\
	mi			\
	Xext			\
	miext			\
	os			\
	randr			\
	render 			\
	Xi			\
	xkb			\
	dbe			\
	record			\
	xfixes			\
	damageext		\
	$(COMPOSITE_DIR)	\
	$(GLX_DIR)		\
	hw			\
	$(NULL)

aclocaldir = $(datadir)/aclocal
aclocal_DATA = xorg-server.m4

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = xorg-server.pc

ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = xorg-server.pc.in xorg-server.m4

DIST_SUBDIRS =			\
	include			\
	dix 			\
	fb			\
	mi			\
	Xext			\
	miext			\
	os			\
	randr			\
	render 			\
	Xi			\
	xkb			\
	dbe			\
	record			\
	xfixes			\
	damageext		\
	composite		\
	GL			\
	hw			\
	$(NULL)
