XCOMM $XFree86: xc/lib/GL/mesa/src/drv/mga/Imakefile.inc,v 1.2 2001/05/02 15:06:03 dawes Exp $

#ifndef MesaDrvSrcDir
#define MesaDrvSrcDir $(MESASRCDIR)/src/mesa/src/drv
#endif

MESADRVSRCDIR = MesaDrvSrcDir

#ifndef MesaDrvMgaBuildDir
#define MesaDrvMgaBuildDir /**/
#endif
MESADRVMGABUILDDIR = MesaDrvMgaBuildDir

#if Malloc0ReturnsNull
ALLOC_DEFINES = -DMALLOC_0_RETURNS_NULL
#endif

#if BuildXF86DRI
  DRI_DEFINES = GlxDefines -DX_BYTE_ORDER=ByteOrder -DUSE_NEW_INTERFACE -DIN_DRI_DRIVER
 DRI_INCLUDES = -I$(GLXLIBSRC)/dri \
		-I$(XINCLUDESRC) \
		-I$(GLXLIBSRC)/glx \
		-I$(GLXLIBSRC)/include \
		-I$(SERVERSRC)/GL/dri \
		-I$(XF86OSSRC) \
		-I$(DRMSRCDIR)/shared-core \
		-I$(XF86DRIVERSRC)/mga \
		-I$(GLXLIBSRC)/dri/drm
#endif

MESA_INCLUDES = -I$(MESASRCDIR)/src/mesa \
		-I$(MESASRCDIR)/src/mesa/main \
		-I$(MESASRCDIR)/src/mesa/glapi \
		-I$(MESASRCDIR)/src/mesa/shader \
		-I$(MESASRCDIR)/include \
		-I$(MESADRVSRCDIR)/common \
		-I$(MESADRVSRCDIR)/mga
   X_INCLUDES = -I$(XINCLUDESRC) -I$(EXTINCSRC)

      MGASRCS = $(MESADRVMGABUILDDIR)mgadd.c \
		$(MESADRVMGABUILDDIR)mgarender.c \
		$(MESADRVMGABUILDDIR)mgaioctl.c \
		$(MESADRVMGABUILDDIR)mgapixel.c \
		$(MESADRVMGABUILDDIR)mgaspan.c \
		$(MESADRVMGABUILDDIR)mgastate.c \
		$(MESADRVMGABUILDDIR)mgatex.c \
		$(MESADRVMGABUILDDIR)mga_texcombine.c \
		$(MESADRVMGABUILDDIR)mgatexmem.c \
		$(MESADRVMGABUILDDIR)mga_texstate.c \
		$(MESADRVMGABUILDDIR)mgatris.c \
		$(MESADRVMGABUILDDIR)mgavb.c \
		$(MESADRVMGABUILDDIR)mga_xmesa.c \

      MGAOBJS = $(MESADRVMGABUILDDIR)mgadd.o \
		$(MESADRVMGABUILDDIR)mgarender.o \
		$(MESADRVMGABUILDDIR)mgaioctl.o \
		$(MESADRVMGABUILDDIR)mgapixel.o \
		$(MESADRVMGABUILDDIR)mgaspan.o \
		$(MESADRVMGABUILDDIR)mgastate.o \
		$(MESADRVMGABUILDDIR)mgatex.o \
		$(MESADRVMGABUILDDIR)mga_texcombine.o \
		$(MESADRVMGABUILDDIR)mgatexmem.o \
		$(MESADRVMGABUILDDIR)mga_texstate.o \
		$(MESADRVMGABUILDDIR)mgatris.o \
		$(MESADRVMGABUILDDIR)mgavb.o \
		$(MESADRVMGABUILDDIR)mga_xmesa.o

      MGAUOBJS = $(MESADRVMGABUILDDIR)unshared/mgadd.o \
		$(MESADRVMGABUILDDIR)unshared/mgarender.o \
		$(MESADRVMGABUILDDIR)unshared/mgaioctl.o \
		$(MESADRVMGABUILDDIR)unshared/mgapixel.o \
		$(MESADRVMGABUILDDIR)unshared/mgaspan.o \
		$(MESADRVMGABUILDDIR)unshared/mgastate.o \
		$(MESADRVMGABUILDDIR)unshared/mgatex.o \
		$(MESADRVMGABUILDDIR)unshared/mga_texcombine.o \
		$(MESADRVMGABUILDDIR)unshared/mgatexmem.o \
		$(MESADRVMGABUILDDIR)unshared/mga_texstate.o \
		$(MESADRVMGABUILDDIR)unshared/mgatris.o \
		$(MESADRVMGABUILDDIR)unshared/mgavb.o \
		$(MESADRVMGABUILDDIR)unshared/mga_xmesa.o

      MGADOBJS = $(MESADRVMGABUILDDIR)debugger/mgadd.o \
		$(MESADRVMGABUILDDIR)debugger/mgarender.o \
		$(MESADRVMGABUILDDIR)debugger/mgaioctl.o \
		$(MESADRVMGABUILDDIR)debugger/mgapixel.o \
		$(MESADRVMGABUILDDIR)debugger/mgaspan.o \
		$(MESADRVMGABUILDDIR)debugger/mgastate.o \
		$(MESADRVMGABUILDDIR)debugger/mgatex.o \
		$(MESADRVMGABUILDDIR)debugger/mga_texcombine.o \
		$(MESADRVMGABUILDDIR)debugger/mgatexmem.o \
		$(MESADRVMGABUILDDIR)debugger/mga_texstate.o \
		$(MESADRVMGABUILDDIR)debugger/mgatris.o \
		$(MESADRVMGABUILDDIR)debugger/mgavb.o \
		$(MESADRVMGABUILDDIR)debugger/mga_xmesa.o

      MGAPOBJS = $(MESADRVMGABUILDDIR)profiled/mgadd.o \
		$(MESADRVMGABUILDDIR)profiled/mgarender.o \
		$(MESADRVMGABUILDDIR)profiled/mgaioctl.o \
		$(MESADRVMGABUILDDIR)profiled/mgapixel.o \
		$(MESADRVMGABUILDDIR)profiled/mgaspan.o \
		$(MESADRVMGABUILDDIR)profiled/mgastate.o \
		$(MESADRVMGABUILDDIR)profiled/mgatex.o \
		$(MESADRVMGABUILDDIR)profiled/mga_texcombine.o \
		$(MESADRVMGABUILDDIR)profiled/mgatexmem.o \
		$(MESADRVMGABUILDDIR)profiled/mga_texstate.o \
		$(MESADRVMGABUILDDIR)profiled/mgatris.o \
		$(MESADRVMGABUILDDIR)profiled/mgavb.o \
		$(MESADRVMGABUILDDIR)profiled/mga_xmesa.o

