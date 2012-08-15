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

#ifndef __CS_VFS_PARTIAL_VIEW_H__
#define __CS_VFS_PARTIAL_VIEW_H__

/**\file
 * Contains class for VFS partial view
 */

#include "csextern.h"

#include "iutil/vfs.h"
#include "csutil/csstring.h"

// Forward declarations
namespace CS
{
  namespace Memory
  {
    class iAllocator;
  }
}
class iDataBuffer;
class csVfsFileBase;

/**
 * Generic partial view class for VFS iFile.
 */
class CS_CRYSTALSPACE_EXPORT csVfsPartialView :
  public scfImplementation1<csVfsPartialView, iFile>
{
protected:
  /// Parent csVfsFileBase
  csRef<csVfsFileBase> parent;
  // Zero-based offset of parent csVfsFileBase
  uint64_t offset;
  /// Length of this view
  uint64_t size;
  /// Zero-based position of this view
  uint64_t pos;
  /// Last error status of this view
  int lastError;

public:
  /**
   * Construct partial view from given csVfsFileBase and parameters.
   * \param parent Pointer to parent csVfsFileBase object
   * \param offset Offset in number of bytes from the start of parent 
   *   csVfsFileBase
   * \param size   Size of this partial view
   * \remarks Actual size will be adjusted if it goes over parent csVfsFileBase
   *   boundary.
   */
  csVfsPartialView (csVfsFileBase *parent, uint64_t offset, uint64_t size);
  /// Destructor. Cleans up any resources that are no longer used.
  virtual ~csVfsPartialView ();
  /**
   * Query the name of this view.
   * \returns The C-string "#csVfsPartialView"
   * \remarks Override this method to change name in subclasses
   */
  virtual const char *GetName () { return "#csVfsPartialView"; }
  /// Query the size of this view.
  virtual uint64_t GetSize () { return size; }
  // Query and reset last error status
  virtual int GetStatus ();
  // Read Length bytes into the buffer at which Data points.
  virtual size_t Read (char *data, size_t length);
  // Write Length bytes from the buffer at which Data points.
  virtual size_t Write (const char *data, size_t length);
  // Flush stream
  virtual void Flush ();
  // Check whether pointer is at End of File
  virtual bool AtEOF ();
  // Query file pointer (absolute position)
  virtual uint64_t GetPos ();
  // Set file pointer (relative position; absolute by default)
  virtual bool SetPos (off64_t newPos, int relativeTo = VFS_POS_ABSOLUTE);
  // Get all data into a single buffer.
  virtual csPtr<iDataBuffer> GetAllData (bool nullTerminated = false);
  // Get all data into a single buffer with custom allocator.
  virtual csPtr<iDataBuffer> GetAllData (CS::Memory::iAllocator *allocator);
  // Get subset of file as iFile
  virtual csPtr<iFile> GetPartialView (uint64_t offset,
                                       uint64_t size = ~(uint64_t)0);
};

#endif
