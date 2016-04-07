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

/*
 * $Id: damage.c,v 1.19 2005/10/06 21:55:41 anholt Exp $
 *
 * Copyright © 2003 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include "regionstr.h"
#include "../../miext/damage/damage.h"
#include <X11/fonts/font.h>

/* prototypes */

static int 
damageText (DrawablePtr	    pDrawable,
	    GCPtr	    pGC,
	    int		    x,
	    int		    y,
	    unsigned long   count,
	    char	    *chars,
	    FontEncoding    fontEncoding,
	    Bool	    textType);
static int
damagePolyText8(DrawablePtr pDrawable,
		GCPtr	    pGC,
		int	    x,
		int	    y,
		int	    count,
		char	    *chars);
static int
damagePolyText16(DrawablePtr	pDrawable,
		 GCPtr		pGC,
		 int		x,
		 int		y,
		 int		count,
		 unsigned short	*chars);
static void
damageImageText8(DrawablePtr	pDrawable,
		 GCPtr		pGC,
		 int		x,
		 int		y,
		 int		count,
		 char		*chars);
static void
damageImageText16(DrawablePtr	pDrawable,
		  GCPtr		pGC,
		  int		x,
		  int		y,
		  int		count,
		  unsigned short *chars);

#include "../../miext/damage/damage.c"

static int 
damageText (DrawablePtr	    pDrawable,
	    GCPtr	    pGC,
	    int		    x,
	    int		    y,
	    unsigned long   count,
	    char	    *chars,
	    FontEncoding    fontEncoding,
	    Bool	    textType)
{
    CharInfoPtr	    *charinfo;
    CharInfoPtr	    *info;
    unsigned long   i;
    unsigned int    n;
    int		    w;
    Bool	    imageblt;

    imageblt = (textType == TT_IMAGE8) || (textType == TT_IMAGE16);

    charinfo = (CharInfoPtr *) ALLOCATE_LOCAL(count * sizeof(CharInfoPtr));
    if (!charinfo)
	return x;

    GetGlyphs(pGC->font, count, (unsigned char *)chars,
	      fontEncoding, &i, charinfo);
    n = (unsigned int)i;
    w = 0;
    if (!imageblt)
	for (info = charinfo; i--; info++)
	    w += (*info)->metrics.characterWidth;

    if (n != 0) {
	damageDamageChars (pDrawable, pGC->font, x + pDrawable->x, y + pDrawable->y, n,
			   charinfo, imageblt, pGC->subWindowMode);

#ifndef NXAGENT_SERVER

	if (imageblt)
	    (*pGC->ops->ImageGlyphBlt)(pDrawable, pGC, x, y, n, charinfo,
				       FONTGLYPHS(pGC->font));
	else
	    (*pGC->ops->PolyGlyphBlt)(pDrawable, pGC, x, y, n, charinfo,
				      FONTGLYPHS(pGC->font));
#endif

    }
    DEALLOCATE_LOCAL(charinfo);
    return x + w;
}

static int
damagePolyText8(DrawablePtr pDrawable,
		GCPtr	    pGC,
		int	    x,
		int	    y,
		int	    count,
		char	    *chars)
{
    DAMAGE_GC_OP_PROLOGUE(pGC, pDrawable);

    if (checkGCDamage (pDrawable, pGC))
	damageText (pDrawable, pGC, x, y, (unsigned long) count, chars,
		    Linear8Bit, TT_POLY8);

    x = (*pGC->ops->PolyText8)(pDrawable, pGC, x, y, count, chars);

    DAMAGE_GC_OP_EPILOGUE(pGC, pDrawable);
    return x;
}

static int
damagePolyText16(DrawablePtr	pDrawable,
		 GCPtr		pGC,
		 int		x,
		 int		y,
		 int		count,
		 unsigned short	*chars)
{
    DAMAGE_GC_OP_PROLOGUE(pGC, pDrawable);

    if (checkGCDamage (pDrawable, pGC))
	damageText (pDrawable, pGC, x, y, (unsigned long) count, (char *) chars,
		    FONTLASTROW(pGC->font) == 0 ? Linear16Bit : TwoD16Bit,
		    TT_POLY16);

    x = (*pGC->ops->PolyText16)(pDrawable, pGC, x, y, count, chars);

    DAMAGE_GC_OP_EPILOGUE(pGC, pDrawable);
    return x;
}

static void
damageImageText8(DrawablePtr	pDrawable,
		 GCPtr		pGC,
		 int		x,
		 int		y,
		 int		count,
		 char		*chars)
{
    DAMAGE_GC_OP_PROLOGUE(pGC, pDrawable);

    if (checkGCDamage (pDrawable, pGC))
	damageText (pDrawable, pGC, x, y, (unsigned long) count, chars,
		    Linear8Bit, TT_IMAGE8);

    (*pGC->ops->ImageText8)(pDrawable, pGC, x, y, count, chars);

    DAMAGE_GC_OP_EPILOGUE(pGC, pDrawable);
}

static void
damageImageText16(DrawablePtr	pDrawable,
		  GCPtr		pGC,
		  int		x,
		  int		y,
		  int		count,
		  unsigned short *chars)
{
    DAMAGE_GC_OP_PROLOGUE(pGC, pDrawable);

    if (checkGCDamage (pDrawable, pGC))
	damageText (pDrawable, pGC, x, y, (unsigned long) count, (char *) chars,
		    FONTLASTROW(pGC->font) == 0 ? Linear16Bit : TwoD16Bit,
		    TT_IMAGE16);

    (*pGC->ops->ImageText16)(pDrawable, pGC, x, y, count, chars);

    DAMAGE_GC_OP_EPILOGUE(pGC, pDrawable);
}

