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

#include "scrnintstr.h"
#include "miscstruct.h"
#include "pixmapstr.h"
#include "dixstruct.h"
#include "regionstr.h"
#include "../../include/gc.h"
#include "servermd.h"
#include "mi.h"

#include "../../fb/fb.h"

#include "Agent.h"
#include "Display.h"
#include "Screen.h"
#include "Pixmaps.h"
#include "Trap.h"
#include "GCs.h"
#include "GCOps.h"
#include "Image.h"
#include "Split.h"
#include "Drawable.h"
#include "Visual.h"
#include "Client.h"
#include "Events.h"
#include "Args.h"
#include "Utils.h"

#include "compext/Compext.h"
#include <nx/NXpack.h>

RESTYPE  RT_NX_PIXMAP;

/*
 * Set here the required log level.
 */

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG
#undef  DUMP

#ifdef TEST
#include "Font.h"
#endif

int nxagentPixmapPrivateIndex;

int nxagentCorruptedPixmaps;
int nxagentCorruptedBackgrounds;

/*
 * Force deallocation of the virtual pixmap.
 */

static Bool nxagentDestroyVirtualPixmap(PixmapPtr pPixmap);

/*
 * This serves as a tool to check the synchronization
 * between pixmaps in framebuffer and the correspondent
 * pixmaps in the real X server.
 */

#ifdef TEST
Bool nxagentCheckPixmapIntegrity(PixmapPtr pPixmap);
#endif

struct nxagentPixmapPair
{
  Pixmap pixmap;
  PixmapPtr pMap;
};

PixmapPtr nxagentCreatePixmap(ScreenPtr pScreen, int width, int height,
                              int depth, unsigned usage_hint)
{
  #ifdef DEBUG
  fprintf(stderr, "nxagentCreatePixmap: Creating pixmap with width [%d] "
              "height [%d] depth [%d] and allocation hint [%d].\n",
              width, height, depth, usage_hint);
  #endif

  /*
   * Create the pixmap structure but do not allocate memory for the
   * data.
   */

  PixmapPtr pPixmap = AllocatePixmap(pScreen, 0);

  if (!pPixmap)
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentCreatePixmap: WARNING! Failed to create pixmap with "
                "width [%d] height [%d] depth [%d] and allocation hint [%d].\n",
                width, height, depth, usage_hint);
    #endif

    return NullPixmap;
  }

  /*
   * Initialize the core members.
   */

  pPixmap -> drawable.type = DRAWABLE_PIXMAP;
  pPixmap -> drawable.class = 0;
  pPixmap -> drawable.pScreen = pScreen;
  pPixmap -> drawable.depth = depth;
  pPixmap -> drawable.bitsPerPixel = BitsPerPixel(depth);
  pPixmap -> drawable.id = 0;
  pPixmap -> drawable.serialNumber = NEXT_SERIAL_NUMBER;
  pPixmap -> drawable.x = 0;
  pPixmap -> drawable.y = 0;
  pPixmap -> drawable.width = width;
  pPixmap -> drawable.height = height;
  pPixmap -> devKind = 0;
  pPixmap -> refcnt = 1;
  pPixmap -> devPrivate.ptr = NULL;
  pPixmap -> usage_hint = usage_hint;

  /*
   * Initialize the privates of the real picture.
   */

  nxagentPrivPixmapPtr pPixmapPriv = nxagentPixmapPriv(pPixmap);

  pPixmapPriv -> isVirtual = False;
  pPixmapPriv -> isShared = nxagentShmPixmapTrap;

  /*
   * The shared memory pixmaps are never
   * synchronized with the remote X Server.
   */

  if (pPixmapPriv -> isShared == 1)
  {
    BoxRec box = { .x1 = 0, .y1 = 0, .x2 = width, .y2 = height };

    pPixmapPriv -> corruptedRegion = RegionCreate(&box, 1);
  }
  else
  {
    pPixmapPriv -> corruptedRegion = RegionCreate((BoxRec *) NULL, 1);
  }

  pPixmapPriv -> corruptedBackground = 0;

  pPixmapPriv -> containGlyphs = 0;
  pPixmapPriv -> containTrapezoids = 0;

  /*
   * The lazy encoding policy generally does not send on remote X
   * server the off-screen images, by preferring to synchronize the
   * windows content. Anyway this behaviour may be inadvisable if a
   * pixmap is used, for example, for multiple copy areas on screen.
   * This counter serves the purpose, taking into account the number
   * of times the pixmap has been used as source for a deferred
   * operation.
   */

  pPixmapPriv -> usageCounter = 0;

  pPixmapPriv -> corruptedBackgroundId = 0;
  pPixmapPriv -> corruptedId = 0;

  pPixmapPriv -> synchronizationBitmap = NullPixmap;

  pPixmapPriv -> corruptedTimestamp = 0;

  pPixmapPriv -> splitResource = NULL;

  pPixmapPriv -> isBackingPixmap = 0;

  /*
   * Create the pixmap based on the default windows. The proxy knows
   * this and uses this information to optimize encode the create
   * pixmap message by including the id of the drawable in the
   * checksum.
   */

  if (width != 0 && height != 0 && nxagentGCTrap == 0)
  {
    pPixmapPriv -> id = XCreatePixmap(nxagentDisplay,
                                      nxagentDefaultWindows[pScreen -> myNum],
                                      width, height, depth);
  }
  else
  {
    pPixmapPriv -> id = 0;

    #ifdef TEST
    fprintf(stderr, "nxagentCreatePixmap: Skipping the creation of pixmap at [%p] on real "
                "X server with nxagentGCTrap [%d].\n", (void *) pPixmap, nxagentGCTrap);
    #endif
  }

  pPixmapPriv -> mid = FakeClientID(serverClient -> index);

  AddResource(pPixmapPriv -> mid, RT_NX_PIXMAP, pPixmap);

  pPixmapPriv -> pRealPixmap = pPixmap;
  pPixmapPriv -> pVirtualPixmap = NULL;
  pPixmapPriv -> pPicture = NULL;

  /*
   * Create the pixmap in the virtual framebuffer.
   */

  PixmapPtr pVirtual = fbCreatePixmap(pScreen, width, height, depth, usage_hint);

  if (pVirtual == NULL)
  {
    #ifdef PANIC
    fprintf(stderr, "nxagentCreatePixmap: PANIC! Failed to create virtual pixmap with "
                "width [%d] height [%d] depth [%d] and allocation hint [%d].\n",
                width, height, depth, usage_hint);
    #endif

    nxagentDestroyPixmap(pPixmap);

    return NullPixmap;
  }

  #ifdef TEST
  fprintf(stderr, "nxagentCreatePixmap: Allocated memory for the Virtual %sPixmap %p of real Pixmap %p (%dx%d),",
              "allocation hint [%d].\n",
              nxagentShmPixmapTrap ? "Shm " : "", (void *) pVirtual, (void *) pPixmap, width, height, usage_hint);
  #endif

  pPixmapPriv -> pVirtualPixmap = pVirtual;

  /*
   * Initialize the privates of the virtual picture. We could avoid to
   * use a flag and just check the pointer to the virtual pixmap that,
   * if the pixmap is actually virtual, will be NULL. Unfortunately
   * the flag can be changed in nxagentValidateGC(). That code should
   * be removed in future.
   */

  nxagentPrivPixmapPtr pVirtualPriv = nxagentPixmapPriv(pVirtual);

  pVirtualPriv -> isVirtual = True;
  pVirtualPriv -> isShared = nxagentShmPixmapTrap;

  pVirtualPriv -> corruptedRegion = RegionCreate((BoxRec *) NULL, 1);

  pVirtualPriv -> corruptedBackground = 0;

  pVirtualPriv -> containGlyphs = 0;
  pVirtualPriv -> containTrapezoids = 0;

  pVirtualPriv -> usageCounter = 0;

  pVirtualPriv -> corruptedBackgroundId = 0;
  pVirtualPriv -> corruptedId = 0;

  pVirtualPriv -> synchronizationBitmap = NullPixmap;

  pVirtualPriv -> corruptedTimestamp = 0;

  pVirtualPriv -> splitResource = NULL;

  /*
   * We might distinguish real and virtual pixmaps by checking the
   * pointers to pVirtualPixmap. We should also remove the copy of id
   * and use the one of the real pixmap.
   */
   
  pVirtualPriv -> id = pPixmapPriv -> id;
  pVirtualPriv -> mid = 0;

  /*
   * Storing a pointer back to the real pixmap is silly. Unfortunately
   * this is the way it has been originally implemented. See also the
   * comment in destroy of the pixmap.
   */

  pVirtualPriv -> pRealPixmap = pPixmap;
  pVirtualPriv -> pVirtualPixmap = NULL;
  pVirtualPriv -> pPicture = NULL;

  #ifdef TEST
  fprintf(stderr, "nxagentCreatePixmap: Created pixmap at [%p] virtual at [%p] with width [%d] "
              "height [%d] depth [%d] and allocation hint [%d].\n",
              (void *) pPixmap, (void *) pVirtual, width, height, depth, usage_hint);
  #endif

  return pPixmap;
}

