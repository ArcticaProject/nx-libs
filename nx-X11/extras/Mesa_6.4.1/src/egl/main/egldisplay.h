#ifndef EGLDISPLAY_INCLUDED
#define EGLDISPLAY_INCLUDED


#include "egltypedefs.h"


struct _egl_display 
{
   EGLDisplay Handle;

   char *Name;
   _EGLDriver *Driver;

   EGLint NumScreens;
   _EGLScreen **Screens;  /* array [NumScreens] */

   EGLint NumConfigs;
   _EGLConfig *Configs;  /* array [NumConfigs] */
};


extern _EGLDisplay *
_eglNewDisplay(NativeDisplayType displayName);


extern _EGLDisplay *
_eglLookupDisplay(EGLDisplay dpy);


extern _EGLDisplay *
_eglGetCurrentDisplay(void);


extern void
_eglCleanupDisplay(_EGLDisplay *disp);


extern EGLBoolean 
_eglQueryDisplayMESA(_EGLDriver *drv, EGLDisplay dpy, EGLint attrib, EGLint *value);



#endif /* EGLDISPLAY_INCLUDED */
