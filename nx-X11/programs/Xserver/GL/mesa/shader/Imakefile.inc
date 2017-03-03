MESASHADERBUILDDIR = MesaShaderBuildDir

  MESA_SHADER_SRCS =						\
                     $(MESASHADERBUILDDIR)arbprogparse.c	\
                     $(MESASHADERBUILDDIR)arbprogram.c		\
                     $(MESASHADERBUILDDIR)atifragshader.c	\
                     $(MESASHADERBUILDDIR)nvfragparse.c		\
                     $(MESASHADERBUILDDIR)nvprogram.c		\
                     $(MESASHADERBUILDDIR)nvvertparse.c		\
                     $(MESASHADERBUILDDIR)prog_debug.c		\
                     $(MESASHADERBUILDDIR)prog_execute.c	\
                     $(MESASHADERBUILDDIR)prog_instruction.c	\
                     $(MESASHADERBUILDDIR)prog_parameter.c	\
                     $(MESASHADERBUILDDIR)prog_print.c		\
                     $(MESASHADERBUILDDIR)prog_statevars.c	\
                     $(MESASHADERBUILDDIR)program.c		\
                     $(MESASHADERBUILDDIR)programopt.c		\
                     $(MESASHADERBUILDDIR)shader_api.c		\
                     $(NULL)

LinkSourceFile(arbprogparse.c, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(arbprogparse.h, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(arbprogram.c, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(arbprogram.h, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(arbprogram_syn.h, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(atifragshader.c, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(atifragshader.h, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(nvfragparse.c, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(nvfragparse.h, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(nvprogram.c, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(nvprogram.h, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(nvvertparse.c, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(nvvertparse.h, $(MESASRCDIR)/src/mesa/shader)
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
LinkSourceFile(prog_statevars.c, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(prog_statevars.h, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(program.c, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(program.h, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(programopt.c, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(programopt.h, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(shader_api.c, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(shader_api.h, $(MESASRCDIR)/src/mesa/shader)

  MESA_SHADER_OBJS =						\
                     $(MESASHADERBUILDDIR)arbprogparse.o	\
                     $(MESASHADERBUILDDIR)arbprogram.o		\
                     $(MESASHADERBUILDDIR)atifragshader.o	\
                     $(MESASHADERBUILDDIR)nvfragparse.o		\
                     $(MESASHADERBUILDDIR)nvprogram.o		\
                     $(MESASHADERBUILDDIR)nvvertparse.o		\
                     $(MESASHADERBUILDDIR)prog_debug.o		\
                     $(MESASHADERBUILDDIR)prog_execute.o	\
                     $(MESASHADERBUILDDIR)prog_instruction.o	\
                     $(MESASHADERBUILDDIR)prog_parameter.o	\
                     $(MESASHADERBUILDDIR)prog_print.o		\
                     $(MESASHADERBUILDDIR)prog_statevars.o	\
                     $(MESASHADERBUILDDIR)program.o		\
                     $(MESASHADERBUILDDIR)programopt.o		\
                     $(MESASHADERBUILDDIR)shader_api.o		\
                     $(NULL)
