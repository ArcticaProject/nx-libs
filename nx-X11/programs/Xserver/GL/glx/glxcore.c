/* $XFree86: xc/programs/Xserver/GL/mesa/src/X/xf86glx.c,v 1.19 2003/07/16 01:38:27 dawes Exp $ */
/**************************************************************************

Copyright 1998-1999 Precision Insight, Inc., Cedar Park, Texas.
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sub license, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial portions
of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Kevin E. Martin <kevin@precisioninsight.com>
 *   Brian E. Paul <brian@precisioninsight.com>
 *
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <string.h>
#include <regionstr.h>
#include <resource.h>
#include <GL/gl.h>
#include <GL/glxint.h>
#include <GL/glxtokens.h>
#include <scrnintstr.h>
#include <windowstr.h>
#include <config.h>
#include <glxserver.h>
#include <glxscreens.h>
#include <glxdrawable.h>
#include <glxcontext.h>
#include <glxext.h>
#include <glxutil.h>
#include "context.h"
#include "xmesaP.h"
#include <GL/xf86glx.h>
#include "context.h"

#include "glcontextmodes.h"
#include "os.h"

/*
 * This define is for the glcore.h header file.
 * If you add it here, then make sure you also add it in
 * ../../../glx/Imakefile.
 */
#if 0
#define DEBUG
#include <GL/internal/glcore.h>
#undef DEBUG
#else
#include <GL/internal/glcore.h>
#endif

typedef struct __GLXMESAscreen   __GLXMESAscreen;
typedef struct __GLXMESAcontext  __GLXMESAcontext;
typedef struct __GLXMESAdrawable __GLXMESAdrawable;

struct __GLXMESAscreen {
    __GLXscreen   base;
    int           index;
    XMesaVisual  *xm_vis;
};

struct __GLXMESAcontext {
    __GLXcontext base;
    XMesaContext xmesa;
};

struct __GLXMESAdrawable {
    __GLXdrawable base;
    XMesaBuffer   xm_buf;
};

static XMesaVisual find_mesa_visual(__GLXscreen *screen, VisualID vid);


static void
__glXMesaDrawableDestroy(__GLXdrawable *base)
{
    __GLXMESAdrawable *glxPriv = (__GLXMESAdrawable *) base;

    if (glxPriv->xm_buf != NULL)
      XMesaDestroyBuffer(glxPriv->xm_buf);
    free(glxPriv);
}

static GLboolean
__glXMesaDrawableResize(__GLXdrawable *base)
{
    __GLXMESAdrawable *glxPriv = (__GLXMESAdrawable *) base;

    XMesaResizeBuffers(glxPriv->xm_buf);

    return GL_TRUE;
}

static GLboolean
__glXMesaDrawableSwapBuffers(__GLXdrawable *base)
{
    __GLXMESAdrawable *glxPriv = (__GLXMESAdrawable *) base;

    /* This is terrifying: XMesaSwapBuffers() ends up calling CopyArea
     * to do the buffer swap, but this assumes that the server holds
     * the lock and has its context visible.  If another screen uses a
     * DRI driver, that will have installed the DRI enter/leave server
     * functions, which lifts the lock during GLX dispatch.  This is
     * why we need to re-take the lock and swap in the server context
     * before calling XMesaSwapBuffers() here.  /me shakes head. */

    __glXenterServer();

    XMesaSwapBuffers(glxPriv->xm_buf);

    __glXleaveServer();

    return GL_TRUE;
}


static __GLXdrawable *
__glXMesaContextCreateDrawable(__GLXcontext *context,
			       DrawablePtr pDraw,
			       XID drawId)
{
    __GLXMESAdrawable *glxPriv;
    __GLXscreen *pGlxScreen;
    XMesaVisual xm_vis;

    glxPriv = malloc(sizeof *glxPriv);
    if (glxPriv == NULL)
	return NULL;

    memset(glxPriv, 0, sizeof *glxPriv);

    if (!__glXDrawableInit(&glxPriv->base, context, pDraw, drawId)) {
        free(glxPriv);
	return NULL;
    }

    glxPriv->base.destroy       = __glXMesaDrawableDestroy;
    glxPriv->base.resize        = __glXMesaDrawableResize;
    glxPriv->base.swapBuffers   = __glXMesaDrawableSwapBuffers;

    pGlxScreen = __glXActiveScreens[pDraw->pScreen->myNum];

    if (glxPriv->base.type == DRAWABLE_WINDOW) {
	VisualID vid = wVisual((WindowPtr)pDraw);

	glxPriv->base.modes = _gl_context_modes_find_visual(pGlxScreen->modes,
							    vid);
    } else {
	glxPriv->base.modes = glxPriv->base.pGlxPixmap->modes;
    }

    xm_vis = find_mesa_visual(pGlxScreen, glxPriv->base.modes->visualID);
    if (xm_vis == NULL) {
	ErrorF("find_mesa_visual returned NULL for visualID = 0x%04x\n",
	       glxPriv->base.modes->visualID);
	free(glxPriv);
	return NULL;
    }

    if (glxPriv->base.type == DRAWABLE_WINDOW) {
	glxPriv->xm_buf = XMesaCreateWindowBuffer(xm_vis, (WindowPtr)pDraw);
    } else {
	glxPriv->xm_buf = XMesaCreatePixmapBuffer(xm_vis, (PixmapPtr)pDraw, 0);
    }

    return &glxPriv->base;
}

