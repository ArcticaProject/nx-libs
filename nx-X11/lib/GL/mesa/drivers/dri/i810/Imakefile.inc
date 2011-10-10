XCOMM $XFree86: xc/lib/GL/mesa/src/drv/i810/Imakefile.inc,v 1.2 2001/05/02 15:06:03 dawes Exp $

#ifndef MesaDrvSrcDir
#define MesaDrvSrcDir $(MESASRCDIR)/src/mesa/drivers/dri
#endif

MESADRVSRCDIR = MesaDrvSrcDir

#ifndef MesaDrvI810BuildDir
#define MesaDrvI810BuildDir /**/
#endif
MESADRVI810BUILDDIR = MesaDrvI810BuildDir

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
		-I$(XF86DRIVERSRC)/i810 \
		-I$(GLXLIBSRC)/dri/drm
#endif

MESA_INCLUDES = -I$(MESASRCDIR)/src/mesa \
		-I$(MESASRCDIR)/src/mesa/main \
		-I$(MESASRCDIR)/src/mesa/glapi \
		-I$(MESASRCDIR)/src/mesa/shader \
		-I$(MESASRCDIR)/include \
		-I$(MESADRVSRCDIR)/common \
		-I$(MESADRVSRCDIR)/i810
   X_INCLUDES = -I$(XINCLUDESRC) -I$(EXTINCSRC)

     I810SRCS = $(MESADRVI810BUILDDIR)i810context.c \
		$(MESADRVI810BUILDDIR)i810ioctl.c \
		$(MESADRVI810BUILDDIR)i810render.c \
		$(MESADRVI810BUILDDIR)i810screen.c \
		$(MESADRVI810BUILDDIR)i810span.c \
		$(MESADRVI810BUILDDIR)i810state.c \
		$(MESADRVI810BUILDDIR)i810tex.c \
		$(MESADRVI810BUILDDIR)i810texmem.c \
		$(MESADRVI810BUILDDIR)i810texstate.c \
		$(MESADRVI810BUILDDIR)i810tris.c \
		$(MESADRVI810BUILDDIR)i810vb.c

     I810OBJS = $(MESADRVI810BUILDDIR)i810context.o \
		$(MESADRVI810BUILDDIR)i810ioctl.o \
		$(MESADRVI810BUILDDIR)i810render.o \
		$(MESADRVI810BUILDDIR)i810screen.o \
		$(MESADRVI810BUILDDIR)i810span.o \
		$(MESADRVI810BUILDDIR)i810state.o \
		$(MESADRVI810BUILDDIR)i810tex.o \
		$(MESADRVI810BUILDDIR)i810texmem.o \
		$(MESADRVI810BUILDDIR)i810texstate.o \
		$(MESADRVI810BUILDDIR)i810tris.o \
		$(MESADRVI810BUILDDIR)i810vb.o

     I810UOBJS = $(MESADRVI810BUILDDIR)unshared/i810context.o \
		$(MESADRVI810BUILDDIR)unshared/i810ioctl.o \
		$(MESADRVI810BUILDDIR)unshared/i810render.o \
		$(MESADRVI810BUILDDIR)unshared/i810screen.o \
		$(MESADRVI810BUILDDIR)unshared/i810span.o \
		$(MESADRVI810BUILDDIR)unshared/i810state.o \
		$(MESADRVI810BUILDDIR)unshared/i810tex.o \
		$(MESADRVI810BUILDDIR)unshared/i810texmem.o \
		$(MESADRVI810BUILDDIR)unshared/i810texstate.o \
		$(MESADRVI810BUILDDIR)unshared/i810tris.o \
		$(MESADRVI810BUILDDIR)unshared/i810vb.o

     I810DOBJS = $(MESADRVI810BUILDDIR)debugger/i810context.o \
		$(MESADRVI810BUILDDIR)debugger/i810ioctl.o \
		$(MESADRVI810BUILDDIR)debugger/i810render.o \
		$(MESADRVI810BUILDDIR)debugger/i810screen.o \
		$(MESADRVI810BUILDDIR)debugger/i810span.o \
		$(MESADRVI810BUILDDIR)debugger/i810state.o \
		$(MESADRVI810BUILDDIR)debugger/i810tex.o \
		$(MESADRVI810BUILDDIR)debugger/i810texmem.o \
		$(MESADRVI810BUILDDIR)debugger/i810texstate.o \
		$(MESADRVI810BUILDDIR)debugger/i810tris.o \
		$(MESADRVI810BUILDDIR)debugger/i810vb.o

     I810POBJS = $(MESADRVI810BUILDDIR)profiled/i810context.o \
		$(MESADRVI810BUILDDIR)profiled/i810ioctl.o \
		$(MESADRVI810BUILDDIR)profiled/i810render.o \
		$(MESADRVI810BUILDDIR)profiled/i810screen.o \
		$(MESADRVI810BUILDDIR)profiled/i810span.o \
		$(MESADRVI810BUILDDIR)profiled/i810state.o \
		$(MESADRVI810BUILDDIR)profiled/i810tex.o \
		$(MESADRVI810BUILDDIR)profiled/i810texmem.o \
		$(MESADRVI810BUILDDIR)profiled/i810texstate.o \
		$(MESADRVI810BUILDDIR)profiled/i810tris.o \
		$(MESADRVI810BUILDDIR)profiled/i810vb.o

#ifdef NeedToLinkMesaSrc
LinkSourceFile(i810_3d_reg.h, $(MESADRVSRCDIR)/i810)
LinkSourceFile(i810context.c, $(MESADRVSRCDIR)/i810)
LinkSourceFile(i810context.h, $(MESADRVSRCDIR)/i810)
LinkSourceFile(i810ioctl.c, $(MESADRVSRCDIR)/i810)
LinkSourceFile(i810ioctl.h, $(MESADRVSRCDIR)/i810)
LinkSourceFile(i810render.c, $(MESADRVSRCDIR)/i810)
LinkSourceFile(i810screen.c, $(MESADRVSRCDIR)/i810)
LinkSourceFile(i810screen.h, $(MESADRVSRCDIR)/i810)
LinkSourceFile(i810span.c, $(MESADRVSRCDIR)/i810)
LinkSourceFile(i810span.h, $(MESADRVSRCDIR)/i810)
LinkSourceFile(i810state.c, $(MESADRVSRCDIR)/i810)
LinkSourceFile(i810state.h, $(MESADRVSRCDIR)/i810)
LinkSourceFile(i810tex.c, $(MESADRVSRCDIR)/i810)
LinkSourceFile(i810tex.h, $(MESADRVSRCDIR)/i810)
LinkSourceFile(i810texmem.c, $(MESADRVSRCDIR)/i810)
LinkSourceFile(i810texstate.c, $(MESADRVSRCDIR)/i810)
LinkSourceFile(i810tris.c, $(MESADRVSRCDIR)/i810)
LinkSourceFile(i810tris.h, $(MESADRVSRCDIR)/i810)
LinkSourceFile(i810vb.c, $(MESADRVSRCDIR)/i810)
LinkSourceFile(i810vb.h, $(MESADRVSRCDIR)/i810)
#endif
