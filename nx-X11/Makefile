# $Xorg: Makefile,v 1.5 2000/08/17 19:41:44 cpqbld Exp $
# $Id: Makefile,v 1.5 2005/12/14 23:41:52 alanc Exp $



# $XFree86: xc/Makefile,v 3.28tsi Exp $

# Luna users will need to either run make as "make MAKE=make"
# or add "MAKE = make" to this file.

RELEASE = "Release 6.9"
SHELL = /bin/sh
RM = rm -f
MV = mv
WORLDOPTS =
WIN32WORLDOPTS = -i
TOP = .
CURRENT_DIR = .
CONFIGSRC = $(TOP)/config
IMAKESRC = $(CONFIGSRC)/imake
DEPENDSRC = $(CONFIGSRC)/makedepend
DEPENDTOP = ../..
IMAKETOP = ../..
IRULESRC = $(CONFIGSRC)/cf
IMAKE = $(IMAKESRC)/imake
IMAKE_CMD = $(IMAKE) -I$(IRULESRC) $(IMAKE_DEFINES)
MAKE_OPTS = -f xmakefile
MAKE_CMD = $(MAKE) $(MAKE_OPTS)
FLAGS = $(MFLAGS) -f Makefile.ini BOOTSTRAPCFLAGS="$(BOOTSTRAPCFLAGS)" CC="$(CC)"
VERSINC = -Iprograms/Xserver/hw/xfree86
VERSSRC = $(CONFIGSRC)/util/printver.c
VERSPROG = $(CONFIGSRC)/util/printver.exe

all:
	@${MAKE} ${MAKE_OPTS} xmakefile-exists || $(MAKE) all-initial
	@${MAKE} ${MAKE_OPTS} $@

all-initial:
	@echo Please use make World, or on NT use nmake World.Win32.
	@echo Check the configuration parameters in the $(IRULESRC) directory,
	@echo and pass BOOTSTRAPCFLAGS if necessary.
	@echo Read the release notes carefully before proceeding.
	@echo Do not name your log file make.log or it will be deleted.

World:
	@-$(RM) $(VERSPROG)
	@-$(CC) $(VERSINC) -o $(VERSPROG) $(VERSSRC)
	@echo ""
	@echo Building $(RELEASE).
	@echo ""
	@-$(RM) $(VERSPROG)
	@case "x$(BOOTSTRAPCFLAGS)" in x) \
	echo I hope you checked the configuration parameters in $(IRULESRC) ; \
	echo to see if you need to pass BOOTSTRAPCFLAGS. ; \
	echo "" ; \
	;; esac;
	@date
	@echo ""
	@if [ -f xmakefile ]; then \
	    ${MAKE} ${MAKE_OPTS} -k distclean || \
	    ${MAKE} ${MAKE_OPTS} -k clean || \
	    $(RM) xmakefile; \
	fi
	@if [ ! -f $(IRULESRC)/host.def ]; then \
	    if [ ! -f $(TOP)/lib/Xt/Imakefile ]; then \
		echo "#define BuildServersOnly YES" > $(IRULESRC)/host.def; \
	    else \
		if [ ! -f $(TOP)/fonts/Imakefile ]; then \
		    echo "#define BuildFonts NO" > $(IRULESRC)/host.def; \
		else \
		    echo "" > $(IRULESRC)/host.def; \
		fi; \
	    fi; \
	fi
#	@rm -f $(IRULESRC)/version.def; echo "" > $(IRULESRC)/version.def;
	@rm -f $(IRULESRC)/date.def; echo "" > $(IRULESRC)/date.def;
