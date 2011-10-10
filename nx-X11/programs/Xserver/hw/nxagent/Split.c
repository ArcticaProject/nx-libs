/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2011 NoMachine, http://www.nomachine.com/.         */
/*                                                                        */
/* NXAGENT, NX protocol compression and NX extensions to this software    */
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

#include "scrnintstr.h"
#include "pixmapstr.h"
#include "windowstr.h"
#include "gcstruct.h"

#include "Agent.h"
#include "Display.h"
#include "Drawable.h"
#include "Events.h"
#include "GCs.h"

#include "NXlib.h"


/*
 * Set here the required log level.
 */

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

/*
 * This should be a macro but for now
 * we make it a real function to log
 * a warning in the logs.
 */

DrawablePtr nxagentSplitDrawable(DrawablePtr pDrawable)
{
  if (pDrawable -> type == DRAWABLE_PIXMAP &&
          nxagentPixmapIsVirtual((PixmapPtr) pDrawable))
  {
    #ifdef TEST
    fprintf(stderr, "nxagentSplitDrawable: WARNING! The drawable at [%p] is "
                "virtual. Assuming real at [%p].\n", (void *) pDrawable,
                    (void *) nxagentRealPixmap((PixmapPtr) pDrawable));
    #endif

    return (DrawablePtr) nxagentRealPixmap((PixmapPtr) pDrawable);
  }
  else
  {
    return pDrawable;
  }
}

void nxagentInitSplitResources()
{
  int resource;

  #ifdef TEST
  fprintf(stderr, "nxagentInitSplitResources: Initializing the split resources.\n");
  #endif

  for (resource = 0; resource < NXNumberOfResources; resource++)
  {
    SplitResourcePtr pResource = &nxagentSplitResources[resource];

    pResource -> pending  = 0;
    pResource -> commit   = 0;
    pResource -> split    = NXNoResource;
    pResource -> unpack   = NXNoResource;
    pResource -> drawable = NULL;
    pResource -> region   = NullRegion;
    pResource -> gc       = NULL;
  }
}

SplitResourcePtr nxagentAllocSplitResource()
{
  int resource;

  SplitResourcePtr pResource;

  for (;;)
  {
    resource = NXAllocSplit(nxagentDisplay, NXAnyResource);

    if (resource != NXNoResource)
    {
      #ifdef TEST
      fprintf(stderr, "nxagentAllocSplitResource: Reserving resource [%d] for the next split.\n",
                  resource);
      #endif

      break;
    }
    else
    {
      #ifdef PANIC
      fprintf(stderr, "nxagentAllocSplitResource: PANIC! No more resources for the next split.\n");
      #endif
/*
FIXME: Must deal with the case all resources are exausted.
*/
      FatalError("nxagentAllocSplitResource: PANIC! No more resources for the next split.\n");
    }
  }

  pResource = &nxagentSplitResources[resource];

  if (pResource -> pending != 0 || pResource -> split != NXNoResource ||
          pResource -> unpack != NXNoResource || pResource -> drawable != NULL ||
              pResource -> region != NullRegion || pResource -> commit != 0 ||
                  pResource -> gc != NULL)
  {
    /*
     * This is really an unrecoverable error.
     */

    #ifdef PANIC
    fprintf(stderr, "nxagentAllocSplitResource: PANIC! Invalid record for resource [%d] with "
                "pending [%d] split [%d] unpack [%d] drawable [%p] region [%p] commit [%d] gc [%p].\n",
                    resource, pResource -> pending, pResource -> split, pResource -> unpack,
                        (void *) pResource -> drawable, (void *) pResource -> region,
                            pResource -> commit, (void *) pResource -> gc);
    #endif

    FatalError("nxagentAllocSplitResource: PANIC! Invalid record for resource [%d] with "
                   "pending [%d] split [%d] unpack [%d] drawable [%p] region [%p] commit [%d] gc [%p].\n",
                       resource, pResource -> pending, pResource -> split, pResource -> unpack,
                           (void *) pResource -> drawable, (void *) pResource -> region,
                               pResource -> commit, (void *) pResource -> gc);
  }

  pResource -> pending = 1;
  pResource -> split   = resource;

  return pResource;
}

