/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2011 NoMachine (http://www.nomachine.com)          */
/* Copyright (c) 2008-2017 Oleksandr Shneyder <o.shneyder@phoca-gmbh.de>  */
/* Copyright (c) 2011-2022 Mike Gabriel <mike.gabriel@das-netzwerkteam.de>*/
/* Copyright (c) 2014-2019 Mihai Moldovan <ionic@ionic.de>                */
/* Copyright (c) 2014-2022 Ulrich Sibiller <uli42@gmx.de>                 */
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
#include "Keyboard.h"
#include "Drawable.h"
#include "Init.h" /* extern Bool nxagentX2go */
#include "Utils.h"

#include <unistd.h>

#include <libxml/parser.h>
#include <libxml/tree.h>

extern Bool nxagentWMIsRunning;
extern char *nxagentKeystrokeFile;

#ifdef NX_DEBUG_INPUT
int nxagentDebugInputDevices = False;
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

/* must be included _after_ DUMP */
#include "Keystroke.h"


/* this table is used to parse actions given on the command line or in the
 * config file, therefore indices have to match the enum in Keystroke.h */
char * nxagentSpecialKeystrokeNames[] = {
       "end_marker",
       "close_session",
       "switch_all_screens",
       "fullscreen",
       "minimize",
       "defer",
       "ignore",
       "force_synchronization",

#ifdef DEBUG_TREE
       "debug_tree",
#endif
#ifdef DUMP
       "regions_on_screen",
#endif
#ifdef NX_DEBUG_INPUT
       "test_input",
       "deactivate_input_devices_grab",
#endif
       "resize",
       "viewport_move_left",
       "viewport_move_up",
       "viewport_move_right",
       "viewport_move_down",
       "viewport_scroll_left",
       "viewport_scroll_up",
       "viewport_scroll_right",
       "viewport_scroll_down",

       "reread_keystrokes",

       "autograb",

       "dump_clipboard",

       NULL,
};

struct nxagentSpecialKeystrokeMap default_map[] = {
  /* stroke, modifierMask, modifierAltMeta, keysym */
#ifdef DEBUG_TREE
  {KEYSTROKE_DEBUG_TREE, ControlMask, True, XK_q},
#endif
  {KEYSTROKE_CLOSE_SESSION, ControlMask, True, XK_t},
  {KEYSTROKE_SWITCH_ALL_SCREENS, ControlMask, True, XK_f},
  {KEYSTROKE_FULLSCREEN, ControlMask | ShiftMask, True, XK_f},
  {KEYSTROKE_MINIMIZE, ControlMask, True, XK_m},
  {KEYSTROKE_DEFER, ControlMask, True, XK_e},
  {KEYSTROKE_FORCE_SYNCHRONIZATION, ControlMask, True, XK_j},
#ifdef DUMP
  {KEYSTROKE_REGIONS_ON_SCREEN, ControlMask, True, XK_a},
#endif
#ifdef NX_DEBUG_INPUT
  {KEYSTROKE_TEST_INPUT, ControlMask, True, XK_x},
  {KEYSTROKE_DEACTIVATE_INPUT_DEVICES_GRAB, ControlMask, True, XK_y},
#endif
  {KEYSTROKE_RESIZE, ControlMask, True, XK_r},
  {KEYSTROKE_VIEWPORT_MOVE_LEFT, ControlMask | ShiftMask, True, XK_Left},
  {KEYSTROKE_VIEWPORT_MOVE_LEFT, ControlMask | ShiftMask, True, XK_KP_Left},
  {KEYSTROKE_VIEWPORT_MOVE_UP, ControlMask | ShiftMask, True, XK_Up},
  {KEYSTROKE_VIEWPORT_MOVE_UP, ControlMask | ShiftMask, True, XK_KP_Up},
  {KEYSTROKE_VIEWPORT_MOVE_RIGHT, ControlMask | ShiftMask, True, XK_Right},
  {KEYSTROKE_VIEWPORT_MOVE_RIGHT, ControlMask | ShiftMask, True, XK_KP_Right},
  {KEYSTROKE_VIEWPORT_MOVE_DOWN, ControlMask | ShiftMask, True, XK_Down},
  {KEYSTROKE_VIEWPORT_MOVE_DOWN, ControlMask | ShiftMask, True, XK_KP_Down},
  {KEYSTROKE_VIEWPORT_SCROLL_LEFT, ControlMask, True, XK_Left},
  {KEYSTROKE_VIEWPORT_SCROLL_LEFT, ControlMask, True, XK_KP_Left},
  {KEYSTROKE_VIEWPORT_SCROLL_UP, ControlMask, True, XK_Up},
  {KEYSTROKE_VIEWPORT_SCROLL_UP, ControlMask, True, XK_KP_Up},
  {KEYSTROKE_VIEWPORT_SCROLL_RIGHT, ControlMask, True, XK_Right},
  {KEYSTROKE_VIEWPORT_SCROLL_RIGHT, ControlMask, True, XK_KP_Right},
  {KEYSTROKE_VIEWPORT_SCROLL_DOWN, ControlMask, True, XK_Down},
  {KEYSTROKE_VIEWPORT_SCROLL_DOWN, ControlMask, True, XK_KP_Down},
  {KEYSTROKE_REREAD_KEYSTROKES, ControlMask, True, XK_k},
  {KEYSTROKE_AUTOGRAB, ControlMask, True, XK_g},
  {KEYSTROKE_DUMP_CLIPBOARD, ControlMask | ShiftMask, True, XK_c},
  {KEYSTROKE_END_MARKER, 0, False, NoSymbol},
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
    state &= ~nxagentAltMetaMask;
  }

  /* ignore CapsLock and/or Numlock if the keystroke does not
     explicitly require them */

  if ( !(mask & nxagentCapsMask) )
    state &= ~nxagentCapsMask;

  if ( !(mask & nxagentNumlockMask) )
    state &= ~nxagentNumlockMask;

  /* all modifiers except meta/alt have to match exactly, extra bits are evil */
  if (mask != state) {
    ret = False;
  }

  return ret;
}

