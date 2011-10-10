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

#include "X.h"

#include "../../include/window.h"
#include "windowstr.h"
#include "colormapst.h"
#include "propertyst.h"

#include "Agent.h"
#include "Display.h"
#include "Drawable.h"
#include "Windows.h"
#include "Pixmaps.h"
#include "Atoms.h"
#include "Trap.h"

#include "NXlib.h"

/*
 * Set here the required log level.
 */

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

/*
 * Assigned at the time the root window is
 * initialized.
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
 * This structure is compatible with 32
 * and 64 bit library interface. It has
 * been copied from Xatomtype.h and it's
 * a parameter of XChangeProperty().
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

/*
 * This is currently unused.
 */

#ifdef TEST

static void nxagentPrintRootlessTopLevelWindowMap(void);

void nxagentPrintRootlessTopLevelWindowMap()
{
  int i;

  fprintf(stderr, "nxagentPrintRootlessTopLevelWindowMap: Map size is [%d] num of entry [%d].\n",
              topLevelParentMap.size, topLevelParentMap.next);

  for (i = 0; i < topLevelParentMap.next; i++)
  {
    fprintf(stderr, "nxagentPrintRootlessTopLevelWindowMap: [%d] pWin at [%p] XID at [%ld].\n",
                i, (void *) topLevelParentMap.elt[i].pWin, (long int) topLevelParentMap.elt[i].xid);
  }
}

#endif