void nxagentFreeSplitResource(SplitResourcePtr pResource)
{
  if (pResource -> pending == 0 || pResource -> split == NXNoResource ||
          pResource -> drawable != NULL || pResource -> region != NullRegion ||
              pResource -> gc != NULL)
  {
    #ifdef PANIC
    fprintf(stderr, "nxagentFreeSplitResource: PANIC! Invalid record provided with values "
                "pending [%d] split [%d] unpack [%d] drawable [%p] region [%p] commit [%d] gc [%p].\n",
                    pResource -> pending, pResource -> split, pResource -> unpack,
                        (void *) pResource -> drawable, (void *) pResource -> region,
                            pResource -> commit, (void *) pResource -> gc);
    #endif

    FatalError("nxagentFreeSplitResource: PANIC! Invalid record provided with values "
                   "pending [%d] split [%d] unpack [%d] drawable [%p] region [%p] commit [%d] gc [%p].\n",
                       pResource -> pending, pResource -> split, pResource -> unpack,
                           (void *) pResource -> drawable, (void *) pResource -> region,
                               pResource -> commit, (void *) pResource -> gc);
  }

  #ifdef TEST
  fprintf(stderr, "nxagentFreeSplitResource: Clearing the record for resource [%d].\n",
              pResource -> split);
  #endif

  NXFreeSplit(nxagentDisplay, pResource -> split);

  pResource -> pending  = 0;
  pResource -> commit   = 0;
  pResource -> split    = NXNoResource;
  pResource -> unpack   = NXNoResource;
  pResource -> drawable = NULL;
  pResource -> region   = NullRegion;
  pResource -> gc       = NULL;
}

void nxagentInitUnpackResources()
{
/*
FIXME: This must be implemented.
*/
}

UnpackResourcePtr nxagentAllocUnpackResource()
{
/*
FIXME: This must be implemented.
*/
  return NULL;
}

void nxagentFreeUnpackResource(UnpackResourcePtr pResource)
{
/*
FIXME: This must be implemented.
*/
}

void nxagentReleaseAllSplits(void)
{
  int resource;

  #ifdef TEST
  fprintf(stderr, "nxagentReleaseAllSplits: Going to release all the split resources.\n");
  #endif

  for (resource = 0; resource < NXNumberOfResources; resource++)
  {
    SplitResourcePtr pResource = &nxagentSplitResources[resource];

    if (pResource != NULL && pResource -> pending == 1)
    {
      DrawablePtr pDrawable = pResource -> drawable;

      if (pDrawable != NULL)
      {
        #ifdef TEST
        fprintf(stderr, "nxagentReleaseAllSplits: Releasing the drawable at [%p] for "
                    "resource [%d].\n", (void *) pDrawable, pResource -> split);
        #endif

        nxagentReleaseSplit(pDrawable);
      }

      #ifdef TEST
      fprintf(stderr, "nxagentReleaseAllSplits: Freeing the resource [%d].\n",
                  resource);
      #endif

      nxagentFreeSplitResource(pResource);
    }
  }
}

/*
 * Check the coherency of the split record.
 */

#ifdef TEST

static void nxagentCheckSplit(DrawablePtr pDrawable, SplitResourcePtr pResource)
{
  if (pResource == NULL)
  {
    #ifdef PANIC
    fprintf(stderr, "nxagentCheckSplit: PANIC! No record associated to drawable at [%p].\n",
              (void *) pDrawable);
    #endif

    FatalError("nxagentCheckSplit: PANIC! No record associated to drawable at [%p].\n",
                   (void *) pDrawable);
  }
  else if (pResource -> drawable != pDrawable ||
               pResource -> split == NXNoResource)
  {
    #ifdef PANIC
    fprintf(stderr, "nxagentCheckSplit: PANIC! The record [%d] doesn't match the drawable at [%p].\n",
                pResource -> split, (void *) pDrawable);
    #endif

    FatalError("nxagentCheckSplit: PANIC! The record [%d] doesn't match the drawable at [%p].\n",
                pResource -> split, (void *) pDrawable);
  }
  else if (pResource -> commit == 1 &&
               pResource -> gc == NULL)
  {
    #ifdef PANIC
    fprintf(stderr, "nxagentCheckSplit: PANIC! The record [%d] doesn't have a valid GC.\n",
                pResource -> split);
    #endif

    FatalError("nxagentCheckSplit: PANIC! The record [%d] doesn't have a valid GC.\n",
                   pResource -> split);
  }
}

static void nxagentCheckResource(SplitResourcePtr pResource, int resource)
{
  if (pResource == NULL)
  {
    #ifdef PANIC
    fprintf(stderr, "nxagentCheckResource: PANIC! No record associated to resource [%d].\n",
                resource);
    #endif

    FatalError("nxagentCheckResource: PANIC! No record associated to resource [%d].\n",
                   resource);
  }
  else if ((pResource -> split != resource || pResource -> pending != 1) ||
               (pResource -> commit == 1 && (pResource -> drawable == NULL ||
                   pResource -> gc == NULL)))
  {
    #ifdef PANIC
    fprintf(stderr, "nxagentCheckResource: PANIC! Invalid record for resource [%d] with "
                "pending [%d] split [%d] unpack [%d] drawable [%p] region [%p] commit [%d] gc [%p].\n",
                    resource, pResource -> pending, pResource -> split, pResource -> unpack,
                        (void *) pResource -> drawable, (void *) pResource -> region,
                            pResource -> commit, (void *) pResource -> gc);
    #endif

    FatalError("nxagentCheckResource: PANIC! Invalid record for resource [%d] with "
                   "pending [%d] split [%d] unpack [%d] drawable [%p] region [%p] commit [%d] gc [%p].\n",
                       resource, pResource -> pending, pResource -> split, pResource -> unpack,
                           (void *) pResource -> drawable, (void *) pResource -> region,
                                pResource -> commit, (void *) pResource -> gc);
  }
}

