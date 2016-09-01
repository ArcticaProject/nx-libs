/*

Copyright 1993, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from The Open Group.

*/

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/X.h>
#include "scrnintstr.h"
#include "misc.h"
#include "os.h"
#include "windowstr.h"
#include "resource.h"
#include "dixstruct.h"
#include "gcstruct.h"
#include "servermd.h"
#include "site.h"


/*
 *  Scratch pixmap management and device independent pixmap allocation
 *  function.
 */


/* callable by ddx */
PixmapPtr
GetScratchPixmapHeader(ScreenPtr pScreen, int width, int height, int depth, 
                       int bitsPerPixel, int devKind, void * pPixData)
{
    PixmapPtr pPixmap = pScreen->pScratchPixmap;

    if (pPixmap)
	pScreen->pScratchPixmap = NULL;
    else
	/* width and height of 0 means don't allocate any pixmap data */
	pPixmap = (*pScreen->CreatePixmap)(pScreen, 0, 0, depth);

    if (pPixmap) {
	if ((*pScreen->ModifyPixmapHeader)(pPixmap, width, height, depth,
					   bitsPerPixel, devKind, pPixData))
	    return pPixmap;
	(*pScreen->DestroyPixmap)(pPixmap);
    }
    return NullPixmap;
}


/* callable by ddx */
void
FreeScratchPixmapHeader(PixmapPtr pPixmap)
{
    if (pPixmap)
    {
	ScreenPtr pScreen = pPixmap->drawable.pScreen;

	pPixmap->devPrivate.ptr = NULL; /* lest ddx chases bad ptr */
	if (pScreen->pScratchPixmap)
	    (*pScreen->DestroyPixmap)(pPixmap);
	else
	    pScreen->pScratchPixmap = pPixmap;
    }
}


Bool
CreateScratchPixmapsForScreen(int scrnum)
{
    /* let it be created on first use */
    screenInfo.screens[scrnum]->pScratchPixmap = NULL;
    return TRUE;
}


void
FreeScratchPixmapsForScreen(int scrnum)
{
    FreeScratchPixmapHeader(screenInfo.screens[scrnum]->pScratchPixmap);
}


/* callable by ddx */
PixmapPtr
AllocatePixmap(ScreenPtr pScreen, int pixDataSize)
{
    PixmapPtr pPixmap;
#ifdef PIXPRIV
    char *ptr;
    DevUnion *ppriv;
    unsigned *sizes;
    unsigned size;
    int i;

    if (pScreen->totalPixmapSize > ((size_t)-1) - pixDataSize)
	return NullPixmap;
    
    /*
     * FIXME: Allocate 4 bytes at the end of each pixmap. This
     * is a quick workaround intended to fix a problem reported
     * by Valgrind due to fbBlt() writing just after the end of
     * the pixmap buffer. This may be a RENDER bug.
     */

    pPixmap = (PixmapPtr)malloc(pScreen->totalPixmapSize + pixDataSize + 4);
    if (!pPixmap)
	return NullPixmap;
    ppriv = (DevUnion *)(pPixmap + 1);
    pPixmap->devPrivates = ppriv;
    sizes = pScreen->PixmapPrivateSizes;
    ptr = (char *)(ppriv + pScreen->PixmapPrivateLen);
    for (i = pScreen->PixmapPrivateLen; --i >= 0; ppriv++, sizes++)
    {
        if ((size = *sizes) != 0)
        {
	    ppriv->ptr = (void *)ptr;
	    ptr += size;
        }
        else
	    ppriv->ptr = (void *)NULL;
    }
#else
    pPixmap = (PixmapPtr)malloc(sizeof(PixmapRec) + pixDataSize);
#endif
    return pPixmap;
}
