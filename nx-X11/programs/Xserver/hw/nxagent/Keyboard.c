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

/*

Copyright 1993 by Davor Matic

Permission to use, copy, modify, distribute, and sell this software
and its documentation for any purpose is hereby granted without fee,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation.  Davor Matic makes no representations about
the suitability of this software for any purpose.  It is provided "as
is" without express or implied warranty.

*/
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define NEED_EVENTS
#include "X.h"
#include "Xproto.h"
#include "keysym.h"
#include "screenint.h"
#include "inputstr.h"
#include "misc.h"
#include "scrnintstr.h"
#include "servermd.h"
#include "dixstruct.h"
#include "extnsionst.h"

#include "Agent.h"
#include "Display.h"
#include "Screen.h"
#include "Keyboard.h"
#include "Events.h"
#include "Options.h"
#include "Error.h"

#include "NXlib.h"

#include "Shadow.h"

#ifdef XKB

#include "globals.h"
#include "property.h"

#include <X11/extensions/XKB.h>

#define XKBSRV_NEED_FILE_FUNCS
#include <X11/extensions/XKBsrv.h>
#include <X11/extensions/XKBconfig.h>

#include "X11/extensions/XKBrules.h"

#include "Xatom.h"

static int nxagentXkbGetNames(char **rules, char **model, char **layout,
                                  char **variant, char **options);

static void nxagentKeycodeConversionSetup(void);

#endif /* XKB */

/*
 * Set here the required log level.
 */

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG
#undef  WATCH

#ifdef WATCH
#include "unistd.h"
#endif

/*
 * Unfortunately we cannot just include XKBlib.h.
 * It conflicts with the server side definitions
 * of the same symbols. This is more a X problem
 * than our.
 */

#ifdef XKB

extern Bool XkbQueryExtension(
#if NeedFunctionPrototypes
        Display *        /* dpy */,
        int *            /* opcodeReturn */,
        int *            /* eventBaseReturn */,
        int *            /* errorBaseReturn */,
        int *            /* majorRtrn */,
        int *            /* minorRtrn */
#endif
);

extern        XkbDescPtr XkbGetKeyboard(
#if NeedFunctionPrototypes
        Display *        /* dpy */,
        unsigned int     /* which */,
        unsigned int     /* deviceSpec */
#endif
);

extern        Status        XkbGetControls(
#if NeedFunctionPrototypes
        Display *        /* dpy */,
        unsigned long    /* which */,
        XkbDescPtr       /* desc */
#endif
);

#ifndef XKB_BASE_DIRECTORY
#define XKB_BASE_DIRECTORY   "/usr/share/X11/xkb"
#endif
#ifndef XKB_ALTERNATE_BASE_DIRECTORY
#define XKB_ALTERNATE_BASE_DIRECTORY   "/usr/X11R6/lib/X11/xkb"
#endif
#ifndef XKB_CONFIG_FILE
#define XKB_CONFIG_FILE      "/etc/nxagent/nxagent.keyboard"
#endif
#ifndef XKB_DFLT_RULES_FILE
#define XKB_DFLT_RULES_FILE  "xfree86"
#endif
#ifndef XKB_ALTS_RULES_FILE
#define XKB_ALTS_RULES_FILE  "xorg"
#endif
#ifndef XKB_DFLT_KB_LAYOUT
#define XKB_DFLT_KB_LAYOUT   "us"
#endif
#ifndef XKB_DFLT_KB_MODEL
#define XKB_DFLT_KB_MODEL    "pc102"
#endif
#ifndef XKB_DFLT_KB_VARIANT
#define XKB_DFLT_KB_VARIANT  NULL
#endif
#ifndef XKB_DFLT_KB_OPTIONS
#define XKB_DFLT_KB_OPTIONS  NULL
#endif

#define NXAGENT_KEYMAP_DIR_FILE "keymap.dir"

extern int XkbDfltRepeatDelay;
extern int XkbDfltRepeatInterval;

#endif /* XKB */

/*
 * Save the values queried from X server.
 */

XkbAgentInfoRec nxagentXkbInfo = { -1, -1, -1, -1, -1 };

/*
 * Keyboard status, updated through XKB
 * events.
 */

XkbAgentStateRec nxagentXkbState = { 0, 0, 0, 0, 0 };

/*
 * Info for disabling/enabling Xkb extension.
 */

XkbWrapperRec nxagentXkbWrapper;

extern char *nxagentKeyboard;

static char *nxagentXkbGetRules(void);

unsigned int nxagentAltMetaMask;

static void nxagentCheckAltMetaKeys(CARD8, int);

CARD8 nxagentCapsLockKeycode = 66;
CARD8 nxagentNumLockKeycode  = 77;

static void nxagentCheckRemoteKeycodes(void);

