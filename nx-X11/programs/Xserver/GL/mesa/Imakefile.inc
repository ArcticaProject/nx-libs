MESABUILDDIR = MesaBuildDir

COREMESASRCS = $(COREMESABASESRCS) \
                $(MESA_MATH_SRCS) \
                $(MESA_SWRAST_SRCS) \
                $(MESA_SWR_SETUP_SRCS) \
                $(MESA_TNL_SRCS) \
                $(MESA_SHADER_SRCS) \
                $(MESA_GRAMMAR_SRCS) \
                $(MESA_SLANG_SRCS)

COREMESAOBJS =  $(COREMESABASEOBJS) \
                $(MESA_MATH_OBJS) \
                $(MESA_SWRAST_OBJS) \
                $(MESA_SWR_SETUP_OBJS) \
                $(MESA_TNL_OBJS) \
                $(MESA_SHADER_OBJS) \
                $(MESA_GRAMMAR_OBJS) \
                $(MESA_SLANG_OBJS)

