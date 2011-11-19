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

#if defined(__CYGWIN32__) || defined(WIN32)

#include <X11/keysym.h>

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

#include "Poller.h"
#include "Logger.h"

Poller::Poller(Input *input, Display *display, int depth) : CorePoller(input, display)
{
  logTrace("Poller::Poller");

  screenDC_ = NULL;
  screenBmp_ = NULL;
  memoryDC_ = NULL;
  pDIBbits_ = NULL;
  DIBBuffer_ = NULL;
  pKey_ = NULL;
  pMouse_ = NULL;
  path_ = NULL;
  keymapName_ = input -> getKeymap();
  keymap_ = NULL;
  toggleButtonState_ = 0;
  serverModifierState_ = 0;
  display_ = display;
  depth_ = DefaultDepth(display_, DefaultScreen(display_));
  oldCursor_ = 0;
  xCursor_ = 0;
}

Poller::~Poller()
{
  logTrace("Poller::~Poller");

  if (screenDC_)
  {
    BOOL result = ReleaseDC(NULL, screenDC_);

    logTest("Poller::~Poller", "ReleaseDC returned [%d].", result);

    screenDC_ = NULL;
  }

  if (memoryDC_)
  {
    BOOL result = DeleteDC(memoryDC_);

    logTest("Poller::~Poller", "DeleteDC returned [%d].", result);

    memoryDC_ = NULL;
  }

  if (screenBmp_)
  {
    BOOL result = DeleteObject(screenBmp_);

    logTest("Poller::~Poller", "DeleteObject returned [%d].", result);

    screenBmp_ = NULL;
  }

  if (DIBBuffer_)
  {
    logDebug("Poller::~Poller", "Delete DIBBuffer_ [%p].", DIBBuffer_);

    delete [] DIBBuffer_;
  }

  if (pKey_)
  {
    logDebug("Poller::~Poller", " pKey_[%p].", pKey_);

    delete [] pKey_;
  }

  if (pMouse_)
  {
    logDebug("Poller::~Poller", " pMouse_[%p].", pMouse_);

    delete [] pMouse_;
  }

  if (keymap_)
  {
    logDebug("Poller::~Poller", " keymap_[%p].", keymap_);

    delete [] keymap_;
  }
}