#endif

int nxagentCreateSplit(DrawablePtr pDrawable, GCPtr *pGC)
{
  SplitResourcePtr pResource;

  pDrawable = nxagentSplitDrawable(pDrawable);

  pResource = nxagentAllocSplitResource();

  if (pDrawable -> type == DRAWABLE_PIXMAP)
  {
    nxagentPixmapPriv((PixmapPtr) pDrawable) -> splitResource = pResource;
  }
  else
  {
    nxagentWindowPriv((WindowPtr) pDrawable) -> splitResource = pResource;
  }

  pResource -> drawable = pDrawable;
  pResource -> commit   = 1;

  /*
   * Make a copy of the GC so the client
   * can safely remove it.
   */

  pResource -> gc = CreateScratchGC(pDrawable -> pScreen, pDrawable -> depth);

/*
FIXME: What do we do here?
*/
  if (pResource -> gc == NULL)
  {
    #ifdef PANIC
    fprintf(stderr, "nxagentCreateSplit: PANIC! Failed to create split GC for resource [%d].\n",
                pResource -> split);
    #endif

    FatalError("nxagentCreateSplit: PANIC! Failed to create split GC for resource [%d].\n",
                   pResource -> split);
  }
  else if (CopyGC(*pGC, pResource -> gc, GCFunction | GCPlaneMask |
                      GCSubwindowMode | GCClipXOrigin | GCClipYOrigin |
                           GCClipMask | GCForeground | GCBackground) != Success)
  {
/*
FIXME: What do we do here?
*/
    #ifdef PANIC
    fprintf(stderr, "nxagentCreateSplit: PANIC! Failed to copy split GC for resource [%d].\n",
                pResource -> split);
    #endif

    FatalError("nxagentCreateSplit: PANIC! Failed to copy split GC for resource [%d].\n",
                   pResource -> split);
  }

  #ifdef TEST
  fprintf(stderr, "nxagentCreateSplit: Associated GC at [%p] to resource [%d] "
              "with id [%lu].\n", (void *) pResource -> gc, pResource -> split,
                  (unsigned long) nxagentGC(pResource -> gc));
  #endif

  *pGC = pResource -> gc;

  #ifdef TEST
  fprintf(stderr, "nxagentCreateSplit: Associated resource [%d] to drawable at [%p].\n",
              pResource -> split, (void *) pResource -> drawable);
  #endif

  return pResource -> split;
}

/*
 * Set the region to be the current
 * streaming region.
 */

void nxagentRegionSplit(DrawablePtr pDrawable, RegionPtr pRegion)
{
  SplitResourcePtr pResource;

  pDrawable = nxagentSplitDrawable(pDrawable);

  pResource = nxagentSplitResource(pDrawable);

  #ifdef TEST

  fprintf(stderr, "nxagentRegionSplit: Associating region to resource [%d] drawable at [%p].\n",
              pResource -> split, (void *) pDrawable);

  nxagentCheckSplit(pDrawable, pResource);

  #endif
  
  if (pResource == NULL)
  {
    #ifdef PANIC
    fprintf(stderr, "nxagentRegionSplit: PANIC! No valid split record for drawable at [%p].\n",
                (void *) pDrawable);
    #endif

    return;
  }

  #ifdef TEST
  fprintf(stderr, "nxagentRegionSplit: Associated region [%d,%d,%d,%d] to drawable at [%p] "
              "with resource [%d].\n", pRegion -> extents.x1, pRegion -> extents.y1,
                  pRegion -> extents.x2, pRegion -> extents.y2, (void *) pDrawable,
                      pResource -> split);
  #endif

  pResource -> region = pRegion;
}

/*
 * Remove the association between the drawable
 * and the split resource. The resource is not
 * deallocated until the end of the split.
 */

