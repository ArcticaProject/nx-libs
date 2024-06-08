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

#include "picturestr.h"
#include "glyphstr.h"

#include "Render.h"

#include "X.h"
#include "Xproto.h"

#include "render.h"
#include "renderproto.h"

#include "mi.h"
#include "fb.h"
#include "mipict.h"
#include "fbpict.h"
#include "dixstruct.h"
#include "protocol-versions.h"

#include "Agent.h"
#include "Drawable.h"
#include "Trap.h"
#include "Args.h"
#include "Utils.h"

#define Atom   XlibAtom
#define Pixmap XlibPixmap
#include "X11/include/Xrenderint_nxagent.h"
#undef  Atom
#undef  Pixmap

#include "region.h"
#include <X11/extensions/extutil.h>

#include "Display.h"
#include "Pixmaps.h"
#include "Cursor.h"
#include "Client.h"
#include "Image.h"
#include "Pixels.h"
#include "Handlers.h"

#include <nx/NXproto.h>

#define MAX_FORMATS 255

#define NXAGENT_PICTURE_ALWAYS_POINTS_TO_VIRTUAL

/*
 * Define if you want split multiple glyph lists
 * into multiple RenderCompositeGlyphs requests.
 */

#undef  SPLIT_GLYPH_LISTS

/*
 * Set here the required log level.
 */

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

/*
FIXME: Most operations don't seem to produce any visible result
       but still generate tons of traffic.
*/
#undef  SKIP_LOUSY_RENDER_OPERATIONS
#undef  SKIP_REALLY_ALL_LOUSY_RENDER_OPERATIONS

/*
 * Margin added around the glyphs extent (in pixels).
 */

#define GLYPH_BOX_MARGIN 2

int nxagentRenderEnable = UNDEFINED;
int nxagentRenderVersionMajor;
int nxagentRenderVersionMinor;

int nxagentPicturePrivateIndex = 0;

static int nxagentNumFormats = 0;

static XRenderPictFormat nxagentArrayFormats[MAX_FORMATS];

XRenderPictFormat *nxagentMatchingFormats(PictFormatPtr pForm);

BoxPtr nxagentGlyphsExtents;
BoxPtr nxagentTrapezoidExtents;

static void nxagentPrintFormat(XRenderPictFormat *pFormat);

/*
 * From NXglyph.c.
 */

extern const CARD8 glyphDepths[];

/*
 * From BitmapUtils.c.
 */

extern void nxagentBitOrderInvert(unsigned char *data, int nbytes);

/*
 * Other functions defined here.
 */

void nxagentQueryFormats(void);

void nxagentCreateGlyphSet(GlyphSetPtr pGly);

int nxagentCursorSaveRenderInfo(ScreenPtr pScreen, CursorPtr pCursor);

void nxagentCursorPostSaveRenderInfo(CursorPtr pCursor, ScreenPtr pScreen,
                                         PicturePtr pPicture, int x, int y);

int nxagentCreatePicture(PicturePtr pPicture, Mask mask);

int nxagentChangePictureClip(PicturePtr pPicture, int clipType, int nRects,
                                 xRectangle *rects, int xOrigin, int yOrigin);

void nxagentComposite(CARD8 op, PicturePtr pSrc, PicturePtr pMask, PicturePtr pDst,
                          INT16 xSrc, INT16 ySrc, INT16 xMask, INT16 yMask, INT16 xDst,
                              INT16 yDst, CARD16 width, CARD16 height);

void nxagentGlyphs(CARD8 op, PicturePtr pSrc, PicturePtr pDst,
                       PictFormatPtr maskFormat, INT16 xSrc, INT16 ySrc, int nlists,
                           XGlyphElt8 *elts, int sizeID, GlyphPtr *glyphsBase);

void nxagentCompositeRects(CARD8 op, PicturePtr pDst, xRenderColor *color,
                               int nRect, xRectangle *rects);

void nxagentTrapezoids(CARD8 op, PicturePtr pSrc, PicturePtr pDst,
                           PictFormatPtr maskFormat, INT16 xSrc, INT16 ySrc,
                               int ntrap, xTrapezoid *traps);

void nxagentChangePicture(PicturePtr pPicture, Mask mask);

void nxagentReferenceGlyphSet(GlyphSetPtr glyphSet);

void nxagentFreeGlyphs(GlyphSetPtr glyphSet, CARD32 *gids, int nglyph);

void nxagentFreeGlyphSet(GlyphSetPtr glyphSet);

void nxagentSetPictureTransform(PicturePtr pPicture, void * transform);

void nxagentSetPictureFilter(PicturePtr pPicture, char *filter, int name_size,
                                 void * params, int nparams);

Bool nxagentReconnectAllGlyphSet(void *p);

Bool nxagentReconnectAllPicture(void *);

Bool nxagentDisconnectAllPicture(void);

#ifdef NXAGENT_RENDER_CLEANUP

#include <stdio.h>

#define ROUNDUP(nbits, pad) ((((nbits) + ((pad)-1)) / (pad)) * ((pad)>>3))

/* Clean the padding bytes of data section of request.*/
void
nxagentCleanGlyphs(xGlyphInfo  *gi,
                   int         nglyphs,
                   CARD8       *images,
                   int         depth,
                   Display     *dpy)
{
  int widthInBits;
  int bytesPerLine;
  int bytesToClean;
  int bitsToClean;
  int widthInBytes;
  int height = gi -> height;

  #ifdef DEBUG
  fprintf(stderr, "%s: Found a Glyph with Depth %d, width %d, pad %d.\n", __func__,
          depth, gi -> width, BitmapPad(dpy));
  #endif

  while (nglyphs > 0)
  {
    if (depth == 24)
    {
      widthInBits = gi -> width * 32;

      bytesPerLine = ROUNDUP(widthInBits, BitmapPad(dpy));

      bytesToClean = bytesPerLine * height;

      #ifdef DUBUG
      fprintf(stderr, "%s: Found glyph with depth 24, bytes to clean is %d"
                  "width in bits is %d bytes per line [%d] height [%d].\n", __func__,
	              bytesToClean, widthInBits, bytesPerLine, height);
      #endif

      if (ImageByteOrder(dpy) == LSBFirst)
      {
        for (int i = 3; i < bytesToClean; i += 4)
        {
          images[i] = 0x00;
        }
      }
      else
      {
        for (int i = 0; i < bytesToClean; i += 4)
        {
          images[i] = 0x00;
        }
      }

      #ifdef DUMP
      fprintf(stderr, "%s: depth %d, bytesToClean %d, scanline: ", __func__, depth, bytesToClean);
      for (int i = 0; i < bytesPerLine; i++)
      {
        fprintf(stderr, "[%d]", images[i]);
      }
      fprintf(stderr,"\n");
      #endif

      images += bytesToClean;

      gi++;

      nglyphs--;
    }
    else if (depth == 1)
    {
      widthInBits = gi -> width;

      bytesPerLine = ROUNDUP(widthInBits, BitmapPad(dpy));

      bitsToClean = (bytesPerLine << 3) - (gi -> width);

      #ifdef DEBUG
      fprintf(stderr, "%s: Found glyph with depth 1, width [%d], height [%d], bitsToClean [%d],"
              " bytesPerLine [%d].\n", __func__, gi -> width, height, bitsToClean, bytesPerLine);
      #endif

      bytesToClean = bitsToClean >> 3;

      bitsToClean &= 7;

      #ifdef DEBUG
      fprintf(stderr, "%s: bitsToClean &=7 is %d, bytesToCLean is %d."
              " byte_order is %d, bitmap_bit_order is %d.\n", __func__, bitsToClean, bytesToClean,
              ImageByteOrder(dpy), BitmapBitOrder(dpy));
      #endif

      for (int i = 1; i <= height; i++)
      {
        int j;
        if (ImageByteOrder(dpy) == BitmapBitOrder(dpy))
        {
          for (j = 1; j <= bytesToClean; j++)
          {
            images[i * bytesPerLine - j] = 0x00;

            #ifdef DEBUG
            fprintf(stderr, "%s: byte_order == bitmap_bit_order, cleaning %d, i=%d, j=%d.\n", __func__,
                    (i * bytesPerLine - j), i, j);
            #endif

          }
        }
        else
        {
          for (j = bytesToClean; j >= 1; j--)
          {
            images[i * bytesPerLine - j] = 0x00;

            #ifdef DEBUG
            fprintf(stderr, "%s: byte_order %d, bitmap_bit_order %d, cleaning %d, i=%d, j=%d.\n", __func__,
                    ImageByteOrder(dpy), BitmapBitOrder(dpy), (i * bytesPerLine - j), i, j);
            #endif

          }
        }

        if (BitmapBitOrder(dpy) == MSBFirst)
        {
          images[i * bytesPerLine - j] &= 0xff << bitsToClean;

          #ifdef DEBUG
          fprintf(stderr, "%s: byte_order MSBFirst, cleaning %d, i=%d, j=%d.\n", __func__,
                  (i * bytesPerLine - j), i, j);
          #endif
        }
        else
        {
          images[i * bytesPerLine - j] &= 0xff >> bitsToClean;

          #ifdef DEBUG
          fprintf(stderr, "%s: byte_order LSBFirst, cleaning %d, i=%d, j=%d.\n", __func__,
                  (i * bytesPerLine - j), i, j);
          #endif
        }
      }

      #ifdef DUMP
      fprintf(stderr, "%s: depth %d, bytesToClean %d, scanline: ", __func__, depth, bytesToClean);
      for (int i = 0; i < bytesPerLine; i++)
      {
        fprintf(stderr, "[%d]", images[i]);
      }
      fprintf(stderr,"\n");
      #endif

      images += bytesPerLine * height;

      gi++;

      nglyphs--;
    }
    else if ((depth == 8) || (depth == 16) )
    {
      widthInBits = gi -> width * depth;

      bytesPerLine = ROUNDUP(widthInBits, BitmapPad(dpy));

      widthInBytes = (widthInBits >> 3);

      bytesToClean = bytesPerLine - widthInBytes;

      #ifdef DEBUG
      fprintf(stderr, "%s: nglyphs is %d, width of glyph in bits is %d, in bytes is %d.\n", __func__,
              nglyphs, widthInBits, widthInBytes);

      fprintf(stderr, "%s: bytesPerLine is %d bytes, there are %d scanlines.\n", __func__, bytesPerLine, height);

      fprintf(stderr, "%s: Bytes to clean for each scanline are %d.\n", __func__, bytesToClean);
      #endif

      if (bytesToClean > 0)
      {
        for (; height > 0; height--)
        {
          for (int i = bytesToClean; i > 0; i--)
          {
            *(images + (bytesPerLine - i)) = 0;

            #ifdef DEBUG
            fprintf(stderr, "%s: cleaned a byte.\n", __func__);
            #endif
          }

          #ifdef DUMP
          fprintf(stderr, "%s: depth %d, bytesToClean %d, scanline: ", __func__, depth, bytesToClean);
          for (int i = 0; i < bytesPerLine; i++)
          {
            fprintf(stderr, "[%d]", images[i]);
          }
          fprintf(stderr,"\n");
          #endif

          images += bytesPerLine;
        }
      }

      gi++;

      nglyphs--;

      #ifdef DEBUG
      fprintf(stderr, "%s: Breaking Out.\n", __func__);
      #endif
    }
    else if (depth == 32)
    {
      #ifdef DEBUG
      fprintf(stderr, "%s: Found glyph with depth 32.\n", __func__);
      #endif

      gi++;

      nglyphs--;
    }
    else
    {
      #ifdef WARNING
      fprintf(stderr, "%s: Unrecognized glyph, depth is not 8/16/24/32, it appears to be %d.\n", __func__,
              depth);
      #endif

      gi++;

      nglyphs--;
    }
  }
}