static Bool read_binding_from_xmlnode(xmlNode *node, struct nxagentSpecialKeystrokeMap *ret)
{
  /* init the struct to have proper values in case not all attributes are found */
  struct nxagentSpecialKeystrokeMap newkm = {
    .stroke = KEYSTROKE_NOTHING,
    .modifierMask = 0,
    .modifierAltMeta = False,
    .keysym = NoSymbol
  };

  for (xmlAttr *attr = node->properties; attr; attr = attr->next)
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
      newkm.stroke = KEYSTROKE_NOTHING;
      for (int i = 0; nxagentSpecialKeystrokeNames[i] != NULL; i++)
      {
        if (strcmp(nxagentSpecialKeystrokeNames[i], (char *)attr->children->content) == 0)
        {
          /* this relies on the values of enum nxagentSpecialKeystroke and the
           * indices of nxagentSpecialKeystrokeNames being in sync */
          newkm.stroke = i;
          break;
        }
      }
      if (newkm.stroke == KEYSTROKE_NOTHING)
        fprintf(stderr, "Info: ignoring unknown keystroke action '%s'.\n", (char *)attr->children->content);
      continue;
    }
    else if (strcmp((char *)attr->name, "key") == 0)
    {
      newkm.keysym = XStringToKeysym((char *)attr->children->content);
      continue;
    }
    else
    {
      /* ignore attributes with value="0" or "false", everything else is interpreted as true */
      if (strcmp((char *)attr->children->content, "0") == 0 || strcmp((char *)attr->children->content, "false") == 0)
        continue;

           if (strcmp((char *)attr->name, "Mod1") == 0)    { newkm.modifierMask |= Mod1Mask; }
      else if (strcmp((char *)attr->name, "Mod2") == 0)    { newkm.modifierMask |= Mod2Mask; }
      else if (strcmp((char *)attr->name, "Mod3") == 0)    { newkm.modifierMask |= Mod3Mask; }
      else if (strcmp((char *)attr->name, "Mod4") == 0)    { newkm.modifierMask |= Mod4Mask; }
      else if (strcmp((char *)attr->name, "Mod5") == 0)    { newkm.modifierMask |= Mod5Mask; }
      else if (strcmp((char *)attr->name, "Control") == 0) { newkm.modifierMask |= ControlMask; }
      else if (strcmp((char *)attr->name, "Shift") == 0)   { newkm.modifierMask |= ShiftMask; }
      else if (strcmp((char *)attr->name, "Lock") == 0)    { newkm.modifierMask |= LockMask;  }
      else if (strcmp((char *)attr->name, "AltMeta") == 0) { newkm.modifierAltMeta = True; }
    }
  }

  if (newkm.stroke != KEYSTROKE_NOTHING && newkm.keysym != NoSymbol)
  {
    /* keysym and stroke are required, everything else is optional */
    memcpy(ret, &newkm, sizeof(struct nxagentSpecialKeystrokeMap));
    return True;
  }
  else
    return False;
}