void nxagentReleaseSplit(DrawablePtr pDrawable)
{
  SplitResourcePtr pResource;

  pDrawable = nxagentSplitDrawable(pDrawable);

  pResource = nxagentSplitResource(pDrawable);

  if (pResource == NULL)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentReleaseSplit: No split resources for drawable at [%p].\n",
                (void *) pDrawable);
    #endif

    return;
  }

  #ifdef TEST

  fprintf(stderr, "nxagentReleaseSplit: Going to release resource [%d] for drawable at [%p].\n",
              pResource -> split, (void *) pDrawable);

  nxagentCheckSplit(pDrawable, pResource);

  #endif

  if (pResource -> region != NullRegion)
  {
    /*
     * If we have a region the commits
     * had to be still valid. In this
     * case tell the proxy to abort the
     * data transfer.
     */

    #ifdef TEST

    if (pResource -> commit == 0)
    {
      #ifdef PANIC
      fprintf(stderr, "nxagentReleaseSplit: PANIC! Found a region for resource [%d] but the "
                  "commits are invalid.\n", pResource -> split);
      #endif

      FatalError("nxagentCheckSplit: PANIC! Found a region for resource [%d] but the "
                     "commits are invalid.\n", pResource -> split);
    }

    #endif

    #ifdef TEST
    fprintf(stderr, "nxagentValidateSplit: Aborting the data transfer for resource [%d].\n",
                pResource -> split);
    #endif

    NXAbortSplit(nxagentDisplay, pResource -> split);

    #ifdef TEST
    fprintf(stderr, "nxagentReleaseSplit: Freeing the region for drawable at [%p].\n",
                (void *) pDrawable);
    #endif

    REGION_DESTROY(pDrawable -> pScreen, pResource -> region);

    pResource -> region = NullRegion;
  }

  if (pResource -> gc != NULL)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentReleaseSplit: Freeing the split GC for drawable at [%p].\n",
                (void *) pDrawable);
    #endif

    FreeScratchGC(pResource -> gc);

    pResource -> gc = NULL;
  }

  /*
   * Remove the association between the
   * drawable and the resource record.
   */

  #ifdef TEST
  fprintf(stderr, "nxagentReleaseSplit: Removing association to drawable at [%p].\n",
              (void *) pDrawable);
  #endif

  if (pDrawable -> type == DRAWABLE_PIXMAP)
  {
    nxagentPixmapPriv((PixmapPtr) pDrawable) -> splitResource = NULL;
  }
  else
  {
    nxagentWindowPriv((WindowPtr) pDrawable) -> splitResource = NULL;
  }

  /*
   * Invalidate the commits and remove the
   * association between the resource and
   * the drawable.
   */

  pResource -> drawable = NULL;
  pResource -> commit   = 0;
}

void nxagentValidateSplit(DrawablePtr pDrawable, RegionPtr pRegion)
{
  SplitResourcePtr pResource;

  pDrawable = nxagentSplitDrawable(pDrawable);

  pResource = nxagentSplitResource(pDrawable);

  if (pResource == NULL)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentValidateSplit: No split resource for drawable at [%p].\n",
                (void *) pDrawable);
    #endif

    return;
  }
  else if (pResource -> region == NullRegion)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentValidateSplit: No split region yet for drawable at [%p].\n",
                (void *) pDrawable);
    #endif

    return;
  }
  else if (pResource -> commit == 0)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentValidateSplit: WARNING! Split for drawable at [%p] was "
                "already invalidated.\n", (void *) pDrawable);
    #endif

    return;
  }

  #ifdef TEST

  fprintf(stderr, "nxagentValidateSplit: Going to validate resource [%d] drawable at [%p].\n",
              pResource -> split, (void *) pDrawable);

  nxagentCheckSplit(pDrawable, pResource);

  #endif
  
  #ifdef TEST
  fprintf(stderr, "nxagentValidateSplit: Checking the region for resource [%d] "
              "and drawable at [%p].\n", pResource -> split, (void *) pDrawable);
  #endif

  /*
   * If a null region is passed as parameter,
   * we assume that all the commits have to
   * be discarded.
   */

  if (pRegion == NullRegion)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentValidateSplit: Forcing all commits as invalid for "
                "drawable at [%p].\n", (void *) pDrawable);
    #endif

    nxagentReleaseSplit(pDrawable);
  }
  else
  {
    RegionRec tmpRegion;

    /*
     * Check if the provided region overlaps
     * the area covered by the image being
     * streamed.
     */

    REGION_INIT(pDrawable -> pScreen, &tmpRegion, NullBox, 1);

    REGION_INTERSECT(pDrawable -> pScreen, &tmpRegion, pResource -> region, pRegion);

    if (REGION_NIL(&tmpRegion) == 0)
    {
      #ifdef TEST
      fprintf(stderr, "nxagentValidateSplit: Marking the overlapping commits as invalid "
                  "for drawable at [%p].\n", (void *) pDrawable);
      #endif

      nxagentReleaseSplit(pDrawable);
    }
    #ifdef TEST
    else
    {
      fprintf(stderr, "nxagentValidateSplit: Leaving the commits as valid for "
                  "drawable at [%p].\n", (void *) pDrawable);
    }
    #endif

    REGION_UNINIT(pDrawable -> pScreen, &tmpRegion);
  }
}

