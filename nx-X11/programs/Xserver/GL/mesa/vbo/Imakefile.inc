MESAVBOBUILDDIR = MesaVboBuildDir

  MESA_VBO_SRCS = $(MESAVBOBUILDDIR)vbo_context.c \
		  $(MESAVBOBUILDDIR)vbo_exec_api.c \
		  $(MESAVBOBUILDDIR)vbo_exec_array.c \
		  $(MESAVBOBUILDDIR)vbo_exec.c \
		  $(MESAVBOBUILDDIR)vbo_exec_draw.c \
		  $(MESAVBOBUILDDIR)vbo_exec_eval.c \
		  $(MESAVBOBUILDDIR)vbo_rebase.c \
		  $(MESAVBOBUILDDIR)vbo_save_api.c \
		  $(MESAVBOBUILDDIR)vbo_save.c \
		  $(MESAVBOBUILDDIR)vbo_save_draw.c \
		  $(MESAVBOBUILDDIR)vbo_save_loopback.c \
		  $(MESAVBOBUILDDIR)vbo_split.c \
		  $(MESAVBOBUILDDIR)vbo_split_copy.c \
		  $(MESAVBOBUILDDIR)vbo_split_inplace.c

#ifdef NeedToLinkMesaSrc
LinkSourceFile(vbo_attrib.h, $(MESASRCDIR)/src/mesa/vbo)
LinkSourceFile(vbo_attrib_tmp.h, $(MESASRCDIR)/src/mesa/vbo)
LinkSourceFile(vbo_context.c, $(MESASRCDIR)/src/mesa/vbo)
LinkSourceFile(vbo_context.h, $(MESASRCDIR)/src/mesa/vbo)
LinkSourceFile(vbo_exec_api.c, $(MESASRCDIR)/src/mesa/vbo)
LinkSourceFile(vbo_exec_array.c, $(MESASRCDIR)/src/mesa/vbo)
LinkSourceFile(vbo_exec.c, $(MESASRCDIR)/src/mesa/vbo)
LinkSourceFile(vbo_exec_draw.c, $(MESASRCDIR)/src/mesa/vbo)
LinkSourceFile(vbo_exec_eval.c, $(MESASRCDIR)/src/mesa/vbo)
LinkSourceFile(vbo_exec.h, $(MESASRCDIR)/src/mesa/vbo)
LinkSourceFile(vbo.h, $(MESASRCDIR)/src/mesa/vbo)
LinkSourceFile(vbo_rebase.c, $(MESASRCDIR)/src/mesa/vbo)
LinkSourceFile(vbo_save_api.c, $(MESASRCDIR)/src/mesa/vbo)
LinkSourceFile(vbo_save.c, $(MESASRCDIR)/src/mesa/vbo)
LinkSourceFile(vbo_save_draw.c, $(MESASRCDIR)/src/mesa/vbo)
LinkSourceFile(vbo_save.h, $(MESASRCDIR)/src/mesa/vbo)
LinkSourceFile(vbo_save_loopback.c, $(MESASRCDIR)/src/mesa/vbo)
LinkSourceFile(vbo_split.c, $(MESASRCDIR)/src/mesa/vbo)
LinkSourceFile(vbo_split_copy.c, $(MESASRCDIR)/src/mesa/vbo)
LinkSourceFile(vbo_split.h, $(MESASRCDIR)/src/mesa/vbo)
LinkSourceFile(vbo_split_inplace.c, $(MESASRCDIR)/src/mesa/vbo)
#endif

  MESA_VBO_OBJS = $(MESAVBOBUILDDIR)vbo_context.o \
		  $(MESAVBOBUILDDIR)vbo_exec_api.o \
		  $(MESAVBOBUILDDIR)vbo_exec_array.o \
		  $(MESAVBOBUILDDIR)vbo_exec.o \
		  $(MESAVBOBUILDDIR)vbo_exec_draw.o \
		  $(MESAVBOBUILDDIR)vbo_exec_eval.o \
		  $(MESAVBOBUILDDIR)vbo_rebase.o \
		  $(MESAVBOBUILDDIR)vbo_save_api.o \
		  $(MESAVBOBUILDDIR)vbo_save.o \
		  $(MESAVBOBUILDDIR)vbo_save_draw.o \
		  $(MESAVBOBUILDDIR)vbo_save_loopback.o \
		  $(MESAVBOBUILDDIR)vbo_split.o \
		  $(MESAVBOBUILDDIR)vbo_split_copy.o \
		  $(MESAVBOBUILDDIR)vbo_split_inplace.o

