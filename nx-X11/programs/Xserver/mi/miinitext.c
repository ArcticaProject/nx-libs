/***********************************************************

Copyright 1987, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.


Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#ifdef HAVE_DMX_CONFIG_H
#include <dmx-config.h>
#endif

#ifdef HAVE_XNEST_CONFIG_H
#include <xnest-config.h>
#undef DPMSExtension
#endif

#include "misc.h"
#include "extension.h"
#include "micmap.h"

extern Bool noTestExtensions;

#ifdef BIGREQS
extern Bool noBigReqExtension;
#endif
#ifdef COMPOSITE
extern Bool noCompositeExtension;
#endif
#ifdef DAMAGE
extern Bool noDamageExtension;
#endif
#ifdef DBE
extern Bool noDbeExtension;
#endif
#ifdef DPMSExtension
extern Bool noDPMSExtension;
#endif
#ifdef GLXEXT
extern Bool noGlxExtension;
#endif
#ifdef SCREENSAVER
extern Bool noScreenSaverExtension;
#endif
#ifdef MITSHM
extern Bool noMITShmExtension;
#endif
#ifdef RANDR
extern Bool noRRExtension;
#endif
#ifdef RENDER
extern Bool noRenderExtension;
#endif
#ifdef SHAPE
extern Bool noShapeExtension;
#endif
#ifdef XCSECURITY
extern Bool noSecurityExtension;
#endif
#ifdef XSYNC
extern Bool noSyncExtension;
#endif
#ifdef RES
extern Bool noResExtension;
#endif
#ifdef XCMISC
extern Bool noXCMiscExtension;
#endif
#ifdef XF86BIGFONT
extern Bool noXFree86BigfontExtension;
#endif
#ifdef XF86DRI
extern Bool noXFree86DRIExtension;
#endif
#ifdef XFIXES
extern Bool noXFixesExtension;
#endif
#ifdef XKB
/* |noXkbExtension| is defined in xc/programs/Xserver/xkb/xkbInit.c */
extern Bool noXkbExtension;
#endif
#ifdef PANORAMIX
extern Bool noPanoramiXExtension;
#endif
#ifdef XINPUT
extern Bool noXInputExtension;
#endif
#ifdef XIDLE
extern Bool noXIdleExtension;
#endif
#ifdef XV
extern Bool noXvExtension;
#endif

typedef void (*InitExtension)(void);

#ifdef MITSHM
#define _XSHM_SERVER_
#include <X11/extensions/shmstr.h>
#endif
#ifdef XTEST
#define _XTEST_SERVER_
#include <nx-X11/extensions/xtestconst.h>
#endif
#ifdef XKB
#include <nx-X11/extensions/XKB.h>
#endif
#ifdef XACE
#include "xace.h"
#endif
#ifdef XCSECURITY
#define _SECURITY_SERVER
#include <nx-X11/extensions/securstr.h>
#endif
#ifdef PANORAMIX
#include <nx-X11/extensions/panoramiXproto.h>
#endif
#ifdef XF86BIGFONT
#include <nx-X11/extensions/xf86bigfproto.h>
#endif
#ifdef RES
#include <nx-X11/extensions/XResproto.h>
#endif

