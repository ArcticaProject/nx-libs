MESASWRSETUPDIR = MesaSwrastSetupBuildDir

 MESA_SWR_SETUP_SRCS = $(MESASWRSETUPDIR)ss_context.c \
		       $(MESASWRSETUPDIR)ss_triangle.c

#ifdef NeedToLinkMesaSrc
LinkSourceFile(ss_context.c, $(MESASRCDIR)/src/mesa/swrast_setup)
LinkSourceFile(ss_context.h, $(MESASRCDIR)/src/mesa/swrast_setup)
LinkSourceFile(ss_triangle.c, $(MESASRCDIR)/src/mesa/swrast_setup)
LinkSourceFile(ss_triangle.h, $(MESASRCDIR)/src/mesa/swrast_setup)
LinkSourceFile(ss_tritmp.h, $(MESASRCDIR)/src/mesa/swrast_setup)
LinkSourceFile(swrast_setup.h, $(MESASRCDIR)/src/mesa/swrast_setup)
#endif

 MESA_SWR_SETUP_OBJS = $(MESASWRSETUPDIR)ss_context.o \
		       $(MESASWRSETUPDIR)ss_triangle.o

#if defined(DoSharedLib) && DoSharedLib
MESA_SWR_SETUP_UOBJS = $(MESASWRSETUPDIR)unshared/ss_context.o \
		       $(MESASWRSETUPDIR)unshared/ss_triangle.o
#else
MESA_SWR_SETUP_UOBJS = $(MESA_SWR_SETUP_OBJS)
#endif

MESA_SWR_SETUP_DOBJS = $(MESASWRSETUPDIR)debugger/ss_context.o \
		       $(MESASWRSETUPDIR)debugger/ss_triangle.o

MESA_SWR_SETUP_POBJS = $(MESASWRSETUPDIR)profiled/ss_context.o \
		       $(MESASWRSETUPDIR)profiled/ss_triangle.o

