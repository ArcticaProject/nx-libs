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

#include <sys/socket.h>

#ifndef __sun
#include <strings.h>
#endif

#include "NX.h"

#include "dix.h"
#include "os.h"

#define NEED_REPLIES

/*
 * Needed to enable definition of the callback
 * functions.
 */

#define NX_TRANS_SOCKET

#include "Xlib.h"
#include "Xutil.h"
#include "Xlibint.h"

#include "NXlib.h"
#include "NXproto.h"
#include "NXpack.h"

#include "Clean.h"
#include "Mask.h"
#include "Colormap.h"
#include "Alpha.h"
#include "Bitmap.h"
#include "Jpeg.h"
#include "Pgn.h"
#include "Rgb.h"
#include "Rle.h"
#include "Z.h"

#include "MD5.h"

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG
#undef  DUMP

/*
 * Maximum number of colors allowed in
 * Png encoding.
 */

#define NB_COLOR_MAX          256

/*
 * Dummy error handlers used internally to catch
 * Xlib failures in replies.
 */

static int _NXInternalReplyErrorFunction(Display *dpy, XErrorEvent *error);

static void _NXInternalLostSequenceFunction(Display *dpy, unsigned long newseq,
                                                unsigned long lastseq, unsigned int type);

/*
 * Resource ids that can be requested by
 * the client for use in split or unpack
 * operations.
 */

static unsigned char _NXSplitResources[NXNumberOfResources];
static unsigned char _NXUnpackResources[NXNumberOfResources];

static Display *_NXDisplayInitialized = NULL;

/*
 * Used in asynchronous handling of
 * GetImage replies.
 */

typedef struct
{
  unsigned long  sequence;
  unsigned int   resource;
  unsigned long  mask;
  int            format;
  int            width;
  int            height;
  _XAsyncHandler *handler;
  XImage         *image;
} _NXCollectImageState;

static _NXCollectImageState *_NXCollectedImages[NXNumberOfResources];

/*
 * Used in asynchronous handling of
 * GetProperty replies.
 */

typedef struct
{
  unsigned long  sequence;
  unsigned int   resource;
  Window         window;
  Atom           property;
  Atom           type;
  int            format;
  unsigned long  items;
  unsigned long  after;
  _XAsyncHandler *handler;
  char           *data;
} _NXCollectPropertyState;

static _NXCollectPropertyState *_NXCollectedProperties[NXNumberOfResources];

/*
 * Used in asynchronous handling of
 * GrabPointer replies.
 */

typedef struct
{
  unsigned long  sequence;
  unsigned int   resource;
  int            status;
  _XAsyncHandler *handler;
} _NXCollectGrabPointerState;

static _NXCollectGrabPointerState *_NXCollectedGrabPointers[NXNumberOfResources];

/*
 * Used in asynchronous handling of
 * GetInputFocus replies.
 */

typedef struct
{
  unsigned long  sequence;
  unsigned int   resource;
  Window         focus;
  int            revert_to;
  _XAsyncHandler *handler;
} _NXCollectInputFocusState;

static _NXCollectInputFocusState *_NXCollectedInputFocuses[NXNumberOfResources];

/*
 * Used by functions handling cache of
 * packed images.
 */

#define MD5_LENGTH            16

typedef struct
{
  md5_byte_t   *md5;
  XImage       *image;
  unsigned int method;
} _NXImageCacheEntry;

int NXImageCacheSize = 0;
int NXImageCacheHits = 0;
int NXImageCacheOps  = 0;

_NXImageCacheEntry *NXImageCache = NULL;

#ifdef DUMP

void _NXCacheDump(const char *label);

void _NXDumpData(const unsigned char *buffer, unsigned int size);

#endif

/*
 * From X11/PutImage.c.
 *
 * Cancel a GetReq operation, before doing
 * _XSend or Data.
 */

#if (defined(__STDC__) && !defined(UNIXCPP)) || defined(ANSICPP)
#define UnGetReq(name)\
    dpy->bufptr -= SIZEOF(x##name##Req);\
    dpy->request--
#else
#define UnGetReq(name)\
    dpy->bufptr -= SIZEOF(x/**/name/**/Req);\
    dpy->request--
#endif

#if (defined(__STDC__) && !defined(UNIXCPP)) || defined(ANSICPP)
#define UnGetEmptyReq()\
    dpy->bufptr -= 4;\
    dpy->request--
#else
#define UnGetEmptyReq(name)\
    dpy->bufptr -= 4;\
    dpy->request--
#endif

/*
 * From X11/ImUtil.c.
 */

extern int _XGetBitsPerPixel(Display *dpy, int depth);
extern int _XGetScanlinePad(Display *dpy, int depth);

#define ROUNDUP(nbytes, pad) (((nbytes) + ((pad) - 1)) & \
                                  ~(long)((pad) - 1))

static unsigned int DepthOnes(unsigned long mask)
{
  register unsigned long y;

  y = (mask >> 1) &033333333333;
  y = mask - y - ((y >>1) & 033333333333);
  return ((unsigned int) (((y + (y >> 3)) &
              030707070707) % 077));
}

#define CanMaskImage(image, mask) \
\
(image -> format == ZPixmap && mask != NULL && \
    (image -> depth == 32 || image -> depth == 24 || \
        (image -> depth == 16 && (image -> red_mask == 0xf800 && \
            image -> green_mask == 0x7e0 && image -> blue_mask == 0x1f))))

#define ShouldMaskImage(image, mask) (mask -> color_mask != 0xff)

/*
 * Initialize and reset the internal structures.
 */

extern int _NXInternalInitResources(Display *dpy);
extern int _NXInternalResetResources(Display *dpy);
extern int _NXInternalInitEncoders(Display *dpy);
extern int _NXInternalResetEncoders(Display *dpy);

int NXInitDisplay(Display *dpy)
{
  #ifdef TEST
  fprintf(stderr, "******NXInitDisplay: Called for display at [%p].\n", (void *) dpy);
  #endif

  if (_NXDisplayInitialized == NULL)
  {
    _NXInternalInitResources(dpy);

    _NXInternalInitEncoders(dpy);

    _NXDisplayInitialized = dpy;

    return 1;
  }

  #ifdef TEST
  fprintf(stderr, "******NXInitDisplay: WARNING! Internal structures already initialized.\n");
  #endif

  return 0;
}

int NXResetDisplay(Display *dpy)
{
  #ifdef TEST
  fprintf(stderr, "******NXResetDisplay: Called for display at [%p].\n", (void *) dpy);
  #endif

  if (_NXDisplayInitialized != NULL)
  {
    _NXInternalResetResources(dpy);

    _NXInternalResetEncoders(dpy);

    _NXDisplayInitialized = NULL;

    return 1;
  }

  #ifdef TEST
  fprintf(stderr, "******NXResetDisplay: WARNING! Internal structures already reset.\n");
  #endif

  return 0;
}

int _NXInternalInitResources(Display *dpy)
{
  return _NXInternalResetResources(dpy);
}

int _NXInternalResetResources(Display *dpy)
{
  int i;

  #ifdef TEST
  fprintf(stderr, "******_NXInternalResetResources: Clearing all the internal structures.\n");
  #endif

  for (i = 0; i < NXNumberOfResources; i++)
  {
    _NXSplitResources[i]  = 0;
    _NXUnpackResources[i] = 0;

    if (_NXCollectedImages[i] != NULL)
    {
      #ifdef TEST
      fprintf(stderr, "******_NXInternalResetResources: WARNING! Clearing collect image data "
                  "for resource [%d].\n", i);
      #endif

      if (_NXCollectedImages[i] -> handler != NULL)
      {
        DeqAsyncHandler(dpy, _NXCollectedImages[i] -> handler);

        Xfree(_NXCollectedImages[i] -> handler);
      }

      if (_NXCollectedImages[i] -> image != NULL)
      {
        XDestroyImage(_NXCollectedImages[i] -> image);
      }

      Xfree(_NXCollectedImages[i]);

      _NXCollectedImages[i] = NULL;
    }

    if (_NXCollectedProperties[i] != NULL)
    {
      #ifdef TEST
      fprintf(stderr, "******_NXInternalResetResources: WARNING! Clearing collect property data "
                  "for resource [%d].\n", i);
      #endif

      if (_NXCollectedProperties[i] -> handler != NULL)
      {
        DeqAsyncHandler(dpy, _NXCollectedProperties[i] -> handler);

        Xfree(_NXCollectedProperties[i] -> handler);
      }

      if (_NXCollectedProperties[i] -> data != NULL)
      {
        Xfree(_NXCollectedProperties[i] -> data);
      }

      Xfree(_NXCollectedProperties[i]);

      _NXCollectedProperties[i] = NULL;
    }

    if (_NXCollectedGrabPointers[i] != NULL)
    {
      #ifdef TEST
      fprintf(stderr, "******_NXInternalResetResources: WARNING! Clearing grab pointer data "
                  "for resource [%d].\n", i);
      #endif

      if (_NXCollectedGrabPointers[i] -> handler != NULL)
      {
        DeqAsyncHandler(dpy, _NXCollectedGrabPointers[i] -> handler);

        Xfree(_NXCollectedGrabPointers[i] -> handler);
      }

      Xfree(_NXCollectedGrabPointers[i]);

      _NXCollectedGrabPointers[i] = NULL;
    }

    if (_NXCollectedInputFocuses[i] != NULL)
    {
      #ifdef TEST
      fprintf(stderr, "******_NXInternalResetResources: WARNING! Clearing collect input focus data "
                  "for resource [%d].\n", i);
      #endif

      if (_NXCollectedInputFocuses[i] -> handler != NULL)
      {
        DeqAsyncHandler(dpy, _NXCollectedInputFocuses[i] -> handler);

        Xfree(_NXCollectedInputFocuses[i] -> handler);
      }

      Xfree(_NXCollectedInputFocuses[i]);

      _NXCollectedInputFocuses[i] = NULL;
    }
  }

  return 1;
}

int _NXInternalInitEncoders(Display *dpy)
{
  ZInitEncoder();

  return 1;
}

int _NXInternalResetEncoders(Display *dpy)
{
  ZResetEncoder();

  return 1;
}

int NXSetDisplayPolicy(Display *dpy, int policy)
{
  if (policy == NXPolicyImmediate)
  {
    return NXTransPolicy(NX_FD_ANY, NX_POLICY_IMMEDIATE);
  }
  else
  {
    return NXTransPolicy(NX_FD_ANY, NX_POLICY_DEFERRED);
  }
}

int NXSetDisplayBuffer(Display *dpy, int size)
{
  /*
   * This is not multi-thread safe, so,
   * if you have threads, be sure that
   * they are stopped.
   */

  char *buffer;

  XFlush(dpy);

  if (dpy -> bufmax - size == dpy -> buffer)
  {
    #ifdef TEST
    fprintf(stderr, "******NXSetDisplayBuffer: Nothing to do with buffer size matching.\n");
    #endif

    return 1;
  }
  else if (dpy -> bufptr != dpy -> buffer)
  {
    #ifdef PANIC
    fprintf(stderr, "******NXSetDisplayBuffer: PANIC! The display buffer is not empty.\n");
    #endif

    return -1;
  }
  else if ((buffer = Xcalloc(1, size)) == NULL)
  {
    #ifdef PANIC
    fprintf(stderr, "******NXSetDisplayBuffer: PANIC! Can't allocate [%d] bytes for the buffer.\n",
                size);
    #endif

    return -1;
  }

  if (dpy -> buffer != NULL)
  {
    Xfree(dpy -> buffer);
  }

  dpy -> buffer = buffer;
  dpy -> bufptr = dpy -> buffer;
  dpy -> bufmax = dpy -> bufptr + size;

  #ifdef TEST
  fprintf(stderr, "******NXSetDisplayBuffer: Set the display output buffer size to [%d].\n",
              size);
  #endif

  return 1;
}

/*
 * If set, the Popen() function in the X server
 * wil remove the LD_LIBRARY_PATH variable from
 * the environment before calling the execl()
 * function on the child process.
 */

int NXUnsetLibraryPath(int value)
{
  int previous = _NXUnsetLibraryPath;

  _NXUnsetLibraryPath = value;

  #ifdef TEST
  fprintf(stderr, "******NXUnsetLibraryPath: Set the flag to [%d] with previous value [%d].\n",
              value, previous);
  #endif

  return previous;
}

/*
 * If set, the Xlib I/O error handler will simply
 * return, instead of quitting the program. This
 * leaves to the application the responsibility
 * of checking the state of the XlibDisplayIOEr-
 * ror flag.
 */

int NXHandleDisplayError(int value)
{
  int previous = _NXHandleDisplayError;

  _NXHandleDisplayError = value;

  #ifdef TEST
  fprintf(stderr, "******NXHandleDisplayError: Set the flag to [%d] with previous value [%d].\n",
              value, previous);
  #endif

  return previous;
}

/*
 * Shutdown the display descriptor and force Xlib
 * to set the I/O error flag.
 */

Bool NXForceDisplayError(Display *dpy)
{
  if (dpy != NULL)
  {
    NXTransClose(dpy -> fd);

    if (!(dpy -> flags & XlibDisplayIOError))
    {
      shutdown(dpy -> fd, SHUT_RDWR);

      _XIOError(dpy);
    }

    return 1;
  }

  return 0;
}

/*
 * Check if the display has become invalid. Similarly
 * to the modified Xlib, we call the predicate funct-
 * ion with the value of the XlibDisplayIOError flag
 * only if the I/O error was not encountered already.
 * The application can use this function to query the
 * XlibDisplayIOError flag because Xlib doesn't expose
 * the internals of the display structure to the appli-
 * cation.
 */

int NXDisplayError(Display *dpy)
{
  if (dpy != NULL)
  {
    return (_XGetIOError(dpy) ||
                (_NXDisplayErrorFunction != NULL &&
                    (*_NXDisplayErrorFunction)(dpy, _XGetIOError(dpy))));
  }

  return 1;
}

/*
 * Various queries related to the state of the
 * display connection.
 */

int NXDisplayReadable(Display *dpy)
{
  int result;
  int readable;

  result = NXTransReadable(dpy -> fd, &readable);

  if (result == 0)
  {
    #ifdef DEBUG
    fprintf(stderr, "******NXDisplayReadable: Returning [%d] bytes readable from fd [%d].\n",
                readable, dpy -> fd);
    #endif

    return readable;
  }

  #ifdef DEBUG
  fprintf(stderr, "******NXDisplayReadable: WARNING! Error detected on display fd [%d].\n",
              dpy -> fd);
  #endif

  return -1;
}

int NXDisplayFlushable(Display *dpy)
{
  #ifdef DEBUG

  int flushable;

  flushable = NXTransFlushable(dpy -> fd) +
                  (dpy -> bufptr - dpy -> buffer);

  fprintf(stderr, "******NXDisplayFlushable: Returning [%d+%d=%d] bytes flushable "
              "to fd [%d].\n", (int) (dpy -> bufptr - dpy -> buffer),
                  (int) (flushable - (dpy -> bufptr - dpy -> buffer)),
                      flushable, dpy -> fd);

  return flushable;

  #else

  return NXTransFlushable(dpy -> fd) + (dpy -> bufptr - dpy -> buffer);

  #endif
}

