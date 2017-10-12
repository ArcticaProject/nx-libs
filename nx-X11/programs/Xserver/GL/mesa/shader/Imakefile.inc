MESASHADERBUILDDIR = MesaShaderBuildDir

  MESA_SHADER_SRCS =	$(MESASHADERBUILDDIR)arbprogparse.c \
			$(MESASHADERBUILDDIR)arbprogram.c \
                        $(MESASHADERBUILDDIR)atifragshader.c \
			$(MESASHADERBUILDDIR)nvprogram.c \
			$(MESASHADERBUILDDIR)nvfragparse.c \
			$(MESASHADERBUILDDIR)nvvertparse.c \
			$(MESASHADERBUILDDIR)program.c

#ifdef NeedToLinkMesaSrc
LinkSourceFile(arbprogparse.c, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(arbprogram.c, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(arbprogparse.h, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(arbprogram.h, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(arbprogram_syn.h, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(atifragshader.c, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(atifragshader.h, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(nvfragparse.h, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(nvprogram.c, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(nvprogram.h, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(nvfragparse.c, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(nvvertparse.c, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(nvvertparse.h, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(program.c, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(program.h, $(MESASRCDIR)/src/mesa/shader)
#endif

  MESA_SHADER_OBJS =	$(MESASHADERBUILDDIR)arbprogparse.o \
			$(MESASHADERBUILDDIR)arbprogram.o \
                        $(MESASHADERBUILDDIR)atifragshader.o \
			$(MESASHADERBUILDDIR)nvprogram.o \
			$(MESASHADERBUILDDIR)nvfragparse.o \
			$(MESASHADERBUILDDIR)nvvertparse.o \
			$(MESASHADERBUILDDIR)program.o

#if defined(DoSharedLib) && DoSharedLib
  MESA_SHADER_UOBJS =	$(MESASHADERBUILDDIR)unshared/arbprogparse.o \
			$(MESASHADERBUILDDIR)unshared/arbprogram.o \
                        $(MESASHADERBUILDDIR)unshared/atifragshader.o \
			$(MESASHADERBUILDDIR)unshared/nvprogram.o \
			$(MESASHADERBUILDDIR)unshared/nvfragparse.o \
			$(MESASHADERBUILDDIR)unshared/nvvertparse.o \
			$(MESASHADERBUILDDIR)unshared/program.o
#else
 MESA_SHADER_UOBJS = $(MESA_SHADER_OBJS)
#endif

  MESA_SHADER_DOBJS =	$(MESASHADERBUILDDIR)debugger/arbprogparse.o \
			$(MESASHADERBUILDDIR)debugger/arbprogram.o \
                        $(MESASHADERBUILDDIR)debugger/atifragshader.o \
			$(MESASHADERBUILDDIR)debugger/nvprogram.o \
			$(MESASHADERBUILDDIR)debugger/nvfragparse.o \
			$(MESASHADERBUILDDIR)debugger/nvvertparse.o \
			$(MESASHADERBUILDDIR)debugger/program.o

  MESA_SHADER_POBJS =	$(MESASHADERBUILDDIR)profiled/arbprogparse.o \
			$(MESASHADERBUILDDIR)profiled/arbprogram.o \
                        $(MESASHADERBUILDDIR)profiled/atifragshader.o \
			$(MESASHADERBUILDDIR)profiled/nvprogram.o \
			$(MESASHADERBUILDDIR)profiled/nvfragparse.o \
			$(MESASHADERBUILDDIR)profiled/nvvertparse.o \
			$(MESASHADERBUILDDIR)profiled/program.o
