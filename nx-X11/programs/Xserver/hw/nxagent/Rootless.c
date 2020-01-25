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

#include "X.h"

#include "../../include/window.h"
#include "windowstr.h"
#include "colormapst.h"
#include "scrnintstr.h"
#include "propertyst.h"

#include "Agent.h"
#include "Display.h"
#include "Drawable.h"
#include "Windows.h"
#include "Pixmaps.h"
#include "Atoms.h"
#include "Trap.h"
#include "Utils.h"

#include "compext/Compext.h"

/*
 * Set here the required log level.
 */

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

/*
 * Assigned at the time the root window is initialized.
 */

typedef struct
{
  CARD32 flags;
  CARD32 input;
  CARD32 initial_state;
  CARD32 icon_pixmap;
  CARD32 icon_window;
  INT32  icon_x;
  INT32  icon_y;
  CARD32 icon_mask;
  CARD32 window_group;
}
nxagentWMHints;

/*
 * This structure is compatible with 32 and 64 bit library
 * interface. It has been copied from Xatomtype.h and it's a parameter
 * of XChangeProperty().
 */

typedef struct
{
  unsigned long flags;
  long          input;
  long          initialState;
  unsigned long iconPixmap;
  unsigned long iconWindow;
  long          iconX;
  long          iconY;
  unsigned long iconMask;
  unsigned long windowGroup;
}
nxagentPropWMHints;

WindowPtr nxagentRootlessWindow = NULL;

#define TOP_LEVEL_TABLE_UNIT 100

typedef struct {
  Window xid;
  WindowPtr pWin;
} TopLevelParentRec;

typedef struct {
  TopLevelParentRec *elt;
  int next;
  int size;
} TopLevelParentMap;

static TopLevelParentMap topLevelParentMap = { NULL, 0, 0 };

static void nxagentRemovePropertyFromList(void);

#if 0
/*
 * This is currently unused.
 */

#ifdef TEST

static void nxagentPrintRootlessTopLevelWindowMap(void);

void nxagentPrintRootlessTopLevelWindowMap(void)
{
  fprintf(stderr, "%s: Map size is [%d] num of entry [%d].\n", __func__,
              topLevelParentMap.size, topLevelParentMap.next);

  for (int i = 0; i < topLevelParentMap.next; i++)
  {
    fprintf(stderr, "%s:: [%d] pWin at [%p] XID at [%ld].\n", __func__,
                i, (void *) topLevelParentMap.elt[i].pWin, (long int) topLevelParentMap.elt[i].xid);
  }
}

#endif
#endif

void nxagentRootlessAddTopLevelWindow(WindowPtr pWin, Window w)
{
  for (int i = 0; i < topLevelParentMap.next; i++)
  {
    if (topLevelParentMap.elt[i].pWin == pWin)
    {
      #ifdef TEST
      fprintf(stderr, "%s: WARNING! Trying to add duplicated entry window at [%p] xid [%d].\n",
                  __func__, (void *) pWin, w);
      #endif

      topLevelParentMap.elt[i].xid = w;

      return;
    }
  }

  if (topLevelParentMap.next == topLevelParentMap.size)
  {
    size_t size = (topLevelParentMap.size += TOP_LEVEL_TABLE_UNIT);

    TopLevelParentRec *ptr = realloc(topLevelParentMap.elt, size * sizeof(TopLevelParentRec));

    if (ptr == NULL)
    {
      #ifdef WARNING
      fprintf(stderr, "%s: Warning failed to allocate memory.\n", __func__);
      #endif

      return;
    }

    topLevelParentMap.elt = ptr;
    topLevelParentMap.size = size;
  }

  topLevelParentMap.elt[topLevelParentMap.next].xid = w;
  topLevelParentMap.elt[topLevelParentMap.next].pWin = pWin;
  topLevelParentMap.next++;
}

WindowPtr nxagentRootlessTopLevelWindow(Window w)
{
  for (int i = 0; i < topLevelParentMap.next; i++)
  {
    if (w == topLevelParentMap.elt[i].xid)
    {
      return topLevelParentMap.elt[i].pWin;
    }
  }
  return NULL;
}

void nxagentRootlessDelTopLevelWindow(WindowPtr pWin)
{
  for (int i = 0; i < topLevelParentMap.next; i++)
  {
    if (pWin == topLevelParentMap.elt[i].pWin)
    {
      topLevelParentMap.elt[i] = topLevelParentMap.elt[topLevelParentMap.next - 1];
      topLevelParentMap.next--;

      return;
    }
  }
}

