/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2010 NoMachine, http://www.nomachine.com/.         */
/*                                                                        */
/* NXCOMP, NX protocol compression and NX extensions to this software     */
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

#ifndef NXalert_H
#define NXalert_H

#define ALERT_CAPTION_PREFIX                                 "NX - "

#define INTERNAL_ERROR_ALERT                                 1
#define INTERNAL_ERROR_ALERT_TYPE                            "error"
#define INTERNAL_ERROR_ALERT_STRING                          \
"\
An unrecoverable internal error was detected.\n\
Press OK to terminate the current session.\n\
"

#define CLOSE_DEAD_X_CONNECTION_CLIENT_ALERT                 2
#define CLOSE_DEAD_X_CONNECTION_CLIENT_ALERT_TYPE            "yesno"
#define CLOSE_DEAD_X_CONNECTION_CLIENT_ALERT_STRING          \
"\
One of the applications currently in use is not responding.\n\
Do you want to terminate the current session?\n\
"

#define CLOSE_DEAD_X_CONNECTION_SERVER_ALERT                 3
#define CLOSE_DEAD_X_CONNECTION_SERVER_ALERT_TYPE            "yesno"
#define CLOSE_DEAD_X_CONNECTION_SERVER_ALERT_STRING          \
"\
One of the applications did not behave correctly and caused\n\
the X server to stop responding in a timely fashion. Do you\n\
want to terminate the current session?\n\
"

#define CLOSE_DEAD_PROXY_CONNECTION_CLIENT_ALERT             4
#define CLOSE_DEAD_PROXY_CONNECTION_CLIENT_ALERT_TYPE        NULL
#define CLOSE_DEAD_PROXY_CONNECTION_CLIENT_ALERT_STRING      NULL

#define CLOSE_DEAD_PROXY_CONNECTION_SERVER_ALERT             5
#define CLOSE_DEAD_PROXY_CONNECTION_SERVER_ALERT_TYPE        "yesno"
#define CLOSE_DEAD_PROXY_CONNECTION_SERVER_ALERT_STRING      \
"\
No response received from the remote server.\n\
Do you want to terminate the current session?\n\
"

#define RESTART_DEAD_PROXY_CONNECTION_CLIENT_ALERT           6
#define RESTART_DEAD_PROXY_CONNECTION_CLIENT_ALERT_TYPE      NULL
#define RESTART_DEAD_PROXY_CONNECTION_CLIENT_ALERT_STRING    NULL

#define RESTART_DEAD_PROXY_CONNECTION_SERVER_ALERT           7
#define RESTART_DEAD_PROXY_CONNECTION_SERVER_ALERT_TYPE      "yesno"
#define RESTART_DEAD_PROXY_CONNECTION_SERVER_ALERT_STRING    \
"\
Connection with remote server was shut down. NX will try\n\
to establish a new server connection. Session could have\n\
been left in a unusable state. Do you want to terminate\n\
the session?\n\
"

#define CLOSE_UNRESPONSIVE_X_SERVER_ALERT                    8
#define CLOSE_UNRESPONSIVE_X_SERVER_ALERT_TYPE               "panic"
#define CLOSE_UNRESPONSIVE_X_SERVER_ALERT_STRING             \
"\
You pressed the key sequence CTRL+ALT+SHIFT+ESC.\n\
This is probably because your X server has become\n\
unresponsive. Session will be terminated in 30\n\
seconds unless you abort the procedure by pressing\n\
the Cancel button.\n\
"

#define WRONG_PROXY_VERSION_ALERT                            9
#define WRONG_PROXY_VERSION_ALERT_TYPE                       "ok"
#define WRONG_PROXY_VERSION_ALERT_STRING                     \
"\
Local NX libraries version " VERSION " do not match the NX\n\
version of the remote server. Please check the error\n\
log on the server to find out which client version you\n\
need to install to be able to access this server.\n\
"

#define FAILED_PROXY_CONNECTION_CLIENT_ALERT                 10
#define FAILED_PROXY_CONNECTION_CLIENT_ALERT_TYPE            NULL
#define FAILED_PROXY_CONNECTION_CLIENT_ALERT_STRING          NULL

#define FAILED_PROXY_CONNECTION_SERVER_ALERT                 11
#define FAILED_PROXY_CONNECTION_SERVER_ALERT_TYPE            "yesno"
#define FAILED_PROXY_CONNECTION_SERVER_ALERT_STRING          \
"\
Could not yet establish the connection to the remote\n\
proxy. Do you want to terminate the current session?\n\
"

#define MISSING_PROXY_CACHE_ALERT                            12
#define MISSING_PROXY_CACHE_ALERT_TYPE                       "ok"
#define MISSING_PROXY_CACHE_ALERT_STRING                     \
"\
NX was unable to negotiate a cache for this session.\n\
This may happen if this is the first time you run a\n\
session on this server or if cache was corrupted or\n\
produced by an incompatible NX version.\n\
"

#define ABORT_PROXY_CONNECTION_ALERT                         13
#define ABORT_PROXY_CONNECTION_ALERT_TYPE                    "ok"
#define ABORT_PROXY_CONNECTION_ALERT_STRING                  \
"\
The connection with the remote server was shut down.\n\
Please check the state of your network connection.\n\
"

