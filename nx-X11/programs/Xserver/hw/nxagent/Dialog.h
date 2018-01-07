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

#ifndef __Dialog_H__
#define __Dialog_H__

#include "X11/X.h"

typedef enum
{
  DIALOG_FIRST_TAG,
  DIALOG_KILL_SESSION = DIALOG_FIRST_TAG,
  DIALOG_SUSPEND_SESSION,
  DIALOG_ROOTLESS,
  DIALOG_PULLDOWN,
  DIALOG_FONT_REPLACEMENT,
  DIALOG_ENABLE_DESKTOP_RESIZE_MODE,
  DIALOG_DISABLE_DESKTOP_RESIZE_MODE,
  DIALOG_FAILED_RECONNECTION,
  DIALOG_ENABLE_DEFER_MODE,
  DIALOG_DISABLE_DEFER_MODE,
  DIALOG_LAST_TAG

} DialogType;

extern int nxagentKillDialogPid;
extern int nxagentSuspendDialogPid;
extern int nxagentRootlessDialogPid;
extern int nxagentPulldownDialogPid;
extern int nxagentFontsReplacementDialogPid;
extern int nxagentEnableRandRModeDialogPid;
extern int nxagentDisableRandRModeDialogPid;
extern int nxagentEnableDeferModePid;
extern int nxagentDisableDeferModePid;

#define NXAGENTFAILEDRECONNECTIONMESSAGELENGTH 256
extern char nxagentFailedReconnectionMessage[NXAGENTFAILEDRECONNECTIONMESSAGELENGTH];

#define NXAGENTPULLDOWNWINDOWLENGTH 16
extern char nxagentPulldownWindow[NXAGENTPULLDOWNWINDOWLENGTH];

extern void nxagentLaunchDialog(DialogType type);
extern void nxagentResetDialog(int pid);
extern void nxagentTerminateDialog(DialogType type);
extern void nxagentFailedReconnectionDialog(int alert, char *error);
extern void nxagentPulldownDialog(Window);
extern void nxagentTerminateDialogs(void);

#define nxagentNoDialogIsRunning \
    (nxagentSuspendDialogPid == 0 && \
         nxagentKillDialogPid == 0 && \
             nxagentEnableRandRModeDialogPid == 0 && \
                 nxagentDisableRandRModeDialogPid == 0 && \
                     nxagentEnableDeferModePid == 0 && \
                         nxagentDisableDeferModePid == 0)

#define DECODE_DIALOG_TYPE(type) \
            ((type) == DIALOG_KILL_SESSION ? "DIALOG_KILL_SESSION" : \
             (type) == DIALOG_SUSPEND_SESSION ? "DIALOG_SUSPEND_SESSION" : \
             (type) == DIALOG_ROOTLESS ? "DIALOG_ROOTLESS" : \
             (type) == DIALOG_PULLDOWN ? "DIALOG_PULLDOWN" : \
             (type) == DIALOG_FONT_REPLACEMENT ? "DIALOG_FONT_REPLACEMENT" : \
             (type) == DIALOG_ENABLE_DESKTOP_RESIZE_MODE ? "DIALOG_ENABLE_DESKTOP_RESIZE_MODE" :\
             (type) == DIALOG_DISABLE_DESKTOP_RESIZE_MODE ? "DIALOG_DISABLE_DESKTOP_RESIZE_MODE" :\
             (type) == DIALOG_FAILED_RECONNECTION ? "DIALOG_FAILED_RECONNECTION" : \
             (type) == DIALOG_ENABLE_DEFER_MODE ? "DIALOG_ENABLE_DEFER_MODE" : \
             (type) == DIALOG_DISABLE_DEFER_MODE ? "DIALOG_DISABLE_DEFER_MODE" : \
             "UNKNOWN_DIALOG")

/*
 * Message to be showed to users when the close
 * button is pressed. The right message is chosen
 * according if session does or does not run in
 * persistent mode.
 */

#define DIALOG_KILL_SESSION_MESSAGE \
\
"\
Do you really want to close the session?\
"

#define DIALOG_KILL_SESSION_TYPE "yesno"

#define DIALOG_KILL_SESSION_LOCAL 0


#define DIALOG_SUSPEND_SESSION_MESSAGE \
\
"\
Press the disconnect button to disconnect the running session.\n\
You will be able to resume the session at later time. Press the\n\
terminate button to exit the session and close all the running\n\
programs.\
"

#define DIALOG_SUSPEND_SESSION_TYPE "yesnosuspend"

#define DIALOG_SUSPEND_SESSION_LOCAL 0


#define DIALOG_ROOTLESS_MESSAGE \
\
"\
All remote applications have been terminated.\n\
Do you want to close the session?\
"

#define DIALOG_ROOTLESS_TYPE "yesno"

#define DIALOG_ROOTLESS_LOCAL 0


#define DIALOG_PULLDOWN_MESSAGE \
\
nxagentPulldownWindow

#define DIALOG_PULLDOWN_TYPE "pulldown"

#define DIALOG_PULLDOWN_LOCAL 0


#define DIALOG_FONT_REPLACEMENT_MESSAGE \
\
"\
Unable to retrieve all the fonts currently in use. \n\
Missing fonts have been replaced.\
"

#define DIALOG_FONT_REPLACEMENT_TYPE "ok"

#define DIALOG_FONT_REPLACEMENT_LOCAL 0


#define DIALOG_FAILED_RECONNECTION_MESSAGE \
\
nxagentFailedReconnectionMessage

#define DIALOG_FAILED_RECONNECTION_TYPE "ok"

#define DIALOG_FAILED_RECONNECTION_LOCAL 0


#define DIALOG_ENABLE_DESKTOP_RESIZE_MODE_MESSAGE \
\
"\
The session is now running in desktop resize mode.\n\
You can resize the desktop by simply dragging the\n\
desktop window's border. You can press Ctrl+Alt+R\n\
again to disable this option.\
"

#define DIALOG_ENABLE_DESKTOP_RESIZE_MODE_TYPE "ok"

#define DIALOG_ENABLE_DESKTOP_RESIZE_MODE_LOCAL 0

#define DIALOG_DISABLE_DESKTOP_RESIZE_MODE_MESSAGE \
\
"\
The session is now running in viewport mode. You can\n\
navigate across different areas of the desktop window\n\
by dragging the desktop with the mouse or by using the\n\
arrows keys while pressing Ctrl+Alt. Press Ctrl+Alt+R\n\
again to return to the desktop resize mode.\
"

#define DIALOG_DISABLE_DESKTOP_RESIZE_MODE_TYPE "ok"

#define DIALOG_DISABLE_DESKTOP_RESIZE_MODE_LOCAL 0


#define DIALOG_ENABLE_DEFER_MODE_MESSAGE \
\
"\
Deferred screen updates are now enabled. You can press\n\
Ctrl+Alt+E again to disable this option.\
"

#define DIALOG_ENABLE_DEFER_MODE_TYPE "ok"

#define DIALOG_ENABLE_DEFER_MODE_LOCAL 0


#define DIALOG_DISABLE_DEFER_MODE_MESSAGE \
\
"\
Deferred screen updates are now disabled. You can press\n\
Ctrl+Alt+E to enable it again.\
"

#define DIALOG_DISABLE_DEFER_MODE_TYPE "ok"

#define DIALOG_DISABLE_DEFER_MODE_LOCAL 0

#endif /* __Dialog_H__ */

