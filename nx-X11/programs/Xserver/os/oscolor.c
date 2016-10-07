/***********************************************************

Copyright 1987, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.


Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

#ifdef NX_TRANS_SOCKET

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

/* FIXME: we need more intelligent code (like provided by the nxagentX2go
 * var in hw/nxagent/Init.h) to detect our current runtime mode (running
 * as x2goagent, running as nxagent)
 */
static char* nxAltRgbPaths[] = {"/etc/x2go/rgb", \
                                "/usr/share/x2go/rgb", \
                                "/usr/local/share/x2go/rgb", \
                                "/etc/nxagent/rgb", \
                                "/usr/share/nx/rgb", \
                                "/usr/local/share/nx/rgb", \
                                "/usr/NX/share/rgb", \
                                "/usr/share/X11/rgb", \
                                "/etc/X11/rgb"};
static char _NXRgbPath[1024];

#endif

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifndef USE_RGB_TXT

#ifdef NDBM
#include <ndbm.h>
#else
#ifdef SVR4
#include <rpcsvc/dbm.h>
#else
#include <dbm.h>
#endif
#endif
#include "rgb.h"
#include "os.h"
#include "opaque.h"

/* Note that we are assuming there is only one database for all the screens. */

#ifdef NDBM
DBM *rgb_dbm = (DBM *)NULL;
#else
int rgb_dbm = 0;
#endif

extern void CopyISOLatin1Lowered(
    char * /*dest*/,
    const char * /*source*/,
    int /*length*/);

int
OsInitColors(void)
{
    if (!rgb_dbm)
    {
#ifdef NDBM
	rgb_dbm = dbm_open(rgbPath, 0, 0);
#else
	if (dbminit(rgbPath) == 0)
	    rgb_dbm = 1;
#endif
	if (!rgb_dbm) {
	    ErrorF( "Couldn't open RGB_DB '%s'\n", rgbPath );
	    return FALSE;
	}
    }
    return TRUE;
}

/*ARGSUSED*/
int
OsLookupColor(int screen, char *name, unsigned int len, 
    unsigned short *pred, unsigned short *pgreen, unsigned short *pblue)
{
    datum		dbent;
    RGB			rgb;
    char		buf[64];
    char		*lowername;

    if(!rgb_dbm)
	return(0);

    /* we use malloc here so that we can compile with cc without alloca
     * when otherwise using gcc */
    if (len < sizeof(buf))
	lowername = buf;
    else if (!(lowername = (char *)malloc(len + 1)))
	return(0);
    CopyISOLatin1Lowered ((unsigned char *) lowername, (unsigned char *) name,
			  (int)len);

    dbent.dptr = lowername;
    dbent.dsize = len;
#ifdef NDBM
    dbent = dbm_fetch(rgb_dbm, dbent);
#else
    dbent = fetch (dbent);
#endif

    if (len >= sizeof(buf))
	free(lowername);

    if(dbent.dptr)
    {
	memmove((char *) &rgb, dbent.dptr, sizeof (RGB));
	*pred = rgb.red;
	*pgreen = rgb.green;
	*pblue = rgb.blue;
	return (1);
    }
    return(0);
}

#else /* USE_RGB_TXT */


/*
 * The dbm routines are a porting hassle. This implementation will do
 * the same thing by reading the rgb.txt file directly, which is much
 * more portable.
 */

#include <stdio.h>
#include "os.h"
#include "opaque.h"

#define HASHSIZE 511

typedef struct _dbEntry * dbEntryPtr;
typedef struct _dbEntry {
  dbEntryPtr     link;
  unsigned short red;
  unsigned short green;
  unsigned short blue;
  char           name[1];	/* some compilers complain if [0] */
} dbEntry;


extern void CopyISOLatin1Lowered(
    char * /*dest*/,
    const char * /*source*/,
    int /*length*/);

static dbEntryPtr hashTab[HASHSIZE];

#ifdef NX_TRANS_SOCKET

