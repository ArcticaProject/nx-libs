XCOMM $XFree86: xc/lib/GL/mesa/src/drv/via/Imakefile.inc,v 1.1 2001/03/23 19:18:43 dawes Exp $

#ifndef MesaDrvSrcDir
#define MesaDrvSrcDir $(MESASRCDIR)/src/mesa/drivers/dri
#endif

MESADRVSRCDIR = MesaDrvSrcDir

#ifndef MesaDrvViaBuildDir
#define MesaDrvViaBuildDir /**/
#endif
MESADRVVIABUILDDIR = MesaDrvViaBuildDir

#if Malloc0ReturnsNull
ALLOC_DEFINES = -DMALLOC_0_RETURNS_NULL
#endif

#if BuildXF86DRI
  DRI_DEFINES = GlxDefines $(VIA_DEFINES) -DX_BYTE_ORDER=$(X_BYTE_ORDER) -DIN_DRI_DRIVER
 DRI_INCLUDES = -I$(GLXLIBSRC)/dri \
		-I$(XINCLUDESRC) \
		-I$(GLXLIBSRC)/glx \
		-I$(GLXLIBSRC)/include \
		-I$(SERVERSRC)/GL/dri \
		-I$(XF86COMSRC) \
		-I$(XF86OSSRC) \
		-I$(DRMSRCDIR)/shared-core \
		-I$(XF86DRIVERSRC)/via \
		-I$(GLXLIBSRC)/dri/drm
#endif

MESA_INCLUDES = -I$(MESASRCDIR)/src/mesa \
		-I$(MESASRCDIR)/src/mesa/main \
		-I$(MESASRCDIR)/src/mesa/glapi \
		-I$(MESASRCDIR)/src/mesa/shader \
		-I$(MESASRCDIR)/include \
		-I$(MESADRVSRCDIR)/common \
		-I$(MESADRVSRCDIR)/unichrome
   X_INCLUDES = -I$(XINCLUDESRC) -I$(EXTINCSRC)


      VIASRCS = $(MESADRVVIABUILDDIR)via_context.c \
		$(MESADRVVIABUILDDIR)via_fb.c \
		$(MESADRVVIABUILDDIR)via_ioctl.c \
		$(MESADRVVIABUILDDIR)via_memcpy.c \
		$(MESADRVVIABUILDDIR)via_render.c \
		$(MESADRVVIABUILDDIR)via_screen.c \
		$(MESADRVVIABUILDDIR)via_span.c \
		$(MESADRVVIABUILDDIR)via_state.c \
		$(MESADRVVIABUILDDIR)via_tex.c \
		$(MESADRVVIABUILDDIR)via_texcombine.c \
		$(MESADRVVIABUILDDIR)via_tris.c

      VIAOBJS =$(MESADRVVIABUILDDIR)via_context.o \
		$(MESADRVVIABUILDDIR)via_fb.o \
		$(MESADRVVIABUILDDIR)via_ioctl.o \
		$(MESADRVVIABUILDDIR)via_memcpy.o \
		$(MESADRVVIABUILDDIR)via_render.o \
		$(MESADRVVIABUILDDIR)via_screen.o \
		$(MESADRVVIABUILDDIR)via_span.o \
		$(MESADRVVIABUILDDIR)via_state.o \
		$(MESADRVVIABUILDDIR)via_tex.o \
		$(MESADRVVIABUILDDIR)via_texcombine.o \
		$(MESADRVVIABUILDDIR)via_tris.o

      VIAUOBJS =$(MESADRVVIABUILDDIR)/unshared/via_context.o \
		$(MESADRVVIABUILDDIR)/unshared/via_fb.o \
		$(MESADRVVIABUILDDIR)/unshared/via_ioctl.o \
		$(MESADRVVIABUILDDIR)/unshared/via_memcpy.o \
		$(MESADRVVIABUILDDIR)/unshared/via_render.o \
		$(MESADRVVIABUILDDIR)/unshared/via_screen.o \
		$(MESADRVVIABUILDDIR)/unshared/via_span.o \
		$(MESADRVVIABUILDDIR)/unshared/via_state.o \
		$(MESADRVVIABUILDDIR)/unshared/via_tex.o \
		$(MESADRVVIABUILDDIR)/unshared/via_texcombine.o \
		$(MESADRVVIABUILDDIR)/unshared/via_tris.o

      VIADOBJS =$(MESADRVVIABUILDDIR)/debugger/via_context.o \
		$(MESADRVVIABUILDDIR)/debugger/via_fb.o \
		$(MESADRVVIABUILDDIR)/debugger/via_ioctl.o \
		$(MESADRVVIABUILDDIR)/debugger/via_memcpy.o \
		$(MESADRVVIABUILDDIR)/debugger/via_render.o \
		$(MESADRVVIABUILDDIR)/debugger/via_screen.o \
		$(MESADRVVIABUILDDIR)/debugger/via_span.o \
		$(MESADRVVIABUILDDIR)/debugger/via_state.o \
		$(MESADRVVIABUILDDIR)/debugger/via_tex.o \
		$(MESADRVVIABUILDDIR)/debugger/via_texcombine.o \
		$(MESADRVVIABUILDDIR)/debugger/via_tris.o

      VIAPOBJS =$(MESADRVVIABUILDDIR)/profiled/via_context.o \
		$(MESADRVVIABUILDDIR)/profiled/via_fb.o \
		$(MESADRVVIABUILDDIR)/profiled/via_ioctl.o \
		$(MESADRVVIABUILDDIR)/profiled/via_memcpy.o \
		$(MESADRVVIABUILDDIR)/profiled/via_render.o \
		$(MESADRVVIABUILDDIR)/profiled/via_screen.o \
		$(MESADRVVIABUILDDIR)/profiled/via_span.o \
		$(MESADRVVIABUILDDIR)/profiled/via_state.o \
		$(MESADRVVIABUILDDIR)/profiled/via_tex.o \
		$(MESADRVVIABUILDDIR)/profiled/via_texcombine.o \
		$(MESADRVVIABUILDDIR)/profiled/via_tris.o