#if defined(DoSharedLib) && DoSharedLib
 MESA_VBO_UOBJS = $(MESAVBOBUILDDIR)unshared/vbo_context.o \
		  $(MESAVBOBUILDDIR)unshared/vbo_exec_api.o \
		  $(MESAVBOBUILDDIR)unshared/vbo_exec_array.o \
		  $(MESAVBOBUILDDIR)unshared/vbo_exec.o \
		  $(MESAVBOBUILDDIR)unshared/vbo_exec_draw.o \
		  $(MESAVBOBUILDDIR)unshared/vbo_exec_eval.o \
		  $(MESAVBOBUILDDIR)unshared/vbo_rebase.o \
		  $(MESAVBOBUILDDIR)unshared/vbo_save_api.o \
		  $(MESAVBOBUILDDIR)unshared/vbo_save.o \
		  $(MESAVBOBUILDDIR)unshared/vbo_save_draw.o \
		  $(MESAVBOBUILDDIR)unshared/vbo_save_loopback.o \
		  $(MESAVBOBUILDDIR)unshared/vbo_split.o \
		  $(MESAVBOBUILDDIR)unshared/vbo_split_copy.o \
		  $(MESAVBOBUILDDIR)unshared/vbo_split_inplace.o
#else
 MESA_VBO_UOBJS = $(MESA_VBO_OBJS)
#endif

 MESA_VBO_DOBJS = $(MESAVBOBUILDDIR)debugger/vbo_context.o \
		  $(MESAVBOBUILDDIR)debugger/vbo_exec_api.o \
		  $(MESAVBOBUILDDIR)debugger/vbo_exec_array.o \
		  $(MESAVBOBUILDDIR)debugger/vbo_exec.o \
		  $(MESAVBOBUILDDIR)debugger/vbo_exec_draw.o \
		  $(MESAVBOBUILDDIR)debugger/vbo_exec_eval.o \
		  $(MESAVBOBUILDDIR)debugger/vbo_rebase.o \
		  $(MESAVBOBUILDDIR)debugger/vbo_save_api.o \
		  $(MESAVBOBUILDDIR)debugger/vbo_save.o \
		  $(MESAVBOBUILDDIR)debugger/vbo_save_draw.o \
		  $(MESAVBOBUILDDIR)debugger/vbo_save_loopback.o \
		  $(MESAVBOBUILDDIR)debugger/vbo_split.o \
		  $(MESAVBOBUILDDIR)debugger/vbo_split_copy.o \
		  $(MESAVBOBUILDDIR)debugger/vbo_split_inplace.o

 MESA_VBO_POBJS = $(MESAVBOBUILDDIR)profiled/vbo_context.o \
		  $(MESAVBOBUILDDIR)profiled/vbo_exec_api.o \
		  $(MESAVBOBUILDDIR)profiled/vbo_exec_array.o \
		  $(MESAVBOBUILDDIR)profiled/vbo_exec.o \
		  $(MESAVBOBUILDDIR)profiled/vbo_exec_draw.o \
		  $(MESAVBOBUILDDIR)profiled/vbo_exec_eval.o \
		  $(MESAVBOBUILDDIR)profiled/vbo_rebase.o \
		  $(MESAVBOBUILDDIR)profiled/vbo_save_api.o \
		  $(MESAVBOBUILDDIR)profiled/vbo_save.o \
		  $(MESAVBOBUILDDIR)profiled/vbo_save_draw.o \
		  $(MESAVBOBUILDDIR)profiled/vbo_save_loopback.o \
		  $(MESAVBOBUILDDIR)profiled/vbo_split.o \
		  $(MESAVBOBUILDDIR)profiled/vbo_split_copy.o \
		  $(MESAVBOBUILDDIR)profiled/vbo_split_inplace.o
