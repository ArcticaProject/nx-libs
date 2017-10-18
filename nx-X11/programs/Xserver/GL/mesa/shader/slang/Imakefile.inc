MESASLANGBUILDDIR=MesaSlangBuildDir

  MESA_SLANG_SRCS = $(MESASLANGBUILDDIR)slang_builtin.c \
                    $(MESASLANGBUILDDIR)slang_codegen.c \
                    $(MESASLANGBUILDDIR)slang_compile.c \
                    $(MESASLANGBUILDDIR)slang_compile_function.c \
                    $(MESASLANGBUILDDIR)slang_compile_operation.c \
                    $(MESASLANGBUILDDIR)slang_compile_struct.c \
                    $(MESASLANGBUILDDIR)slang_compile_variable.c \
                    $(MESASLANGBUILDDIR)slang_emit.c \
                    $(MESASLANGBUILDDIR)slang_ir.c \
                    $(MESASLANGBUILDDIR)slang_label.c \
                    $(MESASLANGBUILDDIR)slang_link.c \
                    $(MESASLANGBUILDDIR)slang_log.c \
                    $(MESASLANGBUILDDIR)slang_mem.c \
                    $(MESASLANGBUILDDIR)slang_preprocess.c \
                    $(MESASLANGBUILDDIR)slang_print.c \
                    $(MESASLANGBUILDDIR)slang_simplify.c \
                    $(MESASLANGBUILDDIR)slang_storage.c \
                    $(MESASLANGBUILDDIR)slang_typeinfo.c \
                    $(MESASLANGBUILDDIR)slang_utility.c \
                    $(MESASLANGBUILDDIR)slang_vartable.c

#ifdef NeedToLinkMesaSrc
LinkSourceFile(slang_builtin.c, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_builtin.h, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_codegen.c, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_codegen.h, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_compile.c, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_compile_function.c, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_compile_function.h, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_compile.h, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_compile_operation.c, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_compile_operation.h, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_compile_struct.c, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_compile_struct.h, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_compile_variable.c, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_compile_variable.h, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_emit.c, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_emit.h, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_ir.c, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_ir.h, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_label.c, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_label.h, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_link.c, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_link.h, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_log.c, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_log.h, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_mem.c, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_mem.h, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_preprocess.c, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_preprocess.h, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_print.c, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_print.h, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_simplify.c, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_simplify.h, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_storage.c, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_storage.h, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_typeinfo.c, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_typeinfo.h, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_utility.c, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_utility.h, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_vartable.c, $(MESASRCDIR)/src/mesa/shader/slang)
LinkSourceFile(slang_vartable.h, $(MESASRCDIR)/src/mesa/shader/slang)
#endif

  MESA_SLANG_OBJS = $(MESASLANGBUILDDIR)slang_builtin.o \
                    $(MESASLANGBUILDDIR)slang_codegen.o \
                    $(MESASLANGBUILDDIR)slang_compile.o \
                    $(MESASLANGBUILDDIR)slang_compile_function.o \
                    $(MESASLANGBUILDDIR)slang_compile_operation.o \
                    $(MESASLANGBUILDDIR)slang_compile_struct.o \
                    $(MESASLANGBUILDDIR)slang_compile_variable.o \
                    $(MESASLANGBUILDDIR)slang_emit.o \
                    $(MESASLANGBUILDDIR)slang_ir.o \
                    $(MESASLANGBUILDDIR)slang_label.o \
                    $(MESASLANGBUILDDIR)slang_link.o \
                    $(MESASLANGBUILDDIR)slang_log.o \
                    $(MESASLANGBUILDDIR)slang_mem.o \
                    $(MESASLANGBUILDDIR)slang_preprocess.o \
                    $(MESASLANGBUILDDIR)slang_print.o \
                    $(MESASLANGBUILDDIR)slang_simplify.o \
                    $(MESASLANGBUILDDIR)slang_storage.o \
                    $(MESASLANGBUILDDIR)slang_typeinfo.o \
                    $(MESASLANGBUILDDIR)slang_utility.o \
                    $(MESASLANGBUILDDIR)slang_vartable.o

