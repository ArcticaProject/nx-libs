
#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifdef SHAPE
extern void ShapeExtensionInit(void);
#include <X11/extensions/shapeproto.h>
#endif

#ifdef XTEST
extern void XTestExtensionInit(void);
#define _XTEST_SERVER_
#include <X11/extensions/xtestproto.h>
#endif

#if 1
extern void XTestExtension1Init(void);
#endif

#ifdef BIGREQS
extern void BigReqExtensionInit(void);
#include <X11/extensions/bigreqsproto.h>
#endif

#ifdef XSYNC
extern void SyncExtensionInit(void);
#define _SYNC_SERVER
#include <nx-X11/extensions/sync.h>
#include <nx-X11/extensions/syncstr.h>
#endif

#ifdef SCREENSAVER
extern void ScreenSaverExtensionInit (void);
#include <X11/extensions/saver.h>
#endif

#ifdef XCMISC
extern void XCMiscExtensionInit(void);
#include <X11/extensions/xcmiscstr.h>
#endif

#ifdef DPMSExtension
extern void DPMSExtensionInit(void);
#include <nx-X11/extensions/dpmsstr.h>
#endif

#ifdef XV
extern void XvExtensionInit(void);
extern void XvMCExtensionInit(void);
extern void XvRegister(void);
#include <X11/extensions/Xv.h>
#include <X11/extensions/XvMC.h>
#endif

#ifdef RES
extern void ResExtensionInit(void);
#include <X11/extensions/XResproto.h>
#endif

#ifdef SHM
extern void ShmExtensionInit(void);
#include <X11/extensions/shmproto.h>
extern void ShmSetPixmapFormat(
    ScreenPtr pScreen,
    int format);
extern void ShmRegisterFuncs(
    ScreenPtr pScreen,
    ShmFuncsPtr funcs);
#endif

#if 1
extern void SecurityExtensionInit(void);
#endif

#if 1
extern void XagExtensionInit(void);
#endif

#if 1
extern void XpExtensionInit(void);
#endif

#if 1
extern void PanoramiXExtensionInit(int argc, char *argv[]);
#endif

#if 1
extern void XkbExtensionInit(void);
#endif
