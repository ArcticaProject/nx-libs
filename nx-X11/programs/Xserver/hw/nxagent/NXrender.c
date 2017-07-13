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

/*
 * Copyright © 2000 SuSE, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of SuSE not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  SuSE makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * SuSE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL SuSE
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Keith Packard, SuSE, Inc.
 */

#include "../render/render.c"

#include "Trap.h"

#include "Render.h"
#include "Pixmaps.h"
#include "Options.h"
#include "Screen.h"
#include "Cursor.h"

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

/*
 * From NXglyph.c.
 */

extern
void GlyphExtents(int nlist, GlyphListPtr list,
                        GlyphPtr *glyphs, BoxPtr extents);

/*
 * From NXmitrap.c.
 */

extern
void miTrapezoidBounds (int ntrap, xTrapezoid *traps, BoxPtr box);

/*
 * Functions from Render.c.
 */

extern int  nxagentCursorSaveRenderInfo(ScreenPtr, CursorPtr);
extern void nxagentCursorPostSaveRenderInfo(CursorPtr, ScreenPtr, PicturePtr, int, int);
extern int  nxagentRenderRealizeCursor(ScreenPtr, CursorPtr);
extern int  nxagentCreatePicture(PicturePtr, Mask);
extern void nxagentChangePicture(PicturePtr, Mask);
extern int  nxagentChangePictureClip(PicturePtr, int, int, xRectangle *, int, int);
extern void nxagentComposite(CARD8, PicturePtr, PicturePtr, PicturePtr, INT16, INT16,
                             INT16, INT16, INT16, INT16, CARD16, CARD16);
extern void nxagentCompositeRects(CARD8, PicturePtr, xRenderColor *, int, xRectangle *);
extern void nxagentCreateGlyphSet(GlyphSetPtr glyphSet);
extern void nxagentReferenceGlyphSet(GlyphSetPtr glyphSet);
extern void nxagentFreeGlyphs(GlyphSetPtr glyphSet, CARD32 *gids, int nglyph);
extern void nxagentFreeGlyphSet(GlyphSetPtr glyphSet);
extern void nxagentSetPictureTransform(PicturePtr pPicture, void * transform);
extern void nxagentSetPictureFilter(PicturePtr pPicture, char *filter, int name_size,
                                    void * params, int nparams);
extern void nxagentTrapezoids(CARD8 op, PicturePtr pSrc, PicturePtr pDst, PictFormatPtr maskFormat,
                              INT16 xSrc, INT16 ySrc, int ntrap, xTrapezoid *traps);
extern void nxagentRenderCreateSolidFill(PicturePtr pPicture, xRenderColor *color);
extern void nxagentRenderCreateLinearGradient(PicturePtr pPicture, xPointFixed *p1,
                                              xPointFixed *p2, int nStops,
                                              xFixed *stops,
                                              xRenderColor *colors);
extern void nxagentRenderCreateRadialGradient(PicturePtr pPicture, xPointFixed *inner,
                                              xPointFixed *outer,
                                              xFixed innerRadius,
                                              xFixed outerRadius,
                                              int nStops,
                                              xFixed *stops,
                                              xRenderColor *colors);
extern void nxagentRenderCreateConicalGradient(PicturePtr pPicture,
                                               xPointFixed *center,
                                               xFixed angle, int nStops, 
                                               xFixed *stops, 
                                               xRenderColor *colors);

/*
 * The void pointer is actually a XGlyphElt8.
 */

void nxagentGlyphs(CARD8, PicturePtr, PicturePtr, PictFormatPtr,
                       INT16, INT16, int, void *, int, GlyphPtr *);

static int
ProcRenderQueryVersion (ClientPtr client)
{
    RenderClientPtr pRenderClient = GetRenderClient (client);
    xRenderQueryVersionReply rep;
    REQUEST(xRenderQueryVersionReq);

    REQUEST_SIZE_MATCH(xRenderQueryVersionReq);

    pRenderClient->major_version = stuff->majorVersion;
    pRenderClient->minor_version = stuff->minorVersion;

    memset(&rep, 0, sizeof(xRenderQueryVersionReply));
    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;

    if ((stuff->majorVersion * 1000 + stuff->minorVersion) <
        (nxagentRenderVersionMajor * 1000 + nxagentRenderVersionMinor))
    {
	rep.majorVersion = stuff->majorVersion;
	rep.minorVersion = stuff->minorVersion;
    } else
    {
	rep.majorVersion = nxagentRenderVersionMajor;
	rep.minorVersion = nxagentRenderVersionMinor;
    }

    if (client->swapped) {
	swaps(&rep.sequenceNumber);
	swapl(&rep.length);
	swapl(&rep.majorVersion);
	swapl(&rep.minorVersion);
    }
    WriteToClient(client, sizeof(xRenderQueryVersionReply), &rep);
    return (client->noClientException);
}