Bool nxagentDestroyPixmap(PixmapPtr pPixmap)
{
  if (!pPixmap)
  {
    #ifdef PANIC
    fprintf(stderr, "nxagentDestroyPixmap: PANIC! Invalid attempt to destroy "
                "a null pixmap pointer.\n");
    #endif

    return False;
  }

  nxagentPrivPixmapPtr pPixmapPriv = nxagentPixmapPriv(pPixmap);

  PixmapPtr pVirtual = pPixmapPriv -> pVirtualPixmap;

  #ifdef TEST
  fprintf(stderr, "nxagentDestroyPixmap: Destroying pixmap at [%p] with virtual at [%p].\n",
              (void *) pPixmap, (void *) pVirtual);
  #endif

  if (pPixmapPriv -> isVirtual)
  {
    /*
     * For some pixmaps we receive the destroy only for the
     * virtual. Infact to draw in the framebuffer we can use the
     * virtual pixmap instead of the pointer to the real one. As the
     * virtual pixmap can collect references, we must transfer those
     * references to the real pixmap so we can continue as the destroy
     * had been requested for it.
     */

    pVirtual = pPixmap;
    pPixmap  = pPixmapPriv -> pRealPixmap;

    pPixmapPriv = nxagentPixmapPriv(pPixmap);

    /*
     * Move the references accumulated by the virtual pixmap into the
     * references of the real one.
     */

    int refcnt = pVirtual -> refcnt - 1;

    #ifdef TEST
    fprintf(stderr, "nxagentDestroyPixmap: Adding [%d] references to pixmap at [%p].\n",
                refcnt, (void *) pPixmap);
    #endif

    pPixmap -> refcnt += refcnt;

    pVirtual -> refcnt -= refcnt;
  }

  --pPixmap -> refcnt;

  #ifdef TEST

  fprintf(stderr, "nxagentDestroyPixmap: Pixmap has now [%d] references with virtual pixmap [%d].\n",
              pPixmap -> refcnt, pVirtual -> refcnt);

  if (pVirtual != NULL && pVirtual -> refcnt != 1)
  {
    fprintf(stderr, "nxagentDestroyPixmap: PANIC! Virtual pixmap has [%d] references.\n",
                pVirtual -> refcnt);
  }

  #endif

  if (pPixmap -> refcnt > 0)
  {
    return True;
  }

  #ifdef TEST
  fprintf(stderr, "nxagentDestroyPixmap: Managing to destroy the pixmap at [%p]\n",
              (void *) pPixmap);
  #endif

  nxagentRemoveItemBSPixmapList(nxagentPixmap(pPixmap));

  nxagentDestroyVirtualPixmap(pPixmap);

  if (pPixmapPriv -> corruptedRegion != NullRegion)
  {
    RegionDestroy(pPixmapPriv -> corruptedRegion);

    pPixmapPriv -> corruptedRegion = NullRegion;
  }

  if (nxagentSynchronization.pDrawable == (DrawablePtr) pPixmap)
  {
    nxagentSynchronization.pDrawable = NULL;

    #ifdef TEST
    fprintf(stderr, "nxagentDestroyPixmap: Synchronization drawable [%p] removed from resources.\n",
                (void *) pPixmap);
    #endif
  }

  nxagentDestroyCorruptedResource((DrawablePtr) pPixmap, RT_NX_CORR_BACKGROUND);

  nxagentDestroyCorruptedResource((DrawablePtr) pPixmap, RT_NX_CORR_PIXMAP);

  nxagentDestroyDrawableBitmap((DrawablePtr) pPixmap);

  if (pPixmapPriv -> splitResource != NULL)
  {
    nxagentReleaseSplit((DrawablePtr) pPixmap);
  }

  /*
   * A pixmap with width and height set to 0 is
   * created at the beginning. To this pixmap is
   * not assigned an id. This is likely a scratch
   * pixmap used by the X server.
   */

  if (pPixmapPriv -> id)
  {
    XFreePixmap(nxagentDisplay, pPixmapPriv -> id);
  }

  if (pPixmapPriv -> mid)
  {
    FreeResource(pPixmapPriv -> mid, RT_NONE);
  }

  SAFE_free(pPixmap);

  return True;
}

