# NXTransDialog API

The nxcomp shared library provides an API/ABI to the nxagent Xserver for
triggering external actions by internal nxagent events.

## Server-Side (Dialog) Actions

### Overview of Dialog Actions

The currently available / implemented actions for dialog management (or
other forms of external event handling) are:

  * **DIALOG_KILL_SESSION**: triggered when...

    - the user attempts to close an nxagent desktop or rootless session
    - and the nxagent session has been launched as a non-persistent session
       (i.e. non-resumable).

    The session closure event is suppressed, while if there is any other
    dialog open still.


  * **DIALOG_SUSPEND_SESSION**: triggered when...

    - the user attempts to close an nxagent desktop or rootless session
    - and the nxagent session has been launched as a persistent session
      (i.e. resumable)

    The session closure event is suppressed, while if there is any other
    dialog open still.


  * **DIALOG_ROOTLESS**: triggered when...

    - ... (currently not used).


  * **DIALOG_PULLDOWN**: triggered when...

    - the pointing devices is moved over the top menu bar of a rootless
      application window (or one of its chilren). The pointing device must
      be approximately in the horizontal middle of the window bar.


  * **DIALOG_FONT_REPLACEMENT**: triggered when...

    - resuming a session from a client-side X11 server that lacks fonts
      that were being used prior to session suspension. There is some
      font replacement code in nxagent, which leads to sessions looking
      after resumptions under the above condition.


  * **DIALOG_FAILED_RECONNECTION**: triggered when...


  * **DIALOG_ENABLE_DESKTOP_RESIZE_MODE**: triggered when...

    - the user enables desktop resizing by toggling the
      resize hotkey combination (by default: Ctrl + Alt + "r")


  * **DIALOG_DISABLE_DESKTOP_RESIZE_MODE**: triggered when...

    - the user disables desktop resizing by toggling the
      resize hotkey combination (by default: Ctrl + Alt + "r")


  * **DIALOG_ENABLE_DEFER_MODE**: triggered when...

    - the user enables deferred screen updates by toggling the
      defer hotkey combination (by default: Ctrl + Alt + "e")


  * **DIALOG_DISABLE_DEFER_MODE**: triggered when...

    - the user disables deferred screen updates by toggling the
      defer hotkey combination (by default: Ctrl + Alt + "e")


  * **DIALOG_DISABLE_XKB**: triggered when...

    - ... (currently not used).


For more detailled insights, see these two nxagent code files:

```
  nx-X11/programs/Xserver/hw/nxagent/Dialog.c
  nx-X11/programs/Xserver/hw/nxagent/Dialog.h
```

### Execution of the external Callback Command

The executed callback command (yes, you need one command as a handler!) is
executed on the machine running nxagent, graphical output (if any)
appears inside the nxagent session.

The hook command gets executed in the following ways:


```
    if (local != 0)
    {
      if (pulldown)
      {
        execlp(command, command, "--dialog", type, "--caption", caption,
                   "--window", window, "--local", "--parent", parent,
                       "--display", display, NULL);
      }
      else
      {
        execlp(command, command, "--dialog", type, "--caption", caption,
                   "--message", message, "--local", "--parent", parent,
                       "--display", display, NULL);
      }
    }
    else
    {
      if (pulldown)
      {
        execlp(command, command, "--dialog", type, "--caption", caption,
                   "--window", window, "--parent", parent,
                       "--display", display, NULL);
      }
      else
      {
        execlp(command, command, "--dialog", type, "--caption", caption,
                   "--message", message, "--parent", parent,
                       "--display", display, NULL);
      }
    }
```

See ``nxcomp/Children.cpp`` for more details.



## Client-Side (Dialog) Actions

If you want to trigger client-side actions via NXTransDialog, then you
have to create a hook command on the machine running nxagent that pushes
nxagent's NXTransDialog events back to the client-side (where nxproxy
normally runs).


# FIXMEs

  * What was the ``--local`` command line switch used for?
