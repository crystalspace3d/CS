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

#include "csutil/allocator.h"
#include "csutil/databuf.h"
#include "cstool/vfspartialview.h"
#include "cstool/vfsfilebase.h"

/* csVfsPartialView Constructor
 - takes parent (underlying iFile instance)
         offset (offset in parent iFile where this view starts)
         size   (size of this view)
 */ 
csVfsPartialView::csVfsPartialView (csVfsFileBase *parent,
                                    uint64_t offset,
                                    uint64_t size) :
  scfImplementationType (this, parent),
  parent (parent),
  offset (offset),
  size (size),
  pos (0),
  lastError (VFS_STATUS_OK)
{
  if (!parent)
  {
    // cannot create a view from null iFile
    lastError = VFS_STATUS_INVALIDARGS;
    return;
  }

  CS::Threading::RecursiveMutexScopedLock lock (parent->mutex);

  size_t parentSize = parent->GetSize ();

  // TODO: check boundary conditions more thoroughly
  // make sure offset is within [0, file size]
  if (offset > parentSize)
    offset = parentSize;

  // make sure offset+size is within [offset, file size]
  // this is safe since offset <= parent->size
  if (size > parentSize - offset)
    size = parentSize - offset;
}

// csVfsPartialView destructor
csVfsPartialView::~csVfsPartialView ()
{
  // Free memory and resources
  parent.Invalidate ();
}

// Reset and return last error status
int csVfsPartialView::GetStatus ()
{
  int status = lastError;
  lastError = VFS_STATUS_OK;
  return status;
}

// Read DataSize bytes from file
size_t csVfsPartialView::Read (char *data, size_t dataSize)
{
  CS::Threading::RecursiveMutexScopedLock lock (parent->mutex);
  // Save last error status
  int parentError = parent->lastError;
  // Save last position
  uint64_t lastPos = parent->GetPos ();

  // Apply offset of this view
  parent->SetPos (offset + pos);
  // Reset parent error so we can collect status info
  parent->lastError = VFS_STATUS_OK;
  // make sure read doesn't exceed view boundary
  size_t toRead = (size - pos < dataSize) ? size - pos : dataSize;

  // try reading...
  size_t bytesRead = Read (data, toRead);
  // update last error of this view..
  if (lastError == VFS_STATUS_OK)
    lastError = parent->lastError;

  // restore parent's last position
  parent->SetPos (lastPos);
  // restore parent's last error
  parent->lastError = parentError;

  return bytesRead;
}

class csVfsPartialView;
// Write DataSize bytes to file
size_t csVfsPartialView::Write (const char *data, size_t dataSize)
{
  // partial view is read-only; access denied.
  lastError = VFS_STATUS_ACCESSDENIED;
  return 0;
}

// Flush strem
void csVfsPartialView::Flush ()
{
  // you can't flush a read-only view
  // therefore do nothing.
}

// Check whether pointer is at End of File
bool csVfsPartialView::AtEOF ()
{
  return pos >= size;
}

// Query file pointer (absolute position)
uint64_t csVfsPartialView::GetPos ()
{
  return pos;
}

// Set file pointer (relative position; absolute by default)
bool csVfsPartialView::SetPos (off64_t newPos, int relativeTo)
{
  // is newPos negative (backwards)
  bool negative = newPos < 0;
  // take absolute value
  uint64_t distance = negative ? -newPos : newPos;
  // only virtual pointer is moved
  switch (relativeTo)
  {
    case VFS_POS_CURRENT:
      // relative to current position
      if (negative) // remember, this is unsigned arithmetic
        pos = (pos < distance) ? 0 : pos - distance;
      else
        pos += distance;
      break;
    case VFS_POS_END:
      // relative to end of view
      if (negative)
        pos = (size < distance) ? 0 : size - distance;
      else
        pos = size;
      break;
    case VFS_POS_ABSOLUTE:
      // absolute mode requested
      pos = ((uint64_t)newPos > size) ? size : newPos;
      break;
    default:
      // unknown constant
      return false;
  }

  return true;
}

// Get all data into a single buffer.
csPtr<iDataBuffer> csVfsPartialView::GetAllData (bool nullTerminated)
{
  // TODO: use private heap for allocation
  // it uses malloc () for now

  // Since client code doesn't know about buffer length, nullTerminated is
  // useless for now.

  char *data = (char *)cs_malloc(size + 1);
  if (!data)
  {
    // TODO: update error code
    return csPtr<iDataBuffer> (nullptr);
  }

  // backup pointer
  uint64_t oldPos = GetPos ();
  // set pointer at beginning
  SetPos (0);
  // read!
  size_t bytesRead = Read (data, size);
  data[bytesRead] = 0; // null-terminated.

  // TODO: add handling if bytesRead < size ?

  // restore pointer
  SetPos (oldPos);

  if (nullTerminated)
    ++bytesRead;

  return csPtr<iDataBuffer> (
    new CS::DataBuffer<CS::Memory::AllocatorMalloc> (data, bytesRead));
}

// Get all data into a single buffer with custom allocator.
// - takes pointer of iAllocator interface to be used for memory allocation
// - returns smart pointer to iDataBuffer containing requested data
csPtr<iDataBuffer> 
csVfsPartialView::GetAllData (CS::Memory::iAllocator *allocator)
{

  using CS::Memory::AllocatorInterface;

  // create iDataBuffer instance with custom allocator
  iDataBuffer *buffer =
    new CS::DataBuffer<AllocatorInterface> (size,
                                            AllocatorInterface (allocator));

  char *data = buffer->GetData();
  if (!data)
  {
    // TODO: update error code
    return csPtr<iDataBuffer> (nullptr);
  }

  // backup pointer
  uint64_t oldPos = GetPos ();
  // set pointer at beginning
  SetPos (0);
  // read!
  size_t bytesRead = Read (data, size);

  // TODO: add handling if bytesRead < size ?

  // restore pointer
  SetPos (oldPos);

  return csPtr<iDataBuffer> (buffer);
}

// Get subset of file as iFile
csPtr<iFile> csVfsPartialView::GetPartialView (uint64_t offset,
                                                uint64_t size)
{
  // Let the constructor handle everything
  return csPtr<iFile> (
    new csVfsPartialView (parent, this->offset + offset, size));
}