/* FIXME: this whole block of externs should be from the appropriate headers */
#ifdef SHAPE
extern void ShapeExtensionInit(void);
#endif
#ifdef MITSHM
extern void ShmExtensionInit(void);
#endif
#ifdef PANORAMIX
extern void PanoramiXExtensionInit(void);
#endif
#ifdef XINPUT
extern void XInputExtensionInit(void);
#endif
#ifdef XTEST
extern void XTestExtensionInit(void);
#endif
#ifdef BIGREQS
extern void BigReqExtensionInit(void);
#endif
#ifdef XIDLE
extern void XIdleExtensionInit(void);
#endif
#ifdef SCREENSAVER
extern void ScreenSaverExtensionInit (void);
#endif
#ifdef XV
extern void XvExtensionInit(void);
extern void XvMCExtensionInit(void);
#endif
#ifdef XSYNC
extern void SyncExtensionInit(void);
#endif
#ifdef XKB
extern void XkbExtensionInit(void);
#endif
#ifdef XCMISC
extern void XCMiscExtensionInit(void);
#endif
#ifdef XRECORD
extern void RecordExtensionInit(void);
#endif
#ifdef DBE
extern void DbeExtensionInit(void);
#endif
#ifdef XACE
extern void XaceExtensionInit(void);
#endif
#ifdef XCSECURITY
extern void SecurityExtensionSetup(void);
extern void SecurityExtensionInit(void);
#endif
#ifdef XF86BIGFONT
extern void XFree86BigfontExtensionInit(void);
#endif
#ifdef GLXEXT
// typedef struct __GLXprovider __GLXprovider;
#ifdef INXDARWINAPP
extern __GLXprovider* __DarwinglXMesaProvider;
extern void DarwinGlxPushProvider(__GLXprovider *impl);
extern void DarwinGlxExtensionInit(INITARGS);
extern void DarwinGlxWrapInitVisuals(miInitVisualsProcPtr *);
#else
// extern __GLXprovider __glXMesaProvider;
// extern void GlxPushProvider(__GLXprovider *impl);
extern void GlxExtensionInit(void);
extern void GlxWrapInitVisuals(miInitVisualsProcPtr *);
#endif // INXDARWINAPP
#endif // GLXEXT
#ifdef XF86DRI
extern void XFree86DRIExtensionInit(void);
#endif
#ifdef DPMSExtension
extern void DPMSExtensionInit(void);
#endif
#ifdef RENDER
extern void RenderExtensionInit(void);
#endif
#ifdef RANDR
extern void RRExtensionInit(void);
#endif
#ifdef RES
extern void ResExtensionInit(void);
#endif
#ifdef DMXEXT
extern void DMXExtensionInit(void);
#endif
#ifdef XFIXES
extern void XFixesExtensionInit(void);
#endif
#ifdef DAMAGE
extern void DamageExtensionInit(void);
#endif
#ifdef COMPOSITE
extern void CompositeExtensionInit(void);
#endif

/* The following is only a small first step towards run-time
 * configurable extensions.
 */
typedef struct {
    char *name;
    Bool *disablePtr;
} ExtensionToggle;

static ExtensionToggle ExtensionToggleList[] =
{
    /* sort order is extension name string as shown in xdpyinfo */
#ifdef BIGREQS
    { "BIG-REQUESTS", &noBigReqExtension },
#endif
#ifdef COMPOSITE
    { "Composite", &noCompositeExtension },
#endif
#ifdef DAMAGE
    { "DAMAGE", &noDamageExtension },
#endif
#ifdef DBE
    { "DOUBLE-BUFFER", &noDbeExtension },
#endif
#ifdef DPMSExtension
    { "DPMS", &noDPMSExtension },
#endif
#ifdef GLXEXT
    { "GLX", &noGlxExtension },
#endif
#ifdef SCREENSAVER
    { "MIT-SCREEN-SAVER", &noScreenSaverExtension },
#endif
#ifdef MITSHM
    { SHMNAME, &noMITShmExtension },
#endif
#ifdef RANDR
    { "RANDR", &noRRExtension },
#endif
#ifdef RENDER
    { "RENDER", &noRenderExtension },
#endif
#ifdef SHAPE
    { "SHAPE", &noShapeExtension },
#endif
#ifdef XCSECURITY
    { "SECURITY", &noSecurityExtension },
#endif
#ifdef XSYNC
    { "SYNC", &noSyncExtension },
#endif
#ifdef RES
    { "X-Resource", &noResExtension },
#endif
#ifdef XCMISC
    { "XC-MISC", &noXCMiscExtension },
#endif
#ifdef XF86BIGFONT
    { "XFree86-Bigfont", &noXFree86BigfontExtension },
#endif
#ifdef XF86DRI
    { "XFree86-DRI", &noXFree86DRIExtension },
#endif
#ifdef XFIXES
    { "XFIXES", &noXFixesExtension },
#endif
#ifdef PANORAMIX
    { "XINERAMA", &noPanoramiXExtension },
#endif
#ifdef XINPUT
    { "XInputExtension", &noXInputExtension },
#endif
#ifdef XKB
    { "XKEYBOARD", &noXkbExtension },
#endif
    { "XTEST", &noTestExtensions },
#ifdef XV
    { "XVideo", &noXvExtension },
#endif
    { NULL, NULL }
};

Bool EnableDisableExtension(char *name, Bool enable)
{
    ExtensionToggle *ext = &ExtensionToggleList[0];

    for (ext = &ExtensionToggleList[0]; ext->name != NULL; ext++) {
	if (strcmp(name, ext->name) == 0) {
	    *ext->disablePtr = !enable;
	    return TRUE;
	}
    }

    return FALSE;
}

