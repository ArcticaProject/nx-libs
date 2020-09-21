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

#ifndef __Clipboard_H__
#define __Clipboard_H__

/*
 * Queried at clipboard initialization.
 */

typedef struct _XFixesAgentInfo
{
  int Opcode;
  int EventBase;
  int ErrorBase;
  int Initialized;
} XFixesAgentInfoRec;

extern XFixesAgentInfoRec nxagentXFixesInfo;

/*
 * Create the NX_SELTRANS_FROM_AGENT atom and initialize the required
 * property to exchange data with the X server.
 */

extern Bool nxagentInitClipboard(WindowPtr pWindow);

/*
 * Called whenever a client or a window is destroyed to let the
 * clipboard code to release any pointer to the referenced structures.
 */

extern void nxagentClearClipboard(ClientPtr pClient, WindowPtr pWindow);

extern int nxagentConvertSelection(ClientPtr client, WindowPtr pWin, Atom selection,
                                      Window requestor, Atom property, Atom target, Time time);

#ifdef XEvent
extern void nxagentHandleSelectionClearFromXServer(XEvent *X);
extern void nxagentHandleSelectionRequestFromXServer(XEvent *X);
extern void nxagentHandleSelectionNotifyFromXServer(XEvent *X);
#else
extern void nxagentHandleSelectionClearFromXServer();
extern void nxagentHandleSelectionRequestFromXServer();
extern void nxagentHandleSelectionNotifyFromXServer();
#endif

extern int nxagentFindCurrentSelectionIndex(Atom sel);
/*
 * Handle the selection property received in the event loop in
 * Events.c.
 */
extern Bool nxagentCollectPropertyEventFromXServer(int resource);

extern WindowPtr nxagentGetClipboardWindow(Atom property);

extern int nxagentSendNotify(xEvent *event);

extern void nxagentDumpClipboardStat(void);

#endif /* __Clipboard_H__ */
