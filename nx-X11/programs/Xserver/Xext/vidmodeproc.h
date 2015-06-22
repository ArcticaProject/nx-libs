/* $XFree86: xc/programs/Xserver/Xext/vidmodeproc.h,v 1.4 1999/12/13 01:39:40 robin Exp $ */

/* Prototypes for DGA functions that the DDX must provide */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifndef _VIDMODEPROC_H_
#define _VIDMODEPROC_H_


typedef enum {
    VIDMODE_H_DISPLAY,
    VIDMODE_H_SYNCSTART,
    VIDMODE_H_SYNCEND,
    VIDMODE_H_TOTAL,
    VIDMODE_H_SKEW,
    VIDMODE_V_DISPLAY,
    VIDMODE_V_SYNCSTART,
    VIDMODE_V_SYNCEND,
    VIDMODE_V_TOTAL,
    VIDMODE_FLAGS,
    VIDMODE_CLOCK
} VidModeSelectMode;

typedef enum {
    VIDMODE_MON_VENDOR,
    VIDMODE_MON_MODEL,
    VIDMODE_MON_NHSYNC,
    VIDMODE_MON_NVREFRESH,
    VIDMODE_MON_HSYNC_LO,
    VIDMODE_MON_HSYNC_HI,
    VIDMODE_MON_VREFRESH_LO,
    VIDMODE_MON_VREFRESH_HI
} VidModeSelectMonitor;

typedef union {
  void * ptr;
  int i;
  float f;
} vidMonitorValue;

void XFree86VidModeExtensionInit(void);

Bool VidModeAvailable(int scrnIndex);
Bool VidModeGetCurrentModeline(int scrnIndex, void **mode, int *dotClock);
Bool VidModeGetFirstModeline(int scrnIndex, void **mode, int *dotClock);
Bool VidModeGetNextModeline(int scrnIndex, void **mode, int *dotClock);
Bool VidModeDeleteModeline(int scrnIndex, void * mode);
Bool VidModeZoomViewport(int scrnIndex, int zoom);
Bool VidModeGetViewPort(int scrnIndex, int *x, int *y);
Bool VidModeSetViewPort(int scrnIndex, int x, int y);
Bool VidModeSwitchMode(int scrnIndex, void * mode);
Bool VidModeLockZoom(int scrnIndex, Bool lock);
Bool VidModeGetMonitor(int scrnIndex, void **monitor);
int VidModeGetNumOfClocks(int scrnIndex, Bool *progClock);
Bool VidModeGetClocks(int scrnIndex, int *Clocks);
ModeStatus VidModeCheckModeForMonitor(int scrnIndex, void * mode);
ModeStatus VidModeCheckModeForDriver(int scrnIndex, void * mode);
void VidModeSetCrtcForMode(int scrnIndex, void * mode);
Bool VidModeAddModeline(int scrnIndex, void * mode);
int VidModeGetDotClock(int scrnIndex, int Clock);
int VidModeGetNumOfModes(int scrnIndex);
Bool VidModeSetGamma(int scrnIndex, float red, float green, float blue);
Bool VidModeGetGamma(int scrnIndex, float *red, float *green, float *blue);
void * VidModeCreateMode(void);
void VidModeCopyMode(void * modefrom, void * modeto);
int VidModeGetModeValue(void * mode, int valtyp);
void VidModeSetModeValue(void * mode, int valtyp, int val);
vidMonitorValue VidModeGetMonitorValue(void * monitor, int valtyp, int indx);
Bool VidModeSetGammaRamp(int, int, CARD16 *, CARD16 *, CARD16 *);
Bool VidModeGetGammaRamp(int, int, CARD16 *, CARD16 *, CARD16 *);
int VidModeGetGammaRampSize(int scrnIndex);

#endif