void nxagentConfigureRootlessWindow(WindowPtr pWin, int x, int y, int w, int h, int bw,
                                        WindowPtr pSib, int stack_mode, Mask mask)
{
  XWindowChanges changes = {
    .x = x,
    .y = y,
    .width = w,
    .height = h,
    .border_width = bw,
    .stack_mode = stack_mode
  };

  if (pSib)
  {
    changes.sibling = nxagentWindow(pSib);
  }

  XConfigureWindow(nxagentDisplay, nxagentWindow(pWin), mask, &changes);
}

void nxagentCirculateRootlessWindows(int direction)
{
  XCirculateSubwindows(nxagentDisplay, DefaultRootWindow(nxagentDisplay), direction);
}

#ifdef DEBUG

Bool nxagentRootlessTreesMatch(void)
{
  XlibWindow root_return;
  XlibWindow parent_return;
  XlibWindow *children_return = NULL;
  unsigned int nChildrenReturn;
  WindowPtr pTestWin = screenInfo.screens[0]->root -> firstChild;
  Bool treesMatch = True;

  Status result = XQueryTree(nxagentDisplay, DefaultRootWindow(nxagentDisplay),
                                 &root_return, &parent_return, &children_return, &nChildrenReturn);

  if (!result)
  {
    #ifdef WARNING
    fprintf(stderr, "%s: WARNING! Failed QueryTree request.\n", __func__);
    #endif
    return False;
  }

  while (nChildrenReturn > 0)
  {
    WindowPtr pW = nxagentWindowPtr(children_return[--nChildrenReturn]);
    if (!pW)
    {
      pW = nxagentRootlessTopLevelWindow(children_return[nChildrenReturn]);
    }

    if (pW && pW != screenInfo.screens[0]->root)
    {
      if (treesMatch && pTestWin && pTestWin == pW)
      {
        pTestWin = pTestWin -> nextSib;
      }
      else
      {
        treesMatch = False;
      }
    }
  }

  SAFE_XFree(children_return);

  return treesMatch;
}

#endif

#ifndef _XSERVER64
void nxagentRootlessRestack(Window children[], unsigned int nchildren)
#else
void nxagentRootlessRestack(unsigned long children[], unsigned int nchildren)
#endif
{
  WindowPtr *toplevel = malloc(sizeof(WindowPtr) * nchildren);

  if (!toplevel)
  {
    /* FIXME: Is this too much and we should simply return here? */
    FatalError("nxagentRootlessRestack: malloc() failed.");
  }

  unsigned int ntoplevel = 0;

  for(int i = 0; i < nchildren; i++)
  {
    WindowPtr pWin = nxagentWindowPtr(children[i]);

    if (!pWin)
    {
      pWin = nxagentRootlessTopLevelWindow(children[i]);
    }

    if (pWin && pWin != screenInfo.screens[0]->root)
    {
      toplevel[ntoplevel++] = pWin;
    }
  }

  if (!ntoplevel)
  {
    SAFE_free(toplevel);
    return;
  }

  #ifdef DEBUG

  fprintf(stderr, "%s: External top level windows before restack:\n", __func__);

  for (int i = 0; i < ntoplevel; i++)
  {
    fprintf(stderr, "%s: [%p]\n", __func__, (void *)toplevel[i]);
  }

  fprintf(stderr, "%s: Internal top level windows before restack:\n", __func__);

  for (WindowPtr pWin = screenInfo.screens[0]->root -> firstChild; pWin != NULL; pWin = pWin -> nextSib)
  {
    fprintf(stderr, "%s: [%p]\n", __func__, (void *)pWin);
  }

  #endif

  WindowPtr pWin = screenInfo.screens[0]->root -> firstChild;

  XID values[2] = {0, (XID) Above};

  for (int i = ntoplevel; i-- && pWin; pWin = toplevel[i] -> nextSib)
  {
    if (toplevel[i] != pWin)
    {
      Mask mask = CWSibling | CWStackMode;
      values[0] = pWin -> drawable.id;
      ClientPtr pClient = wClient(toplevel[i]);
      nxagentScreenTrap = True;
      ConfigureWindow(toplevel[i], mask, values, pClient);
      nxagentScreenTrap = False;

      #ifdef TEST
      fprintf(stderr, "%s: Restacked window [%p].\n", __func__, (void*) toplevel[i]);
      #endif
    }
  }

  #ifdef DEBUG

  fprintf(stderr, "%s: External top level windows after restack:\n", __func__);

  for (int i = 0; i < ntoplevel; i++)
  {
    fprintf(stderr, "%s: [%p]\n", __func__, (void *)toplevel[i]);
  }

  fprintf(stderr, "%s: Internal top level windows after restack:\n", __func__);

  for (pWin = screenInfo.screens[0]->root -> firstChild; pWin != NULL; pWin = pWin -> nextSib)
  {
    fprintf(stderr, "%s: [%p]\n", __func__, (void *)pWin);
  }

  #endif

  SAFE_free(toplevel);

  return;
}