static int
ProcRenderQueryPictFormats (ClientPtr client)
{
    RenderClientPtr		    pRenderClient = GetRenderClient (client);
    xRenderQueryPictFormatsReply    *reply;
    xPictScreen			    *pictScreen;
    xPictDepth			    *pictDepth;
    xPictVisual			    *pictVisual;
    xPictFormInfo		    *pictForm;
    CARD32			    *pictSubpixel;
    ScreenPtr			    pScreen;
    VisualPtr			    pVisual;
    DepthPtr			    pDepth;
    int				    v, d;
    PictureScreenPtr		    ps;
    PictFormatPtr		    pFormat;
    int				    nformat;
    int				    ndepth;
    int				    nvisual;
    int				    rlength;
    int				    s;
    int				    numScreens;
    int				    numSubpixel;

    extern int                      nxagentAlphaEnabled;
/*    REQUEST(xRenderQueryPictFormatsReq); */

    REQUEST_SIZE_MATCH(xRenderQueryPictFormatsReq);

#ifdef PANORAMIX
    if (noPanoramiXExtension)
	numScreens = screenInfo.numScreens;
    else 
        numScreens = ((xConnSetup *)ConnectionInfo)->numRoots;
#else
    numScreens = screenInfo.numScreens;
#endif
    ndepth = nformat = nvisual = 0;
    for (s = 0; s < numScreens; s++)
    {
	pScreen = screenInfo.screens[s];
	for (d = 0; d < pScreen->numDepths; d++)
	{
	    pDepth = pScreen->allowedDepths + d;
	    ++ndepth;

	    for (v = 0; v < pDepth->numVids; v++)
	    {
		pVisual = findVisual (pScreen, pDepth->vids[v]);
		if (pVisual && PictureMatchVisual (pScreen, pDepth->depth, pVisual))
		    ++nvisual;
	    }
	}
	ps = GetPictureScreenIfSet(pScreen);
	if (ps)
	    nformat += ps->nformats;
    }
    if (pRenderClient->major_version == 0 && pRenderClient->minor_version < 6)
	numSubpixel = 0;
    else
	numSubpixel = numScreens;
    
    rlength = (sizeof (xRenderQueryPictFormatsReply) +
	       nformat * sizeof (xPictFormInfo) +
	       numScreens * sizeof (xPictScreen) +
	       ndepth * sizeof (xPictDepth) +
	       nvisual * sizeof (xPictVisual) +
	       numSubpixel * sizeof (CARD32));
    reply = (xRenderQueryPictFormatsReply *) calloc (1, rlength);
    if (!reply)
	return BadAlloc;
    reply->type = X_Reply;
    reply->sequenceNumber = client->sequence;
    reply->length = (rlength - sizeof(xGenericReply)) >> 2;
    reply->numFormats = nformat;
    reply->numScreens = numScreens;
    reply->numDepths = ndepth;
    reply->numVisuals = nvisual;
    reply->numSubpixel = numSubpixel;
    
    pictForm = (xPictFormInfo *) (reply + 1);
    
    for (s = 0; s < numScreens; s++)
    {
	pScreen = screenInfo.screens[s];
	ps = GetPictureScreenIfSet(pScreen);
	if (ps)
	{
	    for (nformat = 0, pFormat = ps->formats; 
		 nformat < ps->nformats;
		 nformat++, pFormat++)
	    {
		pictForm->id = pFormat->id;
		pictForm->type = pFormat->type;
		pictForm->depth = pFormat->depth;
		pictForm->direct.red = pFormat->direct.red;
		pictForm->direct.redMask = pFormat->direct.redMask;
		pictForm->direct.green = pFormat->direct.green;
		pictForm->direct.greenMask = pFormat->direct.greenMask;
		pictForm->direct.blue = pFormat->direct.blue;
		pictForm->direct.blueMask = pFormat->direct.blueMask;
		pictForm->direct.alpha = nxagentAlphaEnabled ? pFormat->direct.alpha : 0;
		pictForm->direct.alphaMask = pFormat->direct.alphaMask;
		if (pFormat->type == PictTypeIndexed && pFormat->index.pColormap)
		    pictForm->colormap = pFormat->index.pColormap->mid;
		else
		    pictForm->colormap = None;
		if (client->swapped)
		{
		    swapl (&pictForm->id);
		    swaps (&pictForm->direct.red);
		    swaps (&pictForm->direct.redMask);
		    swaps (&pictForm->direct.green);
		    swaps (&pictForm->direct.greenMask);
		    swaps (&pictForm->direct.blue);
		    swaps (&pictForm->direct.blueMask);
		    swaps (&pictForm->direct.alpha);
		    swaps (&pictForm->direct.alphaMask);
		    swapl (&pictForm->colormap);
		}
		pictForm++;
	    }
	}
    }
    
    pictScreen = (xPictScreen *) pictForm;
    for (s = 0; s < numScreens; s++)
    {
	pScreen = screenInfo.screens[s];
	pictDepth = (xPictDepth *) (pictScreen + 1);
	ndepth = 0;
	for (d = 0; d < pScreen->numDepths; d++)
	{
	    pictVisual = (xPictVisual *) (pictDepth + 1);
	    pDepth = pScreen->allowedDepths + d;

	    nvisual = 0;
	    for (v = 0; v < pDepth->numVids; v++)
	    {
		pVisual = findVisual (pScreen, pDepth->vids[v]);
		if (pVisual && (pFormat = PictureMatchVisual (pScreen, 
							      pDepth->depth, 
							      pVisual)))
		{
		    pictVisual->visual = pVisual->vid;
		    pictVisual->format = pFormat->id;
		    if (client->swapped)
		    {
			swapl (&pictVisual->visual);
			swapl (&pictVisual->format);
		    }
		    pictVisual++;
		    nvisual++;
		}
	    }
	    pictDepth->depth = pDepth->depth;
	    pictDepth->nPictVisuals = nvisual;
	    if (client->swapped)
	    {
		swaps (&pictDepth->nPictVisuals);
	    }
	    ndepth++;
	    pictDepth = (xPictDepth *) pictVisual;
	}
	pictScreen->nDepth = ndepth;
	ps = GetPictureScreenIfSet(pScreen);
	if (ps)
	    pictScreen->fallback = ps->fallback->id;
	else
	    pictScreen->fallback = 0;
	if (client->swapped)
	{
	    swapl (&pictScreen->nDepth);
	    swapl (&pictScreen->fallback);
	}
	pictScreen = (xPictScreen *) pictDepth;
    }
    pictSubpixel = (CARD32 *) pictScreen;
    
    for (s = 0; s < numSubpixel; s++)
    {
	pScreen = screenInfo.screens[s];
	ps = GetPictureScreenIfSet(pScreen);
	if (ps)
	    *pictSubpixel = ps->subpixel;
	else
	    *pictSubpixel = SubPixelUnknown;
	if (client->swapped)
	{
	    swapl (pictSubpixel);
	}
	++pictSubpixel;
    }
    
    if (client->swapped)
    {
	swaps (&reply->sequenceNumber);
	swapl (&reply->length);
	swapl (&reply->numFormats);
	swapl (&reply->numScreens);
	swapl (&reply->numDepths);
	swapl (&reply->numVisuals);
	swapl (&reply->numSubpixel);
    }
    WriteToClient(client, rlength, reply);
    free (reply);
    return client->noClientException;
}

static int
ProcRenderCreatePicture (ClientPtr client)
{
    PicturePtr	    pPicture;
    DrawablePtr	    pDrawable;
    PictFormatPtr   pFormat;
    int		    len;
    int		    error;
    REQUEST(xRenderCreatePictureReq);

    REQUEST_AT_LEAST_SIZE(xRenderCreatePictureReq);

    LEGAL_NEW_RESOURCE(stuff->pid, client);
    SECURITY_VERIFY_DRAWABLE(pDrawable, stuff->drawable, client,
			     DixWriteAccess);
    pFormat = (PictFormatPtr) SecurityLookupIDByType (client, 
						      stuff->format,
						      PictFormatType,
						      DixReadAccess);
    if (!pFormat)
    {
	client->errorValue = stuff->format;
	return RenderErrBase + BadPictFormat;
    }
    if (pFormat->depth != pDrawable->depth)
	return BadMatch;
    len = client->req_len - (sizeof(xRenderCreatePictureReq) >> 2);
    if (Ones(stuff->mask) != len)
	return BadLength;
    
    pPicture = CreatePicture (stuff->pid,
			      pDrawable,
			      pFormat,
			      stuff->mask,
			      (XID *) (stuff + 1),
			      client,
			      &error);
    if (!pPicture)
	return error;
    nxagentCreatePicture(pPicture, stuff -> mask);

    if (!AddResource (stuff->pid, PictureType, (void *)pPicture))
	return BadAlloc;
    return Success;
}

