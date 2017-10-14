MESATNLBUILDDIR = MesaTnlBuildDir

  MESA_TNL_SRCS = $(MESATNLBUILDDIR)t_context.c \
		  $(MESATNLBUILDDIR)t_draw.c \
		  $(MESATNLBUILDDIR)t_pipeline.c \
		  $(MESATNLBUILDDIR)t_vb_fog.c \
		  $(MESATNLBUILDDIR)t_vb_cull.c \
		  $(MESATNLBUILDDIR)t_vb_light.c \
		  $(MESATNLBUILDDIR)t_vb_normals.c \
		  $(MESATNLBUILDDIR)t_vb_points.c \
		  $(MESATNLBUILDDIR)t_vb_program.c \
		  $(MESATNLBUILDDIR)t_vb_render.c \
		  $(MESATNLBUILDDIR)t_vb_texgen.c \
		  $(MESATNLBUILDDIR)t_vb_texmat.c \
		  $(MESATNLBUILDDIR)t_vb_vertex.c \
		  $(MESATNLBUILDDIR)t_vertex.c \
                  $(MESATNLBUILDDIR)t_vertex_generic.c \
                  $(MESATNLBUILDDIR)t_vertex_sse.c \
                  $(MESATNLBUILDDIR)t_vp_build.c

#ifdef NeedToLinkMesaSrc
LinkSourceFile(t_context.c, $(MESASRCDIR)/src/mesa/tnl)
LinkSourceFile(t_context.h, $(MESASRCDIR)/src/mesa/tnl)
LinkSourceFile(t_draw.c, $(MESASRCDIR)/src/mesa/tnl)
LinkSourceFile(tnl.h, $(MESASRCDIR)/src/mesa/tnl)
LinkSourceFile(t_pipeline.c, $(MESASRCDIR)/src/mesa/tnl)
LinkSourceFile(t_pipeline.h, $(MESASRCDIR)/src/mesa/tnl)
LinkSourceFile(t_vb_cliptmp.h, $(MESASRCDIR)/src/mesa/tnl)
LinkSourceFile(t_vb_fog.c, $(MESASRCDIR)/src/mesa/tnl)
LinkSourceFile(t_vb_cull.c, $(MESASRCDIR)/src/mesa/tnl)
LinkSourceFile(t_vb_light.c, $(MESASRCDIR)/src/mesa/tnl)
LinkSourceFile(t_vb_lighttmp.h, $(MESASRCDIR)/src/mesa/tnl)
LinkSourceFile(t_vb_normals.c, $(MESASRCDIR)/src/mesa/tnl)
LinkSourceFile(t_vb_points.c, $(MESASRCDIR)/src/mesa/tnl)
LinkSourceFile(t_vb_program.c, $(MESASRCDIR)/src/mesa/tnl)
LinkSourceFile(t_vb_render.c, $(MESASRCDIR)/src/mesa/tnl)
LinkSourceFile(t_vb_rendertmp.h, $(MESASRCDIR)/src/mesa/tnl)
LinkSourceFile(t_vb_texgen.c, $(MESASRCDIR)/src/mesa/tnl)
LinkSourceFile(t_vb_texmat.c, $(MESASRCDIR)/src/mesa/tnl)
LinkSourceFile(t_vb_vertex.c, $(MESASRCDIR)/src/mesa/tnl)
LinkSourceFile(t_vertex.c, $(MESASRCDIR)/src/mesa/tnl)
LinkSourceFile(t_vertex.h, $(MESASRCDIR)/src/mesa/tnl)
LinkSourceFile(t_vertex_generic.c, $(MESASRCDIR)/src/mesa/tnl)
LinkSourceFile(t_vertex_sse.c, $(MESASRCDIR)/src/mesa/tnl)
LinkSourceFile(t_vp_build.c, $(MESASRCDIR)/src/mesa/tnl)
LinkSourceFile(t_vp_build.h, $(MESASRCDIR)/src/mesa/tnl)
#endif

  MESA_TNL_OBJS = $(MESATNLBUILDDIR)t_context.o \
                  $(MESATNLBUILDDIR)t_draw.o \
                  $(MESATNLBUILDDIR)t_pipeline.o \
                  $(MESATNLBUILDDIR)t_vb_fog.o \
                  $(MESATNLBUILDDIR)t_vb_cull.o \
                  $(MESATNLBUILDDIR)t_vb_light.o \
                  $(MESATNLBUILDDIR)t_vb_normals.o \
                  $(MESATNLBUILDDIR)t_vb_points.o \
                  $(MESATNLBUILDDIR)t_vb_program.o \
                  $(MESATNLBUILDDIR)t_vb_render.o \
                  $(MESATNLBUILDDIR)t_vb_texgen.o \
                  $(MESATNLBUILDDIR)t_vb_texmat.o \
                  $(MESATNLBUILDDIR)t_vb_vertex.o \
                  $(MESATNLBUILDDIR)t_vertex.o \
                  $(MESATNLBUILDDIR)t_vertex_generic.o \
                  $(MESATNLBUILDDIR)t_vertex_sse.o \
                  $(MESATNLBUILDDIR)t_vp_build.o
		  
