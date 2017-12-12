MESASHADERBUILDDIR = MesaShaderBuildDir

  MESA_SHADER_SRCS =	$(MESASHADERBUILDDIR)arbprogparse.c \
			$(MESASHADERBUILDDIR)arbprogram.c \
			$(MESASHADERBUILDDIR)atifragshader.c \
			$(MESASHADERBUILDDIR)hash_table.c \
			$(MESASHADERBUILDDIR)lex.yy.c \
			$(MESASHADERBUILDDIR)nvfragparse.c \
			$(MESASHADERBUILDDIR)nvprogram.c \
			$(MESASHADERBUILDDIR)nvvertparse.c \
			$(MESASHADERBUILDDIR)prog_cache.c \
			$(MESASHADERBUILDDIR)prog_execute.c \
			$(MESASHADERBUILDDIR)prog_instruction.c \
			$(MESASHADERBUILDDIR)prog_noise.c \
			$(MESASHADERBUILDDIR)prog_optimize.c \
			$(MESASHADERBUILDDIR)prog_parameter.c \
			$(MESASHADERBUILDDIR)prog_parameter_layout.c \
			$(MESASHADERBUILDDIR)prog_print.c \
			$(MESASHADERBUILDDIR)prog_statevars.c \
			$(MESASHADERBUILDDIR)prog_uniform.c \
			$(MESASHADERBUILDDIR)program.c \
			$(MESASHADERBUILDDIR)program_parse.tab.c \
			$(MESASHADERBUILDDIR)program_parse_extra.c \
			$(MESASHADERBUILDDIR)programopt.c \
			$(MESASHADERBUILDDIR)shader_api.c \
			$(MESASHADERBUILDDIR)symbol_table.c