static int
ProcRenderChangePicture (ClientPtr client)
{
    PicturePtr	    pPicture;
    REQUEST(xRenderChangePictureReq);
    int len;
    int error;

    REQUEST_AT_LEAST_SIZE(xRenderChangePictureReq);
    VERIFY_PICTURE (pPicture, stuff->picture, client, DixWriteAccess,
		    RenderErrBase + BadPicture);

    len = client->req_len - (sizeof(xRenderChangePictureReq) >> 2);
    if (Ones(stuff->mask) != len)
	return BadLength;
    
    error = ChangePicture (pPicture, stuff->mask, (XID *) (stuff + 1),
			  (DevUnion *) 0, client);
    
    nxagentChangePicture(pPicture, stuff->mask);

    return error;
}

static int
ProcRenderSetPictureClipRectangles (ClientPtr client)
{
    REQUEST(xRenderSetPictureClipRectanglesReq);
    PicturePtr	    pPicture;
    int		    nr;
    int		    result;

    REQUEST_AT_LEAST_SIZE(xRenderSetPictureClipRectanglesReq);
    VERIFY_PICTURE (pPicture, stuff->picture, client, DixWriteAccess,
		    RenderErrBase + BadPicture);
    if (!pPicture->pDrawable)
        return BadDrawable;

    /*
     * The original code used sizeof(xRenderChangePictureReq).
     * This was harmless, as both structures have the same size.
     *
     * nr = (client->req_len << 2) - sizeof(xRenderChangePictureReq);
     */
    nr = (client->req_len << 2) - sizeof(xRenderSetPictureClipRectanglesReq);
    if (nr & 4)
	return BadLength;
    nr >>= 3;
    result = SetPictureClipRects (pPicture, 
				  stuff->xOrigin, stuff->yOrigin,
				  nr, (xRectangle *) &stuff[1]);
    nxagentChangePictureClip (pPicture,
                              CT_NONE,
                              nr,
                              (xRectangle *) &stuff[1],
                              (int)stuff -> xOrigin,
                              (int)stuff -> yOrigin);

    if (client->noClientException != Success)
        return(client->noClientException);
    else
        return(result);
}

/*
 * Check if both pictures have drawables which are
 * virtual pixmaps. See the corresponding define
 * in NXpicture.c
 */

#define NXAGENT_PICTURE_ALWAYS_POINTS_TO_VIRTUAL

#ifdef NXAGENT_PICTURE_ALWAYS_POINTS_TO_VIRTUAL

#define nxagentCompositePredicate(pSrc, pDst)  TRUE

#else

/*
 * This is still under development. The final
 * goal is to let pictures point to the real
 * pixmaps instead of pointing to virtuals.
 */

int nxagentCompositePredicate(PicturePtr pSrc, PicturePtr pDst)
{
  PixmapPtr pPixmap1;
  PixmapPtr pPixmap2;

  pPixmap1 = (pSrc -> pDrawable -> type == DRAWABLE_PIXMAP ?
                 ((PixmapPtr) pSrc -> pDrawable) : NULL);

  pPixmap2 = (pDst -> pDrawable -> type == DRAWABLE_PIXMAP ?
                 ((PixmapPtr) pDst -> pDrawable) : NULL);

  if (pPixmap1 == NULL || pPixmap2 == NULL)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentCompositePredicate: Case 0.\n");
    #endif

    return FALSE;
  }
  else
  {
    #ifdef TEST
    fprintf(stderr, "nxagentCompositePredicate: Case 1.\n");
    #endif

    if (nxagentPixmapIsVirtual(pPixmap1) == 1 &&
            nxagentPixmapIsVirtual(pPixmap2) == 1)
    {
      #ifdef TEST
      fprintf(stderr, "nxagentCompositePredicate: Case 2.\n");
      #endif

      return TRUE;
    }
  }

  #ifdef TEST
  fprintf(stderr, "nxagentCompositePredicate: Case 3.\n");
  #endif

  return FALSE;
}

#endif

static int
ProcRenderComposite (ClientPtr client)
{
    PicturePtr	pSrc, pMask, pDst;
    REQUEST(xRenderCompositeReq);

    REQUEST_SIZE_MATCH(xRenderCompositeReq);
    if (!PictOpValid (stuff->op))
    {
	client->errorValue = stuff->op;
	return BadValue;
    }
    VERIFY_PICTURE (pDst, stuff->dst, client, DixWriteAccess,
		    RenderErrBase + BadPicture);
    if (!pDst->pDrawable)
        return BadDrawable;
    VERIFY_PICTURE (pSrc, stuff->src, client, DixReadAccess,
		    RenderErrBase + BadPicture);
    VERIFY_ALPHA (pMask, stuff->mask, client, DixReadAccess,
		  RenderErrBase + BadPicture);

    if ((pSrc->pDrawable && pSrc->pDrawable->pScreen != pDst->pDrawable->pScreen) ||
	(pMask && pMask->pDrawable && pDst->pDrawable->pScreen != pMask->pDrawable->pScreen))
	return BadMatch;

    ValidatePicture (pSrc);
    if (pMask)
        ValidatePicture (pMask);
    ValidatePicture (pDst);

    #ifdef NXAGENT_PICTURE_ALWAYS_POINTS_TO_VIRTUAL

    if (nxagentCompositePredicate(pSrc, pDst))
    {
      #ifdef TEST
      fprintf(stderr, "ProcRenderComposite: Going to composite with "
                  "source at [%p] mask at [%p] and destination at [%p].\n",
                      (void *) pSrc, (void *) pMask, (void *) pDst);
      #endif

    CompositePicture (stuff->op,
		      pSrc,
		      pMask,
		      pDst,
		      stuff->xSrc,
		      stuff->ySrc,
		      stuff->xMask,
		      stuff->yMask,
		      stuff->xDst,
		      stuff->yDst,
		      stuff->width,
		      stuff->height);
    }

    #else

    if (pSrc -> pDrawable -> type == DRAWABLE_PIXMAP &&
            pDst -> pDrawable -> type == DRAWABLE_PIXMAP &&
                (!pMask || pMask -> pDrawable -> type == DRAWABLE_PIXMAP))
    {
      PixmapPtr pVirtualPixmapSrc;
      PixmapPtr pVirtualPixmapDst;
      PixmapPtr pVirtualPixmapMask;

      PicturePtr pVirtualPictureSrc;
      PicturePtr pVirtualPictureDst;
      PicturePtr pVirtualPictureMask;

      pVirtualPixmapSrc  = (PixmapPtr) pSrc  -> pDrawable;
      pVirtualPictureSrc = nxagentPixmapPriv(pVirtualPixmapSrc) -> pPicture;

      pVirtualPixmapDst  = (PixmapPtr) pDst  -> pDrawable;
      pVirtualPictureDst = nxagentPixmapPriv(pVirtualPixmapDst) -> pPicture;

      if (pMask)
      {
        pVirtualPixmapMask  = (PixmapPtr) pMask  -> pDrawable;
        pVirtualPictureMask = nxagentPixmapPriv(pVirtualPixmapMask) -> pPicture;
      }
      else
      {
        pVirtualPixmapMask  = NULL;
        pVirtualPictureMask = NULL;
      }

      if (pVirtualPictureSrc && pVirtualPictureDst)
      {
      #ifdef TEST
      fprintf(stderr, "ProcRenderComposite: Going to composite with "
                  "source at [%p] mask at [%p] and destination at [%p].\n",
                      (void *) pVirtualPixmapSrc, (void *) pVirtualPixmapMask,
                          (void *) pVirtualPixmapDst);
      #endif

      CompositePicture (stuff->op,
                        pVirtualPictureSrc,
                        pVirtualPictureMask,
                        pVirtualPictureDst,
                        stuff->xSrc,
                        stuff->ySrc,
                        stuff->xMask,
                        stuff->yMask,
                        stuff->xDst,
                        stuff->yDst,
                        stuff->width,
                        stuff->height);
      }
    }

    #endif

    nxagentComposite (stuff -> op,
                      pSrc,
                      pMask,
                      pDst,
                      stuff -> xSrc,
                      stuff -> ySrc,
                      stuff -> xMask,
                      stuff -> yMask,
                      stuff -> xDst,
                      stuff -> yDst,
                      stuff -> width,
                      stuff -> height);

    return Success;
}