static CARD8 nxagentConvertedKeycodes[] =
{
  /* evdev  pc105*/
  /*   0 */   0,
  /*   1 */   1,
  /*   2 */   2,
  /*   3 */   3,
  /*   4 */   4,
  /*   5 */   5,
  /*   6 */   6,
  /*   7 */   7,
  /*   8 */   8,
  /*   9 */   9,
  /*  10 */   10,
  /*  11 */   11,
  /*  12 */   12,
  /*  13 */   13,
  /*  14 */   14,
  /*  15 */   15,
  /*  16 */   16,
  /*  17 */   17,
  /*  18 */   18,
  /*  19 */   19,
  /*  20 */   20,
  /*  21 */   21,
  /*  22 */   22,
  /*  23 */   23,
  /*  24 */   24,
  /*  25 */   25,
  /*  26 */   26,
  /*  27 */   27,
  /*  28 */   28,
  /*  29 */   29,
  /*  30 */   30,
  /*  31 */   31,
  /*  32 */   32,
  /*  33 */   33,
  /*  34 */   34,
  /*  35 */   35,
  /*  36 */   36,
  /*  37 */   37,
  /*  38 */   38,
  /*  39 */   39,
  /*  40 */   40,
  /*  41 */   41,
  /*  42 */   42,
  /*  43 */   43,
  /*  44 */   44,
  /*  45 */   45,
  /*  46 */   46,
  /*  47 */   47,
  /*  48 */   48,
  /*  49 */   49,
  /*  50 */   50,
  /*  51 */   51,
  /*  52 */   52,
  /*  53 */   53,
  /*  54 */   54,
  /*  55 */   55,
  /*  56 */   56,
  /*  57 */   57,
  /*  58 */   58,
  /*  59 */   59,
  /*  60 */   60,
  /*  61 */   61,
  /*  62 */   62,
  /*  63 */   63,
  /*  64 */   64,
  /*  65 */   65,
  /*  66 */   66,
  /*  67 */   67,
  /*  68 */   68,
  /*  69 */   69,
  /*  70 */   70,
  /*  71 */   71,
  /*  72 */   72,
  /*  73 */   73,
  /*  74 */   74,
  /*  75 */   75,
  /*  76 */   76,
  /*  77 */   77,
  /*  78 */   78,
  /*  79 */   79,
  /*  80 */   80,
  /*  81 */   81,
  /*  82 */   82,
  /*  83 */   83,
  /*  84 */   84,
  /*  85 */   85,
  /*  86 */   86,
  /*  87 */   87,
  /*  88 */   88,
  /*  89 */   89,
  /*  90 */   90,
  /*  91 */   91,
  /*  92 */  124,
  /*  93 */   93,
  /*  94 */   94,
  /*  95 */   95,
  /*  96 */   96,
  /*  97 */  211,
  /*  98 */   98,
  /*  99 */   99,
  /* 100 */  100,
  /* 101 */  208,
  /* 102 */  102,
  /* 103 */  103,
  /* 104 */  108,
  /* 105 */  109,
  /* 106 */  112,
  /* 107 */  111,
  /* 108 */  113,
  /* 109 */  109,
  /* 110 */   97,
  /* 111 */   98,
  /* 112 */   99,
  /* 113 */  100,
  /* 114 */  102,
  /* 115 */  103,
  /* 116 */  104,
  /* 117 */  105,
  /* 118 */  106,
  /* 119 */  107,
  /* 120 */  120,
  /* 121 */  121,
  /* 122 */  122,
  /* 123 */  123,
  /* 124 */  124,
  /* 125 */  126,
  /* 126 */  126,
  /* 127 */  110,
  /* 128 */  128,
  /* 129 */  129,
  /* 130 */  130,
  /* 131 */  131,
  /* 132 */  133,
  /* 133 */  115,
  /* 134 */  116,
  /* 135 */  117,
  /* 136 */  136,
  /* 137 */  137,
  /* 138 */  138,
  /* 139 */  139,
  /* 140 */  140,
  /* 141 */  141,
  /* 142 */  142,
  /* 143 */  143,
  /* 144 */  144,
  /* 145 */  145,
  /* 146 */  146,
  /* 147 */  147,
  /* 148 */  148,
  /* 149 */  149,
  /* 150 */  150,
  /* 151 */  151,
  /* 152 */  152,
  /* 153 */  153,
  /* 154 */  154,
  /* 155 */  155,
  /* 156 */  156,
  /* 157 */  157,
  /* 158 */  158,
  /* 159 */  159,
  /* 160 */  160,
  /* 161 */  161,
  /* 162 */  162,
  /* 163 */  163,
  /* 164 */  164,
  /* 165 */  165,
  /* 166 */  166,
  /* 167 */  167,
  /* 168 */  168,
  /* 169 */  169,
  /* 170 */  170,
  /* 171 */  171,
  /* 172 */  172,
  /* 173 */  173,
  /* 174 */  174,
  /* 175 */  175,
  /* 176 */  176,
  /* 177 */  177,
  /* 178 */  178,
  /* 179 */  179,
  /* 180 */  180,
  /* 181 */  181,
  /* 182 */  182,
  /* 183 */  183,
  /* 184 */  184,
  /* 185 */  185,
  /* 186 */  186,
  /* 187 */  187,
  /* 188 */  188,
  /* 189 */  189,
  /* 190 */  190,
  /* 191 */  118,
  /* 192 */  119,
  /* 193 */  120,
  /* 194 */  121,
  /* 195 */  122,
  /* 196 */  196,
  /* 197 */  197,
  /* 198 */  198,
  /* 199 */  199,
  /* 200 */  200,
  /* 201 */  201,
  /* 202 */  202,
  /* 203 */   93,
  /* 204 */  125,
  /* 205 */  156,
  /* 206 */  127,
  /* 207 */  128,
  /* 208 */  208,
  /* 209 */  209,
  /* 210 */  210,
  /* 211 */  211,
  /* 212 */  212,
  /* 213 */  213,
  /* 214 */  214,
  /* 215 */  215,
  /* 216 */  216,
  /* 217 */  217,
  /* 218 */  218,
  /* 219 */  219,
  /* 220 */  220,
  /* 221 */  221,
  /* 222 */  222,
  /* 223 */  223,
  /* 224 */  224,
  /* 225 */  225,
  /* 226 */  226,
  /* 227 */  227,
  /* 228 */  228,
  /* 229 */  229,
  /* 230 */  230,
  /* 231 */  231,
  /* 232 */  232,
  /* 233 */  233,
  /* 234 */  234,
  /* 235 */  235,
  /* 236 */  236,
  /* 237 */  237,
  /* 238 */  238,
  /* 239 */  239,
  /* 240 */  240,
  /* 241 */  241,
  /* 242 */  242,
  /* 243 */  243,
  /* 244 */  244,
  /* 245 */  245,
  /* 246 */  246,
  /* 247 */  247,
  /* 248 */  248,
  /* 249 */  249,
  /* 250 */  250,
  /* 251 */  251,
  /* 252 */  252,
  /* 253 */  253,
  /* 254 */  254,
  /* 255 */  255
};

static int nxagentKeycodeConversion = 0;

CARD8 nxagentConvertKeycode(CARD8 k)
{
 if (nxagentKeycodeConversion != 0)
 {
   return nxagentConvertedKeycodes[k];
 }
 else
 {
   return k;
 }
}

static int nxagentSaveKeyboardDeviceData(DeviceIntPtr dev, DeviceIntPtr devBackup);

static int nxagentRestoreKeyboardDeviceData(DeviceIntPtr devBackup, DeviceIntPtr dev);

static int nxagentFreeKeyboardDeviceData(DeviceIntPtr dev);

static void nxagentCheckXkbBaseDirectory(void)
{

  /*
   * Set XkbBaseDirectory global
   * variable appropriately.
   */

  #ifdef TEST
  fprintf(stderr, "nxagentCheckXkbBaseDirectory: "
              "Before calling _NXGetXkbBasePath.\n");

  fprintf(stderr, "nxagentCheckXkbBaseDirectory: "
              "XkbBaseDirectory varible [%s].\n",
                  XkbBaseDirectory);
  #endif

  XkbBaseDirectory = _NXGetXkbBasePath(XkbBaseDirectory);

  #ifdef TEST
  fprintf(stderr, "nxagentCheckXkbBaseDirectory: "
              "After calling _NXGetXkbBasePath.\n");

  fprintf(stderr, "nxagentCheckXkbBaseDirectory: "
              "XkbBaseDirectory varible [%s].\n",
                  XkbBaseDirectory);
  #endif

  return;
}