Bool nxagentDestroyVirtualPixmap(PixmapPtr pPixmap)
{
  PixmapPtr pVirtual = nxagentPixmapPriv(pPixmap) -> pVirtualPixmap;

  /*
   * Force the routine to get rid of the virtual
   * pixmap.
   */

  if (pVirtual != NULL)
  {
    pVirtual -> refcnt = 1;

    nxagentPrivPixmapPtr pVirtualPriv = nxagentPixmapPriv(pVirtual);

    if (pVirtualPriv -> corruptedRegion != NullRegion)
    {
      RegionDestroy(pVirtualPriv -> corruptedRegion);

      pVirtualPriv -> corruptedRegion = NullRegion;
    }

    fbDestroyPixmap(pVirtual);
  }

  return True;
}

RegionPtr nxagentPixmapToRegion(PixmapPtr pPixmap)
{
  #ifdef TEST
  fprintf(stderr, "nxagentPixmapToRegion: Pixmap = [%p] nxagentVirtualPixmap = [%p]\n",
              (void *) pPixmap, (void *) nxagentVirtualPixmap(pPixmap));
  #endif

  return fbPixmapToRegion(nxagentVirtualPixmap(pPixmap));
}

Bool nxagentModifyPixmapHeader(PixmapPtr pPixmap, int width, int height, int depth,
                                   int bitsPerPixel, int devKind, void * pPixData)
{
  /*
   * See miModifyPixmapHeader() in miscrinit.c. This function is used
   * to recycle the scratch pixmap for this screen. We let it refer to
   * the virtual pixmap.
   */

  if (!pPixmap)
  {
    return False;
  }

  if (nxagentPixmapIsVirtual(pPixmap))
  {
    #ifdef PANIC
    fprintf(stderr, "nxagentModifyPixmapHeader: PANIC! Pixmap at [%p] is virtual.\n",
                (void *) pPixmap);
    #endif

    FatalError("nxagentModifyPixmapHeader: PANIC! Pixmap is virtual.");
  }

  PixmapPtr pVirtualPixmap = nxagentVirtualPixmap(pPixmap);

  #ifdef TEST
  fprintf(stderr, "nxagentModifyPixmapHeader: Pixmap at [%p] Virtual at [%p].\n",
              (void *) pPixmap, (void *) pVirtualPixmap);

  fprintf(stderr, "nxagentModifyPixmapHeader: Pixmap has width [%d] height [%d] depth [%d] "
              "bits-per-pixel [%d] devKind [%d] pPixData [%p].\n", pPixmap->drawable.width,
                  pPixmap->drawable.height, pPixmap->drawable.depth, pPixmap->drawable.bitsPerPixel,
                      pPixmap->devKind, (void *) pPixmap->devPrivate.ptr);

  fprintf(stderr, "nxagentModifyPixmapHeader: New parameters are width [%d] height [%d] depth [%d] "
              "bits-per-pixel [%d] devKind [%d] pPixData [%p].\n", width, height, depth,
                  bitsPerPixel, devKind, (void *) pPixData);
  #endif

  /*
   * ignore return code, because the only case where this will return
   * FALSE is pPixmap == NULL, which we have already caught above.
   */
  miModifyPixmapHeader(pPixmap, width, height, depth, bitsPerPixel, devKind, pPixData);
  miModifyPixmapHeader(pVirtualPixmap, width, height, depth, bitsPerPixel, devKind, pPixData);

  #ifdef PANIC

  if (!((width > 0) && (height > 0) && (depth > 0) && (bitsPerPixel > 0) &&
	(devKind > 0) && pPixData))
  {
    if (pPixmap->drawable.x != 0 || pPixmap->drawable.y != 0)
    {
      fprintf(stderr, "nxagentModifyPixmapHeader: PANIC! Pixmap at [%p] has x [%d] and y [%d].\n",
                  (void *) pPixmap, pPixmap->drawable.x, pPixmap->drawable.y);

      FatalError("nxagentModifyPixmapHeader: PANIC! Pixmap has x or y greater than zero.");
    }
  }
  #endif

  return True;
}