static void
__glXMesaContextDestroy(__GLXcontext *baseContext)
{
    __GLXMESAcontext *context = (__GLXMESAcontext *) baseContext;

    XMesaDestroyContext(context->xmesa);
    __glXContextDestroy(context);
    free(context);
}

static int
__glXMesaContextMakeCurrent(__GLXcontext *baseContext)

{
    __GLXMESAcontext *context = (__GLXMESAcontext *) baseContext;
    __GLXMESAdrawable *drawPriv = (__GLXMESAdrawable *) context->base.drawPriv;
    __GLXMESAdrawable *readPriv = (__GLXMESAdrawable *) context->base.readPriv;

    return XMesaMakeCurrent2(context->xmesa,
			     drawPriv->xm_buf,
			     readPriv->xm_buf);
}

static int
__glXMesaContextLoseCurrent(__GLXcontext *baseContext)
{
    __GLXMESAcontext *context = (__GLXMESAcontext *) baseContext;

    return XMesaLoseCurrent(context->xmesa);
}

static int
__glXMesaContextCopy(__GLXcontext *baseDst,
		     __GLXcontext *baseSrc,
		     unsigned long mask)
{
    __GLXMESAcontext *dst = (__GLXMESAcontext *) baseDst;
    __GLXMESAcontext *src = (__GLXMESAcontext *) baseSrc;

    _mesa_copy_context(&src->xmesa->mesa, &dst->xmesa->mesa, mask);
    return GL_TRUE;
}

static int
__glXMesaContextForceCurrent(__GLXcontext *baseContext)
{
    __GLXMESAcontext *context = (__GLXMESAcontext *) baseContext;

    GlxSetRenderTables (context->xmesa->mesa.CurrentDispatch);

    return XMesaForceCurrent(context->xmesa);
}

static __GLXcontext *
__glXMesaScreenCreateContext(__GLXscreen *screen,
			     __GLcontextModes *modes,
			     __GLXcontext *baseShareContext)
{
    __GLXMESAcontext *context;
    __GLXMESAcontext *shareContext = (__GLXMESAcontext *) baseShareContext;
    XMesaVisual xm_vis;
    XMesaContext xm_share;

    context = malloc (sizeof (__GLXMESAcontext));
    if (context == NULL)
	return NULL;

    memset(context, 0, sizeof *context);

    context->base.pGlxScreen = screen;
    context->base.modes      = modes;

    context->base.destroy        = __glXMesaContextDestroy;
    context->base.makeCurrent    = __glXMesaContextMakeCurrent;
    context->base.loseCurrent    = __glXMesaContextLoseCurrent;
    context->base.copy           = __glXMesaContextCopy;
    context->base.forceCurrent   = __glXMesaContextForceCurrent;
    context->base.createDrawable = __glXMesaContextCreateDrawable;

    xm_vis = find_mesa_visual(screen, modes->visualID);
    if (!xm_vis) {
	ErrorF("find_mesa_visual returned NULL for visualID = 0x%04x\n",
	       modes->visualID);
	free(context);
	return NULL;
    }

    xm_share = shareContext ? shareContext->xmesa : NULL;
    context->xmesa = XMesaCreateContext(xm_vis, xm_share);
    if (!context->xmesa) {
	free(context);
	return NULL;
    }

    return &context->base;
}

static void
__glXMesaScreenDestroy(__GLXscreen *screen)
{
    __GLXMESAscreen *mesaScreen = (__GLXMESAscreen *) screen;
    int i;

    for (i = 0; i < screen->numVisuals; i++) {
	if (mesaScreen->xm_vis[i])
	    XMesaDestroyVisual(mesaScreen->xm_vis[i]);
    }

    free(mesaScreen->xm_vis);

    __glXScreenDestroy(screen);

    free(screen);
}

