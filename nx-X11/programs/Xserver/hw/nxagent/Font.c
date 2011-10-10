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

Copyright 1993 by Davor Matic

Permission to use, copy, modify, distribute, and sell this software
and its documentation for any purpose is hereby granted without fee,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation.  Davor Matic makes no representations about
the suitability of this software for any purpose.  It is provided "as
is" without express or implied warranty.

*/

#include "scrnintstr.h"
#include "dixstruct.h"
#include "../../../../include/fonts/font.h"
#include "fontstruct.h"
#include "misc.h"
#include "miscstruct.h"
#include "opaque.h"

#include "Agent.h"

#include "Display.h"
#include "Font.h"
#include "Error.h"

#include <stdio.h>
#include <sys/stat.h>
#include "resource.h"
#include "Reconnect.h"

#include "Args.h"

#include "NXlib.h"
#include "NXalert.h"

#include <string.h>
#include <stdlib.h>

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

#define NXAGENT_DEFAULT_FONT_DIR      "/usr/X11R6/lib/X11/fonts"
#define NXAGENT_ALTERNATE_FONT_DIR    "/usr/share/X11/fonts"
#define NXAGENT_ALTERNATE_FONT_DIR_2  "/usr/share/fonts/X11"
#define NXAGENT_ALTERNATE_FONT_DIR_3  "/usr/share/fonts"
#define NXAGENT_ALTERNATE_FONT_DIR_4  "/usr/NX/share/fonts"

#define NXAGENT_DEFAULT_FONT_PATH  \
"/usr/X11R6/lib/X11/fonts/misc/,/usr/X11R6/lib/X11/fonts/Speedo/,\
/usr/X11R6/lib/X11/fonts/Type1/,/usr/X11R6/lib/X11/fonts/75dpi/,\
/usr/X11R6/lib/X11/fonts/100dpi/,/usr/X11R6/lib/X11/fonts/TTF/,\
/usr/NX/share/fonts/base"

#define NXAGENT_ALTERNATE_FONT_PATH  \
"/usr/share/X11/fonts/misc/,/usr/share/X11/fonts/Speedo/,\
/usr/share/X11/fonts/Type1/,/usr/share/X11/fonts/75dpi/,\
/usr/share/X11/fonts/100dpi/,/usr/share/X11/fonts/TTF/,\
/usr/NX/share/fonts/base"

#define NXAGENT_ALTERNATE_FONT_PATH_2  \
"/usr/share/fonts/X11/misc/,/usr/share/fonts/X11/Speedo/,\
/usr/share/fonts/X11/Type1/,/usr/share/fonts/X11/75dpi/,\
/usr/share/fonts/X11/100dpi/,/usr/share/fonts/X11/TTF/,\
/usr/NX/share/fonts/base"

#define NXAGENT_ALTERNATE_FONT_PATH_3  \
"/usr/share/fonts/misc/,/usr/share/fonts/Speedo/,\
/usr/share/fonts/Type1/,/usr/share/fonts/75dpi/,\
/usr/share/fonts/100dpi/,/usr/share/fonts/TTF/,\
/usr/NX/share/fonts/base"

#define NXAGENT_ALTERNATE_FONT_PATH_4  \
"/usr/NX/share/fonts/base"

#undef NXAGENT_FONTCACHE_DEBUG
#undef NXAGENT_RECONNECT_FONT_DEBUG
#undef NXAGENT_FONTMATCH_DEBUG

#define FIELDS 14

static int reconnectFlexibility;

static void nxagentCleanCacheAfterReconnect(void);
static void nxagentFontReconnect(FontPtr, XID, pointer);
static XFontStruct *nxagentLoadBestQueryFont(Display* dpy, char *fontName, FontPtr pFont);
static XFontStruct *nxagentLoadQueryFont(register Display *dpy , char *fontName , FontPtr pFont);
int nxagentFreeFont(XFontStruct *fs);
static Bool nxagentGetFontServerPath(char * fontServerPath);

static char * nxagentMakeScalableFontName(const char *fontName, int scalableResolution);

RESTYPE RT_NX_FONT;

#ifdef NXAGENT_RECONNECT_FONT_DEBUG
static void printFontCacheDump(char*);
#endif

typedef struct _nxagentFontRec
{
  char *name;
  int  status;
} nxagentFontRec, *nxagentFontRecPtr;

typedef struct _nxagentFontList
{
  nxagentFontRecPtr *list;
  int length;
  int listSize;
} nxagentFontList, *nxagentFontListPtr;

nxagentFontList nxagentRemoteFontList = {NULL, (int)0, (int)0};

int nxagentFontPrivateIndex;

typedef struct _nxCacheFontEntry
{
  Atom atom;
  XFontStruct *font_struct;
  char *name;
} nxCacheFontEntryRec, *nxCacheFontEntryRecPtr;

static struct _nxagentFontCache
{
  nxCacheFontEntryRecPtr *entry;
  int index;
  int size;
} nxagentFontCache = { NULL, (int) 0, (int) 0 };

#define CACHE_ENTRY_PTR (nxagentFontCache.entry)
#define CACHE_INDEX (nxagentFontCache.index)
#define CACHE_SIZE (nxagentFontCache.size)
#define CACHE_ENTRY(A) (CACHE_ENTRY_PTR[A])
#define CACHE_FSTRUCT(A) (CACHE_ENTRY(A) -> font_struct)
#define CACHE_NAME(A) (CACHE_ENTRY(A) -> name)
#define CACHE_ATOM(A) (CACHE_ENTRY(A) -> atom)

static struct _nxagentFailedToReconnectFonts
{
  FontPtr *font;
  XID *id;
  int size;
  int index;
} nxagentFailedToReconnectFonts = {NULL, NULL, 0, 0};

/*
 * This is used if nxagentFullGeneration is true
 * in CloseDisplay().
 */

void nxagentFreeFontCache(void)
{
  int i;

  #ifdef NXAGENT_FONTCACHE_DEBUG
  fprintf(stderr, "Font: Freeing nxagent font cache\n");
  #endif

  if (CACHE_INDEX == 0)
    return;

  #ifdef NXAGENT_FONTCACHE_DEBUG
  fprintf(stderr, "Font: Freeing nxagent font cache, there are [%d] entries.\n", CACHE_INDEX);
  #endif

  for (i = 0; i < CACHE_INDEX; i++)
  {
    #ifdef NXAGENT_FONTCACHE_DEBUG
    fprintf(stderr, "Font: Freeing nxagent font cache entry [%d] entry pointer is [%p], name [%s]\n",
                i, CACHE_ENTRY(i), CACHE_NAME(i));
    #endif

    if (CACHE_FSTRUCT(i))
    {
      nxagentFreeFont(CACHE_FSTRUCT(i));
    }

    xfree(CACHE_NAME(i));
    xfree(CACHE_ENTRY(i));
  }

  xfree(CACHE_ENTRY_PTR);
  CACHE_ENTRY_PTR = NULL;
  CACHE_INDEX = 0;
  CACHE_SIZE = 0;

  #ifdef NXAGENT_FONTCACHE_DEBUG
  fprintf(stderr, "Font: nxagent font cache fully freed\n");
  #endif

  return;
}

