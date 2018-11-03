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

#include "../../render/glyph.c"

#ifdef NXAGENT_SERVER

#include "Render.h"

#define PANIC
#define WARNING
#undef  DEBUG
#undef  TEST

#endif

GlyphRefPtr
FindGlyphRef (GlyphHashPtr hash, CARD32 signature, Bool match, GlyphPtr compare)
{
    CARD32	elt, step, s;
    GlyphPtr	glyph;
    GlyphRefPtr	table, gr, del;
    CARD32	tableSize = hash->hashSet->size;

    table = hash->table;
    elt = signature % tableSize;
    step = 0;
    del = 0;
    for (;;)
    {
	gr = &table[elt];
	s = gr->signature;
	glyph = gr->glyph;
	if (!glyph)
	{
	    if (del)
		gr = del;
	    break;
	}
	if (glyph == DeletedGlyph)
	{
	    if (!del)
		del = gr;
	    else if (gr == del)
		break;
	}
#ifdef NXAGENT_SERVER
	else if (s == signature && match && glyph->size != compare->size)
        {
          /*
           * if the glyphsize is different there's no need to do a memcmp
           * because it will surely report difference. And even worse:
           * it will read beyond the end of glyph under some
           * circumstances, which can be detected when compiling with
           * -fsanitize=address.
           */
        }
#endif
	else if (s == signature &&
		 (!match ||
		  memcmp (&compare->info, &glyph->info, compare->size) == 0))
	{
	    break;
	}
	if (!step)
	{
	    step = signature % hash->hashSet->rehash;
	    if (!step)
		step = 1;
	}
	elt += step;
	if (elt >= tableSize)
	    elt -= tableSize;
    }
    return gr;
}

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
	free (glyph);
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

GlyphPtr
FindGlyph (GlyphSetPtr glyphSet, Glyph id)
{
    GlyphPtr    glyph;

#ifdef NXAGENT_SERVER
    GlyphRefPtr gr = FindGlyphRef (&glyphSet->hash, id, FALSE, 0);
    glyph = gr -> glyph;
#else
    glyph = FindGlyphRef (&glyphSet->hash, id, FALSE, 0)->glyph;
#endif
    if (glyph == DeletedGlyph)
    {
        glyph = 0;
    }
#ifdef NXAGENT_SERVER
    else if (gr -> corruptedGlyph == 1)
    {
        #ifdef DEBUG
        fprintf(stderr, "FindGlyphRef: Going to synchronize the glyph [%p] for glyphset [%p].\n",
                 (void *) glyph, (void *) glyphSet);
        #endif

        nxagentAddGlyphs(glyphSet, &id, &(glyph -> info), 1,
                         (CARD8*)(glyph + 1), glyph -> size - sizeof(xGlyphInfo));
    }
#endif

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

                CARD32 c = hash->table[i].corruptedGlyph;

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
	free (hash->table);
    }
    *hash = newHash;
    if (global)
	CheckDuplicates (hash, "ResizeGlyphHash bottom");
    return TRUE;
}
