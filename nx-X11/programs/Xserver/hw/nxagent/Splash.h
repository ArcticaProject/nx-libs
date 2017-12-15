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

#ifndef __Splash_H__
#define __Splash_H__

#include "Windows.h"
#include "X11/Xdmcp.h"
#include <nx/NXalert.h>

#define XDM_TIMEOUT       20000

extern xdmcp_states XdmcpState;
extern int XdmcpTimeOutRtx;
extern int XdmcpStartTime;
extern int nxagentXdmcpUp;

extern int nxagentLogoDepth;
extern int nxagentLogoWhite;
extern int nxagentLogoRed;
extern int nxagentLogoBlack;
extern int nxagentLogoGray;

extern Window nxagentSplashWindow;

extern int nxagentWMPassed;

extern int nxagentShowSplashWindow(Window);

extern void nxagentRemoveSplashWindow(WindowPtr pWin);

#endif /* __Splash_H__ */