int Poller::init()
{
  logTrace("Poller::init");

  int maxLengthArrayINPUT = 6;

  platformOS();

  pKey_ = new INPUT [maxLengthArrayINPUT];

  if (pKey_ == NULL)
  {
    logError("Poller::init", ESET(ENOMEM));

    return -1;
  }

  for (int i = 0; i < maxLengthArrayINPUT; i++)
  {
    pKey_[i].type = INPUT_KEYBOARD;
    pKey_[i].ki.wVk = (WORD) 0;
    pKey_[i].ki.time = (DWORD) 0;
    pKey_[i].ki.dwExtraInfo = (DWORD) 0;
  }

  pMouse_ = new INPUT;

  if (pMouse_ == NULL)
  {
    logError("Poller::init", ESET(ENOMEM));

    return -1;
  }

  pMouse_ -> type = INPUT_MOUSE;

  pMouse_ -> mi.dx = 0;
  pMouse_ -> mi.dy = 0;
  pMouse_ -> mi.mouseData = (DWORD) 0;
  pMouse_ -> mi.time = 0;
  pMouse_ -> mi.dwExtraInfo = (ULONG_PTR) NULL;

  screenDC_ = GetDC(NULL);

  if (screenDC_ == NULL)
  {
    logError("Poller::init", ESET(ENOMSG));

    return -1;
  }

  switch(depth_)
  {
    case 8:
    {
      depth_ = 16;
      break;
    }
    case 16:
    {
      depth_ = 16;
      break;
    }
    case 24:
    {
      depth_ = 32;
      break;
    }
    default:
    {
      logError("Poller::init", ESET(EINVAL));

      return -1;
    }
  }

  width_ = GetDeviceCaps(screenDC_, HORZRES);
  height_ = GetDeviceCaps(screenDC_, VERTRES);

  bpl_ = width_ * (depth_ >> 3);
  bpp_ = (depth_ >> 3);

  logTest("Poller::init", "Screen geometry is [%d, %d] depth is [%d] bpl [%d] bpp [%d].",
              width_, height_, depth_, bpl_, bpp_);

  logTest("Poller::init", "Got device context at [%p] screen size is (%d,%d).",
            screenDC_, width_, height_);

  memoryDC_ = CreateCompatibleDC(screenDC_);

  if (memoryDC_ == NULL)
  {
    logError("Poller::init", ESET(ENOMSG));

    return -1;
  }

  //
  // Delete the old bitmap for the memory device.
  //

  HBITMAP bitmap = (HBITMAP) GetCurrentObject(memoryDC_, OBJ_BITMAP);

  if (bitmap && DeleteObject(bitmap) == 0)
  {
    logError("Poller::init", ESET(ENOMSG));
  }

  //
  // Bitmap header describing the bitmap we want to get from GetDIBits.
  //

  bmi_.bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
  bmi_.bmiHeader.biWidth         = width_;
  bmi_.bmiHeader.biHeight        = -height_;
  bmi_.bmiHeader.biPlanes        = 1;
  bmi_.bmiHeader.biBitCount      = depth_;
  bmi_.bmiHeader.biCompression   = BI_RGB;
  bmi_.bmiHeader.biSizeImage     = 0;
  bmi_.bmiHeader.biXPelsPerMeter = 0;
  bmi_.bmiHeader.biYPelsPerMeter = 0;
  bmi_.bmiHeader.biClrUsed       = 0;
  bmi_.bmiHeader.biClrImportant  = 0;

  screenBmp_ = CreateDIBSection(memoryDC_, &bmi_, DIB_RGB_COLORS, &pDIBbits_, NULL, 0);
  ReleaseDC(NULL,memoryDC_);

  if (screenBmp_ == NULL)
  {
    logTest ("Poller::init", "This video device is not supporting DIB section");

    pDIBbits_ = NULL;

    screenBmp_ = CreateCompatibleBitmap(screenDC_, width_, height_);

    if (screenBmp_ == NULL)
    {
      logError("Poller::init", ESET(ENOMSG));

      return -1;
    }

    if (SelectObject(memoryDC_, screenBmp_) == NULL)
    {
      logError("Poller::init", ESET(ENOMSG));

      return -1;
    }
  }
  else
  {
    logTest ("Poller::init", "Enabled the DIB section");

    if (SelectObject(memoryDC_, screenBmp_) == NULL)
    {
      logError("Poller::init", ESET(ENOMSG));

      return -1;
    }
  }

  //
  // Check if the screen device raster capabilities
  // support the bitmap transfer.
  //

  if ((GetDeviceCaps(screenDC_, RASTERCAPS) & RC_BITBLT) == 0)
  {
    logTest("Poller::init", "This video device is not supporting the bitmap transfer.");

    logError("Poller::init", ESET(ENOMSG));

    return -1;
  }

  //
  // Check if the memory device raster capabilities
  // support the GetDIBits and SetDIBits functions.
  //

  if ((GetDeviceCaps(memoryDC_, RASTERCAPS) & RC_DI_BITMAP) == 0)
  {
    logTest("Poller::init", "This memory device is not supporting the GetDIBits and SetDIBits "
               "function.");

    logError("Poller::init", ESET(ENOMSG));

    return -1;
  }

  if (GetDeviceCaps(screenDC_, PLANES) != 1)
  {
    logTest("Poller::init", "This video device has more than 1 color plane.");

    logError("Poller::init", ESET(ENOMSG));

    return -1;
  }

  return CorePoller::init();
}

