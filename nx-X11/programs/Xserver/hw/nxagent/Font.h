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

#ifndef __Font_H__
#define __Font_H__

#include "fontstruct.h"
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

int nxagentDestroyNewFontResourceType(pointer p, XID id);

Bool nxagentDisconnectAllFonts(void);
Bool nxagentReconnectAllFonts(void *p0);

void nxagentVerifyDefaultFontPath(void);

int nxagentSplitString(char *string, char *fields[], int nfields, char *sep);

#endif /* __Font_H__ */
