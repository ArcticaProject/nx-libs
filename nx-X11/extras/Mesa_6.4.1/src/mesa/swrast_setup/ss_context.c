/*
 * Mesa 3-D graphics library
 * Version:  6.1
 *
 * Copyright (C) 1999-2004  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Keith Whitwell <keith@tungstengraphics.com>
 */

#include "glheader.h"
#include "imports.h"
#include "colormac.h"
#include "ss_context.h"
#include "ss_triangle.h"
#include "swrast_setup.h"
#include "tnl/tnl.h"
#include "tnl/t_context.h"
#include "tnl/t_pipeline.h"
#include "tnl/t_vertex.h"

/* Need to check lighting state and vertex program state to know
 * if two-sided lighting is in effect.
 */
#define _SWSETUP_NEW_RENDERINDEX (_NEW_POLYGON|_NEW_LIGHT|_NEW_PROGRAM)


GLboolean
_swsetup_CreateContext( GLcontext *ctx )
{
   SScontext *swsetup = (SScontext *)CALLOC(sizeof(SScontext));

   if (!swsetup)
      return GL_FALSE;

   ctx->swsetup_context = swsetup;

   swsetup->NewState = ~0;
   _swsetup_trifuncs_init( ctx );

   _tnl_init_vertices( ctx, ctx->Const.MaxArrayLockSize + 12, 
		       sizeof(SWvertex) );


   return GL_TRUE;
}

void
_swsetup_DestroyContext( GLcontext *ctx )
{
   SScontext *swsetup = SWSETUP_CONTEXT(ctx);

   if (swsetup) {
      FREE(swsetup);
      ctx->swsetup_context = 0;
   }

   _tnl_free_vertices( ctx );
}

static void
_swsetup_RenderPrimitive( GLcontext *ctx, GLenum mode )
{
   SWSETUP_CONTEXT(ctx)->render_prim = mode;
   _swrast_render_primitive( ctx, mode );
}

#define SWZ ((SWvertex *)0)
#define SWOffset(MEMBER) (((char *)&(SWZ->MEMBER)) - ((char *)SWZ))

#define EMIT_ATTR( ATTR, STYLE, MEMBER )	\
do {						\
   map[e].attrib = (ATTR);			\
   map[e].format = (STYLE);			\
   map[e].offset = SWOffset(MEMBER);	       	\
   e++;						\
} while (0)


/*
 * We patch this function into tnl->Driver.Render.Start.
 * It's called when we start rendering a vertex buffer.
 */
