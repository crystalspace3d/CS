/*
    Copyright (C) 2001 by Jorrit Tyberghein

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

#ifndef __IUTIL_VIRTCLK_H__
#define __IUTIL_VIRTCLK_H__

#include "csutil/scf.h"

SCF_VERSION (iVirtualClock, 0, 0, 1);

/// A utility class that makes it easier to parse the command line.
struct iVirtualClock : public iBase
{
  /**
   * Advance the engine's virtual-time clock.
   */
  virtual void Advance () = 0;

  /**
   * Suspend the engine's virtual-time clock.<p>
   * Call this function when the client application will fail to invoking
   * NextFrame() for an extended period of time.  Suspending the virtual-time
   * clock prevents a temporal anomaly from occurring the next time
   * GetElapsedTime() is called after the application resumes invoking
   * NextFrame().
   */
  virtual void Suspend () = 0;

  /**
   * Resume the engine's virtual-time clock.<p>
   * Call this function when the client application begins invoking NextFrame()
   * again after extended down-time.  This function is the complement of
   * SuspendVirtualTimeClock().
   */
  virtual void Resume () = 0;

  /**
   * Query the time elapsed between previous call to NextFrame and last
   * call to NextFrame().
   */
  virtual csTicks GetElapsedTicks () const = 0;

  /**
   * Returns the absolute time of the last call
   * to NextFrame().
   */
  virtual csTicks GetCurrentTicks () const = 0;
};

#endif // __IUTIL_VIRTCLK_H__
