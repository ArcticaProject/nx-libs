NULL =

# libXext.la:        includes all extensions and should be linked into Xvfb,
#                    Xnest, Xdmx and Xprt
# libXextbuiltin.la: includes those extensions that are built directly into
#                    Xorg by default
# libXextmodule.la:  includes those extensions that are built into a module
#                    that Xorg loads
noinst_LTLIBRARIES = libXext.la libXextbuiltin.la libXextmodule.la

AM_CPPFLAGS = -I$(top_srcdir)/hw/xfree86/dixmods/extmod -I$(top_srcdir)/hw/xfree86/os-support

AM_CFLAGS = $(DIX_CFLAGS) @SERVER_DEFINES@ @LOADER_DEFINES@

sdk_HEADERS = xvdix.h xvmcext.h

# Sources always included in libXextbuiltin.la & libXext.la
BUILTIN_SRCS =			\
        shape.c			\
        sleepuntil.c		\
        sleepuntil.h		\
        xtest.c			\
        xf86bigfont.c		\
        $(NULL)

# Sources always included in libXextmodule.la & libXext.la
MODULE_SRCS =			\
        bigreq.c		\
        shape.c			\
        sync.c			\
        xcmisc.c		\
        dpms.c			\
        dpmsproc.h		\
        $(NULL)

# Optional sources included if extension enabled by configure.ac rules

if NXAGENT

else
# MIT Shared Memory extension
MITSHM_SRCS = shm.c
if MITSHM
BUILTIN_SRCS += $(MITSHM_SRCS)
endif
endif

# XVideo extension
if NXAGENT
XV_SRCS = xvmain.c xvmc.c xvdix.h xvmcext.h xvdisp.h
else
XV_SRCS = xvmain.c xvdisp.c xvmc.c xvdix.h xvmcext.h xvdisp.h
endif
if XV
MODULE_SRCS  += $(XV_SRCS)
endif

# XResource extension: lets clients get data about per-client resource usage
RES_SRCS = xres.c
if RES
MODULE_SRCS  += $(RES_SRCS)
endif

# MIT ScreenSaver extension
SCREENSAVER_SRCS = saver.c
if SCREENSAVER
MODULE_SRCS  += $(SCREENSAVER_SRCS)
endif

# Xinerama extension: making multiple video devices act as one virtual screen
XINERAMA_SRCS = panoramiX.c panoramiX.h panoramiXh.h panoramiXsrv.h panoramiXprocs.c panoramiXSwap.c 
if XINERAMA
BUILTIN_SRCS += $(XINERAMA_SRCS)
endif

# Security extension: multi-level security to protect clients from each other
XCSECURITY_SRCS = security.c
if XCSECURITY   
BUILTIN_SRCS += $(XCSECURITY_SRCS)

SERVERCONFIGdir = $(libdir)/xserver
SERVERCONFIG_DATA = SecurityPolicy
AM_CFLAGS += -DDEFAULTPOLICYFILE=\"$(SERVERCONFIGdir)/SecurityPolicy\"
endif

# Now take all of the above, mix well, bake for 10 minutes and get libXext*.la

libXext_la_SOURCES =		$(BUILTIN_SRCS) $(MODULE_SRCS)

libXextbuiltin_la_SOURCES =	$(BUILTIN_SRCS)

libXextmodule_la_SOURCES =	$(MODULE_SRCS)
libXextmodule_la_CFLAGS = 	$(AM_CFLAGS) @MODULE_DEFINES@ -DEXTMODULE

EXTRA_DIST =			\
        SecurityPolicy		\
        $(MITSHM_SRCS)		\
        $(XV_SRCS)		\
        $(RES_SRCS)		\
        $(SCREENSAVER_SRCS)	\
        $(XCSECURITY_SRCS)	\
        $(XINERAMA_SRCS)	\
        $(NULL)