/*
 * The one below is a special alert, used to close
 * a previous alert that is running on the given
 * side. This can be used to get rid of a message
 * that has ceased to hold true.
 */

#define DISPLACE_MESSAGE_ALERT                               14
#define DISPLACE_MESSAGE_ALERT_TYPE                          NULL
#define DISPLACE_MESSAGE_ALERT_STRING                        NULL

/*
 * These are the other alert messages that were
 * added in the 1.5.0 release. The first is never
 * shown and is intended just for testing.
 */

#define GREETING_MESSAGE_ALERT                               15
#define GREETING_MESSAGE_ALERT_TYPE                          "ok"
#define GREETING_MESSAGE_ALERT_STRING                        \
"\
Welcome to NX from the NoMachine team. We really\n\
hope you will enjoy this wonderful software as much\n\
as we had fun making it ;-).\n\
"

/*
 * These alerts are intended to notify the user
 * of the reason why the agent failed to resume
 * the session.
 */

#define START_RESUME_SESSION_ALERT                           16
#define START_RESUME_SESSION_ALERT_TYPE                      "ok"
#define START_RESUME_SESSION_ALERT_STRING                    \
"\
You appear to run your NX session across a slow network\n\
connection. Resuming the session may require some time.\n\
Please wait.\
"

#define FAILED_RESUME_DISPLAY_ALERT                          17
#define FAILED_RESUME_DISPLAY_ALERT_TYPE                     "error"
#define FAILED_RESUME_DISPLAY_ALERT_STRING                   \
"\
Failed to open the display. Can't resume the NX\n\
session on this display.\n\
"

#define FAILED_RESUME_DISPLAY_BROKEN_ALERT                   18
#define FAILED_RESUME_DISPLAY_BROKEN_TYPE                    "error"
#define FAILED_RESUME_DISPLAY_BROKEN_STRING                  \
"\
The display connection was broken while trying to\n\
resume the session. Please, check your network\n\
connection and try again.\n\
"

#define FAILED_RESUME_VISUALS_ALERT                          19
#define FAILED_RESUME_VISUALS_ALERT_TYPE                     "error"
#define FAILED_RESUME_VISUALS_ALERT_STRING                   \
"\
Failed to restore all the required visuals.\n\
Can't resume the NX session on this display.\n\
"

#define FAILED_RESUME_COLORMAPS_ALERT                        20
#define FAILED_RESUME_COLORMAPS_ALERT_TYPE                   "error"
#define FAILED_RESUME_COLORMAPS_ALERT_STRING                 \
"\
The number of available colormaps is different\n\
on the new display. Can't resume the NX session\n\
on this display.\n\
"

#define FAILED_RESUME_PIXMAPS_ALERT                          21
#define FAILED_RESUME_PIXMAPS_ALERT_TYPE                     "error"
#define FAILED_RESUME_PIXMAPS_ALERT_STRING                   \
"\
Failed to restore all the required pixmap formats.\n\
Can't resume the NX session on this display.\n\
"

#define FAILED_RESUME_DEPTHS_ALERT                           22
#define FAILED_RESUME_DEPTHS_ALERT_TYPE                      "error"
#define FAILED_RESUME_DEPTHS_ALERT_STRING                    \
"\
Failed to restore all the required screen depths.\n\
Can't resume the NX session on this display.\n\
"

#define FAILED_RESUME_RENDER_ALERT                           23
#define FAILED_RESUME_RENDER_ALERT_TYPE                      "error"
#define FAILED_RESUME_RENDER_ALERT_STRING                    \
"\
The render extension is missing or an incompatible\n\
version was detected on your X server. Can't resume\n\
the NX session on this display.\n\
"

#define FAILED_RESUME_FONTS_ALERT                            24
#define FAILED_RESUME_FONTS_ALERT_TYPE                       "error"
#define FAILED_RESUME_FONTS_ALERT_STRING                     \
"\
One or more of the fonts that are in use by the\n\
session are missing. Can't resume the NX session\n\
on this display.\n\
"

#define ABORT_PROXY_NEGOTIATION_ALERT                        62
#define ABORT_PROXY_NEGOTIATION_ALERT_TYPE                   "ok"
#define ABORT_PROXY_NEGOTIATION_ALERT_STRING                 \
"\
The remote proxy closed the connection while negotiating\n\
the session. This may be due to the wrong authentication\n\
credentials passed to the server.\n\
"

#define ABORT_PROXY_SHUTDOWN_ALERT                           64
#define ABORT_PROXY_SHUTDOWN_ALERT_TYPE                      "ok"
#define ABORT_PROXY_SHUTDOWN_ALERT_STRING                    \
"\
No response received from the remote proxy while\n\
waiting for the session shutdown.\n\
"

#define FAILED_XDMCP_CONNECTION_ALERT                        65
#define FAILED_XDMCP_CONNECTION_ALERT_TYPE                   "ok"
#define FAILED_XDMCP_CONNECTION_ALERT_STRING                 \
"\
The XDM host that was contacted by the NX server doesn't\n\
seem to be able to start the session. Please check your\n\
server configuration.\n\
"

/*
 * Used to handle the backward compatibility.
 * Update the numbers if you add a new alert. 
 */

#define LAST_PROTO_STEP_6_ALERT  63
#define LAST_PROTO_STEP_7_ALERT  65

#endif /* NXalert_H */
