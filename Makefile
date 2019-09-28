#!/usr/bin/make -f

NULL =

# helpers for "install" target
INSTALL_DIR=install -d -m 755
INSTALL_FILE=install -m 644
INSTALL_PROGRAM=install -m 755
INSTALL_SYMLINK=ln -s -f

# helpers for "build" target
SYMLINK_FILE=ln -f -s

# helpers for "clean" and "uninstall" targets
RM_FILE=rm -f
RM_DIR=rmdir -p --ignore-fail-on-non-empty
RM_DIR_REC=rm -Rf

ETCDIR_NX       ?= /etc/nxagent
PREFIX          ?= /usr/local
BINDIR          ?= $(PREFIX)/bin
LIBDIR          ?= $(PREFIX)/lib
SHLIBDIR        ?= $(LIBDIR)
NXLIBDIR        ?= $(SHLIBDIR)/nx
USRLIBDIR       ?= $(NXLIBDIR)/X11
INCLUDEDIR      ?= $(PREFIX)/include
CONFIGURE       ?= ./configure --prefix="$(PREFIX)"

IMAKE_DEFINES	?=

SHELL:=/bin/bash

NX_X11_HEADERS =		\
        Xlib.h			\
        Xresource.h		\
        Xutil.h			\
        cursorfont.h		\
        Xlibint.h		\
        Xcms.h			\
        Xlocale.h		\
        XKBlib.h		\
        XlibConf.h		\
        Xregion.h		\
        ImUtil.h		\
        $(NULL)

NX_XTRANS_HEADERS =		\
        transport.c		\
        Xtrans.c		\
        Xtrans.h		\
        Xtransint.h		\
        Xtranslcl.c		\
        Xtranssock.c		\
        Xtransutil.c		\
        $(NULL)

NX_X11PROTO_HEADERS =		\
        DECkeysym.h		\
        HPkeysym.h		\
        keysymdef.h		\
        keysym.h		\
        Sunkeysym.h		\
        Xalloca.h		\
        Xarch.h			\
        Xatom.h			\
        Xauth.h			\
        Xdefs.h			\
        XF86keysym.h		\
        Xfuncproto.h		\
        Xfuncs.h		\
        X.h			\
        Xmd.h			\
        Xosdefs.h		\
        Xos.h			\
        Xos_r.h			\
        Xpoll.h			\
        Xproto.h		\
        Xprotostr.h		\
        Xthreads.h		\
        Xw32defs.h		\
        XWDFile.h		\
        Xwindows.h		\
        Xwinsock.h		\
        $(NULL)

NX_X11PROTO_EXTENSION_HEADERS =	\
        bigreqstr.h		\
        composite.h		\
        compositeproto.h	\
        damageproto.h		\
        damagewire.h		\
        dpms.h			\
        dpmsstr.h		\
        panoramiXext.h		\
        panoramiXproto.h	\
        randr.h			\
        randrproto.h		\
        record.h		\
        recordstr.h		\
        render.h		\
        renderproto.h		\
        saver.h			\
        saverproto.h		\
        scrnsaver.h		\
        security.h		\
        securstr.h		\
        shapeconst.h		\
        sync.h			\
        syncstr.h		\
        xcmiscstr.h		\
        Xdbeproto.h		\
        xf86bigfont.h		\
        xf86bigfproto.h		\
        xfixesproto.h		\
        xfixeswire.h		\
        XI.h			\
        XIproto.h		\
        XKBconfig.h		\
        XKBfile.h		\
        XKBgeom.h		\
        XKB.h			\
        XKBproto.h		\
        XKBrules.h		\
        XKBsrv.h		\
        XKBstr.h		\
        XKMformat.h		\
        XKM.h			\
        XResproto.h		\
        xtestconst.h		\
        xtestext1.h		\
        xteststr.h		\
        Xv.h			\
        XvMC.h			\
        XvMCproto.h		\
        Xvproto.h		\
        $(NULL)

NX_MESAGL_HEADERS =		\
        glext.h			\
        gl.h			\
        glxext.h		\
        osmesa.h		\
        $(NULL)

NX_GLX_HEADERS =		\
        glx.h			\
        glxint.h		\
        glxmd.h			\
        glxproto.h		\
        glxtokens.h		\
        $(NULL)

NX_COMP_HEADERS =		\
        MD5.h			\
        NXalert.h		\
        NX.h			\
        NXpack.h		\
        NXproto.h		\
        NXvars.h		\
        $(NULL)

NX_COMPSHAD_HEADERS =		\
        Shadow.h		\
        $(NULL)

all: build