void nxagentFreeSplit(int resource)
{
  DrawablePtr pDrawable;

  SplitResourcePtr pResource = &nxagentSplitResources[resource];

  if (pResource == NULL)
  {
    #ifdef PANIC
    fprintf(stderr, "nxagentFreeSplit: PANIC! No valid split record for resource [%d].\n",
                resource);
    #endif

    FatalError("nxagentFreeSplit: PANIC! No valid split record for resource [%d].\n",
                   resource);
  }
  else if (pResource -> split != resource)
  {
    #ifdef PANIC
    fprintf(stderr, "nxagentFreeSplit: PANIC! The record [%d] doesn't match the resource [%d].\n",
                pResource -> split, resource);
    #endif

    FatalError("nxagentFreeSplit: PANIC! The record [%d] doesn't match the resource [%d].\n",
                   pResource -> split, resource);
  }

  pDrawable = pResource -> drawable;

  if (pDrawable != NULL)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentFreeSplit: Removing association to drawable at [%p].\n",
                (void *) pDrawable);
    #endif

    nxagentReleaseSplit(pDrawable);
  }
  #ifdef TEST
  else
  {
    /*
     * The end of the split has come after we have
     * invalidated the operation and removed the
     * association to the drawable. This happens,
     * for example, if the drawable is destroyed.
     */

    fprintf(stderr, "nxagentFreeSplit: WARNING! Releasing invalidated resource [%d].\n",
                pResource -> split);
  }
  #endif

  #ifdef TEST
  fprintf(stderr, "nxagentFreeSplit: Freeing the record for resource [%d].\n",
              pResource -> split);
  #endif

  nxagentFreeSplitResource(pResource);
}

/*
FIXME: This must be enabled when the vanilla
       synchronization procedure is working.
*/
#define USE_FINISH_SPLIT

void nxagentWaitDrawable(DrawablePtr pDrawable)
{
  SplitResourcePtr pResource;

  pDrawable = nxagentSplitDrawable(pDrawable);

  pResource = nxagentSplitResource(pDrawable);

  if (pResource == NULL)
  {
    #ifdef TEST
    fprintf(stderr, "++++++nxagentWaitDrawable: WARNING! The drawable at [%p] is already awake.\n",
	    (void *) pDrawable);
    #endif

    return;
  }

  #ifdef TEST
  fprintf(stderr, "++++++nxagentWaitDrawable: Waiting drawable at [%p] with resource [%d].\n",
              (void *) pDrawable, pResource -> split);
  #endif

  /*
   * Be sure we intercept an I/O error
   * as well as an interrupt.
   */

  #ifdef USE_FINISH_SPLIT

  NXFinishSplit(nxagentDisplay, pResource -> split);

  #endif

  NXFlushDisplay(nxagentDisplay, NXFlushBuffer);

  for (;;)
  {
    /*
     * Handling all the possible events here
     * preempts the queue and makes a better
     * use of the link.
     *
     * We should better use XIfEvent() instead
     * of looping again and again through the
     * event queue.
     */

    nxagentDispatchEvents(NULL);

    /*
     * Wait indefinitely until the resource
     * is released or the display is shut
     * down.
     */

    if (nxagentSplitResource(pDrawable) == NULL ||
            NXDisplayError(nxagentDisplay) == 1)
    {
      #ifdef TEST

      if (NXDisplayError(nxagentDisplay) == 1)
      {
        fprintf(stderr, "++++++nxagentWaitDrawable: WARNING! Display error detected while "
                    "waiting for the drawable.\n");
      }
      else
      {
        fprintf(stderr, "++++++nxagentWaitDrawable: Drawable at [%p] can now be restarted.\n",
		(void *) pDrawable);
      }

      #endif
 
      return;
    }

    #ifdef TEST
    fprintf(stderr, "++++++nxagentWaitDrawable: Yielding control to the NX transport.\n");
    #endif

    nxagentWaitEvents(nxagentDisplay, NULL);
  }
}

static Bool nxagentCommitSplitPredicate(Display *display, XEvent *event, XPointer ptr)
{
  return (event -> type == ClientMessage &&
              event -> xclient.data.l[0] == NXCommitSplitNotify &&
                  event -> xclient.window == 0 && event -> xclient.message_type == 0 &&
                      event -> xclient.format == 32);
}

