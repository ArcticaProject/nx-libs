#!/usr/bin/make -f

INSTALL_DIR=install -d -m 755
INSTALL_FILE=install -m 644
INSTALL_PROGRAM=install -m 755
INSTALL_SYMLINK=ln -s -f
COPY_SYMLINK=cp -a
RM_FILE=rm -f
RM_DIR=rmdir -p --ignore-fail-on-non-empty

ETCDIR_NX   ?= /etc/nxagent
PREFIX      ?= /usr/local
BINDIR      ?= $(PREFIX)/bin
LIBDIR      ?= $(PREFIX)/lib
USRLIBDIR   ?= $(LIBDIR)
INCLUDEDIR  ?= $(PREFIX)/include
NXLIBDIR    ?= $(LIBDIR)/nx
CONFIGURE   ?= ./configure

NX_VERSION_MAJOR=$(shell ./version.sh 1)
NX_VERSION_MINOR=$(shell ./version.sh 2)
NX_VERSION_MICRO=$(shell ./version.sh 3)
NX_VERSION_PATCH=$(shell ./version.sh 4)

SHELL:=/bin/bash

%:
	if test -f nxcomp/Makefile; then ${MAKE} -C nxcomp $@; fi
	if test -f nxproxy/Makefile; then ${MAKE} -C nxproxy $@; fi
	if test -d nx-X11; then \
	    if test -f nxcompext/Makefile; then ${MAKE} -C nxcompext $@; fi; \
	    if test -f nxcompshad/Makefile; then ${MAKE} -C nxcompshad $@; fi; \
	    if test -f nx-X11/Makefile; then ${MAKE} -C nx-X11 $@; fi; \
	fi

	# clean auto-generated nxversion.def file \
	if [ "x$@" == "xclean" ] || [ "x$@" = "xdistclean" ]; then \
	    rm -f nx-X11/config/cf/nxversion.def; \
	    rm -f bin/nxagent; \
	    rm -f bin/nxproxy; \
	fi

all: build

test:
	echo "No testing for NX (redistributed)"

build-lite:
	cd nxcomp && autoconf && (${CONFIGURE}) && ${MAKE}
	cd nxproxy && autoconf && (${CONFIGURE}) && ${MAKE}

build-full:
# in the full case, we rely on "magic" in the nx-X11 imake-based makefiles...
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
	cd nx-X11 && make BuildEnv

	# build libNX_X11 and libNX_Xext prior to building
	# nxcomp{ext,shad}.
	cd nx-X11/lib && make

	cd nxcompext && autoconf && (${CONFIGURE}) && ${MAKE}
	cd nxcompshad && autoconf && (${CONFIGURE}) && ${MAKE}

	cd nx-X11 && ${MAKE} World

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

	for d in nxcompext nxcompshad; do \
	   $(MAKE) -C $$d install; done

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

	$(INSTALL_DIR) $(DESTDIR)$(USRLIBDIR)
	$(COPY_SYMLINK) nx-X11/.build-exports/lib/*.so* $(DESTDIR)$(USRLIBDIR)/
	$(INSTALL_DIR) $(DESTDIR)$(USRLIBDIR)/nx-X11
	$(INSTALL_SYMLINK) ../libNX_X11.so $(DESTDIR)$(USRLIBDIR)/nx-X11/libX11.so
	$(INSTALL_SYMLINK) ../libNX_X11.so.6.2 $(DESTDIR)$(USRLIBDIR)/nx-X11/libX11.so.6.2

	. replace.sh; set -x; find nx-X11/.build-exports/include/ -type d | \
	    while read dirname; do \
	        $(INSTALL_DIR) "$$(string_rep "$$dirname" nx-X11/.build-exports/include "$(DESTDIR)$(INCLUDEDIR)/")"; \
	        $(INSTALL_FILE) $${dirname}/*.h \
	                        "$$(string_rep "$$dirname" nx-X11/.build-exports/include "$(DESTDIR)$(INCLUDEDIR)/")"/ || true; \
	    done; \

	$(INSTALL_DIR) $(DESTDIR)/$(ETCDIR_NX)
	$(INSTALL_FILE) etc/keystrokes.cfg $(DESTDIR)/$(ETCDIR_NX)/
	$(INSTALL_FILE) etc/rgb $(DESTDIR)$(ETCDIR_NX)/
	$(INSTALL_FILE) etc/nxagent.keyboard $(DESTDIR)$(ETCDIR_NX)/

	$(INSTALL_DIR) $(DESTDIR)$(PREFIX)/share/nx
	$(INSTALL_SYMLINK) $(ETCDIR_NX)/rgb $(DESTDIR)$(PREFIX)/share/nx/rgb
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
	$(RM_DIR) $(DESTDIR)$(NXLIBDIR)/share/nx/

uninstall-full:
	for f in nxagent; do \
	    $(RM_FILE) $(DESTDIR)$(BINDIR)/$$f; done

	$(RM_FILE) $(DESTDIR)$(PREFIX)/share/nx/VERSION.nxagent
	$(RM_DIR) $(DESTDIR)$(NXLIBDIR)/share/nx/

	if test -d nx-X11; then \
	    if test -f nxcompext/Makefile; then ${MAKE} -C nxcompext $@; fi; \
	    if test -f nxcompshad/Makefile; then ${MAKE} -C nxcompshad $@; fi; \
	    if test -f nx-X11/Makefile; then \
	        if test -d $(NXLIBDIR); then rm -rf $(NXLIBDIR); fi; \
	        if test -d $(INCLUDEDIR)/nx; then rm -rf $(INCLUDEDIR)/nx; fi; \
	    fi; \
	fi