//
// FIXME: Remove me.
//

void ErrorExit(LPTSTR lpszFunction)
{
    LPVOID lpMsgBuf;
    DWORD dw = GetLastError();

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );

    logTest(lpszFunction, " Failed with error [%ld]: %s", dw, (char*)lpMsgBuf);

    LocalFree(lpMsgBuf);
    ExitProcess(dw);
}

//
// FIXME: End.
//

char *Poller::getRect(XRectangle r)
{
  logTrace("Poller::getRect");

  logDebug("Poller::getRect", "Going to retrive rectangle [%d, %d, %d, %d].",
              r.x, r.y, r.width, r.height);

  //
  // The CAPTUREBLT operation could be a very
  // cpu-consuming task. We should make some
  // test to see how much it is expensive.
  // Anyway we get tooltip windows and any
  // other special effect not included with
  // only the SRCCOPY operation.
  //

  if (BitBlt(memoryDC_, r.x, r.y, r.width, r.height,
                 screenDC_, r.x, r.y, SRCCOPY | CAPTUREBLT) == 0)
  {
    logError("Poller::getRect", ESET(ENOMSG));

    logTest("Poller::getRect", "Failed to perform a bit-block transfer.");
    logTest("Poller::getRect", "bit-block error=%lu", GetLastError());

    return NULL;
  }

  // bmi_.bmiHeader.biWidth = r.width;
  // bmi_.bmiHeader.biHeight = -r.height;

  if (pDIBbits_ == NULL)
  {
    static long nPixel = 0;

    if (nPixel < r.width * r.height)
    {

      if (DIBBuffer_)
      {
        delete [] DIBBuffer_;
      }

      nPixel = r.width * r.height;

      DIBBuffer_ = new char [nPixel * bpp_];

      if (DIBBuffer_ == NULL)
      {
        logError("Poller::getRect", ESET(ENOMEM));

        nPixel = 0;

        return NULL;
      }
    }

    if (GetDIBits(memoryDC_, screenBmp_, height_ - r.height - r.y, r.height,
                      DIBBuffer_, &bmi_, DIB_RGB_COLORS) == 0)
    {
      logError("Poller::getRect", ESET(ENOMSG));

      logTest("Poller::getRect", "Failed to retrieve the screen bitmap.");

      return NULL;
    }

    return DIBBuffer_;
  }
  else
  {
    return (char *) pDIBbits_ + r.y * bpl_ + r.x * bpp_;
  }
}

void Poller::handleKeyboardEvent(Display *display, XEvent *event)
{
  KeySym keysym;
  char *keyname = new char [31];
  keyTranslation tr = {0, 0};
  unsigned char scancode = 0;
  int lengthArrayINPUT = 0;

  if (XLookupString((XKeyEvent *) event, keyname, 30, &keysym, NULL) > 0)
  {
    logTest("Poller::handleKeyboardEvent", "keyname %s, keysym [%x]", keyname, (unsigned int)keysym);
  }

  if (specialKeys(keysym, event -> xkey.state, event -> type) == 1)
  {
    delete[] keyname;
    return;
  }

  tr = xkeymapTranslateKey(keysym, event -> xkey.keycode, event -> xkey.state);
  scancode = tr.scancode;

  logTest("Poller::handleKeyboardEvent", "keyname [%s] scancode [0x%x], keycode[0x%x], keysym [%x]", keyname,
              tr.scancode, event ->xkey.keycode, (unsigned int)keysym);

  if (scancode == 0)
  {
    delete[] keyname;
    return;
  }

  if (event -> type == KeyPress)
  {
    int test1 = MapVirtualKey(scancode, MAPVK_VSC_TO_VK_EX);
    int test2 = MapVirtualKey(0x24, MAPVK_VSC_TO_VK_EX);

    if (test1 == test2)
    {
      simulateCtrlAltDel();
    }

    if (isModifier(scancode) == 0)
    {
      savedServerModifierState_ = serverModifierState_;
    }

    ensureServerModifiers(tr, &lengthArrayINPUT);
    if (sendInput(scancode, 1, &lengthArrayINPUT) == 0)
    {
      logTest("Poller::handleKeyboardEvent", "lengthArrayINPUT [%d].", lengthArrayINPUT);
    }
    restoreServerModifiers(scancode);
  }
  else if (event -> type == KeyRelease)
  {
    if (sendInput(scancode, 0, &lengthArrayINPUT) == 0)
    {
      logTest("Poller::handleKeyboardEvent", "lengthArrayINPUT [%d].", lengthArrayINPUT);
    }
  }

  updateModifierState(scancode, (event -> type == KeyPress));

  delete[] keyname;
}

