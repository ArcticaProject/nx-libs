
#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include <stdio.h>
#include <nx-X11/X.h>
#include "windowstr.h"
#include <xkbsrv.h>

int
XkbDDXPrivate(DeviceIntPtr dev,KeyCode key,XkbAction *act)
{
    return 0;
}

