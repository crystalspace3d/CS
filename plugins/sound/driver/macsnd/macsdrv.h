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

#ifndef __SOUND_DRIVER_MAC_H__
#define __SOUND_DRIVER_MAC_H__

// SoundDriver.H
// csSoundDriverMac class.

#include <Sound.h>

#include "csutil/scf.h"
#include "isound/isnddrv.h"
#include "isys/isystem.h"

class csSoundDriverMac : public iSoundDriver
{
protected:
  iSystem* m_piSystem;
  iSoundRender *m_piSoundRender;
  void * Memory;
  int MemorySize;
  int m_nFrequency;
  bool m_b16Bits;
  bool m_bStereo;

public:
  DECLARE_IBASE;
  csSoundDriverMac(iBase *piBase);
  virtual ~csSoundDriverMac();
  virtual bool Initialize(iSystem *iSys);
  
  virtual bool Open(iSoundRender *render, int frequency, bool bit16, bool stereo);
  virtual void Close();
	
  virtual void LockMemory(void **mem, int *memsize);
  virtual void UnlockMemory();
  virtual bool IsBackground();
  virtual bool Is16Bits();
  virtual bool IsStereo();
  virtual int  GetFrequency();
  virtual bool IsHandleVoidSound();
 
  void SndDoubleBackProc( SndChannelPtr channel, SndDoubleBufferPtr doubleBuffer );
  
 private:
  SndDoubleBufferHeader	mSoundDBHeader;
  SndDoubleBuffer		mSoundDoubleBuffer;
  SndChannelPtr			mSoundChannel;
  
  long	mFramesPerBuffer;
  bool	mStopPlayback;
  long	mOutputVolume;
  
  long	mBuffersFilled;
};

#endif	//__SOUND_DRIVER_MAC_H__

