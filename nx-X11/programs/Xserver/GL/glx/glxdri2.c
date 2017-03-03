/*
 * Copyright © 2007 Red Hat, Inc
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of Red Hat,
 * Inc not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission.  Red Hat, Inc makes no representations about the
 * suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * RED HAT, INC DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL RED HAT, INC BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <dlfcn.h>

#include <drm.h>
#include <GL/gl.h>
#include <GL/internal/dri_interface.h>
#include <GL/glxtokens.h>

#include <windowstr.h>
#include <os.h>

#define _XF86DRI_SERVER_
#include <xf86drm.h>
#include <xf86.h>
#include <dri2.h>

#include "glxserver.h"
#include "glxutil.h"
#include "glxdricommon.h"

#include "g_disptab.h"
#include "glapitable.h"
#include "glapi.h"
#include "glthread.h"
#include "dispatch.h"
#include "extension_string.h"

typedef struct __GLXDRIscreen   __GLXDRIscreen;
typedef struct __GLXDRIcontext  __GLXDRIcontext;
typedef struct __GLXDRIdrawable __GLXDRIdrawable;

struct __GLXDRIscreen {
    __GLXscreen		 base;
    __DRIscreen		*driScreen;
    void		*driver;
    int			 fd;

    xf86EnterVTProc	*enterVT;
    xf86LeaveVTProc	*leaveVT;

    const __DRIcoreExtension *core;
    const __DRIcopySubBufferExtension *copySubBuffer;
    const __DRIswapControlExtension *swapControl;
    const __DRItexBufferExtension *texBuffer;

    unsigned char glx_enable_bits[__GLX_EXT_BYTES];
};

struct __GLXDRIcontext {
    __GLXcontext	 base;
    __DRIcontext	*driContext;
};

struct __GLXDRIdrawable {
    __GLXdrawable	 base;
    __DRIdrawable	*driDrawable;
    __GLXDRIscreen	*screen;
};

static void
__glXDRIdrawableDestroy(__GLXdrawable *drawable)
{
    __GLXDRIdrawable *private = (__GLXDRIdrawable *) drawable;
    const __DRIcoreExtension *core = private->screen->core;
    
    (*core->destroyDrawable)(private->driDrawable);

    /* If the X window was destroyed, the dri DestroyWindow hook will
     * aready have taken care of this, so only call if pDraw isn't NULL. */
    if (drawable->pDraw != NULL)
	DRI2DestroyDrawable(drawable->pDraw);

    free(private);
}

static GLboolean
__glXDRIdrawableResize(__GLXdrawable *glxPriv)
{
    /* Nothing to do here, the DRI driver asks the server for drawable
     * geometry when it sess the SAREA timestamps change.*/

    return GL_TRUE;
}

static GLboolean
__glXDRIdrawableSwapBuffers(__GLXdrawable *drawable)
{
    __GLXDRIdrawable *private = (__GLXDRIdrawable *) drawable;
    const __DRIcoreExtension *core = private->screen->core;

    (*core->swapBuffers)(private->driDrawable);

    return TRUE;
}


static int
__glXDRIdrawableSwapInterval(__GLXdrawable *drawable, int interval)
{
    __GLXDRIdrawable *private = (__GLXDRIdrawable *) drawable;
    const __DRIswapControlExtension *swapControl = private->screen->swapControl;

    if (swapControl)
	swapControl->setSwapInterval(private->driDrawable, interval);

    return 0;
}


static void
__glXDRIdrawableCopySubBuffer(__GLXdrawable *basePrivate,
			       int x, int y, int w, int h)
{
    __GLXDRIdrawable *private = (__GLXDRIdrawable *) basePrivate;
    const __DRIcopySubBufferExtension *copySubBuffer =
	    private->screen->copySubBuffer;

    if (copySubBuffer)
	(*copySubBuffer->copySubBuffer)(private->driDrawable, x, y, w, h);
}

static void
__glXDRIcontextDestroy(__GLXcontext *baseContext)
{
    __GLXDRIcontext *context = (__GLXDRIcontext *) baseContext;
    __GLXDRIscreen *screen = (__GLXDRIscreen *) context->base.pGlxScreen;

    (*screen->core->destroyContext)(context->driContext);
    __glXContextDestroy(&context->base);
    free(context);
}

static int
__glXDRIcontextMakeCurrent(__GLXcontext *baseContext)
{
    __GLXDRIcontext *context = (__GLXDRIcontext *) baseContext;
    __GLXDRIdrawable *draw = (__GLXDRIdrawable *) baseContext->drawPriv;
    __GLXDRIdrawable *read = (__GLXDRIdrawable *) baseContext->readPriv;
    __GLXDRIscreen *screen = (__GLXDRIscreen *) context->base.pGlxScreen;

    return (*screen->core->bindContext)(context->driContext,
					draw->driDrawable,
					read->driDrawable);
}					      