static void
_swsetup_RenderStart( GLcontext *ctx )
{
   SScontext *swsetup = SWSETUP_CONTEXT(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   GLuint new_state = swsetup->NewState;

   if (new_state & _SWSETUP_NEW_RENDERINDEX) {
      _swsetup_choose_trifuncs( ctx );
   }

   swsetup->NewState = 0;

   _swrast_render_start( ctx );

   /* Important:
    */
   VB->AttribPtr[VERT_ATTRIB_POS] = VB->NdcPtr;


   if (tnl->render_inputs != swsetup->last_index) {
      GLuint index = tnl->render_inputs;
      struct tnl_attr_map map[_TNL_ATTRIB_MAX];
      int i, e = 0;

      EMIT_ATTR( _TNL_ATTRIB_POS, EMIT_4F_VIEWPORT, win );
   
      if (index & _TNL_BIT_COLOR0)
	 EMIT_ATTR( _TNL_ATTRIB_COLOR0, EMIT_4CHAN_4F_RGBA, color );

      if (index & _TNL_BIT_COLOR1)
	 EMIT_ATTR( _TNL_ATTRIB_COLOR1, EMIT_4CHAN_4F_RGBA, specular);

      if (index & _TNL_BIT_FOG) 
	 EMIT_ATTR( _TNL_ATTRIB_FOG, EMIT_1F, fog);
	 
      if (index & _TNL_BITS_TEX_ANY) {
	 for (i = 0; i < MAX_TEXTURE_COORD_UNITS; i++) {
	    if (index & _TNL_BIT_TEX(i)) {
	       EMIT_ATTR( _TNL_ATTRIB_TEX0+i, EMIT_4F, texcoord[i] );
	    }
	 }
      }
      
      if (index & _TNL_BIT_INDEX) 
	 EMIT_ATTR( _TNL_ATTRIB_INDEX, EMIT_1F, index );
 
      if (index & _TNL_BIT_POINTSIZE)
	 EMIT_ATTR( _TNL_ATTRIB_POINTSIZE, EMIT_1F, pointSize );
   
      _tnl_install_attrs( ctx, map, e,
			  ctx->Viewport._WindowMap.m,
			  sizeof(SWvertex) ); 
      
      swsetup->last_index = index;
   }

}

/*
 * We patch this function into tnl->Driver.Render.Finish.
 * It's called when we finish rendering a vertex buffer.
 */
static void
_swsetup_RenderFinish( GLcontext *ctx )
{
   _swrast_render_finish( ctx );
}

void
_swsetup_InvalidateState( GLcontext *ctx, GLuint new_state )
{
   SScontext *swsetup = SWSETUP_CONTEXT(ctx);
   swsetup->NewState |= new_state;
   _tnl_invalidate_vertex_state( ctx, new_state );
}


void
_swsetup_Wakeup( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   SScontext *swsetup = SWSETUP_CONTEXT(ctx);

   tnl->Driver.Render.Start = _swsetup_RenderStart;
   tnl->Driver.Render.Finish = _swsetup_RenderFinish;
   tnl->Driver.Render.PrimitiveNotify = _swsetup_RenderPrimitive;
   tnl->Driver.Render.Interp = _tnl_interp;
   tnl->Driver.Render.CopyPV = _tnl_copy_pv;
   tnl->Driver.Render.ClippedPolygon = _tnl_RenderClippedPolygon; /* new */
   tnl->Driver.Render.ClippedLine = _tnl_RenderClippedLine; /* new */
   /* points */
   /* line */
   /* triangle */
   /* quad */
   tnl->Driver.Render.PrimTabVerts = _tnl_render_tab_verts;
   tnl->Driver.Render.PrimTabElts = _tnl_render_tab_elts;
   tnl->Driver.Render.ResetLineStipple = _swrast_ResetLineStipple;
   tnl->Driver.Render.BuildVertices = _tnl_build_vertices;
   tnl->Driver.Render.Multipass = 0;

   _tnl_invalidate_vertices( ctx, ~0 );
   _tnl_need_projected_coords( ctx, GL_TRUE );
   _swsetup_InvalidateState( ctx, ~0 );

   swsetup->verts = (SWvertex *)tnl->clipspace.vertex_buf;
   swsetup->last_index = 0;
}





/* Populate a swrast SWvertex from an attrib-style vertex.
 */
void 
_swsetup_Translate( GLcontext *ctx, const void *vertex, SWvertex *dest )
{
   const GLfloat *m = ctx->Viewport._WindowMap.m;
   GLfloat tmp[4];
   GLuint i;

   _tnl_get_attr( ctx, vertex, _TNL_ATTRIB_POS, tmp );

   dest->win[0] = m[0]  * tmp[0] + m[12];
   dest->win[1] = m[5]  * tmp[1] + m[13];
   dest->win[2] = m[10] * tmp[2] + m[14];
   dest->win[3] =         tmp[3];


   for (i = 0 ; i < ctx->Const.MaxTextureUnits ; i++)
      _tnl_get_attr( ctx, vertex, _TNL_ATTRIB_TEX0+i, dest->texcoord[i] );
	  
   _tnl_get_attr( ctx, vertex, _TNL_ATTRIB_COLOR0, tmp );
   UNCLAMPED_FLOAT_TO_RGBA_CHAN( dest->color, tmp );

   _tnl_get_attr( ctx, vertex, _TNL_ATTRIB_COLOR1, tmp );
   UNCLAMPED_FLOAT_TO_RGBA_CHAN( dest->specular, tmp );

   _tnl_get_attr( ctx, vertex, _TNL_ATTRIB_FOG, tmp );
   dest->fog = tmp[0];

   _tnl_get_attr( ctx, vertex, _TNL_ATTRIB_INDEX, tmp );
   dest->index = (GLuint) tmp[0];

   _tnl_get_attr( ctx, vertex, _TNL_ATTRIB_POINTSIZE, tmp );
   dest->pointSize = tmp[0];
}