#ifdef NeedToLinkMesaSrc
LinkSourceFile(mgacontext.h, $(MESADRVSRCDIR)/mga)
LinkSourceFile(mgadd.c, $(MESADRVSRCDIR)/mga)
LinkSourceFile(mgadd.h, $(MESADRVSRCDIR)/mga)
LinkSourceFile(mgaioctl.c, $(MESADRVSRCDIR)/mga)
LinkSourceFile(mgaioctl.h, $(MESADRVSRCDIR)/mga)
LinkSourceFile(mgapixel.c, $(MESADRVSRCDIR)/mga)
LinkSourceFile(mgapixel.h, $(MESADRVSRCDIR)/mga)
LinkSourceFile(mgarender.c, $(MESADRVSRCDIR)/mga)
LinkSourceFile(mgaregs.h, $(MESADRVSRCDIR)/mga)
LinkSourceFile(mgaspan.c, $(MESADRVSRCDIR)/mga)
LinkSourceFile(mgaspan.h, $(MESADRVSRCDIR)/mga)
LinkSourceFile(mgastate.c, $(MESADRVSRCDIR)/mga)
LinkSourceFile(mgastate.h, $(MESADRVSRCDIR)/mga)
LinkSourceFile(mgatex.c, $(MESADRVSRCDIR)/mga)
LinkSourceFile(mgatex.h, $(MESADRVSRCDIR)/mga)
LinkSourceFile(mga_texcombine.c, $(MESADRVSRCDIR)/mga)
LinkSourceFile(mgatexmem.c, $(MESADRVSRCDIR)/mga)
LinkSourceFile(mga_texstate.c, $(MESADRVSRCDIR)/mga)
LinkSourceFile(mgatris.c, $(MESADRVSRCDIR)/mga)
LinkSourceFile(mgatris.h, $(MESADRVSRCDIR)/mga)
LinkSourceFile(mgavb.c, $(MESADRVSRCDIR)/mga)
LinkSourceFile(mgavb.h, $(MESADRVSRCDIR)/mga)
LinkSourceFile(mga_xmesa.c, $(MESADRVSRCDIR)/mga)
LinkSourceFile(mga_xmesa.h, $(MESADRVSRCDIR)/mga)
#endif
