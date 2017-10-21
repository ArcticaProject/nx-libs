/*
 * Copyright © 2009 Red Hat, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

/**
 * This file specifies the server-supported protocol versions.
 */
#ifndef PROTOCOL_VERSIONS_H
#define PROTOCOL_VERSIONS_H

#ifdef NXAGENT_SERVER
# define XTRANS_SEND_FDS 0
#endif

/* Composite */
#define SERVER_COMPOSITE_MAJOR_VERSION		0
#define SERVER_COMPOSITE_MINOR_VERSION		4

/* Damage */
#define SERVER_DAMAGE_MAJOR_VERSION		1
#ifndef NXAGENT_SERVER
#define SERVER_DAMAGE_MINOR_VERSION		1
#else /* !defined(NXAGENT_SERVER) */
#define SERVER_DAMAGE_MINOR_VERSION		0
#endif /* !defined(NXAGENT_SERVER) */

#ifndef NXAGENT_SERVER
/* DRI3 */
#define SERVER_DRI3_MAJOR_VERSION               1
#define SERVER_DRI3_MINOR_VERSION               0
#endif /* !defined(NXAGENT_SERVER) */

#ifndef NXAGENT_SERVER
/* DMX */
#define SERVER_DMX_MAJOR_VERSION                2
#define SERVER_DMX_MINOR_VERSION                2
#define SERVER_DMX_PATCH_VERSION                20040604
#endif /* !defined(NXAGENT_SERVER) */

#ifndef NXAGENT_SERVER
/* Generic event extension */
#define SERVER_GE_MAJOR_VERSION                 1
#define SERVER_GE_MINOR_VERSION                 0
#endif /* !defined(NXAGENT_SERVER) */

/* GLX */
#define SERVER_GLX_MAJOR_VERSION		1
#ifndef NXAGENT_SERVER
#define SERVER_GLX_MINOR_VERSION		4
#else
#define SERVER_GLX_MINOR_VERSION		2
#endif

/* Xinerama */
#define SERVER_PANORAMIX_MAJOR_VERSION          1
#define SERVER_PANORAMIX_MINOR_VERSION		1

/* Present */
#define SERVER_PRESENT_MAJOR_VERSION            1
#define SERVER_PRESENT_MINOR_VERSION            0

/* RandR */
#define SERVER_RANDR_MAJOR_VERSION		1
#define SERVER_RANDR_MINOR_VERSION		5

/* Record */
#define SERVER_RECORD_MAJOR_VERSION		1
#define SERVER_RECORD_MINOR_VERSION		13

/* Render */
#define SERVER_RENDER_MAJOR_VERSION		0
#ifndef NXAGENT_SERVER
#define SERVER_RENDER_MINOR_VERSION		11
#else /* !defined(NXAGENT_SERVER) */
#define SERVER_RENDER_MINOR_VERSION		10
#endif /* !defined(NXAGENT_SERVER) */

/* RandR Xinerama */
#define SERVER_RRXINERAMA_MAJOR_VERSION		1
#define SERVER_RRXINERAMA_MINOR_VERSION		1

/* Screensaver */
#define SERVER_SAVER_MAJOR_VERSION		1
#define SERVER_SAVER_MINOR_VERSION		1

/* Security */
#define SERVER_SECURITY_MAJOR_VERSION		1
#define SERVER_SECURITY_MINOR_VERSION		0

/* Shape */
#define SERVER_SHAPE_MAJOR_VERSION		1
#define SERVER_SHAPE_MINOR_VERSION		1

/* SHM */
#define SERVER_SHM_MAJOR_VERSION		1
#ifndef NXAGENT_SERVER
#if XTRANS_SEND_FDS
#define SERVER_SHM_MINOR_VERSION		2
#else
#define SERVER_SHM_MINOR_VERSION		1
#endif
#else /* !defined(NXAGENT_SERVER) */
#define SERVER_SHM_MINOR_VERSION		1
#endif /* !defined(NXAGENT_SERVER) */

/* Sync */
#define SERVER_SYNC_MAJOR_VERSION		3
#ifndef NXAGENT_SERVER
#define SERVER_SYNC_MINOR_VERSION		1
#else /* !defined(NXAGENT_SERVER) */
#define SERVER_SYNC_MINOR_VERSION		0
#endif /* !defined(NXAGENT_SERVER) */

/* Big Font */
#define SERVER_XF86BIGFONT_MAJOR_VERSION	1
#define SERVER_XF86BIGFONT_MINOR_VERSION	1

#ifndef NXAGENT_SERVER
/* Vidmode */
#define SERVER_XF86VIDMODE_MAJOR_VERSION	2
#define SERVER_XF86VIDMODE_MINOR_VERSION	2
#endif /* !defined(NXAGENT_SERVER) */

/* Fixes */
#ifndef NXAGENT_SERVER
#define SERVER_XFIXES_MAJOR_VERSION		5
#define SERVER_XFIXES_MINOR_VERSION		0
#else /* !defined(NXAGENT_SERVER) */
#define SERVER_XFIXES_MAJOR_VERSION		3
#define SERVER_XFIXES_MINOR_VERSION		0
#endif /* !defined(NXAGENT_SERVER) */

/* X Input */
#ifndef NXAGENT_SERVER
#define SERVER_XI_MAJOR_VERSION			2
#define SERVER_XI_MINOR_VERSION			3
#else /* !defined(NXAGENT_SERVER) */
#define SERVER_XI_MAJOR_VERSION			1
#define SERVER_XI_MINOR_VERSION			3
#endif /* !defined(NXAGENT_SERVER) */

/* XKB */
#define SERVER_XKB_MAJOR_VERSION		1
#define SERVER_XKB_MINOR_VERSION		0

/* Resource */
#define SERVER_XRES_MAJOR_VERSION		1
#ifndef NXAGENT_SERVER
#define SERVER_XRES_MINOR_VERSION		2
#else /* !defined(NXAGENT_SERVER) */
#define SERVER_XRES_MINOR_VERSION		0
#endif /* !defined(NXAGENT_SERVER) */

#ifndef NXAGENT_SERVER
/* XvMC */
#define SERVER_XVMC_MAJOR_VERSION		1
#define SERVER_XVMC_MINOR_VERSION		1
#endif /* !defined(NXAGENT_SERVER) */

#endif /* PROTOCOL_VERSIONS_H */