static int
__glXDRIcontextLoseCurrent(__GLXcontext *baseContext)
{
    __GLXDRIcontext *context = (__GLXDRIcontext *) baseContext;
    __GLXDRIscreen *screen = (__GLXDRIscreen *) context->base.pGlxScreen;

    return (*screen->core->unbindContext)(context->driContext);
}

static int
__glXDRIcontextCopy(__GLXcontext *baseDst, __GLXcontext *baseSrc,
		    unsigned long mask)
{
    __GLXDRIcontext *dst = (__GLXDRIcontext *) baseDst;
    __GLXDRIcontext *src = (__GLXDRIcontext *) baseSrc;
    __GLXDRIscreen *screen = (__GLXDRIscreen *) dst->base.pGlxScreen;

    return (*screen->core->copyContext)(dst->driContext,
					src->driContext, mask);
}

static int
__glXDRIcontextForceCurrent(__GLXcontext *baseContext)
{
    __GLXDRIcontext *context = (__GLXDRIcontext *) baseContext;
    __GLXDRIdrawable *draw = (__GLXDRIdrawable *) baseContext->drawPriv;
    __GLXDRIdrawable *read = (__GLXDRIdrawable *) baseContext->readPriv;
    __GLXDRIscreen *screen = (__GLXDRIscreen *) context->base.pGlxScreen;

    return (*screen->core->bindContext)(context->driContext,
					draw->driDrawable,
					read->driDrawable);
}

#ifdef __DRI_TEX_BUFFER

static int
__glXDRIbindTexImage(__GLXcontext *baseContext,
		     int buffer,
		     __GLXdrawable *glxPixmap)
{
    __GLXDRIdrawable *drawable = (__GLXDRIdrawable *) glxPixmap;
    const __DRItexBufferExtension *texBuffer = drawable->screen->texBuffer;
    __GLXDRIcontext *context = (__GLXDRIcontext *) baseContext;

    if (texBuffer == NULL)
        return Success;

    texBuffer->setTexBuffer(context->driContext,
			    glxPixmap->target,
			    drawable->driDrawable);

    return Success;
}

static int
__glXDRIreleaseTexImage(__GLXcontext *baseContext,
			int buffer,
			__GLXdrawable *pixmap)
{
    /* FIXME: Just unbind the texture? */
    return Success;
}

#else

static int
__glXDRIbindTexImage(__GLXcontext *baseContext,
		     int buffer,
		     __GLXdrawable *glxPixmap)
{
    return Success;
}

static int
__glXDRIreleaseTexImage(__GLXcontext *baseContext,
			int buffer,
			__GLXdrawable *pixmap)
{
    return Success;
}

#endif

static __GLXtextureFromPixmap __glXDRItextureFromPixmap = {
    __glXDRIbindTexImage,
    __glXDRIreleaseTexImage
};

static void
__glXDRIscreenDestroy(__GLXscreen *baseScreen)
{
    __GLXDRIscreen *screen = (__GLXDRIscreen *) baseScreen;

    (*screen->core->destroyScreen)(screen->driScreen);

    dlclose(screen->driver);

    __glXScreenDestroy(baseScreen);

    free(screen);
}

static __GLXcontext *
__glXDRIscreenCreateContext(__GLXscreen *baseScreen,
			    __GLXconfig *glxConfig,
			    __GLXcontext *baseShareContext)
{
    __GLXDRIscreen *screen = (__GLXDRIscreen *) baseScreen;
    __GLXDRIcontext *context, *shareContext;
    __GLXDRIconfig *config = (__GLXDRIconfig *) glxConfig;
    const __DRIcoreExtension *core = screen->core;
    __DRIcontext *driShare;

    shareContext = (__GLXDRIcontext *) baseShareContext;
    if (shareContext)
	driShare = shareContext->driContext;
    else
	driShare = NULL;

    context = malloc(sizeof *context);
    if (context == NULL)
	return NULL;

    memset(context, 0, sizeof *context);
    context->base.destroy           = __glXDRIcontextDestroy;
    context->base.makeCurrent       = __glXDRIcontextMakeCurrent;
    context->base.loseCurrent       = __glXDRIcontextLoseCurrent;
    context->base.copy              = __glXDRIcontextCopy;
    context->base.forceCurrent      = __glXDRIcontextForceCurrent;
    context->base.textureFromPixmap = &__glXDRItextureFromPixmap;

    context->driContext =
	(*core->createNewContext)(screen->driScreen,
				  config->driConfig, driShare, context);

    return &context->base;
}