void nxagentListRemoteFonts(const char *searchPattern, const int maxNames)
{
  int i, q, p;

  char **xList;
  int  xLen = 0;

  const char *patterns[] = {"*", "-*-*-*-*-*-*-*-*-*-*-*-*-*-*"};
  int patternsQt = 2;

  if (NXDisplayError(nxagentDisplay) == 1)
  {
    return;
  }

  /*
   * Avoid querying again the remote
   * fonts.
   */

  if (nxagentRemoteFontList.length > 0)
  {
    return;
  }

  /*
   * We can't retrieve the full remote font
   * list with a single query, because the
   * number of dashes in the pattern acts as
   * a rule to select how to search for the
   * font names, so the pattern '*' is useful
   * to retrive the font aliases, while the
   * other one will select the 'real' fonts.
   */

  for (p = 0; p < patternsQt; p++)
  {
    xList = XListFonts(nxagentDisplay, patterns[p], maxNames, &xLen);

    #ifdef NXAGENT_FONTMATCH_DEBUG
    fprintf(stderr, "nxagentListRemoteFonts: NXagent remote list [%s] has %d elements.\n", patterns[p], xLen);
    #endif

    /*
     * Add the ListFont request pattern to the list with
     * the last requested maxnames.
     */

    nxagentListRemoteAddName(searchPattern, maxNames);

    for (i = 0; i < xLen; i++)
    {
      q = 1;

      nxagentListRemoteAddName(xList[i], q);
    }

    XFreeFontNames(xList);
  }

  #ifdef NXAGENT_FONTMATCH_DEBUG

  fprintf(stderr, "nxagentListRemoteFonts: Printing remote font list.\n");

  for (i = 0; i < nxagentRemoteFontList.length; i++)
  {
    fprintf(stderr, "Font# %d, \"%s\"\n", i, nxagentRemoteFontList.list[i]->name);
  }

  fprintf(stderr, "nxagentListRemoteFonts: End of list\n");

  #endif
}

void nxagentListRemoteAddName(const char *name, int status)
{
  int pos;

  if (nxagentFontFind(name, &pos))
  {
     if (nxagentRemoteFontList.list[pos]->status < status)
     {
       nxagentRemoteFontList.list[pos]->status = status;

       #ifdef NXAGENT_FONTMATCH_DEBUG
       fprintf(stderr, "Font: Font# %d, [%s] change status to %s\n",
                   pos, nxagentRemoteFontList.list[pos]->name,nxagentRemoteFontList.list[pos]->status?"OK":"deleted");
       #endif
     }
     return;
  }

  if (nxagentRemoteFontList.length == nxagentRemoteFontList.listSize)
  {
     nxagentRemoteFontList.list = xrealloc(nxagentRemoteFontList.list, sizeof(nxagentFontRecPtr)
                                               * (nxagentRemoteFontList.listSize + 1000));

     if (nxagentRemoteFontList.list == NULL)
     {
         FatalError("Font: remote list memory re-allocation failed!.\n");
     }

     nxagentRemoteFontList.listSize += 1000;
  }

  if (pos < nxagentRemoteFontList.length)
  {
    #ifdef NXAGENT_FONTMATCH_DEBUG
    fprintf(stderr, "Font: Going to move list from %p to %p len = %d!.\n",
                &nxagentRemoteFontList.list[pos], &nxagentRemoteFontList.list[pos+1],
                    (nxagentRemoteFontList.length - pos) * sizeof(nxagentFontRecPtr));
    #endif

    memmove(&nxagentRemoteFontList.list[pos+1],
                &nxagentRemoteFontList.list[pos],
                    (nxagentRemoteFontList.length - pos) * sizeof(nxagentFontRecPtr));
  }

  if ((nxagentRemoteFontList.list[pos] = xalloc(sizeof(nxagentFontRec))))
  {
    nxagentRemoteFontList.list[pos]->name = xalloc(strlen(name) +1);
    if (nxagentRemoteFontList.list[pos]->name == NULL)
    {
       fprintf(stderr, "Font: remote list name memory allocation failed!.\n");
       return;
    }
  }
  else
  {
     fprintf(stderr, "Font: remote list record memory allocation failed!.\n");
     return;
  }
  strcpy(nxagentRemoteFontList.list[pos]->name,name);
  nxagentRemoteFontList.list[pos]->status = status;
  nxagentRemoteFontList.length++;

  #ifdef NXAGENT_FONTMATCH_DEBUG
  fprintf(stderr, "Font: remote font list added [%s] in position [%d] as %s !.\n",
              name, pos, status ? "OK" : "deleted");
  fprintf(stderr, "Font: remote font list total len is [%d] Size is [%d] !.\n",
              nxagentRemoteFontList.length, nxagentRemoteFontList.listSize);
  #endif
}

static void nxagentFreeRemoteFontList(nxagentFontList *listRec)
{
  int l;

  for (l = 0; l < listRec -> length; l++)
  {
    if (listRec -> list[l])
    {
      xfree(listRec -> list[l] -> name);
      listRec -> list[l] -> name = NULL;

      xfree(listRec -> list[l]);
      listRec -> list[l] = NULL;
    }
  }

  listRec -> length = listRec -> listSize = 0;

  free(listRec -> list);
  listRec -> list = NULL;

  return;
}

Bool nxagentFontFind(const char *name, int *pos)
{
 int low,high,res,iter,lpos;

 if (!nxagentRemoteFontList.length)
 {
    *pos=0;
    return False;
 }
 low = 0;
 high = nxagentRemoteFontList.length - 1;
 iter = 0;
 res = 1;
 lpos = nxagentRemoteFontList.length;
 while (low <= high)
 {
   *pos = (high + low)/2;
   iter ++;
   res = strcasecmp(nxagentRemoteFontList.list[*pos]->name,name);
   if (res > 0)
   {
      high = *pos - 1;
      lpos = *pos;
      continue;
   }
   else if (res < 0)
   {
      low = *pos + 1;
      lpos = low;
      continue;
   }
   break;
 }
 *pos = (res == 0)?*pos:lpos;

 #ifdef NXAGENT_FONTMATCH_DEBUG
 if (res == 0)
   fprintf(stderr, "Font: font found in %d iterations in pos = %d\n", iter, *pos);
 else
   fprintf(stderr, "Font: not font found in %d iterations insertion pos is = %d\n", iter, *pos);
 #endif

 return (res == 0);

}

Bool nxagentFontLookUp(const char *name)
{
  int i;
  int result;

  char *scalable;

  if (name != NULL && strlen(name) == 0)
  {
    return 0;
  }

  result = nxagentFontFind(name, &i);

  scalable = NULL;

  /*
   * Let's try with the scalable font description.
   */

  if (result == 0)
  {
    scalable = nxagentMakeScalableFontName(name, 0); 

    if (scalable != NULL)
    {
      result = nxagentFontFind(scalable, &i);

      free(scalable);
    }
  }

  /*
   * Let's try again after replacing zero to xdpi and ydpi in the pattern.
   */

  if (result == 0)
  {
    scalable = nxagentMakeScalableFontName(name, 1); 

    if (scalable != NULL)
    {
      result = nxagentFontFind(scalable, &i);

      free(scalable);
    }
  }

  if (result == 0)
  {
    return 0;
  }
  else
  {
    return (nxagentRemoteFontList.list[i]->status > 0);
  }
}