clean: version
	test -f nxcomp/Makefile     && ${MAKE} -C nxcomp clean         || true
	test -f nxproxy/Makefile    && ${MAKE} -C nxproxy clean        || true
	test -f nx-X11/lib/Makefile && ${MAKE} -C nx-X11/lib clean     || true
	test -f nxcompshad/Makefile && ${MAKE} -C nxcompshad clean     || true
	test -f nx-X11/programs/Xserver/Makefile && ${MAKE} -C nx-X11/programs/Xserver clean || true
	test -d nx-X11              && ${MAKE} clean-env               || true
	test -f nxdialog/Makefile   && ${MAKE} -C nxdialog clean       || true

distclean: clean version
	test -f nxcomp/Makefile     && ${MAKE} -C nxcomp distclean     || true
	test -f nxproxy/Makefile    && ${MAKE} -C nxproxy distclean    || true
	test -f nx-X11/lib/Makefile && ${MAKE} -C nx-X11/lib distclean || true
	test -f nxcompshad/Makefile && ${MAKE} -C nxcompshad distclean || true
	test -f nx-X11/programs/Xserver/Makefile && ${MAKE} -C nx-X11/programs/Xserver distclean || true
	test -d nx-X11              && ${MAKE} -C nx-X11 distclean     || true
	test -f nxdialog/Makefile   && ${MAKE} -C nxdialog distclean   || true
	test -x ./mesa-quilt        && ./mesa-quilt pop -a
	$(RM_DIR_REC) nx-X11/extras/Mesa/.pc/
	$(RM_FILE) nx-X11/config/cf/nxversion.def

test:
	echo "No testing for NX (redistributed)"

version:
	# prepare nx-X11/config/cf/nxversion.def
	sed \
	    -e 's/###NX_VERSION_MAJOR###/$(shell ./version.sh 1)/' \
	    -e 's/###NX_VERSION_MINOR###/$(shell ./version.sh 2)/' \
	    -e 's/###NX_VERSION_MICRO###/$(shell ./version.sh 3)/' \
	    -e 's/###NX_VERSION_PATCH###/$(shell ./version.sh 4)/' \
	    nx-X11/config/cf/nxversion.def.in \
	    > nx-X11/config/cf/nxversion.def

build-env: version
	# set up environment for libNX_X11 build (libNX_X11 header files)
	mkdir -p nx-X11/exports/include/nx-X11/
	for header in $(NX_X11_HEADERS); do \
	    ${SYMLINK_FILE} ../../../lib/include/X11/$${header} nx-X11/exports/include/nx-X11/$${header}; \
	done

	# set up environment for libNX_X11 build (NX_Xtrans header/include files)
	mkdir -p nx-X11/exports/include/nx-X11/Xtrans/
	for header in $(NX_XTRANS_HEADERS); do \
	    ${SYMLINK_FILE} ../../../../lib/include/xtrans/$${header} nx-X11/exports/include/nx-X11/Xtrans/$${header}; \
	done

	# setup environment for libNX_X11 and Xserver build (nx-X11 proto headers)
	mkdir -p nx-X11/exports/include/nx-X11/
	for header in $(NX_X11PROTO_HEADERS); do \
	    ${SYMLINK_FILE} ../../../include/$${header} nx-X11/exports/include/nx-X11/$${header}; \
	done

	# setup environment for Xserver build (nx-X11 extension proto headers)
	mkdir -p nx-X11/exports/include/nx-X11/extensions
	for header in $(NX_X11PROTO_EXTENSION_HEADERS); do \
	    ${SYMLINK_FILE} ../../../../include/extensions/$${header} nx-X11/exports/include/nx-X11/extensions/$${header}; \
	done

	# setup environment for Xserver GLX/Mesa build
	for header in $(NX_MESAGL_HEADERS); do \
	    ${SYMLINK_FILE} ../../extras/Mesa/include/GL/$${header} nx-X11/include/GL/$${header}; \
	done
	mkdir -p nx-X11/exports/include/GL
	for header in $(NX_GLX_HEADERS) $(NX_MESAGL_HEADERS); do \
	    ${SYMLINK_FILE} ../../../include/GL/$${header} nx-X11/exports/include/GL/$${header}; \
	done

	# setup environment for building against nxcomp
	mkdir -p nx-X11/exports/include/nx
	for header in $(NX_COMP_HEADERS); do \
	    ${SYMLINK_FILE} ../../../../nxcomp/include/$${header} nx-X11/exports/include/nx/$${header}; \
	done

	# setup environment for building against nxcompshad
	mkdir -p nx-X11/exports/include/nx
	for header in $(NX_COMPSHAD_HEADERS); do \
	    ${SYMLINK_FILE} ../../../../nxcompshad/include/$${header} nx-X11/exports/include/nx/$${header}; \
	done

