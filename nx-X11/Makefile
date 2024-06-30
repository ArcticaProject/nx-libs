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
	@${MAKE} ${MAKE_OPTS} xmakefile-exists || $(MAKE) all-initial
	@${MAKE} ${MAKE_OPTS} $@

all-initial:
	@echo Please use make World, or on NT use nmake World.Win32.
	@echo
	@echo Read the release notes carefully before proceeding.
	@echo Do not name your log file make.log or it will be deleted.

World: BuildEnv
	@echo ""
	@echo Building $(RELEASE).
	@echo ""
	@date
	@echo ""
	${MAKE} ${MAKE_OPTS} $(MFLAGS) $(WORLDOPTS) World
	@echo ""
	@date
	@echo ""
	@echo Full build of $(RELEASE) complete.
	@echo ""

CleanEnv: SetupEnv
	${MAKE} ${MAKE_OPTS} -k clean
	rm -f $(IRULESRC)/date.def; echo "" > $(IRULESRC)/date.def;

SetupEnv: xmakefile VerifyOS Makefiles

BuildEnv: SetupEnv BuildIncludes BuildDependsOnly

BuildIncludes: SetupEnv includes

BuildDependsOnly: SetupEnv depend

Makefile::
	$(MAKE) $(MFLAGS) xmakefile
.PRECIOUS: Makefile

xmakefile:
	if [ -f xmakefile ]; then \
	    set -x; \
	    $(RM) xmakefile.bak; \
	    $(MV) xmakefile xmakefile.bak; \
	else \
	    exit 0; \
	fi
	which $(IMAKE) 1>/dev/null && $(IMAKE_CMD) -s xmakefile -DTOPDIR=$(TOP) -DCURDIR=$(CURRENT_DIR)

# don't allow any default rules in this Makefile
.SUFFIXES:
# quiet "make" programs that display a message if suffix list empty
.SUFFIXES: .Dummy

# a copy of every rule that might be invoked at top level

clean: CleanEnv

dangerous_strip_clean: xmakefile
	${MAKE} ${MAKE_OPTS} $@

distclean: clean
	-${MAKE} ${MAKE_OPTS} ${MFLAGS} $@
	find config programs include -type f -name Makefile | while read makefile; do rm -f $$makefile; done
	$(RM) xmakefile

depend: xmakefile
	${MAKE} ${MAKE_OPTS} ${MFLAGS} $@

Everything: SetupEnv
	${MAKE} ${MAKE_OPTS} ${MFLAGS} $@

external.ln: xmakefile
	${MAKE} ${MAKE_OPTS} ${MFLAGS} $@

includes: xmakefile VerifyOS Makefiles
	${MAKE} ${MAKE_OPTS} ${MFLAGS} $@

install.sdk: SetupEnv
	${MAKE} ${MAKE_OPTS} ${MFLAGS} $@

install.ln: SetupEnv
	${MAKE} ${MAKE_OPTS} ${MFLAGS} $@

install.man: SetupEnv
	${MAKE} ${MAKE_OPTS} ${MFLAGS} $@

install: SetupEnv
	${MAKE} ${MAKE_OPTS} ${MFLAGS} $@

Makefiles: xmakefile VerifyOS
	${MAKE} ${MAKE_OPTS} ${MFLAGS} $@

man_keywords: SetupEnv
	${MAKE} ${MAKE_OPTS} ${MFLAGS} $@

tags: xmakefile
	${MAKE} ${MAKE_OPTS} ${MFLAGS} $@

VerifyOS: xmakefile
	${MAKE} ${MAKE_OPTS} ${MFLAGS} $@
