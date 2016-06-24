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
 * $XFree86: xc/programs/Xserver/render/glyph.c,v 1.5 2001/01/30 07:01:22 keithp Exp $
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

#include "../../render/glyph.c"

#ifdef NXAGENT_SERVER

#include "Render.h"

#define PANIC
#define WARNING
#undef  DEBUG
#undef  TEST

#endif

void
AddGlyph (GlyphSetPtr glyphSet, GlyphPtr glyph, Glyph id)
{
    GlyphRefPtr	    gr;
    CARD32	    hash;

    CheckDuplicates (&globalGlyphs[glyphSet->fdepth], "AddGlyph top global");
    /* Locate existing matching glyph */
    hash = HashGlyph (glyph);
    gr = FindGlyphRef (&globalGlyphs[glyphSet->fdepth], hash, TRUE, glyph);
    if (gr->glyph && gr->glyph != DeletedGlyph)
    {
	xfree (glyph);
	glyph = gr->glyph;
    }
    else
    {
	gr->glyph = glyph;
	gr->signature = hash;
	globalGlyphs[glyphSet->fdepth].tableEntries++;
    }
 
    /* Insert/replace glyphset value */
    gr = FindGlyphRef (&glyphSet->hash, id, FALSE, 0);
    ++glyph->refcnt;
    if (gr->glyph && gr->glyph != DeletedGlyph)
	FreeGlyph (gr->glyph, glyphSet->fdepth);
    else
	glyphSet->hash.tableEntries++;
    gr->glyph = glyph;
    gr->signature = id;

    #ifdef NXAGENT_SERVER

    gr -> corruptedGlyph = 1;

    #endif

    CheckDuplicates (&globalGlyphs[glyphSet->fdepth], "AddGlyph bottom");
}

GlyphPtr FindGlyph (GlyphSetPtr glyphSet, Glyph id)
{
  GlyphRefPtr gr;
  GlyphPtr    glyph;

  gr = FindGlyphRef (&glyphSet->hash, id, FALSE, 0);
  glyph = gr -> glyph;

  if (glyph == DeletedGlyph)
  {
    glyph = 0;
  }
  else if (gr -> corruptedGlyph == 1)
  {
     #ifdef DEBUG
     fprintf(stderr, "FindGlyphRef: Going to synchronize the glyph [%p] for glyphset [%p].\n",
                 (void *) glyph, (void *) glyphSet);
     #endif

    nxagentAddGlyphs(glyphSet, &id, &(glyph -> info), 1,
                         (CARD8*)(glyph + 1), glyph -> size - sizeof(xGlyphInfo));
  }

  return glyph;
}

Bool
ResizeGlyphHash (GlyphHashPtr hash, CARD32 change, Bool global)
{
    CARD32	    tableEntries;
    GlyphHashSetPtr hashSet;
    GlyphHashRec    newHash;
    GlyphRefPtr	    gr;
    GlyphPtr	    glyph;
    int		    i;
    int		    oldSize;
    CARD32	    s;

    #ifdef NXAGENT_SERVER

    CARD32          c;

    #endif

    tableEntries = hash->tableEntries + change;
    hashSet = FindGlyphHashSet (tableEntries);
    if (hashSet == hash->hashSet)
	return TRUE;
    if (global)
	CheckDuplicates (hash, "ResizeGlyphHash top");
    if (!AllocateGlyphHash (&newHash, hashSet))
	return FALSE;
    if (hash->table)
    {
	oldSize = hash->hashSet->size;
	for (i = 0; i < oldSize; i++)
	{
	    glyph = hash->table[i].glyph;
	    if (glyph && glyph != DeletedGlyph)
	    {
		s = hash->table[i].signature;

                #ifdef NXAGENT_SERVER

                c = hash->table[i].corruptedGlyph;

                #endif

		gr = FindGlyphRef (&newHash, s, global, glyph);
		gr->signature = s;
		gr->glyph = glyph;

                #ifdef NXAGENT_SERVER

                gr -> corruptedGlyph = c;

                #endif

		++newHash.tableEntries;
	    }
	}
	xfree (hash->table);
    }
    *hash = newHash;
    if (global)
	CheckDuplicates (hash, "ResizeGlyphHash bottom");
    return TRUE;
}
