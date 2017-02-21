MESAMATHBUILDDIR = MesaMathBuildDir

  MESA_MATH_SRCS = $(MESAMATHBUILDDIR)m_debug_clip.c \
		   $(MESAMATHBUILDDIR)m_debug_norm.c \
		   $(MESAMATHBUILDDIR)m_debug_xform.c \
		   $(MESAMATHBUILDDIR)m_eval.c \
		   $(MESAMATHBUILDDIR)m_matrix.c \
		   $(MESAMATHBUILDDIR)m_translate.c \
		   $(MESAMATHBUILDDIR)m_vector.c \
		   $(MESAMATHBUILDDIR)m_xform.c

#ifdef NeedToLinkMesaSrc
LinkSourceFile(mathmod.h, $(MESASRCDIR)/src/mesa/math)
LinkSourceFile(m_clip_tmp.h, $(MESASRCDIR)/src/mesa/math)
LinkSourceFile(m_copy_tmp.h, $(MESASRCDIR)/src/mesa/math)
LinkSourceFile(m_debug_clip.c, $(MESASRCDIR)/src/mesa/math)
LinkSourceFile(m_debug.h, $(MESASRCDIR)/src/mesa/math)
LinkSourceFile(m_debug_util.h, $(MESASRCDIR)/src/mesa/math)
LinkSourceFile(m_debug_norm.c, $(MESASRCDIR)/src/mesa/math)
LinkSourceFile(m_debug_xform.c, $(MESASRCDIR)/src/mesa/math)
LinkSourceFile(m_dotprod_tmp.h, $(MESASRCDIR)/src/mesa/math)
LinkSourceFile(m_eval.c, $(MESASRCDIR)/src/mesa/math)
LinkSourceFile(m_eval.h, $(MESASRCDIR)/src/mesa/math)
LinkSourceFile(m_matrix.c, $(MESASRCDIR)/src/mesa/math)
LinkSourceFile(m_matrix.h, $(MESASRCDIR)/src/mesa/math)
LinkSourceFile(m_norm_tmp.h, $(MESASRCDIR)/src/mesa/math)
LinkSourceFile(m_translate.c, $(MESASRCDIR)/src/mesa/math)
LinkSourceFile(m_translate.h, $(MESASRCDIR)/src/mesa/math)
LinkSourceFile(m_trans_tmp.h, $(MESASRCDIR)/src/mesa/math)
LinkSourceFile(m_vector.c, $(MESASRCDIR)/src/mesa/math)
LinkSourceFile(m_vector.h, $(MESASRCDIR)/src/mesa/math)
LinkSourceFile(m_xform.c, $(MESASRCDIR)/src/mesa/math)
LinkSourceFile(m_xform.h, $(MESASRCDIR)/src/mesa/math)
LinkSourceFile(m_xform_tmp.h, $(MESASRCDIR)/src/mesa/math)
#endif

  MESA_MATH_OBJS = $(MESAMATHBUILDDIR)m_debug_clip.o \
		   $(MESAMATHBUILDDIR)m_debug_norm.o \
		   $(MESAMATHBUILDDIR)m_debug_xform.o \
		   $(MESAMATHBUILDDIR)m_eval.o \
		   $(MESAMATHBUILDDIR)m_matrix.o \
		   $(MESAMATHBUILDDIR)m_translate.o \
		   $(MESAMATHBUILDDIR)m_vector.o \
		   $(MESAMATHBUILDDIR)m_xform.o