static void nxagentPixmapMatchID(void *p0, XID x1, void *p2)
{
  PixmapPtr pPixmap = (PixmapPtr)p0;
  struct nxagentPixmapPair *pPair = p2;

  if ((pPair -> pMap == NULL) && (nxagentPixmap(pPixmap) == pPair -> pixmap))
  {
    pPair -> pMap = pPixmap;
  }
}

PixmapPtr nxagentPixmapPtr(Pixmap pixmap)
{
  if (pixmap == None)
  {
    return NULL;
  }

  struct nxagentPixmapPair pair = {
    .pixmap = pixmap,
    .pMap = NULL
  };

  FindClientResourcesByType(clients[serverClient -> index], RT_NX_PIXMAP,
                                nxagentPixmapMatchID, &pair);

  for (int i = 0; (pair.pMap == NULL) && (i < MAXCLIENTS); i++)
  {
    if (clients[i])
    {
      FindClientResourcesByType(clients[i], RT_PIXMAP,
                                    nxagentPixmapMatchID, &pair);
    }
  }

  #ifdef WARNING

  if (pair.pMap == NULL)
  {
    fprintf(stderr, "nxagentPixmapPtr: WARNING! Failed to find "
	    "remote pixmap [%ld].\n", (long int) pair.pixmap);
  }
  else if (nxagentDrawableStatus((DrawablePtr) pair.pMap) == NotSynchronized)
  {
    fprintf(stderr, "WARNING! Rootless icon at [%p] [%d,%d] is not synchronized.\n",
                (void *) pair.pMap, pair.pMap -> drawable.width,
                    pair.pMap -> drawable.height);
  }

  #endif

  return pair.pMap;
}

/*
 * Reconnection stuff.
 */

int nxagentDestroyNewPixmapResourceType(void * p, XID id)
{
  /*
   * Address of the destructor is set in Init.c.
   */

  #ifdef TEST
  fprintf(stderr, "nxagentDestroyNewPixmapResourceType: Destroying mirror id [%ld] for pixmap at [%p].\n",
              nxagentPixmapPriv((PixmapPtr) p) -> mid, (void *) p);
  #endif

  nxagentPixmapPriv((PixmapPtr) p) -> mid = None;

  return True;
}

void nxagentDisconnectPixmap(void *p0, XID x1, void *p2)
{
  PixmapPtr pPixmap = (PixmapPtr) p0;

  #ifdef TEST
  Bool *pBool = (Bool*) p2;

  fprintf(stderr, "nxagentDisconnectPixmap: Called with bool [%d] and pixmap at [%p].\n",
              *pBool, (void *) pPixmap);

  fprintf(stderr, "nxagentDisconnectPixmap: Virtual pixmap is [%ld].\n",
              nxagentPixmap(pPixmap));
  #endif

  nxagentPixmap(pPixmap) = None;

  if (nxagentDrawableStatus((DrawablePtr) pPixmap) == NotSynchronized)
  {
    nxagentDestroyCorruptedResource((DrawablePtr) pPixmap, RT_NX_CORR_BACKGROUND);

    nxagentDestroyCorruptedResource((DrawablePtr) pPixmap, RT_NX_CORR_PIXMAP);
  }
}

void nxagentDisconnectAllPixmaps(void)
{
  int r;

  #ifdef TEST
  fprintf(stderr, "nxagentDisconnectAllPixmaps: Going to iterate through pixmap resources.\n");
  #endif

  /*
   * The RT_NX_PIXMAP resource type is allocated only on the server
   * client, so we don't need to find it through the other clients
   * too.
   */

  r = 1;
  FindClientResourcesByType(clients[serverClient -> index], RT_NX_PIXMAP, nxagentDisconnectPixmap, &r);

  #ifdef WARNING

  /* Note: nxagentDisconnectPixmap() does not modify r - so this check can never succeed */
  if (r == 0)
  {
    fprintf(stderr, "nxagentDisconnectAllPixmaps: WARNING! Failed to disconnect "
                "pixmap for client [%d].\n", serverClient -> index);
  }

  #endif

  for (int i = 0; i < MAXCLIENTS; i++)
  {
    if (clients[i])
    {
      #ifdef TEST
      fprintf(stderr, "nxagentDisconnectAllPixmaps: Going to disconnect pixmaps of client [%d].\n", i);
      #endif

      r = 1;
      FindClientResourcesByType(clients[i], RT_PIXMAP, nxagentDisconnectPixmap, &r);

      #ifdef WARNING

      /* Note: nxagentDisconnectPixmap() does not modify r - so this check can never succeed */
      if (r == 0)
      {
        fprintf(stderr, "nxagentDisconnectAllPixmaps: WARNING! Failed to disconnect "
                    "pixmap for client [%d].\n", i);
      }

      #endif
    }
  }

  #ifdef TEST
  fprintf(stderr, "nxagentDisconnectAllPixmaps: Pixmaps disconnection completed.\n");
  #endif

  return;
}

