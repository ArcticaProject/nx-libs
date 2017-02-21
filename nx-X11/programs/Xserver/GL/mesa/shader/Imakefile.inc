MESASHADERBUILDDIR = MesaShaderBuildDir

  MESA_SHADER_SRCS =	$(MESASHADERBUILDDIR)arbprogparse.c \
			$(MESASHADERBUILDDIR)arbprogram.c \
			$(MESASHADERBUILDDIR)arbfragparse.c \
			$(MESASHADERBUILDDIR)arbvertparse.c \
                        $(MESASHADERBUILDDIR)atifragshader.c \
			$(MESASHADERBUILDDIR)nvprogram.c \
			$(MESASHADERBUILDDIR)nvfragparse.c \
			$(MESASHADERBUILDDIR)nvvertexec.c \
			$(MESASHADERBUILDDIR)nvvertparse.c \
			$(MESASHADERBUILDDIR)program.c \
                        $(MESASHADERBUILDDIR)shaderobjects.c \
                        $(MESASHADERBUILDDIR)shaderobjects_3dlabs.c

#ifdef NeedToLinkMesaSrc
LinkSourceFile(arbprogparse.c, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(arbprogram.c, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(arbfragparse.c, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(arbvertparse.c, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(arbfragparse.h, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(arbprogparse.h, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(arbprogram.h, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(arbprogram_syn.h, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(arbvertparse.h, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(atifragshader.c, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(atifragshader.h, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(nvfragparse.h, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(nvfragprog.h, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(nvprogram.c, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(nvprogram.h, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(nvfragparse.c, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(nvvertexec.c, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(nvvertexec.h, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(nvvertparse.c, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(nvvertparse.h, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(nvvertprog.h, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(program.c, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(program.h, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(shaderobjects.c, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(shaderobjects.h, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(shaderobjects_3dlabs.c, $(MESASRCDIR)/src/mesa/shader)
LinkSourceFile(shaderobjects_3dlabs.h, $(MESASRCDIR)/src/mesa/shader)
#endif

  MESA_SHADER_OBJS =	$(MESASHADERBUILDDIR)arbprogparse.o \
			$(MESASHADERBUILDDIR)arbprogram.o \
			$(MESASHADERBUILDDIR)arbfragparse.o \
			$(MESASHADERBUILDDIR)arbvertparse.o \
                        $(MESASHADERBUILDDIR)atifragshader.o \
			$(MESASHADERBUILDDIR)nvprogram.o \
			$(MESASHADERBUILDDIR)nvfragparse.o \
			$(MESASHADERBUILDDIR)nvvertexec.o \
			$(MESASHADERBUILDDIR)nvvertparse.o \
			$(MESASHADERBUILDDIR)program.o \
                        $(MESASHADERBUILDDIR)shaderobjects.o \
                        $(MESASHADERBUILDDIR)shaderobjects_3dlabs.o
