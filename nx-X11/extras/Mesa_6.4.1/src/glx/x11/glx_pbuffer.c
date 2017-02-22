/*
 * (C) Copyright IBM Corporation 2004
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * IBM AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/**
 * \file glx_pbuffer.c
 * Implementation of pbuffer related functions.
 * 
 * \author Ian Romanick <idr@us.ibm.com>
 */

#include <inttypes.h>
#include "glxclient.h"
#include <X11/extensions/extutil.h>
#include <X11/extensions/Xext.h>
#include <assert.h>
#include <string.h>
#include "glapi.h"
#include "glxextensions.h"
#include "glcontextmodes.h"
#include "glheader.h"

static void ChangeDrawableAttribute( Display * dpy, GLXDrawable drawable,
    const CARD32 * attribs, size_t num_attribs );

static void DestroyPbuffer( Display * dpy, GLXDrawable drawable );

static GLXDrawable CreatePbuffer( Display *dpy,
    const __GLcontextModes * fbconfig, unsigned int width, unsigned int height,
    const int *attrib_list, GLboolean size_in_attribs );

static int GetDrawableAttribute( Display *dpy, GLXDrawable drawable,
    int attribute, unsigned int *value );


/**
 * Change a drawable's attribute.
 *
 * This function is used to implement \c glXSelectEvent and
 * \c glXSelectEventSGIX.
 *
 * \note
 * This function dynamically determines whether to use the SGIX_pbuffer
 * version of the protocol or the GLX 1.3 version of the protocol.
 *
 * \todo
 * This function needs to be modified to work with direct-rendering drivers.
 */
static void
ChangeDrawableAttribute( Display * dpy, GLXDrawable drawable,
			 const CARD32 * attribs, size_t num_attribs )
{
   __GLXdisplayPrivate *priv = __glXInitialize(dpy);
   CARD32 * output;


   if ( (dpy == NULL) || (drawable == 0) ) {
      return;
   }


   LockDisplay(dpy);

   if ( (priv->majorVersion > 1) || (priv->minorVersion >= 3) ) {
      xGLXChangeDrawableAttributesReq *req;

      GetReqExtra( GLXChangeDrawableAttributes, 8 + (8 * num_attribs), req );
      output = (CARD32 *) (req + 1);

      req->reqType = __glXSetupForCommand(dpy);
      req->glxCode = X_GLXChangeDrawableAttributes;
      req->drawable = drawable;
      req->numAttribs = (CARD32) num_attribs;
   }
   else {
      xGLXVendorPrivateWithReplyReq *vpreq;

      GetReqExtra( GLXVendorPrivateWithReply, 4 + (8 * num_attribs), vpreq );
      output = (CARD32 *) (vpreq + 1);

      vpreq->reqType = __glXSetupForCommand(dpy);
      vpreq->glxCode = X_GLXVendorPrivateWithReply;
      vpreq->vendorCode = X_GLXvop_ChangeDrawableAttributesSGIX;

      output[0] = (CARD32) drawable;
      output++;
   }

   (void) memcpy( output, attribs, sizeof( CARD32 ) * 2 * num_attribs );

   UnlockDisplay(dpy);
   SyncHandle();

   return;
}


/**
 * Destroy a pbuffer.
 *
 * This function is used to implement \c glXDestroyPbuffer and
 * \c glXDestroyGLXPbufferSGIX.
 *
 * \note
 * This function dynamically determines whether to use the SGIX_pbuffer
 * version of the protocol or the GLX 1.3 version of the protocol.
 * 
 * \todo
 * This function needs to be modified to work with direct-rendering drivers.
 */