int NXDisplayCongestion(Display *dpy)
{
  #ifdef DEBUG

  int congestion = NXTransCongestion(dpy -> fd);

  fprintf(stderr, "******NXDisplayCongestion: Returning [%d] as congestion level for fd [%d].\n",
              congestion, dpy -> fd);

  return congestion;

  #else

  return NXTransCongestion(dpy -> fd);

  #endif
}

int NXFlushDisplay(Display *dpy, int what)
{
  if (!(dpy -> flags & XlibDisplayWriting) &&
          dpy -> bufptr - dpy -> buffer > 0)
  {
    #ifdef DEBUG
    fprintf(stderr, "******NXFlushDisplay: Writing with [%d] bytes in the buffer.\n",
            (int) (dpy -> bufptr - dpy -> buffer));
    #endif

    XFlush(dpy);
  }

  if (what == NXFlushBuffer)
  {
    return 0;
  }

  #ifdef DEBUG
  fprintf(stderr, "******NXFlushDisplay: Flushing with [%d] bytes in the NX transport.\n",
              NXDisplayFlushable(dpy));
  #endif

  return NXTransFlush(dpy -> fd);
}

NXDisplayErrorPredicate NXSetDisplayErrorPredicate(NXDisplayErrorPredicate predicate)
{
  NXDisplayErrorPredicate previous = _NXDisplayErrorFunction;

  _NXDisplayErrorFunction = predicate;

  #ifdef TEST
  fprintf(stderr, "******NXSetDisplayErrorPredicate: Set the predicate to [%p] with previous value [%p].\n",
              predicate, previous);
  #endif

  return previous;
}

NXDisplayBlockHandler NXSetDisplayBlockHandler(NXDisplayBlockHandler handler)
{
  NXDisplayBlockHandler previous = _NXDisplayBlockFunction;

  _NXDisplayBlockFunction = handler;

  #ifdef TEST
  fprintf(stderr, "******NXSetDisplayBlockHandler: Set the handler to [%p] with previous value [%p].\n",
              handler, previous);
  #endif

  return previous;
}

NXDisplayWriteHandler NXSetDisplayWriteHandler(NXDisplayWriteHandler handler)
{
  NXDisplayWriteHandler previous = _NXDisplayWriteFunction;

  _NXDisplayWriteFunction = handler;

  #ifdef TEST
  fprintf(stderr, "******NXSetDisplayWriteHandler: Set the handler to [%p] with previous value [%p].\n",
              handler, previous);
  #endif

  return previous;
}

NXDisplayFlushHandler NXSetDisplayFlushHandler(NXDisplayFlushHandler handler, Display *display)
{
  NXDisplayFlushHandler previous = _NXDisplayFlushFunction;

  _NXDisplayFlushFunction = handler;

  NXTransHandler(NX_FD_ANY, NX_HANDLER_FLUSH,
                     (void (*)(void *, int)) handler, (void *) display);

  #ifdef TEST
  fprintf(stderr, "******NXSetDisplayFlushHandler: Set the handler to [%p] with display [%p] "
              "and previous value [%p].\n", handler, display, previous);
  #endif

  return previous;
}

NXDisplayStatisticsHandler NXSetDisplayStatisticsHandler(NXDisplayStatisticsHandler handler, char **buffer)
{
  NXDisplayStatisticsHandler previous = _NXDisplayStatisticsFunction;

  _NXDisplayStatisticsFunction = handler;

  /*
   * Propagate the handler.
   */

  NXTransHandler(NX_FD_ANY, NX_HANDLER_STATISTICS,
                     (void (*)(void *, int)) handler, (void *) buffer);

  #ifdef TEST
  fprintf(stderr, "******NXSetDisplayStatisticsHandler: Set the handler to [%p] with buffer pointer [%p] "
              "and previous value [%p].\n", handler, buffer, previous);
  #endif

  return previous;
}

NXLostSequenceHandler NXSetLostSequenceHandler(NXLostSequenceHandler handler)
{
  NXLostSequenceHandler previous = _NXLostSequenceFunction;

  _NXLostSequenceFunction = handler;

  #ifdef TEST
  fprintf(stderr, "******NXSetLostSequenceHandler: Set the handler to [%p] with previous value [%p].\n",
              handler, previous);
  #endif

  return previous;
}

int _NXInternalReplyErrorFunction(Display *dpy, XErrorEvent *error)
{
  #ifdef TEST
  fprintf(stderr, "******_NXInternalReplyErrorFunction: Internal error handler called.\n");
  #endif

  return 0;
}

void _NXInternalLostSequenceFunction(Display *dpy, unsigned long newseq,
                                         unsigned long lastseq, unsigned int type)
{
  #ifdef TEST

  fprintf(stderr, "******_NXInternalLostSequenceFunction: WARNING! Sequence lost with new "
              "sequence %ld last request %ld.\n", newseq, dpy -> request);

  /*
   * TODO: Reply or event info must be implemented.
   *
   * fprintf(stderr, "******_NXInternalLostSequenceFunction: WARNING! Expected event or reply "
   *             "was %ld with sequence %ld.\n", (long) rep -> type, (long) rep -> sequenceNumber);
   */

  fprintf(stderr, "******_NXInternalLostSequenceFunction: WARNING! Last sequence read "
              "was %ld display request is %ld.\n", lastseq & 0xffff, dpy -> request & 0xffff);

  #endif
}

Status NXGetControlParameters(Display *dpy, unsigned int *link_type, unsigned int *local_major,
                                  unsigned int *local_minor, unsigned int *local_patch,
                                      unsigned int *remote_major, unsigned int *remote_minor,
                                          unsigned int *remote_patch, int *split_timeout, int *motion_timeout,
                                              int *split_mode, int *split_size, unsigned int *pack_method,
                                                  unsigned int *pack_quality, int *data_level, int *stream_level,
                                                      int *delta_level, unsigned int *load_cache,
                                                          unsigned int *save_cache, unsigned int *startup_cache)
{
  xNXGetControlParametersReply rep;

  register xReq *req;

  LockDisplay(dpy);

  GetEmptyReq(NXGetControlParameters, req);

  #ifdef TEST
  fprintf(stderr, "******NXGetControlParameters: Sending message opcode [%d].\n",
              X_NXGetControlParameters);
  #endif

  if (_XReply(dpy, (xReply *) &rep, 0, xTrue) == xFalse)
  {
    #ifdef TEST
    fprintf(stderr, "******NXGetControlParameters: Error receiving reply.\n");
    #endif

    UnlockDisplay(dpy);

    SyncHandle();

    return 0;
  }

  #ifdef TEST
  fprintf(stderr, "******NXGetControlParameters: Got reply with link type [%u].\n", rep.linkType);

  fprintf(stderr, "******NXGetControlParameters: Local protocol major [%u] minor [%u] patch [%u].\n",
              rep.localMajor, rep.localMinor, rep.localPatch);

  fprintf(stderr, "******NXGetControlParameters: Remote protocol major [%u] minor [%u] patch [%u].\n",
              rep.remoteMajor, rep.remoteMinor, rep.remotePatch);

  fprintf(stderr, "******NXGetControlParameters: Split timeout [%d] motion timeout [%d].\n",
              (int) rep.splitTimeout, (int) rep.motionTimeout);

  fprintf(stderr, "******NXGetControlParameters: Split mode [%d] split size [%d].\n",
              (int) rep.splitMode, (int) rep.splitSize);

  fprintf(stderr, "******NXGetControlParameters: Preferred pack method [%d] pack quality [%d].\n",
              (int) rep.packMethod, (int) rep.packQuality);

  fprintf(stderr, "******NXGetControlParameters: Data level [%d] stream level [%d] delta level [%d].\n",
              rep.dataLevel, rep.streamLevel, rep.deltaLevel);
  #endif

  *link_type = rep.linkType;

  *local_major = rep.localMajor;
  *local_minor = rep.localMinor;
  *local_patch = rep.localPatch;

  *remote_major = rep.remoteMajor;
  *remote_minor = rep.remoteMinor;
  *remote_patch = rep.remotePatch;

  *split_timeout  = rep.splitTimeout;
  *motion_timeout = rep.motionTimeout;

  *split_mode = rep.splitMode;
  *split_size = rep.splitSize;

  *pack_method  = rep.packMethod;
  *pack_quality = rep.packQuality;

  *data_level   = rep.dataLevel;
  *stream_level = rep.streamLevel;
  *delta_level  = rep.deltaLevel;

  *load_cache    = rep.loadCache;
  *save_cache    = rep.saveCache;
  *startup_cache = rep.startupCache;

  UnlockDisplay(dpy);

  SyncHandle();

  /*
   * Install our internal out-of-sync handler.
   */

  _NXLostSequenceFunction = _NXInternalLostSequenceFunction;

  return 1;
}

/*
 * Which unpack methods are supported by the
 * remote proxy?
 */

Status NXGetUnpackParameters(Display *dpy, unsigned int *entries, unsigned char supported_methods[])
{
  register xNXGetUnpackParametersReq *req;

  xNXGetUnpackParametersReply rep;

  register unsigned n;

  #ifdef TEST
  register unsigned i;
  #endif

  if (*entries < NXNumberOfPackMethods)
  {
    #ifdef TEST
    fprintf(stderr, "******NXGetUnpackParameters: Requested only [%d] entries while they should be [%d].\n",
                *entries, NXNumberOfPackMethods);
    #endif

    return 0;
  }

  LockDisplay(dpy);

  GetReq(NXGetUnpackParameters, req);

  req -> entries = *entries;

  #ifdef TEST
  fprintf(stderr, "******NXGetUnpackParameters: Sending message opcode [%d] with [%d] requested entries.\n",
              X_NXGetUnpackParameters, *entries);
  #endif

  if (_XReply(dpy, (xReply *) &rep, 0, xFalse) == xFalse || rep.length == 0)
  {
    #ifdef TEST
    fprintf(stderr, "******NXGetUnpackParameters: Error receiving reply.\n");
    #endif

    UnlockDisplay(dpy);

    SyncHandle();

    return 0;
  }

  if ((n = rep.length << 2) > *entries)
  {
    #ifdef TEST
    fprintf(stderr, "******NXGetUnpackParameters: Got [%d] bytes of reply data while they should be [%d].\n",
                n, *entries);
    #endif

    _XEatData(dpy, (unsigned long) n);

    UnlockDisplay(dpy);

    SyncHandle();

    return 0;
  }

  *entries = n;

  #ifdef TEST
  fprintf(stderr, "******NXGetUnpackParameters: Reading [%d] bytes of reply data.\n", n);
  #endif

  _XReadPad(dpy, (char *) supported_methods, n);

  #ifdef TEST

  fprintf(stderr, "******NXGetUnpackParameters: Got reply with methods: ");

  for (i = 0; i < n; i++)
  {
    if (supported_methods[i] != 0)
    {
      fprintf(stderr, "[%d]", i);
    }
  }
  
  fprintf(stderr, ".\n");

  #endif

  UnlockDisplay(dpy);

  SyncHandle();

  return 1;
}

/*
 * Query and enable the MIT-SHM support between the
 * proxy and the X server. The 'enable' flags must be
 * true if shared memory PutImages and PutPackedImages
 * are desired. On return the flags will say if support
 * has been successfully enabled.
 *
 * Note that the the client part is not useful and not
 * implemented. The size of the segment is chosen by
 * the proxy. The main purpose of the message is to
 * reserve the XID that will be used by the remote.
 */

Status NXGetShmemParameters(Display *dpy, unsigned int *enable_client,
                                unsigned int *enable_server, unsigned int *client_segment,
                                    unsigned int *server_segment, unsigned int *client_size,
                                        unsigned int *server_size)
{
  register xNXGetShmemParametersReq *req;

  register int stage;

  xNXGetShmemParametersReply rep;

  /*
   * Save the previous handler.
   */

  int (*handler)(Display *, XErrorEvent *) = _XErrorFunction;

  *client_segment = 0;
  *server_segment = 0;

  if (*enable_client)
  {
    *client_segment = XAllocID(dpy);
  }

  if (*enable_server)
  {
    *server_segment = XAllocID(dpy);
  }

  LockDisplay(dpy);

  _XErrorFunction = _NXInternalReplyErrorFunction;

  for (stage = 0; stage < 3; stage++)
  {
    GetReq(NXGetShmemParameters, req);

    req -> stage = stage;

    req -> enableClient = (*enable_client != 0 ? 1 : 0);
    req -> enableServer = (*enable_server != 0 ? 1 : 0);

    req -> clientSegment = *client_segment;
    req -> serverSegment = *server_segment;

    #ifdef TEST
    fprintf(stderr, "******NXGetShmemParameters: Sending message opcode [%d] at stage [%d].\n",
                X_NXGetShmemParameters, stage);
    #endif

    #ifdef TEST

    if (stage == 0)
    {
      fprintf(stderr, "******NXGetShmemParameters: Enable client is [%u] enable server is [%u].\n",
                  *enable_client, *enable_server);

      fprintf(stderr, "******NXGetShmemParameters: Client segment is [%u] server segment is [%u].\n",
                  *client_segment, *server_segment);
    }

    #endif

    /*
     * There isn't X server reply in the second stage.
     * The procedure followed at X server side is:
     *
     * Stage 0: Send X_QueryExtension and masquerade
     *          the reply.
     *
     * Stage 1: Allocate the shared memory and send
     *          X_ShmAttach to the X server.
     *
     * Stage 2: Send X_GetInputFocus and masquerade
     *          the reply.
     *
     * The last message is used to force a reply and
     * collect any X error caused by a failure in the
     * shared memory initialization.
     */

    if (stage != 1)
    {
      /*
       * We are only interested in the final reply.
       */

      if (_XReply(dpy, (xReply *) &rep, 0, xTrue) == xFalse)
      {
        #ifdef TEST
        fprintf(stderr, "******NXGetShmemParameters: Error receiving reply.\n");
        #endif

        _XErrorFunction = handler;

        UnlockDisplay(dpy);

        SyncHandle();

        return 0;
      }
    }
  }

  /*
   * Return the settings to client.
   */

  *enable_client = rep.clientEnabled;
  *enable_server = rep.serverEnabled;

  *client_size = rep.clientSize;
  *server_size = rep.serverSize;

  #ifdef TEST
  fprintf(stderr, "******NXGetShmemParameters: Got final reply with enabled client [%u] and server [%u].\n",
              *enable_client, *enable_server);

  fprintf(stderr, "******NXGetShmemParameters: Client segment size [%u] server segment size [%u].\n",
              *client_size, *server_size);
  #endif

  _XErrorFunction = handler;

  UnlockDisplay(dpy);

  SyncHandle();

  return 1;
}

/*
 * Get the path to the font server that can be used by the X
 * server to tunnel the font connections across the NX link.
 * The path actually represents the TCP port where the proxy
 * on the NX client side is listening. The agent can tempora-
 * rily enable the tunneling when it needs a font that is not
 * available on the client, for example when the session is
 * migrated from a different X server.
 *
 * Note that it is not advisable to use the font server chan-
 * nel for other purposes than restoring a font that is found
 * missing at the time the session is migrated to a different
 * display. This is because the agent implements a caching of
 * the list of fonts supported by the client as it needs to
 * advertise only the fonts that can be opened at both sides.
 */

