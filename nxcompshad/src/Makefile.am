NULL =

lib_LTLIBRARIES = libXcompshad.la

libXcompshad_la_SOURCES =				\
    Core.cpp						\
    Input.cpp						\
    Logger.cpp						\
    Manager.cpp						\
    Shadow.cpp						\
    Updater.cpp						\
    Win.cpp						\
    X11.cpp						\
    $(NULL)

libXcompshad_la_LIBADD =				\
    @Xext_LIBS@						\
    @Xdamage_LIBS@					\
    @Xrandr_LIBS@					\
    @Xtst_LIBS@						\
    -L$(top_srcdir)/../nx-X11/exports/lib -lNX_X11	\
    $(NULL)

AM_CFLAGS =						\
    $(Xext_CFLAGS)					\
    $(Xdamage_CFLAGS)					\
    $(Xrandr_CFLAGS)					\
    $(Xtst_CFLAGS)					\
    $(NULL)

AM_CXXFLAGS =						\
    $(BASE_CXXFLAGS)					\
    $(NULL)

AM_CPPFLAGS =						\
    -I$(top_srcdir)/include				\
    -I$(top_srcdir)/../nx-X11/exports/include		\
    $(NULL)

libXcompshad_la_LDFLAGS = -version-number @LT_COMPSHAD_VERSION@ -no-undefined

libXcompshadincludedir = $(includedir)/nx
libXcompshadinclude_HEADERS = $(top_srcdir)/include/Shadow.h
