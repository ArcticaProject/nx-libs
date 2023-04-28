#!/usr/bin/make -f

NULL =

# suppress most make output unless "make VERBOSE=1" has been called
ifndef VERBOSE
.SILENT:
endif

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

# check if the xkbcomp devel pkg is available - we need it for the next step
ifneq ($(shell pkg-config --exists xkbcomp && echo yes), yes)
    $(warning xkbcomp devel package missing, using imake default values)
endif
ifneq ($(shell pkg-config --exists fontutil && echo yes), yes)
    $(warning fontutil devel package missing, using imake default values)
endif

ifneq "$(strip $(NX_VERSION_CUSTOM))" ""
    CUSTOM_VERSION_DEFINE = NX_VERSION_CUSTOM="$(NX_VERSION_CUSTOM)"
endif

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

all: build

clean: version imakeconfig
	@echo
	@echo "===> $@"
	test -f nxcomp/Makefile     && ${MAKE} -C nxcomp clean         || true
	test -f nxproxy/Makefile    && ${MAKE} -C nxproxy clean        || true
	test -f nx-X11/lib/Makefile && ${MAKE} -C nx-X11/lib clean     || true
	test -f nxcompshad/Makefile && ${MAKE} -C nxcompshad clean     || true
	test -d nx-X11              && ${MAKE} clean-env               || true
	test -f nxdialog/Makefile   && ${MAKE} -C nxdialog clean       || true
	@echo "<=== $@"

distclean: clean version imakeconfig
	@echo
	@echo "===> $@"
	test -f nxcomp/Makefile     && ${MAKE} -C nxcomp distclean     || true
	test -f nxproxy/Makefile    && ${MAKE} -C nxproxy distclean    || true
	test -f nx-X11/lib/Makefile && ${MAKE} -C nx-X11/lib distclean || true
	test -f nxcompshad/Makefile && ${MAKE} -C nxcompshad distclean || true
	test -d nx-X11              && ${MAKE} -C nx-X11 distclean     || true
	test -f nxdialog/Makefile   && ${MAKE} -C nxdialog distclean   || true
	test -x ./mesa-quilt        && ./mesa-quilt pop -a
	$(RM_DIR_REC) nx-X11/extras/Mesa/.pc/
	$(RM_FILE) nx-X11/config/cf/nxversion.def
	$(RM_FILE) nx-X11/config/cf/nxconfig.def
	@echo "<=== $@"

test:
	@echo "No testing for NX (redistributed)"

version:
	# prepare nx-X11/config/cf/nxversion.def
	sed \
	    -e 's/###NX_VERSION_MAJOR###/$(shell ./version.sh 1)/' \
	    -e 's/###NX_VERSION_MINOR###/$(shell ./version.sh 2)/' \
	    -e 's/###NX_VERSION_MICRO###/$(shell ./version.sh 3)/' \
	    -e 's/###NX_VERSION_PATCH###/$(shell ./version.sh 4)/' \
	    nx-X11/config/cf/nxversion.def.in \
	    > nx-X11/config/cf/nxversion.def
ifneq "$(strip $(NX_VERSION_CUSTOM))" ""
	echo '#define NX_VERSION_CUSTOM $(NX_VERSION_CUSTOM)' >>nx-X11/config/cf/nxversion.def
endif

imakeconfig:
	@echo;
	@echo "===> $@"
	# auto-config some setting

	# check if system supports Xfont2
	(echo "#define HasXfont2 `pkg-config --exists xfont2 && echo YES || echo NO`") >nx-X11/config/cf/nxconfig.def

	# check if system has an _old_ release of Xfont1
	(echo "#define HasLegacyXfont1 `pkg-config --exists 'xfont < 1.4.2' && echo YES || echo NO`") >>nx-X11/config/cf/nxconfig.def

	# check if system has an _old_ release of XextProto
	(echo "#define HasLegacyXextProto `pkg-config --exists 'xextproto < 7.1.0' && echo YES || echo NO`") >>nx-X11/config/cf/nxconfig.def

	# the system's directory with the xkb data and binary files (these
	# needs to be independent of Imake's ProjectRoot or the configure
	# prefix.)
	(pkg-config --exists xkbcomp && echo "#define SystemXkbConfigDir `pkg-config xkbcomp --variable=xkbconfigdir`"; :) >>nx-X11/config/cf/nxconfig.def
	(pkg-config --exists xkbcomp && echo "#define SystemXkbBinDir `pkg-config xkbcomp --variable=prefix`/bin"; :) >>nx-X11/config/cf/nxconfig.def

	(pkg-config --exists fontutil && echo "#define SystemFontRootDir `pkg-config fontutil --variable=fontrootdir`"; :) >>nx-X11/config/cf/nxconfig.def

	# let's create the nx-X11 Makefiles now, once everything has been defined
	$(MAKE) -j1 -C nx-X11 Makefiles IMAKE_DEFINES="$(IMAKE_DEFINES)"
	@echo "<=== $@"

