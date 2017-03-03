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

/*

Copyright (c) 1995  X Consortium

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from the X Consortium.

*/

#ifndef __Agent_H__
#define __Agent_H__

/*
 * Machines with a 64 bit library interface and a 32 bit
 * server require name changes to protect the guilty.
 */

#ifdef _XSERVER64
#define _XSERVER64_tmp
#undef _XSERVER64
typedef unsigned long XID64;
typedef unsigned long Mask64;
typedef unsigned long Atom64;
typedef unsigned long VisualID64;
typedef unsigned long Time64;
#define XID     XID64
#define Mask    Mask64
#define Atom    Atom64
#define VisualID VisualID64
#define Time    Time64
typedef XID Window64;
typedef XID Drawable64;
typedef XID Font64;
typedef XID Pixmap64;
typedef XID Cursor64;
typedef XID Colormap64;
typedef XID GContext64;
typedef XID KeySym64;
#define Window          Window64
#define Drawable        Drawable64
#define Font            Font64
#define Pixmap          Pixmap64
#define Cursor          Cursor64
#define Colormap        Colormap64
#define GContext        GContext64
#define KeySym          KeySym64

#define XlibAtom        Atom64
#define XlibWindow      Window64
#define XlibPixmap      Pixmap64
#define XlibFont        Font64
#define XlibKeySym      KeySym64
#define XlibXID         XID64

#else   /*_XSERVER64*/

#define XlibAtom        Atom
#define XlibWindow      Window
#define XlibPixmap      Pixmap
#define XlibFont        Font
#define XlibKeySym      KeySym
#define XlibXID         XID

#endif  /*_XSERVER64*/

#define NX_TRANS_SOCKET
#define GC XlibGC
#include <nx-X11/Xlib.h>
#include <X11/extensions/shape.h>
#undef GC

#ifdef _XSERVER64_tmp
#define _XSERVER64
#undef _XSERVER64_tmp
#undef XID
#undef Mask
#undef Atom
#undef VisualID
#undef Time
#undef Window
#undef Drawable
#undef Font
#undef Pixmap
#undef Cursor
#undef Colormap
#undef GContext
#undef KeySym
#endif /*_XSERVER64_tmp*/

#endif /* __Agent_H__ */