Status NXGetFontParameters(Display *dpy, unsigned int path_length, char path_data[])
{
  register xNXGetFontParametersReq *req;

  xNXGetFontParametersReply rep;

  register unsigned n;

  #ifdef TEST
  register unsigned i;
  #endif

  if (path_length < 1)
  {
    #ifdef TEST
    fprintf(stderr, "******NXGetFontParameters: No room to store the reply.\n");
    #endif

    return 0;
  }

  *path_data = '\0';

  LockDisplay(dpy);

  GetReq(NXGetFontParameters, req);

  #ifdef TEST
  fprintf(stderr, "******NXGetFontParameters: Sending message opcode [%d].\n",
              X_NXGetFontParameters);
  #endif

  if (_XReply(dpy, (xReply *) &rep, 0, xFalse) == xFalse || rep.length == 0)
  {
    #ifdef TEST
    fprintf(stderr, "******NXGetFontParameters: Error receiving reply.\n");
    #endif

    UnlockDisplay(dpy);

    SyncHandle();

    return 0;
  }

  if ((n = rep.length << 2) > path_length)
  {
    #ifdef TEST
    fprintf(stderr, "******NXGetFontParameters: Got [%d] bytes of reply data with only room for [%d].\n",
                n, path_length);
    #endif

    _XEatData(dpy, (unsigned long) n);

    UnlockDisplay(dpy);

    SyncHandle();

    return 0;
  }

  #ifdef TEST
  fprintf(stderr, "******NXGetFontParameters: Reading [%d] bytes of reply data.\n", n);
  #endif

  _XReadPad(dpy, (char *) path_data, n);

  /*
   * Check if the string can be fully
   * contained by the buffer.
   */

  if (*path_data > path_length - 1)
  {
    #ifdef TEST
    fprintf(stderr, "******NXGetFontParameters: Inconsistent length in the returned string.\n");
    #endif

    UnlockDisplay(dpy);

    SyncHandle();

    return 0;
  }

  #ifdef TEST

  fprintf(stderr, "******NXGetFontParameters: Got font path of [%d] bytes and value [",
              (int) *path_data);

  for (i = 0; i < *path_data; i++)
  {
    fprintf(stderr, "%c", *(path_data + i + 1));
  }
  
  fprintf(stderr, "].\n");

  #endif

  UnlockDisplay(dpy);

  SyncHandle();

  return 1;
}

unsigned int NXAllocSplit(Display *dpy, unsigned int resource)
{
  if (resource == NXAnyResource)
  {
    for (resource = 0; resource < NXNumberOfResources; resource++)
    {
      if (_NXSplitResources[resource] == 0)
      {
        _NXSplitResources[resource] = 1;

        #ifdef TEST
        fprintf(stderr, "******NXAllocSplit: Reserved resource [%u].\n",
                    resource);
        #endif
    
        return resource;
      }
    }

    #ifdef TEST
    fprintf(stderr, "******NXAllocSplit: WARNING! Resource limit exausted.\n");
    #endif

    return NXNoResource;
  }
  else if (resource >= 0 && resource < NXNumberOfResources)
  {
    #ifdef TEST

    if (_NXSplitResources[resource] == 0)
    {
      fprintf(stderr, "******NXAllocSplit: Reserved requested resource [%u].\n",
                  resource);
    }
    else
    {
      fprintf(stderr, "******NXAllocSplit: Requested resource [%u] already reserved.\n",
                  resource);
    }

    #endif
    
    _NXSplitResources[resource] = 1;
  }

  #ifdef PANIC
  fprintf(stderr, "******NXAllocSplit: PANIC! Can't reserve requested resource [%u].\n",
              resource);
  #endif

  return NXNoResource;
}

/*
 * Tell the proxy to split the next messages.
 */

int NXStartSplit(Display *dpy, unsigned int resource, unsigned int mode)
{
  register xNXStartSplitReq *req;

  LockDisplay(dpy);

  GetReq(NXStartSplit, req);

  req -> resource = resource;
  req -> mode     = mode;

  #ifdef TEST
  fprintf(stderr, "******NXStartSplit: Sending opcode [%d] with resource [%d] mode [%d].\n",
              X_NXStartSplit, resource, mode);
  #endif

  UnlockDisplay(dpy);

  SyncHandle();

  return 1;
}

/*
 * Send the closure of the split sequence and
 * tell the proxy to send the results.
 */

int NXEndSplit(Display *dpy, unsigned int resource)
{
  register xNXEndSplitReq *req;

  LockDisplay(dpy);

  GetReq(NXEndSplit, req);

  req -> resource = resource;

  #ifdef TEST
  fprintf(stderr, "******NXEndSplit: Sending opcode [%d] with resource [%d].\n",
              X_NXStartSplit, resource);
  #endif

  UnlockDisplay(dpy);

  SyncHandle();

  return 1;
}

/*
 * This message must be sent whenever the proxy notifies
 * the client of the completion of a split. If the 'pro-
 * pagate' field is 0, the proxy will not send the ori-
 * ginal request to the X server, but will only free the
 * internal state.
 */

int NXCommitSplit(Display *dpy, unsigned int resource, unsigned int propagate,
                      unsigned char request, unsigned int position)
{
  register xNXCommitSplitReq *req;

  LockDisplay(dpy);

  GetReq(NXCommitSplit, req);

  req -> resource  = resource;
  req -> propagate = propagate;
  req -> request   = request;
  req -> position  = position;

  #ifdef TEST
  fprintf(stderr, "******NXCommitSplit: Sending opcode [%d] with resource [%d] propagate [%d] "
              "request [%d] position [%d].\n", X_NXCommitSplit, resource,
                  propagate, request, position);
  #endif

  UnlockDisplay(dpy);

  SyncHandle();

  return 1;
}

int NXAbortSplit(Display *dpy, unsigned int resource)
{
  register xNXAbortSplitReq *req;

  LockDisplay(dpy);

  GetReq(NXAbortSplit, req);

  #ifdef TEST
  fprintf(stderr, "******NXAbortSplit: Sending message opcode [%d] with resource [%u].\n",
              X_NXAbortSplit, resource);
  #endif

  req -> resource = resource;

  UnlockDisplay(dpy);

  SyncHandle();

  return 1;
}

int NXFinishSplit(Display *dpy, unsigned int resource)
{
  register xNXFinishSplitReq *req;

  LockDisplay(dpy);

  GetReq(NXFinishSplit, req);

  #ifdef TEST
  fprintf(stderr, "******NXFinishSplit: Sending message opcode [%d] with resource [%u].\n",
              X_NXFinishSplit, resource);
  #endif

  req -> resource = resource;

  UnlockDisplay(dpy);

  SyncHandle();

  return 1;
}

int NXFreeSplit(Display *dpy, unsigned int resource)
{
  register xNXFreeSplitReq *req;

  if (_NXSplitResources[resource] != 0)
  {
    LockDisplay(dpy);

    GetReq(NXFreeSplit, req);

    #ifdef TEST
    fprintf(stderr, "******NXFreeSplit: Sending message opcode [%d] with resource [%u].\n",
                X_NXFreeSplit, resource);
    #endif

    req -> resource = resource;

    UnlockDisplay(dpy);

    SyncHandle();

    #ifdef TEST
    fprintf(stderr, "******NXFreeSplit: Making the resource [%u] newly available.\n",
                resource);
    #endif

    _NXSplitResources[resource] = 0;
  }
  #ifdef TEST
  else
  {
    fprintf(stderr, "******NXFreeSplit: Nothing to do for resource [%u].\n",
                resource);
  }
  #endif

  return 1;
}

/*
 * Tell to remote proxy to discard expose events
 * of one or more types.
 */

int NXSetExposeParameters(Display *dpy, int expose, int graphics_expose, int no_expose)
{
  register xNXSetExposeParametersReq *req;

  LockDisplay(dpy);

  GetReq(NXSetExposeParameters, req);

  req -> expose         = expose;
  req -> graphicsExpose = graphics_expose;
  req -> noExpose       = no_expose;

  #ifdef TEST
  fprintf(stderr, "******NXSetExposeParameters: Sending message opcode [%d] with flags [%d][%d][%d].\n",
              X_NXSetExposeParameters, req -> expose, req -> graphicsExpose, req -> noExpose);
  #endif

  UnlockDisplay(dpy);

  SyncHandle();

  return 1;
}

/*
 * Tell to the local proxy how to handle the next requests.
 */

int NXSetCacheParameters(Display *dpy, int enable_cache, int enable_split,
                             int enable_save, int enable_load)
{
  register xNXSetCacheParametersReq *req;

  LockDisplay(dpy);

  GetReq(NXSetCacheParameters, req);

  req -> enableCache = enable_cache;
  req -> enableSplit = enable_split;
  req -> enableSave  = enable_save;
  req -> enableLoad  = enable_load;

  #ifdef TEST
  fprintf(stderr, "******NXSetCacheParameters: Sending message opcode [%d] with "
              "flags [%d][%d][%d][%d].\n", X_NXSetCacheParameters, req -> enableCache,
                  req -> enableSplit, req -> enableSave, req -> enableLoad);
  #endif

  UnlockDisplay(dpy);

  SyncHandle();

  return 1;
}

unsigned int NXAllocUnpack(Display *dpy, unsigned int resource)
{
  if (resource == NXAnyResource)
  {
    for (resource = 0; resource < NXNumberOfResources; resource++)
    {
      if (_NXUnpackResources[resource] == 0)
      {
        _NXUnpackResources[resource] = 1;

        #ifdef TEST
        fprintf(stderr, "******NXAllocUnpack: Reserved resource [%u].\n",
                    resource);
        #endif
    
        return resource;
      }
    }

    #ifdef TEST
    fprintf(stderr, "******NXAllocUnpack: WARNING! Resource limit exausted.\n");
    #endif

    return NXNoResource;
  }
  else if (resource >= 0 && resource < NXNumberOfResources)
  {
    #ifdef TEST

    if (_NXUnpackResources[resource] == 0)
    {
      fprintf(stderr, "******NXAllocUnpack: Reserved requested resource [%u].\n",
                  resource);
    }
    else
    {
      fprintf(stderr, "******NXAllocUnpack: Requested resource [%u] already reserved.\n",
                  resource);
    }

    #endif
    
    _NXUnpackResources[resource] = 1;
  }

  #ifdef PANIC
  fprintf(stderr, "******NXAllocUnpack: PANIC! Can't reserve requested resource [%u].\n",
              resource);
  #endif

  return NXNoResource;
}

int NXSetUnpackGeometry(Display *dpy, unsigned int resource, Visual *visual)
{
  register xNXSetUnpackGeometryReq *req;

  LockDisplay(dpy);

  GetReq(NXSetUnpackGeometry, req);

  req -> resource = resource;

  req -> depth1Bpp  = _XGetBitsPerPixel(dpy, 1);
  req -> depth4Bpp  = _XGetBitsPerPixel(dpy, 4);
  req -> depth8Bpp  = _XGetBitsPerPixel(dpy, 8);
  req -> depth16Bpp = _XGetBitsPerPixel(dpy, 16);
  req -> depth24Bpp = _XGetBitsPerPixel(dpy, 24);
  req -> depth32Bpp = _XGetBitsPerPixel(dpy, 32);

  if (visual != NULL)
  {
    req -> redMask   = visual -> red_mask;
    req -> greenMask = visual -> green_mask;
    req -> blueMask  = visual -> blue_mask;
  }
  else
  {
    #ifdef PANIC
    fprintf(stderr, "******NXSetUnpackGeometry: PANIC! Can't set the geometry without a visual.\n");
    #endif

    UnGetReq(NXSetUnpackGeometry);

    UnlockDisplay(dpy);

    return -1;
  }

  #ifdef TEST
  fprintf(stderr, "******NXSetUnpackGeometry: Resource [%u] Depth/Bpp [1/%d][4/%d][8/%d]"
              "[16/%d][24/%d][32/%d].\n", resource, req -> depth1Bpp, req -> depth4Bpp,
                  req -> depth8Bpp, req -> depth16Bpp, req -> depth24Bpp, req -> depth32Bpp);

  fprintf(stderr, "******NXSetUnpackGeometry: red [0x%x] green [0x%x] blue [0x%x].\n",
               (unsigned) req -> redMask, (unsigned) req -> greenMask, (unsigned) req -> blueMask);
  #endif

  UnlockDisplay(dpy);

  SyncHandle();

  return 1;
}

/*
 * Store a colormap table on the remote side.
 * The colormap can then be used to unpack
 * an image.
 */

int NXSetUnpackColormap(Display *dpy, unsigned int resource, unsigned int method,
                            unsigned int entries, const char *data, unsigned int data_length)
{
  register xNXSetUnpackColormapReq *req;

  register int dst_data_length;

  LockDisplay(dpy);

  GetReq(NXSetUnpackColormap, req);

  req -> resource  = resource;
  req -> method    = method;

  req -> srcLength = data_length;
  req -> dstLength = entries << 2;

  dst_data_length = ROUNDUP(data_length, 4);

  req -> length += (dst_data_length >> 2);

  #ifdef TEST
  fprintf(stderr, "******NXSetUnpackColormap: Resource [%u] data size [%u] destination "
              "data size [%u].\n", resource, data_length, dst_data_length);
  #endif

  if (data_length > 0)
  {
    if (dpy -> bufptr + dst_data_length <= dpy -> bufmax)
    {
      /*
       * Clean the padding bytes in the request.
       */

      *((int *) (dpy -> bufptr + dst_data_length - 4)) = 0x0;

      memcpy(dpy -> bufptr, data, data_length);

      dpy -> bufptr += dst_data_length;
    }
    else
    {
      /*
       * The _XSend() will pad the request for us.
       */

      _XSend(dpy, data, data_length);
    }
  }

  UnlockDisplay(dpy);

  SyncHandle();

  return 1;
}

/*
 * Store data of the alpha blending channel
 * that will be combined with the next image
 * to be unpacked.
 */

int NXSetUnpackAlpha(Display *dpy, unsigned int resource, unsigned int method,
                         unsigned int entries, const char *data, unsigned int data_length)
{
  register xNXSetUnpackAlphaReq *req;

  register unsigned int dst_data_length;

  LockDisplay(dpy);

  GetReq(NXSetUnpackAlpha, req);

  req -> resource  = resource;
  req -> method    = method;

  req -> srcLength = data_length;
  req -> dstLength = entries;

  dst_data_length = ROUNDUP(data_length, 4);

  req -> length += (dst_data_length >> 2);

  #ifdef TEST
  fprintf(stderr, "******NXSetUnpackAlpha: Resource [%u] data size [%u] destination data size [%u].\n",
              resource, data_length, dst_data_length);
  #endif

  if (data_length > 0)
  {
    if (dpy -> bufptr + dst_data_length <= dpy -> bufmax)
    {
      /*
       * Clean the padding bytes in the request.
       */

      *((int *) (dpy -> bufptr + dst_data_length - 4)) = 0x0;

      memcpy(dpy -> bufptr, data, data_length);

      dpy -> bufptr += dst_data_length;
    }
    else
    {
      /*
       * The _XSend() will pad the request for us.
       */

      _XSend(dpy, data, data_length);
    }
  }

  UnlockDisplay(dpy);

  SyncHandle();

  return 1;
}

/*
 * Compatibility versions to be used when
 * connected to a 1.X.X proxy.
 */