#endif /* #ifdef NXAGENT_RENDER_CLEANUP */

void nxagentRenderExtensionInit(void)
{
  int first_event, first_error;
  int major_version, minor_version;

  if (XRenderQueryExtension(nxagentDisplay, &first_event, &first_error))
  {
    XRenderQueryVersion(nxagentDisplay, &major_version, &minor_version);

    /*
     * As the RENDER requests are passed directly to the remote X
     * server this can cause problems if our RENDER version is
     * different from the version supported by the remote. For this
     * reasons let's advertise to our clients the lowest between the
     * two versions.
     */

    if (major_version > SERVER_RENDER_MAJOR_VERSION ||
            (major_version == SERVER_RENDER_MAJOR_VERSION &&
                 minor_version > SERVER_RENDER_MINOR_VERSION))
    {
      #ifdef TEST
      fprintf(stderr, "%s: Using render version [%d.%d] with "
                  "remote version [%d.%d].\n", __func__, SERVER_RENDER_MAJOR_VERSION, SERVER_RENDER_MINOR_VERSION,
                      major_version, minor_version);
      #endif

      nxagentRenderVersionMajor = SERVER_RENDER_MAJOR_VERSION;
      nxagentRenderVersionMinor = SERVER_RENDER_MINOR_VERSION;
    }
    else if (major_version < SERVER_RENDER_MAJOR_VERSION ||
                 (major_version == SERVER_RENDER_MAJOR_VERSION &&
                      minor_version < SERVER_RENDER_MINOR_VERSION))
    {
      #ifdef TEST
      fprintf(stderr, "Info: Local render version %d.%d is higher "
                  "than remote version %d.%d.\n", SERVER_RENDER_MAJOR_VERSION, SERVER_RENDER_MINOR_VERSION,
                      major_version, minor_version);

      fprintf(stderr, "Info: Lowering the render version reported to clients.\n");
      #endif

      nxagentRenderVersionMajor = major_version;
      nxagentRenderVersionMinor = minor_version;
    }
    else
    {
      #ifdef TEST
      fprintf(stderr, "%s: Local render version %d.%d "
                  "matches remote version %d.%d.\n", __func__, SERVER_RENDER_MAJOR_VERSION, SERVER_RENDER_MINOR_VERSION,
                      major_version, minor_version);
      #endif

      nxagentRenderVersionMajor = major_version;
      nxagentRenderVersionMinor = minor_version;
    }
  }
  else
  {
    #ifdef WARNING
    fprintf(stderr, "Warning: Render not available on the remote display.\n");
    #endif

    nxagentRenderEnable = False;
  }
}

int nxagentCursorSaveRenderInfo(ScreenPtr pScreen, CursorPtr pCursor)
{
  pCursor -> devPriv[pScreen -> myNum] = malloc(sizeof(nxagentPrivCursor));

  if (nxagentCursorPriv(pCursor, pScreen) == NULL)
  {
    FatalError("malloc failed");
  }

  nxagentCursorUsesRender(pCursor, pScreen) = 1;
  nxagentCursorPicture(pCursor, pScreen) = NULL;

  return 1;
}

void nxagentCursorPostSaveRenderInfo(CursorPtr pCursor, ScreenPtr pScreen,
                                         PicturePtr pPicture, int x, int y)
{
  nxagentCursorPicture(pCursor, pScreen) = pPicture;
  nxagentCursorXOffset(pCursor, pScreen) = x;
  nxagentCursorYOffset(pCursor, pScreen) = y;
}

void nxagentRenderRealizeCursor(ScreenPtr pScreen, CursorPtr pCursor)
{
  PicturePtr pPicture = nxagentCursorPicture(pCursor, pScreen);

  pPicture -> refcnt++;

  int x = nxagentCursorXOffset(pCursor, pScreen);
  int y = nxagentCursorYOffset(pCursor, pScreen);

  #ifdef TEST
  fprintf(stderr, "%s: Forcing the synchronization of the cursor.\n", __func__);
  #endif

  nxagentMarkCorruptedRegion(pPicture -> pDrawable, NULL);

  /*
   * Set the lossless trap so that the image functions will not try to
   * encode the image using a lossy compression. Drawables should have
   * a quality flag, telling if they were originally encoded with a
   * lossy algorithm. This would allow us to skip the synchronization
   * if the cursor was already encoded with the best quality.
   */

  nxagentLosslessTrap = True;

  nxagentSynchronizeDrawable(pPicture -> pDrawable, DO_WAIT, NEVER_BREAK, NULL);

  nxagentLosslessTrap = False;
  nxagentCursor(pCursor, pScreen) = XRenderCreateCursor(nxagentDisplay, nxagentPicture(pPicture), x, y);
}

/* little helper for better readable code below */
#define COPYPICTVAL(_mask, _member, _src) {                                \
    if (mask & _mask)                                                      \
    {                                                                      \
      attributes._member = (_src);                                         \
      valuemask |= (_mask);                                                \
      nxagentSetPictureRemoteValue(pPicture, _member, attributes._member); \
    }                                                                      \
  }

int nxagentCreatePicture(PicturePtr pPicture, Mask mask)
{
  XRenderPictureAttributes attributes;
  unsigned long            valuemask=0;

  #ifdef DEBUG
  fprintf(stderr, "%s: Function called with picture at [%p] and mask [%ld].\n", __func__,
              (void *) pPicture, mask);
  #endif

  if (pPicture == NULL)
  {
    return 0;
  }

  #ifdef DEBUG

  if (pPicture -> pDrawable -> type == DRAWABLE_PIXMAP)
  {
     if (nxagentIsShmPixmap((PixmapPtr)pPicture -> pDrawable))
     {
       fprintf (stderr, "%s: Picture uses a shared pixmap.\n", __func__);
     }
     else
     {
       fprintf (stderr, "%s: Picture uses a plain pixmap.\n", __func__);
     }
  }
  else
  {
    fprintf (stderr, "%s: Picture uses a window.\n", __func__);
  }

  #endif

  /*
   * All the picture default values are 0.
   */

  memset(&(nxagentPicturePriv(pPicture) -> lastServerValues), 0, sizeof(XRenderPictureAttributes));

  COPYPICTVAL(CPRepeat,           repeat,             (Bool)pPicture->repeat);
  COPYPICTVAL(CPAlphaMap,         alpha_map,          nxagentPicturePriv(pPicture->alphaMap)->picture);
  COPYPICTVAL(CPAlphaXOrigin,     alpha_x_origin,     pPicture->alphaOrigin.x);
  COPYPICTVAL(CPAlphaYOrigin,     alpha_y_origin,     pPicture->alphaOrigin.y);
  COPYPICTVAL(CPClipXOrigin,      clip_x_origin,      pPicture->clipOrigin.x);
  COPYPICTVAL(CPClipYOrigin,      clip_y_origin,      pPicture->clipOrigin.y);
  COPYPICTVAL(CPGraphicsExposure, graphics_exposures, (Bool)pPicture->graphicsExposures);
  COPYPICTVAL(CPSubwindowMode,    subwindow_mode,     pPicture->subWindowMode);
  COPYPICTVAL(CPClipMask,         clip_mask,          None);
  COPYPICTVAL(CPPolyEdge,         poly_edge,          pPicture->polyEdge);
  COPYPICTVAL(CPPolyMode,         poly_mode,          pPicture->polyMode);
  COPYPICTVAL(CPDither,           dither,             pPicture->dither);
  COPYPICTVAL(CPComponentAlpha,   component_alpha,    pPicture->componentAlpha);

  XRenderPictFormat *pForm = NULL;

  #ifdef DEBUG
  fprintf(stderr, "%s: picture format [0x%lx]\n", __func__, pPicture->pFormat);
  #endif

  if (pPicture -> pFormat != NULL)
  {
    pForm = nxagentMatchingFormats(pPicture -> pFormat);
    nxagentPrintFormat(pForm);
  }

  if (pForm == NULL)
  {
    fprintf(stderr, "%s: WARNING! The requested format was not found.\n", __func__);
    return 0;
  }

  Picture id = XRenderCreatePicture(nxagentDisplay,
                                    nxagentDrawable(pPicture -> pDrawable),
                                    pForm,
                                    valuemask,
                                    &attributes);

  #ifdef TEST
  fprintf(stderr, "%s: Created picture at [%p] with drawable at [%p].\n", __func__,
              (void *) pPicture, (void *) pPicture -> pDrawable);
  #endif

  #ifdef DEBUG
  XSync(nxagentDisplay, 0);
  #endif

  nxagentPicturePriv(pPicture) -> picture = id;

  if (nxagentAlphaEnabled && pPicture -> pDrawable -> depth == 32 &&
          pPicture -> pFormat -> direct.alpha != 0)
  {
    if (pPicture -> pDrawable -> type == DRAWABLE_PIXMAP)
    {
      nxagentPixmapPriv(nxagentRealPixmap((PixmapPtr) pPicture -> pDrawable)) -> pPicture = pPicture;
    }
    else if (pPicture -> pDrawable -> type == DRAWABLE_WINDOW)
    {
      nxagentWindowPriv((WindowPtr) pPicture -> pDrawable) -> pPicture = pPicture;
    }
  }

  return 1;
}

XRenderPictFormat *nxagentMatchingFormats(PictFormatPtr pFormat)
{
  for (int i = 0; i < nxagentNumFormats; i++)
  {
    if (pFormat -> type == nxagentArrayFormats[i].type &&
        pFormat -> depth == nxagentArrayFormats[i].depth &&
        pFormat -> direct.red == nxagentArrayFormats[i].direct.red &&
        pFormat -> direct.green == nxagentArrayFormats[i].direct.green &&
        pFormat -> direct.blue == nxagentArrayFormats[i].direct.blue &&
        pFormat -> direct.redMask == nxagentArrayFormats[i].direct.redMask &&
        pFormat -> direct.greenMask == nxagentArrayFormats[i].direct.greenMask &&
        pFormat -> direct.blueMask == nxagentArrayFormats[i].direct.blueMask &&
        pFormat -> direct.alpha == nxagentArrayFormats[i].direct.alpha &&
        pFormat -> direct.alphaMask == nxagentArrayFormats[i].direct.alphaMask)
    {
      return &nxagentArrayFormats[i];
    }
  }

  #ifdef DEBUG
  fprintf(stderr, "%s: The requested format was not found.\n", __func__);
  #endif

  return NULL;
}