void nxagentReconnectPixmap(void *p0, XID x1, void *p2)
{
  PixmapPtr pPixmap = (PixmapPtr) p0;
  Bool *pBool = (Bool*) p2;

  if (!*pBool || pPixmap == NULL ||
          NXDisplayError(nxagentDisplay) == 1)
  {
    *pBool = False;

    #ifdef TEST
    fprintf(stderr, "nxagentReconnectPixmap: Ignoring pixmap at [%p] while "
                "recovering from the error.\n", (void *) pPixmap);
    #endif

    return;
  }
  else if (pPixmap == nxagentDefaultScreen -> pScratchPixmap)
  {
    /*
     * Every time the scratch pixmap is used its data is changed, so
     * we don't need to reconnect it.
     */

    #ifdef TEST
    fprintf(stderr, "nxagentReconnectPixmap: Ignoring scratch pixmap at [%p].\n",
                (void *) pPixmap);
    #endif

    return;
  }

  #ifdef TEST
  fprintf(stderr, "nxagentReconnectPixmap: Called with result [%d] and pixmap at [%p].\n",
              *pBool, (void *) pPixmap);

  fprintf(stderr, "nxagentReconnectPixmap: Virtual pixmap is at [%p] picture is at [%p].\n",
              (void *) nxagentPixmapPriv(pPixmap) -> pVirtualPixmap,
                  (void *) nxagentPixmapPriv(pPixmap) -> pPicture);
  #endif

  nxagentPrivPixmapPtr pPixmapPriv = nxagentPixmapPriv(pPixmap);

  if (pPixmap -> drawable.width && pPixmap -> drawable.height)
  {
    pPixmapPriv -> id = XCreatePixmap(nxagentDisplay,
                                      nxagentDefaultWindows[pPixmap -> drawable.pScreen -> myNum],
                                      pPixmap -> drawable.width,
                                      pPixmap -> drawable.height,
                                      pPixmap -> drawable.depth);

    nxagentPixmap(pPixmapPriv -> pVirtualPixmap) = pPixmapPriv -> id;

    #ifdef TEST
    fprintf(stderr, "nxagentReconnectPixmap: Created virtual pixmap with id [%ld] for pixmap at [%p].\n",
                nxagentPixmap(pPixmap), (void *) pPixmap);
    #endif

    if (pPixmap == (PixmapPtr) nxagentDefaultScreen -> devPrivate)
    {
      #ifdef WARNING
      fprintf(stderr, "nxagentReconnectPixmap: WARNING! Pixmap is root screen. Returning.\n");
      #endif

      return;
    }

    nxagentSplitTrap = 1;

    *pBool = nxagentSynchronizeDrawableData((DrawablePtr) pPixmap, NEVER_BREAK, NULL);

    nxagentSplitTrap = 0;

    if (!*pBool)
    {
      #ifdef PANIC
      fprintf(stderr, "nxagentReconnectPixmap: PANIC! Failed to synchronize the pixmap.\n");
      #endif
    }

     
    if (nxagentDrawableStatus((DrawablePtr) pPixmap) == NotSynchronized)
    {
      if (nxagentIsCorruptedBackground(pPixmap) == 1)
      {
        nxagentAllocateCorruptedResource((DrawablePtr) pPixmap, RT_NX_CORR_BACKGROUND);

        nxagentFillRemoteRegion((DrawablePtr) pPixmap,
                                    nxagentCorruptedRegion((DrawablePtr) pPixmap));
      }
      else
      {
        nxagentAllocateCorruptedResource((DrawablePtr) pPixmap, RT_NX_CORR_PIXMAP);
      }
    }
  }
}

Bool nxagentReconnectAllPixmaps(void *p0)
{
  Bool result = 1;

  #ifdef TEST
  fprintf(stderr, "nxagentReconnectAllPixmaps: Going to recreate all pixmaps.\n");
  #endif

  /*
   * Reset the geometry and alpha information
   * used by proxy to unpack the packed images.
   */

  nxagentResetVisualCache();

  nxagentResetAlphaCache();

  /*
   * The RT_NX_PIXMAP resource type is allocated only on the server
   * client, so we don't need to find it through the other clients
   * too.
   */

  FindClientResourcesByType(clients[serverClient -> index], RT_NX_PIXMAP, nxagentReconnectPixmap, &result);

  #ifdef WARNING

  if (result == 0)
  {
    fprintf(stderr, "nxagentReconnectAllPixmaps: WARNING! Failed to reconnect "
                "pixmap for client [%d].\n", serverClient -> index);
  }

  #endif

  /*
   * FIXME: This is a bit cumbersome: at the end of each iteration
   * result will be reset to 1. Therefore at loop exit result will
   * always be 1 meaning the whole function will always return 1...
   */
  result = 1;
  for (int i = 0; i < MAXCLIENTS; result = 1, i++)
  {
    if (clients[i] != NULL)
    {
      #ifdef TEST
      fprintf(stderr, "nxagentReconnectAllPixmaps: Going to reconnect pixmaps of client [%d].\n", i);
      #endif

      /*
       * Let the pixmap be reconnected as it was an image request
       * issued by the client owning the resource. The client index is
       * used as a subscript by the image routines to cache the data
       * per-client.
       */

      FindClientResourcesByType(clients[i], RT_PIXMAP, nxagentReconnectPixmap, &result);

      #ifdef WARNING

      if (result == 0)
      {
        fprintf(stderr, "nxagentReconnectAllPixmaps: WARNING! Failed to reconnect "
                    "pixmap for client [%d].\n", serverClient -> index);
      }

      #endif
    }
  }

  #ifdef TEST
  fprintf(stderr, "nxagentReconnectAllPixmaps: Pixmaps reconnection completed.\n");
  #endif

  return result;
}

#ifdef TEST

