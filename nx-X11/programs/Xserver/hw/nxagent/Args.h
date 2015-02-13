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

#ifndef __Args_H__
#define __Args_H__

#define MD5_LENGTH  16

struct UserGeometry{
  int flag;
  int X;
  int Y;
  unsigned int Width;
  unsigned int Height;
};

extern Bool nxagentUseNXTrans;

extern char nxagentSessionId[];
extern char nxagentDisplayName[];
extern char nxagentShadowDisplayName[];
extern char nxagentWindowName[];
extern char nxagentDialogName[];

extern Bool nxagentFullGeneration;
extern int nxagentDefaultClass;
extern Bool nxagentUserDefaultClass;
extern int nxagentDefaultDepth;
extern Bool nxagentUserDefaultDepth;
extern int nxagentX;
extern int nxagentY;
extern unsigned int nxagentWidth;
extern unsigned int nxagentHeight;
extern struct UserGeometry nxagentUserGeometry;
extern Bool nxagentUserBorderWidth;
extern int nxagentNumScreens;
extern Bool nxagentDoDirectColormaps;
extern Window nxagentParentWindow;
extern int nxagentMaxAllowedReset;
extern Bool nxagentResizeDesktopAtStartup;
extern Bool nxagentIpaq;

extern int nxagentLockDeferLevel;

Bool nxagentPostProcessArgs(char *name, Display *dpy, Screen *scr);
void nxagentProcessOptionsFile(void);

void nxagentSetPackMethod(void);
void nxagentSetDeferLevel(void);
void nxagentSetBufferSize(void);
void nxagentSetScheduler(void);
void nxagentSetCoalescence(void);

extern int nxagentUserDefinedFontPath;

extern int nxagentRemoteMajor;

extern char *nxagentKeystrokeFile;

#endif /* __Args_H__ */