static char *nxagentXkbGetRules()
{
  int ret;
  int size, sizeDflt, sizeAlt;
  char *path;
  struct stat buf;

  #ifdef TEST
  fprintf(stderr, "nxagentXkbGetRules: XkbBaseDirectory [%s].\n",
              XkbBaseDirectory);
  #endif

  sizeDflt = strlen(XKB_DFLT_RULES_FILE);
  sizeAlt = strlen(XKB_ALTS_RULES_FILE);
  size = strlen(XkbBaseDirectory) + strlen("/rules/");
  size += (sizeDflt > sizeAlt) ? sizeDflt: sizeAlt;

  if ((path = malloc((size + 1) * sizeof(char))) == NULL)
  {
    FatalError("nxagentXkbGetRules: malloc failed.");
  }

  strcpy(path, XkbBaseDirectory);
  strcat(path, "/rules/");
  strcat(path, XKB_DFLT_RULES_FILE);
  ret = stat(path, &buf);

  if (ret == 0)
  {
    free(path);
    return XKB_DFLT_RULES_FILE;
  }

  #ifdef TEST
  fprintf(stderr, "nxagentXkbGetRules: WARNING! Failed to stat file [%s]: %s.\n", path, strerror(ret));
  #endif 

  strcpy(path, XkbBaseDirectory);
  strcat(path, "/rules/");
  strcat(path, XKB_ALTS_RULES_FILE);
  ret = stat(path, &buf);

  if (ret == 0)
  {
    free(path);
    return XKB_ALTS_RULES_FILE;
  }

  #ifdef WARNING
  fprintf(stderr, "nxagentXkbGetRules: WARNING! Failed to stat file [%s]: %s.\n", path, strerror(ret));
  #endif

  free(path);
  return XKB_DFLT_RULES_FILE;
}
 
void nxagentBell(int volume, DeviceIntPtr pDev, pointer ctrl, int cls)
{
  XBell(nxagentDisplay, volume);
}

void nxagentChangeKeyboardControl(DeviceIntPtr pDev, KeybdCtrl *ctrl)
{
  #ifdef XKB

  XkbSrvInfoPtr  xkbi;
  XkbControlsPtr xkbc;

  if (!noXkbExtension)
  {
    xkbi = pDev -> key -> xkbInfo;
    xkbc = xkbi -> desc -> ctrls;

    /*
     * We want to prevent agent generating auto-repeated
     * keystrokes. Let's intercept any attempt by appli-
     * cations to change the default timeouts on the
     * nxagent device.
     */

    #ifdef TEST
    fprintf(stderr, "nxagentChangeKeyboardControl: Repeat delay was [%d] interval was [%d].\n",
                xkbc -> repeat_delay, xkbc -> repeat_interval);
    #endif

    xkbc -> repeat_delay = ~ 0;
    xkbc -> repeat_interval = ~ 0;

    #ifdef TEST
    fprintf(stderr, "nxagentChangeKeyboardControl: Repeat delay is now [%d] interval is now [%d].\n",
                xkbc -> repeat_delay, xkbc -> repeat_interval);
    #endif
  }

  #endif

  /*
   * If enabled, propagate the changes to the
   * devices attached to the real X server.
   */

  if (nxagentOption(DeviceControl) == True)
  {
    unsigned long value_mask;
    XKeyboardControl values;
    int i;

    #ifdef TEST
    fprintf(stderr, "nxagentChangeKeyboardControl: WARNING! Propagating changes to keyboard settings.\n");
    #endif

    value_mask = KBKeyClickPercent |
                 KBBellPercent |
                 KBBellPitch |
                 KBBellDuration;

    values.key_click_percent = ctrl->click;
    values.bell_percent = ctrl->bell;
    values.bell_pitch = ctrl->bell_pitch;
    values.bell_duration = ctrl->bell_duration;

    /*
     * Don't propagate the auto repeat mode. It is forced to be
     * off in the agent server.
     *
     * value_mask |= KBAutoRepeatMode;
     * values.auto_repeat_mode = ctrl->autoRepeat ?
     *                          AutoRepeatModeOn : AutoRepeatModeOff;
     */

    XChangeKeyboardControl(nxagentDisplay, value_mask, &values);

    /*
     * At this point, we need to walk through the vector and
     * compare it to the current server vector. If there are
     * differences, report them.
     */

    value_mask = KBLed | KBLedMode;

    for (i = 1; i <= 32; i++)
    {
      values.led = i;
      values.led_mode = (ctrl->leds & (1 << (i - 1))) ? LedModeOn : LedModeOff;

      XChangeKeyboardControl(nxagentDisplay, value_mask, &values);
    }

    return;
  }

  #ifdef TEST
  fprintf(stderr, "nxagentChangeKeyboardControl: WARNING! Not propagating changes to keyboard settings.\n");
  #endif
}

