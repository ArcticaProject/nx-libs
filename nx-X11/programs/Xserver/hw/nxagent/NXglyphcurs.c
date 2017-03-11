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

/************************************************************************

Copyright 1987, 1998  The Open Group

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


Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

************************************************************************/


#include "../../dix/glyphcurs.c"

#include "../../fb/fb.h"
#include "Pixmaps.h"

#ifndef True
#define True  1
#endif

/*
    get the bits out of the font in a portable way.  to avoid
dealing with padding and such-like, we draw the glyph into
a bitmap, then read the bits out with GetImage, which
uses server-natural format.
    since all screens return the same bitmap format, we'll just use
the first one we find.
    the character origin lines up with the hotspot in the
cursor metrics.
*/

int
ServerBitsFromGlyph(FontPtr pfont, unsigned ch, register CursorMetricPtr cm, unsigned char **ppbits)
{
    register ScreenPtr pScreen;
    register GCPtr pGC;
    xRectangle rect;
    PixmapPtr ppix;
    long nby;
    char *pbits;
    unsigned char char2b[2];

    /* turn glyph index into a protocol-format char2b */
    char2b[0] = (unsigned char)(ch >> 8);
    char2b[1] = (unsigned char)(ch & 0xff);

    pScreen = screenInfo.screens[0];
    nby = BitmapBytePad(cm->width) * (long)cm->height;
    pbits = (char *)malloc(nby);
    if (!pbits)
	return BadAlloc;
    /* zeroing the (pad) bits seems to help some ddx cursor handling */
    bzero(pbits, nby);

    ppix = fbCreatePixmap(pScreen, cm->width, cm->height, 1,
                          CREATE_PIXMAP_USAGE_SCRATCH);
    pGC = GetScratchGC(1, pScreen);
    if (!ppix || !pGC)
    {
	if (ppix)
	    fbDestroyPixmap(ppix);
	if (pGC)
	    FreeScratchGC(pGC);
	free(pbits);
	return BadAlloc;
    }

    #ifdef TEST
    fprintf(stderr, "ServerBitsFromGlyph: Created virtual pixmap at [%p] with width [%d] height [%d] depth [%d].\n",
                (void *) ppix, cm->width, cm->height, 1);
    #endif

    nxagentPixmapPriv(ppix) -> id = 0;
    nxagentPixmapPriv(ppix) -> mid = 0;
    nxagentPixmapPriv(ppix) -> isVirtual = True;
    nxagentPixmapPriv(ppix) -> pRealPixmap = NULL;
    nxagentPixmapPriv(ppix) -> pVirtualPixmap = NULL;

    rect.x = 0;
    rect.y = 0;
    rect.width = cm->width;
    rect.height = cm->height;

    pGC->stateChanges |= GCFunction | GCForeground | GCFont;
    pGC->alu = GXcopy;

    pGC->fgPixel = 0;

    pfont->refcnt++;

    if (pGC->font)
      CloseFont(pGC->font, (Font)0);

    pGC->font = pfont;

    ValidateGC((DrawablePtr)ppix, pGC);
    fbPolyFillRect((DrawablePtr)ppix, pGC, 1, &rect);

    /* draw the glyph */
    pGC->fgPixel = 1;

    pGC->stateChanges |= GCForeground;

    ValidateGC((DrawablePtr)ppix, pGC);
    miPolyText16((DrawablePtr)ppix, pGC, (int)cm->xhot, (int)cm->yhot, (int)1, (unsigned short*)char2b);
    fbGetImage((DrawablePtr)ppix, 0, 0, cm->width, cm->height,
                         XYPixmap, 1, pbits);
    *ppbits = (unsigned char *)pbits;
    FreeScratchGC(pGC);
    fbDestroyPixmap(ppix);

    #ifdef TEST
    fprintf(stderr, "ServerBitsFromGlyph: Destroyed virtual pixmap at [%p].\n",
                (void *) ppix);
    #endif

    return Success;
}