static int
ProcRenderTrapezoids (ClientPtr client)
{
    int		ntraps;
    PicturePtr	pSrc, pDst;
    PictFormatPtr   pFormat;
    REQUEST(xRenderTrapezoidsReq);

    REQUEST_AT_LEAST_SIZE(xRenderTrapezoidsReq);
    if (!PictOpValid (stuff->op))
    {
	client->errorValue = stuff->op;
	return BadValue;
    }
    VERIFY_PICTURE (pSrc, stuff->src, client, DixReadAccess,
		    RenderErrBase + BadPicture);
    VERIFY_PICTURE (pDst, stuff->dst, client, DixWriteAccess,
		    RenderErrBase + BadPicture);
    if (!pDst->pDrawable)
        return BadDrawable;
    if (pSrc->pDrawable && pSrc->pDrawable->pScreen != pDst->pDrawable->pScreen)
	return BadMatch;
    if (stuff->maskFormat)
    {
	pFormat = (PictFormatPtr) SecurityLookupIDByType (client,
							  stuff->maskFormat,
							  PictFormatType,
							  DixReadAccess);
	if (!pFormat)
	{
	    client->errorValue = stuff->maskFormat;
	    return RenderErrBase + BadPictFormat;
	}
    }
    else
	pFormat = 0;
    ntraps = (client->req_len << 2) - sizeof (xRenderTrapezoidsReq);
    if (ntraps % sizeof (xTrapezoid))
	return BadLength;
    ntraps /= sizeof (xTrapezoid);
    if (ntraps)
    {
      if (pFormat != NULL)
      {
        nxagentTrapezoidExtents = (BoxPtr) malloc(sizeof(BoxRec));

        miTrapezoidBounds (ntraps, (xTrapezoid *) &stuff[1], nxagentTrapezoidExtents);
      }

      if (nxagentCompositePredicate(pSrc, pDst) == 1)
      {
	CompositeTrapezoids (stuff->op, pSrc, pDst, pFormat,
			     stuff->xSrc, stuff->ySrc,
			     ntraps, (xTrapezoid *) &stuff[1]);
      }

      nxagentTrapezoids (stuff->op, pSrc, pDst, pFormat,
                             stuff->xSrc, stuff->ySrc,
                                 ntraps, (xTrapezoid *) &stuff[1]);

      if (nxagentTrapezoidExtents != NullBox)
      {
        free(nxagentTrapezoidExtents);

        nxagentTrapezoidExtents = NullBox;
      }
    }

    return client->noClientException;
}

static int
ProcRenderCreateGlyphSet (ClientPtr client)
{
    GlyphSetPtr	    glyphSet;
    PictFormatPtr   format;
    int		    f;
    REQUEST(xRenderCreateGlyphSetReq);

    REQUEST_SIZE_MATCH(xRenderCreateGlyphSetReq);

    LEGAL_NEW_RESOURCE(stuff->gsid, client);
    format = (PictFormatPtr) SecurityLookupIDByType (client,
						     stuff->format,
						     PictFormatType,
						     DixReadAccess);
    if (!format)
    {
	client->errorValue = stuff->format;
	return RenderErrBase + BadPictFormat;
    }
    switch (format->depth) {
    case 1:
	f = GlyphFormat1;
	break;
    case 4:
	f = GlyphFormat4;
	break;
    case 8:
	f = GlyphFormat8;
	break;
    case 16:
	f = GlyphFormat16;
	break;
    case 32:
	f = GlyphFormat32;
	break;
    default:
	return BadMatch;
    }
    if (format->type != PictTypeDirect)
	return BadMatch;
    glyphSet = AllocateGlyphSet (f, format);
    if (!glyphSet)
	return BadAlloc;
    if (!AddResource (stuff->gsid, GlyphSetType, (void *)glyphSet))
	return BadAlloc;

    nxagentCreateGlyphSet(glyphSet);

    return Success;
}

static int
ProcRenderReferenceGlyphSet (ClientPtr client)
{
    GlyphSetPtr     glyphSet;
    REQUEST(xRenderReferenceGlyphSetReq);

    REQUEST_SIZE_MATCH(xRenderReferenceGlyphSetReq);

    LEGAL_NEW_RESOURCE(stuff->gsid, client);

    glyphSet = (GlyphSetPtr) SecurityLookupIDByType (client,
						     stuff->existing,
						     GlyphSetType,
						     DixWriteAccess);
    if (!glyphSet)
    {
	client->errorValue = stuff->existing;
	return RenderErrBase + BadGlyphSet;
    }
    glyphSet->refcnt++;

    nxagentReferenceGlyphSet(glyphSet);

    if (!AddResource (stuff->gsid, GlyphSetType, (void *)glyphSet))
	return BadAlloc;
    return client->noClientException;
}

static int
ProcRenderFreeGlyphSet (ClientPtr client)
{
    GlyphSetPtr     glyphSet;
    REQUEST(xRenderFreeGlyphSetReq);

    REQUEST_SIZE_MATCH(xRenderFreeGlyphSetReq);
    glyphSet = (GlyphSetPtr) SecurityLookupIDByType (client,
						     stuff->glyphset,
						     GlyphSetType,
						     DixDestroyAccess);
    if (!glyphSet)
    {
	client->errorValue = stuff->glyphset;
	return RenderErrBase + BadGlyphSet;
    }

    nxagentFreeGlyphSet(glyphSet);

    FreeResource (stuff->glyphset, RT_NONE);
    return client->noClientException;
}