void Poller::handleWebKeyboardEvent(KeySym keysym, Bool isKeyPress)
{
/*
FIXME
*/
}

void Poller::handleMouseEvent(Display *display, XEvent *event)
{
  DWORD flg = 0;
  DWORD whl = 0;

  if (event -> type == ButtonPress)
  {
    logTest("Poller::handleMouseEvent", "ButtonPress.\n");
    switch (event -> xbutton.button)
    {
      case Button1:
      {
        flg = MOUSEEVENTF_LEFTDOWN;
        break;
      }
      case Button2:
      {
        flg = MOUSEEVENTF_MIDDLEDOWN;
        break;
      }
      case Button3:
      {
        flg = MOUSEEVENTF_RIGHTDOWN;
        break;
      }
      case Button4:
      {
        flg = MOUSEEVENTF_WHEEL;
        whl = WHEEL_DELTA;
        pMouse_ -> mi.mouseData = whl;
        break;
      }
      case Button5:
      {
        flg = MOUSEEVENTF_WHEEL;
        whl = (DWORD) (-WHEEL_DELTA);
        pMouse_ -> mi.mouseData = whl;
        break;
      }
    }
  }
  else if (event -> type == ButtonRelease)
  {
    switch (event -> xbutton.button)
    {
      case Button1:
      {
        flg = MOUSEEVENTF_LEFTUP;
        break;
      }
      case Button2:
      {
        flg = MOUSEEVENTF_MIDDLEUP;
        break;
      }
      case Button3:
      {
        flg = MOUSEEVENTF_RIGHTUP;
        break;
      }
      case Button4:
      {
        flg = MOUSEEVENTF_WHEEL;
        whl = 0;
        pMouse_ -> mi.mouseData = whl;
        break;
      }
      case Button5:
      {
        flg = MOUSEEVENTF_WHEEL;
        whl = 0;
        pMouse_ -> mi.mouseData = whl;
        break;
      }
    }
  }
  else if (event -> type == MotionNotify)
  {
    logTest("Poller::handleMouseEvent", "SetCursor - MotionNotify");

    SetCursorPos(event -> xmotion.x, event -> xmotion.y);
  }

  if (flg > 0)
  {
  //  logTest("Poller::handleMouseEvent", "SetCursor - flg > 0");
    //
    // FIXME: move the cursor to the pace the event occurred
    //

    SetCursorPos(event -> xbutton.x, event -> xbutton.y);

    //
    // FIXME: Remove me: send the click/release event
    // mouse_event(flg, 0, 0, whl, (ULONG_PTR)NULL);
    //

    pMouse_ -> mi.dwFlags = flg;

    if (SendInput(1, pMouse_, sizeof(INPUT)) == 0)
    {
      logTest("Poller::handleMouseEvent", "Failed SendInput");
    }
  }
}