static void nxagentCheckOnePixmapIntegrity(void *p0, XID x1, void *p2)
{
  PixmapPtr pPixmap = (PixmapPtr) p0;
  Bool      *pBool = (Bool*) p2;

  if (!*pBool)
  {
    return;
  }

  if (pPixmap == nxagentDefaultScreen -> devPrivate)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentCheckOnePixmapIntegrity: pixmap %p is screen.\n",
                (void *) pPixmap);
    #endif

    return;
  }

  if (pPixmap == nxagentDefaultScreen -> PixmapPerDepth[0])
  {
    #ifdef TEST
    fprintf(stderr, "nxagentCheckOnePixmapIntegrity: pixmap %p is default stipple of screen.\n",
                (void *) pPixmap);
    #endif

    return;
  }

  *pBool = nxagentCheckPixmapIntegrity(pPixmap);
}

Bool nxagentCheckPixmapIntegrity(PixmapPtr pPixmap)
{
  Bool integrity = True;
  unsigned long plane_mask = AllPlanes;
  PixmapPtr pVirtual = nxagentVirtualPixmap(pPixmap);

  unsigned int width = pPixmap -> drawable.width;
  unsigned int height = pPixmap -> drawable.height;
  unsigned int depth = pPixmap -> drawable.depth;
  int format = (depth == 1) ? XYPixmap : ZPixmap;

  if (width && height)
  {
    unsigned int length = nxagentImageLength(width, height, format, 0, depth);

    char *data = malloc(length);

    if (data == NULL)
    {
      FatalError("nxagentCheckPixmapIntegrity: Failed to allocate a buffer of size %d.\n", length);
    }

    XImage *image = XGetImage(nxagentDisplay, nxagentPixmap(pPixmap), 0, 0,
                          width, height, plane_mask, format);
    if (image == NULL)
    {
      FatalError("XGetImage: Failed.\n");

      SAFE_free(data);

      return False;
    }

    #ifdef WARNING
    fprintf(stderr, "nxagentCheckPixmapIntegrity: Image from X has length [%d] and checksum [0x%s].\n",
                length, nxagentChecksum(image->data, length));
    #endif

    NXCleanImage(image);

    #ifdef WARNING
    fprintf(stderr, "nxagentCheckPixmapIntegrity: Image after clean has checksum [0x%s].\n",
                nxagentChecksum(image->data, length));
    #endif

    fbGetImage((DrawablePtr) pVirtual, 0, 0, width, height, format, plane_mask, data);

    #ifdef WARNING
    fprintf(stderr, "nxagentCheckPixmapIntegrity: Image from FB has length [%d] and checksum [0x%s].\n",
                length, nxagentChecksum(data, length));
    #endif

    if (image != NULL && memcmp(image -> data, data, length) != 0)
    {
      integrity = False;
    }
    else
    {
      integrity = True;

      #ifdef TEST
      fprintf(stderr, "nxagentCheckPixmapIntegrity: Pixmap at [%p] has been realized. "
                  "Now remote and framebuffer data are synchronized.\n", (void *) pPixmap);
      #endif
    }

    #ifdef WARNING

    if (!integrity)
    {
      char *p = image -> data;
      char *q = data;

      for (int i = 0; i < length; i++)
      {
        if (p[i] != q[i])
        {
          fprintf(stderr, "nxagentCheckPixmapIntegrity: Byte [%d] image -> data [%d] data [%d]. "
                      "Buffers differ!\n", i, p[i], q[i]);
        }
        else
        {
          fprintf(stderr, "nxagentCheckPixmapIntegrity: Byte [%d] image -> data [%d] data [%d].\n",
                      i, p[i], q[i]);
        }
      }

      fprintf(stderr, "nxagentCheckPixmapIntegrity: Pixmap at [%p] width [%d], height [%d], has been realized "
                  "but the data buffer still differs.\n", (void *) pPixmap, width, height);

      fprintf(stderr, "nxagentCheckPixmapIntegrity: bytes_per_line [%d] byte pad [%d] format [%d].\n",
                  image -> bytes_per_line, nxagentImagePad(width, height, 0, depth), image -> format);

      FatalError("nxagentCheckPixmapIntegrity: Image is corrupted!!\n");

    }

    #endif

    if (image != NULL)
    {
      XDestroyImage(image);
    }

    SAFE_free(data);
  }
  else
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentCheckPixmapIntegrity: Ignored pixmap at [%p] with geometry [%d] [%d].\n",
                (void *) pPixmap, width, height);
    #endif
  }

  return integrity;
}

Bool nxagentCheckAllPixmapIntegrity(void)
{
  Bool imageIsGood = True;

  #ifdef TEST
  fprintf(stderr, "nxagentCheckAllPixmapIntegrity\n");
  #endif

  FindClientResourcesByType(clients[serverClient -> index], RT_NX_PIXMAP,
                                nxagentCheckOnePixmapIntegrity, &imageIsGood);

  for (int i = 0; (i < MAXCLIENTS) && (imageIsGood); i++)
  {
    if (clients[i])
    {
      FindClientResourcesByType(clients[i], RT_PIXMAP,
                                    nxagentCheckOnePixmapIntegrity, &imageIsGood);

    }
  }

  #ifdef TEST
  fprintf(stderr, "nxagentCheckAllPixmapIntegrity: pixmaps integrity = %d.\n", imageIsGood);
  #endif

  return imageIsGood;
}

#endif