static __GLXdrawable *
__glXDRIscreenCreateDrawable(__GLXscreen *screen,
			     DrawablePtr pDraw,
			     int type,
			     XID drawId,
			     __GLXconfig *glxConfig)
{
    __GLXDRIscreen *driScreen = (__GLXDRIscreen *) screen;
    __GLXDRIconfig *config = (__GLXDRIconfig *) glxConfig;
    __GLXDRIdrawable *private;
    GLboolean retval;
    unsigned int handle, head;

    private = malloc(sizeof *private);
    if (private == NULL)
	return NULL;

    memset(private, 0, sizeof *private);

    private->screen = driScreen;
    if (!__glXDrawableInit(&private->base, screen,
			   pDraw, type, drawId, glxConfig)) {
        free(private);
	return NULL;
    }

    private->base.destroy       = __glXDRIdrawableDestroy;
    private->base.resize        = __glXDRIdrawableResize;
    private->base.swapBuffers   = __glXDRIdrawableSwapBuffers;
    private->base.copySubBuffer = __glXDRIdrawableCopySubBuffer;

    retval = DRI2CreateDrawable(pDraw, &handle, &head);

    private->driDrawable =
	(*driScreen->core->createNewDrawable)(driScreen->driScreen, 
					      config->driConfig,
					      handle, head, private);

    return &private->base;
}

static void dri2ReemitDrawableInfo(__DRIdrawable *draw, unsigned int *tail,
				   void *loaderPrivate)
{
    __GLXDRIdrawable *pdraw = loaderPrivate;

    DRI2ReemitDrawableInfo(pdraw->base.pDraw, tail);
}

static void dri2PostDamage(__DRIdrawable *draw,
			   struct drm_clip_rect *rects,
			   int numRects, void *loaderPrivate)
{ 
    __GLXDRIdrawable *drawable = loaderPrivate;
    DrawablePtr pDraw = drawable->base.pDraw;
    RegionRec region;

    REGION_INIT(pDraw->pScreen, &region, (BoxPtr) rects, numRects);
    REGION_TRANSLATE(pScreen, &region, pDraw->x, pDraw->y);
    DamageDamageRegion(pDraw, &region);
    REGION_UNINIT(pDraw->pScreen, &region);
}

static const __DRIloaderExtension loaderExtension = {
    { __DRI_LOADER, __DRI_LOADER_VERSION },
    dri2ReemitDrawableInfo,
    dri2PostDamage
};

static const __DRIextension *loader_extensions[] = {
    &systemTimeExtension.base,
    &loaderExtension.base,
    NULL
};

static const char dri_driver_path[] = DRI_DRIVER_PATH;

static Bool
glxDRIEnterVT (int index, int flags)
{
    __GLXDRIscreen *screen = (__GLXDRIscreen *) 
	glxGetScreen(screenInfo.screens[index]);

    LogMessage(X_INFO, "AIGLX: Resuming AIGLX clients after VT switch\n");

    if (!(*screen->enterVT) (index, flags))
	return FALSE;
    
    glxResumeClients();

    return TRUE;
}

static void
glxDRILeaveVT (int index, int flags)
{
    __GLXDRIscreen *screen = (__GLXDRIscreen *)
	glxGetScreen(screenInfo.screens[index]);

    LogMessage(X_INFO, "AIGLX: Suspending AIGLX clients for VT switch\n");

    glxSuspendClients();

    return (*screen->leaveVT) (index, flags);
}

static void
initializeExtensions(__GLXDRIscreen *screen)
{
    const __DRIextension **extensions;
    int i;

    extensions = screen->core->getExtensions(screen->driScreen);

    for (i = 0; extensions[i]; i++) {
#ifdef __DRI_COPY_SUB_BUFFER
	if (strcmp(extensions[i]->name, __DRI_COPY_SUB_BUFFER) == 0) {
	    screen->copySubBuffer =
		(const __DRIcopySubBufferExtension *) extensions[i];
	    __glXEnableExtension(screen->glx_enable_bits,
				 "GLX_MESA_copy_sub_buffer");
	    
	    LogMessage(X_INFO, "AIGLX: enabled GLX_MESA_copy_sub_buffer\n");
	}
#endif

#ifdef __DRI_SWAP_CONTROL
	if (strcmp(extensions[i]->name, __DRI_SWAP_CONTROL) == 0) {
	    screen->swapControl =
		(const __DRIswapControlExtension *) extensions[i];
	    __glXEnableExtension(screen->glx_enable_bits,
				 "GLX_SGI_swap_control");
	    __glXEnableExtension(screen->glx_enable_bits,
				 "GLX_MESA_swap_control");
	    
	    LogMessage(X_INFO, "AIGLX: enabled GLX_SGI_swap_control and GLX_MESA_swap_control\n");
	}
#endif

#ifdef __DRI_TEX_BUFFER
	if (strcmp(extensions[i]->name, __DRI_TEX_BUFFER) == 0) {
	    screen->texBuffer =
		(const __DRItexBufferExtension *) extensions[i];
	    /* GLX_EXT_texture_from_pixmap is always enabled. */
	    LogMessage(X_INFO, "AIGLX: GLX_EXT_texture_from_pixmap backed by buffer objects\n");
	}
#endif
	/* Ignore unknown extensions */
    }
}

