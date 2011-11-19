/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2011 NoMachine, http://www.nomachine.com/.         */
/*                                                                        */
/* NXCOMPSHAD, NX protocol compression and NX extensions to this software */
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

#ifdef __CYGWIN32__

#ifndef Win32Poller_H
#define Win32Poller_H

//#include <X11/X.h>

#include <Windows.h>
#include <wingdi.h>
#include <winable.h>
#include <winuser.h>

#define CAPTUREBLT 0x40000000

#define KEYEVENTF_SCANCODE 0x00000008
#define MAPVK_VSC_TO_VK_EX 3
//
// The CAPTUREBLT is a raster operation used
// in bit blit transfer.
//
// Using this operation includes any windows
// that are layered on top of your window in
// the resulting image. By default, the image
// only contains your window.
//

#include "Core.h"

typedef struct _keyTranslation
{
 unsigned char scancode;
 unsigned short modifiers;

}keyTranslation;

class Poller : public CorePoller
{
  public:

  Display *display_;
  keyTranslation *keymap_;
  unsigned char keymapLoaded_;
  int minKeycode_;

  Poller(Input *, Display *display, int = 16);

  ~Poller();

  int init();

  int updateCursor(Window, Visual*);

  private:

  
  int Poller::updateShadowFrameBuffer(void);
  void handleKeyboardEvent(Display *, XEvent *);
  void handleWebKeyboardEvent(KeySym keysym, Bool isKeyPress);
  void addToKeymap(char *keyname, unsigned char scancode, unsigned short modifiers, char *mapname);
  int xkeymapRead(char *mapname);
  FILE *xkeymapOpen(char *filename);
  void xkeymapInit(char *keyMapName);
  keyTranslation xkeymapTranslateKey(unsigned int keysym, unsigned int keycode, unsigned int state);
  unsigned char getKeyState(unsigned int state, unsigned int keysym);
  char *getKsname(unsigned int keysym);
  unsigned char specialKeys(unsigned int keysym, unsigned int state, int pressed);

  unsigned char toggleSwitch(unsigned char ToggleStateClient, unsigned char ToggleStateServer, UINT scancode,
                                 int *lengthArrayINPUT);

  void updateModifierState(UINT, unsigned char);

  unsigned char toggleServerState(UINT scancode);
  unsigned char keyState(UINT scancode, UINT mapType);
  unsigned char keyStateAsync(UINT scancode);

  void handleMouseEvent(Display *, XEvent *);

  Cursor createCursor(Window wnd, Visual *vis, unsigned int x, unsigned int y, int width,
                           int height, unsigned char *xormask, unsigned char *andmask);

  Pixmap createGlyph(Window wnd, Visual *visual, int width, int height, unsigned char *data);

  char isWinNT();
  char selectDesktop(HDESK new_desktop);
  char selectDesktopByName(char *name);
  void platformOS();
  char simulateCtrlAltDel(void);
  DWORD platformID_;

  INPUT *pKey_, *pMouse_;

  char *keymapName_;
  char *path_;

  unsigned char toggleButtonState_;
  unsigned short serverModifierState_;
  unsigned short savedServerModifierState_;

  void ensureServerModifiers(keyTranslation tr, int *lenghtArrayINPUT);
  void restoreServerModifiers(UINT scancode);
  unsigned char isModifier(UINT scancode);

  char sendInput(unsigned char scancode, unsigned char pressed, int *lengthArrayINPUT);

  char *getRect(XRectangle);
  char checkDesktop();

  char *DIBBuffer_;

  HCURSOR oldCursor_;

  VOID *pDIBbits_;
  HDC screenDC_;
  HDC memoryDC_;
  BITMAPINFO bmi_;
  HBITMAP screenBmp_;

  Cursor xCursor_;

};

#undef TEST

inline unsigned char Poller::toggleSwitch(unsigned char ToggleStateClient, unsigned char ToggleStateServer,
                                              UINT scancode, int *lengthArrayINPUT)
{
  return 1;
}

inline unsigned char Poller::toggleServerState(UINT scancode)
{
  return (GetKeyState(MapVirtualKeyEx(scancode, 3, GetKeyboardLayout((DWORD)NULL))) & 0x1);
}

inline unsigned char Poller::keyStateAsync(UINT vKeycode)
{
  return GetAsyncKeyState(vKeycode);
}

inline unsigned char Poller::keyState(UINT code, UINT mapType)
{
  if (mapType == 0)
  {
    //
    // Virtual Keycode
    //
    return ((GetKeyState(code) & 0x80) == 0x80);
  }
  else
  {
    //
    // scancode
    //
    return ((GetKeyState(MapVirtualKeyEx(code, 3, GetKeyboardLayout((DWORD)NULL))) & 0x80) == 0x80);
  }
}

inline char Poller::isWinNT()
{
  return (platformID_ == VER_PLATFORM_WIN32_NT);
}

inline char Poller::sendInput(unsigned char scancode, unsigned char pressed, int *lengthArrayINPUT)
{
  DWORD keyEvent = 0;
  DWORD extended = 0;

  if (scancode > 0)
  {
    if (pressed == 0)
    {
      keyEvent = KEYEVENTF_KEYUP;
    }

    if (scancode & 0x80)
    {
      scancode &= ~0x80;
      extended = KEYEVENTF_EXTENDEDKEY;
    }

    pKey_[*lengthArrayINPUT].ki.wScan = (WORD) scancode;
    pKey_[*lengthArrayINPUT].ki.dwFlags = (DWORD) (keyEvent | KEYEVENTF_SCANCODE | extended);
    (*lengthArrayINPUT)++;
  }


  if (*lengthArrayINPUT > 0)                                                                                                   {
    // FIXME: Remove me.
    logTest("Poller::sendInput", "length Input [%d] event: %s", *lengthArrayINPUT,
              pressed == 1 ? "KeyPress": "KeyRelease");

    if (SendInput(*lengthArrayINPUT, pKey_, sizeof(INPUT)) == 0)
    {
      logTest("Poller::sendInput", "Failed SendInput, event: %s", pressed == 1 ? "KeyPress": "KeyRelease");
      *lengthArrayINPUT = 0;
      return 0;
    }

    *lengthArrayINPUT = 0;
  }

  return 1;
}
#endif /* Win32Poller_H */

#endif /* __CYGWIN32__ */
