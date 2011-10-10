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

#ifndef __Render_H__
#define __Render_H__

#include "screenint.h"
#include "picture.h"
#include "renderproto.h"

#include "NXglyphstr.h"

#include "Agent.h"

extern int nxagentRenderEnable;
extern int nxagentRenderVersionMajor;
extern int nxagentRenderVersionMinor;

extern int nxagentPicturePrivateIndex;

extern BoxPtr nxagentGlyphsExtents;
extern BoxPtr nxagentTrapezoidExtents;

/*
 * Structure imported from Xrender.h. We don't
 * include Xrender.h at this point because of
 * clashes of definition.
 */

/*
 * Xlib Pixmap and Atom types are 8 bytes long
 * on 64-bit archs, whilst they are 4 bytes long
 * on 32-bit ones. At this point, Pixmap and Atom
 * are not Xlib types but Xserver ones: here they
 * are always 4 bytes long. So that we use XlibID
 * symbols defined below to fill the structure with
 * fields having the right size.
 */

typedef struct {
    int                 repeat;
    Picture             alpha_map;
    int                 alpha_x_origin;
    int                 alpha_y_origin;
    int                 clip_x_origin;
    int                 clip_y_origin;
    XlibPixmap          clip_mask;
    Bool                graphics_exposures;
    int                 subwindow_mode;
    int                 poly_edge;
    int                 poly_mode;
    XlibAtom            dither;
    Bool                component_alpha;
} XRenderPictureAttributes_;

typedef struct
{
  Picture picture;

  XRenderPictureAttributes_ lastServerValues;

} nxagentPrivPictureRec;

typedef nxagentPrivPictureRec *nxagentPrivPicturePtr;

#define nxagentPicturePriv(pPicture) \
  ((nxagentPrivPicturePtr) ((pPicture) -> devPrivates[nxagentPicturePrivateIndex].ptr))

#define nxagentPicture(pPicture) (nxagentPicturePriv(pPicture) -> picture)

#define nxagentSetPictureRemoteValue(pPicture, pvalue, value) \
do \
{ \
  nxagentPicturePriv(pPicture) -> lastServerValues.pvalue = value; \
} \
while (0)

#define nxagentCheckPictureRemoteValue(pPicture, pvalue, value) \
  (nxagentPicturePriv(pPicture) -> lastServerValues.pvalue == value)

void nxagentRenderExtensionInit(void);
Bool nxagentPictureInit(ScreenPtr, PictFormatPtr, int);

int nxagentRenderRealizeCursor(ScreenPtr pScreen, CursorPtr pCursor);

void nxagentAddGlyphs(GlyphSetPtr glyphSet, Glyph *gids, xGlyphInfo *gi,
                                  int nglyphs, CARD8 *images, int sizeImages);

void nxagentReconnectPicture(pointer p0, XID x1, void *p2);
void nxagentDisconnectPicture(pointer p0, XID x1, void* p2);

void nxagentReconnectGlyphSet(void* p0, XID x1, void *p2);

void nxagentDestroyPicture(PicturePtr pPicture);

#endif /* __Render_H__ */