static XMesaVisual
find_mesa_visual(__GLXscreen *screen, VisualID vid)
{
    __GLXMESAscreen *mesaScreen = (__GLXMESAscreen *) screen;
    const __GLcontextModes *modes;
    unsigned i = 0;

    for ( modes = screen->modes ; modes != NULL ; modes = modes->next ) {
	if ( modes->visualID == vid ) {
	    break;
	}

	i++;
    }

    return (modes != NULL) ? mesaScreen->xm_vis[i] : NULL;
}

static void init_screen_visuals(__GLXMESAscreen *screen)
{
    ScreenPtr pScreen = screen->base.pScreen;
    __GLcontextModes *modes;
    XMesaVisual *pXMesaVisual;
    int *used;
    int i, j, size;

    /* Alloc space for the list of XMesa visuals */
    size = screen->base.numVisuals * sizeof(XMesaVisual);
    pXMesaVisual = (XMesaVisual *) malloc(size);
    memset(pXMesaVisual, 0, size);

    /* FIXME: Change 'used' to be a array of bits (rather than of ints),
     * FIXME: create a stack array of 8 or 16 bytes.  If 'numVisuals' is less
     * FIXME: than 64 or 128 the stack array can be used instead of calling
     * FIXME: __glXMalloc / __glXFree.  If nothing else, convert 'used' to
     * FIXME: array of bytes instead of ints!
     */
    used = (int *) malloc(pScreen->numVisuals * sizeof(int));
    memset(used, 0, pScreen->numVisuals * sizeof(int));

    i = 0;
    for ( modes = screen->base.modes; modes != NULL; modes = modes->next ) {
	const int vis_class = _gl_convert_to_x_visual_type( modes->visualType );
	const int nplanes = (modes->rgbBits - modes->alphaBits);
	const VisualPtr pVis = pScreen->visuals;

	for (j = 0; j < pScreen->numVisuals; j++) {
	    if (pVis[j].class     == vis_class &&
		pVis[j].nplanes   == nplanes &&
		pVis[j].redMask   == modes->redMask &&
		pVis[j].greenMask == modes->greenMask &&
		pVis[j].blueMask  == modes->blueMask &&
		!used[j]) {

		/* Create the XMesa visual */
		pXMesaVisual[i] =
		    XMesaCreateVisual(pScreen,
				      &pVis[j],
				      modes->rgbMode,
				      (modes->alphaBits > 0),
				      modes->doubleBufferMode,
				      modes->stereoMode,
				      GL_TRUE, /* ximage_flag */
				      modes->depthBits,
				      modes->stencilBits,
				      modes->accumRedBits,
				      modes->accumGreenBits,
				      modes->accumBlueBits,
				      modes->accumAlphaBits,
				      modes->samples,
				      modes->level,
				      modes->visualRating);
		/* Set the VisualID */
		modes->visualID = pVis[j].vid;

		/* Mark this visual used */
		used[j] = 1;
		break;
	    }
	}

	if ( j == pScreen->numVisuals ) {
	    ErrorF("No matching visual for __GLcontextMode with "
		   "visual class = %d (%d), nplanes = %u\n",
		   vis_class, 
		   modes->visualType,
		   (modes->rgbBits - modes->alphaBits) );
	}
	else if ( modes->visualID == -1 ) {
	    FatalError( "Matching visual found, but visualID still -1!\n" );
	}

	i++;
    }

    free(used);

    screen->xm_vis = pXMesaVisual;
}

static __GLXscreen *
__glXMesaScreenProbe(ScreenPtr pScreen)
{
    __GLXMESAscreen *screen;

    screen = malloc(sizeof *screen);
    if (screen == NULL)
	return NULL;

    __glXScreenInit(&screen->base, pScreen);

    screen->base.destroy       = __glXMesaScreenDestroy;
    screen->base.createContext = __glXMesaScreenCreateContext;
    screen->base.pScreen       = pScreen;

    /*
     * Find the GLX visuals that are supported by this screen and create
     * XMesa's visuals.
     */
    init_screen_visuals(screen);

    return &screen->base;
}

__GLXprovider __glXMesaProvider = {
    __glXMesaScreenProbe,
    "MESA",
    NULL
};

__GLXprovider *
GlxGetMesaProvider (void)
{
    return &__glXMesaProvider;
}