static void
DestroyPbuffer( Display * dpy, GLXDrawable drawable )
{
   __GLXdisplayPrivate *priv = __glXInitialize(dpy);

   if ( (dpy == NULL) || (drawable == 0) ) {
      return;
   }


   LockDisplay(dpy);

   if ( (priv->majorVersion > 1) || (priv->minorVersion >= 3) ) {
      xGLXDestroyPbufferReq * req;

      GetReqExtra( GLXDestroyPbuffer, 4, req );
      req->reqType = __glXSetupForCommand(dpy);
      req->glxCode = X_GLXDestroyPbuffer;
      req->pbuffer = (GLXPbuffer) drawable;
   }
   else {
      xGLXVendorPrivateWithReplyReq *vpreq;
      CARD32 * data;

      GetReqExtra( GLXVendorPrivateWithReply, 4, vpreq );
      data = (CARD32 *) (vpreq + 1);

      data[0] = (CARD32) drawable;

      vpreq->reqType = __glXSetupForCommand(dpy);
      vpreq->glxCode = X_GLXVendorPrivateWithReply;
      vpreq->vendorCode = X_GLXvop_DestroyGLXPbufferSGIX;
   }

   UnlockDisplay(dpy);
   SyncHandle();

   return;
}


/**
 * Get a drawable's attribute.
 *
 * This function is used to implement \c glXGetSelectedEvent and
 * \c glXGetSelectedEventSGIX.
 *
 * \note
 * This function dynamically determines whether to use the SGIX_pbuffer
 * version of the protocol or the GLX 1.3 version of the protocol.
 *
 * \todo
 * The number of attributes returned is likely to be small, probably less than
 * 10.  Given that, this routine should try to use an array on the stack to
 * capture the reply rather than always calling Xmalloc.
 *
 * \todo
 * This function needs to be modified to work with direct-rendering drivers.
 */
static int
GetDrawableAttribute( Display *dpy, GLXDrawable drawable,
		      int attribute, unsigned int *value )
{
   __GLXdisplayPrivate *priv = __glXInitialize(dpy);
   xGLXGetDrawableAttributesReply reply;
   CARD32 * data;
   unsigned int length;
   unsigned int i;
   unsigned int num_attributes;
   GLboolean use_glx_1_3 = ((priv->majorVersion > 1)
			    || (priv->minorVersion >= 3));


   if ( (dpy == NULL) || (drawable == 0) ) {
      return 0;
   }

   
   LockDisplay(dpy);

   if ( use_glx_1_3 ) {
      xGLXGetDrawableAttributesReq *req;

      GetReqExtra( GLXGetDrawableAttributes, 4, req );
      req->reqType = __glXSetupForCommand(dpy);
      req->glxCode = X_GLXGetDrawableAttributes;
      req->drawable = drawable;
   }
   else {
      xGLXVendorPrivateWithReplyReq *vpreq;

      GetReqExtra( GLXVendorPrivateWithReply, 4, vpreq );
      data = (CARD32 *) (vpreq + 1);
      data[0] = (CARD32) drawable;

      vpreq->reqType = __glXSetupForCommand(dpy);
      vpreq->glxCode = X_GLXVendorPrivateWithReply;
      vpreq->vendorCode = X_GLXvop_GetDrawableAttributesSGIX;
   }

   _XReply(dpy, (xReply*) &reply, 0, False);

   length = reply.length;
   num_attributes = (use_glx_1_3) ? reply.numAttribs : length / 2;
   data = (CARD32 *) Xmalloc( length * sizeof(CARD32) );
   if ( data == NULL ) {
      /* Throw data on the floor */
      _XEatData(dpy, length);
   } else {
      _XRead(dpy, (char *)data, length * sizeof(CARD32) );
   }

   UnlockDisplay(dpy);
   SyncHandle();


   /* Search the set of returned attributes for the attribute requested by
    * the caller.
    */

   for ( i = 0 ; i < num_attributes ; i++ ) {
      if ( data[i*2] == attribute ) {
	 *value = data[ (i*2) + 1 ];
	 break;
      }
   }

   Xfree( data );

   return 0;
}


/**
 * Create a non-pbuffer GLX drawable.
 *
 * \todo
 * This function needs to be modified to work with direct-rendering drivers.
 */