void nxagentDestroyPicture(PicturePtr pPicture)
{
  if (pPicture == NULL || nxagentPicturePriv(pPicture) -> picture == 0)
  {
    return;
  }

  #ifdef TEST
  fprintf(stderr, "%s: Going to destroy picture at [%p].\n", __func__,
              (void *) pPicture);
  #endif

  XRenderFreePicture(nxagentDisplay,
                     nxagentPicturePriv(pPicture) -> picture);

  #ifdef DEBUG
  XSync(nxagentDisplay, 0);
  #endif
}

int nxagentChangePictureClip(PicturePtr pPicture, int clipType, int nRects,
                                 xRectangle *rects, int xOrigin, int yOrigin)
{
  #ifdef TEST
  fprintf(stderr, "%s: Going to change clip of picture at [%p].\n", __func__,
              (void *) pPicture);
  #endif

  #ifdef DEBUG
  fprintf(stderr, "%s: clipType [%d] nRects [%d] xRectangle [%p] "
              "xOrigin [%d] yOrigin [%d].\n", __func__, clipType, nRects, (void *) rects, xOrigin, yOrigin);
  #endif

  if (pPicture == NULL)
  {
    return 0;
  }

  switch (clipType)
  {
    case CT_PIXMAP:
    {
      #ifdef DEBUG
      fprintf(stderr, "%s: Clip type is [CT_PIXMAP].\n", __func__);
      #endif

      /*
       * if(!nRects)
       * {
       *   return 0;
       * }
       */
/*
FIXME: Is this useful or just a waste of bandwidth?

       Apparently useless with QT.
*/
      #ifndef SKIP_LOUSY_RENDER_OPERATIONS
      XRenderSetPictureClipRectangles(nxagentDisplay,
                                      nxagentPicturePriv(pPicture) -> picture,
                                      xOrigin,
                                      yOrigin,
                                      (XRectangle*)rects,
                                      nRects);

      nxagentSetPictureRemoteValue(pPicture, clip_x_origin, xOrigin);
      nxagentSetPictureRemoteValue(pPicture, clip_y_origin, yOrigin);
      nxagentSetPictureRemoteValue(pPicture, clip_mask, 1);
      #endif

      #ifdef DEBUG
      XSync(nxagentDisplay, 0);
      #endif

      break;
    }
    case CT_NONE:
    {
      #ifdef DEBUG
      fprintf(stderr, "%s: Clip type is [CT_NONE].\n", __func__);
      #endif
/*
FIXME: Is this useful or just a waste of bandwidth?

       Apparently useless with QT.
*/
      #ifndef SKIP_LOUSY_RENDER_OPERATIONS
      XRenderSetPictureClipRectangles(nxagentDisplay,
                                      nxagentPicturePriv(pPicture) -> picture,
                                      xOrigin,
                                      yOrigin,
                                      (XRectangle*)rects,
                                      nRects);

      nxagentSetPictureRemoteValue(pPicture, clip_x_origin, xOrigin);
      nxagentSetPictureRemoteValue(pPicture, clip_y_origin, yOrigin);
      nxagentSetPictureRemoteValue(pPicture, clip_mask, 1);
      #endif

      #ifdef DEBUG
      XSync(nxagentDisplay, 0);
      #endif

      break;
    }
    case CT_REGION:
    {
      Region     reg;
      XRectangle rectangle;
      int        index;

      #ifdef DEBUG
      fprintf(stderr, "%s: Clip type is [CT_REGION].\n", __func__);
      #endif

      reg = XCreateRegion();

      for (index = 0; index <= nRects; index++, rects++)
      {
        rectangle.x = rects -> x;
        rectangle.y = rects -> y;
        rectangle.width = rects -> width;
        rectangle.height = rects -> height;

        XUnionRectWithRegion(&rectangle, reg, reg);
      }
/*
FIXME: Is this useful or just a waste of bandwidth?

       Apparently useless with QT.
*/
      #ifndef SKIP_LOUSY_RENDER_OPERATIONS
      XRenderSetPictureClipRegion(nxagentDisplay,
                                  nxagentPicturePriv(pPicture) -> picture,
                                  reg);

      nxagentSetPictureRemoteValue(pPicture, clip_x_origin, xOrigin);
      nxagentSetPictureRemoteValue(pPicture, clip_y_origin, yOrigin);
      nxagentSetPictureRemoteValue(pPicture, clip_mask, 1);
      #endif

      #ifdef DEBUG
      XSync(nxagentDisplay, 0);
      #endif

      XDestroyRegion(reg);

      break;
    }
    default:
    {
      #ifdef DEBUG
      fprintf(stderr, "%s: clipType not found\n", __func__);
      #endif

      break;
    }
  }

  return 1;
}

/* little helper for better readable code below */
#define CHGPICTVAL(_mask, _member, _src) {                                        \
    if (mask & _mask)                                                             \
    {                                                                             \
      attributes._member = (_src);                                                \
      if (!nxagentCheckPictureRemoteValue(pPicture, _member, attributes._member)) \
      {                                                                           \
        valuemask |= _mask;                                                       \
        nxagentSetPictureRemoteValue(pPicture, _member, attributes._member);      \
      }                                                                           \
    }                                                                             \
  }

void nxagentChangePicture(PicturePtr pPicture, Mask mask)
{
  XRenderPictureAttributes  attributes;
  unsigned long             valuemask = 0;

  #ifdef DEBUG
  fprintf(stderr, "%s: Going to change picture at [%p] with mask [%ld].\n", __func__,
              (void *) pPicture, mask);
  #endif

  if (pPicture == NULL)
  {
    return;
  }

  CHGPICTVAL(CPRepeat,           repeat,             (Bool)pPicture->repeat);
  CHGPICTVAL(CPAlphaMap,         alpha_map,          nxagentPicturePriv(pPicture->alphaMap)->picture);
  CHGPICTVAL(CPAlphaXOrigin,     alpha_x_origin,     pPicture->alphaOrigin.x);
  CHGPICTVAL(CPAlphaYOrigin,     alpha_y_origin,     pPicture->alphaOrigin.y);
  CHGPICTVAL(CPClipXOrigin,      clip_x_origin,      pPicture->clipOrigin.x);
  CHGPICTVAL(CPClipYOrigin,      clip_y_origin,      pPicture->clipOrigin.y);
  CHGPICTVAL(CPGraphicsExposure, graphics_exposures, (Bool)pPicture->graphicsExposures);
  CHGPICTVAL(CPSubwindowMode,    subwindow_mode,     pPicture->subWindowMode);
  if (mask & CPClipMask)
  {
    /*
     * The nxagent doesn't know the remote id of the picture's clip
     * mask, so the clip_mask value is used as a boolean: it is set to
     * 0 when the clip_mask is None, otherwise it is 1.
     */

    attributes.clip_mask = None;
    if (nxagentPicturePriv(pPicture) -> lastServerValues.clip_mask != 0)
    {
      valuemask |= CPClipMask;
      nxagentSetPictureRemoteValue(pPicture, clip_mask, 0);
    }
  }
  CHGPICTVAL(CPPolyEdge,         poly_edge,          pPicture->polyEdge);
  CHGPICTVAL(CPPolyMode,         poly_mode,          pPicture->polyMode);
  CHGPICTVAL(CPDither,           dither,             pPicture->dither);
  CHGPICTVAL(CPComponentAlpha,   component_alpha,    pPicture->componentAlpha);

  #ifdef TEST
  if (pPicture && pPicture->pDrawable && pPicture -> pDrawable -> type == DRAWABLE_PIXMAP)
  {
    fprintf(stderr, "%s: %sPixmap [%p] Picture [%p][%p].\n", __func__,
                nxagentIsShmPixmap((PixmapPtr)pPicture -> pDrawable) ? "Shared " : "",
                    (void *) pPicture -> pDrawable, (void *) nxagentPicturePriv(pPicture) -> picture,
                        (void *) pPicture);
  }
  #endif
/*
FIXME: Is this useful or just a waste of bandwidth?

       Apparently useless with QT.

       Without this the text is not rendered on GTK/Cairo.
*/
  #ifndef SKIP_REALLY_ALL_LOUSY_RENDER_OPERATIONS
  if (valuemask != 0)
  {
    XRenderChangePicture(nxagentDisplay,
                         nxagentPicturePriv(pPicture) -> picture,
                         valuemask,
                         &attributes);
  }
  #ifdef TEST
  else
  {
    fprintf(stderr, "%s: Skipping change of picture [%p] on remote X server.\n", __func__,
                (void *) pPicture);
  }
  #endif
  #endif /* SKIP_REALLY_ALL_LOUSY_RENDER_OPERATIONS */

  #ifdef DEBUG
  XSync(nxagentDisplay, 0);
  #endif
}