int Poller::updateCursor(Window wnd, Visual* vis)
{
  BYTE *mBits, *andBits, *xorBits;

  logTrace("Poller::Cursor");

  //
  // Retrieve mouse cursor handle.
  //

  CURSORINFO cursorInfo;
  cursorInfo.cbSize = sizeof(CURSORINFO);

  if (GetCursorInfo(&cursorInfo) == 0)
  {
    logTest("Poller::Cursor", "GetCursorInfo() failed [%u].\n", (unsigned int)GetLastError());
    LocalFree(&cursorInfo);
    return -1;
  }

  HCURSOR hCursor = cursorInfo.hCursor;

  if (hCursor == 0)
  {
    logTest("Poller::Cursor","Cursor Handle is NULL. Error[%u].\n", (unsigned int)GetLastError());
    return 1;
  }

  if (hCursor == oldCursor_)
  {
    LocalFree(&cursorInfo);
    return 1;
  }
  else
  {
    oldCursor_ = hCursor;
  }

  //
  // Get cursor info.
  //

  //  logTest("Poller::Cursor","hCursor [%xH] GetCursor [%xH].\n", hCursor, GetCursor());

  ICONINFO iconInfo;
  if (GetIconInfo(hCursor, &iconInfo) == 0)
  {
    logTest("Poller::Cursor","GetIconInfo() failed. Error[%d].", (unsigned int)GetLastError());
    LocalFree(&iconInfo);
    //    return -1;
  }

  BOOL isColorCursor = FALSE;
  if (iconInfo.hbmColor != NULL)
  {
    isColorCursor = TRUE;
  }

  if (iconInfo.hbmMask == NULL)
  {
    logTest("Poller::Cursor","Cursor bitmap handle is NULL.\n");
    return -1;
  }

  //
  // Check bitmap info for the cursor
  //

  BITMAP bmMask;
  if (!GetObject(iconInfo.hbmMask, sizeof(BITMAP), (LPVOID)&bmMask))
  {
    logTest("Poller::Cursor","GetObject() for bitmap failed.\n");
    DeleteObject(iconInfo.hbmMask);
    LocalFree(&bmMask);
    return -1;
  }

  if (bmMask.bmPlanes != 1 || bmMask.bmBitsPixel != 1)
  {
    logTest("Poller::Cursor","Incorrect data in cursor bitmap.\n");
    LocalFree(&bmMask);
    DeleteObject(iconInfo.hbmMask);
    return -1;
  }

  // Get monochrome bitmap data for cursor
  // NOTE: they say we should use GetDIBits() instead of GetBitmapBits().
  mBits = new BYTE[bmMask.bmWidthBytes * bmMask.bmHeight];

  if (mBits == NULL)//Data bitmap
  {
    DeleteObject(iconInfo.hbmMask);
    DestroyCursor(cursorInfo.hCursor);
    LocalFree(&iconInfo);
    LocalFree(&bmMask);
    delete[] mBits;
    return -1;
  }

  BOOL success = GetBitmapBits(iconInfo.hbmMask, bmMask.bmWidthBytes * bmMask.bmHeight, mBits);

  if (!success)
  {
    logTest("Poller::Cursor","GetBitmapBits() failed.\n");
    delete[] mBits;
    return -1;
  }

  andBits = mBits;

  long width = bmMask.bmWidth;
  long height = (isColorCursor) ? bmMask.bmHeight : bmMask.bmHeight/2;

  //
  // The bitmask is formatted so that the upper half is
  // the icon AND bitmask and the lower half is the icon XOR bitmask.
  //

  if (!isColorCursor)
  {
    xorBits = andBits + bmMask.bmWidthBytes * height;

/*    logTest("Poller::Cursor","no color widthB[%ld] width[%ld] height[%ld] totByte[%ld] mbits[%ld].\n",
                 bmMask.bmWidthBytes,width,height,success,bmMask.bmHeight * bmMask.bmWidthBytes);*/

    if (xCursor_ > 0)
    {
      XFreeCursor(display_, xCursor_);
    }

    xCursor_ = createCursor(wnd, vis, (unsigned int)iconInfo.xHotspot, (unsigned int)iconInfo.yHotspot,
                    width, height, (unsigned char *)xorBits, (unsigned char *)andBits);

    XDefineCursor(display_, wnd, xCursor_);
  }

  delete []mBits;
  DeleteObject(iconInfo.hbmMask);
  LocalFree(&bmMask);
  DestroyCursor(cursorInfo.hCursor);
  LocalFree(&iconInfo);

  return success;
}

