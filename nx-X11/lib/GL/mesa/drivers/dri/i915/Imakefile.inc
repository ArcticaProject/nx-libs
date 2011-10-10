#ifndef MesaDrvSrcDir
#define MesaDrvSrcDir $(MESASRCDIR)/src/mesa/drivers/dri
#endif

MESADRVSRCDIR = MesaDrvSrcDir

#ifndef MesaDrvI915BuildDir
#define MesaDrvI915BuildDir /**/
#endif
MESADRVI915BUILDDIR = MesaDrvI915BuildDir

#if Malloc0ReturnsNull
ALLOC_DEFINES = -DMALLOC_0_RETURNS_NULL
#endif

#if BuildXF86DRI
  DRI_DEFINES = GlxDefines -DIN_DRI_DRIVER
  DRI_INCLUDES = -I$(GLXLIBSRC)/dri \
		-I$(XINCLUDESRC) \
		-I$(GLXLIBSRC)/glx \
		-I$(GLXLIBSRC)/include \
		-I$(SERVERSRC)/GL/dri \
 		-I$(XF86OSSRC) \
 		-I$(DRMSRCDIR)/shared-core \
 		-I$(XF86DRIVERSRC)/i810 \
		-I$(GLXLIBSRC)/dri/drm
#endif
 
MESA_INCLUDES = -I$(MESASRCDIR)/src/mesa \
		-I$(MESASRCDIR)/src/mesa/main \
		-I$(MESASRCDIR)/src/mesa/glapi \
		-I$(MESASRCDIR)/src/mesa/shader \
		-I$(MESASRCDIR)/include \
		-I$(MESADRVSRCDIR)/common
   X_INCLUDES = -I$(XINCLUDESRC) -I$(EXTINCSRC)

     I915SRCS = 	$(MESADRVI915BUILDDIR)i830_metaops.c \
		$(MESADRVI915BUILDDIR)i830_context.c \
		$(MESADRVI915BUILDDIR)i830_state.c \
		$(MESADRVI915BUILDDIR)i830_tex.c \
		$(MESADRVI915BUILDDIR)i830_texstate.c \
		$(MESADRVI915BUILDDIR)i830_texblend.c \
		$(MESADRVI915BUILDDIR)i830_vtbl.c \
		$(MESADRVI915BUILDDIR)i915_metaops.c \
		$(MESADRVI915BUILDDIR)i915_context.c \
		$(MESADRVI915BUILDDIR)i915_debug.c \
		$(MESADRVI915BUILDDIR)i915_fragprog.c \
		$(MESADRVI915BUILDDIR)i915_state.c \
		$(MESADRVI915BUILDDIR)i915_tex.c \
		$(MESADRVI915BUILDDIR)i915_texprog.c \
		$(MESADRVI915BUILDDIR)i915_texstate.c \
		$(MESADRVI915BUILDDIR)i915_program.c \
		$(MESADRVI915BUILDDIR)i915_vtbl.c \
		$(MESADRVI915BUILDDIR)intel_batchbuffer.c \
		$(MESADRVI915BUILDDIR)intel_context.c \
		$(MESADRVI915BUILDDIR)intel_ioctl.c \
		$(MESADRVI915BUILDDIR)intel_pixel.c \
		$(MESADRVI915BUILDDIR)intel_render.c \
		$(MESADRVI915BUILDDIR)intel_screen.c \
		$(MESADRVI915BUILDDIR)intel_span.c \
		$(MESADRVI915BUILDDIR)intel_state.c \
		$(MESADRVI915BUILDDIR)intel_tex.c \
		$(MESADRVI915BUILDDIR)intel_tris.c 



     I915OBJS = 	$(MESADRVI915BUILDDIR)i830_metaops.o \
		$(MESADRVI915BUILDDIR)i830_context.o \
		$(MESADRVI915BUILDDIR)i830_state.o \
		$(MESADRVI915BUILDDIR)i830_tex.o \
		$(MESADRVI915BUILDDIR)i830_texstate.o \
		$(MESADRVI915BUILDDIR)i830_texblend.o \
		$(MESADRVI915BUILDDIR)i830_vtbl.o \
		$(MESADRVI915BUILDDIR)i915_metaops.o \
		$(MESADRVI915BUILDDIR)i915_context.o \
		$(MESADRVI915BUILDDIR)i915_debug.o \
		$(MESADRVI915BUILDDIR)i915_fragprog.o \
		$(MESADRVI915BUILDDIR)i915_state.o \
		$(MESADRVI915BUILDDIR)i915_tex.o \
		$(MESADRVI915BUILDDIR)i915_texprog.o \
		$(MESADRVI915BUILDDIR)i915_texstate.o \
		$(MESADRVI915BUILDDIR)i915_program.o \
		$(MESADRVI915BUILDDIR)i915_vtbl.o \
		$(MESADRVI915BUILDDIR)intel_batchbuffer.o \
		$(MESADRVI915BUILDDIR)intel_context.o \
		$(MESADRVI915BUILDDIR)intel_ioctl.o \
		$(MESADRVI915BUILDDIR)intel_pixel.o \
		$(MESADRVI915BUILDDIR)intel_render.o \
		$(MESADRVI915BUILDDIR)intel_screen.o \
		$(MESADRVI915BUILDDIR)intel_span.o \
		$(MESADRVI915BUILDDIR)intel_state.o \
		$(MESADRVI915BUILDDIR)intel_tex.o \
		$(MESADRVI915BUILDDIR)intel_tris.o 

    I915UOBJS = 
    I915DOBJS = 
    I915POBJS = 

