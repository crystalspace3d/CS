/*
    Copyright (C) 1998-2000 by Jorrit Tyberghein
  
    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.
  
    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.
  
    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "cssysdef.h"

#include "csgeom/math3d.h"
#include "cssys/system.h"
#include "cssys/win32/win32.h"
#include "csutil/inifile.h"
#include "igraph3d.h"

// Sets up graphics drivers -- Assumes DirectX 5 or later & DirectDraw 3D
#include "cssys/win32/DirectDetection.h"
#include "video/canvas/ddraw/ig2d.h"

#include <windowsx.h>

#include <ddraw.h>

// Use new DirectInput keyboard handling by Xavier Trochu (xtrochu@yahoo.com)?
#ifdef DO_DINPUT_KEYBOARD
#include <dinput.h>
#endif

#include <stdio.h>
#include <time.h>

#if defined(COMP_WCC) || defined(COMP_VC)
#include <sys\timeb.h>
#endif

#if defined(COMP_BC)
#include <dos.h> // For _argc & _argv
#endif

extern HINSTANCE ModuleHandle;
bool ApplicationActive = true;
int ApplicationShow;

void sys_fatalerror(char *s)
{
  MessageBox(NULL, s, "Fatal Error", MB_OK | MB_ICONSTOP);
  exit (1);
}

extern csSystemDriver* System; // Global pointer to system that can be used by event handler

long FAR PASCAL WindowProc( HWND hWnd, UINT message,WPARAM wParam, LPARAM lParam );

//-----------------------------------------------// The System Driver //------//

#ifdef DO_DINPUT_KEYBOARD

/*
 * This table performs the translation from keycode to character.
 * It use this english keyboard layout.
 * ----
 * PS: They may be some errors, it's quite difficult to check. So if you
 * have some keys that are not handled correctly, You'd better check this first.
 *
*/
static int s_KeyTable[257]=
{
  0,
  CSKEY_ESC,'1','2','3','4','5','6','7','8','9','0','-','=',CSKEY_BACKSPACE,CSKEY_TAB,
  'q','w','e','r','t','y','u','i','o','p','[',']','\n',
  CSKEY_CTRL,'a','s','d','f','g','h','j','k','l',';','\'','`',CSKEY_SHIFT,'\\',
  'z','x','c','v','b','n','m',',','.','/',CSKEY_SHIFT,CSKEY_PADMULT,CSKEY_ALT,
  ' ',0/*DIK_CAPITAL*/,
  CSKEY_F1,CSKEY_F2,CSKEY_F3,CSKEY_F4,CSKEY_F5,CSKEY_F6,CSKEY_F7,CSKEY_F8,CSKEY_F9,CSKEY_F10,
  0/*DIK_NUMLOCK*/,0/*DIK_SCROLL*/,
  CSKEY_HOME,CSKEY_UP,CSKEY_PGUP,CSKEY_PADMINUS,CSKEY_LEFT,CSKEY_CENTER,CSKEY_RIGHT,CSKEY_PADPLUS,CSKEY_END,CSKEY_DOWN,CSKEY_PGDN,CSKEY_INS,'.',
  0/*DIK_OEM_102*/,CSKEY_F11,CSKEY_F12,0,0,0,0,0,0,0,0,0,0,0,0/*DIK_F13*/,0/*DIK_F14*/,0/*DIK_F15*/,0,0,0,0,0,0,0,0,0,
  0/*DIK_KANA*/,0,0,0/*DIK_ABNT_C1*/,0,0,0,0,0,0/*DIK_CONVERT*/,0,0/*DIK_NOCONVERT*/,
  0,0/*DIK_YEN*/,0/*DIK_ABNT_C2*/,0,0,0,0,0,0,0,0,0,0,0,0,0,0,'=',0,0,0/*DIK_CIRCUMFLEX*/,0/*DIK_AT*/,
  0/*DIK_COLON*/,0/*DIK_UNDERLINE*/,0/*DIK_KANJI*/,0/*DIK_STOP*/,0/*DIK_AX*/,0/*DIK_UNLABELED*/,0,0,0,0,
  '\n',CSKEY_CTRL,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  CSKEY_DEL,0,CSKEY_PADDIV,0,0/*DIK_SYSRQ*/,CSKEY_ALT,0,0,0,0,0,0,0,0,0,0,0,0,0/*DIK_PAUSE*/,0,
  CSKEY_HOME,CSKEY_UP,CSKEY_PGUP,0,CSKEY_LEFT,0,CSKEY_RIGHT,0,CSKEY_END,CSKEY_DOWN,CSKEY_PGDN,CSKEY_INS,CSKEY_DEL
};

