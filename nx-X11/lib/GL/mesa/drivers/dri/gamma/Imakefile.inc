XCOMM $XFree86: xc/lib/GL/mesa/src/drv/gamma/Imakefile.inc,v 1.2 2001/05/02 15:06:03 dawes Exp $

#ifndef MesaDrvSrcDir
#define MesaDrvSrcDir $(MESASRCDIR)/src/mesa/drivers/dri
#endif

MESADRVSRCDIR = MesaDrvSrcDir

#ifndef MesaDrvGammaBuildDir
#define MesaDrvGammaBuildDir /**/
#endif
MESADRVGAMMABUILDDIR = MesaDrvGammaBuildDir

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
		-I$(XF86OSSRC) \
		-I$(DRMSRCDIR)/shared-core \
		-I$(XF86DRIVERSRC)/glint \
		-I$(GLXLIBSRC)/dri/drm
#endif

MESA_INCLUDES = -I$(MESASRCDIR)/src/mesa \
		-I$(MESASRCDIR)/src/mesa/main \
		-I$(MESASRCDIR)/src/mesa/glapi \
		-I$(MESASRCDIR)/src/mesa/shader \
		-I$(MESASRCDIR)/include \
		-I$(MESADRVSRCDIR)/common \
		-I$(MESADRVSRCDIR)/gamma
   X_INCLUDES = -I$(XINCLUDESRC) -I$(EXTINCSRC)


    GAMMASRCS = $(MESADRVGAMMABUILDDIR)gamma_context.c \
		$(MESADRVGAMMABUILDDIR)gamma_dd.c \
		$(MESADRVGAMMABUILDDIR)gamma_inithw.c \
		$(MESADRVGAMMABUILDDIR)gamma_lock.c \
		$(MESADRVGAMMABUILDDIR)gamma_render.c \
		$(MESADRVGAMMABUILDDIR)gamma_screen.c \
		$(MESADRVGAMMABUILDDIR)gamma_span.c \
		$(MESADRVGAMMABUILDDIR)gamma_state.c \
		$(MESADRVGAMMABUILDDIR)gamma_tex.c \
		$(MESADRVGAMMABUILDDIR)gamma_texmem.c \
		$(MESADRVGAMMABUILDDIR)gamma_texstate.c \
		$(MESADRVGAMMABUILDDIR)gamma_tris.c \
		$(MESADRVGAMMABUILDDIR)gamma_vb.c \
		$(MESADRVGAMMABUILDDIR)gamma_xmesa.c

    GAMMAOBJS = $(MESADRVGAMMABUILDDIR)gamma_context.o \
		$(MESADRVGAMMABUILDDIR)gamma_dd.o \
		$(MESADRVGAMMABUILDDIR)gamma_inithw.o \
		$(MESADRVGAMMABUILDDIR)gamma_lock.o \
		$(MESADRVGAMMABUILDDIR)gamma_render.o \
		$(MESADRVGAMMABUILDDIR)gamma_screen.o \
		$(MESADRVGAMMABUILDDIR)gamma_span.o \
		$(MESADRVGAMMABUILDDIR)gamma_state.o \
		$(MESADRVGAMMABUILDDIR)gamma_tex.o \
		$(MESADRVGAMMABUILDDIR)gamma_texmem.o \
		$(MESADRVGAMMABUILDDIR)gamma_texstate.o \
		$(MESADRVGAMMABUILDDIR)gamma_tris.o \
		$(MESADRVGAMMABUILDDIR)gamma_vb.o \
		$(MESADRVGAMMABUILDDIR)gamma_xmesa.o

    GAMMAUOBJS = $(MESADRVGAMMABUILDDIR)unshared/gamma_context.o \
		$(MESADRVGAMMABUILDDIR)unshared/gamma_dd.o \
		$(MESADRVGAMMABUILDDIR)unshared/gamma_inithw.o \
		$(MESADRVGAMMABUILDDIR)unshared/gamma_lock.o \
		$(MESADRVGAMMABUILDDIR)unshared/gamma_render.o \
		$(MESADRVGAMMABUILDDIR)unshared/gamma_screen.o \
		$(MESADRVGAMMABUILDDIR)unshared/gamma_span.o \
		$(MESADRVGAMMABUILDDIR)unshared/gamma_state.o \
		$(MESADRVGAMMABUILDDIR)unshared/gamma_tex.o \
		$(MESADRVGAMMABUILDDIR)unshared/gamma_texmem.o \
		$(MESADRVGAMMABUILDDIR)unshared/gamma_texstate.o \
		$(MESADRVGAMMABUILDDIR)unshared/gamma_tris.o \
		$(MESADRVGAMMABUILDDIR)unshared/gamma_vb.o \
		$(MESADRVGAMMABUILDDIR)unshared/gamma_xmesa.o

    GAMMADOBJS = $(MESADRVGAMMABUILDDIR)debugger/gamma_context.o \
		$(MESADRVGAMMABUILDDIR)debugger/gamma_dd.o \
		$(MESADRVGAMMABUILDDIR)debugger/gamma_inithw.o \
		$(MESADRVGAMMABUILDDIR)debugger/gamma_lock.o \
		$(MESADRVGAMMABUILDDIR)debugger/gamma_render.o \
		$(MESADRVGAMMABUILDDIR)debugger/gamma_screen.o \
		$(MESADRVGAMMABUILDDIR)debugger/gamma_span.o \
		$(MESADRVGAMMABUILDDIR)debugger/gamma_state.o \
		$(MESADRVGAMMABUILDDIR)debugger/gamma_tex.o \
		$(MESADRVGAMMABUILDDIR)debugger/gamma_texmem.o \
		$(MESADRVGAMMABUILDDIR)debugger/gamma_texstate.o \
		$(MESADRVGAMMABUILDDIR)debugger/gamma_tris.o \
		$(MESADRVGAMMABUILDDIR)debugger/gamma_vb.o \
		$(MESADRVGAMMABUILDDIR)debugger/gamma_xmesa.o

    GAMMAPOBJS = $(MESADRVGAMMABUILDDIR)profiled/gamma_context.o \
		$(MESADRVGAMMABUILDDIR)profiled/gamma_dd.o \
		$(MESADRVGAMMABUILDDIR)profiled/gamma_inithw.o \
		$(MESADRVGAMMABUILDDIR)profiled/gamma_lock.o \
		$(MESADRVGAMMABUILDDIR)profiled/gamma_render.o \
		$(MESADRVGAMMABUILDDIR)profiled/gamma_screen.o \
		$(MESADRVGAMMABUILDDIR)profiled/gamma_span.o \
		$(MESADRVGAMMABUILDDIR)profiled/gamma_state.o \
		$(MESADRVGAMMABUILDDIR)profiled/gamma_tex.o \
		$(MESADRVGAMMABUILDDIR)profiled/gamma_texmem.o \
		$(MESADRVGAMMABUILDDIR)profiled/gamma_texstate.o \
		$(MESADRVGAMMABUILDDIR)profiled/gamma_tris.o \
		$(MESADRVGAMMABUILDDIR)profiled/gamma_vb.o \
		$(MESADRVGAMMABUILDDIR)profiled/gamma_xmesa.o