static int NXVerifyRgbPath(char *path)
{
  int size;
  char *rgbPath;
  struct stat rgbFileStat;

  /*
   * Check if rgb file is present.
   */

  size = strlen(path) + strlen(".txt") + 1;

  rgbPath = (char *) ALLOCATE_LOCAL(size + 1);

  strcpy(rgbPath, path);

  #ifdef NX_TRANS_TEST
  fprintf(stderr, "NXVerifyRgbPath: Looking for [%s] file.\n",
              rgbPath);
  #endif

  if (stat(rgbPath, &rgbFileStat) != 0)
  {

    #ifdef NX_TRANS_TEST
    fprintf(stderr, "NXVerifyRgbPath: Can't find the rgb file [%s].\n",
                rgbPath);
    #endif

    strcat(rgbPath, ".txt");

    #ifdef NX_TRANS_TEST
    fprintf(stderr, "NXVerifyRgbPath: Looking for [%s] file.\n",
                rgbPath);
    #endif

    if (stat(rgbPath, &rgbFileStat) != 0)
    {

      #ifdef NX_TRANS_TEST
      fprintf(stderr, "NXVerifyRgbPath: Can't find the rgb file [%s].\n",
                  rgbPath);
      #endif

      DEALLOCATE_LOCAL(rgbPath);

      return 0;
    }
  }

  #ifdef NX_TRANS_TEST
  fprintf(stderr, "NXVerifyRgbPath: rgb path [%s] is valid.\n",
              path);
  #endif

  DEALLOCATE_LOCAL(rgbPath);

  return 1;
}

static const char *_NXGetRgbPath(const char *path)
{
  const char *systemEnv;
  char rgbPath[1024];
  int numAltRgbPaths;
  int i;

  /*
   * Check the environment only once.
   */

  if (*_NXRgbPath != '\0')
  {
    return _NXRgbPath;
  }

  systemEnv = getenv("NX_SYSTEM");

  if (systemEnv != NULL && *systemEnv != '\0')
  {
    if (strlen(systemEnv) + strlen("/share/rgb") + 1 > 1024)
    {

      #ifdef NX_TRANS_TEST
      fprintf(stderr, "_NXGetRgbPath: WARNING! Maximum length of rgb file path exceeded.\n");
      #endif

      goto _NXGetRgbPathError;
    }

    strcpy(rgbPath, systemEnv);
    strcat(rgbPath, "/share/rgb");

    if (NXVerifyRgbPath(rgbPath) == 1)
    {
      strcpy(_NXRgbPath, systemEnv);
      strcat(_NXRgbPath, "/share/rgb");

      #ifdef NX_TRANS_TEST
      fprintf(stderr, "_NXGetRgbPath: Using rgb file path [%s].\n",
                  _NXRgbPath);
      #endif

      return _NXRgbPath;
    }
  }

  numAltRgbPaths = sizeof(nxAltRgbPaths) / sizeof(*nxAltRgbPaths);

  for (i = 0; i < numAltRgbPaths; i++)
  {
    if (NXVerifyRgbPath(nxAltRgbPaths[i]) == 1)
    {
      if (strlen(nxAltRgbPaths[i]) + 1 > 1024)
      {
        #ifdef NX_TRANS_TEST
        fprintf(stderr, "_NXGetRgbPath: WARNING! Maximum length of rgb file path exceeded.\n");
        #endif

        goto _NXGetRgbPathError;
      }

      strcpy(_NXRgbPath, nxAltRgbPaths[i]);

      #ifdef NX_TRANS_TEST
      fprintf(stderr, "_NXGetRgbPath: Using rgb file path [%s].\n",
                  _NXRgbPath);
      #endif

      return _NXRgbPath;
    }
  }

_NXGetRgbPathError:

  strcpy(_NXRgbPath, path);

  #ifdef NX_TRANS_TEST
  fprintf(stderr, "_NXGetRgbPath: Using default rgb file path [%s].\n",
              _NXRgbPath);
  #endif

  return _NXRgbPath;
}

#endif

