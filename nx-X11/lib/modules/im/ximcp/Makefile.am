NULL =

noinst_LTLIBRARIES=libximcp.la

AM_CPPFLAGS =					\
	-I$(top_srcdir)/include			\
	-I$(top_srcdir)/include/X11		\
	-I$(top_builddir)/include 		\
	-I$(top_builddir)/include/X11		\
	-I$(top_srcdir)/src/xcms		\
	-I$(top_srcdir)/src/xkb			\
	-I$(top_srcdir)/src/xlibi18n		\
	-I$(top_srcdir)/src			\
	-I$(top_srcdir)/../exports/include	\
	-D_GNU_SOURCE				\
	-DXIM_t					\
	-DTRANS_CLIENT				\
	$(NULL)

AM_CFLAGS =					\
	$(X11_CFLAGS)				\
	$(BIGFONT_CFLAGS)			\
	$(MALLOC_ZERO_CFLAGS)			\
	$(CWARNFLAGS)				\
	$(NULL)

ximcp_la_SOURCES =				\
		imCallbk.c			\
		imDefFlt.c			\
		imDefIc.c			\
		imDefIm.c			\
		imDefLkup.c			\
		imDispch.c			\
		imEvToWire.c			\
		imExten.c			\
		imImSw.c			\
		imInsClbk.c			\
		imInt.c				\
		imLcFlt.c			\
		imLcGIc.c			\
		imLcIc.c			\
		imLcIm.c			\
		imLcLkup.c			\
		imLcPrs.c			\
		imLcSIc.c			\
		imRmAttr.c			\
		imRm.c				\
		imThaiFlt.c			\
		imThaiIc.c			\
		imThaiIm.c			\
		imTrans.c			\
		imTransR.c			\
		imTrX.c				\
		$(NULL)

libximcp_la_SOURCES = $(ximcp_la_SOURCES)