#if defined(DoSharedLib) && DoSharedLib
 MESA_SLANG_UOBJS = $(MESASLANGBUILDDIR)/unshared/slang_builtin.o \
                    $(MESASLANGBUILDDIR)/unshared/slang_codegen.o \
                    $(MESASLANGBUILDDIR)/unshared/slang_compile.o \
                    $(MESASLANGBUILDDIR)/unshared/slang_compile_function.o \
                    $(MESASLANGBUILDDIR)/unshared/slang_compile_operation.o \
                    $(MESASLANGBUILDDIR)/unshared/slang_compile_struct.o \
                    $(MESASLANGBUILDDIR)/unshared/slang_compile_variable.o \
                    $(MESASLANGBUILDDIR)/unshared/slang_emit.o \
                    $(MESASLANGBUILDDIR)/unshared/slang_ir.o \
                    $(MESASLANGBUILDDIR)/unshared/slang_label.o \
                    $(MESASLANGBUILDDIR)/unshared/slang_link.o \
                    $(MESASLANGBUILDDIR)/unshared/slang_log.o \
                    $(MESASLANGBUILDDIR)/unshared/slang_mem.o \
                    $(MESASLANGBUILDDIR)/unshared/slang_preprocess.o \
                    $(MESASLANGBUILDDIR)/unshared/slang_print.o \
                    $(MESASLANGBUILDDIR)/unshared/slang_simplify.o \
                    $(MESASLANGBUILDDIR)/unshared/slang_storage.o \
                    $(MESASLANGBUILDDIR)/unshared/slang_typeinfo.o \
                    $(MESASLANGBUILDDIR)/unshared/slang_utility.o \
                    $(MESASLANGBUILDDIR)/unshared/slang_vartable.o
#else
 MESA_SLANG_UOBJS = $(MESA_SLANG_OBJS)
#endif

 MESA_SLANG_UOBJS = $(MESASLANGBUILDDIR)/debugger/slang_builtin.o \
                    $(MESASLANGBUILDDIR)/debugger/slang_codegen.o \
                    $(MESASLANGBUILDDIR)/debugger/slang_compile.o \
                    $(MESASLANGBUILDDIR)/debugger/slang_compile_function.o \
                    $(MESASLANGBUILDDIR)/debugger/slang_compile_operation.o \
                    $(MESASLANGBUILDDIR)/debugger/slang_compile_struct.o \
                    $(MESASLANGBUILDDIR)/debugger/slang_compile_variable.o \
                    $(MESASLANGBUILDDIR)/debugger/slang_emit.o \
                    $(MESASLANGBUILDDIR)/debugger/slang_ir.o \
                    $(MESASLANGBUILDDIR)/debugger/slang_label.o \
                    $(MESASLANGBUILDDIR)/debugger/slang_link.o \
                    $(MESASLANGBUILDDIR)/debugger/slang_log.o \
                    $(MESASLANGBUILDDIR)/debugger/slang_mem.o \
                    $(MESASLANGBUILDDIR)/debugger/slang_preprocess.o \
                    $(MESASLANGBUILDDIR)/debugger/slang_print.o \
                    $(MESASLANGBUILDDIR)/debugger/slang_simplify.o \
                    $(MESASLANGBUILDDIR)/debugger/slang_storage.o \
                    $(MESASLANGBUILDDIR)/debugger/slang_typeinfo.o \
                    $(MESASLANGBUILDDIR)/debugger/slang_utility.o \
                    $(MESASLANGBUILDDIR)/debugger/slang_vartable.o

 MESA_SLANG_POBJS = $(MESASLANGBUILDDIR)/profiled/slang_builtin.o \
                    $(MESASLANGBUILDDIR)/profiled/slang_codegen.o \
                    $(MESASLANGBUILDDIR)/profiled/slang_compile.o \
                    $(MESASLANGBUILDDIR)/profiled/slang_compile_function.o \
                    $(MESASLANGBUILDDIR)/profiled/slang_compile_operation.o \
                    $(MESASLANGBUILDDIR)/profiled/slang_compile_struct.o \
                    $(MESASLANGBUILDDIR)/profiled/slang_compile_variable.o \
                    $(MESASLANGBUILDDIR)/profiled/slang_emit.o \
                    $(MESASLANGBUILDDIR)/profiled/slang_ir.o \
                    $(MESASLANGBUILDDIR)/profiled/slang_label.o \
                    $(MESASLANGBUILDDIR)/profiled/slang_link.o \
                    $(MESASLANGBUILDDIR)/profiled/slang_log.o \
                    $(MESASLANGBUILDDIR)/profiled/slang_mem.o \
                    $(MESASLANGBUILDDIR)/profiled/slang_preprocess.o \
                    $(MESASLANGBUILDDIR)/profiled/slang_print.o \
                    $(MESASLANGBUILDDIR)/profiled/slang_simplify.o \
                    $(MESASLANGBUILDDIR)/profiled/slang_storage.o \
                    $(MESASLANGBUILDDIR)/profiled/slang_typeinfo.o \
                    $(MESASLANGBUILDDIR)/profiled/slang_utility.o \
                    $(MESASLANGBUILDDIR)/profiled/slang_vartable.o
