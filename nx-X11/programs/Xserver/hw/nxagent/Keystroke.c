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
#include "keysym.h"

#include "screenint.h"
#include "scrnintstr.h"

#include "Agent.h"
#include "Display.h"
#include "Events.h"
#include "Options.h"
#include "Keystroke.h"
#include "Drawable.h"
#include "Init.h" /* extern int nxagentX2go */

#include <unistd.h>

#include <libxml/parser.h>
#include <libxml/tree.h>

extern Bool nxagentWMIsRunning;
extern Bool nxagentIpaq;
extern char *nxagentKeystrokeFile;
Bool nxagentKeystrokeFileParsed = False;

#ifdef NX_DEBUG_INPUT
int nxagentDebugInputDevices = 0;
unsigned long nxagentLastInputDevicesDumpTime = 0;
extern void nxagentDeactivateInputDevicesGrabs();
#endif

/*
 * Set here the required log level.
 */

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG
#undef  DUMP


/* this table is used to parse actions given on the command line or in the
 * config file, therefore indices have to match the enum in Keystroke.h */
char * nxagentSpecialKeystrokeNames[] = {
       "end_marker",
       "close_session",
       "switch_all_screens",
       "minimize",
       "left",
       "up",
       "right",
       "down",
       "resize",
       "defer",
       "ignore",
       "force_synchronization",

       "debug_tree",
       "regions_on_screen",
       "test_input",
       "deactivate_input_devices_grab",

       "fullscreen",
       "viewport_move_left",
       "viewport_move_up",
       "viewport_move_right",
       "viewport_move_down",
       NULL,
};

struct nxagentSpecialKeystrokeMap default_map[] = {
  /* stroke, modifierMask, modifierAltMeta, keysym */
  {KEYSTROKE_DEBUG_TREE, ControlMask, 1, XK_q},
  {KEYSTROKE_DEBUG_TREE, ControlMask, 1, XK_Q},
  {KEYSTROKE_CLOSE_SESSION, ControlMask, 1, XK_t},
  {KEYSTROKE_CLOSE_SESSION, ControlMask, 1, XK_T},
  {KEYSTROKE_SWITCH_ALL_SCREENS, ControlMask, 1, XK_f},
  {KEYSTROKE_SWITCH_ALL_SCREENS, ControlMask, 1, XK_F},
  {KEYSTROKE_MINIMIZE, ControlMask, 1, XK_m},
  {KEYSTROKE_MINIMIZE, ControlMask, 1, XK_M},
  {KEYSTROKE_LEFT, ControlMask, 1, XK_Left},
  {KEYSTROKE_LEFT, ControlMask, 1, XK_KP_Left},
  {KEYSTROKE_UP, ControlMask, 1, XK_Up},
  {KEYSTROKE_UP, ControlMask, 1, XK_KP_Up},
  {KEYSTROKE_RIGHT, ControlMask, 1, XK_Right},
  {KEYSTROKE_RIGHT, ControlMask, 1, XK_KP_Right},
  {KEYSTROKE_DOWN, ControlMask, 1, XK_Down},
  {KEYSTROKE_DOWN, ControlMask, 1, XK_KP_Down},
  {KEYSTROKE_RESIZE, ControlMask, 1, XK_r},
  {KEYSTROKE_RESIZE, ControlMask, 1, XK_R},
  {KEYSTROKE_DEFER, ControlMask, 1, XK_e},
  {KEYSTROKE_DEFER, ControlMask, 1, XK_E},
  {KEYSTROKE_IGNORE, ControlMask, 1, XK_BackSpace},
  {KEYSTROKE_IGNORE, 0, 0, XK_Terminate_Server},
  {KEYSTROKE_FORCE_SYNCHRONIZATION, ControlMask, 1, XK_j},
  {KEYSTROKE_FORCE_SYNCHRONIZATION, ControlMask, 1, XK_J},
  {KEYSTROKE_REGIONS_ON_SCREEN, ControlMask, 1, XK_a},
  {KEYSTROKE_REGIONS_ON_SCREEN, ControlMask, 1, XK_A},
  {KEYSTROKE_TEST_INPUT, ControlMask, 1, XK_x},
  {KEYSTROKE_TEST_INPUT, ControlMask, 1, XK_X},
  {KEYSTROKE_DEACTIVATE_INPUT_DEVICES_GRAB, ControlMask, 1, XK_y},
  {KEYSTROKE_DEACTIVATE_INPUT_DEVICES_GRAB, ControlMask, 1, XK_Y},
  {KEYSTROKE_FULLSCREEN, ControlMask | ShiftMask, 1, XK_f},
  {KEYSTROKE_FULLSCREEN, ControlMask | ShiftMask, 1, XK_F},
  {KEYSTROKE_VIEWPORT_MOVE_LEFT, ControlMask | ShiftMask, 1, XK_Left},
  {KEYSTROKE_VIEWPORT_MOVE_LEFT, ControlMask | ShiftMask, 1, XK_KP_Left},
  {KEYSTROKE_VIEWPORT_MOVE_UP, ControlMask | ShiftMask, 1, XK_Up},
  {KEYSTROKE_VIEWPORT_MOVE_UP, ControlMask | ShiftMask, 1, XK_KP_Up},
  {KEYSTROKE_VIEWPORT_MOVE_RIGHT, ControlMask | ShiftMask, 1, XK_Right},
  {KEYSTROKE_VIEWPORT_MOVE_RIGHT, ControlMask | ShiftMask, 1, XK_KP_Right},
  {KEYSTROKE_VIEWPORT_MOVE_DOWN, ControlMask | ShiftMask, 1, XK_Down},
  {KEYSTROKE_VIEWPORT_MOVE_DOWN, ControlMask | ShiftMask, 1, XK_KP_Down},
  {KEYSTROKE_END_MARKER, 0, 0, 0},
};
struct nxagentSpecialKeystrokeMap *map = default_map;