static int
ProcRenderFreeGlyphs (ClientPtr client)
{
    REQUEST(xRenderFreeGlyphsReq);
    GlyphSetPtr     glyphSet;
    int		    nglyph;
    CARD32	    *gids;
    CARD32	    glyph;

    REQUEST_AT_LEAST_SIZE(xRenderFreeGlyphsReq);
    glyphSet = (GlyphSetPtr) SecurityLookupIDByType (client,
						     stuff->glyphset,
						     GlyphSetType,
						     DixWriteAccess);
    if (!glyphSet)
    {
	client->errorValue = stuff->glyphset;
	return RenderErrBase + BadGlyphSet;
    }
    nglyph = ((client->req_len << 2) - sizeof (xRenderFreeGlyphsReq)) >> 2;
    gids = (CARD32 *) (stuff + 1);

    nxagentFreeGlyphs(glyphSet, gids, nglyph);

    while (nglyph-- > 0)
    {
	glyph = *gids++;
	if (!DeleteGlyph (glyphSet, glyph))
	{
	    client->errorValue = glyph;
	    return RenderErrBase + BadGlyph;
	}
    }
    return client->noClientException;
}

typedef struct XGlyphElt8{
    GlyphSet                glyphset;
    _Xconst char            *chars;
    int                     nchars;
    int                     xOff;
    int                     yOff;
} XGlyphElt8;

static int
ProcRenderCompositeGlyphs (ClientPtr client)
{
    GlyphSetPtr     glyphSet;
    GlyphSet	    gs;
    PicturePtr      pSrc, pDst;
    PictFormatPtr   pFormat;
    GlyphListRec    listsLocal[NLOCALDELTA];
    GlyphListPtr    lists, listsBase;
    GlyphPtr	    glyphsLocal[NLOCALGLYPH];
    Glyph	    glyph;
    GlyphPtr	    *glyphs, *glyphsBase;
    xGlyphElt	    *elt;
    CARD8	    *buffer, *end;
    int		    nglyph;
    int		    nlist;
    int		    space;
    int		    size;
    int		    n;
    
    XGlyphElt8      *elements, *elementsBase;

    REQUEST(xRenderCompositeGlyphsReq);

    REQUEST_AT_LEAST_SIZE(xRenderCompositeGlyphsReq);

    switch (stuff->renderReqType) {
    default:			    size = 1; break;
    case X_RenderCompositeGlyphs16: size = 2; break;
    case X_RenderCompositeGlyphs32: size = 4; break;
    }
	    
    if (!PictOpValid (stuff->op))
    {
	client->errorValue = stuff->op;
	return BadValue;
    }
    VERIFY_PICTURE (pSrc, stuff->src, client, DixReadAccess,
		    RenderErrBase + BadPicture);
    VERIFY_PICTURE (pDst, stuff->dst, client, DixWriteAccess,
		    RenderErrBase + BadPicture);
    if (!pDst->pDrawable)
        return BadDrawable;
    if (pSrc->pDrawable && pSrc->pDrawable->pScreen != pDst->pDrawable->pScreen)
	return BadMatch;
    if (stuff->maskFormat)
    {
	pFormat = (PictFormatPtr) SecurityLookupIDByType (client,
							  stuff->maskFormat,
							  PictFormatType,
							  DixReadAccess);
	if (!pFormat)
	{
	    client->errorValue = stuff->maskFormat;
	    return RenderErrBase + BadPictFormat;
	}
    }
    else
	pFormat = 0;

    glyphSet = (GlyphSetPtr) SecurityLookupIDByType (client,
						     stuff->glyphset,
						     GlyphSetType,
						     DixReadAccess);
    if (!glyphSet)
    {
	client->errorValue = stuff->glyphset;
	return RenderErrBase + BadGlyphSet;
    }

    buffer = (CARD8 *) (stuff + 1);
    end = (CARD8 *) stuff + (client->req_len << 2);
    nglyph = 0;
    nlist = 0;
    while (buffer + sizeof (xGlyphElt) < end)
    {
	elt = (xGlyphElt *) buffer;
	buffer += sizeof (xGlyphElt);
	
	if (elt->len == 0xff)
	{
	    buffer += 4;
	}
	else
	{
	    nlist++;
	    nglyph += elt->len;
	    space = size * elt->len;
	    if (space & 3)
		space += 4 - (space & 3);
	    buffer += space;
	}
    }
    if (nglyph <= NLOCALGLYPH)
	glyphsBase = glyphsLocal;
    else
    {
	glyphsBase = (GlyphPtr *) malloc (nglyph * sizeof (GlyphPtr));
	if (!glyphsBase)
	    return BadAlloc;
    }
    if (nlist <= NLOCALDELTA)
	listsBase = listsLocal;
    else
    {
	listsBase = (GlyphListPtr) malloc (nlist * sizeof (GlyphListRec));
	if (!listsBase)
	    return BadAlloc;
    }

    elementsBase = malloc(nlist * sizeof(XGlyphElt8));
    if (!elementsBase)
      return BadAlloc;

    buffer = (CARD8 *) (stuff + 1);
    glyphs = glyphsBase;
    lists = listsBase;
    elements = elementsBase;
    while (buffer + sizeof (xGlyphElt) < end)
    {
	elt = (xGlyphElt *) buffer;
	buffer += sizeof (xGlyphElt);
	
	if (elt->len == 0xff)
	{
            #ifdef DEBUG
            fprintf(stderr, "ProcRenderCompositeGlyphs: Glyphset change with base size [%d].\n",
                        size);
            #endif

	    if (buffer + sizeof (GlyphSet) < end)
	    {
                memcpy(&gs, buffer, sizeof(GlyphSet));
		glyphSet = (GlyphSetPtr) SecurityLookupIDByType (client,
								 gs,
								 GlyphSetType,
								 DixReadAccess);
		if (!glyphSet)
		{
		    client->errorValue = gs;
		    if (glyphsBase != glyphsLocal)
			free (glyphsBase);
		    if (listsBase != listsLocal)
			free (listsBase);
		    return RenderErrBase + BadGlyphSet;
		}
	    }
	    buffer += 4;
	}
	else
	{
	    lists->xOff = elt->deltax;
	    lists->yOff = elt->deltay;
	    lists->format = glyphSet->format;
	    lists->len = 0;

            if (glyphSet -> remoteID == 0)
            {
              #ifdef TEST
              fprintf(stderr, "ProcRenderCompositeGlyphs: Going to reconnect glyphset at [%p].\n",
                          (void *) glyphSet);
              #endif

              nxagentReconnectGlyphSet(glyphSet, (XID) 0, (void*) NULL);
            }

            elements -> glyphset = glyphSet -> remoteID;
            elements -> chars = (char *) buffer;
            elements -> nchars = elt->len;
            elements -> xOff = elt->deltax;
            elements -> yOff = elt->deltay;
	    n = elt->len;
	    while (n--)
	    {
		if (buffer + size <= end)
		{
		    switch (size) {
		    case 1:
			glyph = *((CARD8 *)buffer); break;
		    case 2:
			glyph = *((CARD16 *)buffer); break;
		    case 4:
		    default:
			glyph = *((CARD32 *)buffer); break;
		    }
		    if ((*glyphs = FindGlyph (glyphSet, glyph)))
		    {
			lists->len++;
			glyphs++;
		    }
		}
		buffer += size;
	    }
	    space = size * elt->len;
	    if (space & 3)
		buffer += 4 - (space & 3);
	    lists++;
            elements++;
	}
    }
    if (buffer > end)
	return BadLength;

    /*
     * We need to know the glyphs extents to synchronize
     * the drawables involved in the composite text ope-
     * ration. Also we need to synchronize only the back-
     * ground of the text we are going to render, so the
     * operations on the framebuffer must be executed
     * after the X requests.
     */

    nxagentGlyphsExtents = (BoxPtr) malloc(sizeof(BoxRec));

    GlyphExtents(nlist, listsBase, glyphsBase, nxagentGlyphsExtents);

    nxagentGlyphs(stuff -> op,
                  pSrc,
                  pDst,
                  pFormat,
                  stuff -> xSrc,
                  stuff -> ySrc,
                  nlist,
                  elementsBase,
                  size,
                  glyphsBase);

    if (nxagentCompositePredicate(pSrc, pDst) == 1)
    {
      #ifdef TEST
      fprintf(stderr, "ProcRenderCompositeGlyphs: Going to composite glyphs with "
                  "source at [%p] and destination at [%p].\n",
                      (void *) pSrc, (void *) pDst);
      #endif

      CompositeGlyphs(stuff -> op,
                      pSrc,
                      pDst,
                      pFormat,
                      stuff -> xSrc,
                      stuff -> ySrc,
                      nlist,
                      listsBase,
                      glyphsBase);
    }

    free(nxagentGlyphsExtents);
    nxagentGlyphsExtents = NullBox;

    if (glyphsBase != glyphsLocal)
	free (glyphsBase);
    if (listsBase != listsLocal)
	free (listsBase);
    
    free(elementsBase);

    return client->noClientException;
}

