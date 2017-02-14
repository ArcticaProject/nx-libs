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
 * $XFree86: xc/programs/Xserver/render/picture.c,v 1.29 2002/11/23 02:38:15 keithp Exp $
 *
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

#include "picturestr.h"

#include "Screen.h"
#include "Pixmaps.h"
#include "Drawable.h"
#include "Render.h"

/* prototypes */

PictFormatPtr PictureCreateDefaultFormats (ScreenPtr pScreen, int *nformatp);
PicturePtr AllocatePicture (ScreenPtr  pScreen);
PicturePtr CreatePicture (Picture       pid,
                          DrawablePtr   pDrawable,
                          PictFormatPtr pFormat,
                          Mask          vmask,
                          XID           *vlist,
                          ClientPtr     client,
                          int           *error);
static PicturePtr createSourcePicture(void);
int FreePicture (void *value, XID pid);

#include "../../render/picture.c"


#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

void *nxagentVisualFromID(ScreenPtr pScreen, VisualID visual);

void *nxagentMatchingFormats(PictFormatPtr pForm);

void nxagentPictureCreateDefaultFormats(ScreenPtr pScreen, FormatInitRec *formats, int *nformats);

PictFormatPtr
PictureCreateDefaultFormats (ScreenPtr pScreen, int *nformatp)
{
    int             nformats, f;
    PictFormatPtr   pFormats;
    FormatInitRec   formats[1024];
    CARD32	    format;

    nformats = 0;

    nxagentPictureCreateDefaultFormats(pScreen, formats, &nformats);

    pFormats = (PictFormatPtr) malloc (nformats * sizeof (PictFormatRec));
    if (!pFormats)
	return 0;
    memset (pFormats, '\0', nformats * sizeof (PictFormatRec));
    for (f = 0; f < nformats; f++)
    {
        pFormats[f].id = FakeClientID (0);
        pFormats[f].depth = formats[f].depth;
        format = formats[f].format;
        pFormats[f].format = format;
	switch (PICT_FORMAT_TYPE(format)) {
	case PICT_TYPE_ARGB:
	    pFormats[f].type = PictTypeDirect;
	    
	    pFormats[f].direct.alphaMask = Mask(PICT_FORMAT_A(format));
	    if (pFormats[f].direct.alphaMask)
		pFormats[f].direct.alpha = (PICT_FORMAT_R(format) +
					    PICT_FORMAT_G(format) +
					    PICT_FORMAT_B(format));
	    
	    pFormats[f].direct.redMask = Mask(PICT_FORMAT_R(format));
	    pFormats[f].direct.red = (PICT_FORMAT_G(format) + 
				      PICT_FORMAT_B(format));
	    
	    pFormats[f].direct.greenMask = Mask(PICT_FORMAT_G(format));
	    pFormats[f].direct.green = PICT_FORMAT_B(format);
	    
	    pFormats[f].direct.blueMask = Mask(PICT_FORMAT_B(format));
	    pFormats[f].direct.blue = 0;
	    break;

	case PICT_TYPE_ABGR:
	    pFormats[f].type = PictTypeDirect;
	    
	    pFormats[f].direct.alphaMask = Mask(PICT_FORMAT_A(format));
	    if (pFormats[f].direct.alphaMask)
		pFormats[f].direct.alpha = (PICT_FORMAT_B(format) +
					    PICT_FORMAT_G(format) +
					    PICT_FORMAT_R(format));
	    
	    pFormats[f].direct.blueMask = Mask(PICT_FORMAT_B(format));
	    pFormats[f].direct.blue = (PICT_FORMAT_G(format) + 
				       PICT_FORMAT_R(format));
	    
	    pFormats[f].direct.greenMask = Mask(PICT_FORMAT_G(format));
	    pFormats[f].direct.green = PICT_FORMAT_R(format);
	    
	    pFormats[f].direct.redMask = Mask(PICT_FORMAT_R(format));
	    pFormats[f].direct.red = 0;
	    break;

	case PICT_TYPE_A:
	    pFormats[f].type = PictTypeDirect;

	    pFormats[f].direct.alpha = 0;
	    pFormats[f].direct.alphaMask = Mask(PICT_FORMAT_A(format));

	    /* remaining fields already set to zero */
	    break;
	    
	case PICT_TYPE_COLOR:
	case PICT_TYPE_GRAY:
	    pFormats[f].type = PictTypeIndexed;
	    pFormats[f].index.vid = pScreen->visuals[PICT_FORMAT_VIS(format)].vid;
	    break;
	}

        if (nxagentMatchingFormats(&pFormats[f]) != NULL)
        {
          #ifdef DEBUG
          fprintf(stderr, "PictureCreateDefaultFormats: Format with type [%d] depth [%d] rgb [%d,%d,%d] "
                      "mask rgb [%d,%d,%d] alpha [%d] alpha mask [%d] matches.\n",
                          pFormats[f].type, pFormats[f].depth, pFormats[f].direct.red, pFormats[f].direct.green,
                              pFormats[f].direct.blue, pFormats[f].direct.redMask, pFormats[f].direct.greenMask,
                                  pFormats[f].direct.blueMask, pFormats[f].direct.alpha, pFormats[f].direct.alphaMask);
          #endif
        }
        else
        {
          #ifdef DEBUG
          fprintf(stderr, "PictureCreateDefaultFormats: Format with type [%d] depth [%d] rgb [%d,%d,%d] "
                      "mask rgb [%d,%d,%d] alpha [%d] alpha mask [%d] doesn't match.\n",
                          pFormats[f].type, pFormats[f].depth, pFormats[f].direct.red, pFormats[f].direct.green,
                              pFormats[f].direct.blue, pFormats[f].direct.redMask, pFormats[f].direct.greenMask,
                                  pFormats[f].direct.blueMask, pFormats[f].direct.alpha, pFormats[f].direct.alphaMask);
          #endif
        } 
    }
    *nformatp = nformats;
    return pFormats;
}