void nxagentComposite(CARD8 op, PicturePtr pSrc, PicturePtr pMask, PicturePtr pDst,
                          INT16 xSrc, INT16 ySrc, INT16 xMask, INT16 yMask, INT16 xDst,
                              INT16 yDst, CARD16 width, CARD16 height)
{
  #ifdef DEBUG
  fprintf(stderr, "%s: entering...\n", __func__);
  #endif
  if (pSrc == NULL || pDst == NULL)
  {
    return;
  }

  #ifdef DEBUG
  if (pSrc && pSrc -> pDrawable != NULL)
  {
    fprintf(stderr, "%s: Source Picture [%lu][%p] with drawable [%s%s][%p].\n", __func__,
                nxagentPicturePriv(pSrc) -> picture, (void *) pSrc,
                (pSrc -> pDrawable -> type == DRAWABLE_PIXMAP &&
                     nxagentIsShmPixmap((PixmapPtr) pSrc -> pDrawable)) ? "Shared " : "",
                         pSrc -> pDrawable -> type == DRAWABLE_PIXMAP ? "Pixmap" : "Window",
                             (void *) pSrc -> pDrawable);
  }

  if (pDst && pDst->pDrawable) {
    fprintf(stderr, "%s: Destination Picture [%lu][%p] with drawable [%s%s][%p].\n", __func__,
                nxagentPicturePriv(pDst) -> picture, (void *) pDst,
                (pDst -> pDrawable -> type == DRAWABLE_PIXMAP &&
                    nxagentIsShmPixmap((PixmapPtr) pDst -> pDrawable)) ? "Shared " : "",
                         pDst -> pDrawable -> type == DRAWABLE_PIXMAP ? "Pixmap" : "Window",
                             (void *) pDst -> pDrawable);
  }

  if (pMask && pMask->pDrawable)
  {
    fprintf(stderr, "%s: Mask Picture [%lu][%p] with drawable [%s%s][%p].\n", __func__,
                nxagentPicturePriv(pMask) -> picture, (void *) pMask,
                (pMask -> pDrawable -> type == DRAWABLE_PIXMAP &&
                    nxagentIsShmPixmap((PixmapPtr) pMask -> pDrawable)) ? "Shared " : "",
                       pMask -> pDrawable -> type == DRAWABLE_PIXMAP ? "Pixmap" : "Window",
                           (void *) pMask -> pDrawable);
  }
  #endif

  if (NXAGENT_SHOULD_DEFER_COMPOSITE(pSrc, pMask, pDst))
  {
    RegionPtr pDstRegion = nxagentCreateRegion(pDst -> pDrawable, NULL, xDst, yDst, width, height);

    #ifdef TEST
    if ((pDstRegion) && (pDst && pDst->pDrawable)) {
      fprintf(stderr, "%s: WARNING! Prevented operation on region [%d,%d,%d,%d] "
                  "for drawable at [%p] with type [%s].\n", __func__, pDstRegion -> extents.x1,
                      pDstRegion -> extents.y1, pDstRegion -> extents.x2, pDstRegion -> extents.y2,
                          (void *) pDst -> pDrawable,
                              pDst -> pDrawable -> type == DRAWABLE_PIXMAP ? "pixmap" : "window");
    }
    #endif

    nxagentMarkCorruptedRegion(pDst -> pDrawable, pDstRegion);

    nxagentFreeRegion(pDstRegion);

    return;
  }

  /*
   * Synchronize the content of the shared memory pixmap but pay
   * attention at not doing this more than once.  We need to wait
   * until the image data has been recomposed at the X server side
   * or the operation will use the wrong data.
   */

  if (pSrc -> pDrawable != NULL)
  {
    nxagentSynchronizeShmPixmap(pSrc -> pDrawable, xSrc, ySrc, width, height);

    if (nxagentDrawableStatus(pSrc -> pDrawable) == NotSynchronized)
    {
      #ifdef TEST
      fprintf(stderr, "%s: Synchronizing the source drawable [%p].\n", __func__,
                  (void *) pSrc -> pDrawable);
      #endif

      nxagentSynchronizeDrawable(pSrc -> pDrawable, DO_WAIT, NEVER_BREAK, NULL);
    }
  }

  if (pDst -> pDrawable != pSrc -> pDrawable)
  {
    nxagentSynchronizeShmPixmap(pDst -> pDrawable, xDst, yDst, width, height);

    if (nxagentDrawableStatus(pDst -> pDrawable) == NotSynchronized)
    {
      #ifdef TEST
      fprintf(stderr, "%s: Synchronizing the destination drawable [%p].\n", __func__,
                  (void *) pDst -> pDrawable);
      #endif

      nxagentSynchronizeDrawable(pDst -> pDrawable, DO_WAIT, NEVER_BREAK, NULL);
    }
  }

  if (pMask != NULL && pMask -> pDrawable != NULL &&
          pMask -> pDrawable != pSrc -> pDrawable &&
              pMask -> pDrawable != pDst -> pDrawable)
  {
    nxagentSynchronizeShmPixmap(pMask -> pDrawable, xMask, yMask, width, height);

    if (nxagentDrawableStatus(pMask -> pDrawable) == NotSynchronized)
    {
      #ifdef TEST
      fprintf(stderr, "%s: Synchronizing the mask drawable [%p].\n", __func__,
                  (void *) pMask -> pDrawable);
      #endif

      nxagentSynchronizeDrawable(pMask -> pDrawable, DO_WAIT, NEVER_BREAK, NULL);
    }
  }

  /*
   * The glyphs flag have to be propagated between drawables, in order
   * to avoid to encode the text with lossy algorithms (like
   * JPEG). Unluckily we have verified that if the render composite
   * propagates the flag, the deferring of render trapezoids doesn't
   * work well. Moreover, by commenting out this code we have not
   * noticed any visual problems.
   *
   *  if (nxagentDrawableContainGlyphs(pSrc -> pDrawable) == 1)
   *  {
   *    nxagentSetDrawableContainGlyphs(pDst -> pDrawable, 1);
   *  }
   */

  XRenderComposite(nxagentDisplay,
                   op,
                   nxagentPicturePriv(pSrc) -> picture,
                   pMask ? nxagentPicturePriv(pMask) -> picture : 0,
                   nxagentPicturePriv(pDst) -> picture,
                   xSrc,
                   ySrc,
                   xMask,
                   yMask,
                   xDst,
                   yDst,
                   width,
                   height);

  #ifdef DEBUG
  XSync(nxagentDisplay, 0);
  #endif
}

