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

Copyright 1993 by Davor Matic

Permission to use, copy, modify, distribute, and sell this software
and its documentation for any purpose is hereby granted without fee,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation.  Davor Matic makes no representations about
the suitability of this software for any purpose.  It is provided "as
is" without express or implied warranty.

*/

#ifndef __Font_H__
#define __Font_H__

#include <X11/fonts/fontstruct.h>
#include "resource.h"

extern RESTYPE RT_NX_FONT;

extern int nxagentFontPrivateIndex;

typedef struct
{
  XFontStruct *font_struct;
  char fontName[256];
  XID mirrorID;
} nxagentPrivFont;

extern const int nxagentMaxFontNames;

#define nxagentFontPriv(pFont) \
  ((nxagentPrivFont *)FontGetPrivate(pFont, nxagentFontPrivateIndex))

#define nxagentFontStruct(pFont) (nxagentFontPriv(pFont)->font_struct)

#define nxagentFont(pFont) (nxagentFontStruct(pFont)->fid)

Bool nxagentRealizeFont(ScreenPtr pScreen, FontPtr pFont);
Bool nxagentUnrealizeFont(ScreenPtr pScreen, FontPtr pFont);

void nxagentFreeFontCache(void);

void nxagentListRemoteFonts(const char *searchPattern, const int maxNames);
int nxagentFontLookUp(const char *name);
Bool nxagentFontFind(const char *name, int *pos);
void nxagentListRemoteAddName(const char *name, int status);

int nxagentDestroyNewFontResourceType(void * p, XID id);

Bool nxagentDisconnectAllFonts(void);
Bool nxagentReconnectAllFonts(void *p0);

void nxagentVerifyDefaultFontPath(void);

int nxagentSplitString(char *string, char *fields[], int nfields, char *sep);

#endif /* __Font_H__ */