static dbEntryPtr
lookup(char *name, int len, Bool create)
{
  unsigned int h = 0, g;
  dbEntryPtr   entry, *prev = NULL;
  char         *str = name;

  if (!(name = (char*)ALLOCATE_LOCAL(len +1))) return NULL;
  CopyISOLatin1Lowered(name, str, len);
  name[len] = '\0';

  for(str = name; *str; str++) {
    h = (h << 4) + *str;
    if ((g = h) & 0xf0000000) h ^= (g >> 24);
    h &= g;
  }
  h %= HASHSIZE;

  if ( (entry = hashTab[h]) )
    {
      for( ; entry; prev = (dbEntryPtr*)entry, entry = entry->link )
	if (! strcmp(name, entry->name) ) break;
    }
  else
    prev = &(hashTab[h]);

  if (!entry && create && (entry = (dbEntryPtr)malloc(sizeof(dbEntry) +len)))
    {
      *prev = entry;
      entry->link = NULL;
      strcpy( entry->name, name );
    }

  DEALLOCATE_LOCAL(name);

  return entry;
}


Bool
OsInitColors(void)
{
  FILE       *rgb;
  char       *path;
  char       line[BUFSIZ];
  char       name[BUFSIZ];
  int        red, green, blue, lineno = 0;
  dbEntryPtr entry;

  static Bool was_here = FALSE;

  if (!was_here)
    {
#ifndef __UNIXOS2__
#ifdef NX_TRANS_SOCKET
      /*
       * Add the trailing '.txt' if a
       * 'rgb' file is not found.
       */

      struct stat statbuf;

      path = (char*)ALLOCATE_LOCAL(strlen(_NXGetRgbPath(rgbPath)) + 5);
      strcpy(path, _NXGetRgbPath(rgbPath));

      if (stat(path, &statbuf) != 0)
      {
          strcat(path, ".txt");
      }
#else
      path = (char*)ALLOCATE_LOCAL(strlen(rgbPath) +5);
      strcpy(path, rgbPath);
      strcat(path, ".txt");
#endif
#else
      char *tmp = (char*)__XOS2RedirRoot(rgbPath);
      path = (char*)ALLOCATE_LOCAL(strlen(tmp) +5);
      strcpy(path, tmp);
      strcat(path, ".txt");
#endif
      if (!(rgb = fopen(path, "r")))
        {
#ifdef NX_TRANS_SOCKET
           ErrorF( "Couldn't open RGB_DB '%s'\n", _NXGetRgbPath(rgbPath));
#else
	   ErrorF( "Couldn't open RGB_DB '%s'\n", rgbPath );
#endif
	   DEALLOCATE_LOCAL(path);
	   return FALSE;
	}

      while(fgets(line, sizeof(line), rgb))
	{
	  lineno++;
#ifndef __UNIXOS2__
	  if (sscanf(line,"%d %d %d %[^\n]\n", &red, &green, &blue, name) == 4)
#else
	  if (sscanf(line,"%d %d %d %[^\n\r]\n", &red, &green, &blue, name) == 4)
#endif
	    {
	      if (red >= 0   && red <= 0xff &&
		  green >= 0 && green <= 0xff &&
		  blue >= 0  && blue <= 0xff)
		{
		  if ((entry = lookup(name, strlen(name), TRUE)))
		    {
		      entry->red   = (red * 65535)   / 255;
		      entry->green = (green * 65535) / 255;
		      entry->blue  = (blue  * 65535) / 255;
		    }
		}
	      else
		ErrorF("Value out of range: %s:%d\n", path, lineno);
	    }
	  else if (*line && *line != '#' && *line != '!')
	    ErrorF("Syntax Error: %s:%d\n", path, lineno);
	}
      
      fclose(rgb);
      DEALLOCATE_LOCAL(path);

      was_here = TRUE;
    }

  return TRUE;
}



Bool
OsLookupColor(int screen, char *name, unsigned int len, 
    unsigned short *pred, unsigned short *pgreen, unsigned short *pblue)
{
  dbEntryPtr entry;

  if ((entry = lookup(name, len, FALSE)))
    {
      *pred   = entry->red;
      *pgreen = entry->green;
      *pblue  = entry->blue;
      return TRUE;
    }

  return FALSE;
}

#endif /* USE_RGB_TXT */