void nxagentGlyphs(CARD8 op, PicturePtr pSrc, PicturePtr pDst,
                       PictFormatPtr maskFormat, INT16 xSrc, INT16 ySrc, int nlists,
                           XGlyphElt8 *elts, int sizeID, GlyphPtr *glyphsBase)
{
  BoxRec glyphBox = {0};

  XGlyphElt8 *elements;

  #ifdef DEBUG
  fprintf(stderr, "%s: pSrc [%p] pSrc->pDrawable [%p] pDst [%p] pDst->pDrawable [%p]\n",
	      __func__, pSrc, pSrc ? pSrc->pDrawable : NULL, pDst, pDst ? pDst->pDrawable : NULL);
  #endif

  if (pSrc == NULL || pDst == NULL)
  {
    return;
  }

  #ifdef TEST
  if ((pSrc && pSrc->pDrawable) && (pDst && pDst->pDrawable)) {
      fprintf(stderr, "%s: Called with source [%s][%p] destination [%s][%p] and size id [%d].\n", __func__,
                  (pSrc -> pDrawable -> type == DRAWABLE_PIXMAP ? "pixmap" : "window"), (void *) pSrc,
                      (pDst -> pDrawable -> type == DRAWABLE_PIXMAP ? "pixmap" : "window"), (void *) pDst,
                          sizeID);
  }
  #endif

  XRenderPictFormat *pForm = NULL;

  if (maskFormat != NULL)
  {
    pForm = nxagentMatchingFormats(maskFormat);
    nxagentPrintFormat(pForm);

    if (pForm == NULL)
    {
      return;
    }
  }

  if (nxagentGlyphsExtents != NullBox)
  {
    glyphBox.x1 = nxagentGlyphsExtents -> x1;
    glyphBox.y1 = nxagentGlyphsExtents -> y1;
    glyphBox.x2 = nxagentGlyphsExtents -> x2;
    glyphBox.y2 = nxagentGlyphsExtents -> y2;

    /*
     * By extending the glyph extents the visual aspect looks nicer
     * because the synchronized region is not glued to the fonts.
     */

    if (glyphBox.x2 != glyphBox.x1)
    {
      glyphBox.x1 -= GLYPH_BOX_MARGIN;
      glyphBox.x2 += GLYPH_BOX_MARGIN;
    }

    if (glyphBox.y2 != glyphBox.y1)
    {
      glyphBox.y1 -= GLYPH_BOX_MARGIN;
      glyphBox.y2 += GLYPH_BOX_MARGIN;
    }
  }

  /*
   * If the destination window is hidden, the operation can be
   * prevented.
   */

  if (pDst -> pDrawable -> type == DRAWABLE_WINDOW)
  {
    RegionPtr pRegion = nxagentCreateRegion(pDst -> pDrawable, NULL, glyphBox.x1, glyphBox.y1,
                                                glyphBox.x2 - glyphBox.x1, glyphBox.y2 - glyphBox.y1);

    if (RegionNil(pRegion))
    {
      #ifdef TEST
      fprintf(stderr, "%s: WARNING! Glyphs prevented on hidden window at [%p].\n", __func__,
                  (void *) pDst -> pDrawable);
      #endif

      nxagentFreeRegion(pRegion);

      return;
    }

    nxagentFreeRegion(pRegion);
  }

  /*
   * Need to synchronize the pixmaps involved in the operation before
   * rendering the glyphs on the real X server.
   */

  if (pSrc -> pDrawable != NULL &&
          nxagentDrawableStatus(pSrc -> pDrawable) == NotSynchronized)
  {
    #ifdef TEST
    if (pSrc && pSrc->pDrawable) {
      fprintf(stderr, "%s Synchronizing source [%s] at [%p].\n", __func__,
                  pSrc -> pDrawable -> type == DRAWABLE_PIXMAP ? "pixmap" : "window",
                      (void *) pSrc -> pDrawable);
    }
    #endif

    /*
     * If the source drawable is going to be repeated over the
     * destination drawable during the composite operation, we need to
     * synchronize the whole drawable to avoid graphical problems.
     */

    if (pSrc -> repeat == 1 || nxagentGlyphsExtents == NullBox)
    {
      #ifdef DEBUG
      if (pSrc && pSrc->pDrawable) {
        fprintf(stderr, "%s Synchronizing source [%s] at [%p] "
                    "with geometry [%d,%d,%d,%d].\n", __func__,
                        (pSrc -> pDrawable -> type == DRAWABLE_PIXMAP ? "pixmap" : "window"),
                            (void *) pSrc -> pDrawable, pSrc -> pDrawable -> x, pSrc -> pDrawable -> y,
                                pSrc -> pDrawable -> x + pSrc -> pDrawable -> width,
                                    pSrc -> pDrawable -> y + pSrc -> pDrawable -> height);
      }
      #endif

      nxagentSynchronizeBox(pSrc -> pDrawable, NullBox, NEVER_BREAK);
    }
    else
    {
      #ifdef DEBUG
      if (pSrc && pSrc->pDrawable) {
        fprintf(stderr, "%s Synchronizing region [%d,%d,%d,%d] of source [%s] at [%p] "
                    "with geometry [%d,%d,%d,%d].\n", __func__, glyphBox.x1, glyphBox.y1, glyphBox.x2, glyphBox.y2,
                            (pSrc -> pDrawable -> type == DRAWABLE_PIXMAP ? "pixmap" : "window"),
                                (void *) pSrc -> pDrawable, pSrc -> pDrawable -> x, pSrc -> pDrawable -> y,
                                    pSrc -> pDrawable -> x + pSrc -> pDrawable -> width,
                                        pSrc -> pDrawable -> y + pSrc -> pDrawable -> height);
      }
      #endif

      nxagentSynchronizeBox(pSrc -> pDrawable, &glyphBox, NEVER_BREAK);
    }

    if (pSrc -> pDrawable -> type == DRAWABLE_PIXMAP)
    {
      nxagentIncreasePixmapUsageCounter((PixmapPtr) pSrc -> pDrawable);
    }
  }

  if (pSrc -> pDrawable != pDst -> pDrawable &&
          nxagentDrawableStatus(pDst -> pDrawable) == NotSynchronized)
  {
    #ifdef TEST
    if (pDst && pDst->pDrawable) {
      fprintf(stderr, "%s Synchronizing destination [%s] at [%p].\n", __func__,
                  pDst -> pDrawable -> type == DRAWABLE_PIXMAP ? "pixmap" : "window",
                      (void *) pDst -> pDrawable);
    }
    #endif

    if (nxagentGlyphsExtents == NullBox)
    {
      #ifdef DEBUG
      if (pDst && pDst->pDrawable) {
        fprintf(stderr, "%s Synchronizing destination [%s] at [%p] "
                    "with geometry [%d,%d,%d,%d].\n", __func__,
                        (pDst -> pDrawable -> type == DRAWABLE_PIXMAP ? "pixmap" : "window"),
                            (void *) pDst -> pDrawable, pDst -> pDrawable -> x, pDst -> pDrawable -> y,
                                pDst -> pDrawable -> x + pDst -> pDrawable -> width,
                                    pDst -> pDrawable -> y + pDst -> pDrawable -> height);
      }
      #endif

      nxagentSynchronizeBox(pDst -> pDrawable, NullBox, NEVER_BREAK);
    }
    else
    {
      #ifdef DEBUG
      if (pDst && pDst->pDrawable) {
        fprintf(stderr, "%s Synchronizing region [%d,%d,%d,%d] of destination [%s] at [%p] "
                    "with geometry [%d,%d,%d,%d].\n", __func__, glyphBox.x1, glyphBox.y1, glyphBox.x2, glyphBox.y2,
                            (pDst -> pDrawable -> type == DRAWABLE_PIXMAP ? "pixmap" : "window"),
                                (void *) pDst -> pDrawable, pDst -> pDrawable -> x, pDst -> pDrawable -> y,
                                    pDst -> pDrawable -> x + pDst -> pDrawable -> width,
                                        pDst -> pDrawable -> y + pDst -> pDrawable -> height);
      }
      #endif

      nxagentSynchronizeBox(pDst -> pDrawable, &glyphBox, NEVER_BREAK);
    }

    if (pDst -> pDrawable -> type == DRAWABLE_PIXMAP)
    {
      nxagentIncreasePixmapUsageCounter((PixmapPtr) pDst -> pDrawable);
    }
  }

  nxagentSetDrawableContainGlyphs(pDst -> pDrawable, 1);

  #ifdef TEST
  if (pDst && pDst->pDrawable) {
    fprintf(stderr, "%s Glyph flag set on drawable [%s][%p].\n", __func__,
                pDst -> pDrawable -> type == DRAWABLE_PIXMAP ? "pixmap" : "window",
                    (void *) pDst -> pDrawable);
  }
  #endif

  #ifdef SPLIT_GLYPH_LISTS
  GlyphPtr glyph;

  /*
   * We split glyphs lists here and recalculate the offsets of each
   * list to make them absolute and not relatives to the prior list.
   * This way each time we call XRenderCompositeText it has to deal
   * only with a list of glyphs. This is done to further improve
   * caching.
   */

  elements = elts;

  if (nlists > 1)
  {
    int x;
    int y;

    for (int j = 1; j < nlists; j++)
    {
      x = elements -> xOff;
      y = elements -> yOff;

      #ifdef TEST
      fprintf(stderr, "%s Element [%d] of [%d] has offset [%d,%d].\n", __func__,
                  j, nlists, elements -> xOff, elements -> yOff);
      #endif

      for (int i = 0; i < elements -> nchars; i++)
      {
        glyph = *glyphsBase++;

        x += glyph -> info.xOff;
        y += glyph -> info.yOff;

        #ifdef TEST
        fprintf(stderr, "%s Glyph at index [%d] has offset [%d,%d] and "
                    "position [%d,%d].\n", __func__, i, elements -> nchars, glyph -> info.xOff,
                        glyph -> info.yOff, x, y);
        #endif
      }

      elements++;

      elements -> xOff += x;
      elements -> yOff += y;

      #ifdef TEST
      fprintf(stderr, "%s New offset for list at [%p] is [%d,%d].\n", __func__,
                  elements, elements -> xOff, elements -> yOff);
      #endif
    }

    elements = elts;
  }

  switch (sizeID)
  {
    case 1:
    {
      for (int j = 0; j < nlists; j++)
      {
        #ifdef DEBUG
        fprintf(stderr, "%s: sizeID is [%d] - [%s]\n", __func__, sizeID, "XRenderCompositeText8");
        #endif

        XRenderCompositeText8(nxagentDisplay,
                              op,
                              nxagentPicturePriv(pSrc)->picture,
                              nxagentPicturePriv(pDst)->picture,
                              pForm,
                              xSrc,
                              ySrc,
                              elements -> xOff,
                              elements -> yOff,
                              (XGlyphElt8*) elements,
                              1);

        elements++;
      }
      break;
    }
    case 2:
    {
      for (int j = 0; j < nlists; j++)
      {
        #ifdef DEBUG
        fprintf(stderr, "%s: sizeID is [%d] - [%s]\n", __func__, sizeID, "XRenderCompositeText16");
        #endif

        XRenderCompositeText16(nxagentDisplay,
                               op,
                               nxagentPicturePriv(pSrc) -> picture,
                               nxagentPicturePriv(pDst) -> picture,
                               pForm,
                               xSrc,
                               ySrc,
                               elements -> xOff,
                               elements -> yOff,
                               (XGlyphElt16*) elements,
                               1);

        elements++;
      }
      break;
    }
    case 4:
    {
      for (int j = 0; j < nlists; j++)
      {
        #ifdef DEBUG
        fprintf(stderr, "%s: sizeID is [%d] - [%s]\n", __func__, sizeID, "XRenderCompositeText32");
        #endif

        XRenderCompositeText32(nxagentDisplay,
                               op,
                               nxagentPicturePriv(pSrc) -> picture,
                               nxagentPicturePriv(pDst) -> picture,
                               pForm,
                               xSrc,
                               ySrc,
                               elements -> xOff,
                               elements -> yOff,
                               (XGlyphElt32*) elements,
                               1);

        elements++;
      }
      break;
    }
    default:
    {
      #ifdef WARNING
      fprintf(stderr, "%s WARNING! Invalid size id [%d].\n",
                  sizeID);
      #endif
      break;
    }
  }

  #else /* #ifdef SPLIT_GLYPH_LISTS */

  elements = elts;

  switch (sizeID)
  {
    case 1:
    {
      #ifdef DEBUG
      fprintf(stderr, "%s: sizeID is [%d] - [%s]\n", __func__, sizeID, "XRenderCompositeText8");
      #endif

      XRenderCompositeText8(nxagentDisplay,
                            op,
                            nxagentPicturePriv(pSrc)->picture,
                            nxagentPicturePriv(pDst)->picture,
                            pForm,
                            xSrc,
                            ySrc,
                            elements -> xOff,
                            elements -> yOff,
                            (XGlyphElt8*) elements,
                            nlists);
      break;
    }
    case 2:
    {
      #ifdef DEBUG
      fprintf(stderr, "%s: sizeID is [%d] - [%s]\n", __func__, sizeID, "XRenderCompositeText16");
      #endif

      XRenderCompositeText16(nxagentDisplay,
                             op,
                             nxagentPicturePriv(pSrc) -> picture,
                             nxagentPicturePriv(pDst) -> picture,
                             pForm,
                             xSrc,
                             ySrc,
                             elements -> xOff,
                             elements -> yOff,
                             (XGlyphElt16*) elements,
                             nlists);
      break;
    }
    case 4:
    {
      #ifdef DEBUG
      fprintf(stderr, "%s: sizeID is [%d] - [%s]\n", __func__, sizeID, "XRenderCompositeText32");
      #endif

      XRenderCompositeText32(nxagentDisplay,
                             op,
                             nxagentPicturePriv(pSrc) -> picture,
                             nxagentPicturePriv(pDst) -> picture,
                             pForm,
                             xSrc,
                             ySrc,
                             elements -> xOff,
                             elements -> yOff,
                             (XGlyphElt32*) elements,
                             nlists);
      break;
    }
    default:
    {
      #ifdef WARNING
      fprintf(stderr, "%s WARNING! Invalid size id [%d].\n", __func__,
                  sizeID);
      #endif
      break;
    }
  }
  #endif /* #ifdef SPLIT_GLYPH_LISTS */
}

void nxagentCompositeRects(CARD8 op, PicturePtr pDst, xRenderColor *color,
                               int nRect, xRectangle *rects)
{
  #ifdef DEBUG
  fprintf(stderr, "%s: entering...\n", __func__);
  #endif

  if (pDst == NULL)
  {
    return;
  }

  #ifdef TEST
  if (pDst && pDst->pDrawable) {
    fprintf(stderr, "%s: Called for picture at [%p] with [%s] at [%p].\n", __func__,
                (void *) pDst, (pDst -> pDrawable -> type == DRAWABLE_PIXMAP ? "pixmap" : "window"),
                    (void *) pDst -> pDrawable);
  }
  #endif

  /*
   * The CompositeRects() clears the destination's
   * corrupted region like the PolyFillRects() does.
   * As this case is harder to handle, at the moment
   * we only check for two ops.
   */

  if (nxagentDrawableStatus(pDst -> pDrawable) == NotSynchronized &&
          (op == PictOpSrc ||
               (op == PictOpOver && color -> alpha == 0xffff)))
  {
    RegionPtr rectRegion = RegionFromRects(nRect, rects, CT_REGION);

    if (pDst -> clientClipType != CT_NONE)
    {
      RegionRec tmpRegion;

      RegionInit(&tmpRegion, NullBox, 1);

      RegionCopy(&tmpRegion, (RegionPtr) pDst -> clientClip);

      if (pDst -> clipOrigin.x != 0 || pDst -> clipOrigin.y != 0)
      {
        RegionTranslate(&tmpRegion, pDst -> clipOrigin.x, pDst -> clipOrigin.y);
      }

      RegionIntersect(rectRegion, rectRegion, &tmpRegion);

      RegionUninit(&tmpRegion);
    }

    #ifdef TEST
    fprintf(stderr, "%s: Going to clean the drawable with extents [%d,%d,%d,%d].\n", __func__,
                rectRegion -> extents.x1, rectRegion -> extents.y1, rectRegion -> extents.x2, rectRegion -> extents.y2);
    #endif

    nxagentUnmarkCorruptedRegion(pDst -> pDrawable, rectRegion);

    RegionDestroy(rectRegion);
  }

  XRenderFillRectangles(nxagentDisplay,
                        op,
                        (Picture)nxagentPicturePriv(pDst) -> picture,
                        (XRenderColor *) color,
                        (XRectangle *) rects,
                        nRect);

  #ifdef DEBUG
  XSync(nxagentDisplay, 0);
  #endif
}