// This macro is for COM calls. If it fails, it shows a MessageBox then
// kills the whole process. It's brutal, but as I use another thread,
// it's safer this way
#define CHK_FAILED(x) \
  { if(FAILED(x)) { MessageBox(NULL, #x " Failed!", NULL,MB_OK|MB_ICONERROR); ::ExitProcess(1); } }
// This macro is for COM Release calls
#define CHK_RELEASE(x) \
  { if((x) != NULL) { (x)->Release(); (x)=NULL; } }

/*
 * The thread entry point. Called by ::Open()
 */

#define AUTOREPEAT_WAITTIME 1000 // 1 seconde
#define AUTOREPEAT_TIME      100 // 10 keystroke/seconds

DWORD WINAPI s_threadroutine (LPVOID param)
{
  SysSystemDriver *System = (SysSystemDriver *)param;
  HRESULT hr;
  DWORD dwWait = INFINITE;
  char *buffer;
  int i,lastkey = -1;
#ifndef DI_USEGETDEVICEDATA
  char *oldbuffer = NULL;
#else
#endif
  LPDIRECTINPUT lpdi = NULL;
  LPDIRECTINPUTDEVICE lpKbd = NULL; 
  //Setup for directinput mouse code
  LPDIRECTINPUTDEVICE lpMouse = NULL; 

  HANDLE hEvent [2];
//  HANDLE hevtMouse;

  // Use 0x0300 instead of DIRECTINPUT_VERSION
  // to allow the binaries run under NT4 (which has just DX3)
  CHK_FAILED (DirectInputCreate (ModuleHandle, 0x0300, &lpdi, NULL));
  CHK_FAILED (lpdi->CreateDevice (GUID_SysKeyboard, &lpKbd, NULL));
  CHK_FAILED (lpKbd->SetDataFormat (&c_dfDIKeyboard)); 
  CHK_FAILED (lpKbd->SetCooperativeLevel (FindWindow (WINDOWCLASSNAME, NULL),
									  DISCL_FOREGROUND | DISCL_NONEXCLUSIVE)); 

  //Setup for directinput mouse code
  /*
  CHK_FAILED(lpdi->CreateDevice (GUID_SysMouse, &lpMouse, NULL));
  CHK_FAILED(lpMouse->SetDataFormat(&c_dfDIMouse));
  CHK_FAILED(lpMouse->SetCooperativeLevel(FindWindow (WINDOWCLASSNAME, NULL),
									  DISCL_EXCLUSIVE | DISCL_FOREGROUND));
  hevtMouse = CreateEvent(0, 0, 0, 0);
  CHK_FAILED(lpMouse->SetEventNotification(g_hevtMouse));
  DIPROPDWORD dipdw =
      {
          {
              sizeof(DIPROPDWORD),        // diph.dwSize
              sizeof(DIPROPHEADER),       // diph.dwHeaderSize
              0,                          // diph.dwObj
              DIPH_DEVICE,                // diph.dwHow
          },
          DINPUT_BUFFERSIZE,              // dwData
      };
  CHK_FAILED(lpMouse->SetProperty(DIPROP_BUFFERSIZE, &dipdw.diph));
  */

#ifdef DI_USEGETDEVICEDATA
  {
    DIPROPDWORD dpd;
    dpd.diph.dwSize=sizeof(DIPROPDWORD);
    dpd.diph.dwHeaderSize=sizeof(DIPROPHEADER);
    dpd.diph.dwObj=0;
    dpd.diph.dwHow=DIPH_DEVICE;
    dpd.dwData=10; // The size of the buffer (should be more than sufficient)
#if DIRECTINPUT_VERSION < 0x0700
    CHK_FAILED (lpKbd->SetProperty (DIPROP_BUFFERSIZE, &dpd));
#else 
    //For incomprehensible reason, SetProperty() parameters type has
    //changed between DX6.1 and DX7 SDK
    CHK_FAILED (lpKbd->SetProperty (DIPROP_BUFFERSIZE, &dpd.diph));
#endif
  }
#endif
  hEvent [0] = CreateEvent (NULL, FALSE, FALSE, NULL);
  if (hEvent [0] == NULL)
  {
    MessageBox (NULL, "CreateEvent() Failed!", NULL, MB_OK|MB_ICONERROR);
    ExitProcess (1);
  }
  if (!DuplicateHandle (GetCurrentProcess(), System->m_hEvent, GetCurrentProcess (),
        &hEvent [1], 0, FALSE, DUPLICATE_SAME_ACCESS))
  {
    MessageBox (NULL, "DuplicateEvent() Failed!", NULL, MB_OK|MB_ICONERROR);
    ExitProcess (1);
  }

  hr = lpKbd->SetEventNotification (hEvent [0]);
  switch (hr)
  {
    case DI_OK:
      break;
    default:
      MessageBox (NULL, "lpKbd->SetEventNotification(hEvent) Failed!", NULL,
        MB_OK|MB_ICONERROR);
      ExitProcess (1);
      break;
  }

  while (1)
  {
    hr = lpKbd->Acquire ();
    if (SUCCEEDED (hr))
      break;
    if (WaitForSingleObject (hEvent [1], 0) == WAIT_OBJECT_0 + 1)
    {
      CloseHandle (hEvent [0]);
      CloseHandle (hEvent [1]);
      CHK_RELEASE (lpKbd);
      CHK_RELEASE (lpdi);
#ifndef DI_USEGETDEVICEDATA
      if (oldbuffer) delete[] oldbuffer;
#endif
      return 0;
    }
  }

#ifndef DI_USEGETDEVICEDATA
  oldbuffer = new char [256];
  hr = lpKbd->GetDeviceState (256, oldbuffer);
#endif
  while (1)
  {
    switch (WaitForMultipleObjects (2, hEvent, FALSE, dwWait))
    {
      case WAIT_OBJECT_0:
#ifndef DI_USEGETDEVICEDATA
        buffer = new char [256];
        do
        {
          hr = lpKbd->GetDeviceState (256, buffer);
          switch (hr)
          {
            case DIERR_NOTACQUIRED:
            case DIERR_INPUTLOST:
              lpKbd->Acquire ();
              break;
            case DI_OK:
              break;
            default:
              MessageBox (NULL, "lpKbd->GetDeviceState(hEvent) Failed!",
                NULL, MB_OK|MB_ICONERROR);
              ExitProcess (1);
              break;
          }
        } while (hr != DI_OK);
        for (i = 0; i < 256; i++)
          if (oldbuffer [i] != buffer [i])
          {
            if (buffer [i] & 0X80)
            {
              lastkey = i;
              dwWait = AUTOREPEAT_WAITTIME;
              System->QueueKeyEvent (s_KeyTable [i], true);
            }
            else
            {
              lastkey = -1;
              dwWait = INFINITE;
              System->QueueKeyEvent (s_KeyTable[i], false);
            }
            break;
          }
        delete [] oldbuffer;
        oldbuffer = buffer;
#else
        DIDEVICEOBJECTDATA *lpdidod;
        DWORD dwNb;
        do
        {
          dwNb = INFINITE;
          hr = lpKbd->GetDeviceData (sizeof (DIDEVICEOBJECTDATA), NULL, &dwNb,DIGDD_PEEK);
          switch(hr)
          {
            case DIERR_NOTACQUIRED:
            case DIERR_INPUTLOST:
              lpKbd->Acquire ();
              break;
            case DI_OK:
              break;
            case DI_BUFFEROVERFLOW:
              hr = DI_OK;
              break;
            default:
              MessageBox(NULL, "lpKbd->GetDeviceState(hEvent) Failed!",
                NULL, MB_OK|MB_ICONERROR);
              ExitProcess (1);
              break;
          }
        } while (hr != DI_OK);
        if (!dwNb)
          continue;
        lpdidod = new DIDEVICEOBJECTDATA [dwNb];
        CHK_FAILED (lpKbd->GetDeviceData (sizeof (DIDEVICEOBJECTDATA),
          lpdidod, &dwNb, 0));
        for (i = 0; i < dwNb; i++)
        {
          if (lpdidod [i].dwData & 0X80)
          {
            lastkey = lpdidod [i].dwOfs;
            dwWait = AUTOREPEAT_WAITTIME:
            System->QueueKeyEvent (s_KeyTable [lpdidod [i].dwOfs], true);
          }
          else
          {
            lastkey = -1;
            dwWait = INFINITE;
            System->QueueKeyEvent (s_KeyTable [lpdidod [i].dwOfs], false);
          }
        }
        delete[] lpdidod;
#endif
        break;
      case WAIT_TIMEOUT:  // HANDLE key autorepeat
        buffer = new char [256];
        do
        {
          hr = lpKbd->GetDeviceState (256, buffer);
          switch (hr)
          {
            case DIERR_NOTACQUIRED:
            case DIERR_INPUTLOST:
              lpKbd->Acquire ();
              break;
            case DI_OK:
              break;
            default:
              MessageBox (NULL, "lpKbd->GetDeviceState(hEvent) Failed!",
                NULL, MB_OK|MB_ICONERROR);
              ExitProcess (1);
              break;
          }
        } while (hr != DI_OK);
        // The lastkey is still pressed
        if ((lastkey >= 0) && (buffer [lastkey] & 0X80))
        {
          dwWait = AUTOREPEAT_TIME;
          System->QueueKeyEvent (s_KeyTable [lastkey], true);
        }
        else
        { // Strange.. we didn't get the message that the key was released !
          lastkey = -1;
          dwWait = INFINITE;
        }
        delete [] buffer;
        break;
      case WAIT_OBJECT_0 + 1:
        lpKbd->Unacquire ();
        CloseHandle (hEvent [0]);
        CloseHandle (hEvent [1]);
        CHK_RELEASE (lpKbd);
        CHK_RELEASE (lpdi);
#ifndef DI_USEGETDEVICEDATA
        if (oldbuffer)
          delete [] oldbuffer;
#endif
        return 0;
    }
  }
}

#undef CHK_RELEASE
#undef CHK_FAILED
#endif // DO_DINPUT_KEYBOARD

IMPLEMENT_IBASE_EXT (SysSystemDriver)
  IMPLEMENTS_INTERFACE (iWin32SystemDriver)
IMPLEMENT_IBASE_EXT_END

SysSystemDriver::SysSystemDriver () : csSystemDriver ()
{
  WNDCLASS wc;
  wc.hCursor        = LoadCursor(0, IDC_ARROW);
  wc.hIcon          = LoadIcon( NULL, IDI_APPLICATION );
  wc.lpszMenuName   = NULL;
  wc.lpszClassName  = WINDOWCLASSNAME;
  wc.hbrBackground  = 0;
  wc.hInstance      = ModuleHandle;
  wc.style          = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;;
  wc.lpfnWndProc    = WindowProc;
  wc.cbClsExtra     = 0;
  wc.cbWndExtra     = 0;

  bool bResult;
  bResult = RegisterClass (&wc);
  ASSERT(bResult);
}

bool SysSystemDriver::Open (const char *Title)
{
  if (!csSystemDriver::Open (Title))
    return false;

#ifdef DO_DINPUT_KEYBOARD
  DWORD dwThreadId;
  m_hEvent = CreateEvent (NULL, FALSE, FALSE, NULL);
  m_hThread = CreateThread (NULL, 0, s_threadroutine, this, 0, &dwThreadId);
  if (!m_hEvent || !m_hThread)
  {
    MessageBox (NULL, "CreateEvent() Failed!", NULL, MB_OK|MB_ICONERROR);
    ExitProcess (1);
  }
#endif
  return true;
}

void SysSystemDriver::Close ()
{
  csSystemDriver::Close ();
  ChangeDisplaySettings (NULL, 0);

#ifdef DO_DINPUT_KEYBOARD
  if (m_hEvent)
  {
    SetEvent (m_hEvent);
    CloseHandle (m_hEvent);
    m_hEvent = NULL;
    WaitForSingleObject (m_hThread, 1000);
    CloseHandle (m_hThread);
    m_hThread = NULL;
  }
#endif
}

void SysSystemDriver::Loop ()
{
  MSG msg;
  iGraphics2DDDraw3* piG2Ddd3 = NULL;

  piG2Ddd3 = QUERY_INTERFACE(System, iGraphics2DDDraw3);

  if (piG2Ddd3)
  {
    piG2Ddd3->SetColorPalette();

    piG2Ddd3->DecRef();
    piG2Ddd3 = NULL;
  }

  while(!Shutdown && !ExitLoop)
  {
    if (PeekMessage (&msg, NULL, 0, 0, PM_NOREMOVE))
    {
      if (!GetMessage (&msg, NULL, 0, 0))
        return;
      
      TranslateMessage (&msg); 
      DispatchMessage (&msg);
    }
    else
    {
      static long prev_time = -1;
      long now     = Time ();
      time_t elapsed = (prev_time == -1) ? 0 : now - prev_time;
      prev_time = now;
      NextFrame (elapsed, now);
    }
  }
}

void SysSystemDriver::Alert (const char* s)
{
  bool FullScreen = false;
  int width, height, depth;

  System->GetSettings(width, height, depth, FullScreen);

  if (FullScreen)
  {
    //If fullscreen mode is active, we switch to default screen, because
    //otherwise this message will not be seen.
    ChangeDisplaySettings (NULL, 0);
  }
         
  MessageBox (NULL, s, "Fatal Error", MB_OK | MB_ICONSTOP);
  debug_out (true, s);
}

void SysSystemDriver::Warn (const char* s)
{
  //In windows there is no safe way to display a messagebox and then continue work when
  //you are in fullscreen mode. (If you know some way: You are welcome to change this)
  //For the moment we will display Warning as console messages.
  Printf (MSG_CONSOLE, "Warning:\n%s", s);
  debug_out (true, s);
}

//----------------------------------------------// SCF Implementation //------//

HINSTANCE SysSystemDriver::GetInstance () const
{
  return ModuleHandle;
}

bool SysSystemDriver::GetIsActive () const
{
  return ApplicationActive;
}

int SysSystemDriver::GetCmdShow () const
{
  return ApplicationShow;
}

//----------------------------------------// Windows input translator //------//

#ifndef DO_DINPUT_KEYBOARD
void WinKeyTrans (csSystemDriver* pSystemDriver, WPARAM wParam, bool down)
{
  if (!pSystemDriver)
    return;

  int key = 0;
  switch (wParam)
  {
    case VK_END:     key = CSKEY_END; break;
    case VK_UP:      key = CSKEY_UP; break;
    case VK_DOWN:    key = CSKEY_DOWN; break;
    case VK_LEFT:    key = CSKEY_LEFT; break;
    case VK_RIGHT:   key = CSKEY_RIGHT; break;
    case VK_PRIOR:   key = CSKEY_PGUP; break;
    case VK_NEXT:    key = CSKEY_PGDN; break;
    case VK_MENU:    key = CSKEY_ALT; break;
    case VK_CONTROL: key = CSKEY_CTRL; break;
    case VK_SHIFT:   key = CSKEY_SHIFT; break;
  }
  if (key)
    pSystemDriver->QueueKeyEvent (key, down);
}
#endif

long FAR PASCAL WindowProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
  switch (message)
  {
    case WM_ACTIVATEAPP:
      ApplicationActive = wParam;
      break;
    case WM_ACTIVATE:
      if (System)
        System->QueueFocusEvent (LOWORD (wParam) != WA_INACTIVE);
      break;
    case WM_DESTROY:
      PostQuitMessage (0);
      break;
#ifndef DO_DINPUT_KEYBOARD
    case WM_CHAR:
      if(System)
      {
        if (wParam == '\r') wParam = CSKEY_ENTER;
        System->QueueKeyEvent (wParam, true);
        System->QueueKeyEvent (wParam, false);
      }
      break;
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
      WinKeyTrans (System, wParam, true);
      if (wParam == VK_MENU) return 0;
      break;
    case WM_KEYUP:
    case WM_SYSKEYUP:
      WinKeyTrans (System, wParam, false);
      if (wParam == VK_MENU) return 0;
      break;
#endif
    case WM_LBUTTONDOWN:
      if (System)
        System->QueueMouseEvent (1, true, LOWORD(lParam), HIWORD(lParam));
      break;
    case WM_LBUTTONUP:
      if (System)
        System->QueueMouseEvent (1, false, LOWORD(lParam), HIWORD(lParam));
      break;
    case WM_MBUTTONDOWN:
      if (System)
        System->QueueMouseEvent (3, true, LOWORD(lParam), HIWORD(lParam));
      break;
    case WM_MBUTTONUP:
      if (System)
        System->QueueMouseEvent (3, false, LOWORD(lParam), HIWORD(lParam));
      break;
    case WM_RBUTTONDOWN:
      if (System)
        System->QueueMouseEvent (2, true, LOWORD(lParam), HIWORD(lParam));
      break;
    case WM_RBUTTONUP:
      if (System)
        System->QueueMouseEvent (2, false, LOWORD(lParam), HIWORD(lParam));
      break;
    case WM_MOUSEMOVE:
      if (System)
        System->QueueMouseEvent (0, false, LOWORD(lParam), HIWORD(lParam));
      break;
  }
  return DefWindowProc (hWnd, message, wParam, lParam);
}


extern int csMain (int argc, char* argv[]);

// The main entry for GUI applications
int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance,
  LPSTR lpCmdLine, int nCmdShow)
{
  ModuleHandle = hInstance;
  ApplicationShow = nCmdShow;
  (void)lpCmdLine;
  (void)hPrevInstance;

  return
#ifdef COMP_BC
    csMain ( _argc,  _argv);
#else
    csMain (__argc, __argv);
#endif
}