/*
 * These are for compatibility with the 1.X.X
 * versions.
 */

#define sz_xNXSetUnpackColormapCompatReq        8

typedef struct _NXSetUnpackColormapCompatReq {
    CARD8  reqType;
    CARD8  resource;
    CARD16 length B16;
    CARD32 entries B32;
} xNXSetUnpackColormapCompatReq;

#define X_NXSetUnpackColormapCompat  X_NXSetUnpackColormap

int NXSetUnpackColormapCompat(Display *dpy, unsigned int resource,
                                  unsigned int entries, const char *data)
{
  register xNXSetUnpackColormapCompatReq *req;

  register char *dst_data;

  register int dst_data_length;

  #ifdef DUMP

  int i;

  #endif

  LockDisplay(dpy);

  GetReq(NXSetUnpackColormapCompat, req);

  req -> resource = resource;
  req -> entries  = entries;

  dst_data_length = entries << 2;

  req -> length += (dst_data_length >> 2);

  #ifdef TEST
  fprintf(stderr, "******NXSetUnpackColormapCompat: Resource [%u] number of entries [%u] "
              "destination data size [%u].\n", resource, entries, dst_data_length);
  #endif

  if (entries > 0)
  {
    if ((dpy -> bufptr + dst_data_length) <= dpy -> bufmax)
    {
      dst_data = dpy -> bufptr;
    }
    else
    {
      if ((dst_data = _XAllocScratch(dpy, dst_data_length)) == NULL)
      {
        #ifdef PANIC
        fprintf(stderr, "******NXSetUnpackColormapCompat: PANIC! Cannot allocate memory.\n");
        #endif

        UnGetReq(NXSetUnpackColormapCompat);

        UnlockDisplay(dpy);

        return -1;
      }
    }

    memcpy(dst_data, data, entries << 2);

    #ifdef DUMP

    fprintf(stderr, "******NXSetUnpackColormapCompat: Dumping colormap entries:\n");

    for (i = 0; i < entries; i++)
    {
      fprintf(stderr, "******NXSetUnpackColormapCompat: [%d] -> [0x%x].\n",
                  i, *((int *) (dst_data + (i * 4))));
    }

    #endif

    if (dst_data == dpy -> bufptr)
    {
      dpy -> bufptr += dst_data_length;
    }
    else
    {
      _XSend(dpy, dst_data, dst_data_length);
    }
  }

  UnlockDisplay(dpy);

  SyncHandle();

  return 1;
}

#define sz_xNXSetUnpackAlphaCompatReq  8

typedef struct _NXSetUnpackAlphaCompatReq {
    CARD8  reqType;
    CARD8  resource;
    CARD16 length B16;
    CARD32 entries B32;
} xNXSetUnpackAlphaCompatReq;

#define X_NXSetUnpackAlphaCompat  X_NXSetUnpackAlpha

int NXSetUnpackAlphaCompat(Display *dpy, unsigned int resource,
                               unsigned int entries, const char *data)
{
  register xNXSetUnpackAlphaCompatReq *req;

  register char *dst_data;

  register unsigned int dst_data_length;

  #ifdef DUMP

  int i;

  #endif

  LockDisplay(dpy);

  GetReq(NXSetUnpackAlphaCompat, req);

  req -> resource = resource;
  req -> entries  = entries;

  dst_data_length = ROUNDUP(entries, 4);

  req -> length += (dst_data_length >> 2);

  #ifdef TEST
  fprintf(stderr, "******NXSetUnpackAlphaCompat: Resource [%u] number of entries [%u] "
              "destination data size [%u].\n", resource, entries, dst_data_length);
  #endif

  if (entries > 0)
  {
    if ((dpy -> bufptr + dst_data_length) <= dpy -> bufmax)
    {
      dst_data = dpy -> bufptr;
    }
    else
    {
      if ((dst_data = _XAllocScratch(dpy, dst_data_length)) == NULL)
      {
        #ifdef PANIC
        fprintf(stderr, "******NXSetUnpackAlphaCompat: PANIC! Cannot allocate memory.\n");
        #endif

        UnGetReq(NXSetUnpackAlphaCompat);

        UnlockDisplay(dpy);

        return -1;
      }
    }

    memcpy(dst_data, data, entries);

    if (dst_data_length != entries)
    {
      memset(dst_data + entries, 0, dst_data_length - entries);
    }

    #ifdef DUMP

    fprintf(stderr, "******NXSetUnpackAlphaCompat: Dumping alpha channel data:\n");

    for (i = 0; i < dst_data_length; i++)
    {
      fprintf(stderr, "******NXSetUnpackAlphaCompat: [%d] -> [0x%02x].\n",
                  i, ((unsigned int) *(dst_data + i)) & 0xff);
    }

    #endif

    if (dst_data == dpy -> bufptr)
    {
      dpy -> bufptr += dst_data_length;
    }
    else
    {
      _XSend(dpy, dst_data, dst_data_length);
    }
  }

  UnlockDisplay(dpy);

  SyncHandle();

  return 1;
}

/*
 * Free any geometry, colormap and alpha channel
 * data stored by the remote proxy to unpack the
 * image. Resource, as usual, must be a value
 * between 0 and 255.
 */

int NXFreeUnpack(Display *dpy, unsigned int resource)
{
  register xNXFreeUnpackReq *req;

  if (_NXUnpackResources[resource] != 0)
  {
    LockDisplay(dpy);

    GetReq(NXFreeUnpack, req);

    #ifdef TEST
    fprintf(stderr, "******NXFreeUnpack: Sending message opcode [%d] with resource [%u].\n",
                X_NXFreeUnpack, resource);
    #endif

    req -> resource = resource;

    UnlockDisplay(dpy);

    SyncHandle();

    #ifdef TEST
    fprintf(stderr, "******NXFreeUnpack: Making the resource [%u] newly available.\n",
                resource);
    #endif

    _NXUnpackResources[resource] = 0;
  }
  #ifdef TEST
  else
  {
    fprintf(stderr, "******NXFreeUnpack: Nothing to do for resource [%u].\n",
                resource);
  }
  #endif

  return 1;
}

/*
 * Wrapper of XCreateImage(). Note that we use offset
 * field of XImage to store size of source image in
 * packed format. Note also that method is currently
 * not stored in the NXignored.
 */

NXPackedImage *NXCreatePackedImage(Display *dpy, Visual *visual, unsigned int method,
                                       unsigned int depth, int format, char *data,
                                           int data_length, unsigned int width,
                                               unsigned int height, int bitmap_pad,
                                                   int bytes_per_line)
{
  XImage* image;

  image = XCreateImage(dpy, visual, depth, format, 0, data,
                           width, height, bitmap_pad, bytes_per_line);

  if (image != NULL)
  {
    image -> xoffset = data_length;
  }

  return (NXPackedImage *) image;
}

/*
 * Wrapper of XDestroyImage().
 */

int NXDestroyPackedImage(NXPackedImage *image)
{
  return XDestroyImage((XImage *) image);
}

/*
 * Clean the image data directly in the current buffer.
 */

int NXCleanImage(XImage *image)
{
  #ifdef TEST
  fprintf(stderr, "******NXCleanImage: Cleaning image with format [%d] depth [%d] "
              "bits per pixel [%d].\n", image -> format, image -> depth,
                  image -> bits_per_pixel);
  #endif

  if (image -> format == ZPixmap)
  {
    if (image -> depth == 1)
    {
      return CleanXYImage(image);
    }
    else
    {
      return CleanZImage(image);
    }
  }
  else
  {
    return CleanXYImage(image);
  }
}

NXPackedImage *NXPackImage(Display *dpy, XImage *src_image, unsigned int method)
{
  XImage *dst_image;

  const ColorMask *mask;

  unsigned int dst_data_size;
  unsigned int dst_packed_data_size;

  unsigned int dst_bits_per_pixel;
  unsigned int dst_packed_bits_per_pixel;

  #ifdef TEST
  fprintf(stderr, "******NXPackImage: Going to pack a new image with method [%d].\n",
              method);
  #endif

  /*
   * Get the mask out of the method and 
   * check if the visual is supported by
   * the color reduction algorithm.
   */

  mask = MethodColorMask(method);

  if (mask == NULL)
  {
    #ifdef PANIC
    fprintf(stderr, "******NXPackImage: WARNING! No mask to apply for pack method [%d].\n",
                method);
    #endif

    return NULL;
  }
  else if (CanMaskImage(src_image, mask) == 0)
  {
    #ifdef PANIC
    fprintf(stderr, "******NXPackImage: PANIC! Invalid source with format [%d] depth [%d] bits per pixel [%d].\n",
                src_image -> format, src_image -> depth, src_image -> bits_per_pixel);

    fprintf(stderr, "******NXPackImage: PANIC! Visual colormask is red 0x%lx green 0x%lx blue 0x%lx.\n",
                src_image -> red_mask, src_image -> green_mask, src_image -> blue_mask);
    #endif

    return NULL;
  }

  /*
   * Create a destination image from
   * source and apply the color mask.
   */

  if ((dst_image = (XImage *) Xmalloc(sizeof(XImage))) == NULL)
  {
    #ifdef PANIC
    fprintf(stderr, "******NXPackImage: PANIC! Cannot allocate [%d] bytes for the image.\n",
                (int) sizeof(XImage));
    #endif

    return NULL;
  }

  *dst_image = *src_image;

  #ifdef TEST
  fprintf(stderr, "******NXPackImage: Source width [%d], bytes per line [%d] with depth [%d].\n",
              src_image -> width, src_image -> bytes_per_line, src_image -> depth);
  #endif

  dst_data_size = src_image -> bytes_per_line * src_image -> height;

  dst_image -> data = Xmalloc(dst_data_size);

  if (dst_image -> data == NULL)
  {
    #ifdef PANIC
    fprintf(stderr, "******NXPackImage: PANIC! Cannot allocate [%d] bytes for masked image data.\n",
                dst_data_size);
    #endif

    Xfree(dst_image);

    return NULL;
  }

  /*
   * If the pixel resulting from the mask
   * needs more bits than available, then
   * just clean the padding bits in the
   * image.
   */

  dst_bits_per_pixel = dst_image -> bits_per_pixel;
  dst_packed_bits_per_pixel = MethodBitsPerPixel(method);

  #ifdef TEST
  fprintf(stderr, "******NXPackImage: Destination depth [%d], bits per pixel [%d], packed bits per pixel [%d].\n",
             dst_image -> depth, dst_bits_per_pixel, dst_packed_bits_per_pixel);
  #endif

  if (dst_packed_bits_per_pixel > dst_bits_per_pixel ||
          ShouldMaskImage(src_image, mask) == 0)
  {
    /*
     * Should use the same data for source
     * and destination to avoid the memcpy.
     */

    if (CopyAndCleanImage(src_image, dst_image) <= 0)
    {
      #ifdef PANIC
      fprintf(stderr, "******NXPackImage: PANIC! Failed to clean the image.\n");
      #endif

      Xfree(dst_image -> data);

      Xfree(dst_image);

      return NULL;
    }
  }
  else if (MaskImage(mask, src_image, dst_image) <= 0)
  {
    #ifdef PANIC
    fprintf(stderr, "******NXPackImage: PANIC! Failed to apply the color mask.\n");
    #endif

    Xfree(dst_image -> data);

    Xfree(dst_image);

    return NULL;
  }

  /*
   * Let's pack the same pixels in fewer bytes.
   * Note that we save a new memory allocation
   * by using the same image as source and des-
   * tination. This means that PackImage() must
   * be able to handle ovelapping areas.
   */

  #ifdef TEST
  fprintf(stderr, "******NXPackImage: Plain bits per pixel [%d], data size [%d].\n",
              dst_bits_per_pixel, dst_data_size);
  #endif

  dst_packed_data_size = dst_data_size * dst_packed_bits_per_pixel /
                              dst_bits_per_pixel;

  #ifdef TEST
  fprintf(stderr, "******NXPackImage: Packed bits per pixel [%d], data size [%d].\n",
              dst_packed_bits_per_pixel, dst_packed_data_size);
  #endif

  if (PackImage(method, dst_data_size, dst_image,
                    dst_packed_data_size, dst_image) <= 0)
  {
    #ifdef PANIC
    fprintf(stderr, "******NXPackImage: PANIC! Failed to pack image from [%d] to [%d] bits per pixel.\n",
                dst_bits_per_pixel, dst_packed_bits_per_pixel);
    #endif

    Xfree(dst_image -> data);

    Xfree(dst_image);

    return NULL;
  }

  /*
   * Save data size in xoffset field
   * to comply with NX packed images.
   */

  dst_image -> xoffset = dst_packed_data_size;

  return dst_image;
}

/*
 * NXInPlacePackImage creates a NXPackedImage
 * from a XImage, sharing the same data buffer.
 * Is up to the caller to free the data buffer
 * only once.
 */

XImage *NXInPlacePackImage(Display *dpy, XImage *src_image, unsigned int method)
{
  XImage *dst_image;

  const ColorMask *mask;

  unsigned int dst_data_size;
  unsigned int dst_packed_data_size;

  unsigned int dst_bits_per_pixel;
  unsigned int dst_packed_bits_per_pixel;

  #ifdef TEST
  fprintf(stderr, "******NXInPlacePackImage: Going to pack a new image with method [%d].\n",
              method);
  #endif

  /*
   * Get mask out of method and check if
   * visual is supported by current color
   * reduction algorithm.
   */

  mask = MethodColorMask(method);

  if (mask == NULL)
  {
    #ifdef PANIC
    fprintf(stderr, "******NXInPlacePackImage: WARNING! No mask to apply for pack method [%d].\n",
                method);
    #endif

    return NULL;
  }
  else if (CanMaskImage(src_image, mask) == 0)
  {
    #ifdef PANIC
    fprintf(stderr, "******NXInPlacePackImage: PANIC! Invalid source with format [%d] depth [%d] bits per pixel [%d].\n",
                src_image -> format, src_image -> depth, src_image -> bits_per_pixel);

    fprintf(stderr, "******NXInPlacePackImage: PANIC! Visual colormask is red 0x%lx green 0x%lx blue 0x%lx.\n",
                src_image -> red_mask, src_image -> green_mask, src_image -> blue_mask);
    #endif
    return NULL;
  }

  /*
   * Create a destination image from
   * source and apply the color mask.
   */

  if ((dst_image = (XImage *) Xmalloc(sizeof(XImage))) == NULL)
  {
    #ifdef PANIC
    fprintf(stderr, "******NXInPlacePackImage: PANIC! Cannot allocate [%d] bytes for the image.\n",
                (int) sizeof(XImage));
    #endif

    return NULL;
  }

  *dst_image = *src_image;

  #ifdef TEST
  fprintf(stderr, "******NXInPlacePackImage: Source width [%d], bytes per line [%d] with depth [%d].\n",
              src_image -> width, src_image -> bytes_per_line, src_image -> depth);
  #endif

  dst_data_size = src_image -> bytes_per_line * src_image -> height;

  dst_image -> data = src_image -> data;

  /*
   * If pixel resulting from mask needs
   * more bits than available, then just
   * clean the pad bits in image.
   */

  dst_bits_per_pixel = dst_image -> bits_per_pixel;
  dst_packed_bits_per_pixel = MethodBitsPerPixel(method);

  #ifdef TEST
  fprintf(stderr, "******NXInPlacePackImage: Destination depth [%d], bits per pixel [%d], packed bits per pixel [%d].\n",
              dst_image -> depth, dst_bits_per_pixel, dst_packed_bits_per_pixel);
  #endif

  if (dst_packed_bits_per_pixel > dst_bits_per_pixel ||
          ShouldMaskImage(src_image, mask) == 0)
  {
    #ifdef TEST
    fprintf(stderr, "******NXInPlacePackImage: Just clean image packed_bits_per_pixel[%d], bits_per_pixel[%d].\n",
                dst_packed_bits_per_pixel, dst_bits_per_pixel);
    #endif

    if (NXCleanImage(dst_image) <= 0)
    {
      #ifdef PANIC
      fprintf(stderr, "******NXInPlacePackImage: PANIC! Failed to clean the image.\n");
      #endif

      Xfree(dst_image);

      return NULL;
    }
  }
  else if (MaskInPlaceImage(mask, dst_image) <= 0)
  {
    #ifdef PANIC
    fprintf(stderr, "******NXInPlacePackImage: PANIC! Failed to apply the color mask.\n");
    #endif

    Xfree(dst_image);

    return NULL;
  }

  /*
   * Let's pack the same pixels in fewer bytes.
   * Note that we save a new memory allocation
   * by using the same image as source and des-
   * tination. This means that PackImage() must
   * be able to handle ovelapping areas.
   */

  #ifdef TEST
  fprintf(stderr, "******NXInPlacePackImage: Plain bits per pixel [%d], data size [%d].\n",
             dst_bits_per_pixel, dst_data_size);
  #endif

  dst_packed_data_size = dst_data_size * dst_packed_bits_per_pixel /
    dst_bits_per_pixel;

  #ifdef TEST
  fprintf(stderr, "******NXInPlacePackImage: Packed bits per pixel [%d], data size [%d].\n",
             dst_packed_bits_per_pixel, dst_packed_data_size);
  #endif

  /*
   * Save data size in xoffset field
   * to comply with NX packed images.
   */

  dst_image -> xoffset = dst_packed_data_size;

  return dst_image;
}