static Bool modifier_matches(unsigned int mask, int compare_alt_meta, unsigned int state)
{
  /* nxagentAltMetaMask needs special handling
   * it seems to me its an and-ed mask of all possible meta and alt keys
   * somehow...
   *
   * otherwise this function would be just a simple bitop
   */
  Bool ret = True;

  if (compare_alt_meta) {
    if (! (state & nxagentAltMetaMask)) {
      ret = False;
    }

    mask &= ~nxagentAltMetaMask;
  }

  /* all modifiers except meta/alt have to match exactly, extra bits are evil */
  if ((mask & state) != mask) {
    ret = False;
  }

  return ret;
}

static int read_binding_from_xmlnode(xmlNode *node, struct nxagentSpecialKeystrokeMap *ret)
{
  struct nxagentSpecialKeystrokeMap newkm = {
    .stroke = 0,
    .modifierMask = 0,
    .modifierAltMeta = 0,
    .keysym = NoSymbol
  };
  xmlAttr *attr;

  for (attr = node->properties; attr; attr = attr->next)
  {
    /* ignore attributes without data (which should never happen anyways) */
    if (attr->children->content == NULL)
    {
      #ifdef DEBUG
      char *aname = (attr->name)?((char *)attr->name):"unknown";
      fprintf(stderr, "attribute %s with NULL value", aname);
      #endif
      continue;
    }
    if (strcmp((char *)attr->name, "action") == 0)
    {
      for (int i = 0; nxagentSpecialKeystrokeNames[i] != NULL; i++)
      {
        if (strcmp(nxagentSpecialKeystrokeNames[i],(char *)attr->children->content) == 0)
        {
          /* this relies on the values of enum nxagentSpecialKeystroke and the
           * indices of nxagentSpecialKeystrokeNames being in sync */
          newkm.stroke = i;
          break;
        }
      }
      continue;
    }
    else if (strcmp((char *)attr->name, "key") == 0)
    {
      if (strcmp((char *)attr->children->content, "0") != 0 && strcmp((char *)attr->children->content, "false") != 0)
	newkm.keysym = XStringToKeysym((char *)attr->children->content);

      continue;
    }

    /* ignore attributes with value="0" or "false", everything else is interpreted as true */
    if (strcmp((char *)attr->children->content, "0") == 0 || strcmp((char *)attr->children->content, "false") == 0)
      continue;

         if (strcmp((char *)attr->name, "Mod1") == 0)    { newkm.modifierMask |= Mod1Mask; }
    else if (strcmp((char *)attr->name, "Mod2") == 0)    { newkm.modifierMask |= Mod2Mask; }
    else if (strcmp((char *)attr->name, "Mod3") == 0)    { newkm.modifierMask |= Mod3Mask; }
    else if (strcmp((char *)attr->name, "Mod4") == 0)    { newkm.modifierMask |= Mod4Mask; }
    else if (strcmp((char *)attr->name, "Control") == 0) { newkm.modifierMask |= ControlMask; }
    else if (strcmp((char *)attr->name, "Shift") == 0)   { newkm.modifierMask |= ShiftMask; }
    else if (strcmp((char *)attr->name, "Lock") == 0)    { newkm.modifierMask |= LockMask;  }
    else if (strcmp((char *)attr->name, "AltMeta") == 0) { newkm.modifierAltMeta = 1; }
  }

  if (newkm.stroke != 0 && newkm.keysym != NoSymbol)
  {
    /* keysym and stroke are required, everything else is optional */
    memcpy(ret, &newkm, sizeof(struct nxagentSpecialKeystrokeMap));
    return True;
  }
  else
    return False;
}

