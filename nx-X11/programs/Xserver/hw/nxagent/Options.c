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

#include <stdio.h>
#include <string.h>

#include "X.h"

#include "Agent.h"
#include "Args.h"
#include "Options.h"
#include "Utils.h"

/*
 * Instead of having a single options repository data could be
 * attached to the display or the screen. The macro nxagentOption()
 * should make the transition simple.
 */

AgentOptionsRec nxagentOptions;

AgentOptionsRec nxagentOptionsBackup;

AgentOptionsPtr nxagentOptionsPtr = &nxagentOptions;

/*
 * If this is set, print the geometry in the block handler.
 */

unsigned int nxagentPrintGeometryFlags = 0;
/*
 * This must be called at startup to initialize the options repository
 * to the default values.
 */

void nxagentInitOptions(void)
{
  nxagentOptions.LinkType = UNDEFINED;

  nxagentOptions.Desktop    = True;
  nxagentOptions.Persistent = True;
  nxagentOptions.Rootless   = False;
  nxagentOptions.Shadow     = False;
  nxagentOptions.Fullscreen = False;
  nxagentOptions.AllScreens = False;
  nxagentOptions.NoRootlessExit = False;

  nxagentOptions.X           = 0;
  nxagentOptions.Y           = 0;
  nxagentOptions.Width       = 0;
  nxagentOptions.Height      = 0;
  nxagentOptions.BorderWidth = 0;

  nxagentOptions.WMBorderWidth = -1;
  nxagentOptions.WMTitleHeight = -1;

  nxagentOptions.SavedX      = 0;
  nxagentOptions.SavedY      = 0;
  nxagentOptions.SavedWidth  = 0;
  nxagentOptions.SavedHeight = 0;

  nxagentOptions.Timeout = 0;

  nxagentOptions.Nested = False;

  nxagentOptions.BackingStore = BackingStoreUndefined;

  nxagentOptions.Clipboard = ClipboardBoth;
  nxagentOptions.TextClipboard = False;

  nxagentOptions.SharedMemory = True;

  nxagentOptions.SharedPixmaps = True;

  nxagentOptions.DeviceControl = False;

  nxagentOptions.DeviceControlUserDefined = False;

  nxagentOptions.ResetKeyboardAtResume = True;

  nxagentOptions.Reset = False;

  nxagentOptions.RootX = 0;
  nxagentOptions.RootY = 0;
  nxagentOptions.RootWidth = 0;
  nxagentOptions.RootHeight = 0;

  nxagentOptions.ViewportXSpan = 0;
  nxagentOptions.ViewportYSpan = 0;

  nxagentOptions.DesktopResize = True;

  nxagentOptions.Ratio  = DONT_SCALE;
  nxagentOptions.XRatio = DONT_SCALE;
  nxagentOptions.YRatio = DONT_SCALE;

  nxagentOptions.FloatRatio  = 1.0;
  nxagentOptions.FloatXRatio = 1.0;
  nxagentOptions.FloatYRatio = 1.0;

  nxagentOptions.UseDamage = True;

  nxagentOptions.Binder = False;
  nxagentOptions.BinderOptions = NULL;

  nxagentOptions.Xdmcp = False;

  nxagentOptions.DisplayBuffer  = UNDEFINED;
  nxagentOptions.DisplayCoalescence = 0;

  nxagentOptions.Composite = False;

  nxagentOptions.IgnoreVisibility = False;

  nxagentOptions.ViewOnly = False;

  nxagentOptions.Adaptive = False;

  nxagentOptions.Streaming = False;

  nxagentOptions.DeferLevel   = UNDEFINED;
  nxagentOptions.DeferTimeout = 200;

  nxagentOptions.TileWidth  = UNDEFINED;
  nxagentOptions.TileHeight = UNDEFINED;

  nxagentOptions.Menu = True;

  nxagentOptions.MagicPixel = True;

  nxagentOptions.ClientOs = UNDEFINED;

  nxagentOptions.InhibitXkb = True;

  nxagentOptions.CopyBufferSize = COPY_UNLIMITED;

  nxagentOptions.ImageRateLimit = 0;

  nxagentOptions.Xinerama = True;

  nxagentOptions.SleepTimeMillis = DEFAULT_SLEEP_TIME_MILLIS;

  nxagentOptions.ReconnectTolerance = DEFAULT_TOLERANCE;

  nxagentOptions.KeycodeConversion = DEFAULT_KEYCODE_CONVERSION;

  nxagentOptions.AutoGrab = False;
}

/*
 * This is called at session reconnection to reset some options to
 * their default values. The reason to avoid calling the
 * nxagentInitOptions() is that not all the options can change value
 * when reconnecting.
 */

void nxagentResetOptions(void)
{
  if (nxagentLockDeferLevel == 0)
  {
    nxagentOptions.DeferLevel   = UNDEFINED;
  }

  nxagentOptions.DeferTimeout = 200;

  nxagentOptions.TileWidth  = UNDEFINED;
  nxagentOptions.TileHeight = UNDEFINED;

  nxagentOptions.WMBorderWidth = -1;
  nxagentOptions.WMTitleHeight = -1;

  nxagentOptions.KeycodeConversion = DEFAULT_KEYCODE_CONVERSION;
}

void nxagentSaveOptions(void)
{
  memcpy(&nxagentOptionsBackup, &nxagentOptions, sizeof(AgentOptionsRec));
}

void nxagentRestoreOptions(void)
{
  nxagentOptions.DeferLevel   = nxagentOptionsBackup.DeferLevel;
  nxagentOptions.DeferTimeout = nxagentOptionsBackup.DeferTimeout;

  nxagentOptions.TileWidth  = nxagentOptionsBackup.TileWidth;
  nxagentOptions.TileHeight = nxagentOptionsBackup.TileHeight;
}
