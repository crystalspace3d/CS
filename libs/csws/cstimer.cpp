/*
    Crystal Space Windowing System: timer class
    Copyright (C) 1998,1999 by Andrew Zabolotny <bit@eltech.ru>

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

#include "sysdef.h"
#include "csws/cstimer.h"
#include "csws/csapp.h"
#include "cssys/system.h"

csTimer::csTimer (csComponent *iParent, unsigned long iPeriod)
  : csComponent (iParent)
{
  state |= CSS_TRANSPARENT;
  timeout = iPeriod;
  pause = 0;
  Stopped = false;
  if (app)
    start = System->Time ();
}

bool csTimer::HandleEvent (csEvent &Event)
{
  if (!Stopped
   && (Event.Type == csevBroadcast)
   && (Event.Command.Code == cscmdPreProcess))
  {
    unsigned long current = System->Time ();
    unsigned long delta = current - start;
    if (pause >= delta)
      return false;
    pause = 0;
    if (delta >= timeout)
    {
      parent->SendCommand (cscmdTimerPulse, this);
      // if we're not too far behind, switch to next pulse
      // otherwise we'll have to jump far to the current time
      if (start + timeout >= current)
        start += timeout;
      else
        start = current;
    } /* endif */
  } /* endif */
  return false;
}

void csTimer::Stop ()
{
  Stopped = true;
}

void csTimer::Restart ()
{
  Stopped = false;
  start = System->Time ();
}

void csTimer::Pause (unsigned long iPause)
{
  pause = iPause;
  Restart ();
}
