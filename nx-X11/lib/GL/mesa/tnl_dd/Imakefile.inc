XCOMM $XFree86: xc/lib/GL/mesa/src/tnl_dd/Imakefile.inc,v 1.1tsi Exp $

#ifndef MesaTnlDdBuildDir
#define MesaTnlDdBuildDir $(GLXLIBSRC)/mesa/tnl_dd/
#endif

MESATNLDDBUILDDIR = MesaTnlDdBuildDir

 MESA_TNL_DD_SRCS = $(MESATNLDDBUILDDIR)t_dd.c \
		    $(MESATNLDDBUILDDIR)t_dd_vb.c

#ifdef NeedToLinkMesaSrc
LinkSourceFile(t_dd.c, $(MESASRCDIR)/src/mesa/tnl_dd)
LinkSourceFile(t_dd_vb.c, $(MESASRCDIR)/src/mesa/tnl_dd)
LinkSourceFile(t_dd_dmatmp2.h, $(MESASRCDIR)/src/mesa/tnl_dd)
LinkSourceFile(t_dd_dmatmp.h, $(MESASRCDIR)/src/mesa/tnl_dd)
LinkSourceFile(t_dd_rendertmp.h, $(MESASRCDIR)/src/mesa/tnl_dd)
LinkSourceFile(t_dd_triemit.h, $(MESASRCDIR)/src/mesa/tnl_dd)
LinkSourceFile(t_dd_tritmp.h, $(MESASRCDIR)/src/mesa/tnl_dd)
LinkSourceFile(t_dd_unfilled.h, $(MESASRCDIR)/src/mesa/tnl_dd)
LinkSourceFile(t_dd_vbtmp.h, $(MESASRCDIR)/src/mesa/tnl_dd)
LinkSourceFile(t_dd_vertex.h, $(MESASRCDIR)/src/mesa/tnl_dd)
#endif

 MESA_TNL_DD_OBJS = $(MESATNLDDBUILDDIR)t_dd.o \
		    $(MESATNLDDBUILDDIR)t_dd_vb.o

#if defined(DoSharedLib) && DoSharedLib
MESA_TNL_DD_UOBJS = $(MESATNLDDBUILDDIR)unshared/t_dd.o \
		    $(MESATNLDDBUILDDIR)unshared/t_dd_vb.o
#else
MESA_TNL_DD_UOBJS = $(MESA_TNL_DD_OBJS)
#endif

MESA_TNL_DD_DOBJS = $(MESATNLDDBUILDDIR)debugger/t_dd.o \
		    $(MESATNLDDBUILDDIR)debugger/t_dd_vb.o

MESA_TNL_DD_POBJS = $(MESATNLDDBUILDDIR)profiled/t_dd.o \
		    $(MESATNLDDBUILDDIR)profiled/t_dd_vb.o