PicturePtr
AllocatePicture (ScreenPtr  pScreen)
{
    PictureScreenPtr	ps = GetPictureScreen(pScreen);
    PicturePtr		pPicture;
    char		*ptr;
    DevUnion		*ppriv;
    unsigned int    	*sizes;
    unsigned int    	size;
    int			i;

    pPicture = (PicturePtr) malloc (ps->totalPictureSize);
    if (!pPicture)
	return 0;
    ppriv = (DevUnion *)(pPicture + 1);
    pPicture->devPrivates = ppriv;
    sizes = ps->PicturePrivateSizes;
    ptr = (char *)(ppriv + ps->PicturePrivateLen);
    for (i = ps->PicturePrivateLen; --i >= 0; ppriv++, sizes++)
    {
	if ( (size = *sizes) )
	{
	    ppriv->ptr = (void *)ptr;
	    ptr += size;
	}
	else
	    ppriv->ptr = (void *)NULL;
    }

    nxagentPicturePriv(pPicture) -> picture = 0;

    return pPicture;
}

PicturePtr
CreatePicture (Picture		pid,
	       DrawablePtr	pDrawable,
	       PictFormatPtr	pFormat,
	       Mask		vmask,
	       XID		*vlist,
	       ClientPtr	client,
	       int		*error)
{
    PicturePtr		pPicture;
    PictureScreenPtr	ps = GetPictureScreen(pDrawable->pScreen);

    pPicture = AllocatePicture (pDrawable->pScreen);
    if (!pPicture)
    {
	*error = BadAlloc;
	return 0;
    }

    pPicture->id = pid;
    pPicture->pDrawable = pDrawable;
    pPicture->pFormat = pFormat;
    pPicture->format = pFormat->format | (pDrawable->bitsPerPixel << 24);
    if (pDrawable->type == DRAWABLE_PIXMAP)
    {
        /*
         * Let picture always point to the virtual pixmap.
         * For sure this is not the best way to deal with
         * the virtual frame-buffer.
         */
        pPicture->pDrawable = nxagentVirtualDrawable(pDrawable);

	++((PixmapPtr)pDrawable)->refcnt;
	pPicture->pNext = 0;
    }
    else
    {
	pPicture->pNext = GetPictureWindow(((WindowPtr) pDrawable));
	SetPictureWindow(((WindowPtr) pDrawable), pPicture);
    }

    SetPictureToDefaults (pPicture);
    
    if (vmask)
	*error = ChangePicture (pPicture, vmask, vlist, 0, client);
    else
	*error = Success;
    if (*error == Success)
	*error = (*ps->CreatePicture) (pPicture);
    if (*error != Success)
    {
	FreePicture (pPicture, (XID) 0);
	pPicture = 0;
    }
    return pPicture;
}

