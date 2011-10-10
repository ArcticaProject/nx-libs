XCOMM $XFree86: xc/lib/GL/mesa/src/drv/mach64/Imakefile.inc,v 1.2 2001/05/02 15:06:03 dawes Exp $

#ifndef MesaDrvSrcDir
#define MesaDrvSrcDir $(MESASRCDIR)/src/mesa/drivers/dri
#endif

MESADRVSRCDIR = MesaDrvSrcDir

#ifndef MesaDrvMach64BuildDir
#define MesaDrvMach64BuildDir /**/
#endif
MESADRVMach64BUILDDIR = MesaDrvMach64BuildDir

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
		-I$(DRMSRCDIR)/shared-core/ \
		-I$(XF86DRIVERSRC)/ati \
		-I$(GLXLIBSRC)/dri/drm
#endif

MESA_INCLUDES = -I$(MESASRCDIR)/src/mesa \
		-I$(MESASRCDIR)/src/mesa/main \
		-I$(MESASRCDIR)/src/mesa/glapi \
		-I$(MESASRCDIR)/include \
		-I$(MESADRVSRCDIR)/common \
		-I$(MESADRVSRCDIR)/mach64
   X_INCLUDES = -I$(XINCLUDESRC) -I$(EXTINCSRC)

     Mach64SRCS = $(MESADRVMach64BUILDDIR)mach64_context.c \
		$(MESADRVMach64BUILDDIR)mach64_dd.c \
		$(MESADRVMach64BUILDDIR)mach64_ioctl.c \
		$(MESADRVMach64BUILDDIR)mach64_lock.c \
		$(MESADRVMach64BUILDDIR)mach64_screen.c \
		$(MESADRVMach64BUILDDIR)mach64_span.c \
		$(MESADRVMach64BUILDDIR)mach64_state.c \
		$(MESADRVMach64BUILDDIR)mach64_tex.c \
		$(MESADRVMach64BUILDDIR)mach64_texmem.c \
		$(MESADRVMach64BUILDDIR)mach64_texstate.c \
		$(MESADRVMach64BUILDDIR)mach64_tris.c \
		$(MESADRVMach64BUILDDIR)mach64_vb.c

     Mach64OBJS = $(MESADRVMach64BUILDDIR)mach64_context.o \
		$(MESADRVMach64BUILDDIR)mach64_dd.o \
		$(MESADRVMach64BUILDDIR)mach64_ioctl.o \
		$(MESADRVMach64BUILDDIR)mach64_lock.o \
		$(MESADRVMach64BUILDDIR)mach64_screen.o \
		$(MESADRVMach64BUILDDIR)mach64_span.o \
		$(MESADRVMach64BUILDDIR)mach64_state.o \
		$(MESADRVMach64BUILDDIR)mach64_tex.o \
		$(MESADRVMach64BUILDDIR)mach64_texmem.o \
		$(MESADRVMach64BUILDDIR)mach64_texstate.o \
		$(MESADRVMach64BUILDDIR)mach64_tris.o \
		$(MESADRVMach64BUILDDIR)mach64_vb.o

     Mach64UOBJS = $(MESADRVMach64BUILDDIR)unshared/mach64_context.o \
		$(MESADRVMach64BUILDDIR)unshared/mach64_dd.o \
		$(MESADRVMach64BUILDDIR)unshared/mach64_ioctl.o \
		$(MESADRVMach64BUILDDIR)unshared/mach64_lock.o \
		$(MESADRVMach64BUILDDIR)unshared/mach64_screen.o \
		$(MESADRVMach64BUILDDIR)unshared/mach64_span.o \
		$(MESADRVMach64BUILDDIR)unshared/mach64_state.o \
		$(MESADRVMach64BUILDDIR)unshared/mach64_tex.o \
		$(MESADRVMach64BUILDDIR)unshared/mach64_texmem.o \
		$(MESADRVMach64BUILDDIR)unshared/mach64_texstate.o \
		$(MESADRVMach64BUILDDIR)unshared/mach64_tris.o \
		$(MESADRVMach64BUILDDIR)unshared/mach64_vb.o

     Mach64DOBJS = $(MESADRVMach64BUILDDIR)debugger/mach64_context.o \
                $(MESADRVMach64BUILDDIR)debugger/mach64_dd.o \
                $(MESADRVMach64BUILDDIR)debugger/mach64_ioctl.o \
                $(MESADRVMach64BUILDDIR)debugger/mach64_lock.o \
                $(MESADRVMach64BUILDDIR)debugger/mach64_screen.o \
                $(MESADRVMach64BUILDDIR)debugger/mach64_span.o \
                $(MESADRVMach64BUILDDIR)debugger/mach64_state.o \
                $(MESADRVMach64BUILDDIR)debugger/mach64_tex.o \
                $(MESADRVMach64BUILDDIR)debugger/mach64_texmem.o \
                $(MESADRVMach64BUILDDIR)debugger/mach64_texstate.o \
                $(MESADRVMach64BUILDDIR)debugger/mach64_tris.o \
                $(MESADRVMach64BUILDDIR)debugger/mach64_vb.o

    Mach64POBJS = $(MESADRVMach64BUILDDIR)profiled/mach64_context.o \
                $(MESADRVMach64BUILDDIR)profiled/mach64_dd.o \
                $(MESADRVMach64BUILDDIR)profiled/mach64_ioctl.o \
                $(MESADRVMach64BUILDDIR)profiled/mach64_lock.o \
                $(MESADRVMach64BUILDDIR)profiled/mach64_screen.o \
                $(MESADRVMach64BUILDDIR)profiled/mach64_span.o \
                $(MESADRVMach64BUILDDIR)profiled/mach64_state.o \
                $(MESADRVMach64BUILDDIR)profiled/mach64_tex.o \
                $(MESADRVMach64BUILDDIR)profiled/mach64_texmem.o \
                $(MESADRVMach64BUILDDIR)profiled/mach64_texstate.o \
                $(MESADRVMach64BUILDDIR)profiled/mach64_tris.o \
                $(MESADRVMach64BUILDDIR)profiled/mach64_vb.o