void nxagentRootlessAddTopLevelWindow(WindowPtr pWin, Window w)
{
  int i;

  for (i = 0; i < topLevelParentMap.next; i++)
  {
    if (topLevelParentMap.elt[i].pWin == pWin)
    {
      #ifdef TEST
      fprintf(stderr, "nxagentRootlessAddTopLevelWindow: WARNING! "
                  "Trying to add duplicated entry window at [%p] xid [%ld].\n",
                      (void *) pWin, w);
      #endif

      topLevelParentMap.elt[i].xid = w;

      return;
    }
  }

  if (topLevelParentMap.next == topLevelParentMap.size)
  {
    TopLevelParentRec *ptr = topLevelParentMap.elt;
    size_t size = (topLevelParentMap.size += TOP_LEVEL_TABLE_UNIT);

    ptr = realloc(ptr, size * sizeof(TopLevelParentRec));

    if (ptr == NULL)
    {
      #ifdef WARNING
      fprintf(stderr, "nxagentRootlessAddTopLevelWindow: Warning failed to allocate memory.\n");
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
  int i;

  for (i = 0; i < topLevelParentMap.next; i++)
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
  int i;

  for (i = 0; i < topLevelParentMap.next; i++)
  {
    if (pWin == topLevelParentMap.elt[i].pWin)
    {
      topLevelParentMap.elt[i] = topLevelParentMap.elt[topLevelParentMap.next - 1];
      topLevelParentMap.next--;

      return;
    }
  }
}

Window nxagentRootlessWMTopLevelWindow(WindowPtr pWin);

void nxagentConfigureRootlessWindow(WindowPtr pWin, int x, int y, int w, int h, int bw,
                                        WindowPtr pSib, int stack_mode, Mask mask)
{
  XWindowChanges changes;
  Window sibw = 0;

  changes.x = x;
  changes.y = y;
  changes.width = w;
  changes.height = h;
  changes.border_width = bw;
  changes.stack_mode = stack_mode;

  if (pSib)
  {
    sibw = nxagentWindow(pSib);
  }

  if (sibw)
  {
    changes.sibling = sibw;
  }

  XConfigureWindow(nxagentDisplay, nxagentWindow(pWin), mask, &changes);
}

void nxagentCirculateRootlessWindows(int direction)
{
  XCirculateSubwindows(nxagentDisplay, DefaultRootWindow(nxagentDisplay), direction);
}

#ifdef DEBUG

Bool nxagentRootlessTreesMatch()
{
  Window root_return;
  Window parent_return;
  Window *children_return;
  unsigned int nChildrenReturn;
  WindowPtr pW;
  WindowPtr pTestWin = WindowTable[0] -> firstChild;
  Bool treesMatch = True;
  Status result;

  result = XQueryTree(nxagentDisplay, DefaultRootWindow(nxagentDisplay),
                          &root_return, &parent_return, &children_return, &nChildrenReturn);

  if (!result)
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentRootlessTreesMatch: WARNING! Failed QueryTree request.\n");
    #endif

    return False;
  }

  while (nChildrenReturn > 0)
  {
    pW = nxagentWindowPtr(children_return[--nChildrenReturn]);

    if (!pW)
    {
      pW = nxagentRootlessTopLevelWindow(children_return[nChildrenReturn]);
    }

    if (pW && pW != WindowTable[0])
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

  if (children_return)
  {
    XFree(children_return);
  }

  return treesMatch;
}

#endif

#ifndef _XSERVER64
void nxagentRootlessRestack(Window children[], unsigned int nchildren)
#else
void nxagentRootlessRestack(unsigned long children[], unsigned int nchildren)
#endif
{
  WindowPtr *toplevel;
  unsigned int ntoplevel;
  int i;
  WindowPtr pWin;
  ClientPtr pClient;
  XID values[2];
  Mask mask;

  toplevel = xalloc(sizeof(WindowPtr) * nchildren);
  ntoplevel = 0;

  for(i = 0; i < nchildren; i++)
  {
    pWin = nxagentWindowPtr(children[i]);

    if (!pWin)
    {
      pWin = nxagentRootlessTopLevelWindow(children[i]);
    }

    if (pWin && pWin != WindowTable[0])
    {
      toplevel[ntoplevel++] = pWin;
    }
  }

  if (!ntoplevel)
  {
    return;
  }

  #ifdef DEBUG

  fprintf(stderr, "nxagentRootlessRestack: External top level windows before restack:");

  for (i = 0; i < ntoplevel; i++)
  {
    fprintf(stderr, "[%p]\n", toplevel[i]);
  }

  fprintf(stderr, "nxagentRootlessRestack: Internal top level windows before restack:");

  for (pWin = WindowTable[0] -> firstChild; pWin != NULL; pWin = pWin -> nextSib)
  {
    fprintf(stderr, "[%p]\n", pWin);
  }

  #endif

  pWin = WindowTable[0] -> firstChild;

  values[1] = (XID) Above;

  while(ntoplevel-- > 0 && pWin != NULL)
  {
    if (toplevel[ntoplevel] != pWin)
    {
      mask = CWSibling | CWStackMode;
      values[0] = pWin -> drawable.id;
      pClient = wClient(toplevel[ntoplevel]);
      nxagentScreenTrap = 1;
      ConfigureWindow(toplevel[ntoplevel], mask, (XID *) values, pClient);
      nxagentScreenTrap = 0;

      #ifdef TEST
      fprintf(stderr, "nxagentRootlessRestack: Restacked window [%p].\n", (void*) toplevel[ntoplevel]);
      #endif
    }

    pWin = toplevel[ntoplevel] -> nextSib;
  }

  #ifdef DEBUG

  fprintf(stderr, "nxagentRootlessRestack: External top level windows after restack:");

  ntoplevel = i;

  for (i = 0; i < ntoplevel; i++)
  {
    fprintf(stderr, "[%p]\n", toplevel[i]);
  }

  fprintf(stderr, "nxagentRootlessRestack: Internal top level windows after restack:");

  for (pWin = WindowTable[0] -> firstChild; pWin != NULL; pWin = pWin -> nextSib)
  {
    fprintf(stderr, "[%p]\n", pWin);
  }

  #endif

  xfree(toplevel);

  return;
}

/*
 * Determine if window is a top-level window.
 */

Window nxagentRootlessWindowParent(WindowPtr pWin)
{
  #ifdef TEST
  fprintf(stderr, "nxagentRootlessWindowParent: Called for window at [%p][%ld] with parent [%p][%ld].\n",
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

int nxagentExportAllProperty(pWin)
  WindowPtr pWin;
{
  PropertyPtr pProp;
  int total = 0;

  for (pProp = wUserProps(pWin); pProp; pProp = pProp->next)
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

int nxagentExportProperty(pWin, property, type, format, mode, nUnits, value)
    WindowPtr   pWin;
    Atom        property, type;
    int         format, mode;
    unsigned long nUnits;
    pointer     value;
{
  char *propertyS, *typeS;
  Atom propertyX, typeX;
  char *output = NULL;
  nxagentWMHints wmHints;
  nxagentPropWMHints propHints;
  Bool export = False;
  Bool freeMem = False;

  if (NXDisplayError(nxagentDisplay) == 1)
  {
    return 0;
  }

  propertyS = NameForAtom(property);
  typeS = NameForAtom(type);

  if (strncmp(propertyS, "WM_", 3) != 0 &&
          strncmp(propertyS, "_NET_", 5) != 0 &&
              strcmp(propertyS, "_KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR") != 0)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentExportProperty: WARNING! Ignored ChangeProperty "
                "on %swindow %lx property %s type %s nUnits %ld format %d\n",
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
    int *input = value;
    int i;

    if (buffer)
    {
      freeMem = True;
      export = True;
      output = (char*) buffer;

      for (i = 0; i < nUnits; i++)
      {
        buffer[i] = input[i];
      }
    }
  }
  #endif
  else if (strcmp(typeS, "WM_HINTS") == 0)
  {
    ClientPtr pClient = wClient(pWin);
    wmHints = *(nxagentWMHints*)value;

    wmHints.flags |= InputHint;
    wmHints.input = True;

    /*
     * Initialize the structure used in XChangeProperty().
     */

    propHints.flags = wmHints.flags;
    propHints.input = (wmHints.input == True ? 1 : 0);
    propHints.initialState = wmHints.initial_state;
    propHints.iconPixmap = wmHints.icon_pixmap;
    propHints.iconWindow = wmHints.icon_window;
    propHints.iconX = wmHints.icon_x;
    propHints.iconY = wmHints.icon_y;
    propHints.iconMask = wmHints.icon_mask;
    propHints.windowGroup = wmHints.window_group;

    output = (char*) &propHints;
    export = True;

    if ((wmHints.flags & IconPixmapHint) && (wmHints.icon_pixmap != None))
    {
      PixmapPtr icon = (PixmapPtr)SecurityLookupIDByType(pClient, wmHints.icon_pixmap,
                                                             RT_PIXMAP, SecurityDestroyAccess);

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
        fprintf(stderr, "nxagentExportProperty: WARNING! Failed to look up icon pixmap %x from hint "
                    "exporting property %s type %s on window %p.\n",
                        (unsigned int) wmHints.icon_pixmap, propertyS, typeS,
                            (void *) pWin);
        #endif
      }
    }

    if ((wmHints.flags & IconWindowHint) && (wmHints.icon_window != None))
    {
      WindowPtr icon = (WindowPtr)SecurityLookupWindow(wmHints.icon_window, pClient,
                                                  SecurityDestroyAccess);

      if (icon)
      {
        propHints.iconWindow = nxagentWindow(icon);
      }
      else
      {
        propHints.flags &= ~IconWindowHint;

        #ifdef WARNING
        fprintf(stderr, "nxagentExportProperty: WARNING! Failed to look up icon window %x from hint "
                    "exporting property %s type %s on window %p.\n",
                        (unsigned int) wmHints.icon_window, propertyS, typeS,
                            (void *) pWin);
        #endif
      }
    }

    if ((wmHints.flags & IconMaskHint) && (wmHints.icon_mask != None))
    {
      PixmapPtr icon = (PixmapPtr)SecurityLookupIDByType(pClient, wmHints.icon_mask,
                                                             RT_PIXMAP, SecurityDestroyAccess);

      if (icon)
      {
        propHints.iconMask = nxagentPixmap(icon);
      }
      else
      {
        propHints.flags &= ~IconMaskHint;

        #ifdef WARNING
        fprintf(stderr, "nxagentExportProperty: WARNING! Failed to look up icon mask %x from hint "
                    "exporting property %s type %s on window %p.\n",
                        (unsigned int) wmHints.icon_mask, propertyS, typeS,
                            (void *) pWin);
        #endif
      }
    }

    if ((wmHints.flags & WindowGroupHint) && (wmHints.window_group != None))
    {
      WindowPtr window = (WindowPtr)SecurityLookupWindow(wmHints.window_group, pClient,
                                                  SecurityDestroyAccess);

      if (window)
      {
        propHints.windowGroup = nxagentWindow(window);
      }
      else
      {
        propHints.flags &= ~WindowGroupHint;

        #ifdef WARNING
        fprintf(stderr, "nxagentExportProperty: WARNING! Failed to look up window group %x from hint "
                    "exporting property %s type %s on window %p.\n",
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
    char *atomName = NULL;
    int i;
    int j = 0;

    freeMem = True;
    export = True;
    output = (char *) atoms;

    for (i = 0; i < nUnits; i++)
    {
      /*
       * Exporting the _NET_WM_PING property could
       * result in rootless windows being grayed out
       * when the compiz window manager is running.
       *
       * Better solution would probably be to handle
       * the communication with the window manager
       * instead of just getting rid of the property.
       */

      if ((atomName = NameForAtom(input[i])) != NULL &&
              strcmp(atomName, "_NET_WM_PING") != 0)
      {
        atoms[j] = nxagentLocalToRemoteAtom(input[i]);

        if (atoms[j] == None)
        {
          #ifdef WARNING
          fprintf(stderr, "nxagentExportProperty: WARNING! Failed to convert local atom %ld [%s].\n",
                      (long int) input[i], validateString(atomName));
          #endif
        }

        j++;
      }
      #ifdef TEST
      else
      {
        fprintf(stderr, "nxagentExportProperty: WARNING! "
                    "Not exporting the _NET_WM_PING property.\n");
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
    WindowPtr pWindow;
    int i;

    freeMem = True;
    export = True;
    output = (char*) wind;

    for (i = 0; i < nUnits; i++)
    {
      pWindow = (WindowPtr)SecurityLookupWindow(input[i], pClient,
                                                    SecurityDestroyAccess);
      if ((input[i] != None) && pWindow)
      {
        wind[i] = nxagentWindow(pWindow);
      }
      else
      {
        #ifdef WARNING
        fprintf(stderr, "nxagentExportProperty: WARNING! Failed to look up window %ld "
                    "exporting property %s type %s on window %p.\n",
                        (long int) input[i], propertyS, typeS, (void *) pWin);
        #endif

        /*
         * It seems that clients specifie
         * strange windows, perhaps are
         * not real windows so we can try
         * to let them pass anyway.
         *
         * wind[i] = None;
         *
         */
      }
    }
  }

  if (export)
  {
    propertyX = nxagentLocalToRemoteAtom(property);
    typeX = nxagentLocalToRemoteAtom(type);

    if (propertyX == None || typeX == None)
    {
      #ifdef WARNING
      fprintf(stderr, "nxagentExportProperty: WARNING! Failed to convert local atom.\n");
      #endif

      export = 0;
    }
    else
    {
      #ifdef TEST
      fprintf(stderr, "nxagentExportProperty: Property [%lu] format [%i] "
                  "units [%lu].\n", propertyX, format, nUnits);
      #endif

      if ((format >> 3) * nUnits + sizeof(xChangePropertyReq) <
              (MAX_REQUEST_SIZE << 2))
      {
        XChangeProperty(nxagentDisplay, nxagentWindow(pWin), propertyX, typeX,
                            format, mode, (void*)output, nUnits);
      }
      else if (mode == PropModeReplace)
      {
        int n;
        char *data;

        XDeleteProperty(nxagentDisplay, nxagentWindow(pWin), propertyX);

        data = (char *) output;

        while (nUnits > 0)
        {
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
        fprintf(stderr, "nxagentExportProperty: WARNING! "
                    "Property [%lu] too long.\n", propertyX);
        #endif

        goto nxagentExportPropertyError;
      }

      nxagentAddPropertyToList(propertyX, pWin);
    }
  }
  else
  {
    #ifdef TEST
    fprintf(stderr, "nxagentExportProperty: WARNING! Ignored ChangeProperty "
                "on %swindow %x property %s type %s nUnits %ld format %d\n",
                    nxagentWindowTopLevel(pWin) ? "toplevel " : "",
                        nxagentWindow(pWin), validateString(propertyS), validateString(typeS),
                            nUnits, format);
    #endif
  }

  nxagentExportPropertyError:

  if (freeMem)
  {
    xfree(output);
  }

  return export;
}

void nxagentImportProperty(Window window,
                           Atom property,
                           Atom type,
                           int format,
                           unsigned long nitems,
                           unsigned long bytes_after,
                           unsigned char *buffer)
{
  Atom propertyL;
  Atom typeL;

  WindowPtr pWin;
  Bool import = False;
  Bool freeMem = False;
  nxagentWMHints wmHints;

  typedef struct {
      CARD32 state;
      Window icon;
    } WMState;
  WMState wmState;

  char *output = NULL;
  char *typeS;

  pWin = nxagentWindowPtr(window);

  if (pWin == NULL)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentImportProperty: Failed to look up remote window %lx  property [%ld] exiting.\n",
                window, property);
    #endif

    return;
  }

  propertyL = nxagentRemoteToLocalAtom(property);

  if (!ValidAtom(propertyL))
  {
    #ifdef TEST
    fprintf(stderr, "nxagentImportProperty: Failed to convert remote property atom.\n");
    #endif

    return;
  }

  #ifdef TEST
  fprintf(stderr, "nxagentImportProperty: Window %lx property [%ld]: %s\n",
              window, property, validateString(NameForAtom(propertyL)));
  #endif

  /*
   * We settle a property size limit of
   * 256K beyond which we simply ignore them.
   */

  typeL = nxagentRemoteToLocalAtom(type);
  typeS = NameForAtom(typeL);

  if (buffer == NULL && (nitems > 0))
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentImportProperty: Failed to retrieve remote property [%ld] %s on Window %ld\n",
                (long int) property, validateString(NameForAtom(propertyL)), (long int) window);
    #endif
  }
  else if (bytes_after != 0)
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentImportProperty: Remote property bigger than maximum limits.\n");
    #endif
  }
  else if (!ValidAtom(typeL))
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentImportProperty: Failed to convert remote atoms [%ld].\n",
                (long int) type);
    #endif
  }
  else if (nitems == 0)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentImportProperty: Importing void property.\n");
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
     * Contents of property of type WM_STATE
     * are {CARD32 state, WINDOW icon}. Only
     * the icon field has to be modified before
     * importing the property.
     */

    WindowPtr pIcon;

    wmState = *(WMState*)buffer;
    pIcon = nxagentWindowPtr(wmState.icon);

    if (pIcon || wmState.icon == None)
    {
      import = True;
      output = (char*) &wmState;
      wmState.icon = pIcon ? nxagentWindow(pIcon) : None;
    }
    else if (wmState.icon)
    {
      #ifdef WARNING
      fprintf(stderr, "nxagentImportProperty: WARNING! Failed to convert remote window %ld"
                  " importing property %ld of type WM_STATE", (long int) wmState.icon,
                      (long int) property);
      #endif
    }
  }
  else if (strcmp(typeS, "WM_HINTS") == 0)
  {
    wmHints = *(nxagentWMHints*)buffer;
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
        fprintf(stderr, "nxagentImportProperty: WARNING! Failed to look up remote icon "
                    "pixmap %d from hint importing property [%ld] type %s on window %p.\n",
                        (unsigned int) wmHints.icon_pixmap, (long int) property,
                            typeS, (void *) pWin);
        #endif
      }
    }

    if ((wmHints.flags & IconWindowHint) && (wmHints.icon_window =! None))
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
        fprintf(stderr, "nxagenImportProperty: WARNING! Failed to look up remote icon "
                    "window %x from hint importing property [%ld] type %s on window %p.\n",
                         (unsigned int) wmHints.icon_window,
                             (long int) property, typeS, (void *) pWin);
        #endif
      }
    }

    if ((wmHints.flags & IconMaskHint) && (wmHints.icon_mask =! None))
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
        fprintf(stderr, "nxagentImportProperty: WARNING! Failed to look up remote icon "
                    "mask %x from hint importing property [%ld] type %s on window %p.\n",
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
        fprintf(stderr, "nxagentImportProperty: WARNING! Failed to look up remote window "
                    "group %x from hint importing property [%ld] type %s on window %p.\n",
                          (unsigned int) wmHints.window_group,
                              (long int) property, typeS, (void *) pWin);
        #endif
      }
    }
  }
  else if (strcmp(typeS, "ATOM") == 0)
  {
    Atom *atoms = malloc(nitems * sizeof(Atom));
    Atom *input = (Atom*) buffer;
    int i;

    if (atoms == NULL)
    {
      #ifdef WARNING
      fprintf(stderr, "nxagentImportProperty: WARNING! Malloc failed bailing out.\n");
      #endif

      return;
    }

    freeMem = True;
    import = True;
    output = (char *) atoms;

    for (i = 0; i < nitems; i++)
    {
      atoms[i] = nxagentRemoteToLocalAtom(input[i]);

      if (atoms[i] == None)
      {
        #ifdef WARNING
        fprintf(stderr, "nxagentImportProperty: WARNING! Failed to convert remote atom %ld.\n",
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
    int i;

    freeMem = True;
    import = True;
    output = (char*) wind;

    for (i = 0; i < nitems; i++)
    {
      pWindow = nxagentWindowPtr(input[i]);

      if (pWindow)
      {
        wind[i] = pWindow -> drawable.id;
      }
      else
      {
        #ifdef WARNING
        fprintf(stderr, "nxagentImportProperty: WARNING! Failed to look up remote window %lx "
                    "importing property [%ld] type %s on window %p.\n",
                        (long int) input[i], (long int) property, typeS, (void*)pWin);
        #endif

        wind[i] = None;
      }
    }
  }

  if (import)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentImportProperty: ChangeProperty "
                "on window %lx property [%ld] type %s nitems %ld format %d\n",
                    window, property, typeS, nitems, format);
    #endif

    ChangeWindowProperty(pWin, propertyL, typeL, format,
                             PropModeReplace, nitems, output, 1);
  }
  else
  {
    #ifdef TEST
    fprintf(stderr, "nxagentImportProperty: WARNING! Ignored ChangeProperty "
                "on window %lx property [%ld] type %s ntems %ld format %d\n",
                       window, property, validateString(typeS), nitems, format);
    #endif
  }

  if (freeMem)
  {
    xfree(output);
  }

  return;
}

