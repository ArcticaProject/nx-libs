MESASLANGBUILDDIR=MesaSlangBuildDir

  MESA_SLANG_SRCS =	$(MESASLANGBUILDDIR)slang_assemble_assignment.c \
                        $(MESASLANGBUILDDIR)slang_assemble.c \
                        $(MESASLANGBUILDDIR)slang_assemble_conditional.c \
                        $(MESASLANGBUILDDIR)slang_assemble_constructor.c \
                        $(MESASLANGBUILDDIR)slang_assemble_typeinfo.c \
                        $(MESASLANGBUILDDIR)slang_compile.c \
                        $(MESASLANGBUILDDIR)slang_execute.c \
                        $(MESASLANGBUILDDIR)slang_preprocess.c \
                        $(MESASLANGBUILDDIR)slang_storage.c \
                        $(MESASLANGBUILDDIR)slang_utility.c

#ifdef NeedToLinkMesaSrc
LinkSourceFile(slang_assemble_assignment.c, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_assemble_assignment.h, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_assemble.c, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_assemble.h, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_assemble_conditional.c, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_assemble_conditional.h, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_assemble_constructor.c, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_assemble_constructor.h, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_assemble_typeinfo.c, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_assemble_typeinfo.h, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_compile.c, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_compile.h, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_execute.c, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_execute.h, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_preprocess.c, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_preprocess.h, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_storage.c, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_storage.h, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_utility.c, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_utility.h, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(traverse_wrap.h, $(MESASRCDIR)/src/mesa/shader/slang)
#endif

  MESA_SLANG_OBJS =	$(MESASLANGBUILDDIR)slang_assemble_assignment.o \
                        $(MESASLANGBUILDDIR)slang_assemble.o \
                        $(MESASLANGBUILDDIR)slang_assemble_conditional.o \
                        $(MESASLANGBUILDDIR)slang_assemble_constructor.o \
                        $(MESASLANGBUILDDIR)slang_assemble_typeinfo.o \
                        $(MESASLANGBUILDDIR)slang_compile.o \
                        $(MESASLANGBUILDDIR)slang_execute.o \
                        $(MESASLANGBUILDDIR)slang_preprocess.o \
                        $(MESASLANGBUILDDIR)slang_storage.o \
                        $(MESASLANGBUILDDIR)slang_utility.o
