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

#ifndef __SOUND_DRIVER_WAVEOUT_H__
#define __SOUND_DRIVER_WAVEOUT_H__

// SoundDriver.H
// csSoundDriverWaveOut class.

#include "csutil/scf.h"
#include "isnddrv.h"
#include "isystem.h"

class csSoundDriverWaveOut : public iSoundDriver
{
public:
	DECLARE_IBASE;
	
	csSoundDriverWaveOut(iBase *piBase);
	
	virtual ~csSoundDriverWaveOut();

	/// implementation of interface for iPlugin
	virtual bool Initialize (iSystem *iSys);
	
	bool Open(iSoundRender *render, int frequency, bool bit16, bool stereo);
	void Close();
	
	void SetVolume(float vol);
	float GetVolume();
	void LockMemory(void **mem, int *memsize);
	void UnlockMemory();
	bool IsBackground();
	bool Is16Bits();
	bool IsStereo();
	int GetFrequency();
	bool IsHandleVoidSound();
	
protected:
  // system driver
  iSystem* m_piSystem;

  // sound renderer
  iSoundRender *m_piSoundRender;

  // sound memory
  void * Memory;
  int MemorySize;

  // system volume (restored when the driver is closed)
  unsigned long old_Volume;

  // global volume setting
  float volume;
  // sound format
  int m_nFrequency;
  bool m_b16Bits;
  bool m_bStereo;

  // this function is called again and again in 'function' playback
  static void CALLBACK waveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2);
  // this function is called once in 'thread' playback
  static DWORD CALLBACK waveOutThreadProc( LPVOID dwParam); 
  // this actually sends the sound data to wave-out (used by both of the above functions).
  void SoundProc(LPWAVEHDR OldHeader);

  // wave-out device?
  HWAVEOUT hwo;
  // ???
  HANDLE hThread;
  DWORD dwThread;
};

#endif	//__SOUND_DRIVER_WAVEOUT_H__