build-env: version imakeconfig
	@echo
	@echo "===> $@"
	# prepare Makefiles and the nx-X11 symlinking magic
	${MAKE} -j1 -C nx-X11 BuildIncludes IMAKE_DEFINES="$(IMAKE_DEFINES)"

	# set up environment for libNX_X11 build (X11 header files)
	mkdir -p nx-X11/exports/include/nx-X11/
	for header in $(NX_X11_HEADERS); do \
	    ${SYMLINK_FILE} ../../../lib/include/X11/$${header} nx-X11/exports/include/nx-X11/$${header}; \
	done

	# set up environment for libNX_X11 build (Xtrans header/include files)
	mkdir -p nx-X11/exports/include/nx-X11/Xtrans/
	for header in $(NX_XTRANS_HEADERS); do \
	    ${SYMLINK_FILE} ../../../../lib/include/xtrans/$${header} nx-X11/exports/include/nx-X11/Xtrans/$${header}; \
	done
	@echo "<=== $@"

clean-env: version
	@echo
	@echo "===> $@"
	for header in $(NX_X11_HEADERS); do \
	    ${RM_FILE} nx-X11/exports/include/nx-X11/$${header}; \
	done
	for header in $(NX_XTRANS_HEADERS); do \
	    ${RM_FILE} nx-X11/exports/include/nx-X11/Xtrans/$${header}; \
	done

	[ -d exports/include/nx-X11/Xtrans ] && $(RM_DIR) exports/include/nx-X11/Xtrans/ || :
	[ -d exports/include/nx-X11/ ]       && $(RM_DIR) exports/include/nx-X11/        || :

	${MAKE} -j1 -C nx-X11 clean IMAKE_DEFINES="$(IMAKE_DEFINES)"
	@echo "<=== $@"

build-lite:
	@echo
	@echo "===> $@"
	cd nxcomp  && autoreconf -vfsi && (${CONFIGURE}) && ${MAKE}
	cd nxproxy && autoreconf -vfsi && (${CONFIGURE}) && ${MAKE}
	@echo "<=== $@"

build-full: build-env
# in the full case, we rely on "magic" in the nx-X11 imake-based makefiles...

	@echo
	@echo "===> $@"

	# build nxcomp first
	cd nxcomp && autoreconf -vfsi && (${CONFIGURE} $(CUSTOM_VERSION_DEFINE)) && ${MAKE}

	# build libNX_X11 second
	cd nx-X11/lib && autoreconf -vfsi && (${CONFIGURE} $(CUSTOM_VERSION_DEFINE) --disable-poll) && ${MAKE}
	mkdir -p nx-X11/exports/lib/
	$(SYMLINK_FILE) ../../lib/src/.libs/libNX_X11.so nx-X11/exports/lib/libNX_X11.so
	$(SYMLINK_FILE) ../../lib/src/.libs/libNX_X11.so.6 nx-X11/exports/lib/libNX_X11.so.6
	$(SYMLINK_FILE) ../../lib/src/.libs/libNX_X11.so.6.3.0 nx-X11/exports/lib/libNX_X11.so.6.3.0

	# build nxcompshad third
	cd nxcompshad && autoreconf -vfsi && (${CONFIGURE} $(CUSTOM_VERSION_DEFINE)) && ${MAKE}

	# build nxagent fourth
	./mesa-quilt push -a
	${MAKE} -j1 -C nx-X11 BuildDependsOnly IMAKE_DEFINES="$(IMAKE_DEFINES)"
	${MAKE} -C nx-X11 World USRLIBDIR="$(USRLIBDIR)" SHLIBDIR="$(SHLIBDIR)" IMAKE_DEFINES="$(IMAKE_DEFINES)"

	# build nxproxy fifth
	cd nxproxy && autoreconf -vfsi && (${CONFIGURE} $(CUSTOM_VERSION_DEFINE)) && ${MAKE}

	# "build" nxdialog last
	cd nxdialog && autoreconf -vfsi && (${CONFIGURE}) && ${MAKE}
	@echo "<=== $@"

build:
	@echo
	@echo "===> $@"
	if ! test -d nx-X11; then \
	    ${MAKE} build-lite; \
	else \
	    ${MAKE} build-full; \
	fi
	@echo "<=== $@"

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