void nxagentWaitCommitEvent(int resource)
{
  XEvent event;

  /*
   * Check if there is any commit pending. As
   * we are at it, handle any commit, even those
   * commits pertaining to other resources.
   *
   * We can receive some commits even if we'll
   * later receive a no-split event. The proxy,
   * in fact, may have missed to find the image
   * in the memory cache and could have loaded it
   * from disk (so requiring a commit) before we
   * marked the end of the split sequence.
   */

  while (nxagentCheckEvents(nxagentDisplay, &event,
                                nxagentCommitSplitPredicate, NULL) == 1)
  {
    int client   = event.xclient.data.l[1];
    int request  = event.xclient.data.l[2];
    int position = event.xclient.data.l[3];

    #ifdef TEST
    fprintf(stderr, "nxagentWaitCommitEvent: Commit event received with "
                "client [%d] request [%d] and position [%d].\n",
                    client, request, position);
    #endif

    nxagentHandleCommitSplitEvent(client, request, position);
  }
}

static Bool nxagentWaitSplitPredicate(Display *display, XEvent *event, XPointer ptr)
{
  return (event -> type == ClientMessage &&
              (event -> xclient.data.l[0] == NXNoSplitNotify ||
                  event -> xclient.data.l[0] == NXStartSplitNotify) &&
                      event -> xclient.window == 0 && event -> xclient.message_type == 0 &&
                          event -> xclient.format == 32);
}

int nxagentWaitSplitEvent(int resource)
{
  XEvent event;

  int split = 0;

  /*
   * Don't flush the link. We only want to
   * query the NX transport to check whether
   * the operation caused a split.
   */

  NXFlushDisplay(nxagentDisplay, NXFlushBuffer);

  for (;;)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentWaitSplitEvent: Waiting for the split event for resource [%d].\n",
                resource);
    #endif

    XIfEvent(nxagentDisplay, &event, nxagentWaitSplitPredicate, NULL);

    if (NXDisplayError(nxagentDisplay) == 1)
    {
      #ifdef TEST
      fprintf(stderr, "nxagentWaitSplitEvent: WARNING! Error detected reading the event.\n");
      #endif

      nxagentHandleNoSplitEvent(resource);

      break;
    }

    #ifdef TEST
    fprintf(stderr, "nxagentWaitSplitEvent: Going to process the split event.\n");
    #endif

    if (resource != (int) event.xclient.data.l[1])
    {
      #ifdef PANIC
      fprintf(stderr, "nxagentWaitSplitEvent: PANIC! Got event for resource [%d] while "
                  "waiting for resource [%d].\n", (int) event.xclient.data.l[1], resource);

      fprintf(stderr, "nxagentWaitSplitEvent: PANIC! Restarting resource [%d] due to the "
                  "missing split event.\n", resource);
      #endif

      nxagentHandleNoSplitEvent(resource);

      break;
    }
    else if (event.xclient.data.l[0] == NXNoSplitNotify)
    {
      nxagentHandleNoSplitEvent(resource);

      break;
    }
    else
    {
      nxagentHandleStartSplitEvent(resource);

      split = 1;

      break;
    }
  }

  return split;
}

void nxagentHandleNoSplitEvent(int resource)
{
  if (resource >= 0 && resource < NXNumberOfResources)
  {
    #ifdef TEST

    SplitResourcePtr pResource = &nxagentSplitResources[resource];

    fprintf(stderr, "nxagentHandleNoSplitEvent: Received event for resource [%d].\n",
                resource);

    nxagentCheckResource(pResource, resource);

    #endif

    #ifdef TEST
    fprintf(stderr, "nxagentHandleNoSplitEvent: Checking if there is any commit pending.\n");
    #endif

    nxagentWaitCommitEvent(resource);

    #ifdef TEST
    fprintf(stderr, "nxagentHandleNoSplitEvent: No streaming was required with resource [%d] "
                "and drawable at [%p].\n", resource, (void *) pResource -> drawable);
    #endif

    /*
     * Release the resource.
     */

    nxagentFreeSplit(resource);
  }
  #ifdef PANIC
  else
  {
    fprintf(stderr, "nxagentHandleNoSplitEvent: PANIC! Invalid resource identifier [%d] "
                " received in event.\n", resource);
  }
  #endif
}

void nxagentHandleStartSplitEvent(int resource)
{
  if (resource >= 0 && resource < NXNumberOfResources)
  {
    #ifdef TEST

    SplitResourcePtr pResource = &nxagentSplitResources[resource];

    fprintf(stderr, "nxagentHandleStartSplitEvent: Received event for resource [%d].\n",
                resource);

    nxagentCheckResource(pResource, resource);

    #endif

    #ifdef TEST
    fprintf(stderr, "nxagentHandleStartSplitEvent: Checking if there is any commit pending.\n");
    #endif

    nxagentWaitCommitEvent(resource);

    #ifdef TEST
    fprintf(stderr, "nxagentHandleStartSplitEvent: Streaming started with resource [%d] "
                "and drawable at [%p].\n", resource, (void *) pResource -> drawable);
    #endif
  }
  #ifdef PANIC
  else
  {
    fprintf(stderr, "nxagentHandleStartSplitEvent: PANIC! Invalid resource identifier [%d] "
                " received in event.\n", resource);
  }
  #endif
}