unsigned char Poller::specialKeys(unsigned int keysym, unsigned int state, int pressed)
{
  return 0;
}

void Poller::ensureServerModifiers(keyTranslation tr, int *lengthArrayINPUT)
{
  return;
}

void Poller::restoreServerModifiers(UINT scancode)
{
  keyTranslation dummy;
  int lengthArrayINPUT = 0;

  if (isModifier(scancode) == 1)
  {
    return;
  }

  dummy.scancode = 0;
  dummy.modifiers = savedServerModifierState_;
  ensureServerModifiers(dummy, &lengthArrayINPUT);
  if (sendInput(0, 0, &lengthArrayINPUT) == 0)
  {
    logTest("Poller::restoreServerModifiers", "lengthArrayINPUT [%d]", lengthArrayINPUT);
  }
}

int Poller::updateShadowFrameBuffer(void)
{
  return 1;
}

void Poller::addToKeymap(char *keyname, unsigned char scancode, unsigned short modifiers, char *mapname)
{
  return;
}

FILE *Poller::xkeymapOpen(char *filename)
{
  return NULL;
}

int Poller::xkeymapRead(char *mapname)
{
  return 1;
}

void Poller::xkeymapInit(char *keyMapName)
{
  return;
}

keyTranslation Poller::xkeymapTranslateKey(unsigned int keysym, unsigned int keycode,
                                               unsigned int state)
{
  keyTranslation tr = { 0, 0 };

  return tr;
}

unsigned char Poller::getKeyState(unsigned int state, unsigned int keysym)
{
  return 0;
}

char *Poller::getKsname(unsigned int keysym)
{
        char *ksname = NULL;

        return ksname;
}

//
// Routine used to fool Winlogon into thinking CtrlAltDel was pressed
//
char Poller::simulateCtrlAltDel(void)
{
  HDESK oldDesktop = GetThreadDesktop(GetCurrentThreadId());

  //
  // Switch into the Winlogon desktop.
  //
  if (selectDesktopByName("Winlogon") == 0)
  {
    logTest("SimulateCtrlAltDelThreadFn","Failed to select logon desktop.");
    return 0;
  }

  logTest("SimulateCtrlAltDelThreadFn","Generating ctrl-alt-del.");

  //
  // Winlogon uses hotkeys to trap Ctrl-Alt-Del.
  //
  PostMessage(HWND_BROADCAST, WM_HOTKEY, 0, MAKELONG(MOD_ALT | MOD_CONTROL, VK_DELETE));

  //
  // Switch back to our original desktop.
  //
  if (oldDesktop != NULL)
  {
    selectDesktop(oldDesktop);
  }

  return 1;
}

// Switches the current thread into a different desktop by desktop handle
// This call takes care of all the evil memory management involved
char Poller::selectDesktop(HDESK newDesktop)
{
  //
  // Only on NT.
  //
  if (isWinNT())
  {
    HDESK oldDesktop = GetThreadDesktop(GetCurrentThreadId());

    DWORD dummy;
    char newName[256];

    if (GetUserObjectInformation(newDesktop, UOI_NAME, &newName, 256, &dummy) == 0)
    {
      logDebug("Poller::selectDesktop","GetUserObjectInformation() failed. Error[%lu].", GetLastError());
      return 0;
    }

    logTest("Poller::selectDesktop","New Desktop to [%s] (%x) from (%x).",
                 newName, (unsigned int)newDesktop, (unsigned int)oldDesktop);

    //
    // Switch the desktop.
    //
    if(SetThreadDesktop(newDesktop) == 0)
    {
      logDebug("Poller::SelectDesktop","Unable to SetThreadDesktop(), Error=%lu.", GetLastError());
      return 0;
    }

    //
    // Switched successfully - destroy the old desktop.
    //
    if (CloseDesktop(oldDesktop) == 0)
    {
      logDebug("Poller::selectHdesk","Failed to close old desktop (%x), Error=%lu.",
                   (unsigned int)oldDesktop, GetLastError());
      return 0;
    }
  }

  return 1;
}