int NXPutPackedImage(Display *dpy, unsigned int resource, Drawable drawable,
                         void *gc, NXPackedImage *image, unsigned int method,
                             unsigned int depth, int src_x, int src_y, int dst_x,
                                 int dst_y, unsigned int width, unsigned int height)
{
  register xNXPutPackedImageReq *req;

  register unsigned int src_data_length;
  register unsigned int dst_data_length;

  LockDisplay(dpy);

  FlushGC(dpy, (GC) gc);

  GetReq(NXPutPackedImage, req);

  req -> resource = resource;
  req -> drawable = drawable;
  req -> gc = ((GC) gc) -> gid;

  #ifdef TEST
  fprintf(stderr, "******NXPutPackedImage: Image resource [%d] drawable [%d] gc [%d].\n",
              req -> resource, (int) req -> drawable, (int) req -> gc);
  #endif

  /*
   * There is no leftPad field in request. We only
   * support a leftPad of 0. Anyway, X imposes a
   * leftPad of 0 in case of ZPixmap format.
   */

  req -> format = image -> format;

  /*
   * Source depth, as well as width and height,
   * are taken from the image structure.
   */

  req -> srcDepth = image -> depth;

  req -> srcX = src_x;
  req -> srcY = src_y;

  req -> srcWidth  = image -> width;
  req -> srcHeight = image -> height;

  /*
   * The destination depth is provided
   * by the caller.
   */

  req -> dstDepth = depth;

  req -> dstX = dst_x;
  req -> dstY = dst_y;

  req -> dstWidth  = width;
  req -> dstHeight = height;

  req -> method = method;

  #ifdef TEST
  fprintf(stderr, "******NXPutPackedImage: Source image depth [%d] destination depth [%d] "
              "method [%d].\n", req -> srcDepth, req -> dstDepth, req -> method);
  #endif

  /*
   * Source data length is the size of image in packed format,
   * as stored in xoffset field of XImage. Destination data
   * size is calculated according to bytes per line of target
   * image, so the caller must provide the right depth at the
   * time XImage structure is created.
   */

  req -> srcLength = image -> xoffset;

  if (image -> width == (int) width &&
          image -> height == (int) height)
  {
    req -> dstLength = image -> bytes_per_line * image -> height;
  }
  else if (image -> format == ZPixmap)
  {
    req -> dstLength = ROUNDUP((image -> bits_per_pixel * width),
                                    image -> bitmap_pad) * height >> 3;
  }
  else
  {
    req -> dstLength = ROUNDUP(width, image -> bitmap_pad) * height >> 3;
  }

  src_data_length = image -> xoffset;

  dst_data_length = ROUNDUP(src_data_length, 4);

  #ifdef TEST
  fprintf(stderr, "******NXPutPackedImage: Source data length [%d] request data length [%d].\n",
              src_data_length, dst_data_length);
  #endif

  req -> length += (dst_data_length >> 2);

  if (src_data_length > 0)
  {
    if (dpy -> bufptr + dst_data_length <= dpy -> bufmax)
    {
      /*
       * Clean the padding bytes in the request.
       */

      *((int *) (dpy -> bufptr + dst_data_length - 4)) = 0x0;

      memcpy(dpy -> bufptr, image -> data, src_data_length);

      dpy -> bufptr += dst_data_length;
    }
    else
    {
      /*
       * The _XSend() will pad the request for us.
       */

      _XSend(dpy, image -> data, src_data_length);
    }
  }

  UnlockDisplay(dpy);

  SyncHandle();

  return 1;
}

int NXAllocColors(Display *dpy, Colormap colormap, unsigned int entries,
                      XColor screens_in_out[], Bool results_in_out[])
{
  Status result = 0;
  xAllocColorReply rep;
  register xAllocColorReq *req;

  Bool alloc_error = False;

  register unsigned int i;

  LockDisplay(dpy);

  for (i = 0; i < entries; i++)
  {
    GetReq(AllocColor, req);

    req -> cmap  = colormap;

    req -> red   = screens_in_out[i].red;
    req -> green = screens_in_out[i].green;
    req -> blue  = screens_in_out[i].blue;
  }

  for (i = 0; i < entries; i++)
  {
    result = _XReply(dpy, (xReply *) &rep, 0, xTrue);

    if (result)
    {
      screens_in_out[i].pixel = rep.pixel;

      screens_in_out[i].red   = rep.red;
      screens_in_out[i].green = rep.green;
      screens_in_out[i].blue  = rep.blue;
      
      results_in_out[i] = True;
    } 
    else 
    {
      results_in_out[i] = False;

      alloc_error = True;
    }
  }

  UnlockDisplay(dpy);

  SyncHandle();

  return (alloc_error == False);
}

char *NXEncodeColormap(const char *src_data, unsigned int src_size, unsigned int *dst_size)
{
  return ColormapCompressData(src_data, src_size, dst_size);
}

char *NXEncodeAlpha(const char *src_data, unsigned int src_size, unsigned int *dst_size)
{
  return AlphaCompressData(src_data, src_size, dst_size);
}

NXPackedImage *NXEncodeRgb(XImage *src_image, unsigned int method, unsigned int quality)
{
  NXPackedImage *dst_image = NULL;

  unsigned int dst_size;

  /*
   * Create a new image structure as a copy
   * of the source.
   */

  if ((dst_image = (NXPackedImage *) Xmalloc(sizeof(NXPackedImage))) == NULL)
  {
    #ifdef PANIC
    fprintf(stderr, "******NXEncodeRgb: PANIC! Cannot allocate [%d] bytes for the image.\n",
                (int) sizeof(XImage));
    #endif

    return NULL;
  }

  *dst_image = *src_image;

  dst_image -> data = RgbCompressData(src_image, &dst_size);

  if (dst_image -> data == NULL)
  {
    #ifdef PANIC
    fprintf(stderr, "******NXEncodeRgb: PANIC! Rgb compression failed.\n");
    #endif

    Xfree(dst_image);

    return NULL;
  }

  /*
   * Store the Rgb size in the xoffset field.
   */

  dst_image -> xoffset = dst_size;

  return dst_image;
}

NXPackedImage *NXEncodeRle(XImage *src_image, unsigned int method, unsigned int quality)
{
  NXPackedImage *dst_image = NULL;

  unsigned int dst_size;

  /*
   * Create a new image structure as a copy
   * of the source.
   */

  if ((dst_image = (NXPackedImage *) Xmalloc(sizeof(NXPackedImage))) == NULL)
  {
    #ifdef PANIC
    fprintf(stderr, "******NXEncodeRle: PANIC! Cannot allocate [%d] bytes for the image.\n",
                (int) sizeof(XImage));
    #endif

    return NULL;
  }

  *dst_image = *src_image;

  dst_image -> data = RleCompressData(src_image, &dst_size);

  if (dst_image -> data == NULL)
  {
    #ifdef PANIC
    fprintf(stderr, "******NXEncodeRle: PANIC! Rle compression failed.\n");
    #endif

    Xfree(dst_image);

    return NULL;
  }

  /*
   * Store the Rle size in the xoffset field.
   */

  dst_image -> xoffset = dst_size;

  return dst_image;
}

NXPackedImage *NXEncodeBitmap(XImage *src_image, unsigned int method, unsigned int quality)
{
  NXPackedImage *dst_image = NULL;

  unsigned int dst_size;

  /*
   * Create a new image structure as a copy
   * of the source.
   */

  if ((dst_image = (NXPackedImage *) Xmalloc(sizeof(NXPackedImage))) == NULL)
  {
    #ifdef PANIC
    fprintf(stderr, "******NXEncodeBitmap: PANIC! Cannot allocate [%d] bytes for the image.\n",
                (int) sizeof(XImage));
    #endif

    return NULL;
  }

  *dst_image = *src_image;

  dst_image -> data = BitmapCompressData(src_image, &dst_size);

  if (dst_image -> data == NULL)
  {
    #ifdef PANIC
    fprintf(stderr, "******NXEncodeBitmap: PANIC! Bitmap compression failed.\n");
    #endif

    Xfree(dst_image);

    return NULL;
  }

  /*
   * Store the bitmap size in the xoffset field.
   */

  dst_image -> xoffset = dst_size;

  return dst_image;
}

NXPackedImage *NXEncodeJpeg(XImage *src_image, unsigned int method, unsigned int quality)
{
  NXPackedImage *dst_image = NULL;

  int size;

  /*
   * Check if the bpp of the image is valid
   * for the Jpeg compression.
   */

  if (src_image -> bits_per_pixel < 15)
  {
    #ifdef PANIC
    fprintf(stderr, "******NXEncodeJpeg: PANIC! Invalid bpp for Jpeg compression [%d]\n.",
                src_image -> bits_per_pixel);
    #endif

    return NULL;
  }

  /*
   * Create the destination image as a copy
   * of the source.
   */

  if ((dst_image = (NXPackedImage *) Xmalloc(sizeof(NXPackedImage))) == NULL)
  {
    #ifdef PANIC
    fprintf(stderr, "******NXEncodeJpeg: PANIC! Cannot allocate [%d] bytes for the Jpeg image.\n",
                (int) sizeof(NXPackedImage));
    #endif

    return NULL;
  }

  *dst_image = *src_image;

  dst_image -> data = JpegCompressData(src_image, quality, &size);

  if (dst_image -> data == NULL)
  {
    #ifdef PANIC
    fprintf(stderr, "******NXEncodeJpeg: PANIC! Jpeg compression failed.\n");
    #endif

    Xfree(dst_image);

    return NULL;
  }

  /*
   * Store the Jpeg size in the xoffset field.
   */

  dst_image -> xoffset = size;

  return dst_image;
}

NXPackedImage *NXEncodePng(XImage *src_image, unsigned int method, unsigned int quality)
{
  NXPackedImage *dst_image = NULL;

  int size;

  /*
   * Check if the bpp of the image is valid
   * for png compression.
   */

  if (src_image -> bits_per_pixel < 15)
  {
    #ifdef PANIC
    fprintf(stderr, "******NXEncodePng: PANIC! Invalid bpp for Png compression [%d].\n",
                src_image -> bits_per_pixel);
    #endif

    return NULL;
  }

  if ((dst_image = (NXPackedImage *) Xmalloc(sizeof(NXPackedImage))) == NULL)
  {
    #ifdef PANIC
    fprintf(stderr, "******NXEncodePng: PANIC! Cannot allocate [%d] bytes for the Png image.\n",
                (int) sizeof(NXPackedImage));
    #endif

    return NULL;
  }

  *dst_image = *src_image;

  dst_image -> data = PngCompressData(dst_image, &size);

  if (dst_image -> data == NULL)
  {
    #ifdef PANIC
    fprintf(stderr, "******NXEncodePng: PANIC! Png compression failed.\n");
    #endif

    Xfree(dst_image);

    return NULL;
  }

  /*
   * Store the Png size in the xoffset field.
   */

  dst_image -> xoffset = size;

  return dst_image;
}

int NXEncodeColors(XImage *src_image, NXColorTable *color_table, int nb_max)
{
  int x, y, t, p;

  long pixel;

  /*
   * We need a smarter way to extract
   * the colors from the image and
   * create a color table.
   */

  memset(color_table, 0, nb_max * sizeof(NXColorTable));

  for (x = 0, p = 0; x < src_image -> width; x++)
  {
    for (y = 0; y < src_image -> height; y++)
    {
      pixel  = XGetPixel(src_image, x, y);

      for (t = 0; t < nb_max; t++)
      {
        if ( color_table[t].found == 0)
        {
          color_table[t].pixel =  pixel;
          color_table[t].found =  1;

          p++;

          break;
        }
        else if ((color_table[t].pixel) == pixel)
        {
          break;
        }
      }

      if (p == nb_max)
      {
        return nb_max + 1;
      }
    }
  }

  return p;
}

