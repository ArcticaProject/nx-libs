/**************************************************************************

Copyright 1998-1999 Precision Insight, Inc., Cedar Park, Texas.
Copyright (c) 2002 Apple Computer, Inc.
Copyright (c) 2004 Torrey T. Lyons
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
/* $XFree86: xc/lib/GL/apple/dri_glx.c,v 1.2 2004/04/21 04:59:40 torrey Exp $ */

/*
 * Authors:
 *   Kevin E. Martin <kevin@precisioninsight.com>
 *   Brian Paul <brian@precisioninsight.com>
 *
 */

#ifdef GLX_DIRECT_RENDERING

#include <unistd.h>
#include <X11/Xlibint.h>
#include <X11/extensions/Xext.h>
#include "extutil.h"
#include "glxclient.h"
#include "appledri.h"
#include <stdio.h>
#include "dri_glx.h"
#include <sys/types.h>
#include <stdarg.h>


/* Apple OpenGL "driver" information. */
static const char *__driAppleDriverName = "apple";
static const int __driAppleDriverMajor = 1;
static const int __driAppleDriverMinor = 0;
static const int __driAppleDriverPatch = 0;


/*
 * printf wrappers
 */

static void InfoMessageF(const char *f, ...)
{
    va_list args;
    const char *env;

    if ((env = getenv("LIBGL_DEBUG")) && strstr(env, "verbose")) {
        fprintf(stderr, "libGL: ");
        va_start(args, f);
        vfprintf(stderr, f, args);
        va_end(args);
    }
}

static void ErrorMessageF(const char *f, ...)
{
    va_list args;

    if (getenv("LIBGL_DEBUG")) {
        fprintf(stderr, "libGL error: ");
        va_start(args, f);
        vfprintf(stderr, f, args);
        va_end(args);
    }
}


/*
 * Given a display pointer and screen number, determine the name of
 * the DRI driver for the screen. (I.e. "r128", "tdfx", etc).
 * Return True for success, False for failure.
 */
static Bool GetDriverName(Display *dpy, int scrNum, char **driverName)
{
    int directCapable;

    *driverName = NULL;

    if (!XAppleDRIQueryDirectRenderingCapable(dpy, scrNum, &directCapable)) {
        ErrorMessageF("XAppleDRIQueryDirectRenderingCapable failed\n");
        return False;
    }
    if (!directCapable) {
        ErrorMessageF("XAppleDRIQueryDirectRenderingCapable returned false\n");
        return False;
    }

    *driverName = (char *) __driAppleDriverName;

    InfoMessageF("XF86DRIGetClientDriverName: %d.%d.%d %s (screen %d)\n",
                 __driAppleDriverMajor, __driAppleDriverMinor,
                 __driAppleDriverPatch, *driverName, scrNum);

    return True;
}


/*
 * Exported function for querying the DRI driver for a given screen.
 *
 * The returned char pointer points to a static array that will be
 * overwritten by subsequent calls.
 */
const char *glXGetScreenDriver (Display *dpy, int scrNum) {
    static char ret[32];
    char *driverName;
    if (GetDriverName(dpy, scrNum, &driverName)) {
        int len;
        if (!driverName)
            return NULL;
        len = strlen (driverName);
        if (len >= 31)
            return NULL;
        memcpy (ret, driverName, len+1);
        Xfree(driverName);
        return ret;
    }
    return NULL;
}


/*
 * Exported function for obtaining a driver's option list (UTF-8 encoded XML).
 *
 * The returned char pointer points directly into the driver. Therefore
 * it should be treated as a constant.
 *
 * If the driver was not found or does not support configuration NULL is
 * returned.
 *
 * Note: In a standard GLX imlementation the driver remains opened after
 * this function returns.
 */
const char *glXGetDriverConfig(const char *driverName) {
    /* the apple stub driver does not support configuration */
    return NULL;
}


static void driDestroyDisplay(Display *dpy, void *private)
{
    __DRIdisplayPrivate *pdpyp = (__DRIdisplayPrivate *)private;

    if (pdpyp) {
        Xfree(pdpyp->libraryHandles);
        Xfree(pdpyp);
    }
}


void *driCreateDisplay(Display *dpy, __DRIdisplay *pdisp)
{
    const int numScreens = ScreenCount(dpy);
    __DRIdisplayPrivate *pdpyp;
    int eventBase, errorBase;
    int major, minor, patch;
    int scrn;

    /* Initialize these fields to NULL in case we fail.
     * If we don't do this we may later get segfaults trying to free random
     * addresses when the display is closed.
     */
    pdisp->private = NULL;
    pdisp->destroyDisplay = NULL;
    pdisp->createScreen = NULL;

    if (!XAppleDRIQueryExtension(dpy, &eventBase, &errorBase)) {
        return NULL;
    }

    if (!XAppleDRIQueryVersion(dpy, &major, &minor, &patch)) {
        return NULL;
    }

    pdpyp = (__DRIdisplayPrivate *)Xmalloc(sizeof(__DRIdisplayPrivate));
    if (!pdpyp) {
        return NULL;
    }

    pdpyp->driMajor = major;
    pdpyp->driMinor = minor;
    pdpyp->driPatch = patch;

    pdisp->destroyDisplay = driDestroyDisplay;

    /* allocate array of pointers to createScreen funcs */
    pdisp->createScreen = (CreateScreenFunc *) Xmalloc(numScreens * sizeof(void *));
    if (!pdisp->createScreen)
        return NULL;

    /* allocate array of pointers to createScreen funcs */
    pdisp->createNewScreen = (CreateNewScreenFunc *) Xmalloc(numScreens * sizeof(void *));
    if (!pdisp->createNewScreen) {
        Xfree(pdisp->createScreen);
        Xfree(pdpyp);
        return NULL;
    }

    /* allocate array of library handles */
    pdpyp->libraryHandles = (void **) Xmalloc(numScreens * sizeof(void*));
    if (!pdpyp->libraryHandles) {
        Xfree(pdisp->createNewScreen);
        Xfree(pdisp->createScreen);
        Xfree(pdpyp);
        return NULL;
    }

    /* we'll statically bind to the __driCreateScreen function */
    for (scrn = 0; scrn < numScreens; scrn++) {
        pdisp->createScreen[scrn] = __driCreateScreen;
        pdisp->createNewScreen[scrn] = NULL;
        pdpyp->libraryHandles[scrn] = NULL;
    }

    return (void *)pdpyp;
}


/*
** Here we'll query the DRI driver for each screen and let each
** driver register its GL extension functions.  We only have to
** do this once.  But it MUST be done before we create any contexts
** (i.e. before any dispatch tables are created) and before
** glXGetProcAddressARB() returns.
**
** Currently called by glXGetProcAddress(), __glXInitialize(), and
** __glXNewIndirectAPI().
*/
void
__glXRegisterExtensions(void)
{
   static GLboolean alreadyCalled = GL_FALSE;

   if (alreadyCalled) {
      return;
   }

   __driRegisterExtensions ();

   alreadyCalled = GL_TRUE;
}


#endif /* GLX_DIRECT_RENDERING */
