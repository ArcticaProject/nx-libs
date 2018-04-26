noinst_LTLIBRARIES = librandr.la

AM_CFLAGS = $(DIX_CFLAGS) @SERVER_DEFINES@ @LOADER_DEFINES@ -DNXAGENT_SERVER

if XORG
sdk_HEADERS = randrstr.h
endif

librandr_la_SOURCES =	\
	mirandr.c	\
	randr.c		\
	randrstr.h	\
	rrcrtc.c	\
	rrdispatch.c	\
	rrinfo.c	\
	rrmode.c	\
	rroutput.c	\
	rrpointer.c	\
	rrproperty.c	\
	rrscreen.c	\
	rrsdispatch.c   \
        rrxinerama.c