static GLXDrawable
CreateDrawable( Display *dpy, const __GLcontextModes * fbconfig,
		Drawable drawable, const int *attrib_list,
		CARD8 glxCode )
{
   xGLXCreateWindowReq * req;
   CARD32 * data;
   unsigned int i;

   i = 0;
   if (attrib_list) {
       while (attrib_list[i * 2] != None)
	   i++;
   }

   LockDisplay(dpy);
   GetReqExtra( GLXCreateWindow, 8 * i, req );
   data = (CARD32 *) (req + 1);

   req->reqType = __glXSetupForCommand(dpy);
   req->glxCode = glxCode;
   req->screen = (CARD32) fbconfig->screen;
   req->fbconfig = fbconfig->fbconfigID;
   req->window = (GLXPbuffer) drawable;
   req->glxwindow = (GLXWindow) XAllocID(dpy);
   req->numAttribs = (CARD32) i;

   memcpy( data, attrib_list, 8 * i );
   
   UnlockDisplay(dpy);
   SyncHandle();
   
   return (GLXDrawable)req->glxwindow;
}


/**
 * Destroy a non-pbuffer GLX drawable.
 *
 * \todo
 * This function needs to be modified to work with direct-rendering drivers.
 */
static void
DestroyDrawable( Display * dpy, GLXDrawable drawable, CARD32 glxCode )
{
   xGLXDestroyPbufferReq * req;

   if ( (dpy == NULL) || (drawable == 0) ) {
      return;
   }


   LockDisplay(dpy);

   GetReqExtra( GLXDestroyPbuffer, 4, req );
   req->reqType = __glXSetupForCommand(dpy);
   req->glxCode = glxCode;
   req->pbuffer = (GLXPbuffer) drawable;

   UnlockDisplay(dpy);
   SyncHandle();

   return;
}


/**
 * Create a pbuffer.
 *
 * This function is used to implement \c glXCreatePbuffer and
 * \c glXCreateGLXPbufferSGIX.
 *
 * \note
 * This function dynamically determines whether to use the SGIX_pbuffer
 * version of the protocol or the GLX 1.3 version of the protocol.
 *
 * \todo
 * This function needs to be modified to work with direct-rendering drivers.
 */
static GLXDrawable
CreatePbuffer( Display *dpy, const __GLcontextModes * fbconfig,
	       unsigned int width, unsigned int height, 
	       const int *attrib_list, GLboolean size_in_attribs )
{
   __GLXdisplayPrivate *priv = __glXInitialize(dpy);
   GLXDrawable id = 0;
   CARD32 * data;
   unsigned int  i;

   i = 0;
   if (attrib_list) {
       while (attrib_list[i * 2])
	   i++;
   }

   LockDisplay(dpy);
   id = XAllocID(dpy);

   if ( (priv->majorVersion > 1) || (priv->minorVersion >= 3) ) {
      xGLXCreatePbufferReq * req;
      unsigned int extra = (size_in_attribs) ? 0 : 2;

      GetReqExtra( GLXCreatePbuffer, (8 * (i + extra)), req );
      data = (CARD32 *) (req + 1);

      req->reqType = __glXSetupForCommand(dpy);
      req->glxCode = X_GLXCreatePbuffer;
      req->screen = (CARD32) fbconfig->screen;
      req->fbconfig = fbconfig->fbconfigID;
      req->pbuffer = (GLXPbuffer) id;
      req->numAttribs = (CARD32) (i + extra);

      if ( ! size_in_attribs ) {
	 data[(2 * i) + 0] = GLX_PBUFFER_WIDTH;
	 data[(2 * i) + 1] = width;
	 data[(2 * i) + 2] = GLX_PBUFFER_HEIGHT;
	 data[(2 * i) + 3] = height;
	 data += 4;
      }
   }
   else {
      xGLXVendorPrivateReq *vpreq;

      GetReqExtra( GLXVendorPrivate, 20 + (8 * i), vpreq );
      data = (CARD32 *) (vpreq + 1);

      vpreq->reqType = __glXSetupForCommand(dpy);
      vpreq->glxCode = X_GLXVendorPrivate;
      vpreq->vendorCode = X_GLXvop_CreateGLXPbufferSGIX;

      data[0] = (CARD32) fbconfig->screen;
      data[1] = (CARD32) fbconfig->fbconfigID;
      data[2] = (CARD32) id;
      data[3] = (CARD32) width;
      data[4] = (CARD32) height;
      data += 5;
   }

   (void) memcpy( data, attrib_list, sizeof(CARD32) * 2 * i );

   UnlockDisplay(dpy);
   SyncHandle();

   return id;
}


/**
 * Create a new pbuffer.
 */
