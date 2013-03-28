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

#ifndef __Splash_H__
#define __Splash_H__

#include "Windows.h"
#include "X11/Xdmcp.h"
#include "NXalert.h"

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