/*
 * Determine if window is a top-level window.
 */

Window nxagentRootlessWindowParent(WindowPtr pWin)
{
  #ifdef TEST
  fprintf(stderr, "%s: Called for window at [%p][%d] with parent [%p][%d].\n", __func__,
              (void *) pWin, nxagentWindowPriv(pWin)->window, (void *) pWin->parent,
                  (pWin->parent ? nxagentWindowPriv(pWin->parent)->window : 0));
  #endif

  if (pWin -> parent == NULL)
  {
    return DefaultRootWindow(nxagentDisplay);
  }
  else if (pWin -> parent == nxagentRootlessWindow)
  {
    return DefaultRootWindow(nxagentDisplay);
  }
  else
  {
    return nxagentWindow(pWin -> parent);
  }
}

int nxagentExportAllProperty(WindowPtr pWin)
{
  int total = 0;

  for (PropertyPtr pProp = wUserProps(pWin); pProp; pProp = pProp->next)
  {
    total += nxagentExportProperty(pWin,
                                       pProp->propertyName,
                                           pProp->type,
                                               pProp->format,
                                                   PropModeReplace,
                                                       pProp->size,
                                                           pProp->data);
  }

  return total;
}

int nxagentExportProperty(WindowPtr pWin,
                          Atom property,
                          Atom type,
                          int format,
                          int mode,
                          unsigned long nUnits,
                          void *value)
{
  char *output = NULL;
  Bool export = False;
  Bool freeMem = False;

  if (NXDisplayError(nxagentDisplay) == 1)
  {
    return 0;
  }

  const char *propertyS = NameForAtom(property);
  const char *typeS = NameForAtom(type);

  if (strncmp(propertyS, "WM_", 3) != 0 &&
          strncmp(propertyS, "_NET_", 5) != 0 &&
              strcmp(propertyS, "_KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR") != 0)
  {
    #ifdef TEST
    fprintf(stderr, "%s: WARNING! Ignored ChangeProperty on %swindow [0x%x] property [%s] "
                "type [%s] nUnits [%ld] format [%d]\n", __func__,
                    nxagentWindowTopLevel(pWin) ? "toplevel " : "", nxagentWindow(pWin),
                        validateString(propertyS), validateString(typeS), nUnits, format);
    #endif
  }
  else if (strcmp(typeS, "STRING") == 0 ||
               #ifndef _XSERVER64
               strcmp(typeS, "CARDINAL") == 0 ||
                   strcmp(typeS, "WM_SIZE_HINTS") == 0 ||
               #endif
                       strcmp(typeS, "UTF8_STRING") == 0)
  {
    output = value;
    export = True;
  }
  #ifdef _XSERVER64
  else if (strcmp(typeS, "CARDINAL") == 0 || strcmp(typeS, "WM_SIZE_HINTS") == 0)
  {
    unsigned long *buffer = malloc(nUnits * sizeof(*buffer));
    if (buffer == NULL)
    {
      FatalError("%s: malloc() failed.", __func__);
    }

    int *input = value;

    if (buffer)
    {
      freeMem = True;
      export = True;
      output = (char*) buffer;

      for (int i = 0; i < nUnits; i++)
      {
        buffer[i] = input[i];
      }
    }
  }
  #endif
  else if (strcmp(typeS, "WM_HINTS") == 0)
  {
    ClientPtr pClient = wClient(pWin);
    nxagentWMHints wmHints = *(nxagentWMHints*)value;

    wmHints.flags |= InputHint;
    wmHints.input = True;

    /*
     * Initialize the structure used in XChangeProperty().
     */

    nxagentPropWMHints propHints = {
      .flags = wmHints.flags,
      .input = (wmHints.input == True ? 1 : 0),
      .initialState = wmHints.initial_state,
      .iconPixmap = wmHints.icon_pixmap,
      .iconWindow = wmHints.icon_window,
      .iconX = wmHints.icon_x,
      .iconY = wmHints.icon_y,
      .iconMask = wmHints.icon_mask,
      .windowGroup = wmHints.window_group
    };

    output = (char*) &propHints;
    export = True;

    if ((wmHints.flags & IconPixmapHint) && (wmHints.icon_pixmap != None))
    {
      PixmapPtr icon = (PixmapPtr)SecurityLookupIDByType(pClient, wmHints.icon_pixmap,
                                                             RT_PIXMAP, DixDestroyAccess);

      if (icon)
      {
        if (nxagentDrawableStatus((DrawablePtr) icon) == NotSynchronized)
        {
          nxagentSynchronizeRegion((DrawablePtr) icon, NullRegion, NEVER_BREAK, NULL);
        }

        propHints.iconPixmap = nxagentPixmap(icon);
      }
      else
      {
        propHints.flags &= ~IconPixmapHint;

        #ifdef WARNING
        fprintf(stderr, "%s: WARNING! Failed to look up icon pixmap [0x%x] from hint "
                    "exporting property [%s] type [%s] on window [%p].\n", __func__,
                        (unsigned int) wmHints.icon_pixmap, propertyS, typeS,
                            (void *) pWin);
        #endif
      }
    }

    if ((wmHints.flags & IconWindowHint) && (wmHints.icon_window != None))
    {
      WindowPtr icon = (WindowPtr)SecurityLookupWindow(wmHints.icon_window, pClient,
                                                  DixDestroyAccess);

      if (icon)
      {
        propHints.iconWindow = nxagentWindow(icon);
      }
      else
      {
        propHints.flags &= ~IconWindowHint;

        #ifdef WARNING
        fprintf(stderr, "%s: WARNING! Failed to look up icon window [0x%x] from hint "
                    "exporting property [%s] type [%s] on window [%p].\n", __func__,
                        (unsigned int) wmHints.icon_window, propertyS, typeS,
                            (void *) pWin);
        #endif
      }
    }

    if ((wmHints.flags & IconMaskHint) && (wmHints.icon_mask != None))
    {
      PixmapPtr icon = (PixmapPtr)SecurityLookupIDByType(pClient, wmHints.icon_mask,
                                                             RT_PIXMAP, DixDestroyAccess);

      if (icon)
      {
        propHints.iconMask = nxagentPixmap(icon);
      }
      else
      {
        propHints.flags &= ~IconMaskHint;

        #ifdef WARNING
        fprintf(stderr, "%s: WARNING! Failed to look up icon mask [0x%x] from hint "
                    "exporting property [%s] type [%s] on window [%p].\n", __func__,
                        (unsigned int) wmHints.icon_mask, propertyS, typeS,
                            (void *) pWin);
        #endif
      }
    }

    if ((wmHints.flags & WindowGroupHint) && (wmHints.window_group != None))
    {
      WindowPtr window = (WindowPtr)SecurityLookupWindow(wmHints.window_group, pClient,
                                                  DixDestroyAccess);

      if (window)
      {
        propHints.windowGroup = nxagentWindow(window);
      }
      else
      {
        propHints.flags &= ~WindowGroupHint;

        #ifdef WARNING
        fprintf(stderr, "%s: WARNING! Failed to look up window group [0x%x] from hint "
                    "exporting property [%s] type [%s] on window [%p].\n", __func__,
                        (unsigned int) wmHints.window_group, propertyS, typeS,
                            (void *) pWin);
        #endif
      }
    }
  }
  else if (strcmp(typeS, "ATOM") == 0)
  {
    XlibAtom *atoms = malloc(nUnits * sizeof(*atoms));
    Atom *input = value;
    const char *atomName = NULL;
    int j = 0;

    if (!atoms)
    {
      #ifdef WARNING
      fprintf(stderr, "%s: WARNING! malloc() failed for '[%s]'- bailing out.\n", __func__, typeS);
      #endif
      return False;
    }

    freeMem = True;
    export = True;
    output = (char *) atoms;

    for (int i = 0; i < nUnits; i++)
    {
      /*
       * Exporting the _NET_WM_PING property could result in rootless
       * windows being grayed out when the compiz window manager is
       * running.
       *
       * Better solution would probably be to handle the communication
       * with the window manager instead of just getting rid of the
       * property.
       */

      if ((atomName = NameForAtom(input[i])) != NULL &&
              strcmp(atomName, "_NET_WM_PING") != 0)
      {
        atoms[j] = nxagentLocalToRemoteAtom(input[i]);

        if (atoms[j] == None)
        {
          #ifdef WARNING
          fprintf(stderr, "%s: WARNING! Failed to convert local atom [%ld] [%s].\n", __func__,
                      (long int) input[i], validateString(atomName));
          #endif
        }

        j++;
      }
      #ifdef TEST
      else
      {
        fprintf(stderr, "%s: WARNING! "
		"Not exporting the _NET_WM_PING property.\n", __func__);
      }
      #endif
    }

    nUnits = j;
  }
  else if (strcmp(typeS, "WINDOW") == 0)
  {
    Window *input = value;
    XlibWindow *wind = malloc(nUnits * sizeof(*wind));
    ClientPtr pClient = wClient(pWin);

    if (!wind)
    {
      #ifdef WARNING
      fprintf(stderr, "%s: WARNING! malloc() failed for '[%s]' - bailing out.\n", __func__, typeS);
      #endif
      return False;
    }

    freeMem = True;
    export = True;
    output = (char*) wind;

    for (int i = 0; i < nUnits; i++)
    {
      WindowPtr pWindow = (WindowPtr)SecurityLookupWindow(input[i], pClient,
                                                              DixDestroyAccess);
      if ((input[i] != None) && pWindow)
      {
        wind[i] = nxagentWindow(pWindow);
      }
      else
      {
        #ifdef WARNING
        fprintf(stderr, "%s: WARNING! Failed to look up window [%ld] "
                    "exporting property [%s] type [%s] on window [%p].\n", __func__,
                        (long int) input[i], propertyS, typeS, (void *) pWin);
        #endif

        /*
         * It seems that clients specify strange windows, perhaps are
         * not real windows so we can try to let them pass anyway.
         *
         * wind[i] = None;
         */
      }
    }
  }

  if (export)
  {
    XlibAtom propertyX = nxagentLocalToRemoteAtom(property);
    XlibAtom typeX = nxagentLocalToRemoteAtom(type);

    if (propertyX == None || typeX == None)
    {
      #ifdef WARNING
      fprintf(stderr, "%s: WARNING! Failed to convert local atom.\n", __func__);
      #endif

      export = 0;
    }
    else
    {
      #ifdef TEST
      fprintf(stderr, "%s: Property [%u] format [%i] units [%lu].\n", __func__,
                  propertyX, format, nUnits);
      #endif

      if ((format >> 3) * nUnits + sizeof(xChangePropertyReq) <
              (MAX_REQUEST_SIZE << 2))
      {
        XChangeProperty(nxagentDisplay, nxagentWindow(pWin), propertyX, typeX,
                            format, mode, (void*)output, nUnits);
      }
      else if (mode == PropModeReplace)
      {
        char * data = (char *) output;

        XDeleteProperty(nxagentDisplay, nxagentWindow(pWin), propertyX);

        while (nUnits > 0)
        {
          int n;

          if ((format >> 3) * nUnits + sizeof(xChangePropertyReq) <
                  (MAX_REQUEST_SIZE << 2))
          {
            n = nUnits;
          }
          else
          {
            n = ((MAX_REQUEST_SIZE << 2) - sizeof(xChangePropertyReq)) /
                    (format >> 3);
          }

          XChangeProperty(nxagentDisplay, nxagentWindow(pWin), propertyX,
                              typeX, format, PropModeAppend, (void*) data, n);

          nUnits -= n;

          data = (char *) data + n * (format >> 3);
        }
      }
      else
      {
        #ifdef WARNING
        fprintf(stderr, "%s: WARNING! Property [%lu] too long.\n", __func__,
                    (long unsigned int)propertyX);
        #endif

        goto nxagentExportPropertyError;
      }

      nxagentAddPropertyToList(propertyX, pWin);
    }
  }
  else
  {
    #ifdef TEST
    fprintf(stderr, "%s: WARNING! Ignored ChangeProperty on %swindow [0x%x] property [%s] "
                "type [%s] nUnits [%ld] format [%d]\n", __func__,
                    nxagentWindowTopLevel(pWin) ? "toplevel " : "",
                        nxagentWindow(pWin), validateString(propertyS), validateString(typeS),
                            nUnits, format);
    #endif
  }

  nxagentExportPropertyError:

  if (freeMem)
  {
    SAFE_free(output);
  }

  return export;
}

