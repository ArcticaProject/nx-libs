LIBRARY Xrandr
VERSION LIBRARY_VERSION
EXPORTS
#ifndef __UNIXOS2__
XRRCurrentConfig
#endif
 XRRFindDisplay
#ifndef __UNIXOS2__
XRRFreeScreenInfo
#endif
 XRRGetScreenInfo
 XRRQueryExtension
 XRRQueryVersion
 XRRRootToScreen
 XRRRotations
#ifndef __UNIXOS2__
XRRScreenChangeSelectInput
#endif
 XRRSetScreenConfig
 XRRSizes
 XRRTimes
#ifndef __UNIXOS2__
XRRVisualIDToVisual
XRRVisualToDepth
#else
XRRConfigCurrentConfiguration
XRRConfigSizes
XRRConfigRotations
XRRSelectInput
XRRFreeScreenConfigInfo
XRRUpdateConfiguration
XRRConfigCurrentRate
XRRConfigRates
XRRSetScreenConfigAndRate
#endif  /* __UNIXOS2__
/* $XFree86: xc/lib/Xrandr/Xrandr-def.cpp,v 1.1 2001/08/19 15:22:58 alanh Exp $ */