Bool nxagentRealizeFont(ScreenPtr pScreen, FontPtr pFont)
{
  pointer priv;
  Atom name_atom, value_atom;
  int nprops;
  FontPropPtr props;
  int i;
  char *name;
  char *origName = (char*) pScreen;

  FontSetPrivate(pFont, nxagentFontPrivateIndex, NULL);

  if (requestingClient && XpClientIsPrintClient(requestingClient, NULL))
    return True;

  name_atom = MakeAtom("FONT", 4, True);
  value_atom = 0L;

  nprops = pFont->info.nprops;
  props = pFont->info.props;

  for (i = 0; i < nprops; i++)
    if ((Atom)props[i].name == name_atom) {
      value_atom = props[i].value;
      break;
    }

  if (!value_atom) return False;

  name = (char *)NameForAtom(value_atom);

  #ifdef NXAGENT_FONTCACHE_DEBUG
  fprintf(stderr, "Font: nxagentRealizeFont, realizing font: %s\n", validateString(name));
  fprintf(stderr, "                                 atom: %ld\n", value_atom);
  fprintf(stderr, "Font: Cache dump:\n");
  for (i = 0; i < CACHE_INDEX; i++)
  {
      fprintf(stderr, "nxagentFontCache.entry[%d]->name: %s font_struct at %p\n",
                  i, CACHE_NAME(i), CACHE_FSTRUCT(i));
  }
  #endif

  if (!name) return False;

  if ((strcasecmp(origName, name) != 0) && !strchr(origName,'*'))
  {
     #ifdef NXAGENT_FONTMATCH_DEBUG
     fprintf(stderr, "Font: Changing font name to realize from [%s] to [%s]\n",
                 validateString(name), origName);
     #endif

     name = origName;
  }

  priv = (pointer)xalloc(sizeof(nxagentPrivFont));
  FontSetPrivate(pFont, nxagentFontPrivateIndex, priv);

  nxagentFontPriv(pFont) -> mirrorID = 0;

  for (i = 0; i < nxagentFontCache.index; i++)
  {
/*      if (value_atom == CACHE_ATOM(i))*/
     if (strcasecmp(CACHE_NAME(i), name) == 0)
     {
        #ifdef NXAGENT_FONTCACHE_DEBUG
        fprintf(stderr, "Font: nxagentFontCache hit [%s] = [%s]!\n", CACHE_NAME(i), validateString(name));
        #endif

        break;
     }
  }

  if (i < CACHE_INDEX)
  {
      nxagentFontPriv(pFont)->font_struct = CACHE_FSTRUCT(i);
      strcpy(nxagentFontPriv(pFont)->fontName, name);
  }
  else
  {
      #ifdef NXAGENT_FONTCACHE_DEBUG
      fprintf(stderr, "Font: nxagentFontCache fail.\n");
      #endif

      if (CACHE_INDEX == CACHE_SIZE)
      {
        CACHE_ENTRY_PTR = xrealloc(CACHE_ENTRY_PTR, sizeof(nxCacheFontEntryRecPtr) * (CACHE_SIZE + 100));

        if (CACHE_ENTRY_PTR == NULL)
        {
           FatalError("Font: Cache list memory re-allocation failed.\n");
        }

        CACHE_SIZE += 100;
     }

     CACHE_ENTRY(CACHE_INDEX) = xalloc(sizeof(nxCacheFontEntryRec));

     if (CACHE_ENTRY(CACHE_INDEX) == NULL)
     {
        return False;
     }

     CACHE_NAME(CACHE_INDEX) = xalloc(strlen(name) + 1);

     if (CACHE_NAME(CACHE_INDEX) == NULL)
     {
        return False;
     }

     #ifdef NXAGENT_FONTMATCH_DEBUG
     fprintf(stderr, "Font: Going to realize font [%s],[%s] on real X server.\n", validateString(name), origName);
     #endif

     if (nxagentRemoteFontList.length == 0 && (NXDisplayError(nxagentDisplay) == 0))
     {
       nxagentListRemoteFonts("*", nxagentMaxFontNames);
     }

     nxagentFontPriv(pFont)->font_struct = nxagentLoadQueryFont(nxagentDisplay, name, pFont);
     strcpy(nxagentFontPriv(pFont)->fontName, name);
     if (nxagentFontPriv(pFont)->font_struct != NULL)
     {
       CACHE_ATOM(i) = value_atom;
       strcpy(CACHE_NAME(i), name);
       CACHE_FSTRUCT(i) = nxagentFontPriv(pFont)->font_struct;
       CACHE_INDEX++;

       nxagentFontPriv(pFont) -> mirrorID = FakeClientID(serverClient -> index);
       AddResource(nxagentFontPriv(pFont) -> mirrorID, RT_NX_FONT, pFont);

       #ifdef NXAGENT_FONTCACHE_DEBUG
       fprintf(stderr, "Font: nxagentFontCache adds font [%s] in pos. [%d].\n",
                   validateString(name), CACHE_INDEX - 1);
       #endif
     }
  }

  #ifdef NXAGENT_FONTMATCH_DEBUG

  if (nxagentFontPriv(pFont)->font_struct == NULL)
  {
    if (nxagentFontLookUp(name) == False)
    {
      fprintf(stderr, "Font: nxagentRealizeFont failed with font Font=%s, not in our remote list\n",
                  validateString(name));
    }
    else
    {
      fprintf(stderr, "Font: nxagentRealizeFont failed with font Font=%s but the font is in our remote list\n",
                  validateString(name));
    }
  }
  else
      fprintf(stderr, "Font: nxagentRealizeFont OK realizing font Font=%s\n",
                  validateString(name));

  #endif

  return (nxagentFontPriv(pFont)->font_struct != NULL);
}

Bool nxagentUnrealizeFont(ScreenPtr pScreen, FontPtr pFont)
{
  if (nxagentFontPriv(pFont))
  {
    if (NXDisplayError(nxagentDisplay) == 0)
    {
      if (nxagentFontStruct(pFont))
      {
         int i;

         for (i = 0; i < CACHE_INDEX; i++)
         {
           if (CACHE_FSTRUCT(i) == nxagentFontStruct(pFont))
           {
             #ifdef NXAGENT_FONTCACHE_DEBUG
             fprintf(stderr, "nxagentUnrealizeFont: Not freeing the font in cache.\n");
             #endif

             break;
           }
         }

         if (i == CACHE_INDEX)
         {
           /*
            * This font is not in the cache.
            */

           #ifdef NXAGENT_FONTCACHE_DEBUG
           fprintf(stderr, "nxagentUnrealizeFont: Freeing font not found in cache '%d'\n",
                       CACHE_ATOM(i));
           #endif

           XFreeFont(nxagentDisplay, nxagentFontStruct(pFont));
         }
       }
    }

    if (nxagentFontPriv(pFont) -> mirrorID)
      FreeResource(nxagentFontPriv(pFont) -> mirrorID, RT_NONE);

    xfree(nxagentFontPriv(pFont));
    FontSetPrivate(pFont, nxagentFontPrivateIndex, NULL);
  }

  return True;
}

