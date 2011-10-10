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
 * Create the NX_CUT_BUFFER_CLIENT atom and
 * initialize the required property to exchange
 * data with the X server.
 */

extern int nxagentInitClipboard(WindowPtr pWindow);

/*
 * Called whenever a client or a window is
 * destroyed to let the clipboard code to
 * release any pointer to the referenced
 * structures.
 */

extern void nxagentClearClipboard(ClientPtr pClient, WindowPtr pWindow);

extern void nxagentSetSelectionOwner(Selection *pSelection);
extern int nxagentConvertSelection(ClientPtr client, WindowPtr pWin, Atom selection,
                                      Window requestor, Atom property, Atom target, Time time);

void nxagentClearSelection();
void nxagentRequestSelection();
void nxagentNotifySelection();

#endif /* __Clipboard_H__ */