static int
ProcRenderFillRectangles (ClientPtr client)
{
    PicturePtr	    pDst;
    int             things;
    REQUEST(xRenderFillRectanglesReq);
    
    REQUEST_AT_LEAST_SIZE (xRenderFillRectanglesReq);
    if (!PictOpValid (stuff->op))
    {
	client->errorValue = stuff->op;
	return BadValue;
    }
    VERIFY_PICTURE (pDst, stuff->dst, client, DixWriteAccess,
		    RenderErrBase + BadPicture);
    if (!pDst->pDrawable)
        return BadDrawable;
    
    things = (client->req_len << 2) - sizeof(xRenderFillRectanglesReq);
    if (things & 4)
	return(BadLength);
    things >>= 3;
    
    CompositeRects (stuff->op,
		    pDst,
		    &stuff->color,
		    things,
		    (xRectangle *) &stuff[1]);

    ValidatePicture (pDst);
    nxagentCompositeRects(stuff -> op,
                          pDst,
                          &stuff -> color,
                          things,
                          (xRectangle *) &stuff[1]);
    
    return client->noClientException;
}

static int
ProcRenderCreateCursor (ClientPtr client)
{
    REQUEST(xRenderCreateCursorReq);
    PicturePtr	    pSrc;
    ScreenPtr	    pScreen;
    unsigned short  width, height;
    CARD32	    *argbbits, *argb;
    unsigned char   *srcbits, *srcline;
    unsigned char   *mskbits, *mskline;
    int		    stride;
    int		    x, y;
    int		    nbytes_mono;
    CursorMetricRec cm;
    CursorPtr	    pCursor;
    CARD32	    twocolor[3];
    int		    ncolor;

    RealizeCursorProcPtr saveRealizeCursor;

    REQUEST_SIZE_MATCH (xRenderCreateCursorReq);
    LEGAL_NEW_RESOURCE(stuff->cid, client);
    
    VERIFY_PICTURE (pSrc, stuff->src, client, DixReadAccess,
		    RenderErrBase + BadPicture);
    if (!pSrc->pDrawable)
        return BadDrawable;
    pScreen = pSrc->pDrawable->pScreen;
    width = pSrc->pDrawable->width;
    height = pSrc->pDrawable->height;
    if (height && width > UINT32_MAX/(height*sizeof(CARD32)))
	return BadAlloc;
    if ( stuff->x > width 
      || stuff->y > height )
	return (BadMatch);
    argbbits = malloc (width * height * sizeof (CARD32));
    if (!argbbits)
	return (BadAlloc);
    
    stride = BitmapBytePad(width);
    nbytes_mono = stride*height;
    srcbits = (unsigned char *)malloc(nbytes_mono);
    if (!srcbits)
    {
	free (argbbits);
	return (BadAlloc);
    }
    mskbits = (unsigned char *)malloc(nbytes_mono);
    if (!mskbits)
    {
	free(argbbits);
	free(srcbits);
	return (BadAlloc);
    }
    bzero ((char *) mskbits, nbytes_mono);
    bzero ((char *) srcbits, nbytes_mono);

    if (pSrc->format == PICT_a8r8g8b8)
    {
	(*pScreen->GetImage) (pSrc->pDrawable,
			      0, 0, width, height, ZPixmap,
			      0xffffffff, (void *) argbbits);
    }
    else
    {
	PixmapPtr	pPixmap;
	PicturePtr	pPicture;
	PictFormatPtr	pFormat;
	int		error;

	pFormat = PictureMatchFormat (pScreen, 32, PICT_a8r8g8b8);
	if (!pFormat)
	{
	    free (argbbits);
	    free (srcbits);
	    free (mskbits);
	    return (BadImplementation);
	}
	pPixmap = (*pScreen->CreatePixmap) (pScreen, width, height, 32,
	                                   CREATE_PIXMAP_USAGE_SCRATCH);
	if (!pPixmap)
	{
	    free (argbbits);
	    free (srcbits);
	    free (mskbits);
	    return (BadAlloc);
	}
	pPicture = CreatePicture (0, &pPixmap->drawable, pFormat, 0, 0, 
				  client, &error);
	if (!pPicture)
	{
	    free (argbbits);
	    free (srcbits);
	    free (mskbits);
	    return error;
	}
	(*pScreen->DestroyPixmap) (pPixmap);
	CompositePicture (PictOpSrc,
			  pSrc, 0, pPicture,
			  0, 0, 0, 0, 0, 0, width, height);
	(*pScreen->GetImage) (pPicture->pDrawable,
			      0, 0, width, height, ZPixmap,
			      0xffffffff, (void *) argbbits);
	FreePicture (pPicture, 0);
    }
    /*
     * Check whether the cursor can be directly supported by 
     * the core cursor code
     */
    ncolor = 0;
    argb = argbbits;
    for (y = 0; ncolor <= 2 && y < height; y++)
    {
	for (x = 0; ncolor <= 2 && x < width; x++)
	{
	    CARD32  p = *argb++;
	    CARD32  a = (p >> 24);

	    if (a == 0)	    /* transparent */
		continue;
	    if (a == 0xff)  /* opaque */
	    {
		int n;
		for (n = 0; n < ncolor; n++)
		    if (p == twocolor[n])
			break;
		if (n == ncolor)
		    twocolor[ncolor++] = p;
	    }
	    else
		ncolor = 3;
	}
    }
    
    /*
     * Convert argb image to two plane cursor
     */
    srcline = srcbits;
    mskline = mskbits;
    argb = argbbits;
    for (y = 0; y < height; y++)
    {
	for (x = 0; x < width; x++)
	{
	    CARD32  p = *argb++;

	    if (ncolor <= 2)
	    {
		CARD32	a = ((p >> 24));

		RenderSetBit (mskline, x, a != 0);
		RenderSetBit (srcline, x, a != 0 && p == twocolor[0]);
	    }
	    else
	    {
		CARD32	a = ((p >> 24) * DITHER_SIZE + 127) / 255;
		CARD32	i = ((CvtR8G8B8toY15(p) >> 7) * DITHER_SIZE + 127) / 255;
		CARD32	d = orderedDither[y&(DITHER_DIM-1)][x&(DITHER_DIM-1)];
		/* Set mask from dithered alpha value */
		RenderSetBit (mskline, x, a > d);
		/* Set src from dithered intensity value */
		RenderSetBit (srcline, x, a > d && i <= d);
	    }
	}
	srcline += stride;
	mskline += stride;
    }
    /*
     * Dither to white and black if the cursor has more than two colors
     */
    if (ncolor > 2)
    {
	twocolor[0] = 0xff000000;
	twocolor[1] = 0xffffffff;
    }
    else
    {
	free (argbbits);
	argbbits = 0;
    }
    
#define GetByte(p,s)	(((p) >> (s)) & 0xff)
#define GetColor(p,s)	(GetByte(p,s) | (GetByte(p,s) << 8))
    
    cm.width = width;
    cm.height = height;
    cm.xhot = stuff->x;
    cm.yhot = stuff->y;

    /*
     * This cursor uses RENDER, so we make sure
     * that it is allocated in a way that allows
     * the mi and dix layers to handle it but we
     * later create it on the server by mirror-
     * ing the RENDER operation we got from the
     * client.
     */

    saveRealizeCursor = pScreen -> RealizeCursor;

    pScreen -> RealizeCursor = nxagentCursorSaveRenderInfo;

    pCursor = AllocCursorARGB (srcbits, mskbits, argbbits, &cm,
			       GetColor(twocolor[0], 16),
			       GetColor(twocolor[0], 8),
			       GetColor(twocolor[0], 0),
			       GetColor(twocolor[1], 16),
			       GetColor(twocolor[1], 8),
			       GetColor(twocolor[1], 0));

    pScreen -> RealizeCursor = saveRealizeCursor;

    /*
     * Store into the private data members the
     * information needed to recreate it at
     * reconnection. This is done in two steps
     * as in the first step we don't have the
     * picture info.
     */

    if (pCursor == NULL)
    {
      return BadAlloc;
    }

    nxagentCursorPostSaveRenderInfo(pCursor, pScreen, pSrc, stuff -> x, stuff -> y);

    nxagentRenderRealizeCursor(pScreen, pCursor);

    if (AddResource(stuff->cid, RT_CURSOR, (void *)pCursor))
	return (client->noClientException);
    return BadAlloc;
}