void nxagentSynchronizeShmPixmap(DrawablePtr pDrawable, int xPict, int yPict,
                                     int wPict, int hPict)
{
  if (pDrawable -> type == DRAWABLE_PIXMAP &&
         nxagentIsShmPixmap((PixmapPtr) pDrawable) == 1)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentSynchronizeShmPixmap: WARNING! Synchronizing shared pixmap at [%p].\n",
                (void *) pDrawable);
    #endif

    GCPtr pGC = nxagentGetScratchGC(pDrawable -> depth, pDrawable -> pScreen);

    CARD32 attributes[3];

    attributes[0] = 0x228b22;
    attributes[1] = 0xffffff;
    attributes[2] = FillSolid;

    ChangeGC(pGC, GCForeground | GCBackground | GCFillStyle, attributes);

    ValidateGC(pDrawable, pGC);

    int width  = (wPict != 0 && wPict <= pDrawable -> width) ? wPict : pDrawable -> width;
    int height = (hPict != 0 && hPict <= pDrawable -> height) ? hPict : pDrawable -> height;

    int depth  = pDrawable -> depth;

    int format = (depth == 1) ? XYPixmap : ZPixmap;

    int length = nxagentImageLength(width, height, format, 0, depth);

    int saveTrap = nxagentGCTrap;

    nxagentGCTrap = 0;

    nxagentSplitTrap = 1;

    nxagentFBTrap = 1;

    char *data = malloc(length);

    if (data)
    {
      fbGetImage(nxagentVirtualDrawable(pDrawable), xPict, yPict,
                     width, height, format, 0xffffffff, data);

      nxagentPutImage(pDrawable, pGC, depth, xPict, yPict,
                          width, height, 0, format, data);

      SAFE_free(data);
    }
    #ifdef WARNING
    else
    {
      fprintf(stderr, "nxagentSynchronizeShmPixmap: WARNING! Failed to allocate memory for the operation.\n");
    }
    #endif

    nxagentGCTrap = saveTrap;

    nxagentSplitTrap = 0;

    nxagentFBTrap = 0;

    nxagentFreeScratchGC(pGC);
  }
}

#ifdef DUMP

/*
 * This function is useful to visualize a pixmap and check
 * its data consistency. To avoid the creation of many
 * windows, one pixmap only can be monitored at a time.
 */

Bool nxagentPixmapOnShadowDisplay(PixmapPtr pMap)
{
  static Display *shadow;
  static Window win;
  static int init = True;
  static int showTime;
  static PixmapPtr pPixmap;
  static int depth;
  static int width;
  static int height;
  static int length;
  static unsigned int format;

  if (init)
  {
    if (pMap == NULL)
    {
      return False;
    }
    else
    {
      pPixmap = pMap;
    }

    depth = pPixmap -> drawable.depth;
    width = pPixmap -> drawable.width;
    height = pPixmap -> drawable.height;
    format = (depth == 1) ? XYPixmap : ZPixmap;

    shadow = XOpenDisplay("localhost:0");

    if (shadow == NULL)
    {
      #ifdef WARNING
      fprintf(stderr, "nxagentPixmapOnShadowDisplay: WARNING! Shadow display not opened.\n");
      #endif

      return False;
    }

    init = False;

    win = XCreateSimpleWindow(shadow, DefaultRootWindow(shadow), 0, 0,
                                  width, height, 0, 0xFFCC33, 0xFF);

    XMapWindow(shadow, win);
    XClearWindow(shadow, win);
  }

/*
FIXME: If the pixmap has a different depth from the window, the
       XPutImage returns a BadMatch. For example this may happen if
       the Render extension is enabled.
       Can we fix this creating a new pixmap?
*/

  if (DisplayPlanes(shadow, DefaultScreen(shadow)) != depth)
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentPixmapOnShadowDisplay: Pixmap and Window depths [%d - %d] are not equals!\n",
                depth, DisplayPlanes(shadow, DefaultScreen(shadow)));
    #endif

    return False;
  }

  /*
   * If the framebuffer is updated continuously, the nxagent
   * visualization becomes much too slow.
   */

  if ((GetTimeInMillis() - showTime) < 500)
  {
    return False;
  }

  showTime = GetTimeInMillis();

  length = nxagentImageLength(width, height, format, 0, depth);

  char * data = malloc(length);

  if (data == NULL)
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentPixmapOnShadowDisplay: WARNING! Failed to allocate memory for the operation.\n");
    #endif

    return False;
  }

  fbGetImage((DrawablePtr) nxagentVirtualPixmap(pPixmap), 0, 0,
                 width, height, format, AllPlanes, data);

  Visual *pVisual = nxagentImageVisual((DrawablePtr) pPixmap, depth);

  if (pVisual == NULL)
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentPixmapOnShadowDisplay: WARNING! Visual not found. Using default visual.\n");
    #endif
    
    pVisual = nxagentVisuals[nxagentDefaultVisualIndex].visual;
  } 

  XImage *image = XCreateImage(nxagentDisplay, pVisual,
                                  depth, format, 0, (char *) data,
                                      width, height, BitmapPad(nxagentDisplay),
                                          nxagentImagePad(width, format, 0, depth));

  if (image == NULL)
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentPixmapOnShadowDisplay: XCreateImage failed.\n");
    #endif

    SAFE_free(data);

    return False;
  }

  XGCValues value = {
    .foreground = 0xff0000,
    .background = 0x000000,
    .plane_mask = 0xffffff,
    .fill_style = FillSolid
  };

  XlibGC gc = XCreateGC(shadow, win, GCBackground |
                            GCForeground | GCFillStyle | GCPlaneMask, &value);

  NXCleanImage(image);

  XPutImage(shadow, win, gc, image, 0, 0, 0, 0, width, height);

  XFreeGC(shadow, gc);

  if (image != NULL)
  {
    XDestroyImage(image);
  }

  return True;
}

