/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2011 NoMachine (http://www.nomachine.com)          */
/* Copyright (c) 2008-2017 Oleksandr Shneyder <o.shneyder@phoca-gmbh.de>  */
/* Copyright (c) 2011-2022 Mike Gabriel <mike.gabriel@das-netzwerkteam.de>*/
/* Copyright (c) 2014-2019 Mihai Moldovan <ionic@ionic.de>                */
/* Copyright (c) 2014-2022 Ulrich Sibiller <uli42@gmx.de>                 */
/* Copyright (c) 2015-2016 Qindel Group (http://www.qindel.com)           */
/*                                                                        */
/* NXAGENT, NX protocol compression and NX extensions to this software    */
/* are copyright of the aforementioned persons and companies.             */
/*                                                                        */
/* Redistribution and use of the present software is allowed according    */
/* to terms specified in the file LICENSE which comes in the source       */
/* distribution.                                                          */
/*                                                                        */
/* All rights reserved.                                                   */
/*                                                                        */
/* NOTE: This software has received contributions from various other      */
/* contributors, only the core maintainers and supporters are listed as   */
/* copyright holders. Please contact us, if you feel you should be listed */
/* as copyright holder, as well.                                          */
/*                                                                        */
/**************************************************************************/

/************************************************************

Copyright 1989, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.

********************************************************/

/* THIS IS NOT AN X CONSORTIUM STANDARD OR AN X PROJECT TEAM SPECIFICATION */


#include <nx-X11/X.h>
#include "Trap.h"
#include "Agent.h"

#include "Drawable.h"
#include "Pixmaps.h"

#include "../../Xext/shm.c"

/*
 * Set here the required log level.
 */

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

#include "Literals.h"

extern void fbGetImage(DrawablePtr pDrw, int x, int y, int w, int h,
                           unsigned int format, unsigned long planeMask, char *d);

extern void fbPutImage (DrawablePtr pDrawable, GCPtr pGC, int depth,
                            int x, int y, int w, int h, int leftPad, int format,
                                char *pImage);

extern int nxagentImageLength(int, int, int, int, int);

void
ShmExtensionInit(void)
{
    ExtensionEntry *extEntry;
    int i;

#ifdef MUST_CHECK_FOR_SHM_SYSCALL
    if (!CheckForShmSyscall())
    {
	ErrorF("MIT-SHM extension disabled due to lack of kernel support\n");
	return;
    }
#endif

#ifdef NXAGENT_SERVER
    if (!nxagentOption(SharedMemory))
    {
      return;
    }
#endif

    sharedPixmaps = xFalse;
    pixmapFormat = 0;
    {
#ifdef NXAGENT_SERVER
      sharedPixmaps = nxagentOption(SharedPixmaps);
#else
      sharedPixmaps = xTrue;
#endif
      pixmapFormat = shmPixFormat[0];
      for (i = 0; i < screenInfo.numScreens; i++)
      {
	if (!shmFuncs[i])
        {
            #ifdef TEST
            fprintf(stderr, "ShmExtensionInit: Registering shmFuncs as miFuncs.\n");
            #endif
	    shmFuncs[i] = &miFuncs;
        }
	if (!shmFuncs[i]->CreatePixmap)
	    sharedPixmaps = xFalse;
	if (shmPixFormat[i] && (shmPixFormat[i] != pixmapFormat))
	{
	    sharedPixmaps = xFalse;
	    pixmapFormat = 0;
	}
      }
      if (!pixmapFormat)
	pixmapFormat = ZPixmap;
      if (sharedPixmaps)
      {
	for (i = 0; i < screenInfo.numScreens; i++)
	{
	    destroyPixmap[i] = screenInfo.screens[i]->DestroyPixmap;
	    screenInfo.screens[i]->DestroyPixmap = ShmDestroyPixmap;
	}
#ifdef PIXPRIV
	shmPixmapPrivate = AllocatePixmapPrivateIndex();
	for (i = 0; i < screenInfo.numScreens; i++)
	{
	    if (!AllocatePixmapPrivate(screenInfo.screens[i],
				       shmPixmapPrivate, 0))
		return;
	}
#endif
      }
    }
    ShmSegType = CreateNewResourceType(ShmDetachSegment);
    if (ShmSegType &&
	(extEntry = AddExtension(SHMNAME, ShmNumberEvents, ShmNumberErrors,
				 ProcShmDispatch, SProcShmDispatch,
				 ShmResetProc, StandardMinorOpcode)))
    {
	ShmReqCode = (unsigned char)extEntry->base;
	ShmCompletionCode = extEntry->eventBase;
	BadShmSegCode = extEntry->errorBase;
	EventSwapVector[ShmCompletionCode] = (EventSwapPtr) SShmCompletionEvent;
    }
}

static void
miShmPutImage(dst, pGC, depth, format, w, h, sx, sy, sw, sh, dx, dy, data)
    DrawablePtr dst;
    GCPtr	pGC;
    int		depth, w, h, sx, sy, sw, sh, dx, dy;
    unsigned int format;
    char 	*data;
{
    /* Careful! This wrapper DEACTIVATES the trap! */

    nxagentShmTrap = False;

    xorg_miShmPutImage(dst, pGC, depth, format, w, h, sx, sy, sw, sh, dx, dy, data);

    nxagentShmTrap = True;

    return;
}


static void
fbShmPutImage(dst, pGC, depth, format, w, h, sx, sy, sw, sh, dx, dy, data)
    DrawablePtr dst;
    GCPtr	pGC;
    int		depth, w, h, sx, sy, sw, sh, dx, dy;
    unsigned int format;
    char 	*data;
{
#ifdef NXAGENT_SERVER
    #ifdef TEST
    fprintf(stderr, "fbShmPutImage: Called with drawable at [%p] GC at [%p] data at [%p].\n",
                (void *) dst, (void *) pGC, (void *) data);
    #endif
#endif

    if ((format == ZPixmap) || (depth == 1))
    {
	PixmapPtr pPixmap;

	pPixmap = GetScratchPixmapHeader(dst->pScreen, w, h, depth,
		BitsPerPixel(depth), PixmapBytePad(w, depth), (void *)data);
	if (!pPixmap)
	    return;
	if (format == XYBitmap)
	    (void)(*pGC->ops->CopyPlane)((DrawablePtr)pPixmap, dst, pGC,
					 sx, sy, sw, sh, dx, dy, 1L);
	else
	    (void)(*pGC->ops->CopyArea)((DrawablePtr)pPixmap, dst, pGC,
					sx, sy, sw, sh, dx, dy);

#ifdef NXAGENT_SERVER
        /*
         * We updated the internal framebuffer,
         * now we want to go on the real X.
         */

        #ifdef TEST
        fprintf(stderr, "fbShmPutImage: Realizing the PutImage with depth [%d] "
                    " format [%d] w [%d] h [%d] sx [%d] sy [%d] sw [%d] "
                        " sh [%d] dx [%d].\n", depth, format, w, h,
                            sx, sy, sw, sh, dx);
        #endif

        char *newdata = calloc(1, nxagentImageLength(sw, sh, format, 0, depth));

        if (newdata != NULL)
        {
          fbGetImage((DrawablePtr) pPixmap, sx, sy, sw, sh, format, AllPlanes, newdata);
          (*pGC->ops->PutImage)(dst, pGC, depth, dx, dy, sw, sh, 0, format, newdata);

          free(newdata);
        }
        else
        {
          #ifdef WARNING
          fprintf(stderr, "fbShmPutImage: WARNING! Data allocation failed.\n");
          #endif
        }

#endif /* NXAGENT_SERVER */
	FreeScratchPixmapHeader(pPixmap);
    }
    else
    {
        #ifdef TEST
        fprintf(stderr, "fbShmPutImage: Calling miShmPutImage().\n");
        #endif
	miShmPutImage(dst, pGC, depth, format, w, h, sx, sy, sw, sh, dx, dy,
		      data);
    }
}

static int
ProcShmPutImage(client)
    register ClientPtr client;
{
    register GCPtr pGC;
    register DrawablePtr pDraw;
    long length;
    ShmDescPtr shmdesc;
    REQUEST(xShmPutImageReq);

    REQUEST_SIZE_MATCH(xShmPutImageReq);
    VALIDATE_DRAWABLE_AND_GC(stuff->drawable, pDraw, pGC, client);
    VERIFY_SHMPTR(stuff->shmseg, stuff->offset, FALSE, shmdesc, client);
    if ((stuff->sendEvent != xTrue) && (stuff->sendEvent != xFalse))
	return BadValue;
    if (stuff->format == XYBitmap)
    {
        if (stuff->depth != 1)
            return BadMatch;
        length = PixmapBytePad(stuff->totalWidth, 1);
    }
    else if (stuff->format == XYPixmap)
    {
        if (pDraw->depth != stuff->depth)
            return BadMatch;
        length = PixmapBytePad(stuff->totalWidth, 1);
	length *= stuff->depth;
    }
    else if (stuff->format == ZPixmap)
    {
        if (pDraw->depth != stuff->depth)
            return BadMatch;
        length = PixmapBytePad(stuff->totalWidth, stuff->depth);
    }
    else
    {
	client->errorValue = stuff->format;
        return BadValue;
    }

    /* 
     * There's a potential integer overflow in this check:
     * VERIFY_SHMSIZE(shmdesc, stuff->offset, length * stuff->totalHeight,
     *                client);
     * the version below ought to avoid it
     */
    if (stuff->totalHeight != 0 && 
	length > (shmdesc->size - stuff->offset)/stuff->totalHeight) {
	client->errorValue = stuff->totalWidth;
	return BadValue;
    }
    if (stuff->srcX > stuff->totalWidth)
    {
	client->errorValue = stuff->srcX;
	return BadValue;
    }
    if (stuff->srcY > stuff->totalHeight)
    {
	client->errorValue = stuff->srcY;
	return BadValue;
    }
    if ((stuff->srcX + stuff->srcWidth) > stuff->totalWidth)
    {
	client->errorValue = stuff->srcWidth;
	return BadValue;
    }
    if ((stuff->srcY + stuff->srcHeight) > stuff->totalHeight)
    {
	client->errorValue = stuff->srcHeight;
	return BadValue;
    }

    #ifdef TEST
    fprintf(stderr, "ProcShmPutImage: Format [%d] srcX [%d] srcY [%d], "
                "totalWidth [%d] totalHeight [%d]\n", stuff->format, stuff->srcX,
                    stuff->srcY, stuff->totalWidth, stuff->totalHeight);
    #endif

#ifndef NXAGENT_SERVER
    /*
    It seems like this code was removed for a good reason. Including
    it leads to very strange issues when coupled with libXcomp and using
    connection speed settings lower than LAN (and even on LAN some icons
    are not showing up correctly, e.g., when using MATE).

    Further investigation on why this happens pending and might happen at a
    later time.

    See also ArcticaProject/nx-libs#656
    */
    if ((((stuff->format == ZPixmap) && (stuff->srcX == 0)) ||
         ((stuff->format != ZPixmap) &&
          (stuff->srcX < screenInfo.bitmapScanlinePad) &&
          ((stuff->format == XYBitmap) ||
	   ((stuff->srcY == 0) &&
	    (stuff->srcHeight == stuff->totalHeight))))) &&
        ((stuff->srcX + stuff->srcWidth) == stuff->totalWidth))
        (*pGC->ops->PutImage) (pDraw, pGC, stuff->depth,
                               stuff->dstX, stuff->dstY,
                               stuff->totalWidth, stuff->srcHeight,
                               stuff->srcX, stuff->format,
                               shmdesc->addr + stuff->offset +
                               (stuff->srcY * length));
    else
#endif
    {
        #ifdef TEST
        fprintf(stderr, "ProcShmPutImage: Calling (*shmFuncs[pDraw->pScreen->myNum]->PutImage)().\n");
        #endif

        (*shmFuncs[pDraw->pScreen->myNum]->PutImage)(
                                   pDraw, pGC, stuff->depth, stuff->format,
                                   stuff->totalWidth, stuff->totalHeight,
                                   stuff->srcX, stuff->srcY,
                                   stuff->srcWidth, stuff->srcHeight,
                                   stuff->dstX, stuff->dstY,
                                   shmdesc->addr + stuff->offset);
    }

    if (stuff->sendEvent)
    {
	xShmCompletionEvent ev;

	memset(&ev, 0, sizeof(xShmCompletionEvent));
	ev.type = ShmCompletionCode;
	ev.drawable = stuff->drawable;
	ev.minorEvent = X_ShmPutImage;
	ev.majorEvent = ShmReqCode;
	ev.shmseg = stuff->shmseg;
	ev.offset = stuff->offset;
	WriteEventsToClient(client, 1, (xEvent *) &ev);
    }

    return (client->noClientException);
}

/* derived from Xext/shm.c */
static PixmapPtr
nxagent_fbShmCreatePixmap (pScreen, width, height, depth, addr)
    ScreenPtr	pScreen;
    int		width;
    int		height;
    int		depth;
    char	*addr;
{
    register PixmapPtr pPixmap;

#ifdef NXAGENT_SERVER
    pPixmap = (*pScreen->CreatePixmap)(pScreen, width, height, depth, 0);
#else
    pPixmap = (*pScreen->CreatePixmap)(pScreen, 0, 0, pScreen->rootDepth, 0);
#endif
    if (!pPixmap)
    {
        return NullPixmap;
    }

    if (!(*pScreen->ModifyPixmapHeader)(pPixmap, width, height, depth,
	    BitsPerPixel(depth), PixmapBytePad(width, depth), (void *)addr)) 
    {
        (*pScreen->DestroyPixmap)(pPixmap);
        return NullPixmap;
    }
    return pPixmap;
}

static PixmapPtr
fbShmCreatePixmap (pScreen, width, height, depth, addr)
    ScreenPtr	pScreen;
    int		width;
    int		height;
    int		depth;
    char	*addr;
{
    #ifdef TEST
    fprintf(stderr, "%s: Width [%d] Height [%d] Depth [%d] Hint[%d]\n", __func__,
	    width, height, depth, 0);
    #endif

    nxagentShmPixmapTrap = True;

    PixmapPtr result = nxagent_fbShmCreatePixmap(pScreen, width, height, depth, addr);

    nxagentShmPixmapTrap = False;

    #ifdef WARNING
    if (result == NullPixmap)
    {
      fprintf(stderr, "%s: Return Null Pixmap.\n", __func__);
    }
    #endif

    return result;
}

static int
ProcShmDispatch (register ClientPtr client)
{
    #ifdef TEST
    REQUEST(xReq);
    if (stuff->data <= X_ShmCreatePixmap)
    {
      fprintf(stderr, "%s: Request [%s] OPCODE [%d] for client [%d].\n", __func__,
                      nxagentShmRequestLiteral[stuff->data], stuff->data, client->index);
    }
    #endif

    nxagentShmTrap = True;

    int result = xorg_ProcShmDispatch(client);

    nxagentShmTrap = False;

    return result;
}

static int
SProcShmDispatch (register ClientPtr client)
{
    #ifdef TEST
    REQUEST(xReq);
    if (stuff->data <= X_ShmCreatePixmap)
    {
      fprintf(stderr, "%s: Request [%s] OPCODE [%d] for client [%d].\n", __func__,
                      nxagentShmRequestLiteral[stuff->data], stuff->data, client->index);
    }
    #endif

    nxagentShmTrap = True;

    int result = xorg_SProcShmDispatch(client);

    nxagentShmTrap = False;

    return result;
}