static int
ProcRenderSetPictureTransform (ClientPtr client)
{
    REQUEST(xRenderSetPictureTransformReq);
    PicturePtr	pPicture;
    int		result;

    REQUEST_SIZE_MATCH(xRenderSetPictureTransformReq);
    VERIFY_PICTURE (pPicture, stuff->picture, client, DixWriteAccess,
		    RenderErrBase + BadPicture);
    result = SetPictureTransform (pPicture, (PictTransform *) &stuff->transform);

    nxagentSetPictureTransform(pPicture, &stuff->transform);
    
    if (client->noClientException != Success)
        return(client->noClientException);
    else
        return(result);
}

static int
ProcRenderSetPictureFilter (ClientPtr client)
{
    REQUEST (xRenderSetPictureFilterReq);
    PicturePtr	pPicture;
    int		result;
    xFixed	*params;
    int		nparams;
    char	*name;
    
    REQUEST_AT_LEAST_SIZE (xRenderSetPictureFilterReq);
    VERIFY_PICTURE (pPicture, stuff->picture, client, DixWriteAccess,
		    RenderErrBase + BadPicture);
    name = (char *) (stuff + 1);
    params = (xFixed *) (name + ((stuff->nbytes + 3) & ~3));
    nparams = ((xFixed *) stuff + client->req_len) - params;
    result = SetPictureFilter (pPicture, name, stuff->nbytes, params, nparams);

    nxagentSetPictureFilter(pPicture, name, stuff->nbytes, params, nparams);

    return result;
}

static int
ProcRenderCreateAnimCursor (ClientPtr client)
{
    REQUEST(xRenderCreateAnimCursorReq);
    CursorPtr	    *cursors;
    CARD32	    *deltas;
    CursorPtr	    pCursor;
    int		    ncursor;
    xAnimCursorElt  *elt;
    int		    i;
    int		    ret;

    REQUEST_AT_LEAST_SIZE(xRenderCreateAnimCursorReq);
    LEGAL_NEW_RESOURCE(stuff->cid, client);
    if (client->req_len & 1)
	return BadLength;
    ncursor = (client->req_len - (SIZEOF(xRenderCreateAnimCursorReq) >> 2)) >> 1;
    cursors = malloc (ncursor * (sizeof (CursorPtr) + sizeof (CARD32)));
    if (!cursors)
	return BadAlloc;
    deltas = (CARD32 *) (cursors + ncursor);
    elt = (xAnimCursorElt *) (stuff + 1);
    for (i = 0; i < ncursor; i++)
    {
	cursors[i] = (CursorPtr)SecurityLookupIDByType(client, elt->cursor,
						       RT_CURSOR, DixReadAccess);
	if (!cursors[i])
	{
	    free (cursors);
	    client->errorValue = elt->cursor;
	    return BadCursor;
	}
	deltas[i] = elt->delay;
	elt++;
    }
    ret = AnimCursorCreate (cursors, deltas, ncursor, &pCursor);
    free (cursors);
    if (ret != Success)
	return ret;

    nxagentAnimCursorBits = pCursor -> bits;

    for (i = 0; i < MAXSCREENS; i++)
    {
      pCursor -> devPriv[i] = NULL;
    }

    if (AddResource (stuff->cid, RT_CURSOR, (void *)pCursor))
	return client->noClientException;
    return BadAlloc;
}

