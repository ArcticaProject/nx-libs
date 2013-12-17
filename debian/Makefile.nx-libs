#!/usr/bin/make -f

INSTALL_DIR=install -d -m 755
INSTALL_FILE=install -m 644
INSTALL_PROGRAM=install -m 755
INSTALL_SYMLINK=ln -s -f
COPY_SYMLINK=cp -a
RM_FILE=rm -f
RM_DIR=rmdir -p --ignore-fail-on-non-empty

ETCDIR_NX ?= /etc/nxagent
ETCDIR_X2GO ?= /etc/x2go
PREFIX ?= /usr/local
BINDIR=$(PREFIX)/bin
NXLIBDIR=$(PREFIX)/lib/nx
X2GOLIBDIR=$(PREFIX)/lib/x2go
CONFIGURE ?= ./configure

SHELL:=/bin/bash

%:
	if test -f nxcomp/Makefile; then ${MAKE} -C nxcomp $@; fi
	if test -f nxproxy/Makefile; then ${MAKE} -C nxproxy $@; fi
	if test -d nx-X11; then \
	    if test -f nxcompext/Makefile; then ${MAKE} -C nxcompext $@; fi; \
	    if test -f nxcompshad/Makefile; then ${MAKE} -C nxcompshad $@; fi; \
	    if test -f nx-X11/Makefile; then ${MAKE} -C nx-X11 $@; fi; \
	fi

all: build

test:
	echo "No testing for NX (redistributed)"

build-lite:
	cd nxcomp && autoconf && (${CONFIGURE}) && ${MAKE}
	cd nxproxy && autoconf && (${CONFIGURE}) && ${MAKE}

build-full:
# in the full case, we rely on "magic" in the nx-X11 imake-based makefiles...
	cd nxcomp && autoconf
	cd nxcompext && autoconf
	cd nxcompshad && autoconf
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
	$(INSTALL_DIR) $(DESTDIR)$(BINDIR)
	for f in nxproxy; do \
	   $(INSTALL_PROGRAM) bin/$$f $(DESTDIR)$(BINDIR); done
	for d in nxcomp nxproxy; do \
	   $(MAKE) -C $$d install; done