PicturePtr
CreateSolidPicture (Picture pid, xRenderColor *color, int *error)
{
    PicturePtr pPicture;
    pPicture = createSourcePicture();
    if (!pPicture) {
        *error = BadAlloc;
        return 0;
    }

    pPicture->id = pid;
    pPicture->pSourcePict = (SourcePictPtr) malloc(sizeof(PictSolidFill));
    if (!pPicture->pSourcePict) {
        *error = BadAlloc;
        free(pPicture);
        return 0;
    }
    pPicture->pSourcePict->type = SourcePictTypeSolidFill;
    pPicture->pSourcePict->solidFill.color = xRenderColorToCard32(*color);
    pPicture->pSourcePict->solidFill.fullColor.alpha=color->alpha;
    pPicture->pSourcePict->solidFill.fullColor.red=color->red;
    pPicture->pSourcePict->solidFill.fullColor.green=color->green;
    pPicture->pSourcePict->solidFill.fullColor.blue=color->blue;
    return pPicture;
}

static PicturePtr createSourcePicture(void)
{
    PicturePtr pPicture;

    extern int nxagentPicturePrivateIndex;

    unsigned int totalPictureSize;

    DevUnion *ppriv;

    char *privPictureRecAddr;

    int i;

    /*
     * Compute size of entire PictureRect, plus privates.
     */

    totalPictureSize = sizeof(PictureRec) +
                           picturePrivateCount * sizeof(DevUnion) +
                               sizeof(nxagentPrivPictureRec);

    pPicture = (PicturePtr) malloc(totalPictureSize);

    if (pPicture != NULL)
    {
      ppriv = (DevUnion *) (pPicture + 1);

      for (i = 0; i < picturePrivateCount; ++i)
      {
        /*
         * Other privates are inaccessible.
         */

        ppriv[i].ptr = NULL;
      }

      privPictureRecAddr = (char *) &ppriv[picturePrivateCount];

      ppriv[nxagentPicturePrivateIndex].ptr = (void *) privPictureRecAddr;

      pPicture -> devPrivates = ppriv;

      nxagentPicturePriv(pPicture) -> picture = 0;
    }
    else {
	return 0;
    }

    pPicture->pDrawable = 0;
    pPicture->pFormat = 0;
    pPicture->pNext = 0;

    SetPictureToDefaults(pPicture);
    return pPicture;
}

int
FreePicture (void *	value,
	     XID	pid)
{
    PicturePtr	pPicture = (PicturePtr) value;

    if (--pPicture->refcnt == 0)
    {
        nxagentDestroyPicture(pPicture);

	if (pPicture->transform)
	    free (pPicture->transform);
        if (!pPicture->pDrawable) {
            if (pPicture->pSourcePict) {
                if (pPicture->pSourcePict->type != SourcePictTypeSolidFill)
                    free(pPicture->pSourcePict->linear.stops);
                free(pPicture->pSourcePict);
            }
        } else {
            ScreenPtr	    pScreen = pPicture->pDrawable->pScreen;
            PictureScreenPtr    ps = GetPictureScreen(pScreen);
	
            if (pPicture->alphaMap)
                FreePicture ((void *) pPicture->alphaMap, (XID) 0);
            (*ps->DestroyPicture) (pPicture);
            (*ps->DestroyPictureClip) (pPicture);
            if (pPicture->pDrawable->type == DRAWABLE_WINDOW)
            {
                WindowPtr	pWindow = (WindowPtr) pPicture->pDrawable;
                PicturePtr	*pPrev;

                for (pPrev = (PicturePtr *) &((pWindow)->devPrivates[PictureWindowPrivateIndex].ptr);
                     *pPrev;
                     pPrev = &(*pPrev)->pNext)
                {
                    if (*pPrev == pPicture)
                    {
                        *pPrev = pPicture->pNext;
                        break;
                    }
                }
            }
            else if (pPicture->pDrawable->type == DRAWABLE_PIXMAP)
            {
                (*pScreen->DestroyPixmap) ((PixmapPtr)pPicture->pDrawable);
            }
        }
	free (pPicture);
    }
    return Success;
}