static int ProcRenderCreateSolidFill(ClientPtr client)
{
    PicturePtr	    pPicture;
    int		    error = 0;
    REQUEST(xRenderCreateSolidFillReq);

    REQUEST_AT_LEAST_SIZE(xRenderCreateSolidFillReq);

    LEGAL_NEW_RESOURCE(stuff->pid, client);

    pPicture = CreateSolidPicture(stuff->pid, &stuff->color, &error);
    if (!pPicture)
	return error;
    /* AGENT SERVER */

    nxagentRenderCreateSolidFill(pPicture, &stuff -> color);

    /* AGENT SERVER */
    if (!AddResource (stuff->pid, PictureType, (void *)pPicture))
	return BadAlloc;
    return Success;
}

static int ProcRenderCreateLinearGradient (ClientPtr client)
{
    PicturePtr	    pPicture;
    int		    len;
    int		    error = 0;
    xFixed          *stops;
    xRenderColor   *colors;
    REQUEST(xRenderCreateLinearGradientReq);

    REQUEST_AT_LEAST_SIZE(xRenderCreateLinearGradientReq);

    LEGAL_NEW_RESOURCE(stuff->pid, client);

    len = (client->req_len << 2) - sizeof(xRenderCreateLinearGradientReq);
    if (stuff->nStops > UINT32_MAX/(sizeof(xFixed) + sizeof(xRenderColor)))
	return BadLength;
    if (len != stuff->nStops*(sizeof(xFixed) + sizeof(xRenderColor)))
        return BadLength;

    stops = (xFixed *)(stuff + 1);
    colors = (xRenderColor *)(stops + stuff->nStops);

    pPicture = CreateLinearGradientPicture (stuff->pid, &stuff->p1, &stuff->p2,
                                            stuff->nStops, stops, colors, &error);
    if (!pPicture)
	return error;
    /* AGENT SERVER */

    nxagentRenderCreateLinearGradient(pPicture, &stuff->p1, &stuff->p2,
                                          stuff->nStops, stops, colors);

    /* AGENT SERVER */
    if (!AddResource (stuff->pid, PictureType, (void *)pPicture))
	return BadAlloc;
    return Success;
}

static int ProcRenderCreateRadialGradient (ClientPtr client)
{
    PicturePtr	    pPicture;
    int		    len;
    int		    error = 0;
    xFixed          *stops;
    xRenderColor   *colors;
    REQUEST(xRenderCreateRadialGradientReq);

    REQUEST_AT_LEAST_SIZE(xRenderCreateRadialGradientReq);

    LEGAL_NEW_RESOURCE(stuff->pid, client);

    len = (client->req_len << 2) - sizeof(xRenderCreateRadialGradientReq);
    if (len != stuff->nStops*(sizeof(xFixed) + sizeof(xRenderColor)))
        return BadLength;

    stops = (xFixed *)(stuff + 1);
    colors = (xRenderColor *)(stops + stuff->nStops);

    pPicture = CreateRadialGradientPicture (stuff->pid, &stuff->inner, &stuff->outer,
                                            stuff->inner_radius, stuff->outer_radius,
                                            stuff->nStops, stops, colors, &error);
    if (!pPicture)
	return error;
    /* AGENT SERVER */

    nxagentRenderCreateRadialGradient(pPicture, &stuff->inner, &stuff->outer,
                                          stuff->inner_radius,
                                              stuff->outer_radius, 
                                                  stuff->nStops, stops, colors);

    /* AGENT SERVER */
    if (!AddResource (stuff->pid, PictureType, (void *)pPicture))
	return BadAlloc;
    return Success;
}

static int ProcRenderCreateConicalGradient (ClientPtr client)
{
    PicturePtr	    pPicture;
    int		    len;
    int		    error = 0;
    xFixed          *stops;
    xRenderColor   *colors;
    REQUEST(xRenderCreateConicalGradientReq);

    REQUEST_AT_LEAST_SIZE(xRenderCreateConicalGradientReq);

    LEGAL_NEW_RESOURCE(stuff->pid, client);

    len = (client->req_len << 2) - sizeof(xRenderCreateConicalGradientReq);
    if (len != stuff->nStops*(sizeof(xFixed) + sizeof(xRenderColor)))
        return BadLength;

    stops = (xFixed *)(stuff + 1);
    colors = (xRenderColor *)(stops + stuff->nStops);

    pPicture = CreateConicalGradientPicture (stuff->pid, &stuff->center, stuff->angle,
                                             stuff->nStops, stops, colors, &error);
    if (!pPicture)
	return error;
    /* AGENT SERVER */

    nxagentRenderCreateConicalGradient(pPicture, &stuff->center,
                                           stuff->angle, stuff->nStops, stops,
                                               colors);

    /* AGENT SERVER */
    if (!AddResource (stuff->pid, PictureType, (void *)pPicture))
	return BadAlloc;
    return Success;
}


static int
ProcRenderDispatch (ClientPtr client)
{
    int result;

    REQUEST(xReq);

    /*
     * Let the client fail if we are
     * hiding the RENDER extension.
     */
    
    if (nxagentRenderTrap)
    {
        return BadRequest;
    }

    if (stuff->data < RenderNumberRequests)
    {
        #ifdef TEST
        fprintf(stderr, "ProcRenderDispatch: Request [%s] OPCODE#%d.\n",
                    nxagentRenderRequestLiteral[stuff->data], stuff->data);
        #endif

        /*
         * Set the nxagentGCTrap flag while
         * dispatching a render operation to
         * avoid reentrancy in GCOps.c.
         */

        nxagentGCTrap = 1;

        result = (*ProcRenderVector[stuff->data]) (client);

        nxagentGCTrap = 0;

        return result;
    }
    else
	return BadRequest;
}

static int
SProcRenderDispatch (ClientPtr client)
{
    int result;

    REQUEST(xReq);
    
    /*
     * Let the client fail if we are
     * hiding the RENDER extension.
     */
    
    if (nxagentRenderTrap)
    {
        return BadRequest;
    }

    if (stuff->data < RenderNumberRequests)
    {
        /*
         * Set the nxagentGCTrap flag while
         * dispatching a render operation to
         * avoid reentrancy in GCOps.c.
         */

        nxagentGCTrap = 1;

        result = (*SProcRenderVector[stuff->data]) (client);

        nxagentGCTrap = 0;

        return result;
    }
    else
	return BadRequest;
}