void nxagentHandleCommitSplitEvent(int resource, int request, int position)
{
  if (resource >= 0 && resource < NXNumberOfResources &&
          request >= 0 && position >= 0)
  {
    SplitResourcePtr pResource = &nxagentSplitResources[resource];

    #ifdef TEST
    fprintf(stderr, "nxagentHandleCommitSplitEvent: Received event for resource [%d].\n",
                resource);
    #endif

    if (pResource != NULL && pResource -> commit == 1)
    {
      #ifdef TEST

      nxagentCheckResource(pResource, resource);

      fprintf(stderr, "nxagentHandleCommitSplitEvent: Committing request [%d] with "
                  "position [%d] for resource [%d].\n", request, position, resource);

      #endif

      NXCommitSplit(nxagentDisplay, resource, 1, request, position);
    }
    else
    {
      #ifdef TEST
      fprintf(stderr, "nxagentHandleCommitSplitEvent: Discarding request [%d] for "
                  "resource [%d] with position [%d].\n", request, resource, position);
      #endif

      NXCommitSplit(nxagentDisplay, resource, 0, request, position);
    }
  }
  #ifdef PANIC
  else
  {
    fprintf(stderr, "nxagentHandleCommitSplitEvent: PANIC! Invalid commit event with "
                "request [%d] and position [%d] for resource [%d].\n", request,
                    position, resource);
  }
  #endif
}

void nxagentHandleEndSplitEvent(int resource)
{
  if (resource >= 0 && resource < NXNumberOfResources)
  {
    SplitResourcePtr pResource = &nxagentSplitResources[resource];

    #ifdef TEST
    fprintf(stderr, "nxagentHandleEndSplitEvent: Received event for resource [%d].\n",
                resource);

    nxagentCheckResource(pResource, resource);

    #endif

    #ifdef TEST
    fprintf(stderr, "nxagentHandleEndSplitEvent: Checking if there is any commit pending.\n");
    #endif

    nxagentWaitCommitEvent(resource);

    if (pResource != NULL && pResource -> commit == 1)
    {
      #ifdef TEST

      fprintf(stderr, "nxagentHandleEndSplitEvent: Checking the split region at [%p] "
                  "for drawable at [%p].\n", (void *) pResource -> drawable,
                      (void *) pResource -> region);

      if (pResource -> region == NULL)
      {
        #ifdef PANIC
        fprintf(stderr, "nxagentHandleEndSplitEvent: PANIC! Invalid region [%p] for drawable at [%p].\n",
                    (void *) pResource -> region, (void *) pResource -> drawable);
        #endif

        FatalError("nxagentHandleEndSplitEvent: PANIC! Invalid region [%p] for drawable at [%p].\n",
                       (void *) pResource -> region, (void *) pResource -> drawable);
      }
      else if (pResource -> gc == NULL)
      {
        #ifdef PANIC
        fprintf(stderr, "nxagentHandleEndSplitEvent: PANIC! Invalid GC [%p] for drawable at [%p].\n",
                    (void *) pResource -> gc, (void *) pResource -> drawable);
        #endif

        FatalError("nxagentHandleEndSplitEvent: PANIC! Invalid GC [%p] for drawable at [%p].\n",
                       (void *) pResource -> gc, (void *) pResource -> drawable);
      }

      #endif

      if (pResource -> drawable != NULL &&
              pResource -> region != NullRegion)
      {
        if (REGION_NIL(pResource -> region) == 0)
        {
          REGION_SUBTRACT(pResource -> drawable -> pScreen,
                              nxagentCorruptedRegion(pResource -> drawable),
                                  nxagentCorruptedRegion(pResource -> drawable),
                                      pResource -> region);

/*
FIXME: Implementing the valid region policy

          nxagentExpandValidRegion(pResource -> drawable, pResource -> region);
*/

          #ifdef TEST
          fprintf(stderr, "nxagentHandleEndSplitEvent: Synchronized region [%d,%d,%d,%d] "
                      "for drawable at [%p].\n", pResource -> region -> extents.x1,
                          pResource -> region -> extents.y1, pResource -> region -> extents.x2,
                              pResource -> region -> extents.y2, (void *) pResource -> drawable);
          #endif
        }
        else
        {
          #ifdef PANIC
          fprintf(stderr, "nxagentHandleEndSplitEvent: PANIC! The region [%d,%d,%d,%d] for drawable "
                      "at [%p] is empty.\n", pResource -> region -> extents.x1,
                          pResource -> region -> extents.y1, pResource -> region -> extents.x2,
                              pResource -> region -> extents.y2, (void *) pResource -> drawable);
          #endif

          FatalError("nxagentHandleEndSplitEvent: PANIC! The region [%d,%d,%d,%d] for drawable "
                      "at [%p] is empty.\n", pResource -> region -> extents.x1,
                          pResource -> region -> extents.y1, pResource -> region -> extents.x2,
                              pResource -> region -> extents.y2, (void *) pResource -> drawable);
        }
      }
      else
      {
        #ifdef PANIC
        fprintf(stderr, "nxagentHandleEndSplitEvent: PANIC! Invalid record for resource [%d] with "
                    "pending [%d] split [%d] unpack [%d] drawable [%p] region [%p] commit [%d] gc [%p].\n",
                        resource, pResource -> pending, pResource -> split, pResource -> unpack,
                            (void *) pResource -> drawable, (void *) pResource -> region,
                                pResource -> commit, (void *) pResource -> gc);
        #endif

        FatalError("nxagentHandleEndSplitEvent: PANIC! Invalid record for resource [%d] with "
                       "pending [%d] split [%d] unpack [%d] drawable [%p] region [%p] commit [%d] gc [%p].\n",
                           resource, pResource -> pending, pResource -> split, pResource -> unpack,
                               (void *) pResource -> drawable, (void *) pResource -> region,
                                    pResource -> commit, (void *) pResource -> gc);
      }
    }

    /*
     * Release the resource.
     */

    nxagentFreeSplit(resource);
  }
  #ifdef TEST
  else
  {
    fprintf(stderr, "nxagentHandleEndSplitEvent: WARNING! Ignoring split event "
                "for resource [%d].\n", resource);
  }
  #endif
}