#ifdef NeedToLinkMesaSrc
LinkSourceFile(arbprogparse.c, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(arbprogparse.h, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(arbprogram.c, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(arbprogram.h, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(atifragshader.c, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(atifragshader.h, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(hash_table.c, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(lex.yy.c, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(nvfragparse.c, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(nvfragparse.h, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(nvprogram.c, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(nvprogram.h, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(nvvertparse.c, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(nvvertparse.h, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(prog_cache.c, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(prog_cache.h, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(prog_execute.c, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(prog_execute.h, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(prog_instruction.c, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(prog_instruction.h, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(prog_noise.c, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(prog_optimize.c, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(prog_parameter.c, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(prog_parameter.h, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(prog_parameter_layout.c, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(prog_print.c, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(prog_print.h, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(prog_statevars.c, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(prog_statevars.h, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(prog_uniform.c, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(prog_uniform.h, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(program.c, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(program.h, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(program_parse.tab.c, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(program_parse_extra.c, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(programopt.c, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(programopt.h, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(shader_api.c, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(shader_api.h, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(symbol_table.c, $(MESASRCDIR)/src/mesa/shader)
#endif

  MESA_SHADER_OBJS =	$(MESASHADERBUILDDIR)arbprogparse.o \
			$(MESASHADERBUILDDIR)arbprogram.o \
			$(MESASHADERBUILDDIR)atifragshader.o \
			$(MESASHADERBUILDDIR)hash_table.o \
			$(MESASHADERBUILDDIR)lex.yy.o \
			$(MESASHADERBUILDDIR)nvfragparse.o \
			$(MESASHADERBUILDDIR)nvprogram.o \
			$(MESASHADERBUILDDIR)nvvertparse.o \
			$(MESASHADERBUILDDIR)prog_cache.o \
			$(MESASHADERBUILDDIR)prog_execute.o \
			$(MESASHADERBUILDDIR)prog_instruction.o \
			$(MESASHADERBUILDDIR)prog_noise.o \
			$(MESASHADERBUILDDIR)prog_optimize.o \
			$(MESASHADERBUILDDIR)prog_parameter.o \
			$(MESASHADERBUILDDIR)prog_parameter_layout.o \
			$(MESASHADERBUILDDIR)prog_print.o \
			$(MESASHADERBUILDDIR)prog_statevars.o \
			$(MESASHADERBUILDDIR)prog_uniform.o \
			$(MESASHADERBUILDDIR)program.o \
			$(MESASHADERBUILDDIR)program_parse.tab.o \
			$(MESASHADERBUILDDIR)program_parse_extra.o \
			$(MESASHADERBUILDDIR)programopt.o \
			$(MESASHADERBUILDDIR)shader_api.o \
			$(MESASHADERBUILDDIR)symbol_table.o

#if defined(DoSharedLib) && DoSharedLib
  MESA_SHADER_UOBJS =	$(MESASHADERBUILDDIR)unshared/arbprogparse.o \
			$(MESASHADERBUILDDIR)unshared/arbprogram.o \
			$(MESASHADERBUILDDIR)unshared/atifragshader.o
			$(MESASHADERBUILDDIR)unshared/hash_table.o \
			$(MESASHADERBUILDDIR)unshared/lex.yy.o \
			$(MESASHADERBUILDDIR)unshared/nvfragparse.o \
			$(MESASHADERBUILDDIR)unshared/nvprogram.o \
			$(MESASHADERBUILDDIR)unshared/nvvertparse.o \
			$(MESASHADERBUILDDIR)unshared/prog_cache.o \
			$(MESASHADERBUILDDIR)unshared/prog_execute.o \
			$(MESASHADERBUILDDIR)unshared/prog_instruction.o \
			$(MESASHADERBUILDDIR)unshared/prog_noise.o \
			$(MESASHADERBUILDDIR)unshared/prog_optimize.o \
			$(MESASHADERBUILDDIR)unshared/prog_parameter.o \
			$(MESASHADERBUILDDIR)unshared/prog_parameter_layout.o \
			$(MESASHADERBUILDDIR)unshared/prog_print.o \
			$(MESASHADERBUILDDIR)unshared/prog_statevars.o \
			$(MESASHADERBUILDDIR)unshared/prog_uniform.o \
			$(MESASHADERBUILDDIR)unshared/program.o \
			$(MESASHADERBUILDDIR)unshared/program_parse.tab.o \
			$(MESASHADERBUILDDIR)unshared/program_parse_extra.o \
			$(MESASHADERBUILDDIR)unshared/programopt.o \
			$(MESASHADERBUILDDIR)unshared/shader_api.o \
			$(MESASHADERBUILDDIR)unshared/symbol_table.o
#else
  MESA_SHADER_UOBJS = $(MESA_SHADER_OBJS)
#endif

  MESA_SHADER_DOBJS =	$(MESASHADERBUILDDIR)debugger/arbprogparse.o \
			$(MESASHADERBUILDDIR)debugger/arbprogram.o \
			$(MESASHADERBUILDDIR)debugger/atifragshader.o \
			$(MESASHADERBUILDDIR)debugger/hash_table.o \
			$(MESASHADERBUILDDIR)debugger/lex.yy.o \
			$(MESASHADERBUILDDIR)debugger/nvfragparse.o \
			$(MESASHADERBUILDDIR)debugger/nvprogram.o \
			$(MESASHADERBUILDDIR)debugger/nvvertparse.o \
			$(MESASHADERBUILDDIR)debugger/prog_cache.o \
			$(MESASHADERBUILDDIR)debugger/prog_execute.o \
			$(MESASHADERBUILDDIR)debugger/prog_instruction.o \
			$(MESASHADERBUILDDIR)debugger/prog_noise.o \
			$(MESASHADERBUILDDIR)debugger/prog_optimize.o \
			$(MESASHADERBUILDDIR)debugger/prog_parameter.o \
			$(MESASHADERBUILDDIR)debugger/prog_parameter_layout.o \
			$(MESASHADERBUILDDIR)debugger/prog_print.o \
			$(MESASHADERBUILDDIR)debugger/prog_statevars.o \
			$(MESASHADERBUILDDIR)debugger/prog_uniform.o \
			$(MESASHADERBUILDDIR)debugger/program.o \
			$(MESASHADERBUILDDIR)debugger/program_parse.tab.o \
			$(MESASHADERBUILDDIR)debugger/program_parse_extra.o \
			$(MESASHADERBUILDDIR)debugger/programopt.o \
			$(MESASHADERBUILDDIR)debugger/shader_api.o \
			$(MESASHADERBUILDDIR)debugger/symbol_table.o

  MESA_SHADER_POBJS =	$(MESASHADERBUILDDIR)profiled/arbprogparse.o \
			$(MESASHADERBUILDDIR)profiled/arbprogram.o \
			$(MESASHADERBUILDDIR)profiled/atifragshader.o \
			$(MESASHADERBUILDDIR)profiled/hash_table.o \
			$(MESASHADERBUILDDIR)profiled/lex.yy.o \
			$(MESASHADERBUILDDIR)profiled/nvfragparse.o \
			$(MESASHADERBUILDDIR)profiled/nvprogram.o \
			$(MESASHADERBUILDDIR)profiled/nvvertparse.o \
			$(MESASHADERBUILDDIR)profiled/prog_cache.o \
			$(MESASHADERBUILDDIR)profiled/prog_execute.o \
			$(MESASHADERBUILDDIR)profiled/prog_instruction.o \
			$(MESASHADERBUILDDIR)profiled/prog_noise.o \
			$(MESASHADERBUILDDIR)profiled/prog_optimize.o \
			$(MESASHADERBUILDDIR)profiled/prog_parameter.o \
			$(MESASHADERBUILDDIR)profiled/prog_parameter_layout.o \
			$(MESASHADERBUILDDIR)profiled/prog_print.o \
			$(MESASHADERBUILDDIR)profiled/prog_statevars.o \
			$(MESASHADERBUILDDIR)profiled/prog_uniform.o \
			$(MESASHADERBUILDDIR)profiled/program.o \
			$(MESASHADERBUILDDIR)profiled/program_parse.tab.o \
			$(MESASHADERBUILDDIR)profiled/program_parse_extra.o \
			$(MESASHADERBUILDDIR)profiled/programopt.o \
			$(MESASHADERBUILDDIR)profiled/shader_api.o \
			$(MESASHADERBUILDDIR)profiled/symbol_table.o