#if defined(DoSharedLib) && DoSharedLib
 MESA_TNL_UOBJS = $(MESATNLBUILDDIR)unshared/t_context.o \
		  $(MESATNLBUILDDIR)unshared/t_draw.o \
		  $(MESATNLBUILDDIR)unshared/t_pipeline.o \
		  $(MESATNLBUILDDIR)unshared/t_vb_fog.o \
		  $(MESATNLBUILDDIR)unshared/t_vb_cull.o \
		  $(MESATNLBUILDDIR)unshared/t_vb_light.o \
		  $(MESATNLBUILDDIR)unshared/t_vb_normals.o \
		  $(MESATNLBUILDDIR)unshared/t_vb_points.o \
		  $(MESATNLBUILDDIR)unshared/t_vb_program.o \
		  $(MESATNLBUILDDIR)unshared/t_vb_render.o \
		  $(MESATNLBUILDDIR)unshared/t_vb_texgen.o \
		  $(MESATNLBUILDDIR)unshared/t_vb_texmat.o \
		  $(MESATNLBUILDDIR)unshared/t_vb_vertex.o \
		  $(MESATNLBUILDDIR)unshared/t_vertex.o \
                  $(MESATNLBUILDDIR)unshared/t_vertex_generic.o \
                  $(MESATNLBUILDDIR)unshared/t_vertex_sse.o \
                  $(MESATNLBUILDDIR)unshared/t_vp_build.o 
#else
 MESA_TNL_UOBJS = $(MESA_TNL_OBJS)
#endif

 MESA_TNL_DOBJS = $(MESATNLBUILDDIR)debugger/t_context.o \
		  $(MESATNLBUILDDIR)debugger/t_draw.o \
		  $(MESATNLBUILDDIR)debugger/t_pipeline.o \
		  $(MESATNLBUILDDIR)debugger/t_vb_fog.o \
		  $(MESATNLBUILDDIR)debugger/t_vb_cull.o \
		  $(MESATNLBUILDDIR)debugger/t_vb_light.o \
		  $(MESATNLBUILDDIR)debugger/t_vb_normals.o \
		  $(MESATNLBUILDDIR)debugger/t_vb_points.o \
		  $(MESATNLBUILDDIR)debugger/t_vb_program.o \
		  $(MESATNLBUILDDIR)debugger/t_vb_render.o \
		  $(MESATNLBUILDDIR)debugger/t_vb_texgen.o \
		  $(MESATNLBUILDDIR)debugger/t_vb_texmat.o \
		  $(MESATNLBUILDDIR)debugger/t_vb_vertex.o \
		  $(MESATNLBUILDDIR)debugger/t_vertex.o \
                  $(MESATNLBUILDDIR)debugger/t_vertex_generic.o \
                  $(MESATNLBUILDDIR)debugger/t_vertex_sse.o \
                  $(MESATNLBUILDDIR)debugger/t_vp_build.o

 MESA_TNL_POBJS = $(MESATNLBUILDDIR)profiled/t_context.o \
		  $(MESATNLBUILDDIR)profiled/t_draw.o \
		  $(MESATNLBUILDDIR)profiled/t_pipeline.o \
		  $(MESATNLBUILDDIR)profiled/t_vb_fog.o \
		  $(MESATNLBUILDDIR)profiled/t_vb_cull.o \
		  $(MESATNLBUILDDIR)profiled/t_vb_light.o \
		  $(MESATNLBUILDDIR)profiled/t_vb_normals.o \
		  $(MESATNLBUILDDIR)profiled/t_vb_points.o \
		  $(MESATNLBUILDDIR)profiled/t_vb_program.o \
		  $(MESATNLBUILDDIR)profiled/t_vb_render.o \
		  $(MESATNLBUILDDIR)profiled/t_vb_texgen.o \
		  $(MESATNLBUILDDIR)profiled/t_vb_texmat.o \
		  $(MESATNLBUILDDIR)profiled/t_vb_vertex.o \
		  $(MESATNLBUILDDIR)profiled/t_vertex.o \
                  $(MESATNLBUILDDIR)profiled/t_vertex_generic.o \
                  $(MESATNLBUILDDIR)profiled/t_vertex_sse.o \
                  $(MESATNLBUILDDIR)profiled/t_vp_build.o

