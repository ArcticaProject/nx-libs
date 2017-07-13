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

#ifndef __Render_H__
#define __Render_H__

/*
 * Since upstream changed the definitions of PictFormat et al to an XID
 * base type, referencing Xlib data in the server makes short reads on
 * 64-bit hardware. Previously, the base type was "unsigned int", which
 * is sized differently on x86 machines - either 4 bytes in 32-bit mode,
 * or 8 bytes in 64-bit mode.
 * Since XID's are hardcoded to 4 bytes within X server code, we'll run
 * into a mismatch.
 *
 * As a workaround, pull in stuff from render.h early and override
 * the base type. It's ugly, but should get the work done.
 */
#include "X.h"
#include "Agent.h"
#define XID XlibXID
#include "render.h"
#undef  XID

#include "screenint.h"
#include "cursor.h"
#include "picture.h"
#include "renderproto.h"

#include "glyphstr.h"

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

void nxagentReconnectPicture(void * p0, XID x1, void *p2);
void nxagentDisconnectPicture(void * p0, XID x1, void* p2);

void nxagentReconnectGlyphSet(void* p0, XID x1, void *p2);

void nxagentDestroyPicture(PicturePtr pPicture);

#endif /* __Render_H__ */
