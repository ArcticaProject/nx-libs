/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2011 NoMachine (http://www.nomachine.com)          */
/* Copyright (c) 2008-2014 Oleksandr Shneyder <o.shneyder@phoca-gmbh.de>  */
/* Copyright (c) 2011-2016 Mike Gabriel <mike.gabriel@das-netzwerkteam.de>*/
/* Copyright (c) 2014-2016 Mihai Moldovan <ionic@ionic.de>                */
/* Copyright (c) 2014-2016 Ulrich Sibiller <uli42@gmx.de>                 */
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

#ifdef TEST
#include "Literals.h"
#endif

extern void fbGetImage(DrawablePtr pDrw, int x, int y, int w, int h,
                           unsigned int format, unsigned long planeMask, char *d);

extern void fbPutImage (DrawablePtr pDrawable, GCPtr pGC, int depth,
                            int x, int y, int w, int h, int leftPad, int format,
                                char *pImage);

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
    if (nxagentOption(SharedMemory) == False)
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
nxagent_miShmPutImage(dst, pGC, depth, format, w, h, sx, sy, sw, sh, dx, dy, data)
    DrawablePtr dst;
    GCPtr	pGC;
    int		depth, w, h, sx, sy, sw, sh, dx, dy;
    unsigned int format;
    char 	*data;
{
    PixmapPtr pmap;
    GCPtr putGC;

    putGC = GetScratchGC(depth, dst->pScreen);
    if (!putGC)
    {
	return;
    }
    pmap = (*dst->pScreen->CreatePixmap)(dst->pScreen, sw, sh, depth,
                                        CREATE_PIXMAP_USAGE_SCRATCH);
    if (!pmap)
    {
	FreeScratchGC(putGC);
	return;
    }
    ValidateGC((DrawablePtr)pmap, putGC);
    (*putGC->ops->PutImage)((DrawablePtr)pmap, putGC, depth, -sx, -sy, w, h, 0,
			    (format == XYPixmap) ? XYPixmap : ZPixmap, data);
    FreeScratchGC(putGC);
    if (format == XYBitmap)
	(void)(*pGC->ops->CopyPlane)((DrawablePtr)pmap, dst, pGC, 0, 0, sw, sh,
				     dx, dy, 1L);
    else
	(void)(*pGC->ops->CopyArea)((DrawablePtr)pmap, dst, pGC, 0, 0, sw, sh,
				    dx, dy);
    (*pmap->drawable.pScreen->DestroyPixmap)(pmap);
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

    nxagentShmTrap = 0;

    nxagent_miShmPutImage(dst, pGC, depth, format, w, h, sx, sy, sw, sh, dx, dy, data);

    nxagentShmTrap = 1;

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
    int length;
    char *newdata;
    extern int nxagentImageLength(int, int, int, int, int);

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

        length = nxagentImageLength(sw, sh, format, 0, depth);

        if ((newdata = calloc(1, length)) != NULL)
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


static PixmapPtr
nxagent_fbShmCreatePixmap (pScreen, width, height, depth, addr)
    ScreenPtr	pScreen;
    int		width;
    int		height;
    int		depth;
    char	*addr;
{
    register PixmapPtr pPixmap;

    pPixmap = (*pScreen->CreatePixmap)(pScreen, width, height, depth, 0);

    if (!pPixmap)
    {
      return NullPixmap;
    }

    #if defined(NXAGENT_SERVER) && defined(TEST)
    fprintf(stderr,"fbShmCreatePixmap: Width [%d] Height [%d] Depth [%d] Hint[%d]\n", width, height, depth, 0);
    #endif

    if (!(*pScreen->ModifyPixmapHeader)(pPixmap, width, height, depth,
	    BitsPerPixel(depth), PixmapBytePad(width, depth), (void *)addr)) 
    {
      #if defined(NXAGENT_SERVER) && defined(WARNING)
      fprintf(stderr,"fbShmCreatePixmap: Return Null Pixmap.\n");
      #endif

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
    PixmapPtr result;

    nxagentShmPixmapTrap = 1;

    result = nxagent_fbShmCreatePixmap(pScreen, width, height, depth, addr);

    nxagentShmPixmapTrap = 0;

    return result;
}


static int
nxagent_ProcShmDispatch (client)
    register ClientPtr	client;
{
    REQUEST(xReq);
    switch (stuff->data)
    {
    case X_ShmQueryVersion:
	return ProcShmQueryVersion(client);
    case X_ShmAttach:
	return ProcShmAttach(client);
    case X_ShmDetach:
	return ProcShmDetach(client);
    case X_ShmPutImage:
      {
#ifdef PANORAMIX
        if ( !noPanoramiXExtension )
           return ProcPanoramiXShmPutImage(client);
#endif
        return ProcShmPutImage(client);
      }
    case X_ShmGetImage:
#ifdef PANORAMIX
        if ( !noPanoramiXExtension )
	   return ProcPanoramiXShmGetImage(client);
#endif
	return ProcShmGetImage(client);
    case X_ShmCreatePixmap:
#ifdef PANORAMIX
        if ( !noPanoramiXExtension )
	   return ProcPanoramiXShmCreatePixmap(client);
#endif
	   return ProcShmCreatePixmap(client);
    default:
	return BadRequest;
    }
}

static int
nxagent_SProcShmDispatch (client)
    register ClientPtr	client;
{
    REQUEST(xReq);

    switch (stuff->data)
    {
    case X_ShmQueryVersion:
	return SProcShmQueryVersion(client);
    case X_ShmAttach:
	return SProcShmAttach(client);
    case X_ShmDetach:
	return SProcShmDetach(client);
    case X_ShmPutImage:
        return SProcShmPutImage(client);
    case X_ShmGetImage:
	return SProcShmGetImage(client);
    case X_ShmCreatePixmap:
	return SProcShmCreatePixmap(client);
    default:
	return BadRequest;
    }
}

/* A wrapper that handles the trap. This construct is used
   to keep the derived code closer to the original
*/
static int
ProcShmDispatch (register ClientPtr client)
{
    int result;

    #ifdef TEST
    REQUEST(xReq);
    fprintf(stderr, "ProcShmDispatch: Going to execute operation [%d] for client [%d].\n",
                stuff -> data, client -> index);

    if (stuff->data <= X_ShmCreatePixmap)
    {
      fprintf(stderr, "ProcShmDispatch: Request [%s] OPCODE#%d.\n",
                  nxagentShmRequestLiteral[stuff->data], stuff->data);
    }

    if (stiff->data == X_ShmPutImage)
    {
      fprintf(stderr, "ProcShmDispatch: Going to execute ProcShmPutImage() for client [%d].\n",
                      client -> index);
    }
    #endif

    nxagentShmTrap = 1;

    result = nxagent_ProcShmDispatch(client);

    nxagentShmTrap = 0;

    return result;
}

static int
SProcShmDispatch (register ClientPtr client)
{
    int result;

    #ifdef TEST
    REQUEST(xReq);
    fprintf(stderr, "SProcShmDispatch: Going to execute operation [%d] for client [%d].\n", 
                stuff -> data, client -> index);

    if (stuff->data <= X_ShmCreatePixmap)
    {
      fprintf(stderr, "ProcShmDispatch: Request [%s] OPCODE#%d.\n",
                  nxagentShmRequestLiteral[stuff->data], stuff->data);
    }

    if (stuff->data == X_ProxShmPutImage)
    {
      fprintf(stderr, "SProcShmDispatch: Going to execute SProcShmPutImage() for client [%d].\n",
                      client -> index);
    }
    #endif

    nxagentShmTrap = 1;

    result = nxagent_SProcShmDispatch(client);

    nxagentShmTrap = 0;

    return result;
}