#	@if [ ! -f $(IRULESRC)/version.def ]; then \
#	    rm -f $(IRULESRC)/version.def; \
#	    echo "" > $(IRULESRC)/version.def; \
#	fi
#	@if [ ! -f $(IRULESRC)/date.def ]; then \
#	    rm -f $(IRULESRC)/date.def; \
#	    echo "" > $(IRULESRC)/date.def; \
#	fi
	cd $(IMAKESRC) && $(MAKE) $(FLAGS) clean
	$(MAKE) $(MFLAGS) Makefile.boot
	${MAKE} ${MAKE_OPTS} $(MFLAGS) version.def
	$(MAKE) $(MFLAGS) Makefile.boot
	${MAKE} ${MAKE_OPTS} $(MFLAGS) VerifyOS
	${MAKE} ${MAKE_OPTS} $(MFLAGS) Makefiles
	${MAKE} ${MAKE_OPTS} $(MFLAGS) BOOTSTRAPSUBDIRS= clean
	${MAKE} ${MAKE_OPTS} $(MFLAGS) includes
	${MAKE} ${MAKE_OPTS} $(MFLAGS) depend
	${MAKE} ${MAKE_OPTS} $(MFLAGS) $(WORLDOPTS) World
	@echo ""
	@date
	@echo ""
	@-$(RM) $(VERSPROG)
	@-$(CC) $(VERSINC) -o $(VERSPROG) $(VERSSRC)
	@echo Full build of $(RELEASE) complete.
	@-$(RM) $(VERSPROG)
	@echo ""

.PRECIOUS: Makefile

# This is just a sequence of bootstrapping steps we have to do.
# The steps are listed as separate targets so clearmake can wink in
# the Makefile.proto files.

Makefile.boot: imake.bootstrap
	$(RM) $(IMAKESRC)/Makefile.proto

imake.proto:
	cd $(IMAKESRC) && $(MAKE) $(FLAGS)
	$(RM) $(DEPENDSRC)/Makefile.proto

$(DEPENDSRC)/Makefile.proto: imake.proto
	$(IMAKE_CMD) -s $(DEPENDSRC)/Makefile.proto -f $(DEPENDSRC)/Imakefile -DTOPDIR=$(DEPENDTOP) -DCURDIR=$(DEPENDSRC)

depend.bootstrap: $(DEPENDSRC)/Makefile.proto
	cd $(DEPENDSRC) && $(RM) -r Makefile Makefile.dep makedepend *.o bootstrap
	cd $(DEPENDSRC) && $(MAKE) -f Makefile.proto bootstrap

$(IMAKESRC)/Makefile.proto: depend.bootstrap
	$(IMAKE_CMD) -s $(IMAKESRC)/Makefile.proto -f $(IMAKESRC)/Imakefile -DTOPDIR=$(IMAKETOP) -DCURDIR=$(IMAKESRC) -DBootStrap

imake.bootstrap: $(IMAKESRC)/Makefile.proto
	cd $(IMAKESRC) && $(MAKE) -f Makefile.proto bootstrapdepend
	cd $(IMAKESRC) && $(MAKE) $(FLAGS) bootstrap
	cd $(IMAKESRC) && $(MAKE) -f Makefile.proto imakeonly
	-@if [ -f xmakefile ]; then set -x; \
	  $(RM) xmakefile.bak; $(MV) xmakefile xmakefile.bak; \
	  else exit 0; fi
	$(MAKE) $(MFLAGS) xmakefile

Makefile::
	$(MAKE) $(MFLAGS) xmakefile

xmakefile: Imakefile
	$(RM) xmakefile
	@rm -f $(IRULESRC)/date.def; echo "" > $(IRULESRC)/date.def;
	$(IMAKE_CMD) -s xmakefile -DTOPDIR=$(TOP) -DCURDIR=$(CURRENT_DIR)

