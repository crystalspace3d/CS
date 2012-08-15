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

#ifndef __CS_VFS_FILE_BASE_H__
#define __CS_VFS_FILE_BASE_H__

/**\file
 * Contains definition for abstract VFS iFile with basic common features.
 */

#include "csextern.h"

#include "csutil/scf_implementation.h"
#include "iutil/vfs.h"
#include "csutil/threading/mutex.h"
#include "csutil/csstring.h"

// Forward declarations
class iDataBuffer;
class csVfsPartialView;

/**
 * Abstract iFile class with common functions implemented
 */
class CS_CRYSTALSPACE_EXPORT csVfsFileBase :
  public scfImplementation1<csVfsFileBase, iFile>
{
  friend class csVfsPartialView;

protected:
  /// Mutex that can be used for synchronization
  CS::Threading::RecursiveMutex mutex;
  /// Last error status of this view
  int lastError;

public:
  /// Constructor
  csVfsFileBase () :
    scfImplementationType (this),
    lastError (VFS_STATUS_OK)
  {
  }
  /// Destructor; does nothing
  virtual ~csVfsFileBase () { }
  /**
   * Query and reset last error status. Do not call this function with mutex
   * locked; calling this function with mutex already locked can result in
   * deadlock situation.
   */
  virtual int GetStatus ();
  // Get subset of file as iFile
  virtual csPtr<iFile> GetPartialView (uint64_t offset,
                                       uint64_t size = ~(uint64_t)0);
};

#endif