void NXMaskImage(XImage *image, unsigned int method)
{
  unsigned int maskMethod;

  const ColorMask *mask;

  /*
   * Choose the correct mask method
   */

  switch(method)
  {
    case PACK_JPEG_8_COLORS:
    case PACK_PNG_8_COLORS:
    {
      maskMethod = MASK_8_COLORS;

      #ifdef DEBUG
      fprintf(stderr, "******NXMaskImage: Method is MASK_8_COLORS\n");
      #endif

      break;
    }
    case PACK_JPEG_64_COLORS:
    case PACK_PNG_64_COLORS:
    {
      maskMethod = MASK_64_COLORS;

      #ifdef DEBUG
      fprintf(stderr, "******NXMaskImage: Method is MASK_64K_COLORS\n");
      #endif

      break;
    }
    case PACK_JPEG_256_COLORS:
    case PACK_PNG_256_COLORS:
    {
      maskMethod = MASK_256_COLORS;

      #ifdef DEBUG
      fprintf(stderr, "******NXMaskImage: Method is MASK_256_COLORS\n");
      #endif

      break;
    }
    case PACK_JPEG_512_COLORS:
    case PACK_PNG_512_COLORS:
    {
      maskMethod = MASK_512_COLORS;

      #ifdef DEBUG
      fprintf(stderr, "******NXMaskImage: Method is MASK_512K_COLORS\n");
      #endif

      break;
    }
    case PACK_JPEG_4K_COLORS:
    case PACK_PNG_4K_COLORS:
    {
      maskMethod = MASK_4K_COLORS;

      #ifdef DEBUG
      fprintf(stderr, "******NXMaskImage: Method is MASK_4K_COLORS\n");
      #endif

      break;
    }
    case PACK_JPEG_32K_COLORS:
    case PACK_PNG_32K_COLORS:
    {
      maskMethod = MASK_32K_COLORS;

      #ifdef DEBUG
      fprintf(stderr, "******NXMaskImage: Method is MASK_32K_COLORS\n");
      #endif

      break;
    }
    case PACK_JPEG_64K_COLORS:
    case PACK_PNG_64K_COLORS:
    {
      maskMethod = MASK_64K_COLORS;

      #ifdef DEBUG
      fprintf(stderr, "******NXMaskImage: Method is MASK_64K_COLORS\n");
      #endif

      break;
    }
    case PACK_JPEG_256K_COLORS:
    case PACK_PNG_256K_COLORS:
    {
      maskMethod = MASK_256K_COLORS;

      #ifdef DEBUG
      fprintf(stderr, "******NXMaskImage: Method is MASK_256K_COLORS\n");
      #endif

      break;
    }
    case PACK_JPEG_2M_COLORS:
    case PACK_PNG_2M_COLORS:
    {
      maskMethod = MASK_2M_COLORS;

      #ifdef DEBUG
      fprintf(stderr, "******NXMaskImage: Method is MASK_2M_COLORS\n");
      #endif

      break;
    }
    case PACK_JPEG_16M_COLORS:
    case PACK_PNG_16M_COLORS:
    {
      maskMethod = MASK_16M_COLORS;

      #ifdef DEBUG
      fprintf(stderr, "******NXMaskImage: Method is MASK_16M_COLORS\n");
      #endif

      break;
    }
    default:
    {
      #ifdef PANIC
      fprintf(stderr, "******NXMaskImage: PANIC! Cannot find mask method for pack method [%d]\n",
                  method);
      #endif

      return;
    }
  }

  #ifdef TEST
  fprintf(stderr, "******NXMaskImage: packMethod[%d] => maskMethod[%d]\n",
              method, maskMethod);
  #endif

  /*
   * Get mask out of method and check if
   * visual is supported by current color
   * reduction algorithm.
   */

  mask = MethodColorMask(maskMethod);

  if (mask == NULL)
  {
    #ifdef PANIC
    fprintf(stderr, "******NXMaskImage: PANIC! No mask to apply for pack method [%d].\n",
                method);
    #endif

    return;
  }
  else if (CanMaskImage(image, mask) == 0)
  {
    #ifdef PANIC
    fprintf(stderr, "******NXMaskImage: PANIC! Invalid source with format [%d] depth [%d] bits per pixel [%d].\n",
                image -> format, image -> depth, image -> bits_per_pixel);

    fprintf(stderr, "******NXMaskImage: PANIC! Visual colormask is red 0x%lx green 0x%lx blue 0x%lx.\n",
                image -> red_mask, image -> green_mask, image -> blue_mask);
    #endif

    return;
  }

  /*
   * Calling ShouldMaskImage you get 0 in the case
   * of MASK_256_COLORS and MASK_64K_COLORS, which
   * means that the image should not be masked.
   */

  if (ShouldMaskImage(image, mask) == 0)
  {
    #ifdef TEST
    fprintf(stderr, "******NXMaskImage: the image will not be masked\n");
    #endif
  }
  else
  {
    if (MaskInPlaceImage(mask, image) <= 0)
    {
      #ifdef PANIC
      fprintf(stderr, "******NXMaskImage: PANIC! Failed to apply the color mask in place.\n");
      #endif
    }
  }
}

/*
 * The display parameter is ignored.
 */

void NXInitCache(Display *dpy, int entries)
{
  if (NXImageCache != NULL && NXImageCacheSize == entries)
  {
    #ifdef DEBUG
    fprintf(stderr, "******NXInitCache: Nothing to do with image cache at [%p] and [%d] entries.\n",
                NXImageCache,  NXImageCacheSize);
    #endif

    return;
  }

  #ifdef DEBUG
  fprintf(stderr, "******NXInitCache: Initializing the cache with [%d] entries.\n",
              entries);
  #endif

  NXImageCacheSize = 0;

  if (NXImageCache != NULL)
  {
    Xfree(NXImageCache);

    NXImageCache = NULL;
  }

  if (entries > 0)
  {
    NXImageCache = Xmalloc(entries * sizeof(_NXImageCacheEntry));

    if (NXImageCache != NULL)
    {
      memset(NXImageCache, 0, entries * sizeof(_NXImageCacheEntry));

      NXImageCacheSize = entries;

      #ifdef DEBUG
      fprintf(stderr, "******NXInitCache: Image cache initialized with [%d] entries.\n", entries);
      #endif
    }
  }
}

#ifdef DUMP

void _NXCacheDump(const char *label)
{
  char s[MD5_LENGTH * 2 + 1];

  int i;
  int j;

  #ifdef DEBUG
  fprintf(stderr, "%s: Dumping the content of image cache:\n", label);
  #endif

  for (i = 0; i < NXImageCacheSize; i++)
  {
    if (NXImageCache[i].image == NULL)
    {
      break;
    }

    for (j = 0; j < MD5_LENGTH; j++)
    {
      sprintf(s + (j * 2), "%02X", ((unsigned char *) NXImageCache[i].md5)[j]);
    }

    #ifdef DEBUG
    fprintf(stderr, "%s: [%d][%s].\n", label, i, s);
    #endif
  }
}

#endif

XImage *NXCacheFindImage(NXPackedImage *src_image, unsigned int *method, unsigned char **md5)
{
  md5_state_t  new_state;
  md5_byte_t   *new_md5;
  unsigned int data_size, i;

  if (NXImageCache == NULL)
  {
    return NULL;
  }

  /*
   * Will return the allocated checksum
   * if the image is not found.
   */

  *md5 = NULL;

  if ((new_md5 = Xmalloc(MD5_LENGTH)) == NULL)
  {
    #ifdef PANIC
    fprintf(stderr, "******NXCacheFindImage: Can't allocate memory for the checksum.\n");
    #endif

    return NULL;
  }

  data_size = (src_image -> bytes_per_line * src_image -> height);

  md5_init(&new_state);

  md5_append(&new_state, (unsigned char *) &src_image -> width,  sizeof(int));
  md5_append(&new_state, (unsigned char *) &src_image -> height, sizeof(int));

  md5_append(&new_state, (unsigned char *) src_image -> data, data_size);

  md5_finish(&new_state, new_md5);

  for (i = 0; i < NXImageCacheSize; i++)
  {
    if (NXImageCache[i].image != NULL)
    {
      if (memcmp(NXImageCache[i].md5, new_md5, MD5_LENGTH) == 0)
      {
        _NXImageCacheEntry found;

        found.image  = NXImageCache[i].image;
        found.method = NXImageCache[i].method;
        found.md5    = NXImageCache[i].md5;

        *method = found.method;

        NXImageCacheHits++;

        #ifdef DEBUG
        fprintf(stderr, "******NXCacheFindImage: Found at position [%d] with hits [%d] and [%d] packs.\n",
                    i, NXImageCacheHits, NXImageCacheOps);
        #endif

        Xfree(new_md5);

        /*
         * Move the images down one slot, from
         * the head of the list, and place the
         * image just found at top.
         */

        if (i > 16)
        {
          #ifdef DEBUG
          fprintf(stderr, "******NXCacheFindImage: Moving the image at the head of the list.\n");
          #endif

          memmove(&NXImageCache[1], &NXImageCache[0], (i * sizeof(_NXImageCacheEntry)));

          NXImageCache[0].image  = found.image;
          NXImageCache[0].method = found.method;
          NXImageCache[0].md5    = found.md5;

          #ifdef DUMP

          _NXCacheDump("******NXCacheFindImage");

          #endif
        }

        /*
         * Return the checksum and image
         * structure allocated in cache.
         */

        *md5 = found.md5;

        return found.image;
      }
    }
    else
    {
      break;
    }
  }

  *md5 = new_md5;

  return NULL;
}

/*
 * Add a packed image to the cache. A new image
 * structure is allocated and copied, data and
 * checksum are inherited from the passed image.
 */

int NXCacheAddImage(NXPackedImage *image, unsigned int method, unsigned char *md5)
{
  unsigned int i;

  if (image == NULL || image -> data == NULL)
  {
    #ifdef PANIC
    fprintf(stderr, "******NXCacheAddImage: PANIC! Invalid image passed to function.\n");
    #endif

    return -1;
  }

  i = (NXImageCacheOps < NXImageCacheSize) ? NXImageCacheOps : NXImageCacheSize;

  if (NXImageCacheOps >= NXImageCacheSize)
  {
    #ifdef DEBUG
    fprintf(stderr, "******NXCacheAddImage: Freeing up the oldest entry.\n");
    #endif

    i--;

    Xfree(NXImageCache[NXImageCacheSize - 1].image -> data);
    Xfree(NXImageCache[NXImageCacheSize - 1].image);
    Xfree(NXImageCache[NXImageCacheSize - 1].md5);
  }

  if (i > 0)
  {
    memmove(&NXImageCache[1], &NXImageCache[0], i * sizeof(_NXImageCacheEntry));
  }

  NXImageCacheOps++;

  #ifdef DEBUG
  fprintf(stderr, "******NXCacheAddImage: Going to add new image with data size [%d].\n",
              image -> xoffset);
  #endif

  NXImageCache[0].image  = image;
  NXImageCache[0].method = method;
  NXImageCache[0].md5    = md5;

  #ifdef DUMP

  _NXCacheDump("******NXCacheAddImage");

  #endif

  return 1;
}

/*
 * The display parameter is ignored.
 */

void NXFreeCache(Display *dpy)
{
  int i;

  if (NXImageCache == NULL)
  {
    #ifdef DEBUG
    fprintf(stderr, "******NXFreeCache: Nothing to do with a null image cache.\n");
    #endif

    return;
  }

  #ifdef DEBUG
  fprintf(stderr, "******NXFreeCache: Freeing the cache with [%d] entries.\n",
              NXImageCacheSize);
  #endif

  for (i = 0; i < NXImageCacheSize; i++)
  {
    if (NXImageCache[i].image != NULL)
    {
      if (NXImageCache[i].image -> data != NULL)
      {
        Xfree(NXImageCache[i].image -> data);
      }

      Xfree(NXImageCache[i].image);

      NXImageCache[i].image = NULL;
    }

    if (NXImageCache[i].md5 != NULL)
    {
      Xfree(NXImageCache[i].md5);

      NXImageCache[i].md5 = NULL;
    }
  }

  Xfree(NXImageCache);

  NXImageCache = NULL;

  NXImageCacheSize = 0;
  NXImageCacheHits = 0;
  NXImageCacheOps  = 0;
}

static void _NXNotifyImage(Display *dpy, int resource, Bool success)
{
  XEvent async_event;

  /*
   * Enqueue an event to tell client
   * the result of GetImage.
   */

  async_event.type = ClientMessage;

  async_event.xclient.serial = _NXCollectedImages[resource] -> sequence;

  async_event.xclient.window       = 0;
  async_event.xclient.message_type = 0;
  async_event.xclient.format       = 32;

  async_event.xclient.data.l[0] = NXCollectImageNotify;
  async_event.xclient.data.l[1] = resource;
  async_event.xclient.data.l[2] = success;

  XPutBackEvent(dpy, &async_event);
}

static Bool _NXCollectImageHandler(Display *dpy, xReply *rep, char *buf,
                                       int len, XPointer data)
{
  register _NXCollectImageState *state;

  register xGetImageReply *async_rep;

  char *async_head;
  char *async_data;

  int async_size;

  state = (_NXCollectImageState *) data;

  if ((rep -> generic.sequenceNumber % 65536) !=
          ((int)(state -> sequence) % 65536))
  {
    #ifdef TEST
    fprintf(stderr, "******_NXCollectImageHandler: Unmatched sequence [%d] for opcode [%d] "
                "with length [%d].\n", rep -> generic.sequenceNumber, rep -> generic.type,
                    (int) rep -> generic.length << 2);
    #endif

    return False;
  }

  #ifdef TEST
  fprintf(stderr, "******_NXCollectImageHandler: Going to handle asynchronous GetImage reply.\n");
  #endif

  /*
   * As even reply data is managed asynchronously,
   * we can use state to get to vector and vector
   * to get to handler. In this way, we can safely
   * dequeue and free the handler itself.
   */

  DeqAsyncHandler(dpy, state -> handler);

  Xfree(state -> handler);

  state -> handler = NULL;

  if (rep -> generic.type == X_Error)
  {
    #ifdef TEST
    fprintf(stderr, "******_NXCollectImageHandler: Error received from X server for resource [%d].\n",
                state -> resource);
    #endif

    _NXNotifyImage(dpy, state -> resource, False);

    _NXCollectedImages[state -> resource] = NULL;

    Xfree(state);

    return False;
  }

  #ifdef TEST
  fprintf(stderr, "******_NXCollectImageHandler: Matched request with sequence [%ld].\n",
              state -> sequence);
  #endif

  async_size = SIZEOF(xGetImageReply);

  async_head = Xmalloc(async_size);

  if (async_head == NULL)
  {
    #ifdef PANIC
    fprintf(stderr, "******_NXCollectImageHandler: PANIC! Failed to allocate memory with resource [%d].\n",
                state -> resource);
    #endif

    _NXNotifyImage(dpy, state -> resource, False);

    _NXCollectedImages[state -> resource] = NULL;

    Xfree(state);

    return False;
  }

  #ifdef TEST
  fprintf(stderr, "******_NXCollectImageHandler: Going to get reply with size [%d].\n",
              (int) rep -> generic.length << 2);
  #endif

  async_rep = (xGetImageReply *) _XGetAsyncReply(dpy, async_head, rep, buf, len, 0, False);

  if (async_rep == NULL)
  {
    #ifdef PANIC
    fprintf(stderr, "******_NXCollectImageHandler: PANIC! Failed to get reply with resource [%d].\n",
                state -> resource);
    #endif

    _NXNotifyImage(dpy, state -> resource, False);

    _NXCollectedImages[state -> resource] = NULL;

    Xfree(state);

    Xfree(async_head);

    return False;
  }

  #ifdef TEST
  fprintf(stderr, "******_NXCollectImageHandler: Got reply with depth [%d] visual [%d] size [%d].\n",
              async_rep -> depth, (int) async_rep -> visual, (int) async_rep -> length << 2);
  #endif

  async_size = async_rep -> length << 2;

  if (async_size > 0)
  {
    async_data = Xmalloc(async_size);

    if (async_data == NULL)
    {
      #ifdef PANIC
      fprintf(stderr, "******_NXCollectImageHandler: PANIC! Failed to allocate memory with resource [%d].\n",
                  state -> resource);
      #endif

      _NXNotifyImage(dpy, state -> resource, False);

      _NXCollectedImages[state -> resource] = NULL;

      Xfree(state);

      Xfree(async_head);

      return False;
    }

    #ifdef TEST
    fprintf(stderr, "******_NXCollectImageHandler: Going to get data with size [%d].\n",
                async_size);
    #endif

    _XGetAsyncData(dpy, async_data, buf, len, SIZEOF(xGetImageReply), async_size, async_size);

    /*
     * From now on we can return True, as all
     * data has been consumed from buffer.
     */

    if (state -> format == XYPixmap)
    {
      unsigned long depth = DepthOnes(state -> mask & (((unsigned long)0xFFFFFFFF) >>
                                          (32 - async_rep -> depth)));

      state -> image = XCreateImage(dpy, _XVIDtoVisual(dpy, async_rep -> visual),
                                        depth, XYPixmap, 0, async_data, state -> width,
                                            state -> height, dpy -> bitmap_pad, 0);
    }
    else
    {
      state -> image = XCreateImage(dpy, _XVIDtoVisual(dpy, async_rep -> visual),
                                        async_rep -> depth, ZPixmap, 0, async_data, state -> width,
                                            state -> height, _XGetScanlinePad(dpy, async_rep -> depth), 0);
    }

    if (state -> image == NULL)
    {
      #ifdef PANIC
      fprintf(stderr, "******_NXCollectImageHandler: PANIC! Failed to create image for resource [%d].\n",
                  state -> resource);
      #endif

      _NXNotifyImage(dpy, state -> resource, False);

      _NXCollectedImages[state -> resource] = NULL;

      Xfree(state);

      Xfree(async_head);
      Xfree(async_data);

      return True;
    }

    #ifdef TEST
    fprintf(stderr, "******_NXCollectImageHandler: Successfully stored image data for resource [%d].\n",
                state -> resource);
    #endif
  }
  #ifdef WARNING
  else
  {
    fprintf(stderr, "******_NXCollectImageHandler: WARNING! Null image data stored for resource [%d].\n",
                state -> resource);
  }
  #endif

  _NXNotifyImage(dpy, state -> resource, True);

  Xfree(async_head);

  return True;
}