#ifdef NeedToLinkMesaSrc
LinkSourceFile(gamma_context.c, $(MESADRVSRCDIR)/gamma)
LinkSourceFile(gamma_context.h, $(MESADRVSRCDIR)/gamma)
LinkSourceFile(gamma_client.h, $(MESADRVSRCDIR)/gamma)
LinkSourceFile(gamma_dd.c, $(MESADRVSRCDIR)/gamma)
LinkSourceFile(gamma_inithw.c, $(MESADRVSRCDIR)/gamma)
LinkSourceFile(gamma_lock.c, $(MESADRVSRCDIR)/gamma)
LinkSourceFile(gamma_lock.h, $(MESADRVSRCDIR)/gamma)
LinkSourceFile(gamma_macros.h, $(MESADRVSRCDIR)/gamma)
LinkSourceFile(gamma_render.c, $(MESADRVSRCDIR)/gamma)
LinkSourceFile(gamma_regs.h, $(MESADRVSRCDIR)/gamma)
LinkSourceFile(gamma_screen.c, $(MESADRVSRCDIR)/gamma)
LinkSourceFile(gamma_screen.h, $(MESADRVSRCDIR)/gamma)
LinkSourceFile(gamma_span.c, $(MESADRVSRCDIR)/gamma)
LinkSourceFile(gamma_state.c, $(MESADRVSRCDIR)/gamma)
LinkSourceFile(gamma_tex.c, $(MESADRVSRCDIR)/gamma)
LinkSourceFile(gamma_texmem.c, $(MESADRVSRCDIR)/gamma)
LinkSourceFile(gamma_texstate.c, $(MESADRVSRCDIR)/gamma)
LinkSourceFile(gamma_tris.c, $(MESADRVSRCDIR)/gamma)
LinkSourceFile(gamma_tris.h, $(MESADRVSRCDIR)/gamma)
LinkSourceFile(gamma_tritmp.h, $(MESADRVSRCDIR)/gamma)
LinkSourceFile(gamma_vb.c, $(MESADRVSRCDIR)/gamma)
LinkSourceFile(gamma_vb.h, $(MESADRVSRCDIR)/gamma)
LinkSourceFile(gamma_xmesa.c, $(MESADRVSRCDIR)/gamma)
#endif