static __GLXscreen *
__glXDRIscreenProbe(ScreenPtr pScreen)
{
    const char *driverName;
    __GLXDRIscreen *screen;
    char filename[128];
    size_t buffer_size;
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    unsigned int sareaHandle;
    const __DRIextension **extensions;
    const __DRIconfig **driConfigs;
    int i;

    screen = malloc(sizeof *screen);
    if (screen == NULL)
	return NULL;
    memset(screen, 0, sizeof *screen);

    if (!xf86LoaderCheckSymbol("DRI2Connect") ||
	!DRI2Connect(pScreen, &screen->fd, &driverName, &sareaHandle)) {
	LogMessage(X_INFO,
		   "AIGLX: Screen %d is not DRI2 capable\n", pScreen->myNum);
	return NULL;
    }

    screen->base.destroy        = __glXDRIscreenDestroy;
    screen->base.createContext  = __glXDRIscreenCreateContext;
    screen->base.createDrawable = __glXDRIscreenCreateDrawable;
    screen->base.swapInterval   = __glXDRIdrawableSwapInterval;
    screen->base.pScreen       = pScreen;

    __glXInitExtensionEnableBits(screen->glx_enable_bits);

    snprintf(filename, sizeof filename,
	     "%s/%s_dri.so", dri_driver_path, driverName);

    screen->driver = dlopen(filename, RTLD_LAZY | RTLD_LOCAL);
    if (screen->driver == NULL) {
	LogMessage(X_ERROR, "AIGLX error: dlopen of %s failed (%s)\n",
		   filename, dlerror());
        goto handle_error;
    }

    extensions = dlsym(screen->driver, __DRI_DRIVER_EXTENSIONS);
    if (extensions == NULL) {
	LogMessage(X_ERROR, "AIGLX error: %s exports no extensions (%s)\n",
		   driverName, dlerror());
	goto handle_error;
    }
    
    for (i = 0; extensions[i]; i++) {
        if (strcmp(extensions[i]->name, __DRI_CORE) == 0 &&
	    extensions[i]->version >= __DRI_CORE_VERSION) {
		screen->core = (const __DRIcoreExtension *) extensions[i];
	}
    }

    if (screen->core == NULL) {
	LogMessage(X_ERROR, "AIGLX error: %s exports no DRI extension\n",
		   driverName);
	goto handle_error;
    }

    screen->driScreen =
	(*screen->core->createNewScreen)(pScreen->myNum,
					 screen->fd,
					 sareaHandle,
					 loader_extensions,
					 &driConfigs,
					 screen);

    if (screen->driScreen == NULL) {
	LogMessage(X_ERROR, "AIGLX error: Calling driver entry point failed");
	goto handle_error;
    }

    initializeExtensions(screen);

    screen->base.fbconfigs = glxConvertConfigs(screen->core, driConfigs);

    __glXScreenInit(&screen->base, pScreen);

    buffer_size = __glXGetExtensionString(screen->glx_enable_bits, NULL);
    if (buffer_size > 0) {
	if (screen->base.GLXextensions != NULL) {
	    free(screen->base.GLXextensions);
	}

	screen->base.GLXextensions = xnfalloc(buffer_size);
	(void) __glXGetExtensionString(screen->glx_enable_bits, 
				       screen->base.GLXextensions);
    }

    screen->enterVT = pScrn->EnterVT;
    pScrn->EnterVT = glxDRIEnterVT; 
    screen->leaveVT = pScrn->LeaveVT;
    pScrn->LeaveVT = glxDRILeaveVT;

    LogMessage(X_INFO,
	       "AIGLX: Loaded and initialized %s\n", filename);

    return &screen->base;

 handle_error:
    if (screen->driver)
        dlclose(screen->driver);

    free(screen);

    LogMessage(X_ERROR, "AIGLX: reverting to software rendering\n");

    return NULL;
}

__GLXprovider __glXDRI2Provider = {
    __glXDRIscreenProbe,
    "DRI2",
    NULL
};
