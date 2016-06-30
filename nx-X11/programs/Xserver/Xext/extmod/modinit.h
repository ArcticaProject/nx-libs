/* $XdotOrg: xc/programs/Xserver/Xext/extmod/modinit.h,v 1.5 2005/07/03 07:01:06 daniels Exp $ */
/* $XFree86: xc/programs/Xserver/Xext/extmod/modinit.h,v 1.1 2003/07/16 01:38:33 dawes Exp $ */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifndef INITARGS
#define INITARGS void
#endif

#ifdef SHAPE
extern void ShapeExtensionInit(INITARGS);
#include <X11/extensions/shapeproto.h>
#endif

#ifdef XTEST
extern void XTestExtensionInit(INITARGS);
#define _XTEST_SERVER_
#include <nx-X11/extensions/xtestconst.h>
#include <nx-X11/extensions/xteststr.h>
#endif

#if 1
extern void XTestExtension1Init(INITARGS);
#endif

#ifdef BIGREQS
extern void BigReqExtensionInit(INITARGS);
#include <nx-X11/extensions/bigreqstr.h>
#endif

#ifdef XSYNC
extern void SyncExtensionInit(INITARGS);
#define _SYNC_SERVER
#include <nx-X11/extensions/sync.h>
#include <nx-X11/extensions/syncstr.h>
#endif

#ifdef SCREENSAVER
extern void ScreenSaverExtensionInit (INITARGS);
#include <nx-X11/extensions/saver.h>
#endif

#ifdef XCMISC
extern void XCMiscExtensionInit(INITARGS);
#include <nx-X11/extensions/xcmiscstr.h>
#endif

#ifdef DPMSExtension
extern void DPMSExtensionInit(INITARGS);
#include <nx-X11/extensions/dpmsstr.h>
#endif

#ifdef XV
extern void XvExtensionInit(INITARGS);
extern void XvMCExtensionInit(INITARGS);
extern void XvRegister(INITARGS);
#include <nx-X11/extensions/Xv.h>
#include <nx-X11/extensions/XvMC.h>
#endif

#ifdef RES
extern void ResExtensionInit(INITARGS);
#include <nx-X11/extensions/XResproto.h>
#endif

#ifdef SHM
extern void ShmExtensionInit(INITARGS);
#include <nx-X11/extensions/shmstr.h>
extern void ShmSetPixmapFormat(
    ScreenPtr pScreen,
    int format);
extern void ShmRegisterFuncs(
    ScreenPtr pScreen,
    ShmFuncsPtr funcs);
#endif

#if 1
extern void SecurityExtensionInit(INITARGS);
#endif

#if 1
extern void XagExtensionInit(INITARGS);
#endif

#if 1
extern void XpExtensionInit(INITARGS);
#endif

#if 1
extern void PanoramiXExtensionInit(int argc, char *argv[]);
#endif

#if 1
extern void XkbExtensionInit(INITARGS);
#endif