int nxagentKeyboardProc(DeviceIntPtr pDev, int onoff)
{
  XModifierKeymap *modifier_keymap;
  KeySym *keymap;
  int mapWidth;
  int min_keycode, max_keycode;
  KeySymsRec keySyms;
  CARD8 modmap[256];
  int i, j;
  XKeyboardState values;
  char *model = NULL, *layout = NULL;
  int free_model = 0, free_layout = 0;
  XkbDescPtr xkb = NULL;

  int ret;

  switch (onoff)
  {
    case DEVICE_INIT:

      #ifdef TEST
      fprintf(stderr, "nxagentKeyboardProc: Called for [DEVICE_INIT].\n");
      #endif

      if (NXDisplayError(nxagentDisplay) == 1)
      {
        return Success;
      }

      #ifdef WATCH

      fprintf(stderr, "nxagentKeyboardProc: Watchpoint 9.\n");

/*
Reply   Total	Cached	Bits In			Bits Out		Bits/Reply	  Ratio
------- -----	------	-------			--------		----------	  -----
N/A
*/

      sleep(30);

      #endif

      /*
       * Prevent agent from generating auto-repeat keystroke.
       * Note that this is working only if XKB is enabled.
       * A better solution should account cases where XKB is
       * not available. Check also the behaviour of the
       * DeviceControl nxagent option.
       */

      XkbDfltRepeatDelay = ~ 0;
      XkbDfltRepeatInterval = ~ 0;

      #ifdef TEST
      fprintf(stderr, "nxagentKeyboardProc: Set repeat delay to [%u] interval to [%u].\n",
                  XkbDfltRepeatDelay, XkbDfltRepeatInterval);
      #endif

      modifier_keymap = XGetModifierMapping(nxagentDisplay);

      if (modifier_keymap == NULL)
      {
        return -1;
      }

      XDisplayKeycodes(nxagentDisplay, &min_keycode, &max_keycode);
#ifdef _XSERVER64
      {
        KeySym64 *keymap64;
        int i, len;
        keymap64 = XGetKeyboardMapping(nxagentDisplay,
                                     min_keycode,
                                     max_keycode - min_keycode + 1,
                                     &mapWidth);

        if (keymap64 == NULL)
        {
          XFreeModifiermap(modifier_keymap);

          return -1;
        }

        len = (max_keycode - min_keycode + 1) * mapWidth;
        keymap = (KeySym *)xalloc(len * sizeof(KeySym));
        for(i = 0; i < len; ++i)
          keymap[i] = keymap64[i];
        XFree(keymap64);
      }

#else /* #ifdef _XSERVER64 */

      keymap = XGetKeyboardMapping(nxagentDisplay,
                                   min_keycode,
                                   max_keycode - min_keycode + 1,
                                   &mapWidth);

      if (keymap == NULL)
      {
        XFreeModifiermap(modifier_keymap);

        return -1;
      }

#endif /* #ifdef _XSERVER64 */

      nxagentAltMetaMask = 0;

      for (i = 0; i < 256; i++)
        modmap[i] = 0;
      for (j = 0; j < 8; j++)
        for(i = 0; i < modifier_keymap->max_keypermod; i++) {
          CARD8 keycode;
          if ((keycode =
              modifier_keymap->
                modifiermap[j * modifier_keymap->max_keypermod + i]))
            modmap[keycode] |= 1<<j;

          if (keycode > 0)
          {
            nxagentCheckAltMetaKeys(keycode, j);
          }
        }
      XFreeModifiermap(modifier_keymap);

      nxagentCheckRemoteKeycodes();

      keySyms.minKeyCode = min_keycode;
      keySyms.maxKeyCode = max_keycode;
      keySyms.mapWidth = mapWidth;
      keySyms.map = keymap;

#ifdef XKB

      /*
       * First of all the validity
       * of XkbBaseDirectory global
       * variable is checked.
       */

      nxagentCheckXkbBaseDirectory();

      if (noXkbExtension) {
        #ifdef TEST
        fprintf(stderr, "nxagentKeyboardProc: No XKB extension.\n");
        #endif

XkbError:

        #ifdef TEST
        fprintf(stderr, "nxagentKeyboardProc: XKB error.\n");
        #endif

        XkbFreeKeyboard(xkb, XkbAllComponentsMask, True);
        xkb = NULL;
        if (free_model) 
        {
          free_model = 0;
          free(model);
        }
        if (free_layout)
        {
          free_layout = 0;
          free(layout);
        }
#endif
      XGetKeyboardControl(nxagentDisplay, &values);

      memmove((char *) defaultKeyboardControl.autoRepeats,
             (char *) values.auto_repeats, sizeof(values.auto_repeats));

      ret = InitKeyboardDeviceStruct((DevicePtr) pDev, &keySyms, modmap,
                               nxagentBell, nxagentChangeKeyboardControl);

      #ifdef TEST
      fprintf(stderr, "nxagentKeyboardProc: InitKeyboardDeviceStruct returns [%d].\n", ret);
      #endif

#ifdef XKB
      } else {
        FILE *file;
        XkbConfigRtrnRec config;

        int nxagentXkbConfigFilePathSize;

        char *nxagentXkbConfigFilePath;

        XkbComponentNamesRec names;
        char *rules, *variants, *options;

        #ifdef TEST
        fprintf(stderr, "nxagentKeyboardProc: Using XKB extension.\n");
        #endif

        memset(&names, 0, sizeof(XkbComponentNamesRec));

        rules = nxagentXkbGetRules();

        if ((nxagentKeyboard != NULL) && (strcmp(nxagentKeyboard, "query") !=0))
        {
          for (i = 0; nxagentKeyboard[i] != '/' && nxagentKeyboard[i] != 0; i++);

          if(nxagentKeyboard[i] == 0 || nxagentKeyboard[i + 1] == 0 || i == 0)
          {
            ErrorF("Warning: Wrong keyboard type: %s.\n",nxagentKeyboard);

            goto XkbError;
          }

          free_model = 1;
          model = malloc(i + 1);

          strncpy(model, nxagentKeyboard, i);

          model[i] = '\0';

          free_layout = 1;
          layout = malloc(strlen(&nxagentKeyboard[i + 1]) + 1);

          strcpy(layout, &nxagentKeyboard[i + 1]);

          /*
           * There is no description for pc105 on Solaris.
           * Need to revert to the closest approximation.
           */

          #ifdef TEST
          fprintf(stderr, "nxagentKeyboardProc: Using keyboard model [%s] with layout [%s].\n",
                      model, layout);
          #endif

          #ifdef __sun

          if (strcmp(model, "pc105") == 0)
          {
            #ifdef TEST
            fprintf(stderr, "nxagentKeyboardProc: WARNING! Keyboard model 'pc105' unsupported on Solaris.\n");

            fprintf(stderr, "nxagentKeyboardProc: WARNING! Forcing keyboard model to 'pc104'.\n");
            #endif

            strcpy(model, "pc104");
          }

          #endif
        }
        else
        {
          layout = XKB_DFLT_KB_LAYOUT;
          model = XKB_DFLT_KB_MODEL;

          #ifdef TEST
          fprintf(stderr, "nxagentKeyboardProc: Using default keyboard: model [%s] layout [%s].\n",
                      model, layout);
          #endif
        }

        variants = XKB_DFLT_KB_VARIANT;
        options = XKB_DFLT_KB_OPTIONS;

        #ifdef TEST
        fprintf(stderr, "nxagentKeyboardProc: XkbInitialMap [%s]\n", XkbInitialMap ? XkbInitialMap: "NULL");
        #endif

        if (XkbInitialMap) {
          if ((names.keymap = strchr(XkbInitialMap, '/')) != NULL)
            ++names.keymap;
          else
            names.keymap = XkbInitialMap;
        }

        #ifdef TEST
        fprintf(stderr, "nxagentKeyboardProc: Init XKB extension.\n");
        #endif

        if (XkbQueryExtension(nxagentDisplay,
                              &nxagentXkbInfo.Opcode,
                              &nxagentXkbInfo.EventBase,
                              &nxagentXkbInfo.ErrorBase,
                              &nxagentXkbInfo.MajorVersion,
                              &nxagentXkbInfo.MinorVersion) == 0)
        {
          ErrorF("Unable to initialize XKEYBOARD extension.\n");

          goto XkbError;
        }

        xkb = XkbGetKeyboard(nxagentDisplay, XkbGBN_AllComponentsMask, XkbUseCoreKbd);

        nxagentKeycodeConversionSetup();

        if (xkb == NULL || xkb->geom == NULL)
        {
          #ifdef TEST
          fprintf(stderr, "nxagentKeyboardProc: No current keyboard.\n");
          if (xkb == NULL)
          {
            fprintf(stderr, "nxagentKeyboardProc: xkb is null.\n");
          }
          else
          {
            fprintf(stderr, "nxagentKeyboardProc: xkb->geom is null.\n");
          }
          fprintf(stderr, "nxagentKeyboardProc: Going to set rules and init device.\n");
          #endif

          XkbSetRulesDflts(rules, model, layout, variants, options);
          XkbInitKeyboardDeviceStruct((pointer)pDev, &names, &keySyms, modmap,
                                          nxagentBell, nxagentChangeKeyboardControl);

          if (!nxagentKeyboard ||
                  (nxagentKeyboard && (strcmp(nxagentKeyboard, "query") == 0)))
          {
            goto XkbError;
          }

          goto XkbEnd;
        }

        XkbGetControls(nxagentDisplay, XkbAllControlsMask, xkb);

        nxagentXkbConfigFilePathSize = strlen(XKB_CONFIG_FILE);

        nxagentXkbConfigFilePath = malloc((nxagentXkbConfigFilePathSize + 1) * sizeof(char));

        if ( nxagentXkbConfigFilePath == NULL)
        {
          FatalError("nxagentKeyboardProc: malloc failed.");
        }

        strcpy(nxagentXkbConfigFilePath, XKB_CONFIG_FILE);
 
        #ifdef TEST
        fprintf(stderr, "nxagentKeyboardProc: nxagentXkbConfigFilePath [%s].\n",
                    nxagentXkbConfigFilePath);
        #endif

        if ((file = fopen(nxagentXkbConfigFilePath, "r")) != NULL) {

          #ifdef TEST
          fprintf(stderr, "nxagentKeyboardProc: Going to parse config file.\n");
          #endif

          if (XkbCFParse(file, XkbCFDflts, xkb, &config) == 0) {
            ErrorF("Error parsing config file.\n");

            free(nxagentXkbConfigFilePath);

            fclose(file);
            goto XkbError;
          }
          if (config.rules_file)
            rules = config.rules_file;
          if (config.model)
          {
            if (free_model)
            {
              free_model = 0; 
              free(model);
            }
            model = config.model;
          }
          if (config.layout)
          {
            if (free_layout)
            {
              free_layout = 0;
              free(layout);
            }
            layout = config.layout;
          }
          if (config.variant)
            variants = config.variant;
          if (config.options)
            options = config.options;

          free(nxagentXkbConfigFilePath);

          fclose(file);
        }
        else
        {
          #ifdef TEST
          fprintf(stderr, "nxagentKeyboardProc: No config file.\n");
          #endif

          #ifdef TEST
          fprintf(stderr, "nxagentKeyboardProc: No config file, going to set rules and init device.\n");
          #endif

          XkbSetRulesDflts(rules, model, layout, variants, options);
          XkbInitKeyboardDeviceStruct((pointer)pDev, &names, &keySyms, modmap,
                                          nxagentBell, nxagentChangeKeyboardControl);

          free(nxagentXkbConfigFilePath);

          if (!nxagentKeyboard ||
                 (nxagentKeyboard && (strcmp(nxagentKeyboard, "query") == 0)))
          {
            goto XkbError;
          }

          goto XkbEnd;
        }

        #ifdef TEST
        fprintf(stderr, "nxagentKeyboardProc: Going to set rules and init device.\n");
        #endif

        XkbSetRulesDflts(rules, model, layout, variants, options);
        XkbInitKeyboardDeviceStruct((pointer)pDev, &names, &keySyms, modmap,
                                    nxagentBell, nxagentChangeKeyboardControl);
        XkbDDXChangeControls((pointer)pDev, xkb->ctrls, xkb->ctrls);

XkbEnd:

        if (nxagentOption(Shadow) == 1 && pDev && pDev->key)
        {
          NXShadowInitKeymap(&(pDev->key->curKeySyms));
        }

        if (free_model) 
        {
          free_model = 0;
          free(model);
        }

        if (free_layout)
        {
          free_layout = 0;
          free(layout);
        }

        XkbFreeKeyboard(xkb, XkbAllComponentsMask, True);
        xkb = NULL;
      }
#endif

      #ifdef WATCH

      fprintf(stderr, "nxagentKeyboardProc: Watchpoint 10.\n");

/*
Reply   Total	Cached	Bits In			Bits Out		Bits/Reply	  Ratio
------- -----	------	-------			--------		----------	  -----
#1   U  3	2	80320 bits (10 KB) ->	28621 bits (3 KB) ->	26773/1 -> 9540/1	= 2.806:1
#98     1		256 bits (0 KB) ->	27 bits (0 KB) ->	256/1 -> 27/1	= 9.481:1
#101    1		32000 bits (4 KB) ->	2940 bits (0 KB) ->	32000/1 -> 2940/1	= 10.884:1
#119    1		384 bits (0 KB) ->	126 bits (0 KB) ->	384/1 -> 126/1	= 3.048:1
*/

      sleep(30);

      #endif

#ifdef _XSERVER64
      xfree(keymap);
#else
      XFree(keymap);
#endif
      break;
    case DEVICE_ON:

      #ifdef TEST
      fprintf(stderr, "nxagentKeyboardProc: Called for [DEVICE_ON].\n");
      #endif

      if (NXDisplayError(nxagentDisplay) == 1)
      {
        return Success;
      }

      #ifdef WATCH

      fprintf(stderr, "nxagentKeyboardProc: Watchpoint 11.\n");

/*
Reply   Total	Cached	Bits In			Bits Out		Bits/Reply	  Ratio
------- -----	------	-------			--------		----------	  -----
#117    1		320 bits (0 KB) ->	52 bits (0 KB) ->	320/1 -> 52/1	= 6.154:1
*/

      sleep(30);

      #endif

      nxagentEnableKeyboardEvents();

      break;

    case DEVICE_OFF:

      #ifdef TEST
      fprintf(stderr, "nxagentKeyboardProc: Called for [DEVICE_OFF].\n");
      #endif

      if (NXDisplayError(nxagentDisplay) == 1)
      {
        return Success;
      }

      nxagentDisableKeyboardEvents();

      break;

    case DEVICE_CLOSE:

      #ifdef TEST
      fprintf(stderr, "nxagentKeyboardProc: Called for [DEVICE_CLOSE].\n");
      #endif

      break;
  }

  return Success;
}

