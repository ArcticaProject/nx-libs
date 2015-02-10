/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2011 NoMachine, http://www.nomachine.com/.         */
/*                                                                        */
/* NXAGENT, NX protocol compression and NX extensions to this software    */
/* are copyright of NoMachine. Redistribution and use of the present      */
/* software is allowed according to terms specified in the file LICENSE   */
/* which comes in the source distribution.                                */
/*                                                                        */
/* Check http://www.nomachine.com/licensing.html for applicability.       */
/*                                                                        */
/* NX and NoMachine are trademarks of Medialogic S.p.A.                   */
/*                                                                        */
/* All rights reserved.                                                   */
/*                                                                        */
/**************************************************************************/

#include "NXpicturestr.h"
#include "NXglyphstr.h"

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

#include "Agent.h"
#include "Drawable.h"
#include "Trap.h"
#include "Args.h"

#define Atom   XlibAtom
#define Pixmap XlibPixmap
#include "../../../../lib/Xrender/Xrenderint.h"
#undef  Atom
#undef  Pixmap

#include "region.h"
#include "extutil.h"

#include "Display.h"
#include "Pixmaps.h"
#include "Cursor.h"
#include "Client.h"
#include "Image.h"
#include "Pixels.h"
#include "Handlers.h"

#include "NXproto.h"

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
 * Do we split the big trapezoid requests?
 */

#define TRAPEZOIDS_PER_REQUEST  256

/*
 * Margin added around the glyphs extent (in pixels).
 */

#define GLYPH_BOX_MARGIN 2

int nxagentRenderEnable = UNDEFINED;
int nxagentRenderVersionMajor;
int nxagentRenderVersionMinor;

int nxagentPicturePrivateIndex = 0;

#ifndef NXAGENT_UPGRADE
static int picturePrivateCount = 0;
#endif

static int nxagentNumFormats = 0;

static XRenderPictFormat nxagentArrayFormats[MAX_FORMATS];

XRenderPictFormat *nxagentMatchingFormats(PictFormatPtr pForm);

BoxPtr nxagentGlyphsExtents;
BoxPtr nxagentTrapezoidExtents;

#ifdef DEBUG

static void nxagentPrintFormat(XRenderPictFormat *pFormat);

#endif

/*
 * From NXglyph.c.
 */

extern const CARD8 glyphDepths[];

/*
 * From NXdispatch.c.
 */

extern void BitOrderInvert(unsigned char *data, int nbytes);

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

void nxagentDestroyPictureClip(PicturePtr pPicture);

void nxagentValidatePicture(PicturePtr pPicture, Mask mask);

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

void nxagentRasterizeTrapezoid(PicturePtr pMask, xTrapezoid *trap,
                                   int x_off, int y_off);

void nxagentTriangles(CARD8 op, PicturePtr pSrc, PicturePtr pDst,
                          PictFormatPtr maskFormat, INT16 xSrc, INT16 ySrc,
                              int ntri, xTriangle *tris);

void nxagentTriStrip(CARD8 op, PicturePtr pSrc, PicturePtr pDst,
                         PictFormatPtr  maskFormat, INT16 xSrc, INT16 ySrc,
                             int npoint, xPointFixed *points);

void nxagentChangePicture(PicturePtr pPicture, Mask mask);

void nxagentTriFan(CARD8 op, PicturePtr pSrc, PicturePtr pDst,
                       PictFormatPtr maskFormat, INT16 xSrc, INT16 ySrc,
                           int npoint, xPointFixed *points);

void nxagentReferenceGlyphSet(GlyphSetPtr glyphSet);

void nxagentFreeGlyphs(GlyphSetPtr glyphSet, CARD32 *gids, int nglyph);

void nxagentFreeGlyphSet(GlyphSetPtr glyphSet);

void nxagentSetPictureTransform(PicturePtr pPicture, pointer transform);

void nxagentSetPictureFilter(PicturePtr pPicture, char *filter, int name_size,
                                 pointer params, int nparams);

Bool nxagentReconnectAllGlyphSet(void *p);

Bool nxagentReconnectAllPicture(void *);

Bool nxagentDisconnectAllPicture(void);