#ifdef NeedToLinkMesaSrc
LinkSourceFile(via_3d_reg.h, $(MESADRVSRCDIR)/unichrome)
LinkSourceFile(via_context.c, $(MESADRVSRCDIR)/unichrome)
LinkSourceFile(via_context.h, $(MESADRVSRCDIR)/unichrome)
LinkSourceFile(via_fb.c, $(MESADRVSRCDIR)/unichrome)
LinkSourceFile(via_fb.h, $(MESADRVSRCDIR)/unichrome)
LinkSourceFile(via_ioctl.c, $(MESADRVSRCDIR)/unichrome)
LinkSourceFile(via_ioctl.h, $(MESADRVSRCDIR)/unichrome)
LinkSourceFile(via_memcpy.c, $(MESADRVSRCDIR)/unichrome)
LinkSourceFile(via_render.c, $(MESADRVSRCDIR)/unichrome)
LinkSourceFile(via_screen.c, $(MESADRVSRCDIR)/unichrome)
LinkSourceFile(via_screen.h, $(MESADRVSRCDIR)/unichrome)
LinkSourceFile(via_span.c, $(MESADRVSRCDIR)/unichrome)
LinkSourceFile(via_span.h, $(MESADRVSRCDIR)/unichrome)
LinkSourceFile(via_state.c, $(MESADRVSRCDIR)/unichrome)
LinkSourceFile(via_state.h, $(MESADRVSRCDIR)/unichrome)
LinkSourceFile(via_tex.c, $(MESADRVSRCDIR)/unichrome)
LinkSourceFile(via_texcombine.c, $(MESADRVSRCDIR)/unichrome)
LinkSourceFile(via_tex.h, $(MESADRVSRCDIR)/unichrome)
LinkSourceFile(via_tris.c, $(MESADRVSRCDIR)/unichrome)
LinkSourceFile(via_tris.h, $(MESADRVSRCDIR)/unichrome)
#endif
