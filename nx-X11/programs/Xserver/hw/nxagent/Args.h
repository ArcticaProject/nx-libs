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

#define NXAGENTSESSIONIDLENGTH 256
#define NXAGENTDISPLAYNAMELENGTH 1024
#define NXAGENTSHADOWDISPLAYNAMELENGTH 1024
#define NXAGENTWINDOWNAMELENGTH 256
#define NXAGENTDIALOGNAMELENGTH 256

extern char nxagentSessionId[NXAGENTSESSIONIDLENGTH];
extern char nxagentDisplayName[NXAGENTDISPLAYNAMELENGTH];
extern char nxagentShadowDisplayName[NXAGENTSHADOWDISPLAYNAMELENGTH];
extern char nxagentWindowName[NXAGENTWINDOWNAMELENGTH];
extern char nxagentDialogName[NXAGENTDIALOGNAMELENGTH];

extern Bool nxagentSynchronize;
extern Bool nxagentRealWindowProp;
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
void nxagentProcessOptions(char * string);
void nxagentProcessOptionsFile(char * filename);

void nxagentSetPackMethod(void);
void nxagentSetDeferLevel(void);
void nxagentSetBufferSize(void);
void nxagentSetScheduler(void);
void nxagentSetCoalescence(void);

extern int nxagentUserDefinedFontPath;

extern int nxagentRemoteMajor;

extern char *nxagentKeystrokeFile;

#endif /* __Args_H__ */