void nxagentHandleEmptySplitEvent()
{
/*
FIXME: Should run a consistency check here.
*/
  #ifdef TEST
  fprintf(stderr, "nxagentHandleEmptySplitEvent: No more split event to handle.\n");
  #endif
}

/*
 * The drawable is going to become corrupted.
 */

void nxagentSetCorruptedTimestamp(DrawablePtr pDrawable)
{
  if (nxagentDrawableStatus(pDrawable) == Synchronized)
  {
    if (pDrawable -> type == DRAWABLE_PIXMAP)
    {
      #ifdef TEST
      fprintf(stderr, "nxagentSetCorruptedTimestamp: Corruption timestamp for pixmap at [%p] was [%lu].\n",
                  (void *) pDrawable, nxagentPixmapPriv((PixmapPtr) pDrawable) -> corruptedTimestamp);
      #endif

      nxagentPixmapPriv((PixmapPtr) pDrawable) -> corruptedTimestamp = GetTimeInMillis();

      #ifdef TEST
      fprintf(stderr, "nxagentSetCorruptedTimestamp: New corruption timestamp for pixmap at [%p] is [%lu].\n",
                  (void *) pDrawable, nxagentPixmapPriv((PixmapPtr) pDrawable) -> corruptedTimestamp);
      #endif
    }
    else
    {
      #ifdef TEST
      fprintf(stderr, "nxagentSetCorruptedTimestamp: Corruption timestamp for window at [%p] was [%lu].\n",
                  (void *) pDrawable, nxagentWindowPriv((WindowPtr) pDrawable) -> corruptedTimestamp);
      #endif

      nxagentWindowPriv((WindowPtr) pDrawable) -> corruptedTimestamp = GetTimeInMillis();

      #ifdef TEST
      fprintf(stderr, "nxagentSetCorruptedTimestamp: New corruption timestamp for window at [%p] is [%lu].\n",
                  (void *) pDrawable, nxagentWindowPriv((WindowPtr) pDrawable) -> corruptedTimestamp);
      #endif
    }
  }
}

/*
 * Reset the timestamp taken when the drawable
 * became initially corrupted. The timestamp is
 * reset only after the drawable has been fully
 * synchronized.
 */

void nxagentResetCorruptedTimestamp(DrawablePtr pDrawable)
{
  if (nxagentDrawableStatus(pDrawable) == Synchronized)
  {
    if (pDrawable -> type == DRAWABLE_PIXMAP)
    {
      #ifdef TEST
      fprintf(stderr, "nxagentResetCorruptedTimestamp: Corruption timestamp for pixmap at [%p] was [%lu].\n",
                  (void *) pDrawable, nxagentPixmapPriv((PixmapPtr) pDrawable) -> corruptedTimestamp);
      #endif

      nxagentPixmapPriv((PixmapPtr) pDrawable) -> corruptedTimestamp = 0;
    }
    else
    {
      #ifdef TEST
      fprintf(stderr, "nxagentResetCorruptedTimestamp: Corruption timestamp for window at [%p] was [%lu].\n",
                  (void *) pDrawable, nxagentWindowPriv((WindowPtr) pDrawable) -> corruptedTimestamp);
      #endif

      nxagentWindowPriv((WindowPtr) pDrawable) -> corruptedTimestamp = 0;
    }
  }
}

