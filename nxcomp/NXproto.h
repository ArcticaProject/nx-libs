/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2010 NoMachine, http://www.nomachine.com/.         */
/*                                                                        */
/* NXCOMP, NX protocol compression and NX extensions to this software     */
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

#ifndef NXproto_H
#define NXproto_H

#ifdef __cplusplus
extern "C" {
#endif

#include <X11/X.h>
#include <X11/Xmd.h>
#include <X11/Xproto.h>

/*
 * Force the size to match the wire protocol.
 */
 
#define Drawable CARD32
#define GContext CARD32

#define sz_xNXGetControlParametersReq           4
#define sz_xNXGetCleanupParametersReq           4
#define sz_xNXGetImageParametersReq             4
#define sz_xNXGetUnpackParametersReq            8
#define sz_xNXGetShmemParametersReq             16
#define sz_xNXGetFontParametersReq              4
#define sz_xNXSetExposeParametersReq            8
#define sz_xNXSetCacheParametersReq             8
#define sz_xNXStartSplitReq                     8
#define sz_xNXEndSplitReq                       4
#define sz_xNXCommitSplitReq                    12
#define sz_xNXSetUnpackGeometryReq              24
#define sz_xNXSetUnpackColormapReq              16
#define sz_xNXSetUnpackAlphaReq                 16
#define sz_xNXPutPackedImageReq                 40
#define sz_xNXFreeUnpackReq                     4
#define sz_xNXFinishSplitReq                    4
#define sz_xNXAbortSplitReq                     4
#define sz_xNXFreeSplitReq                      4

#define sz_xGetControlParametersReply           32
#define sz_xGetCleanupParametersReply           32
#define sz_xGetImageParametersReply             32
#define sz_xGetUnpackParametersReply            32
#define sz_xGetShmemParametersReply             32

#define LINK_TYPE_LIMIT                         5

#define LINK_TYPE_NONE                          0
#define LINK_TYPE_MODEM                         1
#define LINK_TYPE_ISDN                          2
#define LINK_TYPE_ADSL                          3
#define LINK_TYPE_WAN                           4
#define LINK_TYPE_LAN                           5

/*
 * NX Replies.
 */

/*
 * The following reply has 4 new boolean
 * fields in the last protocol version.
 */

typedef struct _NXGetControlParametersReply {
    BYTE   type; /* Is X_Reply. */
    CARD8  linkType;
    CARD16 sequenceNumber B16;
    CARD32 length B32; /* Is 0. */
    CARD8  localMajor;
    CARD8  localMinor;
    CARD8  localPatch;
    CARD8  remoteMajor;
    CARD8  remoteMinor;
    CARD8  remotePatch;
    CARD16 splitTimeout B16;
    CARD16 motionTimeout B16;
    CARD8  splitMode;
    CARD8  pad1;
    CARD32 splitSize B32;
    CARD8  packMethod;
    CARD8  packQuality;
    CARD8  dataLevel;
    CARD8  streamLevel;
    CARD8  deltaLevel;
    CARD8  loadCache;
    CARD8  saveCache;
    CARD8  startupCache;
} xNXGetControlParametersReply;

typedef struct _NXGetCleanupParametersReply {
    BYTE   type; /* Is X_Reply. */
    BYTE   pad;
    CARD16 sequenceNumber B16;
    CARD32 length B32; /* Is 0. */
    BOOL   cleanGet;
    BOOL   cleanAlloc;
    BOOL   cleanFlush;
    BOOL   cleanSend;
    BOOL   cleanImages;
    BYTE   pad1, pad2, pad3;
    CARD32 pad4 B32;
    CARD32 pad5 B32;
    CARD32 pad6 B32;
    CARD32 pad7 B32;
} xNXGetCleanupParametersReply;

typedef struct _NXGetImageParametersReply {
    BYTE   type; /* Is X_Reply. */
    BYTE   pad;
    CARD16 sequenceNumber B16;
    CARD32 length B32; /* Is 0. */
    BOOL   imageSplit;
    BOOL   imageMask;
    BOOL   imageFrame;
    CARD8  imageMaskMethod;
    CARD8  imageSplitMethod;
    BYTE   pad1, pad2, pad3;
    CARD32 pad4 B32;
    CARD32 pad5 B32;
    CARD32 pad6 B32;
    CARD32 pad7 B32;
} xNXGetImageParametersReply;

/*
 * Data is made of PACK_METHOD_LIMIT values of
 * type BOOL telling which unpack capabilities
 * are implemented in proxy.
 */

typedef struct _NXGetUnpackParametersReply {
    BYTE   type; /* Is X_Reply. */
    BYTE   pad;
    CARD16 sequenceNumber B16;
    CARD32 length B32; /* Is PACK_METHOD_LIMIT / 4 from NXpack.h. */
    CARD8  entries;    /* Is PACK_METHOD_LIMIT. */
    BYTE   pad1, pad2, pad3;
    CARD32 pad4 B32;
    CARD32 pad5 B32;
    CARD32 pad6 B32;
    CARD32 pad7 B32;
    CARD32 pad8 B32;
} xNXGetUnpackParametersReply;

typedef struct _NXGetShmemParametersReply {
    BYTE   type;             /* Is X_Reply. */
    CARD8  stage;            /* As in the corresponding request. */
    CARD16 sequenceNumber B16;
    CARD32 length B32;       /* Is 0. */
    BOOL   clientEnabled;    /* SHM on path agent to proxy.      */
    BOOL   serverEnabled;    /* SHM on path proxy to X server.   */
    BYTE   pad1, pad2;       /* Previous values can be checked   */
    CARD32 clientSize B32;   /* at end of stage 2.               */
    CARD32 serverSize B32;
    CARD32 pad3 B32;
    CARD32 pad4 B32;
    CARD32 pad5 B32;
} xNXGetShmemParametersReply;

typedef struct _NXGetFontParametersReply {
    BYTE   type; /* Is X_Reply. */
    BYTE   pad1;
    CARD16 sequenceNumber B16;
    CARD32 length B32; /* Is length of path string + 1 / 4. */
    CARD32 pad2 B32;
    CARD32 pad3 B32;
    CARD32 pad4 B32;
    CARD32 pad5 B32;
    CARD32 pad6 B32;
    CARD32 pad7 B32;
} xNXGetFontParametersReply;

/*
 * NX Requests.
 */

typedef struct _NXGetControlParametersReq {
    CARD8  reqType;
    BYTE   pad;
    CARD16 length B16;
} xNXGetControlParametersReq;

typedef struct _NXGetCleanupParametersReq {
    CARD8  reqType;
    BYTE   pad;
    CARD16 length B16;
} xNXGetCleanupParametersReq;

typedef struct _NXGetImageParametersReq {
    CARD8  reqType;
    BYTE   pad;
    CARD16 length B16;
} xNXGetImageParametersReq;

typedef struct _NXGetUnpackParametersReq {
    CARD8  reqType;
    BYTE   pad;
    CARD16 length B16;
    CARD8  entries;
    BYTE   pad1, pad2, pad3;
} xNXGetUnpackParametersReq;

typedef struct _NXGetShmemParametersReq {
    CARD8    reqType;
    CARD8    stage;         /* It is between 0 and 2.     */
    CARD16   length B16;
    BOOL     enableClient;  /* X client side support is   */
    BOOL     enableServer;  /* not implemented yet.       */
    BYTE     pad1, pad2;
    CARD32   clientSegment; /* XID identifying the shared */
    CARD32   serverSegment; /* memory segments.           */
} xNXGetShmemParametersReq;

typedef struct _NXGetFontParametersReq {
    CARD8    reqType;
    CARD8    pad;
    CARD16   length B16;
} xNXGetFontParametersReq;

/*
 * The available split modes.
 */

#define NXSplitModeDefault              0
#define NXSplitModeAsync                1
#define NXSplitModeSync                 2

typedef struct _NXStartSplitReq {
    CARD8  reqType;
    CARD8  resource;
    CARD16 length B16;
    CARD8  mode;
    BYTE   pad1, pad2, pad3;
} xNXStartSplitReq;

typedef struct _NXEndSplitReq {
    CARD8  reqType;
    CARD8  resource;
    CARD16 length B16;
} xNXEndSplitReq;

typedef struct _NXCommitSplitReq {
    CARD8  reqType;
    CARD8  resource;
    CARD16 length B16;
    CARD8  propagate;
    CARD8  request;
    BYTE   pad1, pad2;
    CARD32 position B32;
} xNXCommitSplitReq;

typedef struct _NXFinishSplitReq {
    CARD8  reqType;
    CARD8  resource;
    CARD16 length B16;
} xNXFinishSplitReq;

typedef struct _NXAbortSplitReq {
    CARD8  reqType;
    CARD8  resource;
    CARD16 length B16;
} xNXAbortSplitReq;

typedef struct _NXFreeSplitReq {
    CARD8  reqType;
    CARD8  resource;
    CARD16 length B16;
} xNXFreeSplitReq;

typedef struct _NXSetExposeParametersReq {
    CARD8  reqType;
    BYTE   pad;
    CARD16 length B16;
    BOOL   expose;
    BOOL   graphicsExpose;
    BOOL   noExpose;
    BYTE   pad1;
} xNXSetExposeParametersReq;

typedef struct _NXSetCacheParametersReq {
    CARD8  reqType;
    BYTE   pad;
    CARD16 length B16;
    BOOL   enableCache;
    BOOL   enableSplit;
    BOOL   enableSave;
    BOOL   enableLoad;
} xNXSetCacheParametersReq;

typedef struct _NXSetUnpackGeometryReq {
    CARD8  reqType;
    CARD8  resource;
    CARD16 length B16;
    CARD8  depth1Bpp;
    CARD8  depth4Bpp;
    CARD8  depth8Bpp;
    CARD8  depth16Bpp;
    CARD8  depth24Bpp;
    CARD8  depth32Bpp;
    BYTE   pad1, pad2;
    CARD32 redMask B32;
    CARD32 greenMask B32;
    CARD32 blueMask B32;
} xNXSetUnpackGeometryReq;

typedef struct _NXSetUnpackColormapReq {
    CARD8  reqType;
    CARD8  resource;
    CARD16 length B16;
    CARD8  method;
    BYTE   pad1, pad2, pad3;
    CARD32 srcLength B32;
    CARD32 dstLength B32;
} xNXSetUnpackColormapReq;

typedef struct _NXSetUnpackAlphaReq {
    CARD8  reqType;
    CARD8  resource;
    CARD16 length B16;
    CARD8  method;
    BYTE   pad1, pad2, pad3;
    CARD32 srcLength B32;
    CARD32 dstLength B32;
} xNXSetUnpackAlphaReq;

typedef struct _NXPutPackedImageReq {
    CARD8    reqType;
    CARD8    resource;
    CARD16   length B16;
    Drawable drawable B32;
    GContext gc B32;
    CARD8    method;
    CARD8    format;
    CARD8    srcDepth;
    CARD8    dstDepth;
    CARD32   srcLength B32;
    CARD32   dstLength B32;
    INT16    srcX B16, srcY B16;
    CARD16   srcWidth B16, srcHeight B16;
    INT16    dstX B16, dstY B16;
    CARD16   dstWidth B16, dstHeight B16;
} xNXPutPackedImageReq;

typedef struct _NXFreeUnpackReq {
    CARD8  reqType;
    CARD8  resource;
    CARD16 length B16;
} xNXFreeUnpackReq;

/*
 * The X_NXSplitData and X_NXSplitEvent opcodes
 * are used internally and are ignored if coming
 * from the agent.
 */

#define X_NXInternalGenericData         0
#define X_NXInternalGenericReply        1
#define X_NXInternalGenericRequest      255

#define X_NXInternalShapeExtension      128
#define X_NXInternalRenderExtension     129

#define X_NXFirstOpcode                 230
#define X_NXLastOpcode                  252

#define X_NXGetControlParameters        230
#define X_NXGetCleanupParameters        231
#define X_NXGetImageParameters          232
#define X_NXGetUnpackParameters         233
#define X_NXStartSplit                  234
#define X_NXEndSplit                    235
#define X_NXSplitData                   236
#define X_NXCommitSplit                 237
#define X_NXSetExposeParameters         240
#define X_NXSetUnpackGeometry           241
#define X_NXSetUnpackColormap           242
#define X_NXPutPackedImage              243
#define X_NXSplitEvent                  244
#define X_NXGetShmemParameters          245
#define X_NXSetUnpackAlpha              246
#define X_NXFreeUnpack                  247
#define X_NXFinishSplit                 248
#define X_NXAbortSplit                  249
#define X_NXFreeSplit                   250
#define X_NXGetFontParameters           251
#define X_NXSetCacheParameters          252

/*
 * The following events are received by the agent
 * in the form of a ClientMessage with the value
 * 0 in fields atom and window. The format is
 * always 32. Event specific data starts at byte
 * offset 12.
 *
 * These events are sent by the NX transport to
 * notify the agent about the result of a split
 * operation.
 */

#define NXNoSplitNotify                 1
#define NXStartSplitNotify              2
#define NXCommitSplitNotify             3
#define NXEndSplitNotify                4
#define NXEmptySplitNotify              5

/*
 * Notifications of collect events. These events
 * don't come from the NX transport but are put
 * back in client's event queue by NXlib.
 */

#define NXCollectImageNotify            8
#define NXCollectPropertyNotify         9
#define NXCollectGrabPointerNotify      10
#define NXCollectInputFocusNotify       11

#undef Drawable
#undef GContext

#ifdef __cplusplus
}
#endif

#endif /* NXproto_H */
