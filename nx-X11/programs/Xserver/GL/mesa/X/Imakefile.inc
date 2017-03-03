MESAXBUILDDIR = MesaXBuildDir

   XMESA_SRCS = 				\
                $(MESAXBUILDDIR)xm_api.c	\
                $(MESAXBUILDDIR)xm_buffer.c	\
                $(MESAXBUILDDIR)xm_dd.c		\
                $(MESAXBUILDDIR)xm_glide.c	\
                $(MESAXBUILDDIR)xm_image.c	\
                $(MESAXBUILDDIR)xm_line.c	\
                $(MESAXBUILDDIR)xm_span.c	\
                $(MESAXBUILDDIR)xm_tri.c	\
                $(NULL)

LinkSourceFile(xm_api.c, $(MESASRCDIR)/src/mesa/drivers/x11)
LinkSourceFile(xm_buffer.c, $(MESASRCDIR)/src/mesa/drivers/x11)
LinkSourceFile(xm_dd.c, $(MESASRCDIR)/src/mesa/drivers/x11)
LinkSourceFile(xm_line.c, $(MESASRCDIR)/src/mesa/drivers/x11)
LinkSourceFile(xm_glide.c, $(MESASRCDIR)/src/mesa/drivers/x11)
LinkSourceFile(xm_glide.h, $(MESASRCDIR)/src/mesa/drivers/x11)
LinkSourceFile(xm_image.c, $(MESASRCDIR)/src/mesa/drivers/x11)
LinkSourceFile(xm_image.h, $(MESASRCDIR)/src/mesa/drivers/x11)
LinkSourceFile(xm_span.c, $(MESASRCDIR)/src/mesa/drivers/x11)
LinkSourceFile(xm_tri.c, $(MESASRCDIR)/src/mesa/drivers/x11)

   XMESA_OBJS =					\
                $(MESAXBUILDDIR)xm_api.o	\
                $(MESAXBUILDDIR)xm_buffer.o	\
                $(MESAXBUILDDIR)xm_dd.o		\
                $(MESAXBUILDDIR)xm_glide.o	\
                $(MESAXBUILDDIR)xm_image.o	\
                $(MESAXBUILDDIR)xm_line.o	\
                $(MESAXBUILDDIR)xm_span.o	\
                $(MESAXBUILDDIR)xm_tri.o	\
                $(NULL)