Bool nxagentFbOnShadowDisplay(void)
{
  static Display *shadow;
  static Window win;
  static int init = True;
  static int showTime;
  static int prevWidth, prevHeight;

  WindowPtr pWin = screenInfo.screens[0]->root;

  if (pWin == NULL)
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentFbOnShadowDisplay: The parent window is NULL.\n");
    #endif

    return False;
  }

  int depth = pWin -> drawable.depth;
  int width = pWin -> drawable.width;
  int height = pWin -> drawable.height;
  unsigned int format = (depth == 1) ? XYPixmap : ZPixmap;

  if (init)
  {
    shadow = XOpenDisplay("localhost:0");

    if (shadow == NULL)
    {
      #ifdef WARNING
      fprintf(stderr, "nxagentFbOnShadowDisplay: WARNING! Shadow display not opened.\n");
      #endif

      return False;
    }

    init = False;

    prevWidth = width;
    prevHeight = height;

    win = XCreateSimpleWindow(shadow, DefaultRootWindow(shadow), 0, 0,
                                  width, height, 0, 0xFFCC33, 0xFF);

    XMapWindow(shadow, win);
    XClearWindow(shadow, win);
  }

  if (DisplayPlanes(shadow, DefaultScreen(shadow)) != depth)
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentFbOnShadowDisplay: Depths [%d - %d] are not equals!\n",
                depth, DisplayPlanes(shadow, DefaultScreen(shadow)));
    #endif

    return False;
  }

  /*
   * If the framebuffer is updated continuously, the nxagent
   * visualization becomes too much slow.
   */

  if ((GetTimeInMillis() - showTime) < 500)
  {
    return False;
  }

  showTime = GetTimeInMillis();

  /*
   * If the root window is resized, also the window on shadow
   * display must be resized.
   */

  if (prevWidth != width || prevHeight != height)
  {
    prevWidth = width;
    prevHeight = height;

    XWindowChanges values = {
      .width = width,
      .height = height
    };

    XConfigureWindow(shadow, win, CWWidth | CWHeight, &values);
  }

  int length = nxagentImageLength(width, height, format, 0, depth);

  char *data = malloc(length);

  if (data == NULL)
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentFbOnShadowDisplay: WARNING! Failed to allocate memory for the operation.\n");
    #endif

    return False;
  }

  fbGetImage((DrawablePtr)pWin, 0, 0,
                 width, height, format, AllPlanes, data);

  Visual *pVisual = nxagentImageVisual((DrawablePtr) pWin, depth);

  if (pVisual == NULL)
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentFbOnShadowDisplay: WARNING! Visual not found. Using default visual.\n");
    #endif
    
    pVisual = nxagentVisuals[nxagentDefaultVisualIndex].visual;
  } 

  XImage *image = XCreateImage(nxagentDisplay, pVisual,
                                  depth, format, 0, (char *) data,
                                      width, height, BitmapPad(nxagentDisplay),
                                          nxagentImagePad(width, format, 0, depth));

  if (image == NULL)
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentFbOnShadowDisplay: XCreateImage failed.\n");
    #endif

    SAFE_free(data);

    return False;
  }

  XGCValues value = {
    .foreground = 0xff0000,
    .background = 0x000000,
    .plane_mask = 0xffffff,
    .fill_style = FillSolid
  };

  XlibGC gc = XCreateGC(shadow, win, GCBackground |
                            GCForeground | GCFillStyle | GCPlaneMask, &value);

  NXCleanImage(image);

  XPutImage(shadow, win, gc, image, 0, 0, 0, 0, width, height);

  XFreeGC(shadow, gc);

  if (image != NULL)
  {
    XDestroyImage(image);
  }

  return True;
}

#endif

#ifdef DEBUG

void nxagentPrintResourceTypes(void)
{
  fprintf(stderr, "nxagentPrintResourceTypes: RT_PIXMAP [%lu].\n", (unsigned long) RT_PIXMAP);
  fprintf(stderr, "nxagentPrintResourceTypes: RT_NX_PIXMAP [%lu].\n", (unsigned long) RT_NX_PIXMAP);
  fprintf(stderr, "nxagentPrintResourceTypes: RT_GC [%lu].\n", (unsigned long) RT_GC);
  fprintf(stderr, "nxagentPrintResourceTypes: RT_NX_GC [%lu].\n", (unsigned long) RT_NX_GC);
  fprintf(stderr, "nxagentPrintResourceTypes: RT_FONT [%lu].\n", (unsigned long) RT_FONT);
  fprintf(stderr, "nxagentPrintResourceTypes: RT_NX_FONT [%lu].\n", (unsigned long) RT_NX_FONT);
  fprintf(stderr, "nxagentPrintResourceTypes: RT_CURSOR [%lu].\n", (unsigned long) RT_CURSOR);
  fprintf(stderr, "nxagentPrintResourceTypes: RT_WINDOW [%lu].\n", (unsigned long) RT_WINDOW);
  fprintf(stderr, "nxagentPrintResourceTypes: RT_COLORMAP [%lu].\n", (unsigned long) RT_COLORMAP);
}

void nxagentPrintResourcePredicate(void *value, XID id, XID type, void *cdata)
{
  fprintf(stderr, "nxagentPrintResourcePredicate: Resource [%p] id [%lu] type [%lu].\n",
              (void *) value, (unsigned long) id, (unsigned long) type);
}

void nxagentPrintResources(void)
{
  nxagentPrintResourceTypes();

  for (int i = 0; i < MAXCLIENTS; i++)
  {
    if (clients[i])
    {
      Bool result;

      fprintf(stderr, "nxagentPrintResources: Printing resources for client [%d]:\n",
                  i);

      FindAllClientResources(clients[i], nxagentPrintResourcePredicate, &result);
    }
  }
}

#endif


