/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

#include "i830_context.h"
#include "imports.h"
#include "texmem.h"
#include "intel_tex.h"
#include "tnl/tnl.h"
#include "tnl/t_vertex.h"
#include "tnl/t_context.h"
#include "utils.h"

/***************************************
 * Mesa's Driver Functions
 ***************************************/

static const struct dri_extension i830_extensions[] = {
   {"GL_ARB_texture_env_crossbar", NULL},
   {NULL, NULL}
};


static void
i830InitDriverFunctions(struct dd_function_table *functions)
{
   intelInitDriverFunctions(functions);
   i830InitStateFuncs(functions);
   i830InitTextureFuncs(functions);
}


GLboolean
i830CreateContext(const __GLcontextModes * mesaVis,
                  __DRIcontextPrivate * driContextPriv,
                  void *sharedContextPrivate)
{
   struct dd_function_table functions;
   struct i830_context *i830 = CALLOC_STRUCT(i830_context);
   struct intel_context *intel = &i830->intel;
   GLcontext *ctx = &intel->ctx;
   if (!i830)
      return GL_FALSE;

   i830InitVtbl(i830);
   i830InitDriverFunctions(&functions);

   if (!intelInitContext(intel, mesaVis, driContextPriv,
                         sharedContextPrivate, &functions)) {
      FREE(i830);
      return GL_FALSE;
   }

   intel->ctx.Const.MaxTextureUnits = I830_TEX_UNITS;
   intel->ctx.Const.MaxTextureImageUnits = I830_TEX_UNITS;
   intel->ctx.Const.MaxTextureCoordUnits = I830_TEX_UNITS;

   /* Advertise the full hardware capabilities.  The new memory
    * manager should cope much better with overload situations:
    */
   ctx->Const.MaxTextureLevels = 12;
   ctx->Const.Max3DTextureLevels = 9;
   ctx->Const.MaxCubeTextureLevels = 11;
   ctx->Const.MaxTextureRectSize = (1 << 11);
   ctx->Const.MaxTextureUnits = I830_TEX_UNITS;

   _tnl_init_vertices(ctx, ctx->Const.MaxArrayLockSize + 12,
                      18 * sizeof(GLfloat));

   intel->verts = TNL_CONTEXT(ctx)->clipspace.vertex_buf;

   driInitExtensions(ctx, i830_extensions, GL_FALSE);

   i830InitState(i830);
   i830InitMetaFuncs(i830);

   _tnl_allow_vertex_fog(ctx, 1);
   _tnl_allow_pixel_fog(ctx, 0);

   return GL_TRUE;
}