Bool LegalModifier(key, pDev)
     unsigned int key;
     DevicePtr pDev;
{
  return TRUE;
}

void nxagentNotifyKeyboardChanges(int oldMinKeycode, int oldMaxKeycode)
{
  #ifdef XKB

  if (!noXkbExtension)
  {
    DeviceIntPtr dev;
    XkbDescPtr xkb;
    xkbNewKeyboardNotify nkn;

    dev = inputInfo.keyboard;
    xkb = dev -> key -> xkbInfo -> desc;

    nkn.deviceID = nkn.oldDeviceID = dev -> id;
    nkn.minKeyCode = 8;
    nkn.maxKeyCode = 255;
    nkn.oldMinKeyCode = oldMinKeycode;
    nkn.oldMaxKeyCode = oldMaxKeycode;
    nkn.requestMajor = XkbReqCode;
    nkn.requestMinor = X_kbGetKbdByName;
    nkn.changed = XkbNKN_KeycodesMask;

    XkbSendNewKeyboardNotify(dev, &nkn);
  }
  else
  {

  #endif

    int i;
    xEvent event;

    event.u.u.type = MappingNotify;
    event.u.mappingNotify.request = MappingKeyboard;
    event.u.mappingNotify.firstKeyCode = inputInfo.keyboard -> key -> curKeySyms.minKeyCode;
    event.u.mappingNotify.count = inputInfo.keyboard -> key -> curKeySyms.maxKeyCode -
                                      inputInfo.keyboard -> key -> curKeySyms.minKeyCode;

    /*
     *  0 is the server client
     */

    for (i = 1; i < currentMaxClients; i++)
    {
      if (clients[i] && clients[i] -> clientState == ClientStateRunning)
      {
        event.u.u.sequenceNumber = clients[i] -> sequence;
        WriteEventsToClient(clients[i], 1, &event);
      }
    }

  #ifdef XKB

  }

  #endif

}

