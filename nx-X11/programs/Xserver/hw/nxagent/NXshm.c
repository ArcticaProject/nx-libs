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

/* $XFree86: xc/programs/Xserver/Xext/shm.c,v 3.41 2003/12/17 23:28:56 alanh Exp $ */
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

/* $Xorg: shm.c,v 1.4 2001/02/09 02:04:33 xorgcvs Exp $ */

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

    if (nxagentOption(SharedMemory) == False)
    {
      return;
    }

    sharedPixmaps = xFalse;
    pixmapFormat = 0;
    {
      sharedPixmaps = nxagentOption(SharedPixmaps);
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
    PixmapPtr pmap;
    GCPtr putGC;

    nxagentShmTrap = 0;
    putGC = GetScratchGC(depth, dst->pScreen);
    if (!putGC)
    {
        nxagentShmTrap = 1;
	return;
    }
    pmap = (*dst->pScreen->CreatePixmap)(dst->pScreen, sw, sh, depth);
    if (!pmap)
    {
        nxagentShmTrap = 1;
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
    nxagentShmTrap = 1;
}

static void
fbShmPutImage(dst, pGC, depth, format, w, h, sx, sy, sw, sh, dx, dy, data)
    DrawablePtr dst;
    GCPtr	pGC;
    int		depth, w, h, sx, sy, sw, sh, dx, dy;
    unsigned int format;
    char 	*data;
{
    int length;
    char *newdata;
    extern int nxagentImageLength(int, int, int, int, int);

    #ifdef TEST
    fprintf(stderr, "fbShmPutImage: Called with drawable at [%p] GC at [%p] data at [%p].\n",
                (void *) dst, (void *) pGC, (void *) data);
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

        if ((newdata = malloc(length)) != NULL)
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

    if (stuff->sendEvent)
    {
	xShmCompletionEvent ev;

	ev.type = ShmCompletionCode;
	ev.drawable = stuff->drawable;
	ev.sequenceNumber = client->sequence;
	ev.minorEvent = X_ShmPutImage;
	ev.majorEvent = ShmReqCode;
	ev.shmseg = stuff->shmseg;
	ev.offset = stuff->offset;
	WriteEventsToClient(client, 1, (xEvent *) &ev);
    }

    return (client->noClientException);
}


static PixmapPtr
fbShmCreatePixmap (pScreen, width, height, depth, addr)
    ScreenPtr	pScreen;
    int		width;
    int		height;
    int		depth;
    char	*addr;
{
    register PixmapPtr pPixmap;

    nxagentShmPixmapTrap = 1;

    pPixmap = (*pScreen->CreatePixmap)(pScreen, width, height, depth);

    if (!pPixmap)
    {
      nxagentShmPixmapTrap = 0;

      return NullPixmap;
    }

    #ifdef TEST
    fprintf(stderr,"fbShmCreatePixmap: Width [%d] Height [%d] Depth [%d]\n", width, height, depth);
    #endif

    if (!(*pScreen->ModifyPixmapHeader)(pPixmap, width, height, depth,
	    BitsPerPixel(depth), PixmapBytePad(width, depth), (void *)addr)) 
    {
      #ifdef WARNING
      fprintf(stderr,"fbShmCreatePixmap: Return Null Pixmap.\n");
      #endif

      (*pScreen->DestroyPixmap)(pPixmap);

      nxagentShmPixmapTrap = 0;

      return NullPixmap;
    }

    nxagentShmPixmapTrap = 0;

    return pPixmap;
}


static int
ProcShmDispatch (client)
    register ClientPtr	client;
{
    REQUEST(xReq);

    #ifdef TEST
    fprintf(stderr, "ProcShmDispatch: Going to execute operation [%d] for client [%d].\n", 
                stuff -> data, client -> index);

    if (stuff->data <= X_ShmCreatePixmap)
    {
      fprintf(stderr, "ProcShmDispatch: Request [%s] OPCODE#%d.\n",
                  nxagentShmRequestLiteral[stuff->data], stuff->data);
    }
    #endif

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
        int result;

        #ifdef TEST
        fprintf(stderr, "ProcShmDispatch: Going to execute ProcShmPutImage() for client [%d].\n", 
                    client -> index);
        #endif

        nxagentShmTrap = 1;

#ifdef PANORAMIX
        if ( !noPanoramiXExtension )
        {
           result = ProcPanoramiXShmPutImage(client);

           nxagentShmTrap = 0;

           return result;
        }
#endif

        result = ProcShmPutImage(client);

        nxagentShmTrap = 0;

        #ifdef TEST
        fprintf(stderr, "ProcShmDispatch: Returning from ProcShmPutImage() for client [%d].\n", 
                    client -> index);
        #endif

        return result;
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
SProcShmDispatch (client)
    register ClientPtr	client;
{
    REQUEST(xReq);

    #ifdef TEST
    fprintf(stderr, "SProcShmDispatch: Going to execute operation [%d] for client [%d].\n", 
                stuff -> data, client -> index);
    #endif

    switch (stuff->data)
    {
    case X_ShmQueryVersion:
	return SProcShmQueryVersion(client);
    case X_ShmAttach:
	return SProcShmAttach(client);
    case X_ShmDetach:
	return SProcShmDetach(client);
    case X_ShmPutImage:
      {
        int result;

        #ifdef TEST
        fprintf(stderr, "SProcShmDispatch: Going to execute SProcShmPutImage() for client [%d].\n", 
                    client -> index);
        #endif

        nxagentShmTrap = 1;

        result = SProcShmPutImage(client);

        nxagentShmTrap = 0;

        #ifdef TEST
        fprintf(stderr, "SProcShmDispatch: Returning from SProcShmPutImage() for client [%d].\n", 
                    client -> index);
        #endif

        return result;
      }
    case X_ShmGetImage:
	return SProcShmGetImage(client);
    case X_ShmCreatePixmap:
	return SProcShmCreatePixmap(client);
    default:
	return BadRequest;
    }
}
