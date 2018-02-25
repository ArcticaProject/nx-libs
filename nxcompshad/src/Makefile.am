NULL =

lib_LTLIBRARIES = libXcompshad.la

libXcompshad_la_SOURCES =				\
    Core.cpp						\
    Input.cpp						\
    Logger.cpp						\
    Manager.cpp						\
    Shadow.cpp						\
    Updater.cpp						\
    X11.cpp						\
    $(NULL)

libXcompshad_la_LIBADD =				\
    -lX11						\
    @Xext_LIBS@						\
    @Xdamage_LIBS@					\
    @Xrandr_LIBS@					\
    @Xtst_LIBS@						\
    -L$(top_srcdir)/../nx-X11/lib/src/.libs -lNX_X11	\
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

libXcompshad_la_LDFLAGS =				\
    -version-number @LT_COMPSHAD_VERSION@ -no-undefined	\
    -R '$(libdir)/nx/X11'				\
    $(NULL)

if TARGET_ELF
libXcompshad_la_LDFLAGS +=				\
    -Wl,--enable-new-dtags				\
    $(NULL)
endif TARGET_ELF

libXcompshadincludedir = $(includedir)/nx
libXcompshadinclude_HEADERS =				\
     $(top_srcdir)/include/Shadow.h			\
     $(NULL)
