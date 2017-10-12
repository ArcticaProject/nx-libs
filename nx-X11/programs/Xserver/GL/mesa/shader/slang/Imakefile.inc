MESASLANGBUILDDIR=MesaSlangBuildDir

 MESA_SLANG_SRCS = $(MESASLANGBUILDDIR)slang_compile.c \
                   $(MESASLANGBUILDDIR)slang_compile_function.c \
                   $(MESASLANGBUILDDIR)slang_compile_operation.c \
                   $(MESASLANGBUILDDIR)slang_compile_struct.c \
                   $(MESASLANGBUILDDIR)slang_compile_variable.c \
                   $(MESASLANGBUILDDIR)slang_library_noise.c \
                   $(MESASLANGBUILDDIR)slang_preprocess.c \
                   $(MESASLANGBUILDDIR)slang_storage.c \
                   $(MESASLANGBUILDDIR)slang_typeinfo.c \
                   $(MESASLANGBUILDDIR)slang_utility.c

#ifdef NeedToLinkMesaSrc
LinkSourceFile(slang_compile.c, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_compile.h, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_compile_function.c, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_compile_function.h, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_compile_operation.c, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_compile_operation.h, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_compile_struct.c, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_compile_struct.h, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_compile_variable.c, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_compile_variable.h, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_library_noise.c, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_library_noise.h, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_preprocess.c, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_preprocess.h, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_storage.c, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_storage.h, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_typeinfo.c, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_typeinfo.h, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_utility.c, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_utility.h, $(MESASRCDIR)/src/mesa/shader/slang)
#endif

 MESA_SLANG_OBJS = $(MESASLANGBUILDDIR)slang_compile.o \
                   $(MESASLANGBUILDDIR)slang_compile_function.o \
                   $(MESASLANGBUILDDIR)slang_compile_operation.o \
                   $(MESASLANGBUILDDIR)slang_compile_struct.o \
                   $(MESASLANGBUILDDIR)slang_compile_variable.o \
                   $(MESASLANGBUILDDIR)slang_library_noise.o \
                   $(MESASLANGBUILDDIR)slang_link.o \
                   $(MESASLANGBUILDDIR)slang_preprocess.o \
                   $(MESASLANGBUILDDIR)slang_storage.o \
                   $(MESASLANGBUILDDIR)slang_typeinfo.o \
                   $(MESASLANGBUILDDIR)slang_utility.o

#if defined(DoSharedLib) && DoSharedLib
 MESA_SLANG_UOBJS = $(MESASLANGBUILDDIR)unshared/slang_compile.o \
                    $(MESASLANGBUILDDIR)unshared/slang_compile_function.o \
                    $(MESASLANGBUILDDIR)unshared/slang_compile_operation.o \
                    $(MESASLANGBUILDDIR)unshared/slang_compile_struct.o \
                    $(MESASLANGBUILDDIR)unshared/slang_compile_variable.o \
                    $(MESASLANGBUILDDIR)unshared/slang_library_noise.o \
                    $(MESASLANGBUILDDIR)unshared/slang_link.o \
                    $(MESASLANGBUILDDIR)unshared/slang_preprocess.o \
                    $(MESASLANGBUILDDIR)unshared/slang_storage.o \
                    $(MESASLANGBUILDDIR)unshared/slang_typeinfo.o \
                    $(MESASLANGBUILDDIR)unshared/slang_utility.o
#else
 MESA_SLANG_UOBJS = $(MESA_SLANG_OBJS)
#endif

 MESA_SLANG_UOBJS = $(MESASLANGBUILDDIR)debugger/slang_compile.o \
                    $(MESASLANGBUILDDIR)debugger/slang_compile_function.o \
                    $(MESASLANGBUILDDIR)debugger/slang_compile_operation.o \
                    $(MESASLANGBUILDDIR)debugger/slang_compile_struct.o \
                    $(MESASLANGBUILDDIR)debugger/slang_compile_variable.o \
                    $(MESASLANGBUILDDIR)debugger/slang_library_noise.o \
                    $(MESASLANGBUILDDIR)debugger/slang_link.o \
                    $(MESASLANGBUILDDIR)debugger/slang_preprocess.o \
                    $(MESASLANGBUILDDIR)debugger/slang_storage.o \
                    $(MESASLANGBUILDDIR)debugger/slang_typeinfo.o \
                    $(MESASLANGBUILDDIR)debugger/slang_utility.o

 MESA_SLANG_POBJS = $(MESASLANGBUILDDIR)profiled/slang_compile.o \
                    $(MESASLANGBUILDDIR)profiled/slang_compile_function.o \
                    $(MESASLANGBUILDDIR)profiled/slang_compile_operation.o \
                    $(MESASLANGBUILDDIR)profiled/slang_compile_struct.o \
                    $(MESASLANGBUILDDIR)profiled/slang_compile_variable.o \
                    $(MESASLANGBUILDDIR)profiled/slang_library_noise.o \
                    $(MESASLANGBUILDDIR)profiled/slang_link.o \
                    $(MESASLANGBUILDDIR)profiled/slang_preprocess.o \
                    $(MESASLANGBUILDDIR)profiled/slang_storage.o \
                    $(MESASLANGBUILDDIR)profiled/slang_typeinfo.o \
                    $(MESASLANGBUILDDIR)profiled/slang_utility.o