#ifndef True
# define True 1
#endif

#ifndef False
# define False 0
#endif

void nxagentReconnectPictFormat(void*, XID, void*);

Bool nxagentReconnectAllPictFormat(void *p)
{
  PictFormatPtr formats_old, formats;
  int nformats, nformats_old;
  VisualPtr pVisual;
  Bool success = True;
  Bool matched;
  int i, n;
  CARD32 type, a, r, g, b;

  #if defined(NXAGENT_RECONNECT_DEBUG) || defined(NXAGENT_RECONNECT_PICTFORMAT_DEBUG)
  fprintf(stderr, "nxagentReconnectAllPictFormat\n");
  #endif

  formats_old = GetPictureScreen(nxagentDefaultScreen) -> formats;
  nformats_old = GetPictureScreen(nxagentDefaultScreen) -> nformats;

  /*
   * TODO: We could copy PictureCreateDefaultFormats,
   *       in order not to waste ID with FakeClientID().
   */
  formats = PictureCreateDefaultFormats (nxagentDefaultScreen, &nformats);

  if (!formats)
    return False;

  for (n = 0; n < nformats; n++)
  {
    if (formats[n].type == PictTypeIndexed)
    {
      pVisual = nxagentVisualFromID(nxagentDefaultScreen, formats[n].index.vid);

      if ((pVisual->class | DynamicClass) == PseudoColor)
        type = PICT_TYPE_COLOR;
      else
        type = PICT_TYPE_GRAY;
      a = r = g = b = 0;
    }
    else
    {
      if ((formats[n].direct.redMask|
           formats[n].direct.blueMask|
           formats[n].direct.greenMask) == 0)
        type = PICT_TYPE_A;
      else if (formats[n].direct.red > formats[n].direct.blue)
        type = PICT_TYPE_ARGB;
      else
        type = PICT_TYPE_ABGR;
      a = Ones (formats[n].direct.alphaMask);
      r = Ones (formats[n].direct.redMask);
      g = Ones (formats[n].direct.greenMask);
      b = Ones (formats[n].direct.blueMask);
    }
    formats[n].format = PICT_FORMAT(0,type,a,r,g,b);
  }

  for (n = 0; n < nformats_old; n++)
  {
    for (i = 0, matched = False; (!matched) && (i < nformats); i++)
    {
      if (formats_old[n].format == formats[i].format &&
          formats_old[n].type == formats[i].type &&
          formats_old[n].direct.red == formats[i].direct.red &&
          formats_old[n].direct.green == formats[i].direct.green &&
          formats_old[n].direct.blue == formats[i].direct.blue &&
          formats_old[n].direct.redMask == formats[i].direct.redMask &&
          formats_old[n].direct.greenMask == formats[i].direct.greenMask &&
          formats_old[n].direct.blueMask == formats[i].direct.blueMask &&
          formats_old[n].direct.alpha == formats[i].direct.alpha &&
          formats_old[n].direct.alphaMask == formats[i].direct.alphaMask)
      {
       /*
        * Regard depth 16 and 15 as were the same, if all other values match.
        */

        if ((formats_old[n].depth == formats[i].depth) ||
               ((formats_old[n].depth == 15 || formats_old[n].depth == 16) &&
                    (formats[i].depth == 15 || formats[i].depth == 16)))
        {
          matched = True;
        }
      }
    }

    if (!matched)
    {
      return False;
    }
  }

  free(formats);

  /* TODO: Perhaps do i have to do PictureFinishInit ?. */
  /* TODO: We have to check for new Render protocol version. */

  for (i = 0; (i < MAXCLIENTS) && (success); i++)
  {
    if (clients[i])
    {
      FindClientResourcesByType(clients[i], PictFormatType, nxagentReconnectPictFormat, &success);
    }
  }

  return success;
}

