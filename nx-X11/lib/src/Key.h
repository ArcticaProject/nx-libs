
#ifndef _KEY_H_
#define _KEY_H_

#include <nx-X11/Xlib.h>
#include <nx-X11/Xresource.h>

#ifndef NEEDKTABLE
extern const unsigned char _XkeyTable[];
#endif

extern int
_XKeyInitialize(
    Display *dpy);

extern XrmDatabase
_XInitKeysymDB(
        void);

#endif /* _KEY_H_ */