#ifdef NeedToLinkMesaSrc
LinkSourceFile(i830_metaops.c, $(MESADRVSRCDIR)/i915)
LinkSourceFile(i830_context.c, $(MESADRVSRCDIR)/i915)
LinkSourceFile(i830_context.h, $(MESADRVSRCDIR)/i915)
LinkSourceFile(i830_state.c, $(MESADRVSRCDIR)/i915)
LinkSourceFile(i830_reg.h, $(MESADRVSRCDIR)/i915)
LinkSourceFile(i830_tex.c, $(MESADRVSRCDIR)/i915)
LinkSourceFile(i830_texstate.c, $(MESADRVSRCDIR)/i915)
LinkSourceFile(i830_texblend.c, $(MESADRVSRCDIR)/i915)
LinkSourceFile(i830_vtbl.c, $(MESADRVSRCDIR)/i915)
LinkSourceFile(i915_metaops.c, $(MESADRVSRCDIR)/i915)
LinkSourceFile(i915_context.c, $(MESADRVSRCDIR)/i915)
LinkSourceFile(i915_context.h, $(MESADRVSRCDIR)/i915)
LinkSourceFile(i915_debug.c, $(MESADRVSRCDIR)/i915)
LinkSourceFile(i915_fragprog.c, $(MESADRVSRCDIR)/i915)
LinkSourceFile(i915_state.c, $(MESADRVSRCDIR)/i915)
LinkSourceFile(i915_tex.c, $(MESADRVSRCDIR)/i915)
LinkSourceFile(i915_texprog.c, $(MESADRVSRCDIR)/i915)
LinkSourceFile(i915_texstate.c, $(MESADRVSRCDIR)/i915)
LinkSourceFile(i915_program.c, $(MESADRVSRCDIR)/i915)
LinkSourceFile(i915_program.h, $(MESADRVSRCDIR)/i915)
LinkSourceFile(i915_reg.h, $(MESADRVSRCDIR)/i915)
LinkSourceFile(i915_vtbl.c, $(MESADRVSRCDIR)/i915)
LinkSourceFile(intel_batchbuffer.c, $(MESADRVSRCDIR)/i915)
LinkSourceFile(intel_batchbuffer.h, $(MESADRVSRCDIR)/i915)
LinkSourceFile(intel_context.c, $(MESADRVSRCDIR)/i915)
LinkSourceFile(intel_context.h, $(MESADRVSRCDIR)/i915)
LinkSourceFile(intel_ioctl.c, $(MESADRVSRCDIR)/i915)
LinkSourceFile(intel_ioctl.h, $(MESADRVSRCDIR)/i915)
LinkSourceFile(intel_pixel.c, $(MESADRVSRCDIR)/i915)
LinkSourceFile(intel_reg.h, $(MESADRVSRCDIR)/i915)
LinkSourceFile(intel_render.c, $(MESADRVSRCDIR)/i915)
LinkSourceFile(intel_screen.c, $(MESADRVSRCDIR)/i915)
LinkSourceFile(intel_screen.h, $(MESADRVSRCDIR)/i915)
LinkSourceFile(intel_span.c, $(MESADRVSRCDIR)/i915)
LinkSourceFile(intel_span.h, $(MESADRVSRCDIR)/i915)
LinkSourceFile(intel_state.c, $(MESADRVSRCDIR)/i915)
LinkSourceFile(intel_tex.c, $(MESADRVSRCDIR)/i915)
LinkSourceFile(intel_tex.h, $(MESADRVSRCDIR)/i915)
LinkSourceFile(intel_tris.c, $(MESADRVSRCDIR)/i915)
LinkSourceFile(intel_tris.h, $(MESADRVSRCDIR)/i915)
#endif