/*
 * searches a keystroke xml file
 *
 * search order:
 *  - '-keystrokefile' commandline parameter
 *  - $NXAGENT_KEYSTROKEFILE environment variable
 *  - $HOME/.nx/config/keystrokes.cfg
 *  - /etc/nxagent/keystrokes.cfg
 *  - hardcoded traditional NX default settings
 */
static void parse_keystroke_file(void)
{
  char *filename = NULL;

  char *homefile = "/.nx/config/keystrokes.cfg";
  char *etcfile = "/etc/nxagent/keystrokes.cfg";

  if (nxagentX2go) {
    homefile = "/.x2go/config/keystrokes.cfg";
    etcfile = "/etc/x2go/keystrokes.cfg";
  }

  if (nxagentKeystrokeFile != NULL && access(nxagentKeystrokeFile, R_OK) == 0)
  {
    filename = strdup(nxagentKeystrokeFile);
    if (filename == NULL)
    {
      fprintf(stderr, "malloc failed");
      exit(EXIT_FAILURE);
    }
  }
  else if ((filename = getenv("NXAGENT_KEYSTROKEFILE")) != NULL && access(filename, R_OK) == 0)
  {
    filename = strdup(filename);
    if (filename == NULL)
    {
      fprintf(stderr, "malloc failed");
      exit(EXIT_FAILURE);
    }
  }
  else
  {
    char *homedir = getenv("HOME");
    filename = NULL;
    if (homedir != NULL)
    {
      homedir = strdup(homedir);
      if (homedir == NULL)
      {
        fprintf(stderr, "malloc failed");
          exit(EXIT_FAILURE);
      }
      filename = calloc(1, strlen(homefile) + strlen(homedir) + 1);
      if (filename == NULL)
      {
        fprintf(stderr, "malloc failed");
        exit(EXIT_FAILURE);
      }
      strcpy(filename, homedir);
      strcpy(filename + strlen(homedir), homefile);
      if (homedir)
      {
        free(homedir);
      }
    }

    if (access(filename, R_OK) == 0)
    {
      /* empty */
    }
    else if (access(etcfile, R_OK) == 0)
    {
      if (filename)
        free(filename);
      filename = strdup(etcfile);
      if (filename == NULL)
      {
        fprintf(stderr, "malloc failed");
        exit(EXIT_FAILURE);
      }
    }
    else
    {
      if (filename)
        free(filename);
      filename = NULL;
    }
  }

  /* now we know which file to read, if any */
  if (filename)
  {
    xmlDoc *doc = NULL;
    xmlNode *root = NULL;
    LIBXML_TEST_VERSION
    doc = xmlReadFile(filename, NULL, 0);
    if (doc != NULL)
    {
      xmlNode *cur = NULL;
      root = xmlDocGetRootElement(doc);

      for (cur = root; cur; cur = cur->next)
      {
        if (cur->type == XML_ELEMENT_NODE && strcmp((char *)cur->name, "keystrokes") == 0)
        {
          xmlNode *bindings = NULL;
          int num = 0;
          int idx = 0;

          for (bindings = cur->children; bindings; bindings = bindings->next)
          {
            if (bindings->type == XML_ELEMENT_NODE && strcmp((char *)bindings->name, "keystroke") == 0)
            {
              num++;
            }
          }
          map = calloc((num + 1), sizeof(struct nxagentSpecialKeystrokeMap));
          if (map == NULL)
          {
            fprintf(stderr, "calloc failed");
            exit(EXIT_FAILURE);
          }

          for (bindings = cur->children; bindings; bindings = bindings->next)
          {
            if (bindings->type == XML_ELEMENT_NODE &&
                strcmp((char *)bindings->name, "keystroke") == 0 &&
                read_binding_from_xmlnode(bindings, &(map[idx])))
                  idx++;
          }

          map[idx].stroke = KEYSTROKE_END_MARKER;
        }
      }

      xmlFreeDoc(doc);
      xmlCleanupParser();
    }
    else
    {
      #ifdef DEBUG
      fprintf("XML parsing for %s failed\n", filename);
      #endif
    }
    free(filename);
  }
}

