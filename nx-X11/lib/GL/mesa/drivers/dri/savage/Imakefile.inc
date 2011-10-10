XCOMM $XFree86: xc/lib/GL/mesa/src/drv/mga/Imakefile.inc,v 1.2 2001/05/02 15:06:03 dawes Exp $

#ifndef MesaDrvSrcDir
#define MesaDrvSrcDir $(MESASRCDIR)/src/mesa/src/drv
#endif

MESADRVSRCDIR = MesaDrvSrcDir

#ifndef MesaDrvSavageBuildDir
#define MesaDrvSavageBuildDir /**/
#endif
MESADRVSAVAGEBUILDDIR = MesaDrvSavageBuildDir

#if Malloc0ReturnsNull
ALLOC_DEFINES = -DMALLOC_0_RETURNS_NULL
#endif

#if BuildXF86DRI
  DRI_DEFINES = GlxDefines -DX_BYTE_ORDER=ByteOrder -DIN_DRI_DRIVER
 DRI_INCLUDES = -I$(GLXLIBSRC)/dri \
		-I$(XINCLUDESRC) \
		-I$(GLXLIBSRC)/glx \
		-I$(GLXLIBSRC)/include \
		-I$(SERVERSRC)/GL/dri \
		-I$(XF86COMSRC) \
		-I$(XF86OSSRC) \
		-I$(DRMSRCDIR)/shared-core \
		-I$(XF86DRIVERSRC)/savage \
		-I$(GLXLIBSRC)/dri/drm
#endif

MESA_INCLUDES = -I$(MESASRCDIR)/src/mesa \
		-I$(MESASRCDIR)/src/mesa/main \
		-I$(MESASRCDIR)/src/mesa/glapi \
		-I$(MESASRCDIR)/src/mesa/shader \
		-I$(MESASRCDIR)/include \
		-I$(MESADRVSRCDIR)/common \
		-I$(MESADRVSRCDIR)/savage
   X_INCLUDES = -I$(XINCLUDESRC) -I$(EXTINCSRC)

   SAVAGESRCS = $(MESADRVMGABUILDDIR)savagedd.c \
		$(MESADRVMGABUILDDIR)savageioctl.c \
		$(MESADRVMGABUILDDIR)savagerender.c \
		$(MESADRVMGABUILDDIR)savagespan.c \
		$(MESADRVMGABUILDDIR)savagestate.c \
		$(MESADRVMGABUILDDIR)savagetex.c \
		$(MESADRVMGABUILDDIR)savagetris.c \
		$(MESADRVMGABUILDDIR)savage_xmesa.c

   SAVAGEOBJS = $(MESADRVMGABUILDDIR)savagedd.o \
		$(MESADRVMGABUILDDIR)savageioctl.o \
		$(MESADRVMGABUILDDIR)savagerender.o \
		$(MESADRVMGABUILDDIR)savagespan.o \
		$(MESADRVMGABUILDDIR)savagestate.o \
		$(MESADRVMGABUILDDIR)savagetex.o \
		$(MESADRVMGABUILDDIR)savagetris.o \
		$(MESADRVMGABUILDDIR)savage_xmesa.o

  SAVAGEUOBJS = $(MESADRVMGABUILDDIR)unshared/savagedd.o \
		$(MESADRVMGABUILDDIR)unshared/savageioctl.o \
		$(MESADRVMGABUILDDIR)unshared/savagerender.o \
		$(MESADRVMGABUILDDIR)unshared/savagespan.o \
		$(MESADRVMGABUILDDIR)unshared/savagestate.o \
		$(MESADRVMGABUILDDIR)unshared/savagetex.o \
		$(MESADRVMGABUILDDIR)unshared/savagetris.o \
		$(MESADRVMGABUILDDIR)unshared/savage_xmesa.o

  SAVAGEDOBJS = $(MESADRVMGABUILDDIR)debugger/savagedd.o \
		$(MESADRVMGABUILDDIR)debugger/savageioctl.o \
		$(MESADRVMGABUILDDIR)debugger/savagerender.o \
		$(MESADRVMGABUILDDIR)debugger/savagespan.o \
		$(MESADRVMGABUILDDIR)debugger/savagestate.o \
		$(MESADRVMGABUILDDIR)debugger/savagetex.o \
		$(MESADRVMGABUILDDIR)debugger/savagetris.o \
		$(MESADRVMGABUILDDIR)debugger/savage_xmesa.o

  SAVAGEPOBJS = $(MESADRVMGABUILDDIR)profiled/savagedd.o \
		$(MESADRVMGABUILDDIR)profiled/savageioctl.o \
		$(MESADRVMGABUILDDIR)profiled/savagerender.o \
		$(MESADRVMGABUILDDIR)profiled/savagespan.o \
		$(MESADRVMGABUILDDIR)profiled/savagestate.o \
		$(MESADRVMGABUILDDIR)profiled/savagetex.o \
		$(MESADRVMGABUILDDIR)profiled/savagetris.o \
		$(MESADRVMGABUILDDIR)profiled/savage_xmesa.o

#ifdef NeedToLinkMesaSrc
LinkSourceFile(savage_3d_reg.h, $(MESADRVSRCDIR)/savage)
LinkSourceFile(savage_bci.h, $(MESADRVSRCDIR)/savage)
LinkSourceFile(savagecontext.h, $(MESADRVSRCDIR)/savage)
LinkSourceFile(savagedd.c, $(MESADRVSRCDIR)/savage)
LinkSourceFile(savagedd.h, $(MESADRVSRCDIR)/savage)
LinkSourceFile(savage_init.h, $(MESADRVSRCDIR)/savage)
LinkSourceFile(savageioctl.c, $(MESADRVSRCDIR)/savage)
LinkSourceFile(savageioctl.h, $(MESADRVSRCDIR)/savage)
LinkSourceFile(savagerender.c, $(MESADRVSRCDIR)/savage)
LinkSourceFile(savagespan.c, $(MESADRVSRCDIR)/savage)
LinkSourceFile(savagespan.h, $(MESADRVSRCDIR)/savage)
LinkSourceFile(savagestate.c, $(MESADRVSRCDIR)/savage)
LinkSourceFile(savagestate.h, $(MESADRVSRCDIR)/savage)
LinkSourceFile(savagetex.c, $(MESADRVSRCDIR)/savage)
LinkSourceFile(savagetex.h, $(MESADRVSRCDIR)/savage)
LinkSourceFile(savagetris.c, $(MESADRVSRCDIR)/savage)
LinkSourceFile(savagetris.h, $(MESADRVSRCDIR)/savage)
LinkSourceFile(savage_xmesa.c, $(MESADRVSRCDIR)/savage)
#endif