clean-env: version
	for header in $(NX_X11_HEADERS); do \
	    ${RM_FILE} nx-X11/exports/include/nx-X11/$${header}; \
	done
	for header in $(NX_XTRANS_HEADERS); do \
	    ${RM_FILE} nx-X11/exports/include/nx-X11/Xtrans/$${header}; \
	done
	for header in $(NX_X11PROTO_HEADERS); do \
	    ${RM_FILE} nx-X11/exports/include/nx-X11/$${header}; \
	done
	for header in $(NX_X11PROTO_EXTENSION_HEADERS); do \
	    ${RM_FILE} nx-X11/exports/include/nx-X11/extensions/$${header}; \
	done
	for header in $(NX_GLX_HEADERS) $(NX_MESAGL_HEADERS); do \
	    ${RM_FILE} nx-X11/exports/include/GL/$${header}; \
	done
	for header in $(NX_MESAGL_HEADERS); do \
	    ${RM_FILE} nx-X11/include/GL/$${header}; \
	done
	for header in $(NX_COMP_HEADERS) $(NX_COMPSHAD_HEADERS); do \
	    ${RM_FILE} nx-X11/exports/include/nx/$${header}; \
	done

	[ -d nx-X11/exports/include/nx-X11/Xtrans     ] && $(RM_DIR) nx-X11/exports/include/nx-X11/Xtrans/     || :
	[ -d nx-X11/exports/include/nx-X11/extensions ] && $(RM_DIR) nx-X11/exports/include/nx-X11/extensions/ || :
	[ -d nx-X11/exports/include/nx-X11/ ]           && $(RM_DIR) nx-X11/exports/include/nx-X11/            || :
	[ -d nx-X11/exports/include/nx/ ]               && $(RM_DIR) nx-X11/exports/include/nx/                || :
	[ -d nx-X11/exports/include/GL/ ]               && $(RM_DIR) nx-X11/exports/include/GL/                || :
	[ -d nx-X11/exports/include/ ]                  && $(RM_DIR) nx-X11/exports/include/                   || :
	[ -d nx-X11/exports/ ]                          && $(RM_DIR) nx-X11/exports/                           || :

build-lite:
	cd nxcomp  && autoreconf -vfsi && (${CONFIGURE}) && ${MAKE}
	cd nxproxy && autoreconf -vfsi && (${CONFIGURE}) && ${MAKE}

build-full: build-env
	# build nxcomp first
	cd nxcomp && autoreconf -vfsi && (${CONFIGURE}) && ${MAKE}

	# build libNX_X11 second
	cd nx-X11/lib && autoreconf -vfsi && (${CONFIGURE} --disable-poll) && ${MAKE}
	mkdir -p nx-X11/exports/lib/
	$(SYMLINK_FILE) ../../lib/src/.libs/libNX_X11.so nx-X11/exports/lib/libNX_X11.so
	$(SYMLINK_FILE) ../../lib/src/.libs/libNX_X11.so.6 nx-X11/exports/lib/libNX_X11.so.6
	$(SYMLINK_FILE) ../../lib/src/.libs/libNX_X11.so.6.3.0 nx-X11/exports/lib/libNX_X11.so.6.3.0

	# build nxcompshad third
	cd nxcompshad && autoreconf -vfsi && (${CONFIGURE}) && ${MAKE}

	# build nxagent fourth
	./mesa-quilt push -a
	cd nx-X11/programs/ && CONFIGURE="${CONFIGURE}" ./buildit.sh

	# build nxproxy fifth
	cd nxproxy && autoreconf -vfsi && (${CONFIGURE}) && ${MAKE}

	# "build" nxdialog last
	cd nxdialog && autoreconf -vfsi && (${CONFIGURE}) && ${MAKE}

build:
	if ! test -d nx-X11; then \
	    ${MAKE} build-lite; \
	else \
	    ${MAKE} build-full; \
	fi

install:
	$(MAKE) install-lite
	[ ! -d nx-X11 ] || $(MAKE) install-full

install-lite:
	# install nxcomp library
	$(MAKE) -C nxcomp install

	# install the nxproxy executable and its man page
	$(MAKE) -C nxproxy install

