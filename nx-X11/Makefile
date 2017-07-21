# $Xorg: Makefile,v 1.5 2000/08/17 19:41:44 cpqbld Exp $
# $Id: Makefile,v 1.5 2005/12/14 23:41:52 alanc Exp $



# $XFree86: xc/Makefile,v 3.28tsi Exp $

# Luna users will need to either run make as "make MAKE=make"
# or add "MAKE = make" to this file.

RELEASE = "Release $(shell cat ../VERSION)"
SHELL = /bin/sh
RM = rm -f
MV = mv
WORLDOPTS =
WIN32WORLDOPTS = -i
TOP = .
CURRENT_DIR = .
CONFIGSRC = $(TOP)/config
IRULESRC = $(CONFIGSRC)/cf
IMAKE = imake
IMAKE_CMD = $(IMAKE) -I$(IRULESRC) $(IMAKE_DEFINES)
MAKE_OPTS = -f xmakefile
MAKE_CMD = $(MAKE) $(MAKE_OPTS)

all:
	@${MAKE_CMD} xmakefile-exists || $(MAKE) all-initial
	@${MAKE_CMD} $@

all-initial:
	@echo Please use make World, or on NT use nmake World.Win32.
	@echo
	@echo Read the release notes carefully before proceeding.
	@echo Do not name your log file make.log or it will be deleted.

World:
	@echo ""
	@echo Building $(RELEASE).
	@echo ""
	@date
	@echo ""
	if [ ! -f xmakefile ]; then ${MAKE} ${MFLAGS} BuildEnv; fi
	${MAKE_CMD} $(MFLAGS) $(WORLDOPTS) World
	@echo ""
	@date
	@echo ""
	@echo Full build of $(RELEASE) complete.
	@echo ""

CleanEnv:
	@if [ -f xmakefile ]; then \
	    ${MAKE_CMD} -k distclean || \
	    ${MAKE_CMD} -k clean || \
	    $(RM) xmakefile; \
	fi
	@rm -f $(IRULESRC)/date.def; echo "" > $(IRULESRC)/date.def;
	${MAKE} $(MFLAGS) xmakefile
	${MAKE_CMD} $(MFLAGS) VerifyOS
	${MAKE_CMD} $(MFLAGS) Makefiles
	${MAKE_CMD} $(MFLAGS) BOOTSTRAPSUBDIRS= clean

BuildEnv: CleanEnv BuildIncludes BuildDependsOnly

BuildIncludes: CleanEnv
	${MAKE_CMD} $(MFLAGS) includes

BuildDependsOnly:
	${MAKE_CMD} $(MFLAGS) depend

.PRECIOUS: Makefile

# This is just a sequence of bootstrapping steps we have to do.
# The steps are listed as separate targets so clearmake can wink in
# the Makefile.proto files.

Makefile::
	$(MAKE) $(MFLAGS) xmakefile

xmakefile: Imakefile
	@if [ -f xmakefile ]; then set -x; \
	 $(RM) xmakefile.bak; $(MV) xmakefile xmakefile.bak; \
	 else exit 0; fi
	@rm -f $(IRULESRC)/date.def; echo "" > $(IRULESRC)/date.def;
	$(IMAKE_CMD) -s xmakefile -DTOPDIR=$(TOP) -DCURDIR=$(CURRENT_DIR)

# don't allow any default rules in this Makefile
.SUFFIXES:
# quiet "make" programs that display a message if suffix list empty
.SUFFIXES: .Dummy

# a copy of every rule that might be invoked at top level

clean:
	-${MAKE_CMD} $@
	find config lib programs include -type f -name Makefile | while read makefile; do rm -f $$makefile; done

dangerous_strip_clean:
	    ${MAKE_CMD} $@
distclean:
	    -${MAKE_CMD} $@
	    $(RM) xmakefile
depend:
	    ${MAKE_CMD} $@
Everything:
	    ${MAKE_CMD} $@
external.ln:
	    ${MAKE_CMD} $@
includes:
	    ${MAKE_CMD} $@
install.sdk:
	    ${MAKE_CMD} $@
install.ln:
	    ${MAKE_CMD} $@
install.man:
	    ${MAKE_CMD} $@
install:
	    ${MAKE_CMD} $@
Makefiles:
	    ${MAKE_CMD} $@
man_keywords:
	    ${MAKE_CMD} $@
tags:
	    ${MAKE_CMD} $@
VerifyOS:
	    ${MAKE_CMD} $@
