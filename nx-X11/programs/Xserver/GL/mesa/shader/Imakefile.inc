MESASHADERBUILDDIR = MesaShaderBuildDir

  MESA_SHADER_SRCS =	$(MESASHADERBUILDDIR)arbprogparse.c \
			$(MESASHADERBUILDDIR)arbprogram.c \
                        $(MESASHADERBUILDDIR)atifragshader.c \
			$(MESASHADERBUILDDIR)nvprogram.c \
			$(MESASHADERBUILDDIR)nvfragparse.c \
			$(MESASHADERBUILDDIR)nvvertparse.c \
			$(MESASHADERBUILDDIR)shader_api.c \
			$(MESASHADERBUILDDIR)prog_debug.c \
			$(MESASHADERBUILDDIR)prog_execute.c \
			$(MESASHADERBUILDDIR)prog_instruction.c \
			$(MESASHADERBUILDDIR)prog_parameter.c \
			$(MESASHADERBUILDDIR)prog_print.c \
			$(MESASHADERBUILDDIR)prog_uniform.c \
			$(MESASHADERBUILDDIR)program.c \
			$(MESASHADERBUILDDIR)programopt.c \
			$(MESASHADERBUILDDIR)prog_statevars.c

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
LinkSourceFile(shader_api.c, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(shader_api.h, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(prog_debug.c, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(prog_debug.h, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(prog_execute.c, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(prog_execute.h, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(prog_instruction.c, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(prog_instruction.h, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(prog_parameter.c, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(prog_parameter.h, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(prog_print.c, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(prog_print.h, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(prog_uniform.c, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(prog_uniform.h, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(programopt.c, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(programopt.h, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(prog_statevars.c, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(prog_statevars.h, $(MESASRCDIR)/src/mesa/shader)
#endif

  MESA_SHADER_OBJS =	$(MESASHADERBUILDDIR)arbprogparse.o \
			$(MESASHADERBUILDDIR)arbprogram.o \
                        $(MESASHADERBUILDDIR)atifragshader.o \
			$(MESASHADERBUILDDIR)nvprogram.o \
			$(MESASHADERBUILDDIR)nvfragparse.o \
			$(MESASHADERBUILDDIR)nvvertparse.o \
			$(MESASHADERBUILDDIR)shader_api.o \
			$(MESASHADERBUILDDIR)prog_debug.o \
			$(MESASHADERBUILDDIR)prog_execute.o \
			$(MESASHADERBUILDDIR)prog_instruction.o \
			$(MESASHADERBUILDDIR)prog_parameter.o \
			$(MESASHADERBUILDDIR)prog_print.o \
			$(MESASHADERBUILDDIR)prog_uniform.o \
			$(MESASHADERBUILDDIR)program.o \
			$(MESASHADERBUILDDIR)programopt.o \
			$(MESASHADERBUILDDIR)prog_statevars.o

#if defined(DoSharedLib) && DoSharedLib
  MESA_SHADER_UOBJS =	$(MESASHADERBUILDDIR)unshared/arbprogparse.o \
			$(MESASHADERBUILDDIR)unshared/arbprogram.o \
                        $(MESASHADERBUILDDIR)unshared/atifragshader.o \
			$(MESASHADERBUILDDIR)unshared/nvprogram.o \
			$(MESASHADERBUILDDIR)unshared/nvfragparse.o \
			$(MESASHADERBUILDDIR)unshared/nvvertparse.o \
			$(MESASHADERBUILDDIR)unshared/shader_api.o \
			$(MESASHADERBUILDDIR)unshared/prog_debug.o \
			$(MESASHADERBUILDDIR)unshared/prog_execute.o \
			$(MESASHADERBUILDDIR)unshared/prog_instruction.o \
			$(MESASHADERBUILDDIR)unshared/prog_parameter.o \
			$(MESASHADERBUILDDIR)unshared/prog_print.o \
			$(MESASHADERBUILDDIR)unshared/prog_uniform.o \
			$(MESASHADERBUILDDIR)unshared/program.o \
			$(MESASHADERBUILDDIR)unshared/programopt.o \
			$(MESASHADERBUILDDIR)unshared/prog_statevars.o
#else
 MESA_SHADER_UOBJS = $(MESA_SHADER_OBJS)
#endif

  MESA_SHADER_DOBJS =	$(MESASHADERBUILDDIR)debugger/arbprogparse.o \
			$(MESASHADERBUILDDIR)debugger/arbprogram.o \
                        $(MESASHADERBUILDDIR)debugger/atifragshader.o \
			$(MESASHADERBUILDDIR)debugger/nvprogram.o \
			$(MESASHADERBUILDDIR)debugger/nvfragparse.o \
			$(MESASHADERBUILDDIR)debugger/nvvertparse.o \
			$(MESASHADERBUILDDIR)debugger/shader_api.o \
			$(MESASHADERBUILDDIR)debugger/prog_debug.o \
			$(MESASHADERBUILDDIR)debugger/prog_execute.o \
			$(MESASHADERBUILDDIR)debugger/prog_instruction.o \
			$(MESASHADERBUILDDIR)debugger/prog_parameter.o \
			$(MESASHADERBUILDDIR)debugger/prog_print.o \
			$(MESASHADERBUILDDIR)debugger/prog_uniform.o \
			$(MESASHADERBUILDDIR)debugger/program.o \
			$(MESASHADERBUILDDIR)debugger/programopt.o \
			$(MESASHADERBUILDDIR)debugger/prog_statevars.o

  MESA_SHADER_POBJS =	$(MESASHADERBUILDDIR)profiled/arbprogparse.o \
			$(MESASHADERBUILDDIR)profiled/arbprogram.o \
                        $(MESASHADERBUILDDIR)profiled/atifragshader.o \
			$(MESASHADERBUILDDIR)profiled/nvprogram.o \
			$(MESASHADERBUILDDIR)profiled/nvfragparse.o \
			$(MESASHADERBUILDDIR)profiled/nvvertparse.o \
			$(MESASHADERBUILDDIR)profiled/shader_api.o \
			$(MESASHADERBUILDDIR)profiled/prog_debug.o \
			$(MESASHADERBUILDDIR)profiled/prog_execute.o \
			$(MESASHADERBUILDDIR)profiled/prog_instruction.o \
			$(MESASHADERBUILDDIR)profiled/prog_parameter.o \
			$(MESASHADERBUILDDIR)profiled/prog_print.o \
			$(MESASHADERBUILDDIR)profiled/prog_uniform.o \
			$(MESASHADERBUILDDIR)profiled/program.o \
			$(MESASHADERBUILDDIR)profiled/programopt.o \
			$(MESASHADERBUILDDIR)profiled/prog_statevars.o
