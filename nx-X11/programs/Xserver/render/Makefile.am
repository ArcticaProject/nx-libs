noinst_LTLIBRARIES = librender.la

AM_CFLAGS = $(DIX_CFLAGS)

if NXAGENT
SPECIAL =
else
SPECIAL = picture.c render.c mitrap.c
endif


librender_la_SOURCES =	\
	animcur.c	\
	filter.c	\
	glyph.c		\
	matrix.c	\
	miindex.c	\
	mipict.c	\
	mirect.c	\
	mitri.c		\
	$(SPECIAL)	\
	renderedge.c

sdk_HEADERS = picture.h mipict.h glyphstr.h picturestr.h renderedge.h
