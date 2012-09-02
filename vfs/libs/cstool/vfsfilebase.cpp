/*
    Copyright (C) 2012 by Eunsoo Roh (nes1209@hotmail.com)

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

#include "cstool/vfsfilebase.h"
#include "cstool/vfspartialview.h"

int csVfsFileBase::GetStatus ()
{
  CS::Threading::RecursiveMutexScopedLock lock (mutex);
  int error = lastError;
  lastError = VFS_STATUS_OK;
  return error;
}

csPtr<iFile> csVfsFileBase::GetPartialView (size64_t offset, size64_t size)
{
  // default implementation using csVfsPartialView
  return csPtr<iFile> (new csVfsPartialView (this, offset, size));
}
