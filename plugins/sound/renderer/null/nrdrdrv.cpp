/*
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

#include <stdio.h>
#include <stdarg.h>

#include "sysdef.h"
#include "cscom/com.h"
#include "cssndrdr/null/nrdrcom.h"
#include "cssndrdr/null/nrdrlst.h"
#include "cssndrdr/null/nrdrsrc.h"
#include "isystem.h"
#include "isndlstn.h"
#include "isndsrc.h"

IMPLEMENT_UNKNOWN_NODELETE (csSoundRenderNull)

BEGIN_INTERFACE_TABLE(csSoundRenderNull)
  IMPLEMENTS_INTERFACE(ISoundRender)
END_INTERFACE_TABLE()

csSoundRenderNull::csSoundRenderNull(ISystem* piSystem) : m_pListener(NULL)
{
  m_piSystem = piSystem;

  m_pListener = new csSoundListenerNull ();
}

csSoundRenderNull::~csSoundRenderNull()
{
  if(m_pListener)
    delete m_pListener;
}

STDMETHODIMP csSoundRenderNull::GetListener(ISoundListener ** ppv )
{
  if (!m_pListener)
  {
    *ppv = NULL;
    return E_OUTOFMEMORY;
  }
  
  return m_pListener->QueryInterface (IID_ISoundListener, (void**)ppv);
}

STDMETHODIMP csSoundRenderNull::CreateSource(ISoundSource ** ppv, csSoundBuffer* /*snd*/)
{
  csSoundSourceNull* pNew = new csSoundSourceNull ();
  if (!pNew)
  {
    *ppv = 0;
    return E_OUTOFMEMORY;
  }
  
  return pNew->QueryInterface (IID_ISoundSource, (void**)ppv);
}

STDMETHODIMP csSoundRenderNull::Open()
{
  SysPrintf (MSG_INITIALIZATION, "\nSoundRender Null selected\n");

  return S_OK;
}

STDMETHODIMP csSoundRenderNull::Close()
{
  return S_OK;
}

STDMETHODIMP csSoundRenderNull::Update()
{
  return S_OK;
}

STDMETHODIMP csSoundRenderNull::SetVolume(float /*vol*/)
{
  return S_OK;
}

STDMETHODIMP csSoundRenderNull::GetVolume(float* /*vol*/)
{
  return S_OK;
}

STDMETHODIMP csSoundRenderNull::PlayEphemeral(csSoundBuffer* /*snd*/)
{
  return S_OK;
}

void csSoundRenderNull::SysPrintf(int mode, char* szMsg, ...)
{
  char buf[1024];
  va_list arg;
  
  va_start (arg, szMsg);
  vsprintf (buf, szMsg, arg);
  va_end (arg);
  
  m_piSystem->Print(mode, buf);
}
