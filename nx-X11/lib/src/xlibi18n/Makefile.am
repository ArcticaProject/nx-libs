NULL =

noinst_LTLIBRARIES = libi18n.la

AM_CPPFLAGS =							\
	-I$(top_srcdir)/include					\
	-I$(top_srcdir)/include/X11				\
	-I$(top_builddir)/include				\
	-I$(top_builddir)/include/X11				\
	-I$(top_srcdir)/src/xcms				\
	-I$(top_srcdir)/src/xkb					\
	-I$(top_srcdir)/src/xlibi18n				\
	-I$(top_srcdir)/src					\
	-I$(top_srcdir)/../exports/include			\
	-D_GNU_SOURCE						\
	$(NULL)

AM_CFLAGS =							\
	$(X11_CFLAGS)						\
	$(BIGFONT_CFLAGS)					\
	$(XMALLOC_ZERO_CFLAGS)					\
	$(CWARNFLAGS)						\
	$(NULL)

IM_LIBS =							\
	${top_builddir}/modules/im/ximcp/libximcp.la		\
	$(NULL)

LC_LIBS =							\
	${top_builddir}/modules/lc/def/libxlcDef.la		\
	${top_builddir}/modules/lc/gen/libxlibi18n.la		\
	${top_builddir}/modules/lc/Utf8/libxlcUTF8Load.la	\
	$(NULL)

OM_LIBS =							\
	${top_builddir}/modules/om/generic/libxomGeneric.la	\
	$(NULL)

libi18n_la_LIBADD =						\
	$(IM_LIBS)						\
	$(LC_LIBS)						\
	$(OM_LIBS)						\
	$(NULL)

libi18n_la_SOURCES =						\
	$(XI18N_DL_SOURCES)					\
	xim_trans.c						\
	ICWrap.c						\
	IMWrap.c						\
	imKStoUCS.c						\
	lcCT.c							\
	lcCharSet.c						\
	lcConv.c						\
	lcDB.c							\
	lcFile.c						\
	lcGeneric.c						\
	lcInit.c						\
	lcPrTxt.c						\
	lcPubWrap.c						\
	lcPublic.c						\
	lcRM.c							\
	lcStd.c							\
	lcTxtPr.c						\
	lcUTF8.c						\
	lcUtil.c						\
	lcWrap.c						\
	mbWMProps.c						\
	mbWrap.c						\
	utf8WMProps.c						\
	utf8Wrap.c						\
	wcWrap.c						\
	$(NULL)