int nxagentResetKeyboard(void)
{
  DeviceIntPtr dev = inputInfo.keyboard;
  DeviceIntPtr devBackup;

  int result;
  int oldMinKeycode = 8;
  int oldMaxKeycode = 255;

  int savedBellPercent;
  int savedBellPitch;
  int savedBellDuration;

  if (NXDisplayError(nxagentDisplay) == 1)
  {
    return 0;
  }

  /*
   * Save bell settings.
   */

  savedBellPercent = inputInfo.keyboard -> kbdfeed -> ctrl.bell;
  savedBellPitch = inputInfo.keyboard -> kbdfeed -> ctrl.bell_pitch;
  savedBellDuration = inputInfo.keyboard -> kbdfeed -> ctrl.bell_duration;

  #ifdef TEST
  fprintf(stderr, "nxagentResetKeyboard: bellPercent [%d]  bellPitch [%d]  bellDuration [%d].\n",
              savedBellPercent, savedBellPitch, savedBellDuration);
  #endif

  devBackup = xalloc(sizeof(DeviceIntRec));

  if (devBackup == NULL)
  {
    #ifdef PANIC
    fprintf(stderr, "nxagentSaveKeyboardDeviceData: PANIC! Can't allocate backup structure.\n");
    #endif
  }
  else
  {
    memset(devBackup, 0, sizeof(DeviceIntRec));
  }

  nxagentSaveKeyboardDeviceData(dev, devBackup);

  if (dev->key)
  {
    #ifdef XKB
    if (noXkbExtension == 0 && dev->key->xkbInfo)
    {
      oldMinKeycode = dev->key->xkbInfo -> desc -> min_key_code;
      oldMaxKeycode = dev->key->xkbInfo -> desc -> max_key_code;
    }
    #endif

    dev->key=NULL;
  }

  dev->focus=NULL;

  dev->kbdfeed=NULL;

  #ifdef XKB
  nxagentTuneXkbWrapper();
  #endif

  result = (*inputInfo.keyboard -> deviceProc)(inputInfo.keyboard, DEVICE_INIT);

  if (result == Success && inputInfo.keyboard -> key != NULL)
  {

    /*
     * Restore bell settings.
     */

    inputInfo.keyboard -> kbdfeed -> ctrl.bell = savedBellPercent;
    inputInfo.keyboard -> kbdfeed -> ctrl.bell_pitch = savedBellPitch;
    inputInfo.keyboard -> kbdfeed -> ctrl.bell_duration = savedBellDuration;

    nxagentNotifyKeyboardChanges(oldMinKeycode, oldMaxKeycode);

    nxagentFreeKeyboardDeviceData(devBackup);

    free(devBackup);

    return 1;
  }
  else
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentResetKeyboard: Can't initialize the keyboard device.\n");
    #endif

    nxagentRestoreKeyboardDeviceData(devBackup, dev);

    return 0;
  }
}

void nxagentCheckAltMetaKeys(CARD8 keycode, int j)
{
  if (keycode == XKeysymToKeycode(nxagentDisplay, XK_Meta_L))
  {
    nxagentAltMetaMask |= 1 << j;
  }

  if (keycode == XKeysymToKeycode(nxagentDisplay, XK_Meta_R))
  {
    nxagentAltMetaMask |= 1 << j;
  }

  if (keycode == XKeysymToKeycode(nxagentDisplay, XK_Alt_L))
  {
    nxagentAltMetaMask |= 1 << j;
  }

  if (keycode == XKeysymToKeycode(nxagentDisplay, XK_Alt_R))
  {
    nxagentAltMetaMask |= 1 << j;
  }
}

void nxagentCheckRemoteKeycodes()
{
  nxagentCapsLockKeycode = XKeysymToKeycode(nxagentDisplay, XK_Caps_Lock);

  nxagentNumLockKeycode  = XKeysymToKeycode(nxagentDisplay, XK_Num_Lock);

  #ifdef DEBUG
  fprintf(stderr, "nxagentCheckRemoteKeycodes: Remote CapsLock keycode "
              "is [%d] NumLock [%d].\n", nxagentCapsLockKeycode,
                  nxagentNumLockKeycode);
  #endif
}

