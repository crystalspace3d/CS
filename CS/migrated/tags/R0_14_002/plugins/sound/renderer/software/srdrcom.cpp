/*
    Sound Render interface DLL

    Copyright (C) 1998, 1999 by Nathaniel 'NooTe' Saint Martin
    Copyright (C) 1998, 1999 by Jorrit Tyberghein
    Written by Nathaniel 'NooTe' Saint Martin

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

#include <stdlib.h>
#include "sysdef.h"
#include "cscom/com.h"
#include "cssndrdr/software/srdrcom.h"
#include "isndrdr.h"

static unsigned int gRefCount = 0;
static DllRegisterData gRegData =
{
  &CLSID_SoftwareSoundRender,
  "crystalspace.sound.render.software",
  "Crystal Space Software Sound Render"
};

#ifdef CS_STATIC_LINKED

void SoundRenderSoftwareRegister ()
{
  static csSoundRenderSoftwareFactory gFactory;
  gRegData.pClass = &gFactory;
  csRegisterServer (&gRegData);
}

void SoundRenderSoftwareUnregister ()
{
  csUnregisterServer (&gRegData);
}

#else

// This is the name of the DLL. Make sure to change this if you change the DLL name!
// DAN: this might have to be changed for each OS, cuz each OS has a different extension for DLLs.
#if defined (OS_WIN32)
#define DLL_NAME "SoundRenderSoftware.dll"
#elif defined (OS_OS2)
#define DLL_NAME "sndrdrs.dll"
#elif defined (OS_MACOS)
#define DLL_NAME "sndrdrs.shlb"
#elif defined (OS_NEXT)
#define DLL_NAME "sndrdrs.dylib"
#else
#define DLL_NAME "sndrdrs.so"
#endif

// our main entry point...should be called when we're loaded.
STDAPI DllInitialize ()
{
  csCoInitialize (0);
  gRegData.szInProcServer = DLL_NAME;
  return TRUE;
}

void STDAPICALLTYPE ModuleRelease ()
{
  gRefCount--;
}

void STDAPICALLTYPE ModuleAddRef ()
{
  gRefCount++;
}

// return S_OK if it's ok to unload us now.
STDAPI DllCanUnloadNow ()
{
  return gRefCount ? S_FALSE : S_OK;
}

// used to get a COM class object from us.
STDAPI DllGetClassObject (REFCLSID rclsid, REFIID riid, void** ppv)
{
  static csSoundRenderSoftwareFactory gFactory;
  if (rclsid == CLSID_SoftwareSoundRender)
    return gFactory.QueryInterface(riid, ppv);

  //  if we get here, rclsid is a class we don't implement
  *ppv = NULL;
  return CLASS_E_CLASSNOTAVAILABLE;
}

// Called by RegSvr32.exe
STDAPI DllRegisterServer ()
{
  return csRegisterServer (&gRegData);
}

// Called by RegSvr32.exe
STDAPI DllUnregisterServer ()
{
  return csUnregisterServer(&gRegData);
}

#endif

// Implementation of csSoundRenderSoftwareFactory

IMPLEMENT_UNKNOWN_NODELETE (csSoundRenderSoftwareFactory)

BEGIN_INTERFACE_TABLE (csSoundRenderSoftwareFactory)
  IMPLEMENTS_INTERFACE (ISoundRenderFactory)
END_INTERFACE_TABLE ()

STDMETHODIMP csSoundRenderSoftwareFactory::CreateInstance (REFIID riid, ISystem* piSystem, void** ppv)
{
  if (!piSystem)
  {
    *ppv = 0;
    return E_UNEXPECTED;
  }

  csSoundRenderSoftware* pNew = new csSoundRenderSoftware (piSystem);
  if (!pNew)
  {
    *ppv = 0;
    return E_OUTOFMEMORY;
  }

  return pNew->QueryInterface (riid, ppv);
}

STDMETHODIMP csSoundRenderSoftwareFactory::LockServer (COMBOOL bLock)
{
  if (bLock)
    gRefCount++;
  else
    gRefCount--;

  return S_OK;
}