install-full:
	$(MAKE) -C nxcompshad install

	$(INSTALL_DIR) $(DESTDIR)$(BINDIR)/
	$(INSTALL_PROGRAM) nx-X11/programs/Xserver/nxagent-relink $(DESTDIR)$(BINDIR)/nxagent

	$(INSTALL_DIR) $(DESTDIR)$(PREFIX)/share/pixmaps
	$(INSTALL_FILE) nx-X11/programs/Xserver/hw/nxagent/nxagent.xpm $(DESTDIR)$(PREFIX)/share/pixmaps

	$(INSTALL_DIR) $(DESTDIR)$(PREFIX)/share/nx
	$(INSTALL_FILE) nx-X11/programs/Xserver/Xext/SecurityPolicy $(DESTDIR)$(PREFIX)/share/nx

	# FIXME: Drop this symlink for 3.6.0. Requires that third party frameworks like X2Go have become aware of this...
	$(INSTALL_DIR) $(DESTDIR)$(NXLIBDIR)/bin
	$(INSTALL_SYMLINK) $(BINDIR)/nxagent $(DESTDIR)$(NXLIBDIR)/bin/nxagent

	$(INSTALL_DIR) $(DESTDIR)$(PREFIX)/share/man/man1/
	$(INSTALL_FILE) nx-X11/programs/Xserver/hw/nxagent/man/nxagent.1 $(DESTDIR)$(PREFIX)/share/man/man1/
	gzip $(DESTDIR)$(PREFIX)/share/man/man1/*.1

	# create a clean nx-X11/.build-exports space
	$(RM_DIR_REC) nx-X11/.build-exports
	mkdir -p nx-X11/.build-exports/include
	mkdir -p nx-X11/.build-exports/lib

	# copy headers (for libnx-x11-dev)
	cp -aL nx-X11/exports/include/* nx-X11/.build-exports/include

	# copy libs (for libnx-x11), we want the targets of the links
	. replace.sh; set -x; find nx-X11/exports/lib/ -name "libNX*.so" | while read libpath; do \
	    libfile=$$(basename $$libpath); \
	    libdir=$$(dirname $$libpath); \
	    link=$$(readlink $$libpath); \
	\
	    mkdir -p "$$(string_rep "$$libdir" exports .build-exports)"; \
	    cp -a "$$(string_rep "$$libpath" "$$libfile" "$$link")" "$$(string_rep "$$libdir" exports .build-exports)"; \
	done;

	$(INSTALL_DIR) $(DESTDIR)$(SHLIBDIR)
	$(INSTALL_DIR) $(DESTDIR)$(USRLIBDIR)
	$(INSTALL_SYMLINK) ../../libNX_X11.so.6 $(DESTDIR)$(USRLIBDIR)/libX11.so.6
	$(INSTALL_SYMLINK) ../../libNX_X11.so.6.3.0 $(DESTDIR)$(USRLIBDIR)/libX11.so.6.3.0

	. replace.sh; set -x; find nx-X11/.build-exports/include/{nx*,GL} -type d | \
	    while read dirname; do \
	        $(INSTALL_DIR) "$$(string_rep "$$dirname" nx-X11/.build-exports/include "$(DESTDIR)$(INCLUDEDIR)/")"; \
	        $(INSTALL_FILE) $${dirname}/*.h \
	                        "$$(string_rep "$$dirname" nx-X11/.build-exports/include "$(DESTDIR)$(INCLUDEDIR)/")"/ || true; \
	    done; \

	$(INSTALL_DIR) $(DESTDIR)/$(ETCDIR_NX)
	$(INSTALL_FILE) etc/keystrokes.cfg $(DESTDIR)/$(ETCDIR_NX)/

	$(INSTALL_DIR) $(DESTDIR)$(PREFIX)/share/nx
	$(INSTALL_FILE) VERSION $(DESTDIR)$(PREFIX)/share/nx/VERSION.nxagent
	$(INSTALL_FILE) VERSION $(DESTDIR)$(PREFIX)/share/nx/VERSION.nxproxy

	$(MAKE) -C nx-X11/lib install

	# install the nxdialog executable and its man page
	$(MAKE) -C nxdialog install

uninstall:
	$(MAKE) uninstall-lite
	[ ! -d nx-X11 ] || $(MAKE) uninstall-full

uninstall-lite:
	test -f nxcomp/Makefile  && ${MAKE} -C nxcomp "$@"
	test -f nxproxy/Makefile && ${MAKE} -C nxproxy "$@"

	$(RM_FILE) $(DESTDIR)$(PREFIX)/share/nx/VERSION.nxproxy
	$(RM_DIR) $(DESTDIR)$(PREFIX)/share/nx/

uninstall-full:
	test -f nxcompshad/Makefile && ${MAKE} -C nxcompshad "$@"
	test -f nx-X11/lib/Makefile && ${MAKE} -C nx-X11/lib "$@"

	$(RM_FILE) $(DESTDIR)$(BINDIR)/nxagent

	$(RM_FILE) $(DESTDIR)$(PREFIX)/share/nx/VERSION.nxagent
	$(RM_DIR) $(DESTDIR)$(PREFIX)/share/nx/

	$(RM_DIR_REC) $(DESTDIR)$(NXLIBDIR)
	$(RM_DIR_REC) $(DESTDIR)$(INCLUDEDIR)/nx

	test -f nxdialog/Makefile && ${MAKE} -C nxdialog "$@"