/*
 * It seem we don't have nothing
 * to do for reconnect PictureFormat.
 */

void nxagentReconnectPictFormat(void *p0, XID x1, void *p2)
{
  #if defined(NXAGENT_RECONNECT_DEBUG) || defined(NXAGENT_RECONNECT_PICTFORMAT_DEBUG)
  fprintf(stderr, "nxagentReconnectPictFormat.\n");
  #endif
}

/*
 * The set of picture formats may change considerably
 * between different X servers. This poses a problem
 * while migrating NX sessions, because a requisite to
 * successfully reconnect the session is that all pic-
 * ture formats have to be available on the new X server.
 * To reduce such problems, we use a limited set of
 * pictures available on the most X servers.
 */

void nxagentPictureCreateDefaultFormats(ScreenPtr pScreen, FormatInitRec *formats, int *nformats)
{
  DepthPtr  pDepth;
  VisualPtr pVisual;

  CARD32 format;
  CARD8 depth;

  int r, g, b;
  int bpp;
  int d;
  int v;


  formats[*nformats].format = PICT_a1;
  formats[*nformats].depth = 1;
  *nformats += 1;
  formats[*nformats].format = PICT_a4;
  formats[*nformats].depth = 4;
  *nformats += 1;
  formats[*nformats].format = PICT_a8;
  formats[*nformats].depth = 8;
  *nformats += 1;
  formats[*nformats].format = PICT_a8r8g8b8;
  formats[*nformats].depth = 32;
  *nformats += 1;

  /*
   * This format should be required by the
   * protocol, but it's not used by Xgl.
   *
   * formats[*nformats].format = PICT_x8r8g8b8;
   * formats[*nformats].depth = 32;
   * *nformats += 1;
   */

  /* now look through the depths and visuals adding other formats */
  for (v = 0; v < pScreen->numVisuals; v++)
  {
    pVisual = &pScreen->visuals[v];
    depth = visualDepth (pScreen, pVisual);
    if (!depth)
      continue;

    bpp = BitsPerPixel (depth);

    switch (pVisual->class)
    {
      case DirectColor:
      case TrueColor:
        r = Ones (pVisual->redMask);
        g = Ones (pVisual->greenMask);
        b = Ones (pVisual->blueMask);

        if (pVisual->offsetBlue == 0 &&
            pVisual->offsetGreen == b &&
            pVisual->offsetRed == b + g)
        {
    	  format = PICT_FORMAT(bpp, PICT_TYPE_ARGB, 0, r, g, b);
    	  *nformats = addFormat (formats, *nformats, format, depth);
        }
        break;
      case StaticColor:
      case PseudoColor:
      case StaticGray:
      case GrayScale:
        break;
    }
  }

  for (d = 0; d < pScreen -> numDepths; d++)
  {
    pDepth = &pScreen -> allowedDepths[d];
    bpp = BitsPerPixel(pDepth -> depth);

    switch (bpp) {
    case 16:
      if (pDepth->depth == 15)
      {
        *nformats = addFormat (formats, *nformats,
    			      PICT_x1r5g5b5, pDepth->depth);
      }

      if (pDepth->depth == 16) 
      {
        *nformats = addFormat (formats, *nformats,
    	                      PICT_r5g6b5, pDepth->depth);
      }
      break;
    case 24:
      if (pDepth->depth == 24)
      {
        *nformats = addFormat (formats, *nformats,
    	                      PICT_r8g8b8, pDepth->depth);
      }
      break;
    case 32:
      if (pDepth->depth == 24)
      {
	*nformats = addFormat (formats, *nformats,
			      PICT_x8r8g8b8, pDepth->depth);
      }
      break;
    }
  }
}