void nxagentRenderExtensionInit()
{
  int first_event, first_error;
  int major_version, minor_version;

  if (XRenderQueryExtension(nxagentDisplay, &first_event, &first_error))
  {
    XRenderQueryVersion(nxagentDisplay, &major_version, &minor_version);

    /*
     * As the RENDER requests are passed directly to
     * the remote X server this can cause problems if 
     * our RENDER version is different from the version
     * supported by the remote. For this reasos let's
     * advertise to our clients the lowest between the
     + two versions.
     */

    if (major_version > RENDER_MAJOR || 
            (major_version == RENDER_MAJOR &&
                 minor_version > RENDER_MINOR))
    {
      #ifdef TEST
      fprintf(stderr, "nxagentRenderExtensionInit: Using render version [%d.%d] with "
                  "remote version [%d.%d].\n", RENDER_MAJOR, RENDER_MINOR,
                      major_version, minor_version);
      #endif

      nxagentRenderVersionMajor = RENDER_MAJOR;
      nxagentRenderVersionMinor = RENDER_MINOR;
    }
    else if (major_version < RENDER_MAJOR ||
                 (major_version == RENDER_MAJOR &&
                      minor_version < RENDER_MINOR))
    {
      #ifdef TEST
      fprintf(stderr, "Info: Local render version %d.%d is higher "
                  "than remote version %d.%d.\n", RENDER_MAJOR, RENDER_MINOR,
                      major_version, minor_version);

      fprintf(stderr, "Info: Lowering the render version reported to clients.\n");
      #endif

      nxagentRenderVersionMajor = major_version;
      nxagentRenderVersionMinor = minor_version;
    }
    else
    {
      #ifdef TEST
      fprintf(stderr, "nxagentRenderExtensionInit: Local render version %d.%d "
                  "matches remote version %d.%d.\n", RENDER_MAJOR, RENDER_MINOR,
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
  pCursor -> devPriv[pScreen -> myNum] = xalloc(sizeof(nxagentPrivCursor));

  if (nxagentCursorPriv(pCursor, pScreen) == NULL)
  {
    FatalError("xalloc failed");
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

int nxagentRenderRealizeCursor(ScreenPtr pScreen, CursorPtr pCursor)
{
  int cid;
  int x, y;

  PicturePtr pPicture;

  pPicture = nxagentCursorPicture(pCursor, pScreen);

  pPicture -> refcnt++;

  x = nxagentCursorXOffset(pCursor, pScreen);
  y = nxagentCursorYOffset(pCursor, pScreen);

  /*
   * Set the lossless trap so that the image functions
   * will not try to encode the image using a lossy
   * compression. Drawables should have a quality flag,
   * telling if they were originally encoded with a
   * lossy algorithm. This would allow us to skip the
   * synchronization if the cursor was already encoded
   * with the best quality.
   */

  #ifdef TEST
  fprintf(stderr, "nxagentRenderRealizeCursor: Forcing the synchronization "
              "of the cursor.\n");
  #endif

  nxagentMarkCorruptedRegion(pPicture -> pDrawable, NULL);

  nxagentLosslessTrap = 1;

  nxagentSynchronizeDrawable(pPicture -> pDrawable, DO_WAIT, NEVER_BREAK, NULL);

  nxagentLosslessTrap = 0;

  cid = XRenderCreateCursor(nxagentDisplay, nxagentPicture(pPicture), x, y);

  nxagentCursor(pCursor, pScreen) = cid;

  return 1;
}

int nxagentCreatePicture(PicturePtr pPicture, Mask mask)
{
  XRenderPictureAttributes attributes;
  unsigned long            valuemask=0;
  XRenderPictFormat        *pForm;

  Picture id;

  #ifdef DEBUG
  fprintf(stderr, "nxagentCreatePicture: Function called with picture at [%p] and mask [%ld].\n",
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
       fprintf (stderr, "nxagentCreatePicture: Picture uses a shared pixmap.\n");
     }
     else
     {
       fprintf (stderr, "nxagentCreatePicture: Picture uses a plain pixmap.\n");
     }
  }
  else
  {
     fprintf (stderr, "nxagentCreatePicture: Picture uses a window.\n");
  }

  #endif

  /*
   * All the picture default values are 0.
   */

  memset(&(nxagentPicturePriv(pPicture) -> lastServerValues), 0, sizeof(XRenderPictureAttributes));

  if (mask & CPRepeat)
  {
    attributes.repeat = (Bool)pPicture -> repeat;

    valuemask |= CPRepeat;

    nxagentSetPictureRemoteValue(pPicture, repeat, attributes.repeat);
  }

  if (mask & CPAlphaMap)
  {
    attributes.alpha_map = nxagentPicturePriv(pPicture -> alphaMap) -> picture;

    valuemask |= CPAlphaMap;

    nxagentSetPictureRemoteValue(pPicture, alpha_map, attributes.alpha_map);
  }

  if (mask & CPAlphaXOrigin)
  {
    attributes.alpha_x_origin = pPicture -> alphaOrigin.x;

    valuemask |= CPAlphaXOrigin;

    nxagentSetPictureRemoteValue(pPicture, alpha_x_origin, attributes.alpha_x_origin);
  }

  if (mask & CPAlphaYOrigin)
  {
    attributes.alpha_y_origin = pPicture -> alphaOrigin.y;

    valuemask |= CPAlphaYOrigin;

    nxagentSetPictureRemoteValue(pPicture, alpha_y_origin, attributes.alpha_y_origin);
  }

  if (mask & CPClipXOrigin)
  {
    attributes.clip_x_origin = pPicture -> clipOrigin.x;

    valuemask |= CPClipXOrigin;

    nxagentSetPictureRemoteValue(pPicture, clip_x_origin, attributes.clip_x_origin);
  }

  if (mask & CPClipYOrigin)
  {
    attributes.clip_y_origin = pPicture -> clipOrigin.y;

    valuemask |= CPClipYOrigin;

    nxagentSetPictureRemoteValue(pPicture, clip_y_origin, attributes.clip_y_origin);
  }

  if (mask & CPGraphicsExposure)
  {
    attributes.graphics_exposures = (Bool)pPicture -> graphicsExposures;

    valuemask |= CPGraphicsExposure;

    nxagentSetPictureRemoteValue(pPicture, graphics_exposures, attributes.graphics_exposures);
  }

  if (mask & CPSubwindowMode)
  {
    attributes.subwindow_mode = pPicture -> subWindowMode;

    valuemask |= CPSubwindowMode;

    nxagentSetPictureRemoteValue(pPicture, subwindow_mode, attributes.subwindow_mode);
  }

  if (mask & CPClipMask)
  {
    attributes.clip_mask = None;

    valuemask |= CPClipMask;

    nxagentSetPictureRemoteValue(pPicture, clip_mask, attributes.clip_mask);
  }

  if (mask & CPPolyEdge)
  {
    attributes.poly_edge = pPicture -> polyEdge;

    valuemask |= CPPolyEdge;

    nxagentSetPictureRemoteValue(pPicture, poly_edge, attributes.poly_edge);
  }

  if (mask & CPPolyMode)
  {
    attributes.poly_mode = pPicture -> polyMode;

    valuemask |= CPPolyMode;

    nxagentSetPictureRemoteValue(pPicture, poly_mode, attributes.poly_mode);
  }

  if (mask & CPDither)
  {
    attributes.dither = pPicture -> dither;

    valuemask |= CPDither;

    nxagentSetPictureRemoteValue(pPicture, dither, attributes.dither);
  }

  if (mask & CPComponentAlpha)
  {
    attributes.component_alpha = pPicture -> componentAlpha;

    valuemask |= CPComponentAlpha;

    nxagentSetPictureRemoteValue(pPicture, component_alpha, attributes.component_alpha);
  }

  pForm = NULL;

  if (pPicture -> pFormat != NULL)
  {
    pForm = nxagentMatchingFormats(pPicture -> pFormat);

    #ifdef DEBUG

    nxagentPrintFormat(pForm);

    #endif
  }

  if (pForm == NULL)
  {
    fprintf(stderr, "nxagentCreatePicture: WARNING! The requested format was not found.\n");

    return 0;
  }

  id = XRenderCreatePicture(nxagentDisplay,
                            nxagentDrawable(pPicture -> pDrawable),
                            pForm,
                            valuemask,
                            &attributes);

  #ifdef TEST
  fprintf(stderr, "nxagentCreatePicture: Created picture at [%p] with drawable at [%p].\n",
              (void *) pPicture, (void *) pPicture -> pDrawable);
  #endif

  #ifdef DEBUG

  XSync(nxagentDisplay, 0);

  #endif

  nxagentPicturePriv(pPicture) -> picture = id;

  if (nxagentAlphaEnabled == 1 && pPicture -> pDrawable->depth == 32 &&
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
  int i;

  for (i = 0; i < nxagentNumFormats; i++)
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
  fprintf(stderr, "nxagentMatchingFormats: The requested format was not found.\n");
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
  fprintf(stderr, "nxagentDestroyPicture: Going to destroy picture at [%p].\n",
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
  fprintf(stderr, "nxagentChangePictureClip: Going to change clip of picture at [%p].\n",
              (void *) pPicture);
  #endif

  #ifdef DEBUG
  fprintf(stderr, "nxagentChangePictureClip: clipType [%d] nRects [%d] xRectangle [%p] "
              "xOrigin [%d] yOrigin [%d].\n", clipType, nRects, (void *) rects, xOrigin, yOrigin);
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
      fprintf(stderr, "nxagentChangePictureClip: Clip type is [CT_PIXMAP].\n");
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
      fprintf(stderr, "nxagentChangePictureClip: Clip type is [CT_NONE].\n");
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
      fprintf(stderr, "nxagentChangePictureClip: Clip type is [CT_REGION].\n");
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
      fprintf(stderr, "nxagentChangePictureClip: clipType not found\n");
      #endif

      break;
    }
  }

  return 1;
}

void nxagentDestroyPictureClip(PicturePtr pPicture)
{
  #ifdef DEBUG
  fprintf(stderr, "nxagentDestroyPictureClip: Nothing to do.\n");
  #endif
}

void nxagentChangePicture(PicturePtr pPicture, Mask mask)
{
  XRenderPictureAttributes  attributes;
  unsigned long             valuemask = 0;

  #ifdef DEBUG
  fprintf(stderr, "nxagentChangePicture: Going to change picture at [%p] with mask [%ld].\n",
              (void *) pPicture, mask);
  #endif

  if (pPicture == NULL)
  {
    return;
  }

  if (mask & CPRepeat)
  {
    attributes.repeat = (Bool)pPicture -> repeat;

    if (nxagentCheckPictureRemoteValue(pPicture, repeat, attributes.repeat) == 0)
    {
      valuemask |= CPRepeat;

      nxagentSetPictureRemoteValue(pPicture, repeat, attributes.repeat);
    }
  }

  if (mask & CPAlphaMap)
  {
    attributes.alpha_map = nxagentPicturePriv(pPicture -> alphaMap) -> picture;

    if (nxagentCheckPictureRemoteValue(pPicture, alpha_map, attributes.alpha_map) == 0)
    {
      valuemask |= CPAlphaMap;

      nxagentSetPictureRemoteValue(pPicture, alpha_map, attributes.alpha_map);
    }
  }

  if (mask & CPAlphaXOrigin)
  {
    attributes.alpha_x_origin = pPicture -> alphaOrigin.x;

    if (nxagentCheckPictureRemoteValue(pPicture, alpha_x_origin, attributes.alpha_x_origin) == 0)
    {
      valuemask |= CPAlphaXOrigin;

      nxagentSetPictureRemoteValue(pPicture, alpha_x_origin, attributes.alpha_x_origin);
    }
  }

  if (mask & CPAlphaYOrigin)
  {
    attributes.alpha_y_origin = pPicture -> alphaOrigin.y;

    if (nxagentCheckPictureRemoteValue(pPicture, alpha_y_origin, attributes.alpha_y_origin) == 0)
    {
      valuemask |= CPAlphaYOrigin;

      nxagentSetPictureRemoteValue(pPicture, alpha_y_origin, attributes.alpha_y_origin);
    }
  }

  if (mask & CPClipXOrigin)
  {
    attributes.clip_x_origin = pPicture -> clipOrigin.x;

    if (nxagentCheckPictureRemoteValue(pPicture, clip_x_origin, attributes.clip_x_origin) == 0)
    {
      valuemask |= CPClipXOrigin;

      nxagentSetPictureRemoteValue(pPicture, clip_x_origin, attributes.clip_x_origin);
    }
  }

  if (mask & CPClipYOrigin)
  {
    attributes.clip_y_origin = pPicture -> clipOrigin.y;

    if (nxagentCheckPictureRemoteValue(pPicture, clip_y_origin, attributes.clip_y_origin) == 0)
    {
      valuemask |= CPClipYOrigin;

      nxagentSetPictureRemoteValue(pPicture, clip_y_origin, attributes.clip_y_origin);
    }
  }

  if (mask & CPGraphicsExposure)
  {
    attributes.graphics_exposures = (Bool)pPicture -> graphicsExposures;

    if (nxagentCheckPictureRemoteValue(pPicture, graphics_exposures, attributes.graphics_exposures) == 0)
    {
      valuemask |= CPGraphicsExposure;

      nxagentSetPictureRemoteValue(pPicture, graphics_exposures, attributes.graphics_exposures);
    }
  }

  if (mask & CPSubwindowMode)
  {
    attributes.subwindow_mode = pPicture -> subWindowMode;

    if (nxagentCheckPictureRemoteValue(pPicture, subwindow_mode, attributes.subwindow_mode) == 0)
    {
      valuemask |= CPSubwindowMode;

      nxagentSetPictureRemoteValue(pPicture, subwindow_mode, attributes.subwindow_mode);
    }
  }

  if (mask & CPClipMask)
  {
    attributes.clip_mask = None;

    /*
     * The nxagent doesn't know the remote id of
     * the picture's clip mask, so the clip_mask
     * value is used as a boolean: it is set to 0
     * when the clip_mask is None, otherwise it is
     * 1.
     */

    if (nxagentPicturePriv(pPicture) -> lastServerValues.clip_mask != 0)
    {
      valuemask |= CPClipMask;

      nxagentSetPictureRemoteValue(pPicture, clip_mask, 0);
    }
  }

  if (mask & CPPolyEdge)
  {
    attributes.poly_edge = pPicture -> polyEdge;

    if (nxagentCheckPictureRemoteValue(pPicture, poly_edge, attributes.poly_edge) == 0)
    {
      valuemask |= CPPolyEdge;

      nxagentSetPictureRemoteValue(pPicture, poly_edge, attributes.poly_edge);
    } 
  }

  if (mask & CPPolyMode)
  {
    attributes.poly_mode = pPicture -> polyMode;

    if (nxagentCheckPictureRemoteValue(pPicture, poly_mode, attributes.poly_mode) == 0)
    {
      valuemask |= CPPolyMode;

      nxagentSetPictureRemoteValue(pPicture, poly_mode, attributes.poly_mode);
    }
  }

  if (mask & CPDither)
  {
    attributes.dither = pPicture -> dither;

    if (nxagentCheckPictureRemoteValue(pPicture, dither, attributes.dither) == 0)
    {
      valuemask |= CPDither;

      nxagentSetPictureRemoteValue(pPicture, dither, attributes.dither);
    }
  }

  if (mask & CPComponentAlpha)
  {
    attributes.component_alpha = pPicture -> componentAlpha;

    if (nxagentCheckPictureRemoteValue(pPicture, component_alpha, attributes.component_alpha) == 0)
    {
      valuemask |= CPComponentAlpha;

      nxagentSetPictureRemoteValue(pPicture, component_alpha, attributes.component_alpha);
    }
  }

  #ifdef TEST

  if (pPicture -> pDrawable -> type == DRAWABLE_PIXMAP)
  {
    fprintf(stderr, "nxagentChangePicture: %sPixmap [%p] Picture [%p][%p].\n",
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
    fprintf(stderr, "nxagentChangePicture: Skipping change of picture [%p] on remote X server.\n",
                (void *) pPicture);
  }
  #endif

  #endif

  #ifdef DEBUG

  XSync(nxagentDisplay, 0);

  #endif
}

void nxagentValidatePicture(PicturePtr pPicture, Mask mask)
{
  #ifdef DEBUG
  fprintf(stderr, "nxagentValidatePicture: Nothing to do.\n");
  #endif
}

void nxagentComposite(CARD8 op, PicturePtr pSrc, PicturePtr pMask, PicturePtr pDst,
                          INT16 xSrc, INT16 ySrc, INT16 xMask, INT16 yMask, INT16 xDst,
                              INT16 yDst, CARD16 width, CARD16 height)
{
  RegionPtr pDstRegion;

  if (pSrc == NULL || pDst == NULL)
  {
    return;
  }

  #ifdef DEBUG

  if (pSrc -> pDrawable != NULL)
  {
    fprintf(stderr, "nxagentComposite: Source Picture [%lu][%p] with drawable [%s%s][%p].\n",
                nxagentPicturePriv(pSrc) -> picture, (void *) pSrc,
                (pSrc -> pDrawable -> type == DRAWABLE_PIXMAP &&
                     nxagentIsShmPixmap((PixmapPtr) pSrc -> pDrawable)) ? "Shared " : "",
                         pSrc -> pDrawable -> type == DRAWABLE_PIXMAP ? "Pixmap" : "Window",
                             (void *) pSrc -> pDrawable);
  }

  fprintf(stderr, "nxagentComposite: Destination Picture [%lu][%p] with drawable [%s%s][%p].\n",
              nxagentPicturePriv(pDst) -> picture, (void *) pDst,
              (pDst -> pDrawable -> type == DRAWABLE_PIXMAP &&
                  nxagentIsShmPixmap((PixmapPtr) pDst -> pDrawable)) ? "Shared " : "",
                       pDst -> pDrawable -> type == DRAWABLE_PIXMAP ? "Pixmap" : "Window",
                           (void *) pDst -> pDrawable);

  if (pMask)
  {
    fprintf(stderr, "nxagentComposite: Mask Picture [%lu][%p] with drawable [%s%s][%p].\n",
                nxagentPicturePriv(pMask) -> picture, (void *) pMask,
                (pMask -> pDrawable -> type == DRAWABLE_PIXMAP &&
                    nxagentIsShmPixmap((PixmapPtr) pMask -> pDrawable)) ? "Shared " : "",
                       pMask -> pDrawable -> type == DRAWABLE_PIXMAP ? "Pixmap" : "Window",
                           (void *) pMask -> pDrawable);
  }

  #endif

  if (NXAGENT_SHOULD_DEFER_COMPOSITE(pSrc, pMask, pDst))
  {
    pDstRegion = nxagentCreateRegion(pDst -> pDrawable, NULL, xDst, yDst, width, height);

    #ifdef TEST
    fprintf(stderr, "nxagentComposite: WARNING! Prevented operation on region [%d,%d,%d,%d] "
                "for drawable at [%p] with type [%s].\n", pDstRegion -> extents.x1,
                    pDstRegion -> extents.y1, pDstRegion -> extents.x2, pDstRegion -> extents.y2,
                        (void *) pDst -> pDrawable,
                            pDst -> pDrawable -> type == DRAWABLE_PIXMAP ? "pixmap" : "window");

    #endif

    nxagentMarkCorruptedRegion(pDst -> pDrawable, pDstRegion);

    nxagentFreeRegion(pDst -> pDrawable, pDstRegion);

    return;
  }

  /*
   * Synchronize the content of the shared memory pixmap
   * but pay attention at not doing this more than once.
   * We need to wait until the image data has been recom-
   * posed at the X server side or the operation will use
   * the wrong data.
   */

  if (pSrc -> pDrawable != NULL)
  {
    nxagentSynchronizeShmPixmap(pSrc -> pDrawable, xSrc, ySrc, width, height);

    if (nxagentDrawableStatus(pSrc -> pDrawable) == NotSynchronized)
    {
      #ifdef TEST
      fprintf(stderr, "nxagentComposite: Synchronizing the source drawable [%p].\n",
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
      fprintf(stderr, "nxagentComposite: Synchronizing the destination drawable [%p].\n",
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
      fprintf(stderr, "nxagentComposite: Synchronizing the mask drawable [%p].\n",
                  (void *) pMask -> pDrawable);
      #endif

      nxagentSynchronizeDrawable(pMask -> pDrawable, DO_WAIT, NEVER_BREAK, NULL);
    }
  }

  /*
   * The glyphs flag have to be propagated between
   * drawables, in order to avoid to encode the
   * text with lossy algorithms (like JPEG). Unlu-
   * ckily we have verified that if the render com-
   * posite propagates the flag, the deferring of
   * render trapezoids doesn't work well. Moreover,
   * by commenting out this code we have not noticed
   * any visual problems.
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
  XRenderPictFormat *pForm;

  BoxRec glyphBox;

  XGlyphElt8 *elements;

  #ifdef SPLIT_GLYPH_LISTS

  GlyphPtr glyph;

  int x;
  int y;
  int i;
  int j;

  #endif /* #ifdef SPLIT_GLYPH_LISTS */

  if (pSrc == NULL || pDst == NULL)
  {
    return;
  }

  #ifdef TEST
  fprintf(stderr, "nxagentGlyphs: Called with source [%s][%p] destination [%s][%p] and size id [%d].\n",
              (pSrc -> pDrawable -> type == DRAWABLE_PIXMAP ? "pixmap" : "window"), (void *) pSrc, 
                  (pDst -> pDrawable -> type == DRAWABLE_PIXMAP ? "pixmap" : "window"), (void *) pDst, 
                      sizeID);
  #endif

  pForm = NULL;

  if (maskFormat != NULL)
  {
    pForm = nxagentMatchingFormats(maskFormat);
    
    #ifdef DEBUG

    nxagentPrintFormat(pForm);

    #endif

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
     * By extending the glyph extents the
     * visual aspect looks nicer because
     * the synchronized region is not glued
     * to the fonts.
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
   * If the destination window is hidden, the
   * operation can be prevented.
   */

  if (pDst -> pDrawable -> type == DRAWABLE_WINDOW)
  {
    RegionPtr pRegion;

    pRegion = nxagentCreateRegion(pDst -> pDrawable, NULL, glyphBox.x1, glyphBox.y1,
                                      glyphBox.x2 - glyphBox.x1, glyphBox.y2 - glyphBox.y1);
    
    if (REGION_NIL(pRegion) == 1)
    {
      #ifdef TEST
      fprintf(stderr, "nxagentGlyphs: WARNING! Glyphs prevented on hidden window at [%p].\n",
                  (void *) pDst -> pDrawable);
      #endif

      nxagentFreeRegion(pDst -> pDrawable, pRegion);

      return;
    }

    nxagentFreeRegion(pDst -> pDrawable, pRegion);
  }

  /*
   * Need to synchronize the pixmaps involved in
   * the operation before rendering the glyphs
   * on the real X server.
   */

  if (pSrc -> pDrawable != NULL &&
          nxagentDrawableStatus(pSrc -> pDrawable) == NotSynchronized)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentGlyphs: Synchronizing source [%s] at [%p].\n",
                pSrc -> pDrawable -> type == DRAWABLE_PIXMAP ? "pixmap" : "window",
                    (void *) pSrc -> pDrawable);
    #endif

    /*
     * If the source drawable is going to be
     * repeated over the destination drawable
     * during the composite operation, we need
     * to synchronize the whole drawable to
     * avoid graphical problems.
     */

    if (pSrc -> repeat == 1 || nxagentGlyphsExtents == NullBox)
    {
      #ifdef DEBUG
      fprintf(stderr, "nxagentGlyphs: Synchronizing source [%s] at [%p] "
                  "with geometry [%d,%d,%d,%d].\n", 
                      (pSrc -> pDrawable -> type == DRAWABLE_PIXMAP ? "pixmap" : "window"),
                          (void *) pSrc -> pDrawable, pSrc -> pDrawable -> x, pSrc -> pDrawable -> y,
                              pSrc -> pDrawable -> x + pSrc -> pDrawable -> width,
                                  pSrc -> pDrawable -> y + pSrc -> pDrawable -> height);
      #endif

      nxagentSynchronizeBox(pSrc -> pDrawable, NullBox, NEVER_BREAK);
    }
    else
    {
      #ifdef DEBUG
      fprintf(stderr, "nxagentGlyphs: Synchronizing region [%d,%d,%d,%d] of source [%s] at [%p] "
                  "with geometry [%d,%d,%d,%d].\n", glyphBox.x1, glyphBox.y1, glyphBox.x2, glyphBox.y2,
                          (pSrc -> pDrawable -> type == DRAWABLE_PIXMAP ? "pixmap" : "window"),
                              (void *) pSrc -> pDrawable, pSrc -> pDrawable -> x, pSrc -> pDrawable -> y,
                                  pSrc -> pDrawable -> x + pSrc -> pDrawable -> width,
                                      pSrc -> pDrawable -> y + pSrc -> pDrawable -> height);
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
    fprintf(stderr, "nxagentGlyphs: Synchronizing destination [%s] at [%p].\n",
                pDst -> pDrawable -> type == DRAWABLE_PIXMAP ? "pixmap" : "window",
                    (void *) pDst -> pDrawable);
    #endif

    if (nxagentGlyphsExtents == NullBox)
    {
      #ifdef DEBUG
      fprintf(stderr, "nxagentGlyphs: Synchronizing destination [%s] at [%p] "
                  "with geometry [%d,%d,%d,%d].\n", 
                      (pDst -> pDrawable -> type == DRAWABLE_PIXMAP ? "pixmap" : "window"),
                          (void *) pDst -> pDrawable, pDst -> pDrawable -> x, pDst -> pDrawable -> y,
                              pDst -> pDrawable -> x + pDst -> pDrawable -> width,
                                  pDst -> pDrawable -> y + pDst -> pDrawable -> height);
      #endif

      nxagentSynchronizeBox(pDst -> pDrawable, NullBox, NEVER_BREAK);
    }
    else
    {
      #ifdef DEBUG
      fprintf(stderr, "nxagentGlyphs: Synchronizing region [%d,%d,%d,%d] of destination [%s] at [%p] "
                  "with geometry [%d,%d,%d,%d].\n", glyphBox.x1, glyphBox.y1, glyphBox.x2, glyphBox.y2,
                          (pDst -> pDrawable -> type == DRAWABLE_PIXMAP ? "pixmap" : "window"),
                              (void *) pDst -> pDrawable, pDst -> pDrawable -> x, pDst -> pDrawable -> y,
                                  pDst -> pDrawable -> x + pDst -> pDrawable -> width,
                                      pDst -> pDrawable -> y + pDst -> pDrawable -> height);
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
  fprintf(stderr, "nxagentGlyphs: Glyph flag set on drawable [%s][%p].\n",
              pDst -> pDrawable -> type == DRAWABLE_PIXMAP ? "pixmap" : "window",
                  (void *) pDst -> pDrawable);
  #endif

  #ifdef SPLIT_GLYPH_LISTS

  /*
   * We split glyphs lists here and recalculate
   * the offsets of each list to make them ab-
   * solute and not relatives to the prior list.
   * This way each time we call XRenderComposi-
   * teText it has to deal only with a list of
   * glyphs. This is done to further improve
   * caching.
   */

  elements = elts;

  if (nlists > 1)
  {
    for (j = 1; j < nlists; j++)
    {
      x = elements -> xOff;
      y = elements -> yOff;

      #ifdef TEST
      fprintf(stderr, "nxagentGlyphs: Element [%d] of [%d] has offset [%d,%d].\n",
                  j, nlists, elements -> xOff, elements -> yOff);
      #endif

      for (i = 0; i < elements -> nchars; i++)
      {
        glyph = *glyphsBase++;

        x += glyph -> info.xOff;
        y += glyph -> info.yOff;
        
        #ifdef TEST
        fprintf(stderr, "nxagentGlyphs: Glyph at index [%d] has offset [%d,%d] and "
                    "position [%d,%d].\n", i, elements -> nchars, glyph -> info.xOff,
                        glyph -> info.yOff, x, y);
        #endif
      }

      elements++;

      elements -> xOff += x;
      elements -> yOff += y;

      #ifdef TEST
      fprintf(stderr, "nxagentGlyphs: New offset for list at [%p] is [%d,%d].\n",
                  elements, elements -> xOff, elements -> yOff);
      #endif
    }

    elements = elts;
  }

  switch (sizeID)
  {
    case 1:
    {
      for (j = 0; j < nlists; j++)
      {
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
      for (j = 0; j < nlists; j++)
      {
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
      for (j = 0; j < nlists; j++)
      {
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
      fprintf(stderr, "nxagentGlyphs: WARNING! Invalid size id [%d].\n",
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
      fprintf(stderr, "nxagentGlyphs: WARNING! Invalid size id [%d].\n",
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
  RegionPtr rectRegion;

  if (pDst == NULL)
  {
    return;
  }

  #ifdef TEST
  fprintf(stderr, "nxagentCompositeRects: Called for picture at [%p] with [%s] at [%p].\n",
              (void *) pDst, (pDst -> pDrawable -> type == DRAWABLE_PIXMAP ? "pixmap" : "window"),
                  (void *) pDst -> pDrawable);
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
    rectRegion = RECTS_TO_REGION(pDst -> pDrawable -> pScreen, nRect, rects, CT_REGION);

    if (pDst -> clientClipType != CT_NONE)
    {
      RegionRec tmpRegion;

      REGION_INIT(pDst -> pDrawable -> pScreen, &tmpRegion, NullBox, 1);

      REGION_COPY(pDst -> pDrawable -> pScreen, &tmpRegion, (RegionPtr) pDst -> clientClip);

      if (pDst -> clipOrigin.x != 0 || pDst -> clipOrigin.y != 0)
      {
        REGION_TRANSLATE(pDst -> pDrawable -> pScreen, &tmpRegion, pDst -> clipOrigin.x, pDst -> clipOrigin.y);
      }

      REGION_INTERSECT(pDst -> pDrawable -> pScreen, rectRegion, rectRegion, &tmpRegion);

      REGION_UNINIT(pDst -> pDrawable -> pScreen, &tmpRegion);      
    }

    #ifdef TEST
    fprintf(stderr, "nxagentCompositeRects: Going to clean the drawable with extents [%d,%d,%d,%d].\n",
                rectRegion -> extents.x1, rectRegion -> extents.y1, rectRegion -> extents.x2, rectRegion -> extents.y2);
    #endif

    nxagentUnmarkCorruptedRegion(pDst -> pDrawable, rectRegion);

    REGION_DESTROY(pDrawable -> pScreen, rectRegion);
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
  XRenderPictFormat *pForm;

  XTrapezoid *current = (XTrapezoid *) traps;

  RegionPtr pDstRegion;

  int remaining = ntrap;

  #ifdef TEST
  fprintf(stderr, "nxagentTrapezoids: Source [%p] destination [%p] coordinates "
              "[%d,%d] elements [%d].\n", (void *) pSrc, (void *) pDst,
                  xSrc, ySrc, ntrap);
  #endif

  if (pSrc == NULL || pDst == NULL)
  {
    return;
  }

  pForm = NULL;

  if (maskFormat != NULL)
  {
    pForm = nxagentMatchingFormats(maskFormat);
    
    #ifdef DEBUG

    nxagentPrintFormat(pForm);

    #endif

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

  fprintf(stderr, "nxagentTrapezoids: Source is a [%s] of geometry [%d,%d].\n",
              (pSrc -> pDrawable -> type == DRAWABLE_PIXMAP ? "pixmap" : "window"),
                  pSrc -> pDrawable -> width, pSrc -> pDrawable -> height);

  if (pSrc ->pDrawable != pDst -> pDrawable)
  {
    fprintf(stderr, "nxagentTrapezoids: Destination is a [%s] of geometry [%d,%d].\n",
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
          RECT_IN_REGION(pDst -> pDrawable -> pScreen, nxagentCorruptedRegion(pDst -> pDrawable),
                             nxagentTrapezoidExtents) == rgnIN)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentTrapezoids: WARNING! Prevented operation on region [%d,%d,%d,%d] already dirty "
                "for drawable [%s][%p].\n", nxagentTrapezoidExtents -> x1, nxagentTrapezoidExtents -> y1,
                    nxagentTrapezoidExtents -> x2, nxagentTrapezoidExtents -> y2,
                        pDst -> pDrawable -> type == DRAWABLE_PIXMAP ? "pixmap" : "window",
                            (void *) pDst -> pDrawable);
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
    pDstRegion = nxagentCreateRegion(pDst -> pDrawable, NULL,
                                     nxagentTrapezoidExtents -> x1,
                                     nxagentTrapezoidExtents -> y1,
                                     nxagentTrapezoidExtents -> x2 - nxagentTrapezoidExtents -> x1,
                                     nxagentTrapezoidExtents -> y2 - nxagentTrapezoidExtents -> y1);

    #ifdef TEST
    fprintf(stderr, "nxagentTrapezoids: WARNING! Prevented operation on region [%d,%d,%d,%d] "
                "for drawable [%s][%p].\n", pDstRegion -> extents.x1, pDstRegion -> extents.y1,
                    pDstRegion -> extents.x2, pDstRegion -> extents.y2,
                        pDst -> pDrawable -> type == DRAWABLE_PIXMAP ? "pixmap" : "window",
                            (void *) pDst -> pDrawable);
    #endif

    nxagentMarkCorruptedRegion(pDst -> pDrawable, pDstRegion);

    nxagentFreeRegion(pDst -> pDrawable, pDstRegion);

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
    fprintf(stderr, "nxagentTrapezoids: Going to synchronize the source drawable at [%p].\n",
                (void *) pSrc -> pDrawable);
    #endif

    nxagentSynchronizeBox(pSrc -> pDrawable, NullBox, NEVER_BREAK);
  }

  if (nxagentDrawableStatus(pDst -> pDrawable) == NotSynchronized)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentTrapezoids: Going to synchronize the destination drawable at [%p].\n",
                (void *) pDst -> pDrawable);
    #endif

    nxagentSynchronizeBox(pDst -> pDrawable, nxagentTrapezoidExtents, NEVER_BREAK);
  }

  while (remaining > 0)
  {
    XRenderCompositeTrapezoids(nxagentDisplay,
                               op,
                               nxagentPicturePriv(pSrc) -> picture,
                               nxagentPicturePriv(pDst) -> picture,
                               pForm,
                               xSrc,
                               ySrc,
                               (XTrapezoid *) current,
                               (remaining > TRAPEZOIDS_PER_REQUEST ?
                                   TRAPEZOIDS_PER_REQUEST : remaining));

    remaining -= TRAPEZOIDS_PER_REQUEST;
    current   += TRAPEZOIDS_PER_REQUEST;
  }

  #endif

  #ifdef DEBUG

  XSync(nxagentDisplay, 0);

  #endif
}

void nxagentRasterizeTrapezoid(PicturePtr pMask, xTrapezoid *trap,
                                   int x_off, int y_off)
{
  #ifdef DEBUG
  fprintf(stderr, "nxagentRasterizeTrapezoids: Nothing to do.\n");
  #endif
}

void nxagentTriangles(CARD8 op, PicturePtr pSrc, PicturePtr pDst,
                          PictFormatPtr maskFormat, INT16 xSrc, INT16 ySrc,
                              int ntri, xTriangle *tris)
{
  XRenderPictFormat *pForm;

  #ifdef DEBUG
  fprintf(stderr, "nxagentTriangles: Source [%p] Destination [%p] Coordinates [%d,%d] Elements [%d].\n",
              (void *) pSrc, (void *) pDst, xSrc, ySrc, ntri);
  #endif

  if (pSrc == NULL || pDst == NULL)
  {
    return;
  }

  pForm = NULL;

  if (maskFormat != NULL)
  {
    pForm = nxagentMatchingFormats(maskFormat);

    #ifdef DEBUG

    nxagentPrintFormat(pForm);

    #endif

    if (pForm == NULL)
    {
      return;
    }
  }

  /*
   * If the X_RenderCompositeTriangles requests
   * increment the traffic, we can defer the
   * operation like nxagentTrapezoids() does.
   */

  if (pSrc -> pDrawable != NULL &&
          nxagentDrawableStatus(pSrc -> pDrawable) == NotSynchronized)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentTriangles: Going to synchronize the source drawable at [%p].\n",
                (void *) pSrc -> pDrawable);
    #endif

    nxagentSynchronizeDrawable(pSrc -> pDrawable, DO_WAIT, NEVER_BREAK, NULL);
  }

  if (nxagentDrawableStatus(pDst -> pDrawable) == NotSynchronized)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentTriangles: Going to synchronize the destination drawable at [%p].\n",
                (void *) pDst -> pDrawable);
    #endif

    nxagentSynchronizeDrawable(pDst -> pDrawable, DO_WAIT, NEVER_BREAK, NULL);
  }

  XRenderCompositeTriangles(nxagentDisplay,
                            op,
                            nxagentPicturePriv(pSrc) -> picture,
                            nxagentPicturePriv(pDst) -> picture,
                            pForm,
                            xSrc,
                            ySrc,
                            (XTriangle*)tris,
                            ntri);

  #ifdef DEBUG

  XSync(nxagentDisplay, 0);

  #endif
}

void nxagentTriStrip(CARD8 op, PicturePtr pSrc, PicturePtr pDst,
                         PictFormatPtr  maskFormat, INT16 xSrc, INT16 ySrc,
                             int npoint, xPointFixed *points)
{
  XRenderPictFormat *pForm;

  #ifdef DEBUG
  fprintf(stderr, "nxagentTriStrip: Source [%p] Destination [%p] Coordinates [%d,%d] Elements [%d].\n",
              (void *) pSrc, (void *) pDst, xSrc, ySrc, npoint);
  #endif

  if (pSrc == NULL || pDst == NULL)
  {
    return;
  }

  pForm = NULL;

  if (maskFormat != NULL)
  {
    pForm = nxagentMatchingFormats(maskFormat);

    #ifdef DEBUG

    nxagentPrintFormat(pForm);

    #endif

    if (pForm == NULL)
    {
      return;
    }
  }

  /*
   * If the X_RenderCompositeTriStrip requests
   * increment the traffic, we can defer the
   * operation like nxagentTrapezoids() does.
   */

  if (pSrc -> pDrawable != NULL &&
          nxagentDrawableStatus(pSrc -> pDrawable) == NotSynchronized)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentTriStrip: Going to synchronize the source drawable at [%p].\n",
                (void *) pSrc -> pDrawable);
    #endif

    nxagentSynchronizeDrawable(pSrc -> pDrawable, DO_WAIT, NEVER_BREAK, NULL);
  }

  if (nxagentDrawableStatus(pDst -> pDrawable) == NotSynchronized)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentTriStrip: Going to synchronize the destination drawable at [%p].\n",
                (void *) pDst -> pDrawable);
    #endif

    nxagentSynchronizeDrawable(pDst -> pDrawable, DO_WAIT, NEVER_BREAK, NULL);
  }

  XRenderCompositeTriStrip(nxagentDisplay,
                           op,
                           nxagentPicturePriv(pSrc) -> picture,
                           nxagentPicturePriv(pDst) -> picture,
                           pForm,
                           xSrc,
                           ySrc,
                           (XPointFixed*)points,
                           npoint);

  #ifdef DEBUG

  XSync(nxagentDisplay, 0);

  #endif
}

void nxagentTriFan(CARD8 op, PicturePtr pSrc, PicturePtr pDst,
                       PictFormatPtr maskFormat, INT16 xSrc, INT16 ySrc,
                           int npoint, xPointFixed *points)
{
  XRenderPictFormat *pForm;

  #ifdef DEBUG
  fprintf(stderr, "nxagentTriFan: Source [%p] Destination [%p] Coordinates [%d,%d] Elements [%d].\n",
              (void *) pSrc, (void *) pDst, xSrc, ySrc, npoint);
  #endif

  if (pSrc == NULL || pDst == NULL)
  {
    return;
  }

  pForm = NULL;

  if (maskFormat != NULL)
  {
    pForm = nxagentMatchingFormats(maskFormat);

    #ifdef DEBUG

    nxagentPrintFormat(pForm);

    #endif

    if (pForm == NULL)
    {
      return;
    }
  }

  /*
   * If the X_RenderCompositeTriFan requests
   * increment the traffic, we can defer the
   * operation like nxagentTrapezoids() does.
   */

  if (pSrc -> pDrawable != NULL &&
          nxagentDrawableStatus(pSrc -> pDrawable) == NotSynchronized)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentTriFan: Going to synchronize the source drawable at [%p].\n",
                (void *) pSrc -> pDrawable);
    #endif

    nxagentSynchronizeDrawable(pSrc -> pDrawable, DO_WAIT, NEVER_BREAK, NULL);
  }

  if (nxagentDrawableStatus(pDst -> pDrawable) == NotSynchronized)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentTriFan: Going to synchronize the destination drawable at [%p].\n",
                (void *) pDst -> pDrawable);
    #endif

    nxagentSynchronizeDrawable(pDst -> pDrawable, DO_WAIT, NEVER_BREAK, NULL);
  }

  XRenderCompositeTriFan(nxagentDisplay,
                         op,
                         nxagentPicturePriv(pSrc) -> picture,
                         nxagentPicturePriv(pDst) -> picture,
                         pForm,
                         xSrc,
                         ySrc,
                         (XPointFixed*)points,
                         npoint);

  #ifdef DEBUG

  XSync(nxagentDisplay, 0);

  #endif
}

#ifndef NXAGENT_UPGRADE

/*
FIXME: In the 3.0.0 port these functions have been moved
       to Picture.c. We can remove them when the port is
       is complete.
*/
int AllocatePicturePrivateIndex()
{
  return picturePrivateCount++;
}

Bool AllocatePicturePrivate(register ScreenPtr pScreen, int index2, unsigned amount)
{
  unsigned oldamount;

  PictureScreenPtr ps = GetPictureScreen(pScreen);

  /*
   * Round up the size for proper alignment.
   */

  amount = ((amount + (sizeof(long) - 1)) / sizeof(long)) * sizeof(long);

  if (index2 >= ps -> PicturePrivateLen)
  {
    unsigned *nsizes = (unsigned *) xrealloc(ps -> PicturePrivateSizes,
                           (index2 + 1) * sizeof(unsigned));
    if (nsizes == 0)
    {
      return 0;
    }

    while (ps -> PicturePrivateLen <= index2)
    {
      nsizes[ps -> PicturePrivateLen++] = 0;

      ps -> totalPictureSize += sizeof(DevUnion);
    }

    ps -> PicturePrivateSizes = nsizes;
  }

  oldamount = ps -> PicturePrivateSizes[index2];

  if (amount > oldamount)
  {
    ps -> PicturePrivateSizes[index2] = amount;

    ps -> totalPictureSize += (amount - oldamount);
  }

  return 1;
}
#endif

void nxagentQueryFormats()
{
  XRenderInfo *xri;
  XExtDisplayInfo *info = NULL;
  XRenderPictFormat *pformat=NULL;

  int i;

  #ifdef DEBUG
  fprintf(stderr, "nxagentQueryFormats.\n");
  #endif

  if (XRenderQueryFormats(nxagentDisplay))
  {
    #ifdef DEBUG

    XSync(nxagentDisplay, 0);

    #endif

    info = (XExtDisplayInfo *) XRenderFindDisplay(nxagentDisplay);

    #ifdef DEBUG

    XSync(nxagentDisplay, 0);

    #endif

    xri = (XRenderInfo *) info -> data;

    pformat = xri -> format;

    for (i = 0; i < xri -> nformat; i++)
    {
      nxagentArrayFormats[i] = *pformat;

      #ifdef DEBUG
      fprintf(stderr, "nxagentQueryFormats: Added format type [%d] depth [%d] rgb [%d,%d,%d] "
                  "mask rgb [%d,%d,%d] alpha [%d] alpha mask [%d].\n",
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
      fprintf(stderr, "nxagentQueryFormats: Number of formats is [%d].\n",
                  i);
    }
    else
    {
      fprintf(stderr, "nxagentQueryFormats: Old number of formats is [%d]. New number of formats is [%d].\n",
                  nxagentNumFormats, i);
    }

    #endif

    nxagentNumFormats = i;
  }
}

void nxagentCreateGlyphSet(GlyphSetPtr pGly)
{
  XRenderPictFormat *pForm;

  #ifdef DEBUG
  fprintf(stderr, "nxagentCreateGlyphSet: Glyphset at [%p].\n", (void *) pGly);
  #endif

  pForm = NULL;

  if (pGly -> format != NULL)
  {
    pForm = nxagentMatchingFormats(pGly -> format);

    #ifdef DEBUG

    nxagentPrintFormat(pForm);

    #endif

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
    fprintf(stderr, "nxagentReferenceGlyphSet: Operation deferred because glyphset at [%p] is corrupted.\n",
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
    fprintf(stderr, "nxagentFreeGlyphs: Operation ignored because glyphset at [%p] is corrupted.\n",
                (void *) glyphSet);
    #endif

    return;
  }

  XRenderFreeGlyphSet(nxagentDisplay, glyphSet -> remoteID);
}

void nxagentAddGlyphs(GlyphSetPtr glyphSet, Glyph *gids, xGlyphInfo *gi,
                          int nglyphs, CARD8 *images, int sizeImages)
{
  GlyphRefPtr gr;
  Glyph *tempGids;

  int i;

  CARD8 *normalizedImages;

  #ifdef DEBUG
  fprintf(stderr, "nxagentAddGlyphs: Glyphset at [%p]. Number of glyphs [%d].\n",
              (void *) glyphSet, nglyphs);
  #endif

  if (glyphSet -> remoteID == 0)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentAddGlyphs: Going to reconnect the glyhpset at [%p] before adding glyphs.\n",
                (void *) glyphSet);
    #endif

    nxagentReconnectGlyphSet(glyphSet, (XID) 0, (void*) NULL);
  }

  /*
   * By adding a glyph to a glyphset on
   * remote X server we mark its reference
   * as synchronized.
   */

  tempGids = gids;

  for (i = 0; i < nglyphs; i++)
  {
    if ((gr = FindGlyphRef(&glyphSet -> hash, *tempGids, 0, 0)) &&
            gr -> glyph != DeletedGlyph)
    {
      #ifdef DEBUG
      fprintf(stderr, "nxagentAddGlyphs: Added Glyph [%p][%ld] to glyphset [%p].\n",
                  (void *) gr -> glyph, *tempGids, (void *) glyphSet);
      #endif

      gr -> corruptedGlyph = 0;
    }

    tempGids++;
  }

  normalizedImages = NULL;

  if (sizeImages > 0)
  {
    normalizedImages = xalloc(sizeImages);

    if (normalizedImages != NULL)
    {
      memcpy(normalizedImages, images, sizeImages);

      if (glyphDepths[glyphSet -> fdepth] == 1 &&
              nxagentServerOrder() != BitmapBitOrder(nxagentDisplay))
      {
        BitOrderInvert ((unsigned char *) normalizedImages, sizeImages);
      }
    }
    else
    {
      #ifdef PANIC
      fprintf(stderr, "nxagentAddGlyphs: PANIC! Allocation of normalized glyph images failed.\n");
      #endif
    }
  }

  if (normalizedImages == NULL)
  {
    normalizedImages = images;
  }

  XRenderCleanGlyphs(gi, nglyphs, normalizedImages, glyphDepths[glyphSet -> fdepth], nxagentDisplay);

  XRenderAddGlyphs(nxagentDisplay,
                   glyphSet -> remoteID,
                   gids,
                   (XGlyphInfo*)(gi),
                   nglyphs,
                   (char*) normalizedImages,
                   sizeImages);

  if (normalizedImages != images)
  {
    xfree(normalizedImages);
  }

  #ifdef DEBUG

  XSync(nxagentDisplay, 0);

  #endif
}

void nxagentFreeGlyphs(GlyphSetPtr glyphSet, CARD32 *gids, int nglyph)
{
  GlyphRefPtr gr;
  CARD32 *tempGids;
  Glyph  gid;

  int i;

  if (glyphSet -> remoteID == 0)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentFreeGlyphs: Operation ignored because glyphset at [%p] is corrupted.\n",
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

  tempGids = gids;

  for (i = 0; i < nglyph; i++)
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

void nxagentSetPictureTransform(PicturePtr pPicture, pointer transform)
{
  #ifdef TEST
  fprintf(stderr, "nxagentSetPictureTransform: Going to set transform [%p] to picture at [%p].\n",
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
                                 pointer params, int nparams)
{
  char *szFilter = Xmalloc(name_size + 1);

  if (szFilter == NULL)
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentSetPictureFilter: error allocating memory for filter name.\n");
    #endif

    return;
  }

  strncpy(szFilter, filter, name_size);

  szFilter[name_size] = 0;

  #ifdef TEST
  fprintf(stderr, "nxagentSetPictureFilter: Going to set filter [%s] to picture at [%p].\n",
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

  Xfree(szFilter);
}


Bool nxagentPictureInit(ScreenPtr pScreen, PictFormatPtr formats, int nformats)
{
  #ifdef RENDER

  #ifdef DEBUG
  fprintf(stderr, "nxagentPictureInit: Screen [%p].\n", (void *) pScreen);
  #endif

  nxagentQueryFormats();

  if (fbPictureInit(pScreen, formats, nformats) == 0)
  {
    return FALSE;
  }

  nxagentPicturePrivateIndex = AllocatePicturePrivateIndex();

  AllocatePicturePrivate(pScreen, nxagentPicturePrivateIndex, sizeof(nxagentPrivPictureRec));

  #endif

  return TRUE;
}

#ifdef DEBUG

static void nxagentPrintFormat(XRenderPictFormat *pFormat)
{
  if (pFormat == NULL)
  {
    fprintf(stderr, "nxagentPrintFormat: WARNING! null pointer passed to function.\n");

    return;
  }

  fprintf(stderr, "nxagentPrintFormat: Dumping information for format at [%p]:\n\
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
}

#endif

Bool nxagentFillGlyphSet(GlyphSetPtr pGly)
{
  GlyphPtr    glyph;

  int i;

  #ifdef DEBUG
  fprintf(stderr, "nxagentFillGlyphSet: GlyphSet at [%p] Refcount [%ld] Glyphs [%ld] "
              "Format [%p] FDepth [%d] RemoteID [%ld].\n", (void *) pGly, pGly -> refcnt,
                  pGly -> hash.hashSet -> size, (void *) pGly -> format, pGly -> fdepth, pGly -> remoteID);
  #endif

  /*
   * The glyphs are synchronized when they
   * are used in a composite text. During
   * the reconnection we have only to mark
   * corrupted the glyphs for each glyphset.
   */

  for (i = 0; i < pGly -> hash.hashSet -> size; i++)
  {
    glyph = pGly -> hash.table[i].glyph;

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

  XRenderPictFormat *pForm = NULL;

  int i;

  if (nxagentReconnectTrap == 0)
  {
    #ifdef DEBUG
    fprintf(stderr, "nxagentReconnectGlyphSet: GlyphSet at [%p].\n", (void *) pGly);
    #endif

    if (pGly -> format)
    {
      pForm = nxagentMatchingFormats(pGly -> format);
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
  Bool success = TRUE;
  int i;

  nxagentQueryFormats();

  #ifdef DEBUG
  fprintf(stderr, "nxagentReconnectAllGlyphSet\n");
  #endif

  for (i = 0; (i < MAXCLIENTS) && (success); i++)
  {
    if (clients[i])
    {
      FindClientResourcesByType(clients[i], GlyphSetType, nxagentReconnectGlyphSet, &success);
    }
  }

  return success;
}

void nxagentReconnectPicture(pointer p0, XID x1, void *p2)
{
  PicturePtr pPicture = (PicturePtr) p0;
  Bool *pBool = (Bool *) p2;
  unsigned long mask = 0;

  XRenderPictureAttributes attributes;
  XRenderPictFormat        *pForm;

  #ifdef TEST
  fprintf(stderr, "nxagentReconnectPicture: Called with bool [%d] and picture at [%p].\n",
              *pBool, (void *) pPicture);

  fprintf(stderr, "nxagentReconnectPicture: Virtual picture is [%ld].\n",
              nxagentPicture(pPicture));
  #endif

  /*
   * Check if a previous operation has failed
   * and that the involved objects are valid.
   */

  if (*pBool == 0 || pPicture == NULL ||
          nxagentPicture(pPicture) != 0)
  {
    return;
  }

  if (pPicture -> repeat)
  {
    mask |= CPRepeat;

    attributes.repeat = (Bool) pPicture -> repeat;
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

  pForm = NULL;

  if (pPicture -> pFormat)
  {
    pForm = nxagentMatchingFormats(pPicture -> pFormat);

    #ifdef DEBUG

    nxagentPrintFormat(pForm);

    #endif
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
  fprintf(stderr, "nxagentReconnectPicture: Creating picture at [%p] with drawable [%ld] at [%p].\n",
              (void *) pPicture, nxagentDrawable(pPicture -> pDrawable), (void *) pPicture -> pDrawable);

  fprintf(stderr, "nxagentReconnectPicture: Format is at [%p] mask is [%ld] attributes are at [%p].\n",
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
  fprintf(stderr, "nxagentReconnectPicture: Reconnected picture at [%p] with value [%ld].\n",
              (void *) pPicture, nxagentPicture(pPicture));
  #endif

  if (nxagentAlphaEnabled == 1 && pPicture -> pDrawable -> depth == 32 &&
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
  int i;
  Bool r;

  #ifdef TEST
  fprintf(stderr, "nxagentReconnectAllPicture: Going to recreate all pictures.\n");
  #endif

  for (i = 0, r = 1; i < MAXCLIENTS; i++)
  {
    if (clients[i])
    {
      FindClientResourcesByType(clients[i], PictureType, nxagentReconnectPicture, &r);

      #ifdef WARNING

      if (r == False)
      {
        fprintf(stderr, "nxagentReconnectAllPicture: WARNING! Failed to recreate "
                    "picture for client [%d].\n", i);
      }

      #endif
    }
  }

  return True;
}

void nxagentDisconnectPicture(pointer p0, XID x1, void* p2)
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

Bool nxagentDisconnectAllPicture()
{
  int i;
  Bool r;

  #ifdef DEBUG
  fprintf(stderr, "nxagentDisconnectAllPicture.\n");
  #endif

  for (i = 0, r = 1; i < MAXCLIENTS; i++)
  {
    if (clients[i])
    {
      FindClientResourcesByType(clients[i], PictureType, nxagentDisconnectPicture, &r);

      #ifdef WARNING

      if (r == False)
      {
        fprintf(stderr, "nxagentDisconnectAllPicture: WARNING! Failed to disconnect "
                    "picture for client [%d].\n", i);
      }

      #endif
    }
  }

  return True;
}

void nxagentRenderCreateSolidFill(PicturePtr pPicture, xRenderColor *color)
{
  Picture id;

  if (nxagentRenderEnable == False)
  {
    return;
  }

  #ifdef DEBUG

  fprintf(stderr, "nxagentRenderCreateSolidFill: Got called.\n");

  if (pPicture == NULL)
  {
    fprintf(stderr, "nxagentRenderCreateSolidFill: WARNING! pPicture pointer is NULL.\n");
  }

  if (color == NULL)
  {
    fprintf(stderr, "nxagentRenderCreateSolidFill: WARNING! color pointer is NULL.\n");
  }

  #endif /* #ifdef DEBUG */

  memset(&(nxagentPicturePriv(pPicture) -> lastServerValues), 0,
             sizeof(XRenderPictureAttributes_));

  id = XRenderCreateSolidFill(nxagentDisplay, (XRenderColor *) color);

  #ifdef DEBUG
  XSync(nxagentDisplay, 0);
  #endif

  #ifdef TEST
  fprintf(stderr, "nxagentRenderCreateSolidFill: Created solid fill xid [%lu].\n", id);
  #endif

  nxagentPicturePriv(pPicture) -> picture = id;
}

void nxagentRenderCreateLinearGradient(PicturePtr pPicture, xPointFixed *p1,
                                           xPointFixed *p2, int nStops,
                                               xFixed *stops,
                                                   xRenderColor *colors)
{
  Picture id;

  XLinearGradient linearGradient;

  if (nxagentRenderEnable == False)
  {
    return;
  }

  #ifdef DEBUG

  fprintf(stderr, "nxagentRenderCreateLinearGradient: Got called.\n");

  if (pPicture == NULL)
  {
    fprintf(stderr, "nxagentRenderCreateLinearGradient: WARNING! pPicture pointer is NULL.\n");
  }

  if (p1 == NULL)
  {
    fprintf(stderr, "nxagentRenderCreateLinearGradient: WARNING! p1 pointer is NULL.\n");
  }

  if (p2 == NULL)
  {
    fprintf(stderr, "nxagentRenderCreateLinearGradient: WARNING! p2 pointer is NULL.\n");
  }

  if (stops == NULL)
  {
    fprintf(stderr, "nxagentRenderCreateLinearGradient: WARNING! stops pointer is NULL.\n");
  }

  if (colors == NULL)
  {
    fprintf(stderr, "nxagentRenderCreateLinearGradient: WARNING! colors pointer is NULL.\n");
  }

  #endif /* #ifdef DEBUG */

  memset(&(nxagentPicturePriv(pPicture) -> lastServerValues), 0,
             sizeof(XRenderPictureAttributes_));

  linearGradient.p1.x = (XFixed) p1 -> x;
  linearGradient.p1.y = (XFixed) p1 -> y;
  linearGradient.p2.x = (XFixed) p2 -> x;
  linearGradient.p2.y = (XFixed) p2 -> y;

  id = XRenderCreateLinearGradient(nxagentDisplay, &linearGradient,
                                      (XFixed *) stops,
                                          (XRenderColor *) colors, nStops);

  #ifdef DEBUG
  XSync(nxagentDisplay, 0);
  #endif

  #ifdef TEST
  fprintf(stderr, "nxagentRenderCreateLinearGradient: Created linear gradient xid [%lu].\n", id);
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
  Picture id;

  XRadialGradient radialGradient;

  if (nxagentRenderEnable == False)
  {
    return;
  }

  #ifdef DEBUG

  fprintf(stderr, "nxagentRenderCreateRadialGradient: Got called.\n");

  if (pPicture == NULL)
  {
    fprintf(stderr, "nxagentRenderCreateRadialGradient: WARNING! pPicture pointer is NULL.\n");
  }

  if (inner == NULL)
  {
    fprintf(stderr, "nxagentRenderCreateRadialGradient: WARNING! inner pointer is NULL.\n");
  }

  if (outer == NULL)
  {
    fprintf(stderr, "nxagentRenderCreateRadialGradient: WARNING! outer pointer is NULL.\n");
  }

  if (stops == NULL)
  {
    fprintf(stderr, "nxagentRenderCreateRadialGradient: WARNING! stops pointer is NULL.\n");
  }

  if (colors == NULL)
  {
    fprintf(stderr, "nxagentRenderCreateRadialGradient: WARNING! colors pointer is NULL.\n");
  }

  #endif /* #ifdef DEBUG */

  memset(&(nxagentPicturePriv(pPicture) -> lastServerValues), 0,
               sizeof(XRenderPictureAttributes_));

  radialGradient.inner.x = (XFixed) inner -> x;
  radialGradient.inner.y = (XFixed) inner -> y;
  radialGradient.inner.radius = (XFixed) innerRadius;
  radialGradient.outer.x = (XFixed) outer -> x;
  radialGradient.outer.y = (XFixed) outer -> y;
  radialGradient.outer.radius = (XFixed) outerRadius;

  id = XRenderCreateRadialGradient(nxagentDisplay, &radialGradient,
                                       (XFixed *) stops,
                                           (XRenderColor *) colors, nStops);

  #ifdef DEBUG
  XSync(nxagentDisplay, 0);
  #endif

  #ifdef TEST
  fprintf(stderr, "nxagentRenderCreateRadialGradient: Created radial gradient xid [%lu].\n", id);
  #endif

  nxagentPicturePriv(pPicture) -> picture = id;
}

void nxagentRenderCreateConicalGradient(PicturePtr pPicture,
                                            xPointFixed *center,
                                                xFixed angle, int nStops, 
                                                    xFixed *stops, 
                                                        xRenderColor *colors)
{
  Picture id;

  XConicalGradient conicalGradient;

  if (nxagentRenderEnable == False)
  {
    return;
  }

  #ifdef DEBUG

  fprintf(stderr, "nxagentRenderCreateConicalGradient: Got called.\n");

  if (pPicture == NULL)
  {
    fprintf(stderr, "nxagentRenderCreateConicalGradient: WARNING! pPicture pointer is NULL.\n");
  }

  if (center == NULL)
  {
    fprintf(stderr, "nxagentRenderCreateConicalGradient: WARNING! center pointer is NULL.\n");
  }

  if (stops == NULL)
  {
    fprintf(stderr, "nxagentRenderCreateConicalGradient: WARNING! stops pointer is NULL.\n");
  }

  if (colors == NULL)
  {
    fprintf(stderr, "nxagentRenderCreateConicalGradient: WARNING! colors pointer is NULL.\n");
  }

  #endif /* #ifdef DEBUG */

  memset(&(nxagentPicturePriv(pPicture) -> lastServerValues), 0,
             sizeof(XRenderPictureAttributes_));

  conicalGradient.center.x = (XFixed) center -> x;
  conicalGradient.center.y = (XFixed) center -> y;
  conicalGradient.angle = (XFixed) angle;

  id = XRenderCreateConicalGradient(nxagentDisplay, &conicalGradient,
                                        (XFixed *) stops,
                                            (XRenderColor *) colors, nStops);

  #ifdef DEBUG
  XSync(nxagentDisplay, 0);
  #endif

  #ifdef TEST
  fprintf(stderr, "nxagentRenderCreateConicalGradient: Created conical gradient xid [%lu].\n", id);
  #endif

  nxagentPicturePriv(pPicture) -> picture = id;
}