int nxagentDestroyNewFontResourceType(pointer p, XID id)
{
  #ifdef TEST
  fprintf(stderr, "nxagentDestroyNewFontResourceType: Destroying mirror id [%ld] for font at [%p].\n",
              nxagentFontPriv((FontPtr) p) -> mirrorID, (void *) p);
  #endif

/*
FIXME: It happens that this resource had been already
       destroied. We should verify if the same font is
       assigned both to the server client and another
       client. We had a crash when freeing server client
       resources.
*/
  if (nxagentFontPriv((FontPtr) p) != NULL)
  {
    nxagentFontPriv((FontPtr) p) -> mirrorID = None;
  }

  return 1;
}

static XFontStruct *nxagentLoadBestQueryFont(Display* dpy, char *fontName, FontPtr pFont)
{
  XFontStruct *fontStruct;

  char *substFontBuf;

  /*  X Logical Font Description Conventions
   *  require 14 fields in the font names.
   *
   */
  char *searchFields[FIELDS+1];
  char *fontNameFields[FIELDS+1];
  int i;
  int j;
  int numSearchFields = 0;
  int numFontFields = 0;
  int weight = 0;
  int tempWeight = 1;
  int fieldOrder[14] = {  4,  /* Slant       */
                         11,  /* Spacing     */
                         12,  /* Width info  */
                         13,  /* Charset     */
                         14,  /* Language    */
                          7,  /* Height      */
                          6,  /* Add-style   */
                          3,  /* Weight      */
                          2,  /* Name        */
                          1,  /* Foundry     */
                          9,  /* DPI_x       */
                          5,  /* Set-width   */
                          8,  /* Point size  */
                         10   /* DPI_y       */
                       }; 

  #ifdef NXAGENT_RECONNECT_FONT_DEBUG
  fprintf(stderr, "nxagentLoadBestQueryFont: Searching font '%s' .\n", fontName);
  #endif

  substFontBuf = (char *) xalloc(sizeof(char) * 512);


  numFontFields = nxagentSplitString(fontName, fontNameFields, FIELDS + 1, "-");

  memcpy(substFontBuf, "fixed\0", strlen("fixed") + 1);

  if (numFontFields <= FIELDS)
  {
    #ifdef WARNING
    if (nxagentVerbose == 1)
    {
      fprintf(stderr, "nxagentLoadBestQueryFont: WARNING! Font name in non standard format. \n");
    }
    #endif
  }
  else
  {
    for (i = 1 ; i < nxagentRemoteFontList.length ; i++)
    {
      numSearchFields = nxagentSplitString(nxagentRemoteFontList.list[i]->name, searchFields, FIELDS+1, "-");


      /* The following code attemps to find an accurate approximation
       * of the missing font. The current candidate and the missing font are
       * compared on the 14 fields of the X Logical Font Description Convention.
       * The selection is performed by the analysis of the matching fields,
       * shifting left the value of the Weight variable on the right matches
       * and shifting right the value of the Weight on the wrong ones;
       * due a probability of overmuch right shifting, the starting weight is set
       * to a high value. At the end of matching the selected font is the one
       * with the bigger final Weight. The shift operation has been used instead
       * of other operation for a performance issue.
       * In some check the shift is performed by more than one position, because
       * of the relevance of the field; for example a correct slant or a matching
       * charset is more relevant than the size.
       */

      if (numSearchFields > FIELDS)
      {

        tempWeight = 0;

        for (j = 0; j < FIELDS; j++)
        {
          if (strcasecmp(searchFields[fieldOrder[j]], fontNameFields[fieldOrder[j]]) == 0 ||
                  strcmp(searchFields[fieldOrder[j]], "") == 0 ||
                      strcmp(fontNameFields[fieldOrder[j]], "") != 0 ||
                          strcmp(searchFields[fieldOrder[j]], "*") == 0 ||
                              strcmp(fontNameFields[fieldOrder[j]], "*") == 0)
          {
            tempWeight ++;
          }

          tempWeight <<= 1;
        }

      }

      if (tempWeight > weight)
      {
        /* Found more accurate font  */

        weight = tempWeight;
        memcpy(substFontBuf, nxagentRemoteFontList.list[i]->name, strlen(nxagentRemoteFontList.list[i]->name));
        substFontBuf[strlen(nxagentRemoteFontList.list[i]->name)] = '\0';

        #ifdef NXAGENT_RECONNECT_FONT_DEBUG
        fprintf(stderr, "nxagentLoadBestQueryFont: Weight '%d' of more accurate font '%s' .\n", weight, substFontBuf);
        #endif
      }

      for (j = 0; j < numSearchFields; j++)
      {
        if (searchFields[j] != NULL)
        {
          free(searchFields[j]);
        }
      }
    }
  }

  #ifdef WARNING
  if (nxagentVerbose == 1)
  {
    fprintf(stderr, "nxagentLoadBestQueryFont: WARNING! Failed to load font '%s'. Replacing with '%s'.\n",
                fontName, substFontBuf);
  }
  #endif

  fontStruct = nxagentLoadQueryFont(dpy, substFontBuf, pFont);

  free (substFontBuf);

  for (j = 0; j < numFontFields; j++)
  {
    if (fontNameFields[j] != NULL)
    {
      free(fontNameFields[j]);
    }
  }

  return fontStruct;
}

static void nxagentFontDisconnect(FontPtr pFont, XID param1, pointer param2)
{
  nxagentPrivFont *privFont;
  Bool *pBool = (Bool*)param2;
  int i;

  if (pFont == NULL || !*pBool)
    return;

  privFont = nxagentFontPriv(pFont);

  #ifdef NXAGENT_RECONNECT_FONT_DEBUG
  fprintf(stderr, "nxagentFontDisconnect: pFont %p, XID %lx\n",
              (void *) pFont, privFont -> font_struct ? nxagentFont(pFont) : 0);
  #endif

  for (i = 0; i < CACHE_INDEX; i++)
  {
    if (strcasecmp(CACHE_NAME(i), privFont -> fontName) == 0)
    {
      #ifdef NXAGENT_RECONNECT_FONT_DEBUG
      fprintf(stderr, "nxagentFontDisconnect: font %s found in cache at position %d\n",
              privFont -> fontName, i);
      #endif

      privFont -> font_struct = NULL;
      return;
    }
  }

  #ifdef NXAGENT_RECONNECT_FONT_DEBUG
  fprintf(stderr, "nxagentFontDisconnect: WARNING font %s not found in cache freeing it now\n",
              privFont -> fontName);
  #endif

  if (privFont -> font_struct)
  {
     XFreeFont(nxagentDisplay, privFont -> font_struct);
     privFont -> font_struct = NULL;
  }
}