install-full:
	for f in nxagent nxauth x2goagent; do \
	   $(INSTALL_PROGRAM) bin/$$f $(DESTDIR)$(BINDIR); done
	for d in nxcompext nxcompshad; do \
	   $(MAKE) -C $$d install; done

	$(INSTALL_DIR) $(DESTDIR)$(X2GOLIBDIR)/bin/
	cd $(DESTDIR)$(X2GOLIBDIR)/bin/ && ln -sf $(NXLIBDIR)/bin/nxagent x2goagent

	$(INSTALL_DIR) $(DESTDIR)$(PREFIX)/share/pixmaps
	$(INSTALL_FILE) nx-X11/programs/Xserver/hw/nxagent/x2go.xpm $(DESTDIR)$(PREFIX)/share/pixmaps

	$(INSTALL_DIR) $(DESTDIR)$(PREFIX)/share/x2go
	$(INSTALL_FILE) rgb $(DESTDIR)$(PREFIX)/share/x2go

	$(INSTALL_DIR) $(DESTDIR)$(PREFIX)/share/nx
	$(INSTALL_FILE) nx-X11/programs/Xserver/Xext/SecurityPolicy $(DESTDIR)$(PREFIX)/share/nx

	$(INSTALL_DIR) $(DESTDIR)$(PREFIX)/share/x2go/versions
	$(INSTALL_FILE) VERSION.x2goagent $(DESTDIR)$(PREFIX)/share/x2go/versions

	$(INSTALL_DIR) $(DESTDIR)$(NXLIBDIR)/bin
	$(INSTALL_PROGRAM) nx-X11/programs/nxauth/nxauth $(DESTDIR)$(NXLIBDIR)/bin
	$(INSTALL_PROGRAM) nx-X11/programs/Xserver/nxagent $(DESTDIR)$(NXLIBDIR)/bin

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

	$(INSTALL_DIR) $(DESTDIR)$(NXLIBDIR)/X11
	$(COPY_SYMLINK) nx-X11/.build-exports/lib/*.so* $(DESTDIR)$(NXLIBDIR)/X11/

	. replace.sh; set -x; find nx-X11/.build-exports/include/ -type d | grep -v "include/X11/bitmaps" | \
	    while read dirname; do \
	        $(INSTALL_DIR) "$$(string_rep "$$dirname" nx-X11/.build-exports/include "$(DESTDIR)$(PREFIX)/include/nx")"; \
	        $(INSTALL_FILE) $${dirname}/*.h \
	                        "$$(string_rep "$$dirname" nx-X11/.build-exports/include "$(DESTDIR)$(PREFIX)/include/nx")"/ || true; \
	    done; \

	# Provide means for Xinerama support in NX/X2Go sessions. This
	# This also requires two post-install symlinks:
	# (DESTDIR)$(NXLIBDIR)/X11/Xinerama/libNX_X11.so.6 -> /usr/<libdir>/libX11.so.6
	# (DESTDIR)$(NXLIBDIR)/X11/Xinerama/libNX_Xext.so.6 -> /usr/<libdir>/libXext.so.6
	$(INSTALL_DIR) $(DESTDIR)$(NXLIBDIR)/X11/Xinerama
	$(INSTALL_SYMLINK) ../libNX_Xinerama.so.1 $(DESTDIR)$(NXLIBDIR)/X11/Xinerama/libXinerama.so.1

	$(INSTALL_DIR) $(DESTDIR)$(PREFIX)/include/nx/X11/bitmaps
	$(INSTALL_FILE) nx-X11/.build-exports/include/X11/bitmaps/* \
	                $(DESTDIR)$(PREFIX)/include/nx/X11/bitmaps/

	$(INSTALL_DIR) $(DESTDIR)$(PREFIX)/include/nx/extras/Mesa/GL/internal
	$(INSTALL_FILE) nx-X11/extras/Mesa/include/GL/*.h \
	                $(DESTDIR)$(PREFIX)/include/nx/extras/Mesa/GL/

	$(INSTALL_FILE) nx-X11/extras/Mesa/include/GL/internal/*.h \
	                $(DESTDIR)$(PREFIX)/include/nx/extras/Mesa/GL/internal/

	$(INSTALL_DIR) $(DESTDIR)$(PREFIX)/include/nx/extras/Mesa/GLES/
	$(INSTALL_FILE) nx-X11/extras/Mesa/include/GLES/*.h \
	                $(DESTDIR)$(PREFIX)/include/nx/extras/Mesa/GLES/

	$(INSTALL_DIR) $(DESTDIR)/$(ETCDIR_NX)
	$(INSTALL_DIR) $(DESTDIR)/$(ETCDIR_X2GO)
	$(INSTALL_FILE) etc/keystrokes.cfg $(DESTDIR)/$(ETCDIR_NX)/
	$(INSTALL_FILE) etc/keystrokes.cfg $(DESTDIR)/$(ETCDIR_X2GO)/

uninstall:
	$(MAKE) uninstall-lite
	[ ! -d nx-X11 ] || $(MAKE) uninstall-full

uninstall-lite:
	for f in nxproxy; do \
	   $(RM_FILE) $(DESTDIR)$(BINDIR)/$$f; done

uninstall-full:
	for f in nxagent nxauth x2goagent; do \
	    $(RM_FILE) $(DESTDIR)$(BINDIR)/$$f; done

	$(RM_FILE) $(DESTDIR)$(X2GOLIBDIR)/bin/x2goagent
	$(RM_DIR) $(DESTDIR)$(X2GOLIBDIR)/bin/

	if test -f nxcomp/Makefile; then ${MAKE} -C nxcomp $@; fi
	if test -f nxproxy/Makefile; then ${MAKE} -C nxproxy $@; fi

	if test -d nx-X11; then \
	    if test -f nxcompext/Makefile; then ${MAKE} -C nxcompext $@; fi; \
	    if test -f nxcompshad/Makefile; then ${MAKE} -C nxcompshad $@; fi; \
	    if test -f nx-X11/Makefile; then \
	        if test -d $(PREFIX)/lib/nx; then rm -rf $(PREFIX)/lib/nx; fi; \
	        if test -d $(PREFIX)/include/nx; then rm -rf $(PREFIX)/include/nx; fi; \
	    fi; \
	fi
