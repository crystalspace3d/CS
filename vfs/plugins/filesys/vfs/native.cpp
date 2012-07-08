/*

    Crystal Space Virtual File System class
    Copyright (C) 1998,1999,2000 by Andrew Zabolotny <bit@eltech.ru>
    Copyright (C) 2012 by Eunsoo Roh <nes1209@hotmail.com>

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
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

// Win32 utime() workaround
#ifdef CS_PLATFORM_WIN32
#include <sys/utime.h>
#define utime(a,b) _utime(a,b)
#define utimbuf _utimbuf
#else
#include <utime.h>
#endif

#include "vfs.h"
#include "csutil/scf_implementation.h"
#include "csutil/sysfunc.h"
#include "csutil/syspath.h"
#include "csutil/util.h"
#include "csutil/platformfile.h"
#include "csutil/vfsplat.h"
#include "csutil/databuf.h"

// local helper functions
namespace
{
  // Replace occurences of 'search' characters in String to 'replace'
  char * ReplaceChar (char * string, char search, char replace)
  {
    if (!string)
      return nullptr;

    // replace all occurences; assume String is null-terminated.
    for (char *curr = string; curr != '\0'; ++curr)
      if (*curr == search)
        *curr = replace;

    return string;
  }
}


// this belongs to VFS namespace, for now.
CS_PLUGIN_NAMESPACE_BEGIN(VFS)
{

// OS Native iFileSystem
class NativeFS : public scfImplementation1<NativeFS, iFileSystem>
{
  // Real-world mountpoint of this instance
  char *mountRoot;
  // Last error status
  int lastError;
  // Allocator heap for this filesystem
  //csRef<HeapRefCounted> heap;

  // Construct a NativeFS instance with 'RealPath' as mount root.
  NativeFS (const char *realPath);
  // Convert relative virtual path to real path
  csString ToRealPath (const char *virtualPath);
  // Set last error to specified value
  bool SetLastError (int errorCode);
  // Update last error code from system call
  void UpdateError ();

public:

  // Destructor
  virtual ~NativeFS ();
  // Open a file of given path and return iFile pointer
  virtual csPtr<iFile> Open (const char *path,
                             const char *pathPrefix,
                             int mode,
                             bool useCaching);
  // Move file from OldPath to NewPath
  virtual bool Move (const char *oldPath, const char *newPath);
  // Get permission set of a file
  virtual bool GetPermission (const char *fileName, csFilePermission &oPerm);
  // Set permission set of a file
  virtual bool SetPermission (const char *fileName,
                              const csFilePermission &iPerm);
  // Set file time
  virtual bool GetTime (const char *fileName, csFileTime &oTime);
  // Get file time
  virtual bool SetTime (const char *fileName, const csFileTime &iTime);
  // Get file size
  virtual bool GetSize (const char *fileName, uint64_t &oSize);
  // Query and reset last error status
  virtual int GetStatus ();
};

// OS Native iFile
class NativeFile : public scfImplementation1<NativeFile, iFile>
{
  class View;

  friend class View;
  friend class NativeFS;

  // Fully-qualified VFS path of file
  char *virtualPath;
  // Real path
  char *realPath;
  // File size
  uint64_t size;
  // Last error
  int lastError;
  // Native file handle
  FILE *handle;
  // Read-only?
  bool readOnly;

  // Constructor
  NativeFile (NativeFS *parent,
              const char *virtualPath,
              const char *realPath,
              int mode);
  // Set last error to specified value
  bool SetLastError (int errorCode);
  // Update last error code from system call
  void UpdateError ();
public:
  // Destructor
  virtual ~NativeFile ();
  // Query VFS filename
  virtual const char *GetName () { return virtualPath; }
  // Query file size
  virtual uint64_t GetSize () { return size; }
  // Query and reset last error status
  virtual int GetStatus ();
  // Read Length bytes into the buffer at which Data points.
  virtual size_t Read (char *data, size_t length);
  // Write Length bytes from the buffer at which Data points.
  virtual size_t Write (const char *data, size_t length);
  // Flush strem
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

// View for NativeFile
class NativeFile::View : public scfImplementation1<View, iFile>
{
  friend class NativeFile;

  // parent NativeFile
  csRef<NativeFile> parent;
  // zero-based offset of parent NativeFile
  uint64_t offset;
  // length of this view
  uint64_t size;
  // zero-based position of this view
  uint64_t pos;
  // last error status of this view
  int lastError;

  // Constructor
  View (NativeFile *parent, uint64_t offset, uint64_t size);
public:
  // Destructor
  virtual ~View ();
  // Query filename
  virtual const char *GetName () { return "#View"; }
  // Query file size
  virtual uint64_t GetSize () { return size; }
  // Query and reset last error status
  virtual int GetStatus ();
  // Read Length bytes into the buffer at which Data points.
  virtual size_t Read (char *data, size_t length);
  // Write Length bytes from the buffer at which Data points.
  virtual size_t Write (const char *data, size_t length);
  // Flush strem
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


// NativeFile methods
// NativeFile Constructor
NativeFile::NativeFile (NativeFS *parent,
                        const char *virtualPath,
                        const char *realPath, int mode) :
 scfImplementationType (this)
{
  // TODO: Add debug messages

  // Measure required length
  size_t virtualPathLen  = strlen (virtualPath) + 1;
  size_t realPathLen = strlen (realPath) + 1;
  // Allocate buffer
  this->virtualPath  = (char *)cs_malloc (virtualPathLen);
  this->realPath = (char *)cs_malloc (realPathLen);
  // Copy string content
  memcpy (this->virtualPath,  virtualPath,  virtualPathLen);
  memcpy (this->realPath, realPath, realPathLen);

  bool retry = false;
  int fileMode = mode & VFS_FILE_MODE;
  readOnly = (fileMode == VFS_FILE_READ);
  do
  {
    // Open file handle with specified mode
    switch (fileMode)
    {
      case VFS_FILE_WRITE:
        // write only
        handle = CS::Platform::File::Open (realPath, "wb");
        break;
      case VFS_FILE_APPEND:
        // append mode
        handle = CS::Platform::File::Open (realPath, "ab");
        break;
      case VFS_FILE_READ:
      default:
        // read only or unknown mode
        handle = CS::Platform::File::Open (realPath, "rb");
        break;
    }

    // if this is a retry, exit the loop
    if (retry)
      break;

    if (!handle)
    {
      // somehow failed to open; try creating directory and try again
      if (fileMode != VFS_FILE_READ)
      {
        // TODO: create missing directory tree
        retry = true;
      }
    }
  } while (retry);

  // is this a regular file? (i.e. not fifo, etc.)
  if (handle && !CS::Platform::IsRegularFile (realPath))
  {
    // close the handle
    fclose (handle);
    handle = nullptr;
  }

  if (!handle)
  {
    // Somehow failed to open file
    // TODO: error handling code
  }
  else
  {
    // Store the size
    // TODO: implement 64-bit fail-proof handling
    if (fseek (handle, 0, SEEK_END) == 0)
    {
      // fseek succeeded
      size = ftell (handle);
      if (size == (uint64_t)-1)
      {
        // cannot possibly have this size
        size = 0;
        // TODO: handle errors
      }
    }
    else
      ; // TODO: handle errors

    // for read or write mode, reposition file pointer
    if (fileMode != VFS_FILE_APPEND)
      rewind (handle);
  }
}

// NativeFile destructor
NativeFile::~NativeFile ()
{
  // Free memory and resources
  cs_free(virtualPath);
  cs_free(realPath);
}

// Set last error to specified value
bool NativeFile::SetLastError (int errorCode)
{
  // If new status is VFS_STATUS_OK, do it anyway.
  // Otherwise, update error code if and only if previous error is cleared.
  if (errorCode == VFS_STATUS_OK || lastError == VFS_STATUS_OK)
  {
    lastError = errorCode;
    return true;
  }
  // previous error was not cleared
  return false;
}

// Update last error code from system call
void NativeFile::UpdateError ()
{
  // Update error code if and only if previous error is cleared.
  if (lastError == VFS_STATUS_OK)
    return;

  // Skip if handle is invalid
  if (!handle)
    return;

  switch (errno)
  {
  default:
    break;
  }
}

// Reset and return last error status
int NativeFile::GetStatus ()
{
  int status = lastError;
  lastError = VFS_STATUS_OK;
  return status;
}

// Read DataSize bytes from file
size_t NativeFile::Read (char *data, size_t dataSize)
{
  if (!readOnly)
  {
    // cannot read a file opened with write permission; thus access denied
    lastError = VFS_STATUS_ACCESSDENIED;
    return 0;
  }

  if (!handle)
  {
    // TODO: double check for handling this case
    return 0;
  }
  // try reading...
  size_t bytesRead = fread (data, 1, dataSize, handle);
  if (bytesRead < dataSize)
  {
    // read less... what happened?
    UpdateError ();
  }
  return bytesRead;
}

// Write DataSize bytes to file
size_t NativeFile::Write (const char *data, size_t dataSize)
{
  if (readOnly)
  {
    // cannot write to a read-only file: access denied.
    lastError = VFS_STATUS_ACCESSDENIED;
    return 0;
  }

  if (!handle)
  {
    // TODO: double check for handling this case
    return 0;
  }
  // try writing
  size_t bytesWritten = fwrite (data, 1, dataSize, handle);
  if (bytesWritten < dataSize)
  {
    // written less... what happened?
    UpdateError ();
  }
  return bytesWritten;
}

// Flush strem
void NativeFile::Flush ()
{
  if (handle)
    fflush (handle);
}

// Check whether pointer is at End of File
bool NativeFile::AtEOF ()
{
  // if feof returns nonzero, file is at EOF
  if (handle)
    return feof (handle) != 0;

  return true;
}

// Query file pointer (absolute position)
uint64_t NativeFile::GetPos ()
{
  // TODO: ensure large file compatibility
  if (handle)
    return ftell (handle);

  return (uint64_t)-1;
}

// Set file pointer (relative position; absolute by default)
bool NativeFile::SetPos (off64_t newPos, int relativeTo)
{
  // currently, null handle will simply fail...
  if (!handle)
    return false;

  int mode; // seek mode for fseek ()

  switch (relativeTo)
  {
    case VFS_POS_CURRENT:
      mode = SEEK_CUR;
      break;
    case VFS_POS_END:
      mode = SEEK_END;
      break;
    case VFS_POS_ABSOLUTE:
    default:
      // absolute mode requested, or unknown constant
      mode = SEEK_SET;
      break;
  }

  // TODO: ensure large file compatibility
  // try seek
  if (fseek (handle, newPos, mode) != 0)
  {
    UpdateError ();
    return false;
  }

  return true;
}

// Get all data into a single buffer.
csPtr<iDataBuffer> NativeFile::GetAllData (bool nullTerminated)
{
  // we need read permission... check it
  if (!readOnly)
  {
    // update error code
    lastError = VFS_STATUS_ACCESSDENIED;
    return csPtr<iDataBuffer> (nullptr);
  }

  // TODO: implement mmap () feature
  // TODO: check how csVFS::heap works and fix if necessary.
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

  // increment the size to include null-terminator if required
  if (nullTerminated)
    ++bytesRead;

  return csPtr<iDataBuffer> (
    new CS::DataBuffer<Memory::AllocatorMalloc> (data, bytesRead));
}

// Get all data into a single buffer with custom allocator.
csPtr<iDataBuffer> NativeFile::GetAllData (CS::Memory::iAllocator *allocator)
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
csPtr<iFile> NativeFile::GetPartialView (uint64_t offset, uint64_t size)
{
  // this doesn't really work with iFile not opened for read access
  if (!readOnly)
  {
    lastError = VFS_STATUS_ACCESSDENIED;
    return csPtr<iFile> (nullptr);
  }

  // TODO: check boundary conditions more thoroughly
  // make sure offset is within [0, file size]
  if (offset > this->size)
    offset = this->size;

  // make sure offset+size is within [offset, file size]
  // this is safe since offset <= this->size
  if (size > this->size - offset)
    size = this->size - offset;

  // only pass safe values to View constructor
  return csPtr<iFile> (new View (this, offset, size));
}

// NativeFile::View methods
// View Constructor
NativeFile::View::View (NativeFile *parent,
                        uint64_t offset,
                        uint64_t size) :
 scfImplementationType (this, parent),
 pos (0),
 lastError (VFS_STATUS_OK)
{
  // assign members as appropriate
  this->parent = parent;
  this->offset = offset;
  this->size   = size;
}

// NativeFile::View destructor
NativeFile::View::~View ()
{
  // Free memory and resources
  parent = 0;
}

// Reset and return last error status
int NativeFile::View::GetStatus ()
{
  int status = lastError;
  lastError = VFS_STATUS_OK;
  return status;
}

// Read DataSize bytes from file
size_t NativeFile::View::Read (char *data, size_t dataSize)
{
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
  lastError = parent->lastError;

  // restore last position
  parent->SetPos (lastPos);
  // restore last error
  parent->lastError = parentError;

  return bytesRead;
}

// Write DataSize bytes to file
size_t NativeFile::View::Write (const char *data, size_t dataSize)
{
  // partial view is read-only; access denied.
  lastError = VFS_STATUS_ACCESSDENIED;
  return 0;
}

// Flush strem
void NativeFile::View::Flush ()
{
  // you can't flush a read-only view
  // therefore do nothing.
}

// Check whether pointer is at End of File
bool NativeFile::View::AtEOF ()
{
  return pos >= size;
}

// Query file pointer (absolute position)
uint64_t NativeFile::View::GetPos ()
{
  return pos;
}

// Set file pointer (relative position; absolute by default)
bool NativeFile::View::SetPos (off64_t newPos, int relativeTo)
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
    default:
      // absolute mode requested, or unknown constant
      pos = ((uint64_t)newPos > size) ? size : newPos;
      break;
  }

  return true;
}

// Get all data into a single buffer.
csPtr<iDataBuffer> NativeFile::View::GetAllData (bool nullTerminated)
{
  // TODO: implement mmap () feature
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
    new CS::DataBuffer<Memory::AllocatorMalloc> (data, bytesRead));
}

// Get all data into a single buffer with custom allocator.
csPtr<iDataBuffer> 
NativeFile::View::GetAllData (CS::Memory::iAllocator *allocator)
{

  lastError = VFS_STATUS_NOTIMPLEMENTED;
  return csPtr<iDataBuffer> (nullptr);
}

// Get subset of file as iFile
csPtr<iFile> NativeFile::View::GetPartialView (uint64_t offset,
                                               uint64_t size)
{
  // TODO: check boundary conditions more thoroughly
  // make sure offset is within [0, current view size]
  if (offset > this->size)
    offset = this->size;

  // make sure offset+size is within [offset, current view size]
  // this is safe since offset <= this->size
  if (size > this->size - offset)
    size = this->size - offset;

  // only pass safe values to View constructor
  return csPtr<iFile> (new View (parent, this->offset + offset, size));
}

// NativeFS methods

// NativeFS constructor
NativeFS::NativeFS (const char *realPath) : scfImplementationType(this)
{
  // Store mount root (real path) of this instance
  size_t mountRootLen = strlen(realPath) + 1; // +1 for null-terminator
  mountRoot = (char *)cs_malloc(mountRootLen);
  memcpy(mountRoot, realPath, mountRootLen);
  // Set last error to VFS_STATUS_OK
  lastError = VFS_STATUS_OK;
}

// NativeFS destructor
NativeFS::~NativeFS ()
{
  // clean up resources
  cs_free(mountRoot);
}

// Convert virtual path to real path
csString NativeFS::ToRealPath (const char *virtualPath)
{
  // string constants to use
  static const char search[] = { VFS_PATH_SEPARATOR, 0 };
  static const char replace[] = { CS_PATH_SEPARATOR, 0 };

  csString path (mountRoot);
  csString suffix (virtualPath);
  // replace virtual path separators to platform specific ones
  suffix.ReplaceAll (search, replace);
  // append rest of the path to the base
  path << suffix;

  return path;
}

// Set last error status
bool NativeFS::SetLastError (int errorCode)
{
  // If new status is VFS_STATUS_OK, do it anyway.
  // Otherwise, update error code if and only if previous error is cleared.
  if (errorCode == VFS_STATUS_OK || lastError == VFS_STATUS_OK)
  {
    lastError = errorCode;
    return true;
  }
  // previous error was not cleared
  return false;
}

// Update error code from last system call
void NativeFS::UpdateError ()
{  
  // Update error code if and only if previous error is cleared.
  if (lastError == VFS_STATUS_OK)
    return;

  switch (errno)
  {
  default:
    break;
  }
}

// Open a particular file
csPtr<iFile> NativeFS::Open (const char *path,
                             const char *pathPrefix,
                             int mode,
                             bool useCaching)
{
  // TODO: implement checks for boundary cases


  // setup required parameters;
  csString realPath (ToRealPath (path));
  csString vfsPath (pathPrefix);

  // append VFS path separator if needed
  if ((vfsPath[vfsPath.Length ()-1] != VFS_PATH_SEPARATOR)
      && (path[0] != VFS_PATH_SEPARATOR))
    vfsPath << VFS_PATH_SEPARATOR;

  // append rest of the path to the base
  vfsPath << path;

  // try creating an instance
  iFile *file = new NativeFile (this, vfsPath, realPath, mode);

  // did it go well?
  switch (lastError = file->GetStatus ())
  {
    case VFS_STATUS_OK:
      // ok!
      break;
    default:
      // error happened; invalidate csPtr
      delete file;
      file = nullptr;
      break;
  }

  return csPtr<iFile> (file);
}

// Move file from OldPath to NewPath
bool NativeFS::Move (const char *oldPath, const char *newPath)
{
  // This only works within current file system.
  SetLastError (VFS_STATUS_NOTIMPLEMENTED);
  return false;
}

// Get permission set of a file
bool NativeFS::GetPermission (const char *fileName, csFilePermission &oPerm)
{
  csString path (ToRealPath (fileName));
  struct stat info;

#if defined(WIN32) && !defined(__CYGWIN__) && !defined(__CYGWIN32__)
  // TODO: implement Win32 emulated permission
  SetLastError (VFS_STATUS_UNSUPPORTED);
  return false;
#endif
  if (stat (path, &info) != 0)
  {
    // stat() failed... update error status
    UpdateError ();
    return false;
  }

  // Using obtained information, fill in the structure
  csFilePermission permission (info.st_mode);
  // done; copy contents
  oPerm = permission;

  return true;
}

// Set permission set of a file
bool NativeFS::SetPermission (const char *fileName,
                              const csFilePermission &iPerm)
{
  // TODO: implement feature
  SetLastError (VFS_STATUS_NOTIMPLEMENTED);
  return false;
}

// Set file time
bool NativeFS::GetTime (const char *fileName, csFileTime &oTime)
{
  csString path (ToRealPath (fileName));
  struct stat info;

  // retrieve file information with stat()
  int result = stat (path, &info);

  if (result != 0)
  {
    // stat() failed... update error status
    UpdateError ();
    return false;
  }

  // Using obtained information, fill in the structure
  const time_t mtime = info.st_mtime;
  struct tm *localtm = localtime (&mtime);
  oTime = *localtm;

  return true;
}

// Get file time
bool NativeFS::SetTime (const char *fileName, const csFileTime &iTime)
{
  // TODO: implement feature
  csString path (ToRealPath (fileName));
  struct tm curtm = iTime;
  struct utimbuf times;

  times.actime = mktime (&curtm); // access time
  times.modtime = times.actime;  // modification time

  // try updating time...
  int result = utime (path, &times);

  if (result != 0)
  {
    // operation failed.
    UpdateError ();
    return false;
  }
  return true;
}

// Get file size
bool NativeFS::GetSize (const char *fileName, uint64_t &oSize)
{
  csString path (ToRealPath (fileName));
  struct stat info;
  // TODO: ensure large file compatibility
  if (stat (path, &info) != 0)
  {
    // failure; update error code
    UpdateError ();
    return false;
  }
  // copy the information
  oSize = info.st_size;

  return true;
}

// Query and reset last error status
int NativeFS::GetStatus ()
{
  int status = lastError;
  lastError = VFS_STATUS_OK;
  return status;
}

}
CS_PLUGIN_NAMESPACE_END(VFS)
