noinst_LTLIBRARIES = libdamage.la

AM_CFLAGS = $(DIX_CFLAGS) @SERVER_DEFINES@ @MODULE_DEFINES@ @LOADER_DEFINES@

AM_CPPFLAGS = -I$(srcdir)/../cw

if NXAGENT
DAMAGEC =
else
DAMAGEC = damage.c
endif

libdamage_la_SOURCES =	\
	$(DAMAGEC)	\
	damage.h	\
	damagestr.h
