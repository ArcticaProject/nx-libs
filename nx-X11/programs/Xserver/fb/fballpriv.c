/*
 * Copyright © 1998 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "fb.h"

#ifdef FB_SCREEN_PRIVATE
int fbScreenPrivateIndex;
int fbGetScreenPrivateIndex(void)
{
    return fbScreenPrivateIndex;
}
#endif
int fbGCPrivateIndex;
int fbGetGCPrivateIndex(void)
{
    return fbGCPrivateIndex;
}
#ifndef FB_NO_WINDOW_PIXMAPS
int fbWinPrivateIndex;
int fbGetWinPrivateIndex(void)
{
    return fbWinPrivateIndex;
}
#endif
int fbGeneration;

Bool
fbAllocatePrivates(ScreenPtr pScreen, int *pGCIndex)
{
    if (fbGeneration != serverGeneration)
    {
	fbGCPrivateIndex = miAllocateGCPrivateIndex ();
#ifndef FB_NO_WINDOW_PIXMAPS
	fbWinPrivateIndex = AllocateWindowPrivateIndex();
#endif
#ifdef FB_SCREEN_PRIVATE
	fbScreenPrivateIndex = AllocateScreenPrivateIndex ();
	if (fbScreenPrivateIndex == -1)
	    return FALSE;
#endif
	
	fbGeneration = serverGeneration;
    }
    if (pGCIndex)
	*pGCIndex = fbGCPrivateIndex;
    if (!AllocateGCPrivate(pScreen, fbGCPrivateIndex, sizeof(FbGCPrivRec)))
	return FALSE;
#ifndef FB_NO_WINDOW_PIXMAPS
    if (!AllocateWindowPrivate(pScreen, fbWinPrivateIndex, 0))
	return FALSE;
#endif
#ifdef FB_SCREEN_PRIVATE
    {
	FbScreenPrivPtr	pScreenPriv;

	pScreenPriv = (FbScreenPrivPtr) malloc (sizeof (FbScreenPrivRec));
	if (!pScreenPriv)
	    return FALSE;
	pScreen->devPrivates[fbScreenPrivateIndex].ptr = (void *) pScreenPriv;
    }
#endif
    return TRUE;
}

#ifdef FB_ACCESS_WRAPPER
ReadMemoryProcPtr wfbReadMemory;
WriteMemoryProcPtr wfbWriteMemory;
#endif