static void nxagentCollectFailedFont(FontPtr fpt, XID id)
{

  if (nxagentFailedToReconnectFonts.font == NULL)
  {
    nxagentFailedToReconnectFonts.size = 8;

    nxagentFailedToReconnectFonts.font = malloc(nxagentFailedToReconnectFonts.size *
                                                    sizeof(FontPtr));

    nxagentFailedToReconnectFonts.id = malloc(nxagentFailedToReconnectFonts.size * sizeof(XID));

    if (nxagentFailedToReconnectFonts.font == NULL || nxagentFailedToReconnectFonts.id == NULL)
    {
      FatalError("Font: font not reconnected memory allocation failed!.\n");
    }

    #ifdef NXAGENT_RECONNECT_FONT_DEBUG
    fprintf(stderr, "nxagentCollectFailedFont: allocated [%d] bytes.\n",
                8 * (sizeof(FontPtr)+ sizeof(XID)));
    #endif
  }
  else if (nxagentFailedToReconnectFonts.index == nxagentFailedToReconnectFonts.size - 1)
  {
    nxagentFailedToReconnectFonts.size *= 2;

    nxagentFailedToReconnectFonts.font = realloc(nxagentFailedToReconnectFonts.font,
                                                     nxagentFailedToReconnectFonts.size *
                                                         sizeof(FontPtr));

    nxagentFailedToReconnectFonts.id = realloc(nxagentFailedToReconnectFonts.id,
                                                   nxagentFailedToReconnectFonts.size *
                                                         sizeof(XID));

    if (nxagentFailedToReconnectFonts.font == NULL || nxagentFailedToReconnectFonts.id == NULL)
    {
      FatalError("Font: font not reconnected memory re-allocation failed!.\n");
    }

    #ifdef NXAGENT_RECONNECT_FONT_DEBUG
    fprintf(stderr,"nxagentCollectFailedFont: reallocated memory.\n ");
    #endif
  }

  nxagentFailedToReconnectFonts.font[nxagentFailedToReconnectFonts.index] = fpt;

  nxagentFailedToReconnectFonts.id[nxagentFailedToReconnectFonts.index] = id;

  #ifdef NXAGENT_RECONNECT_FONT_DEBUG
  fprintf(stderr, "nxagentCollectFailedFont: font not reconnected at [%p], "
              "put in nxagentFailedToReconnectFonts.font[%d] = [%p], with XID = [%lu].\n",
              (void*) fpt, nxagentFailedToReconnectFonts.index,
              (void *)nxagentFailedToReconnectFonts.font[nxagentFailedToReconnectFonts.index],
              nxagentFailedToReconnectFonts.id[nxagentFailedToReconnectFonts.index]);
  #endif

  nxagentFailedToReconnectFonts.index++;
}

static void nxagentFontReconnect(FontPtr pFont, XID param1, pointer param2)
{
  int i;
  nxagentPrivFont *privFont;
  Bool *pBool = (Bool*)param2;

  if (pFont == NULL)
    return;

  privFont = nxagentFontPriv(pFont);

  #ifdef NXAGENT_RECONNECT_FONT_DEBUG
  fprintf(stderr, "nxagentFontReconnect: pFont %p - XID %lx - name %s\n",
              (void*) pFont, (privFont -> font_struct) ? nxagentFont(pFont) : 0,
                  privFont -> fontName);
  #endif

  for (i = 0; i < CACHE_INDEX; i++)
  {
    if (strcasecmp(CACHE_NAME(i), privFont -> fontName) == 0)
    {
      #ifdef NXAGENT_RECONNECT_FONT_DEBUG
      fprintf(stderr, "\tfound in cache");
      #endif

      if (!CACHE_FSTRUCT(i))
      {
        #ifdef NXAGENT_RECONNECT_FONT_DEBUG
        fprintf(stderr, " --- font struct not valid\n");
        #endif

        break;
      }

      nxagentFontStruct(pFont) = CACHE_FSTRUCT(i);

      return;
    }
  }

  if (i == CACHE_INDEX)
  {
    FatalError("nxagentFontReconnect: font not found in cache.");
  }

  privFont -> font_struct = nxagentLoadQueryFont(nxagentDisplay, privFont -> fontName, pFont);

  if ((privFont -> font_struct == NULL) && reconnectFlexibility)
  {
    privFont -> font_struct = nxagentLoadBestQueryFont(nxagentDisplay, privFont -> fontName, pFont);
  }

  if (privFont->font_struct != NULL)
  {
    #ifdef NXAGENT_RECONNECT_FONT_DEBUG
    fprintf(stderr, "\tXID %lx\n", privFont -> font_struct -> fid);
    #endif

    CACHE_FSTRUCT(i) = privFont -> font_struct;
  }
  else
  {
    #ifdef NXAGENT_RECONNECT_FONT_DEBUG
    fprintf(stderr, "nxagentFontReconnect: failed\n");
    #endif

    nxagentCollectFailedFont(pFont, param1);

    #ifdef NXAGENT_RECONNECT_FONT_DEBUG
    fprintf(stderr, "nxagentFontReconnect: reconnection of font [%s] failed.\n",
                privFont -> fontName);
    #endif

    nxagentSetReconnectError(FAILED_RESUME_FONTS_ALERT,
                                 "Couldn't restore the font '%s'", privFont -> fontName);

    *pBool = False;
  }

  return;
}

static void nxagentFreeCacheBeforeReconnect(void)
{
  int i;

  #ifdef NXAGENT_RECONNECT_FONT_DEBUG
  printFontCacheDump("nxagentFreeCacheBeforeReconnect");
  #endif

  for (i = 0; i < CACHE_INDEX; i++)
  {
    if (CACHE_FSTRUCT(i))
    {
      nxagentFreeFont(CACHE_FSTRUCT(i));
      CACHE_FSTRUCT(i) = NULL;
    }
  }
}

static void nxagentCleanCacheAfterReconnect(void)
{
  int i, j;
  int real_size = CACHE_INDEX;
  nxCacheFontEntryRecPtr swapEntryPtr;

  #ifdef NXAGENT_RECONNECT_FONT_DEBUG
  printFontCacheDump("nxagentCleanCacheAfterReconnect");
  #endif

  for (i = 0; i < CACHE_INDEX; i++)
  {
    if(CACHE_FSTRUCT(i) == NULL)
    {
      XFree(CACHE_NAME(i));
      real_size--;
    }
  }

  for (i = 0; i < real_size; i++)
  {
      /* Find - first bad occurrence if exist. */
      while ((i < real_size) && CACHE_FSTRUCT(i)) i++;

      /* Really nothing more to do. */
      if (i == real_size)
        break;

      /*
       * Find - first good occurrence (moving backward from right end) entry in
       *        order to replace the bad one.
       */
      for (j = CACHE_INDEX - 1; CACHE_FSTRUCT(j) == NULL; j--);

      /*
       * Now we can swap the two entry
       * and reduce the Cache index
       */
      swapEntryPtr = CACHE_ENTRY(i);
      CACHE_ENTRY(i) = CACHE_ENTRY(j);
      CACHE_ENTRY(j) = swapEntryPtr;
  }

  CACHE_INDEX = real_size;
}

#ifdef NXAGENT_RECONNECT_FONT_DEBUG
static void printFontCacheDump(char* msg)
{
  int i;

  fprintf(stderr, "%s - begin -\n", msg);

  for (i = 0; i < CACHE_INDEX; i++)
  {
    if (CACHE_FSTRUCT(i))
    {
      fprintf(stderr, "\tXID %lx - %s\n", CACHE_FSTRUCT(i) -> fid, CACHE_NAME(i));
    }
    else
    {
      fprintf(stderr, "\tdestroyed   - %s\n", CACHE_NAME(i));
    }
  }
  fprintf(stderr, "%s - end   -\n", msg);
}
#endif

