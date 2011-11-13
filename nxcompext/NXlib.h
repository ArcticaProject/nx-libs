/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2007 NoMachine, http://www.nomachine.com/.         */
/*                                                                        */
/* NXCOMPEXT, NX protocol compression and NX extensions to this software  */
/* are copyright of NoMachine. Redistribution and use of the present      */
/* software is allowed according to terms specified in the file LICENSE   */
/* which comes in the source distribution.                                */
/*                                                                        */
/* Check http://www.nomachine.com/licensing.html for applicability.       */
/*                                                                        */
/* NX and NoMachine are trademarks of NoMachine S.r.l.                    */
/*                                                                        */
/* All rigths reserved.                                                   */
/*                                                                        */
/**************************************************************************/

#ifndef NXlib_H
#define NXlib_H

#ifdef __cplusplus
extern "C" {
#endif

#include <X11/X.h>
#include <X11/Xlib.h>

#include "NX.h"
#include "NXpack.h"
#include "NXproto.h"
#include "NXvars.h"

/*
 * All the NX code should use these.
 */

#define Xmalloc(size) malloc((size))
#define Xfree(ptr)    free((ptr))
 
/*
 * Maximum number of supported pack methods.
 */

#define NXNumberOfPackMethods  128

/*
 * Assume this as the limit of resources that
 * can be provided to the split and unpack
 * requests.
 */

#define NXNumberOfResources    256

#define NXNoResource           256 + 1
#define NXAnyResource          256 + 2

/*
 * Initialize the internal structures used by
 * the library. Should be executed again after
 * having reopened the display.
 */

extern int NXInitDisplay(
#if NeedFunctionPrototypes
    Display*            /* display */
#endif
);

/*
 * Reset all the internal structures. Should be
 * executed after closing the display.
 */

extern int NXResetDisplay(
#if NeedFunctionPrototypes
    Display*            /* display */
#endif
);

/*
 * Set the NX display flush policy. The policy can
 * be either NXFlushDeferred or NXFlushImmediate.
 */

extern int NXSetDisplayPolicy(
#if NeedFunctionPrototypes
    Display*            /* display */,
    int                 /* policy */
#endif
);

/*
 * Set the display output buffer size.
 */

extern int NXSetDisplayBuffer(
#if NeedFunctionPrototypes
    Display*            /* display */,
    int                 /* size */
#endif
);

/*
 * If set, the Popen() function in the X server
 * wil remove the LD_LIBRARY_PATH variable from
 * the environment before calling the execl()
 * function on the child process. The function
 * returns the previous value.
 */

extern int NXUnsetLibraryPath(
#if NeedFunctionPrototypes
    int                 /* value */
#endif
);

/*
 * If the parameter is true, the Xlib I/O error
 * handler will return, instead of quitting the
 * program. The function returns the previous
 * value.
 */

extern int NXHandleDisplayError(
#if NeedFunctionPrototypes
    int                 /* value */
#endif
);

/*
 * Shutdown the display descriptor and force Xlib
 * to set the I/O error flag.
 */

extern Bool NXForceDisplayError(
#if NeedFunctionPrototypes
    Display*            /* display */
#endif
);

/*
 * Check the value of the XlibDisplayIOError flag.
 * If not set, try to call the display error hand-
 * ler to give to the application a chance to see
 * whether it needs to close the connection.
 */

extern int NXDisplayError(
#if NeedFunctionPrototypes
    Display*            /* display */
#endif
);

/*
 * Query the number of bytes readable from the
 * display connection.
 */

extern int NXDisplayReadable(
#if NeedFunctionPrototypes
    Display*            /* display */
#endif
);

/*
 * Query the number of the outstanding bytes to
 * flush to the display connection.
 */

extern int NXDisplayFlushable(
#if NeedFunctionPrototypes
    Display*            /* display */
#endif
);

/*
 * Return a value between 0 and 9 indicating the
 * congestion level of the NX transport based on
 * the tokens remaining. A value of 9 means that
 * the link is congested and no further data can
 * be sent.
 */

extern int NXDisplayCongestion(
#if NeedFunctionPrototypes
    Display*            /* display */
#endif
);

/*
 * Flush the Xlib display buffer and/or the
 * outstanding data accumulated by the NX
 * transport.
 */

extern int NXFlushDisplay(
#if NeedFunctionPrototypes
    Display*            /* display */,
    int                 /* what */
#endif
);

/*
 * Public interfaces used to set the handlers.
 * They all return the previous handler.
 */

extern NXDisplayErrorPredicate NXSetDisplayErrorPredicate(
#if NeedFunctionPrototypes
    NXDisplayErrorPredicate /* predicate */
#endif
);

/*
 * Called when the display blocks waiting to read or
 * write more data.
 */

extern NXDisplayBlockHandler NXSetDisplayBlockHandler(
#if NeedFunctionPrototypes
    NXDisplayBlockHandler /* handler */
#endif
);

/*
 * Called after more data is written to the display.
 * When the NX transport is running, data may be queued
 * until an explicit flush.
 */

extern NXDisplayWriteHandler NXSetDisplayWriteHandler(
#if NeedFunctionPrototypes
    NXDisplayWriteHandler /* handler */
#endif
);

/*
 * Called after more data is sent to the remote proxy.
 *
 * Here the display pointer is passed as the second
 * parameter to make clear that the function does not
 * tie the callback to the display, but, similarly to
 * all the Xlib error handlers, to a global variable
 * shared by all the Xlib functions. The display
 * pointer will be passed back by nxcomp at the time
 * it will call the handler. This is because nxcomp
 * doesn't have access to the display structure.
 */

extern NXDisplayFlushHandler NXSetDisplayFlushHandler(
#if NeedFunctionPrototypes
    NXDisplayFlushHandler      /* handler */,
    Display*                   /* display */
#endif
);

/*
 * Get an arbitrary null terminated buffer to be added
 * to the NX statistics.
 */

extern NXDisplayStatisticsHandler NXSetDisplayStatisticsHandler(
#if NeedFunctionPrototypes
    NXDisplayStatisticsHandler /* handler */,
    char **                    /* buffer */
#endif
);

/*
 * Redefine the function called by Xlib in the case of
 * an out-of-order sequence number received in the X
 * protocol stream.
 */
 
extern NXLostSequenceHandler NXSetLostSequenceHandler(
#if NeedFunctionPrototypes
    NXLostSequenceHandler /* handler */
#endif
);

/*
 * The agent should get the NX parameters at startup, just after
 * having opened the display. If the agent is not able to satisfy
 * the pack method set by user (because a method is not applica-
 * ble, it is not supported by the remote or it simply requires a
 * screen depth greater than the depth available), it should fall
 * back to the nearest method of the same type.
 */

extern Status NXGetControlParameters(
#if NeedFunctionPrototypes
    Display*            /* display */,
    unsigned int*       /* link_type */,
    unsigned int*       /* local_major */,
    unsigned int*       /* local_minor */,
    unsigned int*       /* local_patch */,
    unsigned int*       /* remote_major */,
    unsigned int*       /* remote_minor */,
    unsigned int*       /* remote_patch */,
    int*                /* frame_timeout */,
    int*                /* ping_timeout */,
    int*                /* split_mode */,
    int*                /* split_size */,
    unsigned int*       /* pack_method */,
    unsigned int*       /* pack_quality */,
    int*                /* data_level */,
    int*                /* stream_level */,
    int*                /* delta_level */,
    unsigned int*       /* load_cache */,
    unsigned int*       /* save_cache */,
    unsigned int*       /* startup_cache */
#endif
);

/*
 * Which unpack methods are supported by the remote proxy?
 */

extern Status NXGetUnpackParameters(
#if NeedFunctionPrototypes
    Display*            /* display */,
    unsigned int*       /* entries */,
    unsigned char[]     /* supported_methods */
#endif
);

/*
 * Query and enable shared memory support on path agent to X
 * client proxy and X server proxy to real X server. At the
 * moment only the path proxy to real X server is implemented.
 * On return flags will say if support has been successfully
 * activated. Segments will contain the XID associated to the
 * shared memory blocks. A MIT-SHM compliant protocol is used
 * between proxy and the real server, while a simplified
 * version is used between the agent and the client proxy to
 * accomodate both packed images and plain X bitmaps.
 */

extern Status NXGetShmemParameters(
#if NeedFunctionPrototypes
    Display*            /* display */,
    unsigned int*       /* enable_client  */,
    unsigned int*       /* enable_server */,
    unsigned int*       /* client_segment */,
    unsigned int*       /* server_segment */,
    unsigned int*       /* client_size */,
    unsigned int*       /* server_size */
#endif
);

/*
 * Get the path to the font server that can be used by the X
 * server to tunnel the font connections across the NX link.
 * The path actually represents the TCP port where the proxy
 * on the NX client side is listening. The agent can tempora-
 * rily enable the tunneling when it needs a font that is not
 * available on the client, for example when the session is
 * migrated from a different X server.
 */

extern Status NXGetFontParameters(
#if NeedFunctionPrototypes
    Display*            /* display */,
    unsigned int        /* path_length */,
    char[]              /* path_data */
#endif
);

/*
 * This set of functions is used to leverage the image stream-
 * ing capabilities built in nxcomp. An image can be streamed
 * by sending a start-split message, followed by the X messages
 * that will have to be split by the proxy, followed by an end-
 * split closure. Usually, in the middle of a start-split/end-
 * split sequence there will be a single PutImage() or PutPack-
 * edImage(), that, in turn, can generate multiple partial
 * requests, like a SetUnpackColormap() and SetUnpackAlpha()
 * that will be later used to decompress the image to its ori-
 * ginal form. Multiple requests may be also generated because
 * of the maximum size of a X request being exceeded, so that
 * Xlib has to divide the single image in multiple sub-image re-
 * quests. The agent doesn't need to take care of these details
 * but will rather have to track the result of the split opera-
 * tion. By monitoring the notify events sent by the proxy, the
 * agent will have to implement its own strategy to deal with
 * the resources. For example, it will be able to:
 *
 * - Mark a drawable as dirty, if the image was not sent
 *   synchronously, in the main X oputput stream.
 *
 * - Choose to commit or discard the original image, at the
 *   time it will be recomposed at the remote side. This may
 *   include all the messages that were part of the split
 *   (the colormap, the alpha channel, etc.)
 *
 * - Mark the drawable as clean again, if the image was
 *   committed and the drawable didn't change in the mean-
 *   while.
 *
 * At the time the proxy receives the end-split, it reports the
 * result of the operation to the agent. The agent will be able
 * to identify the original split operation (the one referenced
 * in the start-split/end-split sequence) by the small integer
 * number (0-255) named 'resource' sent in the events.
 *
 * One of the following cases may be encountered:
 *
 *
 * NXNoSplitNotify      All messages were sent in the main out-
 *                      put stream, so that no split actually
 *                      took place.
 *
 * NXStartSplitNotify   One or more messages were split, so,
 *                      at discrection of the agent, the client
 *                      may be suspended until the transferral
 *                      is completed.
 *
 * NXCommitSplitNotify  One of the requests that made up the
 *                      split was recomposed. The agent should
 *                      either commit the given request or tell
 *                      the proxy to discard it.
 *
 * NXEndSplitNotify     The split was duly completed. The agent
 *                      can restart the client.
 *
 * NXEmptySplitNotify   No more split operation are pending.
 *                      The agent can use this information to
 *                      implement specific strategies requiring
 *                      that all messages have been recomposed
 *                      at the remote end, like updating the
 *                      drawables that were not synchronized
 *                      because of the lazy encoding.
 *
 * The 'mode' field that is sent by the agent in the start-split
 * request, determines the strategy that the proxy will adopt to
 * deal with the image. If set to 'eager', the proxy will only
 * split the messages whose size exceeds the split threshold (the
 * current threshold can be found in the NXGetControlParameters()
 * reply). If the mode is set to lazy, the proxy will split any
 * image that would have generated an actual transfer of the data
 * part (in practice all images that are not found in the cache).
 * This second strategy can be leveraged by an agent to further
 * reduce the bandwidth requirements. For example, by setting the
 * mode to lazy and by monitoring the result, an agent can easi-
 * ly verify if the drawable was successfully updated, mark the
 * drawable if not, and synchronize it at later time.
 *
 * See NXproto.h for the definition of the available modes.
 */

extern unsigned int NXAllocSplit(
#if NeedFunctionPrototypes
    Display*            /* display */,
    unsigned int        /* resource */
#endif
);

extern int NXStartSplit(
#if NeedFunctionPrototypes
    Display*            /* display */,
    unsigned int        /* resource */,
    unsigned int        /* mode */
#endif
);

extern int NXEndSplit(
#if NeedFunctionPrototypes
    Display*            /* display */,
    unsigned int        /* resource */
#endif
);

extern int NXCommitSplit(
#if NeedFunctionPrototypes
    Display*            /* display */,
    unsigned int        /* resource */,
    unsigned int        /* propagate */,
    unsigned char       /* request */,
    unsigned int        /* position */
#endif
);

extern int NXAbortSplit(
#if NeedFunctionPrototypes
    Display*            /* display */,
    unsigned int        /* resource */
#endif
);

extern int NXFinishSplit(
#if NeedFunctionPrototypes
    Display*            /* display */,
    unsigned int        /* resource */
#endif
);

extern int NXFreeSplit(
#if NeedFunctionPrototypes
    Display*            /* display */,
    unsigned int        /* resource */
#endif
);

extern int NXSetExposeParameters(
#if NeedFunctionPrototypes
    Display*            /* display */,
    int                 /* expose */,
    int                 /* graphics_expose */,
    int                 /* no_expose */
#endif
);

extern int NXSetCacheParameters(
#if NeedFunctionPrototypes
    Display*            /* display */,
    int                 /* enable_cache */,
    int                 /* enable_split */,
    int                 /* enable_save */,
    int                 /* enable_load */
#endif
);

extern unsigned int NXAllocUnpack(
#if NeedFunctionPrototypes
    Display*            /* display */,
    unsigned int        /* resource */
#endif
);

extern int NXSetUnpackGeometry(
#if NeedFunctionPrototypes
    Display*            /* display */,
    unsigned int        /* resource */,
    Visual*             /* visual */
#endif
);

extern int NXSetUnpackColormap(
#if NeedFunctionPrototypes
    Display*            /* display */,
    unsigned int        /* resource */,
    unsigned int        /* method */,
    unsigned int        /* entries */,
    const char*         /* data */,
    unsigned int        /* data_length */
#endif
);

extern int NXSetUnpackAlpha(
#if NeedFunctionPrototypes
    Display*            /* display */,
    unsigned int        /* resource */,
    unsigned int        /* method */,
    unsigned int        /* entries */,
    const char*         /* data */,
    unsigned int        /* data_length */
#endif
);

extern int NXSetUnpackColormapCompat(
#if NeedFunctionPrototypes
    Display*            /* display */,
    unsigned int        /* resource */,
    unsigned int        /* entries */,
    const char*         /* data */
#endif
);

extern int NXSetUnpackAlphaCompat(
#if NeedFunctionPrototypes
    Display*            /* display */,
    unsigned int        /* resource */,
    unsigned int        /* entries */,
    const char*         /* data */
#endif
);

extern int NXFreeUnpack(
#if NeedFunctionPrototypes
    Display*            /* display */,
    unsigned int        /* resource */
#endif
);

/*
 * A packed image is a XImage but with
 * offset field containing total amount
 * of packed image data.
 */

typedef XImage NXPackedImage;

NXPackedImage *NXCreatePackedImage(
#if NeedFunctionPrototypes
    Display*            /* display */,
    Visual*             /* visual */,
    unsigned int        /* method */,
    unsigned int        /* depth */,
    int                 /* format */,
    char*               /* data */,
    int                 /* data_length */,
    unsigned int        /* width */,
    unsigned int        /* height */,
    int                 /* bitmap_pad */,
    int                 /* bytes_per_line */
#endif
);

extern int NXDestroyPackedImage(
#if NeedFunctionPrototypes
    NXPackedImage*      /* image */
#endif
);

NXPackedImage *NXPackImage(
#if NeedFunctionPrototypes
    Display*            /* display */,
    XImage*             /* src_image */,
    unsigned int        /* method */
#endif
);

NXPackedImage *NXInPlacePackImage(
#if NeedFunctionPrototypes
    Display*            /* display */,
    XImage*             /* src_image */,
    unsigned int        /* method */
#endif
);

/*
 * GC is declared void * to get rid of mess
 * with different GC definitions in some X
 * server code (like in nxagent).
 */

extern int NXPutPackedImage(
#if NeedFunctionPrototypes
    Display*            /* display */,
    unsigned int        /* resource */,
    Drawable            /* drawable */,
    void*               /* gc */,
    NXPackedImage*      /* image */,
    unsigned int        /* method */,
    unsigned int        /* depth */,
    int                 /* src_x */,
    int                 /* src_y */,
    int                 /* dst_x */,
    int                 /* dst_y */,
    unsigned int        /* width */,
    unsigned int        /* height */
#endif
);

/*
 * Get multiple colors with a single call by
 * pipelining X_AllocColor requests/replies.
 */

extern int NXAllocColors(
#if NeedFunctionPrototypes
    Display*            /* display */,
    Colormap            /* colormap */,
    unsigned int        /* entries */,
    XColor[]            /* screens_in_out */,
    Bool []             /* flags allocation errors */
#endif
);

/*
 * Encode the data in the given format.
 */

extern char *NXEncodeColormap(
#if NeedFunctionPrototypes
    const char*         /* src_data */,
    unsigned int        /* src_size */,
    unsigned int*       /* dst_size */
#endif
);

extern char *NXEncodeAlpha(
#if NeedFunctionPrototypes
    const char*         /* src_data */,
    unsigned int        /* src_size */,
    unsigned int*       /* dst_size */
#endif
);

extern NXPackedImage *NXEncodeRgb(
#if NeedFunctionPrototypes
    XImage*             /* src_image */,
    unsigned int        /* method */,
    unsigned int        /* quality */
#endif
);

extern NXPackedImage *NXEncodeRle(
#if NeedFunctionPrototypes
    XImage*             /* src_image */,
    unsigned int        /* method */,
    unsigned int        /* quality */
#endif
);

extern NXPackedImage *NXEncodeJpeg(
#if NeedFunctionPrototypes
    XImage*             /* src_image */,
    unsigned int        /* method */,
    unsigned int        /* quality */
#endif
);

typedef struct
{
  long pixel;
  int found;

} NXColorTable;

extern int NXEncodeColors(
#if NeedFunctionPrototypes
  XImage*                 /* src_image */,
  NXColorTable*           /* color_table */,
  int                     /* nb_max */
#endif
);

extern NXPackedImage *NXEncodePng(
#if NeedFunctionPrototypes
    XImage*             /* src_image */,
    unsigned int        /* method */,
    unsigned int        /* quality */
#endif
);

extern NXPackedImage *NXEncodeBitmap(
#if NeedFunctionPrototypes
    XImage*             /* src_image */,
    unsigned int        /* method */,
    unsigned int        /* quality */
#endif
);

extern int NXCleanImage(
#if NeedFunctionPrototypes
    XImage*
#endif
);

extern void NXMaskImage(
#if NeedFunctionPrototypes
    XImage*              /* pointer to image to mask */ ,
    unsigned int         /* method */
#endif
);

extern int NXImageCacheSize;

extern void NXInitCache(
#if NeedFunctionPrototypes
    Display*             /* display */,
    int                  /* entries in image cache */
#endif
);

extern void NXFreeCache(
#if NeedFunctionPrototypes
    Display*             /* display */
#endif
);

extern XImage *NXCacheFindImage(
#if NeedFunctionPrototypes
    NXPackedImage*       /* packed image to find */,
    unsigned int*        /* pointer to the pack method if found */,
    unsigned char**      /* pointer to the calculated MD5 if found */
#endif
);

extern int NXCacheAddImage(
#if NeedFunctionPrototypes
    NXPackedImage*       /* packed image to be added to the cache */,
    unsigned int         /* pack method of the image to add */,
    unsigned char*       /* pointer to MD5 of the original unpacked image */
#endif
);


extern int NXGetCollectImageResource(
#if NeedFunctionPrototypes
    Display*            /* display */
#endif
);

extern int NXCollectImage(
#if NeedFunctionPrototypes
    Display*            /* display */,
    unsigned int        /* resource */,
    Drawable            /* drawable */,
    int                 /* src_x */,
    int                 /* src_y */,
    unsigned int        /* width */,
    unsigned int        /* height */,
    unsigned long       /* plane_mask */,
    int                 /* format */
#endif
);

extern int NXGetCollectedImage(
#if NeedFunctionPrototypes
    Display*            /* display */,
    unsigned int        /* resource */,
    XImage**            /* image */
#endif
);

extern int NXGetCollectPropertyResource(
#if NeedFunctionPrototypes
    Display*            /* display */
#endif
);

extern int NXCollectProperty(
#if NeedFunctionPrototypes
    Display*            /* display */,
    unsigned int        /* resource */,
    Window              /* window */,
    Atom                /* property */,
    long                /* long_offset */,
    long                /* long_length */,
    Bool                /* delete */,
    Atom                /* req_type */
#endif
);

extern int NXGetCollectedProperty(
#if NeedFunctionPrototypes
    Display*            /* display */,
    unsigned int        /* resource */,
    Atom*               /* actual_type_return */,
    int*                /* actual_format_return */,
    unsigned long*      /* nitems_return */,
    unsigned long*      /* bytes_after_return */,
    unsigned char**     /* data */
#endif
);

extern int NXGetCollectGrabPointerResource(
#if NeedFunctionPrototypes
    Display*            /* display */
#endif
);

extern int NXCollectGrabPointer(
#if NeedFunctionPrototypes
    Display*            /* display */,
    unsigned int        /* resource */,
    Window              /* grab_window */,
    Bool                /* owner_events */,
    unsigned int        /* event_mask */,
    int                 /* pointer_mode */,
    int                 /* keyboard_mode */,
    Window              /* confine_to */,
    Cursor              /* cursor */,
    Time                /* time */
#endif
);

extern int NXGetCollectedGrabPointer(
#if NeedFunctionPrototypes
    Display*            /* display */,
    unsigned int        /* resource */,
    int*                /* status */
#endif
);

extern int NXGetCollectInputFocusResource(
#if NeedFunctionPrototypes
    Display*            /* display */
#endif
);

extern int NXCollectInputFocus(
#if NeedFunctionPrototypes
    Display*            /* display */,
    unsigned int        /* resource */
#endif
);

extern int NXGetCollectedInputFocus(
#if NeedFunctionPrototypes
    Display*            /* display */,
    unsigned int        /* resource */,
    Window*             /* focus_return */,
    int*                /* revert_to_return */
#endif
);

#ifdef __cplusplus
}
#endif

#endif /* NXlib_H */