static enum nxagentSpecialKeystroke find_keystroke(XKeyEvent *X)
{
  int keysyms_per_keycode_return;
  XlibKeySym *keysym = XGetKeyboardMapping(nxagentDisplay,
                                           X->keycode,
                                           1,
                                           &keysyms_per_keycode_return);

  struct nxagentSpecialKeystrokeMap *cur = map;

  if (! nxagentKeystrokeFileParsed)
  {
    parse_keystroke_file();
    nxagentKeystrokeFileParsed = True;
  }

  enum nxagentSpecialKeystroke ret = KEYSTROKE_NOTHING;

  while (cur->stroke != KEYSTROKE_END_MARKER) {
    if (cur->keysym == keysym[0] && modifier_matches(cur->modifierMask, cur->modifierAltMeta, X->state)) {

      free(keysym);
      return cur->stroke;
    }
    cur++;
  }

  free(keysym);
  return ret;
}

int nxagentCheckSpecialKeystroke(XKeyEvent *X, enum HandleEventResult *result)
{
  enum nxagentSpecialKeystroke stroke = find_keystroke(X);

  *result = doNothing;

  /*
   * I don't know how much hard work is doing this operation.
   * Do we need a cache ?
   */

  int keysyms_per_keycode_return;
  XlibKeySym *sym = XGetKeyboardMapping(nxagentDisplay,
                                        X->keycode,
                                        1,
                                        &keysyms_per_keycode_return);

  if (sym[0] == XK_VoidSymbol || sym[0] == NoSymbol)
  {
    free(sym);
    return 0;
  }

  #ifdef TEST
  fprintf(stderr, "nxagentCheckSpecialKeystroke: got code %x - state %x - sym %lx\n",
              X -> keycode, X -> state, sym[0]);
  #endif
  free(sym);

  /*
   * Check special keys.
   */

  /*
   * FIXME: We should use the keysym instead that the keycode
   *        here.
   */

  if (X -> keycode == 130 && nxagentIpaq)
  {
    *result = doStartKbd;

    return 1;
  }

  switch (stroke) {
    case KEYSTROKE_DEBUG_TREE:
      #ifdef DEBUG_TREE
      *result = doDebugTree;
      #endif
      break;
    case KEYSTROKE_CLOSE_SESSION:
      *result = doCloseSession;
      break;
    case KEYSTROKE_SWITCH_ALL_SCREENS:
      if (nxagentOption(Rootless) == False) {
        *result = doSwitchAllScreens;
      }
      break;
    case KEYSTROKE_MINIMIZE:
      if (nxagentOption(Rootless) == False) {
        *result = doMinimize;
      }
      break;
    case KEYSTROKE_LEFT:
      if (nxagentOption(Rootless) == False &&
          nxagentOption(DesktopResize) == False) {
        *result = doViewportLeft;
      }
      break;
    case KEYSTROKE_UP:
      if (nxagentOption(Rootless) == False &&
          nxagentOption(DesktopResize) == False) {
        *result = doViewportUp;
      }
      break;
    case KEYSTROKE_RIGHT:
      if (nxagentOption(Rootless) == False &&
          nxagentOption(DesktopResize) == False) {
        *result = doViewportRight;
      }
      break;
    case KEYSTROKE_DOWN:
      if (nxagentOption(Rootless) == False &&
          nxagentOption(DesktopResize) == False) {
        *result = doViewportDown;
      }
      break;
    case KEYSTROKE_RESIZE:
      if (nxagentOption(Rootless) == False) {
        *result = doSwitchResizeMode;
      }
      break;
    case KEYSTROKE_DEFER:
      *result = doSwitchDeferMode;
      break;
    case KEYSTROKE_IGNORE:
      /* this is used e.g. to ignore C-A-Backspace aka XK_Terminate_Server */
      return 1;
      break;
    case KEYSTROKE_FORCE_SYNCHRONIZATION:
      nxagentForceSynchronization = 1;
      break;
    case KEYSTROKE_REGIONS_ON_SCREEN:
      #ifdef DUMP
      nxagentRegionsOnScreen();
      #endif
      break;
    case KEYSTROKE_TEST_INPUT:
      /*
       * Used to test the input devices state.
       */
      #ifdef NX_DEBUG_INPUT
      if (X -> type == KeyPress) {
        if (nxagentDebugInputDevices == 0) {
          fprintf(stderr, "Info: Turning input devices debug ON.\n");
          nxagentDebugInputDevices = 1;
        } else {
          fprintf(stderr, "Info: Turning input devices debug OFF.\n");
          nxagentDebugInputDevices = 0;
          nxagentLastInputDevicesDumpTime = 0;
        }
      }
      return 1;
      #endif
      break;
    case KEYSTROKE_DEACTIVATE_INPUT_DEVICES_GRAB:
      #ifdef NX_DEBUG_INPUT
      if (X->type == KeyPress) {
        nxagentDeactivateInputDevicesGrab();
      }
      return 1;
      #endif
      break;
    case KEYSTROKE_FULLSCREEN:
      if (nxagentOption(Rootless) == 0) {
        *result = doSwitchFullscreen;
      }
      break;
    case KEYSTROKE_VIEWPORT_MOVE_LEFT:
      if (nxagentOption(Rootless) == 0 &&
          nxagentOption(DesktopResize) == 0) {
        *result = doViewportMoveLeft;
      }
      break;
    case KEYSTROKE_VIEWPORT_MOVE_UP:
      if (nxagentOption(Rootless) == 0 &&
          nxagentOption(DesktopResize) == 0) {
        *result = doViewportMoveUp;
      }
      break;
    case KEYSTROKE_VIEWPORT_MOVE_RIGHT:
      if (nxagentOption(Rootless) == 0 &&
          nxagentOption(DesktopResize) == 0) {
        *result = doViewportMoveRight;
      }
      break;
    case KEYSTROKE_VIEWPORT_MOVE_DOWN:
      if (nxagentOption(Rootless) == 0 &&
          nxagentOption(DesktopResize) == 0) {
        *result = doViewportMoveDown;
      }
      break;
    case KEYSTROKE_NOTHING: /* do nothing. difference to KEYSTROKE_IGNORE is the return value */
    case KEYSTROKE_END_MARKER: /* just to make gcc STFU */
    case KEYSTROKE_MAX:
      break;
  }
  return (*result == doNothing) ? 0 : 1;
}
