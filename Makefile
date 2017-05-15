#!/usr/bin/make -f

INSTALL_DIR=install -d -m 755
INSTALL_FILE=install -m 644
INSTALL_PROGRAM=install -m 755
INSTALL_SYMLINK=ln -s -f
COPY_SYMLINK=cp -a
COPY_DEREFERENCED=cp -RH
RM_FILE=rm -f
RM_DIR=rmdir -p --ignore-fail-on-non-empty

ETCDIR_NX       ?= /etc/nxagent
PREFIX          ?= /usr/local
BINDIR          ?= $(PREFIX)/bin
LIBDIR          ?= $(PREFIX)/lib
SHLIBDIR        ?= $(LIBDIR)
NXLIBDIR        ?= $(SHLIBDIR)/nx
USRLIBDIR       ?= $(NXLIBDIR)/X11
INCLUDEDIR      ?= $(PREFIX)/include
CONFIGURE       ?= ./configure

# use Xfont2 if available in the build env
FONT_DEFINES	?= $(shell pkg-config --modversion xfont2 1>/dev/null 2>/dev/null && echo "-DHAS_XFONT2")
XFONTLIB	?= $(shell pkg-config --modversion xfont2 1>/dev/null 2>/dev/null && echo "-lXfont2" || echo "-lXfont")

NX_VERSION_MAJOR=$(shell ./version.sh 1)
NX_VERSION_MINOR=$(shell ./version.sh 2)
NX_VERSION_MICRO=$(shell ./version.sh 3)
NX_VERSION_PATCH=$(shell ./version.sh 4)

SHELL:=/bin/bash

%:
	if test -f nxcomp/Makefile; then ${MAKE} -C nxcomp $@; fi
	if test -f nxproxy/Makefile; then ${MAKE} -C nxproxy $@; fi
	if test -d nx-X11; then \
	    if test -f nxcompshad/Makefile; then ${MAKE} -C nxcompshad $@; fi; \
	    if test -f nx-X11/Makefile; then ${MAKE} -C nx-X11 $@; fi; \
	fi

	# clean auto-generated files
	if [ "x$@" == "xclean" ] || [ "x$@" = "xdistclean" ]; then \
	    ./mesa-quilt pop -a; \
	    rm -Rf nx-X11/extras/Mesa/.pc/; \
	    rm -f nx-X11/config/cf/nxversion.def; \
	    rm -f nx-X11/config/cf/date.def; \
	    rm -f bin/nxagent; \
	    rm -f bin/nxproxy; \
	    rm -f nx*/install-sh; \
	    rm -f nx*/mkinstalldirs; \
	fi

all:
	${MAKE} build

test:
	echo "No testing for NX (redistributed)"

build-lite:
	ln -s /usr/share/gnulib/build-aux/install-sh    nxcomp/
	ln -s /usr/share/gnulib/build-aux/mkinstalldirs nxcomp/

	cd nxcomp && autoconf && (${CONFIGURE}) && ${MAKE}

	ln -s /usr/share/gnulib/build-aux/install-sh    nxproxy/
	ln -s /usr/share/gnulib/build-aux/mkinstalldirs nxproxy/

	cd nxproxy && autoconf && (${CONFIGURE}) && ${MAKE}

