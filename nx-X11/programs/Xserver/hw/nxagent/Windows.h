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

#ifndef __Window_H__
#define __Window_H__

#include "Options.h"
#include "Rootless.h"
#include "Pixmaps.h"

#include "validate.h"

typedef struct
{
  Window window;
  int x;
  int y;
  unsigned int width;
  unsigned int height;
  unsigned int borderWidth;
  Window siblingAbove;
  int backingStore;
#ifdef SHAPE
  RegionPtr boundingShape;
  RegionPtr clipShape;
#endif /* SHAPE */

  void *pPicture;

  /*
   * Set if the window is mapped
   * on the remote server.
   */

  int isMapped;

  /*
   * Set if the window on the remote
   * server is redirected by using
   * the composite extension.
   */

  int isRedirected;

  int visibilityState;

  RegionPtr corruptedRegion;

  int hasTransparentChildren;

  int containGlyphs;

  int deferredBackgroundExpose;

  XID corruptedId;

  PixmapPtr synchronizationBitmap;

  Time corruptedTimestamp;

  SplitResourcePtr splitResource;

} nxagentPrivWindowRec;

typedef nxagentPrivWindowRec *nxagentPrivWindowPtr;

typedef struct
{
  unsigned long storingPixmapId;

  PixmapPtr pStoringPixmap;

  WindowPtr pSavedWindow;

  int backingStoreX;

  int backingStoreY;

} StoringPixmapRec;

typedef StoringPixmapRec *StoringPixmapPtr;

int nxagentAddItemBSPixmapList(unsigned long, PixmapPtr, WindowPtr, int, int);
int nxagentRemoveItemBSPixmapList(unsigned long);
void nxagentInitBSPixmapList(void);
int nxagentEmptyBSPixmapList(void);
StoringPixmapPtr nxagentFindItemBSPixmapList (unsigned long);

extern int nxagentWindowPrivateIndex;

#define nxagentWindowPriv(pWin) \
  ((nxagentPrivWindowPtr)((pWin)->devPrivates[nxagentWindowPrivateIndex].ptr))

#define nxagentWindow(pWin) (nxagentWindowPriv(pWin)->window)

/*
 * Window is either a child of our root
 * or a child of the root of the real X
 * server.
 */

#define nxagentWindowParent(pWin) \
    (nxagentOption(Rootless) ? \
         nxagentRootlessWindowParent(pWin) : \
             ((pWin)->parent ? \
                 nxagentWindow((pWin)->parent) : \
                     nxagentDefaultWindows[pWin->drawable.pScreen->myNum]))

/*
 * True if this is a top level window.
 */

#define nxagentWindowTopLevel(pWin) \
    (pWin && (pWin -> parent == NULL || \
         pWin->parent == nxagentRootlessWindow))

#define nxagentWindowSiblingAbove(pWin) \
  ((pWin)->prevSib ? nxagentWindow((pWin)->prevSib) : None)

#define nxagentWindowSiblingBelow(pWin) \
  ((pWin)->nextSib ? nxagentWindow((pWin)->nextSib) : None)

#define nxagentWindowCorruptedRegion(pWin) \
  (nxagentWindowPriv(pWin) -> corruptedRegion)

#define nxagentWindowContainGlyphs(pWin) \
  (nxagentWindowPriv(pWin) -> containGlyphs)

#define nxagentWindowTimestamp(pWin) \
  (nxagentWindowPriv(pWin) -> corruptedTimestamp)

#define nxagentWindowIsVisible(pWin) \
  ((pWin) -> viewable == 1 && \
      (pWin) -> drawable.class != InputOnly && \
          (pWin) -> visibility != VisibilityFullyObscured)

#define nxagentDefaultWindowIsVisible() \
              (nxagentVisibility != VisibilityFullyObscured)

#define CWParent CWSibling
#define CWStackingOrder CWStackMode

#define CW_Map    (1 << 15)
#define CW_Update (1 << 16)
#define CW_Shape  (1 << 17)
#define CW_RootlessRestack  (1 << 18)

/*
 * This force the agent to send exposures
 * for all windows.
 */

#define nxagentRefreshScreen() \
do\
{\
  nxagentRefreshWindows(WindowTable[0]);\
} while (0)

WindowPtr nxagentWindowPtr(Window window);

extern Atom serverCutProperty;

/*
 * If the rectangles in an exposed region exceed
 * the number of 4, we let the function decide if
 * it is better to send the window extents rather
 * than the rectangles in the region.
 */