//
// Switches the current thread into a different desktop, by name
// Calling with a valid desktop name will place the thread in that desktop.
// Calling with a NULL name will place the thread in the current input desktop.
//

char Poller::selectDesktopByName(char *name)
{
  //
  // Only on NT.
  //
  if (isWinNT())
  {
    HDESK desktop;

    if (name != NULL)
    {
      //
      // Attempt to open the named desktop.
      //
      desktop = OpenDesktop(name, 0, FALSE,
                                DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW |
                                DESKTOP_ENUMERATE | DESKTOP_HOOKCONTROL |
                                DESKTOP_WRITEOBJECTS | DESKTOP_READOBJECTS |
                                DESKTOP_SWITCHDESKTOP | GENERIC_WRITE);
    }
    else
    {
      //
      // Open the input desktop.
      //
      desktop = OpenInputDesktop(0, FALSE,
                                     DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW |
                                     DESKTOP_ENUMERATE | DESKTOP_HOOKCONTROL |
                                     DESKTOP_WRITEOBJECTS | DESKTOP_READOBJECTS |
                                     DESKTOP_SWITCHDESKTOP | GENERIC_WRITE);
    }

    if (desktop == NULL)
    {
      logDebug("Poller::selectDesktopByName","Unable to open desktop, Error=%lu.", GetLastError());
      return 0;
    }

    //
    // Switch to the new desktop
    //
    if (selectDesktop(desktop) == 0)
    {
      //
      // Failed to enter the new desktop, so free it!
      //
      logDebug("Poller::selectDesktopByName","Failed to select desktop.");

      if (CloseDesktop(desktop) == 0)
      {
        logDebug("Poller::selectDesktopByName","Failed to close desktop, Error=%lu.", GetLastError());
        return 0;
      }
    }

    return 1;
  }

  return (name == NULL);
}

void Poller::platformOS()
{
    OSVERSIONINFO osversioninfo;
    osversioninfo.dwOSVersionInfoSize = sizeof(osversioninfo);

    //
    // Get the current OS version.
    //
    if (GetVersionEx(&osversioninfo) == 0)
    {
      platformID_ = 0;
    }
    platformID_ = osversioninfo.dwPlatformId;

//
//    versionMajor = osversioninfo.dwMajorVersion;
//    versionMinor = osversioninfo.dwMinorVersion;
//
}

