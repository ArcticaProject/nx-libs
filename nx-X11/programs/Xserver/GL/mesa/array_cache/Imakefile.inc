MESAACBUILDDIR = MesaACBuildDir

  MESA_AC_SRCS = $(MESAACBUILDDIR)ac_context.c \
		 $(MESAACBUILDDIR)ac_import.c

#ifdef NeedToLinkMesaSrc
LinkSourceFile(acache.h, $(MESASRCDIR)/src/mesa/array_cache)
LinkSourceFile(ac_context.c, $(MESASRCDIR)/src/mesa/array_cache)
LinkSourceFile(ac_context.h, $(MESASRCDIR)/src/mesa/array_cache)
LinkSourceFile(ac_import.c, $(MESASRCDIR)/src/mesa/array_cache)
#endif

  MESA_AC_OBJS = $(MESAACBUILDDIR)ac_context.o \
		 $(MESAACBUILDDIR)ac_import.o