void nxagentTrapezoids(CARD8 op, PicturePtr pSrc, PicturePtr pDst,
                           PictFormatPtr maskFormat, INT16 xSrc, INT16 ySrc,
                               int ntrap, xTrapezoid *traps)
{
  XTrapezoid *current = (XTrapezoid *) traps;

  int remaining = ntrap;

  #ifdef TEST
  fprintf(stderr, "%s: Source [%p] destination [%p] coordinates "
              "[%d,%d] elements [%d].\n", __func__, (void *) pSrc, (void *) pDst,
                  xSrc, ySrc, ntrap);
  #endif

  if (pSrc == NULL || pDst == NULL)
  {
    return;
  }

  XRenderPictFormat *pForm = NULL;

  if (maskFormat != NULL)
  {
    pForm = nxagentMatchingFormats(maskFormat);
    nxagentPrintFormat(pForm);

    if (pForm == NULL)
    {
      return;
    }
  }
/*
FIXME: Is this useful or just a waste of bandwidth?

       Apparently useless with QT.
*/
  #ifndef SKIP_LOUSY_RENDER_OPERATIONS

  #ifdef TEST
  if (pSrc->pDrawable) {
    fprintf(stderr, "%s: Source is a [%s] of geometry [%d,%d].\n", __func__,
                (pSrc -> pDrawable -> type == DRAWABLE_PIXMAP ? "pixmap" : "window"),
                    pSrc -> pDrawable -> width, pSrc -> pDrawable -> height);
  }
  if (pSrc ->pDrawable != pDst -> pDrawable)
  {
    fprintf(stderr, "%s: Destination is a [%s] of geometry [%d,%d].\n", __func__,
                (pDst -> pDrawable -> type == DRAWABLE_PIXMAP ? "pixmap" : "window"),
                    pDst -> pDrawable -> width, pDst -> pDrawable -> height);
  }
  #endif

  /*
   * If the destination drawable is not synchronized
   * but the trapezoids extents are included in the
   * dirty region, we can defer the operation.
   */

  if (nxagentDrawableStatus(pDst -> pDrawable) == NotSynchronized &&
          RegionContainsRect(nxagentCorruptedRegion(pDst -> pDrawable),
                             nxagentTrapezoidExtents) == rgnIN)
  {
    #ifdef TEST
    if (pDst && pDst->pDrawable) {
      fprintf(stderr, "%s: WARNING! Prevented operation on region [%d,%d,%d,%d] already dirty "
                  "for drawable [%s][%p].\n", __func__, nxagentTrapezoidExtents -> x1, nxagentTrapezoidExtents -> y1,
                      nxagentTrapezoidExtents -> x2, nxagentTrapezoidExtents -> y2,
                          pDst -> pDrawable -> type == DRAWABLE_PIXMAP ? "pixmap" : "window",
                              (void *) pDst -> pDrawable);
    }
    #endif

    if (pDst -> pDrawable -> type == DRAWABLE_PIXMAP)
    {
      nxagentPixmapContainTrapezoids((PixmapPtr) pDst -> pDrawable) = 1;
    }

    return;
  }

  /*
   * If the destination doesn't contain any glyphs,
   * we can defer the trapezoids drawing by marking
   * the destination as dirty.
   */

  if (NXAGENT_SHOULD_DEFER_TRAPEZOIDS(pDst -> pDrawable))
  {
    RegionPtr pDstRegion = nxagentCreateRegion(pDst -> pDrawable, NULL,
                                               nxagentTrapezoidExtents -> x1,
                                               nxagentTrapezoidExtents -> y1,
                                               nxagentTrapezoidExtents -> x2 - nxagentTrapezoidExtents -> x1,
                                               nxagentTrapezoidExtents -> y2 - nxagentTrapezoidExtents -> y1);

    #ifdef TEST
    if (pDst && pDst->pDrawable) {
      fprintf(stderr, "%s: WARNING! Prevented operation on region [%d,%d,%d,%d] "
                  "for drawable [%s][%p].\n", __func__, pDstRegion -> extents.x1, pDstRegion -> extents.y1,
                      pDstRegion -> extents.x2, pDstRegion -> extents.y2,
                          pDst -> pDrawable -> type == DRAWABLE_PIXMAP ? "pixmap" : "window",
                              (void *) pDst -> pDrawable);
    }
    #endif

    nxagentMarkCorruptedRegion(pDst -> pDrawable, pDstRegion);

    nxagentFreeRegion(pDstRegion);

    if (pDst -> pDrawable -> type == DRAWABLE_PIXMAP)
    {
      nxagentPixmapContainTrapezoids((PixmapPtr) pDst -> pDrawable) = 1;
    }

    return;
  }

  if (pSrc -> pDrawable != NULL &&
          nxagentDrawableStatus(pSrc -> pDrawable) == NotSynchronized)
  {
    #ifdef TEST
    fprintf(stderr, "%s: Going to synchronize the source drawable at [%p].\n", __func__,
                (void *) pSrc -> pDrawable);
    #endif

    nxagentSynchronizeBox(pSrc -> pDrawable, NullBox, NEVER_BREAK);
  }

  if (nxagentDrawableStatus(pDst -> pDrawable) == NotSynchronized)
  {
    #ifdef TEST
    fprintf(stderr, "%s: Going to synchronize the destination drawable at [%p].\n", __func__,
                (void *) pDst -> pDrawable);
    #endif

    nxagentSynchronizeBox(pDst -> pDrawable, nxagentTrapezoidExtents, NEVER_BREAK);
  }

  XRenderCompositeTrapezoids(nxagentDisplay,
                             op,
                             nxagentPicturePriv(pSrc) -> picture,
                             nxagentPicturePriv(pDst) -> picture,
                             pForm,
                             xSrc,
                             ySrc,
                             (XTrapezoid *) current,remaining);


  #endif

  #ifdef DEBUG
  XSync(nxagentDisplay, 0);
  #endif
}

void nxagentQueryFormats(void)
{
  #ifdef DEBUG
  fprintf(stderr, "%s.\n", __func__);
  #endif

  if (XRenderQueryFormats(nxagentDisplay))
  {
    int i;

    #ifdef DEBUG
    XSync(nxagentDisplay, 0);
    #endif

    XExtDisplayInfo *info = (XExtDisplayInfo *) XRenderFindDisplay(nxagentDisplay);

    #ifdef DEBUG
    XSync(nxagentDisplay, 0);
    #endif

    XRenderInfo *xri = (XRenderInfo *) info -> data;

    XRenderPictFormat *pformat = xri -> format;

    for (i = 0; i < xri -> nformat; i++)
    {
      nxagentArrayFormats[i] = *pformat;

      #ifdef DEBUG
      fprintf(stderr, "%s: Added format id [%d] type [%d] depth [%d] rgb [%d,%d,%d] "
                  "mask rgb [%d,%d,%d] alpha [%d] alpha mask [%d].\n", __func__, nxagentArrayFormats[i].id,
                      nxagentArrayFormats[i].type, nxagentArrayFormats[i].depth, nxagentArrayFormats[i].direct.red,
                          nxagentArrayFormats[i].direct.green, nxagentArrayFormats[i].direct.blue,
                              nxagentArrayFormats[i].direct.redMask, nxagentArrayFormats[i].direct.greenMask,
                                  nxagentArrayFormats[i].direct.blueMask, nxagentArrayFormats[i].direct.alpha,
                                      nxagentArrayFormats[i].direct.alphaMask);
      #endif

      pformat++;
    }

    #ifdef DEBUG
    if (nxagentNumFormats == 0)
    {
      fprintf(stderr, "%s: Number of formats is [%d].\n", __func__, i);
    }
    else
    {
      fprintf(stderr, "%s: Old number of formats is [%d]. New number of formats is [%d].\n", __func__,
                  nxagentNumFormats, i);
    }
    #endif

    nxagentNumFormats = i;
  }
}

void nxagentCreateGlyphSet(GlyphSetPtr pGly)
{
  #ifdef DEBUG
  fprintf(stderr, "%s: Glyphset at [%p].\n", __func__, (void *) pGly);
  #endif

  XRenderPictFormat *pForm = NULL;

  if (pGly -> format != NULL)
  {
    pForm = nxagentMatchingFormats(pGly -> format);
    nxagentPrintFormat(pForm);

    if (pForm == NULL)
    {
      return;
    }
  }

  pGly -> remoteID = XRenderCreateGlyphSet(nxagentDisplay, pForm);

  #ifdef DEBUG
  XSync(nxagentDisplay, 0);
  #endif
}

void nxagentReferenceGlyphSet(GlyphSetPtr glyphSet)
{
  if (glyphSet -> remoteID == 0)
  {
    #ifdef TEST
    fprintf(stderr, "%s: Operation deferred because glyphset at [%p] is corrupted.\n", __func__,
                (void *) glyphSet);
    #endif

    return;
  }

  XRenderReferenceGlyphSet (nxagentDisplay, glyphSet -> remoteID);
}

void nxagentFreeGlyphSet(GlyphSetPtr glyphSet)
{
  if (glyphSet -> remoteID == 0)
  {
    #ifdef TEST
    fprintf(stderr, "%s: Operation ignored because glyphset at [%p] is corrupted.\n", __func__,
                (void *) glyphSet);
    #endif

    return;
  }

  XRenderFreeGlyphSet(nxagentDisplay, glyphSet -> remoteID);
}

void nxagentAddGlyphs(GlyphSetPtr glyphSet, Glyph *gids, xGlyphInfo *gi,
                          int nglyphs, CARD8 *images, int sizeImages)
{
  #ifdef DEBUG
  fprintf(stderr, "%s: Glyphset at [%p]. Number of glyphs [%d].\n", __func__,
              (void *) glyphSet, nglyphs);
  #endif

  if (glyphSet -> remoteID == 0)
  {
    #ifdef TEST
    fprintf(stderr, "%s: Going to reconnect the glyhpset at [%p] before adding glyphs.\n", __func__,
                (void *) glyphSet);
    #endif

    nxagentReconnectGlyphSet(glyphSet, (XID) 0, (void*) NULL);
  }

  /*
   * By adding a glyph to a glyphset on
   * remote X server we mark its reference
   * as synchronized.
   */

  Glyph *tempGids = gids;

  for (int i = 0; i < nglyphs; i++)
  {
    GlyphRefPtr gr = FindGlyphRef(&glyphSet -> hash, *tempGids, 0, 0);

    if (gr && gr -> glyph != DeletedGlyph)
    {
      #ifdef DEBUG
      fprintf(stderr, "%s: Added Glyph [%p][%ld] to glyphset [%p].\n", __func__,
                  (void *) gr -> glyph, *tempGids, (void *) glyphSet);
      #endif

      gr -> corruptedGlyph = 0;
    }

    tempGids++;
  }

  CARD8 *normalizedImages = NULL;

  #ifdef DEBUG
  fprintf(stderr, "%s: sizeImages [%d]\n", __func__, sizeImages);
  #endif

  if (sizeImages > 0)
  {
    normalizedImages = malloc(sizeImages);

    if (normalizedImages != NULL)
    {
      memcpy(normalizedImages, images, sizeImages);

      if (glyphDepths[glyphSet -> fdepth] == 1 &&
              nxagentServerOrder() != BitmapBitOrder(nxagentDisplay))
      {
        nxagentBitOrderInvert ((unsigned char *) normalizedImages, sizeImages);
      }
    }
    else
    {
      #ifdef PANIC
      fprintf(stderr, "%s: PANIC! Allocation of normalized glyph images failed.\n", __func__);
      #endif
    }
  }

  if (normalizedImages == NULL)
  {
    normalizedImages = images;
  }

  #ifdef NXAGENT_RENDER_CLEANUP
  nxagentCleanGlyphs(gi, nglyphs, normalizedImages, glyphDepths[glyphSet -> fdepth], nxagentDisplay);
  #endif /* NXAGENT_RENDER_CLEANUP */

  XRenderAddGlyphs(nxagentDisplay,
                   glyphSet -> remoteID,
                   gids,
                   (XGlyphInfo*)(gi),
                   nglyphs,
                   (char*) normalizedImages,
                   sizeImages);

  if (normalizedImages != images)
  {
    SAFE_free(normalizedImages);
  }

  #ifdef DEBUG
  XSync(nxagentDisplay, 0);
  #endif
}

