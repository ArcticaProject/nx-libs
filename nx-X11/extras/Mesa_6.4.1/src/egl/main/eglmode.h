#ifndef EGLMODE_INCLUDED
#define EGLMODE_INCLUDED

#include "egltypedefs.h"


/**
 * Data structure which corresponds to an EGLModeMESA.
 */
struct _egl_mode
{
   EGLModeMESA Handle;     /* the public/opaque handle which names this mode */
   EGLint Width, Height;   /* size in pixels */
   EGLint RefreshRate;     /* rate * 1000.0 */
   EGLBoolean Stereo;
   const char *Name;

   /* Other possible attributes */
   /* interlaced */
   /* external sync */
};


extern _EGLMode *
_eglLookupMode(EGLDisplay dpy, EGLModeMESA mode);


extern _EGLMode *
_eglAddMode(_EGLScreen *screen, EGLint width, EGLint height,
            EGLint refreshRate, const char *name);


extern EGLBoolean
_eglChooseModeMESA(_EGLDriver *drv, EGLDisplay dpy, EGLScreenMESA screen,
                   const EGLint *attrib_list, EGLModeMESA *modes,
                   EGLint modes_size, EGLint *num_modes);


extern EGLBoolean
_eglGetModesMESA(_EGLDriver *drv, EGLDisplay dpy, EGLScreenMESA screen,
                 EGLModeMESA *modes, EGLint modes_size, EGLint *num_modes);


extern EGLBoolean
_eglGetModeAttribMESA(_EGLDriver *drv, EGLDisplay dpy, EGLModeMESA mode,
                      EGLint attribute, EGLint *value);


extern const char *
_eglQueryModeStringMESA(_EGLDriver *drv, EGLDisplay dpy, EGLModeMESA mode);


#endif /* EGLMODE_INCLUDED */