char Poller::checkDesktop()
{
  //
  // Only on NT.
  //
  if (isWinNT())
  {
    //
    // Get the input and thread desktops.
    //
    HDESK desktop = GetThreadDesktop(GetCurrentThreadId());
    HDESK inputDesktop = OpenInputDesktop(0, FALSE,
                        DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW |
                        DESKTOP_ENUMERATE | DESKTOP_HOOKCONTROL |
                        DESKTOP_WRITEOBJECTS | DESKTOP_READOBJECTS |
                        DESKTOP_SWITCHDESKTOP | GENERIC_WRITE);

    if (inputDesktop == NULL)
    {
      return 0;
    }

    DWORD dummy;
    char desktopName[256];
    char inputName[256];

    if (GetUserObjectInformation(desktop, UOI_NAME, &desktopName, 256, &dummy) == 0)
    {
      if (CloseDesktop(inputDesktop) == 0)
      {
        logDebug("Poller::checkDesktop", "Failed to close desktop, Error[%d].", (unsigned int)GetLastError());
        return 0;
      }
    }

    if (GetUserObjectInformation(inputDesktop, UOI_NAME, &inputName, 256, &dummy) == 0)
    {
      if (CloseDesktop(inputDesktop) == 0)
      {
        logDebug("Poller::checkDesktop", "Failed to close input desktop, Error[%d].", (unsigned int)GetLastError());
        return 0;
      }
    }

    if (strcmp(desktopName, inputName) != 0)
    {
      //
      // Switch to new desktop.
      //
      selectDesktop(inputDesktop);
    }

    if (CloseDesktop(desktop) == 0)
    {
      logDebug("Poller::checkDesktop", "Failed to close input desktop, Error[%d].", (unsigned int)GetLastError());
      return 0;
    }

    if (CloseDesktop(inputDesktop) == 0)
    {
      logDebug("Poller::checkDesktop", "Failed to close input desktop, Error[%d].", (unsigned int)GetLastError());
      return 0;
    }
  }

  return 1;
}

unsigned char Poller::isModifier(UINT scancode)
{
  return 0;
}

void Poller::updateModifierState(UINT scancode, unsigned char pressed)
{
  return;
}

Cursor Poller::createCursor(Window wnd, Visual *vis,unsigned int x, unsigned int y,
                                int width, int height, unsigned char *xormask, unsigned char *andmask)
{
  Pixmap maskglyph, cursorglyph;
  XColor bg, fg;
  Cursor xcursor;
  unsigned char *cursor;
  unsigned char *mask, *pmask, *pcursor, tmp;
  int scanline, offset;

  scanline = (width + 7) / 8;
  offset = scanline * height;

  pmask = andmask;
  pcursor = xormask;
  for (int i = 0; i < offset; i++)
  {
    //
    // The pixel is black if both the bit of andmask and xormask is one.
    //

    tmp = *pcursor & *pmask;
    *pcursor ^= tmp;
    *pmask ^= tmp;

    *pmask = ~(*pmask);

    pmask++;
    pcursor++;
  }

  cursor = new unsigned char[offset];
  memcpy(cursor, xormask, offset);

  mask = new unsigned char[offset];
  memcpy(mask, andmask, offset);

  fg.red = fg.blue = fg.green = 0xffff;
  bg.red = bg.blue = bg.green = 0x0000;
  fg.flags = bg.flags = DoRed | DoBlue | DoGreen;

  cursorglyph = createGlyph(wnd, vis, width, height, cursor);
  maskglyph = createGlyph(wnd, vis, width, height, mask);

  xcursor = XCreatePixmapCursor(display_, cursorglyph, maskglyph, &fg, &bg, x, y);

  XFreePixmap(display_, maskglyph);
  XFreePixmap(display_, cursorglyph);
  delete[]mask;
  delete[]cursor;

  return xcursor;
}

Pixmap Poller::createGlyph(Window wnd, Visual *visual, int width, int height, unsigned char *data)
{
  XImage *image;
  Pixmap bitmap;
  int scanline;
  GC glyphGC;

  scanline = (width + 7) / 8;

  bitmap = XCreatePixmap(display_, wnd, width, height, 1);
  glyphGC = XCreateGC(display_, bitmap, 0, NULL);

  image = XCreateImage(display_, visual, 1, ZPixmap, 0, (char *)data, width, height, 8, scanline);
  image->byte_order = 1; // MSBFirst -- LSBFirst = 0
  image->bitmap_bit_order = 1;
  XInitImage(image);

/*  logTest("Poller::createGlyph","XPutImage on pixmap %d,%d,%d,%d.\n",
              0, 0, width, height);*/
  XPutImage(display_, bitmap, glyphGC, image, 0, 0, 0, 0, width, height);
  XFree(image);

  return bitmap;
}
#endif /* defined(__CYGWIN32__) || defined(WIN32) */