void nxagentFreeGlyphs(GlyphSetPtr glyphSet, CARD32 *gids, int nglyph)
{
  GlyphRefPtr gr;
  Glyph  gid;

  if (glyphSet -> remoteID == 0)
  {
    #ifdef TEST
    fprintf(stderr, "%s: Operation ignored because glyphset at [%p] is corrupted.\n", __func__,
                (void *) glyphSet);
    #endif

    return;
  }

  /*
   * We loop across the list of glyphs id
   * to establish if they have been added
   * to glyphset on remote X server, so
   * they can be freed.
   */

  CARD32 *tempGids = gids;

  for (int i = 0; i < nglyph; i++)
  {
    gid = (Glyph)*tempGids;

    if ((gr = FindGlyphRef(&glyphSet -> hash, *tempGids, 0, 0)) &&
            gr -> glyph != DeletedGlyph &&
                gr -> corruptedGlyph == 0)
    {
      XRenderFreeGlyphs(nxagentDisplay, glyphSet -> remoteID, &gid, 1);
    }

    tempGids++;
  }
}

void nxagentSetPictureTransform(PicturePtr pPicture, void * transform)
{
  #ifdef TEST
  fprintf(stderr, "%s: Going to set transform [%p] to picture at [%p].\n", __func__,
              (void *) transform, (void *) pPicture);
  #endif

/*
FIXME: Is this useful or just a waste of bandwidth?

       Apparently useless with QT.
*/
  #ifndef SKIP_LOUSY_RENDER_OPERATIONS
  XRenderSetPictureTransform(nxagentDisplay,
                                 nxagentPicturePriv(pPicture) -> picture,
                                     (XTransform *) transform);
  #endif
}

void nxagentSetPictureFilter(PicturePtr pPicture, char *filter, int name_size,
                                 void * params, int nparams)
{
  char *szFilter = Xmalloc(name_size + 1);

  if (szFilter == NULL)
  {
    #ifdef WARNING
    fprintf(stderr, "%s: error allocating memory for filter name.\n", __func__);
    #endif

    return;
  }

  strncpy(szFilter, filter, name_size);

  szFilter[name_size] = 0;

  #ifdef TEST
  fprintf(stderr, "%s: Going to set filter [%s] to picture at [%p].\n", __func__,
              szFilter, (void *) pPicture);
  #endif
/*
FIXME: Is this useful or just a waste of bandwidth?

       Apparently useless with QT.
*/
  #ifndef SKIP_LOUSY_RENDER_OPERATIONS
  XRenderSetPictureFilter(nxagentDisplay,
                          nxagentPicturePriv(pPicture) -> picture,
                          szFilter,
                          (XFixed *) params,
                          nparams);
  #endif

  SAFE_free(szFilter);
}


Bool nxagentPictureInit(ScreenPtr pScreen, PictFormatPtr formats, int nformats)
{
  #ifdef RENDER
  #ifdef DEBUG
  fprintf(stderr, "%s: Screen [%p].\n", __func__, (void *) pScreen);
  #endif

  nxagentQueryFormats();

  if (fbPictureInit(pScreen, formats, nformats) == FALSE)
  {
    #ifdef DEBUG
    fprintf(stderr, "%s: failed: fbPictureInit returned [FALSE]\n", __func__);
    #endif
    return FALSE;
  }

  nxagentPicturePrivateIndex = AllocatePicturePrivateIndex();

  AllocatePicturePrivate(pScreen, nxagentPicturePrivateIndex, sizeof(nxagentPrivPictureRec));
  #endif

  return TRUE;
}

static void nxagentPrintFormat(XRenderPictFormat *pFormat)
{
#ifdef DEBUG
  if (pFormat == NULL)
  {
    fprintf(stderr, "%s: WARNING! null pointer passed to function.\n", __func__);
    return;
  }

  fprintf(stderr, "%s: Dumping information for format at [%p]:\n\
                   type=%d\n\
                   depth=%d\n\
                   red=%d\n\
                   redMask=%d\n\
                   green=%d\n\
                   greenMask=%d\n\
                   blue=%d\n\
                   blueMask=%d\n\
                   alpha=%d\n\
                   alphaMask=%d\n",
                   __func__,
                   (void *) pFormat,
                   pFormat -> type,
                   pFormat -> depth,
                   pFormat -> direct.red,
                   pFormat -> direct.redMask,
                   pFormat -> direct.green,
                   pFormat -> direct.greenMask,
                   pFormat -> direct.blue,
                   pFormat -> direct.blueMask,
                   pFormat -> direct.alpha,
                   pFormat -> direct.alphaMask);
#endif
}

Bool nxagentFillGlyphSet(GlyphSetPtr pGly)
{
  #ifdef DEBUG
  fprintf(stderr, "%s: GlyphSet at [%p] Refcount [%ld] Glyphs [%ld] "
              "Format [%p] FDepth [%d] RemoteID [%ld].\n", __func__, (void *) pGly, pGly -> refcnt,
                  pGly -> hash.hashSet -> size, (void *) pGly -> format, pGly -> fdepth, pGly -> remoteID);
  #endif

  /*
   * The glyphs are synchronized when they
   * are used in a composite text. During
   * the reconnection we have only to mark
   * corrupted the glyphs for each glyphset.
   */

  for (int i = 0; i < pGly -> hash.hashSet -> size; i++)
  {
    GlyphPtr glyph = pGly -> hash.table[i].glyph;

    if (glyph && (glyph != DeletedGlyph))
    {
      pGly -> hash.table[i].corruptedGlyph = 1;
    }
  }

  return TRUE;
}

void nxagentReconnectGlyphSet(void* p0, XID x1, void *p2)
{
  GlyphSetPtr pGly = (GlyphSetPtr) p0;

  if (!nxagentReconnectTrap)
  {
    int i;
    XRenderPictFormat *pForm = NULL;

    #ifdef DEBUG
    fprintf(stderr, "%s: GlyphSet at [%p].\n", __func__, (void *) pGly);
    #endif

    if (pGly -> format)
    {
      pForm = nxagentMatchingFormats(pGly -> format);
      nxagentPrintFormat(pForm);
    }

    pGly -> remoteID = XRenderCreateGlyphSet(nxagentDisplay, pForm);

    /*
     * If we have deferred the operation, we
     * have to check the number of references
     * to the glyphset to update the X server.
     */

    if ((i = pGly -> refcnt) > 1)
    {
      while (i-- > 1)
      {
        nxagentReferenceGlyphSet(pGly);
      }
    }

    #ifdef DEBUG
    XSync(nxagentDisplay, 0);
    #endif

    nxagentFillGlyphSet(pGly);
  }
  else
  {
    pGly -> remoteID = 0;
  }
}

Bool nxagentReconnectAllGlyphSet(void *p)
{
  Bool success = True;

  nxagentQueryFormats();

  #ifdef DEBUG
  fprintf(stderr, "%s\n", __func__);
  #endif

  for (int i = 0; (i < MAXCLIENTS) && (success); i++)
  {
    if (clients[i])
    {
      FindClientResourcesByType(clients[i], GlyphSetType, nxagentReconnectGlyphSet, &success);
    }
  }

  return success;
}

void nxagentReconnectPicture(void * p0, XID x1, void *p2)
{
  PicturePtr pPicture = (PicturePtr) p0;
  Bool *pBool = (Bool *) p2;
  unsigned long mask = 0;

  XRenderPictureAttributes attributes;

  #ifdef TEST
  fprintf(stderr, "%s: Called with bool [%d] and picture at [%p].\n", __func__,
              *pBool, (void *) pPicture);

  fprintf(stderr, "%s: Virtual picture is [%ld].\n", __func__,
              nxagentPicture(pPicture));
  #endif

  /*
   * Check if a previous operation has failed
   * and that the involved objects are valid.
   */

  if (!*pBool || pPicture == NULL ||
          nxagentPicture(pPicture) != 0)
  {
    return;
  }

  if (pPicture -> repeat)
  {
    attributes.repeat = (Bool) pPicture -> repeat;
    mask |= CPRepeat;
  }

  if (pPicture -> alphaMap)
  {
    if (!nxagentPicture(pPicture -> alphaMap))
    {
      nxagentReconnectPicture(pPicture -> alphaMap, 0, pBool);

      if (!*pBool || !nxagentPicture(pPicture -> alphaMap))
      {
        return;
      }
    }

    attributes.alpha_map = nxagentPicture(pPicture -> alphaMap);
    attributes.alpha_x_origin = pPicture -> alphaOrigin.x;
    attributes.alpha_y_origin = pPicture -> alphaOrigin.y;
    mask |= (CPAlphaMap | CPAlphaXOrigin | CPAlphaYOrigin);
  }

  if (pPicture -> graphicsExposures)
  {
    attributes.graphics_exposures = pPicture -> graphicsExposures;
    mask |= CPGraphicsExposure;
  }

  attributes.subwindow_mode = pPicture -> subWindowMode;
  mask |= CPSubwindowMode;

  attributes.poly_edge = pPicture -> polyEdge;
  mask |= CPPolyEdge;

  attributes.poly_mode = pPicture -> polyMode;
  mask |= CPPolyMode;

  attributes.dither = pPicture -> dither;
  mask |= CPDither;

  attributes.component_alpha = pPicture -> componentAlpha;
  mask |= CPComponentAlpha;

  XRenderPictFormat *pForm = NULL;

  if (pPicture -> pFormat)
  {
    pForm = nxagentMatchingFormats(pPicture -> pFormat);
    nxagentPrintFormat(pForm);
  }

  if (!pForm && pPicture->pSourcePict)
  {
        /*possible we need to add support for other picture types, for example gradients...*/
        switch(pPicture->pSourcePict->type)
        {
        case SourcePictTypeSolidFill:
            nxagentPicturePriv(pPicture) -> picture = XRenderCreateSolidFill(nxagentDisplay,
                    (const XRenderColor*) &pPicture->pSourcePict->solidFill.fullColor);
            break;
        }
        return;
  }

  #ifdef TEST
  fprintf(stderr, "%s: Creating picture at [%p] with drawable [%ld] at [%p].\n", __func__,
              (void *) pPicture, nxagentDrawable(pPicture -> pDrawable), (void *) pPicture -> pDrawable);

  fprintf(stderr, "%s: Format is at [%p] mask is [%ld] attributes are at [%p].\n", __func__,
              (void *) pForm, mask, (void *) &attributes);
  #endif

  nxagentPicture(pPicture) = XRenderCreatePicture(nxagentDisplay,
                                                  nxagentDrawable(pPicture -> pDrawable),
                                                  pForm,
                                                  mask,
                                                  &attributes);

  #ifdef TEST
  XSync(nxagentDisplay, 0);
  #endif

  #ifdef TEST
  fprintf(stderr, "%s: Reconnected picture at [%p] with value [%ld].\n", __func__,
              (void *) pPicture, nxagentPicture(pPicture));
  #endif

  if (nxagentAlphaEnabled && pPicture -> pDrawable -> depth == 32 &&
          pPicture -> pFormat -> direct.alpha != 0)
  {
    if (pPicture -> pDrawable -> type == DRAWABLE_PIXMAP)
    {
      nxagentPixmapPriv((PixmapPtr) pPicture -> pDrawable) -> pPicture = pPicture;
    }
    else if (pPicture -> pDrawable -> type == DRAWABLE_WINDOW)
    {
      nxagentWindowPriv((WindowPtr) pPicture -> pDrawable) -> pPicture = pPicture;
    }
  }
}