World.Win32:
	@echo :
	@echo Building $(RELEASE).
	@echo :
	@echo :
	-@if not exist $(IRULESRC)\host.def echo > $(IRULESRC)\host.def
	cd $(IMAKESRC)
	$(MAKE) -f Makefile.ini clean.Win32
	$(MAKE) -f Makefile.ini imake.exe
	cd ..\..
	-if exist xmakefile.bak del xmakefile.bak
	-if exist xmakefile ren xmakefile xmakefile.bak
	$(IMAKE:/=\) -s xmakefile -I$(IRULESRC) $(IMAKE_DEFINES) -DTOPDIR=$(TOP) -DCURDIR=$(CURRENT_DIR)
	${MAKE} ${MAKE_OPTS} $(MFLAGS) VerifyOS
	${MAKE} ${MAKE_OPTS} $(MFLAGS) Makefiles
	${MAKE} ${MAKE_OPTS} $(MFLAGS) clean
	cd $(CONFIGSRC)\util
	$(MAKE) mkdirhier.exe
	cd ..\..
	${MAKE} ${MAKE_OPTS} $(MFLAGS) includes
	${MAKE} ${MAKE_OPTS} $(MFLAGS) depend
	${MAKE} ${MAKE_OPTS} $(MFLAGS) $(WIN32WORLDOPTS)
	@echo :
	@echo :
	@echo Full build of $(RELEASE) complete.
	@echo :

World.OS2:
	@echo :
	@echo Building $(RELEASE) on OS/2.
	@echo :
	@echo :
	-@if not exist $(IRULESRC)\host.def echo > $(IRULESRC)\host.def
	\indir $(IMAKESRC) $(MAKE) SHELL= -f Makefile.ini clean.os2
	\indir $(IMAKESRC) $(MAKE) SHELL= CC=gcc -f Makefile.ini imake.os2
	-if exist Makefile.bak del Makefile.bak
	-if exist Makefile ren Makefile Makefile.bak
	$(subst /,\,$(IMAKE)) -I$(IRULESRC) $(IMAKE_DEFINES) -DTOPDIR=$(TOP) -DCURDIR=$(CURRENT_DIR)
	$(MAKE) $(MFLAGS) VerifyOS
	$(MAKE) $(MFLAGS) Makefiles
	$(MAKE) $(MFLAGS) clean
	$(MAKE) $(MFLAGS) includes
	$(MAKE) $(MFLAGS) depend
	$(MAKE) $(MFLAGS)
	@echo :
	@echo :
	@echo Full build of $(RELEASE) on OS/2 complete.
	@echo :

# don't allow any default rules in this Makefile
.SUFFIXES:
# quiet "make" programs that display a message if suffix list empty
.SUFFIXES: .Dummy

# a copy of every rule that might be invoked at top level

clean:
	    -${MAKE_CMD} ${MAKE_OPTS} $@
	    find config -type f | egrep '.*/Makefile$$' | while read makefile; do rm -f $$makefile; done
	    find lib -type f | egrep '.*/Makefile$$' | while read makefile; do rm -f $$makefile; done
	    find programs -type f | egrep '.*/Makefile$$' | while read makefile; do rm -f $$makefile; done
	    find include -type f | egrep '.*/Makefile$$' | while read makefile; do rm -f $$makefile; done
	    rm -f config/cf/{version.def,date.def}
dangerous_strip_clean:
	    ${MAKE} ${MAKE_OPTS} $@
distclean:
	    -${MAKE} ${MAKE_OPTS} $@
	    $(RM) xmakefile $(IRULESRC)/version.def $(IRULESRC)/date.def
depend:
	    ${MAKE} ${MAKE_OPTS} $@
Everything:
	    ${MAKE} ${MAKE_OPTS} $@
external.ln:
	    ${MAKE} ${MAKE_OPTS} $@
includes:
	    ${MAKE} ${MAKE_OPTS} $@
install.sdk:
	    ${MAKE} ${MAKE_OPTS} $@
install.ln:
	    ${MAKE} ${MAKE_OPTS} $@
install.man:
	    ${MAKE} ${MAKE_OPTS} $@
install:
	    ${MAKE} ${MAKE_OPTS} $@
Makefiles:
	    ${MAKE} ${MAKE_OPTS} $@
man_keywords:
	    ${MAKE} ${MAKE_OPTS} $@
tags:
	    ${MAKE} ${MAKE_OPTS} $@
VerifyOS:
	    ${MAKE} ${MAKE_OPTS} $@
