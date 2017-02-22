/* $XFree86: xc/lib/GL/mesa/src/drv/radeon/radeon_screen.h,v 1.5 2002/12/16 16:18:58 dawes Exp $ */
/**************************************************************************

Copyright 2000, 2001 ATI Technologies Inc., Ontario, Canada, and
                     VA Linux Systems Inc., Fremont, California.

All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Kevin E. Martin <martin@valinux.com>
 *   Gareth Hughes <gareth@valinux.com>
 */

#ifndef __RADEON_SCREEN_H__
#define __RADEON_SCREEN_H__

/*
 * IMPORTS: these headers contain all the DRI, X and kernel-related
 * definitions that we need.
 */
/* #include "dri_util.h" */
#include "radeon_dri.h"
#include "radeon_reg.h"
#include "drm_sarea.h"
#include "xmlconfig.h"


typedef struct {
   drm_handle_t handle;			/* Handle to the DRM region */
   drmSize size;			/* Size of the DRM region */
   drmAddress map;			/* Mapping of the DRM region */
} radeonRegionRec, *radeonRegionPtr;

/* chipset features */
#define RADEON_CHIPSET_TCL	(1 << 0)
#define RADEON_CHIPSET_BROKEN_STENCIL (1 << 1)

typedef struct {

   int chipset;
   int cpp;
   int IsPCI;				/* Current card is a PCI card */
   int AGPMode;
   unsigned int irq;			/* IRQ number (0 means none) */

   unsigned int fbLocation;
   unsigned int frontOffset;
   unsigned int frontPitch;
   unsigned int backOffset;
   unsigned int backPitch;

   unsigned int depthOffset;
   unsigned int depthPitch;

    /* Shared texture data */
   int numTexHeaps;
   int texOffset[RADEON_NR_TEX_HEAPS];
   int texSize[RADEON_NR_TEX_HEAPS];
   int logTexGranularity[RADEON_NR_TEX_HEAPS];

   radeonRegionRec mmio;
   radeonRegionRec status;
   radeonRegionRec gartTextures;

   drmBufMapPtr buffers;

   __volatile__ u_int32_t *scratch;

   __DRIscreenPrivate *driScreen;
   unsigned int sarea_priv_offset;
   unsigned int gart_buffer_offset;	/* offset in card memory space */
   unsigned int gart_texture_offset;	/* offset in card memory space */

   GLboolean depthHasSurface;

   /* Configuration cache with default values for all contexts */
   driOptionCache optionCache;
} radeonScreenRec, *radeonScreenPtr;

#endif /* __RADEON_SCREEN_H__ */