int nxagentExtentsPredicate(int total);

/*
 * Agent's nested window procedures. Look also
 * at Rootless.h for the rootless counterparts.
 */

Bool nxagentCreateWindow(WindowPtr pWin);

Bool nxagentDestroyWindow(WindowPtr pWin);

Bool nxagentPositionWindow(WindowPtr pWin, int x, int y);

Bool nxagentChangeWindowAttributes(WindowPtr pWin, unsigned long mask);

Bool nxagentRealizeWindow(WindowPtr pWin);

Bool nxagentUnrealizeWindow(WindowPtr pWin);

Bool nxagentCheckIllegalRootMonitoring(WindowPtr pWin, Mask mask);

void nxagentWindowExposures(WindowPtr pWin, RegionPtr pRgn, RegionPtr other_exposed);

void nxagentPaintWindowBackground(WindowPtr pWin, RegionPtr pRegion, int what);

void nxagentPaintWindowBorder(WindowPtr pWin, RegionPtr pRegion, int what);

void nxagentCopyWindow(WindowPtr pWin, xPoint oldOrigin, RegionPtr oldRegion);

void nxagentClipNotify(WindowPtr pWin, int dx, int dy);

void nxagentRestackWindow(WindowPtr pWin, WindowPtr pOldNextSib);

void nxagentReparentWindow(WindowPtr pWin, WindowPtr pOldParent);

void nxagentRefreshWindows(WindowPtr pWin);

void nxagentSetTopLevelEventMask(WindowPtr pWin);

void nxagentSwitchFullscreen(ScreenPtr pScreen, Bool switchOn);

void nxagentSwitchAllScreens(ScreenPtr pScreen, Bool switchOn);

void nxagentMoveViewport(ScreenPtr pScreen, int hShift, int vShift);

#ifdef VIEWPORT_FRAME

void nxagentUpdateViewportFrame(int x, int y, int w, int h);

#else /* #ifdef VIEWPORT_FRAME */

#define nxagentUpdateViewportFrame(x, y, w, h)

#endif /* #ifdef VIEWPORT_FRAME */

void nxagentUnmapWindows(void);

void nxagentMapDefaultWindows(void);

Bool nxagentSetWindowCursors(void *p0);

/*
 * The ConfigureWindow procedure has not
 * a pointer in the screen structure.
 */

void nxagentConfigureWindow(WindowPtr pWin, unsigned int mask);

/*
 * Used to track nxagent window's visibility.
 */

extern int nxagentVisibility;
extern unsigned long nxagentVisibilityTimeout;
extern Bool nxagentVisibilityStop;

/*
 * Return the pointer to the window given the
 * remote id. It tries to match the id from
 * the last matched window before iterating
 * through the hierarchy.
 */

WindowPtr nxagentGetWindowFromID(Window id);

/*
 * Handle the shape bitmap for windows.
 */

#ifdef SHAPE

void nxagentShapeWindow(WindowPtr pWin);

#endif

extern Window nxagentConfiguredSynchroWindow;
extern Bool nxagentExposeArrayIsInitialized;

typedef struct _ConfiguredWindow
{
  WindowPtr pWin;
  struct _ConfiguredWindow *next;
  struct _ConfiguredWindow *prev;
  unsigned int valuemask;
} ConfiguredWindowStruct;

ConfiguredWindowStruct *nxagentConfiguredWindowList;

typedef struct _StaticResizedWindow
{
  WindowPtr pWin;
  struct _StaticResizedWindow *next;
  struct _StaticResizedWindow *prev;
  unsigned long sequence;
  int offX;
  int offY;
} StaticResizedWindowStruct;

StaticResizedWindowStruct *nxagentStaticResizedWindowList;

void nxagentPostValidateTree(WindowPtr pParent, WindowPtr pChild, VTKind kind);

void nxagentFlushConfigureWindow(void);

void nxagentAddConfiguredWindow(WindowPtr pWin, unsigned int valuemask);

void nxagentDeleteConfiguredWindow(WindowPtr pWin);

void nxagentAddStaticResizedWindow(WindowPtr pWin, unsigned long sequence, int offX, int offY);

void nxagentDeleteStaticResizedWindow(unsigned long sequence);

StaticResizedWindowStruct *nxagentFindStaticResizedWindow(unsigned long sequence);

void nxagentEmptyAllBackingStoreRegions(void);

#endif /* __Window_H__ */