int NXGetCollectImageResource(Display *dpy)
{
  int i;

  for (i = 0; i < NXNumberOfResources; i++)
  {
    if (_NXCollectedImages[i] == NULL)
    {
      return i;
    }
  }

  return -1;
}

int NXCollectImage(Display *dpy, unsigned int resource, Drawable drawable,
                       int src_x, int src_y, unsigned int width, unsigned int height,
                           unsigned long plane_mask, int format)
{
  register xGetImageReq *req;

  _NXCollectImageState *state;
  _XAsyncHandler *handler;

  if (resource >= NXNumberOfResources)
  {
    #ifdef PANIC
    fprintf(stderr, "******NXCollectImage: PANIC! Provided resource [%u] is out of range.\n",
                resource);
    #endif

    return -1;
  }

  state = _NXCollectedImages[resource];

  if (state != NULL)
  {
    #ifdef PANIC
    fprintf(stderr, "******NXCollectImage: PANIC! Having to remove previous state for resource [%u].\n",
                resource);
    #endif

    if (state -> handler != NULL)
    {
      DeqAsyncHandler(dpy, state -> handler);

      Xfree(state -> handler);
    }

    if (state -> image != NULL)
    {
      XDestroyImage(state -> image);
    }

    Xfree(state);

    _NXCollectedImages[resource] = NULL;
  }

  LockDisplay(dpy);

  GetReq(GetImage, req);

  req -> format    = format;
  req -> drawable  = drawable;
  req -> x         = src_x;
  req -> y         = src_y;
  req -> width     = width;
  req -> height    = height;
  req -> planeMask = plane_mask;

  #ifdef TEST
  fprintf(stderr, "******NXCollectImage: Sending message opcode [%d] sequence [%ld] for resource [%d].\n",
              X_GetImage, dpy -> request, resource);

  fprintf(stderr, "******NXCollectImage: Format [%d] drawable [%d] src_x [%d] src_y [%d].\n",
              req -> format, (int) req -> drawable, req -> x, req -> y);

  fprintf(stderr, "******NXCollectImage: Width [%d] height [%d] plane_mask [%x].\n",
              req -> width, req -> height, (int) req -> planeMask);
  #endif

  state   = Xmalloc(sizeof(_NXCollectImageState));
  handler = Xmalloc(sizeof(_XAsyncHandler));

  if (state == NULL || handler == NULL)
  {
    #ifdef PANIC
    fprintf(stderr, "******NXCollectImage: PANIC! Failed to allocate memory with resource [%d].\n",
                resource);
    #endif

    UnGetReq(GetImage);

    if (state != NULL)
    {
      Xfree(state);
    }

    if (handler != NULL)
    {
      Xfree(handler);
    }

    UnlockDisplay(dpy);

    return -1;
  }

  state -> sequence = dpy -> request;
  state -> resource = resource;
  state -> mask     = plane_mask;
  state -> format   = format;
  state -> width    = width;
  state -> height   = height;
  state -> image    = NULL;

  state -> handler = handler;

  handler -> next = dpy -> async_handlers;
  handler -> handler = _NXCollectImageHandler;
  handler -> data = (XPointer) state;
  dpy -> async_handlers = handler;

  _NXCollectedImages[resource] = state;

  UnlockDisplay(dpy);

  SyncHandle();

  return 1;
}

int NXGetCollectedImage(Display *dpy, unsigned int resource, XImage **image)
{
  register _NXCollectImageState *state;

  state = _NXCollectedImages[resource];

  if (state == NULL)
  {
    #ifdef PANIC
    fprintf(stderr, "******NXGetCollectedImage: PANIC! No image collected for resource [%u].\n",
                resource);
    #endif

    return 0;
  }

  _NXCollectedImages[resource] = NULL;

  *image = state -> image;

  Xfree(state);

  #ifdef TEST
  fprintf(stderr, "******NXGetCollectedImage: Returning GetImage data for resource [%u].\n",
              resource);
  #endif

  return 1;
}

static void _NXNotifyProperty(Display *dpy, int resource, Bool success)
{
  XEvent async_event;

  /*
   * Enqueue an event to tell client
   * the result of GetProperty.
   */

  async_event.type = ClientMessage;

  async_event.xclient.serial = _NXCollectedProperties[resource] -> sequence;

  async_event.xclient.window       = 0;
  async_event.xclient.message_type = 0;
  async_event.xclient.format       = 32;

  async_event.xclient.data.l[0] = NXCollectPropertyNotify;
  async_event.xclient.data.l[1] = resource;
  async_event.xclient.data.l[2] = success;

  XPutBackEvent(dpy, &async_event);
}

static Bool _NXCollectPropertyHandler(Display *dpy, xReply *rep, char *buf,
                                          int len, XPointer data)
{
  register _NXCollectPropertyState *state;

  register xGetPropertyReply *async_rep;

  char *async_head;
  char *async_data;

  int async_size;

  state = (_NXCollectPropertyState *) data;

  if ((rep -> generic.sequenceNumber % 65536) !=
          ((int)(state -> sequence) % 65536))
  {
    #ifdef TEST
    fprintf(stderr, "******_NXCollectPropertyHandler: Unmatched sequence [%d] for opcode [%d] "
                "with length [%d].\n", rep -> generic.sequenceNumber, rep -> generic.type,
                    (int) rep -> generic.length << 2);
    #endif

    return False;
  }

  #ifdef TEST
  fprintf(stderr, "******_NXCollectPropertyHandler: Going to handle asynchronous GetProperty reply.\n");
  #endif

  /*
   * Reply data is managed asynchronously. We can
   * use state to get to vector and vector to get
   * to handler. In this way, we can dequeue and
   * free the handler itself.
   */

  DeqAsyncHandler(dpy, state -> handler);

  Xfree(state -> handler);

  state -> handler = NULL;

  if (rep -> generic.type == X_Error)
  {
    #ifdef TEST
    fprintf(stderr, "******_NXCollectPropertyHandler: Error received from X server for resource [%d].\n",
                state -> resource);
    #endif

    _NXNotifyProperty(dpy, state -> resource, False);

    _NXCollectedProperties[state -> resource] = NULL;

    Xfree(state);

    return False;
  }

  #ifdef TEST
  fprintf(stderr, "******_NXCollectPropertyHandler: Matched request with sequence [%ld].\n",
              state -> sequence);
  #endif

  async_size = SIZEOF(xGetPropertyReply);

  async_head = Xmalloc(async_size);

  if (async_head == NULL)
  {
    #ifdef PANIC
    fprintf(stderr, "******_NXCollectPropertyHandler: PANIC! Failed to allocate memory with resource [%d].\n",
                state -> resource);
    #endif

    _NXNotifyProperty(dpy, state -> resource, False);

    _NXCollectedProperties[state -> resource] = NULL;

    Xfree(state);

    return False;
  }

  #ifdef TEST
  fprintf(stderr, "******_NXCollectPropertyHandler: Going to get reply with size [%d].\n",
              (int) rep -> generic.length << 2);
  #endif

  async_rep = (xGetPropertyReply *) _XGetAsyncReply(dpy, async_head, rep, buf, len, 0, False);

  if (async_rep == NULL)
  {
    #ifdef PANIC
    fprintf(stderr, "******_NXCollectPropertyHandler: PANIC! Failed to get reply with resource [%d].\n",
                state -> resource);
    #endif

    _NXNotifyProperty(dpy, state -> resource, False);

    _NXCollectedProperties[state -> resource] = NULL;

    Xfree(state);

    Xfree(async_head);

    return False;
  }

  #ifdef TEST
  fprintf(stderr, "******_NXCollectPropertyHandler: Got reply with format [%d] type [%d] size [%d].\n",
              async_rep -> format, (int) async_rep -> propertyType, (int) async_rep -> length << 2);

  fprintf(stderr, "******_NXCollectPropertyHandler: Bytes after [%d] number of items [%d].\n",
              (int) async_rep -> bytesAfter, (int) async_rep -> nItems);
  #endif

  state -> format = async_rep -> format;
  state -> type   = async_rep -> propertyType;
  state -> items  = async_rep -> nItems;
  state -> after  = async_rep -> bytesAfter;

  async_size = async_rep -> length << 2;

  if (async_size > 0)
  {
    async_data = Xmalloc(async_size);

    if (async_data == NULL)
    {
      #ifdef PANIC
      fprintf(stderr, "******_NXCollectPropertyHandler: PANIC! Failed to allocate memory with resource [%d].\n",
                  state -> resource);
      #endif

      _NXNotifyProperty(dpy, state -> resource, False);

      _NXCollectedProperties[state -> resource] = NULL;

      Xfree(state);

      Xfree(async_head);

      return False;
    }

    #ifdef TEST
    fprintf(stderr, "******_NXCollectPropertyHandler: Going to get data with size [%d].\n",
                async_size);
    #endif

    _XGetAsyncData(dpy, async_data, buf, len, SIZEOF(xGetPropertyReply), async_size, async_size);

    /*
     * From now on we can return True, as all
     * data has been consumed from buffer.
     */

    state -> data = async_data;

    #ifdef TEST
    fprintf(stderr, "******_NXCollectPropertyHandler: Successfully stored property data for resource [%d].\n",
                state -> resource);
    #endif
  }
  #ifdef TEST
  else
  {
    fprintf(stderr, "******_NXCollectPropertyHandler: WARNING! Null property data stored for resource [%d].\n",
                state -> resource);
  }
  #endif

  _NXNotifyProperty(dpy, state -> resource, True);

  Xfree(async_head);

  return True;
}

int NXGetCollectPropertyResource(Display *dpy)
{
  int i;

  for (i = 0; i < NXNumberOfResources; i++)
  {
    if (_NXCollectedProperties[i] == NULL)
    {
      return i;
    }
  }

  return -1;
}

int NXCollectProperty(Display *dpy, unsigned int resource, Window window, Atom property,
                          long long_offset, long long_length, Bool delete, Atom req_type)
{
  register xGetPropertyReq *req;

  _NXCollectPropertyState *state;
  _XAsyncHandler *handler;

  if (resource >= NXNumberOfResources)
  {
    #ifdef PANIC
    fprintf(stderr, "******NXCollectProperty: PANIC! Provided resource [%u] is out of range.\n",
                resource);
    #endif

    return -1;
  }

  state = _NXCollectedProperties[resource];

  if (state != NULL)
  {
    #ifdef PANIC
    fprintf(stderr, "******NXCollectProperty: PANIC! Having to remove previous state for resource [%u].\n",
                resource);
    #endif

    if (state -> handler != NULL)
    {
      DeqAsyncHandler(dpy, state -> handler);

      Xfree(state -> handler);
    }

    if (state -> data != NULL)
    {
      Xfree(state -> data);
    }

    Xfree(state);

    _NXCollectedProperties[resource] = NULL;
  }

  LockDisplay(dpy);

  GetReq(GetProperty, req);

  req -> delete     = delete;
  req -> window     = window;
  req -> property   = property;
  req -> type       = req_type;
  req -> longOffset = long_offset;
  req -> longLength = long_length;

  #ifdef TEST
  fprintf(stderr, "******NXCollectProperty: Sending message opcode [%d] sequence [%ld] for resource [%d].\n",
              X_GetProperty, dpy -> request, resource);

  fprintf(stderr, "******NXCollectProperty: Delete [%u] window [%d] property [%d] type [%d].\n",
              req -> delete, (int) req -> window, (int) req -> property, (int) req -> type);

  fprintf(stderr, "******NXCollectProperty: Long offset [%d] long length [%d].\n",
              (int) req -> longOffset, (int) req -> longLength);
  #endif

  state   = Xmalloc(sizeof(_NXCollectPropertyState));
  handler = Xmalloc(sizeof(_XAsyncHandler));

  if (state == NULL || handler == NULL)
  {
    #ifdef PANIC
    fprintf(stderr, "******NXCollectProperty: Failed to allocate memory with resource [%d].\n",
                resource);
    #endif

    if (state != NULL)
    {
      Xfree(state);
    }

    if (handler != NULL)
    {
      Xfree(handler);
    }

    UnGetReq(GetProperty);

    UnlockDisplay(dpy);

    return -1;
  }

  state -> sequence = dpy -> request;
  state -> resource = resource;
  state -> window   = window;
  state -> property = property;
  state -> type     = 0;
  state -> format   = 0;
  state -> items    = 0;
  state -> after    = 0;
  state -> data     = NULL;

  state -> handler = handler;

  handler -> next = dpy -> async_handlers;
  handler -> handler = _NXCollectPropertyHandler;
  handler -> data = (XPointer) state;
  dpy -> async_handlers = handler;

  _NXCollectedProperties[resource] = state;

  UnlockDisplay(dpy);

  SyncHandle();

  return True;
}

int NXGetCollectedProperty(Display *dpy, unsigned int resource, Atom *actual_type_return,
                               int *actual_format_return, unsigned long *nitems_return,
                                   unsigned long *bytes_after_return, unsigned char **data)
{
  register _NXCollectPropertyState *state;

  state = _NXCollectedProperties[resource];

  if (state == NULL)
  {
    #ifdef PANIC
    fprintf(stderr, "******NXGetCollectedProperty: PANIC! No data collected for resource [%u].\n",
                resource);
    #endif

    return 0;
  }

  *actual_type_return   = state -> type;
  *actual_format_return = state -> format;
  *nitems_return        = state -> items;
  *bytes_after_return   = state -> after;

  *data = (unsigned char *) _NXCollectedProperties[resource] -> data;

  Xfree(state);

  _NXCollectedProperties[resource] = NULL;

  #ifdef TEST
  fprintf(stderr, "******NXGetCollectedProperty: Returning GetProperty data for resource [%u].\n",
              resource);
  #endif

  return True;
}