void nxagentImportProperty(Window window,
                           XlibAtom property,
                           XlibAtom type,
                           int format,
                           unsigned long nitems,
                           unsigned long bytes_after,
                           unsigned char *buffer)
{
  Bool import = False;
  Bool freeMem = False;

  typedef struct {
      CARD32 state;
      Window icon;
    } WMState;
  WMState wmState;

  char *output = NULL;

  WindowPtr pWin = nxagentWindowPtr(window);

  if (pWin == NULL)
  {
    #ifdef TEST
    fprintf(stderr, "%s: Failed to look up remote window [0x%x]  property [%d] exiting.\n",
                __func__, window, property);
    #endif

    return;
  }

  Atom propertyL = nxagentRemoteToLocalAtom(property);

  if (!ValidAtom(propertyL))
  {
    #ifdef TEST
    fprintf(stderr, "%s: Failed to convert remote property atom.\n", __func__);
    #endif

    return;
  }

  #ifdef TEST
  fprintf(stderr, "%s: Window [0x%x] property [%d]: [%s]\n", __func__,
              window, property, validateString(NameForAtom(propertyL)));
  #endif

  /*
   * We settle a property size limit of 256K beyond which we simply
   * ignore them.
   */

  Atom typeL = nxagentRemoteToLocalAtom(type);
  const char *typeS = NameForAtom(typeL);

  if (buffer == NULL && (nitems > 0))
  {
    #ifdef WARNING
    fprintf(stderr, "%s: Failed to retrieve remote property [%ld] [%s] on Window [%ld]\n", __func__,
                (long int) property, validateString(NameForAtom(propertyL)), (long int) window);
    #endif
  }
  else if (bytes_after != 0)
  {
    #ifdef WARNING
    fprintf(stderr, "%s: Remote property bigger than maximum limits.\n", __func__);
    #endif
  }
  else if (!ValidAtom(typeL))
  {
    #ifdef WARNING
    fprintf(stderr, "%s: Failed to convert remote atoms [%ld].\n", __func__,
                (long int) type);
    #endif
  }
  else if (nitems == 0)
  {
    #ifdef TEST
    fprintf(stderr, "%s: Importing void property.\n", __func__);
    #endif

    import = True;
  }
  else if (strcmp(typeS, "STRING") == 0 ||
               strcmp(typeS, "UTF8_STRING") == 0 ||
                   strcmp(typeS, "CARDINAL") == 0 ||
                       strcmp(typeS, "WM_SIZE_HINTS") == 0)
  {
    output = (char*)buffer;
    import = True;
  }
  else if (strcmp(typeS, "WM_STATE") == 0)
  {
    /*
     * Contents of property of type WM_STATE are {CARD32 state, WINDOW
     * icon}. Only the icon field has to be modified before importing
     * the property.
     */

    wmState = *(WMState*)buffer;
    WindowPtr pIcon = nxagentWindowPtr(wmState.icon);

    if (pIcon || wmState.icon == None)
    {
      import = True;
      output = (char*) &wmState;
      wmState.icon = pIcon ? nxagentWindow(pIcon) : None;
    }
    else if (wmState.icon)
    {
      #ifdef WARNING
      fprintf(stderr, "%s: WARNING! Failed to convert remote window [%ld] importing property [%ld]"
                  " of type WM_STATE", __func__, (long int) wmState.icon, (long int) property);
      #endif
    }
  }
  else if (strcmp(typeS, "WM_HINTS") == 0)
  {
    nxagentWMHints wmHints = *(nxagentWMHints*)buffer;
    output = (char*) &wmHints;
    import = True;

    if ((wmHints.flags & IconPixmapHint) && (wmHints.icon_pixmap != None))
    {
      PixmapPtr icon = nxagentPixmapPtr(wmHints.icon_pixmap);

      if (icon)
      {
        wmHints.icon_pixmap = icon -> drawable.id;
      }
      else
      {
        wmHints.flags &= ~IconPixmapHint;

        #ifdef WARNING
        fprintf(stderr, "%s: WARNING! Failed to look up remote icon pixmap [%d] from hint importing"
                    " property [%ld] type [%s] on window [%p].\n", __func__,
                        (unsigned int) wmHints.icon_pixmap, (long int) property,
                            typeS, (void *) pWin);
        #endif
      }
    }

    if ((wmHints.flags & IconWindowHint) && (wmHints.icon_window != None))
    {
      WindowPtr icon = nxagentWindowPtr(wmHints.icon_window);

      if (icon)
      {
        wmHints.icon_window = icon -> drawable.id;
      }
      else
      {
        wmHints.flags &= ~IconWindowHint;

        #ifdef WARNING
        fprintf(stderr, "%s: WARNING! Failed to look up remote icon window [0x%x] from hint importing"
                " property [%ld] type [%s] on window [%p].\n", __func__,
                         (unsigned int) wmHints.icon_window,
                             (long int) property, typeS, (void *) pWin);
        #endif
      }
    }

    if ((wmHints.flags & IconMaskHint) && (wmHints.icon_mask != None))
    {
      PixmapPtr icon = nxagentPixmapPtr(wmHints.icon_mask);

      if (icon)
      {
        wmHints.icon_mask = icon -> drawable.id;
      }
      else
      {
        wmHints.flags &= ~IconMaskHint;

        #ifdef WARNING
        fprintf(stderr, "%s: WARNING! Failed to look up remote icon mask [0x%x] from hint importing"
                    " property [%ld] type [%s] on window [%p].\n", __func__,
                          (unsigned int) wmHints.icon_mask, (long int) property, typeS, (void *) pWin);
        #endif
      }
    }

    if ((wmHints.flags & WindowGroupHint) && (wmHints.window_group != None))
    {
      WindowPtr group = nxagentWindowPtr(wmHints.window_group);

      if (group)
      {
        wmHints.window_group = group -> drawable.id;
      }
      else
      {
        wmHints.flags &= ~WindowGroupHint;

        #ifdef WARNING
        fprintf(stderr, "%s: WARNING! Failed to look up remote window group [0x%x] from hint importing"
                    " property [%ld] type [%s] on window [%p].\n", __func__,
                          (unsigned int) wmHints.window_group,
                              (long int) property, typeS, (void *) pWin);
        #endif
      }
    }
  }
  else if (strcmp(typeS, "ATOM") == 0)
  {
    Atom *atoms = malloc(nitems * sizeof(Atom));
    CARD32 *input = (CARD32*) buffer;

    if (atoms == NULL)
    {
      #ifdef WARNING
      fprintf(stderr, "%s: WARNING! malloc() failed for '[%s]' - bailing out.\n", __func__, typeS);
      #endif
      return;
    }

    freeMem = True;
    import = True;
    output = (char *) atoms;

    for (int i = 0; i < nitems; i++)
    {
      atoms[i] = nxagentRemoteToLocalAtom((XlibAtom)input[i]);

      if (atoms[i] == None)
      {
        #ifdef WARNING
        fprintf(stderr, "%s: WARNING! Failed to convert remote atom [%ld].\n", __func__,
                    (long int) input[i]);
        #endif
      }
    }
  }
  else if (strcmp(typeS, "WINDOW") == 0)
  {
    Window *input = (Window*) buffer;
    Window *wind = malloc(nitems * sizeof(Window));
    WindowPtr pWindow;

    if (!wind)
    {
      #ifdef WARNING
      fprintf(stderr, "%s: WARNING! malloc() failed for '[%s]' - bailing out.\n", __func__, typeS);
      #endif
      return;
    }
    freeMem = True;
    import = True;
    output = (char*) wind;

    for (int i = 0; i < nitems; i++)
    {
      pWindow = nxagentWindowPtr(input[i]);

      if (pWindow)
      {
        wind[i] = pWindow -> drawable.id;
      }
      else
      {
        #ifdef WARNING
        fprintf(stderr, "%s: WARNING! Failed to look up remote window [0x%lx] importing property [%ld]"
                    " type [%s] on window [%p].\n", __func__,
                        (long int) input[i], (long int) property, typeS, (void*)pWin);
        #endif

        wind[i] = None;
      }
    }
  }

  if (import)
  {
    #ifdef TEST
    fprintf(stderr, "%s: ChangeProperty on window [0x%x] property [%d] type [%s]"
                " nitems [%ld] format [%d]\n", __func__,
                    window, property, typeS, nitems, format);
    #endif

    ChangeWindowProperty(pWin, propertyL, typeL, format,
                             PropModeReplace, nitems, output, 1);
  }
  else
  {
    #ifdef TEST
    fprintf(stderr, "%s: WARNING! Ignored ChangeProperty on window [0x%x] property [%d] type [%s]"
                " ntems [%ld] format [%d]\n", __func__,
                       window, property, validateString(typeS), nitems, format);
    #endif
  }

  if (freeMem)
  {
    SAFE_free(output);
  }

  return;
}