void EnableDisableExtensionError(char *name, Bool enable)
{
    ExtensionToggle *ext = &ExtensionToggleList[0];

    ErrorF("Extension \"%s\" is not recognized\n", name);
    ErrorF("Only the following extensions can be run-time %s:\n",
	   enable ? "enabled" : "disabled");
    for (ext = &ExtensionToggleList[0]; ext->name != NULL; ext++)
	ErrorF("    %s\n", ext->name);
}


/*ARGSUSED*/
void
InitExtensions(argc, argv)
    int		argc;
    char	*argv[];
{
#ifdef XCSECURITY
    SecurityExtensionSetup();
#endif
#ifdef PANORAMIX
# if !defined(PRINT_ONLY_SERVER) && !defined(NO_PANORAMIX)
  if (!noPanoramiXExtension) PanoramiXExtensionInit();
# endif
#endif
#ifdef SHAPE
    if (!noShapeExtension) ShapeExtensionInit();
#endif
#ifdef MITSHM
    if (!noMITShmExtension) ShmExtensionInit();
#endif
#if defined(XINPUT)
    if (!noXInputExtension) XInputExtensionInit();
#endif
#ifdef XTEST
    if (!noTestExtensions) XTestExtensionInit();
#endif
#ifdef BIGREQS
    if (!noBigReqExtension) BigReqExtensionInit();
#endif
#ifdef XIDLE
    if (!noXIdleExtension) XIdleExtensionInit();
#endif
#if defined(SCREENSAVER) && !defined(PRINT_ONLY_SERVER)
    if (!noScreenSaverExtension) ScreenSaverExtensionInit ();
#endif
#ifdef XV
    if (!noXvExtension) {
      XvExtensionInit();
      XvMCExtensionInit();
    }
#endif
#ifdef XSYNC
    if (!noSyncExtension) SyncExtensionInit();
#endif
#if defined(XKB) && !defined(PRINT_ONLY_SERVER)
    if (!noXkbExtension) XkbExtensionInit();
#endif
#ifdef XCMISC
    if (!noXCMiscExtension) XCMiscExtensionInit();
#endif
#ifdef XRECORD
    if (!noTestExtensions) RecordExtensionInit(); 
#endif
#ifdef DBE
    if (!noDbeExtension) DbeExtensionInit();
#endif
#ifdef XACE
    XaceExtensionInit();
#endif
#ifdef XCSECURITY
    if (!noSecurityExtension) SecurityExtensionInit();
#endif
#if defined(DPMSExtension) && !defined(NO_HW_ONLY_EXTS)
    if (!noDPMSExtension) DPMSExtensionInit();
#endif
#ifdef XF86BIGFONT
    if (!noXFree86BigfontExtension) XFree86BigfontExtensionInit();
#endif
#if !defined(PRINT_ONLY_SERVER) && !defined(NO_HW_ONLY_EXTS)
#ifdef XF86DRI
    if (!noXFree86DRIExtension) XFree86DRIExtensionInit();
#endif
#endif

#ifdef GLXEXT
#ifdef INXDARWINAPP
    DarwinGlxPushProvider(__DarwinglXMesaProvider);
    if (!noGlxExtension) DarwinGlxExtensionInit();
#else
    // GlxPushProvider(&__glXMesaProvider);
    if (!noGlxExtension) GlxExtensionInit();
#endif // INXDARWINAPP
#endif // GLXEXT
#ifdef XFIXES
    /* must be before Render to layer DisplayCursor correctly */
    if (!noXFixesExtension) XFixesExtensionInit();
#endif
#ifdef RENDER
    if (!noRenderExtension) RenderExtensionInit();
#endif
#ifdef RANDR
    if (!noRRExtension) RRExtensionInit();
#endif
#ifdef RES
    if (!noResExtension) ResExtensionInit();
#endif
#ifdef DMXEXT
    DMXExtensionInit(); /* server-specific extension, cannot be disabled */
#endif
#ifdef COMPOSITE
    if (!noCompositeExtension) CompositeExtensionInit();
#endif
#ifdef DAMAGE
    if (!noDamageExtension) DamageExtensionInit();
#endif
}

void
InitVisualWrap()
{
    miResetInitVisuals();
#ifdef GLXEXT
#ifdef INXDARWINAPP
    DarwinGlxWrapInitVisuals(&miInitVisualsProc);
#endif // INXDARWINAPP
#endif // GLXEXT
}