#ifdef NeedToLinkMesaSrc
LinkSourceFile(mach64_reg.h, $(MESADRVSRCDIR)/mach64)
LinkSourceFile(mach64_context.c, $(MESADRVSRCDIR)/mach64)
LinkSourceFile(mach64_context.h, $(MESADRVSRCDIR)/mach64)
LinkSourceFile(mach64_dd.c, $(MESADRVSRCDIR)/mach64)
LinkSourceFile(mach64_dd.h, $(MESADRVSRCDIR)/mach64)
LinkSourceFile(mach64_ioctl.c, $(MESADRVSRCDIR)/mach64)
LinkSourceFile(mach64_ioctl.h, $(MESADRVSRCDIR)/mach64)
LinkSourceFile(mach64_lock.c, $(MESADRVSRCDIR)/mach64)
LinkSourceFile(mach64_lock.h, $(MESADRVSRCDIR)/mach64)
LinkSourceFile(mach64_native_vb.c, $(MESADRVSRCDIR)/mach64)
LinkSourceFile(mach64_native_vbtmp.h, $(MESADRVSRCDIR)/mach64)
LinkSourceFile(mach64_screen.c, $(MESADRVSRCDIR)/mach64)
LinkSourceFile(mach64_screen.h, $(MESADRVSRCDIR)/mach64)
LinkSourceFile(mach64_span.c, $(MESADRVSRCDIR)/mach64)
LinkSourceFile(mach64_span.h, $(MESADRVSRCDIR)/mach64)
LinkSourceFile(mach64_state.c, $(MESADRVSRCDIR)/mach64)
LinkSourceFile(mach64_state.h, $(MESADRVSRCDIR)/mach64)
LinkSourceFile(mach64_tex.c, $(MESADRVSRCDIR)/mach64)
LinkSourceFile(mach64_tex.h, $(MESADRVSRCDIR)/mach64)
LinkSourceFile(mach64_texmem.c, $(MESADRVSRCDIR)/mach64)
LinkSourceFile(mach64_texstate.c, $(MESADRVSRCDIR)/mach64)
LinkSourceFile(mach64_tris.c, $(MESADRVSRCDIR)/mach64)
LinkSourceFile(mach64_tris.h, $(MESADRVSRCDIR)/mach64)
LinkSourceFile(mach64_vb.c, $(MESADRVSRCDIR)/mach64)
LinkSourceFile(mach64_vb.h, $(MESADRVSRCDIR)/mach64)
LinkSourceFile(mach64_vbtmp.h, $(MESADRVSRCDIR)/mach64)
#endif