Bool nxagentReconnectAllFonts(void *p0)
{
  int cid;
  Bool fontSuccess = True;

  reconnectFlexibility = *((int *) p0);

  #if defined(NXAGENT_RECONNECT_DEBUG) || defined(NXAGENT_RECONNECT_FONT_DEBUG)
  fprintf(stderr, "nxagentReconnectAllFonts\n");
  #endif

  /*
   * The resource type RT_NX_FONT is created on the
   * server client only, so we can avoid to loop
   * through all the clients.
   */

  FindClientResourcesByType(clients[serverClient -> index], RT_NX_FONT,
                                (FindResType) nxagentFontReconnect, &fontSuccess);

  for (cid = 0; cid < MAXCLIENTS; cid++)
  {
    if (clients[cid])
    {
      FindClientResourcesByType(clients[cid], RT_FONT,
                                    (FindResType) nxagentFontReconnect, &fontSuccess);
    }
  }

  if (fontSuccess)
  {
    nxagentCleanCacheAfterReconnect();
  }

  return fontSuccess;
}

static void nxagentFailedFontReconnect(FontPtr pFont, XID param1, pointer param2)
{
  int i;
  nxagentPrivFont *privFont;
  Bool *pBool = (Bool*)param2;

  if (pFont == NULL)
    return;

  privFont = nxagentFontPriv(pFont);

  #ifdef NXAGENT_RECONNECT_FONT_DEBUG
  fprintf(stderr, "nxagentFailedFontReconnect: pFont %p - XID %lx - name %s\n",
              (void*) pFont, (privFont -> font_struct) ? nxagentFont(pFont) : 0,
                  privFont -> fontName);
  #endif

  for (i = 0; i < CACHE_INDEX; i++)
  {
    if (strcasecmp(CACHE_NAME(i), privFont -> fontName) == 0)
    {
      #ifdef NXAGENT_RECONNECT_FONT_DEBUG
      fprintf(stderr, "\tfound in cache");
      #endif

      if (!CACHE_FSTRUCT(i))
      {
        #ifdef NXAGENT_RECONNECT_FONT_DEBUG
        fprintf(stderr, " --- font struct not valid\n");
        #endif

        break;
      }

      nxagentFontStruct(pFont) = CACHE_FSTRUCT(i);

      return;
    }
  }

  if (i == CACHE_INDEX)
  {
    FatalError("nxagentFailedFontReconnect: font not found in cache.");
  }

  privFont -> font_struct = nxagentLoadQueryFont(nxagentDisplay, privFont -> fontName, pFont);

  if (privFont -> font_struct == NULL)
  {
    privFont -> font_struct = nxagentLoadBestQueryFont(nxagentDisplay, privFont -> fontName, pFont);
  }

  if (privFont->font_struct != NULL)
  {
    #ifdef NXAGENT_RECONNECT_FONT_DEBUG
    fprintf(stderr, "\tXID %lx\n", privFont -> font_struct -> fid);
    #endif

    CACHE_FSTRUCT(i) = privFont -> font_struct;
  }
  else
  {
    #ifdef NXAGENT_RECONNECT_FONT_DEBUG
    fprintf(stderr, "nxagentFailedFontReconnect: failed\n");
    #endif

    #ifdef NXAGENT_RECONNECT_FONT_DEBUG
    fprintf(stderr, "nxagentFailedFontReconnect: reconnection of font [%s] failed.\n",
                privFont -> fontName);
    #endif

    nxagentSetReconnectError(FAILED_RESUME_FONTS_ALERT,
                                 "Couldn't restore the font '%s'", privFont -> fontName);

    *pBool = False;
  }

  return;
}

static void nxagentFreeFailedToReconnectFonts()
{
  if (nxagentFailedToReconnectFonts.font != NULL)
  {
    free(nxagentFailedToReconnectFonts.font);
    nxagentFailedToReconnectFonts.font = NULL;
  }

  if (nxagentFailedToReconnectFonts.id != NULL)
  {
    free(nxagentFailedToReconnectFonts.id);
    nxagentFailedToReconnectFonts.id = NULL;
  }

  nxagentFailedToReconnectFonts.size = 0;
  nxagentFailedToReconnectFonts.index = 0;
}

Bool nxagentReconnectFailedFonts(void *p0)
{
  int i;
  int attempt = 1;
  const int maxAttempt = 5;

  char **fontPaths, **localFontPaths, **newFontPaths;
  char fontServerPath[256] = "";
  int nPaths = 0;

  Bool repeat = True;
  Bool fontSuccess = True;

  reconnectFlexibility = *((int *) p0);

  #ifdef NXAGENT_RECONNECT_FONT_DEBUG
  fprintf(stderr, "nxagentReconnectFailedFonts: \n");
  #endif

  if (nxagentGetFontServerPath(fontServerPath) == False)
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentReconnectFailedFonts: WARNING! "
                "Font server tunneling not retrieved.\n");
    #endif
  }

  #ifdef NXAGENT_RECONNECT_FONT_DEBUG
  fprintf(stderr, "nxagentReconnectFailedFonts: font server path [%s]\n", fontServerPath);
  #endif

  fontPaths = XGetFontPath(nxagentDisplay, &nPaths);

  if ((newFontPaths =  malloc((nPaths + 1) * sizeof(char *))) == NULL)
  {
    FatalError("nxagentReconnectFailedFonts: malloc failed.");
  }

  memcpy(newFontPaths, fontPaths, nPaths * sizeof(char*));

  localFontPaths = newFontPaths;
  localFontPaths += nPaths;
  *localFontPaths = fontServerPath;

  while(repeat)
  {
    #ifdef NXAGENT_RECONNECT_FONT_DEBUG
    fprintf(stderr, "nxagentReconnectFailedFonts: attempt [%d].\n", attempt);
    #endif

    repeat = False;

    XSetFontPath(nxagentDisplay, newFontPaths, nPaths  + 1);
    nxagentFreeRemoteFontList(&nxagentRemoteFontList);
    nxagentListRemoteFonts("*", nxagentMaxFontNames);

    for(i = 0; i < nxagentFailedToReconnectFonts.index; i++)
    {
      fontSuccess = True;

      if(nxagentFailedToReconnectFonts.font[i])
      {
        nxagentFailedFontReconnect(nxagentFailedToReconnectFonts.font[i],
                                       nxagentFailedToReconnectFonts.id[i],
                                           &fontSuccess);

        if (fontSuccess)
        {
          nxagentFailedToReconnectFonts.font[i] = NULL;
        }
        else
        {
          repeat = True;
        }

      }
    }

    attempt++;

    if (attempt > maxAttempt)
    {
      nxagentFreeFailedToReconnectFonts();

      XSetFontPath(nxagentDisplay, fontPaths, nPaths);
      nxagentFreeRemoteFontList(&nxagentRemoteFontList);
      nxagentListRemoteFonts("*", nxagentMaxFontNames);

      XFreeFontPath(fontPaths);
      free(newFontPaths);

      return False;
    }
  }

  nxagentFreeFailedToReconnectFonts();

  XSetFontPath(nxagentDisplay, fontPaths, nPaths);

  XFreeFontPath(fontPaths);
  free(newFontPaths);

  nxagentCleanCacheAfterReconnect();

  return True;
}

