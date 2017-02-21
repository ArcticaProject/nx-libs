MESAXBUILDDIR = MesaXBuildDir

   XMESA_SRCS = $(XMESA_GLX_SRCS) \
		$(MESAXBUILDDIR)xm_api.c \
                $(MESAXBUILDDIR)xm_buffer.c \
		$(MESAXBUILDDIR)xm_dd.c \
		$(MESAXBUILDDIR)xm_line.c \
		$(MESAXBUILDDIR)xm_span.c \
		$(MESAXBUILDDIR)xm_tri.c

#ifdef NeedToLinkMesaSrc
LinkSourceFile(xm_api.c, $(MESASRCDIR)/src/mesa/drivers/x11)
LinkSourceFile(xm_buffer.c, $(MESASRCDIR)/src/mesa/drivers/x11)
LinkSourceFile(xm_dd.c, $(MESASRCDIR)/src/mesa/drivers/x11)
LinkSourceFile(xm_line.c, $(MESASRCDIR)/src/mesa/drivers/x11)
LinkSourceFile(xm_span.c, $(MESASRCDIR)/src/mesa/drivers/x11)
LinkSourceFile(xm_tri.c, $(MESASRCDIR)/src/mesa/drivers/x11)
#endif

   XMESA_OBJS = $(XMESA_GLX_OBJS) \
		$(MESAXBUILDDIR)xm_api.o \
                $(MESAXBUILDDIR)xm_buffer.o \
		$(MESAXBUILDDIR)xm_dd.o \
		$(MESAXBUILDDIR)xm_line.o \
		$(MESAXBUILDDIR)xm_span.o \
		$(MESAXBUILDDIR)xm_tri.o