PUBLIC GLXPbufferSGIX
glXCreateGLXPbufferSGIX(Display *dpy, GLXFBConfigSGIX config,
			unsigned int width, unsigned int height,
			int *attrib_list)
{
   return (GLXPbufferSGIX) CreatePbuffer( dpy, (__GLcontextModes *) config,
					  width, height,
					  attrib_list, GL_FALSE );
}


/**
 * Create a new pbuffer.
 */
PUBLIC GLXPbuffer
glXCreatePbuffer(Display *dpy, GLXFBConfig config, const int *attrib_list)
{
   return (GLXPbuffer) CreatePbuffer( dpy, (__GLcontextModes *) config,
				      0, 0,
				      attrib_list, GL_TRUE );
}


/**
 * Destroy an existing pbuffer.
 */
PUBLIC void
glXDestroyPbuffer(Display *dpy, GLXPbuffer pbuf)
{
   DestroyPbuffer( dpy, pbuf );
}


/**
 * Query an attribute of a drawable.
 */
PUBLIC void
glXQueryDrawable(Display *dpy, GLXDrawable drawable,
		 int attribute, unsigned int *value)
{
   GetDrawableAttribute( dpy, drawable, attribute, value );
}


/**
 * Query an attribute of a pbuffer.
 */
PUBLIC int
glXQueryGLXPbufferSGIX(Display *dpy, GLXPbufferSGIX drawable,
		       int attribute, unsigned int *value)
{
   return GetDrawableAttribute( dpy, drawable, attribute, value );
}


/**
 * Select the event mask for a drawable.
 */
PUBLIC void
glXSelectEvent(Display *dpy, GLXDrawable drawable, unsigned long mask)
{
   CARD32 attribs[2];

   attribs[0] = (CARD32) GLX_EVENT_MASK;
   attribs[1] = (CARD32) mask;

   ChangeDrawableAttribute( dpy, drawable, attribs, 1 );
}


/**
 * Get the selected event mask for a drawable.
 */
PUBLIC void
glXGetSelectedEvent(Display *dpy, GLXDrawable drawable, unsigned long *mask)
{
   unsigned int value;


   /* The non-sense with value is required because on LP64 platforms
    * sizeof(unsigned int) != sizeof(unsigned long).  On little-endian
    * we could just type-cast the pointer, but why?
    */

   GetDrawableAttribute( dpy, drawable, GLX_EVENT_MASK_SGIX, & value );
   *mask = value;
}


PUBLIC GLXPixmap
glXCreatePixmap( Display *dpy, GLXFBConfig config, Pixmap pixmap,
		 const int *attrib_list )
{
   return CreateDrawable( dpy, (__GLcontextModes *) config,
			  (Drawable) pixmap, attrib_list,
			  X_GLXCreatePixmap );
}


PUBLIC GLXWindow
glXCreateWindow( Display *dpy, GLXFBConfig config, Window win,
		 const int *attrib_list )
{
   return CreateDrawable( dpy, (__GLcontextModes *) config,
			  (Drawable) win, attrib_list,
			  X_GLXCreateWindow );
}


PUBLIC void
glXDestroyPixmap(Display *dpy, GLXPixmap pixmap)
{
   DestroyDrawable( dpy, (GLXDrawable) pixmap, X_GLXDestroyPixmap );
}


PUBLIC void
glXDestroyWindow(Display *dpy, GLXWindow win)
{
   DestroyDrawable( dpy, (GLXDrawable) win, X_GLXDestroyWindow );
}


PUBLIC GLX_ALIAS_VOID(glXDestroyGLXPbufferSGIX,
	  (Display *dpy, GLXPbufferSGIX pbuf),
	  (dpy, pbuf),
	  glXDestroyPbuffer)

PUBLIC GLX_ALIAS_VOID(glXSelectEventSGIX,
	  (Display *dpy, GLXDrawable drawable, unsigned long mask),
	  (dpy, drawable, mask),
	  glXSelectEvent)

PUBLIC GLX_ALIAS_VOID(glXGetSelectedEventSGIX,
	  (Display *dpy, GLXDrawable drawable, unsigned long *mask),
	  (dpy, drawable, mask),
	  glXGetSelectedEvent)
