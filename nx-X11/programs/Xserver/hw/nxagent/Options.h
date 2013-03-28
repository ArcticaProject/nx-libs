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

#ifndef __Options_H__
#define __Options_H__

#ifndef True
#define True   1
#endif

#ifndef False
#define False  0
#endif

#define UNDEFINED -1
#define COPY_UNLIMITED -1

extern unsigned int nxagentPrintGeometryFlags;

typedef enum _BackingStoreMode
{
  BackingStoreUndefined = -1,
  BackingStoreNever,
  BackingStoreWhenRequested,
  BackingStoreForce

} BackingStoreMode;

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

/*
 * Set of options affecting agent operations.
 */

typedef struct _AgentOptions
{
  /*
   * Link type of the NX connection or none,
   * if this is a direct X11 connection.
   */

  int LinkType;

  /*
   * Is agent running in desktop mode? This
   * is presently the default.
   */

  int Desktop;

  /*
   * True if user activated rootless mode.
   */

  int Rootless;

  /*
   * True for shadow mode.
   */

  int Shadow;

  /*
   * True if user activated persistent mode.
   */

  int Persistent;

  /*
   * True if user activated fullscreen mode.
   */

  int Fullscreen;

  /*
   * True if the fullscreen NX session will
   * extend on all available screens.
   */

  int AllScreens;

  /*
   * Set to the auto-disconnect timeout, if
   * the user activated this feature.
   */

  int Timeout;

  /*
   * Geometry of the agent's window.
   */

  int X;
  int Y;
  int Width;
  int Height;
  int BorderWidth;

  /*
   * Geometry of the agent's window in window
   * mode. Used to restore window size when
   * switching back to window mode from full-
   * screen.
   */

  int WMBorderWidth;
  int WMTitleHeight;

  int SavedX;
  int SavedY;
  int SavedWidth;
  int SavedHeight;

  int SavedRootWidth;
  int SavedRootHeight;

  /*
   * Set if agent is running nested in another
   * agent X server.
   */

  int Nested;

  /*
   * Selected backing-store mode.
   */

  BackingStoreMode BackingStore;

  /*
   * Selected clipboard mode.
   */

  ClipboardMode Clipboard;

  /*
   * Enable agent to use the MITSHM extension in
   * path from remote proxy to the real X server.
   */

  int SharedMemory;

  /*
   * Enable agent to use shared Pixmaps
   */

  int SharedPixmaps;

  /*
   * Enable agent to propagate keyboard and pointer
   * device configuration to the remote X server.
   */

  int DeviceControl;

  /*
   * Explicitly asked config propagation.
   */

  int DeviceControlUserDefined;

  /*
   * Resuming keyboard device corrects keymap if session
   * migrates across platforms with different keycode
   * layout.
   */

  int ResetKeyboardAtResume;

  /*
   * Reset server when the last client disconnects.
   */

  int Reset;

  /* 
   * Geometry of the agent root window, relative to
   * the agent default window.
   */

  int RootX;
  int RootY;
  int RootWidth;
  int RootHeight;

  /*
   * Horizontal and vertical span of the
   * agent viewport.
   */

  int ViewportXSpan;
  int ViewportYSpan;

  /*
   * True if the user can resize the desktop
   * by dragging the window border.
   */

  int DesktopResize;

  /*
   * The scaling ratio of the shadow agent.
   */

  int Ratio;

  int XRatio;

  int YRatio;

  float FloatRatio;

  float FloatXRatio;

  float FloatYRatio;

  /*
   * The shadow agent uses the Damage extension.
   */

  int UseDamage;

  /*
   * Was the agent run with the -B option?
   */

  int Binder;

  char *BinderOptions;

  /*
   * Set if the agent has to connect to a
   * desktop manager to start the session.
   */

  int Xdmcp;

  /*
   * Latency of the link. It is simply set
   * to a reference value, calculated based
   * on the time required to complete the
   * query of the agent's atoms at session
   * startup.
   */

  int DisplayLatency;

  /*
   * Size of the Xlib display buffer. The
   * default is set according to the link
   * type.
   */

  int DisplayBuffer;

  /*
   * Buffer coalescence timeout.
   */

  int DisplayCoalescence;

  /*
   * Use the composite extension when
   * available on the remote display.
   */

  int Composite;

  /*
   * If set, don't skip internal operations
   * when the agent window is not fully visible.
   */

  int IgnoreVisibility;

  /*
   * If set, prevent the shadow session to
   * interact with master diplay.
   */

  int ViewOnly;

  /*
   * If true select a lossy or lossless comp-
   * ression method based on the characterist-
   * ics of the image.
   */

  int Adaptive;

  /*
   * Stream the images and update the display
   * when the image has been completely trans-
   * ferred.
   */

  int Streaming;

  /*
   * Use a lazy approach in updating the remote
   * display. This means delaying the bandwidth
   * consuming graphic operations and synchroniz-
   * ing the screen at idle time.
   */

  int DeferLevel;

  /*
   * Maxuimum elapsed time before a new full
   * synchronization.
   */

  unsigned long DeferTimeout;

  /*
   * Maximum size of the tile used when sending
   * an image to the remote display.
   */

  int TileWidth;
  int TileHeight;

  /*
   * Enabling/disabling the pulldown menu.
   */

  int Menu;

  /*
   * Specify the Operative System of the client.
   */

  int ClientOs;

  /*
   * Inhibit some XKEYBOARD requests.
   */

  int InhibitXkb;

  /*
   * Maximum number of bytes that can be pasted from
   * an NX session into an external application.
   */

  int CopyBufferSize;

  /*
   * Max image data rate to the encoder input.
   */

  int ImageRateLimit;

 /*
  * True if agent should not exit if there are no
  * clients in rootless mode
  */

  int NoRootlessExit;

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
 * Initialize some options to the default values
 * at reconnection.
 */

extern void nxagentResetOptions(void);

/*
 * Save a copy of the current option repository.
 */

extern void nxagentSaveOptions(void);

/*
 * Restore the options reset by nxagentResetOptions
 * to their backup value.
 */

extern void nxagentRestoreOptions(void);

#endif /* __Options_H__ */
