/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2011 NoMachine (http://www.nomachine.com)          */
/* Copyright (c) 2008-2017 Oleksandr Shneyder <o.shneyder@phoca-gmbh.de>  */
/* Copyright (c) 2011-2022 Mike Gabriel <mike.gabriel@das-netzwerkteam.de>*/
/* Copyright (c) 2014-2019 Mihai Moldovan <ionic@ionic.de>                */
/* Copyright (c) 2014-2022 Ulrich Sibiller <uli42@gmx.de>                 */
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

#ifndef __Options_H__
#define __Options_H__

/* Bool is defined in Xlib.h but we do not want to include that here, so let's
   clone the definition */
#ifndef Bool
#define Bool int
#endif

#ifndef True
#define True   1
#endif

#ifndef False
#define False  0
#endif

#define UNDEFINED -1
#define COPY_UNLIMITED -1

/* in milliseconds */
#define DEFAULT_SLEEP_TIME_MILLIS 50

extern unsigned int nxagentPrintGeometryFlags;

typedef enum _BackingStoreMode
{
  BackingStoreUndefined = -1,
  BackingStoreNever,
  BackingStoreWhenRequested,
  BackingStoreForce

} BackingStoreMode;

/* since nx 2.0.0-32 clipboard data exchange can be limited. Client
   here means "nxclient":

  Enable or disable copy and paste operations from the user's desktop
  to the NX session or vice versa. This option can take four values:

  client  The content copied on the client can be pasted inside the
          NX session.

  server  The content copied inside the NX session can be pasted
          on the client.

  both    The copy & paste operations are allowed both between the
          client and the NX session and viceversa.

  none    The copy&paste operations between the client and the NX
          session are never allowed.
*/
typedef enum _ClipboardMode
{
  ClipboardBoth,
  ClipboardClient,
  ClipboardServer,
  ClipboardNone

} ClipboardMode;

typedef enum _ClientOsType
{
  ClientOsWinnt = 0,
  ClientOsLinux,
  ClientOsSolaris,
  ClientOsMac

} ClientOsType;

typedef enum _ToleranceChecksMode
{
  ToleranceChecksStrict = 0,
  ToleranceChecksSafe = 1,
  ToleranceChecksRisky = 2,
  ToleranceChecksBypass = 3
} ToleranceChecksMode;


#define DEFAULT_TOLERANCE ToleranceChecksStrict

typedef enum _KeycodeConversion
{
  KeycodeConversionOn = 0,
  KeycodeConversionOff = 1,
  KeycodeConversionAuto = 2
} KeycodeConversionMode;

#define DEFAULT_KEYCODE_CONVERSION KeycodeConversionAuto

/*
 * Set of options affecting agent operations.
 */

typedef struct _AgentOptions
{
  /*
   * Link type of the NX connection or none, if this is a direct X11
   * connection.
   */
  int LinkType;

  /*
   * Is agent running in desktop mode? This is presently the default.
   */
  Bool Desktop;

  /*
   * True if user activated rootless mode.
   */
  Bool Rootless;

  /*
   * True for shadow mode.
   */
  Bool Shadow;

  /*
   * True if user activated persistent mode.
   */
  Bool Persistent;

  /*
   * True if user activated fullscreen mode.
   */
  Bool Fullscreen;

  /*
   * True if the fullscreen NX session will extend on all available
   * screens.
   */
  Bool AllScreens;

  /*
   * Set to the auto-disconnect timeout, if the user activated this
   * feature.
   */
  int Timeout;

  /*
   * Geometry of the agent's window.
   */
  int X, Y, Width, Height;
  int BorderWidth;

  /*
   * Geometry of the agent's window in window mode. Used to restore
   * window size when switching back to window mode from fullscreen.
   */
  int WMBorderWidth;
  int WMTitleHeight;

  int SavedX, SavedY, SavedWidth, SavedHeight;

  int SavedRootWidth, SavedRootHeight;

  /*
   * Set if agent is running nested in another agent X server.
   */
  Bool Nested;

  /*
   * Selected backing-store mode.
   */
  BackingStoreMode BackingStore;

  /*
   * Selected clipboard mode.
   */
  ClipboardMode Clipboard;

  /*
   * transfer TARGETS to remote side or answer with a limited
   * hardcoded text target list
   * Should be Bool but we'd have to include Xlib.h for that
   */
  int TextClipboard;

  /*
   * Enable agent to use the MITSHM extension in path from remote
   * proxy to the real X server.
   */
  Bool SharedMemory;

  /*
   * Enable agent to use shared Pixmaps
   */
  Bool SharedPixmaps;

  /*
   * Enable agent to propagate keyboard and pointer device
   * configuration to the remote X server.
   */
  int DeviceControl;

  /*
   * Explicitly asked config propagation.
   */
  int DeviceControlUserDefined;

  /*
   * Resuming keyboard device corrects keymap if session migrates
   * across platforms with different keycode layout.
   */
  Bool ResetKeyboardAtResume;

  /*
   * Reset server when the last client disconnects.
   */
  Bool Reset;

  /*
   * Geometry of the agent root window, relative to the agent default
   * window.
   */
  int RootX, RootY, RootWidth, RootHeight;

  /*
   * Horizontal and vertical span of the agent viewport.
   */
  int ViewportXSpan, ViewportYSpan;

  /*
   * True if the user can resize the desktop by dragging the window
   * border.
   */
  Bool DesktopResize;

  /*
   * The scaling ratio of the shadow agent.
   */
  int Ratio, XRatio, YRatio;

  float FloatRatio, FloatXRatio, FloatYRatio;

  /*
   * The shadow agent uses the Damage extension.
   */
  Bool UseDamage;

  /*
   * Was the agent run with the -B option?
   */
  Bool Binder;
  char *BinderOptions;

  /*
   * Set if the agent has to connect to a desktop manager to start the
   * session.
   */
  Bool Xdmcp;

  /*
   * Size of the Xlib display buffer. The default is set according to
   * the link type.
   */
  int DisplayBuffer;

  /*
   * Buffer coalescence timeout.
   */
  int DisplayCoalescence;

  /*
   * Use the composite extension when available on the remote display.
   */
  Bool Composite;

  /*
   * If set, don't skip internal operations when the agent window is
   * not fully visible.
   */
  Bool IgnoreVisibility;

  /*
   * If set, prevent the shadow session to interact with master
   * display.
   */
  Bool ViewOnly;

  /*
   * If true select a lossy or lossless compression method based on
   * the characteristics of the image.
   */
  Bool Adaptive;

  /*
   * Stream the images and update the display when the image has been
   * completely transerred.
   */
  Bool Streaming;

  /*
   * Use a lazy approach in updating the remote display. This means
   * delaying the bandwidth consuming graphic operations and
   * synchronizing the screen at idle time.
   */
  int DeferLevel;

  /*
   * Maximum elapsed time before a new full synchronization.
   */
  unsigned long DeferTimeout;

  /*
   * Maximum size of the tile used when sending an image to the remote
   * display.
   */
  int TileWidth, TileHeight;

  /*
   * Enabling/disabling the pulldown menu.
   */
  Bool Menu;

  /*
   * Enabling/disabling the magic pixel.
   */
  Bool MagicPixel;

  /*
   * Specify the Operative System of the client.
   */
  int ClientOs;

  /*
   * Inhibit some XKEYBOARD requests.
   */
  Bool InhibitXkb;

  /*
   * Maximum number of bytes that can be pasted from an NX session
   * into an external application.
   */
  int CopyBufferSize;

  /*
   * Max image data rate to the encoder input.
   */
  int ImageRateLimit;

 /*
  * True if agent should not exit if there are no clients in rootless
  * mode
  */
  Bool NoRootlessExit;

 /*
  * Store if the user wants Xinerama. There are variables called
  * noPanoramiXExtension and noRRXineramaExtensison in os/utils.c but
  * we cannot rely on them because RandR and PanoramiX change their
  * values when trying to initialize. So we use this variable to save
  * the user preference provided by the -/+(rr)xinerama parameter(s)
  * before initializing those extensions.
  */
  Bool Xinerama;

  /*
   * Sleep delay in milliseconds.
   */
  unsigned int SleepTimeMillis;

  /*
   * Tolerance - tightens or loosens reconnect checks.
   */
  ToleranceChecksMode ReconnectTolerance;

  /*
   * Convert evdev keycodes to pc105.
   */
  KeycodeConversionMode KeycodeConversion;

  /*
   * True if agent should grab the input in windowed mode whenever the
   * agent window gets the focus
   */
  Bool AutoGrab;

} AgentOptionsRec;

typedef AgentOptionsRec *AgentOptionsPtr;

extern AgentOptionsPtr nxagentOptionsPtr;

/*
 * Macros and functions giving access to options.
 */

#define nxagentOption(option) \
    (nxagentOptionsPtr -> option)

#define nxagentChangeOption(option, value) \
    (nxagentOptionsPtr -> option = (value))

#define nxagentOptions() \
    (nxagentOptionsPtr)

/*
 * Initialize the options to the default values.
 */

extern void nxagentInitOptions(void);

/*
 * Initialize some options to the default values at reconnection.
 */

extern void nxagentResetOptions(void);

/*
 * Save a copy of the current option repository.
 */

extern void nxagentSaveOptions(void);

/*
 * Restore the options reset by nxagentResetOptions to their backup
 * value.
 */

extern void nxagentRestoreOptions(void);

#endif /* __Options_H__ */