Bool nxagentReconnectAllPicture(void *p)
{
  Bool r = True;

  #ifdef TEST
  fprintf(stderr, "%s: Going to recreate all pictures.\n", __func__);
  #endif

  for (int i = 0; i < MAXCLIENTS; i++)
  {
    if (clients[i])
    {
      FindClientResourcesByType(clients[i], PictureType, nxagentReconnectPicture, &r);

      #ifdef WARNING
      if (!r)
      {
        fprintf(stderr, "%s: WARNING! Failed to recreate "
                    "picture for client [%d].\n", __func__, i);
      }
      #endif
    }
  }

  return True;
}

void nxagentDisconnectPicture(void * p0, XID x1, void* p2)
{
  PicturePtr pPicture = (PicturePtr) p0;
  Bool *pBool = (Bool *) p2;

  #ifdef TEST
  fprintf(stderr, "nxagentDisconnectPicture: Called with bool [%d] and picture at [%p].\n",
              *pBool, (void *) pPicture);

  fprintf(stderr, "nxagentDisconnectPicture: Virtual picture is [%ld].\n",
              nxagentPicture(pPicture));
  #endif

  if (!*pBool || !pPicture)
  {
    return;
  }

  #ifdef DEBUG
  fprintf(stderr, "nxagentDisconnectPicture: %p - XID %lx\n",
              (void *) pPicture, nxagentPicture(pPicture));
  #endif

  nxagentPicture(pPicture) = None;
}

Bool nxagentDisconnectAllPicture(void)
{
  Bool r = True;

  #ifdef DEBUG
  fprintf(stderr, "%s.\n", __func__);
  #endif

  for (int i = 0; i < MAXCLIENTS; i++)
  {
    if (clients[i])
    {
      FindClientResourcesByType(clients[i], PictureType, nxagentDisconnectPicture, &r);

      #ifdef WARNING
      if (!r)
      {
        fprintf(stderr, "%s: WARNING! Failed to disconnect "
                    "picture for client [%d].\n", __func__, i);
      }
      #endif
    }
  }

  return True;
}

void nxagentRenderCreateSolidFill(PicturePtr pPicture, xRenderColor *color)
{
  if (nxagentRenderEnable == False)
  {
    return;
  }

  #ifdef DEBUG
  fprintf(stderr, "%s: Got called.\n", __func__);

  if (pPicture == NULL)
  {
    fprintf(stderr, "%s: WARNING! pPicture pointer is NULL.\n", __func__);
  }

  if (color == NULL)
  {
    fprintf(stderr, "%s: WARNING! color pointer is NULL.\n", __func__);
  }
  #endif /* #ifdef DEBUG */

  memset(&(nxagentPicturePriv(pPicture) -> lastServerValues), 0,
             sizeof(XRenderPictureAttributes_));

  Picture id = XRenderCreateSolidFill(nxagentDisplay, (XRenderColor *) color);

  #ifdef DEBUG
  XSync(nxagentDisplay, 0);
  #endif

  #ifdef TEST
  fprintf(stderr, "%s: Created solid fill xid [%lu].\n", __func__, id);
  #endif

  nxagentPicturePriv(pPicture) -> picture = id;
}

void nxagentRenderCreateLinearGradient(PicturePtr pPicture, xPointFixed *p1,
                                           xPointFixed *p2, int nStops,
                                               xFixed *stops,
                                                   xRenderColor *colors)
{
  if (nxagentRenderEnable == False)
  {
    return;
  }

  #ifdef DEBUG
  fprintf(stderr, "%s: Got called.\n", __func__);

  if (pPicture == NULL)
  {
    fprintf(stderr, "%s: WARNING! pPicture pointer is NULL.\n", __func__);
  }

  if (p1 == NULL)
  {
    fprintf(stderr, "%s: WARNING! p1 pointer is NULL.\n", __func__);
  }

  if (p2 == NULL)
  {
    fprintf(stderr, "%s: WARNING! p2 pointer is NULL.\n", __func__);
  }

  if (stops == NULL)
  {
    fprintf(stderr, "%s: WARNING! stops pointer is NULL.\n", __func__);
  }

  if (colors == NULL)
  {
    fprintf(stderr, "%s: WARNING! colors pointer is NULL.\n", __func__);
  }
  #endif /* #ifdef DEBUG */

  memset(&(nxagentPicturePriv(pPicture) -> lastServerValues), 0,
             sizeof(XRenderPictureAttributes_));

  XLinearGradient linearGradient;

  linearGradient.p1.x = (XFixed) p1 -> x;
  linearGradient.p1.y = (XFixed) p1 -> y;
  linearGradient.p2.x = (XFixed) p2 -> x;
  linearGradient.p2.y = (XFixed) p2 -> y;

  Picture id = XRenderCreateLinearGradient(nxagentDisplay, &linearGradient,
                                              (XFixed *) stops,
                                                  (XRenderColor *) colors, nStops);

  #ifdef DEBUG
  XSync(nxagentDisplay, 0);
  #endif

  #ifdef TEST
  fprintf(stderr, "%s: Created linear gradient xid [%lu].\n", __func__, id);
  #endif

  nxagentPicturePriv(pPicture) -> picture = id;
}

void nxagentRenderCreateRadialGradient(PicturePtr pPicture, xPointFixed *inner,
                                           xPointFixed *outer,
                                               xFixed innerRadius,
                                                   xFixed outerRadius,
                                                       int nStops,
                                                           xFixed *stops,
                                                               xRenderColor *colors)
{
  if (nxagentRenderEnable == False)
  {
    return;
  }

  #ifdef DEBUG
  fprintf(stderr, "%s: Got called.\n", __func__);

  if (pPicture == NULL)
  {
    fprintf(stderr, "%s: WARNING! pPicture pointer is NULL.\n", __func__);
  }

  if (inner == NULL)
  {
    fprintf(stderr, "%s: WARNING! inner pointer is NULL.\n", __func__);
  }

  if (outer == NULL)
  {
    fprintf(stderr, "%s: WARNING! outer pointer is NULL.\n", __func__);
  }

  if (stops == NULL)
  {
    fprintf(stderr, "%s: WARNING! stops pointer is NULL.\n", __func__);
  }

  if (colors == NULL)
  {
    fprintf(stderr, "%s: WARNING! colors pointer is NULL.\n", __func__);
  }
  #endif /* #ifdef DEBUG */

  memset(&(nxagentPicturePriv(pPicture) -> lastServerValues), 0,
               sizeof(XRenderPictureAttributes_));

  XRadialGradient radialGradient;

  radialGradient.inner.x = (XFixed) inner -> x;
  radialGradient.inner.y = (XFixed) inner -> y;
  radialGradient.inner.radius = (XFixed) innerRadius;
  radialGradient.outer.x = (XFixed) outer -> x;
  radialGradient.outer.y = (XFixed) outer -> y;
  radialGradient.outer.radius = (XFixed) outerRadius;

  Picture id = XRenderCreateRadialGradient(nxagentDisplay, &radialGradient,
                                       (XFixed *) stops,
                                           (XRenderColor *) colors, nStops);

  #ifdef DEBUG
  XSync(nxagentDisplay, 0);
  #endif

  #ifdef TEST
  fprintf(stderr, "%s: Created radial gradient xid [%lu].\n", __func__, id);
  #endif

  nxagentPicturePriv(pPicture) -> picture = id;
}

void nxagentRenderCreateConicalGradient(PicturePtr pPicture,
                                            xPointFixed *center,
                                                xFixed angle, int nStops,
                                                    xFixed *stops,
                                                        xRenderColor *colors)
{
  if (nxagentRenderEnable == False)
  {
    return;
  }

  #ifdef DEBUG
  fprintf(stderr, "%s: Got called.\n", __func__);

  if (pPicture == NULL)
  {
    fprintf(stderr, "%s: WARNING! pPicture pointer is NULL.\n", __func__);
  }

  if (center == NULL)
  {
    fprintf(stderr, "%s: WARNING! center pointer is NULL.\n", __func__);
  }

  if (stops == NULL)
  {
    fprintf(stderr, "%s: WARNING! stops pointer is NULL.\n", __func__);
  }

  if (colors == NULL)
  {
    fprintf(stderr, "%s: WARNING! colors pointer is NULL.\n", __func__);
  }
  #endif /* #ifdef DEBUG */

  memset(&(nxagentPicturePriv(pPicture) -> lastServerValues), 0,
             sizeof(XRenderPictureAttributes_));

  XConicalGradient conicalGradient;

  conicalGradient.center.x = (XFixed) center -> x;
  conicalGradient.center.y = (XFixed) center -> y;
  conicalGradient.angle = (XFixed) angle;

  Picture id = XRenderCreateConicalGradient(nxagentDisplay, &conicalGradient,
                                                (XFixed *) stops,
                                                    (XRenderColor *) colors, nStops);

  #ifdef DEBUG
  XSync(nxagentDisplay, 0);
  #endif

  #ifdef TEST
  fprintf(stderr, "%s: Created conical gradient xid [%lu].\n", __func__, id);
  #endif

  nxagentPicturePriv(pPicture) -> picture = id;
}
