/*
 * Declarations for the BIGFONT extension.
 *
 * Copyright (c) 1999-2000  Bruno Haible
 * Copyright (c) 1999-2000  The XFree86 Project, Inc.
 */

/* THIS IS NOT AN X CONSORTIUM STANDARD */

#ifndef _XF86BIGFONT_H_
#define _XF86BIGFONT_H_

#include <nx-X11/Xfuncproto.h>

#define X_XF86BigfontQueryVersion	0
#define X_XF86BigfontQueryFont		1

#define XF86BigfontNumberEvents		0

#define XF86BigfontNumberErrors		0

#ifdef _XF86BIGFONT_SERVER_

_XFUNCPROTOBEGIN

#include <X11/fonts/font.h>

extern void XFree86BigfontExtensionInit(void);
extern void XF86BigfontFreeFontShm(FontPtr);
extern void XF86BigfontCleanup(void);

_XFUNCPROTOEND

#endif /* _XF86BIGFONT_SERVER_ */

#endif /* _XF86BIGFONT_H_ */