build-full:
# in the full case, we rely on "magic" in the nx-X11 imake-based makefiles...

	ln -s /usr/share/gnulib/build-aux/install-sh    nxcomp/
	ln -s /usr/share/gnulib/build-aux/mkinstalldirs nxcomp/

	cd nxcomp && autoconf && (${CONFIGURE}) && ${MAKE}

	# prepare nx-X11/config/cf/nxversion.def
	sed \
	    -e 's/###NX_VERSION_MAJOR###/$(NX_VERSION_MAJOR)/' \
	    -e 's/###NX_VERSION_MINOR###/$(NX_VERSION_MINOR)/' \
	    -e 's/###NX_VERSION_MICRO###/$(NX_VERSION_MICRO)/' \
	    -e 's/###NX_VERSION_PATCH###/$(NX_VERSION_PATCH)/' \
	    nx-X11/config/cf/nxversion.def.in \
	    > nx-X11/config/cf/nxversion.def

	# prepare Makefiles and the nx-X11 symlinking magic
	cd nx-X11 && make BuildEnv FONT_DEFINES=$(FONT_DEFINES)

	# build libNX_X11 and libNX_Xext prior to building
	# nxcomp{ext,shad}.
	cd nx-X11/lib && make

	ln -s /usr/share/gnulib/build-aux/install-sh    nxcompshad/
	ln -s /usr/share/gnulib/build-aux/mkinstalldirs nxcompshad/

	cd nxcompshad && autoconf && (${CONFIGURE}) && ${MAKE}

	./mesa-quilt push -a

	cd nx-X11 && ${MAKE} World USRLIBDIR=$(USRLIBDIR) SHLIBDIR=$(SHLIBDIR) FONT_DEFINES=$(FONT_DEFINES) XFONTLIB=$(XFONTLIB)

	ln -s /usr/share/gnulib/build-aux/install-sh    nxproxy/
	ln -s /usr/share/gnulib/build-aux/mkinstalldirs nxproxy/

	cd nxproxy && autoconf && (${CONFIGURE}) && ${MAKE}

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

	# install nxproxy wrapper script
	$(INSTALL_DIR) $(DESTDIR)$(BINDIR)
	sed -e 's|@@NXLIBDIR@@|$(NXLIBDIR)|g' bin/nxproxy.in > bin/nxproxy
	$(INSTALL_PROGRAM) bin/nxproxy $(DESTDIR)$(BINDIR)

	# FIXME: the below install logic should work via nxproxy/Makefile.in
	# overriding for now...
	$(INSTALL_DIR) $(DESTDIR)$(NXLIBDIR)/bin
	$(INSTALL_PROGRAM) nxproxy/nxproxy $(DESTDIR)$(NXLIBDIR)/bin

	$(INSTALL_DIR) $(DESTDIR)$(PREFIX)/share/man/man1/
	$(INSTALL_FILE) nxproxy/man/nxproxy.1 $(DESTDIR)$(PREFIX)/share/man/man1/
	gzip $(DESTDIR)$(PREFIX)/share/man/man1/*.1

install-full:
	# install nxagent wrapper script
	$(INSTALL_DIR) $(DESTDIR)$(BINDIR)
	sed -e 's|@@NXLIBDIR@@|$(NXLIBDIR)|g' bin/nxagent.in > bin/nxagent
	$(INSTALL_PROGRAM) bin/nxagent $(DESTDIR)$(BINDIR)

	$(MAKE) -C nxcompshad install

	$(INSTALL_DIR) $(DESTDIR)$(PREFIX)/share/pixmaps
	$(INSTALL_FILE) nx-X11/programs/Xserver/hw/nxagent/nxagent.xpm $(DESTDIR)$(PREFIX)/share/pixmaps

	$(INSTALL_DIR) $(DESTDIR)$(PREFIX)/share/nx
	$(INSTALL_FILE) nx-X11/programs/Xserver/Xext/SecurityPolicy $(DESTDIR)$(PREFIX)/share/nx

	$(INSTALL_DIR) $(DESTDIR)$(NXLIBDIR)/bin
	$(INSTALL_PROGRAM) nx-X11/programs/Xserver/nxagent $(DESTDIR)$(NXLIBDIR)/bin

	$(INSTALL_DIR) $(DESTDIR)$(PREFIX)/share/man/man1/
	$(INSTALL_FILE) nx-X11/programs/Xserver/hw/nxagent/man/nxagent.1 $(DESTDIR)$(PREFIX)/share/man/man1/
	gzip $(DESTDIR)$(PREFIX)/share/man/man1/*.1

	# create a clean nx-X11/.build-exports space
	rm -Rf nx-X11/.build-exports
	mkdir -p nx-X11/.build-exports/include
	mkdir -p nx-X11/.build-exports/lib

	# copy headers (for libnx-x11-dev)
	cp -aL nx-X11/exports/include/* nx-X11/.build-exports/include

	# copy libs (for libnx-x11), we want the targets of the links
	. replace.sh; set -x; find nx-X11/exports/lib/ | grep -F ".so" | while read libpath; do \
	    libfile=$$(basename $$libpath); \
	    libdir=$$(dirname $$libpath); \
	    link=$$(readlink $$libpath); \
	\
	    mkdir -p "$$(string_rep "$$libdir" exports .build-exports)"; \
	    cp -a "$$(string_rep "$$libpath" "$$libfile" "$$link")" "$$(string_rep "$$libdir" exports .build-exports)"; \
	done;

	$(INSTALL_DIR) $(DESTDIR)$(SHLIBDIR)
	$(COPY_SYMLINK) nx-X11/.build-exports/lib/libNX_X11.so $(DESTDIR)$(SHLIBDIR)/
	$(COPY_SYMLINK) nx-X11/.build-exports/lib/libNX_X11.so.6 $(DESTDIR)$(SHLIBDIR)/
	$(COPY_DEREFERENCED) nx-X11/.build-exports/lib/libNX_X11.so.6.2 $(DESTDIR)$(SHLIBDIR)/
	$(INSTALL_DIR) $(DESTDIR)$(USRLIBDIR)
	$(INSTALL_SYMLINK) ../../libNX_X11.so.6 $(DESTDIR)$(USRLIBDIR)/libX11.so.6
	$(INSTALL_SYMLINK) ../../libNX_X11.so.6.2 $(DESTDIR)$(USRLIBDIR)/libX11.so.6.2

	. replace.sh; set -x; find nx-X11/.build-exports/include/{nx*,GL} -type d | \
	    while read dirname; do \
	        $(INSTALL_DIR) "$$(string_rep "$$dirname" nx-X11/.build-exports/include "$(DESTDIR)$(INCLUDEDIR)/")"; \
	        $(INSTALL_FILE) $${dirname}/*.h \
	                        "$$(string_rep "$$dirname" nx-X11/.build-exports/include "$(DESTDIR)$(INCLUDEDIR)/")"/ || true; \
	    done; \

	$(INSTALL_DIR) $(DESTDIR)/$(ETCDIR_NX)
	$(INSTALL_FILE) etc/keystrokes.cfg $(DESTDIR)/$(ETCDIR_NX)/
	$(INSTALL_FILE) etc/nxagent.keyboard $(DESTDIR)$(ETCDIR_NX)/

	$(INSTALL_DIR) $(DESTDIR)$(PREFIX)/share/nx
	$(INSTALL_FILE) nx-X11/lib/X11/XErrorDB $(DESTDIR)$(PREFIX)/share/nx/
	$(INSTALL_FILE) nx-X11/lib/X11/Xcms.txt $(DESTDIR)$(PREFIX)/share/nx/
	$(INSTALL_FILE) VERSION $(DESTDIR)$(PREFIX)/share/nx/VERSION.nxagent
	$(INSTALL_FILE) VERSION $(DESTDIR)$(PREFIX)/share/nx/VERSION.nxproxy

uninstall:
	$(MAKE) uninstall-lite
	[ ! -d nx-X11 ] || $(MAKE) uninstall-full

uninstall-lite:
	if test -f nxcomp/Makefile; then ${MAKE} -C nxcomp $@; fi

	# uninstall nproxy wrapper script
	$(RM_FILE) $(DESTDIR)$(BINDIR)/nxproxy
	# FIXME: don't use uninstall rule in nxproxy/Makefile.in, let's do
        # it on our own for now...
	$(RM_FILE) $(DESTDIR)$(NXLIBDIR)/bin/nxproxy
	$(RM_DIR) $(DESTDIR)$(NXLIBDIR)/bin/
	$(RM_FILE) $(DESTDIR)$(PREFIX)/share/man/man1/*.1
	$(RM_FILE) $(DESTDIR)$(PREFIX)/share/nx/VERSION.nxproxy
	$(RM_DIR) $(DESTDIR)$(PREFIX)/share/nx/

uninstall-full:
	for f in nxagent; do \
	    $(RM_FILE) $(DESTDIR)$(BINDIR)/$$f; done

	$(RM_FILE) $(DESTDIR)$(PREFIX)/share/nx/VERSION.nxagent
	$(RM_DIR) $(DESTDIR)$(PREFIX)/share/nx/

	if test -d nx-X11; then \
	    if test -f nxcompshad/Makefile; then ${MAKE} -C nxcompshad $@; fi; \
	    if test -f nx-X11/Makefile; then \
	        if test -d $(NXLIBDIR); then rm -rf $(NXLIBDIR); fi; \
	        if test -d $(INCLUDEDIR)/nx; then rm -rf $(INCLUDEDIR)/nx; fi; \
	    fi; \
	fi