char *checkKeystrokeFile(char *filename)
{
  if (!filename)
    return NULL;

  if (access(filename, R_OK) == 0)
  {
    return filename;
  }
  else
  {
    #ifdef WARNING
    fprintf(stderr, "Warning: Cannot read keystroke file '%s'.\n", filename);
    #endif
    return NULL;
  }
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
 * If run in x2go flavour different filenames and varnames are used.
 */
void nxagentInitKeystrokes(Bool force)
{
  char *filename = NULL;

  char *homefile;
  char *etcfile;
  char *envvar;

  /* used for tracking if the config file parsing has already been
     done (regardless of the result) */
  static Bool done = False;

  if (force) {
    if (map != default_map)
    {
      SAFE_free(map);
      map = default_map;
    }
    fprintf(stderr, "Info: re-reading keystrokes configuration\n");
  }
  else
  {
    if (done)
      return;
  }

  done = True;

#ifdef X2GO
  if (nxagentX2go) {
    homefile = "/.x2go/config/keystrokes.cfg";
    etcfile = "/etc/x2go/keystrokes.cfg";
    envvar = "X2GO_KEYSTROKEFILE";
  }
  else
#endif
  {
    homefile = "/.nx/config/keystrokes.cfg";
    etcfile = "/etc/nxagent/keystrokes.cfg";
    envvar = "NXAGENT_KEYSTROKEFILE";
  }

  char *homepath = NULL;
  char *homedir = getenv("HOME");
  if (homedir)
  {
    if (-1 == asprintf(&homepath, "%s%s", homedir, homefile))
    {
      fprintf(stderr, "malloc failed");
      exit(EXIT_FAILURE);
    }
  }

  /* if any of the files can be read we have our candidate */
  if ((filename = checkKeystrokeFile(nxagentKeystrokeFile)) ||
      (filename = checkKeystrokeFile(getenv(envvar))) ||
      (filename = checkKeystrokeFile(homepath)) ||
      (filename = checkKeystrokeFile(etcfile)))
  {
    LIBXML_TEST_VERSION
    xmlDoc *doc = xmlReadFile(filename, NULL, 0);
    if (doc)
    {
      fprintf(stderr, "Info: using keystrokes file '%s'\n", filename);
      for (xmlNode *cur = xmlDocGetRootElement(doc); cur; cur = cur->next)
      {
        if (cur->type == XML_ELEMENT_NODE && strcmp((char *)cur->name, "keystrokes") == 0)
        {
          xmlNode *bindings;
          int num = 0;
          int idx = 0;

          for (bindings = cur->children; bindings; bindings = bindings->next)
          {
            if (bindings->type == XML_ELEMENT_NODE && strcmp((char *)bindings->name, "keystroke") == 0)
            {
              num++;
            }
          }
          #ifdef DEBUG
          fprintf(stderr, "%s: found %d keystrokes in %s\n", __func__, num, filename);
          #endif
          if (!(map = calloc(num+1, sizeof(struct nxagentSpecialKeystrokeMap))))
          {
            fprintf(stderr, "calloc failed");
            exit(EXIT_FAILURE);
          }

          for (bindings = cur->children; bindings; bindings = bindings->next)
          {
            map[idx].stroke = KEYSTROKE_NOTHING;
            if (bindings->type == XML_ELEMENT_NODE &&
                strcmp((char *)bindings->name, "keystroke") == 0 &&
                read_binding_from_xmlnode(bindings, &(map[idx])))
              {
                Bool store = True;
                for (int j = 0; j < idx; j++)
                {
                  if (map[j].stroke != KEYSTROKE_NOTHING &&
                      map[idx].keysym != NoSymbol &&
                      map[j].keysym == map[idx].keysym &&
                      map[j].modifierMask == map[idx].modifierMask &&
                      map[j].modifierAltMeta == map[idx].modifierAltMeta)
                  {
                      #ifdef WARNING
                      fprintf(stderr, "Warning: ignoring keystroke '%s' (already in use by '%s')\n",
                              nxagentSpecialKeystrokeNames[map[idx].stroke],
                              nxagentSpecialKeystrokeNames[map[j].stroke]);
                      #endif
                      store = False;
                      break;
                  }
                }

                if (store)
                  idx++;
                else
                  map[idx].stroke = KEYSTROKE_NOTHING;
              }
          }
          #ifdef DEBUG
          fprintf(stderr, "%s: read %d keystrokes", __func__, idx);
          #endif

          map[idx].stroke = KEYSTROKE_END_MARKER;
        }
      }

      xmlFreeDoc(doc);
      xmlCleanupParser();
    }
    #ifdef WARNING
    else /* if (doc) */
    {
      fprintf(stderr, "Warning: could not read/parse keystrokes file '%s'\n", filename);
    }
    #endif
    filename = NULL;
  }
  SAFE_free(homepath);

  if (map == default_map)
  {
    fprintf(stderr, "Info: Using builtin keystrokes.\n");
  }

  nxagentDumpKeystrokes();
}

static char *nxagentGetSingleKeystrokeString(struct nxagentSpecialKeystrokeMap *cur)
{
  if (!cur)
    return strdup(""); /* caller is expected to free the returned string */

  char *s1, *s2, *s3, *s4, *s5, *s6, *s7, *s8, *s9, *s10, *s11;
  s1 = s2 = s3 = s4 = s5 = s6 = s7 = s8 = s9 = s10 = s11 = "";

  unsigned int mask = cur->modifierMask;

  if (mask & ControlMask)        {s1  = "Ctrl+";     mask &= ~ControlMask;}
  if (mask & ShiftMask)          {s2  = "Shift+";    mask &= ~ShiftMask;}

  /* these are only here for better readable modifier names. Normally
     they are covered by the Mod<n> and Lock lines below */
  if (cur->modifierAltMeta)      {s3  = "Alt+";      mask &= ~(cur->modifierAltMeta);}
  if (mask & nxagentCapsMask)    {s4  = "CapsLock+"; mask &= ~nxagentCapsMask;}
  if (mask & nxagentNumlockMask) {s5  = "NumLock+";  mask &= ~nxagentNumlockMask;}

  if (mask & Mod1Mask)           {s6  = "Mod1+";     mask &= ~Mod1Mask;}
  if (mask & Mod2Mask)           {s7  = "Mod2+";     mask &= ~Mod2Mask;}
  if (mask & Mod3Mask)           {s8  = "Mod3+";     mask &= ~Mod3Mask;}
  if (mask & Mod4Mask)           {s9  = "Mod4+";     mask &= ~Mod4Mask;}
  if (mask & Mod5Mask)           {s10 = "Mod5+";     mask &= ~Mod5Mask;}
  if (mask & LockMask)           {s11 = "Lock+";     mask &= ~LockMask;}

  char *ret = NULL;
  asprintf(&ret, "%s%s%s%s%s%s%s%s%s%s%s%s", s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, XKeysymToString(cur->keysym));
  return ret;
}

/*
 * return the _first_ keystroke for the passed keystroke name
 *
 * e.g. nxagentFindFirstKeystroke("resize") -> "Ctrl+Alt+r"
 *
 * result must be free()d after use.
 */
char *nxagentFindFirstKeystroke(char *name)
{
  for (struct nxagentSpecialKeystrokeMap *cur = map; cur->stroke != KEYSTROKE_END_MARKER; cur++)
  {
    if (nxagentSpecialKeystrokeNames[cur->stroke] &&
            strcmp(nxagentSpecialKeystrokeNames[cur->stroke], name) == 0)
    {
      return nxagentGetSingleKeystrokeString(cur);
    }
  }
  return NULL;
}

/*
 * return a string with linefeeds of all keystrokes who's name starts
 * with the the passed string,
 *
 * e.g. nxagentFindKeystrokeString("viewport_scroll_")
 * ->
 * "  viewport_scroll_left  : Ctrl+Alt+Left
 *   viewport_scroll_left  : Ctrl+Alt+KP_Left
 *   viewport_scroll_up    : Ctrl+Alt+Up
 *   viewport_scroll_up    : Ctrl+Alt+KP_Up
 *   viewport_scroll_right : Ctrl+Alt+Right
 *   viewport_scroll_right : Ctrl+Alt+KP_Right
 *   viewport_scroll_down  : Ctrl+Alt+Down
 *   viewport_scroll_down  : Ctrl+Alt+KP_Down
 * "
 * result must be free()d after use.
 */
char *nxagentFindMatchingKeystrokes(char *name)
{
  int maxlen = 0;
  for (int i = 0; nxagentSpecialKeystrokeNames[i]; i++)
    maxlen = max(maxlen, strlen(nxagentSpecialKeystrokeNames[i]));

  char * res = strdup(""); /* let the caller free the string */
  for (struct nxagentSpecialKeystrokeMap *cur = map; cur->stroke != KEYSTROKE_END_MARKER; cur++)
  {
    if (nxagentSpecialKeystrokeNames[cur->stroke] &&
        strncmp(nxagentSpecialKeystrokeNames[cur->stroke], name, strlen(name)) == 0)
    {
      char *tmp;
      char *tmp1 = nxagentGetSingleKeystrokeString(cur);
      if (-1 == asprintf(&tmp, "%s\n  %-*s : %s", res, maxlen,
                         nxagentSpecialKeystrokeNames[cur->stroke],
                         tmp1))
      {
        SAFE_free(tmp1);
        #ifdef TEST
        fprintf(stderr, "%s: returning incomplete result:%s\n", __func__, res);
        #endif
        return res;
      }
      else
      {
        SAFE_free(tmp1);
        free(res);
        res = tmp;
      }
    }
  }
  #ifdef TEST
  fprintf(stderr, "%s: returning result:\n%s", __func__, res);
  #endif
  return res;
}

void nxagentDumpKeystrokes(void)
{
  char *s = nxagentFindMatchingKeystrokes("");
  fprintf(stderr, "Currently known keystrokes:\n%s", s);
  SAFE_free(s);
}

static enum nxagentSpecialKeystroke find_keystroke(XKeyEvent *X)
{
  enum nxagentSpecialKeystroke ret = KEYSTROKE_NOTHING;

  KeySym keysym = XKeycodeToKeysym(nxagentDisplay, X->keycode, 0);

  #ifdef DEBUG
  fprintf(stderr, "%s: got keysym '%c' (%d)\n", __func__, keysym, keysym);
  #endif
  for (struct nxagentSpecialKeystrokeMap *cur = map; cur->stroke != KEYSTROKE_END_MARKER; cur++) {
    #ifdef DEBUG
    fprintf(stderr,"%s: keysym %d stroke %d, type %d\n", __func__, cur->keysym, cur->stroke, X->type);
    #endif
    if (cur->keysym == keysym && modifier_matches(cur->modifierMask, cur->modifierAltMeta, X->state)) {
      #ifdef DEBUG
      fprintf(stderr, "%s: match including modifiers for keysym '%c' (%d), stroke %d (%s)\n", __func__, cur->keysym, cur->keysym, cur->stroke, nxagentSpecialKeystrokeNames[cur->stroke]);
      #endif
      return cur->stroke;
    }
  }

  return ret;
}

/*
 * returns True if a special keystroke has been pressed. *result will contain the action.
 */
Bool nxagentCheckSpecialKeystroke(XKeyEvent *X, enum HandleEventResult *result)
{
  enum nxagentSpecialKeystroke stroke = find_keystroke(X);
  *result = doNothing;

  #ifdef TEST
  if (stroke != KEYSTROKE_NOTHING && stroke != KEYSTROKE_END_MARKER)
    fprintf(stderr, "nxagentCheckSpecialKeystroke: got code %x - state %x - stroke %d (%s)\n",
            X -> keycode, X -> state, stroke, nxagentSpecialKeystrokeNames[stroke]);
  else
    fprintf(stderr, "nxagentCheckSpecialKeystroke: got code %x - state %x - stroke %d (unused)\n",
            X -> keycode, X -> state, stroke);
  #endif

  if (stroke == KEYSTROKE_NOTHING)
    return False;

  /*
   * Check special keys.
   */

  /*
   * FIXME: We should use the keysym instead that the keycode
   *        here.
   */

  switch (stroke) {
#ifdef DEBUG_TREE
    case KEYSTROKE_DEBUG_TREE:
      *result = doDebugTree;
      break;
#endif
    case KEYSTROKE_CLOSE_SESSION:
      *result = doCloseSession;
      break;
    case KEYSTROKE_SWITCH_ALL_SCREENS:
      if (!nxagentOption(Rootless)) {
        *result = doSwitchAllScreens;
      }
      break;
    case KEYSTROKE_MINIMIZE:
      if (!nxagentOption(Rootless)) {
        *result = doMinimize;
      }
      break;
    case KEYSTROKE_DEFER:
      *result = doSwitchDeferMode;
      break;
    case KEYSTROKE_IGNORE:
      /* this is used e.g. to ignore C-A-Backspace aka XK_Terminate_Server */
      return True;
      break;
    case KEYSTROKE_FORCE_SYNCHRONIZATION:
      nxagentForceSynchronization = True;
      break;
#ifdef DUMP
    case KEYSTROKE_REGIONS_ON_SCREEN:
      nxagentRegionsOnScreen();
      break;
#endif
#ifdef NX_DEBUG_INPUT
    case KEYSTROKE_TEST_INPUT:
      /*
       * Used to test the input devices state.
       */
      if (X -> type == KeyPress) {
        if (!nxagentDebugInputDevices) {
          fprintf(stderr, "Info: Turning input devices debug ON.\n");
          nxagentDebugInputDevices = True;
        } else {
          fprintf(stderr, "Info: Turning input devices debug OFF.\n");
          nxagentDebugInputDevices = False;
          nxagentLastInputDevicesDumpTime = 0;
        }
      }
      return True;
      break;
    case KEYSTROKE_DEACTIVATE_INPUT_DEVICES_GRAB:
      if (X->type == KeyPress) {
        nxagentDeactivateInputDevicesGrabs();
      }
      return True;
      break;
#endif
    case KEYSTROKE_FULLSCREEN:
      if (!nxagentOption(Rootless)) {
        *result = doSwitchFullscreen;
      }
      break;
    case KEYSTROKE_RESIZE:
      if (!nxagentOption(Rootless)) {
        *result = doSwitchResizeMode;
      }
      break;
    case KEYSTROKE_VIEWPORT_MOVE_LEFT:
      if (!nxagentOption(Rootless) && !nxagentOption(DesktopResize)) {
        *result = doViewportMoveLeft;
      }
      break;
    case KEYSTROKE_VIEWPORT_MOVE_UP:
      if (!nxagentOption(Rootless) && !nxagentOption(DesktopResize)) {
        *result = doViewportMoveUp;
      }
      break;
    case KEYSTROKE_VIEWPORT_MOVE_RIGHT:
      if (!nxagentOption(Rootless) && !nxagentOption(DesktopResize)) {
        *result = doViewportMoveRight;
      }
      break;
    case KEYSTROKE_VIEWPORT_MOVE_DOWN:
      if (!nxagentOption(Rootless) && !nxagentOption(DesktopResize)) {
        *result = doViewportMoveDown;
      }
      break;
    case KEYSTROKE_VIEWPORT_SCROLL_LEFT:
      if (!nxagentOption(Rootless) && !nxagentOption(DesktopResize)) {
        *result = doViewportLeft;
      }
      break;
    case KEYSTROKE_VIEWPORT_SCROLL_UP:
      if (!nxagentOption(Rootless) && !nxagentOption(DesktopResize)) {
        *result = doViewportUp;
      }
      break;
    case KEYSTROKE_VIEWPORT_SCROLL_RIGHT:
      if (!nxagentOption(Rootless) && !nxagentOption(DesktopResize)) {
        *result = doViewportRight;
      }
      break;
    case KEYSTROKE_VIEWPORT_SCROLL_DOWN:
      if (!nxagentOption(Rootless) && !nxagentOption(DesktopResize)) {
        *result = doViewportDown;
      }
      break;
    case KEYSTROKE_REREAD_KEYSTROKES:
      /* two reasons to check on KeyRelease:
         - this code is called for KeyPress and KeyRelease, so we
           would read the keystroke file twice
         - if the keystroke file changes settings for this key this
           might lead to unexpected behaviour
      */
      if (X->type == KeyRelease)
        nxagentInitKeystrokes(True);
      break;
    case KEYSTROKE_AUTOGRAB:
      *result = doAutoGrab;
      break;
    case KEYSTROKE_DUMP_CLIPBOARD:
      *result = doDumpClipboard;
      break;
    case KEYSTROKE_NOTHING: /* do nothing. difference to KEYSTROKE_IGNORE is the return value */
    case KEYSTROKE_END_MARKER: /* just to make gcc STFU */
    case KEYSTROKE_MAX:
      break;
  }
  return (*result != doNothing);
}