static void _NXNotifyGrabPointer(Display *dpy, int resource, Bool success)
{
  XEvent async_event;

  async_event.type = ClientMessage;

  async_event.xclient.serial = _NXCollectedGrabPointers[resource] -> sequence;

  async_event.xclient.window       = 0;
  async_event.xclient.message_type = 0;
  async_event.xclient.format       = 32;

  async_event.xclient.data.l[0] = NXCollectGrabPointerNotify;
  async_event.xclient.data.l[1] = resource;
  async_event.xclient.data.l[2] = success;

  XPutBackEvent(dpy, &async_event);
}

static Bool _NXCollectGrabPointerHandler(Display *dpy, xReply *rep, char *buf,
                                             int len, XPointer data)
{
  register _NXCollectGrabPointerState *state;

  register xGrabPointerReply *async_rep;

  char *async_head;

  int async_size;

  state = (_NXCollectGrabPointerState *) data;

  if ((rep -> generic.sequenceNumber % 65536) !=
          ((int)(state -> sequence) % 65536))
  {
    #ifdef TEST
    fprintf(stderr, "******_NXCollectGrabPointerHandler: Unmatched sequence [%d] for opcode [%d] "
                "with length [%d].\n", rep -> generic.sequenceNumber, rep -> generic.type,
                    (int) rep -> generic.length << 2);
    #endif

    return False;
  }

  #ifdef TEST
  fprintf(stderr, "******_NXCollectGrabPointerHandler: Going to handle asynchronous GrabPointer reply.\n");
  #endif

  DeqAsyncHandler(dpy, state -> handler);

  Xfree(state -> handler);

  state -> handler = NULL;

  if (rep -> generic.type == X_Error)
  {
    #ifdef TEST
    fprintf(stderr, "******_NXCollectGrabPointerHandler: Error received from X server for resource [%d].\n",
                state -> resource);
    #endif

    _NXNotifyGrabPointer(dpy, state -> resource, False);

    _NXCollectedGrabPointers[state -> resource] = NULL;

    Xfree(state);

    return False;
  }

  #ifdef TEST
  fprintf(stderr, "******_NXCollectGrabPointerHandler: Matched request with sequence [%ld].\n",
              state -> sequence);
  #endif

  async_size = SIZEOF(xGrabPointerReply);

  async_head = Xmalloc(async_size);

  if (async_head == NULL)
  {
    #ifdef PANIC
    fprintf(stderr, "******_NXCollectGrabPointerHandler: PANIC! Failed to allocate memory with resource [%d].\n",
                state -> resource);
    #endif

    _NXNotifyGrabPointer(dpy, state -> resource, False);

    _NXCollectedGrabPointers[state -> resource] = NULL;

    Xfree(state);

    return False;
  }

  #ifdef TEST
  fprintf(stderr, "******_NXCollectGrabPointerHandler: Going to get reply with size [%d].\n",
              (int) rep -> generic.length << 2);
  #endif

  async_rep = (xGrabPointerReply *) _XGetAsyncReply(dpy, async_head, rep, buf, len, 0, False);

  if (async_rep == NULL)
  {
    #ifdef PANIC
    fprintf(stderr, "******_NXCollectGrabPointerHandler: PANIC! Failed to get reply with resource [%d].\n",
                state -> resource);
    #endif

    _NXNotifyGrabPointer(dpy, state -> resource, False);

    _NXCollectedGrabPointers[state -> resource] = NULL;

    Xfree(state);

    Xfree(async_head);

    return False;
  }

  #ifdef TEST
  fprintf(stderr, "******_NXCollectGrabPointerHandler: Got reply with status [%d] size [%d].\n",
              async_rep -> status, (int) async_rep -> length << 2);
  #endif

  state -> status = async_rep -> status;

  _NXNotifyGrabPointer(dpy, state -> resource, True);

  Xfree(async_head);

  return True;
}

int NXGetCollectGrabPointerResource(Display *dpy)
{
  int i;

  for (i = 0; i < NXNumberOfResources; i++)
  {
    if (_NXCollectedGrabPointers[i] == NULL)
    {
      return i;
    }
  }

  return -1;
}

int NXCollectGrabPointer(Display *dpy, unsigned int resource, Window grab_window, Bool owner_events,
                             unsigned int event_mask, int pointer_mode, int keyboard_mode,
                                 Window confine_to, Cursor cursor, Time time)
{
  register xGrabPointerReq *req;

  _NXCollectGrabPointerState *state;
  _XAsyncHandler *handler;

  if (resource >= NXNumberOfResources)
  {
    #ifdef PANIC
    fprintf(stderr, "******NXCollectGrabPointer: PANIC! Provided resource [%u] is out of range.\n",
                resource);
    #endif

    return -1;
  }

  state = _NXCollectedGrabPointers[resource];

  if (state != NULL)
  {
    #ifdef PANIC
    fprintf(stderr, "******NXCollectGrabPointer: PANIC! Having to remove previous state for resource [%u].\n",
                resource);
    #endif

    if (state -> handler != NULL)
    {
      DeqAsyncHandler(dpy, state -> handler);

      Xfree(state -> handler);
    }

    Xfree(state);

    _NXCollectedGrabPointers[resource] = NULL;
  }

  LockDisplay(dpy);

  GetReq(GrabPointer, req);

  req -> grabWindow   = grab_window;
  req -> ownerEvents  = owner_events;
  req -> eventMask    = event_mask;
  req -> pointerMode  = pointer_mode;
  req -> keyboardMode = keyboard_mode;
  req -> confineTo    = confine_to;
  req -> cursor       = cursor;
  req -> time         = time;

  #ifdef TEST
  fprintf(stderr, "******NXCollectGrabPointer: Sending message opcode [%d] sequence [%ld] "
              "for resource [%d].\n", X_GrabPointer, dpy -> request, resource);
  #endif

  state   = Xmalloc(sizeof(_NXCollectGrabPointerState));
  handler = Xmalloc(sizeof(_XAsyncHandler));

  if (state == NULL || handler == NULL)
  {
    #ifdef PANIC
    fprintf(stderr, "******NXCollectGrabPointer: Failed to allocate memory with resource [%d].\n",
                resource);
    #endif

    if (state != NULL)
    {
      Xfree(state);
    }

    if (handler != NULL)
    {
      Xfree(handler);
    }

    UnGetReq(GrabPointer);

    UnlockDisplay(dpy);

    return -1;
  }

  state -> sequence = dpy -> request;
  state -> resource = resource;
  state -> status   = 0;

  state -> handler = handler;

  handler -> next = dpy -> async_handlers;
  handler -> handler = _NXCollectGrabPointerHandler;
  handler -> data = (XPointer) state;
  dpy -> async_handlers = handler;

  _NXCollectedGrabPointers[resource] = state;

  UnlockDisplay(dpy);

  SyncHandle();

  return True;
}

int NXGetCollectedGrabPointer(Display *dpy, unsigned int resource, int *status)
{
  register _NXCollectGrabPointerState *state;

  state = _NXCollectedGrabPointers[resource];

  if (state == NULL)
  {
    #ifdef PANIC
    fprintf(stderr, "******NXGetCollectedGrabPointer: PANIC! No data collected for resource [%u].\n",
                resource);
    #endif

    return 0;
  }

  *status = state -> status;

  Xfree(state);

  _NXCollectedGrabPointers[resource] = NULL;

  #ifdef TEST
  fprintf(stderr, "******NXGetCollectedGrabPointer: Returning GrabPointer data for resource [%u].\n",
              resource);
  #endif

  return True;
}

static void _NXNotifyInputFocus(Display *dpy, int resource, Bool success)
{
  XEvent async_event;

  async_event.type = ClientMessage;

  async_event.xclient.serial = _NXCollectedInputFocuses[resource] -> sequence;

  async_event.xclient.window       = 0;
  async_event.xclient.message_type = 0;
  async_event.xclient.format       = 32;

  async_event.xclient.data.l[0] = NXCollectInputFocusNotify;
  async_event.xclient.data.l[1] = resource;
  async_event.xclient.data.l[2] = success;

  XPutBackEvent(dpy, &async_event);
}

static Bool _NXCollectInputFocusHandler(Display *dpy, xReply *rep, char *buf,
                                            int len, XPointer data)
{
  register _NXCollectInputFocusState *state;

  register xGetInputFocusReply *async_rep;

  char *async_head;

  int async_size;

  state = (_NXCollectInputFocusState *) data;

  if ((rep -> generic.sequenceNumber % 65536) !=
          ((int)(state -> sequence) % 65536))
  {
    #ifdef TEST
    fprintf(stderr, "******_NXCollectInputFocusHandler: Unmatched sequence [%d] for opcode [%d] "
                "with length [%d].\n", rep -> generic.sequenceNumber, rep -> generic.type,
                    (int) rep -> generic.length << 2);
    #endif

    return False;
  }

  #ifdef TEST
  fprintf(stderr, "******_NXCollectInputFocusHandler: Going to handle asynchronous GetInputFocus reply.\n");
  #endif

  DeqAsyncHandler(dpy, state -> handler);

  Xfree(state -> handler);

  state -> handler = NULL;

  if (rep -> generic.type == X_Error)
  {
    #ifdef TEST
    fprintf(stderr, "******_NXCollectInputFocusHandler: Error received from X server for resource [%d].\n",
                state -> resource);
    #endif

    _NXNotifyInputFocus(dpy, state -> resource, False);

    _NXCollectedInputFocuses[state -> resource] = NULL;

    Xfree(state);

    return False;
  }

  #ifdef TEST
  fprintf(stderr, "******_NXCollectInputFocusHandler: Matched request with sequence [%ld].\n",
              state -> sequence);
  #endif

  async_size = SIZEOF(xGetInputFocusReply);

  async_head = Xmalloc(async_size);

  if (async_head == NULL)
  {
    #ifdef PANIC
    fprintf(stderr, "******_NXCollectInputFocusHandler: PANIC! Failed to allocate memory with resource [%d].\n",
                state -> resource);
    #endif

    _NXNotifyInputFocus(dpy, state -> resource, False);

    _NXCollectedInputFocuses[state -> resource] = NULL;

    Xfree(state);

    return False;
  }

  #ifdef TEST
  fprintf(stderr, "******_NXCollectInputFocusHandler: Going to get reply with size [%d].\n",
              (int) rep -> generic.length << 2);
  #endif

  async_rep = (xGetInputFocusReply *) _XGetAsyncReply(dpy, async_head, rep, buf, len, 0, False);

  if (async_rep == NULL)
  {
    #ifdef PANIC
    fprintf(stderr, "******_NXCollectInputFocusHandler: PANIC! Failed to get reply with resource [%d].\n",
                state -> resource);
    #endif

    _NXNotifyInputFocus(dpy, state -> resource, False);

    _NXCollectedInputFocuses[state -> resource] = NULL;

    Xfree(state);

    Xfree(async_head);

    return False;
  }

  #ifdef TEST
  fprintf(stderr, "******_NXCollectInputFocusHandler: Got reply with focus [%d] revert to [%d] "
              "size [%d].\n", (int) async_rep -> focus, (int) async_rep -> revertTo,
                  (int) async_rep -> length << 2);
  #endif

  state -> focus     = async_rep -> focus;
  state -> revert_to = async_rep -> revertTo;

  _NXNotifyInputFocus(dpy, state -> resource, True);

  Xfree(async_head);

  return True;
}

int NXGetCollectInputFocusResource(Display *dpy)
{
  int i;

  for (i = 0; i < NXNumberOfResources; i++)
  {
    if (_NXCollectedInputFocuses[i] == NULL)
    {
      return i;
    }
  }

  return -1;
}

int NXCollectInputFocus(Display *dpy, unsigned int resource)
{
  register xReq *req;

  _NXCollectInputFocusState *state;
  _XAsyncHandler *handler;

  if (resource >= NXNumberOfResources)
  {
    #ifdef PANIC
    fprintf(stderr, "******NXCollectInputFocus: PANIC! Provided resource [%u] is out of range.\n",
                resource);
    #endif

    return -1;
  }

  state = _NXCollectedInputFocuses[resource];

  if (state != NULL)
  {
    #ifdef PANIC
    fprintf(stderr, "******NXCollectInputFocus: PANIC! Having to remove previous state for resource [%u].\n",
                resource);
    #endif

    if (state -> handler != NULL)
    {
      DeqAsyncHandler(dpy, state -> handler);

      Xfree(state -> handler);
    }

    Xfree(state);

    _NXCollectedInputFocuses[resource] = NULL;
  }

  LockDisplay(dpy);

  GetEmptyReq(GetInputFocus, req);

  #ifdef TEST
  fprintf(stderr, "******NXCollectInputFocus: Sending message opcode [%d] sequence [%ld] for resource [%d].\n",
              X_GetInputFocus, dpy -> request, resource);
  #endif

  state   = Xmalloc(sizeof(_NXCollectInputFocusState));
  handler = Xmalloc(sizeof(_XAsyncHandler));

  if (state == NULL || handler == NULL)
  {
    #ifdef PANIC
    fprintf(stderr, "******NXCollectInputFocus: Failed to allocate memory with resource [%d].\n",
                resource);
    #endif

    if (state != NULL)
    {
      Xfree(state);
    }

    if (handler != NULL)
    {
      Xfree(handler);
    }

    UnGetEmptyReq();

    UnlockDisplay(dpy);

    return -1;
  }

  state -> sequence  = dpy -> request;
  state -> resource  = resource;
  state -> focus     = 0;
  state -> revert_to = 0;

  state -> handler = handler;

  handler -> next = dpy -> async_handlers;
  handler -> handler = _NXCollectInputFocusHandler;
  handler -> data = (XPointer) state;
  dpy -> async_handlers = handler;

  _NXCollectedInputFocuses[resource] = state;

  UnlockDisplay(dpy);

  SyncHandle();

  return True;
}

int NXGetCollectedInputFocus(Display *dpy, unsigned int resource,
                                 Window *focus_return, int *revert_to_return)
{
  register _NXCollectInputFocusState *state;

  state = _NXCollectedInputFocuses[resource];

  if (state == NULL)
  {
    #ifdef PANIC
    fprintf(stderr, "******NXGetCollectedInputFocus: PANIC! No data collected for resource [%u].\n",
                resource);
    #endif

    return 0;
  }

  *focus_return     = state -> focus;
  *revert_to_return = state -> revert_to;

  Xfree(state);

  _NXCollectedInputFocuses[resource] = NULL;

  #ifdef TEST
  fprintf(stderr, "******NXGetCollectedInputFocus: Returning GetInputFocus data for resource [%u].\n",
              resource);
  #endif

  return True;
}

#ifdef DUMP

void _NXDumpData(const unsigned char *buffer, unsigned int size)
{
  if (buffer != NULL)
  {
    unsigned int i = 0;

    unsigned int ii;

    while (i < size)
    {
      fprintf(stderr, "[%d]\t", i);

      for (ii = 0; i < size && ii < 8; i++, ii++)
      {
        fprintf(stderr, "%d\t", (unsigned int) (buffer[i]));
      }

      fprintf(stderr, "\n");
    }
  }
}

#endif