/*
 * We want to import all properties changed by external clients to
 * reflect properties of our internal windows but we must ignore all
 * the property notify events generated by our own requests.  For this
 * purpose we implement a FIFO to record every change property request
 * that we dispatch. In this way, when processing a property notify,
 * we can distinguish between the notifications generated by our
 * requests from those generated by other clients connected to the
 * real X server.
 */

struct nxagentPropertyRec{
  Window window;
  XlibAtom property;
  struct nxagentPropertyRec *next;
};

static struct{
  struct nxagentPropertyRec *first;
  struct nxagentPropertyRec *last;
  int size;
} nxagentPropertyList = {NULL, NULL, 0};

/*
 * Removing first element from list.
 */

void nxagentRemovePropertyFromList(void)
{
  struct nxagentPropertyRec *tmp = nxagentPropertyList.first;

  #ifdef TEST
  fprintf(stderr, "%s: Property [%d] on Window [0x%x] to list, list size is [%d].\n", __func__,
              nxagentPropertyList.first -> property, nxagentPropertyList.first -> window,
                 nxagentPropertyList.size);
  #endif

  if (nxagentPropertyList.first)
  {
    nxagentPropertyList.first = nxagentPropertyList.first -> next;

    if (--nxagentPropertyList.size == 0)
    {
      nxagentPropertyList.last = NULL;
    }

    SAFE_free(tmp);
  }
}