static int nxagentSaveKeyboardDeviceData(DeviceIntPtr dev, DeviceIntPtr devBackup)
{
  if (devBackup == NULL)
  {
    #ifdef PANIC
    fprintf(stderr, "nxagentSaveKeyboardDeviceData: PANIC! Pointer to backup structure is null.\n");
    #endif

    return -1;
  }

  devBackup -> key = dev -> key;

  devBackup -> focus = dev -> focus;

  devBackup -> kbdfeed = dev -> kbdfeed;

  #ifdef DEBUG
  fprintf(stderr, "nxagentSaveKeyboardDeviceData: Saved device data.\n");
  #endif

  return 1;
}

static int nxagentRestoreKeyboardDeviceData(DeviceIntPtr devBackup, DeviceIntPtr dev)
{
  if (devBackup == NULL)
  {
    #ifdef PANIC
    fprintf(stderr, "nxagentRestoreKeyboardDeviceData: PANIC! Pointer to backup structure is null.\n");
    #endif

    return -1;
  }

  dev -> key = devBackup -> key;

  dev -> focus = devBackup -> focus;

  dev -> kbdfeed = devBackup -> kbdfeed;

  #ifdef DEBUG
  fprintf(stderr, "nxagentRestoreKeyboardDeviceData: Restored device data.\n");
  #endif

  return 1;
}


static int nxagentFreeKeyboardDeviceData(DeviceIntPtr dev)
{
  KbdFeedbackPtr k, knext;

  if (dev == NULL)
  {
    #ifdef PANIC
    fprintf(stderr, "nxagentFreeKeyboardDeviceData: PANIC! Pointer to device structure is null.\n");
    #endif

    return -1;
  }

  if (dev->key)
  {
      #ifdef XKB
      if (noXkbExtension == 0 && dev->key->xkbInfo)
      {
          XkbFreeInfo(dev->key->xkbInfo);
          dev->key->xkbInfo = NULL;
      }
      #endif

      xfree(dev->key->curKeySyms.map);
      xfree(dev->key->modifierKeyMap);
      xfree(dev->key);

      dev->key=NULL;
  }

  if (dev->focus)
  {
      xfree(dev->focus->trace);
      xfree(dev->focus);
      dev->focus=NULL;
  }

  for (k = dev->kbdfeed; k; k = knext)
  {
      knext = k->next;
      #ifdef XKB
      if (k->xkb_sli)
          XkbFreeSrvLedInfo(k->xkb_sli);
      #endif
      xfree(k);
  }

  #ifdef DEBUG
  fprintf(stderr, "nxagentFreeKeyboardDeviceData: Freed device data.\n");
  #endif

  return 1;
}

#if XKB

int ProcXkbInhibited(register ClientPtr client)
{
  unsigned char majorop;
  unsigned char minorop;

  #ifdef TEST
  fprintf(stderr, "ProcXkbInhibited: Called.\n");
  #endif

  majorop = ((xReq *)client->requestBuffer)->reqType;

  #ifdef PANIC
  if (majorop != (unsigned char)nxagentXkbWrapper.base)
  {
    fprintf(stderr, "ProcXkbInhibited: MAJOROP is [%d] but should be [%d].\n",
            majorop, nxagentXkbWrapper.base);
  }
  #endif

  minorop = *((unsigned char *) client->requestBuffer + 1);

  #ifdef TEST
  fprintf(stderr, "ProcXkbInhibited: MAJOROP is [%d] MINOROP is [%d].\n",
              majorop, minorop);
  #endif

  switch (minorop)
  {
    case X_kbLatchLockState:
    case X_kbSetControls:
    case X_kbSetCompatMap:
    case X_kbSetIndicatorMap:
    case X_kbSetNamedIndicator:
    case X_kbSetNames:
    case X_kbSetGeometry:
    case X_kbSetDebuggingFlags:
    case X_kbSetMap:
    {
      return client->noClientException;
    }
    case X_kbGetKbdByName:
    {
      return BadAccess;
    }
    default:
    {
      return (client->swapped ? nxagentXkbWrapper.SProcXkbDispatchBackup(client) :
                  nxagentXkbWrapper.ProcXkbDispatchBackup(client));
    }
  }
}

void nxagentInitXkbWrapper(void)
{
  ExtensionEntry * extension;

  #ifdef TEST
  fprintf(stderr, "nxagentInitXkbWrapper: Called.\n");
  #endif

  if (nxagentOption(InhibitXkb) == 0)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentInitXkbWrapper: Nothing to do.\n");
    #endif

    return;
  }

  memset(&nxagentXkbWrapper, 0, sizeof(XkbWrapperRec));

  extension = CheckExtension("XKEYBOARD");

  if (extension != NULL)
  {
    nxagentXkbWrapper.base = extension -> base;
    nxagentXkbWrapper.eventBase = extension -> eventBase;
    nxagentXkbWrapper.errorBase = extension -> errorBase;
    nxagentXkbWrapper.ProcXkbDispatchBackup  = NULL;
    nxagentXkbWrapper.SProcXkbDispatchBackup = NULL;

    #ifdef TEST
    fprintf(stderr, "nxagentInitXkbWrapper: base [%d] eventBase [%d] errorBase [%d].\n",
                extension -> base, extension -> eventBase, extension -> errorBase);
    #endif
  }
  else
  {
    nxagentXkbWrapper.base = -1;

    #ifdef TEST
    fprintf(stderr, "nxagentInitXkbWrapper: XKEYBOARD extension not found.\n");
    #endif
  }
}

void nxagentDisableXkbExtension(void)
{  
  #ifdef TEST
  fprintf(stderr, "nxagentDisableXkbExtension: Called.\n");
  #endif

  if (nxagentXkbWrapper.base > 0)
  {
    if (nxagentXkbWrapper.ProcXkbDispatchBackup == NULL)
    {
      nxagentXkbWrapper.ProcXkbDispatchBackup  = ProcVector[nxagentXkbWrapper.base];

      ProcVector[nxagentXkbWrapper.base] = ProcXkbInhibited;
    }
    #ifdef TEST
    else
    {
      fprintf(stderr, "nxagentDisableXkbExtension: Nothing to be done for ProcXkbDispatch.\n");
    }
    #endif

    if (nxagentXkbWrapper.SProcXkbDispatchBackup == NULL)
    {
      nxagentXkbWrapper.SProcXkbDispatchBackup = SwappedProcVector[nxagentXkbWrapper.base];

      SwappedProcVector[nxagentXkbWrapper.base] = ProcXkbInhibited;
    }
    #ifdef TEST
    else
    {
      fprintf(stderr, "nxagentDisableXkbExtension: Nothing to be done for SProcXkbDispatch.\n");
    }
    #endif
  }
}