Bool nxagentDisconnectAllFonts()
{
  int cid;
  Bool fontSuccess = True;

  #if defined(NXAGENT_RECONNECT_DEBUG) || defined(NXAGENT_RECONNECT_FONT_DEBUG)
  fprintf(stderr, "nxagentDisconnectAllFonts\n");
  #endif

  nxagentFreeRemoteFontList(&nxagentRemoteFontList);
  nxagentFreeCacheBeforeReconnect();

  /*
   * The resource type RT_NX_FONT is created on the
   * server client only, so we can avoid to loop
   * through all the clients.
   */

  FindClientResourcesByType(clients[serverClient -> index], RT_NX_FONT,
                                (FindResType) nxagentFontDisconnect, &fontSuccess);

  for(cid = 0; cid < MAXCLIENTS; cid++)
  {
    if( clients[cid] && fontSuccess )
    {
      FindClientResourcesByType(clients[cid], RT_FONT,
                                    (FindResType) nxagentFontDisconnect, &fontSuccess);
    }
  }

  return True;
}

static Bool nxagentGetFontServerPath(char * fontServerPath)
{
  char path[256];

  if (NXGetFontParameters(nxagentDisplay, 256, path) == True)
  {
    if (*path != '\0')
    {
      strncpy(fontServerPath, path + 1, *path);

      *(fontServerPath + *path) = '\0';

      #ifdef TEST
      fprintf(stderr, "nxagentGetFontServerPath: Got path [%s].\n",
                  fontServerPath);
      #endif
    }
    else
    {
      #ifdef TEST
      fprintf(stderr, "nxagentGetFontServerPath: WARNING! Font server tunneling not enabled.\n");
      #endif

      return False;
    }
  }
  else
  {
    #ifdef TEST
    fprintf(stderr, "nxagentGetFontServerPath: WARNING! Failed to get path for font server tunneling.\n");
    #endif

    return False;
  }

  return True;
}

void nxagentVerifyDefaultFontPath(void)
{
  struct stat dirStat;
  static char *fontPath;

  #ifdef TEST
  fprintf(stderr, "nxagentVerifyDefaultFontPath: Going to search for one or more valid font paths.\n");
  #endif

  fontPath = malloc(strlen(defaultFontPath) + 1);

  if (fontPath == NULL)
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentVerifyDefaultFontPath: WARNING! Unable to allocate memory for a new font path. "
                "Using the default font path [%s].\n", validateString(defaultFontPath));
    #endif

    return;
  }

  /*
   * Set the default font path as the first choice.
   */

  strcpy(fontPath, defaultFontPath);

  if (stat(NXAGENT_DEFAULT_FONT_DIR, &dirStat) == 0 &&
          S_ISDIR(dirStat.st_mode) != 0)
  {
    /*
     * Let's use the old "/usr/X11R6/lib/X11/fonts" style.
     */

    #ifdef TEST
    fprintf(stderr, "nxagentVerifyDefaultFontPath: Assuming fonts in directory [%s].\n",
                validateString(NXAGENT_DEFAULT_FONT_DIR));
    #endif

    if (*fontPath != '\0')
    {
      fontPath = realloc(fontPath, strlen(fontPath) + strlen(NXAGENT_DEFAULT_FONT_PATH) + 2);
      strcat(fontPath, ",");
    }
    else
    {
      fontPath = realloc(fontPath, strlen(fontPath) + strlen(NXAGENT_DEFAULT_FONT_PATH) + 1);
    }

    strcat(fontPath, NXAGENT_DEFAULT_FONT_PATH);
  }

  if (stat(NXAGENT_ALTERNATE_FONT_DIR, &dirStat) == 0 &&
          S_ISDIR(dirStat.st_mode) != 0)
  {
    /*
     * Let's use the new "/usr/share/X11/fonts" path.
     */

    #ifdef TEST
    fprintf(stderr, "nxagentVerifyDefaultFontPath: Assuming fonts in directory [%s].\n",
                validateString(NXAGENT_ALTERNATE_FONT_DIR));
    #endif

    if (*fontPath != '\0')
    {
      fontPath = realloc(fontPath, strlen(fontPath) + strlen(NXAGENT_ALTERNATE_FONT_PATH) + 2);
      strcat(fontPath, ",");
    }
    else
    {
      fontPath = realloc(fontPath, strlen(fontPath) + strlen(NXAGENT_ALTERNATE_FONT_PATH) + 1);
    }

    strcat(fontPath, NXAGENT_ALTERNATE_FONT_PATH);
  }

  if (stat(NXAGENT_ALTERNATE_FONT_DIR_2, &dirStat) == 0 &&
          S_ISDIR(dirStat.st_mode) != 0)
  {
    /*
     * Let's use the "/usr/share/fonts/X11" path.
     */

    #ifdef TEST
    fprintf(stderr, "nxagentVerifyDefaultFontPath: Assuming fonts in directory [%s].\n",
                validateString(NXAGENT_ALTERNATE_FONT_DIR_2));
    #endif

    if (*fontPath != '\0')
    {
      fontPath = realloc(fontPath, strlen(fontPath) + strlen(NXAGENT_ALTERNATE_FONT_PATH_2) + 2);
      strcat(fontPath, ",");
    }
    else
    {
      fontPath = realloc(fontPath, strlen(fontPath) + strlen(NXAGENT_ALTERNATE_FONT_PATH_2) + 1);
    }

    strcat(fontPath, NXAGENT_ALTERNATE_FONT_PATH_2);
  }

  if (stat(NXAGENT_ALTERNATE_FONT_DIR_3, &dirStat) == 0 &&
          S_ISDIR(dirStat.st_mode) != 0)
  {
    /*
     * Let's use the "/usr/share/fonts" path.
     */

    #ifdef TEST
    fprintf(stderr, "nxagentVerifyDefaultFontPath: Assuming fonts in directory [%s].\n",
                validateString(NXAGENT_ALTERNATE_FONT_DIR_3));
    #endif

    if (*fontPath != '\0')
    {
      fontPath = realloc(fontPath, strlen(fontPath) + strlen(NXAGENT_ALTERNATE_FONT_PATH_3) + 2);
      strcat(fontPath, ",");
    }
    else
    {
      fontPath = realloc(fontPath, strlen(fontPath) + strlen(NXAGENT_ALTERNATE_FONT_PATH_3) + 1);
    }

    strcat(fontPath, NXAGENT_ALTERNATE_FONT_PATH_3);
  }

  if (stat(NXAGENT_ALTERNATE_FONT_DIR_4, &dirStat) == 0 &&
          S_ISDIR(dirStat.st_mode) != 0)
  {
    /*
     * Let's use the "/usr/NX/share/fonts" path.
     */

    #ifdef TEST
    fprintf(stderr, "nxagentVerifyDefaultFontPath: Assuming fonts in directory [%s].\n",
                validateString(NXAGENT_ALTERNATE_FONT_DIR_4));
    #endif

    if (*fontPath != '\0')
    {
      fontPath = realloc(fontPath, strlen(fontPath) + strlen(NXAGENT_ALTERNATE_FONT_PATH_4) + 2);
      strcat(fontPath, ",");
    }
    else
    {
      fontPath = realloc(fontPath, strlen(fontPath) + strlen(NXAGENT_ALTERNATE_FONT_PATH_4) + 1);
    }

    strcat(fontPath, NXAGENT_ALTERNATE_FONT_PATH_4);
  }

  if (*fontPath == '\0') 
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentVerifyDefaultFontPath: WARNING! Can't find a valid font directory.\n");

    fprintf(stderr, "nxagentVerifyDefaultFontPath: WARNING! Using font path [%s].\n",
                validateString(defaultFontPath));
    #endif
  }
  else
  {
    defaultFontPath = fontPath;
 
    #ifdef TEST
    fprintf(stderr, "nxagentVerifyDefaultFontPath: Using font path [%s].\n",
                validateString(defaultFontPath));
    #endif
 }

  return;
}

