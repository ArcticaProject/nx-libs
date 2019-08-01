noinst_LTLIBRARIES = librandr.la

AM_CFLAGS = $(DIX_CFLAGS) -DNXAGENT_SERVER

if XORG
sdk_HEADERS = randrstr.h
endif

librandr_la_SOURCES =	\
	randr.c		\
	randrstr.h	\
	rrcrtc.c	\
	rrdispatch.c	\
	rrinfo.c	\
	rrmode.c	\
	rrmonitor.c	\
	rroutput.c	\
	rrpointer.c	\
	rrproperty.c	\
	rrprovider.c	\
	rrproviderproperty.c	\
	rrscreen.c	\
	rrtransform.c	\
	rrsdispatch.c	\
	rrxinerama.c