void nxagentEnableXkbExtension(void)
{
  #ifdef TEST
  fprintf(stderr, "nxagentEnableXkbExtension: Called.\n");
  #endif

  if (nxagentXkbWrapper.base > 0)
  {
    if (nxagentXkbWrapper.ProcXkbDispatchBackup != NULL)
    {
      ProcVector[nxagentXkbWrapper.base] = nxagentXkbWrapper.ProcXkbDispatchBackup;

      nxagentXkbWrapper.ProcXkbDispatchBackup = NULL;
    }
    #ifdef TEST
    else
    {
      fprintf(stderr, "nxagentEnableXkbExtension: Nothing to be done for ProcXkbDispatch.\n");
    }
    #endif

    if (nxagentXkbWrapper.SProcXkbDispatchBackup != NULL)
    {
      SwappedProcVector[nxagentXkbWrapper.base] = nxagentXkbWrapper.SProcXkbDispatchBackup;

      nxagentXkbWrapper.SProcXkbDispatchBackup = NULL;
    }
    #ifdef TEST
    else
    {
      fprintf(stderr, "nxagentEnableXkbExtension: Nothing to be done for SProcXkbDispatch.\n");
    }
    #endif
  }
}

void nxagentTuneXkbWrapper(void)
{
  if (nxagentOption(InhibitXkb) == 0)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentTuneXkbWrapper: Nothing to do.\n");
    #endif

    return;
  }

  if (nxagentKeyboard != NULL && 
          strcmp(nxagentKeyboard, "query") == 0)
  {
    nxagentDisableXkbExtension();
  }
  else
  {
    nxagentEnableXkbExtension();
  }
}

static int nxagentXkbGetNames(char **rules, char **model, char **layout,
                                  char **variant, char **options)
{
  Atom atom;
  #ifdef _XSERVER64
  Atom64 type;
  #else
  Atom type;
  #endif
  int format;
  unsigned long n;
  unsigned long after;
  char *data;
  char *name;
  Status result;

  data = name = NULL;

  *rules = NULL;
  *model = NULL;
  *layout = NULL;
  *variant = NULL;
  *options = NULL;

  atom = XInternAtom(nxagentDisplay, "_XKB_RULES_NAMES", 1);

  if (atom == 0)
  {
    return 0; 
  }

  result = XGetWindowProperty(nxagentDisplay, DefaultRootWindow(nxagentDisplay),
                                  atom, 0, 256, 0, XA_STRING, &type, &format,
                                      &n, &after, (unsigned char **)&data);

  if (result !=Success || data == NULL)
  {
    return 0;
  }

  if ((after > 0) || (type != XA_STRING) || (format != 8))
  {
    if (data != NULL)
    {
      XFree(data);
      return 0;
    }
  }

  name = data;

  if (name < data + n)
  {
    *rules =  name;
    name += strlen(name) + 1;
  }

  if (name < data + n)
  {
    *model = name;
    name += strlen(name) + 1;
  }

  if (name < data + n)
  {
    *layout = name;
    name += strlen(name) + 1;
  }

  if (name < data + n)
  {
    *variant = name;
    name += strlen(name) + 1;
  }

  if (name < data + n)
  {
    *options = name;
    name += strlen(name) + 1;
  }

  return n;
}

void nxagentKeycodeConversionSetup(void)
{
  char *drules = 0;
  char *dmodel = 0;
  char *dlayout = 0;
  char *dvariant = 0;
  char *doptions = 0;
  unsigned int drulesLen;

  nxagentKeycodeConversion = 0;

  drulesLen = nxagentXkbGetNames(&drules, &dmodel, &dlayout,
                                     &dvariant, &doptions);

  #ifdef DEBUG
  if (drulesLen != 0 && drules != NULL && dmodel != NULL)
  {
    fprintf(stderr, "nxagentKeycodeConversionSetup: "
                "Remote: [%s,%s,%s,%s,%s].\n", drules, dmodel, dlayout,
                    dvariant, doptions);
  }
  else
  {
    fprintf(stderr, "nxagentKeycodeConversionSetup: "
                "Failed to retrieve remote rules.\n");
  }
  #endif

  if (drulesLen != 0)
  {
    char *sessionpath = nxagentGetSessionPath();
    if (sessionpath != NULL){
      int keyboard_file_path_size = strlen(sessionpath) + strlen("/keyboard");
      char *keyboard_file_path = malloc((keyboard_file_path_size + 1) * sizeof(char));
      FILE *keyboard_file;
      if ( keyboard_file_path == NULL)
      {
        FatalError("nxagentKeyboardProc: malloc failed.");
      }
      strcpy(keyboard_file_path, sessionpath);
      strcat(keyboard_file_path, "/keyboard");
      if ((keyboard_file = fopen(keyboard_file_path, "w")) != NULL) {
        if ( drules != NULL )
          fprintf(keyboard_file, "rules=%s\n", drules);
        if ( dmodel != NULL )
          fprintf(keyboard_file, "model=%s\n", dmodel);
        if ( dlayout != NULL )
          fprintf(keyboard_file, "layout=%s\n", dlayout);
        if ( dvariant != NULL )
          fprintf(keyboard_file, "variant=%s\n", dvariant);
        if ( doptions != NULL )
          fprintf(keyboard_file, "options=%s\n", doptions);
        fclose(keyboard_file);
      }
      free(keyboard_file_path);
      fprintf(stderr, "keyboard file created\n");
    }
    fprintf(stderr, "SessionPath not defined\n");
  }
  else
  {
    fprintf(stderr, "Failed to create the keyboard file\n");
  }

  if (nxagentOption(ClientOs) == ClientOsLinux &&
            drules != NULL && dmodel != NULL &&
                (strcmp(drules, "evdev") == 0 ||
                    strcmp(dmodel, "evdev") == 0))
  {
    nxagentKeycodeConversion = 1;
  }

  if (drules != NULL)
  {
    XFree(drules);
  }
}

void nxagentResetKeycodeConversion(void)
{
  int result;
  XkbAgentInfoRec info;

  result = XkbQueryExtension(nxagentDisplay, &info.Opcode, &info.EventBase,
                                 &info.ErrorBase, &info.MajorVersion,
                                     &info.MinorVersion);

  if (result != 0)
  {
    nxagentKeycodeConversionSetup();
  }
  else
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentResetKeycodeConversion: "
                "WARNING! Failed to query XKB extension.\n");
    #endif

    nxagentKeycodeConversion = 0;
  }
}

#endif