XFontStruct* nxagentLoadQueryFont(register Display *dpy, char *name, FontPtr pFont)
{
  XFontStruct* fs;
  xCharInfo *xcip;

  fs = (XFontStruct *) malloc (sizeof (XFontStruct));

  if (fs == NULL)
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentLoadQueryFont: WARNING! Failed allocation of XFontStruct.\n");
    #endif

    return (XFontStruct *)NULL;
  }

    #ifdef NXAGENT_RECONNECT_FONT_DEBUG
    fprintf(stderr, "nxagentLoadQueryFont: Looking for font '%s'.\n", name);
    #endif

  if (nxagentFontLookUp(name) == 0)
  {
    #ifdef DEBUG
    fprintf(stderr, "nxagentLoadQueryFont: WARNING! Font not found '%s'.\n", name);
    #endif

    Xfree(fs);

    return (XFontStruct *) NULL;
  }

  fs -> ext_data           = NULL;                      /* Hook for extension to hang data.*/
  fs -> fid                = XLoadFont(dpy, name);      /* Font id for this font. */
  fs -> direction          = pFont->info.drawDirection; /* Hint about the direction font is painted. */
  fs -> min_char_or_byte2  = pFont->info.firstCol;      /* First character. */
  fs -> max_char_or_byte2  = pFont->info.lastCol;       /* Last character. */
  fs -> min_byte1          = pFont->info.firstRow;      /* First row that exists. */
  fs -> max_byte1          = pFont->info.lastRow;       /* Last row that exists. */
  fs -> all_chars_exist    = pFont->info.allExist;      /* Flag if all characters have nonzero size. */
  fs -> default_char       = pFont->info.defaultCh;     /* Char to print for undefined character. */
  fs -> n_properties       = pFont->info.nprops;        /* How many properties there are. */

  /*
   * If no properties defined for the font, then it is bad
   * font, but shouldn't try to read nothing.
   */

  if (fs -> n_properties > 0)
  {
    register long nbytes;

    nbytes = pFont -> info.nprops * sizeof(XFontProp);
    fs -> properties = (XFontProp *) Xalloc((unsigned) nbytes);

    if (fs -> properties == NULL)
    {
      #ifdef WARNING
      fprintf(stderr, "nxagentLoadQueryFont: WARNING! Failed allocation of XFontProp.");
      #endif

      Xfree((char *) fs);
      return (XFontStruct *) NULL;
    }

    memmove(fs -> properties, pFont -> info.props, nbytes);
  }

  xcip = (xCharInfo *) &pFont -> info.ink_minbounds;

  fs -> min_bounds.lbearing      = cvtINT16toShort(xcip -> leftSideBearing);
  fs -> min_bounds.rbearing      = cvtINT16toShort(xcip -> rightSideBearing);
  fs -> min_bounds.width         = cvtINT16toShort(xcip -> characterWidth);
  fs -> min_bounds.ascent        = cvtINT16toShort(xcip -> ascent);
  fs -> min_bounds.descent       = cvtINT16toShort(xcip -> descent);
  fs -> min_bounds.attributes    = xcip -> attributes;

  xcip = (xCharInfo *) &pFont -> info.ink_maxbounds;

  fs -> max_bounds.lbearing      = cvtINT16toShort(xcip -> leftSideBearing);
  fs -> max_bounds.rbearing      = cvtINT16toShort(xcip -> rightSideBearing);
  fs -> max_bounds.width         = cvtINT16toShort(xcip -> characterWidth);
  fs -> max_bounds.ascent        = cvtINT16toShort(xcip -> ascent);
  fs -> max_bounds.descent       = cvtINT16toShort(xcip -> descent);
  fs -> max_bounds.attributes    = xcip -> attributes;

  fs -> per_char           = NULL;                              /* First_char to last_char information. */
  fs -> ascent             = pFont->info.fontAscent;            /* Logical extent above baseline for spacing. */
  fs -> descent            = pFont->info.fontDescent;           /* Logical decent below baseline for spacing. */

  return fs;
}

int nxagentFreeFont(XFontStruct *fs)
{

  if (fs -> per_char)
  {
    #ifdef USE_XF86BIGFONT
    _XF86BigfontFreeFontMetrics(fs);
    #else
    Xfree ((char *) fs->per_char);
    #endif
  }

  if (fs -> properties)
  {
    Xfree (fs->properties);
  }

  XFree(fs);

  return 1;
}


int nxagentSplitString(char *string, char *fields[], int nfields, char *sep)
{
  int seplen;
  int fieldlen;
  int last;
  int len;
  int i;

  char *current;
  char *next;

  seplen = strlen(sep);
  len = strlen(string);

  current = string;

  i = 0;
  last = 0;

  for (;;)
  {
    next = NULL;

    if (current < string + len)
    {
      next = strstr(current, sep);
    }

    if (next == NULL)
    {
      next = string + len;
      last = 1;
    }

    fieldlen = next - current;

    if (i < nfields)
    {
      fields[i] = (char *) malloc(fieldlen + 1);
      strncpy(fields[i], current, fieldlen);
      *(fields[i] + fieldlen) = 0;
    }
    else
    {
      fields[i] = NULL;
    }

    current = next + seplen;

    i++;

    if (last == 1)
    {
      break;
    }
  }

  return i;
}

char *nxagentMakeScalableFontName(const char *fontName, int scalableResolution)
{
  char *scalableFontName;
  const char *s;
  int len;
  int field;

  len = strlen(fontName) + 1;

  scalableFontName = malloc(len);

  if (scalableFontName == NULL)
  {
    #ifdef PANIC
    fprintf(stderr, "nxagentMakeScalableFontName: PANIC! malloc() failed.\n");
    #endif

    return NULL;
  }

  scalableFontName[0] = 0;

  if (*fontName != '-')
  {
    goto MakeScalableFontNameError;
  }

  s = fontName;

  field = 0;

  while (s != NULL)
  {
    s = strchr(s + 1, '-');

    if (s != NULL)
    {
      if (field == 6 || field == 7 || field == 11)
      {
        /*
         * PIXEL_SIZE || POINT_SIZE || AVERAGE_WIDTH
         */

        strcat(scalableFontName, "-0");
      }
      else if (scalableResolution == 1 && (field == 8 || field == 9))
      {
        /*
         * RESOLUTION_X || RESOLUTION_Y
         */

        strcat(scalableFontName, "-0");
      }
      else
      {
        strncat(scalableFontName, fontName, s - fontName);
      }

      fontName = s;
    }
    else
    {
      strcat(scalableFontName, fontName);
    }

    field++;
  }

  if (field != 14)
  {
    goto MakeScalableFontNameError;
  }

  return scalableFontName;

MakeScalableFontNameError:

  free(scalableFontName);

  #ifdef DEBUG
  fprintf(stderr, "nxagentMakeScalableFontName: Invalid font name.\n");
  #endif

  return NULL;
}