/*
 * We want to import all properties changed by external clients to
 * reflect properties of our internal windows but we must ignore
 * all the property notify events generated by our own requests.
 * For this purpose we implement a FIFO to record every change pro-
 * perty request that we dispatch. In this way, when processing a
 * property notify, we can distinguish between the notifications
 * generated by our requests from those generated by other clients
 * connected to the real X server.
 */

struct nxagentPropertyRec{
  Window window;
  Atom property;
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

void nxagentRemovePropertyFromList()
{
  struct nxagentPropertyRec *tmp = nxagentPropertyList.first;

  #ifdef TEST
  fprintf(stderr, "nxagentRemovePropertyFromList: Property %ld on Window %lx to list, list size is %d.\n\n",
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

    xfree(tmp);
  }
}

/*
 * Add the record to the list.
 */

void nxagentAddPropertyToList(Atom property, WindowPtr pWin)
{
  struct nxagentPropertyRec *tmp;

  if (NXDisplayError(nxagentDisplay) == 1)
  {
    return;
  }

  if ((tmp = malloc(sizeof(struct nxagentPropertyRec))) == NULL)
  {
    FatalError("nxagentAddPropertyToList: malloc failed.");
  }

  #ifdef TEST
  fprintf(stderr, "nxagentAddPropertyToList: Adding record Property %ld - Window %lx[%p]"
             "to list, list size is %d.\n", property, nxagentWindow(pWin), (void*) pWin,
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

void nxagentFreePropertyList()
{
  while (nxagentPropertyList.size != 0)
  {
    nxagentRemovePropertyFromList();
  }
}

/*
 * We are trying to distinguish notify generated by
 * an external client from those genarated by our
 * own requests.
 */

Bool nxagentNotifyMatchChangeProperty(void *p)
{
  struct nxagentPropertyRec *first = nxagentPropertyList.first;
  XPropertyEvent *X = p;

  #ifdef TEST
  fprintf(stderr, "nxagentNotifyMatchChangeProperty: Property notify on window %lx property %ld.\n",
              X -> window, X -> atom);

  if (first)
  {
    fprintf(stderr, "nxagentNotifyMatchChangeProperty: First element on list is window %lx property %ld list size is %d.\n",
                first -> window, first -> property, nxagentPropertyList.size);
  }
  else
  {
    fprintf(stderr, "nxagentNotifyMatchChangeProperty: List is empty.\n");
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