/*
 * Add the record to the list.
 */

void nxagentAddPropertyToList(XlibAtom property, WindowPtr pWin)
{
  if (NXDisplayError(nxagentDisplay) == 1)
  {
    return;
  }

  struct nxagentPropertyRec *tmp = malloc(sizeof(struct nxagentPropertyRec));
  if (tmp == NULL)
  {
    FatalError("%s: malloc() failed.", __func__);
  }

  #ifdef TEST
  fprintf(stderr, "%s: Adding record Property [%d] - Window [0x%x][%p] to list, list"
              " size is [%d].\n", __func__, property, nxagentWindow(pWin), (void*) pWin,
                 nxagentPropertyList.size);
  #endif

  tmp -> property = property;
  tmp -> window = nxagentWindow(pWin);
  tmp -> next = NULL;

  if (nxagentPropertyList.size == 0)
  {
    nxagentPropertyList.first = tmp;
  }
  else
  {
    nxagentPropertyList.last -> next = tmp;
  }

  nxagentPropertyList.last = tmp;
  nxagentPropertyList.size++;
}

void nxagentFreePropertyList(void)
{
  while (nxagentPropertyList.size != 0)
  {
    nxagentRemovePropertyFromList();
  }
}

/*
 * We are trying to distinguish notifications generated by an external
 * client from those genarated by our own requests.
 */

Bool nxagentNotifyMatchChangeProperty(void *p)
{
  struct nxagentPropertyRec *first = nxagentPropertyList.first;
  XPropertyEvent *X = p;

  #ifdef TEST
  fprintf(stderr, "%s: Property notify on window [0x%lx] property [%ld].\n", __func__,
              X -> window, X -> atom);

  if (first)
  {
    fprintf(stderr, "%s: First element on list is window [0x%x] property [%d] list size is [%d].\n", __func__,
                first -> window, first -> property, nxagentPropertyList.size);
  }
  else
  {
    fprintf(stderr, "%s: List is empty.\n", __func__);
  }
  #endif

  if (first == NULL ||
          X -> window != first -> window ||
              X -> atom != first -> property)
  {
    return False;
  }

  nxagentRemovePropertyFromList();

  return True;
}
